#include <EEPROM.h>
#include <SPI.h>
#include <EtherCard.h>
#include <Wire.h>
#include <DebounceButton.h>
//#include <avr/wdt.h> // support software reboot

#include "dp_listener.h"
#include "lcd_control.h"
#include "lcd_info.h"
#include "helpers.h"
#include "debug.h"

#define MENU_MAIN                                         0
#define MENU_CDP                                          1
#define MENU_EDP                                          2
#define MENU_LLDP                                         3
#define MENU_TCPIP                                        4
#define MENUSIZE_MAIN                                     7
#define MENUSIZE_CDP                                      9  
#define MENUSIZE_EDP                                      5
#define MENUSIZE_LLDP                                     6
#define MENUSIZE_TCPIP                                    4

#define MENUITEM_MAIN_TRACE_CDP_EDP                       0
#define MENUITEM_MAIN_TRACE_LLDP                          1
#define MENUITEM_MAIN_PING                                2
#define MENUITEM_MAIN_CONFIG_TCPIP                        3
#define MENUITEM_MAIN_CONFIG_MY_MAC                       4
#define MENUITEM_MAIN_SAVE_CONFIG                         5
#define MENUITEM_MAIN_REBOOT                              6
#define MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_CDP_EDP       11
#define MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_LLDP          MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_CDP_EDP
#define MENUITEM_VALUEBUFFERSIZE_MAIN_PING                16
#define MENUITEM_VALUEBUFFERSIZE_MAIN_CONFIG_TCPIP        2
#define MENUITEM_VALUEBUFFERSIZE_MAIN_CONFIG_MY_MAC       13
#define MENUITEM_VALUEBUFFERSIZE_MAIN_SAVE_CONFIG         2
#define MENUITEM_VALUEBUFFERSIZE_MAIN_REBOOT              1

#define MENUITEM_CDP_VERSION                              0
#define MENUITEM_CDP_PORT_ID                              1
#define MENUITEM_CDP_DEVICE_ID                            2
#define MENUITEM_CDP_DEVICE_MAC                           3
#define MENUITEM_CDP_ADDRESSES                            4
#define MENUITEM_CDP_NATIVE_VLAN                          5
#define MENUITEM_CDP_SOFTWARE                             6
#define MENUITEM_CDP_PLATFORM                             7
#define MENUITEM_CDP_DUPLEX                               8
#define MENUITEM_VALUEBUFFERSIZE_CDP_VERSION              3
#define MENUITEM_VALUEBUFFERSIZE_CDP_PORT_ID              21
#define MENUITEM_VALUEBUFFERSIZE_CDP_DEVICE_ID            21
#define MENUITEM_VALUEBUFFERSIZE_CDP_DEVICE_MAC           13
#define MENUITEM_VALUEBUFFERSIZE_CDP_ADDRESSES            65
#define MENUITEM_VALUEBUFFERSIZE_CDP_NATIVE_VLAN          6
#define MENUITEM_VALUEBUFFERSIZE_CDP_SOFTWARE             41 
#define MENUITEM_VALUEBUFFERSIZE_CDP_PLATFORM             21
#define MENUITEM_VALUEBUFFERSIZE_CDP_DUPLEX               5


#define MENUITEM_EDP_VERSION                              0
#define MENUITEM_EDP_SLOT_PORT                            1
#define MENUITEM_EDP_DEVICE_NAME                          2
#define MENUITEM_EDP_DEVICE_MAC                           3
#define MENUITEM_EDP_SOFTWARE_VERSION                     4
#define MENUITEM_VALUEBUFFERSIZE_EDP_VERSION              3
#define MENUITEM_VALUEBUFFERSIZE_EDP_SLOT_PORT            11
#define MENUITEM_VALUEBUFFERSIZE_EDP_DEVICE_NAME          21
#define MENUITEM_VALUEBUFFERSIZE_EDP_DEVICE_MAC           13
#define MENUITEM_VALUEBUFFERSIZE_EDP_SOFTWARE_VERSION     16


#define MENUITEM_LLDP                                     0
#define MENUITEM_LLDP_PORT_ID                             1
#define MENUITEM_LLDP_PORT_DESCRIPTION                    2
#define MENUITEM_LLDP_DEVICE_NAME                         3
#define MENUITEM_LLDP_DEVICE_MAC                          4
#define MENUITEM_LLDP_DEVICE_DESCRIPTION                  5
#define MENUITEM_VALUEBUFFERSIZE_LLDP                     0
#define MENUITEM_VALUEBUFFERSIZE_LLDP_PORT_ID             21
#define MENUITEM_VALUEBUFFERSIZE_LLDP_PORT_DESCRIPTION    21
#define MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_NAME         21
#define MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_MAC          13
#define MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_DESCRIPTION  41


#define MENUITEM_TCPIP_MY_IP                              0
#define MENUITEM_TCPIP_NETMASK                            1
#define MENUITEM_TCPIP_GATEWAY_IP                         2
#define MENUITEM_TCPIP_DNS_IP                             3
#define MENUITEM_VALUEBUFFERSIZE_TCPIP_MY_IP              16
#define MENUITEM_VALUEBUFFERSIZE_TCPIP_NETMASK            16
#define MENUITEM_VALUEBUFFERSIZE_TCPIP_GATEWAY_IP         16 
#define MENUITEM_VALUEBUFFERSIZE_TCPIP_DNS_IP             16

menu_item menu_buffer[9] = { //9 - MAX of MENUSIZE
	{ NULL, NULL, 0 }
};

uint8_t my_mac[]     = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };

/*
uint8_t my_ip[]      = { 0x00, 0x00, 0x00, 0x00 };
uint8_t netmask[]    = { 0x00, 0x00, 0x00, 0x00 };
uint8_t gw_ip[]      = { 0x00, 0x00, 0x00, 0x00 };
uint8_t dns_ip[]     = { 0x00, 0x00, 0x00, 0x00 };
*/
#define TCPIP_TYPE_NONE   0
#define TCPIP_TYPE_DHCP   1
#define TCPIP_TYPE_STATIC 2
uint8_t tcpip_type = TCPIP_TYPE_NONE;
uint8_t save_config  = 0;

char label_value_buffer[300];

//uint32_t last_lcd_update = 0;
uint32_t last_lcd_progress_update = 0;
//uint32_t last_serial_ping = 0;
uint32_t last_lcd_ping_update = 0;

//volatile uint32_t last_dp_received = 0;
//volatile uint32_t dp_packets_received = 0;

#define PING_TIMER_STOP      -9999999 
#define PING_TIMER_START     -9999998 
volatile int32_t ping_timer = PING_TIMER_STOP; // start timing out right away

volatile int ping_response_time = -1; // -1 means the first packet not be sent,
									  // ping_test_send() will set 0, ping_test_recv() set > 0 if response.

int8_t ping_dot_pos = 0; 

#define MODIFIER_OFFSET      1
#define MODIFIER_LEADINGZERO 2

typedef void(*value_handler) (
	const uint8_t packet[], size_t* p_index, uint16_t field_length,
	char buffer[], size_t* p_buffer_index, size_t buffer_size, uint8_t modifier, int modifier_value);


DebounceButton btnUp(PIN_BUTTON_UP, DBTN_PULLUP_INTERNAL, 50, 600, 300); // 50 ms debounce, 0.6 sec before hold interval, 300 ms hold events
DebounceButton btnDown(PIN_BUTTON_DOWN, DBTN_PULLUP_INTERNAL, 50, 600, 300); // 50 ms debounce, 0.6 sec before hold interval, 300 ms hold events
DebounceButton btnLeft(PIN_BUTTON_LEFT, DBTN_PULLUP_INTERNAL, 50, 400, 0); // 50 ms debounce, 0.6 sec before hold interval, 300 ms hold events
DebounceButton btnRight(PIN_BUTTON_RIGHT, DBTN_PULLUP_INTERNAL, 50, 400, 0); // 50 ms debounce, 0.6 sec before hold interval, 300 ms hold events


/*
// http://www.xappsoftware.com/wordpress/2013/06/24/three-ways-to-reset-an-arduino-board-by-code/
// http://www.codeproject.com/Articles/1012319/Arduino-Software-Reset
void software_reboot(uint8_t prescaller) {
	// start watchdog with the provided prescaller
	wdt_enable(prescaller);
	// wait for the prescaller time to expire
	// without sending the reset signal by using
	// the wdt_reset() method
	while (1) {}
}
*/

// Restarts program from beginning but 
// does not reset the peripherals and registers
void software_restart(){
	// jump to the start of the program
	asm volatile ("jmp 0");
}



void init_tcpip() {
	switch (tcpip_type) {
	case TCPIP_TYPE_NONE:
		/*
		for (int8_t i = 0; i < IP_LENGTH; i++) {
			ether.myip[i] = 0;
			ether.netmask[i] = 0;
			ether.gwip[i] = 0;
			ether.dnsip[i] = 0;
		}
		*/
		memset(ether.myip, 0, IP_LENGTH);
		memset(ether.netmask, 0, IP_LENGTH);
		memset(ether.gwip, 0, IP_LENGTH);
		memset(ether.dnsip, 0, IP_LENGTH);
		dp_listener_tcpip_static();
		break;
	case TCPIP_TYPE_DHCP:
		lcd_control_message((const __FlashStringHelper *)F("Waiting DHCP..."), 500);
		if (!dp_listener_tcpip_dhcp()) {
			lcd_control_message((const __FlashStringHelper *)F("DHCP failed."), 4000);
		}
		else {
			lcd_control_message((const __FlashStringHelper *)F("DHCP OK."), 1000);
		}
		break;
	case TCPIP_TYPE_STATIC:
		// If ether.my_ip, ether.netmask, ether.gw_ip, ether.dns_ip manually set first, 
		// according to the source etherCard.cpp, it shoud also 
		// call EtherCard::staticSetup(NULL, NULL, NULL, NULL)
		// and it always return true.
		dp_listener_tcpip_static();
		lcd_control_message((const __FlashStringHelper *)F("Static IP OK."), 1000);
		break;
	}
}

void init_nic() {
	lcd_control_message((const __FlashStringHelper *)F("Init NIC..."), 1000);
	dp_listener_init();

	init_tcpip();
}





void handleAsciiField(
	const uint8_t packet[], size_t* p_index, uint16_t field_length,
	char buffer[], size_t* p_buffer_index, size_t buffer_size, uint8_t modifier = 0, int modifier_value = 0) {

	//DEBUG_PRINTLN_WITH_TITLE(F("handleAsciiField:"), DEBUG_PRINT_STR(packet, *p_index, field_length));

	buffer_size--; // is for the tail '\0' space
	buffer_size += *p_buffer_index; // let buffer_size offset from current *p_buffer_index

	for (uint16_t i = 0; i < field_length && i < buffer_size; ++i) {
		buffer[(*p_buffer_index)++] = packet[(*p_index)++];
	}
	buffer[*p_buffer_index] = '\0';
}

void handleHexField(
	const uint8_t packet[], size_t* p_index, uint16_t field_length,
	char buffer[], size_t* p_buffer_index, size_t buffer_size, uint8_t modifier = 0, int modifier_value = 0) {

	buffer_size--; // is for the tail '\0' space
	buffer_size += *p_buffer_index; // let buffer_size offset from current *p_buffer_index

	//DEBUG_PRINTLN_WITH_TITLE(F("handleHexField:"), DEBUG_PRINT_HEX(packet, *p_index, field_length));

	for (uint16_t i = 0; i < field_length && *p_buffer_index + 1 < buffer_size; ++i) {
		buffer[(*p_buffer_index)++] = val2hex(packet[*p_index] >> 4);
		buffer[(*p_buffer_index)++] = val2hex(packet[(*p_index)++] & 0xf);
	}
	buffer[*p_buffer_index] = '\0';
}



