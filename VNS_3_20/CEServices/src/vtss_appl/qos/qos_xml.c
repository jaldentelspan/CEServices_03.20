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
#include "qos_api.h"

#include "conf_xml_api.h"
#include "conf_xml_trace_def.h"
#include "misc_api.h"
#include "mgmt_api.h"
#if defined(VTSS_FEATURE_QCL_V2)
#include "port_api.h"
#if VTSS_SWITCH_STACKABLE
#include "topo_api.h"
#endif
#endif

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
#include "qos_features.h"

#ifdef  _VTSS_QOS_FEATURES_H_ // This is just a hack in order to keep lint happy :-)
#endif
#endif

#if (VTSS_PORT_POLICERS != 1) && (VTSS_PORT_POLICERS != 4) // Sanity check
#error VTSS_PORT_POLICERS must be set to 1 or 4
#endif

/* Tag IDs */
enum {
    /* Module tags */
    CX_TAG_QOS,

    /* Group tags */
    CX_TAG_PORT_TABLE,
    CX_TAG_QCL_TABLE,
#if defined(VTSS_FEATURE_QOS_WRED)
    CX_TAG_WRED_TABLE,
#endif /* VTSS_FEATURE_QOS_WRED */

    /* Parameter tags */
    CX_TAG_ENTRY,
    CX_TAG_CLASSES,
    CX_TAG_STORM_UNICAST,
    CX_TAG_STORM_MULTICAST,
    CX_TAG_STORM_BROADCAST,

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    CX_TAG_DSCP_TRUST_MODE,
    CX_TAG_DSCP_QOS_CLASS_MAP,
    CX_TAG_DSCP_DPL_MAP,
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    CX_TAG_DSCP_TRANSLATION_MAP,
    CX_TAG_DSCP_CLASS_MODE,
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    CX_TAG_DSCP_CLASS_MAP_DP_0,
    CX_TAG_DSCP_CLASS_MAP_DP_1,
    CX_TAG_DSCP_REMAP_DP_UNAWARE,
    CX_TAG_DSCP_REMAP_DP_AWARE,
#else /* JAG */
    CX_TAG_DSCP_CLASS_MAP,
    CX_TAG_DSCP_REMAP,
#endif /* VTSS_ARCH_LUTON26 */
#endif /* defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

#ifdef VTSS_FEATURE_VSTAX_V2
    CX_TAG_CMEF,
#endif /* VTSS_FEATURE_VSTAX_V2 */

    /* Last entry */
    CX_TAG_NONE
};

#define QOS_BUFF_LEN  256

/* Tag table */
static cx_tag_entry_t qos_cx_tag_table[CX_TAG_NONE + 1] = {
    [CX_TAG_QOS] = {
        .name  = "qos",
        .descr = "Quality of Service",
        .type = CX_TAG_TYPE_MODULE
    },
    [CX_TAG_PORT_TABLE] = {
        .name  = "port_table",
        .descr = "",
        .type = CX_TAG_TYPE_GROUP
    },
    [CX_TAG_QCL_TABLE] = {
        .name  = "qcl_table",
        .descr = "",
        .type = CX_TAG_TYPE_GROUP
    },
#if defined(VTSS_FEATURE_QOS_WRED)
    [CX_TAG_WRED_TABLE] = {
        .name  = "wred_table",
        .descr = "",
        .type = CX_TAG_TYPE_GROUP
    },
#endif /* VTSS_FEATURE_QOS_WRED */
    [CX_TAG_ENTRY] = {
        .name  = "entry",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_CLASSES] = {
        .name  = "classes",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_STORM_UNICAST] = {
        .name  = "storm_unicast",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_STORM_MULTICAST] = {
        .name  = "storm_multicast",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_STORM_BROADCAST] = {
        .name  = "storm_broadcast",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    [CX_TAG_DSCP_TRUST_MODE] = {
        .name  = "dscp_trust",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_DSCP_QOS_CLASS_MAP] = {
        .name  = "dscp_map_qos_class",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_DSCP_DPL_MAP] = {
        .name  = "dscp_map_dpl",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    [CX_TAG_DSCP_TRANSLATION_MAP] = {
        .name  = "dscp_translation_map",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_DSCP_CLASS_MODE] = {
        .name  = "dscp_ingress_classification_mode",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    [CX_TAG_DSCP_CLASS_MAP_DP_0] = {
        .name  = "qos_class_dp_0_map_dscp",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_DSCP_CLASS_MAP_DP_1] = {
        .name  = "qos_class_dp_1_map_dscp",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_DSCP_REMAP_DP_UNAWARE] = {
        .name  = "dscp_egress_remap_dp_unaware_or_dp_0",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_DSCP_REMAP_DP_AWARE] = {
        .name  = "dscp_egress_remap_dp_aware_and_dp_1",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
#else /* JAG */
    [CX_TAG_DSCP_CLASS_MAP] = {
        .name  = "qos_class_map_dscp",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_DSCP_REMAP] = {
        .name  = "dscp_egress_remap",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
#endif /* VTSS_ARCH_LUTON26 */
#endif /* defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

#ifdef VTSS_FEATURE_VSTAX_V2
    [CX_TAG_CMEF] = {
        .name  = "cmef",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
#endif /* VTSS_FEATURE_VSTAX_V2 */

    /* Last entry */
    [CX_TAG_NONE] = {
        .name  = "",
        .descr = "",
        .type = CX_TAG_TYPE_NONE
    }
};

// The pn in cx_pn_xxxx below is a short form of parameter name
#ifdef VTSS_ARCH_LUTON28
static const char *cx_stx_class        = "1-4 or low/normal/medium/high";
static const char *cx_pn_default_class = "default";
static const char *cx_pn_default_pcp   = "tag_prio";
static const char *cx_pn_policer_mode  = "rate_limiter_mode";
static const char *cx_pn_policer_rate  = "rate_limiter_rate";
static const char *cx_pn_epshaper_mode = "shaper_mode";
static const char *cx_pn_epshaper_rate = "shaper_rate";
#else
static const char *cx_stx_class        = "0-7";
static const char *cx_pn_default_class = "def_class";
static const char *cx_pn_default_pcp   = "def_pcp";
static const char *cx_pn_epshaper_mode = "epshaper_mode";         // Egress Port Shaper Mode
static const char *cx_pn_epshaper_rate = "epshaper_rate";         // Egress Port Shaper Rate
static const char *cx_pn_policer_mode  = "ippolicer_mode";        // Ingress Port Policer Mode
static const char *cx_pn_policer_rate  = "ippolicer_rate";        // Ingress Port Policer Rate

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
static const char *cx_pn_policer_unit      = "ippolicer_unit";      // Ingress Port Policer Unit
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL)
static const char *cx_pn_policer_dpbl      = "ippolicer_dpbl";      // Ingress Port Policer DP Bypass Level
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
static const char *cx_pn_policer_unicast   = "ippolicer_unicast";   // Ingress Port Policer Unicast
static const char *cx_pn_policer_multicast = "ippolicer_multicast"; // Ingress Port Policer Multicast
static const char *cx_pn_policer_broadcast = "ippolicer_broadcast"; // Ingress Port Policer Broadcast
static const char *cx_pn_policer_flooding  = "ippolicer_flooding";  // Ingress Port Policer Flooding
static const char *cx_pn_policer_learning  = "ippolicer_learning";  // Ingress Port Policer Learning
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
static const char *cx_pn_policer_fc        = "ippolicer_fc";        // Ingress Port Policer Flow Control
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */

#if (VTSS_PORT_POLICERS == 1)
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
/* Keyword for policer unit */
static const cx_kw_t cx_kw_policer_unit[] = {
    { "kbps", 0 },
    { "fps",  1 },
    { NULL,   0 }
};
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#else
static const char *cx_stx_policer_mode      = "a,b,c,d (ippolicer_mode 0/1)";
static const char *cx_stx_policer_rate      = "a,b,c,d (ippolicer_rate "vtss_xstr(QOS_BITRATE_MIN)".."vtss_xstr(QOS_BITRATE_MAX)")";
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
static const char *cx_stx_policer_unit      = "a,b,c,d (ippolicer_unit 0/1)";
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL)
static const char *cx_stx_policer_dpbl      = "a,b,c,d (ippolicer_dpbl 0..3)";
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
static const char *cx_stx_policer_unicast   = "a,b,c,d (ippolicer_unicast 0/1)";
static const char *cx_stx_policer_multicast = "a,b,c,d (ippolicer_multicast 0/1)";
static const char *cx_stx_policer_broadcast = "a,b,c,d (ippolicer_broadcast 0/1)";
static const char *cx_stx_policer_flooding  = "a,b,c,d (ippolicer_flooding 0/1)";
static const char *cx_stx_policer_learning  = "a,b,c,d (ippolicer_learning 0/1)";
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
static const char *cx_stx_policer_fc        = "a,b,c,d (ippolicer_fc 0/1)";
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#endif /* (VTSS_PORT_POLICERS == 1) */

#endif /* VTSS_ARCH_LUTON28 */

#if defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_QUEUE_POLICER)
static const char *cx_pn_iqpolicer_mode    = "iqpolicer_mode";
static const char *cx_stx_iqpolicer_mode   = "a,b,c,d,e,f,g,h (iqpolicer_mode 0/1)";
static const char *cx_pn_iqpolicer_rate    = "iqpolicer_rate";
static const char *cx_stx_iqpolicer_rate   = "a,b,c,d,e,f,g,h (iqpolicer_rate "vtss_xstr(QOS_BITRATE_MIN)".."vtss_xstr(QOS_BITRATE_MAX)")";
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */

#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
static const char *cx_pn_default_dpl        = "def_dpl";
static const char *cx_pn_default_dei        = "def_dei";
static const char *cx_pn_tag_class          = "tag_class";
static const char *cx_pn_tag_map_class      = "tag_map_class";
static const char *cx_pn_tag_map_dpl        = "tag_map_dpl";
static const char *cx_stx_tag_map_class     = "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p (tag_map_class 0..7)";
static const char *cx_stx_tag_map_dpl       = "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p (tag_map_dpl 0.."vtss_xstr(QOS_DPL_MAX)")";
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
static const char *cx_pn_port_dscp_qos_class     = "dscp_qos_class_mode";
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

#if defined(VTSS_FEATURE_QCL_V1)
static const char *cx_pn_qcl_id             = "qcl_id";
static const char *cx_pn_qce_id             = "qce_id";
#endif /* VTSS_FEATURE_QCL_V1 */

#if defined(VTSS_FEATURE_QCL_V2)
#define CX_ULONG_ANY    0XFFFFFFFF

static const cx_kw_t cx_kw_tag[] = {
    { "any",        VTSS_VCAP_BIT_ANY},
    { "untag",      VTSS_VCAP_BIT_0},
    { "tag",        VTSS_VCAP_BIT_1},
    { NULL,         0}
};

/* Keyword for QCL dmac type */
static const cx_kw_t cx_kw_dmac_type[] = {
    { "any",         QOS_QCE_DMAC_TYPE_ANY},
    { "unicast",     QOS_QCE_DMAC_TYPE_UC},
    { "multicast",   QOS_QCE_DMAC_TYPE_MC},
    { "broadcast",   QOS_QCE_DMAC_TYPE_BC},
    { NULL,          0}
};

static const cx_kw_t cx_kw_fragment[] = {
    { "any",        VTSS_VCAP_BIT_ANY},
    { "no",         VTSS_VCAP_BIT_0},
    { "yes",        VTSS_VCAP_BIT_1},
    { NULL,         0}
};

/* Keyword for ACL frame types */
static const cx_kw_t cx_kw_frame_type[] = {
    { "etype",       VTSS_QCE_TYPE_ETYPE},
    { "any",         VTSS_QCE_TYPE_ANY},
    { "llc",         VTSS_QCE_TYPE_LLC},
    { "snap",        VTSS_QCE_TYPE_SNAP},
    { "ipv4",        VTSS_QCE_TYPE_IPV4},
    { "ipv6",        VTSS_QCE_TYPE_IPV6},
    { NULL,          0}
};
#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)
static const char *cx_pn_eqshaper_mode    = "eqshaper_mode";
static const char *cx_stx_eqshaper_mode   = "a,b,c,d,e,f,g,h (eqshaper_mode 0/1)";
static const char *cx_pn_eqshaper_rate    = "eqshaper_rate";
static const char *cx_stx_eqshaper_rate   = "a,b,c,d,e,f,g,h (eqshaper_rate "vtss_xstr(QOS_BITRATE_MIN)".."vtss_xstr(QOS_BITRATE_MAX)")";
static const char *cx_pn_eqshaper_excess  = "eqshaper_excess";
static const char *cx_stx_eqshaper_excess = "a,b,c,d,e,f,g,h (eqshaper_excess 0/1)";
#endif /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */

#if defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)
/* Keyword for tag remark mode */
static const cx_kw_t cx_kw_tag_remark_mode[] = {
    { "classified", VTSS_TAG_REMARK_MODE_CLASSIFIED },
    { "default",    VTSS_TAG_REMARK_MODE_DEFAULT },
    { "mapped",     VTSS_TAG_REMARK_MODE_MAPPED },
    { NULL,         0 }
};
static const char *cx_pn_tag_remark_mode     = "tr_mode";
static const char *cx_pn_tag_remark_def_pcp  = "tr_def_pcp";
static const char *cx_pn_tag_remark_def_dei  = "tr_def_dei";
static const char *cx_pn_tag_remark_dp_map   = "tr_dp_map";
static const char *cx_stx_tag_remark_dp_map  = "a,b,c,d (tr_dp_map 0..1)";
static const char *cx_pn_tag_remark_pcp_map  = "tr_pcp_map";
static const char *cx_stx_tag_remark_pcp_map = "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p (tr_pcp_map 0..7)";
static const char *cx_pn_tag_remark_dei_map  = "tr_dei_map";
static const char *cx_stx_tag_remark_dei_map = "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p (tr_dei_map 0..1)";
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#if defined(VTSS_FEATURE_QOS_WFQ_PORT) || defined(VTSS_FEATURE_QOS_SCHEDULER_V2)
/* Keyword for scheduler mode */
static const cx_kw_t cx_kw_scheduler_mode[] = {
    { "weighted", 1 },
    { "strict",   0 },
    { NULL,       0 }
};
#if defined(VTSS_FEATURE_QOS_WFQ_PORT)
static const char *cx_pn_scheduler_mode     = "mode";
static const char *cx_pn_scheduler_weight   = "weight";
static const char *cx_stx_scheduler_weight  = "a,b,c,d (weight 1/2/4/8)";
#else
static const char *cx_pn_scheduler_mode     = "sched_mode";
static const char *cx_pn_scheduler_weight   = "sched_weight";
static const char *cx_stx_scheduler_weight  = "a,b,c,d,e,f (sched_weight 1..100)";
#endif
#endif /* defined(VTSS_FEATURE_QOS_WFQ_PORT) || defined(VTSS_FEATURE_QOS_SCHEDULER_V2) */

#ifdef VTSS_FEATURE_QOS_DSCP_REMARK
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
static const char *cx_pn_dscp_remark_mode   = "dscp_remarking_mode";
static const char *cx_pn_dscp_remark_map    = "dscp_queue_mapping";
static const char *cx_stx_dscp_remark_map   = "a,b,c,d (dscp_queue_mapping 00/08/16/24/32/40/48/56/46)";
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
/* Keyword for tag remark mode */
static const char *cx_pn_port_dscp_translate_mode   = "dscp_translate_mode";
static const char *cx_pn_port_dscp_ingress_class_mode  = "dscp_ingress_class_mode";
static const char *cx_pn_port_dscp_egress_remarking_mode  = "dscp_egress_remarking_mode";
static const cx_kw_t cx_kw_port_dscp_ingress_class_mode[] = {
    { "none",     VTSS_DSCP_MODE_NONE },
    { "zero",     VTSS_DSCP_MODE_ZERO },
    { "selected", VTSS_DSCP_MODE_SEL },
    { "all",      VTSS_DSCP_MODE_ALL },
    { NULL,       0 }
};
static const cx_kw_t cx_kw_port_dscp_egress_remarking_mode[] = {
    { "disabled",          VTSS_DSCP_EMODE_DISABLE },
    { "enabled",           VTSS_DSCP_EMODE_REMARK },
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    { "remap_dp_unaware", VTSS_DSCP_EMODE_REMAP },
    { "remap_dp_aware",   VTSS_DSCP_EMODE_REMAP_DPA },
#else
    { "remap", VTSS_DSCP_EMODE_REMAP },
#endif
    { NULL,       0 }
};
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */

/* Add priority */
static vtss_rc cx_add_attr_prio(cx_get_state_t *s, const char *name, vtss_prio_t prio)
{
#ifdef VTSS_ARCH_LUTON28
    return cx_add_attr_txt(s, name, mgmt_prio2txt(prio, 1));
#else
    char buf[8];
    sprintf(buf, "%u", iprio2uprio(prio));
    return cx_add_attr_txt(s, name, buf);
#endif
}

#if defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
#ifdef VTSS_ARCH_LUTON28
static const char *cx_stx_storm_rate = "1/2/4/.../512/1k/2k/4k/.../1024k";
#else
static const char *cx_stx_storm_rate = "1/2/4/.../512/1k/2k/4k/.../32768k";
#endif
static const char *cx_pn_storm_mode     = "mode";
static const char *cx_pn_storm_rate     = "rate";

/* Add QoS storm control */
static vtss_rc cx_add_attr_storm(cx_get_state_t *s, cx_tag_id_t id,
                                 BOOL enable, vtss_packet_rate_t rate)
{
    char buf[8];

    /* Syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_stx_bool(s, cx_pn_storm_mode));
    CX_RC(cx_add_attr_txt(s, cx_pn_storm_rate, cx_stx_storm_rate));
    CX_RC(cx_add_stx_end(s));

    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_bool(s, cx_pn_storm_mode, enable));
    if (rate < 10000) {
        CX_RC(cx_add_attr_ulong(s, cx_pn_storm_rate, rate));
    } else {
        sprintf(buf, "%uk", rate / 1000);
        CX_RC(cx_add_attr_txt(s, cx_pn_storm_rate, buf));
    }
    CX_RC(cx_add_attr_end(s, id));

    return cx_size_check(s);
}
#endif /* VTSS_FEATURE_QOS_POLICER_UC_SWITCH && VTSS_FEATURE_QOS_POLICER_MC_SWITCH && VTSS_FEATURE_QOS_POLICER_BC_SWITCH */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
static const char *cx_pn_dscp  = "val";
static const char *cx_stx_dscp_trust = "0-en,1-dis,2-dis,... (dscp-en/dis, for 64 DSCP)";
static const char *cx_stx_dscp_qos_class_map = "0-a,1-b,2-c,... (dscp-class, for 64 DSCP, valid class: 0..7)";
static const char *cx_stx_dscp_dpl_map = "0-a,1-b,2-c,... (dscp-dpl, for 64 DSCP, valid DP level: 0.."vtss_xstr(QOS_DPL_MAX)")";
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
static const char *cx_stx_dscp_translate_map = "0-a,1-b,2-c,... (old_dscp-translated_dscp, for 64 DSCP)";
static const char *cx_stx_dscp_ingress_class = "0-en,1-dis,2-dis,... (dscp-en/dis, for 64 DSCP)";
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
static const char *cx_stx_qos_class_dp_0_dscp_map = "0-a,1-b,2-c,... (class-dscp, for 8 QoS Class, valid class: 0..7, valid DSCP: 0..63)";
static const char *cx_stx_qos_class_dp_1_dscp_map = "0-a,1-b,2-c,... (class-dscp, for 8 QoS Class, valid class: 0..7, valid DSCP: 0..63)";
static const char *cx_stx_dscp_egress_remap_dp0 = "0-a,1-b,2-c,... (dscp-mapped_dscp, for 64 DSCP)";
static const char *cx_stx_dscp_egress_remap_dp1 = "0-a,1-b,2-c,... (dscp-mapped_dscp, for 64 DSCP)";
#else /* JAG */
static const char *cx_stx_qos_class_dscp_map = "0-a,1-b,2-c,... (class-dscp, for 8 QoS Class, valid class: 0..7, valid DSCP: 0..63)";
static const char *cx_stx_dscp_egress_remap = "0-a,1-b,2-c,... (dscp-mapped_dscp, for 64 DSCP)";
#endif /* VTSS_ARCH_LUTON26 */
#endif /* defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || (defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2))
/* Add QoS DSCP global conf */
static vtss_rc cx_add_attr_dscp(cx_get_state_t *s, qos_dscp_t *conf)
{
    int             dscp, prio;
    char            buf[1024], *p;

#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    /* DSCP trust: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_dscp_trust));
    CX_RC(cx_add_stx_end(s));

    /* DSCP trust */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_TRUST_MODE));
    for (p = &buf[0], dscp = 0; dscp < 64; dscp++) {
        p += sprintf(p, "%s%d-%s", (dscp == 0) ? "" : ",", dscp, conf->dscp_trust[dscp] ? "en" : "dis");
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_TRUST_MODE));

    /* DSCP QoS class map: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_dscp_qos_class_map));
    CX_RC(cx_add_stx_end(s));

    /* DSCP QoS class map */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_QOS_CLASS_MAP));
    for (p = &buf[0], dscp = 0; dscp < 64; dscp++) {
        p += sprintf(p, "%s%d-%u", (dscp == 0) ? "" : ",", dscp, conf->dscp_qos_class_map[dscp]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_QOS_CLASS_MAP));

    /* DSCP DPL map: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_dscp_dpl_map));
    CX_RC(cx_add_stx_end(s));

    /* DSCP DPL map */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_DPL_MAP));
    for (p = &buf[0], dscp = 0; dscp < 64; dscp++) {
        p += sprintf(p, "%s%d-%d", (dscp == 0) ? "" : ",", dscp, conf->dscp_dp_level_map[dscp]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_DPL_MAP));
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

