#include "menu.h"

// 菜单状态变量
volatile bool inMenuMode = false;

// 初始化函数（可扩展）
void initMenu() {
    // 可放置菜单初始化资源
}

// 判断是否经过了指定的时间间隔
bool hasElapsed(unsigned long startTime, unsigned long intervalMs) {
    return (millis() - startTime) >= intervalMs;
}

void handleMenuInterface() {
    // 简单菜单项列表
    static int menuIndex = 0;

    const char* menuItems[] = {
    "1.Brightness",
    "2.Status Info",
    "3.Reset WiFi",
    "4.About",
    "5.Exit Menu"
    };
    const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);

    static unsigned long menuEnterTime = 0;
    static bool firstEntry = true;

    unsigned long now = millis();

    if (firstEntry) {
        menuEnterTime = now;
        firstEntry = false;
    }

    lcd_text("Menu:", 1);
    lcd_text(menuItems[menuIndex], 2);

    // 等待300ms后才允许按键生效，防止误触
    if (now - menuEnterTime < 300) {
        // 只显示菜单，不处理按键
        return;
    }

    // while(hasElapsed(menuEnterTime, 400)){}


    // 显示当前菜单项
    lcd_text("Menu:", 1);
    lcd_text(menuItems[menuIndex], 2);

    // 菜单控制：按键切换
    if (buttonJustPressed[LEFT]) {
        menuIndex = (menuIndex + menuCount - 1) % menuCount;
    }
    if (buttonJustPressed[RIGHT]) {
        menuIndex = (menuIndex + 1) % menuCount;
    }

    // 按下确认键执行操作
    if (buttonJustPressed[CENTER]) {
        switch (menuIndex) {
            case 0: {  // 调节亮度
                lcd_text("Brightness:", 1);
                lcd_text(String(brightness), 2);
                now= millis();
                while(!hasElapsed(now, 200)){}

                static unsigned long lastAdjustTime = 0;

                while (!buttonJustPressed[CENTER])
                {
                    now = millis();
                    if (hasElapsed(lastAdjustTime, 5)) {  // 防抖/调节间隔
                        if (buttonJustPressed[LEFT]) {
                            changeBrightness(-1);
                            lastAdjustTime = now;
                        }
                        if (buttonJustPressed[RIGHT]) {
                            changeBrightness(1);
                            lastAdjustTime = now;
                        }
                        lcd_text("Brightness:", 1);
                        lcd_text(String(brightness), 2);
                    }
                }
                lcd_text("Back to Menu", 1);
                delay(500);
                break;
            }


            case 1:  // 查看信息
                Serial.println("Status Info");
                if (WiFi.status() == WL_CONNECTED) {
                    lcd_text("SSID:" + savedSSID, 1);
                    lcd_text("IP:" + WiFi.localIP().toString(), 2);
                } else {
                    lcd_text("Not Connected", 1);
                    lcd_text("", 2);
                }
                delay(2000);
                break;

            case 2:  // 重置网络
                lcd_text("Clearing WiFi", 1);
                lcd_text("Rebooting...", 2);
                FFat.remove("/wifi.txt");
                delay(1000);
                ESP.restart();
                break;

            case 3:  // 关于本机
                lcd_text("Code by Kulib", 1);
                lcd_text("2025/6/16", 2);  // 或显示版本号等
                delay(2000);
                break;

            case 4:  // 退出菜单
                Serial.println("Exit Menu");
                lcd_text("Exit Menu", 1);
                lcd_text("Restoring...", 2);
                delay(1000);
                if(inConfigMode){
                    // 在屏幕上显示ip
                    lcd_text("Connect to AP",1);
                    lcd_text("IP:" + WiFi.softAPIP().toString(),2);
                }
                else{
                    // 在屏幕上显示ip
                    lcd_text("SSID:" + savedSSID ,1);
                    lcd_text("IP:" + WiFi.localIP().toString(),2);
                }
                inMenuMode = false;
                firstEntry = true;  // 退出菜单，重置首次进入标志
                break;
        }
    }

    delay(120);  // 节流
}