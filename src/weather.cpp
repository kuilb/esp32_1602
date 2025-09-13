#include "weather.h"
#include <HTTPClient.h>
#include <zlib_turbo.h>



// Base64URL编码
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
    // Base64URL不需要填充
    return b64;
}

// 生成JWT Token
String generate_jwt(const String& kid, const String& project_id, const uint8_t* seed32) {
    //Serial.println("[DEBUG] start generate_jwt...");

    // Header
    StaticJsonDocument<128> header;
    header["alg"] = "EdDSA";
    header["kid"] = kid;
    String header_json;
    serializeJson(header, header_json);
    String header_b64 = base64url_encode((const uint8_t*)header_json.c_str(), header_json.length());
    //Serial.println("[DEBUG] header_b64: " + header_b64);

    // Payload
    StaticJsonDocument<128> payload;
    unsigned long now = time(nullptr);
    payload["sub"] = project_id;
    payload["iat"] = now - 30;
    payload["exp"] = now + 8000; // 有效期8000秒
    //Serial.println("[DEBUG] sub: " + project_id);
    //Serial.printf("[DEBUG] iat: %lu\n", now - 30);
    //Serial.printf("[DEBUG] exp: %lu\n", now + 8000);

    String payload_json;
    serializeJson(payload, payload_json);
    String payload_b64 = base64url_encode((const uint8_t*)payload_json.c_str(), payload_json.length());
    //Serial.println("[DEBUG] payload_b64: " + payload_b64);

    // 签名
    String signing_input = header_b64 + "." + payload_b64;
    uint8_t pk[crypto_sign_PUBLICKEYBYTES], sk[crypto_sign_SECRETKEYBYTES];
    crypto_sign_seed_keypair(pk, sk, seed32);
    //Serial.println("[DEBUG] Public Key: " + String((char*)pk));
    //Serial.println("[DEBUG] Secret Key: " + String((char*)sk));

    uint8_t signature[crypto_sign_BYTES];
    unsigned long long siglen;
    crypto_sign_detached(signature, &siglen,
        (const unsigned char*)signing_input.c_str(), signing_input.length(), sk);

    String signature_b64 = base64url_encode(signature, siglen);

    // 拼接JWT
    return signing_input + "." + signature_b64;
}

// 你的API参数
const char* apiHost = "k2436grq42.re.qweatherapi.com"; // 和风天气API Host
const char* location = "101010100"; // 北京LocationID
const String kid = "K75DVFV3J5";
const String project_id = "4KDX498E6E";
//const uint8_t seed32[32] = { /* 你的Ed25519 seed，32字节 */ };
const uint8_t seed32[32] = {
  0x2C, 0x89, 0x83, 0xA2, 0xDD, 0x37, 0xA2, 0x10,
  0xB3, 0x33, 0xBD, 0xEE, 0x8F, 0xFE, 0xFF, 0x59,
  0xA2, 0x60, 0x6D, 0xEC, 0x43, 0x24, 0xAB, 0xAE,
  0x4E, 0xDA, 0x75, 0xE5, 0xDF, 0x07, 0xA2, 0x8A
}; 

// 天气数据
bool weatherSynced = false;
String currentWeather = "N/A";
String currentTemp = "--C";
String currentCity = "N/A";
String weatherUpdateTime = "--:--";
String feelsLike = "--C";
String windDir = "";
String windScale = "";
String humidity = "";
String pressure = "";
String obsTime = "";
unsigned long lastWeatherUpdate = 0;

// 只负责显示天气信息
void updateWeatherScreen() {
    lcd_text("Beijing", 1); // 第一行显示城市（最多16字符）
    String line2 = (getWeatherInEnglish(currentWeather)  + " T" + currentTemp +" "+"W"+windScale); // 第二行天气+温度
    lcd_text(line2, 2);
    Serial.println("weather:"+currentWeather+" temp:"+currentTemp+" city:"+currentCity+" time:"+weatherUpdateTime+" feels:"+feelsLike+" wind:"+windDir+windScale+" humi:"+humidity+" pres:"+pressure+" obs:"+obsTime);
}

