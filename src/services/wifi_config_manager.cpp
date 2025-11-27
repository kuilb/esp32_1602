#include "wifi_config_manager.h"

WifiConfigManager::WifiConfigManager(const String& configFilePath) : ConfigManager(configFilePath), ssid(""), password("") {
    LOG_CONFIG_INFO("WifiConfigManager initialized with config file: %s", configFilePath.c_str());
}

WifiConfigManager::~WifiConfigManager() {
    LOG_CONFIG_INFO("WifiConfigManager destroyed");
}

bool WifiConfigManager::init() {
    LOG_CONFIG_INFO("WifiConfigManager init called");
    if(!loadConfig()){
        if(lastError == Error::FileNotFound){
            LOG_CONFIG_WARN("Config file not found, creating default config");
            if(saveConfig()) return true;
            return false;
        }
        else{
            LOG_CONFIG_ERROR("Failed to load WiFi config with error: %s", getLastErrorString(lastError).c_str());
            return false;
        }
    }
    return true;  // 配置加载成功
}

bool WifiConfigManager::loadConfig() {
    LOG_CONFIG_INFO("Loading WiFi config from file: %s", configFilePath.c_str());
    String configContent;
    if (!readFile(configContent)) {
        LOG_CONFIG_WARN("Failed to read WiFi config file");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configContent);
    if (error) {
        LOG_CONFIG_ERROR("Failed to parse WiFi config JSON: %s", error.c_str());
        return false;
    }

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();

    // 验证 SSID 不为空(password 可以为空,用于开放网络)
    if(ssid.length() == 0) {
        LOG_CONFIG_WARN("WiFi config has empty SSID, treating as invalid config");
        setLastError(Error::FileNotFound);  // 设置为文件未找到,触发创建默认配置
        return false;
    }

    LOG_CONFIG_INFO("WiFi config loaded successfully - SSID: %s", ssid.c_str());
    return true;
}

bool WifiConfigManager::saveConfig() {
    LOG_CONFIG_INFO("Saving WiFi config to file: %s", configFilePath.c_str());
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    String jsonString;
    serializeJson(doc, jsonString);

    if(writeFile(jsonString)){
        LOG_CONFIG_INFO("WiFi config saved successfully" +  jsonString);
        return true;
    }
    else{
        LOG_CONFIG_ERROR("Failed to write WiFi config to file");
        return false;
    }   
}

bool WifiConfigManager::resetConfig() {
    LOG_CONFIG_INFO("Resetting WiFi config to defaults");
    ssid = "";
    password = "";

    if(saveConfig()) return true;;
    return false;
}

String WifiConfigManager::getSSID() {
    return ssid;
}

String WifiConfigManager::getPassword() {
    return password;
}

bool WifiConfigManager::setSSID(const String& newSsid) {
    if(newSsid.length() > 32 || newSsid.length() == 0) {
        LOG_CONFIG_ERROR("SSID length invalid: %d", newSsid.length());
        return false;
    }

    ssid = newSsid;
    LOG_CONFIG_INFO("WiFi SSID set to: %s", ssid.c_str());

    if(saveConfig()) return true;
    return false;
}

bool WifiConfigManager::setPassword(const String& newPassword) {
    if(newPassword.length() > 63 || (newPassword.length() < 8 && newPassword.length() != 0)) {
        LOG_CONFIG_ERROR("Password length invalid: %d", newPassword.length());
        return false;
    }
    password = newPassword;
    LOG_CONFIG_INFO("WiFi password set to: %s", password.c_str());

    if(saveConfig()) return true;;
    return false;
}