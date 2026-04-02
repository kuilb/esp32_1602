#include <unity.h>
#include <Arduino.h>
#include "../../include/hardware/lcd_driver.h"
#include "../../include/services/time_manager.h"
#include "../common/test_init.h"  // 引入共享的测试初始化

// 永远成功的测试用例
void alwaysPassTest() {
    TEST_ASSERT_TRUE(true);  // 断言 true，总是成功
}

void setup() {
	// 通用测试环境初始化（串口、SPIFFS、LCD等，如需可在此添加）
    globalTestSetup();  // 全局初始化 - 只在开始时调用一次
    UNITY_BEGIN();  // 启动 Unity 测试框架

    RUN_TEST(alwaysPassTest);  // 运行永远成功的测试
    UNITY_END();  // 结束 Unity 测试框架

    initTime(false);
}

void loop() {
	
}

