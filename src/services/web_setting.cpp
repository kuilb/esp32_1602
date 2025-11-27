#include "web_setting.h"

extern QWeatherAuthConfigManager qweatherAuthConfigManager;

WebServer settingServer(80);
volatile bool isConfigDone = false;
volatile bool isKeyDone = false;
volatile bool otaUploadSuccess = false;
static size_t otaExpectedSize = 0;      // 预期的OTA文件大小

// OTA页面处理
void webSettingHandleOTA() {
    // 计算总长度
    unsigned int len1 = strlen_P(webComponent);
    unsigned int len2 = strlen_P(ota_html);
    unsigned int totalLen = len1 + len2;

    // 设置 Content-Length 并发送头（空 body）
    settingServer.setContentLength(totalLen);
    settingServer.send(200, "text/html; charset=utf-8", "");

    // 直接发送 PROGMEM 内容块
    settingServer.sendContent_P(webComponent, len1);
    settingServer.sendContent_P(ota_html, len2);
}

// OTA URL处理
void webSettingHandleOTAURL() {
    // 只处理GET请求
    if (settingServer.method() != HTTP_GET) {
        settingServer.send(405, "application/json; charset=utf-8", "{\"error\":\"请使用 GET 方法上传 URL\"}");
        LOG_WEB_WARN("Received non-GET request for OTA URL");
        return;
    }

    // 如果发送的请求不含URL那么 HTTP400 Bad Request
    if (!settingServer.hasArg("url")) {
        settingServer.send(400, "application/json", "{\"success\":false,\"error\":\"请提供 URL\"}");
        return;
    }
    
    // 如果已有 OTA 在进行那么 HTTP409 Conflict
    if (OTAManager::isInProgress()) {
        settingServer.send(409, "application/json", 
            "{\"success\":false,\"error\":\"当前正在进行 OTA\"}");
        return;
    }
    
    String url = settingServer.arg("url");
    LOG_SYSTEM_INFO("OTA from URL: %s", url.c_str());
    
    // 先响应前端,告诉它 OTA 已开始 (HTTP 202 Accepted)
    settingServer.send(202, "application/json", 
        "{\"success\":true,\"message\":\"OTA started\"}");
    
    // 创建独立任务执行 OTA
    xTaskCreate([](void* param) {
        String* urlPtr = (String*)param;
        bool useHTTPS;
        if(urlPtr->startsWith("https://")){
            useHTTPS = true;
        } else {
            useHTTPS = false;
        }
        OTAResult result = OTAManager::updateFromURL(*urlPtr, useHTTPS);
        if (result != OTA_SUCCESS) {
            LOG_SYSTEM_ERROR("OTA failed: %s", OTAManager::getErrorString().c_str());
        }
        delete urlPtr;
        vTaskDelete(NULL);
    }, "OTA_Task", 8192, new String(url), 5, NULL);
}

// OTA进度查询
void webSettingHandleOTAProgress() {
    int progress = OTAManager::getProgress();
    OTAStatus status = OTAManager::getStatus();
    String statusStr;
    
    switch(status) {
        case OTA_IDLE:
            statusStr = "\"idle\"";
            break;
        case OTA_RUNNING:
            statusStr = "\"in_progress\"";
            break;
        case OTA_COMPLETED_SUCCESS:
            statusStr = "\"success\"";
            break;
        case OTA_COMPLETED_FAILED:
            statusStr = "\"failed\"";
            break;
        default:
            statusStr = "\"unknown\"";
            break;
    }

    String json = "";
    String errorJson = ",\"error\":\"";

    // 有错误信息时提示客户端
    if(OTAManager::getErrorString() != ""){
        json = "{\"progress\":\"0\",\"status\":\"failed\",\"error\":\"" + OTAManager::getErrorString() + "\"}";
        LOG_SYSTEM_DEBUG("OTA Progress queried with error: " + json);
    }
    
    // 无错误信息时正常返回进度和状态
    else{
        json = "{\"progress\":" + String(progress) + ",\"status\":" + statusStr + "}";
        LOG_SYSTEM_DEBUG("OTA Progress queried: " + json);
    }

    settingServer.send(200, "application/json", json);
}