#if defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    /* DSCP translation map: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_dscp_translate_map));
    CX_RC(cx_add_stx_end(s));

    /* DSCP translation map */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_TRANSLATION_MAP));
    for (p = &buf[0], dscp = 0; dscp < 64; dscp++) {
        p += sprintf(p, "%s%d-%d", (dscp == 0) ? "" : ",", dscp, conf->translate_map[dscp]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_TRANSLATION_MAP));

    /* DSCP classification mode: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_dscp_ingress_class));
    CX_RC(cx_add_stx_end(s));

    /* DSCP classification mode */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_CLASS_MODE));
    for (p = &buf[0], dscp = 0; dscp < 64; dscp++) {
        p += sprintf(p, "%s%d-%s", (dscp == 0) ? "" : ",", dscp, conf->ingress_remark[dscp] ? "en" : "dis");
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_CLASS_MODE));

#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    /* DSCP classification map, DP = 0: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_qos_class_dp_0_dscp_map));
    CX_RC(cx_add_stx_end(s));

    /* DSCP classification map, DP = 0 */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_CLASS_MAP_DP_0));
    for (p = &buf[0], prio = 0; prio < VTSS_PRIO_ARRAY_SIZE; prio++) {
        p += sprintf(p, "%s%d-%d", (prio == 0) ? "" : ",", prio, conf->qos_class_dscp_map[prio]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_CLASS_MAP_DP_0));

    /* DSCP classification map, DP = 1: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_qos_class_dp_1_dscp_map));
    CX_RC(cx_add_stx_end(s));

    /* DSCP classification map, DP = 1 */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_CLASS_MAP_DP_1));
    for (p = &buf[0], prio = 0; prio < VTSS_PRIO_ARRAY_SIZE; prio++) {
        p += sprintf(p, "%s%d-%d", (prio == 0) ? "" : ",", prio, conf->qos_class_dscp_map_dp1[prio]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_CLASS_MAP_DP_1));

    /* DSCP Remap, DP unaware: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_dscp_egress_remap_dp0));
    CX_RC(cx_add_stx_end(s));

    /* DSCP remap, DP unaware */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_REMAP_DP_UNAWARE));
    for (p = &buf[0], dscp = 0; dscp < 64; dscp++) {
        p += sprintf(p, "%s%d-%d", (dscp == 0) ? "" : ",", dscp, conf->egress_remap[dscp]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_REMAP_DP_UNAWARE));

    /* DSCP Remap, DP aware: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_dscp_egress_remap_dp1));
    CX_RC(cx_add_stx_end(s));

    /* DSCP remap, DP aware */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_REMAP_DP_AWARE));
    for (p = &buf[0], dscp = 0; dscp < 64; dscp++) {
        p += sprintf(p, "%s%d-%d", (dscp == 0) ? "" : ",", dscp, conf->egress_remap_dp1[dscp]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_REMAP_DP_AWARE));
#else /* JAG */
    /* DSCP classification map: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_qos_class_dscp_map));
    CX_RC(cx_add_stx_end(s));

    /* DSCP classification map */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_CLASS_MAP));
    for (p = &buf[0], prio = 0; prio < VTSS_PRIO_ARRAY_SIZE; prio++) {
        p += sprintf(p, "%s%d-%d", (prio == 0) ? "" : ",", prio, conf->qos_class_dscp_map[prio]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_CLASS_MAP));

    /* DSCP Remap: syntax */
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, cx_stx_dscp_egress_remap));
    CX_RC(cx_add_stx_end(s));

    /* DSCP remap */
    CX_RC(cx_add_attr_start(s, CX_TAG_DSCP_REMAP));
    for (p = &buf[0], dscp = 0; dscp < 64; dscp++) {
        p += sprintf(p, "%s%d-%d", (dscp == 0) ? "" : ",", dscp, conf->egress_remap[dscp]);
    }
    CX_RC(cx_add_attr_txt(s, cx_pn_dscp, buf));
    CX_RC(cx_add_attr_end(s, CX_TAG_DSCP_REMAP));
#endif /* VTSS_ARCH_LUTON26 */
#endif /* defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
    return VTSS_OK;
}
#endif /* defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || (defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)) */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

static BOOL cx_qos_match(const cx_table_context_t *context, ulong port_a, ulong port_b)
{
    qos_port_conf_t conf_a, conf_b;

    if (qos_port_conf_get(context->isid, port_a, &conf_a) != VTSS_OK ||
        qos_port_conf_get(context->isid, port_b, &conf_b) != VTSS_OK) {
        return FALSE;
    }

#if defined(VTSS_FEATURE_QCL_V1)
    /* The default QCL ID is occupied by other module, we store it as
       QCL_ID_START here. The occupied module will set the correct
       value subsequent. */
    if (conf_a.qcl_no > QCL_ID_END) {
        conf_a.qcl_no = QCL_ID_START;
    }
    if (conf_b.qcl_no > QCL_ID_END) {
        conf_b.qcl_no = QCL_ID_START;
    }
#endif /* VTSS_FEATURE_QCL_V1 */

    return (memcmp(&conf_a, &conf_b, sizeof(conf_a)) == 0);
}

static vtss_rc cx_qos_print(cx_get_state_t *s, const cx_table_context_t *context, ulong port_no, char *ports)
{
    qos_port_conf_t conf;

    if (ports == NULL) {
        /* Syntax */
        CX_RC(cx_add_stx_start(s));
        CX_RC(cx_add_stx_port(s));
        CX_RC(cx_add_attr_txt(s, cx_pn_default_class, cx_stx_class));
        CX_RC(cx_add_stx_ulong(s, cx_pn_default_pcp, 0, 7));

#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
        CX_RC(cx_add_stx_ulong(s, cx_pn_default_dpl, 0, QOS_DPL_MAX));
        CX_RC(cx_add_stx_ulong(s, cx_pn_default_dei, 0, 1));
        CX_RC(cx_add_stx_bool(s, cx_pn_tag_class));
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_map_class, cx_stx_tag_map_class));
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_map_dpl, cx_stx_tag_map_dpl));
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
        CX_RC(cx_add_stx_bool(s, cx_pn_port_dscp_qos_class));
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

#if defined(VTSS_FEATURE_QCL_V1)
        CX_RC(cx_add_stx_ulong(s, cx_pn_qcl_id, QCL_ID_START, QCL_ID_END));
#endif /* VTSS_FEATURE_QCL_V1 */

#if (VTSS_PORT_POLICERS == 1)
        CX_RC(cx_add_stx_bool(s, cx_pn_policer_mode));
        CX_RC(cx_add_stx_ulong(s, cx_pn_policer_rate, QOS_BITRATE_MIN, QOS_BITRATE_MAX));
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
        CX_RC(cx_add_stx_kw(s, cx_pn_policer_unit, cx_kw_policer_unit));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
        CX_RC(cx_add_stx_bool(s, cx_pn_policer_fc));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#else
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_mode,      cx_stx_policer_mode));
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_rate,      cx_stx_policer_rate));
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_unit,      cx_stx_policer_unit));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL)
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_dpbl,      cx_stx_policer_dpbl));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_unicast,   cx_stx_policer_unicast));
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_multicast, cx_stx_policer_multicast));
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_broadcast, cx_stx_policer_broadcast));
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_flooding,  cx_stx_policer_flooding));
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_learning,  cx_stx_policer_learning));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_fc,        cx_stx_policer_fc));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#endif /* (VTSS_PORT_POLICERS == 1) */

#if defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_QUEUE_POLICER)
        CX_RC(cx_add_attr_txt(s, cx_pn_iqpolicer_mode, cx_stx_iqpolicer_mode));
        CX_RC(cx_add_attr_txt(s, cx_pn_iqpolicer_rate, cx_stx_iqpolicer_rate));
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */

        CX_RC(cx_add_stx_bool(s, cx_pn_epshaper_mode));
        CX_RC(cx_add_stx_ulong(s, cx_pn_epshaper_rate, QOS_BITRATE_MIN, QOS_BITRATE_MAX));

#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)
        CX_RC(cx_add_attr_txt(s, cx_pn_eqshaper_mode, cx_stx_eqshaper_mode));
        CX_RC(cx_add_attr_txt(s, cx_pn_eqshaper_rate, cx_stx_eqshaper_rate));
        CX_RC(cx_add_attr_txt(s, cx_pn_eqshaper_excess, cx_stx_eqshaper_excess));
#endif /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */

#ifdef VTSS_FEATURE_QOS_WFQ_PORT
        CX_RC(cx_add_stx_kw(s, cx_pn_scheduler_mode, cx_kw_scheduler_mode));
        CX_RC(cx_add_attr_txt(s, cx_pn_scheduler_weight, cx_stx_scheduler_weight));
#endif /* VTSS_FEATURE_QOS_WFQ_PORT */

#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
        CX_RC(cx_add_stx_kw(s, cx_pn_scheduler_mode, cx_kw_scheduler_mode));
        CX_RC(cx_add_attr_txt(s, cx_pn_scheduler_weight, cx_stx_scheduler_weight));
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

#if defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)
        CX_RC(cx_add_stx_kw(s, cx_pn_tag_remark_mode, cx_kw_tag_remark_mode));
        CX_RC(cx_add_stx_ulong(s, cx_pn_tag_remark_def_pcp, 0, 7));
        CX_RC(cx_add_stx_ulong(s, cx_pn_tag_remark_def_dei, 0, 1));
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_remark_dp_map, cx_stx_tag_remark_dp_map));
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_remark_pcp_map, cx_stx_tag_remark_pcp_map));
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_remark_dei_map, cx_stx_tag_remark_dei_map));
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#ifdef VTSS_FEATURE_QOS_DSCP_REMARK
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
        CX_RC(cx_add_stx_bool(s, cx_pn_dscp_remark_mode));
        CX_RC(cx_add_attr_txt(s, cx_pn_dscp_remark_map, cx_stx_dscp_remark_map));
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
        CX_RC(cx_add_stx_bool(s, cx_pn_port_dscp_translate_mode));
        CX_RC(cx_add_stx_kw(s, cx_pn_port_dscp_ingress_class_mode, cx_kw_port_dscp_ingress_class_mode));
        CX_RC(cx_add_stx_kw(s, cx_pn_port_dscp_egress_remarking_mode, cx_kw_port_dscp_egress_remarking_mode));
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */

        return cx_add_stx_end(s);
    }

    CX_RC(qos_port_conf_get(context->isid, port_no, &conf));
    CX_RC(cx_add_port_start(s, CX_TAG_ENTRY, ports));
    CX_RC(cx_add_attr_prio(s, cx_pn_default_class, conf.default_prio));
    CX_RC(cx_add_attr_ulong(s, cx_pn_default_pcp, conf.usr_prio));

