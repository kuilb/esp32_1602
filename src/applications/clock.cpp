#include "clock.h"
#include "utils/logger.h"

bool timeSynced = false;

// 时间同步相关变量
bool timeSyncInProgress = false;
static unsigned long timeSyncStartTime = 0;
static unsigned long lastTimeSyncAttempt = 0;
static const unsigned long TIME_SYNC_TIMEOUT = 10000; // 10秒超时
static const unsigned long TIME_SYNC_RETRY_INTERVAL = 1000; // 1秒重试间隔

// 初始化时间同步
void initTimeSync() {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_TIME_ERROR("WiFi not connected, cannot sync time");
        return;
    }
    
    if (timeSynced) {
        LOG_TIME_INFO("Time already synced");
        return;
    }
    
    if (timeSyncInProgress) {
        LOG_TIME_DEBUG("Time sync already in progress");
        return;
    }
    
    LOG_TIME_INFO("Initializing non-blocking time sync...");
    configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp1.aliyun.com", "ntp.ntsc.ac.cn");
    timeSyncInProgress = true;
    timeSyncStartTime = millis();
    lastTimeSyncAttempt = millis();
    LOG_TIME_DEBUG("NTP servers configured, waiting for response...");
}

// 更新非阻塞时间同步状态（应在主循环中调用）
void updateTimeSync() {
    if (!timeSyncInProgress || timeSynced) {
        return;
    }
    
    unsigned long now = millis();
    
    // 检查超时
    if (now - timeSyncStartTime > TIME_SYNC_TIMEOUT) {
        LOG_TIME_WARN("Timeout after 10 seconds");
        timeSyncInProgress = false;
        return;
    }
    
    // 每秒尝试一次
    if (now - lastTimeSyncAttempt > TIME_SYNC_RETRY_INTERVAL) {
        lastTimeSyncAttempt = now;
        
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            LOG_TIME_INFO("NTP Time Synced successfully!");
            timeSynced = true;
            timeSyncInProgress = false;
            
            // 获取 UNIX 时间戳
            time_t unixTimestamp = mktime(&timeinfo);
            LOG_TIME_DEBUG("UNIX Timestamp: %ld", unixTimestamp);
            
            // 显示同步成功的时间
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            LOG_TIME_INFO("Current time: %s", timeStr);
        } else {
            LOG_VERBOSE(LOG_MODULE_TIME_SYNC, "Waiting for NTP response...");
        }
    }
}

void setupTime() {
    if(WiFi.status() != WL_CONNECTED) {
        LOG_TIME_ERROR("WiFi not connected, cannot sync time");
        lcd_text("no connect", 1);
        lcd_text("exiting...", 2);
        return;
    }

    if (!timeSynced && !timeSyncInProgress) {
        initTimeSync();
    }
    
    lcd_text("Waiting for", 1);
    lcd_text("NTP time sync...", 2);

    updateTimeSync();
}

void updateClockScreen() {
    if (timeSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            char timeBuf[17];
            char dateBuf[17];

            // 格式化时间
            strftime(timeBuf, sizeof(timeBuf), "    %H:%M:%S", &timeinfo);
            
            // 格式化日期和星期
            strftime(dateBuf, sizeof(dateBuf), "   %m/%d  %a", &timeinfo);

            lcd_text(dateBuf, 2);   // 第1行显示日期 + 星期
            lcd_text(timeBuf, 1);   // 第2行显示时间
        } else {
            lcd_text("Time Error", 2);
        }
    } else {
        lcd_text("Can't get Time", 1);
        lcd_text("Check network", 2);
        delay(800);
    }
}
