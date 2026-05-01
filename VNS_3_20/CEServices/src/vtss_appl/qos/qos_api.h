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
/* debug msg can be enabled by cmd "debug trace module level qos default debug" */

#ifndef _VTSS_QOS_APPL_API_H_
#define _VTSS_QOS_APPL_API_H_

#include "vtss_api.h" /* For vtss_rc, vtss_vid_t, etc. */

#if defined(VTSS_ARCH_JAGUAR_1) && (!defined(VTSS_SW_OPTION_BUILD_CE) || defined(VTSS_ARCH_JAGUAR_1_DUAL) || VTSS_SWITCH_STACKABLE)
/* All Jaguar builds except single chip standalone CE uses fixed mapping */
#define QOS_USE_FIXED_PCP_QOS_MAP /* Bug#7119 */
#endif

/* Architecture dependent constants */
#ifdef VTSS_ARCH_LUTON28
#define QOS_BITRATE_MIN 500        /* Minimum rate for policer/shaper (kbps) */
#define QOS_BITRATE_MAX 1000000    /* Maximum rate for policer/shaper (kbps) */
#define QOS_BITRATE_DEF 500        /* Default rate for policer/shaper (kbps) */
#define QOS_CLASS_CNT   4          /* Maximum number of QoS classes */
#define QOS_DPL_MAX     0          /* Maximum value for Drop Precedence Levels */
#elif defined(VTSS_ARCH_LUTON26)
#define QOS_BITRATE_MIN 100
#define QOS_BITRATE_MAX 3300000
#define QOS_BITRATE_DEF 500
#define QOS_CLASS_CNT   8
#define QOS_DPL_MAX     1
#elif defined(VTSS_ARCH_SERVAL)
#define QOS_BITRATE_MIN 100
#define QOS_BITRATE_MAX 3300000
#define QOS_BITRATE_DEF 500
#define QOS_CLASS_CNT   8
#define QOS_DPL_MAX     1
#elif defined(VTSS_ARCH_JAGUAR_1)
#define QOS_BITRATE_MIN 100
#define QOS_BITRATE_MAX 13200000
#define QOS_BITRATE_DEF 500
#define QOS_CLASS_CNT   8
#define QOS_DPL_MAX     3
#endif

/* Various array size definitions */

#if defined(VTSS_SW_OPTION_JR_STORM_POLICERS)
#define QOS_PORT_POLICER_CNT         1 /* Only use one policer (#0) as plain port policer in user interface */
/* Map policer 1, 2 and 3 as storm policers */
#define QOS_STORM_POLICER_UNICAST    1 /* Use policer #1 as storm policer for unicast frames */
#define QOS_STORM_POLICER_BROADCAST  2 /* Use policer #2 as storm policer for broadcast frames */
#define QOS_STORM_POLICER_UNKNOWN    3 /* Use policer #3 as storm policer for unknown frames */
#else
#define QOS_PORT_POLICER_CNT         VTSS_PORT_POLICERS
#endif /* defined(VTSS_SW_OPTION_JR_STORM_POLICERS) */
#define QOS_PORT_QUEUE_CNT           VTSS_QUEUE_ARRAY_SIZE
#define QOS_PORT_WEIGHTED_QUEUE_CNT (VTSS_QUEUE_ARRAY_SIZE - 2)
#define QOS_PORT_PRIO_CNT            VTSS_PRIO_ARRAY_SIZE
#define QOS_PORT_TR_DPL_CNT          4  /* 2 bit DP level gives 4 distinct values. TR is a short name for TagRemarking */

/* QOS error codes (vtss_rc) */
enum {
    QOS_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_QOS),  /* Generic error code */
    QOS_ERROR_PARM,           /* Illegal parameter */
    QOS_ERROR_QCE_NOT_FOUND,  /* QCE not found */
    QOS_ERROR_QCE_TABLE_FULL,  /* QCE table full */