#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
    CX_RC(cx_add_attr_ulong(s, cx_pn_default_dpl, conf.default_dpl));
    CX_RC(cx_add_attr_ulong(s, cx_pn_default_dei, conf.default_dei));
    CX_RC(cx_add_attr_bool(s, cx_pn_tag_class, conf.tag_class_enable));
    {
        int             pcp, dei;
        char            buf[32], *p;

        for (p = &buf[0], pcp = 0; pcp < VTSS_PCPS; pcp++) {
            for (dei = 0; dei < VTSS_DEIS; dei++) {
                p += sprintf(p, "%s%u", ((pcp == 0) && (dei == 0)) ? "" : ",", conf.qos_class_map[pcp][dei]);
            }
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_map_class, buf));

        for (p = &buf[0], pcp = 0; pcp < VTSS_PCPS; pcp++) {
            for (dei = 0; dei < VTSS_DEIS; dei++) {
                p += sprintf(p, "%s%u", ((pcp == 0) && (dei == 0)) ? "" : ",", conf.dp_level_map[pcp][dei]);
            }
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_map_dpl, buf));
    }
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    CX_RC(cx_add_attr_bool(s, cx_pn_port_dscp_qos_class, conf.dscp_class_enable));
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

#if defined(VTSS_FEATURE_QCL_V1)
    if (conf.qcl_no > QCL_ID_END) {
        conf.qcl_no = QCL_ID_START;
    }
    CX_RC(cx_add_attr_ulong(s, cx_pn_qcl_id, conf.qcl_no));
#endif /* VTSS_FEATURE_QCL_V1 */

#if (VTSS_PORT_POLICERS == 1)
    CX_RC(cx_add_attr_bool(s, cx_pn_policer_mode, conf.port_policer[0].enabled));
    CX_RC(cx_add_attr_ulong(s, cx_pn_policer_rate, conf.port_policer[0].policer.rate));
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
    CX_RC(cx_add_attr_kw(s, cx_pn_policer_unit, cx_kw_policer_unit, conf.port_policer_ext[0].frame_rate));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
    CX_RC(cx_add_attr_bool(s, cx_pn_policer_fc, conf.port_policer_ext[0].flow_control));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#else
    {
        int             policer;
        char            buf[128], *p;

        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer[policer].enabled ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_mode, buf));

        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%u", policer == 0 ? "" : ",", conf.port_policer[policer].policer.rate);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_rate, buf));

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer_ext[policer].frame_rate ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_unit, buf));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL)
        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer_ext[policer].dp_bypass_level);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_dpbl, buf));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer_ext[policer].unicast ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_unicast, buf));

        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer_ext[policer].multicast ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_multicast, buf));

        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer_ext[policer].broadcast ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_broadcast, buf));

        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer_ext[policer].flooded ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_flooding, buf));

        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer_ext[policer].learning ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_learning, buf));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
        for (p = &buf[0], policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            p += sprintf(p, "%s%d", policer == 0 ? "" : ",", conf.port_policer_ext[policer].flow_control ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_policer_fc, buf));
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
    }
#endif /* (VTSS_PORT_POLICERS == 1) */

#if defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_QUEUE_POLICER)
    {
        int             queue;
        char            buf[128], *p;

        for (p = &buf[0], queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
            p += sprintf(p, "%s%d", queue == 0 ? "" : ",", conf.queue_policer[queue].enabled ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_iqpolicer_mode, buf));

        for (p = &buf[0], queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
            p += sprintf(p, "%s%u", queue == 0 ? "" : ",", conf.queue_policer[queue].policer.rate);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_iqpolicer_rate, buf));
    }
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */

    CX_RC(cx_add_attr_bool(s, cx_pn_epshaper_mode, conf.shaper_status));
    CX_RC(cx_add_attr_ulong(s, cx_pn_epshaper_rate, conf.shaper_rate));

#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)
    {
        int             queue;
        char            buf[128], *p;

        for (p = &buf[0], queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
            p += sprintf(p, "%s%d", queue == 0 ? "" : ",", conf.queue_shaper[queue].enable ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_eqshaper_mode, buf));

        for (p = &buf[0], queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
            p += sprintf(p, "%s%u", queue == 0 ? "" : ",", conf.queue_shaper[queue].rate);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_eqshaper_rate, buf));

        for (p = &buf[0], queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
            p += sprintf(p, "%s%d", queue == 0 ? "" : ",", conf.excess_enable[queue] ? 1 : 0);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_eqshaper_excess, buf));
    }
#endif /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */

#ifdef VTSS_FEATURE_QOS_WFQ_PORT
    {
        vtss_prio_t     prio;
        vtss_weight_t   w;
        char            buf[16], *p;
        CX_RC(cx_add_attr_kw(s, cx_pn_scheduler_mode, cx_kw_scheduler_mode, conf.wfq_enable));
        for (p = &buf[0], prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
            w = conf.weight[prio];
            p += sprintf(p, "%s%u", prio == VTSS_PRIO_START ? "" : ",",
                         w == VTSS_WEIGHT_1 ? 1 : w == VTSS_WEIGHT_2 ? 2 :
                         w == VTSS_WEIGHT_4 ? 4 : w == VTSS_WEIGHT_8 ? 8 : 7);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_scheduler_weight, buf));
    }
#endif /* VTSS_FEATURE_QOS_WFQ_PORT */

#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
    {
        int             queue;
        char            buf[32], *p;
        CX_RC(cx_add_attr_kw(s, cx_pn_scheduler_mode, cx_kw_scheduler_mode, conf.dwrr_enable));
        for (p = &buf[0], queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
            p += sprintf(p, "%s%u", queue == 0 ? "" : ",", conf.queue_pct[queue]);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_scheduler_weight, buf));
    }
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

#if defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)
    CX_RC(cx_add_attr_kw(s, cx_pn_tag_remark_mode, cx_kw_tag_remark_mode, conf.tag_remark_mode));
    CX_RC(cx_add_attr_ulong(s, cx_pn_tag_remark_def_pcp, conf.tag_default_pcp));
    CX_RC(cx_add_attr_ulong(s, cx_pn_tag_remark_def_dei, conf.tag_default_dei));
    {
        int             class, dpl;
        char            buf[32], *p;

        for (p = &buf[0], dpl = 0; dpl < QOS_PORT_TR_DPL_CNT; dpl++) {
            p += sprintf(p, "%s%u", (dpl == 0) ? "" : ",", conf.tag_dp_map[dpl]);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_remark_dp_map, buf));

        for (p = &buf[0], class = 0; class < QOS_PORT_PRIO_CNT; class++) {
            for (dpl = 0; dpl < 2; dpl++) {
                p += sprintf(p, "%s%u", ((class == 0) && (dpl == 0)) ? "" : ",", conf.tag_pcp_map[class][dpl]);
            }
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_remark_pcp_map, buf));

        for (p = &buf[0], class = 0; class < QOS_PORT_PRIO_CNT; class++) {
            for (dpl = 0; dpl < 2; dpl++) {
                p += sprintf(p, "%s%u", ((class == 0) && (dpl == 0)) ? "" : ",", conf.tag_dei_map[class][dpl]);
            }
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_tag_remark_dei_map, buf));
    }
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#ifdef VTSS_FEATURE_QOS_DSCP_REMARK
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
    {
        vtss_prio_t     prio;
        vtss_weight_t   w;
        char            buf[16], *p;
        CX_RC(cx_add_attr_bool(s, cx_pn_dscp_remark_mode, conf.qos_remarking_enable));
        for (p = &buf[0], prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
            w = conf.dscp_remarking_val[prio];
            p += sprintf(p, "%s%02u", prio == VTSS_PRIO_START ? "" : ",", w);
        }
        CX_RC(cx_add_attr_txt(s, cx_pn_dscp_remark_map, buf));
    }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    CX_RC(cx_add_attr_bool(s, cx_pn_port_dscp_translate_mode, conf.dscp_translate));
    CX_RC(cx_add_attr_kw(s, cx_pn_port_dscp_ingress_class_mode, cx_kw_port_dscp_ingress_class_mode, conf.dscp_imode));
    CX_RC(cx_add_attr_kw(s, cx_pn_port_dscp_egress_remarking_mode, cx_kw_port_dscp_egress_remarking_mode, conf.dscp_emode));
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */

    return cx_add_port_end(s, CX_TAG_ENTRY);
}

#if defined(VTSS_FEATURE_QCL_V1)
/* Parse priority list e.g. 1,1,2,2,3,3,4,4 or low,low,normal,normal,medium,medium,high,high */
static vtss_rc cx_parse_class_list(cx_set_state_t *s, char *name, vtss_prio_t *prio)
{
    char buf[64], *p;
    int  i, j, len;

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf) - 1));
    len = strlen(buf);
    buf[len] = ',';
    for (i = 0, j = 0, p = &buf[0]; i <= len && j < 8; i++)
        if (buf[i] == ',') {
            buf[i] = '\0';
            if (mgmt_txt2prio(p, &prio[j]) != VTSS_OK) {
                break;
            }
            j++;
            p = &buf[i + 1];
        }
    if (j != 8) {
        CX_RC(cx_parm_invalid(s));
    }
    return s->rc;
}
#endif /* VTSS_FEATURE_QCL_V1 */

#if defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
/* Parse packet rate */
static vtss_rc cx_parse_packet_rate(cx_set_state_t *s, const char *name, vtss_packet_rate_t *rate)
{
    char buf[32];

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf) - 1));
#ifdef VTSS_ARCH_LUTON28
    if (mgmt_txt2rate(buf, rate, 10) != VTSS_OK) {
#else
    if (mgmt_txt2rate(buf, rate, 15) != VTSS_OK) {
#endif
        CX_RC(cx_parm_invalid(s));
    }
    return s->rc;
}
#endif /* VTSS_FEATURE_QOS_POLICER_UC_SWITCH && VTSS_FEATURE_QOS_POLICER_MC_SWITCH && VTSS_FEATURE_QOS_POLICER_BC_SWITCH */

/* Parse bit rate in kbps */
static vtss_rc cx_parse_bit_rate(cx_set_state_t *s, const char *name, vtss_bitrate_t *rate)
{
    return cx_parse_ulong(s, name, rate, QOS_BITRATE_MIN, QOS_BITRATE_MAX);
}

#if defined(VTSS_FEATURE_QCL_V2)
static vtss_rc cx_parse_portlist_any(cx_set_state_t *s, char *name, BOOL *list)
{
    vtss_rc rc1 = VTSS_OK;
    char    buf[80];
    int     iport;
    BOOL    uport_list[VTSS_PORT_ARRAY_SIZE + 1];

    memset(uport_list, 0, sizeof(uport_list));
    if (cx_parse_word(s, name, "any") == VTSS_OK) {
        for (iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
            list[iport] = TRUE;
        }
    } else if ((rc1 = cx_parse_txt(s, name, buf, sizeof(buf))) == VTSS_OK) {
        if (mgmt_uport_txt2list(buf, uport_list) != VTSS_OK) {
            CX_RC(cx_parm_invalid(s));
        } else {
            for (iport = VTSS_PORT_NO_START; iport < VTSS_PORT_NO_END; iport++) {
                list[iport] = uport_list[iport2uport(iport)];
            }
        }
    }
    return (rc1 == VTSS_OK ? s->rc : rc1);
}

/* Parse ulong or 'any' parameter */
static vtss_rc cx_parse_ulong_any(cx_set_state_t *s, char *name, ulong *val,
                                  ulong min, ulong max)
{
    *val = CX_ULONG_ANY;
    return (cx_parse_any(s, name) == VTSS_OK ? VTSS_OK :
            cx_parse_ulong(s, name, val, min, max));
}

/* Parse IPv4 address or 'any' */
static vtss_rc cx_parse_ipv4_any(cx_set_state_t *s, char *name, vtss_qce_ip_t *ip)
{
    if (cx_parse_any(s, name) == VTSS_OK) {
        ip->value = 0;
        ip->mask = 0;
        return VTSS_OK;
    }
    return cx_parse_ipv4(s, name, &ip->value, &ip->mask, 0);
}

/* Parse IP address or 'any' */
static vtss_rc cx_parse_proto_any(cx_set_state_t *s, char *name, vtss_qce_u8_t *data)
{
    ulong val;

    if (cx_parse_word(s, name, "TCP") == VTSS_OK) {
        data->value = 6;
        data->mask = 0xFF;
        return VTSS_OK;
    }
    if (cx_parse_word(s, name, "UDP") == VTSS_OK) {
        data->value = 17;
        data->mask = 0xFF;
        return VTSS_OK;
    }

    CX_RC(cx_parse_ulong_any(s, name, &val, 0, 255));
    data->value = (val == CX_ULONG_ANY ? 0 : val);
    data->mask = (val == CX_ULONG_ANY ? 0 : 0xFF);
    return VTSS_OK;
}

static vtss_rc cx_parse_vr_u16_any(cx_set_state_t *s, char *name,
                                   qos_qce_vr_u16_t *vr, ulong min, ulong max)
{
    char  buf[32];
    ulong req_min, req_max;

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
    if (cx_parse_any(s, name) == VTSS_OK) {
        vr->in_range = FALSE;
        vr->vr.v.value = 0;
        vr->vr.v.mask = 0;
    } else if (mgmt_txt2range(buf, &req_min, &req_max, min, max) == VTSS_OK) {
        if (req_min == req_max) {
            vr->in_range = FALSE;
            vr->vr.v.value = req_min;
            vr->vr.v.mask = 0xFFFF;
        } else {
            vr->in_range = TRUE;
            vr->vr.r.low = req_min;
            vr->vr.r.high = req_max;
        }
    } else {
        CX_RC(cx_parm_invalid(s));
    }
    return s->rc;
}

static vtss_rc cx_parse_vr_u8_any(cx_set_state_t *s, char *name,
                                  qos_qce_vr_u8_t *vr, ulong min, ulong max)
{
    char  buf[32];
    ulong req_min, req_max;

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
    if (cx_parse_any(s, name) == VTSS_OK) {
        vr->in_range = FALSE;
        vr->vr.v.value = 0;
        vr->vr.v.mask = 0;
    } else if (mgmt_txt2range(buf, &req_min, &req_max, min, max) == VTSS_OK) {
        if (req_min == req_max) {
            vr->in_range = FALSE;
            vr->vr.v.value = req_min;
            vr->vr.v.mask = 0xFF;
        } else {
            vr->in_range = TRUE;
            vr->vr.r.low = req_min;
            vr->vr.r.high = req_max;
        }
    } else {
        CX_RC(cx_parm_invalid(s));
    }
    return s->rc;
}

static vtss_rc cx_parse_pcp(cx_set_state_t *s, char *name, vtss_qce_u8_t *val)
{
    char  buf[32];
    ulong low, high;

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
    if (cx_parse_any(s, name) == VTSS_OK) {
        val->value = 0;
        val->mask = 0;
    } else if (mgmt_txt2range(buf, &low, &high, VTSS_PCP_START,
                              VTSS_PCP_END - 1) == VTSS_OK) {
        if (low == high) {
            val->value = low;
            val->mask = 0xFF;
        } else if ((low == 0 && high == 1) || (low == 2 && high == 3) ||
                   (low == 4 && high == 5) || (low == 6 && high == 7) ||
                   (low == 0 && high == 3) || (low == 4 && high == 7)) {
            val->value = low;
            val->mask = 0xFF & (~((uchar)(high - low)));
        } else {
            CX_RC(cx_parm_invalid(s));
        }
    } else {
        CX_RC(cx_parm_invalid(s));
    }
    return s->rc;
}


static vtss_rc cx_parse_smac_any(cx_set_state_t *s, char *name,
                                 uchar *mac, BOOL *is_any)
{
    char buf[32];
    int  i, j, c;

    if (cx_parse_any(s, name) == VTSS_OK) {
        *is_any = TRUE;
        return VTSS_OK;
    } else {
        *is_any = FALSE;
        CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
        if (strlen(buf) != 8) {
            CX_RC(cx_parm_invalid(s));
        }
        for (i = 0; i < 8; i++) {
            j = (i % 3);
            c = tolower(buf[i]);
            if (j == 2) {
                if (c != '-') {
                    return cx_parm_invalid(s);
                }
            } else {
                if (!isxdigit(c)) {
                    return cx_parm_invalid(s);
                }
                c = (isdigit(c) ? (c - '0') : (10 + c - 'a'));
                if (j == 0) {
                    mac[i / 3] = c * 16;
                } else {
                    mac[i / 3] += c;
                }
            }
        }
    }
    return s->rc;
}
#define CX_QCE_PARM_RC(s, p)    if ((p) == TRUE) { \
                                          CX_RC(cx_parm_invalid(s)); } \
                                      (p) = TRUE;