// 负责网络请求和数据解析（HTTPS + gzip解压）
void fetchWeatherData() {
    Serial.println("[DEBUG] Enter fetchWeatherData");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[DEBUG] WiFi not found");
        lcd_text("WiFi not found", 1);
        lcd_text("Weather failed", 2);
        return;
    }

    Serial.println("[DEBUG] Generating JWT token...");

    // 检查参数
    Serial.print("[DEBUG] kid: ");
    Serial.println(kid);
    
    Serial.print("[DEBUG] project_id: ");
    Serial.println(project_id);

    if (seed32 == nullptr) {
        Serial.println("[ERROR] seed32 is null!");
    }
    
    Serial.print("[DEBUG] seed32: ");
    for (int i = 0; i < 32; ++i) {
        Serial.printf("%02X ", seed32[i]);
    }
    Serial.println();

    String jwtToken = generate_jwt(kid, project_id, seed32);
    Serial.println("[DEBUG] JWT token: " + jwtToken);

    String url = String("https://") + apiHost + "/v7/weather/now?location=" + location;
    Serial.println("[DEBUG] Request URL: " + url);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Accept-Encoding", "gzip");
    http.addHeader("Authorization", "Bearer " + jwtToken);

    Serial.println("[DEBUG] Sending HTTP GET...");
    int httpCode = http.GET();
    Serial.printf("[DEBUG] HTTP code: %d\n", httpCode);

    if (httpCode != 200) {
        Serial.print("[DEBUG] HTTP error: ");
        Serial.println(httpCode);
        lcd_text("HTTP error", 1);
        lcd_text(String(httpCode), 2);
        http.end();
        return;
    }

    int payloadSize = http.getSize();
    Serial.printf("[DEBUG] Payload size: %d\n", payloadSize);

    uint8_t *pCompressed = (uint8_t *)malloc(payloadSize + 8);
    if (!pCompressed) {
        Serial.println("[DEBUG] malloc failed for compressed buffer");
        lcd_text("Mem fail", 1);
        lcd_text("", 2);
        http.end();
        return;
    }

    WiFiClient *stream = http.getStreamPtr();
    long startMillis = millis();
    int iCount = 0;
    Serial.println("[DEBUG] Start reading compressed data...");
    while (iCount < payloadSize && (millis() - startMillis) < 4000) {
        if (stream->available()) {
            pCompressed[iCount++] = stream->read();
        } else {
            vTaskDelay(5);
        }
    }
    Serial.printf("[DEBUG] Compressed data read: %d bytes\n", iCount);
    http.end();

    String jsonData;
    zlib_turbo zt;
    if (pCompressed[0] == 0x1f && pCompressed[1] == 0x8b) {
        Serial.println("[DEBUG] It's a gzip file!");
        int uncompSize = zt.gzip_info(pCompressed, payloadSize);
        Serial.printf("[DEBUG] gzip_info returned: %d\n", uncompSize);
        if (uncompSize > 0) {
            uint8_t *pUncompressed = (uint8_t *)malloc(uncompSize + 8);
            if (!pUncompressed) {
                Serial.println("[DEBUG] malloc failed for uncompressed buffer");
                lcd_text("Mem fail", 1);
                lcd_text("", 2);
                free(pCompressed);
                return;
            }
            //Serial.println("[DEBUG] Start gunzip...");
            int rc = zt.gunzip(pCompressed, payloadSize, pUncompressed);
            //Serial.printf("[DEBUG] gunzip returned: %d\n", rc);
            if (rc == ZT_SUCCESS) {
                //Serial.printf("[DEBUG] Uncompressed size = %d bytes\n", uncompSize);
                jsonData = String((char *)pUncompressed, uncompSize);
                free(pUncompressed);
            } else {
                //Serial.println("[DEBUG] Gzip decompress failed");
                lcd_text("Gzip failed", 1);
                lcd_text("", 2);
                free(pUncompressed);
                free(pCompressed);
                return;
            }
        } else {
            //Serial.println("[DEBUG] gzip_info failed");
            lcd_text("Gzip info fail", 1);
            lcd_text("", 2);
            free(pCompressed);
            return;
        }
    } else {
        //Serial.println("[DEBUG] Not a gzip file, try as plain text");
        jsonData = String((char *)pCompressed, payloadSize);
    }
    free(pCompressed);

    //Serial.println("[DEBUG] Weather API JSON:");
    Serial.println(jsonData);

    //Serial.println("[DEBUG] Start JSON parsing...");
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        Serial.print("[DEBUG] JSON parse failed: ");
        Serial.println(error.c_str());
        lcd_text("JSON failed", 1);
        lcd_text("", 2);
        return;
    }

    String code = doc["code"].as<String>();
    Serial.println("[DEBUG] code: " + code);
    if (code != "200") {
        lcd_text("API error", 1);
        lcd_text(("code:" + code).substring(0, 16), 2);
        return;
    }

    currentCity = String(location);
    currentWeather = doc["now"]["text"].as<String>();
    currentTemp = doc["now"]["temp"].as<String>() + "C";
    feelsLike = doc["now"]["feelsLike"].as<String>() + "C";
    windDir = doc["now"]["windDir"].as<String>();
    windScale = doc["now"]["windScale"].as<String>();
    humidity = doc["now"]["humidity"].as<String>() + "%";
    pressure = doc["now"]["pressure"].as<String>() + "hPa";
    obsTime = doc["now"]["obsTime"].as<String>();
    weatherUpdateTime = doc["updateTime"].as<String>();
    weatherSynced = true;
    lastWeatherUpdate = millis();

    Serial.println("[DEBUG] Weather data parsed and ready to display.");
    updateWeatherScreen();
}