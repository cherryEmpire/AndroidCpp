// ============================================================================
// MessageQueue.h
// ============================================================================
#pragma once
#include "Message.h"
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>

class MessageQueue {
private:
    std::shared_ptr<Message> mMessages;
    std::mutex mMutex;
    std::condition_variable mCondition;
    std::atomic<bool> mQuitting{false};

    // 用于管理链表中所有消息的所有权
    std::map<Message*, std::shared_ptr<Message>> mMessageOwners;

    int64_t now();
    void removeAllMessagesLocked();
    void trackMessage(std::shared_ptr<Message> msg);
    void untrackMessage(Message* msg);
    std::shared_ptr<Message> getOwner(Message* msg);

public:
    MessageQueue() = default;

    bool enqueueMessage(std::shared_ptr<Message> msg, int64_t when);
    std::shared_ptr<Message> next();
    void quit(bool safe);
    void removeMessages(Handler* h, int what);
    void removeCallbacksAndMessages(Handler* h);
    bool removeTask(Handler* h, int64_t taskId);
    bool hasTask(Handler* h, int64_t taskId);
};