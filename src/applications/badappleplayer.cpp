#include "./applications/badappleplayer.h"
#include "hardware/buzzer.h"

static const uint8_t BADAPPLE_BUZZER_VOLUME = 35;

static inline void _setBadAppleTone(uint16_t frequency) {
    if (frequency > 0) {
        buzzerTone(frequency, BADAPPLE_BUZZER_VOLUME);
    } else {
        buzzerNoTone();
    }
}

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
    Serial.println("尝试打开文件: " + String(path));
    
    // 检查文件是否存在
    if (!SPIFFS.exists(path)) {
        LOG_SYSTEM_WARN("Bad Apple file not found: %s", path);
        lcdText("File Not Found",1);
        lcdText(" ",2);
        delay(1000);
        return;
    }
    
    File file = SPIFFS.open(path, "r");
    if (!file) {
        LOG_SYSTEM_WARN("Failed to open Bad Apple file: %s", path);
        lcdText("Err Open File",1);
        lcdText(" ",2);
        delay(1000);
        return;
    }

    LOG_SYSTEM_INFO("Playing Bad Apple from file: %s, size: %d bytes", path, file.size());
    int totalFrames = (int)(file.size() / 64);
    LOG_SYSTEM_INFO("Melody length: %d, Frame count: %d", badAppleMelodyLength, totalFrames);

    // 计算音频总时长（ms）
    uint64_t audioTotalMs = 0;
    for (int i = 0; i < badAppleMelodyLength; ++i) {
        audioTotalMs += pgm_read_word(&badAppleMelodyDurations[i]);
    }
    float audioTotalSec = audioTotalMs / 1000.0f;

    // 计算画面总时长（ms）
    const int64_t frameDurationUs = 33333; // 1000000 / 30
    float videoTotalMs = totalFrames * frameDurationUs / 1000.0f;
    float videoTotalSec = videoTotalMs / 1000.0f;

    LOG_SYSTEM_INFO("Audio total: %llu ms (%.2f s), Video total: %.0f ms (%.2f s)", audioTotalMs, audioTotalSec, videoTotalMs, videoTotalSec);

    // 预计算每个音符的累计起始时间（ms）
    static uint32_t melodyCumulativeMs[4096]; // 足够大
    melodyCumulativeMs[0] = 0;
    for (int i = 1; i < badAppleMelodyLength; ++i) {
        melodyCumulativeMs[i] = melodyCumulativeMs[i-1] + pgm_read_word(&badAppleMelodyDurations[i-1]);
    }

    // 计算每帧的理论时间戳（ms）
    static uint32_t frameTimestampMs[8192]; // 足够大
    for (int i = 0; i < totalFrames; ++i) {
        frameTimestampMs[i] = (uint32_t)(i * frameDurationUs / 1000);
    }

    const size_t frameSize = 64;
    uint8_t frame[frameSize];
    int currentFrame = 0;
    lcdText(" ",1);
    lcdText(" ",2);

    // 使用微秒级计时器进行精确控制 (30 FPS)
    int64_t startTime = esp_timer_get_time();
    int64_t nextFrameTime = startTime;

    // 音符同步推进
    int melodyIndex = 0;
    int64_t now = startTime;
    int64_t nextNoteTime = now;
    uint16_t currentFreq = 0;
    uint16_t currentDur = 0;
    // 初始化第一音符
    if (melodyIndex < badAppleMelodyLength) {
        currentFreq = pgm_read_word(&badAppleMelodyFrequencies[melodyIndex]);
        currentDur = pgm_read_word(&badAppleMelodyDurations[melodyIndex]);
        nextNoteTime = now + currentDur * 1000;
        _setBadAppleTone(currentFreq);
    }

    while (file.available() >= frameSize && melodyIndex < badAppleMelodyLength) {
        now = esp_timer_get_time();

        // 调试信息：当前帧、音符索引、计时器
        if (currentFrame % 10 == 0 || melodyIndex % 10 == 0) {
            LOG_SYSTEM_DEBUG("Frame: %d/%d, Melody: %d/%d, Timer(us): %lld", currentFrame, (int)(file.size() / frameSize), melodyIndex, badAppleMelodyLength, now);
        }

        // 音符推进
        if (now >= nextNoteTime && melodyIndex < badAppleMelodyLength - 1) {
            melodyIndex++;
            currentFreq = pgm_read_word(&badAppleMelodyFrequencies[melodyIndex]);
            currentDur = pgm_read_word(&badAppleMelodyDurations[melodyIndex]);
            nextNoteTime += currentDur * 1000;
            _setBadAppleTone(currentFreq);
        }

        // 画面推进
        if (now >= nextFrameTime) {
            // 读取并显示当前帧
            file.read(frame, frameSize);
            _processBadapple(frame, currentFrame);
            currentFrame++;
            nextFrameTime += frameDurationUs;
        } else {
            // 等待下一个事件
            int64_t waitUs = std::min(nextFrameTime, nextNoteTime) - now;
            if (waitUs > 2000) {
                vTaskDelay(pdMS_TO_TICKS(waitUs / 1000));
            } else if (waitUs > 0) {
                delayMicroseconds(waitUs);
            }
        }

        // 退出条件
        if(buttonJustPressed[CENTER] && currentFrame >= 15){
            clearCurrentInterface();
            buzzerNoTone();
            break;
        }
        // 快退/快进
        bool seeked = false;
        int seekFrame = currentFrame;
        if(buttonJustPressed[LEFT]){
            seekFrame = max(0, currentFrame - 20);
            seeked = true;
        } else if(buttonJustPressed[RIGHT]){
            seekFrame = min((int)(file.size() / frameSize) - 1, currentFrame + 10);
            seeked = true;
        }
        if (seeked) {
            currentFrame = seekFrame;
            file.seek(currentFrame * frameSize, SeekSet);
            nextFrameTime = esp_timer_get_time();
            // 精准音符同步：找到累计音符时长 >= 当前帧理论时间戳的第一个音符
            uint32_t targetMs = frameTimestampMs[currentFrame];
            int newMelodyIdx = 0;
            while (newMelodyIdx < badAppleMelodyLength-1 && melodyCumulativeMs[newMelodyIdx+1] <= targetMs) {
                ++newMelodyIdx;
            }
            melodyIndex = newMelodyIdx;
            currentFreq = pgm_read_word(&badAppleMelodyFrequencies[melodyIndex]);
            currentDur = pgm_read_word(&badAppleMelodyDurations[melodyIndex]);
            now = esp_timer_get_time();
            // 计算下一个音符的绝对时间戳
            uint32_t melodyElapsedMs = melodyCumulativeMs[melodyIndex];
            nextNoteTime = now + (currentDur - (targetMs - melodyElapsedMs)) * 1000;
            _setBadAppleTone(currentFreq);
        }
    }

    int64_t endTime = esp_timer_get_time();
    file.close();
    buzzerNoTone();
    int64_t totalUs = endTime - startTime;
    float totalSec = totalUs / 1000000.0f;
    LOG_DISPLAY_INFO("Bad Apple playback finished. Total time: %lld us (%.2f s)", totalUs, totalSec);
}
