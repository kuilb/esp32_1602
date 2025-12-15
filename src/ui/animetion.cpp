#include "./ui/animations.h"

static const uint8_t wifi0[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t wifi1[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 };
static const uint8_t wifi2[8] = { 0x00, 0x00, 0x00, 0x04, 0x0A, 0x00, 0x00, 0x04 };
static const uint8_t wifi3[8] = { 0x0E, 0x11, 0x00, 0x04, 0x0A, 0x00, 0x00, 0x04 };
static const uint8_t* wifi[] = { wifi0, wifi1, wifi2, wifi3 };
static IconAnimation wifiSearchingAnimation = {
    "wifi_searching",
    4,
    500, // 500 毫秒
    0,
    0,
    (const uint8_t**)wifi,
    false,
    false
};

static const uint8_t spn0[8] = { 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t spn1[8] = { 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t spn2[8] = { 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t spn3[8] = { 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t spn4[8] = { 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00 };
static const uint8_t spn5[8] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00 };
static const uint8_t spn6[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00 };
static const uint8_t spn7[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00 };
static const uint8_t spn8[8] = { 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00 };
static const uint8_t spn9[8] = { 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00 };
static const uint8_t spn10[8] = { 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t spn11[8] = { 0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t* spinnerFrames[] = { spn0, spn1, spn2, spn3, spn4, spn5, spn6, spn7, spn8, spn9, spn10, spn11 };
static IconAnimation spinnerAnimation = {
    "spinner",
    12,
    100, // 100 毫秒
    0,
    0,
    (const uint8_t**)spinnerFrames,
    false,
    false
};

static const uint8_t wrong0[8] = { 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00, 0x00 };
static const uint8_t wrong1[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t* wrongFrames[] = { wrong0, wrong1 };
static IconAnimation wrongAnimation = {
    "wrong",
    2,
    800, // 800 毫秒
    0,
    0,
    (const uint8_t**)wrongFrames,
    false,
    false
};

static const uint8_t clock0[8] = { 0x0E, 0x11, 0x15, 0x15, 0x13, 0x11, 0x0E, 0x00 };
static const uint8_t clock1[8] = { 0x0E, 0x11, 0x15, 0x15, 0x15, 0x13, 0x0E, 0x00 };
static const uint8_t clock2[8] = { 0x0E, 0x11, 0x15, 0x15, 0x15, 0x15, 0x0E, 0x00 };
static const uint8_t clock3[8] = { 0x0E, 0x11, 0x15, 0x15, 0x15, 0x19, 0x0E, 0x00 };
static const uint8_t clock4[8] = { 0x0E, 0x11, 0x15, 0x15, 0x19, 0x11, 0x0E, 0x00 };
static const uint8_t clock5[8] = { 0x0E, 0x11, 0x15, 0x1D, 0x11, 0x11, 0x0E, 0x00 };
static const uint8_t clock6[8] = { 0x0E, 0x19, 0x15, 0x15, 0x11, 0x11, 0x0E, 0x00 };
static const uint8_t clock7[8] = { 0x0E, 0x15, 0x15, 0x15, 0x11, 0x11, 0x0E, 0x00 };
static const uint8_t clock8[8] = { 0x0E, 0x13, 0x15, 0x15, 0x11, 0x11, 0x0E, 0x00 };
static const uint8_t clock9[8] = { 0x0E, 0x11, 0x17, 0x15, 0x11, 0x11, 0x0E, 0x00 };
static const uint8_t clock10[8] = { 0x0E, 0x11, 0x15, 0x17, 0x11, 0x11, 0x0E, 0x00 };
static const uint8_t* clockFrames [] = { clock0, clock1, clock2, clock3, clock4, clock5, clock6, clock7, clock8, clock9, clock10 };
static IconAnimation clockAnimation = {
    "clock",
    11,
    200, // 200 毫秒
    0,
    0,
    (const uint8_t**)clockFrames,
    false,
    false
};

static IconAnimation* s_anims[4] = { &wifiSearchingAnimation, &spinnerAnimation, &wrongAnimation, &clockAnimation };
static uint8_t s_animCount = 4;

namespace Animations {
    // 在此文件中实现动画相关的功能
    void registerAnimation(IconAnimation& animation) {
        // 实现注册动画的逻辑
        animation.currentFrame = 0;
        animation.lastUpdate = millis();
        animation.enabled = false;
    }

    IconAnimation* getAnimation(const char* animationName) {
        // 实现获取动画的逻辑
        if(animationName == nullptr) {
            return nullptr;
        }
        for (uint8_t i = 0; i < s_animCount; ++i) {
            if (s_anims[i]->name && strcmp(s_anims[i]->name, animationName) == 0) {
                return s_anims[i];
            }
        }
        return nullptr;
    }

    void setEnable(const char* name, bool enable) {
        IconAnimation* anim = getAnimation(name);
        if (anim) {
            anim->enabled = enable;
        }
    }

    void update() {
        unsigned long currentMillis = millis();
        for (uint8_t i = 0; i < s_animCount; ++i) {
            IconAnimation* anim = s_anims[i];
            if (anim->enabled) {
                if (currentMillis - anim->lastUpdate >= anim->frameDelay) {
                    anim->currentFrame = (anim->currentFrame + 1) % anim->frameCount;
                    anim->lastUpdate = currentMillis;
                    anim->needToUpdate = true;
                }
            }
        }
    }
    
    bool getNeedToUpdate() {
        for (uint8_t i = 0; i < s_animCount; ++i) {
            if (s_anims[i]->needToUpdate) {
                s_anims[i]->needToUpdate = false; // 重置标志
                return true;
            }
        }
        return false;
    }
}