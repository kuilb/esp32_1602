#include "web_setting.h"

WebServer setting_server(80);
volatile bool isConfigDone = false;
volatile bool isKeyDone = false;

// OTA页面处理
void web_setting_handleOTA() {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>OTA升级</title>";
    html += "<style>"
            "body{font-family:Arial; text-align:center; padding:20px; background:#f0f2f5;}"
            ".container{background:#fff; padding:30px; border-radius:8px; display:inline-block; box-shadow:0 0 10px rgba(0,0,0,0.1);}"
            "input[type=text], input[type=file]{width:400px; padding:10px; margin:10px 0; border:1px solid #ccc; border-radius:4px;}"
            "button{padding:12px 30px; margin:10px 5px; border:none; border-radius:4px; cursor:pointer; font-size:16px;}"
            ".btn-primary{background-color:#4CAF50; color:white;}"
            ".btn-primary:hover{background-color:#45a049;}"
            ".btn-secondary{background-color:#2196F3; color:white;}"
            ".btn-secondary:hover{background-color:#1976D2;}"
            ".progress{width:400px; height:30px; background:#e0e0e0; border-radius:15px; margin:20px auto; overflow:hidden;}"
            ".progress-bar{height:100%; background:#4CAF50; width:0%; transition:width 0.3s; text-align:center; line-height:30px; color:white;}"
            ".info{margin:15px 0; color:#666;}"
            "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>🔄 OTA固件升级</h1>";
    
    // 显示当前版本信息
    html += "<div class='info'>";
    html += "<p><strong>当前版本:</strong> v1.0.0</p>";
    html += "<p><strong>分区方案:</strong> OTA双分区</p>";
    html += "<p><strong>当前分区:</strong> " + String(esp_ota_get_running_partition()->label) + "</p>";
    html += "</div>";
    
    // URL升级
    html += "<h3>方式1: 从URL升级</h3>";
    html += "<input type='text' id='otaUrl' placeholder='http://yourserver.com/firmware.bin'>";
    html += "<br><button class='btn-primary' onclick='startOTAFromURL()'>开始URL升级</button>";
    
    // 文件上传升级
    html += "<h3>方式2: 上传固件文件</h3>";
    html += "<input type='file' id='otaFile' accept='.bin'>";
    html += "<br><button class='btn-secondary' onclick='startOTAFromFile()'>开始文件升级</button>";
    
    // 进度条
    html += "<div class='progress' id='progressBar' style='display:none;'>";
    html += "<div class='progress-bar' id='progress'>0%</div></div>";
    html += "<p id='status'></p>";
    
    html += "</div>";
    
    // JavaScript
    html += "<script>"
        "function startOTAFromURL(){"
        "  var url = document.getElementById('otaUrl').value;"
        "  if(!url){alert('请输入URL');return;}"
        "  document.getElementById('progressBar').style.display='block';"
        "  document.getElementById('status').innerText='正在下载固件...';"
        "  fetch('/ota/url?url='+encodeURIComponent(url))"
        "    .then(r=>r.json())"
        "    .then(data=>{"
        "      if(data.success){"
        "        document.getElementById('status').innerText='升级成功!设备将重启...';"
        "        document.getElementById('progress').style.width='100%';"
        "        document.getElementById('progress').innerText='100%';"
        "      }else{"
        "        document.getElementById('status').innerText='升级失败: '+data.error;"
        "      }"
        "    });"
        "  pollProgress();"
        "}"
        ""
        "function startOTAFromFile(){"
        "  var file = document.getElementById('otaFile').files[0];"
        "  if(!file){alert('请选择文件');return;}"
        "  document.getElementById('progressBar').style.display='block';"
        "  document.getElementById('status').innerText='正在上传固件...';"
        "  var formData = new FormData();"
        "  formData.append('firmware', file);"
        "  var xhr = new XMLHttpRequest();"
        "  xhr.upload.onprogress = function(e){"
        "    if(e.lengthComputable){"
        "      var pct = Math.round((e.loaded/e.total)*100);"
        "      document.getElementById('progress').style.width=pct+'%';"
        "      document.getElementById('progress').innerText=pct+'%';"
        "    }"
        "  };"
        "  xhr.onload = function(){"
        "    if(xhr.status==200){"
        "      var resp = JSON.parse(xhr.responseText);"
        "      if(resp.success){"
        "        document.getElementById('status').innerText='升级成功!设备将重启...';"
        "      }else{"
        "        document.getElementById('status').innerText='升级失败: '+resp.error;"
        "      }"
        "    }"
        "  };"
        "  xhr.open('POST', '/ota/upload');"
        "  xhr.send(formData);"
        "}"
        ""
        "function pollProgress(){"
        "  var interval = setInterval(function(){"
        "    fetch('/ota/progress')"
        "      .then(r=>r.json())"
        "      .then(data=>{"
        "        var pct = data.progress;"
        "        document.getElementById('progress').style.width=pct+'%';"
        "        document.getElementById('progress').innerText=pct+'%';"
        "        if(pct>=100)clearInterval(interval);"
        "      });"
        "  }, 500);"
        "}"
        "</script>";
    
    html += "</body></html>";
    setting_server.send(200, "text/html; charset=utf-8", html);
}

