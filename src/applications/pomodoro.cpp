#include "./applications/pomodoro.h"
#include "./ui/hold_progress.h"

namespace {
constexpr uint32_t kWorkSeconds = 25U * 60U;
constexpr uint32_t kBreakSeconds = 5U * 60U;
constexpr uint8_t kFlashRounds = 6;
constexpr unsigned long kHoldProgressStartMs = 300;
constexpr unsigned long kSkipTriggerMs = 1800;


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

struct PomodoroState {
    bool active = false;
    bool isWorkPhase = true;
    bool isPaused = true;
    uint32_t elapsedBeforePauseSec = 0;
    uint32_t phaseDurationSec = kWorkSeconds;
    uint32_t lastRenderedRemain = UINT32_MAX;
    unsigned long phaseStartMs = 0;
    bool rightLastState = false;
    bool rightOverlayStarted = false;
    bool rightSkipTriggered = false;
    unsigned long rightPressStartMs = 0;
    bool centerLastState = false;
};

static PomodoroState s_pomo;

static void _resetPomodoroState() {
    s_pomo = PomodoroState{};
}
}  // namespace

void enterPomodoroInterface() {
    _resetPomodoroState();
    s_pomo.active = true;
    s_pomo.phaseStartMs = millis();
    enterAppInterface(handlePomodoroInterface, false);
    globalButtonDelay(FIRST_TIME_DELAY);
    LOG_SYSTEM_INFO("Pomodoro app started");
}

void handlePomodoroInterface() {
    if (!s_pomo.active) {
        return;
    }

    const unsigned long nowMs = millis();

    const bool centerState = buttonJustPressed[CENTER];
    if (centerState && !s_pomo.centerLastState) {
        s_pomo.isPaused = !s_pomo.isPaused;
        if (s_pomo.isPaused) {
            const uint32_t elapsedSec = (nowMs - s_pomo.phaseStartMs) / 1000U;
            s_pomo.elapsedBeforePauseSec = (elapsedSec > s_pomo.phaseDurationSec) ? s_pomo.phaseDurationSec : elapsedSec;
            buzzerPlaySelectSound();
        } else {
            s_pomo.phaseStartMs = nowMs - (s_pomo.elapsedBeforePauseSec * 1000UL);
            buzzerPlaySelectSound();
        }
        s_pomo.lastRenderedRemain = UINT32_MAX;
    }
    s_pomo.centerLastState = centerState;

    const bool rightState = (digitalRead(BUTTON_RIGHT_PIN) == HIGH);

    if (rightState && !s_pomo.rightLastState) {
        s_pomo.rightPressStartMs = nowMs;
        s_pomo.rightOverlayStarted = false;
        s_pomo.rightSkipTriggered = false;
    }

    if (rightState && !s_pomo.rightSkipTriggered) {
        const unsigned long pressDurationMs = nowMs - s_pomo.rightPressStartMs;
        if (!s_pomo.rightOverlayStarted && pressDurationMs >= kHoldProgressStartMs) {
            s_pomo.rightOverlayStarted = true;
            lcdPushOverlayFrame();
            lcdClear();
            renderHoldProgressBar("Hold R to skip", 0);
        }

        if (s_pomo.rightOverlayStarted) {
            const unsigned long progressElapsed = pressDurationMs - kHoldProgressStartMs;
            const unsigned long progressWindow = kSkipTriggerMs - kHoldProgressStartMs;
            const uint8_t percent = (progressWindow == 0)
                ? 100
                : static_cast<uint8_t>(min(100UL, (progressElapsed * 100UL) / progressWindow));

            renderHoldProgressBar("Hold R to skip", percent);

            if (pressDurationMs >= kSkipTriggerMs) {
                lcdPopOverlayFrame();
                buzzerPlaySleepSound();
                s_pomo.isWorkPhase = !s_pomo.isWorkPhase;
                s_pomo.phaseDurationSec = s_pomo.isWorkPhase ? kWorkSeconds : kBreakSeconds;
                s_pomo.isPaused = false;
                s_pomo.elapsedBeforePauseSec = 0;
                s_pomo.phaseStartMs = nowMs;
                s_pomo.lastRenderedRemain = UINT32_MAX;
                s_pomo.rightOverlayStarted = false;
                // 本次按住仅允许触发一次，必须松开后才可再次触发。
                s_pomo.rightSkipTriggered = true;
            }
        }
    }

    if (!rightState && s_pomo.rightLastState) {
        if (s_pomo.rightOverlayStarted) {
            lcdPopOverlayFrame();
            s_pomo.rightOverlayStarted = false;
            s_pomo.lastRenderedRemain = UINT32_MAX;
        }
    }
    s_pomo.rightLastState = rightState;

    if (s_pomo.rightOverlayStarted) {
        return;
    }

    if (s_pomo.isPaused) {
        const uint32_t remainPausedSec = (s_pomo.elapsedBeforePauseSec >= s_pomo.phaseDurationSec)
            ? 0
            : (s_pomo.phaseDurationSec - s_pomo.elapsedBeforePauseSec);
        if (remainPausedSec != s_pomo.lastRenderedRemain) {
            renderPomodoroPausedScreen(s_pomo.isWorkPhase, remainPausedSec);
            s_pomo.lastRenderedRemain = remainPausedSec;
        }
        return;
    }

    const uint32_t elapsedSec = (nowMs - s_pomo.phaseStartMs) / 1000U;
    if (elapsedSec >= s_pomo.phaseDurationSec) {
        s_pomo.isWorkPhase = !s_pomo.isWorkPhase;
        s_pomo.phaseDurationSec = s_pomo.isWorkPhase ? kWorkSeconds : kBreakSeconds;
        alertPhaseSwitch(s_pomo.isWorkPhase);
        s_pomo.phaseStartMs = millis();
        s_pomo.lastRenderedRemain = UINT32_MAX;
        return;
    }

    const uint32_t remainSec = s_pomo.phaseDurationSec - elapsedSec;
    if (remainSec != s_pomo.lastRenderedRemain) {
        renderPomodoroScreen(s_pomo.isWorkPhase, remainSec);
        s_pomo.lastRenderedRemain = remainSec;
    }
}

