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

#include "mydefine.h"
#include "button.h"
#include "menu.h"

/**
 * @brief 显示创建者信息
 * 
 * 此函数显示多页个人创建者信息,
 * 允许使用左右按键导航页面, 并使用中心按键退出
 */
void aboutMe();

/**
 * @brief 显示项目信息
 * 
 * 此函数显示多页项目相关信息,
 * 允许使用左右按键导航页面, 并使用中心按键退出
 */
void aboutProject();

#endif