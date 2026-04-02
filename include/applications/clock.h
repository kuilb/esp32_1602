/**
 * @file clock.h
 * @brief 时钟应用头文件，提供时间同步和显示功能
 *
 * 此文件声明NTP时间同步和LCD时钟显示的函数和变量
 *
 * @author kulib
 * @date 2025-11-04
 */
#ifndef CLOCK_H
#define CLOCK_H

#include "./hardware/lcd_driver.h"
#include "./applications/menu.h"
#include "./hardware/button.h"
#include "./utils/logger.h"
#include "./services/time_manager.h"

extern struct tm localTimeInfo;  /**< 本地时间信息结构体 */

void updateClockScreen();            /**< 更新LCD上的时钟显示 */

#endif
