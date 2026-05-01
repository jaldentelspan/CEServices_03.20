/*

 Vitesse Switch API software.

 Copyright (c) 2002-2012 Vitesse Semiconductor Corporation "Vitesse". All
 Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted. Permission to
 integrate into other products, disclose, transmit and distribute the software
 in an absolute machine readable format (e.g. HEX file) is also granted.  The
 source code of the software may not be disclosed, transmitted or distributed
 without the written permission of Vitesse. The software and its source code
 may only be used in products utilizing the Vitesse switch products.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software. Vitesse retains all ownership,
 copyright, trade secret and proprietary rights in the software.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
 INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR USE AND NON-INFRINGEMENT.

*/

#include "main.h"
#include "conf_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "vtss_ecos_mutex_api.h"
#include "misc_api.h"
#if defined(VTSS_SW_OPTION_IP2)
#include "ip2_api.h"
#include "packet_api.h"
#if defined(VTSS_SW_OPTION_DHCP_CLIENT)
#include "dhcp_client_api.h"
#endif /* VTSS_SW_OPTION_DHCP_CLIENT */
#endif /* VTSS_SW_OPTION_IP2 */







#include "dhcp_helper_api.h"
#include "dhcp_helper.h"
#include "acl_api.h"
#include "port_api.h"
#include "vlan_api.h"
#include "syslog_api.h"
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#else
// #include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */

#include <network.h>
#include <dhcp.h>
#include <netinet/udp.h>

/* DHCP port number */
#define DHCP_SERVER_UDP_PORT    67
#define DHCP_CLINET_UDP_PORT    68

#define DHCP_REQUESTED_ADDRESS          50
#define DHCP_LEASE_TIME                 51

#ifndef IP_VHL_HL
#define IP_VHL_HL(vhl)      ((vhl) & 0x0f)
#endif /* IP_VHL_HL */


static void DHCP_HELPER_rx_filter_register(BOOL registerd);


/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static dhcp_helper_global_t DHCP_HELPER_global;

#if (VTSS_TRACE_ENABLED)
static vtss_trace_reg_t DHCP_HELPER_trace_reg = {
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "dhcphelper",
    .descr     = "DHCP helper, processing all DHCP received packets"
};

static vtss_trace_grp_t DHCP_HELPER_trace_grps[TRACE_GRP_CNT] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        .name      = "default",
        .descr     = "Default",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [TRACE_GRP_CRIT] = {
        .name      = "crit",
        .descr     = "Critical regions ",
        .lvl       = VTSS_TRACE_LVL_ERROR,
        .timestamp = 1,
    }
};
#define DHCP_HELPER_CRIT_ENTER() critd_enter(&DHCP_HELPER_global.crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#define DHCP_HELPER_CRIT_EXIT()  critd_exit( &DHCP_HELPER_global.crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#else
#define DHCP_HELPER_CRIT_ENTER() critd_enter(&DHCP_HELPER_global.crit)
#define DHCP_HELPER_CRIT_EXIT()  critd_exit( &DHCP_HELPER_global.crit)
#endif /* VTSS_TRACE_ENABLED */

/* Thread variables */
#define DHCP_HELPER_CERT_THREAD_STACK_SIZE       8192
static cyg_handle_t DHCP_HELPER_thread_handle;
static cyg_thread   DHCP_HELPER_thread_block;
static char         DHCP_HELPER_thread_stack[DHCP_HELPER_CERT_THREAD_STACK_SIZE];

/* Record incoming frame information */
#define DHCP_HELPER_FRAME_INFO_OBSOLETED    1500

struct dhcp_helper_frame_info_list {
    struct dhcp_helper_frame_info_list  *next;
    dhcp_helper_frame_info_t            info;
} *dhcp_helper_frame_info;

/* packet rx filter */
static packet_rx_filter_t  DHCP_HELPER_rx_filter;
static void                 *DHCP_HELPER_filter_id = NULL; // Filter id for subscribing dhcp packet.

/* Callback function ptr */
static dhcp_helper_stack_rx_callback_t DHCP_HELPER_dhcp_server_cb = NULL, DHCP_HELPER_dhcp_relay_cb = NULL, DHCP_HELPER_dhcp_snooping_cb = NULL;
static dhcp_helper_frame_info_callback_t DHCP_HELPER_frame_info_cb = NULL, DHCP_HELPER_release_frame_cb = NULL;


/****************************************************************************/
/*  Frame information maintain functions                                    */
/****************************************************************************/

/* Register DHCP incoming frame information */
void dhcp_helper_frame_info_register(dhcp_helper_frame_info_callback_t cb)
{
    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_frame_info_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }

    DHCP_HELPER_frame_info_cb = cb;
    DHCP_HELPER_CRIT_EXIT();
}

/* Register DHCP release frame */
void dhcp_helper_release_frame_register(dhcp_helper_frame_info_callback_t cb)
{
    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_release_frame_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }

    DHCP_HELPER_release_frame_cb = cb;
    DHCP_HELPER_CRIT_EXIT();
}

/* Clear DHCP helper frame information entry */
static void DHCP_HELPER_frame_info_clear(void)
{
    struct dhcp_helper_frame_info_list *sp;

    DHCP_HELPER_CRIT_ENTER();
    sp = dhcp_helper_frame_info;
    while (sp) {
        dhcp_helper_frame_info = sp->next;
        free(sp);
        sp = dhcp_helper_frame_info;
    }
    DHCP_HELPER_CRIT_EXIT();
}

/* Clear DHCP helper frame information obsoleted entry */
static void DHCP_HELPER_frame_info_clear_obsoleted(void)
{
    struct dhcp_helper_frame_info_list  *sp, *prev_sp = NULL;
    u32                                 timestamp = cyg_current_time();
    DHCP_HELPER_CRIT_ENTER();
    sp = dhcp_helper_frame_info;

    /* clear entries by mac and also clear time expired entries (15 seconds) */
    while (sp) {
        if (timestamp - sp->info.timestamp > DHCP_HELPER_FRAME_INFO_OBSOLETED) {
            if (prev_sp) {
                prev_sp->next = sp->next;
                free(sp);
                sp = prev_sp->next;
            } else {
                dhcp_helper_frame_info = sp->next;
                free(sp);
                sp = dhcp_helper_frame_info;
            }
            continue;
        }
        prev_sp = sp;
        sp = sp->next;
    }
    DHCP_HELPER_CRIT_EXIT();
}

/* Add DHCP helper frame information entry */
static void DHCP_HELPER_frame_info_add(dhcp_helper_frame_info_t *info)
{
    struct dhcp_helper_frame_info_list *sp, *search_sp;

    sp = ((struct dhcp_helper_frame_info_list *) malloc(sizeof(*sp)));
    if (!sp) {
        T_W("Unable to allocate %zu bytes for DHCP frame information", sizeof(*sp));
        return;
    }

    DHCP_HELPER_CRIT_ENTER();
    /* search entry exist? */
    for (search_sp = dhcp_helper_frame_info; search_sp; search_sp = search_sp->next) {
        if (!memcmp(search_sp->info.mac, info->mac, 6) && search_sp->info.transaction_id == info->transaction_id) {
            /* found it, update content */
            search_sp->info = *info;
            free(sp);
            DHCP_HELPER_CRIT_EXIT();
            return;
        }
    }

    sp->info = *info;
    sp->next = dhcp_helper_frame_info;
    dhcp_helper_frame_info = sp;
    DHCP_HELPER_CRIT_EXIT();
}

