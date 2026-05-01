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

   $Id$
   $Revision$

 */

#include "main.h"
#include "port_api.h"
#include "vlan_api.h"
#include "cli_trace_def.h"
#include "cli.h"
#include "mgmt_api.h"
#include "vlan_cli.h"

/* Temporary disabled while XXRP is under construction */
#undef VTSS_SW_OPTION_MVRP

typedef struct {
    BOOL                    vlan_cmd_all;
    BOOL                    tag_this;
    BOOL                    untag_this;
    BOOL                    tag_all;
    BOOL                    untag_all;
    BOOL                    untag_pvid;
    BOOL                    all;
    BOOL                    tagged;
    BOOL                    untagged;
    BOOL                    conflicts_disp;
    vlan_user_t             vlan_user;
    vlan_user_t             vlan_membership_user;
    BOOL                    usr_valid;
#ifdef VTSS_SW_OPTION_VLAN_NAME
    BOOL                    is_vid;
    BOOL                    name;
    BOOL                    vid_exists;
    BOOL                    single_name;
#endif
    vlan_port_type_t        port_type;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    vtss_etype_t            tpid;
#endif
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    BOOL                    clear;
#endif /* VTSS_FEATURE_VLAN_COUNTERS */
    BOOL                    uports_list[VTSS_PORT_ARRAY_SIZE + 1];
} vlan_cli_req_t;

#if defined(VTSS_SW_OPTION_DOT1X) || defined(VTSS_SW_OPTION_MSTP) || \
    defined(VTSS_SW_OPTION_MVR) || defined(VTSS_SW_OPTION_MVRP) ||  \
defined(VTSS_SW_OPTION_VOICE_VLAN) || defined(VTSS_SW_OPTION_ERPS) || \
defined(VTSS_SW_OPTION_MEP) || defined(VTSS_SW_OPTION_VCL) || defined(VTSS_SW_OPTION_RSPAN)
#define VLAN_USER_DEBUG_CMD_ENABLE  1 
// One or more of "nas|mvr|mvrp|voice_vlan|mstp|vcl|rspan"
static char VLAN_users_names[64];
// Following rw syntax must be initialized in order to be sorted correctly. 
static char VLAN_debug_rw_syntax_porttype[164] = "Debug VLAN PortType";
static char VLAN_debug_rw_syntax_pvid[164]     = "Debug VLAN PVID";
static char VLAN_debug_rw_syntax_ingr[164]     = "Debug VLAN IngressFilter";
static char VLAN_debug_rw_syntax_fr_type[164]  = "Debug VLAN FrameType";
static char VLAN_debug_rw_syntax_tx_tag[164]   = "Debug VLAN Tx_tag";
static char VLAN_debug_rw_syntax_del[164]      = "Debug VLAN Port_del";
#endif

static char *cli_vlan_frame_type_txt(vtss_vlan_frame_t tagged)
{
    if (tagged == VTSS_VLAN_FRAME_ALL) {
        return "All";
    } else if (tagged == VTSS_VLAN_FRAME_TAGGED) {
        return "Tagged";
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    } else if (tagged == VTSS_VLAN_FRAME_UNTAGGED) {
        return "Untagged";
#endif
    } else {
        return "Unknown";
    }
}

static char *cli_vlan_static_tag_type_txt(vlan_tx_tag_type_t tx_tag_type)
{
    if (tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_THIS) {
        return "Untag PVID";
    } else if (tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_ALL) {
        return "Untag All";
    } else if (tx_tag_type == VLAN_TX_TAG_TYPE_TAG_ALL) {
        return "Tag All";
    }
    return "";
}

static char *cli_vlan_port_type_txt(vlan_port_type_t pt)
{
    if (pt == VLAN_PORT_TYPE_UNAWARE) {
        return "Unaware";
    } else if (pt == VLAN_PORT_TYPE_C) {
        return "C-Port";
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    } else if (pt == VLAN_PORT_TYPE_S) {
        return "S-Port";
    } else if (pt == VLAN_PORT_TYPE_S_CUSTOM) {
        return "S-Custom-Port";
#endif
    } else {
        return "Unknown-Port";
    }
}

#ifdef VLAN_USER_DEBUG_CMD_ENABLE
/* Generate dynamic parser strings */
static void VLAN_create_parser_strings(void)
{
    unsigned int cnt     = 0;
    unsigned int entries = 0;

#ifdef VTSS_SW_OPTION_DOT1X
    cnt += snprintf(VLAN_users_names + cnt, sizeof(VLAN_users_names) - cnt, 
            "%s%s", entries++ ? "|" : "", "nas");
#endif
#ifdef VTSS_SW_OPTION_MSTP
    cnt += snprintf(VLAN_users_names + cnt, sizeof(VLAN_users_names) - cnt, 
            "%s%s", entries++ ? "|" : "", "mstp");
#endif
#ifdef VTSS_SW_OPTION_MVR
    cnt += snprintf(VLAN_users_names + cnt, sizeof(VLAN_users_names) - cnt, 
            "%s%s", entries++ ? "|" : "", "mvr");
#endif




#ifdef VTSS_SW_OPTION_VOICE_VLAN
    cnt += snprintf(VLAN_users_names + cnt, sizeof(VLAN_users_names) - cnt, 
            "%s%s", entries++ ? "|" : "", "voice_vlan");
#endif
#ifdef VTSS_SW_OPTION_ERPS
    cnt += snprintf(VLAN_users_names + cnt, sizeof(VLAN_users_names) - cnt,
            "%s%s", entries++ ? "|" : "", "erps");
#endif
#ifdef VTSS_SW_OPTION_VCL
    cnt += snprintf(VLAN_users_names + cnt, sizeof(VLAN_users_names) - cnt, 
            "%s%s", entries++ ? "|" : "", "vcl");
#endif
#ifdef VTSS_SW_OPTION_MEP
    cnt += snprintf(VLAN_users_names + cnt, sizeof(VLAN_users_names) - cnt,
            "%s%s", entries++ ? "|" : "", "mep");
#endif
#ifdef VTSS_SW_OPTION_RSPAN
    cnt += snprintf(VLAN_users_names + cnt, sizeof(VLAN_users_names) - cnt, 
            "%s%s", entries++ ? "|" : "", "rspan");
#endif
    if (cnt >= sizeof(VLAN_users_names)) {
        T_E("VLAN_users_names truncated (%u,%zu)", 
                cnt, sizeof(VLAN_users_names));
    }

    cnt = snprintf(VLAN_debug_rw_syntax_porttype,  
            sizeof(VLAN_debug_rw_syntax_porttype),  
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
            "Debug VLAN PortType [<port_list>] %s [unaware|c-port]", 
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
            "Debug VLAN PortType [<port_list>] %s [unaware|c-port|s-port|s-custom-port]", 
#endif
            VLAN_users_names);
    if (cnt >= sizeof(VLAN_debug_rw_syntax_porttype)) {
        T_E("VLAN_debug_rw_syntax_porttype truncated (%u,%zu)", 
                cnt, sizeof(VLAN_debug_rw_syntax_porttype));
    }

    cnt = snprintf(VLAN_debug_rw_syntax_pvid,  
            sizeof(VLAN_debug_rw_syntax_pvid),  
            "Debug VLAN PVID [<port_list>] %s [<vid>]", VLAN_users_names);
    if (cnt >= sizeof(VLAN_debug_rw_syntax_pvid)) {
        T_E("VLAN_debug_rw_syntax_pvid truncated (%u,%zu)", 
                cnt, sizeof(VLAN_debug_rw_syntax_pvid));
    }

    cnt = snprintf(VLAN_debug_rw_syntax_ingr,  
            sizeof(VLAN_debug_rw_syntax_ingr),  
            "Debug VLAN IngressFilter [<port_list>] %s [enable|disable]", 
            VLAN_users_names);
    if (cnt >= sizeof(VLAN_debug_rw_syntax_ingr)) {
        T_E("VLAN_debug_rw_syntax_ingr truncated (%u,%zu)", 
                cnt, sizeof(VLAN_debug_rw_syntax_ingr));
    }

    cnt = snprintf(VLAN_debug_rw_syntax_fr_type,  
            sizeof(VLAN_debug_rw_syntax_fr_type),  
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
            "Debug VLAN FrameType [<port_list>] %s [all|tagged]", 
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
            "Debug VLAN FrameType [<port_list>] %s [all|tagged|untagged]", 
#endif
            VLAN_users_names);
    if (cnt >= sizeof(VLAN_debug_rw_syntax_fr_type)) {
        T_E("VLAN_debug_rw_syntax_fr_type truncated (%u,%zu)", 
                cnt, sizeof(VLAN_debug_rw_syntax_fr_type));
    }

    cnt = snprintf(VLAN_debug_rw_syntax_tx_tag,  
            sizeof(VLAN_debug_rw_syntax_tx_tag),  
            "Debug VLAN Tx_tag [<port_list>] %s [tag_this|untag_this|tag_all|untag_all] "
            "[<vid>]", VLAN_users_names);
    if (cnt >= sizeof(VLAN_debug_rw_syntax_tx_tag)) {
        T_E("VLAN_debug_rw_syntax_tx_tag truncated (%u,%zu)", 
                cnt, sizeof(VLAN_debug_rw_syntax_tx_tag));
    }

    cnt = snprintf(VLAN_debug_rw_syntax_del,  
            sizeof(VLAN_debug_rw_syntax_del),  
            "Debug VLAN Port_del [<port_list>] %s", VLAN_users_names);
    if (cnt >= sizeof(VLAN_debug_rw_syntax_del)) {
        T_E("VLAN_debug_rw_syntax_del truncated (%u,%zu)", cnt, 
                sizeof(VLAN_debug_rw_syntax_del));
    }
}
#endif  //VLAN_USER_DEBUG_CMD_ENABLE

/****************************************************************************/
/*  Initialization                                                          */
/****************************************************************************/

void vlan_cli_req_init(void)
{
#ifdef VLAN_USER_DEBUG_CMD_ENABLE
    VLAN_create_parser_strings();
#endif
    /* register the size required for VLAN req. structure */
    cli_req_size_register(sizeof(vlan_cli_req_t));
}

static void vlan_cli_req_delete_default_set ( cli_req_t * req )
{
    vlan_cli_req_t *vlan_req = req->module_req;

    vlan_req->vlan_membership_user = VLAN_USER_STATIC;
    vlan_req->vlan_cmd_all = 0;
    vlan_req->usr_valid = 1;
}

static void vlan_cli_req_lookup_default_set ( cli_req_t * req )
{
    vlan_cli_req_t *vlan_req = req->module_req;

    vlan_req->vlan_membership_user = VLAN_USER_ALL;
    vlan_req->vlan_cmd_all = 0;
    vlan_req->usr_valid = 1;
}

static void vlan_cli_req_conf_default_set ( cli_req_t * req )
{
    vlan_cli_req_t *vlan_req = req->module_req;

    vlan_req->vlan_membership_user = VLAN_USER_STATIC;
    vlan_req->vlan_cmd_all = 0;
    vlan_req->usr_valid = 1;
}


static char *cli_vlan_tag_type_txt(vlan_tx_tag_type_t tx_tag_type)
{
    if (tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_THIS) {
        return "Untag This";
    } else if (tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_ALL) {
        return "Untag All";
    } else if (tx_tag_type == VLAN_TX_TAG_TYPE_TAG_THIS) {
        return "Tag This";
    } else if (tx_tag_type == VLAN_TX_TAG_TYPE_TAG_ALL) {
        return "Tag All";
    }
    return "";
}

