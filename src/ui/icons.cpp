#include "./ui/icons.h"

// 系统图标定义
namespace SystemIcons {
    uint8_t wifiIcon[] = {0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x0E, 0x00};
    uint8_t wifiOffIcon[] = {0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x0E, 0x11};
    uint8_t bluetoothIcon[] = {0x04, 0x0A, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x0A};
    uint8_t batteryFullIcon[] = {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x1F};
    uint8_t batteryHalfIcon[] = {0x1F, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

    uint8_t tempIcon[] = {0x04, 0x0A, 0x0E, 0x0A, 0x0E, 0x11, 0x11, 0x0E}; // 温度图标
    uint8_t celsius[] = {0x1C, 0x14, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00}; // ℃ 图标

    uint8_t unknowIcon[] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04, 0x00}; // 未知图标

    uint8_t* getIcon(const String& iconName) {
        if (iconName == "wifi") return wifiIcon;
        if (iconName == "wifi_off") return wifiOffIcon;
        if (iconName == "bluetooth") return bluetoothIcon;
        if (iconName == "battery_full") return batteryFullIcon;
        if (iconName == "battery_half") return batteryHalfIcon;
        if (iconName == "temperature") return tempIcon;
        if (iconName == "celsius") return celsius;
        if (iconName == "unknown") return unknowIcon;

        return nullptr;
    }
}

// 风向图标定义
namespace WindIcons {
    uint8_t northWindIcon [] = {0x00, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00};
    uint8_t southWindIcon [] = {0x00, 0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x00};
    uint8_t eastWindIcon  [] = {0x00, 0x04, 0x08, 0x1F, 0x08, 0x04, 0x00, 0x00};
    uint8_t westWindIcon  [] = {0x00, 0x04, 0x02, 0x1F, 0x02, 0x04, 0x00, 0x00};
    uint8_t northEastWindIcon [] = {0x00, 0x01, 0x12, 0x14, 0x18, 0x1E, 0x00, 0x00};
    uint8_t southEastWindIcon [] = {0x00, 0x1E, 0x18, 0x14, 0x12, 0x01, 0x00, 0x00};
    uint8_t southWestWindIcon [] = {0x00, 0x0F, 0x03, 0x05, 0x09, 0x10, 0x00, 0x00};
    uint8_t northWestWindIcon [] = {0x00, 0x10, 0x09, 0x05, 0x03, 0x0F, 0x00, 0x00};

    uint8_t* getIcon(const String& direction) {
        if (direction == "N" || direction == "北风") return northWindIcon;
        if (direction == "S" || direction == "南风") return southWindIcon;
        if (direction == "E" || direction == "东风") return eastWindIcon;
        if (direction == "W" || direction == "西风") return westWindIcon;
        if (direction == "NE" || direction == "东北风") return northEastWindIcon;
        if (direction == "SE" || direction == "东南风") return southEastWindIcon;
        if (direction == "SW" || direction == "西北风") return southWestWindIcon;
        if (direction == "NW" || direction == "西南风") return northWestWindIcon;
        return SystemIcons::unknowIcon;
    }
}

// 天气图标定义
namespace WeatherIcons {
    // 晴 (Sunny)
    uint8_t sunnyLeftIcon[] = {0x08, 0x05, 0x03, 0x1B, 0x03, 0x05, 0x08, 0x00}; 
    uint8_t sunnyRightIcon[] = {0x02, 0x14, 0x18, 0x1B, 0x18, 0x14, 0x02, 0x00};

    // 多云 (Cloudy)
    uint8_t cloudyLeftIcon[] = {0x0C, 0x12, 0x13, 0x0C, 0x10, 0x10, 0x0F, 0x00};  
    uint8_t cloudyRightIcon[] = {0x00, 0x08, 0x14, 0x02, 0x02, 0x01, 0x1E, 0x00};

    // 阴 (Overcast)
    uint8_t overcastLeftIcon[] = {0x00, 0x00, 0x03, 0x0C, 0x10, 0x10, 0x0F, 0x00};  
    uint8_t overcastRightIcon[] = {0x00, 0x08, 0x14, 0x02, 0x02, 0x01, 0x1E, 0x00};

    // 小雨 (Light Rain)
    uint8_t lightRainLeftIcon[] = {0x00, 0x03, 0x0C, 0x08, 0x07, 0x00, 0x01, 0x01};  
    uint8_t lightRainRightIcon[] = {0x08, 0x14, 0x02, 0x02, 0x1C, 0x00, 0x10, 0x10};

    // 中雨 (Moderate Rain)
    uint8_t moderateRainLeftIcon[] = {0x00, 0x03, 0x0C, 0x08, 0x07, 0x00, 0x05, 0x05};
    uint8_t moderateRainRightIcon[] = {0x08, 0x14, 0x02, 0x02, 0x1C, 0x00, 0x10, 0x10};

    // 大雨 (Heavy Rain)
    uint8_t heavyRainLeftIcon[] = {0x00, 0x03, 0x0C, 0x08, 0x07, 0x00, 0x05, 0x05};
    uint8_t heavyRainRightIcon[] = {0x08, 0x14, 0x02, 0x02, 0x1C, 0x00, 0x14, 0x14};

