#ifndef MYHADER_H
#define MYHADER_H

// ==============================
// 标准库 / 系统功能头文件
// ==============================

#include <WiFi.h>             ///< ESP32 WiFi 功能支持（连接、热点、IP 获取等）
#include <WiFiClientSecure.h> ///< 支持 HTTPS 的 WiFi 客户端
#include <HTTPClient.h>       ///< HTTP 客户端（用于网络请求）
#include <WebServer.h>        ///< 简易 HTTP Web Server（用于配网页面）
#include <FFat.h>             ///< 使用 Flash Fat 文件系统（用于存储配置）
#include <LittleFS.h>
#include <SPIFFS.h>
#include <time.h>             ///< 标准 time 结构

#include <freertos/FreeRTOS.h>   ///< FreeRTOS 基础支持
#include <freertos/task.h>       ///< FreeRTOS 任务管理
#include <freertos/semphr.h>     ///< FreeRTOS 信号量/互斥锁（用于线程同步）

#include "driver/gpio.h"         ///< GPIO 驱动底层访问


// ==============================
// 第三方库
// ==============================

#include <FastLED.h>          ///< 控制 WS2812 等 RGB 灯条的库
#include <ArduinoJson.h>      ///< JSON 解析库（用于天气数据解析）
#include <sodium.h>           ///< libsodium 加密库（用于 JWT 签名）
#include <zlib_turbo.h>       ///< Gzip 解压库

// ==============================
// STL 容器扩展
// ==============================

#include <deque>              ///< 双端队列（用于帧缓存）
#include <vector>             ///< 动态数组容器


#endif