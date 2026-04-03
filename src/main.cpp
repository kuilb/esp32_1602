#include "mydefine.h"

#include <esp32-hal-cpu.h>

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
#include "esp_freertos_hooks.h"

#ifndef ENABLE_CPU_USAGE_MONITOR
#define ENABLE_CPU_USAGE_MONITOR 0
#endif

#ifndef ENABLE_DYNAMIC_CPU_FREQ
#define ENABLE_DYNAMIC_CPU_FREQ 0
#endif

WifiConfigManager wifiConfigManager("/wifi_config.txt");
QWeatherAuthConfigManager qweatherAuthConfigManager("/qweather_auth_config.txt");

static TaskHandle_t s_taskHealthMonitorHandle = nullptr;
static TaskHandle_t s_cpuUsageMonitorTaskHandle = nullptr;
static esp_pm_lock_handle_t s_noLightSleepLock = nullptr;
static esp_pm_lock_handle_t s_cpuFreqMaxLock = nullptr;
static bool s_cpuFreqMaxLockHeld = false;
static bool s_lowTxPowerMode = false;

static void _setWiFiTxPowerMode(bool lowPower);

static void _stabilizePowerManagement() {
    esp_err_t err = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "nols", &s_noLightSleepLock);
    if (err == ESP_OK && s_noLightSleepLock != nullptr) {
        err = esp_pm_lock_acquire(s_noLightSleepLock);
        if (err == ESP_OK) {
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

#if ENABLE_CPU_USAGE_MONITOR
static volatile uint32_t s_idleCounterCore0 = 0;
static volatile uint32_t s_idleCounterCore1 = 0;

static bool IRAM_ATTR _idleHookCore0(void) {
    s_idleCounterCore0++;
    return false;
}

static bool IRAM_ATTR _idleHookCore1(void) {
    s_idleCounterCore1++;
    return false;
}

static void _cpuUsageMonitorTask(void* parameter) {
    (void)parameter;

    uint32_t lastIdle0 = s_idleCounterCore0;
    uint32_t lastIdle1 = s_idleCounterCore1;
    uint32_t maxIdleDelta0 = 1;
    uint32_t maxIdleDelta1 = 1;

    while (!shouldExitTasks) {
        vTaskDelay(pdMS_TO_TICKS(2000));

        const uint32_t nowIdle0 = s_idleCounterCore0;
        const uint32_t nowIdle1 = s_idleCounterCore1;

        const uint32_t delta0 = nowIdle0 - lastIdle0;
        const uint32_t delta1 = nowIdle1 - lastIdle1;

        lastIdle0 = nowIdle0;
        lastIdle1 = nowIdle1;

        if (delta0 > maxIdleDelta0) maxIdleDelta0 = delta0;
        if (delta1 > maxIdleDelta1) maxIdleDelta1 = delta1;

        float usage0 = 100.0f * (1.0f - (float)delta0 / (float)maxIdleDelta0);
        float usage1 = 100.0f * (1.0f - (float)delta1 / (float)maxIdleDelta1);

        if (usage0 < 0.0f) usage0 = 0.0f;
        if (usage0 > 100.0f) usage0 = 100.0f;
        if (usage1 < 0.0f) usage1 = 0.0f;
        if (usage1 > 100.0f) usage1 = 100.0f;

        LOG_SYSTEM_INFO("CPU usage: Core0=%.1f%% Core1=%.1f%%", usage0, usage1);
    }

    s_cpuUsageMonitorTaskHandle = nullptr;
    vTaskDelete(NULL);
}
#endif

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

    // 在调试构建下限制高频模块日志，避免串口输出风暴触发中断看门狗。
    if (DEFAULT_LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        Logger::setModuleLevel(LOG_MODULE_NETWORK, LOG_LEVEL_INFO);
        Logger::setModuleLevel(LOG_MODULE_WIFI, LOG_LEVEL_INFO);
        Logger::setModuleLevel(LOG_MODULE_DISPLAY, LOG_LEVEL_INFO);
        Logger::setModuleLevel(LOG_MODULE_BATTERY, LOG_LEVEL_WARN);
        Logger::setModuleLevel(LOG_MODULE_ALS, LOG_LEVEL_WARN);
    }

    LOG_SYSTEM_INFO("Wireless 1602A by Kulib");
    LOG_SYSTEM_INFO("Build Information:");
    LOG_SYSTEM_INFO("  Firmware Version: %s", PROJECT_VERSION);
    LOG_SYSTEM_INFO("  Build version: %s", BUILD_VERSION);
    LOG_SYSTEM_INFO("  Build Timestamp: %s \n", BUILD_TIMESTAMP);
    LOG_SYSTEM_INFO("Starting initialization...");

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

    // 尽早初始化 LCD，启动阶段立即关闭光标/闪烁，避免上电后长时间可见闪烁光标。
    lcdInit();
    
    buzzerInit();
    initBQ27421(BATTERY_DESIGN_CAPACITY_MAH);  // 初始化燃料计芯片
    // 初始化光传感器 OPT3001
    initOPT3001();
    
    // 初始化自动背光调节（会自动启用），但不立即启动任务
    initAutoBrightness();
    
    initButtonsPin();
    initrgb();

    initKanaMap();  // 初始化假名表

    // 网络模块互斥锁先初始化，避免后续多任务并发访问 WiFiClient
    initNetwork();
    WiFi.setSleep(false);
    _stabilizePowerManagement();

    startButtonTask();

    #if ENABLE_CPU_USAGE_MONITOR
    // 注册空闲钩子，用于统计双核空闲占比并换算 CPU 占用。
    esp_register_freertos_idle_hook_for_cpu(_idleHookCore0, 0);
    esp_register_freertos_idle_hook_for_cpu(_idleHookCore1, 1);

    // CPU 占用监控：每 2 秒输出一次到串口日志。
    xTaskCreatePinnedToCore(_cpuUsageMonitorTask, "CpuUsage", 3072, NULL, 1, &s_cpuUsageMonitorTaskHandle, 0);
    #else
    LOG_SYSTEM_INFO("CPU usage monitor disabled (ENABLE_CPU_USAGE_MONITOR=0)");
    #endif

    // 低频任务健康监测：提前发现栈水位过低，便于排查WDT/栈破坏。
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
    
    if(!wifiConfigManager.init()){ 
        LOG_SYSTEM_ERROR("WiFi config manager initialization failed!");
        LOG_SYSTEM_ERROR("Last error: %s", wifiConfigManager.getLastErrorString(wifiConfigManager.getLastError()).c_str());
        fatalError("WiFi config init failed"); 
    }
    
    if(!qweatherAuthConfigManager.init()){ 
        LOG_SYSTEM_ERROR("QWeather config manager initialization failed!");
        LOG_SYSTEM_ERROR("Last error: %s", qweatherAuthConfigManager.getLastErrorString(qweatherAuthConfigManager.getLastError()).c_str());
        fatalError("QWeather auth config init failed"); 
    }

    // 加载并应用自动亮度开关持久化状态（若配置不存在则保留默认行为）。
    bool autoBrightnessEnabledPersisted = false;
    if (ConfigManager::loadAutoBrightnessEnabled(autoBrightnessEnabledPersisted)) {
        if (autoBrightnessEnabledPersisted) {
            enableAutoBrightness();
        } else {
            disableAutoBrightness();
        }
    }

    // 加载并应用按键音效开关持久化状态（若配置不存在则保留默认开启）。
    bool soundEffectsEnabledPersisted = true;
    if (ConfigManager::loadSoundEffectsEnabled(soundEffectsEnabledPersisted)) {
        buzzerSetUiSoundEnabled(soundEffectsEnabledPersisted);
    }

    // buzzerPlayStartup();
    delay(300);
    lcdClear();
    loadJwtConfig();  // 初始化JWT
    wifiinit();  // 初始化WiFi配置（非阻塞，后台连接）
    ensureTimeSyncTaskRunning(); // 启动期兜底：确保自动校时任务被拉起
    initMenu();  // 初始化菜单系统（立即进入主界面）
    
    // 所有初始化完成后，按配置决定是否启动自动背光调节后台任务
    delay(500);  // 等待WiFi初始化稳定
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

    if (inConfigMode) {
        dnsServer.processNextRequest();  // 处理劫持DNS请求
        apServer.handleClient();
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
    static const unsigned long kLowPowerListenCycleMs = 2200;
    static const unsigned long kLowPowerListenWindowMs = 350;

    if (isStaConnected || hasIp) {
        wifiConnectionState = WIFI_CONNECTED;
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

    // 连接失败后持续自愈：避免 WIFI_FAILED 后不再重连，导致上位机长期无法连接。
    if (!isStaConnected && !hasIp
        && !inConfigMode
        && (wifiConnectionState == WIFI_DISCONNECTED || wifiConnectionState == WIFI_FAILED || wifiConnectionState == WIFI_IDLE)) {
        if (nowMs - lastWiFiReconnectAttemptMs >= 5000) {
            lastWiFiReconnectAttemptMs = nowMs;
            LOG_SYSTEM_WARN("WiFi offline (state=%d), retry connecting...", static_cast<int>(wifiConnectionState));
            connectToWiFi();
        }
    }

    static unsigned long lastAcceptPollMs = 0;
    static bool standbyPowerMode = false;
    if (!clientConnected && !frameCache.empty()) {
        frameCache.clear();
    }
    const bool hasStreamWork = clientConnected || !frameCache.empty();
    const bool wirelessScreenDisconnectedIdle = (!inMenuMode
        && currentState == STATE_MENU
        && !inConfigMode
        && !hasStreamWork);

    const bool lowActivityIdle = (inMenuMode || wirelessScreenDisconnectedIdle)
        && !inConfigMode
        && !hasStreamWork
        && wifiConnectionState == WIFI_CONNECTED
        && timeSyncState != TIME_SYNC_IN_PROGRESS;

    if (lowActivityIdle && !standbyPowerMode) {
        standbyPowerMode = true;
        _setCpuMaxPerfMode(false);
        WiFi.setSleep(true);
        _setWiFiTxPowerMode(true);
    } else if (!lowActivityIdle && standbyPowerMode) {
        standbyPowerMode = false;
        WiFi.setSleep(false);
        _setCpuMaxPerfMode(true);
        _setWiFiTxPowerMode(false);
    }

    // 监听策略：低活跃时不常驻监听，按周期短窗口拉起 server；活跃时常驻监听。
    if (isStaConnected || hasIp) {
        if (!lowActivityIdle || hasStreamWork) {
            lowPowerListenWindowOpen = false;
            if (!tcpServerReady) {
                server.begin();
                tcpServerReady = true;
                LOG_SYSTEM_INFO("TCP server ensured on port %d", CONNECT_PORT);
            }
        } else {
            const bool shouldOpenWindow = !lowPowerListenWindowOpen
                && (nowMs - lowPowerListenCycleStartMs >= kLowPowerListenCycleMs);

            if (shouldOpenWindow) {
                lowPowerListenCycleStartMs = nowMs;
                lowPowerListenWindowStartMs = nowMs;
                lowPowerListenWindowOpen = true;

                if (!tcpServerReady) {
                    server.begin();
                    tcpServerReady = true;
                }
            }

            if (lowPowerListenWindowOpen
                && !clientConnected
                && (nowMs - lowPowerListenWindowStartMs >= kLowPowerListenWindowMs)) {
                server.end();
                tcpServerReady = false;
                lowPowerListenWindowOpen = false;
            }
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
    const unsigned long acceptPollIntervalMs = lowPowerListenWindowOpen ? 80 : ((wirelessScreenDisconnectedIdle || lowActivityIdle) ? 300 : 200);
    if (allowAcceptPolling && (hasStreamWork || (nowMs - lastAcceptPollMs >= acceptPollIntervalMs))) {
        acceptClientIfNew();
        lastAcceptPollMs = nowMs;
    }

    if (hasStreamWork) {
        receiveClientData();
        tryDisplayCachedFrames();
        const bool stillBusy = clientConnected || !frameCache.empty();
        // 略放缓流模式轮询，减少空转唤醒频率，平衡温升与延迟。
        vTaskDelay(pdMS_TO_TICKS(stillBusy ? 12 : 20));
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

    const wifi_power_t target = lowPower ? WIFI_POWER_8_5dBm : WIFI_POWER_19_5dBm;
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