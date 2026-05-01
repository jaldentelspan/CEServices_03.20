/*
 * listener.c
 *
 *  Created on: Aug 7, 2014
 *      Author: Jason Alden
 */


/*
** listener.c -- a datagram sockets "server"
*/
//#include "main.h"

//#include "misc_api.h"
#include "gvcp.h"
#include "gvcp_api.h"
//#include "critd_api.h"
/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
*/
#include <network.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "gvcp_cmd_packet.h"
#include "gvcp_ack_packet.h"
#include "gvcp_types.h"
#include "readreg.h"

#define GVCP_LISTEN_STATUS_CLEAN 0
#define GVCP_LISTEN_STATUS_FORCEIP_OK 1
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_GVCP
#define DEBUG 1
#define GVCP_DEBUG 1



#define MAXBUFLEN 100
#define GVCP_USING_ISGVCP_PACKAGE 1

int gvcp_listen(void);
void *get_in_addr(struct sockaddr *sa);

/****************************************************************************/
/*  Trace stuff                                                             */
/****************************************************************************/
#if (VTSS_TRACE_ENABLED)
// Trace registration. Initialized by vns_fpga_init() */
static vtss_trace_reg_t gvcp_trace_reg =
{
  .module_id = VTSS_TRACE_MODULE_ID,
  .name      = "gvcp",
  .descr     = "GVCP module"
};

#ifndef VTSS_GVCP_DEFAULT_TRACE_LVL
#define VTSS_GVCP_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif
#define GVCP_TRACE_GRP_CNT          1

static vtss_trace_grp_t gvcp_trace_grps[TRACE_GRP_CNT] = {
  [VTSS_TRACE_GRP_DEFAULT] = {
    .name      = "default",
    .descr     = "Default",
    .lvl       = VTSS_GVCP_DEFAULT_TRACE_LVL,
    .timestamp = 1,
  },
};
#endif /* VTSS_TRACE_ENABLED */
/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
//static gvcp_global_t GVCP_global;

/* Thread variables */
#define GVCP_CERT_THREAD_STACK_SIZE       16384
static cyg_handle_t GVCP_thread_handle;
static cyg_thread   GVCP_thread_block;
static char         GVCP_thread_stack[GVCP_CERT_THREAD_STACK_SIZE];

