#include "./menu/status_bar_renderer.h"
#include "./menu/menu.h"

static auto s_wifianim = Animations::getAnimation("wifi_searching");
static auto s_spinneranim = Animations::getAnimation("spinner");
static auto s_wronganim = Animations::getAnimation("wrong");

static uint8_t s_cachedBatterySOC = 0;
static int16_t s_cachedCurrentBattery_mA = 0;
static uint16_t s_cachedBatteryVoltage_mV = 0;
static unsigned long s_lastBatteryReadMs = 0;

static TimeSyncState s_lastTimeSyncState = TIME_SYNC_IDLE;
static WiFiConnectionState s_lastWiFiState = WIFI_IDLE;
static tm s_statusTimeInfo = {};
static unsigned long s_lastTimePollMs = 0;

static bool s_needsRedraw = true;
static bool s_spinnerEnabled = false;
static bool s_wifiSearchingEnabled = false;
static bool s_wrongEnabled = false;

static const unsigned long BATTERY_READ_INTERVAL_MS = 10000;
static const unsigned long ANIMATION_UPDATE_INTERVAL_MS = 50;
static unsigned long s_lastAnimationUpdateMs = 0;

static bool _shouldShowSpinner() {
    if (timeSyncState == TIME_SYNC_IN_PROGRESS && wifiConnectionState == WIFI_CONNECTED) {
        return true;
    }
    return wifiConnectionState == WIFI_CONNECTING
        || wifiConnectionState == WIFI_DISCONNECTED
        || wifiConnectionState == WIFI_IDLE;
}

static bool _shouldShowWifiSearching() {
    return wifiConnectionState == WIFI_CONNECTING;
}

static bool _shouldShowWrong() {
    return wifiConnectionState == WIFI_FAILED;
}

static void _syncAnimationEnableStates() {
    const bool spinnerNow = _shouldShowSpinner();
    const bool wifiSearchingNow = _shouldShowWifiSearching();
    const bool wrongNow = _shouldShowWrong();

    if (spinnerNow != s_spinnerEnabled) {
        s_spinnerEnabled = spinnerNow;
        Animations::setEnable("spinner", s_spinnerEnabled);
        s_needsRedraw = true;
    }
    if (wifiSearchingNow != s_wifiSearchingEnabled) {
        s_wifiSearchingEnabled = wifiSearchingNow;
        Animations::setEnable("wifi_searching", s_wifiSearchingEnabled);
        s_needsRedraw = true;
    }
    if (wrongNow != s_wrongEnabled) {
        s_wrongEnabled = wrongNow;
        Animations::setEnable("wrong", s_wrongEnabled);
        s_needsRedraw = true;
    }
}

static void _formatStatusBarTime(char* outBuf, size_t outBufLen) {
    if (!outBuf || outBufLen == 0) {
        return;
    }

    if (outBufLen < 7) {
        outBuf[0] = '\0';
        return;
    }

    char hmBuf[6] = {0};

    if (timeSyncState == TIME_SYNC_SUCCESS) {
        strftime(hmBuf, sizeof(hmBuf), "%H:%M", &localTimeInfo);
        snprintf(outBuf, outBufLen, " %s", hmBuf);
        return;
    }

    if (getRtcTime().tv_sec > 1765967312) { // 2025-12-17 18:40 GMT+8
        time_t rtcTime = getRtcTime().tv_sec;

        if (timeSyncState == TIME_SYNC_IDLE) {
            rtcTime += GMT_OFFSET_HOUR * 3600;
        }

        localtime_r(&rtcTime, &s_statusTimeInfo);
        strftime(hmBuf, sizeof(hmBuf), "%H:%M", &s_statusTimeInfo);
        snprintf(outBuf, outBufLen, " %s", hmBuf);
        return;
    }

    strncpy(outBuf, " --:--", outBufLen);
    outBuf[outBufLen - 1] = '\0';
}

void statusBarRendererInit() {
    Animations::registerAnimation(*s_wifianim);
    Animations::registerAnimation(*s_spinneranim);
    Animations::registerAnimation(*s_wronganim);

    Animations::setEnable("wifi_searching", false);
    Animations::setEnable("spinner", false);
    Animations::setEnable("wrong", false);

    s_cachedBatterySOC = readStateOfCharge();
    s_cachedCurrentBattery_mA = readAverageCurrent();
    s_cachedBatteryVoltage_mV = readVoltage();
    s_lastBatteryReadMs = millis();

    s_lastWiFiState = wifiConnectionState;
    s_lastTimeSyncState = timeSyncState;
    s_lastTimePollMs = millis();
    s_lastAnimationUpdateMs = millis();
    _syncAnimationEnableStates();
    s_needsRedraw = true;
}