#if defined(VTSS_FEATURE_QCL_V2)
    QOS_ERROR_QCL_USER_NOT_FOUND,  /* User not found */
    QOS_ERROR_STACK_STATE,     /* User not found */
    QOS_ERROR_REQ_TIMEOUT,     /* Msg request wait time out */
#endif
};

#if defined(VTSS_FEATURE_QOS_WRED)
/* Weighted Random Early Detection configuration */
typedef struct {
    BOOL       enable;          /**< Enable/disable WRED */
    vtss_pct_t min_th;          /**< Minimum threshold */
    vtss_pct_t max_prob_1;      /**< Maximum drop probability 1 */
    vtss_pct_t max_prob_2;      /**< Maximum drop probability 2 */
    vtss_pct_t max_prob_3;      /**< Maximum drop probability 3 */
} qos_wred_t;
#endif /* VTSS_FEATURE_QOS_WRED */

#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
/* DSCP global configuration */
typedef struct {
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    BOOL             dscp_trust[64]; /* Ingress: Only trusted DSCP values are used for QOS class and DP level classification */
    vtss_prio_t      dscp_qos_class_map[64]; /* Ingress: Mapping from DSCP value to QOS class */
    vtss_dp_level_t  dscp_dp_level_map[64]; /* Ingress: Mapping from DSCP value to DP level */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    vtss_dscp_t      translate_map[64]; /* Ingress: Translated DSCP value. Used when port.dscp_translate = TRUE */
    BOOL             ingress_remark[64]; /* Ingress: DSCP remarking enable. Used when port.dscp_mode = VTSS_DSCP_MODE_SEL */
    vtss_dscp_t      qos_class_dscp_map[VTSS_PRIO_ARRAY_SIZE]; /* Ingress: Mapping from QoS class to DSCP (DP unaware or DP level = 0) */
    vtss_dscp_t      egress_remap[64]; /* Egress: Remap one DSCP to another (DP unaware or DP level = 0) */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
    vtss_dscp_t      qos_class_dscp_map_dp1[VTSS_PRIO_ARRAY_SIZE]; /* Ingress: Mapping from QoS class to DSCP (DP aware and DP level = 1) */
    vtss_dscp_t      egress_remap_dp1[64]; /* Egress: Remap one DSCP to another (DP aware and DP level = 1) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
} qos_dscp_t;
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */

/* QOS General configuration */
typedef struct {
    vtss_prio_t prio_no;
#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH)
    vtss_packet_rate_t policer_mc;        /* Multicast packet policer */
    BOOL               policer_mc_status; /* status of Port Limiter (1:enable 0:disable) */
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) */
#if defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
    vtss_packet_rate_t policer_bc;        /* Broadcast packet policer */
    BOOL               policer_bc_status; /* status of Port Limiter (1:enable 0:disable) */
#endif /* defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) */
#if defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
    vtss_packet_rate_t policer_uc;        /* Unicast packet policer */
    BOOL               policer_uc_status; /* status of Port Limiter (1:enable 0:disable) */
#endif /* defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */
#if defined(VTSS_FEATURE_QOS_WRED)
    qos_wred_t         wred[QOS_PORT_WEIGHTED_QUEUE_CNT]; /* Weighted Random Early Detection */
#endif /* VTSS_FEATURE_QOS_WRED */
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    qos_dscp_t         dscp; /* DSCP ingress and egress config */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) || defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2) */
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
    u8                 header_size; /* header size to be subtracted from payload by rate limiter */
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */
#ifdef VTSS_FEATURE_VSTAX_V2
    BOOL               cmef_disable; /* Disable Congestion Management */
#endif /* VTSS_FEATURE_VSTAX_V2 */
} qos_conf_t;

/* QOS Policer configuration */
typedef struct {
    BOOL           enabled; /* Policer enabled state */
    vtss_policer_t policer; /* Policer configuration */
} qos_policer_t;