/* Delete DHCP helper frame information entry */
static void DHCP_HELPER_frame_info_del(dhcp_helper_frame_info_t *info)
{
    struct dhcp_helper_frame_info_list *sp, *prev_sp = NULL;

    DHCP_HELPER_CRIT_ENTER();
    for (sp = dhcp_helper_frame_info; sp; sp = sp->next) {
        if (!memcmp(sp->info.mac, info->mac, sizeof(sp->info.mac)) && sp->info.transaction_id == sp->info.transaction_id) {
            if (prev_sp == NULL) {
                dhcp_helper_frame_info = sp->next;
            } else {
                prev_sp->next = sp->next;
            }
            free(sp);
            break;
        }
        prev_sp = sp;
    }
    DHCP_HELPER_CRIT_EXIT();
}

#if 0
/* Delete DHCP helper frame information entry by MAC address */
static void DHCP_HELPER_frame_info_del_by_mac(dhcp_helper_frame_info_t *info)
{
    struct dhcp_helper_frame_info_list *sp, *prev_sp = NULL;

    DHCP_HELPER_CRIT_ENTER();
    sp = dhcp_helper_frame_info;

    /* clear entries by mac */
    while (sp) {
        if (!memcmp(sp->info.mac, info->mac, sizeof(sp->info.mac))) {
            if (prev_sp) {
                prev_sp->next = sp->next;
                free(sp);
                sp = prev_sp->next;
            } else {
                dhcp_helper_frame_info = sp->next;
                free(sp);
                sp = dhcp_helper_frame_info;
            }
            continue;
        }
        prev_sp = sp;
        sp = sp->next;
    }
    DHCP_HELPER_CRIT_EXIT();
}
#endif

/* Lookup DHCP helper frame information entry */
BOOL dhcp_helper_frame_info_lookup(u8 *mac, uint transaction_id, dhcp_helper_frame_info_t *info)
{
    struct dhcp_helper_frame_info_list *sp;

    DHCP_HELPER_CRIT_ENTER();
    sp = dhcp_helper_frame_info;
    while (sp) {
        if (!memcmp(sp->info.mac, mac, 6) && sp->info.transaction_id == transaction_id) {
            *info = sp->info;
            DHCP_HELPER_CRIT_EXIT();
            return TRUE;
        }
        sp = sp->next;
    }

    DHCP_HELPER_CRIT_EXIT();
    return FALSE;
}

/* Getnext DHCP helper frame information entry */
BOOL dhcp_helper_frame_info_getnext(u8 *mac, dhcp_helper_frame_info_t *info)
{
    u8 null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    DHCP_HELPER_CRIT_ENTER();
    if (!dhcp_helper_frame_info) {
        DHCP_HELPER_CRIT_EXIT();
        return FALSE;
    }
    DHCP_HELPER_CRIT_EXIT();

    if (!memcmp(null_mac, mac, 6)) {
        DHCP_HELPER_CRIT_ENTER();
        *info = dhcp_helper_frame_info->info;
        DHCP_HELPER_CRIT_EXIT();
        return TRUE;
    } else {
        struct dhcp_helper_frame_info_list  *sp;
        int                                 found = 0;

        DHCP_HELPER_CRIT_ENTER();
        sp = dhcp_helper_frame_info;
        while (sp) {
            if (!memcmp(sp->info.mac, mac, 6) && sp->info.transaction_id == info->transaction_id) {
                found = 1;
                break;
            }
            sp = sp->next;
        }
        DHCP_HELPER_CRIT_EXIT();

        if (found && sp && sp->next) {
            *info = sp->next->info;
            return TRUE;
        } else {
            return FALSE;
        }
    }
}


/****************************************************************************/
/*  Callback functions                                                      */
/****************************************************************************/

/* Do IP checksum */
static u16 DHCP_HELPER_do_ip_chksum(u16 ip_hdr_len, u16 ip_hdr[])
{
    u16  padd = (ip_hdr_len % 2);
    u16  word16;
    u32 sum = 0;
    int i;

    /* Calculate the sum of all 16 bit words */
    for (i = 0; i < (ip_hdr_len / 2); i++) {
        word16 = ip_hdr[i];
        sum += (u32)word16;
    }

    /* Add odd byte if needed */
    if (padd == 1) {
        word16 = ip_hdr[(ip_hdr_len / 2)] & 0xFF00;
        sum += (u32)word16;
    }

    /* Keep only the last 16 bits */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    /* One's complement of sum */
    sum = ~sum;

    return ((u16) sum);
}

