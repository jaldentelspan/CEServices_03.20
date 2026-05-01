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

#include "cli.h"
#include "qos_api.h"
#include "qos_cli.h"
#include "cli_trace_def.h"
#include "mgmt_api.h"
#include "port_api.h"

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
#include "ce_max_api.h"
#endif

/* Simplify the QCL include test using a local #define */
#undef QOS_QCL_INCLUDE

#if defined(VTSS_SW_OPTION_QOS_QCL_INCLUDE)
# if VTSS_SW_OPTION_QOS_QCL_INCLUDE == 1
# define QOS_QCL_INCLUDE /* Force inclusion */
# endif
#elif defined(VTSS_ARCH_JAGUAR_1)
# if defined(VTSS_SW_OPTION_BUILD_CE)
# define QOS_QCL_INCLUDE /* Only include in CEServices on Jaguar1 builds */
# endif
#else
# define QOS_QCL_INCLUDE /* Default inclusion */
#endif /* defined(VTSS_SW_OPTION_QOS_QCL_INCLUDE) */

#ifndef QOS_QCL_INCLUDE
# undef VTSS_FEATURE_QCL_V2 /* Remove QCL from user interface */
#endif

/*
 * I have decided to split the Luton28 and Jaguar/Luton26/Serval CLI into two separate
 * implementations instead of using a lot of #ifdef's all over the code.
 * Less than 5% of the Luton28 code can currently be reused for Jaguar/Luton26,
 * but if the QCL for Jaguar/Luton26/Serval can reuse more of the Luton28 code,
 * we might need to reconsider this decision.
 * JEA - 12.Oct 2009
 */

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
#include "qos_features.h"

#ifdef VTSS_FEATURE_QCL_V2
#if VTSS_SWITCH_STACKABLE
#include "topo_api.h"
#endif /* VTSS_SWITCH_STACKABLE */
#endif /* VTSS_FEATURE_QCL_V2 */

#ifdef  _VTSS_QOS_FEATURES_H_ // This is just a hack in order to keep lint happy :-)
#endif

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
/* Make it look exactly like a Luton26 regarding port policers */
#undef VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL
#undef VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

/****************************************************************************/
/*  JAGUAR/Luton26/Serval code starts here                                  */
/****************************************************************************/
typedef struct {
    BOOL                   policer_list[QOS_PORT_POLICER_CNT + 1];    // valid index: 1..QOS_PORT_POLICER_CNT
    BOOL                   queue_list[QOS_PORT_QUEUE_CNT];            // valid index: 0..QOS_PORT_QUEUE_CNT -1
    BOOL                   class_list[QOS_PORT_PRIO_CNT];             // valid index: 0..QOS_PORT_PRIO_CNT - 1
    BOOL                   dpl_list[2];                               // valid index: 0..1
    BOOL                   pcp_list[VTSS_PCPS];                       // valid index: 0..VTSS_PCPS - 1
    BOOL                   dei_list[VTSS_DEIS];                       // valid index: 0..VTSS_DEIS - 1
    vtss_tagprio_t         pcp;
    cli_spec_t             bit_rate_spec;
    vtss_bitrate_t         bit_rate;
    BOOL                   kbps;
    BOOL                   fps;
    vtss_dp_level_t        dp_bypass_level;
    BOOL                   weighted;
    vtss_pct_t             weight;
    vtss_dei_t             dei;
    BOOL                   dei_valid;
    vtss_dp_level_t        dpl;
    BOOL                   dpl_valid;
    vtss_tag_remark_mode_t tag_remark_mode;
    vtss_dei_t             tag_dp_map[QOS_PORT_TR_DPL_CNT];
    uint                   tag_dp_map_cnt;
    cli_spec_t             packet_rate_spec;
    vtss_packet_rate_t     packet_rate;
#ifdef VTSS_FEATURE_QOS_WRED
    vtss_pct_t             min_th;
    BOOL                   min_th_valid;
    vtss_pct_t             mdp_1;
    BOOL                   mdp_1_valid;
    vtss_pct_t             mdp_2;
    BOOL                   mdp_2_valid;
    vtss_pct_t             mdp_3;
    BOOL                   mdp_3_valid;
#endif /* VTSS_FEATURE_QOS_WRED */
#ifdef VTSS_FEATURE_QCL_V2
#if VTSS_SWITCH_STACKABLE
    cli_spec_t            usid_qce_spec;
    vtss_usid_t           usid_qce;
#endif
    vtss_qce_id_t          qce_id;
    vtss_qce_id_t          qce_id_next;

    cli_spec_t             port_spec;

    /* QCE key fields */
    cli_spec_t             tag_spec;
    vtss_qce_bit_t         tag;
    cli_spec_t             vid_spec;
    vtss_vid_t             vid_min;
    vtss_vid_t             vid_max;
    cli_spec_t             qce_pcp_spec;
    uchar                  qce_pcp_low;
    uchar                  qce_pcp_high;
    cli_spec_t             qce_dei_spec;
    vtss_qce_bit_t         qce_dei;
    cli_spec_t             smac_spec;
    uchar                  smac[3];
    cli_spec_t             dmac_type_spec;
    qos_qce_dmac_type_t    dmac_type;

    cli_spec_t             qce_type_spec;
    vtss_qce_type_t        qce_type;

    /* etype */
    cli_spec_t             etype_spec;
    vtss_etype_t           etype;

    /* LLC/SNAP */
    cli_spec_t             dsap_spec;
    uchar                  dsap;
    cli_spec_t             ssap_spec;
    uchar                  ssap;
    cli_spec_t             control_spec;
    uchar                  control;
    cli_spec_t             snap_pid_spec;
    u16                    snap_pid;

    cli_spec_t             protocol_spec;
    uchar                  protocol;
    cli_spec_t             sip_spec;
    ulong                  sip;
    ulong                  sip_mask;
    cli_spec_t             dscp_spec;
    vtss_dscp_t            dscp_min;
    vtss_dscp_t            dscp_max;
    cli_spec_t             ipv4_fragment_spec;
    vtss_qce_bit_t         ipv4_fragment;

    cli_spec_t             sport_spec;
    vtss_udp_tcp_t         sport_min;
    vtss_udp_tcp_t         sport_max;
    cli_spec_t             dport_spec;
    vtss_udp_tcp_t         dport_min;
    vtss_udp_tcp_t         dport_max;

    /* QCE Action fields */
    cli_spec_t             action_class_spec;
    vtss_prio_t            action_class;
    cli_spec_t             action_dp_spec;
    vtss_dp_level_t        action_dp;
    cli_spec_t             action_dscp_spec;
    vtss_dscp_t            action_dscp;

    int                    qcl_user;
    BOOL                   qcl_user_valid;
#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    BOOL                   dscp_list[64];    // valid index: 0..63
    vtss_dscp_t            dscp;
    vtss_dscp_mode_t       dscp_ingr_class_mode;
    vtss_dscp_emode_t      dscp_remark_mode;
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    BOOL                   lport_list[VTSS_LPORTS + 1];
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
    u8                     mode_sel;
    BOOL                   frame_overhead_enable;
    u32                    frame_overhead;
    BOOL                   payload_rate_enable;
    u32                    payload_rate;
    BOOL                   frame_rate_enable;
    u32                    frame_rate;
    BOOL                   preamble_in_payload;
    BOOL                   header_in_payload;
    u8                     header_size;
    BOOL                   accumulate_mode_enable;
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */

} qos_cli_req_t;

#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
static const u8 enable_str[2][10] = {"disabled", "enabled"};
static const u8 include_str[2][10] = {"exclude", "include"};
#endif /* VTSS_FEATUTE_QOS_DOT3AR_RATE_LIMITER */

static char qos_port_policer_fc_cmd_ro[128];
static char qos_port_policer_fc_cmd_rw[128];
static BOOL qos_port_policer_fc_cmd_disabled;

static char QOS_cli_sid_help_str[30];

/****************************************************************************/
/*  Private functions                                                       */
/****************************************************************************/

/* Setup our req_t with default values */
static void QOS_cli_default_set(cli_req_t *req)
{
    qos_cli_req_t *qos_req = req->module_req;
    (void) cli_parm_parse_list(NULL, qos_req->policer_list, 1, QOS_PORT_POLICER_CNT,   1);
    (void) cli_parm_parse_list(NULL, qos_req->queue_list,   0, QOS_PORT_QUEUE_CNT - 1, 1);
    (void) cli_parm_parse_list(NULL, qos_req->class_list,   0, QOS_PORT_PRIO_CNT - 1,  1);
    (void) cli_parm_parse_list(NULL, qos_req->dpl_list,     0, 1,                      1);
    (void) cli_parm_parse_list(NULL, qos_req->pcp_list,     0, VTSS_PCPS - 1,          1);
    (void) cli_parm_parse_list(NULL, qos_req->dei_list,     0, VTSS_DEIS - 1,          1);
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    (void) cli_parm_parse_list(NULL, qos_req->lport_list,   1, VTSS_LPORTS,            1);
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

#ifdef VTSS_FEATURE_QCL_V2
    qos_req->qce_id = QCE_ID_NONE;
    qos_req->qce_id_next = QCE_ID_NONE;
    qos_req->qce_type = VTSS_QCE_TYPE_ANY;
#endif /*  VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    (void) cli_parm_parse_list(NULL, qos_req->dscp_list, 0, 63, 1);
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
}

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
static char *QOS_cli_storm_rate_txt(ulong rate, char *buf)
{
    char *txt = "";

    if (rate >= 1000 && (rate % 1000) == 0) {
        rate /= 1000;
        txt = "k";
    }
    sprintf(buf, "%5d %sfps", rate, txt);
    return buf;
}
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */

static char *QOS_cli_pkt_rate_txt(ulong rate, char *buf)
{
    char *txt = "";

    if (rate < 10000) {
        txt = "f";
    } else if (rate < 10000000) {
        rate = (rate / 1000);
        txt = "kf";
    } else {
        rate = (rate / 1000000);
        txt = "Mf";
    }
    sprintf(buf, "%4d %sps", rate, txt);
    return buf;
}

static char *QOS_cli_bit_rate_txt(ulong rate, char *buf)
{
    char c;

    if (rate < 10000) {
        c = 'k';
    } else if (rate < 10000000) {
        rate = (rate / 1000);
        c = 'M';
    } else {
        rate = (rate / 1000000);
        c = 'G';
    }
    sprintf(buf, "%4d %cbps", rate, c);
    return buf;
}

#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
/*
 * Calculate the actual weight in percent.
 * This calculation includes the round off errors that is caused by the
 * conversion from weight to cost that is done in the API.
 */
#define QOS_COST_MAX 32   // 5 bit on L26 ans Jaguar line ports - 7 bit (128) on Jaguar host port
static void QOS_cli_weight2pct(const vtss_pct_t *weight, vtss_pct_t *pct)
{
    int        queue;
    vtss_pct_t w_min = 100;
    u8         cost[QOS_PORT_WEIGHTED_QUEUE_CNT];
    u8         c_max = 0;
    vtss_pct_t new_weight[QOS_PORT_WEIGHTED_QUEUE_CNT];
    int        w_sum = 0;

    for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
        w_min = MIN(w_min, weight[queue]); // Save the lowest weight for use in next round
    }
    for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        u32 c = ((((QOS_COST_MAX << 4) * w_min) / weight[queue]) + 8) >> 4;
        cost[queue] = MAX(1, c); // Force cost to be in range 1..QOS_COST_MAX
        c_max = MAX(c_max, cost[queue]); // Save the highest cost for use in next round
    }
    for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        new_weight[queue] = (((c_max << 4) / cost[queue]) + 8) >> 4;
        w_sum += new_weight[queue]; // Calculate the sum of weights for use in next round
    }
    for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        vtss_pct_t p =  ((((new_weight[queue] << 4) * 100) / w_sum) + 8) >> 4;
        pct[queue] = MAX(1, p); // Force pct to be in range 1..100
    }
}
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
#define QOS_LPORT_QUEUE_COST_MAX 128   // 5 bit on L26 ans Jaguar line ports - 7 bit (128) on Jaguar host port
#define QOS_LPORT_WEIGHTED_QUEUE_CNT 6 // Max number of Lport queues 
static void QOS_cli_lport_queue_weight2pct(const vtss_pct_t *weight, vtss_pct_t *pct)
{
    int        queue;
    vtss_pct_t w_min = 100;
    u8         cost[QOS_LPORT_WEIGHTED_QUEUE_CNT];
    u8         c_max = 0;
    vtss_pct_t new_weight[QOS_LPORT_WEIGHTED_QUEUE_CNT];
    int        w_sum = 0;
    u8         queue_cnt = 5;

    if (ce_max_mgmt_lport_queue_is_valid(queue_cnt)) {
        queue_cnt = 6;
    } else {
        queue_cnt = 2;
    }

    for (queue = 0; queue < queue_cnt; queue++) {
        w_min = MIN(w_min, weight[queue]); // Save the lowest weight for use in next round
    }
    for (queue = 0; queue < queue_cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        u32 c = ((((QOS_LPORT_QUEUE_COST_MAX << 4) * w_min) / weight[queue]) + 8) >> 4;
        cost[queue] = MAX(1, c); // Force cost to be in range 1..QOS_COST_MAX
        c_max = MAX(c_max, cost[queue]); // Save the highest cost for use in next round
    }
    for (queue = 0; queue < queue_cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        new_weight[queue] = (((c_max << 4) / cost[queue]) + 8) >> 4;
        w_sum += new_weight[queue]; // Calculate the sum of weights for use in next round
    }
    for (queue = 0; queue < queue_cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        vtss_pct_t p =  ((((new_weight[queue] << 4) * 100) / w_sum) + 8) >> 4;
        pct[queue] = MAX(1, p); // Force pct to be in range 1..100
    }
}

static void QOS_cli_lport_weight2pct(const vtss_pct_t *weight, vtss_pct_t *pct, u8 entry_cnt)
{
    int        queue;
    vtss_pct_t w_min = 100;
    u8         cost[VTSS_LPORTS];
    u8         c_max = 0;
    vtss_pct_t new_weight[VTSS_LPORTS];
    int        w_sum = 0;

    for (queue = 0; queue < entry_cnt; queue++) {
        w_min = MIN(w_min, weight[queue]); // Save the lowest weight for use in next round
    }
    for (queue = 0; queue < entry_cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        u32 c = ((((QOS_LPORT_QUEUE_COST_MAX << 4) * w_min) / weight[queue]) + 8) >> 4;
        cost[queue] = MAX(1, c); // Force cost to be in range 1..QOS_COST_MAX
        c_max = MAX(c_max, cost[queue]); // Save the highest cost for use in next round
    }
    for (queue = 0; queue < entry_cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        new_weight[queue] = (((c_max << 4) / cost[queue]) + 8) >> 4;
        w_sum += new_weight[queue]; // Calculate the sum of weights for use in next round
    }
    for (queue = 0; queue < entry_cnt; queue++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        vtss_pct_t p =  ((((new_weight[queue] << 4) * 100) / w_sum) + 8) >> 4;
        pct[queue] = MAX(1, p); // Force pct to be in range 1..100
    }
}

#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */


/****************************************************************************/
/*  Command functions                                                       */
/****************************************************************************/

/*
 * QoS port classification
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_port_classification(cli_req_t *req,
                                            BOOL      pcp,
                                            BOOL      dei,
                                            BOOL      class,
                                            BOOL      dpl,
                                            BOOL      tag_class)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                if (pcp) {
                    conf.usr_prio = qos_req->pcp;
                }
                if (dei) {
                    conf.default_dei = qos_req->dei;
                }
                if (class) {
                    conf.default_prio = req->class_;
                }
                if (dpl) {
                    conf.default_dpl = qos_req->dpl;
                }
                if (tag_class) {
                    if (req->enable) {
                        conf.tag_class_enable = TRUE;
                    }
                    if (req->disable) {
                        conf.tag_class_enable = FALSE;
                    }
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    if (class) {
                        p += sprintf(p, "QoS class  ");
                    }
                    if (dpl) {
                        p += sprintf(p, "DP level  ");
                    }
#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
                    if (pcp) {
                        p += sprintf(p, "PCP  ");
                    }
                    if (dei) {
                        p += sprintf(p, "DEI  ");
                    }
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
                    if (tag_class) {
                        p += sprintf(p, "Tag class.  ");
                    }
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                if (class) {
                    vtss_prio_t vol_prio = QOS_PORT_PRIO_UNDEF;
                    (void) qos_port_volatile_get_default_prio(isid, pit.iport, &vol_prio);
                    if (vol_prio != QOS_PORT_PRIO_UNDEF) {
                        CPRINTF("%-2d(%d)      ", conf.default_prio, vol_prio);
                    } else {
                        CPRINTF("%-11d", conf.default_prio);
                    }
                }
                if (dpl) {
                    CPRINTF("%-10d", conf.default_dpl);
                }
#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
                if (pcp) {
                    CPRINTF("%-5d", conf.usr_prio);
                }
                if (dei) {
                    CPRINTF("%-5d", conf.default_dei);
                }
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
                if (tag_class) {
                    CPRINTF("%-9s", cli_bool_txt(conf.tag_class_enable));
                }
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */
                CPRINTF("\n");
            }
        }
    }
}

static void QOS_cli_cmd_port_classification_class(cli_req_t *req)
{
    QOS_cli_cmd_port_classification(req, 0, 0, 1, 0, 0);
}

static void QOS_cli_cmd_port_classification_dpl(cli_req_t *req)
{
    QOS_cli_cmd_port_classification(req, 0, 0, 0, 1, 0);
}

#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
static void QOS_cli_cmd_port_classification_pcp(cli_req_t *req)
{
    QOS_cli_cmd_port_classification(req, 1, 0, 0, 0, 0);
}

static void QOS_cli_cmd_port_classification_dei(cli_req_t *req)
{
    QOS_cli_cmd_port_classification(req, 0, 1, 0, 0, 0);
}

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static void QOS_cli_cmd_port_classification_tag_class(cli_req_t *req)
{
    QOS_cli_cmd_port_classification(req, 0, 0, 0, 0, 1);
}

static void QOS_cli_cmd_port_classification_map(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    int             pcp;
    int             dei;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                for (pcp = 0; pcp < VTSS_PCPS; pcp++) {
                    if (qos_req->pcp_list[pcp] == 0) {
                        continue;
                    }
                    for (dei = 0; dei < VTSS_DEIS; dei++) {
                        if (qos_req->dei_list[dei] == 0) {
                            continue;
                        }
                        conf.qos_class_map[pcp][dei] = req->class_;
                        if (qos_req->dpl_valid) {
                            conf.dp_level_map[pcp][dei] = qos_req->dpl;
                        }
                    }
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    p += sprintf(p, "PCP  ");
                    p += sprintf(p, "DEI  ");
                    p += sprintf(p, "QoS class  ");
                    p += sprintf(p, "DP level  ");
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                for (pcp = 0; pcp < VTSS_PCPS; pcp++) {
                    if (qos_req->pcp_list[pcp] == 0) {
                        continue;
                    }
                    for (dei = 0; dei < VTSS_DEIS; dei++) {
                        if (qos_req->dei_list[dei] == 0) {
                            continue;
                        }
                        CPRINTF("%d    ",        pcp);
                        CPRINTF("%d    ",        dei);
                        CPRINTF("%d          ", conf.qos_class_map[pcp][dei]);
                        CPRINTF("%d\n      ",    conf.dp_level_map[pcp][dei]);
                    }
                }
                CPRINTF("\r");
            }
        }
    }
}
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */

/*
 * QoS port policer
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_port_policer(cli_req_t *req,
                                     BOOL      mode,
                                     BOOL      rate,
                                     BOOL      unit,
                                     BOOL      dpbl,
                                     BOOL      unicast,
                                     BOOL      multicast,
                                     BOOL      broadcast,
                                     BOOL      flooding,
                                     BOOL      learning,
                                     BOOL      flow_control)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    int             policer;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                    if (qos_req->policer_list[policer] == 0) {
                        continue;
                    }
                    if (mode) {
                        if (req->enable) {
                            conf.port_policer[policer - 1].enabled = TRUE;
                        }
                        if (req->disable) {
                            conf.port_policer[policer - 1].enabled = FALSE;
                        }
                    }
                    if (rate) {
                        conf.port_policer[policer - 1].policer.rate = qos_req->bit_rate;
                    }
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS
                    if (unit) {
                        conf.port_policer_ext[policer - 1].frame_rate = qos_req->fps;
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL
                    if (dpbl) {
                        conf.port_policer_ext[policer - 1].dp_bypass_level = qos_req->dp_bypass_level;
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM
                    if (unicast) {
                        if (req->enable) {
                            conf.port_policer_ext[policer - 1].unicast = TRUE;
                        }
                        if (req->disable) {
                            conf.port_policer_ext[policer - 1].unicast = FALSE;
                        }
                    }
                    if (multicast) {
                        if (req->enable) {
                            conf.port_policer_ext[policer - 1].multicast = TRUE;
                        }
                        if (req->disable) {
                            conf.port_policer_ext[policer - 1].multicast = FALSE;
                        }
                    }
                    if (broadcast) {
                        if (req->enable) {
                            conf.port_policer_ext[policer - 1].broadcast = TRUE;
                        }
                        if (req->disable) {
                            conf.port_policer_ext[policer - 1].broadcast = FALSE;
                        }
                    }
                    if (flooding) {
                        if (req->enable) {
                            conf.port_policer_ext[policer - 1].flooded = TRUE;
                        }
                        if (req->disable) {
                            conf.port_policer_ext[policer - 1].flooded = FALSE;
                        }
                    }
                    if (learning) {
                        if (req->enable) {
                            conf.port_policer_ext[policer - 1].learning = TRUE;
                        }
                        if (req->disable) {
                            conf.port_policer_ext[policer - 1].learning = FALSE;
                        }
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC
                    if (flow_control) {
                        if (req->enable) {
                            conf.port_policer_ext[policer - 1].flow_control = TRUE;
                        }
                        if (req->disable) {
                            conf.port_policer_ext[policer - 1].flow_control = FALSE;
                        }
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    p += sprintf(p, "Parm      ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        /*lint -e{506} */
                        if (QOS_PORT_POLICER_CNT > 1) {
                            p += sprintf(p, "Policer %d  ", policer);
                        } else {
                            p += sprintf(p, "Policer    ");
                        }
                    }
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                if (mode) {
                    CPRINTF("Mode      ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11s", cli_bool_txt(conf.port_policer[policer - 1].enabled));
                    }
                    CPRINTF("\n      ");
                }
                if (rate) {
                    CPRINTF("Rate      ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS
                        if (conf.port_policer_ext[policer - 1].frame_rate) {
                            CPRINTF("%-11s", QOS_cli_pkt_rate_txt(conf.port_policer[policer - 1].policer.rate, buf));
                        } else
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
                            CPRINTF("%-11s", QOS_cli_bit_rate_txt(conf.port_policer[policer - 1].policer.rate, buf));
                    }
                    CPRINTF("\n      ");
                }
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS
                if (unit) {
                    CPRINTF("Unit      ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11s", conf.port_policer_ext[policer - 1].frame_rate ? "fps" : "kbps");
                    }
                    CPRINTF("\n      ");
                }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL
                if (dpbl) {
                    CPRINTF("DPB Level ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11d", conf.port_policer_ext[policer - 1].dp_bypass_level);
                    }
                    CPRINTF("\n      ");
                }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM
                if (unicast) {
                    CPRINTF("Unicast   ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11s", cli_bool_txt(conf.port_policer_ext[policer - 1].unicast));
                    }
                    CPRINTF("\n      ");
                }
                if (multicast) {
                    CPRINTF("Multicast ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11s", cli_bool_txt(conf.port_policer_ext[policer - 1].multicast));
                    }
                    CPRINTF("\n      ");
                }
                if (broadcast) {
                    CPRINTF("Broadcast ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11s", cli_bool_txt(conf.port_policer_ext[policer - 1].broadcast));
                    }
                    CPRINTF("\n      ");
                }
                if (flooding) {
                    CPRINTF("Flooding  ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11s", cli_bool_txt(conf.port_policer_ext[policer - 1].flooded ));
                    }
                    CPRINTF("\n      ");
                }
                if (learning) {
                    CPRINTF("Learning  ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11s", cli_bool_txt(conf.port_policer_ext[policer - 1].learning));
                    }
                    CPRINTF("\n      ");
                }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC
                if (flow_control) {
                    CPRINTF("Flow Ctl  ");
                    for (policer = 1; policer <= QOS_PORT_POLICER_CNT; policer++) {
                        if (qos_req->policer_list[policer] == 0) {
                            continue;
                        }
                        CPRINTF("%-11s", cli_bool_txt(conf.port_policer_ext[policer - 1].flow_control));
                    }
                    CPRINTF("\n      ");
                }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
                CPRINTF("\r");
            }
        }
    }
}

static void QOS_cli_cmd_port_policer_mode(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static void QOS_cli_cmd_port_policer_rate(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS
static void QOS_cli_cmd_port_policer_unit(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
}
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */

#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL
static void QOS_cli_cmd_port_policer_dpbl(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0);
}
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */

#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM
static void QOS_cli_cmd_port_policer_unicast(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0);
}

static void QOS_cli_cmd_port_policer_multicast(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
}

static void QOS_cli_cmd_port_policer_broadcast(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

static void QOS_cli_cmd_port_policer_flooding(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0);
}

static void QOS_cli_cmd_port_policer_learning(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0);
}
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */

#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC
static void QOS_cli_cmd_port_policer_fc(cli_req_t *req)
{
    QOS_cli_cmd_port_policer(req, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
}
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */

#if defined(VTSS_SW_OPTION_BUILD_CE)
#ifdef VTSS_FEATURE_QOS_QUEUE_POLICER
/*
 * QoS port queue policer
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_port_queue_policer(cli_req_t *req,
                                           BOOL      mode,
                                           BOOL      rate)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    int             queue;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                    if (qos_req->queue_list[queue] == 0) {
                        continue;
                    }
                    if (mode) {
                        if (req->enable) {
                            conf.queue_policer[queue].enabled = TRUE;
                        }
                        if (req->disable) {
                            conf.queue_policer[queue].enabled = FALSE;
                        }
                    }
                    if (rate) {
                        conf.queue_policer[queue].policer.rate = qos_req->bit_rate;
                    }
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    p += sprintf(p, "Queue  ");
                    if (mode) {
                        p += sprintf(p, "Mode      ");
                    }
                    if (rate) {
                        p += sprintf(p, "Rate       ");
                    }
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                    if (qos_req->queue_list[queue] == 0) {
                        continue;
                    }
                    CPRINTF("%d      ", queue);
                    if (mode) {
                        CPRINTF("%-10s", cli_bool_txt(conf.queue_policer[queue].enabled));
                    }
                    if (rate) {
                        CPRINTF("%-11s", QOS_cli_bit_rate_txt(conf.queue_policer[queue].policer.rate, buf));
                    }
                    CPRINTF("\n      ");
                }
                CPRINTF("\r");
            }
        }
    }
}

static void QOS_cli_cmd_port_queue_policer_mode(cli_req_t *req)
{
    QOS_cli_cmd_port_queue_policer(req, 1, 0);
}

static void QOS_cli_cmd_port_queue_policer_rate(cli_req_t *req)
{
    QOS_cli_cmd_port_queue_policer(req, 0, 1);
}
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */

#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
static void QOS_cli_cmd_port_scheduler_mode(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                conf.dwrr_enable = qos_req->weighted;
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    p += sprintf(p, "Mode      ");
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                CPRINTF("%-10s\n", conf.dwrr_enable ? "Weighted" : "Strict");
            }
        }
    }
}

static void QOS_cli_cmd_port_scheduler_weight(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    int             queue;
    vtss_pct_t      pct[QOS_PORT_WEIGHTED_QUEUE_CNT] = {0}; // Zero based! Initialized to zero (keep lint happy)
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                    if (qos_req->queue_list[queue] == 0) {
                        continue;
                    }
                    conf.queue_pct[queue] = qos_req->weight;
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                QOS_cli_weight2pct(conf.queue_pct, pct);
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    p += sprintf(p, "Queue  ");
                    p += sprintf(p, "Weight     ");
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                    if (qos_req->queue_list[queue] == 0) {
                        continue;
                    }
                    CPRINTF("%d      ", queue);
                    CPRINTF("%-3d (%d%%)", conf.queue_pct[queue], pct[queue]);
                    CPRINTF("\n      ");
                }
                CPRINTF("\r");
            }
        }
    }
}

