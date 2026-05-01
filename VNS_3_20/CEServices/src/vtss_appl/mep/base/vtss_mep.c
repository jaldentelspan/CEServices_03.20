/*

 Vitesse MEP software.

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
 
 $Id: vtss_mep.c,v 1.73 2011/03/30 15:01:22 henrikb Exp $
 $Revision: 1.73 $

*/

#include "vtss_mep_api.h"
#include "vtss_mep_supp_api.h"
#include "vtss_api.h"
#include <string.h>

/*lint -sem( vtss_mep_crit_lock, thread_lock ) */
/*lint -sem( vtss_mep_crit_unlock, thread_unlock ) */

//#define VTSS_ARCH_LUTON26 0
/****************************************************************************/
/*  MEP global variables                                                    */
/****************************************************************************/
//#undef VTSS_SW_OPTION_ACL
/****************************************************************************/
/*  MEP local variables                                                     */
/****************************************************************************/

#define     EVENT_IN_TX_APS       0x00000001
#define     EVENT_IN_CONFIG       0x00000002
#define     EVENT_IN_CC_CONFIG    0x00000004
#define     EVENT_IN_LM_CONFIG    0x00000008
#define     EVENT_IN_DM_CONFIG    0x00000010
#define     EVENT_IN_APS_CONFIG   0x00000020
#define     EVENT_IN_LT_CONFIG    0x00000040
#define     EVENT_IN_LB_CONFIG    0x00000080
#define     EVENT_IN_AIS_CONFIG   0x00000100
#define     EVENT_IN_LCK_CONFIG   0x00000200
#define     EVENT_IN_TST_CONFIG   0x00000400
#define     EVENT_IN_TST_CLEAR    0x00000800
#define     EVENT_IN_LM_CLEAR     0x00001000
#define     EVENT_IN_DM_CLEAR     0x00002000
#define     EVENT_IN_APS_INFO     0x00004000
#define     EVENT_IN_DEFECT_STATE 0x00008000
#define     EVENT_IN_SSF_STATE    0x00010000
#define     EVENT_IN_LTR_NEW      0x00020000
#define     EVENT_IN_LBR_NEW      0x00040000
#define     EVENT_IN_DMR_NEW      0x00080000
#define     EVENT_IN_1DM_NEW      0x00100000
#define     EVENT_IN_CCM_STATE    0x00200000
#define     EVENT_IN_TST_NEW      0x00400000
#define     EVENT_IN_LM_TIMER     0x00800000
#define     EVENT_IN_LT_TIMER     0x01000000
#define     EVENT_IN_LB_TIMER     0x02000000
#define     EVENT_IN_TST_TIMER    0x04000000

#define     EVENT_IN_MASK         0x0FFFFFFF

#define     EVENT_OUT_SIGNAL      0x80000000
#define     EVENT_OUT_APS         0x40000000
#define     EVENT_OUT_SF_SD       0x20000000
#define     EVENT_OUT_MASK        0xF0000000


#define MEP_LTR_MAX                 5
#define MEP_DM_TS_MCAST             0

#define UP_MEP_U_TST_ENTRY          1       /* This is number used to calculate the MCE entry ID for each Up-MEP instance (DS1076) */
#define UP_MEP_U_DM_ENTRY           2
#define UP_MEP_N_TST_ENTRY          3
#define UP_MEP_N_DM_ENTRY           4
#define UP_MEP_N_OAM_ENTRY          5
#define UP_MEP_ENTRY_MAX            6

#define UP_MEP_U_OAM_ENTRY          VTSS_MEP_INSTANCE_MAX + UP_MEP_ENTRY_MAX    /* This is the MCE entry ID for the common OAM catching IS1 on UNI */

#define UP_MEP_QOS_OAM              7
#define UP_MEP_PAG_OAM              0xFF
#define UP_MEP_PAG_PORT             0xFB

#define UP_MEP_UNI_FIRST            12      /* This must be the first Up-MEP UNI port (DS1076) */
#define UP_MEP_INGRESS_LOOP_OFFSET  4       /* This is the offset between UNI port number and ingress loop port */
#define UP_MEP_EGRESS_LOOP_OFFSET   8       /* This is the offset between UNI port number and egress loop port */


typedef struct
{
    vtss_mep_supp_defect_state_t    defect_state;
    u32                             dmr_delay[VTSS_MEP_DM_MAX];
    u32                             dmr_delay_var[VTSS_MEP_DM_MAX];
    u32                             dm1_delay_state_far_to_near[VTSS_MEP_DM_MAX];
    u32                             dm1_delay_state_var_far_to_near[VTSS_MEP_DM_MAX];
    u32                             dm1_delay_state_near_to_far[VTSS_MEP_DM_MAX];
    u32                             dm1_delay_state_var_near_to_far[VTSS_MEP_DM_MAX];       
} supp_state_t;

typedef struct
{
    u32     transaction_id;
    u32     timer;
} lt_state_t;

typedef struct
{
    u32     timer;
    u32     rx_transaction_id[VTSS_MEP_PEER_MAX];
} lb_state_t;

typedef struct
{
    BOOL    rx_count_active;
    u32     rx_frame_size;
    u32     timer;
    u32     tst_count;
} tst_state_t;

typedef struct
{
    vtss_mep_mgmt_state_t           state;
    vtss_mep_mgmt_lm_state_t        lm_state;
    vtss_mep_mgmt_lt_state_t        lt_state;
    vtss_mep_mgmt_lb_state_t        lb_state;
    vtss_mep_mgmt_dm_state_t        dmr_state;
    vtss_mep_mgmt_dm_state_t        dm1_state_far_to_near;
    vtss_mep_mgmt_dm_state_t        dm1_state_near_to_far;
    vtss_mep_mgmt_tst_state_t       tst_state;
    u8                              rx_aps[VTSS_MEP_APS_DATA_LENGTH];
} out_state_t;

typedef struct
{
    i32     raw_cnt;
    i32     buf[10];
    u8      in, out;
} los_filter_t;

typedef struct
{
    u32                         instance;
    vtss_mep_mgmt_conf_t        config;                             /* Input from management */
    vtss_mep_mgmt_cc_conf_t     cc_config;
    vtss_mep_mgmt_lm_conf_t     lm_config;
    vtss_mep_mgmt_dm_conf_t     dm_config;
    vtss_mep_mgmt_aps_conf_t    aps_config;
    vtss_mep_mgmt_lt_conf_t     lt_config;
    vtss_mep_mgmt_lb_conf_t     lb_config;
    vtss_mep_mgmt_ais_conf_t    ais_config;                        /* AIS configuration input from management */
    vtss_mep_mgmt_lck_conf_t    lck_config;                        /* LCK configuration input from management */
    vtss_mep_mgmt_tst_conf_t    tst_config;                        /* TST configuration input from management */

    supp_state_t                supp_state;                         /* Input from lower support */

    u8                          tx_aps[VTSS_MEP_APS_DATA_LENGTH];   /* input from module interface */
    BOOL                        ssf_state;
    BOOL                        raps_forward;

    out_state_t                 out_state;                          /* output state */

    u32                         event_flags;                        /* Flags used to indicate what input/output events has activated the 'run' thread */

    lt_state_t                  lt_state;                           /* Internal state */
    lb_state_t                  lb_state;
    tst_state_t                 tst_state;
    u32                         flr_interval_count;
    los_filter_t                near_los_filter;
    los_filter_t                far_los_filter;
    i32                         near_los_counter;
    i32                         far_los_counter;
    u32                         near_tx_counter;
    u32                         far_tx_counter;
    u32                         dmr_rec_count;
    u32                         dmr_next_prt;
    u32                         dmr_sum;
    u32                         dmr_var_sum;
    u32                         dmr_n_sum;
    u32                         dmr_n_var_sum;
    u32                         dm1_rec_count_far_to_near;
    u32                         dm1_next_prt_far_to_near;
    u32                         dm1_sum_far_to_near;
    u32                         dm1_var_sum_far_to_near;
    u32                         dm1_n_sum_far_to_near;
    u32                         dm1_n_var_sum_far_to_near; 
    u32                         dm1_rec_count_near_to_far;
    u32                         dm1_next_prt_near_to_far;
    u32                         dm1_sum_near_to_far;
    u32                         dm1_var_sum_near_to_far;
    u32                         dm1_n_sum_near_to_far;
    u32                         dm1_n_var_sum_near_to_far;             

    u32                         rule_vid;                           /* The VID of rules for this flow - used for create/delete of PDU capture/forward rules */
    BOOL                        rule_ports[VTSS_PORT_ARRAY_SIZE];   /* This ports are used to create capture/forward rules */
    BOOL                        update_rule;                        /* something has changed for this instance that requires update of rules */
    BOOL                        tunnel;                             /* This MEP is related to a tunnel EVC */
    BOOL                        up_mep;                             /* This MEP is an Up-MEP according to DS1076 */
    u32                         up_mep_pop_cnt;                     /* This is the pop count value used to strip EVC tag('s) on ingress NNI */
    u32                         up_mep_e_vid;                       /* EVC outer VID - port classification VID - needed to create Up-MEP MCE (key VID) */
    u32                         ace_count;                          /* Last counter value from HW CCM ACE rule */
} mep_instance_data_t;


static mep_instance_data_t  instance_data[VTSS_MEP_INSTANCE_MAX];
static u32                  vlan_raps_acl_id[4096];
static u32                  vlan_raps_mac_octet[4096];
static u32                  timer_res;
static u32                  lm_timer=0;
static u32                  update_rule_timer=0;
static u8 all_zero_mac[VTSS_MEP_MAC_LENGTH] = {0,0,0,0,0,0};
static u8 raps_mac[VTSS_MEP_MAC_LENGTH] = {0x01,0x19,0xA7,0x00,0x00,0x01};
static u8 class1_multicast[8][VTSS_MEP_MAC_LENGTH] = {{0x01,0x80,0xC2,0x00,0x00,0x30},
                                                      {0x01,0x80,0xC2,0x00,0x00,0x31},
                                                      {0x01,0x80,0xC2,0x00,0x00,0x32},
                                                      {0x01,0x80,0xC2,0x00,0x00,0x33},
                                                      {0x01,0x80,0xC2,0x00,0x00,0x34},
                                                      {0x01,0x80,0xC2,0x00,0x00,0x35},
                                                      {0x01,0x80,0xC2,0x00,0x00,0x36},
                                                      {0x01,0x80,0xC2,0x00,0x00,0x37}};

#if defined(VTSS_SW_OPTION_UP_MEP)
static BOOL                 up_mep_enabled = FALSE;   /* (DS1076) */
#endif

#if defined(VTSS_ARCH_LUTON26)
typedef struct
{
    u32 cnt;        /* Number of ACL rules for this level range */
    u8 level[4];    /* There can be a max of four ACL rules implementing a level range */
    u8 mask[4];     /* Mask is pointing on significant bits in level - '0' indicate 'don't care' */
} level_table_t;
                                           /* level 0-0  */
