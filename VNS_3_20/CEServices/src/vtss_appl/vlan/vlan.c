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
#include "vlan_api.h"
#include "vlan.h"
#include "port_api.h"   /* For a.o. PORT_NO_STACK_0 */
#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h" /* For definition of S_E.   */
#endif /* VTSS_SW_OPTION_SYSLOG */
#ifdef VTSS_SW_OPTION_VCLI
#include "vlan_cli.h"
#endif
#include "mgmt_api.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "vlan_icfg.h"
#endif

#undef VTSS_SW_OPTION_MVRP

#define CALLBACK_FUNC 10
#define VLAN_TPID_CALLBACK_FUNC 5 

#if defined(VTSS_SW_OPTION_DOT1X) || defined(VTSS_SW_OPTION_MSTP) || \
    defined(VTSS_SW_OPTION_MVR) || defined(VTSS_SW_OPTION_MVRP) ||  \
    defined(VTSS_SW_OPTION_VOICE_VLAN) || defined(VTSS_SW_OPTION_ERPS) || \
    defined(VTSS_SW_OPTION_MEP) || defined(VTSS_SW_OPTION_EVC) || \
    defined(VTSS_SW_OPTION_VCL) || defined(VTSS_SW_OPTION_RSPAN)

    #define VOLATILE_VLAN_USER_PRESENT  1 
#endif

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* Global structure */
static vlan_global_t vlan;
static i32 vlan_forbid_callbacks;
static vlan_forbid_change_callback_t callback_forbid_list[CALLBACK_FUNC];
static u32 vlan_port_conf_change_callbacks;
static vlan_port_change_callback_t vlan_port_conf_change_callbacks_list[5];
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
static u32 vlan_tpid_conf_change_callbacks;
static vlan_s_port_tpid_conf_change_callback_t tpid_conf_change_callback_list[VLAN_TPID_CALLBACK_FUNC];
#endif
static vlan_t *vlan_multi_membership_table[VTSS_VIDS];
static vlan_t *vlan_forbidden_table[VTSS_VIDS];
static i32 vlan_membership_callbacks;
static vlan_membership_change_callback_t callback_membership_list[CALLBACK_FUNC];

#if (VTSS_TRACE_ENABLED)
static vtss_trace_reg_t trace_reg =
{
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "vlan",
    .descr     = "VLAN table"
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
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [TRACE_GRP_CRIT_CB] = {
        .name      = "cb_crit",
        .descr     = "VLAN Callback Critical regions ",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    }
};

#define VLAN_CRIT_ENTER() critd_enter(&vlan.crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE,  __FUNCTION__, __LINE__)
#define VLAN_CRIT_EXIT()  critd_exit( &vlan.crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE,  __FUNCTION__, __LINE__)
#define VLAN_CRIT_ASSERT_LOCKED() critd_assert_locked(&vlan.crit, TRACE_GRP_CRIT, __FILE__, __LINE__)
#define VLAN_CB_CRIT_ENTER() critd_enter(&vlan.cb_crit, TRACE_GRP_CRIT_CB, VTSS_TRACE_LVL_NOISE,  __FUNCTION__, __LINE__)
#define VLAN_CB_CRIT_EXIT()  critd_exit( &vlan.cb_crit, TRACE_GRP_CRIT_CB, VTSS_TRACE_LVL_NOISE,  __FUNCTION__, __LINE__)
#define VLAN_CB_CRIT_ASSERT_LOCKED() critd_assert_locked(&vlan.cb_crit, TRACE_GRP_CRIT_CB, __FILE__, __LINE__)
#else
#define VLAN_CRIT_ENTER() critd_enter(&vlan.crit)
#define VLAN_CRIT_EXIT()  critd_exit( &vlan.crit)
#define VLAN_CRIT_ASSERT_LOCKED() critd_assert_locked(&vlan.crit)
#define VLAN_CB_CRIT_ENTER() critd_enter(&vlan.cb_crit)
#define VLAN_CB_CRIT_EXIT()  critd_exit( &vlan.cb_crit)
#define VLAN_CB_CRIT_ASSERT_LOCKED() critd_assert_locked(&vlan.cb_crit)
#endif /* VTSS_TRACE_ENABLED */

/****************************************************************************/
/* Predefined local functions                                                 */
/****************************************************************************/

static vtss_rc vlan_list_vlan_del(vtss_isid_t isid_del,
                                              vtss_vid_t vid, vlan_user_t user);
static char *vlan_user_id_txt(vlan_user_t user);
static vtss_rc vlan_get_from_hardware(vtss_vid_t vid,
                  vlan_mgmt_entry_t *vlan_mgmt_entry, BOOL next);

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/

static inline BOOL is_vlan_usr_conf(vtss_isid_t isid, vtss_vid_t vid, vlan_user_t user)
{
    VLAN_CRIT_ASSERT_LOCKED();
    return (vlan.vlan_users[isid - VTSS_ISID_START][vid - 1] & (1U << user)) ? TRUE : FALSE;
}
static inline BOOL IF_MULTI_USER_GET_IDX(vlan_user_t user,
                                multi_user_index_t *user_idx)
{
    switch (user) {
        case VLAN_USER_STATIC:
          *user_idx = VLAN_MULTI_STATIC;
          return TRUE;





#ifdef VTSS_SW_OPTION_MEP
        case VLAN_USER_MEP:
          *user_idx = VLAN_MULTI_MEP;
          return TRUE;
#endif
#ifdef VTSS_SW_OPTION_EVC
        case VLAN_USER_EVC:
          *user_idx = VLAN_MULTI_EVC;
          return TRUE;
#endif
#ifdef VTSS_SW_OPTION_MVR
        case VLAN_USER_MVR:
            *user_idx = VLAN_MULTI_MVR;
            return TRUE;
#endif /* VTSS_SW_OPTION_MVR */
        default:
          T_D("Invalid user (%d)", user);
          *user_idx = VLAN_MULTI_CNT;
          return FALSE;
    }
}

#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT

static inline BOOL IF_SAME_USER_GET_IDX(vlan_user_t user,
                             same_user_index_t *user_idx)
{
    switch (user) {
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        case VLAN_USER_VOICE_VLAN:
            *user_idx = VLAN_SAME_VOICE_VLAN;
            return TRUE;
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        case VLAN_USER_RSPAN:
            *user_idx = VLAN_SAME_RSPAN;
            return TRUE;
#endif /* VTSS_SW_OPTION_RSPAN */
        default:
            T_D("Invalid user (%d)", user);
            *user_idx = VLAN_SAME_CNT;
            return FALSE;
    }
}

#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */

#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
static inline BOOL IF_SINGLE_USER_GET_IDX(vlan_user_t user,
                                                single_user_index_t *user_idx)
{
    switch (user) {
#ifdef VTSS_SW_OPTION_DOT1X
        case VLAN_USER_DOT1X:
            *user_idx = VLAN_SINGLE_DOT1X;
            return TRUE;
#endif /* VTSS_SW_OPTION_DOT1X */
        default:
            T_D("Invalid user (%d)", user);
            *user_idx = VLAN_SINGLE_CNT;
            return FALSE;
    }
}

#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
/****************************************************************************/
/* Description: Some modules need to know when vlan configuration changes. For*/
/*              doing this a callback function is provided. At the time of  */
/*              writing there is no need for being able to de-register      */
/* Input:                                                                   */
/* Output:                                                                  */
/****************************************************************************/
void vlan_port_conf_change_register(vlan_port_change_callback_t cb)
{
    T_R("Registering for callback VLAN port configuration change");
    VLAN_CB_CRIT_ENTER();
    if (vlan_port_conf_change_callbacks <
            ARRSZ(vlan_port_conf_change_callbacks_list)) {
        vlan_port_conf_change_callbacks_list[vlan_port_conf_change_callbacks++]
            = cb;
    } else {
        T_E("Not possible to register more vlan_port_conf_change_callbacks, "
            "please increse vlan_port_conf_change_callbacks_list in the source "
            "code");
    } 
    VLAN_CB_CRIT_EXIT();
}

static void vlan_port_conf_change_callback(vtss_port_no_t port_no, 
                                            vlan_port_conf_t *conf)
{
    unsigned int                i;
    vlan_port_change_callback_t func;

    T_I("vlan_port_conf_change_callback: port_no: %d, port_type: %d, pvid %d, untagged_vid: %d, "
            "frame_type: %d, ingress_flt: %d, tx_tag_type: %d\n",
            port_no,
            conf->port_type,
            conf->pvid,
            conf->untagged_vid,
            conf->frame_type,
            conf->ingress_filter,
            conf->tx_tag_type);

    VLAN_CB_CRIT_ENTER();
    for (i = 0; i < vlan_port_conf_change_callbacks; i++) {
        T_D("Calling VLAN Port conf callback\n");
        func = vlan_port_conf_change_callbacks_list[i];
        VLAN_CB_CRIT_EXIT();
        func(port_no, conf);
        VLAN_CB_CRIT_ENTER();
    }
    VLAN_CB_CRIT_EXIT();
}

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
void vlan_s_port_tpid_conf_change_register(vlan_s_port_tpid_conf_change_callback_t cb)
{
    T_R("Registering for callback : VLAN S-port TPID configuration change");
    VLAN_CB_CRIT_ENTER();
    if (vlan_tpid_conf_change_callbacks < ARRSZ(tpid_conf_change_callback_list)) {
        tpid_conf_change_callback_list[vlan_tpid_conf_change_callbacks++] = cb;
    } else {
        T_E("Not possible to register more vlan_tpid_conf_change_callbacks, "
            "please increse tpid_conf_change_callback_list in the source "
            "code");
    } 
    VLAN_CB_CRIT_EXIT();
}

static void vlan_s_port_tpid_conf_change_callback(vtss_etype_t tpid)
{
    unsigned int                            i;
    vlan_s_port_tpid_conf_change_callback_t func;

    T_I("vlan_s_port_tpid_conf_change_callback: tpid 0x%x", tpid);

    VLAN_CB_CRIT_ENTER();
    for (i = 0; i < vlan_tpid_conf_change_callbacks; i++) {
        T_D("Calling VLAN S-port tpid conf change callback\n");
        func = tpid_conf_change_callback_list[i];
        VLAN_CB_CRIT_EXIT();
        func(tpid);
        VLAN_CB_CRIT_ENTER();
    }
    VLAN_CB_CRIT_EXIT();
}
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */
/*****************************************************************/
/* Description: Determine if VLAN port configuration has changed */
/*                                                               */
/* Input:                                                        */
/* Output: Return non zero if changed                            */
/*****************************************************************/
static i32 vlan_port_conf_changed(vlan_port_conf_t *old, vlan_port_conf_t *new)
{
    return (memcmp(new, old, sizeof(vlan_port_conf_t)));
}


/*****************************************************************/
/* Description: Setup default vlan port ocnfiguration            */
/*                                                               */
/* Input:  Pointer to vlan port entry                            */
/* Output:                                                       */
/*****************************************************************/
static void vlan_port_default_set(vlan_port_conf_t *vlan_port_conf)
{
    /* Use default values */
    vlan_port_conf->pvid = VLAN_TAG_DEFAULT;
    vlan_port_conf->untagged_vid = vlan_port_conf->pvid;
    vlan_port_conf->frame_type = VTSS_VLAN_FRAME_ALL;
    vlan_port_conf->ingress_filter = 0;
    vlan_port_conf->tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_THIS;
    vlan_port_conf->port_type = VLAN_PORT_TYPE_UNAWARE;
    vlan_port_conf->flags = VLAN_PORT_FLAGS_ALL;
}

/*****************************************************************/
/* Description:Setup VLAN port configuration via switch API      */
/* - Calling API function vtss_vlan_port_mode_set                */
/* Input:                                                        */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
static vtss_rc vlan_port_conf_set(vtss_port_no_t port_no,
                                  vlan_port_conf_t *conf)
{
    vtss_rc rc;
    vtss_vlan_port_conf_t vlan_mode;

#if defined(VTSS_FEATURE_VLAN_PORT_V1)
    vlan_mode.stag = 0;
    vlan_mode.aware = conf->port_type;
#endif /* VTSS_FEATURE_VLAN_PORT_V1 */
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    vlan_mode.port_type = conf->port_type;
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */
    /* If PVID is zero, the default VID (1) is used to ensure that untagged
       frames (e.g. BPDUs) are learned in a valid VLAN */
    vlan_mode.pvid  = (conf->pvid == VTSS_VID_NULL ?
                      VTSS_VID_DEFAULT : conf->pvid);
    vlan_mode.untagged_vid = (conf->untagged_vid == VLAN_TAG_ALL_NOT_PVID ?
                              conf->pvid : conf->untagged_vid);
    vlan_mode.frame_type = conf->frame_type;
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
    /* Option available & controllable */
    vlan_mode.ingress_filter = conf->ingress_filter;
#else
    vlan_mode.ingress_filter = TRUE; /* Enable always */
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */

    T_D("VLAN Port change port %d, pvid %d",port_no, vlan_mode.pvid);
    rc = vtss_vlan_port_conf_set(NULL, port_no, &vlan_mode);


    /* Signal to all modules that want to know that the VLAN port configuration
       has changed. */
    vlan_port_conf_change_callback(port_no, conf);


    return rc;
}

/*****************************************************************/
/* Description:Get VLAN port configuration via switch API        */
/* - Calling API function vtss_vlan_port_conf_get                */
/* Input:  port number, vlan_port_conf_t pointer                 */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
static vtss_rc vlan_port_conf_get(vtss_port_no_t port_no, vlan_port_conf_t *conf)
{
    vtss_rc rc;
    vtss_vlan_port_conf_t vlan_mode;

    memset(conf, 0, sizeof(vlan_port_conf_t));
    rc = vtss_vlan_port_conf_get(NULL, port_no, &vlan_mode);
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
    conf->port_type = (vlan_mode.aware ? VLAN_PORT_TYPE_C : VLAN_PORT_TYPE_UNAWARE);
#endif /* VTSS_FEATURE_VLAN_PORT_V1 */
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    conf->port_type = vlan_mode.port_type;
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */
    conf->pvid = vlan_mode.pvid;
    conf->untagged_vid = vlan_mode.untagged_vid;
    conf->frame_type = vlan_mode.frame_type;
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
    conf->ingress_filter = vlan_mode.ingress_filter;
#endif  /* VTSS_SW_OPTION_VLAN_INGRESS_FILTERING */

    return rc;
}

/*****************************************************************/
/* Description:Get VLAN via switch API                           */
/* - Calling API function vtss_vlan_port_members_get             */
/* Input:                                                        */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
static vtss_rc vlan_entry_get(vtss_vid_t vid, BOOL *member)
{
    vtss_rc rc = VTSS_RC_ERROR;

    rc = vtss_vlan_port_members_get(NULL, vid, member);
    if (rc != VTSS_OK) {
        T_D("vtss_vlan_port_member_get returns error-[%d]",rc);
        return VTSS_RC_ERROR;
    }
    return rc;
}

/*****************************************************************/
/* Description:Add VLAN entry via switch API                     */
/* - Calling API function vtss_vlan_port_members_set             */
/* Input:                                                        */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
static vtss_rc vlan_entry_add(BOOL              vid_delete,
                              vlan_entry_conf_t *conf,
                              vtss_isid_t       isid)
{
    vtss_rc             rc;
    BOOL                member[VTSS_PORT_ARRAY_SIZE];
    port_iter_t         pit;

    memset(member, 0, sizeof(member));
    (void) port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        member[pit.iport] = (VTSS_BF_GET(conf->ports[isid], pit.iport) && (!vid_delete)) ? 1 : 0;
    }

    T_D("VLAN Add: vid %u, vid_delete %d, ports 0x%x",
        conf->vid,
        vid_delete,
        conf->ports[isid][0]);

    rc = vtss_vlan_port_members_set(NULL, conf->vid, member);
    if (rc != VTSS_OK) {
        T_D("vtss_vlan_port_member_set returns error-[%d]",rc);
        return VTSS_RC_ERROR;
    }

    /* Enable port isolation for this VLAN */
    rc = vtss_isolated_vlan_set(NULL, conf->vid, !vid_delete);

   return rc;
}


/************************************************************/
// vlan_forbidden_list_vlan_add()
// Adds or overwrites a forbidden port list to @isid_add.
// @isid_add may be either
// VTSS_ISID_LEGAL: Any legal switch ID, i.e. we modify the stack-specific list.
// Output: Return VTSS_OK if success                                        
/************************************************************/

static vtss_rc vlan_forbidden_list_vlan_add(vlan_entry_conf_t   *vlan_entry,
                                            vtss_isid_t         isid_add,
                                            vlan_user_t         user)
{
    vlan_t              *new_forbidden_vlan = NULL;
    vtss_rc             rc = VTSS_OK;
    vlan_list_t         *fb_list;
    vtss_isid_t         zero_based_isid = isid_add - VTSS_ISID_START;
    multi_user_index_t  multi_user_idx = VLAN_MULTI_CNT;
    port_iter_t         pit;
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    same_user_index_t               same_user_ct = VLAN_SAME_CNT;
    same_user_index_t               same_user_idx  = VLAN_SAME_CNT;
    vlan_same_membership_entry_t    *same_member_entry = NULL;
#endif  /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */
#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    single_user_index_t             single_user_ct = VLAN_SINGLE_CNT;
    single_user_index_t             single_user_idx  = VLAN_SINGLE_CNT;
    vlan_single_membership_entry_t  *single_member_entry = NULL;
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
  
    T_D("Enter: vid %d isid_add %d user:%s", vlan_entry->vid, isid_add, vlan_user_id_txt(user));
    VLAN_CRIT_ENTER();
    if (vlan_entry->vid >= VTSS_VIDS) {
        rc = VTSS_RC_ERROR;
        goto exit_func;
    }
    if (IF_MULTI_USER_GET_IDX(user, &multi_user_idx)) {    
        /* Now, prev_vlan points to the entry to which to append the
        new entry, or in case of an overwrite, the entry to overwrite.
        The remainder of the list is given by temp_vlan. */
        fb_list = &vlan.stack_vlan;
        /*if vlan entry not exist then add in forbidden list.*/
        if (vlan_multi_membership_table[vlan_entry->vid] == NULL) {
            if (vlan_forbidden_table[vlan_entry->vid] == NULL) {
                if (fb_list->free == NULL) {
                    rc = VLAN_ERROR_VLAN_TABLE_FULL; 
                    /* goto statements are quite alright when we always need 
                    to do something special before exiting a function. */
                    goto exit_func;
                }
                new_forbidden_vlan = fb_list->free;
                fb_list->free = fb_list->free->next;
                new_forbidden_vlan->next = NULL;
                vlan_forbidden_table[vlan_entry->vid] = new_forbidden_vlan;
            } else {
                new_forbidden_vlan = vlan_forbidden_table[vlan_entry->vid];
            }
        } else {
            /* if vlan exist in vlan list then check for which user the vlan entry is configured.
               if it is static user skip to add in forbidden list other wise add in forbidden list 
               and notify vlan mgmt api*/
            (void) port_iter_init(&pit, NULL, isid_add, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                for(multi_user_idx = VLAN_MULTI_STATIC ; multi_user_idx < VLAN_MULTI_CNT ; multi_user_idx++) {
                    if (VTSS_BF_GET(vlan_entry->ports[isid_add], pit.iport) && 
                        VTSS_BF_GET(vlan_multi_membership_table[vlan_entry->vid]->
                                    conf.ports[multi_user_idx][zero_based_isid], pit.iport)) {
                        if(multi_user_idx == VLAN_MULTI_STATIC) {
                            rc = VLAN_ERROR_VLAN_FORBIDDEN;
                            goto exit_func;
                        }
                        rc = VLAN_ERROR_VLAN_DYNAMIC_USER;
                    }
                }
            }
        }
        (void) port_iter_init(&pit, NULL, isid_add, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        /* check if vlan is configured for SAME or SINGLE user. if yes then notify to vlan mgmt api*/ 
        while (port_iter_getnext(&pit)) {
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
            for (same_user_ct = (same_user_index_t)0; same_user_ct < VLAN_SAME_CNT; same_user_ct++) {
                if (same_user_ct != same_user_idx) {
                    same_member_entry = &(vlan.vlan_same_membership_entries[same_user_ct]);
                    if(VTSS_BF_GET(vlan_entry->ports[isid_add], pit.iport) &&
                        VTSS_BF_GET(same_member_entry->ports[zero_based_isid], pit.iport)) {
                        if( same_member_entry->vid == vlan_entry->vid ) {
                            rc = VLAN_ERROR_VLAN_DYNAMIC_USER;
                        }
                    }
                }
            }
#endif
#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
            for (single_user_ct = (single_user_index_t)0;
                  single_user_ct < VLAN_SINGLE_CNT;single_user_ct++) {
                if (single_user_ct != single_user_idx){
                    single_member_entry = &vlan.vlan_single_membership_entries[single_user_ct];
                    vtss_vid_t *vid_ptr = &single_member_entry->vid[zero_based_isid][pit.iport];
                    if (VTSS_BF_GET(vlan_entry->ports[isid_add], pit.iport)) {
                        if(vlan_entry->vid == *vid_ptr) {
                            rc = VLAN_ERROR_VLAN_DYNAMIC_USER;
                        }
                    }
                }
            }
#endif
        }
        if(vlan_forbidden_table[vlan_entry->vid] == NULL) {
            new_forbidden_vlan = vlan_multi_membership_table[vlan_entry->vid];
            vlan_forbidden_table[vlan_entry->vid] = new_forbidden_vlan;
        } else {
            new_forbidden_vlan = vlan_forbidden_table[vlan_entry->vid];
        }
        if (new_forbidden_vlan == NULL) {
            rc = VTSS_RC_ERROR;
            goto exit_func;
        }
        // Overwrite the Portion respective to the isid_add.
        memcpy(&(new_forbidden_vlan->fb_port.ports[zero_based_isid][0]),
               &(vlan_entry->ports[isid_add][0]), VTSS_PORT_BF_SIZE);
    }else {
        T_D("Invalid VLAN User - %s", vlan_user_id_txt(user));
    }
    exit_func:
        T_D(":exit");
        VLAN_CRIT_EXIT();
        return rc;
}

/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/

/****************************************************************************/
// vlan_list_vlan_add()
// Adds or overwrites a vlan to @isid_add.
// @isid_add may be either
// VTSS_ISID_LEGAL: Any legal switch ID, i.e. we modify the stack-specific list.
// Output: Return VTSS_OK if success                                        
/****************************************************************************/
static vtss_rc vlan_list_vlan_add(vlan_entry_conf_t *vlan_entry, vtss_isid_t isid_add, vlan_user_t user)
{
    vlan_t         *new_vlan = NULL;
    vtss_rc        rc = VTSS_OK;
    vlan_list_t    *list;
    vtss_isid_t    zero_based_isid = isid_add - VTSS_ISID_START;
    port_iter_t    pit;
   
    #ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    same_user_index_t    same_user_ct = VLAN_SAME_CNT;
    same_user_index_t    same_user_idx  = VLAN_SAME_CNT;
    vlan_same_membership_entry_t   *same_member_entry = NULL;
    #endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */
    
    multi_user_index_t   multi_user_idx = VLAN_MULTI_CNT;
    
    #ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    single_user_index_t  single_user_idx  = VLAN_SINGLE_CNT;
    vlan_single_membership_entry_t *single_member_entry = NULL;
    #endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
    
    T_D("enter: vid %d isid_add %d user:%s",
    vlan_entry->vid, isid_add, vlan_user_id_txt(user));
    
    VLAN_CRIT_ENTER();
    
    if (vlan_entry->vid >= VTSS_VIDS) {
        rc = VTSS_RC_ERROR;
        goto exit_func;
    }
    
    if (IF_MULTI_USER_GET_IDX(user, &multi_user_idx)) {
        list = &vlan.stack_vlan;
        
        /* Now, prev_vlan points to the entry to which to append the
        new entry, or in case of an overwrite, the entry to overwrite.
        The remainder of the list is given by temp_vlan. */
        
        /* If the vlan entry not exist in forbidden list then add in vlan entry table*/
        if(vlan_forbidden_table[vlan_entry->vid] == NULL) {
            if (vlan_multi_membership_table[vlan_entry->vid] == NULL) {
                if (list->free == NULL) {
                    rc = VLAN_ERROR_VLAN_TABLE_FULL;
                    T_D("VLAN Table full");
                    
                    /* goto statements are quite alright when we always need to do 
                    something special before exiting a function. */
                    goto exit_func;
                }
                // Pick the first item from the free list.
                new_vlan = list->free;
                list->free = list->free->next;
                new_vlan->next = NULL;
                
                vlan_multi_membership_table[vlan_entry->vid] = new_vlan;
            } else {
                new_vlan = vlan_multi_membership_table[vlan_entry->vid];
            }
            // Insert the new entry in the appropriate position in list->used.
        } else {
            /* vlan exist in forbidden list and if vlan user is STATIC user then skip to add in vlan list
               if other USERS add the vlan in vlan list and notify them as conflicts to user*/
            (void) port_iter_init(&pit, NULL, isid_add, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (VTSS_BF_GET(vlan_entry->ports[isid_add], pit.iport) && 
                (VTSS_BF_GET(vlan_forbidden_table[vlan_entry->vid]->fb_port.ports[zero_based_isid], pit.iport)) ) {
                    if( multi_user_idx == VLAN_USER_STATIC ) {
                        rc = VLAN_ERROR_VLAN_FORBIDDEN;
                        goto exit_func;
                    }
                }
            }
            
            if(vlan_multi_membership_table[vlan_entry->vid] == NULL) {
                new_vlan = vlan_forbidden_table[vlan_entry->vid];
                vlan_multi_membership_table[vlan_entry->vid] = new_vlan;
            } else {
                new_vlan = vlan_multi_membership_table[vlan_entry->vid];
            }
        }
        
        /* This check is to satisfy the Lint as Lint complains new_vlan can
        * be NULL which is not possible as can be seen above.
        */
        
        if (new_vlan == NULL) {
            rc = VTSS_RC_ERROR;
            T_D("new_vlan NULL");
            goto exit_func;
        }
        // Overwrite the Portion respective to the isid_add.
        memcpy(&(new_vlan->conf.ports[multi_user_idx][zero_based_isid][0]),
        &(vlan_entry->ports[isid_add][0]), VTSS_PORT_BF_SIZE);
    }
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    else if (IF_SAME_USER_GET_IDX(user, &same_user_idx)) {
        if (same_user_idx == VLAN_SAME_CNT) {
            rc = VTSS_RC_ERROR;
            goto exit_func;
        }
        same_member_entry = &(vlan.vlan_same_membership_entries[same_user_idx]);
        
        T_D("Before Checking for Previously Configured VID");
        
        /* Check if the User Entry is previously configured */
        if ((same_member_entry->vid != VLAN_ID_NONE) &&
        (same_member_entry->vid != vlan_entry->vid)) {
            T_D("%s User Previously Configured", vlan_user_id_txt(user));
            rc = VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED;
            goto exit_func;
        }
        
        T_D("Before Checking for Same VID User");
        /* Check for the other VLAN Id's with similar entries */
        for (same_user_ct = (same_user_index_t)0; same_user_ct < VLAN_SAME_CNT;
        same_user_ct++) {
            T_D("Checking User %s same_user_ct %d same_user_idx %d",
            vlan_user_id_txt(user), same_user_ct, same_user_idx );
            
            if (same_user_ct == same_user_idx)
                continue;
            
            same_member_entry = &(vlan.vlan_same_membership_entries[same_user_ct]);
            
            if (same_member_entry->vid == vlan_entry->vid) {
                rc = VLAN_ERROR_USER_VLAN_ID_MISMATCH;
                goto exit_func;
            }
        }
        
        T_D("Updating the %s Database", vlan_user_id_txt(user));
        same_member_entry = &(vlan.vlan_same_membership_entries[same_user_idx]);

        /* Update the User Data Base */
        same_member_entry->vid = vlan_entry->vid;
        // Overwrite the entire portion.
        memcpy(&(same_member_entry->ports[zero_based_isid][0]),
                &(vlan_entry->ports[isid_add][0]),VTSS_PORT_BF_SIZE);
        T_D("After Updating %s isid %d vid %d",
                vlan_user_id_txt(user), isid_add, vlan_entry->vid);
    }
#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */

#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    else if (IF_SINGLE_USER_GET_IDX(user, &single_user_idx)) {
        if (single_user_idx == VLAN_SINGLE_CNT) {
            rc = VTSS_RC_ERROR;
            goto exit_func;
        }
        
        single_member_entry = &vlan.vlan_single_membership_entries[single_user_idx];
        // Loop through and set our database's VID for ports set in the
        // users port list. Before setting, check that the entry is not
        // currently used, since that's not allowed for SINGLE users.
        // Also remember to clear out ports that match the user request's
        // VID, but are not set in the user's request (anymore).

        (void) port_iter_init(&pit, NULL, isid_add, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_vid_t *vid_ptr = &single_member_entry->vid[zero_based_isid][pit.iport];
            if (VTSS_BF_GET(vlan_entry->ports[isid_add], pit.iport)) {
                // User wants to make this port member of vlan_entry->vid.
                // Check if it's OK (and skip the stack ports in this check,
                // since they may have any VID value).
                if (*vid_ptr != 0 && *vid_ptr != vlan_entry->vid) {
                    rc = VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED;
                    goto exit_func;
                }
                *vid_ptr = vlan_entry->vid;
            } else if (*vid_ptr == vlan_entry->vid) {
            // User no longer wants this port to be member of the VLAN ID.
            *vid_ptr = 0;
            }
        }
    }
    
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
    else {
        T_D("Invalid VLAN User - %s", vlan_user_id_txt(user));
    }
    exit_func:
        T_D(":exit rc = 0x%x", rc);
        VLAN_CRIT_EXIT();
        return rc;
}

