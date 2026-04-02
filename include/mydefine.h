#ifndef MYDEFINE_H
#define MYDEFINE_H

#include <Arduino.h>

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
inline void setInputPullDown(int pin) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << pin;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;   // 启用内部下拉
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;      // 禁用上拉
    gpio_config(&io_conf);
}

/**
 * @brief 将指定 GPIO 引脚配置为输入模式（带内部上拉）
 * 
 * @param[in] pin GPIO 引脚号
 * 
 * @note 启用内部上拉，禁用下拉电阻，用于按钮等输入
 */
inline void setInputPullUp(int pin) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << pin;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;  // 禁用内部下拉
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;       // 启用内部上拉
    gpio_config(&io_conf);
}


// ==============================
// LCD 引脚定义
// ==============================

#define LCD_RS              1   ///< LCD RS
#define LCD_E               2  ///< LCD E
#define LCD_D4              3  ///< LCD D4
#define LCD_D5              4  ///< LCD D5
#define LCD_D6              5  ///< LCD D6
#define LCD_D7              6  ///< LCD D7
#define LCD_BLA             7  ///< LCD 背光
#define LCD_CTL            18  ///< LCD 对比度控制引脚（ADC）

// ==============================
// PWM 参数定义（LCD 背光）
// ==============================

#define LCD_BLA_PWM_PIN         LCD_BLA       ///< LCD 背光控制引脚
#define LCD_BLA_PWM_CHANNEL     0             ///< PWM 通道（0~7）
#define LCD_BLA_PWM_FREQ        5000          ///< PWM 频率（Hz）
#define LCD_BLA_PWM_RESOLUTION  8             ///< 分辨率（位数）
#define LCD_BLA_PWM_MAX_DUTY    ((1 << LCD_BLA_PWM_RESOLUTION) - 1) ///< 最大占空比

// ==============================
// PWM 参数定义（LCD 对比度）
// ==============================

#define LCD_CTL_PWM_PIN         LCD_CTL       ///< LCD 对比度控制引脚
#define LCD_CTL_PWM_CHANNEL     2             ///< PWM 通道（0~7）
#define LCD_CTL_PWM_FREQ        5000          ///< PWM 频率（Hz）
#define LCD_CTL_PWM_RESOLUTION  8             ///< 分辨率（位数）
#define LCD_CTL_PWM_MAX_DUTY    ((1 << LCD_CTL_PWM_RESOLUTION) - 1) ///< 最大占空比

// ==============================
// 按键引脚定义
// ==============================
#define BUTTON_LEFT_PIN         9  ///< 左按键
#define BUTTON_RIGHT_PIN        11  ///< 右按键
#define BUTTON_CENTER_PIN       10   ///< 中按键
#define BUTTON_SIDE_PIN         12  ///< 侧按键

#define BUTTON_POWER_PIN        12   ///< 电源按键

// ==============================
// 燃料计IC引脚定义
// ==============================
#define FUEL_GAUGE_WARNING_PIN  17  ///< 燃料计警告引脚

//i2c
#define SDA_PIN     38   ///< SDA 引脚
#define SCL_PIN     21   ///< SCL 引脚

// ==============================
// 板载外设
// ==============================

#define RGB_PIN             8  ///< 板载 RGB 灯
#define BUZZER_PIN         13  ///< 板载 蜂鸣器
#define DC_PLUG_PIN        14  ///< 板载 DC 插入检测

// ==============================
// 系统参数设置
// ==============================
#define BATTERY_DESIGN_CAPACITY_MAH 1000  ///< 电池设计容量 (mAh)
#define VISIBLE_LINES       2           ///< LCD 行数

#define BaudRate            115200      ///< 串口通信波特率
#define MAX_CACHE_SIZE      200         ///< 最大缓存帧数量
#define MAX_RECV_BUFFER_SIZE 1024       ///< 最大接收缓冲区大小（字节）
#define MAX_LATENCY_MS      300         ///< 最大缓存延迟（单位：毫秒）
#define CONNECT_PORT        13000       ///< TCP/UDP 通信端口号
#define CONNECT_TIMEOUT_MS  5000        ///< 连接超时时间（单位：毫秒）

#define DEBOUNCE_TIME       20          ///< 按钮扫描消抖时间（单位：毫秒）
#define BUTTON_DEBOUNCE_DELAY 150       ///< 按钮软件消抖延迟（单位：毫秒）
#define FIRST_TIME_DELAY    300         ///< 首次启动延迟时间（单位：毫秒）

#define TIME_SYNC_TIMEOUT   30000       ///< 时间同步超时时间（单位：毫秒）
#define TIME_SYNC_RETRY_INTERVAL 1000   ///< 时间同步重试间隔（单位：毫秒）
#define GMT_OFFSET_HOUR     8           ///< GMT 偏移（时间），北京时间为 UTC+8

#endif