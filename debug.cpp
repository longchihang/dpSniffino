#include "debug.h"
void debug_print_str(const uint8_t a[], uint16_t offset, uint16_t length) {
	for (uint16_t i = offset; i < offset + length; ++i) {
		Serial.write(a[i]);
	}
}

void debug_print_dec(const uint8_t a[], uint16_t offset, uint16_t length) {
	for (uint16_t i = offset; i < offset + length; ++i) {
		if (i>offset) Serial.print('.');
		//Serial.print(a[i], DEC);
		DEBUG_PRINT_A_DEC(a[i]);
	}
}


void debug_print_char(const char a[], uint16_t offset, uint16_t length) {
	for (uint16_t i = offset; i<offset + length; ++i) {
		DEBUG_PRINT_A_HEX(a[i]);
	}
	Serial.println();
}

void debug_print_hex(const uint8_t a[], uint16_t offset, uint16_t length) {
	for (uint16_t i = offset; i<offset + length; ++i) {
		if (i > 0) Serial.print(' ');
		DEBUG_PRINT_A_HEX(a[i]);
	}
}