#include "jwt_auth.h"

// API参数
char apiHost[128] = "";
char kid[64] = "";
char project_id[64] = "";
char base64_key[256] = "";
char* location = ""; // LocationID
char* city_name = ""; // 地名

//Ed25519 seed，32字节
uint8_t seed32[32] = {}; 

// 初始化 JWT 配置
void init_jwt() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS 初始化失败");
        return;
    }

    if (!SPIFFS.exists("/jwt_config.txt")) {
        Serial.println("JWT 配置文件不存在");
        return;
    }

    File file = SPIFFS.open("/jwt_config.txt", "r");
    if (!file) {
        Serial.println("打开 JWT 配置文件失败");
        return;
    }

    file.readBytesUntil('\n', apiHost, sizeof(apiHost));
    file.readBytesUntil('\n', kid, sizeof(kid));
    file.readBytesUntil('\n', project_id, sizeof(project_id));
    file.readBytesUntil('\n', base64_key, sizeof(base64_key));
    // 读取locationID（第五行）
    char location_buf[32] = "";
    if (file.available()) {
        file.readBytesUntil('\n', location_buf, sizeof(location_buf));
        if (strlen(location_buf) > 0) {
            location = strdup(location_buf);
        }
    }
    // 读取地名（第六行）
    char city_buf[64] = "";
    if (file.available()) {
        file.readBytesUntil('\n', city_buf, sizeof(city_buf));
        if (strlen(city_buf) > 0) {
            city_name = strdup(city_buf);
        }
    }
    file.close();

    Serial.println("JWT 配置已加载:");
    Serial.println("apiHost: " + String(apiHost));
    Serial.println("kid: " + String(kid));
    Serial.println("project_id: " + String(project_id));
    Serial.println("base64_key: " + String(base64_key));
    Serial.println("locationID: " + String(location));
    Serial.println("city_name: " + String(city_name));
}


// -------- Base64 Decode --------
const int8_t b64_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
};

int base64_decode(const char* input, uint8_t* output) {
    int len = 0;
    int val = 0;
    int valb = -8;
    for (int i = 0; input[i]; i++) {
        int8_t c = b64_table[(uint8_t)input[i]];
        if (c == -1) continue;
        if (c == -2) break;
        val = (val << 6) | c;
        valb += 6;
        if (valb >= 0) {
            output[len++] = (val >> valb) & 0xFF;
            valb -= 8;
        }
    }
    return len;
}

// -------- Extract seed32 from PKCS#8 DER --------
bool extract_ed25519_seed(const uint8_t* der, size_t der_len, uint8_t* seed32) {
    Serial.println("[DER] Extracting Ed25519 seed from DER data...");
    Serial.println("[DER] DER length: " + String(der_len));
    
    for (size_t i = 0; i < der_len - 1; i++) {
        // 查找连续 0x04 0x20 开头的 32 字节数据
        if (der[i] == 0x04 && der[i+1] == 0x20 && (i + 34) <= der_len) {
            Serial.println("[DER] Found Ed25519 seed pattern at offset: " + String(i));
            memcpy(seed32, der + i + 2, 32);
            Serial.println("[DER] Successfully extracted 32-byte seed");
            return true;
        }
    }
    // 打印调试信息
    Serial.println("[DER] ERROR: Failed to extract seed32! DER bytes:");
    for (size_t i = 0; i < der_len && i < 64; i++) { // 限制输出长度
        Serial.printf("%02X ", der[i]);
        if ((i + 1) % 16 == 0) Serial.println();
    }
    if (der_len > 64) Serial.println("... (truncated)");
    Serial.println();
    return false;
}

// Base64 decode + 提取 seed 函数
void generateSeed32() {
    Serial.println("[SEED] Generating seed32 from base64 key...");
    
    if (strlen(base64_key) == 0) {
        Serial.println("[SEED] ERROR: No base64 key, cannot generate seed32");
        return;
    }

    Serial.println("[SEED] Base64 key length: " + String(strlen(base64_key)));
    Serial.println("[SEED] Base64 key (first 20 chars): " + String(base64_key).substring(0, 20) + "...");

    uint8_t der[128];
    int der_len = base64_decode(base64_key, der);
    Serial.println("[SEED] DER decoded length: " + String(der_len));
    
    if(!extract_ed25519_seed(der, der_len, seed32)) {
        Serial.println("[SEED] ERROR: Failed to extract seed32!");
        return;
    }
    Serial.print("[SEED] Extracted Seed32: ");
    for(int i=0;i<32;i++){
        Serial.printf("%02X", seed32[i]);
    }
    Serial.println();
    Serial.println("[SEED] Seed32 generation completed successfully");
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
    
    // Base64URL规范：移除末尾的填充字符 '='
    // 注意：这里不需要添加填充字符，因为JWT规范要求移除它们
    
    return b64;
}

