/*
 * gvcp_cmd_packet.c
 *
 *  Created on: Aug 13, 2014
 *      Author: jalden
 */

#include <stddef.h>
#include <network.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "gvcp_ack_packet.h"
#include "gvcp_types.h"
#include "vns_types.h"


//***********************************************************//
//*   				 Acknowledge Interface  			    *//
//***********************************************************//
unsigned char *Get_Ack_Packet_Header_Data(gvcp_ack_header_t *hdr)
{
	return (hdr->ack_header);
}

unsigned short Get_Ack_Status(gvcp_ack_header_t *hdr)
{
	int i = ACK_DISC_STATUS_INDEX;
	unsigned short val = ((unsigned short)(hdr->ack_header[i++])) << 8;
	val |= (unsigned short)(hdr->ack_header[i++]);
	return val;
}
void Set_Ack_Status(gvcp_ack_header_t *hdr, unsigned short val)
{
	int i = ACK_DISC_STATUS_INDEX;

	hdr->ack_header[i++] = val >> 8;
	hdr->ack_header[i++] = val & 0xFF;
}
unsigned short Get_Ack_Command(gvcp_ack_header_t *hdr)
{
	int i = ACK_DISC_CMD_INDEX;
	unsigned short val = ((unsigned short)(hdr->ack_header[i++])) << 8;
	val |= (unsigned short)(hdr->ack_header[i++]);
	return val;
}
void Set_Ack_Command(gvcp_ack_header_t *hdr, unsigned short val)
{
	int i = ACK_DISC_CMD_INDEX;
	hdr->ack_header[i++] = val >> 8;
	hdr->ack_header[i++] = val & 0xFF;
}
unsigned short Get_Ack_Length(gvcp_ack_header_t *hdr)
{
	int i = ACK_DISC_LENGTH_INDEX;
	unsigned short val = ((unsigned short)(hdr->ack_header[i++])) << 8;
	val |= (unsigned short)(hdr->ack_header[i++]);
	return val;
}
void Set_Ack_Length(gvcp_ack_header_t *hdr, unsigned short val)
{
	int i = ACK_DISC_LENGTH_INDEX;
	hdr->ack_header[i++] = val >> 8;
	hdr->ack_header[i++] = val & 0xFF;
}
unsigned short Get_Ack_ID(gvcp_ack_header_t *hdr)
{
	int i = ACK_DISC_ID_INDEX;
	unsigned short val = ((unsigned short)(hdr->ack_header[i++])) << 8;
	val |= (unsigned short)(hdr->ack_header[i++]);
	return val;
}
void Set_Ack_ID(gvcp_ack_header_t *hdr, unsigned short val)
{
	int i = ACK_DISC_ID_INDEX;
	hdr->ack_header[i++] = val >> 8;
	hdr->ack_header[i++] = val & 0xFF;
}

//***********************************************************//
//*                Discovery Acknowledgment                 *//
//***********************************************************//
void Set_Ack_Packet_Header(gvcp_discovery_ack_t *gvcp_discovery_ack, gvcp_ack_header_t *gvcp_ack_header)
{

	//memset(&gvcp_ack_header,0, sizeof(gvcp_ack_header_t));
	memmove(gvcp_discovery_ack->discovery_ack, gvcp_ack_header->ack_header, sizeof(gvcp_ack_header_t) );
}