//************************* Testing for Broadcast functionality ***/****
//
//static int
//broadcast_startup(const char *intf, struct ifreq *ifrp )
//{
//    int s = -1;
//    int one = 1;
//
//    struct sockaddr_in *addrp;
//    struct ecos_rtentry route;
//    int retcode = false;
//
//    // Ensure clean slate
//    cyg_route_reinit("eth0");  // Force any existing routes to be forgotten
//
//    s = socket(AF_INET, SOCK_DGRAM, 0);
//    if (s < 0) {
//        perror("socket");
//        goto out;
//    }
//
//    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one))) {
//        perror("setsockopt");
//        goto out;
//    }
//
//    addrp = (struct sockaddr_in *) &ifrp->ifr_addr;
//    memset(addrp, 0, sizeof(*addrp));
//    addrp->sin_family = AF_INET;
//    addrp->sin_len = sizeof(*addrp);
//    addrp->sin_port = 0;
//    addrp->sin_addr.s_addr = INADDR_ANY;
//
//    strcpy(ifrp->ifr_name, intf);
//    if (ioctl(s, SIOCSIFADDR, ifrp)) { /* set ifnet address */
//        perror("SIOCSIFADDR");
//        goto out;
//    }
//
//    if (ioctl(s, SIOCSIFNETMASK, ifrp)) { /* set net addr mask */
//        perror("SIOCSIFNETMASK");
//        goto out;
//    }
//
//    /* the broadcast address is 255.255.255.255 */
//    memset(&addrp->sin_addr, 255, sizeof(addrp->sin_addr));
//    if (ioctl(s, SIOCSIFBRDADDR, ifrp)) { /* set broadcast addr */
//        perror("SIOCSIFBRDADDR");
//        goto out;
//    }
//
//    ifrp->ifr_flags = IFF_UP | IFF_BROADCAST | IFF_RUNNING;
//    if (ioctl(s, SIOCSIFFLAGS, ifrp)) { /* set ifnet flags */
//        perror("SIOCSIFFLAGS up");
//        goto out;
//    }
//
//    if (ioctl(s, SIOCGIFHWADDR, ifrp) < 0) { /* get MAC address */
//        perror("SIOCGIFHWADDR 1");
//        goto out;
//    }
//
//    // Set up routing
//    addrp->sin_family = AF_INET;
//    addrp->sin_port = 0;
//    addrp->sin_len = sizeof(*addrp);  // Size of address
//
//    /* the broadcast address is 255.255.255.255 */
//    memset(&addrp->sin_addr, 255, sizeof(addrp->sin_addr));
//    memset(&route, 0, sizeof(route));
//    memcpy(&route.rt_gateway, addrp, sizeof(*addrp));
//
//    addrp->sin_addr.s_addr = INADDR_ANY;
//    memcpy(&route.rt_dst, addrp, sizeof(*addrp));
//    memcpy(&route.rt_genmask, addrp, sizeof(*addrp));
//
//    route.rt_dev = ifrp->ifr_name;
//    route.rt_flags = RTF_UP|RTF_GATEWAY;
//    route.rt_metric = 0;
//
//    if (ioctl(s, SIOCADDRT, &route)) { /* add route */
//        if (errno != EEXIST) {
//            perror("SIOCADDRT 3");
//            goto out;
//        }
//    }
//    retcode = true;
// out:
//    if (s != -1)
//      close(s);
//
//    return retcode;
//}
//************************* Testing for Broadcast functionality **

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/****************************************************************************
 * Module thread
 ****************************************************************************/
/* We create a new thread to do it for instead of in 'Init Modules' thread.
   That we don't need wait a long time in 'Init Modules' thread. */
