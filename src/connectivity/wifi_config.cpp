#include "./connectivity/wifi_config.h"

extern WifiConfigManager wifiConfigManager;

// 配网模式使用的 Web 服务器（监听端口 80）
WebServer apServer(80);

// DNS 服务器用于强制门户
DNSServer dnsServer;
const byte DNS_PORT = 53;

// 当前是否处于配网模式的标志位
bool inConfigMode = false;

// WiFi连接状态
WiFiConnectionState wifiConnectionState = WIFI_IDLE;

// 防止重复创建连接任务 / 时间同步任务导致资源耗尽和状态抖动
static TaskHandle_t wifiConnectTaskHandle = nullptr;
static TaskHandle_t timeSyncTaskHandle = nullptr;

// 扫描状态
WifiScanState wifiScanState = WIFI_SCAN_IDLE;
String scanResult = "";

// 保存WiFi信息
void _saveWiFiCredentials(const String& ssid, const String& password) {
	wifiConfigManager.setSSID(ssid);
	wifiConfigManager.setPassword(password);

	LOG_WIFI_INFO("WiFi config saved, restarting...");
	lcdText("Config Saved", 1);
	lcdText("Restarting...", 2);
}

void wifiConfigHandler(){
	// 计算总长度
    unsigned int len1 = strlen_P(webComponent);
    unsigned int len2 = strlen_P(wifiConfigHtml);
    unsigned int totalLen = len1 + len2;

    // 设置 Content-Length 并发送头（空 body）
    apServer.setContentLength(totalLen);
    apServer.send(200, "text/html; charset=utf-8", "");

    // 直接发送 PROGMEM 内容块
    apServer.sendContent_P(webComponent, len1);
    apServer.sendContent_P(wifiConfigHtml, len2);
}

