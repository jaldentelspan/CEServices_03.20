/*
 * gvcp_cmd_packet.c
 *
 *  Created on: Aug 13, 2014
 *      Author: jalden
 */

#include "gvcp_cmd_packet.h"
#include "gvcp_types.h"
#include "gvcp_ack_packet.h"
#include <network.h>
#include <netinet/udp.h>


unsigned char Get_Cmd_Key(gvcp_cmd_header_t *hdr)
{
	return hdr->cmd_header[0];
}

void Set_Cmd_Key(gvcp_cmd_header_t *hdr, unsigned char val)
{
	hdr->cmd_header[0] = val;
}
unsigned char Get_Cmd_Flag(gvcp_cmd_header_t *hdr)
{
	return hdr->cmd_header[1];
}

void Set_Cmd_Flag(gvcp_cmd_header_t *hdr, unsigned char val)
{
	hdr->cmd_header[1] = val;
}
unsigned short Get_Cmd_Command(gvcp_cmd_header_t *hdr)
{
	int i = 2;
	unsigned short val = ((unsigned short)(hdr->cmd_header[i++])) << 8;
	val |= (unsigned short)(hdr->cmd_header[i++]);
	return val;
}

void Set_Cmd_Command(gvcp_cmd_header_t *hdr, unsigned short val)
{
	int i = 2;
	hdr->cmd_header[i++] = val >> 8;
	hdr->cmd_header[i++] = val & 0xFF;
}
unsigned short Get_Cmd_Length(gvcp_cmd_header_t *hdr)
{
	int i = 4;
	unsigned short val = ((unsigned short)(hdr->cmd_header[i++])) << 8;
	val |= (unsigned short)(hdr->cmd_header[i++]);
	return val;
}

void Set_Cmd_Length(gvcp_cmd_header_t *hdr, unsigned short val)
{
	int i = 4;
	hdr->cmd_header[i++] = val >> 8;
	hdr->cmd_header[i++] = val & 0xFF;
}
unsigned short Get_Cmd_REQ_ID(gvcp_cmd_header_t *hdr)
{
	int i = 6;
	unsigned short val = ((unsigned short)(hdr->cmd_header[i++])) << 8;
	val |= (unsigned short)(hdr->cmd_header[i++]);
	return val;
}

