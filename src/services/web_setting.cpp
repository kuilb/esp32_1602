#include "web_setting.h"

WebServer setting_server(80);
volatile bool isConfigDone = false;
volatile bool isKeyDone = false;

// OTA页面处理
void webSettingHandleOTA() {
    String html = getWebComponent();
    html += R"(
<!DOCTYPE html>
<html>

<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>OTA升级</title>
    <style>
        .progress {
            width: 100%;
            max-width: 400px;
            height: 40px;
            background: linear-gradient(to right, #e8f4f8, #f0f0f0);
            border-radius: 20px;
            margin: 20px auto 0 auto;
            overflow: hidden;
            box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.1);
            position: relative;
        }

        .progress-bar {
            height: 100%;
            background: linear-gradient(90deg, var(--blue) 0%, var(--cyan) 100%);
            width: 0%;
            transition: width 0.4s ease;
            text-align: center;
            line-height: 40px;
            color: var(--white);
            font-weight: 600;
            font-size: 15px;
            box-shadow: 0 2px 8px rgba(0, 103, 182, 0.3);
            position: relative;
            overflow: hidden;
        }

        .progress-bar::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.3), transparent);
            animation: shimmer 2s infinite;
        }

        @keyframes shimmer {
            0% {
                left: -100%;
            }

            100% {
                left: 100%;
            }
        }

        #status {
            margin-top: 16px;
            min-height: 24px;
            color: var(--blue);
            font-size: 1.08em;
        }
    </style>
</head>

<body>
    <web-styles></web-styles>
    <div class='container'>
        <button class='btn-red' onclick='goBack()' style='position: absolute; left: 25px; top: 20;'>返回</button>

        <h1>OTA固件升级</h1>
        <h3>方式1: 从URL升级</h3>
        <input type='text' id='otaUrl' placeholder='http://yourserver.com/firmware.bin'>
        <button class='btn-blue' onclick='startOTAFromURL(event)'>开始URL升级</button>
        <br><br>

        <h3>方式2: 上传固件文件</h3>
        <input type='file' id='otaFile' accept='.bin'>
        <button class='btn-cyan' onclick='startOTAFromFile()'>开始文件升级</button>
        <div class='progress' id='progressBar' style='display:none;'>
            <div class='progress-bar' id='progress'>0%</div>
        </div>
        <p id='status'></p>
    </div>
    <script>
        var pollInterval = null;
        function goBack() {
            window.location.href = '../';
        }
        function OTASuccess() {
            document.getElementById('status').innerText = '升级成功!设备将重启...';
            setTimeout(function () { 
                alert('请手动关闭此页面。');
                document.body.innerHTML = `
                <div class='container'>
                    <svg width='56' height='56' viewBox='0 0 24 24' fill='none' stroke='#0067b6' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round' style='margin-bottom: 18px;'>
                        <circle cx='12' cy='12' r='10' fill='#eaf6ff' />
                        <polyline points='8 12.5 11 15.5 16 10.5' stroke='#2ecc71' stroke-width='2.2' fill='none' />
                        <circle cx='12' cy='12' r='10' stroke='#0067b6' stroke-width='2.2' fill='none' />
                    </svg>
                    <h1>OTA已完成</h1>
                    <p style='font-size: 1.18em; color: var(--gray-lighter); margin-bottom: 18px;'>请手动关闭此页面</p>
                </div>
                `;
            }, 2000);
        }
        function startOTAFromURL(event) {
            var url = document.getElementById('otaUrl').value;
            if (!url) { alert('请输入URL'); return; }
            if (!url.startsWith('http://') && !url.startsWith('https://')) {
                url = 'http://' + url;
            }

            var btn = event.target;
            btn.disabled = true;
            document.getElementById('status').innerText = '正在启动 OTA...';
            document.getElementById('progressBar').style.display = 'block';
            fetch('/ota/url?url=' + encodeURIComponent(url))
                .then(r => {
                    if (r.status === 202) {
                        document.getElementById('status').innerText = '正在下载固件...';
                        pollInterval = setInterval(function () {
                            // /ota/progress返回Json的 progress status error对象
                            fetch('/ota/progress')
                                .then(r => r.json())
                                .then(data => {
                                    var pct = data.progress || 0;
                                    document.getElementById('progress').style.width = pct + '%';
                                    document.getElementById('progress').innerText = pct + '%';
                                    if (data.status === 'success') {
                                        clearInterval(pollInterval);
                                        OTASuccess();
                                    } else if (data.status === 'failed') {
                                        clearInterval(pollInterval);
                                        document.getElementById('status').innerText = '升级失败: ' + data.error;
                                        btn.disabled = false;
                                    }
                                })
                                .catch(() => { });
                        }, 500);
                    } else if (!r.ok) {
                        throw new Error('HTTP ' + r.status);
                    }
                })
                .catch(error => {
                    document.getElementById('status').innerText = '启动OTA失败: ' + error.message;
                    btn.disabled = false;
                    document.getElementById('progressBar').style.display = 'none';
                });
        }

        function startOTAFromFile() {
            var file = document.getElementById('otaFile').files[0];
            if (!file) { alert('请选择文件'); return; }

            document.getElementById('progressBar').style.display = 'block';
            document.getElementById('status').innerText = '正在上传固件...';

            var formData = new FormData();
            formData.append('firmware', file);
            var xhr = new XMLHttpRequest();
            xhr.upload.onprogress = function (e) {
                if (e.lengthComputable) {
                    var pct = Math.round((e.loaded / e.total) * 100);
                    document.getElementById('progress').style.width = pct + '%';
                    document.getElementById('progress').innerText = pct + '%';
                }
            };

            xhr.onload = function () {
                if (xhr.status == 200) {
                    var resp = JSON.parse(xhr.responseText);
                    if (resp.success) {
                        OTASuccess();
                    } else {
                        document.getElementById('status').innerText = '升级失败: ' + resp.error;
                    }
                }
                else {
                    document.getElementById('status').innerText = '上传失败: HTTP ' + xhr.status;
                }
            };
            xhr.open('POST', '/ota/upload');
            xhr.send(formData);
        }
    </script>
