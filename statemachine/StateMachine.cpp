#include "StateMachine.h"
#include <iostream>
#include <algorithm>

// LogRec toString implementation
std::string LogRec::toString(StateMachine* sm) const {
    std::stringstream ss;

    auto time_t = std::chrono::system_clock::to_time_t(mTime);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            mTime.time_since_epoch()) % 1000;

    ss << "time=";
    ss << std::put_time(std::localtime(&time_t), "%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();

    ss << " state=" << (mState ? mState->getName() : "<null>");
    ss << " orgState=" << (mOrgState ? mOrgState->getName() : "<null>");
    ss << " what=";

    std::string whatStr = sm->getWhatToString(mWhat);
    if (whatStr.empty()) {
        ss << mWhat << "(0x" << std::hex << mWhat << std::dec << ")";
    } else {
        ss << whatStr;
    }

    if (!mInfo.empty()) {
        ss << " " << mInfo;
    }

    return ss.str();
}

// SmHandler implementation
StateMachine::SmHandler::SmHandler(StateMachine* sm)
        : mDbg(false), mIsConstructionCompleted(false), mStateStackTopIndex(-1),
          mTempStateStackCount(0), mHaltingState(sm), mSm(sm),
          mInitialState(nullptr), mDestState(nullptr), mQuitting(false) {

    addState(&mHaltingState, nullptr);
    addState(&mQuittingState, nullptr);

    // Start message loop thread
    mHandlerThread = std::thread(&SmHandler::messageLoop, this);
}

StateMachine::SmHandler::~SmHandler() {
    // Ensure quitting flag is set
    {
        std::lock_guard<std::mutex> lock(mQueueMutex);
        mQuitting = true;
    }
    mQueueCondition.notify_all();

    // Wait for thread to finish
    if (mHandlerThread.joinable()) {
        mHandlerThread.join();
    }

    // Clean up state info
    for (auto& pair : mStateInfo) {
        delete pair.second;
    }
}

void StateMachine::SmHandler::messageLoop() {
    while (true) {
        Message msg;
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            mQueueCondition.wait(lock, [this] {
                return !mMessageQueue.empty() || mQuitting;
            });

            if (mQuitting && mMessageQueue.empty()) {
                break;
            }

            if (!mMessageQueue.empty()) {
                msg = mMessageQueue.front();
                mMessageQueue.pop();
            } else {
                continue;
            }
        }

        handleMessage(msg);

        // Check if we should quit after handling message
        if (mQuitting) {
            break;
        }
    }
}

void StateMachine::SmHandler::handleMessage(const Message& msg) {
    if (mDbg) {
        std::cout << "handleMessage: E msg.what=" << msg.what << std::endl;
    }

    mMsg = msg;

    if (mIsConstructionCompleted) {
        processMsg(msg);
    } else if (!mIsConstructionCompleted && (msg.what == SM_INIT_CMD)) {
        mIsConstructionCompleted = true;
        invokeEnterMethods(0);
    } else {
        throw std::runtime_error("StateMachine.handleMessage: The start method not called");
    }

    performTransitions();

    if (mDbg) {
        std::cout << "handleMessage: X" << std::endl;
    }
}

void StateMachine::SmHandler::performTransitions() {
    State* destState = nullptr;
    while (mDestState != nullptr) {
        if (mDbg) {
            std::cout << "handleMessage: new destination call exit" << std::endl;
        }

        destState = mDestState;
        mDestState = nullptr;

        StateInfo* commonStateInfo = setupTempStateStackWithStatesToEnter(destState);
        invokeExitMethods(commonStateInfo);
        int stateStackEnteringIndex = moveTempStateStackToStateStack();
        invokeEnterMethods(stateStackEnteringIndex);

        moveDeferredMessageAtFrontOfQueue();
    }

    if (destState != nullptr) {
        if (destState == &mQuittingState) {
            mSm->onQuitting();
            cleanupAfterQuitting();
        } else if (destState == &mHaltingState) {
            mSm->onHalting();
        }
    }
}

