#ifndef OPT3001_H
#define OPT3001_H

#include <Wire.h>

#include "mydefine.h"
#include "./utils/logger.h"

const uint8_t OPT3001_I2C_ADDR = 0x44;                 // OPT3001 I2C设备地址 (ADDR引脚接GND)

// ==================== OPT3001 寄存器地址定义 ====================
const uint8_t REG_OPT_RESULT = 0x00;                   // Result - 光照强度转换结果寄存器 (只读)
const uint8_t REG_OPT_CONFIG = 0x01;                   // Configuration - 配置寄存器 (读写)
const uint8_t REG_OPT_LOW_LIMIT = 0x02;                // Low Limit - 低阈值寄存器 (读写)
const uint8_t REG_OPT_HIGH_LIMIT = 0x03;               // High Limit - 高阈值寄存器 (读写)
const uint8_t REG_OPT_MANUFACTURER_ID = 0x7E;          // Manufacturer ID - 制造商ID寄存器 (只读, 0x5449 for TI)
const uint8_t REG_OPT_DEVICE_ID = 0x7F;                // Device ID - 设备ID寄存器 (只读, 0x3001)

// ==================== OPT3001 配置寄存器位定义 ====================
// RN[3:0] - 量程数字字段 (Bits 15-12)
const uint16_t CONFIG_RN_MASK = 0xF000;                // 量程掩码
const uint16_t CONFIG_RN_SHIFT = 12;                   // 量程位移

// CT - 转换时间 (Bit 11)
const uint16_t CONFIG_CT_100MS = 0x0000;               // 转换时间 100ms
const uint16_t CONFIG_CT_800MS = 0x0800;               // 转换时间 800ms

// M[1:0] - 转换模式 (Bits 10-9)
const uint16_t CONFIG_MODE_SHUTDOWN = 0x0000;          // 关断模式
const uint16_t CONFIG_MODE_SINGLE = 0x0200;            // 单次转换模式
const uint16_t CONFIG_MODE_CONTINUOUS = 0x0600;        // 连续转换模式 (默认)

// OVF - 溢出标志 (Bit 8, 只读)
const uint16_t CONFIG_OVF = 0x0100;                    // 溢出标志位

// CRF - 转换完成标志 (Bit 7, 只读)
const uint16_t CONFIG_CRF = 0x0080;                    // 转换完成标志位

// FH - 高阈值标志 (Bit 6, 只读)
const uint16_t CONFIG_FH = 0x0040;                     // 超过高阈值标志

// FL - 低阈值标志 (Bit 5, 只读)
const uint16_t CONFIG_FL = 0x0020;                     // 低于低阈值标志

// L - 锁存标志 (Bit 4)
const uint16_t CONFIG_L_TRANSPARENT = 0x0000;          // 透明比较模式
const uint16_t CONFIG_L_LATCHED = 0x0010;              // 锁存窗口模式

// POL - 极性 (Bit 3)
const uint16_t CONFIG_POL_ACTIVE_LOW = 0x0000;         // INT引脚低电平有效
const uint16_t CONFIG_POL_ACTIVE_HIGH = 0x0008;        // INT引脚高电平有效

// ME - 中断屏蔽 (Bit 2)
const uint16_t CONFIG_ME_DISABLE = 0x0000;             // 禁用中断报告
const uint16_t CONFIG_ME_ENABLE = 0x0004;              // 启用中断报告

// FC[1:0] - 故障计数 (Bits 1-0)
const uint16_t CONFIG_FC_1 = 0x0000;                   // 1次故障触发中断
const uint16_t CONFIG_FC_2 = 0x0001;                   // 2次故障触发中断
const uint16_t CONFIG_FC_4 = 0x0002;                   // 4次故障触发中断
const uint16_t CONFIG_FC_8 = 0x0003;                   // 8次故障触发中断

// ==================== 预定义配置模式 ====================
// 默认配置: 连续转换, 800ms转换时间, 自动满量程
// 0xCE10 = [15:12]=C(自动量程) + [11]=1(800ms) + [10:9]=3(连续模式) + 其他=0
const uint16_t CONFIG_DEFAULT = 0xCE10;                // 默认配置: 连续模式, 800ms, 自动量程

// 低功耗配置: 单次转换, 100ms转换时间
const uint16_t CONFIG_LOW_POWER = 0xC200;              // 低功耗配置: 单次模式, 100ms

// ==================== 制造商和设备ID ====================
const uint16_t OPT3001_MANUFACTURER_ID = 0x5449;       // 德州仪器制造商ID "TI"
const uint16_t OPT3001_DEVICE_ID = 0x3001;             // OPT3001设备ID

void initOPT3001();                                     /**< 初始化 OPT3001 光传感器 */
void configureOPT3001(uint16_t configValue);            /**< 配置 OPT3001 传感器模式 */

extern bool isOPT3001Connected;                         /**< 光传感器连接状态标志 */

// ==================== 基础测量函数 ====================
float readLux();                                        /**< 读取光照强度 (单位: lux) */
uint16_t readRawResult();                               /**< 读取原始转换结果寄存器值 */
bool isConversionReady();                               /**< 检查转换是否完成 */
bool isOverflow();                                      /**< 检查是否溢出 */

// ==================== 配置与状态函数 ====================
uint16_t readConfig();                                  /**< 读取当前配置寄存器值 */
void setLowLimit(float luxValue);                       /**< 设置低阈值 (单位: lux) */
void setHighLimit(float luxValue);                      /**< 设置高阈值 (单位: lux) */
float readLowLimit();                                   /**< 读取低阈值 (单位: lux) */
float readHighLimit();                                  /**< 读取高阈值 (单位: lux) */

// ==================== 设备识别函数 ====================
uint16_t readManufacturerID();                          /**< 读取制造商ID */
uint16_t readDeviceID();                                /**< 读取设备ID */
bool verifyOPT3001();                                   /**< 验证设备是否为OPT3001 */

void readLightInfo();                                   /**< 读取并打印所有光传感器信息到串口和日志 */

#endif
