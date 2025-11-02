#ifndef LOGGER_H
#define LOGGER_H

#include "myhader.h"

/**
 * @brief 日志级别枚举
 */
enum LogLevel {
    LOG_LEVEL_NONE = 0,    // 不输出任何日志
    LOG_LEVEL_ERROR = 1,   // 只输出错误
    LOG_LEVEL_WARN = 2,    // 输出警告和错误
    LOG_LEVEL_INFO = 3,    // 输出信息、警告和错误
    LOG_LEVEL_DEBUG = 4,   // 输出所有日志
    LOG_LEVEL_VERBOSE = 5  // 输出详细日志
};

/**
 * @brief 日志模块枚举
 */
enum LogModule {
    LOG_MODULE_SYSTEM = 0,
    LOG_MODULE_WIFI,
    LOG_MODULE_TIME_SYNC,
    LOG_MODULE_WEATHER,
    LOG_MODULE_MENU,
    LOG_MODULE_LCD,
    LOG_MODULE_BUTTON,
    LOG_MODULE_JWT,
    LOG_MODULE_WEB,
    LOG_MODULE_NETWORK,
    LOG_MODULE_MEMORY,
    LOG_MODULE_RGB,
    LOG_MODULE_DISPLAY,
    LOG_MODULE_MAX
};

/**
 * @brief 日志管理器类
 */
class Logger {
private:
    static LogLevel globalLogLevel;
    static LogLevel moduleLogLevels[LOG_MODULE_MAX];
    static const char* modulePrefixes[LOG_MODULE_MAX];
    static const char* levelPrefixes[6];
    static bool initialized;
    
    static void printTimestamp();
    static const char* getModulePrefix(LogModule module);
    static const char* getLevelPrefix(LogLevel level);
    
public:
    /**
     * @brief 初始化日志系统
     */
    static void init(LogLevel defaultLevel = LOG_LEVEL_INFO);
    
    /**
     * @brief 设置全局日志级别
     */
    static void setGlobalLevel(LogLevel level);
    
    /**
     * @brief 设置特定模块的日志级别
     */
    static void setModuleLevel(LogModule module, LogLevel level);
    
    /**
     * @brief 检查是否应该输出日志
     */
    static bool shouldLog(LogModule module, LogLevel level);
    
    /**
     * @brief 输出日志
     */
    static void log(LogModule module, LogLevel level, const char* format, ...);
    
    /**
     * @brief 输出日志（String版本）
     */
    static void log(LogModule module, LogLevel level, const String& message);
    
    /**
     * @brief 获取当前可用堆内存（用于内存监控日志）
     */
    static void logMemoryInfo(LogModule module = LOG_MODULE_MEMORY);
};

// 便捷宏定义
#define LOG_ERROR(module, ...) Logger::log(module, LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_WARN(module, ...)  Logger::log(module, LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_INFO(module, ...)  Logger::log(module, LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DEBUG(module, ...) Logger::log(module, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_VERBOSE(module, ...) Logger::log(module, LOG_LEVEL_VERBOSE, __VA_ARGS__)

// 特定模块的便捷宏
#define LOG_DISPLAY_ERROR(...)   LOG_ERROR(LOG_MODULE_DISPLAY, __VA_ARGS__)
#define LOG_DISPLAY_WARN(...)    LOG_WARN(LOG_MODULE_DISPLAY, __VA_ARGS__)
#define LOG_DISPLAY_INFO(...)    LOG_INFO(LOG_MODULE_DISPLAY, __VA_ARGS__)
#define LOG_DISPLAY_DEBUG(...)   LOG_DEBUG(LOG_MODULE_DISPLAY, __VA_ARGS__)
#define LOG_DISPLAY_VERBOSE(...) LOG_VERBOSE(LOG_MODULE_DISPLAY, __VA_ARGS__)

#define LOG_RGB_ERROR(...)   LOG_ERROR(LOG_MODULE_RGB, __VA_ARGS__)
#define LOG_RGB_WARN(...)    LOG_WARN(LOG_MODULE_RGB, __VA_ARGS__)
#define LOG_RGB_INFO(...)    LOG_INFO(LOG_MODULE_RGB, __VA_ARGS__)
#define LOG_RGB_DEBUG(...)   LOG_DEBUG(LOG_MODULE_RGB, __VA_ARGS__)
#define LOG_RGB_VERBOSE(...) LOG_VERBOSE(LOG_MODULE_RGB, __VA_ARGS__)

#define LOG_BUTTON_ERROR(...)   LOG_ERROR(LOG_MODULE_BUTTON, __VA_ARGS__)
#define LOG_BUTTON_WARN(...)    LOG_WARN(LOG_MODULE_BUTTON, __VA_ARGS__)
#define LOG_BUTTON_INFO(...)    LOG_INFO(LOG_MODULE_BUTTON, __VA_ARGS__)
#define LOG_BUTTON_DEBUG(...)   LOG_DEBUG(LOG_MODULE_BUTTON, __VA_ARGS__)
#define LOG_BUTTON_VERBOSE(...) LOG_VERBOSE(LOG_MODULE_BUTTON, __VA_ARGS__)

#define LOG_LCD_ERROR(...)   LOG_ERROR(LOG_MODULE_LCD, __VA_ARGS__)
#define LOG_LCD_WARN(...)    LOG_WARN(LOG_MODULE_LCD, __VA_ARGS__)
#define LOG_LCD_INFO(...)    LOG_INFO(LOG_MODULE_LCD, __VA_ARGS__)
#define LOG_LCD_DEBUG(...)   LOG_DEBUG(LOG_MODULE_LCD, __VA_ARGS__)
#define LOG_LCD_VERBOSE(...) LOG_VERBOSE(LOG_MODULE_LCD, __VA_ARGS__)

