/*
 * gvcp_types.c
 *
 *  Created on: Aug 21, 2014
 *      Author: jalden
 */
#include <stdio.h>
#include <stdlib.h>
#include <network.h>

#include "gvcp_types.h"
#include "gvcp.h"
#include "ip2_utils.h"
#include "ip2_types.h"
#include "ip2_api.h"
#include "conf_api.h"
#include "mac_api.h"
#include "msg_api.h"



//#include "main.h"
#include "cli.h"
//#include "cli_api.h"
#include "mgmt_api.h"
//#include "port_api.h"
//#include "mac_api.h"
//#include "conf_api.h"
//#include "mac_cli.h"
//#include "vtss_module_id.h"
//#include "cli_trace_def.h"
//#include "msg_api.h"


void mac_entry_print(vtss_mac_table_entry_t *mac_entry);
/****************************************************************************/
/*  Trace stuff     all of this is in gvcp.c ... do not re-define           */
/****************************************************************************/
//#if (VTSS_TRACE_ENABLED)
//// Trace registration. Initialized by vns_fpga_init() */
//static vtss_trace_reg_t gvcp_trace_reg =
//{
//  .module_id = VTSS_TRACE_MODULE_ID,
//  .name      = "gvcp",
//  .descr     = "GVCP module"
//};
//
//#ifndef VTSS_GVCP_DEFAULT_TRACE_LVL
//#define VTSS_GVCP_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
//#endif
//#define GVCP_TRACE_GRP_CNT          1
//
//static vtss_trace_grp_t gvcp_trace_grps[TRACE_GRP_CNT] = {
//  [VTSS_TRACE_GRP_DEFAULT] = {
//    .name      = "default",
//    .descr     = "Default",
//    .lvl       = VTSS_GVCP_DEFAULT_TRACE_LVL,
//    .timestamp = 1,
//  },
//};
//#endif /* VTSS_TRACE_ENABLED */
typedef struct {
    ulong      mac_max;
    ulong      mac_age_time;
    BOOL       auto_keyword;
    BOOL       secure;
    int        int_values[CLI_INT_VALUES_MAX];
} mac_cli_req_t;

void Set_Packet_Header(unsigned char *pckt, unsigned char *header)
{
	memcpy(pckt, header, ACK_HEADER_MAX_SIZE );
}
void init_linked_list(linked_list_t * list)
{
	list->head = NULL;
	list->tail = NULL;
	list->conductor = NULL;
	list->list_size = 0;
	return;
}

