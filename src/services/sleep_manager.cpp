#include "./services/sleep_manager.h"

#include "driver/gpio.h"
#include "esp_task_wdt.h"

// 任务退出标志
volatile bool shouldExitTasks = false;

void initSleepManager() {
    shouldExitTasks = false;

    // 如果上次进入深睡时启用了 GPIO hold，唤醒后需要显式释放
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)BUTTON_POWER_PIN);
    gpio_pulldown_en((gpio_num_t)BUTTON_POWER_PIN);
    
    LOG_SYSTEM_DEBUG("Sleep manager initialized");
}

void enterDeepSleep() {
    lcdText("Entering Sleep", 1);
    lcdText("", 2);

    LOG_SLEEP_INFO("Preparing to enter deep sleep...");
    
    // 设置任务退出标志
    LOG_SLEEP_DEBUG("Signaling tasks to exit...");
    shouldExitTasks = true;
    
    // 等待其他任务退出（最多等待500ms）
    int waitCount = 0;
    while (waitCount < 50) {
        if (handleTaskHandle == NULL && _menuTaskHandle == NULL) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        waitCount++;
    }
    LOG_SLEEP_DEBUG("Tasks exit wait completed (%d ms)", waitCount * 10);

    lcdText("Sleep...", 1);
    lcdText("", 2);
    
    // 断开所有网络连接（加锁，避免与 loopTask/read 或按键任务/print 并发）
    initNetwork();
    if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(250)) == pdTRUE) {
        if (client.connected()) {
            LOG_SLEEP_DEBUG("Disconnecting TCP client...");
            client.stop();
        }
        clientConnected = false;

        // 停止TCP服务器
        LOG_SLEEP_DEBUG("Stopping TCP server...");
        server.end();

        xSemaphoreGive(clientMutex);
    } else {
        // 获取锁失败时也继续深睡流程，避免卡死；此时 loopTask 已被 shouldExitTasks 挡住
        LOG_SLEEP_WARN("Failed to acquire network mutex; continuing sleep shutdown.");
        if (clientConnected) {
            clientConnected = false;
        }
        server.end();
    }
    
    // 断开WiFi连接
    if (WiFi.status() == WL_CONNECTED) {
        LOG_SLEEP_DEBUG("Disconnecting WiFi...");
        WiFi.disconnect(true);  // true = 关闭WiFi射频
        WiFi.mode(WIFI_OFF);
        delay(100);  // 等待WiFi完全断开
    }
    
    // 渐暗背光
    for (int i = 0; i < 256; i++) {
        changeBrightness(-10);
        delay(10);
    }
    updateBrightness(0);

    // 保存当前时间戳和RTC计数器到 RTC 内存
    gettimeofday(&sleep_enter_time, NULL);
    sleep_enter_rtc_time = rtc_time_get();  // 保存RTC计数器值
    
    // 确保RTC时钟源在深度睡眠期间保持运行
    rtc_clk_32k_enable(true);
    rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
    
    // 配置RTC域在深度睡眠期间保持供电
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    
    LOG_SLEEP_DEBUG("Sleep enter - System time: %ld.%06ld, RTC ticks: %llu", 
                    sleep_enter_time.tv_sec, sleep_enter_time.tv_usec, sleep_enter_rtc_time);
    
    // 先释放GPIO hold并配置下拉
    gpio_hold_dis((gpio_num_t)BUTTON_POWER_PIN);  // 释放可能的锁定状态
    gpio_pulldown_en((gpio_num_t)BUTTON_POWER_PIN); // 启用内部下拉
    delay(10);  // 等待下拉稳定
    
    // 检查电源键状态，确保为低电平（未按下）
    int pin_state = gpio_get_level((gpio_num_t)BUTTON_POWER_PIN);
    LOG_SLEEP_DEBUG("BUTTON_POWER_PIN state before sleep: %d", pin_state);
    
    if (pin_state == 1) {
        LOG_SLEEP_WARN("Button is pressed, waiting for release...");
        // 等待按钮释放
        while (gpio_get_level((gpio_num_t)BUTTON_POWER_PIN) == 1) {
            delay(50);
        }
        delay(100);  // 去抖动
        LOG_SLEEP_DEBUG("Button released");
    }
    
    // 配置 BUTTON_POWER_PIN 为唤醒源（高电平按下唤醒）
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_POWER_PIN, HIGH);  // 高电平唤醒

    // 在深度睡眠期间保持GPIO配置
    gpio_hold_en((gpio_num_t)BUTTON_POWER_PIN);   // 锁定配置

    LOG_SLEEP_INFO("Entering deep sleep...");
    delay(100);  // 确保日志输出完成和所有任务停止
    
    // 刷新串口缓冲区
    Serial.flush();
    
    esp_deep_sleep_start();  // 进入深度睡眠
}

bool isWakeupFromDeepSleep() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    return (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0);
}
