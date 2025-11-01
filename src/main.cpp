#include "myhader.h"
#include "mydefine.h"
#include "kanamap.h"
#include "lcd_driver.h"
#include "wifi_config.h"
#include "rgb_led.h"
#include "network.h"
#include "protocol.h"
#include "playbuffer.h"
#include "button.h"
#include "menu.h"
#include "clock.h"
#include "jwt_auth.h"
using namespace std;

void init(){                        //初始化引脚
    setOutput(LCD_RS);
    setOutput(LCD_E);
    setOutput(LCD_D4);
    setOutput(LCD_D5);
    setOutput(LCD_D6);
    setOutput(LCD_D7);
    setOutput(LCD_BLA);

    setInput(BUTTEN_UP);
    setInput(BUTTEN_DOWN);
    setInput(BUTTEN_LEFT);
    setInput(BUTTEN_RIGHT);
    setInput(BUTTEN_CENTER);
}

void listDir(const char* dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = SPIFFS.open(dirname);
    if(!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file) {
        if(file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels) {
                listDir(file.name(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void setup() {
    init();
    initrgb();
    initKanaMap();  // 初始化假名表
    lcd_init();
    startButtonTask();

    // 欢迎消息
    lcd_text("Wireless 1602A",1);
    lcd_text("2025/11/01",2);

    Serial.begin(BaudRate);
    while(!Serial){}
    
    Serial.println("Wireless 1602A by Kulib");
    Serial.println("2025/11/01");
    Serial.println("开始初始化...");

    // 挂载 SPIFFS
    if (!SPIFFS.begin(true)) {      // 失败时自动格式化
        Serial.println("无法挂载SPIFFS, 正在格式化...");
        if (SPIFFS.format()) {
            Serial.println("SPIFFS已格式化!");
        }
        if (!SPIFFS.begin(true)) {
            lcd_text("NO SPIFFS", 1);
            lcd_text("Check Serial", 2);
            Serial.println("仍然无法挂载SPIFFS, 请检查Flash分区设置");
            updateColor(CRGB::Red);  // 失败变红
            int fadeStep = 2;
            uint8_t brightness = 64;
            while (1) {
                brightness += fadeStep;
                if (brightness == 0 || brightness == 192) {
                    fadeStep = -fadeStep;
                }
                updateBrightness(brightness);
                delay(5);
            };
        }
    } else {
        Serial.println("SPIFFS已挂载");
        listDir("/", 0);    // 打印根目录文件
    }

    delay(500);
    init_jwt();  // 初始化JWT
    wifiinit();  // 初始化WiFi配置
    initMenu();  // 初始化菜单系统
}

void loop(){
    if (inConfigMode) {
        AP_server.handleClient();
    }

    acceptClientIfNew();
    receiveClientData();
    tryDisplayCachedFrames();
}