#define CX_QCE_FRAME_RC(s, e, t) if ((e) != (t)) { \
                                          CX_RC(cx_parm_invalid(s)); }

#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || (defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2))
static vtss_rc qos_parse_dscp(char *buf, int *val, int size, BOOL bool_val)
{
    int indx, n;
    char  *end, *token;
    BOOL  error;

    /* initialize to undefine value */
    for (indx = 0; indx < size; indx++) {
        val[indx] = -1;
    }

    error = (buf == NULL);
    T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
    token = (error ? NULL : strtok(buf, ","));
    while (token) {
        /* Read first part of "a-b" */
        n = strtol(token, &end, 0);
        if (end == token || n < 0 || n >= size) {
            error = 1;
            break;
        }
        token = end;

        if (*token != '-') {
            error = 1;
            break;
        }
        indx = n;
        token++;

        /* Read last part of "a-b" */
        if (!bool_val) {
            n = strtol(token, &end, 0);
            if (end == token) {
                error = 1;
                break;
            }
        } else {
            /* boolean can be "en" or "dis" */
            if (!strcasecmp(token, "en")) {
                n = TRUE;
            } else if (!strcasecmp(token, "dis")) {
                n = FALSE;
            } else {
                error = 1;
                break;
            }
        }
        val[indx] = n;

        token = strtok(NULL, ",");
    }
    return (error ? VTSS_UNSPECIFIED_ERROR : VTSS_OK);
}
#endif /* defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || (defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)) */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

static vtss_rc qos_cx_parse_func(cx_set_state_t *s)
{
    switch (s->cmd) {
    case CX_PARSE_CMD_PARM: {
        vtss_port_no_t       port_idx;
        BOOL                 port_list[VTSS_PORT_ARRAY_SIZE + 1];
        ulong                val;
        BOOL                 global;
        qos_conf_t           conf;
        qos_qce_entry_conf_t qce;
#if defined(VTSS_FEATURE_QCL_V1)
        vtss_qcl_id_t        qcl_id;
#endif /* VTSS_FEATURE_QCL_V1 */
#if defined(VTSS_FEATURE_QCL_V2)
        qos_qce_vr_u16_t     vr_u16;
        qos_qce_vr_u8_t      vr_u8 = {};
        uchar                smac[3];
        BOOL                 is_any;
        vtss_qce_ip_t        sip;
        vtss_qce_u8_t        proto, pcp_val = {};
#if VTSS_SWITCH_STACKABLE
        BOOL    isid_done;
#endif
        BOOL    port_done, tag_done, vid_done, pcp_done, dei_done;
        BOOL    smac_done, dmac_type_done, frame_type_done;
        BOOL    etype_done, dsap_done, ssap_done, control_done, pid_done;
        BOOL    proto_done, sip_done, fragment_done, dscp_done;
        BOOL    sport_done, dport_done, sipv6_done;
        BOOL    class_done, dp_done, cls_dscp_done;
#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) ||\
(VTSS_PORT_POLICERS != 1)                       ||\
defined(VTSS_FEATURE_QOS_WFQ_PORT)              ||\
defined(VTSS_FEATURE_QOS_SCHEDULER_V2)          ||\
(defined(VTSS_SW_OPTION_BUILD_CE) && defined(VTSS_FEATURE_QOS_QUEUE_POLICER)) ||\
defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)  ||\
defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)         ||\
(defined(VTSS_FEATURE_QOS_DSCP_REMARK) && (defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)) && (defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)))
        char buf[512];
#endif
#if (defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)) && (defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || (defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)))
        int value[64];
        int indx;
#endif

        T_DG(VTSS_TRACE_GRP_QOS, "CX_PARSE_CMD_PARM");
        global = (s->isid == VTSS_ISID_GLOBAL);
        if (global && s->apply && qos_conf_get(&conf) != VTSS_OK) {
            break;
        }

        switch (s->id) {
#ifdef VTSS_ARCH_LUTON28
        case CX_TAG_CLASSES:
            if (global &&
                cx_parse_val_ulong(s, &val, 1, VTSS_PRIOS) == VTSS_OK) {
                conf.prio_no = val;
                if (val == 3) {
                    CX_RC(cx_parm_invalid(s));
                }
            } else {
                s->ignored = 1;
            }
            break;
#endif /* VTSS_ARCH_LUTON28 */
#if defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
        case CX_TAG_STORM_UNICAST:
        case CX_TAG_STORM_MULTICAST:
        case CX_TAG_STORM_BROADCAST:
            if (global) {
                BOOL               *mode;
                vtss_packet_rate_t *rate;
                if (s->id == CX_TAG_STORM_UNICAST) {
                    mode = &conf.policer_uc_status;
                    rate = &conf.policer_uc;
                } else if (s->id == CX_TAG_STORM_MULTICAST) {
                    mode = &conf.policer_mc_status;
                    rate = &conf.policer_mc;
                } else {
                    mode = &conf.policer_bc_status;
                    rate = &conf.policer_bc;
                }
                for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                    if (cx_parse_bool(s, cx_pn_storm_mode, mode, 1) != VTSS_OK &&
                        cx_parse_packet_rate(s, cx_pn_storm_rate, rate) != VTSS_OK) {
                        CX_RC(cx_parm_unknown(s));
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
#endif /* VTSS_FEATURE_QOS_POLICER_UC_SWITCH && VTSS_FEATURE_QOS_POLICER_MC_SWITCH && VTSS_FEATURE_QOS_POLICER_BC_SWITCH */

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
        case CX_TAG_DSCP_TRUST_MODE:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, 64, 1) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < 64; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        conf.dscp.dscp_trust[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_DSCP_QOS_CLASS_MAP:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, 64, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < 64; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > (QOS_CLASS_CNT - 1)) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.dscp_qos_class_map[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_DSCP_DPL_MAP:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, 64, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < 64; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > QOS_DPL_MAX) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.dscp_dp_level_map[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
        case CX_TAG_DSCP_TRANSLATION_MAP:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, 64, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < 64; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > 64) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.translate_map[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_DSCP_CLASS_MODE:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, 64, 1) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < 64; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        conf.dscp.ingress_remark[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
        case CX_TAG_DSCP_CLASS_MAP_DP_0:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, VTSS_PRIO_ARRAY_SIZE, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < VTSS_PRIO_ARRAY_SIZE; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > 64) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.qos_class_dscp_map[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_DSCP_CLASS_MAP_DP_1:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, VTSS_PRIO_ARRAY_SIZE, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < VTSS_PRIO_ARRAY_SIZE; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > 64) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.qos_class_dscp_map_dp1[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_DSCP_REMAP_DP_UNAWARE:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, 64, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < 64; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > 64) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.egress_remap[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_DSCP_REMAP_DP_AWARE:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, 64, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < 64; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > 64) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.egress_remap_dp1[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
#else /* JAG */
        case CX_TAG_DSCP_CLASS_MAP:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, VTSS_PRIO_ARRAY_SIZE, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < VTSS_PRIO_ARRAY_SIZE; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > 64) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.qos_class_dscp_map[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_DSCP_REMAP:
            if (global) {
                if (cx_parse_txt(s, cx_pn_dscp, buf, sizeof(buf) - 1) == VTSS_OK) {
                    if (qos_parse_dscp(buf, value, 64, 0) != VTSS_OK) {
                        CX_RC(cx_parm_invalid(s));
                        break;
                    }
                    for (indx = 0; indx < 64; indx++) {
                        /* check if the DSCP is mentioned */
                        if (value[indx] < 0) {
                            continue;
                        }
                        if (value[indx] > 64) {
                            T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %d", indx, value[indx]);
                            CX_RC(cx_parm_invalid(s));
                            break;
                        }
                        conf.dscp.egress_remap[indx] = value[indx];
                    }
                }
            } else {
                s->ignored = 1;
            }
            break;
#endif /* VTSS_ARCH_LUTON26 */
#endif /* defined(VTSS_FEATURE_QOS_DSCP_REMARK) && defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */

#ifdef VTSS_FEATURE_VSTAX_V2
        case CX_TAG_CMEF:
            if (global) {
                BOOL enabled;
                CX_RC(cx_parse_val_bool(s, &enabled, 1));
                conf.cmef_disable = !enabled;
            } else {
                s->ignored = 1;
            }
            break;
#endif /* VTSS_FEATURE_VSTAX_V2 */

#if defined(VTSS_FEATURE_QOS_WRED)
        case CX_TAG_WRED_TABLE:
            if (!global) {
                s->ignored = 1;
            }
            break;
#endif /* VTSS_FEATURE_QOS_WRED */

        case CX_TAG_QCL_TABLE:
            if (global) {
#if defined(VTSS_FEATURE_QCL_V1)
                /* Flush QCL table */
                for (qcl_id = QCL_ID_START; s->apply && qcl_id <= QCL_ID_END; qcl_id++) {
                    while (qos_mgmt_qce_entry_get(qcl_id, QCE_ID_NONE, &qce, 1) == VTSS_OK) {
                        CX_RC(qos_mgmt_qce_entry_del(qcl_id, qce.id));
                    }
                }
#endif /* VTSS_FEATURE_QCL_V1 */
#if defined(VTSS_FEATURE_QCL_V2)
                /* Flush QCL table */
                while (qos_mgmt_qce_entry_get(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, QCE_ID_NONE, &qce, 1) == VTSS_OK) {
                    CX_RC(qos_mgmt_qce_entry_del(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, qce.id));
                }
#endif /* VTSS_FEATURE_QCL_V2 */
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_PORT_TABLE:
            if (global) {
                s->ignored = 1;
            }
            break;
        case CX_TAG_ENTRY:
            if (global && s->group == CX_TAG_QCL_TABLE) {
#if defined(VTSS_FEATURE_QCL_V1)
                vtss_prio_t prio = VTSS_PRIO_END;

                qcl_id = QCL_ID_NONE;
                qce.id = QCL_ID_NONE;
                qce.type = 100; /* Unknown type */
                for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                    if (cx_parse_ulong(s, cx_pn_qcl_id, &val, QCL_ID_START, QCL_ID_END) == VTSS_OK) {
                        qcl_id = val;
                    } else if (cx_parse_ulong(s, cx_pn_qce_id, &val, QCE_ID_START, QCE_ID_END) == VTSS_OK) {
                        qce.id = val;
                    } else if (cx_parse_ulong(s, "etype", &val, 0x0600, 0xffff) == VTSS_OK) {
                        qce.type = VTSS_QCE_TYPE_ETYPE;
                        qce.frame.etype.val = val;
                    } else if (cx_parse_vid(s, &qce.frame.vlan.vid, 0) == VTSS_OK) {
                        qce.type = VTSS_QCE_TYPE_VLAN;
                    } else if (cx_parse_udp_tcp(s, "udp_tcp_port", &qce.frame.udp_tcp.low, &qce.frame.udp_tcp.high) == VTSS_OK) {
                        qce.type = VTSS_QCE_TYPE_UDP_TCP;
                    } else if (cx_parse_ulong(s, "dscp", &val, 0, 63) == VTSS_OK) {
                        qce.type = VTSS_QCE_TYPE_DSCP;
                        qce.frame.dscp.dscp_val = val;
                    } else if (cx_parse_class_list(s, "tos_class", qce.frame.tos_prio) == VTSS_OK) {
                        qce.type = VTSS_QCE_TYPE_TOS;
                    } else if (cx_parse_class_list(s, "tag_class", qce.frame.tag_prio) == VTSS_OK) {
                        qce.type = VTSS_QCE_TYPE_TAG;
                    } else if (cx_parse_class(s, "class", &prio) != VTSS_OK) {
                        CX_RC(cx_parm_unknown(s));
                    }
                }
                if (s->rc != VTSS_OK) {
                    break;
                } else if (qcl_id == QCL_ID_NONE) {
                    CX_RC(cx_parm_found_error(s, cx_pn_qcl_id));
                } else if (qce.id == QCE_ID_NONE) {
                    CX_RC(cx_parm_found_error(s, cx_pn_qcl_id));
                } else {
                    switch (qce.type) {
                    case VTSS_QCE_TYPE_ETYPE:
                        qce.frame.etype.prio = prio;
                        break;
                    case VTSS_QCE_TYPE_VLAN:
                        qce.frame.vlan.prio = prio;
                        break;
                    case VTSS_QCE_TYPE_UDP_TCP:
                        qce.frame.udp_tcp.prio = prio;
                        break;
                    case VTSS_QCE_TYPE_DSCP:
                        qce.frame.dscp.prio = prio;
                        break;
                    case VTSS_QCE_TYPE_TOS:
                    case VTSS_QCE_TYPE_TAG:
                        prio = VTSS_PRIO_START;
                        break;
                    default:
                        CX_RC(cx_parm_error(s, "Unknown QCE type"));
                        break;
                    }
                    if (prio == VTSS_PRIO_END) {
                        CX_RC(cx_parm_found_error(s, "class"));
                    }
                }
                if (s->apply) {
                    CX_RC(qos_mgmt_qce_entry_add(qcl_id, QCE_ID_NONE, &qce));
                }
#endif /* VTSS_FEATURE_QCL_V1 */
#if defined(VTSS_FEATURE_QCL_V2)
                if (cx_parse_ulong(s, "qce_id", &val, QCE_ID_START, QCE_ID_END) == VTSS_OK) {
                    /* Initialize QCE before parsing parameters */
                    memset(&qce, 0, sizeof(qce));
                    qce.id = val;
                    qce.type = VTSS_QCE_TYPE_ANY;
                    qce.isid = VTSS_ISID_GLOBAL;
#if VTSS_SWITCH_STACKABLE
                    isid_done = FALSE;
#endif
                    port_done = tag_done = vid_done = pcp_done = dei_done = FALSE;
                    smac_done = dmac_type_done = frame_type_done = FALSE;
                    etype_done = dsap_done = ssap_done = control_done = pid_done = FALSE;
                    proto_done = sip_done = fragment_done = dscp_done = FALSE;
                    sport_done = dport_done = sipv6_done = FALSE;
                    class_done = dp_done = cls_dscp_done = FALSE;
                    s->p = s->next;
                    for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                        if (cx_parse_portlist_any(s, "port", port_list) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, port_done);
                            for (port_idx = VTSS_PORT_NO_START; port_idx < VTSS_PORT_NO_END; port_idx++) {
                                VTSS_PORT_BF_SET(qce.port_list, port_idx, port_list[port_idx]);
                            }
#if VTSS_SWITCH_STACKABLE
                        } else if (cx_parse_ulong_any(s, "sid", &val, VTSS_USID_START, VTSS_USID_END - 1) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, isid_done);
                            qce.isid = (val == CX_ULONG_ANY ? VTSS_ISID_GLOBAL :
                                        topo_usid2isid(val));
#endif /* VTSS_SWITCH_STACKABLE */
                        } else if (cx_parse_kw(s, "tag", cx_kw_tag, &val, 1) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, tag_done);
                            QCE_ENTRY_CONF_KEY_SET(qce.key.key_bits, QOS_QCE_VLAN_TAG, val);
                        } else if (cx_parse_vr_u16_any(s, "vid", &vr_u16, 1, 4095) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, vid_done);
                            qce.key.vid = vr_u16;
                        } else if (cx_parse_pcp(s, "pcp", &pcp_val) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, pcp_done);
                            qce.key.pcp = pcp_val;
                        } else if (cx_parse_ulong_any(s, "dei", &val, VTSS_DEI_START, VTSS_DEI_END - 1) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, dei_done);
                            QCE_ENTRY_CONF_KEY_SET(qce.key.key_bits, QOS_QCE_VLAN_DEI, (val == CX_ULONG_ANY ? VTSS_VCAP_BIT_ANY : val + 1));
                        } else if (cx_parse_smac_any(s, "smac", smac, &is_any) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, smac_done);
                            if (is_any) {
                                qce.key.smac.mask[0] = qce.key.smac.mask[1] = qce.key.smac.mask[2] = 0;
                                qce.key.smac.value[0] = qce.key.smac.value[1] = qce.key.smac.value[2] = 0;
                            } else {
                                qce.key.smac.value[0] = smac[0];
                                qce.key.smac.value[1] = smac[1];
                                qce.key.smac.value[2] = smac[2];
                                qce.key.smac.mask[0] = qce.key.smac.mask[1] = qce.key.smac.mask[2] = 0xFF;
                            }
                        } else if (cx_parse_kw(s, "dmac_type", cx_kw_dmac_type, &val, 1) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, dmac_type_done);
                            QCE_ENTRY_CONF_KEY_SET(qce.key.key_bits, QOS_QCE_DMAC_TYPE, val);
                        } else if (cx_parse_kw(s, "frame", cx_kw_frame_type, &val, 1) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, frame_type_done);
                            qce.type = val;
                        } else if (cx_parse_ulong_any(s, "etype", &val, 0x0600,
                                                      0xFFFF) == VTSS_OK) {
                            CX_QCE_FRAME_RC(s, qce.type, VTSS_QCE_TYPE_ETYPE);
                            CX_QCE_PARM_RC(s, etype_done);
                            if (val == CX_ULONG_ANY) {
                                qce.key.frame.etype.etype.value[0] = 0;
                                qce.key.frame.etype.etype.value[1] = 0;
                                qce.key.frame.etype.etype.mask[0] = 0;
                                qce.key.frame.etype.etype.mask[1] = 0;
                            } else {
                                qce.key.frame.etype.etype.value[0] = (val >> 8) & 0xFF;
                                qce.key.frame.etype.etype.value[1] = val & 0xFF;
                                qce.key.frame.etype.etype.mask[0] = 0xFF;
                                qce.key.frame.etype.etype.mask[1] = 0xFF;
                            }
                        } else if (cx_parse_ulong_any(s, "dsap", &val, 0x00,
                                                      0xFF) == VTSS_OK) {
                            CX_QCE_FRAME_RC(s, qce.type, VTSS_QCE_TYPE_LLC);
                            CX_QCE_PARM_RC(s, dsap_done);
                            if (val == CX_ULONG_ANY) {
                                qce.key.frame.llc.dsap.value = 0;
                                qce.key.frame.llc.dsap.mask = 0;
                            } else {
                                qce.key.frame.llc.dsap.value = val;
                                qce.key.frame.llc.dsap.mask = 0xFF;
                            }
                        } else if (cx_parse_ulong_any(s, "ssap", &val, 0x00,
                                                      0xFF) == VTSS_OK) {
                            CX_QCE_FRAME_RC(s, qce.type, VTSS_QCE_TYPE_LLC);
                            CX_QCE_PARM_RC(s, ssap_done);
                            if (val == CX_ULONG_ANY) {
                                qce.key.frame.llc.ssap.value = 0;
                                qce.key.frame.llc.ssap.mask = 0;
                            } else {
                                qce.key.frame.llc.ssap.value = val;
                                qce.key.frame.llc.ssap.mask = 0xFF;
                            }
                        } else if (cx_parse_ulong_any(s, "control", &val, 0x00,
                                                      0xFF) == VTSS_OK) {
                            CX_QCE_FRAME_RC(s, qce.type, VTSS_QCE_TYPE_LLC);
                            CX_QCE_PARM_RC(s, control_done);
                            if (val == CX_ULONG_ANY) {
                                qce.key.frame.llc.control.value = 0;
                                qce.key.frame.llc.control.mask = 0;
                            } else {
                                qce.key.frame.llc.control.value = val;
                                qce.key.frame.llc.control.mask = 0xFF;
                            }
                        } else if (cx_parse_ulong_any(s, "pid", &val, 0x00,
                                                      0xFFFF) == VTSS_OK) {
                            CX_QCE_FRAME_RC(s, qce.type, VTSS_QCE_TYPE_SNAP);
                            CX_QCE_PARM_RC(s, pid_done);
                            if (val == CX_ULONG_ANY) {
                                qce.key.frame.snap.pid.value[0] = 0;
                                qce.key.frame.snap.pid.value[1] = 0;
                                qce.key.frame.snap.pid.mask[0] = 0;
                                qce.key.frame.snap.pid.mask[1] = 0;
                            } else {
                                qce.key.frame.snap.pid.value[0] = (val >> 8) & 0xFF;
                                qce.key.frame.snap.pid.value[1] = val & 0xFF;
                                qce.key.frame.snap.pid.mask[0] = 0xFF;
                                qce.key.frame.snap.pid.mask[1] = 0xFF;
                            }
                        } else if (cx_parse_proto_any(s, "protocol", &proto) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, proto_done);
                            if (qce.type != VTSS_QCE_TYPE_IPV4 &&
                                qce.type != VTSS_QCE_TYPE_IPV6) {
                                CX_RC(cx_parm_invalid(s));
                            }
                            if (qce.type == VTSS_QCE_TYPE_IPV4) {
                                qce.key.frame.ipv4.proto = proto;
                            } else {
                                qce.key.frame.ipv6.proto = proto;
                            }
                        } else if (cx_parse_kw(s, "fragment", cx_kw_fragment,
                                               &val, 1) == VTSS_OK) {
                            CX_QCE_FRAME_RC(s, qce.type, VTSS_QCE_TYPE_IPV4);
                            CX_QCE_PARM_RC(s, fragment_done);
                            QCE_ENTRY_CONF_KEY_SET(qce.key.key_bits, QOS_QCE_IPV4_FRAGMENT, val);
                        } else if (cx_parse_ipv4_any(s, "sip", &sip) == VTSS_OK) {
                            CX_QCE_FRAME_RC(s, qce.type, VTSS_QCE_TYPE_IPV4);
                            CX_QCE_PARM_RC(s, sip_done);
                            qce.key.frame.ipv4.sip = sip;
                        } else if (cx_parse_vr_u8_any(s, "dscp", &vr_u8, 0, 63) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, dscp_done);
                            if (qce.type != VTSS_QCE_TYPE_IPV4 &&
                                qce.type != VTSS_QCE_TYPE_IPV6) {
                                CX_RC(cx_parm_invalid(s));
                            }
                            if (qce.type == VTSS_QCE_TYPE_IPV4) {
                                qce.key.frame.ipv4.dscp = vr_u8;
                            } else {
                                qce.key.frame.ipv6.dscp = vr_u8;
                            }
                        } else if (cx_parse_vr_u16_any(s, "sport", &vr_u16, 0x00, 0xFFFF) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, sport_done);
                            if (qce.type != VTSS_QCE_TYPE_IPV4 &&
                                qce.type != VTSS_QCE_TYPE_IPV6) {
                                CX_RC(cx_parm_invalid(s));
                            }
                            if (qce.type == VTSS_QCE_TYPE_IPV4) {
                                qce.key.frame.ipv4.sport = vr_u16;
                            } else {
                                qce.key.frame.ipv6.sport = vr_u16;
                            }
                        } else if (cx_parse_vr_u16_any(s, "dport", &vr_u16, 0x00, 0xFFFF) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, dport_done);
                            if (qce.type != VTSS_QCE_TYPE_IPV4 &&
                                qce.type != VTSS_QCE_TYPE_IPV6) {
                                CX_RC(cx_parm_invalid(s));
                            }
                            if (qce.type == VTSS_QCE_TYPE_IPV4) {
                                qce.key.frame.ipv4.dport = vr_u16;
                            } else {
                                qce.key.frame.ipv6.dport = vr_u16;
                            }
                        } else if (cx_parse_ipv4_any(s, "sipv6", &sip) == VTSS_OK) {
                            CX_QCE_FRAME_RC(s, qce.type, VTSS_QCE_TYPE_IPV6);
                            CX_QCE_PARM_RC(s, sipv6_done);
                            qce.key.frame.ipv6.sip.value[0] = (sip.value >> 24) & 0xFF;
                            qce.key.frame.ipv6.sip.value[1] = (sip.value >> 16) & 0xFF;
                            qce.key.frame.ipv6.sip.value[2] = (sip.value >> 8) & 0xFF;
                            qce.key.frame.ipv6.sip.value[3] = sip.value & 0xFF;
                            qce.key.frame.ipv6.sip.mask[0] = (sip.mask >> 24) & 0xFF;
                            qce.key.frame.ipv6.sip.mask[1] = (sip.mask >> 16) & 0xFF;
                            qce.key.frame.ipv6.sip.mask[2] = (sip.mask >> 8) & 0xFF;
                            qce.key.frame.ipv6.sip.mask[3] = sip.mask & 0xFF;
                        } else if (cx_parse_ulong(s, "class", &val, 0,
                                                  QOS_CLASS_CNT - 1) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, class_done);
                            QCE_ENTRY_CONF_ACTION_SET(qce.action.action_bits,
                                                      QOS_QCE_ACTION_PRIO, 1);
                            qce.action.prio = val;
                        } else if (cx_parse_ulong(s, "dp", &val, 0,
                                                  QOS_DPL_MAX) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, dp_done);
                            QCE_ENTRY_CONF_ACTION_SET(qce.action.action_bits,
                                                      QOS_QCE_ACTION_DP, 1);
                            qce.action.dp = val;
                        } else if (cx_parse_ulong(s, "classified_dscp", &val, 0, 63) == VTSS_OK) {
                            CX_QCE_PARM_RC(s, cls_dscp_done);
                            QCE_ENTRY_CONF_ACTION_SET(qce.action.action_bits,
                                                      QOS_QCE_ACTION_DSCP, 1);
                            qce.action.dscp = val;
                        }
                    }

                    if (!class_done && !dp_done && !cls_dscp_done) {
                        CX_RC(cx_parm_found_error(s, "Action Parameter"));
                    }

                    if (s->apply) {
                        CX_RC(qos_mgmt_qce_entry_add(QCL_USER_STATIC, QCL_ID_END, QCE_ID_NONE, &qce));
                    }
                }
