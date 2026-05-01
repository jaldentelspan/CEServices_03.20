/*

 Vitesse API software.

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

#ifndef _VTSS_STATE_H_
#define _VTSS_STATE_H_

#ifdef VTSS_CHIP_CU_PHY
#include "vtss_phy.h"
#endif
#ifdef VTSS_CHIP_10G_PHY
#include "vtss_phy_10g.h"
#endif
#ifdef VTSS_FEATURE_WIS
#include "vtss_wis_api.h"
#include "vtss_wis.h"
#endif
#ifdef VTSS_FEATURE_OTN
#include "vtss_otn_api.h"
#endif
#include "vtss_phy_ts_api.h"
#if defined(VTSS_FEATURE_FDMA)
#include "vtss_fdma.h"
#endif
#if defined(VTSS_FEATURE_ANEG)
#include "vtss_aneg_api.h"
#endif
#if defined(VTSS_FEATURE_XFI)
#include "vtss_xfi_api.h"
#endif
#if defined(VTSS_FEATURE_SFI4)
#include "vtss_sfi4_api.h"
#endif
#if defined(VTSS_FEATURE_TFI5)
#include "vtss_tfi5_api.h"
#endif
#if defined(VTSS_FEATURE_XAUI)
#include "vtss_xaui_api.h"
#endif
#if defined(VTSS_FEATURE_UPI)
#include "vtss_upi_api.h"
#endif
#if defined(VTSS_FEATURE_TFI5)
#include "vtss_tfi5_api.h"
#endif
#if defined(VTSS_FEATURE_GFP)
#include "vtss_gfp_api.h"
#endif
#if defined(VTSS_FEATURE_RAB)
#include "vtss_rab_api.h"
#endif
#if defined(VTSS_FEATURE_AE)
#include "vtss_ae_api.h"
#endif
#if defined(VTSS_FEATURE_MAC10G)
#include "vtss_mac10g_api.h"
#endif
#if defined(VTSS_FEATURE_PCS_10GBASE_R)
#include "vtss_pcs_10gbase_r_api.h"
#endif
#if defined(VTSS_FEATURE_I2C)
#include "vtss_i2c_api.h"
#endif
#if defined(VTSS_FEATURE_OHA)
#include "vtss_oha_api.h"
#endif
#if defined(VTSS_FEATURE_MPLS)
#include "vtss_mpls_api.h"
#endif
#if defined(VTSS_ARCH_SERVAL)
#include "vtss_serval.h"
#endif
#include "vtss_misc_api.h"

/* Call Chip Interface Layer function if it exists */
#define VTSS_FUNC(func, ...) (vtss_state->cil_func.func == NULL ? VTSS_RC_ERROR : vtss_state->cil_func.func(__VA_ARGS__))

/* Call Chip Interface Layer function if it exists, given the state as an argument. This is useful for functions that don't require vtss_state to be set-up. */
#define VTSS_FUNC_FROM_STATE(state, func, ...) ((state == NULL || state->cil_func.func == NULL) ? VTSS_RC_ERROR : state->cil_func.func(state, __VA_ARGS__))

/* Call Chip Interface Layer function if it exists, return error code, if error */
#define VTSS_FUNC_RC(func, ...) { vtss_rc __rc__ = (vtss_state->cil_func.func == NULL ? VTSS_RC_ERROR : vtss_state->cil_func.func(__VA_ARGS__)); if (__rc__ < VTSS_RC_OK) return __rc__; }

/* Call Chip Interface Layer function if it exists and we are in cold start mode */
#define VTSS_FUNC_COLD(func, ...) (vtss_state->cil_func.func == NULL ? VTSS_RC_ERROR : vtss_state->warm_start_cur ? VTSS_RC_OK : vtss_state->cil_func.func(__VA_ARGS__))

/* Call function in cold start mode only */
#define VTSS_RC_COLD(expr) (vtss_state->warm_start_cur ? VTSS_RC_OK : (expr))

/* Set currently selected device */
#define VTSS_SELECT_CHIP(__chip_no__) { (vtss_state->chip_no) = (__chip_no__); }
#define VTSS_SELECT_CHIP_PORT_NO(port_no) VTSS_SELECT_CHIP(vtss_state->port_map[port_no].chip_no)
/* API enter/exit macros for protection */
extern const char *vtss_func;
#define VTSS_ENTER(...) { vtss_callout_lock(__FUNCTION__); vtss_func = __FUNCTION__; }
#define VTSS_EXIT(...) { vtss_callout_unlock(__FUNCTION__); }
#define VTSS_EXIT_ENTER(...) { vtss_state_t *old_state = vtss_state; vtss_chip_no_t old_chip = vtss_state->chip_no; vtss_callout_unlock(__FUNCTION__); vtss_callout_lock(__FUNCTION__); vtss_state = old_state; vtss_state->chip_no = old_chip; }

#define VTSS_RC(expr) { vtss_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) return __rc__; }

#define VTSS_BOOL(expr) ((expr) ? 1 : 0)

/* Check instance */
vtss_rc vtss_inst_check(const vtss_inst_t inst);

/* Check instance and port number */
vtss_rc vtss_inst_port_no_check(const vtss_inst_t    inst,
                                const vtss_port_no_t port_no);

#define VTSS_CHIP_PORT(port_no)                   vtss_state->port_map[port_no].chip_port
#define VTSS_CHIP_NO(port_no)                     vtss_state->port_map[port_no].chip_no
#define VTSS_CHIP_PORT_FROM_STATE(state, port_no) (state)->port_map[port_no].chip_port
#define VTSS_CHIP_NO_FROM_STATE(state, port_no)   (state)->port_map[port_no].chip_no
#define VTSS_PORT_CHIP_SELECTED(port_no)          (vtss_state->port_map[port_no].chip_no == vtss_state->chip_no)

/* Bit field macros */
#define VTSS_BF_SIZE(n)      ((n+7)/8)
#define VTSS_BF_GET(a, n)    ((a[(n)/8] & (1<<((n)%8))) ? 1 : 0)
#define VTSS_BF_SET(a, n, v) { u8 m=(1<<((n)%8)); if (v) { a[(n)/8] |= m; } else { a[(n)/8] &= ~m; }}
#define VTSS_BF_CLR(a, n)    (memset(a, 0, VTSS_BF_SIZE(n)))

/* Port member bit field macros */
#define VTSS_PORT_BF_SIZE                VTSS_BF_SIZE(VTSS_PORTS)
#define VTSS_PORT_BF_GET(a, port_no)     VTSS_BF_GET(a, (port_no) - VTSS_PORT_NO_START)
#define VTSS_PORT_BF_SET(a, port_no, v)  VTSS_BF_SET(a, (port_no) - VTSS_PORT_NO_START, v)
#define VTSS_PORT_BF_CLR(a)              VTSS_BF_CLR(a, VTSS_PORTS)

/* Number of chips per instance */
#define VTSS_CHIP_CNT 2

/* Get array size */
#define VTSS_ARRSZ(t) /*lint -e{574} */ (sizeof(t)/sizeof(t[0])) /* Suppress Lint Warning 574: Signed-unsigned mix with relational */

/* ================================================================= *
 *  Internal VTSS_FEATURE_* defines
 * ================================================================= */
#if defined(VTSS_ARCH_LUTON28)
#define VTSS_FEATURE_VCAP /* VCAP */
#define VTSS_FEATURE_IS2  /* VCAP IS2 */
#endif /* VTSS_ARCH_LUTON28 */

#if defined(VTSS_ARCH_LUTON26)
#define VTSS_FEATURE_VCAP /* VCAP */
#define VTSS_FEATURE_IS1  /* VCAP IS1 */
#define VTSS_FEATURE_IS2  /* VCAP IS2 */
#define VTSS_FEATURE_ES0  /* VCAP ES0 */
#endif /* VTSS_ARCH_LUTON26 */

#if defined(VTSS_ARCH_SERVAL)
#define VTSS_FEATURE_VCAP /* VCAP */
#if defined(VTSS_ARCH_SERVAL_CE)
#define VTSS_FEATURE_IS0  /* VCAP IS0 */
#endif
#define VTSS_FEATURE_IS1  /* VCAP IS1 */
#define VTSS_FEATURE_IS2  /* VCAP IS2 */
#define VTSS_FEATURE_ES0  /* VCAP ES0 */
#endif /* SERVAL */

#if defined(VTSS_ARCH_JAGUAR_1)
#define VTSS_FEATURE_VCAP /* VCAP */
#define VTSS_FEATURE_IS0  /* VCAP IS0 */
#define VTSS_FEATURE_IS1  /* VCAP IS1 */
#define VTSS_FEATURE_IS2  /* VCAP IS2 */
#define VTSS_FEATURE_ES0  /* VCAP ES0 */
#endif /* VTSS_ARCH_JAGUAR_1 */

/* Target architecture */
typedef enum {
    VTSS_ARCH_CU_PHY,  /* Cu PHYs */
    VTSS_ARCH_10G_PHY, /* 10G PHYs */
    VTSS_ARCH_BR2,     /* Barrington-II */
    VTSS_ARCH_L28,     /* Luton28 */
    VTSS_ARCH_L26,     /* Luton26 */
    VTSS_ARCH_SRVL,    /* Serval */
    VTSS_ARCH_JR1,     /* Jaguar-1 */
    VTSS_ARCH_DAYT     /* Daytona */
} vtss_arch_t;

/* ================================================================= *
 *  Miscellaneous
 * ================================================================= */

/* Warm start scratch-pad 32-bit register layout */
#define VTSS_RESTART_VERSION_OFFSET 0
#define VTSS_RESTART_VERSION_WIDTH  16
#define VTSS_RESTART_TYPE_OFFSET    16
#define VTSS_RESTART_TYPE_WIDTH     3

/* API version used to determine if warm start can be done while downgrading/upgrading */
#define VTSS_API_VERSION 1

/* Ethernet types */
#define VTSS_ETYPE_TAG_C 0x8100
#define VTSS_ETYPE_TAG_S 0x88a8

/* ================================================================= *
 *  Port control
 * ================================================================= */

#if defined(VTSS_FEATURE_PORT_CONTROL)  || defined(VTSS_FEATURE_OAM)
typedef struct {
    u64 value; /**< Accumulated value (64 bit) */
    u64 prev;  /**< Previous value read (32 or 40 bit) */
} vtss_chip_counter_t;
#endif

#if defined(VTSS_FEATURE_PORT_CONTROL)

typedef struct {
    /* Rx counters */
    vtss_chip_counter_t rx_octets;
    vtss_chip_counter_t rx_drops;
    vtss_chip_counter_t rx_packets;
    vtss_chip_counter_t rx_broadcasts;
    vtss_chip_counter_t rx_multicasts;
    vtss_chip_counter_t rx_crc_align_errors;
    vtss_chip_counter_t rx_shorts;
    vtss_chip_counter_t rx_longs;
    vtss_chip_counter_t rx_fragments;
    vtss_chip_counter_t rx_jabbers;
    vtss_chip_counter_t rx_64;
    vtss_chip_counter_t rx_65_127;
    vtss_chip_counter_t rx_128_255;
    vtss_chip_counter_t rx_256_511;
    vtss_chip_counter_t rx_512_1023;
    vtss_chip_counter_t rx_1024_1526;
    vtss_chip_counter_t rx_1527_max;

    /* Tx counters */
    vtss_chip_counter_t tx_octets;
    vtss_chip_counter_t tx_drops;
    vtss_chip_counter_t tx_packets;
    vtss_chip_counter_t tx_broadcasts;
    vtss_chip_counter_t tx_multicasts;
    vtss_chip_counter_t tx_collisions;
    vtss_chip_counter_t tx_64;
    vtss_chip_counter_t tx_65_127;
    vtss_chip_counter_t tx_128_255;
    vtss_chip_counter_t tx_256_511;
    vtss_chip_counter_t tx_512_1023;
    vtss_chip_counter_t tx_1024_1526;
    vtss_chip_counter_t tx_1527_max;
    vtss_chip_counter_t tx_fifo_drops;
    vtss_chip_counter_t rx_pauses;
    vtss_chip_counter_t tx_pauses;
    vtss_chip_counter_t rx_classified_drops;
    vtss_chip_counter_t rx_class[VTSS_PRIOS];
    vtss_chip_counter_t tx_class[VTSS_PRIOS];
    vtss_chip_counter_t rx_local_drops;
    vtss_chip_counter_t rx_unicast;
    vtss_chip_counter_t tx_unicast;
    vtss_chip_counter_t tx_aging;
} vtss_port_luton28_counters_t;

typedef struct {
    /* Rx counters */
    vtss_chip_counter_t rx_octets;
    vtss_chip_counter_t rx_broadcast;
    vtss_chip_counter_t rx_multicast;
    vtss_chip_counter_t rx_unicast;
    vtss_chip_counter_t rx_shorts;
    vtss_chip_counter_t rx_fragments;
    vtss_chip_counter_t rx_jabbers;
    vtss_chip_counter_t rx_crc_align_errors;
    vtss_chip_counter_t rx_symbol_errors;
    vtss_chip_counter_t rx_64;
    vtss_chip_counter_t rx_65_127;
    vtss_chip_counter_t rx_128_255;
    vtss_chip_counter_t rx_256_511;
    vtss_chip_counter_t rx_512_1023;
    vtss_chip_counter_t rx_1024_1526;
    vtss_chip_counter_t rx_1527_max;
    vtss_chip_counter_t rx_pause;
    vtss_chip_counter_t rx_control;
    vtss_chip_counter_t rx_longs;
    vtss_chip_counter_t rx_classified_drops;
    vtss_chip_counter_t rx_red_class[VTSS_PRIOS];
    vtss_chip_counter_t rx_yellow_class[VTSS_PRIOS];
    vtss_chip_counter_t rx_green_class[VTSS_PRIOS];
    /* Drop counters */
    vtss_chip_counter_t dr_local;
    vtss_chip_counter_t dr_tail;
    vtss_chip_counter_t dr_yellow_class[VTSS_PRIOS];
    vtss_chip_counter_t dr_green_class[VTSS_PRIOS];
    /* Tx counters */
    vtss_chip_counter_t tx_octets;
    vtss_chip_counter_t tx_unicast;
    vtss_chip_counter_t tx_multicast;
    vtss_chip_counter_t tx_broadcast;
    vtss_chip_counter_t tx_collision;
    vtss_chip_counter_t tx_drops;
    vtss_chip_counter_t tx_pause;
    vtss_chip_counter_t tx_64;
    vtss_chip_counter_t tx_65_127;
    vtss_chip_counter_t tx_128_255;
    vtss_chip_counter_t tx_256_511;
    vtss_chip_counter_t tx_512_1023;
    vtss_chip_counter_t tx_1024_1526;
    vtss_chip_counter_t tx_1527_max;
    vtss_chip_counter_t tx_yellow_class[VTSS_PRIOS];
    vtss_chip_counter_t tx_green_class[VTSS_PRIOS];
    vtss_chip_counter_t tx_aging;
} vtss_port_luton26_counters_t;

typedef struct {
    /* Rx counters */
    vtss_chip_counter_t rx_in_bytes;
    vtss_chip_counter_t rx_symbol_carrier_err;
    vtss_chip_counter_t rx_pause;
    vtss_chip_counter_t rx_unsup_opcode;
    vtss_chip_counter_t rx_ok_bytes;
    vtss_chip_counter_t rx_bad_bytes;
    vtss_chip_counter_t rx_unicast;
    vtss_chip_counter_t rx_multicast;
    vtss_chip_counter_t rx_broadcast;
    vtss_chip_counter_t rx_crc_err;
    vtss_chip_counter_t rx_undersize;
    vtss_chip_counter_t rx_fragments;
    vtss_chip_counter_t rx_in_range_length_err;
    vtss_chip_counter_t rx_out_of_range_length_err;
    vtss_chip_counter_t rx_oversize;
    vtss_chip_counter_t rx_jabbers;
    vtss_chip_counter_t rx_size64;
    vtss_chip_counter_t rx_size65_127;
    vtss_chip_counter_t rx_size128_255;
    vtss_chip_counter_t rx_size256_511;
    vtss_chip_counter_t rx_size512_1023;
    vtss_chip_counter_t rx_size1024_1518;
    vtss_chip_counter_t rx_size1519_max;
    vtss_chip_counter_t rx_filter_drops;
#if defined(VTSS_FEATURE_QOS)
    vtss_chip_counter_t rx_policer_drops[VTSS_PORT_POLICERS];
#endif /* VTSS_FEATURE_QOS */
    vtss_chip_counter_t rx_class[VTSS_PRIOS];
    vtss_chip_counter_t rx_queue_drops[VTSS_PRIOS];
    vtss_chip_counter_t rx_red_drops[VTSS_PRIOS];
    vtss_chip_counter_t rx_error_drops[VTSS_PRIOS];

    /* Tx counters */
    vtss_chip_counter_t tx_out_bytes;
    vtss_chip_counter_t tx_pause;
    vtss_chip_counter_t tx_ok_bytes;
    vtss_chip_counter_t tx_unicast;
    vtss_chip_counter_t tx_multicast;
    vtss_chip_counter_t tx_broadcast;
    vtss_chip_counter_t tx_size64;
    vtss_chip_counter_t tx_size65_127;
    vtss_chip_counter_t tx_size128_255;
    vtss_chip_counter_t tx_size256_511;
    vtss_chip_counter_t tx_size512_1023;
    vtss_chip_counter_t tx_size1024_1518;
    vtss_chip_counter_t tx_size1519_max;
    vtss_chip_counter_t tx_queue_drops;
    vtss_chip_counter_t tx_red_drops;
    vtss_chip_counter_t tx_error_drops;
    vtss_chip_counter_t tx_queue;
    vtss_chip_counter_t tx_queue_bytes;

    /* These fields are only relevant for D4, for D10 these fields remain 0 */
    vtss_chip_counter_t tx_multi_coll;
    vtss_chip_counter_t tx_late_coll;
    vtss_chip_counter_t tx_xcoll;
    vtss_chip_counter_t tx_defer;
    vtss_chip_counter_t tx_xdefer;
    vtss_chip_counter_t tx_csense;
    vtss_chip_counter_t tx_backoff1;
} vtss_port_b2_counters_t;

typedef struct {
    /* Rx counters */
    vtss_chip_counter_t rx_in_bytes;
    vtss_chip_counter_t rx_symbol_err;
    vtss_chip_counter_t rx_pause;
    vtss_chip_counter_t rx_unsup_opcode;
    vtss_chip_counter_t rx_ok_bytes;
    vtss_chip_counter_t rx_bad_bytes;
    vtss_chip_counter_t rx_unicast;
    vtss_chip_counter_t rx_multicast;
    vtss_chip_counter_t rx_broadcast;
    vtss_chip_counter_t rx_crc_err;
    vtss_chip_counter_t rx_undersize;
    vtss_chip_counter_t rx_fragments;
    vtss_chip_counter_t rx_in_range_len_err;
    vtss_chip_counter_t rx_out_of_range_len_err;
    vtss_chip_counter_t rx_oversize;
    vtss_chip_counter_t rx_jabbers;
    vtss_chip_counter_t rx_size64;
    vtss_chip_counter_t rx_size65_127;
    vtss_chip_counter_t rx_size128_255;
    vtss_chip_counter_t rx_size256_511;
    vtss_chip_counter_t rx_size512_1023;
    vtss_chip_counter_t rx_size1024_1518;
    vtss_chip_counter_t rx_size1519_max;
    vtss_chip_counter_t rx_local_drops;
    vtss_chip_counter_t rx_classified_drops;
    vtss_chip_counter_t rx_red_class[VTSS_PRIOS];
    vtss_chip_counter_t rx_yellow_class[VTSS_PRIOS];
    vtss_chip_counter_t rx_green_class[VTSS_PRIOS];
    vtss_chip_counter_t rx_policer_drops[VTSS_PRIOS];
    vtss_chip_counter_t rx_queue_drops[VTSS_PRIOS];
    vtss_chip_counter_t rx_txqueue_drops[VTSS_PRIOS];

    /* Tx counters */
    vtss_chip_counter_t tx_out_bytes;
    vtss_chip_counter_t tx_pause;
    vtss_chip_counter_t tx_ok_bytes;
    vtss_chip_counter_t tx_unicast;
    vtss_chip_counter_t tx_multicast;
    vtss_chip_counter_t tx_broadcast;
    vtss_chip_counter_t tx_size64;
    vtss_chip_counter_t tx_size65_127;
    vtss_chip_counter_t tx_size128_255;
    vtss_chip_counter_t tx_size256_511;
    vtss_chip_counter_t tx_size512_1023;
    vtss_chip_counter_t tx_size1024_1518;
    vtss_chip_counter_t tx_size1519_max;
    vtss_chip_counter_t tx_yellow_class[VTSS_PRIOS];
    vtss_chip_counter_t tx_green_class[VTSS_PRIOS];
    vtss_chip_counter_t tx_queue_drops[VTSS_PRIOS];
    vtss_chip_counter_t tx_multi_coll;
    vtss_chip_counter_t tx_late_coll;
    vtss_chip_counter_t tx_xcoll;
    vtss_chip_counter_t tx_defer;
    vtss_chip_counter_t tx_xdefer;
    vtss_chip_counter_t tx_csense;
    vtss_chip_counter_t tx_backoff1;
} vtss_port_jr1_counters_t;

#if defined(VTSS_FEATURE_MAC10G)
typedef struct {
    /* Rx counters */
    vtss_chip_counter_t rx_in_bytes;

    vtss_chip_counter_t rx_symbol_err;
    vtss_chip_counter_t rx_pause;
    vtss_chip_counter_t rx_unsup_opcode;
    vtss_chip_counter_t rx_ok_bytes;
    vtss_chip_counter_t rx_bad_bytes;
    vtss_chip_counter_t rx_unicast;
    vtss_chip_counter_t rx_multicast;
    vtss_chip_counter_t rx_broadcast;
    vtss_chip_counter_t rx_crc_err;
    vtss_chip_counter_t rx_undersize;
    vtss_chip_counter_t rx_fragments;
    vtss_chip_counter_t rx_in_range_len_err;
    vtss_chip_counter_t rx_out_of_range_len_err;
    vtss_chip_counter_t rx_oversize;
    vtss_chip_counter_t rx_jabbers;
    vtss_chip_counter_t rx_size64;
    vtss_chip_counter_t rx_size65_127;
    vtss_chip_counter_t rx_size128_255;
    vtss_chip_counter_t rx_size256_511;
    vtss_chip_counter_t rx_size512_1023;
    vtss_chip_counter_t rx_size1024_1518;
    vtss_chip_counter_t rx_size1519_max;

    /* Tx counters */
    vtss_chip_counter_t tx_out_bytes;

    vtss_chip_counter_t tx_pause; /* DBG */
    vtss_chip_counter_t tx_ok_bytes;
    vtss_chip_counter_t tx_unicast;
    vtss_chip_counter_t tx_multicast;
    vtss_chip_counter_t tx_broadcast;
    vtss_chip_counter_t tx_size64;
    vtss_chip_counter_t tx_size65_127;
    vtss_chip_counter_t tx_size128_255;
    vtss_chip_counter_t tx_size256_511;
    vtss_chip_counter_t tx_size512_1023;
    vtss_chip_counter_t tx_size1024_1518;
    vtss_chip_counter_t tx_size1519_max;
} vtss_port_mac10g_counters_t;
#endif /* VTSS_FEATURE_MAC10G */

typedef struct {
    union {
        vtss_port_luton28_counters_t luton28;
        vtss_port_luton26_counters_t luton26;
        vtss_port_b2_counters_t      b2;
        vtss_port_jr1_counters_t     jr1;
#if defined(VTSS_FEATURE_MAC10G)
        vtss_port_mac10g_counters_t  mac10g;
#endif /* VTSS_FEATURE_MAC10G */
    } counter;
} vtss_port_chip_counters_t;