static level_table_t level_table[8][8] = {{{1,{0,0,0,0},{7,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}},
                                           /* level 1-0                level 1-1  */
                                          {{1,{0,0,0,0},{6,0,0,0}}, {1,{1,0,0,0},{7,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}},
                                           /* level 2-0                level 2-1                level 2-2  */
                                          {{2,{2,0,0,0},{7,6,0,0}}, {2,{2,1,0,0},{7,7,0,0}}, {1,{2,0,0,0},{7,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}},
                                           /* level 3-0                level 3-1                level 3-2                level 3-3  */
                                          {{1,{0,0,0,0},{4,0,0,0}}, {2,{2,1,0,0},{6,7,0,0}}, {1,{2,0,0,0},{6,0,0,0}}, {1,{3,0,0,0},{7,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}},
                                           /* level 4-0                level 4-1                level 4-2                level 4-3                level 4-4  */
                                          {{2,{4,0,0,0},{7,4,0,0}}, {3,{4,2,1,0},{7,6,7,0}}, {2,{4,2,0,0},{7,6,0,0}}, {2,{4,3,0,0},{7,7,0,0}}, {1,{4,0,0,0},{7,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}},
                                           /* level 5-0                level 5-1                level 5-2                level 5-3                level 5-4                level 5-5  */
                                          {{2,{4,0,0,0},{6,4,0,0}}, {3,{4,2,1,0},{6,6,7,0}}, {2,{4,2,0,0},{6,6,0,0}}, {2,{4,3,0,0},{6,7,0,0}}, {1,{4,0,0,0},{6,0,0,0}}, {1,{5,0,0,0},{7,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}},
                                           /* level 6-0                level 6-1                level 6-2                level 6-3                level 6-4                level 6-5                level 6-6  */
                                          {{3,{6,4,0,0},{7,6,4,0}}, {4,{6,4,2,1},{7,6,6,7}}, {3,{6,4,2,0},{7,6,6,0}}, {3,{6,4,3,0},{7,6,7,0}}, {2,{6,4,0,0},{7,6,0,0}}, {2,{6,5,0,0},{7,7,0,0}}, {1,{6,0,0,0},{7,0,0,0}}, {0,{0,0,0,0},{0,0,0,0}}},
                                           /* level 7-0                level 7-1                level 7-2                level 7-3                level 7-4                level 7-5                level 7-6                level 7-7 */
                                          {{1,{0,0,0,0},{0,0,0,0}}, {3,{4,2,1,0},{4,6,7,0}}, {2,{4,2,0,0},{4,6,0,0}}, {2,{4,3,0,0},{4,7,0,0}}, {1,{4,0,0,0},{4,0,0,0}}, {2,{6,5,0,0},{6,7,0,0}}, {1,{6,0,0,0},{6,0,0,0}}, {1,{7,0,0,0},{7,0,0,0}}}};
#endif



/****************************************************************************/
/*  MEP local functions                                                     */
/****************************************************************************/
static void run_ssf_state(u32 instance);
static void run_cc_config(u32 instance);
static void run_lm_config(u32 instance);
static void run_aps_config(u32 instance);
static void run_ais_config(u32 instance);
static void run_lck_config(u32 instance);
static vtss_mep_supp_direction_t supp_direction_calc(vtss_mep_mgmt_direction_t  direction);
static vtss_mep_supp_period_t supp_period_calc(vtss_mep_mgmt_period_t period);


static void instance_data_clear(u32 instance,  mep_instance_data_t *data)
{
    u32     rule_vid;                           /* The VID of rules for this flow - used for create/delete of PDU capture/forward rules */
    BOOL    rule_ports[VTSS_PORT_ARRAY_SIZE];

    rule_vid = data->rule_vid;
    memcpy(rule_ports, data->rule_ports, sizeof(rule_ports));

    memset(data, 0, sizeof(mep_instance_data_t));

    strncpy(data->config.name, "VITESS", VTSS_MEP_MEG_CODE_LENGTH);
    strncpy(data->config.meg, "meg000", VTSS_MEP_MEG_CODE_LENGTH);
    data->cc_config.period = VTSS_MEP_MGMT_PERIOD_1S;
    data->lm_config.period = VTSS_MEP_MGMT_PERIOD_1S;
    data->lm_config.flr_interval = 5;
    data->dm_config.way = VTSS_MEP_MGMT_TWO_WAY;
    data->dm_config.gap = 10;
    data->dm_config.count = 10;
    data->aps_config.type = VTSS_MEP_MGMT_L_APS;
    data->lb_config.to_send = 10;
    data->lb_config.size = 100;
    data->lb_config.interval = 10;
    data->lt_config.ttl = 1;
    data->ais_config.period = VTSS_MEP_MGMT_PERIOD_1S;
    data->ais_config.client_domain = VTSS_MEP_MGMT_EVC;
    data->lck_config.period = VTSS_MEP_MGMT_PERIOD_1S;
    data->lck_config.client_domain = VTSS_MEP_MGMT_EVC;
    data->tst_config.rate = 1;
    data->tst_config.size = 64;

    data->instance = instance;
    data->rule_vid = rule_vid;
    memcpy(data->rule_ports, rule_ports, sizeof(data->rule_ports));
}


#if defined(VTSS_ARCH_CARACAL)
#define mce_id_calc(instance, entry) (instance*6)+entry
static BOOL up_mep_update(u32 vid,  BOOL *ports,  mep_instance_data_t *inst)
{/* This is IS1 entry creation for Up-MEP according to DS1076 */
    u32                  i;
    vtss_mce_t           mce;
    vtss_mep_acl_entry   acl;

    /* Delete existing mce */
    for (i=1; i<UP_MEP_ENTRY_MAX; ++i)
        if (vtss_mce_del(NULL, mce_id_calc(inst->instance, i)) != VTSS_RC_OK)   continue;

    if (vtss_mce_init(NULL, &mce) != VTSS_RC_OK)    return(FALSE);

   /* IS1 Loop UNI */
    mce.id = mce_id_calc(inst->instance, UP_MEP_U_TST_ENTRY);
    mce.key.port_list[inst->config.port + UP_MEP_INGRESS_LOOP_OFFSET] = TRUE;
    mce.key.vid.value = vid;
    mce.key.vid.mask = 0x0FFF;
    mce.key.data.mask[1] = 0xFF;
    mce.key.data.value[1] = 37; /* TST PDU type */
    mce.action.pop_cnt = 1;
    mce.action.prio_enable = TRUE;
    mce.action.prio = inst->config.evc_qos;
    mce.action.policy_no = inst->config.evc_pag;
    if (vtss_mce_add(NULL, UP_MEP_U_OAM_ENTRY, &mce) != VTSS_RC_OK)    return(FALSE);

    mce.id = mce_id_calc(inst->instance, UP_MEP_U_DM_ENTRY);
    mce.key.data.mask[1] = 0xFC;
    mce.key.data.value[1] = 0x2C; /* DM PDU type */
    mce.action.policy_no = UP_MEP_PAG_OAM;
    if (vtss_mce_add(NULL, UP_MEP_U_OAM_ENTRY, &mce) != VTSS_RC_OK)    return(FALSE);

    /* IS1 NNI */
    mce.id = mce_id_calc(inst->instance, UP_MEP_N_TST_ENTRY);
    memcpy(mce.key.port_list, inst->rule_ports, sizeof(mce.key.port_list));
    mce.key.vid.value = inst->up_mep_e_vid;
    mce.key.data.mask[0] = 0xE0;
    mce.key.data.value[0] = inst->config.level<<5;  /* Only this level */
    mce.key.data.mask[1] = 0xFF;
    mce.key.data.value[1] = 37; /* TST PDU type */
    mce.action.pop_cnt = inst->up_mep_pop_cnt;
    mce.action.prio_enable = TRUE;
    mce.action.prio = inst->config.evc_qos;
    mce.action.policy_no = UP_MEP_PAG_PORT + (inst->config.port - UP_MEP_UNI_FIRST);
    mce.action.vid = vid;
    if (vtss_mce_add(NULL, UP_MEP_U_OAM_ENTRY, &mce) != VTSS_RC_OK)    return(FALSE);

    mce.id = mce_id_calc(inst->instance, UP_MEP_N_DM_ENTRY);
    mce.key.data.mask[1] = 0xFC;
    mce.key.data.value[1] = 0x2C; /* DM PDU type */
    if (vtss_mce_add(NULL, UP_MEP_U_OAM_ENTRY, &mce) != VTSS_RC_OK)    return(FALSE);

    mce.id = mce_id_calc(inst->instance, UP_MEP_N_OAM_ENTRY);
    mce.key.data.mask[1] = 0x00; /* OAM PDU type */
    mce.action.prio = UP_MEP_QOS_OAM;
    if (vtss_mce_add(NULL, UP_MEP_U_OAM_ENTRY, &mce) != VTSS_RC_OK)    return(FALSE);

    /* IS2 */
    memset(&acl, 0, sizeof(acl));
    acl.vid = vid;
    acl.oam = VTSS_OAM_ANY_TYPE;
    acl.cpu = TRUE;
    acl.ts = FALSE;
    acl.hw_ccm = FALSE;
    memcpy(acl.ing_port, inst->rule_ports, sizeof(acl.ing_port));
    for (i=0; i<level_table[inst->config.level-1][0].cnt; ++i)
    {/* Create the set of ACL rules that capture OAM on lower levels */
        acl.level = level_table[inst->config.level-1][0].level[i];
        acl.mask = level_table[inst->config.level-1][0].mask[i];
    
        if (!vtss_mep_acl_add(&acl))    return(FALSE);
    }

    return(TRUE);
}
#endif


#if (defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1)) && defined(VTSS_SW_OPTION_ACL) && defined(VTSS_FEATURE_ACL_V2)
#define ACL_LIST_MAX        64
#define OAM_LTM_TYPE        05
#define OAM_CCM_TYPE        01

typedef struct
{
    u32   oam;
    u32   level;
//    u8    mac[VTSS_MEP_MAC_LENGTH];
    BOOL  ing_port[VTSS_PORT_ARRAY_SIZE];
    BOOL  eg_port[VTSS_PORT_ARRAY_SIZE];
    BOOL  cpu;
    BOOL  hw_ccm;
    BOOL  range;
    u32   seq;
} acl_logic_rule_t;
static acl_logic_rule_t acl_logic_list[ACL_LIST_MAX];
static u32 acl_logic_cnt;

static BOOL acl_logical_rule_list(u32 vid,  BOOL *ports,  mep_instance_data_t **inst,  u32 inst_cnt)
{
    u32     i;
    i32     lvl_cnt;
    BOOL    mip_ing, mip_eg, mep_ing, mep_eg;

    BOOL    mep_blk[VTSS_PORT_ARRAY_SIZE];
    BOOL    mep_fwr[VTSS_PORT_ARRAY_SIZE];

    BOOL    mip_blk[VTSS_PORT_ARRAY_SIZE];
    BOOL    mip_fwr_ing[VTSS_PORT_ARRAY_SIZE];
    BOOL    mip_fwr_eg[VTSS_PORT_ARRAY_SIZE];

    BOOL    hw_ccm[VTSS_PORT_ARRAY_SIZE];
//    u8      hw_ccm_mac[VTSS_PORT_ARRAY_SIZE][VTSS_MEP_MAC_LENGTH];
    BOOL    all_zero_port[VTSS_PORT_ARRAY_SIZE];

    if (vid == 0)   return(FALSE);

    acl_logic_cnt = 0;
    memset(acl_logic_list, 0, sizeof(acl_logic_list));

    memset(mep_blk, FALSE, sizeof(mep_blk));    /* Default MEP is not blocking any ports */
    memcpy(mep_fwr, ports, sizeof(mep_fwr));    /* Default MEP is forwarding all ports */
    memset(all_zero_port, FALSE, sizeof(all_zero_port));

    mep_ing = mep_eg = FALSE;
    for (lvl_cnt=7; lvl_cnt>=0; --lvl_cnt)
    {/* Run through all levels from the top */
        mip_ing = mip_eg = FALSE;

        memset(hw_ccm, FALSE, sizeof(hw_ccm));                  /* On this level no HW CCM on any ports */
        memcpy(mip_blk, mep_blk, sizeof(mip_blk));              /* On this level MIP is blocking any ports that MEP is on this level and above */
        memcpy(mip_fwr_ing, mep_fwr, sizeof(mip_fwr_ing));      /* On this level MIP is forwarding from any ports that MEP is on this level and above */
        memcpy(mip_fwr_eg, mep_fwr, sizeof(mip_fwr_eg));        /* On this level MIP is forwarding to any ports that MEP is on this level and above */

        for (i=0; i<inst_cnt; ++i)
        {/* Run through all instances in this flow to find MIP on this level */
            if ((inst[i]->config.level == lvl_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MIP))
            {/* A MIP on this level */
                if (inst[i]->config.direction == VTSS_MEP_MGMT_INGRESS)
                {/* Ingress */
                    mip_ing = TRUE;
                    mip_blk[inst[i]->config.port] = TRUE;       /* Ingress MIP is blocking LTM on this port */
                    mip_fwr_ing[inst[i]->config.port] = FALSE;  /* Ingress MIP is not forwarding LTM from this port */
                }
                if (inst[i]->config.direction == VTSS_MEP_MGMT_EGRESS)
                {/* Egress */
                    mip_eg = TRUE;
                    mip_fwr_eg[inst[i]->config.port] = FALSE;  /* Egress MIP is not forwarding LTM to this port */
                }
            }
        }
        for (i=0; i<inst_cnt; ++i)
        {/* Run through all instances in this flow to find MEP on this level */
            if ((inst[i]->config.level == lvl_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MEP))
            {
                if (inst[i]->config.direction == VTSS_MEP_MGMT_INGRESS)
                {
                    mep_ing = TRUE;
                    if (inst[i]->cc_config.enable && vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(inst[i]->cc_config.period)))
                    {/* Set HW CCM on this port if period demanding this */
//                        hw_ccm[inst[i]->config.port] = TRUE;
                    }
                }
                if (inst[i]->config.direction == VTSS_MEP_MGMT_EGRESS)
                    mep_eg = TRUE;

                if ((inst[i]->config.domain != VTSS_MEP_MGMT_EVC) || ports[inst[i]->config.port])    /* For now - block rules are not made on EVC UNI ports (UNI is not in 'ports' array )*/
                    mep_blk[inst[i]->config.port] = TRUE;       /* Ingress/Egress MEP is blocking any PDU on this port */
                mep_fwr[inst[i]->config.port] = FALSE;          /* Ingress/Egress MEP is not forwarding any PDU to/from this port */
                mip_blk[inst[i]->config.port] = TRUE;           /* Ingress/Egress MEP is blocking LTM on this port */
                mip_fwr_ing[inst[i]->config.port] = FALSE;      /* Ingress/Egress MEP is not forwarding LTM from this port */
                mip_fwr_eg[inst[i]->config.port] = FALSE;       /* Ingress/Egress MEP is not forwarding LTM to this port */
            }
        }

        if (mip_ing)
        {   /* Create ingress MIP block rule */
            acl_logic_list[acl_logic_cnt].oam = OAM_LTM_TYPE;
            acl_logic_list[acl_logic_cnt].level = lvl_cnt;
            memcpy(acl_logic_list[acl_logic_cnt].ing_port, mip_blk, sizeof(acl_logic_list[acl_logic_cnt].ing_port));
            acl_logic_list[acl_logic_cnt].cpu = TRUE;
            acl_logic_list[acl_logic_cnt].range = FALSE;
            acl_logic_list[acl_logic_cnt].seq = 0;
            if ((++acl_logic_cnt) >= ACL_LIST_MAX)    return(FALSE);
        }
        if (mip_eg)
        {   /* Create egress MIP forward rule */
            acl_logic_list[acl_logic_cnt].oam = OAM_LTM_TYPE;
            acl_logic_list[acl_logic_cnt].level = lvl_cnt;
            memcpy(acl_logic_list[acl_logic_cnt].ing_port, mip_fwr_ing, sizeof(acl_logic_list[acl_logic_cnt].ing_port));
            memcpy(acl_logic_list[acl_logic_cnt].eg_port, mip_fwr_eg, sizeof(acl_logic_list[acl_logic_cnt].eg_port));
            acl_logic_list[acl_logic_cnt].cpu = TRUE;
            acl_logic_list[acl_logic_cnt].range = FALSE;
            acl_logic_list[acl_logic_cnt].seq = 1;
            if ((++acl_logic_cnt) >= ACL_LIST_MAX)    return(FALSE);
        }
        if (mep_ing)
        {/* Create ingress MEP HW CCM rules */
            for (i=0; i<VTSS_PORT_ARRAY_SIZE; ++i)
            {/* HW CCM on all ports */
                if (hw_ccm[i])
                {/* HW CCM must be created on this port */
                    acl_logic_list[acl_logic_cnt].oam = OAM_CCM_TYPE;
                    acl_logic_list[acl_logic_cnt].level = lvl_cnt;
                    acl_logic_list[acl_logic_cnt].ing_port[i] = TRUE;
                    acl_logic_list[acl_logic_cnt].cpu = FALSE;      /* Hit Me Once must be used */
                    acl_logic_list[acl_logic_cnt].range = FALSE;
                    acl_logic_list[acl_logic_cnt].hw_ccm = TRUE;
//                    memcpy(acl_logic_list[acl_logic_cnt].mac, hw_ccm_mac[i], VTSS_MEP_MAC_LENGTH);
                    acl_logic_list[acl_logic_cnt].seq = 2;
                    if ((++acl_logic_cnt) >= ACL_LIST_MAX)    return(FALSE);
                }
            }
        }
        if (mep_ing || mep_eg)
        {   /* Create MEP block rule */
            acl_logic_list[acl_logic_cnt].oam = VTSS_OAM_ANY_TYPE;
            acl_logic_list[acl_logic_cnt].level = lvl_cnt;
            memcpy(acl_logic_list[acl_logic_cnt].ing_port, mep_blk, sizeof(acl_logic_list[acl_logic_cnt].ing_port));
            memset(acl_logic_list[acl_logic_cnt].eg_port, FALSE, sizeof(acl_logic_list[acl_logic_cnt].eg_port));
            acl_logic_list[acl_logic_cnt].cpu = (mep_ing);
            acl_logic_list[acl_logic_cnt].range = TRUE;
            acl_logic_list[acl_logic_cnt].seq = 3;
            if (memcmp(mep_blk, all_zero_port, sizeof(mep_blk))) /* Don't create a block rule where ingress ports are all zero */
                if ((++acl_logic_cnt) >= ACL_LIST_MAX)    return(FALSE);

            /* Create MEP forward rule */
            acl_logic_list[acl_logic_cnt].oam = VTSS_OAM_ANY_TYPE;
            acl_logic_list[acl_logic_cnt].level = lvl_cnt;
            memcpy(acl_logic_list[acl_logic_cnt].ing_port, mep_fwr, sizeof(acl_logic_list[acl_logic_cnt].ing_port));
            memcpy(acl_logic_list[acl_logic_cnt].eg_port, mep_fwr, sizeof(acl_logic_list[acl_logic_cnt].eg_port));
            acl_logic_list[acl_logic_cnt].cpu = (mep_eg);
            acl_logic_list[acl_logic_cnt].range = TRUE;
            acl_logic_list[acl_logic_cnt].seq = 4;
            if (memcmp(mep_fwr, all_zero_port, sizeof(mep_fwr))) /* Don't create a forward rule where forward ports are all zero */
                if ((++acl_logic_cnt) >= ACL_LIST_MAX)    return(FALSE);
        }
    }
    return(TRUE);
}


#if defined(VTSS_ARCH_LUTON26)
static BOOL acl_luton26_rule_list(u32  vid)
{
    u32                  i, seq, inx, first_level, last_level, oam;
    BOOL                 ing_port[VTSS_PORT_ARRAY_SIZE];
    BOOL                 eg_port[VTSS_PORT_ARRAY_SIZE];
    BOOL                 cpu, hw_ccm;
//    u8                   mac[VTSS_MEP_MAC_LENGTH];
    vtss_mep_acl_entry   acl;

    if (!acl_logic_cnt)     return(TRUE);

    /* On LUTON 26 - logical rules covering a range of levels can (in most cases) be implemented by fewer ACL rules */
    /* The logical range can be implemented as a set of ACL rules using don't cares in level - mask is pointing on significant bits */

    for (seq=0; seq<5; ++seq)
    {/* In a sequence there is max one logical rule on each level - the sequence number is also giving the order ACL rules are created */
        inx = 0;
        do
        {/* Keep on until all logical rules in this sequence are transformed to ACL rules */
            for (inx=inx; inx<acl_logic_cnt; ++inx)     if (acl_logic_list[inx].seq == seq)     break;  /* Search for first rule in the level range (if it is a range) */
    
            if (inx<acl_logic_cnt)
            {/* A rule was found */
                first_level = acl_logic_list[inx].level;    /* First level in range is this level */
                last_level = first_level;                   /* So far last level in range is also this level */
                cpu = acl_logic_list[inx].cpu;              /* Save this information for creation of ACL rules */
                oam = acl_logic_list[inx].oam;
                hw_ccm = acl_logic_list[inx].hw_ccm;
//                memcpy(mac, acl_logic_list[inx].mac, VTSS_MEP_MAC_LENGTH);
                memcpy(ing_port, acl_logic_list[inx].ing_port, sizeof(ing_port));
                memcpy(eg_port, acl_logic_list[inx].eg_port, sizeof(eg_port));

                if (acl_logic_list[inx].range)
                {/* This rule is part of a level range */
                    for (inx=inx+1; inx<acl_logic_cnt; ++inx)   /* Search for end of level range - or end of logical list */
                    {
                        if (acl_logic_list[inx].seq != seq)     continue;
                        if ((oam == acl_logic_list[inx].oam) && !memcmp(acl_logic_list[inx].ing_port, ing_port, sizeof(ing_port)) && !memcmp(acl_logic_list[inx].eg_port, eg_port, sizeof(eg_port)))
                        {/* This rule is in the same range */
                            cpu = cpu || acl_logic_list[inx].cpu;    /* Copy to CPU is an OR of all rules in the range */
                            last_level = acl_logic_list[inx].level;  /* New last level in range is the level of this rule */
                        }
                        else    break;  /* A rule is found that is not part of this level range (start of next range) - index is now pointing on next logical rule */
                    }
                }
                else    inx++;  /* This rule is a single rule - index must point on next logical rule */
    
                /* Create new ACL rules for this level range */
                acl.vid = vid;
                acl.oam = oam;
                acl.cpu = cpu;
                acl.ts = TRUE;
                acl.hw_ccm = hw_ccm;
//                memcpy(acl.mac, mac, VTSS_MEP_MAC_LENGTH);
                memcpy(acl.ing_port, ing_port, sizeof(ing_port));
                memcpy(acl.eg_port, eg_port, sizeof(eg_port));

                for (i=0; i<level_table[first_level][last_level].cnt; ++i)
                {/* Create the set of ACL rules */
                    acl.level = level_table[first_level][last_level].level[i];
                    acl.mask = level_table[first_level][last_level].mask[i];
    
                    if (!vtss_mep_acl_add(&acl))    return(FALSE);
                }
            }
        } while(inx<acl_logic_cnt);
    }

    return(TRUE);
}
#endif /* VTSS_ARCH_LUTON26 */

#ifdef VTSS_ARCH_JAGUAR_1
static BOOL acl_jaguar_rule_list(u32  vid)
{
    u32                  seq, inx, inx_save, level, oam;
    BOOL                 ing_port[VTSS_PORT_ARRAY_SIZE];
    BOOL                 eg_port[VTSS_PORT_ARRAY_SIZE];
    BOOL                 cpu, hw_ccm;
    vtss_mep_acl_entry   acl;

    if (!acl_logic_cnt)     return(TRUE);

    /* On JAGUAR - a logical rules on a level can be implemented by setting that level bit in a ACL rule */
    /* The logical range can be implemented as one ACL rule with the level bit set representing each of the leves in the range */
    /* More logical rules does not need to be in a unbroken range in order to share ACL rule as levels are indicated bit wise */

    for (seq=0; seq<5; ++seq)
    {/* In a sequence there is max one logical rule on each level - the sequence number is also giving the order ACL rules are created */
        inx = 0;
        do
        {/* Keep on until all logical rules in this sequence are transformed to ACL rules */
            for (inx=inx; inx<acl_logic_cnt; ++inx)     if (acl_logic_list[inx].seq == seq)     break;  /* Search for next first rule in this sequence */
    
            if (inx<acl_logic_cnt)
            {/* A rule was found */
                level = 1<<acl_logic_list[inx].level;       /* Level of this logical rule is saved */
                cpu = acl_logic_list[inx].cpu;              /* Save this information for creation of ACL rules */
                oam = acl_logic_list[inx].oam;
                hw_ccm = acl_logic_list[inx].hw_ccm;
                memcpy(ing_port, acl_logic_list[inx].ing_port, sizeof(ing_port));
                memcpy(eg_port, acl_logic_list[inx].eg_port, sizeof(eg_port));
    

                if (acl_logic_list[inx].hw_ccm)
                {/* This rule is a HW CCM - can not be part of a shared ACL */
                    inx++;  /* Continue from next logical rule */
                }
                else
                {/* This rule can be part of a shared ACL */
                    inx_save = inx;
                    for (inx=inx+1; inx<acl_logic_cnt; ++inx)   /* Search for next rule that can be part of this shared ACL - or end of logical list */
                    {
                        if (acl_logic_list[inx].seq != seq)     continue;
                        if ((oam == acl_logic_list[inx].oam) && !memcmp(acl_logic_list[inx].ing_port, ing_port, sizeof(ing_port)) && !memcmp(acl_logic_list[inx].ing_port, ing_port, sizeof(ing_port)))
                        {/* This logical rule can be part of shared ACL */
                            cpu = cpu || acl_logic_list[inx].cpu;    /* Copy to CPU is an OR of all rules that share ACL */
                            level |= 1<<acl_logic_list[inx].level;   /* This level is also covered by this shared ACL */
                            acl_logic_list[inx].seq = 0xFF;          /* Dont create more ACL on this logical rule */
                        }
                        else
                        /* This logical rule can not be part of shared ACL - check for end of range */
                            if (acl_logic_list[inx_save].range)
                                break;  /* A rule is found that is not part of this level range (start of next range) - index is now pointing on start of next range */
                    }
                    if (!acl_logic_list[inx].range)
                        inx = ++inx_save;  /* This is not a range so there might be some logical rules that was 'jumped over' (those that could not share ACL) - so continue from old next */
                }

                /* Create new ACL rule for this level range */
                acl.vid = vid;
                acl.oam = oam;
                acl.cpu = cpu;
                acl.ts = TRUE;
                acl.hw_ccm = hw_ccm;
                acl.level = level;
                acl.mask = 0;
                memcpy(acl.ing_port, ing_port, sizeof(ing_port));
                memcpy(acl.eg_port, eg_port, sizeof(eg_port));

                if (!vtss_mep_acl_add(&acl))    return(FALSE);
            }
        } while(inx<acl_logic_cnt);
    }

    return(TRUE);
}
#endif /* VTSS_ARCH_JAGUAR_1 */

static BOOL acl_list_update(u32 vid,  BOOL *ports,  mep_instance_data_t **inst,  u32 inst_cnt)
{
    vlan_raps_acl_id[vid] = 0xFFFFFFFF;   /* This VID no longer got an ACL rule */

    if (!acl_logical_rule_list(vid, ports, inst, inst_cnt))   return(FALSE);
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    if (!acl_luton26_rule_list(vid))   return(FALSE);
#endif
#ifdef VTSS_ARCH_JAGUAR_1
    if (!acl_jaguar_rule_list(vid))   return(FALSE);
#endif
    return(TRUE);
}
#endif /* VTSS_ARCH_LUTON26 || VTSS_ARCH_JAGUAR_1 */

#if defined(VTSS_ARCH_LUTON28) || defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26)
/* Initialize MAC address entry */
static void mep_mac_init(vtss_mac_table_entry_t *entry)
{
    entry->locked = 1;
    entry->aged = 0;
    entry->copy_to_cpu = 1;
#if defined(VTSS_FEATURE_MAC_CPU_QUEUE)
    entry->cpu_queue = vtss_mep_oam_cpu_queue_get();
#endif /* VTSS_FEATURE_MAC_CPU_QUEUE */
}
#endif

#if defined(VTSS_ARCH_LUTON28) || !defined(VTSS_SW_OPTION_ACL) || !defined(VTSS_FEATURE_ACL_V2)
static u8  ccm_mac[6] = {0x01,0x80,0xC2,0x00,0x00,0x30};

static BOOL mac_table_update(u32 vid,  BOOL *ports,  mep_instance_data_t **inst,  u32 inst_cnt)
{
    vtss_rc                 ret_val=0, ret=0;
    u32                     i, j, port_cnt;
    i32                     lvl_cnt;
    BOOL                    class_port[VTSS_PORT_ARRAY_SIZE];
    BOOL                    tmp_port[VTSS_PORT_ARRAY_SIZE];
    BOOL                    mep_found, mip_found, ingress_mep;
    vtss_mac_table_entry_t  entry;

    /* Initialize entry parameters */
    entry.vid_mac.vid = vid;
    memcpy(entry.vid_mac.mac.addr, ccm_mac, 6);
    memset(entry.destination, 0, sizeof(entry.destination));
    mep_mac_init(&entry);

    mep_found = FALSE;
    mip_found = FALSE;
    ingress_mep = FALSE;
    memcpy(class_port, ports, sizeof(class_port));
    memset(tmp_port, 0, sizeof(tmp_port));

    for (lvl_cnt=7; lvl_cnt>=0; --lvl_cnt)
    {/* Run through all levels from the top */
        entry.vid_mac.mac.addr[5] &= 0xF0;            /* Multicast Class 1 and level */
        entry.vid_mac.mac.addr[5] |= lvl_cnt & 0x07;

        if (!mep_found)
        {/* MEP not found in this VID yet */
            for (i=0; i<inst_cnt; ++i)    if ((inst[i]->config.level == lvl_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MEP))  break;
            mep_found = (i<inst_cnt);
        }
        if (!ingress_mep)
        {/* Ingress MEP not found in this VID yet */
            for (i=0; i<inst_cnt; ++i)    if ((inst[i]->config.level == lvl_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MEP) && (inst[i]->config.direction == VTSS_MEP_MGMT_INGRESS))  break;
            if (i<inst_cnt)
            {   /* Ingress MEP found - means no forwarding on this level and down - on any ports */
                ingress_mep = TRUE;
                memset(class_port, 0, sizeof(class_port));
                memset(tmp_port, 0, sizeof(tmp_port));
            }
        }

        if (!mep_found) /* Check for delete of MAC entry */
        {/* MEP not found in this VID yet */
            ret += vtss_mac_table_del(NULL, &entry.vid_mac);
            for (i=0; i<inst_cnt; ++i)    if ((inst[i]->config.level == lvl_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MIP))  break;
            mip_found = (i<inst_cnt);
            if (!mip_found)
            {/* No MIP either */
                entry.vid_mac.mac.addr[5] |= 0x08;            /* Multicast Class 2 */
                ret += vtss_mac_table_del(NULL, &entry.vid_mac);
            }
        }

        if (mep_found || mip_found)
        { /* MEP is found on this VID or MIP is found on this level */
            if (!ingress_mep)
            {/* ingress MEP not found yet - calculate the port mask for MAC entry */
                memcpy(tmp_port, class_port, sizeof(tmp_port));
                for (i=0; i<inst_cnt; ++i)
                {
                    if ((inst[i]->config.level == lvl_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MEP) && (inst[i]->config.direction == VTSS_MEP_MGMT_EGRESS))
                    {/* Egress MEP found on this level - don't forward class 1 and class 2 to this port on this level and down */
                        class_port[inst[i]->config.port] = tmp_port[inst[i]->config.port] = FALSE;
                        for (j=0, port_cnt=0; j<VTSS_PORT_ARRAY_SIZE; ++j)    if (class_port[j])  port_cnt++;
                        if (port_cnt<2)
                        { /* Only forward if more that one port 'in front' when egress MEP */
                            memset(class_port, 0, sizeof(class_port));
                            memset(tmp_port, 0, sizeof(tmp_port));
                        }
                    }
                    if ((inst[i]->config.level == lvl_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MIP) && (inst[i]->config.direction == VTSS_MEP_MGMT_INGRESS))
                    /* Inress MIP found on this level - don't forward class 2 to ports behind on this level */
                        for (port_cnt=0; port_cnt<VTSS_PORT_ARRAY_SIZE; ++port_cnt)    if (inst[i]->config.port != port_cnt)  tmp_port[port_cnt] = FALSE;
                    if ((inst[i]->config.level == lvl_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MIP) && (inst[i]->config.direction == VTSS_MEP_MGMT_EGRESS))
                    /* Egress MIP found on this level - don't forward class 2 to this port on this level */
                        tmp_port[inst[i]->config.port] = FALSE;
                }
            }

            if (mep_found)
            { /* Multicast Class 1 is only for MEP */
                for (i=0; i<inst_cnt; ++i)
                    if ((inst[i]->config.level == lvl_cnt) && inst[i]->cc_config.enable && vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(inst[i]->cc_config.period)))  break;    /* If any MEP has HW CCM on this level - no copy to CPU */
                if (i<inst_cnt)     entry.copy_to_cpu = 0;
                memcpy(entry.destination, class_port, sizeof(entry.destination));
                ret_val += vtss_mac_table_add(NULL, &entry);
                entry.copy_to_cpu = 1;
            }

            entry.vid_mac.mac.addr[5] |= 0x08;            /* Multicast Class 2 */
            memcpy(entry.destination, tmp_port, sizeof(entry.destination));
            ret_val += vtss_mac_table_add(NULL, &entry);
        }
    } /* for to */

    if (ret != VTSS_RC_OK)      vtss_mep_trace("mac_table_update: Return value is not OK", vid, 0, 0, 0);
    return((ret_val == VTSS_RC_OK));
}
#endif

static void raps_forwarding_control(u32 vid)
{
    BOOL tmp_port[VTSS_PORT_ARRAY_SIZE];
    u32  i, j, port_cnt, forward, voe_cnt, voe[2], octet=01;

    if (vid == 0)       return;

    memset(tmp_port, 0, sizeof(tmp_port));
    memset(voe, 0, sizeof(voe));

    /* Count how many MEP instances that has RAPS enabled and check if forwardig is enabled */
    for (i=0, port_cnt=0, voe_cnt=0, forward=0; i<VTSS_MEP_INSTANCE_MAX; ++i)   
    {
        if (!port_cnt)  octet = instance_data[i].aps_config.raps_octet;
        if (instance_data[i].config.enable && instance_data[i].aps_config.enable && (instance_data[i].rule_vid == vid) &&
            (instance_data[i].aps_config.type == VTSS_MEP_MGMT_R_APS) && (octet == instance_data[i].aps_config.raps_octet))
        {/* RAPS is enabled in this VID for this instance */
            port_cnt++;
            if (instance_data[i].raps_forward)      forward++;
            if (instance_data[i].config.voe) {
                if (voe_cnt < 2)   voe[voe_cnt] = i;
                voe_cnt++;
            }
            for (j=0; j<VTSS_PORT_ARRAY_SIZE; ++j)  if (instance_data[i].rule_ports[j])     tmp_port[j] = TRUE;
        }
    }

#if (defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)) && defined(VTSS_SW_OPTION_ACL) && defined(VTSS_FEATURE_ACL_V2)
    if (voe_cnt == 1)   /* Only one VOE with RAPS - disable forwarding */
        (void)vtss_mep_supp_raps_forwarding(voe[0], FALSE);
    else if (voe_cnt == 2) { /* Two VOE with RAPS - enable forwarding if both has forwarding active */
        (void)vtss_mep_supp_raps_forwarding(voe[0], (forward == 2) && (port_cnt == 2));
        (void)vtss_mep_supp_raps_forwarding(voe[1], (forward == 2) && (port_cnt == 2));
    }
    else if (voe_cnt == 0) { /* No VOE with RAPS - RAPS is controlled using ACL rules */
        if (vlan_raps_acl_id[vid] != 0xFFFFFFFF)
        {/* This VID got an RAPS ACL rule - delete it */
            vtss_mep_acl_raps_del(vlan_raps_acl_id[vid]);
            vlan_raps_acl_id[vid] = 0xFFFFFFFF;
        }
    
        if ((port_cnt == 2) && (forward == 2))
        {
            raps_mac[5] = octet;
            vtss_mep_acl_raps_add(vid,  tmp_port,  raps_mac,  &vlan_raps_acl_id[vid]);         /* No RAPS ACL rule on this VID yet - create one */
        }
    }

#if defined(VTSS_ARCH_SERVAL)
    /* On Serval the port MEP VLAN is only created when two MEP has enabled ERPS */
    (void)vtss_mep_vlan_member(vid, tmp_port, (port_cnt==2));
#endif
#else
    vtss_mac_table_entry_t    entry;
    memcpy(entry.vid_mac.mac.addr, raps_mac, sizeof(entry.vid_mac.mac.addr));
    entry.vid_mac.vid = vid;
    mep_mac_init(&entry);

    if (vlan_raps_mac_octet[vid] != 0xFFFFFFFF)
    {/* This VID has a MAC entry - delete entry */
        entry.vid_mac.mac.addr[5] = vlan_raps_mac_octet[vid];
        vlan_raps_mac_octet[vid] = 0xFFFFFFFF;
        if (vtss_mac_table_del(NULL, &entry.vid_mac) != VTSS_RC_OK)    return;
    }

    if (port_cnt)
    {/* At least one MEP has RAPS enabled */
        entry.vid_mac.mac.addr[5] = octet;
        if ((port_cnt == 2) && (forward == 2))  memcpy(entry.destination, tmp_port, sizeof(entry.destination));  /* Two MEP with RAPS enabled and forwarding enabled - add entry with active destination ports */
        else                                    memset(entry.destination, 0, sizeof(entry.destination));         /* Otherwise no active destination ports */
        if (vtss_mac_table_add(NULL, &entry) != VTSS_RC_OK)    return;
        vlan_raps_mac_octet[vid] = octet;
    }
#endif
}


#if defined(VTSS_ARCH_LUTON28) || defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26)
static BOOL pdu_fwr_cap_update(u32 vid,  BOOL *ports)
{
    mep_instance_data_t     *inst[VTSS_MEP_INSTANCE_MAX];
    mep_instance_data_t     *up_mep=NULL;
    u32                     i, port_cnt, inst_cnt;
    vtss_mac_table_entry_t  entry;
    vtss_rc                 ret_val=0, ret=0;
    BOOL                    tmp_port[VTSS_PORT_ARRAY_SIZE];
//printf("pdu_fwr_cap_update  vid %lu  ports %u-%u-%u-%u-%u-%u-%u-%u-%u-%u-%u-%u\n", vid, ports[0], ports[1], ports[2], ports[3], ports[4], ports[5], ports[6], ports[7], ports[8], ports[9], ports[10], ports[11]);
    if (vid == 0)   return(FALSE);

    memset(inst, 0, sizeof(inst));

    for (inst_cnt=0, i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
    {/* Search for relevant instances on this VID */
        if (instance_data[i].config.enable && (instance_data[i].rule_vid == vid)) {
            if (!instance_data[i].up_mep)   inst[inst_cnt++] = &instance_data[i];
            if (instance_data[i].up_mep)    up_mep = &instance_data[i];
        }
    }

#if defined(VTSS_ARCH_LUTON28) || !defined(VTSS_SW_OPTION_ACL) || !defined(VTSS_FEATURE_ACL_V2)
    if (!mac_table_update(vid,  ports,  inst,  inst_cnt))     return(FALSE);
#if defined(VTSS_ARCH_JAGUAR_1) && !defined(VTSS_FEATURE_ACL_V2)
    vtss_mep_acl_del(vid);                /* All ACL rules in this VID is deleted */
/********************************************************
THIS IS TEMPORARY IN ORDER TO GET DM PDU TIMESTAMPTING
*********************************************************/
    BOOL level[8];
    memset(level, 0, sizeof(level));
    for (i=0; i<inst_cnt; ++i)
        if (!level[inst[i]->config.level] && (inst[i]->config.mode == VTSS_MEP_MGMT_MEP))
        {
            level[inst[i]->config.level] = TRUE;
            vtss_mep_acl_dm_add(inst[i]->config.domain,  vid,  inst[i]->config.evc_pag,  inst[i]->config.level);
        }
#endif
#endif

#if (defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1)) && defined(VTSS_SW_OPTION_ACL) && defined(VTSS_FEATURE_ACL_V2)
    vtss_mep_acl_del(vid);                /* All ACL rules (IS2) in this VID is deleted */

    if (inst_cnt)
        if (!acl_list_update(vid,  ports,  inst,  inst_cnt))     return(FALSE);
#endif
#ifdef VTSS_ARCH_LUTON26
    if (up_mep) /* This MEP is an Up-MEP according to DS1076 */
        if (!up_mep_update(vid, ports, up_mep))     return(FALSE);
#endif
    for (i=0; i<inst_cnt; ++i) { /* Add ACL rules for HW CCM and TST PDU reception */
        if (inst[i]->config.mode == VTSS_MEP_MGMT_MIP) continue;
        memset(tmp_port, 0, sizeof(tmp_port));
        if (inst[i]->config.direction == VTSS_MEP_MGMT_INGRESS)
            tmp_port[inst[i]->config.port] = TRUE;
        else
        if (inst[i]->config.direction == VTSS_MEP_MGMT_EGRESS)
        {
            memcpy(tmp_port, inst[i]->rule_ports, VTSS_PORT_ARRAY_SIZE);
            tmp_port[inst[i]->config.port] = FALSE;
        }
        if (inst[i]->cc_config.enable && vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(inst[i]->cc_config.period)))  /* HW based CCM is enabled and a ACL rule need to be added for this */
            vtss_mep_acl_ccm_add(inst[i]->instance,  inst[i]->rule_vid,  tmp_port,  inst[i]->config.level,  inst[i]->config.peer_mac[0], inst[i]->config.evc_pag);
        vtss_mep_acl_tst_add(inst[i]->instance,  inst[i]->rule_vid,  tmp_port,  inst[i]->config.level, inst[i]->config.evc_pag);
    }

    if (up_mep)
        if (inst[i]->cc_config.enable && vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(inst[i]->cc_config.period)))
        { /* HW based CCM is enabled and a ACL rule need to be added for this */
            memset(tmp_port, 0, sizeof(tmp_port));
            tmp_port[up_mep->config.port + UP_MEP_EGRESS_LOOP_OFFSET] = TRUE;
            vtss_mep_acl_ccm_add(up_mep->instance,  up_mep->rule_vid,  tmp_port,  up_mep->config.level,  up_mep->config.peer_mac[0], 0);
        }

    entry.vid_mac.vid = vid;
    memset(entry.destination, 0, sizeof(entry.destination));
    mep_mac_init(&entry);

    for (port_cnt=0; port_cnt<VTSS_PORTS; ++port_cnt)
    {   /* If any MEP/MIP is created resident on this port it needs a unicast MAC entry with this port MAC adresse */
        vtss_mep_mac_get(port_cnt, entry.vid_mac.mac.addr);
        for (i=0; i<inst_cnt; ++i)
#if (defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1)) && defined(VTSS_SW_OPTION_ACL) && defined(VTSS_FEATURE_ACL_V2)
            if ((inst[i]->config.port == port_cnt) && (inst[i]->config.mode == VTSS_MEP_MGMT_MIP))  break;       /* On Luton26 and Jaguar a MEP does not need unicast MAC entry as this is handled by the ACL rule - A MIP does need as the ACL rule is for LTM only */
#else
            if (inst[i]->config.port == port_cnt)  break;
#endif
        if (i<inst_cnt)            ret_val += vtss_mac_table_add(NULL, &entry);
        else                       ret += vtss_mac_table_del(NULL, &entry.vid_mac);
    }

    memset(tmp_port, 0, sizeof(tmp_port));
    for (i=0; i<inst_cnt; ++i)
    {
        /* A tagged port MEP need a port that is member of that VLAN in order to assure reception of frames if filtering is enabled */
        if (inst[i]->config.domain == VTSS_MEP_MGMT_PORT)
            tmp_port[inst[i]->config.port] = TRUE;
    }
    if (up_mep)
    {
        /* A DS1076 Up-MEP need a egress loop port that is member of that VLAN in order to assure forwarding to this port */
        tmp_port[up_mep->config.port + UP_MEP_EGRESS_LOOP_OFFSET] = TRUE;
    }
    if (!vtss_mep_vlan_member(vid, tmp_port, (port_cnt!=0)))     return(FALSE);     /* Ports need to be member of VLAN */

    raps_forwarding_control(vid);   /* Add rules for RAPS capture an forwardin */

    if (ret != VTSS_RC_OK)      vtss_mep_trace("pdu_fwr_cap_update: Return value is not OK", vid, 0, 0, 0);

    return((ret_val == VTSS_RC_OK));
}
#endif

#ifdef VTSS_ARCH_SERVAL
static BOOL pdu_fwr_cap_update(u32 vid,  BOOL *ports)
{
    mep_instance_data_t  *data;
    u32   i, j;
    BOOL  tmp_port[VTSS_PORT_ARRAY_SIZE];
    BOOL  mip_uni[VTSS_PORT_ARRAY_SIZE];
    BOOL  mip_nni[VTSS_PORT_ARRAY_SIZE];
    BOOL  mep;

//printf("pdu_fwr_cap_update  vid %u  ports %u-%u-%u-%u-%u-%u-%u-%u-%u-%u-%u\n", vid, ports[0], ports[1], ports[2], ports[3], ports[4], ports[5], ports[6], ports[7], ports[8], ports[9], ports[10]);
    if (vid == 0)   return(FALSE);

    memset(tmp_port, 0, sizeof(tmp_port));
    memset(mip_uni, 0, sizeof(mip_uni));
    memset(mip_nni, 0, sizeof(mip_nni));

    mep = FALSE;

    for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
    {/* Search for all EVC SW (not VOE based) MEP/MIP */
        data = &instance_data[i];

        if (data->config.enable && !data->config.voe) {   /* An enabled SW (IS2) based MEP/MIP */
            if (data->rule_vid == vid)  vtss_mep_acl_del(i);   /* All ACL rules (IS2) for this instance is deleted */
    
            memset(tmp_port, 0, sizeof(tmp_port));
            if (data->config.direction == VTSS_MEP_MGMT_INGRESS)
                tmp_port[data->config.port] = TRUE;
            else
            if (data->config.direction == VTSS_MEP_MGMT_EGRESS)
            {
                memcpy(tmp_port, data->rule_ports, sizeof(tmp_port));
                tmp_port[data->config.port] = FALSE;
            }
    
            if (data->config.domain == VTSS_MEP_MGMT_PORT) {   /* Port MEP */
                if ((data->config.mode == VTSS_MEP_MGMT_MEP) && (data->rule_vid == vid)) {
                    /* Add ACL rules for HW CCM and TST PDU reception */
                    if (data->cc_config.enable && vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(data->cc_config.period)))  /* HW based CCM is enabled and a ACL rule need to be added for this */
                        vtss_mep_acl_ccm_add(data->instance,  vid,  tmp_port,  data->config.level,  data->config.peer_mac[0], VTSS_MEP_SUPP_MEP_PAG);
                    vtss_mep_acl_tst_add(data->instance,  vid,  tmp_port,  data->config.level, VTSS_MEP_SUPP_MEP_PAG);
                }
            }

            if (data->config.domain == VTSS_MEP_MGMT_EVC) {   /* EVC MEP/MIP */
                if ((data->config.mode == VTSS_MEP_MGMT_MEP) && (data->rule_vid == vid)) {
                    /* Add ACL rules for HW CCM and TST PDU reception */
                    if (data->cc_config.enable && vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(data->cc_config.period)))  /* HW based CCM is enabled and a ACL rule need to be added for this */
                        vtss_mep_acl_ccm_add(data->instance,  vtss_mep_supp_rx_isdx_get(i),  tmp_port,  data->config.level,  data->config.peer_mac[0], VTSS_MEP_SUPP_MEP_PAG);
                    vtss_mep_acl_tst_add(data->instance,  vtss_mep_supp_rx_isdx_get(i),  tmp_port,  data->config.level, VTSS_MEP_SUPP_MEP_PAG);
                }
                if ((data->config.mode == VTSS_MEP_MGMT_MIP) && (data->config.direction == VTSS_MEP_MGMT_INGRESS)) {
                    mip_uni[data->config.port] = TRUE;  /* An ingress MIP is enabled on this UNI */
                    if (data->rule_vid == vid)
                        /* Add ACL rules for UNI LBM PDU reception */
                        vtss_mep_acl_mip_lbm_add(data->instance, data->config.port, tmp_port, VTSS_MEP_SUPP_MIP_PAG, TRUE);
                }
                if ((data->config.mode == VTSS_MEP_MGMT_MIP) && (data->config.direction == VTSS_MEP_MGMT_EGRESS)) {
                    for (j=0; j<VTSS_PORT_ARRAY_SIZE; ++j)
                        if (data->rule_ports[j]) 
                            mip_nni[j] = TRUE;  /* An egress MIP is enabled on this NNI */
                    if (data->rule_vid == vid)
                        /* Add ACL rules for NNI LBM PDU reception */
                        vtss_mep_acl_mip_lbm_add(data->instance, data->config.port, tmp_port, VTSS_MEP_SUPP_MIP_PAG, FALSE);
                }
            }
        }

        if (data->config.enable && (data->config.mode == VTSS_MEP_MGMT_MEP) && !data->config.voe)
            mep = TRUE;

        if (!data->config.enable && (data->config.domain == VTSS_MEP_MGMT_EVC) && !data->config.voe)   /* All not enabled SW (IS2) based EVC MEP/MIP must be deleted */
            if (data->rule_vid == vid)  vtss_mep_acl_del(i);   /* All ACL rules (IS2) for this instance is deleted */
    }

    /* ADD ACL rules common for all MEP/MIP instances */
    vtss_mep_acl_evc_add(mep, mip_nni, mip_uni, VTSS_MEP_SUPP_MEP_PAG, VTSS_MEP_SUPP_MIP_PAG);

    raps_forwarding_control(vid);   /* Add rules for RAPS capture and forwarding */

    return(TRUE);
}
#endif

static vtss_mep_supp_period_t supp_period_calc(vtss_mep_mgmt_period_t period)
{
    switch (period)
    {
        case VTSS_MEP_MGMT_PERIOD_300S:    return(VTSS_MEP_SUPP_PERIOD_300S);
        case VTSS_MEP_MGMT_PERIOD_100S:    return(VTSS_MEP_SUPP_PERIOD_100S);
        case VTSS_MEP_MGMT_PERIOD_10S:     return(VTSS_MEP_SUPP_PERIOD_10S);
        case VTSS_MEP_MGMT_PERIOD_1S:      return(VTSS_MEP_SUPP_PERIOD_1S);
        case VTSS_MEP_MGMT_PERIOD_6M:      return(VTSS_MEP_SUPP_PERIOD_6M);
        case VTSS_MEP_MGMT_PERIOD_1M:      return(VTSS_MEP_SUPP_PERIOD_1M);
        case VTSS_MEP_MGMT_PERIOD_6H:      return(VTSS_MEP_SUPP_PERIOD_6H);
        default:                           return(VTSS_MEP_SUPP_PERIOD_10S);
    }
}


static vtss_mep_supp_pattern_t supp_pattern_calc(vtss_mep_mgmt_pattern_t pattern)
{
    switch (pattern)
    {
        case VTSS_MEP_MGMT_PATTERN_ALL_ZERO:    return(VTSS_MEP_SUPP_PATTERN_ALL_ZERO);
        case VTSS_MEP_MGMT_PATTERN_ALL_ONE:     return(VTSS_MEP_SUPP_PATTERN_ALL_ONE);
        case VTSS_MEP_MGMT_PATTERN_0XAA:        return(VTSS_MEP_SUPP_PATTERN_0XAA);
        default:                                return(VTSS_MEP_SUPP_PATTERN_ALL_ZERO);
    }
}


static vtss_mep_supp_aps_type_t supp_aps_calc(vtss_mep_mgmt_aps_type_t aps_type)
{
    switch (aps_type)
    {
        case VTSS_MEP_MGMT_L_APS:    return(VTSS_MEP_SUPP_L_APS);
        case VTSS_MEP_MGMT_R_APS:    return(VTSS_MEP_SUPP_R_APS);
        default:                     return(VTSS_MEP_SUPP_L_APS);
    }
}


static void consequent_action_calc_and_deliver(mep_instance_data_t *data)
{
    u32  rc = VTSS_MEP_SUPP_RC_OK, i;
    BOOL old_aTsf, old_aBlk, dLoc=FALSE;

    old_aTsf = data->out_state.state.aTsf;
    old_aBlk = data->out_state.state.aBlk;

    for (i=0; i<data->config.peer_count; ++i)
        dLoc = dLoc || data->supp_state.defect_state.dLoc[i];

    data->out_state.state.aTsf = (dLoc && data->cc_config.enable) || (data->supp_state.defect_state.dAis && !data->cc_config.enable) ||
                                 data->supp_state.defect_state.dLevel || data->supp_state.defect_state.dMeg || data->supp_state.defect_state.dMep || data->ssf_state;

    data->out_state.state.aBlk = data->supp_state.defect_state.dLevel || data->supp_state.defect_state.dMeg || data->supp_state.defect_state.dMep;

    if (old_aTsf != data->out_state.state.aTsf)
    {/* aTsf has changed */
        data->event_flags |= EVENT_OUT_SF_SD;   /* Deliver SF and SD out */

        rc = vtss_mep_supp_ccm_rdi_set(data->instance,   data->out_state.state.aTsf);   /* Control RDI */

        if (data->ais_config.enable)    rc += vtss_mep_supp_ais_set(data->instance, data->out_state.state.aTsf);  /* Control AIS */
    }

    if (old_aBlk != data->out_state.state.aBlk)
    {/* aBlk has changed */
    }

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("tsf_calc_and_deliver: CCM RDI set failed", data->instance, 0, 0, 0);
}


static void fault_cause_calc(mep_instance_data_t *data)
{
    u32 i;

    data->out_state.state.cLevel = data->supp_state.defect_state.dLevel;
    data->out_state.state.cMeg = data->supp_state.defect_state.dMeg;
    data->out_state.state.cMep = data->supp_state.defect_state.dMep;

    for (i=0; i<VTSS_MEP_PEER_MAX; ++i)
    {
        data->out_state.state.cPeriod[i] = data->supp_state.defect_state.dPeriod[i];
        data->out_state.state.cPrio[i] = data->supp_state.defect_state.dPrio[i];
        data->out_state.state.cRdi[i] = data->supp_state.defect_state.dRdi[i] && data->cc_config.enable;
        data->out_state.state.cLoc[i] = data->supp_state.defect_state.dLoc[i] && !data->supp_state.defect_state.dAis && !data->supp_state.defect_state.dLck && !data->ssf_state && data->cc_config.enable;
    }

    data->out_state.state.cSsf = data->ssf_state || data->supp_state.defect_state.dAis;
    data->out_state.state.cAis = data->supp_state.defect_state.dAis;
    data->out_state.state.cLck = data->supp_state.defect_state.dLck && !data->supp_state.defect_state.dAis;
}


static vtss_mep_supp_mode_t supp_mode_calc(vtss_mep_mgmt_mode_t  mode)
{
    switch (mode)
    {
        case VTSS_MEP_MGMT_MEP: return(VTSS_MEP_SUPP_MEP);
        case VTSS_MEP_MGMT_MIP: return(VTSS_MEP_SUPP_MIP);
        default:                return(VTSS_MEP_SUPP_MEP);
    }
}

static vtss_mep_supp_direction_t supp_direction_calc(vtss_mep_mgmt_direction_t  direction)
{
    switch (direction)
    {
        case VTSS_MEP_MGMT_INGRESS: return(VTSS_MEP_SUPP_DOWN);
        case VTSS_MEP_MGMT_EGRESS:  return(VTSS_MEP_SUPP_UP);
        default:                    return(VTSS_MEP_SUPP_DOWN);
    }
}

static vtss_mep_supp_format_t supp_format_calc(vtss_mep_mgmt_format_t  format)
{
    switch (format)
    {
        case VTSS_MEP_MGMT_ITU_ICC:   return(VTSS_MEP_SUPP_ITU_ICC);
        case VTSS_MEP_MGMT_IEEE_STR:  return(VTSS_MEP_SUPP_IEEE_STR);
        default:                      return(VTSS_MEP_SUPP_ITU_ICC);
    }
}

static vtss_mep_mgmt_mode_t mode_calc(vtss_mep_supp_mode_t  mode)
{
    switch (mode)
    {
        case VTSS_MEP_SUPP_MEP: return(VTSS_MEP_MGMT_MEP);
        case VTSS_MEP_SUPP_MIP: return(VTSS_MEP_MGMT_MIP);
        default:                return(VTSS_MEP_MGMT_MEP);
    }
}

static vtss_mep_mgmt_direction_t direction_calc(vtss_mep_supp_direction_t  direction)
{
    switch (direction)
    {
        case VTSS_MEP_SUPP_DOWN: return(VTSS_MEP_MGMT_INGRESS);
        case VTSS_MEP_SUPP_UP:   return(VTSS_MEP_MGMT_EGRESS);
        default:                 return(VTSS_MEP_MGMT_INGRESS);
    }
}

static BOOL flow_info_calc(mep_instance_data_t      *data,
                           vtss_mep_supp_flow_t     *const info,
                           BOOL                     *mep_port)
{
    u32   pvid, vport=0;
    vtss_mep_tag_type_t ptype;
    BOOL  nni[VTSS_PORT_ARRAY_SIZE];

    memset(info, 0, sizeof(vtss_mep_supp_flow_t));
    memset(data->rule_ports, FALSE, VTSS_PORT_ARRAY_SIZE);
    memset(mep_port, FALSE, VTSS_PORT_ARRAY_SIZE);
    data->rule_vid = 0;

    if (data->config.domain == VTSS_MEP_MGMT_PORT)
    {
        if (data->config.direction == VTSS_MEP_MGMT_INGRESS)
        {
            info->port[data->config.flow] = TRUE;       /* This is a Port domain MEP - flow instance number is a flow port */
            vport = data->config.flow;
            if (vtss_mep_vlan_get(data->config.vid, nni))    memcpy(data->rule_ports, nni, VTSS_PORT_ARRAY_SIZE); /* If there is a VLAN on this VID all NNI must be a rule_port */
            else                                             data->rule_ports[data->config.flow] = TRUE;
            mep_port[data->config.flow] = TRUE;
        }
        if (data->config.direction == VTSS_MEP_MGMT_EGRESS)
        {
            if (!vtss_mep_vlan_get(data->config.vid, nni))    return (FALSE);
            memcpy(info->port, nni, VTSS_PORT_ARRAY_SIZE);
            info->port[data->config.port] = FALSE;      /* This is egress MEP - All but residence port is a flow port */
            memcpy(data->rule_ports, nni, VTSS_PORT_ARRAY_SIZE);
            for (vport=0; vport<VTSS_PORT_ARRAY_SIZE; ++vport)  if (info->port[vport])   break;    /* Assume that all NNI ports has same VLAN configuration */
            memcpy(mep_port, info->port, VTSS_PORT_ARRAY_SIZE);
        }

        if (!vtss_mep_pvid_get(vport, &pvid, &ptype))    return (FALSE);

        if (data->config.vid != 0)        info->mask = VTSS_MEP_SUPP_FLOW_MASK_PORT | (((ptype==VTSS_MEP_TAG_S) || (ptype==VTSS_MEP_TAG_S_CUSTOM)) ? VTSS_MEP_SUPP_FLOW_MASK_SOUTVID : VTSS_MEP_SUPP_FLOW_MASK_COUTVID);
        else                              info->mask = VTSS_MEP_SUPP_FLOW_MASK_PORT;
        info->out_vid = data->config.vid;

        if (data->config.vid != 0)        data->rule_vid = data->config.vid;    /* This is a tagged port MEP - VID is the classified VID */
        else                              data->rule_vid = pvid;                /* This is a un tagged port MEP - VID is the PVID */

        info->vid = data->rule_vid;    /* Classified VID */
    }

#if defined(VTSS_SW_OPTION_EVC)
    u32   vid, evid, tvid, tag_cnt;
    vtss_mep_tag_type_t  ttype;
    BOOL  uni[VTSS_PORT_ARRAY_SIZE];

    if (data->config.domain == VTSS_MEP_MGMT_EVC)
    {   /* Get flow info from EVC */
        if (vtss_mep_evc_flow_info_get(data->config.flow, nni, uni, &vid, &evid, &ttype, &tvid, &data->tunnel, &tag_cnt))
        {
            info->mask = VTSS_MEP_SUPP_FLOW_MASK_EVC_ID;
            info->evc_id = data->config.flow;

            if (data->config.direction == VTSS_MEP_MGMT_INGRESS)
            {
                vport = data->config.port;
                info->port[data->config.port] = TRUE;                       /* This is ingress MEP - only residence port is a flow port */
                mep_port[data->config.port] = TRUE;
            }
            if (data->config.direction == VTSS_MEP_MGMT_EGRESS)
            {
                memcpy(info->port, nni, VTSS_PORT_ARRAY_SIZE);
                info->port[data->config.port] = FALSE;      /* This is egress MEP - All but residence port is a flow port */
                for (vport=0; vport<VTSS_PORT_ARRAY_SIZE; ++vport)  if (info->port[vport])   break;    /* Assume that all NNI ports has same VLAN configuration */
                memcpy(mep_port, info->port, VTSS_PORT_ARRAY_SIZE);
                
                if (data->up_mep)
                {/* This is a DS1076 Up-MEP - the ingress loop port is the flow port in mep_supp */
                    memset(info->port, FALSE, VTSS_PORT_ARRAY_SIZE);
                    info->port[data->config.port + UP_MEP_INGRESS_LOOP_OFFSET] = TRUE;
                    data->up_mep_pop_cnt = tag_cnt;
                    data->up_mep_e_vid = evid;
                    vport = data->config.port + UP_MEP_INGRESS_LOOP_OFFSET;
                }
            }

            if (!vtss_mep_pvid_get(vport, &pvid, &ptype))    return (FALSE);

            info->mask |= VTSS_MEP_SUPP_FLOW_MASK_PORT | (((ptype==VTSS_MEP_TAG_S) || (ptype==VTSS_MEP_TAG_S_CUSTOM)) ? VTSS_MEP_SUPP_FLOW_MASK_SOUTVID : VTSS_MEP_SUPP_FLOW_MASK_COUTVID);
            if (data->up_mep) /* This is a DS1076 Up-MEP - frames on loop ports are with classified VID */
                info->out_vid = vid;
            else {
                if (data->tunnel) { /* This is a tunnel MEP - both innner and outer tag */
                    info->mask |= (((ttype==VTSS_MEP_TAG_S) || (ttype==VTSS_MEP_TAG_S_CUSTOM)) ? VTSS_MEP_SUPP_FLOW_MASK_SINVID : VTSS_MEP_SUPP_FLOW_MASK_CINVID);
                    info->out_vid = tvid;
                    info->in_vid = evid;
                }
                else {
                    if ((data->config.mode == VTSS_MEP_MGMT_MIP) && (data->config.direction == VTSS_MEP_MGMT_INGRESS) && (data->config.vid != 0))   /* Subscriber Down-MIP on UNI */
                        info->out_vid = data->config.vid;
                    else {
                        info->out_vid = evid;

                        if (data->config.vid != 0)
                        {   /* Extra inner vid in case of Subscriber Up-MIP */
                            info->mask |= VTSS_MEP_SUPP_FLOW_MASK_CINVID;
                            info->in_vid = data->config.vid;
                        }
                    }
                }
            }

            data->rule_vid = vid;
            memcpy(data->rule_ports, nni, VTSS_PORT_ARRAY_SIZE);

            info->vid = data->rule_vid;    /* Classified VID */
#ifdef VTSS_ARCH_SERVAL
            u32 i;
            if ((data->config.mode == VTSS_MEP_MGMT_MEP) && (data->config.voe)) {   /* This is a VOE based EVC MEP */
                for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
                    if ((i != data->instance) && (instance_data[i].config.enable) && (instance_data[i].config.domain == VTSS_MEP_MGMT_EVC) &&
                        (instance_data[i].config.mode == VTSS_MEP_MGMT_MEP) && (instance_data[i].config.flow == data->config.flow) && (instance_data[i].config.voe) &&
                        (instance_data[i].config.level < data->config.level)) {
                    /* Another VOE based EVC MEP at a lower level in this flow - this VOE based MEP must point to that VOE based MEP */
                        info->mask |= VTSS_MEP_SUPP_FLOW_MASK_PATH_P;
                        info->path_mep = instance_data[i].instance;
                    }
            }
#endif
        }
        else
        {
            vtss_mep_trace("flow_info_calc: EVC is deleted", 0, 0, 0, 0);
            data->rule_vid = 0;
        }
    }
#endif

    return (data->rule_vid != 0);
}


static BOOL client_flow_info_calc(mep_instance_data_t      *data,
                                  vtss_mep_mgmt_domain_t   client_domain,
                                  u32                      client_flow_count,
                                  u32                      client_flows[VTSS_MEP_CLIENT_FLOWS_MAX],
                                  vtss_mep_supp_flow_t     *const info,
                                  u32                      *count)
{
    u32   vid;
    vtss_mep_tag_type_t ptype;
#if defined(VTSS_SW_OPTION_EVC)
    u32   i, evid, tvid, tag_cnt;
    vtss_mep_tag_type_t  ttype;
    BOOL  tunnel, nni[VTSS_PORT_ARRAY_SIZE];
    BOOL  uni[VTSS_PORT_ARRAY_SIZE];
#endif
    *count = 0;

    if (!vtss_mep_pvid_get(data->config.port, &vid, &ptype))    return (FALSE);

    if (data->config.domain == VTSS_MEP_MGMT_PORT)
    {
#if 0
        if (client_domain == VTSS_MEP_MGMT_PORT)
        {
            if (!client_flow_count)
            {
                if (data->config.vid)   vid = data->config.vid;
                if (vtss_mep_vlan_get(vid, nni))
                {
                    nni[data->config.port] = FALSE;
               
                    memcpy(info[0].port, nni, VTSS_PORT_ARRAY_SIZE);
                    if (data->config.vid != 0)   info[0].mask = VTSS_MEP_SUPP_FLOW_MASK_PORT | (((ptype==VTSS_MEP_TAG_S) || (ptype==VTSS_MEP_TAG_S_CUSTOM)) ? VTSS_MEP_SUPP_FLOW_MASK_SOUTVID : VTSS_MEP_SUPP_FLOW_MASK_COUTVID);
                    else                         info[0].mask = VTSS_MEP_SUPP_FLOW_MASK_PORT;
                    info[0].out_vid = data->config.vid;
                    *count = 1;
                }
            }
            else
            {
                for(i=0; i<client_flow_count; ++i)
                {
                    if (vtss_mep_vlan_get(client_flows[i], nni))
                    {
                        nni[data->config.port] = FALSE;
                   
                        memcpy(info[*count].port, nni, VTSS_PORT_ARRAY_SIZE);
                        info[*count].mask = VTSS_MEP_SUPP_FLOW_MASK_PORT | (((ptype==VTSS_MEP_TAG_S) || (ptype==VTSS_MEP_TAG_S_CUSTOM)) ? VTSS_MEP_SUPP_FLOW_MASK_SOUTVID : VTSS_MEP_SUPP_FLOW_MASK_COUTVID);
                        info[*count].out_vid = client_flows[i];
                        (*count)++;
                    }
                }
            }
        }
#endif
#if defined(VTSS_SW_OPTION_EVC)
        if (client_domain == VTSS_MEP_MGMT_EVC)
        {
            for(i=0; i<client_flow_count; ++i)
            {
                if (vtss_mep_evc_flow_info_get(client_flows[i], nni, uni, &vid, &evid, &ttype, &tvid, &tunnel, &tag_cnt))
                {
                    memcpy(info[*count].port, nni, VTSS_PORT_ARRAY_SIZE);
                    info[*count].evc_id = client_flows[i];
                    info[*count].mask = VTSS_MEP_SUPP_FLOW_MASK_EVC_ID | VTSS_MEP_SUPP_FLOW_MASK_PORT | (((ptype==VTSS_MEP_TAG_S) || (ptype==VTSS_MEP_TAG_S_CUSTOM)) ? VTSS_MEP_SUPP_FLOW_MASK_SOUTVID : VTSS_MEP_SUPP_FLOW_MASK_COUTVID);
                    info[*count].out_vid = evid;
                    (*count)++;
                }
            }
        }
#endif
    }

#if defined(VTSS_SW_OPTION_EVC)
    if (data->config.domain == VTSS_MEP_MGMT_EVC)
    {   /* Get flow info from EVC */
        if (vtss_mep_evc_flow_info_get(data->config.flow, nni, uni, &vid, &evid, &ttype, &tvid, &tunnel, &tag_cnt))
        {
            memcpy(info[0].port, nni, VTSS_PORT_ARRAY_SIZE);
            info[0].evc_id = data->config.flow;
            info[0].mask = VTSS_MEP_SUPP_FLOW_MASK_EVC_ID | VTSS_MEP_SUPP_FLOW_MASK_PORT | (((ptype==VTSS_MEP_TAG_S) || (ptype==VTSS_MEP_TAG_S_CUSTOM)) ? VTSS_MEP_SUPP_FLOW_MASK_SOUTVID : VTSS_MEP_SUPP_FLOW_MASK_COUTVID);
            info[0].out_vid = evid;
            *count = 1;
        }
    }
#endif

    return (TRUE);
}


static void peer_mac_update(mep_instance_data_t *data)
{
    u32 i;
    u8  peer_mac[VTSS_MEP_SUPP_PEER_MAX][VTSS_MEP_SUPP_MAC_LENGTH];

    if (vtss_mep_supp_learned_mac_get(data->instance, peer_mac) != VTSS_MEP_SUPP_RC_OK)     {vtss_mep_trace("peer_mac_update: Call to supp failed", data->instance, 0, 0, 0); return;};

    for (i=0; i<data->config.peer_count; ++i)
        if (memcmp(all_zero_mac, peer_mac[i], VTSS_MEP_MAC_LENGTH)) {
        /* Not all zero mac - use this learned MAC for configuration */
            memcpy(data->config.peer_mac[i], peer_mac[i], VTSS_MEP_MAC_LENGTH);
        }
}


static void ccm_conf_calc(mep_instance_data_t *data,   vtss_mep_supp_ccm_conf_t *ccm_conf)
{
    ccm_conf->period = supp_period_calc((data->cc_config.enable) ? data->cc_config.period : data->lm_config.period);
    ccm_conf->prio = (data->cc_config.enable) ? data->cc_config.prio : data->lm_config.prio;
    ccm_conf->dei = FALSE;
    ccm_conf->format = supp_format_calc(data->config.format);
    memcpy(ccm_conf->name, data->config.name, VTSS_MEP_SUPP_MEG_CODE_LENGTH);
    memcpy(ccm_conf->meg, data->config.meg, VTSS_MEP_SUPP_MEG_CODE_LENGTH);
    ccm_conf->mep = data->config.mep;
    ccm_conf->peer_count = data->config.peer_count;
    memcpy(ccm_conf->peer_mep, data->config.peer_mep, sizeof(*ccm_conf->peer_mep)*VTSS_MEP_SUPP_PEER_MAX);
    if (data->lm_config.enable && (data->lm_config.ended == VTSS_MEP_MGMT_DUAL_ENDED) && (data->lm_config.cast == VTSS_MEP_MGMT_UNICAST))
        memcpy(ccm_conf->dmac, data->config.peer_mac[0], VTSS_MEP_SUPP_MAC_LENGTH);   /* UNICAST dual ended LM is enabled */
    else
        memcpy(ccm_conf->dmac, class1_multicast[data->config.level], VTSS_MEP_SUPP_MAC_LENGTH);
}


static void lt_conf_calc(vtss_mep_supp_ltm_conf_t *conf,  mep_instance_data_t *data)
{
    u32  i;

    conf->enable = data->lt_config.enable;
    conf->transaction_id = data->lt_state.transaction_id;
    conf->dei = FALSE;
    conf->prio = data->lt_config.prio;
    conf->ttl = data->lt_config.ttl;
    memcpy(conf->dmac, class1_multicast[data->config.level], VTSS_MEP_SUPP_MAC_LENGTH);
    conf->dmac[5] |= 0x08;  /* This makes it a class2 multicast address */

    if (!memcmp(all_zero_mac, data->lt_config.mac, VTSS_MEP_MAC_LENGTH))
    {/* All zero mac - mep must be a known peer mep */
        peer_mac_update(data);
        for (i=0; i<data->config.peer_count; ++i)
            if (data->lt_config.mep == data->config.peer_mep[i]) break;
        if (i == data->config.peer_count)                       return;
        memcpy(conf->tmac, data->config.peer_mac[i], VTSS_MEP_SUPP_MAC_LENGTH);
    }
    else
        memcpy(conf->tmac, data->lt_config.mac, VTSS_MEP_SUPP_MAC_LENGTH);
}


static i32 filter_los_counter(los_filter_t  *f, i32 delta_cnt, i32 start_cnt)
{
    i32 rc_cnt, i;

    f->raw_cnt += delta_cnt;
/*vtss_mep_trace("filter_los_counter1 - delta_cnt - f->raw_cnt - f->in - f->out", delta_cnt, f->raw_cnt, f->in, f->out);*/
    if (f->raw_cnt > 0)
    {   /* RAW counter is positive - put it into the filter buffer */
        f->buf[f->in] = f->raw_cnt;
        if (++f->in >= 10)   f->in = 0;

        if (delta_cnt < 0)  /* the RAW count has decreased - go through the filter and "chop the top off" - previous higher counts are only temporary */
            for (i=0; i<10; ++i)
                if (f->buf[i] > f->raw_cnt)  f->buf[i] = f->raw_cnt;

        if (f->in == f->out)
        {   /* Filter buffer has been filled - time to take counts from filter */
            rc_cnt = f->buf[f->out];
            if (++f->out >= 10)  f->out = 0;
        }
        else  rc_cnt = 0;   /* Until filter buffer has been filled just return zero counts */

    }
    else
    {   /* Filter is reset when RAW count becomes negative */
        f->in = f->out = 0;
        rc_cnt = 0;
    }
/*vtss_mep_trace("filter_los_counter2 - rc_cnt - start_cnt", rc_cnt, start_cnt, 0, 0);*/
    return (rc_cnt > start_cnt) ? rc_cnt : start_cnt;
}


static void push_dmr_data(u32 instance, u32 in_de, u32 in_de_var, u32 *out_de, u32 *out_del_var)
{
    mep_instance_data_t  *data;
    
    data = &instance_data[instance];    /* Instance data reference */
    
    if (data->dmr_rec_count >= data->dm_config.count) 
    {
        *out_de      = data->supp_state.dmr_delay[data->dmr_next_prt];
        *out_del_var = data->supp_state.dmr_delay_var[data->dmr_next_prt];
    }
    else
    {
        *out_de      = 0;
        *out_del_var = 0;   
    }     
    
    data->supp_state.dmr_delay[data->dmr_next_prt] = in_de;
    data->supp_state.dmr_delay_var[data->dmr_next_prt] = in_de_var;
    
    data->dmr_next_prt =  (data->dmr_next_prt + 1) % data->dm_config.count;
    data->dmr_rec_count++;
    
}    


static void push_dm1_data_far_to_near(u32 instance, u32 in_de, u32 in_de_var, u32 *out_de, u32 *out_del_var)
{
    mep_instance_data_t  *data;
    u32                  cnt;
    
    data = &instance_data[instance];    /* Instance data reference */
    
    cnt = (data->dm_config.count)? data->dm_config.count : VTSS_MEP_DM_MAX;  
    if (data->dm1_rec_count_far_to_near >= cnt) 
    {
        *out_de      = data->supp_state.dm1_delay_state_far_to_near[data->dm1_next_prt_far_to_near];
        *out_del_var = data->supp_state.dm1_delay_state_var_far_to_near[data->dm1_next_prt_far_to_near];
    }
    else
    {
        *out_de      = 0;
        *out_del_var = 0;   
    }     
    
    data->supp_state.dm1_delay_state_far_to_near[data->dm1_next_prt_far_to_near] = in_de;
    data->supp_state.dm1_delay_state_var_far_to_near[data->dm1_next_prt_far_to_near] = in_de_var;
    
    data->dm1_next_prt_far_to_near =  (data->dm1_next_prt_far_to_near + 1) % cnt;
    data->dm1_rec_count_far_to_near++;
    
}

static void push_dm1_data_near_to_far(u32 instance, u32 in_de, u32 in_de_var, u32 *out_de, u32 *out_del_var)
{
    mep_instance_data_t  *data;
    u32                  cnt;
    
    data = &instance_data[instance];    /* Instance data reference */
    
    cnt = (data->dm_config.count)? data->dm_config.count : VTSS_MEP_DM_MAX;  
    if (data->dm1_rec_count_near_to_far >= cnt) 
    {
        *out_de      = data->supp_state.dm1_delay_state_near_to_far[data->dm1_next_prt_near_to_far];
        *out_del_var = data->supp_state.dm1_delay_state_var_near_to_far[data->dm1_next_prt_near_to_far];
    }
    else
    {
        *out_de      = 0;
        *out_del_var = 0;   
    }     
    
    data->supp_state.dm1_delay_state_near_to_far[data->dm1_next_prt_near_to_far] = in_de;
    data->supp_state.dm1_delay_state_var_near_to_far[data->dm1_next_prt_near_to_far] = in_de_var;
    
    data->dm1_next_prt_near_to_far =  (data->dm1_next_prt_near_to_far + 1) % cnt;
    data->dm1_rec_count_near_to_far++;
}


static void run_config(u32 instance)
{
    mep_instance_data_t         *data;
    vtss_mep_supp_conf_t        conf;
    vtss_mep_supp_ccm_conf_t    ccm_conf;
    BOOL                        mep_port[VTSS_PORT_ARRAY_SIZE];

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_CONFIG;
    memset(&conf, 0, sizeof(conf));
    memset(&ccm_conf, 0, sizeof(ccm_conf));

    if (data->config.enable)
    {
        if (!flow_info_calc(data, &conf.flow, mep_port))      return;   /* Flow information is calculated */
        vtss_mep_port_register(instance, mep_port);

        if (!data->cc_config.enable)      data->cc_config.period = VTSS_MEP_MGMT_PERIOD_1S;

        conf.enable = data->config.enable;
        conf.voe = data->config.voe;
        vtss_mep_mac_get(data->config.port, conf.mac);  /* Get MAC of recidence port */
        conf.level = data->config.level;
        conf.out_of_service = FALSE;
        conf.port = data->config.port;
        if (data->up_mep)  /* This is a DS1076 Up-MEP - the ingress loop port is the residence port in mep_supp */
            conf.port = data->config.port + UP_MEP_INGRESS_LOOP_OFFSET;
        conf.mode = supp_mode_calc(data->config.mode);
        conf.direction = supp_direction_calc(data->config.direction);
        if (vtss_mep_supp_conf_set(instance, &conf) != VTSS_MEP_SUPP_RC_OK)     {vtss_mep_trace("run_config: Call to supp failed", instance, 0, 0, 0); return;}

        ccm_conf_calc(data,   &ccm_conf);
        if (vtss_mep_supp_ccm_conf_set(instance, &ccm_conf) != VTSS_MEP_SUPP_RC_OK)     {vtss_mep_trace("run_config: Call to supp failed", instance, 0, 0, 0); return;}

        if (data->cc_config.enable)      run_cc_config(instance);
        if (data->lm_config.enable)      run_lm_config(instance);
        if (data->aps_config.enable)     run_aps_config(instance);
        if (data->ais_config.enable)     run_ais_config(instance);
        if (data->lck_config.enable)     run_lck_config(instance);

        data->event_flags |= EVENT_OUT_SIGNAL;
        data->event_flags |= EVENT_OUT_SF_SD;

        data->update_rule = TRUE;           /* Rules are updated later - this in order to do this only once per VID */
        update_rule_timer = 500/timer_res;      /* Half second timer */

#if defined(VTSS_FEATURE_PHY_TIMESTAMP)
#if MEP_DM_TS_MCAST 
        vtss_mep_phy_config_mac(data->config.port, 1, 1, conf.mac);
        vtss_mep_phy_config_mac(data->config.port, 1, 0, conf.mac);    
        vtss_mep_phy_config_mac(data->config.port, 1, 1, class1_multicast[data->config.level]);   
        phy_ts_dump(data->config.port);
#endif    
#endif        
        vtss_mep_timer_start();
    }
    else
    {
        conf.enable = FALSE;
#if defined(VTSS_FEATURE_PHY_TIMESTAMP) 
#if MEP_DM_TS_MCAST     
        vtss_mep_mac_get(data->config.port, conf.mac);  /* Get MAC of recidence port */      
        vtss_mep_phy_config_mac(data->config.port, 0, 1, conf.mac);   
        vtss_mep_phy_config_mac(data->config.port, 0, 0, conf.mac);   
        vtss_mep_phy_config_mac(data->config.port, 0, 1, class1_multicast[data->config.level]);   
        phy_ts_dump(data->config.port);       
#endif
#endif        
        if (vtss_mep_supp_conf_set(instance, &conf) != VTSS_MEP_SUPP_RC_OK)     vtss_mep_trace("run_config: Call to supp failed", instance, 0, 0, 0);
        instance_data_clear(instance, data);
    }
}


static void run_cc_config(u32 instance)
{
    u32                         rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t         *data;
    vtss_mep_supp_ccm_conf_t    ccm_conf;
    vtss_mep_supp_gen_conf_t    gen_conf;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&gen_conf, 0, sizeof(gen_conf));
    memset(&ccm_conf, 0, sizeof(ccm_conf));

    data->event_flags &= ~EVENT_IN_CC_CONFIG;

    if (!data->config.voe) {
        data->update_rule = TRUE;               /* Rules are updated later - this in order to do this only once per VID - This is not for VOE MEP */
        update_rule_timer = 500/timer_res;      /* Half second timer */
        vtss_mep_timer_start();
    }

    if (data->cc_config.enable)
    {
        /* CCM is configurated */
        if (data->lm_config.ended == VTSS_MEP_MGMT_DUAL_ENDED)
            data->lm_config.prio = data->cc_config.prio;        /* CC and dual ended LM prioty is set to the same */
        peer_mac_update(data);
        ccm_conf_calc(data, &ccm_conf);
        rc += vtss_mep_supp_ccm_conf_set(instance, &ccm_conf);
    }

    /* CCM Generator is configurated */
    gen_conf.enable = (data->cc_config.enable || ((data->lm_config.enable) && (data->lm_config.ended == VTSS_MEP_MGMT_DUAL_ENDED)));
    gen_conf.lm_enable = ((data->lm_config.enable) && (data->lm_config.ended == VTSS_MEP_MGMT_DUAL_ENDED));
    gen_conf.lm_period = supp_period_calc((data->lm_config.period));
    gen_conf.cc_period = supp_period_calc((data->cc_config.enable) ? data->cc_config.period : data->lm_config.period);
    rc += vtss_mep_supp_ccm_generator_set(instance, &gen_conf);

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_cc_config: Call to supp failed", instance, 0, 0, 0);
}