/* QOS Shaper configuration */
typedef struct {
    BOOL           enable; /* Shaper enabled state */
    vtss_bitrate_t rate;   /* Shaper bitrate */
} qos_shaper_t;

/* QOS port configuration */
typedef struct {
#ifdef VTSS_FEATURE_QCL_V1
    vtss_qcl_id_t          qcl_no;                                     /* QCL number */
#endif /* VTSS_FEATURE_QCL_V1 */
    vtss_prio_t            default_prio;                               /* Default port priority (QoS Class) */
    vtss_tagprio_t         usr_prio;                                   /* Default ingress VLAN tag priority (PCP) */
    qos_policer_t          port_policer[VTSS_PORT_POLICERS];           /* Ingress port policers */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT)
    vtss_policer_ext_t     port_policer_ext[VTSS_PORT_POLICERS];       /* Ingress port policers extensions */
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT */
#if defined(VTSS_SW_OPTION_BUILD_CE)
#ifdef VTSS_FEATURE_QOS_QUEUE_POLICER
    qos_policer_t          queue_policer[QOS_PORT_QUEUE_CNT];          /* Queue policers */
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */
#endif /* defined(VTSS_SW_OPTION_BUILD_CE) */
    vtss_bitrate_t         shaper_rate;                                /* Rate of Port Shaper */
    BOOL                   shaper_status;                              /* status of Port Shaper (1:enable 0:disable) */
#ifdef VTSS_FEATURE_QOS_CLASSIFICATION_V2
    vtss_dp_level_t        default_dpl;                                             /**< Default Ingress Drop Precedence level */
    vtss_dei_t             default_dei;                                             /**< Default Ingress DEI value  */
    BOOL                   tag_class_enable;                                        /**< Ingress classification based on vlan tag (PCP and DEI) */
    vtss_prio_t            qos_class_map[VTSS_PCP_ARRAY_SIZE][VTSS_DEI_ARRAY_SIZE]; /**< Ingress mapping for tagged frames from PCP and DEI to QOS class  */
    vtss_dp_level_t        dp_level_map[VTSS_PCP_ARRAY_SIZE][VTSS_DEI_ARRAY_SIZE];  /**< Ingress mapping for tagged frames from PCP and DEI to DP level */
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
    BOOL                   dscp_class_enable;      /** Ingress classification of QoS class and DP level based on DSCP */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
#ifdef VTSS_FEATURE_QOS_WFQ_PORT
    BOOL                   wfq_enable;
    vtss_weight_t          weight[QOS_PORT_QUEUE_CNT];
#endif /* VTSS_FEATURE_QOS_WFQ_PORT */
#ifdef VTSS_FEATURE_QOS_SCHEDULER_V2
    BOOL                   dwrr_enable;                                /* Enable Weighted fairness queueing */
    vtss_pct_t             queue_pct[QOS_PORT_WEIGHTED_QUEUE_CNT];     /* Queue percentages */
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */
#ifdef VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS
    qos_shaper_t           queue_shaper[QOS_PORT_QUEUE_CNT];           /* Queue shapers */
    BOOL                   excess_enable[QOS_PORT_QUEUE_CNT];          /* Allow this queue to use excess bandwidth */
#endif /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */
#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
    vtss_tag_remark_mode_t tag_remark_mode;                            /* Egress tag remark mode */
    vtss_tagprio_t         tag_default_pcp;                            /* Default PCP value for Egress port */
    vtss_dei_t             tag_default_dei;                            /* Default DEI value for Egress port */
    vtss_dei_t             tag_dp_map[QOS_PORT_TR_DPL_CNT];            /* Egress mapping from 2 bit DP level to 1 bit DP level */
    vtss_tagprio_t         tag_pcp_map[QOS_PORT_PRIO_CNT][2];          /* Egress mapping from QOS class and DP level to PCP */
    vtss_dei_t             tag_dei_map[QOS_PORT_PRIO_CNT][2];          /* Egress mapping from QOS class and DP level to DEI */
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */
#ifdef VTSS_FEATURE_QOS_DSCP_REMARK
#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE)
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
    BOOL                   qos_remarking_enable;                       /* status of DSCP Remarking (1:enable 0:disable) */
    vtss_dscp_t            dscp_remarking_val[VTSS_QUEUE_ARRAY_SIZE];
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    BOOL                   dscp_translate;     /* Ingress: Translate DSCP value before use */
    vtss_dscp_mode_t       dscp_imode;         /* Ingress DSCP mode */
    vtss_dscp_emode_t      dscp_emode;         /* Egress DSCP mode */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