#if defined(VTSS_FEATURE_PCS_10GBASE_R)
/** \brief pcs counters */
typedef struct vtss_pcs_10gbase_r_cnt_s {
	vtss_chip_counter_t ber_count;                 /**< counts each time BER_BAD_SH state is entered */
    vtss_chip_counter_t	rx_errored_block_count;     /**< counts once for each time RX_E state is entered. */
    vtss_chip_counter_t	tx_errored_block_count;     /**< counts once for each time TX_E state is entered. */
    vtss_chip_counter_t	test_pattern_error_count;   /**< When the receiver is in test-pattern mode, the test_pattern_error_count counts
                                                              errors as described in 802.3: 49.2.12. */
} vtss_pcs_10gbase_r_chip_counters_t;
#endif /*VTSS_FEATURE_PCS_10GBASE_R */

typedef struct
{
    BOOL link;        /**< FALSE if link has been down since last status read */
    struct
    {
        BOOL                      complete;               /**< Completion status */
        vtss_port_clause_37_adv_t partner_advertisement;  /**< Clause 37 Advertisement control data */
        vtss_port_sgmii_aneg_t    partner_adv_sgmii;      /**< SGMII Advertisement control data */
    } autoneg;                                            /**< Autoneg status */
} vtss_port_clause_37_status_t;

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
typedef struct
{   /* Rx counters */
    vtss_chip_counter_t rx_yellow[VTSS_PRIOS]; /**< Rx yellow frames */
    vtss_chip_counter_t rx_green[VTSS_PRIOS];  /**< Rx green frames  */
    vtss_chip_counter_t rx_drops[VTSS_PRIOS];  /**< Rx drops         */
    /* Tx counters */
    vtss_chip_counter_t tx_yellow[VTSS_PRIOS]; /**< Tx yellow frames */
    vtss_chip_counter_t tx_green[VTSS_PRIOS];  /**< Tx green frames  */
} vtss_lport_chip_counters_t;
#endif


#if defined(VTSS_ARCH_B2)
/* Internal port numbers */
#define VTSS_INT_PORT_COUNT 96

/* Port forwarding state */
#define VTSS_PORT_RX_FORWARDING(fwd_state) (fwd_state == VTSS_PORT_FORWARD_ENABLED || fwd_state == VTSS_PORT_FORWARD_INGRESS ? 1 : 0)
#define VTSS_PORT_TX_FORWARDING(fwd_state) (fwd_state == VTSS_PORT_FORWARD_ENABLED || fwd_state == VTSS_PORT_FORWARD_EGRESS ? 1 : 0)

#endif /* VTSS_ARCH_B2 */

/** \brief Serdes macro mode */
typedef enum
{
    VTSS_SERDES_MODE_DISABLE,   /**< Disable serdes */
    VTSS_SERDES_MODE_XAUI_12G,  /**< XAUI 12G mode  */
    VTSS_SERDES_MODE_XAUI,      /**< XAUI 10G mode  */
    VTSS_SERDES_MODE_RXAUI,     /**< RXAUI 10G mode */
    VTSS_SERDES_MODE_RXAUI_12G, /**< RXAUI 12G mode */
    VTSS_SERDES_MODE_2G5,       /**< 2.5G mode      */
    VTSS_SERDES_MODE_QSGMII,    /**< QSGMII mode    */
    VTSS_SERDES_MODE_SGMII,     /**< SGMII mode     */
    VTSS_SERDES_MODE_100FX,     /**< 100FX mode     */
    VTSS_SERDES_MODE_1000BaseX  /**< 1000BaseX mode */
} vtss_serdes_mode_t;

#endif /* VTSS_FEATURE_PORT_CONTROL */

/* ================================================================= *
 *  QoS
 * ================================================================= */
#if defined(VTSS_FEATURE_QOS)

#if defined(VTSS_FEATURE_QCL)
#define VTSS_QCL_LIST_SIZE 24

typedef struct vtss_qcl_entry_t {
    struct vtss_qcl_entry_t *next;  /* Next in list */
    vtss_qce_t               qce;   /* This entry */
} vtss_qcl_entry_t;

typedef struct {
    vtss_qcl_entry_t         *qcl_list_used;               /* Free entries for QCL usage */
    vtss_qcl_entry_t         *qcl_list_free;               /* Entries in QCL List */
    vtss_qcl_entry_t         qcl_list[VTSS_QCL_LIST_SIZE]; /* Actual storage for list members */
} vtss_qcl_t;
#endif /* VTSS_FEATURE_QCL */

#if defined(VTSS_ARCH_LUTON26)
#define VTSS_L26_POLICER_CNT 256

/** \brief Shared policer users */
typedef enum
{
    VTSS_POLICER_USER_NONE,    /**< Policer is free */
    VTSS_POLICER_USER_DISCARD, /**< Discard Policer */
    VTSS_POLICER_USER_PORT,    /**< Port Policer */
    VTSS_POLICER_USER_QUEUE,   /**< Queue Policer */
    VTSS_POLICER_USER_ACL,     /**< ACL Policer */
    VTSS_POLICER_USER_EVC,     /**< EVC Policer */
    VTSS_POLICER_USER_MEP,     /**< Up-MEP Policer */

    /* Number of users, must be last field */
    VTSS_POLICER_USER_CNT
} vtss_policer_user_t;

/* Policer allocation */
typedef struct {
    u16 count;   /* Reference count */
    u16 policer; /* Policer index */
} vtss_policer_alloc_t;

/* Policer types used for IS2 entries */
#define VTSS_L26_POLICER_NONE 0
#define VTSS_L26_POLICER_ACL  1
#define VTSS_L26_POLICER_EVC  2

#endif /* VTSS_ARCH_LUTON26 */

#endif /* VTSS_FEATURE_QOS */

/* ================================================================= *
 *  Packet control
 * ================================================================= */

#if defined(VTSS_FEATURE_PACKET)

typedef struct {
    BOOL                    used;        /* Packet used flag */
    u32                     words_read;  /* Number of words read */
    vtss_packet_rx_header_t header;      /* Rx frame header */
} vtss_packet_rx_t;

#endif /* VTSS_FEATURE_PACKET */

/* ================================================================= *
 *  OAM
 * ================================================================= */

#if defined(VTSS_FEATURE_OAM)

typedef struct {
    vtss_chip_counter_t rx;
    vtss_chip_counter_t tx;
} vtss_oam_voe_internal_rx_tx_counter_t;

typedef struct {
    struct {
        vtss_chip_counter_t                   rx_valid_count;
        vtss_chip_counter_t                   rx_invalid_count;
        vtss_chip_counter_t                   rx_invalid_seq_no;
    } ccm;
    struct {
        vtss_oam_voe_internal_rx_tx_counter_t lm_count[VTSS_PRIO_ARRAY_SIZE];
    } lm;
    struct {
        vtss_chip_counter_t                   rx_lbr;
        vtss_chip_counter_t                   tx_lbm;
        vtss_chip_counter_t                   rx_lbr_trans_id_err;
    } lb;
    struct {
        vtss_chip_counter_t                   rx_tst;
        vtss_chip_counter_t                   tx_tst;
        vtss_chip_counter_t                   rx_tst_trans_id_err;
    } tst;
    struct {
        vtss_oam_voe_internal_rx_tx_counter_t selected_frames;
        vtss_oam_voe_internal_rx_tx_counter_t nonselected_frames;
    } sel;
} vtss_oam_voe_internal_counters_t;

typedef struct {
    BOOL                             allocated;      /* TRUE => VOE is allocated by API (vtss_oam_voe_alloc()); FALSE => free */
    vtss_oam_voe_internal_counters_t counters;       /* Chip counters for a VOE */
} vtss_oam_voe_internal_t;

#endif /* VTSS_FEATURE_OAM */

/* ================================================================= *
 *  Layer 2
 * ================================================================= */

#if defined(VTSS_FEATURE_LAYER2)

/* Port forwarding state */
#define VTSS_PORT_RX_FORWARDING(fwd_state) (fwd_state == VTSS_PORT_FORWARD_ENABLED || fwd_state == VTSS_PORT_FORWARD_INGRESS ? 1 : 0)
#define VTSS_PORT_TX_FORWARDING(fwd_state) (fwd_state == VTSS_PORT_FORWARD_ENABLED || fwd_state == VTSS_PORT_FORWARD_EGRESS ? 1 : 0)

#if defined(VTSS_FEATURE_PVLAN)
/* PVLAN entry */
typedef struct {
    BOOL member[VTSS_PORT_ARRAY_SIZE]; /* Member ports */
} vtss_pvlan_entry_t;
#endif /* VTSS_FEATURE_PVLAN */

/* PGID entry */
typedef struct {
    BOOL member[VTSS_PORT_ARRAY_SIZE]; /* Egress ports */
    BOOL resv;                         /* Fixed reservation */
    u32  references;                   /* Number references to entry */
    BOOL cpu_copy;                     /* CPU copy */
    vtss_packet_rx_queue_t cpu_queue;  /* CPU queue */
} vtss_pgid_entry_t;

/* Number of destination masks */

#if defined(VTSS_ARCH_LUTON28)
#define VTSS_PGID_LUTON28 64
#define VTSS_PGIDS        VTSS_PGID_LUTON28
#endif /* VTSS_ARCH_LUTON28 */

#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
#define VTSS_PGID_LUTON26 64
#undef VTSS_PGIDS
#define VTSS_PGIDS        VTSS_PGID_LUTON26
#endif /* VTSS_ARCH_LUTON26/SERVAL */

#if defined(VTSS_ARCH_JAGUAR_1)
#define VTSS_PGID_JAGUAR_1 1312
#undef VTSS_PGIDS
#define VTSS_PGIDS VTSS_PGID_JAGUAR_1
#endif /* VTSS_ARCH_JAGUAR_1 */

/* Pseudo PGID for IPv4/IPv6 MC */
#define VTSS_PGID_NONE VTSS_PGIDS

#define VTSS_GLAG_NO_NONE 0xffffffff

/* Size of lookup page and pointer array */
#define VTSS_MAC_PAGE_SIZE 128
#define VTSS_MAC_PTR_SIZE  (VTSS_MAC_ADDRS/VTSS_MAC_PAGE_SIZE)

/* MAC address table users */
#define VTSS_MAC_USER_NONE 0 /* Normal entries added by the application */
#define VTSS_MAC_USER_SSM  1 /* Internal entries added for SSM purposes */
#define VTSS_MAC_USER_ASM  2 /* Internal entries added for ASM purposes */
typedef u8 vtss_mac_user_t;

/* MAC address table for get next operations */
typedef struct vtss_mac_entry_t {
    struct vtss_mac_entry_t *next;  /* Next in list */
    u32                     mach;  /* VID and 16 MSB of MAC */
    u32                     macl;  /* 32 LSB of MAC */
    u8                      member[VTSS_PORT_BF_SIZE];
    BOOL                    cpu_copy;
    vtss_mac_user_t         user;
} vtss_mac_entry_t;

/* IPv4 and IPv6 multicast address */
#define VTSS_MAC_IPV4_MC(mac) (mac[0] == 0x01 && mac[1] == 0x00 && mac[2] == 0x5e && (mac[3] & 0x80) == 0x00)
#define VTSS_MAC_IPV6_MC(mac) (mac[0] == 0x33 && mac[1] == 0x33)
#define VTSS_MAC_BC(mac)      (mac[0] == 0xff && mac[1] == 0xff && mac[2] == 0xff && mac[3] == 0xff && mac[4] == 0xff && mac[5] == 0xff)

void vtss_mach_macl_get(const vtss_vid_mac_t *vid_mac, u32 *mach, u32 *macl);
void vtss_mach_macl_set(vtss_vid_mac_t *vid_mac, u32 mach, u32 macl);

/* Counter for number of enabled rings with port in discarding state */
#if (VTSS_ERPIS > 255)
typedef u16 vtss_erps_counter_t;
#else
typedef u8 vtss_erps_counter_t;
#endif

/* For IPMC SSM, VLAN ID may be allocated for IPv4/IPv6 FID */
#define IPMC_USED_NONE 0x00
#define IPMC_USED_IPV4 0x01
#define IPMC_USED_IPV6 0x02

/* VLAN entry */
typedef struct {
    BOOL        enabled;                   /* At least one port enabled */
    BOOL        update;                    /* Update flag */
    u8          member[VTSS_PORT_BF_SIZE]; /* Port members */
#if defined(VTSS_FEATURE_VLAN_TX_TAG)
    u8          tx_tag[VTSS_PORT_ARRAY_SIZE]; /* Tx tagging */
#endif /* VTSS_FEATURE_VLAN_TX_TAG */
    vtss_erps_counter_t erps_discard_cnt[VTSS_PORT_ARRAY_SIZE]; /* ERPS discard counter */
    vtss_msti_t msti;                      /* MSTP instance */
    BOOL        isolated;                  /* Port isolation */
    BOOL        learn_disable;             /* Learning disabled */
    BOOL        ipmc_used;                 /* In use for SSM */
    u32         mask;                      /* Previous mask written */
} vtss_vlan_entry_t;

/* MSTP entry */
typedef struct {
    vtss_stp_state_t state[VTSS_PORT_ARRAY_SIZE]; /* MSTP state */
} vtss_mstp_entry_t;

/* ERPS entry */
typedef struct {
    u8   vlan_member[VTSS_BF_SIZE(VTSS_VIDS)]; /* VLAN members */
    u8   port_member[VTSS_PORT_BF_SIZE];       /* Forwarding port members */
} vtss_erps_entry_t;

/* Port protection */
typedef struct {
    vtss_eps_port_conf_t conf;     /* Configuration */
    vtss_eps_selector_t  selector; /* Selector */
} vtss_port_eps_t;

#endif /* VTSS_FEATURE_LAYER2 */
#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
/* VLAN Translation Group entry (Group to VLAN Translation mappings) */
typedef struct vtss_vlan_trans_grp2vlan_entry_t {
    struct vtss_vlan_trans_grp2vlan_entry_t     *next;                             /* Next in list                      */
    vtss_vlan_trans_grp2vlan_conf_t             conf;                              /* Group to VLAN configuration       */
} vtss_vlan_trans_grp2vlan_entry_t;

/* VLAN Translation Group lists - used and free */
typedef struct {
    vtss_vlan_trans_grp2vlan_entry_t  *used;                                       /* used list                         */
    vtss_vlan_trans_grp2vlan_entry_t  *free;                                       /* free list                         */
    vtss_vlan_trans_grp2vlan_entry_t  trans_list[VTSS_VLAN_TRANS_MAX_CNT];         /* Actual storage for list members   */
} vtss_vlan_trans_grp2vlan_t;

/* VLAN Translation Port entry (Ports to Group mappings) */
typedef struct vtss_vlan_trans_port2grp_entry_t {
    struct vtss_vlan_trans_port2grp_entry_t     *next;                             /* Next in list                      */
    vtss_vlan_trans_port2grp_conf_t             conf;                              /* Port to Group configuration       */
} vtss_vlan_trans_port2grp_entry_t;

/* VLAN Translation Port lists - used and free */
typedef struct {
    vtss_vlan_trans_port2grp_entry_t   *used;                                      /* used list                         */
    vtss_vlan_trans_port2grp_entry_t   *free;                                      /* free list                         */
    vtss_vlan_trans_port2grp_entry_t   port_list[VTSS_VLAN_TRANS_GROUP_MAX_CNT];   /* Actual storage for list members   */
} vtss_vlan_trans_port2grp_t;
#endif /* VTSS_FEATURE_VLAN_TRANSLATION */

#if defined(VTSS_FEATURE_VLAN_COUNTERS)
typedef struct {
    vtss_chip_counter_t frames;                         /**< Frame counters */
    vtss_chip_counter_t bytes;                          /**< Byte counters */
} vtss_chip_counter_pair_t;

typedef struct {
    vtss_chip_counter_pair_t rx_vlan_unicast;           /**< The total number of unicast packets and bytes counted on this VLAN */
    vtss_chip_counter_pair_t rx_vlan_multicast;         /**< The total number of multicast packets and bytes counted on this VLAN */
    vtss_chip_counter_pair_t rx_vlan_flood;             /**< The total number of flood packets and bytes counted on this VLAN */
} vtss_vlan_chip_counters_t;

/* VLAN Counter information */
typedef struct {
    u32                         poll_idx;               /* Counter polling index */
    vtss_vlan_chip_counters_t   counters[VTSS_VIDS];    /* Counters for all the VLANs */
} vtss_vlan_counter_info_t;
#endif /* VTSS_FEATURE_VLAN_COUNTERS */

#if defined(VTSS_FEATURE_VSTAX)
/* VStaX information per chip */
typedef struct {
    u32                      mask_a;       /* Stack port A mask */
    u32                      mask_b;       /* Stack port B mask */
    vtss_vstax_port_conf_t   port_conf[2]; /* Stack A/B information */
#if defined(VTSS_FEATURE_VSTAX_V2)
    vtss_vstax_route_table_t rt_table;     /* Route table */
    u32                      upsid_active; /* Active UPSID mask */
#endif /* VTSS_FEATURE_VSTAX_V2 */
} vtss_vstax_chip_info_t;

/* VStaX information */
typedef struct {
    vtss_vstax_upsid_t     upsid[2];                  /* Info per UPS */
    vtss_vstax_chip_info_t chip_info[VTSS_CHIP_CNT];  /* Info per chip */
    vtss_vstax_upsid_t     master_upsid;              /* UPSID of current master */
} vtss_vstax_info_t;
#endif /* VTSS_FEATURE_VSTAX */

#if defined(VTSS_FEATURE_VSTAX_V2)
/* GLAG configuration */
typedef struct {
    vtss_vstax_glag_entry_t entry;   /* UPSID/UPSPN entry */
    vtss_port_no_t          port_no; /* Local port or VTSS_PORT_NO_NONE */
} vtss_glag_conf_t;
#endif /* VTSS_FEATURE_VSTAX_V2 */

#if defined(VTSS_FEATURE_EVC)
/* EVC port information entry */
typedef struct {
    u16 uni_count;
    u16 nni_count;
} vtss_evc_port_info_t;

#define VTSS_EVC_VOE_IDX_NONE 0xff

/* EVC entry */
typedef struct {
    BOOL                  enable;    /* EVC valid indication */
    BOOL                  learning;  /* Learning enabled */
    vtss_vid_t            vid;       /* PB VID */
    vtss_vid_t            ivid;      /* Classified VID */
#if defined(VTSS_ARCH_CARACAL)
    vtss_vid_t            uvid;      /* Classified VID */
    vtss_evc_inner_tag_t  inner_tag; /* Inner tag */
#endif /* VTSS_ARCH_CARACAL */
    u8                    ports[VTSS_PORT_BF_SIZE];
    vtss_evc_policer_id_t policer_id;
#if defined(VTSS_ARCH_SERVAL)
    u8                    voe_idx[VTSS_PORT_ARRAY_SIZE]; /* VOE index */
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_FEATURE_MPLS)
    vtss_mpls_xc_idx_t    pw_ingress_xc; /* XC for ingress unidirectional pseudo-wire */
    vtss_mpls_xc_idx_t    pw_egress_xc;  /* XC for egress unidirectional pseudo-wire */
#endif /* VTSS_FEATURE_MPLS */
} vtss_evc_entry_t;

/* EVC information */
typedef struct {
    u32              max_count;        /* Maximum number of rules */
    u32              count;            /* Actual number of rules */
    vtss_evc_entry_t table[VTSS_EVCS]; /* Entries */
} vtss_evc_info_t;

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
typedef struct {
    vtss_chip_counter_t frames;
    vtss_chip_counter_t bytes;
} vtss_chip_counter_pair_t;

typedef struct {
    vtss_chip_counter_pair_t rx_green;   /**< Rx green frames/bytes */
    vtss_chip_counter_pair_t rx_yellow;  /**< Rx yellow frames/bytes */
    vtss_chip_counter_pair_t rx_red;     /**< Rx red frames/bytes */
    vtss_chip_counter_pair_t rx_discard; /**< Rx discarded frames/bytes */
    vtss_chip_counter_pair_t tx_discard; /**< Tx discarded frames/bytes */
    vtss_chip_counter_pair_t tx_green;   /**< Tx green frames/bytes */
    vtss_chip_counter_pair_t tx_yellow;  /**< Tx yellow frames/bytes */
} vtss_sdx_counters_t;

/* SDX entry */
typedef struct vtss_sdx_entry_t {
    struct vtss_sdx_entry_t *next;   /* next in list */
    u16                     sdx;     /* SDX value */
    vtss_port_no_t          port_no; /* UNI/NNI port number */
} vtss_sdx_entry_t;

/* SDX zero is reserved */
#define VTSS_JR1_SDX_CNT  4095
#define VTSS_SRVL_SDX_CNT 1023

#if defined(VTSS_ARCH_JAGUAR_1)
#define VTSS_SDX_CNT VTSS_JR1_SDX_CNT
#else
#define VTSS_SDX_CNT VTSS_SRVL_SDX_CNT
#endif

/* Table and free list for ISDX/ESDX */
typedef struct {
    u32              count;               /* Actual number of rules */
    vtss_sdx_entry_t *free;               /* List of free entries */
    vtss_sdx_entry_t table[VTSS_SDX_CNT]; /* Table of entries */
} vtss_sdx_list_t;

/* SDX information */
typedef struct {
    u32                 max_count; /* Maximum number of rules */
    u32                 poll_idx;  /* Counter polling index */
    vtss_sdx_counters_t sdx_table[VTSS_SDX_CNT];
    vtss_sdx_list_t     isdx;      /* ISDX list */
    vtss_sdx_list_t     esdx;      /* ESDX list */
} vtss_sdx_info_t;
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */

/* ECE key flags */
#define VTSS_ECE_KEY_DMAC_MC_VLD         0x00000001
#define VTSS_ECE_KEY_DMAC_MC_1           0x00000002
#define VTSS_ECE_KEY_DMAC_BC_VLD         0x00000004
#define VTSS_ECE_KEY_DMAC_BC_1           0x00000008
#define VTSS_ECE_KEY_TAG_DEI_VLD         0x00000010
#define VTSS_ECE_KEY_TAG_DEI_1           0x00000020
#define VTSS_ECE_KEY_TAG_TAGGED_VLD      0x00000040
#define VTSS_ECE_KEY_TAG_TAGGED_1        0x00000080
#define VTSS_ECE_KEY_TAG_S_TAGGED_VLD    0x00000100
#define VTSS_ECE_KEY_TAG_S_TAGGED_1      0x00000200
#define VTSS_ECE_KEY_IN_TAG_DEI_VLD      0x00000400
#define VTSS_ECE_KEY_IN_TAG_DEI_1        0x00000800
#define VTSS_ECE_KEY_IN_TAG_TAGGED_VLD   0x00001000
#define VTSS_ECE_KEY_IN_TAG_TAGGED_1     0x00002000
#define VTSS_ECE_KEY_IN_TAG_S_TAGGED_VLD 0x00004000
#define VTSS_ECE_KEY_IN_TAG_S_TAGGED_1   0x00008000
#define VTSS_ECE_KEY_PROT_IPV4           0x00010000
#define VTSS_ECE_KEY_PROT_IPV6           0x00020000

/* ECE action flags */
#define VTSS_ECE_ACT_DIR_UNI_TO_NNI     0x00000001 /* Flow direction, default BOTH */
#define VTSS_ECE_ACT_DIR_NNI_TO_UNI     0x00000002
#define VTSS_ECE_ACT_POP_1              0x00000004
#define VTSS_ECE_ACT_POP_2              0x00000008
#define VTSS_ECE_ACT_IT_TYPE_C          0x00000010 /* Inner tag, default NONE */
#define VTSS_ECE_ACT_IT_TYPE_S          0x00000020
#define VTSS_ECE_ACT_IT_TYPE_S_CUSTOM   0x00000040
#define VTSS_ECE_ACT_IT_PCP_MODE_FIXED  0x00000080 /* Inner PCP mode, default CLASSIFIED */
#define VTSS_ECE_ACT_IT_PCP_MODE_MAPPED 0x00000100
#define VTSS_ECE_ACT_IT_DEI             0x00000200
#define VTSS_ECE_ACT_IT_DEI_MODE_FIXED  0x00000400 /* Inner DEI mode, default CLASSIFIED */
#define VTSS_ECE_ACT_IT_DEI_MODE_DP     0x00000800
#define VTSS_ECE_ACT_OT_ENA             0x00001000 
#define VTSS_ECE_ACT_OT_PCP_MODE_FIXED  0x00002000 /* Outer PCP mode, default CLASSIFIED */
#define VTSS_ECE_ACT_OT_PCP_MODE_MAPPED 0x00004000
#define VTSS_ECE_ACT_OT_DEI             0x00008000
#define VTSS_ECE_ACT_OT_DEI_MODE_FIXED  0x00010000 /* Outer DEI mode, default CLASSIFIED */
#define VTSS_ECE_ACT_OT_DEI_MODE_DP     0x00020000
#define VTSS_ECE_ACT_PRIO_ENA           0x00040000
#define VTSS_ECE_ACT_DP_ENA             0x00080000
#define VTSS_ECE_ACT_RULE_RX            0x00100000 /* Rule direction, default BOTH */
#define VTSS_ECE_ACT_RULE_TX            0x00200000
#define VTSS_ECE_ACT_TX_LOOKUP_VID_PCP  0x00400000 /* Tx lookup, default VID */
#define VTSS_ECE_ACT_TX_LOOKUP_ISDX     0x00800000
#define VTSS_ECE_ACT_POLICY_NONE        0x01000000