void wifiScanhandler(){
	if (wifiScanState == WIFI_SCAN_IDLE) {
        wifiScanState = WIFI_SCAN_SCANNING;
        apServer.send(202, "application/json", "{\"status\":\"scanning\"}");
        
        // 创建任务进行WiFi扫描
        xTaskCreate([](void*){
            int n = WiFi.scanNetworks();
            LOG_NETWORK_INFO("Find %d WiFi!", n);
            
            scanResult = "{\"status\":\"done\",\"networks\":[";
            
            for (int i = 0; i < n; ++i) {
                scanResult += "{\"ssid\":\"" + WiFi.SSID(i) + 
							  "\",\"rssi\":" + String(WiFi.RSSI(i)) + 
							  ",\"secure\":" + ((WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "true" : "false") + 
							  "}";
                if (i != n - 1) {
                    scanResult += ",";
                }
            }
            
            scanResult += "]}";
            wifiScanState = WIFI_SCAN_DONE;
			LOG_NETWORK_DEBUG(scanResult.c_str());
            vTaskDelete(NULL);
        }, "ScanTask", 4096, NULL, 1, NULL);
    } 

	else {
        if (wifiScanState != WIFI_SCAN_DONE) {
			LOG_NETWORK_DEBUG("send HTTP 202 scanning");
            apServer.send(202, "application/json", "{\"status\":\"scanning\"}");
        } else if (scanResult != "") {
            apServer.send(200, "application/json", scanResult);
            scanResult = "";
        } else {
			LOG_NETWORK_DEBUG("send HTTP 500 (no results)");
            apServer.send(500, "application/json", "{\"error\":\"no result\"}");
			wifiScanState = WIFI_SCAN_IDLE;
        }
    }
}

void wifiSethandler(){
	String ssid = apServer.arg("ssid");
	String password = apServer.arg("password");
	LOG_NETWORK_INFO("access /wifi_set");
	LOG_NETWORK_INFO("ssid: %s", ssid.c_str());
	_saveWiFiCredentials(ssid, password);
	apServer.send(200, "application/json", "{\"success\":true}");
	delay(500);
	ESP.restart();
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
	apServer.onNotFound([](){
		apServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		apServer.send(302, "text/plain", "");
	});

    // 配网页面
	apServer.on("/", wifiConfigHandler);

    // 扫描WiFi并展示列表
	apServer.on("/wifi_scan", wifiScanhandler);

	// 处理WiFi信息提交
	apServer.on("/wifi_set", wifiSethandler);

	// 常见的强制门户检测端点
	// Android 设备检测
	apServer.on("/generate_204", [](){
		apServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		apServer.send(302, "text/plain", "");
	});
	
	// iOS 设备检测
	apServer.on("/hotspot-detect.html", [](){
		apServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		apServer.send(302, "text/plain", "");
	});
	
	// Windows 设备检测
	apServer.on("/ncsi.txt", [](){
		apServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		apServer.send(302, "text/plain", "");
	});
	
	// 通用重定向端点
	apServer.on("/redirect", [](){
		apServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
		apServer.send(302, "text/plain", "");
	});

	apServer.begin();

	// 在屏幕上显示ip
	lcdText("Connect to AP",1);
	lcdText("IP:" + WiFi.softAPIP().toString(),2);
}

// WiFi连接后台任务
void wifiConnectTask(void* parameter) {
	LOG_WIFI_DEBUG("WiFi connection task started");
	
	// 等待一小段时间，确保网络栈完全初始化
	vTaskDelay(100 / portTICK_PERIOD_MS);
	
	updateColor(CRGB::Blue);  // 连接中蓝灯

	// 避免底层自动重连导致“超时失败后仍在后台不断重试”，这里交由上层逻辑控制重连
	WiFi.persistent(false);
	WiFi.setAutoReconnect(false);

	// 连接期间关闭省电，避免连接抖动/状态不同步
	WiFi.setSleep(false);
	
	WiFi.begin(wifiConfigManager.getSSID().c_str(), wifiConfigManager.getPassword().c_str());
	
	unsigned long startTime = millis();
	int fadeStep = 2;
	uint8_t brightness = 64;
	
	// 连接中蓝灯闪烁
	uint32_t lastDotMs = 0;
	while (millis() - startTime < 15000 && !shouldExitTasks) {
		const IPAddress ipNow = WiFi.localIP();
		if (WiFi.status() == WL_CONNECTED || ipNow != IPAddress(0, 0, 0, 0)) {
			break;
		}

		brightness += fadeStep;

		if (brightness == 0 || brightness == 192) {
				fadeStep = -fadeStep;
		}

		const uint32_t nowMs = millis();
		if ((uint32_t)(nowMs - lastDotMs) >= 500) {
			lastDotMs = nowMs;
			LOG_WIFI_DEBUG(".");
		}
		updateBrightness(brightness);
		vTaskDelay(5 / portTICK_PERIOD_MS);
	}
	updateBrightness(128);
	
	// 检查是否因睡眠退出
	if (shouldExitTasks) {
		LOG_WIFI_INFO("WiFi connect task exiting due to sleep request");
		vTaskDelay(pdMS_TO_TICKS(5));
		vTaskDelete(NULL);
		return;
	}

	const IPAddress ipAfter = WiFi.localIP();
	if (WiFi.status() == WL_CONNECTED || ipAfter != IPAddress(0, 0, 0, 0)) {
		wifiConnectionState = WIFI_CONNECTED;
		LOG_WIFI_DEBUG("WiFi connected successfully");
		
		updateColor(CRGB::Green);  	// 连接成功绿灯
		updateBrightness(10);
		
		// 等待 DHCP 分配到有效 IP
		unsigned long ipWaitStart = millis();
		IPAddress ip = WiFi.localIP();
		while (ip == IPAddress(0, 0, 0, 0) && (millis() - ipWaitStart) < 5000 && !shouldExitTasks) {
			vTaskDelay(pdMS_TO_TICKS(50));
			ip = WiFi.localIP();
		}
		LOG_WIFI_DEBUG("STA IP after connect: %s", ip.toString().c_str());

		// 启动TCP服务器（放在 IP/DNS 完成后）
		LOG_WIFI_DEBUG("Starting TCP server...");
		server.begin();
		LOG_WIFI_INFO("TCP server started on port %d", CONNECT_PORT);

		LOG_WIFI_INFO("connected: %s", wifiConfigManager.getSSID().c_str());
		LOG_WIFI_INFO("IP: %s", WiFi.localIP().toString().c_str());
		LOG_WIFI_DEBUG("starting background time sync...");

		// 默认保持 WiFi 睡眠
		WiFi.setSleep(true);

		// 创建后台时间同步任务（initNtpTimeSync 移到后台任务中，避免阻塞）
		if (timeSyncTaskHandle == nullptr) {
			xTaskCreate(timeSyncTask, "TimeSyncTask", 4096, &timeSyncTaskHandle, 1, &timeSyncTaskHandle);
		} else {
			LOG_WIFI_DEBUG("TimeSyncTask already running, skip create.");
		}
	} else {
			wifiConnectionState = WIFI_FAILED;
			// 关闭射频，防止 WiFi 底层在后台继续自动尝试连接
			WiFi.disconnect(true);
			WiFi.mode(WIFI_OFF);
			LOG_WIFI_ERROR("can't connect to WiFi");

			updateColor(CRGB::Red);  // 失败变红
	}
	
	// 任务完成，删除任务自身
	vTaskDelay(pdMS_TO_TICKS(5));	//延迟5MS确保RGB灯状态更新
	wifiConnectTaskHandle = nullptr;
	vTaskDelete(NULL);
}

void connectToWiFi() {
		if(wifiConfigManager.getSSID() == ""){
			LOG_WIFI_WARN("config not found");
			wifiConnectionState = WIFI_FAILED;
			updateColor(CRGB::Red);  		// 无配置红灯
			return;
		}

		// 避免重复创建连接任务
		if (wifiConnectionState == WIFI_CONNECTING || wifiConnectTaskHandle != nullptr) {
			LOG_WIFI_DEBUG("WiFi connect already in progress, skip.");
			return;
		}

		LOG_WIFI_INFO("will connect to: %s", wifiConfigManager.getSSID());

		// 连接前做一次硬断开，清理底层状态/停止可能存在的后台重连
		WiFi.persistent(false);
		WiFi.setAutoReconnect(false);
		WiFi.disconnect(true);
		WiFi.mode(WIFI_OFF);
		vTaskDelay(pdMS_TO_TICKS(50));

		// 设置连接中状态
		wifiConnectionState = WIFI_CONNECTING;
		
		// 先设置WiFi模式，确保网络栈已初始化
		WiFi.mode(WIFI_STA);
		vTaskDelay(50 / portTICK_PERIOD_MS);  // 给WiFi栈一点时间初始化
		
		// 创建后台任务进行WiFi连接，不阻塞主线程
		xTaskCreate(wifiConnectTask, "WiFiConnectTask", 4096, NULL, 1, &wifiConnectTaskHandle);
}

// 初始化wifi
void wifiinit(){
	if (digitalRead(BUTTON_CENTER_PIN)) {
			LOG_WIFI_INFO("Entering config mode by button");
			enterConfigMode();
	} else {
			connectToWiFi();
	}
}