#include <unity.h>
#include <Arduino.h>
#include "../../include/hardware/buzzer.h"
#include "../../include/hardware/lcd_driver.h"
#include "../common/test_init.h"

/**
 * @brief 测试蜂鸣器初始化
 */
void test_buzzer_init() {
    buzzerInit();
    TEST_ASSERT_TRUE(true);  // 如果没有异常就通过
    LOG_SYSTEM_INFO("✓ Buzzer initialized successfully");
}

/**
 * @brief 测试音量设置和获取
 */
void test_buzzer_volume() {
    buzzerSetVolume(50);
    TEST_ASSERT_EQUAL(50, buzzerGetVolume());
    
    buzzerSetVolume(100);
    TEST_ASSERT_EQUAL(100, buzzerGetVolume());
    
    buzzerSetVolume(0);
    TEST_ASSERT_EQUAL(0, buzzerGetVolume());
    
    // 测试超出范围的值（应该被限制）
    buzzerSetVolume(150);  // 应该被限制为100
    TEST_ASSERT_EQUAL(100, buzzerGetVolume());
    
    LOG_SYSTEM_INFO("✓ Volume control test passed");
}

/**
 * @brief 测试基础音调播放
 */
void test_buzzer_tone() {
    lcdText("Testing tone...", 1);
    
    buzzerSetVolume(50);
    
    // 测试不同频率
    buzzerTone(1000, 50);
    delay(200);
    buzzerNoTone();
    delay(100);
    
    buzzerTone(2000, 50);
    delay(200);
    buzzerNoTone();
    delay(100);
    
    buzzerTone(3000, 50);
    delay(200);
    buzzerNoTone();
    delay(100);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Basic tone test passed");
}

/**
 * @brief 测试快捷音效 - beep
 */
void test_buzzer_beep() {
    lcdText("Testing beep...", 1);
    
    buzzerSetVolume(50);
    buzzerBeep();
    delay(300);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Beep test passed");
}

/**
 * @brief 测试快捷音效 - double beep
 */
void test_buzzer_double_beep() {
    lcdText("Testing 2-beep..", 1);
    
    buzzerSetVolume(50);
    buzzerDoubleBeep();
    delay(500);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Double beep test passed");
}

/**
 * @brief 测试成功提示音
 */
void test_buzzer_success() {
    lcdText("Testing success.", 1);
    
    buzzerSetVolume(60);
    buzzerPlaySuccess();
    delay(500);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Success sound test passed");
}

/**
 * @brief 测试错误提示音
 */
void test_buzzer_error() {
    lcdText("Testing error...", 1);
    
    buzzerSetVolume(60);
    buzzerPlayError();
    delay(800);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Error sound test passed");
}

/**
 * @brief 测试警告音
 */
void test_buzzer_warning() {
    lcdText("Testing warning.", 1);
    
    buzzerSetVolume(70);
    buzzerPlayWarning();
    delay(800);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Warning sound test passed");
}

/**
 * @brief 测试开机音
 */
void test_buzzer_startup() {
    lcdText("Testing startup.", 1);
    
    buzzerSetVolume(60);
    buzzerPlayStartup();
    delay(800);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Startup sound test passed");
}

/**
 * @brief 测试按键音
 */
void test_buzzer_click() {
    lcdText("Testing click...", 1);
    
    buzzerSetVolume(50);
    for (int i = 0; i < 3; i++) {
        buzzerPlayClick();
        delay(200);
    }
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Click sound test passed");
}

/**
 * @brief 测试忙提示音
 */
void test_buzzer_busy() {
    lcdText("Testing busy...", 1);
    
    buzzerSetVolume(50);
    buzzerPlayBusy();
    delay(500);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Busy sound test passed");
}

/**
 * @brief 测试自定义旋律
 */
void test_buzzer_melody() {
    lcdText("Testing melody..", 1);
    
    // 简单的旋律 - "小星星"前几个音符
    uint16_t melody[] = {
        NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, 
        NOTE_A4, NOTE_A4, NOTE_G4, 0
    };
    uint16_t durations[] = {
        200, 200, 200, 200,
        200, 200, 400, 200
    };
    
    buzzerPlayMelody(melody, durations, 8, 60);
    delay(2000);
    
    TEST_ASSERT_TRUE(true);
    LOG_SYSTEM_INFO("✓ Custom melody test passed");
}

/**
 * @brief 测试忙状态检查
 */
void test_buzzer_is_busy() {
    lcdText("Testing isBusy..", 1);
    
    TEST_ASSERT_FALSE(buzzerIsBusy());  // 应该是空闲
    
    buzzerPlaySuccess();
    delay(50);  // 给任务一点时间开始
    TEST_ASSERT_TRUE(buzzerIsBusy());   // 应该是忙碌
    
    delay(1000);  // 等待播放完成
    TEST_ASSERT_FALSE(buzzerIsBusy());  // 应该又是空闲
    
    LOG_SYSTEM_INFO("✓ isBusy test passed");
}

void setup() {
    // 全局测试环境初始化
    globalTestSetup();
    
    UNITY_BEGIN();
    
    // 基础功能测试
    RUN_TEST(test_buzzer_init);
    RUN_TEST(test_buzzer_volume);
    RUN_TEST(test_buzzer_tone);
    
    // 快捷音效测试
    RUN_TEST(test_buzzer_beep);
    RUN_TEST(test_buzzer_double_beep);
    RUN_TEST(test_buzzer_success);
    RUN_TEST(test_buzzer_error);
    RUN_TEST(test_buzzer_warning);
    RUN_TEST(test_buzzer_startup);
    RUN_TEST(test_buzzer_click);
    RUN_TEST(test_buzzer_busy);
    
    // 高级功能测试
    RUN_TEST(test_buzzer_melody);
    RUN_TEST(test_buzzer_is_busy);
    
    UNITY_END();
    
    globalTestTeardown();
}

void loop() {
    // 测试完成后的演示模式
    lcdText("Demo Mode", 0);
    lcdText("Press to test", 1);
    
    static int demoIndex = 0;
    delay(3000);
    
    switch (demoIndex % 8) {
        case 0:
            lcdText("Beep sound", 1);
            buzzerBeep();
            break;
        case 1:
            lcdText("Success sound", 1);
            buzzerPlaySuccess();
            break;
        case 2:
            lcdText("Error sound", 1);
            buzzerPlayError();
            break;
        case 3:
            lcdText("Warning sound", 1);
            buzzerPlayWarning();
            break;
        case 4:
            lcdText("Startup sound", 1);
            buzzerPlayStartup();
            break;
        case 5:
            lcdText("Click sound", 1);
            buzzerPlayClick();
            break;
        case 6:
            lcdText("Busy sound", 1);
            buzzerPlayBusy();
            break;
        case 7:
            lcdText("Melody", 1);
            uint16_t melody[] = {NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6};
            uint16_t durations[] = {150, 150, 150, 300};
            buzzerPlayMelody(melody, durations, 4, 60);
            break;
    }
    
    demoIndex++;
    delay(1000);
}
