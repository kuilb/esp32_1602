#include "./services/config_manager.h"

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

    size_t fileSize = file.size();
    LOG_CONFIG_DEBUG("Reading file: %s, size: %d bytes", configFilePath.c_str(), fileSize);
    
    if (fileSize == 0) {
        LOG_CONFIG_WARN("Config file is empty: %s", configFilePath.c_str());
        file.close();
        configContent = "";
        lastError = Error::ReadError;
        return false;
    }

    // 尝试读取文件内容
    configContent = "";
    configContent.reserve(fileSize + 1);
    
    // 方法1: 使用readString
    configContent = file.readString();
    
    // 如果readString失败,尝试逐字节读取
    if (configContent.length() == 0) {
        LOG_CONFIG_WARN("readString failed, trying byte-by-byte read");
        file.seek(0);  // 重置文件指针
        
        char buffer[fileSize + 1];
        size_t bytesRead = file.readBytes(buffer, fileSize);
        buffer[bytesRead] = '\0';
        configContent = String(buffer);
        
        LOG_CONFIG_DEBUG("Byte-by-byte read: %d bytes", bytesRead);
    }
    
    file.close();
    LOG_CONFIG_VERBOSE("Config read (%d bytes): %s", configContent.length(), configContent.c_str());
    
    if (configContent.length() == 0) {
        LOG_CONFIG_ERROR("Failed to read content from file: %s", configFilePath.c_str());
        lastError = Error::ReadError;
        return false;
    }
    
    return true;
}

bool ConfigManager::writeFile(const String& configContent) {
    LOG_CONFIG_VERBOSE("Writing config (%d bytes): %s", configContent.length(), configContent.c_str());
    
    // 删除旧文件(如果存在)
    if(SPIFFS.exists(configFilePath)) {
        LOG_CONFIG_DEBUG("Removing existing file: %s", configFilePath.c_str());
        SPIFFS.remove(configFilePath);
        delay(50);  // 等待Flash完成删除操作
    }

    File file = SPIFFS.open(configFilePath, "w", true);  // create if not exists
    if (!file) {
        lastError = Error::WriteError;
        LOG_CONFIG_ERROR("Failed to open config file for writing: %s", configFilePath.c_str());
        return false;
    }

    size_t written = file.print(configContent);
    file.flush();  // 确保数据完全写入Flash
    file.close();
    
    delay(100);  // 等待Flash完成写入操作

    if (written != configContent.length()) {
        lastError = Error::WriteError;
        LOG_CONFIG_ERROR("Failed to write complete config to file: %s (wrote %d/%d bytes)", 
                        configFilePath.c_str(), written, configContent.length());
        return false;
    }
    
    // 验证写入
    if (!SPIFFS.exists(configFilePath)) {
        lastError = Error::WriteError;
        LOG_CONFIG_ERROR("File verification failed: file not found after write: %s", configFilePath.c_str());
        return false;
    }
    
    // 验证文件大小
    File verifyFile = SPIFFS.open(configFilePath, "r");
    if (verifyFile) {
        size_t actualSize = verifyFile.size();
        verifyFile.close();
        if (actualSize != configContent.length()) {
            lastError = Error::WriteError;
            LOG_CONFIG_ERROR("File size mismatch: expected %d, got %d", configContent.length(), actualSize);
            return false;
        }
    }
    
    LOG_CONFIG_INFO("Config saved successfully: %s (%d bytes)", configFilePath.c_str(), written);
    return true;
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
        root.close();
        return;
    }

    File file = root.openNextFile();
    while(file) {
        const char* fileName = file.name();
        if (!fileName) {
            LOG_SYSTEM_WARN("File has no name, skipping");
            file.close();
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
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

// 初始化 SPIFFS
bool ConfigManager::initSPIFFS() {
    if(isSPIFFSInitialized) {
        LOG_CONFIG_INFO("SPIFFS already initialized");
        return true;
    }

    LOG_CONFIG_INFO("Initializing SPIFFS...");
    
    // 先尝试正常挂载(不自动格式化)
    bool mounted = SPIFFS.begin(false);
    
    if (!mounted) {
        LOG_CONFIG_WARN("SPIFFS mount failed, formatting...");
        // 如果挂载失败,进行格式化
        if (!SPIFFS.format()) {
            LOG_CONFIG_ERROR("SPIFFS format failed");
            return false;
        }
        
        delay(100);  // 等待Flash完成格式化
        
        // 重新挂载
        if (!SPIFFS.begin(false)) {
            LOG_CONFIG_ERROR("SPIFFS mount failed after format");
            return false;
        }
    }
    
    LOG_CONFIG_INFO("SPIFFS mounted successfully");
    LOG_CONFIG_INFO("Total: %d bytes, Used: %d bytes", SPIFFS.totalBytes(), SPIFFS.usedBytes());
    
    // 打印文件列表
    LOG_CONFIG_INFO("Checking SPIFFS files...");
    listDir("/", 0);
    
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
        case Error::InvalidData:
            return "Invalid data";
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