static void run_lm_config(u32 instance)
{
    u32                         rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t         *data;
    vtss_mep_supp_ccm_conf_t    ccm_conf;
    vtss_mep_supp_lmm_conf_t    lmm_conf;
    vtss_mep_supp_gen_conf_t    gen_conf;
    u32                         i;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&lmm_conf, 0, sizeof(lmm_conf));
    memset(&gen_conf, 0, sizeof(gen_conf));
    memset(&ccm_conf, 0, sizeof(ccm_conf));

    data->event_flags &= ~EVENT_IN_LM_CONFIG;

    if (data->lm_config.enable)
    {
    /* Enable LM */
        if (data->lm_config.ended == VTSS_MEP_MGMT_DUAL_ENDED)
        {
        /* LM based on SW generated CCM session */
            /* CCM is configurated */
            data->cc_config.prio = data->lm_config.prio;    /* CC and dual ended LM priority is set to the same */
            ccm_conf_calc(data, &ccm_conf);
            rc += vtss_mep_supp_ccm_conf_set(instance, &ccm_conf);
        }

        /* Start one second loss measurement timer */
        if (lm_timer == 0)    lm_timer = 1000/timer_res;      /* One second timer */
        vtss_mep_timer_start();
    }
    else
    {
    /* Disable LM */
        /* Stop one second loss measurement timer */
        for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)     if(instance_data[i].lm_config.enable)   break;
        if (i==VTSS_MEP_INSTANCE_MAX)    lm_timer = 0;      /* No LM active - stop timer */

        /* No PDU has been received yet */
        memset(&data->out_state.lm_state, 0, sizeof(vtss_mep_mgmt_lm_state_t));
    
        memset(&data->near_los_filter, 0, sizeof(los_filter_t));
        memset(&data->far_los_filter, 0, sizeof(los_filter_t));
        data->near_tx_counter = 0;
        data->far_tx_counter = 0;
        data->flr_interval_count = 0;
    }

    /* LM based on LMM */
    lmm_conf.enable = ((data->lm_config.enable) && (data->lm_config.ended == VTSS_MEP_MGMT_SINGEL_ENDED));
    lmm_conf.prio = data->lm_config.prio;
    lmm_conf.dei = FALSE;
    lmm_conf.period = supp_period_calc(data->lm_config.period);
    peer_mac_update(data);
    if (data->lm_config.cast == VTSS_MEP_MGMT_UNICAST)    memcpy(lmm_conf.dmac, data->config.peer_mac[0], VTSS_MEP_MAC_LENGTH);
    else                                                  memcpy(lmm_conf.dmac, class1_multicast[data->config.level], VTSS_MEP_MAC_LENGTH);
    rc += vtss_mep_supp_lmm_conf_set(instance, &lmm_conf);

    /* CCM Generator is configurated */
    gen_conf.enable = (data->cc_config.enable || ((data->lm_config.enable) && (data->lm_config.ended == VTSS_MEP_MGMT_DUAL_ENDED)));
    gen_conf.lm_enable = ((data->lm_config.enable) && (data->lm_config.ended == VTSS_MEP_MGMT_DUAL_ENDED));
    gen_conf.lm_period = supp_period_calc(data->lm_config.period);
    gen_conf.cc_period = supp_period_calc((data->cc_config.enable) ? data->cc_config.period : data->lm_config.period);
    rc += vtss_mep_supp_ccm_generator_set(instance, &gen_conf);

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_lm_config: Call to supp failed", instance, 0, 0, 0);
}