/* VLAN Port configuration */
static void cli_cmd_vlan_port_conf(cli_req_t *req, BOOL pvid, BOOL frametype, 
                                   BOOL ingressfilter, BOOL porttype, BOOL tx_tag)
{
    vtss_isid_t           usid;
    vtss_isid_t           isid;
    vlan_port_conf_t      conf;
    char                  buf[80], *p;
    vlan_cli_req_t *      vlan_req = req->module_req;
    port_iter_t           pit;
    BOOL                  first = TRUE;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req))
        return;

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END)
            continue;

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || vlan_mgmt_port_conf_get(isid, pit.iport, &conf, VLAN_USER_STATIC) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                conf.flags = VLAN_PORT_FLAGS_ALL;
                if (pvid) {
#ifdef VTSS_SW_OPTION_ACL
                    if (mgmt_vlan_pvid_change(isid, pit.iport, req->vid, conf.pvid) 
                            != VTSS_OK) {
                        return;
                    }
#endif /* VTSS_SW_OPTION_ACL */
                    conf.pvid = req->vid;
                }
                if (frametype) {
                    if (vlan_req->all) {
                        conf.frame_type = VTSS_VLAN_FRAME_ALL;
                    } else if (vlan_req->tagged) {
                        conf.frame_type = VTSS_VLAN_FRAME_TAGGED;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
                    } else if (vlan_req->untagged) {
                        conf.frame_type = VTSS_VLAN_FRAME_UNTAGGED;
#endif
                    } else {
                        T_D("Invalid FrameType. Setting it to default value");
                        conf.frame_type = VTSS_VLAN_FRAME_ALL;
                    }
                }
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                if (ingressfilter) {
                    conf.ingress_filter = req->enable;
                }
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
                if (tx_tag) {
                    if (vlan_req->untag_all) {
                        conf.tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_ALL;
                    } else if (vlan_req->tag_all) {
                        conf.tx_tag_type = VLAN_TX_TAG_TYPE_TAG_ALL;
                    } else if (vlan_req->untag_pvid) {
                        /* port configuration is already fetched, use pvid from that configuration */
                        conf.untagged_vid = conf.pvid;
                        conf.tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_THIS;
                    } else {
                        T_D("tx_tag processing error VLAN CLI\n");
                    }
                }
                if (porttype) {
                    conf.port_type = vlan_req->port_type;
                }
                if(vlan_mgmt_port_conf_set(isid, pit.iport, &conf, VLAN_USER_STATIC)                   
                        != VTSS_OK) {
                    return;
                }
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    if (pvid)
                        p += sprintf(p, "PVID  ");
                    if (frametype)
                        p += sprintf(p, "Frame Type  ");
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                    if (ingressfilter)
                        p += sprintf(p, "Ingress Filter  ");
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
                    if (tx_tag)
                        p += sprintf(p, "Tx Tag      ");
                    if (porttype)
                        p += sprintf(p, "Port Type      ");
                    cli_table_header(buf);
                }
                CPRINTF("%-2d    ", pit.uport);
                if (pvid) {
                    if ((conf.pvid == VLAN_TAG_DEFAULT) && (conf.untagged_vid == VTSS_VID_NULL) &&
                        (conf.tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_THIS)) 
                        CPRINTF("None  ");
                    else
                        CPRINTF("%-4d  ", conf.pvid);
                }
                if (frametype)
                    CPRINTF("%-11s ", cli_vlan_frame_type_txt(conf.frame_type));
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                if (ingressfilter)
                    CPRINTF("%-16s", cli_bool_txt(conf.ingress_filter));
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
                if (tx_tag)
                    CPRINTF("%-12s", cli_vlan_static_tag_type_txt(conf.tx_tag_type));
                if (porttype)
                    CPRINTF("%-15s", cli_vlan_port_type_txt(conf.port_type));
                CPRINTF("\n");
            }
        }
    }
}

