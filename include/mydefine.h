#ifndef MYDEFINE_H
#define MYDEFINE_H

#include "myhader.h"

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
 * @note 启用内部下拉，禁用上拉电阻，适合按钮等默认低电平输入设备
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

#define LCD_RS              9   ///< LCD 寄存器选择（数据/指令）引脚
#define LCD_E               10  ///< LCD 使能引脚（触发数据传输）
#define LCD_D4              11  ///< LCD 数据引脚 D4
#define LCD_D5              12  ///< LCD 数据引脚 D5
#define LCD_D6              13  ///< LCD 数据引脚 D6
#define LCD_D7              14  ///< LCD 数据引脚 D7
#define LCD_BLA             8   ///< LCD 背光控制引脚（可接 PWM 实现调光）

// ==============================
// PWM 参数定义（LCD 背光）
// ==============================

#define LCD_BLA_PWM_PIN         LCD_BLA       ///< LCD 背光控制引脚
#define LCD_BLA_PWM_CHANNEL     0             ///< PWM 通道（0~7）
#define LCD_BLA_PWM_FREQ        5000          ///< PWM 频率（Hz）
#define LCD_BLA_PWM_RESOLUTION  8             ///< 分辨率（位数）：8-bit → 0~255
#define LCD_BLA_PWM_MAX_DUTY    ((1 << LCD_BLA_PWM_RESOLUTION) - 1) ///< 最大占空比

// ==============================
// 按键引脚定义（带方向语义）
// ==============================

#define BUTTEN_UP           17  ///< 向上按键
#define BUTTEN_DOWN         18  ///< 向下按键
#define BUTTEN_LEFT         6   ///< 向左按键
#define BUTTEN_RIGHT        4   ///< 向右按键
#define BUTTEN_CENTER       5   ///< 确认/中心按键

// ==============================
// 板载外设
// ==============================

#define RGB_PIN             35  ///< 板载 RGB 灯控制引脚

// ==============================
// 系统参数设置
// ==============================

#define BaudRate            115200  ///< 串口通信波特率
#define MAX_CACHE_SIZE      200     ///< 最大缓存帧数量
#define MAX_LATENCY_MS      1500    ///< 最大缓存延迟（单位：毫秒）
#define CONNECT_PORT        13000   ///< TCP/UDP 通信端口号
#define DEBOUNCE_TIME       30      ///< 按钮消抖时间（单位：毫秒）

#endif