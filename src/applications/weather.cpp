#include "weather.h"

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
unsigned int interface_num = 0; // 当前显示的界面编号

// 只负责显示天气信息
void updateWeatherScreen() {
    if(interface_num == 0){
        lcdResetCursor();
        extern char* city_name;
        if (city_name && strlen(city_name) > 0) {
            lcd_text(city_name, 1); // 第一行显示配置地名
        } else {
            lcd_text("N/A", 1);
        }

        lcdSetCursor(16); 
        
        // 安全地获取天气图标，确保指针不为空
        uint8_t* leftIcon = getWeatherLeftIcon(currentWeather);
        uint8_t* rightIcon = getWeatherRightIcon(currentWeather);
        
        Serial.println("[WEATHER] Getting icons for weather: '" + currentWeather + "'");
        Serial.println("[WEATHER] Left icon valid: " + String(leftIcon != nullptr));
        Serial.println("[WEATHER] Right icon valid: " + String(rightIcon != nullptr));
        
        if (leftIcon && rightIcon) {
            lcdCreateChar(0, leftIcon);
            lcdCreateChar(1, rightIcon);
            lcdCreateChar(2, tempIcon);
            lcdCreateChar(3, celsius);

            lcdDisCustom(0);
            lcdDisCustom(1);
            lcdDisCustom(2);
            lcdPrint(currentTemp);
            lcdDisCustom(3);
            lcdPrint(" Feel" + feelsLike);
            lcdDisCustom(3);
        } else {
            // 如果天气图标获取失败，显示文本形式
            lcdCreateChar(2, tempIcon);
            lcdCreateChar(3, celsius);
            lcdPrint(currentWeather.substring(0, 8)); // 限制长度
            lcdPrint(" ");
            lcdDisCustom(2);
            lcdPrint(currentTemp);
            lcdDisCustom(3);
        }

        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分
        
    } else if(interface_num == 1){
        lcdResetCursor();
        
        // 安全地获取风向图标
        uint8_t* windIcon = getWindIcon(windDir);
        Serial.println("[WEATHER] Getting wind icon for direction: '" + windDir + "'");
        Serial.println("[WEATHER] Wind icon valid: " + String(windIcon != nullptr));
        
        if (windIcon) {
            lcdCreateChar(4, windIcon);
            lcdPrint("Wind ");
            lcdDisCustom(4);
            lcdPrint(windScale);
        } else {
            lcdPrint("Wind:");
            lcdPrint(windDir.substring(0, 6)); // 限制长度，显示文本
            lcdPrint(" ");
            lcdPrint(windScale);
        }
        
        for(int i=lcdCursor;i<15;i++) lcdDisChar(' '); // 清除剩余部分

        lcdSetCursor(16); 
        lcdPrint("Humi:"); // 第二行显示湿度
        lcdPrint(humidity);
        // lcdPrint("%");
        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分

    } else if(interface_num == 2){
        lcdResetCursor();
        lcdPrint("Pres:");
        lcdPrint(pressure);
        // lcdPrint("hPa");
        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分

        // lcd_text("Obs:", 2); // 第二行显示观测时间
        // lcdSetCursor(16); 
        // lcdPrint(obsTime);
        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分
    }
}

