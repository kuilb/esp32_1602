#include "badappleplayer.h"

const LyricLine* _getLyricForFrame(int currentFrame) {
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
String _convertUtf8ToKana(const char* utf8Str) {
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

void _processBlock(int startBlock, const uint8_t* raw){
    for (int block = startBlock; block < startBlock + 4; block++) {
        uint8_t charMap[8];
        for (int row = 0; row < 8; row++) {
            charMap[row] = raw[block * 8 + row] & 0x1F;  // 5bit像素
        }
        lcdCreateChar(block, charMap);
        lcdDisCustom(block);
    }
}

void _lyricDisplay(const LyricLine* lyric,int lineNumber){
    if (lyric != NULL) {
        String lineConverted;
        if (lineNumber == 1) {
            lineConverted = _convertUtf8ToKana(lyric->text_line1);
        }
        else if (lineNumber == 2) {
            lineConverted = _convertUtf8ToKana(lyric->text_line2);
        }
        else{
            LOG_DISPLAY_WARN("lyricDisplay: Invalid line number %d", lineNumber);
            lcdPrint("           "); // 显示空行
            return;
        }

        int len = lineConverted.length();
        lcdDisChar(' ');
        for (int i = 0; i < 11; i++) {
            if (i < len) {
                lcdDisChar(lineConverted[i]);
            } else {
                lcdDisChar(' '); // 自动补空格
            }
        }
    }
}


void _processBadapple(const uint8_t* raw, int currentFrame) {
    uint8_t customCharIndex = 0;
    const LyricLine* lyric = _getLyricForFrame(currentFrame);
    lcdResetCursor();

    // 上半屏：块0~3 -> 显示在 LCD 第一行前4列
    _processBlock(0, raw);
    _lyricDisplay(lyric,1);
    

    // 下半屏：块4~7 -> 显示在 LCD 第二行前4列
    _processBlock(4, raw);
    _lyricDisplay(lyric,2);
}


// 播放 Bad Apple 原始帧数据（每帧64字节，不含协议头）
void playBadAppleFromFileRaw(const char* path) {
    unsigned long lastFrameTime = millis();
    Serial.println("尝试打开文件: " + String(path));
    
    // 检查文件是否存在
    if (!SPIFFS.exists(path)) {
        LOG_SYSTEM_WARN("Bad Apple file not found: %s", path);
        lcd_text("File Not Found",1);
        lcd_text(" ",2);
        delay(1000);
        return;
    }
    
    File file = SPIFFS.open(path, "r");
    if (!file) {
        LOG_SYSTEM_WARN("Failed to open Bad Apple file: %s", path);
        lcd_text("Err Open File",1);
        lcd_text(" ",2);
        delay(1000);
        return;
    }

    LOG_SYSTEM_INFO("Playing Bad Apple from file: %s, size: %d bytes", path, file.size());

    const size_t frameSize = 64;
    uint8_t frame[frameSize];
    int currentFrame = 0;
    lcd_text(" ",1);
    lcd_text(" ",2);

    while (file.available() >= frameSize) {
        if(buttonJustPressed[CENTER] && currentFrame >= 15){
            currentState = STATE_MENU;
            break;
        }

        unsigned long now = millis();
        
        // 快退：每次快退20帧
        if(buttonJustPressed[LEFT]){
            currentFrame = max(0, currentFrame - 20);
            file.seek(currentFrame * frameSize, SeekSet);
            file.read(frame, frameSize);
            _processBadapple(frame, currentFrame);
            lastFrameTime = now;
        }
        // 快进：每次快进10帧
        else if(buttonJustPressed[RIGHT]){
            currentFrame = min((int)(file.size() / frameSize) - 1, currentFrame + 10);
            file.seek(currentFrame * frameSize, SeekSet);
            file.read(frame, frameSize);
            _processBadapple(frame, currentFrame);
            lastFrameTime = now;
        }
        else if (now - lastFrameTime >= 33) {
            file.read(frame, frameSize);
            _processBadapple(frame, currentFrame);

            lastFrameTime += 33;  // 累加，不用now避免抖动误差
            currentFrame++;
        }
    }

    file.close();
    LOG_DISPLAY_INFO("Bad Apple playback finished");
}