/****************************************************************************/
// vlan_list_vlan_forbidden_del()
// Deleting a forbidden port list on the stack works globally.
// All ISIDs get it removed.
/****************************************************************************/
static vtss_rc vlan_list_vlan_forbidden_del(vtss_isid_t isid_del,
                                vtss_vid_t vid, vlan_user_t user)
{
    vlan_list_t      *list;
    vtss_rc          rc = VTSS_OK;
    int              entry_del = TRUE;
    vtss_isid_t      zero_based_isid = isid_del - VTSS_ISID_START;
    multi_user_index_t   multi_user_idx = VLAN_MULTI_CNT;
    vlan_forbidden_list  *multi_member_entry = NULL;
    u8     empty_stack_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];
    u8     empty_ports[VTSS_PORT_BF_SIZE];
    
    VLAN_CRIT_ENTER();
    
    memset(&empty_stack_ports[0][0],0,sizeof(empty_stack_ports));
    memset(&empty_ports[0],0,sizeof(empty_ports));
    T_D("Delete old VLAN vid: %u", vid);
    
    if (IF_MULTI_USER_GET_IDX(user, &multi_user_idx)) {
        if (!vlan_forbidden_table[vid]) {
            rc = VLAN_ERROR_ENTRY_NOT_FOUND;
            goto exit_func;
        }
        
        list = &vlan.stack_vlan;
        multi_member_entry = &(vlan_forbidden_table[vid]->fb_port);
        
        // Move it to free list
        if (isid_del == VTSS_ISID_GLOBAL) {
            memset(&(multi_member_entry->ports[0][0]),0,sizeof(empty_stack_ports));
        } else {
            memset(&(multi_member_entry->ports[zero_based_isid][0]),0,sizeof(empty_ports));
        }
        
        if (memcmp(&(multi_member_entry->ports[0][0]),
        &(empty_stack_ports[0][0]), sizeof(empty_stack_ports))) {
            entry_del = FALSE;
        }
        
        if (entry_del) {
            if(vlan_multi_membership_table[vid]) {
                vlan_forbidden_table[vid] = NULL; /* free the NULL Ptr */
            } else {
                vlan_forbidden_table[vid]->next = list->free;
                list->free = vlan_forbidden_table[vid];
                vlan_forbidden_table[vid] = NULL; /* free the NULL Ptr */
            }
        }
    } else {
        T_D("Invalid VLAN User %s", vlan_user_id_txt(user));
    }
    
    exit_func:
        VLAN_CRIT_EXIT();
        return rc;
}

/****************************************************************************/
// vlan_list_vlan_del()
// If @local is TRUE, modifies vlan.switch_vlan, i.e. this switch's
// configuration. Otherwise modifies vlan.stack_vlan.
// Deleting a VLAN on the stack works globally. All ISIDs get it removed.
/****************************************************************************/
static vtss_rc vlan_list_vlan_del(vtss_isid_t isid_del, vtss_vid_t vid, vlan_user_t user)
{
    vlan_list_t   *list;
    vtss_rc       rc = VTSS_OK;
    int           entry_del = TRUE;
    vtss_isid_t   zero_based_isid = isid_del - VTSS_ISID_START;
    u8            empty_stack_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];
    u8            empty_ports[VTSS_PORT_BF_SIZE];
    
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    same_user_index_t               same_user_idx   = VLAN_SAME_CNT;
#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */
    multi_user_index_t              multi_user_idx  = VLAN_MULTI_CNT;
    vlan_multi_membership_entry_t   *multi_member_entry = NULL;
#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    single_user_index_t             single_user_idx = VLAN_SINGLE_CNT;
    vlan_single_membership_entry_t  *single_member_entry = NULL;
    port_iter_t                     pit;
    switch_iter_t                   sit;
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
    
    
    VLAN_CRIT_ENTER();
    memset(&empty_stack_ports[0][0],0,sizeof(empty_stack_ports));
    memset(&empty_ports[0],0,sizeof(empty_ports));
    T_D("Delete old VLAN vid: %u", vid);
    
    if (IF_MULTI_USER_GET_IDX(user, &multi_user_idx)) {
        if (!vlan_multi_membership_table[vid]) {
            rc = VLAN_ERROR_ENTRY_NOT_FOUND;
            goto exit_func;
        }
        list = &vlan.stack_vlan;
        multi_member_entry = &(vlan_multi_membership_table[vid]->conf);
        // Move it to free list
        if (isid_del == VTSS_ISID_GLOBAL) {
            memset(&(multi_member_entry->ports[multi_user_idx][0][0]),0,
                                       sizeof(empty_stack_ports));
        }  else {
            memset(&(multi_member_entry->ports[multi_user_idx][zero_based_isid][0]),0,
                                       sizeof(empty_ports));
        }
        
        for (multi_user_idx = (multi_user_index_t)0;
        multi_user_idx < VLAN_MULTI_CNT; multi_user_idx++) {
            if (memcmp(&(multi_member_entry->ports[multi_user_idx][0][0]),
            &(empty_stack_ports[0][0]), sizeof(empty_stack_ports))) {
                entry_del = FALSE;
            }
        }
        if (entry_del) {
            if (vlan_forbidden_table[vid]) {
                vlan_multi_membership_table[vid] = NULL; /* free the NULL Ptr */
            } else {
                vlan_multi_membership_table[vid]->next = list->free;
                list->free = vlan_multi_membership_table[vid];
                vlan_multi_membership_table[vid] = NULL; /* free the NULL Ptr */
            }
        }
    }
    
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    else if (IF_SAME_USER_GET_IDX(user, &same_user_idx)) {
        memset(&(vlan.vlan_same_membership_entries[same_user_idx]), 0,
        sizeof(vlan_same_membership_entry_t));
    }
#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */
    
#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    else if (IF_SINGLE_USER_GET_IDX(user, &single_user_idx)) {
        single_member_entry = &(vlan.vlan_single_membership_entries[single_user_idx]);
        /* Traverse all the Isid's and ports and update the VID information */
        (void) switch_iter_init(&sit, isid_del, SWITCH_ITER_SORT_ORDER_ISID_ALL);
        (void) port_iter_init(&pit, &sit, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (single_member_entry->vid[sit.isid - VTSS_ISID_START][pit.iport] == vid) {
                single_member_entry->vid[sit.isid - VTSS_ISID_START][pit.iport] = 0;
            }
        }
    }
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
    else {
        T_D("Invalid VLAN User %s", vlan_user_id_txt(user));
    }
    exit_func:
        VLAN_CRIT_EXIT();
        return rc;
}

/****************************************************************************/
// Help Routines used to give Strings for USER types
/****************************************************************************/

static char *vlan_user_id_txt(vlan_user_t user)
{
    char *txt;

    switch (user) {
    case VLAN_USER_STATIC :
        txt = "VLAN_USER_STATIC";
        break;
#ifdef VTSS_SW_OPTION_DOT1X
    case VLAN_USER_DOT1X  :
        txt = "VLAN_USER_DOT1X";
        break;
#endif





#ifdef VTSS_SW_OPTION_MVR
    case VLAN_USER_MVR :
        txt = "VLAN_USER_MVR";
        break;
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    case VLAN_USER_VOICE_VLAN :
        txt = "VLAN_USER_VOICE_VLAN";
        break;
#endif
#ifdef VTSS_SW_OPTION_MSTP
    case VLAN_USER_MSTP    :
        txt = "VLAN_USER_MSTP";
        break;
#endif
#ifdef VTSS_SW_OPTION_VCL
    case VLAN_USER_VCL    :
        txt = "VLAN_USER_VCL";
        break;
#endif
#ifdef VTSS_SW_OPTION_RSPAN
    case VLAN_USER_RSPAN    :
        txt = "VLAN_USER_RSPAN";
        break;
#endif
    case VLAN_USER_ALL     :
        txt = "VLAN_USER_ALL";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}


static char *vlan_msg_id_txt(vlan_msg_id_t msg_id)
{
    char *txt;

    switch (msg_id) {
    case VLAN_MSG_ID_CONF_SET_REQ:
        txt = "VLAN_CONF_SET_REQ";
        break;
    case VLAN_MSG_ID_CONF_ADD_REQ:
        txt = "VLAN_CONF_ADD_REQ";
        break;
    case VLAN_MSG_ID_CONF_DEL_REQ:
        txt = "VLAN_CONF_DEL_REQ";
        break;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    case VLAN_MSG_ID_CONF_TPID_REQ:
        txt = "VLAN_MSG_ID_CONF_TPID_REQ";
        break;
#endif
    case VLAN_MSG_ID_PORT_CONF_SET_REQ:
        txt = "VLAN_PORT_CONF_SET_REQ";
        break;
    case VLAN_MSG_ID_SINGLE_PORT_CONF_SET_REQ:
        txt = "VLAN_MSG_ID_SINGLE_PORT_CONF_SET_REQ";
        break;
    default:
        txt = "?";
        break;
    }
    return txt;
}

static void vlan_user_conf(vtss_isid_t isid, vid_t vid, vlan_user_t usr, BOOL add)
{
    u16             users;
    vtss_isid_t     isid_temp, isid_start, isid_end;

    VLAN_CRIT_ASSERT_LOCKED();
    if (isid == VTSS_ISID_GLOBAL) {
        isid_start = VTSS_ISID_START;
        isid_end = VTSS_ISID_END;
    } else {
        isid_start = isid;
        isid_end = isid + 1;
    }
    for (isid_temp = isid_start; isid_temp < isid_end; isid_temp++) {
        users = vlan.vlan_users[isid_temp - VTSS_ISID_START][vid - 1];
        if (add) {
            users = users | (1U << usr);
        } else {
            users = users & ~(1U << usr);
        }
        vlan.vlan_users[isid_temp - VTSS_ISID_START][vid - 1] = users;
    }
}

/* Allocate message buffer from normal request pool */
static vlan_msg_req_t *vlan_msg_alloc(vlan_msg_id_t msg_id, u32 ref_cnt)
{
    vlan_msg_req_t *msg;

    if (ref_cnt == 0) {
        return NULL;
    }

    msg = msg_buf_pool_get(vlan.request_pool);
    if (ref_cnt > 1) {
        msg_buf_pool_ref_cnt_set(msg, ref_cnt);
    }
    msg->msg_id = msg_id;
    return msg;
}

/* Allocate message buffer from the request pool meant for "large" messages */
static vlan_msg_large_req_t *vlan_msg_large_alloc(vlan_msg_id_t msg_id)
{
    vlan_msg_large_req_t *msg = msg_buf_pool_get(vlan.large_request_pool);
    msg->msg_id = msg_id;
    return msg;
}

/* Free the memory allocated to the message after sending */
static void vlan_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    vlan_msg_id_t msg_id = *(vlan_msg_id_t *)msg;

    T_D("msg_id: %d, %s", msg_id, vlan_msg_id_txt(msg_id));
    (void)msg_buf_pool_put(msg);
}

/* Send the VLAN Message */
static void vlan_msg_tx(void *msg, vtss_isid_t isid, size_t len)
{
    vlan_msg_id_t msg_id = *(vlan_msg_id_t *)msg;

    T_D("msg_id: %d, %s, len: %zd, isid: %d", msg_id, vlan_msg_id_txt(msg_id), len, isid);

    // Avoid "Warning -- Constant value Boolean" Lint warning, due to the use of MSG_TX_DATA_HDR_LEN_MAX() below
    /*lint -e(506) */
    msg_tx_adv(NULL, vlan_msg_tx_done, MSG_TX_OPT_DONT_FREE, VTSS_MODULE_ID_VLAN, isid, msg, len + MSG_TX_DATA_HDR_LEN_MAX(vlan_msg_req_t, req, vlan_msg_large_req_t, large_req));
}

/* VLAN Message Receive Handler */
static BOOL vlan_msg_rx(void *contxt, const void * const rx_msg, const size_t len, const vtss_module_id_t modid, const ulong isid)
{
    vlan_msg_req_t *msg = (vlan_msg_req_t *)rx_msg; // Plain and simple, when we only have request messages in this module (well, for the large_req, we override it in VLAN_MSG_ID_CONF_SET_REQ, but - by design - the message ID is at the same location)
    vtss_rc rc = VTSS_OK;

    T_D("msg_id: %d, %s, len: %zd, isid: %u", msg->msg_id, vlan_msg_id_txt(msg->msg_id), len, isid);


    switch (msg->msg_id) {
    case VLAN_MSG_ID_CONF_SET_REQ: {
        // Sets the whole configuration - not just one VLAN entry.
        // We need to delete all currently configured VLANs and
        // possibly create some new ones. This will shortly give
        // rise to no configured VLANs, but that's ignorable.
        vlan_msg_large_req_t *large_msg = (vlan_msg_large_req_t *)rx_msg;
        vtss_vid_t           vid_trav = VTSS_VID_NULL;
        u32                  j = 0, count;
        vlan_entry_conf_t    vlan_entry;

        memset(&vlan_entry,0,sizeof(vlan_entry_conf_t));

        // Delete all existing entries.
        for (vid_trav = VTSS_VID_DEFAULT ; vid_trav < VTSS_VIDS; vid_trav++) {
            vlan_entry.vid = vid_trav;
            rc = vlan_entry_add(VLAN_DELETE, &vlan_entry, VTSS_ISID_LOCAL);
            if (rc != VTSS_OK) {
                T_D("vlan_entry_add(delete) returns error-[%d]",rc);
                return FALSE;
            }
        }

        count = large_msg->large_req.set.count;
        if (count > VLAN_MAX) {
            T_E("Invalid VLAN count (%u)", count);
            count = VLAN_MAX;
        }

        // Add the requested VLANs
        for (j = 0; j < count; j++) {

            /* Update the Hardware */
            rc = vlan_entry_add(VLAN_ADD, &large_msg->large_req.set.table[j], VTSS_ISID_LOCAL);
            if (rc != VTSS_RC_OK) {
                T_E("vlan_entry_add() failed");
            }
        }
        break;
    }

    case VLAN_MSG_ID_PORT_CONF_SET_REQ: {
        vlan_port_conf_t *conf;
        port_iter_t      pit;

        (void) port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            conf = &msg->req.port_set.conf[pit.iport];
            T_D("Calling vlan_port_conf_set from VLAN_MSG_ID_PORT_CONF_SET_REQ with isid %u", isid);
            (void)vlan_port_conf_set(pit.iport, conf);
        }
        break;
    }

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    case VLAN_MSG_ID_CONF_TPID_REQ: {
        vtss_vlan_conf_t conf;

        conf.s_etype = msg->req.tpid.conf;
        T_D("EType = 0x%x\n", conf.s_etype);
        vlan_s_port_tpid_conf_change_callback(conf.s_etype); 
        if (vtss_vlan_conf_set(NULL, &conf) != VTSS_RC_OK) {
            T_D("Setting TPID Configuration failed\n");
        }
        break;
    }
#endif

    case VLAN_MSG_ID_SINGLE_PORT_CONF_SET_REQ: {
        vlan_port_conf_t conf;

        T_D("Calling vlan_port_conf_set from VLAN_MSG_ID_SINGLE_PORT_CONF_SET_REQ with port# %u", msg->req.single_port_set.port);

        if (vtss_vlan_port_conf_set(NULL, msg->req.single_port_set.port, &msg->req.single_port_set.conf) != VTSS_RC_OK) {
            T_E("vtss_vlan_port_conf_set() failed");
        }
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        conf.port_type = msg->req.single_port_set.conf.port_type;
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
        conf.port_type = msg->req.single_port_set.conf.aware;
#endif
        conf.pvid = msg->req.single_port_set.conf.pvid;
        conf.ingress_filter = msg->req.single_port_set.conf.ingress_filter;
        conf.frame_type = msg->req.single_port_set.conf.frame_type;
        conf.untagged_vid = msg->req.single_port_set.conf.untagged_vid;
        conf.tx_tag_type = 0;
        conf.flags = VLAN_PORT_FLAGS_ALL;
        /* Signal to all modules that want to know that the VLAN port configuration
           has changed. */
        vlan_port_conf_change_callback(msg->req.single_port_set.port, &conf);
        break;
    }

    case VLAN_MSG_ID_CONF_ADD_REQ: {
        T_D(" - VLAN_ADD rx_msg, isid %u, vid=%u", isid, msg->req.add.conf.vid);

        /* Update the Hardware tables */
        rc = vlan_entry_add(VLAN_ADD, &msg->req.add.conf, VTSS_ISID_LOCAL);
        break;
    }

    case VLAN_MSG_ID_CONF_DEL_REQ: {
        T_D(" - VLAN_DELETE  rx_msg, isid %u, vid=%u", isid, msg->req.del.conf.vid);
        /* Update the Hardware */
        rc = vlan_entry_add(VLAN_DELETE, &msg->req.del.conf, VTSS_ISID_LOCAL);
        break;
    }

    default:
        T_W("unknown message ID: %d", msg->msg_id);
        break;
    }

    return rc == VTSS_RC_OK ? TRUE : FALSE;
}

/* Register to the stack call back */
static vtss_rc vlan_stack_register(void)
{
    msg_rx_filter_t filter;

    memset(&filter, 0, sizeof(filter));
    filter.cb = vlan_msg_rx;
    filter.modid = VTSS_MODULE_ID_VLAN;
    return msg_rx_filter_register(&filter);
}

/* Set port configuration */
static vtss_rc vlan_stack_port_conf_set(vtss_isid_t isid)
{
    vlan_msg_req_t *msg;

    T_D("enter, isid: %d", isid);

    msg = vlan_msg_alloc(VLAN_MSG_ID_PORT_CONF_SET_REQ, 1);
    VLAN_CRIT_ENTER();
    memcpy(msg->req.port_set.conf, vlan.vlan_port_conf[VLAN_USER_ALL][isid - VTSS_ISID_START], sizeof(msg->req.port_set.conf));
    VLAN_CRIT_EXIT();

    vlan_msg_tx(msg, isid, sizeof(msg->req.port_set));
    T_D("exit, isid: %d", isid);

    return VTSS_OK;
}

/** Set port configuration */
static vtss_rc vlan_single_stack_port_conf_set(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vlan_port_conf_t *conf)
{
    vlan_msg_req_t *msg;

    T_D("enter, isid: %d", isid);

    msg = vlan_msg_alloc(VLAN_MSG_ID_SINGLE_PORT_CONF_SET_REQ, 1);
    msg->req.single_port_set.port = port_no;
    memcpy(&msg->req.single_port_set.conf, conf, sizeof(msg->req.single_port_set.conf));
    vlan_msg_tx(msg, isid, sizeof(msg->req.single_port_set));
    T_D("exit, isid: %d", isid);

    return VTSS_OK;
}

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
/** Set TPID configuration */
static vtss_rc vlan_tpid_conf_set(vtss_etype_t *tpid)
{
    vlan_msg_req_t *msg;
    switch_iter_t  sit;

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);

    /* Allocate a message with a ref-count corresponding to the number of times switch_iter_getnext() will return TRUE. */
    if ((msg = vlan_msg_alloc(VLAN_MSG_ID_CONF_TPID_REQ, sit.remaining)) != NULL) {
        memcpy(&msg->req.tpid.conf, tpid, sizeof(msg->req.tpid.conf));
        while (switch_iter_getnext(&sit)) {
            vlan_msg_tx(msg, sit.isid, sizeof(msg->req.tpid.conf));
        }
    }

    return VTSS_OK;
}
#endif

/* Set VLAN configuration to switch */
static vtss_rc vlan_stack_vlan_conf_set(vtss_isid_t isid_add)
{
    vlan_msg_large_req_t *msg;
    vtss_isid_t          zero_based_isid;
    vtss_vid_t           vlan_idx;
    int                  i;
    switch_iter_t        sit;

    T_D("enter, isid_add: %d", isid_add);
    (void) switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        zero_based_isid = sit.isid - VTSS_ISID_START;
        msg = vlan_msg_large_alloc(VLAN_MSG_ID_CONF_SET_REQ);
        i = 0;

        VLAN_CRIT_ENTER();

        /* After Reboot/System Restore default, only the static entries have to be updated */
        for (vlan_idx = VTSS_VID_DEFAULT; vlan_idx < VTSS_VIDS; vlan_idx++) {
            if (vlan_multi_membership_table[vlan_idx]) {
                msg->large_req.set.table[i].vid = vlan_idx;
                memcpy(&msg->large_req.set.table[i].ports[VTSS_ISID_LOCAL][0], &vlan_multi_membership_table[vlan_idx]->conf.ports[VLAN_USER_STATIC][zero_based_isid][0], VTSS_PORT_BF_SIZE);
                i++;
            }
        }

        if (i > VLAN_MAX) {
            i = VLAN_MAX;
            T_E("Internal code error");
        }
        msg->large_req.set.count = i;

        VLAN_CRIT_EXIT();

        vlan_msg_tx(msg, sit.isid, sizeof(msg->large_req.set) - (VLAN_MAX - i) * sizeof(vlan_entry_conf_t));
    }

    T_D("exit, isid: %d", isid_add);
    return VTSS_OK;
}

/* Gets the Port Bitmap of all the VLAN Users for a VID*/
static vtss_rc vlan_list_vlan_get_port_bitmap(vtss_isid_t isid_min, vtss_isid_t isid_max, vtss_vid_t vid, vlan_entry_conf_t *conf)
{
    u8                              empty_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];
    vtss_isid_t                     isid = VTSS_ISID_START;
    vtss_isid_t                     zero_based_isid = VTSS_ISID_START;
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    same_user_index_t               same_user_ct   = VLAN_SAME_CNT;
    vlan_same_membership_entry_t    *same_member_entry = NULL;
#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */
#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    vlan_single_membership_entry_t  *single_member_entry = NULL;
    single_user_index_t             single_user_ct = VLAN_SINGLE_CNT;
    port_iter_t                     pit;
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
    multi_user_index_t              multi_user_ct  = VLAN_MULTI_CNT;
    vlan_multi_membership_entry_t   *multi_member_entry = NULL;
    BOOL                            found = 0;
    int                             j;

    T_D("enter, isid_min: %d, isid_max: %d vid: %u", isid_min, isid_max, vid);

    VLAN_CRIT_ENTER();
    memset(conf,0,sizeof(vlan_entry_conf_t));
    memset(&empty_ports[0][0],0,sizeof(empty_ports));
    /* We can either call vlan_mgmt_vlan_get or populate the port
     * membership locally. vlan_mgmt_vlan_get is a costly operation, so
     * populating the port information locally */
    /* First Populate the MULTI_VLAN_USERS Information */
    conf->vid = vid;
    if (vlan_multi_membership_table[vid]) {
        found = 1;
        multi_member_entry = &(vlan_multi_membership_table[vid]->conf);
        for (multi_user_ct = (multi_user_index_t) 0;
             multi_user_ct < VLAN_MULTI_CNT; multi_user_ct++) {
            for (isid = isid_min; isid < isid_max; isid++) {
                zero_based_isid = isid - VTSS_ISID_START;
                for (j = 0; j < VTSS_PORT_BF_SIZE; j++) {
                    conf->ports[isid][j] |= multi_member_entry->
                        ports[multi_user_ct][zero_based_isid][j];
                }
            }
        }
    }
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    /* Populate the Same VLAN Users Information */
    for (same_user_ct = (same_user_index_t) 0; same_user_ct < VLAN_SAME_CNT;
         same_user_ct++) {
        if (vlan.vlan_same_membership_entries[same_user_ct].vid == vid) {
            found = 1;
            same_member_entry =
                &(vlan.vlan_same_membership_entries[same_user_ct]);
            for (isid = isid_min; isid < isid_max; isid++) {
                zero_based_isid = isid - VTSS_ISID_START;
                for (j = 0; j < VTSS_PORT_BF_SIZE; j++) {
                    conf->ports[isid][j] |=
                        same_member_entry->ports[zero_based_isid][j];
                }
            }
        }
    }
#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */
    /* Populate the Single VLAN Users Information */
