#include "./applications/about.h"

struct AboutPage {
    const char* line1;
    const char* line2;
};

const AboutPage aboutMePages[] = {
    {"Created: kulib", "kulib88.com"},
    {"これからも", "よろしくおねがいします!"},
    {"Thanks!", "Please report"},
    {"Contact:", "kulib@icloud.com"},
    {"GitHub:", "github.com/kuilb"},
    {"Enjoy!", ":-)"},
};

const AboutPage aboutProjectPages[] = {
    {"ESP32 1602A", "Ver.2026/04/03"},
    {"ESP32-S3-N8", "128MB FLASH"},
    {"Features:", "Wireless LCD"},
    {"WiFi Config", "AP + Web UI"},
    {"JWT Auth", "Ed25519 Crypto"},
    {"Gzip Support", "Multi-language"},
    {"Libraries:", "FastLED"},
    {"ArduinoJson", "libsodium"},
    {"zlib_turbo", "etc..."},
};

static int s_aboutCurrentPage = 0;
static bool s_aboutIsNewPage = true;

static void _aboutHandleNavigation(int totalPages) {
    if (isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)) {
        menuReturnToCurrentSubMenu();
        return;
    }
    if (isButtonReadyToRespond(LEFT, BUTTON_DEBOUNCE_DELAY) && s_aboutCurrentPage > 0) {
        s_aboutCurrentPage--;
        s_aboutIsNewPage = true;
    } else if (isButtonReadyToRespond(RIGHT, BUTTON_DEBOUNCE_DELAY) && s_aboutCurrentPage < totalPages - 1) {
        s_aboutCurrentPage++;
        s_aboutIsNewPage = true;
    }
}

void enterBuildInfoInterface() {
    setCurrentInterface(handleBuildInfoInterface);
    globalButtonDelay(FIRST_TIME_DELAY);
    String ver = String(PROJECT_VERSION) + "  " + String(BUILD_VERSION);
    lcdText(ver, 1);
    lcdText(BUILD_TIMESTAMP, 2);
}

void handleBuildInfoInterface() {
    if (isButtonReadyToRespond(CENTER, BUTTON_DEBOUNCE_DELAY)) {
        menuReturnToCurrentSubMenu();
    }
}

void enterAboutMeInterface() {
    s_aboutCurrentPage = 0;
    s_aboutIsNewPage = true;
    setCurrentInterface(handleAboutMeInterface);
    globalButtonDelay(FIRST_TIME_DELAY);
}

void enterAboutProjectInterface() {
    s_aboutCurrentPage = 0;
    s_aboutIsNewPage = true;
    setCurrentInterface(handleAboutProjectInterface);
    globalButtonDelay(FIRST_TIME_DELAY);
}

void handleAboutMeInterface() {
    const int totalPages = sizeof(aboutMePages) / sizeof(aboutMePages[0]);
    if (s_aboutIsNewPage) {
        lcdText(aboutMePages[s_aboutCurrentPage].line1, 1);
        lcdText(aboutMePages[s_aboutCurrentPage].line2, 2);
        s_aboutIsNewPage = false;
    }
    _aboutHandleNavigation(totalPages);
}

void handleAboutProjectInterface() {
    const int totalPages = sizeof(aboutProjectPages) / sizeof(aboutProjectPages[0]);
    if (s_aboutIsNewPage) {
        lcdText(aboutProjectPages[s_aboutCurrentPage].line1, 1);
        lcdText(aboutProjectPages[s_aboutCurrentPage].line2, 2);
        s_aboutIsNewPage = false;
    }
    _aboutHandleNavigation(totalPages);
}
