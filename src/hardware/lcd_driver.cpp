#include "lcd_driver.h"
#include <esp_timer.h>

int lcdCursor = 0;  // 当前光标位置，全局变量 0~31

int brightness = 255;  // 默认亮度

uint32_t mask, value;

//触发E引脚
inline void _triggerE(){
    uint64_t start = esp_timer_get_time();
    while (esp_timer_get_time() - start < 15) {}

    GPIO.out |= (1 << LCD_E);

    start = esp_timer_get_time();
    while (esp_timer_get_time() - start < 15) {}

    GPIO.out &= ~(1 << LCD_E);

    start = esp_timer_get_time();
    while (esp_timer_get_time() - start < 15) {}
}

// 写入一个数据或指令
inline void _gpioWrite(int data,int mode){

    //设定模式(指令/字符)
    gpio_set_level((gpio_num_t)(LCD_RS), mode);

    // 前四位
    uint32_t mask = (1 << LCD_D7) | (1 << LCD_D6) | (1 << LCD_D5) | (1 << LCD_D4);
    uint32_t value = 0;
    if (data & 0x80) value |= (1 << LCD_D7);
    if (data & 0x40) value |= (1 << LCD_D6);
    if (data & 0x20) value |= (1 << LCD_D5);
    if (data & 0x10) value |= (1 << LCD_D4);
    GPIO.out = (GPIO.out & ~mask) | value;
    _triggerE();

    // 后四位
    value = 0;
    if (data & 0x08) value |= (1 << LCD_D7);
    if (data & 0x04) value |= (1 << LCD_D6);
    if (data & 0x02) value |= (1 << LCD_D5);
    if (data & 0x01) value |= (1 << LCD_D4);
    GPIO.out = (GPIO.out & ~mask) | value;
    _triggerE();
}

// 初始化 LCD 背光
inline void _initLcdBacklightPwm(uint8_t initialDuty = 255) {
    ledcSetup(LCD_BLA_PWM_CHANNEL, LCD_BLA_PWM_FREQ, LCD_BLA_PWM_RESOLUTION);
    ledcAttachPin(LCD_BLA_PWM_PIN, LCD_BLA_PWM_CHANNEL);
    ledcWrite(LCD_BLA_PWM_CHANNEL, initialDuty);        // 设置初始亮度
}

// 改变当前亮度值
void changeBrightness(int delta) {
    brightness += delta;
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    setLcdBrightness(brightness);
}

// 设置 LCD 背光亮度（0-255）
inline void setLcdBrightness(uint8_t duty) {
    if (duty > LCD_BLA_PWM_MAX_DUTY) duty = LCD_BLA_PWM_MAX_DUTY;
    ledcWrite(LCD_BLA_PWM_CHANNEL, duty);
}

// 初始化 LCD 显示模块
void lcdInit(){
    _initLcdBacklightPwm(255);           // 默认亮度 255

    _gpioWrite(0x33,CMD);               // 设置LCD进入8位模式
    delay(5);
    _gpioWrite(0x32,CMD);               // 设置LCD切换为4位模式
    delay(5);
    _gpioWrite(0x06,CMD);               // 设定向右写入字符，设置屏幕内容不滚动
    delay(5);
    _gpioWrite(0x0C,CMD);               // 开启屏幕显示，关闭光标显示，关闭光标闪烁
    delay(5);
    _gpioWrite(0x28,CMD);               // 设定数据总线为四位，显示2行字符，使用5*8字符点阵
    delay(5);
    _gpioWrite(0x01,CMD);               // 清屏并将地址指针归位
    delay(5);

    LOG_LCD_INFO("LCD initialized");
}

