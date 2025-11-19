#include "about.h"

enum NavigationResult {
    NAV_NONE,      // 无按键
    NAV_PAGE_CHANGE, // 页面改变
    NAV_EXIT       // 退出
};

struct AboutPage {
    const char* line1;
    const char* line2;
};

const AboutPage aboutMePages[] = {
    {"Created: kulib", "kulib88.com"},
    {"だいがくせいかつが", "まもなくおわるけど"},
    {"これからも", "がんばります"},
    {"Thanks!", "Please report"},
    {"Contact:    john", "kulib@icloud.com"},
    {"License:", "GPL3.0"},
};

const AboutPage aboutProjectPages[] = {
    {"ESP32 1602A", "Ver.2025/11/18"},
    {"ESP32-S3-N8R8", "8MB FLASH+PSRAM"},
    {"Features:", "Wireless LCD"},
    {"WiFi Config", "AP + Web UI"},
    {"JWT Auth", "Ed25519 Crypto"},
    {"Gzip Support", "Multi-language"},
    {"Libraries:", "FastLED"},
    {"ArduinoJson", "libsodium"},
    {"zlib_turbo", "etc..."},
    {"Usage Tips:", "Hold CENTER cfg"},
    {"Changelog:", "updated About"},
};

void showAboutPage(const char* line1, const char* line2) {
    lcdText(line1, 1);
    lcdText(line2, 2);
}

NavigationResult _handleAboutNavigation(int& currentPage, int totalPages) {
    if (isButtonReadyToRespond(LEFT, BUTTON_DEBOUNCE_DELAY) && currentPage > 0) {
        currentPage--;
        return NAV_PAGE_CHANGE;
    }
    else if (isButtonReadyToRespond(RIGHT, BUTTON_DEBOUNCE_DELAY) && currentPage < totalPages - 1) {
        currentPage++;
        return NAV_PAGE_CHANGE;
    }
    else if (isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)) {
        currentState = STATE_MENU;
        inMenuMode = true;
        return NAV_EXIT;
    }
    return NAV_NONE; // 没有按键按下
}

void aboutMe(){
    inMenuMode = false;
    int currentPage = 0;
    const int totalPages = 6; // 页面数量
    
    while(true) {
        showAboutPage(aboutMePages[currentPage].line1, aboutMePages[currentPage].line2);
        
        // 等待按键输入
        while(true) {
            NavigationResult result = _handleAboutNavigation(currentPage, totalPages);
            if (result == NAV_PAGE_CHANGE) {
                break; // 页面改变，跳出内层循环，重新显示
            } else if (result == NAV_EXIT) {
                return; // 退出关于界面
            }
            // NAV_NONE: 继续等待
            delay(50);
        }
    }
}

void aboutProject(){
    inMenuMode = false;
    int currentPage = 0;
    const int totalPages = 11; // 页面数量
    
    // 初始延迟，防止一进入就退出
    globalButtonDelay(FIRST_TIME_DELAY);
    
    while(true) {
        showAboutPage(aboutProjectPages[currentPage].line1, aboutProjectPages[currentPage].line2);
        
        // 等待按键输入
        while(true) {
            NavigationResult result = _handleAboutNavigation(currentPage, totalPages);
            if (result == NAV_PAGE_CHANGE) {
                break; // 页面改变，跳出内层循环，重新显示
            } else if (result == NAV_EXIT) {
                return; // 退出关于界面
            }
            // NAV_NONE: 继续等待
            delay(50);
        }
    }
}