#include "clock.h"
#include "utils/logger.h"

// 时间同步相关变量
volatile bool isTimeSyncInProgress = false;
static unsigned long timeSyncStartTime = 0;
static unsigned long lastTimeSyncAttempt = 0;
TimeSyncState timeSyncState = TIME_SYNC_IDLE;

// 初始化时间同步
void initTimeSync() {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_TIME_ERROR("WiFi not connected, cannot sync time");
        return;
    }
    
    if (timeSyncState == TIME_SYNC_SUCCESS) {
        LOG_TIME_WARN("Time already synced");
        return;
    }
    
    if (timeSyncState == TIME_SYNC_IN_PROGRESS) {
        LOG_TIME_WARN("Time sync already in progress");
        return;
    }
    
    LOG_TIME_INFO("Initializing time sync...");
    configTime(GMT_OFFSET_HOUR * 3600, 0, "ntp.aliyun.com", "ntp1.aliyun.com", "ntp.ntsc.ac.cn");
    isTimeSyncInProgress = true;
    timeSyncStartTime = millis();
    lastTimeSyncAttempt = millis();
    LOG_TIME_DEBUG("NTP servers configured, waiting for response...");
}

// 更新时间同步状态
void updateTimeSync() {
    if (timeSyncState == TIME_SYNC_IN_PROGRESS || timeSyncState == TIME_SYNC_SUCCESS || WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    unsigned long now = millis();
    
    // 检查超时
    if (now - timeSyncStartTime > TIME_SYNC_TIMEOUT) {
        LOG_TIME_WARN("Sync timeout after 10 seconds");
        isTimeSyncInProgress = false;
        return;
    }
    
    // 每秒尝试一次
    if (now - lastTimeSyncAttempt > TIME_SYNC_RETRY_INTERVAL) {
        lastTimeSyncAttempt = now;
        
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            LOG_TIME_INFO("NTP Time Synced successfully!");
            timeSyncState = TIME_SYNC_SUCCESS;
            isTimeSyncInProgress = false;
            
            // 获取 UNIX 时间戳
            time_t unixTimestamp = mktime(&timeinfo);
            LOG_TIME_DEBUG("UNIX Timestamp: %ld", unixTimestamp);
            
            // 显示同步成功的时间
            LOG_TIME_INFO("Time sync took %lu ms", now - timeSyncStartTime);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            LOG_TIME_INFO("Current time: %s", timeStr);
        } else {
            LOG_TIME_INFO("Waiting for NTP response...");
        }
    }
}

void updateClockScreen() {
    if (timeSyncState == TIME_SYNC_SUCCESS) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            char timeBuf[17];
            char dateBuf[17];

            // 格式化时间
            strftime(timeBuf, sizeof(timeBuf), "    %H:%M:%S", &timeinfo);
            
            // 格式化日期和星期
            strftime(dateBuf, sizeof(dateBuf), "   %m/%d  %a", &timeinfo);

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
        delay(800);
    }
}