#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    for (single_user_ct = (single_user_index_t) 0;  single_user_ct < VLAN_SINGLE_CNT; single_user_ct++) {
        single_member_entry = &(vlan.vlan_single_membership_entries[single_user_ct]);
        for (isid = isid_min; isid < isid_max; isid++) {
            zero_based_isid = isid - VTSS_ISID_START;
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (single_member_entry->vid[zero_based_isid][pit.iport] == vid) {
                    found = 1;
                    VTSS_BF_SET(empty_ports[zero_based_isid], pit.iport, 0x1);
                }
            }
            for (j = 0; j < VTSS_PORT_BF_SIZE; j++) {
                conf->ports[isid][j] |= empty_ports[zero_based_isid][j];
            }
        }
    }
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
    VLAN_CRIT_EXIT();
    if (found) {
        return VTSS_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

/*****************************************************************/
// vlan_stack_vlan_conf_add()
// Transmit the VLAN configuration to @isid_add for vid = @vid.
// @isid may be VTSS_ISID_GLOBAL if the configuration for @vid
// should be sent to all existing switches in the stack. Otherwise
// @isid is just a legal SID for which the VID's port configuration
// has changed (i.e. the VID already existed).
/*****************************************************************/
static void vlan_stack_vlan_conf_add(vtss_isid_t isid_add, vtss_vid_t vid)
{
    vlan_msg_req_t    *msg;
    vtss_isid_t       isid_min, isid_max;
    vlan_entry_conf_t conf;
    int               j;
    switch_iter_t     sit;

    T_D("enter, isid_add: %d, vid: %u", isid_add, vid);
    // Find boundaries
    if (isid_add == VTSS_ISID_GLOBAL) {
        isid_min = VTSS_ISID_START;
        isid_max = VTSS_ISID_END;
    } else {
        isid_min = isid_add;
        isid_max = isid_add+1;
    }
    memset(&conf,0,sizeof(vlan_entry_conf_t));
    if (vlan_list_vlan_get_port_bitmap(isid_min, isid_max, vid, &conf) == VTSS_RC_ERROR) {
        return;
    }
    VLAN_CRIT_ENTER();
    /* Mask with forbidden list. Forbidden port list have high priority. Dynamic VLAN 
       users will not configure on hardware if port is forbidden
     */
    if(vlan_forbidden_table[vid] != NULL) {
        (void) switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID_ALL);
        while (switch_iter_getnext(&sit)) {
            for (j = 0; j < VTSS_PORT_BF_SIZE; j++) {
                conf.ports[sit.isid][j] = (conf.ports[sit.isid][j]& ~(vlan_forbidden_table[vid]->fb_port.ports[sit.isid-1][j]));
            }
        }
    }
    VLAN_CRIT_EXIT();
    (void) switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        msg = vlan_msg_alloc(VLAN_MSG_ID_CONF_ADD_REQ, 1);
        msg->req.add.conf.vid = conf.vid;
        memcpy(&msg->req.add.conf.ports[VTSS_ISID_LOCAL][0], &conf.ports[sit.isid][0], VTSS_PORT_BF_SIZE);
        vlan_msg_tx(msg, sit.isid, sizeof(msg->req.add));
    }
    T_D("exit, isid: %d, vid: %u", isid_add, conf.vid);
    return;
}

/* Delete VLAN from switch */
static void vlan_stack_vlan_conf_del(vtss_isid_t        isid_del, 
                                     vlan_entry_conf_t  *vlan_entry)
{
    vlan_msg_req_t    *msg;
    vlan_entry_conf_t conf;
    switch_iter_t     sit;
   
    T_D("enter, isid: %d, vid: %u", isid_del, vlan_entry->vid);
    (void) switch_iter_init(&sit, isid_del, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    VLAN_CRIT_ENTER();
    conf.vid = vlan_entry->vid;
    while (switch_iter_getnext(&sit)) {
        memcpy(&conf.ports[sit.isid][0], &vlan_entry->ports[sit.isid][0], VTSS_PORT_BF_SIZE);
    }
    VLAN_CRIT_EXIT();

    (void) switch_iter_init(&sit, isid_del, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        msg = vlan_msg_alloc(VLAN_MSG_ID_CONF_DEL_REQ, 1);
        memset(&msg->req.del.conf, 0, sizeof(vlan_entry_conf_t));
        msg->req.del.conf.vid = conf.vid;
        memcpy(&msg->req.del.conf.ports[VTSS_ISID_LOCAL][0], &conf.ports[sit.isid][0], VTSS_PORT_BF_SIZE);
        vlan_msg_tx(msg, sit.isid, sizeof(msg->req.del));
    }
    T_D("exit, isid: %d, vid: %u", isid_del, conf.vid);
}

/****************************************************************************/
/*  Management functions                                                    */
/****************************************************************************/

/* VLAN error text */
char *vlan_error_txt(vtss_rc rc)
{
    char *txt;

    switch (rc) {
    case VLAN_ERROR_GEN:
        txt = "VLAN generic error";
        break;
    case VLAN_ERROR_PARM:
        txt = "VLAN parameter error";
        break;
    case VLAN_ERROR_ENTRY_NOT_FOUND:
        txt = "Entry not found";
        break;
    case VLAN_ERROR_VLAN_TABLE_FULL:
        txt = "VLAN table full";
        break;
    case VLAN_ERROR_VLAN_TABLE_EMPTY:
        txt = "VLAN table empty";
        break;
    case VLAN_ERROR_USER_VLAN_ID_MISMATCH:
        txt = "VLAN Error VID Mismatch";
        break;
    case VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED:
        txt = "VLAN Error Previously Configured";
        break;
    case VLAN_ERROR_VLAN_FORBIDDEN:
        txt = "VLAN ID In Forbidden List";
        break;
    default:
        txt = "VLAN unknown error";
        break;
    }
    return txt;
}

/* Determine if port and ISID are valid */
static BOOL vlan_mgmt_port_sid_invalid(vtss_isid_t isid, vtss_port_no_t port_no, BOOL set)
{
    if (port_no >= port_isid_port_count(isid)) {
        T_D("illegal port_no: %u", port_no);
        return 1;
    }
    if (port_isid_port_no_is_stack(isid, port_no)) {
        T_D("stack port is not allowed. port_no: %u", port_no);
        return 1;
    }
    /* Check ISID */
    if (isid >= VTSS_ISID_END) {
        T_W("illegal isid: %d", isid);
        return 1;
    }
    if (set && isid == VTSS_ISID_LOCAL) {
        T_W("SET not allowed, isid: %d", isid);
        return 1;
    }

    return 0;
}

#ifdef  VOLATILE_VLAN_USER_PRESENT
static vtss_rc vlan_check_conflict(vlan_port_conf_t *first_conf, 
       vlan_port_conf_t *second_conf, u8 flag, 
       vlan_user_t user, vlan_port_conflicts_t *conflicts)
{
    if ((first_conf == NULL) || (second_conf == NULL) || 
        (conflicts == NULL)) {
        T_D("Invalid input parameters");
        return VTSS_RC_ERROR;
    }
    if (flag == VLAN_PORT_FLAGS_PVID) {
        if (second_conf->flags & VLAN_PORT_FLAGS_PVID) {
            // This VLAN user wants to control the port's pvid
            if (first_conf->flags & VLAN_PORT_FLAGS_PVID) {
                // So did a previous user.                
                if (first_conf->pvid != second_conf->pvid) {
                    /* And the previous module wanted another value of the pvid 
                     * flag than we do.  But since he came before us in the 
                     * vlan_user_t enumeration, he wins. 
                     */
                    conflicts->port_flags |= VLAN_PORT_FLAGS_PVID;
                    conflicts->vlan_users[VLAN_PORT_PVID_IDX] |= (1 << (u8)user);
                    T_D("VLAN PVID Conflict");
                } else {
                    /* Luckily, the previous module wants to set pvid to the 
                     *  same value as us. No conflict. 
                     */
                } /* if (first_conf->pvid != second_conf->pvid */
            } else {
                /* VLAN pvid not previously overridden, but this user wants 
                 * to override it. 
                 */
                first_conf->pvid = second_conf->pvid;
                first_conf->flags |= VLAN_PORT_FLAGS_PVID; // Overridden
            } /* if (first_conf->flags & VLAN_PORT_FLAGS_PVID) */
        }
    } else if (flag == VLAN_PORT_FLAGS_AWARE) {
        if (second_conf->flags & VLAN_PORT_FLAGS_AWARE) {
            // This VLAN user wants to control the port's VLAN awareness
            if (first_conf->flags & VLAN_PORT_FLAGS_AWARE) {
                // So did a previous user.
                if (first_conf->port_type != second_conf->port_type) {
                    /* And the previous module wanted another value of the 
                     * awareness flag than we do. But since he came before 
                     * us in the vlan_user_t enumeration, he wins. 
                     */
                    conflicts->port_flags |= VLAN_PORT_FLAGS_AWARE;
                    conflicts->vlan_users[VLAN_PORT_PTYPE_IDX] 
                        |= (1 << (u8)user);
                    T_D("VLAN Awareness Conflict");
                } else {
                    /* Luckily, the previous module wants to set awareness 
                     * to the same value as us. No conflict. 
                     */
                } /* if (first_conf->port_type != second_conf->port_type)  */
            } else {
                /* VLAN awareness not previously overridden, but this user
                 * wants to override it. 
                 */
                first_conf->port_type = second_conf->port_type;
                first_conf->flags |= VLAN_PORT_FLAGS_AWARE; // Overridden
            } /* if (first_conf->flags & VLAN_PORT_FLAGS_AWARE)  */
        }
    } else if (flag == VLAN_PORT_FLAGS_INGR_FILT) {
        if (second_conf->flags & VLAN_PORT_FLAGS_INGR_FILT) {
            // This VLAN user wants to control the port's VLAN ingress_filter
            if (first_conf->flags & VLAN_PORT_FLAGS_INGR_FILT) {
                // So did a previous user.
                if (first_conf->ingress_filter != second_conf->ingress_filter) {
                    /* And the previous module wanted another value of the 
                     * ingress_filter flag than we do. But since he came 
                     * before us in the vlan_user_t enumeration, he wins. 
                     */
                    conflicts->port_flags  |= VLAN_PORT_FLAGS_INGR_FILT;
                    conflicts->vlan_users[VLAN_PORT_INGR_FILT_IDX] 
                        |= (1 << (u8)user);
                    T_D("VLAN Ingress Filter Conflict");
                } else {
                    /* Luckily, the previous module wants to set ingress_filter
                     * to the same value as us. No conflict.
                     */
                } /* if (first_conf->ingress_filter != second_conf->ingress  */
            } else {
                /* VLAN ingress_filter not previously overridden, but this 
                 * user wants to override it. 
                 */
                first_conf->ingress_filter = second_conf->ingress_filter;
                first_conf->flags |= VLAN_PORT_FLAGS_INGR_FILT; // Overridden
            } /* if (first_conf->flags & VLAN_PORT_FLAGS_INGR_FILT)  */
        }
    } else if (flag == VLAN_PORT_FLAGS_RX_TAG_TYPE) {
        if (second_conf->flags & VLAN_PORT_FLAGS_RX_TAG_TYPE) {
            // This VLAN user wants to control the port's ingress frame_type
            if (first_conf->flags & VLAN_PORT_FLAGS_RX_TAG_TYPE) {
                // So did a previous user.                
                if (first_conf->frame_type != second_conf->frame_type) {
                    /* And the previous module wanted another value of the 
                     * ingress frame_type flag than we do. But since he came 
                     * before us in the vlan_user_t enumeration, he wins. 
                     */
                    conflicts->port_flags |= VLAN_PORT_FLAGS_RX_TAG_TYPE;
                    conflicts->vlan_users[VLAN_PORT_RX_TAG_TYPE_IDX] |= 
                        (1 << (u8)user);
                    T_D("VLAN Frame_Type Conflict");
                } else {
                    /* Luckily, the previous module wants to set ingress 
                     * frame_type to the same value as us. No conflict.
                     */
                } /* if (first_conf->frame_type != second_conf->frame_type)  */
            } else {
                /* VLAN ingress frame_type not previously overridden, 
                 * but this user wants to override it.
                 */
                first_conf->frame_type = second_conf->frame_type;
                first_conf->flags |= VLAN_PORT_FLAGS_RX_TAG_TYPE; // Overridden
            } /* if (first_conf->flags & VLAN_PORT_FLAGS_RX_TAG_TYPE)  */
        }
    } else {
        T_D("Invalid input flag");
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

static vtss_rc vlan_check_uvid_conflicts(vtss_isid_t isid, vtss_port_no_t port, 
                                         vlan_port_conf_t *actual_conf, 
                                         BOOL *tag_pvid, 
                                         vlan_port_conflicts_t *vlan_conflicts)
{
    /* This one is used to figure out whether we're allowed to send all frames 
     * untagged or not. 
     */
    BOOL at_least_one_module_wants_to_send_a_specific_vid_tagged = FALSE;
    vlan_user_t user;
    vlan_port_conf_t temp_conf;

    if ((actual_conf == NULL) || (tag_pvid == NULL) || 
            (vlan_conflicts == NULL)) {
        T_D ("Invalid input parameters");
        return VTSS_RC_ERROR;
    }

    /* Check for .tx_tag_type. In the loop below, actual_conf->pvid must 
     * be resolved. That's the reason for iterating over the VLAN users 
     * in two tempi. 
     */
    for (user = (VLAN_USER_STATIC + 1); user < VLAN_USER_ALL; user++) {
        temp_conf = vlan.
        vlan_port_conf[user][isid - VTSS_ISID_START][port - VTSS_PORT_NO_START];

        if (temp_conf.flags & VLAN_PORT_FLAGS_TX_TAG_TYPE) {
            // This VLAN user wants to control the Tx Tagging on this port
            if (actual_conf->flags & VLAN_PORT_FLAGS_TX_TAG_TYPE) {
                /* So did a higher-prioritized user. Let's see if this gives 
                 * rise to conflicts. 
                 */
                switch (actual_conf->tx_tag_type) {
                    case VLAN_TX_TAG_TYPE_UNTAG_THIS:
                        switch (temp_conf.tx_tag_type) {
                            case VLAN_TX_TAG_TYPE_UNTAG_THIS:
                                /* A previous module wants to send untagged, 
                                 * and so do we. If the .uvid of the previous 
                                 * one is the same as ours, we're OK with it.
                                 */
                                if (actual_conf->untagged_vid != 
                                        temp_conf.untagged_vid) {
                                    /* The previous module wants to untag a 
                                     * specific VLAN ID, which is not the same 
                                     * as the one we want to untag. 
                                     */
                                    if (at_least_one_module_wants_to_send_a_specific_vid_tagged) {
                                        /* At the same time, another user 
                                         * module wants to send a specific VID 
                                         * tagged. This is not possible, 
                                         * because we want to resolve the 
                                         * "two-different-untagged-VIDs" 
                                         * conflict by sending all frames 
                                         * untagged. 
                                         */
                                        vlan_conflicts->port_flags = 
                                            vlan_conflicts->port_flags | 
                                            VLAN_PORT_FLAGS_TX_TAG_TYPE;
                                        vlan_conflicts->
                                            vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX]
                                            |= (1 << (u8)user);
                                    } else {
                                        actual_conf->tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_ALL;
                                    }
                                }
                                break;
                            case VLAN_TX_TAG_TYPE_UNTAG_ALL:
                                if (at_least_one_module_wants_to_send_a_specific_vid_tagged) {
                                    /* At the same time, another user module wants to send 
                                     * a specific VID tagged. This is not possible.
                                     */
                                    vlan_conflicts->port_flags =
                                        vlan_conflicts->port_flags |
                                        VLAN_PORT_FLAGS_TX_TAG_TYPE;
                                    vlan_conflicts->
                                        vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX]
                                        |= (1 << (u8)user);
                                } else {
                                    actual_conf->tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_ALL;
                                }
                                break;

                            case VLAN_TX_TAG_TYPE_TAG_THIS:
                                /* A previous module wants to send untagged, but
                                 * we want to send tagged. This is possible if 
                                 * not two or more previous modules want to send
                                 * untagged (indicated by .uvid = VTSS_VID_ALL),
                                 * and if the .uvid we want to send tagged is 
                                 * different from the .uvid that a previous 
                                 * user wants to send untagged. 
                                 */
                                if (actual_conf->untagged_vid == 
                                                        temp_conf.untagged_vid) {
                                    vlan_conflicts->port_flags = 
                                        vlan_conflicts->port_flags | 
                                        VLAN_PORT_FLAGS_TX_TAG_TYPE;
                                    vlan_conflicts->
                                        vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX] |=
                                        (1 << (u8)user);
                                } else {
                                    /* Do not overwrite actual_conf->uvid, but 
                                     * keep track of the fact that at least one
                                     * module wants to send a specific VID 
                                     * tagged. 
                                     */
                                    at_least_one_module_wants_to_send_a_specific_vid_tagged = TRUE;
                                }
                                break;

                            case VLAN_TX_TAG_TYPE_TAG_ALL:
                                /* A previous module wants to untag a specific 
                                 * VID, but we want to tag all. This is 
                                 * impossible. 
                                 */
                                vlan_conflicts->port_flags = 
                                    vlan_conflicts->port_flags | 
                                    VLAN_PORT_FLAGS_TX_TAG_TYPE;
                                vlan_conflicts->
                                    vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX] |= 
                                    (1 << (u8)user);
                                break;

                            default:
                                T_E("Actual config tx untag this, current " 
                                    "config Tx Tag error");
                                return VTSS_RC_ERROR;
                        }
                        break;

                    case VLAN_TX_TAG_TYPE_UNTAG_ALL:
                        switch (temp_conf.tx_tag_type) {
                            case VLAN_TX_TAG_TYPE_UNTAG_THIS:
                            case VLAN_TX_TAG_TYPE_UNTAG_ALL:
                                /* A previous user wants to send all
                                 * vids untagged and current user either 
                                 * wants to untag one vid or all vids.
                                 */
                                break;
                            case VLAN_TX_TAG_TYPE_TAG_THIS:
                            case VLAN_TX_TAG_TYPE_TAG_ALL:
                                /* A previous user wants to send all
                                 * vids untagged. Hence current user
                                 * cannot tag a vid. Impossible.
                                 */
                                vlan_conflicts->port_flags = 
                                    vlan_conflicts->port_flags | 
                                    VLAN_PORT_FLAGS_TX_TAG_TYPE;
                                vlan_conflicts->
                                    vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX] |= 
                                    (1 << (u8)user);
                                break;
                        }
                        break;

                    case VLAN_TX_TAG_TYPE_TAG_THIS:
                        switch (temp_conf.tx_tag_type) {
                            case VLAN_TX_TAG_TYPE_UNTAG_THIS:
                                /* A previous module wants to send a specific 
                                 * VID tagged, and we want to send a specific 
                                 * VID untagged. This is possible if the 
                                 * .uvids are not the same. 
                                 */
                                if (actual_conf->untagged_vid != 
                                        temp_conf.untagged_vid) {
                                    /* Here we have to change the tag-type 
                                     * to "untag", and set the uvid to the 
                                     * untagged uvid. 
                                     */ 
                                    actual_conf->tx_tag_type   = 
                                        VLAN_TX_TAG_TYPE_UNTAG_THIS;
                                    actual_conf->untagged_vid  = 
                                        temp_conf.untagged_vid;
                                    /* And remember that at least one module 
                                     * wanted to send a specific VID tagged.
                                     */
                                    at_least_one_module_wants_to_send_a_specific_vid_tagged 
                                        = TRUE;
                                } else {
                                    /* Cannot send the same VID tagged and 
                                     * untagged at the same time. 
                                     */
                                    vlan_conflicts->port_flags = 
                                        vlan_conflicts->port_flags | 
                                        VLAN_PORT_FLAGS_TX_TAG_TYPE;
                                    vlan_conflicts->
                                        vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX] |= 
                                        (1 << (u8)user);
                                }
                                break;

                            case VLAN_TX_TAG_TYPE_UNTAG_ALL:
                                /* Cannot send the untag_all and 
                                 * tag a vid at the same time. 
                                 */
                                vlan_conflicts->port_flags =
                                    vlan_conflicts->port_flags |
                                    VLAN_PORT_FLAGS_TX_TAG_TYPE;
                                vlan_conflicts->
                                    vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX] |=
                                    (1 << (u8)user);
                                break;

                            case VLAN_TX_TAG_TYPE_TAG_THIS:
                                if (actual_conf->untagged_vid != 
                                        temp_conf.untagged_vid) {
                                    // Tell the API to send all frames tagged.
                                    actual_conf->tx_tag_type = 
                                        VLAN_TX_TAG_TYPE_TAG_ALL;
                                } else {
                                    /* Both a previous module and we want to 
                                     * tag a specific VID. No problem. 
                                     */
                                }
                                break;

                            case VLAN_TX_TAG_TYPE_TAG_ALL:
                                /* A previous module wants to tag a specific VID
                                 * , and we want to tag all. No problem, but 
                                 * change the tx_tag_type to this more 
                                 * restrictive. 
                                 */
                                actual_conf->tx_tag_type = 
                                    VLAN_TX_TAG_TYPE_TAG_ALL;
                                break;

                            default:
                                T_E("Actual config tx tag this, current "
                                    "config Tx Tag error");
                                return VTSS_RC_ERROR;
                        }
                        break;

                    case VLAN_TX_TAG_TYPE_TAG_ALL:
                        switch (temp_conf.tx_tag_type) {
                            case VLAN_TX_TAG_TYPE_UNTAG_THIS:
                            case VLAN_TX_TAG_TYPE_UNTAG_ALL:
                                /* A previous module wants all VIDs to be tagged
                                 * , but this module wants a specific VID or all 
                                 * vids to be untagged. Impossible. 
                                 */
                                vlan_conflicts->port_flags = 
                                    vlan_conflicts->port_flags | 
                                    VLAN_PORT_FLAGS_TX_TAG_TYPE;
                                vlan_conflicts->
                                    vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX] |= 
                                    (1 << (u8)user);
                                break;

                            case VLAN_TX_TAG_TYPE_TAG_THIS:
                                /* A previous module wants all VIDs to be tagged
                                 * , and we want to tag a specific. No problem.
                                 */
                                break;

                            case VLAN_TX_TAG_TYPE_TAG_ALL:
                                /* Both a previous module and we want to tag all
                                 * frames. No problem. 
                                 */
                                break;

                            default:
                                T_E("Actual config tx tag all, current config "
                                    "Tx Tag error");
                                return VTSS_RC_ERROR;
                        }
                        break;

                    default:
                        T_E("actual config tx tag error");
                        return VTSS_RC_ERROR;
                }
            } else { /* if (actual_conf->flags & VLAN_PORT_FLAGS_TX_TAG_TYPE) */
                // We're the first user module with Tx tagging requirements
                actual_conf->tx_tag_type    =  temp_conf.tx_tag_type;
                actual_conf->untagged_vid   =  temp_conf.untagged_vid;
                actual_conf->flags          |= VLAN_PORT_FLAGS_TX_TAG_TYPE;
                if (temp_conf.tx_tag_type == VLAN_TX_TAG_TYPE_TAG_THIS) {
                    at_least_one_module_wants_to_send_a_specific_vid_tagged = 
                                                                          TRUE;
                    /* Also, if it happens that the tagged VID is equal to the
                     * resolved PVID, we must send the pvid tagged.
                     * Here we *must* have the pvid resoved. Hence, the double 
                     * for-loop.
                     */
                    if (temp_conf.untagged_vid == actual_conf->pvid) {
                        *tag_pvid = TRUE;
                    }
                } /* if (temp_conf.tx_tag_type == VLAN_TX_TAG_TYPE_TAG_THIS)  */
            } /* if (actual_conf->flags & VLAN_PORT_FLAGS_TX_TAG_TYPE)  */
        } /* if (temp_conf.flags & VLAN_PORT_FLAGS_TX_TAG_TYPE)  */
    } /* for (user... */
    return VTSS_RC_OK;
}
#endif

/* This function can be called by VLAN port configuration function to either 
 * configure port configuration or fetch the conflicts. 
 */
static vtss_rc vlan_port_conflict_resolver(vtss_isid_t isid, 
        vtss_port_no_t port,  vlan_port_conflicts_t *vlan_conflicts, 
        vtss_vlan_port_conf_t *api_conf)
{
    // Default to the static user's configuration
    vlan_port_conf_t new_conf;
#ifdef  VOLATILE_VLAN_USER_PRESENT
    vlan_user_t user;
    vlan_port_conf_t temp_conf;
#endif
    // This one is only valid if it turns out that a module wants to tag a 
    // specific VID, which happens to be the PVID.
    BOOL tag_pvid = FALSE;

    //Critical section should have been taken by now.
    VLAN_CRIT_ASSERT_LOCKED();

    new_conf = vlan.vlan_port_conf[VLAN_USER_STATIC]
               [isid - VTSS_ISID_START][port - VTSS_PORT_NO_START];
    // All the static configuration can be overridden:
    new_conf.flags = 0;

#ifdef  VOLATILE_VLAN_USER_PRESENT
    /* This code is assuming that VLAN_USER_STATIC is the very first in the 
     * vlan_user_t enum. 
     */
    for (user = (VLAN_USER_STATIC + 1); user < VLAN_USER_ALL; user++) {
        temp_conf = 
        vlan.
        vlan_port_conf[user][isid - VTSS_ISID_START][port - VTSS_PORT_NO_START];

        // Check for aware conflicts.
        if (vlan_check_conflict (&new_conf, &temp_conf, 
                                 VLAN_PORT_FLAGS_AWARE, 
                                 user, vlan_conflicts) != VTSS_RC_OK) {
            T_D("Error in checking the vlan aware conflict");
            return VTSS_RC_ERROR;
        }
        // Check for ingress_filter conflicts.
        if (vlan_check_conflict (&new_conf, &temp_conf, 
                                 VLAN_PORT_FLAGS_INGR_FILT, 
                                 user, vlan_conflicts) != VTSS_RC_OK) {
            T_D("Error in checking the ingress filter conflict");
            return VTSS_RC_ERROR;
        }
        // Check for pvid conflicts.
        if (vlan_check_conflict (&new_conf, &temp_conf, 
                                 VLAN_PORT_FLAGS_PVID,
                                 user, vlan_conflicts) != VTSS_RC_OK) {
            T_D("Error in checking the pvid conflict");
            return VTSS_RC_ERROR;
        }
        // Check for ingress frame_type conflicts.
        if (vlan_check_conflict (&new_conf, &temp_conf, 
                                 VLAN_PORT_FLAGS_RX_TAG_TYPE,
                                 user, vlan_conflicts) != VTSS_RC_OK) {
            T_D("Error in checking the RX TAG conflict");
            return VTSS_RC_ERROR;
        }
    } /* for (user... */

    if (vlan_check_uvid_conflicts (isid, port, &new_conf, 
                                   &tag_pvid, vlan_conflicts) != VTSS_RC_OK) {
        T_D("Error in checking uvid conflict");
        return VTSS_RC_ERROR;
    }
#endif

    /* The final port configuration is now held in new_conf. Check to see if 
     * this is the same as what we've already applied. If not, send it to the 
     * relevant switch. Remember that VLAN_USER_CNT index is used for actual 
     * configuration. 
     */
    if (memcmp(&new_conf, &vlan.vlan_port_conf[VLAN_USER_ALL] 
                [isid - VTSS_ISID_START]
                [port - VTSS_PORT_NO_START], sizeof(new_conf)) != 0) {

        /* We need to convert the vlan_port_conf_t structure to a structure 
         * accepted by the API. It contains fields we don't care about.
         */
        memset(api_conf, 0, sizeof(vtss_vlan_port_conf_t)); 
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
        api_conf->aware          = new_conf.port_type;
#endif /* VTSS_FEATURE_VLAN_PORT_V1 */
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        api_conf->port_type      = new_conf.port_type;
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */
        api_conf->pvid           = new_conf.pvid;
        api_conf->frame_type     = new_conf.frame_type;
        api_conf->ingress_filter = new_conf.ingress_filter;
        switch (new_conf.tx_tag_type) {
            case VLAN_TX_TAG_TYPE_UNTAG_THIS:
                /* In cases where only one VID should be untagged, all other 
                 * VIDs will be tagged and .uvid will in those cases contain 
                 * a valid VID. Also, it might be that at least one user module 
                 * wants a specific VID tagged, but that is already catered 
                 * for in the conflict resolver above. 
                 */
                api_conf->untagged_vid = new_conf.untagged_vid;
                break;

            case VLAN_TX_TAG_TYPE_UNTAG_ALL:
                /* The .uvid member is VTSS_VID_ALL if more than one user module
                 * wants to untag different VIDs or a single user wants to untag 
                 * all, which is exactly what the API needs in order to untag all.
                 */
                api_conf->untagged_vid = VTSS_VID_ALL;
                new_conf.untagged_vid = VTSS_VID_ALL;
                break;

            case VLAN_TX_TAG_TYPE_TAG_THIS:
                /* This is the least prioritized tx-tag-type. If any of 
                 * the user modules want to untag a specific VID or wants 
                 * to tag all VIDs, we cannot end up here. The default is 
                 * to tag everything, but the PVID. However, if at least 
                 * one user module asked to tag the PVID, we must do it. 
                 * Whether to tag the PVID or not is held in the tag_pvid 
                 * boolean. 
                 */
                api_conf->untagged_vid = tag_pvid ? 
                              VTSS_VID_NULL : new_conf.pvid;
                break;

            case VLAN_TX_TAG_TYPE_TAG_ALL:
                // Ask the API to tag all VIDs.
                api_conf->untagged_vid = VTSS_VID_NULL;
                new_conf.untagged_vid = VTSS_VID_NULL;
                break;
        } /* switch (new_conf.tx_tag_type) */

        //Actual port configuration should always have all the port flags set.
        new_conf.flags = VLAN_PORT_FLAGS_ALL;
   
        // Save the new conf in the actual conf.
        vlan.vlan_port_conf[VLAN_USER_ALL]
             [isid - VTSS_ISID_START][port - VTSS_PORT_NO_START]
        = new_conf;
    } /* if (memcmp(&new_conf,   */
    return VTSS_OK;
}

