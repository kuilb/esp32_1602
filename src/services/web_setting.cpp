#include "web_setting.h"

WebServer setting_server(80);
volatile bool isConfigDone = false;
volatile bool isKeyDone = false;

// OTA页面处理
void web_setting_handleOTA() {
    String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>OTA升级</title><style>:root{--blue:#0067b6;--blue-dark:#0045a4;--red:#e57373;--red-dark:#d35f5f;--cyan:#77eedd;--cyan-dark:#55ccbb;--gray-dark:#6a7690;--gray:#6c757d;--gray-light:#f8f9fa;--gray-lighter:#343a40;--white:#fff;--border:#dee2e6;--shadow:0 4px 12px rgba(0,0,0,0.08);--border-radius:8px;}*{box-sizing:border-box;margin:0;padding:0;}body{background:linear-gradient(90deg,rgba(179,255,253,0.5) 0%,rgba(227,230,255,0.5) 50%,rgba(253,229,245,0.5) 100%);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:var(--gray-light);color:var(--gray-lighter);line-height:1.5;text-align:center;}.container{max-width:500px;margin:40px auto;padding:28px 24px;background:var(--white);border-radius:var(--border-radius);box-shadow:var(--shadow);display:flex;flex-direction:column;align-items:center;}h1{color:var(--blue);font-weight:600;margin:0;}h3{color:var(--gray-dark);margin:18px 0 10px 0;}.info{margin:15px 0 22px 0;color:var(--gray);font-size:1.05em;}input[type=text],input[type=file]{width:100%;max-width:400px;padding:10px;margin:10px 0;border:1px solid var(--border);border-radius:6px;font-size:16px;transition:border-color 0.2s,box-shadow 0.2s;}input[type=text]:focus,input[type=file]:focus{outline:none;border-color:var(--blue);box-shadow:0 0 0 3px rgba(0,123,255,0.15);}button{padding:12px 30px;margin:10px 5px;border:none;border-radius:var(--border-radius);cursor:pointer;font-size:16px;font-weight:500;transition:background-color 0.2s,transform 0.1s;color:var(--white);}.btn-primary{background-color:var(--blue);}.btn-primary:hover{background-color:var(--blue-dark);}.btn-secondary{background-color:var(--cyan);color:var(--gray-lighter);}.btn-secondary:hover{background-color:var(--cyan-dark);}.progress{width:100%;max-width:400px;height:40px;background:linear-gradient(to right,#e8f4f8,#f0f0f0);border-radius:20px;margin:20px auto 0 auto;overflow:hidden;box-shadow:inset 0 2px 4px rgba(0,0,0,0.1);position:relative;}.progress-bar{height:100%;background:linear-gradient(90deg,var(--blue) 0%,var(--cyan) 100%);width:0%;transition:width 0.4s ease;text-align:center;line-height:40px;color:var(--white);font-weight:600;font-size:15px;box-shadow:0 2px 8px rgba(0,103,182,0.3);position:relative;overflow:hidden;}.progress-bar::before{content:'';position:absolute;top:0;left:-100%;width:100%;height:100%;background:linear-gradient(90deg,transparent,rgba(255,255,255,0.3),transparent);animation:shimmer 2s infinite;}@keyframes shimmer{0%{left:-100%;}100%{left:100%;}}#status{margin-top:16px;min-height:24px;color:var(--blue);font-size:1.08em;}.btn-back{position:absolute;left:-130px;top:0;background-color:var(--red-dark);color:var(--white);border:none;border-radius:var(--border-radius);padding:10px 20px;cursor:pointer;font-size:16px;font-weight:500;transition:background-color 0.2s;}.btn-back:hover{background-color:var(--red-dark);}.header{position:relative;margin-bottom:18px;}</style></head><body><div class=\"container\"><div class=\"header\"><button class=\"btn-back\" onclick=\"goBack()\">返回</button><h1>OTA固件升级</h1></div><div class=\"info\"><p><strong>当前版本:</strong> v1.0.0</p><p><strong>分区方案:</strong> OTA双分区</p><p><strong>当前分区:</strong> <!-- 这里应由后端动态填充分区名 --></p></div><h3>方式1: 从URL升级</h3><input type=\"text\" id=\"otaUrl\" placeholder=\"http://yourserver.com/firmware.bin\"><button class=\"btn-primary\" onclick=\"startOTAFromURL()\">开始URL升级</button><h3>方式2: 上传固件文件</h3><input type=\"file\" id=\"otaFile\" accept=\".bin\"><button class=\"btn-secondary\" onclick=\"startOTAFromFile()\">开始文件升级</button><div class=\"progress\" id=\"progressBar\" style=\"display:none;\"><div class=\"progress-bar\" id=\"progress\">0%</div></div><p id=\"status\"></p></div><script>function goBack(){window.location.href = '../';}function startOTAFromURL(){var url=document.getElementById('otaUrl').value;if(!url){alert('请输入URL');return;}document.getElementById('progressBar').style.display='block';document.getElementById('status').innerText='正在下载固件...';fetch('/ota/url?url='+encodeURIComponent(url)).then(r=>r.json()).then(data=>{if(data.success){document.getElementById('status').innerText='升级成功!设备将重启...';document.getElementById('progress').style.width='100%';document.getElementById('progress').innerText='100%';setTimeout(function(){window.location.href='/?ota=success';},1200);}else{document.getElementById('status').innerText='升级失败: '+data.error;}});pollProgress();}function startOTAFromFile(){var file=document.getElementById('otaFile').files[0];if(!file){alert('请选择文件');return;}document.getElementById('progressBar').style.display='block';document.getElementById('status').innerText='正在上传固件...';var formData=new FormData();formData.append('firmware',file);var xhr=new XMLHttpRequest();xhr.upload.onprogress=function(e){if(e.lengthComputable){var pct=Math.round((e.loaded/e.total)*100);document.getElementById('progress').style.width=pct+'%';document.getElementById('progress').innerText=pct+'%';}};xhr.onload=function(){if(xhr.status==200){var resp=JSON.parse(xhr.responseText);if(resp.success){document.getElementById('status').innerText='升级成功!设备将重启...';setTimeout(function(){window.location.href='/?ota=success';},1200);}else{document.getElementById('status').innerText='升级失败: '+resp.error;}}};xhr.open('POST','/ota/upload');xhr.send(formData);}function pollProgress(){var interval=setInterval(function(){fetch('/ota/progress').then(r=>r.json()).then(data=>{var pct=data.progress;document.getElementById('progress').style.width=pct+'%';document.getElementById('progress').innerText=pct+'%';if(pct>=100)clearInterval(interval);});},500);}</script></body></html>";
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
    String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>ESP32 配置页面</title></head><body><div class=\"container\"><h2>ESP32 设置</h2><div class=\"main-buttons\"><button type=\"button\" class=\"btn search-btn\" onclick=\"openCitySearch()\"><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"11\" cy=\"11\" r=\"8\"></circle><line x1=\"21\" y1=\"21\" x2=\"16.65\" y2=\"16.65\"></line></svg><span>搜索城市</span></button><button type=\"button\" class=\"btn ota-btn\" onclick=\"location.href='/ota'\"><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4\"></path><polyline points=\"17 8 12 3 7 8\"></polyline><line x1=\"12\" y1=\"3\" x2=\"12\" y2=\"15\"></line></svg><span>OTA升级</span></button><button type=\"button\" id=\"toggle-key-btn\" class=\"btn key-btn\" onclick=\"toggleKeySettings()\"><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21 2l-2 2m-7.61 7.61a5.5 5.5 0 1 1-7.778 7.778 5.5 5.5 0 0 1 7.777-7.777zm0 0L15.5 7.5m0 0l3 3L22 7l-3-3m-3.5 3.5L19 4\"></path></svg><span>和风天气密钥</span></button><button type=\"button\" class=\"btn exit-btn\" onclick=\"exitSettings()\"><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4\"></path><polyline points=\"16 17 21 12 16 7\"></polyline><line x1=\"21\" y1=\"12\" x2=\"9\" y2=\"12\"></line></svg><span>退出设置</span></button></div><form id=\"key-settings-form\" action=\"/set\" method=\"GET\" style=\"display:none;\"><div class=\"key-fields\">";

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
    html += "<label>和风天气 API Host:</label><input type='text' name='host' value='" + host_val + "'><br><label for=\"kid\">凭据ID:</label><input id=\"kid\" type=\"text\" name=\"kid\" value=\"" + kid_val + "\"><label for=\"project\">项目ID:</label><input id=\"project\" type=\"text\" name=\"project\" value=\"" + project_val + "\"><label for=\"key\">私钥:</label><input id=\"key\" type=\"text\" name=\"key\" value=\"" + key_val + "\"></div><div class=\"button-row\"><input type=\"submit\" value=\"提交\" class=\"btn submit-btn\"></div></form></div><style>:root{--blue:#0067b6;--blue-dark:#0045a4;--red:#e57373;--red-dark:#d35f5f;--cyan:#77eedd;--cyan-dark:#55ccbb;--gray-dark:#6a7690;--gray:#6c757d;--gray-light:#f8f9fa;--gray-lighter:#343a40;--white:#fff;--border:#dee2e6;--shadow:0 4px 12px rgba(0,0,0,0.08);--border-radius:8px;}*{box-sizing:border-box;margin:0;padding:0;}body{background:linear-gradient(90deg,rgba(179,255,253,0.5) 0%,rgba(227,230,255,0.5) 50%,rgba(253,229,245,0.5) 100%);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:var(--gray-light);color:var(--gray-lighter);line-height:1.5;}.container{max-width:500px;margin:32px auto;padding:24px;background:var(--white);border-radius:var(--border-radius);box-shadow:var(--shadow);}h2{text-align:center;margin-bottom:24px;color:var(--blue);font-weight:600;}.main-buttons{display:grid;grid-template-columns:1fr 1fr;gap:16px;}.btn{display:flex;align-items:center;justify-content:center;gap:8px;padding:12px;border:none;border-radius:var(--border-radius);font-size:16px;font-weight:500;cursor:pointer;transition:background-color 0.2s ease,transform 0.1s ease;color:var(--white);text-decoration:none;}.btn:hover{transform:translateY(-2px);}.btn:active{transform:translateY(0);}.btn svg{vertical-align:middle;}.search-btn{background-color:var(--blue);}.search-btn:hover{background-color:var(--blue-dark);}.ota-btn{background-color:var(--cyan);color:var(--gray-lighter);}.ota-btn:hover{background-color:var(--cyan-dark);}.key-btn{background-color:var(--gray-dark);}.key-btn:hover{background-color:var(--gray-dark);}.exit-btn{background-color:var(--red);}.exit-btn:hover{background-color:var(--red-dark);}#key-settings-form{margin-top:24px;padding:20px;background-color:#fdfdff;border:1px solid var(--border);border-radius:var(--border-radius);}.key-fields{display:flex;flex-direction:column;gap:12px;}.key-fields label{font-weight:500;color:var(--gray);}.key-fields input[type=text]{width:100%;padding:10px;border:1px solid var(--border);border-radius:6px;font-size:16px;transition:border-color 0.2s,box-shadow 0.2s;}.key-fields input[type=text]:focus{outline:none;border-color:var(--blue);box-shadow:0 0 0 3px rgba(0,123,255,0.25);}.button-row{display:flex;justify-content:flex-end;margin-top:20px;}.submit-btn{background-color:var(--blue);}.submit-btn:hover{background-color:var(--blue-dark);}</style><script>function openCitySearch(){window.location.href='/citysearch';}function exitSettings(){fetch('/exit').finally(()=>{alert('请手动关闭此页面。');document.body.innerHTML = `<div class=\"container\" style=\"max-width: 420px; margin: 60px auto; padding: 36px 28px; background: var(--white); border-radius: var(--border-radius); box-shadow: var(--shadow); display: flex; flex-direction: column; align-items: center;\"><svg width=\"56\" height=\"56\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#0067b6\" stroke-width=\"2.2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" style=\"margin-bottom: 18px;\"><circle cx=\"12\" cy=\"12\" r=\"10\" fill=\"#eaf6ff\"/><polyline points=\"8 12.5 11 15.5 16 10.5\" stroke=\"#2ecc71\" stroke-width=\"2.2\" fill=\"none\"/><circle cx=\"12\" cy=\"12\" r=\"10\" stroke=\"#0067b6\" stroke-width=\"2.2\" fill=\"none\"/></svg><h2 style=\"color: var(--primary-color); font-weight: 600; margin-bottom: 12px;\">设置已完成</h2><p style=\"font-size: 1.18em; color: var(--dark-gray); margin-bottom: 18px;\">请手动关闭此页面</p></div><style>body { background: linear-gradient(90deg, rgba(179,255,253,0.5) 0%, rgba(227,230,255,0.5) 50%, rgba(253,229,245,0.5) 100%); }</style>`;});}function toggleKeySettings(){var form=document.getElementById('key-settings-form');var btn=document.getElementById('toggle-key-btn').querySelector('span');if(form.style.display==='none'){form.style.display='block';btn.textContent='收起密钥设置';}else{form.style.display='none';btn.textContent='和风天气密钥';}}</script></body></html>";

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
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>城市搜索</title><style>:root{--blue:#0067b6;--blue-dark:#0045a4;--gray-dark:#6a7690;--gray:#6c757d;--gray-light:#f8f9fa;--gray-lighter:#343a40;--white:#fff;--border:#dee2e6;--shadow:0 4px 12px rgba(0,0,0,0.08);--border-radius:8px;}*{box-sizing:border-box;margin:0;padding:0;}body{background:linear-gradient(90deg,rgba(179,255,253,0.5) 0%,rgba(227,230,255,0.5) 50%,rgba(253,229,245,0.5) 100%);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:var(--gray-light);color:var(--gray-lighter);line-height:1.5;}.container{max-width:420px;margin:40px auto;padding:28px 24px;background:var(--white);border-radius:var(--border-radius);box-shadow:var(--shadow);}h2{text-align:center;margin-bottom:24px;color:var(--blue);font-weight:600;}.search-form{display:flex;flex-direction:column;gap:16px;}.search-form input[type=text]{width:100%;padding:10px;border:1px solid var(--border);border-radius:6px;font-size:16px;transition:border-color 0.2s,box-shadow 0.2s;}.search-form input[type=text]:focus{outline:none;border-color:var(--blue);box-shadow:0 0 0 3px rgba(0,123,255,0.15);}.search-form input[type=submit]{background-color:var(--blue);color:var(--white);padding:12px;border:none;border-radius:var(--border-radius);font-size:16px;font-weight:500;cursor:pointer;transition:background-color 0.2s,transform 0.1s;}.search-form input[type=submit]:hover{background-color:var(--blue-dark);transform:translateY(-2px);}</style></head><body><div class='container'><div class='header' style='position:relative;margin-bottom:18px;'><button class='btn-back' onclick='goBack()'>返回</button><h2 style='margin:0;'>城市搜索</h2></div><style>.btn-back{position:absolute;left:0px;top:0;background-color:var(--red-dark,#d35f5f);color:var(--white);border:none;border-radius:var(--border-radius);padding:10px 20px;cursor:pointer;font-size:16px;font-weight:500;transition:background-color 0.2s;}.btn-back:hover{background-color:var(--red-dark,#d35f5f);}</style><form class='search-form' action='/citysearch_result' method='GET'><input type='text' name='location' placeholder='请输入城市名或拼音' required><input type='submit' value='搜索'></form></div></body><script>function goBack(){window.location.href='../';}</script></html>";
        setting_server.send(200, "text/html; charset=utf-8", html);
    });

    // 城市搜索结果页面
    setting_server.on("/citysearch_result", [](){
        if (!setting_server.hasArg("location")) {
            String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>缺少搜索参数</title><style>:root{--blue:#0067b6;--blue-dark:#0045a4;--red:#e57373;--red-dark:#d35f5f;--gray-dark:#6a7690;--gray:#6c757d;--gray-light:#f8f9fa;--gray-lighter:#343a40;--white:#fff;--border:#dee2e6;--shadow:0 4px 12px rgba(0,0,0,0.08);--border-radius:8px;}*{box-sizing:border-box;margin:0;padding:0;}body{background:linear-gradient(90deg,rgba(179,255,253,0.5) 0%,rgba(227,230,255,0.5) 50%,rgba(253,229,245,0.5) 100%);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:var(--gray-light);color:var(--gray-lighter);line-height:1.5;}.container{max-width:420px;margin:60px auto;padding:36px 28px;background:var(--white);border-radius:var(--border-radius);box-shadow:var(--shadow);display:flex;flex-direction:column;align-items:center;}.icon-error{width:56px;height:56px;margin-bottom:18px;}h2{color:var(--red-dark);font-weight:600;margin-bottom:12px;}.desc{font-size:1.1em;color:var(--gray-dark);margin-bottom:8px;}</style><script>setTimeout(function(){window.location.href='/';},2000);</script></head><body><div class='container'><svg class='icon-error' viewBox='0 0 24 24' fill='none' stroke='#e57373' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='10' fill='#ffeaea'/><line x1='8' y1='8' x2='16' y2='16' stroke='#e57373' stroke-width='2.2'/><line x1='16' y1='8' x2='8' y2='16' stroke='#e57373' stroke-width='2.2'/><circle cx='12' cy='12' r='10' stroke='#e57373' stroke-width='2.2' fill='none'/></svg><h2>缺少搜索参数</h2><div class='desc'>2秒后自动返回主页...</div></div></body></html>";
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
        
        html += "<div style='max-width:420px;margin:40px auto;padding:28px 24px;background:#fff;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,0.08);'>";
        html += "<h2 style='text-align:center;color:#0067b6;font-weight:600;margin-bottom:24px;'>搜索结果</h2>";
        html += "<ul style='list-style:none;padding:0;margin:0;'>";
        for (JsonObject obj : locArr) {
            String name = obj["name"].as<String>();
            String adm1 = obj["adm1"].as<String>();
            String country = obj["country"].as<String>();
            String locid = obj["id"].as<String>();
            String fxlink = obj["fxLink"].as<String>();
            html += "<li style='margin:16px 0;display:flex;justify-content:center;'>";
            html += "<form action='/set_location' method='POST' style='display:inline;'>";
            html += "<input type='hidden' name='locid' value='" + locid + "'>";
            html += "<input type='hidden' name='fxlink' value='" + fxlink + "'>";
            html += "<button type='submit' style='padding:12px 28px;border-radius:6px;background:#0067b6;color:#fff;border:none;cursor:pointer;font-size:16px;font-weight:500;box-shadow:0 2px 8px rgba(0,103,182,0.10);transition:background 0.2s;min-width:180px;'>" + name + " (" + adm1 + ", " + country + ")</button>";
            html += "</form></li>";
        }
        html += "</ul>";
        html += "</div>";
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
        String response = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>设置成功</title><style>:root{--blue:#0067b6;--blue-dark:#0045a4;--green:#4CAF50;--gray-dark:#6a7690;--gray:#6c757d;--gray-light:#f8f9fa;--gray-lighter:#343a40;--white:#fff;--border:#dee2e6;--shadow:0 4px 12px rgba(0,0,0,0.08);--border-radius:8px;}*{box-sizing:border-box;margin:0;padding:0;}body{background:linear-gradient(90deg,rgba(179,255,253,0.5) 0%,rgba(227,230,255,0.5) 50%,rgba(253,229,245,0.5) 100%);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:var(--gray-light);color:var(--gray-lighter);line-height:1.5;}.container{max-width:420px;margin:60px auto;padding:36px 28px;background:var(--white);border-radius:var(--border-radius);box-shadow:var(--shadow);display:flex;flex-direction:column;align-items:center;}.icon-success{width:56px;height:56px;margin-bottom:18px;}h2{color:var(--green);font-weight:600;margin-bottom:12px;}.info{margin:10px 0;color:var(--gray);font-size:1.08em;}.desc{font-size:1.1em;color:var(--gray-dark);margin-bottom:8px;}</style><script>setTimeout(function(){window.location.href='/';},2000);</script></head><body><div class='container'><svg class='icon-success' viewBox='0 0 24 24' fill='none' stroke='#4CAF50' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='10' fill='#eaf6ff'/><polyline points='8 12.5 11 15.5 16 10.5' stroke='#4CAF50' stroke-width='2.2' fill='none'/><circle cx='12' cy='12' r='10' stroke='#4CAF50' stroke-width='2.2' fill='none'/></svg><h2>✓ 设置成功</h2><div class='info'>LocationID: " + locid + "</div><div class='info'>城市: " + cityname + "</div><div class='desc'>窗口将在2秒后自动关闭...</div></div></body></html>";
        
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