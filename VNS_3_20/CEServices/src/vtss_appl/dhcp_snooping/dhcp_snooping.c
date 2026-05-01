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
#include "port_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "dhcp_snooping_api.h"
#include "dhcp_snooping.h"
#include "dhcp_helper_api.h"
#include "packet_api.h"
#include "vlan_api.h"
#include "mac_api.h"
#include "aggr_api.h"
#include <network.h>
#include <dhcp.h>
#ifdef VTSS_SW_OPTION_VCLI
#include "dhcp_snooping_cli.h"
#endif
#ifdef VTSS_SW_OPTION_ICFG
#include "dhcp_snooping_icfg.h"
#endif

#define DHCP_SNOOPING_MAC_VERI_SUPPORTED    0

#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
#include "psec_api.h"
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */

#ifndef IP_VHL_HL
#define IP_VHL_HL(vhl)      ((vhl) & 0x0f)
#endif /* IP_VHL_HL */


/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static dhcp_snooping_global_t DHCP_SNOOPING_global;

#if (VTSS_TRACE_ENABLED)
static vtss_trace_reg_t DHCP_SNOOPING_trace_reg = {
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "dhcps",
    .descr     = "DHCP Snooping"
};

static vtss_trace_grp_t DHCP_SNOOPING_trace_grps[TRACE_GRP_CNT] = {
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
#define DHCP_SNOOPING_CRIT_ENTER() critd_enter(&DHCP_SNOOPING_global.crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#define DHCP_SNOOPING_CRIT_EXIT()  critd_exit( &DHCP_SNOOPING_global.crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#else
#define DHCP_SNOOPING_CRIT_ENTER() critd_enter(&DHCP_SNOOPING_global.crit)
#define DHCP_SNOOPING_CRIT_EXIT()  critd_exit( &DHCP_SNOOPING_global.crit)
#endif /* VTSS_TRACE_ENABLED */

/* Thread variables */
#define DHCP_SNOOPING_CERT_THREAD_STACK_SIZE       8192
static cyg_handle_t DHCP_SNOOPING_thread_handle;
static cyg_thread   DHCP_SNOOPING_thread_block;
static char         DHCP_SNOOPING_thread_stack[DHCP_SNOOPING_CERT_THREAD_STACK_SIZE];

struct DHCP_SNOOPING_ip_assigned_info_list {
    struct DHCP_SNOOPING_ip_assigned_info_list  *next;
    dhcp_snooping_ip_assigned_info_t            info;
} *DHCP_SNOOPING_ip_assigned_info;


/******************************************************************************/
/*  IP Assignment callback function                                           */
/******************************************************************************/
#define DHCP_SNOOPING_IP_ASSIGNED_INFO_MAX_CALLBACK_NUM  2

typedef struct {
    int                                         active;
    dhcp_snooping_ip_assigned_info_callback_t   callback;
} dhcp_snooping_ip_assigned_info_callback_list_t;

static dhcp_snooping_ip_assigned_info_callback_list_t DHCP_SNOOPING_ip_assigned_info_callback_list[DHCP_SNOOPING_IP_ASSIGNED_INFO_MAX_CALLBACK_NUM];

/* Register IP assigned information */
void dhcp_snooping_ip_assigned_info_register(dhcp_snooping_ip_assigned_info_callback_t cb)
{
    uint idx, inactive_idx = ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list), found_inactive_flag = 0;

    if (!msg_switch_is_master()) {
        T_W("not master");
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    for (idx = 0; idx < ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list); idx++) {
        if (DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active && DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback == cb) {
            //found the same callback, do nothing and return
            DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active = 1;
            DHCP_SNOOPING_CRIT_EXIT();
            return;
        }
        if (found_inactive_flag == 0 && DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active == 0) {
            //get the first incative index
            inactive_idx = idx;
            found_inactive_flag = 1;
        }
    }

    if (inactive_idx == ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list)) { //callback list full
        DHCP_SNOOPING_CRIT_EXIT();
        return;
    }

    //active the callback to callback list[found_inactive_flag]
    DHCP_SNOOPING_ip_assigned_info_callback_list[inactive_idx].active = 1;
    DHCP_SNOOPING_ip_assigned_info_callback_list[inactive_idx].callback = cb;
    DHCP_SNOOPING_CRIT_EXIT();
}

/* Unregister IP assigned information */
void dhcp_snooping_ip_assigned_info_unregister(dhcp_snooping_ip_assigned_info_callback_t cb)
{
    uint idx;

    if (!msg_switch_is_master()) {
        T_W("not master");
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    for (idx = 0; idx < ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list); idx++) {
        if (DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active && DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback == cb) {
            DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active = 0;
            DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback = NULL;
            break;
        }
    }
    DHCP_SNOOPING_CRIT_EXIT();
}


/****************************************************************************/
/*  Volatile MAC add/delete functions                                       */
/****************************************************************************/
#if 0
/* Add volatile MAC entry */
static vtss_rc DHCP_SNOOPING_volatile_mac_add(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t vid, u8 *mac)
{
    mac_mgmt_addr_entry_t entry;
    vtss_rc               rc;

    memset(&entry, 0, sizeof(entry));
    memcpy(entry.vid_mac.mac.addr, mac, 6);
    entry.vid_mac.vid       = vid;
    entry.not_stack_ports   = 1; // Always
    entry.volatil           = 1; // Always
    entry.copy_to_cpu       = 0;

    entry.destination[port_no] = 1;
    rc = mac_mgmt_table_add(isid, &entry);

    return rc;
}

/* Delete volatile MAC entry */
static vtss_rc DHCP_SNOOPING_volatile_mac_del(vtss_isid_t isid, vtss_vid_t vid, u8 *mac)
{
    vtss_vid_mac_t  entry;
    vtss_rc         rc;

    entry.vid = vid;
    memcpy(entry.mac.addr, mac, 6);

    rc = mac_mgmt_table_del(isid, &entry, 1);

    return rc;
}
#endif