void handleNumField(
	const uint8_t packet[], size_t* p_index, uint16_t field_length,
	char buffer[], size_t* p_buffer_index, size_t buffer_size, uint8_t modifier = 0, int modifier_value = 0) {

	uint32_t num = 0;
	for (uint16_t i = 0; i < field_length; ++i) {
		num <<= 8;
		num += packet[(*p_index)++];
	}

	// modifier is used for offset as edp slot and port is 0-based, by physical switch is 1-based.
	if (modifier == MODIFIER_OFFSET)
		num += modifier_value;

	//DEBUG_PRINTLN_WITH_TITLE(F("handleNumField:"), DEBUG_PRINT_DEC(packet, *p_index, field_length));

	*p_buffer_index += snprintnum(buffer + *p_buffer_index, buffer_size, num, 10, (modifier == MODIFIER_LEADINGZERO) ? modifier_value : 0);
	// if buffer is run out, should return the end of buffer_size (the '\0' position)
	//if(buffer_index < buffer_size)
	//	return buffer_index;
	//else
	//	return buffer_size;

	// snprintnum will give the tail '\0'
}


// To handle ip address (e.g 11.22.33.44) or version number (e.g. 1.2.3.4)
// and ip address list (e.g. 11.22.33.44;99.88.77.66).
// packet[] = {0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44, 0xE1, 0xE2, 0xE3, 0xE4, 0x21, 0x32, 0x43, 0x54}
// field_length = 2
// set_count = 4
// set_delimiter = '.'
// group_count = 2
// group_delimiter = ';'
// --> AABB.CCDD.1122.3344;E1E2.E3E4.2132.4354
void handleMultiField(value_handler handler,
	const uint8_t packet[], size_t* p_index, uint16_t field_length,
	char buffer[], size_t* p_buffer_index, size_t buffer_size,
	uint8_t set_count, char set_delimiter,
	uint8_t group_count, char group_delimiter, 
	uint8_t modifier = 0, int modifier_value = 0) {

	buffer_size--; // is for the tail '\0' space
	buffer_size += *p_buffer_index; // let buffer_size offset from current *p_buffer_index

	uint8_t i, j;
	for (j = 0; j < group_count; ++j) {
		if (j > 0 && *p_buffer_index < buffer_size) buffer[(*p_buffer_index)++] = group_delimiter;
		// the really usable buffer_size decided by inside loop handleNumField
		for (i = 0; i < set_count; ++i) {
			if (i > 0 && *p_buffer_index < buffer_size) buffer[(*p_buffer_index)++] = set_delimiter;

			handler(packet, p_index, field_length,
				buffer, p_buffer_index, buffer_size, modifier, modifier_value);
		}

	}
	// ensure the tail is '\0' if quit loop after add delimiter or fields_delimiter
	buffer[*p_buffer_index] = '\0';

}

void handleCdpAddresses(const uint8_t packet[], size_t* p_index, uint16_t field_length,
	char buffer[], size_t* p_buffer_index, size_t buffer_size, uint8_t modifier = 0, int modifier_value = 0) {

	buffer_size--; // is for the tail '\0' space

	uint32_t addresses_count = (packet[*p_index] << 24)
		| (packet[*p_index + 1] << 16)
		| (packet[*p_index + 2] << 8)
		| packet[*p_index + 3];
	*p_index += 4;
	// it should not to handle many IP 
	if (addresses_count > 4) return;

	uint8_t protocol_type = packet[(*p_index)++];
	uint8_t protocol_length = packet[(*p_index)++];
	// uint8_t protocol[8];
	// for(uint8_t j=0; j<protoLength; ++j) {
	//	proto[j] = a[(*p_index)++];
	//}
	*p_index += protocol_length;
	uint16_t address_length = (packet[*p_index] << 8) | packet[*p_index + 1];
	*p_index += 2;

	//buffer_index = 0;
	handleMultiField(&handleNumField,
		packet, p_index, 1,
		buffer, p_buffer_index, buffer_size,
		IP_LENGTH, '.', (uint8_t)addresses_count, ',');
}


void lcd_update() {
	//lcd_delta_t = (millis() - last_dp_received) / 1000;
	if (millis() > last_lcd_progress_update + 400) {
		lcd_control_update_progress(
			(select_menu_item == MENUITEM_MAIN_TRACE_CDP_EDP ||
			select_menu_item == MENUITEM_MAIN_TRACE_LLDP ||
			ping_timer != PING_TIMER_STOP));
		last_lcd_progress_update = millis();
	}

	
	if (millis() > last_lcd_ping_update + 500) {
		ping_test_send_and_lcd_update_response(ping_timer != PING_TIMER_STOP);
		last_lcd_ping_update = millis();
	}

	// Use static-updated (by button or data received) 
	// to replace cdpSniffino updated in interval.
	//
	//if (millis() > last_lcd_update + 100) {
	//	//lcd_control_update_lines();
	//	last_lcd_update = millis();
	//}


	//if (millis() > last_serial_ping + 2000) {
	//	DEBUG_PRINTLN_VAR(lcd_delta_t);
	//	last_serial_ping = millis();
	//}

}



void init_main_menu(int8_t item = MENUITEM_NOTHING) {
	lcd_info_current_menu_set(MENU_MAIN, MENUSIZE_MAIN, MENUITEM_NOTHING);

	menu_item* pm = NULL;
	size_t index = 0;
	//#define MENU_MAIN_BUFFER_OFFSET          0
	size_t buffer_index = 0;// MENU_MAIN_BUFFER_OFFSET;

	// CDP/EDP and LLDP use the same buffer to show the count receive other packages
	pm = lcd_info_menu_item_setup(MENUITEM_MAIN_TRACE_CDP_EDP, (const __FlashStringHelper *)F("Trace CDP/EDP"),
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_CDP_EDP);
	// Trace CDP/EDP and Trace LLDP show the same dp_packets_received.
	//snprintnum(pm->p_value, MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_CDP_EDP, dp_packets_received, 10); /* packet_length*/
	//buffer_index += (MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_CDP_EDP + 1); 

	pm = lcd_info_menu_item_setup(MENUITEM_MAIN_TRACE_LLDP, (const __FlashStringHelper *)F("Trace LLDP"),
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_LLDP);
	snprintnum(pm->p_value, MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_LLDP, dp_packets_received, 10); /* packet_length*/
	buffer_index += (MENUITEM_VALUEBUFFERSIZE_MAIN_TRACE_LLDP + 1);

	menu_buffer[MENUITEM_MAIN_TRACE_CDP_EDP].p_value = menu_buffer[MENUITEM_MAIN_TRACE_LLDP].p_value;


	// ping
	index = 0;
	pm = lcd_info_menu_item_setup(MENUITEM_MAIN_PING, (const __FlashStringHelper *)F("Ping"),
		label_value_buffer, &buffer_index, -MENUITEM_VALUEBUFFERSIZE_MAIN_PING);
	handleMultiField(&handleNumField,
		ether.hisip, &index, 1,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_MAIN_PING,
		IP_LENGTH, '.', 1, ',',
		MODIFIER_LEADINGZERO, 3);
	buffer_index++; // skip the '\0'


	// my_mac
	index = 0;
	pm = lcd_info_menu_item_setup(MENUITEM_MAIN_CONFIG_MY_MAC, (const __FlashStringHelper *)F("Config my MAC"),
		label_value_buffer, &buffer_index, -MENUITEM_VALUEBUFFERSIZE_MAIN_CONFIG_MY_MAC);

	//DEBUG_PRINTLN_WITH_TITLE(F("init_main_menu my_mac: "), DEBUG_PRINT_HEX(my_mac, 0, MAC_LENGTH));

	handleHexField(my_mac, &index, MAC_LENGTH,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_MAIN_CONFIG_MY_MAC);
	buffer_index++; // skip the '\0'
	
	// tcpip
	pm = lcd_info_menu_item_setup(MENUITEM_MAIN_CONFIG_TCPIP, (const __FlashStringHelper *)F("Config TCP/IP"),
		label_value_buffer, &buffer_index, -MENUITEM_VALUEBUFFERSIZE_MAIN_CONFIG_TCPIP);
	snprintnum(pm->p_value, MENUITEM_VALUEBUFFERSIZE_MAIN_CONFIG_TCPIP, tcpip_type, 10);
	buffer_index += (MENUITEM_VALUEBUFFERSIZE_MAIN_CONFIG_TCPIP + 1);

	// save config
	pm = lcd_info_menu_item_setup(MENUITEM_MAIN_SAVE_CONFIG, (const __FlashStringHelper *)F("Save config"),
		label_value_buffer, &buffer_index, -MENUITEM_VALUEBUFFERSIZE_MAIN_SAVE_CONFIG);
	snprintnum(pm->p_value, MENUITEM_VALUEBUFFERSIZE_MAIN_SAVE_CONFIG, save_config, 10); /* packet_length*/
	buffer_index += (MENUITEM_VALUEBUFFERSIZE_MAIN_SAVE_CONFIG + 1);

	// restart
	pm = lcd_info_menu_item_setup(MENUITEM_MAIN_REBOOT, (const __FlashStringHelper *)F("Restart"),
		label_value_buffer, &buffer_index, -MENUITEM_VALUEBUFFERSIZE_MAIN_REBOOT);
	buffer_index += (MENUITEM_VALUEBUFFERSIZE_MAIN_REBOOT + 1);  // skip the '\0'




	if (item != MENUITEM_NOTHING) {
		lcd_info_current_menu_item_set(item);
		lcd_info_current_menu_item_ensure_visible(DIRECTION_NEXT, true);

		//lcd_info_current_menu_item_show();
	}

}

