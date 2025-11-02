#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "lcd_driver.h"
#include "mydefine.h"
#include "rgb_led.h"
#include "myheader.h"
#include "network.h"

/**
 * @brief 用于配网模式的 Web 服务器（监听端口 80）
 */
extern WebServer AP_server;

// 前置声明DNSServer类
class DNSServer;

/**
 * @brief DNS 服务器用于强制门户
 */
extern DNSServer dnsServer;

/**
 * @brief 从用户网页输入的 WiFi SSID 和密码
 */
extern String ssid_input, password_input;

/**
 * @brief 已保存的 WiFi SSID 和密码（由文件系统读取）
 */
extern String savedSSID, savedPassword;

/**
 * @brief WiFi连接状态枚举
 */
enum WiFiConnectionState {
    WIFI_IDLE,         // 初始状态，未开始连接
    WIFI_CONNECTING,   // 正在连接中
    WIFI_CONNECTED,    // 已连接
    WIFI_FAILED        // 连接失败
};

/**
 * @brief 当前WiFi连接状态
 */
extern WiFiConnectionState wifiConnectionState;

/**
 * @brief 配网模式状态标志，true 表示当前处于配网模式
 */
extern bool inConfigMode;

/**
 * @brief 进入配网模式，开启软AP及配置网页服务器
 * 
 * 设置设备进入配网模式，启动名为 "1602A_Config" 的 WiFi 软AP，  
 * 并开启HTTP服务器，处理根路径（"/"）和保存配置（"/set"）请求。  
 * 同时通过串口和LCD显示当前软AP的IP地址，方便用户连接配置。
 */
void enterConfigMode();      // 启动配网页面（配网模式）

/**
 * @brief 连接到保存的 WiFi 网络，如失败则进入配网模式
 * 
 * 该函数首先尝试加载保存的 WiFi 信息（SSID 和密码），  
 * 若无保存信息则自动进入配网模式。  
 * 若有信息则尝试连接，并在 15 秒内等待连接成功，  
 * 通过 LED 灯颜色和 LCD 显示连接过程中的状态：  
 * - 紫色：初次配网  
 * - 蓝色：连接中
 * - 绿色：连接成功
 * - 红色：连接失败，将进入配网模式
 */
void connectToWiFi();        // 尝试连接 WiFi，失败进入配网模式

/**
 * @brief 保存 WiFi 信息（SSID 和密码）到 FFat 文件系统
 * 
 * 将 WiFi 的 SSID 和密码写入 "/wifi.txt" 文件，
 * 以便设备重启后能够读取并自动连接。
 * 成功保存后会通过串口和 LCD 显示提示信息。
 * 失败时会在串口打印错误提示并在 LCD 显示错误信息。
 * 
 * @param[in] ssid WiFi 名称（SSID）
 * @param[in] password WiFi 密码
 */
void saveWiFiCredentials(const String& ssid, const String& password);

/**
 * @brief 从 FFat 文件系统加载保存的 WiFi 信息（SSID 和密码）
 * 
 * 如果 "/wifi.txt" 文件存在，则读取文件中保存的 SSID 和密码，
 * 并去除前后空白字符，赋值给全局变量 savedSSID 和 savedPassword。
 * 若文件不存在或打开失败，则将 savedSSID 和 savedPassword 置为空字符串。
 */
void loadWiFiCredentials();

/**
 * @brief 处理配网网页请求的内部函数
 * 
 * 该函数响应客户端对根路径（"/"）的HTTP请求，
 * 返回配网用的HTML页面内容。
 * 一般为内部调用，无需外部直接调用。
 */
void handleRoot();

/**
 * @brief 处理保存WiFi信息的请求（内部网页处理函数）
 * 
 * 从网页表单中读取 SSID 和密码参数，  
 * 调用 saveWiFiCredentials 保存到文件系统，  
 * 并返回保存成功的文本响应，随后延时重启设备。  
 * 该函数一般作为内部回调使用，无需外部调用。
 */
void handleSet();

/**
 * @brief 初始化 WiFi 连接流程（含按键检测与服务器启动）
 * 
 * 如果检测到中键（BUTTEN_CENTER）被按下，则强制进入配网模式；  
 * 否则尝试连接到已保存的 WiFi 网络，连接失败时将自动切换为配网。  
 * 无论哪种方式，最后都启动主服务器
 */
void wifiinit();

#endif