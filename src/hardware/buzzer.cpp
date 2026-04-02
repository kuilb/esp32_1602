#include "hardware/buzzer.h"

// ==============================
// 静态变量定义
// ==============================

static uint8_t buzzerVolume = 50;              ///< 默认音量
static uint8_t buzzerPwmChannel = 6;           ///< PWM通道（使用独立计时器通道，避免与LCD通道0/2频率串扰）
static const uint32_t buzzerPwmResolution = 8; ///< PWM分辨率
static TaskHandle_t buzzerTaskHandle = nullptr; ///< 任务句柄
static QueueHandle_t buzzerQueue = nullptr;    ///< 音频队列句柄
static volatile bool buzzerBusy = false;       ///< 忙标志

// ==============================
// 内部辅助函数声明
// ==============================

static uint32_t volumeToDutyCycle(uint8_t volume);
static void toneBlocking(uint16_t frequency, uint32_t duration, uint8_t volume);
static bool enqueueSound(const BuzzerQueueItem& item);
static void processQueueItem(const BuzzerQueueItem& item);
static void buzzerTaskFunction(void* parameter);

// ==============================
// 内部辅助函数实现
// ==============================

static uint32_t volumeToDutyCycle(uint8_t volume) {
    uint32_t maxDuty = (1 << buzzerPwmResolution) - 1;
    return map(volume, 0, 100, 0, maxDuty);
}

static void toneBlocking(uint16_t frequency, uint32_t duration, uint8_t volume) {
    if (frequency == 0) {
        buzzerNoTone();
        vTaskDelay(pdMS_TO_TICKS(duration));
        return;
    }
    
    uint8_t vol = constrain(volume, 0, 100);
    // 播放前先attach引脚到PWM
    ledcAttachPin(BUZZER_PIN, buzzerPwmChannel);
    ledcWriteTone(buzzerPwmChannel, frequency);
    ledcWrite(buzzerPwmChannel, volumeToDutyCycle(vol));
    vTaskDelay(pdMS_TO_TICKS(duration));
    buzzerNoTone();  // 播放完成后detach
}

static bool enqueueSound(const BuzzerQueueItem& item) {
    if (!buzzerQueue) return false;
    
    // 检查超时
    uint32_t currentTime = millis();
    if (currentTime - item.timestamp > BUZZER_TIMEOUT_MS) {
        return false;  // 超时，忽略请求
    }
    
    // 尝试加入队列（不阻塞）
    if (xQueueSend(buzzerQueue, &item, 0) != pdTRUE) {
        return false;  // 队列满
    }
    
    return true;
}