// OTA文件上传处理
void webSettingHandleOTAUpload() {
    if(settingServer.method() != HTTP_POST) {
        settingServer.send(405, "application/json; charset=utf-8", "{\"error\":\"请使用 POST 方法上传固件\"}");
        LOG_WEB_WARN("Received non-POST request for OTA upload");
        return;
    }

    HTTPUpload& upload = settingServer.upload();

    // 只处理三种合法状态，如果没有文件那么 HTTP400 Bad Request
    if (upload.status != UPLOAD_FILE_START && upload.status != UPLOAD_FILE_WRITE && upload.status != UPLOAD_FILE_END) {
        LOG_WEB_WARN("OTA upload: unexpected upload.status=%d", upload.status);
        settingServer.send(400, "application/json; charset=utf-8", "{\"error\":\"无效的上传状态\"}");
        return;
    }
    
    // 请求上传阶段
    if (upload.status == UPLOAD_FILE_START){
        // 有文件名 => 确认是文件上传
        if (upload.filename && upload.filename.length() > 0) {
            LOG_SYSTEM_INFO("OTA Upload Start: %s", upload.filename.c_str());
            lcdText("Uploading...", 1);
            lcdText(upload.filename, 2);
            updateColor(CRGB::Orange);
            otaExpectedSize = 0;  // 重置预期大小
            
            // http上传无法提前获取文件大小，让update库使用未知大小模式
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                LOG_SYSTEM_ERROR("OTA begin failed");
                lcdText("OTA Begin Fail", 1);
                lcdText("", 2);
                settingServer.send(500, "application/json", 
                    "{\"success\":false,\"error\":\"" + String(Update.errorString()) + "\"}");
                return;
            }
        }
    }

    // 分片上传阶段
    else if (upload.status == UPLOAD_FILE_WRITE) {
        // 如果写入字节不匹配
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            LOG_SYSTEM_ERROR("OTA write failed");
            Update.abort();  // abort 回滚
            settingServer.send(500, "application/json", 
                "{\"success\":false,\"error\":\"写入失败\"}");
            lcdText("OTA Write Fail", 1);
            lcdText("", 2);
            return;
        }
        
        // 显示已写入的字节数
        static size_t lastReported = 0;
        size_t written = Update.progress();
        // 每100KB显示一次进度
        if (written - lastReported >= 102400 || (written >= 10240 && lastReported == 0)) {
            LOG_SYSTEM_INFO("OTA uploading:%u B (%u KB) written", written, written / 1024);
            lastReported = written;
        }
    }

    // 上传结束阶段
    else if (upload.status == UPLOAD_FILE_END) {
        LOG_SYSTEM_INFO("OTA Upload End: %u bytes (%.2f KB)", upload.totalSize, upload.totalSize / 1024.0);
        otaExpectedSize = upload.totalSize;  // 保存最终大小
        if (Update.end(true)) {
            LOG_SYSTEM_INFO("OTA Success! Firmware size: %u", upload.totalSize);
            lcdText("OTA Success!", 1);
            lcdText("Rebooting...", 2);
            updateColor(CRGB::Green);
            otaUploadSuccess = true;  // 标记上传成功
            
            // 创建后台重启任务，等待结束响应发送完成
            xTaskCreate([](void*){
                delay(2000); 
                ESP.restart();
            }, "Restart_Task", 2048, NULL, 1, NULL);
        } 
        else {  // 如果结束时出错
            LOG_SYSTEM_ERROR("OTA End failed: %s", Update.errorString());
            otaUploadSuccess = false;
            return;
        }
    }
}

// 主页处理函数
void webSettingHandleRoot() {
    // 计算总长度
    unsigned int len1 = strlen_P(webComponent);
    unsigned int len2 = strlen_P(index_html);
    unsigned int totalLen = len1 + len2;

    // 设置 Content-Length 并发送头（空 body）
    settingServer.setContentLength(totalLen);
    settingServer.send(200, "text/html; charset=utf-8", "");

    // 直接发送 PROGMEM 内容块
    settingServer.sendContent_P(webComponent, len1);
    settingServer.sendContent_P(index_html, len2);
}

