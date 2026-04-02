/**
 * @file network.h
 * @brief 网络模块头文件，提供网络连接和数据处理功能
 *
 * 此文件声明网络服务器、客户端和数据接收的函数和变量
 *
 * @author kulib
 * @date 2025-11-05
 */
#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <deque>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "mydefine.h"
#include "./hardware/rgb_led.h"
#include "./services/protocol.h"

/**
 * @brief 外部变量声明，用于网络模块间共享
 * 
 * 这些变量在 network.cpp 文件中定义，供其他文件引用使用
 */
extern WiFiServer server;                   /**< WiFi 服务器对象 */
extern WiFiClient client;                   /**< 当前连接的客户端对象 */
extern bool clientConnected;                /**< 客户端连接状态标志 */
extern std::vector<uint8_t> recvBuffer;     /**< 接收数据缓冲区 */
extern unsigned long lastClientActivity;    /**< 上次客户端活动时间戳 */
extern SemaphoreHandle_t clientMutex;       /**< client/server 互斥锁，避免多任务并发读写/关闭导致 lwIP 断言 */

/**
 * @brief 帧缓存元素
 * @details
 * - frameIntervalMs: 发送端期望的帧间隔（用于播放节流）
 * - enqueueMs: 本机接收并入队的时间戳（用于限制真实显示延迟）
 */
struct FramePacket {
	std::vector<uint8_t> data;
	uint16_t frameIntervalMs;
	uint32_t enqueueMs;
};

/**
 * @brief 帧缓存队列（network.cpp 中定义，playbuffer.cpp 中消费）
 */
extern std::deque<FramePacket> frameCache;

/**
 * @brief 初始化网络模块内部资源（如互斥锁）
 * @details 可安全重复调用；建议在创建会访问 client 的任务前调用。
 */
void initNetwork();

/**
 * @brief 如果当前无客户端连接，则尝试接受新的客户端连接
 * @details 检查连接状态，如果未连接则调用 server.accept() 接受新客户端，并更新状态和日志
 */
void acceptClientIfNew();

/**
 * @brief 接收并处理客户端传输的数据
 * @details 检查客户端连接状态，读取可用数据到缓冲区，解析完整包并处理心跳或帧数据
 */
void receiveClientData();

#endif