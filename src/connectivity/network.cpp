#include "./connectivity/network.h"

#include "./services/frame_stats.h"

volatile uint32_t gFramesEnqueued = 0;
volatile uint32_t gFramesDropped = 0;

WiFiClient client;  // 定义客户端对象
SemaphoreHandle_t clientMutex = nullptr;
std::deque<FramePacket> frameCache;     // 用双端队列方便插入删除

// 连接用
WiFiServer server(CONNECT_PORT);        //连接端口
bool clientConnected = false;

// 网络用缓存
std::vector<uint8_t> recvBuffer;
unsigned long lastClientActivity = 0;
static size_t recvHead = 0; // recvBuffer 中未消费数据的起始偏移，避免频繁 erase() 造成 O(n) 搬移

static size_t trimFrameCacheLockedByPolicy(uint32_t nowMs) {
    // 以“真实时间延迟”为主：
    // - 即使 frameIntervalMs=0（立即帧），也要限制端到端显示滞后
    // 以“数量”为辅：
    // - 避免内存被撑爆
    size_t dropped = 0;
    while (!frameCache.empty()) {
        const bool tooMany = frameCache.size() > MAX_CACHE_SIZE;
        const bool tooOld = (uint32_t)(nowMs - frameCache.front().enqueueMs) > (uint32_t)MAX_LATENCY_MS;
        if (!tooMany && !tooOld) {
            break;
        }
        frameCache.pop_front();
        dropped++;
    }
    return dropped;
}

static void logTrimIfNeeded(size_t dropped, uint32_t nowMs) {
    static uint32_t lastLogMs = 0;
    if (dropped == 0) return;
    if ((uint32_t)(nowMs - lastLogMs) < 500) return;
    lastLogMs = nowMs;
    LOG_NETWORK_WARN("Frame cache trimmed (dropped=%u), keep latency <= %u ms (size=%u).",
        static_cast<unsigned int>(dropped),
        static_cast<unsigned int>(MAX_LATENCY_MS),
        static_cast<unsigned int>(frameCache.size()));
}

void initNetwork() {
    if (clientMutex == nullptr) {
        clientMutex = xSemaphoreCreateMutex();
    }

    // 预留容量，减少运行时反复扩容/搬移
    if (recvBuffer.capacity() < MAX_RECV_BUFFER_SIZE) {
        recvBuffer.reserve(MAX_RECV_BUFFER_SIZE);
    }
}

// 解析帧率
uint16_t _parseFrameInterval(const std::vector<uint8_t>& packet) {
    if (packet.size() < 5) return 0;
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
    initNetwork();

    if (!clientConnected) {
        if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(25)) == pdTRUE) {
            client = server.accept();
            if (client) {
                clientConnected = true;
                // 低延迟：关闭 Nagle，减少小包合并带来的额外等待
                client.setNoDelay(true);
                updateColor(CRGB::Orange);      // RGB灯=黄色
                lastClientActivity = millis();
                recvBuffer.clear();
                recvHead = 0;

                LOG_NETWORK_INFO("Socket Client connected from %s:%d",
                    client.remoteIP().toString().c_str(),
                    client.remotePort());
            }
            xSemaphoreGive(clientMutex);
        }
    }
}

