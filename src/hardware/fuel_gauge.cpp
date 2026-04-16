#include "./hardware/fuel_gauge.h"

bool isfuelICConnected = false;  /**< 燃料计芯片连接状态 */

// 检测I2C设备是否存在（带重试，防止上电/唤醒时I2C总线未稳定导致伪ACK）
bool _isBQ27421Present() {
    for (int attempt = 0; attempt < 3; attempt++) {
        Wire.beginTransmission(BQ27421_I2C_ADDR);
        uint8_t error = Wire.endTransmission();
        if (error == 0) return true;  // 0 = 成功
        delay(5);
    }
    return false;
}

// 读取16位寄存器 (小端序)
uint16_t _readBQ27421Register(uint8_t reg) {
    Wire.beginTransmission(BQ27421_I2C_ADDR);
    Wire.write(reg);
    uint8_t error = Wire.endTransmission(false);
    if (error != 0) {
        LOG_BATTERY_WARN("I2C transmission error: %d", error);
        return 0;
    }
    
    Wire.requestFrom(BQ27421_I2C_ADDR, (uint8_t)2);
    if (Wire.available() == 2) {
        uint8_t lsb = Wire.read();
        uint8_t msb = Wire.read();
        return (msb << 8) | lsb;
    }
    return 0;
}

// 单字节读取函数(用于BlockData区域)
uint8_t _readBQ27421Byte(uint8_t reg) {
    Wire.beginTransmission(BQ27421_I2C_ADDR);
    Wire.write(reg);
    uint8_t error = Wire.endTransmission(false);
    if (error != 0) {
        return 0;
    }
    
    Wire.requestFrom(BQ27421_I2C_ADDR, (uint8_t)1);
    if (Wire.available() == 1) {
        return Wire.read();
    }
    return 0;
}

// 写入16位寄存器 (小端序)
void _writeBQ27421Register(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(BQ27421_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value & 0xFF);        // LSB first
    Wire.write(value >> 8);          // MSB second
    Wire.endTransmission();
}