static void GVCP_thread(cyg_addrword_t data)
{
#if GVCP_USING_ISGVCP_PACKAGE
//int gvcp_listen_status = GVCP_LISTEN_STATUS_CLEAN;
	   while (1)
	   {
	        VTSS_OS_MSLEEP(1000);
	        T_D("GVCP thread has started and is looping..");
	        if(!gvcp_listen())
	        {
	        	T_D("gvcp_listen_status out of range.. Starting clean");

	        }
	   }
//	CPRINTF("GVCP thread has started..");
#else
    while (1) {
        VTSS_OS_MSLEEP(1000);
    }
#endif /* GVCP_USING_ISGVCP_PACKAGE */
}
int gvcp_listen()
{

	int sockfd;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	int numbytes;
	char buf[MAXBUFLEN];

	int rv;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	hints.ai_protocol = IPPROTO_UDP ;//0 (auto) or IPPROTO_TCP, IPPROTO_UDP
	hints.ai_canonname = 	NULL;
	hints.ai_addr = 	NULL;
	hints.ai_next = 	NULL;

//************* Setup Broadcast End Point Structure ********************************
	struct sockaddr_in brdcst_addr; // connector's address information
	int broadcast = 1;
	//char broadcast = '1';
	memset((char *) &brdcst_addr, 0, sizeof(brdcst_addr));
	brdcst_addr.sin_family = AF_INET;
	brdcst_addr.sin_len = sizeof(brdcst_addr);
	brdcst_addr.sin_port = htons(atoi(GVCP_SERVERPORT)); // short, network byte order
//	brdcst_addr.sin_addr.s_addr = htonl(get_vns_gateway());
	brdcst_addr.sin_addr.s_addr =  htonl(INADDR_BROADCAST);
	memset(brdcst_addr.sin_zero, '\0', sizeof brdcst_addr.sin_zero);

//*********** Setup GVCP Structures ***************************
	gvcp_discovery_ack_t disc_pckt;
	gvcp_readreg_ack_t readreg_pckt;
	readreg_pckt.data = NULL;
	gvcp_cmd_header_t *cmd_header;
	gvcp_forceip_ack_t forceip_pckt;
	linked_list_t readreg_list;
	init_linked_list(&readreg_list);
	init_linked_list(&MAC_TABLE);
//	struct ifreq ifr;
//    if ( ! broadcast_startup( "eth0", &ifr ) )
//    	T_D("broadcast setup failed ");

//************* Setup Socket *********************************
	if ((rv = getaddrinfo(NULL, "3956", &hints, &servinfo)) != 0)
	{
		//T_W(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		goto SHUTDOWN_SEQ;
	}
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			T_W("li#include <vtss_trace_api.h>stener: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
		{
			T_W("setsockopt (SO_BROADCAST)");

		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			T_W("listener: bind");
			continue;
		}

		break;
	}
	T_D("After Bind p->ai_addr:%u", (unsigned int)p->ai_addr);

	if (p == NULL) {
		T_W("listener: failed to bind socket");
		goto SHUTDOWN_SEQ;
	}

	freeaddrinfo(servinfo);           /* No longer needed */

	while(1)
	{
//************* Waiting for connection
		addr_len = sizeof(struct sockaddr_storage);
		T_D("listener: waiting to recvfrom...\n");
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1)
		{
			T_W("recvfrom");
			goto SHUTDOWN_SEQ;
		}
		T_D("Packet Recieved!!");
		cmd_header = (gvcp_cmd_header_t *) buf;

		if(Get_Cmd_Command(cmd_header) == DISCOVERY_CMD)
		{
			T_D("Entered DISCOVERY_CMD\n");
			build_gvcp_discovery_ack_packet(&disc_pckt,	Get_Cmd_REQ_ID(cmd_header));
			T_D("DISCOVERY Packet has been built");
			T_D("Sending to: packet\n");
			if ((numbytes=sendto(sockfd,&disc_pckt, sizeof(disc_pckt),0,(struct sockaddr *)&their_addr, addr_len)) == -1)
			{
				if(!(BROADCAST_CMD_FLAG_MASK & Get_Cmd_Flag(cmd_header)))
				{
					T_D("Sending Failed.. trying to broadcast");
					if ((numbytes=sendto(sockfd,&disc_pckt, sizeof(disc_pckt),0,(struct sockaddr *)&brdcst_addr, sizeof(brdcst_addr))) == -1)
					{
						T_W("sendto");
						goto SHUTDOWN_SEQ;
					}
				}
				else
				{
					T_W("sendto");
					goto SHUTDOWN_SEQ;
				}
			}
		}
		else if(Get_Cmd_Command(cmd_header) == READREG_CMD)
		{
			T_D("Entered READREG_CMD");
			T_D("Start Get_Cmd_ReadReg_Addr");

			Get_Cmd_ReadReg_Addr(&readreg_list, buf,Get_Cmd_Length(cmd_header));
			T_D("Packet details: packet length: %d", Get_Cmd_Length(cmd_header));

			Gvcp_Read_Reg_Batch(&readreg_list,&readreg_list);
			if(build_gvcp_readreg_ack_packet_malloc(&readreg_pckt,&readreg_list,Get_Cmd_Length(cmd_header),Get_Cmd_REQ_ID(cmd_header)))
			{
				T_E("OUT OF MEMORY; GVCP read register malloc size %d", readreg_pckt.data_size);
				exit(1);
			}
			T_D("GVCP read register malloc size %d", readreg_pckt.data_size);
			if ((numbytes = sendto(sockfd, readreg_pckt.data, readreg_pckt.data_size, 0,(struct sockaddr *)&their_addr, addr_len)) == -1)
			{
				T_W("sendto failed");
				goto SHUTDOWN_SEQ;
			}
			free_gvcp_readreg_ack_packet_malloc(&readreg_pckt);
			T_D("GVCP read register malloc free size %d", readreg_pckt.data_size);
			clear_list(&readreg_list);
		}
		else if(Get_Cmd_Command(cmd_header) == FORCEIP_CMD)
		{
			T_D("Entered FORCEIP_CMD");
			gvcp_forceip_cmd_t *forceip_cmd_pckt = (gvcp_forceip_cmd_t *) buf;
			int forceip_status;
			T_D("checking to see if it is a matching MAC");
			if( get_vns_mac_addr_high() == Get_Cmd_Device_MAC_High(forceip_cmd_pckt) &&
					get_vns_mac_addr_low() == Get_Cmd_Device_MAC_Low(forceip_cmd_pckt))
			{
				T_D("Setting IP address");
				forceip_status = change_ip_address(VLAN_1,Get_Cmd_Cur_IP(forceip_cmd_pckt),Get_Cmd_Cur_Subnet(forceip_cmd_pckt));
				T_D("Build ForceIP Packet");
				build_gvcp_forceip_ack_packet(&forceip_pckt, Get_Cmd_REQ_ID(cmd_header), forceip_status);
				T_D("Sending FORCEIP ACK");
				if ((numbytes = sendto(sockfd, &forceip_pckt, sizeof(gvcp_forceip_ack_t), 0,(struct sockaddr *)&their_addr, addr_len)) == -1)
				{
					if(!(BROADCAST_CMD_FLAG_MASK & Get_Cmd_Flag(cmd_header)))
					{
						if ((numbytes=sendto(sockfd,&disc_pckt, sizeof(disc_pckt),0,(struct sockaddr *)&brdcst_addr, sizeof(brdcst_addr))) == -1)
						{
							T_W("talker: sendto");
							goto SHUTDOWN_SEQ;
						}
					}
					else
					{
						T_W("talker: sendto");
						goto SHUTDOWN_SEQ;
					}

				}
			goto SHUTDOWN_SEQ;

			}
		}

	}

