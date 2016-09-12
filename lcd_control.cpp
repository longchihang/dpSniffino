#include "lcd_control.h"

#ifdef _LCD_I2C
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(PIN_LCD_I2C_ADDR, PIN_LCD_I2C_EN, 1PIN_LCD_I2C_RW, PIN_LCD_I2C_RS, PIN_LCD_I2C_D4, PIN_LCD_I2C_D5, PIN_LCD_I2C_D6, PIN_LCD_I2C_D7, PIN_LCD_I2C_BL, PIN_LCD_I2C_BLPOL);
#else
//               RS, E, D4, D5, D6, D7
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
#endif

lcd_control_update_lines_callback lcd_control_update_lines_callback_fn = NULL;

#define BACK_SLASH byte(0)

char lcd_progress = ' ';

void lcd_control_init() {
	lcd.begin(16, 2);
	/*
	// blinking 3 times
	for(int i = 0; i < 3; i++) {
		lcd.backlight();
		delay(250);
		lcd.noBacklight();
		delay(250);
	}
	lcd.backlight();
	*/
	PROGMEM uint8_t back_slash[8] = {
		B00000,
		B10000,
		B01000,
		B00100,
		B00010,
		B00001,
		B00000
	};
	lcd.createChar(BACK_SLASH, back_slash);

	lcd_control_message((const __FlashStringHelper*)(F("dpSniffino v" VERSION_STR)), 500);

	//lcd.setCursor(0,1);
	//lcd.print(F("Initializing..."));
}

void lcd_control_message(const __FlashStringHelper* p_msg, uint32_t delay_ms) {
	lcd.noCursor();
	lcd.noBlink();
	// https://www.arduino.cc/en/Reference/LiquidCrystalClear
	// Clears the LCD screen and positions the cursor in the upper-left corner.
	lcd.clear(); 
	//lcd.setCursor(0, 0);
	if (p_msg != NULL) {
		lcd.print(p_msg);
		if (delay_ms > 0) {
			delay(delay_ms);
		}
	}
		
}

/*
void lcd_control_message_box(const __FlashStringHelper* p_prompt, int8_t choice_count, const __FlashStringHelper* p_choice[], const int8_t choice_x_pos[]) {
	lcd.clear();
	lcd.setCursor(0, 0);
	if (p_prompt != NULL) {
		lcd.print(p_prompt);
		int8_t i, l;
		//int8_t k = 0;
		for (i = 0; i < choice_count; i++) {
			//l = strlen_P((const char PROGMEM *)p_choice[i]);
			//lcd.setCursor(k, 1);
			//lcd.print(p_choice[i]);
			//k += l + 1; // + 1 for a space
			lcd.setCursor(choice_x_pos[i], 1);
			lcd.print(p_choice[i]);
		}
	}
}
*/
/*
void lcd_control_done() {
	lcd.clear();
	lcd.setCursor(0,0);
}
*/

