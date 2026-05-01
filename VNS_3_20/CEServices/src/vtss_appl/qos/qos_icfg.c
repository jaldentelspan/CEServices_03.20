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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "qos_api.h"
#include "qos_icfg.h"
#include "topo_api.h"
#include "misc_api.h"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

#undef IC_RC
#define IC_RC ICLI_VTSS_RC

/*
 *  DCFG implements some code that would have to be dublicated a number of times
 *  'p' is a struct member of c.
 * 'nostr' is the string to be printed if p == default value
 * 'str' is the string to be printed if p != default value
 *
 * If anyone adds the GNU attribute '__attribute__ ((format (__printf__, 2, 3)))'
 * in the definition of vtss_icfg_printf() this macro will emit a lot of
 * 'warning: too many arguments for format' for the 'nostr' parameter.
 *
 * To disable these warnings, you can enable the following pragma:
 * #pragma GCC diagnostic ignored "-Wformat-extra-args"
 */
#undef DCFG
#define DCFG(p, nostr, str, ...) do {                            \
    if (req->all_defaults || (c.p != dc.p)) {                    \
        if (c.p != dc.p) {                                       \
            IC_RC(vtss_icfg_printf(result, str, __VA_ARGS__));   \
        } else {                                                 \
            IC_RC(vtss_icfg_printf(result, nostr, __VA_ARGS__)); \
        }                                                        \
    }                                                            \
} while (0)

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

static vtss_rc QOS_ICFG_global_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    qos_conf_t dc;
    qos_conf_t c;

    IC_RC(qos_conf_get_default(&dc));
    IC_RC(qos_conf_get(&c));

#if defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
    {
        char buf[32];
        (void)qos_icfg_storm_rate_txt(c.policer_uc, buf);
        DCFG(policer_uc_status, "no qos storm unicast\n", "qos storm unicast %s\n", buf);
    }
#endif /* defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH)
    {
        char buf[32];
        (void)qos_icfg_storm_rate_txt(c.policer_mc, buf);
        DCFG(policer_mc_status, "no qos storm multicast\n", "qos storm multicast %s\n", buf);
    }
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) */

#if defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
    {
        char buf[32];
        (void)qos_icfg_storm_rate_txt(c.policer_bc, buf);
        DCFG(policer_bc_status, "no qos storm broadcast\n", "qos storm broadcast %s\n", buf);
    }
#endif /* defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) */

#if defined(VTSS_FEATURE_QOS_WRED)
    {
        int i;
        for (i = 0; i < QOS_PORT_WEIGHTED_QUEUE_CNT; i++) {
            DCFG(wred[i].enable, "no qos wred queue %d\n", "qos wred queue %d min_th %u mdp_1 %u mdp_2 %u mdp_3 %u\n",
                 i, c.wred[i].min_th, c.wred[i].max_prob_1, c.wred[i].max_prob_2, c.wred[i].max_prob_3);
        }
    }
#endif /* VTSS_FEATURE_QOS_WRED */

#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)

