#ifndef _LCD_INFO_H_
#define _LCD_INFO_H_
//#define __PROG_TYPES_COMPAT__
//#include <avr/pgmspace.h>
#include <Arduino.h>

#define MENU_NOTHING                        -1
#define MENUITEM_NOTHING                    -1
#define MENUITEM_SELECT_BY_HOLD             50

#define DIRECTION_NEXT                      true
#define DIRECTION_PREV                      false

#define INVISIBLE   NULL

typedef struct {
	char* p_label;             // NULL is invisible
    char* p_value;
	int8_t value_buffer_size;  // Less than 0 is editable
} menu_item;

extern menu_item menu_buffer[];

extern int8_t current_menu;
extern int8_t current_menu_size;
extern int8_t current_menu_item;
extern int8_t select_menu;
extern int8_t select_menu_item;
extern int8_t more_value_offset;
extern int8_t edit_value_buffer_index;
extern bool   edit_value_is_changed;



//extern volatile uint16_t lcd_delta_t;
//extern volatile uint16_t lcd_ttl;
void lcd_info_current_menu_set(int8_t menu = MENU_NOTHING, int8_t menu_size = 0, int8_t item = MENUITEM_NOTHING);
void lcd_info_current_menu_item_set(int8_t item = MENUITEM_NOTHING);


menu_item* lcd_info_menu_item_setup(int item_index, const __FlashStringHelper* p_label, char buffer[], size_t* p_buffer_index, int8_t value_buffer_size, const __FlashStringHelper* p_value = NULL);
void lcd_info_current_menu_item_ensure_visible(bool direction_next, bool current_menu_item_check_first);
void lcd_info_current_menu_item_down();
void lcd_info_current_menu_item_up();
void lcd_info_current_menu_item_more_value();
void lcd_info_current_menu_item_show();
void lcd_info_current_menu_item_back();
//void lcd_info_menu_item_less();

void lcd_info_current_menu_item_edit_value_move_cursor(char skipchar = '\0');
void lcd_info_current_menu_item_edit_value_back();
void lcd_info_current_menu_item_edit_value_up(int8_t mode);
void lcd_info_current_menu_item_edit_value_down(int8_t mode);


/*
bool lcd_info_current_menu_item_was_not_selected();
inline bool lcd_info_current_menu_item_was_not_selected() {
	return select_menu_item == MENUITEM_NOTHING;
}

bool lcd_info_current_menu_item_was_selected();
inline bool lcd_info_current_menu_item_was_selected() {
	return select_menu_item == current_menu_item;
}

bool lcd_info_current_menu_item_was_hold_selected();
inline bool lcd_info_current_menu_item_was_hold_selected() {
	return select_menu_item == current_menu_item + MENUITEM_SELECT_BY_HOLD;
}
*/
void lcd_info_select_menu_item_set(int8_t sel_menu = MENU_NOTHING, int8_t sel_item = MENUITEM_NOTHING);
int8_t lcd_info_select_menu_item_get();

bool lcd_info_current_menu_item_is_editing();
inline bool lcd_info_current_menu_item_is_editing() {
	return (edit_value_buffer_index > -1);
}

bool lcd_info_memu_item_was_not_selected();
inline bool lcd_info_memu_item_was_not_selected() {
	return select_menu_item == MENUITEM_NOTHING;
}

bool lcd_info_menu_item_was_click_selected();
inline bool lcd_info_menu_item_was_click_selected() {
	return (select_menu_item > MENUITEM_NOTHING && select_menu_item < MENUITEM_SELECT_BY_HOLD);
}

bool lcd_info_menu_item_was_hold_selected();
inline bool lcd_info_menu_item_was_hold_selected() {
	return select_menu_item >= MENUITEM_SELECT_BY_HOLD;
}


#endif
