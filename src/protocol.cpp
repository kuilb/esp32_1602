#include "protocol.h"

// 假名unicode转换
int utf8ToUnicode(uint8_t c0, uint8_t c1, uint8_t c2) {
  return ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
}

// 解码函数
void prosessIncoming(const uint8_t* raw, unsigned int bodyLen) {
    // Serial.print("\nProcess start:");
    // Serial.println(millis());

    // Serial.print("Raw bytes: ");
    // for (unsigned int i = 0; i < bodyLen; i++) {
    //     Serial.print("0x");
    //     Serial.print(raw[i], HEX);
    //     Serial.print(" ");
    // }
    // Serial.println();

    if (bodyLen < 5) {  // 头(2) + 长度(1) + 帧率(2) + 最少1字节数据
        Serial.println("数据包太短，无法解析");
        return;
    }

    // 检查协议头
    if (raw[0] != 0xAA || raw[1] != 0x55) {
        Serial.println("无效数据头");
        return;
    }

    // 读取帧率字段（两字节，高字节先发）
    uint16_t frameInterval = (raw[3] << 8) | raw[4];
    Serial.print("帧率 (ms): ");
    Serial.println(frameInterval);

    // 初始化显示
    lcd_setCursor(0, 0);    // 回到屏幕起点
    lcdCursor = 0;          // 重置全局光标

    unsigned int i = 5;          // 从数据体开始
    uint8_t customCharIndex = 0; // 自定义字符编号（0~7）
    String charBuffer = "";      // 用于批量显示普通字符

    while (i < bodyLen && lcdCursor < 32) {
        uint8_t flag = raw[i++];
        if (flag == 0x00) {
            if (i >= bodyLen) break;
            char c = raw[i++];
            if ((uint8_t)c != 0xE3) {
                // 普通 ASCII 字符
                charBuffer += c;
            } 
            else if((uint8_t)c == 0xE3){
                bool unknowKana = false;
                if (i + 1 >= bodyLen) break;
                char c1 = raw[++i];
                char c2 = raw[i+=2];
                int key = utf8ToUnicode(c, c1, c2) - 12000;
                //Serial.println(key);
                //Serial.println(kanaMap[key]);

                if(kanaMap[key] == ""){   // 未定义假名，显示空格
                    unknowKana = true;
                    charBuffer += " ";
                }
                if(!unknowKana){
                    String kana = kanaMap[key];
                    charBuffer += kana[0];
                    if ((uint8_t)kana[1] == 222) {
                        charBuffer += kana[1];
                    }
                }
                i++;
            }
            else{   // 未定义utf-8字符，显示空格
                charBuffer += " ";
            }
        } 

        // 自定义字符
        else if (flag == 0x01) {
            // 输出缓存中的普通字符
            if (charBuffer.length() > 0) {
                //uint32_t t0_dis_char = millis();
                lcd_print(charBuffer);
                charBuffer = "";
                //Serial.print("显示buffer耗时: ");
                //Serial.println(millis() - t0_dis_char);
            }

            if (i + 7 >= bodyLen) {
                Serial.println("自定义字符数据不足");
                break;
            }

            uint8_t charMap[8]; // 8行点阵
            for (int j = 0; j < 8; j++) {
                charMap[j] = raw[i++];
            }
            
            // 显示点阵
            lcd_createChar(customCharIndex, charMap);
            lcd_dis_costom(customCharIndex);
            lcd_next_cousor();

            // 循环使用 slot
            customCharIndex = (customCharIndex + 1) % 8;
        } 

        else {
            Serial.print("未知标志: ");
            Serial.println(flag, HEX);
            break;
        }
    }

    // 输出剩余字符
    if (charBuffer.length() > 0) {
        lcd_print(charBuffer);
    }

    // 填充空格至满屏
    while (lcdCursor < 32) {
        lcd_dis_chr(' ');
    }

    // Serial.print("Process end:");
    // Serial.println(millis());
}