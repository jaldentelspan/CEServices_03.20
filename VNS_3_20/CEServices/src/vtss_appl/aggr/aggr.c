/*

 Vitesse Switch API software.

 Copyright (c) 2002-2011 Vitesse Semiconductor Corporation "Vitesse". All
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
#if VTSS_SWITCH_STACKABLE
#include "topo_api.h"
#endif /* VTSS_SWITCH_STACKABLE */

#include "msg_api.h"

#include "conf_api.h"
#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#include "critd_api.h"

#ifdef VTSS_SW_OPTION_LACP
#include "l2proto_api.h"
#include "lacp_api.h"
#ifndef _VTSS_LACP_OS_H_
#include "vtss_lacp_os.h"
#endif
#endif /* VTSS_SW_OPTION_LACP */
#if defined(VTSS_SW_OPTION_DOT1X)
#include "dot1x_api.h"
#endif /* VTSS_SW_OPTION_DOT1X */
#include "aggr_api.h"
#include "aggr.h"

#ifdef VTSS_SW_OPTION_CLI
#include "aggr_cli.h"
#endif
#ifdef VTSS_SW_OPTION_ICLI
#include "aggr_icfg.h"
#endif

#define VTSS_RC(expr)   { vtss_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) return __rc__; }
/*
  Synopsis.
  The aggregation module controls static and dynamic (LACP) created aggregations.

  VSTAX_V1:  Stack configuration as supported by L28.
  VSTAX_V2:  Stack configuration as supported by JR.
  STANDALONE:  Standalone configuration as supported by L26, L28 and JR/JR-48 standalone. Only LLAGs

  Configuration.
  The static aggregations are saved to flash while only the LACP configuration is saved to ram and lost after reset.
  If a portlink goes down that port is removed from a dynamic created aggregation (at chiplevel) while the statically 
  created will be left untouched (the portmask will be modified by the port-module).

  Aggregation id's.
  VSTAX_V1 (L28): LLAG is numbered from 1 and GLAGS start where LLAGS end. 
  VSTAX_V2 (JR): No LLAGs. GLAGs numbered in the same way as AGGR_MGMT_GROUP_NO_START - AGGR_MGMT_GROUP_NO_END.
  All aggregations are identified by AGGR_MGMT_GROUP_NO_START - AGGR_MGMT_GROUP_NO_END.
  The aggregations are kept in one common pool for both static and dynamic usage, i.e. each could occupie all groups.

  Dynamic LLAGs and GLAGs.
  VSTAX_V1 (L28):  Dynamic aggregation will always try to limit it selv to a LLAG, if it's portrange only span one switch.  
             The same dynamic aggregation can switch between a LLAG and a GLAG if a port joins or leaves from another switch.
  VSTAX_V2  (JR):  Dynamic aggregation are based on 32 GLAGs.

  Limited HW resources.
  The HW limits GLAG to hold max 8 ports while LLAG can hold 16. 

  Aggregation map.
  The core of the LACP does not know anything about stacking.  All it's requests are based on aggregation id (aid) and a port number (l2_port).
  It's aid's spans the same range as number of ports, i.e. per default all ports are member of it's own aid.

  Crosshecks.
  The status of Dynamic and static aggregation are checked before they are created.
  This applies both to port member list as well as the aggr id.
  Furthermore dot1x may not coexist with an aggregation and is also checked before an aggregation 
  is created.

  Callbacks.
  Some switch features are depended on aggregation status, e.g. STP.  Therefore they can be notified when an aggregation changes
  by use of callback subscription.  See aggr_api for details.

*/


/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/
#if (VTSS_TRACE_ENABLED)
static vtss_trace_reg_t trace_reg =
{ 
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "aggr",
    .descr     = "AGGR"
};

static vtss_trace_grp_t trace_grps[TRACE_GRP_CNT] =
{
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

#define AGGR_CRIT_ENTER() critd_enter(&aggrglb.aggr_crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#define AGGR_CRIT_EXIT()  critd_exit( &aggrglb.aggr_crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#define AGGR_CB_CRIT_ENTER()  critd_enter( &aggrglb.aggr_cb_crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#define AGGR_CB_CRIT_EXIT()  critd_exit( &aggrglb.aggr_cb_crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#else
#define AGGR_CRIT_ENTER() critd_enter(&aggrglb.aggr_crit)
#define AGGR_CRIT_EXIT()  critd_exit( &aggrglb.aggr_crit)
#define AGGR_CB_CRIT_ENTER()  critd_enter( &aggrglb.aggr_cb_crit)
#define AGGR_CB_CRIT_EXIT()  critd_exit( &aggrglb.aggr_cb_crit)
#endif /* VTSS_TRACE_ENABLED */

#define AGGRFLAG_ABORT_WAIT         (1 << 0)
#define AGGRFLAG_COULD_NOT_TX       (1 << 1)
#define AGGRFLAG_WAIT_DONE          (1 << 2)

/* Global structure */
static aggr_global_t  aggrglb;

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

/* Callback function if aggregation changes */
static void aggr_change_callback(vtss_isid_t isid, int aggr_no)
{
    u32 i;
    T_D("ISID:%d Aggr %d changed",isid,aggr_no);
    AGGR_CB_CRIT_ENTER();
    for(i = 0; i < aggrglb.aggr_callbacks; i++) 
        aggrglb.callback_list[i](isid, aggr_no);
    AGGR_CB_CRIT_EXIT();
    
}

/* Save the config to flash */
static void aggr_save_to_flash(void)
{
    aggr_conf_stack_aggr_t   *aggr_blk;
    if ((aggr_blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_AGGR_TABLE, NULL)) == NULL)
        T_E("failed to open port config table");
    else {    
        AGGR_CRIT_ENTER();
        *aggr_blk = aggrglb.aggr_config_stack;        
        AGGR_CRIT_EXIT();
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_AGGR_TABLE);
        T_D("Added new aggr entry to Flash");
    }
}


/* Port state callback function. This function is called if a GLOBAL port state change occur.  */
/* Only for JR stackable (GLAGs) */
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
static void aggr_global_port_state_change_callback(vtss_isid_t isid, vtss_port_no_t port_no, port_info_t *info)
{
    vtss_glag_no_t glag_no;
    AGGR_CRIT_ENTER();
    glag_no = aggrglb.active_aggr_ports[isid][port_no];
    AGGR_CRIT_EXIT();
    
    if(glag_no != VTSS_GLAG_NO_NONE) {
        T_D("isid:%d port_no: %u. link %s\n", isid, port_no, info->link ? "up" : "down");      
        (void)aggr_change_callback(isid, glag_no);
    }
    return; /* Jaguar GLAGs are handled elsewere */
} 
#elif defined(VTSS_FEATURE_VSTAX_V1)     
/* Only for Luton_28 GLAGs/LLAGs */
static void aggr_global_port_state_change_callback(vtss_isid_t isid, vtss_port_no_t port_no, port_info_t *info)
{   
    aggr_mgmt_group_member_t members;
    int aggr_no;
    vtss_isid_t              isid_tmp;

    T_D("port_no: %u. link %s\n", port_no, info->link ? "up" : "down");      

    if (!info->link) { 
        return;
    }
   
    if(msg_switch_is_master() && !info->stack) {              
        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {
            if (aggr_mgmt_port_members_get(isid, aggr_no, &members, 0) == VTSS_OK) {
                if (members.entry.member[port_no]) {
                    /* If the port is in HDX or not in the same speed as rest of the group then the port is removed */
                    if (!info->fdx) {            
                        /* HDX ports not allowed in an aggregation */
                        members.entry.member[port_no] = FALSE;            
                    }       
                    AGGR_CRIT_ENTER();
                    /* GLAGs: Verify that the port have the same speed as the group speed */
                    if (AGGR_MGMT_GROUP_IS_GLAG(aggr_no) && (aggrglb.aggr_group_speed[isid][aggr_no] == VTSS_SPEED_UNDEFINED)) {
                        for (isid_tmp = VTSS_ISID_START; isid_tmp < VTSS_ISID_END; isid_tmp++) {           
                            if (aggrglb.aggr_group_speed[isid_tmp][aggr_no] != VTSS_SPEED_UNDEFINED) {
                                if (info->speed != aggrglb.aggr_group_speed[isid_tmp][aggr_no]) {
                                    T_W("Port speed must be the same for all GLAG members. Port %u removed from group.",port_no);
                                    members.entry.member[port_no] = FALSE;
                                    break;
                                }
                            }
                        }
                    } else {
                        /* LAGs: Verify that the port have the same speed as the group speed */
                        if (aggrglb.aggr_group_speed[isid][aggr_no] != VTSS_SPEED_UNDEFINED) {
                            if (info->speed != aggrglb.aggr_group_speed[isid][aggr_no]) {
                                members.entry.member[port_no] = FALSE;
                            }
                        }
                    }

                    AGGR_CRIT_EXIT();
                    if (aggr_mgmt_port_members_add(isid, aggr_no, &members.entry) != VTSS_OK) {
                        T_E("Could not remove port %u from aggr group:%d\n",port_no,aggr_no);
                        return;
                    }
                    break;
                }                    
            }
        }
    }
}
#else
/* Port state callback function  This function is called if a GLOBAL port state change occur.  */
/* For JR/JR48 standalone, Luton26 (LLAGS) */
static void aggr_global_port_state_change_callback(vtss_isid_t isid, vtss_port_no_t port_no, port_info_t *info)
{
    BOOL  aggr_members[VTSS_PORT_ARRAY_SIZE], save_to_flash=0;
    aggr_mgmt_group_no_t aggr_id;    

    AGGR_CRIT_ENTER();
    aggr_id = aggrglb.port_aggr_member[port_no];
    AGGR_CRIT_EXIT();

    T_D("port_no: %u. link %s\n", port_no, info->link ? "up" : "down");
    if(aggr_id != VTSS_AGGR_NO_NONE) {
        if (info->link) {
            AGGR_CRIT_ENTER();
            if (!info->fdx) {
                /* HDX ports not allowed in an aggregation */
                T_D("HDX ports not allowed in an aggregation");
                aggrglb.port_active_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
            } else if (aggrglb.aggr_group_speed[isid][aggr_id] == VTSS_SPEED_UNDEFINED) {
                /* Speed is undefined -> define it */
                T_D("Speed is undefined, and will now be defined");
                aggrglb.aggr_group_speed[isid][aggr_id] = info->speed;
                aggrglb.port_active_aggr_member[port_no] = aggr_id;
                aggrglb.aggr_config_stack.switch_id[isid][aggr_id-AGGR_MGMT_GROUP_NO_START].speed = aggrglb.aggr_group_speed[isid][aggr_id];
                save_to_flash = 1;
            } else if (info->speed != aggrglb.aggr_group_speed[isid][aggr_id]) {
                /* Speed is different from the group speed */
                T_D("Speed is different from the group speed - inactivating port");
                aggrglb.port_active_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
            } else {
                /* Everything looks right, activate the port */
                aggrglb.port_active_aggr_member[port_no] = aggr_id;
            }
            for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
                aggr_members[port_no] = (aggrglb.port_active_aggr_member[port_no] == aggr_id) ? 1 : 0;
            }
            AGGR_CRIT_EXIT();
            if (vtss_aggr_port_members_set(NULL, aggr_id-AGGR_MGMT_GROUP_NO_START, aggr_members) != VTSS_RC_OK) {
                T_E("port_no: %u. Could not apply aggr settings to API\n", port_no);
            }        
            if (save_to_flash) {
                (void)aggr_save_to_flash();
            }
            (void)aggr_change_callback(isid, aggr_id);   
        } else {
            AGGR_CRIT_ENTER();
            aggrglb.port_active_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
            AGGR_CRIT_EXIT();
        }
    }
}
#endif


#if VTSS_SWITCH_STACKABLE
void aggr_glag_callback (topo_state_change_mask_t mask)
{
#if defined(VTSS_FEATURE_VSTAX_V1)
    topo_sit_t *sit;
    int i;
    uint j;
    vtss_glag_no_t glag_no;
    u32 port_member_array[VTSS_GLAG_PORT_ARRAY_SIZE];
    vtss_port_no_t p;
    vtss_vstax_upsid_t upsid;
    int glag_mbr_count;
    topo_sit_entry_t *entry;
        
    T_D("Glag_callback. Mask:%d",mask);        
    if (mask == TOPO_STATE_CHANGE_MASK_GLAG) {
        T_D("TOPO_STATE_CHANGE_MASK_GLAG (GLAG member count changed)");        
    }
    if (mask == TOPO_STATE_CHANGE_MASK_TTL) {
        T_D("TOPO_STATE_CHANGE_MASK_TTL (TTL changed)");        
    }
    
    if (!(sit = malloc(sizeof(topo_sit_t)))) {
        T_E("malloc of topo_sit_t failed, size=%zu", sizeof(topo_sit_t));
        return;
    }
   
    /* fetch switch information table */
    if (topo_sit_get(sit) != VTSS_OK) {
        T_E("Could not get information table");
    }

    /* Debug only */
    for (i = 0; i < VTSS_SPROUT_SIT_SIZE; i++) {
        if (sit->si[i].vld) {
            T_D("Found switch with sid:%u  upsid:%d\n", sit->si[i].id, sit->si[i].chip[0].upsid[0]);
            T_D("This shortest path to this switch:%u\n", sit->si[i].chip[0].shortest_path);
            T_D("GLAG members 1:%u  2:%u\n", sit->si[i].glag_mbr_cnt[1], sit->si[i].glag_mbr_cnt[2]);
        }
    }

    /* run through all possible GLAG's */
    for(glag_no = VTSS_GLAG_NO_START; glag_no < VTSS_GLAG_NO_END; glag_no++) {
        glag_mbr_count = 1;
        /* build member array, sorted in UPSID order */
        for(upsid = VTSS_VSTAX_UPSID_MIN; upsid <= VTSS_VSTAX_UPSID_MAX; upsid++) {
            entry = NULL;
            /* find current SIT entry (if any) for this UPSID */
            for(i = 0; i < TOPO_SIT_SIZE; i++) {
                if(sit->si[i].vld && sit->si[i].chip[0].upsid[0] == upsid) {
                    T_D("Found switch/UPSID %d in SIT", upsid);
                    entry = &sit->si[i];
                    break;
                }
            }
            /* then this UPSID is found in the stack */
            if(entry != NULL) {
                /* 
                ** if the switch we're looking at has ports in this GLAG, we need to include it in 
                ** the member array we supply to the API
                */
                if(entry->glag_mbr_cnt[glag_no+1] > 0) {

                    if(entry->chip[0].shortest_path == 0) {
                        T_D("Local switch");
                        /* insert all local ports (in any state as documented in vtss.h) 
                        ** with this GLAG id in member array (array must be sorted) */
                        AGGR_CRIT_ENTER();
                        for(p = VTSS_PORT_NO_START; p < VTSS_PORT_NO_END; p++) {
                            if(aggrglb.port_glag_member[p] == glag_no+AGGR_MGMT_GLAG_START) {
                                port_member_array[glag_mbr_count-1] = p;
                                glag_mbr_count++;
                                if(glag_mbr_count > (VTSS_GLAG_PORTS+1)) {
                                    T_E("Too many ports (%d) in GLAG #%u. p=%u", 
                                            glag_mbr_count, glag_no, p);

                                    aggrglb.aggr_glag_config_result[glag_no] = 0;
                                    free(sit);
                                    AGGR_CRIT_EXIT();
                                    return;
                                }
                            }
                        }
                        AGGR_CRIT_EXIT();
                    } else {
                        T_D("Remote switch");
                        /* insert n entries with stack port with shortest path to target switch */
                        for(j = 0; j < entry->glag_mbr_cnt[glag_no+1]; j++) {
                            port_member_array[glag_mbr_count-1] = entry->chip[0].shortest_path;
                            glag_mbr_count++;
                            if(glag_mbr_count > (VTSS_GLAG_PORTS+1)) {
                                T_E("Too many ports (%d) in GLAG #%u. j=%d", 
                                        glag_mbr_count, glag_no, j);

                                AGGR_CRIT_ENTER();
                                aggrglb.aggr_glag_config_result[glag_no] = 0;
                                AGGR_CRIT_EXIT();
                                free(sit);
                                return;
                            }
                        }
                    }
                }
            }            
        }


        /* If glag_mbr_count == 1 then we try to put in all local ports in GLAG
        ** this enables the GLAG even if no stack communication has occured - i.e. if no UPSIDs have
        ** been assigned */
        AGGR_CRIT_ENTER();
        if(glag_mbr_count == 1) {
            /* insert all local ports (in any state as documented in vtss.h) 
            ** with this GLAG id in member array (array must be sorted) */
            for(p = VTSS_PORT_NO_START; p < VTSS_PORT_NO_END; p++) {
                if(aggrglb.port_glag_member[p] == glag_no+AGGR_MGMT_GLAG_START) {
                    port_member_array[glag_mbr_count-1] = p;
                    glag_mbr_count++;
                    if(glag_mbr_count > (VTSS_GLAG_PORTS+1)) {
                        T_E("Too many ports (%d) in GLAG #%u. p=%u", 
                                glag_mbr_count, glag_no, p);

                        aggrglb.aggr_glag_config_result[glag_no] = 0;
                        free(sit);
                        AGGR_CRIT_EXIT();
                        return;
                    }
                }
            }
        }

        /* zero terminate array and let API update masks */
        port_member_array[glag_mbr_count-1]= VTSS_PORT_NO_NONE;

        /* debug only */
        T_D("glag_mbr_count = %d", glag_mbr_count);
        for(j = 0; j < (uint)glag_mbr_count; j++) {
            T_D("GLAG #%u   [%d] = %u", glag_no, j, port_member_array[j]);
        }
        
        if (vtss_aggr_glag_set(NULL, glag_no, port_member_array) == VTSS_OK) {
            aggrglb.aggr_glag_config_result[glag_no] = 1;
        } 
        AGGR_CRIT_EXIT();

    }
    free(sit);
#endif
}