static void processQueueItem(const BuzzerQueueItem& item) {
    // 再次检查超时
    uint32_t currentTime = millis();
    if (currentTime - item.timestamp > BUZZER_TIMEOUT_MS) {
        return;  // 超时，跳过
    }
    
    buzzerBusy = true;
    
    switch (item.type) {
        case BuzzerSoundType::TONE:
            toneBlocking(item.frequency, item.duration, item.volume);
            break;
            
        case BuzzerSoundType::BEEP:
            toneBlocking(2000, 100, item.volume);
            break;
            
        case BuzzerSoundType::DOUBLE_BEEP:
            toneBlocking(2000, 80, item.volume);
            vTaskDelay(pdMS_TO_TICKS(80));
            toneBlocking(2000, 80, item.volume);
            break;
            
        case BuzzerSoundType::SUCCESS:
            toneBlocking(NOTE_C5, 100, item.volume);
            vTaskDelay(pdMS_TO_TICKS(50));
            toneBlocking(NOTE_E5, 100, item.volume);
            vTaskDelay(pdMS_TO_TICKS(50));
            toneBlocking(NOTE_G5, 200, item.volume);
            break;
            
        case BuzzerSoundType::ERROR:
            toneBlocking(NOTE_A4, 150, item.volume);
            vTaskDelay(pdMS_TO_TICKS(50));
            toneBlocking(NOTE_F4, 150, item.volume);
            vTaskDelay(pdMS_TO_TICKS(50));
            toneBlocking(NOTE_D4, 300, item.volume);
            break;
            
        case BuzzerSoundType::WARNING:
            for (int i = 0; i < 3; i++) {
                toneBlocking(1000, 100, item.volume);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            break;
            
        case BuzzerSoundType::STARTUP:
            toneBlocking(NOTE_E5, 100, item.volume);
            vTaskDelay(pdMS_TO_TICKS(50));
            toneBlocking(NOTE_C5, 100, item.volume);
            break;
            
        case BuzzerSoundType::CLICK:
            toneBlocking(3000, 30, item.volume);
            break;
            
        case BuzzerSoundType::BUSY:
            toneBlocking(1500, 50, item.volume);
            vTaskDelay(pdMS_TO_TICKS(30));
            toneBlocking(1500, 50, item.volume);
            vTaskDelay(pdMS_TO_TICKS(30));
            toneBlocking(1500, 50, item.volume);
            break;
            
        case BuzzerSoundType::MELODY:
            if (item.melody && item.durations) {
                int64_t startTime = esp_timer_get_time();
                int64_t accumulatedDurationUs = 0;

                for (uint16_t i = 0; i < item.melodyLength; i++) {
                    // 检查是否有新的高优先级任务（例如按键音），如果有则中断旋律
                    if (uxQueueMessagesWaiting(buzzerQueue) > 0) {
                        BuzzerQueueItem nextItem;
                        if (xQueuePeek(buzzerQueue, &nextItem, 0) == pdTRUE) {
                            // 如果不是旋律类型，说明是短音效，应该打断当前旋律
                            if (nextItem.type != BuzzerSoundType::MELODY) {
                                break;
                            }
                        }
                    }

                    // 从 PROGMEM 读取数据
                    uint16_t freq = pgm_read_word(&item.melody[i]);
                    uint16_t durMs = pgm_read_word(&item.durations[i]);
                    
                    // 累加目标时长
                    accumulatedDurationUs += (int64_t)durMs * 1000;
                    
                    // 计算当前应该结束的时间点
                    int64_t targetEndTime = startTime + accumulatedDurationUs;
                    int64_t now = esp_timer_get_time();
                    int64_t remainingUs = targetEndTime - now;

                    if (remainingUs > 0) {
                        if (freq == 0) {
                            buzzerNoTone();
                            // 使用 vTaskDelay 处理长休止，delayMicroseconds 处理短休止
                            if (remainingUs > 20000) {
                                vTaskDelay(pdMS_TO_TICKS(remainingUs / 1000));
                                // 补齐剩余微秒
                                delayMicroseconds(remainingUs % 1000);
                            } else {
                                delayMicroseconds(remainingUs);
                            }
                        } else {
                            // 播放音符
                            uint8_t vol = constrain(item.volume, 0, 100);
                            ledcAttachPin(BUZZER_PIN, buzzerPwmChannel);
                            ledcWriteTone(buzzerPwmChannel, freq);
                            ledcWrite(buzzerPwmChannel, volumeToDutyCycle(vol));
                            
                            // 保持发声直到时间结束
                            // 对于长音符，让出 CPU
                            if (remainingUs > 20000) {
                                vTaskDelay(pdMS_TO_TICKS(remainingUs / 1000));
                                // 补齐剩余微秒
                                delayMicroseconds(remainingUs % 1000);
                            } else {
                                delayMicroseconds(remainingUs);
                            }
                            
                            // 稍微断开一下，制造音符间隔感（非阻塞式）
                            // 注意：这里不再额外延时，而是包含在下一个音符的起始计算中
                            // 如果需要断奏，应该在 MIDI 生成阶段就处理好
                        }
                    } else {
                        // 如果已经超时（落后了），直接跳过等待，立即播放下一个
                        // 但仍然要发声一小会儿，避免完全吞音
                        if (freq > 0) {
                            uint8_t vol = constrain(item.volume, 0, 100);
                            ledcAttachPin(BUZZER_PIN, buzzerPwmChannel);
                            ledcWriteTone(buzzerPwmChannel, freq);
                            ledcWrite(buzzerPwmChannel, volumeToDutyCycle(vol));
                            delayMicroseconds(1000); // 极短发声
                        }
                    }
                }
                buzzerNoTone(); // 旋律结束关闭蜂鸣器
            }
            break;
    }
    
    buzzerBusy = false;
}

static void buzzerTaskFunction(void* parameter) {
    BuzzerQueueItem item;
    
    while (true) {
        // 等待队列中的音频项
        if (xQueueReceive(buzzerQueue, &item, portMAX_DELAY) == pdTRUE) {
            processQueueItem(item);
        }
    }
}

// ==============================
// 公共 API 函数实现
// ==============================

void buzzerInit() {
    // 先将引脚设置为输出低电平，防止浮空产生毛刺
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // 配置PWM通道但不立即attach，避免初始化时的信号
    ledcSetup(buzzerPwmChannel, 2000, buzzerPwmResolution);  // 默认2000Hz, 8位分辨率
    // 注意：不在这里attach引脚，保持引脚为普通GPIO低电平
    
    // 创建音频队列
    buzzerQueue = xQueueCreate(BUZZER_QUEUE_SIZE, sizeof(BuzzerQueueItem));
    
    // 创建音频处理任务
    xTaskCreate(
        buzzerTaskFunction,    // 任务函数
        "BuzzerTask",          // 任务名称
        8192,                  // 堆栈大小（从2048增加到8192，避免溢出）
        nullptr,               // 参数
        1,                     // 优先级
        &buzzerTaskHandle      // 任务句柄
    );
    
    LOG_SYSTEM_INFO("Buzzer initialized");
}

void buzzerTone(uint16_t frequency, uint8_t volume) {
    if (frequency == 0) {
        buzzerNoTone();
        return;
    }
    
    uint8_t vol = constrain(volume, 0, 100);
    // 播放前先attach引脚到PWM
    ledcAttachPin(BUZZER_PIN, buzzerPwmChannel);
    ledcWriteTone(buzzerPwmChannel, frequency);
    ledcWrite(buzzerPwmChannel, volumeToDutyCycle(vol));
}

void buzzerNoTone() {
    // 完全停止PWM：先设置占空比为0，然后detach引脚
    ledcWrite(buzzerPwmChannel, 0);
    ledcDetachPin(BUZZER_PIN);
    // 将引脚设回普通GPIO低电平，避免发热
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void buzzerSetVolume(uint8_t volume) {
    buzzerVolume = constrain(volume, 0, 100);
}

uint8_t buzzerGetVolume() {
    return buzzerVolume;
}

bool buzzerIsBusy() {
    return buzzerBusy;
}

// ==============================
// 快捷音效函数实现
// ==============================

void buzzerBeep() {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::BEEP,
        .frequency = 0,
        .duration = 0,
        .volume = buzzerVolume,
        .timestamp = millis(),
        .melody = nullptr,
        .durations = nullptr,
        .melodyLength = 0
    };
    enqueueSound(item);
}

void buzzerDoubleBeep() {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::DOUBLE_BEEP,
        .frequency = 0,
        .duration = 0,
        .volume = buzzerVolume,
        .timestamp = millis(),
        .melody = nullptr,
        .durations = nullptr,
        .melodyLength = 0
    };
    enqueueSound(item);
}