#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_FEATURE_QOS_WRED)
            } else if (global && s->group == CX_TAG_WRED_TABLE) {
                BOOL found_queue = FALSE, found_mode = FALSE, found_min_th = FALSE, found_max_dp1 = FALSE, found_max_dp2 = FALSE, found_max_dp3 = FALSE, found_errors = FALSE;
                ulong queue   = 0;
                BOOL  mode    = FALSE;
                ulong min_th  = 0;
                ulong max_dp1 = 0;
                ulong max_dp2 = 0;
                ulong max_dp3 = 0;

                for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                    if (cx_parse_ulong(s, "queue", &queue, 0, QOS_PORT_WEIGHTED_QUEUE_CNT - 1) == VTSS_OK) {
                        found_queue = TRUE;
                    } else if (cx_parse_bool(s, "mode", &mode, 1) == VTSS_OK) {
                        found_mode = TRUE;
                    } else if (cx_parse_ulong(s, "min_th", &min_th, 0, 100) == VTSS_OK) {
                        found_min_th = TRUE;
                    } else if (cx_parse_ulong(s, "max_dp1", &max_dp1, 0, 100) == VTSS_OK) {
                        found_max_dp1 = TRUE;
                    } else if (cx_parse_ulong(s, "max_dp2", &max_dp2, 0, 100) == VTSS_OK) {
                        found_max_dp2 = TRUE;
                    } else if (cx_parse_ulong(s, "max_dp3", &max_dp3, 0, 100) == VTSS_OK) {
                        found_max_dp3 = TRUE;
                    } else {
                        CX_RC(cx_parm_unknown(s));
                        found_errors = TRUE;
                    }
                }
                if (found_queue && !found_errors) {
                    if (found_mode) {
                        conf.wred[queue].enable = mode;
                    }
                    if (found_min_th) {
                        conf.wred[queue].min_th = min_th;
                    }
                    if (found_max_dp1) {
                        conf.wred[queue].max_prob_1 = max_dp1;
                    }
                    if (found_max_dp2) {
                        conf.wred[queue].max_prob_2 = max_dp2;
                    }
                    if (found_max_dp3) {
                        conf.wred[queue].max_prob_3 = max_dp3;
                    }
                }
#endif /* VTSS_FEATURE_QOS_WRED */

            } else if (!global && s->group == CX_TAG_PORT_TABLE &&
                       cx_parse_ports(s, port_list, 1) == VTSS_OK) {
                qos_port_conf_t new, qos_port_conf;
                BOOL            class_seen = 0, pcp_seen = 0, p_mode_seen = 0, p_rate_seen = 0, s_mode_seen = 0, s_rate_seen = 0;
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
                BOOL            p_unit_seen = 0;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL)
                BOOL            p_dpbl_seen = 0;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
                BOOL            p_unicast_seen = 0, p_multicast_seen = 0, p_broadcast_seen = 0, p_flooding_seen = 0, p_learning_seen = 0;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
                BOOL            p_fc_seen = 0;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#if defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_QUEUE_POLICER)
                BOOL            qp_mode_seen = 0, qp_rate_seen = 0;
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */
#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
                BOOL            dpl_seen = 0, dei_seen = 0, tag_class_seen = 0, tag_map_class_seen = 0, tag_map_dpl_seen = 0;
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
                BOOL            dscp_class_seen = 0;
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QCL_V1)
                BOOL            qcl_seen = 0;
#endif /* VTSS_FEATURE_QCL_V1 */
#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)
                BOOL            qs_mode_seen = 0, qs_rate_seen = 0, qs_excess_seen = 0;
#endif /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */
#if defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)
                BOOL            tr_mode_seen = 0, tr_def_pcp_seen = 0, tr_def_dei_seen = 0, tr_dp_map_seen = 0, tr_pcp_map_seen = 0, tr_dei_map_seen = 0;
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */
#if defined(VTSS_FEATURE_QOS_WFQ_PORT) || defined(VTSS_FEATURE_QOS_SCHEDULER_V2)
                BOOL            sched_mode_seen = 0, sched_weight_seen = 0;
