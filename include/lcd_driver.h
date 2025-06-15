#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H


#include "myhader.h"
#include "mydefine.h"

extern int lcdCursor;

void triggle_E();
void gpio_write(int data,int mode);
void lcd_init();
void lcd_text(String ltext,int line);
void lcd_setCursor(int col, int row);
void lcd_next_cousor();
void lcd_createChar(int slot, uint8_t data[8]);
void lcd_dis_costom(int index);
void lcd_dis_chr(char text);
void lcd_clear();
void lcd_print(String s);

#endif