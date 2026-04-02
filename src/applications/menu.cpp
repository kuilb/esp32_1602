#include "./applications/menu.h"
#include "./services/auto_brightness.h"

// 前置声明
void _displayMenu(const Menu* menu, int menuIndex, int scrollOffset);
bool _checkStateChanges();

void _menuTask(void* parameter);

// =====================
// 内部辅助函数（仅本文件使用）
// 目的：把“渲染/输入/滚动”拆开，降低嵌套与分支复杂度。
// 说明：这里只做前置声明；实现放在全局变量/菜单表定义之后。
// =====================

static inline bool _isMainMenu(const Menu* menu);
static int _readMenuButtonOnce();
static void _formatStatusBarTime(char* outBuf, size_t outBufLen);
static void _renderMainMenuStatusBar();
static void _renderMenuItemLine(const Menu* menu, int menuItemIndex, int visibleIndex, int lcdLine);
static void _followCursorToKeepVisible(const Menu* menu);
static bool _shouldHideMenuItem(const Menu* menu, int menuItemIndex);
static int _getVisibleMenuItemCount(const Menu* menu);
static int _getActualMenuIndexFromVisible(const Menu* menu, int visibleIndex);
static void _syncMenuNavigationState();
static void _switchToMenu(const Menu* menu);
static void _moveMenuCursorUp();
static void _moveMenuCursorDown();
static void _activateCurrentMenuItem();

enum SettingsMenuIndex {
    SETTINGS_ITEM_WEB = 0,
    SETTINGS_ITEM_WIFI_CONFIG,
    SETTINGS_ITEM_AUTO_BRIGHTNESS,
    SETTINGS_ITEM_BRIGHTNESS,
    SETTINGS_ITEM_BATTERY_INFO,
    SETTINGS_ITEM_RESET_FUEL_IC,
    SETTINGS_ITEM_REBOOT,
    SETTINGS_ITEM_RETURN
};

// 菜单状态变量
static bool isNewInterface = false;
volatile bool inMenuMode = true;
volatile bool isReadyToDisplay = false;
static bool isDisplayNeedsUpdate = true;
static TimeSyncState lastTimeSyncState = TIME_SYNC_IDLE;

// 电量缓存（避免频繁I2C读取）
static uint8_t cachedBatterySOC = 0;
static int16_t cachedCurrentBattery_mA = 0;
static uint16_t cachedBatteryVoltage_mV = 0;
static unsigned long lastBatteryRead = 0;
#define BATTERY_READ_INTERVAL 10000  // 10秒更新一次电量

long lastWeatherFail = -15000;
InterfaceState currentState = STATE_MENU;
WiFiConnectionState currentWiFiState = WIFI_IDLE;
tm currentTimeInfo;

extern WifiConfigManager wifiConfigManager;

static auto wifianim = Animations::getAnimation("wifi_searching");
static auto spinneranim = Animations::getAnimation("spinner");
static auto wronganim = Animations::getAnimation("wrong");

void _enterWirelessScreen(){
    LOG_MENU_INFO("Entering Wireless Screen");
    inMenuMode = false;

    // WiFi 未连接时，启动配网
    if (WiFi.status() != WL_CONNECTED) {
        LOG_MENU_INFO("Starting AP config mode from menu");
        enterConfigMode();
    }
    // WiFi 已连接时，显示连接信息
    else{
        lcdText("SSID:" + wifiConfigManager.getSSID(),1);
        lcdText("IP:" + WiFi.localIP().toString(),2);
    }
}

