#include "mydefine.h"

#include "./menu/menu.h"
#include "./applications/clock.h"

#include "./connectivity/wifi_config.h"
#include "./connectivity/network.h"
#include "./connectivity/jwt_auth.h"

#include "./hardware/button.h"
#include "./hardware/lcd_driver.h"
#include "./hardware/rgb_led.h"
#include "./hardware/fuel_gauge.h"
#include "./hardware/buzzer.h"

#include "./hardware/opt3001.h"
#include "./services/auto_brightness.h"
#include "./services/config_manager.h"
#include "./services/kanamap.h"
#include "./services/protocol.h"
#include "./services/playbuffer.h"
#include "./services/wifi_config_manager.h"
#include "./services/qweather_auth_config_manager.h"
#include "./services/sleep_manager.h"

#include "./utils/logger.h"

#include <cstdlib>
#include <esp_system.h>
#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp32-hal-cpu.h>
#ifndef ENABLE_DYNAMIC_CPU_FREQ
#define ENABLE_DYNAMIC_CPU_FREQ 0
#endif

WifiConfigManager wifiConfigManager("/wifi_config.txt");
QWeatherAuthConfigManager qweatherAuthConfigManager("/qweather_auth_config.txt");

static TaskHandle_t s_taskHealthMonitorHandle = nullptr;
static esp_pm_lock_handle_t s_noLightSleepLock = nullptr;
static esp_pm_lock_handle_t s_cpuFreqMaxLock = nullptr;
static bool s_cpuFreqMaxLockHeld = false;
static bool s_noLightSleepLockHeld = false;
static bool s_lowTxPowerMode = false;
static int s_currentCpuFreqMHz = 240;
static uint8_t s_wifiRxPsLevel = 0xFF; // 0=NONE, 1=MIN_MODEM, 2=MAX_MODEM

static void _setWiFiTxPowerMode(bool lowPower);
static void _setWiFiRxPowerSave(uint8_t level);

static void _setCpuTargetFreqMhz(int targetMHz) {
    if (targetMHz <= 0 || targetMHz == s_currentCpuFreqMHz) {
        return;
    }
    if (setCpuFrequencyMhz(targetMHz)) {
        s_currentCpuFreqMHz = targetMHz;
        LOG_SYSTEM_INFO("CPU frequency -> %d MHz", s_currentCpuFreqMHz);
    } else {
        LOG_SYSTEM_WARN("Failed to set CPU frequency -> %d MHz", targetMHz);
    }
}

static void _setWiFiRxPowerSave(uint8_t level) {
    if (level > 2 || level == s_wifiRxPsLevel) {
        return;
    }

    wifi_ps_type_t psType = WIFI_PS_NONE;
    const char* name = "NONE";
    if (level == 1) {
        psType = WIFI_PS_MIN_MODEM;
        name = "MIN_MODEM";
    } else if (level == 2) {
        psType = WIFI_PS_MAX_MODEM;
        name = "MAX_MODEM";
    }

    WiFi.setSleep(level != 0);
    esp_err_t err = esp_wifi_set_ps(psType);
    if (err == ESP_OK) {
        s_wifiRxPsLevel = level;
        LOG_SYSTEM_INFO("WiFi RX power-save -> %s", name);
    } else {
        LOG_SYSTEM_WARN("Failed to set WiFi RX power-save -> %s (err=%d)",
            name,
            static_cast<int>(err));
    }
}