#define VTSS_ECE_ACT_DIR_ONE        (VTSS_ECE_ACT_DIR_UNI_TO_NNI | VTSS_ECE_ACT_DIR_NNI_TO_UNI)
#define VTSS_ECE_ACT_IT_TYPE_USED   (VTSS_ECE_ACT_IT_TYPE_C | VTSS_ECE_ACT_IT_TYPE_S | VTSS_ECE_ACT_IT_TYPE_S_CUSTOM)

/* ECE entry */
typedef struct vtss_ece_entry_t {
    struct vtss_ece_entry_t *next;     /* Next in list */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    vtss_sdx_entry_t        *isdx_list; /* ISDX list */
    vtss_sdx_entry_t        *esdx_list; /* ESDX list */
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
    vtss_ece_id_t           ece_id;    /* ECE ID */

    /* Key/action flags */
    u32                     key_flags;
    u32                     act_flags;

    /* Action fields */
    vtss_evc_id_t           evc_id;
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    vtss_evc_policer_id_t   policer_id;
    vtss_vid_t              it_vid;
    vtss_vid_t              ot_vid;
    u8                      it_pcp;
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
#if defined(VTSS_ARCH_CARACAL) | defined(VTSS_ARCH_SERVAL)
    u8                      prio;
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
    u8                      dp;
#endif /* VTSS_ARCH_SERVAL */
    u8                      ot_pcp;
    u8                      policy_no;

    /* Key fields */
    vtss_vcap_u8_t          pcp;
    vtss_vcap_vr_t          vid;
    u8                      ports[VTSS_PORT_BF_SIZE];

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    vtss_vcap_vr_t          in_vid;
    vtss_vcap_u8_t          in_pcp;
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
    vtss_vcap_u48_t         smac;
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
    vtss_vcap_u48_t         dmac;
#endif /* VTSS_ARCH_CARACAL */
    union
    {
        vtss_ece_frame_ipv4_t ipv4;
        vtss_ece_frame_ipv6_t ipv6;
    } frame;  /**< Frame type specific data */
} vtss_ece_entry_t;

/* Number of ECEs */
#define VTSS_ECES VTSS_EVCS

/* ECE info */
typedef struct {
    u32              max_count;        /* Maximum number of rules */
    u32              count;            /* Actual number of rules */
    vtss_ece_entry_t *used;            /* Used list */
    vtss_ece_entry_t *free;            /* Free list */
    vtss_ece_entry_t table[VTSS_ECES]; /* Entries */
} vtss_ece_info_t;

/* Resource update command */
typedef enum {
    VTSS_RES_CMD_CALC, /* Calculate resources */
    VTSS_RES_CMD_DEL,  /* Delete resources */
    VTSS_RES_CMD_ADD   /* Add resources */
} vtss_res_cmd_t;

#if defined(VTSS_ARCH_SERVAL)
/* Two MCEs pr COS-ID can map to the same VOE */
#define VTSS_MCES (2 * 8 * VTSS_OAM_PATH_SERVICE_VOE_CNT)

/* MCE entry */
typedef struct vtss_mce_entry_t {
    struct vtss_mce_entry_t *next;      /* Next in list */
    vtss_mce_t              conf;       /* Configuration */
    vtss_sdx_entry_t        *isdx_list; /* ISDX list */
    vtss_sdx_entry_t        *esdx_list; /* ESDX list */
} vtss_mce_entry_t;

/* MCE info */
typedef struct {
    u32              max_count;        /* Maximum number of rules */
    u32              count;            /* Actual number of rules */
    vtss_mce_entry_t *used;            /* Used list */
    vtss_mce_entry_t *free;            /* Free list */
    vtss_mce_entry_t table[VTSS_MCES]; /* Entries */
} vtss_mce_info_t;
#endif /* VTSS_ARCH_SERVAL */

#endif /* VTSS_FEATURE_EVC */

#if defined(VTSS_FEATURE_VCAP)

/** \brief VCAP key size */
typedef enum
{
    VTSS_VCAP_KEY_SIZE_FULL,   /**< Full key */
    VTSS_VCAP_KEY_SIZE_HALF,   /**< Half key */
    VTSS_VCAP_KEY_SIZE_QUARTER /**< Quarter key */
} vtss_vcap_key_size_t;

#define VTSS_VCAP_KEY_SIZE_LAST VTSS_VCAP_KEY_SIZE_QUARTER
#define VTSS_VCAP_KEY_SIZE_MAX  (VTSS_VCAP_KEY_SIZE_LAST + 1)

/* Resource change data */
typedef struct {
    u32 add;                             /* Number of added items */
    u32 del;                             /* Number of deleted items */
    u32 add_key[VTSS_VCAP_KEY_SIZE_MAX]; /* Added rules for each key size */
    u32 del_key[VTSS_VCAP_KEY_SIZE_MAX]; /* Deleted rules for each key size */
} vtss_res_chg_t;

/* Resource change information */
typedef struct {
#if defined(VTSS_FEATURE_EVC)
    BOOL            port_add[VTSS_PORT_ARRAY_SIZE];
    BOOL            port_del[VTSS_PORT_ARRAY_SIZE];
    BOOL            port_chg[VTSS_PORT_ARRAY_SIZE];
    BOOL            port_nni[VTSS_PORT_ARRAY_SIZE];
    BOOL            ece_add;
    BOOL            ece_del;
    BOOL            es0_add[VTSS_PORT_ARRAY_SIZE];
    BOOL            es0_del[VTSS_PORT_ARRAY_SIZE];
    vtss_ece_dir_t  dir_old;
    vtss_ece_dir_t  dir_new;
#if defined(VTSS_ARCH_SERVAL)
    vtss_ece_rule_t rule_old;
    vtss_ece_rule_t rule_new;
#endif /* VTSS_ARCH_SERVAL */
#endif /* VTSS_FEATURE_EVC */
    vtss_res_chg_t  is0;
    vtss_res_chg_t  is1;
    vtss_res_chg_t  is2;
    vtss_res_chg_t  es0;
    vtss_res_chg_t  isdx;
    vtss_res_chg_t  esdx;
} vtss_res_t;

typedef enum {
    VTSS_VCAP_TYPE_IS0,
    VTSS_VCAP_TYPE_IS1,
    VTSS_VCAP_TYPE_IS2,
    VTSS_VCAP_TYPE_ES0
} vtss_vcap_type_t;

/* VCAP ID */
typedef u64 vtss_vcap_id_t;

/* VCAP users in prioritized order */
typedef enum {
    /* IS0 users */
    VTSS_IS0_USER_EVC,         /* EVC (JR1) */
    VTSS_IS0_USER_MPLS_LL,     /* MPLS link layer (Serval) */
    VTSS_IS0_USER_MPLS_MLBS_3, /* MPLS label stack depth 3 (Serval) */
    VTSS_IS0_USER_MPLS_MLBS_2, /* MPLS label stack depth 2 (Serval) */
    VTSS_IS0_USER_MPLS_MLBS_1, /* MPLS label stack depth 1 (Serval) */

    /* IS1 users */
    VTSS_IS1_USER_VCL,      /* VCL (first lookup) */
    VTSS_IS1_USER_VLAN,     /* VLAN translation (first lookup) */
    VTSS_IS1_USER_MEP,      /* MEP (first lookup, L26/SRVL) */
    VTSS_IS1_USER_EVC,      /* EVC (first lookup, L26/SRVL) */
    VTSS_IS1_USER_QOS,      /* QoS QCL (second lookup for L26/JR1, third lookup for SRVL) */
    VTSS_IS1_USER_ACL,      /* ACL SIP/SMAC (third lookup, L26) */
    VTSS_IS1_USER_SSM,      /* SSM (first lookup, L26) */

    /* IS2 users */
    VTSS_IS2_USER_IGMP,     /* IGMP rules (first lookup, JR1) */
    VTSS_IS2_USER_SSM,      /* SSM rules (first lookup, JR1) */
    VTSS_IS2_USER_IGMP_ANY, /* IGMP any rules (first lookup, JR1) */
    VTSS_IS2_USER_EEE,      /* EEE loopback port rules (second lookup, JR1) */
    VTSS_IS2_USER_ACL_PTP,  /* ACL PTP rules (second lookup, L26) */
    VTSS_IS2_USER_ACL,      /* ACL rules (first lookup for L26, second lookup for JR1) */
    VTSS_IS2_USER_ACL_SIP,  /* ACL SIP/SMAC rules (Serval) */

    /* ES0 users */
    VTSS_ES0_USER_VLAN,     /* VLAN translation */
    VTSS_ES0_USER_MEP,      /* MEP rules */
    VTSS_ES0_USER_EVC,      /* EVC rules */
    VTSS_ES0_USER_TX_TAG,   /* VLAN Tx tagging */
    VTSS_ES0_USER_MPLS,     /* MPLS (Serval) */
} vtss_vcap_user_t;

#if defined(VTSS_FEATURE_IS0)

#if defined(VTSS_ARCH_JAGUAR_1)

/* IS0 action */
typedef struct {
    BOOL       s1_dmac_ena;
    u8         vlan_pop_cnt;
    BOOL       vid_ena;
    BOOL       pcp_dei_ena;
    u8         pcp;
    BOOL       dei;
    u8         pag;
    u16        isdx;
    vtss_vid_t vid;
} vtss_is0_action_t;

typedef enum
{
    VTSS_IS0_TYPE_ISID,
    VTSS_IS0_TYPE_DBL_VID,
    VTSS_IS0_TYPE_MPLS,
    VTSS_IS0_TYPE_MAC_ADDR
} vtss_is0_type_t;

typedef struct {
    vtss_vcap_bit_t tagged;
    vtss_vcap_vid_t vid;
    vtss_vcap_u8_t  pcp;
    vtss_vcap_bit_t dei;
    vtss_vcap_bit_t s_tag;
} vtss_vcap_tag_t;

typedef enum {
    VTSS_IS0_PROTO_ANY,
    VTSS_IS0_PROTO_NON_IP,
    VTSS_IS0_PROTO_IPV4,
    VTSS_IS0_PROTO_IPV6
} vtss_is0_proto_t;

/* IS0 key */
typedef struct {
    vtss_port_no_t  port_no;
    vtss_is0_type_t type;

    union {
        struct {
            vtss_vcap_tag_t  outer_tag;
            vtss_vcap_tag_t  inner_tag;
            vtss_vcap_u8_t   dscp;
            vtss_is0_proto_t proto;
        } dbl_vid;
    } data;
} vtss_is0_key_t;

/* IS0 entry */
typedef struct {
    vtss_is0_action_t action;
    vtss_is0_key_t    key;
} vtss_is0_entry_t;

/* IS0 data */
typedef struct {
    vtss_is0_entry_t *entry;
} vtss_is0_data_t;

#elif defined(VTSS_ARCH_SERVAL_CE)

typedef enum {
    VTSS_MLL_ETHERTYPE_DOWNSTREAM_ASSIGNED = 1,
    VTSS_MLL_ETHERTYPE_UPSTREAM_ASSIGNED   = 2
} vtss_mll_ethertype_t;

typedef enum {
    VTSS_IS0_MLBS_OAM_NONE    = 0,
    VTSS_IS0_MLBS_OAM_VCCV1   = 1,
    VTSS_IS0_MLBS_OAM_VCCV2   = 2,
    VTSS_IS0_MLBS_OAM_VCCV3   = 3,
    VTSS_IS0_MLBS_OAM_GAL_MEP = 4,
    VTSS_IS0_MLBS_OAM_GAL_MIP = 5
} vtss_is0_mlbs_oam_t;

typedef enum {
    VTSS_IS0_MLBS_POPCOUNT_0  = 1,
    VTSS_IS0_MLBS_POPCOUNT_14 = 2,
    VTSS_IS0_MLBS_POPCOUNT_18 = 3,
    VTSS_IS0_MLBS_POPCOUNT_22 = 4,
    VTSS_IS0_MLBS_POPCOUNT_26 = 5,
    VTSS_IS0_MLBS_POPCOUNT_30 = 6,
    VTSS_IS0_MLBS_POPCOUNT_34 = 7
} vtss_is0_mlbs_popcount_t;

typedef enum {
    VTSS_IS0_TYPE_MLL,
    VTSS_IS0_TYPE_MLBS
} vtss_is0_type_t;

typedef struct {
    BOOL                     physical[VTSS_PORT_ARRAY_SIZE]; /**< Physical port list */
    BOOL                     cpu;
} vtss_is0_b_portlist_t;

typedef struct {
    vtss_port_no_t           ingress_port;
    vtss_mll_tagtype_t       tag_type;
    vtss_vid_t               b_vid;
    vtss_mac_t               dmac;
    vtss_mac_t               smac;
    vtss_mll_ethertype_t     ether_type;
} vtss_is0_mll_key_t;

typedef struct {
    u8                       linklayer_index;
    BOOL                     mpls_forwarding;
    vtss_is0_b_portlist_t    b_portlist;
    u8                       cpu_queue;

    u16                      oam_isdx;
    BOOL                     oam_isdx_add_replace;

    // Below: Only used when mpls_forwarding == FALSE
    u16                      isdx;
    u8                       vprofile_index;
    vtss_vid_t               classified_vid;
    BOOL                     use_service_config;

    // Below: Only used when mpls_forwarding == FALSE && use_service_config == TRUE
    vtss_prio_t              qos;
    vtss_dp_level_t          dp;
} vtss_is0_mll_action_t;

typedef struct {
    u32                         linklayer_index;
    struct {
        vtss_mpls_label_value_t value;                          /**< Label value or VTSS_MPLS_LABEL_VALUE_DONTCARE */
        vtss_mpls_tc_t          tc;                             /**< TC value (0-7) or VTSS_MPLS_TC_VALUE_DONTCARE */
    } label_stack[3];   // 0 is top-of-stack
} vtss_is0_mlbs_key_t;

typedef struct {
    u16                      isdx;
    u8                       cpu_queue;
    vtss_is0_b_portlist_t    b_portlist;
    vtss_is0_mlbs_oam_t      oam;
    BOOL                     oam_buried_mip;
    u8                       oam_reserved_label_value;
    BOOL                     oam_reserved_label_bottom_of_stack;
    u16                      oam_isdx;
    BOOL                     oam_isdx_add_replace;   /**< TRUE = replace; FALSE = add */

    BOOL                     cw_enable;
    BOOL                     terminate_pw;

    u8                       tc_label_index;
    u8                       ttl_label_index;
    u8                       swap_label_index;
    vtss_is0_mlbs_popcount_t pop_count; /**< 0, 14, 18, 22, 26, 30, 34 */

    BOOL                     e_lsp;
    u8                       tc_maptable_index;
    u8                       l_lsp_qos_class;
    BOOL                     add_tc_to_isdx;

    BOOL                     swap_is_bottom_of_stack;

    u8                       vprofile_index;
    vtss_vid_t               classified_vid;
    BOOL                     use_service_config;

    // Below: Only used when use_service_config == TRUE
    BOOL                     s_tag;          /**< FALSE = C-tag */
    vtss_tagprio_t           pcp;            /**< PCP value */
    vtss_dei_t               dei;            /**< DEI value */
} vtss_is0_mlbs_action_t;

typedef union {
    vtss_is0_mll_key_t       mll;
    vtss_is0_mlbs_key_t      mlbs;
} vtss_is0_key_t;

typedef union {
    vtss_is0_mll_action_t    mll;
    vtss_is0_mlbs_action_t   mlbs;
} vtss_is0_action_t;

/* IS0 entry */
typedef struct {
    vtss_is0_type_t          type;
    vtss_is0_key_t           key;
    vtss_is0_action_t        action;
} vtss_is0_entry_t;

/* IS0 data */
typedef struct {
    vtss_is0_entry_t *entry;
} vtss_is0_data_t;

#endif /* VTSS_ARCH_SERVAL_CE */

#endif /* VTSS_FEATURE_IS0 */

typedef enum {
    VTSS_FID_SEL_DEFAULT = 0,   /* Disabled: FID = classified VID. */
    VTSS_FID_SEL_SMAC,          /* Use FID_VAL for SMAC lookup in MAC table. */
    VTSS_FID_SEL_DMAC,          /* Use FID_VAL for DMAC lookup in MAC table. */
    VTSS_FID_SEL_BOTHMAC        /* Use FID_VAL for DMAC and SMAC lookup in MAC */
} vtss_fid_sel_t;

#if defined(VTSS_FEATURE_IS1)
typedef struct {
    BOOL            dscp_enable;    /**< Enable DSCP classification */
    vtss_dscp_t     dscp;           /**< DSCP value */
    BOOL            dp_enable;      /**< Enable DP classification */
    vtss_dp_level_t dp;             /**< DP value */
    BOOL            prio_enable;    /**< Enable priority classification */
    vtss_prio_t     prio;           /**< Priority value */
    BOOL            vid_enable;     /**< VLAN ID enable */
    vtss_vid_t      vid;            /**< VLAN ID or VTSS_VID_NULL */
    vtss_fid_sel_t  fid_sel;        /**< FID Select */
    vtss_vid_t      fid_val;        /**< FID value */
    BOOL            pcp_dei_enable; /**< Enable PCP and DEI classification */
    vtss_tagprio_t  pcp;            /**< PCP value */
    vtss_dei_t      dei;            /**< DEI value */
    BOOL            host_match;     /**< Host match */
    BOOL            isdx_enable;    /**< ISDX enable */
    u16             isdx;           /**< ISDX value */
    BOOL            pag_enable;     /**< PAG enable */
    u8              pag;            /**< PAG value */
    BOOL            pop_enable;     /**< VLAN POP enable */
    u8              pop;            /**< VLAN POP count */
#if defined(VTSS_ARCH_SERVAL_CE)
    vtss_mce_oam_detect_t oam_detect; /**< OAM detection */
#endif /* VTSS_ARCH_SERVAL_CE */
} vtss_is1_action_t;

typedef enum
{
    VTSS_IS1_TYPE_ANY,      /**< Any frame type */
    VTSS_IS1_TYPE_ETYPE,    /**< Ethernet Type */
    VTSS_IS1_TYPE_LLC,      /**< LLC */
    VTSS_IS1_TYPE_SNAP,     /**< SNAP */
    VTSS_IS1_TYPE_IPV4,     /**< IPv4 */
    VTSS_IS1_TYPE_IPV6,     /**< IPv6 */
    VTSS_IS1_TYPE_SMAC_SIP  /**< SMAC/SIP */
} vtss_is1_type_t;

typedef struct
{
    vtss_vcap_bit_t dmac_mc; /**< Multicast DMAC */
    vtss_vcap_bit_t dmac_bc; /**< Broadcast DMAC */
    vtss_vcap_u48_t smac;    /**< SMAC */
#if defined(VTSS_ARCH_SERVAL)
    vtss_vcap_u48_t dmac;    /**< DMAC */
#endif /* VTSS_ARCH_SERVAL */
} vtss_is1_mac_t; /**< MAC header */

typedef struct
{
    vtss_vcap_vr_t  vid;        /**< VLAN ID (12 bit) */
    vtss_vcap_u8_t  pcp;        /**< PCP (3 bit) */
    vtss_vcap_bit_t dei;        /**< DEI */
    vtss_vcap_bit_t tagged;     /**< Tagged frame */
    vtss_vcap_bit_t s_tag;      /**< S-tag type */
} vtss_is1_tag_t; /**< VLAN Tag */

typedef struct {
    vtss_vcap_u16_t etype; /**< Ethernet Type value */
    vtss_vcap_u32_t data;  /**< MAC data */
} vtss_is1_frame_etype_t;

typedef struct {
    vtss_vcap_u48_t data; /**< Data */
} vtss_is1_frame_llc_t;

typedef struct {
    vtss_vcap_u48_t data; /**< Data */
} vtss_is1_frame_snap_t;

typedef struct {
    vtss_vcap_bit_t ip_mc;    /**< IP_MC field */
    vtss_vcap_bit_t fragment; /**< Fragment */
    vtss_vcap_bit_t options;  /**< Header options */
    vtss_vcap_vr_t  dscp;     /**< DSCP field (6 bit) */
    vtss_vcap_u8_t  proto;    /**< Protocol */
    vtss_vcap_ip_t  sip;      /**< Source IP address */
#if defined(VTSS_ARCH_SERVAL)
    vtss_vcap_ip_t  dip;      /**< Destination IP address */
#endif /* VTSS_ARCH_SERVAL */
    vtss_vcap_vr_t  sport;    /**< UDP/TCP: Source port */
    vtss_vcap_vr_t  dport;    /**< UDP/TCP: Destination port */
} vtss_is1_frame_ipv4_t;

typedef struct {
    vtss_vcap_bit_t  ip_mc; /**< IP_MC field */
    vtss_vcap_vr_t   dscp;  /**< DSCP field (6 bit) */
    vtss_vcap_u8_t   proto; /**< Protocol */
    vtss_vcap_u128_t sip;   /**< Source IP address */
#if defined(VTSS_ARCH_SERVAL)
    vtss_vcap_u128_t dip;   /**< Destination IP adddress */
#endif /* VTSS_ARCH_SERVAL */
    vtss_vcap_vr_t   sport; /**< UDP/TCP: Source port */
    vtss_vcap_vr_t   dport; /**< UDP/TCP: Destination port */
} vtss_is1_frame_ipv6_t;

typedef struct {
    vtss_port_no_t port_no; /**< Ingress port or VTSS_PORT_NO_NONE */
    vtss_mac_t     smac;    /**< SMAC */
    vtss_ip_t      sip;     /**< Source IP address */
} vtss_is1_frame_smac_sip_t;

typedef struct {
    vtss_is1_type_t      type;      /**< Frame type */

#if defined(VTSS_ARCH_SERVAL)
    vtss_vcap_key_type_t key_type;  /**< Key type */
    vtss_vcap_u16_t      isdx;      /**< ISDX */
#endif /* VTSS_ARCH_SERVAL */

    BOOL                 port_list[VTSS_PORT_ARRAY_SIZE]; /**< Port list */
    vtss_is1_mac_t       mac;       /**< MAC header */
    vtss_is1_tag_t       tag;       /**< VLAN Tag */
    vtss_is1_tag_t       inner_tag; /**< Inner Tag, only 'tagged' field valid for L26/JR1 */

    union
    {
        /* VTSS_IS1_TYPE_ANY: No specific fields */
        vtss_is1_frame_etype_t    etype;    /**< VTSS_IS1_TYPE_ETYPE */
        vtss_is1_frame_llc_t      llc;      /**< VTSS_IS1_TYPE_LLC */
        vtss_is1_frame_snap_t     snap;     /**< VTSS_IS1_TYPE_SNAP */
        vtss_is1_frame_ipv4_t     ipv4;     /**< VTSS_IS1_TYPE_IPV4 */
        vtss_is1_frame_ipv6_t     ipv6;     /**< VTSS_IS1_TYPE_IPV6 */
        vtss_is1_frame_smac_sip_t smac_sip; /**< VTSS_IS1_TYPE_SMAC_SIP */
    } frame; /**< Frame type specific data */
} vtss_is1_key_t;

typedef struct {
    vtss_is1_action_t action;
    vtss_is1_key_t    key;
} vtss_is1_entry_t;