/*
 * QoS port shaper
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_port_shaper(cli_req_t *req,
                                    BOOL      mode,
                                    BOOL      rate)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                if (mode) {
                    if (req->enable) {
                        conf.shaper_status = TRUE;
                    }
                    if (req->disable) {
                        conf.shaper_status = FALSE;
                    }
                }
                if (rate) {
                    conf.shaper_rate = qos_req->bit_rate;
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    if (mode) {
                        p += sprintf(p, "Mode      ");
                    }
                    if (rate) {
                        p += sprintf(p, "Rate       ");
                    }
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                if (mode) {
                    CPRINTF("%-10s", cli_bool_txt(conf.shaper_status));
                }
                if (rate) {
                    CPRINTF("%-11s", QOS_cli_bit_rate_txt(conf.shaper_rate, buf));
                }
                CPRINTF("\n");
            }
        }
    }
}

static void QOS_cli_cmd_port_shaper_mode(cli_req_t *req)
{
    QOS_cli_cmd_port_shaper(req, 1, 0);
}

static void QOS_cli_cmd_port_shaper_rate(cli_req_t *req)
{
    QOS_cli_cmd_port_shaper(req, 0, 1);
}

/*
 * QoS port queue shaper
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_port_queue_shaper(cli_req_t *req,
                                          BOOL      mode,
                                          BOOL      rate,
                                          BOOL      excess)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    int             queue;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                    if (qos_req->queue_list[queue] == 0) {
                        continue;
                    }
                    if (mode) {
                        if (req->enable) {
                            conf.queue_shaper[queue].enable = TRUE;
                        }
                        if (req->disable) {
                            conf.queue_shaper[queue].enable = FALSE;
                        }
                    }
                    if (rate) {
                        conf.queue_shaper[queue].rate = qos_req->bit_rate;
                    }
                    if (excess) {
                        if (req->enable) {
                            conf.excess_enable[queue] = TRUE;
                        }
                        if (req->disable) {
                            conf.excess_enable[queue] = FALSE;
                        }
                    }
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    p += sprintf(p, "Queue  ");
                    if (mode) {
                        p += sprintf(p, "Mode      ");
                    }
                    if (rate) {
                        p += sprintf(p, "Rate       ");
                    }
                    if (excess) {
                        p += sprintf(p, "Excess    ");
                    }
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                    if (qos_req->queue_list[queue] == 0) {
                        continue;
                    }
                    CPRINTF("%d      ", queue);
                    if (mode) {
                        CPRINTF("%-10s", cli_bool_txt(conf.queue_shaper[queue].enable));
                    }
                    if (rate) {
                        CPRINTF("%-11s", QOS_cli_bit_rate_txt(conf.queue_shaper[queue].rate, buf));
                    }
                    if (excess) {
                        CPRINTF("%-10s", cli_bool_txt(conf.excess_enable[queue]));
                    }
                    CPRINTF("\n      ");
                }
                CPRINTF("\r");
            }
        }
    }
}

static void QOS_cli_cmd_port_queue_shaper_mode(cli_req_t *req)
{
    QOS_cli_cmd_port_queue_shaper(req, 1, 0, 0);
}

static void QOS_cli_cmd_port_queue_shaper_rate(cli_req_t *req)
{
    QOS_cli_cmd_port_queue_shaper(req, 0, 1, 0);
}

static void QOS_cli_cmd_port_queue_shaper_excess(cli_req_t *req)
{
    QOS_cli_cmd_port_queue_shaper(req, 0, 0, 1);
}
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

/*
 * QoS port tag remark
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static void QOS_cli_cmd_port_tag_remark(cli_req_t *req,
                                        BOOL      mode,
                                        BOOL      pcp,
                                        BOOL      dei,
                                        BOOL      dpl)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                if (mode) {
                    conf.tag_remark_mode = qos_req->tag_remark_mode;
                }
                if (pcp) {
                    conf.tag_default_pcp = qos_req->pcp;
                }
                if (dei) {
                    conf.tag_default_dei = qos_req->dei;
                }
                if (dpl) {
                    int i;
                    for (i = 0; i < QOS_PORT_TR_DPL_CNT; i++) {
                        conf.tag_dp_map[i] = qos_req->tag_dp_map[i];
                    }
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    if (mode) {
                        p += sprintf(p, "Mode        ");
                    }
                    if (pcp) {
                        p += sprintf(p, "PCP  ");
                    }
                    if (dei) {
                        p += sprintf(p, "DEI  ");
                    }
#ifdef VTSS_ARCH_JAGUAR_1
                    if (dpl) {
                        p += sprintf(p, "DPL mapping  ");
                    }
#endif /* VTSS_ARCH_JAGUAR_1 */
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                if (mode) {
                    CPRINTF("%-12s", qos_port_tag_remarking_mode_txt(conf.tag_remark_mode));
                }
                if (pcp) {
                    CPRINTF("%-5d", conf.tag_default_pcp);
                }
                if (dei) {
                    CPRINTF("%-5d", conf.tag_default_dei);
                }
#ifdef VTSS_ARCH_JAGUAR_1
                if (dpl) {
                    int i;
                    for (i = 0; i < QOS_PORT_TR_DPL_CNT; i++) {
                        CPRINTF("%d  ", conf.tag_dp_map[i]);
                    }
                }
#endif /* VTSS_ARCH_JAGUAR_1 */
                CPRINTF("\n");
            }
        }
    }
}

static void QOS_cli_cmd_port_tag_remarking_mode(cli_req_t *req)
{
    QOS_cli_cmd_port_tag_remark(req, 1, 0, 0, 0);
}

static void QOS_cli_cmd_port_tag_remarking_pcp(cli_req_t *req)
{
    QOS_cli_cmd_port_tag_remark(req, 0, 1, 0, 0);
}

static void QOS_cli_cmd_port_tag_remarking_dei(cli_req_t *req)
{
    QOS_cli_cmd_port_tag_remark(req, 0, 0, 1, 0);
}

static void QOS_cli_cmd_port_tag_remarking_dpl(cli_req_t *req)
{
    QOS_cli_cmd_port_tag_remark(req, 0, 0, 0, 1);
}

static void QOS_cli_cmd_port_tag_remarking_map(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    int             class;
    int             dpl;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                for (class = 0; class < QOS_PORT_PRIO_CNT; class++) {
                    if (qos_req->class_list[class] == 0) {
                        continue;
                    }
                    for (dpl = 0; dpl < 2; dpl++) {
                        if (qos_req->dpl_list[dpl] == 0) {
                            continue;
                        }
                        conf.tag_pcp_map[class][dpl] = qos_req->pcp;
                        if (qos_req->dei_valid) {
                            conf.tag_dei_map[class][dpl] = qos_req->dei;
                        }
                    }
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    p += sprintf(p, "QoS class  ");
                    p += sprintf(p, "DP level  ");
                    p += sprintf(p, "PCP  ");
                    p += sprintf(p, "DEI  ");
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                for (class = 0; class < QOS_PORT_PRIO_CNT; class++) {
                    if (qos_req->class_list[class] == 0) {
                        continue;
                    }
                    for (dpl = 0; dpl < 2; dpl++) {
                        if (qos_req->dpl_list[dpl] == 0) {
                            continue;
                        }
                        CPRINTF("%d          ", class);
                        CPRINTF("%d         ",  dpl);
                        CPRINTF("%d    ",      conf.tag_pcp_map[class][dpl]);
                        CPRINTF("%d\n      ",   conf.tag_dei_map[class][dpl]);
                    }
                }
                CPRINTF("\r");
            }
        }
    }
}
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
/*
 * QoS port storm control
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_port_storm(cli_req_t *req, BOOL uc, BOOL bc, BOOL un)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                int pol_ix = uc ? QOS_STORM_POLICER_UNICAST : bc ? QOS_STORM_POLICER_BROADCAST : QOS_STORM_POLICER_UNKNOWN;
                if (req->enable) {
                    conf.port_policer[pol_ix].enabled = TRUE;
                }
                if (req->disable) {
                    conf.port_policer[pol_ix].enabled = FALSE;
                }
                if (qos_req->bit_rate_spec) {
                    conf.port_policer[pol_ix].policer.rate = qos_req->bit_rate;
                }
                if (qos_req->kbps) {
                    conf.port_policer_ext[pol_ix].frame_rate = FALSE;
                }
                if (qos_req->fps) {
                    conf.port_policer_ext[pol_ix].frame_rate = TRUE;
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    if (uc) {
                        p += sprintf(p, "Uc. Mode  Rate       ");
                    }
                    if (bc) {
                        p += sprintf(p, "Bc. Mode  Rate       ");
                    }
                    if (un) {
                        p += sprintf(p, "Un. Mode  Rate       ");
                    }
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                if (uc) {
                    CPRINTF("%-10s%-11s",
                            cli_bool_txt(conf.port_policer[QOS_STORM_POLICER_UNICAST].enabled),
                            conf.port_policer_ext[QOS_STORM_POLICER_UNICAST].frame_rate ?
                            QOS_cli_pkt_rate_txt(conf.port_policer[QOS_STORM_POLICER_UNICAST].policer.rate, buf) :
                            QOS_cli_bit_rate_txt(conf.port_policer[QOS_STORM_POLICER_UNICAST].policer.rate, buf));
                }
                if (bc) {
                    CPRINTF("%-10s%-11s",
                            cli_bool_txt(conf.port_policer[QOS_STORM_POLICER_BROADCAST].enabled),
                            conf.port_policer_ext[QOS_STORM_POLICER_BROADCAST].frame_rate ?
                            QOS_cli_pkt_rate_txt(conf.port_policer[QOS_STORM_POLICER_BROADCAST].policer.rate, buf) :
                            QOS_cli_bit_rate_txt(conf.port_policer[QOS_STORM_POLICER_BROADCAST].policer.rate, buf));
                }
                if (un) {
                    CPRINTF("%-10s%-11s",
                            cli_bool_txt(conf.port_policer[QOS_STORM_POLICER_UNKNOWN].enabled),
                            conf.port_policer_ext[QOS_STORM_POLICER_UNKNOWN].frame_rate ?
                            QOS_cli_pkt_rate_txt(conf.port_policer[QOS_STORM_POLICER_UNKNOWN].policer.rate, buf) :
                            QOS_cli_bit_rate_txt(conf.port_policer[QOS_STORM_POLICER_UNKNOWN].policer.rate, buf));
                }
                CPRINTF("\n");
            }
        }
    }
}

static void QOS_cli_cmd_port_storm_unicast(cli_req_t *req)
{
    QOS_cli_cmd_port_storm(req, 1, 0, 0);
}
static void QOS_cli_cmd_port_storm_broadcast(cli_req_t *req)
{
    QOS_cli_cmd_port_storm(req, 0, 1, 0);
}
static void QOS_cli_cmd_port_storm_unknown(cli_req_t *req)
{
    QOS_cli_cmd_port_storm(req, 0, 0, 1);
}
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
static void QOS_cli_cmd_storm(cli_req_t *req, BOOL uc, BOOL mc, BOOL bc)
{
    qos_conf_t         conf;
    vtss_packet_rate_t *rate;
    BOOL               *enabled;
    char               buf[32];
    qos_cli_req_t      *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_conf_get(&conf) != VTSS_OK) {
        return;
    }

    if (req->set) {
        enabled = (uc ? &conf.policer_uc_status : mc ? &conf.policer_mc_status : &conf.policer_bc_status);
        rate = (uc ? &conf.policer_uc : mc ? &conf.policer_mc : &conf.policer_bc);
        if (req->enable) {
            *enabled = 1;
        }
        if (req->disable) {
            *enabled = 0;
        }
        if (qos_req->packet_rate_spec == CLI_SPEC_VAL) {
            *rate = qos_req->packet_rate;
        }
        (void) qos_conf_set(&conf);
    } else {
        if (uc) {
            CPRINTF("Storm Unicast  : %s %s\n",
                    cli_bool_txt(conf.policer_uc_status),
                    QOS_cli_storm_rate_txt(conf.policer_uc, buf));
        }
        if (mc) {
            CPRINTF("Storm Multicast: %s %s\n",
                    cli_bool_txt(conf.policer_mc_status),
                    QOS_cli_storm_rate_txt(conf.policer_mc, buf));
        }
        if (bc) {
            CPRINTF("Storm Broadcast: %s %s\n",
                    cli_bool_txt(conf.policer_bc_status),
                    QOS_cli_storm_rate_txt(conf.policer_bc, buf));
        }
    }
}

static void QOS_cli_cmd_storm_uc(cli_req_t *req)
{
    QOS_cli_cmd_storm(req, 1, 0, 0);
}
static void QOS_cli_cmd_storm_mc(cli_req_t *req)
{
    QOS_cli_cmd_storm(req, 0, 1, 0);
}
static void QOS_cli_cmd_storm_bc(cli_req_t *req)
{
    QOS_cli_cmd_storm(req, 0, 0, 1);
}
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */

#if defined(VTSS_FEATURE_QOS_WRED)
static void QOS_cli_cmd_wred(cli_req_t *req)
{
    qos_conf_t         conf;
    int                queue;
    qos_cli_req_t      *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_conf_get(&conf) != VTSS_OK) {
        return;
    }

    if (req->set) {
        for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
            if (qos_req->queue_list[queue] == 0) {
                continue;
            }
            if (req->enable) {
                conf.wred[queue].enable = 1;
            }
            if (req->disable) {
                conf.wred[queue].enable = 0;
            }
            if (qos_req->min_th_valid) {
                conf.wred[queue].min_th = qos_req->min_th;
            }
            if (qos_req->mdp_1_valid) {
                conf.wred[queue].max_prob_1 = qos_req->mdp_1;
            }
            if (qos_req->mdp_2_valid) {
                conf.wred[queue].max_prob_2 = qos_req->mdp_2;
            }
            if (qos_req->mdp_3_valid) {
                conf.wred[queue].max_prob_3 = qos_req->mdp_3;
            }
        }
        (void) qos_conf_set(&conf);
    } else {
        cli_table_header("Queue  Mode      Min Th  Mdp 1  Mdp 2  Mdp 3");
        for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
            if (qos_req->queue_list[queue] == 0) {
                continue;
            }
            CPRINTF("%d      %s    %3u    %3u    %3u    %3u\n",
                    queue,
                    cli_bool_txt(conf.wred[queue].enable),
                    conf.wred[queue].min_th,
                    conf.wred[queue].max_prob_1,
                    conf.wred[queue].max_prob_2,
                    conf.wred[queue].max_prob_3);
        }
    }
}
#endif /* VTSS_FEATURE_QOS_WRED */

#if defined(VTSS_FEATURE_VSTAX_V2)
static void QOS_cli_cmd_cmef(cli_req_t *req)
{
    qos_conf_t         conf;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_conf_get(&conf) != VTSS_OK) {
        return;
    }

    if (req->set) {
        conf.cmef_disable = !req->enable;
        (void) qos_conf_set(&conf);
    } else {
        CPRINTF("Congestion Management: %s\n", cli_bool_txt(!conf.cmef_disable));
    }
}
#endif /* defined(VTSS_FEATURE_VSTAX_V2) */

static void QOS_cli_cmd_debug_regs(cli_req_t *req)
{
    qos_port_conf_change_reg_t reg;
    BOOL                       first = TRUE;

    reg.module_id = VTSS_MODULE_ID_NONE;
    while (qos_port_conf_change_reg_get(&reg, req->clear) == VTSS_OK) {
        if (first) {
            first = FALSE;
            cli_header("QoS Port Conficuration Change Registrations", 1);
            cli_table_header("Type    Module           Max Callback [msec]");
        }
        CPRINTF("%-6s  %-15.15s  %llu\n", reg.global ? "Global" : "Local", vtss_module_names[reg.module_id], VTSS_OS_TICK2MSEC(reg.max_ticks));
    }
    if (!req->clear && first) {
        CPRINTF("No registrations found!\n");
    }
}

#ifdef VTSS_FEATURE_QCL_V2
static void cli_qce_range_set_u16(qos_qce_vr_u16_t *dest, u16 min, u16 max,
                                  u16 mask, cli_spec_t spec, BOOL add)
{
    if (add || spec != CLI_SPEC_NONE) {
        if (spec == CLI_SPEC_VAL && min < max) {
            dest->in_range = TRUE;
        } else {
            dest->in_range = FALSE;
        }

        if (dest->in_range) {
            dest->vr.r.low = min;
            dest->vr.r.high = max;
        } else {
            dest->vr.v.value = min;
            dest->vr.v.mask = (spec == CLI_SPEC_VAL ? mask : 0);
        }
    }
}

static void cli_qce_range_set_u8(qos_qce_vr_u8_t *dest, u8 min, u8 max,
                                 u8 mask, cli_spec_t spec, BOOL add)
{
    if (add || spec != CLI_SPEC_NONE) {
        if (spec == CLI_SPEC_VAL && min < max) {
            dest->in_range = TRUE;
        } else {
            dest->in_range = FALSE;
        }

        if (dest->in_range) {
            dest->vr.r.low = min;
            dest->vr.r.high = max;
        } else {
            dest->vr.v.value = min;
            dest->vr.v.mask = (spec == CLI_SPEC_VAL ? mask : 0);
        }
    }
}

static void cli_qce_action_set(cli_req_t *req, qos_qce_action_t *action)
{
    qos_cli_req_t  *qos_req = req->module_req;

    if (qos_req->action_class_spec != CLI_SPEC_NONE) {
        action->prio = qos_req->action_class;
        QCE_ENTRY_CONF_ACTION_SET(action->action_bits, QOS_QCE_ACTION_PRIO, 1);
    }
    if (qos_req->action_dp_spec != CLI_SPEC_NONE) {
        action->dp = qos_req->action_dp;
        QCE_ENTRY_CONF_ACTION_SET(action->action_bits, QOS_QCE_ACTION_DP, 1);
    }
    if (qos_req->action_dscp_spec != CLI_SPEC_NONE) {
        action->dscp = qos_req->action_dscp;
        QCE_ENTRY_CONF_ACTION_SET(action->action_bits, QOS_QCE_ACTION_DSCP, 1);
    }
}

static void cli_qce_mac_set(vtss_qce_u24_t *qce_mac, uchar *mac,
                            cli_spec_t spec, BOOL add)
{
    int i;

    if (add || spec != CLI_SPEC_NONE) {
        for (i = 0; i < 3; i++) {
            qce_mac->value[i] = mac[i];
            qce_mac->mask[i] = (spec == CLI_SPEC_VAL ? 0xFF : 0);
        }
    }
}

static void cli_qce_ip_set(vtss_qce_ip_t *qce_ip, ulong ip, ulong mask,
                           cli_spec_t spec, BOOL add)
{
    if (add || spec != CLI_SPEC_NONE) {
        qce_ip->value = ip;
        qce_ip->mask = (spec == CLI_SPEC_VAL ? mask : 0);
    }
}

static void QOS_cli_cmd_qcl_add(cli_req_t *req)
{
    qos_qce_entry_conf_t qce, qce_next;
    vtss_qce_id_t        qce_id, next_id;
    BOOL                 add;
    qos_cli_req_t        *qos_req = req->module_req;
    vtss_port_no_t        iport;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    /* If tag type is untagged and vid or PCP or DEI is valid, return error */
    if ((qos_req->tag == VTSS_VCAP_BIT_0) && (qos_req->tag_spec == CLI_SPEC_VAL)) {
        if ((qos_req->vid_spec == CLI_SPEC_VAL) || (qos_req->qce_pcp_spec == CLI_SPEC_VAL) ||
            (qos_req->qce_dei_spec == CLI_SPEC_VAL)) {
            CPRINTF("vid, pcp or dei cannot be specified with untagged option\n");
            return;
        }
    }

    memset(&qce, 0, sizeof(qce));
    qce_id = qos_req->qce_id;
    add = (qce_id == QCE_ID_NONE ||
           qos_mgmt_qce_entry_get(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, qce_id, &qce, 0) != VTSS_OK);

    /* adjust qce_id_next for keyword 'last', don't change the location for
       modification if next_id is QCE_ID_NONE */
    next_id = qos_req->qce_id_next;
    if (next_id > QCE_ID_END) {
        qos_req->qce_id_next = QCE_ID_NONE;
    } else if (add == FALSE && next_id == QCE_ID_NONE) {
        if (qos_mgmt_qce_entry_get(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, qce_id, &qce_next, 1) == VTSS_OK) {
            qos_req->qce_id_next = qce_next.id;
        }
    }

    qce.id = qce_id;
    /* Clear the old frame fields if changing from one frame type to
       another frame type, as same space is shared by all frame types */
    if (add == FALSE && qos_req->qce_type_spec != CLI_SPEC_NONE &&
        qos_req->qce_type != qce.type) {
        memset(&qce.key.frame, 0, sizeof(qce.key.frame));
    }

    /* Frame Type */
    if (add || qos_req->qce_type_spec != CLI_SPEC_NONE) {
        qce.type = (qos_req->qce_type_spec == CLI_SPEC_ANY ?
                    VTSS_QCE_TYPE_ANY : qos_req->qce_type);
    }

    /* Switch ID */
    qce.isid = VTSS_ISID_GLOBAL;
#if VTSS_SWITCH_STACKABLE
    if (add || qos_req->usid_qce_spec != CLI_SPEC_NONE) {
        qce.isid = (qos_req->usid_qce_spec == CLI_SPEC_VAL ?
                    topo_usid2isid(qos_req->usid_qce) :
                    VTSS_ISID_GLOBAL);
    }
#endif /* VTSS_SWITCH_STACKABLE */

    /* Port list */
    if (add || qos_req->port_spec != CLI_SPEC_NONE) {
        VTSS_PORT_BF_CLR(qce.port_list);
        for (iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
            VTSS_PORT_BF_SET(qce.port_list, iport, req->uport_list[iport2uport(iport)]);
        }
    }

    /* Action */
    cli_qce_action_set(req, &qce.action);
    if (add && qos_req->action_class_spec == CLI_SPEC_NONE &&
        qos_req->action_dp_spec == CLI_SPEC_NONE &&
        qos_req->action_dscp_spec == CLI_SPEC_NONE) {
        CPRINTF("QCL Add failed: classified parameter missing\n");
        return;
    }

    /* VLAN info */
    cli_qce_range_set_u16(&qce.key.vid, qos_req->vid_min, qos_req->vid_max,
                          0xFFF, qos_req->vid_spec, add);

    if (add || qos_req->tag_spec != CLI_SPEC_NONE) {
        QCE_ENTRY_CONF_KEY_SET(qce.key.key_bits, QOS_QCE_VLAN_TAG, qos_req->tag);
    }

    if (add || qos_req->qce_pcp_spec != CLI_SPEC_NONE) {
        qce.key.pcp.value = qos_req->qce_pcp_low;
        if (qos_req->qce_pcp_high) {
            qce.key.pcp.mask = 0xFF & (~((uchar)(qos_req->qce_pcp_high - qos_req->qce_pcp_low)));
        } else {
            qce.key.pcp.mask = (qos_req->qce_pcp_spec == CLI_SPEC_VAL ? 0xFF : 0);
        }
    }

    if (add || qos_req->qce_dei_spec != CLI_SPEC_NONE) {
        QCE_ENTRY_CONF_KEY_SET(qce.key.key_bits, QOS_QCE_VLAN_DEI, qos_req->qce_dei);
    }

    /* MAC data */
    cli_qce_mac_set(&qce.key.smac, qos_req->smac, qos_req->smac_spec, add);

    if (add || qos_req->dmac_type_spec != CLI_SPEC_NONE) {
        QCE_ENTRY_CONF_KEY_SET(qce.key.key_bits, QOS_QCE_DMAC_TYPE, qos_req->dmac_type);
    }

    switch (qos_req->qce_type) {
    case VTSS_QCE_TYPE_ETYPE: {
        if (add || qos_req->etype_spec != CLI_SPEC_NONE) {
            qce.key.frame.etype.etype.value[0] = (qos_req->etype >> 8) & 0xFF;
            qce.key.frame.etype.etype.mask[0] =
                (qos_req->etype_spec == CLI_SPEC_VAL ? 0xFF : 0);
            qce.key.frame.etype.etype.value[1] = qos_req->etype & 0xFF;
            qce.key.frame.etype.etype.mask[1] = qce.key.frame.etype.etype.mask[0];
        }
        break;
    }
    case VTSS_QCE_TYPE_LLC:
        if (add || qos_req->dsap_spec != CLI_SPEC_NONE) {
            qce.key.frame.llc.dsap.value = qos_req->dsap;
            qce.key.frame.llc.dsap.mask =
                (qos_req->dsap_spec == CLI_SPEC_VAL ? 0xFF : 0);
        }
        if (add || qos_req->ssap_spec != CLI_SPEC_NONE) {
            qce.key.frame.llc.ssap.value = qos_req->ssap;
            qce.key.frame.llc.ssap.mask =
                (qos_req->ssap_spec == CLI_SPEC_VAL ? 0xFF : 0);
        }
        if (add || qos_req->control_spec != CLI_SPEC_NONE) {
            qce.key.frame.llc.control.value = qos_req->control;
            qce.key.frame.llc.control.mask =
                (qos_req->control_spec == CLI_SPEC_VAL ? 0xFF : 0);
        }
        break;
    case VTSS_QCE_TYPE_SNAP:
        if (add || qos_req->snap_pid_spec != CLI_SPEC_NONE) {
            qce.key.frame.snap.pid.value[0] = (qos_req->snap_pid >> 8) & 0xFF;
            qce.key.frame.snap.pid.mask[0] =
                (qos_req->snap_pid_spec == CLI_SPEC_VAL ? 0xFF : 0);

            qce.key.frame.snap.pid.value[1] = qos_req->snap_pid & 0xFF;
            qce.key.frame.snap.pid.mask[1] = qce.key.frame.snap.pid.mask[0];
        }
        break;
    case VTSS_QCE_TYPE_IPV4:
        if (add || qos_req->ipv4_fragment_spec != CLI_SPEC_NONE) {
            QCE_ENTRY_CONF_KEY_SET(qce.key.key_bits, QOS_QCE_IPV4_FRAGMENT,
                                   qos_req->ipv4_fragment);
        }

        cli_qce_range_set_u8(&qce.key.frame.ipv4.dscp, qos_req->dscp_min,
                             qos_req->dscp_max, 0x3F, qos_req->dscp_spec, add);

        if (add || qos_req->protocol_spec != CLI_SPEC_NONE) {
            qce.key.frame.ipv4.proto.value = qos_req->protocol;
            qce.key.frame.ipv4.proto.mask =
                (qos_req->protocol_spec == CLI_SPEC_VAL ? 0xFF : 0);
        }

        cli_qce_ip_set(&qce.key.frame.ipv4.sip, qos_req->sip,
                       qos_req->sip_mask, qos_req->sip_spec, add);

        cli_qce_range_set_u16(&qce.key.frame.ipv4.sport, qos_req->sport_min,
                              qos_req->sport_max, 0xFFFF, qos_req->sport_spec, add);

        cli_qce_range_set_u16(&qce.key.frame.ipv4.dport, qos_req->dport_min,
                              qos_req->dport_max, 0xFFFF, qos_req->dport_spec, add);

        break;
    case VTSS_QCE_TYPE_IPV6: {
        int  i;
        cli_qce_range_set_u8(&qce.key.frame.ipv6.dscp, qos_req->dscp_min,
                             qos_req->dscp_max, 0x3F, qos_req->dscp_spec, add);

        if (add || qos_req->protocol_spec != CLI_SPEC_NONE) {
            qce.key.frame.ipv6.proto.value = qos_req->protocol;
            qce.key.frame.ipv6.proto.mask =
                (qos_req->protocol_spec == CLI_SPEC_VAL ? 0xFF : 0);
        }

        if (add || qos_req->sip_spec != CLI_SPEC_NONE) {
            for (i = 0; i < 4; i++) {
                qce.key.frame.ipv6.sip.value[i] = (qos_req->sip >> ((3 - i) * 8)) & 0xFF;
                qce.key.frame.ipv6.sip.mask[i] =
                    (qos_req->sip_spec == CLI_SPEC_VAL ?
                     ((qos_req->sip_mask >> ((3 - i) * 8)) & 0xFF) : 0);
            }
        }

        cli_qce_range_set_u16(&qce.key.frame.ipv6.sport, qos_req->sport_min,
                              qos_req->sport_max, 0xFFFF, qos_req->sport_spec, add);

        cli_qce_range_set_u16(&qce.key.frame.ipv6.dport, qos_req->dport_min,
                              qos_req->dport_max, 0xFFFF, qos_req->dport_spec, add);

        break;
    }
    default:
        break;
    }

    if (qos_mgmt_qce_entry_add(QCL_USER_STATIC, QCL_ID_END, qos_req->qce_id_next, &qce) == VTSS_OK) {
        CPRINTF("QCE ID %d %s ", qce.id, add ? "added" : "modified");
        if (add) {
            if (next_id && next_id < QCE_ID_END) {
                CPRINTF("before QCE ID %d\n", qos_req->qce_id_next);
            } else {
                CPRINTF("last\n");
            }
        } else {
            if (next_id > QCE_ID_END) {
                CPRINTF("and placed at last\n");
            } else if (next_id) {
                CPRINTF("and placed before QCE ID %d\n", qos_req->qce_id_next);
            } else {
                CPRINTF("\n");
            }
        }
    } else {
        CPRINTF("QCL Add failed\n");
    }
}

static void QOS_cli_cmd_qcl_del(cli_req_t *req)
{
    qos_cli_req_t  *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_mgmt_qce_entry_del(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, qos_req->qce_id) != VTSS_OK) {
        CPRINTF("QCL Delete failed\n");
    }
}

static char *cli_qcl_user_txt(BOOL acronym, qcl_user_t user_id, char *buf)
{
    if (acronym) {
        memcpy(buf, qcl_user_names[user_id], 1);
        buf[1] = '\0';
    } else {
        sprintf(buf, "%s", qcl_user_names[user_id]);
    }

    return buf;
}

#if VTSS_SWITCH_STACKABLE
static char *cli_qcl_usid_txt(vtss_isid_t isid, char *buf)
{
    if (isid == VTSS_ISID_GLOBAL) {
        strcpy(buf, "Any");
    } else {
        sprintf(buf, "%-2d", topo_isid2usid(isid));
    }
    return buf;
}
#endif

/* QCL frame type text */
static char *qcl_qce_type_txt(vtss_qce_type_t type, char *buf, BOOL full)
{
    switch (type) {
    case VTSS_QCE_TYPE_ETYPE:
        if (full) {
            strcpy(buf, "Ethernet");
        } else {
            strcpy(buf, "EType");
        }
        break;
    case VTSS_QCE_TYPE_ANY:
        strcpy(buf, "Any");
        break;
    case VTSS_QCE_TYPE_LLC:
        strcpy(buf, "LLC");
        break;
    case VTSS_QCE_TYPE_SNAP:
        strcpy(buf, "SNAP");
        break;
    case VTSS_QCE_TYPE_IPV4:
        strcpy(buf, "IPv4");
        break;
    case VTSS_QCE_TYPE_IPV6:
        strcpy(buf, "IPv6");
        break;
    default:
        strcpy(buf, "?");
        break;
    }
    return (buf);
}

static char *qcl_qce_range_txt_u16(qos_qce_vr_u16_t *range, char *buf)
{
    if (range->in_range) {
        sprintf(buf, "%u-%u", range->vr.r.low, range->vr.r.high);
    } else if (range->vr.v.mask) {
        sprintf(buf, "%u", range->vr.v.value);
    } else {
        sprintf(buf, "Any");
    }
    return (buf);
}

static char *qcl_qce_range_txt_u8(qos_qce_vr_u8_t *range, char *buf)
{
    if (range->in_range) {
        sprintf(buf, "%u-%u", range->vr.r.low, range->vr.r.high);
    } else if (range->vr.v.mask) {
        sprintf(buf, "%u", range->vr.v.value);
    } else {
        sprintf(buf, "Any");
    }
    return (buf);
}

static char *qcl_qce_proto_txt(vtss_qce_u8_t *proto, char *buf)
{
    if (proto->mask) {
        if (proto->value == 6) {
            sprintf(buf, "TCP");
        } else if (proto->value == 17) {
            sprintf(buf, "UDP");
        } else {
            sprintf(buf, "%u", proto->value);
        }
    } else {
        sprintf(buf, "Any");
    }
    return (buf);
}

static char *qcl_qce_port_list_txt(qos_qce_entry_conf_t *conf, char *buf)
{
    BOOL          port_list[VTSS_PORTS];
    int           i;

    for (i = VTSS_PORT_NO_START; i < VTSS_PORT_NO_END; i++) {
        if (VTSS_PORT_BF_GET(conf->port_list, i)) {
            port_list[i] = 1;
        } else {
            port_list[i] = 0;
        }
    }
    return cli_iport_list_txt(port_list, buf);
}