char *vlan_mgmt_vlan_user_to_txt(vlan_user_t usr)
{
    switch (usr) {
        case VLAN_USER_STATIC:
            return "Static";
#ifdef VTSS_SW_OPTION_DOT1X
        case VLAN_USER_DOT1X:
            return "NAS";
#endif
#ifdef VTSS_SW_OPTION_MSTP
        case VLAN_USER_MSTP:
            return "MSTP";
#endif




#ifdef VTSS_SW_OPTION_MVR
        case VLAN_USER_MVR:
            return "MVR";
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        case VLAN_USER_VOICE_VLAN:
            return "Voice VLAN";
#endif
#ifdef VTSS_SW_OPTION_ERPS
        case VLAN_USER_ERPS:
            return "ERPS";
#endif
#ifdef VTSS_SW_OPTION_MEP
        case VLAN_USER_MEP:
            return "MEP";
#endif
#ifdef VTSS_SW_OPTION_EVC
        case VLAN_USER_EVC:
            return "EVC";
#endif
#ifdef VTSS_SW_OPTION_VCL
        case VLAN_USER_VCL:
            return "VCL";
#endif
#ifdef VTSS_SW_OPTION_RSPAN
        case VLAN_USER_RSPAN:
            return "RSPAN";
#endif
        case VLAN_USER_ALL:
            return "Combined";
        case VLAN_USER_CNT:
            return "";
    }
    return "";
}

#if defined(VOLATILE_VLAN_USER_PRESENT) && defined(VTSS_SW_OPTION_SYSLOG)
/*  This function compares the previous and present conflicts and 
 *  logs the conflicts to RAM based syslog, if conflicts exist
 *  Input parms - prev_conflicts and current_conflicts
 *  Output parms - none
 */
static void vlan_log_conflict(vlan_port_conflicts_t prev_conflicts, 
        vlan_port_conflicts_t current_conflicts)
{
    vlan_user_t usr;

    //This condition checks if there are any PVID conflicts.
    if (current_conflicts.port_flags & VLAN_PORT_FLAGS_PVID) {
        /* This will get all the conflicting VLAN Users. Each bit in the
         * vlan_users field indicates a VLAN User.
         */
        for (usr = (VLAN_USER_STATIC + 1); usr < VLAN_USER_ALL; usr++) {
            if ((current_conflicts.vlan_users[VLAN_PORT_PVID_IDX] & 
                        (1 << (u8)usr))  && 
                    !(prev_conflicts.vlan_users[VLAN_PORT_PVID_IDX] & 
                        (1 << (u8)usr))) {
                S_E("VLAN Port Configuration PVID Conflict - %s", vlan_mgmt_vlan_user_to_txt(usr));
            }
        }
    }

    //This condition checks if there are any Aware conflicts.
    if (current_conflicts.port_flags & VLAN_PORT_FLAGS_AWARE) {
        /* This will get all the conflicting VLAN Users. Each bit in the
         * vlan_users field indicates a VLAN User.
         */
        for (usr = (VLAN_USER_STATIC + 1); usr < VLAN_USER_ALL; usr++) {
            if ((current_conflicts.vlan_users[VLAN_PORT_PTYPE_IDX] & 
                        (1 << (u8)usr))  &&
                    !(prev_conflicts.vlan_users[VLAN_PORT_PTYPE_IDX] &
                        (1 << (u8)usr))) {
                S_E("VLAN Port Configuration Awareness Conflict - %s",
                        vlan_mgmt_vlan_user_to_txt(usr));
            }
        }
    }

    //This condition checks if there are any Ingr Filter conflicts.
    if (current_conflicts.port_flags & VLAN_PORT_FLAGS_INGR_FILT) {
        /* This will get all the conflicting VLAN Users. Each bit in the
         * vlan_users field indicates a VLAN User.
         */
        for (usr = (VLAN_USER_STATIC + 1); usr < VLAN_USER_ALL; usr++) {
            if ((current_conflicts.vlan_users[VLAN_PORT_INGR_FILT_IDX] & 
                        (1 << (u8)usr))  &&
                    !(prev_conflicts.vlan_users[VLAN_PORT_INGR_FILT_IDX] &
                        (1 << (u8)usr))) {
                S_E("VLAN Port Configuration Ingress Filter Conflict - %s",
                        vlan_mgmt_vlan_user_to_txt(usr));
            }
        }
    }

    //This condition checks if there are any Frame Type conflicts.
    if (current_conflicts.port_flags & VLAN_PORT_FLAGS_RX_TAG_TYPE) {
        /* This will get all the conflicting VLAN Users. Each bit in the
         * vlan_users field indicates a VLAN User.
         */
        for (usr = (VLAN_USER_STATIC + 1); usr < VLAN_USER_ALL; usr++) {
            if ((current_conflicts.vlan_users[VLAN_PORT_RX_TAG_TYPE_IDX] & 
                        (1 << (u8)usr))  &&
                    !(prev_conflicts.vlan_users[VLAN_PORT_RX_TAG_TYPE_IDX] &
                        (1 << (u8)usr))) {
                S_E("VLAN Port Configuration Frame Type Conflict - %s",
                        vlan_mgmt_vlan_user_to_txt(usr));
            }
        }
    }

    //This condition checks if there are any UVID conflicts.
    if (current_conflicts.port_flags & VLAN_PORT_FLAGS_TX_TAG_TYPE) {
        /* This will get all the conflicting VLAN Users. Each bit in the
         * vlan_users field indicates a VLAN User.
         */
        for (usr = (VLAN_USER_STATIC + 1); usr < VLAN_USER_ALL; usr++) {
            if ((current_conflicts.vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX] & 
                        (1 << (u8)usr))  &&
                    !(prev_conflicts.vlan_users[VLAN_PORT_TX_TAG_TYPE_IDX] &
                        (1 << (u8)usr))) {
                S_E("VLAN Port Configuration UVID Conflict - %s",
                        vlan_mgmt_vlan_user_to_txt(usr));
            }
        }
    }
}
#endif  /* VOLATILE_VLAN_USER_PRESENT */

/*****************************************************************/
/* Description:Management VLAN port get function                 */
/* - Only accessing switch module linked list                    */
/*                                                               */
/* Input: isid, port to access, port configuration and VLAN User */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
vtss_rc vlan_mgmt_port_conf_get(vtss_isid_t         isid, 
                                vtss_port_no_t      port_no,
                                vlan_port_conf_t    *conf, 
                                vlan_user_t         vlan_user)
{
    T_D("enter, port_no: %d", port_no);
    if (port_no >= VTSS_PORT_ARRAY_SIZE) {
        T_D("Invalid port\n");
        return VLAN_ERROR_PARM;
    }
    /*
     * If the unit is slave, isid must always be VTSS_ISID_LOCAL.
     */
    if ((!msg_switch_is_master()) && (isid != VTSS_ISID_LOCAL)) {
        T_E("vlan_mgmt_port_conf_get can only be called with isid equal to VTSS_ISID_LOCAL on slave units\n");
        return VLAN_ERROR_PARM;
    }
    if (vlan_mgmt_port_sid_invalid(isid, port_no, 0) || (isid == VTSS_ISID_GLOBAL)) {
        T_D("Invalid ISID or port\n");
        return VLAN_ERROR_PARM;
    }
    /* 
     * Slaves do not store VLAN port configuration for each VLAN User. 
     * Hence, combined information can only be fetched on the slaves from 
     * the VTSS API.
     */
    if (isid == VTSS_ISID_LOCAL) {
        if (vlan_user != VLAN_USER_ALL) {
            T_E("VLAN User should always be VLAN_USER_ALL when isid is VTSS_ISID_LOCAL\n");
            return VLAN_ERROR_PARM;
        }
        if (vlan_port_conf_get(port_no, conf) != VTSS_OK) {
            return VTSS_RC_ERROR;
        }
        return VTSS_OK;
    }
    VLAN_CRIT_ENTER();
    *conf = vlan.vlan_port_conf[vlan_user][isid - VTSS_ISID_START][port_no];
    VLAN_CRIT_EXIT();
    T_D("exit");

    return VTSS_OK;
}

/* VLAN set function */
/*****************************************************************/
/* Description:Management VLAN port set function                 */
/* - Both configuration update and chip update through message   */
/* - module and switch API                                       */
/* Input: isid, port number, port struct and VLAN User           */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
vtss_rc vlan_mgmt_port_conf_set(vtss_isid_t isid, vtss_port_no_t port_no, vlan_port_conf_t *conf, vlan_user_t vlan_user)
{
    vtss_rc                 rc = VTSS_OK;
    vlan_port_blk_t         *port_blk;
    vlan_port_conf_t        *port_conf;
    i32                     changed = 0;
    conf_blk_id_t           blk_id;
    vlan_port_conflicts_t   prev_conflicts, current_conflicts;
    vtss_vlan_port_conf_t   api_conf;

    //Check for NULL pointer.
    if(!conf) {
        T_D("vlan_mgmt_port_conf_set: conf is NULL");
        return VLAN_ERROR_PARM;
    }

    T_D("enter, isid: %d, port_no: %d, pvid %d, untagged_vid: %d, "
        "frame_type: %d, ingress_flt: %d, tx_tag_type: %d,  port_type: %d, vlan_user: %d",
        isid,
        port_no,
        conf->pvid,
        conf->untagged_vid,
        conf->frame_type,
        conf->ingress_filter,
        conf->tx_tag_type,
        conf->port_type,
        vlan_user);
    
    if (port_no >= VTSS_PORT_ARRAY_SIZE) {
        T_D("Invalid port\n");
        return VLAN_ERROR_PARM;
    }

    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("vlan_mgmt_port_conf_set :Invalid isid or port number "
            "isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }

    if (msg_switch_exists(isid)) {
        /* Setting uvid to pvid will not impact the static user as static user can only configure
           uvid to be tag_all, untag_all or untag_pvid. For tag_all or untag_all, uvid will be
           changed appropriately not bothering about the uvid present in the conf structure.
         */
        VLAN_CRIT_ENTER();

        if ((conf->flags & VLAN_PORT_FLAGS_PVID) && (vlan_user == VLAN_USER_STATIC)) {
            conf->untagged_vid = conf->pvid;
        }
        /* To fix Bugzilla#8502, pvid = none is treated as pvid = 1  and uvid = 0 (tag_all) */
        if ((conf->flags & VLAN_PORT_FLAGS_PVID) && (conf->pvid == 0) && (vlan_user == VLAN_USER_STATIC)) {
            conf->pvid = VTSS_VID_DEFAULT;
        }
        if (conf->flags & VLAN_PORT_FLAGS_TX_TAG_TYPE) {
            if (conf->tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_ALL) {
                conf->untagged_vid = VTSS_VID_ALL;
            } else if (conf->tx_tag_type == VLAN_TX_TAG_TYPE_TAG_ALL) {
                conf->untagged_vid = VTSS_VID_NULL;
            }
        }
        //Get the current VLAN Port configuration.
        port_conf = &vlan.vlan_port_conf[vlan_user][isid - VTSS_ISID_START][port_no];

        changed = vlan_port_conf_changed(port_conf, conf);
        if (!changed) {
            T_D("Previous VLAN Port configuration is same as the current configuration");
            VLAN_CRIT_EXIT();
            return rc;
        }

        memset(&prev_conflicts, 0, sizeof(vlan_port_conflicts_t));
        memset(&current_conflicts, 0, sizeof(vlan_port_conflicts_t));
        memset(&api_conf, 0, sizeof(vtss_vlan_port_conf_t));

        /* This call will get all port conflicts before applying port
         * configuration.
         */
        if (vlan_port_conflict_resolver(isid, port_no, &prev_conflicts, &api_conf) != VTSS_OK) {
            VLAN_CRIT_EXIT();
            return VTSS_RC_ERROR;
        }
       
        memcpy(port_conf, conf, sizeof(vlan_port_conf_t));

        // This call will get port conflicts after port configuration is stored.
        if (vlan_port_conflict_resolver(isid, port_no, &current_conflicts, &api_conf) != VTSS_OK) {
            VLAN_CRIT_EXIT();
            return VTSS_RC_ERROR;
        }
#if defined(VOLATILE_VLAN_USER_PRESENT) && defined(VTSS_SW_OPTION_SYSLOG)
        //This function will dump the current conflicts to the RAM based syslog.
        vlan_log_conflict(prev_conflicts, current_conflicts);
#endif
        VLAN_CRIT_EXIT();
    } else {
        T_D("isid %d not active", isid);
        return VLAN_ERROR_STACK_STATE;
    }

    //Store the Static configuration in the Flash.
    if (changed && (vlan_user == VLAN_USER_STATIC)) {
        /* Save changed configuration */
        blk_id = CONF_BLK_VLAN_PORT_TABLE;
        if ((port_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, NULL)) == NULL) {
            T_W("failed to open VLAN port table");
        } else {
            port_blk->conf[isid - VTSS_ISID_START][port_no] = *conf;
            conf_sec_close(CONF_SEC_GLOBAL, blk_id);
        }
    }

    // Pass on this port configuration in the stack.
    if (vlan_single_stack_port_conf_set(isid, port_no, &api_conf) != VTSS_OK) {
        return VTSS_RC_ERROR;
    }

    T_D("exit, isid: %d, port_no: %d", isid, port_no);
    return rc;
}

vtss_rc vlan_mgmt_native_vlan_tagging_set(vtss_isid_t isid, vtss_port_no_t port_no, BOOL enable)
{

    vlan_port_mode_t  mode;
    vlan_port_conf_t  vport_conf;
    
    VLAN_CRIT_ENTER();
    if (vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].tag_native_vlan == enable) {
        VLAN_CRIT_EXIT();
        return VTSS_RC_OK;
    }
    vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].tag_native_vlan = enable;
    mode = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode;
    VLAN_CRIT_EXIT();

    if (mode == VLAN_PORT_MODE_TRUNK) {
        if ((vlan_mgmt_port_conf_get(isid, port_no, &vport_conf, VLAN_USER_STATIC)) == VTSS_OK) { 
            if (enable) {
                vport_conf.tx_tag_type = VLAN_TX_TAG_TYPE_TAG_ALL;
                vport_conf.frame_type = VTSS_VLAN_FRAME_TAGGED;
            } else {
                vport_conf.tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_THIS;
                vport_conf.frame_type = VTSS_VLAN_FRAME_ALL;
            }
            vport_conf.flags = VLAN_PORT_FLAGS_ALL;
            if((vlan_mgmt_port_conf_set(isid, port_no, &vport_conf, VLAN_USER_STATIC)) != VTSS_OK) {
                T_N("vlan_mgmt_port_conf_set failed");
            } /* if((vlan_mgmt_port_conf_set(isid, port_no, &vport_conf, VLAN_USER_STATIC)) != VTSS_OK) */
        } /* if ((vlan_mgmt_port_conf_get(isid, port_no, &vport_conf, VLAN_USER_STATIC)) == VTSS_OK) */
    } /* if (mode == VLAN_PORT_MODE_TRUNK) */

    return VTSS_RC_OK;
}

vtss_rc vlan_mgmt_native_vlan_tagging_get(vtss_isid_t isid, vtss_port_no_t port_no, BOOL *tagging_enabled)
{
    VLAN_CRIT_ENTER();
    *tagging_enabled = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].tag_native_vlan;
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}
vtss_rc vlan_mgmt_port_mode_set(vtss_isid_t isid, vtss_port_no_t port_no, vlan_port_mode_conf_t *pmode)
{
    vlan_port_conf_t        vport_conf, temp_conf;
    vlan_port_mode_t        prev_mode;
    vlan_mgmt_entry_t       conf;
    vtss_vid_t              vid, native_vid = 1, access_vid;
    BOOL                    add_reqd, tag_native_vlan;
    u8                      vlan_bit_mask[VLAN_BIT_MASK_LEN]; 
    vlan_member_port_type_t empty_ports[VTSS_PORT_ARRAY_SIZE];

    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    memset(empty_ports, 0, sizeof(empty_ports));
    memset(vlan_bit_mask, 0, sizeof(vlan_bit_mask));
    VLAN_CRIT_ENTER();
    prev_mode = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode;
    vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode = pmode->mode;
    access_vid = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].access_vid;
    tag_native_vlan = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].tag_native_vlan;
    if (pmode->mode == VLAN_PORT_MODE_HYBRID) {
        native_vid = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hybrid_native_vid;
        memcpy(vlan_bit_mask, vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_allowed_bit_mask, sizeof(vlan_bit_mask));
    } else if (pmode->mode == VLAN_PORT_MODE_TRUNK) {
        native_vid = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].native_vid;
        memcpy(vlan_bit_mask, vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].allowed_bit_mask, sizeof(vlan_bit_mask));
    }
    memcpy(&temp_conf, &(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_port_conf), sizeof(temp_conf));
    VLAN_CRIT_EXIT();

    if (prev_mode != pmode->mode) {
        if (pmode->mode == VLAN_PORT_MODE_TRUNK) {
            /* aware(C-port), ingress_filter(enable), frame_type(all) */
            if (tag_native_vlan) {
                vport_conf.tx_tag_type = VLAN_TX_TAG_TYPE_TAG_ALL;
                vport_conf.frame_type = VTSS_VLAN_FRAME_TAGGED;
            } else {
                vport_conf.tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_THIS;
                vport_conf.frame_type = VTSS_VLAN_FRAME_ALL;
            }
            vport_conf.port_type = VLAN_PORT_TYPE_C;
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
            vport_conf.ingress_filter = TRUE;
#endif
            vport_conf.pvid = native_vid;
            vport_conf.untagged_vid = native_vid;
            vport_conf.flags = VLAN_PORT_FLAGS_ALL;
        } else if (pmode->mode == VLAN_PORT_MODE_ACCESS) {
            /* untag_all, aware, ingress_filter(enable), frame_type (all) */
            vport_conf.tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_ALL;
            vport_conf.port_type = VLAN_PORT_TYPE_C;
#ifdef VTSS_SW_OPTION_VLAN_INGRESS_FILTERING
            vport_conf.ingress_filter = TRUE;
#endif
            vport_conf.frame_type = VTSS_VLAN_FRAME_ALL;
            vport_conf.pvid = access_vid;
            vport_conf.untagged_vid = access_vid;
            vport_conf.flags = VLAN_PORT_FLAGS_ALL;
        } else if (pmode->mode == VLAN_PORT_MODE_HYBRID) {
            memcpy(&vport_conf, &temp_conf, sizeof(temp_conf));
            vport_conf.pvid = native_vid;
            if (vport_conf.tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_THIS) {
                vport_conf.untagged_vid = native_vid;
            }
            vport_conf.flags = VLAN_PORT_FLAGS_ALL;
        } else {
            T_D("Invalid port mode");
            return VTSS_RC_ERROR;
        }
        if((vlan_mgmt_port_conf_set(isid, port_no, &vport_conf, VLAN_USER_STATIC)) != VTSS_OK) {
            T_E("vlan_mgmt_port_conf_set failed");
            return VTSS_RC_ERROR;
        }
        for (vid = VLAN_TAG_MIN_VALID; vid < VLAN_TAG_MAX_VALID; vid++) {
            if ((vlan_mgmt_vlan_get(isid, vid, &conf, 0, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP)) != VTSS_RC_OK) {
                memset(&conf, 0, sizeof(conf));
            }
            conf.vid = vid;
            add_reqd = FALSE;
            if (pmode->mode == VLAN_PORT_MODE_ACCESS) {
                if ((conf.ports[port_no] == 1) && (vid != access_vid)) {
                    add_reqd = TRUE;
                    conf.ports[port_no] = 0;
                }
                /* This port should only be a member of Access VLAN */
                if ((conf.ports[port_no] == 0) && (vid == access_vid)) {
                    add_reqd = TRUE;
                    conf.ports[port_no] = 1;
                }
            } else if ((pmode->mode == VLAN_PORT_MODE_TRUNK) || (pmode->mode == VLAN_PORT_MODE_HYBRID)) {
                if (VLAN_IS_BIT_SET(vlan_bit_mask, vid)) {
                    if (conf.ports[port_no] == 0) {
                        add_reqd = TRUE;
                        conf.ports[port_no] = 1;
                    }
                } else {
                    if (conf.ports[port_no] == 1) {
                        add_reqd = TRUE;
                        conf.ports[port_no] = 0;
                    }
                }
                if ((vid == native_vid) && (conf.ports[port_no] == 0)) {
                    add_reqd = TRUE;
                    conf.ports[port_no] = 1;
                }
            }
            if (add_reqd == TRUE) {
                if (memcmp(conf.ports, empty_ports, sizeof(empty_ports))) {
                    if ((vlan_mgmt_vlan_add(isid, &conf, VLAN_USER_STATIC)) != VTSS_RC_OK) {
                        T_N("failed adding VLAN");
                    }
                } else {
                    if (vlan_mgmt_vlan_del(isid, vid, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
                        T_N("vlan_mgmt_vlan_del(%u): failed", vid);
                    } /* if (vlan_mgmt_vlan_del(isid, vid, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) */
                } /* else of if (memcmp(mconf.ports, empty_ports, sizeof(empty_ports))) */
            } /* if (add_reqd == TRUE) */
        } /* for (vid = VLAN_TAG_MIN_VALID; vid < VLAN_TAG_MAX_VALID; vid++) */
    } /* if (prev_mode != pmode->mode) */

    return VTSS_RC_OK;
}

