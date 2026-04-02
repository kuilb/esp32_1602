#include <unity.h>
#include <Arduino.h>
#include "../../include/hardware/lcd_driver.h"
#include "../../include/hardware/fuel_gauge.h"
#include "../common/test_init.h"  // 引入共享的测试初始化
#include "../../include/utils/logger.h"

// 永远成功的测试用例
void alwaysPassTest() {
    TEST_ASSERT_TRUE(true);  // 断言 true，总是成功
}

void readBatteryInfo() {
    // ==================== 基础测量 ====================
    uint16_t voltage = readVoltage();
    int16_t current = readAverageCurrent();
    uint16_t batteryTemp = readBatteryTemperature();
    uint16_t internalTemp = readInternalTemperature();
    int16_t standbyCurrent = readStandbyCurrent();
    int16_t maxLoadCurrent = readMaxLoadCurrent();
    int16_t avgPower = readAveragePower();
    
    // ==================== 容量与电量 ====================
    uint8_t soc = readStateOfCharge();
    uint16_t remainingCap = readRemainingCapacity();
    uint16_t fullChargeCap = readFullChargeCapacity();
    uint16_t nominalAvailCap = readNominalAvailableCapacity();
    uint16_t fullAvailCap = readFullAvailableCapacity();
    
    // 滤波/未滤波数据
    uint16_t remCapUnfilt = readRemainingCapacityUnfiltered();
    uint16_t remCapFilt = readRemainingCapacityFiltered();
    uint16_t fullCapUnfilt = readFullChargeCapacityUnfiltered();
    uint16_t fullCapFilt = readFullChargeCapacityFiltered();
    uint8_t socUnfilt = readStateOfChargeUnfiltered();
    
    // ==================== 健康状态 ====================
    uint16_t soh = readStateOfHealth();

    // ==================== 串口输出 ====================
    Serial.println("\n========== BQ27421 电池信息 ==========");
    
    Serial.println("--- 基础测量 ---");
    Serial.printf("电压: %d mV\n", voltage);
    Serial.printf("电流: %d mA %s\n", current, current >= 0 ? "(充电)" : "(放电)");
    Serial.printf("电池温度: %d °C\n", batteryTemp);
    Serial.printf("芯片温度: %d °C\n", internalTemp);
    Serial.printf("待机电流: %d mA\n", standbyCurrent);
    Serial.printf("最大负载电流: %d mA\n", maxLoadCurrent);
    Serial.printf("平均功率: %d mW\n", avgPower);
    
    Serial.println("\n--- 容量与电量 ---");
    Serial.printf("电量百分比: %d %%\n", soc);
    Serial.printf("剩余容量: %d mAh\n", remainingCap);
    Serial.printf("满充容量: %d mAh\n", fullChargeCap);
    Serial.printf("标称可用容量: %d mAh\n", nominalAvailCap);
    Serial.printf("满可用容量: %d mAh\n", fullAvailCap);
    
    Serial.println("\n--- 滤波数据对比 ---");
    Serial.printf("剩余容量 (未滤波): %d mAh\n", remCapUnfilt);
    Serial.printf("剩余容量 (滤波):   %d mAh\n", remCapFilt);
    Serial.printf("满充容量 (未滤波): %d mAh\n", fullCapUnfilt);
    Serial.printf("满充容量 (滤波):   %d mAh\n", fullCapFilt);
    Serial.printf("电量 (未滤波): %d %%\n", socUnfilt);
    Serial.printf("电量 (滤波):   %d %%\n", soc);
    
    Serial.println("\n--- 健康状态 ---");
    Serial.printf("健康状态 (SOH): %d %%\n", soh);
    
    Serial.println("======================================\n");

    // ==================== LCD 显示 ====================
    lcdText(String(voltage) + "mV " + String(current) + "mA", 1);
    lcdText(String(soc) + "% " + String(remainingCap) + "mAh", 2);
}

void setup() {
	// 通用测试环境初始化（串口、SPIFFS、LCD等，如需可在此添加）
    globalTestSetup();  // 全局初始化 - 只在开始时调用一次
    UNITY_BEGIN();  // 启动 Unity 测试框架

    RUN_TEST(alwaysPassTest);  // 运行永远成功的测试
    UNITY_END();  // 结束 Unity 测试框架

    lcdText("   Fuel Gauge",1);
    initI2C();
    
    initBQ27421(1800); // 设计容量为 1800mAh
}

void loop() {
    readBatteryInfo();
    delay(2000);  // 每2秒读取一次电池电压
}