</body>

</html>
    )";
    setting_server.send(200, "text/html; charset=utf-8", html);
}

// OTA URL处理
void webSettingHandleOTAURL() {

    // 如果发送的请求不含URL那么 HTTP400 Bad Request
    if (!setting_server.hasArg("url")) {
        setting_server.send(400, "application/json", "{\"success\":false,\"error\":\"No URL\"}");
        return;
    }
    
    // 如果已有 OTA 在进行那么 HTTP409 Conflict
    if (OTAManager::isInProgress()) {
        setting_server.send(409, "application/json", 
            "{\"success\":false,\"error\":\"OTA already in progress\"}");
        return;
    }
    
    String url = setting_server.arg("url");
    LOG_SYSTEM_INFO("OTA from URL: %s", url.c_str());
    
    // 先响应前端,告诉它 OTA 已开始 (HTTP 202 Accepted)
    setting_server.send(202, "application/json", 
        "{\"success\":true,\"message\":\"OTA started, please poll progress\"}");
    
    // 创建独立任务执行 OTA (不阻塞主循环)
    xTaskCreate([](void* param) {
        String* urlPtr = (String*)param;
        OTAResult result = OTAManager::updateFromURL(*urlPtr);
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
    
    String json = "{\"progress\":" + String(progress) + 
                  ",\"status\":" + statusStr +
                  ",\"error\":\"" + OTAManager::getErrorString() + "\"}";
    setting_server.send(200, "application/json", json);
}

// OTA文件上传处理
void webSettingHandleOTAUpload() {
    HTTPUpload& upload = setting_server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        LOG_SYSTEM_INFO("OTA Upload Start: %s", upload.filename.c_str());
        lcdText("Uploading...", 1);
        lcdText(upload.filename, 2);
        updateColor(CRGB::Orange);
        
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            LOG_SYSTEM_ERROR("OTA begin failed");
            lcdText("OTA Begin Fail", 1);
            lcdText("", 2);
        }
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) {
        // 如果写入字节不匹配
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            LOG_SYSTEM_ERROR("OTA write failed");
            lcdText("OTA Write Fail", 1);
            lcdText("", 2);
        }
    } 
    else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            LOG_SYSTEM_INFO("OTA Success! Size: %u", upload.totalSize);
            lcdText("OTA Success!", 1);
            lcdText("Rebooting...", 2);
            updateColor(CRGB::Green);
            setting_server.send(200, "application/json", "{\"success\":true}");
            delay(1000);
            ESP.restart();
        } else {
            // 如果结束时出错，HTTP500 Internal Server Error
            LOG_SYSTEM_ERROR("OTA End failed: %s", Update.errorString());
            setting_server.send(500, "application/json", 
                "{\"success\":false,\"error\":\"" + String(Update.errorString()) + "\"}");
        }
    }
}

