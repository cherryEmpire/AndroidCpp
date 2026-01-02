#include <vector>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <algorithm>

// CAN信号配置结构体
struct CanSignalConfig {
    std::string name;      // 信号名称
    uint32_t start_bit;    // 起始位 (0-based, LSB起始)
    uint32_t length;       // 信号长度 (位)
    int32_t offset;         // 偏移量
    float scale;          // 缩放因子
    bool is_signed;        // 是否为有符号数
    bool is_big_endian;    // 是否为大端字节序 (Motorola格式)
    bool is_little_endian; // 是否为小端字节序 (Intel格式)

    CanSignalConfig(const std::string &name = "",
                    uint32_t start_bit = 0,
                    uint32_t length = 0,
                    int32_t offset = 0,
                    float scale = 1.0,
                    bool is_signed = false,
                    bool is_big_endian = false)
            : name(name), start_bit(start_bit), length(length), offset(offset), scale(scale), is_signed(is_signed),
              is_big_endian(is_big_endian), is_little_endian(!is_big_endian) {}
};

// CAN信号值结构体
struct CanSignalValue {
    std::string name;
    double physical_value;  // 物理值
    uint64_t raw_value;     // 原始值

    CanSignalValue(const std::string &name = "",
                   double physical_value = 0.0,
                   uint64_t raw_value = 0)
            : name(name), physical_value(physical_value), raw_value(raw_value) {}
};

struct CanMsgInfo {
    std::string name;
    std::uint32_t id;
    std::vector<CanSignalConfig> configs;
    std::vector<uint8_t> data;
};


// CAN帧解析器类
class CanFrameDecoder {
private:
    std::unordered_map<std::string, CanSignalConfig> signalNameMap_;

    // 从CAN数据帧中提取原始值 (Motorola格式 - 大端字节序)
    uint64_t extractMotorolaValue(const std::vector<uint8_t> &data,
                                  uint32_t start_bit,
                                  uint32_t length) const;

    // 从CAN数据帧中提取原始值 (Intel格式 - 小端字节序)
    uint64_t extractIntelValue(const std::vector<uint8_t> &data,
                               uint32_t start_bit,
                               uint32_t length) const;

    // 提取原始值，根据字节序选择方法
    uint64_t extractRawValue(const std::vector<uint8_t> &data,
                             const CanSignalConfig &config) const;

    // 将原始值转换为有符号数
    int64_t toSignedValue(uint64_t raw_value, uint32_t length) const;

public:
    CanFrameDecoder(const std::vector<CanSignalConfig> &configs);

    // 获取信号配置
    const CanSignalConfig *getSignalConfig(const std::string &signal_name) const;

    // 解析单个信号
    CanSignalValue decodeSignal(const std::vector<uint8_t> &data,
                                const std::string &signal_name) const;

    // 解析整帧所有信号
    std::vector<CanSignalValue> decodeFrame(const std::vector<uint8_t> &data) const;


    // 获取已配置的信号数量
    size_t getSignalCount() const;

    // 打印所有信号配置
    void printSignalConfigs() const;
};

