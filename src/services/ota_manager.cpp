#include "services/ota_manager.h"

namespace {
int g_otaProgress = 0;
String g_otaLastError = "";
volatile OTAStatus g_otaCurrentStatus = OTA_IDLE;
volatile OTAResult g_otaCurrentResult = OTA_IN_PROGRESS;

bool otaDownloadFirmware(HTTPClient& http, size_t contentLength) {
    WiFiClient* stream = http.getStreamPtr();       //使用传入的http客户端获取流
    
    if (!stream) {
        g_otaLastError = "Stream pointer is null";
        LOG_SYSTEM_ERROR("OTA: %s", g_otaLastError.c_str());
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
                g_otaLastError = "Write failed at " + String(written);
                LOG_SYSTEM_ERROR("OTA write error: %s", g_otaLastError.c_str());
                return false;
            }
            
            written += currentSize;
            
            // 定期更新进度显示
            uint32_t now = millis();
            if (now - lastProgressTime >= PROGRESS_UPDATE_INTERVAL) {
                g_otaProgress = (written * 100) / contentLength;
                if (g_otaProgress != lastDisplayedProgress) {
                    LOG_SYSTEM_DEBUG("OTA Progress: %d%% (%d/%d bytes)", 
                                    g_otaProgress, written, contentLength);
                    lcdText("Updating: " + String(g_otaProgress) + "%", 1);
                    lcdText("" + String(written/1024) + "/" + String(contentLength/1024) + " KB", 2);
                    lastDisplayedProgress = g_otaProgress;
                }
                lastProgressTime = now;
            }
        } else {
            vTaskDelay(1);
        }
    }
    
    // 验证下载完整性
    if (written != contentLength) {
        g_otaLastError = "Download incomplete: " + String(written) + "/" + String(contentLength);
        LOG_SYSTEM_ERROR("OTA: %s", g_otaLastError.c_str());
        lcdText("OTA Failed!", 1);
        lcdText("Incomplete DL", 2);
        return false;
    }
    
    LOG_SYSTEM_INFO("OTA firmware download complete: %d bytes", written);
    g_otaProgress = 100;
    return true;
}
}

void otaInit() {
    LOG_SYSTEM_INFO("OTA Manager initialized");
    g_otaCurrentStatus = OTA_IDLE;
    g_otaCurrentResult = OTA_IN_PROGRESS;
}

OTAResult otaUpdateFromURL(const String& url, bool useHTTPS) {
    g_otaProgress = 0;
    g_otaLastError = "";
    g_otaCurrentStatus = OTA_RUNNING;
    g_otaCurrentResult = OTA_IN_PROGRESS;
    
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
        g_otaLastError = "HTTP Error: " + String(httpCode);
        LOG_SYSTEM_ERROR("OTA HTTP failed: %d", httpCode);
        http.end();
        lcdText("OTA Failed!", 1);
        lcdText("HTTP Error", 2);
        updateColor(CRGB::Red);
        g_otaCurrentStatus = OTA_COMPLETED_FAILED;
        g_otaCurrentResult = OTA_FAIL_DOWNLOAD;
        return OTA_FAIL_DOWNLOAD;
    }
    
    size_t contentLength = http.getSize();
    if (contentLength == 0) {
        g_otaLastError = "Content-Length is 0";
        LOG_SYSTEM_ERROR("OTA: Invalid content length");
        lcdText("OTA Failed!", 1);
        lcdText("No Content", 2);
        http.end();
        g_otaCurrentStatus = OTA_COMPLETED_FAILED;
        g_otaCurrentResult = OTA_FAIL_DOWNLOAD;
        return OTA_FAIL_DOWNLOAD;
    }
    
    LOG_SYSTEM_INFO("Firmware size: %d bytes", contentLength);
    lcdText("Downloading...", 1);
    lcdText("Size: " + String(contentLength) + " bytes", 2);
    
    if (!Update.begin(contentLength)) {
        g_otaLastError = "Not enough space: " + String(Update.errorString());
        LOG_SYSTEM_ERROR("OTA begin failed: %s", g_otaLastError.c_str());
        lcdText("OTA Failed!", 1);
        lcdText("No Space", 2);
        http.end();
        g_otaCurrentStatus = OTA_COMPLETED_FAILED;
        g_otaCurrentResult = OTA_FAIL_WRITE;
        return OTA_FAIL_WRITE;
    }
    
    // 下载并写入固件
    bool result = otaDownloadFirmware(http, contentLength);
    http.end();
    
    if (!result) {
        LOG_SYSTEM_ERROR("OTA firmware download failed: %s", g_otaLastError.c_str());
        Update.abort();
        lcdText("OTA Failed!", 1);
        lcdText("Download Error", 2);
        updateColor(CRGB::Red);
        g_otaCurrentStatus = OTA_COMPLETED_FAILED;
        g_otaCurrentResult = OTA_FAIL_WRITE;
        return OTA_FAIL_WRITE;
    }
    
    if (Update.end(false)) {  // false = 不立即重启,先返回状态
        LOG_SYSTEM_INFO("OTA Update Success! Rebooting...");
        lcdText("OTA Success!", 1);
        lcdText("Rebooting...", 2);
        updateColor(CRGB::Green);
        g_otaProgress = 100;
        g_otaCurrentStatus = OTA_COMPLETED_SUCCESS;
        g_otaCurrentResult = OTA_SUCCESS;
        delay(2000);
        ESP.restart();
        return OTA_SUCCESS;
    } else {
        g_otaLastError = Update.getError() + ": " + String(Update.errorString());
        LOG_SYSTEM_ERROR("OTA Update failed: %s", g_otaLastError.c_str());
        Update.abort();
        lcdText("OTA Failed!", 1);
        lcdText(g_otaLastError.substring(0, 16), 2);
        updateColor(CRGB::Red);
        g_otaCurrentStatus = OTA_COMPLETED_FAILED;
        g_otaCurrentResult = OTA_FAIL_WRITE;
        return OTA_FAIL_WRITE;
    }
}

OTAResult otaUpdateFromFile(uint8_t* data, size_t length) {
    (void)data;
    (void)length;
    g_otaLastError = "OTA from file not implemented";
    g_otaCurrentStatus = OTA_COMPLETED_FAILED;
    g_otaCurrentResult = OTA_FAIL_WRITE;
    return OTA_FAIL_WRITE;
}

int otaGetProgress() {
    LOG_SYSTEM_DEBUG("Got OTA Progress: %d%%", g_otaProgress);
    return g_otaProgress;
}

String otaGetErrorString() {
    return g_otaLastError;
}

OTAStatus otaGetStatus() {
    return g_otaCurrentStatus;
}

bool otaIsInProgress() {
    return g_otaCurrentStatus == OTA_RUNNING;
}

void otaCheckForUpdate(const String& versionCheckURL) {
    // 可选: 实现版本检查逻辑
    // 从服务器获取最新版本号并比较
    (void)versionCheckURL;
}