#define LOG_SYSTEM_ERROR(...)   LOG_ERROR(LOG_MODULE_SYSTEM, __VA_ARGS__)
#define LOG_SYSTEM_WARN(...)    LOG_WARN(LOG_MODULE_SYSTEM, __VA_ARGS__)
#define LOG_SYSTEM_INFO(...)    LOG_INFO(LOG_MODULE_SYSTEM, __VA_ARGS__)
#define LOG_SYSTEM_DEBUG(...)   LOG_DEBUG(LOG_MODULE_SYSTEM, __VA_ARGS__)
#define LOG_SYSTEM_VERBOSE(...) LOG_VERBOSE(LOG_MODULE_SYSTEM, __VA_ARGS__)

#define LOG_WIFI_ERROR(...)     LOG_ERROR(LOG_MODULE_WIFI, __VA_ARGS__)
#define LOG_WIFI_WARN(...)      LOG_WARN(LOG_MODULE_WIFI, __VA_ARGS__)
#define LOG_WIFI_INFO(...)      LOG_INFO(LOG_MODULE_WIFI, __VA_ARGS__)
#define LOG_WIFI_DEBUG(...)     LOG_DEBUG(LOG_MODULE_WIFI, __VA_ARGS__)
#define LOG_WIFI_VERBOSE(...)   LOG_VERBOSE(LOG_MODULE_WIFI, __VA_ARGS__)

#define LOG_TIME_ERROR(...)     LOG_ERROR(LOG_MODULE_TIME_SYNC, __VA_ARGS__)
#define LOG_TIME_WARN(...)      LOG_WARN(LOG_MODULE_TIME_SYNC, __VA_ARGS__)
#define LOG_TIME_INFO(...)      LOG_INFO(LOG_MODULE_TIME_SYNC, __VA_ARGS__)
#define LOG_TIME_DEBUG(...)     LOG_DEBUG(LOG_MODULE_TIME_SYNC, __VA_ARGS__)
#define LOG_TIME_VERBOSE(...)   LOG_VERBOSE(LOG_MODULE_TIME_SYNC, __VA_ARGS__)

#define LOG_WEATHER_ERROR(...)  LOG_ERROR(LOG_MODULE_WEATHER, __VA_ARGS__)
#define LOG_WEATHER_WARN(...)   LOG_WARN(LOG_MODULE_WEATHER, __VA_ARGS__)
#define LOG_WEATHER_INFO(...)   LOG_INFO(LOG_MODULE_WEATHER, __VA_ARGS__)
#define LOG_WEATHER_DEBUG(...)  LOG_DEBUG(LOG_MODULE_WEATHER, __VA_ARGS__)
#define LOG_WEATHER_VERBOSE(...) LOG_VERBOSE(LOG_MODULE_WEATHER, __VA_ARGS__)

#define LOG_MENU_ERROR(...)     LOG_ERROR(LOG_MODULE_MENU, __VA_ARGS__)
#define LOG_MENU_WARN(...)      LOG_WARN(LOG_MODULE_MENU, __VA_ARGS__)
#define LOG_MENU_INFO(...)      LOG_INFO(LOG_MODULE_MENU, __VA_ARGS__)
#define LOG_MENU_DEBUG(...)     LOG_DEBUG(LOG_MODULE_MENU, __VA_ARGS__)
#define LOG_MENU_VERBOSE(...)   LOG_VERBOSE(LOG_MODULE_MENU, __VA_ARGS__)

#define LOG_JWT_ERROR(...)      LOG_ERROR(LOG_MODULE_JWT, __VA_ARGS__)
#define LOG_JWT_WARN(...)       LOG_WARN(LOG_MODULE_JWT, __VA_ARGS__)
#define LOG_JWT_INFO(...)       LOG_INFO(LOG_MODULE_JWT, __VA_ARGS__)
#define LOG_JWT_DEBUG(...)      LOG_DEBUG(LOG_MODULE_JWT, __VA_ARGS__)
#define LOG_JWT_VERBOSE(...)    LOG_VERBOSE(LOG_MODULE_JWT, __VA_ARGS__)

#define LOG_WEB_ERROR(...)      LOG_ERROR(LOG_MODULE_WEB, __VA_ARGS__)
#define LOG_WEB_WARN(...)       LOG_WARN(LOG_MODULE_WEB, __VA_ARGS__)
#define LOG_WEB_INFO(...)       LOG_INFO(LOG_MODULE_WEB, __VA_ARGS__)
#define LOG_WEB_DEBUG(...)      LOG_DEBUG(LOG_MODULE_WEB, __VA_ARGS__)
#define LOG_WEB_VERBOSE(...)    LOG_VERBOSE(LOG_MODULE_WEB, __VA_ARGS__)

#define LOG_MEMORY_ERROR(...)   LOG_ERROR(LOG_MODULE_MEMORY, __VA_ARGS__)
#define LOG_MEMORY_WARN(...)    LOG_WARN(LOG_MODULE_MEMORY, __VA_ARGS__)
#define LOG_MEMORY_INFO_MSG(...) LOG_INFO(LOG_MODULE_MEMORY, __VA_ARGS__)
#define LOG_MEMORY_DEBUG(...)   LOG_DEBUG(LOG_MODULE_MEMORY, __VA_ARGS__)
#define LOG_MEMORY_VERBOSE(...) LOG_VERBOSE(LOG_MODULE_MEMORY, __VA_ARGS__)

#define LOG_MEMORY_INFO()       Logger::logMemoryInfo()

#endif