/* If a GLAG configuration (Add/Del) occurs the 'aggr_glag_config_or_link_state_change'
** function is called.  This function will notify the Topo module.
** If a port link state changes the 'aggr_glag_config_or_link_state_change' will be called 
** through a port link callback function.
*/

#if defined(VTSS_FEATURE_VSTAX_V1) && VTSS_SWITCH_STACKABLE   
static void aggr_glag_config_or_link_state_change (void)
{
    vtss_glag_no_t glag_no;
    vtss_port_no_t port_no;
    uint glag_mbr_count;
    port_info_t info;

    const topo_chip_idx_t chip_idx=0;
    vtss_rc rc;

    T_D("Inside glag_config_or_link_state_change. Active_glags 0:%d  1:%d.\n",aggrglb.glag_active_count[0],aggrglb.glag_active_count[1]);
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        if ((aggrglb.port_glag_member[port_no] == AGGR_MGMT_GLAG_START) || (aggrglb.port_glag_member[port_no] == AGGR_MGMT_GLAG_START+1)) {
            T_D("port_no %u is member of glag %u\n",port_no,aggrglb.port_glag_member[port_no]);
        }
    }
                
    /* update all GLAGs with number of member ports */
    for(glag_no = VTSS_GLAG_NO_START; glag_no < VTSS_GLAG_NO_END; glag_no++) {
        glag_mbr_count = 0;
        for(port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {

            /* Get current status */            
            if(aggrglb.port_glag_member[port_no] == glag_no+AGGR_MGMT_GLAG_START) {                
                if (port_info_get(port_no, &info) == VTSS_OK && info.link) {                    
                    glag_mbr_count++;
                    T_D("port_no:%u is a member (%d) and with link.",port_no,glag_mbr_count);
                } 
            }
        }        

        if(aggrglb.glag_active_count[glag_no] != glag_mbr_count) {
            T_D("aggrglb.glag_active_count:%d !=  glag_mbr_count:%d. Updating topo_glag_mbr_cnt_set with glag:%u \n",
                aggrglb.glag_active_count[glag_no],glag_mbr_count,glag_no);
            aggrglb.glag_active_count[glag_no] = glag_mbr_count;
            rc = topo_glag_mbr_cnt_set(chip_idx, glag_no+1, glag_mbr_count);
            if (rc != VTSS_OK) 
                T_E("Problem with updating aggr config over Stack.\n");

            
        } else {
            /* local configuration with port with no link */
            T_D("aggrglb.glag_active_count[glag_no] = glag_mbr_count\n");
            aggrglb.aggr_glag_config_result[glag_no] = 1;            
        }

    }    
}
#endif /* VTSS_FEATURE_VSTAX_V1 */

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE   
static vtss_rc glag_update_master(aggr_mgmt_group_no_t glag_no, vtss_port_speed_t spd)
{
    aggr_msg_glag_update_req_t   *msg;   
    vtss_port_no_t               port_no; 
    u32                          port_count = port_isid_port_count(VTSS_ISID_LOCAL);

    msg = malloc(sizeof(*msg));
    if (msg) {
        msg->msg_id = AGGR_MSG_ID_GLAG_UPDATE_REQ;
        msg->aggr_no = glag_no;
        msg->grp_speed = spd;
        AGGR_CRIT_ENTER();
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (aggrglb.port_active_glag_member[port_no] == glag_no) {            
                msg->members.member[port_no] = 1; 
            } else {
                msg->members.member[port_no] = 0; 
            }
        }        
        AGGR_CRIT_EXIT ();
        msg_tx(VTSS_MODULE_ID_AGGR, 0, msg, sizeof(*msg));            
        return VTSS_OK;
    } else {
        return VTSS_RC_ERROR;
    }

}
#endif /* VTSS_FEATURE_VSTAX_V2 */

/* Port state callback function  This function is called if a LOCAL port state change occur.  */
static void aggr_port_state_change_callback(vtss_port_no_t port_no, port_info_t *info)
{

    /* If the port is a member of an GLAG then call aggr_glag_config_or_link_state_change */
    AGGR_CRIT_ENTER();
    if(aggrglb.port_glag_member[port_no] != VTSS_GLAG_NO_NONE) {
        T_D("port_no: %u. link %s\n", port_no, info->link ? "up" : "down");      
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE   
        vtss_glag_no_t glag_no;
        vtss_port_speed_t spd;
        glag_no = aggrglb.port_glag_member[port_no];
        spd = aggrglb.aggr_group_speed[VTSS_ISID_LOCAL+1][glag_no];
        if (info->link) {
            if (!info->fdx) {            
                /* HDX ports not allowed in an aggregation */
                T_W("Port:%u. HDX ports not allowed - removing port from aggregation",port_no);       
                 aggrglb.port_active_glag_member[port_no] = VTSS_GLAG_NO_NONE;
            } else if (spd == VTSS_SPEED_UNDEFINED) {
                aggrglb.aggr_group_speed[VTSS_ISID_LOCAL+1][glag_no] = info->speed;
                spd = info->speed;
                aggrglb.port_active_glag_member[port_no] = glag_no;            
            } else if (info->speed != spd) {
                T_W("Port:%u. Speed does not match group speed  - removing port from aggregation",port_no);       
                aggrglb.port_active_glag_member[port_no] = VTSS_GLAG_NO_NONE;
            } else {
                aggrglb.port_active_glag_member[port_no] = glag_no;            
            }
        } else {
            aggrglb.port_active_glag_member[port_no] = VTSS_GLAG_NO_NONE;
        }
        AGGR_CRIT_EXIT(); 
        if (glag_update_master(glag_no, spd) != VTSS_OK) {
            T_E("Could not update master after port event\n");
        }
        AGGR_CRIT_ENTER(); 
#else
        aggr_glag_config_or_link_state_change();
#endif  
    }                
    AGGR_CRIT_EXIT(); 
}

#endif /* VTSS_SWITCH_STACKABLE */


#if VTSS_SWITCH_STACKABLE
/* Delete GLAG group (1,2) from chip API.*/
static vtss_rc glag_group_del(vtss_glag_no_t glag_no)
{
    vtss_rc rc = VTSS_OK;
#if defined(VTSS_FEATURE_VSTAX_V1)
    vtss_port_no_t port_no;
    const topo_chip_idx_t chip_idx=0;
    BOOL glag_active = 0;

    T_D("Deleting glag group:%u \n",glag_no);

    AGGR_CRIT_ENTER();

    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        if (aggrglb.port_glag_member[port_no] == glag_no+AGGR_MGMT_GLAG_START) {
            T_D("DEL:Got port %u for agg del.\n",port_no);
            aggrglb.port_glag_member[port_no] = VTSS_GLAG_NO_NONE;
            glag_active = 1;            
        }
    }
    if (glag_active) {
        rc = topo_glag_mbr_cnt_set(chip_idx, glag_no+1, 0);
        aggrglb.glag_active_count[glag_no] = 0;
    }

    AGGR_CRIT_EXIT();        

#endif    
    return rc;
}
#endif /* VTSS_SWITCH_STACKABLE */

#if defined(VTSS_FEATURE_VSTAX_V1) /* Luton28 */
/* Add ports through  chip API*/
static vtss_rc aggr_port_add(aggr_mgmt_group_no_t aggr_no, aggr_glag_members_t *members)
{
    vtss_rc rc;
    vtss_port_no_t port_no;
    uint members_no = 1;  

    if (AGGR_MGMT_GROUP_IS_LAG(aggr_no)) {
        T_D("ADD: Got lag %u for port add",aggr_no);
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) 
            if (members->member[port_no]) {
                if (members_no > AGGR_MGMT_LAG_PORTS_MAX) {
                    T_E("To many members in the LAG group:%d\n",members_no);
                    return VTSS_INVALID_PARAMETER;
                }
                members_no++;
                T_D("Adding aggr_no:%u  port_no:%u \n",aggr_no, port_no);                
            }
        /* API access */
        rc = vtss_aggr_port_members_set(NULL, aggr_no-AGGR_MGMT_GROUP_NO_START, members->member); 
        
#if VTSS_SWITCH_STACKABLE /* Luton28 stackable */
    } else if (AGGR_MGMT_GROUP_IS_GLAG(aggr_no)) {
        vtss_glag_no_t glag_no;
        int res;

        T_D("ADD: Got glag %u for port add.",aggr_no);

        AGGR_CRIT_ENTER();

        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            if (members->member[port_no]) {
                aggrglb.port_glag_member[port_no] = aggr_no;
            } else if (aggrglb.port_glag_member[port_no] == aggr_no) {
                aggrglb.port_glag_member[port_no] = VTSS_GLAG_NO_NONE;
            }
        }     
        glag_no = aggr_no-AGGR_MGMT_GLAG_START+VTSS_GLAG_NO_START;
        aggrglb.aggr_glag_config_result[glag_no] = 0;
        /* Submit the change  */
        aggr_glag_config_or_link_state_change(); 
        res = aggrglb.aggr_glag_config_result[glag_no];
        AGGR_CRIT_EXIT();
            
        /* Need to verify that the GLAG configuration was successful, */
        if (res) {            
            T_D("Returning VTSS_OK");
            return VTSS_OK;                
        } else {
            T_D("Returning VTSS_ERROR");
            /* The addition did not work out. Remove the group */                    
            if ((rc = glag_group_del(glag_no)) != VTSS_OK)
                return rc;
            return VTSS_UNSPECIFIED_ERROR;
        }                 

#endif /* VTSS_SWITCH_STACKABLE */
    } else {
        T_E("Fail. Got glag %u for port add.\n",aggr_no);
        rc = VTSS_INVALID_PARAMETER;
    }
    return rc;
}
#endif /* !(defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE) */


/* Get config from chip API*/
static vtss_rc aggr_ports_get(aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t  *members)
{
    vtss_rc        rc = VTSS_OK;
    vtss_port_no_t port_no;
    u32            port_count = port_isid_port_count(VTSS_ISID_LOCAL);
    
    memset(members, 0, sizeof(aggr_mgmt_group_t));
    if (AGGR_MGMT_GROUP_IS_LAG(aggr_no) || aggr_no == AGGR_UPLINK) {        
        rc = vtss_aggr_port_members_get(NULL, aggr_no-AGGR_MGMT_GROUP_NO_START, members->member); 

    } else if (AGGR_MGMT_GROUP_IS_GLAG(aggr_no)) {
        T_D("GET:Got glag %u for agg get.\n",aggr_no);
        AGGR_CRIT_ENTER();
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (aggrglb.port_glag_member[port_no] == aggr_no) {
                members->member[port_no] = 1;                
            } else {
                members->member[port_no] = 0;                
            }
        }
        AGGR_CRIT_EXIT();                   
    } else {
        T_D("Fail. Got %u for port get.\n",aggr_no);
        rc = VTSS_INVALID_PARAMETER;
    }
    
    return rc;
}

/* Set aggregation mode in chip API*/
static vtss_rc aggr_mode_set(vtss_aggr_mode_t *mode)
{
    T_D("Setting aggr mode:smac:%d, dmac:%d, IP:%d, port:%d.\n", mode->smac_enable,
        mode->dmac_enable, mode->sip_dip_enable, mode->sport_dport_enable);   

    return vtss_aggr_mode_set(NULL, mode);
}

