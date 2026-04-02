#include "./services/auto_brightness.h"

#include "./menu/menu.h"
#include "./connectivity/network.h"

// ==================== 全局变量定义 ====================
bool isAutoBrightnessEnabled = true;
float smoothedLux = 0.0;
uint8_t targetBrightness = 128;
uint8_t lastAppliedBrightness = 0;

// 任务句柄
static TaskHandle_t autoBrightnessTaskHandle = nullptr;

// 当前使用的映射表
static const LuxBrightnessPoint* currentLuxMap = DEFAULT_LUX_MAP;
static uint8_t currentMapSize = LUX_MAP_SIZE;

// ==================== 内部辅助函数 ====================

/**
 * @brief 线性插值计算亮度
 * @param lux 当前光照强度
 * @return 插值计算得到的亮度值
 */
static uint8_t _interpolateBrightness(float lux) {
    // 如果低于最小值，返回最小亮度
    if (lux <= currentLuxMap[0].lux) {
        return currentLuxMap[0].brightness;
    }
    
    // 如果高于最大值，返回最大亮度
    if (lux >= currentLuxMap[currentMapSize - 1].lux) {
        return currentLuxMap[currentMapSize - 1].brightness;
    }
    
    // 在映射表中查找合适的区间进行线性插值
    for (uint8_t i = 0; i < currentMapSize - 1; i++) {
        if (lux >= currentLuxMap[i].lux && lux <= currentLuxMap[i + 1].lux) {
            // 线性插值公式: y = y1 + (x - x1) * (y2 - y1) / (x2 - x1)
            float luxRange = currentLuxMap[i + 1].lux - currentLuxMap[i].lux;
            float brightnessRange = currentLuxMap[i + 1].brightness - currentLuxMap[i].brightness;
            float luxOffset = lux - currentLuxMap[i].lux;
            
            uint8_t brightness = currentLuxMap[i].brightness + 
                                (uint8_t)((luxOffset / luxRange) * brightnessRange);
            
            return brightness;
        }
    }
    
    // 默认返回中等亮度（理论上不会到这里）
    return 128;
}

/**
 * @brief 平滑光照强度值
 * @param newLux 新读取的光照强度
 * @return 平滑后的光照强度
 */
static float _smoothLux(float newLux) {
    if (smoothedLux == 0.0) {
        // 首次读取，直接使用
        smoothedLux = newLux;
    } else {
        // 指数移动平均: smoothed = alpha * new + (1-alpha) * old
        smoothedLux = LUX_SMOOTHING_FACTOR * newLux + 
                     (1.0 - LUX_SMOOTHING_FACTOR) * smoothedLux;
    }
    return smoothedLux;
}

// ==================== 公共函数实现 ====================

void initAutoBrightness() {
    LOG_DISPLAY_INFO("Initializing auto brightness control...");
    
    // 确保光传感器已初始化
    if (!isOPT3001Connected) {
        LOG_DISPLAY_WARN("OPT3001 not connected, auto brightness disabled");
        isAutoBrightnessEnabled = false;
        return;
    }
    
    // 读取初始光照强度
    float initialLux = readLux();
    smoothedLux = initialLux;
    
    // 计算初始亮度
    targetBrightness = calculateBrightness(initialLux);
    lastAppliedBrightness = targetBrightness;
    
    // 应用初始亮度
    setLcdBrightness(targetBrightness);
    
    isAutoBrightnessEnabled = true;
    
    LOG_DISPLAY_INFO("Auto brightness initialized: %.2f lux -> %d/255", 
                     initialLux, targetBrightness);
}

void enableAutoBrightness() {
    if (!isOPT3001Connected) {
        LOG_DISPLAY_WARN("Cannot enable auto brightness: OPT3001 not connected");
        return;
    }
    
    isAutoBrightnessEnabled = true;
    LOG_DISPLAY_INFO("Auto brightness enabled");
}

void disableAutoBrightness() {
    isAutoBrightnessEnabled = false;
    LOG_DISPLAY_INFO("Auto brightness disabled");
}

bool toggleAutoBrightness() {
    if (isAutoBrightnessEnabled) {
        disableAutoBrightness();
    } else {
        enableAutoBrightness();
    }
    return isAutoBrightnessEnabled;
}

bool isAutoBrightnessActive() {
    return isAutoBrightnessEnabled && isOPT3001Connected;
}

uint8_t calculateBrightness(float lux) {
    uint8_t brightness = _interpolateBrightness(lux);
    
    // 应用亮度限制
    if (brightness < MIN_BRIGHTNESS) brightness = MIN_BRIGHTNESS;
    if (brightness > MAX_BRIGHTNESS) brightness = MAX_BRIGHTNESS;
    
    return brightness;
}

