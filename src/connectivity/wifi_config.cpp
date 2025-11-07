#include "wifi_config.h"
#include "clock.h"
#include "logger.h"

// 配网模式使用的 Web 服务器（监听端口 80）
WebServer AP_server(80);

// DNS 服务器用于强制门户
DNSServer dnsServer;
const byte DNS_PORT = 53;

// 保存的 WiFi SSID 和密码（初始化为空字符串）
String savedSSID = "", savedPassword = "";

// 当前是否处于配网模式的标志位
bool inConfigMode = false;

// WiFi连接状态
WiFiConnectionState wifiConnectionState = WIFI_IDLE;

// 保存WiFi信息
void _saveWiFiCredentials(const String& ssid, const String& password) {
	File file = SPIFFS.open("/wifi.txt", "w");
	if (!file) {
			LOG_WIFI_ERROR("保存WiFi信息失败, 无法打开文件");
			lcdText("Save WiFi Fail", 1);
			lcdText("Check FS/Retry", 2);
			return;
	}
	if (!file.println(ssid)) {
			LOG_WIFI_ERROR("保存WiFi信息失败, 写入SSID失败");
			lcdText("Save WiFi Fail", 1);
			lcdText("Write Err", 2);
			file.close();
			return;
	}
	if (!file.println(password)) {
			LOG_WIFI_ERROR("保存WiFi信息失败, 写入密码失败");
			lcdText("Save WiFi Fail", 1);
			lcdText("Write Err", 2);
			file.close();
			return;
	}
	file.flush();
	file.close();

	LOG_WIFI_INFO("WiFi config saved, restarting...");
	lcdText("Config Saved", 1);
	lcdText("Restarting...", 2);
}

// 加载WiFi信息
void _loadWiFiCredentials() {
		if (!SPIFFS.exists("/wifi.txt")) {
				savedSSID = "";
				savedPassword = "";
				return;
		}
		File file = SPIFFS.open("/wifi.txt", "r");
		if (file) {
				savedSSID = file.readStringUntil('\n');
				LOG_WIFI_DEBUG("Loaded SSID: %s", savedSSID.c_str());
				savedSSID.trim();
				savedPassword = file.readStringUntil('\n');
				savedPassword.trim();
				file.close();
		} else {
				LOG_WIFI_ERROR("Failed to open WiFi config file for reading");
				savedSSID = "";
				savedPassword = "";
		}
}