static void DHCP_HELPER_do_rx_callback(const void *packet,
                                       size_t len,
                                       vtss_vid_t vid,
                                       vtss_isid_t isid,
                                       vtss_port_no_t port_no,
                                       vtss_glag_no_t glag_no)
{
    dhcp_helper_frame_info_t    record_info;
    u8                          *ptr = (u8 *)(packet);
    u16                         *ether_type = (u16 *)(ptr + 12);
    struct ip                   *ip = (struct ip *)(ptr + 14);
    int                         ip_header_len = IP_VHL_HL(ip->ip_hl) << 2;
    struct bootp                bp;
    u8                          *client_mac, dhcp_message;
    u32                         transaction_id, bp_client_addr, lease_time = 0;
    int                         get_completed_info = 0, is_request_ack = 0;
    u8                          system_mac_addr[6];
    u8                          broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};









    T_D("enter, RX isid %d vid %d port %d len %zd", isid, vid, port_no, len);

    /* We only process IPv4 now */
    if (*ether_type != htons(0x0800)) {
        return;
    }

    if (conf_mgmt_mac_addr_get(system_mac_addr, 0) < 0) {
        return;
    }

    memcpy(&bp, ptr + 14 + ip_header_len + 8, sizeof(bp)); /* 14:DA+SA+ETYPE, 8:udp header length */
    dhcp_message = bp.bp_vend[6];
    client_mac = (u8 *)bp.bp_chaddr;
    transaction_id = bp.bp_xid;
    bp_client_addr = bp.bp_ciaddr.s_addr;

    DHCP_HELPER_CRIT_ENTER();
    if (!DHCP_HELPER_dhcp_server_cb && !DHCP_HELPER_dhcp_relay_cb && DHCP_HELPER_dhcp_snooping_cb && memcmp(system_mac_addr, client_mac, 6)) {
        /* The statistics only counter packet under DHCP snooping mode is enabled
           and relay/server mode is disabled. And it doesn't count the DHCP packets for
           system DHCP client. */
        switch (dhcp_message) {
        case DHCP_HELPER_MSG_TYPE_DISCOVER:
            DHCP_HELPER_global.stats[isid][port_no].discover_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_OFFER:
            DHCP_HELPER_global.stats[isid][port_no].offer_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_REQUEST:
            DHCP_HELPER_global.stats[isid][port_no].request_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_DECLINE:
            DHCP_HELPER_global.stats[isid][port_no].decline_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_ACK:
            DHCP_HELPER_global.stats[isid][port_no].ack_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_NAK:
            DHCP_HELPER_global.stats[isid][port_no].nak_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_RELEASE:
            DHCP_HELPER_global.stats[isid][port_no].release_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_INFORM:
            DHCP_HELPER_global.stats[isid][port_no].inform_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_LEASEQUERY:
            DHCP_HELPER_global.stats[isid][port_no].leasequery_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_LEASEUNASSIGNED:
            DHCP_HELPER_global.stats[isid][port_no].leaseunassigned_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_LEASEUNKNOWN:
            DHCP_HELPER_global.stats[isid][port_no].leaseunknown_rx++;
            break;
        case DHCP_HELPER_MSG_TYPE_LEASEACTIVE:
            DHCP_HELPER_global.stats[isid][port_no].leaseactive_rx++;
            break;
        }
    }

    /* Drop server message if it come from untrust port */
    if (DHCP_HELPER_MSG_FROM_SERVER(dhcp_message) &&
        DHCP_HELPER_global.port_conf[isid].port_mode[port_no] == DHCP_HELPER_PORT_MODE_UNTRUSTED) {
        DHCP_HELPER_global.stats[isid][port_no].discard_untrust_rx++;
        DHCP_HELPER_CRIT_EXIT();
        memcpy(record_info.mac, client_mac, 6);
        record_info.transaction_id = transaction_id;
        DHCP_HELPER_frame_info_del(&record_info);
        if (!memcmp(system_mac_addr, client_mac, 6)) {
            char msg_buf[128];
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
            sprintf(msg_buf, "Receives the DHCP message that is reply to the switch but coming from the untrusted port %d/%d", topo_isid2usid(isid), iport2uport(port_no));
#else
            sprintf(msg_buf, "Receives the DHCP message that is reply to the switch but coming from the untrusted port %d", iport2uport(port_no));
#endif /* VTSS_SWITCH_STACKABLE */
            T_W("%s", msg_buf);
            S_W("%s", msg_buf);
        }
        return;
    }
    DHCP_HELPER_CRIT_EXIT();

    if (!memcmp(system_mac_addr, client_mac, 6)) {
        /* The packet is replied to system, forward it to TCP/IP stack.
           The system DHCP module will process the packet via socket interface. */
#if defined(VTSS_SW_OPTION_IP2)
#if defined(VTSS_SW_OPTION_DHCP_CLIENT)
        vtss_packet_rx_info_t rx_info;
        rx_info.length = len;
        rx_info.tag.vid = vid;
        rx_info.port_no = port_no;
        rx_info.glag_no = glag_no;
        (void) vtss_dhcp_client_packet_handler(NULL, packet, &rx_info);
#else
        if (vtss_ip2_if_inject((vtss_if_id_vlan_t) vid, len, packet)) {
            T_W("Calling vtss_ip2_if_inject() failed.\n");
        }

#endif /* VTSS_SW_OPTION_DHCP_CLIENT */
#else
        memset(&rx_info, 0, sizeof(rx_info));
        rx_info.length = len;
        rx_info.tag.vid = vid;

        /* Ingore ingress filter in IPP_rx_packet() */
        rx_info.port_no = VTSS_PORT_NO_NONE;
        rx_info.glag_no = VTSS_GLAG_NO_NONE;

        /* Make the IPP_rx_packet() is the only entry point before the incoming packets
           hit the TCP/IP stack. */
        /* Get rl by mapping from mamaged VID because the packet is replied to system */










        if (IPP_rx_packet(NULL, ptr, &rx_info)) {
            T_W("Calling IPP_rx_packet() failed.\n");
        }

#endif /* VTSS_SW_OPTION_IP2 */

        return;
    }

    memset(&record_info, 0x0, sizeof(record_info));

    /* Only record the incoming information form DHCP client */
    if (!DHCP_HELPER_MSG_FROM_SERVER(dhcp_message)) {
        /* Record incoming frame information */
        memcpy(record_info.mac, client_mac, 6);
        record_info.vid = vid;
        record_info.isid = isid;
        record_info.port_no = port_no;
        record_info.glag_no = glag_no;
        record_info.op_code = dhcp_message;
        record_info.transaction_id = transaction_id;
        if (bp_client_addr) {
            record_info.assigned_ip = ntohl(bp_client_addr);
        }
        record_info.timestamp = cyg_current_time();
        DHCP_HELPER_frame_info_add(&record_info);
    } else if (DHCP_HELPER_dhcp_snooping_cb && dhcp_message == DHCP_HELPER_MSG_TYPE_ACK) {
        /* Only snooping module need record lease time,
           we need get original incoming frame information first */
        memcpy(record_info.mac, client_mac, 6);
        record_info.transaction_id = transaction_id;
        if (!dhcp_helper_frame_info_lookup(record_info.mac, record_info.transaction_id, &record_info)) {
            return;
        }
        if (record_info.op_code == DHCP_HELPER_MSG_TYPE_REQUEST || record_info.op_code == DHCP_HELPER_MSG_TYPE_INFORM) {
            is_request_ack = 1;
        }
    }

    /* Record assigned IP and lease time */
    if (DHCP_HELPER_dhcp_snooping_cb) {
        /* Drop incoming reply packet if it come from untrust port */
        DHCP_HELPER_CRIT_ENTER();
        if (DHCP_HELPER_dhcp_snooping_cb && DHCP_HELPER_global.port_conf[isid].port_mode[port_no] == DHCP_HELPER_PORT_MODE_UNTRUSTED && DHCP_HELPER_MSG_FROM_SERVER(dhcp_message)) {
            DHCP_HELPER_CRIT_EXIT();
            return;
        }
        DHCP_HELPER_CRIT_EXIT();

        if ((bp_client_addr == 0 && dhcp_message == DHCP_HELPER_MSG_TYPE_REQUEST) ||
            (is_request_ack && dhcp_message == DHCP_HELPER_MSG_TYPE_ACK)) {
            u8 *op, *nextop, *sp, *max;

            max = (u8 *)(ptr + len);
            sp = op = &bp.bp_vend[4];

            if (!dhcp_helper_frame_info_lookup(record_info.mac, record_info.transaction_id, &record_info)) {
                return;
            }

            while (op < max) {
                if (*op == DHCP_REQUESTED_ADDRESS) {
                    memcpy(&bp_client_addr, &op[2], 4);
                    record_info.assigned_ip = ntohl(bp_client_addr);
                    T_D("[assigned_ip]");
                    DHCP_HELPER_frame_info_add(&record_info);
                } else if (*op == DHCP_LEASE_TIME) {
                    memcpy(&lease_time, &op[2], 4);
                    record_info.lease_time = ntohl(lease_time);
                    T_D("[lease_time]");
                    DHCP_HELPER_frame_info_add(&record_info);
                    if (dhcp_message == DHCP_HELPER_MSG_TYPE_ACK) {
                        get_completed_info = 1;
                    }
                } else if (*op == 0x0) {
                    if (sp != op) {
                        *sp = *op;
                    }
                    ++op;
                    ++sp;
                    T_D("[0x0 continue]");
                    continue;
                } else if (*op == 0xFF) {  /* Quit immediately if we hit an End option. */
                    if (sp != op) {
                        *sp++ = *op++;
                    }
                    T_D("[0xff break]");
                    break;
                }

                nextop = op + op[1] + 2;
                if (nextop > max) {
                    T_D("[return]");
                    return;
                }
                if (sp != op) {
                    memmove(sp, op, op[1] + 2);
                    sp += op[1] + 2;
                    op = nextop;
                } else {
                    op = sp = nextop;
                }
                continue;
            }
        }
    }

    /* Notice snooping module the completed information */
    if (get_completed_info && DHCP_HELPER_dhcp_snooping_cb && DHCP_HELPER_frame_info_cb) {
        T_D("[notice snooping]");
        DHCP_HELPER_frame_info_cb(&record_info);
    }
    /* Notice snooping module the client release dynamic IP */
    if (dhcp_message == DHCP_HELPER_MSG_TYPE_RELEASE && DHCP_HELPER_dhcp_snooping_cb && DHCP_HELPER_release_frame_cb) {
        DHCP_HELPER_release_frame_cb(&record_info);
    }

    if (!DHCP_HELPER_dhcp_server_cb && DHCP_HELPER_dhcp_relay_cb && !DHCP_HELPER_MSG_FROM_SERVER(dhcp_message) && memcmp(broadcast_mac, ptr, 6)) {
        /* If the packet is unicast and not given to system, the relay socket won't receive the packet.
           we need modify DIP to broadcast IP that relay socket can process it. */
        struct udphdr *udp_hdr = (struct udphdr *)(ptr + 14 + ip_header_len);

        ip->ip_dst.s_addr = 0xFFFFFFFF; //IP broadcast
        ip->ip_sum = 0; //clear before do checksum
        ip->ip_sum = DHCP_HELPER_do_ip_chksum(ip_header_len, (u16 *)&ptr[14]);
        udp_hdr->uh_sum = 0; //clean UDP checksum, that it ignore checksum checking
    }

    if (DHCP_HELPER_dhcp_server_cb) {
        DHCP_HELPER_dhcp_server_cb(packet, len, vid, isid, port_no, glag_no);
    } else if (DHCP_HELPER_dhcp_relay_cb) {
        /* When received the broadcast DHCP packet form server, discard the packet
           (broadcast should not receive from different domain). */
        if (DHCP_HELPER_MSG_FROM_SERVER(dhcp_message) && !memcmp(broadcast_mac, ptr, 6)) {
            return;
        }
        DHCP_HELPER_dhcp_relay_cb(packet, len, vid, isid, port_no, glag_no);
    } else if (DHCP_HELPER_dhcp_snooping_cb) {
        DHCP_HELPER_dhcp_snooping_cb(packet, len, vid, isid, port_no, glag_no);
    }

    T_D("exit");
}