void StateMachine::SmHandler::cleanupAfterQuitting() {
    {
        std::lock_guard<std::mutex> lock(mQueueMutex);
        mQuitting = true;
    }
    mQueueCondition.notify_all();

    mLogRecords.cleanup();

    mStateStack.clear();
    mTempStateStack.clear();

    for (auto& pair : mStateInfo) {
        delete pair.second;
    }
    mStateInfo.clear();

    mInitialState = nullptr;
    mDestState = nullptr;
    mDeferredMessages.clear();
}

void StateMachine::SmHandler::completeConstruction() {
    if (mDbg) {
        std::cout << "completeConstruction: E" << std::endl;
    }

    int maxDepth = 0;
    for (auto& pair : mStateInfo) {
        int depth = 0;
        for (StateInfo* i = pair.second; i != nullptr; depth++) {
            i = i->parentStateInfo;
        }
        if (maxDepth < depth) {
            maxDepth = depth;
        }
    }

    if (mDbg) {
        std::cout << "completeConstruction: maxDepth=" << maxDepth << std::endl;
    }

    mStateStack.resize(maxDepth);
    mTempStateStack.resize(maxDepth);
    setupInitialStateStack();

    Message initMsg(SM_INIT_CMD);
    sendMessageAtFrontOfQueue(initMsg);

    if (mDbg) {
        std::cout << "completeConstruction: X" << std::endl;
    }
}

void StateMachine::SmHandler::processMsg(const Message& msg) {
    StateInfo* curStateInfo = mStateStack[mStateStackTopIndex];
    if (mDbg) {
        std::cout << "processMsg: " << curStateInfo->state->getName() << std::endl;
    }

    if (isQuit(msg)) {
        transitionTo(&mQuittingState);
    } else {
        while (!curStateInfo->state->processMessage(msg)) {
            curStateInfo = curStateInfo->parentStateInfo;
            if (curStateInfo == nullptr) {
                mSm->unhandledMessage(msg);
                break;
            }
            if (mDbg) {
                std::cout << "processMsg: " << curStateInfo->state->getName() << std::endl;
            }
        }

        if (mSm->recordLogRec(msg)) {
            if (curStateInfo != nullptr) {
                State* orgState = mStateStack[mStateStackTopIndex]->state;
                mLogRecords.add(msg, mSm->getLogRecString(msg), curStateInfo->state, orgState);
            } else {
                mLogRecords.add(msg, mSm->getLogRecString(msg), nullptr, nullptr);
            }
        }
    }
}

void StateMachine::SmHandler::invokeExitMethods(StateInfo* commonStateInfo) {
    while ((mStateStackTopIndex >= 0) &&
           (mStateStack[mStateStackTopIndex] != commonStateInfo)) {
        State* curState = mStateStack[mStateStackTopIndex]->state;
        if (mDbg) {
            std::cout << "invokeExitMethods: " << curState->getName() << std::endl;
        }
        curState->exit();
        mStateStack[mStateStackTopIndex]->active = false;
        mStateStackTopIndex--;
    }
}

void StateMachine::SmHandler::invokeEnterMethods(int stateStackEnteringIndex) {
    for (int i = stateStackEnteringIndex; i <= mStateStackTopIndex; i++) {
        if (mDbg) {
            std::cout << "invokeEnterMethods: " << mStateStack[i]->state->getName() << std::endl;
        }
        mStateStack[i]->state->enter();
        mStateStack[i]->active = true;
    }
}

void StateMachine::SmHandler::moveDeferredMessageAtFrontOfQueue() {
    for (int i = mDeferredMessages.size() - 1; i >= 0; i--) {
        Message curMsg = mDeferredMessages[i];
        if (mDbg) {
            std::cout << "moveDeferredMessageAtFrontOfQueue; what=" << curMsg.what << std::endl;
        }
        sendMessageAtFrontOfQueue(curMsg);
    }
    mDeferredMessages.clear();
}