// 单字节写入函数(用于BlockData区域)
void _writeBQ27421Byte(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(BQ27421_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void readFuelGaugeStatus() {
    if (!isfuelICConnected) {
        LOG_BATTERY_WARN("Fuel gauge IC not connected, cannot read status");
        return;
    }

    // 读取 CONTROL_STATUS
    _writeBQ27421Register(REG_CONTROL, CMD_CONTROL_STATUS);
    delay(5);
    uint16_t controlStatus = _readBQ27421Register(REG_CONTROL);
    LOG_BATTERY_DEBUG("CONTROL_STATUS: 0x%04X", controlStatus);

    // 读取 DEVICE_TYPE
    _writeBQ27421Register(REG_CONTROL, CMD_DEVICE_TYPE);
    delay(5);
    uint16_t deviceType = _readBQ27421Register(REG_CONTROL);
    LOG_BATTERY_DEBUG("DEVICE_TYPE: 0x%04X", deviceType);

    // 读取 FW_VERSION
    _writeBQ27421Register(REG_CONTROL, CMD_FW_VERSION);
    delay(5);
    uint16_t fwVersion = _readBQ27421Register(REG_CONTROL);
    LOG_BATTERY_DEBUG("FW_VERSION: 0x%04X", fwVersion);

    // 读取 DM_CODE
    _writeBQ27421Register(REG_CONTROL, CMD_DM_CODE);
    delay(5);
    uint16_t dmCode = _readBQ27421Register(REG_CONTROL);
    LOG_BATTERY_DEBUG("DM_CODE: 0x%04X", dmCode);

    // 读取 CHEM_ID
    _writeBQ27421Register(REG_CONTROL, CMD_CHEM_ID);
    delay(5);
    uint16_t chemId = _readBQ27421Register(REG_CONTROL);
    LOG_BATTERY_DEBUG("CHEM_ID: 0x%04X", chemId);
}

void initBQ27421(uint16_t designCapacity_mAh) {
    LOG_BATTERY_INFO("Starting BQ27421 initialization...");
    isfuelICConnected = false;  // 初始化为未连接状态
    
    // 尝试初始化I2C（如果已初始化则不会重复）
    // 注意：如果OPT3001已经初始化了I2C，这里会返回false但I2C仍然可用
    Wire.begin(SDA_PIN, SCL_PIN);
    delay(10);  // 等待I2C稳定
    LOG_BATTERY_DEBUG("I2C bus ready for BQ27421");

    // 检查设备是否存在
    if (!_isBQ27421Present()) {
        LOG_BATTERY_ERROR("BQ27421 device not found on I2C bus (addr 0x%02X), skip init", BQ27421_I2C_ADDR);
        return;
    }
    LOG_BATTERY_DEBUG("BQ27421 device detected");

    // 额外验证 DEVICE_TYPE，防止伪ACK误判（BQ27421应返回 0x0421）
    _writeBQ27421Register(REG_CONTROL, CMD_DEVICE_TYPE);
    delay(5);
    uint16_t deviceType = _readBQ27421Register(REG_CONTROL);
    if (deviceType != 0x0421) {
        LOG_BATTERY_ERROR("BQ27421 DEVICE_TYPE mismatch: 0x%04X (expected 0x0421), skip init", deviceType);
        return;
    }
    LOG_BATTERY_DEBUG("BQ27421 DEVICE_TYPE verified: 0x%04X", deviceType);

    // 检查 ITPOR 标志位
    uint16_t flags = _readBQ27421Register(REG_FLAGS);
    if (flags & FLAG_ITPOR) {
        LOG_BATTERY_WARN("ITPOR flag is set. Configuration update required.");
        // 需要初始化配置
        setBQ27421DesignCapacity(designCapacity_mAh);
    } else {
        LOG_BATTERY_INFO("ITPOR flag is not set. Configuration is up-to-date.");
        // 配置已存在，直接标记为已连接
    }
    
    // 初始化完成，标记设备已连接
    isfuelICConnected = true;
    LOG_BATTERY_INFO("BQ27421 initialization completed, device ready");
    readFuelGaugeStatus();  // 读取并输出状态信息
}

void setBQ27421DesignCapacity(uint16_t designCapacity_mAh) {
    // 参数验证
    if (designCapacity_mAh == 0 || designCapacity_mAh > 5000) {
        LOG_BATTERY_ERROR("Invalid design capacity: %d mAh", designCapacity_mAh);
        return;
    }

    // 1. 解封芯片 (Unseal)
    LOG_BATTERY_DEBUG("Unsealing BQ27421...");
    _writeBQ27421Register(REG_CONTROL, CMD_UNSEAL);
    delay(10);
    _writeBQ27421Register(REG_CONTROL, CMD_UNSEAL);
    delay(10);

    // 2. 进入配置更新模式
    LOG_BATTERY_DEBUG("Entering CONFIG UPDATE mode...");
    _writeBQ27421Register(REG_CONTROL, CMD_SET_CFGUPDATE);
    delay(50);  // 等待芯片响应CONFIG UPDATE命令
    
    // 等待进入配置更新模式 (检查CFGUPMODE标志)
    uint16_t timeout = 0;
    while (!(_readBQ27421Register(REG_FLAGS) & FLAG_CFGUPMODE)) {
        delay(10);
        if (timeout++ > 100) {  // 1秒超时
            LOG_BATTERY_ERROR("Timeout entering CONFIG UPDATE mode");
            return;
        }
    }
    LOG_BATTERY_DEBUG("Entered CONFIG UPDATE mode");

    // 3. 访问 State of Health 数据块
    _writeBQ27421Byte(REG_BLOCK_DATA_CTRL, 0x00);  // 启用块数据访问
    delay(1);
    _writeBQ27421Byte(REG_DATA_CLASS, DATA_CLASS_STATE);  // 选择数据类82
    delay(1);
    _writeBQ27421Byte(REG_DATA_BLOCK, 0x00);  // 选择数据块0
    delay(10);  // 等待数据加载

    // 4. 读取原始 BlockData (32字节)
    uint8_t blockData[32];
    for (int i = 0; i < 32; i++) {
        blockData[i] = _readBQ27421Byte(REG_BLOCK_DATA + i);
    }
    
    // 读取旧校验和用于调试
    uint8_t oldChecksum = _readBQ27421Byte(REG_CHECKSUM);
    LOG_BATTERY_DEBUG("Old checksum: 0x%02X", oldChecksum);

    // 5. 更新参数
    // 设定健康100% (Qmax Cell)
    blockData[OFFSET_QMAX_CELL] = (uint8_t)(16384 >> 8);  // 设置最大化学容量为100% (比例系数)
    blockData[OFFSET_QMAX_CELL + 1] = (uint8_t)(16384 & 0xFF);
    LOG_BATTERY_DEBUG("Qmax Cell set to 100%% (16384)");
    
    // 设定学习模式
    blockData[OFFSET_UPDATE_STATUS] = 0x03;  // 设置学习模式位
    LOG_BATTERY_DEBUG("Set learning mode in Update Status");

    // 设计容量 (mAh) - Big Endian
    blockData[OFFSET_DESIGN_CAPACITY] = (uint8_t)(designCapacity_mAh >> 8);
    blockData[OFFSET_DESIGN_CAPACITY + 1] = (uint8_t)(designCapacity_mAh & 0xFF);
    LOG_BATTERY_DEBUG("Design Capacity: %d mAh", designCapacity_mAh);
    
    // 设计能量 (mWh) = 容量 * 3.7V (使用整数运算避免浮点)
    uint16_t designEnergy = (designCapacity_mAh * 37) / 10;
    blockData[OFFSET_DESIGN_ENERGY] = (uint8_t)(designEnergy >> 8);
    blockData[OFFSET_DESIGN_ENERGY + 1] = (uint8_t)(designEnergy & 0xFF);
    LOG_BATTERY_DEBUG("Design Energy: %d mWh", designEnergy);
    
    // 终止电压 (mV) - 设为3400mV
    uint16_t termVolt = 3400;
    blockData[OFFSET_TERMINATE_VOLTAGE] = (uint8_t)(termVolt >> 8);
    blockData[OFFSET_TERMINATE_VOLTAGE + 1] = (uint8_t)(termVolt & 0xFF);
    LOG_BATTERY_DEBUG("Terminate Voltage: %d mV", termVolt);

    // 6. 写入更新后的 BlockData
    for (int i = 0; i < 32; i++) {
        _writeBQ27421Byte(REG_BLOCK_DATA + i, blockData[i]);
    }
    delay(1);

    // 7. 计算并写入新校验和
    uint32_t tempSum = 0;
    for (int i = 0; i < 32; i++) {
        tempSum += blockData[i];
    }
    uint8_t newChecksum = (uint8_t)(255 - (tempSum % 256));
    _writeBQ27421Byte(REG_CHECKSUM, newChecksum);
    LOG_BATTERY_DEBUG("New checksum: 0x%02X", newChecksum);
    delay(10);

    // 8. 验证写入 (读回Design Capacity确认)
    _writeBQ27421Byte(REG_BLOCK_DATA_CTRL, 0x00);
    delay(1);
    _writeBQ27421Byte(REG_DATA_CLASS, DATA_CLASS_STATE);
    delay(1);
    _writeBQ27421Byte(REG_DATA_BLOCK, 0x00);
    delay(10);
    
    uint16_t verifyCapacity = (_readBQ27421Byte(REG_BLOCK_DATA + OFFSET_DESIGN_CAPACITY) << 8) | 
                              _readBQ27421Byte(REG_BLOCK_DATA + OFFSET_DESIGN_CAPACITY + 1);
    LOG_BATTERY_DEBUG("Verified Design Capacity: %d mAh", verifyCapacity);
    
    if (verifyCapacity != designCapacity_mAh) {
        LOG_BATTERY_ERROR("Design Capacity verification failed! Expected %d, got %d", 
                         designCapacity_mAh, verifyCapacity);
    }

    // 9. 退出配置更新模式
    LOG_BATTERY_DEBUG("Exiting CONFIG UPDATE mode...");
    _writeBQ27421Register(REG_CONTROL, CMD_SOFT_RESET);
    
    // 等待退出配置更新模式
    timeout = 0;
    while (_readBQ27421Register(REG_FLAGS) & FLAG_CFGUPMODE) {
        delay(10);
        if (timeout++ > 100) {  // 1秒超时
            LOG_BATTERY_ERROR("Timeout exiting CONFIG UPDATE mode");
            return;
        }
    }
    LOG_BATTERY_DEBUG("Exited CONFIG UPDATE mode");
    
    // 10. 等待设备稳定
    delay(100);

    // 11. 启用阻抗跟踪算法
    LOG_BATTERY_DEBUG("Enabling Impedance Track...");
    _writeBQ27421Register(REG_CONTROL, CMD_IT_ENABLE);
    delay(10);
    
    // 验证IT状态 (ITPOR=0表示IT正在运行, ITPOR=1表示需要学习周期)
    uint16_t flags = _readBQ27421Register(REG_FLAGS);
    LOG_BATTERY_DEBUG("Flags after IT enable: 0x%04X", flags);
    
    if (!(flags & FLAG_ITPOR)) {
        LOG_BATTERY_INFO("BQ27421 initialized successfully (IT algorithm running)");
    } else {
        LOG_BATTERY_WARN("BQ27421 initialized (IT needs learning cycle - charge/discharge required)");
    }

    // 12. 软复位燃料计以应用新配置
    LOG_BATTERY_DEBUG("Performing soft reset...");
    _writeBQ27421Register(REG_CONTROL, CMD_SOFT_RESET);
    delay(1000);  // 等待复位完成
    LOG_BATTERY_DEBUG("Soft reset completed");

    // 13. 重新密封设备 (保护配置不被意外修改)
    LOG_BATTERY_DEBUG("Sealing BQ27421...");
    _writeBQ27421Register(REG_CONTROL, CMD_SEAL);
    delay(10);
    
    // 验证SEAL状态 (读取Control Status)
    _writeBQ27421Register(REG_CONTROL, CMD_CONTROL_STATUS);
    delay(5);
    uint16_t controlStatus = _readBQ27421Register(REG_CONTROL);
    LOG_BATTERY_DEBUG("Control Status after seal: 0x%04X", controlStatus);
    
    // Bit 13 (SS) = 1 表示已密封
    if (controlStatus & 0x2000) {
        LOG_BATTERY_INFO("BQ27421 sealed successfully");
    } else {
        LOG_BATTERY_WARN("BQ27421 seal status uncertain");
    }
}

// ==================== 基础测量函数 ====================

// 读取电池电压 (单位: mV)
uint16_t readVoltage() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_VOLTAGE);  // Voltage() 寄存器 0x04-0x05
}

