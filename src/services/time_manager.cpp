#include "./services/time_manager.h"
#include "esp_sleep.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

struct tm localTimeInfo;
time_t timeStamp = 0;  // 当前时间戳（秒）
RTC_DATA_ATTR struct timeval sleep_enter_time;
RTC_DATA_ATTR uint64_t sleep_enter_rtc_time = 0;  // 睡眠时的RTC计数器值

TimeSyncState timeSyncState = TIME_SYNC_IDLE;   //< 当前时间同步状态
unsigned long timeSyncStartTime = 0;        //< 时间同步开始时间
unsigned long lastTimeSyncAttempt = 0;      //< 上次时间同步尝试时间
static unsigned long s_lastTimeSyncRetryAfterFailureMs = 0;

static const unsigned long TIME_SYNC_RETRY_AFTER_FAILURE_MS = 15000;

void initTime(bool isSleepWakeup) {
    // 启用外部32.768kHz晶振
    rtc_clk_32k_enable(true);
    delay(100);  // 等待晶振稳定
    
    // 设置RTC慢时钟源为外部晶振
    rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
    
    // 确保RTC域在深度睡眠时保持供电
    // 这对于保持RTC时钟运行至关重要
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);

    if (isSleepWakeup && sleep_enter_rtc_time > 0) {
        // 从深度睡眠唤醒，使用RTC计数器来计算实际睡眠时长
        uint64_t wakeup_rtc_time = rtc_time_get();
        
        // 计算RTC刻度差值
        uint64_t rtc_ticks = wakeup_rtc_time - sleep_enter_rtc_time;
        
        // 获取RTC慢时钟校准值（Q13.19格式，表示1个RTC周期等于多少微秒）
        uint32_t cal_val = rtc_clk_cal(RTC_CAL_32K_XTAL, 100);
        
        // 将RTC刻度转换为微秒：(ticks * cal_val) >> 19
        // cal_val是以Q13.19定点格式存储的，表示1个RTC周期的微秒数
        // RTC_CLK_CAL_FRACT = 19（定点小数位数）
        uint64_t sleep_duration_us = (rtc_ticks * (uint64_t)cal_val) >> RTC_CLK_CAL_FRACT;
        
        LOG_SYSTEM_DEBUG("RTC ticks on wakeup: %llu", wakeup_rtc_time);
        LOG_SYSTEM_DEBUG("RTC ticks at sleep: %llu", sleep_enter_rtc_time);
        LOG_SYSTEM_DEBUG("RTC ticks elapsed: %llu (cal_val=%lu)", rtc_ticks, cal_val);
        LOG_SYSTEM_DEBUG("Actual sleep duration: %llu us (%.2f seconds)", sleep_duration_us, sleep_duration_us / 1000000.0);
        
        // 恢复系统时间：睡眠前的时间 + 实际睡眠时长
        struct timeval corrected_time;
        corrected_time.tv_sec = sleep_enter_time.tv_sec + (sleep_duration_us / 1000000);
        corrected_time.tv_usec = sleep_enter_time.tv_usec + (sleep_duration_us % 1000000);
        
        // 处理微秒溢出
        if (corrected_time.tv_usec >= 1000000) {
            corrected_time.tv_sec += 1;
            corrected_time.tv_usec -= 1000000;
        }
        
        // 设置系统时间
        settimeofday(&corrected_time, NULL);
        LOG_SYSTEM_INFO("System time restored after sleep: %ld.%06ld s", corrected_time.tv_sec, corrected_time.tv_usec);
        
        struct timeval verify_time;
        gettimeofday(&verify_time, NULL);
        LOG_SYSTEM_DEBUG("Verified system time: %ld.%06ld s", verify_time.tv_sec, verify_time.tv_usec);
    } else {
        // 正常启动或首次启动
        struct timeval now;
        gettimeofday(&now, NULL);
        LOG_SYSTEM_DEBUG("Normal boot, system time: %ld.%06ld s", now.tv_sec, now.tv_usec);
    }

    rtc_slow_freq_t rtc_source = rtc_clk_slow_freq_get();
    if (rtc_source == RTC_SLOW_FREQ_32K_XTAL) {
        LOG_SYSTEM_DEBUG("RTC clock source: External 32.768 kHz crystal");
    }
    else if(rtc_source == RTC_SLOW_FREQ_RTC) {
        LOG_SYSTEM_DEBUG("RTC clock source: Internal 136 kHz RC oscillator");
    }
    else if(rtc_source == RTC_SLOW_FREQ_8MD256) {
        LOG_SYSTEM_DEBUG("RTC clock source: Internal 17.5 MHz RC oscillator divided by 256");
    }
    else{
        LOG_SYSTEM_DEBUG ("RTC clock source: Internal RC oscillator");
    }

    // 获取当前 RTC 时间
    uint64_t rtc_time_us = esp_timer_get_time();
    LOG_SYSTEM_DEBUG("Current absolute time: %llu us\n", rtc_time_us);
}

