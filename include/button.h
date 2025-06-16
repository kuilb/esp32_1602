#ifndef BUTTON_H
#define BUTTON_H

#include "myhader.h"
#include "mydefine.h"
#include "network.h"
#include "button.h"
#include "menu.h"

/**
 * @brief 按钮状态结构体
 * 
 * 用于记录单个按钮的硬件引脚、标签、上一次状态、
 * 按下开始时间和是否已触发事件的状态信息。
 */
struct ButtonState {
    uint8_t pin;                  ///< GPIO 引脚编号
    const char* label;            ///< 按钮标识符字符串（如 "UP", "CENTER"）
    bool lastState;               ///< 上一次读取的电平状态（LOW 或 HIGH）
    unsigned long pressStartTime; ///< 按钮按下时的时间戳（用于判断按下时长）
    bool triggered;               ///< 是否已触发按下事件（防止重复触发）
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

extern volatile bool buttonJustPressed[];

/**
 * @brief WiFi 客户端对象，用于发送按钮信息
 * 
 * 通过该客户端实现与服务器的 TCP 连接，传输按钮状态数据。
 */
extern WiFiClient client;

/**
 * @brief 按键扫描任务（运行于独立线程）
 *
 * 周期性扫描所有定义的按键引脚，检测按下和松开状态。
 * 更新 `buttonJustPressed[]` 状态数组，
 * 供其他任务（如按键事件处理任务）读取使用。
 *
 * @param pvParameters 未使用
 */
void scanButtonsTask(void *pvParameters);

/**
 * @brief 按键事件处理任务（运行于独立线程）
 * 
 * 负责检测按键状态并触发对应事件：
 * - 检测 Center + Up 组合键长按（1秒）进入菜单模式（inMenuMode）。
 * - 非菜单模式下，将按键事件通过 WiFiClient 发送出去。
 * - 菜单模式下按键处理交由其他模块处理，此任务暂时休眠。
 * 
 * @param pvParameters 未使用
 */
void handleButtonsTask(void *pvParameters);

/**
 * @brief 启动按键相关任务
 * 
 */
void startButtonTask();

#endif