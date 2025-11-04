#include "weather.h"
#include "memory_utils.h"
#include "logger.h"

// 天气服务，API接口通过ESP32向云端获取JSON数据
bool weatherSynced = false;
String currentWeather = "N/A";
String currentTemp = "--C";
String currentCity = "N/A";
String weatherUpdateTime = "--/-- --:--";
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
        extern char city_name[64];
        if (city_name[0] != '\0' && strlen(city_name) > 0) {
            lcd_text(city_name, 1); // 第一行显示配置地名
        } else {
            lcd_text("N/A", 1);
        }

        lcdSetCursor(16); 
        
        // 安全地获取天气图标，确保指针不为空
        uint8_t* leftIcon = getWeatherLeftIcon(currentWeather);
        uint8_t* rightIcon = getWeatherRightIcon(currentWeather);
        
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
            // 如果天气图标获取失败，显示空白
            lcdPrint("  ");
        }

        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分
        
    } else if(interface_num == 1){
        lcdResetCursor();
        
        // 安全地获取风向图标
        uint8_t* windIcon = getWindIcon(windDir);
        
        if (windIcon) {
            lcdCreateChar(4, windIcon);
            lcdPrint("Wind ");
            lcdDisCustom(4);
            lcdPrint(windScale);
        } else {
            lcdPrint("Wind:");
            lcdPrint("  ");
            lcdPrint(windScale);
        }
        
        for(int i=lcdCursor;i<15;i++) lcdDisChar(' '); // 清除剩余部分

        lcdSetCursor(16); 
        lcdPrint("Humi:"); // 第二行显示湿度
        lcdPrint(humidity);
        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分

    } else if(interface_num == 2){
        lcdResetCursor();
        lcdPrint("Pres:");
        lcdPrint(pressure);
        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分
        
    } else if(interface_num == 3){
        lcdResetCursor();
        lcdPrint("Obs:");
        lcdPrint(obsTime);
        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分

        lcdSetCursor(16); 
        lcdPrint("Upd:"); // 第二行显示观测时间
        lcdPrint(weatherUpdateTime);
        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分
    }
}

