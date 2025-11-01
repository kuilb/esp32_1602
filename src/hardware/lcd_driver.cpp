#include "lcd_driver.h"

int lcdCursor = 0;  // 当前光标位置，全局变量 0~31

int brightness = 255;  // 默认亮度

//触发E引脚
void _trigger_E(){                     
    delayMicroseconds(15);
    gpio_set_level((gpio_num_t)(LCD_E), 1);
    delayMicroseconds(15);
    gpio_set_level((gpio_num_t)(LCD_E), 0);
    delayMicroseconds(15);
}

// 写入一个数据或指令
void _gpio_write(int data,int mode){

    //设定模式(指令/字符)
    gpio_set_level((gpio_num_t)(LCD_RS), mode);

     // 前四位
    gpio_set_level((gpio_num_t)LCD_D7, data & 0x80);
    gpio_set_level((gpio_num_t)LCD_D6, data & 0x40);
    gpio_set_level((gpio_num_t)LCD_D5, data & 0x20);
    gpio_set_level((gpio_num_t)LCD_D4, data & 0x10);
    _trigger_E();

    // 后四位
    gpio_set_level((gpio_num_t)LCD_D7, data & 0x08);
    gpio_set_level((gpio_num_t)LCD_D6, data & 0x04);
    gpio_set_level((gpio_num_t)LCD_D5, data & 0x02);
    gpio_set_level((gpio_num_t)LCD_D4, data & 0x01);
    _trigger_E();
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
void lcd_init(){
    _initLcdBacklightPwm(255);           // 默认亮度 255

    _gpio_write(0x33,CMD);               // 设置LCD进入8位模式
    delay(5);
    _gpio_write(0x32,CMD);               // 设置LCD切换为4位模式
    delay(5);
    _gpio_write(0x06,CMD);               // 设定向右写入字符，设置屏幕内容不滚动
    delay(5);
    _gpio_write(0x0C,CMD);               // 开启屏幕显示，关闭光标显示，关闭光标闪烁
    delay(5);
    _gpio_write(0x28,CMD);               // 设定数据总线为四位，显示2行字符，使用5*8字符点阵
    delay(5);
    _gpio_write(0x01,CMD);               // 清屏并将地址指针归位
    delay(5);
}

// 显示函数(用于简单显示/调试)
void lcd_text(String ltext,int line){
    // 设置行地址
    if (line == 1)
        _gpio_write(LCD_line1, CMD);
    else if (line == 2)
        _gpio_write(LCD_line2, CMD);
    else
        return;     // 非法行号，直接返回

    int tsize=ltext.length();
    for(int size=0;size<16;size++){     //逐字写入
        if(size>tsize-1){
            _gpio_write(0x20,CHR);       //若字符串长度小于16，则填充空格
        }
        else{
            _gpio_write(int(ltext[size]),CHR); //转换成RAW编码后写入
        }
    }
}

// 设置光标位置
void lcdSetCursor(int changecursor){
    //Serial.print("set cursor in "+String(col)+","+String(row) + " ");
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
        return; // 无效行
    }

    int command = 0x80 | addr;  // 设置 DDRAM 地址命令
    _gpio_write(command, CMD);
}

void lcdResetCursor(){
    lcdSetCursor(0);    // 回到屏幕起点
}

// 光标向后移动一格
void _nextCursor(){
    if (lcdCursor >= 32) {
        lcdSetCursor(33);   //将光标设置为显示区域外
    }
    lcdSetCursor(lcdCursor+1);
}

// 写入一字节的自定义字符
void lcdCreateChar(int slot, uint8_t data[8]){
    if (slot < 0 || slot > 7) return;       // 限制 slot 范围

    int cgram_addr = 0x40 | (slot << 3);    // CGRAM 写入起始地址
    _gpio_write(cgram_addr, CMD);           // 设置 CGRAM 地址（指令模式）

    // 连续写入 8 字节点阵数据
    for (int i = 0; i < 8; i++) {
        _gpio_write(data[i], CHR);          // 字符数据（数据模式）
    }

    // 设置回 DDRAM 当前光标对应位置
    lcdSetCursor(lcdCursor);                // 将光标设置回当前显示位置
}

// 显示自定义字符
void lcdDisCustom(int index){
    if (index < 0 || index > 7) return;  // 只能是 0~7 槽
        _gpio_write(index, CHR);         // 写入字符数据（模式 1 表示数据模式）

    _nextCursor();
}

// 显示普通字符
void lcdDisChar(char text){             //显示函数
        _gpio_write(int(text),CHR);     //直接写入
        _nextCursor();
}

// 显示整段的普通字符
void lcdPrint(String s) {
    for (unsigned int i = 0; i < s.length(); i++) {
        lcdDisChar(s[i]);
    }
}
