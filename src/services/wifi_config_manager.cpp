#include "./services/wifi_config_manager.h"

WifiConfigManager::WifiConfigManager(const String& configFilePath) : ConfigManager(configFilePath), ssid(""), password("") {
    LOG_CONFIG_INFO("WifiConfigManager initialized with config file: %s", configFilePath.c_str());
}

WifiConfigManager::~WifiConfigManager() {
    LOG_CONFIG_INFO("WifiConfigManager destroyed");
}

bool WifiConfigManager::init() {
    if(!loadConfig()){
        if(lastError == Error::FileNotFound){
            LOG_CONFIG_WARN("Config file not found, creating default config");
            if(saveConfig()) return true;
            return false;
        }
        else if(lastError == Error::InvalidData){
            // 配置为空或无效,但配置管理器本身工作正常,返回 true
            // WiFi 连接层会检查 SSID 是否为空并适当处理
            LOG_CONFIG_INFO("WiFi config is empty, waiting for user configuration via web interface");
            return true;
        }
        else{
            LOG_CONFIG_ERROR("Failed to load WiFi config with error: %s", getLastErrorString(lastError).c_str());
            return false;
        }
    }
    return true;  // 配置加载成功
}

bool WifiConfigManager::loadConfig() {
    LOG_CONFIG_DEBUG("Loading WiFi config from file: %s", configFilePath.c_str());
    String configContent;
    if (!readFile(configContent)) {
        LOG_CONFIG_WARN("Failed to read WiFi config file");
        return false;
    }

    if (configContent.length() == 0) {
        LOG_CONFIG_WARN("WiFi config file is empty");
        setLastError(Error::InvalidData);  // 文件存在但为空,属于无效数据
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configContent);
    if (error) {
        LOG_CONFIG_ERROR("Failed to parse WiFi config JSON: %s", error.c_str());
        setLastError(Error::InvalidData);  // JSON 解析失败,属于无效数据
        return false;
    }

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();

    // 验证 SSID 不为空(password 可以为空,用于开放网络)
    if(ssid.length() == 0) {
        LOG_CONFIG_WARN("WiFi config has empty SSID, waiting for user configuration");
        setLastError(Error::InvalidData);  // SSID 为空,属于无效数据而非文件不存在
        return false;
    }

    LOG_CONFIG_DEBUG("WiFi config loaded successfully - SSID: %s", ssid.c_str());
    return true;
}

bool WifiConfigManager::saveConfig() {
    LOG_CONFIG_DEBUG("Saving WiFi config to file: %s", configFilePath.c_str());
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    String jsonString;
    serializeJson(doc, jsonString);

    if(writeFile(jsonString)){
        LOG_CONFIG_INFO("WiFi config saved successfully");
        LOG_CONFIG_DEBUG("WiFi config content: %s", jsonString.c_str());
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