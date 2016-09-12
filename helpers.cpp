#include "helpers.h"

size_t snprintnum(char* buffer, size_t buffer_size, unsigned long n, uint8_t base, uint8_t leadingzero) {
	char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
	char *str = &buf[sizeof(buf) - 1];

	*str = '\0';

	if (base < 2) base = 10; // prevent crash

	do {
		unsigned long m = n;
		n /= base;
		char c = (char)( m - base * n);
		*--str = c < 10 ? c + '0' : c + 'A' - 10;
	} while (n);

	unsigned int i = 0;

	unsigned int l = strlen(str);
	if (leadingzero > 0 && l < leadingzero) {
		leadingzero -= l;
		while (i < leadingzero) {
			buffer[i++] = '0';
		}
	}

	while (*str != '\0' && i < (buffer_size - 1)) {
		buffer[i++] = *str++;
	}
	buffer[i] = '\0';

	return i;
}






/*
char val2dec(uint8_t b) {
	switch (b) {
		//case 0: return '0';
	case 1: return '1';
	case 2: return '2';
	case 3: return '3';
	case 4: return '4';
	case 5: return '5';
	case 6: return '6';
	case 7: return '7';
	case 8: return '8';
	case 9: return '9';
	default: return '0';
	}
}
*/



char val2hex(uint8_t b) {
	/*
	switch (b) {
		//case 0x0: return '0';
	case 0x1: return '1';
	case 0x2: return '2';
	case 0x3: return '3';
	case 0x4: return '4';
	case 0x5: return '5';
	case 0x6: return '6';
	case 0x7: return '7';
	case 0x8: return '8';
	case 0x9: return '9';
	case 0xA: return 'A';
	case 0xB: return 'B';
	case 0xC: return 'C';
	case 0xD: return 'D';
	case 0xE: return 'E';
	case 0xF: return 'F';
	default: return '0';
	}
	*/
	char digitals[] = "0123456789ABCDEF";
	return digitals[b];
}

int8_t hex2val(char v) {
	char digitals[] = "0123456789ABCDEF";
	int8_t i;
	for (i = 0; i < 16; i++) {
		if (digitals[i] == v) return i;
	}
	return -1; // not found;
}




// http://stackoverflow.com/questions/4003232/how-to-code-a-modulo-operator-in-c-c-obj-c-that-handles-negative-numbers
int8_t mod(int8_t a, int8_t b)
{
	if (b < 0) //you can check for b == 0 separately and do what you want
		return mod(a, -b);
	int ret = a % b;
	if (ret < 0)
		ret += b;
	return ret;
}


char offset_digital_char(char v, int8_t offset, int8_t mode) {
	//#define NUM_MODE_HEX 16
	//#define NUM_MODE_DEC 10
	//#define NUM_MODE_BIN 2

	//char digitals[] = "0123456789ABCDEF"; // not support float
	//int8_t i;
	//for (i = 0; i < mode; i++) {
	//	if (digitals[i] == v)
	//		return digitals[mod(i + offset, mode)];
	//}

	int8_t i;
	i = hex2val(v);
	if (i >= 0 && i < mode)
		return val2hex(mod(i + offset, mode));
	else
		return '\0';
}


void str_hex_to_uint8_t(const char a[], size_t* p_index, uint16_t field_length,
	uint8_t buffer[], size_t* p_buffer_index, size_t buffer_size) {

	uint16_t i;
	int8_t t;
	for (i = 0; i < field_length && a[*p_index] != '\0' && *p_buffer_index + 1 <= buffer_size; i += 2) {
		t = hex2val(a[(*p_index)++]);
		if (t > -1)
			buffer[*p_buffer_index] = (t << 4);
		t = hex2val(a[(*p_index)++]);
		if (t > -1)
			buffer[(*p_buffer_index)++] |= t;
	}
}


//REFERENCE: http://www.geeksforgeeks.org/write-your-own-atoi/
bool multi_dec_to_uint8_t(const char a[], size_t* p_index, size_t field_length,
	uint8_t buffer[], size_t* p_buffer_index, size_t buffer_size, char delimiter) {

	uint16_t i;
	uint16_t t = 0;
	bool first_char = true;

	//for (i = 0; i < field_length && a[*p_index] != '\0'  && *p_buffer_index < buffer_size; i++) {
	for (i = 0; i < field_length  && *p_buffer_index < buffer_size; i++) {
		if (a[*p_index] != delimiter && a[*p_index] != '\0') {
			if (first_char) {
				if (a[*p_index] == '\t' || a[*p_index] == ' ') {
					(*p_index)++;
					continue;
				}
				else {
					first_char = false;
				}
			}

			if (a[i] < '0' || a[i] > '9')
				return false;

			t = t * 10 + a[*p_index] - '0';

		}
		else {

			if (t > 255)
				return false;

			if (buffer != NULL)
				buffer[*p_buffer_index] = (uint8_t)t;

			(*p_buffer_index)++;
			t = 0;
			first_char = false;

			if (a[*p_index] == '\0') break;
		}
		(*p_index)++;
	}

	//printf("i: %d, *p_buffer_index: %d\n", i, *p_buffer_index);
	return true;
}