// 负责网络请求和数据解析（HTTPS + gzip解压）
bool fetchWeatherData() {
    Serial.println("[DEBUG] Enter fetchWeatherData");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[DEBUG] WiFi not found");
        lcd_text("WiFi not found", 1);
        lcd_text("Weather failed", 2);
        return false;
    }

    // 检查 API 配置是否完整
    if (strlen(apiHost) == 0 || strlen(base64_key) == 0 || strlen(kid) == 0 || strlen(project_id) == 0) {
        Serial.println("[DEBUG] Missing API configuration");
        lcd_text("No API config", 1);
        lcd_text("Use web config", 2);
        return false;
    }

    // 检查城市配置是否为空
    if (!location || strlen(location) == 0) {
        Serial.println("[DEBUG] Missing city/location configuration");
        lcd_text("No City Set", 1);
        lcd_text("Use Web Config", 2);
        return false;
    }

    generateSeed32();

    Serial.println("[DEBUG] API config OK, fetching weather...");

    lcd_text("Updating weather", 1);
    lcd_text("Please wait...", 2);

    // 去掉 apiHost 和 location 前后空格
    String hostStr = String(apiHost);
    hostStr.trim();
    String locStr = String(location);
    locStr.trim();

    // 生成 JWT
    String jwtToken = generate_jwt(kid, project_id, seed32);
    Serial.println("[DEBUG] JWT token: " + jwtToken);

    // 拼接 URL
    String url = "https://" + hostStr + "/v7/weather/now?location=" + locStr;
    Serial.println("[DEBUG] Final Request URL: " + url);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Accept-Encoding", "gzip");
    http.addHeader("Authorization", "Bearer " + jwtToken);

    int httpCode = http.GET();
    Serial.printf("[DEBUG] HTTP code: %d\n", httpCode);

    if (httpCode != 200) {
        Serial.print("[DEBUG] HTTP error: ");
        Serial.println(httpCode);
        if (httpCode == 401) {
            Serial.println("[DEBUG] HTTP 401响应内容:");
            int payloadSize = http.getSize();
            if (payloadSize > 0) {
                uint8_t *pCompressed = (uint8_t *)malloc(payloadSize + 8);
                if (pCompressed) {
                    WiFiClient *stream = http.getStreamPtr();
                    long startMillis = millis();
                    int iCount = 0;
                    while (iCount < payloadSize && (millis() - startMillis) < 4000) {
                        if (stream->available()) {
                            pCompressed[iCount++] = stream->read();
                        } else {
                            vTaskDelay(5);
                        }
                    }
                    String respBody;
                    zlib_turbo zt;
                    if (iCount >= 2 && pCompressed[0] == 0x1f && pCompressed[1] == 0x8b) {
                        int uncompSize = zt.gzip_info(pCompressed, iCount);
                        if (uncompSize > 0) {
                            uint8_t *pUncompressed = (uint8_t *)malloc(uncompSize + 8);
                            if (pUncompressed) {
                                int rc = zt.gunzip(pCompressed, iCount, pUncompressed);
                                if (rc == ZT_SUCCESS) {
                                    respBody = String((char *)pUncompressed, uncompSize);
                                }
                                free(pUncompressed);
                            }
                        }
                    } else {
                        respBody = String((char *)pCompressed, iCount);
                    }
                    Serial.println("[BODY]\n" + respBody);
                    free(pCompressed);
                }
            }
        }
        lcd_text("HTTP error", 1);
        lcd_text(String(httpCode), 2);
        http.end();
        return false; // 直接返回，避免解析空数据
    }

    int payloadSize = http.getSize();
    if (payloadSize <= 0) {
        Serial.println("[DEBUG] Empty payload");
        lcd_text("Empty payload", 1);
        lcd_text("", 2);
        http.end();
        return false;
    }

    uint8_t *pCompressed = (uint8_t *)malloc(payloadSize + 8);
    if (!pCompressed) {
        Serial.println("[DEBUG] malloc failed for compressed buffer");
        lcd_text("Mem fail", 1);
        lcd_text("", 2);
        http.end();
        return false;
    }

    WiFiClient *stream = http.getStreamPtr();
    long startMillis = millis();
    int iCount = 0;
    while (iCount < payloadSize && (millis() - startMillis) < 4000) {
        if (stream->available()) {
            pCompressed[iCount++] = stream->read();
        } else {
            vTaskDelay(5);
        }
    }
    http.end();

    String jsonData;
    zlib_turbo zt;

    if (iCount >= 2 && pCompressed[0] == 0x1f && pCompressed[1] == 0x8b) {
        int uncompSize = zt.gzip_info(pCompressed, iCount);
        if (uncompSize <= 0) {
            Serial.println("[DEBUG] gzip_info failed");
            lcd_text("Gzip info fail", 1);
            lcd_text("", 2);
            free(pCompressed);
            return false;
        }
        uint8_t *pUncompressed = (uint8_t *)malloc(uncompSize + 8);
        if (!pUncompressed) {
            Serial.println("[DEBUG] malloc failed for uncompressed buffer");
            lcd_text("Mem fail", 1);
            lcd_text("", 2);
            free(pCompressed);
            return false;
        }
        int rc = zt.gunzip(pCompressed, iCount, pUncompressed);
        if (rc != ZT_SUCCESS) {
            Serial.println("[DEBUG] Gzip decompress failed");
            lcd_text("Gzip failed", 1);
            lcd_text("", 2);
            free(pUncompressed);
            free(pCompressed);
            return false;
        }
        jsonData = String((char *)pUncompressed, uncompSize);
        free(pUncompressed);
    } else {
        jsonData = String((char *)pCompressed, iCount);
    }
    free(pCompressed);

    if (jsonData.length() == 0) {
        Serial.println("[DEBUG] Empty response");
        lcd_text("Empty response", 1);
        lcd_text("", 2);
        return false;
    }

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        Serial.print("[DEBUG] JSON parse failed: ");
        Serial.println(error.c_str());
        lcd_text("JSON failed", 1);
        lcd_text("", 2);
        return false;
    }

    if (!doc.containsKey("code")) {
        Serial.println("[DEBUG] No 'code' field in response");
        lcd_text("No code field", 1);
        lcd_text("", 2);
        return false;
    }

    String code = doc["code"].as<String>();
    if (code != "200") {
        Serial.println("[DEBUG] API returned error code: " + code);
        lcd_text("API error", 1);
        lcd_text(("code:" + code).substring(0, min((int)code.length()+5, 16)), 2);
        return false;
    }

    // 成功解析，赋值天气数据（添加安全检查）
    currentCity = String(location);
    
    // 安全地获取天气数据，避免空值
    if (doc["now"]["text"].is<String>()) {
        currentWeather = doc["now"]["text"].as<String>();
        if (currentWeather.length() == 0) {
            currentWeather = "Unknown";
        }
    } else {
        currentWeather = "Unknown";
    }
    
    currentTemp = (doc["now"]["temp"].is<String>() ? doc["now"]["temp"].as<String>() : "0") + "C";
    feelsLike = (doc["now"]["feelsLike"].is<String>() ? doc["now"]["feelsLike"].as<String>() : "0") + "C";
    windDir = doc["now"]["windDir"].is<String>() ? doc["now"]["windDir"].as<String>() : "";
    windScale = doc["now"]["windScale"].is<String>() ? doc["now"]["windScale"].as<String>() : "0";
    humidity = (doc["now"]["humidity"].is<String>() ? doc["now"]["humidity"].as<String>() : "0") + "%";
    pressure = (doc["now"]["pressure"].is<String>() ? doc["now"]["pressure"].as<String>() : "0") + "hPa";
    obsTime = doc["now"]["obsTime"].is<String>() ? doc["now"]["obsTime"].as<String>() : "";
    weatherUpdateTime = doc["updateTime"].is<String>() ? doc["updateTime"].as<String>() : "";
    
    weatherSynced = true;
    lastWeatherUpdate = millis();

    Serial.println("[DEBUG] Weather data parsed and ready to display.");
    Serial.println("[DEBUG] currentWeather: " + currentWeather);
    Serial.println("[DEBUG] currentTemp: " + currentTemp);
    updateWeatherScreen();
    return true;
}