// OTA URL处理
void web_setting_handleOTAURL() {
    if (!setting_server.hasArg("url")) {
        setting_server.send(400, "application/json", "{\"success\":false,\"error\":\"No URL\"}");
        return;
    }
    
    String url = setting_server.arg("url");
    LOG_SYSTEM_INFO("OTA from URL: %s", url.c_str());
    
    // 异步执行OTA (避免阻塞Web响应)
    setting_server.send(200, "application/json", "{\"success\":true}");
    
    delay(500); // 让响应发送出去
    OTAResult result = OTAManager::updateFromURL(url);
    
    if (result != OTA_SUCCESS) {
        LOG_SYSTEM_ERROR("OTA failed: %s", OTAManager::getErrorString().c_str());
    }
}

// OTA进度查询
void web_setting_handleOTAProgress() {
    int progress = OTAManager::getProgress();
    String json = "{\"progress\":" + String(progress) + "}";
    setting_server.send(200, "application/json", json);
}

// OTA文件上传处理
void web_setting_handleOTAUpload() {
    HTTPUpload& upload = setting_server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        LOG_SYSTEM_INFO("OTA Upload Start: %s", upload.filename.c_str());
        lcd_text("Uploading...", 1);
        updateColor(CRGB::Orange);
        
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            LOG_SYSTEM_ERROR("OTA begin failed");
        }
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            LOG_SYSTEM_ERROR("OTA write failed");
        }
    } 
    else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            LOG_SYSTEM_INFO("OTA Success! Size: %u", upload.totalSize);
            lcd_text("OTA Success!", 1);
            updateColor(CRGB::Green);
            setting_server.send(200, "application/json", "{\"success\":true}");
            delay(1000);
            ESP.restart();
        } else {
            setting_server.send(500, "application/json", 
                "{\"success\":false,\"error\":\"" + String(Update.errorString()) + "\"}");
        }
    }
}

