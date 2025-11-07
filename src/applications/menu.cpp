#include "menu.h"

// 前置声明
void _displayMenu(const Menu* menu, int menuIndex, int scrollOffset);
bool _checkStateChanges();

void _menuTask(void* parameter);

// 菜单状态变量
static bool isNewInterface = false;
volatile bool inMenuMode = true;
volatile bool isReadyToDisplay = false;
static bool isDisplayNeedsUpdate = true;
static TimeSyncState lastTimeSyncState = TIME_SYNC_IDLE;

long lastWeatherFail = -15000;
InterfaceState currentState = STATE_MENU;
WiFiConnectionState currentWiFiState = WIFI_IDLE;

void _enterWirelessScreen(){
    inMenuMode = false;
    LOG_MENU_INFO("Entering Wireless Screen");

    if (WiFi.status() != WL_CONNECTED) {
        // WiFi 未连接时，启动配网
        LOG_MENU_INFO("Starting AP config mode from menu");
        enterConfigMode();
    }
    else{
        // WiFi 已连接时，显示连接信息
        lcdText("SSID:" + savedSSID ,1);
        lcdText("IP:" + WiFi.localIP().toString(),2);
    }
}

void _enterBrightnessScreen() {
    lcdText("Brightness:", 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
    lcdText(buf, 2);

    globalButtonDelay(FIRST_TIME_DELAY);  // 进入时防抖
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

void _resetWifi(){
    inMenuMode = false;
    SPIFFS.remove("/wifi.txt");
    lcdText("WiFi cleared", 1);
    lcdText("Rebooting...", 2);
    LOG_SYSTEM_INFO("WiFi config cleared, restarting...");
    delay(800);
    ESP.restart();
}

void _setupWebSetting(){
    web_setting_setupWebServer();
    LOG_WEB_INFO("Web configured");
}

void _connectInfo(){
    if (WiFi.status() == WL_CONNECTED) {
        lcdText("SSID:" + savedSSID, 1);
        lcdText("IP:" + WiFi.localIP().toString(), 2);
    } 
    else {
        lcdText("Not Connected", 1);
        lcdText("", 2);
    }
    
    globalButtonDelay(FIRST_TIME_DELAY);  // 防止立即退出

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
    globalButtonDelay(FIRST_TIME_DELAY);  // 防止立即退出
}

void _setWeatherInterface(){
    isNewInterface = true;
    currentState = STATE_WEATHER;
    globalButtonDelay(FIRST_TIME_DELAY);  // 防止立即退出
}

void _playBadAppleWrapper() {
    playBadAppleFromFileRaw("/badapple.bin");
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
    {"Brightness",      _enterBrightnessScreen, MENU_NONE},
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

// 初始化
void initMenu() {
    // 显示菜单界面
    _displayMenu(currentMenu, menuCursor, scrollOffset);
    
    // 启动菜单任务
    xTaskCreate(_menuTask, "_menuTask", 16384, NULL, 1, &_menuTaskHandle);
}

void _displayMenu(const Menu* menu, int menuIndex, int scrollOffset) {
    // LOG_MENU_VERBOSE("menu cursor at index " + String(menuIndex) + " scroll offset " + String(scrollOffset));
    lcdResetCursor();
    
    // 检查是否为主菜单，如果是则显示状态栏
    if (menu == &allMenus[MENU_MAIN]) {
        // 主菜单：状态栏作为第-1项（虚拟项），可滚动但不可选中
        for (int i = 0; i < VISIBLE_LINES; i++) {
            int displayIndex = scrollOffset + i;
            
            if (displayIndex == -1) {
                // 显示状态栏（虚拟项-1）
                String statusLine = "";
                
                // 根据WiFi连接状态显示不同内容
                switch (wifiConnectionState) {
                    case WIFI_CONNECTING:
                        statusLine = "W:connecting";
                        break;
                    case WIFI_CONNECTED:
                        statusLine = "W:OK ";
                        if(isTimeSyncInProgress){
                            statusLine += "T:Syncing...";
                        } else if (timeSyncState == TIME_SYNC_SUCCESS) {
                            statusLine += "T:OK";
                        } else {
                            statusLine += "T:--";
                        }
                        break;
                    case WIFI_FAILED:
                        statusLine = "W:can't connect";
                        break;
                    case WIFI_IDLE:
                    default:
                        statusLine = "offline mode";
                        break;
                }
                
                lcdText(statusLine, i + 1);  // 状态栏始终没有光标
            } else if (displayIndex >= 0 && displayIndex < menu->itemCount) {
                // 显示正常菜单项
                String itemName = menu->items[displayIndex].name;
                
                // 如果是Wireless Screen，根据 WiFi 状态动态修改名称
                if (displayIndex == 0) {
                    if (wifiConnectionState == WIFI_CONNECTED) {
                        itemName = "Wireless Screen";
                    } else {
                        itemName = "Config WiFi";
                    }
                }
                
                if (menuCursor == displayIndex)
                    lcdText(">" + itemName, i + 1);
                else
                    lcdText(" " + itemName, i + 1);
            } else {
                lcdText(" ", i + 1);  // 清空无效行
            }
        }
    } else {
        // 其他菜单保持原有显示方式
        for (int i = 0; i < VISIBLE_LINES; i++) {
            int menuItemIndex = scrollOffset + i;

            if (menuItemIndex >= menu->itemCount) {
                lcdText(" ", i + 1);  // 清空无效行
                continue;
            }

            if (menuCursor == menuItemIndex)
                lcdText(">" + (String)menu->items[menuItemIndex].name, i + 1);
            else
                lcdText(" " + (String)menu->items[menuItemIndex].name, i + 1);
        }
    }
}

// 根据菜单枚举获取对应菜单结构体指针
const Menu* _getMenuByState(MenuState state) {
    switch (state) {
        case MENU_MAIN:
            return &allMenus[MENU_MAIN];
        case MENU_SETTINGS:
            return &allMenus[MENU_SETTINGS];
        case MENU_WIFI_CONFIG:
            return &allMenus[MENU_WIFI_CONFIG];
        case MENU_ABOUT:
            return &allMenus[MENU_ABOUT];

        default:
            return NULL;
    }
}

void _handleMenuInterface() {
    // 显示菜单项
    if (isDisplayNeedsUpdate || _checkStateChanges()) {
        _displayMenu(currentMenu, menuCursor, scrollOffset);
        isDisplayNeedsUpdate = false;
    }

    static bool firstEntry = true;
    if (firstEntry) {
        globalButtonDelay(FIRST_TIME_DELAY);
        firstEntry = false;
    }

    int lastPressedButton = -1;
    for (int i = 0; i < 5; i++) {
        if (isButtonReadyToRespond(i, BUTTON_DEBOUNCE_DELAY)) { 
            lastPressedButton = i;
            break;  // 只处理一个键
        }
    }

    switch (lastPressedButton) {
        //光标上移
        case LEFT:
            if (currentMenu == &allMenus[MENU_MAIN]) {
                // 主菜单特殊处理：允许滚动到状态栏
                if (menuCursor > 0) {
                    // 光标最多滚动到0项
                    menuCursor--;
                    isDisplayNeedsUpdate = true;
                } else if (scrollOffset > -1) {
                    // 如果光标在第0项且还能向上滚动，则滚动显示状态栏
                    scrollOffset--;
                    isDisplayNeedsUpdate = true;
                }
            } else {
                // 其他菜单
                if (menuCursor > 0) {
                    isDisplayNeedsUpdate = true;
                    menuCursor = constrain(menuCursor - 1, 0, currentMenu->itemCount - 1);
                }
            }
            break;

        case RIGHT:
            if (currentMenu == &allMenus[MENU_MAIN]) {
                // 主菜单特殊处理
                if (scrollOffset == -1 && menuCursor == 0) {
                    // 如果当前显示状态栏且光标在第0项，向下滚动隐藏状态栏
                    isDisplayNeedsUpdate = true;
                    scrollOffset = 0;
                } else {
                    isDisplayNeedsUpdate = true;
                    menuCursor = constrain(menuCursor + 1, 0, currentMenu->itemCount - 1);
                }
            } else {
                // 滚动光标同时防止越界
                isDisplayNeedsUpdate = true;
                menuCursor = constrain(menuCursor + 1, 0, currentMenu->itemCount - 1);
            }
            break;

        case CENTER:
            isDisplayNeedsUpdate = true;

            // 菜单选项
            firstEntry = true;     // 重置防误触
            if (currentMenu->items[menuCursor].nextState != MENU_NONE) {
                const Menu* nextMenu = _getMenuByState(currentMenu->items[menuCursor].nextState);
                if (nextMenu) {
                    currentMenu = nextMenu;
                    menuCursor = 0; // 重置光标
                    
                    // 如果进入主菜单，初始化scrollOffset为-1以显示状态栏
                    if (currentMenu == &allMenus[MENU_MAIN]) {
                        scrollOffset = -1;
                    } else {
                        scrollOffset = 0;
                    }
                }
            }
            // 动作选项
            else if (currentMenu->items[menuCursor].action) {
                currentMenu->items[menuCursor].action();  // 触发动作
            } 
            break;
    }

    // 更新 scrollOffset 以保持光标在可视区
    if (scrollOffset >= 0) {
        if (menuCursor < scrollOffset) {
            // 可视范围跟随光标上移
            scrollOffset = menuCursor;
        } 
        else if (menuCursor >= scrollOffset + VISIBLE_LINES) {
            // 可视范围跟随光标下移
            scrollOffset = menuCursor - VISIBLE_LINES + 1;
        }
    }
    
    // 确保scrollOffset在合理范围内
    if (scrollOffset < -1) {
        LOG_MENU_WARN("Adjusting scrollOffset from " + String(scrollOffset) + " to -1");
        scrollOffset = -1;
    }
    
    int maxScrollOffset = currentMenu->itemCount - VISIBLE_LINES;
    if (maxScrollOffset < -1) {
        LOG_MENU_WARN("Adjusting maxScrollOffset from " + String(maxScrollOffset) + " to -1");
        maxScrollOffset = -1;
    }
    if (scrollOffset > maxScrollOffset) {
        LOG_MENU_WARN("Adjusting scrollOffset from " + String(scrollOffset) + " to " + String(maxScrollOffset));
        scrollOffset = maxScrollOffset;
    }
}

bool _ensureisTimeSynced() {
    if (timeSyncState != TIME_SYNC_SUCCESS) {
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
    
    return false;
}

// 菜单任务
TaskHandle_t _menuTaskHandle = NULL;
void _menuTask(void* parameter) {
    static unsigned long lastDisplayUpdate = 0;
    static bool hasTriedTimeSync = false;  // 标记是否已经尝试过时间同步

    while (true) {
        // 一次性后台时间同步检查
        if (!hasTriedTimeSync && WiFi.status() == WL_CONNECTED && timeSyncState != TIME_SYNC_SUCCESS) {
            hasTriedTimeSync = true;
            updateTimeSync();
        }
        
        if (inMenuMode) {
            switch (currentState) {
                case STATE_MENU:
                    _handleMenuInterface();
                    break;

                case STATE_CLOCK:
                    if(!_ensureisTimeSynced())
                        break;

                    // 每隔1秒刷新一次时间显示
                    if (millis() - lastDisplayUpdate > 1000 || isNewInterface) {
                        isNewInterface = false;
                        updateClockScreen();
                        lastDisplayUpdate = millis();
                    }

                    // 始终检查是否需要退出
                    if(isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)){
                        LOG_MENU_INFO("exit to main menu");
                        currentState = STATE_MENU;
                        resetButtonDebounce();  // 重置防抖
                        break;
                    }

                    break;

                case STATE_WEATHER: 
                    if(!_ensureisTimeSynced()){
                        currentState = STATE_MENU;
                        break;
                    }

                    // 每隔10分钟更新一次天气数据
                    if (!isReadyToDisplay || millis() - lastWeatherUpdate > 10*60*1000) {
                        if (millis() - lastWeatherFail > 15*1000) { // 失败后15秒再试
                            init_jwt();
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
                        resetButtonDebounce();  // 重置防抖
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
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 稳定刷新
    }
}