// 读取平均电流 (单位: mA, 有符号数)
int16_t readAverageCurrent() {
    if (!isfuelICConnected) return 0;
    return (int16_t)_readBQ27421Register(REG_AVG_CURRENT);  // 正数=充电, 负数=放电
}

// 读取电池温度 (单位: °C, 返回整数摄氏度)
uint16_t readBatteryTemperature() {
    if (!isfuelICConnected) return 0;
    uint16_t tempK = _readBQ27421Register(REG_TEMPERATURE);  // Temperature() 寄存器 0x02-0x03, 0.1K单位
    // 转换为摄氏度: (tempK * 0.1 - 273.1)
    // 使用整数运算: (tempK - 2731) / 10
    int16_t tempC = (int16_t)((tempK - 2731 + 5) / 10);  // +5 用于四舍五入
    return (tempC < 0) ? 0 : (uint16_t)tempC;  // 确保返回非负值
}

// 读取芯片内部温度 (单位: °C, 返回整数摄氏度)
uint16_t readInternalTemperature() {
    if (!isfuelICConnected) return 0;
    uint16_t tempK = _readBQ27421Register(REG_INTERNAL_TEMPERATURE);  // InternalTemperature() 寄存器 0x1E-0x1F, 0.1K单位
    // 转换为摄氏度: (tempK * 0.1 - 273.1)
    int16_t tempC = (int16_t)((tempK - 2731 + 5) / 10);  // +5 用于四舍五入
    return (tempC < 0) ? 0 : (uint16_t)tempC;  // 确保返回非负值
}