typedef struct {
    u8               lookup;      /* Lookup (Serval) or first flag (L26/JR) */
    u32              vid_range;   /* VID range */
    u32              dscp_range;  /* DSCP range */
    u32              sport_range; /* Source port range */
    u32              dport_range; /* Destination port range */
    vtss_is1_entry_t *entry; /* IS1 entry */
} vtss_is1_data_t;
#endif /* VTSS_FEATURE_IS1 */

#if defined(VTSS_FEATURE_IS2)
typedef struct {
    BOOL       first;        /* First or second lookup */
#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
    BOOL       host_match;   /* Host match from IS1 */
    BOOL       udp_tcp_any;  /* Match UDP/TCP frames */
#endif /* VTSS_ARCH_LUTON26/SERVAL */
#if defined(VTSS_ARCH_JAGUAR_1)
    u32        type;                /* Entry type */
    u32        type_mask;           /* Type mask */
    u32        action_ext;          /* Action extensions */
    u32        chip_port_mask[2];   /* Mask used when non-zero, one per possible chip */
    BOOL       include_int_ports;   /* Include internal ports for VTSS_PORT_NO_ANY */
    BOOL       include_stack_ports; /* Include stack ports for VTSS_PORT_NO_ANY */
#endif /* VTSS_ARCH_JAGUAR_1 */
    vtss_ace_t ace;          /* ACE structure */
} vtss_is2_entry_t;

typedef struct {
    u32              srange;       /* Source port range */
    u32              drange;       /* Destination port range */
#if defined(VTSS_ARCH_LUTON26)
    u8               policer_type; /* Policer type */
    u8               policer;      /* Allocated policer index */
#endif /* VTSS_ARCH_LUTON26 */
    vtss_is2_entry_t *entry; /* ACE data */
} vtss_is2_data_t;
#endif /* VTSS_FEATURE_IS2 */

#if defined(VTSS_FEATURE_ES0)
/* ES0 tag */
typedef enum {
    VTSS_ES0_TAG_NONE, /* No ES0 tag */
    VTSS_ES0_TAG_ES0,  /* ES0 tag only */
    VTSS_ES0_TAG_PORT, /* ES0 tag and port tag, if enabled (not valid for Serval inner_tag) */
    VTSS_ES0_TAG_BOTH  /* ES0 and port tag (not valid for Serval) */
} vtss_es0_tag_t;

/* ES0 TPI */
typedef enum {
    VTSS_ES0_TPID_C,   /* C-tag */
    VTSS_ES0_TPID_S,   /* S-tag */
    VTSS_ES0_TPID_PORT /* Port tag type */
} vtss_es0_tpid_t;

/* ES0 QOS */
typedef enum {
    VTSS_ES0_QOS_CLASS,
    VTSS_ES0_QOS_ES0,
    VTSS_ES0_QOS_PORT,
    VTSS_ES0_QOS_MAPPED
} vtss_es0_qos_t;

#if defined(VTSS_ARCH_SERVAL)
/* ES0 VID information */
typedef struct {
    BOOL       sel; /* Enable to select value (default classified) */
    vtss_vid_t val; /* VLAN ID value */
} vtss_es0_vid_t;

/* ES0 PCP selection */
typedef enum {
    VTSS_ES0_PCP_CLASS,  /* Classified PCP */
    VTSS_ES0_PCP_ES0,    /* PCP_VAL in ES0 */
    VTSS_ES0_PCP_MAPPED, /* Mapped PCP */
    VTSS_ES0_PCP_QOS     /* QoS class */
} vtss_es0_pcp_sel_t;

/* ES0 PCP information */
typedef struct {
    vtss_es0_pcp_sel_t sel; /* PCP selection */
    vtss_tagprio_t     val; /* PCP value */
} vtss_es0_pcp_t;

/* ES0 DEI selection */
typedef enum {
    VTSS_ES0_DEI_CLASS,  /* Classified DEI */
    VTSS_ES0_DEI_ES0,    /* DEI_VAL in ES0 */
    VTSS_ES0_DEI_MAPPED, /* Mapped DEI */
    VTSS_ES0_DEI_DP      /* DP value */
} vtss_es0_dei_sel_t;

/* ES0 DEI information */
typedef struct {
    vtss_es0_dei_sel_t sel; /* DEI selection */
    vtss_dei_t         val; /* DEI value */
} vtss_es0_dei_t;

/* ES0 tag information */
typedef struct {
    vtss_es0_tag_t  tag;  /* Tag selection */
    vtss_es0_tpid_t tpid; /* TPID selection */
    vtss_es0_vid_t  vid;  /* VLAN ID selection and value */
    vtss_es0_pcp_t  pcp;  /* PCP selection and value */
    vtss_es0_dei_t  dei;  /* DEI selection and value */
} vtss_es0_tag_conf_t;

#endif /* VTSS_ARCH_SERVAL */

#if defined(VTSS_ARCH_SERVAL_CE)
typedef enum {
    VTSS_ES0_MPLS_ENCAP_LEN_NONE = 0,
    VTSS_ES0_MPLS_ENCAP_LEN_14   = 2,
    VTSS_ES0_MPLS_ENCAP_LEN_18   = 3,
    VTSS_ES0_MPLS_ENCAP_LEN_22   = 4,
    VTSS_ES0_MPLS_ENCAP_LEN_26   = 5,
    VTSS_ES0_MPLS_ENCAP_LEN_30   = 6,
    VTSS_ES0_MPLS_ENCAP_LEN_34   = 7
} vtss_es0_mpls_encap_len_t;
#endif /* VTSS_ARCH_SERVAL_CE */

/* ES0 action */
typedef struct {
#if defined(VTSS_ARCH_SERVAL)
    /* For Serval, the following two fields replace the tag related fields below */
    vtss_es0_tag_conf_t       outer_tag;
    vtss_es0_tag_conf_t       inner_tag;
    BOOL                      mep_idx_enable;
    u8                        mep_idx;
#if defined(VTSS_FEATURE_MPLS)
    u32                       mpls_encap_idx;
    vtss_es0_mpls_encap_len_t mpls_encap_len;
#endif /* VTSS_FEATURE_MPLS */
#endif /* VTSS_ARCH_SERVAL */

    vtss_es0_tag_t  tag;
    vtss_es0_tpid_t tpid;
    vtss_es0_qos_t  qos;
    vtss_vid_t      vid_a; /* Outer/port tag */
    vtss_vid_t      vid_b; /* Inner/ES0 tag */
    u8              pcp;
    BOOL            dei;

    u16             esdx;  /* Jaguar/Serval only */
} vtss_es0_action_t;

typedef enum
{
    VTSS_ES0_TYPE_VID,
    VTSS_ES0_TYPE_ISDX /* Jaguar only */
} vtss_es0_type_t;

/* ES0 key */
typedef struct {
    vtss_port_no_t  port_no;   /* Port number or VTSS_PORT_NO_NONE */
    vtss_es0_type_t type;      /* Jaguar only */
    vtss_vcap_bit_t isdx_neq0; /* Jaguar only */

    union {
        struct {
            vtss_vid_t     vid;
            vtss_vcap_u8_t pcp;
        } vid;

        struct {
            u16            isdx;
            vtss_vcap_u8_t pcp;
        } isdx;
    } data;
} vtss_es0_key_t;

/* ES0 entry */
typedef struct {
    vtss_es0_action_t action;
    vtss_es0_key_t    key;
} vtss_es0_entry_t;

#define VTSS_ES0_FLAG_MASK_PORT 0x00ff /* Flags related to ES0 egress port */
#define VTSS_ES0_FLAG_MASK_NNI  0xff00 /* Flags related to ES0 NNI port */

#define VTSS_ES0_FLAG_TPID      0x0001 /* Use port TPID in outer tag */
#define VTSS_ES0_FLAG_QOS       0x0002 /* Use port QoS setup in outer tag */
#define VTSS_ES0_FLAG_PCP_MAP   0x0100 /* Use mapped PCP for ECE */

/* ES0 data */
typedef struct {
    vtss_es0_entry_t *entry;
    u16              flags; 
    u8               prio;    /* ECE priority */
    vtss_port_no_t   port_no; /* Egress port number */
    vtss_port_no_t   nni;     /* NNI port affecting ES0 entry for UNI port */
} vtss_es0_data_t;
#endif /* VTSS_FEATURE_ES0 */

/* VCAP data */
typedef struct {
    vtss_vcap_key_size_t key_size;   /* Key size */
    union {
#if defined(VTSS_FEATURE_IS0)
        vtss_is0_data_t is0;
#endif /* VTSS_FEATURE_IS0 */
#if defined(VTSS_FEATURE_IS1)
        vtss_is1_data_t is1;
#endif /* VTSS_FEATURE_IS1 */
#if defined(VTSS_FEATURE_IS2)
        vtss_is2_data_t is2;
#endif /* VTSS_FEATURE_IS2 */
#if defined(VTSS_FEATURE_ES0)
        vtss_es0_data_t es0;
#endif /* VTSS_FEATURE_ES0 */
    } u;
} vtss_vcap_data_t;

/* VCAP entry */
typedef struct vtss_vcap_entry_t {
    struct vtss_vcap_entry_t *next; /* Next in list */
    vtss_vcap_user_t         user;  /* User */
    vtss_vcap_id_t           id;    /* Entry ID */
    vtss_vcap_data_t         data;  /* Entry data */
} vtss_vcap_entry_t;

/* VCAP rule index */
typedef struct {
    u32                  row;      /* TCAM row */
    u32                  col;      /* TCAM column */
    vtss_vcap_key_size_t key_size; /* Rule key size */
} vtss_vcap_idx_t;

/* VCAP object */
typedef struct {
    /* VCAP data */
    u32               max_count;      /* Maximum number of rows */
    u32               count;          /* Actual number of rows */
    u32               max_rule_count; /* Maximum number of rules */
    u32               rule_count;     /* Actual number of rules */
    u32               key_count[VTSS_VCAP_KEY_SIZE_MAX]; /* Actual number of rule per key */
    vtss_vcap_entry_t *used;          /* Used entries */
    vtss_vcap_entry_t *free;          /* Free entries */
    const char        *name;          /* VCAP name for debugging */

    /* VCAP methods */
    vtss_rc (* entry_get)(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear);
    vtss_rc (* entry_add)(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter);
    vtss_rc (* entry_del)(vtss_vcap_idx_t *idx);
    vtss_rc (* entry_move)(vtss_vcap_idx_t *idx, u32 count, BOOL up);
} vtss_vcap_obj_t;

/* Special VCAP ID used to add last in list */
#define VTSS_VCAP_ID_LAST 0

/* VCAP ranges */
#define VTSS_VCAP_RANGE_CHK_CNT 8
#define VTSS_VCAP_RANGE_CHK_NONE 0xffffffff

/* VCAP range checker type */
typedef enum {
    VTSS_VCAP_RANGE_TYPE_SPORT,  /* UDP/TCP source port */
    VTSS_VCAP_RANGE_TYPE_DPORT,  /* UDP/TCP destination port */
    VTSS_VCAP_RANGE_TYPE_SDPORT, /* UDP/TCP source/destination port */
    VTSS_VCAP_RANGE_TYPE_VID,    /* VLAN ID */
    VTSS_VCAP_RANGE_TYPE_DSCP    /* DSCP */
} vtss_vcap_range_chk_type_t;

/* VCAP range checker entry */
typedef struct {
    vtss_vcap_range_chk_type_t type;  /* Range type */
    u32                        count; /* Reference count */
    u32                        min;   /* Lower value of range */
    u32                        max;   /* Upper value of range */
} vtss_vcap_range_chk_t;

/* VCAP range checker table */
typedef struct {
    vtss_vcap_range_chk_t entry[VTSS_VCAP_RANGE_CHK_CNT];
} vtss_vcap_range_chk_table_t;
#endif /* VTSS_FEATURE_VCAP */

#if defined(VTSS_FEATURE_IS0)
/* Number of IS0 rules */
#define VTSS_JR1_IS0_CNT  4096
#define VTSS_SRVL_IS0_CNT 768  /* Half entries */

#if defined(VTSS_ARCH_JAGUAR_1)
#define VTSS_IS0_CNT      VTSS_JR1_IS0_CNT
#else
#define VTSS_IS0_CNT      VTSS_SRVL_IS0_CNT
#endif /* VTSS_JR1_IS0_CNT */

/* IS0 information */
typedef struct {
    vtss_vcap_obj_t   obj;                 /* Object */
    vtss_vcap_entry_t table[VTSS_IS0_CNT]; /* Table */
} vtss_is0_info_t;
#endif /* VTSS_FEATURE_IS1 */

#if defined(VTSS_FEATURE_IS1)
/* Number of IS1 rules */
#define VTSS_L26_IS1_CNT  256
#define VTSS_SRVL_IS1_CNT 1024 /* Quarter entries */
#define VTSS_JR1_IS1_CNT  512

#if defined(VTSS_ARCH_JAGUAR_1)
#define VTSS_IS1_CNT      VTSS_JR1_IS1_CNT
#elif defined(VTSS_ARCH_SERVAL)
#define VTSS_IS1_CNT      VTSS_SRVL_IS1_CNT
#else
#define VTSS_IS1_CNT      VTSS_L26_IS1_CNT
#endif

/* IS1 information */
typedef struct {
    vtss_vcap_obj_t   obj;                 /* Object */
    vtss_vcap_entry_t table[VTSS_IS1_CNT]; /* Table */
} vtss_is1_info_t;
#endif /* VTSS_FEATURE_IS1 */

#if defined(VTSS_FEATURE_IS2)
/* Number of IS2 rules */
#define VTSS_L28_IS2_CNT  128
#define VTSS_L26_IS2_CNT  256
#define VTSS_SRVL_IS2_CNT 1024 /* Quarter entries */
#define VTSS_JR1_IS2_CNT  512

#if defined(VTSS_ARCH_SERVAL)
#define VTSS_IS2_CNT      VTSS_SRVL_IS2_CNT
#elif defined(VTSS_ARCH_JAGUAR_1)
#define VTSS_IS2_CNT      VTSS_JR1_IS2_CNT
#elif defined(VTSS_ARCH_LUTON26)
#define VTSS_IS2_CNT      VTSS_L26_IS2_CNT
#else
#define VTSS_IS2_CNT      VTSS_L28_IS2_CNT
#endif /* VTSS_ARCH_LUTON28 */

/* IS2 information */
typedef struct {
    vtss_vcap_obj_t   obj;                 /* Object */
    vtss_vcap_entry_t table[VTSS_IS2_CNT]; /* Table */
} vtss_is2_info_t;
#endif /* VTSS_FEATURE_IS2 */

#if defined(VTSS_FEATURE_ES0)
#define VTSS_L26_ES0_CNT  256
#define VTSS_SRVL_ES0_CNT 1024
#define VTSS_JR1_ES0_CNT  4096

#if defined(VTSS_ARCH_JAGUAR_1)
#define VTSS_ES0_CNT VTSS_JR1_ES0_CNT
#elif defined(VTSS_ARCH_SERVAL)
#define VTSS_ES0_CNT VTSS_SRVL_ES0_CNT
#else
#define VTSS_ES0_CNT VTSS_L26_ES0_CNT
#endif /* VTSS_ARCH_JAGUAR_1 */

/* ES0 information */
typedef struct {
    vtss_vcap_obj_t   obj;                 /* Object */
    vtss_vcap_entry_t table[VTSS_ES0_CNT]; /* Table */
} vtss_es0_info_t;
#endif /* VTSS_FEATURE_ES0 */

#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
/* Numner of sources and destinations */
#define VTSS_IPMC_SRC_MAX 256
#define VTSS_IPMC_DST_MAX 2048

typedef union {
    vtss_ipv4_t ipv4; /* IPv4 address */
    vtss_ipv6_t ipv6; /* IPv6 address */
} vtss_ip_addr_internal_t;

/* IPMC source data */
typedef struct {
    vtss_ip_addr_internal_t sip; /* Source IP address */
    vtss_vid_t              vid; /* VLAN ID */
    vtss_vid_t              fid; /* FID allocated for this source (if SIP non-zero) */
    BOOL                    ssm; /* SSM indication (zero SIP) */
} vtss_ipmc_src_data_t;

/* IPMC destination data */
typedef struct {
    vtss_ip_addr_internal_t dip;                       /* Destination IP address */
    u8                      member[VTSS_PORT_BF_SIZE]; /* Port members */
} vtss_ipmc_dst_data_t;

/* IPMC source and destination data */
typedef struct {
    BOOL                 ipv6;    /* IPv4/IPv6 indication */
    BOOL                 src_add; /* Source add flag */
    BOOL                 src_del; /* Source delete flag */
    BOOL                 dst_add; /* destination add flag */
    BOOL                 dst_del; /* destionation delete flag */
    vtss_ipmc_src_data_t src;
    vtss_ipmc_dst_data_t dst;
} vtss_ipmc_data_t;

/* IPMC destination entry */
typedef struct vtss_ipmc_dst_t {
    struct vtss_ipmc_dst_t *next; /* Next destination entry */
    vtss_ipmc_dst_data_t   data;  /* Entry data */
    BOOL                   add;   /* Internal add flag */
} vtss_ipmc_dst_t;

/* IPMC source entry */
typedef struct vtss_ipmc_src_t {
    struct vtss_ipmc_src_t *next;  /* Next source entry */
    vtss_ipmc_dst_t        *dest;  /* Destination list */
    vtss_ipmc_src_data_t   data;   /* Entry data */
} vtss_ipmc_src_t;

/* IPMC object */
typedef struct {
    u32             src_count;    /* Actual number of sources */
    u32             src_max;      /* Maximum number of sources */
    u32             dst_count;    /* Actual number of destinations */
    u32             dst_max;      /* Maximum number of destinations */
    vtss_ipmc_src_t *src_used[2]; /* Used IPv4(0)/IPv6(1) source entries */
    vtss_ipmc_src_t *src_free;    /* Free source entries */
    vtss_ipmc_dst_t *dst_free;    /* Free destination entries */
    const char      *name;        /* IPMC name for debugging */
} vtss_ipmc_obj_t;

typedef struct {
    vtss_ipmc_obj_t obj;                          /* Object */
    vtss_ipmc_src_t src_table[VTSS_IPMC_SRC_MAX]; /* Source table */
    vtss_ipmc_dst_t dst_table[VTSS_IPMC_DST_MAX]; /* Destination table */
} vtss_ipmc_info_t;

typedef enum {
    VTSS_IPMC_CMD_CHECK, /* Check resources */
    VTSS_IPMC_CMD_ADD,   /* Add resources */
    VTSS_IPMC_CMD_DEL,   /* Delete resources */
} vtss_ipmc_cmd_t;
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */

#if defined(VTSS_FEATURE_TIMESTAMP)

#if defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_SERVAL)
#define VTSS_TS_ID_SIZE  63
#define TS_IDS_RESERVED_FOR_SW 4
#endif /* VTSS_ARCH_LUTON26 || VTSS_ARCH_SERVAL */

#if defined (VTSS_ARCH_SERVAL)
#define VTSS_VOE_ID_SIZE  VTSS_OAM_VOE_CNT
#endif /* VTSS_ARCH_SERVAL */

#if defined(VTSS_ARCH_JAGUAR_1)
#define VTSS_TS_ID_SIZE  3
#define TS_IDS_RESERVED_FOR_SW 3
#endif /* VTSS_ARCH_JAGUAR_1 */

#endif  /* VTSS_FEATURE_TIMESTAMP */

#if defined(VTSS_FEATURE_AFI_SWC)
#define VTSS_AFI_TIMER_CNT 8

typedef enum {
    VTSS_AFI_SLOT_STATE_FREE,     /**< Slot is not in use                                                 */
    VTSS_AFI_SLOT_STATE_RESERVED, /**< Slot has been allocated, but no frames have been injected using it */
    VTSS_AFI_SLOT_STATE_ENABLED,  /**< Slot is allocated and a frame is using it                          */
    VTSS_AFI_SLOT_STATE_PAUSED,   /**< Slot is allocated, but currently paused (disabled in switch core)  */
} vtss_afi_slot_state_t;

typedef struct {
    /**
     * The current state of this slot.
     * The remaining members are only valid if #state != VTSS_AFI_SLOT_STATE_FREE.
     */
    vtss_afi_slot_state_t state;

    /**
     * Which timer is this slot using?
     * This points to an entry of type vtss_afi_timer_conf_t.
     */
    u32 timer_idx;

    /**
     * Which port number is this slot serving?
     */
    vtss_port_no_t port_no;
} vtss_afi_slot_conf_t;

typedef struct {
    u32 ref_cnt;
    u32 fps;
} vtss_afi_timer_conf_t;
#endif /* VTSS_FEATURE_AFI_SWC */

/* Opaque forward declaration */
struct vtss_state_s;

/* ================================================================= *
 *  Chip Interface Layer functions
 * ================================================================= */
typedef struct {
    /* Initialization */
    vtss_rc (* init_conf_set)(void);
    vtss_rc (* reg_read)(const vtss_chip_no_t chip_no, const u32 addr, u32 * const value);
    vtss_rc (* reg_write)(const vtss_chip_no_t chip_no, const u32 addr, const u32 value);

#if defined(VTSS_FEATURE_WARM_START)
    vtss_rc (* restart_conf_set)(void);
#endif /* VTSS_FEATURE_WARM_START */
#if defined(VTSS_GPIOS)
    vtss_rc (* gpio_mode)(const vtss_chip_no_t   chip_no,
                          const vtss_gpio_no_t   gpio_no,
                          const vtss_gpio_mode_t mode);
    vtss_rc (* gpio_read)(const vtss_chip_no_t  chip_no,
                          const vtss_gpio_no_t  gpio_no,
                          BOOL                  *const value);
    vtss_rc (* gpio_write)(const vtss_chip_no_t  chip_no,
                           const vtss_gpio_no_t  gpio_no,
                           const BOOL            value);
    vtss_rc (* gpio_event_poll)(const vtss_chip_no_t  chip_no,
                                BOOL                  *const events);
    vtss_rc (* gpio_event_enable)(const vtss_chip_no_t  chip_no,
                                  const vtss_gpio_no_t  gpio_no,
                                  const BOOL            enable);
#if defined(VTSS_ARCH_B2)
    vtss_rc (* gpio_clk_set)(const vtss_gpio_no_t         gpio_no,
                             const vtss_port_no_t         port_no,
                             const vtss_recovered_clock_t clk);

    vtss_rc (* vid2port_set)(const vtss_vid_t     vid,
                            const vtss_port_no_t port_no);

    vtss_rc (* vid2lport_set)(const vtss_vid_t     vid,
                             const vtss_lport_no_t port_no);

    vtss_rc (* dscp_table_set)(const vtss_dscp_table_no_t table_no);

    vtss_rc (* qos_lport_conf_set)(const vtss_lport_no_t lport_no);

    vtss_rc (* port_filter_set)(const vtss_port_no_t port_no,
                                const vtss_port_filter_t * const filter);

#endif /* VTSS_ARCH_B2 */
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC) || defined(VTSS_ARCH_B2)
    vtss_rc (* host_conf_set)(vtss_port_no_t port_no);
#endif

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    vtss_rc (* xaui_conf_set)(vtss_port_no_t port_no);
    vtss_rc (* qos_lport_conf_set)(const vtss_lport_no_t lport_no);
    vtss_rc (* qos_lport_conf_get)(const vtss_lport_no_t       lport_no,
                                   const vtss_qos_lport_conf_t *const conf);
    BOOL    (* port_is_host)(const vtss_port_no_t port_no);
    vtss_rc (* lport_counters_get)(const vtss_lport_no_t lport_no,
                                   vtss_lport_counters_t *const counters);
    vtss_rc (* lport_counters_clear)(const vtss_lport_no_t lport_no);
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

#endif /* VTSS_GPIOS */
#if defined(VTSS_FEATURE_MISC)
    vtss_rc (* chip_id_get)(vtss_chip_id_t *const chip_id);
    vtss_rc (* intr_sticky_clear)(const struct vtss_state_s *const state,
                                  u32 ext);
    vtss_rc (* poll_1sec)(void);
    vtss_rc (* ptp_event_poll)(vtss_ptp_event_type_t  *const ev_mask);
    vtss_rc (* ptp_event_enable)(vtss_ptp_event_type_t ev_mask, BOOL enable);
    vtss_rc (* dev_all_event_poll)(vtss_dev_all_event_poll_t poll_type, vtss_dev_all_event_type_t  *const ev_mask);
    vtss_rc (* dev_all_event_enable)(vtss_port_no_t port, vtss_dev_all_event_type_t ev_mask, BOOL enable);
