#include "weathertrans.h"

// 函数：将中文天气转换为英文
String getWeatherInEnglish(const String& chineseWeather) {
    if (chineseWeather == "晴") return "Clear";
    if (chineseWeather == "多云") return "Cloudy";
    if (chineseWeather == "阴") return "Overcast";
    if (chineseWeather == "小雨") return "Light Rain";
    if (chineseWeather == "中雨") return "Moderate Rain";
    if (chineseWeather == "大雨") return "Heavy Rain";
    if (chineseWeather == "暴雨") return "Storm";
    if (chineseWeather == "雾") return "Fog";
    if (chineseWeather == "雪") return "Snow";
    if (chineseWeather == "雷阵雨") return "Thunderstorm";
    if (chineseWeather == "沙尘暴") return "Dust Storm";
    return "Unknown Weather";  // 如果不匹配任何已知天气，返回“未知天气”
}