static void run_dm_config(u32 instance)
{
    u32                         rc = VTSS_MEP_SUPP_RC_OK, i;
    mep_instance_data_t         *data;
    vtss_mep_supp_dmm_conf_t    conf;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&conf, 0, sizeof(conf));

    data->event_flags &= ~EVENT_IN_DM_CONFIG;

    if (data->dm_config.enable)
    {
        conf.enable = data->dm_config.enable;
        conf.prio = data->dm_config.prio;
        conf.dei = FALSE;
        conf.gap = data->dm_config.gap;
        conf.txway = data->dm_config.txway;
        conf.calcway = data->dm_config.calcway;
        conf.tunit = data->dm_config.tunit;
        conf.act = data->dm_config.act;
        conf.dm2fordm1 = data->dm_config.dm2fordm1;

        data->dmr_rec_count = 0;
        data->dmr_next_prt = 0;
        data->dmr_sum = 0;
        data->dmr_var_sum = 0;
        data->dmr_n_sum = 0;
        data->dmr_n_var_sum = 0;
        
        data->dm1_rec_count_far_to_near = 0;
        data->dm1_next_prt_far_to_near = 0;
        data->dm1_sum_far_to_near = 0;     
        data->dm1_var_sum_far_to_near = 0;
        data->dm1_n_sum_far_to_near = 0;   
        data->dm1_n_var_sum_far_to_near = 0;
        
        data->dm1_rec_count_near_to_far = 0;
        data->dm1_next_prt_near_to_far = 0;
        data->dm1_sum_near_to_far = 0;     
        data->dm1_var_sum_near_to_far = 0;
        data->dm1_n_sum_near_to_far = 0;   
        data->dm1_n_var_sum_near_to_far = 0;
        

        memset(&data->out_state.dmr_state, 0, sizeof(vtss_mep_mgmt_dm_state_t));
        memset(&data->out_state.dm1_state_far_to_near, 0, sizeof(vtss_mep_mgmt_dm_state_t));
        memset(&data->out_state.dm1_state_near_to_far, 0, sizeof(vtss_mep_mgmt_dm_state_t));
        data->out_state.dmr_state.best_delay = 0xffffffff;
        data->out_state.dm1_state_far_to_near.best_delay = 0xffffffff;
        data->out_state.dm1_state_near_to_far.best_delay = 0xffffffff;
     
        memset(data->supp_state.dmr_delay, 0, sizeof(data->supp_state.dmr_delay));
        memset(data->supp_state.dmr_delay_var, 0, sizeof(data->supp_state.dmr_delay_var));
        memset(data->supp_state.dm1_delay_state_far_to_near, 0, sizeof(data->supp_state.dm1_delay_state_far_to_near));
        memset(data->supp_state.dm1_delay_state_var_far_to_near, 0, sizeof(data->supp_state.dm1_delay_state_var_far_to_near));
        memset(data->supp_state.dm1_delay_state_near_to_far, 0, sizeof(data->supp_state.dm1_delay_state_near_to_far));
        memset(data->supp_state.dm1_delay_state_var_near_to_far, 0, sizeof(data->supp_state.dm1_delay_state_near_to_far));
        
        /* set dmr state time unit */
        data->out_state.dmr_state.tunit = data->dm_config.tunit;
        data->out_state.dm1_state_far_to_near.tunit = data->dm_config.tunit;
        data->out_state.dm1_state_near_to_far.tunit = data->dm_config.tunit;
        
        
        if (data->dm_config.cast == VTSS_MEP_MGMT_UNICAST)
        {
            peer_mac_update(data);
            for (i=0; i<data->config.peer_count; ++i)
                if (data->dm_config.mep == data->config.peer_mep[i]) break;
            if (i == data->config.peer_count)                       return;
            memcpy(conf.dmac, data->config.peer_mac[i], VTSS_MEP_SUPP_MAC_LENGTH);
        }
        else
        {
            memcpy(conf.dmac, class1_multicast[data->config.level], VTSS_MEP_MAC_LENGTH);
        } 
    }
    else {
        conf.enable = data->dm_config.enable;
        conf.tunit = data->dm_config.tunit;
    }

    if (data->dm_config.way == VTSS_MEP_MGMT_ONE_WAY)
        rc = vtss_mep_supp_dm1_conf_set(instance, (vtss_mep_supp_dm1_conf_t *)&conf);
    else 
        rc = vtss_mep_supp_dmm_conf_set(instance, &conf);   
    
    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_dm_config: Call to supp failed", instance, 0, 0, 0);
}


static void run_aps_config(u32 instance)
{
    u32                       rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t       *data;
    vtss_mep_supp_aps_conf_t  conf;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&conf, 0, sizeof(conf));

    data->event_flags &= ~EVENT_IN_APS_CONFIG;

    conf.enable = data->aps_config.enable;
    conf.prio = data->aps_config.prio;
    conf.dei = FALSE;
    conf.type = supp_aps_calc(data->aps_config.type);
    if (data->aps_config.type == VTSS_MEP_MGMT_L_APS)
    {
        peer_mac_update(data);
        if (data->aps_config.cast == VTSS_MEP_MGMT_UNICAST)    memcpy(conf.dmac, data->config.peer_mac[0], VTSS_MEP_MAC_LENGTH);
        else                                                   memcpy(conf.dmac, class1_multicast[data->config.level], VTSS_MEP_MAC_LENGTH);
    }
    else
    {
        raps_mac[5] = data->aps_config.raps_octet;
        memcpy(conf.dmac, raps_mac, VTSS_MEP_MAC_LENGTH);
    }

    raps_forwarding_control(data->config.vid);

    rc = vtss_mep_supp_aps_conf_set(instance, &conf);

    /* No PDU has been received yet */
    memset(data->out_state.rx_aps, 0, VTSS_MEP_APS_DATA_LENGTH);
    instance_data[instance].event_flags |= EVENT_OUT_APS;

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_aps_config: Call to supp failed", instance, 0, 0, 0);
}