int StateMachine::SmHandler::moveTempStateStackToStateStack() {
    int startingIndex = mStateStackTopIndex + 1;
    int i = mTempStateStackCount - 1;
    int j = startingIndex;
    while (i >= 0) {
        if (mDbg) {
            std::cout << "moveTempStackToStateStack: i=" << i << ",j=" << j << std::endl;
        }
        mStateStack[j] = mTempStateStack[i];
        j++;
        i--;
    }

    mStateStackTopIndex = j - 1;
    if (mDbg) {
        std::cout << "moveTempStackToStateStack: X mStateStackTop="
                  << mStateStackTopIndex << ",startingIndex=" << startingIndex
                  << ",Top=" << mStateStack[mStateStackTopIndex]->state->getName() << std::endl;
    }
    return startingIndex;
}

StateMachine::SmHandler::StateInfo*
StateMachine::SmHandler::setupTempStateStackWithStatesToEnter(State* destState) {
    mTempStateStackCount = 0;
    StateInfo* curStateInfo = mStateInfo[destState];
    do {
        mTempStateStack[mTempStateStackCount++] = curStateInfo;
        curStateInfo = curStateInfo->parentStateInfo;
    } while ((curStateInfo != nullptr) && !curStateInfo->active);

    if (mDbg) {
        std::cout << "setupTempStateStackWithStatesToEnter: X mTempStateStackCount="
                  << mTempStateStackCount << ",curStateInfo: "
                  << (curStateInfo ? curStateInfo->toString() : "null") << std::endl;
    }
    return curStateInfo;
}

void StateMachine::SmHandler::setupInitialStateStack() {
    if (mDbg) {
        std::cout << "setupInitialStateStack: E mInitialState="
                  << mInitialState->getName() << std::endl;
    }

    StateInfo* curStateInfo = mStateInfo[mInitialState];
    for (mTempStateStackCount = 0; curStateInfo != nullptr; mTempStateStackCount++) {
        mTempStateStack[mTempStateStackCount] = curStateInfo;
        curStateInfo = curStateInfo->parentStateInfo;
    }

    mStateStackTopIndex = -1;
    moveTempStateStackToStateStack();
}

bool StateMachine::SmHandler::isQuit(const Message& msg) {
    return (msg.what == SM_QUIT_CMD);
}

StateMachine::SmHandler::StateInfo*
StateMachine::SmHandler::addState(State* state, State* parent) {
    if (mDbg) {
        std::cout << "addStateInternal: E state=" << state->getName()
                  << ",parent=" << (parent ? parent->getName() : "") << std::endl;
    }

    StateInfo* parentStateInfo = nullptr;
    if (parent != nullptr) {
        auto it = mStateInfo.find(parent);
        if (it == mStateInfo.end()) {
            parentStateInfo = addState(parent, nullptr);
        } else {
            parentStateInfo = it->second;
        }
    }

    StateInfo* stateInfo = nullptr;
    auto it = mStateInfo.find(state);
    if (it == mStateInfo.end()) {
        stateInfo = new StateInfo();
        mStateInfo[state] = stateInfo;
    } else {
        stateInfo = it->second;
    }

    if ((stateInfo->parentStateInfo != nullptr) &&
        (stateInfo->parentStateInfo != parentStateInfo)) {
        throw std::runtime_error("state already added");
    }

    stateInfo->state = state;
    stateInfo->parentStateInfo = parentStateInfo;
    stateInfo->active = false;

    if (mDbg) {
        std::cout << "addStateInternal: X stateInfo: " << stateInfo->toString() << std::endl;
    }

    return stateInfo;
}

void StateMachine::SmHandler::setInitialState(State* initialState) {
    if (mDbg) {
        std::cout << "setInitialState: initialState=" << initialState->getName() << std::endl;
    }
    mInitialState = initialState;
}

void StateMachine::SmHandler::transitionTo(IState* destState) {
    mDestState = dynamic_cast<State*>(destState);
    if (mDbg) {
        std::cout << "transitionTo: destState=" << mDestState->getName() << std::endl;
    }
}

