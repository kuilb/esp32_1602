#ifndef MENU_H
#define MENU_H

#include "myhader.h"
#include "lcd_driver.h"
#include "network.h"
#include "wifi_config.h"
#include "button.h"
#include "clock.h"
#include "badappleplayer.h"
#include "weather.h"
#include "web_setting.h"

// 按钮索引枚举
enum ButtonIndex {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3,
    CENTER = 4
};

// 功能界面枚举
enum InterfaceState {
    STATE_MENU,
    STATE_BRIGHTNESS,
    STATE_CLOCK,
    STATE_WEATHER,
    STATE_OTHER
};

extern InterfaceState currentState;

// 菜单状态
typedef enum MenuState {
    MENU_NONE = -1,

    MENU_MAIN,
    MENU_SETTINGS,
    MENU_WIFI_CONFIG,
    MENU_ABOUT
} MenuState;

// 菜单栏的项目
typedef struct MenuItem {
    const char* name;                   // 显示的名称
    void (*action)();                   // 执行的操作（可为 NULL）
    MenuState nextState;                // 进入的下一级菜单
} MenuItem;

// 菜单
typedef struct Menu {
    const MenuItem* items;              // 菜单的内容
    int itemCount;                      // 内容数量
} Menu;


// 初始化菜单系统
void initMenu();

/**
 * @brief 判断是否经过了指定的时间间隔
 * 
 * @param startTime 起始时间，通常是 millis() 记录的时间戳
 * @param intervalMs 延迟时间（毫秒）
 * @return true 时间间隔已过
 * @return false 时间间隔未过
 */
bool hasElapsed(unsigned long startTime, unsigned long intervalMs);

// 在主 loop() 中调用：如果 inMenuMode 为 true，执行菜单处理逻辑
void handleMenuInterface();

// 外部访问菜单状态
extern volatile bool inMenuMode;

extern TaskHandle_t menuTaskHandle;
void menuTask(void* parameter);

#endif