/* VLAN VID configuration */
static void cli_cmd_vlan_vid_conf(cli_req_t *req, BOOL add, BOOL delete, BOOL lookup, BOOL forbidden)
{
    vtss_usid_t        usid;
    vtss_isid_t        isid, isid_add_or_del;
    vtss_port_no_t     iport;
    vlan_mgmt_entry_t  conf;
    vlan_mgmt_entry_t  forbid_conflict;
    BOOL               first;
    BOOL               vid_first;
    BOOL               mports[VTSS_PORT_ARRAY_SIZE], fports[VTSS_PORT_ARRAY_SIZE];
    vtss_vid_t         vid;
    vtss_rc            rc;
    char               buf[MGMT_PORT_BUF_SIZE];
    char               conf_buf[MGMT_PORT_BUF_SIZE];
    vlan_cli_req_t     *vlan_req = req->module_req;
    vlan_user_t        vlan_user = VLAN_USER_STATIC, mgmt_user = VLAN_USER_STATIC,
                       start_vlan_user = VLAN_USER_STATIC, end_vlan_user = VLAN_USER_STATIC;

    if (lookup && (!vlan_req->usr_valid)) {
        CPRINTF("%s is not supported\n", vlan_mgmt_vlan_user_to_txt(vlan_req->vlan_membership_user));
        return;
    }
    if (cli_cmd_switch_none(req) ||(lookup == FALSE && cli_cmd_conf_slave(req))) {
        return;
    }

    if (lookup && ( !forbidden)) {
        if (vlan_req->vlan_cmd_all) {
            start_vlan_user = VLAN_USER_STATIC;
            end_vlan_user   = VLAN_USER_CNT;
            mgmt_user       = VLAN_USER_ALL;
        } else {
            start_vlan_user = vlan_req->vlan_membership_user;
            end_vlan_user   = (vlan_user_t)((unsigned int) start_vlan_user + 1);
            mgmt_user     = start_vlan_user;
        }
    }

    if (add || delete) {
        mgmt_user = vlan_req->vlan_membership_user;
    }

    /* If the "stack sel all" is selected, pass isid as VTSS_ISID_GLOBAL */
    if (req->usid_sel == VTSS_USID_ALL) {
        isid_add_or_del = VTSS_ISID_GLOBAL;
    } else {
        for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
            if ((isid_add_or_del = req->stack.isid[usid]) == VTSS_ISID_END) {
                continue;
            } else {
                break;
            }
        }
    }

    if (delete && (!forbidden)) {
        /* Ignore the return values as VLAN might not be present in some of
         * the slots
         */
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        if (vlan_req->is_vid == TRUE) {
#endif
            rc = vlan_mgmt_vlan_del(isid_add_or_del, req->vid, mgmt_user, VLAN_MEMBERSHIP_OP);
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        } else {
            rc = vlan_mgmt_vlan_del_by_name(isid_add_or_del, req->parm, mgmt_user, VLAN_MEMBERSHIP_OP);
        }
#endif

        if (rc != VTSS_RC_OK) {
            CPRINTF("VLAN deletion failure\n");
        }
        return;
    } else if(delete && (forbidden)) {
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        if (vlan_req->is_vid == TRUE) {
#endif
            rc = vlan_mgmt_vlan_del(isid_add_or_del, req->vid, mgmt_user, VLAN_FORBIDDEN_OP);
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        } else {
            rc = vlan_mgmt_vlan_del_by_name(isid_add_or_del, req->parm, mgmt_user, VLAN_FORBIDDEN_OP);
        }
#endif

        if (rc != VTSS_RC_OK) {
            CPRINTF("VLAN deletion failure\n");
        }
        return;
    } else if (add && (!forbidden)) {
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        if (vlan_req->is_vid == TRUE) {
#endif
            conf.vid = req->vid;
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        } else {
            strcpy(conf.vlan_name, req->parm);
        }
#endif
        for (iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
            conf.ports[iport] = (vlan_req->uports_list[iport2uport(iport)]) ? VLAN_MEMBER_PORT : VLAN_NON_MEMBER_PORT;
        }
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        if (vlan_req->is_vid == TRUE) {
#endif        
            rc = vlan_mgmt_vlan_add(isid_add_or_del, &conf, mgmt_user);
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        } else {
            rc = vlan_mgmt_vlan_add_by_name(isid_add_or_del, &conf, mgmt_user);
        }
#endif
        if (rc == VLAN_ERROR_VLAN_TABLE_FULL) {
            CPRINTF("VLAN table full\n");
        } else if (rc == VLAN_ERROR_CONFIG_NOT_OPEN) {
            CPRINTF("VLAN table configuration open error\n");
        } else if (rc == VLAN_ERROR_USER_VLAN_ID_MISMATCH ) {
            CPRINTF("VLAN Id Membership conflict\n");
        } else if (rc == VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED) {
            CPRINTF("%s User Configured a different VID on the ports\n",
                    vlan_mgmt_vlan_user_to_txt(vlan_req->vlan_membership_user));
        } else if (rc == VLAN_ERROR_VLAN_FORBIDDEN) {
            CPRINTF("Error: Part of the VLAN configuration didn't apply as some of ports are in forbidden list for a given VLAN id.\n");
        } else if (rc == VLAN_ERROR_ENTRY_NOT_FOUND) {
            CPRINTF("VLAN name %s not found\n", conf.vlan_name);
        } 
        return;
    } else if(add && forbidden) {   
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        if (vlan_req->is_vid == TRUE) {
#endif
            conf.vid = req->vid;
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        } else {
            strcpy(conf.vlan_name, req->parm);
        }
#endif

        for (iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
            conf.ports[iport] = (req->uport_list[iport2uport(iport)]) ? VLAN_FORBIDDEN_PORT : VLAN_NON_FORBIDDEN_PORT;
        }
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        if (vlan_req->is_vid == TRUE) {
#endif 
            rc= vlan_mgmt_vlan_add(isid_add_or_del, &conf, mgmt_user);
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        } else {
            rc = vlan_mgmt_vlan_add_by_name(isid_add_or_del, &conf, mgmt_user);
        }
#endif
        if (rc == VLAN_ERROR_VLAN_TABLE_FULL) {
            CPRINTF("VLAN table full\n");
        } else if (rc == VLAN_ERROR_CONFIG_NOT_OPEN) {
            CPRINTF("VLAN table configuration open error\n");
        } else if (rc == VLAN_ERROR_USER_VLAN_ID_MISMATCH ) {
            CPRINTF("VLAN Id Membership conflict\n");
        } else if (rc == VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED) {
            CPRINTF("%s User Configured a different VID on the ports\n",
                    vlan_mgmt_vlan_user_to_txt(vlan_req->vlan_membership_user));
        } else if (rc == VLAN_ERROR_VLAN_FORBIDDEN) {
            CPRINTF("Error: Part of the VLAN configuration didn't apply as some of ports are in member list for a given VLAN id.\n");
        } 
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END)
            continue;

        first = 1;

        if (lookup && (!forbidden)) {
            /* Lookup specific vid */
            if (lookup && (req->vid_spec == CLI_SPEC_VAL)) {
#ifdef  VTSS_SW_OPTION_VLAN_NAME
                if ((vlan_req->vid_exists == TRUE) && (vlan_req->name == TRUE)) {
                    CPRINTF("Enter either VID or VLAN Name\n");
                    return ;
                }
#endif
                vid = req->vid;
                BOOL conflict_vid = FALSE;
                for (vlan_user = start_vlan_user; vlan_user < end_vlan_user; vlan_user++) {
                    conflict_vid = FALSE;
#ifndef  VTSS_SW_OPTION_VLAN_NAME
                    if (vlan_mgmt_vlan_get(isid, vid, &conf, 0, vlan_user, VLAN_MEMBERSHIP_OP) == VTSS_OK) {
                        if (first == 1) {
                            cli_cmd_usid_print(usid, req, 1);
                            if (vlan_req->vlan_cmd_all) {
                                cli_table_header("VID   User         Ports                                    Conflicts   Conflict_Ports");
                            } else {
                                cli_table_header("VID   Ports                                    Conflicts   Conflict_Ports");
                            }
                        }
#else
                        if (vlan_req->name == FALSE) {
                            rc = vlan_mgmt_vlan_get(isid, vid, &conf, 0, vlan_user, VLAN_MEMBERSHIP_OP);
                        } else {
                            rc = vlan_mgmt_vlan_get_by_name(isid, req->parm, &conf, 0, vlan_user, VLAN_MEMBERSHIP_OP);
                        }

                        memset(&forbid_conflict, 0, sizeof(vlan_mgmt_entry_t));
                        if (vlan_req->name == FALSE) {
                            (void)vlan_mgmt_vlan_get(isid, vid, &forbid_conflict, 0, VLAN_USER_STATIC, VLAN_FORBIDDEN_OP);
                        } else {
                            (void)vlan_mgmt_vlan_get(isid, conf.vid, &forbid_conflict, 0, VLAN_USER_STATIC, VLAN_FORBIDDEN_OP);
                        }
                        for(iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
                            mports[iport] = (conf.ports[iport] == VLAN_MEMBER_PORT) ? TRUE : FALSE;
                            fports[iport] = (forbid_conflict.ports[iport] == VLAN_FORBIDDEN_PORT) ? TRUE : FALSE;
                            fports[iport] &= mports[iport];
                            if(fports[iport] == 1) {
                                conflict_vid = TRUE;
                            }
                        }

                        if (rc == VTSS_OK) {
                            if (first == 1) {
                                cli_cmd_usid_print(usid, req, 1);
                                if (vlan_req->vlan_cmd_all) {
                                    cli_table_header("VID   VLAN Name                         User        Ports                                    Conflicts   Conflict_Ports");
                                } else {
                                    cli_table_header("VID   VLAN Name                         Ports                                    Conflicts   Conflict_Ports");
                                }
                            }
#endif
                            if (vlan_req->vlan_cmd_all) {
                                if (first) {
#ifndef  VTSS_SW_OPTION_VLAN_NAME
                                    if((vlan_user == VLAN_USER_STATIC) || (vlan_user == VLAN_USER_ALL)) {
                                        CPRINTF("%-4d  %-10s  %-39s  %-9s   %s\n", vid,
                                                vlan_mgmt_vlan_user_to_txt(vlan_user), cli_iport_list_txt(mports, buf), "No", "None");
                                    } else {
                                        if(conflict_vid) {
                                            CPRINTF("%-4d  %-10s  %-39s  %-9s   %s\n", vid,
                                                vlan_mgmt_vlan_user_to_txt(vlan_user), cli_iport_list_txt(mports, buf), "Yes", 
                                                cli_iport_list_txt(fports,conf_buf));
                                            conflict_vid = FALSE;
                                        } else {
                                            CPRINTF("%-4d  %-10s  %-39s  %-9s   %s\n", vid,
                                                vlan_mgmt_vlan_user_to_txt(vlan_user), cli_iport_list_txt(mports, buf), "No", "None");
                                        }
                                    }
#else
                                    if (vlan_req->name == FALSE) {
                                        if((vlan_user == VLAN_USER_STATIC) || (vlan_user == VLAN_USER_ALL)) {
                                            CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", vid, conf.vlan_name, 
                                                vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                cli_iport_list_txt(mports, buf), "No", "None");
                                        } else {
                                            if(conflict_vid) {
                                                CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", vid, conf.vlan_name,
                                                                             vlan_mgmt_vlan_user_to_txt(vlan_user),                                                                                                                                          cli_iport_list_txt(mports, buf), "Yes", 
                                                                             cli_iport_list_txt(fports, conf_buf));
                                                conflict_vid = FALSE;
                                            } else {
                                                 CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", vid, conf.vlan_name,                                                                                                                                      vlan_mgmt_vlan_user_to_txt(vlan_user),                                                                                                                                          cli_iport_list_txt(mports, buf), "No", "None");
                                            }
                                        }
                                    } else {
                                        if((vlan_user == VLAN_USER_STATIC) || (vlan_user == VLAN_USER_ALL)) {
                                            CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", conf.vid, conf.vlan_name,                                                                                                                                      vlan_mgmt_vlan_user_to_txt(vlan_user),                                                                                                                                          cli_iport_list_txt(mports, buf), "No", "None");
                                        } else {
                                            if(conflict_vid) {
                                                CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", conf.vid, conf.vlan_name,                                                                                                                          vlan_mgmt_vlan_user_to_txt(vlan_user), cli_iport_list_txt(mports,buf), 
                                                                  "Yes", cli_iport_list_txt(fports, conf_buf));
                                                conflict_vid = FALSE;
                                            } else {
                                                CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", conf.vid, conf.vlan_name,                                                                                                                                      vlan_mgmt_vlan_user_to_txt(vlan_user),                                                                                                                                          cli_iport_list_txt(mports, buf), "No", "None");
                                            }
                                        }
                                    }
#endif
                                } else {
#ifndef  VTSS_SW_OPTION_VLAN_NAME
                                    if((vlan_user == VLAN_USER_STATIC) || (vlan_user == VLAN_USER_ALL)) {
                                        CPRINTF("      %-10s  %-39s  %-9s   %s\n",
                                                                                  vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                                                  cli_iport_list_txt(mports, buf), "No", "None");
                                    } else {
                                        if(conflict_vid) {
                                            CPRINTF("      %-10s  %-39s  %-9s   %s\n",
                                                                  vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                                  cli_iport_list_txt(mports, buf), "Yes", 
                                                                  cli_iport_list_txt(fports, conf_buf));
                                            conflict_vid = FALSE;
                                        } else {
                                            CPRINTF("      %-10s  %-39s  %-9s   %s\n",
                                                                  vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                                  cli_iport_list_txt(mports, buf), "No", "None");
                                        }
                                    }
#else
                                   if((vlan_user == VLAN_USER_STATIC) || (vlan_user == VLAN_USER_ALL)) {
                                       CPRINTF("                                        %-10s  %-39s  %-9s   %s\n",
                                                               vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                               cli_iport_list_txt(mports, buf), "No", "None");

                                   } else {
                                       if(conflict_vid) {
                                           CPRINTF("                                        %-10s  %-39s  %-9s   %s\n",
                                                                vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                                cli_iport_list_txt(mports, buf), "Yes", 
                                                                cli_iport_list_txt(fports, conf_buf));
                                           conflict_vid = FALSE;
                                       } else {
                                           CPRINTF("                                        %-10s  %-39s  %-9s   %s\n",
                                                               vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                               cli_iport_list_txt(mports, buf), "No", "None");
                                       }
                                   }
#endif
                                }
                            } else {
#ifndef  VTSS_SW_OPTION_VLAN_NAME
                                CPRINTF("%-4d  %s\n", vid, 
                                        cli_iport_list_txt(mports, buf));
#else
                                if (vlan_req->name == FALSE) {
                                    CPRINTF("%-4d  %-32s  %s\n", vid, conf.vlan_name, 
                                            cli_iport_list_txt(mports, buf));
                                } else {
                                    CPRINTF("%-4d  %-32s  %s\n", conf.vid, conf.vlan_name, 
                                            cli_iport_list_txt(mports, buf));
                                }
#endif
                            }
                            first  = 0;
                        }
                    }
            } else {
                /* Lookup all list */
                vid = VTSS_VID_NULL;
                BOOL conflict = FALSE;
                while (vlan_mgmt_vlan_get(isid, vid, &conf, 1, mgmt_user, VLAN_MEMBERSHIP_OP) == VTSS_OK) {
                    T_N("called vlan_mgmt_vlan_get, lookup: %d, vid: %u, req->vid: %u", lookup, vid, req->vid);
                    if (lookup && (req->vid_spec == CLI_SPEC_VAL) && (vid != req->vid)) {
                        continue;
                    }
                    vid_first = 1;
                    vid = conf.vid;
                    if (vlan_req->vlan_cmd_all && (vlan_req->vlan_cmd_all)) {
                        for (vlan_user = start_vlan_user; vlan_user < end_vlan_user ; vlan_user++) {
                            conflict = FALSE;
                            if (vlan_mgmt_vlan_get(isid, vid, &conf, 0, vlan_user, VLAN_MEMBERSHIP_OP) == VTSS_OK) {
                                 memset(&forbid_conflict, 0, sizeof(vlan_mgmt_entry_t));
                                (void)vlan_mgmt_vlan_get(isid, vid, &forbid_conflict, 0, VLAN_USER_STATIC, VLAN_FORBIDDEN_OP);
                                for(iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
                                    mports[iport] = (conf.ports[iport] == VLAN_MEMBER_PORT) ? TRUE : FALSE;
                                    fports[iport] = (forbid_conflict.ports[iport] == VLAN_FORBIDDEN_PORT) ? TRUE : FALSE;
                                    fports[iport] &= mports[iport];
                                    if(fports[iport] == 1) {
                                        conflict = TRUE;
                                    }
                                }

                                if (first == 1) {
                                    cli_cmd_usid_print(usid, req, 1);
                                #ifdef  VTSS_SW_OPTION_VLAN_NAME
                                    cli_table_header("VID   VLAN Name                         User        Ports                                    Conflicts   Conflict_Ports");
                                #else
                                    cli_table_header("VID   User        Ports                                    Conflicts   Conflict_Ports");
                                #endif
                                }
                                if (vid_first) {
                                #ifdef  VTSS_SW_OPTION_VLAN_NAME
                                    if( vlan_user == VLAN_USER_STATIC || vlan_user == VLAN_USER_ALL) {
                                        CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", 
                                               vid, conf.vlan_name, vlan_mgmt_vlan_user_to_txt(vlan_user), 
                                               cli_iport_list_txt(mports, buf), "No", "None");
                                    } else {
                                        if( conflict ) {
                                            CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", vid, conf.vlan_name, 
                                                                             vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                                             cli_iport_list_txt(mports, buf), "Yes", 
                                                                             cli_iport_list_txt(fports, conf_buf));
                                            conflict = FALSE;
                                        } else {
                                            CPRINTF("%-4d  %-32s  %-10s  %-39s  %-9s   %s\n", vid, conf.vlan_name, 
                                                                          vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                                          cli_iport_list_txt(mports, buf), "No", 
                                                                          "None");
                                        }
                                    }
                                #else
                                    if( vlan_user == VLAN_USER_STATIC || vlan_user == VLAN_USER_ALL) {
                                        CPRINTF("%-4d  %-10s  %-39s  %-9s   %s\n", 
                                           vid, vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                  cli_iport_list_txt(mports, buf), "No", "None");
                                    } else {
                                        if( conflict ) {
                                            CPRINTF("%-4d  %-10s  %-39s  %-9s   %s\n",vid, vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                                                  cli_iport_list_txt(mports, buf), "Yes", 
                                                                                  cli_iport_list_txt(fports, conf_buf));
                                            conflict = FALSE;
                                        } else {
                                            CPRINTF("%-4d  %-10s  %-39s  %-9s   %s\n",vid, vlan_mgmt_vlan_user_to_txt(vlan_user),                                                                                                                                      cli_iport_list_txt(mports, buf), "No",
                                                                                      "None");
                                        }
                                    }
                                #endif
                                    vid_first = 0;
                                } else {
                                #ifdef  VTSS_SW_OPTION_VLAN_NAME
                                    if ( vlan_user == VLAN_USER_STATIC || vlan_user == VLAN_USER_ALL) {
                                        CPRINTF("                                        %-10s  %-39s  %-9s   %s \n",                                                                                                                        vlan_mgmt_vlan_user_to_txt(vlan_user), cli_iport_list_txt(mports, buf),
                                                            "No", "None");
                                    } else {
                                        if( conflict ) {
                                            CPRINTF("                                        %-10s  %-39s  %-9s   %s \n", 
                                                            vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                             cli_iport_list_txt(mports, buf), "Yes", 
                                                             cli_iport_list_txt(fports, conf_buf));
                                            conflict = FALSE;
                                        } else {
                                            CPRINTF("                                        %-10s  %-39s  %-9s   %s \n",
                                                                          vlan_mgmt_vlan_user_to_txt(vlan_user), 
                                                                          cli_iport_list_txt(mports, buf), 
                                                                          "No", "None");
                                        }
                                    }
                                #else
                                    if ( vlan_user == VLAN_USER_STATIC || vlan_user == VLAN_USER_ALL) {
                                        CPRINTF("      %-10s  %-39s  %-9s  %s\n", 
                                                vlan_mgmt_vlan_user_to_txt(vlan_user),
                                                cli_iport_list_txt(mports, buf), "No", "None");
                                    } else {
                                        if ( conflict ) {
                                            CPRINTF("      %-10s  %-39s  %-9s  %s\n",
                                                                 vlan_mgmt_vlan_user_to_txt(vlan_user),                                                                                                                                       cli_iport_list_txt(mports, buf), "Yes", 
                                                                 cli_iport_list_txt(fports, conf_buf));
                                            conflict = FALSE;
                                        } else {
                                            CPRINTF("      %-10s  %-39s  %-9s  %s\n", vlan_mgmt_vlan_user_to_txt(vlan_user),                                                                                                                  cli_iport_list_txt(mports, buf), "No", "None");
                                        }
                                    }
                                #endif
                                }
                                first  = 0;
                            }
                        }
                    } else {
                        if (first) {
                            cli_cmd_usid_print(usid, req, 1);
                        #ifdef  VTSS_SW_OPTION_VLAN_NAME
                            cli_table_header("VID   VLAN Name                         Ports");
                        #else
                            cli_table_header("VID   Ports");
                        #endif
                            first = 0;
                        }
                        for(iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
                            mports[iport] = (conf.ports[iport] == VLAN_MEMBER_PORT) ? TRUE : FALSE;
                        }
                #ifdef  VTSS_SW_OPTION_VLAN_NAME
                        CPRINTF("%-4d  %-32s  %s\n", 
                             vid, conf.vlan_name, cli_iport_list_txt(mports, buf));
                #else
                        CPRINTF("%-4d  %s\n", 
                                 vid, cli_iport_list_txt(mports, buf));
                #endif
                    }
                }
            }

            if (first) {
                cli_cmd_usid_print(usid, req, 1);
                if (lookup && req->vid_spec == CLI_SPEC_VAL) {                   
            #ifdef  VTSS_SW_OPTION_VLAN_NAME
                    if (vlan_req->name == FALSE)
            #endif
                        CPRINTF("VLAN ID %d not found\n", req->vid);
            #ifdef  VTSS_SW_OPTION_VLAN_NAME
                    else
                        CPRINTF("VLAN Name not found\n");
            #endif
                } else { 
                    CPRINTF("VLAN table is empty\n");
                }
            #if VTSS_SWITCH_STACKABLE
            } else {
                CPRINTF("Stack Ports are members of all VLANs\n");
        #endif /* VTSS_SWITCH_STACKABLE */
            }
        }
    }


    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END)
            continue;

        first = 1;
        if (lookup && forbidden) {
        #ifdef  VTSS_SW_OPTION_VLAN_NAME
            if ((vlan_req->vid_exists == TRUE) && (vlan_req->name == TRUE)) {
                CPRINTF("Enter either VID or VLAN Name\n");
                return ;
            }
        #endif
            /* Lookup specific vid */
            if (lookup && (req->vid_spec == CLI_SPEC_VAL) && (vlan_req->vid_exists == TRUE)) {
                vid = req->vid;
                if (vlan_mgmt_vlan_get(isid, vid, &conf, 0, mgmt_user, VLAN_FORBIDDEN_OP) == VTSS_OK) {
                    for( iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++ ) {
                        fports[iport] = (conf.ports[iport] == VLAN_FORBIDDEN_PORT) ? TRUE : FALSE;
                    }
                    if (first == 1) {
                        cli_header("VLAN forbidden port list", 0);
                        cli_cmd_usid_print(usid, req, 1);
                        #ifdef VTSS_SW_OPTION_VLAN_NAME
                        cli_table_header("VID   VLAN Name                         Ports");
                        #else
                        cli_table_header("VID   Ports");
                        #endif
                        first = 0;
                    }
                    #ifdef  VTSS_SW_OPTION_VLAN_NAME
                    CPRINTF("%-4d  %-32s  %s\n", vid, conf.vlan_name, cli_iport_list_txt(fports, buf));
                    #else
                    CPRINTF("%-4d  %s\n", vid, cli_iport_list_txt(fports, buf));
                    #endif
                }
            }
            #ifdef VTSS_SW_OPTION_VLAN_NAME 
            else if (lookup && (req->vid_spec == CLI_SPEC_VAL) && (vlan_req->vid_exists == FALSE) && (vlan_req->name == TRUE)) {
                rc = vlan_mgmt_vlan_get_by_name(isid, req->parm, &conf, 0, vlan_user, VLAN_FORBIDDEN_OP);
                for( iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++ ) {
                    fports[iport] = (conf.ports[iport] == VLAN_FORBIDDEN_PORT) ? TRUE : FALSE;
                }
                if (rc == VTSS_OK) {
                    if (first == 1) {
                        cli_header("VLAN forbidden port list", 0);
                        cli_cmd_usid_print(usid, req, 1);
                        cli_table_header("VID   VLAN Name                         Ports");
                        first = 0; 
                    }
                    CPRINTF("%-4d  %-32s  %s\n", conf.vid, conf.vlan_name,
                                                cli_iport_list_txt(fports, buf));
                } 
            }
            #endif 
            else {
                vid = VTSS_VID_NULL;
                while (vlan_mgmt_vlan_get(isid, vid, &conf, 1, mgmt_user, VLAN_FORBIDDEN_OP) == VTSS_OK) {
                    for( iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++ ) {
                        fports[iport] = (conf.ports[iport] == VLAN_FORBIDDEN_PORT) ? TRUE : FALSE;
                    }
                    T_N("called vlan_mgmt_vlan_get, lookup: %d, vid: %u," "req->vid: %u", lookup, vid, req->vid);
                    vid = conf.vid;
                    if (first) {
                        cli_header("VLAN forbidden port list", 0);
                        cli_cmd_usid_print(usid, req, 1);
                        #ifdef VTSS_SW_OPTION_VLAN_NAME
                        cli_table_header("VID   VLAN Name                         Ports");
                        #else
                        cli_table_header("VID   Ports");
                        #endif
                        first = 0;
                    }
                    #ifdef   VTSS_SW_OPTION_VLAN_NAME
                    CPRINTF("%-4d  %-32s  %s\n", vid, conf.vlan_name, cli_iport_list_txt(fports, buf));
                    #else
                    CPRINTF("%-4d  %s\n", vid, cli_iport_list_txt(fports, buf));
                    #endif
                }
            }

            if (first) {
                if (lookup && req->vid_spec == CLI_SPEC_VAL) {
                    cli_cmd_usid_print(usid, req, 1);
                    #ifdef VTSS_SW_OPTION_VLAN_NAME
                    cli_table_header("VID   VLAN Name                         Ports");
                    #else
                    cli_table_header("VID   Ports");
                    #endif
                    CPRINTF("VLAN ID %d not found\n", req->vid);
                } else {
                    cli_cmd_usid_print(usid, req, 1);
                    #ifdef VTSS_SW_OPTION_VLAN_NAME
                    cli_table_header("VID   VLAN Name                         Ports");
                    #else
                    cli_table_header("VID   Ports");
                    #endif
                    CPRINTF("VLAN forbidden table is empty\n");
                }
            #if VTSS_SWITCH_STACKABLE
            } else {
                CPRINTF("Stack Ports are members of all VLANs\n");
            #endif /* VTSS_SWITCH_STACKABLE */
            }
        }
    }
}