// UTF-8假名字符转换到LCD字符编码
String convertUTF8ToKana(const String& text) {
    String result = "";
    int i = 0;
    
    while (i < text.length()) {
        // 检查是否为UTF-8多字节字符（日语假名）
        if ((uint8_t)text[i] >= 0x80) {
            // UTF-8多字节字符处理
            if (i + 2 < text.length()) {
                // 提取3字节的UTF-8字符
                uint8_t b1 = (uint8_t)text[i];
                uint8_t b2 = (uint8_t)text[i+1];
                uint8_t b3 = (uint8_t)text[i+2];
                
                // 计算Unicode码点
                if ((b1 & 0xF0) == 0xE0) {
                    int kanaIndex = -1;

                    uint32_t codepoint = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
                    if ((codepoint >= 12353 && codepoint <= 12438) || (codepoint >= 12449 && codepoint <= 12534))
                        kanaIndex = codepoint - 12000;
                    
                    // 查找假名映射
                    if (kanaIndex >= 0 && kanaIndex < kanaMapSize && kanaMap[kanaIndex] != "") {
                        result += kanaMap[kanaIndex];
                    } else {
                        LOG_LCD_WARN("Unknown kana Unicode: " + String(codepoint));
                        result += " "; // 未找到对应假名时显示空格
                    }
                    
                    i += 3; // 跳过3字节
                } else {
                    LOG_LCD_WARN("Invalid UTF-8 sequence");
                    result += " ";
                    i++;
                }
            } else {
                result += " ";
                i++;
            }
        } else {
            // ASCII字符直接添加
            result += text[i];
            i++;
        }
    }
    
    return result;
}

// 显示函数(用于简单显示/调试，支持日语假名)
void lcdText(String ltext,int line){
    // 设置行地址
    if (line == 1)
        _gpioWrite(LCD_line1, CMD);
    else if (line == 2)
        _gpioWrite(LCD_line2, CMD);
    else
        return;     // 非法行号，直接返回

    // 转换UTF-8假名到LCD字符编码
    String convertedText = convertUTF8ToKana(ltext);
    
    int tsize = convertedText.length();
    for(int size = 0; size < 16; size++){     //逐字写入
        if(size > tsize - 1){
            _gpioWrite(0x20, CHR);       //若字符串长度小于16，则填充空格
        }
        else{
            _gpioWrite(int(convertedText[size]), CHR); //转换成RAW编码后写入
        }
    }
}

// 设置光标位置
void lcdSetCursor(int changecursor){
    // LOG_LCD_VERBOSE("set cursor in " + String(changecursor));
    lcdCursor=changecursor;
    int row = (lcdCursor < 16) ? 0 : 1;
    int col = lcdCursor % 16;

    int addr = 0;
    if (row == 0) {
        addr = 0x00 + col;
    } 
    else if (row == 1) {
        addr = 0x40 + col;
    } 
    else {
        LOG_LCD_WARN("Invalid LCD row: " + String(row));
        return; // 无效行
    }

    int command = 0x80 | addr;  // 设置 DDRAM 地址命令
    _gpioWrite(command, CMD);
}

void lcdResetCursor(){
    // LOG_LCD_VERBOSE("Reset LCD cursor");
    lcdSetCursor(0);    // 回到屏幕起点
}

// 光标向后移动一格
void _nextCursor(){
    if (lcdCursor >= 32) {
        LOG_LCD_WARN("LCD cursor out of bounds: " + String(lcdCursor));
        lcdSetCursor(33);   //将光标设置为显示区域外
    }
    lcdSetCursor(lcdCursor+1);
}

// 写入一字节的自定义字符
void lcdCreateChar(int slot, uint8_t data[8]){
    if (slot < 0 || slot > 7) return;       // 限制 slot 范围

    int cgram_addr = 0x40 | (slot << 3);    // CGRAM 写入起始地址
    _gpioWrite(cgram_addr, CMD);           // 设置 CGRAM 地址（指令模式）

    // 连续写入 8 字节点阵数据
    for (int i = 0; i < 8; i++) {
        _gpioWrite(data[i], CHR);          // 字符数据（数据模式）
    }

    // 设置回 DDRAM 当前光标对应位置
    lcdSetCursor(lcdCursor);                // 将光标设置回当前显示位置
}

// 显示自定义字符
void lcdDisCustom(int index){
    if (index < 0 || index > 7) return;  // 只能是 0~7 槽
        _gpioWrite(index, CHR);         // 写入字符数据（模式 1 表示数据模式）

    _nextCursor();
}

// 显示普通字符
void lcdDisChar(char text){             //显示函数
        _gpioWrite(int(text),CHR);     //直接写入
        _nextCursor();
}

// 连续显示整段的普通字符，不清除其他的内容，注意越界
void lcdPrint(String s) {
    for (unsigned int i = 0; i < s.length(); i++) {
        lcdDisChar(s[i]);
    }
}

// 清除光标后面单行的字符
void clearOtherChar(){   
    if(lcdCursor < 15){
        for(int i = lcdCursor; i < 16; i++){
            lcdDisChar(' ');
        }
    }
    else if(lcdCursor < 31){
        for(int i = lcdCursor; i < 32; i++){
            lcdDisChar(' ');
        }
    }
}
