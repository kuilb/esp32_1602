#include "./hardware/lcd_driver.h"

#include <cstring>
#include <Arduino.h>
#include "esp_rom_sys.h"

inline void _gpioWrite(int data, int mode); // 前置声明
int lcdCursor = 0;  // 当前光标位置，全局变量 0~31

int brightness = 0;  // 默认亮度

int contrast = 128;  // 默认对比度

// 自定义字符槽位自动管理
static int currentCharSlot = 0;     // 当前自动分配的槽位 (0~7)

static constexpr uint8_t LCD_DDRAM_SIZE = 32;
static constexpr uint8_t LCD_CGRAM_SLOTS = 8;
static constexpr uint8_t LCD_CGRAM_ROWS = 8;

// ==============================
// 差分刷新缓存
// ==============================
typedef struct LcdFrameState {
    bool valid;
    uint8_t ddram[LCD_DDRAM_SIZE];
    uint8_t cgram[LCD_CGRAM_SLOTS][LCD_CGRAM_ROWS];
    bool cgramUsed[LCD_CGRAM_SLOTS];
} LcdFrameState;

// 当前硬件状态（上次成功渲染的状态）
static LcdFrameState s_hwFrame = {
    false,
    {
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
    },
    {{0}},
    {false}
};

// 待渲染的目标状态（由上层调用 lcdRenderDiff 更新）
static LcdFrameState s_pendingFrame = {
    false,
    {
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
    },
    {{0}},
    {false}
};

// 叠加显示时的备份状态（用于保存被叠加覆盖的内容，以便恢复）
static LcdFrameState s_overlayBackupFrame = {
    false,
    {
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
    },
    {{0}},
    {false}
};
static int s_overlayBackupCursor = 0;
static bool s_overlayBackupValid = false;

static inline void _writeCgramSlot(uint8_t slot, const uint8_t data[LCD_CGRAM_ROWS]) {
    if (slot >= LCD_CGRAM_SLOTS || data == nullptr) return;

    int cgramAddr = 0x40 | (slot << 3);
    _gpioWrite(cgramAddr, CMD);
    for (uint8_t i = 0; i < LCD_CGRAM_ROWS; ++i) {
        _gpioWrite(data[i], CHR);
    }
}

static inline bool _isCustomCode(uint8_t code) {
    return code <= 7;
}

static inline void _setHardwareCursor(uint8_t pos) {
    if (pos >= LCD_DDRAM_SIZE) return;

    const uint8_t col = pos % 16;
    const uint8_t rowBase = (pos < 16) ? 0x00 : 0x40;
    _gpioWrite(0x80 | (rowBase + col), CMD);
}

static inline void _writeCharAt(uint8_t pos, uint8_t code) {
    if (pos >= 32) return;
    _setHardwareCursor(pos);
    _gpioWrite(static_cast<int>(code), CHR);
}

static inline void _queueCharAt(uint8_t pos, uint8_t code) {
    if (pos >= LCD_DDRAM_SIZE) return;
    s_pendingFrame.ddram[pos] = code;
}

static inline void _rebuildPendingCgramUsageFromDdram() {
    std::memset(s_pendingFrame.cgramUsed, 0, sizeof(s_pendingFrame.cgramUsed));
    for (uint8_t pos = 0; pos < LCD_DDRAM_SIZE; ++pos) {
        const uint8_t code = s_pendingFrame.ddram[pos];
        if (_isCustomCode(code)) {
            s_pendingFrame.cgramUsed[code] = true;
        }
    }
}

static inline uint8_t _flushPendingFrame() {
    _rebuildPendingCgramUsageFromDdram();
    return lcdRenderDiff(s_pendingFrame.ddram, s_pendingFrame.cgram, s_pendingFrame.cgramUsed);
}

