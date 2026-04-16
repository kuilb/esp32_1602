#include "./applications/setting.h"
#include "./menu/menu.h"
#include "./services/auto_brightness.h"
#include "./services/config_manager.h"
#include "./hardware/buzzer.h"
#include "./ui/hold_progress.h"

extern WifiConfigManager wifiConfigManager;

static void handleBrightnessInterface();
static void handleBatteryInfoInterface();
static void handleConnectInfoInterface();

namespace {
static const unsigned long TOGGLE_PROGRESS_START_MS = 300;
static const unsigned long TOGGLE_TRIGGER_MS = 1800;

struct BrightnessScreenState {
	bool active = false;
	int lastBrightness = -1;
	bool exitPending = false;
	unsigned long exitAtMs = 0;
};

struct BatteryInfoScreenState {
	bool active = false;
	unsigned long lastBatteryUpdate = 0;
};

struct ConnectInfoScreenState {
	bool active = false;
	unsigned long lastRefreshMs = 0;
};

static BrightnessScreenState s_brightnessScreen;
static BatteryInfoScreenState s_batteryInfoScreen;
static ConnectInfoScreenState s_connectInfoScreen;

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
				renderHoldProgressBar("Hold to toggle", 0);
			}

			if (overlayStarted) {
				const unsigned long progressElapsed = pressDurationMs - TOGGLE_PROGRESS_START_MS;
				const unsigned long progressWindow = TOGGLE_TRIGGER_MS - TOGGLE_PROGRESS_START_MS;
				const uint8_t percent = (progressWindow == 0)
					? 100
					: static_cast<uint8_t>(min(100UL, (progressElapsed * 100UL) / progressWindow));

				renderHoldProgressBar("Hold to toggle", percent);

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

static void _renderBrightnessScreen() {
	char buf[16];
	snprintf(buf, sizeof(buf), "%d%%", (brightness * 100 + 127) / 255);
	lcdText("Brightness:", 1);
	lcdText(buf, 2);
}

static void _renderBatteryInfoScreen() {
	if (!isfuelICConnected) {
		lcdText("Fuel gauge N/A", 1);
		lcdText("C:Back", 2);
		return;
	}

	readBatteryInfo();
	uint16_t voltage = readVoltage();
	int16_t current = readAverageCurrent();
	uint8_t soc = readStateOfCharge();
	uint16_t remainingCap = readRemainingCapacity();

	lcdText("" + String(voltage) + "mV " + String(current) + "mA", 1);
	lcdText(String(soc) + "% " + String(remainingCap) + "mAh", 2);
}

static void _renderConnectInfoScreen() {
	if (WiFi.status() == WL_CONNECTED) {
		lcdText("SSID:" + wifiConfigManager.getSSID(), 1);
		lcdText("IP:" + WiFi.localIP().toString(), 2);
	} else {
		lcdText("Not Connected", 1);
		lcdText("C:Back", 2);
	}
}

}

void enterBrightnessInterface() {
    // 开发注释：新增同类“参数页”时，复用 active/lastValue/exitPending 三段式状态即可。
    // 手动亮度调节界面：左右键改亮度，中键保存返回（非阻塞）。
	s_brightnessScreen = BrightnessScreenState{};
	s_brightnessScreen.active = true;
	s_brightnessScreen.lastBrightness = brightness;
	enterAppInterface(handleBrightnessInterface, false);
	globalButtonDelay(FIRST_TIME_DELAY);
	_renderBrightnessScreen();
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

void enterConnectInfoInterface(){
    // 开发注释：新增同类“信息页”时，复用 active + lastRefreshMs + appShouldRunPeriodic 模式。
    // 展示当前 WiFi 连接信息，按中键返回（非阻塞）。
	s_connectInfoScreen = ConnectInfoScreenState{};
	s_connectInfoScreen.active = true;
	s_connectInfoScreen.lastRefreshMs = millis() - 2000;
	enterAppInterface(handleConnectInfoInterface, true);
	globalButtonDelay(FIRST_TIME_DELAY);
	_renderConnectInfoScreen();
}

void enterBatteryInfoInterface() {
    // 开发注释：新增同类“周期采样页”时，仅替换刷新间隔与 render 函数即可。
    // 电池信息界面：周期刷新读数，按中键返回（非阻塞）。
	s_batteryInfoScreen = BatteryInfoScreenState{};
	s_batteryInfoScreen.active = true;
	s_batteryInfoScreen.lastBatteryUpdate = millis() - 10000;
	enterAppInterface(handleBatteryInfoInterface, false);
	globalButtonDelay(FIRST_TIME_DELAY);
	_renderBatteryInfoScreen();
}

void _rebootSystem(){
	// 显示重启提示并执行系统重启。
	lcdText("Rebooting...", 1);
	lcdText("", 2);
	LOG_SYSTEM_INFO("System rebooting...");
	delay(400);
	ESP.restart();
}

static void handleBrightnessInterface() {
	if (!s_brightnessScreen.active) {
		return;
	}

	const unsigned long nowMs = millis();
	if (s_brightnessScreen.exitPending) {
		if ((long)(nowMs - s_brightnessScreen.exitAtMs) >= 0) {
			s_brightnessScreen.active = false;
			exitAppInterface(FIRST_TIME_DELAY);
		}
		return;
	}

	if (isButtonReadyToRespond(LEFT, 10)) {
		buzzerPlayNavigateSound();
		changeBrightness(-1);
	}
	if (isButtonReadyToRespond(RIGHT, 10)) {
		buzzerPlayNavigateSound();
		changeBrightness(1);
	}

	if (brightness != s_brightnessScreen.lastBrightness) {
		s_brightnessScreen.lastBrightness = brightness;
		_renderBrightnessScreen();
	}

	if (isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)) {
		buzzerPlaySelectSound();
		lcdText("Brightness saved", 1);
		lcdText("Back to Menu", 2);
		LOG_SYSTEM_INFO("Brightness set to %d%%", (brightness * 100 + 127) / 255);
		s_brightnessScreen.exitPending = true;
		s_brightnessScreen.exitAtMs = nowMs + 500;
	}
}

static void handleBatteryInfoInterface() {
	if (!s_batteryInfoScreen.active) {
		return;
	}

	if (appHandleCenterExit()) {
		s_batteryInfoScreen.active = false;
		return;
	}

	if (appShouldRunPeriodic(s_batteryInfoScreen.lastBatteryUpdate, 10000U)) {
		_renderBatteryInfoScreen();
	}
}

static void handleConnectInfoInterface() {
	if (!s_connectInfoScreen.active) {
		return;
	}

	if (appHandleCenterExit()) {
		s_connectInfoScreen.active = false;
		return;
	}

	if (appShouldRunPeriodic(s_connectInfoScreen.lastRefreshMs, 2000U)) {
		_renderConnectInfoScreen();
	}
}
