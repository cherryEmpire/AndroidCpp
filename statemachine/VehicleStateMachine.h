#ifndef VEHICLE_STATE_MACHINE_H
#define VEHICLE_STATE_MACHINE_H

#include "StateMachine.h"
#include <iostream>

/**
 * Vehicle State Machine Design
 *
 * State Hierarchy:
 *        OnState (Power On)
 *           |
 *      +----+----+
 *      |         |
 *   ParkState  DrivingState
 *      |         |
 * ChargingState  +----+----+----+
 *                |    |    |
 *              ADS  Map  Dashboard
 *
 * State Transition Flow:
 * 1. Power On -> OnState
 * 2. Init Complete -> ParkState
 * 3. ParkState:
 *    - Plug Charger -> ChargingState
 *    - Shift from Park -> DrivingState
 * 4. ChargingState:
 *    - Unplug Charger -> ParkState
 * 5. DrivingState:
 *    - Shift to Park -> ParkState
 *    - Can switch between ADS/Map/Dashboard
 */

class VehicleStateMachine : public StateMachine {
public:
    // Message definitions
    enum MessageType {
        MSG_INIT_COMPLETE = 1,      // Initialization complete
        MSG_CHARGER_PLUGGED,        // Charger plugged in
        MSG_CHARGER_UNPLUGGED,      // Charger unplugged
        MSG_GEAR_TO_PARK,           // Shift to Park
        MSG_GEAR_FROM_PARK,         // Shift from Park (D/R/N)
        MSG_SWITCH_TO_ADS,          // Switch to ADS
        MSG_SWITCH_TO_MAP,          // Switch to Map
        MSG_SWITCH_TO_DASHBOARD,    // Switch to Dashboard
        MSG_SYSTEM_SHUTDOWN,        // System shutdown
    };

    // ==================== ON State (Power On) ====================
    class OnState : public State {
    private:
        VehicleStateMachine* mSm;

    public:
        OnState(VehicleStateMachine* sm) : mSm(sm) {}

        void enter() override {
            std::cout << "[OnState] System powered on, initializing..." << std::endl;
            std::cout << "[OnState] - Initialize hardware" << std::endl;
            std::cout << "[OnState] - Load configuration" << std::endl;
            std::cout << "[OnState] - Start services" << std::endl;

            // Simulate initialization, send complete message after 1 second
            mSm->sendMessageDelayed(MSG_INIT_COMPLETE, 1000);
        }

        bool processMessage(const Message& msg) override {
            switch (msg.what) {
                case MSG_INIT_COMPLETE:
                    std::cout << "[OnState] Initialization complete, transition to Park state" << std::endl;
                    mSm->transitionTo(mSm->mParkState);
                    return HANDLED;

                default:
                    return NOT_HANDLED;
            }
        }

        void exit() override {
            std::cout << "[OnState] Exit power on state" << std::endl;
        }

        std::string getName() const override { return "OnState"; }
    };

    // ==================== Park State ====================
    class ParkState : public State {
    private:
        VehicleStateMachine* mSm;

    public:
        ParkState(VehicleStateMachine* sm) : mSm(sm) {}

        void enter() override {
            std::cout << "[ParkState] Enter park state" << std::endl;
            std::cout << "[ParkState] - Display park UI" << std::endl;
            std::cout << "[ParkState] - Enable park features" << std::endl;
        }

        bool processMessage(const Message& msg) override {
            switch (msg.what) {
                case MSG_CHARGER_PLUGGED:
                    std::cout << "[ParkState] Charger detected, transition to charging state" << std::endl;
                    mSm->transitionTo(mSm->mChargingState);
                    return HANDLED;

                case MSG_GEAR_FROM_PARK:
                    std::cout << "[ParkState] Gear shifted from Park, transition to driving state" << std::endl;
                    mSm->transitionTo(mSm->mDashboardState); // Default to dashboard
                    return HANDLED;

                case MSG_SYSTEM_SHUTDOWN:
                    std::cout << "[ParkState] System shutdown" << std::endl;
                    mSm->transitionToHaltingState();
                    return HANDLED;

                default:
                    return NOT_HANDLED;
            }
        }

        void exit() override {
            std::cout << "[ParkState] Exit park state" << std::endl;
        }

        std::string getName() const override { return "ParkState"; }
    };

    // ==================== Charging State ====================
    class ChargingState : public State {
    private:
        VehicleStateMachine* mSm;

    public:
        ChargingState(VehicleStateMachine* sm) : mSm(sm) {}

        void enter() override {
            std::cout << "[ChargingState] Enter charging state" << std::endl;
            std::cout << "[ChargingState] - Display charging UI" << std::endl;
            std::cout << "[ChargingState] - Start charging" << std::endl;
        }

