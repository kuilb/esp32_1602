#include "network.h"

struct FramePacket {
    std::vector<uint8_t> data;          // 完整包数据（含头长）
    uint16_t frameIntervalMs;           // 帧率，单位毫秒
};
std::deque<FramePacket> frameCache;     // 用双端队列方便插入删除

// 连接用
WiFiServer server(CONNECT_PORT);        //连接端口
const long timeoutTime = 5000;          //连接超时时间5S
bool clientConnected = false;

// 网络用缓存
std::vector<uint8_t> recvBuffer;
unsigned long lastClientActivity = 0;

// 解析帧率
uint16_t parseFrameInterval(const std::vector<uint8_t>& packet) {
    // 帧率是第3和第4字节
    return (uint16_t(packet[3]) << 8) | uint16_t(packet[4]);
}

// 判断心跳包
bool isHeartbeatPacket(const std::vector<uint8_t>& packet) {
    return packet.size() == 4 &&
           packet[0] == 0xAA &&
           packet[1] == 0x55 &&
           packet[2] == 0x04 &&
           packet[3] == 0x02;
}

// 连接客户端
void acceptClientIfNew() {
    if (!clientConnected) {
        WiFiClient newClient = server.accept();
        if (newClient) {
            client = newClient;
            clientConnected = true;
            lastClientActivity = millis();
            recvBuffer.clear();

            Serial.print("Time:");
            Serial.print(millis());
            Serial.println(" Socket Client connected.");
        }
    }
}

// 接收数据
void receiveClientData() {
    if (!clientConnected) return;

    if (client.connected()) {
        updateColor(CRGB::Orange);      // RGB灯=黄色
        if (client.available() > 0) {
            uint8_t buf[256];
            int len = client.read(buf, sizeof(buf));
            recvBuffer.insert(recvBuffer.end(), buf, buf + len);
            lastClientActivity = millis();
        }

        // 处理完整包
        while (recvBuffer.size() >= 3) {
            if (recvBuffer[0] == 0xAA && recvBuffer[1] == 0x55) {
                uint8_t bodyLen = recvBuffer[2];
                unsigned int fullLen = bodyLen;

                if (recvBuffer.size() >= fullLen) {
                    std::vector<uint8_t> fullPacket(recvBuffer.begin(), recvBuffer.begin() + fullLen);
                    recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + fullLen);

                    if (isHeartbeatPacket(fullPacket)) {
                        lastClientActivity = millis();
                        Serial.println("[Heartbeat] Received.");
                    } 
                    else {
                        uint16_t frameInterval = parseFrameInterval(fullPacket);

                        // 无缓存, 尝试播放当前帧
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
                                Serial.println("缓存满，丢弃新帧");
                            }
                        }
                    }
                } 
                else { break; } // 跳出去继续等待完整包
            } 
            else {
                recvBuffer.erase(recvBuffer.begin());       // 删除无法识别的头部，继续找包头
            }
        }

        // 超时断开连接
        if (millis() - lastClientActivity > timeoutTime) {
            updateColor(CRGB::Green);
            client.stop();
            clientConnected = false;
            Serial.println("Timeout. Client disconnected.");
        }

    } 
    // 客户端主动断开连接
    else {
        updateColor(CRGB::Green);
        client.stop();
        clientConnected = false;
        Serial.println("Client disconnected.");
    }
}
