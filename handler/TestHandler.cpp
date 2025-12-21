// ============================================================================
// main.cpp - 使用示例
// ============================================================================
#include "Handler.h"
#include "HandlerThread.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

//class MyHandler : public Handler {
//public:
//    using Handler::Handler;
//
//    void handleMessage(std::shared_ptr<Message> msg) override {
//        std::cout << "[Thread " << std::this_thread::get_id() << "] "
//                  << "Received message: what=" << msg->what
//                  << ", arg1=" << msg->arg1 << "\n";
//    }
//};

std::shared_ptr<Handler> GetMainHandler() {
    std::cout << "\n--- Example 2: Task Management ---\n";
    Looper::prepareMainLooper();
    return std::make_shared<Handler>();
};

int main() {
    std::cout << "=== C++ Message Queue Demo ===\n\n";
    std::cout << "[MainThread " << std::this_thread::get_id() << "] " << "Runnable executed\n";
    auto mainHandler = GetMainHandler();
    HandlerThread thread("WorkerThread");
    thread.start();
    auto workerHandler = std::make_shared<Handler>(thread.getLooper());

    int64_t task1 = workerHandler->sendEmptyMessage(1);
    int64_t task2 = workerHandler->sendEmptyMessageDelayed(2, 1000);
    std::cout << "Posted task1 id: " << task1 << "\n";
    std::cout << "Posted task2 id: " << task2 << "\n";

    int64_t task3 = workerHandler->post([]() {
        std::cout << "[Thread " << std::this_thread::get_id() << "] "
                  << "Runnable executed\n";
    });
    std::cout << "Posted task3 id: " << task3 << "\n";

    int64_t task4 = workerHandler->postDelayed([]() {
        std::cout << "[Delayed Task] This should not execute (will be removed)\n";
    }, 2000);
    std::cout << "Posted task4 id: " << task4 << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\nRemoving task4...\n";
    if (workerHandler->removeTask(task4)) {
        std::cout << "Task4 removed successfully\n";
    }

    std::cout << "Task2 exists: " << (workerHandler->hasTask(task2) ? "yes" : "no") << "\n";
    std::cout << "Task4 exists: " << (workerHandler->hasTask(task4) ? "yes" : "no") << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(2));
    thread.quitSafely();
    thread.join();

    // 示例2: 主线程 Looper


    std::vector<int64_t> taskIds;
    for (int i = 0; i < 5; i++) {
        int64_t taskId = mainHandler->postDelayed([i]() {
            std::cout << "[mainHandler Thread " << std::this_thread::get_id() << "] " << "Runnable executed\n";
            std::cout << "Task " << i << " executed\n";
        }, i * 100);
        taskIds.push_back(taskId);
        std::cout << "Posted task " << i << " with id: " << taskId << "\n";
    }

    std::cout << "\nRemoving task 2 and task 4...\n";
    mainHandler->removeTask(taskIds[2]);
    mainHandler->removeTask(taskIds[4]);

    mainHandler->postDelayed([]() {
        std::cout << "Exiting main looper\n";
        Looper::myLooper()->quit();
    }, 10000);

    Looper::loop();

    std::cout << "\nAll done!\n";
    return 0;
}