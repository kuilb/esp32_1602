#include "./connectivity/network.h"

#include "./services/frame_stats.h"
#include "./hardware/buzzer.h"

volatile uint32_t gFramesEnqueued = 0;
volatile uint32_t gFramesDropped = 0;

WiFiClient client;  // 定义客户端对象
SemaphoreHandle_t clientMutex = nullptr;
std::deque<FramePacket> frameCache;     // 用双端队列方便插入删除

// 连接用
WiFiServer server(CONNECT_PORT);        //连接端口
bool clientConnected = false;
static volatile bool s_appInterfaceNetworkRequired =
#if APP_INTERFACE_DEFAULT_NETWORK_REQUIRED
    true;
#else
    false;
#endif
static volatile bool s_appInterfaceNetworkRequiredOverridden = false;

// 网络用缓存
std::vector<uint8_t> recvBuffer;
unsigned long lastClientActivity = 0;
static size_t recvHead = 0; // recvBuffer 中未消费数据的起始偏移，避免频繁 erase() 造成 O(n) 搬移
static std::vector<uint8_t> s_lastDisplayPacket;
static uint32_t s_duplicateDisplayPackets = 0;
static uint32_t s_lastDuplicateLogMs = 0;
static bool s_clientHasReceivedPayload = false;
static const uint32_t kInitialClientSilentTimeoutMs = 20000;
static const uint32_t kPostAcceptConnectedGraceMs = 1200;
static const uint32_t kTransientDisconnectDebounceMs = 1200;
static bool s_connectedCached = false;
static uint32_t s_lastConnectedCheckMs = 0;
static uint32_t s_disconnectSuspectSinceMs = 0;

static bool _checkClientConnectedLocked(uint32_t nowMs) {
    if (!clientConnected) {
        s_connectedCached = false;
        return false;
    }

    // 新连接短时间内可能出现底层 connected() 抖动，先按“已连接”处理，
    // 给首包/心跳一个到达窗口，避免误判秒断。
    if (!s_clientHasReceivedPayload
        && (uint32_t)(nowMs - lastClientActivity) < kPostAcceptConnectedGraceMs) {
        s_connectedCached = true;
        return true;
    }

    // 限频检查底层 connected()，避免高频调用触发大量核心日志。
    if (!s_connectedCached || (uint32_t)(nowMs - s_lastConnectedCheckMs) >= 200) {
        s_connectedCached = client.connected();
        s_lastConnectedCheckMs = nowMs;
    }
    return s_connectedCached;
}

static void _cleanupClientStreamState() {
    recvBuffer.clear();
    recvHead = 0;
    frameCache.clear();
    s_lastDisplayPacket.clear();
    s_clientHasReceivedPayload = false;
    s_connectedCached = false;
    s_lastConnectedCheckMs = 0;
    s_disconnectSuspectSinceMs = 0;
}

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

