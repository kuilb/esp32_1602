#include "qweather_auth_config_manager.h"

QWeatherAuthConfigManager::QWeatherAuthConfigManager(const String& configFilePath) 
    : ConfigManager(configFilePath), 
    apiHost(""), kId(""), projectID(""), base64Key(""), location(""), cityName("") 
    {
    LOG_CONFIG_INFO("QWeatherAuthConfigManager initialized with config file: %s", configFilePath.c_str());
}

QWeatherAuthConfigManager::~QWeatherAuthConfigManager() {
    LOG_CONFIG_INFO("QWeatherAuthConfigManager destroyed");
}

String QWeatherAuthConfigManager::getApiHost() {return apiHost;}
String QWeatherAuthConfigManager::getKId() {return kId;}
String QWeatherAuthConfigManager::getProjectID() {return projectID;}
String QWeatherAuthConfigManager::getBase64Key() {return base64Key;}
String QWeatherAuthConfigManager::getLocation() {return location;}
String QWeatherAuthConfigManager::getCityName() {return cityName;}

bool QWeatherAuthConfigManager::init() {
    LOG_CONFIG_INFO("QWeatherAuthConfigManager init called");
    if(!loadConfig()){
        if(lastError == Error::FileNotFound){
            LOG_CONFIG_WARN("Config file not found, creating default config");
            if(saveConfig()) {
                setLastQWeatherError(QWeatherError::None);
                return true;
            }
            setLastQWeatherError(QWeatherError::ConfigFileError);
            return false;
        }
        else{
            LOG_CONFIG_ERROR("Failed to load QWeather auth config with error: %s", getLastErrorString(lastError).c_str());
            setLastQWeatherError(QWeatherError::ConfigFileError);
            return false;
        }
    }
    return true;
}

bool QWeatherAuthConfigManager::loadConfig() {
    LOG_CONFIG_INFO("Loading QWeather auth config from file: %s", configFilePath.c_str());
    String configContent;
    if (!readFile(configContent)) {
        LOG_CONFIG_WARN("Failed to read QWeather auth config file");
        setLastQWeatherError(QWeatherError::ConfigFileError);
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configContent);
    if (error) {
        LOG_CONFIG_ERROR("Failed to parse QWeather auth config JSON: %s", error.c_str());
        setLastQWeatherError(QWeatherError::ConfigFileError);
        return false;
    }

    apiHost = doc["apiHost"].as<String>();
    kId = doc["kId"].as<String>();
    projectID = doc["projectID"].as<String>();
    base64Key = doc["base64Key"].as<String>();
    location = doc["location"].as<String>();
    cityName = doc["cityName"].as<String>();

    return true;
}

bool QWeatherAuthConfigManager::saveConfig() {
    LOG_CONFIG_INFO("Saving QWeather auth config to file: %s", configFilePath.c_str());
    JsonDocument doc;
    doc["apiHost"] = apiHost;
    doc["kId"] = kId;
    doc["projectID"] = projectID;
    doc["base64Key"] = base64Key;
    doc["location"] = location;
    doc["cityName"] = cityName;

    String jsonString;
    serializeJson(doc, jsonString);

    if(writeFile(jsonString)){
        LOG_CONFIG_INFO("QWeather auth config saved successfully: %s", jsonString.c_str());
        return true;
    }
    else{
        LOG_CONFIG_ERROR("Failed to write QWeather auth config to file");
        setLastQWeatherError(QWeatherError::ConfigFileError);
        return false;
    }   
}

bool QWeatherAuthConfigManager::resetConfig() {
    LOG_CONFIG_INFO("Resetting QWeather auth config to defaults");
    apiHost = "";
    kId = "";
    projectID = "";
    base64Key = "";
    location = "";
    cityName = "";

    if(saveConfig()) return true;
    return false;
}