void Set_Cmd_REQ_ID(gvcp_cmd_header_t *hdr, unsigned short val)
{
	int i = 6;
	hdr->cmd_header[i++] = val >> 8;
	hdr->cmd_header[i++] = val & 0xFF;
}
void build_gvcp_forceip_cmd_packet(gvcp_forceip_cmd_t *forceip_pckt, unsigned int ip, unsigned short req_id)
{
	gvcp_cmd_header_t cmd_header;

// ******** Initialize header and packet
	memset(&cmd_header,'\0',sizeof(gvcp_cmd_header_t));

	// ******** Build Header
	Set_Cmd_Key(&cmd_header, 0x42);
	Set_Cmd_Flag(&cmd_header, 0x00);
	Set_Cmd_Command(&cmd_header, FORCEIP_CMD);
	Set_Cmd_Length(&cmd_header, 0x0038);
	Set_Cmd_REQ_ID(&cmd_header, req_id);  //Need to pull this from Broadcasted Packet


	//*******************************************************************
	//*						Get & Set IP and Subnet						*
	//*******************************************************************
	char host_name_sub[INET6_ADDRSTRLEN];
	char *host_name,* host_subnet;
//	get_vns_ip_addr_subnet(&host_name_sub, sizeof(host_name_sub));

	host_name = strtok(host_name_sub,"/");
	host_subnet = strtok(NULL,"/");

	Set_Ack_Cur_IP((gvcp_discovery_ack_t *)forceip_pckt, 0xC0A80157);
	Set_Ack_Cur_Subnet((gvcp_discovery_ack_t *)forceip_pckt, 0xFFFFFF00);
	Set_Ack_Cur_Def_Gateway((gvcp_discovery_ack_t *)forceip_pckt, 0xC0A80100 ) ;



}
unsigned short Get_Cmd_Device_MAC_High(gvcp_forceip_cmd_t *forceip_pckt)
{
	int i = CMD_FORCEIP_DEV_MAC_H_INDEX;
	unsigned short val = ((unsigned short)(forceip_pckt->forceip_cmd[i++])) << 8;
	val |= (unsigned short)(forceip_pckt->forceip_cmd[i++]);
	return val;
}
void Set_Cmd_Device_MAC_High(gvcp_forceip_cmd_t *forceip_pckt, unsigned short val)
{
	int i = CMD_FORCEIP_DEV_MAC_H_INDEX;
	forceip_pckt->forceip_cmd[i++] = val >> 8;
	forceip_pckt->forceip_cmd[i++] = val & 0xFF;
}
unsigned int Get_Cmd_Device_MAC_Low(gvcp_forceip_cmd_t *forceip_pckt)
{
	int i = CMD_FORCEIP_DEV_MAC_L_INDEX;
	unsigned int val = ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 24;
	val |= ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 16;
	val |= ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 8;
	val |= (unsigned int)(forceip_pckt->forceip_cmd[i++]);
	return val;
}
void Set_Cmd_Device_MAC_Low(gvcp_forceip_cmd_t *forceip_pckt, unsigned int val)
{
	int i = CMD_FORCEIP_DEV_MAC_L_INDEX;
	forceip_pckt->forceip_cmd[i++] = val >> 24;
	forceip_pckt->forceip_cmd[i++] = val >> 16;
	forceip_pckt->forceip_cmd[i++] = val >> 8;
	forceip_pckt->forceip_cmd[i++] = val & 0xFF;
}
unsigned int Get_Cmd_Cur_IP(gvcp_forceip_cmd_t *forceip_pckt)
{
	int i = CMD_FORCEIP_IP_INDEX;
	unsigned int val = ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 24;
	val |= ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 16;
	val |= ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 8;
	val |= (unsigned int)(forceip_pckt->forceip_cmd[i++]);
	return val;
}
void Set_Cmd_Cur_IP(gvcp_forceip_cmd_t *forceip_pckt, unsigned int val)
{
	int i = CMD_FORCEIP_IP_INDEX;
	forceip_pckt->forceip_cmd[i++] = val >> 24;
	forceip_pckt->forceip_cmd[i++] = val >> 16;
	forceip_pckt->forceip_cmd[i++] = val >> 8;
	forceip_pckt->forceip_cmd[i++] = val & 0xFF;
}
unsigned int Get_Cmd_Cur_Subnet(gvcp_forceip_cmd_t *forceip_pckt)
{
	int i = CMD_FORCEIP_SUBNET_INDEX;
	unsigned int val = ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 24;
	val |= ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 16;
	val |= ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 8;
	val |= (unsigned int)(forceip_pckt->forceip_cmd[i++]);
	return val;
}
void Set_Cmd_Cur_Subnet(gvcp_forceip_cmd_t *forceip_pckt, unsigned int val)
{
	int i = CMD_FORCEIP_SUBNET_INDEX;
	forceip_pckt->forceip_cmd[i++] = val >> 24;
	forceip_pckt->forceip_cmd[i++] = val >> 16;
	forceip_pckt->forceip_cmd[i++] = val >> 8;
	forceip_pckt->forceip_cmd[i++] = val & 0xFF;
}
unsigned int Get_Cmd_Cur_Def_Gateway(gvcp_forceip_cmd_t *forceip_pckt)
{
	int i = CMD_FORCEIP_DEF_GATE_INDEX;
	unsigned int val = ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 24;
	val |= ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 16;
	val |= ((unsigned int)(forceip_pckt->forceip_cmd[i++])) << 8;
	val |= (unsigned int)(forceip_pckt->forceip_cmd[i++]);
	return val;
}
void Set_Cmd_Cur_Def_Gateway(gvcp_forceip_cmd_t *forceip_pckt, unsigned int val)
{
	int i = CMD_FORCEIP_DEF_GATE_INDEX;
	forceip_pckt->forceip_cmd[i++] = val >> 24;
	forceip_pckt->forceip_cmd[i++] = val >> 16;
	forceip_pckt->forceip_cmd[i++] = val >> 8;
	forceip_pckt->forceip_cmd[i++] = val & 0xFF;
}