// 接收数据
void receiveClientData() {
    initNetwork();

    if (!clientConnected) return;

    bool connectedNow = false;
    int availableNow = 0;
    if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(25)) == pdTRUE) {
        connectedNow = clientConnected && client.connected();
        if (connectedNow) {
            availableNow = client.available();
        }
        xSemaphoreGive(clientMutex);
    } else {
        // 无法获取锁时，跳过本轮，避免与其他任务并发访问 client
        return;
    }

    if (connectedNow) {
        if (availableNow > 0) {   // 如果有数据可读
            uint8_t buf[256];

            int len = 0;
            if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                // 再次确认连接，避免锁外状态变化
                if (clientConnected && client.connected()) {
                    len = client.read(buf, sizeof(buf));
                } else {
                    len = -1;
                }
                xSemaphoreGive(clientMutex);
            } else {
                return;
            }

            if (len > 0) {
                // 以“未消费数据量”为准判断是否溢出；必要时先进行一次紧凑化，降低误判断连概率
                const size_t used = (recvBuffer.size() >= recvHead) ? (recvBuffer.size() - recvHead) : recvBuffer.size();
                if (used + static_cast<size_t>(len) > MAX_RECV_BUFFER_SIZE) {
                    if (recvHead > 0) {
                        recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + recvHead);
                        recvHead = 0;
                    }
                }
                if (recvBuffer.size() + static_cast<size_t>(len) > MAX_RECV_BUFFER_SIZE) {
                    LOG_NETWORK_ERROR(
                        "Receive buffer overflow (incoming=%d, current=%u, max=%u), disconnecting client.",
                        len,
                        static_cast<unsigned int>(recvBuffer.size()),
                        static_cast<unsigned int>(MAX_RECV_BUFFER_SIZE));
                    recvBuffer.clear();
                    recvHead = 0;
                    updateColor(CRGB::Green);
                    if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                        client.stop();
                        clientConnected = false;
                        xSemaphoreGive(clientMutex);
                    }
                    return;
                }

                recvBuffer.insert(recvBuffer.end(), buf, buf + len);    // 添加到接收缓存
                lastClientActivity = millis();
            } else if (len < 0) {
                // read() 失败时 len 可能为 -1；若不处理会导致 buf + len 指针越界，进而破坏堆/网络栈
                LOG_NETWORK_ERROR("Socket read failed (len=%d). Disconnecting client.", len);
                recvBuffer.clear();
                recvHead = 0;
                updateColor(CRGB::Green);
                if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    client.stop();
                    clientConnected = false;
                    xSemaphoreGive(clientMutex);
                }
                return;
            }
        }

        // 检查接收缓冲区大小，防止内存耗尽
        if (recvBuffer.size() > MAX_RECV_BUFFER_SIZE) {
            LOG_NETWORK_ERROR("Receive buffer overflow, disconnecting client. Size: %u", static_cast<unsigned int>(recvBuffer.size()));
            recvBuffer.clear();
            recvHead = 0;
            return;
        }

        // 处理完整包
        while ((recvBuffer.size() >= recvHead) && (recvBuffer.size() - recvHead >= 3)) {
            if (recvBuffer[recvHead] == 0xAA && recvBuffer[recvHead + 1] == 0x55) {   // 协议头
                uint8_t fullLen = recvBuffer[recvHead + 2];

                // 防御性校验：fullLen=0/1/2 会导致不前进的循环或无意义包；也避免异常大长度
                if (fullLen < 3 || fullLen > MAX_RECV_BUFFER_SIZE) {
                    LOG_NETWORK_WARN("Invalid packet length: %u. Resyncing...", static_cast<unsigned int>(fullLen));
                    recvHead++;
                    continue;
                }

                if (recvBuffer.size() - recvHead >= fullLen) {
                    std::vector<uint8_t> fullPacket(recvBuffer.begin() + recvHead, recvBuffer.begin() + recvHead + fullLen);  // 提取单个完整数据包
                    recvHead += fullLen;

                    if (_isHeartbeatPacket(fullPacket)) {
                        lastClientActivity = millis();
                        LOG_NETWORK_DEBUG("Heartbeat packet received.");
                    } 
                    else {
                        if (fullPacket.size() < 5) {
                            LOG_NETWORK_WARN("Non-heartbeat packet too short (len=%u), dropped.", static_cast<unsigned int>(fullPacket.size()));
                            continue;
                        }
                        uint16_t frameInterval = _parseFrameInterval(fullPacket);

                        // 一律入队，让显示侧消费；避免在收包路径直接渲染导致 TCP 缓冲被拖慢引发丢包/卡顿
                        // 对于“立即帧”(frameInterval=0)：只保留最新一帧，降低排队导致的显示滞后
                        const uint32_t nowMs = millis();
                        size_t dropped = 0;

                        if (frameInterval == 0) {
                            dropped += frameCache.size();
                            frameCache.clear();
                        } else if (frameCache.size() >= MAX_CACHE_SIZE && !frameCache.empty()) {
                            frameCache.pop_front();
                            dropped++;
                        }

                        frameCache.push_back({std::move(fullPacket), frameInterval, nowMs});

                        // 按真实最大允许延迟继续修剪（可能一次丢多帧）
                        dropped += trimFrameCacheLockedByPolicy(nowMs);
                        logTrimIfNeeded(dropped, nowMs);

                        // 统计
                        gFramesEnqueued++;
                        gFramesDropped += static_cast<uint32_t>(dropped);
                    }
                } 
                else { break; } // 跳出去继续等待完整包
            } 
            else {
                // 查找下一个协议头，如果找到则删除协议头之前的内容
                size_t pos = recvHead + 1; // 从第二个字节开始查找
                bool found = false;
                while (pos + 1 < recvBuffer.size()) {
                    if (recvBuffer[pos] == 0xAA && recvBuffer[pos + 1] == 0x55) {
                        found = true;
                        break;
                    }
                    pos++;
                }
                if (found) {
                    // 删除协议头之前的内容
                    recvHead = pos;
                } else {
                    // 没有找到下一个协议头，删除第一个字节
                    recvHead++;
                }
            }
        }

        // 偶尔做一次紧凑化，避免 recvBuffer 长期增大/碎片化
        if (recvHead > 0 && (recvHead > 256 || recvHead > recvBuffer.size() / 2)) {
            recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + recvHead);
            recvHead = 0;
        }

        // 超时断开连接
        if (millis() - lastClientActivity > CONNECT_TIMEOUT_MS) {
            updateColor(CRGB::Green);
            if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                client.stop();
                clientConnected = false;
                xSemaphoreGive(clientMutex);
            }
            LOG_NETWORK_INFO("Client connection timed out.");
        }

    } 
    // 客户端主动断开连接
    else {
        updateColor(CRGB::Green);
        if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            client.stop();
            clientConnected = false;
            xSemaphoreGive(clientMutex);
        }
        LOG_NETWORK_INFO("Client disconnected.");
    }
}