static void cli_cmd_qcl_overview(qcl_user_t user_id, vtss_qce_id_t id,
                                 qos_qce_entry_conf_t *conf,
                                 BOOL *first, BOOL local_status)
{
    char          buf[MGMT_PORT_BUF_SIZE];
    u8            bit_val;

    if (*first) {
        *first = 0;
        if (local_status) {
            CPRINTF("\nUser          ID   Frame  Class DP DSCP       Conflict Port\n");
            CPRINTF("----------    --   -----  ----- -- ---------  -------- -------\n");
        } else {
#if VTSS_SWITCH_STACKABLE
            CPRINTF("\nSID   ID   Frame  SMAC     DMAC VID        PCP  DEI  Class DP DSCP  Port\n");
            CPRINTF("---   --   -----  -------- ---- ---------  ---  ---  ----- -- ----  -------\n");
#else
            CPRINTF("\nID   Frame  SMAC     DMAC VID        PCP  DEI  Class DP DSCP  Port\n");
            CPRINTF("--   -----  -------- ---- ---------  ---  ---  ----- -- ----  -------\n");
#endif
        }
    }

    if (local_status) {
        CPRINTF("%-14s", cli_qcl_user_txt(0, user_id, buf));
#if VTSS_SWITCH_STACKABLE
    } else {
        CPRINTF("%-6s", cli_qcl_usid_txt(conf->isid, buf));
#endif
    }

    CPRINTF("%-5d", id);
    CPRINTF("%-7s",  qcl_qce_type_txt(conf->type, buf, 0));

    if (!local_status) {
        /* SMAC */
        if (!conf->key.smac.mask[0] && !conf->key.smac.mask[1] &&
            !conf->key.smac.mask[2]) {
            sprintf(buf, "%s", "Any");
        } else {
            sprintf(buf, "%02x-%02x-%02x", conf->key.smac.value[0], conf->key.smac.value[1], conf->key.smac.value[2]);
        }
        CPRINTF("%-9s", buf);

        /* DMAC Type */
        switch (QCE_ENTRY_CONF_KEY_GET(conf->key.key_bits, QOS_QCE_DMAC_TYPE)) {
        case QOS_QCE_DMAC_TYPE_ANY:
            sprintf(buf, "Any");
            break;
        case QOS_QCE_DMAC_TYPE_UC:
            sprintf(buf, "UC");
            break;
        case QOS_QCE_DMAC_TYPE_MC:
            sprintf(buf, "MC");
            break;
        case QOS_QCE_DMAC_TYPE_BC:
            sprintf(buf, "BC");
            break;
        default:
            /* Never reach here */
            sprintf(buf, "?");
            break;
        }
        CPRINTF("%-5s", buf);

        /* VID */
        CPRINTF("%-11s", qcl_qce_range_txt_u16(&conf->key.vid, buf));

        /* PCP and DEI */
        if (!conf->key.pcp.mask)  {
            sprintf(buf, "Any");
        } else if (conf->key.pcp.mask == 0xFF) {
            sprintf(buf, "%u", conf->key.pcp.value);
        } else {
            sprintf(buf, "%u-%u", conf->key.pcp.value, (uchar)(conf->key.pcp.value + ((~conf->key.pcp.mask) & 0xFF)));
        }
        CPRINTF("%-5s", buf);

        bit_val = QCE_ENTRY_CONF_KEY_GET(conf->key.key_bits, QOS_QCE_VLAN_DEI);
        if (bit_val == VTSS_VCAP_BIT_ANY)  {
            sprintf(buf, "Any");
        } else {
            sprintf(buf, "%u", (bit_val - 1));
        }
        CPRINTF("%-5s", buf);
    }

    /* Action */
    if (QCE_ENTRY_CONF_ACTION_GET(conf->action.action_bits, QOS_QCE_ACTION_PRIO)) {
        CPRINTF("%-6u", conf->action.prio);
    } else {
        CPRINTF("-     ");
    }
    if (QCE_ENTRY_CONF_ACTION_GET(conf->action.action_bits, QOS_QCE_ACTION_DP)) {
        CPRINTF("%-3u", conf->action.dp);
    } else {
        CPRINTF("-  ");
    }
    if (QCE_ENTRY_CONF_ACTION_GET(conf->action.action_bits, QOS_QCE_ACTION_DSCP)) {
        CPRINTF("%-11s", qos_dscp2str(conf->action.dscp));
    } else {
        CPRINTF("-          ");
    }

    if (local_status) {
        CPRINTF("%-9s", conf->conflict ? "Yes" : "No");
    }

    CPRINTF("%-s", qcl_qce_port_list_txt(conf, buf));
    CPRINTF("\n");
}

static char *qcl_qce_ipv6_txt(vtss_qce_u32_t *ip, char *buf)
{
    ulong i, n = 0;

    if (!ip->mask[0] && !ip->mask[1] && !ip->mask[2] && !ip->mask[3]) {
        sprintf(buf, "Any");
    } else {
        for (i = 0; i < 32; i++) {
            if (ip->mask[i / 8] & (1 << (i % 8))) {
                n++;
            }
        }
        sprintf(buf, "%u.%u.%u.%u/%d", ip->value[0], ip->value[1],
                ip->value[2], ip->value[3], n);
    }
    return (buf);
}

static void qcl_cli_cmd_list(cli_req_t *req)
{
    qos_qce_entry_conf_t conf;
    BOOL             first = 1;
    vtss_qce_id_t    id;
    char             col[80], buf[MGMT_PORT_BUF_SIZE], cls[16], dp[16], dscp[16];
    vtss_isid_t      isid;
    int              i;
    qos_cli_req_t    *qos_req = req->module_req;
    u8               bit_val;

    if (cli_cmd_switch_none(req)) {
        return;
    }

    isid = (req->stack.master ?
            (req->usid_sel == VTSS_USID_ALL ?
             VTSS_ISID_GLOBAL : req->stack.isid[req->usid_sel]) :
                VTSS_ISID_LOCAL);
    id = qos_req->qce_id;
    if (id != QCE_ID_NONE) {
        if (qos_mgmt_qce_entry_get(isid, QCL_USER_STATIC,
                                   QCL_ID_END, id, &conf, 0) != VTSS_OK) {
            return;
        }

        sprintf(col, "QCE ID     : %d", conf.id);
#if VTSS_SWITCH_STACKABLE
        CPRINTF("%-35s Switch ID: %s\n", col, cli_qcl_usid_txt(conf.isid, buf));
#else
        CPRINTF("%s\n", col);
#endif

        sprintf(col, "Frame Type : %s", qcl_qce_type_txt(conf.type, buf, 1));
        CPRINTF("%-35s Port     : %s\n\n", col, qcl_qce_port_list_txt(&conf, buf));

        sprintf(col, "VLAN Parameters");
        CPRINTF("%-35s MAC Parameters\n", col);
        sprintf(col, "---------------");
        CPRINTF("%-35s --------------\n", col);

        bit_val = QCE_ENTRY_CONF_KEY_GET(conf.key.key_bits, QOS_QCE_VLAN_TAG);
        sprintf(col, "Tag        : %s",
                (bit_val == VTSS_VCAP_BIT_ANY ? "Any" :
                 (bit_val == VTSS_VCAP_BIT_0 ? "Untagged" : "Tagged")));
        if (!conf.key.smac.mask[0] && !conf.key.smac.mask[1] &&
            !conf.key.smac.mask[2]) {
            CPRINTF("%-35s SMAC     : Any\n", col);
        } else {
            CPRINTF("%-35s SMAC     : %02x-%02x-%02x\n", col,
                    conf.key.smac.value[0], conf.key.smac.value[1],
                    conf.key.smac.value[2]);
        }

        /* VID */
        sprintf(col, "VID        : %s", qcl_qce_range_txt_u16(&conf.key.vid, buf));

        /* DMAC Type */
        switch (QCE_ENTRY_CONF_KEY_GET(conf.key.key_bits, QOS_QCE_DMAC_TYPE)) {
        case QOS_QCE_DMAC_TYPE_ANY:
            sprintf(buf, "Any");
            break;
        case QOS_QCE_DMAC_TYPE_UC:
            sprintf(buf, "Unicast");
            break;
        case QOS_QCE_DMAC_TYPE_MC:
            sprintf(buf, "Multicast");
            break;
        case QOS_QCE_DMAC_TYPE_BC:
            sprintf(buf, "Broadcast");
            break;
        default:
            /* never come here! */
            sprintf(buf, "?");
            break;
        }

        CPRINTF("%-35s DMAC Type: %s\n", col, buf);

        /* PCP and DEI */
        if (!conf.key.pcp.mask) {
            CPRINTF("PCP        : %s\n", "Any");
        } else if (conf.key.pcp.mask == 0xFF) {
            CPRINTF("PCP        : %u\n", conf.key.pcp.value);
        } else {
            CPRINTF("PCP        : %u - %u\n", conf.key.pcp.value,
                    (uchar)(conf.key.pcp.value + ((~conf.key.pcp.mask) & 0xFF)));
        }
        bit_val = QCE_ENTRY_CONF_KEY_GET(conf.key.key_bits, QOS_QCE_VLAN_DEI);
        if (bit_val == VTSS_VCAP_BIT_ANY) {
            CPRINTF("DEI        : %s\n\n", "Any");
        } else {
            CPRINTF("DEI        : %u\n\n", (bit_val - 1));
        }

        col[0] = '\0';
        buf[0] = '\0';
        switch (conf.type) {
        case VTSS_QCE_TYPE_ETYPE:
            sprintf(col, "Ethernet Parameters");
            sprintf(buf, "-------------------");
            break;
        case VTSS_QCE_TYPE_LLC:
            sprintf(col, "LLC Parameters");
            sprintf(buf, "--------------");
            break;
        case VTSS_QCE_TYPE_SNAP:
            sprintf(col, "SNAP Parameters");
            sprintf(buf, "---------------");
            break;
        case VTSS_QCE_TYPE_IPV4:
            sprintf(col, "IPv4 Parameters");
            sprintf(buf, "---------------");
            break;
        case VTSS_QCE_TYPE_IPV6:
            sprintf(col, "IPv6 Parameters");
            sprintf(buf, "---------------");
            break;
        default:
            /* frame type 'any', in which case nothing to display */
            break;
        }
        CPRINTF("%-35s Action Parameters\n", col);
        CPRINTF("%-35s -----------------\n", buf);

        if (QCE_ENTRY_CONF_ACTION_GET(conf.action.action_bits, QOS_QCE_ACTION_PRIO)) {
            sprintf(cls, "Class    : %u", conf.action.prio);
        } else {
            sprintf(cls, "Class    : -");
        }
        if (QCE_ENTRY_CONF_ACTION_GET(conf.action.action_bits, QOS_QCE_ACTION_DP)) {
            sprintf(dp, "DP       : %u", conf.action.dp);
        } else {
            sprintf(dp, "DP       : -");
        }
        if (QCE_ENTRY_CONF_ACTION_GET(conf.action.action_bits, QOS_QCE_ACTION_DSCP)) {
            sprintf(dscp, "DSCP     : %s", qos_dscp2str(conf.action.dscp));
        } else {
            sprintf(dscp, "DSCP     : -");
        }

        buf[0] = '\0';
        switch (conf.type) {
        case VTSS_QCE_TYPE_ETYPE:
            if (conf.key.frame.etype.etype.mask[0] || conf.key.frame.etype.etype.mask[1]) {
                sprintf(col, "Ether Type : 0x%x",
                        (conf.key.frame.etype.etype.value[0] << 8) |
                        conf.key.frame.etype.etype.value[1]);
            } else {
                sprintf(col, "Ether Type : Any");
            }
            CPRINTF("%-35s %s\n", col, cls);
            CPRINTF("%-35s %s\n", buf, dp);
            CPRINTF("%-35s %s\n", buf, dscp);
            break;
        case VTSS_QCE_TYPE_LLC:
            if (conf.key.frame.llc.dsap.mask) {
                sprintf(col, "DSAP       : 0x%x", conf.key.frame.llc.dsap.value);
            } else {
                sprintf(col, "DSAP       : Any");
            }
            CPRINTF("%-35s %s\n", col, cls);
            if (conf.key.frame.llc.ssap.mask) {
                sprintf(col, "SSAP       : 0x%x", conf.key.frame.llc.ssap.value);
            } else {
                sprintf(col, "SSAP       : Any");
            }
            CPRINTF("%-35s %s\n", col, dp);
            if (conf.key.frame.llc.control.mask) {
                sprintf(col, "Control    : 0x%x", conf.key.frame.llc.control.value);
            } else {
                sprintf(col, "Control    : Any");
            }
            CPRINTF("%-35s %s\n", col, dscp);
            break;
        case VTSS_QCE_TYPE_SNAP:
            if (conf.key.frame.snap.pid.mask[0] || conf.key.frame.snap.pid.mask[1]) {
                sprintf(col, "PID        : 0x%x",
                        (conf.key.frame.snap.pid.value[0] << 8) |
                        conf.key.frame.snap.pid.value[1]);
            } else {
                sprintf(col, "PID        : Any");
            }
            CPRINTF("%-35s %s\n", col, cls);
            CPRINTF("%-35s %s\n", buf, dp);
            CPRINTF("%-35s %s\n", buf, dscp);
            break;
        case VTSS_QCE_TYPE_IPV4:
            sprintf(col, "Protocol   : %s", qcl_qce_proto_txt(&conf.key.frame.ipv4.proto, buf));
            CPRINTF("%-35s %s\n", col, cls);

            sprintf(col, "SIP        : %s", mgmt_acl_ipv4_txt(&conf.key.frame.ipv4.sip, buf, 0));
            CPRINTF("%-35s %s\n", col, dp);
            sprintf(col, "DSCP       : %s", qcl_qce_range_txt_u8(&conf.key.frame.ipv4.dscp, buf));
            CPRINTF("%-35s %s\n", col, dscp);

            bit_val = QCE_ENTRY_CONF_KEY_GET(conf.key.key_bits,
                                             QOS_QCE_IPV4_FRAGMENT);
            CPRINTF("Fragment   : %s\n",
                    (bit_val == VTSS_VCAP_BIT_ANY ? "Any" :
                     (bit_val == VTSS_VCAP_BIT_0 ? "No" : "Yes")));
            if (conf.key.frame.ipv4.proto.mask &&
                (conf.key.frame.ipv4.proto.value == 6 ||
                 conf.key.frame.ipv4.proto.value == 17)) {
                CPRINTF("Source Port: %s\n", qcl_qce_range_txt_u16(&conf.key.frame.ipv4.sport, buf));
                CPRINTF("Dest Port  : %s\n", qcl_qce_range_txt_u16(&conf.key.frame.ipv4.dport, buf));
            }
            break;
        case VTSS_QCE_TYPE_IPV6:
            sprintf(col, "Protocol   : %s",
                    qcl_qce_proto_txt(&conf.key.frame.ipv6.proto, buf));
            CPRINTF("%-35s %s\n", col, cls);

            sprintf(col, "SIP        : %s",
                    qcl_qce_ipv6_txt(&conf.key.frame.ipv6.sip, buf));
            CPRINTF("%-35s %s\n", col, dp);

            sprintf(col, "DSCP       : %s", qcl_qce_range_txt_u8(&conf.key.frame.ipv6.dscp, buf));
            CPRINTF("%-35s %s\n", col, dscp);

            if (conf.key.frame.ipv6.proto.mask &&
                (conf.key.frame.ipv6.proto.value == 6 ||
                 conf.key.frame.ipv6.proto.value == 17)) {
                CPRINTF("Source Port: %s\n", qcl_qce_range_txt_u16(&conf.key.frame.ipv6.sport, buf));
                CPRINTF("Dest Port  : %s\n", qcl_qce_range_txt_u16(&conf.key.frame.ipv6.dport, buf));
            }
            break;
        default:
            /* frame type 'any', in which case nothing to display */
            buf[0] = '\0';
            CPRINTF("%-35s %s\n", buf, cls);
            CPRINTF("%-35s %s\n", buf, dp);
            CPRINTF("%-35s %s\n", buf, dscp);
            break;
        }
        return;
    }

    /* Overview */
    i = 0;
    id = QCE_ID_NONE;
    while (qos_mgmt_qce_entry_get(isid, QCL_USER_STATIC,
                                  QCL_ID_END, id, &conf, 1) == VTSS_OK) {
        id = conf.id;
        cli_cmd_qcl_overview(QCL_USER_STATIC, id, &conf, &first, 0);
        i++;
    }
    CPRINTF("%sNumber of QCEs: %d\n", i ? "\n" : "", i);
}

static void QOS_cli_cmd_qcl_lookup(cli_req_t *req)
{
    qcl_cli_cmd_list(req);
}

static void QOS_cli_cmd_qcl_status(cli_req_t *req)
{
    qos_cli_req_t       *qos_req = req->module_req;
    int                  qcl_user = qos_req->qcl_user;
    vtss_qce_id_t        qce_id = QCE_ID_NONE;
    qos_qce_entry_conf_t qce_conf;
    BOOL                 first = 1;
    vtss_usid_t          usid;
    vtss_isid_t          isid;
    int                  i = 0;

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++, first = 1) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }
        if (first) {
            cli_cmd_usid_print(usid, req, 1);
        }
        i = 0;
        if (qos_req->qcl_user_valid == 0) {
            qos_req->qcl_user = -1;
        }

        if (qos_req->qcl_user < 0) {
            for (qcl_user = QCL_USER_CNT - 1; qcl_user >= QCL_USER_STATIC; qcl_user--) {
                qce_id = QCE_ID_NONE;
                while (qos_mgmt_qce_entry_get(isid, qcl_user, QCL_ID_END, qce_id, &qce_conf, 1) == VTSS_OK) {
                    qce_id = qce_conf.id;
                    if (qos_req->qcl_user == -1 || (qos_req->qcl_user == -2 && qce_conf.conflict)) {
                        cli_cmd_qcl_overview(qcl_user, qce_id, &qce_conf, &first, 1);
                        i++;
                    }
                }
            }
        } else {
            qce_id = QCE_ID_NONE;
            while (qos_mgmt_qce_entry_get(isid, qcl_user, QCL_ID_END, qce_id, &qce_conf, 1) == VTSS_OK) {
                qce_id = qce_conf.id;
                cli_cmd_qcl_overview(qcl_user, qce_id, &qce_conf, &first, 1);
                i++;
            }
        }
        CPRINTF("%sNumber of QCEs: %d\n", i ? "\n" : "", i);
    }
}

static void QOS_cli_cmd_qcl_refresh(cli_req_t *req)
{
    vtss_isid_t    isid;
    vtss_isid_t    usid;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }
        (void)qos_mgmt_qce_conflict_resolve(isid, QCL_USER_STATIC, QCL_ID_END);
    }
}
#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static char *qos_dscp_classification_txt(vtss_dscp_mode_t mode)
{
    switch (mode) {
    case VTSS_DSCP_MODE_NONE:
        return "None";
    case VTSS_DSCP_MODE_ZERO:
        return "DSCP = 0";
    case VTSS_DSCP_MODE_SEL:
        return "Selected";
    case VTSS_DSCP_MODE_ALL:
        return "All";
    default:
        return NULL;
    }
}

static char *qos_dscp_remark_txt(vtss_dscp_emode_t mode)
{
    switch (mode) {
    case VTSS_DSCP_EMODE_DISABLE:
        return "Disabled";
    case VTSS_DSCP_EMODE_REMARK:
        return "Enabled";
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    case VTSS_DSCP_EMODE_REMAP:
        return "Remap-DP unaware";
    case VTSS_DSCP_EMODE_REMAP_DPA:
        return "Remap-DP aware";
#else
    case VTSS_DSCP_EMODE_REMAP:
        return "Remapped";
#endif
    default:
        return NULL;
    }
}

/*
 * QoS port DSCP configuration: Ingress and Egress
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_port_dscp(cli_req_t *req,
                                  BOOL      qos_class,
                                  BOOL      translate,
                                  BOOL      ingr_rewr,
                                  BOOL      egr_rewr)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                if (qos_class) {
                    if (req->enable) {
                        conf.dscp_class_enable = TRUE;
                    }
                    if (req->disable) {
                        conf.dscp_class_enable = FALSE;
                    }
                }
                if (translate) {
                    if (req->enable) {
                        conf.dscp_translate = TRUE;
                    }
                    if (req->disable) {
                        conf.dscp_translate = FALSE;
                    }
                }
                if (ingr_rewr) {
                    conf.dscp_imode = qos_req->dscp_ingr_class_mode;
                }
                if (egr_rewr) {
                    conf.dscp_emode = qos_req->dscp_remark_mode;
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    if (qos_class) {
                        p += sprintf(p, "DSCP based class.  ");
                    }
                    if (translate) {
                        p += sprintf(p, "DSCP trans.  ");
                    }
                    if (ingr_rewr) {
                        p += sprintf(p, "Ingress class.  ");
                    }
                    if (egr_rewr) {
                        p += sprintf(p, "Rewrite       ");
                    }
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                if (qos_class) {
                    CPRINTF("%-19s", cli_bool_txt(conf.dscp_class_enable));
                }
                if (translate) {
                    CPRINTF("%-13s", cli_bool_txt(conf.dscp_translate));
                }
                if (ingr_rewr) {
                    CPRINTF("%-16s", qos_dscp_classification_txt(conf.dscp_imode));
                }
                if (egr_rewr) {
                    CPRINTF("%-16s", qos_dscp_remark_txt(conf.dscp_emode));
                }
                CPRINTF("\n");
            }
        }
    }
}

/*
 * DSCP based QoS Classification
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_dscp_qos_classification(cli_req_t *req,
                                                BOOL      trust,
                                                BOOL      class)
{
    qos_conf_t      conf;
    int             cnt;
    char            buf[80], *p;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_conf_get(&conf) != VTSS_OK) {
        return;
    }

    if (req->set) {
        for (cnt = 0; cnt < 64; cnt++) {
            if (qos_req->dscp_list[cnt] == 0) {
                continue;
            }

            if (trust) {
                if (req->enable) {
                    conf.dscp.dscp_trust[cnt] = TRUE;
                }
                if (req->disable) {
                    conf.dscp.dscp_trust[cnt] = FALSE;
                }
            }
            if (class) {
                if (req->class_spec == CLI_SPEC_VAL) {
                    conf.dscp.dscp_qos_class_map[cnt] = req->class_;
                }
                if (qos_req->dpl_valid) {
                    conf.dscp.dscp_dp_level_map[cnt] = qos_req->dpl;
                }
            }
        }
        (void) qos_conf_set(&conf);
    } else {
        p = &buf[0];
        p += sprintf(p, "DSCP       ");
        if (trust) {
            p += sprintf(p, "Trust     ");
        }
        if (class) {
            p += sprintf(p, "QoS Class  DP Level");
        }
        cli_table_header(buf);

        for (cnt = 0; cnt < 64; cnt++) {
            if (qos_req->dscp_list[cnt] == 0) {
                continue;
            }
            CPRINTF("%-11s", qos_dscp2str(cnt));
            if (trust) {
                CPRINTF("%-10s", cli_bool_txt(conf.dscp.dscp_trust[cnt]));
            }
            if (class) {
                CPRINTF("%-10s %-8d",
                        mgmt_prio2txt(conf.dscp.dscp_qos_class_map[cnt], 0),
                        conf.dscp.dscp_dp_level_map[cnt]);
            }
            CPRINTF("\n");
        }
    }
}

/*
 * DSCP Translation: ingress and egress
 * This function supports that more than one parameter flag
 * is TRUE in a get request (req->set == FALSE)
 */
static void QOS_cli_cmd_dscp_translation(cli_req_t *req,
                                         BOOL      translate,
                                         BOOL      classify,
                                         BOOL      remap)
{
    qos_conf_t      conf;
    int             cnt;
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    int             dp_cnt;
#endif
    char            buf[80], *p;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_conf_get(&conf) != VTSS_OK) {
        return;
    }

    if (req->set) {
        for (cnt = 0; cnt < 64; cnt++) {
            if (qos_req->dscp_list[cnt] == 0) {
                continue;
            }

            if (translate) {
                conf.dscp.translate_map[cnt] = qos_req->dscp;
            }
            if (classify) {
                if (req->enable) {
                    conf.dscp.ingress_remark[cnt] = TRUE;
                }
                if (req->disable) {
                    conf.dscp.ingress_remark[cnt] = FALSE;
                }
            }
            if (remap) {
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
                for (dp_cnt = 0; dp_cnt <= 1; dp_cnt++) {
                    if (qos_req->dpl_list[dp_cnt] == 0) {
                        continue;
                    }
                    if (dp_cnt == 0) {
                        conf.dscp.egress_remap[cnt] = qos_req->dscp;
                    } else {
                        conf.dscp.egress_remap_dp1[cnt] = qos_req->dscp;
                    }
                }
#else
                conf.dscp.egress_remap[cnt] = qos_req->dscp;
#endif
            }
        }
        (void) qos_conf_set(&conf);
    } else {
        p = &buf[0];
        p += sprintf(p, "DSCP       ");
        if (translate) {
            p += sprintf(p, "Ingr. Trans.  ");
        }
        if (classify) {
            p += sprintf(p, "Ingr. Classify  ");
        }
        if (remap) {
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
            p += sprintf(p, "Egr. Remap-DP0  Egr. Remap-DP1");
#else
            p += sprintf(p, "Egr. Remap");
#endif
        }
        cli_table_header(buf);
        for (cnt = 0; cnt < 64; cnt++) {
            if (qos_req->dscp_list[cnt] == 0) {
                continue;
            }
            CPRINTF("%-11s", qos_dscp2str(cnt));
            if (translate) {
                CPRINTF("%-14s", qos_dscp2str(conf.dscp.translate_map[cnt]));
            }
            if (classify) {
                CPRINTF("%-16s", cli_bool_txt(conf.dscp.ingress_remark[cnt]));
            }
            if (remap) {
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
                CPRINTF("%-15s %-11s", qos_dscp2str(conf.dscp.egress_remap[cnt]), qos_dscp2str(conf.dscp.egress_remap_dp1[cnt]));
#else
                CPRINTF("%-11s", qos_dscp2str(conf.dscp.egress_remap[cnt]));
#endif
            }
            CPRINTF("\n");
        }
    }
}

static void QOS_cli_cmd_port_classification_dscp(cli_req_t *req)
{
    QOS_cli_cmd_port_dscp(req, 1, 0, 0, 0);
}

static void QOS_cli_cmd_port_dscp_translation(cli_req_t *req)
{
    QOS_cli_cmd_port_dscp(req, 0, 1, 0, 0);
}

static void QOS_cli_cmd_dscp_ingr_translation(cli_req_t *req)
{
    QOS_cli_cmd_dscp_translation(req, 1, 0, 0);
}

static void QOS_cli_cmd_dscp_trust(cli_req_t *req)
{
    QOS_cli_cmd_dscp_qos_classification(req, 1, 0);
}

static void QOS_cli_cmd_dscp_ingr_map(cli_req_t *req)
{
    QOS_cli_cmd_dscp_qos_classification(req, 0, 1);
}

static void QOS_cli_cmd_port_dscp_ingr_classification(cli_req_t *req)
{
    QOS_cli_cmd_port_dscp(req, 0, 0, 1, 0);
}

static void QOS_cli_cmd_dscp_ingr_classification_map(cli_req_t *req)
{
    qos_conf_t      conf;
    int             cls_cnt;
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    int             dp_cnt;
#endif
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_conf_get(&conf) != VTSS_OK) {
        return;
    }

    if (req->set) {
        for (cls_cnt = 0; cls_cnt < QOS_PORT_PRIO_CNT; cls_cnt++) {
            if (qos_req->class_list[cls_cnt] == 0) {
                continue;
            }
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
            for (dp_cnt = 0; dp_cnt <= 1; dp_cnt++) {
                if (qos_req->dpl_list[dp_cnt] == 0) {
                    continue;
                }
                if (dp_cnt == 0) {
                    conf.dscp.qos_class_dscp_map[cls_cnt] = qos_req->dscp;
                } else {
                    conf.dscp.qos_class_dscp_map_dp1[cls_cnt] = qos_req->dscp;
                }
            }
#else
            conf.dscp.qos_class_dscp_map[cls_cnt] = qos_req->dscp;
#endif
        }
        (void) qos_conf_set(&conf);
    } else {
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
        cli_table_header("QoS Class  DP Level  DSCP       ");
#else
        cli_table_header("QoS Class  DSCP       ");
#endif
        for (cls_cnt = 0; cls_cnt < QOS_PORT_PRIO_CNT; cls_cnt++) {
            if (qos_req->class_list[cls_cnt] == 0) {
                continue;
            }
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
            for (dp_cnt = 0; dp_cnt <= 1; dp_cnt++) {
                if (qos_req->dpl_list[dp_cnt] == 0) {
                    continue;
                }
                if (dp_cnt == 0) {
                    CPRINTF("%-10d %-9d %-11s\n", cls_cnt, dp_cnt,
                            qos_dscp2str(conf.dscp.qos_class_dscp_map[cls_cnt]));
                } else {
                    CPRINTF("%-10d %-9d %-11s\n", cls_cnt, dp_cnt,
                            qos_dscp2str(conf.dscp.qos_class_dscp_map_dp1[cls_cnt]));
                }
            }
#else
            CPRINTF("%-10d %-11s\n", cls_cnt,
                    qos_dscp2str(conf.dscp.qos_class_dscp_map[cls_cnt]));
#endif
        }
    }
}

static void QOS_cli_cmd_dscp_ingr_classification_mode(cli_req_t *req)
{
    QOS_cli_cmd_dscp_translation(req, 0, 1, 0);
}

static void QOS_cli_cmd_port_dscp_egress_remark_mode(cli_req_t *req)
{
    QOS_cli_cmd_port_dscp(req, 0, 0, 0, 1);
}

static void QOS_cli_cmd_dscp_egress_remap(cli_req_t *req)
{
    QOS_cli_cmd_dscp_translation(req, 0, 0, 1);
}
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

