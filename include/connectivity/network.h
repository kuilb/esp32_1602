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

#include "mydefine.h"
#include "rgb_led.h"
#include "myheader.h"
#include "protocol.h"

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