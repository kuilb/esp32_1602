#include "clock.h"
#include "utils/logger.h"

// 时间同步相关变量
static unsigned long timeSyncStartTime = 0;
static unsigned long lastTimeSyncAttempt = 0;
TimeSyncState timeSyncState = TIME_SYNC_IDLE;
struct tm localTimeInfo;

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
    timeSyncState = TIME_SYNC_IN_PROGRESS;
    timeSyncStartTime = millis();
    lastTimeSyncAttempt = millis();
    LOG_TIME_DEBUG("NTP servers configured, waiting for response...");
}

// 更新时间同步状态
void updateTimeSync() {
    if (timeSyncState == TIME_SYNC_SUCCESS || WiFi.status() != WL_CONNECTED) {
        LOG_TIME_WARN("Time sync not in progress or already successful, or WiFi not connected");
        return;
    }
    
    unsigned long now = millis();
    
    // 检查超时
    if (now - timeSyncStartTime > TIME_SYNC_TIMEOUT) {
        LOG_TIME_WARN("Sync timeout after 10 seconds");
        timeSyncState = TIME_SYNC_FAILED;
        return;
    }
    
    // 每秒尝试一次
    if (now - lastTimeSyncAttempt > TIME_SYNC_RETRY_INTERVAL) {
        lastTimeSyncAttempt = now;
        
        if (getLocalTime(&localTimeInfo)) {
            LOG_TIME_INFO("NTP Time Synced successfully!");
            timeSyncState = TIME_SYNC_SUCCESS;
            
            // 获取 UNIX 时间戳
            time_t unixTimestamp = mktime(&localTimeInfo);
            LOG_TIME_DEBUG("UNIX Timestamp: %ld", unixTimestamp);

            
            // 显示同步成功的时间
            LOG_TIME_INFO("Time sync took %lu ms", now - timeSyncStartTime);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localTimeInfo);
            LOG_TIME_INFO("Current time: %s", timeStr);
        } else {
            LOG_TIME_INFO("Waiting for NTP response...");
        }
    }
}

void updateClockScreen() {
    if (timeSyncState == TIME_SYNC_SUCCESS) {
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

// 时间同步后台任务
void timeSyncTask(void* parameter) {
    LOG_TIME_INFO("Time sync background task started");
    
    while (timeSyncState != TIME_SYNC_SUCCESS) {
        updateTimeSync();
        vTaskDelay(pdMS_TO_TICKS(1000));  // 每秒检查一次
    }
    
    LOG_TIME_INFO("Time sync background task completed");
    vTaskDelete(NULL);
}