/* QoS configuration */
static void QOS_cli_cmd_conf(cli_req_t *req)
{
    if (!req->set) {
        cli_header("QoS Configuration", 1);
    }
    cli_header("QoS Port Classification", 0);
    QOS_cli_cmd_port_classification(req, 1, 1, 1, 1, 1);

#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    cli_header("QoS Port Classification Map", 0);
    QOS_cli_cmd_port_classification_map(req);
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    cli_header("QoS Port DSCP Configuration", 0);
    QOS_cli_cmd_port_dscp(req, 1, 1, 1, 1);
    if (!req->set) {
        qos_cli_req_t   *qos_req = req->module_req;
        (void)cli_parm_parse_list(NULL, qos_req->dscp_list, 0, 63, 1);
        (void) cli_parm_parse_list(NULL, qos_req->class_list,   0, QOS_PORT_PRIO_CNT - 1,  1);
    }
    cli_header("DSCP Based QoS Ingress Classification", 0);
    QOS_cli_cmd_dscp_qos_classification(req, 1, 1);
    cli_header("DSCP Ingress and Egress Translation", 0);
    QOS_cli_cmd_dscp_translation(req, 1, 1, 1);
    cli_header("DSCP Ingress Classification Map", 0);
    QOS_cli_cmd_dscp_ingr_classification_map(req);
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

    cli_header("QoS Port Policer", 0);
    QOS_cli_cmd_port_policer(req, 1, 1, 1, 1, 1, 1, 1, 1, 1, (qos_port_policer_fc_cmd_disabled == FALSE));
#if defined(VTSS_SW_OPTION_BUILD_CE)
#ifdef VTSS_FEATURE_QOS_QUEUE_POLICER
    cli_header("QoS Port Queue Policer", 0);
    QOS_cli_cmd_port_queue_policer(req, 1, 1);
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */
#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
    cli_header("QoS Port Scheduler Mode", 0);
    QOS_cli_cmd_port_scheduler_mode(req);
    cli_header("QoS Port Scheduler Weight", 0);
    QOS_cli_cmd_port_scheduler_weight(req);
    cli_header("QoS Port Queue Shaper", 0);
    QOS_cli_cmd_port_queue_shaper(req, 1, 1, 1);
    cli_header("QoS Port Shaper", 0);
    QOS_cli_cmd_port_shaper(req, 1, 1);
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    cli_header("QoS Port Tag Remarking", 0);
    QOS_cli_cmd_port_tag_remark(req, 1, 1, 1, 1);
    cli_header("QoS Port Tag Remarking Map", 0);
    QOS_cli_cmd_port_tag_remarking_map(req);
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
    cli_header("QoS Port Storm Control", 0);
    QOS_cli_cmd_port_storm(req, 1, 1, 1);
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
    cli_header("QoS Storm Control", 0);
    QOS_cli_cmd_storm(req, 1, 1, 1);
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */

#ifdef VTSS_FEATURE_QOS_WRED
    cli_header("QoS WRED", 0);
    QOS_cli_cmd_wred(req);
#endif /* VTSS_FEATURE_QOS_WRED */

#ifdef VTSS_FEATURE_VSTAX_V2
    cli_header("QoS Congestion Management", 0);
    QOS_cli_cmd_cmef(req);
#endif /* VTSS_FEATURE_VSTAX_V2 */

#ifdef VTSS_FEATURE_QCL_V2
    cli_header("QoS QCL", 0);
    QOS_cli_cmd_qcl_lookup(req);
#endif /* VTSS_FEATURE_QCL_V2 */
}

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
static void QOS_cli_cmd_lport_shaper_mode(cli_req_t *req)
{
    u8                    tmp_lport;
    qos_cli_req_t         *qos_req = req->module_req;
    qos_lport_conf_t      conf;
    vtss_rc               rc;

    if (!req->set) {
        CPRINTF ("Lport  Mode\n");
        CPRINTF ("----  --------\n");
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                CPRINTF ("%-5u  %-10s\n", tmp_lport, cli_bool_txt(conf.shaper_host_status));
            }
        }
    } else {
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (qos_req->lport_list[tmp_lport]) {
                if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                    continue;
                }
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                conf.shaper_host_status = req->enable;
                rc = qos_mgmt_lport_conf_set(tmp_lport - 1, &conf);
            }
        }
    }
}

static void QOS_cli_cmd_lport_shaper_rate(cli_req_t *req)
{
    u8                    tmp_lport;
    qos_cli_req_t         *qos_req = req->module_req;
    char                  buf[80];
    qos_lport_conf_t      conf;
    vtss_rc               rc;

    if (!req->set) {
        CPRINTF ("Lport  Rate\n");
        CPRINTF ("-----  --------\n");
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                CPRINTF ("%-5u  %-11s\n", tmp_lport, QOS_cli_bit_rate_txt(conf.shaper.rate, buf));
            }
        }
    } else {
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                conf.shaper.rate = qos_req->bit_rate;
                rc = qos_mgmt_lport_conf_set(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
            }
        }
    }
}

static void QOS_cli_cmd_lport_queue_shaper_mode(cli_req_t *req)
{
    u8                    tmp_lport;
    u8                    tmp_queue;
    qos_cli_req_t         *qos_req = req->module_req;
    qos_lport_conf_t      conf;
    vtss_rc               rc;
    BOOL                  first = FALSE;
    u8                    max_queue = 7;
    u8                    min_queue = 7;

    if (ce_max_mgmt_lport_queue_is_valid(max_queue) == TRUE) {
        min_queue = 6;
    } else {
        min_queue = 2;
        max_queue = 3;
    }

    if (!req->set) {
        CPRINTF ("Lport  Queue  Mode\n");
        CPRINTF ("-----  -----  --------\n");
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            first = TRUE;
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                for (tmp_queue = max_queue; tmp_queue >= min_queue; tmp_queue--) {
                    if (ce_max_mgmt_lport_queue_is_valid(tmp_queue - 1) == FALSE) {
                        continue;
                    }
                    if (!qos_req->queue_list[tmp_queue]) {
                        continue;
                    }
                    if (first == TRUE) {
                        CPRINTF ("%-5u", tmp_lport);
                        first = FALSE;
                    } else {
                        CPRINTF ("     ");
                    }
                    CPRINTF ("  %-5u  %-10s\n", tmp_queue,
                             cli_bool_txt(conf.scheduler.shaper_queue_status[tmp_queue - min_queue]));
                }
            }
        }
    }
    if (req->set) {
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (qos_req->lport_list[tmp_lport]) {
                if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                    continue;
                }
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                for (tmp_queue = max_queue; tmp_queue >= min_queue; tmp_queue--) {
                    if (ce_max_mgmt_lport_queue_is_valid(tmp_queue - 1) == FALSE) {
                        continue;
                    }
                    if (!qos_req->queue_list[tmp_queue]) {
                        continue;
                    }
                    conf.scheduler.shaper_queue_status[tmp_queue - min_queue] = req->enable;
                }
                rc = qos_mgmt_lport_conf_set(tmp_lport - 1, &conf);
            }
        }
    }

}

static void QOS_cli_cmd_lport_queue_shaper_rate(cli_req_t *req)
{
    u8                    tmp_lport;
    u8                    tmp_queue;
    qos_cli_req_t         *qos_req = req->module_req;
    qos_lport_conf_t      conf;
    vtss_rc               rc;
    BOOL                  first = FALSE;
    i8                    buf[80];
    u8                    max_queue = 7;
    u8                    min_queue = 7;

    if (ce_max_mgmt_lport_queue_is_valid(max_queue) == TRUE) {
        min_queue = 6;
    } else {
        min_queue = 2;
        max_queue = 3;
    }

    if (!req->set) {
        CPRINTF ("Lport  Queue  Rate\n");
        CPRINTF ("-----  -----  --------\n");
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            first = TRUE;
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                for (tmp_queue = max_queue; tmp_queue >= min_queue; tmp_queue--) {
                    if (ce_max_mgmt_lport_queue_is_valid(tmp_queue - 1) == FALSE) {
                        continue;
                    }
                    if (!qos_req->queue_list[tmp_queue]) {
                        continue;
                    }
                    if (first == TRUE) {
                        CPRINTF ("%-5u", tmp_lport);
                        first = FALSE;
                    } else {
                        CPRINTF ("     ");
                    }
                    CPRINTF ("  %-5u  %-10s\n", tmp_queue,
                             QOS_cli_bit_rate_txt(conf.scheduler.queue[tmp_queue - min_queue].rate, buf));
                }
            }
        }
    }
    if (req->set) {
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (qos_req->lport_list[tmp_lport]) {
                if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                    continue;
                }
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                for (tmp_queue = max_queue; tmp_queue >= min_queue; tmp_queue--) {
                    if (ce_max_mgmt_lport_queue_is_valid(tmp_queue - 1) == FALSE) {
                        continue;
                    }
                    conf.scheduler.queue[tmp_queue - min_queue].rate = qos_req->bit_rate;
                }
                rc = qos_mgmt_lport_conf_set(tmp_lport - 1, &conf);
            }
        }
    }
}

static void QOS_cli_cmd_lport_queue_scheduler_weight(cli_req_t *req)
{
    u8                    tmp_lport;
    u8                    tmp_queue;
    qos_cli_req_t         *qos_req = req->module_req;
    qos_lport_conf_t      conf;
    vtss_rc               rc;
    BOOL                  first = FALSE;
    vtss_pct_t            pct[6];
    u8                    max_queue = 5;

    if (ce_max_mgmt_lport_queue_is_valid(max_queue) == TRUE) {
        max_queue = 6;
    } else {
        max_queue = 2;
    }

    if (!req->set) {
        CPRINTF ("Lport  Queue  Weight\n");
        CPRINTF ("-----  -----  --------\n");
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            first = TRUE;
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                QOS_cli_lport_queue_weight2pct(conf.scheduler.queue_pct, pct);
                for (tmp_queue = 0; tmp_queue < max_queue; tmp_queue++) {
                    if (ce_max_mgmt_lport_queue_is_valid(tmp_queue) == FALSE) {
                        continue;
                    }
                    if (!qos_req->queue_list[tmp_queue]) {
                        continue;
                    }
                    if (first == TRUE) {
                        CPRINTF ("%-5u", tmp_lport);
                        first = FALSE;
                    } else {
                        CPRINTF ("     ");
                    }
                    CPRINTF ("  %-5u  %3u (%d%%)\n", tmp_queue, conf.scheduler.queue_pct[tmp_queue], pct[tmp_queue]);
                }
            }
        }
    }
    if (req->set) {
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                for (tmp_queue = 0; tmp_queue < max_queue; tmp_queue++) {
                    if (ce_max_mgmt_lport_queue_is_valid(tmp_queue) == FALSE) {
                        continue;
                    }
                    if (!qos_req->queue_list[tmp_queue]) {
                        continue;
                    }
                    conf.scheduler.queue_pct[tmp_queue] = qos_req->weight;
                }
                rc = qos_mgmt_lport_conf_set(tmp_lport - 1, &conf);
            }
        }
    }

}

static void QOS_cli_cmd_lport_scheduler_weight(cli_req_t *req)
{
    qos_cli_req_t         *qos_req = req->module_req;
    qos_lport_conf_t      conf;
    vtss_pct_t            pct[VTSS_LPORTS];
    u8                    lport_pct[VTSS_LPORTS];
    vtss_host_mode_t      host_mode;
    u8                    total_instance = 0, tmp_instance;
    vtss_port_no_t        hmda = 0, hmdb = 0, tmp_lport, total_lports = 0, tmp_port, start_lport, end_lport;
    BOOL                  first;
    vtss_rc               rc = VTSS_RC_OK;

    /* Find the hmda interafce */
    for (tmp_port = CE_MAX_XAUI_NO_START; tmp_port < (CE_MAX_XAUI_NO_END + 1); tmp_port++) {
        if (hmda == 0 && vtss_port_is_host(NULL, tmp_port) == TRUE) {
            hmda = tmp_port;
            break;
        }
    }
    if (hmda == 0) {
        return; /* No valid HMDA interface */
    }

    /* Find the hmdb interafce */
    for (tmp_port = hmda + 1; tmp_port < (CE_MAX_XAUI_NO_END + 1); tmp_port++) {
        if (hmdb == 0 && vtss_port_is_host(NULL, tmp_port) == TRUE) {
            hmdb = tmp_port;
            break;
        }
    }

    if (ce_max_mgmt_current_host_mode_get(&host_mode) != VTSS_RC_OK) {
        CPRINTF ("Unable to get the current host mode\n");
        return;
    }
    switch (host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
    case VTSS_HOST_MODE_0:
        total_instance = 2;
        total_lports  = 12;
        break;
    case VTSS_HOST_MODE_1:
        total_instance = 2;
        total_lports  = 24;
        break;
#endif
    case VTSS_HOST_MODE_2:
        total_instance = 1;
        total_lports  = 24;
        break;
    case VTSS_HOST_MODE_3:
        total_instance = 1;
        total_lports  = 48;
        break;
    default:
        rc = VTSS_RC_ERROR;
        break;
    }
    if (rc != VTSS_RC_OK) {
        return;
    }

    if ((hmdb && total_instance == 1) || (!hmdb && total_instance == 2)) {
        CPRINTF ("Wrong host interafce \n");
    }

    if (!req->set) {
        CPRINTF ("Port  Lport  Weight\n");
        CPRINTF ("----  -----  ------\n");
        for (tmp_instance = 0; tmp_instance < total_instance; tmp_instance++) {
            first = TRUE;
            start_lport = total_lports * tmp_instance;
            end_lport   = total_lports * (tmp_instance + 1);
            for (tmp_lport = start_lport; tmp_lport < end_lport; tmp_lport++) {
                if (ce_max_mgmt_lport_is_valid(tmp_lport) == FALSE) {
                    continue;
                }
                rc = qos_mgmt_lport_conf_get(tmp_lport, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                lport_pct[tmp_lport - start_lport] = conf.pct;
            }
            QOS_cli_lport_weight2pct(lport_pct, pct, total_lports);

            for (tmp_lport = start_lport; tmp_lport < end_lport ; tmp_lport++) {
                if (ce_max_mgmt_lport_is_valid(tmp_lport) == FALSE) {
                    continue;
                }
                if (qos_req->lport_list[tmp_lport + 1]) {
                    rc = qos_mgmt_lport_conf_get(tmp_lport, &conf);
                    if (rc != VTSS_RC_OK) {
                        continue;
                    }
                    if (first == TRUE) {
                        if (tmp_instance == 0) {
                            CPRINTF("%-4u  ", hmda + 1);
                        } else {
                            CPRINTF("%-4u  ", hmdb + 1);
                        }
                        first = FALSE;
                    } else {
                        CPRINTF ("      ");
                    }
                    CPRINTF ("%-5u  %3u (%d%%)\n", tmp_lport + 1, conf.pct, pct[tmp_lport - start_lport]);
                }
            }
        }
    } else {
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                conf.pct = qos_req->weight;
                rc = qos_mgmt_lport_conf_set(tmp_lport - 1, &conf);
            }
        }
    }
}

static void QOS_cli_cmd_host_wred(cli_req_t *req)
{
    u8                    tmp_lport;
    u8                    tmp_queue;
    qos_cli_req_t         *qos_req = req->module_req;
    qos_lport_conf_t      conf;
    vtss_rc               rc;
    BOOL                  first = FALSE;
    u8                    max_queue = 7;
    u8                    min_queue = 0;

    if (ce_max_mgmt_lport_queue_is_valid(max_queue) == FALSE) {
        max_queue = 3;
    }

    if (!req->set) {
        CPRINTF ("Lport  Queue  Mode      Min Th  Mdp 1  Mdp 2  Mdp 3 \n");
        CPRINTF ("-----  -----  --------  ------  -----  -----  ----- \n");
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            first = TRUE;
            if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                continue;
            }
            if (qos_req->lport_list[tmp_lport]) {
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                for (tmp_queue = min_queue; tmp_queue <= max_queue; tmp_queue++) {
                    if (ce_max_mgmt_lport_queue_is_valid(tmp_queue) == FALSE) {
                        continue;
                    }
                    if (!qos_req->queue_list[tmp_queue]) {
                        continue;
                    }
                    if (first == TRUE) {
                        CPRINTF ("%-5u", tmp_lport);
                        first = FALSE;
                    } else {
                        CPRINTF ("     ");
                    }
                    CPRINTF ("  %-5u  %-10s  ", tmp_queue,
                             cli_bool_txt(conf.red[tmp_queue - min_queue].enable));
                    CPRINTF ("%-6u  %-5u  %-5u  %-5u\n", conf.red[tmp_queue - min_queue].min_th,
                             conf.red[tmp_queue - min_queue].max_prob_1,
                             conf.red[tmp_queue - min_queue].max_prob_2,
                             conf.red[tmp_queue - min_queue].max_prob_3);
                }
            }
        }
    }
    if (req->set) {
        for (tmp_lport = 1; tmp_lport <= VTSS_LPORTS; tmp_lport++) {
            if (qos_req->lport_list[tmp_lport]) {
                if (ce_max_mgmt_lport_is_valid(tmp_lport - 1) == FALSE) {
                    continue;
                }
                rc = qos_mgmt_lport_conf_get(tmp_lport - 1, &conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                for (tmp_queue = min_queue; tmp_queue <= max_queue; tmp_queue++) {
                    if (ce_max_mgmt_lport_queue_is_valid(tmp_queue) == FALSE) {
                        continue;
                    }
                    if (!qos_req->queue_list[tmp_queue]) {
                        continue;
                    }
                    conf.red[tmp_queue - min_queue].enable = req->enable;
                    if (qos_req->min_th_valid) {
                        conf.red[tmp_queue - min_queue].min_th = qos_req->min_th;
                    }
                    if (qos_req->mdp_1_valid) {
                        conf.red[tmp_queue - min_queue].max_prob_1 = qos_req->mdp_1;
                    }
                    if (qos_req->mdp_2_valid) {
                        conf.red[tmp_queue - min_queue].max_prob_2 = qos_req->mdp_2;
                    }
                    if (qos_req->mdp_3_valid) {
                        conf.red[tmp_queue - min_queue].max_prob_3 = qos_req->mdp_3;
                    }
                }
                rc = qos_mgmt_lport_conf_set(tmp_lport - 1, &conf);
            }
        }
    }

    return;
}
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
static void QOS_cli_cmd_dot3ar_rate_limiter(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    vtss_port_no_t  tmp_port, port_no;
    qos_cli_req_t   *qos_req = req->module_req;
    u8              mode_sel;
    qos_port_conf_t port_conf;
    vtss_rc         rc = VTSS_RC_OK;
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    BOOL            first_flag = FALSE;
#endif

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }
        if (req->set) {
            for (tmp_port = VTSS_PORT_NO_START; tmp_port < VTSS_PORT_NO_END; tmp_port++) {
                if (PORT_NO_IS_STACK(tmp_port)) {
                    continue;
                }
                port_no = iport2uport(tmp_port);
                if (req->uport_list[port_no] == 0) {
                    continue;
                }
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
                if (vtss_port_is_host(NULL, tmp_port) == TRUE) {
                    if (first_flag == FALSE) {
                        CPRINTF ("DOT3AR is not supported on host interfaces\n");
                        first_flag = TRUE;
                    }
                    continue;
                }
#endif
                rc = qos_port_conf_get(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                switch (qos_req->mode_sel) {
                case 1:
                    port_conf.tx_rate_limiter.frame_overhead_enable = qos_req->frame_overhead_enable;
                    if (qos_req->frame_overhead) {
                        port_conf.tx_rate_limiter.frame_overhead = qos_req->frame_overhead;
                    }
                    break;
                case 2:
                    port_conf.tx_rate_limiter.payload_rate_enable = qos_req->payload_rate_enable;
                    if (qos_req->payload_rate) {
                        port_conf.tx_rate_limiter.payload_rate = qos_req->payload_rate;
                    }
                    break;
                case 4:
                    if (port_conf.tx_rate_limiter.accumulate_mode_enable == TRUE) {
                        CPRINTF ("Accumulate mode is enabled\n");
                        return;
                    }
                    port_conf.tx_rate_limiter.frame_rate_enable = qos_req->frame_rate_enable;
                    if (qos_req->frame_rate) {
                        port_conf.tx_rate_limiter.frame_rate = qos_req->frame_rate;
                    }
                    break;
                default:
                    rc = VTSS_RC_ERROR;
                    break;
                }
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                rc = qos_port_conf_set(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
            }
        } else {
            if (!qos_req->mode_sel) {
                mode_sel = 0x7; /* All modes */
                CPRINTF ("Port  Frame Overhead       Payload Rate Limiter        Frame Rate Limiter\n");
                CPRINTF ("      Mode      Overhead   Mode      Utilization Rate  Mode      Frame rate\n");
                CPRINTF ("----  -------------------  --------------------------  --------------------\n");

            } else {
                mode_sel = qos_req->mode_sel;
                CPRINTF ("Port  ");
                switch (mode_sel) {
                case 1:
                    CPRINTF("Frame Overhead \n");
                    CPRINTF("      Mode      Overhead \n");
                    CPRINTF("----  -------------------------\n");
                    break;
                case 2:
                    CPRINTF("Payload Rate Limiter \n");
                    CPRINTF("      Mode      Utilization Rate \n");
                    CPRINTF("----  --------------------------\n");
                    break;
                case 3:
                    CPRINTF("Frame Rate Limiter \n");
                    CPRINTF("      Mode     Frame Rate \n");
                    CPRINTF("----  --------------------------\n");
                    break;
                default:
                    break;
                }
            }
            for (tmp_port = VTSS_PORT_NO_START; tmp_port < VTSS_PORT_NO_END; tmp_port++) {
                if (PORT_NO_IS_STACK(tmp_port)) {
                    continue;
                }
                port_no = iport2uport(tmp_port);
                if (req->uport_list[port_no] == 0) {
                    continue;
                }

                rc = qos_port_conf_get(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                if (mode_sel == 0x7) { /* All modes */
                    CPRINTF ("%-4u  %-9s %-9u  ", port_no,
                             enable_str[port_conf.tx_rate_limiter.frame_overhead_enable],
                             port_conf.tx_rate_limiter.frame_overhead);
                    CPRINTF ("%-9s %-16u  ", enable_str[port_conf.tx_rate_limiter.payload_rate_enable],
                             port_conf.tx_rate_limiter.payload_rate);
                    CPRINTF ("%-9s %-10u  \n", enable_str[port_conf.tx_rate_limiter.frame_rate_enable],
                             port_conf.tx_rate_limiter.frame_rate);
                } else {
                    switch (mode_sel) {
                    case 1:
                        CPRINTF ("%-4u  %-9s %-15u  \n", port_no,
                                 enable_str[port_conf.tx_rate_limiter.frame_overhead_enable],
                                 port_conf.tx_rate_limiter.frame_overhead);
                        break;
                    case 2:
                        CPRINTF ("%-4u  %-9s %-15u  \n", port_no,
                                 enable_str[port_conf.tx_rate_limiter.payload_rate_enable],
                                 port_conf.tx_rate_limiter.payload_rate);
                        break;
                    case 3:
                        CPRINTF ("%-4u  %-9s %-15u  \n", port_no,
                                 enable_str[port_conf.tx_rate_limiter.frame_rate_enable],
                                 port_conf.tx_rate_limiter.frame_rate);
                        break;
                    }
                }
            }
        }
    }
    return;
}
static void QOS_cli_cmd_dot3ar_preamble(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    vtss_port_no_t  tmp_port, port_no;
    qos_cli_req_t   *qos_req = req->module_req;
    qos_port_conf_t port_conf;
    vtss_rc         rc = VTSS_RC_OK;
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    BOOL            first_flag = FALSE;
#endif


    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }
        if (req->set) {
            for (tmp_port = VTSS_PORT_NO_START; tmp_port < VTSS_PORT_NO_END; tmp_port++) {
                if (PORT_NO_IS_STACK(tmp_port)) {
                    continue;
                }
                port_no = iport2uport(tmp_port);
                if (req->uport_list[port_no] == 0) {
                    continue;
                }
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
                if (vtss_port_is_host(NULL, tmp_port) == TRUE) {
                    if (first_flag == FALSE) {
                        CPRINTF ("DOT3AR is not supported on host interfaces\n");
                        first_flag = TRUE;
                    }
                    continue;
                }
#endif
                rc = qos_port_conf_get(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                port_conf.tx_rate_limiter.preamble_in_payload = qos_req->preamble_in_payload;
                rc = qos_port_conf_set(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
            }
        } else {
            CPRINTF ("Port  Preamble in payload\n");
            CPRINTF ("----  -------------------\n");
            for (tmp_port = VTSS_PORT_NO_START; tmp_port < VTSS_PORT_NO_END; tmp_port++) {
                if (PORT_NO_IS_STACK(tmp_port)) {
                    continue;
                }
                port_no = iport2uport(tmp_port);
                if (req->uport_list[port_no] == 0) {
                    continue;
                }
                rc = qos_port_conf_get(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                CPRINTF("%-4u  %-9s\n", port_no, include_str[port_conf.tx_rate_limiter.preamble_in_payload]);
            }
        }
    }
    return;
}
static void QOS_cli_cmd_dot3ar_header(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    vtss_port_no_t  tmp_port, port_no;
    qos_cli_req_t   *qos_req = req->module_req;
    qos_port_conf_t port_conf;
    vtss_rc         rc = VTSS_RC_OK;
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    BOOL            first_flag = FALSE;
#endif


    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }
        if (req->set) {
            for (tmp_port = VTSS_PORT_NO_START; tmp_port < VTSS_PORT_NO_END; tmp_port++) {
                if (PORT_NO_IS_STACK(tmp_port)) {
                    continue;
                }
                port_no = iport2uport(tmp_port);
                if (req->uport_list[port_no] == 0) {
                    continue;
                }
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
                if (vtss_port_is_host(NULL, tmp_port) == TRUE) {
                    if (first_flag == FALSE) {
                        CPRINTF ("DOT3AR is not supported on host interfaces\n");
                        first_flag = TRUE;
                    }
                    continue;
                }
#endif
                rc = qos_port_conf_get(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                port_conf.tx_rate_limiter.header_in_payload = qos_req->header_in_payload;
                rc = qos_port_conf_set(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
            }
        } else {
            CPRINTF ("Port  Header in payload \n");
            CPRINTF ("----  ----------------- \n");
            for (tmp_port = VTSS_PORT_NO_START; tmp_port < VTSS_PORT_NO_END; tmp_port++) {
                if (PORT_NO_IS_STACK(tmp_port)) {
                    continue;
                }
                port_no = iport2uport(tmp_port);
                if (req->uport_list[port_no] == 0) {
                    continue;
                }
                rc = qos_port_conf_get(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                CPRINTF("%-4u  %-17s\n", port_no,
                        include_str[port_conf.tx_rate_limiter.header_in_payload]);
            }
        }
    }
    return;
}

static void QOS_cli_cmd_dot3ar_header_size(cli_req_t *req)
{
    qos_cli_req_t   *qos_req = req->module_req;
    qos_conf_t      conf;
    vtss_rc         rc = VTSS_RC_OK;

    if (req->set) {
        rc = qos_conf_get(&conf);
        if (rc != VTSS_RC_OK) {
            return;
        }
        conf.header_size = qos_req->header_size;
        rc = qos_conf_set(&conf);
        if (rc != VTSS_RC_OK) {
            return;
        }
    } else {
        rc = qos_conf_get(&conf);
        if (rc != VTSS_RC_OK) {
            return;
        }
        CPRINTF ("Header Size: %u\n", conf.header_size);
    }

    return;
}

static void QOS_cli_cmd_dot3ar_accumulate(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    vtss_port_no_t  tmp_port, port_no;
    qos_cli_req_t   *qos_req = req->module_req;
    qos_port_conf_t port_conf;
    vtss_rc         rc = VTSS_RC_OK;
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    BOOL            first_flag = FALSE;
#endif


    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }
        if (req->set) {
            for (tmp_port = VTSS_PORT_NO_START; tmp_port < VTSS_PORT_NO_END; tmp_port++) {
                if (PORT_NO_IS_STACK(tmp_port)) {
                    continue;
                }
                port_no = iport2uport(tmp_port);
                if (req->uport_list[port_no] == 0) {
                    continue;
                }
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
                if (vtss_port_is_host(NULL, tmp_port) == TRUE) {
                    if (first_flag == FALSE) {
                        CPRINTF ("DOT3AR is not supported on host interfaces\n");
                        first_flag = TRUE;
                    }
                    continue;
                }
#endif
                rc = qos_port_conf_get(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                if (port_conf.tx_rate_limiter.frame_rate_enable == TRUE) {
                    CPRINTF ("Frame rate mode is enabled\n");
                    return;
                }
                port_conf.tx_rate_limiter.accumulate_mode_enable = qos_req->accumulate_mode_enable;
                rc = qos_port_conf_set(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
            }
        } else {
            CPRINTF ("Port  Accumulate Mode \n");
            CPRINTF ("----  ----------------- \n");
            for (tmp_port = VTSS_PORT_NO_START; tmp_port < VTSS_PORT_NO_END; tmp_port++) {
                if (PORT_NO_IS_STACK(tmp_port)) {
                    continue;
                }
                port_no = iport2uport(tmp_port);
                if (req->uport_list[port_no] == 0) {
                    continue;
                }
                rc = qos_port_conf_get(isid, tmp_port, &port_conf);
                if (rc != VTSS_RC_OK) {
                    continue;
                }
                CPRINTF("%-4u  %-17s\n", port_no,
                        enable_str[port_conf.tx_rate_limiter.accumulate_mode_enable]);
            }
        }
    }

    return;
}
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */

/****************************************************************************/
/*  Parameter parse functions                                               */
/****************************************************************************/
// Specific lists:
static int32_t QOS_cli_parse_policer_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->policer_list, 1, QOS_PORT_POLICER_CNT, 1));

    return error;
}

static int32_t QOS_cli_parse_queue_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->queue_list, 0, QOS_PORT_QUEUE_CNT - 1, 1));

    return error;
}

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
static int32_t QOS_cli_parse_host_weighted_queue_list(char *cmd,
                                                      char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->queue_list, 0, VTSS_PRIOS, 1));

    return error;
}
#endif


#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
static int32_t QOS_cli_parse_weighted_queue_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->queue_list, 0, QOS_PORT_WEIGHTED_QUEUE_CNT - 1, 1));

    return error;
}
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

static int32_t QOS_cli_parse_class_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->class_list, 0, QOS_PORT_PRIO_CNT - 1, 1));

    return error;
}

static int32_t QOS_cli_parse_dpl_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->dpl_list, 0, 1, 1));

    return error;
}

static int32_t QOS_cli_parse_pcp_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->pcp_list, 0, VTSS_PCPS - 1, 1));

    return error;
}

static int32_t QOS_cli_parse_dei_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->dei_list, 0, VTSS_DEIS - 1, 1));

    return error;
}