// 处理上传的 JWT 配置信息 POST + application/json
void webSettingHandleSet() {
    if (settingServer.method() != HTTP_POST) {
        settingServer.send(405, "application/json; charset=utf-8", "{\"error\":\"不允许的请求方法\"}");
        LOG_WEB_WARN("Received non-POST request for JWT config");
        return;
    }

    if (!settingServer.hasArg("plain")) {
        settingServer.send(400, "application/json; charset=utf-8", "{\"error\":\"请求体为空\"}");
        LOG_WEB_WARN("Received empty request body for JWT config");
        return;
    }

    String body = settingServer.arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        settingServer.send(400, "application/json; charset=utf-8", "{\"error\":\"JSON解析失败\"}");
        LOG_WEB_WARN("Failed to parse JSON for JWT config: %s", err.c_str());
        return;
    }
    if (doc.overflowed()) {
        settingServer.send(413, "application/json; charset=utf-8", "{\"error\":\"JSON大小超过限制\"}");
        LOG_WEB_WARN("JSON size exceeded limit for JWT config");
        return;
    }

    String apiHost = doc["apiHost"] | (doc["host"] | "");
    String kid = doc["kID"] | (doc["kid"] | "");
    String project = doc["projectID"] | (doc["project"] | "");
    String privateKey = doc["privateKey"] | (doc["key"] | "");

    apiHost.trim();
    kid.trim();
    project.trim();
    privateKey.trim();
    
    // Validate base64 PKCS#8 Ed25519 private key using jwt_auth helper
    if (!validate_base64_ed25519_key(privateKey.c_str())) {
        settingServer.send(400, "application/json; charset=utf-8", "{\"error\":\"无效的 privateKey\"}");
        LOG_WEB_WARN("Invalid privateKey base64 PKCS#8");
        return;
    }

    if(!qweatherAuthConfigManager.setAuth(apiHost, kid, project, privateKey)){
        settingServer.send(500, "application/json; charset=utf-8", "{\"error\":\""+ qweatherAuthConfigManager.getLastQWeatherErrorString() +"\"}");
        LOG_WEB_ERROR("Failed to save JWT config");
        return;
    }
    loadJwtConfig();

    // 打印到串口DEBUG等级日志
    LOG_WEATHER_DEBUG("==== Configuration received ====");
    LOG_WEATHER_DEBUG("API Host: " + apiHost);
    LOG_WEATHER_DEBUG("kid: " + kid);
    LOG_WEATHER_DEBUG("projectID: " + project);
    LOG_WEATHER_DEBUG("private key length: " + String(privateKey.length()));

    // 保存成功
    settingServer.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

void webSettingHandleGetApiInfo() {
    String json = "{";
    json += "\"apiHost\":\"" + qweatherAuthConfigManager.getApiHost() + "\",";
    json += "\"kID\":\"" + qweatherAuthConfigManager.getKId() + "\",";
    json += "\"projectID\":\"" + qweatherAuthConfigManager.getProjectID() + "\",";
    json += "\"privateKey\":" + String(isKeyDone ? "true" : "false");
    json += "}";
    settingServer.send(200, "application/json; charset=utf-8", json);
}

void webSettingHandleFavicon() {
    LOG_WEB_INFO("Favicon requested, serving /favicon.ico, size=%u", favicon_ico_len);
    // Set cache headers so browsers don't repeatedly request the icon
    settingServer.sendHeader("Cache-Control", "public, max-age=86400");
    settingServer.setContentLength(favicon_ico_len);
    settingServer.send(200, "image/x-icon", "");
    settingServer.sendContent_P(reinterpret_cast<const char*>(favicon_ico), favicon_ico_len);
}

