#include "weathertrans.h"

uint8_t tempIcon[] = {0x04, 0x0A, 0x0E, 0x0A, 0x0E, 0x11, 0x11, 0x0E}; // 温度图标
uint8_t celsius[] = {0x1C, 0x14, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00}; // ℃ 图标

uint8_t northWindIcon [] = {0x00, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00};
uint8_t southWindIcon [] = {0x00, 0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x00};
uint8_t eastWindIcon  [] = {0x00, 0x04, 0x08, 0x1F, 0x08, 0x04, 0x00, 0x00};
uint8_t westWindIcon  [] = {0x00, 0x04, 0x02, 0x1F, 0x02, 0x04, 0x00, 0x00};
uint8_t northEastWindIcon [] = {0x00, 0x01, 0x12, 0x14, 0x18, 0x1E, 0x00, 0x00};
uint8_t southEastWindIcon [] = {0x00, 0x1E, 0x18, 0x14, 0x12, 0x01, 0x00, 0x00};
uint8_t southWestWindIcon [] = {0x00, 0x0F, 0x03, 0x05, 0x09, 0x10, 0x00, 0x00};
uint8_t northWestWindIcon [] = {0x00, 0x10, 0x09, 0x05, 0x03, 0x0F, 0x00, 0x00};

// 依据中文风向获取对应的图标
uint8_t* getWindIcon(const String& windDirection) {
    // 中文风向
    if (windDirection == "北风") {
        return northWindIcon;
    } else if (windDirection == "东风") {
        return eastWindIcon;
    } else if (windDirection == "南风") {
        return southWindIcon;
    } else if (windDirection == "西风") {
        return westWindIcon;
    } else if (windDirection == "东北风") {
        return northEastWindIcon;
    } else if (windDirection == "东南风") {
        return southEastWindIcon;
    } else if (windDirection == "西南风") {
        return southWestWindIcon;
    } else if (windDirection == "西北风") {
        return northWestWindIcon;
    }

    // 英文风向
    else if (windDirection == "N") {
        return northWindIcon;
    } else if (windDirection == "E") {
        return eastWindIcon;
    } else if (windDirection == "S") {
        return southWindIcon;
    } else if (windDirection == "W") {
        return westWindIcon;
    } else if (windDirection == "NE") {
        return northEastWindIcon;
    } else if (windDirection == "SE") {
        return southEastWindIcon;
    } else if (windDirection == "SW") {
        return southWestWindIcon;
    } else if (windDirection == "NW") {
        return northWestWindIcon;
    }
    
    else {
        return northWindIcon; // 返回默认的北风图标而不是 nullptr
    }
}


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

// 函数：根据中文天气获取对应的左半部分图标
uint8_t* getWeatherLeftIcon(const String& chineseWeather) {
    // 中文天气
    if (chineseWeather == "晴") return sunnyLeftIcon;
    if (chineseWeather == "多云") return cloudyLeftIcon;
    if (chineseWeather == "阴") return overcastLeftIcon;
    if (chineseWeather == "小雨") return lightRainLeftIcon;
    if (chineseWeather == "中雨") return moderateRainLeftIcon;
    if (chineseWeather == "大雨") return heavyRainLeftIcon;
    if (chineseWeather == "暴雨") return stormLeftIcon;
    if (chineseWeather == "雾") return fogLeftIcon;
    if (chineseWeather == "雪") return snowLeftIcon;
    if (chineseWeather == "雷阵雨") return thunderLeftIcon;
    if (chineseWeather == "沙尘暴") return dustStormLeftIcon;
    
    // 日语天气
    if (chineseWeather == "晴れ") return sunnyLeftIcon;
    if (chineseWeather == "曇り") return cloudyLeftIcon;
    if (chineseWeather == "驟雨") return lightRainLeftIcon;
    if (chineseWeather == "雪") return snowLeftIcon;
    if (chineseWeather == "雷雨") return thunderLeftIcon;
    if (chineseWeather == "霧") return fogLeftIcon;
    
    // 英文天气
    if (chineseWeather == "Sunny" || chineseWeather == "Clear") return sunnyLeftIcon;
    if (chineseWeather == "Cloudy" || chineseWeather == "Partly Cloudy") return cloudyLeftIcon;
    if (chineseWeather == "Overcast") return overcastLeftIcon;
    if (chineseWeather == "Rain" || chineseWeather == "Light Rain") return lightRainLeftIcon;
    if (chineseWeather == "Heavy Rain") return heavyRainLeftIcon;
    if (chineseWeather == "Snow") return snowLeftIcon;
    if (chineseWeather == "Thunderstorm") return thunderLeftIcon;
    if (chineseWeather == "Fog") return fogLeftIcon;
    
    return sunnyLeftIcon;  // 返回默认的晴天图标
}

// 函数：根据中文天气获取对应的右半部分图标
uint8_t* getWeatherRightIcon(const String& chineseWeather) {
    // 中文天气
    if (chineseWeather == "晴") return sunnyRightIcon;
    if (chineseWeather == "多云") return cloudyRightIcon;
    if (chineseWeather == "阴") return overcastRightIcon;
    if (chineseWeather == "小雨") return lightRainRightIcon;
    if (chineseWeather == "中雨") return moderateRainRightIcon;
    if (chineseWeather == "大雨") return heavyRainRightIcon;
    if (chineseWeather == "暴雨") return stormRightIcon;
    if (chineseWeather == "雾") return fogRightIcon;
    if (chineseWeather == "雪") return snowRightIcon;
    if (chineseWeather == "雷阵雨") return thunderRightIcon;
    if (chineseWeather == "沙尘暴") return dustStormRightIcon;
    
    // 日语天气
    if (chineseWeather == "晴れ") return sunnyRightIcon;
    if (chineseWeather == "曇り") return cloudyRightIcon;
    if (chineseWeather == "驟雨") return lightRainRightIcon;
    if (chineseWeather == "雪") return snowRightIcon;
    if (chineseWeather == "雷雨") return thunderRightIcon;
    if (chineseWeather == "霧") return fogRightIcon;
    
    // 英文天气
    if (chineseWeather == "Sunny" || chineseWeather == "Clear") return sunnyRightIcon;
    if (chineseWeather == "Cloudy" || chineseWeather == "Partly Cloudy") return cloudyRightIcon;
    if (chineseWeather == "Overcast") return overcastRightIcon;
    if (chineseWeather == "Rain" || chineseWeather == "Light Rain") return lightRainRightIcon;
    if (chineseWeather == "Heavy Rain") return heavyRainRightIcon;
    if (chineseWeather == "Snow") return snowRightIcon;
    if (chineseWeather == "Thunderstorm") return thunderRightIcon;
    if (chineseWeather == "Fog") return fogRightIcon;
    
    return sunnyRightIcon;  // 返回默认的晴天图标
}

// 函数：将中文天气转换为英文
String getWeatherInEnglish(const String& chineseWeather) {
    if (chineseWeather == "晴") return "Sunny";
    if (chineseWeather == "多云") return "Cloudy";
    if (chineseWeather == "阴") return "Overcast";
    if (chineseWeather == "小雨") return "Light Rain";
    if (chineseWeather == "中雨") return "Moderate Rain";
    if (chineseWeather == "大雨") return "Heavy Rain";
    if (chineseWeather == "暴雨") return "Storm";
    if (chineseWeather == "雾") return "Fog";
    if (chineseWeather == "雪") return "Snow";
    if (chineseWeather == "雷阵雨") return "Thunder";
    if (chineseWeather == "沙尘暴") return "Dust Storm";
    return "Unknown Weather";  // 如果不匹配任何已知天气，返回“未知天气”
}