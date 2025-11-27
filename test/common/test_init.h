#ifndef TEST_INIT_H
#define TEST_INIT_H

#include <Arduino.h>
#include <SPIFFS.h>
#include "logger.h"
#include "lcd_driver.h"

/**
 * @brief 全局测试初始化 - 在所有测试开始前调用一次
 */
inline void globalTestSetup() {
    // 初始化日志系统
    lcdInit();
    initKanaMap();
    lcdText("Initializing...",1);
    delay(2000); // 等待串口稳定
    Logger::init(LOG_LEVEL_VERBOSE);
    LOG_SYSTEM_INFO("  _____ _____ ____ _____   __  __  ___  ____  _____ ");
    LOG_SYSTEM_INFO(" |_   _| ____/ ___|_   _| |  \\/  |/ _ \\|  _ \\| ____|");
    LOG_SYSTEM_INFO("   | | |  _| \\___ \\ | |   | |\\/| | | | | | | |  _|  ");
    LOG_SYSTEM_INFO("   | | | |___ ___) || |   | |  | | |_| | |_| | |___ ");
    LOG_SYSTEM_INFO("   |_| |_____|____/ |_|   |_|  |_|\\___/|____/|_____|");
    
    // 初始化 SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed, formatting...");
        SPIFFS.format();
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS mount failed after format!");
        }
    }
    
    LOG_SYSTEM_INFO("=== Test Environment Initialized ===");
}

/**
 * @brief 全局测试清理 - 在所有测试结束后调用一次
 */
inline void globalTestTeardown() {
    LOG_SYSTEM_INFO(" _____ _____ ____ _____   _____ _   _ ____  ");
    LOG_SYSTEM_INFO("|_   _| ____/ ___|_   _| | ____| \\ | |  _ \\ ");
    LOG_SYSTEM_INFO("  | | |  _| \\___ \\ | |   |  _| |  \\| | | | |");
    LOG_SYSTEM_INFO("  | | | |___ ___) || |   | |___| |\\  | |_| |");
    LOG_SYSTEM_INFO("  |_| |_____|____/ |_|   |_____|_| \\_|____/ ");
                                             
    LOG_SYSTEM_INFO("=== Test Environment Cleaned ===");
}

/**
 * @brief 每个测试用例前的初始化
 */
inline void perTestSetup() {
    // 确保 SPIFFS 可用
    if (!SPIFFS.begin()) {
        SPIFFS.begin(true);
    }
}

/**
 * @brief 每个测试用例后的清理
 */
inline void perTestTeardown() {
    // 清理本次测试产生的临时文件
}

#endif // TEST_INIT_H
