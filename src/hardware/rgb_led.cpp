#include "rgb_led.h"

// RGB灯用
CRGB leds[1];
CRGB currentColor;                  // 当前颜色
uint8_t currentBrightness = 128;    // 默认亮度一半

// 设置颜色
void updateColor(CRGB newColor) {
  currentColor = newColor;
  LOG_RGB_DEBUG("Updated color to R:" + String(newColor.r) + " G:" + String(newColor.g) + " B:" + String(newColor.b));
}

// 设置亮度
void updateBrightness(uint8_t newBrightness) {
  currentBrightness = newBrightness;
}

// 控制LED灯的任务
void _rgbTask(void* pvParameters) {
  CRGB lastColor = currentColor;
  uint8_t lastBrightness = currentBrightness;

  while (1) {
    if (currentColor != lastColor || currentBrightness != lastBrightness) {
      FastLED.setBrightness(currentBrightness);
      leds[0] = currentColor;
      FastLED.show();
      lastColor = currentColor;
      lastBrightness = currentBrightness;
    }
    vTaskDelay(pdMS_TO_TICKS(10));  // 10ms 检查一次
  }
}

// 初始化RGB
void initrgb(){
  FastLED.addLeds<SK6812, RGB_PIN, GRB>(leds, 1);

  // 初始化颜色
  updateColor(CRGB::Blue);
  updateBrightness(128);

  // RGB任务
  xTaskCreatePinnedToCore(_rgbTask, "RGB Task", 2048, NULL, 1, NULL, 1);

  LOG_RGB_INFO("RGB LED initialized successfully.");
}