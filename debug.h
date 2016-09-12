#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <Arduino.h>

#define DEBUG_PRINT_A_DEC(n) { Serial.print(n, DEC); }
#define DEBUG_PRINT_A_HEX(n) { if((n) < 0x10){ Serial.print('0'); } Serial.print((n),HEX); }

void debug_print_str(const uint8_t a[], uint16_t offset, uint16_t length);

void debug_print_dec(const uint8_t a[], uint16_t offset, uint16_t length);

void debug_print_char(const char a[], uint16_t offset, uint16_t length);

void debug_print_hex(const uint8_t a[], uint16_t offset, uint16_t length);

//#define _DEBUG
#ifdef _DEBUG
#define DEBUG_BEGIN(a) Serial.begin(a);
#define DEBUG_PRINTLN_WITH_TITLE(title, fragment) \
	{ Serial.println(title); \
	{ fragment } \
	Serial.println(); }

#define DEBUG_PRINT(a) Serial.print(a);
#define DEBUG_PRINTLN(a) Serial.println(a);
//http://stackoverflow.com/questions/195975/how-to-make-a-char-string-from-a-c-macros-value
#define QUOTE(name) #name
#define DEBUG_PRINTLN_VAR(var) { Serial.print(QUOTE(var:));  Serial.println(var); }
#define DEBUG_PRINTLN_SPLIT_LINE() {Serial.println(F("--------------------"));}

#define DEBUG_PRINT_STR(a, b, c) debug_print_str(a, b, c);
#define DEBUG_PRINT_DEC(a, b, c)  debug_print_dec(a, b, c);
#define DEBUG_PRINT_CHAR(a, b, c) debug_print_char(a, b, c);
#define DEBUG_PRINT_HEX(a, b, c) debug_print_hex(a, b, c);
#else
#define DEBUG_BEGIN(a)
#define DEBUG_PRINTLN_WITH_TITLE(title, fragment)
#define DEBUG_PRINT(a)
#define DEBUG_PRINTLN(a)
#define DEBUG_PRINTLN_VAR(var)
#define DEBUG_PRINTLN_SPLIT_LINE();
#define DEBUG_PRINT_STR(a, b, c)
#define DEBUG_PRINT_IP(a, b, c) 
#define DEBUG_PRINT_MAC(a, b, c)
#define DEBUG_PRINT_HEX(a, b, c)
#endif

#endif