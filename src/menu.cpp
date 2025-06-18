#include "menu.h"

// 菜单状态变量
volatile bool inMenuMode = true;
InterfaceState currentState = STATE_MENU;

void enterWirelessScreen(){
    inMenuMode = false;
    if (WiFi.status() != WL_CONNECTED) {
        lcd_text("Connect to AP",1);
        lcd_text("IP:" + WiFi.softAPIP().toString(),2);
    }
    else{
        lcd_text("SSID:" + savedSSID ,1);
        lcd_text("IP:" + WiFi.localIP().toString(),2);
    }
}

void enterBrightnessScreen() {
    lcd_text("Brightness:", 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
    lcd_text(buf, 2);

    u32_t now = millis();
    while (!hasElapsed(now, 200)) {}

    static unsigned long lastAdjustTime = 0;
    uint8_t lastBrightness = brightness;

    while (!buttonJustPressed[CENTER]) {
        now = millis();
        if (hasElapsed(lastAdjustTime, 5)) {
            if (buttonJustPressed[LEFT]) {
                changeBrightness(-1);
                lastAdjustTime = now;
            }
            if (buttonJustPressed[RIGHT]) {
                changeBrightness(1);
                lastAdjustTime = now;
            }

            // 只有亮度变化才重绘文字
            if (brightness != lastBrightness) {
                snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
                lcd_text("Brightness:", 1);
                lcd_text(buf, 2);
                lastBrightness = brightness;
            }
        }
    }

    lcd_text("Back to Menu", 1);
    delay(500);
}

void resetWifi(){
    inMenuMode = false;
    lcd_text("Clearing WiFi", 1);
    lcd_text("Rebooting...", 2);
    SPIFFS.remove("/wifi.txt");
    delay(1000);
    ESP.restart();
}

void connectInfo(){
    Serial.println("Status Info");
    if (WiFi.status() == WL_CONNECTED) {
        lcd_text("SSID:" + savedSSID, 1);
        lcd_text("IP:" + WiFi.localIP().toString(), 2);
    } else {
        lcd_text("Not Connected", 1);
        lcd_text("", 2);
    }
    delay(2000);
}

void setClockInterface(){
    currentState = STATE_CLOCK;
}

void playBadAppleWrapper() {
    playBadAppleFromFileRaw("/badapple.bin");
}

const MenuItem mainMenuItems[] = {
    {"Wireless Screen",     enterWirelessScreen, MENU_NONE},       // 无线屏幕
    {"Clock",               setClockInterface, MENU_NONE},       // 时钟
    {"Weather",             NULL, MENU_NONE},       // 天气
    {"Settings",            NULL, MENU_SETTINGS},   // 设置
    {"About",               NULL, MENU_ABOUT},      // 关于
    {"Bad Apple",           playBadAppleWrapper, MENU_NONE}        // Bad Apple
};

const MenuItem settingsMenuItems[] = {
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
    {"About me",        NULL, MENU_NONE},
    {"About Project",   NULL, MENU_NONE},
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
int scrollOffset = 0;   // 当前显示窗口起始项
const int visibleLines = 2;  // LCD 屏行数

// 初始化函数（可扩展）
void initMenu() {
    xTaskCreate(menuTask, "MenuTask", 8192, NULL, 1, &menuTaskHandle);
}

// 判断是否经过了指定的时间间隔
bool hasElapsed(unsigned long startTime, unsigned long intervalMs) {
    return (millis() - startTime) >= intervalMs;
}


void displayMenu(const Menu* menu, int menuIndex, int scrollOffset) {
    // Serial.print((String)menuIndex + " ");
    // Serial.print((String)scrollOffset + "\n");
    lcdResetCursor();
    for (int i = 0; i < visibleLines; i++) {
        int menuIndex = scrollOffset + i;

        if (menuIndex >= menu->itemCount) {
            lcd_text(" ", i + 1);  // 清空无效行
            continue;
        }

        if (menuCursor == menuIndex)
            lcd_text(">" + (String)menu->items[menuIndex].name, i + 1);
        else
            lcd_text(" " + (String)menu->items[menuIndex].name, i + 1);
    }
}

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
        // 其他菜单...
        default:
            return NULL;
    }
}

void handleMenuInterface() {
    // const Menu* menu = &allMenus[currentMenu];
    
    // 显示菜单项（基于 menuCursor）
    displayMenu(currentMenu, menuCursor, scrollOffset);

    static unsigned long menuEnterTime = 0;
    static bool firstEntry = true;
    if (firstEntry) {
        menuEnterTime = millis();
        firstEntry = false;
    }

    // 等待200ms后才允许按键生效，防止误触
    // if (millis() - menuEnterTime < 200) {
    //     // 只显示菜单，不处理按键
    //     return;
    // }

    while (!hasElapsed(menuEnterTime, 200)){}

    int lastPressedButton = -1;
    for (int i = 0; i < 5; i++) {
        if (buttonJustPressed[i]) {
            lastPressedButton = i;
            break;  // 只处理一个键
        }
    }

    switch (lastPressedButton) {
        case LEFT:
            if (menuCursor > 0) menuCursor--;
            break;
        case RIGHT:
            if (menuCursor < currentMenu->itemCount - 1) menuCursor++;
            break;
        case CENTER:
        // 如果有下一菜单，跳转
            firstEntry = true;     // 重置防误触
            if (currentMenu->items[menuCursor].nextState != MENU_NONE) {
                const Menu* nextMenu = getMenuByState(currentMenu->items[menuCursor].nextState);
                if (nextMenu) {
                    currentMenu = nextMenu;
                    menuCursor = 0; // 重置光标
                }
            }
            else if (currentMenu->items[menuCursor].action) {
                currentMenu->items[menuCursor].action();  // 触发动作
            } 
            break;
    }

    // 更新 scrollOffset 以保持光标在可视区
    if (menuCursor < scrollOffset) {
        scrollOffset = menuCursor;
    } 
    else if (menuCursor >= scrollOffset + visibleLines) {
        scrollOffset = menuCursor - visibleLines + 1;
    }

    delay(120);  // 节流
}

TaskHandle_t menuTaskHandle = NULL;
// 菜单任务函数
void menuTask(void* parameter) {
    static unsigned long lastClockUpdate = 0;

    while (true) {
        if (inMenuMode) {
            switch (currentState) {
                case STATE_MENU:
                    handleMenuInterface();
                    break;
                case STATE_CLOCK:
                    if(!timeSynced){setupTime();}
                    // 始终检查是否需要退出
                    if(buttonJustPressed[CENTER] && buttonJustPressed[UP]){
                        currentState = STATE_MENU;
                        break;;
                    }

                    // 每隔1秒刷新一次时间显示（非阻塞）
                    if (millis() - lastClockUpdate > 1000) {
                        updateClockScreen();
                        lastClockUpdate = millis();
                    }
                    
                    break;
                // 更多界面...
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 稳定刷新
    }
}