#include "dp_listener.h"
#include "lcd_info.h"
#include "pins.h"
#include "helpers.h"
#include "debug.h"
//#include <limits.h>
//#include <stdint.h>
#define UINT32_MAX  (0xffffffff)

dp_handler cdp_packet_handler;
dp_handler edp_packet_handler;
dp_handler lldp_packet_handler;
dp_handler other_packet_handler;
dp_handler icmp_packet_handler;

uint8_t Ethernet::buffer[500 + 14]; //receive buffer (was 1500+14, but costs way too much SRAM)

volatile uint32_t last_dp_received = 0;
volatile uint32_t dp_packets_received = 0;

void dp_listener_init() {
	ether.begin(sizeof Ethernet::buffer, my_mac, PIN_NIC_CS);
}

void dp_listener_promiscuous(bool enabled) {
	if (enabled)
		ENC28J60::enablePromiscuous();
	else
		ENC28J60::disablePromiscuous();
}

bool dp_listener_tcpip_dhcp() {
	bool rtn = ether.dhcpSetup();
	if (rtn) {
		/*
		for (int8_t i = 0; i < 4; i++) {
			my_ip[i] = ether.myip[i];
			netmask[i] = ether.netmask[i];
			gw_ip[i] = ether.gwip[i];
			dns_ip[i] = ether.dnsip[i];
		}
		*/
		
	}
	return rtn;
}

bool dp_listener_tcpip_static(/*const uint8_t* my_ip,
								const uint8_t* gw_ip,
								const uint8_t* dns_ip,
								const uint8_t* mask*/) {
	// must manual to set my_ip, 
	bool rtn = ether.staticSetup(ether.myip, ether.gwip, ether.dnsip, ether.netmask);
	return rtn;
}

void received_time_update() {
	last_dp_received = millis();

	if (dp_packets_received == UINT32_MAX) dp_packets_received = 0; // ULONG_MAX
	dp_packets_received++;

	//unsigned long secs = millis() / 1000;
	//int min = secs / 60;
	//int sec = secs % 60;
	//Serial.print(F("Time: "));
	//Serial.print(min); Serial.print(':');
	//if (sec < 10) Serial.print('0');
	//Serial.println(sec);
}


dp_status dp_listener_update() {
	// IEEE 802.3 LLC / SNAP
	// https://www.safaribooksonline.com/library/view/ethernet-the-definitive/9781449362980/images/edg2_0401.png
	// http://godleon.blogspot.com/2007/06/link-layer-ip-ip-datagrams-arp-arp.html
	// http://www.infocellar.com/networks/ethernet/frame.htm
	//
	//
	//
	// http://www.ieee802.org/1/files/public/docs2002/LLDP%20Overview.pdf
	// http://enterprise.huawei.com/ilink/enenterprise/download/HW_U_149738
	// http://support.huawei.com/enterprise/docinforeader.action?contentId=DOC0100523127&partNo=10032
	// http://www.cnblogs.com/rohens-hbg/articles/4765763.html
	// 
	// Ethernet II (DIX)
	// +-----------------------------------------------------------------------------------------------------------------------------------------+
	// | destination_address | source_address | Frame type     |                  Data                                                     | CRC |
	// #=========================================================================================================================================#
	// # destination_address | source_address |  type          | LLDPPDU Data                                                              | CRC # LLDP with Ethernet II
	// # 6 byte              | 6              | 2(0x88CC lldp) | ~                                                                         | 4   # 
	// #=========================================================================================================================================#
	// 
	//
	// IEEE 802.3 with LLC
	// +-----------------------------------------------------------------------------------------------------------------------------------------+
	// | IEEE 802.3 MAC Header                                 | 802.2 LLC Header               |                  Data                    | CRC |
	// #=========================================================================================================================================#
	// # destination_address | source_address | length         | DSAP     | SSAP     | Ctrl     |  Data                                    | CRC #
	// # 6 byte              | 6              | 2              | 1        | 1        | 1        |  ~                                       | 4   #
	// #=========================================================================================================================================#
	//
	//
	// IEEE 802.3 with LLC / SNAP
	// +-----------------------------------------------------------------------------------------------------------------------------------------+
	// | IEEE 802.3 MAC Header                                 | 802.2 LLC Header               | 802.2 SNAP     | Data                    | CRC |
	// #=========================================================================================================================================#
	// # destination_address | source_address | length         | DSAP     | SSAP     | Ctrl     | org_code | type| Data                    | CRC # CDP, EDP, lldp 
	// # 6 byte              | 6              | 2              | 1 (0xAA) | 1 (0xAA) | 1 (0x03) | 3        | 2   | ~                       | 4   #
	// #=========================================================================================================================================#
	//     org_code = 0x00000c (cisco)    , type = 0x2000 (2)  cdp
	//     org_code = 0x00e02b (extreme)  , type = 0x00bb (187) edp
	//     org_code = 0x000000 (lldp)     , type = 0x88cc (35020) lldp. LLDP rarely used SNAP, Cisco and Extreme normally used with Ethernet II.
	//
	
	const PROGMEM uint8_t cdp_destination_mac[MAC_LENGTH]  = { 0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcc }; 
	const PROGMEM uint8_t edp_destination_mac[MAC_LENGTH]  = { 0x00, 0xe0, 0x2b, 0x00, 0x00, 0x00 };
	const PROGMEM uint8_t lldp_destination_mac[MAC_LENGTH] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e };

	// REF: http://www.cs.columbia.edu/~hgs/internet/cdp.shtml