    // 暴雨 (Storm)
    uint8_t stormLeftIcon[] = {0x00, 0x03, 0x1C, 0x10, 0x0F, 0x00, 0x15, 0x15};
    uint8_t stormRightIcon[] = {0x08, 0x14, 0x02, 0x01, 0x1E, 0x00, 0x15, 0x15};

    // 雾 (Fog)
    uint8_t fogLeftIcon[] = {0x17, 0x00, 0x1D, 0x00, 0x1F, 0x00, 0x17, 0x00};
    uint8_t fogRightIcon[] = {0x1D, 0x00, 0x1F, 0x00, 0x1B, 0x00, 0x1D, 0x00};

    // 雪 (Snow)
    uint8_t snowLeftIcon[] = {0x14, 0x0A, 0x01, 0x17, 0x01, 0x0A, 0x04, 0x11};
    uint8_t snowRightIcon[] = {0x05, 0x0A, 0x10, 0x1D, 0x10, 0x0A, 0x04, 0x11};

    // 雷阵雨 (Thunder)
    uint8_t thunderLeftIcon[] = {0x01, 0x0E, 0x10, 0x0F, 0x00, 0x0A, 0x0A, 0x0A};
    uint8_t thunderRightIcon[] = {0x00, 0x1E, 0x01, 0x06, 0x18, 0x0D, 0x09, 0x11};

    // 沙尘暴 (Dust Storm)
    uint8_t dustStormLeftIcon[] = {0x00, 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
    uint8_t dustStormRightIcon[] = {0x00, 0x04, 0x02, 0x1F, 0x02, 0x04, 0x00, 0x00};


    uint8_t* getLeftIcon(const String& weather) {
        // 中文
        if (weather == "晴") return sunnyLeftIcon;
        if (weather == "多云") return cloudyLeftIcon;
        if (weather == "阴") return overcastLeftIcon;
        if (weather == "小雨") return lightRainLeftIcon;
        if (weather == "中雨") return moderateRainLeftIcon;
        if (weather == "大雨") return heavyRainLeftIcon;
        if (weather == "暴雨") return stormLeftIcon;
        if (weather == "雾") return fogLeftIcon;
        if (weather == "雪") return snowLeftIcon;
        if (weather == "雷阵雨") return thunderLeftIcon;
        if (weather == "沙尘暴") return dustStormLeftIcon;
        
        // 日语
        if (weather == "晴れ") return sunnyLeftIcon;
        if (weather == "曇り") return cloudyLeftIcon;
        if (weather == "驟雨") return lightRainLeftIcon;
        if (weather == "雷雨") return thunderLeftIcon;
        if (weather == "霧") return fogLeftIcon;
        
        // 英文
        if (weather == "Sunny" || weather == "Clear") return sunnyLeftIcon;
        if (weather == "Cloudy" || weather == "Partly Cloudy") return cloudyLeftIcon;
        if (weather == "Overcast") return overcastLeftIcon;
        if (weather == "Rain" || weather == "Light Rain") return lightRainLeftIcon;
        if (weather == "Heavy Rain") return heavyRainLeftIcon;
        if (weather == "Snow") return snowLeftIcon;
        if (weather == "Thunderstorm") return thunderLeftIcon;
        if (weather == "Fog") return fogLeftIcon;
        
        return sunnyLeftIcon;
    }

    uint8_t* getRightIcon(const String& weather) {
        // 中文
        if (weather == "晴") return sunnyRightIcon;
        if (weather == "多云") return cloudyRightIcon;
        if (weather == "阴") return overcastRightIcon;
        if (weather == "小雨") return lightRainRightIcon;
        if (weather == "中雨") return moderateRainRightIcon;
        if (weather == "大雨") return heavyRainRightIcon;
        if (weather == "暴雨") return stormRightIcon;
        if (weather == "雾") return fogRightIcon;
        if (weather == "雪") return snowRightIcon;
        if (weather == "雷阵雨") return thunderRightIcon;
        if (weather == "沙尘暴") return dustStormRightIcon;
        
        // 日语
        if (weather == "晴れ") return sunnyRightIcon;
        if (weather == "曇り") return cloudyRightIcon;
        if (weather == "驟雨") return lightRainRightIcon;
        if (weather == "雷雨") return thunderRightIcon;
        if (weather == "霧") return fogRightIcon;
        
        // 英文
        if (weather == "Sunny" || weather == "Clear") return sunnyRightIcon;
        if (weather == "Cloudy" || weather == "Partly Cloudy") return cloudyRightIcon;
        if (weather == "Overcast") return overcastRightIcon;
        if (weather == "Rain" || weather == "Light Rain") return lightRainRightIcon;
        if (weather == "Heavy Rain") return heavyRainRightIcon;
        if (weather == "Snow") return snowRightIcon;
        if (weather == "Thunderstorm") return thunderRightIcon;
        if (weather == "Fog") return fogRightIcon;
        
        return sunnyRightIcon;
    }
}