/****************************************************************************/
/*  Reserved ACEs functions                                                 */
/****************************************************************************/
/* Add reserved ACE */
static void DHCP_HELPER_ace_add(void)
{
    acl_entry_conf_t conf;

    /* Set two ACEs (bootps and bootpc) on all switches (master and slaves). */
    //bootps
    if (acl_mgmt_ace_init(VTSS_ACE_TYPE_IPV4, &conf) != VTSS_OK) {
        return;
    }
    conf.id = DHCP_HELPER_BOOTPS_ACE_ID;
#if defined(VTSS_FEATURE_ACL_V2)
    conf.action.port_action = VTSS_ACL_PORT_ACTION_FILTER;
    memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
#else
    conf.action.permit = FALSE;
#endif /* VTSS_FEATURE_ACL_V2 */
    conf.action.force_cpu = TRUE;
    conf.action.cpu_once = FALSE;
    conf.isid = VTSS_ISID_LOCAL;
    VTSS_BF_SET(conf.flags.mask, ACE_FLAG_IP_FRAGMENT, 1);
    VTSS_BF_SET(conf.flags.value, ACE_FLAG_IP_FRAGMENT, 0);
    conf.frame.ipv4.proto.value = 17; //UDP
    conf.frame.ipv4.proto.mask = 0xFF;
    conf.frame.ipv4.sport.in_range = conf.frame.ipv4.dport.in_range = TRUE;
    conf.frame.ipv4.sport.high = 65535;
    conf.frame.ipv4.dport.low = conf.frame.ipv4.dport.high = DHCP_SERVER_UDP_PORT;
    if (acl_mgmt_ace_add(ACL_USER_DHCP, ACE_ID_NONE, &conf) != VTSS_OK) {
        T_D("Delete DHCP helper reserved ACE (BOOTPS) fail.\n");
    }

    //bootpc
    conf.id = DHCP_HELPER_BOOTPC_ACE_ID;
    conf.frame.ipv4.dport.low = conf.frame.ipv4.dport.high = DHCP_CLINET_UDP_PORT;
    if (acl_mgmt_ace_add(ACL_USER_DHCP, ACE_ID_NONE, &conf) != VTSS_OK) {
        T_D("Delete DHCP helper reserved ACE (BOOTPC) fail.\n");
    }
}

/* Delete reserved ACE */
static void DHCP_HELPER_ace_del(void)
{
    if (acl_mgmt_ace_del(ACL_USER_DHCP, DHCP_HELPER_BOOTPS_ACE_ID) != VTSS_OK) {
        T_D("Delete DHCP helper reserved ACE (BOOTPS) fail.\n");
    }
    if (acl_mgmt_ace_del(ACL_USER_DHCP, DHCP_HELPER_BOOTPC_ACE_ID) != VTSS_OK) {
        T_D("Delete DHCP helper reserved ACE (BOOTPC) fail.\n");
    }
}


/****************************************************************************/
/*  Message functions                                                       */
/****************************************************************************/

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
static char *DHCP_HELPER_msg_id_txt(dhcp_helper_msg_id_t msg_id)
{
    char *txt;

    switch (msg_id) {
    case DHCP_HELPER_MSG_ID_FRAME_RX_IND:
        txt = "DHCP_HELPER_MSG_ID_FRAME_RX_IND";
        break;
    case DHCP_HELPER_MSG_ID_FRAME_TX_REQ:
        txt = "DHCP_HELPER_MSG_ID_FRAME_TX_REQ";
        break;
    case DHCP_HELPER_MSG_ID_LOCAL_ACE_SET:
        txt = "DHCP_HELPER_MSG_ID_LOCAL_ACE_SET";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}
#endif /* VTSS_TRACE_LVL_DEBUG */

/* Allocate request buffer */
static dhcp_helper_msg_t *DHCP_HELPER_msg_req_alloc(dhcp_helper_msg_buf_t *buf, dhcp_helper_msg_id_t msg_id)
{
    dhcp_helper_msg_t *msg = &DHCP_HELPER_global.request.msg;

    buf->sem = &DHCP_HELPER_global.request.sem;
    buf->msg = msg;
    (void) VTSS_OS_SEM_WAIT(buf->sem);
    msg->msg_id = msg_id;
    return msg;
}

/* Free request/reply buffer */
static void DHCP_HELPER_msg_free(vtss_os_sem_t *sem)
{
    VTSS_OS_SEM_POST(sem);
}

static void DHCP_HELPER_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    dhcp_helper_msg_id_t msg_id = *(dhcp_helper_msg_id_t *)msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s", msg_id, DHCP_HELPER_msg_id_txt(msg_id));
    DHCP_HELPER_msg_free(contxt);
}

