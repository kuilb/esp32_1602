#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include "myheader.h"
#include "mydefine.h"
#include "kanamap.h"
#include "logger.h"

// ==============================
// LCD 显示命令定义
// ==============================

#define CMD                 0   ///< 表示发送的是 LCD 命令
#define CHR                 1   ///< 表示发送的是 LCD 字符数据

#define LCD_line1           0x80 ///< LCD 第一行 DDRAM 起始地址
#define LCD_line2           0xc0 ///< LCD 第二行 DDRAM 起始地址


/**
 * @brief 当前 LCD 光标位置
 * 
 * 记录光标在 LCD 显示区的位置索引，  
 * 范围为 0 到 31。  
 * 该变量用于跟踪和控制字符显示位置。
 */
extern int lcdCursor;

// 当前背光亮度（0~255），初始为最大亮度
extern int brightness;

/**
 * @brief 设置 LCD 背光亮度（使用 PWM）
 * 
 * @param duty 占空比亮度值，范围 0~255
 */
void setLcdBrightness(uint8_t duty);

/**
 * @brief 改变当前亮度值
 * 
 * @param delta 增量值（可以为负值）
 */
void changeBrightness(int delta);

/**
 * @brief 初始化 LCD 显示模块
 * 
 * 配置 LCD 所需的 GPIO 引脚，发送初始化指令序列，  
 * 使 LCD 进入可正常显示的工作状态。  
 * 此函数应在系统启动时调用一次。
 */
void lcd_init();

/**
 * @brief 在指定行显示一段文本（最多 16 个字符）
 * 
 * 用于向 LCD 显示器快速输出文本，常用于简单显示或调试输出。  
 * 文本将从指定行的起始位置显示，超过部分将被截断，  
 * 不足部分将自动填充空格补齐。
 * 
 * @param[in] ltext 要显示的文本内容
 * @param[in] line 显示的行号（1 表示第一行，2 表示第二行）
 */
void lcd_text(String ltext,int line);

/**
 * @brief 将 LCD 光标重置到屏幕左上角 (0,0)，并重置全局光标位置变量。
 *
 * 通常在每次刷新、写入新内容或初始化帧时调用。
 */
void lcdResetCursor();

/**
 * @brief 在 CGRAM 中创建自定义字符
 * 
 * 将 8 字节点阵数据写入指定的 CGRAM 位置，  
 * 用于定义自定义 LCD 字符（最多支持 8 个槽位，编号 0~7）。  
 * 写入完成后，光标位置恢复到当前 DDRAM 的显示位置。
 * 
 * @param[in] slot 自定义字符槽位编号，范围 0~7
 * @param[in] data 包含 8 字节点阵数据的数组，每字节对应字符的一行像素
 */
void lcdCreateChar(int slot, uint8_t data[8]);

/**
 * @brief 显示自定义字符
 * 
 * 在当前光标位置显示已定义的自定义字符。  
 * 自定义字符槽位编号范围为 0~7，超出范围则不执行显示。  
 * 该函数通过向 LCD 发送对应槽位的字符编码实现显示。
 * 
 * @param[in] index 自定义字符槽位编号（0~7）
 */
void lcdDisCustom(int index);

/**
 * @brief 显示普通字符
 * 
 * 将单个字符显示在当前光标位置，  
 * 并自动将光标移动到下一个位置。  
 * 适用于显示 ASCII 字符或 LCD 支持的标准字符集。
 * 
 * @param[in] text 要显示的字符
 */
void lcdDisChar(char text);

/**
 * @brief 显示整段普通字符文本
 * 
 * 将字符串中的每个字符依次显示在 LCD 上，  
 * 光标自动移动，支持连续显示文本。  
 * 适合用于输出多字符信息。
 * 
 * @param[in] s 要显示的字符串
 */
void lcdPrint(String s);

void lcdSetCursor(int changecursor);

#endif