#if defined(VTSS_FEATURE_SERIAL_LED)
    vtss_rc (* serial_led_set)(const vtss_led_port_t  port,
                               const vtss_led_mode_t  mode[3]);

    vtss_rc (* serial_led_intensity_set)(const vtss_led_port_t  port,
                                         const i8               intensity);

    vtss_rc (* serial_led_intensity_get)(i8               *intensity);
#endif /* VTSS_FEATURE_SERIAL_LED */
#if defined(VTSS_FEATURE_SERIAL_GPIO)
    vtss_rc (* sgpio_conf_set)(const vtss_chip_no_t     chip_no,
                               const vtss_sgpio_group_t group,
                               const vtss_sgpio_conf_t  *const conf);
    vtss_rc (* sgpio_read)(const vtss_chip_no_t     chip_no,
                           const vtss_sgpio_group_t group,
                           vtss_sgpio_port_data_t   data[VTSS_SGPIO_PORTS]);
    vtss_rc (* sgpio_event_enable)(const vtss_chip_no_t     chip_no,
                                   const vtss_sgpio_group_t group,
                                   const vtss_port_no_t     port,
                                   const u32                bit,
                                   BOOL                     enable);
    vtss_rc (* sgpio_event_poll)(const vtss_chip_no_t     chip_no,
                                 const vtss_sgpio_group_t group,
                                 const u32                bit,
                                 BOOL                     *const events);
#endif /* VTSS_FEATURE_SERIAL_GPIO */
#endif /* VTSS_FEATURE_MISC */
    vtss_rc (* debug_info_print)(const vtss_debug_printf_t prntf,
                                 const vtss_debug_info_t   *const info);
#if defined(VTSS_FEATURE_PORT_CONTROL)
    /* Port control */
    vtss_rc (* miim_read)(vtss_miim_controller_t miim_controller,
                          u8 miim_addr,
                          u8 addr,
                          u16 *value,
                          BOOL report_errors);
    vtss_rc (* miim_write)(vtss_miim_controller_t miim_controller,
                           u8 miim_addr,
                           u8 addr,
                           u16 value,
                           BOOL report_errors);
#if defined(VTSS_FEATURE_10G)
    vtss_rc (* mmd_read)(vtss_miim_controller_t miim_controller,
                         u8 miim_addr, u8 mmd, u16 addr, u16 *value, BOOL report_errors);
    vtss_rc (* mmd_read_inc)(vtss_miim_controller_t miim_controller,
                         u8 miim_addr, u8 mmd, u16 addr, u16 *buf, u8 count, BOOL report_errors);
    vtss_rc (* mmd_write)(vtss_miim_controller_t miim_controller,
                          u8 miim_addr, u8 mmd, u16 addr, u16 data, BOOL report_errors);
#endif /* VTSS_FEATURE_10G */
    vtss_rc (* port_status_interface_set)(const u32 clock);
    vtss_rc (* port_status_interface_get)(u32 *clock);
    vtss_rc (* port_map_set)(void);
    vtss_rc (* port_conf_get)(const vtss_port_no_t port_no,
                              vtss_port_conf_t *const conf);
    vtss_rc (* port_conf_set)(const vtss_port_no_t    port_no);

    vtss_rc (* port_ifh_set)(const vtss_port_no_t    port_no);

    vtss_rc (* port_clause_37_status_get)(const vtss_port_no_t         port_no,
                                          vtss_port_clause_37_status_t *const status);
    vtss_rc (* port_clause_37_control_get)(const vtss_port_no_t           port_no,
                                           vtss_port_clause_37_control_t  *const control);
    vtss_rc (* port_clause_37_control_set)(const vtss_port_no_t port_no);
    vtss_rc (* port_status_get)(const vtss_port_no_t  port_no,
                                vtss_port_status_t    *const status);
    vtss_rc (* port_counters_update)(const vtss_port_no_t port_no);
    vtss_rc (* port_counters_clear)(const vtss_port_no_t port_no);
    vtss_rc (* port_counters_get)(const vtss_port_no_t port_no,
                                  vtss_port_counters_t *const counters);
    vtss_rc (* port_basic_counters_get)(const vtss_port_no_t port_no,
                                        vtss_basic_counters_t *const counters);
    vtss_rc (* port_forward_set)(const vtss_port_no_t port_no);
#endif /* VTSS_FEATURE_PORT_CONTROL */

#if defined(VTSS_FEATURE_INTERRUPTS)
    vtss_rc (* intr_cfg)(const u32 mask, const BOOL polarity, const BOOL enable);
    vtss_rc (* intr_pol_negation)(void);
    vtss_rc (* intr_status_get)(vtss_intr_t *status);
    vtss_rc (* intr_mask_set)(vtss_intr_t *mask);
#endif // VTSS_FEATURE_INTERRUPTS
#if defined(VTSS_FEATURE_QOS)
    vtss_rc (* qos_conf_set)(BOOL changed);
    vtss_rc (* qos_port_conf_set)(const vtss_port_no_t port_no);
    vtss_rc (* qos_port_conf_update)(const vtss_port_no_t port_no);
#if defined(VTSS_FEATURE_QCL)
    vtss_rc (* qce_add)(const vtss_qcl_id_t  qcl_id,
                        const vtss_qce_id_t  qce_id,
                        const vtss_qce_t     *const qce);
    vtss_rc (* qce_del)(const vtss_qcl_id_t  qcl_id,
                        const vtss_qce_id_t  qce_id);
#endif /* VTSS_FEATURE_QCL */
#endif /* VTSS_FEATURE_QOS */

#if defined(VTSS_FEATURE_AFI_SWC)
    vtss_rc (*afi_alloc)(const vtss_afi_frm_dscr_t *const dscr, vtss_afi_id_t *const id);
    vtss_rc (*afi_free)(vtss_afi_id_t id);
#endif /* defined(VTSS_FEATURE_AFI_SWC) */

#if defined(VTSS_FEATURE_PACKET)
    vtss_rc (* rx_conf_set)(void);
    vtss_rc (* rx_frame_get)(const vtss_packet_rx_queue_t  queue_no,
                             vtss_packet_rx_header_t       *const header,
                             u8                            *const frame,
                             const u32                     length);
    vtss_rc (* rx_frame_discard)(const vtss_packet_rx_queue_t  queue_no);
    vtss_rc (* tx_frame_port)(const vtss_port_no_t  port_no,
                              const u8              *const frame,
                              const u32             length,
                              const vtss_vid_t      vid);
    vtss_rc (*rx_hdr_decode)(const struct vtss_state_s   *const state,
                             const vtss_packet_rx_meta_t *const meta,
                                   vtss_packet_rx_info_t *const info);
    vtss_rc (*tx_hdr_encode)(      struct vtss_state_s   *const state,
                             const vtss_packet_tx_info_t *const info,
                                   u8                    *const bin_hdr,
                                   u32                   *const bin_hdr_len);
#endif /* VTSS_FEATURE_PACKET */

#if defined(VTSS_FEATURE_NPI)
    vtss_rc (*npi_conf_set)(const vtss_npi_conf_t *const conf);
#endif /* VTSS_FEATURE_NPI */

#if defined(VTSS_FEATURE_LAYER2)
    /* Layer 2 */
    vtss_rc (* mac_table_add)(const vtss_mac_table_entry_t *const entry, u32 pgid);
    vtss_rc (* mac_table_del)(const vtss_vid_mac_t *const vid_mac);
    vtss_rc (* mac_table_get)(vtss_mac_table_entry_t *const entry, u32 *pgid);
    vtss_rc (* mac_table_get_next)(vtss_mac_table_entry_t *const entry, u32 *pgid);
    vtss_rc (* mac_table_age_time_set)(void);
    vtss_rc (* mac_table_age)(BOOL             pgid_age,
                              u32              pgid,
                              BOOL             vid_age,
                              const vtss_vid_t vid);
    vtss_rc (* mac_table_status_get)(vtss_mac_table_status_t *const status);
#if defined(VTSS_FEATURE_VSTAX_V2)
    vtss_rc (* mac_table_upsid_upspn_flush)(const vtss_vstax_upsid_t upsid,
                                            const vtss_vstax_upspn_t upspn);
#endif /* VTSS_FEATURE_VSTAX_V2 */
    vtss_rc (* learn_port_mode_set)(const vtss_port_no_t port_no);
    vtss_rc (* learn_state_set)(const BOOL member[VTSS_PORT_ARRAY_SIZE]);
    vtss_rc (* mstp_vlan_msti_set)(const vtss_vid_t vid);
    vtss_rc (* mstp_state_set)(const vtss_port_no_t port_no,
                               const vtss_msti_t    msti);
    vtss_rc (* erps_vlan_member_set)(const vtss_erpi_t erpi,
                                     const vtss_vid_t  vid);
    vtss_rc (* erps_port_state_set)(const vtss_erpi_t    erpi,
                                    const vtss_port_no_t port_no);
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    vtss_rc (* vlan_conf_set)(void);
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */
    vtss_rc (* vlan_port_conf_set)(const vtss_port_no_t port_no);
    vtss_rc (* vlan_port_conf_update)(vtss_port_no_t port_no, vtss_vlan_port_conf_t *conf);
    vtss_rc (* vlan_port_members_set)(const vtss_vid_t vid);
#if defined(VTSS_FEATURE_VLAN_TX_TAG)
    vtss_rc (* vlan_tx_tag_set)(const vtss_vid_t vid,
                                const vtss_vlan_tx_tag_t tx_tag[VTSS_PORT_ARRAY_SIZE]);
#endif /* VTSS_FEATURE_VLAN_TX_TAG */
    vtss_rc (* vlan_mask_update)(vtss_vid_t vid, BOOL member[VTSS_PORT_ARRAY_SIZE]);
    vtss_rc (* isolated_vlan_set)(const vtss_vid_t vid);
    vtss_rc (* isolated_port_members_set)(void);
    vtss_rc (* aggr_mode_set)(void);
    vtss_rc (* mirror_port_set)(void);
    vtss_rc (* mirror_ingress_set)(void);
    vtss_rc (* mirror_egress_set)(void);
    vtss_rc (* mirror_cpu_ingress_set)(void);
    vtss_rc (* mirror_cpu_egress_set)(void);
    vtss_rc (* eps_port_set)(const vtss_port_no_t port_no);
    vtss_rc (* flood_conf_set)(void);
#if defined(VTSS_FEATURE_IPV4_MC_SIP)
    vtss_rc (* ipv4_mc_add)(vtss_vid_t vid, vtss_ip_t sip, vtss_ip_t dip,
                            const BOOL member[VTSS_PORT_ARRAY_SIZE]);
    vtss_rc (* ipv4_mc_del)(vtss_vid_t vid, vtss_ip_t sip, vtss_ip_t dip);
#endif /* VTSS_FEATURE_IPV4_MC_SIP */
#if defined(VTSS_FEATURE_IPV6_MC_SIP)
    vtss_rc (* ipv6_mc_add)(vtss_vid_t vid, vtss_ipv6_t sip, vtss_ipv6_t dip,
                            const BOOL member[VTSS_PORT_ARRAY_SIZE]);
    vtss_rc (* ipv6_mc_del)(vtss_vid_t vid, vtss_ipv6_t sip, vtss_ipv6_t dip);
#endif /* VTSS_FEATURE_IPV6_MC_SIP */
#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
    vtss_rc (* ip_mc_update)(vtss_ipmc_data_t *ipmc, vtss_ipmc_cmd_t cmd);
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */
#if defined(VTSS_FEATURE_AGGR_GLAG) && defined(VTSS_FEATURE_VSTAX_V2)
    vtss_rc (* glag_src_table_write)(u32 glag_no, u32 port_count,
                                     BOOL member[VTSS_PORT_ARRAY_SIZE]);
    vtss_rc (* glag_port_write)(u32 glag_no, u32 idx, vtss_vstax_glag_entry_t *entry);
#endif /* VTSS_FEATURE_AGGR_GLAG && VTSS_FEATURE_VSTAX_V2 */
    vtss_rc (* src_table_write)(vtss_port_no_t port_no, BOOL member[VTSS_PORT_ARRAY_SIZE]);
    vtss_rc (* pgid_table_write)(u32 pgid, BOOL member[VTSS_PORT_ARRAY_SIZE]);
    vtss_rc (* aggr_table_write)(u32 ac, BOOL member[VTSS_PORT_ARRAY_SIZE]);
    vtss_rc (* pmap_table_write)(vtss_port_no_t port_no, vtss_port_no_t l_port_no);
#ifdef VTSS_FEATURE_SFLOW
    vtss_rc (*sflow_port_conf_set)(vtss_port_no_t port_no, const vtss_sflow_port_conf_t *const conf);
    vtss_rc (*sflow_sampling_rate_convert)(const struct vtss_state_s *const state, const BOOL power2, const u32 rate_in, u32 *const rate_out);
#endif /* VTSS_FEATURE_SFLOW */
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_VCL)
    vtss_rc (* vcl_port_conf_set)(const vtss_port_no_t port_no);
    vtss_rc (* vce_add)(const vtss_vce_id_t  vce_id,
                        const vtss_vce_t     *const vce);
    vtss_rc (* vce_del)(const vtss_vce_id_t  vce_id);
#endif /* VTSS_FEATURE_VCL */
#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
    vtss_rc (* vlan_trans_group_add) (const u16           group_id,
                                      const vtss_vid_t    vid,
                                      const vtss_vid_t    trans_vid);
    vtss_rc (* vlan_trans_group_del) (const u16           group_id,
                                      const vtss_vid_t    vid);
    vtss_rc (* vlan_trans_group_get) (vtss_vlan_trans_grp2vlan_conf_t *conf, BOOL next);
    vtss_rc (* vlan_trans_port_conf_set) (const vtss_vlan_trans_port2grp_conf_t *conf);
    vtss_rc (* vlan_trans_port_conf_get) (vtss_vlan_trans_port2grp_conf_t *conf, BOOL next);
#endif /* VTSS_FEATURE_VLAN_TRANSLATION */
#if defined(VTSS_ARCH_SERVAL)
    vtss_rc (* vcap_port_conf_set)(const vtss_port_no_t port_no);
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    vtss_rc (* vlan_counters_get)(const vtss_vid_t          vid,
                                  vtss_vlan_counters_t      *const counters);
    vtss_rc (* vlan_counters_clear)(const vtss_vid_t        vid);
#endif /* VTSS_FEATURE_VLAN_COUNTERS */
#if defined(VTSS_FEATURE_VSTAX)
    vtss_rc (* vstax_conf_set)(void);
    vtss_rc (* vstax_port_conf_set)(const BOOL stack_port_a);
    vtss_rc (* vstax_master_upsid_set)(void);
    vtss_rc (* vstax_header2frame)(const struct vtss_state_s     *const state,
                                   const vtss_port_no_t          port_no,
                                   const vtss_vstax_tx_header_t  *const header,
                                   u8                            *const frame);
    vtss_rc (* vstax_frame2header)(const struct vtss_state_s     *const state,
                                   const u8                  *const frame,
                                   vtss_vstax_rx_header_t    *const header);
#if defined(VTSS_FEATURE_VSTAX_V2)
    vtss_rc (* vstax_topology_set)(void);
#endif

#endif /* VTSS_FEATURE_VSTAX */

#if defined(VTSS_FEATURE_EEE)
    vtss_rc (*eee_port_conf_set)   (                                        const vtss_port_no_t port_no, const vtss_eee_port_conf_t    *const eee_conf);
    vtss_rc (*eee_port_state_set)  (const struct vtss_state_s *const state, const vtss_port_no_t port_no, const vtss_eee_port_state_t   *const eee_state);
    vtss_rc (*eee_port_counter_get)(const struct vtss_state_s *const state, const vtss_port_no_t port_no,       vtss_eee_port_counter_t *const eee_counter);
#endif /* VTSS_FEATURE_EEE */

#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    vtss_rc (* evc_policer_conf_set)(const vtss_evc_policer_id_t policer_id);
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */

#if defined(VTSS_FEATURE_EVC)
    vtss_rc (* evc_port_conf_set)(const vtss_port_no_t port_no);
    vtss_rc (* evc_add)(const vtss_evc_id_t evc_id, const vtss_evc_conf_t *const conf);
    vtss_rc (* evc_del)(const vtss_evc_id_t evc_id);
    vtss_rc (* ece_add)(const vtss_ece_id_t ece_id, const vtss_ece_t *const ece);
    vtss_rc (* ece_del)(const vtss_ece_id_t ece_id);
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
    vtss_rc (* mce_add)(const vtss_mce_id_t mce_id, const vtss_mce_t *const mce);
    vtss_rc (* mce_del)(const vtss_mce_id_t mce_id);
#if defined(VTSS_ARCH_SERVAL)
    vtss_rc (* mce_port_get)(const vtss_mce_id_t mce_id, const vtss_port_no_t port_no, 
                             vtss_mce_port_info_t *const info);
#endif /* VTSS_ARCH_SERVAL */
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    vtss_rc (* ece_counters_get)(const vtss_ece_id_t  ece_id,
                                 const vtss_port_no_t port_no,
                                 vtss_evc_counters_t  *const counters);
    vtss_rc (* ece_counters_clear)(const vtss_ece_id_t  ece_id,
                                   const vtss_port_no_t port_no);
    vtss_rc (* evc_counters_get)(const vtss_evc_id_t  evc_id,
                                 const vtss_port_no_t port_no,
                                 vtss_evc_counters_t  *const counters);
    vtss_rc (* evc_counters_clear)(const vtss_evc_id_t  evc_id,
                                   const vtss_port_no_t port_no);
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
    vtss_rc (* evc_update)(vtss_evc_id_t evc_id, vtss_res_t *res, vtss_res_cmd_t cmd);
    vtss_rc (* ece_update)(vtss_ece_entry_t *ece, vtss_res_t *res, vtss_res_cmd_t cmd);
#if defined(VTSS_SDX_CNT)
    vtss_rc (* sdx_counters_update)(vtss_sdx_entry_t *isdx, vtss_sdx_entry_t *esdx,
                                    vtss_evc_counters_t *const counters, BOOL clear);
#endif /* VTSS_SDX_CNT */
#endif /* VTSS_FEATURE_EVC */

#if defined(VTSS_FEATURE_FAN)
    vtss_rc (* fan_controller_init)(const vtss_fan_conf_t *const spec);
    vtss_rc (* fan_cool_lvl_set) (u8 lvl);
    vtss_rc (* fan_cool_lvl_get) (u8 *lvl);
    vtss_rc (* fan_rotation_get) (vtss_fan_conf_t *fan_spec, u32 *rotation_count);
    vtss_rc (* fan_chip_temp_get) (u8 *chip_temp);
#endif /* VTSS_FEATURE_FAN */

    vtss_rc (* chip_temp_get)  (i16 *chip_temp);
    vtss_rc (* chip_temp_init) (const BOOL enable);

#if defined(VTSS_FEATURE_ACL)
    vtss_rc (* acl_policer_set)(const vtss_acl_policer_no_t  policer_no);
    vtss_rc (* acl_port_set)(const vtss_port_no_t  port_no);
    vtss_rc (* acl_port_counter_get)(const vtss_port_no_t     port_no,
                                     vtss_acl_port_counter_t  *const counter);
    vtss_rc (* acl_port_counter_clear)(const vtss_port_no_t     port_no);
    vtss_rc (* acl_ace_add)(const vtss_ace_id_t  ace_id,
                            const vtss_ace_t     *const ace);
    vtss_rc (* acl_ace_del)(const vtss_ace_id_t  ace_id);
    vtss_rc (* acl_ace_counter_get)(const vtss_ace_id_t  ace_id,
                                    vtss_ace_counter_t   *const counter);
    vtss_rc (* acl_ace_counter_clear)(const vtss_ace_id_t  ace_id);
#endif /* VTSS_FEATURE_ACL */
#if defined(VTSS_FEATURE_VCAP)
     vtss_rc (* vcap_range_commit)(void);
 #endif /* VTSS_FEATURE_VCAP */

#if defined(VTSS_FEATURE_ANEG)
    vtss_rc (* aneg_set_config)(const vtss_port_no_t  port_no);
    vtss_rc (* aneg_set_ability)(const vtss_port_no_t  port_no);
    vtss_rc (* aneg_set_enable)(const vtss_port_no_t  port_no);
    vtss_rc (* aneg_restart)(const vtss_port_no_t  port_no);
    vtss_rc (* aneg_reset)(const vtss_port_no_t  port_no);
#endif /* VTSS_FEATURE_ANEG */
#if defined(VTSS_FEATURE_ES0)
    vtss_rc (* es0_entry_update)(vtss_vcap_idx_t *idx, vtss_es0_data_t *es0);
#endif /* VTSS_FEATURE_ES0 */

#if defined(VTSS_FEATURE_XFI)
    vtss_rc (* xfi_config_set)(const vtss_port_no_t  port_no);    
    vtss_rc (* xfi_ae_reset)(const vtss_port_no_t  port_no,vtss_xfi_lane_t lane);
    vtss_rc (* xfi_status_get)(const vtss_port_no_t  port_no, vtss_xfi_status_t *const status);
    vtss_rc (* xfi_test_config_set)(const vtss_port_no_t  port_no);    
    vtss_rc (* xfi_test_status_get)(const vtss_port_no_t  port_no, vtss_xfi_test_status_t *const status);
    vtss_rc (* xfi_event_enable)(vtss_port_no_t port_no, const BOOL enable, const vtss_xfi_event_t ev_mask);
    vtss_rc (* xfi_event_poll)(vtss_port_no_t port_no,vtss_xfi_event_t *const status );
    vtss_rc (* xfi_event_poll_without_mask)(vtss_port_no_t port_no,vtss_xfi_event_t *const status );
    vtss_rc (* xfi_recovered_clock_output_set)(const vtss_port_no_t port_no);
    vtss_rc (* xfi_txeq_mode_set)(const vtss_port_no_t port_no);
    vtss_rc (* xfi_txeq_coef_adjust) (const vtss_port_no_t port_no,
                                      vtss_xfi_ffe_coef_t coef,
                                      BOOL polarity);
    vtss_rc (* xfi_txamp_set) (const vtss_port_no_t port_no);
    vtss_rc (* xfi_txslew_set) (const vtss_port_no_t port_no);
    vtss_rc (* xfi_txeq_8023ap_coef_update) (const vtss_port_no_t port_no, vtss_xfi_lane_t lane,
                                             vtss_xfi_ffe_coef_t coef, vtss_txeq_8023ap_updreq_t updreq);
    vtss_rc (* xfi_txeq_8023ap_coef_stat_get) (const vtss_port_no_t port_no, vtss_xfi_lane_t lane, vtss_txeq_8023ap_coef_stat_t *ae_status);
    vtss_rc (* xfi_txeq_8023ap_fsm_ctl_set) (const vtss_port_no_t port_no, vtss_xfi_lane_t lane, u32 val);
    vtss_rc (* xfi_txeq_8023ap_fsm_stat_get) (const vtss_port_no_t port_no, vtss_xfi_lane_t lane, u32 *val);
    vtss_rc (* xfi_rxeq_mode_set) (const vtss_port_no_t port_no);
#endif /* VTSS_FEATURE_XFI */

#if defined(VTSS_FEATURE_UPI)
    vtss_rc (* upi_config_set)(const vtss_port_no_t  port_no);    
    vtss_rc (* upi_status_get)(const vtss_port_no_t  port_no, vtss_upi_status_t *const status);
    vtss_rc (* upi_test_config_set)(const vtss_port_no_t  port_no);    
    vtss_rc (* upi_test_status_get)(const vtss_port_no_t  port_no, vtss_upi_test_status_t *const status);
    vtss_rc (* upi_event_enable)(vtss_port_no_t port_no, const BOOL enable, const vtss_upi_event_t ev_mask);
    vtss_rc (* upi_event_poll)(vtss_port_no_t port_no,vtss_upi_event_t *const status );
    vtss_rc (* upi_event_poll_without_mask)(vtss_port_no_t port_no,vtss_upi_event_t *const status );
    vtss_rc (* upi_txeq_mode_set)(const vtss_port_no_t port_no);
    vtss_rc (* upi_txeq_coef_adjust) (const vtss_port_no_t port_no,
                                      vtss_upi_ffe_coef_t coef,
                                      BOOL polarity);
    vtss_rc (* upi_txamp_set) (const vtss_port_no_t port_no);
    vtss_rc (* upi_txslew_set) (const vtss_port_no_t port_no);
    vtss_rc (* upi_rxeq_mode_set) (const vtss_port_no_t port_no);