        bool processMessage(const Message& msg) override {
            switch (msg.what) {
                case MSG_CHARGER_UNPLUGGED:
                    std::cout << "[ChargingState] Charger unplugged, return to park state" << std::endl;
                    mSm->transitionTo(mSm->mParkState);
                    return HANDLED;

                case MSG_GEAR_FROM_PARK:
                    std::cout << "[ChargingState] Cannot shift gear while charging" << std::endl;
                    return HANDLED; // Block gear shift while charging

                default:
                    return NOT_HANDLED;
            }
        }

        void exit() override {
            std::cout << "[ChargingState] Exit charging state" << std::endl;
            std::cout << "[ChargingState] - Stop charging" << std::endl;
        }

        std::string getName() const override { return "ChargingState"; }
    };

    // ==================== Driving State (Parent) ====================
    class DrivingState : public State {
    private:
        VehicleStateMachine* mSm;

    public:
        DrivingState(VehicleStateMachine* sm) : mSm(sm) {}

        void enter() override {
            std::cout << "[DrivingState] Enter driving state" << std::endl;
            std::cout << "[DrivingState] - Enable driving features" << std::endl;
        }

        bool processMessage(const Message& msg) override {
            switch (msg.what) {
                case MSG_GEAR_TO_PARK:
                    std::cout << "[DrivingState] Shift to Park, return to park state" << std::endl;
                    mSm->transitionTo(mSm->mParkState);
                    return HANDLED;

                case MSG_CHARGER_PLUGGED:
                    std::cout << "[DrivingState] Cannot plug charger while driving" << std::endl;
                    return HANDLED; // Block charging while driving

                default:
                    return NOT_HANDLED; // Let child states handle
            }
        }

        void exit() override {
            std::cout << "[DrivingState] Exit driving state" << std::endl;
        }

        std::string getName() const override { return "DrivingState"; }
    };

    // ==================== ADS State (Advanced Driver Assistance) ====================
    class ADSState : public State {
    private:
        VehicleStateMachine* mSm;

    public:
        ADSState(VehicleStateMachine* sm) : mSm(sm) {}

        void enter() override {
            std::cout << "  [ADSState] Switch to ADS UI" << std::endl;
            std::cout << "  [ADSState] - Display lane lines" << std::endl;
            std::cout << "  [ADSState] - Display surrounding vehicles" << std::endl;
        }

        bool processMessage(const Message& msg) override {
            switch (msg.what) {
                case MSG_SWITCH_TO_MAP:
                    std::cout << "  [ADSState] Switch to Map" << std::endl;
                    mSm->transitionTo(mSm->mMapState);
                    return HANDLED;

                case MSG_SWITCH_TO_DASHBOARD:
                    std::cout << "  [ADSState] Switch to Dashboard" << std::endl;
                    mSm->transitionTo(mSm->mDashboardState);
                    return HANDLED;

                default:
                    return NOT_HANDLED; // Let parent handle
            }
        }

        void exit() override {
            std::cout << "  [ADSState] Exit ADS UI" << std::endl;
        }

        std::string getName() const override { return "ADSState"; }
    };

    // ==================== Map State ====================
    class MapState : public State {
    private:
        VehicleStateMachine* mSm;

    public:
        MapState(VehicleStateMachine* sm) : mSm(sm) {}

        void enter() override {
            std::cout << "  [MapState] Switch to Map UI" << std::endl;
            std::cout << "  [MapState] - Display navigation route" << std::endl;
            std::cout << "  [MapState] - Update real-time traffic" << std::endl;
        }

        bool processMessage(const Message& msg) override {
            switch (msg.what) {
                case MSG_SWITCH_TO_ADS:
                    std::cout << "  [MapState] Switch to ADS" << std::endl;
                    mSm->transitionTo(mSm->mADSState);
                    return HANDLED;

                case MSG_SWITCH_TO_DASHBOARD:
                    std::cout << "  [MapState] Switch to Dashboard" << std::endl;
                    mSm->transitionTo(mSm->mDashboardState);
                    return HANDLED;

                default:
                    return NOT_HANDLED;
            }
        }

        void exit() override {
            std::cout << "  [MapState] Exit Map UI" << std::endl;
        }

        std::string getName() const override { return "MapState"; }
    };

    // ==================== Dashboard State ====================
    class DashboardState : public State {
    private:
        VehicleStateMachine* mSm;

    public:
        DashboardState(VehicleStateMachine* sm) : mSm(sm) {}

        void enter() override {
            std::cout << "  [DashboardState] Switch to Dashboard UI" << std::endl;
            std::cout << "  [DashboardState] - Display speed" << std::endl;
            std::cout << "  [DashboardState] - Display battery/fuel level" << std::endl;
        }

