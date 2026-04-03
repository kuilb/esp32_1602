#ifndef POMODORO_H
#define POMODORO_H

#include "./hardware/button.h"
#include "./hardware/buzzer.h"
#include "./hardware/lcd_driver.h"
#include "./menu/menu.h"
#include "./utils/logger.h"

/**
 * @brief 番茄钟应用入口
 * @details 25分钟工作 + 5分钟休息循环，阶段切换时进行音效和屏幕闪烁提醒
 */
void runPomodoroApp();

#endif
