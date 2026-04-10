#include "./applications/weather.h"
#include "./hardware/buzzer.h"

// 天气服务，API接口通过ESP32向云端获取JSON数据
extern QWeatherAuthConfigManager qweatherAuthConfigManager;

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
static bool s_weatherIsNewInterface = false;
static bool s_weatherReadyToDisplay = false;
static unsigned long s_lastWeatherFail = 0;
static unsigned long s_lastWeatherFailSoundMs = 0;

static void _playWeatherFailSoundThrottled(unsigned long intervalMs = 2500) {
    const unsigned long now = millis();
    if (now - s_lastWeatherFailSoundMs < intervalMs) {
        return;
    }
    s_lastWeatherFailSoundMs = now;
    buzzerPlayError();
}

static bool _ensureWeatherTimeSynced() {
    if (!(timeSyncState == TIME_SYNC_SUCCESS) && !(getRtcTime().tv_sec > 1765967312)) { // 2025-12-17 18:40 GMT+8
        lcdText("Try time sync", 1);
        lcdText("Please wait", 2);
        updateTimeSync();
        if (timeSyncState != TIME_SYNC_SUCCESS) {
            LOG_WEATHER_WARN("Time not synced yet, cannot display weather");
            lcdText("Time not synced", 1);
            lcdText("", 2);
            _playWeatherFailSoundThrottled();
            delay(500);
            return false;
        }
    }
    return true;
}

void enterWeatherInterface() {
    s_weatherIsNewInterface = true;
    setCurrentInterface(handleWeatherInterface);
}

void handleWeatherInterface() {
    if (!_ensureWeatherTimeSynced()) {
        clearCurrentInterface();
        globalButtonDelay(FIRST_TIME_DELAY);
        return;
    }

    // 进入天气界面时先做本地前置校验，避免被失败重试冷却窗口“卡住”。
    if (WiFi.status() != WL_CONNECTED) {
        lcdText("No WiFi", 1);
        lcdText(" ", 2);
        _playWeatherFailSoundThrottled();
        clearCurrentInterface();
        globalButtonDelay(FIRST_TIME_DELAY);
        return;
    }
    if (!qweatherAuthConfigManager.checkApiConfigValid()) {
        lcdText("No API config", 1);
        lcdText("Use web config", 2);
        _playWeatherFailSoundThrottled();
        clearCurrentInterface();
        globalButtonDelay(FIRST_TIME_DELAY);
        delay(600);
        return;
    }
    if (!qweatherAuthConfigManager.checkLocationConfigValid()) {
        lcdText("No City Set", 1);
        lcdText("Use Web Config", 2);
        _playWeatherFailSoundThrottled();
        clearCurrentInterface();
        globalButtonDelay(FIRST_TIME_DELAY);
        delay(600);
        return;
    }

    // 每隔10分钟更新一次天气数据
    if (!s_weatherReadyToDisplay || millis() - lastWeatherUpdate > 10 * 60 * 1000) {
        if (millis() - s_lastWeatherFail > 15 * 1000) { // 失败后15秒再试
            loadJwtConfig();
            LOG_WEATHER_INFO("Fetching weather data...");
            if (fetchWeatherData()) {
                s_weatherReadyToDisplay = true;
                lastWeatherUpdate = millis();
            } else {
                LOG_WEATHER_WARN("Failed to fetch weather data");
                _playWeatherFailSoundThrottled(4000);
                s_weatherReadyToDisplay = false;
                s_lastWeatherFail = millis();
                clearCurrentInterface();
                globalButtonDelay(FIRST_TIME_DELAY);
                return;
            }
        }
    }

    if (s_weatherIsNewInterface) {
        s_weatherIsNewInterface = false;
        if (s_weatherReadyToDisplay) {
            updateWeatherScreen();
        }
    }

    if (isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)) {
        LOG_WEATHER_INFO("Exit weather interface to menu");
        clearCurrentInterface();
        globalButtonDelay(FIRST_TIME_DELAY);
        return;
    }

    if (!s_weatherReadyToDisplay) {
        // 失败时已显示具体错误信息，并在上方直接返回菜单，这里不再二次覆盖为 No Data。
        return;
    }

    if (isButtonReadyToRespond(LEFT, BUTTON_DEBOUNCE_DELAY)) {
        interface_num = (interface_num + 3) % 4; // 切换到上一个界面
        updateWeatherScreen();
    }
    if (isButtonReadyToRespond(RIGHT, BUTTON_DEBOUNCE_DELAY)) {
        interface_num = (interface_num + 1) % 4; // 切换到下一个界面
        updateWeatherScreen();
    }
}

// 只负责显示天气信息
void updateWeatherScreen() {
    if(interface_num == 0){
        lcdResetCursor();
        if (currentCity != "N/A" && currentCity.length() > 0) {
            lcdText(currentCity.c_str(), 1); // 第一行显示配置地名
        } else {
            lcdText("N/A", 1);
        }

        lcdSetCursor(16); 
        
        lcdCreateCharAuto(WeatherIcons::getLeftIcon(currentWeather));
        lcdCreateCharAuto(WeatherIcons::getRightIcon(currentWeather));
        lcdCreateCharAuto(SystemIcons::tempIcon);

        lcdPrint(currentTemp);
        lcdCreateCharAuto(SystemIcons::celsius);
        lcdPrint(" Fel " + feelsLike);
        lcdCreateCharAuto(SystemIcons::celsius);


        for(int i=lcdCursor;i<32;i++) lcdDisChar(' '); // 清除剩余部分
        
    } else if(interface_num == 1){
        lcdClear();
        
        lcdPrint("Wind ");
        lcdCreateCharAuto(WindIcons::getIcon(windDir));
        lcdPrint(windScale);

        lcdSetCursor(16); 
        lcdPrint("Humi:"); // 第二行显示湿度
        lcdPrint(humidity);

    } else if(interface_num == 2){
        lcdClear();
        lcdPrint("Pres:");
        lcdPrint(pressure);
        lcdText(" ",2);
        
    } else if(interface_num == 3){
        lcdClear();
        lcdPrint("Obs:");
        lcdPrint(obsTime);

        lcdSetCursor(16); 
        lcdPrint("Upd:"); // 第二行显示观测时间
        lcdPrint(weatherUpdateTime);
    }
}

