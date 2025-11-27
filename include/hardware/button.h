/**
 * @file button.h
 * @brief 按钮模块头文件，提供按钮状态管理和事件处理功能
 *
 * 此文件声明按钮状态结构体、外部变量和按钮处理函数
 *
 * @author kulib
 * @date 2025-11-06
 */
#ifndef BUTTON_H
#define BUTTON_H

#include "myheader.h"
#include "mydefine.h"
#include "network.h"
#include "button.h"
#include "menu.h"
#include "logger.h"

/**
 * @brief 按钮状态结构体
 *
 * 用于记录单个按钮的状态信息
 */
struct ButtonState {
    uint8_t pin;                  ///< GPIO 引脚编号
    const char* label;            ///< 按钮标识符字符串
    bool lastState;               ///< 上一次读取的电平状态，LOW 或 HIGH
    unsigned long pressStartTime; ///< 按钮按下时的时间戳
};

/**
 * @brief 外部变量声明，用于按钮模块间共享
 *
 */
extern const int buttonCount;                    /**< 按钮数量 */
extern volatile bool buttonJustPressed[];        /**< 按钮是否刚刚被按下标志数组 */

void initButtonsPin();                          /**< 初始化按钮引脚为输入模式 */

/**
 * @brief 检查按钮是否可以响应，防抖和节流控制
 *
 * @param buttonIndex 按钮索引
 * @param minInterval 最小间隔时间，毫秒，默认150ms
 * @return true 如果按钮可以响应，false 如果应该被忽略
 */
bool isButtonReadyToRespond(int buttonIndex, unsigned long minInterval = 150);

/**
 * @brief 重置所有按钮的防抖计时器
 *
 * 在进入新界面或模式时调用，防止界面切换时的误触
 */
void resetButtonDebounce();

/**
 * @brief 在原来的按钮防抖等待上添加全局延迟
 *
 * @param delayMs 延迟时间，毫秒，默认200ms
 * 在界面切换后调用，防止立即响应按键
 */
void globalButtonDelay(unsigned long delayMs = 200);

/**
 * @brief WiFi 客户端对象，用于发送按钮信息
 *
 * 通过该客户端实现与服务器的 TCP 连接，传输按钮状态数据
 */
extern WiFiClient client;

/**
 * @brief 启动按键相关任务
 *
 */
void startButtonTask();

#endif