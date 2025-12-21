// ============================================================================
// MessageQueue.cpp
// ============================================================================
#include "MessageQueue.h"
#include <chrono>
#include <iostream>

int64_t MessageQueue::now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void MessageQueue::trackMessage(std::shared_ptr<Message> msg) {
    if (msg) {
        mMessageOwners[msg.get()] = msg;
    }
}

void MessageQueue::untrackMessage(Message* msg) {
    if (msg) {
        mMessageOwners.erase(msg);
    }
}

std::shared_ptr<Message> MessageQueue::getOwner(Message* msg) {
    auto it = mMessageOwners.find(msg);
    if (it != mMessageOwners.end()) {
        return it->second;
    }
    return nullptr;
}

bool MessageQueue::enqueueMessage(std::shared_ptr<Message> msg, int64_t when) {
    if (msg->target == nullptr) {
        throw std::runtime_error("Message must have a target.");
    }

    std::unique_lock<std::mutex> lock(mMutex);

    if (mQuitting) {
        std::cout << "MessageQueue is quitting, message dropped\n";
        return false;
    }

    msg->when = when;
    msg->next = nullptr;

    // 追踪消息所有权
    trackMessage(msg);

    Message* p = mMessages.get();

    if (p == nullptr || when == 0 || when < p->when) {
        // 插入到队列头部
        msg->next = mMessages.get();
        mMessages = msg;
        mCondition.notify_one();
    } else {
        // 找到插入位置
        while (p != nullptr && p->next != nullptr && p->next->when <= when) {
            p = p->next;
        }
        msg->next = p->next;
        p->next = msg.get();
    }

    return true;
}

std::shared_ptr<Message> MessageQueue::next() {
    int64_t nextPollTimeoutMillis = 0;

    while (true) {
        std::unique_lock<std::mutex> lock(mMutex);

        if (mQuitting) {
            return nullptr;
        }

        int64_t nowTime = now();
        Message* msg = mMessages.get();

        if (msg != nullptr) {
            if (nowTime < msg->when) {
                nextPollTimeoutMillis = msg->when - nowTime;
            } else {
                // 取出消息头
                std::shared_ptr<Message> result = mMessages;

                // 更新队列头
                if (msg->next != nullptr) {
                    mMessages = getOwner(msg->next);
                } else {
                    mMessages.reset();
                }

                result->next = nullptr;
                untrackMessage(result.get());

                return result;
            }
        } else {
            nextPollTimeoutMillis = -1;
        }

        if (nextPollTimeoutMillis != 0) {
            if (nextPollTimeoutMillis > 0) {
                mCondition.wait_for(lock, std::chrono::milliseconds(nextPollTimeoutMillis));
            } else {
                mCondition.wait(lock);
            }
        }
    }
}

void MessageQueue::quit(bool safe) {
    std::unique_lock<std::mutex> lock(mMutex);

    if (mQuitting) {
        return;
    }

    mQuitting = true;

    if (!safe) {
        removeAllMessagesLocked();
    }

    mCondition.notify_one();
}

void MessageQueue::removeMessages(Handler* h, int what) {
    std::unique_lock<std::mutex> lock(mMutex);

    Message* p = mMessages.get();

    // 先处理头部
    while (p != nullptr && p->target == h && p->what == what) {
        auto nextOwner = getOwner(p->next);
        untrackMessage(p);
        mMessages = nextOwner;
        p = mMessages.get();
    }

    // 处理后续节点
    if (p != nullptr) {
        Message* n = p->next;
        while (n != nullptr) {
            if (n->target == h && n->what == what) {
                p->next = n->next;
                untrackMessage(n);
                n = p->next;
            } else {
                p = n;
                n = n->next;
            }
        }
    }
}

void MessageQueue::removeCallbacksAndMessages(Handler* h) {
    std::unique_lock<std::mutex> lock(mMutex);

    Message* p = mMessages.get();

    // 先处理头部
    while (p != nullptr && p->target == h) {
        auto nextOwner = getOwner(p->next);
        untrackMessage(p);
        mMessages = nextOwner;
        p = mMessages.get();
    }

    // 处理后续节点
    if (p != nullptr) {
        Message* n = p->next;
        while (n != nullptr) {
            if (n->target == h) {
                p->next = n->next;
                untrackMessage(n);
                n = p->next;
            } else {
                p = n;
                n = n->next;
            }
        }
    }
}

bool MessageQueue::removeTask(Handler* h, int64_t taskId) {
    std::unique_lock<std::mutex> lock(mMutex);

    Message* p = mMessages.get();

    // 检查头部
    if (p != nullptr && p->target == h && p->taskId == taskId) {
        auto nextOwner = getOwner(p->next);
        untrackMessage(p);
        mMessages = nextOwner;
        return true;
    }

    // 检查后续节点
    while (p != nullptr && p->next != nullptr) {
        if (p->next->target == h && p->next->taskId == taskId) {
            Message* victim = p->next;
            p->next = victim->next;
            untrackMessage(victim);
            return true;
        }
        p = p->next;
    }

    return false;
}

bool MessageQueue::hasTask(Handler* h, int64_t taskId) {
    std::unique_lock<std::mutex> lock(mMutex);

    Message* p = mMessages.get();
    while (p != nullptr) {
        if (p->target == h && p->taskId == taskId) {
            return true;
        }
        p = p->next;
    }
    return false;
}

void MessageQueue::removeAllMessagesLocked() {
    // 清空所有消息追踪
    mMessageOwners.clear();
    mMessages.reset();
}