static void cli_cmd_debug_vlan_fill(cli_req_t *req)
{
    ushort i;

    for (i=0; i< VLAN_MAX; i++){
        cli_cmd_vlan_vid_conf(req, 1, 0, 0,0);
        req->vid++;
    }
}

static void cli_cmd_debug_vlan_empty(cli_req_t *req)
{
    ushort i;

    for (i=1; i< 4096; i++){
        if (vlan_mgmt_vlan_del(VTSS_ISID_GLOBAL, i, VLAN_USER_ALL, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
            continue;
        }
    }
}

static void cli_cmd_debug_vlan_table_lookup(cli_req_t *req)
{
    u16                 i;
    BOOL                first = 1;
    BOOL                mports[VTSS_PORT_ARRAY_SIZE];
    vlan_mgmt_entry_t   entry_conf;
    char                buf[MGMT_PORT_BUF_SIZE];
    vtss_port_no_t      iport;

    for (i = VTSS_VID_DEFAULT; i < VTSS_VIDS; i++){
        if (vlan_mgmt_vlan_get(VTSS_ISID_LOCAL, i, &entry_conf, FALSE, VLAN_USER_ALL, VLAN_MEMBERSHIP_OP) == VTSS_OK) {
            for(iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
                mports[iport] = (entry_conf.ports[iport] == VLAN_MEMBER_PORT) ? TRUE : FALSE;
            }
            if (first) {
                cli_table_header("VID   Ports");
                first = 0;
            }
            CPRINTF("%-4d  %s\n", entry_conf.vid, cli_iport_list_txt(mports, buf));
        }
    }
}

//Command Handler to get Slave VLAN port configuration.
static void cli_cmd_debug_slave_vlan_port_conf(cli_req_t *req)
{
    vlan_port_conf_t      conf;
    char                  buf[80], *p;
    port_iter_t           pit;

    if (cli_cmd_switch_none(req)) {
        return;
    }

    (void) port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (req->uport_list[pit.uport] == 0) {
            continue;
        }
        /* 
         * If isid is VTSS_ISID_LOCAL and VLAN User is VLAN_USER_ALL, vlan_mgmt_port_conf_get() 
         * gets VLAN port conf from the switch API.
         */
        if (vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, pit.iport, &conf, VLAN_USER_ALL) != VTSS_OK) {
            return;
        }
        if (pit.first) {
            cli_cmd_usid_print(0, req, 1);
            p = &buf[0];
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
            p += sprintf(p, "Port  PVID  Frame Type  UVID  "
                    "Ingress Filter  Port Type      ");
#else
            p += sprintf(p, "Port  PVID  Frame Type  UVID  Port Type      ");
#endif
            cli_table_header(buf);
        }
        CPRINTF("%-2d    ", pit.uport);
        CPRINTF("%-4d  ", conf.pvid);
        CPRINTF("%-11s ", cli_vlan_frame_type_txt(conf.frame_type));
        CPRINTF("%-4d  ", conf.untagged_vid);
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
        CPRINTF("%-16s", cli_bool_txt(conf.ingress_filter));
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
        CPRINTF("%-15s", cli_vlan_port_type_txt(conf.port_type));
        CPRINTF("\n");
    }
}
#ifdef VLAN_USER_DEBUG_CMD_ENABLE
/*Volatile VLAN Port configuration */
static void cli_cmd_debug_vlan_port_conf(cli_req_t *req, BOOL porttype, BOOL pvid, 
                                         BOOL frame_type, BOOL ingr_filt, BOOL tx_tag) 
{
    vtss_isid_t           usid;
    vtss_isid_t           isid;
    vlan_port_conf_t      conf;
    vlan_cli_req_t *      vlan_req = req->module_req;
    BOOL                  delete;
    port_iter_t           pit;

    if (!vlan_req->usr_valid) {
        T_D("Invalid VLAN User");
        return;
    }
    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END)
            continue;

        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if ((req->uport_list[pit.uport] == 0) || 
                (vlan_mgmt_port_conf_get(isid, pit.iport, &conf, vlan_req->vlan_user) != VTSS_OK)) {
                continue;
            }

            if(porttype || pvid || frame_type || ingr_filt || tx_tag) {
                delete = 0;
            } else {
                delete = 1;
            }

            if(delete) {
                conf.flags = 0;
            }

            if (porttype) {
                conf.port_type = vlan_req->port_type;
                conf.flags = (conf.flags | VLAN_PORT_FLAGS_AWARE);
            }
            if (pvid) {
#ifdef VTSS_SW_OPTION_ACL
                if (mgmt_vlan_pvid_change(isid, pit.iport, req->vid, conf.pvid) 
                        != VTSS_OK) {
                    return;
                }
#endif /* VTSS_SW_OPTION_ACL */
                conf.pvid = req->vid;
                conf.flags = (conf.flags | VLAN_PORT_FLAGS_PVID);
            }
            if (frame_type) {
                if (vlan_req->all) {
                    conf.frame_type = VTSS_VLAN_FRAME_ALL;
                } else if (vlan_req->tagged) {
                    conf.frame_type = VTSS_VLAN_FRAME_TAGGED;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
                } else if (vlan_req->untagged) {
                    conf.frame_type = VTSS_VLAN_FRAME_UNTAGGED;
#endif
                } else {
                    T_D("Invalid FrameType. Setting it to Default Value");
                    conf.frame_type = VTSS_VLAN_FRAME_ALL;
                }
                conf.flags = (conf.flags | VLAN_PORT_FLAGS_RX_TAG_TYPE);
            }
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
            if (ingr_filt) {
                conf.ingress_filter = req->enable;
                conf.flags = (conf.flags | VLAN_PORT_FLAGS_INGR_FILT);
            }
            if (tx_tag) {
                if (vlan_req->untag_this) {
                    conf.tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_THIS;
                    conf.untagged_vid = req->vid;
                } else if (vlan_req->untag_all) {
                    conf.tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_ALL;
                    conf.untagged_vid = VTSS_VID_ALL;
                } else if (vlan_req->tag_this) {
                    conf.tx_tag_type = VLAN_TX_TAG_TYPE_TAG_THIS;
                    conf.untagged_vid = req->vid;
                } else if (vlan_req->tag_all) {
                    conf.tx_tag_type = VLAN_TX_TAG_TYPE_TAG_ALL;
                    conf.untagged_vid = VTSS_VID_NULL;
                }
                conf.flags = (conf.flags | VLAN_PORT_FLAGS_TX_TAG_TYPE);
            }
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
            if (vlan_mgmt_port_conf_set(isid, pit.iport, &conf, vlan_req->vlan_user) 
                    != VTSS_OK) {
                return;
            }
        }
    }
}

static void cli_cmd_debug_vlan_porttype(cli_req_t *req)
{
    cli_cmd_debug_vlan_port_conf(req, 1, 0, 0, 0, 0);
}    

static void cli_cmd_debug_vlan_pvid(cli_req_t *req)
{
    cli_cmd_debug_vlan_port_conf(req, 0, 1, 0, 0, 0);
}                       

static void cli_cmd_debug_vlan_frame_type(cli_req_t *req)
{    
    cli_cmd_debug_vlan_port_conf(req, 0, 0, 1, 0, 0);
}               

#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
static void cli_cmd_debug_vlan_ingr_filter(cli_req_t *req)
{
    cli_cmd_debug_vlan_port_conf(req, 0, 0, 0, 1, 0);
}
#endif /*VTSS_SW_OPTION_VLAN_INGRESS_FILTERING*/