static void run_lt_config(u32 instance)
{
    u32                       rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t       *data;
    vtss_mep_supp_ltm_conf_t  conf;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&conf, 0, sizeof(conf));

    data->event_flags &= ~EVENT_IN_LT_CONFIG;

    if (data->lt_config.enable)
    {
        data->lt_state.transaction_id++;
        data->lt_state.timer = 5000/timer_res;
        vtss_mep_timer_start();

        memset(&data->out_state.lt_state, 0, sizeof(vtss_mep_mgmt_lt_state_t));
        data->out_state.lt_state.transaction[0].transaction_id = data->lt_state.transaction_id;

        lt_conf_calc(&conf,  data);
    }
    else
    {
        conf.enable = data->lt_config.enable;
        data->lt_state.timer = 0;
    }

    rc = vtss_mep_supp_ltm_conf_set(instance, &conf);

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_lt_config: Call to supp failed", instance, 0, 0, 0);
}


static void run_lb_config(u32 instance)
{
    u32                        i, rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t        *data;
    vtss_mep_supp_lbm_conf_t   conf;
    vtss_mep_supp_lb_status_t  status;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&conf, 0, sizeof(conf));

    data->event_flags &= ~EVENT_IN_LB_CONFIG;

    if (data->lb_config.enable)
    {
        if (data->lb_config.to_send != VTSS_MEP_LB_MGMT_TO_SEND_INFINITE) { /* Only start the time if this is not infinite LBM transmission */
            data->lb_state.timer = ((data->lb_config.to_send*((data->lb_config.interval*10)+5))+(1000*5))/timer_res; /* Adding 5ms. to the gap to compensate for the sw handling time. Adding 5s. that is the wait time to collect all LBR */
            vtss_mep_timer_start();
        }
        memset(&data->out_state.lb_state, 0, sizeof(vtss_mep_mgmt_lb_state_t));
        if (vtss_mep_supp_lb_status_get(instance, &status) == VTSS_MEP_SUPP_RC_OK) {
            for (i=0; i<VTSS_MEP_PEER_MAX; ++i)
                data->lb_state.rx_transaction_id[i] = status.trans_id;
        }

        conf.enable = data->lb_config.enable;
        conf.prio = data->lb_config.prio;
        conf.dei = data->lb_config.dei;
        conf.to_send = data->lb_config.to_send;
        conf.size = data->lb_config.size;
        conf.interval = data->lb_config.interval;

        if (data->lb_config.cast == VTSS_MEP_MGMT_UNICAST)
        {
            if (!memcmp(all_zero_mac, data->lb_config.mac, VTSS_MEP_MAC_LENGTH))
            {/* All zero mac - mep must be a known peer mep */
                peer_mac_update(data);
                for (i=0; i<data->config.peer_count; ++i)
                    if (data->lb_config.mep == data->config.peer_mep[i]) break;
                if (i == data->config.peer_count)                       return;
                memcpy(conf.dmac, data->config.peer_mac[i], VTSS_MEP_SUPP_MAC_LENGTH);
            }
            else
                memcpy(conf.dmac, data->lb_config.mac, VTSS_MEP_SUPP_MAC_LENGTH);
        }
        else
            memcpy(conf.dmac, class1_multicast[data->config.level], VTSS_MEP_SUPP_MAC_LENGTH);
    }
    else
    {
        conf.enable = data->lb_config.enable;
        data->lb_state.timer = 0;
    }

    rc = vtss_mep_supp_lbm_conf_set(instance, &conf);

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_lb_config: Call to supp failed", instance, 0, 0, 0);
}

static void run_ais_config(u32 instance)
{
    u32                       i, rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t       *data;
    vtss_mep_supp_ais_conf_t  conf;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&conf, 0, sizeof(conf));

    data->event_flags &= ~EVENT_IN_AIS_CONFIG;

    /* copy data from data->ais_config onto conf and let supp know about the same */
    if (data->ais_config.enable)
    {
        conf.period = data->ais_config.period;
        conf.prio = data->ais_config.client_prio;
        conf.dei = FALSE;
        conf.level = data->ais_config.client_level;
        conf.protection = data->ais_config.protection;
        memcpy(conf.dmac, class1_multicast[data->config.level], VTSS_MEP_SUPP_MAC_LENGTH);
        if (!client_flow_info_calc(data, data->ais_config.client_domain, data->ais_config.client_flow_count, data->ais_config.client_flows, conf.flows, &conf.flow_count))      return;
        if (conf.flow_count) {
            for (i=0; i<conf.flow_count; ++i)
                conf.flows[i].port[data->config.port] = FALSE; /* AIS is not transmitted on residence port */
            rc = vtss_mep_supp_ais_conf_set(instance, &conf);
            rc += vtss_mep_supp_ais_set(instance, data->out_state.state.aTsf);  /* Control AIS */
        }
    }
    else
        rc += vtss_mep_supp_ais_set(instance, FALSE); 

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_ais_config: Call to supp failed", instance, 0, 0, 0);
}

static void run_lck_config(u32 instance)
{
    u32                       rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t       *data;
    vtss_mep_supp_lck_conf_t  conf;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&conf, 0, sizeof(conf));

    data->event_flags &= ~EVENT_IN_LCK_CONFIG;

    /* copy data from data->lck_config onto conf and let supp know about the same */
    if (data->lck_config.enable)
    {
        conf.enable = data->lck_config.enable;
        conf.period = data->lck_config.period;
        conf.prio = data->lck_config.client_prio;
        conf.dei = FALSE;
        conf.level = data->lck_config.client_level;
        memcpy(conf.dmac, class1_multicast[data->config.level], VTSS_MEP_SUPP_MAC_LENGTH);
        if (!client_flow_info_calc(data, data->lck_config.client_domain, data->lck_config.client_flow_count, data->lck_config.client_flows, conf.flows, &conf.flow_count))      return;
    }
    else
        conf.enable = data->lck_config.enable;

    rc = vtss_mep_supp_lck_conf_set(instance, &conf);

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_lck_config: Call to supp failed", instance, 0, 0, 0);
}


static void run_tst_config(u32 instance)
{
    u32                       i, rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t       *data;
    vtss_mep_supp_tst_conf_t  conf;

    data = &instance_data[instance];    /* Instance data reference */
    memset(&conf, 0, sizeof(conf));

    data->event_flags &= ~EVENT_IN_TST_CONFIG;

    if (data->tst_config.enable)
    {
        conf.enable = data->tst_config.enable;
        conf.prio = data->tst_config.prio;
        conf.dei = data->tst_config.dei;
        conf.pattern = supp_pattern_calc(data->tst_config.pattern);
        conf.size = data->tst_config.size;
        conf.add_cnt = (data->up_mep && data->tunnel) ? 4 : 0;   /* Up-MEP in tunnel EVC must compensate for addition of tag on NNI */
        conf.rate = data->tst_config.rate;
        conf.sequence = data->tst_config.sequence;
        conf.transaction_id = 0;

        peer_mac_update(data);
        for (i=0; i<data->config.peer_count; ++i)
            if (data->tst_config.mep == data->config.peer_mep[i]) break;
        if (i == data->config.peer_count)                       return;
        memcpy(conf.dmac, data->config.peer_mac[i], VTSS_MEP_SUPP_MAC_LENGTH);
    }
    else {
        conf.enable = FALSE;
    }

    conf.enable_rx = data->tst_config.enable_rx;

    if (conf.enable || conf.enable_rx) {
        memset(&data->tst_state, 0, sizeof(data->tst_state));
        memset(&data->out_state.tst_state, 0, sizeof(data->out_state.tst_state));

        vtss_mep_acl_tst_clear(instance);
        (void)vtss_mep_supp_tst_clear(instance);

        data->tst_state.timer = 1000/timer_res;     /* Start the TST timer only to make 'Test time' counting */
        vtss_mep_timer_start();
    }
    else
        data->tst_state.timer = 0;

    rc = vtss_mep_supp_tst_conf_set(instance, &conf);

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_tst_config: Call to supp failed", instance, 0, 0, 0);
}


static void run_tx_aps(u32 instance)
{
    u32  rc = VTSS_MEP_SUPP_RC_OK;

    instance_data[instance].event_flags &= ~EVENT_IN_TX_APS;
/*
    if (instance_data[instance].aps_config.enable)      rc = vtss_mep_supp_aps_txdata_set(instance, instance_data[instance].tx_aps);
*/
    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_tx_aps: Call to supp failed", instance, 0, 0, 0);
}


static void run_aps_info(u32 instance)
{
    instance_data[instance].event_flags &= ~EVENT_IN_APS_INFO;

    /* Deliver APS to EPS */
    instance_data[instance].event_flags |= EVENT_OUT_APS;
}


static void run_defect_state(u32 instance)
{
    mep_instance_data_t *data;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_DEFECT_STATE;

    if (vtss_mep_supp_defect_state_get(instance,  &data->supp_state.defect_state) != VTSS_MEP_SUPP_RC_OK)     vtss_mep_trace("run_ccm_state: Call to supp failed", instance, 0, 0, 0);

    fault_cause_calc(data);

    consequent_action_calc_and_deliver(data);       /* Calculate aTsf (SF) and deliver to EPS */
}


static void run_ssf_state(u32 instance)
{
    mep_instance_data_t *data;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_SSF_STATE;

    fault_cause_calc(data);

    consequent_action_calc_and_deliver(data);       /* Calculate aTsf (SF) and deliver to EPS */
}


static void run_ltr_new(u32 instance)
{
    mep_instance_data_t             *data;
    vtss_mep_supp_ltr_t             ltr[VTSS_MEP_SUPP_LTR_MAX];
    vtss_mep_mgmt_lt_transaction_t  *transaction;
    vtss_mep_mgmt_lt_reply_t        *reply;
    u32 i, ltr_cnt;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_LTR_NEW;

    if (vtss_mep_supp_ltr_get(instance, &ltr_cnt, ltr) != VTSS_MEP_SUPP_RC_OK)      return;
    if (!data->lt_config.enable)                                                    return;

    transaction = &data->out_state.lt_state.transaction[data->out_state.lt_state.transaction_cnt];

    for (i=0; i<ltr_cnt; ++i)
    {/* Walk through the new received replies */
        if ((data->lt_state.transaction_id == ltr[i].transaction_id) && (transaction->reply_cnt < VTSS_MEP_REPLY_MAX))
        {   /* LT is enabled and LTR has the active transaction ID and there is still room for a reply */
            reply = &transaction->reply[transaction->reply_cnt];

            reply->ttl = ltr[i].ttl;
            reply->mode = mode_calc(ltr[i].mode);
            reply->direction = direction_calc(ltr[i].direction);
            reply->relayed = ltr[i].relayed;
            memcpy(reply->last_egress_mac, ltr[i].last_egress_mac, VTSS_MEP_MAC_LENGTH);
            memcpy(reply->next_egress_mac, ltr[i].next_egress_mac, VTSS_MEP_MAC_LENGTH);

            transaction->reply_cnt++;
        }
    }
}


static void run_lbr_new(u32 instance)
{
    mep_instance_data_t  *data;
    vtss_mep_supp_lbr_t  lbr[VTSS_MEP_SUPP_LBR_MAX];
    u32                  i, j, lbr_cnt;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_LBR_NEW;

    if (vtss_mep_supp_lbr_get(instance, &lbr_cnt, lbr) != VTSS_MEP_SUPP_RC_OK)      return;
    if (!data->lb_config.enable)                                                    return;

    for (j=0; j<lbr_cnt; ++j)
    {/* Walk through the new received replies */
        for(i=0; i<data->out_state.lb_state.reply_cnt; ++i) /* Check if this MAC is already received */
            if (!memcmp(lbr[j].mac, data->out_state.lb_state.reply[i].mac, VTSS_MEP_MAC_LENGTH))    break;
        if (i < VTSS_MEP_PEER_MAX)
        {/* Old reply found or there is still room for more new replies */
            if (i == data->out_state.lb_state.reply_cnt)
            {/* New reply received */
                data->out_state.lb_state.reply_cnt++;
                memcpy(data->out_state.lb_state.reply[i].mac, lbr[j].mac, VTSS_MEP_MAC_LENGTH);
            }
            data->out_state.lb_state.reply[i].lbr_received++;
            if (lbr[j].transaction_id != data->lb_state.rx_transaction_id[i])       data->out_state.lb_state.reply[i].out_of_order++;
            data->lb_state.rx_transaction_id[i] = lbr[j].transaction_id + 1;  /* Next expected Transaction ID is this received - plus one */
        }
    }
}


static void run_tst_new(u32 instance)
{
    mep_instance_data_t  *data;
    u32                  i;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_TST_NEW;

#if defined(VTSS_ARCH_JAGUAR_1) || !defined(VTSS_SW_OPTION_ACL) || !defined(VTSS_FEATURE_ACL_V2)
    vtss_vid_mac_t         vid_mac;
    vtss_mac_table_entry_t entry;

    vid_mac.vid = data->rule_vid;
    vtss_mep_mac_get(data->config.port, vid_mac.mac.addr);
    if (vtss_mac_table_get(NULL, &vid_mac, &entry) == VTSS_RC_OK) {
        entry.copy_to_cpu = 0;
        if (vtss_mac_table_add(NULL, &entry) != VTSS_RC_OK) vtss_mep_trace("run_tst_new: Call to vtss_mac_table_add", instance, 0, 0, 0);
    }
#endif

    if (data->up_mep) { /* This is an Up-MEP. As there is only one IS2 rule, only one Up-MEP can have active TST counting */
        for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i) {
            if ((i != instance) && instance_data[i].config.enable && instance_data[i].up_mep)
                 memset(&instance_data[i].tst_state, 0, sizeof(instance_data[i].tst_state));
        }
    }

    if (data->tst_config.enable || data->tst_config.enable_rx) {
        data->tst_state.rx_count_active = TRUE;
        data->tst_state.timer = 1000/timer_res;
        vtss_mep_timer_start();
    }
}
 

static void near_to_far_calc(u32 instance, u32 txcount, u32 rxcount, u32 rx_err_count, u32 dm[VTSS_MEP_SUPP_DM_MAX])
{
    mep_instance_data_t  *data;
    u32                  i, out_delay;
    u32                  delay_var, out_delay_var, newcount;
    u32                  cnt, ov_cnt;

    data = &instance_data[instance];    /* Instance data reference */

    for (i = 0 ; i < rxcount; ++i)
    {
        if (data->dm1_sum_near_to_far > (data->dm1_sum_near_to_far + dm[i]))
        {
            /* overflow, reset anything */
            ov_cnt = data->out_state.dm1_state_near_to_far.ovrflw_cnt + 1;
            
            data->dm1_rec_count_near_to_far = 0;
            data->dm1_next_prt_near_to_far = 0;
            data->dm1_sum_near_to_far = 0;
            data->dm1_var_sum_near_to_far = 0;
            data->dm1_n_sum_near_to_far = 0;
            data->dm1_n_var_sum_near_to_far = 0;
        
            memset(&data->out_state.dm1_state_near_to_far, 0, sizeof(vtss_mep_mgmt_dm_state_t));
            data->out_state.dm1_state_near_to_far.best_delay = 0xffffffff;
            data->out_state.dm1_state_near_to_far.ovrflw_cnt = ov_cnt; 
     
            memset(data->supp_state.dm1_delay_state_near_to_far, 0, sizeof(data->supp_state.dm1_delay_state_near_to_far));
            memset(data->supp_state.dm1_delay_state_var_near_to_far, 0, sizeof(data->supp_state.dm1_delay_state_var_near_to_far));
        } 
        
        if (data->out_state.dm1_state_near_to_far.rx_cnt == 0)   
        {
            delay_var = 0; /* the first one */    
        }
        else    
        {
            delay_var = ((dm[i]  >=  data->out_state.dm1_state_near_to_far.last_delay))?
                         (dm[i] - data->out_state.dm1_state_near_to_far.last_delay):
                         (data->out_state.dm1_state_near_to_far.last_delay - dm[i]);
        }    
        
        push_dm1_data_near_to_far(instance, dm[i], delay_var, &out_delay, &out_delay_var);
        
        newcount =  data->out_state.dm1_state_near_to_far.rx_cnt + 1; 
        
                  
        data->dm1_sum_near_to_far = data->dm1_sum_near_to_far + dm[i];
        data->out_state.dm1_state_near_to_far.avg_delay = data->dm1_sum_near_to_far / newcount; 
        
        data->dm1_var_sum_near_to_far = data->dm1_var_sum_near_to_far + delay_var;
        data->out_state.dm1_state_near_to_far.avg_delay_var = data->dm1_var_sum_near_to_far / newcount;             
        
        cnt = (data->dm_config.count)? data->dm_config.count : VTSS_MEP_DM_MAX;
        if (data->out_state.dm1_state_near_to_far.rx_cnt >= cnt)
        {  
            data->dm1_n_sum_near_to_far = data->dm1_n_sum_near_to_far + dm[i] - out_delay;
            data->out_state.dm1_state_near_to_far.avg_n_delay = data->dm1_n_sum_near_to_far / cnt;   
            
            data->dm1_n_var_sum_near_to_far = data->dm1_n_var_sum_near_to_far + delay_var - out_delay_var;
            data->out_state.dm1_state_near_to_far.avg_n_delay_var = data->dm1_n_var_sum_near_to_far / cnt; 
        } 
        else 
        {   
            data->dm1_n_sum_near_to_far = data->dm1_n_sum_near_to_far + dm[i] - out_delay;
            data->out_state.dm1_state_near_to_far.avg_n_delay = data->dm1_n_sum_near_to_far / newcount;   
            
            data->dm1_n_var_sum_near_to_far = data->dm1_n_var_sum_near_to_far + delay_var - out_delay_var;
            data->out_state.dm1_state_near_to_far.avg_n_delay_var = data->dm1_n_var_sum_near_to_far / newcount; 
        }    
        
        if (dm[i] < data->out_state.dm1_state_near_to_far.best_delay)
            data->out_state.dm1_state_near_to_far.best_delay = dm[i];    
                
        if (dm[i] > data->out_state.dm1_state_near_to_far.worst_delay)
            data->out_state.dm1_state_near_to_far.worst_delay = dm[i];
            
        data->out_state.dm1_state_near_to_far.rx_cnt = newcount;
        data->out_state.dm1_state_near_to_far.last_delay = dm[i];
    }
    
    data->out_state.dm1_state_near_to_far.tx_cnt += txcount;
    data->out_state.dm1_state_near_to_far.rx_err_cnt += rx_err_count;
}

static void far_to_near_calc(u32 instance, u32 txcount, u32 rxcount, u32 rx_err_count, u32 dm[VTSS_MEP_SUPP_DM_MAX])
{
    mep_instance_data_t  *data;
    u32                  i, out_delay;
    u32                  delay_var, out_delay_var, newcount;
    u32                  cnt, ov_cnt;

    data = &instance_data[instance];    /* Instance data reference */

    for (i = 0 ; i < rxcount; ++i)
    {
        if (data->dm1_sum_far_to_near > (data->dm1_sum_far_to_near + dm[i]))
        {
            /* overflow, reset anything */
            ov_cnt = data->out_state.dm1_state_far_to_near.ovrflw_cnt + 1;
            
            data->dm1_rec_count_far_to_near = 0;
            data->dm1_next_prt_far_to_near = 0;
            data->dm1_sum_far_to_near = 0;
            data->dm1_var_sum_far_to_near = 0;
            data->dm1_n_sum_far_to_near = 0;
            data->dm1_n_var_sum_far_to_near = 0;
        
            memset(&data->out_state.dm1_state_far_to_near, 0, sizeof(vtss_mep_mgmt_dm_state_t));
            data->out_state.dm1_state_far_to_near.best_delay = 0xffffffff;
            data->out_state.dm1_state_far_to_near.ovrflw_cnt = ov_cnt; 
     
            memset(data->supp_state.dm1_delay_state_far_to_near, 0, sizeof(data->supp_state.dm1_delay_state_far_to_near));
            memset(data->supp_state.dm1_delay_state_var_far_to_near, 0, sizeof(data->supp_state.dm1_delay_state_var_far_to_near));
        } 
        
        if (data->out_state.dm1_state_far_to_near.rx_cnt == 0)   
        {
            delay_var = 0; /* the first one */    
        }
        else    
        {
            delay_var = ((dm[i]  >=  data->out_state.dm1_state_far_to_near.last_delay))?
                         (dm[i] - data->out_state.dm1_state_far_to_near.last_delay):
                         (data->out_state.dm1_state_far_to_near.last_delay - dm[i]);
        }    
        
        push_dm1_data_far_to_near(instance, dm[i], delay_var, &out_delay, &out_delay_var);
        
        newcount =  data->out_state.dm1_state_far_to_near.rx_cnt + 1; 
        
                  
        data->dm1_sum_far_to_near = data->dm1_sum_far_to_near + dm[i];
        data->out_state.dm1_state_far_to_near.avg_delay = data->dm1_sum_far_to_near / newcount; 
        
        data->dm1_var_sum_far_to_near = data->dm1_var_sum_far_to_near + delay_var;
        data->out_state.dm1_state_far_to_near.avg_delay_var = data->dm1_var_sum_far_to_near / newcount;             
        
        cnt = (data->dm_config.count)? data->dm_config.count : VTSS_MEP_DM_MAX;
        if (data->out_state.dm1_state_far_to_near.rx_cnt >= cnt)
        {  
            data->dm1_n_sum_far_to_near = data->dm1_n_sum_far_to_near + dm[i] - out_delay;
            data->out_state.dm1_state_far_to_near.avg_n_delay = data->dm1_n_sum_far_to_near / cnt;   
            
            data->dm1_n_var_sum_far_to_near = data->dm1_n_var_sum_far_to_near + delay_var - out_delay_var;
            data->out_state.dm1_state_far_to_near.avg_n_delay_var = data->dm1_n_var_sum_far_to_near / cnt; 
        } 
        else 
        {   
            data->dm1_n_sum_far_to_near = data->dm1_n_sum_far_to_near + dm[i] - out_delay;
            data->out_state.dm1_state_far_to_near.avg_n_delay = data->dm1_n_sum_far_to_near / newcount;   
            
            data->dm1_n_var_sum_far_to_near = data->dm1_n_var_sum_far_to_near + delay_var - out_delay_var;
            data->out_state.dm1_state_far_to_near.avg_n_delay_var = data->dm1_n_var_sum_far_to_near / newcount; 
        }    
        
        if (dm[i] < data->out_state.dm1_state_far_to_near.best_delay)
            data->out_state.dm1_state_far_to_near.best_delay = dm[i];    
                
        if (dm[i] > data->out_state.dm1_state_far_to_near.worst_delay)
            data->out_state.dm1_state_far_to_near.worst_delay = dm[i];
            
        data->out_state.dm1_state_far_to_near.rx_cnt = newcount;
        data->out_state.dm1_state_far_to_near.last_delay = dm[i];
    }
    
    data->out_state.dm1_state_far_to_near.tx_cnt += txcount;
    data->out_state.dm1_state_far_to_near.rx_err_cnt += rx_err_count;
}

static void run_dmr_new(u32 instance)
{
    mep_instance_data_t      *data;
    vtss_mep_supp_dmr_status status;
    u32                      i, out_delay;
    u32                      delay_var, out_delay_var, newcount, ov_cnt;
    

    data = &instance_data[instance];    /* Instance data reference */
    if (!data->config.enable || !data->dm_config.enable)  return;

    data->event_flags &= ~EVENT_IN_DMR_NEW;

    if (vtss_mep_supp_dmr_get(instance, &status) != VTSS_MEP_SUPP_RC_OK)      return;
    if (data->dm_config.act == VTSS_MEP_SUPP_ACT_DISABLE)
    {
        /* overflow, keep the counter */
        if (data->out_state.dmr_state.ovrflw_cnt != 0)
            return;
    }   
    
    for (i=0; i<status.rx_counter; ++i)
    {
        if (data->dmr_sum > (data->dmr_sum + status.tw_delays[i]))
        {
            if (data->dm_config.act == VTSS_MEP_SUPP_ACT_DISABLE)
            {
                /* overflow, keep the counter */
                data->out_state.dmr_state.ovrflw_cnt = 1;
                return;
            }        
            
            /* overflow, reset anything */
            ov_cnt = data->out_state.dmr_state.ovrflw_cnt + 1;
            
            data->dmr_rec_count = 0;
            data->dmr_next_prt = 0;
            data->dmr_sum = 0;
            data->dmr_var_sum = 0;
            data->dmr_n_sum = 0;
            data->dmr_n_var_sum = 0;
        
            memset(&data->out_state.dmr_state, 0, sizeof(vtss_mep_mgmt_dm_state_t));
            data->out_state.dmr_state.best_delay = 0xffffffff;
            data->out_state.dmr_state.ovrflw_cnt = ov_cnt;
     
            memset(data->supp_state.dmr_delay, 0, sizeof(data->supp_state.dmr_delay));
            memset(data->supp_state.dmr_delay_var, 0, sizeof(data->supp_state.dmr_delay_var));
        }
        
        if (data->out_state.dmr_state.rx_cnt == 0)   
        {
            delay_var = 0; /* the first one */    
        }
        else    
        {
            delay_var = ((status.tw_delays[i] >=  data->out_state.dmr_state.last_delay) )?
                         (status.tw_delays[i] - data->out_state.dmr_state.last_delay):
                         (data->out_state.dmr_state.last_delay - status.tw_delays[i]);
        }    
        
        push_dmr_data(instance, status.tw_delays[i], delay_var, &out_delay, &out_delay_var);
        
        newcount =  data->out_state.dmr_state.rx_cnt + 1; 
        
                  
        data->dmr_sum = data->dmr_sum + status.tw_delays[i];
        data->out_state.dmr_state.avg_delay = data->dmr_sum / newcount; 
        
        data->dmr_var_sum = data->dmr_var_sum + delay_var;
        data->out_state.dmr_state.avg_delay_var = data->dmr_var_sum / newcount;             
     
        if (data->out_state.dmr_state.rx_cnt >= data->dm_config.count)
        {  
            data->dmr_n_sum = data->dmr_n_sum + status.tw_delays[i] - out_delay;
            data->out_state.dmr_state.avg_n_delay = data->dmr_n_sum / data->dm_config.count;   
            
            data->dmr_n_var_sum = data->dmr_n_var_sum + delay_var - out_delay_var;
            data->out_state.dmr_state.avg_n_delay_var = data->dmr_n_var_sum / data->dm_config.count; 
        } 
        else 
        {   
            data->dmr_n_sum = data->dmr_n_sum + status.tw_delays[i] - out_delay;
            data->out_state.dmr_state.avg_n_delay = data->dmr_n_sum / newcount;   
            
            data->dmr_n_var_sum = data->dmr_n_var_sum + delay_var - out_delay_var;
            data->out_state.dmr_state.avg_n_delay_var = data->dmr_n_var_sum / newcount; 
        }    
        
        if (status.tw_delays[i] < data->out_state.dmr_state.best_delay)
            data->out_state.dmr_state.best_delay = status.tw_delays[i];
                
        if (status.tw_delays[i] > data->out_state.dmr_state.worst_delay)
            data->out_state.dmr_state.worst_delay = status.tw_delays[i];
            
        data->out_state.dmr_state.rx_cnt = newcount;
        data->out_state.dmr_state.last_delay = status.tw_delays[i];
    }
        
    data->out_state.dmr_state.tx_cnt += status.tx_counter;
    data->out_state.dmr_state.rx_err_cnt += status.tw_rx_err_counter;
    data->out_state.dmr_state.rx_tout_cnt += status.rx_tout_counter;
    data->out_state.dmr_state.late_txtime += status.late_txtime_counter;

    if (data->dm_config.dm2fordm1) {
        far_to_near_calc(instance, 0, status.rx_counter, status.ow_ftn_rx_err_counter, status.ow_ftn_delays);
        near_to_far_calc(instance, status.tx_counter, status.rx_counter, status.ow_ntf_rx_err_counter, status.ow_ntf_delays);
    }
}

