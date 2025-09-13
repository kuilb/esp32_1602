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

void updateWeatherScreen();
void fetchWeatherData();

#endif