static void cli_cmd_debug_vlan_tx_tag(cli_req_t *req)
{   
    cli_cmd_debug_vlan_port_conf(req, 0, 0, 0, 0, 1);
}
static void cli_cmd_debug_vlan_del(cli_req_t *req)
{
    cli_cmd_debug_vlan_port_conf(req, 0, 0, 0, 0, 0);
}
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
static void cli_cmd_debug_vlan_counters(cli_req_t *req)
{
    vtss_vlan_counters_t c;
    vlan_cli_req_t *      vlan_req = req->module_req;
    printf("vid = %d\n", req->vid);
    memset(&c, 0, sizeof(vtss_vlan_counters_t));
    if (vlan_req->clear == 0) {
        if (vtss_vlan_counters_get(NULL, req->vid, &c) != VTSS_RC_OK) {
            CPRINTF("VLAN Counters get error\n");
            return;
        }
    } else {
        if (vtss_vlan_counters_clear(NULL, req->vid) != VTSS_RC_OK) {
            CPRINTF("VLAN Counters clear error\n");
            return;
        }
    }
    CPRINTF("uni_fr: 0x%llx\n uni_by: 0x%llx\n"
            "multi_fr: 0x%llx\n multi_by: 0x%llx\n" 
            "fl_fr: 0x%llx\n fl_by: 0x%llx\n", c.rx_vlan_unicast.frames, c.rx_vlan_unicast.bytes,
             c.rx_vlan_multicast.frames, c.rx_vlan_multicast.bytes,
             c.rx_vlan_flood.frames, c.rx_vlan_flood.bytes);
}
#endif  //VTSS_FEATURE_VLAN_COUNTERS
#if defined(VTSS_FEATURE_VLAN_TX_TAG)
static void cli_cmd_debug_vlan_tag(cli_req_t *req)
{
    port_iter_t        pit;
    vtss_vlan_tx_tag_t tx_tag[VTSS_PORT_ARRAY_SIZE];
    BOOL               first = 1;
    
    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
    if (vtss_vlan_tx_tag_get(NULL, req->vid, tx_tag) != VTSS_OK)
        return;

    while (port_iter_getnext(&pit)) {
        if (req->uport_list[pit.uport] == 0) {
            continue;
        }
        if (req->set) {
            tx_tag[pit.iport] = (req->enable ? VTSS_VLAN_TX_TAG_ENABLE :
                                 req->disable ? VTSS_VLAN_TX_TAG_DISABLE : VTSS_VLAN_TX_TAG_PORT);
        } else {
            if (first) {
                first = 0;
                cli_table_header("Port  Tx Tag");
            }
            CPRINTF("%-6u%s\n", pit.uport, 
                    tx_tag[pit.iport] == VTSS_VLAN_TX_TAG_PORT ? "Port" : 
                    cli_bool_txt(tx_tag[pit.iport] == VTSS_VLAN_TX_TAG_ENABLE));
        }
    }
    if (req->set) {
        (void)vtss_vlan_tx_tag_set(NULL, req->vid, tx_tag);
    }
}
#endif  //VTSS_FEATURE_VLAN_TX_TAG
#endif  //VLAN_USER_DEBUG_CMD_ENABLE
static void cli_cmd_vlan_conf(cli_req_t *req)
{
    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

    if (!req->set) {
        cli_header("VLAN Configuration", 1);
    }

    cli_cmd_vlan_port_conf(req, 1, 1, 1, 1, 1);
    cli_cmd_vlan_vid_conf(req, 0, 0, 1,0);
    cli_cmd_vlan_vid_conf(req, 0, 0, 1,1);
}

static void cli_cmd_vlan_pvid_conf(cli_req_t *req)
{
    cli_cmd_vlan_port_conf(req, 1, 0, 0, 0, 0);
}

static void cli_cmd_vlan_frame_type(cli_req_t *req)
{
    cli_cmd_vlan_port_conf(req, 0, 1, 0, 0, 0);
}

#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
static void cli_cmd_vlan_ingr_filter(cli_req_t *req)
{
    cli_cmd_vlan_port_conf(req, 0, 0, 1, 0, 0);
}
#endif /*VTSS_SW_OPTION_VLAN_INGRESS_FILTERING*/

static void cli_cmd_vlan_tx_tag(cli_req_t *req)
{
    cli_cmd_vlan_port_conf(req, 0, 0, 0, 0, 1);
}

static void cli_cmd_vlan_port_type(cli_req_t *req)
{
    cli_cmd_vlan_port_conf(req, 0, 0, 0, 1, 0);
}
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
static void cli_cmd_vlan_tpid(cli_req_t *req)
{
    vlan_cli_req_t *      vlan_req = req->module_req;
    vtss_etype_t          tpid;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }
    if (req->set) {
        if (vlan_mgmt_vlan_tpid_set(vlan_req->tpid) != VTSS_RC_OK) {
            CPRINTF("Unable to set TPID\n");
        }
    } else {
        if (vlan_mgmt_vlan_tpid_get(&tpid) != VTSS_RC_OK) {
            CPRINTF("Unable to get TPID\n");
        } else {
            CPRINTF("TPID is 0x%x\n", tpid);
        }
    }
}
static void cli_cmd_debug_vlan_tpid(cli_req_t *req)
{
    vtss_etype_t          tpid;

    if (vlan_mgmt_debug_vlan_tpid_get(&tpid) != VTSS_RC_OK) {
        CPRINTF("Unable to get TPID\n");
    } else {
        CPRINTF("TPID is 0x%x\n", tpid);
    }
}
#endif
static void cli_cmd_vlan_add(cli_req_t *req)
{
    cli_cmd_vlan_vid_conf(req, 1, 0, 0,0);
}

static void cli_cmd_forbidden_vlan_add(cli_req_t *req)
{
    cli_cmd_vlan_vid_conf(req, 1, 0, 0,1);
}


static void cli_cmd_debug_vlan_add(cli_req_t *req)
{
    vlan_cli_req_t *      vlan_req = req->module_req;
    vlan_req->is_vid = TRUE;
    cli_cmd_vlan_vid_conf(req, 1, 0, 0,0);
}

static void cli_cmd_vlan_delete(cli_req_t *req)
{
    cli_cmd_vlan_vid_conf(req, 0, 1, 0,0);
}

static void cli_cmd_vlan_forbidden_delete(cli_req_t *req)
{
    cli_cmd_vlan_vid_conf(req, 0, 1, 0,1);
}

static void cli_cmd_debug_vlan_delete(cli_req_t *req)
{
    vlan_cli_req_t *      vlan_req = req->module_req;
    vlan_req->is_vid = TRUE;
    cli_cmd_vlan_vid_conf(req, 0, 1, 0,0);
}

static void cli_cmd_vlan_lookup(cli_req_t *req)
{
    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

    cli_cmd_vlan_vid_conf(req, 0, 0, 1,0);
}

static void cli_cmd_vlan_forbidden_lookup(cli_req_t *req)
{
    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

    cli_cmd_vlan_vid_conf(req, 0, 0, 1,1);
}


#ifdef  VTSS_SW_OPTION_VLAN_NAME
static void cli_cmd_vlan_name_add(cli_req_t *req)
{
    vtss_rc                     rc;
    vlan_mgmt_vlan_name_conf_t  conf;

    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }
    conf.vid = req->vid;
    strcpy(conf.vlan_name, req->parm);
    rc = vlan_mgmt_vlan_name_add(&conf, 0);
    if (rc == VLAN_ERROR_VLAN_NAME_PREVIOUSLY_CONFIGURED) {
        CPRINTF("Error: VLAN Name is already used for different VID\n");
    } else if(rc == VLAN_ERROR_VLAN_NAME_TABLE_FULL) {
        CPRINTF("Error: VLAN Name table Full. Only %d entries can be added\n", VLAN_NAME_MAX_CNT);
    }
}

static void cli_cmd_vlan_name_del(cli_req_t *req)
{
    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }
    if (vlan_mgmt_vlan_name_del(req->parm) != VTSS_RC_OK) {
        CPRINTF("VLAN Name deletion failed.\n");
    }
}

static void cli_cmd_vlan_name_lookup(cli_req_t *req)
{
    vlan_mgmt_vlan_name_conf_t  conf;
    BOOL                        first = TRUE;
    u16                         temp;
    vlan_cli_req_t              *vlan_req = req->module_req;
    i8                          buf[]="VLAN NAME                        vid";

    if (vlan_req->single_name == FALSE) {
        conf.name_valid = FALSE;
        for (temp = 0; temp < VLAN_NAME_MAX_CNT; temp++) {
            if (vlan_mgmt_vlan_name_get(temp, &conf) == VTSS_RC_OK) {
                if (first) {
                    cli_table_header(buf);
                    first = FALSE;
                }
                CPRINTF("%-32s %u\n", conf.vlan_name, conf.vid);
            }
        }
    } else {
        strcpy(conf.vlan_name, req->parm);
        conf.name_valid = TRUE;
        if (vlan_mgmt_vlan_name_get(0, &conf) == VTSS_RC_OK) {
            cli_table_header(buf);
            CPRINTF("%-32s %u\n", conf.vlan_name, conf.vid);
        } else {
            CPRINTF("Error: entry with VLAN name %s not found\n", req->parm);
        }
    }
}
#endif

static void cli_cmd_vlan_port_status(cli_req_t *req)
{
    vtss_isid_t           usid;
    vtss_isid_t           isid;
    BOOL                  disp_port;
    char                  buf[200], *p;
    vlan_port_conf_t      conf;
    vlan_cli_req_t        *vlan_req = req->module_req;
    vlan_user_t           usr, first_user, last_user;
    vlan_port_conflicts_t conflicts;
    u8                    all_usrs = 0;
    vlan_port_flags_idx_t temp;
    port_iter_t           pit;
    BOOL                  first = 1;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req))
        return; 

    //Default User is "combined".
    if(!vlan_req->usr_valid) {
        vlan_req->all = 1;
    }
    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END)
            continue;
        first = 1;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0) {
                continue;
            }
            //This is needed for displaying "all" VLAN Users port properties.
            if (vlan_req->all) {
                first_user = VLAN_USER_STATIC;
                last_user = VLAN_USER_ALL;
            } else {
                first_user = vlan_req->vlan_user;
                last_user = vlan_req->vlan_user;
            }
            disp_port = 1;
            if (vlan_mgmt_conflicts_get(&conflicts, isid, pit.iport) != VTSS_OK) {
                T_D("vlan_mgmt_conflicts_get FAILED");
                return;
            }
            //Collect all Conflicting VLAN Users.
            for (temp = VLAN_PORT_PVID_IDX; temp <= VLAN_PORT_TX_TAG_TYPE_IDX; 
                    temp++) {
                all_usrs |= conflicts.vlan_users[temp];
            }
            for (usr = first_user; usr <= last_user; usr++) {
                if (vlan_mgmt_port_conf_get(isid, pit.iport, &conf, usr) 
                        != VTSS_OK) {
                    continue;
                }
                if (first) {
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    if (vlan_req->all) {
                        p += sprintf(p, "VLAN User    ");
                    }
                    p += sprintf(p, "PortType       ");
                    p += sprintf(p, "PVID  ");
                    p += sprintf(p, "Frame Type  ");
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                    p += sprintf(p, "Ing Filter   ");
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
                    p += sprintf(p, "Tx Tag       ");
                    p += sprintf(p, "UVID   ");
                    if (last_user != VLAN_USER_STATIC) {
                        p += sprintf(p, "Conflicts");
                    }
                    cli_table_header(buf);
                    first = 0;
                }
                if (!(vlan_req->conflicts_disp) || conflicts.port_flags) {
                    if (disp_port) {
                        CPRINTF("%-2d    ", pit.uport);
                        disp_port = 0;
                    }
                    if (vlan_req->all) {
                        if (usr != VLAN_USER_STATIC) {
                            CPRINTF("      ");
                        }
                        CPRINTF("%-10s   ", vlan_mgmt_vlan_user_to_txt(usr));
                    }
                    if (conf.flags & VLAN_PORT_FLAGS_AWARE) {
                        CPRINTF("%-13s  ", cli_vlan_port_type_txt(conf.port_type));
                    } else {
                        CPRINTF("               ");
                    }
                    if (conf.flags & VLAN_PORT_FLAGS_PVID) {
                        if (conf.pvid == 0)
                            CPRINTF("None  ");
                        else
                            CPRINTF("%-4d  ", conf.pvid);
                    } else {
                        CPRINTF("      ");
                    }

                    if (conf.flags & VLAN_PORT_FLAGS_RX_TAG_TYPE) {
                        CPRINTF("%-11s ", 
                                cli_vlan_frame_type_txt(conf.frame_type));
                    } else {
                        CPRINTF("            ");
                    }
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
                    if (conf.flags & VLAN_PORT_FLAGS_INGR_FILT) {
                        CPRINTF("%-12s ", cli_bool_txt(conf.ingress_filter));
                    } else {
                        CPRINTF("             ");
                    }
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
                    if (conf.flags & VLAN_PORT_FLAGS_TX_TAG_TYPE) { 
                        CPRINTF("%-12s ", 
                                cli_vlan_tag_type_txt(conf.tx_tag_type));
                        CPRINTF("%-4d ", conf.untagged_vid); 
                    } else {
                        CPRINTF("                  ");
                    }

                    if (last_user != VLAN_USER_STATIC) {
                        if (usr == VLAN_USER_STATIC) {
                            CPRINTF("%s ", "     ");
                        } else {
                            if (conflicts.port_flags == 0) {
                                CPRINTF("%s ", "  No"); 
                            } else {
                                if (all_usrs & (1 << (u8)usr)) {
                                    CPRINTF("%s ", "  Yes"); 
                                } else {
                                    CPRINTF("%s ", "  No"); 
                                }
                            }
                        }
                    }
                    CPRINTF("\n");
                }
            }
        }
    }
}