/****************************************************************************/
/*  IP assigned information entry control functions                         */
/****************************************************************************/
/* Add DHCP snooping IP assigned information entry */
static void DHCP_SNOOPING_ip_assigned_info_add(dhcp_snooping_ip_assigned_info_t *info)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp, *search_sp;

    sp = ((struct DHCP_SNOOPING_ip_assigned_info_list *) malloc(sizeof(*sp)));
    if (!sp) {
        T_W("Unable to allocate %zu bytes for DHCP frame information", sizeof(*sp));
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    /* search entry exist? */
    for (search_sp = DHCP_SNOOPING_ip_assigned_info; search_sp; search_sp = search_sp->next) {
        if (!memcmp(search_sp->info.mac, info->mac, 6)) {
            /* found it, update content */
            search_sp->info = *info;
            free(sp);
            DHCP_SNOOPING_CRIT_EXIT();
            return;
        }
    }

    sp->info = *info;
    sp->next = DHCP_SNOOPING_ip_assigned_info;
    DHCP_SNOOPING_ip_assigned_info = sp;
    DHCP_SNOOPING_CRIT_EXIT();
}

/* Delete DHCP snooping IP assigned information entry */
static BOOL DHCP_SNOOPING_ip_assigned_info_del(u8 *mac)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp, *prev_sp = NULL;

    DHCP_SNOOPING_CRIT_ENTER();
    for (sp = DHCP_SNOOPING_ip_assigned_info; sp; sp = sp->next) {
        if (!memcmp(sp->info.mac, mac, sizeof(sp->info.mac))) {
            if (prev_sp == NULL) {
                DHCP_SNOOPING_ip_assigned_info = sp->next;
            } else {
                prev_sp->next = sp->next;
            }
            free(sp);
            DHCP_SNOOPING_CRIT_EXIT();
            return TRUE;
        }
        prev_sp = sp;
    }
    DHCP_SNOOPING_CRIT_EXIT();
    return FALSE;
}

/* Lookup DHCP snooping IP assigned information entry */
static BOOL DHCP_SNOOPING_ip_assigned_info_lookup(u8 *mac, dhcp_snooping_ip_assigned_info_t *info)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;
    while (sp) {
        if (!memcmp(sp->info.mac, mac, 6)) {
            *info = sp->info;
            DHCP_SNOOPING_CRIT_EXIT();
            return TRUE;
        }
        sp = sp->next;
    }

    DHCP_SNOOPING_CRIT_EXIT();
    return FALSE;
}

/* Getnext DHCP snooping IP assigned information entry */
BOOL dhcp_snooping_ip_assigned_info_getnext(u8 *mac, dhcp_snooping_ip_assigned_info_t *info)
{
    u8 null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    DHCP_SNOOPING_CRIT_ENTER();
    if (!DHCP_SNOOPING_ip_assigned_info) {
        DHCP_SNOOPING_CRIT_EXIT();
        return FALSE;
    }

    if (!memcmp(null_mac, mac, 6)) {
        *info = DHCP_SNOOPING_ip_assigned_info->info;
        DHCP_SNOOPING_CRIT_EXIT();
        return TRUE;
    } else {
        struct DHCP_SNOOPING_ip_assigned_info_list  *sp;
        int                                         found = 0;

        sp = DHCP_SNOOPING_ip_assigned_info;
        while (sp) {
            if (!memcmp(sp->info.mac, mac, 6)) {
                found = 1;
                break;
            }
            sp = sp->next;
        }

        if (found && sp && sp->next) {
            *info = sp->next->info;
            DHCP_SNOOPING_CRIT_EXIT();
            return TRUE;
        } else {
            DHCP_SNOOPING_CRIT_EXIT();
            return FALSE;
        }
    }
}

/* Clear DHCP snooping IP assigned information entry */
static void DHCP_SNOOPING_ip_assigned_info_clear(void)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;
    while (sp) {
        DHCP_SNOOPING_ip_assigned_info = sp->next;
        free(sp);
        sp = DHCP_SNOOPING_ip_assigned_info;
    }
    DHCP_SNOOPING_CRIT_EXIT();
}

/* Clear DHCP snooping IP assigned information entry by port */
static void DHCP_SNOOPING_ip_assigned_info_clear_port(vtss_isid_t isid, vtss_port_no_t port_no)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp, *prev_sp = NULL;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;

    /* clear entries by mac */
    while (sp) {
        if (isid == sp->info.isid && port_no == sp->info.port_no) {
            if (prev_sp) {
                prev_sp->next = sp->next;
                free(sp);
                sp = prev_sp->next;
            } else {
                DHCP_SNOOPING_ip_assigned_info = sp->next;
                free(sp);
                sp = DHCP_SNOOPING_ip_assigned_info;
            }
            continue;
        }
        prev_sp = sp;
        sp = sp->next;
    }
    DHCP_SNOOPING_CRIT_EXIT();
}

/* Clear DHCP snooping IP assigned information entry by ISID */
static void DHCP_SNOOPING_ip_assigned_info_clear_isid(vtss_isid_t isid)
{
    struct DHCP_SNOOPING_ip_assigned_info_list *sp, *prev_sp = NULL;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;

    /* clear entries by mac */
    while (sp) {
        if (isid == sp->info.isid) {
            if (prev_sp) {
                prev_sp->next = sp->next;
                free(sp);
                sp = prev_sp->next;
            } else {
                DHCP_SNOOPING_ip_assigned_info = sp->next;
                free(sp);
                sp = DHCP_SNOOPING_ip_assigned_info;
            }
            continue;
        }
        prev_sp = sp;
        sp = sp->next;
    }
    DHCP_SNOOPING_CRIT_EXIT();
}

/* DHCP snooping IP assigned information callback */
void dhcp_snooping_ip_assigned_info_callback(dhcp_helper_frame_info_t *info, dhcp_snooping_info_reason_t reason)
{
    uint idx;

    DHCP_SNOOPING_CRIT_ENTER();
#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
    if (DHCP_SNOOPING_global.conf.snooping_mode == DHCP_SNOOPING_MGMT_ENABLED) {
        if (DHCP_SNOOPING_global.port_conf[info->isid].port_mode[info->port_no] == DHCP_SNOOPING_PORT_MODE_UNTRUSTED &&
            DHCP_SNOOPING_global.port_conf[info->isid].veri_mode[info->port_no] == DHCP_SNOOPING_MGMT_ENABLED) {
            if (reason == DHCP_SNOOPING_INFO_REASON_ASSIGN_COMPLETED) {
                vtss_vid_mac_t vid_mac;
                vid_mac.vid = info->vid;
                memcpy(vid_mac.mac.addr, info->mac, 6);
                if (psec_mgmt_mac_chg(PSEC_USER_DHCP_SNOOPING, info->isid, info->port_no, &vid_mac, PSEC_ADD_METHOD_FORWARD) != VTSS_OK) {
                    T_D("psec_mgmt_mac_chg()failed.");
                }
            }
        }
    }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */

    for (idx = 0; idx < ARRSZ(DHCP_SNOOPING_ip_assigned_info_callback_list); idx++) {
        if (DHCP_SNOOPING_ip_assigned_info_callback_list[idx].active && DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback) {
            DHCP_SNOOPING_ip_assigned_info_callback_list[idx].callback(info, reason);
        }
    }
    DHCP_SNOOPING_CRIT_EXIT();
}

