#ifndef AUTO_BRIGHTNESS_H
#define AUTO_BRIGHTNESS_H

#include <Arduino.h>
#include "./hardware/opt3001.h"
#include "./hardware/lcd_driver.h"
#include "./utils/logger.h"

// ==================== 自动背光配置参数 ====================

/**
 * @brief 光照强度到亮度的映射点结构
 */
struct LuxBrightnessPoint {
    float lux;              /**< 光照强度 (lux) */
    uint8_t brightness;     /**< 对应的背光亮度 (0-255) */
};

// 默认映射曲线：光照强度 -> 背光亮度
// 可根据实际使用环境调整这些映射点
const LuxBrightnessPoint DEFAULT_LUX_MAP[] = {
    {0.0,       10},       // 完全黑暗环境 -> 最低亮度 (保持可见性)
    {1.0,       20},      // 极暗环境 (夜间) -> 低亮度
    {10.0,      40},      // 暗环境 (室内无灯) -> 中低亮度
    {50.0,      70},      // 一般室内环境 -> 中等亮度
    {100.0,     100},     // 明亮室内环境 -> 中高亮度
    {500.0,     160},     // 非常明亮 (靠窗) -> 高亮度
    {1000.0,    220},     // 阳光直射 -> 很高亮度
    {5000.0,    255}      // 强烈阳光 -> 最大亮度
};

const uint8_t LUX_MAP_SIZE = sizeof(DEFAULT_LUX_MAP) / sizeof(LuxBrightnessPoint);

// 亮度调节配置
const uint8_t MIN_BRIGHTNESS = 1;         /**< 最小亮度限制 (防止完全看不见) */
const uint8_t MAX_BRIGHTNESS = 255;       /**< 最大亮度限制 */
const uint8_t BRIGHTNESS_HYSTERESIS = 5;  /**< 亮度变化阈值 (防止频繁抖动) */
const float LUX_SMOOTHING_FACTOR = 0.3;   /**< 光照强度平滑系数 (0-1, 越小越平滑) */
const uint8_t BRIGHTNESS_FADE_STEP = 3;   /**< 亮度渐变步进值 (每次调整的亮度差值，越小越平滑) */

// ==================== 全局状态变量 ====================
extern bool isAutoBrightnessEnabled;      /**< 自动背光开关 */
extern float smoothedLux;                 /**< 平滑后的光照强度值 */
extern uint8_t targetBrightness;          /**< 目标背光亮度 */
extern uint8_t lastAppliedBrightness;     /**< 上次应用的背光亮度 */

// ==================== 函数声明 ====================

/**
 * @brief 初始化自动背光调节模块
 * @details 初始化光传感器和相关变量，默认启用自动调节
 */
void initAutoBrightness();

/**
 * @brief 启用自动背光调节
 */
void enableAutoBrightness();

/**
 * @brief 禁用自动背光调节
 */
void disableAutoBrightness();

/**
 * @brief 切换自动背光调节开关
 * @return 当前自动背光状态
 */
bool toggleAutoBrightness();

/**
 * @brief 获取自动背光调节状态
 * @return true=启用, false=禁用
 */
bool isAutoBrightnessActive();

/**
 * @brief 根据光照强度计算目标亮度
 * @param lux 当前光照强度 (lux)
 * @return 计算得到的目标亮度值 (0-255)
 */
uint8_t calculateBrightness(float lux);

/**
 * @brief 更新自动背光亮度（主循环调用）
 * @details 读取光照强度，计算并应用新的背光亮度
 *          使用平滑算法和阈值判断，避免频繁变化
 * @return true=亮度已更新, false=亮度未变化
 */
bool updateAutoBrightness();

/**
 * @brief 设置自定义的光照-亮度映射曲线
 * @param luxMap 映射点数组
 * @param mapSize 数组大小
 */
void setCustomLuxMap(const LuxBrightnessPoint* luxMap, uint8_t mapSize);

/**
 * @brief 手动设置亮度（会自动禁用自动调节）
 * @param brightness 目标亮度 (0-255)
 */
void setManualBrightness(uint8_t brightness);

/**
 * @brief 启动自动背光调节后台任务
 * @details 创建FreeRTOS任务，周期性调用updateAutoBrightness()
 */
void startAutoBrightnessTask();

/**
 * @brief 停止自动背光调节后台任务
 */
void stopAutoBrightnessTask();

/**
 * @brief 获取当前光照强度信息
 * @param smoothed 是否返回平滑后的值
 * @return 光照强度 (lux)
 */
float getCurrentLux(bool smoothed = true);

/**
 * @brief 打印当前自动背光调节状态
 */
void printAutoBrightnessInfo();

#endif