#endif /* defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_CE) */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
    vtss_qos_port_dot3ar_rate_limiter_t tx_rate_limiter;         /* 802.3ar rate limiter */
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */

} qos_port_conf_t;

#if defined(VTSS_FEATURE_QCL_V1)
/* QCL ID numbers */
#ifdef VTSS_SW_OPTION_VOICE_VLAN
#define RESERVED_QCL_CNT            1
#else
#define RESERVED_QCL_CNT            0
#endif

#if VTSS_SWITCH_STACKABLE
#define QCL_MAX (VTSS_QCL_IDS - 2 - RESERVED_QCL_CNT)
#else
#define QCL_MAX (VTSS_QCL_IDS - RESERVED_QCL_CNT)
#endif /* VTSS_SWITCH_STACKABLE */

#elif defined(VTSS_FEATURE_QCL_V2)

/* Only one QCL list */
#define RESERVED_QCL_CNT 0
#define QCL_MAX (VTSS_QCL_IDS - RESERVED_QCL_CNT)

#ifdef VTSS_SW_OPTION_VOICE_VLAN
#define RESERVED_QCE_CNT    1
#else
#define RESERVED_QCE_CNT    0
#endif

#else
#error "No support for this QCL version"
#endif

#define QCL_ID_NONE  0          /* Reserved */
#define QCL_ID_START 1          /* First QCL ID */
#define QCL_ID_END   QCL_MAX    /* Last QCL ID */

/* QCE ID numbers */
#if defined(VTSS_FEATURE_QCL_V1)

#define QCE_MAX      24
#define QCE_ID_NONE  0       /* Reserved */
#define QCE_ID_START 1       /* First QCE ID */
#define QCE_ID_END   QCE_MAX /* Last QCE ID */

#elif defined(VTSS_FEATURE_QCL_V2)

#if defined(VTSS_ARCH_JAGUAR_1)
#define QCE_MAX      256  /* Limiting to 256 for Jaguar_1 at this point though IS1 support entries */
#else
#define QCE_MAX      256
#endif

#define QCE_ID_NONE  0       /* Reserved */
#define QCE_ID_START 1       /* First QCE ID */

#if VTSS_SWITCH_STACKABLE
#define QCE_ID_END   (VTSS_ISID_CNT * QCE_MAX)  /* Last QCE ID */
#else
#define QCE_ID_END   (QCE_MAX)                  /* Last QCE ID */
#endif /* VTSS_SWITCH_STACKABLE */

#endif /* VTSS_FEATURE_QCL_V2 */

/* RATE STATUS */
#define RATE_STATUS_ENABLE  1
#define RATE_STATUS_DISABLE 0

/* Default setting */
#define QOS_PORT_DEFAULT_QCL    QCL_ID_START

/* Value to use to disable volatile default priority */
#define QOS_PORT_PRIO_UNDEF     0xffffffff

#if defined(VTSS_FEATURE_QCL_V2)
/* QCL users declaration */
typedef enum {
    QCL_USER_STATIC = 0,
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    QCL_USER_VOICE_VLAN,
#endif
    QCL_USER_CNT
} qcl_user_t;

extern const char *const qcl_user_names[QCL_USER_CNT];