void _enterBrightnessScreen() {
    lcdText("Brightness:", 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
    lcdText(buf, 2);

    int lastBrightness = brightness;

    while (!isButtonReadyToRespond(CENTER)) {
        if (isButtonReadyToRespond(LEFT, 10)) {
            changeBrightness(-1);
        }
        if (isButtonReadyToRespond(RIGHT, 10)) {
            changeBrightness(1);
        }

        // 只有亮度变化才重绘文字
        if (brightness != lastBrightness) {
            snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
            lcdText("Brightness:", 1);
            lcdText(buf, 2);
            lastBrightness = brightness;
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 节流
    }

    lcdText("Brightness saved", 1);
    lcdText("Back to Menu", 2);
    LOG_SYSTEM_INFO("Brightness set to %d%%", (brightness * 100 + 127) / 255);
    delay(500);
}

void _toggleAutoBrightness() {
    bool isEnabled = toggleAutoBrightness();
    lcdText("Auto Brightness", 1);
    lcdText(isEnabled ? "ON" : "OFF", 2);
    LOG_SYSTEM_INFO("Auto brightness %s", isEnabled ? "enabled" : "disabled");
    delay(500);
}

void _resetWifi(){
    inMenuMode = false;
    SPIFFS.remove("/wifi.txt");
    lcdText("WiFi cleared", 1);
    lcdText("Rebooting...", 2);
    LOG_SYSTEM_INFO("WiFi config cleared, restarting...");
    delay(800);
    ESP.restart();
}

void _resetFuelGauge(){
    // Perform the reset as a menu action (wrapper for the API call)
    setBQ27421DesignCapacity(BATTERY_DESIGN_CAPACITY_MAH);
    lcdText("Fuel gauge reset", 1);
    lcdText("Back to Menu", 2);
    LOG_SYSTEM_INFO("Fuel gauge design capacity set to %d mAh", BATTERY_DESIGN_CAPACITY_MAH);
    delay(500);
}

void _setupWebSetting(){
    webSettingSetupWebServer();
    LOG_WEB_INFO("Web configured");
}

void _connectInfo(){
    if (WiFi.status() == WL_CONNECTED) {
        lcdText("SSID:" + wifiConfigManager.getSSID(), 1);
        lcdText("IP:" + WiFi.localIP().toString(), 2);
    } 
    else {
        lcdText("Not Connected", 1);
        lcdText("", 2);
    }

    for(;;){
        if(isButtonReadyToRespond(CENTER)){
            currentState = STATE_MENU;
            return;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void _setClockInterface(){
    isNewInterface = true;
    currentState = STATE_CLOCK;
}

void _setWeatherInterface(){
    isNewInterface = true;
    currentState = STATE_WEATHER;
}

void _playBadAppleWrapper() {
    playBadAppleFromFileRaw("/badapple.bin");
}

void _enterBatteryInfoScreen() {
    // 使用 unsigned long，避免 millis() 溢出时比较错误
    unsigned long lastBatteryUpdate = millis() - 10000; // 强制首次更新
    while(true){
        // 注意：必须是 millis() - last >= interval
        if(millis() - lastBatteryUpdate >= 10000 && isfuelICConnected){
            readBatteryInfo();
            uint16_t voltage = readVoltage();
            int16_t current = readAverageCurrent();
            uint8_t soc = readStateOfCharge();
            uint16_t remainingCap = readRemainingCapacity();

            lcdText("" + String(voltage) + "mV " + String(current) + "mA", 1);
            lcdText(String(soc) + "% " + String(remainingCap) + "mAh", 2);

            lastBatteryUpdate = millis();
        }
        if(isButtonReadyToRespond(CENTER)){
            currentState = STATE_MENU;
            return;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void _rebootSystem(){
    lcdText("Rebooting...", 1);
    lcdText("", 2);
    LOG_SYSTEM_INFO("System rebooting...");
    delay(400);
    ESP.restart();
}

const MenuItem mainMenuItems[] = {
    {"Wireless Screen",     _enterWirelessScreen, MENU_NONE},       // 无线屏幕
    {"Clock",               _setClockInterface, MENU_NONE},         // 时钟
    {"Weather",             _setWeatherInterface, MENU_NONE},       // 天气
    {"Settings",            NULL, MENU_SETTINGS},                   // 设置
    {"About",               NULL, MENU_ABOUT},                      // 关于
    {"Bad Apple",           _playBadAppleWrapper, MENU_NONE}        // Bad Apple
};

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

const MenuItem wifiConfigMenuItems[] = {
    {"connect Info",    _connectInfo, MENU_NONE},
    {"Reset Wifi",      _resetWifi, MENU_NONE},
    {"Return",          NULL, MENU_SETTINGS}
};

const MenuItem aboutMenuItems[] = {
    {"About me",        aboutMe, MENU_NONE},
    {"About Project",   aboutProject, MENU_NONE},
    {"Return",          NULL, MENU_MAIN}
};

Menu allMenus[] = {
    /* MENU_MAIN */     { 
        mainMenuItems, 
        sizeof(mainMenuItems)/sizeof(MenuItem) 
    },

    /* MENU_SETTINGS */ { 
        settingsMenuItems, 
        sizeof(settingsMenuItems)/sizeof(MenuItem) 
    },

    /* MENU_WIFI_CONFIG */ { 
        wifiConfigMenuItems, 
        sizeof(wifiConfigMenuItems)/sizeof(MenuItem) 
    },

    /* MENU_ABOUT */ { 
        aboutMenuItems, 
        sizeof(aboutMenuItems)/sizeof(MenuItem) 
    },
};


// 菜单显示部分
const Menu* currentMenu = &allMenus[MENU_MAIN]; // 初始为主菜单
int menuCursor = 0;         // 当前菜单项光标位置
int scrollOffset = -1;      // 当前显示窗口起始项，-1为状态栏

// =====================
// 内部辅助函数实现
// =====================

static inline bool _isMainMenu(const Menu* menu) {
    return menu == &allMenus[MENU_MAIN];
}

// 读取一次“有效按键”（带去抖），未检测到返回 -1
static int _readMenuButtonOnce() {
    for (int i = 0; i < 5; i++) {
        if (isButtonReadyToRespond(i, BUTTON_DEBOUNCE_DELAY)) {
            return i;
        }
    }
    return -1;
}

// 生成状态栏时间字符串：固定6字符，右对齐（" HH:MM" / " --:--"）
static void _formatStatusBarTime(char* outBuf, size_t outBufLen) {
    if (!outBuf || outBufLen == 0) return;

    // 需要容纳 6 字符 + '\0'
    if (outBufLen < 7) {
        outBuf[0] = '\0';
        return;
    }

    char hmBuf[6] = {0}; // "HH:MM" + '\0'

    // TIME_SYNC_SUCCESS：直接显示本地时间
    if (timeSyncState == TIME_SYNC_SUCCESS) {
        strftime(hmBuf, sizeof(hmBuf), "%H:%M", &localTimeInfo);
        snprintf(outBuf, outBufLen, " %s", hmBuf);
        return;
    }

    // 未同步成功：尝试使用 RTC
    if (getRtcTime().tv_sec > 1765967312) { // 2025-12-17 18:40 GMT+8
        time_t rtcTime = getRtcTime().tv_sec;

        // 仅在 IDLE（尚未配置时区）时，手动补时区偏移
        if (timeSyncState == TIME_SYNC_IDLE) {
            rtcTime += GMT_OFFSET_HOUR * 3600;
        }

        localtime_r(&rtcTime, &currentTimeInfo);
        strftime(hmBuf, sizeof(hmBuf), "%H:%M", &currentTimeInfo);
        snprintf(outBuf, outBufLen, " %s", hmBuf);
        return;
    }

    strncpy(outBuf, " --:--", outBufLen);
    outBuf[outBufLen - 1] = '\0';
}

// 渲染主菜单状态栏（占用一行）
static void _renderMainMenuStatusBar() {
    lcdSetCursor(0);
    lcdResetCharSlot();

    // 电池状态 6字符 (2图标 + 4文本)
    if(isfuelICConnected && cachedBatteryVoltage_mV < 4500){
        lcdCreateCharAuto(SystemIcons::getBatteryLeftIcon(cachedBatterySOC));
        lcdCreateCharAuto(SystemIcons::getBatteryRightIcon(cachedBatterySOC));

        char tempBuf[5];
        char batteryBuf[5] = {0};
        snprintf(tempBuf, sizeof(tempBuf), "%d%%", cachedBatterySOC);  // 先格式化为 "9%" 或 "99%" 或 "100%"
        
        if (cachedCurrentBattery_mA > 0 && cachedBatterySOC != 100) {
            lcdCreateCharAuto(SystemIcons::getIcon("battery_charging"));
            snprintf(batteryBuf, sizeof(batteryBuf), "%-3s", tempBuf);  // 左对齐，右补空格到3字符
        } else {
            snprintf(batteryBuf, sizeof(batteryBuf), "%-4s", tempBuf);  // 左对齐，右补空格到4字符
        }
        lcdPrint(batteryBuf);
    }
    else{
        lcdCreateCharAuto(SystemIcons::getIcon("dc_left"));
        lcdCreateCharAuto(SystemIcons::getIcon("dc_right"));
        lcdPrint("DC  ");
    }

    // 组件间空 1 格：电量(6) + 空格(1) + WiFi(2) + 空格(1) + 时间(6) = 16
    lcdPrint(" ");

    // WiFi 状态 2字符（中间区域）
    switch (wifiConnectionState) {
        case WIFI_CONNECTING:
            lcdCreateCharAuto(spinneranim->frames[spinneranim->currentFrame]);
            lcdCreateCharAuto(wifianim->frames[wifianim->currentFrame]);
            break;
        case WIFI_CONNECTED:
            lcdPrint(" ");
            lcdCreateCharAuto(SystemIcons::getIcon("wifi"));
            break;
        case WIFI_DISCONNECTED:
            lcdCreateCharAuto(spinneranim->frames[spinneranim->currentFrame]);
            lcdCreateCharAuto(SystemIcons::getIcon("wifi"));
            break;
        case WIFI_FAILED:
            lcdCreateCharAuto(wronganim->frames[wronganim->currentFrame]);
            lcdCreateCharAuto(SystemIcons::getIcon("wifi"));
            break;
        case WIFI_IDLE:
            lcdPrint(" ");
            lcdCreateCharAuto(spinneranim->frames[spinneranim->currentFrame]);
            break;
        default:
            {
                static int lastUnknownWiFiState = -1;
                static unsigned long lastUnknownWiFiLogMs = 0;
                int stateVal = (int)wifiConnectionState;
                if (stateVal != lastUnknownWiFiState && millis() - lastUnknownWiFiLogMs > 1000) {
                    LOG_MENU_WARN("Unknown wifiConnectionState=%d", stateVal);
                    lastUnknownWiFiState = stateVal;
                    lastUnknownWiFiLogMs = millis();
                }
                lcdCreateCharAuto(wronganim->frames[wronganim->currentFrame]);
                lcdPrint(" ");
            }
            break;
    }

    // 组件间空 1 格
    lcdPrint(" ");

    // 时间 6字符（右对齐）
    char timeBuf[8] = {0};
    _formatStatusBarTime(timeBuf, sizeof(timeBuf));
    lcdPrint(timeBuf);
}

// 渲染普通菜单项一行（带 > 光标）
static void _renderMenuItemLine(const Menu* menu, int menuItemIndex, int visibleIndex, int lcdLine) {
    if (!menu || menuItemIndex < 0 || menuItemIndex >= menu->itemCount) {
        lcdText(" ", lcdLine);
        return;
    }

    String itemName = menu->items[menuItemIndex].name;

    // 主菜单第0项：根据 WiFi 状态动态显示名称
    if (_isMainMenu(menu) && menuItemIndex == 0) {
        itemName = (wifiConnectionState == WIFI_CONNECTED) ? "Wireless Screen" : "Config WiFi";
    }

    if (menuCursor == visibleIndex) {
        lcdText(">" + itemName, lcdLine);
    } else {
        lcdText(" " + itemName, lcdLine);
    }
}

static bool _shouldHideMenuItem(const Menu* menu, int menuItemIndex) {
    if (!menu || menuItemIndex < 0 || menuItemIndex >= menu->itemCount) {
        return false;
    }

    // 设置菜单中：启用自动亮度时隐藏手动亮度调节项
    if (menu == &allMenus[MENU_SETTINGS]
        && menuItemIndex == SETTINGS_ITEM_BRIGHTNESS
        && isAutoBrightnessActive()) {
        return true;
    }

    return false;
}

static int _getVisibleMenuItemCount(const Menu* menu) {
    if (!menu) return 0;

    int visibleCount = 0;
    for (int i = 0; i < menu->itemCount; ++i) {
        if (!_shouldHideMenuItem(menu, i)) {
            visibleCount++;
        }
    }
    return visibleCount;
}

static int _getActualMenuIndexFromVisible(const Menu* menu, int visibleIndex) {
    if (!menu || visibleIndex < 0) return -1;

    int currentVisibleIndex = 0;
    for (int i = 0; i < menu->itemCount; ++i) {
        if (_shouldHideMenuItem(menu, i)) {
            continue;
        }

        if (currentVisibleIndex == visibleIndex) {
            return i;
        }
        currentVisibleIndex++;
    }

    return -1;
}

// 根据光标位置，自动修正 scrollOffset（仅用于 scrollOffset>=0 的模式）
static void _followCursorToKeepVisible(const Menu* menu) {
    if (!menu) return;
    if (scrollOffset < 0) return; // -1 表示主菜单显示状态栏，不在这里处理

    int visibleCount = _getVisibleMenuItemCount(menu);
    if (visibleCount <= 0) {
        menuCursor = 0;
        scrollOffset = 0;
        return;
    }

    menuCursor = constrain(menuCursor, 0, visibleCount - 1);

    if (menuCursor < scrollOffset) {
        scrollOffset = menuCursor;
    } else if (menuCursor >= scrollOffset + VISIBLE_LINES) {
        scrollOffset = menuCursor - VISIBLE_LINES + 1;
    }

    // 防止越界
    int maxScrollOffset = visibleCount - VISIBLE_LINES;
    if (maxScrollOffset < 0) maxScrollOffset = 0;
    scrollOffset = constrain(scrollOffset, 0, maxScrollOffset);
}

// 初始化
void initMenu() {
    if(inConfigMode) return; // 配置模式不启动菜单任务

    // 注册动画
    Animations::registerAnimation(*wifianim);
    Animations::registerAnimation(*spinneranim);
    Animations::registerAnimation(*wronganim);

    Animations::setEnable("wifi_searching", true);
    Animations::setEnable("spinner", true);
    Animations::setEnable("wrong", true);

    xTaskCreate(_menuTask, "_menuTask", 16384, NULL, 1, &_menuTaskHandle);
}

void _displayMenu(const Menu* menu, int menuIndex, int scrollOffset) {
    // menuIndex / scrollOffset 由调用者传入（menuCursor/scrollOffset 是全局状态），这里保持参数以便调试
    lcdResetCursor();
    
    // 检查是否为主菜单，如果是则显示状态栏
    if (menu == &allMenus[MENU_MAIN]) {
        // 主菜单：状态栏作为第 -1 项（虚拟项），可滚动但不可选中
        for (int i = 0; i < VISIBLE_LINES; i++) {
            int displayIndex = scrollOffset + i;
            
            if (displayIndex == -1) {
                // 状态栏固定渲染在第 1 行
                _renderMainMenuStatusBar();
            } else {
                // 普通菜单项
                _renderMenuItemLine(menu, displayIndex, displayIndex, i + 1);
            }
        }
    } else {
        // 其他菜单：纯列表显示，不包含状态栏
        for (int i = 0; i < VISIBLE_LINES; i++) {
            int visibleIndex = scrollOffset + i;
            int menuItemIndex = _getActualMenuIndexFromVisible(menu, visibleIndex);

            if (menuItemIndex < 0) {
                lcdText(" ", i + 1);
                continue;
            }

            _renderMenuItemLine(menu, menuItemIndex, visibleIndex, i + 1);
        }
    }
}

// 根据菜单枚举获取对应菜单结构体指针
const Menu* _getMenuByState(MenuState state) {
    // MenuState 与 allMenus 的顺序一致，直接按索引取更简单
    if (state >= MENU_MAIN && state <= MENU_ABOUT) {
        return &allMenus[(int)state];
    }
    return NULL;
}

static void _syncMenuNavigationState() {
    if (!currentMenu) return;

    const bool isMain = _isMainMenu(currentMenu);
    const int visibleItemCount = _getVisibleMenuItemCount(currentMenu);

    if (visibleItemCount <= 0) {
        menuCursor = 0;
        scrollOffset = isMain ? -1 : 0;
        return;
    }

    menuCursor = constrain(menuCursor, 0, visibleItemCount - 1);

    if (isMain) {
        // 主菜单允许 -1（显示状态栏）
        if (scrollOffset < -1) {
            scrollOffset = -1;
        }
    } else if (scrollOffset < 0) {
        scrollOffset = 0;
    }

    _followCursorToKeepVisible(currentMenu);
}

static void _switchToMenu(const Menu* menu) {
    if (!menu) return;

    currentMenu = menu;
    menuCursor = 0;
    scrollOffset = _isMainMenu(currentMenu) ? -1 : 0;
    _syncMenuNavigationState();
    isDisplayNeedsUpdate = true;
}

static void _moveMenuCursorUp() {
    _syncMenuNavigationState();

    const bool isMain = _isMainMenu(currentMenu);
    if (isMain) {
        if (menuCursor > 0) {
            menuCursor--;
        } else if (scrollOffset > -1) {
            scrollOffset = -1;
        }
    } else if (menuCursor > 0) {
        menuCursor--;
    }

    _syncMenuNavigationState();
    isDisplayNeedsUpdate = true;
}

static void _moveMenuCursorDown() {
    _syncMenuNavigationState();

    const int visibleItemCount = _getVisibleMenuItemCount(currentMenu);
    if (visibleItemCount <= 0) {
        isDisplayNeedsUpdate = true;
        return;
    }

    if (_isMainMenu(currentMenu) && scrollOffset == -1 && menuCursor == 0) {
        // 主菜单状态栏可见时，先收起状态栏
        scrollOffset = 0;
    } else {
        menuCursor = constrain(menuCursor + 1, 0, visibleItemCount - 1);
    }

    _syncMenuNavigationState();
    isDisplayNeedsUpdate = true;
}

static void _activateCurrentMenuItem() {
    _syncMenuNavigationState();

    int actualMenuIndex = _getActualMenuIndexFromVisible(currentMenu, menuCursor);
    if (actualMenuIndex < 0) {
        isDisplayNeedsUpdate = true;
        return;
    }

    const MenuItem& item = currentMenu->items[actualMenuIndex];

    if (item.nextState != MENU_NONE) {
        const Menu* nextMenu = _getMenuByState(item.nextState);
        if (nextMenu) {
            _switchToMenu(nextMenu);
            globalButtonDelay(FIRST_TIME_DELAY);  // 切换菜单后防抖
            return;
        }
    }

    if (item.action) {
        item.action();  // 触发动作
        globalButtonDelay(FIRST_TIME_DELAY);  // 执行动作后防抖
    }

    _syncMenuNavigationState();
    isDisplayNeedsUpdate = true;
}

void _handleMenuInterface() {
    _syncMenuNavigationState();

    // 显示菜单项
    if (isDisplayNeedsUpdate || _checkStateChanges()) {
        _displayMenu(currentMenu, menuCursor, scrollOffset);
        isDisplayNeedsUpdate = false;
    }

    // 只处理一个键：一次循环最多响应一次输入
    int lastPressedButton = _readMenuButtonOnce();

    switch (lastPressedButton) {
        // 光标上移（LEFT）
        case LEFT:
            _moveMenuCursorUp();
            break;

        // 光标下移（RIGHT）
        case RIGHT:
            _moveMenuCursorDown();
            break;

        case CENTER:
            _activateCurrentMenuItem();
            break;

        default:
            break;
    }
}

bool _ensureisTimeSynced() {
    if (!(timeSyncState == TIME_SYNC_SUCCESS) && !(getRtcTime().tv_sec > 1765967312)) { // 2025-12-17 18-40 GMT+8
        lcdText("Try time sync", 1);
        lcdText("Please wait", 2);
        updateTimeSync();
        if (timeSyncState != TIME_SYNC_SUCCESS) {
            LOG_MENU_WARN("Time not synced yet, cannot display");
            lcdText("Time not synced", 1);
            lcdText("", 2);
            delay(500);
            return false;
        }
    }
    return true;
}

// 检查状态变化，返回是否需要更新显示
bool _checkStateChanges() {
    // 检查WiFi状态变化
    if (currentWiFiState != wifiConnectionState) {
        currentWiFiState = wifiConnectionState;
        return true;
    }
    
    // 检查时间同步状态变化
    if (lastTimeSyncState != timeSyncState) {
        lastTimeSyncState = timeSyncState;
        return true;
    }

    // 只有在时间同步成功后才检查时间变化
    if (timeSyncState == TIME_SYNC_SUCCESS) {
        getLocalTime(&localTimeInfo);
        if (currentTimeInfo.tm_min != localTimeInfo.tm_min) {
            currentTimeInfo = localTimeInfo;
            return true;
        }
    }
    
    return false;
}

// 菜单任务
TaskHandle_t _menuTaskHandle = NULL;
void _menuTask(void* parameter) {
    static unsigned long lastDisplayUpdate = 0;
    
    // 初始读取电量
    cachedBatterySOC = readStateOfCharge();
    cachedCurrentBattery_mA = readAverageCurrent();
    cachedBatteryVoltage_mV = readVoltage();
    LOG_SYSTEM_DEBUG("Initial battery: %d%%, %dmA, %dmV", cachedBatterySOC, cachedCurrentBattery_mA, cachedBatteryVoltage_mV);

    while (!shouldExitTasks) {
        // 定期更新电量缓存（避免频繁I2C读取）
        if (millis() - lastBatteryRead > BATTERY_READ_INTERVAL) {
            cachedBatterySOC = readStateOfCharge();
            cachedCurrentBattery_mA = readAverageCurrent();
            cachedBatteryVoltage_mV = readVoltage();
            lastBatteryRead = millis();
            // 如果状态栏可见，标记需要更新显示
            if (currentState == STATE_MENU && scrollOffset == -1) {
                isDisplayNeedsUpdate = true;
            }
        }
        
        // 持续更新动画
        Animations::update();
        
        if (inMenuMode) {
            switch (currentState) {
                case STATE_MENU:
                    _handleMenuInterface();
                    if(scrollOffset == -1 && Animations::getNeedToUpdate()) isDisplayNeedsUpdate = true;
                    break;

                case STATE_CLOCK:
                    // 先处理退出按键，避免在未同步时因前置校验导致无法返回
                    if(isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)){
                        LOG_MENU_INFO("exit to main menu");
                        currentState = STATE_MENU;
                        globalButtonDelay(FIRST_TIME_DELAY);  // 状态切换防抖
                        break;
                    }

                    // 时间不可用时，自动返回主菜单，避免卡在 CLOCK 状态反复告警
                    if(!_ensureisTimeSynced()) {
                        currentState = STATE_MENU;
                        globalButtonDelay(FIRST_TIME_DELAY);
                        break;
                    }

                    // 每隔1秒刷新一次时间显示
                    if (millis() - lastDisplayUpdate > 1000 || isNewInterface) {
                        isNewInterface = false;
                        updateClockScreen();
                        lastDisplayUpdate = millis();
                    }

                    break;

                case STATE_WEATHER: 
                    if(!_ensureisTimeSynced()){
                        currentState = STATE_MENU;
                        globalButtonDelay(FIRST_TIME_DELAY);  // 状态切换防抖
                        break;
                    }

                    // 每隔10分钟更新一次天气数据
                    if (!isReadyToDisplay || millis() - lastWeatherUpdate > 10*60*1000) {
                        if (millis() - lastWeatherFail > 15*1000) { // 失败后15秒再试
                            loadJwtConfig();
                            LOG_MENU_INFO("Fetching weather data...");
                            if(fetchWeatherData()) {
                                isReadyToDisplay = true;
                                lastWeatherUpdate = millis();
                            } else {
                                LOG_WEATHER_WARN("Failed to fetch weather data");
                                isReadyToDisplay = false;
                                lastWeatherFail = millis();
                            }
                        }
                    }

                    if (isNewInterface) {
                        isNewInterface = false;
                        if(isReadyToDisplay) {
                            updateWeatherScreen();
                        }
                        lastDisplayUpdate = millis();
                    }

                    // 始终检查是否需要退出
                    if(isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)){
                        LOG_MENU_INFO("exit to main menu");
                        currentState = STATE_MENU;
                        globalButtonDelay(FIRST_TIME_DELAY);  // 状态切换防抖
                        break;
                    }
                    if(isReadyToDisplay == false){
                        lcdText("No Data", 1);
                        lcdText(" ", 2);
                    }
                    else{
                        if(isButtonReadyToRespond(LEFT, BUTTON_DEBOUNCE_DELAY)){
                            lastDisplayUpdate = millis();
                            interface_num = (interface_num + 3) % 4; // 切换到上一个界面
                            updateWeatherScreen();
                        }
                        if(isButtonReadyToRespond(RIGHT, BUTTON_DEBOUNCE_DELAY)){
                            lastDisplayUpdate = millis();
                            interface_num = (interface_num + 1) % 4; // 切换到下一个界面
                            updateWeatherScreen();
                        }
                    }
                    break;
            }
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);  // 快速刷新提升响应
    }
    
    // 任务退出清理
    LOG_MENU_DEBUG("_menuTask exiting...");
    _menuTaskHandle = NULL;
    vTaskDelete(NULL);
}