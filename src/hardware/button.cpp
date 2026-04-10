#include "./hardware/button.h"
#include "./services/sleep_manager.h"
#include "./hardware/buzzer.h"

ButtonState buttons[] = {
    {BUTTON_SIDE_PIN ,  "SIDE",   false, 0},
    {BUTTON_LEFT_PIN,   "LEFT",   false, 0},
    {BUTTON_RIGHT_PIN,  "RIGHT",  false, 0},
    {BUTTON_CENTER_PIN, "CENTER", false, 0},
};

const int buttonCount = sizeof(buttons) / sizeof(buttons[0]);

// 用于记录按钮是否"刚刚被按下"，由扫描线程更新，由处理线程读取
volatile bool buttonJustPressed[buttonCount] = { false };
volatile bool currentButtonState[buttonCount] = { false };

// 全局按钮防抖变量
static unsigned long lastButtonResponseTime[buttonCount] = { 0 };
static unsigned long globalButtonDelayUntil = 0;
static const unsigned long POWER_KEY_PROGRESS_START_MS = 300;
static const unsigned long POWER_KEY_SLEEP_TRIGGER_MS = 1800;

volatile bool powerKeyOverlayActive = false;

// 休眠进度条字符：按 5x8 点阵从左到右填充 0~5 列。
static const uint8_t kSleepBarGlyphs[6][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 0/5
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}, // 1/5
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}, // 2/5
    {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C}, // 3/5
    {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E}, // 4/5
    {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}  // 5/5(黑块)
};

static void _prepareSleepBarGlyphs() {
    for (int slot = 0; slot <= 5; ++slot) {
        lcdCreateChar(slot, kSleepBarGlyphs[slot]);
    }
}

static void _renderSleepProgress(uint8_t percent) {
    if (percent > 100) percent = 100;

    lcdText("Hold to sleep   ", 1);

    _prepareSleepBarGlyphs();

    // 进度条铺满第2行16字符；按 5 像素字符宽 + 1 像素字间空隙计算总进度。
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
            lcdDisCustom(fillInCell); // 1~5，对应部分填充到满黑块
        }
    }
}


void initButtonsPin(){
    setInputPullDown(BUTTON_SIDE_PIN );
    setInputPullDown(BUTTON_LEFT_PIN);
    setInputPullDown(BUTTON_RIGHT_PIN);
    setInputPullDown(BUTTON_CENTER_PIN);

    setInputPullDown(BUTTON_POWER_PIN);
}

// 扫描按键是否按下
void scanButtonsTask(void *pvParameters) {
    while (!shouldExitTasks) {
        unsigned long now = millis();

        for (int i = 0; i < buttonCount; ++i) {
            ButtonState& btn = buttons[i];
            bool currentState = digitalRead(btn.pin);   // 读取的按钮当前状态

            if (currentState && btn.lastState == false) {
                // 检测到刚刚按下
                btn.pressStartTime = now;
                buttonJustPressed[i] = false;
            }
            else if (currentState && !buttonJustPressed[i]) {
                // 按下维持，检查去抖
                if (now - btn.pressStartTime >= DEBOUNCE_TIME) {
                    // LOG_BUTTON_DEBUG("Button %s pressed", btn.label);
                    buttonJustPressed[i] = true;
                }
            }
            // 松开
            else if (!currentState) {
                buttonJustPressed[i] = false;
            }

            btn.lastState = currentState;
        }
        
        // 电源键：
        // - 按住 >=300ms 显示休眠进度条覆盖层
        // - 持续按住直到进度满触发休眠
        // - 提前松开恢复之前界面
        // - <300ms 短按执行“返回上一级/主菜单”
        static bool powerKeyLastState = false;
        static bool powerKeySleepTriggered = false;
        static bool powerKeyOverlayStarted = false;
        static unsigned long powerKeyPressStartMs = 0;

        const bool powerKeyState = (digitalRead(BUTTON_POWER_PIN) == HIGH);
        if (powerKeyState && !powerKeyLastState) {
            powerKeyPressStartMs = now;
            powerKeySleepTriggered = false;
            powerKeyOverlayStarted = false;
            powerKeyOverlayActive = false;
            LOG_BUTTON_DEBUG("Power key pressed");
        }

        if (powerKeyState) {
            const unsigned long pressDurationMs = now - powerKeyPressStartMs;

            if (!powerKeyOverlayStarted && pressDurationMs >= POWER_KEY_PROGRESS_START_MS) {
                powerKeyOverlayStarted = true;
                powerKeyOverlayActive = true;
                lcdPushOverlayFrame();
                lcdClear();
                _renderSleepProgress(0);
                LOG_BUTTON_INFO("Power key hold: show sleep progress");
            }

            if (powerKeyOverlayStarted && !powerKeySleepTriggered) {
                unsigned long progressElapsed = pressDurationMs - POWER_KEY_PROGRESS_START_MS;
                unsigned long progressWindow = POWER_KEY_SLEEP_TRIGGER_MS - POWER_KEY_PROGRESS_START_MS;
                uint8_t percent = (progressWindow == 0)
                    ? 100
                    : static_cast<uint8_t>(min(100UL, (progressElapsed * 100UL) / progressWindow));

                _renderSleepProgress(percent);

                if (pressDurationMs >= POWER_KEY_SLEEP_TRIGGER_MS) {
                    powerKeySleepTriggered = true;
                    powerKeyOverlayActive = false;
                    LOG_BUTTON_INFO("Power key hold complete, entering deep sleep");
                    buzzerPlaySleepSound();
                    vTaskDelay(pdMS_TO_TICKS(120));
                    enterDeepSleep();
                }
            }
        }

        if (!powerKeyState && powerKeyLastState) {
            const unsigned long pressDurationMs = now - powerKeyPressStartMs;

            if (powerKeyOverlayStarted && !powerKeySleepTriggered) {
                lcdPopOverlayFrame();
                powerKeyOverlayActive = false;
                LOG_BUTTON_INFO("Power key released before sleep, restore previous UI");
            } else if (!powerKeyOverlayStarted && pressDurationMs >= DEBOUNCE_TIME) {
                LOG_BUTTON_INFO("Power key short press detected, back action");
                menuHandleBackAction();
            }

            powerKeySleepTriggered = false;
            powerKeyOverlayStarted = false;
            powerKeyOverlayActive = false;
        }

        powerKeyLastState = powerKeyState;

        vTaskDelay(pdMS_TO_TICKS(20));  // 扫描频率20ms，降低空闲功耗
    }
    
    // 任务退出，不清理句柄（将在深度睡眠后自然清除）
    LOG_BUTTON_DEBUG("scanButtonsTask exiting...");
    vTaskDelete(NULL);
}