// 主页处理函数
void web_setting_handleRoot() {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 配置页面</title>";

    html += "<style>"
            "body{font-family:Arial; text-align:center; padding:20px; background:#f0f2f5;}"
            "form{background:#fff; padding:20px; border-radius:8px; display:inline-block; box-shadow:0 0 10px rgba(0,0,0,0.1);}"
            "input[type=text]{width:300px; padding:8px; margin:5px 0; border:1px solid #ccc; border-radius:4px;}"
            "input[type=submit], button{padding:10px 20px; margin-top:10px; border:none; border-radius:4px; cursor:pointer;}"
            "input[type=submit]{background-color:#4CAF50; color:white;}"
            "input[type=submit]:hover{background-color:#45a049;}"
            "button.search-btn{background-color:#2196F3; color:white;}"
            "button.search-btn:hover{background-color:#1976D2;}"
            "button.exit-btn{background-color:#f44336; color:white;}"
            "button.exit-btn:hover{background-color:#d32f2f;}"
            "label{display:block; margin-top:10px; font-weight:bold;}"
            ".button-row{display:flex; justify-content:center; gap:10px; margin-top:10px;}"
            "</style>";

    html += "</head><body>";
    html += "<h1>ESP32 配置页面</h1>";
    html += "<form action='/set' method='GET'>";

    // 读取本地文件内容填入输入框
    String host_val = "", kid_val = "", project_val = "", key_val = "";
    if (SPIFFS.exists("/jwt_config.txt")) {
        File file = SPIFFS.open("/jwt_config.txt", "r");
        if (file) {
            host_val = file.readStringUntil('\n'); host_val.trim();
            kid_val = file.readStringUntil('\n'); kid_val.trim();
            project_val = file.readStringUntil('\n'); project_val.trim();
            key_val = file.readStringUntil('\n'); key_val.trim();
            file.close();
        }
    }

    html += "<label>和风天气 API Host:</label><input type='text' name='host' value='" + host_val + "'><br>";
    html += "<label>kid:</label><input type='text' name='kid' value='" + kid_val + "'><br>";
    html += "<label>project_id:</label><input type='text' name='project' value='" + project_val + "'><br>";
    html += "<label>private key:</label><input type='text' name='key' value='" + key_val + "'><br>";

    // 城市搜索按钮、提交按钮和退出按钮同一行
    html += "<div class='button-row'>";
    html += "<button type='button' class='search-btn' onclick='openCitySearch()'>搜索城市</button>";
    html += "<input type='submit' value='提交'>";
    html += "<button type='button' class='exit-btn' onclick='exitSettings()'>退出设置</button>";
    html += "</div>";

    html += "</form>";

    // OTA
    html += "<div class='button-row'>";
    html += "<button type='button' onclick='location.href=\"/ota\"'>🔄 OTA升级</button>";
    html += "</div>";

    // JavaScript
    html += "<script>"
        "function openCitySearch(){"
        "  window.open('/citysearch','_blank','width=800,height=600');"
        "}"
        "function exitSettings(){"
        "  fetch('/exit').then(response => {"
        "    window.close();"  // 关闭当前窗口
        "  }).catch(error => {"
        "    console.log('退出请求失败:', error);"
        "    window.close();"  // 即使请求失败也关闭窗口
        "  });"
        "}"
        "</script>";

    html += "</body></html>";

    setting_server.send(200, "text/html; charset=utf-8", html);
}


// 保存 JWT 配置信息
void saveJWTConfig(const String& apiHost,
                   const String& kid,
                   const String& project_id,
                   const String& privateKey) {

    File file = SPIFFS.open("/jwt_config.txt", "w");
    if (!file) {
        LOG_WEATHER_ERROR("Failed to save JWT config - cannot open file");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Check FS/Retry", 2);
        return;
    }

    if (!file.println(apiHost)) {
        LOG_WEATHER_ERROR("Failed to save JWT config - write API Host failed");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Write Err", 2);
        file.close();
        return;
    }

    if (!file.println(kid)) {
        LOG_WEATHER_ERROR("Failed to save JWT config - write kid failed");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Write Err", 2);
        file.close();
        return;
    }

    if (!file.println(project_id)) {
        LOG_WEATHER_ERROR("Failed to save JWT config - write project_id failed");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Write Err", 2);
        file.close();
        return;
    }

    if (!file.println(privateKey)) {
        LOG_WEATHER_ERROR("Failed to save JWT config - write private key failed");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Write Err", 2);
        file.close();
        return;
    }

    file.flush();
    file.close();

    LOG_WEATHER_INFO("JWT configuration saved successfully");
    lcd_text("JWT Saved", 1);
    lcd_text(" ", 2);
}

