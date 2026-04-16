#include "./applications/clock.h"
#include "./hardware/buzzer.h"

static bool s_clockIsNewInterface = false;
static unsigned long s_lastClockDisplayUpdate = 0;
static unsigned long s_lastClockFailSoundMs = 0;

static void _playClockFailSoundThrottled(unsigned long intervalMs = 2000) {
    const unsigned long now = millis();
    if (now - s_lastClockFailSoundMs < intervalMs) {
        return;
    }
    s_lastClockFailSoundMs = now;
    buzzerPlayError();
}

static bool _ensureClockTimeSynced() {
    if (!(timeSyncState == TIME_SYNC_SUCCESS) && !(getRtcTime().tv_sec > 1765967312)) { // 2025-12-17 18:40 GMT+8
        lcdText("Try time sync", 1);
        lcdText("Please wait", 2);
        updateTimeSync();
        if (timeSyncState != TIME_SYNC_SUCCESS) {
            LOG_TIME_WARN("Time not synced yet, cannot display");
            lcdText("Time not synced", 1);
            lcdText("", 2);
            _playClockFailSoundThrottled();
            return false;
        }
    }
    return true;
}

void enterClockInterface() {
    s_clockIsNewInterface = true;
    enterAppInterface(handleClockInterface, false);
}

void handleClockInterface() {
    // 先处理退出，避免在未同步时被前置校验“卡住”
    if (isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)) {
        LOG_TIME_INFO("Exit clock interface to menu");
        exitAppInterface(FIRST_TIME_DELAY);
        return;
    }

    if (!_ensureClockTimeSynced()) {
        exitAppInterface(FIRST_TIME_DELAY);
        return;
    }

    if (millis() - s_lastClockDisplayUpdate > 1000 || s_clockIsNewInterface) {
        s_clockIsNewInterface = false;
        updateClockScreen();
        s_lastClockDisplayUpdate = millis();
    }
}

void updateClockScreen() {
    // 检查NTP是否已同步，或RTC时间是否有效（晚于2025-12-17）
    bool timeValid = (timeSyncState == TIME_SYNC_SUCCESS) || (getRtcTime().tv_sec > 1765967312);
    
    if (timeValid) {
        if (getLocalTime(&localTimeInfo)) {
            char timeBuf[17];
            char dateBuf[17];

            // 格式化时间
            strftime(timeBuf, sizeof(timeBuf), "    %H:%M:%S", &localTimeInfo);
            
            // 格式化日期和星期
            strftime(dateBuf, sizeof(dateBuf), "   %m/%d  %a", &localTimeInfo);

            lcdText(dateBuf, 2);   // 第1行显示日期 + 星期
            lcdText(timeBuf, 1);   // 第2行显示时间
        } else {
            lcdText("Time Error", 1);
            lcdText(" ", 2);
            LOG_TIME_ERROR("Failed to get local time for clock display");
            _playClockFailSoundThrottled();
        }
    } else {
        lcdText("Can't get Time", 1);
        lcdText("Check network", 2);
        LOG_TIME_WARN("Time not synced yet, cannot display clock");
        _playClockFailSoundThrottled();
        exitAppInterface(FIRST_TIME_DELAY);
    }
}