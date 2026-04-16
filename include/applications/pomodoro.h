#ifndef POMODORO_H
#define POMODORO_H

#include "./hardware/button.h"
#include "./hardware/buzzer.h"
#include "./hardware/lcd_driver.h"
#include "./menu/menu.h"
#include "./utils/logger.h"

/**
 * @brief 进入番茄钟界面（非阻塞，注册到状态机）
 */
void enterPomodoroInterface();

/**
 * @brief 处理番茄钟界面状态机，每帧调用一次
 */
void handlePomodoroInterface();

#endif