/* Adds ports to a aggregation group.  The 'member' pointer points to the member list  */
#if defined(VTSS_FEATURE_VSTAX_V1)
static vtss_rc aggr_local_port_members_add(aggr_mgmt_group_no_t aggr_no,  aggr_glag_members_t *members)
{
    vtss_rc rc;
    vtss_port_no_t port_no;

    /* Add to chip config */    
    if ((rc = aggr_port_add(aggr_no, members)) != VTSS_OK) {
        return rc;
    }

    /* Add to local config */
    AGGR_CRIT_ENTER();
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) 
        VTSS_BF_SET(aggrglb.aggr_config.groups[aggr_no-AGGR_MGMT_GROUP_NO_START].member, 
                    (port_no-VTSS_PORT_NO_START), members->member[port_no]); 
    AGGR_CRIT_EXIT();   
    return VTSS_RC_OK;
}
#endif /* !(defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE) */

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
static vtss_rc apply_glag_ports_to_api(aggr_mgmt_group_no_t glag_no,  aggr_glag_members_t *members, BOOL conf, vtss_port_speed_t spd)
{
    u32 gentry;
    BOOL link_down=0;
    vtss_rc rc;
    port_info_t info;
    vtss_vstax_conf_t ups_conf;
    vtss_vstax_upsid_t upsid;
    vtss_port_map_t port_map[VTSS_PORT_ARRAY_SIZE];
    vtss_port_no_t  port_no;
    u32             port_count = port_isid_port_count(VTSS_ISID_LOCAL);
 
    if(conf) {
        /* This is GLAG configuration belongs to this unit */
        if ((rc = vtss_vstax_conf_get(NULL, &ups_conf)) != VTSS_RC_OK) {
            return rc;
        }

        /* Get the port map for converting UPSPN to API port */
        if ((rc = vtss_port_map_get(NULL, port_map)) != VTSS_RC_OK) {
            return rc;
        }
        AGGR_CRIT_ENTER();  
        /* Remove old GLAG info */
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            if (aggrglb.port_glag_member[port_no] == glag_no) {
                aggrglb.port_glag_member[port_no] = VTSS_GLAG_NO_NONE;
                aggrglb.port_active_glag_member[port_no] = VTSS_GLAG_NO_NONE;
            }
        }

        /* Convert the UPSPN to API port and save it in this local unit */
        for (gentry = VTSS_GLAG_PORT_START; gentry < VTSS_GLAG_PORT_END; gentry++) {
            if (members->entries[gentry].upspn == VTSS_UPSPN_NONE) {       
                break;
            }

            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (port_map[port_no].chip_no == 0) {
                    upsid = ups_conf.upsid_0;
                } else {
                    upsid = ups_conf.upsid_1;
                }
                if ((port_map[port_no].chip_port == members->entries[gentry].upspn) && (members->entries[gentry].upsid == upsid)) {
                    T_D("Port:%u is a member of GLAG:%u upsid:%d upspn:%u",
                        port_no,glag_no, members->entries[gentry].upsid ,members->entries[gentry].upspn);
                    aggrglb.port_glag_member[port_no] = glag_no;
                    aggrglb.port_active_glag_member[port_no] = glag_no;
                    break; /* Found the API port no for the upspn */
                }
            }             
        }    
        /* Check the link on the GLAG ports:  No link = Update master which will send out updated member list without the link down port(s) */
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (aggrglb.port_active_glag_member[port_no] == glag_no) {
                if (!(port_info_get(port_no, &info) == VTSS_OK && info.link)) { 
                    T_D("Link down on GLAG member:%u, notifying master",port_no);
                    aggrglb.port_active_glag_member[port_no] = VTSS_GLAG_NO_NONE;
                    link_down = 1;                
                } 
            }
        }
        AGGR_CRIT_EXIT();   
    }

    AGGR_CRIT_ENTER();   
    /* store the group speed coming from master */
    aggrglb.aggr_group_speed[VTSS_ISID_LOCAL+1][glag_no] = spd;    
    AGGR_CRIT_EXIT();   

    if (link_down) {     
        /* Not all of the member ports had link, notify master */
        rc = glag_update_master(glag_no, VTSS_SPEED_UNDEFINED);
    } else {
        /* Update Chip masks */
        T_N("Updating GLAG masks");
        rc = vtss_vstax_glag_set(NULL, glag_no-AGGR_MGMT_GROUP_NO_START+VTSS_GLAG_NO_START, members->entries);
    }
    return rc;
}

#endif /* VTSS_FEATURE_VSTAX_V2 */


/* Sets the aggregation mode.  The mode is used by all the aggregation groups */
static vtss_rc aggr_local_mode_set(vtss_aggr_mode_t *mode)
{    
    vtss_rc rc;

   /* Add to chip */
    if ((rc = aggr_mode_set(mode) != VTSS_OK)) {
        return rc;
    }
    /* Add to local config */
    AGGR_CRIT_ENTER();
    aggrglb.aggr_config.mode = *mode;
    AGGR_CRIT_EXIT();              
    return VTSS_OK;
}

#if VTSS_SWITCH_STACKABLE || defined(VTSS_FEATURE_VSTAX_V1)
/* Removes all members from a aggregation group. */
static vtss_rc aggr_local_group_del(aggr_mgmt_group_no_t aggr_no)
{
    aggr_mgmt_group_t members;
    vtss_rc rc=VTSS_OK;
    uint  i;
    memset(&members, 0, sizeof(members));
#if VTSS_SWITCH_STACKABLE
    if (AGGR_MGMT_GROUP_IS_GLAG(aggr_no)) {
        /* Delete GLAG group from chip */
        if ((rc = glag_group_del(aggr_no-AGGR_MGMT_GLAG_START+VTSS_GLAG_NO_START)) != VTSS_OK) {
            return rc;
        }
    } else {   
#endif
        if ((rc = vtss_aggr_port_members_set(NULL, aggr_no-AGGR_MGMT_GROUP_NO_START, members.member)) != VTSS_OK) {
            return rc;        
        }
#if VTSS_SWITCH_STACKABLE
    }
#endif
    
    /* Delete from local config */    
    AGGR_CRIT_ENTER();
    for (i = 0;i < VTSS_PORTS;i++) {
        VTSS_BF_SET(aggrglb.aggr_config.groups[aggr_no-AGGR_MGMT_GROUP_NO_START].member, i, 0); 
    }
    AGGR_CRIT_EXIT();     
    
    return VTSS_OK;
}
#endif /* VTSS_SWITCH_STACKABLE */

/* Gets ports in a aggregation group.  The 'member' pointer points to the updated member list  */
static vtss_rc aggr_local_port_members_get(aggr_mgmt_group_no_t aggr_no,  aggr_mgmt_group_member_t *members, BOOL next)

{
    vtss_rc           rc;
    vtss_port_no_t    port_no;
    BOOL              found_group = false;
    aggr_mgmt_group_t local;
    u32               port_count = port_isid_port_count(VTSS_ISID_LOCAL);

    T_D("Enter");

    /* Get the first active aggregation group */
    if (aggr_no == 0 && next) {
        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {        
            if (aggr_no == AGGR_UPLINK) {
                continue;
            }
            AGGR_CRIT_ENTER();
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (VTSS_PORT_BF_GET(aggrglb.aggr_config.groups[aggr_no-AGGR_MGMT_GROUP_NO_START].member, (port_no-VTSS_PORT_NO_START))) {
                    found_group = 1;
                    break;
                }
            } 
            AGGR_CRIT_EXIT();
            if (found_group) {
                break;
            }
        }
        if (!found_group) {
            return AGGR_ERROR_ENTRY_NOT_FOUND;
        }

    } else if (aggr_no != 0 && next) {
        /* Get the next active aggregation group */
        for (aggr_no = (aggr_no+1); aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {        
            if (aggr_no == AGGR_UPLINK) {
                continue;
            }
            AGGR_CRIT_ENTER();
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (VTSS_PORT_BF_GET(aggrglb.aggr_config.groups[aggr_no-AGGR_MGMT_GROUP_NO_START].member, (port_no-VTSS_PORT_NO_START))) {
                    found_group = 1;
                    break;
                }
            }
            AGGR_CRIT_EXIT();
            if (found_group) {
                break;
            }
        }
        if (!found_group) {
            return AGGR_ERROR_ENTRY_NOT_FOUND;
        }
    }
    
    if (AGGR_MGMT_GROUP_IS_AGGR(aggr_no) || aggr_no == AGGR_UPLINK) {
        /* Get from chip */
        rc = aggr_ports_get(aggr_no, &local);      
    } else {
        T_E("%u is not a legal aggregation group.",aggr_no);
        return VTSS_INVALID_PARAMETER;
    }    
            
    members->aggr_no = aggr_no;
    members->entry = local;

    return rc;
}


#if !defined(VTSS_FEATURE_VSTAX_V1)
static u32 number_of_glag_members(aggr_mgmt_group_no_t glag_no) {
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    u32  gentry;
    for (gentry = VTSS_GLAG_PORT_START; gentry < VTSS_GLAG_PORT_END; gentry++) {
        if (aggrglb.glag_ram_entries[glag_no - AGGR_MGMT_GLAG_START][gentry].isid == VTSS_PORT_NO_NONE)
            return gentry;
    }
#endif
    return 0;
}
#endif

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
/* Update and sort the global RAM Glag array */
static vtss_rc update_glag_array(vtss_isid_t isid, aggr_mgmt_group_no_t glag_no, 
                                 aggr_mgmt_group_t  *members)
{
    u32                      gentry, i, x, y;
    vtss_vstax_glag_entry_t  t_ups;
    vtss_isid_t              t_isid;
    u32                      port_count = port_isid_port_count(isid);
    vtss_port_no_t           port_no;
    vtss_glag_no_t           api_glag = glag_no - AGGR_MGMT_GLAG_START;


    if (!msg_switch_exists(isid)) {
        /* The switch has left this world, remove all members */
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            members->member[port_no] = 0;
        }
    } 
    AGGR_CRIT_ENTER();
    if (aggrglb.upsid_table[isid].upsid[0] == VTSS_VSTAX_UPSID_UNDEF) {
        /* The switch does not exists, remove all members */
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            members->member[port_no] = 0;
        }
    } 

    /* All GLAG ports for this ISID are included in the new member list */
    /* Remove old upsid entries before the new one is added */
    for (gentry = VTSS_GLAG_PORT_START,i = VTSS_GLAG_PORT_START; gentry < VTSS_GLAG_PORT_END; gentry++) {
        if (aggrglb.glag_ram_entries[api_glag][gentry].isid == VTSS_PORT_NO_NONE)
            break;

        if (aggrglb.glag_ram_entries[api_glag][gentry].isid != (vtss_isid_t)isid) {
            aggrglb.glag_ram_entries[api_glag][i] = aggrglb.glag_ram_entries[api_glag][gentry];
            i++;
        } 
    }
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (aggrglb.active_aggr_ports[isid][port_no] == glag_no) {
            aggrglb.active_aggr_ports[isid][port_no] = VTSS_AGGR_NO_NONE;
        }
    }

    /* Convert <ISID/port_no> to <UPSPN/UPSID>.  The UPSPN/UPSID is added to all units in the stack */
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (members->member[port_no]) {
            aggrglb.glag_ram_entries[api_glag][i].ups.upspn = aggrglb.port_map_global[isid][port_no];  

            if (aggrglb.upsid_table[isid].upsid[1] == VTSS_VSTAX_UPSID_UNDEF) {
                aggrglb.glag_ram_entries[api_glag][i].ups.upsid = aggrglb.upsid_table[isid].upsid[0];
            } else {
                aggrglb.glag_ram_entries[api_glag][i].ups.upsid = (port_no < aggrglb.upsid_table[isid].port_no) ? aggrglb.upsid_table[isid].upsid[0] : aggrglb.upsid_table[isid].upsid[1];
            }            
            aggrglb.glag_ram_entries[api_glag][i].isid = (vtss_isid_t)isid;                    
            T_D("Port:%u is an active GLAG member. Upsid:%d Upspn:%u",
                port_no,aggrglb.glag_ram_entries[api_glag][i].ups.upsid,aggrglb.glag_ram_entries[api_glag][i].ups.upspn);
            i++;
            aggrglb.active_aggr_ports[isid][port_no] = glag_no;
        }
    }
    aggrglb.glag_ram_entries[api_glag][i].ups.upspn = VTSS_UPSPN_NONE;
    aggrglb.glag_ram_entries[api_glag][i].ups.upsid = VTSS_VSTAX_UPSID_UNDEF;
    aggrglb.glag_ram_entries[api_glag][i].isid = VTSS_PORT_NO_NONE;

    /* Sort the Array: lowest to highest upsid */
    if(i > 0) {
        for (x = VTSS_GLAG_PORT_START; x < i-1; x++) {
            for (y = 0; y < i-x-1; y++) {
                if (aggrglb.glag_ram_entries[api_glag][y].isid > aggrglb.glag_ram_entries[api_glag][y+1].isid) {
                    t_ups=aggrglb.glag_ram_entries[api_glag][y].ups;
                    t_isid=aggrglb.glag_ram_entries[api_glag][y].isid;
                    aggrglb.glag_ram_entries[api_glag][y] = aggrglb.glag_ram_entries[api_glag][y+1];
                    aggrglb.glag_ram_entries[api_glag][y+1].ups=t_ups;
                    aggrglb.glag_ram_entries[api_glag][y+1].isid=t_isid;
                }
            }
        }
    }
    AGGR_CRIT_EXIT();   
    return VTSS_OK;
}
#endif /* VTSS_FEATURE_VSTAX_V2 */


/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
static vtss_rc aggr_vstax_v2_port_add(vtss_isid_t isid_add, aggr_mgmt_group_no_t glag_no, 
                                      aggr_mgmt_group_t  *members, BOOL conf)
{
    aggr_msg_add_req_t       *msg;   
    u32                      gentry;
    switch_iter_t            sit;
    vtss_rc                  rc;
   
    if (!AGGR_MGMT_GROUP_IS_AGGR(glag_no)) {
        T_E("(aggr_stack_port_add) Agg group:%u is not a legal aggr id",glag_no);
        return AGGR_ERROR_PARM;
    }

    if ((rc = update_glag_array(isid_add, glag_no, members)) != VTSS_OK) {
        T_E("(aggr_stack_port_add) Could not update array");
        return rc;
    } 

    /* Add the new GLAG entry (UPSPN/UPSID format) to all ISIDs */
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        if (!msg_switch_exists(sit.isid)) {
            continue;
        }
        msg = malloc(sizeof(*msg));
        if (msg) {
            msg->msg_id = AGGR_MSG_ID_ADD_REQ;
            msg->aggr_no = glag_no;
            msg->conf = (sit.isid == isid_add) ? conf : 0;            
            AGGR_CRIT_ENTER();   
            msg->grp_speed = aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][glag_no];
            for (gentry = VTSS_GLAG_PORT_START; gentry < VTSS_GLAG_PORT_END; gentry++) {
                msg->members.entries[gentry] = aggrglb.glag_ram_entries[glag_no - AGGR_MGMT_GLAG_START][gentry].ups;
            }            

            AGGR_CRIT_EXIT();   
            msg_tx(VTSS_MODULE_ID_AGGR, sit.isid, msg, sizeof(*msg));
        }
    }

    return VTSS_OK;
}
#endif

#if defined(VTSS_FEATURE_VSTAX_V1)
/* Add a aggr group to a switch in the stack */
static vtss_rc aggr_vstax_v1_port_add(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, 
                                   aggr_mgmt_group_t  *members)
{
    aggr_msg_add_req_t           *msg;   
    aggr_glag_members_t          aggr_members;
    vtss_port_no_t               port_no;

    T_D("(aggr_stack_port_add) isid:%d Agg group:%u",isid,aggr_no);

    if (!AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
        T_E("(aggr_stack_port_add) Agg group:%u is not a legal aggr id",aggr_no);
        return AGGR_ERROR_PARM;
    }
    
    if (msg_switch_is_local(isid)) {
        /* As this is the local swith we can take a shortcut  */      
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            aggr_members.member[port_no] = members->member[port_no];
        }
        return aggr_local_port_members_add(aggr_no, &aggr_members);
    } else {        
        msg = malloc(sizeof(*msg));
        if (msg) {
            msg->msg_id = AGGR_MSG_ID_ADD_REQ;
            msg->aggr_no = aggr_no;
            for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
                msg->members.member[port_no] = members->member[port_no]; 
            }
            msg_tx(VTSS_MODULE_ID_AGGR, isid, msg, sizeof(*msg));    
        } else {
            return VTSS_RC_ERROR;
        }
    }
    return VTSS_OK;
}
#endif

static vtss_rc aggr_group_add(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, 
                                   aggr_mgmt_group_t  *members)
{
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE   
    /* JR stackable */ 
    return aggr_vstax_v2_port_add(isid, aggr_no, members, 1); 
#elif defined(VTSS_FEATURE_VSTAX_V1)     
    /* Lu28 */
    return aggr_vstax_v1_port_add(isid, aggr_no, members); 
#else 
    /* Lu26, JR/JR48 standalone */
    vtss_port_no_t port_no;
    port_status_t  port_status; 
    AGGR_CRIT_ENTER();   
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        if (members->member[port_no]) {
            aggrglb.port_aggr_member[port_no] = aggr_no;
        } else if (!members->member[port_no] && aggrglb.port_aggr_member[port_no] == aggr_no) {
            aggrglb.port_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
            aggrglb.port_active_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
        }   

        if (aggrglb.port_aggr_member[port_no] == aggr_no) {
            if (port_mgmt_status_get(isid, port_no, &port_status) != VTSS_OK) {
                T_E("Could not get port info");
                AGGR_CRIT_EXIT();   
                return VTSS_UNSPECIFIED_ERROR;
            }
            if (port_status.status.link) {
                aggrglb.port_active_aggr_member[port_no] = aggr_no;
            } else {
                aggrglb.port_active_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
            }
        }
    }
    AGGR_CRIT_EXIT();   
    /* Apply to chip API */
    return vtss_aggr_port_members_set(NULL, aggr_no-AGGR_MGMT_GROUP_NO_START, members->member); 