/* data types used insdie QCE */
typedef vtss_vcap_bit_t        vtss_qce_bit_t;
typedef vtss_vcap_u8_t         vtss_qce_u8_t;
typedef vtss_vcap_u16_t        vtss_qce_u16_t;
typedef vtss_vcap_u24_t        vtss_qce_u24_t;
typedef vtss_vcap_u32_t        vtss_qce_u32_t;
typedef vtss_vcap_ip_t         vtss_qce_ip_t;
typedef u8                     qos_qce_prio_t;

/* VID and UDP/TCP Port range parameters */
typedef struct {
    BOOL           in_range;   /* range match */

    union {
        struct {
            u16            value;      /* value */
            u16            mask;       /* mask */
        } v;
        struct {
            u16            low;        /* low value */
            u16            high;       /*  high value */
        } r;
    } vr;
} qos_qce_vr_u16_t;

/* DSCP range parameters */
typedef struct {
    BOOL           in_range;   /* range match */

    union {
        struct {
            u8            value;      /* value */
            u8            mask;       /* mask */
        } v;
        struct {
            u8            low;        /* low value */
            u8            high;       /*  high value */
        } r;
    } vr;
} qos_qce_vr_u8_t;

enum {
    QOS_QCE_DSCP_BE = 0,
    QOS_QCE_DSCP_CS1 = 8,
    QOS_QCE_DSCP_CS2 = 16,
    QOS_QCE_DSCP_CS3 = 24,
    QOS_QCE_DSCP_CS4 = 32,
    QOS_QCE_DSCP_CS5 = 40,
    QOS_QCE_DSCP_CS6 = 48,
    QOS_QCE_DSCP_CS7 = 56,
    QOS_QCE_DSCP_EF = 46,

    QOS_QCE_DSCP_AF11 = 10,
    QOS_QCE_DSCP_AF12 = 12,
    QOS_QCE_DSCP_AF13 = 14,
    QOS_QCE_DSCP_AF21 = 18,
    QOS_QCE_DSCP_AF22 = 20,
    QOS_QCE_DSCP_AF23 = 22,
    QOS_QCE_DSCP_AF31 = 26,
    QOS_QCE_DSCP_AF32 = 28,
    QOS_QCE_DSCP_AF33 = 30,
    QOS_QCE_DSCP_AF41 = 34,
    QOS_QCE_DSCP_AF42 = 36,
    QOS_QCE_DSCP_AF43 = 38,
};

/* DMAC Type */
typedef enum {
    QOS_QCE_DMAC_TYPE_ANY,
    QOS_QCE_DMAC_TYPE_UC,
    QOS_QCE_DMAC_TYPE_MC,
    QOS_QCE_DMAC_TYPE_BC,
} qos_qce_dmac_type_t;

/* QCE key bit field name: which internally specify the start bit pos */
enum {
    QOS_QCE_VLAN_TAG      = 0,                     /* VLAN Tag */
    QOS_QCE_VLAN_DEI      = QOS_QCE_VLAN_TAG + 2,  /* DEI */
    QOS_QCE_DMAC_TYPE     = QOS_QCE_VLAN_DEI + 2,  /* DMAC Type */
    QOS_QCE_IPV4_FRAGMENT = QOS_QCE_DMAC_TYPE + 2, /* IP fragment */
};

/* Following bit operation assumes that key bit fields are each 2 bit long */
#define QCE_ENTRY_CONF_KEY_SET(b, n, v)                         \
    {                                                           \
        b &= ~(0x03 << (n));                                    \
        /*lint -e{506} */                                       \
        if (v) {                                                \
            b |= ((v)<<(n));                                    \
        }                                                       \
    }
#define QCE_ENTRY_CONF_KEY_GET(b, n)     ((b >> (n)) & 0x03)

/* QCE action bit field name which internally uses it as bit start pos */
enum {
    QOS_QCE_ACTION_PRIO,
    QOS_QCE_ACTION_DP,
    QOS_QCE_ACTION_DSCP,
};

