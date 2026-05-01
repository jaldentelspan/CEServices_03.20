/*
 * gvcp_types.h
 *
 *  Created on: Aug 13, 2014
 *      Author: jalden
 */

#ifndef GVCP_TYPES_H_
#define GVCP_TYPES_H_

//#include "ip2_utils.h"
//#include "ip2_types.h"
//#include "ip2_api.h"
#include "conf_api.h"
//#include "mgmt_api.h"

#define GVCP_SERVERPORT "3956"	// the port users will be connecting to
#define BROADCAST_ADDR "255.255.255.255"
#define VLAN_1	1
#define BROADCAST_CMD_FLAG_MASK 0x8

#define INET_ADDRSTRLEN 16
#define MAC_ADDRSTRLEN 48

#define GVCP_DISCOVERY_CMD_LENGTH 10
#define CMD_HEADER_SIZE 8
#define MAXBUFLEN 100




//**************************************************************//
//						GVCP Acknowledge Definitions 			//
//**************************************************************//
#define RESERVED_NIBBLE				2
#define RESERVED_INT				4
#define ACK_HEADER_MAX_SIZE 		8
#define ACK_DISCOVERY_MAX_SIZE 		256
#define ACK_READREG_MAX_SIZE 		576
#define ACK_FORCEIP_MAX_SIZE 		ACK_HEADER_MAX_SIZE

#define KEY_MAX_SIZE				1
#define FLAG_MAX_SIZE				1
#define REQ_ID_MAX_SIZE				2

#define STATUS_MAX_SIZE 			2
#define CMD_MAX_SIZE				2
#define LENGTH_MAX_SIZE				2
#define ACK_ID_MAX_SIZE				2
#define SPEC_VER_MAJ_MAX_SIZE		2
#define SPEC_VER_MIN_MAX_SIZE		2
#define DEV_MODE_MAX_SIZE			4
#define DEV_MAC_H_MAX_SIZE			2
#define DEV_MAC_L_MAX_SIZE			4
#define IP_CONF_OPT_MAX_SIZE		4
#define IP_CONF_CUR_MAX_SIZE		4
#define IP_MAX_SIZE					4
#define SUBNET_MAX_SIZE				4
#define DEF_GATE_MAX_SIZE			4
#define MANUFACTURER_NAME_MAX_SIZE 		32
#define MODEL_NAME_MAX_SIZE 			32
#define DEVICE_VERSION_MAX_SIZE 		32
#define MANUFACTURER_SPEC_INFO_MAX_SIZE 48
#define SERIAL_NUMBER_MAX_SIZE 			16
#define USER_DEF_NAME_MAX_SIZE 			16
//#define _MAX_SIZE

#define ACK_DISC_STATUS_INDEX 		0
#define ACK_DISC_CMD_INDEX 			(ACK_DISC_STATUS_INDEX + STATUS_MAX_SIZE)
#define ACK_DISC_LENGTH_INDEX	 	(ACK_DISC_CMD_INDEX + CMD_MAX_SIZE)
#define ACK_DISC_ID_INDEX			(ACK_DISC_LENGTH_INDEX + LENGTH_MAX_SIZE)

#define ACK_DISC_SPEC_MAJ_INDEX		(ACK_DISC_ID_INDEX + ACK_ID_MAX_SIZE )
#define ACK_DISC_SPEC_MIN_INDEX 	(ACK_DISC_SPEC_MAJ_INDEX + SPEC_VER_MAJ_MAX_SIZE)
#define ACK_DISC_DEV_MODE_INDEX 	(ACK_DISC_SPEC_MIN_INDEX + SPEC_VER_MIN_MAX_SIZE)
#define ACK_DISC_DEV_MAC_H_INDEX 	(ACK_DISC_DEV_MODE_INDEX + DEV_MODE_MAX_SIZE+ RESERVED_NIBBLE)
#define ACK_DISC_DEV_MAC_L_INDEX 	(ACK_DISC_DEV_MAC_H_INDEX + DEV_MAC_H_MAX_SIZE)
#define ACK_DISC_IP_CONF_OPT_INDEX 	(ACK_DISC_DEV_MAC_L_INDEX + DEV_MAC_L_MAX_SIZE)
#define ACK_DISC_IP_CONF_CUR_INDEX	(ACK_DISC_IP_CONF_OPT_INDEX + IP_CONF_OPT_MAX_SIZE)
#define ACK_DISC_IP_INDEX			(ACK_DISC_IP_CONF_CUR_INDEX + IP_CONF_CUR_MAX_SIZE +(3* RESERVED_INT))
#define ACK_DISC_SUBNET_INDEX		(ACK_DISC_IP_INDEX + IP_MAX_SIZE +(3* RESERVED_INT))
#define ACK_DISC_DEF_GATE_INDEX 	(ACK_DISC_SUBNET_INDEX + SUBNET_MAX_SIZE +(3* RESERVED_INT))
#define ACK_DISC_MANFACT_NAME_INDEX	(ACK_DISC_DEF_GATE_INDEX + DEF_GATE_MAX_SIZE)
#define ACK_DISC_MODEL_NAME_INDEX	(ACK_DISC_MANFACT_NAME_INDEX + MANUFACTURER_NAME_MAX_SIZE)
#define ACK_DISC_DEV_VER_INDEX		(ACK_DISC_MODEL_NAME_INDEX + MODEL_NAME_MAX_SIZE)
#define ACK_DISC_MANUFACT_SPEC_INFO_INDEX	(ACK_DISC_DEV_VER_INDEX + DEVICE_VERSION_MAX_SIZE)
#define ACK_DISC_SERIAL_NUM_INDEX			(ACK_DISC_MANUFACT_SPEC_INFO_INDEX + MANUFACTURER_SPEC_INFO_MAX_SIZE)
#define ACK_DISC_USER_DEF_NAME_INDEX		(ACK_DISC_SERIAL_NUM_INDEX + SERIAL_NUMBER_MAX_SIZE)


