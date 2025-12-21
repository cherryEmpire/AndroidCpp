// ============================================================================
// Message.cpp
// ============================================================================
#include "Message.h"

std::shared_ptr<Message> Message::obtain() {
    return std::make_shared<Message>();
}

std::shared_ptr<Message> Message::obtain(Handler* h) {
    auto msg = obtain();
    msg->target = h;
    return msg;
}

std::shared_ptr<Message> Message::obtain(Handler* h, int what) {
    auto msg = obtain(h);
    msg->what = what;
    return msg;
}

void Message::recycle() {
    what = 0;
    arg1 = arg2 = 0;
    obj = nullptr;
    callback = nullptr;
    target = nullptr;
    when = 0;
    taskId = 0;
    next = nullptr;
}