#define QCE_ENTRY_CONF_ACTION_SET(b, n, v)                      \
    {                                                           \
        /*lint -e{506} */                                       \
        if (v) { b |= (1U<<(n)); } else { b &= ~(1U<<(n)); }    \
    }
#define QCE_ENTRY_CONF_ACTION_GET(b, n)     ((b & (1U<<(n))) ? 1 : 0)

/* QCE Key */
typedef struct {
    u16                        key_bits; /* Bit fields */
    qos_qce_vr_u16_t           vid;      /* VLAN range */
    vtss_qce_u8_t              pcp;      /* User priority */
    vtss_qce_u24_t             smac;     /* Source MAC: 24 MSB (OUI) */

    union {
        /* Type VTSS_QCE_TYPE_ETYPE */
        struct {
            vtss_qce_u16_t     etype;    /* Ethernet Type */
        } etype;

        /* Type VTSS_QCE_TYPE_LLC */
        struct {
            vtss_qce_u8_t      dsap;     /* DSAP */
            vtss_qce_u8_t      ssap;     /* SSAP */
            vtss_qce_u8_t      control;  /* LLC Control */
        } llc;

        /* Type VTSS_QCE_TYPE_SNAP */
        struct {
            vtss_qce_u16_t     pid;      /* Protocol ID i.e. EtherType */
        } snap;

        /* Type VTSS_QCE_TYPE_IPV4 */
        struct {
            qos_qce_vr_u8_t    dscp;     /* DSCP field (6 bit) */
            vtss_qce_u8_t      proto;    /* Protocol */
            vtss_qce_ip_t      sip;      /* Source IP address */
            qos_qce_vr_u16_t   sport;    /* UDP/TCP: Source port */
            qos_qce_vr_u16_t   dport;    /* UDP/TCP: Destination port */
        } ipv4;

        /* Type VTSS_QCE_TYPE_IPV6 */
        struct {
            qos_qce_vr_u8_t    dscp;     /* DSCP field (6 bit) */
            vtss_qce_u8_t      proto;    /* Protocol */
            vtss_qce_u32_t     sip;      /* IPv6 source address (32 LSB) */
            qos_qce_vr_u16_t   sport;    /* UDP/TCP: Source port */
            qos_qce_vr_u16_t   dport;    /* UDP/TCP: Destination port */
        } ipv6;
    } frame;
} qos_qce_key_t;

/* QCE action */
typedef struct {
    u8                 action_bits;    /* Bit fields */
    qos_qce_prio_t     prio;           /* Priority value */
    vtss_dp_level_t    dp;             /* DP value */
    vtss_dscp_t        dscp;           /* DSCP value */
} qos_qce_action_t;
#endif /* VTSS_FEATURE_QCL_V2 */

/* QoS entry configuration */
typedef struct {
    vtss_qce_id_t     id;
    vtss_qce_type_t   type;

#ifdef VTSS_FEATURE_QCL_V2
    BOOL              conflict; /* Volatile QCE conflict flag */
    vtss_isid_t       isid; /* Switch ID */
    u8                port_list[VTSS_PORT_BF_SIZE]; /* Port list */
    qos_qce_key_t     key;  /* QCE Key */
    qos_qce_action_t  action; /* QCE action */
#endif

#ifdef VTSS_FEATURE_QCL_V1
    union {
        /* Type VTSS_QCE_TYPE_ETYPE */
        struct {
            vtss_etype_t val;  /* Ethernet Type */
            vtss_prio_t  prio; /* Priority mapping */
        } etype;

        /* Type VTSS_QCE_TYPE_VLAN_ID */
        struct {
            vtss_vid_t  vid;  /* VLAN ID value */
            vtss_prio_t prio; /* Priority mapping */
        } vlan;

        /* Type VTSS_QCE_TYPE_UDP_TCP */
        struct {
            vtss_udp_tcp_t low;  /* UDP/TCP port range low value */
            vtss_udp_tcp_t high; /* UDP/TCP port range high value */
            vtss_prio_t prio;    /* Priority mapping */
        } udp_tcp;

        /* Type VTSS_QCE_TYPE_DSCP */
        struct {
            vtss_dscp_t dscp_val; /* DSCP value */
            vtss_prio_t prio; /* Priority mapping */
        } dscp;

        /* Type VTSS_QCE_TYPE_TOS */
        vtss_prio_t tos_prio[8]; /* ToS priority mapping */

        /* Type VTSS_QCE_TYPE_TAG */
        vtss_prio_t tag_prio[8]; /* Tag priority mapping */
    } frame;
#endif
} qos_qce_entry_conf_t;