bool QWeatherAuthConfigManager::setAuth(String apiHost, String kId, String projectID, String base64Key) {
    this->apiHost = apiHost;
    this->kId = kId;
    this->projectID = projectID;
    this->base64Key = base64Key;

    if(this->apiHost.isEmpty() || this->kId.isEmpty() || this->projectID.isEmpty() || this->base64Key.isEmpty()){
        LOG_CONFIG_WARN("One or more auth parameters are empty");
        setLastQWeatherError(QWeatherError::EmptyArguments);
        return false;
    }

    this->apiHost.trim();
    this->kId.trim();
    this->projectID.trim();
    this->base64Key.trim();

    auto isValidApiHost = [](const String& host)->bool{
        // 简单检测：必须包含点且不含空格
        if (host.indexOf(' ') >= 0) return false;
        if (host.indexOf('.') <= 0) return false;
        return true;
    };

    if(isValidApiHost(this->apiHost) == false){
        LOG_CONFIG_WARN("invalid apiHost format");
        setLastQWeatherError(QWeatherError::InvalidApiHost);
        return false;
    }

    auto isValidID = [](const String& s)->bool{
        if (s.length() != 10) return false;
        for (size_t i = 0; i < s.length(); ++i) {
            char c = s.charAt(i);
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) return false;
        }
        return true;
    };

    if(!isValidID(this->kId)){
        LOG_CONFIG_WARN("invalid kId format");
        setLastQWeatherError(QWeatherError::InvalidKid);
        return false;
    }

    if(!isValidID(this->projectID)){
        LOG_CONFIG_WARN("invalid projectID format");
        setLastQWeatherError(QWeatherError::InvalidProjectID);
        return false;
    }

    return saveConfig();
}

bool QWeatherAuthConfigManager::setLocation(String location, String cityName) {
    auto isValidLocation = [](const String& loc)->bool{
        for(size_t i = 0; i < loc.length(); ++i){
            char c = loc.charAt(i);
            if(!(c >= '0' && c <= '9')){
                return false;
            }
        }
        return true;
    };
    if(!isValidLocation(location)){
        LOG_CONFIG_WARN("invalid location format");
        setLastQWeatherError(QWeatherError::InvalidLocation);
        return false;
    }

    auto isValidCityName = [](const String& name)->bool{
        for(size_t i = 0; i < name.length(); ++i){
            char c = name.charAt(i);
            if(!( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == ' ') )){
                return false;
            }
        }
        return true;
    };
    if(!isValidCityName(cityName)){
        LOG_CONFIG_WARN("invalid cityName format");
        setLastQWeatherError(QWeatherError::InvalidCityName);
        return false;
    }

    this->location = location;
    this->cityName = cityName;
    return saveConfig();
}

bool QWeatherAuthConfigManager::checkApiConfigValid() {
    if (apiHost.isEmpty() || kId.isEmpty() || projectID.isEmpty() || base64Key.isEmpty()) {
        setLastQWeatherError(QWeatherError::EmptyArguments);
        return false;
    }
    setLastQWeatherError(QWeatherError::None);
    return true;
}

bool QWeatherAuthConfigManager::checkLocationConfigValid() {
    if (location.isEmpty()) {
        setLastQWeatherError(QWeatherError::EmptyArguments);
        return false;
    }
    setLastQWeatherError(QWeatherError::None);
    return true;
}

QWeatherAuthConfigManager::QWeatherError QWeatherAuthConfigManager::getLastQWeatherError() {
    return lastQWeatherError;
}

void QWeatherAuthConfigManager::setLastQWeatherError(QWeatherError error) {
    lastQWeatherError = error;
}

String QWeatherAuthConfigManager::getLastQWeatherErrorString() {
    switch (lastQWeatherError) {
        case QWeatherError::None:
            return "No Error";
        case QWeatherError::InvalidApiHost:
            return "Invalid API Host";
        case QWeatherError::InvalidKid:
            return "Invalid Key ID";
        case QWeatherError::InvalidProjectID:
            return "Invalid Project ID";
        case QWeatherError::InvalidBase64Key:
            return "Invalid Base64 Key";
        case QWeatherError::EmptyArguments:
            return "One or more arguments are empty";
        case QWeatherError::ConfigFileError:
            return "Configuration file error";
        default:
            return "Unknown Error";
    }
}