SHUTDOWN_SEQ:
T_D("Freeing readreg packet...");
	free_gvcp_readreg_ack_packet_malloc(&readreg_pckt);
T_D("Closing Socket...");
	close(sockfd);
T_D("Freeing addrinfo...");
	freeaddrinfo(servinfo);
T_D("Freeing readreg list...");
	clear_list(&readreg_list);
	return 0;

}
void StartGVCP()
{
//	   gvcp_conf_t *conf_p;



	    /* Initialize DHCP relay configuration */
	//    conf_p = &GVCP_global.gvcp_conf;
	  //  DHCP_RELAY_default_set(conf_p);

	    /* Create semaphore for critical regions */
	//    critd_init(&GVCP_global.crit, "GVCP_global.crit", VTSS_MODULE_ID_GVCP, VTSS_TRACE_MODULE_ID_GVCP, CRITD_TYPE_MUTEX);
	//    GVCP_CRIT_EXIT();


	    /* Create DHCP relay thread */
	    cyg_thread_create(THREAD_DEFAULT_PRIO,
	                      GVCP_thread,
	                      0,
	                      "GVCP",
	                      GVCP_thread_stack,
	                      sizeof(GVCP_thread_stack),
	                      &GVCP_thread_handle,
	                      &GVCP_thread_block);


	    //T_D("exit");
}
/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

vtss_rc gvcp_init(vtss_init_data_t *data)
{

    switch (data->cmd) {
    case INIT_CMD_INIT:
#if (VTSS_TRACE_ENABLED)
        /* Initialize and register trace ressources */
		 VTSS_TRACE_REG_INIT(&gvcp_trace_reg, gvcp_trace_grps, GVCP_TRACE_GRP_CNT);
		 VTSS_TRACE_REGISTER(&gvcp_trace_reg);


#endif
        break;
    case INIT_CMD_START:
    	StartGVCP();
    	break;
    case INIT_CMD_MASTER_UP:
        /* Starting GVCP thread */
        cyg_thread_resume(GVCP_thread_handle);
        break;
    case INIT_CMD_CONF_DEF:
    default:
        break;
    }


    return 0;
}