#endif /* VTSS_FEATURE_UPI */

#if defined(VTSS_FEATURE_TFI5)
    vtss_rc (* tfi5_set_config)(const vtss_port_no_t  port_no);    
    vtss_rc (* tfi5_set_enable)(const vtss_port_no_t  port_no);
    vtss_rc (* tfi5_reset)(const vtss_port_no_t  port_no, BOOL enable);    
    vtss_rc (* tfi5_set_loopback)(const vtss_port_no_t  port_no );        
#endif /* VTSS_FEATURE_TFI5 */

#if defined(VTSS_FEATURE_SFI4)
    vtss_rc (* sfi4_set_config)(const vtss_port_no_t  port_no);    
    vtss_rc (* sfi4_set_enable)(const vtss_port_no_t  port_no);    
    vtss_rc (* sfi4_set_loopback)(const vtss_port_no_t  port_no ); 
#endif /* VTSS_FEATURE_SFI4 */

#if defined(VTSS_FEATURE_XAUI)
    vtss_rc (* xaui_config_set)(const vtss_port_no_t  port_no);    
    vtss_rc (* xaui_status_get)(const vtss_port_no_t  port_no, vtss_xaui_status_t *const status);    
    vtss_rc (* xaui_counters_get) (const vtss_port_no_t port_no, vtss_xaui_pm_cnt_t *const cnt);
    vtss_rc (* xaui_counters_clear) (const vtss_port_no_t port_no);
#endif /* VTSS_FEATURE_XAUI */

#if defined(VTSS_FEATURE_OTN)
    vtss_rc (* otn_och_fec_set)(const vtss_port_no_t port_no, const vtss_otn_och_fec_t *const cfg);
    vtss_rc (* otn_och_loopback_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_otu_tti_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_otu_ais_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_otu_consequent_actions_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_otu_oh_insertion_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_otu_tx_res_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_otu_tx_smres_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_otu_tx_gcc0_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_mode_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_tti_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_ais_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_consequent_actions_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_oh_insertion_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_tx_res_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_tx_exp_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_tx_ftfl_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_tx_aps_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_pt_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_opu_oh_insertion_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odu_opu_test_insertion_set)(const vtss_port_no_t port_no);
    vtss_rc (* otn_odut_mode_set)(const vtss_port_no_t port_no, const u32 level);
    vtss_rc (* otn_odut_tti_set)(const vtss_port_no_t port_no, const u32 level);
    vtss_rc (* otn_odut_consequent_actions_set)(const vtss_port_no_t port_no, const u32 level);
    vtss_rc (* otn_odut_bdi_set)(const vtss_port_no_t port_no, const u32 level);
    vtss_rc (* otn_odut_tx_stat_set)(const vtss_port_no_t port_no, const u32 level);
    vtss_rc (* otn_odut_tx_aps_set)(const vtss_port_no_t port_no, const u32 level);

    vtss_rc (* otn_otu_acc_tti_get)(const vtss_port_no_t port_no, vtss_otn_otu_acc_tti_t *const tti);
    vtss_rc (* otn_otu_acc_res_get)(const vtss_port_no_t port_no, vtss_otn_otu_acc_res_t *const res);
    vtss_rc (* otn_otu_acc_smres_get)(const vtss_port_no_t port_no, vtss_otn_otu_acc_smres_t *const smres);
    vtss_rc (* otn_otu_acc_gcc0_get)(const vtss_port_no_t port_no,vtss_otn_otu_acc_gcc0_t *const gcc0);
    vtss_rc (* otn_odu_acc_tti_get)(const vtss_port_no_t port_no, vtss_otn_odu_acc_tti_t *const tti);
    vtss_rc (* otn_odu_acc_res_get)(const vtss_port_no_t port_no, vtss_otn_odu_acc_res_t *const res);
    vtss_rc (* otn_odu_acc_exp_get)(const vtss_port_no_t port_no, vtss_otn_odu_acc_exp_t *const exp);
    vtss_rc (* otn_odu_acc_ftfl_get)(const vtss_port_no_t port_no, vtss_otn_odu_acc_ftfl_t *const ftfl);
    vtss_rc (* otn_odu_acc_aps_get)(const vtss_port_no_t port_no, vtss_otn_odu_acc_aps_t *const aps);
    vtss_rc (* otn_odu_acc_pt_get)(const vtss_port_no_t port_no, vtss_otn_odu_acc_pt_t *const pt);
    vtss_rc (* otn_odut_acc_tti_get)(const vtss_port_no_t port_no, const u32 level, vtss_otn_odut_acc_tti_t *const tti);
    vtss_rc (* otn_odut_acc_stat_get)(const vtss_port_no_t port_no, const u32 level, vtss_otn_odut_acc_stat_t *const stat);
    vtss_rc (* otn_odut_acc_aps_get)(const vtss_port_no_t port_no, const u32 level, vtss_otn_odut_acc_aps_t *const aps);

    vtss_rc (* otn_och_defects_get)(const vtss_port_no_t port_no, vtss_otn_och_defects_t *const defects);
    vtss_rc (* otn_otu_defects_get)(const vtss_port_no_t port_no, vtss_otn_otu_defects_t *const defects);
    vtss_rc (* otn_odu_defects_get)(const vtss_port_no_t port_no, vtss_otn_odu_defects_t *const defects);
    vtss_rc (* otn_odut_defects_get)(const vtss_port_no_t port_no, const u32 level, vtss_otn_odut_defects_t *const defects);

    vtss_rc (* otn_och_perf_get)(const vtss_port_no_t port_no, vtss_otn_och_perf_t *const perf);
    vtss_rc (* otn_otu_perf_get)(const vtss_port_no_t port_no, vtss_otn_otu_perf_t *const perf);
    vtss_rc (* otn_odu_perf_get)(const vtss_port_no_t port_no, vtss_otn_odu_perf_t *const perf);
    vtss_rc (* otn_odut_perf_get)(const vtss_port_no_t port_no, const u32 level, vtss_otn_odut_perf_t *const perf);

    vtss_rc (* otn_och_event_enable)(const vtss_port_no_t port_no, const BOOL enable, const vtss_otn_och_event_t ev_mask);
    vtss_rc (* otn_och_event_poll)(const vtss_port_no_t port_no, vtss_otn_och_event_t *const status);
    vtss_rc (* otn_och_event_poll_without_mask)(const vtss_port_no_t port_no, vtss_otn_och_event_t *const status);
    vtss_rc (* otn_otu_event_enable)(const vtss_port_no_t port_no, const BOOL enable, const vtss_otn_otu_event_t ev_mask);
    vtss_rc (* otn_otu_event_poll)(const vtss_port_no_t port_no, vtss_otn_otu_event_t *const status);
    vtss_rc (* otn_otu_event_poll_without_mask)(const vtss_port_no_t port_no, vtss_otn_otu_event_t *const status);
    vtss_rc (* otn_odu_event_enable)(const vtss_port_no_t port_no, const BOOL enable, const vtss_otn_odu_event_t ev_mask);
    vtss_rc (* otn_odu_event_poll)(const vtss_port_no_t port_no, vtss_otn_odu_event_t *const status);
    vtss_rc (* otn_odu_event_poll_without_mask)(const vtss_port_no_t port_no, vtss_otn_odu_event_t *const status);
    vtss_rc (* otn_odut_event_enable)(const vtss_port_no_t port_no, const u32 level, const BOOL enable, const vtss_otn_odut_event_t ev_mask);
    vtss_rc (* otn_odut_event_poll)(const vtss_port_no_t port_no, const u32 level, vtss_otn_odut_event_t *const status);
    vtss_rc (* otn_odut_event_poll_without_mask)(const vtss_port_no_t port_no, const u32 level, vtss_otn_odut_event_t *const status);
#endif /* VTSS_FEATURE_OTN */

#if defined(VTSS_FEATURE_GFP)
    vtss_rc (* gfp_single_err_corr_set)(const vtss_port_no_t  port_no);
    vtss_rc (* gfp_cmf_auto_sf_gen_set)(const vtss_port_no_t  port_no);
    vtss_rc (* gfp_cmf_forced_gen_set)(const vtss_port_no_t  port_no);
    vtss_rc (* gfp_cdf_fcs_insert_set)(const vtss_port_no_t  port_no);
    vtss_rc (* gfp_cdf_upi_set)(const vtss_port_no_t  port_no);
    vtss_rc (* gfp_8b10b_control_code_set)(const vtss_port_no_t  port_no);
    vtss_rc (* gfp_cmf_acc_upi_get)(const vtss_port_no_t  port_no, u32 *const upi);
    vtss_rc (* gfp_cdf_acc_upi_get)(const vtss_port_no_t  port_no, u32 *const upi);
    vtss_rc (* gfp_defects_get)(const vtss_port_no_t  port_no, vtss_gfp_defects_t *const defects);
    vtss_rc (* gfp_perf_get)(const vtss_port_no_t  port_no, vtss_gfp_perf_t *const perf);
    vtss_rc (* gfp_event_enable)(const vtss_port_no_t  port_no, const BOOL enable, const vtss_gfp_event_t ev_mask);
    vtss_rc (* gfp_event_poll)(const vtss_port_no_t  port_no, vtss_gfp_event_t *const status);
    vtss_rc (* gfp_event_poll_without_mask)(const vtss_port_no_t  port_no, vtss_gfp_event_t *const status);
#endif /* VTSS_FEATURE_GFP */

#if defined(VTSS_FEATURE_RAB)
    vtss_rc (* rab_config_set)(const vtss_port_no_t  port_no);
    vtss_rc (* rab_counters_get)(const vtss_port_no_t  port_no, vtss_rab_counters_t *const counters);
#endif /* VTSS_FEATURE_RAB */

#if defined(VTSS_FEATURE_OHA)
    vtss_rc (* oha_config_set)(const vtss_port_no_t  port_no);    
#endif /* VTSS_FEATURE_OHA */

#if defined(VTSS_FEATURE_AE)
    vtss_rc (* ae_set_config)(const vtss_ae_lane_t  lane_no);    
    vtss_rc (* ae_set_coeff_updt_req)(const vtss_ae_lane_t  lane_no, const u32 req);     
    vtss_rc (* ae_get_coeff_stat_req)(const vtss_ae_lane_t  lane_no,u32  *const stat ); 
    vtss_rc (* ae_get_eye_data)(const vtss_ae_lane_t  lane_no,const vtss_ae_eye_data_t  cmd,vtss_ae_eye_data_val_t *const retval); 
    vtss_rc (* ae_init_rx)(const vtss_ae_lane_t  lane_no); 
    vtss_rc (* ae_init_tx)(const vtss_ae_lane_t  lane_no); 
    vtss_rc (* ae_return_rx_cfg)(const vtss_ae_lane_t  lane_no);     
#endif /* VTSS_FEATURE_AE */

#if defined(VTSS_FEATURE_MAC10G)
    vtss_rc (* mac10g_config_set)(const vtss_port_no_t  port_no);
    vtss_rc (* mac10g_status_get)(const vtss_port_no_t  port_no, vtss_mac10g_status_t *const status);
    vtss_rc (* mac10g_counters_update)(const vtss_port_no_t  port_no);
    vtss_rc (* mac10g_counters_clear)(const vtss_port_no_t  port_no);
    vtss_rc (* mac10g_counters_get)(const vtss_port_no_t  port_no, vtss_port_counters_t *const cnt);
    vtss_rc (* mac10g_event_enable)(vtss_port_no_t port_no, const BOOL enable, const vtss_mac10g_event_t ev_mask);
    vtss_rc (* mac10g_event_poll)(vtss_port_no_t port_no,vtss_mac10g_event_t *const status );
    vtss_rc (* mac10g_event_poll_without_mask)(vtss_port_no_t port_no,vtss_mac10g_event_t *const status );
#endif /* VTSS_FEATURE_MAC10G */

#if defined(VTSS_FEATURE_PCS_10GBASE_R)
    vtss_rc (* pcs_10gbase_r_config_set)(const vtss_port_no_t  port_no);
    vtss_rc (* pcs_10gbase_r_status_get)(const vtss_port_no_t  port_no, vtss_pcs_10gbase_r_status_t *const status);
    vtss_rc (* pcs_10gbase_r_counters_update)(const vtss_port_no_t  port_no);
    vtss_rc (* pcs_10gbase_r_counters_clear)(const vtss_port_no_t  port_no);
    vtss_rc (* pcs_10gbase_r_counters_get)(const vtss_port_no_t  port_no, vtss_pcs_10gbase_r_cnt_t *const counters);
    vtss_rc (* pcs_10gbase_r_event_enable)(vtss_port_no_t port_no, const BOOL enable, const vtss_pcs_10gbase_r_event_t ev_mask);
    vtss_rc (* pcs_10gbase_r_event_poll)(vtss_port_no_t port_no,vtss_pcs_10gbase_r_event_t *const status );
    vtss_rc (* pcs_10gbase_r_event_poll_without_mask)(vtss_port_no_t port_no,vtss_pcs_10gbase_r_event_t *const status );
#endif /* VTSS_FEATURE_PCS_10GBASE_R */

#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
    vtss_rc (* vxlat_conf_set)(void);
#endif /* VTSS_FEATURE_VLAN_TRANSLATION */

#if defined(VTSS_FEATURE_TIMESTAMP)
    vtss_rc (* ts_timeofday_get)(vtss_timestamp_t               *ts,
                                 u32                            *tc);
    vtss_rc (* ts_timeofday_next_pps_get)(vtss_timestamp_t      *ts);
    vtss_rc (* ts_timeofday_set)(const vtss_timestamp_t         *ts);
    vtss_rc (* ts_timeofday_set_delta)(const vtss_timestamp_t   *ts,
                                      BOOL                 negative);
    vtss_rc (* ts_timeofday_offset_set)(i32                     offset);
    u32     (*ts_ns_cnt_get) (vtss_inst_t inst);
    vtss_rc (* ts_timeofday_one_sec)(void);
    vtss_rc (* ts_adjtimer_set)(void);
    vtss_rc (* ts_freq_offset_get)(i32                  *adj);
#if defined (VTSS_ARCH_SERVAL)
    vtss_rc (* ts_external_clock_mode_set)(int                  idx);
    vtss_rc (* ts_alt_clock_saved_get)(u32                      *saved);
    vtss_rc (* ts_alt_clock_mode_set)(void);
    vtss_rc (* ts_timeofday_next_pps_set)(const vtss_timestamp_t *ts);
#else
    vtss_rc (* ts_external_clock_mode_set)(void);
#endif
    vtss_rc (* ts_ingress_latency_set)(vtss_port_no_t    port_no);
    vtss_rc (* ts_p2p_delay_set)(vtss_port_no_t          port_no);
    vtss_rc (* ts_egress_latency_set)(vtss_port_no_t     port_no);
    vtss_rc (* ts_delay_asymmetry_set)(vtss_port_no_t     port_no);
    vtss_rc (* ts_operation_mode_set)(vtss_port_no_t     port_no);
    vtss_rc (* ts_internal_mode_set) (void);
    vtss_rc (* ts_timestamp_get)(void);
    vtss_rc (* ts_timestamp_convert)(vtss_port_no_t      port_no,
                                     u32                 *ts);
    vtss_rc (* ts_timestamp_id_release)(u32              ts_id);
#endif /* VTSS_FEATURE_TIMESTAMP */

#if defined(VTSS_FEATURE_SYNCE)
    vtss_rc (* synce_clock_out_set)(const u32   clk_port);
    vtss_rc (* synce_clock_in_set)(const u32   clk_port);
#endif /* VTSS_FEATURE_SYNCE */

#if defined(VTSS_FEATURE_WIS)
    vtss_rc (* ewis_events_conf_set)(vtss_port_no_t port_no, const BOOL enable, const vtss_ewis_event_t ev_mask);
    vtss_rc (* ewis_events_poll)(vtss_port_no_t port_no,vtss_ewis_event_t *const status );
    vtss_rc (* ewis_events_poll_without_mask)(vtss_port_no_t port_no,vtss_ewis_event_t *const status );
    vtss_rc (* ewis_events_force)(vtss_port_no_t port_no, const BOOL enable, const vtss_ewis_event_t events);
    vtss_rc (* ewis_static_conf_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_force_conf_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_tx_oh_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_tx_oh_passthru_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_exp_sl_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_mode_conf_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_reset)(vtss_port_no_t port_no);
    vtss_rc (* ewis_cons_action_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_section_txti_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_path_txti_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_test_mode_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_prbs31_err_inj_set)(vtss_port_no_t port_no, const vtss_ewis_prbs31_err_inj_t *const inj);
    vtss_rc (* ewis_test_status_get)(vtss_port_no_t port_no, vtss_ewis_test_status_t *const test_status);
    vtss_rc (* ewis_defects_get)(vtss_port_no_t port_no, vtss_ewis_defects_t *const def);
    vtss_rc (* ewis_status_get)(vtss_port_no_t port_no, vtss_ewis_status_t *const status);
    vtss_rc (* ewis_section_acti_get)(vtss_port_no_t port_no, vtss_ewis_tti_t *const acti);
    vtss_rc (* ewis_path_acti_get)(vtss_port_no_t port_no, vtss_ewis_tti_t *const actii);
    vtss_rc (* ewis_perf_get)(vtss_port_no_t port_no, vtss_ewis_perf_t *const perf);
    vtss_rc (* ewis_counter_get)(vtss_port_no_t port_no, vtss_ewis_counter_t *const perf);
    vtss_rc (* ewis_counter_threshold_set)(vtss_port_no_t port_no);
    vtss_rc (* ewis_perf_mode_set)(vtss_port_no_t port_no);
#endif /* VTSS_FEATURE_WIS */

#if defined(VTSS_FEATURE_I2C)
    vtss_rc (* i2c_set_config)(const vtss_i2c_controller_t *const controller_cfg,
                               const vtss_i2c_cfg_t        *const cfg);
    vtss_rc (* i2c_get_config)(const vtss_i2c_controller_t *const controller_cfg,
                                     vtss_i2c_cfg_t        *const cfg);
    vtss_rc (* i2c_get_status)(const vtss_i2c_controller_t *const controller_cfg,
                               u16                   *const status);
    vtss_rc (* i2c_rx)(const vtss_i2c_controller_t *const controller_cfg,
                       u8                    *const value);
    vtss_rc (* i2c_tx)(const vtss_i2c_controller_t *const controller_cfg,
                       const u8                    *const addr_data);
    vtss_rc (* i2c_cmd)(const vtss_i2c_controller_t *const controller_cfg,
                        const u16                   cmd_flags);
    vtss_rc (* i2c_stop)(const vtss_i2c_controller_t *const controller_cfg);
#endif /* VTSS_FEATURE_I2C */
#if defined(VTSS_FEATURE_OAM)
    vtss_rc (* oam_vop_cfg_set)(void);

    vtss_rc (* oam_event_poll)(vtss_oam_event_mask_t *const mask);
    vtss_rc (* oam_voe_event_enable)(const vtss_oam_voe_idx_t, const vtss_oam_voe_event_mask_t mask, const BOOL enable);
    vtss_rc (* oam_voe_event_poll)(const vtss_oam_voe_idx_t, vtss_oam_voe_event_mask_t *const mask);

    vtss_rc (* oam_voe_alloc)(const vtss_oam_voe_type_t type, const vtss_oam_voe_alloc_cfg_t *data, vtss_oam_voe_idx_t *voe_idx);
    vtss_rc (* oam_voe_free)(const vtss_oam_voe_idx_t);

    vtss_rc (* oam_voe_cfg_set)(const vtss_oam_voe_idx_t);

    vtss_rc (* oam_voe_ccm_arm_hitme)(const vtss_oam_voe_idx_t, const BOOL enable);
    vtss_rc (* oam_voe_ccm_set_rdi_flag)(const vtss_oam_voe_idx_t, const BOOL flag);

    vtss_rc (* oam_ccm_status_get)(const vtss_oam_voe_idx_t, vtss_oam_ccm_status_t *value);

    vtss_rc (* oam_pdu_seen_status_get)(const vtss_oam_voe_idx_t, vtss_oam_pdu_seen_status_t *value);
    vtss_rc (* oam_proc_status_get)(const vtss_oam_voe_idx_t, vtss_oam_proc_status_t *value);

    vtss_rc (* oam_voe_counter_update)(const vtss_oam_voe_idx_t);
    vtss_rc (* oam_voe_counter_get)(const vtss_oam_voe_idx_t, vtss_oam_voe_counter_t *value);
    vtss_rc (* oam_voe_counter_clear)(const vtss_oam_voe_idx_t, const u32 mask);
#if defined(VTSS_ARCH_SERVAL)
    vtss_rc (* oam_voe_counter_update_serval)(const vtss_oam_voe_idx_t);
#endif

    vtss_rc (* oam_ipt_conf_get)(u32 isdx, vtss_oam_ipt_conf_t *cfg);
    vtss_rc (* oam_ipt_conf_set)(u32 isdx, const vtss_oam_ipt_conf_t *const cfg);
#endif /* VTSS_FEATURE_OAM */

#if defined(VTSS_FEATURE_MPLS)
    vtss_rc (* mpls_tc_conf_set)(const vtss_mpls_tc_conf_t * const map);

    vtss_rc (* mpls_l2_alloc)(vtss_mpls_l2_idx_t * const idx);
    vtss_rc (* mpls_l2_free)(const vtss_mpls_l2_idx_t idx);
    vtss_rc (* mpls_l2_set)(const vtss_mpls_l2_idx_t idx, const vtss_mpls_l2_t * const l2);
    vtss_rc (* mpls_l2_segment_attach)(const vtss_mpls_l2_idx_t idx, const vtss_mpls_segment_idx_t seg_idx);
    vtss_rc (* mpls_l2_segment_detach)(const vtss_mpls_segment_idx_t seg_idx);

    vtss_rc (* mpls_xc_alloc)(const vtss_mpls_xc_type_t type, vtss_mpls_xc_idx_t * const idx);
    vtss_rc (* mpls_xc_free)(const vtss_mpls_xc_idx_t idx);
    vtss_rc (* mpls_xc_set)(const vtss_mpls_xc_idx_t idx, const vtss_mpls_xc_t * const xc);

    vtss_rc (* mpls_xc_segment_attach)(const vtss_mpls_xc_idx_t xc_idx, const vtss_mpls_segment_idx_t seg_idx);
    vtss_rc (* mpls_xc_segment_detach)(const vtss_mpls_segment_idx_t seg_idx);

    vtss_rc (* mpls_xc_mc_segment_attach)(const vtss_mpls_xc_idx_t xc_idx, const vtss_mpls_segment_idx_t seg_idx);
    vtss_rc (* mpls_xc_mc_segment_detach)(const vtss_mpls_xc_idx_t xc_idx, const vtss_mpls_segment_idx_t seg_idx);

    vtss_rc (* mpls_segment_alloc)(const BOOL in_seg, vtss_mpls_segment_idx_t * const idx);
    vtss_rc (* mpls_segment_free)(const vtss_mpls_segment_idx_t idx);
    vtss_rc (* mpls_segment_set)(const vtss_mpls_segment_idx_t idx, const vtss_mpls_segment_t * const seg);
    vtss_rc (* mpls_segment_state_get)(const vtss_mpls_segment_idx_t idx, vtss_mpls_segment_state_t * const state);
    vtss_rc (* mpls_segment_server_attach)(const vtss_mpls_segment_idx_t idx, const vtss_mpls_segment_idx_t srv_idx);
    vtss_rc (* mpls_segment_server_detach)(const vtss_mpls_segment_idx_t idx);