/* Frame information completed */
void dhcp_snooping_frame_info_callback(dhcp_helper_frame_info_t *info)
{
    dhcp_snooping_ip_assigned_info_t lookup_info;

    if (info->vid > VLAN_TAG_MAX_VALID || !msg_switch_exists(info->isid) || !info->assigned_ip || !info->lease_time) {
        return;
    }
    if (DHCP_SNOOPING_ip_assigned_info_lookup(info->mac, &lookup_info)) {
        dhcp_snooping_ip_assigned_info_callback(&lookup_info, DHCP_SNOOPING_INFO_REASON_ENTRY_DUPLEXED);
    }
    DHCP_SNOOPING_ip_assigned_info_add(info);
    dhcp_snooping_ip_assigned_info_callback(info, DHCP_SNOOPING_INFO_REASON_ASSIGN_COMPLETED);
}

/* Frame information released */
void dhcp_snooping_release_frame_callback(dhcp_helper_frame_info_t *info)
{
    if (DHCP_SNOOPING_ip_assigned_info_del(info->mac)) {
        dhcp_snooping_ip_assigned_info_callback(info, DHCP_SNOOPING_INFO_REASON_RELEASE);
    }
}

static void DHCP_SNOOPING_state_change_callback(vtss_isid_t isid, vtss_port_no_t port_no, port_info_t *info)
{
    u8                                  mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    dhcp_snooping_ip_assigned_info_t    ip_assigned_info;

    if (msg_switch_is_master() && !info->stack) {
        T_D("port_no: [%d,%u] link %s", isid, port_no, info->link ? "up" : "down");
        if (!msg_switch_exists(isid)) {
            return;
        }
        if (!info->link) {
            while (dhcp_snooping_ip_assigned_info_getnext(mac, &ip_assigned_info)) {
                memcpy(mac, ip_assigned_info.mac, 6);
                if (ip_assigned_info.isid == isid && ip_assigned_info.port_no == port_no) {
                    dhcp_snooping_ip_assigned_info_callback(&ip_assigned_info, DHCP_SNOOPING_INFO_REASON_PORT_LINK_DOWN);
                }
            }
            DHCP_SNOOPING_ip_assigned_info_clear_port(isid, port_no);
        }
    }
}

/****************************************************************************/
/*  compare functions                                                       */
/****************************************************************************/

/* Get DHCP snooping defaults */
void dhcp_snooping_mgmt_conf_get_default(dhcp_snooping_conf_t *conf)
{
    conf->snooping_mode = DHCP_SNOOPING_MGMT_DISABLED;
}

/* Determine if DHCP snooping configuration has changed */
int dhcp_snooping_mgmt_conf_changed(dhcp_snooping_conf_t *old, dhcp_snooping_conf_t *new)
{
    return (memcmp(new, old, sizeof(*new)));
}

/* Get DHCP snooping port defaults */
void dhcp_snooping_mgmt_port_get_default(vtss_isid_t isid, dhcp_snooping_port_conf_t *conf)
{
    vtss_port_no_t  port_no;

    if (isid == VTSS_ISID_GLOBAL) {
        return;
    }

    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        conf->port_mode[port_no] = DHCP_SNOOPING_DEFAULT_PORT_MODE;
#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
        conf->veri_mode[port_no] = DHCP_SNOOPING_MGMT_ENABLED;
#else
        conf->veri_mode[port_no] = DHCP_SNOOPING_MGMT_DISABLED;
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */
    }
}

/* Determine if DHCP snooping port configuration has changed */
int dhcp_snooping_mgmt_port_conf_changed(dhcp_snooping_port_conf_t *old, dhcp_snooping_port_conf_t *new)
{
    return (memcmp(new, old, sizeof(*new)));
}

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/*  Master only. Receive packet from DHCP helper - send it to TCP/IP stack  */
static void DHCP_SNOOPING_stack_receive_callback(const void *packet, size_t len, vtss_vid_t vid, vtss_isid_t isid, vtss_port_no_t src_port_no, vtss_glag_no_t src_glag_no)
{
    dhcp_helper_frame_info_t    record_info;
    u8                          *ptr = (u8 *)(packet);
    struct ip                   *ip = (struct ip *)(ptr + 14);
    int                         ip_header_len = IP_VHL_HL(ip->ip_hl) << 2;
    struct bootp                bp;
    u8                          dhcp_message;
    switch_iter_t               sit;
    port_iter_t                 pit;
    dhcp_snooping_port_conf_t   conf;
    u8                          *pkt_buf;
    void                        *bufref;
    u8                          system_mac_addr[6];
    port_status_t               port_status;
    vlan_mgmt_entry_t           vlan_conf;
    aggr_mgmt_group_no_t        aggr_group_no;

    T_D("enter, RX port %d glag %d len %zd", src_port_no, src_glag_no, len);

    if (!msg_switch_is_master()) {
        return;
    }

    if (conf_mgmt_mac_addr_get(system_mac_addr, 0) < 0) {
        return;
    }

    memcpy(&bp, ptr + 14 + ip_header_len + 8, sizeof(bp)); /* 14:DA+SA+ETYPE, 8:udp header length */
    dhcp_message = bp.bp_vend[6];

    if (DHCP_HELPER_MSG_FROM_SERVER(dhcp_message)) {
        /* Only send reply DHCP message to correct client port */
        memcpy(record_info.mac, bp.bp_chaddr, 6);
        record_info.transaction_id = bp.bp_xid;
        if (!dhcp_helper_frame_info_lookup(record_info.mac, record_info.transaction_id, &record_info) ||
            (record_info.isid == isid && record_info.port_no == src_port_no)) { //filter source port
            return;
        }
        if (port_mgmt_status_get(record_info.isid, record_info.port_no, &port_status) == VTSS_OK && port_status.status.link) {
            if ((pkt_buf = dhcp_helper_alloc_xmit(len, record_info.vid, record_info.isid, record_info.port_no, VTSS_PORT_NO_NONE, VTSS_GLAG_NO_NONE, &bufref)) != NULL) {
                memcpy(pkt_buf, ptr, len);
                if (dhcp_helper_xmit(pkt_buf, len, record_info.vid, record_info.isid, record_info.port_no, VTSS_PORT_NO_NONE, VTSS_GLAG_NO_NONE, bufref, 0)) {
                    T_W("Calling dhcp_helper_xmit() failed.\n");
                }
            }
        }
        return;
    }

    aggr_group_no = aggr_mgmt_get_aggr_id(isid, src_port_no);
    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        if (!msg_switch_exists(sit.isid)) {
            continue;
        }

        if (vlan_mgmt_vlan_get(sit.isid, vid, &vlan_conf, FALSE, VLAN_USER_ALL, VLAN_MEMBERSHIP_OP) == VTSS_OK) {
            DHCP_SNOOPING_CRIT_ENTER();
            conf = DHCP_SNOOPING_global.port_conf[sit.isid];
            DHCP_SNOOPING_CRIT_EXIT();

            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (conf.port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED) {
                    if (vlan_conf.ports[pit.iport] == VLAN_NON_MEMBER_PORT ||
                        (isid == sit.isid && src_port_no == pit.iport)) { //filter source port
                        continue;
                    }

                    if (port_mgmt_status_get(sit.isid, pit.iport, &port_status) == VTSS_OK && port_status.status.link) {
                        if (aggr_group_no && (aggr_group_no == aggr_mgmt_get_aggr_id(sit.isid, pit.iport))) {   //filter same aggregation ID
                            continue;
                        }
                        if ((pkt_buf = dhcp_helper_alloc_xmit(len, vid, sit.isid, pit.iport, VTSS_PORT_NO_NONE, VTSS_GLAG_NO_NONE, &bufref)) != NULL) {
                            memcpy(pkt_buf, ptr, len);
                            if (dhcp_helper_xmit(pkt_buf, len, vid, sit.isid, pit.iport, VTSS_PORT_NO_NONE, VTSS_GLAG_NO_NONE, bufref, 0)) {
                                T_D("Calling dhcp_helper_xmit() failed.\n");
                            }
                        }
                    }
                }
            }
        }
    }

    T_D("exit");
}