vtss_rc vlan_mgmt_port_mode_get(vtss_isid_t isid, vtss_port_no_t port_no, vlan_port_mode_conf_t *pmode)
{
    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    VLAN_CRIT_ENTER();
    pmode->mode = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode;
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

vtss_rc vlan_mgmt_port_access_vlan_set(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t vid)
{
    vlan_port_conf_t        conf;
    vlan_port_mode_t        mode;
    vlan_mgmt_entry_t       mconf;
    vtss_vid_t              prev_vid;
    vlan_member_port_type_t empty_ports[VTSS_PORT_ARRAY_SIZE];
    
    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    memset(empty_ports, 0, sizeof(empty_ports));
    VLAN_CRIT_ENTER();
    mode = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode;
    vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].access_vid = vid;
    VLAN_CRIT_EXIT();

    if (mode == VLAN_PORT_MODE_ACCESS) {
        if ((vlan_mgmt_port_conf_get(isid, port_no, &conf, VLAN_USER_STATIC)) == VTSS_OK) {
            conf.flags = VLAN_PORT_FLAGS_ALL;
#ifdef VTSS_SW_OPTION_ACL
            if (mgmt_vlan_pvid_change(isid, port_no, vid, conf.pvid) != VTSS_OK) {
                T_D("mgmt_vlan_pvid_change failed");
            }
#endif /* VTSS_SW_OPTION_ACL */
            prev_vid = conf.pvid;
            conf.pvid = vid;
            if((vlan_mgmt_port_conf_set(isid, port_no, &conf, VLAN_USER_STATIC)) != VTSS_OK) {
                T_D("vlan_mgmt_port_conf_set failed");
                return VTSS_RC_ERROR;
            }
        } else {
            T_D("vlan_mgmt_port_conf_get failed");
            return VTSS_RC_ERROR;
        }
        /* Add VLAN membership of the port in native VLAN */
        if (vlan_mgmt_vlan_get(isid, vid, &mconf, 0, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
            memset(&mconf, 0, sizeof(mconf));
            mconf.vid = vid;
        }
        mconf.ports[port_no] = 1;
        if ((vlan_mgmt_vlan_add(isid, &mconf, VLAN_USER_STATIC)) != VTSS_RC_OK) {
            T_N("failed adding VLAN");
        }
        memset(&mconf, 0, sizeof(mconf));
        if (vlan_mgmt_vlan_get(isid, prev_vid, &mconf, 0, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) == VTSS_OK) {
            mconf.vid = prev_vid;
            mconf.ports[port_no] = 0;
            if (memcmp(mconf.ports, empty_ports, sizeof(empty_ports))) {
                if ((vlan_mgmt_vlan_add(isid, &mconf, VLAN_USER_STATIC)) != VTSS_RC_OK) {
                    T_N("failed adding VLAN");
                } /* if ((vlan_mgmt_vlan_add(isid, &mconf, VLAN_USER_STATIC)) != VTSS_RC_OK) */
            } else {
                if (vlan_mgmt_vlan_del(isid, prev_vid, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
                    T_N("vlan_mgmt_vlan_del(%u): failed", prev_vid);
                }
            }
        } /* if (vlan_mgmt_vlan_get(isid, prev_vid, &mconf, 0, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) == VTSS_OK) */
    } /* if (mode == VLAN_PORT_MODE_ACCESS) */

    return VTSS_RC_OK;
}

vtss_rc vlan_mgmt_port_access_vlan_get(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t *vid)
{
    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    VLAN_CRIT_ENTER();
    *vid = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].access_vid;
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

/* port_mode is the mode of the native vlan command */
vtss_rc vlan_mgmt_port_native_vlan_set(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t vid, vlan_port_mode_t port_mode)
{
    vlan_port_conf_t        conf;
    vlan_port_mode_t        mode;
    vlan_mgmt_entry_t       mconf;
    vtss_vid_t              prev_vid;
    vlan_member_port_type_t empty_ports[VTSS_PORT_ARRAY_SIZE];
    u8                      allowed_bit_mask[VLAN_BIT_MASK_LEN];

    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    memset(empty_ports, 0, sizeof(empty_ports));
    VLAN_CRIT_ENTER();
    mode = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode;
    if (port_mode == VLAN_PORT_MODE_TRUNK) {
        vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].native_vid = vid;
        memcpy(allowed_bit_mask, vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].allowed_bit_mask, sizeof(allowed_bit_mask));
    } else { /* Hybrid port */
        vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hybrid_native_vid = vid;
        memcpy(allowed_bit_mask, vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_allowed_bit_mask, sizeof(allowed_bit_mask));
    }
    VLAN_CRIT_EXIT();

    if (((mode == VLAN_PORT_MODE_TRUNK) && (port_mode == VLAN_PORT_MODE_TRUNK)) || 
        ((mode == VLAN_PORT_MODE_HYBRID) && (port_mode == VLAN_PORT_MODE_HYBRID))) {
        if ((vlan_mgmt_port_conf_get(isid, port_no, &conf, VLAN_USER_STATIC)) == VTSS_OK) {
            conf.flags = VLAN_PORT_FLAGS_ALL;
#ifdef VTSS_SW_OPTION_ACL
            if (mgmt_vlan_pvid_change(isid, port_no, vid, conf.pvid) != VTSS_OK) {
                T_D("mgmt_vlan_pvid_change failed");;
            }
#endif /* VTSS_SW_OPTION_ACL */
            prev_vid = conf.pvid;
            conf.pvid = vid;
            if((vlan_mgmt_port_conf_set(isid, port_no, &conf, VLAN_USER_STATIC)) != VTSS_OK) {
                T_D("vlan_mgmt_port_conf_set failed");
                return VTSS_RC_ERROR;
            }
        } else {
            T_D("vlan_mgmt_port_conf_get failed");
            return VTSS_RC_ERROR;
        }
        /* Add VLAN membership of the port in native VLAN */
        if (vlan_mgmt_vlan_get(isid, vid, &mconf, 0, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
            memset(&mconf, 0, sizeof(mconf));
            mconf.vid = vid;
        }
        mconf.ports[port_no] = 1;
        if ((vlan_mgmt_vlan_add(isid, &mconf, VLAN_USER_STATIC)) != VTSS_RC_OK) {
            T_N("failed adding VLAN");
        }
        if (!VLAN_IS_BIT_SET(allowed_bit_mask, prev_vid)) {
            memset(&mconf, 0, sizeof(mconf));
            if (vlan_mgmt_vlan_get(isid, prev_vid, &mconf, 0, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) == VTSS_OK) {
                mconf.vid = prev_vid;
                mconf.ports[port_no] = 0;
                if (memcmp(mconf.ports, empty_ports, sizeof(empty_ports))) {
                    if ((vlan_mgmt_vlan_add(isid, &mconf, VLAN_USER_STATIC)) != VTSS_RC_OK) {
                        T_N("failed adding VLAN");
                    } /* if ((vlan_mgmt_vlan_add(isid, &mconf, VLAN_USER_STATIC)) != VTSS_RC_OK) */
                } else {
                    if (vlan_mgmt_vlan_del(isid, prev_vid, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
                        T_N("vlan_mgmt_vlan_del(%u): failed", prev_vid);
                    }
                } /* else of if (memcmp(mconf.ports, empty_ports, sizeof(empty_ports))) */
            } /* if (vlan_mgmt_vlan_get(isid, prev_vid, &mconf, 0, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) == VTSS_OK) */
        } /* if (!VLAN_IS_BIT_SET(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].allowed_bit_mask, prev_vid)) */
    } /* if ((mode == VLAN_PORT_MODE_TRUNK) || (mode == VLAN_PORT_MODE_HYBRID)) */

    return VTSS_RC_OK;
}

vtss_rc vlan_mgmt_port_native_vlan_get(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t *vid, vlan_port_mode_t port_mode)
{
    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    VLAN_CRIT_ENTER();
    if (port_mode == VLAN_PORT_MODE_TRUNK) {
        *vid = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].native_vid;
    } else {
        *vid = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hybrid_native_vid;
    }
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

/* vid_mask is a pointer to array of VLAN_BIT_MASK_LEN */
vtss_rc vlan_mgmt_port_allowed_vlans_set(vtss_isid_t isid, vtss_port_no_t port_no, u8 *vid_mask, vlan_port_mode_t port_mode)
{
    vtss_vid_t              vid, native_vid;
    vlan_port_mode_t        mode;
    vlan_mgmt_entry_t       mconf;
    BOOL                    add_reqd;
    vlan_member_port_type_t empty_ports[VTSS_PORT_ARRAY_SIZE];

    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    memset(empty_ports, 0, sizeof(empty_ports));
    VLAN_CRIT_ENTER();
    if (port_mode == VLAN_PORT_MODE_TRUNK) {
        memcpy(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].allowed_bit_mask, vid_mask, VLAN_BIT_MASK_LEN);
        native_vid = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].native_vid;
    } else {
        memcpy(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_allowed_bit_mask, vid_mask, VLAN_BIT_MASK_LEN);
        native_vid = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hybrid_native_vid;
    }
    mode = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode;
    VLAN_CRIT_EXIT();
    if (((mode == VLAN_PORT_MODE_TRUNK) && (port_mode == VLAN_PORT_MODE_TRUNK)) || 
        ((mode == VLAN_PORT_MODE_HYBRID) && (port_mode == VLAN_PORT_MODE_HYBRID))) {
        for (vid = VLAN_TAG_MIN_VALID; vid <= VLAN_TAG_MAX_VALID; vid++) {
            if (vid == native_vid) {
                continue;
            }
            if (vlan_mgmt_vlan_get(isid, vid, &mconf, 0, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
                memset(&mconf, 0, sizeof(mconf));
                mconf.vid = vid;
            }
            add_reqd = FALSE;
            if (VLAN_IS_BIT_SET(vid_mask, vid)) {
                if (mconf.ports[port_no] == 0) {
                    add_reqd = TRUE;
                    mconf.ports[port_no] = 1;
                }
            } else {
                if (mconf.ports[port_no] == 1) {
                    add_reqd = TRUE;
                    mconf.ports[port_no] = 0;
                }
            }
            if (add_reqd == TRUE) {
                if (memcmp(mconf.ports, empty_ports, sizeof(empty_ports))) {
                    if ((vlan_mgmt_vlan_add(isid, &mconf, VLAN_USER_STATIC)) != VTSS_RC_OK) {
                        T_N("failed adding VLAN");
                    } /* if ((vlan_mgmt_vlan_add(isid, &mconf, VLAN_USER_STATIC)) != VTSS_RC_OK) */
                } else {
                    if (vlan_mgmt_vlan_del(isid, vid, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) {
                        T_N("vlan_mgmt_vlan_del(%u): failed", vid);
                    } /* if (vlan_mgmt_vlan_del(isid, vid, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) != VTSS_OK) */
                } /* else of if (memcmp(mconf.ports, empty_ports, sizeof(empty_ports))) */
            } /* if (add_reqd == TRUE) */
        } /* for (vid = VLAN_TAG_MIN_VALID; vid <= VLAN_TAG_MAX_VALID; vid++) */
    } /* if (mode == VLAN_PORT_MODE_TRUNK) */

    return VTSS_RC_OK;
}


vtss_rc vlan_mgmt_port_allowed_vlans_get(vtss_isid_t isid, vtss_port_no_t port_no, u8 *vid_mask, vlan_port_mode_t port_mode)
{
    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    VLAN_CRIT_ENTER();
    if (port_mode == VLAN_PORT_MODE_TRUNK) {
        memcpy(vid_mask, vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].allowed_bit_mask, 
               sizeof(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].allowed_bit_mask));
    } else {
        memcpy(vid_mask, vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_allowed_bit_mask, 
               sizeof(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_allowed_bit_mask));
    }
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

vtss_rc vlan_mgmt_hybrid_port_conf_set(vtss_isid_t isid, vtss_port_no_t port_no, vlan_port_conf_t *pconf)
{
    vlan_port_mode_t mode;

    //Check the validity of port and isid.
    if ((vlan_mgmt_port_sid_invalid(isid, port_no, 1))) {
        T_D("Invalid isid or port number isid = %d, port_no = %d", isid, port_no);
        return VLAN_ERROR_PARM;
    }
    VLAN_CRIT_ENTER();
    memcpy(&(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_port_conf), pconf, sizeof(vlan_port_conf_t));
    mode = vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode;
    VLAN_CRIT_EXIT();

    if (mode == VLAN_PORT_MODE_HYBRID) {
        if ((vlan_mgmt_port_conf_set(isid, port_no, pconf, VLAN_USER_STATIC)) != VTSS_RC_OK) {
            T_D("vlan_mgmt_port_conf_set failed!");
            return VTSS_RC_ERROR;
        }
    } /* if (mode == VLAN_PORT_MODE_HYBRID) */

    return VTSS_RC_OK;
}

vtss_rc vlan_mgmt_hybrid_port_conf_get(vtss_isid_t isid, vtss_port_no_t port_no, vlan_port_conf_t *pconf)
{
    VLAN_CRIT_ENTER();
    memcpy(pconf, &(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_port_conf), sizeof(vlan_port_conf_t));
    VLAN_CRIT_EXIT();

    return VTSS_RC_OK;
}

i8 *vlan_bit_mask_to_txt(u8 *vlan_mask, i8 *buf)
{
    vtss_vid_t      vid, first_vlan = 1;
    i8              *vlan_buf, p[10];
    BOOL            first_vlan_found = FALSE, more_vlans = FALSE;;

    if ((vlan_buf = (i8 *)malloc(VLAN_LIST_BUF_SIZE)) == NULL) {
        T_W("malloc failed");
        memset(buf, 0, VLAN_LIST_BUF_SIZE);
        return buf;
    }
    memset(vlan_buf, 0, sizeof(vlan_buf));
    for (vid = VLAN_TAG_MIN_VALID; vid <= VLAN_TAG_MAX_VALID; vid++) {
        if (VLAN_IS_BIT_SET(vlan_mask, vid)) {
            if (more_vlans == TRUE) {
                sprintf(p, "%c", ',');
                strcat(vlan_buf, p);
                more_vlans = FALSE;
            }
            if (first_vlan_found == FALSE) {
                first_vlan = vid;
                first_vlan_found = TRUE;
            } /* if (first_vlan_found == FALSE) */
        }
        if ((vid == VLAN_TAG_MAX_VALID) || (!VLAN_IS_BIT_SET(vlan_mask, vid))) {
            if (first_vlan_found == TRUE) {
                first_vlan_found = FALSE;
                more_vlans = TRUE;
                sprintf(p, "%u", first_vlan);
                strcat(vlan_buf, p);
                if (vid != (first_vlan + 1)) {
                    sprintf(p, "%c", '-');
                    strcat(vlan_buf, p);
                    sprintf(p, "%u", (vid - 1));
                    strcat(vlan_buf, p);
                } /* if (vid != (first_vlan + 1)) */
            } /* if (first_vlan_found == TRUE) */
        } /* if ((vid == VLAN_TAG_MAX_VALID) || (!VLAN_IS_BIT_SET(vlan_mask, vid))) */
    } /* for (vid = VLAN_TAG_MIN_VALID; vid <= VLAN_TAG_MAX_VALID; vid++) */
    memcpy(buf, vlan_buf, VLAN_LIST_BUF_SIZE);
    free(vlan_buf);

    return buf;
}

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
static vtss_rc vlan_tpid_conf_commit(vtss_etype_t etype)
{
    conf_blk_id_t     blk_id;
    vlan_table_blk_t  *vlan_blk;

    T_D("vlan_tpid_conf_commit enter");
    blk_id = CONF_BLK_VLAN_TABLE;
    if ((vlan_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, NULL)) == NULL) {
        T_W("failed to open VLAN table");
        return VLAN_ERROR_GEN;
    }

    VLAN_CRIT_ENTER();
    vlan_blk->etype = etype;
    VLAN_CRIT_EXIT();
    conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    T_D("exit");

    return VTSS_OK;
}
vtss_rc vlan_mgmt_vlan_tpid_set(vtss_etype_t tpid)
{
    vtss_rc rc = VTSS_RC_OK;
    BOOL    update = FALSE;

    if (tpid < VLAN_TPID_START) {
        T_E("EtherType should always be greater than or equal to 0x0600\n");
        return VTSS_RC_ERROR;
    }
    VLAN_CRIT_ENTER();
    if (tpid != vlan.tpid_s_custom_port) {
        vlan.tpid_s_custom_port = tpid;
        update = TRUE;
    }
    VLAN_CRIT_EXIT();

    if (update) {
        /* Send the configuration on stack */
        rc = vlan_tpid_conf_set(&tpid);
        if (rc == VTSS_RC_OK) {
            /* Save TPID to flash */
            rc = vlan_tpid_conf_commit(tpid);
        }
    }

    return rc;
}
vtss_rc vlan_mgmt_vlan_tpid_get(vtss_etype_t *tpid)
{
    VLAN_CRIT_ENTER();
    *tpid = vlan.tpid_s_custom_port;
    VLAN_CRIT_EXIT();
    return VTSS_RC_OK;
}
vtss_rc vlan_mgmt_debug_vlan_tpid_get(vtss_etype_t *tpid)
{
    vtss_vlan_conf_t conf;
    vtss_rc          rc;
    rc = vtss_vlan_conf_get(NULL, &conf);
    if (rc == VTSS_RC_OK) {
        *tpid = conf.s_etype;
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}
#endif

static inline vtss_vid_t vlan_list_vlan_forbidden_get(vtss_isid_t isid, vtss_vid_t vid,
                                                      vlan_mgmt_entry_t *vlan_mgmt_entry,
                                                      BOOL next, vlan_user_t user, BOOL vid_only)
{
    BOOL                found = FALSE;
    vtss_vid_t          next_vid = 0;
    vtss_isid_t         zero_based_isid = isid - VTSS_ISID_START;
    multi_user_index_t  multi_user_idx;
    vlan_forbidden_list *multi_member_entry = NULL;
    static u8           empty_ports[VTSS_PORT_BF_SIZE]; // Assumed to be reset to all-zeros.
    port_iter_t         pit;
    
    T_D("isid-[%d], vid-[%d], next-[%d], user-[%d], vid_only-[%d]",isid,vid,next,user,vid_only);
    
    if (IF_MULTI_USER_GET_IDX(user, &multi_user_idx)) {
        if (vid == 0 && next) { /* Get First */
            for (next_vid = VTSS_VID_DEFAULT; next_vid < VTSS_VIDS; next_vid++) {
                if (vlan_forbidden_table[next_vid]) {
                    multi_member_entry = &vlan_forbidden_table[next_vid]->fb_port;
                    if (memcmp(multi_member_entry->ports[zero_based_isid],
                     empty_ports, VTSS_PORT_BF_SIZE)) {
                       found = TRUE;
                       break;
                    }
                }
            }
        } else if (vlan_forbidden_table[vid] && !next) {
            next_vid = vid;
            multi_member_entry = &vlan_forbidden_table[next_vid]->fb_port;
            if (memcmp(multi_member_entry->ports[zero_based_isid],
             empty_ports, VTSS_PORT_BF_SIZE)) {
                found = TRUE;
            }
        } else if (next) {
            for (next_vid = vid + 1; next_vid < VTSS_VIDS; next_vid++) {
                if (vlan_forbidden_table[next_vid]) {
                    multi_member_entry =&vlan_forbidden_table[next_vid]->fb_port;
                    if (memcmp(multi_member_entry->ports[zero_based_isid],
                    empty_ports, VTSS_PORT_BF_SIZE)) {
                       found = TRUE;
                       break;
                    }
                }
            }
        }
        if (found && multi_member_entry) {
            vlan_mgmt_entry->vid = next_vid;
            if (!vid_only) {
                (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    vlan_mgmt_entry->ports[pit.iport] = 
                    (BOOL)VTSS_BF_GET(multi_member_entry->ports[zero_based_isid], pit.iport);
                }
            }
        }
    }else {
        T_D("Invalid VLAN User %s", vlan_user_id_txt(user));
    }
    if (!found) {
        return 0;
    }
    
    return vlan_mgmt_entry->vid;
}

/*****************************************************************/
/* Description: VLAN Database get function                       */
/*                                                               */
/* Input: Note vlan members as boolean port list.                */
/*       If next and vid = 0 then will return first VLAN entry   */
/*       If !next and vid  then returns the VLAN entry           */
/*       If next and vid  then will return the next VLAN entry   */
/* Output: Return valid VID if found, 0 if not.                  */
/*****************************************************************/
static inline vtss_vid_t vlan_list_vlan_get(vtss_isid_t isid, vtss_vid_t vid, vlan_mgmt_entry_t *vlan_mgmt_entry, 
                                            BOOL next, vlan_user_t user, BOOL vid_only)
{
    BOOL                            found = FALSE;
    vtss_vid_t                      next_vid = 0;
    vtss_isid_t                     zero_based_isid = isid - VTSS_ISID_START;
    multi_user_index_t              multi_user_idx;
    vlan_multi_membership_entry_t   *multi_member_entry = NULL;
    port_iter_t                     pit;
#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    single_user_index_t             single_user_idx;
    vlan_single_membership_entry_t  *single_member_entry;
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    same_user_index_t               same_user_idx;
    vlan_same_membership_entry_t    *same_member_entry;
#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */
    
    T_D("isid-[%d], vid-[%d], next-[%d], user-[%d], vid_only-[%d]",isid,vid,next,user,vid_only);
    
    if (IF_MULTI_USER_GET_IDX(user, &multi_user_idx)) {
        if (vid == 0 && next) { /* Get First */
            for (next_vid = VTSS_VID_DEFAULT; next_vid < VTSS_VIDS; next_vid++) {
                if (vlan_multi_membership_table[next_vid] && is_vlan_usr_conf(isid, next_vid, user)) {
                    multi_member_entry = &vlan_multi_membership_table[next_vid]->conf;
                    found = TRUE;
                    break;
                }
            }
        } else if (vlan_multi_membership_table[vid] && !next && is_vlan_usr_conf(isid, vid, user)) {
            next_vid = vid;
            multi_member_entry = &vlan_multi_membership_table[next_vid]->conf;
            found = TRUE;
        } else if (next) {
            for (next_vid = vid + 1; next_vid < VTSS_VIDS; next_vid++) {
                if (vlan_multi_membership_table[next_vid] && is_vlan_usr_conf(isid, next_vid, user)) {
                    multi_member_entry =&vlan_multi_membership_table[next_vid]->conf;
                    found = TRUE;
                    break;
                }
            }
        }
        
        if (found && multi_member_entry) {
            vlan_mgmt_entry->vid = next_vid;
            if (!vid_only) {
                (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    vlan_mgmt_entry->ports[pit.iport] = 
                        (BOOL)VTSS_BF_GET(multi_member_entry->ports[multi_user_idx][zero_based_isid], pit.iport);
                }
            }
        }
    }
    
#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    else if (IF_SAME_USER_GET_IDX(user, &same_user_idx)) {
        same_member_entry = &vlan.vlan_same_membership_entries[same_user_idx];
        if (!vid && next) { /* Get First for Same User */
            if (same_member_entry->vid) {
                found = TRUE;
            }
        } else if (vid) {
            if (!next) { /* Get for Same VLAN User */
                if (same_member_entry->vid == vid) {
                    found = TRUE;
                }
            }
            else if (next) {  /* Get Next for Same VLAN User */
                if (same_member_entry->vid > vid) {
                    found = TRUE;
                }
            }
        }
        if (found) {
            vlan_mgmt_entry->vid = same_member_entry->vid;
            if (!vid_only) {
                (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    vlan_mgmt_entry->ports[pit.iport] = 
                    (BOOL)VTSS_BF_GET(same_member_entry->ports[zero_based_isid], pit.iport);
                }
            }
        }
    }
#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT*/

#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    else if (IF_SINGLE_USER_GET_IDX(user, &single_user_idx)) {
        next_vid = 0; // Used to store the found VLAN ID.
        single_member_entry = &vlan.vlan_single_membership_entries[single_user_idx];
        /* Get the First entry */
        if (next) {
            // User wants the next entry relative to @vid, which can be 0 if he
            // wants to start all over.
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                vtss_vid_t looked_up_vid = single_member_entry->vid[zero_based_isid][pit.iport];
                // Find the next VLAN ID closest to @vid.
                if (looked_up_vid != 0 && looked_up_vid > vid && 
                        (next_vid == 0 || (next_vid != 0 && looked_up_vid < next_vid))) {
                    next_vid = looked_up_vid;
                }
            }
            if (next_vid) {
            // Yihaa. @next_vid is now the next VID closest to @vid.
            found = TRUE;
            }
        } else if (vid != 0) {
            // User doesn't want the next, but the one given by @vid (which mustn't be 0)
            next_vid = vid;
            // Now we're at it, fill in the whole vlan_mgmt_entry, so that we don't
            // have to do that again later (in the if (found) branch below).
            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (single_member_entry->vid[zero_based_isid][pit.iport] == next_vid) {
                    vlan_mgmt_entry->ports[pit.iport] = TRUE;
                    found = TRUE;
                    // Do *not* break. Fill in all ports.
                }
            }
        }
        if (found) {
            vlan_mgmt_entry->vid = next_vid;
            if (!vid_only && next) {
                // If @next == FALSE, the whole array is already populated
                (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    if (single_member_entry->vid[zero_based_isid][pit.iport] == next_vid) {
                        vlan_mgmt_entry->ports[pit.iport] = TRUE;
                    }
                }
            }
        }
    }
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
    else {
        T_D("Invalid VLAN User %s", vlan_user_id_txt(user));
    }
    
    if (!found) {
        return 0;
    }
    return vlan_mgmt_entry->vid;
}

/***************************************************************************************/
// function: vlan_forbidden_list_get                   
//                                                               
//  Purpose: Get the forbidden list from data base. 
//    if next is 1 the immediate VLAN id from vid passed by user in forbidden table
//    if next is 0 the funtion returns if vid passed by user exists or not.
// 
// return VTSS_OK or VLAN_ERROR_ENTRY_NOT_FOUND
/**************************************************************************************/

static vtss_rc vlan_forbidden_list_get(vtss_isid_t          isid, 
                                       vtss_vid_t           vid,
                                       vlan_mgmt_entry_t    *vlan_mgmt_entry,
                                       BOOL                 next, 
                                       vlan_user_t          user)
{
    vlan_mgmt_entry_t vlan_mgmt_user_entries;
    vlan_mgmt_entry_t temp_entry;
    vtss_vid_t        next_stack_wide_vid = VTSS_VIDS;
    BOOL              stack_wide = isid == VTSS_ISID_GLOBAL;
    switch_iter_t     sit;
    port_iter_t       pit;

    memset(&vlan_mgmt_user_entries, 0, sizeof(vlan_mgmt_user_entries));
    memset(&temp_entry,                0, sizeof(temp_entry));
    memset(vlan_mgmt_entry,            0, sizeof(vlan_mgmt_entry_t));
    T_D("isid-[%d],vid-[%d],next-[%d]\n,user-[%d] stack_wide-[%d]\n\r",
        isid,vid,next,user,stack_wide);
    (void) switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    VLAN_CRIT_ENTER();
    vtss_vid_t next_stack_wide_user_vid = VTSS_VIDS;
    while (switch_iter_getnext(&sit)) {
        vtss_vid_t next_switch_user_vid = vlan_list_vlan_forbidden_get(sit.isid, vid, &temp_entry, next, user,stack_wide);
        if (next_switch_user_vid != 0 && next_switch_user_vid < next_stack_wide_user_vid) {
            next_stack_wide_user_vid = next_switch_user_vid;
            vlan_mgmt_user_entries = temp_entry;
            T_D("isid-[%d],next_stack_wide_user_vid-[%d]\n,temp_entry.vid-[%d] \n\r",
                sit.isid,next_stack_wide_user_vid,temp_entry.vid);
        }
    }
    // Get the (next) VID for all requested users and among those, find the smallest.
    // Also update the next stack-wide non-user-dependable VID.
    if (next_stack_wide_user_vid != VTSS_VIDS &&
        next_stack_wide_user_vid < next_stack_wide_vid) {
        next_stack_wide_vid = next_stack_wide_user_vid;
    }
    VLAN_CRIT_EXIT();
    if (next_stack_wide_vid == VTSS_VIDS) {
        return VLAN_ERROR_ENTRY_NOT_FOUND;
    }
    vlan_mgmt_entry->vid = next_stack_wide_vid;
    // Populate the vlan_mgmt_entry
    if (!stack_wide) {
        // Only update the membership info if called with isid != VTSS_ISID_GLOBAL,
        // since there ain't bits enough in vlan_mgmt_entry->ports[] to cover the whole
        // stack anyway.
          if (vlan_mgmt_user_entries.vid == next_stack_wide_vid) {
              (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
              while (port_iter_getnext(&pit)) {
                  vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] |= vlan_mgmt_user_entries.ports[pit.iport];
              }
          }
    }

    return VTSS_OK;
}

//************************************************************************
// vlan_get_from_database()
// Retrieve (next) VID and memberships from the state database.
// This is as opposed to vlan_get_grom_hardware(), which reads the hardware
// directly.
//************************************************************************
static vtss_rc vlan_get_from_database(vtss_isid_t           isid, 
                                      vtss_vid_t            vid,
                                      vlan_mgmt_entry_t     *vlan_mgmt_entry,
                                      BOOL                  next, 
                                      vlan_user_t           user)
{
    vlan_user_t       user_ct;
    vlan_user_t       user_ct_start;
    vlan_user_t       user_ct_end;
    vlan_mgmt_entry_t vlan_mgmt_user_entries[VLAN_USER_ALL];
    vlan_mgmt_entry_t temp_entry;
    vtss_vid_t        next_stack_wide_vid = VTSS_VIDS;
    BOOL              stack_wide = isid == VTSS_ISID_GLOBAL;
    switch_iter_t     sit;
    port_iter_t       pit;
    
    memset(&vlan_mgmt_user_entries[0], 0, sizeof(vlan_mgmt_user_entries));
    memset(&temp_entry,                0, sizeof(temp_entry));
    memset(vlan_mgmt_entry,            0, sizeof(vlan_mgmt_entry_t));
    
    T_D("isid-[%d],vid-[%d],next-[%d],user-[%d] stack_wide-[%d]\n\r",isid,vid,next,user,stack_wide);
    if (user == VLAN_USER_ALL) {
        user_ct_start = VLAN_USER_STATIC;
        user_ct_end   = VLAN_USER_ALL;
    } else {
        user_ct_start = user;
        user_ct_end   = user + 1;
    }
    VLAN_CRIT_ENTER();
    // Get the (next) VID for all requested users and among those, find the smallest.
    for (user_ct = user_ct_start; user_ct < user_ct_end; user_ct++) {
        vtss_vid_t next_stack_wide_user_vid = VTSS_VIDS;
        (void) switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
        while (switch_iter_getnext(&sit)) {
            memset(&temp_entry, 0, sizeof(temp_entry));
            vtss_vid_t next_switch_user_vid = vlan_list_vlan_get(sit.isid, vid, &temp_entry, next, user_ct, stack_wide);
            if (next_switch_user_vid != 0 && next_switch_user_vid < next_stack_wide_user_vid) {
                next_stack_wide_user_vid = next_switch_user_vid;
                vlan_mgmt_user_entries[user_ct] = temp_entry;
                T_D("isid-[%d],next_stack_wide_user_vid-[%d],temp_entry.vid-[%d] \n\r",
                    sit.isid,next_stack_wide_user_vid,temp_entry.vid);
            }
        }
        // Also update the next stack-wide non-user-dependable VID.
        if (next_stack_wide_user_vid != VTSS_VIDS && next_stack_wide_user_vid < next_stack_wide_vid) {
            next_stack_wide_vid = next_stack_wide_user_vid;
        }
    }
    VLAN_CRIT_EXIT();
    if (next_stack_wide_vid == VTSS_VIDS) {
        return VLAN_ERROR_ENTRY_NOT_FOUND;
    }
    vlan_mgmt_entry->vid = next_stack_wide_vid;
    // Populate the vlan_mgmt_entry
    if (!stack_wide) {
        // Only update the membership info if called with isid != VTSS_ISID_GLOBAL,
        // since there ain't bits enough in vlan_mgmt_entry->ports[] to cover the whole
        // stack anyway.
        for (user_ct = user_ct_start; user_ct < user_ct_end; user_ct++) {
            if (vlan_mgmt_user_entries[user_ct].vid == next_stack_wide_vid) {
                (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] |= 
                    vlan_mgmt_user_entries[user_ct].ports[pit.iport];
                }
            }
        }
    }
    
    return VTSS_OK;
}

