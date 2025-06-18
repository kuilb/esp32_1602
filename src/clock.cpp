#include "clock.h"

bool timeSynced = false;

void setupTime() {
    configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp1.aliyun.com", "ntp.ntsc.ac.cn");
    Serial.println("Waiting for NTP time sync...");

    lcd_text("Waiting for", 1);
    lcd_text("NTP time sync...", 2);

    static unsigned long lastAttempt = 0;
    if (timeSynced) return;

    unsigned long now = millis();
    if (now - lastAttempt > 1000) {
        lastAttempt = now;          // 尝试时间点放这里，保证每秒尝试一次
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            Serial.println("NTP Time Synced!");
            timeSynced = true;
        } 
        else {
            Serial.println("NTP Sync Failed.");
        }
    }
}



void updateClockScreen() {
    if (timeSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            char timeBuf[17];
            char dateBuf[17];

            // 格式化时间，例如 "14:23:08"
            strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
            
            // 格式化日期和星期，例如 "06/17 Tue"
            strftime(dateBuf, sizeof(dateBuf), "%m/%d %a", &timeinfo);

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