// 读取待机电流 (单位: mA, 有符号数)
int16_t readStandbyCurrent() {
    if (!isfuelICConnected) return 0;
    return (int16_t)_readBQ27421Register(REG_STANDBY_CURRENT);  // StandbyCurrent() 寄存器 0x12-0x13
}

// 读取最大负载电流 (单位: mA, 有符号数)
int16_t readMaxLoadCurrent() {
    if (!isfuelICConnected) return 0;
    return (int16_t)_readBQ27421Register(REG_MAX_LOAD_CURRENT);  // MaxLoadCurrent() 寄存器 0x14-0x15
}

// 读取平均功率 (单位: mW, 有符号数)
int16_t readAveragePower() {
    if (!isfuelICConnected) return 0;
    return (int16_t)_readBQ27421Register(REG_AVG_POWER);  // AveragePower() 寄存器 0x18-0x19
}

// ==================== 容量与电量函数 ====================

// 读取电量百分比 (单位: %, 0-100)
uint8_t readStateOfCharge() {
    if (!isfuelICConnected) return 0;
    return (uint8_t)_readBQ27421Register(REG_STATE_OF_CHARGE);  // SOC 寄存器 0x1C-0x1D
}

// 读取电池的剩余容量 (单位: mAh)
uint16_t readRemainingCapacity() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_REMAINING_CAP);  // RM 寄存器 0x0C-0x0D
}

