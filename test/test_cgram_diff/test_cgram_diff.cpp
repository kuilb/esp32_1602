#include <unity.h>
#include <Arduino.h>
#include "../../include/hardware/lcd_driver.h"
#include "../common/test_init.h"

// ==============================
// 测试参数
// ==============================
// Bad Apple 总帧数约 6572 帧（30 FPS × ~219 s），取其中 1000 帧做代表性采样
static constexpr int   FRAME_COUNT       = 1000;
static constexpr int   CGRAM_SLOTS       = 8;
// 每次 CGRAM 槽位写入消耗的 LCD 操作数：1×地址CMD + 8×数据行 = 9
static constexpr int   GPIO_OPS_PER_SLOT = 9;

// ==============================
// DDRAM 模板：Bad Apple 占用全部 8 槽
// ==============================
// 第一行前4列显示槽 0~3，第二行前4列显示槽 4~7，其余填充空格
static const uint8_t kBadAppleDdram[32] = {
    0, 1, 2, 3,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    4, 5, 6, 7,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20
};
static bool kAllSlotsUsed[8] = {true, true, true, true, true, true, true, true};

// ==============================
// 帧数据生成工具
// ==============================

// 为指定帧生成 8 槽 CGRAM（每帧内容完全不同 —— worst case）
static void genFrameAllChange(int frameIdx, uint8_t cgram[8][8]) {
    for (int slot = 0; slot < 8; ++slot) {
        for (int row = 0; row < 8; ++row) {
            // 用帧号 + 槽 + 行的组合生成伪随机像素，确保相邻帧不重复
            cgram[slot][row] = static_cast<uint8_t>((frameIdx * 31 + slot * 17 + row * 7) & 0x1F);
        }
    }
}

// 每 4 帧重复一次（25% 变化率，模拟慢速动画）
static void genFrameQuarterChange(int frameIdx, uint8_t cgram[8][8]) {
    int key = frameIdx / 4;
    for (int slot = 0; slot < 8; ++slot) {
        for (int row = 0; row < 8; ++row) {
            cgram[slot][row] = static_cast<uint8_t>((key * 31 + slot * 17 + row * 7) & 0x1F);
        }
    }
}

// 仅槽 0~3 每帧变化，槽 4~7 静止（模拟上半屏动画+下半屏歌词）
static void genFrameHalfChange(int frameIdx, uint8_t cgram[8][8]) {
    for (int slot = 0; slot < 4; ++slot) {
        for (int row = 0; row < 8; ++row) {
            cgram[slot][row] = static_cast<uint8_t>((frameIdx * 31 + slot * 17 + row * 7) & 0x1F);
        }
    }
    // 槽 4~7：静止（全零）
    for (int slot = 4; slot < 8; ++slot) {
        memset(cgram[slot], 0, 8);
    }
}

// ==============================
// 统计输出工具
// ==============================
static void printCgramResult(const char* label,
                              uint32_t diffWrites,
                              uint32_t fullBaseline,
                              uint32_t frames) {
    uint32_t diffGpio = diffWrites   * GPIO_OPS_PER_SLOT;
    uint32_t fullGpio = fullBaseline * GPIO_OPS_PER_SLOT;
    float saving = (fullBaseline > 0)
        ? (1.0f - (float)diffWrites / (float)fullBaseline) * 100.0f
        : 0.0f;
    float diffPerFrame  = (frames > 0) ? (float)diffWrites  / frames : 0.0f;
    float fullPerFrame  = (frames > 0) ? (float)fullBaseline / frames : 0.0f;

    char buf[120];
    snprintf(buf, sizeof(buf),
        "[%-16s] frames=%lu  diff=%lu slots (%lu GPIO)  full=%lu slots (%lu GPIO)  saved=%.1f%%  diff/frame=%.2f  full/frame=%.2f",
        label,
        (unsigned long)frames,
        (unsigned long)diffWrites,  (unsigned long)diffGpio,
        (unsigned long)fullBaseline,(unsigned long)fullGpio,
        saving,
        diffPerFrame, fullPerFrame);
    Serial.println(buf);
    LOG_SYSTEM_INFO(buf);
}