//************************************************************************
// vlan_get_from_hardware()
// Retrieve (next) VID and memberships directly from H/W.
// This is as opposed to vlan_get_from_database(), which works on the
// software database.
//************************************************************************
static vtss_rc vlan_get_from_hardware(vtss_vid_t vid, vlan_mgmt_entry_t *vlan_mgmt_entry, BOOL next)
{
    vtss_rc                 rc = VTSS_RC_ERROR;
    vtss_vid_t              vid_trav, start_vid, end_vid;
    BOOL                    empty_member[VTSS_PORT_ARRAY_SIZE], ports[VTSS_PORT_ARRAY_SIZE];
    BOOL                    found = FALSE;
    vtss_port_no_t          port_no;
#if VTSS_SWITCH_STACKABLE
    BOOL                    member_with_stack_ports_set[VTSS_PORT_ARRAY_SIZE];
    port_isid_info_t        pinfo;
#endif /* VTSS_SWITCH_STACKABLE */

    if ((!vid && !next) || (vlan_mgmt_entry == NULL)) {
        T_D("vlan_get_from_hardware called with vid-[%d], next-[%d] (or) NULL entry is passed",vid,next);
        return VTSS_RC_ERROR;
    }

    memset(vlan_mgmt_entry, 0, sizeof(vlan_mgmt_entry_t));
    start_vid = vid ? ((next) ? (vid + 1) : vid) : VTSS_VID_DEFAULT;
    end_vid = next ?  VTSS_VIDS : (vid + 1);

    memset(empty_member, 0, sizeof(empty_member));
#if VTSS_SWITCH_STACKABLE
    memset(member_with_stack_ports_set, 0, sizeof(empty_member));
    if (port_isid_info_get(VTSS_ISID_LOCAL, &pinfo) == VTSS_RC_OK) {
        member_with_stack_ports_set[pinfo.stack_port_0 - VTSS_PORT_NO_START] = TRUE;
        member_with_stack_ports_set[pinfo.stack_port_1 - VTSS_PORT_NO_START] = TRUE;
    }
#endif /* VTSS_SWITCH_STACKABLE */
    for (vid_trav = start_vid; vid_trav < end_vid; vid_trav++) {
        memset(ports, 0, sizeof(ports));
        rc = vlan_entry_get(vid_trav, ports);
        if (rc != VTSS_OK) {
            T_D("vlan_entry_get returns error-[%d]",rc);
            return VTSS_RC_ERROR;
        }
        if ((memcmp(ports, empty_member, VTSS_PORT_ARRAY_SIZE) == 0) 
#if VTSS_SWITCH_STACKABLE
            || (memcmp(ports, member_with_stack_ports_set, VTSS_PORT_ARRAY_SIZE) == 0)
#endif /* VTSS_SWITCH_STACKABLE */
        ) {
            continue;
        } else {
            found = TRUE;
            vlan_mgmt_entry->vid = vid_trav;
            for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_ARRAY_SIZE; port_no++) {
                vlan_mgmt_entry->ports[port_no] = (ports[port_no]) ? VLAN_MEMBER_PORT : VLAN_NON_MEMBER_PORT;
            }
            break;
        }            
    }
    if (!found) {
        return VLAN_ERROR_ENTRY_NOT_FOUND;
    }
    return VTSS_OK;   
}

#ifdef  VTSS_SW_OPTION_VLAN_NAME
static vtss_rc  vlan_get_vid_from_vlan_name(i8 *vlan_name, vtss_vid_t  *vid) 
{
    u32 temp;

    VLAN_CRIT_ENTER();
    for (temp = 0; temp < VLAN_NAME_MAX_CNT; temp++) {
        if (vlan.vlan_name_conf[temp].name_valid == TRUE) {
            if (!strcmp(vlan_name, vlan.vlan_name_conf[temp].vlan_name)) {
                *vid = vlan.vlan_name_conf[temp].vid;
                VLAN_CRIT_EXIT();
                return  VTSS_RC_OK;
            }
        }
    }
    VLAN_CRIT_EXIT();
    T_D("No matching VLAN Name: %s exists\n", vlan_name);
    return VLAN_ERROR_ENTRY_NOT_FOUND;
}

static vtss_rc vlan_get_vlan_name_from_vid(vtss_vid_t  vid, i8 *vlan_name)
{
    u32 temp;

    T_D("vlan_mgmt_get_vlan_name_from_vid: Enter\n");
    if(vlan_name == NULL) {
        return VLAN_ERROR_PARM;
    }
    if ((vid > VLAN_ID_END) || (vid < VLAN_ID_START)) {
        return VLAN_ERROR_PARM;
    }
    VLAN_CRIT_ENTER();
    for (temp = 0; temp < VLAN_NAME_MAX_CNT; temp++) {
        if (vlan.vlan_name_conf[temp].name_valid == TRUE) {
            if (vid == vlan.vlan_name_conf[temp].vid) {
                strcpy(vlan_name, vlan.vlan_name_conf[temp].vlan_name);
                VLAN_CRIT_EXIT();
                return  VTSS_RC_OK;
            }
        }
    }
    VLAN_CRIT_EXIT();
    return VTSS_RC_OK;
}

#endif

/*****************************************************************/
/* Description:Management VLAN count get function per user       */
/*                                                               */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
vtss_rc vlan_mgmt_vlan_count_get(vtss_isid_t isid, u16 *num_of_vlans, vlan_user_t user)
{
    vlan_mgmt_entry_t vlan_mgmt_entry;
    vtss_vid_t        vid;

    if (!msg_switch_is_master()) {
        T_E("Not master");
        return VLAN_ERROR_STACK_STATE;
    }
    if (!VTSS_ISID_LEGAL(isid) && isid != VTSS_ISID_GLOBAL) {
        T_E("Legal ISID expected");
        return VLAN_ERROR_PARM;
    }
    if (!num_of_vlans) {
        T_E("NULL pointer");
        return VLAN_ERROR_PARM;
    }
    *num_of_vlans = 0;
    vid = 0;
    while ((vlan_mgmt_vlan_get(isid, vid, &vlan_mgmt_entry, 1, user, VLAN_MEMBERSHIP_OP)) == VTSS_RC_OK) {
        vid = vlan_mgmt_entry.vid;
        (*num_of_vlans)++;
    }
    T_I("Number of existing VLANs = %u", *num_of_vlans);
    return VTSS_RC_OK;
}

/*****************************************************************/
/* Description:Management VLAN list get function                 */
/* - Only accessing internal linked lists                        */
/*                                                               */
/* Input:Note vlan members as boolean port list.                 */
/*       If next and vid = 0 then will return first VLAN entry   */
/*       If !next and vid  then returns the VLAN entry           */
/*       If next and vid  then will return the next VLAN entry   */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
vtss_rc vlan_mgmt_vlan_get(vtss_isid_t isid, vtss_vid_t vid, vlan_mgmt_entry_t *vlan_mgmt_entry,
                           BOOL next, vlan_user_t user, vlan_op_type_t oper)
{
    vtss_rc             rc, rc1 = VTSS_RC_ERROR;
    vlan_mgmt_entry_t   forbid_vlan_entry;
    BOOL                members_exist = FALSE, forbid_members_exist = FALSE;
    port_iter_t         pit;

    // Check arguments
    if ((vlan_mgmt_entry == NULL) || (user > VLAN_USER_ALL) || 
        (!VTSS_ISID_LEGAL(isid) && isid != VTSS_ISID_LOCAL && isid != VTSS_ISID_GLOBAL)) {
        return VLAN_ERROR_PARM;
    }
    if ((oper == VLAN_MIXED_OP) && (isid == VTSS_ISID_GLOBAL)) {
        T_D("currently oper == VLAN_MIXED_OP && isid == VTSS_ISID_GLOBAL is not supported");
        return VLAN_ERROR_PARM;
    }
    if (isid == VTSS_ISID_LOCAL) {
        // Read the H/W. In this case, the user must be VLAN_USER_ALL.
        if (user != VLAN_USER_ALL) {
            T_E("When called with VTSS_ISID_LOCAL, the user must be VLAN_USER_ALL and not user=%s", vlan_user_id_txt(user));
            return VLAN_ERROR_PARM;
        }
        rc = vlan_get_from_hardware(vid, vlan_mgmt_entry, next);
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        if (vlan_get_vlan_name_from_vid(vlan_mgmt_entry->vid, vlan_mgmt_entry->vlan_name) != VTSS_RC_OK) {
            T_D("vlan_mgmt_vlan_get: Error while getting VLAN Name from VID\n");
        }
#endif
        return rc;
    } else if (!msg_switch_is_master()) {
        return VLAN_ERROR_STACK_STATE;
    }
    memset(vlan_mgmt_entry, 0, sizeof(vlan_mgmt_entry_t));
    memset(&forbid_vlan_entry, 0, sizeof(vlan_mgmt_entry_t));
    if (oper == VLAN_MEMBERSHIP_OP) {
        if ((rc = vlan_get_from_database(isid, vid, vlan_mgmt_entry, next, user)) != VTSS_RC_OK) {
            T_D("vlan_get_from_database failed to get entry");
            return rc;
        }
        members_exist = TRUE;
    } else if (oper == VLAN_FORBIDDEN_OP) {
        if ((rc = vlan_forbidden_list_get(isid, vid, &forbid_vlan_entry, next, user)) != VTSS_RC_OK) {
            T_D("vlan_forbidden_list_get failed to get entry");
            return rc;
        }
        vlan_mgmt_entry->vid = forbid_vlan_entry.vid;
        forbid_members_exist = TRUE;
    } else if (oper == VLAN_MIXED_OP) {
        rc = vlan_get_from_database(isid, vid, vlan_mgmt_entry, next, user);
        rc1 = vlan_forbidden_list_get(isid, vid, &forbid_vlan_entry, next, user);
        if ((rc == VTSS_RC_OK) && (rc1 == VTSS_RC_OK)) {
            if (vlan_mgmt_entry->vid != forbid_vlan_entry.vid) {
                if (vlan_mgmt_entry->vid < forbid_vlan_entry.vid) { /* Ascending order of traversal */
                    rc1 = VTSS_RC_ERROR; /* No entry with this VID */
                    members_exist = TRUE;
                } else {
                    vlan_mgmt_entry->vid = forbid_vlan_entry.vid; /* This is what is returned to the caller */
                    rc = VTSS_RC_ERROR; /* No entry with this VID */
                    forbid_members_exist = TRUE;
                }
            } else {
                members_exist = TRUE;
                forbid_members_exist = TRUE;
            } /* else of if (vlan_mgmt_entry->vid != forbid_vlan_entry.vid) */
        } else if ((rc != VTSS_RC_OK) && (rc1 == VTSS_RC_OK)) { /* Forbidden VID should be copied to the caller structure */
            vlan_mgmt_entry->vid = forbid_vlan_entry.vid;
            forbid_members_exist = TRUE;
        } else if ((rc == VTSS_RC_OK) && (rc1 != VTSS_RC_OK)) {
            members_exist = TRUE;
        } /* else if ((rc == VTSS_RC_OK) && (rc1 != VTSS_RC_OK)) */
    } else {
        T_E("Invalid oper specified");
        return VLAN_ERROR_PARM;
    }
    if (isid != VTSS_ISID_GLOBAL) { /* If isid == VTSS_ISID_GLOBAL, port list is not valid */
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (members_exist) {
                if (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START]) {
                    vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] = VLAN_MEMBER_PORT;
                } else {
                    vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] = VLAN_NON_MEMBER_PORT;
                }
            } /* if (members_exist) */
            if (forbid_members_exist) {
                if (forbid_vlan_entry.ports[pit.iport + VTSS_PORT_NO_START]) {
                    vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] = VLAN_FORBIDDEN_PORT;
                } else {
                    if (!members_exist) {
                        vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] = VLAN_NON_FORBIDDEN_PORT;
                    }
                }
            } /* if (forbid_members_exist) */
        } /* while (port_iter_getnext(&pit)) */
    } /* if (isid != VTSS_ISID_GLOBAL) */
#ifdef  VTSS_SW_OPTION_VLAN_NAME
    if (vlan_get_vlan_name_from_vid(vlan_mgmt_entry->vid, vlan_mgmt_entry->vlan_name) != VTSS_RC_OK) {
            T_D("vlan_mgmt_vlan_get: Error while getting VLAN Name from VID\n");
    }
#endif
    return ((rc == VTSS_RC_OK) || (rc1 == VTSS_RC_OK)) ? VTSS_RC_OK : rc;
}

#ifdef  VTSS_SW_OPTION_VLAN_NAME
vtss_rc vlan_mgmt_vlan_get_by_name(vtss_isid_t isid, i8 *vlan_name, vlan_mgmt_entry_t *vlan_mgmt_entry, 
                                   BOOL next, vlan_user_t user, vlan_op_type_t oper)
{
    vtss_rc     rc;
    vtss_vid_t  vid;

    if (vlan_name == NULL) {
        T_E("vlan_mgmt_vlan_get_by_name: vlan_name is NULL\n");
        return VLAN_ERROR_PARM;
    }

    if (vlan_mgmt_entry == NULL || user > VLAN_USER_ALL || 
        (!VTSS_ISID_LEGAL(isid) && isid != VTSS_ISID_LOCAL && isid != VTSS_ISID_GLOBAL)) {
      return VLAN_ERROR_PARM;
    }

    T_D("isid: %u, vlan_name: %s user:%s\n", isid, vlan_name, vlan_user_id_txt(user));

    rc = vlan_get_vid_from_vlan_name(vlan_name, &vid);
    if (rc != VTSS_RC_OK) {
        T_D("vlan_mgmt_vlan_get_by_name: Invalid vlan and invalid name\n");
        return  VLAN_ERROR_PARM;
    }
    rc = vlan_mgmt_vlan_get(isid, vid, vlan_mgmt_entry, next, user, oper);
    return rc;
}
#endif

#ifdef  VTSS_SW_OPTION_VLAN_NAME
static vtss_rc vlan_name_conf_commit(void)
{
    conf_blk_id_t           blk_id;
    vlan_name_table_blk_t   *vlan_name_blk;
    u32                     i;

    T_D("vlan_name_conf_commit enter\n");
    blk_id = CONF_BLK_VLAN_NAME_TABLE;
    if ((vlan_name_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, NULL)) == NULL) {
        T_W("failed to open VLAN Name table");
        return VLAN_ERROR_GEN;
    }
    VLAN_CRIT_ENTER();
    vlan_name_blk->count = 0;
    for (i = 0; i < VLAN_NAME_MAX_CNT; i++ ) {
        if (vlan.vlan_name_conf[i].name_valid == TRUE) {
            vlan_name_blk->table[vlan_name_blk->count].vid = vlan.vlan_name_conf[i].vid;
            strcpy(vlan_name_blk->table[vlan_name_blk->count].vlan_name, vlan.vlan_name_conf[i].vlan_name);
            vlan_name_blk->count++;
        }
    }
    VLAN_CRIT_EXIT();
    conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    T_D("vlan_name_conf_commit exit\n");
    return VTSS_OK;
}

vtss_rc vlan_mgmt_vlan_name_add(vlan_mgmt_vlan_name_conf_t *conf, BOOL unique_name)
{
    u16     temp, first_free_entry = VLAN_NAME_MAX_CNT /* initialize first_free_entry with Invalid index */;
    BOOL    first = TRUE, vid_already_exists = FALSE;

    T_D("vlan_mgmt_vlan_name_add: Enter vid = %u, name %s\n", conf->vid, conf->vlan_name);
    if(conf == NULL) {
      return VLAN_ERROR_PARM;
    }
    VLAN_CRIT_ENTER();
    for (temp = 0; temp < VLAN_NAME_MAX_CNT; temp++) {
        /* Check whether VLAN ID is already configured */
        if ((vlan.vlan_name_conf[temp].vid == conf->vid) && (vlan.vlan_name_conf[temp].name_valid == TRUE)) {
            vid_already_exists = TRUE;
            first_free_entry = temp;
        }
        /* Check whether VLAN Name is already configured */
        if (vlan.vlan_name_conf[temp].name_valid == TRUE) {
            if (!unique_name) {
                if (!strcmp(conf->vlan_name, vlan.vlan_name_conf[temp].vlan_name)) {
                    T_D("VLAN Name: %s is already configured for VLAN %u\n", conf->vlan_name, 
                            vlan.vlan_name_conf[temp].vid);
                    VLAN_CRIT_EXIT();
                    return  VLAN_ERROR_VLAN_NAME_PREVIOUSLY_CONFIGURED;
                }
            }
        } else if ((first == TRUE) && (vid_already_exists == FALSE)) { /* Find first free entry */
            first = FALSE;
            first_free_entry = temp;
        }
    }
    if (first_free_entry == VLAN_NAME_MAX_CNT) {
        T_D("VLAN Name table full!!\n");
        VLAN_CRIT_EXIT();
        return  VLAN_ERROR_VLAN_NAME_TABLE_FULL;
    }
    if ((first == FALSE) || (vid_already_exists == TRUE)) {
        vlan.vlan_name_conf[first_free_entry].name_valid = TRUE;
        strcpy(vlan.vlan_name_conf[first_free_entry].vlan_name, conf->vlan_name);
        vlan.vlan_name_conf[first_free_entry].vid = conf->vid;
    }
    VLAN_CRIT_EXIT();
    if (vlan_name_conf_commit() != VTSS_RC_OK) {
        T_E("VLAN Name config commit failure\n");
        return VLAN_ERROR_CONFIG_NOT_OPEN;
    }

    return VTSS_RC_OK;
}

vtss_rc vlan_mgmt_vlan_name_del(i8 *vlan_name) 
{
    u16 temp; 
    vtss_rc rc = VLAN_ERROR_ENTRY_NOT_FOUND;

    VLAN_CRIT_ENTER();
    for (temp = 0; temp < VLAN_NAME_MAX_CNT; temp++) {
        if ((vlan.vlan_name_conf[temp].name_valid == TRUE) && (!strcmp(vlan.vlan_name_conf[temp].vlan_name, vlan_name))) {
            vlan.vlan_name_conf[temp].name_valid = FALSE;
            rc = VTSS_RC_OK;
            break;
        }
    }
    VLAN_CRIT_EXIT();
    return rc;
}

vtss_rc vlan_mgmt_vlan_name_del_by_vid(u16 vid) 
{
    u16 temp; 
    vtss_rc rc = VLAN_ERROR_ENTRY_NOT_FOUND;

    VLAN_CRIT_ENTER();
    for (temp = 0; temp < VLAN_NAME_MAX_CNT; temp++) {
        if ((vlan.vlan_name_conf[temp].name_valid == TRUE) && (vlan.vlan_name_conf[temp].vid == vid)) {
            vlan.vlan_name_conf[temp].name_valid = FALSE;
            rc = VTSS_RC_OK;
            break;
        }
    }
    VLAN_CRIT_EXIT();
    return rc;
}

vtss_rc vlan_mgmt_vlan_name_get(u16 indx, vlan_mgmt_vlan_name_conf_t  *conf)
{
    vtss_rc    rc = VTSS_RC_OK;
    u16        temp;
    BOOL       entry_found = FALSE;
    
    T_D("vlan_mgmt_vlan_name_get:  ENTER\n");
    VLAN_CRIT_ENTER();
    
    if (conf->name_valid == FALSE) {
        /* Check whether VLAN ID is already configured */
        if (vlan.vlan_name_conf[indx].name_valid == TRUE) {
            conf->vid = vlan.vlan_name_conf[indx].vid;
            strcpy(conf->vlan_name, vlan.vlan_name_conf[indx].vlan_name);
        } else {
            rc = VLAN_ERROR_ENTRY_NOT_FOUND;
        }
    } else {
        for (temp = 0; temp < VLAN_NAME_MAX_CNT; temp++) {
            /* Check whether VLAN ID is already configured */
            if (vlan.vlan_name_conf[temp].name_valid == TRUE) {
                if (!strcmp(vlan.vlan_name_conf[temp].vlan_name, conf->vlan_name)) {
                    conf->vid = vlan.vlan_name_conf[temp].vid;
                    entry_found = TRUE;
                    break;
                }
            }
        }
        
        if (entry_found == FALSE) {
            rc = VLAN_ERROR_ENTRY_NOT_FOUND;
        }
    }
    
    VLAN_CRIT_EXIT();
    return rc;
}
#endif

/************************************************************
Function : vlan_forbidden_conf_commit

Purpose : The purpose of this function is to save the forbidden port list in flash.
return : VTSS_RC state.
*************************************************************/

/* Store VLAN configuration */
/* TODO: Improve the logic */
static vtss_rc vlan_forbidden_conf_commit(vtss_vid_t vid, BOOL add)
{
    conf_blk_id_t               blk_id;
    vlan_forbid_table_blk_t     *vlan_blk;
    static u8                   empty_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];
    vtss_vid_t                  vlan_idx = VLAN_ID_NONE;
    vtss_isid_t                 zero_based_isid = VTSS_ISID_START;
    switch_iter_t               sit;
    i32                         first_free_index = -1;
    BOOL                        first = TRUE; 
    
    T_D("vlan_forbidden_conf_commit enter");
    blk_id = CONF_BLK_VLAN_FORBIDDEN;
    if ((vlan_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, NULL)) == NULL) {
        T_W("failed to open VLAN member table");
        return VLAN_ERROR_GEN;
    }
    VLAN_CRIT_ENTER();
    if (vlan_blk->version == 0) {
        vlan_blk->fb_count = 0;
        for (vlan_idx = 0; vlan_idx < ARRSZ(vlan_forbidden_table); vlan_idx++) {
            if (vlan_forbidden_table[vlan_idx]) {
                if (memcmp(vlan_forbidden_table[vlan_idx]->fb_port.ports, empty_ports,sizeof(empty_ports)) != 0) {
                    // The VID is defined, and non-empty for VTSS_USER_STATIC
                    vlan_blk->forbidden[vlan_blk->fb_count].vid = vlan_idx;
                    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                    while (switch_iter_getnext(&sit)) {
                        zero_based_isid = sit.isid - VTSS_ISID_START;
                        memcpy(&(vlan_blk->forbidden[vlan_blk->fb_count].ports[sit.isid][0]),
                               &(vlan_forbidden_table[vlan_idx]->fb_port.ports[zero_based_isid][0]),
                               VTSS_PORT_BF_SIZE);
                    }
                    vlan_blk->fb_count++;
                } /* if (memcmp(vlan_forbidden_table[vlan_idx]->fb_port.ports, empty_ports,sizeof(empty_ports)) != 0) */
            } /* if (vlan_forbidden_table[vlan_idx]) */
        } /* for (vlan_idx = 0; vlan_idx < ARRSZ(vlan_forbidden_table); vlan_idx++) */
    } else { /* Newer version */
        if (add) {
            // The VID is defined for VTSS_USER_STATIC
            for (vlan_idx = 0; vlan_idx < VLAN_MAX; vlan_idx++) {
                if ((first == TRUE) && (vlan_blk->forbidden[vlan_idx].vid == 0)) { /* free entry at vlan_idx */
                    first_free_index = vlan_idx;
                    first = FALSE;
                }
                if (vlan_blk->forbidden[vlan_idx].vid == vid) { /* update the entry */
                    first_free_index = vlan_idx;
                    break;
                }
            }
            if (first_free_index != -1) {
                vlan_blk->forbidden[first_free_index].vid = vid;
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                while (switch_iter_getnext(&sit)) {
                    zero_based_isid = sit.isid - VTSS_ISID_START;
                    memcpy(&(vlan_blk->forbidden[first_free_index].ports[sit.isid][0]),
                           &(vlan_forbidden_table[vid]->fb_port.ports[zero_based_isid][0]),
                           VTSS_PORT_BF_SIZE);
                }
                vlan_blk->fb_count++;
            } else {
                T_W("vlan_forbidden_conf_commit Failed!");
            }
        } else { /* Delete */
            for (vlan_idx = 0; vlan_idx < VLAN_MAX; vlan_idx++) {
                if (vlan_blk->forbidden[vlan_idx].vid == vid) { /* free the entry at vlan_idx */
                    memset(&vlan_blk->forbidden[vlan_idx], 0, sizeof(vlan_blk->forbidden[vlan_idx]));
                    vlan_blk->fb_count--;
                    break;
                }
            }
        }
    } /* if (vlan_blk->version == 0) */
    VLAN_CRIT_EXIT();
    conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    T_D("exit");

    return VTSS_OK;
}

/* Store VLAN configuration */
static vtss_rc vlan_conf_commit(vtss_vid_t vid, BOOL add)
{
    conf_blk_id_t     blk_id;
    vlan_table_blk_t  *vlan_blk;
    vtss_vid_t        vlan_idx = VLAN_ID_NONE;
    vtss_isid_t       zero_based_isid = VTSS_ISID_START;
    switch_iter_t     sit;
    i32               first_free_index = -1;
    BOOL              first = TRUE;

    T_D("vlan_conf_commit enter");
    blk_id = CONF_BLK_VLAN_TABLE;
    if ((vlan_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, NULL)) == NULL) {
        T_W("failed to open VLAN member table");
        return VLAN_ERROR_GEN;
    }
    VLAN_CRIT_ENTER();
    if (vlan_blk->version == 0) {
        vlan_blk->count = 0;
        for (vlan_idx = 0; vlan_idx < ARRSZ(vlan_multi_membership_table); vlan_idx++ ) {
            if (vlan_multi_membership_table[vlan_idx]) {
                // The VID is defined, and non-empty for VTSS_USER_STATIC
                vlan_blk->table[vlan_blk->count].vid = vlan_idx;
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                while (switch_iter_getnext(&sit)) {
                    zero_based_isid = sit.isid - VTSS_ISID_START;
                    memcpy(&(vlan_blk->table[vlan_blk->count].ports[sit.isid][0]), &(vlan_multi_membership_table[vlan_idx]->
                                conf.ports[VLAN_USER_STATIC][zero_based_isid][0]), VTSS_PORT_BF_SIZE);
                }
                vlan_blk->count++;
            }
        }
    } else { /* New version */
        if (add) {
            // The VID is defined for VTSS_USER_STATIC
            for (vlan_idx = 0; vlan_idx < VLAN_MAX; vlan_idx++) {
                if ((first == TRUE) && (vlan_blk->table[vlan_idx].vid == 0)) { /* free entry at vlan_idx */
                    first_free_index = vlan_idx;
                    first = FALSE;
                }
                if (vlan_blk->table[vlan_idx].vid == vid) { /* update the entry */
                    first_free_index = vlan_idx;
                    break;
                }
            }
            if (first_free_index != -1) {
                vlan_blk->table[first_free_index].vid = vid;
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                while (switch_iter_getnext(&sit)) {
                    zero_based_isid = sit.isid - VTSS_ISID_START;
                    memcpy(&(vlan_blk->table[first_free_index].ports[sit.isid][0]), &(vlan_multi_membership_table[vid]->
                                conf.ports[VLAN_USER_STATIC][zero_based_isid][0]), VTSS_PORT_BF_SIZE);
                }
                vlan_blk->count++;
            } else {
                T_W("vlan_conf_commit Failed!");
            }
        } else {
            for (vlan_idx = 0; vlan_idx < VLAN_MAX; vlan_idx++) {
                if (vlan_blk->table[vlan_idx].vid == vid) { /* free the entry at vlan_idx */
                    memset(&vlan_blk->table[vlan_idx], 0, sizeof(vlan_blk->table[vlan_idx]));
                    vlan_blk->count--;
                    break;
                }
            }
        }
    }
    VLAN_CRIT_EXIT();
    conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    T_D("exit");

    return VTSS_OK;
}

void vlan_forbid_change_register(vlan_forbid_change_callback_t cb)
{
    VLAN_CB_CRIT_ENTER();
    VTSS_ASSERT((u32)vlan_forbid_callbacks < ARRSZ(callback_forbid_list));
    callback_forbid_list[vlan_forbid_callbacks++] = cb;
    VLAN_CB_CRIT_EXIT();
}

