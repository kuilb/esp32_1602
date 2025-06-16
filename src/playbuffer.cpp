#include "playbuffer.h"

unsigned long lastDisplayTime = 0;   // 上一帧显示时间戳
bool isDisplayingCache = false;      // 当前是否正在播放缓存

// 播放缓存内容
void tryDisplayCachedFrames() {
    if (frameCache.empty()) {
        isDisplayingCache = false;
        //Serial.println("播放完毕，停止缓存播放");
        return;
    }

    unsigned long now = millis();

    // 立即显示第一帧（首次触发播放）
    if (!isDisplayingCache) {
        isDisplayingCache = true;
        lastDisplayTime = now;
        processIncoming(frameCache.front().data.data(), frameCache.front().data.size());
        // Serial.println("display buffer (first)");
        frameCache.pop_front();
        return;
    }

    // 匹配帧率
    if (now - lastDisplayTime >= frameCache.front().frameIntervalMs) {
        lastDisplayTime = now;
        processIncoming(frameCache.front().data.data(), frameCache.front().data.size());
        // Serial.println("display buffer (interval)");
        frameCache.pop_front();
    }

    // 如果播放完了要清除播放状态
    if (frameCache.empty()) {
        isDisplayingCache = false;
    }
}