// ==============================
// 场景 A：全部 8 槽每帧全变（Bad Apple 最坏情况）
//   预期：差分无法节省，diffWrites ≈ fullBaseline（第1帧后每帧8写）
// ==============================
void test_cgram_all_change() {
    lcdResetCgramStats();
    // 确保 hwFrame 从"未初始化"状态出发，但不重置 LCD 硬件（无 lcdInit）
    uint8_t cgram[8][8];

    for (int f = 0; f < FRAME_COUNT; ++f) {
        genFrameAllChange(f, cgram);
        lcdRenderDiff(kBadAppleDdram, cgram, kAllSlotsUsed);
    }

    uint32_t diffW, fullB, frames;
    lcdGetCgramStats(&diffW, &fullB, &frames);
    printCgramResult("ALL-CHANGE", diffW, fullB, frames);

    // 全变情况下，差分写入应接近全量基线（节省 ≈ 0%）
    // 允许第1帧8次触发 valid 标志后每帧都写 → diffWrites == fullBaseline
    TEST_ASSERT_EQUAL(fullB, diffW);
}

// ==============================
// 场景 B：每 4 帧重复（25% 变化率）
//   预期：约 75% 帧可省略 CGRAM 写入
// ==============================
void test_cgram_quarter_change() {
    lcdResetCgramStats();

    uint8_t cgram[8][8];
    for (int f = 0; f < FRAME_COUNT; ++f) {
        genFrameQuarterChange(f, cgram);
        lcdRenderDiff(kBadAppleDdram, cgram, kAllSlotsUsed);
    }

    uint32_t diffW, fullB, frames;
    lcdGetCgramStats(&diffW, &fullB, &frames);
    printCgramResult("QUARTER-CHANGE", diffW, fullB, frames);

    // 25% 变化率：预期差分写入 ≈ fullBaseline × 25%
    // 加上第1帧强制写入，实际约 ≤ 30%
    float ratio = (fullB > 0) ? (float)diffW / fullB : 1.0f;
    char msg[60];
    snprintf(msg, sizeof(msg), "ratio=%.3f expected<=0.30", ratio);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(0.30f, ratio);
}

// ==============================
// 场景 C：仅上半屏（槽 0~3）每帧变化，下半屏（槽 4~7）静止
//   模拟 Bad Apple 画面 + 固定歌词
//   预期：差分写入 ≈ 全量基线的 50%
// ==============================
void test_cgram_half_change() {
    lcdResetCgramStats();

    uint8_t cgram[8][8];
    for (int f = 0; f < FRAME_COUNT; ++f) {
        genFrameHalfChange(f, cgram);
        lcdRenderDiff(kBadAppleDdram, cgram, kAllSlotsUsed);
    }

    uint32_t diffW, fullB, frames;
    lcdGetCgramStats(&diffW, &fullB, &frames);
    printCgramResult("HALF-CHANGE", diffW, fullB, frames);

    // 下半 4 槽静止，第1帧后节省约 50%
    // 预期 ratio ≈ 0.50，容差 ±5%
    float ratio = (fullB > 0) ? (float)diffW / fullB : 1.0f;
    char msg[60];
    snprintf(msg, sizeof(msg), "ratio=%.3f expected in [0.45, 0.55]", ratio);
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.45f, ratio);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(0.55f, ratio);
}

// ==============================
// Arduino 入口
// ==============================
void setup() {
    globalTestSetup();

    UNITY_BEGIN();
    RUN_TEST(test_cgram_all_change);
    RUN_TEST(test_cgram_quarter_change);
    RUN_TEST(test_cgram_half_change);
    UNITY_END();
}

void loop() { }
