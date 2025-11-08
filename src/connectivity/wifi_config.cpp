#include "wifi_config.h"
#include <lwip/dns.h>

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
		String html = getWebComponent();
		html += R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>WiFi 配置</title>
</head>
<style>
    
</style>
<body>
    <web-styles></web-styles>
    <div class='container'>
        <h1>WiFi 配网</h1>
        <form action='/wifi_set' method='POST' style='width:100%;max-width:340px;' id='wifiForm'>
            <label for='manualssid'>WiFi名称(SSID):</label>
            <input type='text' name='ssid' id='manualssid' placeholder='请输入SSID' required>
            <label for='password'>密码:</label>
            <input type='password' name='password' id='password' required>
            <div class='button-group'>
                <input type='submit' value='连接WiFi' class='btn-blue' id='connectBtn'>
                <button type='button' class='btn-cyan' id='scanBtn'>扫描附近WiFi</button>
            </div>
        </form>
        <div id='loading' class='loading hidden'>
            <div class='spinner'></div>
            <p>扫描中，请稍候...</p>
        </div>
    </div>
    <script>
        document.getElementById('scanBtn').addEventListener('click', function(e) {
            document.getElementById('loading').classList.remove('hidden');
            document.querySelector('.container').style.opacity = '0.5';
            fetch('/wifi_scan')
            .then(response => response.text())
            .then(data => {
                document.body.innerHTML = data;
            })
            .catch(error => {
                console.error('扫描失败:', error);
                document.getElementById('loading').classList.add('hidden');
                document.querySelector('.container').style.opacity = '1';
                alert('扫描失败,请重试');
            });
        });

        // 检查连接WiFi输入
        document.getElementById('wifiForm').addEventListener('submit', function(e) {
            const ssid = document.getElementById('manualssid').value.trim();
            const password = document.getElementById('password').value.trim();
            if (!ssid) {
                alert('请输入WiFi名称(SSID)');
                e.preventDefault();
                return;
            }
            if (!password) {
                alert('请输入密码');
                e.preventDefault();
                return;
            }
            if (password.length < 8) {
                alert('密码至少8位');
                e.preventDefault();
                return;
            }
        });
    </script>
</body>
</html>
		)";

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
        String html = getWebComponent();
		html += R"(
<!DOCTYPE html>
<html>

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi选择</title> 
</head>

<body>
    <web-styles></web-styles>
    <div class="container">
        <h1>WiFi配网</h1>
        <form action="/wifi_set" method="POST" style="width:100%;max-width:340px;margin:0 auto;" id="wifiForm">
            <label for="ssid">WiFi列表:</label><select name="ssid" id="ssid">
		)";
		for (int i = 0; i < n; ++i) {
			html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
		}
		html += R"(
            </select><label for="password">密码:</label>
            <input type="password" name="password" id="password" required>
            <br><br>
            <input type="submit" value="连接WiFi" class="btn-blue">
        </form>
    </div>
</body>
<script>
    document.getElementById("wifiForm").addEventListener("submit", function(e) {
        const password = document.getElementById("password").value.trim();
        if (!password) {
            alert("请输入密码");
            e.preventDefault();
            return;
        }
        if (password.length < 8) {
            alert("密码至少8位");
            e.preventDefault();
            return;
        }
    });
</script>
</html>
		)";

        AP_server.send(200, "text/html; charset=utf-8", html);
    });

	AP_server.on("/wifi_set", [](){
		String ssid = AP_server.arg("ssid");
		String password = AP_server.arg("password");
		_saveWiFiCredentials(ssid, password);

        // 显示保存成功页面并重启
		String html = getWebComponent();
		html += R"(
<!DOCTYPE html>
<html>

<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>WiFi配置</title>
    <style>
        p {
            color: var(--cyan);
            font-size: 1.08em;
            animation: pulse 1.5s infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.3; }
        }
    </style>
</head>

<body>
    <web-styles></web-styles>
    <div class="container">
        <h1>WiFi信息已保存</h1>
        <p>设备正在重启，请稍候...</p>
    </div>
</body>
</html>
	)";

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

				// 设置DNS服务器为Google Public DNS
				IPAddress dns1(223, 5, 5, 5);
				IPAddress dns2(1, 1, 1, 1);
				WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), dns1, dns2);
				LOG_WIFI_INFO("DNS set to 223.5.5.5 and 1.1.1.1");

				LOG_WIFI_INFO("connected: %s", savedSSID.c_str());
				LOG_WIFI_INFO("IP: %s", WiFi.localIP().toString().c_str());
				LOG_WIFI_INFO("TCP server started on port %d", CONNECT_PORT);

				// 联网成功后启动非阻塞时间同步
				LOG_WIFI_INFO("starting background time sync...");
				initTimeSync();
				
				// 创建后台时间同步任务
				xTaskCreate(timeSyncTask, "TimeSyncTask", 4096, NULL, 1, NULL);
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