#include "badappleplayer.h"

const LyricLine* getLyricForFrame(int currentFrame) {
    const LyricLine* last = NULL;
    for (int i = 0; i < lyricCount; i++) {
        if (lyrics[i].frameIndex <= currentFrame) {
            last = &lyrics[i];
        } else {
            break;
        }
    }
    return last;
}

// UTF-8三字节转Unicode码
int _utf8ToUnicode(char c0, char c1, char c2) {
    return ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
}

// 转换UTF-8字符串到假名字符串
String convertUtf8ToKana(const char* utf8Str) {
    String result = "";
    int i = 0;
    int len = strlen(utf8Str);
    while (i < len) {
        uint8_t c = (uint8_t)utf8Str[i];

        if (c == 0xE3) {                // 3字节UTF-8起始
            if (i + 2 >= len) break;
            char c1 = utf8Str[i + 1];
            char c2 = utf8Str[i + 2];

            int key = _utf8ToUnicode(c, c1, c2) - 12000;
            if (key >= 0 && key < kanaMapSize && kanaMap[key] != "") {
                String kana = kanaMap[key];
                result += kana[0];
                if ((uint8_t)kana[1] == 222) {  // 如果有第二字节，加入
                    result += kana[1];
                }
            } 
            else {
                // 未知假名，替换为空格
                result += " ";
            }
            i += 3;
        } else if (c < 0x80) {
            // ASCII字符直接加入
            result += (char)c;
            i++;
        } else {
            // 其他UTF-8字节简单跳过或替换为空格
            i++;
        }
    }
    return result;
}


void processBadapple(const uint8_t* raw, int currentFrame) {
    uint8_t customCharIndex = 0;
    const LyricLine* lyric = getLyricForFrame(currentFrame);
    lcdResetCursor();

    // 上半屏：块0~3 -> 显示在 LCD 第一行前4列
    for (int block = 0; block < 4; block++) {
        uint8_t charMap[8];
        for (int row = 0; row < 8; row++) {
            charMap[row] = raw[block * 8 + row] & 0x1F;  // 5bit像素
        }
        lcdCreateChar(block, charMap);
        lcdDisCustom(block);
    }

    if (lyric != NULL) {
        String line1Converted = convertUtf8ToKana(lyric->text_line1);

        int len = line1Converted.length();
        lcdDisChar(' ');
        for (int i = 0; i < 11; i++) {
            if (i < len) {
                lcdDisChar(line1Converted[i]);
            } else {
                lcdDisChar(' '); // 自动补空格
            }
        }
    }

    // 下半屏：块4~7 -> 显示在 LCD 第二行前4列
    for (int block = 4; block < 8; block++) {
        uint8_t charMap[8];
        for (int row = 0; row < 8; row++) {
            charMap[row] = raw[block * 8 + row] & 0x1F;
        }
        lcdCreateChar(block, charMap);
        lcdDisCustom(block);
    }

    if (lyric != NULL) {
        String line2Converted = convertUtf8ToKana(lyric->text_line2);

        int len = line2Converted.length();
        lcdDisChar(' ');
        for (int i = 0; i < 11; i++) {
            if (i < len) {
                lcdDisChar(line2Converted[i]);
            } else {
                lcdDisChar(' '); // 自动补空格
            }
        }
    }
}


// 播放 Bad Apple 原始帧数据（每帧64字节，不含协议头）
void playBadAppleFromFileRaw(const char* path) {
    unsigned long lastFrameTime = millis();
    File file = SPIFFS.open(path, "r");
    if (!file) {
        Serial.println("无法打开文件");
        return;
    } else {
        Serial.println("已打开文件");
    }

    const size_t frameSize = 64;
    uint8_t frame[frameSize];
    int currentFrame = 0;
    lcd_text(" ",1);
    lcd_text(" ",2);

    while (file.available() >= frameSize) {
        unsigned long now = millis();
        if (now - lastFrameTime >= 33) {
            file.read(frame, frameSize);
            processBadapple(frame, currentFrame);

            lastFrameTime += 33;  // 累加，不用now避免抖动误差
            currentFrame++;
        }
        // 可选：yield(); 防止看门狗复位
        yield();
    }

    file.close();
    Serial.println("播放完成");
}
