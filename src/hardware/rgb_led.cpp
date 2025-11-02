#include "rgb_led.h"

// RGB灯用
CRGB leds[1];
CRGB currentColor;                  // 当前颜色
uint8_t currentBrightness = 128;    // 默认亮度一半
SemaphoreHandle_t ledMutex;         // 控制颜色访问的互斥锁

// 设置颜色
void updateColor(CRGB newColor) {
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    currentColor = newColor;
    xSemaphoreGive(ledMutex);
  }
  LOG_RGB_DEBUG("Updated color to R:" + String(newColor.r) + " G:" + String(newColor.g) + " B:" + String(newColor.b));
}

// 设置亮度
void updateBrightness(uint8_t newBrightness) {
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    currentBrightness = newBrightness;
    xSemaphoreGive(ledMutex);
  }
}

// 控制LED灯的任务
void rgbTask(void* pvParameters) {
  while (1) {
    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
      FastLED.setBrightness(currentBrightness);
      leds[0] = currentColor;
      FastLED.show();
      xSemaphoreGive(ledMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(50));  // 50ms 刷新一次
  }
}

// 初始化RGB
void initrgb(){
  FastLED.addLeds<SK6812, RGB_PIN, GRB>(leds, 1);

  // 初始化互斥锁
  ledMutex = xSemaphoreCreateMutex();
  if (ledMutex == NULL) {
    LOG_RGB_ERROR("RGB LED cannot create mutex!");
    while (1){
      updateColor(CRGB::Red);  // 失败变红
          int fadeStep = 2;
          uint8_t brightness = 64;
          while (1){
              brightness += fadeStep;

              if (brightness == 0 || brightness == 192) {
                  fadeStep = -fadeStep;
              }
              updateBrightness(brightness);
              delay(5);
          };
    };

    LOG_RGB_INFO("RGB LED initialized successfully.");
  }

  // 初始化颜色
  updateColor(CRGB::Blue);
  updateBrightness(128);

  // RGB任务
  xTaskCreatePinnedToCore(rgbTask, "RGB Task", 2048, NULL, 1, NULL, 1);
}