void webSettingHandleCitySearch() {
    // 计算总长度
    unsigned int len1 = strlen_P(webComponent);
    unsigned int len2 = strlen_P(city_search_html);
    unsigned int totalLen = len1 + len2;

    // 设置 Content-Length 并发送头（空 body）
    settingServer.setContentLength(totalLen);
    settingServer.send(200, "text/html; charset=utf-8", "");

    // 直接发送 PROGMEM 内容块
    settingServer.sendContent_P(webComponent, len1);
    settingServer.sendContent_P(city_search_html, len2);
}

String fetchCitySearchResult(String& location) {
    location.trim();

    if(!qweatherAuthConfigManager.checkApiConfigValid()){
        LOG_WEATHER_ERROR("API configuration missing for city search");
        settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"API配置缺失，无法进行城市搜索\"}");
        return " ";
    }

    String varApiHost = qweatherAuthConfigManager.getApiHost();
    String varKid = qweatherAuthConfigManager.getKId();
    String varProjectID = qweatherAuthConfigManager.getProjectID();
    String varBase64Key = qweatherAuthConfigManager.getBase64Key();
    String jwtToken;
    
    // 确保先生成seed32
    generateSeed32();

    // 生成JWT
    jwtToken = generate_jwt(varKid, varProjectID, seed32);
    LOG_WEATHER_DEBUG("City search JWT token generated, length: " + String(jwtToken.length()));
        
    if (varApiHost.length() == 0 || jwtToken.length() == 0) {
        LOG_WEATHER_ERROR("API configuration missing for city search (host or token empty)");
        settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"API配置缺失，无法进行城市搜索\"}");
        return " ";
    }


    // 请求城市搜索API
    String url = "https://" + varApiHost + "/geo/v2/city/lookup?location=" + location + "&number=10";
    LOG_WEATHER_INFO("City search request URL: " + url);
    
    HTTPClient http;
    http.begin(url);                            // 让HTTPClient自动处理HTTPS和DNS
    http.addHeader("Accept-Encoding", "gzip");
    http.addHeader("Authorization", "Bearer " + jwtToken);
    LOG_WEATHER_DEBUG("City search authorization header set");
    
    int httpCode = http.GET();
    LOG_WEATHER_INFO("City search HTTP response code: " + String(httpCode));
    if (httpCode != 200) {
        http.end();
        settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"请求失败，HTTP代码：" + String(httpCode) + "\"}");
        return " ";
    }
    int payloadSize = http.getSize();
    
    // 使用RAII内存管理
    MemoryManager::SafeBuffer compressedBuffer(payloadSize + 8, "CitySearch_Response");
    if (!compressedBuffer.isValid()) {
        http.end();
        settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"内存分配失败\"}");
        return " ";
    }
    
    WiFiClient *stream = http.getStreamPtr();
    long startMillis = millis();
    int iCount = 0;
    while (iCount < payloadSize && (millis() - startMillis) < 4000) {
        if (stream->available()) {
            compressedBuffer.get()[iCount++] = stream->read();
        } else {
            vTaskDelay(5);
        }
    }
    http.end();
    
    String jsonData;
    zlib_turbo zt;
    if (iCount >= 2 && compressedBuffer.get()[0] == 0x1f && compressedBuffer.get()[1] == 0x8b) {
        int uncompSize = zt.gzip_info(compressedBuffer.get(), iCount);
        if (uncompSize <= 0) {
            settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"Gzip解压失败\"}");
            return " ";
        }
        
        MemoryManager::SafeBuffer uncompressedBuffer(uncompSize + 8, "CitySearch_Decompressed");
        if (!uncompressedBuffer.isValid()) {
            settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"内存分配失败\"}");
            return " ";
        }
        
        int rc = zt.gunzip(compressedBuffer.get(), iCount, uncompressedBuffer.get());
        if (rc != ZT_SUCCESS) {
            settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"Gzip解压失败，错误代码：" + String(rc) + "\"}");
            return " ";
        }
        jsonData = String((char *)uncompressedBuffer.get(), uncompSize);
        // uncompressedBuffer 会在作用域结束时自动释放
    } else {
        jsonData = String((char *)compressedBuffer.get(), iCount);
    }
    // compressedBuffer 会在作用域结束时自动释放

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        LOG_WEATHER_ERROR("City search JSON parse failed: " + String(error.c_str()));
        settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"JSON解析失败\"}");
        return " ";
    }
    
    LOG_WEATHER_DEBUG("City search JSON parsed successfully");
    LOG_WEATHER_DEBUG("JSON response: " + jsonData);
    
    // 检查API响应状态
    if (doc["code"].as<String>() != "200") {
        String code = doc["code"].as<String>();
        LOG_WEATHER_WARN("City search API error code: " + code);
        settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"API错误，错误代码：" + code + "\"}");
        return " ";
    }
    
    // 检查location字段是否存在且为数组
    if (!doc["location"].is<JsonArray>()) {
        LOG_WEATHER_WARN("City search response: location field missing or not array");
        settingServer.send(500, "text/html; charset=utf-8", "{\"error\":\"API响应格式错误，缺少 location 字段\"}");
        return " ";
    }
    JsonArray locArr = doc["location"].as<JsonArray>();
    LOG_WEATHER_INFO("City search found " + String(locArr.size()) + " cities");
    
    if (locArr.size() == 0) {
        settingServer.send(200, "application/json; charset=utf-8", "{\"success\":true,\"results\":[]}");
        return " ";
    }

    for (JsonObject obj : locArr) {
        String name = obj["name"].as<String>();
        String adm1 = obj["adm1"].as<String>();
        String country = obj["country"].as<String>();
        String locid = obj["id"].as<String>();
    }
    // 发送响应JSON
    String json = "{";
    json += "\"success\":true,";
    json += "\"results\":[";

    for (size_t i = 0; i < locArr.size(); ++i) {
        JsonObject obj = locArr[i];
        String name = obj["name"].as<String>();
        String adm1 = obj["adm1"].as<String>();
        String country = obj["country"].as<String>();
        String locid = obj["id"].as<String>();
        String fxlink = obj["fxLink"].as<String>();

        json += "{";
        json += "\"name\":\"" + name + "\",";
        json += "\"adm1\":\"" + adm1 + "\",";
        json += "\"country\":\"" + country + "\",";
        json += "\"locid\":\"" + locid + "\",";
        json += "\"fxlink\":\"" + fxlink + "\"";
        json += "}";

        if (i != locArr.size() - 1) json += ",";
    }

    json += "]";
    json += "}";

    LOG_WEATHER_DEBUG("City search result JSON: " + json);
    return json;
}

