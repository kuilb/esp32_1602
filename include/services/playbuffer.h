#ifndef PLAYBUFFER_H
#define PLAYBUFFER_H

#include "myheader.h"
#include "protocol.h"
#include "network.h"

/**
 * @brief 帧数据结构体，包含帧内容和帧间隔时间（毫秒）
 */
struct FrameData {
    std::vector<uint8_t> data;       /**< 帧的原始数据 */
    uint16_t frameIntervalMs;        /**< 两帧之间的间隔时间，单位：毫秒 */
};

/**
 * @brief 上一帧显示时间戳，单位为毫秒
 */
extern unsigned long lastDisplayTime;


/**
 * @brief 声明全局变量，指示是否正在播放缓存帧
 */
extern bool isDisplayingCache;

/**
 * @brief 声明帧缓存队列，用于存储待播放的帧数据
 */
extern std::deque<FrameData> frameCache;


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