String errorCitySerachHandle(String errorMsg) {
    String html = getWebComponent();

    html += R"(
<!DOCTYPE html>
<html>
<head>
	<meta charset='UTF-8'>
	<meta name='viewport' content='width=device-width, initial-scale=1.0'>
	<title>城市搜索</title>
</head>
<body>
	<web-styles></web-styles>
	<div class='container'>
		<svg class='icon-error' width='56' height='56' viewBox='0 0 24 24' fill='none' stroke='#e57373' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='10' fill='#ffeaea'/><line x1='8' y1='8' x2='16' y2='16' stroke='#e57373' stroke-width='2.2'/><line x1='16' y1='8' x2='8' y2='16' stroke='#e57373' stroke-width='2.2'/><circle cx='12' cy='12' r='10' stroke='#e57373' stroke-width='2.2' fill='none'/></svg>
		<h1 style='color: var(--red)'>)" + errorMsg + R"("</h1>
		<div class='desc'>2秒后自动返回主页...</div>
	</div>
</body>
<script>
		setTimeout(function(){window.location.href = '/';}, 2000);
</script>
</html>
    )";
    return html;
}

// 主页处理函数
void webSettingHandleRoot() {
    String varApiHost = apiHost;
    String varKid = kid;
    String varProjectID = projectID;
    String varBase64Key = base64Key;

    String html = getWebComponent();

    html += R"(
<!DOCTYPE html>
<html>

<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>ESP32 配置页面</title>
    <style>
        .main-buttons {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 16px;
        }

        .button-row {
            display: flex;
            justify-content: flex-end;
            margin-top: 10px;
        }

        #key-settings-form {
            text-align: left;
            width: 80%;
            display: none;
        }
    </style>
</head>

