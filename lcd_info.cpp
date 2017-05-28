#include "lcd_info.h"
#include "lcd_control.h"
#include "helpers.h"

int8_t current_menu                  = MENU_NOTHING;
int8_t current_menu_size             = 0;
int8_t current_menu_item             = MENUITEM_NOTHING;
int8_t select_menu                   = MENU_NOTHING;
int8_t select_menu_item              = MENUITEM_NOTHING;
int8_t more_value_offset             = 0;
int8_t edit_value_buffer_index       = -1;
bool   edit_value_is_changed         = false;


//volatile uint16_t lcd_delta_t = 0;
//volatile uint16_t lcd_ttl = 0;

void lcd_info_current_menu_set(int8_t menu, int8_t menu_size, int8_t item) {
	//if (current_menu != menu) {
		current_menu = menu;
		current_menu_size = menu_size; ;
		current_menu_item = MENUITEM_NOTHING;
	//}

	/*
	menu_item* pm;
	for (int8_t i = 0; i < menu_size; i++) {
		pm = &menu_buffer[i];
		pm->p_label = NULL;
		pm->p_value = NULL;
		pm->value_buffer_size = 0;
		pm->editable = false;
	}
	*/
	if (menu_size > 0)
		memset(menu_buffer, 0, sizeof(menu_item) * menu_size);

	lcd_info_current_menu_item_set(item);
}

void lcd_info_current_menu_item_set(int8_t item) {
	current_menu_item = item;
	more_value_offset = 0;
}

void lcd_info_select_menu_item_set(int8_t sel_menu, int8_t sel_item) {
	select_menu = sel_menu;
	select_menu_item = sel_item;
	edit_value_buffer_index = -1;
	edit_value_is_changed = false;
}

int8_t lcd_info_select_menu_item_get() {
	if (lcd_info_menu_item_was_hold_selected())
		return select_menu_item - MENUITEM_SELECT_BY_HOLD;
	else
		return select_menu_item;
}

menu_item* lcd_info_menu_item_setup(int item_index, const __FlashStringHelper* p_label, 
	char buffer[], size_t* p_buffer_index, int8_t value_buffer_size/*, const __FlashStringHelper* p_value*/) {


	menu_item* pm = &menu_buffer[item_index];
	pm->p_label = &buffer[*p_buffer_index];
	//strcpy_P(p_value, (const prog_char *)p_label);
	//uint8_t label_length = strlen_P((const prog_char *)p_label);
	// http://www.nongnu.org/avr-libc/user-manual/group__avr__pgmspace.html#gaa475b6b81fd8b34de45695da1da523b6
	strcpy_P(pm->p_label, (const char PROGMEM  *)p_label);
	//pm->p_value += strlen_P((const char PROGMEM *)p_label);
	size_t label_length_plus_endzero = strlen(pm->p_label) + 1;
	label_length_plus_endzero++;  // skip the label '\0'
	*p_buffer_index += label_length_plus_endzero;

	if (value_buffer_size != 0) {
		pm->p_value = pm->p_label + label_length_plus_endzero;
		pm->value_buffer_size = value_buffer_size;
		memset(pm->p_value, 0, abs(pm->value_buffer_size)); // clear buffer
	}
	/*
	if (p_value != NULL) {
		strcpy_P(pm->p_value, (const char PROGMEM  *)p_value);
		*p_buffer_index += (strlen(pm->p_value) + 1);
	}*/

	return pm;
}

void lcd_info_current_menu_item_ensure_visible(bool direction_next, bool current_menu_item_check_first) {
	// To find the first visible item if current_menu_item is nothing.
	int8_t new_menu_item = (current_menu_item == MENUITEM_NOTHING) ? 0 : current_menu_item; 

	do {
		// If current_menu_item_check_first, first to check new_menu_item (as current_menu_item),
		// and than offset next or prev. 
		if (current_menu_item_check_first) {
			current_menu_item_check_first = false;
		}
		else {
			if (direction_next) {
				if (++new_menu_item >= current_menu_size)
					new_menu_item = 0;
			}
			else {
				if (--new_menu_item < 0)
					new_menu_item = current_menu_size - 1;
			}

		}
	} while (menu_buffer[new_menu_item].p_label == NULL);

	//} while ((menu_buffer[new_menu_item].p_value == NULL &&
	//	menu_buffer[new_menu_item].value_buffer_size > 0) &&
	//	new_menu_item != current_menu_item);

	//more_value_offset = 0;
	//current_menu_item = new_menu_item;
	if (new_menu_item != current_menu_item)
		lcd_info_current_menu_item_set(new_menu_item);
}

void lcd_info_current_menu_item_up() {
	//DEBUG_PRINTLN(F("lcd_info_current_menu_item_up"));
	//DEBUG_PRINTLN_VAR(current_menu_size);
	//DEBUG_PRINTLN_VAR(current_menu_item);

	lcd_info_current_menu_item_ensure_visible(DIRECTION_PREV, false);
	lcd_control_update_lines();

	//DEBUG_PRINTLN_WITH_TITLE(F(" --> "), DEBUG_PRINTLN_VAR(current_menu_item));
}