static void logTrimIfNeeded(size_t policyDropped, size_t replaceDropped, uint32_t nowMs) {
    static uint32_t lastPolicyLogMs = 0;
    static uint32_t lastReplaceLogMs = 0;
    static uint32_t replaceAccum = 0;

    if (policyDropped > 0 && (uint32_t)(nowMs - lastPolicyLogMs) >= 500) {
        lastPolicyLogMs = nowMs;
        LOG_NETWORK_WARN("Frame cache policy drop (dropped=%u), keep latency <= %u ms (size=%u).",
            static_cast<unsigned int>(policyDropped),
            static_cast<unsigned int>(MAX_LATENCY_MS),
            static_cast<unsigned int>(frameCache.size()));
    }

    if (replaceDropped > 0) {
        replaceAccum += static_cast<uint32_t>(replaceDropped);
        if ((uint32_t)(nowMs - lastReplaceLogMs) >= 2000) {
            lastReplaceLogMs = nowMs;
            LOG_NETWORK_DEBUG("Realtime frame replacement dropped=%u (2s)",
                static_cast<unsigned int>(replaceAccum));
            replaceAccum = 0;
        }
    }
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

// V2 TONE命令包：
// [AA][55][LEN][0x20][0x02][seq][freqH][freqL][durH][durL][optional volume]
static bool _isToneCommandPacket(const std::vector<uint8_t>& packet) {
    if (packet.size() < 10) return false;
    if (packet[0] != 0xAA || packet[1] != 0x55) return false;
    if (packet[3] != 0x20 || packet[4] != 0x02) return false;
    return true;
}

static bool _handleToneCommandPacket(const std::vector<uint8_t>& packet) {
    if (!_isToneCommandPacket(packet)) {
        return false;
    }

    // 兼容 10字节(无音量) 和 11字节(带音量)
    if (packet.size() != 10 && packet.size() != 11) {
        LOG_NETWORK_WARN("Invalid TONE packet length: %u", static_cast<unsigned int>(packet.size()));
        return true;
    }

    const uint16_t frequency = (uint16_t(packet[6]) << 8) | uint16_t(packet[7]);
    const uint16_t duration = (uint16_t(packet[8]) << 8) | uint16_t(packet[9]);
    uint8_t volume = buzzerGetVolume();
    if (packet.size() == 11) {
        volume = static_cast<uint8_t>(constrain(packet[10], 0, 100));
    }

    if (frequency == 0 || duration == 0) {
        LOG_NETWORK_WARN("Ignore TONE packet with invalid params: f=%u, d=%u",
            static_cast<unsigned int>(frequency),
            static_cast<unsigned int>(duration));
        return true;
    }

    buzzerPlayTone(frequency, duration, volume);
    LOG_NETWORK_DEBUG("TONE cmd: f=%uHz d=%ums v=%u",
        static_cast<unsigned int>(frequency),
        static_cast<unsigned int>(duration),
        static_cast<unsigned int>(volume));
    return true;
}

// 连接客户端
void acceptClientIfNew() {
    initNetwork();

    if (!clientConnected) {
        if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(25)) == pdTRUE) {
            client = server.accept();
            if (client) {
                _cleanupClientStreamState();
                clientConnected = true;
                s_connectedCached = true;
                s_lastConnectedCheckMs = millis();
                // 立即唤醒 WiFi：防止 sleep 模式在连接建立瞬间导致 RST
                WiFi.setSleep(false);
                // 低延迟：关闭 Nagle，减少小包合并带来的额外等待
                client.setNoDelay(true);
                updateColor(CRGB::Orange);      // RGB灯=黄色
                lastClientActivity = millis();

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

    if (!clientConnected) {
        _cleanupClientStreamState();
        return;
    }

    bool connectedNow = false;
    int availableNow = 0;
    uint32_t nowMs = 0;
    if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(25)) == pdTRUE) {
        nowMs = millis();
        connectedNow = _checkClientConnectedLocked(nowMs);
        if (clientConnected) {
            availableNow = client.available();
        }

        // 某些场景 connected() 会短暂抖动为 false，但缓冲区仍有数据可读，
        // 此时不应判定断连。
        if (!connectedNow && availableNow > 0) {
            connectedNow = true;
            s_connectedCached = true;
            s_lastConnectedCheckMs = nowMs;
        }
        xSemaphoreGive(clientMutex);
    } else {
        // 无法获取锁时，跳过本轮，避免与其他任务并发访问 client
        return;
    }

    // 断连去抖：必须持续一段时间判定为断开，避免误判导致重连风暴。
    if (!connectedNow) {
        if (s_disconnectSuspectSinceMs == 0) {
            s_disconnectSuspectSinceMs = nowMs;
            return;
        }

        if ((uint32_t)(nowMs - s_disconnectSuspectSinceMs) < kTransientDisconnectDebounceMs) {
            return;
        }
    } else {
        s_disconnectSuspectSinceMs = 0;
    }

    if (connectedNow) {
        if (availableNow > 0) {   // 如果有数据可读
            uint8_t buf[256];

            int len = 0;
            if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                // 锁内以缓存连接态为准，避免重复调用 connected() 产生额外日志与开销。
                if (clientConnected) {
                    len = client.read(buf, sizeof(buf));
                } else {
                    len = -1;
                }
                xSemaphoreGive(clientMutex);
            } else {
                return;
            }

            if (len > 0) {
                s_clientHasReceivedPayload = true;
                // 以“未消费数据量”为准判断是否溢出；必要时先进行一次紧凑化，降低误判断连概率
                // Lazy compaction: erase dead prefix only when it occupies half the buffer,
                // amortising the O(n) memmove; ensures overflow check always uses actual unread bytes.
                if (recvHead > 0 && recvHead >= recvBuffer.size() / 2) {
                    recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + recvHead);
                    recvHead = 0;
                }
                const size_t unread = recvBuffer.size() - recvHead;
                if (unread + static_cast<size_t>(len) > MAX_RECV_BUFFER_SIZE) {
                    LOG_NETWORK_ERROR(
                        "Receive buffer overflow (incoming=%d, unread=%u, max=%u), disconnecting client.",
                        len,
                        static_cast<unsigned int>(unread),
                        static_cast<unsigned int>(MAX_RECV_BUFFER_SIZE));
                    _cleanupClientStreamState();
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
                s_connectedCached = false;
                _cleanupClientStreamState();
                updateColor(CRGB::Green);
                if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    client.stop();
                    clientConnected = false;
                    xSemaphoreGive(clientMutex);
                }
                return;
            }
        }

        // 检查接收缓冲区大小，防止内存耗尽（此处同样基于未读字节数）
        if (recvBuffer.size() - recvHead > MAX_RECV_BUFFER_SIZE) {
            LOG_NETWORK_ERROR("Receive buffer overflow, disconnecting client. Unread: %u", static_cast<unsigned int>(recvBuffer.size() - recvHead));
            _cleanupClientStreamState();
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
                        // 蜂鸣器命令独立于显示链路：收到后立即播放，不入显示缓存。
                        if (_handleToneCommandPacket(fullPacket)) {
                            continue;
                        }

                        if (fullPacket.size() < 5) {
                            LOG_NETWORK_WARN("Non-heartbeat packet too short (len=%u), dropped.", static_cast<unsigned int>(fullPacket.size()));
                            continue;
                        }

                        // 连续重复帧直接丢弃：对所有显示帧生效，降低无效入队/解析/差分比较带来的CPU与发热。
                        if (!s_lastDisplayPacket.empty()
                            && s_lastDisplayPacket.size() == fullPacket.size()
                            && memcmp(s_lastDisplayPacket.data(), fullPacket.data(), fullPacket.size()) == 0) {
                            s_duplicateDisplayPackets++;
                            gFramesDropped++;

                            const uint32_t nowMs = millis();
                            if ((uint32_t)(nowMs - s_lastDuplicateLogMs) >= 2000) {
                                LOG_NETWORK_INFO("Dropping duplicate display frames: %u", static_cast<unsigned int>(s_duplicateDisplayPackets));
                                s_duplicateDisplayPackets = 0;
                                s_lastDuplicateLogMs = nowMs;
                            }
                            continue;
                        }

                        // 菜单态不消费流媒体帧：直接丢弃，避免无意义解析/缓存导致发热。
                        if (inMenuMode) {
                            static uint32_t droppedInMenu = 0;
                            static uint32_t lastMenuDropLogMs = 0;
                            droppedInMenu++;
                            gFramesDropped++;

                            const uint32_t nowMs = millis();
                            if ((uint32_t)(nowMs - lastMenuDropLogMs) >= 2000) {
                                LOG_NETWORK_INFO("Dropping stream frames in menu mode (count=%u)", static_cast<unsigned int>(droppedInMenu));
                                droppedInMenu = 0;
                                lastMenuDropLogMs = nowMs;
                            }
                            continue;
                        }

                        uint16_t frameInterval = _parseFrameInterval(fullPacket);

                        // 一律入队，让显示侧消费；避免在收包路径直接渲染导致 TCP 缓冲被拖慢引发丢包/卡顿
                        // 对于“立即帧”(frameInterval=0)：只保留最新一帧，降低排队导致的显示滞后
                        const uint32_t nowMs = millis();
                        size_t droppedByReplace = 0;

                        if (frameInterval == 0) {
                            // 即时帧保留最近 N 帧，既控制延迟也给显示侧留出小幅追帧空间。
                            while (!frameCache.empty() && frameCache.size() >= IMMEDIATE_FRAME_CACHE_KEEP) {
                                frameCache.pop_front();
                                droppedByReplace++;
                            }
                        }

                        if (frameCache.size() >= MAX_CACHE_SIZE && !frameCache.empty()) {
                            frameCache.pop_front();
                            droppedByReplace++;
                        }

                        frameCache.push_back({std::move(fullPacket), frameInterval, nowMs});
                        s_lastDisplayPacket = frameCache.back().data;

                        // 按真实最大允许延迟继续修剪（可能一次丢多帧）
                        const size_t droppedByPolicy = trimFrameCacheLockedByPolicy(nowMs);
                        const size_t dropped = droppedByReplace + droppedByPolicy;
                        logTrimIfNeeded(droppedByPolicy, droppedByReplace, nowMs);

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
        const uint32_t timeoutLimitMs = s_clientHasReceivedPayload ? CONNECT_TIMEOUT_MS : kInitialClientSilentTimeoutMs;
        if (millis() - lastClientActivity > timeoutLimitMs) {
            const bool hadPayload = s_clientHasReceivedPayload;
            updateColor(CRGB::Green);
            if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                client.stop();
                clientConnected = false;
                s_connectedCached = false;
                xSemaphoreGive(clientMutex);
            }
            _cleanupClientStreamState();
            if (hadPayload) {
                LOG_NETWORK_INFO("Client connection timed out.");
            } else {
                LOG_NETWORK_INFO("Client silent after connect, closed by timeout (%u ms).",
                    static_cast<unsigned int>(timeoutLimitMs));
            }
        }

    } 
    // 客户端主动断开连接
    else {
        updateColor(CRGB::Green);
        if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            client.stop();
            clientConnected = false;
            s_connectedCached = false;
            xSemaphoreGive(clientMutex);
        }
        _cleanupClientStreamState();
        LOG_NETWORK_INFO("Client disconnected.");
    }
}

void setAppInterfaceNetworkRequired(bool required) {
    s_appInterfaceNetworkRequired = required;
    s_appInterfaceNetworkRequiredOverridden = true;
}

void resetAppInterfaceNetworkRequiredToDefault() {
    s_appInterfaceNetworkRequired =
#if APP_INTERFACE_DEFAULT_NETWORK_REQUIRED
        true;
#else
        false;
#endif
    s_appInterfaceNetworkRequiredOverridden = false;
}

bool isAppInterfaceNetworkRequired() {
    return s_appInterfaceNetworkRequired;
}

bool hasAppInterfaceNetworkRequirementOverride() {
    return s_appInterfaceNetworkRequiredOverridden;
}