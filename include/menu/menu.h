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

#include "./applications/clock.h"
#include "./applications/badappleplayer.h"
#include "./applications/weather.h"
#include "./applications/about.h"
#include "./applications/pomodoro.h"

#include "./connectivity/network.h"
#include "./connectivity/wifi_config.h"

#include "./hardware/lcd_driver.h"
#include "./hardware/button.h"
#include "./hardware/button.h"
#include "./hardware/fuel_gauge.h"

#include "./services/wifi_config_manager.h"
#include "./services/web_setting.h"
#include "./services/time_manager.h"
#include "./services/sleep_manager.h"

#include "./ui/icons.h"
#include "./ui/animations.h"
#include "./utils/logger.h"

/** @brief 表示是否准备好显示 */
extern volatile bool isReadyToDisplay;

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

/** @brief 菜单上下文（导航与显示状态） */
typedef struct MenuContext {
    const Menu* currentMenu;            /**< 当前菜单指针 */
    int menuCursor;                     /**< 当前光标在可见项中的索引 */
    int scrollOffset;                   /**< 当前滚动偏移（主菜单可为 -1） */
    bool isDisplayNeedsUpdate;          /**< 是否需要重绘菜单 */
} MenuContext;

extern const MenuItem mainMenuItems[];       /**< 主菜单项数组 */
extern const MenuItem settingsMenuItems[];   /**< 设置菜单项数组 */
extern const MenuItem wifiConfigMenuItems[]; /**< WiFi配置菜单项数组 */
extern const MenuItem aboutMenuItems[];      /**< 关于菜单项数组 */
extern Menu allMenus[];                      /**< 全量菜单注册表 */

/** @brief 初始化菜单系统, 设置菜单的初始状态 */
void initMenu();

/** @brief 在主 loop() 中调用, 如果 inMenuMode 为 true, 执行菜单处理逻辑 */
void handleMenuInterface();

/**
 * @brief 处理“返回上一级”动作
 * @details
 * - 菜单内：返回父菜单（例如 WIFI_CONFIG -> SETTINGS -> MAIN）
 * - 非菜单界面（如无线显示/应用界面）：回到主菜单
 */
void menuHandleBackAction();

extern volatile bool inMenuMode;        /**< 菜单模式标志 */
extern TaskHandle_t _menuTaskHandle;    /**< 菜单任务句柄 */

#endif