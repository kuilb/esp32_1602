#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include "myhader.h"
#include "mydefine.h"

/**
 * @brief 当前 LCD 光标位置
 * 
 * 记录光标在 LCD 显示区的位置索引，  
 * 范围为 0 到 31。  
 * 该变量用于跟踪和控制字符显示位置。
 */
extern int lcdCursor;

/**
 * @brief 触发 LCD 的 E 引脚以启动数据处理
 * 
 * 向 LCD 的 Enable（E）引脚发送一个上升沿脉冲，  
 * 通知 LCD 开始处理当前传输的数据或指令。
 */
void trigger_E();

/**
 * @brief 向 LCD 缓冲区写入一个字节的数据
 * 
 * 根据指定的模式（命令或字符）将 8 位数据写入 LCD 的数据线。
 * 
 * @param[in] data 要写入的数据字节（8 位）
 * @param[in] mode 写入模式：0 表示命令（CMD），1 表示字符（CHR）
 */
void gpio_write(int data,int mode);

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
 * @brief 设置 LCD 光标的位置
 * 
 * 将光标移动到指定的列和行，以便在该位置显示字符。  
 * 
 * @param[in] col 列索引（0~15）
 * @param[in] row 行索引（0 表示第一行，1 表示第二行）
 */
void lcd_setCursor(int col, int row);

/**
 * @brief 光标向后移动一格
 * 
 * 将当前光标位置向右移动一个字符位置。  
 * 当光标超出显示范围(32)时，  
 * 将光标设置到显示区域之外，  
 * 用于标识光标已超出显示边界。
 */
void lcd_next_cursor();

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
void lcd_createChar(int slot, uint8_t data[8]);

/**
 * @brief 显示自定义字符
 * 
 * 在当前光标位置显示已定义的自定义字符。  
 * 自定义字符槽位编号范围为 0~7，超出范围则不执行显示。  
 * 该函数通过向 LCD 发送对应槽位的字符编码实现显示。
 * 
 * @param[in] index 自定义字符槽位编号（0~7）
 */
void lcd_dis_custom(int index);

/**
 * @brief 显示普通字符
 * 
 * 将单个字符显示在当前光标位置，  
 * 并自动将光标移动到下一个位置。  
 * 适用于显示 ASCII 字符或 LCD 支持的标准字符集。
 * 
 * @param[in] text 要显示的字符
 */
void lcd_dis_chr(char text);

/**
 * @brief 清屏并重置光标位置
 * 
 * 发送清屏命令清除 LCD 显示内容，  
 * 并将光标重置到第一行第一列。  
 */
void lcd_clear();

/**
 * @brief 显示整段普通字符文本
 * 
 * 将字符串中的每个字符依次显示在 LCD 上，  
 * 光标自动移动，支持连续显示文本。  
 * 适合用于输出多字符信息。
 * 
 * @param[in] s 要显示的字符串
 */
void lcd_print(String s);

#endif