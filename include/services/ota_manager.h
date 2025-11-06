#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "myheader.h"
#include <Update.h>
#include <WiFiClientSecure.h>

enum OTAResult {
    OTA_SUCCESS,
    OTA_FAIL_DOWNLOAD,
    OTA_FAIL_VERIFY,    // 待实现
    OTA_FAIL_WRITE,
    OTA_FAIL_NETWORK,
    OTA_IN_PROGRESS
};

enum OTAStatus {
    OTA_IDLE,
    OTA_RUNNING,
    OTA_COMPLETED_SUCCESS,
    OTA_COMPLETED_FAILED
};

class OTAManager {
public:
    OTAManager();
    ~OTAManager();
    static OTAResult updateFromURL(const String& url, bool useHTTPS = false);
    static OTAResult updateFromFile(uint8_t* data, size_t length);
    static int getProgress();
    static String getErrorString();
    static void checkForUpdate(const String& versionCheckURL);
    static OTAStatus getStatus();
    static bool isInProgress();
    
private:
    static bool downloadFirmware(HTTPClient& http, size_t contentLength);
    static bool verifyChecksum(const String& expectedMD5);
    static int progress;
    static String lastError;
    static volatile OTAStatus currentStatus;
    static volatile OTAResult currentResult;
};

#endif