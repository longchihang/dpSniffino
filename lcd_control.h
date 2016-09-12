#ifndef _LCD_CONTROL_H_
#define _LCD_CONTROL_H_

#include <Arduino.h>
#include <Wire.h>

#include "pins.h"
#include "lcd_info.h"

#define LCD_CHAR_MORE_LEFT 0x7f 
//0xae
#define LCD_CHAR_MORE_RIGHT 0x7e
//0xaf
#define LCD_CHAR_DELTA 0x5e
#define LCD_WIDTH  16
#define LCD_HEIGHT 2

// #define _LCD_I2C
#ifdef _LCD_I2C
#include <LiquidCrystal_I2C.h>
extern LiquidCrystal_I2C lcd;
#else
#include <LiquidCrystal.h>
extern LiquidCrystal lcd;
#endif


typedef void(*lcd_control_update_lines_callback)();

extern lcd_control_update_lines_callback lcd_control_update_lines_callback_fn;

void lcd_control_init();
//void lcd_control_done();
void lcd_control_message(const __FlashStringHelper* p_msg, uint32_t delay_ms = 0);

void lcd_control_update_lines();
//void lcd_bt_click();
void lcd_control_update_edit_cursor();
void lcd_control_update_progress(bool enabled);
void lcd_control_update_editable();

#endif
