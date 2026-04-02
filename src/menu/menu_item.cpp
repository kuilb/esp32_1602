#include "./menu/menu.h"
#include "./applications/setting.h"

// 菜单动作函数由 menu.cpp 提供实现，这里只做声明用于菜单表绑定。
void _enterWirelessScreen();
void _setClockInterface();
void _setWeatherInterface();
void _playBadAppleWrapper();

// 主菜单
const MenuItem mainMenuItems[] = {
    {"Wireless Screen",     _enterWirelessScreen, MENU_NONE},
    {"Clock",               _setClockInterface, MENU_NONE},
    {"Weather",             _setWeatherInterface, MENU_NONE},
    {"Settings",            NULL, MENU_SETTINGS},
    {"About",               NULL, MENU_ABOUT},
    {"Bad Apple",           _playBadAppleWrapper, MENU_NONE}
};

// 设置菜单
const MenuItem settingsMenuItems[] = {
    {"Web setting",     _setupWebSetting, MENU_NONE},
    {"WiFi Config",     NULL, MENU_WIFI_CONFIG},
    {"Auto Bright",     _toggleAutoBrightness, MENU_NONE},
    {"Brightness",      _enterBrightnessScreen, MENU_NONE},
    {"Battery info",    _enterBatteryInfoScreen, MENU_NONE},
    {"Reset fuel IC",   _resetFuelGauge, MENU_NONE},
    {"Reboot",          _rebootSystem, MENU_NONE},
    {"Return",          NULL, MENU_MAIN}
};

// WiFi 配置菜单
const MenuItem wifiConfigMenuItems[] = {
    {"connect Info",    _connectInfo, MENU_NONE},
    {"Reset Wifi",      _resetWifi, MENU_NONE},
    {"Return",          NULL, MENU_SETTINGS}
};

// 关于菜单
const MenuItem aboutMenuItems[] = {
    {"About me",        aboutMe, MENU_NONE},
    {"About Project",   aboutProject, MENU_NONE},
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
