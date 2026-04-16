#include "./applications/badappleplayer.h"
#include "hardware/buzzer.h"
#include <cstring>

static const uint8_t BADAPPLE_BUZZER_VOLUME = 35;
static const int64_t kBadAppleFrameDurationUs = 33333;

struct BadAppleRuntimeState {
    bool active = false;
    File file;
    int totalFrames = 0;
    int currentFrame = 0;
    int melodyIndex = 0;
    int64_t startTime = 0;
    int64_t nextFrameTime = 0;
    int64_t nextNoteTime = 0;
    uint16_t currentFreq = 0;
    uint16_t currentDur = 0;
    uint32_t melodyCumulativeMs[4096] = {0};
    uint32_t frameTimestampMs[8192] = {0};
};

static BadAppleRuntimeState s_badapple;

static void _resetBadAppleRuntimeState() {
    s_badapple.active = false;
    s_badapple.file = File();
    s_badapple.totalFrames = 0;
    s_badapple.currentFrame = 0;
    s_badapple.melodyIndex = 0;
    s_badapple.startTime = 0;
    s_badapple.nextFrameTime = 0;
    s_badapple.nextNoteTime = 0;
    s_badapple.currentFreq = 0;
    s_badapple.currentDur = 0;
    std::memset(s_badapple.melodyCumulativeMs, 0, sizeof(s_badapple.melodyCumulativeMs));
    std::memset(s_badapple.frameTimestampMs, 0, sizeof(s_badapple.frameTimestampMs));
}

static void _stopBadApplePlayback(bool completed) {
    if (s_badapple.file) {
        s_badapple.file.close();
    }
    buzzerNoTone();
    const int64_t endTime = esp_timer_get_time();
    if (s_badapple.startTime > 0) {
        const int64_t totalUs = endTime - s_badapple.startTime;
        LOG_DISPLAY_INFO("Bad Apple playback %s. Total time: %lld us (%.2f s)",
            completed ? "finished" : "stopped",
            totalUs,
            totalUs / 1000000.0f);
    }
    _resetBadAppleRuntimeState();
    exitAppInterface(FIRST_TIME_DELAY);
}

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

void enterBadAppleInterface() {
    // 统一应用入口：非联网、非阻塞状态机。
    enterAppInterface(handleBadAppleInterface, false);

    if (!SPIFFS.exists("/badapple.bin")) {
        LOG_SYSTEM_WARN("Bad Apple file not found: /badapple.bin");
        lcdText("File Not Found", 1);
        lcdText(" ", 2);
        exitAppInterface(FIRST_TIME_DELAY);
        return;
    }

    _resetBadAppleRuntimeState();
    s_badapple.file = SPIFFS.open("/badapple.bin", "r");
    if (!s_badapple.file) {
        LOG_SYSTEM_WARN("Failed to open Bad Apple file: /badapple.bin");
        lcdText("Err Open File", 1);
        lcdText(" ", 2);
        exitAppInterface(FIRST_TIME_DELAY);
        return;
    }

    s_badapple.totalFrames = static_cast<int>(s_badapple.file.size() / 64);
    for (int i = 1; i < badAppleMelodyLength; ++i) {
        s_badapple.melodyCumulativeMs[i] = s_badapple.melodyCumulativeMs[i - 1]
            + pgm_read_word(&badAppleMelodyDurations[i - 1]);
    }
    for (int i = 0; i < s_badapple.totalFrames; ++i) {
        s_badapple.frameTimestampMs[i] = static_cast<uint32_t>(i * kBadAppleFrameDurationUs / 1000);
    }

    s_badapple.startTime = esp_timer_get_time();
    s_badapple.nextFrameTime = s_badapple.startTime;
    s_badapple.melodyIndex = 0;
    if (badAppleMelodyLength > 0) {
        s_badapple.currentFreq = pgm_read_word(&badAppleMelodyFrequencies[0]);
        s_badapple.currentDur = pgm_read_word(&badAppleMelodyDurations[0]);
        s_badapple.nextNoteTime = s_badapple.startTime + static_cast<int64_t>(s_badapple.currentDur) * 1000;
        _setBadAppleTone(s_badapple.currentFreq);
    }
    s_badapple.active = true;

    lcdText(" ", 1);
    lcdText(" ", 2);
    LOG_SYSTEM_INFO("Bad Apple state-machine started: frames=%d melody=%d",
        s_badapple.totalFrames,
        badAppleMelodyLength);
}