        bool processMessage(const Message& msg) override {
            switch (msg.what) {
                case MSG_SWITCH_TO_ADS:
                    std::cout << "  [DashboardState] Switch to ADS" << std::endl;
                    mSm->transitionTo(mSm->mADSState);
                    return HANDLED;

                case MSG_SWITCH_TO_MAP:
                    std::cout << "  [DashboardState] Switch to Map" << std::endl;
                    mSm->transitionTo(mSm->mMapState);
                    return HANDLED;

                default:
                    return NOT_HANDLED;
            }
        }

        void exit() override {
            std::cout << "  [DashboardState] Exit Dashboard UI" << std::endl;
        }

        std::string getName() const override { return "DashboardState"; }
    };

    // State instances
    OnState* mOnState;
    ParkState* mParkState;
    ChargingState* mChargingState;
    DrivingState* mDrivingState;
    ADSState* mADSState;
    MapState* mMapState;
    DashboardState* mDashboardState;

public:
    VehicleStateMachine(const std::string& name) : StateMachine(name) {
        std::cout << "\n========== Vehicle State Machine Initialization ==========" << std::endl;

        // Create state instances
        mOnState = new OnState(this);
        mParkState = new ParkState(this);
        mChargingState = new ChargingState(this);
        mDrivingState = new DrivingState(this);
        mADSState = new ADSState(this);
        mMapState = new MapState(this);
        mDashboardState = new DashboardState(this);

        // Build state hierarchy
        addState(mOnState);
        addState(mParkState);
        addState(mDrivingState);
        addState(mChargingState, mParkState);
        addState(mADSState, mDrivingState);
        addState(mMapState, mDrivingState);
        addState(mDashboardState, mDrivingState);

        // Set initial state
        setInitialState(mOnState);

        std::cout << "State hierarchy built" << std::endl;
        std::cout << "======================================\n" << std::endl;
    }

    ~VehicleStateMachine() {
        delete mOnState;
        delete mParkState;
        delete mChargingState;
        delete mDrivingState;
        delete mADSState;
        delete mMapState;
        delete mDashboardState;
    }

    // Convenience methods: simulate various events
    void simulateInitComplete() {
        sendMessage(MSG_INIT_COMPLETE);
    }

    void simulatePlugCharger() {
        std::cout << "\n>>> User Action: Plug Charger <<<" << std::endl;
        sendMessage(MSG_CHARGER_PLUGGED);
    }

    void simulateUnplugCharger() {
        std::cout << "\n>>> User Action: Unplug Charger <<<" << std::endl;
        sendMessage(MSG_CHARGER_UNPLUGGED);
    }

    void simulateShiftToPark() {
        std::cout << "\n>>> User Action: Shift to Park <<<" << std::endl;
        sendMessage(MSG_GEAR_TO_PARK);
    }

    void simulateShiftFromPark() {
        std::cout << "\n>>> User Action: Shift to Drive <<<" << std::endl;
        sendMessage(MSG_GEAR_FROM_PARK);
    }

    void simulateSwitchToADS() {
        std::cout << "\n>>> User Action: Switch to ADS <<<" << std::endl;
        sendMessage(MSG_SWITCH_TO_ADS);
    }

    void simulateSwitchToMap() {
        std::cout << "\n>>> User Action: Switch to Map <<<" << std::endl;
        sendMessage(MSG_SWITCH_TO_MAP);
    }

    void simulateSwitchToDashboard() {
        std::cout << "\n>>> User Action: Switch to Dashboard <<<" << std::endl;
        sendMessage(MSG_SWITCH_TO_DASHBOARD);
    }

    void simulateShutdown() {
        std::cout << "\n>>> System Shutdown <<<" << std::endl;
        sendMessage(MSG_SYSTEM_SHUTDOWN);
    }

protected:
    std::string getWhatToString(int what) override {
        switch (what) {
            case MSG_INIT_COMPLETE: return "MSG_INIT_COMPLETE";
            case MSG_CHARGER_PLUGGED: return "MSG_CHARGER_PLUGGED";
            case MSG_CHARGER_UNPLUGGED: return "MSG_CHARGER_UNPLUGGED";
            case MSG_GEAR_TO_PARK: return "MSG_GEAR_TO_PARK";
            case MSG_GEAR_FROM_PARK: return "MSG_GEAR_FROM_PARK";
            case MSG_SWITCH_TO_ADS: return "MSG_SWITCH_TO_ADS";
            case MSG_SWITCH_TO_MAP: return "MSG_SWITCH_TO_MAP";
            case MSG_SWITCH_TO_DASHBOARD: return "MSG_SWITCH_TO_DASHBOARD";
            case MSG_SYSTEM_SHUTDOWN: return "MSG_SYSTEM_SHUTDOWN";
            default: return "";
        }
    }
};

#endif // VEHICLE_STATE_MACHINE_H