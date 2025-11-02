#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

enum OTAResult {
    OTA_SUCCESS,
    OTA_FAIL_DOWNLOAD,
    OTA_FAIL_VERIFY,
    OTA_FAIL_WRITE,
    OTA_FAIL_NETWORK,
    OTA_IN_PROGRESS
};

class OTAManager {
public:
    static void init();
    static OTAResult updateFromURL(const String& url, bool useHTTPS = false);
    static OTAResult updateFromFile(uint8_t* data, size_t length);
    static int getProgress();
    static String getErrorString();
    static void checkForUpdate(const String& versionCheckURL);
    
private:
    static bool downloadFirmware(HTTPClient& http, size_t contentLength);
    static bool verifyChecksum(const String& expectedMD5);
    static int progress;
    static String lastError;
};

#endif