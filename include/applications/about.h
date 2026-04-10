/**
 * @file about.h
 * @brief About 应用程序模块的头文件
 * 
 * 此模块提供在 ESP32 设备上显示创建者和项目信息的函数,
 * 使用 LCD 显示屏, 并通过按键输入处理多页信息的导航
 * 
 * @author kulib
 * @date 2025-11-04
 */

#ifndef ABOUT_H
#define ABOUT_H

#include "./hardware/button.h"
#include "./menu/menu.h"

/**
 * @brief 进入"关于我"界面（非阻塞，注册到状态机）
 */
void enterAboutMeInterface();

/**
 * @brief 进入"关于项目"界面（非阻塞，注册到状态机）
 */
void enterAboutProjectInterface();

/**
 * @brief 进入"构建信息"界面（非阻塞，注册到状态机）
 */
void enterBuildInfoInterface();

/**
 * @brief 处理"构建信息"界面逻辑，每帧调用一次
 */
void handleBuildInfoInterface();

/**
 * @brief 处理"关于项目"界面逻辑，每帧调用一次
 */
void handleAboutProjectInterface();

/**
 * @brief 处理"关于我"界面逻辑，每帧调用一次
 */
void handleAboutMeInterface();

#endif