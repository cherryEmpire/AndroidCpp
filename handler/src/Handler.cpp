// ============================================================================
// Handler.cpp
// ============================================================================
#include "Handler.h"
#include <chrono>
#include <stdexcept>

int64_t Handler::now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

int64_t Handler::generateTaskId() {
    return mNextTaskId.fetch_add(1, std::memory_order_relaxed);
}

Handler::Handler() : Handler(Looper::myLooper()) {}

Handler::Handler(std::shared_ptr<Looper> looper) {
    if (looper == nullptr) {
        throw std::runtime_error("Can't create handler inside thread that has not called Looper.prepare()");
    }
    mLooper = looper;
    mQueue = looper->getQueue();
}

void Handler::handleMessage(std::shared_ptr<Message> msg) {
    // 默认空实现
}

void Handler::dispatchMessage(std::shared_ptr<Message> msg) {
    if (msg->callback) {
        msg->callback();
    } else {
        handleMessage(msg);
    }
}

int64_t Handler::sendMessage(std::shared_ptr<Message> msg) {
    return sendMessageDelayed(msg, 0);
}

int64_t Handler::sendEmptyMessage(int what) {
    return sendEmptyMessageDelayed(what, 0);
}

int64_t Handler::sendEmptyMessageDelayed(int what, int64_t delayMillis) {
    auto msg = Message::obtain(this);
    msg->what = what;
    return sendMessageDelayed(msg, delayMillis);
}

int64_t Handler::sendMessageDelayed(std::shared_ptr<Message> msg, int64_t delayMillis) {
    if (delayMillis < 0) {
        delayMillis = 0;
    }
    return sendMessageAtTime(msg, now() + delayMillis);
}

int64_t Handler::sendMessageAtTime(std::shared_ptr<Message> msg, int64_t uptimeMillis) {
    msg->target = this;
    msg->taskId = generateTaskId();
    int64_t taskId = msg->taskId;

    if (mQueue->enqueueMessage(msg, uptimeMillis)) {
        return taskId;
    }
    return 0;
}

int64_t Handler::post(std::function<void()> r) {
    return postDelayed(r, 0);
}

int64_t Handler::postDelayed(std::function<void()> r, int64_t delayMillis) {
    auto msg = Message::obtain(this);
    msg->callback = r;
    return sendMessageDelayed(msg, delayMillis);
}

int64_t Handler::postAtTime(std::function<void()> r, int64_t uptimeMillis) {
    auto msg = Message::obtain(this);
    msg->callback = r;
    return sendMessageAtTime(msg, uptimeMillis);
}

void Handler::removeMessages(int what) {
    mQueue->removeMessages(this, what);
}

void Handler::removeCallbacksAndMessages() {
    mQueue->removeCallbacksAndMessages(this);
}

bool Handler::removeTask(int64_t taskId) {
    return mQueue->removeTask(this, taskId);
}

bool Handler::hasTask(int64_t taskId) {
    return mQueue->hasTask(this, taskId);
}

std::shared_ptr<Looper> Handler::getLooper() {
    return mLooper;
}