static int32_t cli_vlan_vid_none_parse (char *cmd, char *cmd2, char *stx, 
        char *cmd_org, cli_req_t *req)
{
    i32 error = 0;
    ulong value = 0;

    req->parm_parsed = 1;
    error = (cli_parse_ulong(cmd, &value, VTSS_VID_DEFAULT, VTSS_VID_RESERVED) 
            &&
            cli_parm_parse_keyword(cmd, cmd2, stx, cmd_org, req));
    req->vid = value;

    return error;
}

static int32_t cli_vlan_vid_parse (char *cmd, char *cmd2, char *stx, 
        char *cmd_org, cli_req_t *req)
{
    i32 error = 0;
    ulong value = 0;
    vlan_cli_req_t *vlan_req = req->module_req;

    req->parm_parsed = 1;
    vlan_req->vlan_membership_user = VLAN_USER_STATIC;
#ifdef  VTSS_SW_OPTION_VLAN_NAME
    if (strncmp("name", cmd, strlen(cmd))) {
        vlan_req->vid_exists = TRUE;
    }
#endif
    vlan_req->vlan_cmd_all = 1;
    error = cli_parse_ulong(cmd, &value, VTSS_VID_DEFAULT, VTSS_VID_RESERVED);
    if (!error) {
        req->vid_spec = CLI_SPEC_VAL;
        req->vid = value;
    }

    return error;
}

#ifdef  VTSS_SW_OPTION_VLAN_NAME
static int32_t cli_vlan_vid_name_parse(char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    vlan_cli_req_t *vlan_req = req->module_req;
    u32            i;
    int32_t        error = 0;

    T_D("cli_vlan_vid_name_parse: Enter\n");
    error = cli_parse_text(cmd_org, req->parm, VLAN_NAME_MAX_LEN);
    if (error != 0) {
        return VTSS_INVALID_PARAMETER;
    }
    if (strlen(req->parm) > VLAN_NAME_MAX_LEN) {
        T_D("cli_vlan_vid_name_parse: VTSS_INVALID_PARAMETER\n");
        return VTSS_INVALID_PARAMETER;
    }
    req->vid_spec = CLI_SPEC_VAL;
    vlan_req->is_vid = TRUE;
    for (i=0; i < strlen(req->parm); i++) {
        if ((!isalpha(req->parm[i])) && (!isdigit(req->parm[i]))) {
            CPRINTF("Only alphabets and numbers are allowed in VLAN name. At least one alphabet must be present\n");
            return VTSS_RC_ERROR;
        }
        if (isalpha(req->parm[i])) {
            vlan_req->is_vid = FALSE;
        }
    }
    if (vlan_req->is_vid == TRUE) {
        T_D("cli_vlan_vid_name_parse: vid entered\n");
        req->vid = atoi(req->parm);
        if ((req->vid < VTSS_VID_DEFAULT) || (req->vid > VTSS_VID_RESERVED)) {
            return VTSS_INVALID_PARAMETER;
        }
    }
    return VTSS_OK; 
}
#endif

#ifdef  VTSS_SW_OPTION_VLAN_NAME
static int32_t vlan_cli_name_parse(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t         error = 0;
    u32             i, num_digits = 0;
    vlan_cli_req_t  *vlan_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_text(cmd_org, req->parm, VLAN_NAME_MAX_LEN);
    for (i=0; i < strlen(req->parm); i++) {
        if ((!isalpha(req->parm[i])) && (!isdigit(req->parm[i]))) {
            CPRINTF("Only alphabets and numbers are allowed in VLAN name. At least one alphabet must be present\n");
            return VTSS_RC_ERROR;
        }
        if (isdigit(req->parm[i])) {
            num_digits++;
        }
    }
    /* There should be at least one alphabet in the user input, else return error */
    if (num_digits == i) {
        CPRINTF("Enter at least one alphabetic character in name string\n");
        error = 1;
    }
    req->vid_spec = CLI_SPEC_VAL;
    vlan_req->single_name = TRUE;

    return error;
}
#endif

static int32_t cli_vlan_parse_keyword (char *cmd, char *cmd2, char *stx,
        char *cmd_org, cli_req_t *req)
{
    vlan_cli_req_t * vlan_req = NULL;
    char *found = cli_parse_find(cmd, stx);

    req->parm_parsed = 1;
    vlan_req = req->module_req;

    if (found != NULL) {
        if (!strncmp(found, "static",6)) {
            vlan_req->vlan_user = VLAN_USER_STATIC;
            vlan_req->usr_valid = 1;
#ifdef VTSS_SW_OPTION_DOT1X
        } else if (!strncmp(found, "nas", 3)) {
            vlan_req->vlan_user = VLAN_USER_DOT1X;
            vlan_req->usr_valid = 1;
#endif
#ifdef VTSS_SW_OPTION_MVR
        } else if (!strncmp(found, "mvr",3)) {
            vlan_req->vlan_user = VLAN_USER_MVR;
            vlan_req->usr_valid = 1;
#endif





#ifdef VTSS_SW_OPTION_VOICE_VLAN
        } else if (!strncmp(found, "voice_vlan",10)) {
            vlan_req->vlan_user = VLAN_USER_VOICE_VLAN;
            vlan_req->usr_valid = 1;
#endif
#ifdef VTSS_SW_OPTION_MSTP
        } else if (!strncmp(found, "mstp",4)) {
            vlan_req->vlan_user = VLAN_USER_MSTP;
            vlan_req->usr_valid = 1;
#endif
#ifdef VTSS_SW_OPTION_ERPS
        } else if (!strncmp(found, "erps",4)) {
            vlan_req->vlan_user = VLAN_USER_ERPS;
            vlan_req->usr_valid = 1;
#endif
#ifdef VTSS_SW_OPTION_MEP
        } else if (!strncmp(found, "mep",4)) {
            vlan_req->vlan_user = VLAN_USER_MEP;
            vlan_req->usr_valid = 1;
#endif
#ifdef VTSS_SW_OPTION_EVC
        } else if (!strncmp(found, "evc",3)) {
            vlan_req->vlan_user = VLAN_USER_EVC;
            vlan_req->usr_valid = 1;
#endif
#ifdef VTSS_SW_OPTION_VCL
        } else if (!strncmp(found, "vcl",3)) {
            vlan_req->vlan_user = VLAN_USER_VCL;
            vlan_req->usr_valid = 1;
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        } else if (!strncmp(found, "rspan",5)) {
            vlan_req->vlan_user = VLAN_USER_RSPAN;
            vlan_req->usr_valid = 1;
#endif
        } else if (!strncmp(found, "combined",8)) {
            vlan_req->vlan_user = VLAN_USER_ALL;
            vlan_req->usr_valid = 1;
        } else if (!strncmp(found, "all",3)) {
            vlan_req->all = 1;
            vlan_req->usr_valid = 1;
        } else if (!strncmp(found, "tagged",6)) {
            vlan_req->tagged = 1;
        } else if (!strncmp(found, "untagged",8)) {
            vlan_req->untagged = 1;
        } else if (!strncmp(found, "conflicts",9)) {
            vlan_req->conflicts_disp = 1;
            vlan_req->all = 1;
        } else if (!strncmp(found, "tag_this",8)) {
            vlan_req->tag_this = 1;
        } else if (!strncmp(found, "untag_this",10)) {
            vlan_req->untag_this = 1;
        } else if (!strncmp(found, "tag_all",7)) {
            vlan_req->tag_all = 1;
        } else if (!strncmp(found, "untag_all",9)) {
            vlan_req->untag_all = 1;
        } else if (!strncmp(found, "untag_pvid",10)) {
            vlan_req->untag_pvid = 1;
#ifdef VTSS_SW_OPTION_VLAN_NAME
        } else if (!strncmp(found, "name",4)) {
            vlan_req->name = 1;
#endif
        } else if (!strncmp(found, "unaware",7)) {
            vlan_req->port_type = VLAN_PORT_TYPE_UNAWARE;
        } else if (!strncmp(found, "c-port",6)) {
            vlan_req->port_type = VLAN_PORT_TYPE_C;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        } else if (!strncmp(found, "s-port",6)) {
            vlan_req->port_type = VLAN_PORT_TYPE_S;
        } else if (!strncmp(found, "s-custom-port",13)) {
            vlan_req->port_type = VLAN_PORT_TYPE_S_CUSTOM;
#endif
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
        } else if (!strncmp(found, "clear",5)) {
            vlan_req->clear = 1;
#endif /* VTSS_FEATURE_VLAN_COUNTERS */
        }
    }
    return (found == NULL ? 1 : 0);
}

static int32_t cli_vlan_membership_parse_keyword(char *cmd, char *cmd2, char *stx, 
        char *cmd_org, cli_req_t *req)
{
    char *found = cli_parse_find(cmd, stx);
    vlan_cli_req_t *vlan_req = req->module_req;

    req->parm_parsed = 1;
    vlan_req->vlan_membership_user = VLAN_USER_STATIC;
    vlan_req->vlan_cmd_all = 0;
    vlan_req->usr_valid = 0;
    if (found != NULL) {
        if (!strncmp(found, "combined", 8)) {
            vlan_req->vlan_membership_user = VLAN_USER_ALL;
            vlan_req->usr_valid = 1;
        }
        else if (!strncmp(found, "static", 6)) {
            vlan_req->vlan_membership_user = VLAN_USER_STATIC;
            vlan_req->usr_valid = 1;
        }
#ifdef VTSS_SW_OPTION_DOT1X
        else if (!strncmp(found, "nas", 3)) {
            vlan_req->vlan_membership_user = VLAN_USER_DOT1X;
            vlan_req->usr_valid = 1;
        }
#endif /* VTSS_SW_OPTION_DOT1X */






#ifdef VTSS_SW_OPTION_MVR
        else if (!strncmp(found, "mvr", 3)) {
            vlan_req->vlan_membership_user = VLAN_USER_MVR;
            vlan_req->usr_valid = 1;
        }
#endif /* VTSS_SW_OPTION_MVR */
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        else if (!strncmp(found, "voice_vlan", 10)) {
            vlan_req->vlan_membership_user = VLAN_USER_VOICE_VLAN;
            vlan_req->usr_valid = 1;
        }
#endif /* VTSS_SW_OPTION_VOICE_VLAN */
#ifdef VTSS_SW_OPTION_EVC
        else if (!strncmp(found, "evc", 3)) {
            vlan_req->vlan_membership_user = VLAN_USER_EVC;
            vlan_req->usr_valid = 1;
        }
#endif /* VTSS_SW_OPTION_EVC */
#ifdef VTSS_SW_OPTION_RSPAN
        else if (!strncmp(found, "rspan", 5)) {
            vlan_req->vlan_membership_user = VLAN_USER_RSPAN;
            vlan_req->usr_valid = 1;
        }
#endif /* VTSS_SW_OPTION_EVC */

        else if (!strncmp(found, "all", 3)) {
            vlan_req->vlan_membership_user = VLAN_USER_ALL;
            vlan_req->vlan_cmd_all = 1;
            vlan_req->usr_valid = 1;
        }
    }

    return (found == NULL ? 1 : 0);
}

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
static int32_t cli_vlan_etype_parse(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int             error = 0;
    ulong           value = 0;
    vlan_cli_req_t  *vlan_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0x600, 0xFFFF);
    if (!error) {
        vlan_req->tpid = value;
    }

    return(error);
}
#endif

#ifdef VLAN_USER_DEBUG_CMD_ENABLE
#if defined(VTSS_FEATURE_VLAN_TX_TAG)
int cli_vlan_parse_tag(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    char *found = cli_parse_find(cmd, stx);

    if (found != NULL) {
        if (*found == 'e')
            req->enable = 1;
        else if (*found == 'd')
            req->disable = 1;
    }
    return (found == NULL ? 1 : 0);
}
#endif /* VTSS_FEATURE_VLAN_TX_TAG */
#endif  //VLAN_USER_DEBUG_CMD_ENABLE