void init_tcpip_menu(int8_t item = MENUITEM_NOTHING) {
	//if (select_menu_item != MENUITEM_MAIN_CONFIG_TCPIP) return;
	lcd_info_current_menu_set(MENU_TCPIP, MENUSIZE_TCPIP);


	menu_item* pm = NULL;
	size_t index = 0;
	size_t buffer_index = 0;


	pm = lcd_info_menu_item_setup(MENUITEM_TCPIP_MY_IP, (const __FlashStringHelper *)F("IP"),
		label_value_buffer, &buffer_index, 
		(tcpip_type == TCPIP_TYPE_STATIC) ? -MENUITEM_VALUEBUFFERSIZE_TCPIP_MY_IP : MENUITEM_VALUEBUFFERSIZE_TCPIP_MY_IP);
	handleMultiField(&handleNumField,
		ether.myip, &index, 1,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_TCPIP_MY_IP,
		IP_LENGTH, '.', 1, ',',
		MODIFIER_LEADINGZERO, 3);
	buffer_index++; // skip the '\0'

	//DEBUG_PRINTLN_WITH_TITLE(F("IP: "), DEBUG_PRINT_DEC(ether.myip, 0, IP_LENGTH));

	index = 0;
	pm = lcd_info_menu_item_setup(MENUITEM_TCPIP_NETMASK, (const __FlashStringHelper *)F("Netmask"),
		label_value_buffer, &buffer_index, 
		(tcpip_type == TCPIP_TYPE_STATIC) ? -MENUITEM_VALUEBUFFERSIZE_TCPIP_NETMASK : MENUITEM_VALUEBUFFERSIZE_TCPIP_NETMASK);
	handleMultiField(&handleNumField,
		ether.netmask, &index, 1,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_TCPIP_NETMASK,
		IP_LENGTH, '.', 1, ',',
		MODIFIER_LEADINGZERO, 3);
	buffer_index++; // skip the '\0'

	//DEBUG_PRINTLN_WITH_TITLE(F("Netmask: "), DEBUG_PRINT_DEC(ether.netmask, 0, IP_LENGTH));

	index = 0;
	pm = lcd_info_menu_item_setup(MENUITEM_TCPIP_GATEWAY_IP, (const __FlashStringHelper *)F("Gateway"),
		label_value_buffer, &buffer_index, 
		(tcpip_type == TCPIP_TYPE_STATIC) ? -MENUITEM_VALUEBUFFERSIZE_TCPIP_GATEWAY_IP : MENUITEM_VALUEBUFFERSIZE_TCPIP_GATEWAY_IP);
	handleMultiField(&handleNumField,
		ether.gwip, &index, 1,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_TCPIP_GATEWAY_IP,
		IP_LENGTH, '.', 1, ',',
		MODIFIER_LEADINGZERO, 3);
	buffer_index++; // skip the '\0'

	//DEBUG_PRINTLN_WITH_TITLE(F("Gateway: "), DEBUG_PRINT_DEC(ether.gwip, 0, IP_LENGTH));

	index = 0;
	pm = lcd_info_menu_item_setup(MENUITEM_TCPIP_DNS_IP, (const __FlashStringHelper *)F("DNS"),
		label_value_buffer, &buffer_index, 
		(tcpip_type == TCPIP_TYPE_STATIC) ? -MENUITEM_VALUEBUFFERSIZE_TCPIP_DNS_IP : MENUITEM_VALUEBUFFERSIZE_TCPIP_DNS_IP);
	handleMultiField(&handleNumField,
		ether.dnsip, &index, 1,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_TCPIP_DNS_IP,
		IP_LENGTH, '.', 1, ',',
		MODIFIER_LEADINGZERO, 3);
	buffer_index++; // skip the '\0'

	//DEBUG_PRINTLN_WITH_TITLE(F("DNS: "), DEBUG_PRINT_DEC(ether.gwip, 0, IP_LENGTH));

	if (item != MENUITEM_NOTHING) {
		lcd_info_current_menu_item_set(item);
		lcd_info_current_menu_item_ensure_visible(DIRECTION_NEXT, true);

		//lcd_info_current_menu_item_show();
	}
}
/*
bool edit_ip_value_update(uint8_t ip[], const __FlashStringHelper * wrong_msg) {

	uint8_t temp[] = { 0x00, 0x00, 0x00, 0x00 };
	menu_item* pcm = &menu_buffer[current_menu_item];
	size_t index = 0;
	size_t buffer_index = 0;

	bool rtn = multi_dec_to_uint8_t(pcm->p_value, &index, 16,
		temp, &buffer_index, IP_LENGTH, '.');
	//if (edit_value_is_changed) {
		if (rtn) {
			for (buffer_index = 0; buffer_index < IP_LENGTH; buffer_index++)
				ip[buffer_index] = temp[buffer_index];

			init_tcpip(); 

			lcd_info_current_menu_item_edit_value_back();
		}
		else {
			lcd_control_message(wrong_msg, 1000);
			lcd_info_current_menu_item_show();
		}
	//}
	return rtn;
}
*/
int8_t edit_ip_value_update(uint8_t ip[]) {

	uint8_t temp[] = { 0x00, 0x00, 0x00, 0x00 };
	menu_item* pcm = &menu_buffer[current_menu_item];
	size_t index = 0;
	size_t buffer_index = 0;

	bool rtn = multi_dec_to_uint8_t(pcm->p_value, &index, 16,
		temp, &buffer_index, IP_LENGTH, '.');
	if (rtn) {
		//for (buffer_index = 0; buffer_index < IP_LENGTH; buffer_index++)
		//	ip[buffer_index] = temp[buffer_index];
		ether.copyIp(ip, temp);

		//init_tcpip();
		//lcd_info_current_menu_item_edit_value_back();
	}
	else {
		//lcd_control_message(wrong_msg, 1000);
		//lcd_info_current_menu_item_show();
	}

	return (int8_t)rtn;
}


void btnUp_click_hold(DebounceButton* btn) {
	DEBUG_PRINTLN_WITH_TITLE(F("[UP]"),  DEBUG_PRINTLN_VAR(current_menu);
													DEBUG_PRINTLN_VAR(current_menu_item);
													DEBUG_PRINTLN_VAR(select_menu_item); );

	switch (current_menu) {
	case MENU_CDP:
	case MENU_EDP:
	case MENU_LLDP:
		//if (lcd_info_memu_item_was_not_selected()) {
			lcd_info_current_menu_item_up();
		//}
		break;

	case MENU_MAIN:
		if (lcd_info_memu_item_was_not_selected()) {
			lcd_info_current_menu_item_up();
		}
		else if (lcd_info_menu_item_was_click_selected()){
		}
		else if (lcd_info_menu_item_was_hold_selected()) {
			switch (current_menu_item) {
			case MENUITEM_MAIN_PING:
				if (ping_timer == PING_TIMER_STOP)
					lcd_info_current_menu_item_edit_value_up(10);
				break;
			case MENUITEM_MAIN_CONFIG_MY_MAC:
				lcd_info_current_menu_item_edit_value_up(16);
				break;
			case MENUITEM_MAIN_CONFIG_TCPIP:
				lcd_info_current_menu_item_edit_value_up(3);
				break;
			case MENUITEM_MAIN_SAVE_CONFIG:
				lcd_info_current_menu_item_edit_value_up(2);
				break;
			}
		}
		break;

	case MENU_TCPIP:
		if (lcd_info_memu_item_was_not_selected()) {
			lcd_info_current_menu_item_up();
		}
		else if (lcd_info_menu_item_was_click_selected()) {
		}
		else if(lcd_info_menu_item_was_hold_selected()) {
			// edit ip value up 
			if (tcpip_type == TCPIP_TYPE_STATIC) {
				lcd_info_current_menu_item_edit_value_up(10);
			}
		}
		break;
	}

	DEBUG_PRINTLN_SPLIT_LINE();
}



void btnDown_click_hold(DebounceButton* btn) {
	DEBUG_PRINTLN_WITH_TITLE(F("[DOWN]"), DEBUG_PRINTLN_VAR(current_menu);
													DEBUG_PRINTLN_VAR(current_menu_item);
													DEBUG_PRINTLN_VAR(select_menu_item); );

	switch (current_menu) {
	case MENU_CDP:
	case MENU_EDP:
	case MENU_LLDP:
		//if (lcd_info_memu_item_was_not_selected()) {
			lcd_info_current_menu_item_down();
		//}
		break;

	case MENU_MAIN:
		if (lcd_info_memu_item_was_not_selected()) {
			lcd_info_current_menu_item_down();
		}
		else if (lcd_info_menu_item_was_click_selected()) {
		}
		else if (lcd_info_menu_item_was_hold_selected()) {
			switch (current_menu_item) {
			case MENUITEM_MAIN_PING:
				if (ping_timer == PING_TIMER_STOP)
					lcd_info_current_menu_item_edit_value_down(10);
				break;
			case MENUITEM_MAIN_CONFIG_MY_MAC:
				lcd_info_current_menu_item_edit_value_down(16);
				break;
			case MENUITEM_MAIN_CONFIG_TCPIP:
				lcd_info_current_menu_item_edit_value_down(3);
				break;
			case MENUITEM_MAIN_SAVE_CONFIG:
				lcd_info_current_menu_item_edit_value_down(2);
				break;
			}
		}
		break;

	case MENU_TCPIP:
		if (lcd_info_memu_item_was_not_selected()) {
			lcd_info_current_menu_item_down();
		}
		else if (lcd_info_menu_item_was_click_selected()) {
		}
		else if (lcd_info_menu_item_was_hold_selected()) {
			// edit ip value up 
			if (tcpip_type == TCPIP_TYPE_STATIC) {
				lcd_info_current_menu_item_edit_value_down(10);
			}
		}
		break;
	}

	DEBUG_PRINTLN_SPLIT_LINE();
}


void btnLeft_click_hold(DebounceButton* btn) {
	DEBUG_PRINTLN_WITH_TITLE(F("[LEFT]"), DEBUG_PRINTLN_VAR(current_menu);
														DEBUG_PRINTLN_VAR(current_menu_item);
														DEBUG_PRINTLN_VAR(select_menu_item); );

	switch (current_menu) {
	case MENU_CDP:
	case MENU_EDP:
		//if (lcd_info_memu_item_was_not_selected() ||
		//	lcd_info_menu_item_was_click_selected() ||
		//	lcd_info_menu_item_was_hold_selected()) {

		// Because it selected from MENUITEM_MAIN_TRACE_CDP_EDP / MENUITEM_MAIN_TRACE_LLDP
		//if (select_menu_item == MENUITEM_MAIN_TRACE_CDP_EDP || 
		//	select_menu_item == MENUITEM_MAIN_TRACE_LLDP) {

			DEBUG_PRINTLN(F("(BACK) "));
			dp_listener_promiscuous(false);

			// Back to menu_buffer, and move to "Trace CDP/EDP" item.
			init_main_menu(MENUITEM_MAIN_TRACE_CDP_EDP);
			lcd_info_current_menu_item_back();


		//}
		break;

	case MENU_LLDP:

			DEBUG_PRINTLN(F("(BACK) "));
			dp_listener_promiscuous(false);

			// Back to menu_buffer, and move to "Trace LLDP" item.
			init_main_menu(MENUITEM_MAIN_TRACE_LLDP);
			lcd_info_current_menu_item_back();
	

		break;

	case MENU_MAIN:
		switch (current_menu_item) {
		case MENUITEM_MAIN_TRACE_CDP_EDP:
		case MENUITEM_MAIN_TRACE_LLDP:
			if (lcd_info_menu_item_was_click_selected()) {
				DEBUG_PRINTLN(F("(BACK) "));

				dp_listener_promiscuous(false);

				lcd_info_select_menu_item_set(MENU_NOTHING, MENUITEM_NOTHING); // to stop the tracing
			}
			break;
		case MENUITEM_MAIN_PING:
			if (lcd_info_menu_item_was_hold_selected()) {
				if (ping_timer == PING_TIMER_STOP) {
					init_main_menu(current_menu_item);
					lcd_info_current_menu_item_back();
				}
				else {
					DEBUG_PRINTLN(F("(BACK) leave "));
					ping_timer = PING_TIMER_STOP;

					// force to stop ping_test_send_and_lcd_update_response(), so it not be run by lcd_update().
					// force to stop lcd_control_update_progress(), so lcd_control_update_progress() not be run by lcd_update().
					// since they will affect below lcd_info_current_menu_item_show(), such as # * and cursor line _ not showing.
					ping_test_send_and_lcd_update_response(false);
					lcd_control_update_progress(false);

					lcd_info_current_menu_item_show();
				}
			}
			break;
		case MENUITEM_MAIN_CONFIG_MY_MAC:
		case MENUITEM_MAIN_CONFIG_TCPIP:
		case MENUITEM_MAIN_SAVE_CONFIG:
			if (lcd_info_menu_item_was_hold_selected()) {
				//select_menu_item = MENUITEM_NOTHING;
				//lcd_info_current_menu_item_edit_value_back();
				init_main_menu(current_menu_item);
				lcd_info_current_menu_item_back();
			}
			break;
		}
		break;

	case MENU_TCPIP:
		if (lcd_info_memu_item_was_not_selected() ||
			lcd_info_menu_item_was_click_selected()) {
			// Back to menu_buffer, and move to "Trace LLDP" item.
			init_main_menu(MENUITEM_MAIN_CONFIG_TCPIP);
			lcd_info_current_menu_item_back();
		}
		else if (lcd_info_menu_item_was_hold_selected()) {
			init_tcpip_menu(current_menu_item);
			lcd_info_current_menu_item_back();
		}
		break;
	}

	DEBUG_PRINTLN_SPLIT_LINE();
}

