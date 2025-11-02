#include "jwt_auth.h"
#include "utils/logger.h"

// API参数
char apiHost[128] = "";
char kid[64] = "";
char project_id[64] = "";
char base64_key[256] = "";
char location[32] = "";    // LocationID (改为固定数组,避免内存泄漏)
char city_name[64] = "";   // 地名 (改为固定数组,避免内存泄漏)

//Ed25519 seed，32字节
uint8_t seed32[32] = {}; 

// 初始化 JWT 配置
void init_jwt() {
    if (!SPIFFS.begin(true)) {
        LOG_JWT_ERROR("SPIFFS 初始化失败");
        return;
    }

    if (!SPIFFS.exists("/jwt_config.txt")) {
        LOG_JWT_WARN("JWT 配置文件不存在");
        return;
    }

    File file = SPIFFS.open("/jwt_config.txt", "r");
    if (!file) {
        LOG_JWT_ERROR("打开 JWT 配置文件失败");
        return;
    }

    file.readBytesUntil('\n', apiHost, sizeof(apiHost));
    file.readBytesUntil('\n', kid, sizeof(kid));
    file.readBytesUntil('\n', project_id, sizeof(project_id));
    file.readBytesUntil('\n', base64_key, sizeof(base64_key));

    // 读取locationID（第五行）- 直接读入固定数组,避免内存泄漏
    file.readBytesUntil('\n', location, sizeof(location));
    // 移除换行符和空白字符
    for (int i = 0; i < sizeof(location); i++) {
        if (location[i] == '\n' || location[i] == '\r') {
            location[i] = '\0';
            break;
        }
    }

    // 读取地名（第六行）- 直接读入固定数组,避免内存泄漏
    file.readBytesUntil('\n', city_name, sizeof(city_name));
    // 移除换行符和空白字符
    for (int i = 0; i < sizeof(city_name); i++) {
        if (city_name[i] == '\n' || city_name[i] == '\r') {
            city_name[i] = '\0';
            break;
        }
    }
    file.close();

    LOG_JWT_INFO("JWT 配置已加载");
    LOG_JWT_DEBUG("apiHost: %s", apiHost);
    LOG_JWT_DEBUG("kid: %s", kid);
    LOG_JWT_DEBUG("project_id: %s", project_id);
    LOG_JWT_DEBUG("base64_key: %s", base64_key);
    LOG_JWT_DEBUG("locationID: %s", location);
    LOG_JWT_DEBUG("city_name: %s", city_name);
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
};  // -1:无效字符 -2:停止解码 0-63: 有效Base64字符对应的数值


int base64_decode(const char* input, uint8_t* output) {
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
    }
    return len;
}

// -------- Extract seed32 from PKCS#8 DER --------
bool extract_ed25519_seed(const uint8_t* der, size_t der_len, uint8_t* seed32) {
    LOG_JWT_DEBUG("Extracting Ed25519 seed from DER data...");
    LOG_JWT_DEBUG("DER length: %d", der_len);
    
    for (size_t i = 0; i < der_len - 1; i++) {
        // 查找连续 0x04 0x20 开头的 32 字节数据
        if (der[i] == 0x04 && der[i+1] == 0x20 && (i + 34) <= der_len) {
            LOG_JWT_DEBUG("Found Ed25519 seed pattern at offset: %d", i);
            memcpy(seed32, der + i + 2, 32);
            LOG_JWT_DEBUG("Successfully extracted 32-byte seed");
            return true;
        }
    }
    // 打印调试信息
    LOG_JWT_ERROR("无法找到seed32! DER bytes:");
    for (size_t i = 0; i < der_len && i < 64; i++) { // 限制输出长度
        Serial.printf("%02X ", der[i]);
        if ((i + 1) % 16 == 0) Serial.println();
    }
    if (der_len > 64) Serial.println("... (truncated)");
    Serial.println();
    return false;
}