#define FRAME_TYPE_LENGTH 2
	const PROGMEM uint8_t lldp_frame_type_bytes[FRAME_TYPE_LENGTH]           = { 0x88, 0xcc }; // When lldp used Ethernet II, not LLC/SNAP.
#define LLC_HEADER_LENGTH 3
	const PROGMEM uint8_t llc_header_bytes[LLC_HEADER_LENGTH]                = { 0xaa, 0xaa, 0x03 };
#define LLC_SNAP_HEADER_LENGTH 5
	const PROGMEM uint8_t cdp_llc_snap_header_bytes[LLC_SNAP_HEADER_LENGTH]  = { 0x00, 0x00, 0x0c, 0x20, 0x00 }; // Cisco(0x00000c), cdp(0x2000) 
	const PROGMEM uint8_t edp_llc_snap_header_bytes[LLC_SNAP_HEADER_LENGTH]  = { 0x00, 0xe0, 0x2b, 0x00, 0xbb }; // Extreme(0x00e02b), edp(0x00bb)
	const PROGMEM uint8_t lldp_llc_snap_header_bytes[LLC_SNAP_HEADER_LENGTH] = { 0x00, 0x00, 0x00, 0x88, 0xcc }; // IEEE(0x000000), lldp(0x88cc)

	uint16_t received_packet_length = ether.packetReceive();//sizeof(edp_packet);
	uint16_t pos = ether.packetLoop(received_packet_length); // Reponse ICMP pings

	size_t packet_index = 0; //2; // 2 bytes preamble	
	
	dp_status status = DP_STATUS_NOTHING;
	uint8_t* p_packet = Ethernet::buffer;

	if (received_packet_length == 0)
		return DP_STATUS_NOTHING;

	DEBUG_PRINTLN_WITH_TITLE(F("Data recv: "), DEBUG_PRINT_HEX(p_packet, 0, received_packet_length));

	// destination_address
	if (byte_array_contains(p_packet, packet_index, cdp_destination_mac, MAC_LENGTH)) {
		status = DP_STATUS_CDP;
	}
	else if (byte_array_contains(p_packet, packet_index, edp_destination_mac, MAC_LENGTH)) {
		status = DP_STATUS_EDP;
	}
	else if (byte_array_contains(p_packet, packet_index, lldp_destination_mac, MAC_LENGTH)) {
		status = DP_STATUS_LLDP;
	}
	else if (ether.packetLoopIcmpCheckReply(ether.hisip)) { // Is this response packet I ping?
		status = DP_STATUS_ICMP;
	}
	else { // other packet
		status = DP_STATUS_OTHER_PACKET;
	}


	packet_index += MAC_LENGTH; 

	// source_address
	uint8_t* psource_mac = p_packet + packet_index;
	packet_index += MAC_LENGTH;
	

	if (status == DP_STATUS_LLDP && 
		byte_array_contains(p_packet, packet_index, lldp_frame_type_bytes, FRAME_TYPE_LENGTH)) {
		// LLDP with Ethernet II (DIX)
		packet_index += FRAME_TYPE_LENGTH;
		status = DP_STATUS_LLDP_ETHERNET_II;
	}
	else if (status == DP_STATUS_ICMP) {
	}
	else if (status == DP_STATUS_OTHER_PACKET) {
	}
	else {
		// 802.3 packet

		// length: 2 bytes
		uint16_t packet_length_value = (p_packet[packet_index] << 8) | p_packet[packet_index + 1];
		packet_index += 2;

		if (packet_length_value != received_packet_length - packet_index) {
			status = DP_STATUS_INCOMPLETE_PACKET;
		}
		else {
			// 802.2 LLC
			if (byte_array_contains(p_packet, packet_index, llc_header_bytes, LLC_HEADER_LENGTH)) {
				packet_index += LLC_HEADER_LENGTH;

				switch (status) {
				case DP_STATUS_CDP:
					if (!byte_array_contains(p_packet, packet_index, cdp_llc_snap_header_bytes, LLC_SNAP_HEADER_LENGTH))
						status = DP_STATUS_UNKNOWN_LLC_SNAP;
					break;
				case DP_STATUS_EDP:
					if (!byte_array_contains(p_packet, packet_index, edp_llc_snap_header_bytes, LLC_SNAP_HEADER_LENGTH))
						status = DP_STATUS_UNKNOWN_LLC_SNAP;
					break;
				case DP_STATUS_LLDP:
					if (!byte_array_contains(p_packet, packet_index, lldp_llc_snap_header_bytes, LLC_SNAP_HEADER_LENGTH))
						status = DP_STATUS_UNKNOWN_LLC_SNAP;
					break;
				}
				packet_index += LLC_SNAP_HEADER_LENGTH;
			}
			else {
				status = DP_STATUS_UNKNOWN_LLC;
			}
		}
	}
	
	switch (status) {
	case DP_STATUS_CDP:
		DEBUG_PRINTLN(F("CDP!"));
		received_time_update();
		cdp_packet_handler(p_packet, &packet_index, received_packet_length /* 20211021 - fix cdp summary garbled *//* - packet_index */, psource_mac);
		break;
	case DP_STATUS_EDP:
		DEBUG_PRINTLN(F("EDP!"));
		received_time_update();
		edp_packet_handler(p_packet, &packet_index, received_packet_length /* 20211021 - fix cdp summary garbled *//* - packet_index */, psource_mac);
		break;
	case DP_STATUS_LLDP:
	case DP_STATUS_LLDP_ETHERNET_II:
		DEBUG_PRINTLN(F("LLDP!"));
		received_time_update();
		lldp_packet_handler(p_packet, &packet_index, received_packet_length /* 20211021 - fix cdp summary garbled *//* - packet_index */, psource_mac);
		break;
	case DP_STATUS_ICMP:
		DEBUG_PRINTLN(F("Ping response!"));
		received_time_update();
		icmp_packet_handler(p_packet, &packet_index, received_packet_length /* 20211021 - fix cdp summary garbled *//* - packet_index */, psource_mac);
		break;

	case DP_STATUS_OTHER_PACKET:
		// To show infomation that sniffer is receiving
		DEBUG_PRINTLN(F("Other packet!"));
		received_time_update();
		other_packet_handler(p_packet, &packet_index, received_packet_length /* 20211021 - fix cdp summary garbled *//* - packet_index */, psource_mac); 

		break;
	default:
		DEBUG_PRINTLN(F("Unknown packet!"));
	}

	return status;
}



bool byte_array_contains(const uint8_t a[], uint16_t offset, const uint8_t b[], uint16_t length) {
  for(uint16_t i=offset, j=0; j<length; ++i, ++j) {
    if(a[i] != b[j]) return false;
  }
  return true;
}