void buzzerPlaySuccess() {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::SUCCESS,
        .frequency = 0,
        .duration = 0,
        .volume = buzzerVolume,
        .timestamp = millis(),
        .melody = nullptr,
        .durations = nullptr,
        .melodyLength = 0
    };
    enqueueSound(item);
}

void buzzerPlayError() {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::ERROR,
        .frequency = 0,
        .duration = 0,
        .volume = buzzerVolume,
        .timestamp = millis(),
        .melody = nullptr,
        .durations = nullptr,
        .melodyLength = 0
    };
    enqueueSound(item);
}

void buzzerPlayWarning() {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::WARNING,
        .frequency = 0,
        .duration = 0,
        .volume = buzzerVolume,
        .timestamp = millis(),
        .melody = nullptr,
        .durations = nullptr,
        .melodyLength = 0
    };
    enqueueSound(item);
}

void buzzerPlayStartup() {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::STARTUP,
        .frequency = 0,
        .duration = 0,
        .volume = buzzerVolume,
        .timestamp = millis(),
        .melody = nullptr,
        .durations = nullptr,
        .melodyLength = 0
    };
    enqueueSound(item);
}

void buzzerPlayClick() {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::CLICK,
        .frequency = 0,
        .duration = 0,
        .volume = buzzerVolume,
        .timestamp = millis(),
        .melody = nullptr,
        .durations = nullptr,
        .melodyLength = 0
    };
    enqueueSound(item);
}

void buzzerPlayBusy() {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::BUSY,
        .frequency = 0,
        .duration = 0,
        .volume = buzzerVolume,
        .timestamp = millis(),
        .melody = nullptr,
        .durations = nullptr,
        .melodyLength = 0
    };
    enqueueSound(item);
}

void buzzerPlayMelody(const uint16_t* melody, const uint16_t* durations, uint16_t length, uint8_t volume) {
    BuzzerQueueItem item = {
        .type = BuzzerSoundType::MELODY,
        .frequency = 0,
        .duration = 0,
        .volume = volume,
        .timestamp = millis(),
        .melody = melody,
        .durations = durations,
        .melodyLength = length
    };
    enqueueSound(item);
}