void append_node( linked_list_t * list, void *data, size_t data_size )
{
	linked_list_node_t *node = malloc(sizeof(linked_list_node_t));
	T_D("malloc for new node size: %d", sizeof(linked_list_node_t));
	if(node == NULL)
	{
		perror("Error:Out of Memory...\n");
		exit(1);
	}
	node->data = malloc(data_size);
	T_D("malloc for node_data size: %d", sizeof(data_size));
	if(node->data == NULL)
	{
		node->data_size = 0;
		perror("Error:Out of Memory...\n");
		exit(1);
	}
	memcpy(node->data,data,data_size);
	node->data_size = data_size;

	if( list->head == NULL )
	{						/* if there are no nodes in list, then         */
	   list->head = node;         //../* set head to this new node
	   list->tail = node;
	}
	else
	{
	   list->tail->next = (linked_list_node_t *)node;
	   list->tail = node;
	}
	node->next = NULL;
	list->list_size++;

}
void remove_node( linked_list_t * list, void * data )
{


}
void clear_list(linked_list_t * list)
{
	if(list->head != NULL)
	{
		while(list->head != list->tail)
		{
			T_D("In clear_list loop");
			list->conductor = list->head;
			list->head = (linked_list_node_t *)(list->head->next);
			list->list_size--;
			if(list->conductor->data != NULL)
			{
				T_D("freeing malloc for node_data size: %d", list->conductor->data_size);
				free(list->conductor->data);
				list->conductor->data = NULL;
			}
			if(list->conductor != NULL)
			{
				T_D("freeing malloc for node size: %d", sizeof(linked_list_node_t));
				free(list->conductor);
				list->conductor = NULL;
			}
		}
		T_D("exit clear_list loop");
		list->conductor = list->head;
		if(list->conductor->data != NULL)
		{
			T_D("freeing malloc for node_data size: %d", list->conductor->data_size);
			free(list->conductor->data);
			list->conductor->data = NULL;
		}
		if(list->conductor != NULL)
		{
			T_D("freeing malloc for node size: %d", sizeof(linked_list_node_t));
			free(list->conductor);
			list->conductor = NULL;
		}
	}
	init_linked_list(list);
}
void print_list(linked_list_t *list)//, void(*print)(void const*))
{
	list->conductor = list->head;

	if(list->conductor != NULL)
	{
//		T_D("Node: 0x%X",list->conductor->data);
		while(list->conductor->next != NULL)
		{
			list->conductor = (linked_list_node_t *)(list->conductor->next);
//			T_D("Node: 0x%X",list->conductor->data);
		}
	}
	else
	{
		T_D("There are no items in this list... Head set to NULL");
	}
}
uint32_t parseIPV4string(char* ipAddress)
{
  int ipbytes[4];
  sscanf(ipAddress, "%i.%i.%i.%i", &ipbytes[3], &ipbytes[2], &ipbytes[1], &ipbytes[0]);


  return ((unsigned int)(ipbytes[0] | ipbytes[1] << 8 | ipbytes[2] << 16 | ipbytes[3] << 24));
}
void remove_all_chars(char* str, char c)
{
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}
int get_vns_ip_addr_subnet(char * host_name_sub, size_t host_len)
{
	///*************************************** Retrieving IP ***************************
	vtss_if_id_vlan_t vid;
//	unsigned int ifct;
//	vtss_if_status_t ifstat;
	vtss_rc rc;
	vid = VTSS_VID_NULL;
	int ct;
    char buf[128];
    char * host_name_sub_ptr;

	 while (vtss_ip2_if_id_next(vid, &vid) == VTSS_OK) {
	     vtss_ip_conf_t ipconf, ipconf6;
	     vtss_if_status_t ifstat;
	     unsigned int ifct;
	     if ((rc = vtss_ip2_ipv4_conf_get(vid, &ipconf)) == VTSS_OK) {
	         /* Vid + IPv4 (dhcp+static) */
	         ct = snprintf(host_name_sub, host_len,
	                       "%d#%d#",
	                       vid,
	                       ipconf.dhcpc);
	         if (ipconf.dhcpc || ipconf.network.address.type != VTSS_IP_TYPE_IPV4) {
	         } else {
	             (void) vtss_ip2_ip_addr_to_txt(buf, sizeof(buf), &ipconf.network.address);
	             ct = snprintf(host_name_sub, host_len,
	                           "%s#%d#", buf, ipconf.network.prefix_size);
	         }
	         /* IPv4 - current */
	         if (vtss_ip2_if_status_get(VTSS_IF_STATUS_TYPE_IPV4, vid, 1, &ifct, &ifstat) == VTSS_OK &&
	             ifct == 1 &&
	             ifstat.type == VTSS_IF_STATUS_TYPE_IPV4) {
	             ct = snprintf(host_name_sub, host_len,
	                           VTSS_IPV4N_FORMAT,
	                           VTSS_IPV4N_ARG(ifstat.u.ipv4.net));
	         }
	         if (vtss_ip2_hasipv6() && (rc = vtss_ip2_ipv6_conf_get(vid, &ipconf6)) == VTSS_OK) {
	             /* IPv6 - configured */
	             if (ipconf6.network.address.type == VTSS_IP_TYPE_IPV6) {
	                 (void) misc_ipv6_txt(&ipconf6.network.address.addr.ipv6, buf);
	                 ct = snprintf(host_name_sub, host_len, "%s#%d#", buf, ipconf6.network.prefix_size);
	             } else {
	             }
	         }
	     } else {
	     }
	     /* End of interface */
	  }
	 host_name_sub_ptr = strtok(host_name_sub,"\\" );
	 return 0;
	///*************************************** Retrieving IP ***************************
}
int get_vns_mac_addr(char * mac_addr, size_t mac_len)
{
	///*************************************** Retrieving MAC ***************************

	conf_board_t  conf;
	(void)conf_mgmt_board_get(&conf);
	//int ct =
			snprintf(mac_addr, mac_len, "%02x%02x%02x%02x%02x%02x",
	              conf.mac_address[0],
	              conf.mac_address[1],
	              conf.mac_address[2],
	              conf.mac_address[3],
	              conf.mac_address[4],
	              conf.mac_address[5]);
	return 0;
	///*************************************** Retrieving MAC ***************************
}
int get_vns_serial_num(char * serial_num, size_t mac_len)
{
	///*************************************** Retrieving MAC ***************************

	conf_board_t  conf;
	(void)conf_mgmt_board_get(&conf);
//	int ct =
			snprintf(serial_num, mac_len, "%02x%02x",
	              conf.mac_address[5],
	              conf.mac_address[4]);
	return 0;
	///*************************************** Retrieving MAC ***************************
}
int get_system_name(char * val, size_t val_size)
{
	system_conf_t conf;
T_D("...1");
	if (system_get_config(&conf) != VTSS_OK)
	{
T_D("...2");
		return -1;
	}

	memcpy(val,&conf.sys_name,val_size);
T_D("...3");
	return (0);
}
unsigned int get_vns_mac_addr_low(void)
{
	///*************************************** Retrieving MAC ***************************

	conf_board_t  conf;
	(void)conf_mgmt_board_get(&conf);
	unsigned int val = ((unsigned int)(conf.mac_address[2])) << 24;
	val |= ((unsigned int)(conf.mac_address[3])) << 16;
	val |= ((unsigned int)( conf.mac_address[4])) << 8;
	val |= (unsigned int)(conf.mac_address[5]);

	return val;
	///*************************************** Retrieving MAC ***************************
}
unsigned short get_vns_mac_addr_high(void)
{
	///*************************************** Retrieving MAC ***************************

	conf_board_t  conf;
	(void)conf_mgmt_board_get(&conf);

	unsigned short val = ((unsigned short)(conf.mac_address[0])) << 8;
	val |= (unsigned short)(conf.mac_address[1]);
	return val;
	///*************************************** Retrieving MAC ***************************
}
unsigned int cidr_to_subnet_mask(int cidr_num)
{
	int i;
	unsigned int subnet = 0x80000000;
	if((cidr_num < 0) || (cidr_num > 32) )
	{
		T_E("CIDR is out of range: %u",cidr_num);
		return 0x00000001;
	}
	for(i = 31; i > (31-cidr_num); i-- )
	{
		subnet |= 1 << i;
	}
	return subnet;
}

