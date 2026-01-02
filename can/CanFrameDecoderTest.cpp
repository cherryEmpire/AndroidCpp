#include "CanFrameDecoder.h"
#include <iostream>
#include <fstream>
#include <memory>
#include "json.hpp"

using namespace nlohmann;
// 使用示例
int main() {
    std::ifstream file("D:\\work\\cpp_test\\can\\can.json");
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: ");
    }
    json j;
    file >> j;
    file.close();

    std::map<uint32_t, CanMsgInfo> msgMap{};

    for (auto d = j.begin(); d != j.end(); d++) {
        const std::string messageName = d.key();
        json msg_info = j[messageName];
        CanMsgInfo msg{};
        msg.name = msg_info["name"].get<std::string>();
        msg.id = msg_info["frame_id"].get<uint32_t>();
        json signals = msg_info["signals"];
        for (const auto& signal_json : signals) {
            CanSignalConfig signal_config{};
            signal_config.name = signal_json["name"].get<std::string>();
            signal_config.start_bit = signal_json["start"].get<uint32_t>();
            signal_config.length = signal_json["length"].get<uint32_t>();
            signal_config.offset = signal_json["offset"].get<int32_t>();
            signal_config.scale = signal_json["scale"].get<float>();
            signal_config.is_signed = false;
            signal_config.is_big_endian = true;
            msg.configs.push_back(signal_config);
        }
        msgMap[msg.id] = msg;
    }

//    auto decoder = std::make_shared<CanFrameDecoder>(msgMap[712].configs);
//    std::vector<uint8_t> can_frame = {0xB3, 0xFE, 0xFF, 0x00, 0x81, 0x00, 0x00, 0x00};
//    std::vector<CanSignalValue> all_signals = decoder->decodeFrame(can_frame);
//    for (const auto &signal : all_signals) {
//        std::cout << signal.name << ": " << signal.physical_value
//                  << " (Raw: 0x" << std::hex << signal.raw_value
//                  << std::dec << ")" << std::endl;
//    }

    auto decoder1 = std::make_shared<CanFrameDecoder>(msgMap[160].configs);
    std::vector<uint8_t> can_frame1 = {0x53, 0x52, 0x10, 0x00, 0xA0, 0x01, 0x00, 0x00};
    std::vector<CanSignalValue> all_signals1 = decoder1->decodeFrame(can_frame1);
    for (const auto &signal : all_signals1) {
        std::cout << signal.name << ": " << signal.physical_value
                  << " (Raw: 0x" << std::hex << signal.raw_value
                  << std::dec << ")" << std::endl;
    }

    return 0;
}