static void run_dm1_new(u32 instance)
{
    vtss_mep_supp_dm1_status status;

    instance_data[instance].event_flags &= ~EVENT_IN_1DM_NEW;

    if (vtss_mep_supp_dm1_get(instance, &status) != VTSS_MEP_SUPP_RC_OK)      return;

    near_to_far_calc(instance, status.tx_counter, 0, 0, status.delays);
    far_to_near_calc(instance, 0, status.rx_counter, status.rx_err_counter, status.delays);
}

static void run_lm_timer(u32 instance)
{
    u32                           rc = VTSS_MEP_SUPP_RC_OK;
    vtss_mep_mgmt_lm_state_t      *lm_state;
    mep_instance_data_t           *data;
    vtss_mep_supp_lm_counters_t   counters;

    data = &instance_data[instance];    /* Instance data reference */
    lm_state = &data->out_state.lm_state;

    data->event_flags &= ~EVENT_IN_LM_TIMER;

    /* Single/Dual ended - based on LMM/LMR or CCM */
    if (data->lm_config.ended == VTSS_MEP_MGMT_SINGEL_ENDED)    rc += vtss_mep_supp_lmm_lm_counters_get(instance, &counters);
    else                                                        rc += vtss_mep_supp_ccm_lm_counters_get(instance, &counters);

    /* Accumulate to management - counters stand on max value */
    lm_state->tx_counter += counters.tx_counter;
    lm_state->rx_counter += counters.rx_counter;
    lm_state->near_los_counter = filter_los_counter(&data->near_los_filter, counters.near_los_counter, lm_state->near_los_counter);
    lm_state->far_los_counter = filter_los_counter(&data->far_los_filter, counters.far_los_counter, lm_state->far_los_counter);

    /* Accumulate for ratio calculation */
    data->near_los_counter += counters.near_los_counter;
    data->far_los_counter += counters.far_los_counter;
    data->near_tx_counter += counters.near_tx_counter;
    data->far_tx_counter += counters.far_tx_counter;

    data->flr_interval_count++;
    if (data->flr_interval_count >= data->lm_config.flr_interval)
    {
    /* Frame Loss Ratio interval has expired - calculate ratio */
        if ((data->near_los_counter > 0) && (data->far_tx_counter > 0))
            lm_state->near_los_ratio = (((u32)(data->near_los_counter)*100)/data->far_tx_counter);
        else
            lm_state->near_los_ratio = 0;

        if ((data->far_los_counter > 0) && (data->near_tx_counter > 0))
            lm_state->far_los_ratio = (((u32)(data->far_los_counter)*100)/data->near_tx_counter);
        else
            lm_state->far_los_ratio = 0;

        /* restart Frame Loss Ratio interval */
        data->near_los_counter = 0;
        data->far_los_counter = 0;
        data->near_tx_counter = 0;
        data->far_tx_counter = 0;
        data->flr_interval_count = 0;
    }

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_lm_timer: Call to supp failed", instance, 0, 0, 0);
}


static void run_lt_timer(u32 instance)
{
    u32                         rc = VTSS_MEP_SUPP_RC_OK;
    mep_instance_data_t         *data;
    vtss_mep_supp_ltm_conf_t    conf;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_LT_TIMER;

    if (data->lt_config.enable)
    {
        data->out_state.lt_state.transaction_cnt++;
        if (data->out_state.lt_state.transaction_cnt < VTSS_MEP_TRANSACTION_MAX)
        {
            data->lt_state.transaction_id++;
            data->lt_state.timer = 5000/timer_res;
            vtss_mep_timer_start();

            data->out_state.lt_state.transaction[data->out_state.lt_state.transaction_cnt].transaction_id = data->lt_state.transaction_id;

            lt_conf_calc(&conf,  data);

            rc = vtss_mep_supp_ltm_conf_set(instance, &conf);
        }
        else
        {   /* No more transactions - disable LTM */
            data->lt_config.enable = FALSE;
            conf.enable = FALSE;
            rc += vtss_mep_supp_ltm_conf_set(instance, &conf);
        }
        if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_lt_timer: Call to supp failed", instance, 0, 0, 0);
    }
}


static void run_lb_timer(u32 instance)
{
    mep_instance_data_t         *data;
    vtss_mep_supp_lbm_conf_t    conf;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_LB_TIMER;

    if (data->lb_config.enable) {
        data->lb_config.enable = FALSE;

        conf.enable = FALSE;
        if (vtss_mep_supp_lbm_conf_set(instance, &conf) != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("run_lb_timer: Call to supp failed", instance, 0, 0, 0);
    }
}


static void run_tst_timer(u32 instance)
{
    u32                         count, sec_count, rx_bits;
    vtss_mep_supp_tst_status_t  tst_state;
    mep_instance_data_t         *data;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_TST_TIMER;

    if (data->tst_state.rx_count_active) {
#if defined(VTSS_ARCH_JAGUAR_1) || !defined(VTSS_SW_OPTION_ACL) || !defined(VTSS_FEATURE_ACL_V2)
        vtss_vid_mac_t         vid_mac;
        vtss_mac_table_entry_t entry;
    
        vid_mac.vid = data->rule_vid;   /* Every second the copy to CPU is enabled in MAC table. When TST PDU is received it is disabled as all TST PDU will go to CPU and overload it */
        vtss_mep_mac_get(data->config.port, vid_mac.mac.addr);
        if (vtss_mac_table_get(NULL, &vid_mac, &entry) == VTSS_RC_OK) {
            entry.copy_to_cpu = 1;
        if (vtss_mac_table_add(NULL, &entry) != VTSS_RC_OK) vtss_mep_trace("run_tst_timer: Call to vtss_mac_table_add", instance, 0, 0, 0);
        }
#endif

        if (!data->config.voe) {   /* This is not a VOE based MEP */
            vtss_mep_acl_tst_count(instance,  &count);      /* Get ACL counter */
            sec_count = count - data->tst_state.tst_count;   /* This is received in this second */
            data->tst_state.tst_count = count;
            data->out_state.tst_state.rx_counter += sec_count;
        }
        else {  /* This is VOE based - get the TST frame counter from SUPP */
            if (vtss_mep_supp_tst_status_get(instance, &tst_state) != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("vtss_mep_mgmt_tst_state_get: Call to supp failed", instance, 0, 0, 0);
            sec_count = tst_state.rx_counter - data->tst_state.tst_count;   /* This is received in this second */
            data->tst_state.tst_count = tst_state.rx_counter;
        }

        rx_bits = sec_count * data->tst_state.rx_frame_size * 8;    /* Calculate the information rate for this second */
        data->out_state.tst_state.rx_rate = rx_bits/100000;
    }

    data->out_state.tst_state.time += 1;    /* Increment the 'Test time' */
    data->tst_state.timer = 1000/timer_res;
    vtss_mep_timer_start();
}



static void run_lm_clear(u32 instance)
{
    mep_instance_data_t     *data;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_LM_CLEAR;

    memset(&data->near_los_filter, 0, sizeof(los_filter_t));
    memset(&data->far_los_filter, 0, sizeof(los_filter_t));
    data->near_los_counter = 0;
    data->far_los_counter = 0;
    data->near_tx_counter = 0;
    data->far_tx_counter = 0;
    data->flr_interval_count = 0;

    data->out_state.lm_state.tx_counter = 0;
    data->out_state.lm_state.rx_counter = 0;
    data->out_state.lm_state.near_los_counter = 0;
    data->out_state.lm_state.far_los_counter = 0;
    data->out_state.lm_state.near_los_ratio = 0;
    data->out_state.lm_state.far_los_ratio = 0;
}


static void run_dm_clear(u32 instance)
{
    mep_instance_data_t         *data;
    vtss_mep_mgmt_dm_tunit_t    tunit_dm1, tunit_dmr;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_DM_CLEAR;

    /* Time unit must be reserved */
    tunit_dmr = data->out_state.dmr_state.tunit;
    tunit_dm1 = data->out_state.dm1_state_far_to_near.tunit;
    
    data->dmr_rec_count = 0;
    data->dmr_next_prt = 0;
    data->dmr_sum = 0;
    data->dmr_var_sum = 0;
    data->dmr_n_sum = 0;
    data->dmr_n_var_sum = 0;
    
    data->dm1_rec_count_far_to_near = 0;
    data->dm1_next_prt_far_to_near = 0;
    data->dm1_sum_far_to_near = 0;     
    data->dm1_var_sum_far_to_near = 0;
    data->dm1_n_sum_far_to_near = 0;   
    data->dm1_n_var_sum_far_to_near = 0;
    
    data->dm1_rec_count_near_to_far = 0;
    data->dm1_next_prt_near_to_far = 0;
    data->dm1_sum_near_to_far = 0;     
    data->dm1_var_sum_near_to_far = 0;
    data->dm1_n_sum_near_to_far = 0;   
    data->dm1_n_var_sum_near_to_far = 0; 

    memset(&data->out_state.dmr_state, 0, sizeof(vtss_mep_mgmt_dm_state_t));
    memset(&data->out_state.dm1_state_far_to_near, 0, sizeof(vtss_mep_mgmt_dm_state_t));
    memset(&data->out_state.dm1_state_near_to_far, 0, sizeof(vtss_mep_mgmt_dm_state_t));
    data->out_state.dmr_state.best_delay = 0xffffffff;
    data->out_state.dm1_state_far_to_near.best_delay = 0xffffffff;
    data->out_state.dm1_state_near_to_far.best_delay = 0xffffffff;
    
    /* Revert time unit */
    data->out_state.dmr_state.tunit = tunit_dmr;
    data->out_state.dm1_state_far_to_near.tunit = tunit_dm1; 
     
    memset(data->supp_state.dmr_delay, 0, sizeof(data->supp_state.dmr_delay));
    memset(data->supp_state.dmr_delay_var, 0, sizeof(data->supp_state.dmr_delay_var));
    memset(data->supp_state.dm1_delay_state_far_to_near, 0, sizeof(data->supp_state.dm1_delay_state_far_to_near));
    memset(data->supp_state.dm1_delay_state_var_far_to_near, 0, sizeof(data->supp_state.dm1_delay_state_var_far_to_near));
    memset(data->supp_state.dm1_delay_state_near_to_far, 0, sizeof(data->supp_state.dm1_delay_state_near_to_far));
    memset(data->supp_state.dm1_delay_state_var_near_to_far, 0, sizeof(data->supp_state.dm1_delay_state_var_near_to_far));

}


static void run_tst_clear(u32 instance)
{
    mep_instance_data_t     *data;

    data = &instance_data[instance];    /* Instance data reference */

    data->event_flags &= ~EVENT_IN_TST_CLEAR;

    vtss_mep_acl_tst_clear(instance);
    (void)vtss_mep_supp_tst_clear(instance);

    memset(&data->tst_state, 0, sizeof(data->tst_state));
    memset(&data->out_state.tst_state, 0, sizeof(data->out_state.tst_state));

    if (data->tst_config.enable || data->tst_config.enable_rx) {
        data->tst_state.timer = 1000/timer_res;     /* Start the TST timer only to make 'Test time' counting */
        vtss_mep_timer_start();
    }

#if defined(VTSS_ARCH_JAGUAR_1) || !defined(VTSS_SW_OPTION_ACL) || !defined(VTSS_FEATURE_ACL_V2)
    vtss_vid_mac_t         vid_mac;
    vtss_mac_table_entry_t entry;
    
    vid_mac.vid = data->rule_vid;
    vtss_mep_mac_get(data->config.port, vid_mac.mac.addr);
    if (vtss_mac_table_get(NULL, &vid_mac, &entry) == VTSS_RC_OK) {
        entry.copy_to_cpu = 1;
        if (vtss_mac_table_add(NULL, &entry) != VTSS_RC_OK) vtss_mep_trace("run_tst_clear: Call to vtss_mac_table_add", instance, 0, 0, 0);
    }
#endif
}




/****************************************************************************/
/*  MEP management interface                                                */
/****************************************************************************/

u32 vtss_mep_mgmt_conf_set(const u32                    instance,
                           const vtss_mep_mgmt_conf_t   *const conf)
{
    u32                   i, j, vid, port_cnt;
    mep_instance_data_t   *data;    /* Instance data reference */
    BOOL                  nni[VTSS_PORT_ARRAY_SIZE];

    if (instance >= VTSS_MEP_INSTANCE_MAX)                                                      return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    vid = data->rule_vid;
    port_cnt = vtss_mep_port_count();

    if (!conf->enable && !data->config.enable)                                                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}

    if (conf->enable)                                                       
    {                                                                                    
        if (!memcmp(&data->config, conf, sizeof(vtss_mep_mgmt_conf_t)))                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
        if (conf->level > 7)                                                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->mep > 0x1FFF)                                                                 {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->vid > 4095)                                                                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->port >= port_cnt)                                                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->domain == VTSS_MEP_MGMT_PORT) && (conf->flow != conf->port))                 {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->peer_count > VTSS_MEP_PEER_MAX)                                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->domain == VTSS_MEP_MGMT_PORT) && (conf->mode != VTSS_MEP_MGMT_MEP))          {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->domain == VTSS_MEP_MGMT_PORT) &&
            (conf->direction == VTSS_MEP_MGMT_EGRESS) && (conf->vid == 0))                      {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        for (i=0; i<conf->peer_count; ++i)                                               
            if (conf->peer_mep[i] > 0x1FFF)                                                     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->peer_count == 0) &&
            (data->cc_config.enable || data->lm_config.enable || data->aps_config.enable))      {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_PEER_CNT);}
        if (conf->peer_count > 1)                                                        
        {                                                                                
            for (i=0; i<conf->peer_count; ++i)                                           
                for (j=0; j<conf->peer_count; ++j)                                       
                    if ((i != j) && (conf->peer_mep[i] == conf->peer_mep[j]))                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_PEER_ID);}
        }

        for (i=0; i<VTSS_MEP_MEG_CODE_LENGTH; ++i)      if (conf->name[i] == '\0') break;
        if ((i == VTSS_MEP_MEG_CODE_LENGTH) ||
            ((conf->format == VTSS_MEP_MGMT_ITU_ICC) && (i != 6)))                              {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        for (i=0; i<VTSS_MEP_MEG_CODE_LENGTH; ++i)      if (conf->meg[i] == '\0') break;
        if ((i == VTSS_MEP_MEG_CODE_LENGTH) ||
            ((conf->format == VTSS_MEP_MGMT_ITU_ICC) && (i != 6) && (i != 7)))                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}

        if ((conf->domain == VTSS_MEP_MGMT_PORT) && (conf->direction == VTSS_MEP_MGMT_EGRESS))
        { /* An Egress port MEP is created - ATM this is only for support of ERPS version 2 */
            if (!vtss_mep_vlan_get(conf->vid, nni))                                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_VID);}
            if (!nni[conf->port])                                                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            for (i=0; i<VTSS_PORT_ARRAY_SIZE; ++i)  /* There must be a active NNI that is not my residence port */
                if ((i != conf->port) && nni[i])    break;
            if (i == VTSS_PORT_ARRAY_SIZE)                                                      {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
            {
                if ((i != instance) && (instance_data[i].config.enable) &&
                    (instance_data[i].config.domain == VTSS_MEP_MGMT_PORT) &&
                    (instance_data[i].config.vid == conf->vid))
                {/* Another port MEP is created in this VLAN - not all combinations are allowed */
                    if (instance_data[i].config.level == conf->level)
                    {/* MEP on same level already created */
                        if (instance_data[i].config.direction == VTSS_MEP_MGMT_EGRESS)                                                       {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        if ((instance_data[i].config.direction == VTSS_MEP_MGMT_INGRESS) && (conf->port != instance_data[i].config.port))    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                    }
                }
            }
        }

#if defined(VTSS_SW_OPTION_EVC)
        vtss_mep_mgmt_conf_t  *iconf;
        u32  evid, tvid, tag_cnt;
        BOOL tunnel;
        vtss_mep_tag_type_t  ttype;
        BOOL uni[VTSS_PORT_ARRAY_SIZE], up_mep=FALSE;

        if (conf->domain == VTSS_MEP_MGMT_EVC)
        {   /* Get flow info from EVC */
            if (!vtss_mep_evc_flow_info_get(conf->flow, nni, uni, &vid, &evid, &ttype, &tvid, &tunnel, &tag_cnt))       {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_EVC);}
            if (vid == 0)                                                                                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_EVC);}
#ifdef VTSS_ARCH_SERVAL                                                                                                 
#ifndef VTSS_SW_OPTION_MEP_LOOP_PORT
            if ((conf->mode == VTSS_MEP_MGMT_MEP) && (conf->direction == VTSS_MEP_MGMT_EGRESS))                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#endif
            if ((conf->mode != VTSS_MEP_MGMT_MIP) && (conf->vid != 0))                                                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            if ((conf->mode == VTSS_MEP_MGMT_MIP) && (conf->voe || (conf->vid == 0) || !uni[conf->port]))               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            if ((conf->mode == VTSS_MEP_MGMT_MEP) && (conf->direction == VTSS_MEP_MGMT_INGRESS) && (!nni[conf->port]))  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            if ((conf->mode == VTSS_MEP_MGMT_MIP) && (conf->direction == VTSS_MEP_MGMT_INGRESS) && (!uni[conf->port]))  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#else
            if (conf->vid != 0)                                                                                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            if ((conf->direction == VTSS_MEP_MGMT_INGRESS) && (!nni[conf->port]))                                       {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#endif
            if (conf->direction == VTSS_MEP_MGMT_EGRESS)
            {
#if defined(VTSS_SW_OPTION_UP_MEP)
                if (up_mep_enabled) {
                    for (i=0; i<4; ++i)
                        if (conf->port == (UP_MEP_UNI_FIRST+i))
                            up_mep = TRUE;
                }
#endif
#ifndef VTSS_ARCH_SERVAL
                if (data->cc_config.enable && vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(data->cc_config.period)) &&
                    (!up_mep))                                                                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#endif
                if ((conf->mode == VTSS_MEP_MGMT_MEP) && (!uni[conf->port]))                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                for (i=0; i<VTSS_PORT_ARRAY_SIZE; ++i)  /* There must be a active NNI that is not my residence port */
                    if ((i != conf->port) && nni[i])    break;
                if (i == VTSS_PORT_ARRAY_SIZE)                                                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            }
            for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
            {
                if ((i != instance) && (instance_data[i].config.enable) && (instance_data[i].config.domain == VTSS_MEP_MGMT_EVC) && (instance_data[i].config.flow == conf->flow))
                {/* Another EVC related Instance is created in this flow - not all combinations are allowed */
                    iconf = &instance_data[i].config;
                    if (conf->mode == VTSS_MEP_MGMT_MEP)
                    {/* MEP is created */
                        if (instance_data[i].up_mep && up_mep)     /* Only one Up-MEP allowed (DS1076) */               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#ifdef VTSS_ARCH_SERVAL
                        if (conf->voe && (conf->direction == VTSS_MEP_MGMT_INGRESS) &&
                            iconf->voe && (iconf->direction == VTSS_MEP_MGMT_INGRESS))                                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        if (conf->voe && (conf->direction == VTSS_MEP_MGMT_EGRESS) &&                                   
                            iconf->voe && (iconf->direction == VTSS_MEP_MGMT_EGRESS))                                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        if (conf->voe && (conf->direction == VTSS_MEP_MGMT_INGRESS) && (iconf->mode == VTSS_MEP_MGMT_MEP) &&
                            (iconf->direction == VTSS_MEP_MGMT_INGRESS) && (iconf->level >= conf->level))               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        if (conf->voe && (conf->direction == VTSS_MEP_MGMT_EGRESS) && (iconf->mode == VTSS_MEP_MGMT_MEP) &&
                            (iconf->direction == VTSS_MEP_MGMT_EGRESS) && (iconf->level <= conf->level))                {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        if (!conf->voe && (conf->direction == VTSS_MEP_MGMT_INGRESS) && 
                            iconf->voe && (iconf->direction == VTSS_MEP_MGMT_INGRESS) && (iconf->level <= conf->level)) {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        if (!conf->voe && (conf->direction == VTSS_MEP_MGMT_EGRESS) && 
                            iconf->voe && (iconf->direction == VTSS_MEP_MGMT_EGRESS) && (iconf->level >= conf->level))  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#endif
                        if ((iconf->direction == VTSS_MEP_MGMT_EGRESS) && (iconf->mode == VTSS_MEP_MGMT_MEP) &&
                            (iconf->level <= conf->level) && (conf->direction == VTSS_MEP_MGMT_INGRESS))                {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        if ((iconf->direction == VTSS_MEP_MGMT_INGRESS) && (iconf->mode == VTSS_MEP_MGMT_MEP) &&
                            (iconf->level >= conf->level) && (conf->direction == VTSS_MEP_MGMT_EGRESS))                 {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}

                        if ((iconf->mode == VTSS_MEP_MGMT_MEP) && (iconf->level == conf->level))
                        {/* MEP on same level already created */
                            if ((conf->direction == iconf->direction) && (conf->port == iconf->port))                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                            if ((conf->direction == VTSS_MEP_MGMT_EGRESS) && (iconf->direction == VTSS_MEP_MGMT_EGRESS)){vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        }

                        if ((iconf->mode == VTSS_MEP_MGMT_MIP) && (iconf->vid == 0))
                            if ((iconf->level <= conf->level) && (conf->port == iconf->port))                           {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                    }
                    if ((conf->mode == VTSS_MEP_MGMT_MIP) && (conf->vid == 0))
                    {/* MIP is created */
                        if ((iconf->mode == VTSS_MEP_MGMT_MIP) && (iconf->vid == 0) && (iconf->level == conf->level))
                            if ((conf->direction == iconf->direction) && (conf->port == iconf->port))                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                        if ((iconf->mode == VTSS_MEP_MGMT_MEP) && (conf->port == iconf->port))                          
                            if ((iconf->level >= conf->level) && (conf->port == iconf->port))                           {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                    }
#ifdef VTSS_ARCH_SERVAL
                    if (conf->mode == VTSS_MEP_MGMT_MIP)
                    {/* MIP is created */
                        if ((iconf->mode == VTSS_MEP_MGMT_MIP) && (conf->direction == iconf->direction))                {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                    }
#endif
                }
            }
        }
        data->up_mep = up_mep;
#else
        if (conf->domain == VTSS_MEP_MGMT_EVC)                                                                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_EVC);}
#endif
#ifdef VTSS_ARCH_SERVAL
        u32 port_voe = 0;
        u32 service_voe = 0;
        if (conf->voe) {
            for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i) {
                if (i == instance)  continue;
                if (instance_data[i].config.enable && instance_data[i].config.voe && (instance_data[i].config.domain == VTSS_MEP_MGMT_EVC)) ++service_voe;
                if (instance_data[i].config.enable && instance_data[i].config.voe && (instance_data[i].config.domain == VTSS_MEP_MGMT_PORT)) ++port_voe;
            }
            if ((service_voe >= VTSS_MEP_SUPP_SERVICE_VOE_MAX) || (port_voe >= VTSS_MEP_SUPP_PORT_VOE_MAX))     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NO_VOE);}
            for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i) {
                if (i == instance)  continue;
                if (instance_data[i].config.enable && instance_data[i].config.voe && (instance_data[i].config.domain == VTSS_MEP_MGMT_PORT) &&
                    (conf->domain == VTSS_MEP_MGMT_PORT) && (conf->port == instance_data[i].config.port))       {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            }
        }
#else
        if (conf->voe)                                                                                          {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NO_VOE);}