void vlan_membership_change_register(vlan_membership_change_callback_t cb)
{
    VLAN_CB_CRIT_ENTER();
    VTSS_ASSERT((u32)vlan_membership_callbacks < ARRSZ(callback_membership_list));
    callback_membership_list[vlan_membership_callbacks++] = cb;
    VLAN_CB_CRIT_EXIT();
}

static void vlan_membership_change_callback(vtss_isid_t isid, vtss_vid_t vid, vlan_user_t user, 
                                            u8 added_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                            u8 deleted_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                            u8 current_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                            vlan_action_t action)
{
    vlan_membership_change_callback_t   func;
    i32                                 i;

#if 0
    u32 tisid, port;
    printf("VID = %u, action = %u\n", vid, action);
    for (tisid = 0; tisid < VTSS_ISID_CNT; tisid++) {
        if (msg_switch_exists(tisid + 1)) {
            printf("\n");
            printf("ISID: %lu\n", tisid + 1);
            printf("added_ports:\n");
            for (port = 0; port < VTSS_PORT_NO_END; port++) {
                if (VTSS_BF_GET(added_ports[tisid], port)) {
                    printf("%lu,", (port + 1));
                }
            }
            printf("\n");
            printf("deleted_ports:\n");
            for (port = 0; port < VTSS_PORT_NO_END; port++) {
                if (VTSS_BF_GET(deleted_ports[tisid], port)) {
                    printf("%lu,", (port + 1));
                }
            }
            printf("\n");
            printf("current_ports:\n");
            for (port = 0; port < VTSS_PORT_NO_END; port++) {
                if (VTSS_BF_GET(current_ports[tisid], port)) {
                    printf("%lu,", (port + 1));
                }
            }
            printf("\n");
        }
    }
#endif
    T_D("Enter: vid = %u\n", vid);
    VLAN_CB_CRIT_ENTER();
    for (i = 0; i < vlan_membership_callbacks; i++){
        func = callback_membership_list[i];
        VLAN_CB_CRIT_EXIT();
        func(isid, vid, user, added_ports, deleted_ports, current_ports, action);
        VLAN_CB_CRIT_ENTER();
    }

    VLAN_CB_CRIT_EXIT();
    T_D("Exit: vid = %u\n", vid);
}

static void vlan_forbid_change_callback(vtss_isid_t isid, vtss_vid_t vid, 
                                        u8 added_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                        u8 deleted_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                        u8 current_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE]
                                        )
{
    vlan_forbid_change_callback_t   func;
    i32                             i;

#if 0
    u32 tisid, port;
    printf("VID = %u\n", vid);
    for (tisid = 0; tisid < VTSS_ISID_CNT; tisid++) {
        if (msg_switch_exists(tisid + 1)) {
            printf("\n");
            printf("ISID: %lu\n", tisid + 1);
            printf("added_ports:\n");
            for (port = 0; port < VTSS_PORT_NO_END; port++) {
                if (VTSS_BF_GET(added_ports[tisid], port)) {
                    printf("%lu,", (port + 1));
                }
            }
            printf("\n");
            printf("deleted_ports:\n");
            for (port = 0; port < VTSS_PORT_NO_END; port++) {
                if (VTSS_BF_GET(deleted_ports[tisid], port)) {
                    printf("%lu,", (port + 1));
                }
            }
            printf("\n");
            printf("current_ports:\n");
            for (port = 0; port < VTSS_PORT_NO_END; port++) {
                if (VTSS_BF_GET(current_ports[tisid], port)) {
                    printf("%lu,", (port + 1));
                }
            }
            printf("\n");
        }
    }
#endif

    T_D("Enter: vid = %u\n", vid);

    VLAN_CB_CRIT_ENTER();

    for (i = 0; i < vlan_forbid_callbacks; i++) {
        func = callback_forbid_list[i];
        VLAN_CB_CRIT_EXIT();
        func(isid, vid, added_ports, deleted_ports, current_ports);
        VLAN_CB_CRIT_ENTER();
    }

    VLAN_CB_CRIT_EXIT();
    T_D("Exit: vid = %u\n", vid);
}


/*****************************************************
Function : vlan_mgmt_vlan_registration_get

Purpose: The purpose of this function is to return the ports information 
for a given vid.
port information is like : VLAN included, VLAN not included and forbidden port.

return : VTSS_RC state
*****************************************************/
vtss_rc vlan_mgmt_vlan_registration_get(vtss_isid_t                 isid, 
                                        vtss_vid_t                  vid,
                                        vlan_registration_type_t    *vlan_registration_entry, 
                                        vlan_user_t                 user)
{
    vtss_rc                         rc = VTSS_OK;
    vlan_forbidden_list             *forbidden_entry = NULL;
    vlan_registration_entry_t       vlan_mgmt_entry;
    multi_user_index_t              multi_user_idx;
    vlan_multi_membership_entry_t   *multi_member_entry = NULL;
    port_iter_t                     pit;
        
    if (!msg_switch_is_master()) {
        T_W("Not master");
        return VLAN_ERROR_STACK_STATE;
    }
    
    if (!(VTSS_ISID_LEGAL(isid) || (isid == VTSS_ISID_GLOBAL))) {
        T_E("Invalid ISID (%u). LEGAL expected", isid);
        return VLAN_ERROR_PARM;
    }
    VLAN_CRIT_ENTER();

    isid = isid - VTSS_ISID_START;
    
    if(vlan_multi_membership_table[vid]) {
        if (IF_MULTI_USER_GET_IDX(VLAN_USER_STATIC, &multi_user_idx)) {
            multi_member_entry = &(vlan_multi_membership_table[vid]->conf);
            (void) port_iter_init(&pit, NULL, isid + VTSS_ISID_START, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (isid >= VTSS_ISID_CNT) {
                    break;
                }
                vlan_mgmt_entry.ports[pit.iport] = 
                (BOOL)VTSS_BF_GET(multi_member_entry->ports[multi_user_idx][isid], pit.iport);
                if(vlan_mgmt_entry.ports[pit.iport]) {
                    *(vlan_registration_entry + pit.iport) = VLAN_FIXED; 
                } else {
                    *(vlan_registration_entry + pit.iport) = VLAN_NORMAL;
                }
            }
        }
    }
    
    if(vlan_forbidden_table[vid]) {
        forbidden_entry = &(vlan_forbidden_table[vid]->fb_port);
        (void) port_iter_init(&pit, NULL, isid + VTSS_ISID_START, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (isid >= VTSS_ISID_CNT) {
                break;
            }
            vlan_mgmt_entry.ports[pit.iport] = 
            (BOOL)VTSS_BF_GET(forbidden_entry->ports[isid], pit.iport);
            if(vlan_mgmt_entry.ports[pit.iport]) {
                *(vlan_registration_entry + pit.iport) = VLAN_FORBIDDEN;
            } else if ((*(vlan_registration_entry + pit.iport) != VLAN_FIXED)) {
                *(vlan_registration_entry + pit.iport) = VLAN_NORMAL;
            }
        }
    } 
    
    if((!vlan_multi_membership_table[vid]) && (!vlan_forbidden_table[vid])) {
        (void) port_iter_init(&pit, NULL, isid + VTSS_ISID_START, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            *(vlan_registration_entry + pit.iport) = VLAN_NORMAL;
        }
    }
    
    VLAN_CRIT_EXIT();
    return rc;
}
/*****************************************************************/
/* Description:Management VLAN list add function                 */
/* - Both configuration update and chip update through message   */
/* - module and API                                              */
/*                                                               */
/* Input: Note vlan members as boolean port list                 */
/*        @isid must be LEGAL or GLOBAL                          */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
vtss_rc vlan_mgmt_vlan_add(vtss_isid_t isid_add, vlan_mgmt_entry_t *vlan_mgmt_entry, vlan_user_t user)
{
    vtss_rc             rc = VTSS_OK, rc1 = VTSS_OK, rc2 = VTSS_OK;
    switch_iter_t       sit;
    port_iter_t         pit;
    vlan_mgmt_entry_t   prev_entry, f_prev_entry;
    vlan_entry_conf_t   vlan_entry, f_vlan_entry; 
    vlan_action_t       action = VLAN_ACTION_ADD;
    vtss_isid_t         zero_based_isid;
    vtss_port_no_t      port_no;
    BOOL                vlan_exists = FALSE, error_conflict;
    BOOL                member_ports_exist, forbid_ports_conf_exist = FALSE, forbid_ports_exists_on_this_switch;
    BOOL                stack_conf_update = FALSE, forbid_stack_conf_update = FALSE;
    /* The following three port arrays are used for VLAN member callback */
    u8                  added_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
    u8                  deleted_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
    u8                  current_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
    /* The following three port arrays are used for VLAN forbidden callback */
    u8                  f_added_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
    u8                  f_deleted_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
    u8                  f_current_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
    
    T_D("enter, isid_add: %u, vid: %u user:%s", isid_add, vlan_mgmt_entry->vid, vlan_user_id_txt( user));
    /* Validation checks */
    if (!msg_switch_is_master()) {
        T_W("Not master");
        return VLAN_ERROR_STACK_STATE;
    }
    if (!(VTSS_ISID_LEGAL(isid_add) || (isid_add == VTSS_ISID_GLOBAL))) {
        T_E("Invalid ISID (%u). LEGAL expected", isid_add);
        return VLAN_ERROR_PARM;
    }
    vlan_entry.vid = vlan_mgmt_entry->vid;
    f_vlan_entry.vid = vlan_mgmt_entry->vid;
    /* Loop through all the ports to find whether forbidden port conf exists */
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        if ((vlan_mgmt_entry->ports[port_no] == VLAN_FORBIDDEN_PORT) || 
            (vlan_mgmt_entry->ports[port_no] == VLAN_NON_FORBIDDEN_PORT)) {
            forbid_ports_conf_exist = TRUE;
        }
    } /* for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) */
    if (forbid_ports_conf_exist && (user != VLAN_USER_STATIC)) {
        T_E("Only static VLAN user can configure forbidden ports");
        return VLAN_ERROR_PARM;
    }
    (void) switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID);
    /* Loop through all the switches and add to database */
    while (switch_iter_getnext(&sit)) {
        member_ports_exist = FALSE;
        forbid_ports_exists_on_this_switch = FALSE;
        memset(&vlan_entry.ports[0][0], 0, sizeof(vlan_entry.ports));
        if (forbid_ports_conf_exist) {
            memset(&f_vlan_entry.ports[0][0],0,sizeof(f_vlan_entry.ports));
        }
        error_conflict = FALSE; /* per-switch check */
        zero_based_isid = sit.isid - VTSS_ISID_START;
        /* Derive newly added ports */
        if (vlan_get_from_database(sit.isid, vlan_mgmt_entry->vid, &prev_entry, FALSE, user) == VTSS_RC_OK) {
            /* This means VLAN already exists */
            vlan_exists = TRUE;
        } else {
            memset(&prev_entry, 0, sizeof(prev_entry));
        }
        if (vlan_forbidden_list_get(sit.isid, vlan_mgmt_entry->vid, &f_prev_entry, FALSE, user) != VTSS_OK) {
            memset(&f_prev_entry, 0, sizeof(f_prev_entry));
            T_I("vlan_forbidden_list_get failed\n");
        }
        (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        /* Loop through all the ports and add to appropriate list */
        while (port_iter_getnext(&pit)) {
            if ((vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] != VLAN_FORBIDDEN_PORT) &&
                (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] != VLAN_NON_FORBIDDEN_PORT)){
                /* Check if the port is already forbidden. This applies to only Static user */
                if ((f_prev_entry.ports[pit.iport + VTSS_PORT_NO_START]) && (user == VLAN_USER_STATIC)) {
                    T_I("This port is already forbidden. This cannot be added as member port\n");
                    error_conflict = TRUE;
                    break;
                }
                if (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] == VLAN_MEMBER_PORT) {
                    VTSS_BF_SET(vlan_entry.ports[sit.isid], pit.iport, 1);
                } else {
                    VTSS_BF_SET(vlan_entry.ports[sit.isid], pit.iport, 0);
                }
                member_ports_exist = TRUE;
            } else { /* populate forbid ports */
                /* Check if the port is already a member port */
                if ((prev_entry.ports[pit.iport + VTSS_PORT_NO_START]) && (user == VLAN_USER_STATIC)) {
                    if (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] == VLAN_FORBIDDEN_PORT) { /* is this forbidden */
                        T_I("This port is already a member port. This cannot be added as forbidden port\n");
                        error_conflict = TRUE;
                        break;
                    }
                }
                forbid_ports_exists_on_this_switch = TRUE;
                if (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] == VLAN_FORBIDDEN_PORT) {
                    VTSS_BF_SET(f_vlan_entry.ports[sit.isid], pit.iport, 1);
                } else {
                    VTSS_BF_SET(f_vlan_entry.ports[sit.isid], pit.iport, 0);
                }
            }
            /* Populate newly added ports */
            if (!prev_entry.ports[pit.iport + VTSS_PORT_NO_START]) { /* This means non-member port */
                if (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] == VLAN_MEMBER_PORT) {
                    VTSS_PORT_BF_SET(added_ports[zero_based_isid], pit.iport, 1); 
                }
            }
            /* Populate deleted ports */
            if (prev_entry.ports[pit.iport + VTSS_PORT_NO_START]) { /* This means member-port */
                if (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] == VLAN_NON_MEMBER_PORT) {
                    VTSS_PORT_BF_SET(deleted_ports[zero_based_isid], pit.iport, 1); 
                }
            }
            /* Populate current ports */
            if (prev_entry.ports[pit.iport + VTSS_PORT_NO_START] ||
                    VTSS_PORT_BF_GET(added_ports[zero_based_isid], pit.iport)) {
                if (!VTSS_PORT_BF_GET(deleted_ports[zero_based_isid], pit.iport)) {
                    VTSS_PORT_BF_SET(current_ports[zero_based_isid], pit.iport, 1); 
                }
            }
            if (forbid_ports_conf_exist) {
                /* Populate newly added forbidden ports */
                if (!f_prev_entry.ports[pit.iport + VTSS_PORT_NO_START]) { /* This means non-forbidden port */
                    if (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] == VLAN_FORBIDDEN_PORT) {
                        VTSS_PORT_BF_SET(f_added_ports[zero_based_isid], pit.iport, 1); 
                    }
                }
                /* Populate deleted forbidden ports */
                if (f_prev_entry.ports[pit.iport + VTSS_PORT_NO_START]) { /* This means forbidden port */
                    if (vlan_mgmt_entry->ports[pit.iport + VTSS_PORT_NO_START] == VLAN_NON_FORBIDDEN_PORT) {
                        VTSS_PORT_BF_SET(f_deleted_ports[zero_based_isid], pit.iport, 1); 
                    }
                }
                /* Populate current forbidden ports */
                if (f_prev_entry.ports[pit.iport + VTSS_PORT_NO_START] ||
                        VTSS_PORT_BF_GET(f_added_ports[zero_based_isid], pit.iport)) {
                    if (!VTSS_PORT_BF_GET(f_deleted_ports[zero_based_isid], pit.iport)) {
                        VTSS_PORT_BF_SET(f_current_ports[zero_based_isid], pit.iport, 1); 
                    }
                }
            } /* if (forbid_ports_conf_exist) */
        } /*  while (port_iter_getnext(&pit)) */
        /* If there are no conflicts, then add the configuration to the list. If not, the configuration will not be
           applied to this particular switch */
        if (!error_conflict) {
            if (member_ports_exist) {
                T_D("member_ports_exist");
                // Add/modify VID. It may return rc != VTSS_OK if table is full.
                if ((rc += vlan_list_vlan_add(&vlan_entry, sit.isid, user)) == VTSS_OK) {
                    stack_conf_update = TRUE;
                    VLAN_CRIT_ENTER();
                    vlan_user_conf(sit.isid, vlan_mgmt_entry->vid, user, 1);
                    VLAN_CRIT_EXIT();
                    if (user == VLAN_USER_STATIC) {
                        rc += vlan_conf_commit(vlan_mgmt_entry->vid, TRUE); // Save it to flash
                    }
                } else {
                    T_D("Error in adding the VLAN-ID:%d membership in Database,User: %s Reason: %s",
                            vlan_entry.vid, vlan_user_id_txt(user), vlan_error_txt(rc));
                } /* else of if (rc += vlan_list_vlan_add(&vlan_entry, sit.isid, user)) == VTSS_OK) */
            }
            /* Only static user can add the forbidden ports */
            if (forbid_ports_exists_on_this_switch && (user == VLAN_USER_STATIC)) {
                T_D("forbid_ports_exists_on_this_switch");
                rc1 =  vlan_forbidden_list_vlan_add(&f_vlan_entry, sit.isid, user);
                if ((rc1 == VTSS_OK) || (rc1 == VLAN_ERROR_VLAN_DYNAMIC_USER)) {
                    forbid_stack_conf_update = TRUE;
                    rc += vlan_forbidden_conf_commit(vlan_mgmt_entry->vid, TRUE);     // Save it to flash
                } else {
                    T_E("Error in adding the VLAN-ID:%d forbidden membership in Database,User: %s Reason: %s",
                        f_vlan_entry.vid, vlan_user_id_txt(user), vlan_error_txt(rc));
                    rc = rc1;
                }
            } /* if (forbid_ports_exists_on_this_switch && (user == VLAN_USER_STATIC)) */
        } else {
            rc2 = VLAN_ERROR_VLAN_FORBIDDEN;
        } /* if (!error_conflict) */
    } /* while (switch_iter_getnext(&sit))  */
    /* Inform slaves and call the module callbacks to inform other modules of VLAN changes */
    if (stack_conf_update || (forbid_stack_conf_update && forbid_ports_conf_exist && (rc1 == VLAN_ERROR_VLAN_DYNAMIC_USER))) {
        vlan_stack_vlan_conf_add(isid_add, vlan_entry.vid);
        if (stack_conf_update) {
            if (vlan_exists == TRUE) {
                action = VLAN_ACTION_UPDATE;
            }
            vlan_membership_change_callback(isid_add, vlan_mgmt_entry->vid, user, added_ports, deleted_ports, current_ports, action);
        }
    }
    if (forbid_stack_conf_update && forbid_ports_conf_exist) {
        vlan_forbid_change_callback(isid_add, vlan_mgmt_entry->vid, f_added_ports, f_deleted_ports, f_current_ports);
    }
    T_D("exit: rc %d, rc2 %d",rc, rc2);

    return (rc2 == VTSS_RC_OK) ? rc : rc2;
}

#ifdef  VTSS_SW_OPTION_VLAN_NAME
/*****************************************************************/
/* Description:Management VLAN list add function                 */
/* - Get vid from VLAN name                                      */
/* - Both configuration update and chip update through message   */
/* - module and API                                              */
/*                                                               */
/* Input: Note vlan members as boolean port list                 */
/*        @isid must be LEGAL or GLOBAL                          */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
vtss_rc vlan_mgmt_vlan_add_by_name(vtss_isid_t isid_add, vlan_mgmt_entry_t *vlan_mgmt_entry, vlan_user_t user)
{
    vtss_rc   rc;

    if (!msg_switch_is_master()) {
        T_W("Not master");
        return VLAN_ERROR_STACK_STATE;
    }       

    if (!(VTSS_ISID_LEGAL(isid_add) || (isid_add == VTSS_ISID_GLOBAL))) {
        T_E("Invalid ISID (%u). LEGAL expected", isid_add);
        return VLAN_ERROR_PARM;
    }
    if (vlan_mgmt_entry == NULL) {
        T_E("vlan_mgmt_entry is NULL\n");
        return VLAN_ERROR_PARM;
    }
    T_D("vlan_mgmt_vlan_add_by_name: isid_add: %u, vlan name: %s user:%s",
            isid_add, vlan_mgmt_entry->vlan_name, vlan_user_id_txt(user));
    rc = vlan_get_vid_from_vlan_name(vlan_mgmt_entry->vlan_name, &(vlan_mgmt_entry->vid));
    if (rc != VTSS_RC_OK) {
        T_D("vlan_mgmt_vlan_add_by_name: Invalid name\n");
        return  rc;
    }
    rc = vlan_mgmt_vlan_add(isid_add, vlan_mgmt_entry, user);

    return rc;
}
#endif

/*****************************************************************/
/* Description:Management VLAN list delete function              */
/* - Both configuration update and chip update through API       */
/* Deletes globally. Use "cli: vlan add <vid> <stack_port>" to   */
/* delete frontports locally on a switch.                        */
/*                                                               */
/* Input:                                                        */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/
/* Delete VLAN */
vtss_rc vlan_mgmt_vlan_del(vtss_isid_t isid_del, vtss_vid_t vid, vlan_user_t user, vlan_op_type_t oper)
{
    vtss_rc             rc = VTSS_RC_OK, rc1 = VTSS_RC_OK, rc2 = VTSS_RC_OK;
    vlan_entry_conf_t   vlan_entry; 
    vlan_mgmt_entry_t   prev_entry, f_prev_entry;
    vlan_action_t       action = VLAN_ACTION_UPDATE;
    port_iter_t         pit;
    vtss_isid_t         zero_based_isid;
    u8                  added_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
    u8                  deleted_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
    u8                  current_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE] = {{0}};
#ifdef VTSS_SW_OPTION_VLAN_NAME
    switch_iter_t       sit;
    BOOL                vlan_name_delete = TRUE;
    i8                  vlan_name[VLAN_NAME_MAX_LEN];
#endif

    T_D("enter, vid: %u user: %s", vid, vlan_user_id_txt( user));
    if (!msg_switch_is_master()) {
        T_W("Not master");
        return VLAN_ERROR_STACK_STATE;
    }
    if (!(VTSS_ISID_LEGAL(isid_del) || (isid_del == VTSS_ISID_GLOBAL))) {
        T_E("Invalid ISID (%u). LEGAL expected", isid_del);
        return VLAN_ERROR_PARM;
    }
    /* TODO: This will be implemented later */
    if (oper == VLAN_MIXED_OP) {
        T_E("Currently this operation is not supported");
        return VLAN_ERROR_PARM;
    }
    (void) switch_iter_init(&sit, isid_del, SWITCH_ITER_SORT_ORDER_ISID);
    /* Loop through all the switches to populate deleted_ports */
    while (switch_iter_getnext(&sit)) {
        zero_based_isid = sit.isid - VTSS_ISID_START;
        rc1 = vlan_get_from_database(sit.isid, vid, &prev_entry, FALSE, user);
        if (rc1 == VTSS_RC_OK) { /* This means VLAN members exist */
            if (((oper == VLAN_MEMBERSHIP_OP) && ((isid_del != VTSS_ISID_GLOBAL) && (isid_del != sit.isid))) ||
                (oper == VLAN_FORBIDDEN_OP)) { /* VLAN members exist on other ISIDs or forbidden members exist on this ISID */
                vlan_name_delete = FALSE;
            }
        }
        rc2 = vlan_forbidden_list_get(sit.isid, vid, &f_prev_entry, FALSE, user);
        if (rc2 == VTSS_RC_OK) { /* This means forbid members exist */
            if (((oper == VLAN_FORBIDDEN_OP) && ((isid_del != VTSS_ISID_GLOBAL) && (isid_del != sit.isid))) ||
                (oper == VLAN_MEMBERSHIP_OP)) { /* VLAN forbidden members exist on other ISIDs or VLAN members exist on this ISID*/
                vlan_name_delete = FALSE;
            }
        }
        if ((rc1 == VTSS_RC_OK) ||(rc2 == VTSS_RC_OK)) {
            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (oper == VLAN_MEMBERSHIP_OP) {
                    if (prev_entry.ports[pit.iport]) {
                        VTSS_PORT_BF_SET(deleted_ports[zero_based_isid], pit.iport, 1); 
                    }
                } else if (oper == VLAN_FORBIDDEN_OP) {
                    if (f_prev_entry.ports[pit.iport]) {
                        VTSS_PORT_BF_SET(deleted_ports[zero_based_isid], pit.iport, 1); 
                    }
                } else {
                    T_D("Illegal VLAN operation");
                    return VLAN_ERROR_PARM;
                } /* else of if (oper == VLAN_MEMBERSHIP_OP) */
            } /* while (port_iter_getnext(&pit)) */
        } /* if (rc == VTSS_RC_OK) */
    } /* while (switch_iter_getnext(&sit)) */
    /* Populate the vlan_entry here */
    vlan_entry.vid = vid;
    memset(&vlan_entry.ports[0][0], 0, sizeof(vlan_entry.ports));
    if (oper == VLAN_MEMBERSHIP_OP) {
        rc += vlan_list_vlan_del(isid_del, vid, user);
        VLAN_CRIT_ENTER();
        vlan_user_conf(isid_del, vid, user, 0);
        VLAN_CRIT_EXIT();
        if (user == VLAN_USER_STATIC) {
            rc += vlan_conf_commit(vid, FALSE); // Save it to flash
        }
    } else { /* VLAN_FORBIDDEN_OP */
        rc += vlan_list_vlan_forbidden_del(isid_del, vid, user);
        if (user == VLAN_USER_STATIC) {
            rc += vlan_forbidden_conf_commit(vid, FALSE); // Save it to flash
        }
    }
    if (rc == VTSS_OK) {
#ifdef VTSS_SW_OPTION_VLAN_NAME
        if (vlan_name_delete && (user == VLAN_USER_STATIC)) { /* VLAN Name can only be configured by static VLAN user */
            if (vlan_get_vlan_name_from_vid(vid, vlan_name) != VTSS_RC_OK) {
                T_D("vlan_mgmt_vlan_del: Error while getting VLAN Name from VID\n");
            }
            if (vlan_mgmt_vlan_name_del(vlan_name) != VTSS_RC_OK) {
                T_D("vlan_mgmt_vlan_del: Error while deleting VLAN Name\n");
            } /* if (vlan_mgmt_vlan_name_del(vlan_name) */
        } /* if (vlan_name_delete) */
#endif
        if (vlan_name_delete) { /* VLAN name is deleted if none of VLAN members or forbidden members exist */
            action = VLAN_ACTION_DELETE;
        }
        /* Announce changes */
        if (oper == VLAN_MEMBERSHIP_OP) {
            vlan_membership_change_callback(isid_del, vid, user, added_ports, deleted_ports, current_ports, action);
        } else {
             vlan_forbid_change_callback(isid_del, vid, added_ports, deleted_ports, current_ports);
        }
        /* Tell it to all switches in the stack. */
        if (vlan_list_vlan_get_port_bitmap(VTSS_ISID_START, VTSS_ISID_END, vid, &vlan_entry) != VTSS_OK) {
            vlan_stack_vlan_conf_del(isid_del, &vlan_entry);
        } else {
            vlan_stack_vlan_conf_add(isid_del, vid);
        }
    } /* if (rc == VTSS_OK) */
    T_D("exit, id: %u", vid);

    return rc;
}

#ifdef  VTSS_SW_OPTION_VLAN_NAME
/*****************************************************************/
/* Description:Management VLAN list delete function that uses    */
/* VLAN Name insted of vid. It derives vid from VLAN name.       */
/* - Both configuration update and chip update through API       */
/* Deletes globally.                                             */
/*                                                               */
/* Input:                                                        */
/* Output: Return VTSS_OK if success                             */
/*****************************************************************/