<body>
    <web-styles></web-styles>
    <div class='container'>
        <h1>ESP32 设置</h1>
        <div class='main-buttons'>
            <button type='button' class='btn-blue' onclick='openCitySearch()'><svg xmlns='http://www.w3.org/2000/svg'
                    width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'
                    stroke-linecap='round' stroke-linejoin='round'>
                    <circle cx='11' cy='11' r='8'></circle>
                    <line x1='21' y1='21' x2='16.65' y2='16.65'></line>
                </svg><span>搜索城市</span></button>
            <button type='button' class='btn-cyan' onclick="location.href='/ota'
                "><svg xmlns='http://www.w3.org/2000/svg' width='18' height='18' viewBox='0 0 24 24' fill='none'
                    stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>
                    <path d='M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4'></path>
                    <polyline points='17 8 12 3 7 8'></polyline>
                    <line x1='12' y1='3' x2='12' y2='15'></line>
                </svg><span>OTA升级</span></button>
            <button type='button' id='toggle-key-btn' class='btn-gray' onclick='toggleKeySettings()'><svg
                    xmlns='http://www.w3.org/2000/svg' width='18' height='18' viewBox='0 0 24 24' fill='none'
                    stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>
                    <path
                        d='M21 2l-2 2m-7.61 7.61a5.5 5.5 0 1 1-7.778 7.778 5.5 5.5 0 0 1 7.777-7.777zm0 0L15.5 7.5m0 0l3 3L22 7l-3-3m-3.5 3.5L19 4'>
                    </path>
                </svg><span>和风天气密钥</span></button>
            <button type='button' class='btn-red' onclick='exitSettings()'><svg xmlns='http://www.w3.org/2000/svg'
                    width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'
                    stroke-linecap='round' stroke-linejoin='round'>
                    <path d='M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4'></path>
                    <polyline points='16 17 21 12 16 7'></polyline>
                    <line x1='21' y1='12' x2='9' y2='12'></line>
                </svg><span>退出设置</span></button>
        </div>
        <form id='key-settings-form' action='/set' method='GET' style='margin-top: 10px;'>
            <label>和风天气 API Host:</label>
            <input type='text' name='host' id='apiHost' value=')" + varApiHost +  R"(' required>

            <label for='kid'>凭据ID:</label>
            <input id='kid' type='text' name='kid' value=')" + varKid +  R"(' required>

            <label for='project'>项目ID:</label>
            <input id='project' type='text' name='project' value=')" + varProjectID + R"(' required>

            <label for='key'>私钥:</label>
            <input id='key' type='text' name='key' value=')" + varBase64Key + R"(' required>
            <div style='display: flex; justify-content: flex-end; margin-top: 10px;'>
                <button type='button' class='btn-blue' onclick='submitKeySettings()'>提交</button>
            </div>
            <p id='key-error' style='color:#e57373;margin-top:8px;display:none;'>请完整填写所有字段。</p>
        </form>
    </div>
    <script>
        function openCitySearch() {
            window.location.href = '/citysearch';
        }
        function exitSettings() {
            fetch('/exit').finally(() => {
                alert('请手动关闭此页面。');
                document.body.innerHTML = `
                <div class='container'>
                    <svg width='56' height='56' viewBox='0 0 24 24' fill='none' stroke='#0067b6' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round' style='margin-bottom: 18px;'>
                        <circle cx='12' cy='12' r='10' fill='#eaf6ff' />
                        <polyline points='8 12.5 11 15.5 16 10.5' stroke='#2ecc71' stroke-width='2.2' fill='none' />
                        <circle cx='12' cy='12' r='10' stroke='#0067b6' stroke-width='2.2' fill='none' />
                    </svg>
                    <h1>设置已完成</h1>
                    <p style='font-size: 1.18em; color: var(--gray-lighter); margin-bottom: 18px;'>请手动关闭此页面</p>
                </div>
                `;
            });
        }
        function submitKeySettings() {
            var host = document.getElementById('apiHost').value.trim();
            var kid = document.getElementById('kid').value.trim();
            var project = document.getElementById('project').value.trim();
            var key = document.getElementById('key').value.trim();
            var errorEl = document.getElementById('key-error');
            if (!host || !kid || !project || !key) {
                errorEl.style.display = 'block';
                return;
            }
            errorEl.style.display = 'none';
            fetch(`/set?host=${encodeURIComponent(host)}&kid=${encodeURIComponent(kid)}&project=${encodeURIComponent(project)}&key=${encodeURIComponent(key)}`)
                .then(function (res) { return res.json ? res.json() : res.text(); })
                .then(function (data) {
                    if (data && data.error) {
                        errorEl.textContent = data.error;
                        errorEl.style.display = 'block';
                    } else {
                        alert('保存成功');
                    }
                })
                .catch(function () {
                    errorEl.textContent = '提交失败，请稍后重试。';
                    errorEl.style.display = 'block';
                });
        }
        function toggleKeySettings() {
            var form = document.getElementById('key-settings-form');
            var btn = document.getElementById('toggle-key-btn').querySelector('span');
            if (form.style.display === 'block') {
                form.style.display = 'none';
                btn.textContent = '和风天气密钥';
            }
            else {
                form.style.display = 'block';
                btn.textContent = '收起密钥设置';
            }
        }
    </script>