void btnRight_click(DebounceButton* btn) {
	DEBUG_PRINTLN_WITH_TITLE(F("[RIGHT-CLICK]"), DEBUG_PRINTLN_VAR(current_menu);
																DEBUG_PRINTLN_VAR(current_menu_item);
																DEBUG_PRINTLN_VAR(select_menu_item); );

	switch (current_menu) {
	case MENU_CDP:
	case MENU_EDP:
	case MENU_LLDP:
		//if (lcd_info_memu_item_was_not_selected()) {
			lcd_info_current_menu_item_more_value();
		//}
		break;
	case MENU_MAIN:
		if (lcd_info_memu_item_was_not_selected()) {
			switch (current_menu_item) {
			case MENUITEM_MAIN_TRACE_CDP_EDP:
			case MENUITEM_MAIN_TRACE_LLDP:
				DEBUG_PRINTLN(F("(SELECT) to trace"));
				lcd_info_current_menu_item_set(current_menu_item);
				lcd_info_select_menu_item_set(current_menu, current_menu_item); // to enable the tracing
				dp_listener_promiscuous(true);
				break;
			case MENUITEM_MAIN_CONFIG_TCPIP:
				if (tcpip_type != TCPIP_TYPE_NONE) {
					DEBUG_PRINTLN(F("(SELECT) to enter"));
					init_tcpip_menu(MENUITEM_TCPIP_MY_IP);
					lcd_info_current_menu_item_show();
				}
				break;

			}
		}
		else if (lcd_info_menu_item_was_click_selected()) {

		}
		else if (lcd_info_menu_item_was_hold_selected()) {
			switch (current_menu_item) {
			case MENUITEM_MAIN_PING:
				if (ping_timer == PING_TIMER_STOP) {
					DEBUG_PRINTLN(F("(SELECT to move cursor) "));
					lcd_info_current_menu_item_edit_value_move_cursor('.');
				}
				break;
			case MENUITEM_MAIN_CONFIG_MY_MAC:
			case MENUITEM_MAIN_CONFIG_TCPIP:
			case MENUITEM_MAIN_SAVE_CONFIG:
				DEBUG_PRINTLN(F("(SELECT to move cursor) "));
				lcd_info_current_menu_item_edit_value_move_cursor();
				break;
			}
		}
		break;

	case MENU_TCPIP:
		if (tcpip_type == TCPIP_TYPE_STATIC) {
			if (lcd_info_menu_item_was_hold_selected()) {
				switch (current_menu_item) {
				case MENUITEM_TCPIP_MY_IP:
				case MENUITEM_TCPIP_NETMASK:
				case MENUITEM_TCPIP_GATEWAY_IP:
				case MENUITEM_TCPIP_DNS_IP:
					DEBUG_PRINTLN(F("(SELECT to move cursor) "));

					lcd_info_current_menu_item_edit_value_move_cursor('.');
				}
			}
		}
		else {
			if (lcd_info_memu_item_was_not_selected()) {
				lcd_info_current_menu_item_more_value();
			}
		}
			
		break;
	}

	DEBUG_PRINTLN_SPLIT_LINE();
}

void btnRight_hold(DebounceButton* btn) {
	DEBUG_PRINTLN_WITH_TITLE(F("[RIGHT-HOLD]"), DEBUG_PRINTLN_VAR(current_menu);
															DEBUG_PRINTLN_VAR(current_menu_item);
															DEBUG_PRINTLN_VAR(select_menu_item); );

	switch (current_menu) {
	case MENU_MAIN:
		if (lcd_info_memu_item_was_not_selected()) {
			switch (current_menu_item) {
			case MENUITEM_MAIN_PING:
				if (ping_timer == PING_TIMER_STOP) {
					lcd_info_select_menu_item_set(current_menu, current_menu_item + MENUITEM_SELECT_BY_HOLD);
					lcd_info_current_menu_item_edit_value_move_cursor('.');
				}

				break;
			case MENUITEM_MAIN_CONFIG_MY_MAC:
			case MENUITEM_MAIN_CONFIG_TCPIP:
			case MENUITEM_MAIN_SAVE_CONFIG:
				DEBUG_PRINTLN(F("(Select to edit) "));
				lcd_info_select_menu_item_set(current_menu, current_menu_item + MENUITEM_SELECT_BY_HOLD);
				lcd_info_current_menu_item_edit_value_move_cursor();
				break;
			case MENUITEM_MAIN_REBOOT:
				DEBUG_PRINTLN(F("(Select to reboot) "));
				//software_reboot(WDTO_60MS);
				software_restart();
				break;
			}

		}
		else if (lcd_info_menu_item_was_click_selected()) {
		}
		else if (lcd_info_menu_item_was_hold_selected()) {
			switch (current_menu_item) {
			case MENUITEM_MAIN_PING:
				if (ping_timer == PING_TIMER_STOP) {
					int8_t updated = 1;
					if (edit_value_is_changed) { // if try to ping the same ip
						updated = edit_ip_value_update(ether.hisip);
						if (updated == 0) {
							lcd_control_message((const __FlashStringHelper *)F("Wrong IP!"), 1000);
						}
					}
					switch (updated) {
					case 0:
						lcd_info_current_menu_item_show();
						break;
					case 1:
						// Start ping...
						ping_timer = PING_TIMER_START;
						ping_test_send_and_lcd_update_response(true); // Force to show the ping ip title.
						break;
					}
				}
				break;
			case MENUITEM_MAIN_CONFIG_MY_MAC:
				DEBUG_PRINTLN(F("(SELECT to update) "));

				if (edit_value_is_changed) {
					menu_item* pcm = &menu_buffer[current_menu_item];
					size_t index = 0;
					size_t buffer_index = 0;
					str_hex_to_uint8_t(pcm->p_value, &index, abs(pcm->value_buffer_size) - 1, my_mac, &buffer_index, MAC_LENGTH);


					DEBUG_PRINTLN_WITH_TITLE(F("MAC changed: "), DEBUG_PRINT_HEX(my_mac, 0, MAC_LENGTH));


					//dp_listener_init();
					init_nic();
				}

				lcd_info_current_menu_item_edit_value_back();
				break;

			case MENUITEM_MAIN_CONFIG_TCPIP:
				DEBUG_PRINTLN(F("(SELECT to update) "));
				{
					menu_item* pcm = &menu_buffer[current_menu_item];
					uint8_t temp = hex2val(*pcm->p_value);
					// Static ip and Dhcp can force to do without edit_value_is_changed.
					if (edit_value_is_changed || tcpip_type == TCPIP_TYPE_DHCP || tcpip_type == TCPIP_TYPE_STATIC) { 
						tcpip_type = temp;
						init_tcpip();
					}
					lcd_info_current_menu_item_edit_value_back();
				}
				break;

			case MENUITEM_MAIN_SAVE_CONFIG:
				DEBUG_PRINTLN(F("(SELECT to save) "));

				if (edit_value_is_changed) {
					menu_item* pcm = &menu_buffer[current_menu_item];
					save_config = hex2val(*pcm->p_value);
					if (save_config == 1) {
						lcd_control_message((const __FlashStringHelper *)F("Writing EEPROM!"), 1000);

						eeprom_write_mac();
						eeprom_write_tcpip();
						eeprom_write_tcpip_addresses();

						save_config = 0;
						snprintnum(pcm->p_value, 2, save_config, 10);

						DEBUG_PRINTLN_WITH_TITLE(F("Save_config: "), DEBUG_PRINT_CHAR(pcm->p_value, 0, 1));
					}
				}

				lcd_info_current_menu_item_edit_value_back();

				break;
			}
		}

		break;
	case MENU_TCPIP:
		if (tcpip_type == TCPIP_TYPE_STATIC) {
			if (lcd_info_memu_item_was_not_selected()) {
				switch (current_menu_item) {
				case MENUITEM_TCPIP_MY_IP:
				case MENUITEM_TCPIP_NETMASK:
				case MENUITEM_TCPIP_GATEWAY_IP:
				case MENUITEM_TCPIP_DNS_IP:
					DEBUG_PRINTLN(F("(Select to edit) "));
		
					lcd_info_select_menu_item_set(current_menu, current_menu_item + MENUITEM_SELECT_BY_HOLD);
					lcd_info_current_menu_item_edit_value_move_cursor('.');
				}
			}
			else if (lcd_info_menu_item_was_click_selected()) {
			}
			else if (lcd_info_menu_item_was_hold_selected()) {
				DEBUG_PRINTLN(F("(Select to save) "));
				int8_t updated = -1;
				switch (current_menu_item) {
				case MENUITEM_TCPIP_MY_IP:
					// update my ip
					if (edit_value_is_changed) {
						updated = edit_ip_value_update(ether.myip);
						if (updated == 0) {
							lcd_control_message((const __FlashStringHelper *)F("Wrong IP!"), 1000);
						}
					}
					else {
						updated = 2;
					}
					break;
				case MENUITEM_TCPIP_NETMASK:
					// update netmask
					if (edit_value_is_changed) {
						updated = edit_ip_value_update(ether.netmask);
						if (updated == 0) {
							lcd_control_message((const __FlashStringHelper *)F("Wrong Netmask!"), 1000);
						}
					}
					else {
						updated = 2;
					}
					break;
				case MENUITEM_TCPIP_GATEWAY_IP:
					// update gateway
					if (edit_value_is_changed) {
						updated = edit_ip_value_update(ether.gwip);
						if (updated == 0) {
							lcd_control_message((const __FlashStringHelper *)F("Wrong Gateway!"), 1000);
						}	
					}
					else {
						updated = 2;
					}
					break;
				case MENUITEM_TCPIP_DNS_IP:
					// update dns ip
					if (edit_value_is_changed) {
						updated = edit_ip_value_update(ether.dnsip);
						if (updated == 0) {
							lcd_control_message((const __FlashStringHelper *)F("Wrong DNS!"), 1000);
						}
					}
					else {
						updated = 2;
					}
					break;
				}
				switch (updated) {
				case 0:
					lcd_info_current_menu_item_show();
					break;
				case 1:
				case 2: // Not changed, hold to set IP again.
					init_tcpip();
					lcd_info_current_menu_item_edit_value_back();
					break;
				}
			}
		}
		break;
	}

	DEBUG_PRINTLN_SPLIT_LINE();
}