static void _stabilizePowerManagement() {
    // 启用 light sleep：当所有 FreeRTOS 任务均处于 blocked 状态时，
    // 系统自动进入 light sleep（由 tickless idle 触发）。
    esp_pm_config_esp32s3_t pm_config = {};
    pm_config.max_freq_mhz    = 240;
    pm_config.min_freq_mhz    = 80;
    pm_config.light_sleep_enable = true;
    esp_err_t pm_err = esp_pm_configure(&pm_config);
    if (pm_err != ESP_OK) {
        LOG_SYSTEM_WARN("esp_pm_configure failed: %d (light sleep may not activate)", static_cast<int>(pm_err));
    } else {
        LOG_SYSTEM_INFO("PM configured: light sleep ENABLED, max=%dMHz, min=%dMHz",
            pm_config.max_freq_mhz, pm_config.min_freq_mhz);
    }

    esp_err_t err = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "nols", &s_noLightSleepLock);
    if (err == ESP_OK && s_noLightSleepLock != nullptr) {
        // 初始刨持锁：防止 setup() 期间过早进入 light sleep
        err = esp_pm_lock_acquire(s_noLightSleepLock);
        if (err == ESP_OK) {
            s_noLightSleepLockHeld = true;
            LOG_SYSTEM_INFO("PM lock acquired: NO_LIGHT_SLEEP");
        } else {
            LOG_SYSTEM_WARN("PM lock acquire failed: NO_LIGHT_SLEEP, err=%d", static_cast<int>(err));
        }
    } else {
        LOG_SYSTEM_WARN("PM lock create failed: NO_LIGHT_SLEEP, err=%d", static_cast<int>(err));
    }

    err = esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "cpumax", &s_cpuFreqMaxLock);
    if (err == ESP_OK && s_cpuFreqMaxLock != nullptr) {
        err = esp_pm_lock_acquire(s_cpuFreqMaxLock);
        if (err == ESP_OK) {
            s_cpuFreqMaxLockHeld = true;
            LOG_SYSTEM_INFO("PM lock acquired: CPU_FREQ_MAX");
        } else {
            LOG_SYSTEM_WARN("PM lock acquire failed: CPU_FREQ_MAX, err=%d", static_cast<int>(err));
        }
    } else {
        LOG_SYSTEM_WARN("PM lock create failed: CPU_FREQ_MAX, err=%d", static_cast<int>(err));
    }
}

static void _setCpuMaxPerfMode(bool enabled) {
    if (s_cpuFreqMaxLock == nullptr) {
        return;
    }

    if (enabled && !s_cpuFreqMaxLockHeld) {
        const esp_err_t err = esp_pm_lock_acquire(s_cpuFreqMaxLock);
        if (err == ESP_OK) {
            s_cpuFreqMaxLockHeld = true;
            LOG_SYSTEM_INFO("Power mode -> PERFORMANCE");
        } else {
            LOG_SYSTEM_WARN("Failed to acquire CPU max lock, err=%d", static_cast<int>(err));
        }
    } else if (!enabled && s_cpuFreqMaxLockHeld) {
        const esp_err_t err = esp_pm_lock_release(s_cpuFreqMaxLock);
        if (err == ESP_OK) {
            s_cpuFreqMaxLockHeld = false;
            LOG_SYSTEM_INFO("Power mode -> STANDBY");
        } else {
            LOG_SYSTEM_WARN("Failed to release CPU max lock, err=%d", static_cast<int>(err));
        }
    }
}

/**
 * @brief 控制是否允许进入 light sleep。
 * @param allowed true = 释放 NO_LIGHT_SLEEP 锁（允许睡眠）； false = 持有锁（禁止睡眠）。
 */
static void _setLightSleepAllowed(bool allowed) {
    if (s_noLightSleepLock == nullptr) {
        return;
    }

    if (allowed && s_noLightSleepLockHeld) {
        const esp_err_t err = esp_pm_lock_release(s_noLightSleepLock);
        if (err == ESP_OK) {
            s_noLightSleepLockHeld = false;
            LOG_SYSTEM_INFO("Light sleep ALLOWED");
        } else {
            LOG_SYSTEM_WARN("Failed to release NO_LIGHT_SLEEP lock, err=%d", static_cast<int>(err));
        }
    } else if (!allowed && !s_noLightSleepLockHeld) {
        const esp_err_t err = esp_pm_lock_acquire(s_noLightSleepLock);
        if (err == ESP_OK) {
            s_noLightSleepLockHeld = true;
            LOG_SYSTEM_INFO("Light sleep INHIBITED");
        } else {
            LOG_SYSTEM_WARN("Failed to acquire NO_LIGHT_SLEEP lock, err=%d", static_cast<int>(err));
        }
    }
}

static void _logTaskStackIfLow(const char* taskName, TaskHandle_t handle, UBaseType_t thresholdWords) {
    if (!taskName || handle == nullptr) {
        return;
    }

    UBaseType_t highWater = uxTaskGetStackHighWaterMark(handle);
    if (highWater <= thresholdWords) {
        LOG_SYSTEM_WARN("Task stack low: %s highWater=%u words", taskName, static_cast<unsigned int>(highWater));
    }
}

