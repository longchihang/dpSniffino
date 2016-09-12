#ifndef _DP_LISTENER_H_
#define _DP_LISTENER_H_


#include <Arduino.h>
#include <SPI.h>

#include <EtherCard.h>

typedef uint8_t dp_status;

#define DP_STATUS_NOTHING           0
#define DP_STATUS_INCOMPLETE_PACKET 1
#define DP_STATUS_OTHER_PACKET      2
#define DP_STATUS_UNKNOWN_LLC       3
#define DP_STATUS_UNKNOWN_LLC_SNAP  4
#define DP_STATUS_CDP               5
#define DP_STATUS_EDP               6
#define DP_STATUS_LLDP              7
#define DP_STATUS_LLDP_ETHERNET_II  8
#define DP_STATUS_ICMP              9

#define MAC_LENGTH 6
#define IP_LENGTH 4


// Public functions
void dp_listener_init();
void dp_listener_promiscuous(bool enabled = true);
bool dp_listener_tcpip_dhcp();
bool dp_listener_tcpip_static(/*const uint8_t* my_ip = NULL,
							const uint8_t* gw_ip = NULL,
							const uint8_t* dns_ip = NULL,
							const uint8_t* mask = NULL*/);
dp_status dp_listener_update();


// Public handlers
typedef void(*dp_handler) (
	const uint8_t packet[], size_t* p_packet_index, size_t packet_length, const uint8_t source_mac[]);

extern uint8_t my_mac[];
/*
extern uint8_t my_ip[];
extern uint8_t netmask[];
extern uint8_t gw_ip[];
extern uint8_t dns_ip[];
*/

extern dp_handler cdp_packet_handler;
extern dp_handler edp_packet_handler;
extern dp_handler lldp_packet_handler;
extern dp_handler other_packet_handler;
extern dp_handler icmp_packet_handler;

extern volatile uint32_t last_dp_received;
extern volatile uint32_t dp_packets_received;

//extern uint8_t tcpip_type;
void received_time_update();

// Helper functions
bool byte_array_contains(const uint8_t a[], uint16_t offset, const uint8_t b[], uint16_t length);
// to complete the leading 0


#endif