//**************************************************************//
//						GVCP Command Definitions 				//
//**************************************************************//
#define CMD_HEADER_MAX_SIZE 8

#define CMD_READREG_ADDR_BASE_INDEX (CMD_HEADER_MAX_SIZE)

#define CMD_FORCEIP_MAX_SIZE 512

//#define CMD__INDEX				( + )
#define CMD_HEADER_KEY_INDEX 			0
#define CMD_HEADER_FLAG_INDEX 			(CMD_HEADER_KEY_INDEX + KEY_MAX_SIZE)
#define CMD_HEADER_CMD_INDEX 			(CMD_HEADER_FLAG_INDEX + KEY_MAX_SIZE)
#define CMD_HEADER_LENGTH_INDEX	 		(CMD_HEADER_CMD_INDEX + CMD_MAX_SIZE)
#define CMD_HEADER_ID_INDEX			(CMD_HEADER_LENGTH_INDEX + LENGTH_MAX_SIZE)

#define CMD_FORCEIP_DEV_MAC_H_INDEX 	(CMD_HEADER_ID_INDEX + REQ_ID_MAX_SIZE + RESERVED_NIBBLE)
#define CMD_FORCEIP_DEV_MAC_L_INDEX 	(CMD_FORCEIP_DEV_MAC_H_INDEX + DEV_MAC_H_MAX_SIZE)
#define CMD_FORCEIP_IP_INDEX			(CMD_FORCEIP_DEV_MAC_L_INDEX + DEV_MAC_L_MAX_SIZE +(3* RESERVED_INT))
#define CMD_FORCEIP_SUBNET_INDEX		(CMD_FORCEIP_IP_INDEX + IP_MAX_SIZE +(3* RESERVED_INT))
#define CMD_FORCEIP_DEF_GATE_INDEX 		(CMD_FORCEIP_SUBNET_INDEX + SUBNET_MAX_SIZE +(3* RESERVED_INT))

//**************************************************************//
//				GVCP Generic Structures Definitions 			//
//**************************************************************//

typedef struct linked_list_node
{
  void *data;
  int data_size;
  struct linked_list_node *next, *prev;
}linked_list_node_t;

typedef struct linked_list
{
	linked_list_node_t *head,*tail,*conductor;
	unsigned int list_size;
}linked_list_t;

linked_list_t MAC_TABLE;

typedef struct gvcpreg_mac_table_entry
{
	vtss_mac_table_entry_t vtss_mac_table_entry;
	unsigned int reg_addr_high;
	unsigned int reg_addr_low;
	unsigned int mactbl_addr_high;
	unsigned int mactbl_addr_low;
}gvcpreg_mac_table_entry_t;


typedef enum gvcp_message
{
//*************** Discovery Protocol ***************//
	DISCOVERY_CMD 		= 0x0002,
	DISCOVERY_ACK 		= 0x0003,
	FORCEIP_CMD 		= 0x0004,
	FORCEIP_ACK 		= 0x0005,
//*************** Streaming Protocol Control ***************//
	PACKETRESEND_CMD 	= 0x0040,
//*************** Device Memory Access ***************//
	READREG_CMD 		= 0x0080,
	READREG_ACK 		= 0x0081,
	WRITEREG_CMD 		= 0x0082,
	WRITEREG_ACK 		= 0x0083,
	READMEM_CMD 		= 0x0084,
	READMEM_ACK 		= 0x0085,
	WRITEMEM_CMD 		= 0x0086,
	WRITE_ACK 			= 0x0087,
//*************** Asynchronous Events ***************//
	PENDING_ACK 		= 0x0089,
	EVENT_CMD 			= 0x00C0,
	EVENT_ACK 			= 0x00C1,
	EVENTDATA_CMD 		= 0x00C2,
	EVENTDATA_ACK 		= 0x00C3,
//*************** Miscellaneous ***************//
	ACTION_CMD 			= 0x0100,
	ACTION_ACK 			= 0x0101,
} gvcp_message_t;
void Set_Packet_Header(unsigned char *pckt, unsigned char *header);

void init_linked_list(linked_list_t * list);
//void append( linked_list_node_t * );
void append_node( linked_list_t * list, void *data, size_t data_size );
//void remove( linked_list_t * );
void deletelist( linked_list_t * );
void clear_list(linked_list_t *list);
void print_list(linked_list_t *list);//, void(*print)(void const*));

unsigned int parseIPV4string(char* ipAddress);
void remove_all_chars(char* str, char c);
int get_vns_ip_addr_subnet(char * host_name_sub, size_t host_len);
int get_vns_mac_addr(char * mac_addr, size_t mac_len);
int get_vns_serial_num(char * serial_num, size_t mac_len);
int get_system_name(char * val, size_t val_size);
unsigned int get_vns_mac_addr_low(void);
unsigned short get_vns_mac_addr_high(void);
unsigned int cidr_to_subnet_mask(int cidr_num);
unsigned short subnet_mask_to_cidr(unsigned int subnet_mask);
unsigned int get_vns_gateway(void);

int build_mac_table(linked_list_t *list);

void print_mac_table(void);

void trigger_gvcp(void);
void gvcpT_D(const char *format, ...);

int change_ip_address(unsigned short vlan, unsigned int ip_address, unsigned int subnet );

#endif /* GVCP_TYPES_H_ */
