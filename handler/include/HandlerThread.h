// ============================================================================
// HandlerThread.h
// ============================================================================
#pragma once
#include "Handler.h"
#include <string>
#include <thread>
#include <condition_variable>

class HandlerThread {
private:
    std::string mName;
    std::thread mThread;
    std::shared_ptr<Looper> mLooper;
    std::mutex mMutex;
    std::condition_variable mCondition;
    bool mLooperReady = false;

    void run();

protected:
    virtual void onLooperPrepared();

public:
    explicit HandlerThread(const std::string& name);
    ~HandlerThread();

    void start();
    std::shared_ptr<Looper> getLooper();
    bool quit();
    bool quitSafely();
    void join();
};