enum FetchCitySearchState {
    IDLE,
    FETCHING,
    COMPLETED
};
FetchCitySearchState citySearchState = IDLE;
String citySearchResultJson = "";

void webSettingHandleCitySearchResult() {
    if(citySearchState == IDLE){
        if (!settingServer.hasArg("location")) {
            settingServer.send(400, "application/json; charset=utf-8", "{\"error\":\"缺少 location 参数\"}");
            return;
        }
        citySearchState = FETCHING;
        settingServer.send(202, "application/json; charset=utf-8", "{\"status\":\"processing\"}");
        String location = settingServer.arg("location");
        xTaskCreate([](void* param) {
            String loc = *((String*)param);
            LOG_WEATHER_DEBUG("City search task started for location: " + loc);
            citySearchResultJson = fetchCitySearchResult(loc);
            citySearchState = COMPLETED;
            delete (String*)param;
            vTaskDelete(NULL);
        }, "CitySearchTask", 16384, new String(location), 2, NULL);
    } 
    else if(citySearchState == COMPLETED){
        settingServer.send(200, "application/json; charset=utf-8", citySearchResultJson);
        citySearchState = IDLE;
        citySearchResultJson = "";
    }
    else if(citySearchState == FETCHING){
        settingServer.send(202, "application/json; charset=utf-8", "{\"status\":\"processing\"}");
    }
    else {
        settingServer.send(429, "application/json; charset=utf-8", "{\"error\":\"正在处理另一个请求，请稍后再试\"}");
    }

    String location = settingServer.arg("location");
}

