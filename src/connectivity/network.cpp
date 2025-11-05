#include "network.h"

WiFiClient client;  // 定义客户端对象

struct FramePacket {
    std::vector<uint8_t> data;          // 完整包数据（含头长）
    uint16_t frameIntervalMs;           // 帧率，单位毫秒
};
std::deque<FramePacket> frameCache;     // 用双端队列方便插入删除

// 连接用
WiFiServer server(CONNECT_PORT);        //连接端口
bool clientConnected = false;

// 网络用缓存
std::vector<uint8_t> recvBuffer;
unsigned long lastClientActivity = 0;

// 解析帧率
uint16_t _parseFrameInterval(const std::vector<uint8_t>& packet) {
    return (uint16_t(packet[3]) << 8) | uint16_t(packet[4]);    // 帧率是第3和第4字节
}

// 判断心跳包
bool _isHeartbeatPacket(const std::vector<uint8_t>& packet) {
    return packet.size() == 4 &&
           packet[0] == 0xAA &&
           packet[1] == 0x55 &&
           packet[2] == 0x04 &&
           packet[3] == 0x02;
}

// 连接客户端
void acceptClientIfNew() {
    if (!clientConnected) {
        client = server.accept();
        if (client) {
            clientConnected = true;
            updateColor(CRGB::Orange);      // RGB灯=黄色
            lastClientActivity = millis();
            recvBuffer.clear();

            LOG_NETWORK_INFO("Socket Client connected from %s:%d",
                client.remoteIP().toString().c_str(),
                client.remotePort());
        }
    }
}

// 接收数据
void receiveClientData() {
    if (!clientConnected) return;

    if (client.connected()) {
        if (client.available() > 0) {   // 如果有数据可读
            uint8_t buf[256];
            int len = client.read(buf, sizeof(buf));
            recvBuffer.insert(recvBuffer.end(), buf, buf + len);    // 添加到接收缓存
            lastClientActivity = millis();
        }

        // 检查接收缓冲区大小，防止内存耗尽
        if (recvBuffer.size() > MAX_RECV_BUFFER_SIZE) {
            LOG_NETWORK_ERROR("Receive buffer overflow, disconnecting client. Size: %d", recvBuffer.size());
            recvBuffer.clear();
            return;
        }

        // 处理完整包
        while (recvBuffer.size() >= 3) {
            if (recvBuffer[0] == 0xAA && recvBuffer[1] == 0x55) {   // 协议头
                uint8_t fullLen = recvBuffer[2];

                if (recvBuffer.size() >= fullLen) {
                    std::vector<uint8_t> fullPacket(recvBuffer.begin(), recvBuffer.begin() + fullLen);  // 提取单个完整数据包
                    recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + fullLen);                 // 移除已处理数据

                    if (_isHeartbeatPacket(fullPacket)) {
                        lastClientActivity = millis();
                        LOG_NETWORK_DEBUG("Heartbeat packet received.");
                    } 
                    else {
                        uint16_t frameInterval = _parseFrameInterval(fullPacket);

                        // 帧率为0表示立即处理
                        if (frameInterval == 0) {
                            processIncoming(fullPacket.data(), fullPacket.size());
                        } 
                        else {
                            if (frameCache.size() < MAX_CACHE_SIZE) {
                                //Serial.print("buffer size: ");
                                //Serial.println(frameCache.size());
                                frameCache.push_back({fullPacket, frameInterval});
                            } 
                            else {
                                LOG_NETWORK_WARN("Frame cache full, dropping new frame.");
                            }
                        }
                    }
                } 
                else { break; } // 跳出去继续等待完整包
            } 
            else {
                // 查找下一个协议头，如果找到则删除协议头之前的内容
                size_t pos = 1; // 从第二个字节开始查找
                bool found = false;
                while (pos < recvBuffer.size() - 1) {
                    if (recvBuffer[pos] == 0xAA && recvBuffer[pos + 1] == 0x55) {
                        found = true;
                        break;
                    }
                    pos++;
                }
                if (found) {
                    // 删除协议头之前的内容
                    recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + pos);
                } else {
                    // 没有找到下一个协议头，删除第一个字节
                    recvBuffer.erase(recvBuffer.begin());
                }
            }
        }

        // 超时断开连接
        if (millis() - lastClientActivity > CONNECT_TIMEOUT_MS) {
            updateColor(CRGB::Green);
            client.stop();
            clientConnected = false;
            LOG_NETWORK_INFO("Client connection timed out.");
        }

    } 
    // 客户端主动断开连接
    else {
        updateColor(CRGB::Green);
        client.stop();
        clientConnected = false;
        LOG_NETWORK_INFO("Client disconnected.");
    }
}