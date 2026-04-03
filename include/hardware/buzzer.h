/**
 * @file buzzer.h
 * @brief 蜂鸣器模块头文件，提供蜂鸣器控制功能
 *
 * 此文件声明蜂鸣器初始化、音频播放和控制的函数
 * 使用 FreeRTOS 任务实现非阻塞音频播放
 *
 * @author kulib
 * @date 2025-12-26
 */
#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "mydefine.h"
#include "./utils/logger.h"

// ==============================
// 蜂鸣器配置定义
// ==============================

#define BUZZER_QUEUE_SIZE 5       ///< 音频队列大小
#define BUZZER_TIMEOUT_MS 3000    ///< 音频请求超时时间（毫秒）

// ==============================
// 音符频率定义（Hz）
// 基于标准音高 A4=440Hz 的十二平均律
// ==============================

// 第4八度（中央C所在八度）
#define NOTE_C4   262   ///< C4 (中央C)
#define NOTE_CS4  277   ///< C#4 / Db4
#define NOTE_Db4  277   ///< Db4 / C#4
#define NOTE_D4   294   ///< D4
#define NOTE_DS4  311   ///< D#4 / Eb4
#define NOTE_Eb4  311   ///< Eb4 / D#4
#define NOTE_E4   330   ///< E4
#define NOTE_F4   349   ///< F4
#define NOTE_FS4  370   ///< F#4 / Gb4
#define NOTE_Gb4  370   ///< Gb4 / F#4
#define NOTE_G4   392   ///< G4
#define NOTE_GS4  415   ///< G#4 / Ab4
#define NOTE_Ab4  415   ///< Ab4 / G#4
#define NOTE_A4   440   ///< A4 (标准音高)
#define NOTE_AS4  466   ///< A#4 / Bb4
#define NOTE_Bb4  466   ///< Bb4 / A#4
#define NOTE_B4   494   ///< B4

// 第5八度
#define NOTE_C5   523   ///< C5
#define NOTE_CS5  554   ///< C#5 / Db5
#define NOTE_Db5  554   ///< Db5 / C#5
#define NOTE_D5   587   ///< D5
#define NOTE_DS5  622   ///< D#5 / Eb5
#define NOTE_Eb5  622   ///< Eb5 / D#5
#define NOTE_E5   659   ///< E5
#define NOTE_F5   698   ///< F5
#define NOTE_FS5  740   ///< F#5 / Gb5
#define NOTE_Gb5  740   ///< Gb5 / F#5
#define NOTE_G5   784   ///< G5
#define NOTE_GS5  831   ///< G#5 / Ab5
#define NOTE_Ab5  831   ///< Ab5 / G#5
#define NOTE_A5   880   ///< A5
#define NOTE_AS5  932   ///< A#5 / Bb5
#define NOTE_Bb5  932   ///< Bb5 / A#5
#define NOTE_B5   988   ///< B5

// 第6八度
#define NOTE_C6   1047  ///< C6
#define NOTE_CS6  1109  ///< C#6 / Db6
#define NOTE_Db6  1109  ///< Db6 / C#6
#define NOTE_D6   1175  ///< D6
#define NOTE_DS6  1245  ///< D#6 / Eb6
#define NOTE_Eb6  1245  ///< Eb6 / D#6
#define NOTE_E6   1319  ///< E6
#define NOTE_F6   1397  ///< F6
#define NOTE_FS6  1480  ///< F#6 / Gb6
#define NOTE_Gb6  1480  ///< Gb6 / F#6
#define NOTE_G6   1568  ///< G6
#define NOTE_GS6  1661  ///< G#6 / Ab6
#define NOTE_Ab6  1661  ///< Ab6 / G#6
#define NOTE_A6   1760  ///< A6
#define NOTE_AS6  1865  ///< A#6 / Bb6
#define NOTE_Bb6  1865  ///< Bb6 / A#6
#define NOTE_B6   1976  ///< B6

// 第3八度（低音区）
#define NOTE_C3   131   ///< C3
#define NOTE_CS3  139   ///< C#3 / Db3
#define NOTE_Db3  139   ///< Db3 / C#3
#define NOTE_D3   147   ///< D3
#define NOTE_DS3  156   ///< D#3 / Eb3
#define NOTE_Eb3  156   ///< Eb3 / D#3
#define NOTE_E3   165   ///< E3
#define NOTE_F3   175   ///< F3
#define NOTE_FS3  185   ///< F#3 / Gb3
#define NOTE_Gb3  185   ///< Gb3 / F#3
#define NOTE_G3   196   ///< G3
#define NOTE_GS3  208   ///< G#3 / Ab3
#define NOTE_Ab3  208   ///< Ab3 / G#3
#define NOTE_A3   220   ///< A3
#define NOTE_AS3  233   ///< A#3 / Bb3
#define NOTE_Bb3  233   ///< Bb3 / A#3
#define NOTE_B3   247   ///< B3