/* Delete VLAN by VLAN name*/
vtss_rc vlan_mgmt_vlan_del_by_name(vtss_isid_t isid_del, i8 *vlan_name, vlan_user_t user, vlan_op_type_t oper) 
{
    vtss_rc     rc;
    vtss_vid_t  vid;

    if (vlan_name == NULL) {
        T_E("vlan_mgmt_vlan_del_by_name: vlan_name is NULL\n");
        return VLAN_ERROR_PARM;
    }

    T_D("vlan_mgmt_vlan_del_by_name: enter, isid_del: %u, vlan name: %s\n", isid_del, vlan_name);

    if (!msg_switch_is_master()) {
        T_W("Not master");
        return VLAN_ERROR_STACK_STATE;
    }

    if (!(VTSS_ISID_LEGAL(isid_del) || (isid_del == VTSS_ISID_GLOBAL))) {
        T_E("Invalid ISID (%u). LEGAL expected", isid_del);
        return VLAN_ERROR_PARM;
    }

    rc = vlan_get_vid_from_vlan_name(vlan_name, &vid);
    if (rc != VTSS_RC_OK) {
        T_D("vlan_mgmt_vlan_del_by_name: Invalid vlan and invalid name\n");
        return  VLAN_ERROR_PARM;
    }
    rc = vlan_mgmt_vlan_del(isid_del, vid, user, oper);
    return rc;
}
#endif

/* vlan_mgmt_conflicts_get - Get the current conflicts from VLAN Port
 *                            Configuration database
 *  Input  parms @ pointer to conflicts structure, isid and port
 *  Output parms @ VTSS error code and populated conflicts structure
 */
vtss_rc vlan_mgmt_conflicts_get(vlan_port_conflicts_t* conflicts,
        vtss_isid_t isid, vtss_port_no_t port)
{
    vtss_rc rc = VTSS_OK;
    vtss_vlan_port_conf_t api_conf;

    memset(conflicts, 0, sizeof(vlan_port_conflicts_t));
    memset (&api_conf, 0, sizeof(vtss_vlan_port_conf_t));
    VLAN_CRIT_ENTER();
    if (vlan_port_conflict_resolver(isid, port, conflicts, &api_conf)
            != VTSS_OK) {
        rc = VTSS_RC_ERROR;
    }
    VLAN_CRIT_EXIT();
    return rc;
}

vtss_rc vlan_mgmt_default_port_conf_get(vlan_port_conf_t *vlan_port_conf)
{
    /* Use default values */
    vlan_port_conf->pvid = VLAN_TAG_DEFAULT;
    vlan_port_conf->untagged_vid = vlan_port_conf->pvid;
    vlan_port_conf->frame_type = VTSS_VLAN_FRAME_ALL;
    vlan_port_conf->ingress_filter = 0;
    vlan_port_conf->tx_tag_type = VLAN_TX_TAG_TYPE_UNTAG_THIS;
    vlan_port_conf->port_type = VLAN_PORT_TYPE_UNAWARE;
    vlan_port_conf->flags = VLAN_PORT_FLAGS_ALL;

    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/*****************************************************************/
/* Description:Read/create VLAN configuration                    */
/* - VLAN port configuration                                     */
/*                                                               */
/* Input: if (create) then new else read current                 */
/* Output: Return void                                           */
/*****************************************************************/
static void vlan_conf_read_switch(vtss_isid_t isid_add)
{
    conf_blk_id_t         blk_id;
    vlan_port_blk_t       *port_blk;
    vlan_port_conf_t      new_conf, *port_conf, default_conf={0};
    u32                   i, j;
    BOOL                  do_create;
    ulong                 size;
#ifdef  VOLATILE_VLAN_USER_PRESENT
    vlan_user_t           usr;
#endif
    vtss_vlan_port_conf_t api_conf;
    vlan_port_conflicts_t conflicts;
    switch_iter_t         sit;
    port_iter_t           pit;

    T_I("enter, isid_add: %d", isid_add);
    /* Read/create VLAN port configuration */
    blk_id = CONF_BLK_VLAN_PORT_TABLE;
    if ((port_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, &size)) == NULL || size != sizeof(*port_blk)) {
        T_W("conf_sec_open failed or size mismatch, creating defaults");
        port_blk = conf_sec_create(CONF_SEC_GLOBAL, blk_id, sizeof(*port_blk));
        do_create = 1;
    } else if (port_blk->version != VLAN_PORT_BLK_VERSION) {
        T_W("version mismatch, creating defaults");
        do_create = 1;
    } else {
        do_create = (isid_add != VTSS_ISID_GLOBAL);
    }
    if (do_create) {
        /* Use default values */
        vlan_port_default_set(&default_conf);
    }
    if (isid_add == VTSS_ISID_GLOBAL) {
        (void) switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    } else {
        (void) switch_iter_init(&sit, isid_add, SWITCH_ITER_SORT_ORDER_ISID);
    }
    while (switch_iter_getnext(&sit)) {
        VLAN_CRIT_ENTER();
        (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT_ALL, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            i = (sit.isid - VTSS_ISID_START);
            j = (pit.iport - VTSS_PORT_NO_START);
            if (do_create) {
                if (port_blk != NULL) {
                    port_blk->conf[i][j] = default_conf;
                }
                port_conf = &vlan.vlan_port_conf[VLAN_USER_STATIC][sit.isid - VTSS_ISID_START][j];
                memcpy(port_conf, &default_conf, sizeof(vlan_port_conf_t));
                memset(&conflicts, 0, sizeof(vlan_port_conflicts_t));
                memset (&api_conf, 0, sizeof(vtss_vlan_port_conf_t));
                if (vlan_port_conflict_resolver(sit.isid, pit.iport, &conflicts, &api_conf) != VTSS_OK) {
                    VLAN_CRIT_EXIT();
                    return;
                }
            } else {
                /* Use new configuration */
                new_conf = port_blk->conf[i][j];
                port_conf = &vlan.vlan_port_conf[VLAN_USER_STATIC][sit.isid - VTSS_ISID_START][j];
                *port_conf = new_conf;
#ifdef  VOLATILE_VLAN_USER_PRESENT
                for (usr = (VLAN_USER_STATIC + 1); usr < VLAN_USER_ALL; usr++) {
                    port_conf = &vlan.vlan_port_conf[usr][i][j];
                    port_conf->flags = 0;
                }
#endif
                /* Sync static configuration to actual configuration */
                port_conf = &vlan.vlan_port_conf[VLAN_USER_ALL][sit.isid - VTSS_ISID_START][j];
                *port_conf = new_conf;
            }

        } /* while (port_iter_getnext(&pit)) */
        VLAN_CRIT_EXIT();
        if ((isid_add != VTSS_ISID_GLOBAL) && (msg_switch_exists(sit.isid))) {
            if (vlan_stack_port_conf_set(sit.isid) != VTSS_OK) {
                T_D("vlan_stack_port_conf_set FAILED");
            }
        } /* if (isid_add!=VTSS_ISID_GLOBAL && msg_switch_exists(sit.isid)) */
    } /*  while (switch_iter_getnext(&sit)) */
    if (port_blk == NULL) {
        T_W("failed to open VLAN port table");
    } else {
        port_blk->version = VLAN_PORT_BLK_VERSION;
        conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    }
    T_D("exit");

    return;
} /* End - Read/create VLAN Switch configuration */

#ifdef  VTSS_SW_OPTION_VLAN_NAME
static void vlan_name_conf_read_stack(BOOL create)
{
    vlan_name_table_blk_t   *vlan_name_blk;
    conf_blk_id_t           blk_id;
    BOOL                    do_create;
    ulong                   size;
    u32                     i, zero_based_idx;

    T_D("enter, create: %d", create);

    /* Read/create VLAN table configuration */
    blk_id = CONF_BLK_VLAN_NAME_TABLE;

    if (((vlan_name_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, &size)) == NULL) ||
            (size != sizeof(*vlan_name_blk))) {
        T_W("conf_sec_open failed or size mismatch, creating defaults");
        vlan_name_blk = conf_sec_create(CONF_SEC_GLOBAL, blk_id, sizeof(*vlan_name_blk));
        do_create = 1;
    } else if (vlan_name_blk->version != VLAN_NAME_BLK_VERSION) {
        T_W("version mismatch, creating defaults");
        do_create = 1;
    } else {
        do_create = create;
    }

    VLAN_CRIT_ENTER();
    if (do_create && vlan_name_blk != NULL) {
        T_D("do_create && vlan_name_blk != NULL");
        vlan_name_blk->count = 1;
        vlan_name_blk->size = sizeof(vlan_name_conf_t);
        vlan_name_blk->table[0].vid = VTSS_VID_DEFAULT;
        strcpy(vlan_name_blk->table[0].vlan_name, "default");
    }
    memset(vlan.vlan_name_conf, 0, sizeof(vlan.vlan_name_conf));
    if (vlan_name_blk == NULL) {
        T_W("failed to open VLAN Name table");
    } else {
        for (i = vlan_name_blk->count; i != 0; i--) {
            zero_based_idx = i - 1;
            vlan.vlan_name_conf[zero_based_idx].vid = vlan_name_blk->table[zero_based_idx].vid;
            strcpy(vlan.vlan_name_conf[zero_based_idx].vlan_name, 
                                vlan_name_blk->table[zero_based_idx].vlan_name);
            vlan.vlan_name_conf[zero_based_idx].name_valid = TRUE;
        }
        vlan_name_blk->version = VLAN_NAME_BLK_VERSION;
        conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    }
    VLAN_CRIT_EXIT();
}
#endif

/* Read/create VLAN stack configuration */
static void vlan_conf_read_stack(BOOL create)
{
    conf_blk_id_t           blk_id;
    conf_blk_id_t           blk_fb_id;
    vlan_table_blk_t        *vlan_blk;
    vlan_forbid_table_blk_t *vlan_fb_blk = NULL;
    vlan_list_t             *list;
    vlan_t                  *vlan_stack;
    vlan_t                  *vlan_p;
    u32                     i,zero_based_blk_ct = 0, changed = FALSE;
    BOOL                    do_create;
    vtss_isid_t             zero_based_isid = VTSS_ISID_START;
    ulong                   size;
    switch_iter_t           sit;
    u32                     port_iter;
    vtss_vid_t              vlan_idx;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    vtss_etype_t            tpid = VLAN_TPID_DEFAULT;
#endif
    
    T_D("enter, create: %d", create);
    /* Read/create VLAN table configuration */
    blk_id = CONF_BLK_VLAN_TABLE;
    if ((vlan_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_id, &size)) == NULL ||
    size != sizeof(*vlan_blk)) {
        T_W("conf_sec_open failed or size mismatch, creating defaults");
        vlan_blk = conf_sec_create(CONF_SEC_GLOBAL, blk_id, sizeof(*vlan_blk));
        do_create = 1;
    } else if (vlan_blk->version != VLAN_MEMBERSHIP_BLK_VERSION) {
        if (vlan_blk->version != 0) {
            T_W("version mismatch, creating defaults version = %u, current version = %d\n", vlan_blk->version, 
                VLAN_MEMBERSHIP_BLK_VERSION);
            do_create = 1;
        } else {
            do_create = create;
        }
    } else {
        do_create = create;
    }
    VLAN_CRIT_ENTER();
    if (do_create && vlan_blk != NULL) {
        T_D("do_create && vlan_blk != NULL");
        for (vlan_idx = 0; vlan_idx < VLAN_MAX; vlan_idx++) {
            vlan_blk->table[vlan_idx].vid = 0;
        }
        vlan_blk->count = 1;
        vlan_blk->size = sizeof(vlan_entry_conf_t);
        vlan_blk->table[0].vid = VTSS_VID_DEFAULT;
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
        while (switch_iter_getnext(&sit)) {
            memset(&(vlan_blk->table[0].ports[sit.isid][0]),0xff,VTSS_PORT_BF_SIZE);
            for (port_iter = VTSS_PORT_NO_START; port_iter < port_isid_port_count(sit.isid); port_iter++) {
                if (port_isid_port_no_is_stack(sit.isid, port_iter)) {
                    VTSS_BF_SET(vlan_blk->table[0].ports[sit.isid], (port_iter - VTSS_PORT_NO_START), 0x0);
                }
            }
        }
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        vlan_blk->etype = tpid;
#endif
        changed = TRUE;
    }
    /* Initialize VLAN for stack: All free */
    memset(vlan_multi_membership_table, 0,sizeof(vlan_multi_membership_table));
    memset(vlan.vlan_stack_table,0,sizeof(vlan.vlan_stack_table));
    memset(vlan.vlan_users, 0, sizeof(vlan.vlan_users));
    list = &vlan.stack_vlan;
    list->free = NULL;
    for (i = 0; i < VLAN_MAX; i++) {
        vlan_p = &vlan.vlan_stack_table[i];
        vlan_p->next = list->free;
        list->free = vlan_p;
    }
    if (vlan_blk == NULL) {
        T_W("failed to open VLAN table");
    } else {
        /* Add new VLANs */
        if (vlan_blk->version == 0) {
            for (i = vlan_blk->count; i != 0; i--) {
                zero_based_blk_ct = i - 1;
                /* Move entry from free list to used list */
                if ((vlan_stack = list->free) == NULL)
                    break;
                list->free = vlan_stack->next;
                vlan_stack->next = NULL;
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                while (switch_iter_getnext(&sit)) {
                    zero_based_isid = sit.isid - VTSS_ISID_START;
                    /* Assign The ports for 1-24 Bit Fields */
                    memcpy(&(vlan_stack->conf.ports[VLAN_USER_STATIC][zero_based_isid][0]), 
                           &(vlan_blk->table[zero_based_blk_ct].ports[sit.isid][0]),
                           VTSS_PORT_BF_SIZE);
                    vlan_user_conf(sit.isid, vlan_blk->table[zero_based_blk_ct].vid, VLAN_USER_STATIC, 1);
                }
                vlan_multi_membership_table[vlan_blk->table[zero_based_blk_ct].vid] = vlan_stack;
            }
        } else { /* newer version */
            i = 0;
            for (vlan_idx = 0; vlan_idx < VLAN_MAX; vlan_idx++) {
                if (i < vlan_blk->count) {
                    if (vlan_blk->table[vlan_idx].vid != 0) {
                        /* Move entry from free list to used list */
                        if ((vlan_stack = list->free) == NULL)
                            break;
                        list->free = vlan_stack->next;
                        vlan_stack->next = NULL; 
                        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                        while (switch_iter_getnext(&sit)) {
                            zero_based_isid = sit.isid - VTSS_ISID_START;
                            /* Assign The ports for 1-24 Bit Fields */
                            memcpy(&(vlan_stack->conf.ports[VLAN_USER_STATIC][zero_based_isid][0]),
                                    &(vlan_blk->table[vlan_idx].ports[sit.isid][0]),
                                    VTSS_PORT_BF_SIZE);
                            vlan_user_conf(sit.isid, vlan_blk->table[vlan_idx].vid, VLAN_USER_STATIC, 1);
                        }
                        vlan_multi_membership_table[vlan_blk->table[vlan_idx].vid] = vlan_stack;
                        i++;
                    }
                } else {
                    vlan_blk->table[vlan_idx].vid = 0; /* 0 indicates invalid entry */
                }
            }
        }
        vlan_blk->version = VLAN_MEMBERSHIP_BLK_VERSION;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        vlan.tpid_s_custom_port = vlan_blk->etype;
        tpid = vlan_blk->etype;
#endif
        conf_sec_close(CONF_SEC_GLOBAL, blk_id);
    }
    blk_fb_id = CONF_BLK_VLAN_FORBIDDEN;
    if ((vlan_fb_blk = conf_sec_open(CONF_SEC_GLOBAL, blk_fb_id, &size)) == NULL 
    ||size != sizeof(*vlan_fb_blk)) {
        T_W("conf_sec_open failed or size mismatch, creating defaults");
        vlan_fb_blk = conf_sec_create(CONF_SEC_GLOBAL, blk_fb_id, sizeof(*vlan_fb_blk));
        do_create = 1;
    } else if (vlan_fb_blk->version != VLAN_FORBID_MEMBERSHIP_BLK_VERSION) {
        if (vlan_fb_blk->version != 0) {
            T_W("forbid version mismatch, creating defaults version = %u, current version = %d\n", vlan_fb_blk->version, 
                VLAN_FORBID_MEMBERSHIP_BLK_VERSION);
            do_create = 1;
        } else {
            do_create = create;
        }
    } else {
        do_create = create;
    }
    if (do_create && vlan_fb_blk != NULL) {
        T_D("do_create && vlan_blk != NULL");
        for (vlan_idx = 0; vlan_idx < VLAN_MAX; vlan_idx++) {
            vlan_fb_blk->forbidden[vlan_idx].vid = 0;
        }
        vlan_fb_blk->size = sizeof(vlan_entry_conf_t);
        vlan_fb_blk->fb_count = 0;
        memset(vlan_fb_blk->forbidden,0,sizeof(vlan_fb_blk->forbidden));
    }
    memset(vlan_forbidden_table, 0,sizeof(vlan_forbidden_table));
    if (vlan_fb_blk == NULL) {
        T_W("failed to open VLAN forbidden table");
    } else {
        if (vlan_fb_blk->version == 0) {
            for(i = vlan_fb_blk->fb_count; i != 0; i--) {
                zero_based_blk_ct = i - 1;
                if(vlan_multi_membership_table[vlan_fb_blk->forbidden[zero_based_blk_ct].vid]) {
                    vlan_stack = vlan_multi_membership_table[vlan_fb_blk->forbidden[zero_based_blk_ct].vid] ;
                    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                    while (switch_iter_getnext(&sit)) {
                        zero_based_isid = sit.isid - VTSS_ISID_START;
                        /* Assign The ports for 1-24 Bit Fields */
                        memcpy(&(vlan_stack->fb_port.ports[zero_based_isid][0]), 
                               &(vlan_fb_blk->forbidden[zero_based_blk_ct].ports[sit.isid][0]),
                               VTSS_PORT_BF_SIZE);
                    }
                    vlan_forbidden_table[vlan_fb_blk->forbidden[zero_based_blk_ct].vid] = vlan_stack;
                }
                if(vlan_forbidden_table[vlan_fb_blk->forbidden[zero_based_blk_ct].vid]  == NULL) {
                    /* Move entry from free list to used list */
                    if ((vlan_stack = list->free) == NULL)
                        break;
                    list->free = vlan_stack->next;
                    vlan_stack->next = NULL;
                    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                    while (switch_iter_getnext(&sit)) {
                        zero_based_isid = sit.isid - VTSS_ISID_START;
                        /* Assign The ports for 1-24 Bit Fields */
                        memcpy(&(vlan_stack->fb_port.ports[zero_based_isid][0]),
                               &(vlan_fb_blk->forbidden[zero_based_blk_ct].ports[sit.isid][0]),
                               VTSS_PORT_BF_SIZE);
                    }
                    vlan_forbidden_table[vlan_fb_blk->forbidden[zero_based_blk_ct].vid] = vlan_stack;
                }
            }
        } else { /* new version */
            i = 0;
            for (vlan_idx = 0; vlan_idx < VLAN_MAX; vlan_idx++) {
                if (i < vlan_fb_blk->fb_count) {
                    if (vlan_fb_blk->forbidden[vlan_idx].vid != 0) {
                        if(vlan_multi_membership_table[vlan_fb_blk->forbidden[vlan_idx].vid]) {
                            vlan_stack = vlan_multi_membership_table[vlan_fb_blk->forbidden[vlan_idx].vid] ;
                            (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                            while (switch_iter_getnext(&sit)) {
                                zero_based_isid = sit.isid - VTSS_ISID_START;
                                /* Assign The ports for 1-24 Bit Fields */
                                memcpy(&(vlan_stack->fb_port.ports[zero_based_isid][0]), 
                                        &(vlan_fb_blk->forbidden[vlan_idx].ports[sit.isid][0]),
                                        VTSS_PORT_BF_SIZE);
                            }
                            vlan_forbidden_table[vlan_fb_blk->forbidden[vlan_idx].vid] = vlan_stack;
                        }
                        if(vlan_forbidden_table[vlan_fb_blk->forbidden[vlan_idx].vid]  == NULL) {
                            /* Move entry from free list to used list */
                            if ((vlan_stack = list->free) == NULL)
                                break;
                            list->free = vlan_stack->next;
                            vlan_stack->next = NULL;
                            (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
                            while (switch_iter_getnext(&sit)) {
                                zero_based_isid = sit.isid - VTSS_ISID_START;
                                /* Assign The ports for 1-24 Bit Fields */
                                memcpy(&(vlan_stack->fb_port.ports[zero_based_isid][0]),
                                        &(vlan_fb_blk->forbidden[vlan_idx].ports[sit.isid][0]),
                                        VTSS_PORT_BF_SIZE);
                            }
                            vlan_forbidden_table[vlan_fb_blk->forbidden[vlan_idx].vid] = vlan_stack;
                        }
                    }
                } else {
                    vlan_fb_blk->forbidden[vlan_idx].vid = 0; /* 0 indicates invalid entry */
                } /* else of if (i < vlan_fb_blk->fb_count) */
            }
        } /* else of if (vlan_fb_blk->version == 0) */
        vlan_fb_blk->version = VLAN_FORBID_MEMBERSHIP_BLK_VERSION;
        conf_sec_close(CONF_SEC_GLOBAL, blk_fb_id);
    }
    VLAN_CRIT_EXIT();
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    if (vlan_tpid_conf_set(&tpid) != VTSS_RC_OK) {
        T_D("Setting default TPID failed\n");
    }
#endif
    if (changed && do_create) {
        if (vlan_stack_vlan_conf_set(VTSS_ISID_GLOBAL) != VTSS_RC_OK) {
            T_D("vlan_stack_vlan_conf_set Failed\n");
        }
    }

    return;
} /* End - Read/create VLAN configuration */

/* Module start */
static void vlan_start(BOOL init)
{
    u32              i;
    vlan_t           *vlan_p;
    vtss_rc          rc = VTSS_OK;
    vlan_list_t      *list;
    vlan_port_conf_t *vlan_port_conf;
    vtss_isid_t      isid;
    vtss_port_no_t   port_no;

    T_D("enter init %d", init);

    if (init) {
        memset(vlan_multi_membership_table,0,sizeof(vlan_multi_membership_table));
        memset(vlan_forbidden_table,0,sizeof(vlan_forbidden_table));
        memset(vlan.vlan_users, 0, sizeof(vlan.vlan_users));

#ifdef VTSS_SW_OPTION_VLAN_NAME
        memset(vlan.vlan_name_conf, 0, sizeof(vlan.vlan_name_conf));
        /* Set default name to VLAN 1 */
        vlan.vlan_name_conf[0].vid = 1;
        vlan.vlan_name_conf[0].name_valid = TRUE;
        strcpy(vlan.vlan_name_conf[0].vlan_name,"default");;
#endif
        /* Initialize vlan port configuration */
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            for (port_no = VTSS_PORT_NO_START; port_no < VTSS_FRONT_PORT_COUNT; port_no++) {
                vlan_port_conf = &vlan.vlan_port_conf[VLAN_USER_STATIC][isid-VTSS_ISID_START][port_no - VTSS_PORT_NO_START];
                vlan_port_default_set(vlan_port_conf);
                vlan_port_conf = &vlan.vlan_port_conf[VLAN_USER_ALL][isid-VTSS_ISID_START][port_no - VTSS_PORT_NO_START];
                vlan_port_default_set(vlan_port_conf);
                vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].mode = VLAN_PORT_MODE_ACCESS;
                vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].access_vid = VLAN_TAG_DEFAULT;
                vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].native_vid = VLAN_TAG_DEFAULT;
                vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hybrid_native_vid = VLAN_TAG_DEFAULT;
                vlan_port_default_set(&(vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].hyb_port_conf));
                vlan.port_mode_conf[isid - VTSS_ISID_START][port_no].tag_native_vlan = FALSE;
            }
        }

        /* Initialize VLAN for stack: All free */
        list = &vlan.stack_vlan;
        list->free = NULL;
        for (i = 0; i < VLAN_MAX; i++) {
            vlan_p = &vlan.vlan_stack_table[i];
            vlan_p->next = list->free;
            list->free = vlan_p;
        }

        /* Initialize message buffer pools */
        /* Essentially, we could live with one pool of two VLANs due to the two requests made in SWITCH_ADD event.
         * However, the VLAN_MSG_ID_CONF_SET_REQ is up to 500 KBytes big (depending on VLAN_MAX), so allocating two
         * of those would be overkill. Therefore, we split the pools into two: One with normal requests and one with
         * the large VLAN configuration set request.
         * For the sake of conf_xml, instead of setting the normal request pool to 1, we increase it to 10, which
         * allows for having more requests in the pipeline. At the end of the day, it's the message module's window
         * size that determines how many outstanding requests the master cna have towards a given slave.
         */
        vlan.request_pool       = msg_buf_pool_create(VTSS_MODULE_ID_VLAN, "Request",       10, sizeof(vlan_msg_req_t));
        vlan.large_request_pool = msg_buf_pool_create(VTSS_MODULE_ID_VLAN, "Large Request", 1,  sizeof(vlan_msg_large_req_t));

        /* Create semaphore for critical regions */
        critd_init(&vlan.crit, "vlan.crit", VTSS_MODULE_ID_VLAN, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        VLAN_CRIT_EXIT();
        critd_init(&vlan.cb_crit, "vlan.cb_crit", VTSS_MODULE_ID_VLAN, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        VLAN_CB_CRIT_EXIT();

    } else {
        /* Register for stack messages */
        rc = vlan_stack_register();
    }

    if (rc)
       return;
    T_D("exit");
}

/* Initialize module */
vtss_rc vlan_init(vtss_init_data_t *data)
{
    vtss_isid_t     isid = data->isid;
    vtss_rc         rc = VTSS_OK;
    switch_iter_t   sit;
    
    if (data->cmd == INIT_CMD_INIT) {
        /* Initialize and register trace ressources */
        VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&trace_reg);
    }

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        vlan_start(1);
#ifdef VTSS_SW_OPTION_VCLI
        (void) vlan_cli_req_init();
#endif
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = VLAN_icfg_init()) != VTSS_OK) {
            T_D("Calling vlan_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif
        break;
    case INIT_CMD_START:
        vlan_start(0);
        break;
    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
        } else if (isid == VTSS_ISID_GLOBAL) {
            /* Reset stack configuration */
            vlan_conf_read_stack(1);
#ifdef  VTSS_SW_OPTION_VLAN_NAME
            vlan_name_conf_read_stack(1);
#endif
            /* Reset all the units configuration in a stack */
            (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
            while (switch_iter_getnext(&sit)) {
                vlan_conf_read_switch(sit.isid);
            }
        } else if (VTSS_ISID_LEGAL(isid)) {
            /* Reset switch configuration */
        }
        break;
    case INIT_CMD_MASTER_UP:
        T_D("MASTER_UP");
        /* Read stack and switch configuration */
        vlan_conf_read_stack(0);
#ifdef  VTSS_SW_OPTION_VLAN_NAME
        vlan_name_conf_read_stack(0);
#endif
        vlan_conf_read_switch(VTSS_ISID_GLOBAL);
        break;
    case INIT_CMD_MASTER_DOWN:
        T_D("MASTER_DOWN");
        break;
    case INIT_CMD_SWITCH_ADD:
        T_D("SWITCH_ADD, isid: %d", isid);
        /* Apply all configuration to switch */
        rc = vlan_stack_port_conf_set(isid);
        if (rc != VTSS_OK) {
            T_D("vlan_stack_port_conf_set returned error %d", rc);
            return VTSS_RC_ERROR;
        }
        rc = vlan_stack_vlan_conf_set(isid);
        break;
    case INIT_CMD_SWITCH_DEL:
        T_D("SWITCH_DEL, isid: %d", isid);
        break;
    default:
        break;
    }

    T_D("exit");
    return rc;
}

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
