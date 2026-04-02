#include "./hardware/opt3001.h"

bool isOPT3001Connected = false;  /**< 光传感器连接状态 */

// 检测I2C设备是否存在
bool _isOPT3001Present() {
    Wire.beginTransmission(OPT3001_I2C_ADDR);
    uint8_t error = Wire.endTransmission();
    return (error == 0);  // 0 = 成功
}

// 读取16位寄存器 (大端序)
uint16_t _readOPT3001Register(uint8_t reg) {
    Wire.beginTransmission(OPT3001_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(OPT3001_I2C_ADDR, (uint8_t)2);

    if (Wire.available() == 2) {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        return (msb << 8) | lsb;
    }
    return 0;
}

// 写入16位寄存器 (大端序)
void _writeOPT3001Register(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(OPT3001_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value >> 8);      // MSB first
    Wire.write(value & 0xFF);    // LSB second
    Wire.endTransmission();
}

// 将原始结果转换为 lux 值
float _convertToLux(uint16_t rawData) {
    // OPT3001 结果格式: [15:12] = 指数E, [11:0] = 结果R
    uint16_t exponent = (rawData >> 12) & 0x0F;
    uint16_t result = rawData & 0x0FFF;
    
    // 计算 lux = 0.01 * 2^E * R
    float lux = 0.01 * (1 << exponent) * result;
    return lux;
}

// 将 lux 值转换为原始结果格式
uint16_t _convertFromLux(float luxValue) {
    // 找到合适的指数E
    uint16_t exponent = 0;
    float mantissa = luxValue / 0.01;
    
    while (mantissa >= 4095.0 && exponent < 11) {
        mantissa /= 2.0;
        exponent++;
    }
    
    uint16_t result = (uint16_t)mantissa;
    if (result > 4095) result = 4095;
    
    return (exponent << 12) | result;
}

void readOPT3001Status() {
    if (!isOPT3001Connected) {
        LOG_ALS_WARN("OPT3001 not connected, cannot read status");
        return;
    }

    // 读取配置寄存器
    uint16_t config = _readOPT3001Register(REG_OPT_CONFIG);
    LOG_ALS_DEBUG("OPT3001 Configuration: 0x%04X", config);

    // 读取制造商ID
    uint16_t mfgId = _readOPT3001Register(REG_OPT_MANUFACTURER_ID);
    LOG_ALS_DEBUG("Manufacturer ID: 0x%04X (Expected: 0x5449)", mfgId);

    // 读取设备ID
    uint16_t devId = _readOPT3001Register(REG_OPT_DEVICE_ID);
    LOG_ALS_DEBUG("Device ID: 0x%04X (Expected: 0x3001)", devId);

    // 解析配置寄存器
    uint8_t rangeNumber = (config >> 12) & 0x0F;
    bool is800ms = (config & CONFIG_CT_800MS) != 0;
    uint8_t mode = (config >> 9) & 0x03;
    bool overflow = (config & CONFIG_OVF) != 0;
    bool convReady = (config & CONFIG_CRF) != 0;

    LOG_ALS_DEBUG("Range Number: %d", rangeNumber);
    LOG_ALS_DEBUG("Conversion Time: %s", is800ms ? "800ms" : "100ms");
    LOG_ALS_DEBUG("Mode: %s", 
        mode == 0 ? "Shutdown" : 
        mode == 1 ? "Single-shot" : 
        mode == 3 ? "Continuous" : "Reserved");
    LOG_ALS_DEBUG("Overflow: %s", overflow ? "Yes" : "No");
    LOG_ALS_DEBUG("Conversion Ready: %s", convReady ? "Yes" : "No");
}

void initOPT3001() {
    LOG_ALS_INFO("Starting OPT3001 initialization...");
    isOPT3001Connected = false;  // 初始化为未连接状态
    
    // I2C已经由BQ27421初始化，不需要再次初始化
    LOG_ALS_DEBUG("Using shared I2C bus (already initialized)");

    // 检查设备是否存在
    if (!_isOPT3001Present()) {
        LOG_ALS_ERROR("OPT3001 device not found on I2C bus (addr 0x%02X), skip init", OPT3001_I2C_ADDR);
        return;
    }
    LOG_ALS_DEBUG("OPT3001 device detected");

    // 验证设备ID
    if (!verifyOPT3001()) {
        LOG_ALS_ERROR("OPT3001 device verification failed");
        return;
    }

    // 配置为默认模式: 连续转换, 800ms转换时间, 自动量程
    configureOPT3001(CONFIG_DEFAULT);
    
    // 等待首次转换完成
    delay(850);  // 等待超过800ms
    
    // 初始化完成，标记设备已连接
    isOPT3001Connected = true;
    LOG_ALS_INFO("OPT3001 initialization completed, device ready");
    readOPT3001Status();  // 读取并输出状态信息
}

void configureOPT3001(uint16_t configValue) {
    _writeOPT3001Register(REG_OPT_CONFIG, configValue);
    delay(10);
    
    // 读回配置验证
    uint16_t readBack = _readOPT3001Register(REG_OPT_CONFIG);
    LOG_ALS_DEBUG("OPT3001 configured: wrote=0x%04X, readback=0x%04X", configValue, readBack);
    
    if (readBack != configValue) {
        LOG_ALS_WARN("Configuration mismatch! Expected 0x%04X but got 0x%04X", configValue, readBack);
    }
}

// ==================== 基础测量函数 ====================

// 读取光照强度 (单位: lux)
float readLux() {
    if (!isOPT3001Connected) {
        LOG_ALS_WARN("readLux() called but OPT3001 not connected");
        return 0.0;
    }
    
    uint16_t rawResult = _readOPT3001Register(REG_OPT_RESULT);
    float lux = _convertToLux(rawResult);
    
    LOG_ALS_VERBOSE("readLux(): raw=0x%04X, lux=%.2f", rawResult, lux);
    
    return lux;
}

// 读取原始转换结果寄存器值
uint16_t readRawResult() {
    if (!isOPT3001Connected) return 0;
    return _readOPT3001Register(REG_OPT_RESULT);
}

// 检查转换是否完成
bool isConversionReady() {
    if (!isOPT3001Connected) return false;
    
    uint16_t config = _readOPT3001Register(REG_OPT_CONFIG);
    return (config & CONFIG_CRF) != 0;
}

// 检查是否溢出
bool isOverflow() {
    if (!isOPT3001Connected) return false;
    
    uint16_t config = _readOPT3001Register(REG_OPT_CONFIG);
    return (config & CONFIG_OVF) != 0;
}

// ==================== 配置与状态函数 ====================

// 读取当前配置寄存器值
uint16_t readConfig() {
    if (!isOPT3001Connected) return 0;
    return _readOPT3001Register(REG_OPT_CONFIG);
}

// 设置低阈值 (单位: lux)
void setLowLimit(float luxValue) {
    if (!isOPT3001Connected) return;
    
    uint16_t limitValue = _convertFromLux(luxValue);
    _writeOPT3001Register(REG_OPT_LOW_LIMIT, limitValue);
    LOG_ALS_DEBUG("OPT3001 low limit set to %.2f lux (0x%04X)", luxValue, limitValue);
}

// 设置高阈值 (单位: lux)
void setHighLimit(float luxValue) {
    if (!isOPT3001Connected) return;
    
    uint16_t limitValue = _convertFromLux(luxValue);
    _writeOPT3001Register(REG_OPT_HIGH_LIMIT, limitValue);
    LOG_ALS_DEBUG("OPT3001 high limit set to %.2f lux (0x%04X)", luxValue, limitValue);
}

// 读取低阈值 (单位: lux)
float readLowLimit() {
    if (!isOPT3001Connected) return 0.0;
    
    uint16_t limitValue = _readOPT3001Register(REG_OPT_LOW_LIMIT);
    return _convertToLux(limitValue);
}

// 读取高阈值 (单位: lux)
float readHighLimit() {
    if (!isOPT3001Connected) return 0.0;
    
    uint16_t limitValue = _readOPT3001Register(REG_OPT_HIGH_LIMIT);
    return _convertToLux(limitValue);
}

// ==================== 设备识别函数 ====================

// 读取制造商ID
uint16_t readManufacturerID() {
    if (!isOPT3001Connected) return 0;
    return _readOPT3001Register(REG_OPT_MANUFACTURER_ID);
}

// 读取设备ID
uint16_t readDeviceID() {
    if (!isOPT3001Connected) return 0;
    return _readOPT3001Register(REG_OPT_DEVICE_ID);
}

// 验证设备是否为OPT3001
bool verifyOPT3001() {
    uint16_t mfgId = _readOPT3001Register(REG_OPT_MANUFACTURER_ID);
    uint16_t devId = _readOPT3001Register(REG_OPT_DEVICE_ID);
    
    bool isValid = (mfgId == OPT3001_MANUFACTURER_ID) && (devId == OPT3001_DEVICE_ID);
    
    if (!isValid) {
        LOG_ALS_ERROR("OPT3001 verification failed - MFG ID: 0x%04X (expected 0x%04X), DEV ID: 0x%04X (expected 0x%04X)",
                 mfgId, OPT3001_MANUFACTURER_ID, devId, OPT3001_DEVICE_ID);
    } else {
        LOG_ALS_DEBUG("OPT3001 verified successfully");
    }
    
    return isValid;
}

void readLightInfo() {
    if (!isOPT3001Connected) {
        LOG_ALS_WARN("OPT3001 not connected, skipping light info read");
        return;
    }
    
    // ==================== 基础测量 ====================
    float lux = readLux();
    uint16_t rawResult = readRawResult();
    bool convReady = isConversionReady();
    bool overflow = isOverflow();
    
    // ==================== 配置信息 ====================
    uint16_t config = readConfig();
    uint16_t mfgId = readManufacturerID();
    uint16_t devId = readDeviceID();
    
    // ==================== 阈值信息 ====================
    float lowLimit = readLowLimit();
    float highLimit = readHighLimit();
    
    // 解析配置
    uint8_t rangeNumber = (config >> 12) & 0x0F;
    bool is800ms = (config & CONFIG_CT_800MS) != 0;
    uint8_t mode = (config >> 9) & 0x03;
    bool flagHigh = (config & CONFIG_FH) != 0;
    bool flagLow = (config & CONFIG_FL) != 0;
    
    const char* modeStr;
    switch(mode) {
        case 0: modeStr = "关断模式"; break;
        case 1: modeStr = "单次转换"; break;
        case 3: modeStr = "连续转换"; break;
        default: modeStr = "保留"; break;
    }

    // ==================== 串口输出 ====================
    LOG_ALS_DEBUG("========== OPT3001 光传感器信息 ==========");
    
    LOG_ALS_DEBUG("--- 设备识别 ---");
    LOG_ALS_DEBUG("制造商ID: 0x%04X %s", mfgId, mfgId == OPT3001_MANUFACTURER_ID ? "(TI)" : "(未知)");
    LOG_ALS_DEBUG("设备ID: 0x%04X %s", devId, devId == OPT3001_DEVICE_ID ? "(OPT3001)" : "(未知)");
    
    LOG_ALS_DEBUG("--- 光照测量 ---");
    LOG_ALS_DEBUG("光照强度: %.2f lux", lux);
    LOG_ALS_DEBUG("原始结果: 0x%04X", rawResult);
    LOG_ALS_DEBUG("转换状态: %s", convReady ? "完成" : "进行中");
    LOG_ALS_DEBUG("溢出状态: %s", overflow ? "是" : "否");
    
    LOG_ALS_DEBUG("--- 配置信息 ---");
    LOG_ALS_DEBUG("配置寄存器: 0x%04X", config);
    LOG_ALS_DEBUG("量程编号: %d", rangeNumber);
    LOG_ALS_DEBUG("转换时间: %s", is800ms ? "800ms" : "100ms");
    LOG_ALS_DEBUG("工作模式: %s", modeStr);
    
    LOG_ALS_DEBUG("--- 阈值设置 ---");
    LOG_ALS_DEBUG("低阈值: %.2f lux %s", lowLimit, flagLow ? "[已触发]" : "");
    LOG_ALS_DEBUG("高阈值: %.2f lux %s", highLimit, flagHigh ? "[已触发]" : "");
    
    LOG_ALS_DEBUG("==========================================\n");
}