void cdp_packet_handler_callback(const uint8_t packet[], size_t* p_packet_index, size_t packet_length, const uint8_t source_mac[]) {

	// https://www.cloudshark.org/captures/0ec97234cc09
	//
	// CDP data (Version 2)
	// #=======================================================...
	// # version       | ttl            | checksum | TLV       ...
	// # 1 byte (0x02) | 1 (?? seconds) | 2        |           ...
	// #=======================================================...
	//                                             /             \
	//    ________________________________________/               \
	//   /                                                         \
	//  / CDP TLV (marker = 0x99) // length value include type-length and data space
	// #============================================================...
	// #  type        | length | Data                               ...
	// #  2 bytes     | 2      |                                    ...
	// #============================================================...
	//                         +-------------...
	//   type = 0x0001         | device_id  ...
	//   (name)                | ~          ...
	//                         +-------------...
	//                         +-----------------------------------...
	//   type = 0x0005         | software_version (0x0a : new line)...
	//  (software_version)     | ~                                 ...
	//                         +-----------------------------------...
	//                         +------------...
	//   type = 0x0006         | platform   ...
	//   (platform)            | ~          ...
	//                         +------------...
	//                         +----------------------------------------------------------------------------------------------------...
	//   type = 0x0002         | num_of_addresses     |	IP address                                                                  ...
	//   (addresses)           | 4                    |                                                                             ...
	//                         +----------------------------------------------------------------------------------------------------...
	//                                                | protocol_type  | protocol_length | protocol    | address_length | address   ...
	//                                                | 1 (NLPID 0x01) | 1 (0x01)        | 1 (IP 0xcc) | 2              | 4         ...
	//                                                +-----------------------------------------------------------------------------...
	//                         +------------...
	//   type = 0x0003         | port_id    ...
	//   (port_id)             | ~          ...
	//                         +------------...
	//                         +----------------------+
	//   type = 0x0004         | capabilities         |
	//   (capabilities)        | 4 (0x000000028)      |
	//                         +----------------------+
	//                         .... .... .... .... .... .... .... ...0 = Router: No
	//                         .... .... .... .... .... .... .... ..0. = Transparent Bridge: No
	//                         .... .... .... .... .... .... .... .0.. = Source Route Bridge: No
	//                         .... .... .... .... .... .... .... 1... = Switch: Yes
	//                         .... .... .... .... .... .... ...0 .... = Host: No
	//                         .... .... .... .... .... .... ..1. .... = IGMP capable: Yes
	//                         .... .... .... .... .... .... .0.. .... = Repeater: No
	//
	//                         +--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
	//   type = 0x0008         | oui          | protocol_id | cluster_master | ip     | version | sub_version | status | UNKNOWN  | cluster_commander_mac | switch_mac | UNKNOWN  | management_vlan |
	//   (protocol_hello)      | 3 (0x00000c) | 2 (0x0112)  | 4              | 4      | 1       | 1           | 1      | 1 (0xff) | 6                     | 6          | 1 (0xff) | 2               |
	//                         +--------------------------------------------------------------------------------------------------------------------------------------------------------------------+	
	//                         +------------------------...
	//   type = 0x0009         | vtp_management_domain  ...
	//  (vtp_management_domain)| 0~                     ...
	//                         +------------------------...
	//                         +----------------+
	//   type = 0x000a         | native_vlan    |
	//   (native_vlan)         | 2              |
	//                         +----------------+
	//                         +----------------+
	//   type = 0x000b         | duplex         |
	//   (duplex)              | 1 (0x01 full)  |
	//                         +----------------+
	//                         +----------------+
	//   type = 0x0012         | trust_bitmap   |
	//   (trust_bitmap)        | 1 (0x00)       |
	//                         +----------------+
	//                         +------------------+
	//   type = 0x0013         | untrust_port_cos |
	//   (untrust_port_cos)    | 1 (0x00)         |
	//                         +------------------+
	//                         +----------------------------------------------------------------------------------------------------...
	//   type = 0x0016         | num_of_addresses     |	IP address                                                                  ...
	//   (management_address)  | 4                    |                                                                             ...
	//                         +----------------------------------------------------------------------------------------------------...
	//                                                | protocol_type  | protocol_length | protocol    | address_length | address   ...
	//                                                | 1 (NLPID 0x01) | 1 (0x01)        | 1 (IP 0xcc) | 2              | 4         ...
	//                                                +-----------------------------------------------------------------------------...
	//                         +------------------------------------------------------------------+
	//   type = 0x001a         | request_id   | management_id | power_available | power_available |
	//   (power_available)     | 2 (0x0000)   | 2 (0x0001)    | 4 (??mW)        | 4 (??mW)        |
	//                         +------------------------------------------------------------------+	
	//
	//

	//received_time_update();
	// To skip LLDP packet if select "Trace CDP/EDP", or when not select "Trace CDP/EDP".
	if (select_menu_item != MENUITEM_MAIN_TRACE_CDP_EDP) return;

	// current_menu_item to keep current_menu_item on old position of MENU_EDP
	lcd_info_current_menu_set(MENU_CDP, MENUSIZE_CDP, current_menu_item);

	//DEBUG_PRINTLN_WITH_TITLE(F("Recv from: "), DEBUG_PRINT_HEX(source_mac, 0, MAC_LENGTH));

	size_t buffer_index = 0;

	menu_item* pm = NULL;


	{
		size_t index = 0; // use source_mac, non use p_packet, need dummy to pass handleHexField 
		pm = lcd_info_menu_item_setup(MENUITEM_CDP_DEVICE_MAC, (const __FlashStringHelper *)F("Device MAC"), 
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_DEVICE_MAC);

		handleHexField(source_mac, &index, MAC_LENGTH,
			label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_DEVICE_MAC);
		buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

		//printf("%s: %s\n", pm->label, pm->value);
	}

	//int version = packet[(*p_packet_index)++];
	//if (version != 0x02) { // this code handle 0x02 version
	//	  DEBUG_PRINT_VAR(version);
	//}
	//printf("CDP version: %d\n", version);

	pm = lcd_info_menu_item_setup(MENUITEM_CDP_VERSION, (const __FlashStringHelper *)F("CDP version"), 
									label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_VERSION);
	handleNumField(packet, p_packet_index, 1,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_VERSION);
	buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'


	int ttl = packet[(*p_packet_index)++];
	//DEBUG_PRINTLN_VAR(ttl);

	uint16_t checksum = (packet[*p_packet_index] << 8) | packet[*p_packet_index + 1];
	*p_packet_index += 2;
	//DEBUG_PRINTLN_WITH_TITLE(F("Checksum: "), DEBUG_PRINT_A_HEX(checksum >> 8);
	//										DEBUG_PRINT_A_HEX(checksum & 0xFF));

#define CDP_TLV_TYPE_DEVICE_ID			0x0001
#define CDP_TLV_TYPE_ADDRESSES			0x0002
#define CDP_TLV_TYPE_PORT_ID			0x0003
#define CDP_TLV_TYPE_CAPABILITIES		0x0004
#define CDP_TLV_TYPE_SOFTWARE			0x0005
#define CDP_TLV_TYPE_PLATFORM			0x0006
#define CDP_TLV_TYPE_NATIVE_VLAN		0x000a
#define CDP_TLV_TYPE_DUPLEX				0x000b

	while (*p_packet_index < packet_length) { // read all remaining TLV fields
		uint16_t tlv_type = (packet[*p_packet_index] << 8) | packet[*p_packet_index + 1];
		*p_packet_index += 2;
		uint16_t tlv_length = (packet[*p_packet_index] << 8) | packet[*p_packet_index + 1];
		*p_packet_index += 2;
		tlv_length -= 4; // the length include this tlv marker size.

		uint16_t next_tlv = *p_packet_index + tlv_length;


		switch (tlv_type) {
		case CDP_TLV_TYPE_DEVICE_ID:
			pm = lcd_info_menu_item_setup(MENUITEM_CDP_DEVICE_ID, (const __FlashStringHelper *)F("Device id"),
											label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_DEVICE_ID);
			handleAsciiField(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_DEVICE_ID);
			buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

							//printf("%s: %s\n", pm->label, pm->value);
			break;

		case CDP_TLV_TYPE_ADDRESSES:
			/*
			unsigned long addresses_count = (packet[*p_packet_index] << 24)
			| (packet[*p_packet_index + 1] << 16)
			| (packet[*p_packet_index + 2] << 8)
			| packet[*p_packet_index + 3];
			*p_packet_index += 4
			// it should not to handle many IP
			if (address_count > 4) break;

			uint16_t protocol_type = packet[(*p_buffer_index)++];
			uint16_t protocol_length = packet[(*p_buffer_index)++];
			// byte protocol[8];
			// for(uint16_t j=0; j<protoLength; ++j) {
			//	proto[j] = packet[(*p_buffer_index)++];
			//}
			*p_buffer_index += protocol_length;
			uint16_t address_length = (packet[*p_packet_index] << 8) | packet[*p_packet_index + 1];
			*p_packet_index += 2

			buffer_index = 0;
			handleMultiField(&handleNumField,
			packet, p_packet_index, 1,
			value_ip_buffer, &buffer_index, sizeof(value_ip_buffer),
			IP_LENGTH, '.', (uint8_t)address_count, ',');

			*/
			pm = lcd_info_menu_item_setup(MENUITEM_CDP_ADDRESSES, (const __FlashStringHelper *)F("Device Address"), 
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_ADDRESSES);
			handleCdpAddresses(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_ADDRESSES);
			buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

							//printf("%s: %s\n", pm->label, pm->value);
			break;

		case CDP_TLV_TYPE_PORT_ID:
			//buffer_index = 0;
			pm = lcd_info_menu_item_setup(MENUITEM_CDP_PORT_ID, (const __FlashStringHelper *)F("Port id"), 
									label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_PORT_ID);
			handleAsciiField(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_PORT_ID);
			buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

			break;

			// case CDP_TLV_TYPE_CAPABILITIES:
			//		handleCdpCapabilities(packet, packet_index, tlv_length);
			//		break;

		case CDP_TLV_TYPE_SOFTWARE:
			pm = lcd_info_menu_item_setup(MENUITEM_CDP_SOFTWARE, (const __FlashStringHelper *)F("Software"), 
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_SOFTWARE );
			handleAsciiField(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_SOFTWARE);
			buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

							//printf("%s: %s\n", pm->label, pm->value);
			break;

		case CDP_TLV_TYPE_PLATFORM:
			//buffer_index = 0;
			pm = lcd_info_menu_item_setup(MENUITEM_CDP_PLATFORM, (const __FlashStringHelper *)F("Platform"), 
									label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_PLATFORM);
			handleAsciiField(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_PLATFORM);
			buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

							//printf("%s: %s\n", pm->label, pm->value);
			break;

		case CDP_TLV_TYPE_NATIVE_VLAN:
			pm = lcd_info_menu_item_setup(MENUITEM_CDP_NATIVE_VLAN, (const __FlashStringHelper *)F("Native vlan"), 
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_NATIVE_VLAN);
			handleNumField(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_NATIVE_VLAN);
			buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

							//printf("%s: %s\n", pm->label, pm->value);
			break;

		case CDP_TLV_TYPE_DUPLEX:
		{
			pm = lcd_info_menu_item_setup(MENUITEM_CDP_DUPLEX, (const __FlashStringHelper *)F("Duplex"),
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_DUPLEX);
			uint8_t duplex = packet[(*p_packet_index)++];
			const uint8_t full[] = { 0x46, 0x75, 0x6c, 0x6c }; // Full
			const uint8_t half[] = { 0x48, 0x61, 0x6c, 0x66 }; // Half
			size_t index = 0;
			handleAsciiField((duplex == 0x01 ? full : half), &index, 4,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_CDP_DUPLEX);
			buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

							//printf("%s: %s\n", pm->label, pm->value);
		}
		break;

		default:
			// TODO: raw field
			//DEBUG_PRINTLN_WITH_TITLE(F("Field: "), 
			//	DEBUG_PRINT_A_HEX(tlv_type >> 8);
			//	DEBUG_PRINT_A_HEX(tlv_type & 0xFF); 
			//	DEBUG_PRINT(", Length: ");
			//	DEBUG_PRINT_A_DEC(tlv_length););
			break;
		}

		//*p_packet_index += tlv_length;
		*p_packet_index = next_tlv;
	}

	// ensure to show the first visible item in menu
	lcd_info_current_menu_item_ensure_visible(DIRECTION_NEXT, true);
}



