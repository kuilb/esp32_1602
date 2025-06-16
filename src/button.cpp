#include "button.h"

ButtonState buttons[] = {
    {BOTTEN_UP,     "UP",     LOW, 0, false},
    {BOTTEN_DOWN,   "DOWN",   LOW, 0, false},
    {BOTTEN_LEFT,   "LEFT",   LOW, 0, false},
    {BOTTEN_RIGHT,  "RIGHT",  LOW, 0, false},
    {BOTTEN_CENTER, "CENTER", LOW, 0, false},
};

const int buttonCount = sizeof(buttons) / sizeof(buttons[0]);

// 发送按钮信息用的客户端信息
WiFiClient client;

// 按键处理线程
void buttonTask(void *pvParameters) {
    while (true) {
        if (clientConnected && client.connected()) {
            for (int i = 0; i < buttonCount; ++i) {
                ButtonState& btn = buttons[i];
                bool currentState = digitalRead(btn.pin);

                if (currentState == HIGH && btn.lastState == LOW) {
                    // 按钮刚刚被按下
                    btn.pressStartTime = millis();
                    btn.triggered = false;
                } else if (currentState == HIGH && !btn.triggered) {
                    // 按下保持中
                    if (millis() - btn.pressStartTime >= DEBOUNCE_TIME) {
                        Serial.println(btn.label);
                        //client.write(String("KEY:") + btn.label + "\n");
                        client.print(String("KEY:") + btn.label + "\n");
                        btn.triggered = true;
                    }
                } else if (currentState == LOW) {
                    // 按钮松开
                    btn.triggered = false;
                }

                btn.lastState = currentState;
            }
        }
        delay(10);  // 检查频率
    }
}

TaskHandle_t buttonTaskHandle = NULL;  // 任务句柄
void startButtonTask(){
    xTaskCreatePinnedToCore(
        buttonTask, "Button Task",
        2048, NULL, 1,
        &buttonTaskHandle, 1
    );
}