int cli_parm_parse_ports_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int             error = 0;
    ulong           min = 1, max = VTSS_PORTS;
    vlan_cli_req_t  *vlan_req = req->module_req;
    u32             port;

    req->parm_parsed = 1;
    memset(vlan_req->uports_list, 0, sizeof(vlan_req->uports_list));
    if (!cli_parse_all(cmd)) {
        /* If "all" is selected, select all ports */
        for (port = 1; port <= VTSS_PORT_ARRAY_SIZE; port++) {
            vlan_req->uports_list[port] = 1;
        }
    } else {
        error = cli_parm_parse_list(cmd, vlan_req->uports_list, min, max, 0);
    }
    return(error);
}

static cli_parm_t vlan_cli_parm_table[] = {
    {
        "<vid>|none",
        "Port VLAN ID (1-4095) or 'none', default: Show port VLAN ID",
        CLI_PARM_FLAG_SET,
        cli_vlan_vid_none_parse,
        cli_cmd_vlan_pvid_conf
    },
    {
        "<vid>",
        "VLAN ID (1-4095), default: Show all VLANs",
        CLI_PARM_FLAG_SET,
        cli_vlan_vid_parse,
        cli_cmd_vlan_lookup
    },
    {
        "<vid>",
        "VLAN ID (1-4095), default: Show all VLANs",
        CLI_PARM_FLAG_SET,
        cli_vlan_vid_parse,
        cli_cmd_vlan_forbidden_lookup
    },
#ifdef VTSS_SW_OPTION_VLAN_NAME
    {
        "<vid>",
        "VLAN ID (1-4095)",
        CLI_PARM_FLAG_SET,
        cli_vlan_vid_parse,
        cli_cmd_vlan_name_add
    },
    {
        "<vid>|<name>",
        "VLAN ID (1-4095) or VLAN Name",
        CLI_PARM_FLAG_NONE,
        cli_vlan_vid_name_parse,
        NULL
    },
#endif
    {
        "<vid>",
        "Start VLAN ID (1-(4095-MaxVLAN)), default: 1",
        CLI_PARM_FLAG_NONE,
        cli_vlan_vid_parse,
        cli_cmd_debug_vlan_fill
    },
#ifdef  VLAN_USER_DEBUG_CMD_ENABLE
    {
        "<vid>",
        "Start VLAN ID (1-(4095-MaxVLAN)), default: 1",
        CLI_PARM_FLAG_NONE,
        cli_vlan_vid_parse,
        cli_cmd_debug_vlan_pvid 
    },
    {
        "<vid>",
        "Start VLAN ID (1-(4095-MaxVLAN)), default: 1",
        CLI_PARM_FLAG_NONE,
        cli_vlan_vid_parse,
        cli_cmd_debug_vlan_tx_tag
    },
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
    {
        "all|tagged",
        "all        : Allow tagged and untagged frames\n"
            "tagged     : Allow tagged frames only\n",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        cli_cmd_debug_vlan_frame_type
    },
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    {
        "all|tagged|untagged",
        "all          : Allow tagged and untagged frames\n"
            "tagged       : Allow tagged frames only\n"
            "untagged     : Allow untagged frames only\n",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        cli_cmd_debug_vlan_frame_type
    },

#endif
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
    {
        "all|tagged",
        "all          : Allow tagged and untagged frames\n"
            "tagged       : Allow tagged frames only\n"
            "(default: Show accepted frame types)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        cli_cmd_vlan_frame_type
    },
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    {
        "all|tagged|untagged",
        "all          : Allow tagged and untagged frames\n"
            "tagged       : Allow tagged frames only\n"
            "untagged     : Allow untagged frames only\n"
            "(default: Show accepted frame types)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        cli_cmd_vlan_frame_type
    },
#endif
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
    {
        "enable|disable",
        "enable     : Enable VLAN ingress filtering\n"
            "disable    : Disable VLAN ingress filtering\n"
            "(default: Show VLAN ingress filtering)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        cli_cmd_vlan_ingr_filter
    },
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
#ifdef  VLAN_USER_DEBUG_CMD_ENABLE
    {
        "enable|disable",
        "enable     : Enable the VLAN Ingress Filter\n"
            "disable    : Disable the VLAN Ingress Filter\n",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        cli_cmd_debug_vlan_ingr_filter
    },
#endif
    {
        "combined|static"
#ifdef VTSS_SW_OPTION_DOT1X
        "|nas"
#endif
#ifdef VTSS_SW_OPTION_MVR
        "|mvr"
#endif



#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "|voice_vlan"
#endif
#ifdef VTSS_SW_OPTION_MSTP
        "|mstp"
#endif
#ifdef VTSS_SW_OPTION_ERPS
        "|erps"
#endif
#ifdef VTSS_SW_OPTION_MEP
        "|mep"
#endif
#ifdef VTSS_SW_OPTION_VCL
        "|vcl"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "|rspan"
#endif
        "|all|conflicts",
        "combined        : combined VLAN Users configuration\n"
        "static          : static port configuration\n"
#ifdef VTSS_SW_OPTION_DOT1X
        "nas             : NAS port configuration\n"
#endif
#ifdef VTSS_SW_OPTION_MVR
        "mvr             : MVR port configuration\n"
#endif



#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "voice_vlan      : Voice VLAN port configuration\n"
#endif
#ifdef VTSS_SW_OPTION_MSTP
        "mstp            : MSTP port configuration\n"
#endif
#ifdef VTSS_SW_OPTION_ERPS
        "erps            : ERPS port configuration\n"
#endif
#ifdef VTSS_SW_OPTION_MEP
        "mep             : MEP port configuration\n"
#endif
#ifdef VTSS_SW_OPTION_VCL
        "vcl             : VCL port configuration\n"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "rspan      : RSPAN port configuration\n"
#endif
        "all             : All VLAN Users configuration\n"
        "(default: all VLAN Users configuration)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        cli_cmd_vlan_port_status
    },
#ifdef  VLAN_USER_DEBUG_CMD_ENABLE
    {
        VLAN_users_names,
#ifdef  VTSS_SW_OPTION_DOT1X
        "nas             : NAS port configuration\n"
#endif
#ifdef  VTSS_SW_OPTION_MVR
        "mvr             : MVR port configuration\n"
#endif



#ifdef  VTSS_SW_OPTION_VOICE_VLAN
        "voice_vlan      : Voice VLAN port configuration\n"
#endif
#ifdef  VTSS_SW_OPTION_MSTP
        "mstp            : MSTP port configuration\n"
#endif
#ifdef VTSS_SW_OPTION_ERPS
        "erps            : ERPS port configuration\n"
#endif
#ifdef  VTSS_SW_OPTION_VCL
        "vcl             : VCL port configuration\n"
#endif
#ifdef  VTSS_SW_OPTION_RSPAN
        "rspan      : RSPAN port configuration\n"
#endif
        "\n",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        NULL
    },
#endif
    {
        "combined|static"
#ifdef VTSS_SW_OPTION_DOT1X
        "|nas"
#endif
#ifdef VTSS_SW_OPTION_MVR
        "|mvr"
#endif



#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "|voice_vlan"
#endif
#ifdef VTSS_SW_OPTION_EVC
        "|evc"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "|rspan"
#endif
        "|all",
        "combined   : Shows All the Combined VLAN database\n"
        "static     : Shows the VLAN entries configured by the administrator\n"
#ifdef VTSS_SW_OPTION_DOT1X
        "nas        : Shows the VLANs configured by NAS\n"
#endif



#ifdef VTSS_SW_OPTION_MVR
        "mvr        : Shows the VLANs configured by MVR\n"
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "voice_vlan : Shows the VLANs configured by Voice VLAN\n"
#endif
#ifdef VTSS_SW_OPTION_EVC
        "evc        : Shows the VLANs configured by EVC\n"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "rspan : Shows the VLANs configured by RSPAN\n"
#endif
        "all        : Shows all VLANs configuration\n"
        "(default: combined VLAN Users configuration)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_membership_parse_keyword,
        NULL
    },
    {
        "static"
#ifdef VTSS_SW_OPTION_DOT1X
        "|nas"
#endif
#ifdef VTSS_SW_OPTION_MVR
        "|mvr"
#endif



#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "|voice_vlan"
#endif
#ifdef VTSS_SW_OPTION_EVC
        "|evc"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "|rspan"
#endif
        ,
        "static     : Shows the VLAN entries configured by the administrator\n"
#ifdef VTSS_SW_OPTION_DOT1X
        "nas        : Shows the VLANs configured by NAS\n"
#endif



#ifdef VTSS_SW_OPTION_MVR
        "mvr        : Shows the VLANs configured by MVR\n"
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "voice_vlan : Shows the VLANs configured by Voice VLAN\n"
#endif
#ifdef VTSS_SW_OPTION_EVC
        "evc        : Shows the VLANs configured by EVC\n"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "rspan : Shows the VLANs configured by RSPAN\n"
#endif
        ,
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_membership_parse_keyword,
        NULL
    },
#ifdef  VLAN_USER_DEBUG_CMD_ENABLE
    {
        "tag_this|untag_this|tag_all|untag_all",
        "Tag Type",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        cli_cmd_debug_vlan_tx_tag
    },
#endif
    {
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        "unaware|c-port|s-port|s-custom-port",
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
        "unaware|c-port",
#endif
        "Port Type",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        NULL
    },
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    {
        "<etype>",
        "Ether Type (0x0600-0xFFFF)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_etype_parse,
        cli_cmd_vlan_tpid
    },

#endif
#ifdef VTSS_SW_OPTION_VLAN_NAME
    {
        "<name>",
        "VLAN name - Maximum of 32 characters. VLAN Name can only contain alphabets or numbers.\n"
        "VLAN name should contain at least one alphabet.",
        CLI_PARM_FLAG_NONE,
        vlan_cli_name_parse,
        NULL
    },
    {
        "name",
        "VLAN name string",
        CLI_PARM_FLAG_NONE,
        cli_vlan_parse_keyword,
        cli_cmd_vlan_lookup
    },
    {
        "name",
        "VLAN name string",
        CLI_PARM_FLAG_NONE,
        cli_vlan_parse_keyword,
        cli_cmd_vlan_forbidden_lookup
    },
#endif
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    {
        "clear",
        "Clear VLAN Counters",
        CLI_PARM_FLAG_NONE,
        cli_vlan_parse_keyword,
        NULL
    },
#endif /* VTSS_FEATURE_VLAN_COUNTERS */
    {
        "untag_pvid|untag_all|tag_all",
        "Tx tag      :  (Egress tagging)\n"
        "untag_pvid  :  All VLANs except pvid will be tagged\n"
        "untag_all   :  All VLANs will be untagged\n"
        "tag_all     :  All VLANs will be tagged\n",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_vlan_parse_keyword,
        cli_cmd_vlan_tx_tag        
    },
    {
        "<ports_list>",
        "Ports list. By default none of the ports are selected. To select all ports, use 'all' keyword",
        CLI_PARM_FLAG_NONE,
        cli_parm_parse_ports_list,
        NULL
    },
#ifdef VLAN_USER_DEBUG_CMD_ENABLE
#if defined(VTSS_FEATURE_VLAN_TX_TAG)
    {
        "<vid>",
        "VLAN ID (1-4095)",
        CLI_PARM_FLAG_NONE,
        cli_parm_parse_vid,
        cli_cmd_debug_vlan_tag
    },
    {
        "port|enable|disable",
        "Egress tag type",
        CLI_PARM_FLAG_SET,
        cli_vlan_parse_tag,
        cli_cmd_debug_vlan_tag
    },
#endif /* VTSS_FEATURE_VLAN_TX_TAG */
#endif  //VLAN_USER_DEBUG_CMD_ENABLE
    {NULL, NULL, 0, 0, NULL}
};

/* Vlan CLI Command sorting order */
enum {
    PRIO_VLAN_CONF,
    PRIO_VLAN_PVID,
    PRIO_VLAN_FRAME_TYPE,
    PRIO_VLAN_INGR_FILTER,
    PRIO_VLAN_TX_TAG,
    PRIO_VLAN_PORT_TYPE,
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    PRIO_VLAN_TPID_TYPE,
#endif
    PRIO_VLAN_ADD,
    PRIO_VLAN_DEL,
    PRIO_VLAN_LOOKUP,
#ifdef VTSS_SW_OPTION_VLAN_NAME
    PRIO_VLAN_NAME_ADD,
    PRIO_VLAN_NAME_DEL,
    PRIO_VLAN_NAME_LOOKUP,
#endif
    PRIO_VLAN_PORT_STATUS,
    PRIO_DEBUG_VLAN_FILL_UP  = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_EMPTY  = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_ADD  = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_DELETE  = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_TABLE_LOOKUP  = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_TPID  = CLI_CMD_SORT_KEY_DEFAULT,
	PRIO_DEBUG_VLAN_PORTTYPE = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_PVID = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_FRAME_TYPE = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_INGR_FILTER = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_TX_TAG = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_DEL = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_PORT_SLAVE = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_PORT_CONF = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_VLAN_TAG = CLI_CMD_SORT_KEY_DEFAULT,
};