/****************************************************************************/
// Port security callback functions
/****************************************************************************/

#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
/* Port security module loop through callback function
 * In all cases where this function is called back, we delete all existing. */
static psec_add_method_t DHCP_SNOOPING_psec_on_mac_loop_through_cb(void                       *user_ctx,
                                                                   vtss_isid_t                isid,
                                                                   vtss_port_no_t             port,
                                                                   vtss_vid_mac_t             *vid_mac,
                                                                   u32                        mac_cnt_before_callback,
                                                                   BOOL                       *keep,
                                                                   psec_loop_through_action_t *action)
{
    // Remove this entry.
    *keep = FALSE;
    // Return value doesn't matter when we remove it.
    return PSEC_ADD_METHOD_FORWARD;
}

/* Port security module MAC add callback function */
static psec_add_method_t DHCP_SNOOPING_psec_mac_add_cb(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_mac_t *vid_mac, u32 mac_cnt_before_callback, psec_add_action_t *action)
{
    psec_add_method_t                           rc = PSEC_ADD_METHOD_BLOCK;
    struct DHCP_SNOOPING_ip_assigned_info_list  *sp;

    DHCP_SNOOPING_CRIT_ENTER();
    sp = DHCP_SNOOPING_ip_assigned_info;
    while (sp) {
        if (sp->info.isid == isid && sp->info.port_no == port_no) {
            rc = PSEC_ADD_METHOD_FORWARD;
            break;
        }
        sp = sp->next;
    }
    DHCP_SNOOPING_CRIT_EXIT();

    return rc;
}

/* Proess DHCP snooping MAC verification */
static vtss_rc DHCP_SNOOPING_veri_process(vtss_isid_t isid, vtss_port_no_t port_no, u32 enable)
{
    vtss_rc rc = VTSS_OK;

    T_D("enter");

    if (!msg_switch_is_master()) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_MASTER;
    }
    if (!msg_switch_exists(isid)) {
        T_W("isid: %d not exist", isid);
        T_D("exit");
        return DHCP_SNOOPING_ERROR_ISID;
    }

    if (enable == DHCP_SNOOPING_MGMT_ENABLED) {
        /* Set untrust port to MAC sercure learning */
        rc = psec_mgmt_port_cfg_set(PSEC_USER_DHCP_SNOOPING, NULL, isid, port_no, TRUE,  FALSE, DHCP_SNOOPING_psec_on_mac_loop_through_cb, PSEC_PORT_MODE_NORMAL);
    } else {
        /* Revert port state */
        rc = psec_mgmt_port_cfg_set(PSEC_USER_DHCP_SNOOPING, NULL, isid, port_no, FALSE, FALSE, DHCP_SNOOPING_psec_on_mac_loop_through_cb, PSEC_PORT_MODE_NORMAL);
    }

    T_D("exit");
    return rc;
}

/* Start proess DHCP snooping MAC verification */
static vtss_rc DHCP_SNOOPING_veri_process_start(vtss_isid_t isid, BOOL is_restart)
{
    vtss_rc                     rc = VTSS_OK;
    port_iter_t                 pit;
    dhcp_snooping_conf_t        conf;
    u8                          port_mode, veri_mode;
    dhcp_snooping_port_conf_t   port_conf;

    T_D("enter, isid: %d, is_restart: %d", isid, is_restart);

    if (!msg_switch_is_master() || !msg_switch_exists(isid)) {
        T_D("exit, isid: %d", isid);
        return VTSS_OK;
    }

    if (is_restart) {
        /* Initialize DHCP snooping port configuration */
        dhcp_snooping_mgmt_port_get_default(isid, &port_conf);
        dhcp_snooping_mgmt_port_conf_set(isid, &port_conf);
    } else {
        DHCP_SNOOPING_CRIT_ENTER();
        conf = DHCP_SNOOPING_global.conf;

        /* Set DHCP snooping MAC verification configuration */
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            port_mode = DHCP_SNOOPING_global.port_conf[isid].port_mode[pit.iport];
            veri_mode = DHCP_SNOOPING_global.port_conf[isid].veri_mode[pit.iport];
            if (conf.snooping_mode == DHCP_SNOOPING_MGMT_ENABLED &&
                port_mode == DHCP_SNOOPING_PORT_MODE_UNTRUSTED &&
                veri_mode == DHCP_SNOOPING_MGMT_ENABLED) {
                rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_ENABLED);
            }
        }
        DHCP_SNOOPING_CRIT_EXIT();
    }

    T_D("exit, isid: %d", isid);
    return rc;
}
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */

