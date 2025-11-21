/**
 * @file jwt_auth.h
 * @brief JWT 认证头文件，提供 JWT 生成和配置功能
 *
 * 此文件声明 JWT 配置、种子生成和 Token 生成的函数和变量
 *
 * @author kulib
 * @date 2025-11-05
 */
#ifndef JWT_AUTH_H
#define JWT_AUTH_H

#include "mydefine.h"
#include "myheader.h"

extern char apiHost[128];          /**< API 地址 */
extern char kid[64];               /**< Key ID */
extern char projectID[64];         /**< 项目 ID */
extern char base64Key[256];        /**< Base64 编码的 PKCS#8 私钥 */

extern char location[32];          /**< LocationID */
extern char cityName[64];          /**< 城市名称 */

extern uint8_t seed32[32];         /**< 32 字节 Ed25519 种子 */

/**
 * @brief 初始化 JWT 配置
 * @details 从 SPIFFS 文件系统加载 JWT 配置文件，读取 API 地址、Key ID 等参数
 */
void init_jwt();

/**
 * @brief 生成 seed32
 * @details 从 Base64 编码的 PKCS#8 私钥中解码并提取 Ed25519 的 32 字节种子
 */
void generateSeed32();

/**
 * @brief 生成 JWT Token
 * @param kid Key ID，用于 JWT header
 * @param projectID 项目 ID，用于 JWT payload
 * @param seed32 32 字节 Ed25519 种子，用于生成密钥对和签名
 * @return 生成的 JWT Token 字符串
 */
String generate_jwt(const String& kid, const String& projectID, const uint8_t* seed32);

// 检测 base64 PKCS#8 Ed25519 私钥 (返回是否有效)
bool validate_base64_ed25519_key(const char* base64);

#endif