// 负责网络请求和数据解析（HTTPS + gzip解压）
bool fetchWeatherData() {
    // 打印内存使用情况
    MemoryManager::printMemoryInfo("Weather fetch start");
    
    if (WiFi.status() != WL_CONNECTED) {
        LOG_WEATHER_ERROR("WiFi not connected");
        lcd_text("No WiFi", 1);
        lcd_text(" ", 2);
        return false;
    }

    // 检查 API 配置是否完整
    if (strlen(apiHost) == 0 || strlen(base64_key) == 0 || strlen(kid) == 0 || strlen(project_id) == 0) {
        LOG_WEATHER_ERROR("Missing API configuration");
        lcd_text("No API config", 1);
        lcd_text("Use web config", 2);
        return false;
    }

    // 检查城市配置是否为空
    if (!location || strlen(location) == 0) {
        LOG_WEATHER_ERROR("Missing city/location configuration");
        lcd_text("No City Set", 1);
        lcd_text("Use Web Config", 2);
        return false;
    }

    generateSeed32(); // 生成Seed32

    LOG_WEATHER_INFO("API config OK, fetching weather...");

    lcd_text("Updating weather", 1);
    lcd_text("Please wait...", 2);

    // 去掉 apiHost 和 location 前后空格
    String hostStr = String(apiHost);
    hostStr.trim();
    String locStr = String(location);
    locStr.trim();

    // 生成 JWT
    String jwtToken = generate_jwt(kid, project_id, seed32);
    LOG_WEATHER_DEBUG("JWT token: " + jwtToken);

    // 拼接 URL
    String url = "https://" + hostStr + "/v7/weather/now?location=" + locStr;
    LOG_WEATHER_DEBUG("Final Request URL: %s", url.c_str());

    HTTPClient http;
    http.begin(url);
    http.addHeader("Accept-Encoding", "gzip");                  // 请求 gzip 压缩响应
    http.addHeader("Authorization", "Bearer " + jwtToken);      // 使用 Bearer 令牌进行授权

    int httpCode = http.GET();

    if (httpCode == 200)
        LOG_WEATHER_DEBUG("HTTP code: %d", httpCode);
    else {
        LOG_WEATHER_ERROR("HTTP error: %d", httpCode);
        lcd_text("HTTP error", 1);
        lcd_text(String(httpCode), 2);
        http.end();
        return false; // 直接返回，避免解析空数据
    } 

    // 获取响应体大小
    int payloadSize = http.getSize();
    if (payloadSize <= 0) {
        LOG_WEATHER_ERROR("Empty response");
        lcd_text("Empty response", 1);
        lcd_text("", 2);
        http.end();
        return false;
    }

    // 使用RAII缓冲区
    MemoryManager::SafeBuffer compressedBuffer(payloadSize + 8, "HTTP_Response");
    if (!compressedBuffer.isValid()) {
        LOG_WEATHER_ERROR("malloc failed for compressed buffer");
        lcd_text("Mem fail", 1);
        lcd_text("", 2);
        http.end();
        return false;
    }

    // 读取数据到缓冲区
    WiFiClient *stream = http.getStreamPtr();
    long startMillis = millis();
    int iCount = 0;                 // 已读取字节数

    while (iCount < payloadSize && (millis() - startMillis) < 4000) {   // 最多等待4秒
        if (stream->available()) {
            compressedBuffer.get()[iCount++] = stream->read();
        } else {
            vTaskDelay(5);  // 延迟以避免占用过多资源
        }
    }
    if((millis() - startMillis) >= 4000){
        LOG_WEATHER_ERROR("Read timeout");
        lcd_text("Read timeout", 1);
        lcd_text("", 2);
        http.end();
        return false;
    }
    http.end();

    if (iCount == 0) {
        LOG_WEATHER_ERROR("No data received");
        lcd_text("No data received", 1);
        lcd_text("", 2);
        return false;
    }

    String jsonData;
    zlib_turbo zturbo;      // zlib_turbo 实例

    // 检查是否为 gzip 格式（检查前两个字节 0x1f 0x8b）
    if (iCount >= 2 && compressedBuffer.get()[0] == 0x1f && compressedBuffer.get()[1] == 0x8b) {
        int uncompSize = zturbo.gzip_info(compressedBuffer.get(), iCount); // 获取解压后大小
        if (uncompSize <= 0) {
            LOG_WEATHER_ERROR("get gzip_info failed");
            lcd_text("Gzip info fail", 1);
            lcd_text("", 2);
            return false;
        }

        // 使用RAII解压缓冲区
        MemoryManager::SafeBuffer uncompressedBuffer(uncompSize + 8, "Gzip_Decompressed");
        if (!uncompressedBuffer.isValid()) {
            LOG_WEATHER_ERROR("malloc failed for uncompressed buffer");
            lcd_text("Mem fail", 1);
            lcd_text("", 2);
            return false;
        }

        // 执行解压
        int unzipResult = zturbo.gunzip(compressedBuffer.get(), iCount, uncompressedBuffer.get());
        if (unzipResult != ZT_SUCCESS) {
            LOG_WEATHER_ERROR("Gzip decompress failed");
            lcd_text("Gzip failed", 1);
            lcd_text("", 2);
            return false;
        }
        jsonData = String((char *)uncompressedBuffer.get(), uncompSize);
        // uncompressedBuffer 会在作用域结束时自动释放
    } else {
        jsonData = String((char *)compressedBuffer.get(), iCount);
    }
    // compressedBuffer 会在作用域结束时自动释放

    if (jsonData.length() == 0) {
        LOG_WEATHER_ERROR("Empty response");
        lcd_text("Empty response", 1);
        lcd_text("", 2);
        return false;
    }

    // 解析Json
    JsonDocument doc;       // 自动选择合适的内存分配器
    DeserializationError error = deserializeJson(doc, jsonData);    // 反序列化JSON
    if (error) {
        LOG_WEATHER_ERROR("JSON parse failed: %s", error.c_str());
        lcd_text("JSON failed", 1);
        lcd_text("", 2);
        return false;
    }

    // 成功解析，赋值天气数据
    currentCity = String(location);
    
    // 安全地获取天气数据，避免空值
    if (doc["now"]["text"].is<String>()) {
        currentWeather = doc["now"]["text"].as<String>();
        if (currentWeather.length() == 0) {
            LOG_WEATHER_WARN("Empty weather text");
            currentWeather = "Unknown";
        }
    } else {
        LOG_WEATHER_WARN("unknown weather text");
        currentWeather = "Unknown";
    }
    
    // 安全地获取其他数据
    currentTemp = (doc["now"]["temp"].is<String>() ? doc["now"]["temp"].as<String>() : "?") + "C";
    feelsLike = (doc["now"]["feelsLike"].is<String>() ? doc["now"]["feelsLike"].as<String>() : "?") + "C";
    windDir = doc["now"]["windDir"].is<String>() ? doc["now"]["windDir"].as<String>() : "";
    windScale = doc["now"]["windScale"].is<String>() ? doc["now"]["windScale"].as<String>() : "?";
    humidity = (doc["now"]["humidity"].is<String>() ? doc["now"]["humidity"].as<String>() : "?") + "%";
    pressure = (doc["now"]["pressure"].is<String>() ? doc["now"]["pressure"].as<String>() : "?") + "hPa";
    
    // 格式化 obsTime 为 MM/DD HH:MM
    String rawObsTime = doc["now"]["obsTime"].is<String>() ? doc["now"]["obsTime"].as<String>() : "";
    if (rawObsTime.length() >= 16) {
        // 格式为ISO8601：2023-11-01T14:30:00+08:00
        String month = rawObsTime.substring(5, 7);   // 提取月份
        String day = rawObsTime.substring(8, 10);    // 提取日期
        String hour = rawObsTime.substring(11, 13);  // 提取小时
        String minute = rawObsTime.substring(14, 16); // 提取分钟
        obsTime = month + "/" + day + " " + hour + ":" + minute;
    } else {
        obsTime = "--/-- --:--";
    }
    
    // 格式化 weatherUpdateTime 为 MM/DD HH:MM
    String rawUpdateTime = doc["updateTime"].is<String>() ? doc["updateTime"].as<String>() : "";
    if (rawUpdateTime.length() >= 16) {
        // 格式为ISO8601
        String month = rawUpdateTime.substring(5, 7);   // 提取月份
        String day = rawUpdateTime.substring(8, 10);    // 提取日期
        String hour = rawUpdateTime.substring(11, 13);  // 提取小时
        String minute = rawUpdateTime.substring(14, 16); // 提取分钟
        weatherUpdateTime = month + "/" + day + " " + hour + ":" + minute;
    } else {
        weatherUpdateTime = "--/-- --:--";
    }
    
    weatherSynced = true;
    lastWeatherUpdate = millis();

    // 打印内存使用情况
    MemoryManager::printMemoryInfo("Weather fetch complete");
    
    LOG_WEATHER_INFO("Weather updated: %s, %s", currentWeather.c_str(), currentTemp.c_str());
    updateWeatherScreen();
    return true;
}