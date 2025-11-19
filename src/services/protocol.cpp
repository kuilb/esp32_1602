#include "protocol.h"
#include "esp_task_wdt.h"

static unsigned int frameCount = 0;
static unsigned int lastSecond = 0;
static float currentFPS = 0.0f;

// 解码函数
void processIncoming(const uint8_t* raw, unsigned int fullLen) {
    if (inMenuMode) return;
    
    LOG_DISPLAY_VERBOSE("Processing incoming data frame of length " + String(fullLen) + " bytes");

    // LOG_DISPLAY_VERBOSE("Raw bytes: ");
    // for (unsigned int i = 0; i < fullLen; i++) {
    //     LOG_DISPLAY_VERBOSE("0x" + String(raw[i], HEX));
    //     LOG_DISPLAY_VERBOSE(" ");
    // }

    // 帧率统计
    uint32_t now = millis();
    uint32_t currentSecond = now / 1000;

    frameCount++;

    if (currentSecond != lastSecond) {
        currentFPS = frameCount * 1000.0f / (now - lastSecond * 1000.0f);
        frameCount = 0;
        lastSecond = currentSecond;

        // 打印 FPS
        Serial.println("FPS:"+ (String)currentFPS);
    }

    if (fullLen < 5) {  // 头(2) + 长度(1) + 帧率(2) + 最少1字节数据
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
    // Serial.print("帧率 (ms): ");
    // Serial.println(frameInterval);

    // 初始化显示
    lcdResetCursor();

    unsigned int i = 5;          // 从数据体开始
    uint8_t customCharIndex = 0; // 自定义字符编号（0~7）
    String charBuffer = "";      // 用于批量显示普通字符
    
    // 添加安全计数器，防止无限循环
    unsigned int loopCounter = 0;
    const unsigned int maxLoops = 1000; // 最大循环次数限制

    while (i < fullLen && lcdCursor < 32 && loopCounter < maxLoops) {
        loopCounter++; // 增加循环计数
        
        // 每处理10个字符就喂一次狗
        if (loopCounter % 10 == 0) {
            esp_task_wdt_reset();
        }
        uint8_t flag = raw[i++];
        if (flag == 0x00) {
            if (i >= fullLen) break;
            char c = raw[i++];
            if ((uint8_t)c != 0xE3) {
                // 普通 ASCII 字符
                charBuffer += c;
            } 
            else if((uint8_t)c == 0xE3){
                bool unknowKana = false;
                // 添加边界检查
                if (i + 2 >= fullLen) {
                    Serial.println("UTF-8数据不完整");
                    break;
                }
                
                char c1 = raw[i + 1];
                char c2 = raw[i + 2];
                i += 2; // 正确更新索引
                
                int key = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F) - 12000;
                
                // 添加key范围检查
                if (key < 0 || key >= kanaMapSize) {
                    unknowKana = true;
                    charBuffer += " ";
                } else if(kanaMap[key] == ""){   // 未定义假名，显示空格
                    unknowKana = true;
                    charBuffer += " ";
                }
                
                if(!unknowKana){
                    String kana = kanaMap[key];
                    charBuffer += kana[0];
                    if (kana.length() > 1 && (uint8_t)kana[1] == 222) {
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
                lcdPrint(charBuffer);
                charBuffer = "";
                //Serial.print("显示buffer耗时: ");
                //Serial.println(millis() - t0_dis_char);
            }

            // 添加边界检查
            if (i + 8 > fullLen) {
                Serial.println("自定义字符数据不足");
                break;
            }

            uint8_t charMap[8]; // 8行点阵
            for (int j = 0; j < 8; j++) {
                charMap[j] = raw[i++];
            }
            
            // 显示点阵
            lcdCreateChar(customCharIndex, charMap);
            lcdDisCustom(customCharIndex);

            // 循环使用 slot
            customCharIndex = (customCharIndex + 1) % 8;
        } 

        else {
            Serial.print("未知标志: ");
            Serial.println(flag, HEX);
            break;
        }
    }

    // 检查是否因为安全限制退出循环
    if (loopCounter >= maxLoops) {
        Serial.println("警告: 处理循环达到安全限制，可能存在数据异常");
    }

    // 输出剩余字符
    if (charBuffer.length() > 0) {
        lcdPrint(charBuffer);
    }

    // 填充空格至满屏
    while (lcdCursor < 32) {
        lcdDisChar(' ');
    }

    // Serial.print("Process end:");
    // Serial.println(millis());
}