unsigned short Get_Ack_Spec_Version_Major(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_SPEC_MAJ_INDEX;
	unsigned short val = ((unsigned short)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned short)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_Spec_Version_Major(gvcp_discovery_ack_t *disc_pckt, unsigned short val)
{
	int i = ACK_DISC_SPEC_MAJ_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
unsigned short Get_Ack_Spec_Version_Minor(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_SPEC_MIN_INDEX;
	unsigned short val = ((unsigned short)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned short)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_Spec_Version_Minor(gvcp_discovery_ack_t *disc_pckt, unsigned short val)
{
	int i = ACK_DISC_SPEC_MIN_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
unsigned int Get_Ack_Device_Mode(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_DEV_MODE_INDEX;
	unsigned int val = ((unsigned int)(disc_pckt->discovery_ack[i++])) << 24;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 16;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned int)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_Device_Mode(gvcp_discovery_ack_t *disc_pckt, unsigned int val)
{
	int i = ACK_DISC_DEV_MODE_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 24;
	disc_pckt->discovery_ack[i++] = val >> 16;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}

unsigned short Get_Ack_Device_MAC_High(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_DEV_MAC_H_INDEX;
	unsigned short val = ((unsigned short)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned short)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_Device_MAC_High(gvcp_discovery_ack_t *disc_pckt, unsigned short val)
{
	int i = ACK_DISC_DEV_MAC_H_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
unsigned int Get_Ack_Device_MAC_Low(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_DEV_MAC_L_INDEX;
	unsigned int val = ((unsigned int)(disc_pckt->discovery_ack[i++])) << 24;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 16;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned int)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_Device_MAC_Low(gvcp_discovery_ack_t *disc_pckt, unsigned int val)
{
	int i = ACK_DISC_DEV_MAC_L_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 24;
	disc_pckt->discovery_ack[i++] = val >> 16;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
unsigned int Get_Ack_IP_Conf_Opt(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_IP_CONF_OPT_INDEX;
	unsigned int val = ((unsigned int)(disc_pckt->discovery_ack[i++])) << 24;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 16;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned int)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_IP_Conf_Opt(gvcp_discovery_ack_t *disc_pckt, unsigned int val)
{
	int i = ACK_DISC_IP_CONF_OPT_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 24;
	disc_pckt->discovery_ack[i++] = val >> 16;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
unsigned int Get_Ack_IP_Conf_Cur(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_IP_CONF_CUR_INDEX;
	unsigned int val = ((unsigned int)(disc_pckt->discovery_ack[i++])) << 24;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 16;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned int)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_IP_Conf_Cur(gvcp_discovery_ack_t *disc_pckt, unsigned int val)
{
	int i = ACK_DISC_IP_CONF_CUR_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 24;
	disc_pckt->discovery_ack[i++] = val >> 16;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
unsigned int Get_Ack_Cur_IP(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_IP_INDEX;
	unsigned int val = ((unsigned int)(disc_pckt->discovery_ack[i++])) << 24;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 16;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned int)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_Cur_IP(gvcp_discovery_ack_t *disc_pckt, unsigned int val)
{
	int i = ACK_DISC_IP_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 24;
	disc_pckt->discovery_ack[i++] = val >> 16;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
unsigned int Get_Ack_Cur_Subnet(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_SUBNET_INDEX;
	unsigned int val = ((unsigned int)(disc_pckt->discovery_ack[i++])) << 24;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 16;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned int)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_Cur_Subnet(gvcp_discovery_ack_t *disc_pckt, unsigned int val)
{
	int i = ACK_DISC_SUBNET_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 24;
	disc_pckt->discovery_ack[i++] = val >> 16;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
unsigned int Get_Ack_Cur_Def_Gateway(gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_DEF_GATE_INDEX;
	unsigned int val = ((unsigned int)(disc_pckt->discovery_ack[i++])) << 24;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 16;
	val |= ((unsigned int)(disc_pckt->discovery_ack[i++])) << 8;
	val |= (unsigned int)(disc_pckt->discovery_ack[i++]);
	return val;
}
void Set_Ack_Cur_Def_Gateway(gvcp_discovery_ack_t *disc_pckt, unsigned int val)
{
	int i = ACK_DISC_DEF_GATE_INDEX;
	disc_pckt->discovery_ack[i++] = val >> 24;
	disc_pckt->discovery_ack[i++] = val >> 16;
	disc_pckt->discovery_ack[i++] = val >> 8;
	disc_pckt->discovery_ack[i++] = val & 0xFF;
}
void Get_Ack_Manufacturer_Name(manufact_name_t *manufact_name, gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_MANFACT_NAME_INDEX;
	memmove(manufact_name, disc_pckt->discovery_ack+i,sizeof(manufact_name_t));
}
void Set_Ack_Cur_Manufacturer_Name(gvcp_discovery_ack_t *disc_pckt, unsigned char *val)
{
	int i = ACK_DISC_MANFACT_NAME_INDEX;
	memmove(disc_pckt->discovery_ack+i,val, MANUFACTURER_NAME_MAX_SIZE);
}
void Get_Ack_Model_Name(model_name_t *model_name, gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_MODEL_NAME_INDEX;
	memmove(model_name, disc_pckt->discovery_ack+i,sizeof(model_name_t));
}
void Set_Ack_Model_Name(gvcp_discovery_ack_t *disc_pckt, unsigned char *val)
{
	int i = ACK_DISC_MODEL_NAME_INDEX;
	memmove(disc_pckt->discovery_ack+i,val, sizeof(model_name_t));
}
void Get_Ack_Device_Version(device_version_t *device_version, gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_DEV_VER_INDEX;
	memmove(device_version, disc_pckt->discovery_ack+i,sizeof(device_version_t));
}
void Set_Ack_Device_Version(gvcp_discovery_ack_t *disc_pckt, unsigned char *val)
{
	int i = ACK_DISC_DEV_VER_INDEX;
	memmove(disc_pckt->discovery_ack+i,val, sizeof(device_version_t));
}
void Get_Manufact_Spec_Info(manufact_spec_info_t *manufact_spec_info, gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_MANUFACT_SPEC_INFO_INDEX;
	memmove(manufact_spec_info, disc_pckt->discovery_ack+i,sizeof(device_version_t));
}
void Set_Manufact_Spec_Info(gvcp_discovery_ack_t *disc_pckt, unsigned char *val)
{
	int i = ACK_DISC_MANUFACT_SPEC_INFO_INDEX;
	memmove(disc_pckt->discovery_ack+i,val, sizeof(manufact_spec_info_t));
}
void Get_Serial_Num(serial_number_t *serial_number, gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_SERIAL_NUM_INDEX;
	memmove(serial_number, disc_pckt->discovery_ack+i,sizeof(serial_number_t));
}
void Set_Serial_Num(gvcp_discovery_ack_t *disc_pckt, unsigned char *val)
{
	int i = ACK_DISC_SERIAL_NUM_INDEX;
	memmove(disc_pckt->discovery_ack+i,val, sizeof(serial_number_t));
}
void Get_User_Def_Name(user_def_name_t *user_def_name, gvcp_discovery_ack_t *disc_pckt)
{
	int i = ACK_DISC_USER_DEF_NAME_INDEX;
	memmove(user_def_name, disc_pckt->discovery_ack+i,sizeof(user_def_name_t));
}
void Set_User_Def_Name(gvcp_discovery_ack_t *disc_pckt, unsigned char *val)
{
	int i = ACK_DISC_USER_DEF_NAME_INDEX;
	memmove(disc_pckt->discovery_ack+i,val, sizeof(user_def_name_t));
}
void build_gvcp_discovery_ack_packet(gvcp_discovery_ack_t *disc_pckt, unsigned short req_id)//, unsigned int ip)
{
	gvcp_ack_header_t ack_header;

	manufact_name_t manufact_name;
	model_name_t model_name;
	device_version_t device_version;
	manufact_spec_info_t manufact_spec_info;
	serial_number_t serial_number;
	user_def_name_t user_def_name;


	memset(&ack_header,'\0',ACK_HEADER_MAX_SIZE);
	memset(disc_pckt,'\0',ACK_DISCOVERY_MAX_SIZE);
	Set_Ack_Status(&ack_header, 0x0000);
	Set_Ack_Command(&ack_header, DISCOVERY_ACK);
	Set_Ack_Length(&ack_header, 0x00f8);
	Set_Ack_ID(&ack_header, req_id);

	unsigned char* ack_header_ptr = (unsigned char*)&ack_header;
	unsigned char* disc_pckt_ptr = (unsigned char*)disc_pckt;
	Set_Packet_Header(disc_pckt_ptr,ack_header_ptr);

	Set_Ack_Spec_Version_Major(disc_pckt, 0x0001);
	Set_Ack_Spec_Version_Minor(disc_pckt, 0x0000);
	// 1 010 0000 0000000000000000 00000001
	// A0 00 00 01
	Set_Ack_Device_Mode(disc_pckt, 0xA0000001);

	//*******************************************************************
	//*						Get & Set MAC							*
	//*******************************************************************


	Set_Ack_Device_MAC_High(disc_pckt, get_vns_mac_addr_high());
	Set_Ack_Device_MAC_Low(disc_pckt, get_vns_mac_addr_low());

	Set_Ack_IP_Conf_Opt(disc_pckt, 0x00000007);
	Set_Ack_IP_Conf_Cur(disc_pckt, 0x00000006);


	//*******************************************************************
	//*						Get & Set IP and Subnet						*
	//*******************************************************************
	char host_name_sub[INET6_ADDRSTRLEN];
	char *host_name,* host_subnet;
	get_vns_ip_addr_subnet((unsigned char*)&host_name_sub, sizeof(host_name_sub));

	host_name = strtok(host_name_sub,"/");
	host_subnet = strtok(NULL,"/");
	Set_Ack_Cur_IP(disc_pckt, parseIPV4string(host_name));
	Set_Ack_Cur_Subnet(disc_pckt, cidr_to_subnet_mask(atoi(host_subnet)));
	Set_Ack_Cur_Def_Gateway(disc_pckt, get_vns_gateway() ) ;


	memset(manufact_name.data, 0 , sizeof(manufact_name.data));
	memcpy(manufact_name.data, MANUFACTURER_NAME, sizeof(MANUFACTURER_NAME));
	Set_Ack_Cur_Manufacturer_Name(disc_pckt, (unsigned char*)&manufact_name);


	memset(model_name.data, 0 , sizeof(model_name.data));
	memcpy(model_name.data, MODEL_NAME, sizeof(MODEL_NAME));
	Set_Ack_Model_Name(disc_pckt, (unsigned char*)&model_name);


	memset(device_version.data, 0 , sizeof(device_version.data));

	sprintf(device_version.data,"%u.%02u",VNS_VERSION_MAJOR,VNS_VERSION_MINOR);
	Set_Ack_Device_Version(disc_pckt, (unsigned char*)&device_version);


	memset(manufact_spec_info.data, 0 , sizeof(manufact_spec_info.data));
	memcpy(manufact_spec_info.data, MODEL_NAME, sizeof(MODEL_NAME));
	Set_Manufact_Spec_Info(disc_pckt, (unsigned char*)&manufact_spec_info);


	char serial_num[2];
	get_vns_serial_num((char*)&serial_num, sizeof(serial_num));
	memset(serial_number.data, 0 , sizeof(serial_number.data));
	memcpy(serial_number.data,serial_num,sizeof(serial_num));
	Set_Serial_Num(disc_pckt, (unsigned char*)&serial_num);



	memset(user_def_name.data, 0 , sizeof(user_def_name.data));
	get_system_name((char*)&user_def_name.data, sizeof(user_def_name.data));
	Set_User_Def_Name(disc_pckt, (unsigned char*)&user_def_name);
	return;
}
int Get_Cmd_ReadReg_Addr(linked_list_t *readreg_list,void * buf, unsigned short gvcp_pckt_length)
{
//***************************************************************************************************
//*		Function: 	Retrieves and creates a list of register address to be read.  Can then be		*
//*					given to Gvcp_Read_Reg_Batch to be read											*
//***************************************************************************************************
	unsigned short i;
	unsigned short num_elements = gvcp_pckt_length >> 2;
	unsigned int * element_array = ((unsigned int*)buf + CMD_READREG_ADDR_BASE_INDEX);
// ********* load elements into list
	if(num_elements > 0)
	{
		append_node(readreg_list, element_array ,sizeof(int));

	for(i=1; i < num_elements; i++)
		{
			append_node(readreg_list, ( element_array + i ), sizeof(int));

		}
	}
	else
	{
		clear_list(readreg_list);
		return 0;

	}
	return (readreg_list->list_size);



}
int build_gvcp_readreg_ack_packet_malloc(gvcp_readreg_ack_t *readreg_pckt, linked_list_t *readreg_list,unsigned short length, unsigned short req_id)
{
	gvcp_ack_header_t ack_header;
//	linked_list_node_t *conductor_node = readreg_list->head;
//	int i;
	unsigned int * conductor_ptr;
//******** compute packet size
	readreg_pckt->data_size = sizeof(gvcp_ack_header_t) + (sizeof(unsigned int)*(length >> 2)) ;
	readreg_pckt->data = malloc(readreg_pckt->data_size);
	if(readreg_pckt->data == NULL)
	{
//		perror("Out of Memory Error");
		return -1;
	}


// ******** Initialize header and packet
	memset(&ack_header,'\0',sizeof(gvcp_ack_header_t));
	memset(readreg_pckt->data,'\0',readreg_pckt->data_size);


	// ******** Build Header
	Set_Ack_Status(&ack_header, 0x0000);
	Set_Ack_Command(&ack_header, READREG_ACK);
	Set_Ack_Length(&ack_header, length);
	Set_Ack_ID(&ack_header, req_id);  //Need to pull this from Broadcasted Packet

	memcpy(readreg_pckt->data,&ack_header, sizeof(gvcp_ack_header_t));

	readreg_list->conductor = readreg_list->head;
	conductor_ptr = (unsigned int *)readreg_pckt->data + CMD_READREG_ADDR_BASE_INDEX;

	if(readreg_list->conductor != NULL)
	{

		unsigned int *reg_data = readreg_list->conductor->data;
		memcpy(conductor_ptr,reg_data, sizeof(unsigned int));
//		for(i = 1;readreg_list->conductor->next !=NULL; i++)
//		{
//			readreg_list->conductor = readreg_list->conductor->next;
//			conductor_ptr[i] = readreg_list->conductor->data;
//			//memcpy(conductor_ptr[i],readreg_list->conductor->data, sizeof(int));
//		}

	}
	return 0;
}
void free_gvcp_readreg_ack_packet_malloc(gvcp_readreg_ack_t *readreg_pckt)
{
	if(readreg_pckt->data != NULL)
	{
		free(readreg_pckt->data);
		readreg_pckt->data = NULL;
	}
}
void build_gvcp_forceip_ack_packet(gvcp_forceip_ack_t *forceip_pckt, unsigned short req_id, unsigned short forceip_status)
{
	gvcp_ack_header_t ack_header;

	memset(&ack_header,'\0',ACK_HEADER_MAX_SIZE);
	memset(forceip_pckt,'\0',ACK_DISCOVERY_MAX_SIZE);
	Set_Ack_Status(&ack_header, forceip_status);
	Set_Ack_Command(&ack_header, FORCEIP_ACK);
	Set_Ack_Length(&ack_header, 0x0000);
	Set_Ack_ID(&ack_header, req_id);  //Need to pull this from Broadcasted Packet
//	Set_Ack_Packet_Header(disc_pckt, &ack_header );
	Set_Packet_Header((unsigned char*)forceip_pckt, (unsigned char*)&ack_header);
	return;
}
