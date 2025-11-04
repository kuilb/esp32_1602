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

#include "myheader.h"
#include "lcd_driver.h"
#include "menu.h"
#include "button.h"

extern bool isTimeSynced;              /**< 时间是否已同步的标志 */
extern bool isTimeSyncInProgress;      /**< 时间同步是否正在进行的标志 */

void initTimeSync();                 /**< 初始化NTP时间同步 */
void updateTimeSync();               /**< 更新时间同步状态 */
void updateClockScreen();            /**< 更新LCD上的时钟显示 */

#endif
