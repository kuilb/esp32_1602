#include "./applications/setting.h"
#include "./menu/menu.h"
#include "./services/auto_brightness.h"
#include "./services/config_manager.h"
#include "./hardware/buzzer.h"

extern WifiConfigManager wifiConfigManager;

namespace {
static const unsigned long TOGGLE_PROGRESS_START_MS = 300;
static const unsigned long TOGGLE_TRIGGER_MS = 1800;

static const uint8_t kToggleBarGlyphs[6][8] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10},
	{0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
	{0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C},
	{0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E},
	{0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}
};

static void _prepareToggleBarGlyphs() {
	for (int slot = 0; slot <= 5; ++slot) {
		lcdCreateChar(slot, kToggleBarGlyphs[slot]);
	}
}

static void _renderToggleProgress(uint8_t percent) {
	if (percent > 100) percent = 100;

	lcdText("Hold to toggle  ", 1);
	_prepareToggleBarGlyphs();

	const int barSlots = 16;
	const int cellCols = 5;
	const int gapCols = 1;
	const int totalVirtualCols = barSlots * cellCols + (barSlots - 1) * gapCols;
	const int filledVirtualCols = (percent * totalVirtualCols) / 100;

	lcdSetCursor(16);
	for (int slot = 0; slot < barSlots; ++slot) {
		const int cellStart = slot * (cellCols + gapCols);
		int fillInCell = filledVirtualCols - cellStart;
		if (fillInCell < 0) fillInCell = 0;
		if (fillInCell > cellCols) fillInCell = cellCols;

		if (fillInCell == 0) {
			lcdDisChar(' ');
		} else {
			lcdDisCustom(fillInCell);
		}
	}
}

static void _showTogglePrompt(const char* itemName, bool enabled) {
	char line1[17];
	snprintf(line1, sizeof(line1), "%s:%s", itemName, enabled ? "ON" : "OFF");
	lcdText(line1, 1);
	lcdText("Long Press C", 2);
}

static bool _waitToggleConfirm(const char* itemName, bool currentEnabled) {
	while (digitalRead(BUTTON_CENTER_PIN) == HIGH) {
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	_showTogglePrompt(itemName, currentEnabled);

	bool centerLastState = false;
	bool overlayStarted = false;
	unsigned long pressStartMs = 0;

	for (;;) {
		const unsigned long now = millis();
		const bool centerPressed = (digitalRead(BUTTON_CENTER_PIN) == HIGH);

		if (centerPressed && !centerLastState) {
			pressStartMs = now;
			overlayStarted = false;
		}

		if (centerPressed) {
			const unsigned long pressDurationMs = now - pressStartMs;

			if (!overlayStarted && pressDurationMs >= TOGGLE_PROGRESS_START_MS) {
				overlayStarted = true;
				lcdPushOverlayFrame();
				lcdClear();
				_renderToggleProgress(0);
			}

			if (overlayStarted) {
				const unsigned long progressElapsed = pressDurationMs - TOGGLE_PROGRESS_START_MS;
				const unsigned long progressWindow = TOGGLE_TRIGGER_MS - TOGGLE_PROGRESS_START_MS;
				const uint8_t percent = (progressWindow == 0)
					? 100
					: static_cast<uint8_t>(min(100UL, (progressElapsed * 100UL) / progressWindow));

				_renderToggleProgress(percent);

				if (pressDurationMs >= TOGGLE_TRIGGER_MS) {
					lcdPopOverlayFrame();
					return true;
				}
			}
		}

		if (!centerPressed && centerLastState) {
			if (overlayStarted) {
				lcdPopOverlayFrame();
			}
			buzzerPlayBackSound();
			return false;
		}

		centerLastState = centerPressed;
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}
}

void _enterBrightnessScreen() {
	// 手动亮度调节界面：左右键改亮度，中键保存返回。
	lcdText("Brightness:", 1);
	char buf[16];
	snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
	lcdText(buf, 2);

	int lastBrightness = brightness;

	while (!isButtonReadyToRespond(CENTER)) {
		if (isButtonReadyToRespond(LEFT, 10)) {
			buzzerPlayNavigateSound();
			changeBrightness(-1);
		}
		if (isButtonReadyToRespond(RIGHT, 10)) {
			buzzerPlayNavigateSound();
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
	buzzerPlaySelectSound();

	lcdText("Brightness saved", 1);
	lcdText("Back to Menu", 2);
	LOG_SYSTEM_INFO("Brightness set to %d%%", (brightness * 100 + 127) / 255);
	delay(500);
}

void _toggleAutoBrightness() {
	const bool currentEnabled = isAutoBrightnessActive();
	if (!_waitToggleConfirm("AutoBright", currentEnabled)) {
		return;
	}

	const bool isEnabled = toggleAutoBrightness();
	if (!ConfigManager::saveAutoBrightnessEnabled(isEnabled)) {
		LOG_CONFIG_WARN("Failed to persist auto brightness state");
	}
	buzzerPlaySelectSound();
	_showTogglePrompt("AutoBright", isEnabled);
	LOG_SYSTEM_INFO("Auto brightness %s", isEnabled ? "enabled" : "disabled");
	delay(500);
}

void _toggleSoundEffects() {
	const bool currentEnabled = buzzerIsUiSoundEnabled();
	if (!_waitToggleConfirm("SoundFX", currentEnabled)) {
		return;
	}

	const bool enabled = !currentEnabled;
	buzzerSetUiSoundEnabled(enabled);
	if (!ConfigManager::saveSoundEffectsEnabled(enabled)) {
		LOG_CONFIG_WARN("Failed to persist sound effects state");
	}
	if (enabled) {
		buzzerPlaySelectSound();
	}
	_showTogglePrompt("SoundFX", enabled);
	LOG_SYSTEM_INFO("Sound effects %s", enabled ? "enabled" : "disabled");
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