static void DHCP_HELPER_msg_tx(dhcp_helper_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    dhcp_helper_msg_id_t msg_id = *(dhcp_helper_msg_id_t *)buf->msg;
#endif /* VTSS_TRACE_LVL_DEBUG */

    T_D("msg_id: %d, %s, len: %zd, isid: %d", msg_id, DHCP_HELPER_msg_id_txt(msg_id), len, isid);
    msg_tx_adv(buf->sem, DHCP_HELPER_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_MODULE_ID_DHCP_HELPER, isid, buf->msg, len + MSG_TX_DATA_HDR_LEN(dhcp_helper_msg_t, data));
}

static BOOL DHCP_HELPER_msg_rx(void *contxt,
                               const void *rx_msg,
                               size_t len,
                               vtss_module_id_t modid,
                               u32 isid)
{
    const dhcp_helper_msg_t *msg = (dhcp_helper_msg_t *)rx_msg;

    T_D("Sid %u, rx %zd bytes, msg %d", isid, len, msg->msg_id);

    switch (msg->msg_id) {
    case DHCP_HELPER_MSG_ID_FRAME_RX_IND: {
        DHCP_HELPER_do_rx_callback(&msg[1], msg->data.rx_ind.len, msg->data.rx_ind.vid, isid, msg->data.rx_ind.port_no, msg->data.rx_ind.glag_no);
        break;
    }
    case DHCP_HELPER_MSG_ID_FRAME_TX_REQ: {
        if (!msg_switch_is_master()) {
            void *frame = packet_tx_alloc(msg->data.tx_req.len);
            if (frame) {
                vtss_rc             rc;
                packet_tx_props_t   tx_props;

                memcpy(frame, &msg[1], msg->data.tx_req.len);
                packet_tx_props_init(&tx_props);
                tx_props.tx_info.tag.vid             = msg->data.tx_req.vid;
                tx_props.packet_info.filter.enable   = TRUE;
                tx_props.packet_info.filter.src_port = msg->data.tx_req.src_port_no; /* Not needed if you know that dst != src */
#if defined(VTSS_FEATURE_AGGR_GLAG)
                tx_props.packet_info.filter.glag_no  = msg->data.tx_req.src_glag_no; /* Avoid transmitting on ingress GLAG */
#endif /* VTSS_FEATURE_AGGR_GLAG */
                tx_props.packet_info.modid           = VTSS_MODULE_ID_DHCP_HELPER;
                tx_props.packet_info.frm[0]          = frame;
                tx_props.packet_info.len[0]          = msg->data.tx_req.len;
                tx_props.tx_info.dst_port_mask       = VTSS_BIT64(msg->data.tx_req.port_no);
                if ((rc = packet_tx(&tx_props)) == VTSS_RC_ERROR) {
                    T_E("Frame transmit on port %d failed", msg->data.tx_req.port_no);
                } else if (rc == VTSS_RC_INV_STATE) {
                    T_D("Frame was filtered on port %d", msg->data.tx_req.port_no);
                }
            }
        }
        break;
    }
    case DHCP_HELPER_MSG_ID_LOCAL_ACE_SET: {
        if (msg->data.local_ace_set.add) {
            DHCP_HELPER_rx_filter_register(TRUE);
            DHCP_HELPER_ace_add();
        } else {
            DHCP_HELPER_ace_del();
            DHCP_HELPER_rx_filter_register(FALSE);
            DHCP_HELPER_frame_info_clear();
        }
        break;
    }
    default:
        T_W("unknown message ID: %d", msg->msg_id);
        break;
    }
    return TRUE;
}

/* Stack Register */
static vtss_rc DHCP_HELPER_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = DHCP_HELPER_msg_rx;
    filter.modid = VTSS_MODULE_ID_DHCP_HELPER;
    return msg_rx_filter_register(&filter);
}

/* Set stack DHCP helper local ACEs */
static void DHCP_HELPER_stack_ace_conf_set(vtss_isid_t isid_add)
{
    dhcp_helper_msg_t       *msg;
    dhcp_helper_msg_buf_t   buf;
    vtss_isid_t             isid;

    T_D("enter, isid_add: %d", isid_add);
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if ((isid_add != VTSS_ISID_GLOBAL && isid_add != isid) ||
            !msg_switch_exists(isid)) {
            continue;
        }
        DHCP_HELPER_CRIT_ENTER();
        msg = DHCP_HELPER_msg_req_alloc(&buf, DHCP_HELPER_MSG_ID_LOCAL_ACE_SET);
        msg->data.local_ace_set.add = ((DHCP_HELPER_dhcp_server_cb == NULL && DHCP_HELPER_dhcp_snooping_cb == NULL && DHCP_HELPER_dhcp_relay_cb == NULL) ? FALSE : TRUE);
        DHCP_HELPER_CRIT_EXIT();
        DHCP_HELPER_msg_tx(&buf, isid, sizeof(msg->data.local_ace_set));
    }

    T_D("exit, isid_add: %d", isid_add);
}


/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* DHCP helper error text */
char *dhcp_helper_error_txt(vtss_rc rc)
{
    switch (rc) {
    case DHCP_HELPER_ERROR_MUST_BE_MASTER:
        return "Operation only valid on master switch";

    case DHCP_HELPER_ERROR_ISID:
        return "Invalid Switch ID";

    case DHCP_HELPER_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    default:
        return "DHCP Helper: Unknown error code";
    }
}

/* Get DHCP helper port configuration */
vtss_rc dhcp_helper_mgmt_port_conf_get(vtss_isid_t isid, dhcp_helper_port_conf_t *switch_cfg)
{
    T_D("enter");

    if (switch_cfg == NULL) {
        T_W("not master");
        T_D("exit");
        return DHCP_HELPER_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_master()) {
        T_W("not master");
        T_D("exit");
        return DHCP_HELPER_ERROR_MUST_BE_MASTER;
    }
    if (!msg_switch_exists(isid)) {
        T_W("isid: %d not exist", isid);
        T_D("exit");
        return DHCP_HELPER_ERROR_ISID;
    }

    DHCP_HELPER_CRIT_ENTER();
    *switch_cfg = DHCP_HELPER_global.port_conf[isid];
    DHCP_HELPER_CRIT_EXIT();

    T_D("exit");
    return VTSS_OK;
}

