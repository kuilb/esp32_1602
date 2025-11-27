/**
 * @file config_manager.h
 * @brief 配置管理器基类
 * 
 * 提供配置文件的读写、错误处理和SPIFFS文件系统初始化等基础功能
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "myheader.h"
#include "logger.h"

/**
 * @class ConfigManager
 * @brief 配置管理器抽象基类
 * 
 * 为各种配置管理器提供统一的接口和基础功能实现，包括：
 * - 文件读写操作
 * - 错误处理机制
 * - SPIFFS文件系统管理
 */
class ConfigManager{
public:
    /**
     * @enum Error
     * @brief 错误类型枚举
     */
    enum class Error {
        None = 0,           ///< 无错误
        FileNotFound,       ///< 文件未找到
        ReadError,          ///< 读取错误
        WriteError,         ///< 写入错误
        UnknownError        ///< 未知错误
    };
protected:
    String configFilePath;  ///< 配置文件路径
    Error lastError;        ///< 最后一次错误类型

    /**
     * @brief 构造函数
     * @param configFilePath 配置文件的完整路径
     */
    ConfigManager(const String& configFilePath);
    
    /**
     * @brief 虚析构函数
     */
    virtual ~ConfigManager();
    
    /**
     * @brief 从文件读取配置内容
     * @param configContent 用于存储读取内容的字符串引用
     * @return true 读取成功，false 读取失败
     */
    bool readFile(String& configContent);
    
    /**
     * @brief 将配置内容写入文件
     * @param configContent 要写入的配置内容
     * @return true 写入成功，false 写入失败
     */
    bool writeFile(const String& configContent);
    
    /**
     * @brief 列出目录内容
     * @param dirname 目录名称
     * @param levels 列出的目录层级深度
     */
    static void listDir(const char* dirname, uint8_t levels);
public:
    static bool isSPIFFSInitialized;  ///< SPIFFS初始化状态标志
    
    /**
     * @brief 初始化SPIFFS文件系统
     * @return true 初始化成功，false 初始化失败
     */
    static bool initSPIFFS();

    /**
     * @brief 获取最后一次错误类型
     * @return Error 错误类型枚举值
     */
    Error getLastError() const;
    
    /**
     * @brief 获取错误类型的字符串描述
     * @param error 错误类型
     * @return String 错误描述字符串
     */
    String getLastErrorString(Error error) const;
    
    /**
     * @brief 设置错误类型
     * @param error 要设置的错误类型
     */
    void setLastError(Error error);
    
    /**
     * @brief 清除错误状态
     */
    void clearLastError();

    /**
     * @brief 初始化配置管理器
     * @return true 初始化成功，false 初始化失败
     * @note 纯虚函数，由派生类实现具体逻辑
     */
    virtual bool init() = 0;
    
    /**
     * @brief 加载配置
     * @return true 加载成功，false 加载失败
     * @note 纯虚函数，由派生类实现具体逻辑
     */
    virtual bool loadConfig() = 0;
    
    /**
     * @brief 保存配置
     * @return true 保存成功，false 保存失败
     * @note 纯虚函数，由派生类实现具体逻辑
     */
    virtual bool saveConfig() = 0;
    
    /**
     * @brief 重置配置为默认值
     * @return true 重置成功，false 重置失败
     * @note 纯虚函数，由派生类实现具体逻辑
     */
    virtual bool resetConfig() = 0;
};

#endif  // CONFIG_MANAGER_H