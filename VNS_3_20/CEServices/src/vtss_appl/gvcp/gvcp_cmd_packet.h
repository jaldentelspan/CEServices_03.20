/*
 * gvcp_cmd_packet.h
 *
 *  Created on: Aug 13, 2014
 *      Author: jalden
 */

#ifndef GVCP_CMD_PACKET_H_
#define GVCP_CMD_PACKET_H_

#include "gvcp_types.h"

typedef struct gvcp_cmd_header
{
	unsigned char cmd_header[8];
}gvcp_cmd_header_t;

//*************** Command Header Interface ***************//
unsigned char Get_Cmd_Key(gvcp_cmd_header_t *hdr);
void Set_Cmd_Key(gvcp_cmd_header_t *hdr, unsigned char val);

unsigned char Get_Cmd_Flag(gvcp_cmd_header_t *hdr);
void Set_Cmd_Flag(gvcp_cmd_header_t *hdr, unsigned char val);

unsigned short Get_Cmd_Command(gvcp_cmd_header_t *hdr);
void Set_Cmd_Command(gvcp_cmd_header_t *hdr, unsigned short val);

unsigned short Get_Cmd_Length(gvcp_cmd_header_t *hdr);
void Set_Cmd_Length(gvcp_cmd_header_t *hdr, unsigned short val);

unsigned short Get_Cmd_REQ_ID(gvcp_cmd_header_t *hdr);
void Set_Cmd_REQ_ID(gvcp_cmd_header_t *hdr, unsigned short val);



typedef struct gvcp_readreg_cmd
{
// This is dynamic see gvcp_readreg_ack
	//unsigned char readreg_ack[ACK_READREG_MAX_SIZE];

}gvcp_readreg_cmd_t;

//*************** GVCP FORCEIP CMD assembly ***************//


typedef struct gvcp_forceip_cmd
{
	unsigned char forceip_cmd[CMD_FORCEIP_MAX_SIZE];
}gvcp_forceip_cmd_t;
void build_gvcp_forceip_cmd_packet(gvcp_forceip_cmd_t *forceip_pckt, unsigned int ip, unsigned short req_id);

unsigned short Get_Cmd_Device_MAC_High(gvcp_forceip_cmd_t *forceip_pckt);
void Set_Cmd_Device_MAC_High(gvcp_forceip_cmd_t *forceip_pckt, unsigned short val);

unsigned int Get_Cmd_Device_MAC_Low(gvcp_forceip_cmd_t *forceip_pckt);
void Set_Cmd_Device_MAC_Low(gvcp_forceip_cmd_t *forceip_pckt, unsigned int val);

unsigned int Get_Cmd_Cur_IP(gvcp_forceip_cmd_t *forceip_pckt);
void Set_Cmd_Cur_IP(gvcp_forceip_cmd_t *forceip_pckt, unsigned int val);

unsigned int Get_Cmd_Cur_Subnet(gvcp_forceip_cmd_t *forceip_pckt);
void Set_Cmd_Cur_Subnet(gvcp_forceip_cmd_t *forceip_pckt, unsigned int val);

unsigned int Get_Cmd_Cur_Def_Gateway(gvcp_forceip_cmd_t *forceip_pckt);
void Set_Cmd_Cur_Def_Gateway(gvcp_forceip_cmd_t *forceip_pckt, unsigned int val);




#endif /* GVCP_CMD_PACKET_H_ */
