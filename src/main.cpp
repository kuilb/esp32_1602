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

WifiConfigManager wifiConfigManager("/wifi_config.txt");
QWeatherAuthConfigManager qweatherAuthConfigManager("/qweather_auth_config.txt");

static TaskHandle_t s_taskHealthMonitorHandle = nullptr;

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
    
    buzzerInit();
    initBQ27421(BATTERY_DESIGN_CAPACITY_MAH);  // 初始化燃料计芯片
    // 初始化光传感器 OPT3001
    initOPT3001();
    
    // 初始化自动背光调节（会自动启用），但不立即启动任务
    initAutoBrightness();
    
    initButtonsPin();
    initrgb();

    initKanaMap();  // 初始化假名表
    lcdInit();

    // 网络模块互斥锁先初始化，避免后续多任务并发访问 WiFiClient
    initNetwork();

    startButtonTask();

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

    // buzzerPlayStartup();
    delay(300);
    lcdClear();
    loadJwtConfig();  // 初始化JWT
    wifiinit();  // 初始化WiFi配置（非阻塞，后台连接）
    initMenu();  // 初始化菜单系统（立即进入主界面）
    
    // 所有初始化完成后，启动自动背光调节后台任务（避免与WiFi初始化冲突）
    delay(500);  // 等待WiFi初始化稳定
    startAutoBrightnessTask();
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

    // WiFi 状态自愈：在少数情况下可能出现“已拿到 IP 但 WiFi.status() 尚未更新为 WL_CONNECTED”的瞬间
    // 这里以 localIP!=0.0.0.0 作为更可靠的“已联通”信号，避免误判触发重连并把状态错误置为 DISCONNECTED
    const bool hasIp = (WiFi.localIP() != IPAddress(0, 0, 0, 0));
    const bool isStaConnected = (WiFi.status() == WL_CONNECTED);
    static unsigned long lastWiFiReconnectAttemptMs = 0;

    if (isStaConnected || hasIp) {
        wifiConnectionState = WIFI_CONNECTED;
    }

    if (wifiConnectionState == WIFI_CONNECTED && !isStaConnected && !hasIp) {
        const unsigned long nowMs = millis();
        if (nowMs - lastWiFiReconnectAttemptMs < 3000) {
            vTaskDelay(pdMS_TO_TICKS(5));
            return;
        }
        lastWiFiReconnectAttemptMs = nowMs;
        LOG_SYSTEM_WARN("trying to reconnect WiFi...");
        wifiConnectionState = WIFI_DISCONNECTED;
        connectToWiFi();  // 尝试重新连接WiFi
    }
    
    acceptClientIfNew();
    receiveClientData();
    tryDisplayCachedFrames();
    
    vTaskDelay(pdMS_TO_TICKS(5));  // 主循环降频，降低空转与抢占压力
}