// Specific parameters:
static int32_t QOS_cli_parse_pcp(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 7);
    qos_req->pcp = value;

    return error;
}

static int32_t QOS_cli_parse_frame_rate(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (mgmt_txt2rate(cmd, &qos_req->packet_rate, 10) != VTSS_OK);
    if (!error) {
        qos_req->packet_rate_spec = CLI_SPEC_VAL;
    }

    return error;
}

static int32_t QOS_cli_parse_bit_rate(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, QOS_BITRATE_MIN, QOS_BITRATE_MAX);
    if (!error) {
        qos_req->bit_rate_spec = CLI_SPEC_VAL;
        qos_req->bit_rate = value;
    }

    return error;
}

static int32_t QOS_cli_parse_dpbl(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 3);
    qos_req->dp_bypass_level = value;

    return error;
}

static int32_t QOS_cli_parse_weight(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 1, 100);
    qos_req->weight = value;

    return error;
}

static int32_t QOS_cli_parse_dei(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 1);
    if (!error) {
        qos_req->dei = value;
        qos_req->dei_valid = 1;
    }

    return error;
}

static int32_t QOS_cli_parse_dpl(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, QOS_DPL_MAX);
    if (!error) {
        qos_req->dpl = value;
        qos_req->dpl_valid = 1;
    }

    return error;
}

#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static int32_t QOS_cli_parse_dpl_array(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 1);
    if (!error && (qos_req->tag_dp_map_cnt < QOS_PORT_TR_DPL_CNT)) {

        if (qos_req->tag_dp_map_cnt && (qos_req->tag_dp_map[qos_req->tag_dp_map_cnt - 1] > value)) {
            error = 1; // New value is lower than the previous one - A zero must not follow a one
        } else {
            int i;
            for (i = qos_req->tag_dp_map_cnt; i < QOS_PORT_TR_DPL_CNT; i++) {
                qos_req->tag_dp_map[i] = value;
            }
            qos_req->tag_dp_map_cnt++;
        }
    }
    return error;
}
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#ifdef VTSS_FEATURE_QOS_WRED
static int32_t QOS_cli_parse_min_th(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 100);
    if (!error) {
        qos_req->min_th = value;
        qos_req->min_th_valid = 1;
    }

    return error;
}

static int32_t QOS_cli_parse_mdp_1(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 100);
    if (!error) {
        qos_req->mdp_1 = value;
        qos_req->mdp_1_valid = 1;
    }

    return error;
}

static int32_t QOS_cli_parse_mdp_2(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 100);
    if (!error) {
        qos_req->mdp_2 = value;
        qos_req->mdp_2_valid = 1;
    }

    return error;
}

static int32_t QOS_cli_parse_mdp_3(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 100);
    if (!error) {
        qos_req->mdp_3 = value;
        qos_req->mdp_3_valid = 1;
    }

    return error;
}
#endif /* VTSS_FEATURE_QOS_WRED */

// Specific keywords:
static int32_t QOS_cli_parse_keyword(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    char *found = cli_parse_find(cmd, stx);
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    if (found != NULL) {
        if (!strncmp(found, "fps", 3)) {
            qos_req->fps = 1;
        } else if (!strncmp(found, "kbps", 4)) {
            qos_req->kbps = 1;
        } else if (!strncmp(found, "weighted", 8)) {
            qos_req->weighted = 1;
        } else if (!strncmp(found, "classified", 10)) {
            qos_req->tag_remark_mode = VTSS_TAG_REMARK_MODE_CLASSIFIED;
        } else if (!strncmp(found, "default", 7)) {
            qos_req->tag_remark_mode = VTSS_TAG_REMARK_MODE_DEFAULT;
        } else if (!strncmp(found, "mapped", 6)) {
            qos_req->tag_remark_mode = VTSS_TAG_REMARK_MODE_MAPPED;
#ifdef VTSS_FEATURE_QCL_V2
        } else if (!strncmp(found, "etype", 5)) {
            qos_req->qce_type = VTSS_QCE_TYPE_ETYPE;
            qos_req->qce_type_spec = CLI_SPEC_VAL;
        } else if (!strncmp(found, "llc", 3)) {
            qos_req->qce_type = VTSS_QCE_TYPE_LLC;
            qos_req->qce_type_spec = CLI_SPEC_VAL;
        } else if (!strncmp(found, "snap", 4)) {
            qos_req->qce_type = VTSS_QCE_TYPE_SNAP;
            qos_req->qce_type_spec = CLI_SPEC_VAL;
        } else if (!strncmp(found, "ipv4", 4)) {
            qos_req->qce_type = VTSS_QCE_TYPE_IPV4;
            qos_req->qce_type_spec = CLI_SPEC_VAL;
        } else if (!strncmp(found, "ipv6", 4)) {
            qos_req->qce_type = VTSS_QCE_TYPE_IPV6;
            qos_req->qce_type_spec = CLI_SPEC_VAL;
#endif /* VTSS_FEATURE_QCL_V2 */
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
        } else if (!strncmp(found, "none", 4)) {
            qos_req->dscp_ingr_class_mode = VTSS_DSCP_MODE_NONE;
        } else if (!strncmp(found, "zero", 4)) {
            qos_req->dscp_ingr_class_mode = VTSS_DSCP_MODE_ZERO;
        } else if (!strncmp(found, "selected", 8)) {
            qos_req->dscp_ingr_class_mode = VTSS_DSCP_MODE_SEL;
        } else if (!strncmp(found, "all", 3)) {
            qos_req->dscp_ingr_class_mode = VTSS_DSCP_MODE_ALL;
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
        }
    }

    return (found == NULL ? 1 : 0);
}

#if defined(VTSS_FEATURE_QCL_V2) || defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static int32_t qos_cli_parse_dscp(char *cmd, ulong *req, ulong min, ulong max)
{
    if (!strncmp(cmd, "be", 2)) {
        *req = QOS_QCE_DSCP_BE;
    } else if (!strncmp(cmd, "cs1", 3)) {
        *req = QOS_QCE_DSCP_CS1;
    } else if (!strncmp(cmd, "cs2", 3)) {
        *req = QOS_QCE_DSCP_CS2;
    } else if (!strncmp(cmd, "cs3", 3)) {
        *req = QOS_QCE_DSCP_CS3;
    } else if (!strncmp(cmd, "cs4", 3)) {
        *req = QOS_QCE_DSCP_CS4;
    } else if (!strncmp(cmd, "cs5", 3)) {
        *req = QOS_QCE_DSCP_CS5;
    } else if (!strncmp(cmd, "cs6", 3)) {
        *req = QOS_QCE_DSCP_CS6;
    } else if (!strncmp(cmd, "cs7", 3)) {
        *req = QOS_QCE_DSCP_CS7;
    } else if (!strncmp(cmd, "ef", 2)) {
        *req = QOS_QCE_DSCP_EF;
    } else if (!strncmp(cmd, "af11", 4)) {
        *req = QOS_QCE_DSCP_AF11;
    } else if (!strncmp(cmd, "af12", 4)) {
        *req = QOS_QCE_DSCP_AF12;
    } else if (!strncmp(cmd, "af13", 4)) {
        *req = QOS_QCE_DSCP_AF13;
    } else if (!strncmp(cmd, "af21", 4)) {
        *req = QOS_QCE_DSCP_AF21;
    } else if (!strncmp(cmd, "af22", 4)) {
        *req = QOS_QCE_DSCP_AF22;
    } else if (!strncmp(cmd, "af23", 4)) {
        *req = QOS_QCE_DSCP_AF23;
    } else if (!strncmp(cmd, "af31", 4)) {
        *req = QOS_QCE_DSCP_AF31;
    } else if (!strncmp(cmd, "af32", 4)) {
        *req = QOS_QCE_DSCP_AF32;
    } else if (!strncmp(cmd, "af33", 4)) {
        *req = QOS_QCE_DSCP_AF33;
    } else if (!strncmp(cmd, "af41", 4)) {
        *req = QOS_QCE_DSCP_AF41;
    } else if (!strncmp(cmd, "af42", 4)) {
        *req = QOS_QCE_DSCP_AF42;
    } else if (!strncmp(cmd, "af43", 4)) {
        *req = QOS_QCE_DSCP_AF43;
    } else {
        return cli_parse_ulong(cmd, req, min, max);
    }

    return VTSS_OK;
}
#endif /* defined(VTSS_FEATURE_QCL_V2) || defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

#ifdef VTSS_FEATURE_QCL_V2
#if VTSS_SWITCH_STACKABLE
static int32_t QOS_cli_parse_qce_sid(char *cmd, char *cmd2,
                                     char *stx, char *cmd_org, cli_req_t *req)
{
    int           error;
    ulong         value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    error = cli_parse_ulong(cmd, &value, VTSS_USID_START, VTSS_USID_END - 1);
    if (!error) {
        qos_req->usid_qce_spec = CLI_SPEC_VAL;
        qos_req->usid_qce = value;
    } else {
        error = cli_parse_wc(cmd, &qos_req->usid_qce_spec);
    }
    return error;
}
#endif /* VTSS_SWITCH_STACKABLE */

static int32_t QOS_cli_parse_qce_id(char *cmd, char *cmd2,
                                    char *stx, char *cmd_org, cli_req_t *req)
{
    qos_cli_req_t *qos_req = req->module_req;
    return (cli_parse_ulong(cmd, &qos_req->qce_id, QCE_ID_START, QCE_ID_END));
}

static int32_t QOS_cli_parse_qce_id_next(char *cmd, char *cmd2,
                                         char *stx, char *cmd_org, cli_req_t *req)
{
    qos_cli_req_t *qos_req = req->module_req;
    int           error;

    if (!(error = cli_parse_word(cmd, "next_id"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        if (!(error = cli_parse_word(cmd2, "last"))) {
            qos_req->qce_id_next = QCE_ID_END + 1;
        } else {
            error = cli_parse_ulong(cmd2, &qos_req->qce_id_next, QCE_ID_START, QCE_ID_END);
        }
    }

    return error;
}

static int32_t QOS_cli_parse_qce_port_list(char *cmd, char *cmd2,
                                           char *stx, char *cmd_org, cli_req_t *req)
{
    qos_cli_req_t *qos_req = req->module_req;
    int      error;
    ulong    min = 1, max = VTSS_PORTS;

    if (!(error = cli_parse_word(cmd, "port"))) {
        if (cmd2 == NULL) {
            return 1;
        }

        req->parm_parsed = 2;
        qos_req->port_spec = CLI_SPEC_VAL;
        error = (cli_parse_all(cmd2) &&
                 cli_parm_parse_list(cmd2, req->uport_list, min, max, 1));
    }

    return error;
}

static int32_t QOS_cli_parse_qce_tag(char *cmd, char *cmd2,
                                     char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    qos_cli_req_t *qos_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "any"))) {
        qos_req->tag = VTSS_VCAP_BIT_ANY;
        qos_req->tag_spec = CLI_SPEC_ANY;
    } else if (!(error = cli_parse_word(cmd, "untag"))) {
        qos_req->tag = VTSS_VCAP_BIT_0;
        qos_req->tag_spec = CLI_SPEC_VAL;
    } else if (!(error = cli_parse_word(cmd, "tag"))) {
        qos_req->tag = VTSS_VCAP_BIT_1;
        qos_req->tag_spec = CLI_SPEC_VAL;
    }

    return error;
}

static int32_t QOS_cli_parse_qce_vid(char *cmd, char *cmd2,
                                     char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    ulong          min, max;
    qos_cli_req_t *qos_req = req->module_req;

    error = cli_parse_range(cmd, &min, &max, VTSS_VID_DEFAULT, VTSS_VID_RESERVED);
    if (error) {
        error = cli_parse_wc(cmd, &qos_req->vid_spec);
    } else {
        qos_req->vid_spec = CLI_SPEC_VAL;
        qos_req->vid_min = min;
        qos_req->vid_max = max;
    }

    return error;
}

static int32_t QOS_cli_parse_qce_pcp(char *cmd, char *cmd2,
                                     char *stx, char *cmd_org, cli_req_t *req)
{
    int           error;
    ulong         low, high;
    qos_cli_req_t *qos_req = req->module_req;

    /* check for 'any' */
    error = cli_parse_wc(cmd, &qos_req->qce_pcp_spec);
    if (error) {
        /* check for specific value */
        error = cli_parse_ulong(cmd, &low, VTSS_PCP_START, VTSS_PCP_END - 1);
        if (error) {
            /* check for range */
            error = cli_parse_range(cmd, &low, &high, VTSS_PCP_START, VTSS_PCP_END - 1);
            if (!error) {
                if ((low == 0 && high == 1) || (low == 2 && high == 3) ||
                    (low == 4 && high == 5) || (low == 6 && high == 7) ||
                    (low == 0 && high == 3) || (low == 4 && high == 7)) {
                    qos_req->qce_pcp_low = low;
                    qos_req->qce_pcp_high = high;
                    qos_req->qce_pcp_spec = CLI_SPEC_VAL;
                } else {
                    error = VTSS_INVALID_PARAMETER;
                }
            }
        } else {
            qos_req->qce_pcp_low = low;
            qos_req->qce_pcp_high = 0;
            qos_req->qce_pcp_spec = CLI_SPEC_VAL;
        }
    } else {
        qos_req->qce_pcp_low = 0;
        qos_req->qce_pcp_high = 0;
    }

    return error;
}

static int32_t QOS_cli_parse_qce_dei(char *cmd, char *cmd2,
                                     char *stx, char *cmd_org, cli_req_t *req)
{
    int           error;
    ulong         value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    error = cli_parse_ulong(cmd, &value, VTSS_DEI_START, VTSS_DEI_END - 1);
    if (error) {
        error = cli_parse_wc(cmd, &qos_req->qce_dei_spec);
    } else {
        qos_req->qce_dei_spec = CLI_SPEC_VAL;
        qos_req->qce_dei = value;
    }


    if (!error) {
        if (qos_req->qce_dei_spec == CLI_SPEC_VAL) {
            qos_req->qce_dei = ((value == 0) ? VTSS_VCAP_BIT_0 : VTSS_VCAP_BIT_1);
        } else {
            qos_req->qce_dei = VTSS_VCAP_BIT_ANY;
        }
    }

    return(error);
}

/* MAC address of the form 00-11-22 i.e. the 24 most significant bits (OUI) */
static int32_t QOS_cli_parse_qce_smac(char *cmd, char *cmd2,
                                      char *stx, char *cmd_org, cli_req_t *req)
{
    qos_cli_req_t *qos_req = req->module_req;
    uint          i, mac[3];
    int           error = 1;

    if (sscanf(cmd, "%2x-%2x-%2x", &mac[0], &mac[1], &mac[2]) == 3) {
        for (i = 0; i < 3; i++) {
            qos_req->smac[i] = (mac[i] & 0xff);
        }
        qos_req->smac_spec = CLI_SPEC_VAL;
        error = 0;
    } else {
        error = cli_parse_wc(cmd, &qos_req->smac_spec);
    }
    return error;
}

static int32_t QOS_cli_parse_qce_dmac_type(char *cmd, char *cmd2,
                                           char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    qos_cli_req_t *qos_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "any"))) {
        qos_req->dmac_type = QOS_QCE_DMAC_TYPE_ANY;
        qos_req->dmac_type_spec = CLI_SPEC_ANY;
    } else  if (!(error = cli_parse_word(cmd, "unicast"))) {
        qos_req->dmac_type = QOS_QCE_DMAC_TYPE_UC;
        qos_req->dmac_type_spec = CLI_SPEC_VAL;
    } else  if (!(error = cli_parse_word(cmd, "multicast"))) {
        qos_req->dmac_type = QOS_QCE_DMAC_TYPE_MC;
        qos_req->dmac_type_spec = CLI_SPEC_VAL;
    } else  if (!(error = cli_parse_word(cmd, "broadcast"))) {
        qos_req->dmac_type = QOS_QCE_DMAC_TYPE_BC;
        qos_req->dmac_type_spec = CLI_SPEC_VAL;
    }

    return error;
}

static int32_t QOS_cli_parse_qce_etype(char *cmd, char *cmd2,
                                       char *stx, char *cmd_org, cli_req_t *req)
{
    int error;
    ulong value;
    qos_cli_req_t *qos_req = req->module_req;

    error = cli_parse_ulong_wc(cmd, &value, 0x600, 0xFFFF, &qos_req->etype_spec);
    if (!error && qos_req->etype_spec == CLI_SPEC_VAL) {
        if (value == 0x800 || value == 0x86DD) {
            CPRINTF("Ether Type 0x0800(IPv4) and 0x86DD(IPv6) are reserved\n");
            return(VTSS_UNSPECIFIED_ERROR);
        }
        qos_req->etype = (vtss_etype_t)value;
    } else if (!error && qos_req->etype_spec == CLI_SPEC_ANY) {
        qos_req->etype = (vtss_etype_t)0;
    }

    return(error);
}

static int32_t QOS_cli_parse_qce_llc_dsap(char *cmd, char *cmd2,
                                          char *stx, char *cmd_org, cli_req_t *req)
{
    int error;
    ulong value;
    qos_cli_req_t *qos_req = req->module_req;

    error = cli_parse_ulong_wc(cmd, &value, 0x0, 0xFF, &qos_req->dsap_spec);
    if (!error && qos_req->dsap_spec == CLI_SPEC_VAL) {
        qos_req->dsap = (uchar)value;
    }

    return(error);
}

static int32_t QOS_cli_parse_qce_llc_ssap(char *cmd, char *cmd2,
                                          char *stx, char *cmd_org, cli_req_t *req)
{
    int error;
    ulong value;
    qos_cli_req_t *qos_req = req->module_req;

    error = cli_parse_ulong_wc(cmd, &value, 0x0, 0xFF, &qos_req->ssap_spec);
    if (!error && qos_req->ssap_spec == CLI_SPEC_VAL) {
        qos_req->ssap = (uchar)value;
    }

    return(error);
}

static int32_t QOS_cli_parse_qce_llc_control(char *cmd, char *cmd2,
                                             char *stx, char *cmd_org, cli_req_t *req)
{
    int error;
    ulong value;
    qos_cli_req_t *qos_req = req->module_req;

    error = cli_parse_ulong_wc(cmd, &value, 0x0, 0xFF, &qos_req->control_spec);
    if (!error && qos_req->control_spec == CLI_SPEC_VAL) {
        qos_req->control = (uchar)value;
    }

    return(error);
}

static int32_t QOS_cli_parse_qce_snap_pid(char *cmd, char *cmd2,
                                          char *stx, char *cmd_org, cli_req_t *req)
{
    int error;
    ulong value;
    qos_cli_req_t *qos_req = req->module_req;

    error = cli_parse_ulong_wc(cmd, &value, 0x0, 0xFFFF, &qos_req->snap_pid_spec);
    if (!error && qos_req->snap_pid_spec == CLI_SPEC_VAL) {
        qos_req->snap_pid = (u16)value;
    }

    return(error);
}


static int32_t QOS_cli_parse_qce_protocol(char *cmd, char *cmd2,
                                          char *stx, char *cmd_org, cli_req_t *req)
{
    int error;
    ulong value;
    qos_cli_req_t *qos_req = req->module_req;

    if (!strncmp(cmd, "tcp", 3)) {
        qos_req->protocol = 6;
        qos_req->protocol_spec = CLI_SPEC_VAL;
        return VTSS_OK;
    } else if (!strncmp(cmd, "udp", 3)) {
        qos_req->protocol = 17;
        qos_req->protocol_spec = CLI_SPEC_VAL;
        return VTSS_OK;
    }
    error = cli_parse_ulong_wc(cmd, &value, 0x0, 0xFF, &qos_req->protocol_spec);
    if (!error && qos_req->protocol_spec == CLI_SPEC_VAL) {
        if (value == 6 || value == 17) {
            CPRINTF("select TCP or UDP instead of 6 and 17\n");
            return(VTSS_UNSPECIFIED_ERROR);
        }
        qos_req->protocol = (uchar)value;
    }

    return(error);
}

static int32_t QOS_cli_parse_qce_sip(char *cmd, char *cmd2,
                                     char *stx, char *cmd_org, cli_req_t *req)
{
    qos_cli_req_t *qos_req = req->module_req;
    return(cli_parse_ipv4(cmd, &qos_req->sip, &qos_req->sip_mask, &qos_req->sip_spec, 0));
}

#if 0
static int32_t QOS_cli_parse_qce_sipv6(char *cmd, char *cmd2,
                                       char *stx, char *cmd_org, cli_req_t *req)
{
    qos_cli_req_t *qos_req = req->module_req;
    int error;

    error = cli_parse_wc(cmd, &qos_req->sipv6_spec);
    if (error) {
        error = cli_parse_ipv6(cmd, &qos_req->sipv6, &qos_req->sipv6_spec);
    }

    return error;
}
#endif


static int32_t QOS_cli_parse_qce_dscp(char *cmd, char *cmd2,
                                      char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    ulong          min, max;
    qos_cli_req_t *qos_req = req->module_req;
    char *cptr1, *cptr2;

    cptr1 = strtok(cmd, "-");
    cptr2 = strtok(NULL, "-");
    error = qos_cli_parse_dscp(cptr1, &min, 0, 63);
    if (!error) {
        /* user has typed fixed value, not 'any' */
        if (cptr2) {
            /* user has entered a range of DSCP */
            error = qos_cli_parse_dscp(cptr2, &max, 0, 63);
            if (!error && (min < max)) {
                qos_req->dscp_spec = CLI_SPEC_VAL;
                qos_req->dscp_min = min;
                qos_req->dscp_max = max;
            } else {
                error = VTSS_UNSPECIFIED_ERROR;
            }
        } else {
            /* only single DSCP value entered */
            qos_req->dscp_spec = CLI_SPEC_VAL;
            qos_req->dscp_min = min;
            qos_req->dscp_max = min;
        }
    } else if (cptr2 == NULL) {
        /* if user has typed 'any' */
        error = cli_parse_wc(cmd, &qos_req->dscp_spec);
    }

    return error;
}

static int32_t QOS_cli_parse_qce_ipv4_fragment(char *cmd, char *cmd2,
                                               char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    qos_cli_req_t *qos_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "any"))) {
        qos_req->ipv4_fragment = VTSS_VCAP_BIT_ANY;
        qos_req->ipv4_fragment_spec = CLI_SPEC_ANY;
    } else  if (!(error = cli_parse_word(cmd, "yes"))) {
        qos_req->ipv4_fragment = VTSS_VCAP_BIT_1;
        qos_req->ipv4_fragment_spec = CLI_SPEC_VAL;
    } else  if (!(error = cli_parse_word(cmd, "no"))) {
        qos_req->ipv4_fragment = VTSS_VCAP_BIT_0;
        qos_req->ipv4_fragment_spec = CLI_SPEC_VAL;
    }

    return error;
}

static int32_t QOS_cli_parse_qce_sport(char *cmd, char *cmd2,
                                       char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    ulong          min, max;
    qos_cli_req_t *qos_req = req->module_req;

    if ((qos_req->protocol != 6) && (qos_req->protocol != 17)) {
        return VTSS_INVALID_PARAMETER;
    }

    error = cli_parse_range(cmd, &min, &max, 0, 0xFFFF);
    if (error) {
        error = cli_parse_wc(cmd, &qos_req->sport_spec);
    } else {
        qos_req->sport_spec = CLI_SPEC_VAL;
        qos_req->sport_min = min;
        qos_req->sport_max = max;
    }
    return error;
}

static int32_t QOS_cli_parse_qce_dport(char *cmd, char *cmd2,
                                       char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    ulong          min, max;
    qos_cli_req_t *qos_req = req->module_req;

    if ((qos_req->protocol != 6) && (qos_req->protocol != 17)) {
        return VTSS_INVALID_PARAMETER;
    }

    error = cli_parse_range(cmd, &min, &max, 0, 0xFFFF);
    if (error) {
        error = cli_parse_wc(cmd, &qos_req->dport_spec);
    } else {
        qos_req->dport_spec = CLI_SPEC_VAL;
        qos_req->dport_min = min;
        qos_req->dport_max = max;
    }
    return error;
}

static int32_t QOS_cli_parse_qce_class(char *cmd, char *cmd2,
                                       char *stx, char *cmd_org, cli_req_t *req)
{
    int           error;
    ulong         value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "class"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        error = cli_parse_ulong(cmd2, &value, 0, QOS_CLASS_CNT - 1);
        if (!error) {
            qos_req->action_class_spec = CLI_SPEC_VAL;
            qos_req->action_class = value;
        }
    }

    return error;
}

static int32_t QOS_cli_parse_qce_dp(char *cmd, char *cmd2,
                                    char *stx, char *cmd_org, cli_req_t *req)
{
    int           error;
    ulong         value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "dp"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        error = cli_parse_ulong(cmd2, &value, 0, QOS_DPL_MAX);
        if (!error) {
            qos_req->action_dp_spec = CLI_SPEC_VAL;
            qos_req->action_dp = value;
        }
    }
    return error;
}

static int32_t QOS_cli_parse_qce_dscp_classified(char *cmd, char *cmd2,
                                                 char *stx, char *cmd_org, cli_req_t *req)
{
    int           error;
    ulong         value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "dscp"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        error = qos_cli_parse_dscp(cmd2, &value, 0, 63);
        if (!error) {
            qos_req->action_dscp_spec = CLI_SPEC_VAL;
            qos_req->action_dscp = value;
        }
    }
    return error;
}

static int32_t QOS_cli_parse_qce_user(char *cmd, char *cmd2,
                                      char *stx, char *cmd_org, cli_req_t *req)
{
    char          *found = cli_parse_find(cmd, stx);
    qos_cli_req_t *qos_req = req->module_req;

    if (found != NULL) {
        if (!strncmp(found, "combined", 8)) {
            qos_req->qcl_user = -1;
            qos_req->qcl_user_valid = 1;
        } else if (!strncmp(found, "static", 6)) {
            qos_req->qcl_user = QCL_USER_STATIC;
            qos_req->qcl_user_valid = 1;
        }
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        else if (!strncmp(found, "voice_vlan", 10)) {
            qos_req->qcl_user = QCL_USER_VOICE_VLAN;
            qos_req->qcl_user_valid = 1;
        }
#endif
        else if (!strncmp(found, "conflict", 8)) {
            qos_req->qcl_user = -2;
            qos_req->qcl_user_valid = 1;
        }
    }

    return (found == NULL ? 1 : 0);
}
#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static int32_t QOS_cli_parse_dscp_list(char *cmd, char *cmd2,
                                       char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->dscp_list, 0, 63, 1));

    return error;
}

static int32_t QOS_cli_parse_dscp_val(char *cmd, char *cmd2, char *stx,
                                      char *cmd_org, cli_req_t *req)
{
    int           error;
    ulong         value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    error = qos_cli_parse_dscp(cmd, &value, 0, 63);
    if (!error) {
        qos_req->dscp = value;
    }
    return error;
}

static int32_t QOS_cli_parse_dscp_remark_mode(char *cmd, char *cmd2, char *stx,
                                              char *cmd_org, cli_req_t *req)
{
    char *found = cli_parse_find(cmd, stx);
    qos_cli_req_t *qos_req = req->module_req;

    if (found != NULL) {
        if (!strncmp(found, "enable", 6)) {
            qos_req->dscp_remark_mode = VTSS_DSCP_EMODE_REMARK;
        } else if (!strncmp(found, "disable", 7)) {
            qos_req->dscp_remark_mode = VTSS_DSCP_EMODE_DISABLE;
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
        } else if (!strncmp(found, "remap_dp_unaware", 16)) {
            qos_req->dscp_remark_mode = VTSS_DSCP_EMODE_REMAP;
        } else if (!strncmp(found, "remap_dp_aware", 14)) {
            qos_req->dscp_remark_mode = VTSS_DSCP_EMODE_REMAP_DPA;
#else
        } else if (!strncmp(found, "remap", 5)) {
            qos_req->dscp_remark_mode = VTSS_DSCP_EMODE_REMAP;
#endif
        }
    }

    return (found == NULL ? 1 : 0);
}
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
static int32_t QOS_cli_parse_lport_list(char *cmd, char *cmd2,
                                        char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    error = (cli_parse_all(cmd) && cli_parm_parse_list(cmd, qos_req->lport_list, 0, 48, 1));

    return error;
}
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)

static int32_t QOS_cli_parse_dot3ar_mode_conf(char *cmd, char *cmd2,
                                              char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t       error = 0;
    qos_cli_req_t *qos_req = req->module_req;
    BOOL          mode_conf = FALSE;

    req->parm_parsed = 1;

    if (!(error = cli_parse_word(cmd, "enable"))) {
        mode_conf = TRUE;
    } else if (!(error = cli_parse_word(cmd, "disable"))) {
        mode_conf = FALSE;
    } else {
        error = -1;
        return error;
    }
    switch (qos_req->mode_sel) {
    case 1:
        qos_req->frame_overhead_enable = mode_conf;
        break;
    case 2:
        qos_req->payload_rate_enable = mode_conf;
        break;
    case 4:
        qos_req->frame_rate_enable = mode_conf;
        break;
    default:
        break;
    }
    return error;
}
static int32_t QOS_cli_parse_dot3ar_rate_conf(char *cmd, char *cmd2,
                                              char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t       error = 0;
    qos_cli_req_t *qos_req = req->module_req;
    u32           value;

    req->parm_parsed = 1;

    error = cli_parse_ulong(cmd, &value, 0, 261120);
    if (error == -1) {
        return error;
    }
    switch (qos_req->mode_sel) {
    case 1:
        if (value < 12 || value > 261120) {
            error = -1;
            break;
        }
        qos_req->frame_overhead = value;
        break;
    case 2:
        if (value > 100) {
            error = -1;
            break;
        }
        qos_req->payload_rate = value;
        break;
    case 4:
        if (value < 12 || value > 16383) {
            error = -1;
            break;
        }
        qos_req->frame_rate = value;
        break;
    default:
        error = -1;
        break;
    }
    return error;
}
static int32_t QOS_cli_parse_dot3ar_mode_sel(char *cmd, char *cmd2,
                                             char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;


    if (!(error = cli_parse_word(cmd, "frame-overhead"))) {
        req->parm_parsed = 1;
        qos_req->mode_sel = 1;
    } else if (!(error = cli_parse_word(cmd, "payload-rate-limiter"))) {
        req->parm_parsed = 1;
        qos_req->mode_sel = 2;
    } else if (!(error = cli_parse_word(cmd, "frame-rate-limiter"))) {
        req->parm_parsed = 1;
        qos_req->mode_sel = 4;
    } else {
        error = -1;
    }

    return error;
}

