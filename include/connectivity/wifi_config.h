/**
 * @file wifi_config.h
 * @brief WiFi 配置头文件，提供 WiFi 连接和配网功能
 *
 * 此文件声明 WiFi 配网、连接和配置的函数和变量
 *
 * @author kulib
 * @date 2025-11-05
 */
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "lcd_driver.h"
#include "mydefine.h"
#include "rgb_led.h"
#include "myheader.h"
#include "network.h"
#include "web_pages.h"
#include "wifi_config_manager.h"

extern WebServer apServer;                     /**< 配网模式使用的 Web 服务器 */
extern DNSServer dnsServer;                     /**< DNS 服务器用于强制门户 */

enum WifiScanState {
    WIFI_SCAN_IDLE,
    WIFI_SCAN_SCANNING,
    WIFI_SCAN_DONE
};

/**
 * @brief WiFi 连接状态枚举
 */
enum WiFiConnectionState {
    WIFI_IDLE,         /**< 初始状态，未开始连接 */
    WIFI_CONNECTING,   /**< 正在连接中 */
    WIFI_CONNECTED,    /**< 已连接 */
    WIFI_DISCONNECTED, /**< 连接断开 */
    WIFI_FAILED        /**< 连接失败 */
};

extern WiFiConnectionState wifiConnectionState;     /**< 当前 WiFi 连接状态 */
extern bool inConfigMode;                           /**< 配网模式状态标志 */

/**
 * @brief 进入配网模式
 * @details 启动软 AP 和 Web 服务器，提供配网页面和 DNS 重定向
 */
void enterConfigMode();

/**
 * @brief 连接到保存的 WiFi 网络
 * @details 加载保存的 WiFi 信息，尝试连接，失败则进入配网模式
 */
void connectToWiFi();

/**
 * @brief 初始化 WiFi 连接流程
 * @details 检查按键，决定进入配网或连接模式，并启动服务器
 */
void wifiinit();

#endif