#endif
}

#if VTSS_SWITCH_STACKABLE 
static char *aggr_msg_id_txt(aggr_msg_id_t msg_id)
{
    char *txt;

    switch (msg_id) {
    case AGGR_MSG_ID_ADD_REQ:
        txt = "AGGR_MSG_ID_ADD_REQ";
        break;
    case AGGR_MSG_ID_DEL_REQ:
        txt = "AGGR_MSG_ID_DEL_REQ";
        break;
    case AGGR_MSG_ID_MODE_SET_REQ:
        txt = "AGGR_MSG_ID_MODE_SET_REQ";
        break;
    case AGGR_MSG_ID_GLAG_UPDATE_REQ:
        txt = "AGGR_MSG_ID_GLAG_UPDATE_REQ";
        break;
    case AGGR_MSG_ID_PORTMAP_REQ:
        txt = "AGGR_MSG_ID_PORTMAP_REQ";
        break;
    case AGGR_MSG_ID_PORTMAP_RESP:
        txt = "AGGR_MSG_ID_PORTMAP_RESP";
        break;
     default:
        txt = "?";
        break;
    }
    return txt;
}

/* Receive message from the msg module and prepare a response */
static BOOL aggr_msg_rx(void *contxt, const void * const rx_msg, const size_t len, const vtss_module_id_t modid, const ulong isid)
{
    aggr_msg_id_t msg_id = *(aggr_msg_id_t *)rx_msg;
    
    T_N("Rx: %s, len: %zd, isid: %u", aggr_msg_id_txt(msg_id), len, isid);

    switch (msg_id) {
    case AGGR_MSG_ID_ADD_REQ:
    {
        aggr_msg_add_req_t              *msg_rx = (aggr_msg_add_req_t *)rx_msg;

        if (!AGGR_MGMT_GROUP_IS_AGGR(msg_rx->aggr_no)) {
            T_E("(aggr_msg_rx) Agg group:%u is not a legal aggr id",msg_rx->aggr_no);
            break;
        }
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
        if ((apply_glag_ports_to_api(msg_rx->aggr_no, &msg_rx->members, msg_rx->conf, msg_rx->grp_speed)) != VTSS_OK) {
            T_W("(mac_msg_rx)Could not glag entry");
        }
#else        
        if (aggr_local_port_members_add(msg_rx->aggr_no, &msg_rx->members) != VTSS_OK) {
            T_W("(mac_msg_rx)Could not add llag entry");
        }
#endif
        break;
    }

    case AGGR_MSG_ID_DEL_REQ:
    {
        aggr_msg_del_req_t              *msg_rx = (aggr_msg_del_req_t *)rx_msg;
        int                             group_no;
      
        T_I("(AGGR_MSG_ID_DEL_REQ) Agg group:%u ",msg_rx->aggr_no);

        if (msg_rx->aggr_no == AGGR_ALL_GROUPS) {
            T_D("Deleting all aggr groups in this switch");
            for (group_no = AGGR_MGMT_GROUP_NO_START; group_no < AGGR_MGMT_GROUP_NO_END; group_no++) {
                if (group_no == AGGR_UPLINK) 
                    continue;
                
                if (aggr_local_group_del(group_no) != VTSS_OK) {
                    T_W("(mac_msg_rx)Could not del aggr entry");
                }
            }
        } else {            
            if ((aggr_local_group_del(msg_rx->aggr_no)) != VTSS_OK) {
                T_W("(mac_msg_rx)Could not del aggr entry");
            }
        }

        break;
    }

    case AGGR_MSG_ID_MODE_SET_REQ:
    {
        aggr_msg_mode_set_req_t         *msg_rx = (aggr_msg_mode_set_req_t *)rx_msg;   
      
        if (aggr_local_mode_set(&msg_rx->mode) != VTSS_OK) {
            T_W("(mac_msg_rx)Could not set aggr mode");
        }
        break;
    }
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    case AGGR_MSG_ID_GLAG_UPDATE_REQ:
    {
        aggr_msg_glag_update_req_t      *msg_rx = (aggr_msg_glag_update_req_t *)rx_msg;   
        BOOL save_to_flash = 0;
        AGGR_CRIT_ENTER();
        /* If group speed is undefined then update the speed with speed from slave */
        if (aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][msg_rx->aggr_no] == VTSS_SPEED_UNDEFINED && (msg_rx->grp_speed != VTSS_SPEED_UNDEFINED)) {
            aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][msg_rx->aggr_no] = msg_rx->grp_speed;
            aggrglb.aggr_config_stack.switch_id[VTSS_ISID_LOCAL][msg_rx->aggr_no-AGGR_MGMT_GROUP_NO_START].speed = msg_rx->grp_speed;
            save_to_flash = 1;
        }
        AGGR_CRIT_EXIT();
        if (save_to_flash) {
            (void)aggr_save_to_flash();
        }
        if (aggr_vstax_v2_port_add(isid, msg_rx->aggr_no, &msg_rx->members, 0) != VTSS_OK) {
            T_W("Could not update GLAG group:%u",msg_rx->aggr_no);
        }
        break;
    }
    case AGGR_MSG_ID_PORTMAP_REQ:
    {
        aggr_msg_portmap_resp_t  *msg;   
        vtss_port_map_t          port_map[VTSS_PORT_ARRAY_SIZE];
        vtss_port_no_t           port_no;
        u32                      port_count = port_isid_port_count(VTSS_ISID_LOCAL);

        if (vtss_port_map_get(NULL, port_map) != VTSS_RC_OK) {
            break;
        }

        msg = malloc(sizeof(*msg));
        if (msg) {
            msg->msg_id = AGGR_MSG_ID_PORTMAP_RESP;
            for(port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                msg->chip_port[port_no] = port_map[port_no].chip_port;
            }
            msg_tx(VTSS_MODULE_ID_AGGR, isid, msg, sizeof(*msg));
        }
        break;
    }
    case AGGR_MSG_ID_PORTMAP_RESP:
    {
        aggr_msg_portmap_resp_t      *msg_rx = (aggr_msg_portmap_resp_t *)rx_msg; 
        vtss_port_no_t               port_no;
        u32                          port_count = port_isid_port_count(isid);

        AGGR_CRIT_ENTER();
        for(port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            aggrglb.port_map_global[isid][port_no] = msg_rx->chip_port[port_no];
        }

        AGGR_CRIT_EXIT();   

        /* Release the sync flag */
        cyg_flag_setbits(&aggrglb.wait_flags[isid], AGGRFLAG_WAIT_DONE);

        break;
    }
#endif /* VTSS_FEATURE_VSTAX_V2 */


    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }

    return TRUE;
}


/* Register the module to receive messages */
static vtss_rc aggr_stack_register(void)
{
    msg_rx_filter_t filter;    

    memset(&filter, 0, sizeof(filter));
    filter.cb = aggr_msg_rx;
    filter.modid = VTSS_MODULE_ID_AGGR;
    return msg_rx_filter_register(&filter);
}
#endif /* VTSS_SWITCH_STACKABLE */


/* Remove a aggr group in a switch in the stack */
static void aggr_group_del(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
#if defined(VTSS_FEATURE_VSTAX_V1)
    aggr_msg_del_req_t            *msg;
    aggr_mgmt_group_no_t          group_no;
    
    if (msg_switch_is_local(isid)) {
        /* As this is the local swith we can take a shortcut  */
        if (aggr_no == AGGR_ALL_GROUPS) {
            T_D("Deleting all aggr groups in this switch");
            for (group_no = AGGR_MGMT_GROUP_NO_START; group_no < AGGR_MGMT_GROUP_NO_END; group_no++) {
                if (group_no == AGGR_UPLINK) {
                    continue;
                }
               

                if (aggr_local_group_del(group_no) != VTSS_OK) {
                    T_W("Could not del aggr group");
                }
                AGGR_CRIT_ENTER();
                aggrglb.aggr_group_speed[isid][group_no] = VTSS_SPEED_UNDEFINED;
                AGGR_CRIT_EXIT();
            }
        } else {            
            if ((aggr_local_group_del(aggr_no)) != VTSS_OK) {
                T_W("Could not del aggr group");
            }
            AGGR_CRIT_ENTER();
            aggrglb.aggr_group_speed[isid][aggr_no] = VTSS_SPEED_UNDEFINED;
            AGGR_CRIT_EXIT();
        }        
    } else {
        msg = malloc(sizeof(*msg));
        if (msg) {
            msg->msg_id = AGGR_MSG_ID_DEL_REQ;
            msg->aggr_no = aggr_no;          
            msg_tx(VTSS_MODULE_ID_AGGR, isid, msg, sizeof(*msg));        
        }
    }
#else
    /* 'Delete' is actually an 'Add' with an empty port list */
    aggr_mgmt_group_t     mgmt_members;
    aggr_mgmt_group_no_t          group_no;
    vtss_isid_t          isid_tmp = isid;
    memset(&mgmt_members, 0, sizeof(mgmt_members));        
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    isid_tmp = VTSS_ISID_LOCAL;
#endif
    if (aggr_no == AGGR_ALL_GROUPS) {
        for (group_no = AGGR_MGMT_GROUP_NO_START; group_no < AGGR_MGMT_GROUP_NO_END; group_no++) {
            if (group_no == AGGR_UPLINK) 
                continue;
                       
            if (aggr_group_add(isid, group_no, &mgmt_members) != VTSS_OK) {
                T_W("Could not del GLAG group");
            }
            AGGR_CRIT_ENTER();
            aggrglb.aggr_group_speed[isid_tmp][group_no] = VTSS_SPEED_UNDEFINED;
            AGGR_CRIT_EXIT();
        }
        
    } else {
        if (aggr_group_add(isid, aggr_no, &mgmt_members) != VTSS_OK) {
            T_W("Could not del group:%u",aggr_no);
        }
        AGGR_CRIT_ENTER();
        if (number_of_glag_members(aggr_no) == 0) {
            aggrglb.aggr_group_speed[isid_tmp][aggr_no] = VTSS_SPEED_UNDEFINED;
        }
        AGGR_CRIT_EXIT();
    }
#endif
}

/* Set the aggr mode in a switch in the stack */
static void aggr_stack_mode_set(vtss_isid_t isid, vtss_aggr_mode_t *mode)
{
    aggr_msg_mode_set_req_t       *msg;

    if (msg_switch_is_local(isid)) {
        /* As this is the local swith we can take a shortcut  */

        if (aggr_local_mode_set(mode) != VTSS_OK)
            T_E("(aggr_stack_mode_set) Could not set mode");
    } else {        
        msg = malloc(sizeof(*msg));
        if (msg) {
            msg->msg_id = AGGR_MSG_ID_MODE_SET_REQ;
            msg->mode = *mode;
            msg_tx(VTSS_MODULE_ID_AGGR, isid, msg, sizeof(*msg));    
        }
    }
}
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
static void aggr_stack_port_map_get(vtss_isid_t isid) {
    if (msg_switch_is_local(isid)) {
        u32 port_count = port_isid_port_count(isid);
        vtss_port_map_t          port_map[VTSS_PORT_ARRAY_SIZE];
        vtss_port_no_t           port_no;

       (void)vtss_port_map_get(NULL, port_map);
       AGGR_CRIT_ENTER();
       for(port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
           aggrglb.port_map_global[isid][port_no] = port_map[port_no].chip_port;
       }
       AGGR_CRIT_EXIT();
    } else {
        aggr_msg_mode_set_req_t       *msg;    
        msg = malloc(sizeof(*msg));
        if (msg) {
            msg->msg_id = AGGR_MSG_ID_PORTMAP_REQ;
            msg_tx(VTSS_MODULE_ID_AGGR, isid, msg, sizeof(*msg));    
        }
    }
}
#endif

/* Add configuration to the new switch in the stack  */
static void aggr_switch_add(vtss_isid_t isid)
{
    aggr_mgmt_group_no_t  group_no;
    BOOL                  active_group=0;
    aggr_mgmt_group_t     mgmt_members;
    port_iter_t           pit;
    vtss_aggr_mode_t      mode;
    vtss_isid_t           isid_tmp = isid;
    vtss_isid_t           isid_id = isid-VTSS_ISID_START;

    if (!msg_switch_exists(isid)){
        return;
    }

    memset(&mgmt_members, 0, sizeof(mgmt_members));   

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    isid_tmp = isid_id = VTSS_ISID_LOCAL; /* we only need 1 table for GLAGs */
    AGGR_CRIT_ENTER();
    cyg_flag_init(&aggrglb.wait_flags[isid]);
    AGGR_CRIT_EXIT();
    (void)aggr_stack_port_map_get(isid);    
    /* Wait for reply from switch because the portmap must be updated before continuing */
    if (!msg_switch_is_local(isid)) {
        (void)cyg_flag_wait( &aggrglb.wait_flags[isid], AGGRFLAG_WAIT_DONE | AGGRFLAG_COULD_NOT_TX, 
                             CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR);
    }

#endif
    /* Remove all groups */
    (void)aggr_group_del(isid, AGGR_ALL_GROUPS);
  
    /* Run through the groups and update if there is a group with members */ 
    for (group_no = AGGR_MGMT_GROUP_NO_START; group_no < AGGR_MGMT_GROUP_NO_END; group_no++) {
        if (group_no == AGGR_UPLINK) {
            continue;
        }

        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            AGGR_CRIT_ENTER();
            mgmt_members.member[pit.iport] = VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START]\
                                             [group_no-AGGR_MGMT_GROUP_NO_START].member,(pit.iport-VTSS_PORT_NO_START));

            if (mgmt_members.member[pit.iport]) {
                active_group = 1;               
            }
            AGGR_CRIT_EXIT();
        }
        AGGR_CRIT_ENTER();
        aggrglb.aggr_group_speed[isid_tmp][group_no] = aggrglb.aggr_config_stack.switch_id[isid_id][group_no-AGGR_MGMT_GROUP_NO_START].speed;
        AGGR_CRIT_EXIT();

        if (active_group) {
            /* Add the group */
            if (!AGGR_MGMT_GROUP_IS_AGGR(group_no)) {
                T_E("(aggr_switch_add) Group:%u is not an legal aggr id",group_no);
                return;
            }
            if (aggr_group_add(isid, group_no, &mgmt_members) != VTSS_OK) {
                return;
            }           
            active_group = 0;                                            
            /* Inform subscribers of aggregation changes */
            (void)aggr_change_callback(isid, group_no);   
        }        
    }

    /* Set the aggregation Mode  */
    AGGR_CRIT_ENTER();
    mode = aggrglb.aggr_config_stack.mode;
    AGGR_CRIT_EXIT();
    aggr_stack_mode_set(isid, &mode);    
}

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
/* Delete a unit  */
static void aggr_switch_del(vtss_isid_t isid)
{
    aggr_mgmt_group_t     glag_members;
    aggr_mgmt_group_no_t  glag_no;
    vtss_port_no_t        port_no;
    BOOL                  member;

    memset(&glag_members, 0, sizeof(glag_members));   
    AGGR_CRIT_ENTER();
    cyg_flag_setbits(&aggrglb.wait_flags[isid], AGGRFLAG_WAIT_DONE);
    AGGR_CRIT_EXIT();
    for (glag_no = AGGR_MGMT_GROUP_NO_START; glag_no < AGGR_MGMT_GROUP_NO_END; glag_no++) {
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            AGGR_CRIT_ENTER();
            member = VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START][glag_no-AGGR_MGMT_GROUP_NO_START].member,
                                      (port_no-VTSS_PORT_NO_START));
            AGGR_CRIT_EXIT();                
            if (member) {
                break;
            }
        }
        if (member) {
            if (aggr_vstax_v2_port_add(isid, glag_no, &glag_members, 0) != VTSS_OK) {
                T_E("(aggr_switch_del) Could not delete switch");
            }                
        }
    }
}

