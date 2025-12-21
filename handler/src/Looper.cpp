// ============================================================================
// Looper.cpp
// ============================================================================
#include "Looper.h"
#include "Handler.h"  // 完整定义
#include <iostream>

thread_local std::shared_ptr<Looper> Looper::sThreadLocal = nullptr;
std::shared_ptr<Looper> Looper::sMainLooper = nullptr;
std::mutex Looper::sMainLooperMutex;

Looper::Looper()
        : mQueue(std::make_shared<MessageQueue>()),
          mThread(std::this_thread::get_id()) {
}

void Looper::prepare() {
    if (sThreadLocal != nullptr) {
        throw std::runtime_error("Only one Looper may be created per thread");
    }
    sThreadLocal = std::make_shared<Looper>();
}

void Looper::prepareMainLooper() {
    prepare();
    std::lock_guard<std::mutex> lock(sMainLooperMutex);
    if (sMainLooper != nullptr) {
        throw std::runtime_error("The main Looper has already been prepared.");
    }
    sMainLooper = myLooper();
}

std::shared_ptr<Looper> Looper::getMainLooper() {
    std::lock_guard<std::mutex> lock(sMainLooperMutex);
    return sMainLooper;
}

std::shared_ptr<Looper> Looper::myLooper() {
    return sThreadLocal;
}

std::shared_ptr<MessageQueue> Looper::myQueue() {
    return myLooper()->mQueue;
}

void Looper::loop() {
    auto me = myLooper();
    if (me == nullptr) {
        throw std::runtime_error("No Looper; Looper.prepare() wasn't called on this thread.");
    }

    if (me->mInLoop) {
        std::cout << "Warning: Loop again would have the queued messages be executed "
                  << "before this one completed.\n";
    }

    me->mInLoop = true;

    std::cout << "Looper started on thread " << std::this_thread::get_id() << "\n";

    while (true) {
        auto msg = me->mQueue->next();

        if (msg == nullptr) {
            std::cout << "Looper exiting\n";
            return;
        }

        try {
            msg->target->dispatchMessage(msg);  // 现在 Handler 已完整定义
        } catch (const std::exception& e) {
            std::cerr << "Exception in message dispatch: " << e.what() << "\n";
        }

        msg->recycle();
    }
}

void Looper::quit() {
    mQueue->quit(false);
}

void Looper::quitSafely() {
    mQueue->quit(true);
}

std::shared_ptr<MessageQueue> Looper::getQueue() {
    return mQueue;
}

bool Looper::isCurrentThread() {
    return std::this_thread::get_id() == mThread;
}