void statusBarRendererTick() {
    const unsigned long nowMs = millis();

    _syncAnimationEnableStates();

    if (millis() - s_lastBatteryReadMs > BATTERY_READ_INTERVAL_MS) {
        s_cachedBatterySOC = readStateOfCharge();
        s_cachedCurrentBattery_mA = readAverageCurrent();
        s_cachedBatteryVoltage_mV = readVoltage();
        s_lastBatteryReadMs = nowMs;
        s_needsRedraw = true;
    }

    // 动画帧最小更新时间为 50ms，避免高频空轮询。
    if ((s_spinnerEnabled || s_wifiSearchingEnabled || s_wrongEnabled)
        && nowMs - s_lastAnimationUpdateMs >= ANIMATION_UPDATE_INTERVAL_MS) {
        Animations::update();
        s_lastAnimationUpdateMs = nowMs;
        if (Animations::getNeedToUpdate()) {
            s_needsRedraw = true;
        }
    }
}

bool statusBarRendererNeedsRedraw() {
    bool changed = false;

    if (s_lastWiFiState != wifiConnectionState) {
        s_lastWiFiState = wifiConnectionState;
        changed = true;
    }

    if (s_lastTimeSyncState != timeSyncState) {
        s_lastTimeSyncState = timeSyncState;
        changed = true;
    }

    const unsigned long nowMs = millis();
    if (timeSyncState == TIME_SYNC_SUCCESS && (nowMs - s_lastTimePollMs >= 1000)) {
        s_lastTimePollMs = nowMs;
        getLocalTime(&localTimeInfo);
        if (s_statusTimeInfo.tm_min != localTimeInfo.tm_min) {
            s_statusTimeInfo = localTimeInfo;
            changed = true;
        }
    }

    return changed || s_needsRedraw;
}

void renderStatusBar() {
    lcdSetCursor(0);
    lcdResetCharSlot();

    if (isfuelICConnected && s_cachedBatteryVoltage_mV < 4500) {
        lcdCreateCharAuto(SystemIcons::getBatteryLeftIcon(s_cachedBatterySOC));
        lcdCreateCharAuto(SystemIcons::getBatteryRightIcon(s_cachedBatterySOC));

        char tempBuf[5];
        char batteryBuf[5] = {0};
        snprintf(tempBuf, sizeof(tempBuf), "%d%%", s_cachedBatterySOC);

        if (s_cachedCurrentBattery_mA > 0 && s_cachedBatterySOC != 100) {
            lcdCreateCharAuto(SystemIcons::getIcon("battery_charging"));
            snprintf(batteryBuf, sizeof(batteryBuf), "%-3s", tempBuf);
        } else {
            snprintf(batteryBuf, sizeof(batteryBuf), "%-4s", tempBuf);
        }
        lcdPrint(batteryBuf);
    } else {
        lcdCreateCharAuto(SystemIcons::getIcon("dc_left"));
        lcdCreateCharAuto(SystemIcons::getIcon("dc_right"));
        lcdPrint("DC  ");
    }

    lcdPrint(" ");

    // 时间同步进行中时，优先显示“转圈 + WiFi”，便于用户感知正在校时。
    if (timeSyncState == TIME_SYNC_IN_PROGRESS && wifiConnectionState == WIFI_CONNECTED) {
        lcdCreateCharAuto(s_spinneranim->frames[s_spinneranim->currentFrame]);
        lcdCreateCharAuto(SystemIcons::getIcon("wifi"));
    } else switch (wifiConnectionState) {
        case WIFI_CONNECTING:
            lcdCreateCharAuto(s_spinneranim->frames[s_spinneranim->currentFrame]);
            lcdCreateCharAuto(s_wifianim->frames[s_wifianim->currentFrame]);
            break;
        case WIFI_CONNECTED:
            lcdPrint(" ");
            lcdCreateCharAuto(SystemIcons::getIcon("wifi"));
            break;
        case WIFI_DISCONNECTED:
            lcdCreateCharAuto(s_spinneranim->frames[s_spinneranim->currentFrame]);
            lcdCreateCharAuto(SystemIcons::getIcon("wifi"));
            break;
        case WIFI_FAILED:
            lcdCreateCharAuto(s_wronganim->frames[s_wronganim->currentFrame]);
            lcdCreateCharAuto(SystemIcons::getIcon("wifi"));
            break;
        case WIFI_IDLE:
            lcdPrint(" ");
            lcdCreateCharAuto(s_spinneranim->frames[s_spinneranim->currentFrame]);
            break;
        default:
            {
                static int lastUnknownWiFiState = -1;
                static unsigned long lastUnknownWiFiLogMs = 0;
                int stateVal = (int)wifiConnectionState;
                if (stateVal != lastUnknownWiFiState && millis() - lastUnknownWiFiLogMs > 1000) {
                    LOG_MENU_WARN("Unknown wifiConnectionState=%d", stateVal);
                    lastUnknownWiFiState = stateVal;
                    lastUnknownWiFiLogMs = millis();
                }
                lcdCreateCharAuto(s_wronganim->frames[s_wronganim->currentFrame]);
                lcdPrint(" ");
            }
            break;
    }

    lcdPrint(" ");

    char timeBuf[8] = {0};
    _formatStatusBarTime(timeBuf, sizeof(timeBuf));
    lcdPrint(timeBuf);

    s_needsRedraw = false;
}
