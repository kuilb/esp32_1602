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

#include <Update.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "./utils/logger.h"
#include "./hardware/lcd_driver.h"
#include "./hardware/rgb_led.h"

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
 * @brief 初始化 OTA 模块状态
 */
void otaInit();

/**
 * @brief 从 URL 更新固件
 * @param url 固件下载 URL
 * @param useHTTPS 是否使用 HTTPS
 * @return 升级结果
 */
OTAResult otaUpdateFromURL(const String& url, bool useHTTPS = false);

/**
 * @brief 从文件数据更新固件
 * @param data 固件数据指针
 * @param length 数据长度
 * @return 升级结果
 */
OTAResult otaUpdateFromFile(uint8_t* data, size_t length);

/**
 * @brief 获取升级进度
 * @return 进度百分比 (0-100)
 */
int otaGetProgress();

/**
 * @brief 获取错误信息
 * @return 错误描述字符串
 */
String otaGetErrorString();

/**
 * @brief 检查版本更新
 * @param versionCheckURL 版本检查 URL
 */
void otaCheckForUpdate(const String& versionCheckURL);

/**
 * @brief 获取当前状态
 * @return OTA 状态
 */
OTAStatus otaGetStatus();

/**
 * @brief 检查是否正在进行升级
 * @return true 如果正在升级，false 否则
 */
bool otaIsInProgress();

#endif