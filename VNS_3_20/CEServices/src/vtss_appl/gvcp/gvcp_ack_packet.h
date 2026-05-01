/*
 * gvcp_ack_packet.h
 *
 *  Created on: Aug 13, 2014
 *      Author: jalden
 */

#ifndef GVCP_ACK_PACKET_H_
#define GVCP_ACK_PACKET_H_

#include "gvcp_types.h"
//*************** GVCP Acknowledge Interface ***************//
typedef struct manufact_name
{
	unsigned char data[MANUFACTURER_NAME_MAX_SIZE];
} manufact_name_t;
typedef struct model_name
{
	unsigned char data[MODEL_NAME_MAX_SIZE];
} model_name_t;
typedef struct device_version
{
	unsigned char data[DEVICE_VERSION_MAX_SIZE];
} device_version_t;
typedef struct manufact_spec_info
{
	unsigned char data[MANUFACTURER_SPEC_INFO_MAX_SIZE];
} manufact_spec_info_t;
typedef struct serial_number
{
	unsigned char data[SERIAL_NUMBER_MAX_SIZE];
} serial_number_t;
typedef struct user_def_name
{
	unsigned char data[USER_DEF_NAME_MAX_SIZE];
} user_def_name_t;

//*************** GVCP ACK Header assembly ***************//
typedef struct gvcp_ack_header
{
	unsigned char ack_header[ACK_HEADER_MAX_SIZE]; // first eight bits of ack messages

}gvcp_ack_header_t;
unsigned char *Get_Ack_Packet_Header_Data(gvcp_ack_header_t *hdr);

unsigned short Get_Ack_Status(gvcp_ack_header_t *hdr);
void Set_Ack_Status(gvcp_ack_header_t *hdr, unsigned short val);

unsigned short Get_Ack_Command(gvcp_ack_header_t *hdr);
void Set_Ack_Command(gvcp_ack_header_t *hdr, unsigned short val);

unsigned short Get_Ack_Length(gvcp_ack_header_t *hdr);
void Set_Ack_Length(gvcp_ack_header_t *hdr, unsigned short val);

unsigned short Get_Ack_ID(gvcp_ack_header_t *hdr);
void Set_Ack_ID(gvcp_ack_header_t *hdr, unsigned short val);



//*************** GVCP Discovery ACK assembly ***************//
typedef struct gvcp_discovery_ack
{
	unsigned char discovery_ack[ACK_DISCOVERY_MAX_SIZE];

}gvcp_discovery_ack_t;
typedef struct gvcp_readreg_ack
{
	void *data;
	unsigned int data_size;
	linked_list_node_t root_addr_node;
}gvcp_readreg_ack_t;

void Set_Ack_Packet_Header(gvcp_discovery_ack_t *gvcp_discovery_ack, gvcp_ack_header_t *gvcp_ack_header);

unsigned short Get_Ack_Spec_Version_Major(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Spec_Version_Major(gvcp_discovery_ack_t *disc_pckt, unsigned short val);

//unsigned short Get_Ack_Spec_Version_Minor(gvcp_discovery_ack_t *disc_pckt);
//void Set_Ack_Spec_Version_Minor(gvcp_discovery_ack_t *disc_pckt, unsigned short val);

unsigned int Get_Ack_Device_Mode(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Device_Mode(gvcp_discovery_ack_t *disc_pckt, unsigned int val);

unsigned short Get_Ack_Device_MAC_High(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Device_MAC_High(gvcp_discovery_ack_t *disc_pckt, unsigned short val);

unsigned int Get_Ack_Device_MAC_Low(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Device_MAC_Low(gvcp_discovery_ack_t *disc_pckt, unsigned int val);

unsigned int Get_Ack_IP_Conf_Opt(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_IP_Conf_Opt(gvcp_discovery_ack_t *disc_pckt, unsigned int val);

unsigned int Get_Ack_IP_Conf_Cur(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_IP_Conf_Cur(gvcp_discovery_ack_t *disc_pckt, unsigned int val);

unsigned int Get_Ack_Cur_IP(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Cur_IP(gvcp_discovery_ack_t *disc_pckt, unsigned int val);

unsigned int Get_Ack_Cur_Subnet(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Cur_Subnet(gvcp_discovery_ack_t *disc_pckt, unsigned int val);

unsigned int Get_Ack_Cur_Def_Gateway(gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Cur_Def_Gateway(gvcp_discovery_ack_t *disc_pckt, unsigned int val);

void Get_Ack_Manufacturer_Name(manufact_name_t *manufact_name, gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Cur_Manufacturer_Name(gvcp_discovery_ack_t *disc_pckt, unsigned char *val);

void Get_Ack_Model_Name(model_name_t *model_name, gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Model_Name(gvcp_discovery_ack_t *disc_pckt, unsigned char *val);

void Get_Ack_Device_Version(device_version_t *device_version, gvcp_discovery_ack_t *disc_pckt);
void Set_Ack_Device_Version(gvcp_discovery_ack_t *disc_pckt, unsigned char *val);

void Get_Manufact_Spec_Info(manufact_spec_info_t *manufact_spec_info, gvcp_discovery_ack_t *disc_pckt);
void Set_Manufact_Spec_Info(gvcp_discovery_ack_t *disc_pckt, unsigned char *val);

void Get_Serial_Num(serial_number_t *serial_number, gvcp_discovery_ack_t *disc_pckt);
void Set_Serial_Num(gvcp_discovery_ack_t *disc_pckt, unsigned char *val);

void Get_User_Def_Name(user_def_name_t *user_def_name, gvcp_discovery_ack_t *disc_pckt);
void Set_User_Def_Name(gvcp_discovery_ack_t *disc_pckt, unsigned char *val);

void build_gvcp_discovery_ack_packet(gvcp_discovery_ack_t *disc_pckt, unsigned short req_id);//, unsigned int ip);

int Get_Cmd_ReadReg_Addr(linked_list_t *readreg_list,void * buf, unsigned short gvcp_pckt_length);

int build_gvcp_readreg_ack_packet_malloc(gvcp_readreg_ack_t *readreg_pckt, linked_list_t *readreg_list,unsigned short length, unsigned short req_id);
void free_gvcp_readreg_ack_packet_malloc(gvcp_readreg_ack_t *readreg_pckt);
//*************** GVCP Discovery ACK assembly ***************//
typedef struct gvcp_forceip_ack
{
	unsigned char foceip_ack[ACK_FORCEIP_MAX_SIZE];
}gvcp_forceip_ack_t;

void build_gvcp_forceip_ack_packet(gvcp_forceip_ack_t *forceip_pckt, unsigned short req_id, unsigned short forceip_status );

#endif /* GVCP_ACK_PACKET_H_ */
