#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <sstream>
#include <iomanip>

// Forward declarations
class StateMachine;
class State;

// Constants
constexpr bool HANDLED = true;
constexpr bool NOT_HANDLED = false;

// Message structure similar to Android Message
class Message {
public:
    int what;
    int arg1;
    int arg2;
    std::shared_ptr<void> obj;

    // Internal use
    std::chrono::system_clock::time_point when;

    Message() : what(0), arg1(0), arg2(0), obj(nullptr),
                when(std::chrono::system_clock::now()) {}

    Message(int w) : what(w), arg1(0), arg2(0), obj(nullptr),
                     when(std::chrono::system_clock::now()) {}

    Message(int w, std::shared_ptr<void> o) : what(w), arg1(0), arg2(0), obj(o),
                                              when(std::chrono::system_clock::now()) {}

    Message(int w, int a1, int a2) : what(w), arg1(a1), arg2(a2), obj(nullptr),
                                     when(std::chrono::system_clock::now()) {}

    Message(int w, int a1, int a2, std::shared_ptr<void> o)
            : what(w), arg1(a1), arg2(a2), obj(o),
              when(std::chrono::system_clock::now()) {}

    void copyFrom(const Message& other) {
        what = other.what;
        arg1 = other.arg1;
        arg2 = other.arg2;
        obj = other.obj;
        when = other.when;
    }
};

// IState interface
class IState {
public:
    virtual ~IState() = default;
    virtual std::string getName() const = 0;
};

// State base class
class State : public IState {
public:
    virtual ~State() = default;

    virtual void enter() {}
    virtual void exit() {}
    virtual bool processMessage(const Message& msg) = 0;

    virtual std::string getName() const {
        // Return class name by default
        return typeid(*this).name();
    }
};

// Log record structure
class LogRec {
private:
    std::chrono::system_clock::time_point mTime;
    int mWhat;
    std::string mInfo;
    State* mState;
    State* mOrgState;

public:
    LogRec(const Message& msg, const std::string& info, State* state, State* orgState)
            : mWhat(msg.what), mInfo(info), mState(state), mOrgState(orgState) {
        mTime = std::chrono::system_clock::now();
    }

    void update(const Message& msg, const std::string& info, State* state, State* orgState) {
        mTime = std::chrono::system_clock::now();
        mWhat = msg.what;
        mInfo = info;
        mState = state;
        mOrgState = orgState;
    }

    std::chrono::system_clock::time_point getTime() const { return mTime; }
    int getWhat() const { return mWhat; }
    std::string getInfo() const { return mInfo; }
    State* getState() const { return mState; }
    State* getOriginalState() const { return mOrgState; }

    std::string toString(StateMachine* sm) const;
};

// Log records container
class LogRecords {
private:
    static constexpr int DEFAULT_SIZE = 20;
    std::vector<LogRec> mLogRecords;
    int mMaxSize;
    int mOldestIndex;
    int mCount;
    mutable std::mutex mMutex;

public:
    LogRecords() : mMaxSize(DEFAULT_SIZE), mOldestIndex(0), mCount(0) {}

    void setSize(int maxSize) {
        std::lock_guard<std::mutex> lock(mMutex);
        mMaxSize = maxSize;
        mCount = 0;
        mLogRecords.clear();
    }

    int size() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mLogRecords.size();
    }

    int count() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mCount;
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(mMutex);
        mLogRecords.clear();
    }

    LogRec* get(int index) {
        std::lock_guard<std::mutex> lock(mMutex);
        int nextIndex = mOldestIndex + index;
        if (nextIndex >= mMaxSize) {
            nextIndex -= mMaxSize;
        }
        if (nextIndex >= static_cast<int>(mLogRecords.size())) {
            return nullptr;
        }
        return &mLogRecords[nextIndex];
    }

    void add(const Message& msg, const std::string& messageInfo, State* state, State* orgState) {
        std::lock_guard<std::mutex> lock(mMutex);
        mCount++;
        if (static_cast<int>(mLogRecords.size()) < mMaxSize) {
            mLogRecords.emplace_back(msg, messageInfo, state, orgState);
        } else {
            mLogRecords[mOldestIndex].update(msg, messageInfo, state, orgState);
            mOldestIndex++;
            if (mOldestIndex >= mMaxSize) {
                mOldestIndex = 0;
            }
        }
    }
};

// StateMachine class
class StateMachine {
private:
    static constexpr int SM_QUIT_CMD = -1;
    static constexpr int SM_INIT_CMD = -2;

    std::string mName;

    // SmHandler equivalent - internal message handler
    class SmHandler {
    private:
        bool mDbg;
        Message mMsg;
        LogRecords mLogRecords;
        bool mIsConstructionCompleted;

        // State info structure
        struct StateInfo {
            State* state;
            StateInfo* parentStateInfo;
            bool active;

            StateInfo() : state(nullptr), parentStateInfo(nullptr), active(false) {}

