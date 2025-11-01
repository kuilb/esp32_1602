// clock.h
#ifndef CLOCK_H
#define CLOCK_H

#include "myhader.h"
#include "lcd_driver.h"
#include "menu.h"
#include "button.h"

extern bool timeSynced;

void setupTime();

void updateClockScreen();

int get_timestamp();

#endif