#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    {
        int i;
        for (i = 0; i < 64; i++) {
            DCFG(dscp.dscp_trust[i], "no qos map dscp-cos %d\n", "qos map dscp-cos %d cos %lu dpl %u\n",
                 i, c.dscp.dscp_qos_class_map[i], c.dscp.dscp_dp_level_map[i]);
        }
    }
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    {
        int i;
        for (i = 0; i < 64; i++) {
            DCFG(dscp.translate_map[i], "no qos map dscp-ingress-translation %d\n", "qos map dscp-ingress-translation %d to %u\n",
                 i, c.dscp.translate_map[i]);
        }

        for (i = 0; i < 64; i++) {
            DCFG(dscp.ingress_remark[i], "no qos map dscp-classify %d\n", "qos map dscp-classify %d\n", i);
        }

        for (i = 0; i < VTSS_PRIO_ARRAY_SIZE; i++) {
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
            DCFG(dscp.qos_class_dscp_map[i],     "no qos map cos-dscp %d 0\n", "qos map cos-dscp %d 0 %u\n", i, c.dscp.qos_class_dscp_map[i]);
            DCFG(dscp.qos_class_dscp_map_dp1[i], "no qos map cos-dscp %d 1\n", "qos map cos-dscp %d 1 %u\n", i, c.dscp.qos_class_dscp_map_dp1[i]);
#else
            DCFG(dscp.qos_class_dscp_map[i],     "no qos map cos-dscp %d\n",   "qos map cos-dscp %d %u\n",   i, c.dscp.qos_class_dscp_map[i]);
#endif
        }

        for (i = 0; i < 64; i++) {
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
            DCFG(dscp.egress_remap[i],     "no qos map dscp-egress-translation %d 0\n", "qos map dscp-egress-translation %d 0 to %u\n", i, c.dscp.egress_remap[i]);
            DCFG(dscp.egress_remap_dp1[i], "no qos map dscp-egress-translation %d 1\n", "qos map dscp-egress-translation %d 1 to %u\n", i, c.dscp.egress_remap_dp1[i]);
#else
            DCFG(dscp.egress_remap[i],     "no qos map dscp-egress-translation %d\n",   "qos map dscp-egress-translation %d to %u\n",   i, c.dscp.egress_remap[i]);
#endif
        }
    }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
    return VTSS_OK;
}

static vtss_rc QOS_ICFG_port_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    qos_port_conf_t dc;
    qos_port_conf_t c;
    vtss_isid_t     isid = topo_usid2isid(req->instance_id.port.usid);
    vtss_port_no_t  iport = uport2iport(req->instance_id.port.begin_uport);

    IC_RC(qos_port_conf_get_default(&dc));
    IC_RC(qos_port_conf_get(isid, iport, &c));

    DCFG(default_prio,      " no qos cos\n",       " qos cos %lu\n",   c.default_prio);
#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
    DCFG(usr_prio,          " no qos pcp\n",       " qos pcp %lu\n",   c.usr_prio);
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */

#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
    DCFG(default_dpl,       " no qos dpl\n",       " qos dpl %u\n",    c.default_dpl);
#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
    DCFG(default_dei,       " no qos dei\n",       " qos dei %u\n",    c.default_dei);
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if !defined(QOS_USE_FIXED_PCP_QOS_MAP)
    DCFG(tag_class_enable,  " no qos trust tag\n", " qos trust tag\n", "");

    {
        int pcp, dei;
        for (pcp = 0; pcp < VTSS_PCP_ARRAY_SIZE; pcp++) {
            for (dei = 0; dei < VTSS_DEI_ARRAY_SIZE; dei++) {
                if (req->all_defaults || (c.qos_class_map[pcp][dei] != dc.qos_class_map[pcp][dei]) || (c.dp_level_map[pcp][dei] != dc.dp_level_map[pcp][dei])) {
                    if ((c.qos_class_map[pcp][dei] != dc.qos_class_map[pcp][dei]) || (c.dp_level_map[pcp][dei] != dc.dp_level_map[pcp][dei])) {
                        IC_RC(vtss_icfg_printf(result, " qos map tag-cos pcp %d dei %d cos %lu dpl %u\n", pcp, dei, c.qos_class_map[pcp][dei], c.dp_level_map[pcp][dei]));
                    } else {
                        IC_RC(vtss_icfg_printf(result, " no qos map tag-cos pcp %d dei %d\n", pcp, dei));
                    }
                }
            }
        }
    }
#endif /* !defined(QOS_USE_FIXED_PCP_QOS_MAP) */
    DCFG(dscp_class_enable, " no qos trust dscp\n", " qos trust dscp\n", "");
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

    {
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
        BOOL fr = c.port_policer_ext[0].frame_rate;
#else
        BOOL fr = FALSE;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
        BOOL fc = c.port_policer_ext[0].flow_control;
#else
        BOOL fc = FALSE;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */

        DCFG(port_policer[0].enabled, " no qos policer\n", " qos policer %lu %s %s\n", c.port_policer[0].policer.rate, fr ? "fps" : "kbps", fc ? "flowcontrol" : "");
    }