// 读取电池的满充容量 (单位: mAh)
uint16_t readFullChargeCapacity() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_FULL_CHARGE_CAP);  // FCC 寄存器 0x0E-0x0F
}

// 读取未滤波剩余容量 (单位: mAh)
uint16_t readRemainingCapacityUnfiltered() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_REM_CAP_UNFILTERED);  // RemainingCapacityUnfiltered() 寄存器 0x28-0x29
}

// 读取滤波剩余容量 (单位: mAh)
uint16_t readRemainingCapacityFiltered() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_REM_CAP_FILTERED);  // RemainingCapacityFiltered() 寄存器 0x2A-0x2B
}

// 读取未滤波满充容量 (单位: mAh)
uint16_t readFullChargeCapacityUnfiltered() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_FULL_CHG_CAP_UNFILTERED);  // FullChargeCapacityUnfiltered() 寄存器 0x2C-0x2D
}

// 读取滤波满充容量 (单位: mAh)
uint16_t readFullChargeCapacityFiltered() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_FULL_CHG_CAP_FILTERED);  // FullChargeCapacityFiltered() 寄存器 0x2E-0x2F
}

// 读取未滤波电量百分比 (单位: %, 0-100)
uint8_t readStateOfChargeUnfiltered() {
    if (!isfuelICConnected) return 0;
    return (uint8_t)_readBQ27421Register(REG_SOC_UNFILTERED);  // StateOfChargeUnfiltered() 寄存器 0x30-0x31
}

// ==================== 健康与状态函数 ====================

// 读取电池健康状态 (单位: %, 0-100)
uint16_t readStateOfHealth() {
    if (!isfuelICConnected) return 0;
    uint16_t soh_raw = _readBQ27421Register(REG_STATE_OF_HEALTH);  // SOH 寄存器 0x20-0x21
    // 数据手册: Bits[15:8]=SOH%, Bits[7:0]=SOH状态
    uint8_t soh_status = (uint8_t)(soh_raw >> 8);
    if(soh_status == 0x00){
        LOG_BATTERY_WARN("SOH not valid (initialization)");
        return 0;
    }
    else if(soh_status == 0x01){
        LOG_BATTERY_DEBUG("Instant SOH value ready");
    }
    else if(soh_status == 0x02){
        LOG_BATTERY_DEBUG("Initial SOH value ready");
    }
    else if(soh_status == 0x03){
        LOG_BATTERY_VERBOSE("SOH value ready");
    }

    // 读取低字节
    uint8_t soh_percent = (uint8_t)(soh_raw & 0xFF);
    return soh_percent;
}

// 读取标称可用容量 (单位: mAh)
uint16_t readNominalAvailableCapacity() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_NOMINAL_AVAIL_CAP);  // NAC 寄存器 0x08-0x09
}

