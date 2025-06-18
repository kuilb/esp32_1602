#include "button.h"

ButtonState buttons[] = {
    {BUTTEN_UP,     "UP",     LOW, 0, false},
    {BUTTEN_DOWN,   "DOWN",   LOW, 0, false},
    {BUTTEN_LEFT,   "LEFT",   LOW, 0, false},
    {BUTTEN_RIGHT,  "RIGHT",  LOW, 0, false},
    {BUTTEN_CENTER, "CENTER", LOW, 0, false},
};

const int buttonCount = sizeof(buttons) / sizeof(buttons[0]);

// 用于记录按钮是否“刚刚被按下”，由扫描线程更新，由处理线程读取
volatile bool buttonJustPressed[buttonCount] = { false };

// 发送按钮信息用的客户端信息
WiFiClient client;

// 扫描按键是否按下
void scanButtonsTask(void *pvParameters) {
    while (true) {
        unsigned long now = millis();

        for (int i = 0; i < buttonCount; ++i) {
            ButtonState& btn = buttons[i];
            bool currentState = digitalRead(btn.pin);

            if (currentState && !btn.lastState) {
                // 检测到刚刚按下
                btn.pressStartTime = now;
                buttonJustPressed[i] = false;
            }
            else if (currentState && !buttonJustPressed[i]) {
                // 按下维持，检查去抖
                if (now - btn.pressStartTime >= DEBOUNCE_TIME) {
                    buttonJustPressed[i] = true;
                }
            }
            else if (!currentState) {
                // 松开
                buttonJustPressed[i] = false;
            }

            btn.lastState = currentState;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// 处理按键事件
void handleButtonsTask(void *pvParameters) {
    while (true) {
        if (inMenuMode) {
            // 菜单模式下按键逻辑交由菜单处理函数处理
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        bool center = buttonJustPressed[4];
        bool up     = buttonJustPressed[0];

        // 按键按下
        if (center && up) {
            inMenuMode = true;
            currentState = STATE_MENU;
            Serial.println("[Menu] Combo triggered: Center + Up");
        } 

        // 若已连接则发送按键消息
        if (clientConnected && client.connected()) {
            for (int i = 0; i < buttonCount; ++i) {
                if (buttonJustPressed[i]) {
                    client.print("KEY:" + String(buttons[i].label) + "\n");
                    buttonJustPressed[i] = false;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


TaskHandle_t scanTaskHandle = NULL;
TaskHandle_t handleTaskHandle = NULL;
void startButtonTask() {
    xTaskCreatePinnedToCore(scanButtonsTask, "Button Scan", 2048, NULL, 1, &scanTaskHandle, 1);
    xTaskCreatePinnedToCore(handleButtonsTask, "Button Handle", 2048, NULL, 1, &handleTaskHandle, 1);
}