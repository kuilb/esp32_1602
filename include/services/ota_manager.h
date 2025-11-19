/**
 * @file ota_manager.h
 * @brief OTA 升级管理头文件，提供固件空中升级功能
 *
 * 此文件声明 OTA 升级管理类、枚举和相关函数
 *
 * @author kulib
 * @date 2025-11-08
 */
#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "myheader.h"
#include <Update.h>
#include <WiFiClientSecure.h>

/**
 * @brief OTA 升级结果枚举
 */
enum OTAResult {
    OTA_SUCCESS,           /**< 升级成功 */
    OTA_FAIL_DOWNLOAD,     /**< 下载失败 */
    OTA_FAIL_WRITE,        /**< 写入失败 */
    OTA_FAIL_NETWORK,      /**< 网络失败 */
    OTA_IN_PROGRESS        /**< 升级进行中 */
};

/**
 * @brief OTA 升级状态枚举
 */
enum OTAStatus {
    OTA_IDLE,              /**< 空闲状态 */
    OTA_RUNNING,           /**< 运行中 */
    OTA_COMPLETED_SUCCESS, /**< 成功完成 */
    OTA_COMPLETED_FAILED   /**< 失败完成 */
};

/**
 * @brief OTA 升级管理类
 * @details 提供固件空中升级功能，支持从 URL 下载和本地文件升级
 */
class OTAManager {
public:
    /**
     * @brief 构造函数
     */
    OTAManager();

    /**
     * @brief 析构函数
     */
    ~OTAManager();

    /**
     * @brief 从 URL 更新固件
     * @param url 固件下载 URL
     * @param useHTTPS 是否使用 HTTPS
     * @return 升级结果
     */
    static OTAResult updateFromURL(const String& url, bool useHTTPS = false);

    /**
     * @brief 从文件数据更新固件
     * @param data 固件数据指针
     * @param length 数据长度
     * @return 升级结果
     */
    static OTAResult updateFromFile(uint8_t* data, size_t length);

    /**
     * @brief 获取升级进度
     * @return 进度百分比 (0-100)
     */
    static int getProgress();

    /**
     * @brief 获取错误信息
     * @return 错误描述字符串
     */
    static String getErrorString();

    /**
     * @brief 检查版本更新
     * @param versionCheckURL 版本检查 URL
     */
    static void checkForUpdate(const String& versionCheckURL);

    /**
     * @brief 获取当前状态
     * @return OTA 状态
     */
    static OTAStatus getStatus();

    /**
     * @brief 检查是否正在进行升级
     * @return true 如果正在升级，false 否则
     */
    static bool isInProgress();

private:
    /**
     * @brief 下载固件数据
     * @param http HTTP 客户端引用
     * @param contentLength 内容长度
     * @return true 如果下载成功，false 否则
     */
    static bool downloadFirmware(HTTPClient& http, size_t contentLength);

    /**
     * @brief 验证校验和
     * @param expectedMD5 期望的 MD5 值
     * @return true 如果校验成功，false 否则
     */
    static bool verifyChecksum(const String& expectedMD5);

    static int progress;               /**< 升级进度百分比 */
    static String lastError;           /**< 最后错误信息 */
    static volatile OTAStatus currentStatus;   /**< 当前 OTA 状态 */
    static volatile OTAResult currentResult;   /**< 当前 OTA 结果 */
};

#endif