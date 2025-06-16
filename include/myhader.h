#ifndef MYHADER_H
#define MYHADER_H

// ==============================
// 标准库 / 系统功能头文件
// ==============================

#include <WiFi.h>             ///< ESP32 WiFi 功能支持（连接、热点、IP 获取等）
#include <WebServer.h>        ///< 简易 HTTP Web Server（用于配网页面）
#include <FFat.h>             ///< 使用 Flash Fat 文件系统（替代 SPIFFS，用于存储配置）

#include <freertos/FreeRTOS.h>   ///< FreeRTOS 基础支持
#include <freertos/task.h>       ///< FreeRTOS 任务管理
#include <freertos/semphr.h>     ///< FreeRTOS 信号量/互斥锁（用于线程同步）

#include "driver/gpio.h"         ///< GPIO 驱动底层访问（更细粒度控制引脚）

// ==============================
// 第三方库
// ==============================

#include <FastLED.h>          ///< 控制 WS2812 等 RGB 灯条的库

// ==============================
// STL 容器扩展
// ==============================

#include <map>                ///< 映射表容器（键值对查找）
#include <deque>              ///< 双端队列（可用于帧缓存、任务队列等）
#include <vector>             ///< 动态数组容器


#endif