#endif /* VTSS_FEATURE_MPLS */

#if defined(VTSS_ARCH_JAGUAR_1)
    vtss_rc (* l3_rleg_counters_get)(const vtss_l3_rleg_id_t, vtss_l3_counters_t * const);
    vtss_rc (* l3_rleg_counters_reset)(void);
    vtss_rc (* l3_common_set)(const vtss_l3_common_conf_t * const);
    vtss_rc (* l3_rleg_set)(const vtss_l3_rleg_id_t, const vtss_l3_rleg_conf_t * const);
    vtss_rc (* l3_vlan_set)(const vtss_l3_rleg_id_t, const vtss_vid_t, const BOOL);
    vtss_rc (* l3_arp_set)(const u32, const vtss_mac_t * const, const vtss_vid_t);
    vtss_rc (* l3_arp_clear)(const u32);
    vtss_rc (* l3_ipv4_uc_set)(const u32, const u32, const BOOL, const u32, const u32);
    vtss_rc (* l3_ipv6_uc_set)(const u32, const u32, const BOOL, const u32, const u32, const u32, const u32, const u32, const u32, const u32, const u32);
    vtss_rc (* l3_ipv4_uc_clear)(const u32);
    vtss_rc (* l3_ipv4_mc_clear)(const u32);
    vtss_rc (* l3_ipv6_uc_clear)(const u32);
    vtss_rc (* l3_lpm_move)(const u32, const u32, const u32, const BOOL);
    vtss_rc (* l3_debug_sticky_clear)(void);
#endif /* VTSS_ARCH_JAGUAR_1*/

#ifdef VTSS_ARCH_DAYTONA
    vtss_rc (* interrupt_enable_set)(const vtss_interrupt_block_t  *const blocks);
    vtss_rc (* interrupt_enable_get)(vtss_interrupt_block_t  *const blocks);
    vtss_rc (* interrupt_status_get)(vtss_interrupt_block_t *const status);

    vtss_rc (* pmtick_enable_set)(const vtss_port_no_t port_no,
                                  const vtss_pmtick_control_t *const control);
    vtss_rc (* channel_mode_set)(u16 channel, vtss_config_mode_t conf_mode, u32 two_lane_upi);
#endif /* VTSS_ARCH_DAYTONA */

} vtss_cil_func_t;

#if defined(VTSS_FEATURE_TIMESTAMP)
typedef struct {
    i32 adj;
    i32 adj_divisor;
    vtss_ts_ext_clock_mode_t ext_clock_mode;
#if defined(VTSS_ARCH_SERVAL)
    vtss_ts_ext_clock_mode_t ext_clock_mode_alt;
    vtss_ts_alt_clock_mode_t alt_clock_mode;

#endif
    u32 sec_offset; /* current offset from HW timer */
    u32 outstanding_adjustment; /* set in timeofday_set, cleared in timeofday_onesec */
    i32 outstanding_corr;       /* value to be subtracted from hw time to get PTP time during the adjustment period */
    BOOL awaiting_adjustment; /* set when a clock onetime adjustment has been set, cleared after one sec */
    u32 delta_sec;              /* outstanding delta time */
    u16 delta_sec_msb;          /* outstanding delta time */
    u16 delta_sign;             /* 0 if no outstanding delta, 1 if neg, 2 if pos */
    BOOL sw_ts_id_res [TS_IDS_RESERVED_FOR_SW]; /* reservation flags for Luton 26 timestamps */
} vtss_ts_conf_t;

typedef struct {
    vtss_timeinterval_t ingress_latency;
    vtss_timeinterval_t p2p_delay;
    vtss_timeinterval_t egress_latency;
    vtss_timeinterval_t delay_asymmetry;
    vtss_ts_operation_mode_t mode;
} vtss_ts_port_conf_t;

/* Serval Luton 26 and Jaguar timestamp table structure */
typedef struct {
    u64 reserved_mask;                      /* port mask indicating which ports this tx idx is reserved for */
    u64 valid_mask;                         /* indication pr. port if there is a valid timestamp in the table  */
    u32 age;                                /* ageing counter */
    u32 tx_tc [VTSS_PORT_ARRAY_SIZE];       /* actual transmit time counter for the [idx][port] */
    u32 tx_id [VTSS_PORT_ARRAY_SIZE];       /* actual transmit time stamp id read from HW [idx][port] */
    u32 tx_sq [VTSS_PORT_ARRAY_SIZE];       /* actual transmit sequence number read from HW [idx][port] (Serval)*/
    u32 rx_tc;                              /* actual receive time counter for the [id] (Luton26) */
    BOOL rx_tc_valid;                       /* actual receive time counter is valid for the [id] (Luton26) */
    void * context [VTSS_PORT_ARRAY_SIZE];  /* context aligned to the  [idx,port] */
    void (*cb  [VTSS_PORT_ARRAY_SIZE]) (void *context, u32 port_no, vtss_ts_timestamp_t *ts); /* timestamp callback functions */
} vtss_ts_timestamp_status_t;

#if defined (VTSS_ARCH_SERVAL)
/* Serval OAM timestamp table structure
 * When an OAM timestamp is registered in HW, it is saved in this table
 * For each VOE instance there is place for up to VTSS_SERVAL_MAX_OAM_ENTRIES timestamps, the sequence number is used
 * to distinguish the timestamps,
 * A ringbuffer mechanism is used to fill entries into the buffer.
 * If the buffer runs over, the oldest timestamps are thrown away.
 *
 */
typedef struct {
    u32 tc ;      /* actual time counter  read from HW */
    u32 id;       /* actual time stamp id (VOE index) read from HW*/
    u32 sq;       /* actual sequence number corrsponding to the timestamp */
    u32 port;     /* actual port number corrsponding to the timestamp */
    BOOL valid;   /* true indicates that the timestamp is valid */
} vtss_oam_timestamp_entry_t;

#define VTSS_SERVAL_MAX_OAM_ENTRIES 5

typedef struct {
    vtss_oam_timestamp_entry_t entry [VTSS_SERVAL_MAX_OAM_ENTRIES];
    i32 last;
} vtss_oam_timestamp_status_t;
#endif /* VTSS_ARCH_SERVAL */

#endif /* VTSS_FEATURE_TIMESTAMP */

#if defined(VTSS_FEATURE_PHY_TIMESTAMP)
typedef struct {
    BOOL                            eng_used; /* allocated the engine to application */
    vtss_phy_ts_encap_t             encap_type; /* engine encapsulation */
    vtss_phy_ts_engine_flow_match_t flow_match_mode; /* strict/non-strict flow match */
    u8                              flow_st_index; /* start index of flow */
    u8                              flow_end_index; /* end index of flow */
    vtss_phy_ts_engine_flow_conf_t  flow_conf; /* engine flow config */
    vtss_phy_ts_engine_action_t     action_conf; /* engine action */
    u8                              action_flow_map[6]; /* action map to flow */
} vtss_phy_ts_eng_conf_t;

#if defined(VTSS_CHIP_CU_PHY) && defined(VTSS_PHY_TS_SPI_CLK_THRU_PPS0)
typedef struct {
    u16                      phy_type;
    u16                      channel_id;
    BOOL                     enable;
    BOOL                     alt_port_init_done;
    vtss_phy_ts_fifo_mode_t  alt_port_tx_fifo_mode;
} vtss_phy_ts_new_spi_conf_t;
#endif /* defined(VTSS_CHIP_CU_PHY) && defined(VTSS_PHY_TS_SPI_CLK_THRU_PPS0) */

typedef struct {
    BOOL                             port_ts_init_done; /* PHY TS init done */
    BOOL                             eng_init_done; /* 1588 TS engine init done */
    BOOL                             port_ena;
    vtss_port_no_t                   base_port; /* base port for multi-channel PHY */
    vtss_phy_ts_clockfreq_t          clk_freq;  /* reference clock frequency */
    vtss_phy_ts_clock_src_t          clk_src;   /* reference clock source */
    vtss_phy_ts_rxtimestamp_pos_t    rx_ts_pos; /* Rx timestamp position */
    vtss_phy_ts_rxtimestamp_len_t    rx_ts_len; /* Rx timestamp length */
    vtss_phy_ts_fifo_mode_t          tx_fifo_mode; /* Tx TSFIFO access mode */
    vtss_phy_ts_fifo_timestamp_len_t tx_ts_len; /* timestamp size in Tx TSFIFO */
    vtss_phy_ts_tc_op_mode_t         tc_op_mode; /* TC operating Mode: Mode A or B*/
    vtss_phy_ts_8487_xaui_sel_t      xaui_sel_8487; /* 8487 XAUI Lane selection */
    vtss_phy_ts_fifo_sig_mask_t      sig_mask;  /* FIFO signature */
    u32                              fifo_age;  /* SW TSFIFO age in milli-sec */
    vtss_timeinterval_t              ingress_latency;
    vtss_timeinterval_t              egress_latency;
    vtss_timeinterval_t              path_delay;
    vtss_timeinterval_t              delay_asym;
    vtss_phy_ts_scaled_ppb_t         rate_adj;  /* clock rate adjustment */
    vtss_phy_ts_eng_conf_t           ingress_eng_conf[4]; /*port ingress engine configuration including encapsulation, comparator configuration and action  */
    vtss_phy_ts_eng_conf_t           egress_eng_conf[4]; /*port egress engine configuration including encapsulation, comparator configuration and action  */
    vtss_phy_ts_event_t              event_mask; /* interrupt mask config */
    BOOL                             event_enable; /* interrupt enable/disable */
#if defined(VTSS_CHIP_CU_PHY) && defined(VTSS_PHY_TS_SPI_CLK_THRU_PPS0)
    vtss_phy_ts_new_spi_conf_t       new_spi_conf; /* Config info for New SPI Mode */
#endif /* defined(VTSS_CHIP_CU_PHY) && defined(VTSS_PHY_TS_SPI_CLK_THRU_PPS0) */
#if defined (VTSS_SW_OPTION_REMOTE_TS_PHY)
    BOOL                              remote_phy;    /**< TRUE if the phy is remote (e.g. Daytona validation board) */
#endif /* VTSS_SW_OPTION_REMOTE_TS_PHY */
} vtss_phy_ts_port_conf_t;
#endif  /* VTSS_FEATURE_PHY_TIMESTAMP */

#if defined(VTSS_FEATURE_SERIAL_GPIO)
typedef struct {
    BOOL     enable[VTSS_SGPIO_PORTS][4]; /* each port has four bits */
} vtss_sgpio_event_enable_t;
#endif /* VTSS_FEATURE_SERIAL_GPIO */

#if defined(VTSS_SW_OPTION_L3RT) && defined(VTSS_ARCH_JAGUAR_1)
typedef enum {
    VTSS_L3_LPM_MATCH_BOTH,
    VTSS_L3_LPM_MATCH_LPM,
    VTSS_L3_LPM_MATCH_SECURITY,
} vtss_l3_lpm_match_type;

typedef enum {
    VTSS_L3_LPM_ENTRY_IPV4_UC,
    VTSS_L3_LPM_ENTRY_IPV6_UC,
    VTSS_L3_LPM_ENTRY_IPV4_MC,
} vtss_l3_lpm_entry_type;

typedef struct {
    BOOL                   valid;  /* current entry is valid  (book keeping only - not in hardware) */
    vtss_l3_lpm_match_type match;  /* lpm/security/both */
    vtss_l3_lpm_entry_type  type;  /* ipv4_uc/ipv6_uc/ipv4_mc */
    u32                     data;  /* ip address to match */
    u32                     mask;  /* mask to match */
    u32                     dest;  /* destination of the route (book keeping only - not in hardware) */
    BOOL                    ecmp;  /* current entry represents ecmp */
    BOOL               ptr_valid;  /* pointer valid (active in hardware) */
    u32                      ptr;  /* pointer to ARP/ICMP entry */
} vtss_l3_lpm_entry_t;

typedef struct {
    vtss_l3_lpm_entry_t e[VTSS_LPM_CNT];
} vtss_l3_lpm_table_t;

typedef struct {
    BOOL        valid;
    vtss_ipv4_t ip;
    u32         ptr;
} vtss_l3_arp_ipv4_entry_t;

typedef struct {
    BOOL        valid;
    vtss_ipv6_t ip;
    u32         ptr;
} vtss_l3_ndp_ipv6_entry_t;

typedef struct {
    BOOL              valid;       /* current entry is valid */
    vtss_l3_rleg_id_t egress_rleg; /* router leg used for transmitting frames */
    BOOL              secure_vmid; /* ? */
    BOOL              secure_mac;  /* ? */
    BOOL              arp_enable;  /* arp enable */
    vtss_mac_t        dmac;        /* destination mac address */
} vtss_l3_arp_hw_entry_t;

typedef struct {
    vtss_l3_arp_hw_entry_t   hw[VTSS_ARP_CNT];
    vtss_l3_arp_ipv4_entry_t ipv4[VTSS_ARP_IPV4_RELATIONS];
    vtss_l3_ndp_ipv6_entry_t ipv6[VTSS_ARP_IPV6_RELATIONS];
} vtss_l3_neighbour_cache_t;

typedef struct {
    vtss_l3_counters_t interface_shadow_counter[VTSS_RLEG_CNT];
    vtss_l3_counters_t interface_counter[VTSS_RLEG_CNT];
    vtss_l3_counters_t system_counter;
} vtss_l3_statistics_t;
#endif /* VTSS_SW_OPTION_L3RT && VTSS_ARCH_JAGUAR_1*/


#if defined(VTSS_ARCH_JAGUAR_1)
typedef struct {
    BOOL                       initialized;
    vtss_l3_common_conf_t      common;
    vtss_l3_rleg_conf_t        rleg_conf[VTSS_RLEG_CNT];
#  if defined(VTSS_SW_OPTION_L3RT)
    vtss_l3_lpm_table_t        lpm;
    vtss_l3_neighbour_cache_t  neighbour_cache;
    vtss_l3_statistics_t       statistics;
#  endif /* VTSS_SW_OPTION_L3RT */
} vtss_l3_state_t;
#endif /* VTSS_ARCH_JAGUAR_1*/

#if defined (VTSS_FEATURE_MPLS)

// VProfile table

#define VTSS_MPLS_VPROFILE_CNT          64
#define VTSS_MPLS_VPROFILE_LSR_IDX      (VTSS_PORTS + 1)                    /* +1 due to CPU port */
#define VTSS_MPLS_VPROFILE_OAM_IDX      (VTSS_PORTS + 2)
#define VTSS_MPLS_VPROFILE_RESERVED_CNT (VTSS_MPLS_VPROFILE_OAM_IDX + 1)    /* Reserved ports start at index 0 */

typedef i8  vtss_mpls_vprofile_idx_t;                   /* Vprofile index */

typedef struct {
    vtss_port_no_t  port;
    BOOL            s1_s2_enable;
    BOOL            learn_enable;
    BOOL            recv_enable;
    BOOL            ptp_dly1_enable;
    BOOL            vlan_aware;
    BOOL            map_dscp_cos_enable;
    BOOL            map_eth_cos_enable;
    BOOL            src_mirror_enable;
} vtss_mpls_vprofile_t;

typedef struct {
    vtss_mpls_vprofile_t        pub;
    vtss_mpls_vprofile_idx_t    next_free;      /* 0 == no more free */
} vtss_mpls_vprofile_internal_t;

// In- and Out-segment encapsulation

#define VTSS_MPLS_IN_ENCAP_CNT          128     /* Number of HW entries */
#define VTSS_MPLS_OUT_ENCAP_CNT         1024    /* Number of HW entries */
#define VTSS_MPLS_IN_ENCAP_LABEL_CNT    3       /* Number of HW labels supported at ingress */
#define VTSS_MPLS_OUT_ENCAP_LABEL_CNT   3       /* Number of HW labels supported at egress */

#define VTSS_MPLS_L2_CNT                128     /* Number of L2 LER/LSR peer/VLAN combos */

typedef struct {
    vtss_mpls_l2_t          pub;
    // Internal data:
    i16                     ll_idx_upstream;    /* VTSS_MPLS_IDX_UNDEFINED == no IS0 MLL entry allocated */
    i16                     ll_idx_downstream;  /* VTSS_MPLS_IDX_UNDEFINED == no IS0 MLL entry allocated */
    vtss_mpls_l2_idx_t      next_free;          /* VTSS_MPLS_IDX_UNDEFINED == no more free in list */
} vtss_mpls_l2_internal_t;

// MPLS segments and cross-connects

#define VTSS_MPLS_LSP_CNT               512     /* Uni-directional LSPs */
#define VTSS_MPLS_BIDIR_PW_CNT          (VTSS_MPLS_VPROFILE_CNT - VTSS_MPLS_VPROFILE_RESERVED_CNT)     /* Bi-directional PWs */
#define VTSS_MPLS_SEGMENT_CNT           (2*VTSS_MPLS_LSP_CNT + 2*2*VTSS_MPLS_BIDIR_PW_CNT)
#define VTSS_MPLS_XC_CNT                (VTSS_MPLS_LSP_CNT + 2*VTSS_MPLS_BIDIR_PW_CNT)

typedef struct {
    vtss_mpls_segment_t     pub;                /* Public API struct */
    // Internal data:
    union {
        struct {
            BOOL                     has_mll;
            BOOL                     has_mlbs;     /* TRUE == IS0 MLBS entry allocated */
            vtss_mpls_vprofile_idx_t vprofile_idx; /* VProfile idx for LER (only) */
        } in;
        struct {
            BOOL                    has_encap;
            BOOL                    has_es0;
            vtss_sdx_entry_t        *esdx;
            i16                     encap_idx;
            u16                     encap_bytes;  /* Length of encap in bytes */
        } out;
    } u;
    BOOL                    need_hw_alloc;      /* TRUE if segment needs to try and allocate HW */
    vtss_mpls_segment_idx_t next_free;          /* VTSS_MPLS_IDX_UNDEFINED == no more free in list */
} vtss_mpls_segment_internal_t;

typedef struct {
    vtss_mpls_xc_t          pub;
    // Internal data:
    vtss_sdx_entry_t        *isdx;              /* Service index */
    vtss_mpls_xc_idx_t      next_free;          /* VTSS_MPLS_IDX_UNDEFINED == no more free in list */
} vtss_mpls_xc_internal_t;

typedef struct {
    BOOL    b_domain;                   /* TRUE == B-domain port */
    u16     l2_refcnt;                  /* Count of number of MPLS L2 entries using this port */
} vtss_mpls_port_state_t;

/* We estimate the following number of index chain entries is needed:
 *   * each MPLS segment may be part of a multicast chain
 *   * each MPLS segment may point to a L2 entry
 *   * (almost) each MPLS segment may use a server segment
 *   * one per IS0 MLL key (free-list)
 *   * one per egress MPLS encapsulation (free-list)
 *   + 512 spare entries
 */
#define VTSS_MPLS_IDXCHAIN_ENTRY_CNT    (VTSS_MPLS_SEGMENT_CNT + \
                                         VTSS_MPLS_SEGMENT_CNT + \
                                         VTSS_MPLS_SEGMENT_CNT + \
                                         VTSS_MPLS_IN_ENCAP_CNT + \
                                         VTSS_MPLS_OUT_ENCAP_CNT + \
                                         512)