static void DHCP_SNOOPING_conf_apply(void)
{
    vtss_rc                             rc;
    u8                                  mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    dhcp_snooping_ip_assigned_info_t    ip_assigned_info;
    u32                                 snooping_mode;
    vtss_isid_t                         isid;
    vtss_port_no_t                      port_no;
    dhcp_helper_port_conf_t             dhcp_helper_port_conf;

    if (!msg_switch_is_master()) {
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    snooping_mode = DHCP_SNOOPING_global.conf.snooping_mode;
    DHCP_SNOOPING_CRIT_EXIT();
    if (snooping_mode) {
        //Add ACEs again when topology change
        dhcp_helper_snooping_receive_register(DHCP_SNOOPING_stack_receive_callback);
    } else {
        dhcp_helper_snooping_receive_unregister();

        while (dhcp_snooping_ip_assigned_info_getnext(mac, &ip_assigned_info)) {
            memcpy(mac, ip_assigned_info.mac, 6);
            dhcp_snooping_ip_assigned_info_callback(&ip_assigned_info, DHCP_SNOOPING_INFO_REASON_MODE_DISABLED);
        }

        DHCP_SNOOPING_ip_assigned_info_clear();
    }

    /* If the snooping mode is disabled, set all ports to trusted mode. */
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        dhcp_helper_port_conf.port_mode[port_no] = DHCP_SNOOPING_PORT_MODE_TRUSTED;
    }

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (!msg_switch_exists(isid)) {
            continue;
        }

        if (snooping_mode) {
            DHCP_SNOOPING_CRIT_ENTER();
            if ((rc = dhcp_helper_mgmt_port_conf_set(isid, (dhcp_helper_port_conf_t *) &DHCP_SNOOPING_global.port_conf[isid])) != VTSS_OK) {
                T_W("Calling dhcp_helper_mgmt_port_conf_set(isid = %u): failed rc = %s", isid, dhcp_helper_error_txt(rc));
            }
            DHCP_SNOOPING_CRIT_EXIT();
        } else {
            if ((rc = dhcp_helper_mgmt_port_conf_set(isid, &dhcp_helper_port_conf)) != VTSS_OK) {
                T_W("Calling dhcp_helper_mgmt_port_conf_set(isid = %u): failed rc = %s", isid, dhcp_helper_error_txt(rc));
            }
        }
    }
}

static void DHCP_SNOOPING_port_conf_apply(vtss_isid_t isid)
{
    vtss_rc rc;

    if (!msg_switch_is_master() || !msg_switch_exists(isid)) {
        return;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    if (DHCP_SNOOPING_global.conf.snooping_mode) {
        if ((rc = dhcp_helper_mgmt_port_conf_set(isid, (dhcp_helper_port_conf_t *) &DHCP_SNOOPING_global.port_conf[isid])) != VTSS_OK) {
            T_W("Calling dhcp_helper_mgmt_port_conf_set(isid = %u): failed rc = %s", isid, dhcp_helper_error_txt(rc));
        }
    }
    DHCP_SNOOPING_CRIT_EXIT();
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* DHCP snooping error text */
char *dhcp_snooping_error_txt(vtss_rc rc)
{
    switch (rc) {
    case DHCP_SNOOPING_ERROR_MUST_BE_MASTER:
        return "Operation only valid on master switch";

    case DHCP_SNOOPING_ERROR_ISID:
        return "Invalid Switch ID";

    case DHCP_SNOOPING_ERROR_INV_PARAM:
        return "Invalid parameter supplied to function";

    default:
        return "DHCP Snooping: Unknown error code";
    }
}

/* Get DHCP snooping configuration */
vtss_rc dhcp_snooping_mgmt_conf_get(dhcp_snooping_conf_t *glbl_cfg)
{
    T_D("enter");

    if (glbl_cfg == NULL) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_master()) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_MASTER;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    *glbl_cfg = DHCP_SNOOPING_global.conf;
    DHCP_SNOOPING_CRIT_EXIT();

    T_D("exit");
    return VTSS_OK;
}

/* Set DHCP snooping configuration */
vtss_rc dhcp_snooping_mgmt_conf_set(dhcp_snooping_conf_t *glbl_cfg)
{
    vtss_rc                     rc      = VTSS_OK;
    int                         changed = 0;
    conf_blk_id_t               blk_id  = CONF_BLK_DHCP_SNOOPING_CONF;
    dhcp_snooping_conf_blk_t    *dhcp_snooping_conf_blk_p;
#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
    switch_iter_t               sit;
    port_iter_t                 pit;
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */

    T_D("enter, mode: %d", glbl_cfg->snooping_mode);

    if (glbl_cfg == NULL) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_master()) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_MASTER;
    }

    /* Check illegal parameter */
    if (glbl_cfg->snooping_mode != DHCP_SNOOPING_MGMT_ENABLED && glbl_cfg->snooping_mode != DHCP_SNOOPING_MGMT_DISABLED) {
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    changed = dhcp_snooping_mgmt_conf_changed(&DHCP_SNOOPING_global.conf, glbl_cfg);
    DHCP_SNOOPING_global.conf = *glbl_cfg;
    DHCP_SNOOPING_CRIT_EXIT();

    if (changed) {
        /* Save changed configuration */
        if ((dhcp_snooping_conf_blk_p = conf_sec_open(CONF_SEC_GLOBAL, blk_id, NULL)) == NULL) {
            T_W("failed to open DHCP snooping table");
        } else {
            dhcp_snooping_conf_blk_p->conf = *glbl_cfg;
            conf_sec_close(CONF_SEC_GLOBAL, blk_id);
        }

        /* Activate changed configuration */
        DHCP_SNOOPING_conf_apply();

#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            if (!msg_switch_exists(isid)) {
                continue;
            }
            DHCP_SNOOPING_CRIT_ENTER();
            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (DHCP_SNOOPING_global.port_conf[sit.isid].veri_mode[pit.iport] == DHCP_SNOOPING_MGMT_DISABLED ||
                    DHCP_SNOOPING_global.port_conf[sit.isid].port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED) {
                    continue;
                }
                if (DHCP_SNOOPING_global.conf.snooping_mode == DHCP_SNOOPING_MGMT_DISABLED) {
                    //enabled to disabled
                    if ((rc = DHCP_SNOOPING_veri_process(sit.isid, pit.iport, DHCP_SNOOPING_MGMT_DISABLED)) != VTSS_OK) {
                        T_W("Calling DHCP_SNOOPING_veri_process(isid = %u): failed rc = %s", sit.isid, dhcp_snooping_error_txt(rc));
                    }
                } else {
                    //disabled to enabled
                    if ((rc = DHCP_SNOOPING_veri_process(sit.isid, pit.iport, DHCP_SNOOPING_MGMT_ENABLED)) != VTSS_OK) {
                        T_W("Calling DHCP_SNOOPING_veri_process(isid = %u): failed rc = %s", sit.isid, dhcp_snooping_error_txt(rc));
                    }
                }
            }
            DHCP_SNOOPING_CRIT_EXIT();
        }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */
    }

    T_D("exit");

    return rc;
}

