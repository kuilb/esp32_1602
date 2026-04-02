#include "./applications/clock.h"

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
        }
    } else {
        lcdText("Can't get Time", 1);
        lcdText("Check network", 2);
        LOG_TIME_WARN("Time not synced yet, cannot display clock");
        currentState = STATE_MENU;
        delay(800);
    }
}