#endif /* defined(VTSS_FEATURE_QOS_WFQ_PORT) || defined(VTSS_FEATURE_QOS_SCHEDULER_V2) */
#ifdef VTSS_FEATURE_QOS_DSCP_REMARK
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
                BOOL            dscp_remark_mode_seen = 0;
                BOOL            dscp_remark_map_seen = 0;
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
                BOOL            dscp_translate_seen = 0;
                BOOL            dscp_ingress_class_seen = 0;
                BOOL            dscp_egress_remarking_seen = 0;
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */
                s->p = s->next;
                for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                    if (cx_parse_class(s, cx_pn_default_class, &new.default_prio) == VTSS_OK) {
                        class_seen = 1;
                    } else if (cx_parse_ulong(s, cx_pn_default_pcp, &val, 0, 7) == VTSS_OK) {
                        pcp_seen = 1;
                        new.usr_prio = val;
#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
                    } else if (cx_parse_ulong(s, cx_pn_default_dpl, &val, 0, QOS_DPL_MAX) == VTSS_OK) {
                        dpl_seen = 1;
                        new.default_dpl = val;
                    } else if (cx_parse_ulong(s, cx_pn_default_dei, &val, 0, 1) == VTSS_OK) {
                        dei_seen = 1;
                        new.default_dei = val;
                    } else if (cx_parse_bool(s, cx_pn_tag_class, &new.tag_class_enable, 1) == VTSS_OK) {
                        tag_class_seen = 1;
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
                    } else if (cx_parse_bool(s, cx_pn_port_dscp_qos_class, &new.dscp_class_enable, 1) == VTSS_OK) {
                        dscp_class_seen = 1;
#endif
                    } else if (cx_parse_txt(s, cx_pn_tag_map_class, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   pcp, dei;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        tag_map_class_seen = 1;
                        for (pcp = 0; pcp < VTSS_PCPS; pcp++) {
                            for (dei = 0; dei < VTSS_DEIS; dei++) {
                                val = strtol(start, &end, 0);
                                if (val > (QOS_CLASS_CNT - 1)) {
                                    T_DG(VTSS_TRACE_GRP_QOS, "val[%d][%d] = %u", pcp, dei, val);
                                    CX_RC(cx_parm_invalid(s));
                                    break;
                                }
                                new.qos_class_map[pcp][dei] = val;
                                start = end + 1; // skip the comma
                            }
                        }
                    } else if (cx_parse_txt(s, cx_pn_tag_map_dpl, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   pcp, dei;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        tag_map_dpl_seen = 1;
                        for (pcp = 0; pcp < VTSS_PCPS; pcp++) {
                            for (dei = 0; dei < VTSS_DEIS; dei++) {
                                val = strtol(start, &end, 0);
                                if (val > QOS_DPL_MAX) {
                                    T_DG(VTSS_TRACE_GRP_QOS, "val[%d][%d] = %u", pcp, dei, val);
                                    CX_RC(cx_parm_invalid(s));
                                    break;
                                }
                                new.dp_level_map[pcp][dei] = val;
                                start = end + 1; // skip the comma
                            }
                        }
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QCL_V1)
                    } else if (cx_parse_ulong(s, cx_pn_qcl_id, &val, QCL_ID_START,
                                              QCL_ID_END + RESERVED_QCL_CNT) == VTSS_OK) {
                        qcl_seen = 1;
                        new.qcl_no = val;
#endif /* VTSS_FEATURE_QCL_V1 */
                    }
#if (VTSS_PORT_POLICERS == 1)
                    else if (cx_parse_bool(s, cx_pn_policer_mode, &new.port_policer[0].enabled, 1) == VTSS_OK) {
                        p_mode_seen = 1;
                    } else if (cx_parse_bit_rate(s, cx_pn_policer_rate, &new.port_policer[0].policer.rate) == VTSS_OK) {
                        p_rate_seen = 1;
                    }
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
                    else if (cx_parse_kw(s, cx_pn_policer_unit, cx_kw_policer_unit, &val, 1) == VTSS_OK) {
                        p_unit_seen = 1;
                        new.port_policer_ext[0].frame_rate = val;
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
                    else if (cx_parse_bool(s, cx_pn_policer_fc, &new.port_policer_ext[0].flow_control, 1) == VTSS_OK) {
                        p_fc_seen = 1;
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#else
                    else if (cx_parse_txt(s, cx_pn_policer_mode, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_mode_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > 1)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer[policer].enabled = val;
                            start = end + 1; // skip the comma
                        }
                    } else if (cx_parse_txt(s, cx_pn_policer_rate, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_rate_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val < QOS_BITRATE_MIN) || (val > QOS_BITRATE_MAX)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer[policer].policer.rate = val;
                            start = end + 1; // skip the comma
                        }
                    }
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
                    else if (cx_parse_txt(s, cx_pn_policer_unit, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_unit_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > 1)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer_ext[policer].frame_rate = val;
                            start = end + 1; // skip the comma
                        }
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL)
                    else if (cx_parse_txt(s, cx_pn_policer_dpbl, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_dpbl_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > QOS_DPL_MAX)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer_ext[policer].dp_bypass_level = val;
                            start = end + 1; // skip the comma
                        }
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
                    else if (cx_parse_txt(s, cx_pn_policer_unicast, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_unicast_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > 1)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer_ext[policer].unicast = val;
                            start = end + 1; // skip the comma
                        }
                    } else if (cx_parse_txt(s, cx_pn_policer_multicast, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_multicast_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > 1)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer_ext[policer].multicast = val;
                            start = end + 1; // skip the comma
                        }
                    } else if (cx_parse_txt(s, cx_pn_policer_broadcast, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_broadcast_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > 1)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer_ext[policer].broadcast = val;
                            start = end + 1; // skip the comma
                        }
                    } else if (cx_parse_txt(s, cx_pn_policer_flooding, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_flooding_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > 1)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer_ext[policer].flooded = val;
                            start = end + 1; // skip the comma
                        }
                    } else if (cx_parse_txt(s, cx_pn_policer_learning, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_learning_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > 1)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer_ext[policer].learning = val;
                            start = end + 1; // skip the comma
                        }
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
                    else if (cx_parse_txt(s, cx_pn_policer_fc, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   policer;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        p_fc_seen = 1;
                        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                            val = strtol(start, &end, 0);
                            if ((val > 1)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", policer, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.port_policer_ext[policer].flow_control = val;
                            start = end + 1; // skip the comma
                        }
                    }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */

#endif /* (VTSS_PORT_POLICERS == 1) */
#if defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_QUEUE_POLICER)
                    else if (cx_parse_txt(s, cx_pn_iqpolicer_mode, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int queue;
                        u32 qp_val[QOS_PORT_QUEUE_CNT];

                        qp_mode_seen = 1;
                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        if (sscanf(buf, "%u,%u,%u,%u,%u,%u,%u,%u",
                                   &qp_val[0], &qp_val[1], &qp_val[2], &qp_val[3],
                                   &qp_val[4], &qp_val[5], &qp_val[6], &qp_val[7]) != QOS_PORT_QUEUE_CNT) {
                            CX_RC(cx_parm_invalid(s));
                        }
                        for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                            if (qp_val[queue] > 1) {
                                T_DG(VTSS_TRACE_GRP_QOS, "qp_val[%d] = %u", queue, qp_val[queue]);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.queue_policer[queue].enabled = qp_val[queue];
                        }
                    } else if (cx_parse_txt(s, cx_pn_iqpolicer_rate, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int queue;
                        u32 qp_val[QOS_PORT_QUEUE_CNT];

                        qp_rate_seen = 1;
                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        if (sscanf(buf, "%u,%u,%u,%u,%u,%u,%u,%u",
                                   &qp_val[0], &qp_val[1], &qp_val[2], &qp_val[3],
                                   &qp_val[4], &qp_val[5], &qp_val[6], &qp_val[7]) != QOS_PORT_QUEUE_CNT) {
                            CX_RC(cx_parm_invalid(s));
                        }
                        for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                            if ((qp_val[queue] < QOS_BITRATE_MIN) || (qp_val[queue] > QOS_BITRATE_MAX)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "qp_val[%d] = %u", queue, qp_val[queue]);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.queue_policer[queue].policer.rate = qp_val[queue];
                        }
                    }
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */
                    else if (cx_parse_bool(s, cx_pn_epshaper_mode, &new.shaper_status, 1) == VTSS_OK) {
                        s_mode_seen = 1;
                    } else if (cx_parse_bit_rate(s, cx_pn_epshaper_rate, &new.shaper_rate) == VTSS_OK) {
                        s_rate_seen = 1;
                    }
#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)
                    else if (cx_parse_txt(s, cx_pn_eqshaper_mode, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int queue;
                        u32 qs_val[QOS_PORT_QUEUE_CNT];

                        qs_mode_seen = 1;
                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        if (sscanf(buf, "%u,%u,%u,%u,%u,%u,%u,%u",
                                   &qs_val[0], &qs_val[1], &qs_val[2], &qs_val[3],
                                   &qs_val[4], &qs_val[5], &qs_val[6], &qs_val[7]) != QOS_PORT_QUEUE_CNT) {
                            CX_RC(cx_parm_invalid(s));
                        }
                        for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                            if (qs_val[queue] > 1) {
                                T_DG(VTSS_TRACE_GRP_QOS, "qs_val[%d] = %u", queue, qs_val[queue]);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.queue_shaper[queue].enable = qs_val[queue];
                        }
                    } else if (cx_parse_txt(s, cx_pn_eqshaper_rate, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int queue;
                        u32 qs_val[QOS_PORT_QUEUE_CNT];

                        qs_rate_seen = 1;
                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        if (sscanf(buf, "%u,%u,%u,%u,%u,%u,%u,%u",
                                   &qs_val[0], &qs_val[1], &qs_val[2], &qs_val[3],
                                   &qs_val[4], &qs_val[5], &qs_val[6], &qs_val[7]) != QOS_PORT_QUEUE_CNT) {
                            CX_RC(cx_parm_invalid(s));
                        }
                        for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                            if ((qs_val[queue] < QOS_BITRATE_MIN) || (qs_val[queue] > QOS_BITRATE_MAX)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "qs_val[%d] = %u", queue, qs_val[queue]);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.queue_shaper[queue].rate = qs_val[queue];
                        }
                    } else if (cx_parse_txt(s, cx_pn_eqshaper_excess, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int queue;
                        u32 qs_val[QOS_PORT_QUEUE_CNT];

                        qs_excess_seen = 1;
                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        if (sscanf(buf, "%u,%u,%u,%u,%u,%u,%u,%u",
                                   &qs_val[0], &qs_val[1], &qs_val[2], &qs_val[3],
                                   &qs_val[4], &qs_val[5], &qs_val[6], &qs_val[7]) != QOS_PORT_QUEUE_CNT) {
                            CX_RC(cx_parm_invalid(s));
                        }
                        for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                            if (qs_val[queue] > 1) {
                                T_DG(VTSS_TRACE_GRP_QOS, "qs_val[%d] = %u", queue, qs_val[queue]);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.excess_enable[queue] = qs_val[queue];
                        }
                    }
#endif /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */
#if defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)
                    else if (cx_parse_kw(s, cx_pn_tag_remark_mode, cx_kw_tag_remark_mode, &val, 1) == VTSS_OK) {
                        tr_mode_seen = 1;
                        new.tag_remark_mode = val;
                    } else if (cx_parse_ulong(s, cx_pn_tag_remark_def_pcp, &val, 0, 7) == VTSS_OK) {
                        tr_def_pcp_seen = 1;
                        new.tag_default_pcp = val;
                    } else if (cx_parse_ulong(s, cx_pn_tag_remark_def_dei, &val, 0, 1) == VTSS_OK) {
                        tr_def_dei_seen = 1;
                        new.tag_default_dei = val;
                    } else if (cx_parse_txt(s, cx_pn_tag_remark_dp_map, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int dpl;
                        u32 dpl_val[QOS_PORT_TR_DPL_CNT];

                        tr_dp_map_seen = 1;
                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        if (sscanf(buf, "%u,%u,%u,%u",
                                   &dpl_val[0], &dpl_val[1], &dpl_val[2], &dpl_val[3]) != QOS_PORT_TR_DPL_CNT) {
                            CX_RC(cx_parm_invalid(s));
                        }
                        for (dpl = 0; dpl < QOS_PORT_TR_DPL_CNT; dpl++) {
                            if (dpl_val[dpl] > 1) {
                                T_DG(VTSS_TRACE_GRP_QOS, "dpl_val[%d] = %u", dpl, dpl_val[dpl]);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.tag_dp_map[dpl] = dpl_val[dpl];
                        }
                    } else if (cx_parse_txt(s, cx_pn_tag_remark_pcp_map, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   class, dpl;
                        char *end = NULL;
                        char *start = buf;

                        tr_pcp_map_seen = 1;
                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        for (class = 0; class < QOS_PORT_PRIO_CNT; class++) {
                            for (dpl = 0; dpl < 2; dpl++) {
                                val = strtol(start, &end, 0);
                                if (val > 7) {
                                    T_DG(VTSS_TRACE_GRP_QOS, "val[%d][%d] = %u", class, dpl, val);
                                    CX_RC(cx_parm_invalid(s));
                                    break;
                                }
                                new.tag_pcp_map[class][dpl] = val;
                                start = end + 1; // skip the comma
                            }
                        }
                    } else if (cx_parse_txt(s, cx_pn_tag_remark_dei_map, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   class, dpl;
                        char *end = NULL;
                        char *start = buf;

                        tr_dei_map_seen = 1;
                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        for (class = 0; class < QOS_PORT_PRIO_CNT; class++) {
                            for (dpl = 0; dpl < 2; dpl++) {
                                val = strtol(start, &end, 0);
                                if (val > 1) {
                                    T_DG(VTSS_TRACE_GRP_QOS, "val[%d][%d] = %u", class, dpl, val);
                                    CX_RC(cx_parm_invalid(s));
                                    break;
                                }
                                new.tag_dei_map[class][dpl] = val;
                                start = end + 1; // skip the comma
                            }
                        }
                    }
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */
#ifdef VTSS_FEATURE_QOS_WFQ_PORT
                    else if (cx_parse_kw(s, cx_pn_scheduler_mode, cx_kw_scheduler_mode, &val, 1) == VTSS_OK) {
                        sched_mode_seen = 1;
                        new.wfq_enable = val;
                    } else if (cx_parse_txt(s, cx_pn_scheduler_weight, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int         i, len;
                        char        c;
                        vtss_prio_t prio;

                        sched_weight_seen = 1;
                        len = strlen(buf);
                        buf[len] = ',';
                        for (prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
                            i = 2 * (prio - VTSS_PRIO_START);
                            c = buf[i + 1];
                            buf[i + 1] = '\0';
                            if (len != 7 || c != ',' ||
                                mgmt_txt2ulong(&buf[i], &val, 1, 8) != VTSS_OK || val == 3 ||
                                (val > 4 && val < 8)) {
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.weight[prio] = (val == 1 ? VTSS_WEIGHT_1 :
                                                val == 2 ? VTSS_WEIGHT_2 :
                                                val == 4 ? VTSS_WEIGHT_4 : VTSS_WEIGHT_8);
                        }
                    }
#endif /* VTSS_FEATURE_QOS_WFQ_PORT */
#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
                    else if (cx_parse_kw(s, cx_pn_scheduler_mode, cx_kw_scheduler_mode, &val, 1) == VTSS_OK) {
                        sched_mode_seen = 1;
                        new.dwrr_enable = val;
                    } else if (cx_parse_txt(s, cx_pn_scheduler_weight, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int   queue;
                        char *end = NULL;
                        char *start = buf;

                        T_DG(VTSS_TRACE_GRP_QOS, "buf = %s", buf);
                        sched_weight_seen = 1;
                        for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                            val = strtol(start, &end, 0);
                            if ((val < 1) || (val > 100)) {
                                T_DG(VTSS_TRACE_GRP_QOS, "val[%d] = %u", queue, val);
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.queue_pct[queue] = val;
                            start = end + 1; // skip the comma
                        }
                    }
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */
#ifdef VTSS_FEATURE_QOS_DSCP_REMARK
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
                    else if (cx_parse_bool(s, cx_pn_dscp_remark_mode, &new.qos_remarking_enable, 1) == VTSS_OK) {
                        dscp_remark_mode_seen = 1;
                    } else if (cx_parse_txt(s, cx_pn_dscp_remark_map, buf, sizeof(buf) - 1) == VTSS_OK) {
                        int         i, len;
                        char        c;
                        vtss_prio_t prio;

                        dscp_remark_map_seen = 1;
                        len = strlen(buf);
                        buf[len] = ',';
                        for (prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
                            i = 3 * (prio - VTSS_PRIO_START);
                            c = buf[i + 2];
                            if (c != ',') {
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            if (buf[i] == '0') {
                                buf[i] = ' ';
                            }
                            buf[i + 2] = '\0';
                            if (mgmt_txt2ulong(&buf[i], &val, 0, 63) != VTSS_OK) {
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            if (val != 0  && val != 8  && val != 16 &&
                                val != 24 && val != 32 && val != 40 &&
                                val != 48 && val != 56 && val != 46) {
                                CX_RC(cx_parm_invalid(s));
                                break;
                            }
                            new.dscp_remarking_val[prio] = val;
                        }
                    }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
                    else if (cx_parse_bool(s, cx_pn_port_dscp_translate_mode, &new.dscp_translate, 1) == VTSS_OK) {
                        dscp_translate_seen = 1;
                    } else if (cx_parse_kw(s, cx_pn_port_dscp_ingress_class_mode, cx_kw_port_dscp_ingress_class_mode, &val, 1) == VTSS_OK) {
                        dscp_ingress_class_seen = 1;
                        new.dscp_imode = val;
                    } else if (cx_parse_kw(s, cx_pn_port_dscp_egress_remarking_mode, cx_kw_port_dscp_egress_remarking_mode, &val, 1) == VTSS_OK) {
                        dscp_egress_remarking_seen = 1;
                        new.dscp_emode = val;
                    }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */
                    else {
                        CX_RC(cx_parm_unknown(s));
                    }
                }
                for (port_idx = VTSS_PORT_NO_START; port_idx < VTSS_PORT_NO_END; port_idx++)
                    if (s->apply && port_list[iport2uport(port_idx)] &&
                        qos_port_conf_get(s->isid, port_idx, &qos_port_conf) == VTSS_OK) {
                        if (class_seen) {
                            qos_port_conf.default_prio = new.default_prio;
                        }
                        if (pcp_seen) {
                            qos_port_conf.usr_prio = new.usr_prio;
                        }
#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
                        if (dpl_seen) {
                            qos_port_conf.default_dpl = new.default_dpl;
                        }
                        if (dei_seen) {
                            qos_port_conf.default_dei = new.default_dei;
                        }
                        if (tag_class_seen) {
                            qos_port_conf.tag_class_enable = new.tag_class_enable;
                        }
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
                        if (dscp_class_seen) {
                            qos_port_conf.dscp_class_enable = new.dscp_class_enable;
                        }
#endif
                        if (tag_map_class_seen) {
                            int   pcp, dei;
                            for (pcp = 0; pcp < VTSS_PCPS; pcp++) {
                                for (dei = 0; dei < VTSS_DEIS; dei++) {
                                    qos_port_conf.qos_class_map[pcp][dei] = new.qos_class_map[pcp][dei];
                                }
                            }
                        }
                        if (tag_map_dpl_seen) {
                            int   pcp, dei;
                            for (pcp = 0; pcp < VTSS_PCPS; pcp++) {
                                for (dei = 0; dei < VTSS_DEIS; dei++) {
                                    qos_port_conf.dp_level_map[pcp][dei] = new.dp_level_map[pcp][dei];
                                }
                            }
                        }
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QCL_V1)
                        if (qcl_seen) {
                            qos_port_conf.qcl_no = new.qcl_no;
                        }
#endif /* VTSS_FEATURE_QCL_V1 */
#if (VTSS_PORT_POLICERS == 1)
                        if (p_mode_seen) {
                            qos_port_conf.port_policer[0].enabled = new.port_policer[0].enabled;
                        }
                        if (p_rate_seen) {
                            qos_port_conf.port_policer[0].policer.rate = new.port_policer[0].policer.rate;
                        }
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
                        if (p_unit_seen) {
                            qos_port_conf.port_policer_ext[0].frame_rate = new.port_policer_ext[0].frame_rate;
                        }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
                        if (p_fc_seen) {
                            qos_port_conf.port_policer_ext[0].flow_control = new.port_policer_ext[0].flow_control;
                        }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#else
                        if (p_mode_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer[policer].enabled = new.port_policer[policer].enabled;
                            }
                        }
                        if (p_rate_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer[policer].policer.rate = new.port_policer[policer].policer.rate;
                            }
                            qos_port_conf.port_policer[0].policer.rate = new.port_policer[0].policer.rate;
                        }
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
                        if (p_unit_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer_ext[policer].frame_rate = new.port_policer_ext[policer].frame_rate;
                            }
                        }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL)
                        if (p_dpbl_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer_ext[policer].dp_bypass_level = new.port_policer_ext[policer].dp_bypass_level;
                            }
                        }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
                        if (p_unicast_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer_ext[policer].unicast = new.port_policer_ext[policer].unicast;
                            }
                        }
                        if (p_multicast_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer_ext[policer].multicast = new.port_policer_ext[policer].multicast;
                            }
                        }
                        if (p_broadcast_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer_ext[policer].broadcast = new.port_policer_ext[policer].broadcast;
                            }
                        }
                        if (p_flooding_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer_ext[policer].flooded = new.port_policer_ext[policer].flooded;
                            }
                        }
                        if (p_learning_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer_ext[policer].learning = new.port_policer_ext[policer].learning;
                            }
                        }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
                        if (p_fc_seen) {
                            int policer;
                            for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
                                qos_port_conf.port_policer_ext[policer].flow_control = new.port_policer_ext[policer].flow_control;
                            }
                        }
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */

#endif /* (VTSS_PORT_POLICERS == 1) */
#if defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_QUEUE_POLICER)
                        if (qp_mode_seen) {
                            int queue;
                            for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                                qos_port_conf.queue_policer[queue].enabled = new.queue_policer[queue].enabled;
                            }
                        }
                        if (qp_rate_seen) {
                            int queue;
                            for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                                qos_port_conf.queue_policer[queue].policer.rate = new.queue_policer[queue].policer.rate;
                            }
                        }
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */
                        if (s_mode_seen) {
                            qos_port_conf.shaper_status = new.shaper_status;
                        }
                        if (s_rate_seen) {
                            qos_port_conf.shaper_rate = new.shaper_rate;
                        }