// 静音
#define NOTE_REST 0     ///< 静音/休止符

// ==============================
// 音频类型枚举
// ==============================

enum class BuzzerSoundType {
    TONE,           ///< 单音调
    BEEP,           ///< 简单提示音
    DOUBLE_BEEP,    ///< 双音提示
    SUCCESS,        ///< 成功提示音
    ERROR,          ///< 错误提示音
    WARNING,        ///< 警告音
    STARTUP,        ///< 开机音
    CLICK,          ///< 按键音
    BUSY,           ///< 忙提示音
    MELODY          ///< 旋律
};

// ==============================
// 音频队列项结构
// ==============================

struct BuzzerQueueItem {
    BuzzerSoundType type;      ///< 音频类型
    uint16_t frequency;        ///< 频率（用于TONE类型）
    uint32_t duration;         ///< 持续时间（毫秒）
    uint8_t volume;            ///< 音量
    uint32_t timestamp;        ///< 请求时间戳（毫秒）
    const uint16_t* melody;    ///< 旋律数组指针
    const uint16_t* durations; ///< 持续时间数组指针
    uint16_t melodyLength;     ///< 旋律长度（改为 uint16_t 支持大于 255 的长度）
};

// ==============================
// 函数声明
// ==============================

/**
 * @brief 初始化蜂鸣器模块
 * @details 配置 PWM 通道，创建音频队列和处理任务
 */
void buzzerInit();

/**
 * @brief 播放指定频率和音量的声音（连续）
 * @param[in] frequency 频率（Hz）
 * @param[in] volume 音量 (0-100)，默认50
 */
void buzzerTone(uint16_t frequency, uint8_t volume = 50);

/**
 * @brief 停止播放
 */
void buzzerNoTone();

/**
 * @brief 设置默认音量
 * @param[in] volume 音量 (0-100)
 */
void buzzerSetVolume(uint8_t volume);

/**
 * @brief 获取当前默认音量
 * @return 当前音量 (0-100)
 */
uint8_t buzzerGetVolume();

/**
 * @brief 检查是否正在播放
 * @return true 正在播放，false 空闲
 */
bool buzzerIsBusy();

/**
 * @brief 设置UI按键音效总开关
 * @param[in] enabled true启用，false禁用
 */
void buzzerSetUiSoundEnabled(bool enabled);

/**
 * @brief 获取UI按键音效开关状态
 * @return true 已启用，false 已禁用
 */
bool buzzerIsUiSoundEnabled();

// ===== 快捷音效函数 =====

/**
 * @brief 播放简单提示音（非阻塞）
 */
void buzzerBeep();

/**
 * @brief 播放双音提示（非阻塞）
 */
void buzzerDoubleBeep();

/**
 * @brief 播放成功提示音（非阻塞）
 */
void buzzerPlaySuccess();

/**
 * @brief 播放错误提示音（非阻塞）
 */
void buzzerPlayError();

/**
 * @brief 播放警告音（非阻塞）
 */
void buzzerPlayWarning();

/**
 * @brief 播放开机音（非阻塞）
 */
void buzzerPlayStartup();

/**
 * @brief 播放按键音（非阻塞）
 */
void buzzerPlayClick();

/**
 * @brief 播放忙提示音（非阻塞）
 */
void buzzerPlayBusy();

/**
 * @brief 播放一次指定频率与时长的单音（非阻塞）
 * @param[in] frequency 频率（Hz）
 * @param[in] duration 持续时间（ms）
 * @param[in] volume 音量 (0-100)，默认当前音量
 */
void buzzerPlayTone(uint16_t frequency, uint16_t duration, uint8_t volume = 50);

/**
 * @brief 播放导航移动音效（旋钮移动）
 */
void buzzerPlayNavigateSound();

/**
 * @brief 播放确认音效（点击/确认）
 */
void buzzerPlaySelectSound();

/**
 * @brief 播放返回音效
 */
void buzzerPlayBackSound();

/**
 * @brief 播放休眠确认音效（长按触发休眠）
 */
void buzzerPlaySleepSound();

/**
 * @brief 播放自定义旋律（非阻塞）
 * @param[in] melody 旋律数组（频率）
 * @param[in] durations 持续时间数组（毫秒）
 * @param[in] length 数组长度（支持大于 255 的长度）
 * @param[in] volume 音量 (0-100)，默认50
 */
void buzzerPlayMelody(const uint16_t* melody, const uint16_t* durations, uint16_t length, uint8_t volume = 50);

#endif // BUZZER_H