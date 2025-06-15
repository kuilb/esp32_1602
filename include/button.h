#ifndef BUTTON_H
#define BUTTON_H

#include "myhader.h"
#include "mydefine.h"
#include "network.h"

// 按钮状态结构体
struct ButtonState {
    uint8_t pin;
    const char* label;
    bool lastState;
    unsigned long pressStartTime;
    bool triggered;
};

// 按钮数组及数量，extern 声明，实际定义在 button.cpp 中
extern ButtonState buttons[];
extern const int buttonCount;

// 发送按钮信息用的客户端（如果多个文件需要访问）
extern WiFiClient client;

// 按键处理任务函数
void buttonTask(void *pvParameters);

// 外部任务函数声明
void buttonTask(void *pvParameters);
void startButtonTask();

#endif