#endif /* VTSS_FEATURE_VSTAX_V2 */

static vtss_rc aggr_mgmt_port_sid_invalid(vtss_isid_t isid, vtss_port_no_t port_no, aggr_mgmt_group_no_t aggr_no, BOOL set)
{
    if (isid != VTSS_ISID_LOCAL && !msg_switch_is_master()) {
        T_W("not master");
        return AGGR_ERROR_NOT_MASTER;
    }

    if (port_no >= port_isid_port_count(isid)) {
        T_W("illegal port_no: %u", port_no);
        return AGGR_ERROR_INVALID_PORT;
    }

    if (isid >= VTSS_ISID_END) {
        T_W("illegal isid: %d", isid);
        return AGGR_ERROR_INVALID_ISID;
    }

    if (!AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
        T_W("%u is not a legal aggregation group.",aggr_no);
        return AGGR_ERROR_INVALID_ID;
    }

    if (set && isid == VTSS_ISID_LOCAL) {
        T_W("SET not allowed, isid: %d", isid);
        return AGGR_ERROR_GEN;
    }

    return VTSS_RC_OK;
}

static vtss_rc aggr_return_error(vtss_rc rc, vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    AGGR_CRIT_ENTER();
    aggrglb.aggr_group_speed[isid][aggr_no] = VTSS_SPEED_UNDEFINED;
    AGGR_CRIT_EXIT();
    return rc;
}

/****************************************************************************/
/*  Management/API functions                                                */
/****************************************************************************/
char *aggr_error_txt(vtss_rc rc)
{
    switch (rc) {
    case AGGR_ERROR_GEN:
        return "Aggregation generic error";
    case AGGR_ERROR_PARM:
        return "Illegal parameter";
    case AGGR_ERROR_REG_TABLE_FULL:
        return "Registration table full";
    case AGGR_ERROR_REQ_TIMEOUT:
        return "Timeout on message request";
    case AGGR_ERROR_STACK_STATE:
        return "Illegal MASTER/SLAVE state";
    case AGGR_ERROR_GROUP_IN_USE:
        return "Group already in use";
    case AGGR_ERROR_PORT_IN_GROUP:
        return "Port already in another group";
    case AGGR_ERROR_LACP_ENABLED:
        return "LACP aggregation is enabled";
    case AGGR_ERROR_DOT1X_ENABLED:
        return "DOT1X is enabled";
    case AGGR_ERROR_ENTRY_NOT_FOUND:
        return "Entry not found";
    case AGGR_ERROR_HDX_SPEED_ERROR:
        return "Illegal duplex or speed state";
    case AGGR_ERROR_MEMBER_OVERFLOW:
        return "To many port members";
    case AGGR_ERROR_INVALID_ID:
        return "Invalid group id";
    default:
        return "Aggregation unknown error";
    }
}

vtss_rc aggr_mgmt_port_members_add(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t *members)
{
    vtss_port_no_t           port_no;

    BOOL                     first = TRUE;
    port_status_t            port_status;
    u32                      port_count = port_isid_port_count(isid);  
    vtss_rc                  rc;
    vtss_isid_t              isid_local=isid, isid_id = (isid-VTSS_ISID_START);
    vtss_port_speed_t        group_speed = VTSS_SPEED_UNDEFINED;
    aggr_mgmt_group_no_t     gr;

    T_D("Doing a aggr add group %u at switch isid:%d",aggr_no, isid);    

    VTSS_RC(aggr_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, aggr_no, 1));

    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        /* Check for stack port */
        if (members->member[port_no] && port_isid_port_no_is_stack(isid, port_no)) {
            return AGGR_ERROR_PARM;
        }  

        /* Check if the member port is already part of another group */
        for (gr = AGGR_MGMT_GROUP_NO_START; gr < AGGR_MGMT_GROUP_NO_END; gr++) {        
            if (gr == aggr_no) {
                continue;
            }
            AGGR_CRIT_ENTER();
            if (VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START] \
                                 [gr-AGGR_MGMT_GROUP_NO_START].member, (port_no-VTSS_PORT_NO_START)) && members->member[port_no]) {
                AGGR_CRIT_EXIT();
                return AGGR_ERROR_PORT_IN_GROUP;
            }
            AGGR_CRIT_EXIT();    
        }
    }

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    isid_local = VTSS_ISID_LOCAL; /* we only need 1 table for GLAGs */
    isid_id = VTSS_ISID_LOCAL; 
#endif
     
#ifdef VTSS_SW_OPTION_LACP
    vtss_lacp_port_config_t  conf;
    aggr_mgmt_group_member_t members_tmp;
    int aid;
    
    /* Check if the ports are occupied by LACP */
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {             
        if (members->member[port_no]) {
            if ((rc=lacp_get_port_config(isid, port_no, &conf)) != VTSS_OK) {
                return rc;
            }
            if (conf.enable_lacp) {
                return AGGR_ERROR_LACP_ENABLED;                    
            }
        }
    }
    
    /* Check if the group is occupied by LACP */
    if (AGGR_MGMT_GROUP_IS_LAG(aggr_no)) {            
        if (aggr_mgmt_lacp_members_get(isid, aggr_no,  &members_tmp, 0) == VTSS_OK)
            return AGGR_ERROR_GROUP_IN_USE;
    } else {        
        AGGR_CRIT_ENTER();
        for (aid = VTSS_PORT_NO_START; aid < (VTSS_LACP_MAX_PORTS + VTSS_PORT_NO_START); aid++) {                
            if (aggrglb.aggr_lacp[aid].aggr_no < AGGR_MGMT_GROUP_NO_START)
                continue;
            
            if (aggrglb.aggr_lacp[aid].aggr_no == aggr_no) {
                AGGR_CRIT_EXIT();
                return AGGR_ERROR_GROUP_IN_USE;
            }
        }
        AGGR_CRIT_EXIT();
    }
#endif /* VTSS_SW_OPTION_LACP */

  
#if defined(VTSS_SW_OPTION_DOT1X)
    dot1x_switch_cfg_t switch_cfg;
    
    /* Check if dot1x is enabled */    
    if (dot1x_mgmt_switch_cfg_get(isid,&switch_cfg) != VTSS_OK) {
        return VTSS_UNSPECIFIED_ERROR;  
    }        
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {             
        if (members->member[port_no] && switch_cfg.port_cfg[port_no - VTSS_PORT_NO_START].admin_state != NAS_PORT_CONTROL_FORCE_AUTHORIZED) {
            return AGGR_ERROR_DOT1X_ENABLED;
        }
    }
#endif /* VTSS_SW_OPTION_DOT1X */              

    /* Verify that all port are FDX and same speed */
    AGGR_CRIT_ENTER();                
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {             
        if (members->member[port_no]) {

            if (port_mgmt_status_get(isid, port_no, &port_status) != VTSS_OK) {
                T_E("Could not get port info");
                AGGR_CRIT_EXIT();                
                return aggr_return_error(VTSS_UNSPECIFIED_ERROR, isid, aggr_no);
            } else {
                if (!port_status.status.link) {/* Link is off, port will get added */
                    continue;
                }
            }
            /* No HDX allowed */
            if (!port_status.status.fdx) {
                T_W("Half duplex ports are not allowed to join aggregation groups.");             
                AGGR_CRIT_EXIT();                
                return aggr_return_error(AGGR_ERROR_HDX_SPEED_ERROR, isid, aggr_no);
            }


#if defined(VTSS_FEATURE_VSTAX_V1)            
            /* Check GLAGs, port speed must match group speed */
            if (AGGR_MGMT_GROUP_IS_GLAG(aggr_no)) {
                if (aggrglb.aggr_group_speed[isid][aggr_no] == VTSS_SPEED_UNDEFINED) {
                    vtss_isid_t              isid_tmp;
                    for (isid_tmp = VTSS_ISID_START; isid_tmp < VTSS_ISID_END; isid_tmp++) {
                        if (aggrglb.aggr_group_speed[isid_tmp][aggr_no] != VTSS_SPEED_UNDEFINED) {
                            if (port_status.status.speed != aggrglb.aggr_group_speed[isid_tmp][aggr_no]) {
                                T_W("Port speed must be the same for all GLAG members. Port %u removed from group.",port_no);
                                AGGR_CRIT_EXIT();
                                return AGGR_ERROR_HDX_SPEED_ERROR;
                            }
                        }
                    }
                }
            }
#endif
            if (!AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
                AGGR_CRIT_EXIT();
                return aggr_return_error(VTSS_UNSPECIFIED_ERROR, isid_local, aggr_no);
            }

            /* Glags are not existing or ok. Port speed must match group speed */
            if (aggrglb.aggr_group_speed[isid_local][aggr_no] == VTSS_SPEED_UNDEFINED) {
                if (first) {
                    group_speed = port_status.status.speed;
                    aggrglb.aggr_group_speed[isid_local][aggr_no] = group_speed;
                    first = FALSE;
                } else {
                    if (group_speed != port_status.status.speed) {
                        AGGR_CRIT_EXIT();
                        T_W("Port speed must be the same for all aggregation members");
                        return aggr_return_error(AGGR_ERROR_HDX_SPEED_ERROR, isid_local, aggr_no);
                    }
                }
            } else {
                if (aggrglb.aggr_group_speed[isid_local][aggr_no] != port_status.status.speed) {
                    T_W("Port speed (%s) must be the same for all aggregation members",
                        aggrglb.aggr_group_speed[isid_local][aggr_no]==VTSS_SPEED_10M?"10M":
                        aggrglb.aggr_group_speed[isid_local][aggr_no]==VTSS_SPEED_100M?"100M":
                        aggrglb.aggr_group_speed[isid_local][aggr_no]==VTSS_SPEED_1G?"1G":"?");
                    AGGR_CRIT_EXIT();
                    return aggr_return_error(AGGR_ERROR_HDX_SPEED_ERROR, isid_local, aggr_no);
                }
            }
        }
    }
    
    AGGR_CRIT_EXIT();

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    switch_iter_t         sit;
    port_iter_t           pit;
    u32                   glag_members=0, new_members=0, local_members=0;

    AGGR_CRIT_ENTER();    
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        if (!msg_switch_exists(sit.isid)) {
            continue;
        }
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {                    
            if (VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[sit.isid-VTSS_ISID_START] \
                                 [aggr_no-AGGR_MGMT_GROUP_NO_START].member, (pit.iport-VTSS_PORT_NO_START))) {
                if (sit.isid == isid) {
                    local_members++;
                }
                glag_members++;
            }
        }
    }
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {             
        if (members->member[port_no]) {
            new_members++;
        }
    }
    if (glag_members-local_members+new_members > AGGR_MGMT_GLAG_PORTS_MAX) {
        T_W("To many members, the max is:%d",AGGR_MGMT_GLAG_PORTS_MAX);
        AGGR_CRIT_EXIT();
        return AGGR_ERROR_MEMBER_OVERFLOW;
    }      
    AGGR_CRIT_EXIT();
#endif /* VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE */
    
    if ((rc = aggr_group_add(isid, aggr_no, members)) != VTSS_OK) {
        return rc;
    }

    /* Add to stack config */
    if (!AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
        return VTSS_UNSPECIFIED_ERROR;
    }
    AGGR_CRIT_ENTER();        
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {             
        VTSS_BF_SET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START][aggr_no-AGGR_MGMT_GROUP_NO_START].member,
                    (port_no-VTSS_PORT_NO_START), members->member[port_no]);
    }
    aggrglb.aggr_config_stack.switch_id[isid_id][aggr_no-AGGR_MGMT_GROUP_NO_START].speed = aggrglb.aggr_group_speed[isid_local][aggr_no];
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if ( VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START][aggr_no-AGGR_MGMT_GROUP_NO_START].member,
                              (port_no-VTSS_PORT_NO_START))) {
            T_D("Aggr:%u  port:%u are conf agg members\n",aggr_no,port_no);
        }
    }
    AGGR_CRIT_EXIT();    

    (void)aggr_save_to_flash();

    /* Inform subscribers of aggregation changes */
    (void)aggr_change_callback(isid, aggr_no);   
    
    return VTSS_OK;
}


/* Removes all members from a aggregation group. */
vtss_rc aggr_mgmt_group_del(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    uint i;

    T_D("Doing a aggr del group %u at switch isid:%d",aggr_no,isid);    
    
    VTSS_RC(aggr_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, aggr_no, 1));
    
#ifdef VTSS_SW_OPTION_LACP
    aggr_mgmt_group_member_t members_tmp;
    /* Check if the group or ports is occupied by LACP in this isid */
    if (aggr_mgmt_lacp_members_get(isid, aggr_no,  &members_tmp, 0) == VTSS_OK) {
        return AGGR_ERROR_GROUP_IN_USE;
    }
#endif /* VTSS_SW_OPTION_LACP */
                       
    (void)aggr_group_del(isid, aggr_no);
    AGGR_CRIT_ENTER();

    /* Delete from stack config */
    for (i = 0;i < VTSS_PORTS;i++) {
        VTSS_BF_SET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START][aggr_no-AGGR_MGMT_GROUP_NO_START].member, i, 0); 
    }

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    aggrglb.aggr_config_stack.switch_id[VTSS_ISID_LOCAL][aggr_no-AGGR_MGMT_GROUP_NO_START].speed = aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][aggr_no];
#else
    aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START][aggr_no-AGGR_MGMT_GROUP_NO_START].speed = VTSS_SPEED_UNDEFINED;
#endif
    AGGR_CRIT_EXIT();
        
    (void)aggr_save_to_flash();
   
    /* Inform subscribers of aggregation changes */
    (void)aggr_change_callback(isid, aggr_no);   

    return VTSS_OK;
}


