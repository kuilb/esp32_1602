#include <unity.h>
#include <Arduino.h>
#include "../../include/hardware/lcd_driver.h"
#include "../../include/ui/animations.h"
#include "../common/test_init.h"  // 引入共享的测试初始化

void setup() {
	// 通用测试环境初始化（串口、SPIFFS、LCD等，如需可在此添加）
    globalTestSetup();  // 全局初始化 - 只在开始时调用一次
}

void loop() {
	// Unity 测试通常在 setup 中一次性运行完毕
    lcdText("    Anim test",1);
    auto wifiAnim = Animations::getAnimation("wifi_searching");
    Animations::registerAnimation(*wifiAnim);
    Animations::setEnable("wifi_searching", true);

    auto spnanim = Animations::getAnimation("spinner");
    Animations::registerAnimation(*spnanim);
    Animations::setEnable("spinner", true);

    auto wrongAnim = Animations::getAnimation("wrong");
    Animations::registerAnimation(*wrongAnim);
    Animations::setEnable("wrong", true);

    auto clockAnim = Animations::getAnimation("clock");
    Animations::registerAnimation(*clockAnim);
    Animations::setEnable("clock", true);
    
    while(1) {
        Animations::update();
        if(!Animations::getNeedToUpdate()) {
            lcdCreateChar(0, wifiAnim->frames[wifiAnim->currentFrame]);
            lcdCreateChar(1, spnanim->frames[spnanim->currentFrame]);
            lcdCreateChar(2, wrongAnim->frames[wrongAnim->currentFrame]);
            lcdCreateChar(3, clockAnim->frames[clockAnim->currentFrame]);
            lcdSetCursor(0);
            lcdDisCustom(0);
            lcdDisCustom(1);
            lcdDisCustom(2);
            lcdDisCustom(3);
            continue;
        }
        delay(1);
    }
}