</body>

</html>
    )";

    setting_server.send(200, "text/html; charset=utf-8", html);
}


// 保存 JWT 配置信息
bool saveJWTConfig(const String& apiHost,
                   const String& kid,
                   const String& projectID,
                   const String& privateKey) {

    File file = SPIFFS.open("/jwt_config.txt", "w");
    if (!file) {
        LOG_WEATHER_ERROR("Failed to save JWT config - cannot open file");
        return false;
    }

    if (!file.println(apiHost)) {
        LOG_WEATHER_ERROR("Failed to save JWT config - write API Host failed");
        file.close();
        return false;
    }

    if (!file.println(kid)) {
        LOG_WEATHER_ERROR("Failed to save JWT config - write kid failed");
        file.close();
        return false;
    }

    if (!file.println(projectID)) {
        LOG_WEATHER_ERROR("Failed to save JWT config - write projectID failed");
        file.close();
        return false;
    }

    if (!file.println(privateKey)) {
        LOG_WEATHER_ERROR("Failed to save JWT config - write private key failed");
        file.close();
        return false;
    }

    file.flush();
    file.close();

    LOG_WEATHER_INFO("JWT configuration saved successfully");
    return true;
}

// 处理设置
void web_setting_handleSet() {
    // 基本参数存在性检查
    if (!(setting_server.hasArg("host") &&
          setting_server.hasArg("kid") &&
          setting_server.hasArg("project") &&
          setting_server.hasArg("key"))) {
        setting_server.send(400, "application/json; charset=utf-8", "{\"error\":\"提交数据不完整\"}");
        return;
    }

    String apiHost = setting_server.arg("host");
    String kid = setting_server.arg("kid");
    String project = setting_server.arg("project");
    String privateKey = setting_server.arg("key");

    apiHost.trim();
    kid.trim();
    project.trim();
    privateKey.trim();

    // 简单有效性校验：不能为空
    if (apiHost.length() == 0) {
        setting_server.send(400, "application/json; charset=utf-8", "{\"error\":\"API Host 不能为空\"}");
        return;
    }
    if (kid.length() == 0) {
        setting_server.send(400, "application/json; charset=utf-8", "{\"error\":\"凭据ID 不能为空\"}");
        return;
    }
    if (project.length() == 0) {
        setting_server.send(400, "application/json; charset=utf-8", "{\"error\":\"项目ID 不能为空\"}");
        return;
    }
    if (privateKey.length() == 0) {
        setting_server.send(400, "application/json; charset=utf-8", "{\"error\":\"私钥 不能为空\"}");
        return;
    }

    // 打印到串口DEBUG等级日志
    LOG_WEATHER_DEBUG("==== Configuration received ====");
    LOG_WEATHER_DEBUG("API Host: " + apiHost);
    LOG_WEATHER_DEBUG("kid: " + kid);
    LOG_WEATHER_DEBUG("projectID: " + project);
    LOG_WEATHER_DEBUG("private key length: " + String(privateKey.length()));

    if (!saveJWTConfig(apiHost, kid, project, privateKey)) {
        // 保存失败
        setting_server.send(500, "application/json; charset=utf-8", "{\"error\":\"保存配置失败\"}");
        return;
    }

    // 保存成功
    setting_server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

void web_setting_setupWebServer() {
    if(wifiConnectionState != WIFI_CONNECTED) {
        LOG_SYSTEM_WARN("Web setting server setup called but WiFi not connected");
        lcdText("WiFi Not Conn", 1);
        lcdText(" ", 2);
        return;
    }

    isConfigDone=false;

    setting_server.on("/", webSettingHandleRoot);       // 主页
    setting_server.on("/set", web_setting_handleSet);      // 设置参数

    // OTA相关路由
    setting_server.on("/ota", webSettingHandleOTA);
    setting_server.on("/ota/url", webSettingHandleOTAURL);
    setting_server.on("/ota/progress", webSettingHandleOTAProgress);
    setting_server.on("/ota/upload", HTTP_POST, 
        []() { 
            /* 上传完成后的响应 */ 
        },
        webSettingHandleOTAUpload  // 上传处理函数
    );

    setting_server.on("/exit", [](){
        isConfigDone = true;
        setting_server.send(200, "text/plain", "Exiting configuration...");
    });

    // 城市搜索页面
    setting_server.on("/citysearch", [](){
        String html = getWebComponent();
        html += R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>城市搜索</title>
</head>
<body>
    <web-styles></web-styles>
    <div class='container' style='position: relative;'>
        <button type='button' class='btn-red' onclick='goBack()' style='position: absolute; left: 25px; top: 20;'>返回</button>
        <div style='margin-bottom:18px;'>
            <h1 style='text-align: center; margin:0;'>城市搜索</h1>
        </div>
        <form action='/citysearch_result' method='GET'>
            <input type='text' name='location' placeholder='请输入城市名或拼音' required style='width: 100%;'>
            <div style='display: flex;flex-direction: column; gap: 16px; width: 100%;'>
                <input type='submit' value='搜索' class='btn-blue' onclick='showLoading()'>
            </div>
        </form>
        <div class='loading hidden'>
            <div class='spinner'></div>
            <p>搜索中...</p>
        </div>
    </div>
</body>
<script>
    function goBack() {
        window.location.href = '../';
    }
    function showLoading() {
        var input = document.querySelector('input[name=\'location\']');
        if (input.value.trim() === '') return;
        document.querySelector('.loading').classList.remove('hidden');
    }
</script>
</html>
        )";

        setting_server.send(200, "text/html; charset=utf-8", html);
    });

    // 城市搜索结果页面
    setting_server.on("/citysearch_result", [](){
        if (!setting_server.hasArg("location")) {
            LOG_WEATHER_WARN("City search request missing 'location' parameter");
            setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("请输入地名"));
            return;
        }

        String location = setting_server.arg("location");
        location.trim();

        // 读取API配置
        String varApiHost = apiHost;
        varApiHost.trim();
        String varKid = kid;
        varKid.trim();
        String varProjectID = projectID;
        varProjectID.trim();
        String varBase64Key = base64Key;
        varBase64Key.trim();

        String jwtToken;
        jwtToken.trim();
        
        // 确保先生成seed32
        generateSeed32();

        // 生成JWT
        jwtToken = generate_jwt(varKid, varProjectID, seed32);
        LOG_WEATHER_DEBUG("City search JWT token generated, length: " + String(jwtToken.length()));
           
        if (varApiHost.length() == 0 || jwtToken.length() == 0) {
            LOG_WEATHER_ERROR("API configuration missing for city search (host or token empty)");
            setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("API配置错误"));
            return;
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
            setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("请求失败，HTTP代码：" + String(httpCode)));
            return;
        }
        int payloadSize = http.getSize();
        
        // 使用RAII内存管理
        MemoryManager::SafeBuffer compressedBuffer(payloadSize + 8, "CitySearch_Response");
        if (!compressedBuffer.isValid()) {
            http.end();
            setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("内存分配失败"));
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
                setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("Gzip解压失败"));
                return;
            }
            
            MemoryManager::SafeBuffer uncompressedBuffer(uncompSize + 8, "CitySearch_Decompressed");
            if (!uncompressedBuffer.isValid()) {
                setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("内存分配失败"));
                return;
            }
            
            int rc = zt.gunzip(compressedBuffer.get(), iCount, uncompressedBuffer.get());
            if (rc != ZT_SUCCESS) {
                setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("Gzip解压失败"));
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
            setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("JSON解析失败"));
            return;
        }
        
        LOG_WEATHER_DEBUG("City search JSON parsed successfully");
        LOG_WEATHER_DEBUG("JSON response: " + jsonData.substring(0, min(200, (int)jsonData.length())));
        
        // 检查API响应状态
        if (doc["code"].as<String>() != "200") {
            String code = doc["code"].as<String>();
            LOG_WEATHER_WARN("City search API error code: " + code);
            setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("API错误，错误代码：" + code));
            return;
        }
        
        // 检查location字段是否存在且为数组
        if (!doc["location"].is<JsonArray>()) {
            LOG_WEATHER_WARN("City search response: location field missing or not array");
            setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("未找到候选城市"));
            return;
        }
        JsonArray locArr = doc["location"].as<JsonArray>();
        LOG_WEATHER_INFO("City search found " + String(locArr.size()) + " cities");
        
        if (locArr.size() == 0) {
            setting_server.send(200, "text/html; charset=utf-8", errorCitySerachHandle("未找到匹配的城市"));
            return;
        }

        String html = getWebComponent();
        html += R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <title>搜索结果</title>