/* Get members in a given aggr group. 'Next' is used to browes trough active groups.  */
vtss_rc aggr_mgmt_port_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no,  aggr_mgmt_group_member_t *members, BOOL next)
{
    vtss_rc            rc = AGGR_ERROR_ENTRY_NOT_FOUND;
    vtss_port_no_t     port_no;
    BOOL               found_group=false;
    port_iter_t        pit;

    VTSS_RC(aggr_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, 1, 0));
    
    if (isid == VTSS_ISID_LOCAL) {
        rc = aggr_local_port_members_get(aggr_no, members, next);
    } else {
        /* Get the first active aggregation group */
        if (aggr_no == 0 && next) {
            for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {        
                if (aggr_no == AGGR_UPLINK) {
                    continue;
                }
                AGGR_CRIT_ENTER();
                (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {                    
                    if (VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START]\
                                         [aggr_no-AGGR_MGMT_GROUP_NO_START].member, (pit.iport-VTSS_PORT_NO_START))) {
                        T_D("Found (next) static members in aggr:%u, isid:%d",aggr_no,isid);   
                        found_group = 1;
                        break;
                    }
                } 
                AGGR_CRIT_EXIT();
                if (found_group) {
                    break;
                }
            }
            if (!found_group) {
                return AGGR_ERROR_ENTRY_NOT_FOUND;
            }
            
        } else if (aggr_no != 0 && next) {
            /* Get the next active aggregation group */
            for (aggr_no = (aggr_no+1); aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {        
                if (aggr_no == AGGR_UPLINK) {
                    continue;
                }
                AGGR_CRIT_ENTER();
                (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {                    
                    if (VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START]\
                                         [aggr_no-AGGR_MGMT_GROUP_NO_START].member, (pit.iport-VTSS_PORT_NO_START))) {
                        T_D("Found (next) static members in aggr:%u, isid:%d",aggr_no,isid);   
                        found_group = 1;
                        break;
                    }
                }
                AGGR_CRIT_EXIT();
                if (found_group) {
                    break;
                }
            }
            if (!found_group) {
                return AGGR_ERROR_ENTRY_NOT_FOUND;
            }
        }
        
        members->aggr_no = aggr_no;
        if (AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
            AGGR_CRIT_ENTER();
            for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
                members->entry.member[port_no] =    
                    VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid-VTSS_ISID_START]\
                                     [aggr_no-AGGR_MGMT_GROUP_NO_START].member, (port_no-VTSS_PORT_NO_START));
               
                if (members->entry.member[port_no]) {
                    rc = VTSS_OK;               
                }
            }
            AGGR_CRIT_EXIT();
        } else {
            return AGGR_ERROR_PARM;
        }                        
    }
    return rc;        
}
/* Get the speed of the group */
vtss_port_speed_t aggr_mgmt_speed_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    vtss_port_speed_t spd;
    AGGR_CRIT_ENTER();
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE 
    spd = aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][aggr_no];
#else
    spd = aggrglb.aggr_group_speed[isid][aggr_no];
#endif
    AGGR_CRIT_EXIT();
    return spd;
}

#ifdef VTSS_SW_OPTION_LACP

/* Find the next vacant aggregation id, LLAG or GLAG     */
aggr_mgmt_group_no_t aggr_find_group(vtss_isid_t isid, aggr_mgmt_group_no_t start_group) 
{
    aggr_mgmt_group_no_t  aggr_no;
    aggr_mgmt_group_member_t group;
    int                    aggr_taken=0;
    switch_iter_t          sit;


    for (aggr_no = start_group; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {
        if (aggr_no == AGGR_UPLINK)
            continue;        
       
        if (AGGR_MGMT_GROUP_IS_LAG(start_group)) {
 
            /* Static groups */
            if (aggr_mgmt_port_members_get(isid, aggr_no, &group, 0) == VTSS_OK) {
                continue;
            }
            
            /* LACP groups */
            if (aggr_mgmt_lacp_members_get(isid, aggr_no, &group, 0) == VTSS_OK) {
                continue;
            }
        } else {       
            /* For GLAGs we need to run through all ISIDs */
            (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
            while (switch_iter_getnext(&sit)) {
                if (!msg_switch_exists(sit.isid)) {
                    continue;
                }
                aggr_taken = 0;
                if (aggr_mgmt_lacp_members_get(sit.isid, aggr_no, &group, 0) == VTSS_OK) {
                    aggr_taken = 1;
                    break;
                }

                if (aggr_mgmt_port_members_get(sit.isid, aggr_no, &group, 0) == VTSS_OK) {
                    aggr_taken = 1;
                    break;
                }
                
            }                        
            if (aggr_taken) {
                continue;
            }
        }

        return aggr_no;        
    }
    
    /* No vacancy */
    return 0;
}


vtss_rc aggr_mgmt_lacp_members_add(uint aid, int new_port)
{
    aggr_mgmt_group_no_t     aggr_no=0;
    int                      start_group,llag=0,member_cnt=0;
    vtss_isid_t              isid;
    vtss_port_no_t           isid_port,port_no;
    aggr_mgmt_group_t        entry;
    u32                      port_count;  

    memset(&entry, 0, sizeof(entry));                         
    T_D("aggr_mgmt_lacp_members_add.  Aid:%d port:%d",aid,new_port);
    
    /* Port to join a aggregation must wait until another port joins before the group is created */
    /* When 2 port are ready to join, a group is created with the first vacant Aggr id available */ 

    /* Convert new_port to isid,port */
    if (!l2port2port(new_port, &isid, &isid_port)) {
        T_E("Could not find a isid,port for l2_proto_port_t:%d\n",new_port);
        return AGGR_ERROR_PARM;
    }
    port_count = port_isid_port_count(isid);  
    AGGR_CRIT_ENTER();
    if (!aggrglb.aggr_lacp[aid].aggr_no) {
        /* First port to join. Find the real aggr_id when the next port joins. */
        aggrglb.aggr_lacp[aid].aggr_no = LACP_ONLY_ONE_MEMBER; 
        aggrglb.aggr_lacp[aid].members[new_port] = 1;  
        AGGR_CRIT_EXIT();
        return VTSS_OK;           
    } else if  (aggrglb.aggr_lacp[aid].aggr_no == LACP_ONLY_ONE_MEMBER) {
        /* Second port to join. Find the aggregation id and create the aggregation */
        T_D("Creating aggregation with 2 ports at aid:%d\n",aid);

        /* Check if the 2 ports are within the same ISID */
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            l2_port_no_t l2port;
            if (port_isid_port_no_is_stack(isid, port_no))
                continue;
            l2port = L2PORT2PORT(isid, port_no);
            if (aggrglb.aggr_lacp[aid].members[l2port]) 
                llag = 1; /* Found the port, create a llag */
        }
                
        if (!llag) {
            start_group = AGGR_MGMT_GLAG_START;
        } else {
            start_group = AGGR_MGMT_GROUP_NO_START;
        }

    } else {
        /* New port wants to join an existing aggregation */
        /* This could mean that we have to  move LLAG to GLAG. */
        /* If the new port is within a different switch than the existing ports */
        /* the LLAG will be deleted and GLAG created - if there is a vacant GLAG  */
        l2port_iter_t l2pit;

        (void) l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_PHYS);
        while(l2port_iter_getnext(&l2pit)) {
            if (aggrglb.aggr_lacp[aid].members[l2pit.l2port]) 
                member_cnt++;
        }

        if (AGGR_MGMT_GROUP_IS_LAG(aggrglb.aggr_lacp[aid].aggr_no)) {
            if (member_cnt >= AGGR_MGMT_LAG_PORTS_MAX) {
                T_W("There can only be %d ports in a LLAG",AGGR_MGMT_LAG_PORTS_MAX);
                AGGR_CRIT_EXIT();
                return AGGR_ERROR_REG_TABLE_FULL;
            }
        } else {
            if (member_cnt >= AGGR_MGMT_GLAG_PORTS_MAX) {
                T_W("There can only be %d ports in a GLAG",AGGR_MGMT_GLAG_PORTS_MAX);
                AGGR_CRIT_EXIT();
                return AGGR_ERROR_REG_TABLE_FULL;
            }
        }

        start_group = AGGR_MGMT_GROUP_NO_END; /* Disable new group search */        
        if (AGGR_MGMT_GROUP_IS_LAG(aggrglb.aggr_lacp[aid].aggr_no)) {
            /* Check wether the new member is within the same ISID as the existing members */
            (void) l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_PHYS);
            while(l2port_iter_getnext(&l2pit)) {
                if (aggrglb.aggr_lacp[aid].members[l2pit.l2port]) {
                    if (l2pit.isid != isid) {                        
                        
                        AGGR_CRIT_EXIT();
                        if (!aggr_find_group(VTSS_ISID_LOCAL, AGGR_MGMT_GLAG_START)) {
                            T_W("All GLAGs in use!");
                            return AGGR_ERROR_REG_TABLE_FULL;
                        }                
                        if (member_cnt >= AGGR_MGMT_GLAG_PORTS_MAX) {
                            T_W("There can only be %d ports in a GLAG",AGGR_MGMT_GLAG_PORTS_MAX);
                            return AGGR_ERROR_REG_TABLE_FULL;
                        }
                        AGGR_CRIT_ENTER();
                        aggrglb.aggr_lacp[aid].members[new_port] = 1;
                        T_D("Deleting LLAG id %u and creating a GLAG instead.\n",aggrglb.aggr_lacp[aid].aggr_no);
                        /* Delete the LAG */                            
                        (void)aggr_group_del(l2pit.isid, aggrglb.aggr_lacp[aid].aggr_no);

                        /* Inform subscribers of aggregation changes */
                        (void)aggr_change_callback(l2pit.isid, aggrglb.aggr_lacp[aid].aggr_no);   
                                                
                        /* Find a GLAG id and create the GLAG */
                        start_group = AGGR_MGMT_GLAG_START;
                    }
                    break;                
                } 
            }
        }
        aggrglb.aggr_lacp[aid].members[new_port] = 1;
    }
    AGGR_CRIT_EXIT();
    if (start_group != AGGR_MGMT_GROUP_NO_END) {
        aggr_no = aggr_find_group(isid, start_group);
        if (aggr_no == 0) {
            T_W("All %s in use!",(start_group == AGGR_MGMT_GLAG_START)?"GLAGs":"LLAGs and GLAGs");
            return AGGR_ERROR_REG_TABLE_FULL;
        } else {
            AGGR_CRIT_ENTER();
            aggrglb.aggr_lacp[aid].aggr_no =  aggr_no;
            aggrglb.aggr_lacp[aid].members[new_port] = 1;
            AGGR_CRIT_EXIT();
        }
    }
    
    T_D("Using aggr no:%u for aid:%d\n",aggr_no,aid);                            
    AGGR_CRIT_ENTER();
    switch_iter_t sit;
    /* If the groups is new and is a GLAG then we have to go through all members of all ISID's */
    if (start_group == AGGR_MGMT_GLAG_START) {
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    } else {
        /* Add a new port to an existing aggr group (lag or glag) */
        (void) switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    }
    while (switch_iter_getnext(&sit)) {
        if (!msg_switch_exists(sit.isid)) {
            continue;
        }
        l2port_iter_t l2unit;
        /* For this isid, setup all ports being members of aggegation */
        (void) l2port_iter_init(&l2unit, sit.isid, L2PORT_ITER_TYPE_PHYS);
        memset(entry.member, 0, sizeof(entry.member));
        while (l2port_iter_getnext(&l2unit)) {
            entry.member[l2unit.iport] = aggrglb.aggr_lacp[aid].members[l2unit.l2port];
            T_N("l2port %d: ISID %d Port %d member of aggr %d = %d", 
                l2unit.l2port, l2unit.isid, l2unit.iport, aid, entry.member[l2unit.iport]);
        }
        /* Apply the aggregation to the chip */
        AGGR_CRIT_EXIT();
        if (aggr_group_add(sit.isid, aggrglb.aggr_lacp[aid].aggr_no, &entry) != VTSS_OK) { /* lacp member add */
            return VTSS_UNSPECIFIED_ERROR;
        }
        AGGR_CRIT_ENTER();
    }
    AGGR_CRIT_EXIT();
    T_D("Added isid:%d, port:%u to aggr id %u\n", isid, isid_port, aggrglb.aggr_lacp[aid].aggr_no);
    /* Inform subscribers of aggregation changes */
    (void)aggr_change_callback(isid, aggrglb.aggr_lacp[aid].aggr_no);   
    return VTSS_OK;
}

void aggr_mgmt_lacp_member_del(uint aid, int old_port)
{
    bool empty = 1;
    int                  member_cnt=0,last_port=0;
    vtss_port_no_t       isid_port, port_no;
    vtss_isid_t          isid, isid_cmp=0;
    l2_port_no_t         l2port;
    l2port_iter_t        l2pit;
    aggr_mgmt_group_t    entry;
    BOOL                 first=1,del_glag=TRUE;
    aggr_mgmt_group_no_t aggr_no, tmp_no;
    u32                  port_count;  

    memset(&entry, 0, sizeof(entry));
    T_D("aggr_mgmt_lacp_members_del.  Aid:%d port:%d",aid,old_port);

    if (!l2port2port(old_port, &isid, &isid_port)) {
        T_E("Could not find a isid,port for l2_proto_port_t:%d\n",old_port);
        return;
    }
    AGGR_CRIT_ENTER();
    port_count = port_isid_port_count(isid);  
    aggrglb.aggr_lacp[aid].members[old_port] = 0;

    if (aggrglb.aggr_lacp[aid].aggr_no == LACP_ONLY_ONE_MEMBER) {
        aggrglb.aggr_lacp[aid].aggr_no = 0;
        AGGR_CRIT_EXIT();
        return;
    }

    if (!AGGR_MGMT_GROUP_IS_AGGR(aggrglb.aggr_lacp[aid].aggr_no)) {
        T_E("%u is not a legal aggregation group.",aggrglb.aggr_lacp[aid].aggr_no);
        AGGR_CRIT_EXIT();
        return;
    }

    (void) l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_PHYS);
    while(l2port_iter_getnext(&l2pit)) {
        if (aggrglb.aggr_lacp[aid].members[l2pit.l2port]) {
            empty = 0; /* Group not empty */
            member_cnt++;
            last_port = l2pit.l2port;
        }         
    }

    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {                
        if (port_isid_port_no_is_stack(isid, port_no)) {
            continue;
        }
        l2port = L2PORT2PORT(isid, port_no);
        entry.member[port_no] = aggrglb.aggr_lacp[aid].members[l2port];
    }

    AGGR_CRIT_EXIT();
    if (!empty) {
        /* Modify the aggregation */
        if (aggr_group_add(isid, aggrglb.aggr_lacp[aid].aggr_no, &entry) != VTSS_OK) {
            return;
        }
    } else {
        /* Remove the aggregation */
        (void)aggr_group_del(isid, aggrglb.aggr_lacp[aid].aggr_no);
    }
    AGGR_CRIT_ENTER();
    T_D("Removed port %u from aggr id %u\n",isid_port,aggrglb.aggr_lacp[aid].aggr_no);

    tmp_no = aggrglb.aggr_lacp[aid].aggr_no;
    AGGR_CRIT_EXIT();
    /* Check if there is only one port left int the group.  If that is, remove the group */
    if (member_cnt == 1) {
        /* Find the last port ISID */
        if (!l2port2port(last_port, &isid, &isid_port)) {
            T_E("Could not find a isid,port for l2_proto_port_t:%d\n",old_port);
            return;
        }
        T_D("Only one port:%d is left.  Removing the aggregation:%d but keeping the last port on 'stand by'",last_port,aid);
        (void)aggr_group_del(isid, aggrglb.aggr_lacp[aid].aggr_no);
        /* LACP considers the last port to be member though   */
        AGGR_CRIT_ENTER();
        aggrglb.aggr_lacp[aid].aggr_no = LACP_ONLY_ONE_MEMBER; 
        aggrglb.aggr_lacp[aid].members[last_port] = 1;  
        AGGR_CRIT_EXIT();
    }                
    AGGR_CRIT_ENTER();
    if (empty) {
        T_D("aggr id %u disabled\n",aggrglb.aggr_lacp[aid].aggr_no);
        aggrglb.aggr_lacp[aid].aggr_no = 0;
    } else if (member_cnt == 1) {
        /* Do nothing  */
    } else {

        /* A port is now removed from the AGGR. */
        /* If the AGGR is a GLAG, check if the rest of the ports are connected to the same switch i.e. same isid. */
        /* If they are, change the GLAG to a LLAG  */

        if (AGGR_MGMT_GROUP_IS_GLAG(aggrglb.aggr_lacp[aid].aggr_no)) {            

            (void) l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_PHYS);
            while(l2port_iter_getnext(&l2pit)) {
                if (aggrglb.aggr_lacp[aid].members[l2pit.l2port]) {
                    
                    if (!l2port2port(l2pit.l2port, &isid, &port_no)) {
                        T_W("Could not convert l2port\n");
                        AGGR_CRIT_EXIT();
                        return;
                    }
                    
                    if (first) {
                        isid_cmp = isid;
                        first = 0;                    
                    } else {
                        if (isid_cmp != isid) {
                            /* Keep the GLAG */
                            del_glag = FALSE;
                            break;
                        }
                    }
                }
            }

            if (del_glag) {                
                T_D("Deleting GLAG id %u and creating a LLAG instead.\n",aggrglb.aggr_lacp[aid].aggr_no);
                AGGR_CRIT_EXIT();
                aggr_no = aggr_find_group(VTSS_ISID_LOCAL, AGGR_MGMT_GROUP_NO_START);
                AGGR_CRIT_ENTER();

                if (!AGGR_MGMT_GROUP_IS_LAG(aggr_no)) {
                    AGGR_CRIT_EXIT();
                    return; /* No LLAG vacancy - stick with the GLAG */
                }
                                   
                /* Delete the GLAG from all ISID's that have members */                            
                isid_cmp = 0;
                (void) l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_PHYS);
                while(l2port_iter_getnext(&l2pit)) {
                    if (aggrglb.aggr_lacp[aid].members[l2pit.l2port]) {
                        if (!l2port2port(l2pit.l2port, &isid, &port_no)) {
                            T_W("Could not convert l2port\n");
                            AGGR_CRIT_EXIT();
                            return;
                        }
                        if (isid != isid_cmp) {
                            (void)aggr_group_del(isid, aggrglb.aggr_lacp[aid].aggr_no);
                            /* Inform subscribers of aggregation changes */
                            (void)aggr_change_callback(isid, aggrglb.aggr_lacp[aid].aggr_no);         
                            isid_cmp = isid;
                        }
                    }
                }

                if (!VTSS_ISID_LEGAL(isid)) {
                    AGGR_CRIT_EXIT();
                    return;
                }
                
                /* Use the new LLAG id */
                aggrglb.aggr_lacp[aid].aggr_no = aggr_no;

                for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {                
                    if (port_isid_port_no_is_stack(isid, port_no)) {
                        continue;
                    }

                    l2port = L2PORT2PORT(isid, port_no);
                    entry.member[port_no] = aggrglb.aggr_lacp[aid].members[l2port];
                }
                
                /* Apply the new aggregation to the chip */
                if (aggr_group_add(isid, aggrglb.aggr_lacp[aid].aggr_no, &entry) != VTSS_OK) {
                    AGGR_CRIT_EXIT();
                    return;
                }
               
                /* Inform subscribers of aggregation changes */
                (void)aggr_change_callback(isid, aggrglb.aggr_lacp[aid].aggr_no);         
            }                        
        }  
    }
    AGGR_CRIT_EXIT();
    /* Inform subscribers of aggregation changes */
    (void)aggr_change_callback(isid, tmp_no);         
}