/* Get DHCP snooping port configuration */
vtss_rc dhcp_snooping_mgmt_port_conf_get(vtss_isid_t isid, dhcp_snooping_port_conf_t *switch_cfg)
{
    T_D("enter");

    if (switch_cfg == NULL) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_master()) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_MASTER;
    }
    if (!msg_switch_exists(isid)) {
        T_W("isid: %d not exist", isid);
        T_D("exit");
        return DHCP_SNOOPING_ERROR_ISID;
    }

    DHCP_SNOOPING_CRIT_ENTER();
    *switch_cfg = DHCP_SNOOPING_global.port_conf[isid];
    DHCP_SNOOPING_CRIT_EXIT();

    T_D("exit");
    return VTSS_OK;
}

/* Set DHCP snooping port configuration */
vtss_rc dhcp_snooping_mgmt_port_conf_set(vtss_isid_t isid, dhcp_snooping_port_conf_t *switch_cfg)
{
    vtss_rc                         rc      = VTSS_OK;
    int                             changed = 0;
    conf_blk_id_t                   blk_id  = CONF_BLK_DHCP_SNOOPING_PORT_CONF;
    dhcp_snooping_port_conf_blk_t   *dhcp_snooping_port_conf_blk_p;
#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
    dhcp_snooping_port_conf_t       original_conf;
    port_iter_t                     pit;
#endif

    T_D("enter");

    if (switch_cfg == NULL) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_INV_PARAM;
    }
    if (!msg_switch_is_master()) {
        T_W("not master");
        T_D("exit");
        return DHCP_SNOOPING_ERROR_MUST_BE_MASTER;
    }
    if (!msg_switch_exists(isid)) {
        T_W("isid: %d not exist", isid);
        T_D("exit");
        return DHCP_SNOOPING_ERROR_ISID;
    }

    DHCP_SNOOPING_CRIT_ENTER();
#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
    original_conf = DHCP_SNOOPING_global.port_conf[isid];
#else
    memcpy(switch_cfg->veri_mode, DHCP_SNOOPING_global.port_conf[isid].veri_mode, sizeof(DHCP_SNOOPING_global.port_conf[isid].veri_mode));
#endif
    changed = dhcp_snooping_mgmt_port_conf_changed(&DHCP_SNOOPING_global.port_conf[isid], switch_cfg);
    DHCP_SNOOPING_global.port_conf[isid] = *switch_cfg;
    DHCP_SNOOPING_CRIT_EXIT();

    if (changed) {
        /* Save changed configuration */
        if ((dhcp_snooping_port_conf_blk_p = conf_sec_open(CONF_SEC_GLOBAL, blk_id, NULL)) == NULL) {
            T_W("failed to open DHCP snooping table");
        } else {
            dhcp_snooping_port_conf_blk_p->port_conf[isid - VTSS_ISID_START] = *switch_cfg;
            conf_sec_close(CONF_SEC_GLOBAL, blk_id);
        }

        /* Activate changed configuration */
        DHCP_SNOOPING_port_conf_apply(isid);

#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
        DHCP_SNOOPING_CRIT_ENTER();
        if (DHCP_SNOOPING_global.conf.snooping_mode == DHCP_SNOOPING_MGMT_ENABLED) {
            /* Process MAC verification */
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (switch_cfg->veri_mode[pit.iport] != original_conf.veri_mode[pit.iport]) {
                    if (DHCP_SNOOPING_global.port_conf[isid].port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED) {
                        continue;
                    }
                    if (switch_cfg->veri_mode[pit.iport] == DHCP_SNOOPING_MGMT_ENABLED) {
                        //MAC verification disabled to enabled
                        rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_ENABLED);
                    } else {
                        //MAC verification enabled to disabled
                        rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_DISABLED);
                    }
                } else if (switch_cfg->veri_mode[pit.iport] == DHCP_SNOOPING_MGMT_ENABLED) {
                    if (switch_cfg->port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_UNTRUSTED &&
                        original_conf.port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED) {
                        //trusted to untrusted
                        rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_ENABLED);
                    } else if (switch_cfg->port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_TRUSTED &&
                               original_conf.port_mode[pit.iport] == DHCP_SNOOPING_PORT_MODE_UNTRUSTED) {
                        //untrusted to trusted
                        rc = DHCP_SNOOPING_veri_process(isid, pit.iport, DHCP_SNOOPING_MGMT_DISABLED);
                    }
                }
            }
        }
        DHCP_SNOOPING_CRIT_EXIT();
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */
    }

    T_D("exit");

    return rc;
}


/****************************************************************************
 * Module thread
 ****************************************************************************/