bool updateAutoBrightness() {
    // 检查是否启用自动调节
    if (!isAutoBrightnessActive()) {
        return false;
    }
    
    // 读取当前光照强度
    float currentLux = readLux();
    if (currentLux < 0.0) {
        LOG_DISPLAY_WARN("Failed to read lux value");
        return false;
    }
    
    // 平滑光照强度值
    float smoothed = _smoothLux(currentLux);
    
    // 计算目标亮度
    uint8_t newTargetBrightness = calculateBrightness(smoothed);
    
    // 更新目标亮度（仅当变化超过阈值时）
    int targetDiff = abs((int)newTargetBrightness - (int)targetBrightness);
    if (targetDiff >= BRIGHTNESS_HYSTERESIS) {
        targetBrightness = newTargetBrightness;
        LOG_DISPLAY_DEBUG("Target brightness updated: %.2f lux -> %d/255 (raw: %.2f lux)", 
                         smoothed, targetBrightness, currentLux);
    }
    
    // 渐变调整当前亮度向目标亮度靠近
    if (lastAppliedBrightness != targetBrightness) {
        int diff = (int)targetBrightness - (int)lastAppliedBrightness;
        
        if (abs(diff) <= BRIGHTNESS_FADE_STEP) {
            // 差值小于步进值，直接到达目标
            lastAppliedBrightness = targetBrightness;
        } else {
            // 按步进值渐变
            if (diff > 0) {
                lastAppliedBrightness += BRIGHTNESS_FADE_STEP;
            } else {
                lastAppliedBrightness -= BRIGHTNESS_FADE_STEP;
            }
        }
        
        setLcdBrightness(lastAppliedBrightness);
        
        LOG_DISPLAY_VERBOSE("Brightness fading: %d -> %d (target: %d)", 
                           lastAppliedBrightness - (diff > 0 ? BRIGHTNESS_FADE_STEP : -BRIGHTNESS_FADE_STEP),
                           lastAppliedBrightness, targetBrightness);
        
        return true;
    }
    
    return false;
}

void setCustomLuxMap(const LuxBrightnessPoint* luxMap, uint8_t mapSize) {
    if (luxMap == nullptr || mapSize == 0) {
        LOG_DISPLAY_ERROR("Invalid lux map");
        return;
    }
    
    currentLuxMap = luxMap;
    currentMapSize = mapSize;
    
    LOG_DISPLAY_INFO("Custom lux map applied (%d points)", mapSize);
}

void setManualBrightness(uint8_t brightness) {
    // 禁用自动调节
    isAutoBrightnessEnabled = false;
    
    // 应用手动亮度
    setLcdBrightness(brightness);
    lastAppliedBrightness = brightness;
    
    LOG_DISPLAY_INFO("Manual brightness set: %d/255", brightness);
}

float getCurrentLux(bool smoothed) {
    if (!isOPT3001Connected) {
        return -1.0;
    }
    
    if (smoothed) {
        return smoothedLux;
    } else {
        return readLux();
    }
}

void printAutoBrightnessInfo() {
    LOG_DISPLAY_INFO("========== Auto Brightness Status ==========");
    LOG_DISPLAY_INFO("Status: %s", isAutoBrightnessActive() ? "ENABLED" : "DISABLED");
    LOG_DISPLAY_INFO("OPT3001 Connected: %s", isOPT3001Connected ? "YES" : "NO");
    
    if (isOPT3001Connected) {
        float currentLux = readLux();
        LOG_DISPLAY_INFO("Current Lux: %.2f lux (raw)", currentLux);
        LOG_DISPLAY_INFO("Smoothed Lux: %.2f lux", smoothedLux);
        LOG_DISPLAY_INFO("Target Brightness: %d/255 (%d%%)", 
                        targetBrightness, (targetBrightness * 100) / 255);
        LOG_DISPLAY_INFO("Applied Brightness: %d/255 (%d%%)", 
                        lastAppliedBrightness, (lastAppliedBrightness * 100) / 255);
    }
    
    LOG_DISPLAY_INFO("Min Brightness: %d", MIN_BRIGHTNESS);
    LOG_DISPLAY_INFO("Max Brightness: %d", MAX_BRIGHTNESS);
    LOG_DISPLAY_INFO("Hysteresis: %d", BRIGHTNESS_HYSTERESIS);
    LOG_DISPLAY_INFO("Smoothing Factor: %.2f", LUX_SMOOTHING_FACTOR);
    LOG_DISPLAY_INFO("==========================================");
}

// ==================== 后台任务实现 ====================

/**
 * @brief 自动背光调节任务
 * @param parameter 任务参数（未使用）
 */
static void _autoBrightnessTask(void* parameter) {
    LOG_DISPLAY_INFO("Auto brightness task started");
    
    while (true) {
        // 如果任务被禁用，等待后继续检查
        if (!isAutoBrightnessActive()) {
            vTaskDelay(pdMS_TO_TICKS(1000));  // 禁用时1秒检查一次
            continue;
        }

        const bool menuIdle = inMenuMode && !clientConnected;
        const TickType_t updateInterval = menuIdle ? pdMS_TO_TICKS(1200) : pdMS_TO_TICKS(300);
        
        // 更新背光亮度
        updateAutoBrightness();
        
        // 等待下一次更新
        vTaskDelay(updateInterval);
    }
}

void startAutoBrightnessTask() {
    // 如果任务已经在运行，先停止
    if (autoBrightnessTaskHandle != nullptr) {
        LOG_DISPLAY_WARN("Auto brightness task already running, stopping old task");
        stopAutoBrightnessTask();
    }
    
    // 创建任务
    BaseType_t result = xTaskCreate(
        _autoBrightnessTask,           // 任务函数
        "AutoBrightness",              // 任务名称
        4096,                          // 栈大小（增加到4KB）
        nullptr,                       // 任务参数
        1,                             // 优先级
        &autoBrightnessTaskHandle      // 任务句柄
    );
    
    if (result == pdPASS) {
        LOG_DISPLAY_INFO("Auto brightness task created successfully");
    } else {
        LOG_DISPLAY_ERROR("Failed to create auto brightness task");
        autoBrightnessTaskHandle = nullptr;
    }
}

void stopAutoBrightnessTask() {
    if (autoBrightnessTaskHandle != nullptr) {
        vTaskDelete(autoBrightnessTaskHandle);
        autoBrightnessTaskHandle = nullptr;
        LOG_DISPLAY_INFO("Auto brightness task stopped");
    }
}
