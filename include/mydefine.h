#ifndef MYDEFINE_H
#define MYDEFINE_H

#include "myheader.h"

/**
 * @brief 将指定 GPIO 引脚配置为输出模式
 * 
 * @param[in] pin GPIO 引脚号
 */
inline void setOutput(int pin) {
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
}

/**
 * @brief 将指定 GPIO 引脚配置为输入模式（带内部下拉）
 * 
 * @param[in] pin GPIO 引脚号
 * 
 * @note 启用内部下拉，禁用上拉电阻，用于按钮等输入
 */
inline void setInput(int pin) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << pin;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;   // 启用内部下拉
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;      // 禁用上拉
    gpio_config(&io_conf);
}


// ==============================
// LCD 引脚定义
// ==============================

#define LCD_RS              9   ///< LCD RS
#define LCD_E               10  ///< LCD E
#define LCD_D4              11  ///< LCD D4
#define LCD_D5              12  ///< LCD D5
#define LCD_D6              13  ///< LCD D6
#define LCD_D7              14  ///< LCD D7
#define LCD_BLA             8   ///< LCD 背光

// ==============================
// PWM 参数定义（LCD 背光）
// ==============================

#define LCD_BLA_PWM_PIN         LCD_BLA       ///< LCD 背光控制引脚
#define LCD_BLA_PWM_CHANNEL     0             ///< PWM 通道（0~7）
#define LCD_BLA_PWM_FREQ        5000          ///< PWM 频率（Hz）
#define LCD_BLA_PWM_RESOLUTION  8             ///< 分辨率（位数）
#define LCD_BLA_PWM_MAX_DUTY    ((1 << LCD_BLA_PWM_RESOLUTION) - 1) ///< 最大占空比

// ==============================
// 按键引脚定义（带方向语义）
// ==============================

#define BUTTEN_UP           17  ///< 上按键
#define BUTTEN_DOWN         18  ///< 下按键
#define BUTTEN_LEFT         6   ///< 左按键
#define BUTTEN_RIGHT        4   ///< 右按键
#define BUTTEN_CENTER       5   ///< 中按键

// ==============================
// 板载外设
// ==============================

#define RGB_PIN             35  ///< 板载 RGB 灯

// ==============================
// 系统参数设置
// ==============================
#define VISIBLE_LINES       2           ///< LCD 行数

#define BaudRate            115200      ///< 串口通信波特率
#define MAX_CACHE_SIZE      200         ///< 最大缓存帧数量
#define MAX_LATENCY_MS      1500        ///< 最大缓存延迟（单位：毫秒）
#define CONNECT_PORT        13000       ///< TCP/UDP 通信端口号

#define DEBOUNCE_TIME       30          ///< 按钮扫描消抖时间（单位：毫秒）
#define BUTTON_DEBOUNCE_DELAY 150       ///< 按钮软件消抖延迟（单位：毫秒）
#define FIRST_TIME_DELAY    300         ///< 首次启动延迟时间（单位：毫秒）

#define TIME_SYNC_TIMEOUT   10000       ///< 时间同步超时时间（单位：毫秒）
#define TIME_SYNC_RETRY_INTERVAL 1000   ///< 时间同步重试间隔（单位：毫秒）
#define GMT_OFFSET_HOUR     8           ///< GMT 偏移（时间），北京时间为 UTC+8

#endif