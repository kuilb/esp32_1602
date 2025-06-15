#ifndef RGB_LED_H
#define RGB_LED_H

#include "mydefine.h"
#include "myhader.h"

extern CRGB leds[1];
extern CRGB currentColor;
extern uint8_t currentBrightness;
extern SemaphoreHandle_t ledMutex;

// 设置颜色与亮度（线程安全）
void updateColor(CRGB newColor);
void updateBrightness(uint8_t newBrightness);

// 启动控制任务
void rgbTask(void* pvParameters);

void initrgb();

#endif