// 处理按键事件
void handleButtonsTask(void *pvParameters) {
    while (!shouldExitTasks) {

        // 菜单模式下按键逻辑交由菜单处理函数处理
        if (inMenuMode) {
            vTaskDelay(pdMS_TO_TICKS(100));     // 节流
            continue;
        }

        // 若已连接则发送按键消息（加锁，避免与 loopTask 的 read()/stop() 并发）
        if (clientMutex != nullptr && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (clientConnected && client.connected()) {
                for (int i = 0; i < buttonCount; ++i) {
                    if (buttonJustPressed[i] && !currentButtonState[i]) {
                        client.print("KEY_PRESS:" + String(buttons[i].label) + "\n");
                        currentButtonState[i] = true;

                        LOG_BUTTON_DEBUG("Send key pressed message: %s", buttons[i].label);
                    }
                    if(buttonJustPressed[i] == false && currentButtonState[i]) {
                        client.print("KEY_STOP:" + String(buttons[i].label) + "\n");
                        currentButtonState[i] = false;

                        LOG_BUTTON_DEBUG("Send key stop message: %s", buttons[i].label);
                    }
                }
            }
            xSemaphoreGive(clientMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    // 任务退出清理
    LOG_BUTTON_DEBUG("handleButtonsTask exiting...");
    handleTaskHandle = NULL;
    vTaskDelete(NULL);
}


TaskHandle_t scanTaskHandle = NULL;
TaskHandle_t handleTaskHandle = NULL;
void startButtonTask() {
    // 按键扫描
    xTaskCreatePinnedToCore(scanButtonsTask, 
        "Button Scan", 
        6144, NULL, 1, 
        &scanTaskHandle, 0);

    // 按键处理
    xTaskCreatePinnedToCore(handleButtonsTask, 
        "Button Handle", 6144, NULL, 1, 
        &handleTaskHandle, 0);
}

// 全局按钮防抖
bool isButtonReadyToRespond(int buttonIndex, unsigned long minInterval) {
    if (buttonIndex < 0 || buttonIndex >= buttonCount) {
        return false;
    }
    
    unsigned long currentTime = millis();
    
    // 检查全局延迟
    if (currentTime < globalButtonDelayUntil) {
        return false;
    }
    
    // 检查按钮是否真的被按下
    if (!buttonJustPressed[buttonIndex]) {
        return false;
    }
    
    // 检查按钮间隔防抖
    if (currentTime - lastButtonResponseTime[buttonIndex] < minInterval) {
        return false;
    }
    
    // 更新最后响应时间
    lastButtonResponseTime[buttonIndex] = currentTime;
    
    return true;
}

// 重置按钮防抖计时器
void resetButtonDebounce() {
    unsigned long currentTime = millis();
    for (int i = 0; i < buttonCount; i++) {
        lastButtonResponseTime[i] = currentTime;
        buttonJustPressed[i] = false;  // 清除所有按钮状态
    }
}

// 全局按钮防抖等待
void globalButtonDelay(unsigned long delayMs) {
    globalButtonDelayUntil = millis() + delayMs;
    resetButtonDebounce();
}