#include "myheader.h"
#include "mydefine.h"
#include "kanamap.h"
#include "lcd_driver.h"
#include "wifi_config.h"
#include "rgb_led.h"
#include "network.h"
#include "protocol.h"
#include "playbuffer.h"
#include "button.h"
#include "menu.h"
#include "clock.h"
#include "jwt_auth.h"
#include "utils/logger.h"
#include "config_manager.h"
#include "wifi_config_manager.h"
#include "qweather_auth_config_manager.h"
#include <DNSServer.h>
using namespace std;

WifiConfigManager wifiConfigManager("/wifi_config.txt");
QWeatherAuthConfigManager qweatherAuthConfigManager("/qweather_auth_config.txt");

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
    // 首先初始化日志系统
    #ifndef DEFAULT_LOG_LEVEL
    #define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO  // 默认INFO级别
    #endif
    Logger::init(DEFAULT_LOG_LEVEL);  // 使用 platformio.ini 中定义的日志级别
    
    initButtonsPin();
    initrgb();
    initKanaMap();  // 初始化假名表
    lcdInit();
    startButtonTask();

    // 欢迎消息
    String ver = String(PROJECT_VERSION) + "  " +String(BUILD_VERSION);
    lcdText(ver,1);
    lcdText(BUILD_TIMESTAMP,2);

    LOG_SYSTEM_INFO("Wireless 1602A by Kulib");
    LOG_SYSTEM_INFO("Build Information:");
    LOG_SYSTEM_INFO("  Firmware Version: %s", PROJECT_VERSION);
    LOG_SYSTEM_INFO("  Build version: %s", BUILD_VERSION);
    LOG_SYSTEM_INFO("  Build Timestamp: %s \n", BUILD_TIMESTAMP);
    LOG_SYSTEM_INFO("开始初始化...");

    // 挂载 SPIFFS
    if(!ConfigManager::initSPIFFS()) { fatalError("SPIFFS 初始化失败"); }
    if(!wifiConfigManager.init()){ fatalError("WiFi 配置初始化失败"); }
    if(!qweatherAuthConfigManager.init()){ fatalError("QWeather 认证配置初始化失败"); }

    delay(300);
    loadJwtConfig();  // 初始化JWT
    wifiinit();  // 初始化WiFi配置（非阻塞，后台连接）
    initMenu();  // 初始化菜单系统（立即进入主界面）
}

void loop(){
    if (inConfigMode) {
        dnsServer.processNextRequest();  // 处理劫持DNS请求
        apServer.handleClient();
    }

    if(wifiConnectionState == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED) {
        LOG_SYSTEM_WARN("trying to reconnect WiFi...");
        wifiConnectionState = WIFI_DISCONNECTED;
        connectToWiFi();  // 尝试重新连接WiFi
    }
    
    acceptClientIfNew();
    receiveClientData();
    tryDisplayCachedFrames();
    
    vTaskDelay(1);  // 1ms延时节流
}