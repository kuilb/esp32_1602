#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>

struct IconAnimation {
    const char* name;
    int frameCount;
    int frameDelay;             // 毫秒
    int currentFrame;
    unsigned long lastUpdate;   // 上次更新时间戳
    const uint8_t** frames;     // 帧数据指针数组
    bool enabled;
    bool needToUpdate;
};

namespace Animations {
    void registerAnimation(IconAnimation& animation);
    IconAnimation* getAnimation(const char* animationName);
    void setEnable(const char* name, bool enable);
    void update();
    bool getNeedToUpdate();
}

#endif // ANIMATIONS_H