/* Caculate the lease time */
static void DHCP_SNOOPING_thread(cyg_addrword_t data)
{
    dhcp_snooping_ip_assigned_info_t    info;
    u32                                 sleep_time, defalt_sleep_time = 60000, cur_time = 0;
    u8                                  mac[6], null_mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    BOOL                                rc;
    uint                                entry_cnt;

    sleep_time = defalt_sleep_time;
    while (1) {
        if (msg_switch_is_master()) {
            while (msg_switch_is_master()) {
                VTSS_OS_MSLEEP((cyg_tick_count_t) sleep_time);

                memcpy(mac, null_mac, sizeof(mac));
                cur_time = (u32) cyg_current_time();
                entry_cnt = 0;

                do {
                    rc = dhcp_snooping_ip_assigned_info_getnext(mac, &info);
                    if (rc) {
                        entry_cnt++;

                        if (((cur_time - info.timestamp) / CYGNUM_HAL_RTC_DENOMINATOR /* seconds */) >= info.lease_time) {
                            if (DHCP_SNOOPING_ip_assigned_info_del(info.mac)) {
                                entry_cnt--;
                                dhcp_snooping_ip_assigned_info_callback(&info, DHCP_SNOOPING_INFO_REASON_LEASE_TIMEOUT);
                            }
                        } else {
                            /* Select the minimun lease time as next sleep time */
                            sleep_time = sleep_time > (info.lease_time * CYGNUM_HAL_RTC_DENOMINATOR - (cur_time - info.timestamp)) ? (info.lease_time * CYGNUM_HAL_RTC_DENOMINATOR - (cur_time - info.timestamp)) : sleep_time;

                            memcpy(mac, info.mac, 6);
                        }
                    }

                    if (!entry_cnt) {
                        sleep_time = defalt_sleep_time;
                    }
                } while (rc);
            } // while (msg_switch_is_master())
        } // if(msg_switch_is_master())

        // No reason for using CPU ressources when we're a slave
        T_D("Suspending DHCP snooping thread");
        cyg_thread_suspend(DHCP_SNOOPING_thread_handle);
        T_D("Resumed DHCP snooping thread");
    } // while(1)

}


/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create DHCP snooping switch configuration */
static void DHCP_SNOOPING_conf_read_switch(vtss_isid_t isid_add)
{
    conf_blk_id_t                   blk_id;
    dhcp_snooping_port_conf_t       *port_conf, new_port_conf;
    dhcp_snooping_port_conf_blk_t   *port_blk;
    int                             changed;
    BOOL                            do_create;
    u32                             size;
    vtss_isid_t                     isid;

    T_D("enter, isid_add: %d", isid_add);

    /* read DHCP snooping port configuration */
    blk_id = CONF_BLK_DHCP_SNOOPING_PORT_CONF;
    if ((port_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, &size)) == NULL ||
        size != sizeof(*port_blk)) {
        T_W("conf_sec_open failed or size mismatch, creating defaults");
        port_blk = conf_sec_create(CONF_SEC_GLOBAL, blk_id, sizeof(*port_blk));
        do_create = 1;
    } else if (port_blk->version != DHCP_SNOOPING_PORT_CONF_BLK_VERSION) {
        T_W("version mismatch, creating defaults");
        do_create = 1;
    } else {
        do_create = (isid_add != VTSS_ISID_GLOBAL);
    }

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (isid_add != VTSS_ISID_GLOBAL && isid_add != isid) {
            continue;
        }

        changed = 0;
        DHCP_SNOOPING_CRIT_ENTER();
        if (do_create) {
            /* Use default values */
            dhcp_snooping_mgmt_port_get_default(isid, &new_port_conf);
            if (port_blk != NULL) {
                port_blk->port_conf[isid - VTSS_ISID_START] = new_port_conf;
            }
        } else {
            /* Use new configuration */
            new_port_conf = port_blk->port_conf[isid - VTSS_ISID_START];
        }
        port_conf = &DHCP_SNOOPING_global.port_conf[isid];
        if (dhcp_snooping_mgmt_port_conf_changed(port_conf, &new_port_conf)) {
            changed = 1;
        }
        *port_conf = new_port_conf;
        DHCP_SNOOPING_CRIT_EXIT();
        if (changed && isid_add != VTSS_ISID_GLOBAL && msg_switch_exists(isid)) {
            DHCP_SNOOPING_port_conf_apply(isid);
        }
    }
    if (port_blk == NULL) {
        T_W("failed to open DHCP snooping port table");
    } else {
        port_blk->version = DHCP_SNOOPING_PORT_CONF_BLK_VERSION;
        conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    }

    T_D("exit");
}

/* Read/create DHCP snooping stack configuration */
static void DHCP_SNOOPING_conf_read_stack(BOOL create)
{
    int                         changed;
    BOOL                        do_create;
    u32                         size;
    dhcp_snooping_conf_t        *old_dhcp_snooping_conf_p, new_dhcp_snooping_conf;
    dhcp_snooping_conf_blk_t    *conf_blk_p;
    conf_blk_id_t               blk_id;
    u32                         blk_version;

    T_D("enter, create: %d", create);

    /* Read/create DHCP snooping configuration */
    blk_id = CONF_BLK_DHCP_SNOOPING_CONF;
    blk_version = DHCP_SNOOPING_CONF_BLK_VERSION;
    if ((conf_blk_p = conf_sec_open(CONF_SEC_GLOBAL, blk_id, &size)) == NULL ||
        size != sizeof(*conf_blk_p)) {
        T_W("conf_sec_open failed or size mismatch, creating defaults");
        conf_blk_p = conf_sec_create(CONF_SEC_GLOBAL, blk_id, sizeof(*conf_blk_p));
        do_create = 1;
    } else if (conf_blk_p->version != blk_version) {
        T_W("version mismatch, creating defaults");
        do_create = 1;
    } else {
        do_create = create;
    }

    changed = 0;
    DHCP_SNOOPING_CRIT_ENTER();
    if (do_create) {
        /* Use default values */
        dhcp_snooping_mgmt_conf_get_default(&new_dhcp_snooping_conf);
        if (conf_blk_p != NULL) {
            conf_blk_p->conf = new_dhcp_snooping_conf;
        }
    } else {
        /* Use new configuration */
        new_dhcp_snooping_conf = conf_blk_p->conf;
    }
    old_dhcp_snooping_conf_p = &DHCP_SNOOPING_global.conf;
    if (dhcp_snooping_mgmt_conf_changed(old_dhcp_snooping_conf_p, &new_dhcp_snooping_conf)) {
        changed = 1;
    }
    DHCP_SNOOPING_global.conf = new_dhcp_snooping_conf;
    DHCP_SNOOPING_CRIT_EXIT();
    if (conf_blk_p == NULL) {
        T_W("failed to open DHCP snooping table");
    } else {
        conf_blk_p->version = blk_version;
        conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    }
    if (changed && create) { //Always set when topology change
        DHCP_SNOOPING_conf_apply();
    }

    T_D("exit");
}