// 进入配网
void enterConfigMode() {
	updateColor(CRGB::Purple);  // 配网紫灯

	inConfigMode = true;
	WiFi.softAP("1602A_Config");

    LOG_WIFI_INFO("Entering config mode");
	LOG_WIFI_INFO("Config webpage started at IP: %s", WiFi.softAPIP().toString().c_str());

	// 启动DNS服务器，将所有域名请求劫持到ESP32的IP
	dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

	// 捕获所有DNS请求并重定向到配网页面
	AP_server.onNotFound([](){
		AP_server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		AP_server.send(302, "text/plain", "");
	});

    // 配网页面
	AP_server.on("/", [](){
		String html = "";
		html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>WiFi 配置</title><style>:root{--blue:#0067b6;--blue-dark:#0045a4;--cyan:#66ddcc;--cyan-dark:#55ccbb;--gray:#6c757d;--gray-light:#f8f9fa;--gray-lighter:#343a40;--white:#fff;--border:#dee2e6;--shadow:0 4px 12px rgba(0,0,0,0.08);--border-radius:8px;}body{background:linear-gradient(90deg,rgba(179,255,253,0.5) 0%,rgba(227,230,255,0.5) 50%,rgba(253,229,245,0.5) 100%);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:var(--gray-light);color:var(--gray-lighter);line-height:1.5;text-align:center;margin:0;}.container{max-width:500px;margin:32px auto;padding:24px;background:rgba(255,255,255,0.6);backdrop-filter:blur(10px);-webkit-backdrop-filter:blur(10px);border-radius:25px;box-shadow:var(--shadow);display:flex;flex-direction:column;align-items:center;}h1{color:var(--blue);font-weight:600;margin:0 0 18px 0;font-size:1.5em;}label{display:block;margin:0.8em 0 0.2em;color:var(--blue);font-weight:bold;font-size:1em;text-align:left;}input[type=password],input[type=text]{width:90%;padding:10px;margin:10px 0;border:1px solid var(--border);border-radius:6px;font-size:16px;transition:border-color 0.2s,box-shadow 0.2s;background:linear-gradient(135deg,rgba(255,255,255,0.8),rgba(240,248,255,0.8));box-shadow:inset 0 2px 4px rgba(0,0,0,0.1);}input[type=password]:focus,input[type=text]:focus{outline:none;border-color:var(--blue);box-shadow:0 0 0 3px rgba(0,123,255,0.15),inset 0 1px 2px rgba(0,0,0,0.1);transform:scale(1.02);}input[type=password]::placeholder,input[type=text]::placeholder{color:var(--gray);font-style:italic;opacity:0.7;}button,input[type=submit]{padding:12px 30px;margin:10px 5px;border:none;border-radius:var(--border-radius);cursor:pointer;font-size:16px;font-weight:500;transition:background-color 0.2s,transform 0.1s;color:var(--white);}.btn-primary{background-color:var(--blue);}.btn-primary:hover{background-color:var(--blue-dark);}.btn-secondary{background-color:var(--cyan);color:var(--gray-lighter);}.btn-secondary:hover{background-color:var(--cyan-dark);}.button-group{display:flex;justify-content:center;gap:10px;width:100%;max-width:340px;}.loading{display:flex;flex-direction:column;align-items:center;margin-top:20px;}.loading.hidden{display:none;}.spinner{width:40px;height:40px;border:4px solid var(--gray-light);border-top:4px solid var(--blue);border-radius:50%;animation:spin 1s linear infinite;}@keyframes spin{0%{transform:rotate(0deg);}100%{transform:rotate(360deg);}}.loading p{color:var(--blue);font-weight:500;margin-top:10px;}@media(max-width:800px){.container{width:80vw;}}</style></head>";
		html += "<body><div class='container'><h1>WiFi 配网</h1><form action='/wifi_set' method='POST' style='width:100%;max-width:340px;'><label for='manualssid'>WiFi名称(SSID):</label><input type='text' name='ssid' id='manualssid' placeholder='请输入SSID' required><label for='password'>密码:</label><input type='password' name='password' id='password' required></form><div class=\"button-group\"><form action='/wifi_set' method='POST'><input type='submit' value='连接WiFi' class='btn-secondary'></form><form id='scanForm' action='/wifi_scan' method='GET'><input type='submit' value='扫描附近WiFi' class='btn-primary' id='scanBtn'></form></div><div id='loading' class='loading hidden'><div class='spinner'></div><p>扫描中，请稍候...</p></div></div>";
		html += "<script>document.getElementById('scanBtn').addEventListener('click',function(e){e.preventDefault();document.getElementById('loading').classList.remove('hidden');document.querySelector('.container').style.opacity='0.5';fetch('/wifi_scan',{method:'GET'}).then(response=>response.text()).then(data=>{document.body.innerHTML=data;}).catch(error=>{console.error('扫描失败:',error);document.getElementById('loading').classList.add('hidden');document.querySelector('.container').style.opacity='1';alert('扫描失败，请重试');});});document.querySelector('.btn-secondary').addEventListener('click',function(e){const ssid=document.getElementById('manualssid').value.trim();const password=document.getElementById('password').value.trim();if(!ssid){alert('请输入WiFi名称(SSID)');e.preventDefault();return;}if(!password){alert('请输入密码');e.preventDefault();return;}if(password.length<8){alert('密码至少8位');e.preventDefault();return;}});</script></body></html>";
        AP_server.send(200, "text/html; charset=utf-8", html);
	});

    // 扫描WiFi并展示列表
	AP_server.on("/wifi_scan", [](){
        int n = WiFi.scanNetworks();
        LOG_NETWORK_INFO("Find %d WiFi!" ,n);
        if (n == 0) {
                AP_server.send(500, "text/plain", "No WiFi networks found");
                return;
        }
        String html = "";
		html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>WiFi选择</title><style>:root{--blue:#0067b6;--blue-dark:#0045a4;--cyan:#77eedd;--cyan-dark:#55ccbb;--gray-dark:#6a7690;--gray:#6c757d;--gray-light:#f8f9fa;--gray-lighter:#343a40;--white:#fff;--border:#dee2e6;--shadow:0 4px 12px rgba(0,0,0,0.08);--border-radius:8px;}*{box-sizing:border-box;margin:0;padding:0;}body{background:linear-gradient(90deg,rgba(179,255,253,0.5) 0%,rgba(227,230,255,0.5) 50%,rgba(253,229,245,0.5) 100%);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:var(--gray-light);color:var(--gray-lighter);line-height:1.5;text-align:center;}.container{max-width:500px;margin:32px auto;padding:24px;background:rgba(255,255,255,0.6);backdrop-filter:blur(10px);-webkit-backdrop-filter:blur(10px);border-radius:25px;box-shadow:var(--shadow);}h1{color:var(--blue);font-weight:600;margin:0 0 18px 0;font-size:1.5em;}h3{color:var(--gray-dark);margin:18px 0 10px 0;}label{display:block;margin:0.8em 0 0.2em;color:var(--blue);font-weight:bold;font-size:1em;text-align:left;}input[type=password],input[type=text],select{width:90%;padding:10px;margin:10px 0;border:1px solid var(--border);border-radius:6px;font-size:16px;transition:border-color 0.2s,box-shadow 0.2s;background:linear-gradient(135deg,rgba(255,255,255,0.8),rgba(240,248,255,0.8));box-shadow:inset 0 2px 4px rgba(0,0,0,0.1);}input[type=password]:focus,input[type=text]:focus,select:focus{outline:none;border-color:var(--blue);box-shadow:0 0 0 3px rgba(0,123,255,0.15),inset 0 1px 2px rgba(0,0,0,0.1);transform:scale(1.02);}input[type=password]::placeholder,input[type=text]::placeholder{color:var(--gray);font-style:italic;opacity:0.7;}button,input[type=submit]{padding:12px 30px;margin:10px 5px;border:none;border-radius:var(--border-radius);cursor:pointer;font-size:16px;font-weight:500;transition:background-color 0.2s,transform 0.1s;color:var(--white);}.btn-primary,input[type=submit]{background-color:var(--blue);}.btn-primary:hover,input[type=submit]:hover{background-color:var(--blue-dark);}.btn-secondary{background-color:var(--cyan);color:var(--gray-lighter);}.btn-secondary:hover{background-color:var(--cyan-dark);}.info{margin:15px 0 22px 0;color:var(--gray);font-size:1.05em;}</style></head>";
		html += "<body><div class='container'><h1>WiFi配网</h1><div class='info'><p>请选择要连接的WiFi</p></div><form action='/wifi_set' method='POST' style='width:100%;max-width:340px;margin:0 auto;'><label for='ssid'>WiFi列表:</label><select name='ssid' id='ssid'>";
		for (int i = 0; i < n; ++i) {
			html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
		}
		html += "</select><label for='password'>密码:</label><input type='password' name='password' id='password' required><input type='submit' value='连接WiFi' class='btn-secondary'></form></div></body></html>";

        AP_server.send(200, "text/html; charset=utf-8", html);
    });

	AP_server.on("/wifi_set", [](){
		String ssid = AP_server.arg("ssid");
		String password = AP_server.arg("password");
		_saveWiFiCredentials(ssid, password);

        // 显示保存成功页面并重启
		String html = "";
		html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
		html += "<title>WiFi配置</title><style>:root{--blue:#0067b6;--blue-dark:#0045a4;--green:#388E3C;--gray-light:#f8f9fa;--gray-lighter:#343a40;--white:#fff;--shadow:0 4px 12px rgba(0,0,0,0.08);}body{background:linear-gradient(90deg,rgba(179,255,253,0.5) 0%,rgba(227,230,255,0.5) 50%,rgba(253,229,245,0.5) 100%);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background-color:var(--gray-light);color:var(--gray-lighter);min-height:100vh;margin:0;align-items:center;justify-content:center;}.container{max-width:500px;margin:32px auto;padding:24px;background:rgba(255,255,255,0.6);backdrop-filter:blur(10px);-webkit-backdrop-filter:blur(10px);border-radius:25px;box-shadow:var(--shadow);}h2{color:var(--blue);font-size:1.35em;margin-bottom:0.7em;font-weight:600;}p{color:var(--green);font-size:1.08em;animation:pulse 1.5s infinite;}@keyframes pulse{0%,100%{opacity:1;}50%{opacity:0.3;}}@media(max-width:800px){.container{width:80vw;}}</style></head>";
		html += "<body><div class='container'><h2>WiFi信息已保存</h2><p>设备正在重启，请稍候...</p></div></body></html>";

	    AP_server.send(200, "text/html; charset=utf-8", html);
		delay(500);
		ESP.restart();
	});

	// 常见的强制门户检测端点
	// Android 设备检测
	AP_server.on("/generate_204", [](){
		AP_server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		AP_server.send(302, "text/plain", "");
	});
	
	// iOS 设备检测
	AP_server.on("/hotspot-detect.html", [](){
		AP_server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		AP_server.send(302, "text/plain", "");
	});
	
	// Windows 设备检测
	AP_server.on("/ncsi.txt", [](){
		AP_server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		AP_server.send(302, "text/plain", "");
	});
	
	// 通用重定向端点
	AP_server.on("/redirect", [](){
		AP_server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		AP_server.send(302, "text/plain", "");
	});

	AP_server.begin();

	// 在屏幕上显示ip
	lcdText("Connect to AP",1);
	lcdText("IP:" + WiFi.softAPIP().toString(),2);
}

// WiFi连接后台任务
void wifiConnectTask(void* parameter) {
		LOG_WIFI_INFO("WiFi connection task started");
		
		// 等待一小段时间，确保网络栈完全初始化
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
		updateColor(CRGB::Blue);  // 连接中蓝灯
		
		WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
		
		unsigned long startTime = millis();
		int fadeStep = 2;
		uint8_t brightness = 64;
		
        // 连接中蓝灯闪烁
		while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
            brightness += fadeStep;

            if (brightness == 0 || brightness == 192) {
                    fadeStep = -fadeStep;
            }

            if((millis() - startTime) % 500 == 0){
                    LOG_WIFI_DEBUG(".");
            }
            updateBrightness(brightness);
            vTaskDelay(5 / portTICK_PERIOD_MS);
		}
		updateBrightness(128);
		

		if (WiFi.status() == WL_CONNECTED) {
				wifiConnectionState = WIFI_CONNECTED;
				updateColor(CRGB::Green);  	// 连接成功绿灯
				server.begin();				// 启动TCP服务器

				LOG_WIFI_INFO("connected: %s", savedSSID.c_str());
				LOG_WIFI_INFO("IP: %s", WiFi.localIP().toString().c_str());
				LOG_WIFI_INFO("TCP server started on port %d", CONNECT_PORT);

				// 联网成功后启动非阻塞时间同步
				LOG_WIFI_INFO("starting background time sync...");
				initTimeSync();
				updateTimeSync();
		} else {
				wifiConnectionState = WIFI_FAILED;
				WiFi.disconnect();
				LOG_WIFI_ERROR("can't connect to WiFi");

				updateColor(CRGB::Red);  // 失败变红
		}
		
		// 任务完成，删除任务自身
		vTaskDelay(pdMS_TO_TICKS(5));	//延迟5MS确保RGB灯状态更新
		vTaskDelete(NULL);
}

void connectToWiFi() {
		_loadWiFiCredentials();

		if (savedSSID == "") {
				LOG_WIFI_WARN("config not found, entering config mode");
				wifiConnectionState = WIFI_FAILED;
				// enterConfigMode();
				return;
		}

		LOG_WIFI_INFO("will connect to: %s", savedSSID.c_str());

		// 设置连接中状态
		wifiConnectionState = WIFI_CONNECTING;
		
		// 先设置WiFi模式，确保网络栈已初始化
		WiFi.mode(WIFI_STA);
		vTaskDelay(50 / portTICK_PERIOD_MS);  // 给WiFi栈一点时间初始化
		
		// 创建后台任务进行WiFi连接，不阻塞主线程
		xTaskCreate(wifiConnectTask, "WiFiConnectTask", 4096, NULL, 1, NULL);
}

// 初始化wifi
void wifiinit(){
	// 连接WiFi
	if (digitalRead(BUTTEN_CENTER_PIN)) {
			LOG_WIFI_INFO("Entering config mode by button");
			enterConfigMode();
	} else {
			connectToWiFi();
	}
}