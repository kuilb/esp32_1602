#ifndef MYDEFINE_H
#define MYDEFINE_H

#include "myhader.h"

#define setHigh(pin)        gpio_set_level((gpio_num_t)(pin), 1)
#define setLow(pin)         gpio_set_level((gpio_num_t)(pin), 0)

inline void setOutput(int pin) {
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
}

// inline void setInput(int pin) {
//     gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
// }

inline void setInput(int pin) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << pin;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;   // 启用内部下拉
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;      // 禁用上拉
    gpio_config(&io_conf);
}


// 以下是引脚定义
#define LCD_RS              9           // 数据/指令切换引脚
#define LCD_E               10          // 触发引脚
#define LCD_D4              11          // 四位数据引脚
#define LCD_D5              12
#define LCD_D6              13
#define LCD_D7              14
#define LCD_BLA             8           // 背光控制

#define BOTTEN_UP           17          // 按钮             
#define BOTTEN_DOWN         18            
#define BOTTEN_LEFT         6
#define BOTTEN_RIGHT        4
#define BOTTEN_CENTER       5

#define RGB_PIN             35           // 板载RGB灯

// 以下是命令定义
#define CMD                 0            // 命令为0
#define CHR                 1            // 数据为1
#define char_per_line       16           // 每行16个字符
#define LCD_line1           0x80         // 第一行地址
#define LCD_line2           0xc0         // 第二行地址

//以下是常用设置
#define BaudRate            115200       // 串口波特率
#define MAX_CACHE_SIZE      200          // 最大缓存包数
#define MAX_LATENCY_MS      1500         // 最多缓存 1500ms 画面
#define CONNECT_PORT        13000        // 连接端口
#define DEBOUNCE_TIME       30          // 按钮消抖时长

#endif