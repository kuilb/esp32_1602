#include "config_manager.h"

bool ConfigManager::isSPIFFSInitialized = false;

ConfigManager::ConfigManager(const String& configFilePath) : configFilePath(configFilePath) {
    LOG_CONFIG_INFO("ConfigManager initialized");
    LOG_CONFIG_DEBUG("Config file path: %s", configFilePath.c_str());
};

ConfigManager::~ConfigManager(){
    LOG_CONFIG_INFO("ConfigManager destroyed");
};

bool ConfigManager::readFile(String& configContent) {
    if (!SPIFFS.exists(configFilePath)) {
        LOG_CONFIG_WARN("Config file not found: %s", configFilePath.c_str());
        lastError = Error::FileNotFound;
        listDir("/", 0); // 列出根目录以帮助调试
        return false;
    }

    File file = SPIFFS.open(configFilePath, "r");
    if (!file) {
        lastError = Error::ReadError;
        LOG_CONFIG_ERROR("Failed to open config file: %s", configFilePath.c_str());
        return false;
    }

    configContent = file.readString();
    file.close();
    LOG_CONFIG_VERBOSE("Config read: %s", configContent.c_str());
    return true;
}

bool ConfigManager::writeFile(const String& configContent) {
    if(!SPIFFS.exists(configFilePath)) {
        lastError = Error::FileNotFound;
        LOG_CONFIG_INFO("Config file does not exist, it will be created: %s", configFilePath.c_str());
        listDir("/", 0); // 列出根目录以帮助调试
    }

    File file = SPIFFS.open(configFilePath, "w");
    if (!file) {
        lastError = Error::WriteError;
        LOG_CONFIG_ERROR("Failed to open config file for writing: %s", configFilePath.c_str());
        return false;
    }

    LOG_CONFIG_VERBOSE("Writing config: %s", configContent.c_str());
    size_t written = file.print(configContent);
    file.close();

    if (written == configContent.length()) {
        LOG_CONFIG_INFO("Config saved: %s", configFilePath.c_str());
        return true;
    }

    lastError = Error::WriteError;
    LOG_CONFIG_ERROR("Failed to write complete config to file: %s", configFilePath.c_str());
    return false;
}

// 打印目录
void ConfigManager::listDir(const char* dirname, uint8_t levels) {
    LOG_SYSTEM_DEBUG("Listing directory: %s", dirname);

    File root = SPIFFS.open(dirname);
    if(!root) {
        LOG_SYSTEM_ERROR("Failed to open directory: %s", dirname);
        return;
    }
    if(!root.isDirectory()) {
        LOG_SYSTEM_ERROR("Not a directory: %s", dirname);
        return;
    }

    File file = root.openNextFile();
    while(file) {
        // 检查 file.name() 是否为空指针
        const char* fileName = file.name();
        if (!fileName) {
            LOG_SYSTEM_WARN("File has no name, skipping");
            file = root.openNextFile();
            continue;
        }
        
        if(file.isDirectory()) {
            LOG_SYSTEM_DEBUG("  DIR : %s", fileName);
            if(levels) {
                listDir(fileName, levels - 1);
            }
        } else {
            LOG_SYSTEM_DEBUG("  FILE: %s\tSIZE: %d", fileName, file.size());
        }
        file = root.openNextFile();
    } 
}

// 初始化 SPIFFS
bool ConfigManager::initSPIFFS() {
    if(isSPIFFSInitialized) {
        LOG_CONFIG_INFO("SPIFFS already initialized");
        return true;
    }

    LOG_CONFIG_INFO("Initializing SPIFFS...");
    if (!SPIFFS.begin(true)) {
        LOG_CONFIG_ERROR("SPIFFS mount failed, try formatting");
        SPIFFS.format();
        if (!SPIFFS.begin()) {
            LOG_CONFIG_ERROR("SPIFFS mount failed after format");
            return false;
        }
    }
    LOG_CONFIG_INFO("SPIFFS mounted successfully");
    listDir("/", 0);    // 打印根目录文件
    isSPIFFSInitialized = true;
    return true;
}

ConfigManager::Error ConfigManager::getLastError() const {
    return lastError;
}

String ConfigManager::getLastErrorString(Error error) const {
    switch (lastError) {
        case Error::None:
            return "No error";
        case Error::FileNotFound:
            return "File not found";
        case Error::ReadError:
            return "Read error";
        case Error::WriteError:
            return "Write error";
        default:
            return "Unknown error";
    }
}

void ConfigManager::clearLastError() {
    lastError = Error::None;
}

void ConfigManager::setLastError(Error error) {
    lastError = error;
}