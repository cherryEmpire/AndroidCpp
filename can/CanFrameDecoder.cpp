#include "CanFrameDecoder.h"
#include "ic_log.h"

#define RDA_LOG_TAG "[IC_SRV][" __FILE_NAME__ "]"

CanFrameDecoder::CanFrameDecoder(const std::vector<CanSignalConfig> &configs) {
    for (const auto &config : configs) {
        signalNameMap_.emplace(config.name, config);
    }
}

// 从CAN数据帧中提取原始值 (Motorola格式 - 大端字节序)
// DBC格式: start_bit是MSB在整个64位帧中的位置
uint64_t CanFrameDecoder::extractMotorolaValue(const std::vector<uint8_t> &data,
                                               uint32_t start_bit,
                                               uint32_t length) const {
    uint64_t value = 0;

    // 将start_bit转换为字节索引和位索引
    uint32_t start_byte = start_bit / 8;
    uint32_t start_bit_in_byte = start_bit % 8;

    // Motorola格式：从MSB向LSB读取，可能跨越多个字节
    int32_t current_bit_pos = start_bit;  // 使用有符号数以处理负值

    for (uint32_t i = 0; i < length; i++) {
        // 计算当前要读取的绝对位位置
        int32_t bit_index = current_bit_pos;

        // 在Motorola格式中，如果当前字节读完，需要移到下一个字节的MSB
        if (bit_index < 0 || bit_index / 8 >= static_cast<int32_t>(data.size())) {
            IC_LOG_ERROR("Bit index out of range in Motorola decoding");
            return value;
        }

        uint32_t byte_index = bit_index / 8;
        uint32_t bit_in_byte = bit_index % 8;

        // 提取该位的值
        uint8_t bit_value = (data[byte_index] >> bit_in_byte) & 0x01;

        // 将位值放入结果中（从MSB到LSB）
        value = (value << 1) | bit_value;

        // 移动到下一位
        // Motorola格式：在字节内向下，跨字节时到下一字节的bit 7
        if (bit_in_byte == 0) {
            // 当前字节读完，跳到下一个字节的最高位
            current_bit_pos = (byte_index + 1) * 8 + 7;
        } else {
            // 继续在当前字节内向下读
            current_bit_pos--;
        }
    }

    return value;
}

// 从CAN数据帧中提取原始值 (Intel格式 - 小端字节序)
// Intel格式: start_bit指向信号的LSB (最低有效位)
uint64_t CanFrameDecoder::extractIntelValue(const std::vector<uint8_t> &data,
                                            uint32_t start_bit,
                                            uint32_t length) const {
    uint64_t value = 0;
    uint32_t current_bit = start_bit;
    uint32_t bits_remaining = length;
    uint32_t shift = 0;

    while (bits_remaining > 0) {
        uint32_t byte_index = current_bit / 8;
        uint32_t bit_in_byte = current_bit % 8;

        if (byte_index >= data.size()) {
            IC_LOG_ERROR("Byte index out of range");
            return value;
        }

        // 当前字节中可用的位数
        uint32_t bits_in_current_byte = std::min(bits_remaining, 8U - bit_in_byte);

        // 提取当前部分的位
        uint8_t current_byte = data[byte_index];
        uint8_t mask = ((1U << bits_in_current_byte) - 1) << bit_in_byte;
        uint8_t bits_value = (current_byte & mask) >> bit_in_byte;

        // 将提取的位放入结果中
        value |= (static_cast<uint64_t>(bits_value) << shift);

        current_bit += bits_in_current_byte;
        bits_remaining -= bits_in_current_byte;
        shift += bits_in_current_byte;
    }

    return value;
}

// 提取原始值，根据字节序选择方法
uint64_t CanFrameDecoder::extractRawValue(const std::vector<uint8_t> &data,
                                          const CanSignalConfig &config) const {
    if (config.is_big_endian) {
        return extractMotorolaValue(data, config.start_bit, config.length);
    } else {
        return extractIntelValue(data, config.start_bit, config.length);
    }
}

// 将原始值转换为有符号数
int64_t CanFrameDecoder::toSignedValue(uint64_t raw_value, uint32_t length) const {
    // 如果最高位为1，表示负数
    if (raw_value & (1ULL << (length - 1))) {
        // 计算补码
        int64_t signed_value = static_cast<int64_t>(raw_value);
        // 扩展符号位
        signed_value |= ~((1ULL << length) - 1);
        return signed_value;
    }
    return static_cast<int64_t>(raw_value);
}

// 获取信号配置
const CanSignalConfig *CanFrameDecoder::getSignalConfig(const std::string &signal_name) const {
    auto it = signalNameMap_.find(signal_name);
    if (it != signalNameMap_.end()) {
        return &it->second;
    }
    return nullptr;
}

// 解析单个信号
CanSignalValue CanFrameDecoder::decodeSignal(const std::vector<uint8_t> &data,
                                             const std::string &signal_name) const {
    auto it = signalNameMap_.find(signal_name);
    if (it == signalNameMap_.end()) {
        IC_LOG_ERROR("Signal not found: %s", signal_name.c_str());
        CanSignalValue value;
        value.name = "";
        return value;
    }

    const CanSignalConfig &config = it->second;

    // 提取原始值
    uint64_t raw_value = extractRawValue(data, config);

    // 转换为物理值
    double physical_value = 0.0;

    if (config.is_signed) {
        // 有符号数处理
        int64_t signed_value = toSignedValue(raw_value, config.length);
        physical_value = static_cast<double>(signed_value) * config.scale + config.offset;
    } else {
        // 无符号数处理
        physical_value = static_cast<double>(raw_value) * config.scale + config.offset;
    }

    return CanSignalValue(config.name, physical_value, raw_value);
}

// 解析整帧所有信号
std::vector<CanSignalValue> CanFrameDecoder::decodeFrame(const std::vector<uint8_t> &data) const {
    std::vector<CanSignalValue> results;

    for (const auto &pair : signalNameMap_) {
        try {
            CanSignalValue value = decodeSignal(data, pair.first);
            results.push_back(value);
        } catch (const std::exception &e) {
            // 可以记录日志或处理异常
            IC_LOG_ERROR("Error decoding signal %s-%s", pair.first.c_str(), e.what());
        }
    }

    return results;
}

size_t CanFrameDecoder::getSignalCount() const {
    return signalNameMap_.size();
}