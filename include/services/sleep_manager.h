/**
 * @file sleep_manager.h
 * @brief 睡眠管理模块头文件，提供深度睡眠进入和唤醒管理功能
 *
 * 此文件声明睡眠管理相关的函数和变量，负责系统进入深度睡眠前的资源清理
 * 和唤醒后的状态恢复
 *
 * @author kulib
 * @date 2025-12-18
 */
#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>

#include "mydefine.h"
#include "./hardware/lcd_driver.h"
#include "./hardware/rgb_led.h"
#include "./connectivity/network.h"
#include "./services/time_manager.h"
#include "./utils/logger.h"
#include "esp_sleep.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

/**
 * @brief 任务退出标志
 * 
 * 当设置为 true 时，所有任务应该优雅退出
 */
extern volatile bool shouldExitTasks;

/**
 * @brief 初始化睡眠管理器
 * 
 * 初始化睡眠相关的全局变量，应在系统启动时调用
 */
void initSleepManager();

/**
 * @brief 进入深度睡眠
 *
 * 执行进入深度睡眠前的所有准备工作：
 * 1. 通知所有任务退出
 * 2. 断开网络连接
 * 3. 关闭WiFi
 * 4. 保存RTC时间
 * 5. 配置唤醒源
 * 6. 进入深度睡眠
 */
void enterDeepSleep();

/**
 * @brief 检查是否从深度睡眠唤醒
 *
 * @return true 如果是从深度睡眠唤醒，false 如果是正常启动
 */
bool isWakeupFromDeepSleep();

#endif