/* Set DHCP helper port configuration */
vtss_rc dhcp_helper_mgmt_port_conf_set(vtss_isid_t isid, dhcp_helper_port_conf_t *switch_cfg)
{
    vtss_rc         rc = VTSS_OK;
    vtss_port_no_t  port_idx;

    T_D("enter");

    if (switch_cfg == NULL) {
        T_W("not master");
        T_D("exit");
        return DHCP_HELPER_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_master()) {
        T_W("not master");
        T_D("exit");
        return DHCP_HELPER_ERROR_MUST_BE_MASTER;
    }
    if (!msg_switch_exists(isid)) {
        T_W("isid: %d not exist", isid);
        T_D("exit");
        return DHCP_HELPER_ERROR_ISID;
    }

    DHCP_HELPER_CRIT_ENTER();
    DHCP_HELPER_global.port_conf[isid] = *switch_cfg;

    /* Reset stack */
    for (port_idx = VTSS_PORT_NO_START; port_idx < VTSS_PORT_NO_END; port_idx++) {
        if (port_isid_port_no_is_stack(isid, port_idx)) {
            switch_cfg->port_mode[port_idx] = DHCP_HELPER_PORT_MODE_UNTRUSTED;
        }
    }
    DHCP_HELPER_CRIT_EXIT();

    T_D("exit");

    return rc;
}


/****************************************************************************
 * Module thread
 ****************************************************************************/
/* Clear the uncompleted entries in period time */
static void DHCP_HELPER_thread(cyg_addrword_t data)
{
    while (1) {
        if (msg_switch_is_master()) {
            while (msg_switch_is_master()) {
                VTSS_OS_MSLEEP(30000);
                DHCP_HELPER_frame_info_clear_obsoleted();
            } // while(msg_switch_is_master())
        } // if(msg_switch_is_master())

        // No reason for using CPU ressources when we're a slave
        T_D("Suspending DHCP helper thread");
        cyg_thread_suspend(DHCP_HELPER_thread_handle);
        T_D("Resumed DHCP helper thread");
    } // while(1)
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Module start */
static void DHCP_HELPER_start(BOOL init)
{
    vtss_rc rc;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize DHCP helper port configuration */
        memset(DHCP_HELPER_global.port_conf, 0x0, sizeof(DHCP_HELPER_global.port_conf));
        memset(DHCP_HELPER_global.stats, 0x0, sizeof(DHCP_HELPER_global.stats));

        /* Initialize message buffers */
        VTSS_OS_SEM_CREATE(&DHCP_HELPER_global.request.sem, 1);

        /* Create semaphore for critical regions */
        critd_init(&DHCP_HELPER_global.crit, "DHCP_HELPER_global.crit", VTSS_MODULE_ID_DHCP_HELPER, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        DHCP_HELPER_CRIT_EXIT();

        /* Create DHCP helper thread */
        cyg_thread_create(THREAD_DEFAULT_PRIO,
                          DHCP_HELPER_thread,
                          0,
                          "DHCP Helper",
                          DHCP_HELPER_thread_stack,
                          sizeof(DHCP_HELPER_thread_stack),
                          &DHCP_HELPER_thread_handle,
                          &DHCP_HELPER_thread_block);
    } else {
        /* Register for stack messages */
        if ((rc = DHCP_HELPER_stack_register()) != VTSS_OK) {
            T_W("VOICE_VLAN_stack_register(): failed rc = %d", rc);
        }
    }
    T_D("exit");
}

/* Initialize module */
vtss_rc dhcp_helper_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    if (data->cmd == INIT_CMD_INIT) {
        /* Initialize and register trace ressources */
        VTSS_TRACE_REG_INIT(&DHCP_HELPER_trace_reg, DHCP_HELPER_trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&DHCP_HELPER_trace_reg);
    }

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        DHCP_HELPER_start(1);
        break;
    case INIT_CMD_START:
        /* Register for stack messages */
        DHCP_HELPER_start(0);
        break;
    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            DHCP_HELPER_CRIT_ENTER();
            memset(DHCP_HELPER_global.stats, 0x0, sizeof(DHCP_HELPER_global.stats));
            DHCP_HELPER_CRIT_EXIT();
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }
        break;
    case INIT_CMD_MASTER_UP:
        T_D("MASTER_UP");
        /* Starting DHCP relay thread (became master) */
        cyg_thread_resume(DHCP_HELPER_thread_handle);
        DHCP_HELPER_CRIT_ENTER();
        memset(DHCP_HELPER_global.stats, 0x0, sizeof(DHCP_HELPER_global.stats));
        DHCP_HELPER_CRIT_EXIT();
        break;
    case INIT_CMD_MASTER_DOWN:
        T_D("MASTER_DOWN");
        break;
    case INIT_CMD_SWITCH_ADD:
        T_D("SWITCH_ADD, isid: %d", isid);
        DHCP_HELPER_stack_ace_conf_set(isid);
        T_D("SWITCH_ADD, isid: %d", isid);
        break;
    case INIT_CMD_SWITCH_DEL:
        T_D("SWITCH_DEL, isid: %d", isid);
        break;
    default:
        break;
    }

    T_D("exit");

    return VTSS_OK;
}


/****************************************************************************/
/*  DHCP helper receive functions                                           */
/****************************************************************************/

// Avoid "Custodual pointer 'msg' has not been freed or returned, since
// the msg is freed by the message module.
/*lint -e{429} */
static void DHCP_HELPER_receive_indication(const void *packet,
                                           size_t len,
                                           vid_t vid,
                                           vtss_port_no_t switchport,
                                           vtss_glag_no_t glag_no)
{
    T_D("len %zd port %u vid %d glag %u", len, switchport, vid, glag_no);

    size_t msg_len = sizeof(dhcp_helper_msg_t) + len;
    dhcp_helper_msg_t *msg = malloc(msg_len);
    if (msg) {
        msg->msg_id = DHCP_HELPER_MSG_ID_FRAME_RX_IND;
        msg->data.rx_ind.len = len;
        msg->data.rx_ind.vid = vid;
        msg->data.rx_ind.port_no = switchport;
        msg->data.rx_ind.glag_no = glag_no;
        memcpy(&msg[1], packet, len); /* Copy frame */
        // These frames are subject to shaping.
        msg_tx_adv(NULL, NULL, MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK | MSG_TX_OPT_SHAPE, VTSS_MODULE_ID_DHCP_HELPER, 0, msg, msg_len);
    } else {
        T_W("Unable to allocate %zd bytes, tossing frame on port %u", msg_len, switchport);
    }
}


/****************************************************************************/
/*  Rx filter register functions                                            */
/****************************************************************************/