int build_gvcpreg_mac_table_entry(gvcpreg_mac_table_entry_t *gvcpreg_mac_entry, vtss_mac_table_entry_t *vtss_mac_entry, unsigned int reg_addr)
{

	char buf[MGMT_PORT_BUF_SIZE];



// T_D("Building gvcp reg mac table enty...");
	gvcpreg_mac_entry->vtss_mac_table_entry = *vtss_mac_entry;
// T_D("..");
	//misc_mac_txt(mac_entry->vid_mac.mac.addr, buf));
	uchar * mac = vtss_mac_entry->vid_mac.mac.addr;
// T_D("..2\n");
//	uchar * port_ptr = vtss_mac_entry->destination;
// T_D("port_ptr: %08X  %s",port_ptr,cli_iport_list_txt(vtss_mac_entry->destination, buf));
	unsigned short port = atoi(cli_iport_list_txt(vtss_mac_entry->destination, buf));

// T_D("..3");
	gvcpreg_mac_entry->mactbl_addr_low = (unsigned int)(mac[5]| mac[4] << 8|mac[3] << 16|mac[2] << 24); // set lower 32 bits of mac
// T_D("..4");
	gvcpreg_mac_entry->mactbl_addr_high = (unsigned int)(mac[1] | mac[0] << 8|port << 16); // set lower 16 bits to upper part of mac addr

// T_D("port: %08X",port);

	gvcpreg_mac_entry->reg_addr_high = reg_addr + 1;
// T_D("..6 reg_high: %X",gvcpreg_mac_entry->reg_addr_high);
	gvcpreg_mac_entry->reg_addr_low = reg_addr + 2;
// T_D("..7reg_low: %X",gvcpreg_mac_entry->reg_addr_low);
	return 0;
}
void free_gvcpreg_mac_table_entry(gvcpreg_mac_table_entry_t * entry)
{
	if(entry != NULL)
	{
		free(entry);
	}
}
void print_mac_table(void)
{
	int i = 0;
	gvcpreg_mac_table_entry_t * entry;
	MAC_TABLE.conductor = MAC_TABLE.head;
	T_D("****** Therer are %d nodes in list",MAC_TABLE.list_size);
	while(MAC_TABLE.conductor != NULL)
	{
		entry =  MAC_TABLE.conductor->data;
		T_D("\nNode_%d\n",i);
		i++;
		T_D("\t%X",entry->mactbl_addr_high);
		T_D("\t%X",entry->mactbl_addr_low);
		T_D("\t%X",entry->reg_addr_high);
		T_D("\t%X",entry->reg_addr_low);
		MAC_TABLE.conductor = (linked_list_node_t *)MAC_TABLE.conductor->next;

	}
}

