/**
 * @file menu.h
 * @brief 菜单系统头文件，提供菜单显示和导航功能
 *
 * 此文件声明菜单系统的枚举、结构体和函数，用于ESP32设备的LCD菜单界面管理
 *
 * @author kulib
 * @date 2025-11-04
 */
#ifndef MENU_H
#define MENU_H

#include "myheader.h"
#include "lcd_driver.h"
#include "network.h"
#include "wifi_config.h"
#include "button.h"
#include "clock.h"
#include "badappleplayer.h"
#include "weather.h"
#include "web_setting.h"
#include "about.h"
#include "logger.h"
#include "button.h"
#include "wifi_config.h"

/** @brief 表示是否准备好显示 */
extern volatile bool isReadyToDisplay;

/** @brief 定义按钮的索引值 */
enum ButtonIndex {
    UP = 0,      /**< 上按钮 */
    DOWN = 1,    /**< 下按钮 */
    LEFT = 2,    /**< 左按钮 */
    RIGHT = 3,   /**< 右按钮 */
    CENTER = 4   /**< 中按钮 */
};

/** @brief 定义当前界面的状态 */
enum InterfaceState {
    STATE_MENU,         /**< 菜单状态 */
    STATE_BRIGHTNESS,   /**< 亮度设置状态 */
    STATE_CLOCK,        /**< 时钟状态 */
    STATE_WEATHER,      /**< 天气状态 */
    STATE_OTHER         /**< 其他状态 */
};

/** @brief 当前界面状态 */
extern InterfaceState currentState;

/** @brief 定义菜单的类型 */
typedef enum MenuState {
    MENU_NONE = -1,     /**< 无菜单 */
    MENU_MAIN,          /**< 主菜单 */
    MENU_SETTINGS,      /**< 设置菜单 */
    MENU_WIFI_CONFIG,   /**< WiFi 配置菜单 */
    MENU_ABOUT          /**< 关于菜单 */
} MenuState;

/** @brief 定义菜单项的属性 */
typedef struct MenuItem {
    const char* name;                   /**< 显示的名称 */
    void (*action)();                   /**< 执行的操作（可为 NULL） */
    MenuState nextState;                /**< 进入的下一级菜单 */
} MenuItem;

/** @brief 定义菜单的整体结构 */
typedef struct Menu {
    const MenuItem* items;              /**< 菜单的内容 */
    int itemCount;                      /**< 内容数量 */
} Menu;

/** @brief 初始化菜单系统, 设置菜单的初始状态 */
void initMenu();

/** @brief 在主 loop() 中调用, 如果 inMenuMode 为 true, 执行菜单处理逻辑 */
void handleMenuInterface();

extern volatile bool inMenuMode;        /**< 菜单模式标志 */
extern TaskHandle_t _menuTaskHandle;    /**< 菜单任务句柄 */

#endif