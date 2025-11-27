#include "jwt_auth.h"
#include "utils/logger.h"

extern QWeatherAuthConfigManager qweatherAuthConfigManager;

// API参数
static char apiHost[128] = "";     // API地址
static char kid[64] = "";          // Key ID
static char projectID[64] = "";   // 项目ID
static char base64Key[256] = "";  // Base64编码的PKCS#8私钥
static char location[32] = "";     // LocationID
static char cityName[64] = "";    // 地名

// 32字节 Ed25519 seed
uint8_t seed32[32] = {}; 

// 初始化 JWT 配置
void loadJwtConfig() {
    qweatherAuthConfigManager.loadConfig();
    strncpy(apiHost, qweatherAuthConfigManager.getApiHost().c_str(), sizeof(apiHost) - 1);
    apiHost[sizeof(apiHost) - 1] = '\0';
    strncpy(kid,  qweatherAuthConfigManager.getKId().c_str(), sizeof(kid) - 1);
    kid[sizeof(kid) - 1] = '\0';
    strncpy(projectID, qweatherAuthConfigManager.getProjectID().c_str(), sizeof(projectID) - 1);
    projectID[sizeof(projectID) - 1] = '\0';
    strncpy(base64Key, qweatherAuthConfigManager.getBase64Key().c_str(), sizeof(base64Key) - 1);
    base64Key[sizeof(base64Key) - 1] = '\0';
    strncpy(location, qweatherAuthConfigManager.getLocation().c_str(), sizeof(location) - 1);
    location[sizeof(location) - 1] = '\0';
    strncpy(cityName, qweatherAuthConfigManager.getCityName().c_str(), sizeof(cityName) - 1);
    cityName[sizeof(cityName) - 1] = '\0';

    LOG_JWT_INFO("JWT 配置已加载");
    LOG_JWT_DEBUG("apiHost: %s", apiHost);
    LOG_JWT_DEBUG("kid: %s", kid);
    LOG_JWT_DEBUG("projectID: %s", projectID);
    LOG_JWT_DEBUG("base64Key: %s", base64Key);
    LOG_JWT_DEBUG("locationID: %s", location);
    LOG_JWT_DEBUG("cityName: %s", cityName);
}


// -------- Base64 Decode --------
const int8_t b64_table[256] = {     // base64 ascii对应解码表
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
};  // -1:无效字符 -2:停止解码 0-63:有效Base64字符对应的数值


int _base64_decode(const char* input, uint8_t* output) {
    int len = 0;    // 输出长度
    int val = 0;    // 累积的位值
    int valb = -8;  // 累积的位数
    for (int i = 0; input[i]; i++) {    // 遍历输入字符串
        int8_t c = b64_table[(uint8_t)input[i]];
        if (c == -1) continue;
        if (c == -2) break;
        val = (val << 6) | c;   // 累积6位
        valb += 6;
        if (valb >= 0) {
            output[len++] = (val >> valb) & 0xFF;   // 提取8位输出
            valb -= 8;
        }
        // 如果还有不满8位的数据那么忽略
    }
    return len;
}

// 从PKCS#8 DER私钥数据中提取Ed25519的32字节seed
bool _extract_ed25519_seed(const uint8_t* der, size_t der_len, uint8_t* seed32) {
    LOG_JWT_DEBUG("Extracting Ed25519 seed from DER data...");
    LOG_JWT_DEBUG("PKCS#8 DER length: %d", der_len);
    
    for (size_t i = 0; i < der_len - 1; i++) {
        // 查找连续 0x04 0x20 开头的 32 字节数据
        if (der[i] == 0x04 && der[i+1] == 0x20 && (i + 34) <= der_len) {
            LOG_JWT_DEBUG("Found Ed25519 seed pattern at offset: %d", i);
            memcpy(seed32, der + i + 2, 32);    // 复制32字节seed
            return true;
        }
    }

    // 打印seed32数据
    LOG_JWT_ERROR("could not find seed32! \nDER bytes:");
    for (size_t i = 0; i < der_len; i++) {
        Serial.printf("%02X ", der[i]);
        if ((i + 1) % 16 == 0) Serial.println();
    }
    Serial.println();
    return false;
}

// PKCS#8 DER私钥Base64解码 + 提取seed32
void generateSeed32() {
    if (strlen(base64Key) == 0) {
        LOG_JWT_ERROR("No base64 key, cannot generate seed32");
        return;
    }

    LOG_JWT_VERBOSE("Base64 key length: %d", strlen(base64Key));
    LOG_JWT_VERBOSE("Base64 key (first 20 chars): %s...", base64Key);

    uint8_t der[128];   // DER编码
    int der_len = _base64_decode(base64Key, der);
    LOG_JWT_VERBOSE("DER decoded length: %d", der_len);
    
    if(!_extract_ed25519_seed(der, der_len, seed32)) {
        // LOG_JWT_ERROR("Failed to extract seed32!");
        return;
    }
    // LOG_JWT_DEBUG("Extracted Seed32: ");
    // for(int i=0;i<32;i++){
    //     Serial.printf("%02X", seed32[i]);
    // }
    // Serial.println();
    LOG_JWT_INFO("Seed32 generation completed successfully");
}