/* Local port packet receive indication - forward through DHCP helper */
static BOOL DHCP_HELPER_rx_packet_callback(void *contxt, const u8 *const frm, const vtss_packet_rx_info_t *const rx_info)
{
    u8              *ptr = (u8 *)(frm);
    struct ip       *ip = (struct ip *)(ptr + 14);
    int             ip_header_len = IP_VHL_HL(ip->ip_hl) << 2;
    struct udphdr   *udp_header = (struct udphdr *)(ptr + 14 + ip_header_len); /* 14:DA+SA+ETYPE */
    u16             sport = ntohs(udp_header->uh_sport);
    u16             dport = ntohs(udp_header->uh_dport);
    BOOL            rc = FALSE;

    T_D("enter, port_no: %u len %d vid %d glag %u", rx_info->port_no, rx_info->length, rx_info->tag.vid, rx_info->glag_no);

    if (sport == DHCP_SERVER_UDP_PORT || sport == DHCP_CLINET_UDP_PORT || dport == DHCP_SERVER_UDP_PORT || dport == DHCP_CLINET_UDP_PORT) {
        DHCP_HELPER_receive_indication(frm, rx_info->length, rx_info->tag.vid, rx_info->port_no, rx_info->glag_no);
        rc = TRUE; // Do not allow other subscribers to receive the packet
    }

    T_D("exit");

    return rc;
}

static void DHCP_HELPER_rx_filter_register(BOOL registerd)
{
    DHCP_HELPER_CRIT_ENTER();

    if (!DHCP_HELPER_filter_id) {
        memset(&DHCP_HELPER_rx_filter, 0, sizeof(DHCP_HELPER_rx_filter));
    }

    DHCP_HELPER_rx_filter.modid           = VTSS_MODULE_ID_DHCP_HELPER;
    DHCP_HELPER_rx_filter.match           = PACKET_RX_FILTER_MATCH_ACL | PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_IPV4_PROTO;
    DHCP_HELPER_rx_filter.etype           = 0x0800; // IP
    DHCP_HELPER_rx_filter.ipv4_proto      = 17; //UDP
    DHCP_HELPER_rx_filter.prio            = PACKET_RX_FILTER_PRIO_NORMAL;
    DHCP_HELPER_rx_filter.cb              = DHCP_HELPER_rx_packet_callback;

    if (registerd && !DHCP_HELPER_filter_id) {
        if (packet_rx_filter_register(&DHCP_HELPER_rx_filter, &DHCP_HELPER_filter_id) != VTSS_OK) {
            T_W("DHCP helper module register packet RX filter fail./n");
        }
    } else if (!registerd && DHCP_HELPER_filter_id) {
        if (packet_rx_filter_unregister(DHCP_HELPER_filter_id) == VTSS_OK) {
            DHCP_HELPER_filter_id = NULL;
        }
    }

    DHCP_HELPER_CRIT_EXIT();
}


/****************************************************************************/
/*  Receive register functions                                              */
/****************************************************************************/

/* Register DHCP server receive */
#ifdef VTSS_SW_OPTION_DHCP_SERVER
void dhcp_helper_server_receive_register(dhcp_helper_stack_rx_callback_t cb)
{
    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_dhcp_server_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }
    DHCP_HELPER_dhcp_server_cb = cb;
    DHCP_HELPER_CRIT_EXIT();

    DHCP_HELPER_stack_ace_conf_set(VTSS_ISID_GLOBAL);
}

/* Unregister DHCP server receive */
void dhcp_helper_server_receive_unregister(void)
{
    DHCP_HELPER_CRIT_ENTER();
    if (!DHCP_HELPER_dhcp_server_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }
    DHCP_HELPER_dhcp_server_cb = NULL;
    DHCP_HELPER_CRIT_EXIT();

    DHCP_HELPER_stack_ace_conf_set(VTSS_ISID_GLOBAL);
}
#endif /* VTSS_SW_OPTION_DHCP_SERVER */

#ifdef VTSS_SW_OPTION_DHCP_RELAY
/* Register DHCP relay receive */
void dhcp_helper_relay_receive_register(dhcp_helper_stack_rx_callback_t cb)
{
    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_dhcp_relay_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }
    DHCP_HELPER_dhcp_relay_cb = cb;
    DHCP_HELPER_CRIT_EXIT();

    DHCP_HELPER_stack_ace_conf_set(VTSS_ISID_GLOBAL);
}

/* Unregister DHCP relay receive */
void dhcp_helper_relay_receive_unregister(void)
{
    DHCP_HELPER_CRIT_ENTER();
    if (!DHCP_HELPER_dhcp_relay_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }
    DHCP_HELPER_dhcp_relay_cb = NULL;
    DHCP_HELPER_CRIT_EXIT();

    DHCP_HELPER_stack_ace_conf_set(VTSS_ISID_GLOBAL);
}
#endif /* VTSS_SW_OPTION_DHCP_RELAY */

#ifdef VTSS_SW_OPTION_DHCP_SNOOPING
/* Register DHCP snooping receive */
void dhcp_helper_snooping_receive_register(dhcp_helper_stack_rx_callback_t cb)
{
    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_dhcp_snooping_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }
    DHCP_HELPER_dhcp_snooping_cb = cb;
    DHCP_HELPER_CRIT_EXIT();

    DHCP_HELPER_stack_ace_conf_set(VTSS_ISID_GLOBAL);
}

/* Unregister DHCP snooping receive */
void dhcp_helper_snooping_receive_unregister(void)
{
    DHCP_HELPER_CRIT_ENTER();
    if (!DHCP_HELPER_dhcp_snooping_cb) {
        DHCP_HELPER_CRIT_EXIT();
        return;
    }
    DHCP_HELPER_dhcp_snooping_cb = NULL;
    DHCP_HELPER_CRIT_EXIT();

    DHCP_HELPER_stack_ace_conf_set(VTSS_ISID_GLOBAL);
}
#endif /* VTSS_SW_OPTION_DHCP_SNOOPING */

/****************************************************************************/
/*  DHCP helper transmit functions                                          */
/****************************************************************************/

/* Alloc memory for transmit DHCP frame */
void *dhcp_helper_alloc_xmit(size_t len,
                             vtss_vid_t vid,
                             vtss_isid_t isid,
                             vtss_port_no_t port_no,
                             vtss_port_no_t src_port_no,
                             vtss_glag_no_t src_glag_no,
                             void **pbufref)
{
    void *p = NULL;

    if (msg_switch_is_local(isid)) {
        p = packet_tx_alloc(len);
        *pbufref = NULL;    /* Local operation */
    } else {                /* Remote */
        dhcp_helper_msg_t *msg = malloc(sizeof(dhcp_helper_msg_t) + len);
        if (msg) {
            msg->msg_id = DHCP_HELPER_MSG_ID_FRAME_TX_REQ;
            msg->data.tx_req.len = len;
            msg->data.tx_req.vid = vid;
            msg->data.tx_req.isid = isid;
            msg->data.tx_req.port_no = port_no;
            msg->data.tx_req.src_port_no = VTSS_PORT_NO_NONE;
            msg->data.tx_req.src_glag_no = VTSS_GLAG_NO_NONE;
            *pbufref = (void *) msg; /* Remote op */
            p = ((u8 *) msg) + sizeof(*msg);
        } else {
            T_E("Allocation failure, TX port %d length %zd", port_no, len);
        }
    }

    T_D("%s(%zd) ret %p", __FUNCTION__, len, p);

    return p;
}

/* Transmit DHCP frame
   Return 0  : Success
   Return -1 : Fail */