static int32_t QOS_cli_parse_dot3ar_preamble(char *cmd, char *cmd2,
                                             char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t       error = 0;
    qos_cli_req_t *qos_req = req->module_req;
    BOOL          preamble_conf = FALSE;

    req->parm_parsed = 1;

    if (!(error = cli_parse_word(cmd, "include"))) {
        preamble_conf = TRUE;
    } else if (!(error = cli_parse_word(cmd, "exclude"))) {
        preamble_conf = FALSE;
    } else {
        error = -1;
        return error;
    }
    qos_req->preamble_in_payload = preamble_conf;

    return error;
}
static int32_t QOS_cli_parse_dot3ar_header(char *cmd, char *cmd2,
                                           char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t       error = 0;
    qos_cli_req_t *qos_req = req->module_req;
    BOOL          header_conf = FALSE;

    req->parm_parsed = 1;

    if (!(error = cli_parse_word(cmd, "include"))) {
        header_conf = TRUE;
    } else if (!(error = cli_parse_word(cmd, "exclude"))) {
        header_conf = FALSE;
    } else {
        error = -1;
        return error;
    }
    qos_req->header_in_payload = header_conf;

    return error;
}

static int32_t QOS_cli_parse_dot3ar_header_size(char *cmd, char *cmd2,
                                                char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t       error = 0;
    qos_cli_req_t *qos_req = req->module_req;
    u32           value;

    req->parm_parsed = 1;

    error = cli_parse_ulong(cmd, &value, 1, 31);
    if (error == -1) {
        return error;
    }
    qos_req->header_size = (value) & 0x1f;

    return error;
}
static int32_t QOS_cli_parse_dot3ar_accumulate(char *cmd, char *cmd2,
                                               char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t       error = 0;
    qos_cli_req_t *qos_req = req->module_req;
    BOOL          accumulate_conf = FALSE;

    req->parm_parsed = 1;

    if (!(error = cli_parse_word(cmd, "enable"))) {
        accumulate_conf = TRUE;
    } else if (!(error = cli_parse_word(cmd, "disable"))) {
        accumulate_conf = FALSE;
    } else {
        error = -1;
        return error;
    }
    qos_req->accumulate_mode_enable = accumulate_conf;

    return error;
}
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */








/****************************************************************************/
/*  Parameter table                                                         */
/****************************************************************************/

static cli_parm_t qos_cli_parm_table[] = {
// Specific lists:
    {
        "<policer_list>",
        "Policer list or 'all', default: All policers (1-4)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_policer_list,
        NULL
    },
#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
    {
        "<queue_list>",
        "Weighted queue list or 'all', default: All weighted queues (0-5)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_weighted_queue_list,
        QOS_cli_cmd_port_scheduler_weight
    },
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */
#ifdef VTSS_FEATURE_QOS_WRED
    {
        "<queue_list>",
        "WRED queue list or 'all', default: All WRED queues (0-5)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_weighted_queue_list,
        QOS_cli_cmd_wred
    },
    {
        "<min_th>",
        "Minimum threshold (0-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_min_th,
        QOS_cli_cmd_wred
    },
    {
        "<mdp_1>",
        "Maximum Drop Probability for DP level 1 (0-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_mdp_1,
        QOS_cli_cmd_wred
    },
    {
        "<mdp_2>",
        "Maximum Drop Probability for DP level 2 (0-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_mdp_2,
        QOS_cli_cmd_wred
    },
    {
        "<mdp_3>",
        "Maximum Drop Probability for DP level 3 (0-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_mdp_3,
        QOS_cli_cmd_wred
    },
#endif /* VTSS_FEATURE_QOS_WRED */

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
    {
        "enable|disable",
        "enable     : Enable storm policing of unicast frames\n"
        "disable    : Disable storm policing of unicast frames",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_storm_unicast
    },
    {
        "enable|disable",
        "enable     : Enable storm policing of broadcast frames\n"
        "disable    : Disable storm policing of broadcast frames",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_storm_broadcast
    },
    {
        "enable|disable",
        "enable     : Enable storm policing of unknown frames\n"
        "disable    : Disable storm policing of unknown frames",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_storm_unknown
    },
    {
        "<rate>",
        "Rate in kbps or fps ("vtss_xstr(QOS_BITRATE_MIN)"-"vtss_xstr(QOS_BITRATE_MAX)")",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_bit_rate,
        QOS_cli_cmd_port_storm_unicast
    },
    {
        "<rate>",
        "Rate in kbps or fps ("vtss_xstr(QOS_BITRATE_MIN)"-"vtss_xstr(QOS_BITRATE_MAX)")",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_bit_rate,
        QOS_cli_cmd_port_storm_broadcast
    },
    {
        "<rate>",
        "Rate in kbps or fps ("vtss_xstr(QOS_BITRATE_MIN)"-"vtss_xstr(QOS_BITRATE_MAX)")",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_bit_rate,
        QOS_cli_cmd_port_storm_unknown
    },
    {
        "kbps|fps",
        "kbps       : Unit is kilo bits per second\n"
        "fps        : Unit is frames per second",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_port_storm_unicast
    },
    {
        "kbps|fps",
        "kbps       : Unit is kilo bits per second\n"
        "fps        : Unit is frames per second",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_port_storm_broadcast
    },
    {
        "kbps|fps",
        "kbps       : Unit is kilo bits per second\n"
        "fps        : Unit is frames per second",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_port_storm_unknown
    },
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

#ifdef VTSS_FEATURE_QCL_V2
#if VTSS_SWITCH_STACKABLE
    {
        "<sid>",
        QOS_cli_sid_help_str,
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_sid,
        QOS_cli_cmd_qcl_add
    },
    {
        "<qce_id>",
        "QCE ID (1-"vtss_xstr(VTSS_ISID_CNT)"*"vtss_xstr(QCE_MAX)"), default: Next available ID",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_id,
        NULL
    },
    {
        "<qce_id_next>",
        "Next QCE ID: \"next_id (1-"vtss_xstr(VTSS_ISID_CNT)"*"vtss_xstr(QCE_MAX)") or 'last'\"",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_id_next,
        QOS_cli_cmd_qcl_add
    },
#else /* VTSS_SWITCH_STACKABLE */
    {
        "<qce_id>",
        "QCE ID (1-"vtss_xstr(QCE_MAX)"), default: Next available ID",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_id,
        NULL
    },
    {
        "<qce_id_next>",
        "Next QCE ID: \"next_id (1-"vtss_xstr(QCE_MAX)") or 'last'\"",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_id_next,
        QOS_cli_cmd_qcl_add
    },
#endif /* VTSS_SWITCH_STACKABLE */
    {
        "<port_list>",
        "Port List: \"port <port_list> or 'all'\", default: All ports",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_qce_port_list,
        QOS_cli_cmd_qcl_add
    },
    {
        "<tag>",
        "Frame tag: untag|tag|any",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_tag,
        QOS_cli_cmd_qcl_add
    },
    {
        "<vid>",
        "VID: 1-4095 or 'any', either a specific VID or range of VIDs",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_vid,
        QOS_cli_cmd_qcl_add
    },
    {
        "<pcp>",
        "Priority Code Point: specific(0, 1, 2, 3, 4, 5, 6, 7) or\n"
        "                   range(0-1, 2-3, 4-5, 6-7, 0-3, 4-7) or 'any'",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_pcp,
        QOS_cli_cmd_qcl_add
    },
    {
        "<dei>",
        "Drop Eligible Indicator: 0-1 or 'any'",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_dei,
        QOS_cli_cmd_qcl_add
    },
    {
        "<smac>",
        "Source MAC address: (xx-xx-xx) or 'any', 24 MS bits (OUI)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_smac,
        QOS_cli_cmd_qcl_add
    },
    {
        "<dmac_type>",
        "Destination MAC type: unicast|multicast|broadcast|any",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_dmac_type,
        QOS_cli_cmd_qcl_add
    },
    {
        "etype",
        "Ethernet Type keyword",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_qcl_add
    },
    {
        "<etype>",
        "Ethernet Type: 0x600-0xFFFF or 'any' but excluding 0x800(IPv4) and 0x86DD(IPv6)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_etype,
        QOS_cli_cmd_qcl_add
    },
    {
        "llc",
        "LLC keyword",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_qcl_add
    },
    {
        "<dsap>",
        "Destination Service Access Point: 0x00-0xFF or 'any'",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_llc_dsap,
        QOS_cli_cmd_qcl_add
    },
    {
        "<ssap>",
        "Source Service Access Point: 0x00-0xFF or 'any'",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_llc_ssap,
        QOS_cli_cmd_qcl_add
    },
    {
        "<control>",
        "LLC control: 0x00-0xFF or 'any'",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_llc_control,
        QOS_cli_cmd_qcl_add
    },
    {
        "snap",
        "SNAP keyword",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_qcl_add
    },
    {
        "<pid>",
        "Protocol ID (EtherType) or 'any'",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_snap_pid,
        QOS_cli_cmd_qcl_add
    },
    {
        "ipv4",
        "IPv4 keyowrd",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_qcl_add
    },
    {
        "ipv6",
        "IPv6 keyowrd",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_qcl_add
    },
    {
        "<protocol>",
        "IP protocol number: (0-255, TCP or UDP) or 'any'",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_protocol,
        QOS_cli_cmd_qcl_add
    },
    {
        "<sip>",
        "Source IP address: (a.b.c.d/n) or 'any'",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_sip,
        QOS_cli_cmd_qcl_add
    },
    {
        "<sip_v6>",
        "IPv6 source address: (a.b.c.d/n) or 'any', 32 LS bits",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_sip,
        QOS_cli_cmd_qcl_add
    },
    {
        "<dscp>",
        "DSCP: (0-63,BE,CS1-CS7,EF or AF11-AF43) or 'any', specific or range",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_dscp,
        QOS_cli_cmd_qcl_add
    },
    {
        "<fragment>",
        "IPv4 frame fragmented: yes|no|any",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_ipv4_fragment,
        QOS_cli_cmd_qcl_add
    },
    {
        "<sport>",
        "Source TCP/UDP port:(0-65535) or 'any', specific or port range",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_sport,
        QOS_cli_cmd_qcl_add
    },
    {
        "<dport>",
        "Dest. TCP/UDP port:(0-65535) or 'any', specific or port range",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_dport,
        QOS_cli_cmd_qcl_add
    },
    {
        "<class>",
        "QoS Class: \"class (0-7)\", default: basic classification",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_class,
        QOS_cli_cmd_qcl_add
    },
    {
        "<dp>",
        "DP Level: \"dp (0-"vtss_xstr(QOS_DPL_MAX)")\", default: basic classification",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_dp,
        QOS_cli_cmd_qcl_add
    },
    {
        "<classified_dscp>",
        "DSCP: \"dscp (0-63, BE, CS1-CS7, EF or AF11-AF43)\"",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_dscp_classified,
        QOS_cli_cmd_qcl_add
    },
    {
        "combined|static"
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "|voice_vlan"
#endif
        "|conflicts",
        "combined        : Shows the combined status\n"
        "static          : Shows the static user configured status\n"
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        "voice_vlan      : Shows the status by Voice VLAN\n"
#endif
        "conflicts       : Shows all conflict status\n"
        "(default        : Shows the combined status)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_user,
        QOS_cli_cmd_qcl_status
    },
#endif /* VTSS_FEATURE_QCL_V2 */
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    {
        "<queue_list>",
        "Strict queue list or 'all', default: Depending on the host mode (6-7) or (2-3)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_queue_list,
        QOS_cli_cmd_lport_queue_shaper_mode
    },
    {
        "<queue_list>",
        "Strict queue list or 'all', default: Depending on the host mode (6-7) or (2-3)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_queue_list,
        QOS_cli_cmd_lport_queue_shaper_rate
    },
    {
        "<queue_list>",
        "Weighted queue list or 'all', default: Depending on the host mode (0-5) or (0-1)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_queue_list,
        QOS_cli_cmd_lport_scheduler_weight
    },
    {
        "<queue_list>",
        "WRED queue list or 'all', default: Depending on the host mode (0-7) or (0-3)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_host_weighted_queue_list,
        QOS_cli_cmd_host_wred
    },
    {
        "enable|disable",
        "enable      : Enable WRED\n"
        "disable     : Disable WRED",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_host_wred
    },
    {
        "<min_th>",
        "Minimum threshold (0-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_min_th,
        QOS_cli_cmd_host_wred
    },
    {
        "<mdp_1>",
        "Maximum Drop Probability for DP level 1 (0-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_mdp_1,
        QOS_cli_cmd_host_wred
    },
    {
        "<mdp_2>",
        "Maximum Drop Probability for DP level 2 (0-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_mdp_2,
        QOS_cli_cmd_host_wred
    },
    {
        "<mdp_3>",
        "Maximum Drop Probability for DP level 3 (0-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_mdp_3,
        QOS_cli_cmd_host_wred
    },
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */


    {
        "<queue_list>",
        "Queue list or 'all', default: All queues (0-7)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_queue_list,
        NULL
    },
    {
        "<class_list>",
        "QoS class list or 'all', default: All QoS classes (0-7)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_class_list,
        NULL
    },
    {
        "<dpl_list>",
        "DP level list or 'all', default: All DP levels (0-1)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_dpl_list,
        NULL
    },
    {
        "<pcp_list>",
        "PCP list or 'all', default: All PCPs (0-7)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_pcp_list,
        NULL
    },
    {
        "<dei_list>",
        "DEI list or 'all', default: All DEIs (0-1)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_dei_list,
        NULL
    },
// Specific parameters:
    {
        "<class>",
        "QoS class (0-7)",
        CLI_PARM_FLAG_SET,
        cli_parm_parse_class,
        NULL
    },
    {
        "<rate>",
        "Rate in kbps or fps ("vtss_xstr(QOS_BITRATE_MIN)"-"vtss_xstr(QOS_BITRATE_MAX)")",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_bit_rate,
        QOS_cli_cmd_port_policer_rate
    },
    {
        "<packet_rate>",
        "Rate in fps (1, 2, 4, ..., 512, 1k, 2k, 4k, ..., 1024k)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_frame_rate,
        NULL
    },
    {
        "<dpbl>",
        "Drop Precedence Bypass Level (0-3)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dpbl,
        NULL
    },
    {
        "<weight>",
        "Scheduler weight (1-100)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_weight,
        NULL
    },
    {
        "<bit_rate>",
        "Rate in kilo bits per second ("vtss_xstr(QOS_BITRATE_MIN)"-"vtss_xstr(QOS_BITRATE_MAX)")",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_bit_rate,
        NULL
    },
    {
        "<pcp>",
        "Priority Code Point (0-7)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_pcp,
        NULL
    },
    {
        "<dei>",
        "Drop Eligible Indicator (0-1)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dei,
        NULL
    },
#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    {
        "<dpl>",
        "Drop Precedence Level (0-1)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dpl_array,
        QOS_cli_cmd_port_tag_remarking_dpl
    },
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */
    {
        "<dpl>",
        "Drop Precedence Level (0-"vtss_xstr(QOS_DPL_MAX)")",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dpl,
        NULL
    },
// Specific keywords:
    {
        "kbps|fps",
        "kbps  : Unit is kilo bits per second\n"
        "fps   : Unit is frames per second\n"
        "(default: Show port policer unit)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        NULL
    },
#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    {
        "enable|disable",
        "enable     : Enable tag classification\n"
        "disable    : Disable tag classification\n"
        "(default: Show tag classification  mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_classification_tag_class
    },
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */
    {
        "enable|disable",
        "enable     : Enable port policer\n"
        "disable    : Disable port policer\n"
        "(default: Show port policer mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_policer_mode
    },
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM
    {
        "enable|disable",
        "enable     : Enable port policing of unicast frames\n"
        "disable    : Disable port policing of unicast frames\n"
        "(default: Show port policer unicast mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_policer_unicast
    },
    {
        "enable|disable",
        "enable     : Enable port policing of multicast frames\n"
        "disable    : Disable port policing of multicast frames\n"
        "(default: Show port policer multicast mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_policer_multicast
    },
    {
        "enable|disable",
        "enable     : Enable port policing of broadcast frames\n"
        "disable    : Disable port policing of broadcast frames\n"
        "(default: Show port policer broadcast mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_policer_broadcast
    },
    {
        "enable|disable",
        "enable     : Enable port policing of flooding frames\n"
        "disable    : Disable port policing of flooding frames\n"
        "(default: Show port policer flooding mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_policer_flooding
    },
    {
        "enable|disable",
        "enable     : Enable port policing of learning frames\n"
        "disable    : Disable port policing of learning frames\n"
        "(default: Show port policer learning mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_policer_learning
    },
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC
    {
        "enable|disable",
        "enable     : Enable port policer flow control\n"
        "disable    : Disable port policer flow control\n"
        "(default: Show port policer flow control mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_policer_fc
    },
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#if defined(VTSS_SW_OPTION_BUILD_CE)
#ifdef VTSS_FEATURE_QOS_QUEUE_POLICER
    {
        "enable|disable",
        "enable     : Enable port queue policer\n"
        "disable    : Disable port queue policer\n"
        "(default: Show port queue policer mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_queue_policer_mode
    },

#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
    {
        "enable|disable",
        "enable       : Enable unicast storm control\n"
        "disable      : Disable unicast storm control",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_storm_uc
    },
    {
        "enable|disable",
        "enable       : Enable multicast storm control\n"
        "disable      : Disable multicast storm control",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_storm_mc
    },
    {
        "enable|disable",
        "enable       : Enable broadcast storm control\n"
        "disable      : Disable broadcast storm control",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_storm_bc
    },
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */
#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
    {
        "enable|disable",
        "enable     : Enable use of excess bandwidth\n"
        "disable    : Disable use of excess bandwidth\n"
        "(default: Show port queue excess bandwidth mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_queue_shaper_excess
    },
    {
        "enable|disable",
        "enable     : Enable port queue shaper\n"
        "disable    : Disable port queue shaper\n"
        "(default: Show port queue shaper mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_queue_shaper_mode
    },
    {
        "strict|weighted",
        "strict  : Strict mode\n"
        "weighted: Weighted mode\n"
        "(default: Show port scheduler mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        NULL
    },
    {
        "enable|disable",
        "enable     : Enable port shaper\n"
        "disable    : Disable port shaper\n"
        "(default: Show port shaper mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_shaper_mode
    },
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */
#ifdef VTSS_FEATURE_QOS_WRED
    {
        "enable|disable",
        "enable      : Enable WRED\n"
        "disable     : Disable WRED",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_wred
    },
#endif /* VTSS_FEATURE_QOS_WRED */
#ifdef VTSS_FEATURE_VSTAX_V2
    {
        "enable|disable",
        "enable      : Enable Congestion Management\n"
        "disable     : Disable Congestion Management",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_cmef
    },
#endif /* VTSS_FEATURE_VSTAX_V2 */
    {
        "classified|default|mapped",
        "classified: Use classified PCP/DEI values\n"
        "default   : Use default PCP/DEI values\n"
        "mapped    : Use mapped versions of QoS class and DP level\n"
        "(default: Show port tag remarking mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        NULL
    },
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    {
        "enable|disable",
        "enable     : Enable DSCP based classification\n"
        "disable    : Disable DSCP based classification\n"
        "(default: Show DSCP based classification  mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_classification_dscp
    },
    {
        "enable|disable",
        "enable     : Enable DSCP ingress translation\n"
        "disable    : Disable DSCP ingress translation\n"
        "(default: Show DSCP ingress translation mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_port_dscp_translation
    },
    {
        "<dscp_list>",
        "DSCP (0-63) list or 'all'\n"
        "(default: Show DSCP translation table)",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_dscp_list,
        QOS_cli_cmd_dscp_ingr_translation
    },
    {
        "<trans_dscp>",
        "Translated DSCP: 0-63, BE, CS1-CS7, EF or AF11-AF43",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dscp_val,
        QOS_cli_cmd_dscp_ingr_translation
    },
    {
        "enable|disable",
        "enable     : Set DSCP as trusted DSCP\n"
        "disable    : Set DSCP as un-trusted DSCP\n"
        "(default: Show DSCP Trust status)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_dscp_trust
    },
    {
        "<dscp_list>",
        "DSCP (0-63 list or 'all'\n"
        "(default: Show DSCP ingress map table i.e. DSCP->(class, DPL))",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_dscp_list,
        QOS_cli_cmd_dscp_ingr_map
    },
    {
        "none|zero|selected|all",
        "none     : No DSCP ingress classification\n"
        "zero     : Classify DSCP if DSCP = 0\n"
        "selected : Classify DSCP for which class. mode is 'enable'\n"
        "all      : Classify all DSCP\n"
        "(default: Show port DSCP ingress classification mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_port_dscp_ingr_classification
    },
    {
        "<dscp>",
        "Mapped DSCP: 0-63, BE, CS1-CS7, EF or AF11-AF43",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dscp_val,
        QOS_cli_cmd_dscp_ingr_classification_map
    },
    {
        "<dscp_list>",
        "DSCP (0-63) list or 'all'",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_dscp_list,
        NULL
    },
    {
        "enable|disable",
        "enable     : Enable DSCP ingress classification\n"
        "disable    : Disable DSCP ingress classification\n"
        "(default: Show DSCP classification mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_dscp_ingr_classification_mode
    },
    {
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
        "disable|enable|remap_dp_unaware|remap_dp_aware",
        "disable          : Disable DSCP egress rewrite\n"
        "enable           : Enable DSCP egress rewrite with the value\n"
        "                   received from analyzer\n"
        "remap_dp_unaware : Rewrite DSCP in egress frame with remapped\n"
        "                   DSCP where remap is DP unaware or DP = 0\n"
        "remap_dp_aware   : Rewrite DSCP in egress frame with remapped\n"
        "                   DSCP where remap is DP aware and DP = 1\n"
#else
        "disable|enable|remap",
        "disable : Disable DSCP egress rewrite\n"
        "enable  : Enable DSCP egress rewrite with the value\n"
        "          received from analyzer\n"
        "remap   : Rewrite DSCP in egress frame with remapped DSCP\n"
#endif
        "(default: Show port DSCP egress remarking mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_dscp_remark_mode,
        QOS_cli_cmd_port_dscp_egress_remark_mode
    },
    {
        "<dscp>",
        "Egress remapped DSCP: 0-63, BE, CS1-CS7, EF or AF11-AF43",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dscp_val,
        QOS_cli_cmd_dscp_egress_remap
    },
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
    {
        "<mode>",
        "802.3ar modes. The following modes are supported \n"
        " frame-overhead\n"
        " payload-rate-limiter\n"
        " frame-rate-limiter",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_dot3ar_mode_sel,
        NULL
    },
    {
        "enable|disable",
        "Enable or disable 802.3ar mode",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dot3ar_mode_conf,
        QOS_cli_cmd_dot3ar_rate_limiter
    },
    {
        "<rate>",
        "Rate configurations.\n"
        " The following rates are allowed based on mode selections\n"
        " 12-261120    : Frame overhead. (Defaulut value:12)\n"
        " 1-100        : Port utilization rate. (Default value:100)\n"
        " 12-16383     : Frame rate. (Default: 64)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dot3ar_rate_conf,
        NULL
    },
    {
        "include|exclude",
        "Include or exclude preamble in payload length calculation",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dot3ar_preamble,
        QOS_cli_cmd_dot3ar_preamble
    },
    {
        "include|exclude",
        "Include or exclude header size in payload length calculation",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dot3ar_header,
        QOS_cli_cmd_dot3ar_header
    },
    {
        "<header-size>",
        "Header length to be excluded from payload length in the 1-31",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dot3ar_header_size,
        QOS_cli_cmd_dot3ar_header_size
    },
    {
        "enable|disable",
        "Enable or disable 802.3ar accumulate mode",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_dot3ar_accumulate,
        QOS_cli_cmd_dot3ar_accumulate,
    },
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */


#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    {
        "<lport_list>",
        "Lport list",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_lport_list,
        NULL
    },
    {
        "enable|disable",
        "enable      : Enable Shaper\n"
        "disable     : Disable Shaper",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        NULL
    },
#endif
    {NULL, NULL, 0, 0, NULL}
};

/****************************************************************************/
/*  Command table                                                           */
/****************************************************************************/
enum {
    PRIO_QOS_CONF,
    PRIO_QOS_PORT_CLASSIFICATION_CLASS,
    PRIO_QOS_PORT_CLASSIFICATION_DPL,
    PRIO_QOS_PORT_CLASSIFICATION_PCP,
    PRIO_QOS_PORT_CLASSIFICATION_DEI,
    PRIO_QOS_PORT_CLASSIFICATION_TAG_CLASS,
    PRIO_QOS_PORT_CLASSIFICATION_MAP,
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    PRIO_QOS_PORT_CLASSIFICATION_DSCP,
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
    PRIO_QOS_PORT_POLICER_MODE,
    PRIO_QOS_PORT_POLICER_RATE,
    PRIO_QOS_PORT_POLICER_UNIT,
    PRIO_QOS_PORT_POLICER_DPBL,
    PRIO_QOS_PORT_POLICER_UNICAST,
    PRIO_QOS_PORT_POLICER_MULTICAST,
    PRIO_QOS_PORT_POLICER_BROADCAST,
    PRIO_QOS_PORT_POLICER_FLOODING,
    PRIO_QOS_PORT_POLICER_LEARNING,
    PRIO_QOS_PORT_POLICER_FC,
    PRIO_QOS_PORT_QUEUE_POLICER_MODE,
    PRIO_QOS_PORT_QUEUE_POLICER_RATE,
    PRIO_QOS_PORT_SCHEDULER_MODE,
    PRIO_QOS_PORT_SCHEDULER_WEIGHT,
    PRIO_QOS_PORT_SHAPER_MODE,
    PRIO_QOS_PORT_SHAPER_RATE,
    PRIO_QOS_PORT_QUEUE_SHAPER_MODE,
    PRIO_QOS_PORT_QUEUE_SHAPER_RATE,
    PRIO_QOS_PORT_QUEUE_SHAPER_EXCESS,
    PRIO_QOS_PORT_TAG_REMARKING_MODE,
    PRIO_QOS_PORT_TAG_REMARKING_PCP,
    PRIO_QOS_PORT_TAG_REMARKING_DEI,
    PRIO_QOS_PORT_TAG_REMARKING_DPL,
    PRIO_QOS_PORT_TAG_REMARKING_MAP,
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    PRIO_QOS_PORT_DSCP_TRANSLATION,
    PRIO_QOS_PORT_DSCP_INGR_CLASSIFICATION,
    PRIO_QOS_PORT_DSCP_EGRESS_REMARK_MODE,
    PRIO_QOS_DSCP_INGR_MAP,
    PRIO_QOS_DSCP_INGR_TRANSLATION,
    PRIO_QOS_DSCP_TRUST,
    PRIO_QOS_DSCP_INGR_CLASSIFICATION_MODE,
    PRIO_QOS_DSCP_INGR_CLASSIFICATION_MAP,
    PRIO_QOS_DSCP_EGRESS_REMAP,
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
    PRIO_QOS_PORT_STORM_UNICAST,
    PRIO_QOS_PORT_STORM_BROADCAST,
    PRIO_QOS_PORT_STORM_UNKNOWN,
    PRIO_QOS_STORM_UC,
    PRIO_QOS_STORM_MC,
    PRIO_QOS_STORM_BC,
    PRIO_QOS_WRED,
    PRIO_QOS_QCL_ADD,
    PRIO_QOS_QCL_DEL,
    PRIO_QOS_QCL_LOOKUP,
    PRIO_QOS_QCL_STATUS,
    PRIO_QOS_QCL_REFRESH,
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    PRIO_QOS_LPORT_SHAPER_MODE,
    PRIO_QOS_LPORT_SHAPER_RATE,
    PRIO_QOS_LPORT_QUEUE_SHAPER_MODE,
    PRIO_QOS_LPORT_QUEUE_SHAPER_RATE,
    PRIO_QOS_LPORT_SCHEDULER_WEIGHT,
    PRIO_QOS_LPORT_QUEUE_SCHEDULER_WEIGHT,
    PRIO_QOS_LPORT_WRED,
#endif
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
    PRIO_QOS_PORT_DOT3AR_CONF,
    PRIO_QOS_PORT_DOT3AR_RATE_LIMITER,
    PRIO_QOS_PORT_DOT3AR_HEADER_SIZE,
    PRIO_QOS_PORT_DOT3AR_HEADER,
    PRIO_QOS_PORT_DOT3AR_PREAMBLE,
    PRIO_QOS_PORT_DOT3AR_ACCUMULATE_MODE,
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */
    PRIO_DEBUG_QOS_CMEF = CLI_CMD_SORT_KEY_DEFAULT,
    PRIO_DEBUG_QOS_REGISTRATIONS = CLI_CMD_SORT_KEY_DEFAULT,
};

