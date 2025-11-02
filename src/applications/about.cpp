#include "about.h"

void aboutMe(){
    inMenuMode = false;
    int currentPage = 0;
    const int totalPages = 7;
    
    // 初始延迟，防止一进入就退出
    delay(300);
    
    while(true) {
        switch(currentPage) {
            case 0:
                lcd_text("Created: kulib", 1);
                lcd_text("kulib88.com", 2);
                break;
            case 2:
                lcd_text("だいがくせいかつが", 1);
                lcd_text("まもなくおわるけど", 2);
                break;
            case 3:
                lcd_text("これからも", 1);
                lcd_text("がんばります", 2);
                break;
            case 4:
                lcd_text("Thanks!", 1);
                lcd_text("Please report", 2);
                break;
            case 5:
                lcd_text("Contact:    john", 1);
                lcd_text("kulib@icloud.com", 2);
                break;
            case 6:
                lcd_text("License:", 1);
                lcd_text("GPL3.0", 2);
                break;    
        }
        
        bool keyPressed = false;
        while(!keyPressed) {
            if(isButtonReadyToRespond(LEFT, BUTTON_DEBOUNCE_DELAY) && currentPage > 0) {
                currentPage--;
                keyPressed = true;
            }
            else if(isButtonReadyToRespond(RIGHT, BUTTON_DEBOUNCE_DELAY) && currentPage < totalPages - 1) {
                currentPage++;
                keyPressed = true;
            }
            else if(isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)) {
                currentState = STATE_MENU;
                inMenuMode = true;
                return;
            }
            delay(50);
        }
    }
}

void aboutProject(){
    inMenuMode = false;
    int currentPage = 0;
    const int totalPages = 11;
    
    // 初始延迟，防止一进入就退出
    delay(300);
    
    while(true) {
        switch(currentPage) {
            case 0:
                lcd_text("ESP32 1602A", 1);
                lcd_text("Ver.2025/11/3", 2);
                break;
            case 1:
                lcd_text("ESP32-S3-N8R8", 1);
                lcd_text("8MB FLASH+PSRAM", 2);
                break;
            case 2:
                lcd_text("Features:", 1);
                lcd_text("Wireless LCD", 2);
                break;
            case 3:
                lcd_text("WiFi Config", 1);
                lcd_text("AP + Web UI", 2);
                break;
            case 4:
                lcd_text("JWT Auth", 1);
                lcd_text("Ed25519 Crypto", 2);
                break;
            case 5:
                lcd_text("Gzip Support", 1);
                lcd_text("Multi-language", 2);
                break;
            case 6:
                lcd_text("Libraries:", 1);
                lcd_text("FastLED", 2);
                break;
            case 7:
                lcd_text("ArduinoJson", 1);
                lcd_text("libsodium", 2);
                break;
            case 8:
                lcd_text("zlib_turbo", 1);
                lcd_text("etc...", 2);
                break;
            case 9:
                lcd_text("Usage Tips:", 1);
                lcd_text("Hold CENTER cfg", 2);
                break;
            case 10:
                lcd_text("Changelog:", 1);
                lcd_text("test OTA", 2);
                break;
        }
        
        // 等待按键，添加防抖延迟
        bool keyPressed = false;
        while(!keyPressed) {
            if(isButtonReadyToRespond(LEFT,BUTTON_DEBOUNCE_DELAY) && currentPage > 0) {
                currentPage--;
                keyPressed = true;
            }
            else if(isButtonReadyToRespond(RIGHT,BUTTON_DEBOUNCE_DELAY) && currentPage < totalPages - 1) {
                currentPage++;
                keyPressed = true;
            }
            else if(isButtonReadyToRespond(CENTER,BUTTON_DEBOUNCE_DELAY)) {
                currentState = STATE_MENU;
                inMenuMode = true;
                return;
            }
            delay(50);
        }
    }
}