void edp_packet_handler_callback(const uint8_t packet[], size_t* p_packet_index, size_t packet_length, const uint8_t source_mac[]) {

	// https://www.cloudshark.org/captures/b2e59aeb2e4c?filter=frame%20and%20eth%20and%20llc%20and%20edp
	//
	// EDP data (Version 1)
	// #=========================================================================================================...
	// # version       | reserved | data_length | checksum | sequence_number | machine_id | machine_mac | TLV    ...
	// # 1 byte (0x01) | 1        | 2           | 2        | 2               | 2          | 6           | ~      ...
	// #==========================================================================================================...
	//                                                                                                 /           \
	//    ____________________________________________________________________________________________/             \
	//   /                                                                                                           \
	//  / EDP TLV (marker = 0x99)  // length value include marker-type-length and data space                          \
	// #==============================================================================================================...
	// # marker        | type | length | Data                                                                         ...
	// # 1 byte (0x99) | 1    | 2      | ~                                                                            ...
	// #==============================================================================================================...
	//                                 +---------------------------------------------------------------+
	//               type = 0x02       | slot | port | virt_chassis | reserved | version | connections |
	//               (info)            | 2    | 2    | 2            | 6        | 4       | ~           |
	//                                 +---------------------------------------------------------------+
	//                                 +-------------------------------------------------------------------+
	//               type = 0x01       | name                                                              |
	//               (display)         | 256                                                               |
	//                                 +-------------------------------------------------------------------+
	//                                 ++
	//               type = 0x00       ||
	//               (null)            || 
	//                                 ++
	//
	//


	//received_time_update();
	// To skip LLDP packet if select "Trace CDP/EDP", or when not select "Trace CDP/EDP".
	if (select_menu_item != MENUITEM_MAIN_TRACE_CDP_EDP) return;

	// current_menu_item to keep current_menu_item on old position of MENU_EDP
	lcd_info_current_menu_set(MENU_EDP, MENUSIZE_EDP, current_menu_item); 

	//DEBUG_PRINTLN_WITH_TITLE(F("Recv from: "), DEBUG_PRINT_HEX(source_mac, 0, MAC_LENGTH));
	

	size_t buffer_index = 0;

	menu_item* pm = NULL;

	//int version = packet[(*p_packet_index)++];

	pm = lcd_info_menu_item_setup(MENUITEM_EDP_VERSION, (const __FlashStringHelper *)F("EDP version"), 
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_VERSION);
	handleNumField(packet, p_packet_index, 1,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_VERSION);
	buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'

	// version + reserved + data_length + checksum + sequence_number + machine_id
	*p_packet_index += 9;


	// device mac
	pm = lcd_info_menu_item_setup(MENUITEM_EDP_DEVICE_MAC, (const __FlashStringHelper *)F("Device MAC"),
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_DEVICE_MAC);
	handleHexField(packet, p_packet_index, MAC_LENGTH,
		label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_DEVICE_MAC);
	buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'
					//printf("%s: %s\n", pm->label, pm->value);

	//DEBUG_PRINTLN_VAR(buffer_index);
	//DEBUG_PRINTLN_VAR(pm->p_label);
	//DEBUG_PRINTLN_VAR(pm->p_value);


#define EDP_TLV_MARKER       0x99
#define EDP_TLV_TYPE_INFO    0x02
#define EDP_TLV_TYPE_DISPLAY 0x01
#define EDP_TLV_TYPE_NULL    0x00



	while (*p_packet_index < packet_length) {
		if (packet[*p_packet_index] == EDP_TLV_MARKER) {
			(*p_packet_index)++;
			uint16_t tlv_type = packet[(*p_packet_index)++];
			uint16_t tlv_length = (packet[*p_packet_index] << 8) | packet[*p_packet_index + 1];
			*p_packet_index += 2;
			tlv_length -= 4; // the length include this tlv marker size.

			uint16_t next_tlv = *p_packet_index + tlv_length;


			switch (tlv_type) {
			case EDP_TLV_TYPE_INFO:
				// device port and slot
				pm = lcd_info_menu_item_setup(MENUITEM_EDP_SLOT_PORT, (const __FlashStringHelper *)F("Slot-port"), 
											label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_SLOT_PORT);
				handleMultiField(&handleNumField,
					packet, p_packet_index, 2,
					label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_SLOT_PORT,
					2, ':', 1, ',', 
					MODIFIER_OFFSET, 1);
				buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'
								//printf("%s: %s\n", pm->label, pm->value);

				//DEBUG_PRINTLN_VAR(buffer_index);
				//DEBUG_PRINTLN_VAR(pm->p_label);
				//DEBUG_PRINTLN_VAR(pm->p_value);

								// virt_chassis + reserved, don't want to display
				*p_packet_index += 8;

				// device version
				pm = lcd_info_menu_item_setup(MENUITEM_EDP_SOFTWARE_VERSION, (const __FlashStringHelper *)F("Version"), 
											label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_SOFTWARE_VERSION);
				handleMultiField(&handleNumField,
					packet, p_packet_index, 1,
					label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_SOFTWARE_VERSION,
					4, '.', 1, ',');
				buffer_index++; // skip the '\0'
								//printf("%s: %s\n", pm->label, pm->value);
				//DEBUG_PRINTLN_VAR(buffer_index);
				//DEBUG_PRINTLN_VAR(pm->p_label);
				//DEBUG_PRINTLN_VAR(pm->p_value);
				break;

			case EDP_TLV_TYPE_DISPLAY:
				pm = lcd_info_menu_item_setup(MENUITEM_EDP_DEVICE_NAME, (const __FlashStringHelper *)F("Device name"), 
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_DEVICE_NAME);
				handleAsciiField(packet, p_packet_index, 256,
					label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_EDP_DEVICE_NAME);
				buffer_index++; // skip the '\0'
								//printf("%s: %s\n", pm->label, pm->value);
				//DEBUG_PRINTLN_VAR(buffer_index);
				//DEBUG_PRINTLN_VAR(pm->p_label);
				//DEBUG_PRINTLN_VAR(pm->p_value);
				break;

			case EDP_TLV_TYPE_NULL:
				// the tail of edp, nothing to do and will exit loop
				break;
			}
			*p_packet_index = next_tlv;
		}
		else {
			*p_packet_index++; // skip character
		}
	}

	// ensure to show the first visible item in menu
	lcd_info_current_menu_item_ensure_visible(DIRECTION_NEXT, true);
}