/* Command table entries */
cli_cmd_tab_entry (
    "VLAN Configuration [<port_list>]",
    NULL,
    "Show VLAN configuration",
    PRIO_VLAN_CONF,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_conf,
    vlan_cli_req_conf_default_set,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_SYS_CONF
);
cli_cmd_tab_entry (
    "VLAN PVID [<port_list>]",
    "VLAN PVID [<port_list>] [<vid>|none]",
    "Set or show the port VLAN ID",
    PRIO_VLAN_PVID,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_pvid_conf,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
CLI_CMD_TAB_ENTRY_DECL(cli_cmd_vlan_frame_type) = {
    "VLAN FrameType [<port_list>]",
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
    "VLAN FrameType [<port_list>] [all|tagged]",
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    "VLAN FrameType [<port_list>] [all|tagged|untagged]",
#endif
    "Set or show the port VLAN frame type",
    PRIO_VLAN_FRAME_TYPE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_frame_type,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
cli_cmd_tab_entry (
    "VLAN IngressFilter [<port_list>]",
    "VLAN IngressFilter [<port_list>] [enable|disable]",
    "Set or show the port VLAN ingress filter",
    PRIO_VLAN_INGR_FILTER,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_ingr_filter,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */
cli_cmd_tab_entry (
    "VLAN tx_tag [<port_list>]",
    "VLAN tx_tag [<port_list>] [untag_pvid|untag_all|tag_all]",
    "Set or show the port egress tagging",
    PRIO_VLAN_TX_TAG,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_tx_tag,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
CLI_CMD_TAB_ENTRY_DECL(cli_cmd_vlan_port_type) = {
    "VLAN PortType [<port_list>]",
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
    "VLAN PortType [<port_list>] [unaware|c-port]",
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    "VLAN PortType [<port_list>] [unaware|c-port|s-port|s-custom-port]",
#endif
    "Set or show the VLAN port Type",
    PRIO_VLAN_PORT_TYPE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_port_type,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
cli_cmd_tab_entry (
    "VLAN EtypeCustomSport",
    "VLAN EtypeCustomSport [<etype>]",
    "Set or show the Custom S-port EtherType",
    PRIO_VLAN_TPID_TYPE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_tpid,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif  /* VTSS_FEATURE_VLAN_PORT_V2 */
CLI_CMD_TAB_ENTRY_DECL(cli_cmd_vlan_add) = {
    NULL,
#ifdef VTSS_SW_OPTION_VLAN_NAME
    "VLAN Add <vid>|<name> [<ports_list>]",
#else
    "VLAN Add <vid> [<ports_list>]",
#endif
    "Add or modify VLAN entry",
    PRIO_VLAN_ADD,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_add,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(cli_cmd_forbidden_vlan_add) = {
    NULL,
#ifdef VTSS_SW_OPTION_VLAN_NAME
     "VLAN Forbidden Add <vid>|<name> [<port_list>]",
#else
    "VLAN Forbidden Add <vid> [<port_list>]",
#endif
    "Add or modify VLAN entry in forbidden table",
    PRIO_VLAN_ADD,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_forbidden_vlan_add,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(cli_cmd_vlan_delete) = {
    NULL,
#ifdef VTSS_SW_OPTION_VLAN_NAME
    "VLAN Delete <vid>|<name>",
#else
    "VLAN Delete <vid>",
#endif
    "Delete VLAN entry",
    PRIO_VLAN_DEL,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_delete,
    vlan_cli_req_delete_default_set,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(cli_cmd_vlan_forbidden_delete) = {
    NULL,
#ifdef VTSS_SW_OPTION_VLAN_NAME
    "VLAN Forbidden Delete <vid>|<name>",
#else
    "VLAN Forbidden Delete <vid>",
#endif
    "Delete VLAN entry",
    PRIO_VLAN_DEL,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_forbidden_delete,
    vlan_cli_req_delete_default_set,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
}; 

CLI_CMD_TAB_ENTRY_DECL(cli_cmd_vlan_lookup) = {
#ifdef VTSS_SW_OPTION_VLAN_NAME
    "VLAN Lookup [<vid>] [(name <name>)] [combined|static"
#else
        "VLAN Lookup [<vid>] [combined|static"
#endif
#ifdef VTSS_SW_OPTION_DOT1X
        "|nas"
#endif
#ifdef VTSS_SW_OPTION_MVR
        "|mvr"
#endif



#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "|voice_vlan"
#endif
#ifdef VTSS_SW_OPTION_EVC
        "|evc"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "|rspan"
#endif
        "|all]",
    NULL,
    "Lookup VLAN entry",
    PRIO_VLAN_LOOKUP,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_lookup,
    vlan_cli_req_lookup_default_set,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(cli_cmd_vlan_forbidden_lookup) = {
#ifdef VTSS_SW_OPTION_VLAN_NAME
    "VLAN Forbidden Lookup [<vid>] [(name <name>)] ",
#else
    "VLAN Forbidden Lookup [<vid>]",
#endif
    NULL,
    "Lookup VLAN Forbidden port entry",
    PRIO_VLAN_LOOKUP,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_forbidden_lookup,
    vlan_cli_req_lookup_default_set,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

#ifdef VTSS_SW_OPTION_VLAN_NAME
cli_cmd_tab_entry (
        NULL,
        "VLAN Name Add <name> <vid>",
        "Add VLAN Name to a VLAN ID Mapping",
        PRIO_VLAN_NAME_ADD,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VLAN,
        cli_cmd_vlan_name_add,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
cli_cmd_tab_entry (
        NULL,
        "VLAN Name Delete <name>",
        "Delete VLAN Name to VLAN ID Mapping",
        PRIO_VLAN_NAME_DEL,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_VLAN,
        cli_cmd_vlan_name_del,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
cli_cmd_tab_entry (
        "VLAN Name Lookup [<name>]",
        NULL,
        "Show VLAN Name table",
        PRIO_VLAN_NAME_LOOKUP,
        CLI_CMD_TYPE_STATUS,
        VTSS_MODULE_ID_VLAN,
        cli_cmd_vlan_name_lookup,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#endif
CLI_CMD_TAB_ENTRY_DECL(cli_cmd_debug_vlan_add) = {
    "Debug VLAN Add <vid> [<ports_list>] "
        "[static"
#ifdef VTSS_SW_OPTION_DOT1X
        "|nas"
#endif
#ifdef VTSS_SW_OPTION_MVR
        "|mvr"
#endif



#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "|voice_vlan"
#endif
#ifdef VTSS_SW_OPTION_EVC
        "|evc"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "|rspan"
#endif
        "]",
    NULL,
    "Add VLAN entry",
    PRIO_DEBUG_VLAN_ADD,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_DEBUG,
    cli_cmd_debug_vlan_add,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
CLI_CMD_TAB_ENTRY_DECL(cli_cmd_vlan_port_status) = {
    "VLAN Status [<port_list>] "
        "[combined|static"
#ifdef VTSS_SW_OPTION_DOT1X
        "|nas"
#endif
#ifdef VTSS_SW_OPTION_MVR
        "|mvr"
#endif



#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "|voice_vlan"
#endif
#ifdef VTSS_SW_OPTION_MSTP
        "|mstp"
#endif
#ifdef VTSS_SW_OPTION_ERPS
        "|erps"
#endif
#ifdef VTSS_SW_OPTION_MEP
        "|mep"
#endif
#ifdef VTSS_SW_OPTION_VCL
        "|vcl"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "|rspan"
#endif
        "|all|conflicts]",
    NULL,
    "VLAN Port Configuration Status",
    PRIO_VLAN_PORT_STATUS,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_VLAN,
    cli_cmd_vlan_port_status,
    NULL,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
CLI_CMD_TAB_ENTRY_DECL(cli_cmd_debug_vlan_delete) = {
    "Debug VLAN Delete <vid> "
        "[static"
#ifdef VTSS_SW_OPTION_DOT1X
        "|nas"
#endif
#ifdef VTSS_SW_OPTION_MVR
        "|mvr"
#endif



#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "|voice_vlan"
#endif
#ifdef VTSS_SW_OPTION_EVC
        "|evc"
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        "|rspan"
#endif
        "]",
    NULL,
    "Delete VLAN entry",
    PRIO_DEBUG_VLAN_DELETE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_DEBUG,
    cli_cmd_debug_vlan_delete,
    vlan_cli_req_delete_default_set,
    vlan_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

cli_cmd_tab_entry (
        NULL,
        "Debug VLAN Fillup <vid>",
        "Fill up the vlan table",
        PRIO_DEBUG_VLAN_FILL_UP,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_fill,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
cli_cmd_tab_entry (
        NULL,
        "Debug VLAN Empty",
        "Empty the vlan table",
        PRIO_DEBUG_VLAN_EMPTY,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_empty,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
cli_cmd_tab_entry (
        NULL,
        "Debug VLAN Lookup",
        "Read the VLAN configuration from the Hardware",
        PRIO_DEBUG_VLAN_TABLE_LOOKUP,
        CLI_CMD_TYPE_STATUS,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_table_lookup,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
cli_cmd_tab_entry (
        NULL,
        "Debug VLAN Etype",
        "Read the VLAN Ethernet Type from the Hardware",
        PRIO_DEBUG_VLAN_TPID,
        CLI_CMD_TYPE_STATUS,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_tpid,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#endif
cli_cmd_tab_entry (
        NULL,
        "Debug VLAN Conf [<port_list>]",
        "Fetch the VLAN port configuration on slaves",
        PRIO_DEBUG_VLAN_PORT_SLAVE,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_slave_vlan_port_conf,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#ifdef  VLAN_USER_DEBUG_CMD_ENABLE
cli_cmd_tab_entry (
        NULL,
        VLAN_debug_rw_syntax_porttype,
        "Set the port VLAN PortType for Volatile VLAN User",
        PRIO_DEBUG_VLAN_PORTTYPE,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_porttype,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );  
cli_cmd_tab_entry (
        NULL,
        VLAN_debug_rw_syntax_pvid,
        "Set the port VLAN ID for Volatile VLAN User",
        PRIO_DEBUG_VLAN_PVID,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_pvid,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );  
cli_cmd_tab_entry (
        NULL,
        VLAN_debug_rw_syntax_fr_type,
        "Set the port VLAN frame type for Volatile VLAN User",
        PRIO_DEBUG_VLAN_FRAME_TYPE,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_frame_type,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
cli_cmd_tab_entry (
        NULL,
        VLAN_debug_rw_syntax_ingr,
        "Set the port VLAN ingress filter for Volatile VLAN User",
        PRIO_DEBUG_VLAN_INGR_FILTER,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_ingr_filter,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#endif
cli_cmd_tab_entry (
        NULL,
        VLAN_debug_rw_syntax_tx_tag,
        "Set the port VLAN tx_tag type for Volatile VLAN User",
        PRIO_DEBUG_VLAN_TX_TAG,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_tx_tag,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
cli_cmd_tab_entry (
        NULL,
        VLAN_debug_rw_syntax_del,
        "Delete the VLAN port configuration set by Volatile VLAN User",
        PRIO_DEBUG_VLAN_DEL,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_del,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
cli_cmd_tab_entry (
        NULL,
        "Debug VLAN Counters <vid> [clear]",
        "Read the VLAN Counters from the Hardware",
        PRIO_DEBUG_VLAN_TPID,
        CLI_CMD_TYPE_STATUS,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_counters,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#endif  //VTSS_FEATURE_VLAN_COUNTERS
#if defined(VTSS_FEATURE_VLAN_TX_TAG)
cli_cmd_tab_entry (
        NULL,
        "Debug VLAN Tag <vid> [<port_list>] [port|enable|disable]",
        "Set or show the VLAN Tx tag configuration",
        PRIO_DEBUG_VLAN_TAG,
        CLI_CMD_TYPE_CONF,
        VTSS_MODULE_ID_DEBUG,
        cli_cmd_debug_vlan_tag,
        NULL,
        vlan_cli_parm_table,
        CLI_CMD_FLAG_NONE
        );
#endif  //VTSS_FEATURE_VLAN_TX_TAG
#endif  //VLAN_USER_DEBUG_CMD_ENABLE
/****************************************************************************/
    /*                                                                          */
    /*  End of file.                                                            */
    /*                                                                          */
    /****************************************************************************/