void handleBadAppleInterface() {
    if (!s_badapple.active) {
        return;
    }

    int64_t now = esp_timer_get_time();

    while (now >= s_badapple.nextNoteTime && s_badapple.melodyIndex < badAppleMelodyLength - 1) {
        s_badapple.melodyIndex++;
        s_badapple.currentFreq = pgm_read_word(&badAppleMelodyFrequencies[s_badapple.melodyIndex]);
        s_badapple.currentDur = pgm_read_word(&badAppleMelodyDurations[s_badapple.melodyIndex]);
        s_badapple.nextNoteTime += static_cast<int64_t>(s_badapple.currentDur) * 1000;
        _setBadAppleTone(s_badapple.currentFreq);
    }

    if (buttonJustPressed[CENTER] && s_badapple.currentFrame >= 15) {
        _stopBadApplePlayback(false);
        return;
    }

    bool seeked = false;
    int seekFrame = s_badapple.currentFrame;
    if (buttonJustPressed[LEFT]) {
        seekFrame = max(0, s_badapple.currentFrame - 20);
        seeked = true;
    } else if (buttonJustPressed[RIGHT]) {
        seekFrame = min(s_badapple.totalFrames - 1, s_badapple.currentFrame + 10);
        seeked = true;
    }

    if (seeked) {
        s_badapple.currentFrame = seekFrame;
        s_badapple.file.seek(static_cast<size_t>(s_badapple.currentFrame) * 64, SeekSet);
        s_badapple.nextFrameTime = now;

        uint32_t targetMs = s_badapple.frameTimestampMs[s_badapple.currentFrame];
        int newMelodyIdx = 0;
        while (newMelodyIdx < badAppleMelodyLength - 1
            && s_badapple.melodyCumulativeMs[newMelodyIdx + 1] <= targetMs) {
            ++newMelodyIdx;
        }
        s_badapple.melodyIndex = newMelodyIdx;
        s_badapple.currentFreq = pgm_read_word(&badAppleMelodyFrequencies[s_badapple.melodyIndex]);
        s_badapple.currentDur = pgm_read_word(&badAppleMelodyDurations[s_badapple.melodyIndex]);
        uint32_t melodyElapsedMs = s_badapple.melodyCumulativeMs[s_badapple.melodyIndex];
        uint32_t deltaMs = (targetMs > melodyElapsedMs) ? (targetMs - melodyElapsedMs) : 0;
        uint32_t remainMs = (s_badapple.currentDur > deltaMs) ? (s_badapple.currentDur - deltaMs) : 1;
        s_badapple.nextNoteTime = now + static_cast<int64_t>(remainMs) * 1000;
        _setBadAppleTone(s_badapple.currentFreq);
    }

    if (now >= s_badapple.nextFrameTime) {
        const size_t frameSize = 64;
        if (s_badapple.file.available() >= static_cast<int>(frameSize)
            && s_badapple.melodyIndex < badAppleMelodyLength) {
            uint8_t frame[frameSize];
            s_badapple.file.read(frame, frameSize);
            _processBadapple(frame, s_badapple.currentFrame);
            s_badapple.currentFrame++;
            s_badapple.nextFrameTime += kBadAppleFrameDurationUs;
        } else {
            _stopBadApplePlayback(true);
            return;
        }
    }
}