//触发E引脚
inline void _triggerE(){
    // 使用 ROM 微秒延时
    esp_rom_delay_us(3);
    GPIO.out |= (1 << LCD_E);
    esp_rom_delay_us(3);
    GPIO.out &= ~(1 << LCD_E);
    esp_rom_delay_us(3);
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

    // HD44780 指令执行时间：普通指令/数据写入约 37us，清屏/归位需约 1.52ms。
    if (mode == CMD && (data == 0x01 || data == 0x02)) {
        esp_rom_delay_us(1600);
    } else {
        esp_rom_delay_us(40);
    }
}


inline void _initLcdContrastPwm(uint8_t initialDuty = 128) {
    ledcSetup(LCD_CTL_PWM_CHANNEL, LCD_CTL_PWM_FREQ, LCD_CTL_PWM_RESOLUTION);
    ledcAttachPin(LCD_CTL_PWM_PIN, LCD_CTL_PWM_CHANNEL);
    ledcWrite(LCD_CTL_PWM_CHANNEL, initialDuty);        // 设置初始对比度
    contrast = initialDuty;
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
void setLcdBrightness(uint8_t duty) {
    if (duty > LCD_BLA_PWM_MAX_DUTY) duty = LCD_BLA_PWM_MAX_DUTY;
    ledcWrite(LCD_BLA_PWM_CHANNEL, duty);
    if( brightness != duty ){
        brightness = duty;
    }
}

// 初始化 LCD 显示模块
void lcdInit(){
    setOutput(LCD_RS);
    setOutput(LCD_E);
    setOutput(LCD_D4);
    setOutput(LCD_D5);
    setOutput(LCD_D6);
    setOutput(LCD_D7);
    setOutput(LCD_BLA);
    setOutput(LCD_CTL);

    // 等待 LCD 上电稳定，避免早期指令丢失导致初始化不完整（如光标闪烁未关闭）。
    delay(50);

    _gpioWrite(0x33,CMD);               // 强制 8-bit 初始化序列（兼容上电未知状态）
    delay(5);
    _gpioWrite(0x33,CMD);
    delay(5);
    _gpioWrite(0x32,CMD);               // 设置LCD切换为4位模式
    delay(5);
    _gpioWrite(0x28,CMD);               // 4-bit, 2-line, 5x8
    delay(5);
    _gpioWrite(0x08,CMD);               // 先关闭显示，避免初始化过程可见闪烁
    delay(5);
    _gpioWrite(0x01,CMD);               // 清屏并将地址指针归位
    delay(5);
    _gpioWrite(0x06,CMD);               // 设定向右写入字符，设置屏幕内容不滚动
    delay(5);
    _gpioWrite(0x0C,CMD);               // 开启显示，关闭光标显示，关闭光标闪烁
    delay(5);

    // 保险：再次写入显示控制，确保 C/B 位被明确清零。
    _gpioWrite(0x0C, CMD);
    delay(2);

    _initLcdBacklightPwm(0);           // 默认亮度 0
    _initLcdContrastPwm(96);         // 默认对比度 96

    for(int i=0;i<=26;i++){
        changeBrightness(10);           // 渐亮背光
        delay(10);
    }

    s_hwFrame.valid = false;
    s_pendingFrame.valid = false;
    std::memset(s_hwFrame.ddram, 0x20, LCD_DDRAM_SIZE);
    std::memset(s_pendingFrame.ddram, 0x20, LCD_DDRAM_SIZE);
    std::memset(s_hwFrame.cgram, 0, sizeof(s_hwFrame.cgram));
    std::memset(s_pendingFrame.cgram, 0, sizeof(s_pendingFrame.cgram));
    std::memset(s_hwFrame.cgramUsed, 0, sizeof(s_hwFrame.cgramUsed));
    std::memset(s_pendingFrame.cgramUsed, 0, sizeof(s_pendingFrame.cgramUsed));

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
void lcdText(const String& ltext,int line){
    // 设置行地址
    int rowStart = 0;
    if (line == 1)
        rowStart = 0;
    else if (line == 2)
        rowStart = 16;
    else
        return;     // 非法行号，直接返回

    // 转换UTF-8假名到LCD字符编码
    String convertedText = convertUTF8ToKana(ltext);
    
    int tsize = convertedText.length();
    for(int size = 0; size < 16; size++){     // 逐字写入待渲染缓冲
        uint8_t code = 0x20;
        if (size <= tsize - 1) {
            code = static_cast<uint8_t>(convertedText[size]);
        }
        _queueCharAt(static_cast<uint8_t>(rowStart + size), code);
    }

    lcdCursor = rowStart;
    _flushPendingFrame();
}

// 设置光标位置（逻辑位置，差分渲染器负责硬件光标定位）
void lcdSetCursor(int changecursor){
    // LOG_LCD_VERBOSE("set cursor in " + String(changecursor));
    lcdCursor = changecursor;
}

void lcdResetCursor(){
    // LOG_LCD_VERBOSE("Reset LCD cursor");
    lcdCursor = 0;
}

// 清除LCD屏幕内容
void lcdClear(){
    std::memset(s_pendingFrame.ddram, 0x20, LCD_DDRAM_SIZE);
    std::memset(s_pendingFrame.cgram, 0, sizeof(s_pendingFrame.cgram));
    std::memset(s_pendingFrame.cgramUsed, 0, sizeof(s_pendingFrame.cgramUsed));

    _flushPendingFrame();
    lcdResetCursor();
    lcdResetCharSlot();
}

// 光标向后移动一格
void _nextCursor(){
    if (lcdCursor >= LCD_DDRAM_SIZE) {
        LOG_LCD_WARN("LCD cursor out of bounds: " + String(lcdCursor));
        lcdCursor = LCD_DDRAM_SIZE;   // 将光标设置为显示区域外
        return;
    }
    lcdCursor += 1;
}

// 写入一字节的自定义字符（手动指定槽位）
void lcdCreateChar(int slot, const uint8_t data[8]){
    if (slot < 0 || slot > 7) return;       // 限制 slot 范围

    std::memcpy(s_pendingFrame.cgram[slot], data, 8);
}

// 写入一字节的自定义字符并立即显示(自动分配槽位)
int lcdCreateCharAuto(const uint8_t data[8]){
    int slotToUse = currentCharSlot;
    
    lcdCreateChar(slotToUse, data);
    _queueCharAt(static_cast<uint8_t>(lcdCursor), static_cast<uint8_t>(slotToUse));
    _nextCursor();
    _flushPendingFrame();
    
    currentCharSlot = (currentCharSlot + 1) % 8;  // 循环使用 0~7
    
    if (currentCharSlot == 0) {
        LOG_LCD_WARN("Auto slot wrapped around, overwriting previous slots!");
    }
    
    return slotToUse;
}

// 重置自动分配的槽位计数器
void lcdResetCharSlot(){
    currentCharSlot = 0;
}

// 显示自定义字符
void lcdDisCustom(int index){
    if (index < 0 || index > 7) return;  // 只能是 0~7 槽
    _queueCharAt(static_cast<uint8_t>(lcdCursor), static_cast<uint8_t>(index));
    _nextCursor();
    _flushPendingFrame();
}

// 显示普通字符
void lcdDisChar(char text){             //显示函数
    _queueCharAt(static_cast<uint8_t>(lcdCursor), static_cast<uint8_t>(text));
    _nextCursor();
    _flushPendingFrame();
}

// 连续显示整段的普通字符，不清除其他的内容，注意越界
void lcdPrint(const String& s) {
    String converted = convertUTF8ToKana(s);
    for (unsigned int i = 0; i < converted.length(); i++) {
        _queueCharAt(static_cast<uint8_t>(lcdCursor), static_cast<uint8_t>(converted[i]));
        _nextCursor();
    }
    _flushPendingFrame();
}

void lcdPrint(const char* s) {
    if (s == nullptr) {
        return;
    }
    lcdPrint(String(s));
}

uint8_t lcdRenderDiff(const uint8_t ddram32[32], const uint8_t cgram8x8[8][8], const bool cgramUsed[8]) {
    uint8_t updatedCells = 0;
    bool slotChanged[8] = {false};
    bool effectiveUsed[8] = {false};

    // 统一“槽位是否被使用”的判断：外部传入标记 + DDRAM 中实际引用。
    for (uint8_t pos = 0; pos < LCD_DDRAM_SIZE; ++pos) {
        const uint8_t code = ddram32[pos];
        if (_isCustomCode(code)) {
            effectiveUsed[code] = true;
        }
    }
    if (cgramUsed != nullptr) {
        for (uint8_t slot = 0; slot < LCD_CGRAM_SLOTS; ++slot) {
            effectiveUsed[slot] = effectiveUsed[slot] || cgramUsed[slot];
        }
    }

    // 先更新本帧用到的 CGRAM 槽位（仅当点阵变化时才写）
    for (int slot = 0; slot < 8; slot++) {
        if (!effectiveUsed[slot]) continue;

        const bool needWrite = (!s_hwFrame.valid)
            || (!s_hwFrame.cgramUsed[slot])
            || (std::memcmp(s_hwFrame.cgram[slot], cgram8x8[slot], 8) != 0);
        if (needWrite) {
            _writeCgramSlot(slot, cgram8x8[slot]);
            std::memcpy(s_hwFrame.cgram[slot], cgram8x8[slot], 8);
            slotChanged[slot] = true;
        }
        s_hwFrame.cgramUsed[slot] = true;
    }

    // DDRAM 差分写入：字符码变化 or 该位置引用的自定义槽位刚被重写
    for (uint8_t pos = 0; pos < LCD_DDRAM_SIZE; pos++) {
        const uint8_t code = ddram32[pos];
        const bool changed = (!s_hwFrame.valid) || (s_hwFrame.ddram[pos] != code);
        const bool affectedBySlot = _isCustomCode(code) && slotChanged[code];
        if (changed || affectedBySlot) {
            _writeCharAt(pos, code);
            s_hwFrame.ddram[pos] = code;
            if (updatedCells < 255) updatedCells++;
        }
    }

    // 更新 lastCgramUsed：未使用的槽位标记为 false（下帧若再次使用可正确触发写入）
    for (int slot = 0; slot < 8; slot++) {
        if (!effectiveUsed[slot]) {
            s_hwFrame.cgramUsed[slot] = false;
        }
    }

    s_hwFrame.valid = true;

    // 同步待渲染缓冲，保证统一差分路径与直接差分路径一致。
    std::memcpy(s_pendingFrame.ddram, ddram32, LCD_DDRAM_SIZE);
    std::memcpy(s_pendingFrame.cgram, cgram8x8, sizeof(s_pendingFrame.cgram));
    std::memcpy(s_pendingFrame.cgramUsed, effectiveUsed, sizeof(s_pendingFrame.cgramUsed));
    s_pendingFrame.valid = true;

    return updatedCells;
}

void lcdPushOverlayFrame() {
    std::memcpy(&s_overlayBackupFrame, &s_pendingFrame, sizeof(LcdFrameState));
    s_overlayBackupCursor = lcdCursor;
    s_overlayBackupValid = true;
}

void lcdPopOverlayFrame() {
    if (!s_overlayBackupValid) {
        return;
    }

    lcdRenderDiff(s_overlayBackupFrame.ddram, s_overlayBackupFrame.cgram, s_overlayBackupFrame.cgramUsed);
    lcdCursor = s_overlayBackupCursor;

    std::memcpy(&s_pendingFrame, &s_overlayBackupFrame, sizeof(LcdFrameState));
    s_overlayBackupValid = false;
}