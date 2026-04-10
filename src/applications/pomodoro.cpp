#include "./applications/pomodoro.h"

namespace {
constexpr uint32_t kWorkSeconds = 25U * 60U;
constexpr uint32_t kBreakSeconds = 5U * 60U;
constexpr uint8_t kFlashRounds = 6;
constexpr unsigned long kHoldProgressStartMs = 300;
constexpr unsigned long kSkipTriggerMs = 1800;

// 与休眠进度条同款 5x8 部分填充字符。
const uint8_t kProgressGlyphs[6][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10},
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C},
    {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E},
    {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}
};

void renderPomodoroScreen(bool isWorkPhase, uint32_t remainSeconds) {
    const uint32_t minutes = remainSeconds / 60U;
    const uint32_t seconds = remainSeconds % 60U;

    char line1[17];
    char line2[17];
    snprintf(line1, sizeof(line1), "%s %02lu:%02lu",
        isWorkPhase ? "Work" : "Break",
        static_cast<unsigned long>(minutes),
        static_cast<unsigned long>(seconds));
    snprintf(line2, sizeof(line2), "C:Pause R:Skip");

    lcdText(line1, 1);
    lcdText(line2, 2);
}

void renderPomodoroPausedScreen(bool isWorkPhase, uint32_t remainSeconds) {
    const uint32_t minutes = remainSeconds / 60U;
    const uint32_t seconds = remainSeconds % 60U;

    char line1[17];
    snprintf(line1, sizeof(line1), "%s PAUSE %02lu:%02lu",
        isWorkPhase ? "W" : "B",
        static_cast<unsigned long>(minutes),
        static_cast<unsigned long>(seconds));

    lcdText(line1, 1);
    lcdText("C:Start R:Skip", 2);
}

void prepareProgressGlyphs() {
    for (int slot = 0; slot <= 5; ++slot) {
        lcdCreateChar(slot, kProgressGlyphs[slot]);
    }
}

void renderSkipProgress(uint8_t percent) {
    if (percent > 100) percent = 100;

    lcdText("Hold R to skip ", 1);
    prepareProgressGlyphs();

    const int barSlots = 16;
    const int cellCols = 5;
    const int gapCols = 1;
    const int totalVirtualCols = barSlots * cellCols + (barSlots - 1) * gapCols;
    const int filledVirtualCols = (percent * totalVirtualCols) / 100;

    lcdSetCursor(16);
    for (int slot = 0; slot < barSlots; ++slot) {
        const int cellStart = slot * (cellCols + gapCols);
        int fillInCell = filledVirtualCols - cellStart;
        if (fillInCell < 0) fillInCell = 0;
        if (fillInCell > cellCols) fillInCell = cellCols;

        if (fillInCell == 0) {
            lcdDisChar(' ');
        } else {
            lcdDisCustom(fillInCell);
        }
    }
}

void alertPhaseSwitch(bool nextIsWorkPhase) {
    const int originalBrightness = brightness;
    const char* phaseText = nextIsWorkPhase ? "Start Work" : "Start Break";

    buzzerPlayWarning();

    for (uint8_t i = 0; i < kFlashRounds; ++i) {
        setLcdBrightness(255);
        lcdText(phaseText, 1);
        lcdText("Get Ready", 2);
        vTaskDelay(pdMS_TO_TICKS(120));

        setLcdBrightness(20);
        lcdClear();
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    setLcdBrightness(static_cast<uint8_t>(constrain(originalBrightness, 0, 255)));
}
}  // namespace

