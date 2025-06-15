#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "lcd_driver.h"
#include "mydefine.h"
#include "rgb_led.h"
#include "myhader.h"
#include "network.h"

// 外部变量定义（供主函数或其他模块调用）
extern WebServer AP_server;
extern String ssid_input, password_input;
extern String savedSSID, savedPassword;
extern bool inConfigMode;

// 配置函数
void enterConfigMode();      // 启动配网页面（配网模式）
void connectToWiFi();        // 尝试连接 WiFi，失败进入配网模式

// 存储与加载 WiFi 信息
void saveWiFiCredentials(const String& ssid, const String& password);
void loadWiFiCredentials();

// 内部网页处理函数（一般无需外部调用）
void handleRoot();
void handleSet();

void wifiinit();

#endif