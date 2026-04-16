#include "./menu/menu.h"
#include "./applications/clock.h"
#include "./applications/weather.h"
#include "./applications/pomodoro.h"
#include "./applications/badappleplayer.h"
#include "./applications/setting.h"
#include "./applications/about.h"

// 新应用接入最小步骤：
// 1) include 对应应用头；
// 2) 在 mainMenuItems 添加 APP_MENU_ITEM("Name", enterXxxInterface, true/false)；
// 3) 无需额外 wrapper，网络需求由第三个参数统一声明。

// 主菜单
const MenuItem mainMenuItems[] = {
    {"Wireless Screen",     enterWirelessScreenInterface, MENU_NONE},
    APP_MENU_ITEM("Clock",     enterClockInterface, false),
    APP_MENU_ITEM("Weather",   enterWeatherInterface, true),
    APP_MENU_ITEM("Pomodoro",  enterPomodoroInterface, false),
    {"Settings",            NULL, MENU_SETTINGS},
    {"About",               NULL, MENU_ABOUT},
    APP_MENU_ITEM("Bad Apple", enterBadAppleInterface, false)
};

// 设置菜单
const MenuItem settingsMenuItems[] = {
    {"Web setting",     _setupWebSetting, MENU_NONE},
    {"WiFi Config",     NULL, MENU_WIFI_CONFIG},
    {"Auto Bright",     _toggleAutoBrightness, MENU_NONE},
    {"Sound FX",        _toggleSoundEffects, MENU_NONE},
    {"Brightness",      enterBrightnessInterface, MENU_NONE},
    {"Battery info",    enterBatteryInfoInterface, MENU_NONE},
    {"Reset fuel IC",   _resetFuelGauge, MENU_NONE},
    {"Reboot",          _rebootSystem, MENU_NONE},
    {"Return",          NULL, MENU_MAIN}
};

// WiFi 配置菜单
const MenuItem wifiConfigMenuItems[] = {
    {"connect Info",    enterConnectInfoInterface, MENU_NONE},
    {"Reset Wifi",      _resetWifi, MENU_NONE},
    {"Return",          NULL, MENU_SETTINGS}
};

// 关于菜单
const MenuItem aboutMenuItems[] = {
    {"Build info",      enterBuildInfoInterface, MENU_NONE},
    {"About me",        enterAboutMeInterface, MENU_NONE},
    {"About Project",   enterAboutProjectInterface, MENU_NONE},
    {"Return",          NULL, MENU_MAIN}
};

// 全量菜单注册表，按 MenuState 顺序排列，便于通过状态索引访问。
Menu allMenus[] = {
    /* MENU_MAIN */ {
        mainMenuItems,
        sizeof(mainMenuItems) / sizeof(MenuItem)
    },

    /* MENU_SETTINGS */ {
        settingsMenuItems,
        sizeof(settingsMenuItems) / sizeof(MenuItem)
    },

    /* MENU_WIFI_CONFIG */ {
        wifiConfigMenuItems,
        sizeof(wifiConfigMenuItems) / sizeof(MenuItem)
    },

    /* MENU_ABOUT */ {
        aboutMenuItems,
        sizeof(aboutMenuItems) / sizeof(MenuItem)
    },
};