#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)
                        if (qs_mode_seen) {
                            int queue;
                            for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                                qos_port_conf.queue_shaper[queue].enable = new.queue_shaper[queue].enable;
                            }
                        }
                        if (qs_rate_seen) {
                            int queue;
                            for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                                qos_port_conf.queue_shaper[queue].rate = new.queue_shaper[queue].rate;
                            }
                        }
                        if (qs_excess_seen) {
                            int queue;
                            for (queue = 0; queue < QOS_PORT_QUEUE_CNT; queue++) {
                                qos_port_conf.excess_enable[queue] = new.excess_enable[queue];
                            }
                        }
#endif /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */
#if defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)
                        if (tr_mode_seen) {
                            qos_port_conf.tag_remark_mode = new.tag_remark_mode;
                        }
                        if (tr_def_pcp_seen) {
                            qos_port_conf.tag_default_pcp = new.tag_default_pcp;
                        }
                        if (tr_def_dei_seen) {
                            qos_port_conf.tag_default_dei = new.tag_default_dei;
                        }
                        if (tr_dp_map_seen) {
                            int dpl;
                            for (dpl = 0; dpl < QOS_PORT_TR_DPL_CNT; dpl++) {
                                qos_port_conf.tag_dp_map[dpl] = new.tag_dp_map[dpl];
                            }
                        }
                        if (tr_pcp_map_seen) {
                            int   class, dpl;
                            for (class = 0; class < QOS_PORT_PRIO_CNT; class++) {
                                for (dpl = 0; dpl < 2; dpl++) {
                                    qos_port_conf.tag_pcp_map[class][dpl] = new.tag_pcp_map[class][dpl];
                                }
                            }
                        }
                        if (tr_dei_map_seen) {
                            int   class, dpl;
                            for (class = 0; class < QOS_PORT_PRIO_CNT; class++) {
                                for (dpl = 0; dpl < 2; dpl++) {
                                    qos_port_conf.tag_dei_map[class][dpl] = new.tag_dei_map[class][dpl];
                                }
                            }
                        }
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */
#ifdef VTSS_FEATURE_QOS_WFQ_PORT
                        if (sched_mode_seen) {
                            qos_port_conf.wfq_enable = new.wfq_enable;
                        }
                        if (sched_weight_seen) {
                            {
                                vtss_prio_t prio;
                                for (prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
                                    qos_port_conf.weight[prio] = new.weight[prio];
                                }
                            }
                        }
#endif /* VTSS_FEATURE_QOS_WFQ_PORT */
#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
                        if (sched_mode_seen) {
                            qos_port_conf.dwrr_enable = new.dwrr_enable;
                        }
                        if (sched_weight_seen) {
                            int queue;
                            for (queue = 0; queue < QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                                qos_port_conf.queue_pct[queue] = new.queue_pct[queue];
                            }
                        }
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */
#ifdef VTSS_FEATURE_QOS_DSCP_REMARK
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
                        if (dscp_remark_mode_seen) {
                            qos_port_conf.qos_remarking_enable = new.qos_remarking_enable;
                        }
                        if (dscp_remark_map_seen) {
                            vtss_prio_t prio;
                            for (prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
                                qos_port_conf.dscp_remarking_val[prio] = new.dscp_remarking_val[prio];
                            }
                        }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
                        if (dscp_translate_seen) {
                            qos_port_conf.dscp_translate = new.dscp_translate;
                        }
                        if (dscp_ingress_class_seen) {
                            qos_port_conf.dscp_imode = new.dscp_imode;
                        }
                        if (dscp_egress_remarking_seen) {
                            qos_port_conf.dscp_emode = new.dscp_emode;
                        }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */
                        CX_RC(qos_port_conf_set(s->isid, port_idx, &qos_port_conf));
                    }
            } else {
                s->ignored = 1;
            }
            break;
        default:
            s->ignored = 1;
            break;
        }
        if (global && s->apply) {
            CX_RC(qos_conf_set(&conf));
        }
        break;
    } /* CX_PARSE_CMD_PARM */
    case CX_PARSE_CMD_GLOBAL:
        T_DG(VTSS_TRACE_GRP_QOS, "CX_PARSE_CMD_GLOBAL");
        break;
    case CX_PARSE_CMD_SWITCH:
        T_DG(VTSS_TRACE_GRP_QOS, "CX_PARSE_CMD_SWITCH");
        break;
    default:
        T_E("Unknown command (%d)", s->cmd);
        return VTSS_RC_ERROR;
    }

    return s->rc;
}

#if defined(VTSS_FEATURE_QCL_V2)
#define QOS_QCE_RANGE_TXT(buf, min, max) sprintf(buf, "any or %d-%d (range allowed. e.g. 10-20)", (min), (max))
#define QOS_QCE_TXT(buf, min, max)       sprintf(buf, "any or %d-%d", (min), (max))
#define QOS_QCE_TXT_HEX(buf, min, max)       sprintf(buf, "any or 0x%x-%x", (min), (max))

static char *qcl_qce_range_txt_u16(qos_qce_vr_u16_t *range, char *buf)
{
    if (range->in_range) {
        sprintf(buf, "%u-%u", range->vr.r.low, range->vr.r.high);
    } else if (range->vr.v.mask) {
        sprintf(buf, "%u", range->vr.v.value);
    } else {
        sprintf(buf, "any");
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
        sprintf(buf, "any");
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
        sprintf(buf, "any");
    }
    return(buf);
}

static char *qcl_qce_uchar_txt(vtss_qce_u8_t *data, char *buf, BOOL in_hex)
{
    uchar high;

    if (!data->mask) {
        strcpy(buf, "any");
    } else if (data->mask == 0xFF) { /*specific value */
        if (in_hex) {
            sprintf(buf, "0x%x", data->value);
        } else {
            sprintf(buf, "%u", data->value);
        }
    } else { /* range value */
        high = data->value + (uchar)((~data->mask) & 0xFF);
        if (in_hex) {
            sprintf(buf, "0x%x-%x", data->value, high);
        } else {
            sprintf(buf, "%u-%u", data->value, high);
        }
    }
    return(buf);
}


/* QCL uchar2 text */
static char *qcl_qce_uchar2_txt(vtss_qce_u16_t *data, char *buf, BOOL in_hex)
{
    u16 val = (data->value[0] << 8 | data->value[1]);

    if (data->mask[0] || data->mask[1]) {
        if (in_hex) {
            sprintf(buf, "0x%x", val);
        } else {
            sprintf(buf, "%u", val);
        }
    } else {
        strcpy(buf, "any");
    }
    return(buf);
}

static char *qcl_qce_ipv6_txt(vtss_qce_u32_t *ip, char *buf)
{
    ulong i, n = 0;

    if (!ip->mask[0] && !ip->mask[1] && !ip->mask[2] && !ip->mask[3]) {
        sprintf(buf, "any");
    } else {
        for (i = 0; i < 32; i++) {
            if (ip->mask[i / 8] & (1 << (i % 8))) {
                n++;
            }
        }
        sprintf(buf, "%u.%u.%u.%u/%d", ip->value[0], ip->value[1],
                ip->value[2], ip->value[3], n);
    }
    return(buf);
}
#endif /* VTSS_FEATURE_QCL_V2 */

