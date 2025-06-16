#ifndef BUTTON_H
#define BUTTON_H

#include "myhader.h"
#include "mydefine.h"
#include "network.h"

/**
 * @brief 按钮状态结构体
 * 
 * 用于记录单个按钮的硬件引脚、标签、上一次状态、
 * 按下开始时间和是否已触发事件的状态信息。
 */
struct ButtonState {
    uint8_t pin;
    const char* label;
    bool lastState;
    unsigned long pressStartTime;
    bool triggered;
};

/**
 * @brief 按钮状态数组，存储所有按钮的状态信息
 * 
 * 数组元素类型为 ButtonState，包含引脚、标签、状态等信息。
 * 实际定义在 button.cpp 文件中。
 */
extern ButtonState buttons[];

/**
 * @brief 按钮数量
 * 
 * 表示 buttons 数组中按钮的总个数。
 */
extern const int buttonCount;

/**
 * @brief WiFi 客户端对象，用于发送按钮信息
 * 
 * 通过该客户端实现与服务器的 TCP 连接，传输按钮状态数据。
 */
extern WiFiClient client;

/**
 * @brief 按键处理任务函数
 * 
 * 该任务在 FreeRTOS 线程中运行，持续检测所有按钮的状态变化。
 * - 监听按钮按下事件，经过防抖时间后触发一次按下动作。
 * - 通过 WiFi 客户端向服务器发送按键标签消息。
 * - 确保按键按下时只发送一次消息，松开后重置状态。
 * 
 * @param[in] pvParameters 任务参数指针
 */
void buttonTask(void *pvParameters);

/**
 * @brief 启动按键检测任务
 * 
 * 创建并启动名为 "Button Task" 的 FreeRTOS 任务，
 */
void startButtonTask();

#endif