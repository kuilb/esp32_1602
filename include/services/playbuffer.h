#ifndef PLAYBUFFER_H
#define PLAYBUFFER_H

#include "./services/protocol.h"
#include "./connectivity/network.h"

/**
 * @brief 上一帧显示时间戳，单位为毫秒
 */
extern unsigned long lastDisplayTime;


/**
 * @brief 声明全局变量，指示是否正在播放缓存帧
 */
extern bool isDisplayingCache;

// frameCache / FramePacket 在 network.h 中声明


/**
 * @brief 尝试播放缓存的帧数据，根据帧率间隔控制显示
 * 
 * @details
 * - 如果缓存为空，停止缓存播放并退出
 * - 首次调用立即显示第一帧，并开始计时
 * - 之后根据帧率间隔定时显示下一帧
 * - 播放完毕后重置播放状态
 * 
 * @note
 * - 使用 processIncoming() 处理具体帧数据的显示
 */
void tryDisplayCachedFrames();

#endif