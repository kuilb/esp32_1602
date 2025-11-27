/**
 * @file wifi_config_manager.h
 * @brief WiFi配置管理器
 * 
 * 提供WiFi配置的读取、保存和管理功能，继承自ConfigManager基类
 */

#ifndef WIFI_CONFIG_MANAGER_H
#define WIFI_CONFIG_MANAGER_H
#include "config_manager.h"
#include "logger.h"
#include <ArduinoJson.h>

/**
 * @class WifiConfigManager
 * @brief WiFi配置管理器类
 * 
 * 管理WiFi连接的SSID和密码配置，支持：
 * - 从SPIFFS文件系统加载配置
 * - 保存配置到文件
 * - 重置为默认配置
 */
class WifiConfigManager : public ConfigManager {
private:
    String ssid;        ///< WiFi网络名称(SSID)
    String password;    ///< WiFi密码
public: 
    /**
     * @brief 构造函数
     * @param configFilePath WiFi配置文件的完整路径
     */
    WifiConfigManager(const String& configFilePath);
    
    /**
     * @brief 析构函数
     */
    ~WifiConfigManager() override;
    
    /**
     * @brief 初始化WiFi配置管理器
     * @return true 初始化成功，false 初始化失败
     */
    bool init() override;
    
    /**
     * @brief 从文件加载WiFi配置
     * @return true 加载成功，false 加载失败
     */
    bool loadConfig() override;
    
    /**
     * @brief 保存WiFi配置到文件
     * @return true 保存成功，false 保存失败
     */
    bool saveConfig() override;
    
    /**
     * @brief 重置WiFi配置为默认值
     * @return true 重置成功，false 重置失败
     */
    bool resetConfig() override;

    /**
     * @brief 获取WiFi SSID
     * @return String WiFi网络名称
     */
    String getSSID();
    
    /**
     * @brief 获取WiFi密码
     * @return String WiFi密码
     */
    String getPassword();

    /**
     * @brief 设置WiFi SSID
     * @param ssid 要设置的WiFi网络名称
     * @return true 设置成功，false 设置失败
     */
    bool setSSID(const String& ssid);
    
    /**
     * @brief 设置WiFi密码
     * @param password 要设置的WiFi密码
     * @return true 设置成功，false 设置失败
     */
    bool setPassword(const String& password);
};

#endif  // WIFI_CONFIG_MANAGER_H