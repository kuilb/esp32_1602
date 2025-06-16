#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "myhader.h"
#include "lcd_driver.h"
#include "kanamap.h"

/**
 * @brief 将UTF-8编码的假名字符转换为Unicode编码
 * 
 * @param[in] c0 UTF-8编码的第1字节
 * @param[in] c1 UTF-8编码的第2字节
 * @param[in] c2 UTF-8编码的第3字节
 * @return int 对应的Unicode编码值
 * 
 * @note 该函数假设输入为3字节UTF-8编码，适用于假名字符。
 */
int utf8ToUnicode(uint8_t c0, uint8_t c1, uint8_t c2);

/**
 * @brief 解析并显示接收到的数据帧内容
 * 
 * @param[in] raw    指向接收数据缓冲区的指针
 * @param[in] bodyLen 数据包长度（字节数）
 * 
 * @details
 *  1. 检查数据包长度和头部有效性
 *  2. 读取帧率字段（2字节，高字节先发）
 *  3. 解析数据体，处理普通字符和自定义字符
 *  4. 支持UTF-8假名字符的转换和显示，使用kanaMap映射
 *  5. 自定义字符（8字节点阵）循环复用，最多支持8个自定义字符槽
 *  6. 显示过程中缓冲普通字符批量输出
 *  7. 处理未知标志和错误情况，并打印调试信息
 *  8. 最后填充空格确保屏幕内容满屏
 */
void processIncoming(const uint8_t* data, size_t len);

#endif