void runPomodoroApp() {
    inMenuMode = false;
    globalButtonDelay(FIRST_TIME_DELAY);

    bool isWorkPhase = true;
    bool isPaused = true;
    uint32_t elapsedBeforePauseSec = 0;
    uint32_t phaseDurationSec = kWorkSeconds;
    uint32_t lastRenderedRemain = UINT32_MAX;
    unsigned long phaseStartMs = millis();

    bool rightLastState = false;
    bool rightOverlayStarted = false;
    unsigned long rightPressStartMs = 0;

    LOG_SYSTEM_INFO("Pomodoro app started");

    while (true) {
        // 电源键短按会由全局按键逻辑切回菜单，这里检测状态变化后退出应用循环。
        if (inMenuMode) {
            LOG_SYSTEM_INFO("Pomodoro app exit by power/back action");
            return;
        }

        if (isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)) {
            isPaused = !isPaused;
            if (isPaused) {
                const uint32_t elapsedSec = (millis() - phaseStartMs) / 1000U;
                elapsedBeforePauseSec = (elapsedSec > phaseDurationSec) ? phaseDurationSec : elapsedSec;
                buzzerPlaySelectSound();
            } else {
                phaseStartMs = millis() - (elapsedBeforePauseSec * 1000UL);
                buzzerPlaySelectSound();
            }
            lastRenderedRemain = UINT32_MAX;
        }

        const bool rightState = (digitalRead(BUTTON_RIGHT_PIN) == HIGH);
        const unsigned long nowMs = millis();

        if (rightState && !rightLastState) {
            rightPressStartMs = nowMs;
            rightOverlayStarted = false;
        }

        if (rightState) {
            const unsigned long pressDurationMs = nowMs - rightPressStartMs;
            if (!rightOverlayStarted && pressDurationMs >= kHoldProgressStartMs) {
                rightOverlayStarted = true;
                lcdPushOverlayFrame();
                lcdClear();
                renderSkipProgress(0);
            }

            if (rightOverlayStarted) {
                const unsigned long progressElapsed = pressDurationMs - kHoldProgressStartMs;
                const unsigned long progressWindow = kSkipTriggerMs - kHoldProgressStartMs;
                const uint8_t percent = (progressWindow == 0)
                    ? 100
                    : static_cast<uint8_t>(min(100UL, (progressElapsed * 100UL) / progressWindow));

                renderSkipProgress(percent);

                if (pressDurationMs >= kSkipTriggerMs) {
                    lcdPopOverlayFrame();
                    buzzerPlaySleepSound();
                    isWorkPhase = !isWorkPhase;
                    phaseDurationSec = isWorkPhase ? kWorkSeconds : kBreakSeconds;
                    isPaused = false;
                    elapsedBeforePauseSec = 0;
                    phaseStartMs = millis();
                    lastRenderedRemain = UINT32_MAX;
                    rightOverlayStarted = false;
                    vTaskDelay(pdMS_TO_TICKS(120));
                }
            }
        }

        if (!rightState && rightLastState) {
            if (rightOverlayStarted) {
                lcdPopOverlayFrame();
                rightOverlayStarted = false;
                lastRenderedRemain = UINT32_MAX;
            }
        }
        rightLastState = rightState;

        if (rightOverlayStarted) {
            vTaskDelay(pdMS_TO_TICKS(40));
            continue;
        }

        if (isPaused) {
            const uint32_t remainPausedSec = (elapsedBeforePauseSec >= phaseDurationSec)
                ? 0
                : (phaseDurationSec - elapsedBeforePauseSec);
            if (remainPausedSec != lastRenderedRemain) {
                renderPomodoroPausedScreen(isWorkPhase, remainPausedSec);
                lastRenderedRemain = remainPausedSec;
            }
            vTaskDelay(pdMS_TO_TICKS(80));
            continue;
        }

        const uint32_t elapsedSec = (millis() - phaseStartMs) / 1000U;
        if (elapsedSec >= phaseDurationSec) {
            isWorkPhase = !isWorkPhase;
            phaseDurationSec = isWorkPhase ? kWorkSeconds : kBreakSeconds;
            alertPhaseSwitch(isWorkPhase);
            phaseStartMs = millis();
            lastRenderedRemain = UINT32_MAX;
            continue;
        }

        const uint32_t remainSec = phaseDurationSec - elapsedSec;
        if (remainSec != lastRenderedRemain) {
            renderPomodoroScreen(isWorkPhase, remainSec);
            lastRenderedRemain = remainSec;
        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }
}