void lcd_control_update_lines() {
	menu_item* pcm = &menu_buffer[current_menu_item];

	int8_t i, j, k;
	char lcd_line[LCD_HEIGHT][LCD_WIDTH + 1] = { { '\0' },{ '\0' } }; // LCD_WIDTH + \0


	// pcm->value == NULL && pcm->value_buffer_size > 0  // Item is invisible. (NOT OUTPUT)
	// pcm->value == NULL && pcm->value_buffer_size == 0 // Item no value, just output as a item title. (OUTPUT)
	// pcm->value != NULL && pcm->value_buffer_size > 0 // Item is visible. (OUTPUT)
	// pcm->value != NULL && pcm->value_buffer_size == 0 // No this situation.
	if (pcm->p_label != NULL) {
		size_t label_length = strlen(pcm->p_label);
		strncpy(lcd_line[0], pcm->p_label, LCD_WIDTH);
		i = label_length;
		/*
		if (pcm->value_buffer_size > 0) { // not a item title
			lcd_line[0][i++] = ':';
			lcd_line[0][i++] = ' ';
		}
		*/


		if (pcm->p_value != NULL) {
			i = 0;
			// If already offset leading part, show '<' in lcd[0,1].
			// 'i' is point to lcd output position, is after '<' .
			if (more_value_offset > 0) { 
				lcd_line[1][i++] = LCD_CHAR_MORE_LEFT; 
			}

			// 'value_length - more_value_offset' is the length of remain value that need to show.
			// 'LCD_WIDTH - i' is the actual length can show.
			// If the lcd actual length is not enough for output remain length p->value,
			// so let the lcd show 'LCD_WIDTH - i - 1' length of remain value because need to show '>'
			// at lcd[15,1].
			size_t value_length = (lcd_info_current_menu_item_is_editing()) ? abs(pcm->value_buffer_size) - 1 : strlen(pcm->p_value);

			// something at right  
			if ((value_length - more_value_offset) > (LCD_WIDTH - i) || 
				/* Let a space at right to easy to know that is the end.  'i > 0' means (having '<' ) moved from left to here. */
				(i > 0 && (value_length - more_value_offset) == (LCD_WIDTH - i)) ) { 
				value_length = LCD_WIDTH - i - 1; // the last 1 is include right '>' size
				lcd_line[1][LCD_WIDTH - 1] = LCD_CHAR_MORE_RIGHT; // show '>' in 1,0
			}
			else {
				value_length -= more_value_offset;
			}
			// to print string after the number of more_value_offset

			strncpy(&lcd_line[1][i], &pcm->p_value[more_value_offset], value_length);
		}
		else {
			// for just printa title ( label without value ), e.g. The first LLDP in lldp_menu
			// or value not given.
			//strncpy(lcd_line[1], "", LCD_WIDTH);
			lcd_line[1][0] = '\0';
		}

	}
	/*
	else {
		// restore lcd_line0 data to empty
		//for(i = 0; i < LCD_WIDTH; ++i) lcd_line0[i] = ' ';
		// restore lcd_line1 data to empty
		//for(i = 0; i < LCD_WIDTH; ++i) lcd_line1[i] = ' ';
		strncpy(lcd_line[0], "Waiting...", LCD_WIDTH);
		lcd_line[1][0] = '\0';
		//strncpy(lcd_line[1], F(""), LCD_WIDTH);

		//for(j = 0; j < LCD_HEIGHT; ++j) {
		//	for (i = 0; i < LCD_WIDTH; ++i) {
		//		lcd_line[j][i] = ' ';
		//	}
		//}
	}
	*/
	lcd_line[0][LCD_WIDTH] = '\0'; // ensure end of '\0'
	lcd_line[1][LCD_WIDTH] = '\0';

	// to output data to lcd
	bool complete;
	for (j = 0; j < LCD_HEIGHT; ++j) {
		complete = false;
		lcd.setCursor(0, j);
		for (k = 0; k < LCD_WIDTH; ++k) {
			// if data is printed complete, let print ' 's at tail til the line end.
			if (!complete) {
				if (lcd_line[j][k] != '\0') {
					lcd.write(lcd_line[j][k]);
					continue;
				}
				else {
					complete = true;
				}
			}

			
			if (j != 0 || k != LCD_WIDTH - 1) {
				lcd.write(' ');
			}
			else {
				// draw the status position 
				if (pcm->value_buffer_size < 0) 
					lcd.write((lcd_info_current_menu_item_is_editing()) ? '*' : '#');
				else
					lcd.write(lcd_progress);
			}

		}
	}

	lcd_control_update_editable();


	if (lcd_control_update_lines_callback_fn != NULL) {
		lcd_control_update_lines_callback_fn();
	}
}

void lcd_control_update_edit_cursor() {
	// If edit value, need to show current cursor.
	if (lcd_info_current_menu_item_is_editing()) {
		if (edit_value_buffer_index <= LCD_WIDTH - 2) {
			lcd.setCursor(edit_value_buffer_index, 1);
		}
		else {
			// (LCD_WIDTH - 1, 1); // Because is the tail of value, not show '>' right arrow.
			//lcd.setCursor((edit_value_buffer_index < abs(pcm->value_buffer_size) - 1) ? LCD_WIDTH - 2 : LCD_WIDTH - 1, 1);
			// -1 (not -2), let a space at right to easy to know that is the end.
			lcd.setCursor(LCD_WIDTH - 1, 1);
		}
		lcd.cursor();
		//lcd.blink();
	}
	else {
		lcd.noCursor();
		//lcd.noBlink();
	}
}


void lcd_control_update_progress(bool enabled) {
	/*
	static uint8_t progress = 0;
	lcd.setCursor(LCD_WIDTH - 1, 0);
	switch (progress++) {
	case 0: lcd.write('|'); break;
	case 1: lcd.write('/'); break;
	case 2: lcd.write('-'); break;
	default: lcd.write(uint8_t(BACK_SLASH)); progress = 0; // not back-slash in lcd, need to make font.
	}
	delay(50);
	*/
	if (enabled) {
		switch (lcd_progress) {
		case '|':  lcd_progress = '/'; break;
		case '/':  lcd_progress = '-'; break;
		case '-':  lcd_progress = BACK_SLASH; break;
		default: lcd_progress = '|'; break;// not back-slash in lcd, need to make font.
		}

	}
	else {
		if (lcd_progress != ' ')
			lcd_progress = ' ';
		else
			return;
	}
	lcd.setCursor(LCD_WIDTH - 1, 0);
	lcd.write(lcd_progress);

}

void lcd_control_update_editable() {
	menu_item* pcm = &menu_buffer[current_menu_item];
	if (pcm->value_buffer_size < 0) {
		lcd.setCursor(LCD_WIDTH - 1, 0);
		lcd.write((lcd_info_current_menu_item_is_editing()) ? '*' : '#');
	}
}