void initNtpTimeSync() {
    LOG_TIME_DEBUG("initNtpTimeSync called, checking WiFi status...");
    
    if (WiFi.status() != WL_CONNECTED) {
        LOG_TIME_ERROR("WiFi not connected, cannot sync time");
        return;
    }
    
    LOG_TIME_DEBUG("WiFi connected, checking sync state...");
    
    if (timeSyncState == TIME_SYNC_SUCCESS) {
        LOG_TIME_WARN("Time already synced");
        return;
    }
    
    if (timeSyncState == TIME_SYNC_IN_PROGRESS) {
        LOG_TIME_WARN("Time sync already in progress");
        return;
    }
    
    LOG_TIME_INFO("Initializing time sync...");
    
    // 配置NTP服务器，设置GMT偏移为8小时
    // 这样系统时间直接存储为本地时间（UTC+8），无需额外时区转换
    configTime(GMT_OFFSET_HOUR * 3600, 0, "ntp.aliyun.com", "ntp1.aliyun.com", "ntp.ntsc.ac.cn");
    timeSyncState = TIME_SYNC_IN_PROGRESS;
    timeSyncStartTime = millis();
    lastTimeSyncAttempt = millis();
    LOG_TIME_DEBUG("NTP servers configured, waiting for response...");
}

// 更新时间同步状态
void updateTimeSync() {
    // 仅在“进行中 + WiFi 已连接”时更新，其他状态静默返回，避免高频噪音日志。
    if (timeSyncState != TIME_SYNC_IN_PROGRESS || WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    unsigned long now = millis();
    
    // 检查超时
    if (now - timeSyncStartTime > TIME_SYNC_TIMEOUT) {
        LOG_TIME_WARN("Sync timeout after %lu seconds", TIME_SYNC_TIMEOUT / 1000UL);
        timeSyncState = TIME_SYNC_FAILED;
        s_lastTimeSyncRetryAfterFailureMs = now;
        return;
    }
    
    // 每秒尝试一次
    if (now - lastTimeSyncAttempt > TIME_SYNC_RETRY_INTERVAL) {
        lastTimeSyncAttempt = now;
        
        if (getLocalTime(&localTimeInfo)) {
            LOG_TIME_INFO("NTP Time Synced successfully!");
            timeSyncState = TIME_SYNC_SUCCESS;
            
            // 直接获取UTC时间戳
            time_t unixTimestamp = time(NULL);
            LOG_TIME_DEBUG("UNIX Timestamp (UTC): %ld", unixTimestamp);
            
            // 显示同步成功的时间
            LOG_TIME_DEBUG("Time sync took %lu ms", now - timeSyncStartTime);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localTimeInfo);
            LOG_TIME_INFO("Current time: %s", timeStr);
        } else {
            LOG_TIME_INFO("Waiting for NTP response...");
        }
    }
}

// 时间同步后台任务
void timeSyncTask(void* parameter) {
    LOG_TIME_DEBUG("Time sync background task started");

    // 可选：若调用方传入 TaskHandle_t*，则在退出前清空，便于避免重复创建任务
    TaskHandle_t* handlePtr = static_cast<TaskHandle_t*>(parameter);
    
    // 等待网络完全就绪
    vTaskDelay(pdMS_TO_TICKS(500));
    LOG_TIME_DEBUG("Starting time sync loop...");
    
    while (!shouldExitTasks) {
        const bool wifiReady = (WiFi.status() == WL_CONNECTED);
        const unsigned long now = millis();

        if (!wifiReady) {
            // 掉线后恢复到空闲，等待重连后再发起同步。
            if (timeSyncState == TIME_SYNC_IN_PROGRESS || timeSyncState == TIME_SYNC_FAILED) {
                timeSyncState = TIME_SYNC_IDLE;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (timeSyncState == TIME_SYNC_SUCCESS) {
            // 已同步后低频巡检，降低后台开销。
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        if (timeSyncState == TIME_SYNC_IDLE) {
            initNtpTimeSync();
        } else if (timeSyncState == TIME_SYNC_IN_PROGRESS) {
            updateTimeSync();
        } else if (timeSyncState == TIME_SYNC_FAILED) {
            // 失败后退避重试，保证“自动同步”可持续恢复。
            if (now - s_lastTimeSyncRetryAfterFailureMs >= TIME_SYNC_RETRY_AFTER_FAILURE_MS) {
                LOG_TIME_WARN("Retrying NTP sync after previous failure...");
                timeSyncState = TIME_SYNC_IDLE;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    if (shouldExitTasks) {
        LOG_TIME_INFO("Time sync task exiting due to sleep request");
    } 

    if (handlePtr != nullptr) {
        *handlePtr = nullptr;
    }
    vTaskDelete(NULL);
}

timeval getRtcTime(){
    struct timeval now;
    gettimeofday(&now, NULL); // 获取当前的 RTC 时间
    return now;
}