#endif
    }

    if (((conf->enable) && (data->config.enable) &&
         ((conf->voe && !data->config.voe) || ((conf->domain == VTSS_MEP_MGMT_PORT) && (conf->vid != data->config.vid)) || ((conf->domain == VTSS_MEP_MGMT_EVC) && (vid != data->rule_vid)))) ||
        (!conf->enable))
    {
    /* NOT VOE based is change to VOE based - OR - Tagged VID is changed on a PORT MEP - OR - VID has changed on a EVC MEP - OR - this MEP is disabled */
        /* Rules are updated with this MEP disabled - existing rules on "old" VID are maybe removed */
        data->config.enable = FALSE;
        if (!pdu_fwr_cap_update(data->rule_vid,  data->rule_ports))   vtss_mep_rule_update_failed(data->rule_vid);
    }

    if (conf->enable)   data->config = *conf;
    else                data->config.enable = FALSE;
    data->event_flags |= EVENT_IN_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_conf_get(const u32                  instance,
                           u8                         *const mac,
                           vtss_mep_mgmt_conf_t       *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)          return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (instance_data[instance].config.enable) {
        vtss_mep_mac_get(instance_data[instance].config.port, mac);  /* Get MAC of recidence port */
        peer_mac_update(&instance_data[instance]);
    }
    *conf = instance_data[instance].config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_cc_conf_set(const u32                        instance,
                              const vtss_mep_mgmt_cc_conf_t    *const conf)
{
    mep_instance_data_t     *data;    /* Instance data reference */

    if (instance >= VTSS_MEP_INSTANCE_MAX)                                                      return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!conf->enable && !data->cc_config.enable)                                                            {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
    if (!data->config.enable)                                                                                {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)                                                              {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
                                                                                                     
    if (conf->enable)                                                                                
    {                                                                                                
        if (!memcmp(&data->cc_config, conf, sizeof(vtss_mep_mgmt_cc_conf_t)))                                {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
        if (conf->period == VTSS_MEP_MGMT_PERIOD_INV)                                                        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (data->config.peer_count == 0)                                                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_PEER_CNT);}
        if (!data->config.voe) {
#if !defined(VTSS_FEATURE_AFI_FDMA) && !defined(VTSS_FEATURE_AFI_SWC)
            if (vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(conf->period)))                          {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#endif
#ifndef VTSS_ARCH_SERVAL
            if (vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(conf->period)) && (data->config.domain == VTSS_MEP_MGMT_EVC) &&
                (data->config.direction == VTSS_MEP_MGMT_EGRESS) && (!data->up_mep))                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#endif
            peer_mac_update(data);
            if (vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(conf->period)) &&
                (!memcmp(all_zero_mac, data->config.peer_mac[0], VTSS_MEP_MAC_LENGTH)))                      {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_PEER_MAC);}
        }
#if defined(VTSS_SW_OPTION_EVC) && defined(VTSS_ARCH_SERVAL)
        BOOL     cos_id[VTSS_MEP_COS_ID_SIZE];

        if (data->config.domain == VTSS_MEP_MGMT_EVC) { /* This EVC domain MEP - check for valid priority - COS-ID */
            vtss_mep_evc_cos_id_get(data->config.flow, cos_id);
            if (!cos_id[conf->prio])                                                                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_COS_ID);}
        }
#endif
    }

    if (conf->enable)   data->cc_config = *conf;
    else                data->cc_config.enable = FALSE;
    data->event_flags |= EVENT_IN_CC_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_cc_conf_get(const u32                 instance,
                              vtss_mep_mgmt_cc_conf_t   *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)               return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].cc_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lm_conf_set(const u32                      instance,
                              const vtss_mep_mgmt_lm_conf_t  *const conf)
{
    mep_instance_data_t     *data;    /* Instance data reference */

    if (instance >= VTSS_MEP_INSTANCE_MAX)                                                      return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!conf->enable && !data->lm_config.enable)                                                     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
    if (!data->config.enable)                                                                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)                                                       {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
                                                                                                      
    if (conf->enable)                                                                                 
    {                                                                                                 
        if (!memcmp(&data->lm_config, conf, sizeof(vtss_mep_mgmt_lm_conf_t)))                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
        if (conf->period == VTSS_MEP_MGMT_PERIOD_INV)                                                 {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#if defined(VTSS_ARCH_CARACAL)
        if ((conf->ended == VTSS_MEP_MGMT_DUAL_ENDED) && data->cc_config.enable &&
            vtss_mep_supp_hw_ccm_generator_get(supp_period_calc(data->cc_config.period)))             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#endif
        if (data->lm_config.enable && (conf->ended != data->lm_config.ended))                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->period != VTSS_MEP_MGMT_PERIOD_1S) && (conf->period != VTSS_MEP_MGMT_PERIOD_10S))  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (data->config.peer_count != 1)                                                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_PEER_CNT);}
        peer_mac_update(data);
        if ((conf->cast == VTSS_MEP_MGMT_UNICAST) &&                                                  
            (!memcmp(all_zero_mac, data->config.peer_mac[0], VTSS_MEP_MAC_LENGTH)))                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#if defined(VTSS_SW_OPTION_EVC) && defined(VTSS_ARCH_SERVAL)
        BOOL     cos_id[VTSS_MEP_COS_ID_SIZE];

        if (data->config.domain == VTSS_MEP_MGMT_EVC) { /* This EVC domain MEP - check for valid priority - COS-ID */
            vtss_mep_evc_cos_id_get(data->config.flow, cos_id);
            if (!cos_id[conf->prio])                                                                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_COS_ID);}
        }
#endif
    }

    if (conf->enable)   data->lm_config = *conf;
    else                data->lm_config.enable = FALSE;
    data->event_flags |= EVENT_IN_LM_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lm_conf_get(const u32                instance,
                             vtss_mep_mgmt_lm_conf_t   *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)               return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].lm_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_dm_conf_set(const u32                      instance,
                              const vtss_mep_mgmt_dm_conf_t  *const conf)
{
    mep_instance_data_t     *data;    /* Instance data reference */
    u8                      i;
    
    if (instance >= VTSS_MEP_INSTANCE_MAX)                                                      return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!data->config.enable)                                                                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)                                                 {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
    if (!memcmp(&data->dm_config, conf, sizeof(vtss_mep_mgmt_dm_conf_t)))                       {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}

    if (conf->enable)
    {
        if (data->config.peer_count == 0)                                                       {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_PEER_CNT);}
        if (conf->cast == VTSS_MEP_MGMT_UNICAST)
        {/* All zero mac - mep must be a known peer mep */
            peer_mac_update(data);
            for (i=0; i<data->config.peer_count; ++i)
                if (conf->mep == data->config.peer_mep[i]) break;
            if (i == data->config.peer_count)                                                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            if (!memcmp(all_zero_mac, data->config.peer_mac[i], VTSS_MEP_MAC_LENGTH))           {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        }
        if ((conf->way == VTSS_MEP_MGMT_TWO_WAY) &&
            (conf->txway == VTSS_MEP_MGMT_PROP) &&
            (conf->calcway == VTSS_MEP_MGMT_RDTRP))                                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (data->dm_config.enable && conf->way != data->dm_config.way)                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->gap < VTSS_MEP_MGMT_MIN_GAP) || (conf->gap > VTSS_MEP_MGMT_MAX_GAP))         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->count < VTSS_MEP_MGMT_MIN_CNT) || (conf->count > VTSS_MEP_MGMT_MAX_CNT))     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->tunit != VTSS_MEP_MGMT_US) && (conf->tunit != VTSS_MEP_MGMT_NS))             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->act != VTSS_MEP_MGMT_DISABLE) && (conf->act != VTSS_MEP_MGMT_CONTINUE))      {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}                            
#if defined(VTSS_SW_OPTION_EVC) && defined(VTSS_ARCH_SERVAL)
        BOOL     cos_id[VTSS_MEP_COS_ID_SIZE];

        if (data->config.domain == VTSS_MEP_MGMT_EVC) { /* This EVC domain MEP - check for valid priority - COS-ID */
            vtss_mep_evc_cos_id_get(data->config.flow, cos_id);
            if (!cos_id[conf->prio])                                                            {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_COS_ID);}
        }
#endif
    }

    if (conf->enable)   data->dm_config = *conf;
    else {
        data->dm_config.enable = FALSE;
        data->dm_config.tunit = conf->tunit;    /* This is only in order to change the time unit for 1DM reception - 1DM is not enabled  */
        data->dm_config.way = conf->way;
    }
    data->event_flags |= EVENT_IN_DM_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_dm_conf_get(const u32                instance,
                             vtss_mep_mgmt_dm_conf_t   *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)               return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].dm_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_aps_conf_set(const u32                       instance,
                               const vtss_mep_mgmt_aps_conf_t  *const conf)
{
    mep_instance_data_t     *data;    /* Instance data reference */

    if (instance >= VTSS_MEP_INSTANCE_MAX)                                       return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!conf->enable && !data->aps_config.enable)                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
    if (!data->config.enable)                                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)                                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
    if ((conf->type == VTSS_MEP_MGMT_R_APS) &&
        (data->config.domain != VTSS_MEP_MGMT_PORT))                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_APS_DOMAIN);}
    if ((conf->type == VTSS_MEP_MGMT_R_APS) &&
        (data->config.vid == 0))                                                 {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}

    if (conf->enable)
    {
        if (!memcmp(&data->aps_config, conf, sizeof(vtss_mep_mgmt_aps_conf_t)))  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
    }

    if (conf->enable)   data->aps_config = *conf;
    else                data->aps_config.enable = FALSE;
    data->event_flags |= EVENT_IN_APS_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_aps_conf_get(const u32                 instance,
                               vtss_mep_mgmt_aps_conf_t  *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].aps_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lt_conf_set(const u32                      instance,
                              const vtss_mep_mgmt_lt_conf_t  *const conf)
{
    u32                   i;
    mep_instance_data_t   *data;    /* Instance data reference */

    if (instance >= VTSS_MEP_INSTANCE_MAX)                              return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!data->config.enable)                                           {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (conf->enable == data->lt_config.enable)                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}

    if (conf->enable)
    {
        if (data->config.peer_count == 0)                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_PEER_CNT);}
        if (!memcmp(all_zero_mac, conf->mac, VTSS_MEP_MAC_LENGTH))
        {/* All zero mac - mep must be a known peer mep */
            peer_mac_update(data);
            for (i=0; i<data->config.peer_count; ++i)
                if (conf->mep == data->config.peer_mep[i]) break;
            if (i == data->config.peer_count)                           {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            if (!memcmp(all_zero_mac, data->config.peer_mac[i], VTSS_MEP_MAC_LENGTH))    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        }
#if defined(VTSS_SW_OPTION_EVC) && defined(VTSS_ARCH_SERVAL)
        BOOL     cos_id[VTSS_MEP_COS_ID_SIZE];

        if (data->config.domain == VTSS_MEP_MGMT_EVC) { /* This EVC domain MEP - check for valid priority - COS-ID */
            vtss_mep_evc_cos_id_get(data->config.flow, cos_id);
            if (!cos_id[conf->prio])                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_COS_ID);}
        }
#endif
    }

    if (conf->enable)   data->lt_config = *conf;
    else                data->lt_config.enable = FALSE;
    data->event_flags |= EVENT_IN_LT_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lt_conf_get(const u32                 instance,
                               vtss_mep_mgmt_lt_conf_t  *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].lt_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lb_conf_set(const u32                      instance,
                              const vtss_mep_mgmt_lb_conf_t  *const conf)
{
    u32                   i;
    mep_instance_data_t   *data;    /* Instance data reference */

    if (instance >= VTSS_MEP_INSTANCE_MAX)                              return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!data->config.enable)                                                                 {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (conf->enable == data->lb_config.enable)                                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)                                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
                                                                                              
    if (conf->enable)                                                                         
    {                                                                                         
        if ((conf->to_send != VTSS_MEP_LB_MGMT_TO_SEND_INFINITE) && conf->to_send == 0)       {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->to_send == VTSS_MEP_LB_MGMT_TO_SEND_INFINITE) && !data->config.voe)        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->size > 1400)                                                                {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->to_send != VTSS_MEP_LB_MGMT_TO_SEND_INFINITE) && (conf->interval > 100))   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((conf->to_send == VTSS_MEP_LB_MGMT_TO_SEND_INFINITE) && (conf->interval > 10000)) {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (data->config.peer_count == 0)                                                     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_PEER_CNT);}
        if (conf->cast == VTSS_MEP_MGMT_UNICAST)
        {
            if (!memcmp(all_zero_mac, conf->mac, VTSS_MEP_MAC_LENGTH))
            {/* All zero mac - mep must be a known peer mep */
                peer_mac_update(data);
                for (i=0; i<data->config.peer_count; ++i)
                    if (conf->mep == data->config.peer_mep[i]) break;
                if (i == data->config.peer_count)                                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
                if (!memcmp(all_zero_mac, data->config.peer_mac[i], VTSS_MEP_MAC_LENGTH))     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
            }
        }
#if defined(VTSS_SW_OPTION_EVC) && defined(VTSS_ARCH_SERVAL)
        BOOL     cos_id[VTSS_MEP_COS_ID_SIZE];

        if (data->config.domain == VTSS_MEP_MGMT_EVC) { /* This EVC domain MEP - check for valid priority - COS-ID */
            vtss_mep_evc_cos_id_get(data->config.flow, cos_id);
            if (!cos_id[conf->prio])                                                          {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_COS_ID);}
        }
#endif
    }

    if (conf->enable)   data->lb_config = *conf;
    else                data->lb_config.enable = FALSE;
    data->event_flags |= EVENT_IN_LB_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lb_conf_get(const u32                 instance,
                              vtss_mep_mgmt_lb_conf_t   *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)       return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].lb_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_ais_conf_set(const u32                       instance,
                               const vtss_mep_mgmt_ais_conf_t  *const conf)
{
    mep_instance_data_t     *data;    /* Instance data reference */

    if (instance >= VTSS_MEP_INSTANCE_MAX)      return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!data->config.enable)                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
    if (conf->enable) {
        if (conf->client_flow_count > VTSS_MEP_CLIENT_FLOWS_MAX)                                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (data->config.direction == VTSS_MEP_MGMT_EGRESS)                                              {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->period == VTSS_MEP_MGMT_PERIOD_INV)                                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((data->config.domain != VTSS_MEP_MGMT_PORT) && (data->config.domain != VTSS_MEP_MGMT_EVC))   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->client_domain != VTSS_MEP_MGMT_EVC)                                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((data->config.domain == VTSS_MEP_MGMT_PORT) && (conf->client_domain == VTSS_MEP_MGMT_EVC) &&
            (!conf->client_flow_count))                                                                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((data->config.domain == VTSS_MEP_MGMT_EVC) && (conf->client_flow_count))                     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
    }

    data->ais_config = *conf;

    data->event_flags |= EVENT_IN_AIS_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}

u32 vtss_mep_mgmt_ais_conf_get(const u32                 instance,
                               vtss_mep_mgmt_ais_conf_t  *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].ais_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}

u32 vtss_mep_mgmt_lck_conf_set(const u32                       instance,
                               const vtss_mep_mgmt_lck_conf_t  *const conf)
{
    mep_instance_data_t     *data;    /* Instance data reference */

    if (instance >= VTSS_MEP_INSTANCE_MAX)      return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!data->config.enable)                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
    if (conf->enable) {
        if (conf->client_flow_count > VTSS_MEP_CLIENT_FLOWS_MAX)                                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (data->config.direction == VTSS_MEP_MGMT_EGRESS)                                              {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->period == VTSS_MEP_MGMT_PERIOD_INV)                                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((data->config.domain != VTSS_MEP_MGMT_PORT) && (data->config.domain != VTSS_MEP_MGMT_EVC))   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->client_domain != VTSS_MEP_MGMT_EVC)                                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((data->config.domain == VTSS_MEP_MGMT_PORT) && (conf->client_domain == VTSS_MEP_MGMT_EVC) &&
            (!conf->client_flow_count))                                                                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if ((data->config.domain == VTSS_MEP_MGMT_EVC) && (conf->client_flow_count))                     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#if defined(VTSS_SW_OPTION_EVC) && defined(VTSS_ARCH_SERVAL)
        BOOL     cos_id[VTSS_MEP_COS_ID_SIZE];

        if (data->config.domain == VTSS_MEP_MGMT_EVC) { /* This EVC domain MEP - check for valid priority - COS-ID */
            vtss_mep_evc_cos_id_get(data->config.flow, cos_id);
            if (!cos_id[conf->client_prio])                                                              {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_COS_ID);}
        }
#endif
    }

    data->lck_config = *conf;

    data->event_flags |= EVENT_IN_LCK_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}

u32 vtss_mep_mgmt_lck_conf_get(const u32                 instance,
                               vtss_mep_mgmt_lck_conf_t  *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].lck_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}

u32 vtss_mep_mgmt_tst_conf_set(const u32                       instance,
                               const vtss_mep_mgmt_tst_conf_t  *const conf)
{
    mep_instance_data_t     *data;    /* Instance data reference */
    if (instance >= VTSS_MEP_INSTANCE_MAX)      return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    data = &instance_data[instance];
    if (!data->config.enable)                               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (data->config.mode == VTSS_MEP_MGMT_MIP)             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
#if defined(VTSS_FEATURE_AFI_FDMA) || defined(VTSS_FEATURE_AFI_SWC)
    u32 i;
    if (conf->enable)
    {
        if (!memcmp(&data->tst_config, conf, sizeof(vtss_mep_mgmt_aps_conf_t)))  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}
        if (conf->rate > VTSS_MEP_SUPP_TST_RATE_MAX)        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->rate < 1)                                 {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->size > VTSS_MEP_SUPP_TST_SIZE_MAX)        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        if (conf->size < VTSS_MEP_SUPP_TST_SIZE_MIN)        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        for (i=0; i<data->config.peer_count; ++i)
            if (conf->mep == data->config.peer_mep[i]) break;
        if (i == data->config.peer_count)                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
        peer_mac_update(data);
        if (!memcmp(all_zero_mac, data->config.peer_mac[i], VTSS_MEP_MAC_LENGTH))    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
#if defined(VTSS_SW_OPTION_EVC) && defined(VTSS_ARCH_SERVAL)
        BOOL     cos_id[VTSS_MEP_COS_ID_SIZE];

        if (data->config.domain == VTSS_MEP_MGMT_EVC) { /* This EVC domain MEP - check for valid priority - COS-ID */
            vtss_mep_evc_cos_id_get(data->config.flow, cos_id);
            if (!cos_id[conf->prio])                        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_COS_ID);}
        }
#endif
    }

    if (conf->enable || conf->enable_rx)   data->tst_config = *conf;
    else {
        data->tst_config.enable = FALSE;
        data->tst_config.enable_rx = FALSE;
    }

    data->event_flags |= EVENT_IN_TST_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
#else /* No AFI support */
    vtss_mep_crit_unlock();
    return(VTSS_MEP_RC_INVALID_PARAMETER);
#endif /* defined(VTSS_FEATURE_AFI_FDMA) || defined(VTSS_FEATURE_AFI_SWC) */

}

u32 vtss_mep_mgmt_tst_conf_get(const u32                 instance,
                               vtss_mep_mgmt_tst_conf_t  *const conf)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    *conf = instance_data[instance].tst_config;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}

u32 vtss_mep_mgmt_state_get(const u32                instance,
                            vtss_mep_mgmt_state_t    *const state)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}

    *state = instance_data[instance].out_state.state;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lm_state_get(const u32                   instance,
                               vtss_mep_mgmt_lm_state_t    *const state)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)          return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}

    *state = instance_data[instance].out_state.lm_state;
    if (state->near_los_counter < 0) state->near_los_counter = 0;
    if (state->far_los_counter < 0) state->far_los_counter = 0;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}

u32 vtss_mep_mgmt_dm_timestamp_get(const u32                    instance,
                                   vtss_mep_mgmt_dm_timestamp_t *const dm1_timestamp_far_to_near,
                                   vtss_mep_mgmt_dm_timestamp_t *const dm1_timestamp_near_to_far)
{
    u32 ret_val = VTSS_MEP_RC_OK;
    if (instance >= VTSS_MEP_INSTANCE_MAX)          return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (VTSS_MEP_SUPP_RC_OK != vtss_mep_supp_dmr_timestamps_get(instance,
                                dm1_timestamp_far_to_near,
                        dm1_timestamp_near_to_far)) {
        ret_val = VTSS_MEP_RC_NO_TIMESTAMP_DATA;
    }
    
    vtss_mep_crit_unlock();

    return(ret_val);
}



u32 vtss_mep_mgmt_dm_state_get(const u32                   instance,
                               vtss_mep_mgmt_dm_state_t    *const dmr_state,
                               vtss_mep_mgmt_dm_state_t    *const dm1_state_far_to_near,
                               vtss_mep_mgmt_dm_state_t    *const dm1_state_near_to_far)
{    
    if (instance >= VTSS_MEP_INSTANCE_MAX)          return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}

    memcpy(dmr_state, &instance_data[instance].out_state.dmr_state, sizeof(vtss_mep_mgmt_dm_state_t));
    if (dmr_state->best_delay == 0xffffffff)
        dmr_state->best_delay = 0;

    memcpy(dm1_state_far_to_near, &instance_data[instance].out_state.dm1_state_far_to_near, sizeof(vtss_mep_mgmt_dm_state_t));
    dm1_state_far_to_near->tx_cnt = 0;
    if (dm1_state_far_to_near->best_delay == 0xffffffff)
        dm1_state_far_to_near->best_delay = 0;

    memcpy(dm1_state_near_to_far, &instance_data[instance].out_state.dm1_state_near_to_far, sizeof(vtss_mep_mgmt_dm_state_t));
    dm1_state_near_to_far->rx_cnt = 0;
    dm1_state_near_to_far->rx_tout_cnt = 0;
//    dm1_state_near_to_far->rx_err_cnt = 0;
    if (dm1_state_near_to_far->best_delay == 0xffffffff)
        dm1_state_near_to_far->best_delay = 0;        

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}