/* QOS error text */
char *qos_error_txt(vtss_rc rc);

/* QOS DSCP value to string */
const char *qos_dscp2str(vtss_dscp_t dscp);

#ifdef VTSS_FEATURE_QOS_TAG_REMARK_V2
/* QoS tag remarking mode text string */
char *qos_port_tag_remarking_mode_txt(vtss_tag_remark_mode_t mode);
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

/* Set QoS General configuration  */
vtss_rc qos_conf_set(qos_conf_t *conf);

/* Get QoS General configuration  */
vtss_rc qos_conf_get(qos_conf_t *conf);

/* Get default QoS General configuration  */
vtss_rc qos_conf_get_default(qos_conf_t *conf);

/* Set port QoS configuration  */
vtss_rc qos_port_conf_set(vtss_isid_t isid, vtss_port_no_t port_no, qos_port_conf_t *conf);

/* Get port QoS configuration  */
vtss_rc qos_port_conf_get(vtss_isid_t isid, vtss_port_no_t port_no, qos_port_conf_t *conf);

/* Get default port QoS configuration  */
vtss_rc qos_port_conf_get_default(qos_port_conf_t *conf);

/* Set port QoS volatile default priority. Use QOS_PORT_PRIO_UNDEF to disable */
vtss_rc qos_port_volatile_set_default_prio(vtss_isid_t isid, vtss_port_no_t port_no, vtss_prio_t default_prio);

/* Get port QoS volatile default priority. Returns QOS_PORT_PRIO_UNDEF if not set */
vtss_rc qos_port_volatile_get_default_prio(vtss_isid_t isid, vtss_port_no_t port_no, vtss_prio_t *default_prio);

/* ================================================================= *
 *  QoS port configuration change events
 * ================================================================= */

/**
 * \brief QoS port configuration change callback.
 *
 * Global callbacks are executed in management thread context (e.g Console, Telnet, SSH or Web) and are not time sensitive.
 * Local callbacks are executed in msg rx thread context and must NOT contain lengthy operations.
 *
 * \param isid     [IN]  Switch ID on global registrations, otherwise VTSS_ISID_LOCAL.
 * \param iport    [IN]  Port number.
 * \param conf     [OUT] New configuration.
 *
 * \return Nothing.
 */
typedef void (*qos_port_conf_change_cb_t)(const vtss_isid_t isid, const vtss_port_no_t iport, const qos_port_conf_t *const conf);

/**
 * \brief QoS port configuration change callback registration.
 *
 * \param global    [IN]  FALSE: Callback is called on local switch only (master or slave) with isid == VTSS_ISID_LOCAL.
 *                        TRUE:  Callback is called on master only and contains actual isid.
 *                               Use FALSE if you module is distributed among all switches.
 *                               Use TRUE if you module is centralized on the master.
 * \param module_id [IN]  Callers module ID.
 * \param callback  [IN]  Callback function to be called on QoS port configuration changes.
 *
 * \return VTSS_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
vtss_rc qos_port_conf_change_register(BOOL global, vtss_module_id_t module_id, qos_port_conf_change_cb_t callback);

/* QoS port configuration change registration info - for debug only */
typedef struct {
    BOOL                      global;    /* Local or global */
    vtss_module_id_t          module_id; /* Module ID */
    qos_port_conf_change_cb_t callback;  /* User callback function */
    cyg_tick_count_t          max_ticks; /* Maximum ticks */
} qos_port_conf_change_reg_t;