void StateMachine::SmHandler::deferMessage(const Message& msg) {
    if (mDbg) {
        std::cout << "deferMessage: msg=" << msg.what << std::endl;
    }

    Message newMsg;
    newMsg.copyFrom(msg);
    mDeferredMessages.push_back(newMsg);
}

void StateMachine::SmHandler::quit() {
    if (mDbg) {
        std::cout << "quit:" << std::endl;
    }
    Message msg(SM_QUIT_CMD);
    sendMessage(msg);
}

void StateMachine::SmHandler::quitNow() {
    if (mDbg) {
        std::cout << "quitNow:" << std::endl;
    }
    Message msg(SM_QUIT_CMD);
    sendMessageAtFrontOfQueue(msg);
}

void StateMachine::SmHandler::sendMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(mQueueMutex);
    if (!mQuitting) {
        mMessageQueue.push(msg);
        mQueueCondition.notify_one();
    }
}

void StateMachine::SmHandler::sendMessageDelayed(const Message& msg, long delayMillis) {
    // For simplicity, implementing basic delay using thread
    std::thread([this, msg, delayMillis]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMillis));
        this->sendMessage(msg);
    }).detach();
}

void StateMachine::SmHandler::sendMessageAtFrontOfQueue(const Message& msg) {
    std::lock_guard<std::mutex> lock(mQueueMutex);
    if (!mQuitting) {
        std::queue<Message> tempQueue;
        tempQueue.push(msg);
        while (!mMessageQueue.empty()) {
            tempQueue.push(mMessageQueue.front());
            mMessageQueue.pop();
        }
        mMessageQueue = tempQueue;
        mQueueCondition.notify_one();
    }
}

void StateMachine::SmHandler::removeMessages(int what) {
    std::lock_guard<std::mutex> lock(mQueueMutex);
    std::queue<Message> tempQueue;
    while (!mMessageQueue.empty()) {
        Message msg = mMessageQueue.front();
        mMessageQueue.pop();
        if (msg.what != what) {
            tempQueue.push(msg);
        }
    }
    mMessageQueue = tempQueue;
}

IState* StateMachine::SmHandler::getCurrentState() {
    if (mStateStackTopIndex >= 0 && mStateStackTopIndex < static_cast<int>(mStateStack.size())) {
        return mStateStack[mStateStackTopIndex]->state;
    }
    return nullptr;
}

// StateMachine implementation
StateMachine::StateMachine(const std::string& name) : mName(name) {
    mSmHandler = std::make_unique<SmHandler>(this);
}