// 处理设置
void web_setting_handleSet() {
    if (setting_server.hasArg("host") &&
        setting_server.hasArg("kid") &&
        setting_server.hasArg("project") &&
        setting_server.hasArg("key")) {

        String apiHost = setting_server.arg("host");
        String kid = setting_server.arg("kid");
        String project = setting_server.arg("project");
        String privateKey = setting_server.arg("key");

        // 打印到串口，调试用
        LOG_WEATHER_DEBUG("==== Configuration received ====");
        LOG_WEATHER_DEBUG("API Host: " + apiHost);
        LOG_WEATHER_DEBUG("kid: " + kid);
        LOG_WEATHER_DEBUG("project_id: " + project);
        LOG_WEATHER_DEBUG("private key length: " + String(privateKey.length()));

        saveJWTConfig(apiHost, kid, project, privateKey);

        setting_server.send(200, "text/html; charset=utf-8", "JWT 配置已保存");
    } else {
        setting_server.send(200, "text/html; charset=utf-8", "提交数据不完整");
    }

    // 配置完成标志
    isConfigDone = true;
}

void web_setting_setupWebServer() {
    isConfigDone=false;

    setting_server.on("/", web_setting_handleRoot);       // 主页
    setting_server.on("/set", web_setting_handleSet);      // 设置参数

    // OTA相关路由
    setting_server.on("/ota", web_setting_handleOTA);
    setting_server.on("/ota/url", web_setting_handleOTAURL);
    setting_server.on("/ota/progress", web_setting_handleOTAProgress);
    setting_server.on("/ota/upload", HTTP_POST, 
        []() { /* 上传完成后的响应 */ },
        web_setting_handleOTAUpload  // 上传处理函数
    );

    setting_server.on("/exit", [](){
        isConfigDone = true;
        setting_server.send(200, "text/plain", "Exiting configuration...");
    });

    // 城市搜索页面
    setting_server.on("/citysearch", [](){
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>城市搜索</title>";
        html += "<style>body{font-family:Arial;text-align:center;padding:20px;background:#f0f2f5;}form{background:#fff;padding:20px;border-radius:8px;display:inline-block;box-shadow:0 0 10px rgba(0,0,0,0.1);}input[type=text]{width:300px;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;}input[type=submit]{background-color:#2196F3;color:white;padding:10px 20px;margin-top:10px;border:none;border-radius:4px;cursor:pointer;}input[type=submit]:hover{background-color:#1976D2;}</style>";
        html += "</head><body>";
        html += "<h2>城市搜索</h2>";
        html += "<form action='/citysearch_result' method='GET'>";
        html += "<input type='text' name='location' placeholder='请输入城市名或拼音' required><br>";
        html += "<input type='submit' value='搜索'>";
        html += "</form>";
        html += "</body></html>";
        setting_server.send(200, "text/html; charset=utf-8", html);
    });

    // 城市搜索结果页面
    setting_server.on("/citysearch_result", [](){
        if (!setting_server.hasArg("location")) {
            setting_server.send(200, "text/html; charset=utf-8", "缺少搜索参数");
            return;
        }
        String location = setting_server.arg("location");
        location.trim();

        // 读取API配置
        String hostStr = "";
        String jwtToken = "";
        if (SPIFFS.exists("/jwt_config.txt")) {
            File file = SPIFFS.open("/jwt_config.txt", "r");
            if (file) {
                hostStr = file.readStringUntil('\n'); hostStr.trim();
                LOG_WEATHER_DEBUG("City search API host: " + hostStr);
                String kid = file.readStringUntil('\n'); kid.trim();
                LOG_WEATHER_DEBUG("City search kid: " + kid);
                String project = file.readStringUntil('\n'); project.trim();
                LOG_WEATHER_DEBUG("City search project: " + project);
                String key = file.readStringUntil('\n'); key.trim();
                LOG_WEATHER_DEBUG("City search key length: " + String(key.length()));
                file.close();
                // 确保先生成seed32
                generateSeed32();
                // 生成JWT
                jwtToken = generate_jwt(kid, project, seed32);
                LOG_WEATHER_DEBUG("City search JWT token generated, length: " + String(jwtToken.length()));
            } else {
                LOG_WEATHER_ERROR("Failed to open jwt_config.txt for city search");
            }
        } else {
            LOG_WEATHER_ERROR("jwt_config.txt file not found for city search");
        }
        if (hostStr.length() == 0 || jwtToken.length() == 0) {
            LOG_WEATHER_ERROR("API configuration missing for city search (host or token empty)");
            setting_server.send(200, "text/html; charset=utf-8", "API配置缺失，请先配置主页面参数");
            return;
        }

        // 请求城市搜索API
        String url = "https://" + hostStr + "/geo/v2/city/lookup?location=" + location + "&number=10";
        LOG_WEATHER_INFO("City search request URL: " + url);
        HTTPClient http;
        http.begin(url);
        http.addHeader("Accept-Encoding", "gzip");
        http.addHeader("Authorization", "Bearer " + jwtToken);
        LOG_WEATHER_DEBUG("City search authorization header set");
        int httpCode = http.GET();
        LOG_WEATHER_INFO("City search HTTP response code: " + String(httpCode));
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>搜索结果</title></head><body>";
        html += "<h2>搜索结果</h2>";
        if (httpCode != 200) {
            html += "<p>请求失败，HTTP代码：" + String(httpCode) + "</p></body></html>";
            http.end();
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
        }
        int payloadSize = http.getSize();
        
        // 使用RAII内存管理
        MemoryManager::SafeBuffer compressedBuffer(payloadSize + 8, "CitySearch_Response");
        if (!compressedBuffer.isValid()) {
            html += "<p>内存分配失败</p></body></html>";
            http.end();
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
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
                html += "<p>Gzip解压失败</p></body></html>";
                setting_server.send(200, "text/html; charset=utf-8", html);
                return;
            }
            
            MemoryManager::SafeBuffer uncompressedBuffer(uncompSize + 8, "CitySearch_Decompressed");
            if (!uncompressedBuffer.isValid()) {
                html += "<p>内存分配失败</p></body></html>";
                setting_server.send(200, "text/html; charset=utf-8", html);
                return;
            }
            
            int rc = zt.gunzip(compressedBuffer.get(), iCount, uncompressedBuffer.get());
            if (rc != ZT_SUCCESS) {
                html += "<p>Gzip解压失败</p></body></html>";
                setting_server.send(200, "text/html; charset=utf-8", html);
                return;
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
            html += "<p>JSON解析失败</p></body></html>";
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
        }
        
        LOG_WEATHER_DEBUG("City search JSON parsed successfully");
        LOG_WEATHER_DEBUG("JSON response: " + jsonData.substring(0, min(200, (int)jsonData.length())));
        
        // 检查API响应状态
        if (doc["code"].as<String>() != "200") {
            String code = doc["code"].as<String>();
            LOG_WEATHER_WARN("City search API error code: " + code);
            html += "<p>API错误，错误代码：" + code + "</p></body></html>";
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
        }
        
        // 检查location字段是否存在且为数组
        if (!doc["location"].is<JsonArray>()) {
            LOG_WEATHER_WARN("City search response: location field missing or not array");
            html += "<p>未找到候选城市</p></body></html>";
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
        }
        JsonArray locArr = doc["location"].as<JsonArray>();
        LOG_WEATHER_INFO("City search found " + String(locArr.size()) + " cities");
        
        if (locArr.size() == 0) {
            html += "<p>未找到匹配的城市</p></body></html>";
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
        }
        
        html += "<ul style='list-style:none;padding:0;'>";
        for (JsonObject obj : locArr) {
            String name = obj["name"].as<String>();
            String adm1 = obj["adm1"].as<String>();
            String country = obj["country"].as<String>();
            String locid = obj["id"].as<String>();
            String fxlink = obj["fxLink"].as<String>();
            html += "<li style='margin:10px 0;'><form action='/set_location' method='POST' style='display:inline;'><input type='hidden' name='locid' value='" + locid + "'><input type='hidden' name='fxlink' value='" + fxlink + "'><button type='submit' style='padding:8px 16px;border-radius:4px;background:#4CAF50;color:white;border:none;cursor:pointer;'>" + name + " (" + adm1 + ", " + country + ")</button></form></li>";
        }
        html += "</ul>";
        html += "</body></html>";
        setting_server.send(200, "text/html; charset=utf-8", html);
    });

    // 设置LocationID
    setting_server.on("/set_location", [](){
        if (!setting_server.hasArg("locid")) {
            setting_server.send(200, "text/html; charset=utf-8", "参数错误");
            return;
        }
        String locid = setting_server.arg("locid");
        locid.trim();
        String cityname = "";
        if (setting_server.hasArg("fxlink")) {
            String fxlink = setting_server.arg("fxlink");
            int start = fxlink.indexOf("/weather/");
            int end = fxlink.lastIndexOf("-");
            if (start != -1 && end != -1 && end > start+9) {
                cityname = fxlink.substring(start+9, end);
            }
        }
        // 保存到配置文件（追加或覆盖）
        if (SPIFFS.exists("/jwt_config.txt")) {
            File file = SPIFFS.open("/jwt_config.txt", "r");
            String host = "", kid = "", project = "", key = "", oldloc = "";
            if (file) {
                host = file.readStringUntil('\n'); host.trim();
                kid = file.readStringUntil('\n'); kid.trim();
                project = file.readStringUntil('\n'); project.trim();
                key = file.readStringUntil('\n'); key.trim();
                oldloc = file.readStringUntil('\n'); oldloc.trim(); // 旧locationID（可选）
                file.close();
            }
            // 重新写入，LocationID和地名
            file = SPIFFS.open("/jwt_config.txt", "w");
            if (file) {
                file.println(host);
                file.println(kid);
                file.println(project);
                file.println(key);
                file.println(locid); // 新增一行保存LocationID
                file.println(cityname); // 新增一行保存地名
                file.close();
            }
        }
        // 重新读入地名和ID，并重置weatherSynced
        extern char location[32];
        extern char city_name[64];
        extern bool weatherSynced;
        if (SPIFFS.exists("/jwt_config.txt")) {
            File file = SPIFFS.open("/jwt_config.txt", "r");
            if (file) {
                file.readStringUntil('\n'); // host
                file.readStringUntil('\n'); // kid
                file.readStringUntil('\n'); // project
                file.readStringUntil('\n'); // key
                String newLoc = file.readStringUntil('\n'); newLoc.trim();
                String newCity = file.readStringUntil('\n'); newCity.trim();
                strncpy(location, newLoc.c_str(), sizeof(location) - 1);
                location[sizeof(location) - 1] = '\0';  // 确保null结尾
                strncpy(city_name, newCity.c_str(), sizeof(city_name) - 1);
                city_name[sizeof(city_name) - 1] = '\0';  // 确保null结尾
                file.close();
            }
        }
        weatherSynced = false;
        isReadyToDisplay = false;
        
        // 返回成功页面并自动关闭弹出窗口
        String response = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>设置成功</title>";
        response += "<style>body{font-family:Arial;text-align:center;padding:40px;background:#f0f2f5;}";
        response += ".success{background:#fff;padding:30px;border-radius:8px;display:inline-block;box-shadow:0 0 10px rgba(0,0,0,0.1);}";
        response += ".success h2{color:#4CAF50;margin-bottom:20px;}";
        response += ".info{margin:10px 0;color:#666;}";
        response += "</style>";
        response += "<script>";
        response += "setTimeout(function(){window.close();}, 2000);";  // 2秒后自动关闭
        response += "</script>";
        response += "</head><body>";
        response += "<div class='success'>";
        response += "<h2>✓ 设置成功</h2>";
        response += "<div class='info'>LocationID: " + locid + "</div>";
        response += "<div class='info'>城市: " + cityname + "</div>";
        response += "<p>窗口将在2秒后自动关闭...</p>";
        response += "</div></body></html>";
        
        setting_server.send(200, "text/html; charset=utf-8", response);
    });
    
    setting_server.begin();
    LOG_WEATHER_INFO("Web configuration server started, access via IP address");
    
    // 获取当前IP地址并显示
    String ipAddress = WiFi.localIP().toString();
    lcd_text("Config Mode", 1);
    lcd_text(ipAddress.c_str(), 2);
    while(isConfigDone == false){
        setting_server.handleClient();
        delay(1);
    }
    lcd_text("Config Done", 1);
    lcd_text("Saved & Exit", 2);
    delay(500);
}