static BOOL aggr_group_exist(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t *members)
{
    l2_port_no_t       l2port;
    l2port_iter_t      l2pit;
    BOOL               found_group=0;
    vtss_port_no_t     port_no=0;
    u32                port_count = port_isid_port_count(isid);  

    AGGR_CRIT_ENTER();
    (void) l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_PHYS);
    while(l2port_iter_getnext(&l2pit)) {                            
        if (aggrglb.aggr_lacp[l2pit.l2port].aggr_no == aggr_no) {
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {                
                if (port_isid_port_no_is_stack(isid, port_no))
                    continue;
                l2port = L2PORT2PORT(isid, port_no);
                if ((members->member[port_no] = aggrglb.aggr_lacp[l2pit.l2port].members[l2port]) == 1) {
                    found_group = 1;
                }                                
            }
        }
    }
    AGGR_CRIT_EXIT();
    return found_group;
}

vtss_rc aggr_mgmt_lacp_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no,  aggr_mgmt_group_member_t *members, BOOL next)
{
    vtss_rc            rc = AGGR_ERROR_ENTRY_NOT_FOUND;
    vtss_port_no_t     port_no=0;
    BOOL               found_group=false;
    u32                port_count = port_isid_port_count(isid);  
    aggr_mgmt_group_t  local_members;

    VTSS_RC(aggr_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, 1, 0));

    memset(&local_members,0,sizeof(local_members));
    /* Get the first active aggregation group */
    if (aggr_no == 0 && next) {
        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {        
            if (aggr_no == AGGR_UPLINK) {
                continue;
            }
            if ((found_group = aggr_group_exist(isid, aggr_no, &local_members) == 1)) {
                rc = VTSS_OK;
                break;
            }                        
        }
        if (!found_group) {
            return AGGR_ERROR_ENTRY_NOT_FOUND;
        }        
    } else if (aggr_no != 0 && next) {
        /* Get the next active aggregation group */
        for (aggr_no = (aggr_no+1); aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {        
            if (aggr_no == AGGR_UPLINK) {
                continue;
            }
            if ((found_group = aggr_group_exist(isid, aggr_no, &local_members) == 1)) {
                rc = VTSS_OK;
                break;
            }
        }
        if (!found_group) {
            return AGGR_ERROR_ENTRY_NOT_FOUND;
        }
    }
        
    members->aggr_no = aggr_no;
    /* Check if the group is known */
    if (!next) {
        if (aggr_group_exist(isid, aggr_no, &local_members)) {
            /* Its there */
            rc = VTSS_OK;
        } 
    }
    /* Return the portlist (empty or not)  */
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        members->entry.member[port_no] = local_members.member[port_no];
    }

    return rc;        
}

aggr_mgmt_group_no_t lacp_to_aggr_id(int aid) 
{
    aggr_mgmt_group_no_t aggr;
    AGGR_CRIT_ENTER();
    aggr = aggrglb.aggr_lacp[aid].aggr_no;
    AGGR_CRIT_EXIT();
    return aggr;
}

#endif /* VTSS_SW_OPTION_LACP */

/* Get members in a given aggr group for both LACP and STATIC. 'Next' is used to browes trough active groups.  */
/* Note! Only members with portlink will be returned.                                                          */
/* If less than 2 members are found, 'AGGR_ERROR_ENTRY_NOT_FOUND' will be returnd.                                                           */
vtss_rc aggr_mgmt_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no,  aggr_mgmt_group_member_t *members, BOOL next)
{
    vtss_port_no_t port_no;
    u32            member_cnt=0;    
    port_status_t  port_status;
    aggr_mgmt_group_member_t m;
    u32            port_count = port_isid_port_count(isid);  
    switch_iter_t  sit;
    vtss_isid_t    isid_tmp=isid;
   
    VTSS_RC(aggr_mgmt_port_sid_invalid(isid, VTSS_PORT_NO_START, 1, 0));
       
    T_D("Enter aggr_mgmt_members_get (Static and LACP get) isid:%d aggr_no:%u",isid,aggr_no);

    if (AGGR_MGMT_GROUP_IS_LAG(aggr_no)) {    
        /* LLAGs */
        if (aggr_mgmt_port_members_get(isid, aggr_no, members, next) == VTSS_OK) {     
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (members->entry.member[port_no]) {
                    AGGR_CRIT_ENTER();
                    if (aggrglb.port_active_aggr_member[port_no] == VTSS_AGGR_NO_NONE) {
                        members->entry.member[port_no] = 0; /* No aggregation (no link, wrong speed or hdx) */
                    } else {
                        member_cnt++;
                    }
                    AGGR_CRIT_EXIT();
                }
            }
        }       

    } else if (AGGR_MGMT_GROUP_IS_GLAG(aggr_no)) {    

        /* GLAGs */
        if (aggr_mgmt_port_members_get(isid, aggr_no, members, next) == VTSS_OK) {     
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (members->entry.member[port_no]) {
                    if (port_mgmt_status_get(isid, port_no, &port_status) != VTSS_OK) {
                        T_W("Could not get update from port module");       
                        return VTSS_UNSPECIFIED_ERROR;  
                    }
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE 
                    isid_tmp = VTSS_ISID_LOCAL;
                    AGGR_CRIT_ENTER();
                    if ((aggrglb.active_aggr_ports[isid][port_no] == VTSS_AGGR_NO_NONE) || !port_status.status.link) {
                        members->entry.member[port_no] = 0; /* No aggregation */
                    } else {
                        member_cnt++;
                    }
                    AGGR_CRIT_EXIT();
#else
                    if (!port_status.status.link) {
                        members->entry.member[port_no] = 0; /* No link, no aggregation */
                    } else {
                        member_cnt++;
                    }
#endif
                }
            }
        }       

        /* If this is the last port in the GLAG we pretend as there is no ports in the GLAG*/
        if (member_cnt == 1) {
            member_cnt = 0;
            (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
            while (switch_iter_getnext(&sit)) {
                if (!msg_switch_exists(sit.isid)) {
                    continue;
                }
                port_count = port_isid_port_count(sit.isid);
                if (aggr_mgmt_port_members_get(sit.isid, aggr_no, &m, next) == VTSS_OK) {     
                    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                        if (m.entry.member[port_no]) {
                            if (port_mgmt_status_get(sit.isid, port_no, &port_status) != VTSS_OK) {
                                T_W("Could not get update from port module");       
                                return VTSS_UNSPECIFIED_ERROR;  
                            }
                            AGGR_CRIT_ENTER();
                            if (!port_status.status.link || (port_status.status.speed != aggrglb.aggr_group_speed[isid_tmp][aggr_no])) {
                                m.entry.member[port_no] = 0; /* No link or wrong speed -> no aggregation */
                            } else {
                                member_cnt++;
                            }
                            AGGR_CRIT_EXIT();
                        }
                    }                
                }
            }                        
        }
    }       

    if (member_cnt > 1) {
        T_D("Found static aggr with %u members",member_cnt);
        return VTSS_OK;
    }
#ifdef VTSS_SW_OPTION_LACP                
    member_cnt=0;   
    if (AGGR_MGMT_GROUP_IS_LAG(aggr_no)) {
        if (aggr_mgmt_lacp_members_get(isid, aggr_no, members, next) == VTSS_OK) {
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (members->entry.member[port_no]) 
                    member_cnt++;
            }
        }
    } else {            
        if (aggr_mgmt_lacp_members_get(isid, aggr_no, members, next) == VTSS_OK) {
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (members->entry.member[port_no]) 
                    member_cnt++;
            }
        }       

        /* If this is the last port in the GLAG we pretend as there is no ports in the GLAG*/
        if (member_cnt == 1) {
            member_cnt = 0;
            (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
            while (switch_iter_getnext(&sit)) {
                if (!msg_switch_exists(sit.isid)) {
                    continue;
                }
                port_count = port_isid_port_count(sit.isid);
                if (aggr_mgmt_lacp_members_get(sit.isid, aggr_no, &m, next) == VTSS_OK) {
                    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                        if (m.entry.member[port_no]) 
                            member_cnt++;
                    }                
                }
            }                        
        }
    }

    if (member_cnt > 1) {
        T_D("Found LACP aggr with %u members",member_cnt);
        return VTSS_OK;        
    }
#endif /* VTSS_SW_OPTION_LACP */
       
    memset(members, 0, sizeof(*members)); 
    T_D("No members found in aggr %u",aggr_no);
    return AGGR_ERROR_ENTRY_NOT_FOUND;
}

/* Returns the aggr number for a port.  Returns 0 if the port is not a member or if the link is down. */
aggr_mgmt_group_no_t aggr_mgmt_get_aggr_id(vtss_isid_t isid, vtss_port_no_t port_no)
{
    aggr_mgmt_group_member_t members;
    aggr_mgmt_group_no_t aggr_no; 

    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {
        if (aggr_mgmt_members_get(isid, aggr_no,  &members, 0) == VTSS_OK) {                                                        
            if (members.entry.member[port_no]) {
                return aggr_no;            
            }
        }
    }
    return 0;
}

aggr_mgmt_group_no_t aggr_mgmt_get_port_aggr_id(vtss_isid_t isid, vtss_port_no_t port_no)
{
    aggr_mgmt_group_member_t members;
    aggr_mgmt_group_no_t aggr_no; 

    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {
        if (aggr_mgmt_port_members_get(isid, aggr_no,  &members, 0) == VTSS_OK) {
            if (members.entry.member[port_no]) {
                return aggr_no;            
            }
        }
    }
    return 0;
}


/* Returns information if the port is participating in LACP or Static aggregation */
/* 0 = No participation                                                           */         
/* 1 = Static aggregation participation                                           */        
/* 2 = LACP aggregation participation                                             */         

int aggr_mgmt_port_participation(vtss_isid_t isid, vtss_port_no_t port_no)
{
    aggr_mgmt_group_member_t members;
    aggr_mgmt_group_no_t aggr_no; 

#ifdef VTSS_SW_OPTION_LACP
    vtss_lacp_port_config_t  conf;
    
    /* Check if the group or ports is occupied by LACP in this isid */
    if  (lacp_get_port_config(isid, port_no, &conf) == VTSS_OK) {
        if (conf.enable_lacp) 
            return 2;     
    }               
#endif

    /* Check if the group or ports is occupied by Static aggr in this isid */
    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {
        if (aggr_mgmt_port_members_get(isid, aggr_no, &members, 0) == VTSS_OK) {                
            if (members.entry.member[port_no]) {
                return 1;
            }
        }        
    }
        
    return 0;        
}

/* Sets the aggregation mode.  The mode is used by all the aggregation groups */
vtss_rc aggr_mgmt_aggr_mode_set(vtss_aggr_mode_t *mode)
{    
    switch_iter_t          sit;

    if (!msg_switch_is_master()) {
        T_W("not master");
        return AGGR_ERROR_STACK_STATE;
    }
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {        
        if (!msg_switch_exists(sit.isid)) {
            continue;
        }
        /* Add to isid */
        aggr_stack_mode_set(sit.isid, mode);                
    }

    /* Add to local and stack config */
    AGGR_CRIT_ENTER();
    aggrglb.aggr_config.mode = *mode;
    aggrglb.aggr_config_stack.mode = *mode;
    AGGR_CRIT_EXIT();              
    (void)aggr_save_to_flash();       
    return VTSS_OK;
}


/* Gets the aggregation mode.  The 'mode' points to the updated mode type */
vtss_rc aggr_mgmt_aggr_mode_get(vtss_aggr_mode_t *mode)
{        
   /* Get from local config */
    AGGR_CRIT_ENTER();
    *mode = aggrglb.aggr_config.mode;
    AGGR_CRIT_EXIT();
    return VTSS_OK;
}

/* Registration for callbacks if aggregation changes */
void aggr_change_register(aggr_change_callback_t cb)
{
    VTSS_ASSERT(aggrglb.aggr_callbacks < ARRSZ(aggrglb.callback_list));
    cyg_scheduler_lock();
    aggrglb.callback_list[aggrglb.aggr_callbacks++] = cb;
    cyg_scheduler_unlock();
}


