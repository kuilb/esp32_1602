#ifndef FUEL_GAUGE_H
#define FUEL_GAUGE_H

#include <Wire.h>

#include "mydefine.h"
#include "./utils/logger.h"

const uint8_t BQ27421_I2C_ADDR = 0x55;                 // BQ27421 I2C设备地址

// ==================== BQ27421 寄存器地址定义 ====================
// 标准命令寄存器
const uint8_t REG_CONTROL = 0x00;                      // Control() - 控制寄存器 (0x00-0x01)
const uint8_t REG_TEMPERATURE = 0x02;                  // Temperature() - 燃料计测量的温度 (0x02-0x03, 0.1°K)
const uint8_t REG_VOLTAGE = 0x04;                      // Voltage() - 电池电压 (0x04-0x05, mV)
const uint8_t REG_FLAGS = 0x06;                        // Flags() - 状态寄存器，描述当前的运行状态 (0x06-0x07)
const uint8_t REG_NOMINAL_AVAIL_CAP = 0x08;            // NominalAvailableCapacity() - 未补偿（小于 C/20 负载）的剩余电池容量 (0x08-0x09, mAh)
const uint8_t REG_FULL_AVAIL_CAP = 0x0A;               // FullAvailableCapacity() - 电池完全充电时的未补偿容量(小于 C/20 负载) (0x0A-0x0B, mAh)
const uint8_t REG_REMAINING_CAP = 0x0C;                // RemainingCapacity() - 剩余容量 (0x0C-0x0D, mAh)
const uint8_t REG_FULL_CHARGE_CAP = 0x0E;              // FullChargeCapacity() - 满充容量 (0x0E-0x0F, mAh)
const uint8_t REG_AVG_CURRENT = 0x10;                  // AverageCurrent() - 平均电流 每秒更新一次 (0x10-0x11, mA)
const uint8_t REG_STANDBY_CURRENT = 0x12;              // StandbyCurrent() - 待机电流 (0x12-0x13, mA)
const uint8_t REG_MAX_LOAD_CURRENT = 0x14;             // MaxLoadCurrent() - 最大负载电流 (0x14-0x15, mA)
const uint8_t REG_AVG_POWER = 0x18;                    // AveragePower() - 平均功率 (0x18-0x19, mW)
const uint8_t REG_STATE_OF_CHARGE = 0x1C;              // StateOfCharge() - 电量百分比 (0x1C-0x1D, %)
const uint8_t REG_INTERNAL_TEMPERATURE = 0x1E;         // InternalTemperature() - 芯片内部温度 (0x1E-0x1F, 0.1°K)
const uint8_t REG_STATE_OF_HEALTH = 0x20;              // StateOfHealth() - 电池健康度 (0x20-0x21, %)
const uint8_t REG_REM_CAP_UNFILTERED = 0x28;           // RemainingCapacityUnfiltered() - 未滤波剩余容量 (0x28-0x29, mAh)
const uint8_t REG_REM_CAP_FILTERED = 0x2A;             // RemainingCapacityFiltered() - 滤波剩余容量 (0x2A-0x2B, mAh)
const uint8_t REG_FULL_CHG_CAP_UNFILTERED = 0x2C;      // FullChargeCapacityUnfiltered() - 未滤波满充容量 (0x2C-0x2D, mAh)
const uint8_t REG_FULL_CHG_CAP_FILTERED = 0x2E;        // FullChargeCapacityFiltered() - 滤波满充容量 (0x2E-0x2F, mAh)
const uint8_t REG_SOC_UNFILTERED = 0x30;               // StateOfChargeUnfiltered() - 未滤波电量百分比 (0x30-0x31, %)

// 配置相关寄存器
const uint8_t REG_OP_CONFIG = 0x3A;                    // OpConfig() - 操作配置寄存器 (0x3A-0x3B)
const uint8_t REG_CAPACITY = 0x3C;                     // Capacity() - 电池容量寄存器 (0x3C-0x3D)
const uint8_t REG_DATA_CLASS = 0x3E;                   // DataBlockClass() - 数据类选择
const uint8_t REG_DATA_BLOCK = 0x3F;                   // DataBlock() - 数据块选择
const uint8_t REG_BLOCK_DATA = 0x40;                   // BlockData() - 块数据起始地址 (0x40-0x5F)
const uint8_t REG_CHECKSUM = 0x60;                     // BlockDataChecksum() - 块数据校验和
const uint8_t REG_BLOCK_DATA_CTRL = 0x61;              // BlockDataControl() - 块数据控制

