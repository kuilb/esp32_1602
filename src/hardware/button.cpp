#include "./hardware/button.h"
#include "./services/sleep_manager.h"

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


void initButtonsPin(){
    setInputPullDown(BUTTON_SIDE_PIN );
    setInputPullDown(BUTTON_LEFT_PIN);
    setInputPullDown(BUTTON_RIGHT_PIN);
    setInputPullDown(BUTTON_CENTER_PIN);

    setInputPullUp(BUTTON_POWER_PIN);
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
        
        // 检测 GPIO0 的状态
        static bool readytoSleep = false;
        bool powerKeyState = digitalRead(BUTTON_POWER_PIN);
        if (!readytoSleep && powerKeyState == LOW) {  // 按下电源键
            LOG_BUTTON_DEBUG("Power key pressed");
            readytoSleep = true;
        }
        if(readytoSleep && powerKeyState == HIGH) {  // 松开电源键
            LOG_BUTTON_DEBUG("Power key released, entering deep sleep");
            enterDeepSleep();
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // 扫描频率10ms
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

        bool center = buttonJustPressed[CENTER];
        bool up     = buttonJustPressed[SIDE];
        bool down   = buttonJustPressed[LEFT];

        // 按键按下
        if (center && down) {
            inMenuMode = true;
            currentState = STATE_MENU;
            LOG_MENU_INFO("Combo triggered: Center + Down");
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

        vTaskDelay(pdMS_TO_TICKS(10));
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
        4096, NULL, 1, 
        &scanTaskHandle, 1);

    // 按键处理
    xTaskCreatePinnedToCore(handleButtonsTask, 
        "Button Handle", 4096, NULL, 1, 
        &handleTaskHandle, 1);
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