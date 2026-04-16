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

/**
 * @brief 界面处理函数指针类型
 * 每帧由 _menuTask 调用一次
 */
typedef void (*InterfaceHandler)();

/**
 * @brief 切换到指定子界面（注册当前帧处理函数）
 * @param handler 界面处理函数，NULL 等同于 clearCurrentInterface()
 */
void setCurrentInterface(InterfaceHandler handler);

/**
 * @brief 返回菜单（清除当前子界面）
 */
void clearCurrentInterface();

/**
 * @brief 是否处于子界面（非菜单）中
 */
bool isInSubInterface();

/**
 * @brief 统一进入应用界面（非阻塞状态机）
 * @param handler 应用处理函数
 * @param networkRequired 该应用界面是否需要联网
 */
void enterAppInterface(InterfaceHandler handler, bool networkRequired);

/**
 * @brief 统一退出应用界面并回到菜单
 * @param delayMs 退出后按键防抖延迟
 */
void exitAppInterface(unsigned long delayMs = FIRST_TIME_DELAY);

/**
 * @brief 通用处理中键返回：检测到 CENTER 后退出当前应用界面
 * @param debounceMs 按键去抖时间
 * @param delayMs 退出后按键防抖延迟
 * @return true 已触发退出
 */
bool appHandleCenterExit(unsigned long debounceMs = BUTTON_DEBOUNCE_DELAY,
                         unsigned long delayMs = FIRST_TIME_DELAY);

/**
 * @brief 通用处理 LEFT/RIGHT 单步移动（分页、游标等）
 * @param value 当前值（引用）
 * @param minValue 最小值（含）
 * @param maxValue 最大值（含）
 * @param step 单次步长（默认 1）
 * @param debounceMs 按键去抖时间
 * @return true 值发生变化
 */
bool appHandleLeftRightStep(int& value,
                            int minValue,
                            int maxValue,
                            int step = 1,
                            unsigned long debounceMs = BUTTON_DEBOUNCE_DELAY);

/**
 * @brief 通用周期触发器：到达时间间隔后返回 true，并自动更新 lastRunMs
 * @param lastRunMs 上次执行时间戳（会被更新）
 * @param intervalMs 周期间隔（毫秒）
 * @return true 需要执行周期任务
 */
bool appShouldRunPeriodic(unsigned long& lastRunMs, unsigned long intervalMs);

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

// 前置声明：避免头文件循环包含时模板内调用找不到符号。
void setAppInterfaceNetworkRequired(bool required);
bool hasAppInterfaceNetworkRequirementOverride();

/**
 * @brief 轻量应用菜单动作模板
 * @details
 * 用于把“应用注册”压缩为三项：名称 + enter 函数 + 是否联网。
 * enterFn 执行后仅在应用未显式声明联网需求时按注册项兜底设置，
 * 不覆盖应用内部动态策略。
 */
template<void (*EnterFn)(), bool NetworkRequired>
inline void menuLaunchRegisteredApp() {
    EnterFn();
    if (isInSubInterface() && !hasAppInterfaceNetworkRequirementOverride()) {
        setAppInterfaceNetworkRequired(NetworkRequired);
    }
}

/**
 * @brief 应用菜单项宏：只需填写 名称 + enter函数 + 是否联网
 */
#define APP_MENU_ITEM(NAME, ENTER_FN, NETWORK_REQUIRED) \
    { NAME, menuLaunchRegisteredApp<ENTER_FN, NETWORK_REQUIRED>, MENU_NONE }

extern const MenuItem mainMenuItems[];       /**< 主菜单项数组 */
extern const MenuItem settingsMenuItems[];   /**< 设置菜单项数组 */
extern const MenuItem wifiConfigMenuItems[]; /**< WiFi配置菜单项数组 */
extern const MenuItem aboutMenuItems[];      /**< 关于菜单项数组 */
extern Menu allMenus[];                      /**< 全量菜单注册表 */

/** @brief 进入无线推流界面（菜单动作入口） */
void enterWirelessScreenInterface();

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

/**
 * @brief 不弹历史栈，直接回到当前子菜单（用于应用内返回）
 */
void menuReturnToCurrentSubMenu();

extern volatile bool inMenuMode;        /**< 菜单模式标志 */
extern volatile bool wirelessScreenActive; /**< 无线屏幕活动标志（用于电源/射频策略判定） */
extern TaskHandle_t _menuTaskHandle;    /**< 菜单任务句柄 */

#endif