            std::string toString() const {
                std::stringstream ss;
                ss << "state=" << (state ? state->getName() : "null")
                   << ",active=" << active
                   << ",parent=" << (parentStateInfo && parentStateInfo->state
                                     ? parentStateInfo->state->getName() : "null");
                return ss.str();
            }
        };

        std::vector<StateInfo*> mStateStack;
        int mStateStackTopIndex;
        std::vector<StateInfo*> mTempStateStack;
        int mTempStateStackCount;

        // Special states
        class HaltingState : public State {
        private:
            StateMachine* mSm;
        public:
            HaltingState(StateMachine* sm) : mSm(sm) {}
            bool processMessage(const Message& msg) override {
                mSm->haltedProcessMessage(msg);
                return true;
            }
            std::string getName() const override { return "HaltingState"; }
        };

        class QuittingState : public State {
        public:
            bool processMessage(const Message& msg) override {
                return NOT_HANDLED;
            }
            std::string getName() const override { return "QuittingState"; }
        };

        HaltingState mHaltingState;
        QuittingState mQuittingState;
        StateMachine* mSm;

        std::map<State*, StateInfo*> mStateInfo;
        State* mInitialState;
        State* mDestState;
        std::vector<Message> mDeferredMessages;

        // Message queue
        std::queue<Message> mMessageQueue;
        std::mutex mQueueMutex;
        std::condition_variable mQueueCondition;
        bool mQuitting;
        std::thread mHandlerThread;

        // Internal methods
        void handleMessage(const Message& msg);
        void performTransitions();
        void cleanupAfterQuitting();
        void completeConstruction();
        void processMsg(const Message& msg);
        void invokeExitMethods(StateInfo* commonStateInfo);
        void invokeEnterMethods(int stateStackEnteringIndex);
        void moveDeferredMessageAtFrontOfQueue();
        int moveTempStateStackToStateStack();
        StateInfo* setupTempStateStackWithStatesToEnter(State* destState);
        void setupInitialStateStack();
        bool isQuit(const Message& msg);
        StateInfo* addState(State* state, State* parent);

        void messageLoop();

    public:
        SmHandler(StateMachine* sm);
        ~SmHandler();

        void setInitialState(State* initialState);
        void transitionTo(IState* destState);
        void deferMessage(const Message& msg);
        void quit();
        void quitNow();
        bool isDbg() const { return mDbg; }
        void setDbg(bool dbg) { mDbg = dbg; }

        void sendMessage(const Message& msg);
        void sendMessageDelayed(const Message& msg, long delayMillis);
        void sendMessageAtFrontOfQueue(const Message& msg);
        void removeMessages(int what);

        Message getCurrentMessage() const { return mMsg; }
        IState* getCurrentState();

        LogRecords& getLogRecords() { return mLogRecords; }

        friend class StateMachine;
    };

    std::unique_ptr<SmHandler> mSmHandler;

protected:
    // Protected methods for subclasses
    void addState(State* state, State* parent = nullptr);
    void setInitialState(State* initialState);
    void transitionTo(IState* destState);
    void transitionToHaltingState();
    void deferMessage(const Message& msg);

    Message getCurrentMessage();
    IState* getCurrentState();


    void sendMessage(int what, std::shared_ptr<void> obj);
    void sendMessage(const Message& msg);
    void sendMessageDelayed(int what, long delayMillis);
    void sendMessageDelayed(int what, std::shared_ptr<void> obj, long delayMillis);
    void sendMessageDelayed(const Message& msg, long delayMillis);
    void sendMessageAtFrontOfQueue(int what, std::shared_ptr<void> obj);
    void sendMessageAtFrontOfQueue(int what);
    void sendMessageAtFrontOfQueue(const Message& msg);
    void removeMessages(int what);


    void quitNow();

    Message obtainMessage();
    Message obtainMessage(int what);
    Message obtainMessage(int what, std::shared_ptr<void> obj);
    Message obtainMessage(int what, int arg1, int arg2);
    Message obtainMessage(int what, int arg1, int arg2, std::shared_ptr<void> obj);

    // Virtual methods for subclasses to override
    virtual void unhandledMessage(const Message& msg);
    virtual void haltedProcessMessage(const Message& msg);
    virtual void onHalting();
    virtual void onQuitting();
    virtual bool recordLogRec(const Message& msg);
    virtual std::string getLogRecString(const Message& msg);
    virtual std::string getWhatToString(int what);

public:
    explicit StateMachine(const std::string& name);
    virtual ~StateMachine();
    void sendMessage(int what);

    std::string getName() const { return mName; }
    void start();
    void quit();
    void setLogRecSize(int maxSize);
    int getLogRecSize();
    int getLogRecCount();
    LogRec* getLogRec(int index);

    void addLogRec(const std::string& str);
    void addLogRec(const std::string& str, State* state);

    bool isDbg();
    void setDbg(bool dbg);

    void dump(std::ostream& os);

    friend class LogRec;
};

#endif // STATE_MACHINE_H