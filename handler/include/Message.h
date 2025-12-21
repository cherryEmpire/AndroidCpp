// ============================================================================
// Message.h
// ============================================================================
#pragma once
#include <functional>
#include <memory>
#include <cstdint>

class Handler;

class Message {
public:
    int what = 0;
    int arg1 = 0;
    int arg2 = 0;
    void* obj = nullptr;
    std::function<void()> callback = nullptr;
    Handler* target = nullptr;
    int64_t when = 0;
    int64_t taskId = 0;

    Message* next = nullptr;  // 原始指针，不拥有所有权

    Message() = default;
    ~Message() = default;

    // 禁止拷贝，只允许移动
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;
    Message(Message&&) = default;
    Message& operator=(Message&&) = default;

    static std::shared_ptr<Message> obtain();
    static std::shared_ptr<Message> obtain(Handler* h);
    static std::shared_ptr<Message> obtain(Handler* h, int what);

    void recycle();
};