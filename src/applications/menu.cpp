#include "menu.h"

// 函数前置声明
void displayMenu(const Menu* menu, int menuIndex, int scrollOffset);

// 菜单状态变量
volatile bool isNewInterface = false;
volatile bool inMenuMode = true;
volatile bool isReadyToDisplay = false;
static bool displayNeedsUpdate = true;
long lastWeatherFail = -15000;
InterfaceState currentState = STATE_MENU;

void enterWirelessScreen(){
    inMenuMode = false;
    if (WiFi.status() != WL_CONNECTED) {
        // WiFi 未连接时，启动 AP 配网模式
        LOG_MENU_INFO("Starting AP config mode from menu");
        enterConfigMode();
    }
    else{
        // WiFi 已连接时，显示连接信息
        lcd_text("SSID:" + savedSSID ,1);
        lcd_text("IP:" + WiFi.localIP().toString(),2);
    }
}

void enterBrightnessScreen() {
    lcd_text("Brightness:", 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
    lcd_text(buf, 2);

    globalButtonDelay(FIRST_TIME_DELAY);  // 进入时防抖
    uint8_t lastBrightness = brightness;

    while (!isButtonReadyToRespond(CENTER)) {
        if (isButtonReadyToRespond(LEFT, 100)) {
            changeBrightness(-1);
        }
        if (isButtonReadyToRespond(RIGHT, 100)) {
            changeBrightness(1);
        }

        // 只有亮度变化才重绘文字
        if (brightness != lastBrightness) {
            snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
            lcd_text("Brightness:", 1);
            lcd_text(buf, 2);
            lastBrightness = brightness;
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 小延迟防止CPU占用过高
    }

    lcd_text("saved", 1);
    lcd_text("Back to Menu", 2);
    LOG_SYSTEM_INFO("Brightness set to %d%%", (brightness * 100 + 127) / 255);
    delay(500);
}

void resetWifi(){
    inMenuMode = false;
    lcd_text("Clearing WiFi", 1);
    lcd_text("Rebooting...", 2);
    SPIFFS.remove("/wifi.txt");
    LOG_SYSTEM_INFO("WiFi config cleared, restarting...");
    delay(1000);
    ESP.restart();
}

void setupWebSetting(){
    if(!timeSynced){
        setupTime();
    }

    web_setting_setupWebServer();
    LOG_WEB_INFO("Web seted");
}

void connectInfo(){
    LOG_SYSTEM_DEBUG("call connect Info");
    if (WiFi.status() == WL_CONNECTED) {
        lcd_text("SSID:" + savedSSID, 1);
        lcd_text("IP:" + WiFi.localIP().toString(), 2);
    } 
    else {
        lcd_text("Not Connected", 1);
        lcd_text("", 2);
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

void setClockInterface(){
    isNewInterface = true;
    currentState = STATE_CLOCK;
    globalButtonDelay(FIRST_TIME_DELAY);  // 防止立即退出
}

void setWeatherInterface(){
    isNewInterface = true;
    currentState = STATE_WEATHER;
    globalButtonDelay(FIRST_TIME_DELAY);  // 防止立即退出
}

void playBadAppleWrapper() {
    playBadAppleFromFileRaw("/badapple.bin");
}

const MenuItem mainMenuItems[] = {
    {"Wireless Screen",     enterWirelessScreen, MENU_NONE},       // 无线屏幕
    {"Clock",               setClockInterface, MENU_NONE},       // 时钟
    {"Weather",             setWeatherInterface, MENU_NONE},       // 天气
    {"Settings",            NULL, MENU_SETTINGS},   // 设置
    {"About",               NULL, MENU_ABOUT},      // 关于
    {"Bad Apple",           playBadAppleWrapper, MENU_NONE}        // Bad Apple
};

const MenuItem settingsMenuItems[] = {
    {"Web setting",     setupWebSetting, MENU_NONE},
    {"WiFi Config",     NULL, MENU_WIFI_CONFIG},
    {"Brightness",      enterBrightnessScreen, MENU_NONE},
    {"Return",          NULL, MENU_MAIN}
};

const MenuItem wifiConfigMenuItems[] = {
    {"connect Info",    connectInfo, MENU_NONE},
    {"Reset Wifi",      resetWifi, MENU_NONE},
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

const Menu* currentMenu = &allMenus[MENU_MAIN]; // 初始为主菜单
int menuCursor = 0;   // 当前菜单项光标位置
int scrollOffset = -1;   // 当前显示窗口起始项，主菜单初始为-1以显示状态栏
const int visibleLines = 2;  // LCD 屏行数

// 初始化函数（可扩展）
void initMenu() {
    // 立即显示菜单界面，不等待WiFi或时间同步
    displayMenu(currentMenu, menuCursor, scrollOffset);
    
    // 启动菜单任务
    xTaskCreate(menuTask, "MenuTask", 16384, NULL, 1, &menuTaskHandle);
}


void displayMenu(const Menu* menu, int menuIndex, int scrollOffset) {
    LOG_MENU_VERBOSE("menu cursor at index " + String(menuIndex) + " scroll offset " + String(scrollOffset));
    lcdResetCursor();
    
    // 检查是否为主菜单，如果是则包含状态栏参与滚动
    if (menu == &allMenus[MENU_MAIN]) {
        // 主菜单：状态栏作为第-1项（虚拟项），可滚动但不可选中
        for (int i = 0; i < visibleLines; i++) {
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
                        if(timeSyncInProgress){
                            statusLine += "T:Syncing...";
                        } else if (timeSynced) {
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
                
                lcd_text(statusLine, i + 1);  // 状态栏始终没有光标
            } else if (displayIndex >= 0 && displayIndex < menu->itemCount) {
                // 显示正常菜单项
                String itemName = menu->items[displayIndex].name;
                
                // 如果是第一个菜单项（Wireless Screen），根据 WiFi 状态动态修改名称
                if (displayIndex == 0) {
                    if (wifiConnectionState == WIFI_CONNECTED || wifiConnectionState == WIFI_CONNECTING) {
                        itemName = "Wireless Screen";
                    } else {
                        itemName = "Config WiFi";
                    }
                }
                
                if (menuCursor == displayIndex)
                    lcd_text(">" + itemName, i + 1);
                else
                    lcd_text(" " + itemName, i + 1);
            } else {
                lcd_text(" ", i + 1);  // 清空无效行
            }
        }
    } else {
        // 其他菜单保持原有显示方式
        for (int i = 0; i < visibleLines; i++) {
            int menuItemIndex = scrollOffset + i;

            if (menuItemIndex >= menu->itemCount) {
                lcd_text(" ", i + 1);  // 清空无效行
                continue;
            }

            if (menuCursor == menuItemIndex)
                lcd_text(">" + (String)menu->items[menuItemIndex].name, i + 1);
            else
                lcd_text(" " + (String)menu->items[menuItemIndex].name, i + 1);
        }
    }
}

// 根据菜单枚举获取对应菜单结构体指针
const Menu* getMenuByState(MenuState state) {
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

void handleMenuInterface() {
    // 显示菜单项
    if (displayNeedsUpdate) {
        displayMenu(currentMenu, menuCursor, scrollOffset);
        displayNeedsUpdate = false;
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
        case LEFT:
        displayNeedsUpdate = true;
            if (currentMenu == &allMenus[MENU_MAIN]) {
                // 主菜单特殊处理：允许滚动到状态栏
                if (menuCursor > 0) {
                    menuCursor--;
                } else if (scrollOffset > -1) {
                    // 如果光标在第0项且还能向上滚动，则滚动显示状态栏
                    scrollOffset--;
                }
            } else {
                // 其他菜单正常处理
                if (menuCursor > 0) menuCursor--;
            }
            break;

        case RIGHT:
            displayNeedsUpdate = true;
            if (currentMenu == &allMenus[MENU_MAIN]) {
                // 主菜单特殊处理
                if (scrollOffset == -1 && menuCursor == 0) {
                    // 如果当前显示状态栏且光标在第0项，向下滚动隐藏状态栏
                    scrollOffset = 0;
                } else if (menuCursor < currentMenu->itemCount - 1) {
                    menuCursor++;
                }
            } else {
                // 其他菜单正常处理
                menuCursor = constrain(menuCursor + 1, 0, currentMenu->itemCount - 1);
            }
            break;

        case CENTER:
            displayNeedsUpdate = true;

            // 菜单选项
            firstEntry = true;     // 重置防误触
            if (currentMenu->items[menuCursor].nextState != MENU_NONE) {
                const Menu* nextMenu = getMenuByState(currentMenu->items[menuCursor].nextState);
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
    if (currentMenu == &allMenus[MENU_MAIN]) {
        // 主菜单特殊处理：状态栏滚动已在按键处理中控制
        // 这里只处理正常的菜单项滚动逻辑
        if (scrollOffset >= 0) {
            // 正常菜单项的滚动逻辑
            if (menuCursor < scrollOffset) {
                scrollOffset = menuCursor;
            } 
            else if (menuCursor >= scrollOffset + visibleLines) {
                scrollOffset = menuCursor - visibleLines + 1;
            }
        }
        
        // 确保scrollOffset在合理范围内
        if (scrollOffset < -1) {
            LOG_MENU_WARN("Adjusting scrollOffset from " + String(scrollOffset) + " to -1");
            scrollOffset = -1;
        }
        
        int maxScrollOffset = currentMenu->itemCount - visibleLines;
        if (maxScrollOffset < -1) {
            LOG_MENU_WARN("Adjusting maxScrollOffset from " + String(maxScrollOffset) + " to -1");
            maxScrollOffset = -1;
        }
        if (scrollOffset > maxScrollOffset) {
            LOG_MENU_WARN("Adjusting scrollOffset from " + String(scrollOffset) + " to " + String(maxScrollOffset));
            scrollOffset = maxScrollOffset;
        }
    } else {
        // 当光标在可视区域上方时，向上滚动显示窗口
        if (menuCursor < scrollOffset) {
            scrollOffset = menuCursor;
        } 
        // 当光标在可视区域下方时，向下滚动显示窗口
        else if (menuCursor >= scrollOffset + visibleLines) {
            scrollOffset = menuCursor - visibleLines + 1;
        }
    }

    delay(100);  // 节流
}

bool ensureTimeSynced() {
    if (!timeSynced) {
        setupTime();
        if (!timeSynced) {
            LOG_MENU_WARN("Time not synced yet, cannot display");
            delay(1000);
            return false;
        }
    }
    return true;
}

TaskHandle_t menuTaskHandle = NULL;
// 菜单任务函数
void menuTask(void* parameter) {
    static unsigned long lastDisplayUpdate = 0;
    static bool hasTriedTimeSync = false;  // 标记是否已经尝试过时间同步

    while (true) {
        // 一次性后台时间同步检查
        if (!hasTriedTimeSync && WiFi.status() == WL_CONNECTED && !timeSynced) {
            hasTriedTimeSync = true;
            initTimeSync();
        }
        
        if (inMenuMode) {
            switch (currentState) {
                case STATE_MENU:
                    handleMenuInterface();
                    break;

                case STATE_CLOCK:
                    if(!ensureTimeSynced())
                        break;

                    // 每隔1秒刷新一次时间显示（非阻塞）
                    if (millis() - lastDisplayUpdate > 1000 || isNewInterface) {
                        isNewInterface = false;
                        updateClockScreen();
                        lastDisplayUpdate = millis();
                    }

                    // 始终检查是否需要退出
                    if(isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)){
                        LOG_MENU_INFO("exit to main menu");
                        currentState = STATE_MENU;
                        resetButtonDebounce();  // 重置防抖，防止立即再次响应
                        break;
                    }

                    break;

                case STATE_WEATHER: 
                    if(!ensureTimeSynced()){
                        currentState = STATE_MENU;
                        break;
                    }

                    // 每隔10分钟更新一次天气数据（非阻塞）
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
                        resetButtonDebounce();  // 重置防抖，防止立即再次响应
                        break;
                    }
                    if(isReadyToDisplay == false){
                        lcd_text("No Data", 1);
                        lcd_text(" ", 2);
                    }
                    else{
                        if(isButtonReadyToRespond(LEFT, BUTTON_DEBOUNCE_DELAY)){
                            lastDisplayUpdate = millis();
                            interface_num = (interface_num + 2) % 3; // 切换到上一个界面
                            updateWeatherScreen();
                        }
                        if(isButtonReadyToRespond(RIGHT, BUTTON_DEBOUNCE_DELAY)){
                            lastDisplayUpdate = millis();
                            interface_num = (interface_num + 1) % 3; // 切换到下一个界面
                            updateWeatherScreen();
                        }
                    }
                    break;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 稳定刷新
    }
}