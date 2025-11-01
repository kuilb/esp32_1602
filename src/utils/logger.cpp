#include "utils/logger.h"
#include <stdarg.h>

// 静态成员变量定义
LogLevel Logger::globalLogLevel = LOG_LEVEL_INFO;
LogLevel Logger::moduleLogLevels[LOG_MODULE_MAX];
bool Logger::initialized = false;

// 模块前缀定义
const char* Logger::modulePrefixes[LOG_MODULE_MAX] = {
    "SYSTEM",
    "WIFI",
    "TIME_SYNC",
    "WEATHER",
    "MENU",
    "LCD",
    "BUTTON",
    "JWT",
    "WEB",
    "NETWORK",
    "MEMORY",
    "RGB"
};

// 日志级别前缀定义
const char* Logger::levelPrefixes[6] = {
    "NONE",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "VERBOSE"
};

void Logger::init(LogLevel defaultLevel) {
    if (initialized) return;
    
    globalLogLevel = defaultLevel;
    
    // 初始化所有模块的日志级别为默认级别
    for (int i = 0; i < LOG_MODULE_MAX; i++) {
        moduleLogLevels[i] = defaultLevel;
    }
    
    initialized = true;
    
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    
    Serial.println();
    Serial.println("=== Logger System Initialized ===");
    Serial.printf("[INIT] Global log level: %s\n", levelPrefixes[defaultLevel]);
    Serial.println("==================================");
}

void Logger::setGlobalLevel(LogLevel level) {
    globalLogLevel = level;
    Serial.printf("[LOGGER] Global log level set to: %s\n", levelPrefixes[level]);
}

void Logger::setModuleLevel(LogModule module, LogLevel level) {
    if (module < LOG_MODULE_MAX) {
        moduleLogLevels[module] = level;
        Serial.printf("[LOGGER] Module %s log level set to: %s\n", 
                     modulePrefixes[module], levelPrefixes[level]);
    }
}

bool Logger::shouldLog(LogModule module, LogLevel level) {
    if (!initialized || level == LOG_LEVEL_NONE) return false;
    
    // 检查全局级别
    if (level > globalLogLevel) return false;
    
    // 检查模块级别
    if (module < LOG_MODULE_MAX && level > moduleLogLevels[module]) {
        return false;
    }
    
    return true;
}

void Logger::printTimestamp() {
    unsigned long now = millis();
    unsigned long seconds = now / 1000;
    unsigned long milliseconds = now % 1000;
    
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    
    Serial.printf("[%02lu:%02lu:%02lu.%03lu] ", hours, minutes, seconds, milliseconds);
}

const char* Logger::getModulePrefix(LogModule module) {
    if (module < LOG_MODULE_MAX) {
        return modulePrefixes[module];
    }
    return "UNKNOWN";
}

const char* Logger::getLevelPrefix(LogLevel level) {
    if (level >= 0 && level < 6) {
        return levelPrefixes[level];
    }
    return "UNKNOWN";
}

void Logger::log(LogModule module, LogLevel level, const char* format, ...) {
    if (!shouldLog(module, level)) return;
    
    // 打印时间戳
    printTimestamp();
    
    // 打印级别和模块
    Serial.printf("[%s][%s] ", getLevelPrefix(level), getModulePrefix(module));
    
    // 打印消息内容
    va_list args;
    va_start(args, format);
    
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.print(buffer);
    
    va_end(args);
    
    // 如果消息没有以换行符结尾，添加一个
    if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] != '\n') {
        Serial.println();
    }
}

void Logger::log(LogModule module, LogLevel level, const String& message) {
    log(module, level, "%s", message.c_str());
}

void Logger::logMemoryInfo(LogModule module) {
    if (!shouldLog(module, LOG_LEVEL_INFO)) return;
    
    size_t freeHeap = ESP.getFreeHeap();
    size_t totalHeap = ESP.getHeapSize();
    size_t usedHeap = totalHeap - freeHeap;
    
    // 如果有PSRAM，也显示PSRAM信息
    if (ESP.getPsramSize() > 0) {
        size_t freePsram = ESP.getFreePsram();
        size_t totalPsram = ESP.getPsramSize();
        size_t usedPsram = totalPsram - freePsram;
        
        log(module, LOG_LEVEL_INFO, 
            "Memory - Heap: %u/%u bytes (%.1f%%), PSRAM: %u/%u bytes (%.1f%%)",
            usedHeap, totalHeap, (float)usedHeap * 100.0 / totalHeap,
            usedPsram, totalPsram, (float)usedPsram * 100.0 / totalPsram);
    } else {
        log(module, LOG_LEVEL_INFO, 
            "Memory - Heap: %u/%u bytes (%.1f%%)",
            usedHeap, totalHeap, (float)usedHeap * 100.0 / totalHeap);
    }
}