static void _taskHealthMonitorTask(void* parameter) {
    (void)parameter;
    while (!shouldExitTasks) {
        _logTaskStackIfLow("_menuTask", _menuTaskHandle, 256);
        _logTaskStackIfLow("Button Scan", scanTaskHandle, 256);
        _logTaskStackIfLow("Button Handle", handleTaskHandle, 256);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    s_taskHealthMonitorHandle = nullptr;
    vTaskDelete(NULL);
}

void fatalError(const char* msg){
    LOG_SYSTEM_ERROR("Fatal Error: %s", msg);
    updateColor(CRGB::Red);  // 失败变红
    int fadeStep = 2;
    uint8_t brightness = 64;
    while (1) {
        brightness += fadeStep;
        if (brightness == 0 || brightness == 192) {
            fadeStep = -fadeStep;
        }
        updateBrightness(brightness);
        delay(5);
    };
}

void setup() {
    // 初始化日志系统
    #ifndef DEFAULT_LOG_LEVEL
    #define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO  // 默认INFO级别
    #endif
    Logger::init(DEFAULT_LOG_LEVEL);  // 使用 platformio.ini 中定义的日志级别

    LOG_SYSTEM_INFO("Wireless 1602A by Kulib");
    LOG_SYSTEM_INFO("Build Information:");
    LOG_SYSTEM_INFO("  Firmware Version: %s", PROJECT_VERSION);
    LOG_SYSTEM_INFO("  Build version: %s", BUILD_VERSION);
    LOG_SYSTEM_INFO("  Build Timestamp: %s \n", BUILD_TIMESTAMP);
    LOG_SYSTEM_INFO("Starting initialization...");

    // 检查唤醒原因
    esp_reset_reason_t resetReason = esp_reset_reason();
    LOG_SYSTEM_DEBUG("Reset reason: %d", (int)resetReason);
    if (resetReason == ESP_RST_TASK_WDT) {
        LOG_SYSTEM_WARN("Last reset reason: TASK_WDT (reason=6)");
    } else if (resetReason == ESP_RST_INT_WDT) {
        LOG_SYSTEM_WARN("Last reset reason: INT_WDT");
    }
    if (resetReason == ESP_RST_PANIC) {
        LOG_SYSTEM_WARN("Last reset was PANIC. A core dump may be available in flash.");
    }

    // 初始化睡眠管理器
    initSleepManager();

    // 检查唤醒原因
    if (isWakeupFromDeepSleep()) {
        LOG_SYSTEM_INFO("Woke up from deep sleep by BUTTON_POWER_PIN");
        initTime(true);  // 深度睡眠唤醒时初始化时间
    } else {
        initTime(false);  // 正常启动时初始化时间
        LOG_SYSTEM_INFO("Normal boot");
    }

    // 初始化 LCD
    lcdInit();
    lcdReconfigPwmForLightSleep();  // 将 LEDC timer 切换至 RC_FAST 时钟，防止 light sleep 期间背光闪烁
    
    buzzerInit();
    delay(500);
    initBQ27421(BATTERY_DESIGN_CAPACITY_MAH);  // 初始化燃料计芯片
    initOPT3001();  // 初始化光传感器 OPT3001
    
    // 初始化自动背光调节，但不立即启动任务
    initAutoBrightness();
    
    initButtonsPin();
    initrgb();

    initKanaMap();  // 初始化假名表

    // 网络模块互斥锁先初始化，避免后续多任务并发访问 WiFiClient
    initNetwork();
    WiFi.setSleep(false);
    _stabilizePowerManagement();

    startButtonTask();

    // 低频任务健康监测
    xTaskCreatePinnedToCore(_taskHealthMonitorTask, "TaskHealth", 3072, NULL, 1, &s_taskHealthMonitorHandle, 0);

    // 欢迎消息
    String ver = String(PROJECT_VERSION) + "  " +String(BUILD_VERSION);
    lcdText(ver,1);
    lcdText(BUILD_TIMESTAMP,2);

    // 挂载 SPIFFS
    if(!ConfigManager::initSPIFFS()) { 
        LOG_SYSTEM_ERROR("SPIFFS initialization failed!");
        fatalError("SPIFFS init failed"); 
    }
    
    // 初始化配置管理器
    if(!wifiConfigManager.init()){ 
        LOG_SYSTEM_ERROR("WiFi config manager initialization failed!");
        LOG_SYSTEM_ERROR("Last error: %s", wifiConfigManager.getLastErrorString(wifiConfigManager.getLastError()).c_str());
        fatalError("WiFi config init failed"); 
    }
    
    // 初始化和风天气配置管理器
    if(!qweatherAuthConfigManager.init()){ 
        LOG_SYSTEM_ERROR("QWeather config manager initialization failed!");
        LOG_SYSTEM_ERROR("Last error: %s", qweatherAuthConfigManager.getLastErrorString(qweatherAuthConfigManager.getLastError()).c_str());
        fatalError("QWeather auth config init failed"); 
    }

    // 加载并应用自动亮度开关持久化状态
    bool autoBrightnessEnabledPersisted = false;
    if (ConfigManager::loadAutoBrightnessEnabled(autoBrightnessEnabledPersisted)) {
        if (autoBrightnessEnabledPersisted) {
            enableAutoBrightness();
        } else {
            disableAutoBrightness();
        }
    }

    // 加载并应用按键音效开关持久化状态
    bool soundEffectsEnabledPersisted = true;
    if (ConfigManager::loadSoundEffectsEnabled(soundEffectsEnabledPersisted)) {
        buzzerSetUiSoundEnabled(soundEffectsEnabledPersisted);
    }

    // buzzerPlayStartup();
    loadJwtConfig();  // 初始化JWT
    wifiinit();  // 初始化WiFi配置（非阻塞，后台连接）
    ensureTimeSyncTaskRunning(); // 启动期兜底：确保自动校时任务被拉起
    initMenu();  // 初始化菜单系统（立即进入主界面）
    
    // 所有初始化完成后，按配置决定是否启动自动背光调节后台任务
    //lcdClear();
    if (isAutoBrightnessActive()) {
        startAutoBrightnessTask();
    }
    printAutoBrightnessInfo();
}

void loop(){
    // 进入深睡流程时，停止主循环对网络/显示的并发访问，避免与 enterDeepSleep 的资源回收打架
    if (shouldExitTasks) {
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // 配网模式处理劫持DNS请求
    if (inConfigMode) {
        dnsServer.processNextRequest();
        apServer.handleClient();
        // 配网完成后延迟重启（不在回调中直接调用，避免 WiFi 事件卡死）
        if (pendingRestart) {
            delay(500);
            ESP.restart();
        }
    }

    if (powerKeyOverlayActive) {
        vTaskDelay(pdMS_TO_TICKS(20));
        return;
    }

    // WiFi 状态自愈：在少数情况下可能出现“已拿到 IP 但 WiFi.status() 尚未更新为 WL_CONNECTED”的瞬间
    // 这里以 localIP!=0.0.0.0 作为更可靠的“已联通”信号，避免误判触发重连并把状态错误置为 DISCONNECTED
    const unsigned long nowMs = millis();
    const bool hasIp = (WiFi.localIP() != IPAddress(0, 0, 0, 0));
    const bool isStaConnected = (WiFi.status() == WL_CONNECTED);
    static unsigned long lastWiFiReconnectAttemptMs = 0;
    static bool tcpServerReady = false;
    static bool lowPowerListenWindowOpen = false;
    static unsigned long lowPowerListenWindowStartMs = 0;
    static unsigned long lowPowerListenCycleStartMs = 0;
    static const unsigned long kLowPowerListenCycleMs = STANDBY_LISTEN_CYCLE_MS; // 周期唤醒监听，降低待机射频占空比
    static const unsigned long kLowPowerListenWindowMs = STANDBY_LISTEN_WINDOW_MS;  // 监听窗口，平衡连接时延与待机功耗

    static bool wifiEverConnected = false;
    if (isStaConnected || hasIp) {
        wifiConnectionState = WIFI_CONNECTED;
        wifiEverConnected = true;
        ensureTimeSyncTaskRunning();
    } else {
        tcpServerReady = false;
        lowPowerListenWindowOpen = false;
        s_lowTxPowerMode = false;
    }

    if (wifiConnectionState == WIFI_CONNECTED && !isStaConnected && !hasIp) {
        if (nowMs - lastWiFiReconnectAttemptMs < 3000) {
            vTaskDelay(pdMS_TO_TICKS(5));
            return;
        }
        lastWiFiReconnectAttemptMs = nowMs;
        LOG_SYSTEM_WARN("trying to reconnect WiFi...");
        wifiConnectionState = WIFI_DISCONNECTED;
        connectToWiFi();  // 尝试重新连接WiFi
    }

    // 连接失败后自愈重连：仅在曾经成功连接过后断开时重试。
    // 开机首次连接失败（20s超时）后不再无限重试，避免频繁扰动。
    if (!isStaConnected && !hasIp
        && !inConfigMode
        && wifiEverConnected
        && (wifiConnectionState == WIFI_DISCONNECTED || wifiConnectionState == WIFI_FAILED || wifiConnectionState == WIFI_IDLE)) {
        if (nowMs - lastWiFiReconnectAttemptMs >= 5000) {
            lastWiFiReconnectAttemptMs = nowMs;
            LOG_SYSTEM_WARN("WiFi offline (state=%d), retry connecting...", static_cast<int>(wifiConnectionState));
            connectToWiFi();
        }
    }

    static unsigned long lastAcceptPollMs = 0;
    enum class RuntimePowerMode : uint8_t { Standby = 0, ConnectedBalanced = 1, Performance = 2 };
    static RuntimePowerMode runtimePowerMode = RuntimePowerMode::Performance;
    if (!clientConnected && !frameCache.empty()) {
        frameCache.clear();
    }
    const bool hasStreamWork = clientConnected || !frameCache.empty();
    const bool inWirelessScreen = (!inMenuMode
        && !inConfigMode
        && wirelessScreenActive);
    const bool inNonWirelessAppInterface = (!inConfigMode
        && (isInSubInterface() || (!inMenuMode && !wirelessScreenActive)));
    const bool appInterfaceNeedsNetwork = inNonWirelessAppInterface && isAppInterfaceNetworkRequired();
    const bool appInterfaceGateActive = APP_INTERFACE_RF_GATE_ENABLE
        && inNonWirelessAppInterface
        && !appInterfaceNeedsNetwork;

    const bool wirelessScreenDisconnectedIdle = inWirelessScreen && !hasStreamWork;

        const bool timeSyncAllowsStandby = true;

    // 无线推流界面处于“等待客户端”时仍应保持高可连接性，不进入待机低功耗。
    const bool lowActivityIdle = inMenuMode
        && !inConfigMode
        && !hasStreamWork
        && wifiConnectionState == WIFI_CONNECTED
        && timeSyncAllowsStandby;

    RuntimePowerMode desiredPowerMode = RuntimePowerMode::Performance;
    if (appInterfaceGateActive || lowActivityIdle) {
        desiredPowerMode = RuntimePowerMode::Standby;
    } else if ((STREAM_GLOBAL_LOW_POWER && clientConnected) || appInterfaceNeedsNetwork) {
        desiredPowerMode = RuntimePowerMode::ConnectedBalanced;
    }

    if (desiredPowerMode != runtimePowerMode) {
        runtimePowerMode = desiredPowerMode;
        if (runtimePowerMode == RuntimePowerMode::Standby) {
            // 进入待机时立即关闭监听，避免窗口外持续射频监听。
            lowPowerListenWindowOpen = false;
#if STANDBY_LISTEN_WINDOW_ENABLE
            if (tcpServerReady) {
                server.end();
                tcpServerReady = false;
            }
#endif
            lowPowerListenCycleStartMs = nowMs;
            _setCpuMaxPerfMode(false);
            _setCpuTargetFreqMhz(80);
            _setLightSleepAllowed(true);  // 释放 NO_LIGHT_SLEEP 锁，允许 vTaskDelay 期间自动进入 light sleep
            _setWiFiRxPowerSave(2);
            _setWiFiTxPowerMode(true);
            LOG_SYSTEM_INFO("Power profile -> STANDBY");
        } else if (runtimePowerMode == RuntimePowerMode::ConnectedBalanced) {
            _setLightSleepAllowed(false); // 推流中保持低延迟，不进入 light sleep
            _setCpuMaxPerfMode(false);
            _setCpuTargetFreqMhz(STREAM_CONNECTED_CPU_MHZ);
#if STREAM_CONNECTED_ALWAYS_POWER_SAVE
            _setWiFiRxPowerSave((STREAM_CONNECTED_WIFI_PS_LEVEL >= 2) ? 2 : 1);
#else
            _setWiFiRxPowerSave(0);
#endif
            _setWiFiTxPowerMode(STREAM_CONNECTED_LOW_TX_POWER != 0);
            LOG_SYSTEM_INFO("Power profile -> CONNECTED_BALANCED");
        } else {
            lowPowerListenWindowOpen = false;
            _setLightSleepAllowed(false);
            _setWiFiRxPowerSave(0);
            _setCpuMaxPerfMode(true);
            _setCpuTargetFreqMhz(240);
            _setWiFiTxPowerMode(false);
            LOG_SYSTEM_INFO("Power profile -> PERFORMANCE");
        }
    }

    // 监听策略：非无线应用界面默认关闭监听；其余场景按现有策略管理监听窗口。
    if (isStaConnected || hasIp) {
        if (appInterfaceGateActive || inNonWirelessAppInterface) {
            lowPowerListenWindowOpen = false;
            if (tcpServerReady) {
                server.end();
                tcpServerReady = false;
                LOG_SYSTEM_INFO("TCP server paused in app interface");
            }
            if (appInterfaceGateActive) {
                _setWiFiRxPowerSave(2);
                _setWiFiTxPowerMode(true);
            }
        } else if (!lowActivityIdle || hasStreamWork) {
            lowPowerListenWindowOpen = false;
            if (!tcpServerReady) {
                server.begin();
                tcpServerReady = true;
                LOG_SYSTEM_INFO("TCP server ensured on port %d", CONNECT_PORT);
            }
        } else {
#if !STANDBY_LISTEN_WINDOW_ENABLE
            // 关闭监听窗口策略：待机期间保持常驻监听。
            lowPowerListenWindowOpen = false;
            _setWiFiRxPowerSave(0);
            if (!tcpServerReady) {
                server.begin();
                tcpServerReady = true;
                LOG_SYSTEM_INFO("TCP server always listening in standby on port %d", CONNECT_PORT);
            }
#else
            // 待机窗口外强制关闭监听，确保射频不常驻高活跃态。
            if (!lowPowerListenWindowOpen && tcpServerReady) {
                server.end();
                tcpServerReady = false;
                if (runtimePowerMode == RuntimePowerMode::Standby) {
                    _setWiFiRxPowerSave(2);
                }
            }

            const bool shouldOpenWindow = !lowPowerListenWindowOpen
                && (nowMs - lowPowerListenCycleStartMs >= kLowPowerListenCycleMs);

            if (shouldOpenWindow) {
                lowPowerListenCycleStartMs = nowMs;
                lowPowerListenWindowStartMs = nowMs;
                lowPowerListenWindowOpen = true;
                // 唤醒 WiFi：确保握手期间 WiFi 全功率运行，避免 sleep 模式导致 RST
                _setWiFiRxPowerSave(0);

                if (!tcpServerReady) {
                    server.begin();
                    tcpServerReady = true;
                }
            }

            if (lowPowerListenWindowOpen
                && !clientConnected
                && (nowMs - lowPowerListenWindowStartMs >= kLowPowerListenWindowMs)) {
                // 关窗前最后一次 accept，防止已完成握手但尚未消费的连接被 server.end() RST
                acceptClientIfNew();
                if (!clientConnected) {
                    server.end();
                    tcpServerReady = false;
                    lowPowerListenWindowOpen = false;
                    // 无客户端连入，恢复 WiFi sleep
                    if (runtimePowerMode == RuntimePowerMode::Standby) {
                        _setWiFiRxPowerSave(2);
                    }
                }
            }
#endif
        }
    }

    #if ENABLE_DYNAMIC_CPU_FREQ
    static unsigned long lastBusyMs = 0;
    static int currentCpuFreqMHz = 240;
    const bool interactiveBusy = inConfigMode || hasStreamWork || (!inMenuMode && !wirelessScreenDisconnectedIdle);
    if (interactiveBusy) {
        lastBusyMs = nowMs;
    }

    // 纯菜单待机持续一段时间后降频，减少发热；有交互/流量时立即恢复。
    const bool allowLowPower = (inMenuMode || wirelessScreenDisconnectedIdle)
        && !inConfigMode
        && !hasStreamWork
        && (nowMs - lastBusyMs >= 3000);
    const int targetCpuFreqMHz = allowLowPower ? 160 : 240;
    if (targetCpuFreqMHz != currentCpuFreqMHz) {
        if (setCpuFrequencyMhz(targetCpuFreqMHz)) {
            currentCpuFreqMHz = targetCpuFreqMHz;
            LOG_SYSTEM_INFO("CPU frequency -> %d MHz", currentCpuFreqMHz);
        }
    }
    #endif

    // 低活跃时仅在监听窗口内轮询 accept，进一步降低空转占用。
    const bool allowAcceptPolling = hasStreamWork || !lowActivityIdle || lowPowerListenWindowOpen;
    const unsigned long acceptPollIntervalMs = lowPowerListenWindowOpen ? 80 : (lowActivityIdle ? STANDBY_ACCEPT_POLL_INTERVAL_MS : 120);
    if (allowAcceptPolling && (hasStreamWork || (nowMs - lastAcceptPollMs >= acceptPollIntervalMs))) {
        acceptClientIfNew();
        lastAcceptPollMs = nowMs;
    }

    if (hasStreamWork) {
        receiveClientData();
        tryDisplayCachedFrames();
        const bool stillBusy = clientConnected || !frameCache.empty();
        // 流模式下按队列深度自适应轮询：有积压时更积极消费，空闲时降低唤醒频率。
        const size_t qsize = frameCache.size();

        if (runtimePowerMode == RuntimePowerMode::ConnectedBalanced) {
#if STREAM_CONNECTED_ALWAYS_POWER_SAVE
            _setWiFiRxPowerSave((STREAM_CONNECTED_WIFI_PS_LEVEL >= 2) ? 2 : 1);
#else
            _setWiFiRxPowerSave(0);
#endif
        }

        const uint32_t streamDelayMs =
            (qsize >= 3) ? 3 :
            (qsize >= 2) ? 4 :
            (qsize >= 1) ? 6 :
            (stillBusy ? 10 : 20);
        vTaskDelay(pdMS_TO_TICKS(streamDelayMs));
    } else {
        // 主菜单和“无线界面断联空闲”都走低负载节奏，仅保留接入新连接。
        vTaskDelay(pdMS_TO_TICKS((inMenuMode || wirelessScreenDisconnectedIdle) ? 80 : 40));
    }
}

static void _setWiFiTxPowerMode(bool lowPower) {
#if defined(WIFI_POWER_8_5dBm) && defined(WIFI_POWER_19_5dBm)
    if (lowPower == s_lowTxPowerMode) {
        return;
    }

    wifi_power_t target = WIFI_POWER_19_5dBm;
    if (lowPower) {
#if STREAM_CONNECTED_ULTRA_LOW_TX_POWER
    #if defined(WIFI_POWER_2dBm)
        target = WIFI_POWER_2dBm;
    #elif defined(WIFI_POWER_5dBm)
        target = WIFI_POWER_5dBm;
    #elif defined(WIFI_POWER_7dBm)
        target = WIFI_POWER_7dBm;
    #else
        target = WIFI_POWER_8_5dBm;
    #endif
#else
        target = WIFI_POWER_8_5dBm;
#endif
    }

    if (WiFi.setTxPower(target)) {
        s_lowTxPowerMode = lowPower;
        LOG_SYSTEM_INFO("WiFi TX power -> %s", lowPower ? "LOW" : "HIGH");
    } else {
        LOG_SYSTEM_WARN("Failed to set WiFi TX power mode: %s", lowPower ? "LOW" : "HIGH");
    }
#else
    (void)lowPower;
#endif
}