/* Get/clear QoS port configuration change registration info - for debug only  */
vtss_rc qos_port_conf_change_reg_get(qos_port_conf_change_reg_t *entry, BOOL clear);

#if defined(VTSS_FEATURE_QCL_V1)
/* Get QCE or next QCE in the specific QCL (use QCE_ID_NONE to get first) (if "qce_id" = QCE_ID_NONE , the value of "next" will be discarded) */
vtss_rc qos_mgmt_qce_entry_get(vtss_qcl_id_t qcl_id, vtss_qce_id_t qce_id, qos_qce_entry_conf_t *conf, BOOL next);

/* Add QCE entry before given QCE or last (QCE_ID_NONE) to the specific QCL */
vtss_rc qos_mgmt_qce_entry_add(vtss_qcl_id_t qcl_id, vtss_qce_id_t qce_id, qos_qce_entry_conf_t *conf);

/* Delete QCE in the specifci QCL */
vtss_rc qos_mgmt_qce_entry_del(vtss_qcl_id_t qcl_id, vtss_qce_id_t qce_id);
#endif /* VTSS_FEATURE_QCL_V1 */
#if defined(VTSS_FEATURE_QCL_V2)
/* Get QCE or next QCE in the specific QCL (use QCE_ID_NONE to get first) (if "qce_id" = QCE_ID_NONE , the value of "next" will be discarded) */
vtss_rc qos_mgmt_qce_entry_get(vtss_isid_t isid, qcl_user_t user_id,
                               vtss_qcl_id_t qcl_id, vtss_qce_id_t qce_id,
                               qos_qce_entry_conf_t *conf, BOOL next);
/* Add QCE entry before given QCE or last (QCE_ID_NONE) to the specific QCL */
vtss_rc qos_mgmt_qce_entry_add(qcl_user_t user_id, vtss_qcl_id_t qcl_id,
                               vtss_qce_id_t qce_id, qos_qce_entry_conf_t *conf);
/* Delete QCE in the specific QCL */
vtss_rc qos_mgmt_qce_entry_del(vtss_isid_t isid, qcl_user_t user_id,
                               vtss_qcl_id_t qcl_id, vtss_qce_id_t qce_id);
/* Resolve QCE conflict */
vtss_rc qos_mgmt_qce_conflict_resolve(vtss_isid_t isid, qcl_user_t user_id,
                                      vtss_qcl_id_t qcl_id);
#endif /* VTSS_FEATURE_QCL_V2 */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)

typedef struct {
    BOOL          shaper_host_status;         /**< Host shaper status */
    vtss_shaper_t shaper;                     /**< Host port shaper */
    u8            pct;                        /**< lport pct */
    vtss_red_t    red[VTSS_PRIOS];            /**< Random Early Detection configurations*/
    struct {
        vtss_bitrate_t rate;                   /**< Min guaranted port rate */
        vtss_pct_t     queue_pct[6];           /**< DWRR queue percentage */
        BOOL           shaper_queue_status[2]; /**< Queue shaper status   */
        vtss_shaper_t  queue[2];               /**< Strict queue shaper */
    } scheduler;
} qos_lport_conf_t;

/* Set lport QoS configuration  */
vtss_rc qos_mgmt_lport_conf_set(vtss_lport_no_t lport_no, const qos_lport_conf_t *const conf);

/* Get lport QoS configuration  */
vtss_rc qos_mgmt_lport_conf_get(vtss_lport_no_t lport_no,       qos_lport_conf_t *const conf);

#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

/* Initialize module */
vtss_rc qos_init(vtss_init_data_t *data);

#endif /* _VTSS_QOS_APPL_API_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
