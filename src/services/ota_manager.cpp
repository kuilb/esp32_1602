#include "services/ota_manager.h"
#include "utils/logger.h"
#include "hardware/lcd_driver.h"
#include "hardware/rgb_led.h"

int OTAManager::progress = 0;
String OTAManager::lastError = "";

void OTAManager::init() {
    LOG_SYSTEM_INFO("OTA Manager initialized");
}

OTAResult OTAManager::updateFromURL(const String& url, bool useHTTPS) {
    progress = 0;
    lastError = "";
    
    WiFiClient* client;
    WiFiClientSecure secureClient;
    
    if (useHTTPS) {
        secureClient.setInsecure(); // 跳过证书验证,或使用setCACert()
        client = &secureClient;
    } else {
        static WiFiClient normalClient;
        client = &normalClient;
    }
    
    HTTPClient http;
    http.begin(*client, url);
    
    LOG_SYSTEM_INFO("Starting OTA from URL: %s", url.c_str());
    lcd_text("OTA Starting...", 1);
    updateColor(CRGB::Orange);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        lastError = "HTTP Error: " + String(httpCode);
        LOG_SYSTEM_ERROR("OTA HTTP failed: %d", httpCode);
        http.end();
        return OTA_FAIL_DOWNLOAD;
    }
    
    size_t contentLength = http.getSize();
    if (contentLength == 0) {
        lastError = "Content-Length is 0";
        LOG_SYSTEM_ERROR("OTA: Invalid content length");
        http.end();
        return OTA_FAIL_DOWNLOAD;
    }
    
    LOG_SYSTEM_INFO("Firmware size: %d bytes", contentLength);
    
    if (!Update.begin(contentLength)) {
        lastError = "Not enough space: " + String(Update.errorString());
        LOG_SYSTEM_ERROR("OTA begin failed: %s", lastError.c_str());
        http.end();
        return OTA_FAIL_WRITE;
    }
    
    // 下载并写入固件
    bool result = downloadFirmware(http, contentLength);
    http.end();
    
    if (!result) {
        Update.abort();
        return OTA_FAIL_WRITE;
    }
    
    if (Update.end(true)) {
        LOG_SYSTEM_INFO("OTA Update Success! Rebooting...");
        lcd_text("OTA Success!", 1);
        lcd_text("Rebooting...", 2);
        updateColor(CRGB::Green);
        delay(2000);
        ESP.restart();
        return OTA_SUCCESS;
    } else {
        lastError = Update.errorString();
        LOG_SYSTEM_ERROR("OTA Update failed: %s", lastError.c_str());
        return OTA_FAIL_VERIFY;
    }
}

bool OTAManager::downloadFirmware(HTTPClient& http, size_t contentLength) {
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buff[512];
    size_t written = 0;
    
    while (http.connected() && written < contentLength) {
        size_t available = stream->available();
        if (available) {
            int c = stream->readBytes(buff, min(available, sizeof(buff)));
            
            if (Update.write(buff, c) != c) {
                lastError = "Write failed at " + String(written);
                LOG_SYSTEM_ERROR("OTA write error: %s", lastError.c_str());
                return false;
            }
            
            written += c;
            progress = (written * 100) / contentLength;
            
            // 每10%更新一次显示
            if (progress % 10 == 0) {
                LOG_SYSTEM_DEBUG("OTA Progress: %d%%", progress);
                lcd_text("Updating: " + String(progress) + "%", 2);
            }
        }
        delay(1);
    }
    
    return (written == contentLength);
}

int OTAManager::getProgress() {
    return progress;
}

String OTAManager::getErrorString() {
    return lastError;
}

void OTAManager::checkForUpdate(const String& versionCheckURL) {
    // 可选: 实现版本检查逻辑
    // 从服务器获取最新版本号并比较
}