void lcd_info_current_menu_item_down() {
	//DEBUG_PRINTLN(F("lcd_info_current_menu_item_down"));
	//DEBUG_PRINTLN_VAR(current_menu_size);
	//DEBUG_PRINTLN_VAR(current_menu_item);

	lcd_info_current_menu_item_ensure_visible(DIRECTION_NEXT, false);
	lcd_control_update_lines();

	//DEBUG_PRINTLN_WITH_TITLE(F(" --> "), DEBUG_PRINTLN_VAR(current_menu_item));
}


/*
void lcd_info_menu_item_less() {
	//DEBUG_PRINTLN(F("lcd_info_menu_item_less"));
	//DEBUG_PRINTLN_VAR(more_value_offset);


	if(p_current_menu == NULL) return;
	// If first offset add an extra offset to scroll even though prefixing with a char
	if(more_value_offset == 0)
		more_value_offset = 1;

	if(--more_value_offset < 0)
		more_value_offset = 0;

	//DEBUG_PRINTLN_WITH_TITLE(F(" --> "), DEBUG_PRINTLN_VAR(more_value_offset));
}
*/
void lcd_info_current_menu_item_more_value() {
	//DEBUG_PRINTLN(F("lcd_info_current_menu_item_more_value"));
	//DEBUG_PRINTLN_VAR(more_value_offset);

	menu_item* pcm = &menu_buffer[current_menu_item];
	size_t value_length = strlen(pcm->p_value);

	if (value_length > LCD_WIDTH) {
		// If first offset add an extra offset to scroll even though prefixing with a char
		if (more_value_offset == 0)
			more_value_offset = 1;

		//if ((++more_value_offset + LCD_WIDTH - 2) >= value_length)
		if ((++more_value_offset + LCD_WIDTH - 2) > value_length) // > (not >=), it remain a space at right to easy to know that is the end.
			more_value_offset = 0;

		lcd_control_update_lines();
	}

	//DEBUG_PRINTLN_WITH_TITLE(F(" --> "), DEBUG_PRINTLN_VAR(more_value_offset));
}

void lcd_info_current_menu_item_show() {
	lcd_control_update_lines();
	lcd_control_update_edit_cursor();
}

void lcd_info_current_menu_item_back() {
	lcd_info_select_menu_item_set(MENU_NOTHING, MENUITEM_NOTHING);
	lcd_info_current_menu_item_show();
}

void lcd_info_current_menu_item_edit_value_back() {
	lcd_info_current_menu_item_set(current_menu_item);
	lcd_info_current_menu_item_back();
}

void lcd_info_select_current_menu_item(int8_t hold) {
	select_menu = current_menu;
	select_menu_item = current_menu_item + hold;
}

void lcd_info_current_menu_item_edit_value_move_cursor(char skipchar) {
	//DEBUG_PRINTLN(F("lcd_info_current_menu_item_edit_value_move_cursor"));
	//DEBUG_PRINTLN_VAR(edit_value_buffer_index);

	menu_item* pcm = &menu_buffer[current_menu_item];

	size_t value_length = strlen(pcm->p_value);

	int8_t edit_value_buffer_index_old = edit_value_buffer_index;
	do {
		if (++edit_value_buffer_index >= abs(pcm->value_buffer_size) - 1) {
			edit_value_buffer_index = 0;
		}
		// WARNING: Please make sure skipchar won't make infinite loop, such as pcm->p_value is "....." when skipchar is '.',
		// if pcm->p_value like "192.168.000.001" (ip type value) does not make infinite loop.
		//
		// ADD edit_value_buffer_index != edit_value_buffer_index_old to prevent infinite loop.
	} while (skipchar != '\0' && pcm->p_value[edit_value_buffer_index] == skipchar && edit_value_buffer_index != edit_value_buffer_index_old);

	if (value_length > LCD_WIDTH) {
		if (edit_value_buffer_index >= LCD_WIDTH - 1) {
			// If first offset add an extra offset to scroll even though prefixing with a char
			if (more_value_offset == 0) {
				more_value_offset = 1;
			}

			// We can edit the value in length "pcm->value_buffer_size - 1".
			//
			//if ((++more_value_offset + LCD_WIDTH - 2) >= abs(pcm->value_buffer_size) - 1) { 
			if ((++more_value_offset + LCD_WIDTH - 2) > abs(pcm->value_buffer_size) - 1) { 	// > (not >=), it remain a space at right to easy to know that is the end.
				more_value_offset = 0;
				edit_value_buffer_index = 0;
			}

			lcd_control_update_lines();
		}
	}
	else {
		lcd_control_update_edit_cursor();
	}
	
	//DEBUG_PRINTLN_WITH_TITLE(F(" --> "), DEBUG_PRINTLN_VAR(edit_value_buffer_index));
}




void lcd_info_current_menu_item_edit_value_up(int8_t mode) {
	menu_item* pcm = &menu_buffer[current_menu_item];
	*(pcm->p_value + edit_value_buffer_index) = offset_digital_char(*(pcm->p_value + edit_value_buffer_index), -1, mode);
	edit_value_is_changed = true;
	lcd_control_update_lines();
	lcd_control_update_edit_cursor();
}

void lcd_info_current_menu_item_edit_value_down(int8_t mode) {
	menu_item* pcm = &menu_buffer[current_menu_item];
	*(pcm->p_value + edit_value_buffer_index) = offset_digital_char(*(pcm->p_value + edit_value_buffer_index), 1, mode);
	edit_value_is_changed = true;
	lcd_control_update_lines();
	lcd_control_update_edit_cursor();
}

