#ifndef NETWORK_H
#define NETWORK_H

#include "mydefine.h"
#include "rgb_led.h"
#include "myhader.h"
#include "protocol.h"

// 外部变量声明（在 network.cpp 中定义，在其他文件中使用）
extern WiFiServer server;
extern WiFiClient client;
extern bool clientConnected;
extern std::vector<uint8_t> recvBuffer;
extern unsigned long lastClientActivity;

// 函数声明
void acceptClientIfNew();
void receiveClientData();
bool isHeartbeatPacket(const std::vector<uint8_t>& packet);
uint16_t parseFrameInterval(const std::vector<uint8_t>& packet);
bool isHeartbeatPacket(const std::vector<uint8_t>& packet);
void prosessIncoming(const uint8_t* data, size_t len);

#endif