void webSettingHandleSetLocation() {
    if (settingServer.method() != HTTP_POST) {
        settingServer.send(405, "application/json; charset=utf-8", "{\"error\":\"请使用 POST 方法\"}");
        return;
    }
    if (!settingServer.hasArg("locid")) {
        settingServer.send(400, "text/html; charset=utf-8", "参数错误");
        return;
    }
    String locid = settingServer.arg("locid");
    locid.trim();

    String cityname = "";
    if (settingServer.hasArg("fxlink")) {
        String fxlink = settingServer.arg("fxlink");
        int start = fxlink.indexOf("/weather/");
        int end = fxlink.lastIndexOf("-");
        if (start != -1 && end != -1 && end > start+9) {
            cityname = fxlink.substring(start+9, end);
        }
    }
    else{
        LOG_WEATHER_INFO("No fxlink provided, using locid as city name");
        cityname = locid;
    }
    
    // 保存到配置文件（追加或覆盖）
    qweatherAuthConfigManager.setLocation(locid, cityname);
    
    locid = qweatherAuthConfigManager.getLocation().c_str();
    cityname = qweatherAuthConfigManager.getCityName().c_str();

    loadJwtConfig();

    weatherSynced = false;
    isReadyToDisplay = false;

    LOG_WEATHER_INFO("Location set to ID: %s, Name: %s", locid.c_str(), cityname.c_str());
    settingServer.send(200, "application/json; charset=utf-8", "{\"ok\":true,\"locid\":\"" + locid + "\"}");
}

void webSettingSetupWebServer() {
    if(wifiConnectionState != WIFI_CONNECTED) {
        LOG_SYSTEM_WARN("Web setting server setup called but WiFi not connected");
        lcdText("WiFi Not Conn", 1);
        lcdText(" ", 2);
        delay(1000);
        return;
    }

    isConfigDone=false;

    settingServer.onNotFound([](){ settingServer.send(404, "text/plain", "404 Not Found"); });

    // 主页相关路由w
    settingServer.on("/", webSettingHandleRoot);                        // 主页
    settingServer.on("/favicon.ico", webSettingHandleFavicon);          // 网站图标
    settingServer.on("/get_api_info", webSettingHandleGetApiInfo);      // 获取当前API信息
    settingServer.on("/set", HTTP_POST, webSettingHandleSet);           // 接收POST JSON格式的API配置信息
    settingServer.on("/exit", [](){                                     // 退出设置页面
        isConfigDone = true;
        settingServer.send(200, "text/plain", "Exiting configuration...");
    });

    // OTA相关路由
    settingServer.on("/ota", webSettingHandleOTA);                         // OTA页面
    settingServer.on("/ota/url", webSettingHandleOTAURL);                  // OTA URL处理
    settingServer.on("/ota/progress", webSettingHandleOTAProgress);        // OTA进度查询
    settingServer.on("/ota/upload", HTTP_POST, 
        []() { 
            // 处理完成后的回调
            if (otaUploadSuccess) {
                settingServer.send(200, "application/json", "{\"success\":true}");
            } else {
                String errorMsg = Update.hasError() ? String(Update.errorString()) : "上传失败";
                settingServer.send(500, "application/json", 
                    "{\"success\":false,\"error\":\"" + errorMsg + "\"}");
            }
            otaUploadSuccess = false;  // 重置标志
        },
        webSettingHandleOTAUpload  // 上传处理函数
    );

    // 城市搜索相关路由
    settingServer.on("/citysearch", webSettingHandleCitySearch);    // 城市搜索界面
    settingServer.on("/citysearch_result", webSettingHandleCitySearchResult);
    settingServer.on("/set_location", webSettingHandleSetLocation);
    
    settingServer.begin();
    LOG_WEATHER_INFO("Web configuration server started, access via IP address");
    
    // 获取当前IP地址并显示
    String ipAddress = WiFi.localIP().toString();
    lcdText("Config Mode", 1);
    lcdText(ipAddress.c_str(), 2);
    while(isConfigDone == false){
        settingServer.handleClient();
        delay(1);
    }

    lcdText("Config Done", 1);
    lcdText("Exiting...", 2);
    delay(500);
}