/* Module start */
static void DHCP_SNOOPING_start(BOOL init)
{
    dhcp_snooping_conf_t        *conf_p;
    dhcp_snooping_port_conf_t   *port_conf_p;
    vtss_isid_t                 isid;
    vtss_rc                     rc;

    T_D("enter, init: %d", init);

    if (init) {
        /* Initialize DHCP snooping configuration */
        conf_p = &DHCP_SNOOPING_global.conf;
        dhcp_snooping_mgmt_conf_get_default(conf_p);

        /* Initialize DHCP snooping port configuration */
        for (isid = VTSS_ISID_LOCAL; isid < VTSS_ISID_END; isid++) {
            port_conf_p = &DHCP_SNOOPING_global.port_conf[isid];
            dhcp_snooping_mgmt_port_get_default(isid, port_conf_p);
        }

        /* Create semaphore for critical regions */
        critd_init(&DHCP_SNOOPING_global.crit, "DHCP_SNOOPING_global.crit", VTSS_MODULE_ID_DHCP_SNOOPING, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        DHCP_SNOOPING_CRIT_EXIT();

        /* Create DHCP helper thread */
        cyg_thread_create(THREAD_DEFAULT_PRIO,
                          DHCP_SNOOPING_thread,
                          0,
                          "DHCP Snooping",
                          DHCP_SNOOPING_thread_stack,
                          sizeof(DHCP_SNOOPING_thread_stack),
                          &DHCP_SNOOPING_thread_handle,
                          &DHCP_SNOOPING_thread_block);
    } else {
        /* Register for frame information */
        dhcp_helper_frame_info_register(dhcp_snooping_frame_info_callback);
        dhcp_helper_release_frame_register(dhcp_snooping_release_frame_callback);
        if ((rc = port_global_change_register(VTSS_MODULE_ID_DHCP_SNOOPING, DHCP_SNOOPING_state_change_callback)) != VTSS_OK) {
            T_W("port_global_change_register(): failed rc = %d", rc);
        }

#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
        /* Set timer to port security module */
        if ((rc = psec_mgmt_time_cfg_set(PSEC_USER_DHCP_SNOOPING, 300/* 5 mins */, PSEC_HOLD_TIME_MAX)) != VTSS_OK) {
            T_W("psec_mgmt_time_cfg_set(): failed rc = %d", rc);
        }

        /* Register to port security module */
        if ((rc = psec_mgmt_register_callbacks(PSEC_USER_DHCP_SNOOPING, DHCP_SNOOPING_psec_mac_add_cb, NULL)) != VTSS_OK) {
            T_W("psec_mgmt_register_callbacks(): failed rc = %d", rc);
        }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */
    }
    T_D("exit");
}

/* Initialize module */
vtss_rc dhcp_snooping_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    vtss_rc     rc = VTSS_OK;

    if (data->cmd == INIT_CMD_INIT) {
        /* Initialize and register trace ressources */
        VTSS_TRACE_REG_INIT(&DHCP_SNOOPING_trace_reg, DHCP_SNOOPING_trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&DHCP_SNOOPING_trace_reg);
    }

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        DHCP_SNOOPING_start(1);
#ifdef VTSS_SW_OPTION_VCLI
        dhcp_snooping_cli_init();
#endif
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = dhcp_snooping_icfg_init()) != VTSS_OK) {
            T_D("Calling dhcp_snooping_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif
        break;
    case INIT_CMD_START:
        T_D("START");
        DHCP_SNOOPING_start(0);
        break;
    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            DHCP_SNOOPING_conf_read_stack(1);
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
            if ((rc = DHCP_SNOOPING_veri_process_start(isid, 1)) != VTSS_OK) {
                T_D("Calling DHCP_SNOOPING_veri_process_start(isid = %u): failed rc = %s", isid, dhcp_snooping_error_txt(rc));
            }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */
            DHCP_SNOOPING_conf_read_switch(isid);
        }
        break;
    case INIT_CMD_MASTER_UP: {
        T_D("MASTER_UP");
        /* Read stack and switch configuration */
        DHCP_SNOOPING_conf_read_stack(0);
        DHCP_SNOOPING_conf_read_switch(VTSS_ISID_GLOBAL);
        /* Starting DHCP snooping thread (became master) */
        cyg_thread_resume(DHCP_SNOOPING_thread_handle);
        break;
    }
    case INIT_CMD_MASTER_DOWN:
        T_D("MASTER_DOWN");
        DHCP_SNOOPING_ip_assigned_info_clear();
        break;
    case INIT_CMD_SWITCH_ADD: {
        T_D("SWITCH_ADD, isid: %d", isid);
        /* Apply all configuration to switch */
        if (msg_switch_is_local(isid)) {
            DHCP_SNOOPING_conf_apply();
        }

        DHCP_SNOOPING_port_conf_apply(isid);
#if DHCP_SNOOPING_MAC_VERI_SUPPORTED
        if ((rc = DHCP_SNOOPING_veri_process_start(isid, 0)) != VTSS_OK) {
            T_D("Calling DHCP_SNOOPING_veri_process_start(isid = %u): failed rc = %s", isid, dhcp_snooping_error_txt(rc));
        }
#endif /* DHCP_SNOOPING_MAC_VERI_SUPPORTED */
        break;
    }
    case INIT_CMD_SWITCH_DEL: {
        u8                                  mac[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
        dhcp_snooping_ip_assigned_info_t    ip_assigned_info;

        T_D("SWITCH_DEL, isid: %d", isid);
        if (msg_switch_is_master()) {
            while (dhcp_snooping_ip_assigned_info_getnext(mac, &ip_assigned_info)) {
                memcpy(mac, ip_assigned_info.mac, 6);
                if (ip_assigned_info.isid == isid) {
                    dhcp_snooping_ip_assigned_info_callback(&ip_assigned_info, DHCP_SNOOPING_INFO_REASON_SWITCH_DOWN);
                }
            }
            DHCP_SNOOPING_ip_assigned_info_clear_isid(isid);
        }
        break;
    }
    default:
        break;
    }

    T_D("exit");

    return rc;
}


/****************************************************************************/
/*  Statistics functions                                                    */
/****************************************************************************/

/* Get DHCP snooping statistics
   Return 0  : Success
   Return -1 : Fail */
int dhcp_snooping_stats_get(vtss_isid_t isid, vtss_port_no_t port_no, dhcp_snooping_stats_t *stats)
{
    if (!msg_switch_is_master()) {
        T_W("not master");
        return -1;
    }

    return(dhcp_helper_stats_get(isid, port_no, stats));
}

/* Clear DHCP snooping statistics
   Return 0  : Success
   Return -1 : Fail */
int dhcp_snooping_stats_clear(vtss_isid_t isid, vtss_port_no_t port_no)
{
    if (!msg_switch_is_master()) {
        T_W("not master");
        return -1;
    }

    return(dhcp_helper_stats_clear(isid, port_no));
}


/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
