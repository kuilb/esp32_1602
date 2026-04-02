#include "./applications/setting.h"
#include "./menu/menu.h"
#include "./services/auto_brightness.h"
#include "./services/config_manager.h"

extern WifiConfigManager wifiConfigManager;

void _enterBrightnessScreen() {
	// 手动亮度调节界面：左右键改亮度，中键保存返回。
	lcdText("Brightness:", 1);
	char buf[16];
	snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
	lcdText(buf, 2);

	int lastBrightness = brightness;

	while (!isButtonReadyToRespond(CENTER)) {
		if (isButtonReadyToRespond(LEFT, 10)) {
			changeBrightness(-1);
		}
		if (isButtonReadyToRespond(RIGHT, 10)) {
			changeBrightness(1);
		}

		// 只有亮度变化才重绘文字
		if (brightness != lastBrightness) {
			snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
			lcdText("Brightness:", 1);
			lcdText(buf, 2);
			lastBrightness = brightness;
		}

		vTaskDelay(10 / portTICK_PERIOD_MS);  // 节流
	}

	lcdText("Brightness saved", 1);
	lcdText("Back to Menu", 2);
	LOG_SYSTEM_INFO("Brightness set to %d%%", (brightness * 100 + 127) / 255);
	delay(500);
}

void _toggleAutoBrightness() {
	// 切换自动亮度开关，并即时反馈当前状态。
	bool isEnabled = toggleAutoBrightness();
	if (!ConfigManager::saveAutoBrightnessEnabled(isEnabled)) {
		LOG_CONFIG_WARN("Failed to persist auto brightness state");
	}
	lcdText("Auto Brightness", 1);
	lcdText(isEnabled ? "ON" : "OFF", 2);
	LOG_SYSTEM_INFO("Auto brightness %s", isEnabled ? "enabled" : "disabled");
	delay(500);
}

void _resetWifi(){
	// 清除 WiFi 配置并重启，使设备重新进入配网流程。
	inMenuMode = false;
	SPIFFS.remove("/wifi.txt");
	lcdText("WiFi cleared", 1);
	lcdText("Rebooting...", 2);
	LOG_SYSTEM_INFO("WiFi config cleared, restarting...");
	delay(800);
	ESP.restart();
}

void _resetFuelGauge(){
	// 重置燃料计设计容量参数。
	setBQ27421DesignCapacity(BATTERY_DESIGN_CAPACITY_MAH);
	lcdText("Fuel gauge reset", 1);
	lcdText("Back to Menu", 2);
	LOG_SYSTEM_INFO("Fuel gauge design capacity set to %d mAh", BATTERY_DESIGN_CAPACITY_MAH);
	delay(500);
}

void _setupWebSetting(){
	// 启动 Web 设置服务。
	webSettingSetupWebServer();
	LOG_WEB_INFO("Web configured");
}

void _connectInfo(){
	// 展示当前 WiFi 连接信息，按中键返回菜单。
	if (WiFi.status() == WL_CONNECTED) {
		lcdText("SSID:" + wifiConfigManager.getSSID(), 1);
		lcdText("IP:" + WiFi.localIP().toString(), 2);
	}
	else {
		lcdText("Not Connected", 1);
		lcdText("", 2);
	}

	for(;;){
		if(isButtonReadyToRespond(CENTER)){
			currentState = STATE_MENU;
			return;
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void _enterBatteryInfoScreen() {
	// 电池信息界面：周期刷新读数，按中键返回。
	unsigned long lastBatteryUpdate = millis() - 10000; // 强制首次更新
	while(true){
		if(millis() - lastBatteryUpdate >= 10000 && isfuelICConnected){
			readBatteryInfo();
			uint16_t voltage = readVoltage();
			int16_t current = readAverageCurrent();
			uint8_t soc = readStateOfCharge();
			uint16_t remainingCap = readRemainingCapacity();

			lcdText("" + String(voltage) + "mV " + String(current) + "mA", 1);
			lcdText(String(soc) + "% " + String(remainingCap) + "mAh", 2);

			lastBatteryUpdate = millis();
		}
		if(isButtonReadyToRespond(CENTER)){
			currentState = STATE_MENU;
			return;
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void _rebootSystem(){
	// 显示重启提示并执行系统重启。
	lcdText("Rebooting...", 1);
	lcdText("", 2);
	LOG_SYSTEM_INFO("System rebooting...");
	delay(400);
	ESP.restart();
}