void lldp_packet_handler_callback(const uint8_t packet[], size_t* p_packet_index, size_t packet_length, const uint8_t source_mac[]) {

	// http://www.ieee802.org/1/files/public/docs2002/LLDP%20Overview.pdf
	// http://enterprise.huawei.com/ilink/enenterprise/download/HW_U_150294
	// http://enterprise.huawei.com/ilink/enenterprise/download/HW_U_149738
	// http://support.huawei.com/enterprise/docinforeader.action?contentId=DOC0100523127&partNo=10032
	//
	// http://www.brocade.com/content/html/en/configuration-guide/NI_05800a_SWITCHING/GUID-19D1919F-4BB1-42B3-A683-CECFF8094D5A.html
	// http://www.brocade.com/content/html/en/configuration-guide/NI_05800a_SWITCHING/GUID-B6236268-D4BB-400D-A8F1-32D2616FFA69.html
	// http://www.brocade.com/content/html/en/configuration-guide/NI_05800a_SWITCHING/GUID-E0A30BCB-A57C-4DA2-8A4F-5FA3450C8C40.html
	//
	//
	//  TLV type
	//   +------------+--------------------------------------+
	//   | TLV type   | TLV name           | Usage in LLDPDU |
	//   +------------+--------------------+-----------------+
	//   | 0          | End Of LLDPPDU     | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 1          | Chassis ID         | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 2          | Port ID            | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 3          | Time To Live       | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 4          | Port Description   | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 5          | System Name        | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 6          | System Description | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 7          | System Capabilities| Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 8          | Management Address | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 9~126      | reserved           | Mandatory       |
	//   +------------+--------------------+-----------------+
	//   | 127        | Organizationally   | Mandatory       |
	//   |            | specific TLVs      |                 |
	//   +------------+--------------------+-----------------+
	//
	//
	//  Table 20 Chassis ID subtypes
	//   +------------+-------------------+
	//   | ID Subtype | Description       |
	//   +------------+-------------------+
	//   | 0          | Reserved          |
	//   +------------+-------------------+
	//   | 1          | Chassis component |
	//   +------------+-------------------+
	//   | 2          | Interface alias   |
	//   +------------+-------------------+
	//   | 3          | Port component    |
	//   +------------+-------------------+
	//   | 4          | MAC address       |
	//   +------------+-------------------+
	//   | 5          | Network address   |
	//   +------------+-------------------+
	//   | 6          | Interface name    |
	//   +------------+-------------------+
	//   | 7          | Locally assigned  |
	//   +------------+-------------------+
	//   | 8 - 255    | Reserved          |
	//   +------------+-------------------+
	//
	//  Table 21 Port ID subtypes
	//   +------------+-------------------+
	//   | ID Subtype | Description       |
	//   +------------+-------------------+
	//   | 0          | Reserved          |
	//   +------------+-------------------+
	//   | 1          | Interface alias   |
	//   +------------+-------------------+
	//   | 2          | Port component    |
	//   +------------+-------------------+
	//   | 3          | MAC address       |
	//   +------------+-------------------+
	//   | 4          | Network address   |
	//   +------------+-------------------+
	//   | 5          | Interface name    |
	//   +------------+-------------------+
	//   | 6          | Agent circuit ID  |
	//   +------------+-------------------+
	//   | 7          | Locally assigned  |
	//   +------------+-------------------+
	//   | 8 - 255    | Reserved          |
	//   +------------+-------------------+
	//
	// #==============================================================================================================...
	// # tlv_type   | length           | Data                                                                         ...
	// # 7 bits     | 9 bits           | ~                                                                            ...
	// #==============================================================================================================...
	// 
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LLDPPDU MANDATORY TLV !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
	//                                 +--------------------------------------------------...
	//               type = b0000 001  | chassis_id_subtype        |                      ...
	//               (chassis_id)      | 1 bytes                   |                      ...
	//            length = 1 + 6 bytes +--------------------------------------------------...
	//                                   chassis_id_subtype = 0x04 | chassis_id        |
	//                                   (MAC address)             | 6 bytes (MAC)     |     
	//                                                             +----------------------...
	//                                   chassis_id_subtype = 0x01 | chassis_id           ...
	//                                   (Chassis component)       | ~                    ...
	//                                                             +----------------------...
	//                                   chassis_id_subtype = 0x02 | chassis_id           ...
	//                                   (Interface alias )        | ~                    ...    
	//                                                             +----------------------...
	//                                   chassis_id_subtype = 0x03 | chassis_id           ...
	//                                   (Port component  )        | ~                    ...    
	//                                                             +----------------------...
	//                                   chassis_id_subtype = 0x05 | chassis_id           ...
	//                                   (Network address )        | ~                    ...    
	//                                                             +----------------------...
	//                                   chassis_id_subtype = 0x06 | chassis_id           ...
	//                                   (Interface name)          | ~                    ...    
	//                                                             +----------------------...
	//                                   chassis_id_subtype = 0x07 | chassis_id           ...
	//                                   (Locally assigned)        | ~                    ...    
	//                                                             +----------------------...
	//
	//                                 +---------------------------------------------...
	//               type = b0000 002  | port_id_subtype           |                 ...
	//               (port_id)         | 1 bytes                   |                 ...
	// length = 1 + len(port_id) bytes +---------------------------------------------...
	//                                   port_id_subtype = 0x01    | port_id        ...
	//                                   (Interface alias)         | ~             ...
	//                                                             +---------------...
	//                                   port_id_subtype = 0x05    | port_id       ...
	//                                   (Interface name)          | ~             ...
	//                                                             +---------------...
	//                                   port_id_subtype = 0x02    | port_id       ...
	//                                   (Port component)          | ~             ...
	//                                                             +----------------+
	//                                   port_id_subtype = 0x03    | port_id        |
	//                                   (MAC address)             | 6 bytes (MAC)  |
	//                                                             +----------------+
	//                                   port_id_subtype = 0x04    | port_id       ...
	//                                   (Network address )        | ~             ...
	//                                                             +---------------...
	//                                   port_id_subtype = 0x06    | port_id       ...
	//                                   (Agent circuit ID )       | ~             ...
	//                                                             +---------------...
	//                                   port_id_subtype = 0x07    | port_id       ...
	//                                   (Locally assigned)        | ~             ...
	//                                                             +---------------...
	//
	//                                 +------------------------------+
	//               type = b0000 003  | ttl_seconds                  |
	//               (ttl)             | 2 bytes                      |  
	//                length = 2 bytes +------------------------------+
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LLDPPDU MANDATORY TLV !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
	//
	//                                 +------------------------------...
	//               type = b0000 004  | port_description             ...
	//               (port_description)| ~					          ...
	//                                 +------------------------------...
	//                                 +------------------------------...
	//               type = b0000 005  | system_name                  ...
	//               (system_name)     | ~                            ... 
	//                                 +------------------------------...
	//                                 +------------------------------+
	//               type = b0000 006  | system_capabilities          |
	//           (system_capabilities) | 4 bytes (0x0014 0x0004)      |              ... 
	//                                 +------------------------------+
	//                                 capabilities 
	//                                 .... .... .... ...0 = Other: Not capable
	//                                 .... .... .... ..0. = Repeater: Not capable
	//                                 .... .... .... .1.. = Bridge: Capable
	//                                 .... .... .... 0... = WLAN access point: Not capable
	//                                 .... .... ...1 .... = Router: Capable
	//                                 .... .... ..0. .... = Telephone: Not capable
	//                                 .... .... .0.. .... = DOCSIS cable device: Not capable
	//                                 .... .... 0... .... = Station only: Not capable
	//
	//                                 enabled_capabilities
	//                                 .... .... .... ...0 = Other: Not capable
	//                                 .... .... .... ..0. = Repeater: Not capable
	//                                 .... .... .... .1.. = Bridge: Capable
	//                                 .... .... .... 0... = WLAN access point: Not capable
	//                                 .... .... ...0 .... = Router: Not capable
	//                                 .... .... ..0. .... = Telephone: Not capable
	//                                 .... .... .0.. .... = DOCSIS cable device: Not capable
	//                                 .... .... 0... .... = Station only: Not capable
	//
	//                                 +------------------------------------------+--------------------------...
	//               type = b0000 007  | address_string_length | address_subtype  |
	//           (management_address)  | 1 bytes (7)           | 1 bytes          |
	//                                 +-----------------------+------------------+--------------------------...
	//                                                    address_subtype = 0x06  | management_mac_address | 
	//                                                  (management MAC address)  | 6 bytes                |
	//                                                                            +------------------------+
	//                                 +---------------------------------------------...
	//                                 | interface_subtype     |
	//                                 | 1 bytes               | 
	//                                 +---------------------------------------------...
	//                                 +---------------------------------+
	//                                 | interface number                | 
	//                                 | 4 bytes                         | 
	//                                 +---------------------------------+
	//                                 +-----------------------+
	//                                 | oid_string_length     |
	//                                 | 1 bytes (0)           |
	//                                 +-----------------------+
	//                                 ++
	//               type = b0000 000  ||
	//               (end_of_lldpdu)   || 
	//                                 ++
	//

	//received_time_update();
	// To skip CDP/EDP packet if select "Trace LLDP", or when not select "Trace LLDP".
	if (select_menu_item != MENUITEM_MAIN_TRACE_LLDP) return;

	// current_menu_item to keep current_menu_item on old position of MENU_EDP
	lcd_info_current_menu_set(MENU_LLDP, MENUSIZE_LLDP, current_menu_item);


	size_t buffer_index = 0;

	menu_item* pm = NULL;

#define LLDP_TLV_TYPE_CHASSIS                            0x01
#define LLDP_TLV_TYPE_PORT                               0x02
#define LLDP_TLV_TYPE_TTL                                0x03
#define LLDP_TLV_TYPE_PORT_DESCRIPTION                   0x04
#define LLDP_TLV_TYPE_SYSTEM_NAME                        0x05
#define LLDP_TLV_TYPE_SYSTEM_DESCRIPTION                 0x06
#define LLDP_TLV_TYPE_SYSTEM_CAPABILITIES                0x07
#define LLDP_TLV_TYPE_MANAGEMENT_ADDRESS                 0x08

#define LLDP_TLV_TYPE_CHASSIS_SUBTYPE_CHASSIS_COMPONENT  0x01
#define LLDP_TLV_TYPE_CHASSIS_SUBTYPE_INTERFACE_ALIAS    0x02
#define LLDP_TLV_TYPE_CHASSIS_SUBTYPE_PORT_COMPONENT     0x03
#define LLDP_TLV_TYPE_CHASSIS_SUBTYPE_MAC_ADDRESS        0x04
#define LLDP_TLV_TYPE_CHASSIS_SUBTYPE_NETWORK_ADDRESS    0x05
#define LLDP_TLV_TYPE_CHASSIS_SUBTYPE_INTERFACE_NAME     0x06
#define LLDP_TLV_TYPE_CHASSIS_SUBTYPE_LOCALLY_ASSIGNED   0x07


#define LLDP_TLV_TYPE_PORT_SUBTYPE_INTERFACE_ALIAS       0x01
#define LLDP_TLV_TYPE_PORT_SUBTYPE_PORT_COMPONENT        0x02
#define LLDP_TLV_TYPE_PORT_SUBTYPE_MAC_ADDRESS           0x03
#define LLDP_TLV_TYPE_PORT_SUBTYPE_NETWORK_ADDRESS       0x04
#define LLDP_TLV_TYPE_PORT_SUBTYPE_INTERFACE_NAME        0x05
#define LLDP_TLV_TYPE_PORT_SUBTYPE_AGENT_CIRCUIT_ID      0x06
#define LLDP_TLV_TYPE_PORT_SUBTYPE_LOCALLY_ASSIGNED      0x07

	while (*p_packet_index < packet_length) { // read all remaining TLV fields
		uint8_t tlv_type = (packet[*p_packet_index] >> 1);
		uint16_t tlv_length = ((packet[*p_packet_index] & 0x01) << 8) | packet[*p_packet_index + 1];
		*p_packet_index += 2;
		// the length not include this tlv marker size, only count the data

		uint16_t next_tlv = *p_packet_index + tlv_length;

		switch (tlv_type) {
		case LLDP_TLV_TYPE_CHASSIS: // MANDATORY TLV
		{
			uint8_t subtype = packet[(*p_packet_index)++];
			switch (subtype) {
			case LLDP_TLV_TYPE_CHASSIS_SUBTYPE_CHASSIS_COMPONENT:
			case LLDP_TLV_TYPE_CHASSIS_SUBTYPE_INTERFACE_ALIAS:
			case LLDP_TLV_TYPE_CHASSIS_SUBTYPE_PORT_COMPONENT:
				break;
			case LLDP_TLV_TYPE_CHASSIS_SUBTYPE_MAC_ADDRESS:
				tlv_length--; // subtype 1 bytes
				pm = lcd_info_menu_item_setup(MENUITEM_LLDP_DEVICE_MAC, (const __FlashStringHelper *)F("Device MAC"), 
											label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_MAC);
				handleHexField(packet, p_packet_index, MAC_LENGTH,
					label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_MAC);
				buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'
								//lcd_info_menu_item_setup(MENUITEM_DEVICE_MAC, value_mac_buffer);
				//printf("%s: %s\n", pm->label, pm->value);
				break;
			case LLDP_TLV_TYPE_CHASSIS_SUBTYPE_NETWORK_ADDRESS:
			case LLDP_TLV_TYPE_CHASSIS_SUBTYPE_INTERFACE_NAME:
			case LLDP_TLV_TYPE_CHASSIS_SUBTYPE_LOCALLY_ASSIGNED:
				break;

			}

		}
		break;
		case LLDP_TLV_TYPE_PORT:    // MANDATORY TLV
		{
			uint8_t subtype = packet[(*p_packet_index)++];
			switch (subtype) {
			case LLDP_TLV_TYPE_PORT_SUBTYPE_INTERFACE_ALIAS:
			case LLDP_TLV_TYPE_PORT_SUBTYPE_PORT_COMPONENT:
			case LLDP_TLV_TYPE_PORT_SUBTYPE_INTERFACE_NAME:
				tlv_length--; // subtype 1 bytes
				pm = lcd_info_menu_item_setup(MENUITEM_LLDP_PORT_ID, (const __FlashStringHelper *)F("Port id"), 
									label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_LLDP_PORT_ID);
				handleAsciiField(packet, p_packet_index, tlv_length,
					label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_LLDP_PORT_ID);
				buffer_index++; // skip the '\0', if concat next string, don't skip the '\0'
								//lcd_info_menu_item_setup(MENUITEM_DEVICE_MAC, value_mac_buffer);
				//printf("%s: %s\n", pm->label, pm->value);
				break;
			
			case LLDP_TLV_TYPE_PORT_SUBTYPE_MAC_ADDRESS:
			case LLDP_TLV_TYPE_PORT_SUBTYPE_NETWORK_ADDRESS:
			case LLDP_TLV_TYPE_PORT_SUBTYPE_AGENT_CIRCUIT_ID:
			case LLDP_TLV_TYPE_PORT_SUBTYPE_LOCALLY_ASSIGNED:
				break;
			}
		}
		break;
		case LLDP_TLV_TYPE_TTL:     // MANDATORY TLV
			break;

		case LLDP_TLV_TYPE_PORT_DESCRIPTION:
			pm = lcd_info_menu_item_setup(MENUITEM_LLDP_PORT_DESCRIPTION, (const __FlashStringHelper *)F("Port desc"),
										label_value_buffer, &buffer_index, MENUITEM_LLDP_PORT_DESCRIPTION);
			handleAsciiField(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_LLDP_PORT_DESCRIPTION);
			buffer_index++; // skip the '\0'
							//lcd_info_menu_item_setup(MENUITEM_DEVICE_NAME, value_name_buffer);
			//printf("%s: [%s]\n", pm->label, pm->value);
			break;

		case LLDP_TLV_TYPE_SYSTEM_NAME:
			pm = lcd_info_menu_item_setup(MENUITEM_LLDP_DEVICE_NAME, (const __FlashStringHelper *)F("Device name"), 
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_NAME);
			handleAsciiField(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_NAME);
			buffer_index++; // skip the '\0'
							//lcd_info_menu_item_setup(MENUITEM_DEVICE_NAME, value_name_buffer);
			//printf("%s: %s\n", pm->label, pm->value);
			break;

		case LLDP_TLV_TYPE_SYSTEM_DESCRIPTION:
			pm = lcd_info_menu_item_setup(MENUITEM_LLDP_DEVICE_DESCRIPTION, (const __FlashStringHelper *)F("Device desc"),
										label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_DESCRIPTION);
			handleAsciiField(packet, p_packet_index, tlv_length,
				label_value_buffer, &buffer_index, MENUITEM_VALUEBUFFERSIZE_LLDP_DEVICE_DESCRIPTION);
			buffer_index++; // skip the '\0'
							//lcd_info_menu_item_setup(MENUITEM_DEVICE_NAME, value_name_buffer);
			//printf("%s: [%s]\n", pm->label, pm->value);
			break;

		case LLDP_TLV_TYPE_SYSTEM_CAPABILITIES:
			break;
		case LLDP_TLV_TYPE_MANAGEMENT_ADDRESS:
			break;
		}
		*p_packet_index = next_tlv;
	}
	// ensure to show the first visible item in menu
	lcd_info_current_menu_item_ensure_visible(DIRECTION_NEXT, true);
}