cli_cmd_tab_entry (
    "QoS Configuration [<port_list>]",
    NULL,
    "Show QoS Configuration",
    PRIO_QOS_CONF,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_conf,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_SYS_CONF
);
cli_cmd_tab_entry (
    "QoS Port Classification Class [<port_list>]",
    "QoS Port Classification Class [<port_list>] [<class>]",
    "Set or show the default QoS class.\n"
    "If the QoS class has been dynamically changed, then the actual\n"
    "QoS class is shown in parentheses after the configured QoS class",
    PRIO_QOS_PORT_CLASSIFICATION_CLASS,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_classification_class,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
cli_cmd_tab_entry (
    "QoS Port Classification PCP [<port_list>]",
    "QoS Port Classification PCP [<port_list>] [<pcp>]",
    "Set or show the default PCP for an untagged frame",
    PRIO_QOS_PORT_CLASSIFICATION_PCP,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_classification_pcp,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Classification DEI [<port_list>]",
    "QoS Port Classification DEI [<port_list>] [<dei>]",
    "Set or show the default DEI for an untagged frame",
    PRIO_QOS_PORT_CLASSIFICATION_DEI,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_classification_dei,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
cli_cmd_tab_entry (
    "QoS Port Classification Tag [<port_list>]",
    "QoS Port Classification Tag [<port_list>] [enable|disable]",
    "Set or show if the classification is based on the PCP and DEI values in tagged frames",
    PRIO_QOS_PORT_CLASSIFICATION_TAG_CLASS,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_classification_tag_class,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Classification Map [<port_list>] [<pcp_list>] [<dei_list>]",
    "QoS Port Classification Map [<port_list>] [<pcp_list>] [<dei_list>] [<class>] [<dpl>]",
    "Set or show the port classification map.\n"
    "This map is used when port classification tag is enabled,\n"
    "and the purpose is to translate the Priority Code Point (PCP) and \n"
    "Drop Eligible Indicator (DEI) from a tagged frame to QoS class \n"
    "and DP level",
    PRIO_QOS_PORT_CLASSIFICATION_MAP,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_classification_map,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */

cli_cmd_tab_entry (
    "QoS Port Classification DPL [<port_list>]",
    "QoS Port Classification DPL [<port_list>] [<dpl>]",
    "Set or show the default Drop Precedence Level",
    PRIO_QOS_PORT_CLASSIFICATION_DPL,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_classification_dpl,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_port_policer_mode) = {
    "QoS Port Policer Mode [<port_list>]"
#if QOS_PORT_POLICER_CNT > 1
    " [<policer_list>]"
#endif
    ,
    "QoS Port Policer Mode [<port_list>]"
#if QOS_PORT_POLICER_CNT > 1
    " [<policer_list>]"
#endif
    " [enable|disable]",
    "Set or show the port policer mode",
    PRIO_QOS_PORT_POLICER_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_port_policer_rate) = {
    "QoS Port Policer Rate [<port_list>]"
#if QOS_PORT_POLICER_CNT > 1
    " [<policer_list>]"
#endif
    ,
    "QoS Port Policer Rate [<port_list>]"
#if QOS_PORT_POLICER_CNT > 1
    " [<policer_list>]"
#endif
    " [<rate>]",
    "Set or show the port policer rate",
    PRIO_QOS_PORT_POLICER_RATE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_rate,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS
CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_port_policer_unit) = {
    "QoS Port Policer Unit [<port_list>]"
#if QOS_PORT_POLICER_CNT > 1
    " [<policer_list>]"
#endif
    ,
    "QoS Port Policer Unit [<port_list>]"
#if QOS_PORT_POLICER_CNT > 1
    " [<policer_list>]"
#endif
    " [kbps|fps]",
    "Set or show the port policer unit",
    PRIO_QOS_PORT_POLICER_UNIT,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_unit,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
#endif /* defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS) */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL
cli_cmd_tab_entry (
    "QoS Port Policer DPBL [<port_list>] [<policer_list>]",
    "QoS Port Policer DPBL [<port_list>] [<policer_list>] [<dpbl>]",
    "Set or show the port policer drop precedence bypass level",
    PRIO_QOS_PORT_POLICER_DPBL,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_dpbl,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL) */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM
cli_cmd_tab_entry (
    "QoS Port Policer Unicast [<port_list>] [<policer_list>]",
    "QoS Port Policer Unicast [<port_list>] [<policer_list>] [enable|disable]",
    "Set or show the port policer unicast mode",
    PRIO_QOS_PORT_POLICER_UNICAST,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_unicast,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Policer Multicast [<port_list>] [<policer_list>]",
    "QoS Port Policer Multicast [<port_list>] [<policer_list>] [enable|disable]",
    "Set or show the port policer multicast mode",
    PRIO_QOS_PORT_POLICER_MULTICAST,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_multicast,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Policer Broadcast [<port_list>] [<policer_list>]",
    "QoS Port Policer Broadcast [<port_list>] [<policer_list>] [enable|disable]",
    "Set or show the port policer broadcast mode",
    PRIO_QOS_PORT_POLICER_BROADCAST,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_broadcast,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Policer Flooding [<port_list>] [<policer_list>]",
    "QoS Port Policer Flooding [<port_list>] [<policer_list>] [enable|disable]",
    "Set or show the port policer flooding mode",
    PRIO_QOS_PORT_POLICER_FLOODING,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_flooding,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Policer Learning [<port_list>] [<policer_list>]",
    "QoS Port Policer Learning [<port_list>] [<policer_list>] [enable|disable]",
    "Set or show the port policer learning mode",
    PRIO_QOS_PORT_POLICER_LEARNING,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_learning,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM) */
#ifdef VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC
cli_cmd_tab_entry(
    qos_port_policer_fc_cmd_ro,
    qos_port_policer_fc_cmd_rw,
    "Set or show the port policer flow control.\n"
    "If policer flow control is enabled and the port is in flow control mode,\n"
    "then pause frames are sent instead of discarding frames",
    PRIO_QOS_PORT_POLICER_FC,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_policer_fc,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC) */

#if defined(VTSS_SW_OPTION_BUILD_CE)
#ifdef VTSS_FEATURE_QOS_QUEUE_POLICER
cli_cmd_tab_entry (
    "QoS Port QueuePolicer Mode [<port_list>] [<queue_list>]",
    "QoS Port QueuePolicer Mode [<port_list>] [<queue_list>] [enable|disable]",
    "Set or show the port queue policer mode",
    PRIO_QOS_PORT_QUEUE_POLICER_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_queue_policer_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port QueuePolicer Rate [<port_list>] [<queue_list>]",
    "QoS Port QueuePolicer Rate [<port_list>] [<queue_list>] [<bit_rate>]",
    "Set or show the port queue policer rate",
    PRIO_QOS_PORT_QUEUE_POLICER_RATE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_queue_policer_rate,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */

#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
cli_cmd_tab_entry (
    "QoS Port Scheduler Mode [<port_list>]",
    "QoS Port Scheduler Mode [<port_list>] [strict|weighted]",
    "Set or show the port scheduler mode",
    PRIO_QOS_PORT_SCHEDULER_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_scheduler_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Scheduler Weight [<port_list>] [<queue_list>]",
    "QoS Port Scheduler Weight [<port_list>] [<queue_list>] [<weight>]",
    "Set or show the port scheduler weight",
    PRIO_QOS_PORT_SCHEDULER_WEIGHT,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_scheduler_weight,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port QueueShaper Mode [<port_list>] [<queue_list>]",
    "QoS Port QueueShaper Mode [<port_list>] [<queue_list>] [enable|disable]",
    "Set or show the port queue shaper mode",
    PRIO_QOS_PORT_QUEUE_SHAPER_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_queue_shaper_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port QueueShaper Rate [<port_list>] [<queue_list>]",
    "QoS Port QueueShaper Rate [<port_list>] [<queue_list>] [<bit_rate>]",
    "Set or show the port queue shaper rate",
    PRIO_QOS_PORT_QUEUE_SHAPER_RATE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_queue_shaper_rate,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port QueueShaper Excess [<port_list>] [<queue_list>]",
    "QoS Port QueueShaper Excess [<port_list>] [<queue_list>] [enable|disable]",
    "Set or show the port queue excess bandwidth mode",
    PRIO_QOS_PORT_QUEUE_SHAPER_EXCESS,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_queue_shaper_excess,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Shaper Mode [<port_list>]",
    "QoS Port Shaper Mode [<port_list>] [enable|disable]",
    "Set or show the port shaper mode",
    PRIO_QOS_PORT_SHAPER_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_shaper_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Shaper Rate [<port_list>]",
    "QoS Port Shaper Rate [<port_list>] [<bit_rate>]",
    "Set or show the port shaper rate",
    PRIO_QOS_PORT_SHAPER_RATE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_shaper_rate,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
cli_cmd_tab_entry (
    "QoS Port TagRemarking Mode [<port_list>]",
    "QoS Port TagRemarking Mode [<port_list>] [classified|default|mapped]",
    "Set or show the port tag remarking mode",
    PRIO_QOS_PORT_TAG_REMARKING_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_tag_remarking_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port TagRemarking PCP [<port_list>]",
    "QoS Port TagRemarking PCP [<port_list>] [<pcp>]",
    "Set or show the default PCP.\n"
    "This value is used when port tag remarking mode is set to 'default'",
    PRIO_QOS_PORT_TAG_REMARKING_PCP,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_tag_remarking_pcp,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port TagRemarking DEI [<port_list>]",
    "QoS Port TagRemarking DEI [<port_list>] [<dei>]",
    "Set or show the default DEI.\n"
    "This value is used when port tag remarking mode is set to 'default'",
    PRIO_QOS_PORT_TAG_REMARKING_DEI,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_tag_remarking_dei,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#if defined(VTSS_ARCH_JAGUAR_1)
cli_cmd_tab_entry (
    "QoS Port TagRemarking DPL [<port_list>]",
    "QoS Port TagRemarking DPL [<port_list>] [<dpl>] [<dpl>] [<dpl>] [<dpl>]",
    "Set or show the 'Drop Precedence level' translation table.\n"
    "This table is used when port tag remarking mode is set to 'mapped',\n"
    "and the purpose is to translate the internal 2 bit 'Drop Precedence level'\n"
    "to a 1 bit 'Drop Precedence level' used in the mapping process.\n"
    "Up to four values (0 or 1) in increasing order is acccepted.\n"
    "Example:\n"
    "'QoS Port TagRemarking DPL 4 0 0 1 1' sets the table for port 4 to:\n"
    "DP level 0 -> 0, DP level 1 -> 0, DP level 2 -> 1, DP level 3 -> 1",
    PRIO_QOS_PORT_TAG_REMARKING_DPL,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_tag_remarking_dpl,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_ARCH_JAGUAR_1 */
cli_cmd_tab_entry (
    "QoS Port TagRemarking Map [<port_list>] [<class_list>] [<dpl_list>]",
    "QoS Port TagRemarking Map [<port_list>] [<class_list>] [<dpl_list>] [<pcp>] [<dei>]",
    "Set or show the port tag remarking map.\n"
    "This map is used when port tag remarking mode is set to 'mapped',\n"
    "and the purpose is to translate the classified QoS class (0-7) \n"
    "and DP level (0-1) to PCP and DEI",
    PRIO_QOS_PORT_TAG_REMARKING_MAP,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_tag_remarking_map,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
cli_cmd_tab_entry (
    "QoS Port Storm Unicast [<port_list>]",
    "QoS Port Storm Unicast [<port_list>] [enable|disable] [<rate>] [kbps|fps]",
    "Set or show the port storm rate limiter for unicast frames",
    PRIO_QOS_PORT_STORM_UNICAST,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_storm_unicast,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Storm Broadcast [<port_list>]",
    "QoS Port Storm Broadcast [<port_list>] [enable|disable] [<rate>] [kbps|fps]",
    "Set or show the port storm rate limiter for broadcast frames",
    PRIO_QOS_PORT_STORM_BROADCAST,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_storm_broadcast,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Port Storm Unknown [<port_list>]",
    "QoS Port Storm Unknown [<port_list>] [enable|disable] [<rate>] [kbps|fps]",
    "Set or show the port storm rate limiter for unknown (flooded) frames",
    PRIO_QOS_PORT_STORM_UNKNOWN,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_storm_unknown,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
cli_cmd_tab_entry (
    "QoS Storm Unicast",
    "QoS Storm Unicast [enable|disable] [<packet_rate>]",
    "Set or show the unicast storm rate limiter.\n"
    "The limiter will only affect flooded frames,\n"
    "i.e. frames with a (VLAN ID, DMAC) pair not present in the MAC Address table",
    PRIO_QOS_STORM_UC,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_storm_uc,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Storm Multicast",
    "QoS Storm Multicast [enable|disable] [<packet_rate>]",
    "Set or show the multicast storm rate limiter.\n"
    "The limiter will only affect flooded frames,\n"
    "i.e. frames with a (VLAN ID, DMAC) pair not present in the MAC Address table",
    PRIO_QOS_STORM_MC,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_storm_mc,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Storm Broadcast",
    "QoS Storm Broadcast [enable|disable] [<packet_rate>]",
    "Set or show the broadcast storm rate limiter.\n"
    "The limiter will only affect flooded frames,\n"
    "i.e. frames with a (VLAN ID, DMAC) pair not present in the MAC Address table",
    PRIO_QOS_STORM_BC,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_storm_bc,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */

#if defined(VTSS_FEATURE_QOS_WRED)
cli_cmd_tab_entry (
    "QoS WRED",
    "QoS WRED [<queue_list>] [enable|disable] [<min_th>] [<mdp_1>] [<mdp_2>] [<mdp_3>]",
    "Set or show the Weighted Random Early Detection parameters",
    PRIO_QOS_WRED,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_wred,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_FEATURE_QOS_WRED */

#if defined(VTSS_FEATURE_VSTAX_V2)
cli_cmd_tab_entry (
    "Debug QoS CMEF",
    "Debug QoS CMEF [enable|disable]",
    "Set or show the Congestion Management mode",
    PRIO_DEBUG_QOS_CMEF,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_cmef,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_FEATURE_VSTAX_V2 */

cli_cmd_tab_entry (
    "Debug QoS Registrations [clear]",
    NULL,
    "Show or clear QoS module registrations",
    PRIO_DEBUG_QOS_REGISTRATIONS,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_debug_regs,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

#ifdef VTSS_FEATURE_QCL_V2
CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_qcl_add) = {
    NULL,
    "QoS QCL Add [<qce_id>] [<qce_id_next>]\n"
#if VTSS_SWITCH_STACKABLE
    "    [<sid>] [<port_list>]\n"
#else
    "    [<port_list>]\n"
#endif /* VTSS_SWITCH_STACKABLE */
    "    [<tag>] [<vid>] [<pcp>] [<dei>] [<smac>] [<dmac_type>]\n"
    "    [(etype [<etype>]) |\n"
    "    (LLC  [<DSAP>] [<SSAP>] [<control>]) |\n"
    "    (SNAP  [<PID>]) |\n"
    "    (ipv4  [<protocol>] [<sip>] [<dscp>] [<fragment>] [<sport>] [<dport>]) |\n"
    "    (ipv6  [<protocol>] [<sip_v6>] [<dscp>] [<sport>] [<dport>])] \n"
    "    [<class>] [<dp>] [<classified_dscp>]",
    "Add or modify QoS Control Entry (QCE).\n\n"
    "If the QCE ID parameter <qce_id> is specified and an entry with this QCE ID\n"
    "already exists, the QCE will be modified. Otherwise, a new QCE will be added.\n"
    "If the QCE ID is not specified, the next available QCE ID will be used. If\n"
    "the next QCE ID parameter <qce_id_next> is specified, the QCE will be placed\n"
    "before this QCE in the list. If the next QCE ID is not specified and if it\n"
    "is a new entry added, the QCE will be placed last in the list. Otherwise if\n"
    "the next QCE ID is not specified and if existing QCE is modified, QCE will\n"
    "be in the same location in the list. To modify and move the entry to last in\n"
    "the list, use the word 'last' for <qce_id_next>",
    PRIO_QOS_QCL_ADD,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_add,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

cli_cmd_tab_entry (
    NULL,
    "QoS QCL Delete <qce_id>",
    "Delete QCE entry from QoS Control list",
    PRIO_QOS_QCL_DEL,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_del,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS QCL Lookup [<qce_id>]",
    NULL,
    "Lookup QoS Control List",
    PRIO_QOS_QCL_LOOKUP,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_lookup,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_qcl_status) = {
    "QoS QCL Status [combined|static"
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    "|voice_vlan"
#endif
    "|conflicts]",
    NULL,
    "Show QCL status. This can be used to display if there is any\n"
    "conflict in QCE for differnet user types",
    PRIO_QOS_QCL_STATUS,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_status,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_qcl_refresh) = {
    "QoS QCL Refresh",
    NULL,
    "Resolve QCE conflict status. Same H/W resource is shared by multiple\n"
    "applications and it may not be available even before MAX QCE entry. So\n"
    "user can release the resource in use by other applications and use this\n"
    "command to acquire the resource",
    PRIO_QOS_QCL_REFRESH,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_refresh,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
cli_cmd_tab_entry (
    "QoS Port Classification DSCP [<port_list>]",
    "QoS Port Classification DSCP [<port_list>] [enable|disable]",
    "Set or show if the classification is based on DSCP value in IP frames",
    PRIO_QOS_PORT_CLASSIFICATION_DSCP,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_classification_dscp,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS Port DSCP Translation [<port_list>]",
    "QoS Port DSCP Translation [<port_list>] [enable|disable]",
    "Set or show DSCP ingress translation mode.\n"
    "If translation is enabled for a port, incoming frame DSCP value is\n"
    "translated and translated value is used for QoS classification",
    PRIO_QOS_PORT_DSCP_TRANSLATION,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_dscp_translation,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS DSCP Translation [<dscp_list>]",
    "QoS DSCP Translation [<dscp_list>] [<trans_dscp>]",
    "Set or show global ingress DSCP translation table.\n"
    "If port DSCP translation is enabled, translation table is used to\n"
    "translate incoming frames DSCP value and translated value is used\n"
    "to map QoS class and DP level",
    PRIO_QOS_DSCP_INGR_TRANSLATION,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dscp_ingr_translation,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS DSCP Trust [<dscp_list>]",
    "QoS DSCP Trust [<dscp_list>] [enable|disable]",
    "Set or show whether a specific DSCP value is trusted.\n"
    "Only frames with trusted DSCP values are mapped to a specific QoS class and DPL.\n"
    "Frames with untrusted DSCP values are treated as a non-IP frame",
    PRIO_QOS_DSCP_TRUST,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dscp_trust,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS DSCP Map [<dscp_list>]",
    "QoS DSCP Map [<dscp_list>] [<class>] [<dpl>]",
    "Set or show DSCP mapping table.\n"
    "This table is used to map QoS class and DP level based on DSCP value.\n"
    "DSCP value used to map QoS class and DPL is either translated DSCP\n"
    "value or incoming frame DSCP value",
    PRIO_QOS_DSCP_INGR_MAP,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dscp_ingr_map,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_port_dscp_ingr_classification) = {
    "QoS Port DSCP Classification [<port_list>]",
    "QoS Port DSCP Classification [<port_list>] [none|zero|selected|all]",
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    "Set or show DSCP classification based on QoS class and DP level.\n"
    "This enables per port to map new DSCP value based on QoS class and DP level",
#else
    "Set or show DSCP classification based on QoS class.\n"
    "This enables per port to map new DSCP value based on QoS class",
#endif
    PRIO_QOS_PORT_DSCP_INGR_CLASSIFICATION,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_dscp_ingr_classification,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_dscp_ingr_classification_map) = {
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    "QoS DSCP Classification Map [<class_list>] [<dpl_list>]",
    "QoS DSCP Classification Map [<class_list>] [<dpl_list>] [<dscp>]",
    "Set or show DSCP ingress classification table.\n"
    "This table is used to map DSCP from QoS class and DP level. The DSCP\n"
    "which needs to be classified depends on port DSCP classification and\n"
    "DSCP classification mode. Incoming frame DSCP may be translated before"
    "using the value for classification",
#else
    "QoS DSCP Classification Map [<class_list>]",
    "QoS DSCP Classification Map [<class_list>] [<dscp>]",
    "Set or show DSCP ingress classification table.\n"
    "This table is used to map DSCP from QoS class. The DSCP which needs\n"
    "to be classified depends on port DSCP classification and DSCP\n"
    "classification mode. Incoming frame DSCP may be translated before"
    "using the value for classification",
#endif
    PRIO_QOS_DSCP_INGR_CLASSIFICATION_MAP,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dscp_ingr_classification_map,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_dscp_ingr_classification_mode) = {
    "QoS DSCP Classification Mode [<dscp_list>]",
    "QoS DSCP Classification Mode [<dscp_list>] [enable|disable]",
    "Set or show DSCP ingress classification mode.\n"
    "If port DSCP classification is 'selected', DSCP will be classified\n"
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    "based on QoS class and DP level only for DSCP value with classification\n"
    "mode 'enabled'. DSCP may be translated DSCP if translation is enabled\n"
    "for the port",
#else
    "based on QoS class only for DSCP value with classification mode 'enabled'.\n"
    "DSCP may be translated DSCP if translation is enabled for the port",
#endif
    PRIO_QOS_DSCP_INGR_CLASSIFICATION_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dscp_ingr_classification_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_port_dscp_egress_remark_mode) = {
    "QoS Port DSCP EgressRemark [<port_list>]",
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    "QoS Port DSCP EgressRemark [<port_list>] [disable|enable|remap_dp_unaware|remap_dp_aware]",
#else
    "QoS Port DSCP EgressRemark [<port_list>] [disable|enable|remap]",
#endif
    "Set or show the port DSCP remarking mode",
    PRIO_QOS_PORT_DSCP_EGRESS_REMARK_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_port_dscp_egress_remark_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};

CLI_CMD_TAB_ENTRY_DECL(QOS_cli_cmd_dscp_egress_remap) = {
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    "QoS DSCP EgressRemap [<dscp_list>] [<dpl_list>]",
    "QoS DSCP EgressRemap [<dscp_list>] [<dpl_list>] [<dscp>]",
    "Set or show DSCP egress remap table. This table is used if the port\n"
    "egress remarking mode is 'remap' and the purpose is to map the\n"
    "DSCP and DP level to a new DSCP value",
#else
    "QoS DSCP EgressRemap [<dscp_list>]",
    "QoS DSCP EgressRemap [<dscp_list>] [<dscp>]",
    "Set or show DSCP egress remap table. This table is used if the port\n"
    "egress remarking mode is 'remap' and the purpose is to map the\n"
    "classified DSCP to a new DSCP value",
#endif
    PRIO_QOS_DSCP_EGRESS_REMAP,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dscp_egress_remap,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
};
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)

cli_cmd_tab_entry (
    "QoS LPORT Shaper Mode [<lport_list>]",
    "QoS LPORT Shaper Mode [<lport_list>] [enable|disable]",
    "Set or show the lport shaper mode",
    PRIO_QOS_LPORT_SHAPER_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_lport_shaper_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS LPORT Shaper Rate [<lport_list>]",
    "QoS LPORT Shaper Rate [<lport_list>] [<bit_rate>]",
    "Set or show the lport shaper rate",
    PRIO_QOS_LPORT_SHAPER_RATE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_lport_shaper_rate,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS LPORT QueueShaper Mode [<lport_list>] [<queue_list>]",
    "QoS LPORT QueueShaper Mode [<lport_list>] [<queue_list>] [enable|disable]",
    "Set or show the lport queue shaper mode",
    PRIO_QOS_LPORT_QUEUE_SHAPER_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_lport_queue_shaper_mode,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS LPORT QueueShaper Rate [<lport_list>] [<queue_list>]",
    "QoS LPORT QueueShaper Rate [<lport_list>] [<queue_list>] [<bit_rate>]",
    "Set or show the lport Queue shaper rate",
    PRIO_QOS_LPORT_QUEUE_SHAPER_RATE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_lport_queue_shaper_rate,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS LPORT Scheduler Weight [<lport_list>]",
    "QoS LPORT Scheduler Weight [<lport_list>] [<weight>]",
    "Set or show the lport scheduler weight",
    PRIO_QOS_LPORT_SCHEDULER_WEIGHT,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_lport_scheduler_weight,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS LPORT QueueScheduler Weight [<lport_list>] [<queue_list>]",
    "QoS LPORT QueueScheduler Weight [<lport_list>] [<queue_list>] [<weight>]",
    "Set or show the lport queue scheduler weight",
    PRIO_QOS_LPORT_QUEUE_SCHEDULER_WEIGHT,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_lport_queue_scheduler_weight,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS LPORT WRED [<lport_list>] [<queue_list>]",
    "QoS LPORT WRED [<lport_list>] [<queue_list>] [enable|disable] [<min_th>] [<mdp_1>] [<mdp_2>] [<mdp_3>]",
    "Set or show the Weighted Random Early Detection parameters",
    PRIO_QOS_LPORT_WRED,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_host_wred,
    QOS_cli_default_set,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)

cli_cmd_tab_entry (
    "QoS Port DOT3AR Ratelimiter [<port_list>] [<mode>]",
    "QoS Port DOT3AR Ratelimiter [<port_list>] [<mode>] [enable|disable] [<rate>]",
    "Set or show the DOT3AR rate limters",
    PRIO_QOS_PORT_DOT3AR_RATE_LIMITER,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dot3ar_rate_limiter,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS Port DOT3AR Preamble [<port_list>]",
    "QoS Port DOT3AR Preamble [<port_list>] [include|exclude]",
    "Set or show the DOT3AR preamble configuration",
    PRIO_QOS_PORT_DOT3AR_PREAMBLE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dot3ar_preamble,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS Port DOT3AR HeaderSize",
    "QoS Port DOT3AR HeaderSize [<header-size>]",
    "Set or show the DOT3AR header size configuration",
    PRIO_QOS_PORT_DOT3AR_HEADER_SIZE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dot3ar_header_size,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS Port DOT3AR Header",
    "QoS Port DOT3AR Header [<port_list>] [include|exclude]",
    "Set or show the DOT3AR header configuration",
    PRIO_QOS_PORT_DOT3AR_HEADER,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dot3ar_header,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    "QoS Port DOT3AR AccumulateMode [<port_list>]",
    "QoS Port DOT3AR AccumulateMode [<port_list>] [enable|disable]",
    "Set or show the DOT3AR Accumulate mode",
    PRIO_QOS_PORT_DOT3AR_ACCUMULATE_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dot3ar_accumulate,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */





/****************************************************************************/
/*  Initialization                                                          */
/****************************************************************************/

void qos_cli_init(void)
{
    port_isid_info_t       info;
    char                   *txt;

    (void)snprintf(QOS_cli_sid_help_str, sizeof(QOS_cli_sid_help_str), "Switch ID (%d-%d) or 'any'", VTSS_USID_START, VTSS_USID_END - 1);

    /* register the size required for qos req. structure */
    cli_req_size_register(sizeof(qos_cli_req_t));

    (void)port_isid_info_get(VTSS_ISID_LOCAL, &info);

    /* If no ports support flow control, it is a debug command */
    txt = qos_port_policer_fc_cmd_ro;
    if ((info.cap & PORT_CAP_FLOW_CTRL) == 0) {
        txt += sprintf(txt, "Debug ");
        qos_port_policer_fc_cmd_disabled = 1;
    }

#if QOS_PORT_POLICER_CNT > 1
    sprintf(txt, "QoS Port Policer FlowControl [<port_list>] [<policer_list>]");
#else
    sprintf(txt, "QoS Port Policer FlowControl [<port_list>]");
#endif
    sprintf(qos_port_policer_fc_cmd_rw, "%s [enable|disable]", qos_port_policer_fc_cmd_ro);
}

#elif defined(VTSS_ARCH_LUTON28)
/****************************************************************************/
/*  LUTON28 code starts here                                                */
/****************************************************************************/

typedef struct {
    cli_spec_t            packet_rate_spec;
    vtss_packet_rate_t    packet_rate;
    uchar                 tag_prio;
    cli_spec_t            etype_spec;
    vtss_etype_t          etype;
    vtss_prio_t           class_cnt;
    vtss_qcl_id_t         qcl_id;
    vtss_qce_id_t         qce_id;
    vtss_qce_id_t         qce_id_next;
    vtss_dscp_t           dscp;
    BOOL                  tos_list[8];
    BOOL                  tag_prio_list[8];
    cli_spec_t            bit_rate_spec;
    vtss_bitrate_t        bit_rate;
    vtss_qce_type_t       qce_type;
    BOOL                  wfq;
    vtss_weight_t         weight;
    vtss_udp_tcp_t        dport_min;
    vtss_udp_tcp_t        dport_max;
} qos_cli_req_t;


/****************************************************************************/
/*  Private functions                                                       */
/****************************************************************************/


static char *cli_packet_rate_txt(ulong rate, char *buf)
{
    char *txt = "";

    if (rate >= 1000 && (rate % 1000) == 0) {
        rate /= 1000;
        txt = "k";
    }
    sprintf(buf, "%5d %spps", rate, txt);
    return buf;
}

static char *cli_bit_rate_txt(ulong rate, char *buf)
{
    char c;

    if (rate < 10000) {
        c = 'k';
    } else if (rate < 10000000) {
        rate = (rate / 1000);
        c = 'M';
    } else {
        rate = (rate / 1000000);
        c = 'G';
    }
    sprintf(buf, "%4d %cbps", rate, c);
    return buf;
}

/* QoS policer configuration */
static void QOS_cli_cmd_policer(cli_req_t *req, BOOL mc, BOOL bc, BOOL uc)
{
    qos_conf_t         conf;
    vtss_packet_rate_t *rate;
    BOOL               *enabled;
    char               buf[32];
    qos_cli_req_t      *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_conf_get(&conf) != VTSS_OK) {
        return;
    }

    if (req->set) {
        enabled = (mc ? &conf.policer_mc_status : bc ? &conf.policer_bc_status : &conf.policer_uc_status);
        rate = (mc ? &conf.policer_mc : bc ? &conf.policer_bc : &conf.policer_uc);
        if (req->enable) {
            *enabled = 1;
        }
        if (req->disable) {
            *enabled = 0;
        }
        if (qos_req->packet_rate_spec == CLI_SPEC_VAL) {
            *rate = qos_req->packet_rate;
        }
        (void) qos_conf_set(&conf);
    } else {
        if (mc) {
            CPRINTF("Storm Multicast: %s %s\n",
                    cli_bool_txt(conf.policer_mc_status),
                    cli_packet_rate_txt(conf.policer_mc, buf));
        }
        if (bc) {
            CPRINTF("Storm Broadcast: %s %s\n",
                    cli_bool_txt(conf.policer_bc_status),
                    cli_packet_rate_txt(conf.policer_bc, buf));
        }
        if (uc) {
            CPRINTF("Storm Unicast  : %s %s\n",
                    cli_bool_txt(conf.policer_uc_status),
                    cli_packet_rate_txt(conf.policer_uc, buf));
        }
    }
}

/* QoS port configuration */
static void QOS_cli_cmd_port(cli_req_t *req, BOOL def_prio, BOOL tag_prio, BOOL qcl,
                             BOOL mode, BOOL weight, BOOL policer, BOOL shaper)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    vtss_prio_t     prio;
    uint            w;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }

            if (req->set) {
                if (def_prio) {
                    conf.default_prio = req->class_;
                }
                if (tag_prio) {
                    conf.usr_prio = qos_req->tag_prio;
                }
                if (qcl) {
                    if (conf.qcl_no > QCL_ID_END) {
                        CPRINTF("Cannot change the default port QCL ID, it is occupied by other module\n");
                        return;
                    }
                    conf.qcl_no = qos_req->qcl_id;
                }
                if (policer) {
                    if (req->enable) {
                        conf.port_policer[0].enabled = TRUE;
                    }
                    if (req->disable) {
                        conf.port_policer[0].enabled = FALSE;
                    }
                    if (qos_req->bit_rate_spec == CLI_SPEC_VAL) {
                        conf.port_policer[0].policer.rate = qos_req->bit_rate;
                    }
                }
                if (shaper) {
                    if (req->enable) {
                        conf.shaper_status = 1;
                    }
                    if (req->disable) {
                        conf.shaper_status = 0;
                    }
                    if (qos_req->bit_rate_spec == CLI_SPEC_VAL) {
                        conf.shaper_rate = qos_req->bit_rate;
                    }
                }
                if (mode) {
                    conf.wfq_enable = qos_req->wfq;
                }
                for (prio = VTSS_PRIO_START; weight && prio < VTSS_PRIO_END; prio++)
                    if (req->class_spec == CLI_SPEC_NONE || req->class_ == prio) {
                        conf.weight[prio] = qos_req->weight;
                    }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    if (def_prio) {
                        p += sprintf(p, "Default  ");
                    }
                    if (tag_prio) {
                        p += sprintf(p, "Tag Priority  ");
                    }
                    if (qcl) {
                        p += sprintf(p, "QCL ID   ");
                    }
                    if (policer) {
                        p += sprintf(p, "Rate Limiter  %s", shaper ? "" : "      ");
                    }
                    if (shaper) {
                        p += sprintf(p, "Shaper     %s", policer ? "" : "         ");
                    }
                    if (mode) {
                        p += sprintf(p, "Mode      ");
                    }
                    if (weight) {
                        if (mode) {
                            p += sprintf(p, "Weight");
                        } else {
                            for (prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++)
                                if (req->class_spec == CLI_SPEC_NONE || req->class_ == prio) {
                                    p += sprintf(p, "%-6s  ", mgmt_prio2txt(prio, 0));
                                }
                        }
                    }
                    cli_table_header(buf);
                }
                CPRINTF("%-6d", pit.uport);
                if (def_prio) {
                    CPRINTF("%-9s", mgmt_prio2txt(conf.default_prio, 0));
                }
                if (tag_prio) {
                    CPRINTF("%-14d", conf.usr_prio);
                }
                if (qcl) {
                    if (conf.qcl_no > QCL_ID_END) {
                        CPRINTF("%-9s", "Occupied");
                    } else {
                        CPRINTF("%-9d", conf.qcl_no);
                    }
                }
                if (policer) {
                    if (!shaper || !conf.port_policer[0].enabled) {
                        CPRINTF("%s ", cli_bool_txt(conf.port_policer[0].enabled));
                    }
                    if (!shaper || conf.port_policer[0].enabled) {
                        CPRINTF("%-9s", cli_bit_rate_txt(conf.port_policer[0].policer.rate, buf));
                    }
                    if (shaper) {
                        CPRINTF("     ");
                    }
                }
                if (shaper) {
                    if (!policer || !conf.shaper_status) {
                        CPRINTF("%s ", cli_bool_txt(conf.shaper_status));
                    }
                    if (!policer || conf.shaper_status) {
                        CPRINTF("%-9s", cli_bit_rate_txt(conf.shaper_rate, buf));
                    }
                    CPRINTF("  ");
                }
                if (mode) {
                    CPRINTF("%-10s", conf.wfq_enable ? "Weighted" : "Strict");
                }
                for (prio = VTSS_PRIO_START; weight && prio < VTSS_PRIO_END; prio++) {
                    if (req->class_spec == CLI_SPEC_NONE || req->class_ == prio) {
                        w = (conf.weight[prio] == VTSS_WEIGHT_1 ? 1 :
                             conf.weight[prio] == VTSS_WEIGHT_2 ? 2 :
                             conf.weight[prio] == VTSS_WEIGHT_4 ? 4 :
                             conf.weight[prio] == VTSS_WEIGHT_8 ? 8 : 7);
                        if (mode) {
                            CPRINTF("%d%s", w, prio == (VTSS_PRIO_END - 1) ? "" : "/");
                        } else {
                            CPRINTF("%-8d", w);
                        }
                    }
                }
                CPRINTF("\n");
            }
        }
    }
}

/****************************************************************************/
/*  Command functions                                                       */
/****************************************************************************/

/* Number of traffic classes */
static void QOS_cli_cmd_classes(cli_req_t *req)
{
    qos_conf_t    conf;
    qos_cli_req_t *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    if (qos_conf_get(&conf) != VTSS_OK) {
        return;
    }

    if (req->set) {
        conf.prio_no = qos_req->class_cnt;
        (void) qos_conf_set(&conf);
    } else {
        CPRINTF("Traffic Classes: %d\n", conf.prio_no);
    }
}


#ifdef VTSS_SW_OPTION_BUILD_SMB
/* QoS Remarking */
static void QOS_cli_cmd_remarking(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }
            if (req->set) {
                if (req->enable) {
                    conf.qos_remarking_enable = 1;
                } else {
                    conf.qos_remarking_enable = 0;
                }
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    p += sprintf(p, "DSCP Remarking  ");
                    cli_table_header(buf);
                }
                CPRINTF("%-6u", pit.uport);
                CPRINTF("%s ", cli_bool_txt(conf.qos_remarking_enable));
                CPRINTF("\n");
            }
        }
    }
}

/* QoS DSCP queue setting */
static void QOS_cli_cmd_dscp_queue(cli_req_t *req)
{
    vtss_usid_t     usid;
    vtss_isid_t     isid;
    port_iter_t     pit;
    BOOL            first;
    qos_port_conf_t conf;
    char            buf[80], *p;
    vtss_prio_t     prio;
    qos_cli_req_t   *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
        if ((isid = req->stack.isid[usid]) == VTSS_ISID_END) {
            continue;
        }

        first = TRUE;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (req->uport_list[pit.uport] == 0 || qos_port_conf_get(isid, pit.iport, &conf) != VTSS_OK) {
                continue;
            }
            if (req->set) {
                conf.dscp_remarking_val[req->class_] = qos_req->dscp;
                (void) qos_port_conf_set(isid, pit.iport, &conf);
            } else {
                if (first) {
                    first = FALSE;
                    cli_cmd_usid_print(usid, req, 1);
                    p = &buf[0];
                    p += sprintf(p, "Port  ");
                    for (prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
                        p += sprintf(p, "%-6s  ", mgmt_prio2txt(prio, 0));
                    }
                    cli_table_header(buf);
                }
                CPRINTF("%-6u", pit.uport);
                for (prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
                    CPRINTF("%-8d", conf.dscp_remarking_val[prio]);
                }
                CPRINTF("\n");
            }
        }
    }
}
#endif /* VTSS_SW_OPTION_BUILD_SMB */

/* QoS QCL add */
static void QOS_cli_cmd_qcl_add(cli_req_t *req)
{
    vtss_qcl_id_t        qcl_id;
    vtss_qce_id_t        qce_id;
    qos_qce_entry_conf_t qce;
    BOOL                 add;
    int                  i;
    qos_cli_req_t        *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    qcl_id = qos_req->qcl_id;
    if (qcl_id == QCL_ID_NONE) {
        qcl_id = QCL_ID_START;
    }
    qce_id = qos_req->qce_id;
    add = (qce_id == QCE_ID_NONE ||
           qos_mgmt_qce_entry_get(qcl_id, qce_id, &qce, 0) != VTSS_OK ||
           qce.type != qos_req->qce_type);

    qce.id = qce_id;
    qce.type = qos_req->qce_type;
    switch (qos_req->qce_type) {
    case VTSS_QCE_TYPE_ETYPE:
        qce.frame.etype.val = qos_req->etype;
        qce.frame.etype.prio = req->class_;
        break;
    case VTSS_QCE_TYPE_VLAN:
        qce.frame.vlan.vid = req->vid;
        qce.frame.vlan.prio = req->class_;
        break;
    case VTSS_QCE_TYPE_UDP_TCP:
        qce.frame.udp_tcp.low = qos_req->dport_min;
        qce.frame.udp_tcp.high = qos_req->dport_max;
        qce.frame.udp_tcp.prio = req->class_;
        break;
    case VTSS_QCE_TYPE_DSCP:
        qce.frame.dscp.dscp_val = qos_req->dscp;
        qce.frame.dscp.prio = req->class_;
        break;
    case VTSS_QCE_TYPE_TOS:
        for (i = 0; i < 8; i++) {
            if (qos_req->tos_list[i]) {
                qce.frame.tos_prio[i] = req->class_;
            } else if (add) {
                qce.frame.tos_prio[i] = VTSS_PRIO_START;
            }
        }
        break;
    case VTSS_QCE_TYPE_TAG:
        for (i = 0; i < 8; i++) {
            if (qos_req->tag_prio_list[i]) {
                qce.frame.tag_prio[i] = req->class_;
            } else if (add) {
                qce.frame.tag_prio[i] = VTSS_PRIO_START;
            }
        }
        break;
    default:
        break;
    }
    if (qos_mgmt_qce_entry_add(qcl_id, qos_req->qce_id_next, &qce) == VTSS_OK) {
        CPRINTF("QCE ID %d %s ", qce.id, add ? "added" : "modified");
        if (qos_req->qce_id_next) {
            CPRINTF("before QCE ID %d", qos_req->qce_id_next);
        } else {
            CPRINTF("last");
        }
        CPRINTF(" in QCL ID %d\n", qcl_id);
    } else {
        CPRINTF("QCL Add failed\n");
    }
}

/* QoS QCL delete */
static void QOS_cli_cmd_qcl_del(cli_req_t *req)
{
    qos_cli_req_t *qos_req = req->module_req;

    if (cli_cmd_conf_slave(req)) {
        return;
    }

    (void) qos_mgmt_qce_entry_del(qos_req->qcl_id, qos_req->qce_id);
}

/* QoS QCL lookup */
static void QOS_cli_cmd_qcl_lookup(cli_req_t *req)
{
    vtss_qcl_id_t        qcl_id;
    vtss_qce_id_t        qce_id;
    qos_qce_entry_conf_t qce;
    int                  i;
    BOOL                 next, first = 1;
    qos_cli_req_t        *qos_req = req->module_req;

    next = (qos_req->qce_id == QCE_ID_NONE ? 1 : 0);
    for (qcl_id = QCL_ID_START; qcl_id <= QCL_ID_END + RESERVED_QCL_CNT; qcl_id++) {
        if (qos_req->qcl_id != QCL_ID_NONE && qos_req->qcl_id != qcl_id) {
            continue;
        }
        qce_id = qos_req->qce_id;
        while (qos_mgmt_qce_entry_get(qcl_id, qce_id, &qce, next) == VTSS_OK) {
            if (qce_id == QCE_ID_NONE || !next) {
                CPRINTF("%sQCL ID %d%s:\n\n", first ? "" : "\n", qcl_id, qcl_id > QCL_ID_END ? "(Reserved)" : "");
                cli_table_header("QCE ID  Type     Class Mapping");
                first = 0;
            }
            CPRINTF("%-3d     ", qce.id);
            switch (qce.type) {
            case VTSS_QCE_TYPE_ETYPE:
                CPRINTF("EType    0x%04x -> %s",
                        qce.frame.etype.val, mgmt_prio2txt(qce.frame.etype.prio, 0));
                break;
            case VTSS_QCE_TYPE_VLAN:
                CPRINTF("VLAN ID  %d -> %s",
                        qce.frame.vlan.vid, mgmt_prio2txt(qce.frame.vlan.prio, 0));
                break;
            case VTSS_QCE_TYPE_UDP_TCP:
                CPRINTF("UDP/TCP  ");
                if (qce.frame.udp_tcp.low == qce.frame.udp_tcp.high) {
                    CPRINTF("%d", qce.frame.udp_tcp.low);
                } else {
                    CPRINTF("%d-%d", qce.frame.udp_tcp.low,  qce.frame.udp_tcp.high);
                }
                CPRINTF(" -> %s", mgmt_prio2txt(qce.frame.udp_tcp.prio, 0));
                break;
            case VTSS_QCE_TYPE_DSCP:
                CPRINTF("IP DSCP  %d -> %s",
                        qce.frame.dscp.dscp_val, mgmt_prio2txt(qce.frame.dscp.prio, 0));
                break;
            case VTSS_QCE_TYPE_TOS:
                CPRINTF("IP ToS   ");
                for (i = 0; i < 8; i++)
                    CPRINTF("%d:%.4s%s ",
                            i, mgmt_prio2txt(qce.frame.tos_prio[i], 0), i == 7 ? "" : ",");
                break;
            case VTSS_QCE_TYPE_TAG:
                CPRINTF("VLAN Tag ");
                for (i = 0; i < 8; i++)
                    CPRINTF("%d:%.4s%s ",
                            i, mgmt_prio2txt(qce.frame.tag_prio[i], 0), i == 7 ? "" : ",");
                break;
            default:
                break;
            }
            CPRINTF("\n");
            qce_id = qce.id;
            if (!next) {
                break;
            }
        }
    }
}

/* QoS configuration */
static void QOS_cli_cmd_conf(cli_req_t *req)
{
    if (!req->set) {
        cli_header("QoS Configuration", 1);
    }

    QOS_cli_cmd_classes(req);
    CPRINTF("\n");
    QOS_cli_cmd_policer(req, 1, 1, 1);
    QOS_cli_cmd_port(req, 1, 1, 1, 1, 1, 1, 1);
    CPRINTF("\n");
    QOS_cli_cmd_qcl_lookup(req);
}
static void QOS_cli_cmd_default(cli_req_t *req)
{
    QOS_cli_cmd_port(req, 1, 0, 0, 0, 0, 0, 0);
}
static void QOS_cli_cmd_tagprio(cli_req_t *req)
{
    QOS_cli_cmd_port(req, 0, 1, 0, 0, 0, 0, 0);
}
static void QOS_cli_cmd_qcl_port(cli_req_t *req)
{
    QOS_cli_cmd_port(req, 0, 0, 1, 0, 0, 0, 0);
}
static void QOS_cli_cmd_mode(cli_req_t *req)
{
    QOS_cli_cmd_port(req, 0, 0, 0, 1, 0, 0, 0);
}
static void QOS_cli_cmd_weight(cli_req_t *req)
{
    QOS_cli_cmd_port(req, 0, 0, 0, 0, 1, 0, 0);
}
static void QOS_cli_cmd_policer_conf(cli_req_t *req)
{
    QOS_cli_cmd_port(req, 0, 0, 0, 0, 0, 1, 0);
}
static void QOS_cli_cmd_shaper(cli_req_t *req)
{
    QOS_cli_cmd_port(req, 0, 0, 0, 0, 0, 0, 1);
}
static void QOS_cli_cmd_policer_uc(cli_req_t *req)
{
    QOS_cli_cmd_policer(req, 0, 0, 1);
}
static void QOS_cli_cmd_policer_mc(cli_req_t *req)
{
    QOS_cli_cmd_policer(req, 1, 0, 0);
}
static void QOS_cli_cmd_policer_bc(cli_req_t *req)
{
    QOS_cli_cmd_policer(req, 0, 1, 0);
}

/****************************************************************************/
/*  Parameter parse functions                                               */
/****************************************************************************/

static int32_t QOS_cli_parse_etype(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong_wc(cmd, &value, 0x0600, 0xffff, &qos_req->etype_spec);
    qos_req->etype = value;

    return error;
}

static int32_t QOS_cli_parse_class_cnt(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (cli_parse_ulong(cmd, &value, 1, 4) ||
             value == 3);
    qos_req->class_cnt = value;

    return error;
}

static int32_t QOS_cli_parse_weight(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 1, 8);
    switch (value) {
    case 1:
        qos_req->weight = VTSS_WEIGHT_1;
        break;
    case 2:
        qos_req->weight = VTSS_WEIGHT_2;
        break;
    case 4:
        qos_req->weight = VTSS_WEIGHT_4;
        break;
    case 8:
        qos_req->weight = VTSS_WEIGHT_8;
        break;
    default:
        error = 1;
        break;
    }

    return error;
}

#ifdef VTSS_SW_OPTION_BUILD_SMB
static int32_t QOS_cli_parse_dscp_remark(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 63);
    switch (value) {
    case 0:
        qos_req->dscp = 0;
        break;
    case 8:
        qos_req->dscp = 8;
        break;
    case 16:
        qos_req->dscp = 16;
        break;
    case 24:
        qos_req->dscp = 24;
        break;
    case 32:
        qos_req->dscp = 32;
        break;
    case 40:
        qos_req->dscp = 40;
        break;
    case 48:
        qos_req->dscp = 48;
        break;
    case 56:
        qos_req->dscp = 56;
        break;
    case 46:
        qos_req->dscp = 46;
        break;
    default:
        error = 1;
        break;
    }

    return error;
}
#endif /* VTSS_SW_OPTION_BUILD_SMB */

static int32_t cli_qos_all_qcl_id_parse (char *cmd, char *cmd2, char *stx, char *cmd_org,
                                         cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = (qos_cli_req_t *)req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, QCL_ID_START, QCL_ID_END + RESERVED_QCL_CNT);
    qos_req->qcl_id = value;

    return error;
}

