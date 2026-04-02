/**
 * @file lcd_driver.h
 * @brief LCD 驱动模块头文件，提供 LCD 显示控制功能
 *
 * 此文件声明 LCD 初始化、显示和控制的函数和变量
 *
 * @author kulib
 * @date 2025-11-06
 */
#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <Arduino.h>

#include "mydefine.h"
#include "./services/kanamap.h"
#include "./utils/logger.h"

// ==============================
// LCD 显示命令定义
// ==============================

#define CMD                 0   ///< 表示发送的是 LCD 命令
#define CHR                 1   ///< 表示发送的是 LCD 字符数据

#define LCD_line1           0x80 ///< LCD 第一行 DDRAM 起始地址
#define LCD_line2           0xc0 ///< LCD 第二行 DDRAM 起始地址

extern int lcdCursor;                   /**< 当前 LCD 光标位置 */
extern int brightness;                  /**< 当前背光亮度（0~255），初始为最大亮度 */

/**
 * @brief 设置 LCD 背光亮度（使用 PWM）
 * @details 使用 PWM 控制 LCD 背光亮度，占空比范围 0~255
 * @param[in] duty 占空比亮度值, 范围 0~255
 */
void setLcdBrightness(uint8_t duty);

/**
 * @brief 改变当前亮度值
 * @details 根据增量值调整当前背光亮度，防止超出范围
 * @param[in] delta 增量值（可以为负值）
 */
void changeBrightness(int delta);

/**
 * @brief 初始化 LCD 显示模块
 * @details 配置 LCD 所需的 GPIO 引脚，发送初始化指令序列，使 LCD 进入可正常显示的工作状态，此函数应在系统启动时调用一次
 */
void lcdInit();

/**
 * @brief 在指定行显示一段文本（最多 16 个字符）
 * @details 用于向 LCD 显示器快速输出文本，常用于简单显示或调试输出，文本将从指定行的起始位置显示，超过部分将被截断，不足部分将自动填充空格补齐
 * @param[in] ltext 要显示的文本内容
 * @param[in] line 显示的行号（1 表示第一行，2 表示第二行）
 */
void lcdText(const String& ltext,int line);

/**
 * @brief 将 LCD 光标重置到屏幕左上角 (0,0)，并重置全局光标位置变量
 * @details 通常在每次刷新、写入新内容或初始化帧时调用
 */
void lcdResetCursor();

/**
 * @brief 清除 LCD 屏幕内容
 * @details 发送清屏命令清除所有显示内容，并将光标重置到左上角 (0,0)
 */
void lcdClear();

/**
 * @brief 在 CGRAM 中创建自定义字符（手动指定槽位）
 * @details 将 8 字节点阵数据写入指定的 CGRAM 位置，用于定义自定义 LCD 字符（最多支持 8 个槽位，编号 0~7），写入完成后，光标位置恢复到当前 DDRAM 的显示位置
 * @param[in] slot 自定义字符槽位编号, 范围 0~7
 * @param[in] data 包含 8 字节点阵数据的数组, 每字节对应字符的一行像素
 */
void lcdCreateChar(int slot, const uint8_t data[8]);

/**
 * @brief 在 CGRAM 中创建自定义字符并立即显示（自动分配槽位）
 * @details 将 8 字节点阵数据写入自动分配的 CGRAM 位置并在当前光标位置显示，槽位会在 0~7 之间自动循环，如果在重置后调用超过 8 次，会输出警告日志。显示后光标会自动移动到下一个位置
 * @param[in] data 包含 8 字节点阵数据的数组, 每字节对应字符的一行像素
 * @return 返回使用的槽位编号 (0~7)
 */
int lcdCreateCharAuto(const uint8_t data[8]);

/**
 * @brief 重置自动分配的槽位计数器
 * @details 将自动槽位分配重置为 0，用于新一帧显示的开始，确保槽位使用不会超出范围
 */
void lcdResetCharSlot();

/**
 * @brief 显示自定义字符
 * @details 在当前光标位置显示已定义的自定义字符，自定义字符槽位编号范围为 0~7，超出范围则不执行显示，该函数通过向 LCD 发送对应槽位的字符编码实现显示
 * @param[in] index 自定义字符槽位编号（0~7）
 */
void lcdDisCustom(int index);

/**
 * @brief 显示普通字符
 * @details 将单个字符显示在当前光标位置，并自动将光标移动到下一个位置，适用于显示 ASCII 字符或 LCD 支持的标准字符集
 * @param[in] text 要显示的字符
 */
void lcdDisChar(char text);

/**
 * @brief 显示整段普通字符文本
 * @details 将字符串中的每个字符依次显示在 LCD 上，光标自动移动，支持连续显示文本，适合用于输出多字符信息
 * @param[in] s 要显示的字符串
 */
void lcdPrint(const String& s);

/**
 * @brief 显示 C 风格字符串
 * @details 避免临时 String 构造，降低堆分配与拷贝开销
 * @param[in] s 以 '\0' 结尾的字符串
 */
void lcdPrint(const char* s);

/**
 * @brief 设置 LCD 光标位置
 * @details 设置光标到指定的位置索引，范围 0~31
 * @param[in] changecursor 新的光标位置索引, 范围 0~31
 */
void lcdSetCursor(int changecursor);

/**
 * @brief 差分刷新整屏（32字节），只更新变化的位置
 * @details
 * - ddram32: 屏幕 32 个字符位置的字符编码（0~255），其中 0~7 表示自定义字符槽位
 * - cgram8x8: 8 个自定义字符槽的点阵定义（每槽 8 字节）
 * - cgramUsed: 标记某槽本帧是否被使用；未使用槽不会被写入 CGRAM
 *
 * @note
 * - 内部会缓存上一帧的 DDRAM(32) 和 CGRAM(8*8)，仅在变化时写入
 * - 若某槽位点阵发生变化，会自动重写使用该槽位的屏幕位置，避免显示错误
 */
uint8_t lcdRenderDiff(const uint8_t ddram32[32], const uint8_t cgram8x8[8][8], const bool cgramUsed[8]);

#endif