// Base64 URL-safe编码
String _base64url_encode(const uint8_t* data, size_t len) {
    String b64 = "";
    const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    int val = 0, valb = -6;             // 累积的位值和位数
    for (size_t i = 0; i < len; i++) {
        val = (val << 8) + data[i];     // 添加8位待编码的值
        valb += 8;
        while (valb >= 0) {
            b64 += table[(val >> valb) & 0x3F]; // 提取6位进行编码
            valb -= 6;
        }
    }
    if (valb > -6) b64 += table[((val << 8) >> (valb + 8)) & 0x3F]; // 处理长度不足 3 字节倍数的数据，不足的 4 位或 2 位填充 0
    // Base64 URL-safe不添加'='

    return b64;
}

// 生成JWT Token
String generate_jwt(const String& kid, const String& projectID, const uint8_t* seed32) {
    // 去掉前后空格
    String kid_trimmed = kid;
    kid_trimmed.trim();
    String project_trimmed = projectID;
    project_trimmed.trim();

    // Header（只含alg和kid）
    JsonDocument header;
    header["alg"] = "EdDSA";
    header["kid"] = kid_trimmed;    // 凭据ID
    String header_json;
    serializeJson(header, header_json);
    LOG_JWT_DEBUG("Header: %s", header_json.c_str());
    String header_b64 = _base64url_encode((const uint8_t*)header_json.c_str(), header_json.length());

    // Payload（只含sub、iat、exp）
    JsonDocument payload;
    payload["sub"] = project_trimmed;  // 签发主体：项目ID
    unsigned long now = time(nullptr);
    
    // 检查时间是否已同步
    if (now < 1762255390) {         // 2025年11月04日的时间戳
        LOG_JWT_ERROR("time not synced!,check NTP settings.");      // 即使时间不对，也继续生成JWT
        LOG_JWT_DEBUG("Using unsynced time, timestamp: %lu", now);  // 打印可读时间用于调试
        struct tm* timeinfo = localtime((time_t*)&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
        LOG_JWT_DEBUG("Time: %s", timeStr);
    }
    
    unsigned long iat = now - 30;       // 签发时间：当前时间前30秒
    unsigned long exp = now + 3600;     // 有效期1H
    
    payload["iat"] = iat;
    payload["exp"] = exp;
    
    LOG_JWT_VERBOSE("iat: %lu, exp: %lu", iat, exp);
    String payload_json;
    serializeJson(payload, payload_json);   
    LOG_JWT_DEBUG("Payload: %s", payload_json.c_str());
    String payload_b64 = _base64url_encode((const uint8_t*)payload_json.c_str(), payload_json.length());

    // 使用Ed25519算法对header.payload进行签名
    String signing_input = header_b64 + "." + payload_b64;
    LOG_JWT_VERBOSE("Signing input: %s", signing_input.c_str());
    
    // 从seed生成Ed25519密钥对
    // pk：生成的 32 字节公钥
    // sk：生成的 64 字节私钥
    uint8_t pk[crypto_sign_PUBLICKEYBYTES], sk[crypto_sign_SECRETKEYBYTES];
    crypto_sign_seed_keypair(pk, sk, seed32);
    
    // 打印公钥用于调试
    // LOG_JWT_DEBUG("Public key: ");
    // for(int i = 0; i < 8; i++) { // 只打印前8字节
    //     Serial.printf("%02X", pk[i]);
    // }
    // Serial.println("...");
    
    // 使用Ed25519算法生成分离签名
    uint8_t signature[crypto_sign_BYTES];
    unsigned long long siglen;
    crypto_sign_detached(
        signature, 
        &siglen,
        (const unsigned char*)signing_input.c_str(), 
        signing_input.length(), 
        sk          // 上一步生成的64字节私钥
    );
    
    // LOG_JWT_DEBUG("Signature length: %llu", siglen);

    // 对签名进行Base64URL编码
    String signature_b64 = _base64url_encode(signature, siglen);

    // 拼接最终JWT (header.payload.signature)
    String jwt = signing_input + "." + signature_b64;
    LOG_JWT_DEBUG("JWT token generated successfully, length: %d", jwt.length());
    LOG_JWT_VERBOSE("Final JWT: %s", jwt.c_str());
    return jwt;
}

// 检查私钥是否有效
bool validate_base64_ed25519_key(const char* base64) {
    if (!base64) return false;
    size_t len = strlen(base64);
    // 长度应为64
    if (len != 64) {
        LOG_JWT_WARN("validate_base64_ed25519_key: unexpected base64 length: %d", (int)len);
        return false;
    }
    // 检查是否为base64格式
    for (size_t i = 0; base64[i]; ++i) {
        char c = base64[i];
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '-' || c == '_' || c == '=')) {
            LOG_JWT_DEBUG("validate_base64_ed25519_key: invalid char in base64: %c", c);
            return false;
        }
    }

    // 尝试解码并提取seed32
    uint8_t der[128] = {0};
    int der_len = _base64_decode(base64, der);
    if (der_len <= 0) {
        LOG_JWT_DEBUG("validate_base64_ed25519_key: base64 decode failed");
        return false;
    }
    
    uint8_t tmpSeed[32] = {0};
    if (!_extract_ed25519_seed(der, der_len, tmpSeed)) {
        LOG_JWT_DEBUG("validate_base64_ed25519_key: seed extraction failed");
        return false;
    }
    return true;
}