// 读取满可用容量 (单位: mAh)
uint16_t readFullAvailableCapacity() {
    if (!isfuelICConnected) return 0;
    return _readBQ27421Register(REG_FULL_AVAIL_CAP);  // FAC 寄存器 0x0A-0x0B
}

void readBatteryInfo() {
    if (!isfuelICConnected) {
        LOG_BATTERY_WARN("BQ27421 not connected, skipping battery info read");
        return;
    }
    
    // ==================== 基础测量 ====================
    uint16_t voltage = readVoltage();
    int16_t current = readAverageCurrent();
    uint16_t batteryTemp = readBatteryTemperature();
    uint16_t internalTemp = readInternalTemperature();
    int16_t standbyCurrent = readStandbyCurrent();
    int16_t maxLoadCurrent = readMaxLoadCurrent();
    int16_t avgPower = readAveragePower();
    
    // ==================== 容量与电量 ====================
    uint8_t soc = readStateOfCharge();
    uint16_t remainingCap = readRemainingCapacity();
    uint16_t fullChargeCap = readFullChargeCapacity();
    uint16_t nominalAvailCap = readNominalAvailableCapacity();
    uint16_t fullAvailCap = readFullAvailableCapacity();
    
    // 滤波/未滤波数据
    uint16_t remCapUnfilt = readRemainingCapacityUnfiltered();
    uint16_t remCapFilt = readRemainingCapacityFiltered();
    uint16_t fullCapUnfilt = readFullChargeCapacityUnfiltered();
    uint16_t fullCapFilt = readFullChargeCapacityFiltered();
    uint8_t socUnfilt = readStateOfChargeUnfiltered();
    
    // ==================== 健康状态 ====================
    uint16_t soh = readStateOfHealth();

    // ==================== 串口输出 ====================
    LOG_BATTERY_DEBUG("========== BQ27421 电池信息 ==========");
    
    LOG_BATTERY_DEBUG("--- 基础测量 ---");
    LOG_BATTERY_DEBUG("电压: %d mV", voltage);
    LOG_BATTERY_DEBUG("电流: %d mA %s", current, current >= 0 ? "(充电)" : "(放电)");
    LOG_BATTERY_DEBUG("电池温度: %d °C", batteryTemp);
    LOG_BATTERY_DEBUG("芯片温度: %d °C", internalTemp);
    LOG_BATTERY_DEBUG("待机电流: %d mA", standbyCurrent);
    LOG_BATTERY_DEBUG("最大负载电流: %d mA", maxLoadCurrent);
    LOG_BATTERY_DEBUG("平均功率: %d mW", avgPower);
    
    LOG_BATTERY_DEBUG("--- 容量与电量 ---");
    LOG_BATTERY_DEBUG("电量百分比: %d %%", soc);
    LOG_BATTERY_DEBUG("剩余容量: %d mAh", remainingCap);
    LOG_BATTERY_DEBUG("满充容量: %d mAh", fullChargeCap);
    LOG_BATTERY_DEBUG("标称可用容量: %d mAh", nominalAvailCap);
    LOG_BATTERY_DEBUG("满可用容量: %d mAh", fullAvailCap);
    
    LOG_BATTERY_DEBUG("--- 滤波数据对比 ---");
    LOG_BATTERY_DEBUG("剩余容量 (未滤波): %d mAh", remCapUnfilt);
    LOG_BATTERY_DEBUG("剩余容量 (滤波):   %d mAh", remCapFilt);
    LOG_BATTERY_DEBUG("满充容量 (未滤波): %d mAh", fullCapUnfilt);
    LOG_BATTERY_DEBUG("满充容量 (滤波):   %d mAh", fullCapFilt);
    LOG_BATTERY_DEBUG("电量 (未滤波): %d %%", socUnfilt);
    LOG_BATTERY_DEBUG("电量 (滤波):   %d %%", soc);
    
    LOG_BATTERY_DEBUG("--- 健康状态 ---");
    LOG_BATTERY_DEBUG("健康状态 (SOH): %d %%", soh);
    
    LOG_BATTERY_DEBUG("======================================\n");
}