#define VTSS_MPLS_IDX_CHECK(type, idx, low, cnt) \
    do { \
        if ((idx) < (low)  ||  (idx) >= (cnt)) { \
            VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Invalid " type " index: %d", (idx)); \
            return VTSS_RC_ERROR; \
        } \
    } while (0)

#define VTSS_MPLS_IDX_CHECK_L2(idx)        VTSS_MPLS_IDX_CHECK("L2",       idx, 0, VTSS_MPLS_L2_CNT)
#define VTSS_MPLS_IDX_CHECK_XC(idx)        VTSS_MPLS_IDX_CHECK("XC",       idx, 0, VTSS_MPLS_XC_CNT)
#define VTSS_MPLS_IDX_CHECK_SEGMENT(idx)   VTSS_MPLS_IDX_CHECK("Segment",  idx, 0, VTSS_MPLS_SEGMENT_CNT)
#define VTSS_MPLS_IDX_CHECK_VPROFILE(idx)  VTSS_MPLS_IDX_CHECK("VProfile", idx, VTSS_MPLS_VPROFILE_RESERVED_CNT, VTSS_MPLS_VPROFILE_CNT)


#endif /* VTSS_FEATURE_MPLS */

// Register access checking state.
typedef struct {
    struct {
        u32           addr;            // Address accessed.
        BOOL          is_read;         // Was it a read (TRUE) or a write (FALSE).
    } last_accesses[32];               // Info about last - up to - 32 accesses since an error occurred.
    u32               last_access_idx; // Index into reg_check_last_accesses[] to write next address.
    u32               last_error_cnt;  // The previous value of the error register.
    u32               addr;            // Address to check for access errors. Initialized by cil-part, and 0 if not supported.
    u32               disable_cnt;     // Ref. counted. When 0, register checking is *enabled*.
    vtss_reg_read_t   orig_read;       // When register checking is enabled, the normal init_conf.reg_read/write()
    vtss_reg_write_t  orig_write;      // functions get replaced by special ones, and vtss_state.reg_check_orig_read/write()
} vtss_debug_reg_check_t;

/* ================================================================= *
 *  Total API state
 * ================================================================= */

/* State cookie */
#define VTSS_STATE_COOKIE 0x53727910

/* State structure */
typedef struct vtss_state_s {
    u32                           cookie;
    vtss_arch_t                   arch;            /* Architecture */
    vtss_inst_create_t            create;
    vtss_init_conf_t              init_conf;
    BOOL                          restart_updated; /* Restart has been detected */
    BOOL                          warm_start_cur;  /* Current warm start status */
    BOOL                          warm_start_prev; /* Previous warm start status */
#if defined(VTSS_FEATURE_WARM_START)
    vtss_restart_t                restart_cur;     /* Current restart configuration */
    vtss_restart_t                restart_prev;    /* Previous restart configuration */
    vtss_version_t                version_cur;     /* Current version */
    vtss_version_t                version_prev;    /* Previous version */
#endif /* VTSS_FEATURE_WARM_START */

    u32                           chip_count;      /* Number of devices */
    vtss_chip_no_t                chip_no;         /* Currently selected device */
    u32                           gpio_count;
#if defined(VTSS_FEATURE_SERIAL_GPIO)
    u32                           sgpio_group_count;
    vtss_sgpio_conf_t             sgpio_conf[VTSS_CHIP_CNT][VTSS_SGPIO_GROUPS];
    vtss_sgpio_event_enable_t     sgpio_event_enable[VTSS_SGPIO_GROUPS];
#endif /* VTSS_FEATURE_SERIAL_GPIO */
    u32                           port_count;
#if defined(VTSS_CHIP_CU_PHY)
    vtss_phy_port_state_t         phy_state[VTSS_PORT_ARRAY_SIZE];
#endif
#if defined(VTSS_CHIP_10G_PHY)
    vtss_port_no_t                phy_10g_api_base_no; /* Support variable for determining base API port no */
    u32                           phy_channel_id;  /* Support counter for determining channel id */
    BOOL                          phy_88_event_B[2];
    vtss_phy_10g_port_state_t     phy_10g_state[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_CHIP_10G_PHY */
#if defined(VTSS_FEATURE_PORT_CONTROL)
    vtss_chip_id_t                chip_id;
    vtss_port_map_t               port_map[VTSS_PORT_ARRAY_SIZE];
    vtss_port_conf_t              port_conf[VTSS_PORT_ARRAY_SIZE];
    vtss_serdes_mode_t            serdes_mode[VTSS_PORT_ARRAY_SIZE];
    vtss_port_clause_37_control_t port_clause_37[VTSS_PORT_ARRAY_SIZE];
    vtss_port_chip_counters_t     port_counters[VTSS_PORT_ARRAY_SIZE];
    vtss_port_chip_counters_t     cpu_counters;
#if defined(VTSS_ARCH_SERVAL)
    vtss_port_ifh_t               port_ifh_conf[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_ARCH_SERVAL */
#if VTSS_OPT_INT_AGGR
    vtss_port_chip_counters_t     port_int_counters[2]; /* Internal port counters */
#endif /* VTSS_OPT_INT_AGGR */
#if defined(VTSS_ARCH_LUTON28)
    u32                           port_tx_packets[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_ARCH_LUTON28 */
    vtss_port_forward_t           port_forward[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_PORT_CONTROL */
#if defined(VTSS_ARCH_JAGUAR_1)
    u32                           port_int_0;              /* Internal chip port 0 */
    u32                           port_int_1;              /* Internal chip port 1 */
    u32                           mask_int_ports;          /* Internal chip port mask */
    vtss_port_mux_mode_t          mux_mode[VTSS_CHIP_CNT]; /* Port mux modes */
#endif /* VTSS_ARCH_JAGUAR_1 */
    vtss_cil_func_t               cil_func;

#if defined(VTSS_ARCH_B2)
    vtss_port_filter_t            port_filter[VTSS_PORT_ARRAY_SIZE];   /* Filter setup */
    int                           dep_port[VTSS_INT_PORT_COUNT];     /* Departure ports */
    vtss_lport_no_t               lport_map[VTSS_PORT_ARRAY_SIZE];
    vtss_spi4_conf_t              spi4;       /* SPI-4.2 host setup */
    vtss_xaui_conf_t              xaui[2];    /* XAUI host setup */
    vtss_qos_lport_conf_t         qos_lport[VTSS_LPORTS];
    vtss_dscp_entry_t             dscp_table[VTSS_DSCP_TABLE_COUNT][VTSS_DSCP_TABLE_SIZE];
#endif /* VTSS_ARCH_B2 */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    vtss_xaui_conf_t              xaui[4];    /* XAUI Line / Host setup */
    vtss_qos_lport_conf_t         qos_lport[VTSS_LPORTS]; /* Lport QoS configuration */
    vtss_lport_chip_counters_t    lport_chip_counters[VTSS_LPORTS]; /* Lport chip counters */

#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

#if defined(VTSS_FEATURE_QOS)
    BOOL                          wfq;
    vtss_prio_t                   prio_count;
    vtss_qos_conf_t               qos_conf;
    vtss_qos_port_conf_t          qos_port_conf_old;
    vtss_qos_port_conf_t          qos_port_conf[VTSS_PORT_ARRAY_SIZE];
#if defined(VTSS_FEATURE_QCL)
    vtss_qcl_t                    qcl[VTSS_QCL_ARRAY_SIZE];
#endif /* VTSS_FEATURE_QCL */
#if defined(VTSS_ARCH_LUTON26)
    vtss_policer_user_t           policer_user[VTSS_L26_POLICER_CNT];
#endif /* VTSS_ARCH_LUTON26 */
#endif /* VTSS_FEATURE_QOS */

#if defined(VTSS_FEATURE_AFI_SWC)
    vtss_afi_slot_conf_t          afi_slots[VTSS_AFI_SLOT_CNT];
    vtss_afi_timer_conf_t         afi_timers[VTSS_AFI_TIMER_CNT];
#endif /* VTSS_FEATURE_AFI_SWC */

#if defined(VTSS_FEATURE_PACKET)
    u32                           rx_queue_count;
    vtss_packet_rx_conf_t         rx_conf;
    vtss_packet_rx_t              rx_packet[VTSS_PACKET_RX_QUEUE_CNT];
#endif /* VTSS_FEATURE_PACKET */
#if defined(VTSS_FEATURE_PACKET_PORT_REG)
    vtss_packet_rx_port_conf_t    rx_port_conf[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_PACKET_PORT_REG */

#if defined(VTSS_FEATURE_NPI)
    vtss_npi_conf_t               npi_conf;
#endif /* VTSS_FEATURE_NPI */

#if defined(VTSS_FEATURE_LAYER2)
    /* Aggregated forwarding information */
    BOOL                          rx_forward[VTSS_PORT_ARRAY_SIZE];
    BOOL                          tx_forward[VTSS_PORT_ARRAY_SIZE];

    BOOL                          port_state[VTSS_PORT_ARRAY_SIZE];
    vtss_aggr_no_t                port_aggr_no[VTSS_PORT_ARRAY_SIZE];
    vtss_auth_state_t             auth_state[VTSS_PORT_ARRAY_SIZE];
    vtss_stp_state_t              stp_state[VTSS_PORT_ARRAY_SIZE];
#if defined(VTSS_FEATURE_PVLAN)
    vtss_pvlan_entry_t            pvlan_table[VTSS_PVLAN_ARRAY_SIZE];
    BOOL                          apvlan_table[VTSS_PORT_ARRAY_SIZE][VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_PVLAN */
    vtss_dgroup_port_conf_t       dgroup_port_conf[VTSS_PORT_ARRAY_SIZE];
    vtss_mirror_conf_t            mirror_conf;

    BOOL                          mirror_ingress[VTSS_PORT_ARRAY_SIZE];
    BOOL                          mirror_egress[VTSS_PORT_ARRAY_SIZE];
    BOOL                          mirror_cpu_ingress;
    BOOL                          mirror_cpu_egress;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    vtss_vlan_conf_t              vlan_conf;
    vtss_vlan_port_type_t         vlan_port_type[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */
    vtss_vlan_port_conf_t         vlan_port_conf[VTSS_PORT_ARRAY_SIZE];
#if defined(VTSS_FEATURE_VCL)
    vtss_vcl_port_conf_t          vcl_port_conf[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_VCL */
    vtss_port_eps_t               port_protect[VTSS_PORT_ARRAY_SIZE];
    vtss_vlan_entry_t             vlan_table[VTSS_VIDS];
    vtss_mstp_entry_t             mstp_table[VTSS_MSTI_ARRAY_SIZE];
    vtss_erps_entry_t             erps_table[VTSS_ERPI_ARRAY_SIZE];
    vtss_learn_mode_t             learn_mode[VTSS_PORT_ARRAY_SIZE];
    BOOL                          isolated_port[VTSS_PORT_ARRAY_SIZE];
    BOOL                          uc_flood[VTSS_PORT_ARRAY_SIZE];
    BOOL                          mc_flood[VTSS_PORT_ARRAY_SIZE];
    BOOL                          ipv4_mc_flood[VTSS_PORT_ARRAY_SIZE];
    BOOL                          ipv6_mc_flood[VTSS_PORT_ARRAY_SIZE];
    BOOL                          ipv6_mc_scope;
    vtss_mac_table_age_time_t     mac_age_time;
    vtss_mac_table_status_t       mac_status;
    u32                           mac_index_next;  /* Index for MAC table get next */
    u32                           mac_table_max;   /* Maximum number of entries in mac_table */
    u32                           mac_table_count; /* Actual number of entries in mac_table */
    vtss_mac_entry_t              *mac_list_used;  /* Sorted list of entries */
    vtss_mac_entry_t              *mac_list_free;  /* Free list */
    vtss_mac_entry_t              mac_table[VTSS_MAC_ADDRS]; /* Sorted MAC address table */
    u32                           mac_ptr_count;   /* Number of valid pointers */
    vtss_mac_entry_t              *mac_list_ptr[VTSS_MAC_PTR_SIZE]; /* Pointer array */
    u32                           ac_count;
    vtss_aggr_mode_t              aggr_mode;
    u32                           aggr_chip_port_next[2];
    u32                           pgid_count;
    u32                           pgid_drop;
    u32                           pgid_flood;
    u32                           pgid_glag_dest;
    u32                           pgid_glag_src;
    u32                           pgid_glag_aggr_a;
    u32                           pgid_glag_aggr_b;
    vtss_pgid_entry_t             pgid_table[VTSS_PGIDS+1];

#ifdef VTSS_FEATURE_SFLOW
    vtss_sflow_port_conf_t        sflow_conf[VTSS_PORT_ARRAY_SIZE];
#ifdef VTSS_ARCH_JAGUAR_1
    u32                           sflow_ena_cnt[2]; /* Counts - per chip - the number of ports on which sFlow is enabled */
#endif
#endif

#endif /* VTSS_FEATURE_LAYER2 */
#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
    vtss_vlan_trans_grp2vlan_t    vt_trans_conf;
    vtss_vlan_trans_port2grp_t    vt_port_conf;
#endif /* VTSS_FEATURE_VLAN_TRANSLATION */
#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    vtss_vlan_counter_info_t      vlan_counters_info;
#endif /* VTSS_FEATURE_VLAN_COUNTERS */
#if defined(VTSS_FEATURE_AGGR_GLAG)
#if defined(VTSS_FEATURE_VSTAX_V1)
    vtss_port_no_t                glag_members[VTSS_GLAGS][VTSS_GLAG_PORT_ARRAY_SIZE];
#endif /* VSTAX_V1 */
#if defined(VTSS_FEATURE_VSTAX_V2)
    vtss_glag_conf_t              glag_conf[VTSS_GLAGS][VTSS_GLAG_PORT_ARRAY_SIZE];
#endif /* VSTAX_V2 */
    vtss_glag_no_t                port_glag_no[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_AGGR_GLAG */
#if defined(VTSS_FEATURE_VSTAX)
    vtss_vstax_conf_t             vstax_conf;
    BOOL                          vstax_port_enabled[VTSS_PORT_ARRAY_SIZE];
    const vtss_vstax_tx_header_t  *vstax_tx_header;
    vtss_vstax_info_t             vstax_info;
#endif /* VTSS_FEATURE_VSTAX */
#if defined(VTSS_FEATURE_ACL)
    vtss_acl_policer_conf_t       acl_policer_conf[VTSS_ACL_POLICERS];
#if defined(VTSS_ARCH_LUTON26)
    vtss_policer_alloc_t          acl_policer_alloc[VTSS_ACL_POLICERS];
#endif /* VTSS_ARCH_LUTON26 */
    vtss_acl_port_conf_t          acl_old_port_conf;
    vtss_acl_port_conf_t          acl_port_conf[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_ACL */
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    vtss_evc_policer_conf_t       evc_policer_conf[VTSS_EVC_POLICERS];
#if defined(VTSS_ARCH_LUTON26)
    vtss_policer_alloc_t          evc_policer_alloc[VTSS_EVC_POLICERS];
#endif /* VTSS_ARCH_LUTON26 */
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
#if defined(VTSS_ARCH_SERVAL)
    vtss_vcap_port_conf_t         vcap_port_conf[VTSS_PORT_ARRAY_SIZE];
    vtss_vcap_port_conf_t         vcap_port_conf_old;
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_FEATURE_EVC)
    vtss_evc_port_conf_t          evc_port_conf[VTSS_PORT_ARRAY_SIZE];
    vtss_evc_port_conf_t          evc_port_conf_old;
    vtss_evc_port_info_t          evc_port_info[VTSS_PORT_ARRAY_SIZE];
    vtss_evc_info_t               evc_info;
    vtss_ece_info_t               ece_info;
#if defined(VTSS_ARCH_SERVAL)
    vtss_mce_info_t               mce_info;
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_SDX_CNT)
    vtss_sdx_info_t               sdx_info;
#endif /* VTSS_SDX_CNT */
#endif /* VTSS_FEATURE_EVC */
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    u32                           evc_policer_max;
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
#if defined(VTSS_FEATURE_VCAP)
    vtss_vcap_range_chk_table_t   vcap_range;
    u32                           vcap_counter[2]; /* Multi-chip support */
#endif /* VTSS_FEATURE_VCAP */
#if defined(VTSS_FEATURE_IS0)
    vtss_is0_info_t               is0;
#endif /* VTSS_FEATURE_IS0 */
#if defined(VTSS_FEATURE_IS1)
    vtss_is1_info_t               is1;
#endif /* VTSS_FEATURE_IS1 */
#if defined(VTSS_FEATURE_IS2)
    vtss_is2_info_t               is2;
#endif /* VTSS_FEATURE_IS2 */
#if defined(VTSS_FEATURE_ES0)
    vtss_es0_info_t               es0;
#endif /* VTSS_FEATURE_ES0 */
#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
    fdma_state_t                  fdma_state; /* The whole FDMA state */
#endif

#if defined(VTSS_FEATURE_ANEG)
    vtss_aneg_state_t             aneg_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_XFI)
    vtss_xfi_state_t             xfi_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_SFI4)
    vtss_sfi4_state_t             sfi4_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_TFI5)
    vtss_tfi5_state_t             tfi5_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_XAUI)
    vtss_xaui_state_t             xaui_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_UPI)
    vtss_upi_state_t              upi_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_GFP)
    vtss_gfp_state_t              gfp_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_RAB)
    vtss_rab_state_t              rab_state[VTSS_PORT_ARRAY_SIZE];
#endif
   
#if defined(VTSS_FEATURE_OTN)
    vtss_otn_state_t              otn_state[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_OTN */

#if defined(VTSS_FEATURE_OHA)
    vtss_oha_state_t              oha_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_AE)
    vtss_ae_state_t               ae_state[VTSS_AE_LANE_CNT];
    u32                           ae_lane_cnt;
#endif 

#if defined(VTSS_FEATURE_MAC10G)
    vtss_mac10g_state_t           mac10g_state[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_PCS_10GBASE_R)
    vtss_pcs_10gbase_r_state_t          pcs_10gbase_r_state[VTSS_PORT_ARRAY_SIZE];
    vtss_pcs_10gbase_r_chip_counters_t  pcs_10gbase_r_cntrs[VTSS_PORT_ARRAY_SIZE];
#endif

#if defined(VTSS_FEATURE_EEE)
    BOOL                          eee_ena[VTSS_PORT_ARRAY_SIZE]; // Signaling current state in the switch api
    BOOL                          eee_ena_phy[VTSS_PORT_ARRAY_SIZE]; // Signaling current state in the phy api

#ifdef VTSS_ARCH_JAGUAR_1
    u32                           eee_loopback_chip_port[2];
    u32                           eee_ena_cnt;
#endif
#endif
#if defined(VTSS_FEATURE_FAN)
    i16                           chip_temp;      // Chip temperature
    u16                           rotation_count; // Fan rotations count
    vtss_fan_conf_t               fan_conf;       // Fan configuration/specifications.
    u8                            lvl;            // Fan speed level
#endif

#ifdef VTSS_CHIP_CU_PHY
  vtss_phy_led_mode_select_t led_mode_select; // LED blink mode
#endif
#if defined(VTSS_FEATURE_LED_POW_REDUC)
    u8                            led_intensity;
    vtss_phy_enhanced_led_control_t enhanced_led_control;
#endif

#if defined(VTSS_FEATURE_TIMESTAMP)
    vtss_ts_conf_t   ts_conf;
    vtss_ts_internal_mode_t ts_int_mode;
    vtss_ts_port_conf_t ts_port_conf[VTSS_PORT_ARRAY_SIZE];
    vtss_ts_timestamp_status_t ts_status [VTSS_TS_ID_SIZE];
#if defined (VTSS_ARCH_SERVAL_CE)
    vtss_oam_timestamp_status_t oam_ts_status [VTSS_VOE_ID_SIZE];
#endif /* VTSS_ARCH_SERVAL_CE */
#if defined (VTSS_ARCH_SERVAL)
    BOOL ts_add_sub_option;
#endif /* VTSS_ARCH_SERVAL */
#endif  /* VTSS_FEATURE_TIMESTAMP */

#if defined(VTSS_FEATURE_PHY_TIMESTAMP)
    vtss_phy_ts_port_conf_t  phy_ts_port_conf[VTSS_PORT_ARRAY_SIZE];
#if !defined(VTSS_PHY_TS_ENBL_TS_TABLE)
     vtss_phy_ts_fifo_read ts_fifo_cb;
     void *cntxt;
#endif /* VTSS_PHY_TS_ENBL_TS_TABLE */
#endif  /* VTSS_FEATURE_PHY_TIMESTAMP */

#if defined(VTSS_FEATURE_SYNCE)
    u32    old_port_no[VTSS_SYNCE_CLK_PORT_ARRAY_SIZE];
    vtss_synce_clock_in_t   synce_in_conf[VTSS_SYNCE_CLK_PORT_ARRAY_SIZE];
    vtss_synce_clock_out_t  synce_out_conf[VTSS_SYNCE_CLK_PORT_ARRAY_SIZE];
#endif  /* VTSS_FEATURE_SYNCE */

#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
    vtss_ipmc_info_t    ipmc;
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */

#if defined(VTSS_FEATURE_WIS)
    vtss_ewis_conf_t              ewis_conf[VTSS_PORT_ARRAY_SIZE];
#endif /* VTSS_FEATURE_WIS */

#if defined(VTSS_FEATURE_OAM)
    vtss_oam_vop_conf_t           oam_vop_conf;
    vtss_oam_voe_conf_t           oam_voe_conf[VTSS_OAM_VOE_CNT];
    vtss_oam_voe_internal_t       oam_voe_internal[VTSS_OAM_VOE_CNT];
    vtss_oam_voe_idx_t            oam_voe_poll_idx;
#endif /* VTSS_FEATURE_WIS */

#if defined(VTSS_FEATURE_MPLS)
    vtss_mpls_idxchain_entry_t    mpls_idxchain[VTSS_MPLS_IDXCHAIN_ENTRY_CNT];        /**< [0] is reserved for free-chain */

    vtss_mpls_vprofile_internal_t mpls_vprofile_conf[VTSS_MPLS_VPROFILE_CNT];
    vtss_mpls_l2_internal_t       mpls_l2_conf[VTSS_MPLS_L2_CNT];
    vtss_mpls_segment_internal_t  mpls_segment_conf[VTSS_MPLS_SEGMENT_CNT];
    vtss_mpls_xc_internal_t       mpls_xc_conf[VTSS_MPLS_XC_CNT];

    vtss_mpls_port_state_t        mpls_port_state[VTSS_PORT_ARRAY_SIZE];

    vtss_mpls_tc_conf_t           mpls_tc_conf;

    vtss_mpls_vprofile_idx_t      mpls_vprofile_free_list;
    vtss_mpls_l2_idx_t            mpls_l2_free_list;
    vtss_mpls_segment_idx_t       mpls_segment_free_list;
    vtss_mpls_xc_idx_t            mpls_xc_free_list;

    vtss_mpls_idxchain_t          mpls_is0_mll_free_chain;
    vtss_mpls_idxchain_t          mpls_encap_free_chain;
    
    // Counts of used HW entries of various types
    u32                           mpls_is0_mll_cnt;
    u32                           mpls_is0_mlbs_cnt;
    u32                           mpls_encap_cnt;
    u32                           mpls_vprofile_cnt;
#endif /* VTSS_FEATURE_MPLS */

#if defined(VTSS_ARCH_JAGUAR_1)
    vtss_l3_state_t l3;
#endif /* VTSS_ARCH_JAGUAR_1 */

    BOOL                   system_reseting; // Signaling if system is rebooting.

    vtss_debug_reg_check_t reg_check; // For holding register check state.

} vtss_state_t;

extern const char *vtss_port_if_txt(vtss_port_interface_t if_type);
extern vtss_state_t *vtss_state;
extern vtss_inst_t vtss_default_inst;

#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
BOOL l26_ssm_validate_fid(u16 *fid, BOOL fid_only);
vtss_rc l26_ssm_conflict_adjustment(vtss_ipmc_src_t *ssm);
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */

#if defined(VTSS_FEATURE_LAYER2)
vtss_rc vtss_mac_add(vtss_mac_user_t user, const vtss_mac_table_entry_t *const entry);
vtss_rc vtss_mac_del(vtss_mac_user_t user, const vtss_vid_mac_t *const vid_mac);
vtss_rc vtss_mac_get(const vtss_vid_mac_t   *const vid_mac,
                     vtss_mac_table_entry_t *const entry);
#endif /* VTSS_FEATURE_LAYER2 */

/* ================================================================= *
 *  Trace
 * ================================================================= */

#if VTSS_OPT_TRACE

/*lint -esym(459, vtss_trace_conf) */
extern vtss_trace_conf_t vtss_trace_conf[];

/* Default trace layer */
#ifndef VTSS_TRACE_LAYER
#define VTSS_TRACE_LAYER VTSS_TRACE_LAYER_AIL
#endif /* VTSS_TRACE_LAYER */

/* Default trace group */
#ifndef VTSS_TRACE_GROUP
#define VTSS_TRACE_GROUP VTSS_TRACE_GROUP_DEFAULT
#endif /* VTSS_TRACE_GROUP */

#define VTSS_E(...) VTSS_EG(VTSS_TRACE_GROUP, ##__VA_ARGS__)
#define VTSS_I(...) VTSS_IG(VTSS_TRACE_GROUP, ##__VA_ARGS__)
#define VTSS_D(...) VTSS_DG(VTSS_TRACE_GROUP, ##__VA_ARGS__)
#define VTSS_N(...) VTSS_NG(VTSS_TRACE_GROUP, ##__VA_ARGS__)

#define VTSS_E_HEX(_byte_p, _byte_cnt) VTSS_EG_HEX(VTSS_TRACE_GROUP, _byte_p, _byte_cnt)
#define VTSS_I_HEX(_byte_p, _byte_cnt) VTSS_IG_HEX(VTSS_TRACE_GROUP, _byte_p, _byte_cnt)
#define VTSS_D_HEX(_byte_p, _byte_cnt) VTSS_DG_HEX(VTSS_TRACE_GROUP, _byte_p, _byte_cnt)
#define VTSS_N_HEX(_byte_p, _byte_cnt) VTSS_NG_HEX(VTSS_TRACE_GROUP, _byte_p, _byte_cnt)

/* For files with multiple trace groups: */
#define VTSS_T(_grp, _lvl, ...) { if (vtss_trace_conf[_grp].level[VTSS_TRACE_LAYER] >= _lvl) vtss_callout_trace_printf(VTSS_TRACE_LAYER, _grp, _lvl, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); }
#define VTSS_EG(_grp, ...) VTSS_T(_grp, VTSS_TRACE_LEVEL_ERROR, __VA_ARGS__)
#define VTSS_IG(_grp, ...) VTSS_T(_grp, VTSS_TRACE_LEVEL_INFO,  __VA_ARGS__)
#define VTSS_DG(_grp, ...) VTSS_T(_grp, VTSS_TRACE_LEVEL_DEBUG, __VA_ARGS__)
#define VTSS_NG(_grp, ...) VTSS_T(_grp, VTSS_TRACE_LEVEL_NOISE, __VA_ARGS__)

#define VTSS_HEX(_grp, _lvl, _byte_p, _byte_cnt) { if (vtss_trace_conf[_grp].level[VTSS_TRACE_LAYER] >= _lvl) vtss_callout_trace_hex_dump(VTSS_TRACE_LAYER, _grp, _lvl, __FILE__, __LINE__, __FUNCTION__, _byte_p, _byte_cnt); }
#define VTSS_EG_HEX(_grp, _byte_p, _byte_cnt) VTSS_HEX(_grp, VTSS_TRACE_LEVEL_ERROR, _byte_p, _byte_cnt)
#define VTSS_IG_HEX(_grp, _byte_p, _byte_cnt) VTSS_HEX(_grp, VTSS_TRACE_LEVEL_INFO,  _byte_p, _byte_cnt)
#define VTSS_DG_HEX(_grp, _byte_p, _byte_cnt) VTSS_HEX(_grp, VTSS_TRACE_LEVEL_DEBUG, _byte_p, _byte_cnt)
#define VTSS_NG_HEX(_grp, _byte_p, _byte_cnt) VTSS_HEX(_grp, VTSS_TRACE_LEVEL_NOISE, _byte_p, _byte_cnt)

#else /* VTSS_OPT_TRACE */

/* No trace */
#define VTSS_E(...)
#define VTSS_I(...)
#define VTSS_D(...)
#define VTSS_N(...)

#define VTSS_EG(grp, ...)
#define VTSS_IG(grp, ...)
#define VTSS_DG(grp, ...)
#define VTSS_NG(grp, ...)

#define VTSS_EG(_grp, ...)
#define VTSS_IG(_grp, ...)
#define VTSS_DG(_grp, ...)
#define VTSS_NG(_grp, ...)

#define VTSS_EG_HEX(_grp, _byte_p, _byte_cnt)
#define VTSS_IG_HEX(_grp, _byte_p, _byte_cnt)
#define VTSS_DG_HEX(_grp, _byte_p, _byte_cnt)
#define VTSS_NG_HEX(_grp, _byte_p, _byte_cnt)

#endif /* VTSS_OPT_TRACE */


#endif /* _VTSS_STATE_H_ */
