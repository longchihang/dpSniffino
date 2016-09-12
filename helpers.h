#ifndef _HELPERS_H_
#define _HELPERS_H_

#include "Arduino.h"

size_t snprintnum(char* buffer, size_t buffer_size, unsigned long n, uint8_t base, uint8_t leadingzero = 0);
//char val2dec(uint8_t b);
char val2hex(uint8_t b);
int8_t mod(int8_t a, int8_t b);
int8_t hex2val(char v);

bool multi_dec_to_uint8_t(const char a[], size_t* p_index, size_t field_length,
	uint8_t buffer[], size_t* p_buffer_index, size_t buffer_size, char delimiter = '\0');

char offset_digital_char(char v, int8_t offset, int8_t mode);
void str_hex_to_uint8_t(const char a[], size_t* p_index, uint16_t field_length,
	uint8_t buffer[], size_t* p_buffer_index, size_t buffer_size);

#endif