int dhcp_helper_xmit(void *frame,
                     size_t len,
                     vtss_vid_t vid,
                     vtss_isid_t isid,
                     vtss_port_no_t port_no,
                     vtss_port_no_t src_port_no,
                     vtss_glag_no_t src_glag_no,
                     void *bufref,
                     int is_relay)
{
    dhcp_helper_msg_t   *msg = (dhcp_helper_msg_t *)bufref;
    u8                  *ptr = (u8 *)(frame);
    struct ip           *ip = (struct ip *)(ptr + 14);
    int                 ip_header_len = IP_VHL_HL(ip->ip_hl) << 2;
    struct bootp        bp;
    u8                  dhcp_message;

    T_D("%s(%p, %zd, %d, %d, %d, %d, %d)", __FUNCTION__, frame, len, isid, vid, port_no, src_port_no, src_glag_no);

    if (!frame) {
        T_W("TX frame point is NULL");
        return -1;
    }

    memcpy(&bp, ptr + 14 + ip_header_len + 8, sizeof(bp)); /* 14:DA+SA+ETYPE, 8:udp header length */
    dhcp_message = bp.bp_vend[6];

    /* Only forward client's packet to trust port */
    DHCP_HELPER_CRIT_ENTER();
    if (DHCP_HELPER_dhcp_snooping_cb && DHCP_HELPER_global.port_conf[isid].port_mode[port_no] == DHCP_HELPER_PORT_MODE_UNTRUSTED && !DHCP_HELPER_MSG_FROM_SERVER(dhcp_message)) {
        DHCP_HELPER_CRIT_EXIT();
        return -1;
    }
    DHCP_HELPER_CRIT_EXIT();

    if (msg) {
        if (msg_switch_is_local(msg->data.tx_req.isid)) {
            T_E("ISID became local (%d)?", msg->data.tx_req.isid);
            free(msg);
            return -1;
        } else {
            msg_tx(VTSS_MODULE_ID_DHCP_HELPER,
                   msg->data.tx_req.isid, msg, len + sizeof(*msg));
        }
    } else {
        vtss_rc             rc;
        packet_tx_props_t   tx_props;

        packet_tx_props_init(&tx_props);

        tx_props.tx_info.tag.vid             = vid;
        /* Enable tx_props.filter will insert the correct VLAN tagged information if needed. */
        tx_props.packet_info.filter.enable   = TRUE;
        tx_props.packet_info.filter.src_port = src_port_no; /* Not needed if you know that dst != src */
#if defined(VTSS_FEATURE_AGGR_GLAG)
        tx_props.packet_info.filter.glag_no  = src_glag_no; /* Avoid transmitting on ingress GLAG */
#endif /* VTSS_FEATURE_AGGR_GLAG */
        tx_props.packet_info.modid           = VTSS_MODULE_ID_DHCP_HELPER;
        tx_props.packet_info.frm[0]          = frame;
        tx_props.packet_info.len[0]          = len;
        tx_props.tx_info.dst_port_mask       = VTSS_BIT64(port_no);
        if ((rc = packet_tx(&tx_props)) == VTSS_RC_ERROR) {
            T_E("Frame transmit on port %d failed", port_no);
            return -1;
        } else if (rc == VTSS_RC_INV_STATE) {
            T_D("Frame was filtered on port %d", port_no);
            return 0;
        }
    }

    if (DHCP_HELPER_MSG_FROM_SERVER(dhcp_message)) {
        /* Clear releated record informations when relayed packets to client,
        cause we got the output sid/port now, don't need it anymore */
        dhcp_helper_frame_info_t record_info;
        memcpy(record_info.mac, bp.bp_chaddr, 6);
        record_info.transaction_id = bp.bp_xid;
        DHCP_HELPER_frame_info_del(&record_info);
    }

    DHCP_HELPER_CRIT_ENTER();
    if (!is_relay && DHCP_HELPER_dhcp_snooping_cb) {
        /* The statistics only counter packet under DHCP snooping mode is enabled
           and relay mode is disabled. And it doesn't count the DHCP packets for
           system DHCP client. */
        switch (dhcp_message) {
        case DHCP_HELPER_MSG_TYPE_DISCOVER:
            DHCP_HELPER_global.stats[isid][port_no].discover_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_OFFER:
            DHCP_HELPER_global.stats[isid][port_no].offer_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_REQUEST:
            DHCP_HELPER_global.stats[isid][port_no].request_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_DECLINE:
            DHCP_HELPER_global.stats[isid][port_no].decline_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_ACK:
            DHCP_HELPER_global.stats[isid][port_no].ack_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_NAK:
            DHCP_HELPER_global.stats[isid][port_no].nak_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_RELEASE:
            DHCP_HELPER_global.stats[isid][port_no].release_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_INFORM:
            DHCP_HELPER_global.stats[isid][port_no].inform_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_LEASEQUERY:
            DHCP_HELPER_global.stats[isid][port_no].leasequery_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_LEASEUNASSIGNED:
            DHCP_HELPER_global.stats[isid][port_no].leaseunassigned_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_LEASEUNKNOWN:
            DHCP_HELPER_global.stats[isid][port_no].leaseunknown_tx++;
            break;
        case DHCP_HELPER_MSG_TYPE_LEASEACTIVE:
            DHCP_HELPER_global.stats[isid][port_no].leaseactive_tx++;
            break;
        }
    }
    DHCP_HELPER_CRIT_EXIT();

    return 0;
}

/****************************************************************************/
/*  Statistics functions                                                    */
/****************************************************************************/

/* Get DHCP helper statistics */
int dhcp_helper_stats_get(vtss_isid_t isid, vtss_port_no_t port_no, dhcp_helper_stats_t *stats)
{
    T_D("enter, isid: %d, port_no: %u", isid, port_no);

    if (!msg_switch_is_master()) {
        T_W("not master");
        return -1;
    }

    if (!msg_switch_exists(isid)) {
        T_W("not exist");
        return -1;
    }

    if (port_no >= VTSS_PORT_NO_END) {
        T_W("illegal port_no: %u", port_no);
        return -1;
    }

    if (!msg_switch_exists(isid)) {
        return -1;
    }

    DHCP_HELPER_CRIT_ENTER();
    *stats = DHCP_HELPER_global.stats[isid][port_no];
    DHCP_HELPER_CRIT_EXIT();

    T_D("exit, isid: %d, port_no: %u", isid, port_no);
    return 0;
}

/* Clear DHCP helper statistics */
int dhcp_helper_stats_clear(vtss_isid_t isid, vtss_port_no_t port_no)
{
    T_D("enter, isid: %d, port_no: %u", isid, port_no);

    if (!msg_switch_is_master()) {
        T_W("not master");
        return -1;
    }

    if (!msg_switch_exists(isid)) {
        T_W("not exist");
        return -1;
    }

    if (port_no >= VTSS_PORT_NO_END) {
        T_W("illegal port_no: %u", port_no);
        return -1;
    }

    DHCP_HELPER_CRIT_ENTER();
    memset(&DHCP_HELPER_global.stats[isid][port_no], 0x0, sizeof(dhcp_helper_stats_t));
    DHCP_HELPER_CRIT_EXIT();

    T_D("exit, isid: %d, port_no: %u", isid, port_no);

    return 0;
}



/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