// ==================== BQ27421 控制命令 ====================
const uint16_t CMD_CONTROL_STATUS = 0x0000;            // CONTROL_STATUS - 读取控制状态
const uint16_t CMD_DEVICE_TYPE = 0x0001;               // DEVICE_TYPE - 读取设备类型，返回值为 0x0421
const uint16_t CMD_FW_VERSION = 0x0002;                // FW_VERSION - 读取固件版本
const uint16_t CMD_DM_CODE = 0x0004;                   // DM_CODE - 读取设计容量，作为 16 位返回值的低字节
const uint16_t CMD_PREV_MACWRITE = 0x0007;             // PREV_MACWRITE - 读取上次主机写入的命令
const uint16_t CMD_CHEM_ID = 0x0008;                   // CHEM_ID - 读取化学标识符
const uint16_t CMD_BAT_INSERT = 0x000C;                // BAT_INSERT - 当 OpConfig [BIE] 位为 0（禁用引脚自动检测）时，模拟电池插入事件
const uint16_t CMD_BAT_REMOVE = 0x000D;                // BAT_REMOVE - 当 OpConfig [BIE] 位为 0 时，模拟电池移除事件
const uint16_t CMD_SET_HIBERNATE = 0x0011;             // SET_HIBERNATE - 使电量计准备进入冬眠模式
const uint16_t CMD_CLEAR_HIBERNATE = 0x0012;           // CLEAR_HIBERNATE - 强制将 CONTROL_STATUS [HIBERNATE] 位清零，取消冬眠请求
const uint16_t CMD_SET_CFGUPDATE = 0x0013;             // SET_CFGUPDATE - 进入配置更新模式
const uint16_t CMD_SHUTDOWN_ENABLE = 0x001B;           // SHUTDOWN_ENABLE - 启用关机模式
const uint16_t CMD_SHUTDOWN = 0x001C;                  // SHUTDOWN - 立即进入关断模式,此命令在 SEALED 模式下不可用
const uint16_t CMD_SEAL = 0x0020;                      // SEAL - 密封设备，防止配置被修改
const uint16_t CMD_IT_ENABLE = 0x0021;                 // IT_ENABLE - 启用阻抗跟踪算法
const uint16_t CMD_PULSE_SOC_INT = 0x0023;             // PULSE_SOC_INT - 触发 SOC 中断
const uint16_t CMD_RESET = 0x00411;                    // RESET - 执行完整的设备复位
const uint16_t CMD_SOFT_RESET = 0x0042;                // SOFT_RESET - 软复位，退出配置模式
const uint16_t CMD_EXIT_CFGUPDATE = 0x0043;            // EXIT_CFGUPDATE - 退出配置更新模式，但不进行开路电压（OCV）测量，也不重新模拟更新 StateOfCharge()

const uint16_t CMD_UNSEAL = 0x8000;                    // UNSEAL - 解封设备，允许修改配置 (默认密钥)

// ==================== BQ27421 CONTROL_STATUS 寄存器位定义掩码 ====================
// 低字节
const uint16_t STATUS_VOK = 0b00000010;            // Bit 1 - 电池电压稳定，适合进行 Qmax 更新
const uint16_t STATUS_SLEEP = 0b00010000;          // Bit 4 - 设备处于睡眠模式
const uint16_t STATUS_HIBERNATE = 0b01000000;      // Bit 6 - 指示系统已请求进入冬眠模式
const uint16_t STATUS_INITCOMP = 0b10000000;       // Bit 7 - 初始化完成标志。为 1 时表示电量计已完成启动初始化

// 高字节
const uint16_t STATUS_RES_UP = 0b00000001;         // Bit 0 - 指示电池电阻谱已更新
const uint16_t STATUS_QMAX_UP = 0b00000010;        // Bit 1 - 指示 Qmax (电池化学容量) 已更新。上电或更换电池后会清零，以开启快速学习过程 
const uint16_t STATUS_CALMODE = 0b00000100;        // Bit 2 - 指示当前处于校准模式
const uint16_t STATUS_SS = 0b00100000;             // Bit 5 - 若置位，表示电量计处于 SEALED 模式
const uint16_t STATUS_SHUTDOWNEN = 0b10000000;     // Bit 7 - 指示已收到关断启用命令，处于准备关断状态

// ==================== BQ27421 Flags 寄存器标志位 ====================
// 低字节
const uint16_t FLAG_DSG = 0b00000001;                     // Bit 0 - 放电标志 (1=放电, 0=充电或空闲)
const uint16_t FLAG_SOCF = 0b00000010;                    // Bit 1 - 当 SOC 下降到 SOCF Set Threshold（默认 2%）及以下时置位，作为关机警告
const uint16_t FLAG_SOC1 = 0b00000100;                    // Bit 2 - 当 SOC 下降到 SOC1 Set Threshold（默认 10%）及以下时置位
const uint16_t FLAG_BAT_DET = 0b00001000;                 // Bit 3 - 电池检测标志：为 1 时表示检测到电池插入，为 0 表示电池已移除
const uint16_t FLAG_CFGUPMODE = 0b00010000;               // Bit 4 - 指示电量计当前处于配置更新模式，计算已暂停
const uint16_t FLAG_ITPOR = 0b00100000;                   // Bit 5 - 上电复位标志：指示发生了 POR 或发送了 RESET 子命令，RAM 已返回 ROM 默认值
const uint16_t FLAG_OCVTAKEN = 0b10000000;                // Bit 7 - OCV 测量完成：指示电量计已成功获取开路电压 (OCV) 读数

