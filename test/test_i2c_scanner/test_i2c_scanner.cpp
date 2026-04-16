#include <Arduino.h>
#include <Wire.h>
#include "../../include/utils/logger.h"
#include "../../include/mydefine.h"
#include "../common/test_init.h"

/**
 * @brief I2C 地址扫描程序
 * 扫描 0x03-0x77 地址范围，列出总线上所有响应的设备
 */

void performI2CScan() {
    Serial.println("\n========== I2C Address Scanner ==========");
    Serial.println("Scanning I2C addresses 0x03 to 0x77...\n");
    
    uint8_t foundCount = 0;
    uint8_t foundAddresses[128];
    
    // 扫描地址 0x03 到 0x77 (0x00-0x02 和 0x78-0x7F 都是保留地址)
    for (uint8_t address = 0x03; address < 0x78; address++) {
        // 尝试与该地址通信
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            // 设备应答，记录该地址
            foundAddresses[foundCount] = address;
            foundCount++;
            
            // 输出到串口和日志
            Serial.printf("[I2C] Device found at address: 0x%02X (%d)\n", address, address);
            LOG_SYSTEM_INFO("I2C Device found at address: 0x%02X (%d)", address, address);
        } else if (error == 4) {
            // 其他错误（不常见）
            Serial.printf("[I2C] Unknown error at address 0x%02X\n", address);
        }
    }
    
    // 输出扫描结果统计
    Serial.println("\n========== Scan Results ==========");
    Serial.printf("Total devices found: %d\n", foundCount);
    
    if (foundCount == 0) {
        Serial.println("No devices found on I2C bus!");
        LOG_SYSTEM_WARN("No devices found on I2C bus!");
    } else {
        Serial.println("\nDevices found at addresses:");
        for (uint8_t i = 0; i < foundCount; i++) {
            uint8_t addr = foundAddresses[i];
            String deviceName = "Unknown";
            
            // 根据已知地址识别设备
            if (addr == 0x55) {
                deviceName = "BQ27421 (Fuel Gauge)";
            } else if (addr == 0x44) {
                deviceName = "OPT3001 (Light Sensor)";
            }
            
            Serial.printf("  [%d] 0x%02X - %s\n", i + 1, addr, deviceName.c_str());
            LOG_SYSTEM_INFO("  [%d] 0x%02X - %s", i + 1, addr, deviceName.c_str());
        }
    }
    
    Serial.println("=====================================\n");
}

void setup() {
    // 初始化日志和 LCD
    Logger::init(LOG_LEVEL_VERBOSE);
    lcdInit();
    lcdText("I2C Scanner", 1);
    lcdText("Scanning...", 2);
    
    delay(1000);
    
    LOG_SYSTEM_INFO("===== I2C Address Scanner Started =====");
    LOG_SYSTEM_INFO("SDA Pin: %d, SCL Pin: %d", SDA_PIN, SCL_PIN);
    LOG_SYSTEM_INFO("I2C Frequency: 100000 Hz");
    
    // 初始化 I2C 总线
    Wire.begin(SDA_PIN, SCL_PIN, 100000);  // 100kHz 频率
    delay(100);
    
    // 执行扫描
    performI2CScan();
    
    // 显示扫描完成
    lcdText("Scan Complete", 1);
    lcdText("Check Serial", 2);
    
    LOG_SYSTEM_INFO("===== I2C Address Scanner Finished =====");
}

void loop() {
    // 每 5 秒重新扫描一次
    delay(5000);
    
    Serial.println("\n[Periodic Scan]");
    performI2CScan();
}