// 生成JWT Token
String generate_jwt(const String& kid, const String& project_id, const uint8_t* seed32) {
    // 去掉前后空格
    String kid_trimmed = kid;
    kid_trimmed.trim();
    String project_trimmed = project_id;
    project_trimmed.trim();

    // Header（严格按照和风天气规范：只含alg和kid）
    StaticJsonDocument<128> header;
    header["alg"] = "EdDSA";  // 必须是EdDSA
    header["kid"] = kid_trimmed;  // 凭据ID
    // 注意：不添加typ、iss、aud、nbf等保留字段
    String header_json;
    serializeJson(header, header_json);
    Serial.println("[JWT] Header: " + header_json);
    String header_b64 = base64url_encode((const uint8_t*)header_json.c_str(), header_json.length());

    // Payload（严格按照和风天气规范：只含sub、iat、exp）
    StaticJsonDocument<128> payload;
    payload["sub"] = project_trimmed;  // 签发主体：项目ID
    unsigned long now = time(nullptr);
    
    // 检查时间是否已同步 - 更严格的检查
    if (now < 1609459200) { // 2021年1月1日的时间戳，如果小于这个值说明时间肯定不对
        Serial.println("[JWT] ERROR: System time not synchronized! Current timestamp: " + String(now));
        Serial.println("[JWT] Please ensure NTP time sync is working");
        // 即使时间不对，也继续生成JWT，但会在调试中明确标识
    } else {
        Serial.println("[JWT] System time OK, timestamp: " + String(now));
        // 打印可读时间用于调试
        struct tm* timeinfo = localtime((time_t*)&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
        Serial.println("[JWT] Current time: " + String(timeStr));
    }
    
    unsigned long iat = now - 30;  // 签发时间：当前时间前30秒（防止时间误差）
    unsigned long exp = now + 3600;  // 过期时间：1小时后（符合24小时内的要求）
    
    payload["iat"] = iat;
    payload["exp"] = exp;
    
    Serial.println("[JWT] iat: " + String(iat) + ", exp: " + String(exp));
    // 注意：不添加iss、aud、nbf、typ等保留字段
    String payload_json;
    serializeJson(payload, payload_json);
    Serial.println("[JWT] Payload: " + payload_json);
    String payload_b64 = base64url_encode((const uint8_t*)payload_json.c_str(), payload_json.length());

    // 签名：使用Ed25519算法对header.payload进行签名
    String signing_input = header_b64 + "." + payload_b64;
    Serial.println("[JWT] Signing input: " + signing_input);
    
    // 从seed生成Ed25519密钥对
    uint8_t pk[crypto_sign_PUBLICKEYBYTES], sk[crypto_sign_SECRETKEYBYTES];
    crypto_sign_seed_keypair(pk, sk, seed32);
    
    // 打印公钥用于调试
    Serial.print("[JWT] Public key: ");
    for(int i = 0; i < 8; i++) { // 只打印前8字节
        Serial.printf("%02X", pk[i]);
    }
    Serial.println("...");
    
    // 使用Ed25519算法生成分离签名
    uint8_t signature[crypto_sign_BYTES];
    unsigned long long siglen;
    crypto_sign_detached(signature, &siglen,
        (const unsigned char*)signing_input.c_str(), signing_input.length(), sk);
    
    Serial.println("[JWT] Signature length: " + String(siglen));
    // 对签名进行Base64URL编码
    String signature_b64 = base64url_encode(signature, siglen);

    // 拼接最终JWT：header.payload.signature
    String jwt = signing_input + "." + signature_b64;
    Serial.println("[JWT] Final JWT length: " + String(jwt.length()));
    Serial.println("[JWT] Final JWT: " + jwt);
    return jwt;
}