#ifndef _PINS_H_
#define _PINS_H_

//#define VERSION_STR "0.7" // 20200410 modified - for choosing show lldp port or desc to summary
#define VERSION_STR "0.8" // 20211021 - fix cdp summary garbled

#define PIN_BUTTON_UP      6
#define PIN_BUTTON_DOWN    7
#define PIN_BUTTON_LEFT    8
#define PIN_BUTTON_RIGHT   9

#define PIN_BATTERY_VOLTAGE A2

#define PIN_LCD_I2C_ADDR	0x27
#define PIN_LCD_I2C_EN		2
#define PIN_LCD_I2C_RW		1
#define PIN_LCD_I2C_RS		0
#define PIN_LCD_I2C_D4		4
#define PIN_LCD_I2C_D5		5
#define PIN_LCD_I2C_D6		6
#define PIN_LCD_I2C_D7		7
#define PIN_LCD_I2C_BL		3
#define PIN_LCD_I2C_BLPOL	POSITIVE

#define PIN_LCD_RS          5
#define PIN_LCD_EN          4
#define PIN_LCD_D4          3
#define PIN_LCD_D5          2
#define PIN_LCD_D6          A1
#define PIN_LCD_D7          A0

#define PIN_NIC_CS          10



#endif
