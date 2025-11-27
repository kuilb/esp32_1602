#include "wifi_config.h"
#include <lwip/dns.h>

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
                scanResult += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + ",\"secure\":" + ((WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "true" : "false") + "}";
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
			LOG_NETWORK_DEBUG("send HTTP 500 scanning (no results)");
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
		LOG_WIFI_INFO("WiFi connection task started");
		
		// 等待一小段时间，确保网络栈完全初始化
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
		updateColor(CRGB::Blue);  // 连接中蓝灯
		
		WiFi.begin(wifiConfigManager.getSSID().c_str(), wifiConfigManager.getPassword().c_str());
		
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

				LOG_WIFI_INFO("connected: %s", wifiConfigManager.getSSID().c_str());
				LOG_WIFI_INFO("IP: %s", WiFi.localIP().toString().c_str());
				LOG_WIFI_INFO("TCP server started on port %d", CONNECT_PORT);

				// 联网成功后启动非阻塞时间同步
				LOG_WIFI_INFO("starting background time sync...");
				initTimeSync();
				
				// 启用WiFi Modem Sleep以降低功耗
				WiFi.setSleep(true);
				LOG_WIFI_INFO("WiFi Modem Sleep enabled for power saving");
				
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
		if(wifiConfigManager.getSSID() == ""){
			LOG_WIFI_WARN("config not found");
			wifiConnectionState = WIFI_FAILED;
			updateColor(CRGB::Red);  		// 无配置红灯
			return;
		}

		LOG_WIFI_INFO("will connect to: %s", wifiConfigManager.getSSID());

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
	if (digitalRead(BUTTEN_CENTER_PIN)) {
			LOG_WIFI_INFO("Entering config mode by button");
			enterConfigMode();
	} else {
			connectToWiFi();
	}
}