#ifndef KANAMAP_H
#define KANAMAP_H

#include "myhader.h"

/**
 * @brief 假名映射表大小
 */
extern const int kanaMapSize;

/**
 * @brief 假名映射数组，用于字符编码转换
 * 
 * 大小为 kanaMapSize，存储不同编码对应的假名字符串。
 */
extern String kanaMap[];

/**
 * @brief 初始化假名映射表（kanaMap）
 * 
 * 将特定编码映射到对应的假名字符编码，支持多种输入编码对应相同假名。
 * 例如，将不同编码映射到「あ」行的五个假名。
 * 
 * 该函数应在程序启动时调用，确保假名映射表正确初始化。
 */
void initKanaMap();

#endif