static int32_t QOS_cli_parse_qcl_id(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, QCL_ID_START, QCL_ID_END);
    qos_req->qcl_id = value;

    return error;
}

static int32_t QOS_cli_parse_qce_id(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, QCE_ID_START, QCE_ID_END);
    qos_req->qce_id = value;

    return error;
}

static int32_t QOS_cli_parse_qce_id_next(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, QCE_ID_START, QCE_ID_END);
    qos_req->qce_id_next = value;

    return error;
}

static int32_t QOS_cli_parse_sdport(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   min   = 1;
    ulong   max   = VTSS_PORTS;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_range(cmd, &min, &max, 0, 0xffff);
    qos_req->dport_min = min;
    qos_req->dport_max = max;

    return error;
}

static int32_t QOS_cli_parse_dscp(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 63);
    qos_req->dscp = value;

    return error;
}

static int32_t QOS_cli_parse_tos_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error   = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parm_parse_list(cmd, qos_req->tos_list, 0, 7, 0);

    return error;
}

static int32_t QOS_cli_parse_tag_prio(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    ulong   value = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 7);
    qos_req->tag_prio = value;

    return error;
}

static int32_t QOS_cli_parse_prio_list(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parm_parse_list(cmd, qos_req->tag_prio_list, 0, 7, 0);

    return error;
}

static int32_t QOS_cli_parse_packet_rate(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = (mgmt_txt2rate(cmd, &qos_req->packet_rate, 10) != VTSS_OK);
    if (!error) {
        qos_req->packet_rate_spec = CLI_SPEC_VAL;
    }

    return error;
}

static int32_t QOS_cli_parse_bit_rate(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &qos_req->bit_rate, QOS_BITRATE_MIN, QOS_BITRATE_MAX);
    if (!error) {
        qos_req->bit_rate_spec = CLI_SPEC_VAL;
    }

    return error;
}

static int32_t QOS_cli_parse_keyword(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    char *found = cli_parse_find(cmd, stx);
    qos_cli_req_t *qos_req = req->module_req;

    req->parm_parsed = 1;
    if (found != NULL) {
        if (!strncmp(found, "etype", 5)) {
            qos_req->qce_type = VTSS_QCE_TYPE_ETYPE;
        } else if (!strncmp(found, "weighted", 8)) {
            qos_req->wfq = 1;
        } else if (!strncmp(found, "tag_prio", 8)) {
            qos_req->qce_type = VTSS_QCE_TYPE_TAG;
        } else if (!strncmp(found, "port", 4)) {
            req->port = 1;
            qos_req->qce_type = VTSS_QCE_TYPE_UDP_TCP;
        } else if (!strncmp(found, "dscp", 4)) {
            qos_req->qce_type = VTSS_QCE_TYPE_DSCP;
        } else if (!strncmp(found, "tos", 3)) {
            qos_req->qce_type = VTSS_QCE_TYPE_TOS;
        } else if (!strncmp(found, "vid", 3)) {
            qos_req->qce_type = VTSS_QCE_TYPE_VLAN;
        }
    }

    return (found == NULL ? 1 : 0);
}

/****************************************************************************/
/*  Parameter table                                                         */
/****************************************************************************/

static cli_parm_t qos_cli_parm_table[] = {
    {
        "enable|disable",
        "enable     : Enable rate limiter\n"
        "disable    : Disable rate limiter\n"
        "(default: Show rate limiter mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_policer_conf
    },
    {
        "enable|disable",
        "enable     : Enable shaper\n"
        "disable    : Disable shaper\n"
        "(default: Show shaper mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_shaper
    },
    {
        "enable|disable",
        "enable       : Enable unicast storm control\n"
        "disable      : Disable unicast storm control",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_policer_uc
    },
    {
        "enable|disable",
        "enable       : Enable multicast storm control\n"
        "disable      : Disable multicast storm control",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_policer_mc
    },
    {
        "enable|disable",
        "enable       : Enable broadcast storm control\n"
        "disable      : Disable broadcast storm control",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_policer_bc
    },
#ifdef VTSS_SW_OPTION_BUILD_SMB
    {
        "enable|disable",
        "enable       : Enable QoS Remarking\n"
        "disable      : Disable QoS Remarking",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        cli_parm_parse_keyword,
        QOS_cli_cmd_remarking
    },
#endif /* VTSS_SW_OPTION_BUILD_SMB */
    {
        "<etype>",
        "Ethernet Type",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_etype,
        QOS_cli_cmd_qcl_add
    },
    {
        "etype",
        "Ethernet Type keyword",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_keyword,
        NULL
    },
    {
        "<class>",
        "Number of traffic classes (1,2 or 4)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_class_cnt,
        QOS_cli_cmd_classes
    },
    {
        "<class>",
        "Traffic class low/normal/medium/high or 1/2/3/4",
        CLI_PARM_FLAG_NONE,
        cli_parm_parse_class,
        QOS_cli_cmd_weight
    },
    {
        "<class>",
        "Traffic class low/normal/medium/high or 1/2/3/4",
        CLI_PARM_FLAG_SET,
        cli_parm_parse_class,
        NULL
    },
    {
        "<weight>",
        "Traffic class weight 1/2/4/8",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_weight,
        NULL
    },
#ifdef VTSS_SW_OPTION_BUILD_SMB
    {
        "<dscp>",
        "QoS DSCP Remarking Value 0/8/16/24/32/40/48/56/46",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dscp_remark,
        QOS_cli_cmd_dscp_queue
    },
#endif /* VTSS_SW_OPTION_BUILD_SMB */
    {
        "strict|weighted",
        "strict  : Strict mode\n"
        "weighted: Weighted mode\n"
        "(default: Show QoS mode)",
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        QOS_cli_parse_keyword,
        NULL
    },
    {
        "<qcl_id>",
        "QCL ID",
        CLI_PARM_FLAG_SET,
        cli_qos_all_qcl_id_parse,
        QOS_cli_cmd_qcl_lookup
    },
    {
        "<qcl_id>",
        "QCL ID",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qcl_id,
        NULL
    },
    {
        "<qce_id>",
        "QCE ID (1-24)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_id,
        NULL
    },
    {
        "<qce_id_next>",
        "Next QCE ID (1-24)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_qce_id_next,
        NULL
    },
    {
        "<udp_tcp_port>",
        "Source or destination UDP/TCP port (0-65535)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_sdport,
        NULL
    },
    {
        "<dscp>",
        "IP DSCP (0-63)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_dscp,
        NULL
    },
    {
        "<tos_list>",
        "IP ToS list (0-7)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_tos_list,
        NULL
    },
    {
        "<tag_prio_list>",
        "VLAN tag priority list (0-7)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_prio_list,
        NULL
    },
    {
        "<tag_prio>",
        "VLAN tag priority (0-7)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_tag_prio,
        NULL
    },
    {
        "tag_prio",
        "VLAN tag priority keyword",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_keyword,
        NULL
    },
    {
        "port",
        "UDP/TCP port keyword",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_keyword,
        QOS_cli_cmd_qcl_add
    },
    {
        "dscp",
        "IP DSCP keyword",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_keyword,
        NULL
    },
    {
        "tos",
        "IP ToS keyword",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_keyword,
        NULL
    },
    {
        "<packet_rate>",
        "Rate in pps (1, 2, 4, ..., 512, 1k, 2k, 4k, ..., 1024k)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_packet_rate,
        NULL
    },
    {
        "<bit_rate>",
        "Rate in 1000 bits per second ("vtss_xstr(QOS_BITRATE_MIN)"-"vtss_xstr(QOS_BITRATE_MAX)" kbps)",
        CLI_PARM_FLAG_SET,
        QOS_cli_parse_bit_rate,
        NULL
    },
    {
        "vid",
        "VLAN ID keyword",
        CLI_PARM_FLAG_NONE,
        QOS_cli_parse_keyword,
        NULL
    },
    {NULL, NULL, 0, 0, NULL}
};

/****************************************************************************/
/*  Command table                                                           */
/****************************************************************************/
enum {
    PRIO_QOS_CONF,
    PRIO_QOS_CLASSES,
    PRIO_QOS_DEFAULT,
    PRIO_QOS_TAGPRIO,
    PRIO_QOS_QCL_PORT,
    PRIO_QOS_QCL_ADD,
    PRIO_QOS_QCL_DEL,
    PRIO_QOS_QCL_LOOKUP,
    PRIO_QOS_MODE,
    PRIO_QOS_WEIGHT,
    PRIO_QOS_POLICER,
    PRIO_QOS_SHAPER,
    PRIO_QOS_POLICER_UC,
    PRIO_QOS_POLICER_MC,
    PRIO_QOS_POLICER_BC,
#ifdef VTSS_SW_OPTION_BUILD_SMB
    PRIO_QOS_REMARKING,
    PRIO_QOS_DSCP_QUEUE,
#endif /* VTSS_SW_OPTION_BUILD_SMB */
};

cli_cmd_tab_entry (
    "QoS Configuration [<port_list>]",
    NULL,
    "Show QoS Configuration",
    PRIO_QOS_CONF,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_conf,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_SYS_CONF
);
cli_cmd_tab_entry (
    "QoS Classes",
    "QoS Classes [<class>]",
    "Set or show the number of traffic classes",
    PRIO_QOS_CLASSES,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_classes,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Default [<port_list>]",
    "QoS Default [<port_list>] [<class>]",
    "Set or show the default port priority",
    PRIO_QOS_DEFAULT,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_default,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Tagprio [<port_list>]",
    "QoS Tagprio [<port_list>] [<tag_prio>]",
    "Set or show the port VLAN tag priority",
    PRIO_QOS_TAGPRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_tagprio,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS QCL Port [<port_list>]",
    "QoS QCL Port [<port_list>] [<qcl_id>]",
    "Set or show the port QCL ID",
    PRIO_QOS_QCL_PORT,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_port,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    NULL,
    "QoS QCL Add [<qcl_id>] [<qce_id>] [<qce_id_next>]\n"
    "            (etype <etype>) |\n"
    "            (vid <vid>) |\n"
    "            (port <udp_tcp_port>) |\n"
    "            (dscp <dscp>) |\n"
    "            (tos <tos_list>) |\n"
    "            (tag_prio <tag_prio_list>)\n"
    "            <class>",
    "Add or modify QoS Control Entry (QCE).\n\n"
    "If the QCE ID parameter <qce_id> is specified and an entry with this QCE ID\n"
    "already exists, the QCE will be modified. Otherwise, a new QCE will be added.\n"
    "If the QCE ID is not specified, the next available QCE ID will be used.\n\n"
    "If the next QCE ID parameter <qce_id_next> is specified, the QCE will be placed\n"
    "before this QCE in the list. If the next QCE ID is not specified, the QCE\n"
    "will be placed last in the list",
    PRIO_QOS_QCL_ADD,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_add,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    NULL,
    "QoS QCL Delete <qcl_id> <qce_id>",
    "Delete QCE",
    PRIO_QOS_QCL_DEL,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_del,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS QCL Lookup [<qcl_id>] [<qce_id>]",
    NULL,
    "Lookup QCE",
    PRIO_QOS_QCL_LOOKUP,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_qcl_lookup,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Mode [<port_list>]",
    "QoS Mode [<port_list>] [strict|weighted]",
    "Set or show the port egress scheduler mode",
    PRIO_QOS_MODE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_mode,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Weight [<port_list>]",
    "QoS Weight [<port_list>] [<class>] [<weight>]",
    "Set or show the port egress scheduler weight",
    PRIO_QOS_WEIGHT,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_weight,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Rate Limiter [<port_list>]",
    "QoS Rate Limiter [<port_list>] [enable|disable] [<bit_rate>]",
    "Set or show the port rate limiter",
    PRIO_QOS_POLICER,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_policer_conf,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Shaper [<port_list>]",
    "QoS Shaper [<port_list>] [enable|disable] [<bit_rate>]",
    "Set or show the port shaper",
    PRIO_QOS_SHAPER,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_shaper,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Storm Unicast",
    "QoS Storm Unicast [enable|disable] [<packet_rate>]",
    "Set or show the unicast storm rate limiter.\n"
    "The limiter will only affect flooded frames,\n"
    "i.e. frames with a (VLAN ID, DMAC) pair not present in the MAC Address table",
    PRIO_QOS_POLICER_UC,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_policer_uc,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Storm Multicast",
    "QoS Storm Multicast [enable|disable] [<packet_rate>]",
    "Set or show the multicast storm rate limiter.\n"
    "The limiter will only affect flooded frames,\n"
    "i.e. frames with a (VLAN ID, DMAC) pair not present in the MAC Address table",
    PRIO_QOS_POLICER_MC,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_policer_mc,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS Storm Broadcast",
    "QoS Storm Broadcast [enable|disable] [<packet_rate>]",
    "Set or show the broadcast storm rate limiter.\n"
    "The limiter will only affect flooded frames,\n"
    "i.e. frames with a (VLAN ID, DMAC) pair not present in the MAC Address table",
    PRIO_QOS_POLICER_BC,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_policer_bc,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#ifdef VTSS_SW_OPTION_BUILD_SMB
cli_cmd_tab_entry (
    "QoS DSCP Remarking [<port_list>]",
    "QoS DSCP Remarking [<port_list>] [enable|disable]",
    "Set or show the status of QoS DSCP Remarking",
    PRIO_QOS_REMARKING,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_remarking,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
cli_cmd_tab_entry (
    "QoS DSCP Queue Mapping [<port_list>]",
    "QoS DSCP Queue Mapping [<port_list>] [<class>] [<dscp>]",
    "Set or show the DSCP value for QoS DSCP Remarking",
    PRIO_QOS_DSCP_QUEUE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_QOS,
    QOS_cli_cmd_dscp_queue,
    NULL,
    qos_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_SW_OPTION_BUILD_SMB */

/****************************************************************************/
/*  Initialization                                                          */
/****************************************************************************/

void qos_cli_init(void)
{
    /* register the size required for qos req. structure */
    cli_req_size_register(sizeof(qos_cli_req_t));
}
#else
#error "Unsupported architecture"
#endif

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