/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Read/create and activate aggregation  configuration */
static void aggr_conf_stack_read(vtss_isid_t isid_add, BOOL force_default)
{

    aggr_conf_stack_aggr_t *aggr_blk;
    ulong                  size;
    BOOL                   create=0; 
    int                    i, aggr_no,group=0;
    aggr_mgmt_group_member_t members;
    vtss_aggr_mode_t       mode;
    switch_iter_t            sit;

    T_D("enter, isid_add: %d", isid_add);

    /* Restore. Open or create configuration block */
    if ((aggr_blk = conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_AGGR_TABLE, &size)) == NULL) {
        create = 1;
        T_W("Could not open conf block (new switch?). Defaulting.");
    } else {       
        /* If the configuration size have changed then create a block defaults */
        if (size != sizeof(*aggr_blk)) {
            create = 1;         
            T_W("Configuration size have changed, creating defaults");
        } else if (aggr_blk->version != AGGR_CONF_VERSION) {        
            T_W("Version mismatch, creating defaults");
            create = 1;
        }    
    }

#ifdef VTSS_SW_OPTION_LACP
    /* Only reset the aggrglb.aggr_lacp stucture when becoming a master */
    /* When 'System Restore Default' is issued then LACP will delete */
    /* all of its members and notify subscribers.  */
    if (!force_default)
        memset(aggrglb.aggr_lacp, 0, sizeof(aggrglb.aggr_lacp));
#endif /* VTSS_SW_OPTION_LACP */
  
    /* Defaulting */
    if (create || force_default) {
        T_D("Defaulting configuration.");

       aggr_blk = conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_AGGR_TABLE, sizeof(aggr_conf_stack_aggr_t));
        T_D("Defaulting aggr mode, isid:%d",isid_add);
        /* Enable default aggregation code contributions */
        AGGR_CRIT_ENTER();
        aggrglb.aggr_config_stack.mode.smac_enable = 1;
        aggrglb.aggr_config_stack.mode.dmac_enable = 0;
        aggrglb.aggr_config_stack.mode.sip_dip_enable = 1;
        aggrglb.aggr_config_stack.mode.sport_dport_enable = 1;
        aggrglb.aggr_config_stack.version = AGGR_CONF_VERSION;            
        AGGR_CRIT_EXIT();
              
        /* Write to the switch */

        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {        
            if (!msg_switch_exists(sit.isid)) {
                continue;
            }
            T_D("Force defaulting, isid:%d",sit.isid);
            
            /* Must notify subscribers of the aggr deletion - only from master */
            while (msg_switch_is_master() &&  aggr_mgmt_port_members_get(sit.isid, group,  &members, 1) == VTSS_OK) {
                group = members.aggr_no;
                if (aggr_mgmt_group_del(sit.isid, group) != VTSS_OK)
                    T_W("Could not delete group:%d at isid:%d",group,sit.isid);
                
                /* Inform subscribers of aggregation changes */
                aggr_change_callback(sit.isid, group);   
            }
            
            /* Reset the local memory configuration */
            AGGR_CRIT_ENTER();
            for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {
                for (i = 0;i < VTSS_PORTS;i++) {
                    VTSS_BF_SET(aggrglb.aggr_config_stack.switch_id[sit.isid-VTSS_ISID_START][aggr_no-AGGR_MGMT_GROUP_NO_START].member, i, 0); 
                }
                aggrglb.aggr_config_stack.switch_id[sit.isid-VTSS_ISID_START][aggr_no-AGGR_MGMT_GROUP_NO_START].speed = VTSS_SPEED_UNDEFINED;
            }
            AGGR_CRIT_EXIT();
            
            /* Reset the chip configuration */
            (void)aggr_group_del(sit.isid, AGGR_ALL_GROUPS);
            AGGR_CRIT_ENTER();
            mode = aggrglb.aggr_config_stack.mode;
            AGGR_CRIT_EXIT();
            /* Set the default aggr mode    */
            aggr_stack_mode_set(sit.isid, &mode);    
        }

        /* Write to flash */
        if (aggr_blk != NULL) {
            AGGR_CRIT_ENTER();
            *aggr_blk = aggrglb.aggr_config_stack;
            AGGR_CRIT_EXIT();
        } else {
            T_E("Failed to open aggr config table");
            return;
        }
    } else {
        /* Use stored configuration */
        T_D("Using stored config");
        
       /* Move entries from flash to local structure */
        if (aggr_blk != NULL) {
            AGGR_CRIT_ENTER();
            aggrglb.aggr_config_stack = *aggr_blk;                                                
            AGGR_CRIT_EXIT();
        }
    }
     
    if (aggr_blk == NULL) {
        T_E("Failed to open aggr config table");
    } else {
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_AGGR_TABLE);
    }

    T_D("exit");
}

/* Re-build the configuration to get the new UPSID incorporated */
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
static void topo_upsid_change_callback(vtss_isid_t isid)
{
    switch_iter_t            sit;

    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {        
        if (!msg_switch_exists(sit.isid)) {
            continue;
        }
        (void)aggr_switch_del(sit.isid);
    }    
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {        
        if (!msg_switch_exists(sit.isid)) {
            continue;
        }
        (void)aggr_switch_add(sit.isid);
    }    
}
#endif /* VTSS_FEATURE_VSTAX_V2 */

static void aggr_start(BOOL init)
{
    aggr_mgmt_group_no_t aggr;    
    vtss_isid_t          isid;
    vtss_port_no_t       port_no;
    
    if (init) {
        /* Initialize global area */
        memset(&aggrglb, 0, sizeof(aggrglb));

        /* Initialize message buffers */
        VTSS_OS_SEM_CREATE(&aggrglb.request.sem, 1);
        VTSS_OS_SEM_CREATE(&aggrglb.reply.sem, 1);
        
        /* Reset the switch aggr conf */
        for (isid = 0; isid < VTSS_ISID_END; isid++) {           
            for (aggr = AGGR_MGMT_GROUP_NO_START; aggr < AGGR_MGMT_GROUP_NO_END; aggr++) 
                aggrglb.aggr_group_speed[isid][aggr] = VTSS_SPEED_UNDEFINED;
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
            for (port_no = 0; port_no < VTSS_PORT_NO_END; port_no++) 
                aggrglb.active_aggr_ports[isid][port_no] = VTSS_AGGR_NO_NONE;
#endif
        }
         
        /* Open up switch API after initialization */
        critd_init(&aggrglb.aggr_crit, "aggr_crit", VTSS_MODULE_ID_AGGR, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        AGGR_CRIT_EXIT();
        critd_init(&aggrglb.aggr_cb_crit, "aggr_cb_crit", VTSS_MODULE_ID_AGGR, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        AGGR_CB_CRIT_EXIT();

    } else {
        for (port_no = 0; port_no < VTSS_PORT_NO_END; port_no++) {
            aggrglb.port_glag_member[port_no] = VTSS_GLAG_NO_NONE;
            aggrglb.port_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
            aggrglb.port_active_aggr_member[port_no] = VTSS_AGGR_NO_NONE;            
        }
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
        for (port_no = 0; port_no < VTSS_PORT_NO_END; port_no++) {
            aggrglb.port_active_glag_member[port_no] = VTSS_GLAG_NO_NONE;            
        }
        for (aggr = AGGR_MGMT_GROUP_NO_START-AGGR_MGMT_GROUP_NO_START; aggr < AGGR_MGMT_GROUP_NO_END-AGGR_MGMT_GROUP_NO_START; aggr++) {
            aggrglb.glag_ram_entries[aggr][VTSS_GLAG_PORT_START].ups.upsid = VTSS_VSTAX_UPSID_UNDEF; 
            aggrglb.glag_ram_entries[aggr][VTSS_GLAG_PORT_START].ups.upspn = VTSS_PORT_NO_NONE;
            aggrglb.glag_ram_entries[aggr][VTSS_GLAG_PORT_START].isid = VTSS_PORT_NO_NONE;
        }
#endif /* VTSS_FEATURE_VSTAX_V2 */

        /* Register for Port GLOBAL change callback */
        (void)port_global_change_register(VTSS_MODULE_ID_AGGR, aggr_global_port_state_change_callback);

#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
        /* Register for UPSID change */
        (void)topo_upsid_change_callback_register(topo_upsid_change_callback, VTSS_MODULE_ID_AGGR);
#endif /* VTSS_FEATURE_VSTAX_V2 */

#if VTSS_SWITCH_STACKABLE
        /* Register for stack messages */
        (void)aggr_stack_register();   
        /* Register for Port LOCAL change callback */
        (void)port_change_register(VTSS_MODULE_ID_AGGR, aggr_port_state_change_callback);
        /* Register for stack topology change */
        (void)(topo_state_change_callback_register(aggr_glag_callback, VTSS_MODULE_ID_AGGR));
#endif /* VTSS_SWITCH_STACKABLE */
    }
}

void aggr_mgmt_dump(aggr_dbg_printf_t dbg_printf)
{
    vtss_port_no_t port_no;
#if VTSS_SWITCH_STACKABLE
    vtss_isid_t isid;
#endif
#if defined(VTSS_FEATURE_VSTAX_V1)
    (void)dbg_printf("VTSS_FEATURE_VSTAX_V1 is defined, \nVTSS_SWITCH_STACKABLE:%d\n",VTSS_SWITCH_STACKABLE);
#endif
#if defined(VTSS_FEATURE_VSTAX_V2)
    (void)dbg_printf("VTSS_FEATURE_VSTAX_V2 is defined, \nVTSS_SWITCH_STACKABLE:%d\n",VTSS_SWITCH_STACKABLE);
#endif
#if !defined(VTSS_FEATURE_VSTAX_V1) && !defined(VTSS_FEATURE_VSTAX_V2) 
    (void)dbg_printf("VTSS_FEATURE_VSTAX_V1 and VTSS_FEATURE_VSTAX_V2 are not defined. VTSS_SWITCH_STACKABLE:%d\n",VTSS_SWITCH_STACKABLE);
#endif
    (void)dbg_printf("VTSS_PORTS:%d \nVTSS_AGGRS:%d \nAGGR_LLAG_CNT:%d \nAGGR_GLAG_CNT:%d \nAGGR_MGMT_LAG_PORTS_MAX:%d \nAGGR_MGMT_GLAG_PORTS_MAX:%d\n",
                     VTSS_PORTS, VTSS_AGGRS, AGGR_LLAG_CNT,AGGR_GLAG_CNT,AGGR_MGMT_LAG_PORTS_MAX, AGGR_MGMT_GLAG_PORTS_MAX);

    (void)dbg_printf("AGGR_MGMT_GROUP_NO_START:%d  \nAGGR_MGMT_GROUP_NO_END:%d\n",AGGR_MGMT_GROUP_NO_START, AGGR_MGMT_GROUP_NO_END);


#if !VTSS_SWITCH_STACKABLE
    (void)dbg_printf("LLAGs:\n");
    for (port_no = 0; port_no < VTSS_PORT_NO_END; port_no++) {
        if(aggrglb.port_aggr_member[port_no] != VTSS_AGGR_NO_NONE || aggrglb.port_active_aggr_member[port_no] != VTSS_AGGR_NO_NONE) {
            (void)dbg_printf("Port:%u Aggr:%u Active:%s\n",port_no, aggrglb.port_aggr_member[port_no],
                             (aggrglb.port_active_aggr_member[port_no] == aggrglb.port_aggr_member[port_no])?"Yes":"No");
        }
    }
#endif
#if VTSS_SWITCH_STACKABLE
    aggr_mgmt_group_no_t aggr_no;    
    (void)dbg_printf("GLAGs:\n");
    for (port_no = 0; port_no < VTSS_PORT_NO_END; port_no++) {
        if(aggrglb.port_glag_member[port_no] != VTSS_AGGR_NO_NONE || aggrglb.port_active_glag_member[port_no] != VTSS_AGGR_NO_NONE) {
            (void)dbg_printf("(Local)Port:%u Aggr:%u Active:%s\n",port_no, aggrglb.port_glag_member[port_no],
                             (aggrglb.port_active_glag_member[port_no] == aggrglb.port_glag_member[port_no])?"Yes":"No");
        }
    }

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {           
        for (port_no = 0; port_no < VTSS_PORT_NO_END; port_no++) {
            if (aggrglb.active_aggr_ports[isid][port_no] != VTSS_AGGR_NO_NONE) {
                (void)dbg_printf("ISID:%d Port:%u is Active in aggr:%u speed:%d\n",isid, port_no, 
                                 aggrglb.active_aggr_ports[isid][port_no],aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][aggrglb.active_aggr_ports[isid][port_no]]);
            } 
        }
    }

    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END; aggr_no++) {        
        if (aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][aggr_no] != VTSS_SPEED_UNDEFINED) {
            (void)dbg_printf("Group:%u speed:%d\n",aggr_no, aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][aggr_no]); 
        }
    }
#endif
}

 
vtss_rc aggr_init(vtss_init_data_t *data)
{

    vtss_isid_t isid = data->isid;
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
    vtss_vstax_upsid_t upsid;
    vtss_port_no_t port_no;
#endif

    if (data->cmd == INIT_CMD_INIT) {
        VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&trace_reg);
    /* Initilize the module */
    }

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        /* Initilize the module */
        aggr_start(1);        
#ifdef VTSS_SW_OPTION_CLI
        aggr_cli_req_init();
#endif
#ifdef VTSS_SW_OPTION_ICFG
       if (aggr_icfg_init() != VTSS_OK) {
           T_D("Calling aggr_icfg_init() failed");
       }
#endif
       break;
    case INIT_CMD_START:  
        aggr_start(0);        
        break;
    case INIT_CMD_CONF_DEF:
        T_D("INIT_CMD_CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset configuration to default (local config on specific switch or global config for whole stack) */
            aggr_conf_stack_read(VTSS_ISID_GLOBAL,1);    
        }
        break;
    case INIT_CMD_MASTER_UP:
        T_D("MASTER_UP");
        aggr_conf_stack_read(VTSS_ISID_GLOBAL,0);
        break;
    case INIT_CMD_MASTER_DOWN:
        T_D("MASTER_DOWN");
        break;
    case INIT_CMD_SWITCH_ADD:
        T_D("SWITCH_ADD, isid: %d", isid);
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
        /* Store the upsid for later use */
        if ((upsid = topo_isid_port2upsid(isid,VTSS_PORT_NO_START)) == VTSS_VSTAX_UPSID_UNDEF) {
            aggrglb.upsid_table[isid].upsid[0] = upsid;
            T_D("Could not get UPSID from Topo during Switch ADD, isid: %d", isid);
            break;
        }
        aggrglb.upsid_table[isid].upsid[0] = upsid;
        aggrglb.upsid_table[isid].upsid[1] = VTSS_VSTAX_UPSID_UNDEF;
        T_D("Switch ADD, isid: %d upsid:%d\n", isid, aggrglb.upsid_table[isid].upsid[0]);
        for (port_no = VTSS_PORT_NO_START+1; (port_no < data->port_cnt); port_no++) {
            if ((upsid = topo_isid_port2upsid(isid,port_no)) == VTSS_VSTAX_UPSID_UNDEF) {
                T_D("Could not get UPSID from Topo during Switch ADD, isid: %d", isid);
                break;
            }         
            if (aggrglb.upsid_table[isid].upsid[0] != upsid) {
                aggrglb.upsid_table[isid].upsid[1] = upsid;
                aggrglb.upsid_table[isid].port_no = port_no; /* First port on second UPSID */
                break;
            }            
        }
#endif /* (VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE */
        /* Configure the new switch */
        aggr_switch_add(isid);
        break;
    case INIT_CMD_SWITCH_DEL:
        T_D("SWITCH_DEL, isid: %d", isid);
#if defined(VTSS_FEATURE_VSTAX_V2) && VTSS_SWITCH_STACKABLE
        aggr_switch_del(isid);
#endif
        break;
    default:
        break;
    }

    T_D("exit");
    return 0;
}

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/


