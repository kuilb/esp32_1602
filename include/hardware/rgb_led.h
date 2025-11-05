/**
 * @file rgb_led.h
 * @brief RGB LED 模块头文件，提供 RGB LED 控制功能
 *
 * 此文件声明 RGB LED 初始化、颜色和亮度控制的函数和变量
 *
 * @author kulib
 * @date 2025-11-06
 */
#ifndef RGB_LED_H
#define RGB_LED_H

#include "mydefine.h"
#include "myheader.h"
#include "logger.h"

/**
 * @brief 设置当前 RGB 灯颜色
 * @details 更新全局颜色变量，任务会检测变化并刷新 LED
 * @param[in] newColor 要设置的新颜色
 */
void updateColor(CRGB newColor);

/**
 * @brief 设置 LED 亮度
 * @details 更新全局亮度变量，任务会检测变化并刷新 LED
 * @param[in] newBrightness 要设置的新亮度, 范围 0-255
 */
void updateBrightness(uint8_t newBrightness);

/**
 * @brief 初始化 RGB LED 和相关资源
 * @details 初始化 FastLED 库，设置默认颜色和亮度，并创建控制任务
 */
void initrgb();

#endif