void other_packet_handler_callback(const uint8_t packet[], size_t* p_packet_index, size_t packet_length, const uint8_t source_mac[]) {
	//received_time_update();
}

void icmp_packet_handler_callback(const uint8_t packet[], size_t* p_packet_index, size_t packet_length, const uint8_t source_mac[]) {
	//received_time_update();
}

void update_lines_callback() {
	uint8_t value = 0;
	menu_item* pcm;
	switch (current_menu) {
	case MENU_MAIN:
		switch (current_menu_item) {
		case MENUITEM_MAIN_CONFIG_TCPIP:
		case MENUITEM_MAIN_SAVE_CONFIG:
			pcm = &menu_buffer[current_menu_item];
			value = hex2val(*pcm->p_value);

			lcd.setCursor(1, 1);
			switch (current_menu_item) {
			case MENUITEM_MAIN_CONFIG_TCPIP:
				switch (value) {
				case 0: lcd.print(F(":None")); break;
				case 1: lcd.print(F(":DHCP")); break;
				case 2: lcd.print(F(":Static")); break;
				}
				break;
			case MENUITEM_MAIN_SAVE_CONFIG:
				switch (value) {
				case 0: lcd.print(F(":No")); break;
				case 1: lcd.print(F(":Yes")); break;
				}
				break;
			}

			break;
		}
		break;
	}
}


void eeprom_read_mac() {
	if (EEPROM.read(0) == 1) { 
		for (int8_t i = 0; i < MAC_LENGTH; i++) {
			my_mac[i] = (uint8_t)EEPROM.read(1 + i);
		}
	}
}

void eeprom_write_mac() {
	EEPROM.write(0, 1);
	for (int8_t i = 0; i < MAC_LENGTH; i++) {
		EEPROM.write(1 + i, my_mac[i]);
	}
}


void eeprom_read_tcpip() {
	if (EEPROM.read(7) == 1) { 
		tcpip_type = (uint8_t)EEPROM.read(8);
	}
}

void eeprom_write_tcpip() {
	EEPROM.write(7, 1);
	EEPROM.write(8, tcpip_type);
}

void eeprom_read_tcpip_addresses() {
	if (tcpip_type == TCPIP_TYPE_STATIC) {
		if (EEPROM.read(9) == 1) { 
			for (int8_t i = 0; i < IP_LENGTH; i++) {
				ether.myip[i] = (uint8_t)EEPROM.read(10 + i);
				ether.netmask[i] = (uint8_t)EEPROM.read(14 + i);
				ether.gwip[i] = (uint8_t)EEPROM.read(18 + i);
				ether.dnsip[i] = (uint8_t)EEPROM.read(22 + i);
			}
		}
	}

}

void eeprom_write_tcpip_addresses() {
	if (tcpip_type == TCPIP_TYPE_STATIC) {
		EEPROM.write(9, 1);
		for (int8_t i = 0; i < IP_LENGTH; i++) {
			EEPROM.write(10 + i, ether.myip[i]);
			EEPROM.write(14 + i, ether.netmask[i]);
			EEPROM.write(18 + i, ether.gwip[i]);
			EEPROM.write(22 + i, ether.dnsip[i]);
		}
	}
}


inline void ping_test_recv() {
	ping_response_time = (micros() - ping_timer) * 0.001;
	//DEBUG_PRINTLN_VAR(ping_response_time);
}

void ping_test_send_and_lcd_update_response(bool enabled) {
	static char lcd_line1[LCD_WIDTH - 4] = { '\0' };
	if (enabled) {
		if (ping_response_time != -1) {
			if (ping_dot_pos > LCD_WIDTH - 5) {
				//lcd.scrollDisplayLeft();
				memcpy(&lcd_line1[0], &lcd_line1[1], LCD_WIDTH - 5);
				ping_dot_pos = LCD_WIDTH - 5;
			}

			lcd_line1[ping_dot_pos] = (ping_response_time == 0) ? '.' : '!';
			lcd_line1[ping_dot_pos+1] = '\0';
			ping_dot_pos++;

			lcd.setCursor(0, 1);
			lcd.print(ping_response_time);
			lcd.print(F("   "));

			lcd.setCursor(4, 1);
			lcd.print(lcd_line1);
		}
		else {
			// first time, to print ping ip title.
			menu_item* pcm = &menu_buffer[current_menu_item];
			lcd.noCursor();
			lcd.clear();
			lcd.print(pcm->p_value);
			
		}
		ping_timer = micros();
		ping_response_time = 0;

		ether.clientIcmpRequest(ether.hisip);
	}
	else {
		if (ping_response_time != -1) {
			ping_response_time = -1;
			ping_dot_pos = 0;
			memset(lcd_line1, 0, LCD_WIDTH - 4);
		}
	}
}

float battery_voltage(int* analogread = NULL) {
#define MULTIMETER_TEST           4.04; // V
#define PRE_ANALOGREAD            881;
	// Vcc for voltage reference,
	// to resolved the real Vcc voltage.
	//
	// Having:
	//  881     4.04
	// ------ = ------
	//  1023     Vcc
	//
	//  So, Vcc = 1023 * 4040 / 881
	//
	// To calculate aother analogread associate voltage
	//
	//  V = analogread * Vcc / 1023
	//    = analogread * 4040 / 881
	int analogamount = analogRead(PIN_BATTERY_VOLTAGE);
	if (analogread != NULL)
		*analogread = analogamount;
	float correct = MULTIMETER_TEST;
	correct /= PRE_ANALOGREAD;
	float voltage = analogamount * correct;
	return voltage;
}

void setup() {
	//wdt_disable(); //always good to disable it, if it was left 'on' or you need init time
	DEBUG_BEGIN(115200);
	lcd_control_init();

	btnUp.onClick    = btnUp_click_hold;
	btnUp.onHold     = btnUp_click_hold;
	btnDown.onClick  = btnDown_click_hold;
	btnDown.onHold   = btnDown_click_hold;
	btnLeft.onClick  = btnLeft_click_hold;
	btnLeft.onHold   = btnLeft_click_hold;
	btnRight.onClick = btnRight_click;
	btnRight.onHold  = btnRight_hold;

	// battery voltage
	pinMode(PIN_BATTERY_VOLTAGE, INPUT);
	analogReference(DEFAULT);
	int analogread;
	float voltage = battery_voltage(&analogread);
	
	lcd.clear();
	lcd.print(F("VBAT: "));
	lcd.setCursor(6, 0);
	lcd.print(voltage, 2);
	lcd.setCursor(0, 1);
	lcd.print(analogread);
	lcd.setCursor(5, 1);
	lcd.print((analogread >= 3.5) ? F("Battery OK") : F("Low Battery"));
	delay(1000);

	cdp_packet_handler = cdp_packet_handler_callback;
	edp_packet_handler = edp_packet_handler_callback;
	lldp_packet_handler = lldp_packet_handler_callback;

	other_packet_handler = other_packet_handler_callback;
	icmp_packet_handler  = icmp_packet_handler_callback;
	
	lcd_control_update_lines_callback_fn = update_lines_callback;
	
	
	//dhcp_listener_init();

	//lcd_control_done();
	eeprom_read_mac();
	eeprom_read_tcpip();
	eeprom_read_tcpip_addresses();

	init_main_menu(MENUITEM_NOTHING);
	lcd_info_select_menu_item_set(MENUITEM_NOTHING);

	init_nic();

	// http://jeelabs.org/pub/docs/ethercard/classEtherCard.html#aea7b5998c41a424c9026624a26f6758e

	lcd_info_current_menu_item_set(MENUITEM_NOTHING);
	// 'lcd_info_current_menu_item_ensure_visible' will find the first item for showing 
	// if current_menu_item is MENUITEM_NOTHING.
	lcd_info_current_menu_item_ensure_visible(DIRECTION_NEXT, true); 
	lcd_info_current_menu_item_show();

	//wdt_enable(WDTO_500MS); //enable it, and set it to the time
}

void loop() {
	dp_status status = DP_STATUS_NOTHING;
	if (ether.isLinkUp()) {
		switch (select_menu_item) {
		case MENUITEM_MAIN_PING + MENUITEM_SELECT_BY_HOLD:
			if (ping_timer == PING_TIMER_STOP) break;
		case MENUITEM_MAIN_TRACE_CDP_EDP:
		case MENUITEM_MAIN_TRACE_LLDP:
			status = dp_listener_update();
			break;
		}

		switch (status) {
		case DP_STATUS_NOTHING:
			break;
		case DP_STATUS_OTHER_PACKET:
		case DP_STATUS_CDP:
		case DP_STATUS_EDP:
		case DP_STATUS_LLDP:
		case DP_STATUS_LLDP_ETHERNET_II:
			if (select_menu_item == MENUITEM_MAIN_TRACE_CDP_EDP ||
				select_menu_item == MENUITEM_MAIN_TRACE_LLDP) {
				if (current_menu == MENU_MAIN) {
					// Trace CDP/EDP and Trace LLDP show the same dp_packets_received.
					snprintnum(menu_buffer[select_menu_item].p_value, 11, dp_packets_received, 10);
				}

				lcd_control_update_lines();
			}
			break;
		case DP_STATUS_ICMP:
			if (select_menu_item == MENUITEM_MAIN_PING + MENUITEM_SELECT_BY_HOLD) {
				if (ping_timer != PING_TIMER_STOP)
					ping_test_recv();
			}
			break;

		case DP_STATUS_INCOMPLETE_PACKET:
			DEBUG_PRINTLN(F("Incomplete packet.")); 
			break;
		case DP_STATUS_UNKNOWN_LLC:
			DEBUG_PRINTLN(F("Unexpected LLC packet."));
			break;
		case DP_STATUS_UNKNOWN_LLC_SNAP:
			break;

		}
	}

	DebounceButton::updateAll();
	lcd_update();
	
}