// 负责网络请求和数据解析（HTTPS + gzip解压）
bool fetchWeatherData() {
    // 打印内存使用情况
    MemoryManager::printMemoryInfo("Weather fetch start");
    
    if (WiFi.status() != WL_CONNECTED) {
        LOG_WEATHER_ERROR("WiFi not connected");
        lcdText("No WiFi", 1);
        lcdText(" ", 2);
        _playWeatherFailSoundThrottled();
        return false;
    }

    // 检查 API 配置是否完整
    if (!qweatherAuthConfigManager.checkApiConfigValid()) {
        LOG_WEATHER_ERROR("Missing API configuration");
        lcdText("No API config", 1);
        lcdText("Use web config", 2);
        _playWeatherFailSoundThrottled();
        delay(1000);
        return false;
    }

    // 检查城市配置是否为空
    if (!qweatherAuthConfigManager.checkLocationConfigValid()) {
        LOG_WEATHER_ERROR("Missing city/location configuration");
        lcdText("No City Set", 1);
        lcdText("Use Web Config", 2);
        _playWeatherFailSoundThrottled();
        delay(1000);
        return false;
    }

    generateSeed32(); // 生成Seed32

    LOG_WEATHER_INFO("API config OK, fetching weather...");

    lcdText("Updating weather", 1);
    lcdText("Please wait...", 2);

    // 生成 JWT
    String jwtToken = generate_jwt(qweatherAuthConfigManager.getKId(), qweatherAuthConfigManager.getProjectID(), seed32);
    LOG_WEATHER_DEBUG("JWT token: " + jwtToken);

    // 拼接 URL
    String url = "https://" + qweatherAuthConfigManager.getApiHost() + "/v7/weather/now?location=" + qweatherAuthConfigManager.getLocation();
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
        lcdText("HTTP error", 1);
        lcdText(String(httpCode), 2);
        _playWeatherFailSoundThrottled();
        http.end();
        return false; // 直接返回，避免解析空数据
    } 

    // 获取响应体大小
    int payloadSize = http.getSize();
    if (payloadSize <= 0) {
        LOG_WEATHER_ERROR("Empty response");
        lcdText("Empty response", 1);
        lcdText("", 2);
        _playWeatherFailSoundThrottled();
        http.end();
        return false;
    }

    // 使用RAII缓冲区
    MemoryManager::SafeBuffer compressedBuffer(payloadSize + 8, "HTTP_Response");
    if (!compressedBuffer.isValid()) {
        LOG_WEATHER_ERROR("malloc failed for compressed buffer");
        lcdText("Mem fail", 1);
        lcdText("", 2);
        _playWeatherFailSoundThrottled();
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
        lcdText("Read timeout", 1);
        lcdText("", 2);
        _playWeatherFailSoundThrottled();
        http.end();
        return false;
    }
    http.end();

    if (iCount == 0) {
        LOG_WEATHER_ERROR("No data received");
        lcdText("No data received", 1);
        lcdText("", 2);
        _playWeatherFailSoundThrottled();
        return false;
    }

    String jsonData;
    zlib_turbo zturbo;      // zlib_turbo 实例

    // 检查是否为 gzip 格式（检查前两个字节 0x1f 0x8b）
    if (iCount >= 2 && compressedBuffer.get()[0] == 0x1f && compressedBuffer.get()[1] == 0x8b) {
        int uncompSize = zturbo.gzip_info(compressedBuffer.get(), iCount); // 获取解压后大小
        if (uncompSize <= 0) {
            LOG_WEATHER_ERROR("get gzip_info failed");
            lcdText("Gzip info fail", 1);
            lcdText("", 2);
            _playWeatherFailSoundThrottled();
            return false;
        }

        // 使用RAII解压缓冲区
        MemoryManager::SafeBuffer uncompressedBuffer(uncompSize + 8, "Gzip_Decompressed");
        if (!uncompressedBuffer.isValid()) {
            LOG_WEATHER_ERROR("malloc failed for uncompressed buffer");
            lcdText("Mem fail", 1);
            lcdText("", 2);
            _playWeatherFailSoundThrottled();
            return false;
        }

        // 执行解压
        int unzipResult = zturbo.gunzip(compressedBuffer.get(), iCount, uncompressedBuffer.get());
        if (unzipResult != ZT_SUCCESS) {
            LOG_WEATHER_ERROR("Gzip decompress failed");
            lcdText("Gzip failed", 1);
            lcdText("", 2);
            _playWeatherFailSoundThrottled();
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
        lcdText("Empty response", 1);
        lcdText("", 2);
        _playWeatherFailSoundThrottled();
        return false;
    }

    // 解析Json
    JsonDocument doc;       // 自动选择合适的内存分配器
    DeserializationError error = deserializeJson(doc, jsonData);    // 反序列化JSON
    if (error) {
        LOG_WEATHER_ERROR("JSON parse failed: %s", error.c_str());
        lcdText("JSON failed", 1);
        lcdText("", 2);
        _playWeatherFailSoundThrottled();
        return false;
    }

    // 成功解析，赋值天气数据
    currentCity = qweatherAuthConfigManager.getCityName();
    
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