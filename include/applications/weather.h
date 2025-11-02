#ifndef WEATHER_H
#define WEATHER_H

#include "myheader.h"
#include "lcd_driver.h"
#include "menu.h"
#include "button.h"
#include "clock.h"
#include "weathertrans.h"
#include "jwt_auth.h"

extern bool weatherSynced;
extern unsigned long lastWeatherUpdate;
extern unsigned int interface_num; // 当前显示的界面编号

void updateWeatherScreen();
bool fetchWeatherData();

#endif
