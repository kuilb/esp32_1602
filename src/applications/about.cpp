#include "about.h"

void aboutMe(){
    inMenuMode = false;
    int currentPage = 0;
    const int totalPages = 6;
    
    // 初始延迟，防止一进入就退出
    delay(300);
    
    while(true) {
        switch(currentPage) {
            case 0:
                lcd_text("Created by:kulib", 1);
                lcd_text("HP:kulib88.com", 2);
                break;
            case 1:
                lcd_text("ESP32-S3", 1);
                lcd_text("1602Display", 2);
                break;
            case 2:
                lcd_text("2025/11/01", 1);
                lcd_text("つぎはにほんご", 2);
                break;
            case 3:
                lcd_text("だいがくせいかつが", 1);
                lcd_text("まもなくおわるけど", 2);
                break;
            case 4:
                lcd_text("これからも", 1);
                lcd_text("がんばります", 2);
                break;
            case 5:
                lcd_text("どうぞよろしく", 1);
                lcd_text("おねがいいたします", 2);
                break;    
        }
        
        // 等待按键，添加防抖延迟
        bool keyPressed = false;
        while(!keyPressed) {
            if(isButtonReadyToRespond(LEFT,150) && currentPage > 0) {
                currentPage--;
                keyPressed = true;
            }
            else if(isButtonReadyToRespond(RIGHT,150) && currentPage < totalPages - 1) {
                currentPage++;
                keyPressed = true;
            }
            else if(isButtonReadyToRespond(CENTER,150)) {
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
    const int totalPages = 5;
    
    // 初始延迟，防止一进入就退出
    globalButtonDelay(300);
    
    while(true) {
        switch(currentPage) {
            case 0:
                lcd_text("ESP32-S3-N8R8", 1);
                lcd_text("8MB+8MB PSRAM", 2);
                break;
            case 1:
                lcd_text("Features:", 1);
                lcd_text("wireless Screen", 2);
                break;
            case 2:
                lcd_text("WiFi Config", 1);
                lcd_text("Web Settings", 2);
                break;
            case 3:
                lcd_text("JWT Auth", 1);
                lcd_text("Ed25519 Crypto", 2);
                break;
            case 4:
                lcd_text("Gzip Support", 1);
                lcd_text("Multi-language", 2);
                break;
        }
        
        // 等待按键，添加防抖延迟
        bool keyPressed = false;
        while(!keyPressed) {
            if(isButtonReadyToRespond(LEFT,150) && currentPage > 0) {
                currentPage--;
                keyPressed = true;
            }
            else if(isButtonReadyToRespond(RIGHT,150) && currentPage < totalPages - 1) {
                currentPage++;
                keyPressed = true;
            }
            else if(buttonJustPressed[CENTER]) {
                currentState = STATE_MENU;
                inMenuMode = true;
                return;
            }
            delay(50);
        }
    }
}