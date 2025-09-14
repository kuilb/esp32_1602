#ifndef WEATHER_TRANS_H
#define WEATHER_TRANS_H

#include "myhader.h"

extern uint8_t tempIcon[];
extern uint8_t celsius[];

String getWeatherInEnglish(const String& chineseWeather);
uint8_t* getWindIcon(const String& windDirection);
uint8_t* getWeatherLeftIcon(const String& chineseWeather);
uint8_t* getWeatherRightIcon(const String& chineseWeather);

#endif