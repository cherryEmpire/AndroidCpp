// ============================================================================
// HandlerThread.cpp
// ============================================================================
#include "HandlerThread.h"
#include <iostream>

HandlerThread::HandlerThread(const std::string& name) : mName(name) {}

HandlerThread::~HandlerThread() {
    if (mThread.joinable()) {
        quit();
        mThread.join();
    }
}

void HandlerThread::run() {
    Looper::prepare();

    {
        std::lock_guard<std::mutex> lock(mMutex);
        mLooper = Looper::myLooper();
        mLooperReady = true;
        mCondition.notify_all();
    }

    onLooperPrepared();

    Looper::loop();
}

void HandlerThread::onLooperPrepared() {
    // 子类可以重写
}

void HandlerThread::start() {
    mThread = std::thread(&HandlerThread::run, this);
}

std::shared_ptr<Looper> HandlerThread::getLooper() {
    if (!mThread.joinable()) {
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [this] { return mLooperReady; });
    return mLooper;
}

bool HandlerThread::quit() {
    auto looper = getLooper();
    if (looper != nullptr) {
        looper->quit();
        return true;
    }
    return false;
}

bool HandlerThread::quitSafely() {
    auto looper = getLooper();
    if (looper != nullptr) {
        looper->quitSafely();
        return true;
    }
    return false;
}

void HandlerThread::join() {
    if (mThread.joinable()) {
        mThread.join();
    }
}