// 高字节
const uint16_t FLAG_CHG = 0b00000001;                     // Bit 0 - 当 SOC 达到 TCA Set % 或满足充电终止条件时置位
const uint16_t FLAG_FC = 0b00000010;                      // Bit 1 - 满充标志：为 1 时表示电池已充满
const uint16_t FLAG_UT = 0b01000000;                      // Bit 6 - 当检测到电池温度低于安全阈值时置位
const uint16_t FLAG_OT = 0b10000000;                      // Bit 7 - 当检测到电池温度高于安全阈值时置位

// ==================== BQ27421 数据存储器类 子类ID ====================
const uint8_t DATA_CLASS_SAFETY = 2;              // 数据类 2 - Safety
const uint8_t DATA_CLASS_CHARGE_TERMINATION = 36;       // 数据类 36 - Charge Termination
const uint8_t DATA_CLASS_DISCHARGE = 49;                // 数据类 49 - Discharge
const uint8_t DATA_CLASS_REGISTERS = 64;                // 数据类 64 - Registers
const uint8_t DATA_CLASS_POWER = 68;                    // 数据类 68 - Power
const uint8_t DATA_CLASS_IT_CFG = 80;                   // 数据类 80 - IT Cfg 
const uint8_t DATA_CLASS_CURRENT_THRESHOLDS = 81;       // 数据类 81 - Current Thresholds
const uint8_t DATA_CLASS_STATE = 82;                    // 数据类 82 - State of Health

// ==================== State of Health 数据类偏移  ====================
const uint8_t OFFSET_QMAX_CELL = 0;                    // 偏移 0 - 最大化学容量(比例系数)，默认为16384 (100%)

/*
    算法学习控制位：Bit 0 (0x01) 与 Bit 1 (0x02)
    正常运行模式 (默认：0x00)
    学习模式/制作黄金镜像 (Set)：0x01

    安全锁定标志位：Bit 7 (0x80)
*/
const uint8_t OFFSET_UPDATE_STATUS = 2;                // 偏移 2
const uint8_t OFFSET_RESERVE_CAP_MAH = 3;              // 偏移 3 - 预留容量 (mAh)
const uint8_t OFFSET_DESIGN_CAPACITY = 10;             // 偏移 10 - 设计容量 (mAh)
const uint8_t OFFSET_DESIGN_ENERGY = 12;               // 偏移 12 - 设计能量 (mWh)
const uint8_t OFFSET_TERMINATE_VOLTAGE = 16;           // 偏移 16 - 终止电压 (mV)

void initBQ27421(uint16_t designCapacity_mAh);  /**< 初始化 BQ27421 燃料计芯片，设置设计容量 (单位: mAh) */
void setBQ27421DesignCapacity(uint16_t designCapacity_mAh);  /**< 设置 BQ27421 设计容量 (单位: mAh) */

extern bool isfuelICConnected;  /**< 燃料计芯片连接状态标志 */

// ==================== 基础测量函数 ====================
uint16_t readVoltage();                         /**< 读取电池电压 (单位: mV) */
int16_t readAverageCurrent();                   /**< 读取平均电流 (单位: mA, 有符号数, 正数=充电, 负数=放电) */
uint16_t readBatteryTemperature();              /**< 读取电池温度 (单位: °C) */
uint16_t readInternalTemperature();             /**< 读取芯片内部温度 (单位: °C) */
int16_t readStandbyCurrent();                   /**< 读取待机电流 (单位: mA, 有符号数) */
int16_t readMaxLoadCurrent();                   /**< 读取最大负载电流 (单位: mA, 有符号数) */
int16_t readAveragePower();                     /**< 读取平均功率 (单おんせい 位: mW, 有符号数) */

// ==================== 容量与电量函数 ====================
uint8_t readStateOfCharge();                    /**< 读取电量百分比 (单位: %, 0-100) */
uint16_t readRemainingCapacity();               /**< 读取电池的剩余容量 (单位: mAh) */
uint16_t readFullChargeCapacity();              /**< 读取电池的满充容量 (单位: mAh) */
uint16_t readRemainingCapacityUnfiltered();     /**< 读取未滤波剩余容量 (单位: mAh) */
uint16_t readRemainingCapacityFiltered();       /**< 读取滤波剩余容量 (单位: mAh) */
uint16_t readFullChargeCapacityUnfiltered();    /**< 读取未滤波满充容量 (单位: mAh) */
uint16_t readFullChargeCapacityFiltered();      /**< 读取滤波满充容量 (单位: mAh) */
uint8_t readStateOfChargeUnfiltered();          /**< 读取未滤波电量百分比 (单位: %, 0-100) */

// ==================== 健康与状态函数 ====================
uint16_t readStateOfHealth();                   /**< 读取电池健康状态 (单位: %, 0-100) */
uint16_t readNominalAvailableCapacity();        /**< 读取标称可用容量 (单位: mAh) */
uint16_t readFullAvailableCapacity();           /**< 读取满可用容量 (单位: mAh) */

void readBatteryInfo();                         /**< 读取并打印所有电池信息到串口和日志 */

#endif