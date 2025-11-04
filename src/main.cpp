#include "myheader.h"
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
#include "utils/logger.h"
#include <DNSServer.h>
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
    LOG_SYSTEM_DEBUG("Listing directory: %s", dirname);

    File root = SPIFFS.open(dirname);
    if(!root) {
        LOG_SYSTEM_ERROR("Failed to open directory: %s", dirname);
        return;
    }
    if(!root.isDirectory()) {
        LOG_SYSTEM_ERROR("Not a directory: %s", dirname);
        return;
    }

    File file = root.openNextFile();
    while(file) {
        if(file.isDirectory()) {
            LOG_SYSTEM_DEBUG("  DIR : %s", file.name());
            if(levels) {
                listDir(file.name(), levels - 1);
            }
        } else {
            LOG_SYSTEM_DEBUG("  FILE: %s\tSIZE: %d", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}

void setup() {
    // 首先初始化日志系统
    Logger::init(LOG_LEVEL_DEBUG);  // 可以通过此处调整全局日志级别
    
    init();
    initrgb();
    initKanaMap();  // 初始化假名表
    lcd_init();
    startButtonTask();

    // 欢迎消息
    lcd_text("Wireless 1602A",1);
    lcd_text("2025/11/02",2);

    LOG_SYSTEM_INFO("Wireless 1602A by Kulib");
    LOG_SYSTEM_INFO("2025/11/01");
    LOG_SYSTEM_INFO("开始初始化...");

    // 挂载 SPIFFS
    if (!SPIFFS.begin(true)) {      // 失败时自动格式化
        LOG_SYSTEM_WARN("无法挂载SPIFFS, 正在格式化...");
        if (SPIFFS.format()) {
            LOG_SYSTEM_INFO("SPIFFS已格式化!");
        }
        if (!SPIFFS.begin(true)) {
            lcd_text("NO SPIFFS", 1);
            lcd_text("Check Serial", 2);
            LOG_SYSTEM_ERROR("仍然无法挂载SPIFFS, 请检查Flash分区设置");
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
        LOG_SYSTEM_INFO("SPIFFS已挂载");
        listDir("/", 0);    // 打印根目录文件
    }

    delay(300);  // 缩短延迟，快速进入主界面
    init_jwt();  // 初始化JWT
    wifiinit();  // 初始化WiFi配置（非阻塞，后台连接）
    initMenu();  // 初始化菜单系统（立即进入主界面）
}

void loop(){
    if (inConfigMode) {
        dnsServer.processNextRequest();  // 处理劫持DNS请求
        AP_server.handleClient();
    }

    // 更新非阻塞时间同步
    updateTimeSync();
    
    acceptClientIfNew();
    receiveClientData();
    tryDisplayCachedFrames();
}