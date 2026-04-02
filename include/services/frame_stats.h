#ifndef FRAME_STATS_H
#define FRAME_STATS_H

#include <stdint.h>

// 统计“帧”而非 TCP 包：
// - enqueued: 成功进入 frameCache 的帧数
// - dropped: 因缓存策略/过期等原因被丢弃的帧数
extern volatile uint32_t gFramesEnqueued;
extern volatile uint32_t gFramesDropped;

#endif
