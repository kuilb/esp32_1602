#ifndef HOLD_PROGRESS_H
#define HOLD_PROGRESS_H

#include <Arduino.h>

/**
 * @brief 渲染长按进度条（2行LCD）
 * @param title 第一行提示文案，会自动裁剪/补空格到 16 列
 * @param percent 进度百分比（0~100）
 */
void renderHoldProgressBar(const char* title, uint8_t percent);

#endif
