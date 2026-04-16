#include <unity.h>
#include <Arduino.h>
#include <esp_timer.h>
#include <algorithm>
#include "../../include/hardware/lcd_driver.h"
#include "../common/test_init.h"

// ==============================
// 采样配置
// ==============================
static constexpr int SAMPLE_COUNT = 1000;

// ==============================
// 采样缓冲（存放每帧耗时 μs）
// ==============================
static uint32_t s_samples[SAMPLE_COUNT];

// ==============================
// 统计工具
// ==============================
struct Stats {
    uint32_t avg_us;
    uint32_t max_us;
    uint32_t p99_us;
};

static Stats calcStats(uint32_t* buf, int count) {
    std::sort(buf, buf + count);
    uint64_t sum = 0;
    for (int i = 0; i < count; ++i) sum += buf[i];

    Stats s;
    s.avg_us = static_cast<uint32_t>(sum / count);
    s.max_us = buf[count - 1];
    s.p99_us = buf[static_cast<int>(count * 0.99f)];
    return s;
}

static void printStats(const char* label, const Stats& s) {
    char buf[80];
    snprintf(buf, sizeof(buf), "[%s] avg=%lu us  max=%lu us  p99=%lu us  (~%.1f FPS avg)",
             label,
             (unsigned long)s.avg_us,
             (unsigned long)s.max_us,
             (unsigned long)s.p99_us,
             s.avg_us > 0 ? 1000000.0f / s.avg_us : 0.0f);
    LOG_SYSTEM_INFO(buf);
    Serial.println(buf);
}

// ==============================
// 场景准备数据
// ==============================

// 全屏静止帧：全部空格，两帧内容相同
static uint8_t s_staticDdram[32];
static uint8_t s_staticCgram[8][8];
static bool    s_staticUsed[8];

// 单字符局部更新：两帧只有 pos=0 不同
static uint8_t s_singleDdramA[32];
static uint8_t s_singleDdramB[32];

// 全屏变化帧：两帧内容完全不同
static uint8_t s_fullDdramA[32];
static uint8_t s_fullDdramB[32];

static void prepareSceneData() {
    memset(s_staticCgram, 0, sizeof(s_staticCgram));
    memset(s_staticUsed,  0, sizeof(s_staticUsed));

    // 静止帧
    memset(s_staticDdram, 0x20, 32);

    // 单字符局部更新帧
    memset(s_singleDdramA, 0x20, 32);
    memset(s_singleDdramB, 0x20, 32);
    s_singleDdramA[0] = 'A';
    s_singleDdramB[0] = 'B';

    // 全屏变化帧（A/B 交替所有字符）
    for (int i = 0; i < 32; ++i) {
        s_fullDdramA[i] = static_cast<uint8_t>('A' + (i % 26));
        s_fullDdramB[i] = static_cast<uint8_t>('a' + (i % 26));
    }
}

// ==============================
// 场景1：全屏静止帧（理论零写入）
// ==============================
void test_fps_static_frame() {
    // 先渲染一帧让 hwFrame 同步，后续全部是 no-op 写入
    lcdRenderDiff(s_staticDdram, s_staticCgram, s_staticUsed);

    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        int64_t t0 = esp_timer_get_time();
        lcdRenderDiff(s_staticDdram, s_staticCgram, s_staticUsed);
        int64_t t1 = esp_timer_get_time();
        s_samples[i] = static_cast<uint32_t>(t1 - t0);
    }

    Stats s = calcStats(s_samples, SAMPLE_COUNT);
    printStats("STATIC  ", s);

    // 静止帧几乎只有函数调用开销，预期 avg < 500 μs
    TEST_ASSERT_LESS_THAN(500UL, (unsigned long)s.avg_us);
}

// ==============================
// 场景2：单字符局部更新
// ==============================
void test_fps_single_char() {
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        const uint8_t* frame = (i & 1) ? s_singleDdramB : s_singleDdramA;
        int64_t t0 = esp_timer_get_time();
        lcdRenderDiff(frame, s_staticCgram, s_staticUsed);
        int64_t t1 = esp_timer_get_time();
        s_samples[i] = static_cast<uint32_t>(t1 - t0);
    }

    Stats s = calcStats(s_samples, SAMPLE_COUNT);
    printStats("1-CHAR  ", s);

    // 每帧1字符写入 ≈ 37μs GPIO + overhead，预期 avg < 1000 μs
    TEST_ASSERT_LESS_THAN(1000UL, (unsigned long)s.avg_us);
}

// ==============================
// 场景3：全屏32字符全部变化
// ==============================
void test_fps_full_frame() {
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        const uint8_t* frame = (i & 1) ? s_fullDdramB : s_fullDdramA;
        int64_t t0 = esp_timer_get_time();
        lcdRenderDiff(frame, s_staticCgram, s_staticUsed);
        int64_t t1 = esp_timer_get_time();
        s_samples[i] = static_cast<uint32_t>(t1 - t0);
    }

    Stats s = calcStats(s_samples, SAMPLE_COUNT);
    printStats("FULL-32 ", s);

    // 32字符每个 ≈ 37μs + cursor set，上限 32*40 = 1280μs → ≈780 FPS 理论值
    // 留出余量，avg < 5000 μs（覆盖 Serial/中断抖动）
    TEST_ASSERT_LESS_THAN(5000UL, (unsigned long)s.avg_us);
}

// ==============================
// Arduino 入口
// ==============================
void setup() {
    globalTestSetup();
    prepareSceneData();

    UNITY_BEGIN();
    RUN_TEST(test_fps_static_frame);
    RUN_TEST(test_fps_single_char);
    RUN_TEST(test_fps_full_frame);
    UNITY_END();
}

void loop() { }
