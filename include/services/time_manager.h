#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <WiFi.h>
#include "soc/rtc.h"

#include "mydefine.h"
#include "./services/sleep_manager.h"
#include "./utils/logger.h"

extern struct tm localTimeInfo;  /**< 本地时间信息结构体 */
extern RTC_DATA_ATTR struct timeval sleep_enter_time;
extern RTC_DATA_ATTR uint64_t sleep_enter_rtc_time;  /**< 睡眠时的RTC计数器值 */

enum TimeSyncState {
    TIME_SYNC_IDLE = 0,        /**< 空闲状态 */
    TIME_SYNC_IN_PROGRESS,     /**< 同步进行中 */
    TIME_SYNC_SUCCESS,         /**< 同步成功 */
    TIME_SYNC_FAILED           /**< 同步失败 */
};

extern TimeSyncState timeSyncState;  /**< 当前时间同步状态 */

void initTime(bool isSleepWakeup);    /**< 初始化时间管理系统 */
void initNtpTimeSync();               /**< 初始化 NTP 时间同步 */
void updateTimeSync();                /**< 更新时间同步状态 */
void timeSyncTask(void* parameter);   /**< 时间同步任务 */
timeval getRtcTime();                 /**< 获取 RTC 时间 */

#endif