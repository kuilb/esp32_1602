#include "web_setting.h"

WebServer setting_server(80);
volatile bool isConfigDone = false;
volatile bool isKeyDone = false;

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

    // JavaScript
    html += "<script>"
        "function openCitySearch(){"
        "  window.open('/citysearch','_blank','width=600,height=400');"
        "}"
        "function exitSettings(){"
        "  fetch('/exit').then(response => {"
        "    window.location.href='/';"  // 返回主页或关闭页面
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
        Serial.println("保存JWT信息失败, 无法打开文件");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Check FS/Retry", 2);
        return;
    }

    if (!file.println(apiHost)) {
        Serial.println("保存JWT信息失败, 写入API Host失败");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Write Err", 2);
        file.close();
        return;
    }

    if (!file.println(kid)) {
        Serial.println("保存JWT信息失败, 写入kid失败");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Write Err", 2);
        file.close();
        return;
    }

    if (!file.println(project_id)) {
        Serial.println("保存JWT信息失败, 写入project_id失败");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Write Err", 2);
        file.close();
        return;
    }

    if (!file.println(privateKey)) {
        Serial.println("保存JWT信息失败, 写入private key失败");
        lcd_text("Save JWT Fail", 1);
        lcd_text("Write Err", 2);
        file.close();
        return;
    }

    file.flush();
    file.close();

    Serial.println("JWT 信息已保存");
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
        Serial.println("==== 接收到配置 ====");
        Serial.println("API Host: " + apiHost);
        Serial.println("kid: " + kid);
        Serial.println("project_id: " + project);
        Serial.println("private key: " + privateKey);

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
                Serial.println("[CITYSEARCH] hostStr: " + hostStr);
                String kid = file.readStringUntil('\n'); kid.trim();
                Serial.println("[CITYSEARCH] kid: " + kid);
                String project = file.readStringUntil('\n'); project.trim();
                Serial.println("[CITYSEARCH] project: " + project);
                String key = file.readStringUntil('\n'); key.trim();
                Serial.println("[CITYSEARCH] key: " + key);
                file.close();
                // 生成JWT
                jwtToken = generate_jwt(kid, project, seed32); // seed32需提前生成
                Serial.println("[CITYSEARCH] jwtToken: " + jwtToken);
            } else {
                Serial.println("[CITYSEARCH] jwt_config.txt文件打开失败");
            }
        } else {
            Serial.println("[CITYSEARCH] jwt_config.txt文件不存在");
        }
        if (hostStr.length() == 0 || jwtToken.length() == 0) {
            Serial.println("[CITYSEARCH] hostStr或jwtToken为空");
            setting_server.send(200, "text/html; charset=utf-8", "API配置缺失，请先配置主页面参数");
            return;
        }

        // 请求城市搜索API
        String url = "https://" + hostStr + "/geo/v2/city/lookup?location=" + location + "&number=10";
        Serial.println("[CITYSEARCH] 请求URL: " + url);
        HTTPClient http;
        http.begin(url);
        http.addHeader("Accept-Encoding", "gzip");
        http.addHeader("Authorization", "Bearer " + jwtToken);
        Serial.println("[CITYSEARCH] Authorization: Bearer " + jwtToken);
        int httpCode = http.GET();
        Serial.println("[CITYSEARCH] HTTP响应代码: " + String(httpCode));
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>搜索结果</title></head><body>";
        html += "<h2>搜索结果</h2>";
        if (httpCode != 200) {
            html += "<p>请求失败，HTTP代码：" + String(httpCode) + "</p></body></html>";
            http.end();
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
        }
        int payloadSize = http.getSize();
        uint8_t *pCompressed = (uint8_t *)malloc(payloadSize + 8);
        if (!pCompressed) {
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
                html += "<p>Gzip解压失败</p></body></html>";
                free(pCompressed);
                setting_server.send(200, "text/html; charset=utf-8", html);
                return;
            }
            uint8_t *pUncompressed = (uint8_t *)malloc(uncompSize + 8);
            if (!pUncompressed) {
                html += "<p>内存分配失败</p></body></html>";
                free(pCompressed);
                setting_server.send(200, "text/html; charset=utf-8", html);
                return;
            }
            int rc = zt.gunzip(pCompressed, iCount, pUncompressed);
            if (rc != ZT_SUCCESS) {
                html += "<p>Gzip解压失败</p></body></html>";
                free(pUncompressed);
                free(pCompressed);
                setting_server.send(200, "text/html; charset=utf-8", html);
                return;
            }
            jsonData = String((char *)pUncompressed, uncompSize);
            free(pUncompressed);
        } else {
            jsonData = String((char *)pCompressed, iCount);
        }
        free(pCompressed);

        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, jsonData);
        if (error) {
            html += "<p>JSON解析失败</p></body></html>";
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
        }
        if (!doc.containsKey("location")) {
            html += "<p>未找到候选城市</p></body></html>";
            setting_server.send(200, "text/html; charset=utf-8", html);
            return;
        }
        JsonArray locArr = doc["location"].as<JsonArray>();
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
        extern char* location;
        extern char* city_name;
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
                if (location) free(location);
                if (city_name) free(city_name);
                location = strdup(newLoc.c_str());
                city_name = strdup(newCity.c_str());
                file.close();
            }
        }
        weatherSynced = false;
        setting_server.send(200, "text/html; charset=utf-8", "<p>LocationID已设置为：" + locid + "<br>地名：" + cityname + "</p><a href='/'>返回配置页面</a>");
    });
    
    setting_server.begin();
    Serial.println("Web 服务器已启动，访问 IP 地址以配置参数");
    lcd_text("Config Mode", 1);
    lcd_text(" ", 2);
    while(isConfigDone == false){
        setting_server.handleClient();
        delay(1);
    }
    lcd_text("Config Done", 1);
    lcd_text(" ", 2);
    delay(500);
}