#if defined(VTSS_SW_OPTION_BUILD_CE)
#ifdef VTSS_FEATURE_QOS_QUEUE_POLICER
    {
        int i;
        for (i = 0; i < QOS_PORT_QUEUE_CNT; i++) {
            DCFG(queue_policer[i].enabled, " no qos queue-policer queue %d\n", " qos queue-policer queue %d %lu\n", i, c.queue_policer[i].policer.rate);
        }
    }
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */

    DCFG(shaper_status, " no qos shaper\n", " qos shaper %lu\n", c.shaper_rate);

#ifdef VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS
    {
        int i;
        for (i = 0; i < QOS_PORT_QUEUE_CNT; i++) {
            DCFG(queue_shaper[i].enable, " no qos queue-shaper queue %d\n", " qos queue-shaper queue %d %lu %s\n", i, c.queue_shaper[i].rate, c.excess_enable[i] ? "excess" : "");
        }
    }
#endif /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */

#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
    DCFG(dwrr_enable, " no qos wrr\n", " qos wrr %u %u %u %u %u %u\n", c.queue_pct[0], c.queue_pct[1], c.queue_pct[2], c.queue_pct[3], c.queue_pct[4], c.queue_pct[5]);
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */


#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
    switch (c.tag_remark_mode) {
    case VTSS_TAG_REMARK_MODE_CLASSIFIED:
        DCFG(tag_remark_mode, " no qos tag-remark\n", " qos tag-remark\n", "");
        break;
    case VTSS_TAG_REMARK_MODE_DEFAULT:
        DCFG(tag_remark_mode, " no qos tag-remark\n", " qos tag-remark pcp %lu dei %u\n", c.tag_default_pcp, c.tag_default_dei);
        break;
    case VTSS_TAG_REMARK_MODE_MAPPED: {
        int green;
        for (green = 0; green < QOS_PORT_TR_DPL_CNT; green++) { // count the number of green entries
            if (c.tag_dp_map[green]) {
                break; // break on the first non-green (yellow) entry
            }
        }
        // green is now the number of green entries. 'QOS_PORT_TR_DPL_CNT - green' is the number of yellow entries.
        DCFG(tag_remark_mode, " no qos tag-remark\n", " qos tag-remark mapped yellow %d\n", QOS_PORT_TR_DPL_CNT - green);
        break;
    }
    default:
        break;
    }

    {
        int cos, dpl;
        for (cos = 0; cos < QOS_PORT_PRIO_CNT; cos++) {
            for (dpl = 0; dpl < 2; dpl++) {
                if (req->all_defaults || (c.tag_pcp_map[cos][dpl] != dc.tag_pcp_map[cos][dpl]) || (c.tag_dei_map[cos][dpl] != dc.tag_dei_map[cos][dpl])) {
                    if ((c.tag_pcp_map[cos][dpl] != dc.tag_pcp_map[cos][dpl]) || (c.tag_dei_map[cos][dpl] != dc.tag_dei_map[cos][dpl])) {
                        IC_RC(vtss_icfg_printf(result, " qos map cos-tag cos %d dpl %d pcp %lu dei %u\n", cos, dpl, c.tag_pcp_map[cos][dpl], c.tag_dei_map[cos][dpl]));
                    } else {
                        IC_RC(vtss_icfg_printf(result, " no qos map cos-tag cos %d dpl %d\n", cos, dpl));
                    }
                }
            }
        }
    }
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#ifdef VTSS_FEATURE_QOS_DSCP_REMARK
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    DCFG(dscp_translate, " no qos dscp-translate\n", " qos dscp-translate\n", "");

    switch (c.dscp_imode) {
    case VTSS_DSCP_MODE_NONE:
        DCFG(dscp_imode, " no qos dscp-classify\n", " qos dscp-classify\n", "");
        break;
    case VTSS_DSCP_MODE_ZERO:
        DCFG(dscp_imode, " no qos dscp-classify\n", " qos dscp-classify zero\n", "");
        break;
    case VTSS_DSCP_MODE_SEL:
        DCFG(dscp_imode, " no qos dscp-classify\n", " qos dscp-classify selected\n", "");
        break;
    case VTSS_DSCP_MODE_ALL:
        DCFG(dscp_imode, " no qos dscp-classify\n", " qos dscp-classify any\n", "");
        break;
    default:
        break;
    }

    switch (c.dscp_emode) {
    case VTSS_DSCP_EMODE_DISABLE:
        DCFG(dscp_emode, " no qos dscp-remark\n", " qos dscp-remark\n", "");
        break;
    case VTSS_DSCP_EMODE_REMARK:
        DCFG(dscp_emode, " no qos dscp-remark\n", " qos dscp-remark rewrite\n", "");
        break;
    case VTSS_DSCP_EMODE_REMAP:
        DCFG(dscp_emode, " no qos dscp-remark\n", " qos dscp-remark remap\n", "");
        break;
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
    case VTSS_DSCP_EMODE_REMAP_DPA:
        DCFG(dscp_emode, " no qos dscp-remark\n", " qos dscp-remark remap-dp\n", "");
        break;
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE */
    default:
        break;
    }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
    {
        BOOL fr;

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
        fr = c.port_policer_ext[QOS_STORM_POLICER_UNICAST].frame_rate;
#else
        fr = FALSE;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */

        DCFG(port_policer[QOS_STORM_POLICER_UNICAST].enabled, " no qos storm unicast\n", " qos storm unicast %lu %s\n",
             c.port_policer[QOS_STORM_POLICER_UNICAST].policer.rate, fr ? "fps" : "kbps");

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
        fr = c.port_policer_ext[QOS_STORM_POLICER_BROADCAST].frame_rate;
#else
        fr = FALSE;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */

        DCFG(port_policer[QOS_STORM_POLICER_BROADCAST].enabled, " no qos storm broadcast\n", " qos storm broadcast %lu %s\n",
             c.port_policer[QOS_STORM_POLICER_BROADCAST].policer.rate, fr ? "fps" : "kbps");

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
        fr = c.port_policer_ext[QOS_STORM_POLICER_UNKNOWN].frame_rate;
#else
        fr = FALSE;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */

        DCFG(port_policer[QOS_STORM_POLICER_UNKNOWN].enabled, " no qos storm unknown\n", " qos storm unknown %lu %s\n",
             c.port_policer[QOS_STORM_POLICER_UNKNOWN].policer.rate, fr ? "fps" : "kbps");
    }

#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */

    return VTSS_OK;
}

static vtss_rc QOS_ICFG_qce_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
#if defined(VTSS_FEATURE_QCL_V2)
    vtss_qce_id_t        id = QCE_ID_NONE;
    qos_qce_entry_conf_t conf;

    while (qos_mgmt_qce_entry_get(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, id, &conf, 1) == VTSS_OK) {
        id = conf.id;
        IC_RC(vtss_icfg_printf(result, " qos qce %lu\n", id));
    }
#endif /* VTSS_FEATURE_QCL_V2 */

    return VTSS_OK;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
char *qos_icfg_storm_rate_txt(u32 rate, char *buf)
{
    if (rate >= 1000 && (rate % 1000) == 0) {
        rate /= 1000;
        sprintf(buf, "%u kfps", rate);
    } else {
        sprintf(buf, "%u", rate);
    }
    return buf;
}
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */

/* Initialization function */
vtss_rc qos_icfg_init(void)
{
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_QOS_GLOBAL_CONF, QOS_ICFG_global_conf));
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_QOS_PORT_CONF, QOS_ICFG_port_conf));
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_QOS_QCE_CONF, QOS_ICFG_qce_conf));
    return VTSS_OK;
}