static vtss_rc qos_cx_gen_func(cx_get_state_t *s)
{
    switch (s->cmd) {
    case CX_GEN_CMD_GLOBAL:
        /* Global - QoS */
        T_DG(VTSS_TRACE_GRP_QOS, "CX_GEN_CMD_GLOBAL");
        CX_RC(cx_add_tag_line(s, CX_TAG_QOS, 0));
        {
            qos_conf_t           conf;
            qos_qce_entry_conf_t qce;
            char                 buf[QOS_BUFF_LEN];
            int                  i;
#if defined(VTSS_FEATURE_QCL_V1)
            vtss_qcl_id_t        qcl_id;
            vtss_prio_t          prio;
            char                 *p;
            BOOL                 tos;
#endif /* VTSS_FEATURE_QCL_V1 */
#if defined(VTSS_FEATURE_QCL_V2)
            char                 buf_1[80];
            BOOL                 port_list[VTSS_PORTS];
            u8                   bit_val;
#endif /* VTSS_FEATURE_QCL_V2 */

            if (qos_conf_get(&conf) == VTSS_OK) {
#ifdef VTSS_ARCH_LUTON28
                CX_RC(cx_add_val_ulong_stx(s, CX_TAG_CLASSES, conf.prio_no, "1/2/4"));
#endif /* VTSS_ARCH_LUTON28 */
#if defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) && defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
                CX_RC(cx_add_attr_storm(s, CX_TAG_STORM_UNICAST,
                                        conf.policer_uc_status, conf.policer_uc));
                CX_RC(cx_add_attr_storm(s, CX_TAG_STORM_MULTICAST,
                                        conf.policer_mc_status, conf.policer_mc));
                CX_RC(cx_add_attr_storm(s, CX_TAG_STORM_BROADCAST,
                                        conf.policer_bc_status, conf.policer_bc));
#endif /* VTSS_FEATURE_QOS_POLICER_UC_SWITCH && VTSS_FEATURE_QOS_POLICER_MC_SWITCH && VTSS_FEATURE_QOS_POLICER_BC_SWITCH */
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
                CX_RC(cx_add_attr_dscp(s, &conf.dscp));
#endif /* defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#ifdef VTSS_FEATURE_VSTAX_V2
                CX_RC(cx_add_val_bool(s, CX_TAG_CMEF, !conf.cmef_disable));
#endif /* VTSS_FEATURE_VSTAX_V2 */

#if defined(VTSS_FEATURE_QOS_WRED)
                CX_RC(cx_add_tag_line(s, CX_TAG_WRED_TABLE, 0));

                /* Entry syntax */
                CX_RC(cx_add_stx_start(s));
                CX_RC(cx_add_stx_ulong(s, "queue", 0, QOS_PORT_WEIGHTED_QUEUE_CNT - 1));
                CX_RC(cx_add_stx_bool(s, "mode"));
                CX_RC(cx_add_stx_ulong(s, "min_th", 0, 100));
                CX_RC(cx_add_stx_ulong(s, "max_dp1", 0, 100));
                CX_RC(cx_add_stx_ulong(s, "max_dp2", 0, 100));
                CX_RC(cx_add_stx_ulong(s, "max_dp3", 0, 100));
                CX_RC(cx_add_stx_end(s));

                for (i = 0; i < QOS_PORT_WEIGHTED_QUEUE_CNT; i++) {
                    CX_RC(cx_add_attr_start(s, CX_TAG_ENTRY));
                    CX_RC(cx_add_attr_ulong(s, "queue", i));
                    CX_RC(cx_add_attr_bool(s, "mode", conf.wred[i].enable));
                    CX_RC(cx_add_attr_ulong(s, "min_th", conf.wred[i].min_th));
                    CX_RC(cx_add_attr_ulong(s, "max_dp1", conf.wred[i].max_prob_1));
                    CX_RC(cx_add_attr_ulong(s, "max_dp2", conf.wred[i].max_prob_2));
                    CX_RC(cx_add_attr_ulong(s, "max_dp3", conf.wred[i].max_prob_3));
                    CX_RC(cx_add_attr_end(s, CX_TAG_ENTRY));
                }

                CX_RC(cx_add_tag_line(s, CX_TAG_WRED_TABLE, 1));
#endif /* VTSS_FEATURE_QOS_WRED */
            }
            CX_RC(cx_add_tag_line(s, CX_TAG_QCL_TABLE, 0));

            /* Entry syntax */
#if defined(VTSS_FEATURE_QCL_V1)
            for (qce.type = VTSS_QCE_TYPE_ETYPE; qce.type <= VTSS_QCE_TYPE_TAG; qce.type++) {
                CX_RC(cx_add_stx_start(s));
                CX_RC(cx_add_stx_ulong(s, cx_pn_qcl_id, QCL_ID_START, QCL_ID_END));
                CX_RC(cx_add_stx_ulong(s, cx_pn_qce_id, QCE_ID_START, QCE_ID_END));
                tos = (qce.type == VTSS_QCE_TYPE_TOS);
                if (tos || qce.type == VTSS_QCE_TYPE_TAG) {
                    sprintf(buf, "a,b,c,d,e,f,g,h where each letter is %s", cx_stx_class);
                    CX_RC(cx_add_attr_txt(s, tos ? "tos_class" : "tag_class", buf));
                } else {
                    if (qce.type == VTSS_QCE_TYPE_ETYPE) {
                        CX_RC(cx_add_attr_txt(s, "etype", "0x0600-0xffff"));
                    } else if (qce.type == VTSS_QCE_TYPE_VLAN) {
                        CX_RC(cx_add_stx_ulong(s, "vid", VTSS_VID_DEFAULT, VTSS_VID_RESERVED));
                    } else if (qce.type == VTSS_QCE_TYPE_UDP_TCP) {
                        CX_RC(cx_add_attr_txt(s, "udp_tcp_port", "0-65535 (range allowed, e.g. 10-20)"));
                    } else {
                        CX_RC(cx_add_stx_ulong(s, "dscp", 0, 63));
                    }
                    CX_RC(cx_add_attr_txt(s, "class", cx_stx_class));
                }
                CX_RC(cx_add_stx_end(s));
            }

            for (qcl_id = QCL_ID_START; qcl_id <= QCL_ID_END; qcl_id++) {
                qce.id = QCE_ID_NONE;
                while (qos_mgmt_qce_entry_get(qcl_id, qce.id, &qce, 1) == VTSS_OK) {
                    CX_RC(cx_add_attr_start(s, CX_TAG_ENTRY));
                    CX_RC(cx_add_attr_ulong(s, cx_pn_qcl_id, qcl_id));
                    CX_RC(cx_add_attr_ulong(s, cx_pn_qce_id, qce.id));
                    tos = (qce.type == VTSS_QCE_TYPE_TOS);
                    if (tos || qce.type == VTSS_QCE_TYPE_TAG) {
                        for (p = &buf[0], i = 0; i < 8; i++) {
                            prio = (tos ? qce.frame.tos_prio[i] : qce.frame.tag_prio[i]);
                            p += sprintf(p, "%s%s", i == 0 ? "" : ",", mgmt_prio2txt(prio, 1));
                        }
                        CX_RC(cx_add_attr_txt(s, tos ? "tos_class" : "tag_class", buf));
                    } else {
                        if (qce.type == VTSS_QCE_TYPE_ETYPE) {
                            sprintf(buf, "0x%04x", qce.frame.etype.val);
                            CX_RC(cx_add_attr_txt(s, "etype", buf));
                            prio = qce.frame.etype.prio;
                        } else if (qce.type == VTSS_QCE_TYPE_VLAN) {
                            CX_RC(cx_add_attr_ulong(s, "vid", qce.frame.vlan.vid));
                            prio = qce.frame.vlan.prio;
                        } else if (qce.type == VTSS_QCE_TYPE_UDP_TCP) {
                            if (qce.frame.udp_tcp.low == qce.frame.udp_tcp.high) {
                                sprintf(buf, "%u", qce.frame.udp_tcp.low);
                            } else
                                sprintf(buf, "%u-%u",
                                        qce.frame.udp_tcp.low, qce.frame.udp_tcp.high);
                            CX_RC(cx_add_attr_txt(s, "udp_tcp_port", buf));
                            prio = qce.frame.udp_tcp.prio;
                        } else {
                            CX_RC(cx_add_attr_ulong(s, "dscp", qce.frame.dscp.dscp_val));
                            prio = qce.frame.dscp.prio;
                        }
                        CX_RC(cx_add_attr_prio(s, "class", prio));
                    }
                    CX_RC(cx_add_attr_end(s, CX_TAG_ENTRY));
                }
            }
#endif /* VTSS_FEATURE_QCL_V1 */
#if defined(VTSS_FEATURE_QCL_V2)
            CX_RC(cx_add_comment(s, "The 'qce_id' is the only mandatory attribute"));
            CX_RC(cx_add_comment(s, "At least one of 'class', 'dp' or 'dscp' needs to specify"));
            CX_RC(cx_add_comment(s, "The 'frame' attribute determines the syntax as shown below"));
            for (qce.type = VTSS_QCE_TYPE_ETYPE; qce.type <= VTSS_QCE_TYPE_IPV6; qce.type++) {
                CX_RC(cx_add_stx_start(s));
                CX_RC(cx_add_stx_ulong(s, "qce_id", QCE_ID_START, QCE_ID_END));
#if VTSS_SWITCH_STACKABLE
                QOS_QCE_TXT(buf, VTSS_USID_START, VTSS_USID_END - 1);
                CX_RC(cx_add_attr_txt(s, "sid", buf));
#endif
                sprintf(buf_1, "any or %s", cx_list_txt(buf, 1, VTSS_FRONT_PORT_COUNT));
                CX_RC(cx_add_attr_txt(s, "port", buf_1));

                CX_RC(cx_add_stx_kw(s, "tag", cx_kw_tag));
                QOS_QCE_RANGE_TXT(buf, VTSS_VID_DEFAULT, VTSS_VID_RESERVED);
                CX_RC(cx_add_attr_txt(s, "vid", buf));
                //QOS_QCE_TXT(buf, VTSS_PCP_START, VTSS_PCP_END - 1);
                sprintf(buf, "any or specific (0, 1, 2, 3, 4, 5, 6, 7) or range (0-1, 2-3, 4-5, 6-7, 0-3, 4-7)");
                CX_RC(cx_add_attr_txt(s, "pcp", buf));
                QOS_QCE_TXT(buf, VTSS_DEI_START, VTSS_DEI_END - 1);
                CX_RC(cx_add_attr_txt(s, "dei", buf));
                CX_RC(cx_add_attr_txt(s, "smac", "any or xx-xx-xx (OUI notation)"));
                CX_RC(cx_add_stx_kw(s, "dmac_type", cx_kw_dmac_type));

                CX_RC(cx_add_attr_kw(s, "frame", cx_kw_frame_type, qce.type));
                switch (qce.type) {
                case VTSS_QCE_TYPE_ETYPE:
                    QOS_QCE_TXT_HEX(buf, 0x600, 0xFFFF);
                    CX_RC(cx_add_attr_txt(s, "etype", buf));
                    break;
                case VTSS_QCE_TYPE_LLC:
                    QOS_QCE_TXT_HEX(buf, 0x00, 0xFF);
                    CX_RC(cx_add_attr_txt(s, "dsap", buf));
                    QOS_QCE_TXT_HEX(buf, 0x00, 0xFF);
                    CX_RC(cx_add_attr_txt(s, "ssap", buf));
                    QOS_QCE_TXT_HEX(buf, 0x00, 0xFF);
                    CX_RC(cx_add_attr_txt(s, "control", buf));
                    break;
                case VTSS_QCE_TYPE_SNAP:
                    QOS_QCE_TXT_HEX(buf, 0x00, 0xFFFF);
                    CX_RC(cx_add_attr_txt(s, "pid", buf));
                    break;
                case VTSS_QCE_TYPE_IPV4:
                case VTSS_QCE_TYPE_IPV6:
                    QOS_QCE_TXT(buf, 0, 255);
                    CX_RC(cx_add_attr_txt(s, "protocol", buf));
                    if (qce.type == VTSS_QCE_TYPE_IPV4) {
                        CX_RC(cx_add_stx_ipv4(s, "sip"));
                        CX_RC(cx_add_stx_kw(s, "fragment", cx_kw_fragment));
                    } else {
                        CX_RC(cx_add_attr_txt(s, "sipv6", "any or a.b.c.d/n (32 LSB)"));
                    }
                    QOS_QCE_RANGE_TXT(buf, 0, 63);
                    CX_RC(cx_add_attr_txt(s, "dscp", buf));
                    QOS_QCE_RANGE_TXT(buf, 0, 65535);
                    CX_RC(cx_add_attr_txt(s, "sport", buf));
                    QOS_QCE_RANGE_TXT(buf, 0, 65535);
                    CX_RC(cx_add_attr_txt(s, "dport", buf));
                    break;
                default:
                    break;
                }

                /* Action */
                CX_RC(cx_add_stx_ulong(s, "class", 0, QOS_CLASS_CNT - 1));
                CX_RC(cx_add_stx_ulong(s, "dp", 0, QOS_DPL_MAX));
                CX_RC(cx_add_stx_ulong(s, "classified_dscp", 0, 63));

                CX_RC(cx_add_stx_end(s));
            }

            qce.id = QCE_ID_NONE;
            while (qos_mgmt_qce_entry_get(VTSS_ISID_GLOBAL, QCL_USER_STATIC, QCL_ID_END, qce.id, &qce, 1) == VTSS_OK) {
                CX_RC(cx_add_attr_start(s, CX_TAG_ENTRY));

                CX_RC(cx_add_attr_ulong(s, "qce_id", qce.id));
#if VTSS_SWITCH_STACKABLE
                if (qce.isid == VTSS_ISID_GLOBAL) {
                    sprintf(buf, "any");
                } else {
                    sprintf(buf, "%u", topo_isid2usid(qce.isid));
                }
                CX_RC(cx_add_attr_txt(s, "sid", buf));
#endif
                /* port */
                for (i = VTSS_PORT_NO_START; i < VTSS_PORT_NO_END; i++) {
                    if (VTSS_PORT_BF_GET(qce.port_list, i)) {
                        port_list[i] = 1;
                    } else {
                        port_list[i] = 0;
                    }
                }
                CX_RC(cx_add_attr_txt(s, "port", mgmt_iport_list2txt(port_list, buf)));
                /* VLAN */
                bit_val = QCE_ENTRY_CONF_KEY_GET(qce.key.key_bits, QOS_QCE_VLAN_TAG);
                CX_RC(cx_add_attr_kw(s, "tag", cx_kw_tag, bit_val));
                CX_RC(cx_add_attr_txt(s, "vid", qcl_qce_range_txt_u16(&qce.key.vid, buf)));
                CX_RC(cx_add_attr_txt(s, "pcp", qcl_qce_uchar_txt(&qce.key.pcp, buf, 0)));
                bit_val = QCE_ENTRY_CONF_KEY_GET(qce.key.key_bits, QOS_QCE_VLAN_DEI);
                if (bit_val == VTSS_VCAP_BIT_ANY) {
                    sprintf(buf, "any");
                } else {
                    sprintf(buf, "%u", bit_val - 1);
                }
                CX_RC(cx_add_attr_txt(s, "dei", buf));

                /* MAC */
                if (!qce.key.smac.mask[0] && !qce.key.smac.mask[1] &&
                    !qce.key.smac.mask[2]) {
                    sprintf(buf, "any");
                } else {
                    sprintf(buf, "%02x-%02x-%02x", qce.key.smac.value[0],
                            qce.key.smac.value[1], qce.key.smac.value[2]);
                }
                CX_RC(cx_add_attr_txt(s, "smac", buf));
                bit_val = QCE_ENTRY_CONF_KEY_GET(qce.key.key_bits, QOS_QCE_DMAC_TYPE);
                CX_RC(cx_add_attr_kw(s, "dmac_type", cx_kw_dmac_type, bit_val));

                CX_RC(cx_add_attr_kw(s, "frame", cx_kw_frame_type, qce.type));
                switch (qce.type) {
                case VTSS_QCE_TYPE_ETYPE:
                    CX_RC(cx_add_attr_txt(s, "etype", qcl_qce_uchar2_txt(&qce.key.frame.etype.etype, buf, 1)));
                    break;
                case VTSS_QCE_TYPE_LLC:
                    CX_RC(cx_add_attr_txt(s, "dsap", qcl_qce_uchar_txt(&qce.key.frame.llc.dsap, buf, 1)));
                    CX_RC(cx_add_attr_txt(s, "ssap", qcl_qce_uchar_txt(&qce.key.frame.llc.ssap, buf, 1)));
                    CX_RC(cx_add_attr_txt(s, "control", qcl_qce_uchar_txt(&qce.key.frame.llc.control, buf, 1)));
                    break;
                case VTSS_QCE_TYPE_SNAP:
                    CX_RC(cx_add_attr_txt(s, "pid", qcl_qce_uchar2_txt(&qce.key.frame.snap.pid, buf, 1)));
                    break;
                case VTSS_QCE_TYPE_IPV4:
                    CX_RC(cx_add_attr_txt(s, "protocol", qcl_qce_proto_txt(&qce.key.frame.ipv4.proto, buf)));
                    CX_RC(cx_add_attr_txt(s, "sip", mgmt_acl_ipv4_txt(&qce.key.frame.ipv4.sip, buf, 1)));
                    bit_val = QCE_ENTRY_CONF_KEY_GET(qce.key.key_bits, QOS_QCE_IPV4_FRAGMENT);
                    CX_RC(cx_add_attr_kw(s, "fragment", cx_kw_fragment, bit_val));
                    CX_RC(cx_add_attr_txt(s, "dscp", qcl_qce_range_txt_u8(&qce.key.frame.ipv4.dscp, buf)));
                    CX_RC(cx_add_attr_txt(s, "sport", qcl_qce_range_txt_u16(&qce.key.frame.ipv4.sport, buf)));
                    CX_RC(cx_add_attr_txt(s, "dport", qcl_qce_range_txt_u16(&qce.key.frame.ipv4.dport, buf)));
                    break;
                case VTSS_QCE_TYPE_IPV6:
                    CX_RC(cx_add_attr_txt(s, "protocol", qcl_qce_proto_txt(&qce.key.frame.ipv6.proto, buf)));
                    CX_RC(cx_add_attr_txt(s, "sipv6", qcl_qce_ipv6_txt(&qce.key.frame.ipv6.sip, buf)));
                    CX_RC(cx_add_attr_txt(s, "dscp", qcl_qce_range_txt_u8(&qce.key.frame.ipv6.dscp, buf)));
                    CX_RC(cx_add_attr_txt(s, "sport", qcl_qce_range_txt_u16(&qce.key.frame.ipv6.sport, buf)));
                    CX_RC(cx_add_attr_txt(s, "dport", qcl_qce_range_txt_u16(&qce.key.frame.ipv6.dport, buf)));
                    break;
                default:
                    break;
                }

                /* action */
                if (QCE_ENTRY_CONF_ACTION_GET(qce.action.action_bits, QOS_QCE_ACTION_PRIO)) {
                    CX_RC(cx_add_attr_ulong(s, "class", qce.action.prio));
                }
                if (QCE_ENTRY_CONF_ACTION_GET(qce.action.action_bits, QOS_QCE_ACTION_DP)) {
                    CX_RC(cx_add_attr_ulong(s, "dp", qce.action.dp));
                }
                if (QCE_ENTRY_CONF_ACTION_GET(qce.action.action_bits, QOS_QCE_ACTION_DSCP)) {
                    CX_RC(cx_add_attr_ulong(s, "classified_dscp", qce.action.dscp));
                }

                CX_RC(cx_add_attr_end(s, CX_TAG_ENTRY));
            }
#endif /* VTSS_FEATURE_QCL_V2 */

            CX_RC(cx_add_tag_line(s, CX_TAG_QCL_TABLE, 1));
        }
        CX_RC(cx_add_tag_line(s, CX_TAG_QOS, 1));
        break;
    case CX_GEN_CMD_SWITCH:
        /* Switch - QoS */
        T_DG(VTSS_TRACE_GRP_QOS, "CX_GEN_CMD_SWITCH");
        CX_RC(cx_add_tag_line(s, CX_TAG_QOS, 0));
        CX_RC(cx_add_port_table(s, s->isid, CX_TAG_PORT_TABLE, cx_qos_match, cx_qos_print));
        CX_RC(cx_add_tag_line(s, CX_TAG_QOS, 1));
        break;
    default:
        T_E("Unknown command (%d)", s->cmd);
        return VTSS_RC_ERROR;
    } /* End of Switch */

    return VTSS_OK;
}
/* Register the info in to the cx_module_table */
CX_MODULE_TAB_ENTRY(VTSS_MODULE_ID_QOS, qos_cx_tag_table,
                    0, 0,
                    NULL,                  /* init function       */
                    qos_cx_gen_func,       /* Generation fucntion */
                    qos_cx_parse_func);    /* parse fucntion      */
