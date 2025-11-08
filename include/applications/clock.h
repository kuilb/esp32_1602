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

extern struct tm localTimeInfo;  /**< 本地时间信息结构体 */

enum TimeSyncState {
    TIME_SYNC_IDLE = 0,        /**< 空闲状态 */
    TIME_SYNC_IN_PROGRESS,     /**< 同步进行中 */
    TIME_SYNC_SUCCESS,         /**< 同步成功 */
    TIME_SYNC_FAILED           /**< 同步失败 */
};

extern TimeSyncState timeSyncState;          /**< 当前时间同步状态 */
extern volatile bool isTimeSyncInProgress;   /**< 时间同步是否正在进行的标志 */

void initTimeSync();                 /**< 初始化NTP时间同步 */
void updateTimeSync();               /**< 更新时间同步状态 */
void updateClockScreen();            /**< 更新LCD上的时钟显示 */
void timeSyncTask(void* parameter);  /**< 时间同步后台任务 */

#endif
