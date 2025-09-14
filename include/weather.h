#ifndef WEATHER_H
#define WEATHER_H

#include "myhader.h"
#include "lcd_driver.h"
#include "menu.h"
#include "button.h"
#include "clock.h"
#include "weathertrans.h"

extern bool weatherSynced;
extern unsigned long lastWeatherUpdate;
extern unsigned int interface_num; // 当前显示的界面编号

void updateWeatherScreen();
void fetchWeatherData();

#endif