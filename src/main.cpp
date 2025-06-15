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
using namespace std;

void init(){                        //初始化引脚
    setOutput(LCD_RS);
    setOutput(LCD_E);
    setOutput(LCD_D4);
    setOutput(LCD_D5);
    setOutput(LCD_D6);
    setOutput(LCD_D7);
    setOutput(LCD_BLA);

    setInput(BOTTEN_UP);
    setInput(BOTTEN_DOWN);
    setInput(BOTTEN_LEFT);
    setInput(BOTTEN_RIGHT);
    setInput(BOTTEN_CENTER);
}

void setup() {
    Serial.begin(BaudRate);
    setHigh(LCD_BLA);
    while(!Serial){}
    delay(800);
    Serial.println("Wireless 1602A V1.0 by Kulib");
    Serial.println("2025/6/15");
    Serial.println("开始初始化...");

    init();
    initrgb();
    initKanaMap();  // 初始化假名表
    lcd_init();
    startButtonTask();

    // 挂载FFAT
    if (!FFat.begin()) {
        Serial.println("无法挂载FFat, 正在格式化...");
        if (FFat.format()) {
            Serial.println("FFat已格式化!");
        }
        if (!FFat.begin()) {
            lcd_text("NO FFat", LCD_line1);
            lcd_text("Check Serial", LCD_line2);
            Serial.println("仍然无法挂载FFat, 请检查Flash分区设置");
            updateColor(CRGB::Red);  // 失败变红
            int fadeStep = 2;
            uint8_t brightness = 64;
            while (1){
                brightness += fadeStep;

                if (brightness == 0 || brightness == 192) {
                    fadeStep = -fadeStep;
                }
                updateBrightness(brightness);
                delay(5);
            };
        }
    }
    else{
        Serial.println("FFat已挂载");
    }

    // 欢迎消息
    lcd_text("Wireless 1602A",LCD_line1);
    lcd_text("V1.0",LCD_line2);
    delay(1000);

    wifiinit();
}

void loop(){
    if (inConfigMode) {
        AP_server.handleClient();
    }

    acceptClientIfNew();
    receiveClientData();
    tryDisplayCachedFrames();
}