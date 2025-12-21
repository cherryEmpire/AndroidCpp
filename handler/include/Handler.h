// ============================================================================
// Handler.h
// ============================================================================
#pragma once
#include "Looper.h"
#include <atomic>

class Handler {
private:
    std::shared_ptr<Looper> mLooper;
    std::shared_ptr<MessageQueue> mQueue;
    std::atomic<int64_t> mNextTaskId{1};

    int64_t now();
    int64_t generateTaskId();

public:
    Handler();
    explicit Handler(std::shared_ptr<Looper> looper);
    virtual ~Handler() = default;

    virtual void handleMessage(std::shared_ptr<Message> msg);
    void dispatchMessage(std::shared_ptr<Message> msg);

    // 发送消息
    int64_t sendMessage(std::shared_ptr<Message> msg);
    int64_t sendEmptyMessage(int what);
    int64_t sendEmptyMessageDelayed(int what, int64_t delayMillis);
    int64_t sendMessageDelayed(std::shared_ptr<Message> msg, int64_t delayMillis);
    int64_t sendMessageAtTime(std::shared_ptr<Message> msg, int64_t uptimeMillis);

    // Post 任务
    int64_t post(std::function<void()> r);
    int64_t postDelayed(std::function<void()> r, int64_t delayMillis);
    int64_t postAtTime(std::function<void()> r, int64_t uptimeMillis);

    // 移除任务
    void removeMessages(int what);
    void removeCallbacksAndMessages();
    bool removeTask(int64_t taskId);
    bool hasTask(int64_t taskId);

    std::shared_ptr<Looper> getLooper();
};