</head>
<body>
    <web-styles></web-styles>
      <div class='container'>
          <button type='button' class='btn-red' onclick='goBack()' style='position: absolute; left: 25px; top: 20;'>返回</button>
          <h1>搜索结果</h1>
          <ul style='list-style:none;padding:0;margin:0;display:grid;grid-template-columns:1fr 1fr;gap:16px;'>
        )";
        for (JsonObject obj : locArr) {
            String name = obj["name"].as<String>();
            String adm1 = obj["adm1"].as<String>();
            String country = obj["country"].as<String>();
            String locid = obj["id"].as<String>();
            String fxlink = obj["fxLink"].as<String>();

            html += "<li style='margin:0;'><form action='/set_location' method='POST' style='display:inline;'>";
            html += "<input type='hidden' name='locid' value='" + locid + "'>";
            html += "<input type='hidden' name='fxlink' value='" + fxlink + "'>";
            html += "<button type='submit' class = 'btn-blue'>" + name + " (" + adm1 + ", " + country + ")</button>";
            html += "</form></li>";
        }
        html += R"(
</ul>
    </div>
</body>
<script>
    function goBack() {
        window.location.href = '../citysearch';
    }
</script>
</html>
        )";

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
        extern char cityName[64];
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
                strncpy(cityName, newCity.c_str(), sizeof(cityName) - 1);
                cityName[sizeof(cityName) - 1] = '\0';  // 确保null结尾
                file.close();
            }
        }
        weatherSynced = false;
        isReadyToDisplay = false;
        
        // 返回成功页面并自动关闭弹出窗口
        String response = getWebComponent();
        response += R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>设置成功</title>
