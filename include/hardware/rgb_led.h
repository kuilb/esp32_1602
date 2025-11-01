#ifndef RGB_LED_H
#define RGB_LED_H

#include "mydefine.h"
#include "myhader.h"

/**
 * @brief 定义一个包含单个 LED 的 CRGB 数组
 */
extern CRGB leds[1];

/**
 * @brief 当前 LED 颜色，受互斥锁保护
 */
extern CRGB currentColor;

/**
 * @brief 当前 LED 亮度，范围 0-255，受互斥锁保护
 */
extern uint8_t currentBrightness;

/**
 * @brief 保护 LED 颜色和亮度变量的 FreeRTOS 互斥锁句柄
 */
extern SemaphoreHandle_t ledMutex;

/**
 * @brief 设置当前 RGB 灯颜色（线程安全）
 * 
 * 使用互斥锁（ledMutex）确保多线程环境下对灯光颜色的安全访问。
 *
 * @param[in] newColor 要设置的新颜色（类型为 CRGB）
 */
void updateColor(CRGB newColor);

/**
 * @brief 设置 LED 亮度（线程安全）
 * 
 * 使用互斥锁（ledMutex）确保在多线程环境中安全更新亮度值。
 * 
 * @param[in] newBrightness 要设置的新亮度（0–255）
 */
void updateBrightness(uint8_t newBrightness);

/**
 * @brief LED 灯控制任务，周期更新颜色和亮度
 * 
 * 该任务运行在 FreeRTOS 线程中，持续更新 LED 灯的显示状态。
 * 通过互斥锁保护共享资源 currentColor 和 currentBrightness，确保线程安全。
 * 每 50 毫秒刷新一次 LED 灯光。
 * 
 * @param[in] pvParameters 任务参数指针（未使用）
 */
void rgbTask(void* pvParameters);

/**
 * @brief 初始化 RGB LED 和相关资源
 * 
 * 初始化 FastLED 库，创建用于保护 LED 资源的互斥锁。
 * 若互斥锁创建失败，则进入错误状态，LED 显示红色呼吸灯提示。
 * 成功时设置默认颜色和亮度，并创建用于控制 LED 的 FreeRTOS 任务。
 */
void initrgb();

#endif