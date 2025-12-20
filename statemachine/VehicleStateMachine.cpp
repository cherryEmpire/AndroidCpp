#include "VehicleStateMachine.h"
#include <thread>
#include <chrono>
#include <iostream>

// Helper function: wait for state machine to process messages
void waitForProcessing(int milliseconds = 300) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Safely delete state machine
void safeDeleteStateMachine(VehicleStateMachine *vsm) {
    if (vsm) {
        vsm->quit();
        waitForProcessing(200);
        delete vsm;
    }
}

// Main function
int main() {
    std::cout << "\n+============================================================+" << std::endl;
    std::cout << "|         Vehicle State Machine Test Program                |" << std::endl;
    std::cout << "+============================================================+\n" << std::endl;

    std::cout << "State Hierarchy:" << std::endl;
    std::cout << "  OnState (Power On)" << std::endl;
    std::cout << "    +-- ParkState" << std::endl;
    std::cout << "    |     +-- ChargingState" << std::endl;
    std::cout << "    +-- DrivingState" << std::endl;
    std::cout << "          +-- ADSState" << std::endl;
    std::cout << "          +-- MapState" << std::endl;
    std::cout << "          +-- DashboardState" << std::endl;
    std::cout << std::endl;

    VehicleStateMachine *vsm = new VehicleStateMachine("VehicleSM");
    vsm->setDbg(false);
    vsm->start();

    waitForProcessing(1500);

    vsm->sendMessage(VehicleStateMachine::MessageType::MSG_INIT_COMPLETE);

    waitForProcessing(1000);

    vsm->sendMessage(VehicleStateMachine::MessageType::MSG_CHARGER_PLUGGED);

    waitForProcessing(1000);

    vsm->sendMessage(VehicleStateMachine::MessageType::MSG_CHARGER_UNPLUGGED);

    waitForProcessing(5000);
    std::cout << "\n+============================================================+" << std::endl;
    std::cout << "|              All Test Scenarios Complete!                 |" << std::endl;
    std::cout << "+============================================================+\n" << std::endl;
    safeDeleteStateMachine(vsm);
    return 0;
}