</head>
<body>
    <web-styles></web-styles>
    <div class='container'>
        <svg class='icon-success' width='56' height='56' viewBox='0 0 24 24' fill='none' stroke='#4CAF50' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='10' fill='#eaf6ff'/><polyline points='8 12.5 11 15.5 16 10.5' stroke='#4CAF50' stroke-width='2.2' fill='none'/><circle cx='12' cy='12' r='10' stroke='#4CAF50' stroke-width='2.2' fill='none'/></svg>
        <h1>设置成功</h1>
        <div class='info'>LocationID: )" + locid + R"(</div>
        <div class='info'>城市: )" + cityname + R"(</div>
        <div class='desc'>窗口将在2秒后自动关闭...</div>
    </div>
</body>
<script>
		setTimeout(function(){window.location.href = '/';}, 2000);
</script>
</html>
        )";

        setting_server.send(200, "text/html; charset=utf-8", response);
    });
    
    setting_server.begin();
    LOG_WEATHER_INFO("Web configuration server started, access via IP address");
    
    // 获取当前IP地址并显示
    String ipAddress = WiFi.localIP().toString();
    lcdText("Config Mode", 1);
    lcdText(ipAddress.c_str(), 2);
    while(isConfigDone == false){
        setting_server.handleClient();
        delay(1);
    }
    lcdText("Config Done", 1);
    lcdText("Exiting...", 2);
    delay(500);
}