static vtss_isid_t get_local_isid (void)
{
    vtss_isid_t      isid;

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (!msg_switch_exists(isid))
            continue;
        if (msg_switch_is_local(isid)) {
            break;
        }
    }
    return isid;
}

int build_mac_table(linked_list_t *list)
{
	// ******************************************************************************
	// * returns number of devices connected..
	// * Created from from: static void mac_cli_cmd_mac_dump ( cli_req_t * req )
	// ******************************************************************************

	vtss_usid_t            usid;
    vtss_isid_t            isid;
    ulong                  i, max;
    vtss_vid_mac_t         vid_mac;
    vtss_mac_table_entry_t mac_entry;
    gvcpreg_mac_table_entry_t  gvcpreg_mac_entry;
    unsigned int reg_addr = 0xF000;

    max = 0xFFFFFFFF;

    /* isid was being uninitialized... added the following to hopefully avoid any issues */
    isid = get_local_isid();

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++)
    {

        vid_mac.vid = 1;
        for (i = 0; i < 6; i++)
            vid_mac.mac.addr[i] = 0;

        for (i = 0; i < max; i++)
        {
            if (i == 0)
            {
                if (mac_mgmt_table_get_next(isid, &vid_mac, &mac_entry, FALSE) != VTSS_OK)
                {
                    // If lookup wasn't found do a lookup of the next entry
                    if (mac_mgmt_table_get_next(isid, &vid_mac, &mac_entry, TRUE) != VTSS_OK)
                    {
                        break ;
                    }
                }
            }
            else
            {
                if (mac_mgmt_table_get_next(isid, &vid_mac, &mac_entry, 1) != VTSS_OK)
                {
                	break;
                }
            }
            if(!mac_entry.locked)
            {
T_D("gvcpreg_mac_table_entry_ptr = build_gvcpreg_mac_table_entry(&mac_entry,reg_addr)\n");
            	build_gvcpreg_mac_table_entry(&gvcpreg_mac_entry, &mac_entry,reg_addr);
            	reg_addr = reg_addr + 2;
T_D("append_node(list,&gvcpreg_mac_entry, sizeof(gvcpreg_mac_table_entry_t)");
            	append_node(list,&gvcpreg_mac_entry, sizeof(gvcpreg_mac_table_entry_t));
//T_D("mac_entry_print(&mac_entry");
//				mac_entry_print(&mac_entry);
//T_D("finished..mac_entry_print(&mac_entry")	;
			}
            vid_mac = mac_entry.vid_mac;
        }
    }
    return ((list->list_size));
}
void mac_entry_print(vtss_mac_table_entry_t *mac_entry)
{
    char buf[MGMT_PORT_BUF_SIZE];
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    if (vtss_stacking_enabled()) {
        if (!msg_switch_is_master())
            T_D("%-9s", mac_entry->vstax2.remote_entry ? "Remote" : "Local");
        else
            T_D("%-9d", mac_mgmt_upsid2usid(mac_entry->vstax2.upsid));
    }
#endif
    T_D("%s %-4d %s  ",
            mac_entry->locked ? "Static " : "Dynamic",
            mac_entry->vid_mac.vid, misc_mac_txt(mac_entry->vid_mac.mac.addr, buf));

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    T_D("%s%s\n",
            (!msg_switch_is_master() && mac_entry->vstax2.remote_entry) ? "-" : cli_iport_list_txt(mac_entry->destination, buf),
            mac_entry->copy_to_cpu ? ",CPU" : "");
#else
    T_D("%s%s\n", cli_iport_list_txt(mac_entry->destination, buf), mac_entry->copy_to_cpu ? ",CPU" : "");

#endif

}
int change_ip_address(unsigned short vlan, unsigned int ip_address, unsigned int subnet )
{
	  vtss_ip_conf_t conf;
//	  char *ip_net_ptr;
	//  unsigned char *parse = ip_address;
	//  sT_D(ip_net_ptr,"%02X.%02X.%02X.%02X/%d",parse,parse +1,parse +2,parse+3, subnet_mask_to_cidr(subnet) );
	  if(vtss_ip2_ipv4_conf_get(vlan, &conf) != VTSS_RC_OK)
	  {
		return -1;
	  }
	  conf.network.address.addr.ipv4 = ip_address; // needs XXX.XXX.XXX.XXX/ZZ Format
	  T_D("Subnet to be set: %08X",subnet);
	  T_D("Subnet converted: %08X",subnet_mask_to_cidr(subnet));
	  conf.network.prefix_size = subnet_mask_to_cidr(subnet);
	  if(vtss_ip2_ipv4_conf_set(vlan, &conf) != VTSS_RC_OK)
	  {
		return -1;
	  }
	  return 0;
}
unsigned short subnet_mask_to_cidr(unsigned int subnet_mask)
{
	short int i;
	unsigned int bits_in_int = sizeof(int)<<4;
	unsigned int bit_mask = 0x80000000;
	for(i=0; i < bits_in_int; i++)
	{
		if(!(bit_mask & (subnet_mask << i)) )
		{
			return i;
		}
	}
	return -1;
}
unsigned int get_vns_gateway(void)
{
	char host_name_cidr_sub[INET6_ADDRSTRLEN];
	char *host_name_cidr_sub_ptr = host_name_cidr_sub;
	char *host_name,* host_subnet;
	get_vns_ip_addr_subnet(host_name_cidr_sub_ptr, sizeof(host_name_cidr_sub));

	host_name = strtok(host_name_cidr_sub,"/");
	host_subnet = strtok(NULL,"/");
	unsigned int gateway_u32 = parseIPV4string(host_name);
	unsigned short cidr_subnet = atoi(host_subnet);

	int i;

	if((cidr_subnet < 1) || (cidr_subnet > 30) )
	{
		return 0x00000001;
	}
	for(i = 0; i < (32-cidr_subnet); i++ )
	{
		gateway_u32 |= 1 << i;
	}
	return gateway_u32;

}
void trigger_gvcp(void)
{
	T_D("Entering trigger");
	int sockfd = 0;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
//	char s[INET6_ADDRSTRLEN];
//	int broadcast = 1;

	int i;
	//char * port_name;
//	char * ip_name;
//	char port_name[16];

//	unsigned int val;
	//char ip_name[16];





	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, GVCP_SERVERPORT, &hints, &servinfo)) != 0)
	{
		T_E( "getaddrinfo: %s\n", gai_strerror(rv));
		return ;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
		{
			perror("listener: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

T_D("After Bind p->ai_addr:%u", (unsigned int)p->ai_addr);

	if (p == NULL) {
		T_D( "listener: failed to bind socket");
		return ;
	}


	addr_len = sizeof (their_addr);
	i=0;




T_D("listener: waiting to recvfrom...");

		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
		}
T_D("Packet Recieved!!");

T_D("Exiting trigger");
			close(sockfd);
			return;
}
void gvcpT_D(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	memset(trace_buff, '\0', TRACE_BUFF_LEN);
	vsnprintf(trace_buff, (unsigned long int)TRACE_BUFF_LEN, format, args);
	va_end(args);
	T_D("%s", trace_buff);
}