u32 vtss_mep_mgmt_dm_state_clear_set(const u32    instance)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)                                 return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)                            {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)          {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}

    instance_data[instance].event_flags |= EVENT_IN_DM_CLEAR;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_dm_db_state_get(const u32  instance,
                                  u32        *delay,
                                  u32        *delay_var)
{ 
    if (instance >= VTSS_MEP_INSTANCE_MAX)          return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)     {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}

    if (instance_data[instance].dmr_rec_count == 0)
    { 
        memset(delay, 0, sizeof(u32) * instance_data[instance].dm_config.count);
        memset(delay_var, 0, sizeof(u32) * instance_data[instance].dm_config.count);  
    }    
    else 
    {
        if (instance_data[instance].dmr_rec_count < instance_data[instance].dm_config.count)
        {
            memcpy(delay, instance_data[instance].supp_state.dmr_delay, sizeof(u32) * instance_data[instance].dmr_rec_count);
            memcpy(delay_var, instance_data[instance].supp_state.dmr_delay_var, sizeof(u32) * instance_data[instance].dmr_rec_count);
        }
        else
        {
            /* overwritting occurs */
            memcpy(delay, 
               &instance_data[instance].supp_state.dmr_delay[instance_data[instance].dmr_next_prt],
               sizeof(u32) * (instance_data[instance].dm_config.count - instance_data[instance].dmr_next_prt));
            memcpy(&delay[instance_data[instance].dm_config.count - instance_data[instance].dmr_next_prt],
               &instance_data[instance].supp_state.dmr_delay[0],
               sizeof(u32) * (instance_data[instance].dmr_next_prt));        
            memcpy(delay_var,
               &instance_data[instance].supp_state.dmr_delay_var[instance_data[instance].dmr_next_prt],
               sizeof(u32) * (instance_data[instance].dm_config.count - instance_data[instance].dmr_next_prt));
            memcpy(&delay_var[instance_data[instance].dm_config.count - instance_data[instance].dmr_next_prt],
               &instance_data[instance].supp_state.dmr_delay_var[0],
               sizeof(u32) * (instance_data[instance].dmr_next_prt));               
        }
    }    
    
    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lt_state_get(const u32                  instance,
                               vtss_mep_mgmt_lt_state_t   *const state)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}

    *state = instance_data[instance].out_state.lt_state;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lb_state_get(const u32                  instance,
                               vtss_mep_mgmt_lb_state_t   *const state)
{
    vtss_mep_supp_lb_status_t  status;

    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}

    if (instance_data[instance].lb_config.to_send != VTSS_MEP_LB_MGMT_TO_SEND_INFINITE) {   /* This is not HW generated LBM with VOE support - LB status is generated here in upper layer */
        if (vtss_mep_supp_lb_status_get(instance, &status) == VTSS_MEP_SUPP_RC_OK) {
            instance_data[instance].out_state.lb_state.transaction_id = status.trans_id;
            instance_data[instance].out_state.lb_state.lbm_transmitted = status.lbm_counter;
        }
        *state = instance_data[instance].out_state.lb_state;
    }
    else
    {
        if (vtss_mep_supp_lb_status_get(instance, &status) == VTSS_MEP_SUPP_RC_OK) {    /* This is HW generated LBM with VOE support - LB status is taken from VOE */
            instance_data[instance].out_state.lb_state.transaction_id = status.trans_id;
            instance_data[instance].out_state.lb_state.lbm_transmitted = status.lbm_counter;
            instance_data[instance].out_state.lb_state.reply_cnt = 1;
            memset(instance_data[instance].out_state.lb_state.reply[0].mac, 0, sizeof(instance_data[instance].out_state.lb_state.reply[0].mac));
            instance_data[instance].out_state.lb_state.reply[0].lbr_received = status.lbr_counter;
            instance_data[instance].out_state.lb_state.reply[0].out_of_order = status.oo_counter;
            *state = instance_data[instance].out_state.lb_state;
        }
    }

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_lm_state_clear_set(const u32    instance)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)                                 return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)                            {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)          {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}

    instance_data[instance].event_flags |= EVENT_IN_LM_CLEAR;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_tst_state_get(const u32                   instance,
                                vtss_mep_mgmt_tst_state_t   *const state)
{
    vtss_mep_supp_tst_status_t  supp_state;

    if (instance >= VTSS_MEP_INSTANCE_MAX)             return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}

    if (vtss_mep_supp_tst_status_get(instance, &supp_state) != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("vtss_mep_mgmt_tst_state_get: Call to supp failed", instance, 0, 0, 0);
    instance_data[instance].out_state.tst_state.tx_counter = supp_state.tx_counter;
    if (instance_data[instance].config.voe)    /* This is a VOE based MEP */
        instance_data[instance].out_state.tst_state.rx_counter = supp_state.rx_counter;
    instance_data[instance].out_state.tst_state.oo_counter = supp_state.oo_counter;
    *state = instance_data[instance].out_state.tst_state;

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_tst_state_clear_set(const u32    instance)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)                                 return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    if (!instance_data[instance].config.enable)                            {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)          {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}

    instance_data[instance].event_flags |= EVENT_IN_TST_CLEAR;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_mgmt_change_set(const u32   instance)
{
    if (instance >= VTSS_MEP_INSTANCE_MAX)           return(VTSS_MEP_RC_INVALID_PARAMETER);

    vtss_mep_crit_lock();

    instance_data[instance].event_flags |= EVENT_IN_CONFIG;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


#if defined(VTSS_SW_OPTION_UP_MEP)
void vtss_mep_mgmt_up_mep_enable(void)
{
    vtss_mep_crit_lock();
    up_mep_enabled = TRUE;
    vtss_mep_crit_unlock();
}
#endif


/****************************************************************************/
/*  MEP module interface                                                    */
/****************************************************************************/

u32 vtss_mep_tx_aps_info_set(const u32    instance,
                             const u8     *aps,
                             const BOOL   event)
{
    u32   rc = VTSS_MEP_SUPP_RC_OK;

    vtss_mep_crit_lock();

    if (instance >= VTSS_MEP_INSTANCE_MAX)                                                {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
    if (!instance_data[instance].config.enable)                                           {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
    if (event && (instance_data[instance].aps_config.type != VTSS_MEP_MGMT_R_APS))        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
    if ((instance_data[instance].aps_config.type == VTSS_MEP_MGMT_R_APS) && 
        !event && !memcmp(instance_data[instance].tx_aps, aps, VTSS_MEP_APS_DATA_LENGTH)) {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_OK);}

    memcpy(instance_data[instance].tx_aps, aps, VTSS_MEP_APS_DATA_LENGTH);

    vtss_mep_crit_unlock();

    rc = vtss_mep_supp_aps_txdata_set(instance, aps, event);

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("vtss_mep_tx_aps_info_set: Call to supp failed", instance, 0, 0, 0);

    return(VTSS_MEP_RC_OK);
}


void vtss_mep_new_ssf_state(const u32    instance,
                            const BOOL   state)
{
    BOOL                    aTsf_c, aTsd_c;
    vtss_mep_mgmt_domain_t  domain_c;

    vtss_mep_crit_lock();

    if (instance >= VTSS_MEP_INSTANCE_MAX)                                         {vtss_mep_crit_unlock(); return;}
    if (!instance_data[instance].config.enable)                                    {vtss_mep_crit_unlock(); return;}
    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)                  {vtss_mep_crit_unlock(); return;}

    instance_data[instance].ssf_state = state;

    instance_data[instance].event_flags |= EVENT_IN_SSF_STATE;

    run_ssf_state(instance);
//    vtss_mep_run();   No longer asynchronious

    if (instance_data[instance].event_flags & EVENT_OUT_SF_SD)
    {   /* Transmit SF/SD info to EPS */
        instance_data[instance].event_flags &= ~EVENT_OUT_SF_SD;
        domain_c = instance_data[instance].config.domain;
        aTsf_c = instance_data[instance].out_state.state.aTsf;
        aTsd_c = instance_data[instance].out_state.state.aTsd;

        vtss_mep_crit_unlock();
        vtss_mep_sf_sd_state_set(instance,  domain_c,   aTsf_c,   aTsd_c);
        return;
    }

    vtss_mep_crit_unlock();
}


u32 vtss_mep_signal_in(const u32   instance)
{
    vtss_mep_crit_lock();

    if (instance >= VTSS_MEP_INSTANCE_MAX)                             {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
    if (!instance_data[instance].config.enable)                        {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)      {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}

    if (instance_data[instance].aps_config.enable)
        instance_data[instance].event_flags |= EVENT_OUT_APS;

    instance_data[instance].event_flags |= EVENT_OUT_SF_SD;

    vtss_mep_run();

    vtss_mep_crit_unlock();

    return(VTSS_MEP_RC_OK);
}


u32 vtss_mep_raps_forwarding(const u32    instance,
                             const BOOL   enable)
{
    BOOL  ret = TRUE;

    vtss_mep_crit_lock();

    if (instance >= VTSS_MEP_INSTANCE_MAX)                                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
    if (!instance_data[instance].config.enable)                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}
    if (instance_data[instance].config.domain != VTSS_MEP_MGMT_PORT)               {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
    if (instance_data[instance].config.vid == 0)                                   {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}

    instance_data[instance].raps_forward = enable;

    raps_forwarding_control(instance_data[instance].config.vid);

    vtss_mep_crit_unlock();

    return((ret) ? VTSS_MEP_RC_OK : VTSS_MEP_RC_INVALID_PARAMETER);
}


u32 vtss_mep_raps_transmission(const u32    instance,
                               const BOOL   enable)
{
    u32   rc = VTSS_MEP_SUPP_RC_OK;

    vtss_mep_crit_lock();

    if (instance >= VTSS_MEP_INSTANCE_MAX)                                         {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_INVALID_PARAMETER);}
    if (!instance_data[instance].config.enable)                                    {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_NOT_ENABLED);}
    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)                  {vtss_mep_crit_unlock(); return(VTSS_MEP_RC_MIP);}

    vtss_mep_crit_unlock();

    rc = vtss_mep_supp_raps_transmission(instance, enable);

    if (rc != VTSS_MEP_SUPP_RC_OK)       vtss_mep_trace("vtss_mep_raps_transmission: Call to supp failed", instance, 0, 0, 0);

    return(VTSS_MEP_RC_OK);
}




/****************************************************************************/
/*  MEP lower support interface                                             */
/****************************************************************************/

void vtss_mep_supp_new_aps(const u32    instance,
                           const u8     *aps)
{
    vtss_mep_mgmt_domain_t  domain_c;

    vtss_mep_crit_lock();

    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)          {vtss_mep_crit_unlock();    return;}

    memcpy(instance_data[instance].out_state.rx_aps, aps, VTSS_MEP_APS_DATA_LENGTH);
    domain_c = instance_data[instance].config.domain;

    vtss_mep_crit_unlock();

    vtss_mep_rx_aps_info_set(instance,   domain_c,   aps);
}


void vtss_mep_supp_new_defect_state(const u32  instance)
{
    vtss_mep_crit_lock();

    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)          {vtss_mep_crit_unlock();    return;}

    instance_data[instance].event_flags |= EVENT_IN_DEFECT_STATE;
    vtss_mep_run();

    vtss_mep_crit_unlock();
}


void vtss_mep_supp_new_ccm_state(const u32  instance)
{
    vtss_mep_crit_lock();

    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)          {vtss_mep_crit_unlock();    return;}

    instance_data[instance].event_flags |= EVENT_IN_CCM_STATE;
    vtss_mep_run();

    vtss_mep_crit_unlock();
}


void vtss_mep_supp_new_ltr(const u32     instance)
{
    vtss_mep_crit_lock();

    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)          {vtss_mep_crit_unlock(); return;}

    instance_data[instance].event_flags |= EVENT_IN_LTR_NEW;
    vtss_mep_run();

    vtss_mep_crit_unlock();
}



void vtss_mep_supp_new_lbr(const u32     instance)
{
    vtss_mep_crit_lock();

    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)          {vtss_mep_crit_unlock(); return;}

    instance_data[instance].event_flags |= EVENT_IN_LBR_NEW;
    vtss_mep_run();

    vtss_mep_crit_unlock();
}


void vtss_mep_supp_new_dmr(const u32     instance)
{
    vtss_mep_crit_lock();

    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)         {vtss_mep_crit_unlock(); return;}
    if (!instance_data[instance].dm_config.enable)                        {vtss_mep_crit_unlock(); return;}

    instance_data[instance].event_flags |= EVENT_IN_DMR_NEW;
    vtss_mep_run();
    
    vtss_mep_crit_unlock();
}


void vtss_mep_supp_new_dm1(const u32  instance)
{
    vtss_mep_crit_lock();

    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)         {vtss_mep_crit_unlock(); return;}
    
    instance_data[instance].event_flags |= EVENT_IN_1DM_NEW;
    vtss_mep_run();

    vtss_mep_crit_unlock();
}


void vtss_mep_supp_new_tst(const u32   instance,
                           const u32   frame_size)
{
    vtss_mep_crit_lock();

    if (instance_data[instance].config.mode == VTSS_MEP_MGMT_MIP)         {vtss_mep_crit_unlock(); return;}
    
    instance_data[instance].event_flags |= EVENT_IN_TST_NEW;
    instance_data[instance].tst_state.rx_frame_size = frame_size;
    vtss_mep_run();

    vtss_mep_crit_unlock();
}


void vtss_mep_supp_counters_get(const u32       instance,
                                u32             *const rx_frames,
                                u32             *const tx_frames)
{

    vtss_mep_crit_lock();

    if (instance_data[instance].config.domain == VTSS_MEP_MGMT_PORT)
    {
        vtss_basic_counters_t   counters;

        if (vtss_port_basic_counters_get(0, instance_data[instance].config.flow, &counters) != VTSS_RC_OK)    {vtss_mep_crit_unlock(); return;}

        *rx_frames = counters.rx_frames;
        *tx_frames = counters.tx_frames;
    }
#if defined(VTSS_SW_OPTION_EVC)
    else
    if (instance_data[instance].config.domain == VTSS_MEP_MGMT_EVC)
    {
#if defined(VTSS_ARCH_JAGUAR_1)
        vtss_evc_counters_t   counters;

        if (vtss_mep_evc_counters_get(instance, instance_data[instance].config.flow, instance_data[instance].config.port, &counters))     {vtss_mep_crit_unlock(); return;}

        *rx_frames = counters.rx_green.frames + counters.rx_yellow.frames;
        *tx_frames = counters.tx_green.frames + counters.tx_yellow.frames;
#endif
#if defined(VTSS_ARCH_CARACAL)
        vtss_port_counters_t     counters, iLoop_counters, eLoop_counters;
        if (instance_data[instance].up_mep) { /* This MEP is an Up-MEP according to DS1076 */
            if (vtss_mep_port_counters_get(instance, instance_data[instance].config.port, &counters))                                     {vtss_mep_crit_unlock(); return;}
            if (vtss_mep_port_counters_get(instance, instance_data[instance].config.port+UP_MEP_INGRESS_LOOP_OFFSET, &iLoop_counters))    {vtss_mep_crit_unlock(); return;}
            if (vtss_mep_port_counters_get(instance, instance_data[instance].config.port+UP_MEP_EGRESS_LOOP_OFFSET, &eLoop_counters))     {vtss_mep_crit_unlock(); return;}

            *rx_frames = eLoop_counters.evc.tx_green[instance_data[instance].config.evc_qos] + eLoop_counters.evc.tx_yellow[instance_data[instance].config.evc_qos];
            *tx_frames = counters.evc.rx_green[instance_data[instance].config.evc_qos] + counters.evc.rx_yellow[instance_data[instance].config.evc_qos] +
                         iLoop_counters.evc.rx_green[instance_data[instance].config.evc_qos] + iLoop_counters.evc.rx_yellow[instance_data[instance].config.evc_qos];
        }
        else {
            if (vtss_mep_port_counters_get(instance, instance_data[instance].config.port, &counters))           {vtss_mep_crit_unlock(); return;}
            *rx_frames = counters.evc.rx_green[instance_data[instance].config.evc_qos] + counters.evc.rx_yellow[instance_data[instance].config.evc_qos];
            *tx_frames = counters.evc.tx_green[instance_data[instance].config.evc_qos] + counters.evc.tx_yellow[instance_data[instance].config.evc_qos];
        }
#endif
    }
#endif
    else    {vtss_mep_crit_unlock(); return;}

    vtss_mep_crit_unlock();
}





/****************************************************************************/
/*  MEP platform interface                                                  */
/****************************************************************************/

#if defined(VTSS_SW_OPTION_UP_MEP)
void up_mep_init(void)
{
    u32                    i;
    vtss_mce_t             mce;
    vtss_eps_port_conf_t   port_conf;

    if (!vtss_up_mep_acl_add(UP_MEP_PAG_OAM, UP_MEP_PAG_PORT, UP_MEP_UNI_FIRST, (UP_MEP_UNI_FIRST+UP_MEP_EGRESS_LOOP_OFFSET)))
        vtss_mep_trace("vtss_mep_run_thread: Call to vtss_up_mep_acl_add() failed", 0, 0, 0, 0);

    if (vtss_mce_init(NULL, &mce) != VTSS_RC_OK)    vtss_mep_trace("vtss_mep_run_thread: Call to vtss_mce_init() failed", 0, 0, 0, 0);

    /* Common OAM catching IS1 entry on ingress Loop UNI - this is now the 'LAST' MCE entry, all other MCE in front of this */
    mce.id = UP_MEP_U_OAM_ENTRY;
    for (i=0; i<4; ++i)
        mce.key.port_list[UP_MEP_UNI_FIRST + UP_MEP_INGRESS_LOOP_OFFSET + i] = TRUE;
    mce.action.pop_cnt = 1;
    mce.action.prio_enable = TRUE;
    mce.action.prio = UP_MEP_QOS_OAM;
    mce.action.policy_no = UP_MEP_PAG_OAM;
    if (vtss_mce_add(NULL, VTSS_MCE_ID_LAST, &mce) != VTSS_RC_OK)    vtss_mep_trace("vtss_mep_run_thread: Call to vtss_mce_add() failed", 0, 0, 0, 0);

    /* This port protection assure that all service frames send to UNI is dublicated to egress loop port */
    port_conf.type = VTSS_EPS_PORT_1_PLUS_1;
    for (i=0; i<4; ++i) {
        port_conf.port_no = UP_MEP_UNI_FIRST + UP_MEP_EGRESS_LOOP_OFFSET + i;
        if (vtss_eps_port_conf_set(NULL, UP_MEP_UNI_FIRST + i, &port_conf) != VTSS_RC_OK)                       vtss_mep_trace("vtss_mep_run_thread: Call to vtss_eps_port_conf_set() failed", 0, 0, 0, 0);
        if (vtss_eps_port_selector_set(NULL, UP_MEP_UNI_FIRST + i, VTSS_EPS_SELECTOR_WORKING) != VTSS_RC_OK)    vtss_mep_trace("vtss_mep_run_thread: Call to vtss_eps_port_selector_set() failed", 0, 0, 0, 0);
    }
}
#endif

void vtss_mep_timer_thread(BOOL  *const stop)
{
    u32  i, j;
    BOOL run;

    run = FALSE;
    *stop = TRUE;

    vtss_mep_crit_lock();

    if (lm_timer)
    {
        lm_timer--;
        if (!lm_timer)
        {
            lm_timer = 1000/timer_res;
            for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
                if (instance_data[i].lm_config.enable)   instance_data[i].event_flags |= EVENT_IN_LM_TIMER;
            run = TRUE;
            *stop = FALSE;
        }
    }

    if (update_rule_timer)
    {
        update_rule_timer--;
        if (!update_rule_timer)
        {
            for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
            {
                if (instance_data[i].update_rule)
                { /* Rules need to be updated for this VID */
                    if (!pdu_fwr_cap_update(instance_data[i].rule_vid,  instance_data[i].rule_ports))   vtss_mep_rule_update_failed(instance_data[i].rule_vid);
                    for (j=0; j<VTSS_MEP_INSTANCE_MAX; ++j) /* All instances with this VID is now updated */
                        if (instance_data[i].rule_vid == instance_data[j].rule_vid)     instance_data[j].update_rule = FALSE;
                }
            }
        }
        else
            *stop = FALSE;
    }

    for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
    {
        if (!instance_data[i].config.enable) continue;
        if (instance_data[i].lt_state.timer)
        {
            instance_data[i].lt_state.timer--;
            if (!instance_data[i].lt_state.timer)
            {
                instance_data[i].event_flags |= EVENT_IN_LT_TIMER;
                run = TRUE;
            }
            else
                *stop = FALSE;
        }
        if (instance_data[i].lb_state.timer)
        {
            instance_data[i].lb_state.timer--;
            if (!instance_data[i].lb_state.timer)
            {
                instance_data[i].event_flags |= EVENT_IN_LB_TIMER;
                run = TRUE;
            }
            else
                *stop = FALSE;
        }
        if (instance_data[i].tst_state.timer)
        {
            instance_data[i].tst_state.timer--;
            if (!instance_data[i].tst_state.timer)
            {
                instance_data[i].event_flags |= EVENT_IN_TST_TIMER;
                run = TRUE;
            }
            else
                *stop = FALSE;
        }
    }

    if (run)    vtss_mep_run();

    vtss_mep_crit_unlock();
}


void vtss_mep_run_thread(void)
{
    u32                     i;
    u8                      aps_c[VTSS_MEP_APS_DATA_LENGTH];
    BOOL                    aTsf_c, aTsd_c;
    vtss_mep_mgmt_domain_t  domain_c;

#if defined(VTSS_SW_OPTION_UP_MEP)
static BOOL init = TRUE;
    if (init & up_mep_enabled) {
        init = FALSE;
        up_mep_init();
    }
#endif

    vtss_mep_crit_lock();

    for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
    {
        if (instance_data[i].event_flags & EVENT_IN_MASK)
        {/* New input */
            if (instance_data[i].event_flags & EVENT_IN_TX_APS)         run_tx_aps(i);
            if (instance_data[i].event_flags & EVENT_IN_CONFIG)         run_config(i);
            if (instance_data[i].event_flags & EVENT_IN_CC_CONFIG)      run_cc_config(i);
            if (instance_data[i].event_flags & EVENT_IN_LM_CONFIG)      run_lm_config(i);
            if (instance_data[i].event_flags & EVENT_IN_DM_CONFIG)      run_dm_config(i);
            if (instance_data[i].event_flags & EVENT_IN_APS_CONFIG)     run_aps_config(i);
            if (instance_data[i].event_flags & EVENT_IN_LT_CONFIG)      run_lt_config(i);
            if (instance_data[i].event_flags & EVENT_IN_LB_CONFIG)      run_lb_config(i);
            if (instance_data[i].event_flags & EVENT_IN_AIS_CONFIG)     run_ais_config(i);
            if (instance_data[i].event_flags & EVENT_IN_LCK_CONFIG)     run_lck_config(i);
            if (instance_data[i].event_flags & EVENT_IN_TST_CONFIG)     run_tst_config(i);
            if (instance_data[i].event_flags & EVENT_IN_APS_INFO)       run_aps_info(i);
            if (instance_data[i].event_flags & EVENT_IN_DEFECT_STATE)   run_defect_state(i);
            if (instance_data[i].event_flags & EVENT_IN_SSF_STATE)      run_ssf_state(i);
            if (instance_data[i].event_flags & EVENT_IN_LM_CLEAR)       run_lm_clear(i);
            if (instance_data[i].event_flags & EVENT_IN_DM_CLEAR)       run_dm_clear(i);
            if (instance_data[i].event_flags & EVENT_IN_TST_CLEAR)      run_tst_clear(i);
            if (instance_data[i].event_flags & EVENT_IN_LTR_NEW)        run_ltr_new(i);
            if (instance_data[i].event_flags & EVENT_IN_LBR_NEW)        run_lbr_new(i);
            if (instance_data[i].event_flags & EVENT_IN_DMR_NEW)        run_dmr_new(i);
            if (instance_data[i].event_flags & EVENT_IN_1DM_NEW)        run_dm1_new(i);    
            if (instance_data[i].event_flags & EVENT_IN_TST_NEW)        run_tst_new(i);    
            if (instance_data[i].event_flags & EVENT_IN_LM_TIMER)       run_lm_timer(i);
            if (instance_data[i].event_flags & EVENT_IN_LT_TIMER)       run_lt_timer(i);
            if (instance_data[i].event_flags & EVENT_IN_LB_TIMER)       run_lb_timer(i);
            if (instance_data[i].event_flags & EVENT_IN_TST_TIMER)      run_tst_timer(i);
        }

        if (instance_data[i].event_flags & EVENT_OUT_MASK)
        {/* New output */
            /* Now do unlocked call-out to avoid any deadlock */
            if (instance_data[i].event_flags & EVENT_OUT_APS)
            {   /* Transmit APS info to EPS */
                instance_data[i].event_flags &= ~EVENT_OUT_APS;
    
                /* Copy data for call-out */
                domain_c = instance_data[i].config.domain;
                memcpy(aps_c, instance_data[i].out_state.rx_aps, VTSS_MEP_APS_DATA_LENGTH);
    
                vtss_mep_crit_unlock();
                vtss_mep_rx_aps_info_set(i,   domain_c,   aps_c);
                vtss_mep_crit_lock();
            }
            if (instance_data[i].event_flags & EVENT_OUT_SF_SD)
            {   /* Transmit SF/SD info to EPS */
                instance_data[i].event_flags &= ~EVENT_OUT_SF_SD;
    
                /* Copy data for call-out */
                domain_c = instance_data[i].config.domain;
                aTsf_c = instance_data[i].out_state.state.aTsf;
                aTsd_c = instance_data[i].out_state.state.aTsd;
    
                vtss_mep_crit_unlock();
                vtss_mep_sf_sd_state_set(i,  domain_c,   aTsf_c,   aTsd_c);
                vtss_mep_crit_lock();
            }
            if (instance_data[i].event_flags & EVENT_OUT_SIGNAL)
            {   /* Send signal */
                instance_data[i].event_flags &= ~EVENT_OUT_SIGNAL;
    
                domain_c = instance_data[i].config.domain;
    
                vtss_mep_crit_unlock();
                vtss_mep_signal_out(i,   domain_c);
                vtss_mep_crit_lock();
            }
        }
    }
    vtss_mep_crit_unlock();
}


#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL) || defined(VTSS_ARCH_JAGUAR_1)
static vtss_mtimer_t hit_timer;
static u32 hit_inx=0;
static u32 poll_cnt=0;
void vtss_mep_ccm_thread(void)
{
    u32        rc=0, i, hit_cnt=0;
    BOOL       stop=FALSE;
    u32        counter;
    vtss_rc    rc1=VTSS_RC_OK;

    vtss_mep_crit_lock();

    poll_cnt++;
    for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
    {
        if (instance_data[i].cc_config.enable && !instance_data[i].config.voe &&
            ((instance_data[i].cc_config.period == VTSS_MEP_MGMT_PERIOD_300S) || (instance_data[i].cc_config.period == VTSS_MEP_MGMT_PERIOD_100S)))
        {/* Active HW based CCM - poll ACL counter. */
            hit_cnt++;

            if ((instance_data[i].cc_config.period == VTSS_MEP_MGMT_PERIOD_300S) || (poll_cnt%3 == 0))
            { /* The 3.3 ms interval is polled every 10 ms and the 10 ms interval is polled every 30 ms */
                vtss_mep_acl_ccm_count(i, &counter);
                if ((counter - instance_data[i].ace_count) == 0)    rc += vtss_mep_supp_loc_state(i, TRUE);
                else                                                rc += vtss_mep_supp_loc_state(i, FALSE);
                instance_data[i].ace_count = counter;
            }
        }
    }

    if (hit_cnt)
    {   /* There are some MEP with active HW CCM */
        if (VTSS_MTIMER_TIMEOUT(&hit_timer))
        {
            i = 0;
            stop = FALSE;
            do
            {
                if (instance_data[hit_inx].cc_config.enable && !instance_data[hit_inx].config.voe &&
                    ((instance_data[hit_inx].cc_config.period == VTSS_MEP_MGMT_PERIOD_300S) || (instance_data[hit_inx].cc_config.period == VTSS_MEP_MGMT_PERIOD_100S)))
                {
                    vtss_mep_acl_ccm_hit(hit_inx);
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
                    instance_data[hit_inx].ace_count = 0;   /* On Luton26 and Serval the ACE counter is cleared in order to trick Hit Me Once */
#endif
                    stop = TRUE;
                }
                hit_inx++;
                if (hit_inx>=VTSS_MEP_INSTANCE_MAX)     hit_inx = 0;
            }
            while ((++i < VTSS_MEP_INSTANCE_MAX) && !stop);

            VTSS_MTIMER_START(&hit_timer, 950/hit_cnt);    /* All MEP with active HW CCM must be "hit" once a second */
        }
    }
    else
        hit_inx = 0;

    if (rc || (rc1 != VTSS_RC_OK))        vtss_mep_trace("vtss_mep_ccm_thread: error during poll", 0, 0, 0, 0);

    vtss_mep_crit_unlock();
}
#endif


u32 vtss_mep_init(const u32  timer_resolution)
{
    u32         i;

    if ((timer_resolution == 0) || (timer_resolution > 100))     return(VTSS_MEP_RC_INVALID_PARAMETER);

    timer_res = timer_resolution;

    /* Initialize all instance data */
    for (i=0; i<VTSS_MEP_INSTANCE_MAX; ++i)
        instance_data_clear(i, &instance_data[i]);

    memset(vlan_raps_acl_id, 0xFF, sizeof(vlan_raps_acl_id));
    memset(vlan_raps_mac_octet, 0xFF, sizeof(vlan_raps_mac_octet));

#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1)
    VTSS_MTIMER_START(&hit_timer, 1000);
#endif
    return(VTSS_MEP_RC_OK);
}


/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
