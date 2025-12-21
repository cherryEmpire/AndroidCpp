// ============================================================================
// Looper.h
// ============================================================================
#pragma once
#include "MessageQueue.h"
#include <thread>
#include <memory>

class Looper {
private:
    static thread_local std::shared_ptr<Looper> sThreadLocal;
    static std::shared_ptr<Looper> sMainLooper;
    static std::mutex sMainLooperMutex;

    std::shared_ptr<MessageQueue> mQueue;
    std::thread::id mThread;
    std::atomic<bool> mInLoop{false};

public:
    // 构造函数公开，但通过 prepare() 调用
    Looper();

    static void prepare();
    static void prepareMainLooper();
    static std::shared_ptr<Looper> getMainLooper();
    static std::shared_ptr<Looper> myLooper();
    static std::shared_ptr<MessageQueue> myQueue();
    static void loop();

    void quit();
    void quitSafely();
    std::shared_ptr<MessageQueue> getQueue();
    bool isCurrentThread();
};