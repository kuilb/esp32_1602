#ifndef NETWORK_H
#define NETWORK_H

#include "mydefine.h"
#include "rgb_led.h"
#include "myhader.h"
#include "protocol.h"

/**
 * @brief 外部变量声明，用于网络模块间共享
 * 
 * 这些变量在 network.cpp 文件中定义，供其他文件引用使用。
 */
extern WiFiServer server;                   /**< WiFi 服务器对象 */
extern WiFiClient client;                   /**< 当前连接的客户端对象 */
extern bool clientConnected;                /**< 客户端连接状态标志 */
extern std::vector<uint8_t> recvBuffer;     /**< 接收数据缓冲区 */
extern unsigned long lastClientActivity;    /**< 上次客户端活动时间，单位毫秒 */


/**
 * @brief 如果当前无客户端连接，则尝试接受新的客户端连接
 * 
 * - 检查是否已有客户端连接
 * - 如果没有，调用 server.accept() 接收新的客户端
 * - 成功连接后更新 client 对象，标记连接状态，清空接收缓冲区
 * - 记录最后活动时间，并打印连接日志
 */
void acceptClientIfNew();

/**
 * @brief 接收并处理客户端传输的数据
 * 
 */
void receiveClientData();

/**
 * @brief 从数据包中解析帧率间隔（单位通常为毫秒）
 * 
 * 帧率信息存储在数据包的第3和第4字节（索引2和3），
 * 高字节在第3字节，低字节在第4字节，组成一个16位无符号整数。
 * 
 * @param[in] packet 包含帧率信息的数据包字节向量
 * @return uint16_t 解析出的帧率间隔值
 */
uint16_t parseFrameInterval(const std::vector<uint8_t>& packet);

/**
 * @brief 判断数据包是否为心跳包
 * 
 * @param[in] packet 待判断的数据包字节向量
 * @return true 如果是心跳包
 * @return false 如果不是心跳包
 */
bool isHeartbeatPacket(const std::vector<uint8_t>& packet);

#endif