StateMachine::~StateMachine() {
    if (mSmHandler) {
        mSmHandler->quit();
        // Give time for thread to finish
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void StateMachine::start() {
    if (mSmHandler) {
        mSmHandler->completeConstruction();
    }
}

void StateMachine::addState(State* state, State* parent) {
    mSmHandler->addState(state, parent);
}

void StateMachine::setInitialState(State* initialState) {
    mSmHandler->setInitialState(initialState);
}

void StateMachine::transitionTo(IState* destState) {
    mSmHandler->transitionTo(destState);
}

void StateMachine::transitionToHaltingState() {
    mSmHandler->transitionTo(&mSmHandler->mHaltingState);
}

void StateMachine::deferMessage(const Message& msg) {
    mSmHandler->deferMessage(msg);
}

Message StateMachine::getCurrentMessage() {
    return mSmHandler->getCurrentMessage();
}

IState* StateMachine::getCurrentState() {
    return mSmHandler->getCurrentState();
}

void StateMachine::sendMessage(int what) {
    if (!mSmHandler) return;
    mSmHandler->sendMessage(obtainMessage(what));
}

void StateMachine::sendMessage(int what, std::shared_ptr<void> obj) {
    if (!mSmHandler) return;
    mSmHandler->sendMessage(obtainMessage(what, obj));
}

void StateMachine::sendMessage(const Message& msg) {
    if (!mSmHandler) return;
    mSmHandler->sendMessage(msg);
}

void StateMachine::sendMessageDelayed(int what, long delayMillis) {
    if (!mSmHandler) return;
    mSmHandler->sendMessageDelayed(obtainMessage(what), delayMillis);
}

void StateMachine::sendMessageDelayed(int what, std::shared_ptr<void> obj, long delayMillis) {
    if (!mSmHandler) return;
    mSmHandler->sendMessageDelayed(obtainMessage(what, obj), delayMillis);
}

void StateMachine::sendMessageDelayed(const Message& msg, long delayMillis) {
    if (!mSmHandler) return;
    mSmHandler->sendMessageDelayed(msg, delayMillis);
}

void StateMachine::sendMessageAtFrontOfQueue(int what, std::shared_ptr<void> obj) {
    mSmHandler->sendMessageAtFrontOfQueue(obtainMessage(what, obj));
}

void StateMachine::sendMessageAtFrontOfQueue(int what) {
    mSmHandler->sendMessageAtFrontOfQueue(obtainMessage(what));
}

void StateMachine::sendMessageAtFrontOfQueue(const Message& msg) {
    mSmHandler->sendMessageAtFrontOfQueue(msg);
}

void StateMachine::removeMessages(int what) {
    mSmHandler->removeMessages(what);
}

void StateMachine::quit() {
    if (!mSmHandler) return;
    mSmHandler->quit();
}

void StateMachine::quitNow() {
    if (!mSmHandler) return;
    mSmHandler->quitNow();
}

Message StateMachine::obtainMessage() {
    return Message();
}

Message StateMachine::obtainMessage(int what) {
    return Message(what);
}

Message StateMachine::obtainMessage(int what, std::shared_ptr<void> obj) {
    return Message(what, obj);
}

Message StateMachine::obtainMessage(int what, int arg1, int arg2) {
    return Message(what, arg1, arg2);
}

Message StateMachine::obtainMessage(int what, int arg1, int arg2, std::shared_ptr<void> obj) {
    return Message(what, arg1, arg2, obj);
}

void StateMachine::unhandledMessage(const Message& msg) {
    if (mSmHandler && mSmHandler->isDbg()) {
        std::cerr << mName << " - unhandledMessage: msg.what=" << msg.what << std::endl;
    }
}

void StateMachine::haltedProcessMessage(const Message& msg) {
    // Empty default implementation
}

void StateMachine::onHalting() {
    // Empty default implementation
}

void StateMachine::onQuitting() {
    // Empty default implementation
}

bool StateMachine::recordLogRec(const Message& msg) {
    return true;
}

std::string StateMachine::getLogRecString(const Message& msg) {
    return "";
}

std::string StateMachine::getWhatToString(int what) {
    return "";
}

void StateMachine::setLogRecSize(int maxSize) {
    mSmHandler->getLogRecords().setSize(maxSize);
}

int StateMachine::getLogRecSize() {
    return mSmHandler->getLogRecords().size();
}

int StateMachine::getLogRecCount() {
    return mSmHandler->getLogRecords().count();
}

LogRec* StateMachine::getLogRec(int index) {
    return mSmHandler->getLogRecords().get(index);
}

void StateMachine::addLogRec(const std::string& str) {
    Message emptyMsg;
    mSmHandler->getLogRecords().add(emptyMsg, str, nullptr, nullptr);
}

void StateMachine::addLogRec(const std::string& str, State* state) {
    Message emptyMsg;
    mSmHandler->getLogRecords().add(emptyMsg, str, state, nullptr);
}

bool StateMachine::isDbg() {
    if (!mSmHandler) return false;
    return mSmHandler->isDbg();
}

void StateMachine::setDbg(bool dbg) {
    if (!mSmHandler) return;
    mSmHandler->setDbg(dbg);
}

void StateMachine::dump(std::ostream& os) {
    os << getName() << ":" << std::endl;
    os << " total records=" << getLogRecCount() << std::endl;
    for (int i = 0; i < getLogRecSize(); i++) {
        LogRec* rec = getLogRec(i);
        if (rec) {
            os << " rec[" << i << "]: " << rec->toString(this) << std::endl;
        }
    }
    IState* curState = getCurrentState();
    if (curState) {
        os << "curState=" << curState->getName() << std::endl;
    }
}