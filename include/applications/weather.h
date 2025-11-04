/**
 * @file weather.h
 * @brief 天气应用头文件，提供天气数据获取和显示功能
 *
 * 此文件声明天气系统的变量和函数，用于ESP32设备的天气信息管理
 *
 * @author kulib
 * @date 2025-11-04
 */
#ifndef WEATHER_H
#define WEATHER_H

#include "myheader.h"
#include "lcd_driver.h"
#include "menu.h"
#include "button.h"
#include "clock.h"
#include "weathertrans.h"
#include "jwt_auth.h"

extern bool weatherSynced;              /**< 天气是否已同步的标志 */
extern unsigned long lastWeatherUpdate; /**< 最后天气更新的时间戳 */
extern unsigned int interface_num;      /**< 当前显示的界面编号 */

void updateWeatherScreen();             /**< 更新LCD上的天气显示 */
bool fetchWeatherData();                /**< 获取天气数据 */

#endif
