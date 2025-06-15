#ifndef PLAYBUFFER_H
#define PLAYBUFFER_H

#include "myhader.h"
#include "protocol.h"
#include "network.h"

// 帧结构体
struct FrameData {
    std::vector<uint8_t> data;
    uint16_t frameIntervalMs;
};

// 外部变量声明
extern bool isDisplayingCache;
extern std::deque<FrameData> frameCache;

// 播放函数声明
void tryDisplayCachedFrames();

// 回调函数（必须由主程序实现）
void prosessIncoming(const uint8_t* data, size_t len);

#endif