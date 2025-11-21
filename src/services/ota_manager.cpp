#include "services/ota_manager.h"
#include "logger.h"
#include "lcd_driver.h"
#include "rgb_led.h"

int OTAManager::progress = 0;
String OTAManager::lastError = "";
volatile OTAStatus OTAManager::currentStatus = OTA_IDLE;
volatile OTAResult OTAManager::currentResult = OTA_IN_PROGRESS;

OTAManager::OTAManager() {
    LOG_SYSTEM_INFO("OTA Manager initialized");
    currentStatus = OTA_IDLE;
    currentResult = OTA_IN_PROGRESS;
}

OTAManager::~OTAManager() {
    LOG_SYSTEM_INFO("OTA Manager destroyed");
}

OTAResult OTAManager::updateFromURL(const String& url, bool useHTTPS) {
    progress = 0;
    lastError = "";
    currentStatus = OTA_RUNNING;
    currentResult = OTA_IN_PROGRESS;
    
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
    lcdText("OTA Starting...", 1);
    lcdText("Connecting...", 2);
    updateColor(CRGB::Orange);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        lastError = "HTTP Error: " + String(httpCode);
        LOG_SYSTEM_ERROR("OTA HTTP failed: %d", httpCode);
        http.end();
        lcdText("OTA Failed!", 1);
        lcdText("HTTP Error", 2);
        updateColor(CRGB::Red);
        currentStatus = OTA_COMPLETED_FAILED;
        currentResult = OTA_FAIL_DOWNLOAD;
        return OTA_FAIL_DOWNLOAD;
    }
    
    size_t contentLength = http.getSize();
    if (contentLength == 0) {
        lastError = "Content-Length is 0";
        LOG_SYSTEM_ERROR("OTA: Invalid content length");
        lcdText("OTA Failed!", 1);
        lcdText("No Content", 2);
        http.end();
        currentStatus = OTA_COMPLETED_FAILED;
        currentResult = OTA_FAIL_DOWNLOAD;
        return OTA_FAIL_DOWNLOAD;
    }
    
    LOG_SYSTEM_INFO("Firmware size: %d bytes", contentLength);
    lcdText("Downloading...", 1);
    lcdText("Size: " + String(contentLength) + " bytes", 2);
    
    if (!Update.begin(contentLength)) {
        lastError = "Not enough space: " + String(Update.errorString());
        LOG_SYSTEM_ERROR("OTA begin failed: %s", lastError.c_str());
        lcdText("OTA Failed!", 1);
        lcdText("No Space", 2);
        http.end();
        currentStatus = OTA_COMPLETED_FAILED;
        currentResult = OTA_FAIL_WRITE;
        return OTA_FAIL_WRITE;
    }
    
    // 下载并写入固件
    bool result = downloadFirmware(http, contentLength);
    http.end();
    
    if (!result) {
        LOG_SYSTEM_ERROR("OTA firmware download failed: %s", lastError.c_str());
        Update.abort();
        lcdText("OTA Failed!", 1);
        lcdText("Download Error", 2);
        updateColor(CRGB::Red);
        currentStatus = OTA_COMPLETED_FAILED;
        currentResult = OTA_FAIL_WRITE;
        return OTA_FAIL_WRITE;
    }
    
    if (Update.end(false)) {  // false = 不立即重启,先返回状态
        LOG_SYSTEM_INFO("OTA Update Success! Rebooting...");
        lcdText("OTA Success!", 1);
        lcdText("Rebooting...", 2);
        updateColor(CRGB::Green);
        progress = 100;
        currentStatus = OTA_COMPLETED_SUCCESS;
        currentResult = OTA_SUCCESS;
        delay(2000);
        ESP.restart();
        return OTA_SUCCESS;
    } else {
        lastError = Update.getError() + ": " + String(Update.errorString());
        LOG_SYSTEM_ERROR("OTA Update failed: %s", lastError.c_str());
        Update.abort();
        lcdText("OTA Failed!", 1);
        lcdText(lastError.substring(0, 16), 2);
        updateColor(CRGB::Red);
        currentStatus = OTA_COMPLETED_FAILED;
        currentResult = OTA_FAIL_WRITE;
        return OTA_FAIL_WRITE;
    }
}

bool OTAManager::downloadFirmware(HTTPClient& http, size_t contentLength) {
    WiFiClient* stream = http.getStreamPtr();       //使用传入的http客户端获取流
    
    if (!stream) {
        lastError = "Stream pointer is null";
        LOG_SYSTEM_ERROR("OTA: %s", lastError.c_str());
        return false;
    }
    
    uint8_t buff[512];
    size_t written = 0;
    int lastDisplayedProgress = -1;
    uint32_t lastProgressTime = millis();
    const uint32_t PROGRESS_UPDATE_INTERVAL = 50;
    
    while (http.connected() && written < contentLength) {
        size_t available = stream->available();
        if (available) {
            // 写入流数据到缓冲区
            int currentSize = stream->readBytes(buff, min(available, sizeof(buff)));
            
            if (currentSize <= 0) {       // 未找到有效数据，等待流
                vTaskDelay(1);
                continue;
            }
            
            // 写入缓冲区数据到Flash
            if (Update.write(buff, currentSize) != currentSize) {
                lastError = "Write failed at " + String(written);
                LOG_SYSTEM_ERROR("OTA write error: %s", lastError.c_str());
                return false;
            }
            
            written += currentSize;
            
            // 定期更新进度显示
            uint32_t now = millis();
            if (now - lastProgressTime >= PROGRESS_UPDATE_INTERVAL) {
                progress = (written * 100) / contentLength;
                if (progress != lastDisplayedProgress) {
                    LOG_SYSTEM_DEBUG("OTA Progress: %d%% (%d/%d bytes)", 
                                    progress, written, contentLength);
                    lcdText("Updating: " + String(progress) + "%", 1);
                    lcdText("" + String(written/1024) + "/" + String(contentLength/1024) + " KB", 2);
                    lastDisplayedProgress = progress;
                }
                lastProgressTime = now;
            }
        } else {
            vTaskDelay(1);
        }
    }
    
    // 验证下载完整性
    if (written != contentLength) {
        lastError = "Download incomplete: " + String(written) + "/" + String(contentLength);
        LOG_SYSTEM_ERROR("OTA: %s", lastError.c_str());
        lcdText("OTA Failed!", 1);
        lcdText("Incomplete DL", 2);
        return false;
    }
    
    LOG_SYSTEM_INFO("OTA firmware download complete: %d bytes", written);
    progress = 100;
    return true;
}

int OTAManager::getProgress() {
    LOG_SYSTEM_DEBUG("Got OTA Progress: %d%%", progress);
    return progress;
}

String OTAManager::getErrorString() {
    return lastError;
}

OTAStatus OTAManager::getStatus() {
    return currentStatus;
}

bool OTAManager::isInProgress() {
    return currentStatus == OTA_RUNNING;
}

void OTAManager::checkForUpdate(const String& versionCheckURL) {
    // 可选: 实现版本检查逻辑
    // 从服务器获取最新版本号并比较
}