// Base64解码 + 提取seed函数
void generateSeed32() {
    LOG_JWT_DEBUG("Generating seed32 from base64 key...");
    
    if (strlen(base64_key) == 0) {
        LOG_JWT_ERROR("No base64 key, cannot generate seed32");
        return;
    }

    // LOG_JWT_VERBOSE("Base64 key length: %d", strlen(base64_key));
    // LOG_JWT_VERBOSE("Base64 key (first 20 chars): %.*s...", 20, base64_key);

    uint8_t der[128];   // DER编码
    int der_len = base64_decode(base64_key, der);
    // LOG_JWT_DEBUG("DER decoded length: %d", der_len);
    
    if(!extract_ed25519_seed(der, der_len, seed32)) {
        LOG_JWT_ERROR("Failed to extract seed32!");
        return;
    }
    // LOG_JWT_DEBUG("Extracted Seed32: ");
    for(int i=0;i<32;i++){
        Serial.printf("%02X", seed32[i]);
    }
    Serial.println();
    LOG_JWT_INFO("Seed32 generation completed successfully");
}

// Base64 URL-safe编码
String base64url_encode(const uint8_t* data, size_t len) {
    String b64 = "";
    const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    int val = 0, valb = -6;
    for (size_t i = 0; i < len; i++) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            b64 += table[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }
    if (valb > -6) b64 += table[((val << 8) >> (valb + 8)) & 0x3F];
    
    return b64;
}

// 生成JWT Token
String generate_jwt(const String& kid, const String& project_id, const uint8_t* seed32) {
    // 去掉前后空格
    String kid_trimmed = kid;
    kid_trimmed.trim();
    String project_trimmed = project_id;
    project_trimmed.trim();

    // Header（只含alg和kid）
    StaticJsonDocument<128> header;
    header["alg"] = "EdDSA";        // 必须是EdDSA
    header["kid"] = kid_trimmed;    // 凭据ID
    String header_json;
    serializeJson(header, header_json);
    LOG_JWT_DEBUG("Header: %s", header_json.c_str());
    String header_b64 = base64url_encode((const uint8_t*)header_json.c_str(), header_json.length());

    // Payload（只含sub、iat、exp）
    StaticJsonDocument<128> payload;
    payload["sub"] = project_trimmed;  // 签发主体：项目ID
    unsigned long now = time(nullptr);
    
    // 检查时间是否已同步
    if (now < 1609459200) {         // 2021年1月1日的时间戳
        LOG_JWT_ERROR("系统时间未同步! 当前时间戳: %lu", now);
        LOG_JWT_WARN("请确保NTP时间同步正常");
        // 即使时间不对，也继续生成JWT，但会在调试中标识
    } else {
        LOG_JWT_DEBUG("系统时间正常, 时间戳: %lu", now);
        // 打印可读时间用于调试
        struct tm* timeinfo = localtime((time_t*)&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
        LOG_JWT_DEBUG("当前时间: %s", timeStr);
    }
    
    unsigned long iat = now - 30;  // 签发时间：当前时间前30秒
    unsigned long exp = now + 3600;  // 有效期1H
    
    payload["iat"] = iat;
    payload["exp"] = exp;
    
    // LOG_JWT_VERBOSE("iat: %lu, exp: %lu", iat, exp);
    String payload_json;
    serializeJson(payload, payload_json);   
    // LOG_JWT_DEBUG("Payload: %s", payload_json.c_str());
    String payload_b64 = base64url_encode((const uint8_t*)payload_json.c_str(), payload_json.length());

    // 使用Ed25519算法对header.payload进行签名
    String signing_input = header_b64 + "." + payload_b64;
    // LOG_JWT_VERBOSE("Signing input: %s", signing_input.c_str());
    
    // 从seed生成Ed25519密钥对
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
    crypto_sign_detached(signature, &siglen,
        (const unsigned char*)signing_input.c_str(), signing_input.length(), sk);
    
    // LOG_JWT_DEBUG("Signature length: %llu", siglen);

    // 对签名进行Base64URL编码
    String signature_b64 = base64url_encode(signature, siglen);

    // 拼接最终JWT (header.payload.signature)
    String jwt = signing_input + "." + signature_b64;
    LOG_JWT_INFO("JWT token generated successfully, length: %d", jwt.length());
    // LOG_JWT_VERBOSE("Final JWT: %s", jwt.c_str());
    return jwt;
}