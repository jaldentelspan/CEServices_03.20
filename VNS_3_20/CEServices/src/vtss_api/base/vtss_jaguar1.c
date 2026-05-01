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

*/

/* Use relative addresses for registers - must be first */
#define VTSS_IOADDR(t,o) ((((t) - VTSS_TO_DEVCPU_ORG) >> 2) + (o))
#define VTSS_IOREG(t,o)  (VTSS_IOADDR(t,o))

#define VTSS_TRACE_LAYER VTSS_TRACE_LAYER_CIL

// Avoid "vtss_api.h not used in module vtss_jaguar1.c"
/*lint --e{766} */

#include "vtss_api.h"
#include "vtss_state.h"

#if defined(VTSS_ARCH_JAGUAR_1)
#include <vtss_os.h>
#include "vtss_common.h"
#include "vtss_jaguar1.h"
#include "vtss_jaguar_reg.h"
#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
#include "vtss_vcoreiii_fdma.h"
#endif
#include "vtss_util.h"

#if defined(VTSS_FEATURE_WIS)
#include "vtss_wis_api.h"
#endif /* VTSS_FEATURE_WIS */

/* Chip ports */
#define VTSS_CHIP_PORTS      32
#define VTSS_CHIP_PORT_CPU_0 33
#define VTSS_CHIP_PORT_CPU_1 34
#define VTSS_CHIP_PORTS_ALL  35 /* Includes CPU ports */

#define VTSS_PORT_10G_START  27
#define VTSS_PORT_NPI        31
#define JR_STACK_PORT_START  24 /* Port 24-30 are potential stack ports */
#define JR_STACK_PORT_CNT     7 /* Port 24-30 are potential stack ports */
#define JR_STACK_PORT_END    (JR_STACK_PORT_START + JR_STACK_PORT_CNT)

#define VTSS_PORT_IS_1G(port)             ((port) < VTSS_PORT_10G_START || (port) == VTSS_PORT_NPI)
#define VTSS_PORT_IS_10G(port)            ((port) >= VTSS_PORT_10G_START && (port) < VTSS_PORT_NPI)
#define VTSS_PORT_IS_STACKABLE(chip_port) ((chip_port) >= JR_STACK_PORT_START && (chip_port) < VTSS_PORT_NPI)

#define VTSS_TO_DEV(port) (port == VTSS_PORT_NPI ? VTSS_TO_DEVRGMII_0 : VTSS_PORT_IS_10G(port) ? \
                           (VTSS_TO_DEV10G_0 + (port-27) * (VTSS_TO_DEV10G_1 - VTSS_TO_DEV10G_0)) :\
                           (VTSS_TO_DEV1G_0 + (port) * (VTSS_TO_DEV1G_1 - VTSS_TO_DEV1G_0)))


/* MC PGIDs */
#define PGID_UC_FLOOD     32
#define PGID_MC_FLOOD     33
#define PGID_IPV4_MC_DATA 34
#define PGID_IPV4_MC_CTRL 35
#define PGID_IPV6_MC_DATA 36
#define PGID_IPV6_MC_CTRL 37
#define PGID_DROP         38
#define PGID_FLOOD        39

/* GLAG PGID */
#define PGID_GLAG_START   800

/* Host Queue */
#define PGID_HQ_START   1024+32

/* Total number of SQS Cells */
#define JR_TOTAL_NUM_CELLS 26240

#define JR_PRIOS 8   /* Number of priorities */

#define JR_GPIOS        24
#define JR_SGPIO_GROUPS 2
#define JR_ACS          16  /* Number of aggregation masks */

#define JR_MIRROR_PROBE_RX  0 /* Ingress mirror probe */
#define JR_MIRROR_PROBE_TX  1 /* Egress mirror probe */
#define JR_MIRROR_PROBE_EEE 2 /* EEE mirror probe */

/* Counter allocations, 4 RX and 2 TX */
/* RX */
#define JR_CNT_RX_GREEN     0
#define JR_CNT_RX_YELLOW    1
#define JR_CNT_RX_QDROPS    2  /* RX Queue drop  */
#define JR_CNT_RX_PDROPS    3  /* Policer drop   */
/* TX */
#define JR_CNT_TX_DROPS     0  /* TX Queue drop  */
#define JR_CNT_TX_GREEN     0
#define JR_CNT_TX_YELLOW    1

#define JR_CNT_ANA_AC_FILTER 0

/* Packet injection and extraction */
#define JR_PACKET_RX_QUEUE_CNT 10
#define JR_PACKET_RX_GRP_CNT   4
#define JR_PACKET_TX_GRP_CNT   5

/* Reserved EVC policers */
#define JR_EVC_POLICER_NONE     0 /* Policer 0 must always be open, so we use this as "None" */
#define JR_EVC_POLICER_DISCARD  1 /* Policer 1 used to discard frames */
#define JR_EVC_POLICER_RESV_CNT 2 /* Number of reserved policers */

/* CE-MaX host interfaces */
#if defined(VTSS_CHIP_CE_MAX_24)
#define VTSS_CE_MAC_HMDA 29
#define VTSS_CE_MAC_HMDB 30
#elif defined(VTSS_CHIP_CE_MAX_12)
#define VTSS_CE_MAC_HMDA 28
#define VTSS_CE_MAC_HMDB 28
#endif /* VTSS_CHIP_CE_MAX_24 */

/* sFlow H/W-related min/max */
#define JR_SFLOW_MIN_SAMPLE_RATE       1 /**< Minimum allowable sampling rate for sFlow */
#define JR_SFLOW_MAX_SAMPLE_RATE   32767 /**< Maximum allowable sampling rate for sFlow */

/* ================================================================= *
 *  Function declarations
 * ================================================================= */
static vtss_rc jr_gpio_mode(const vtss_chip_no_t  chip_no,
                            const vtss_gpio_no_t  gpio_no,
                            const vtss_gpio_mode_t mode);
static vtss_rc jr_qos_wred_conf_set(const vtss_port_no_t port_no);
static vtss_rc jr_port_policer_fc_set(const vtss_port_no_t port_no, u32 chipport);
static vtss_rc jr_port_policer_set(u32 port, u32 idx, vtss_policer_t *conf, vtss_policer_ext_t *conf_ext);
static vtss_rc jr_queue_policer_set(u32 port, u32 queue, vtss_policer_t *conf);

#if defined(VTSS_FEATURE_VSTAX_V2)
static inline BOOL jr_is_frontport(vtss_port_no_t port_no);
static vtss_rc jr_vstax_update_ingr_shaper(u32 chipport, vtss_port_speed_t speed, BOOL front_port);
#endif /* VTSS_FEATURE_VSTAX_V2 */

static vtss_rc jr_setup_mtu(u32 chipport, u32 mtu, BOOL front_port);

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
static vtss_rc jr_init_cemax_oobfc_conf_set(const u8 host_instance, const BOOL fc_enable);
#endif

#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
static vtss_rc jr_qos_port_dot3ar_rate_limiter_conf_set(const vtss_port_no_t port_no);
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */

/* ================================================================= *
 *  Static variable declarations
 * ================================================================= */
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
/*lint -esym(459, ce_mac_hmda, ce_mac_hmdb) */
static i32 ce_mac_hmda; /* Host interface A */
static i32 ce_mac_hmdb; /* Host interface B */
#endif


/* ================================================================= *
 *  Register access
 * ================================================================= */

/* Concatenations */
#define JR_ADDR(tgt, addr)                 VTSS_##tgt##_##addr
#define JR_ADDR_X(tgt, addr, x)            VTSS_##tgt##_##addr(x)
#define JR_ADDR_XY(tgt, addr, x, y)        VTSS_##tgt##_##addr(x,y)
#define JR_GET_FLD(tgt, addr, fld, value)  VTSS_X_##tgt##_##addr##_##fld(value)
#define JR_PUT_FLD(tgt, addr, fld, value)  VTSS_F_##tgt##_##addr##_##fld(value)
#define JR_GET_BIT(tgt, addr, fld, value)  (VTSS_F_##tgt##_##addr##_##fld & (value) ? 1 : 0)
#define JR_PUT_BIT(tgt, addr, fld, value)  ((value) ? VTSS_F_##tgt##_##addr##_##fld : 0)
#define JR_SET_BIT(tgt, addr, fld)         VTSS_F_##tgt##_##addr##_##fld
#define JR_MSK(tgt, addr, fld)             VTSS_M_##tgt##_##addr##_##fld
#define JR_CNT(xmit, id)                   JR_CNT_##xmit##_##id

/* - Helpers (not used directly) ----------------------------------- */

/* Read register */
#define JR_RD_REG(reg, value)               VTSS_RC(jr_rd(reg, value));
#define JR_RD_REG_CHIP(chip_no, reg, value) VTSS_RC(jr_rd_chip(chip_no, reg, value));

/* Write register */
#define JR_WR_REG(reg, value)               VTSS_RC(jr_wr(reg, value));
#define JR_WR_REG_CHIP(chip_no, reg, value) VTSS_RC(jr_wr_chip(chip_no, reg, value));

/* Write register masked */
#define JR_WR_MSK(reg, value, mask)                VTSS_RC(jr_wrm(reg, value, mask));
#define JR_WR_MSK_CHIP(chip_no, reg, value, mask)  VTSS_RC(jr_wrm_chip(chip_no, reg, value, mask));

/* Read and extract register field */
#define JR_RD_FLD(reg, tgt, addr, fld, value)     \
{                                                 \
    u32 __x__;                                    \
    JR_RD_REG(reg, &__x__);                       \
    *(value) = JR_GET_FLD(tgt, addr, fld, __x__); \
}

/* Read and extract register field */
#define JR_RD_FLD_CHIP(chip_no, reg, tgt, addr, fld, value) \
{                                                           \
    u32 __x__;                                              \
    JR_RD_REG_CHIP(chip_no, reg, &__x__);                   \
    *(value) = JR_GET_FLD(tgt, addr, fld, __x__);           \
}

/* Write register field */
#define JR_WR_FLD(reg, tgt, addr, fld, value) \
    JR_WR_MSK(reg, JR_PUT_FLD(tgt, addr, fld, value), JR_MSK(tgt, addr, fld))
#define JR_WR_FLD_CHIP(chip_no, reg, tgt, addr, fld, value) \
    JR_WR_MSK_CHIP(chip_no, reg, JR_PUT_FLD(tgt, addr, fld, value), JR_MSK(tgt, addr, fld))

/* Read and extract register bit field */
#define JR_RD_BIT(reg, tgt, addr, fld, value)     \
{                                                 \
    u32 __x__;                                    \
    JR_RD_REG(reg, &__x__);                       \
    *(value) = JR_GET_BIT(tgt, addr, fld, __x__); \
}

/* Write register bit field */
#define JR_WR_BIT(reg, tgt, addr, fld, value) \
    JR_WR_MSK(reg, JR_PUT_BIT(tgt, addr, fld, value), JR_PUT_BIT(tgt, addr, fld, 1))
#define JR_WR_BIT_CHIP(chip_no, reg, tgt, addr, fld, value) \
    JR_WR_MSK_CHIP(chip_no, reg, JR_PUT_BIT(tgt, addr, fld, value), JR_PUT_BIT(tgt, addr, fld, 1))


/* - No replication ------------------------------------------------ */

/* Read register */
#define JR_RD(tgt, addr, value) JR_RD_REG(JR_ADDR(tgt, addr), value)

/* Write register */
#define JR_WR(tgt, addr, value)               JR_WR_REG(JR_ADDR(tgt, addr), value);
#define JR_WR_CHIP(chip_no, tgt, addr, value) JR_WR_REG_CHIP(chip_no, JR_ADDR(tgt, addr), value);

/* Read register field */
#define JR_RDF(tgt, addr, fld, value)               JR_RD_FLD(JR_ADDR(tgt, addr), tgt, addr, fld, value)
#define JR_RDF_CHIP(chip_no, tgt, addr, fld, value) JR_RD_FLD_CHIP(chip_no, JR_ADDR(tgt, addr), tgt, addr, fld, value)

/* Write register field */
#define JR_WRF(tgt, addr, fld, value) JR_WR_FLD(JR_ADDR(tgt, addr), tgt, addr, fld, value)

/* Read register bit field */
#define JR_RDB(tgt, addr, fld, value) JR_RD_BIT(JR_ADDR(tgt, addr), tgt, addr, fld, value)

/* Write register bit field */
#define JR_WRB(tgt, addr, fld, value)               JR_WR_BIT(JR_ADDR(tgt, addr), tgt, addr, fld, value)
#define JR_WRB_CHIP(chip_no, tgt, addr, fld, value) JR_WR_BIT_CHIP(chip_no, JR_ADDR(tgt, addr), tgt, addr, fld, value)


/* Write register masked and set/clear */
#define JR_WRM(tgt, addr, value, mask) JR_WR_MSK(JR_ADDR(tgt, addr), value, mask)
#define JR_WRM_SET(tgt, addr, mask)    JR_WRM(tgt, addr, mask, mask)
#define JR_WRM_CLR(tgt, addr, mask)    JR_WRM(tgt, addr, 0,    mask)

/* - Single replication (X) ---------------------------------------- */

/* Read register */
#define JR_RDX(tgt, addr, x, value) \
    JR_RD_REG(JR_ADDR_X(tgt, addr, x), value)

#define JR_RDX_CHIP(chip_no, tgt, addr, x, value) \
    JR_RD_REG_CHIP(chip_no, JR_ADDR_X(tgt, addr, x), value)

/* Write register */
#define JR_WRX(tgt, addr, x, value) \
    JR_WR_REG(JR_ADDR_X(tgt, addr, x), value)
#define JR_WRX_CHIP(chip_no, tgt, addr, x, value) \
    JR_WR_REG_CHIP(chip_no, JR_ADDR_X(tgt, addr, x), value)

/* Read register field */
#define JR_RDXF(tgt, addr, x, fld, value) \
    JR_RD_FLD(JR_ADDR_X(tgt, addr, x), tgt, addr, fld, value)
#define JR_RDXF_CHIP(chip_no, tgt, addr, x, fld, value) \
    JR_RD_FLD_CHIP(chip_no, JR_ADDR_X(tgt, addr, x), tgt, addr, fld, value)

/* Write register field */
#define JR_WRXF(tgt, addr, x, fld, value) \
    JR_WR_FLD(JR_ADDR_X(tgt, addr, x), tgt, addr, fld, value)
#define JR_WRXF_CHIP(chip_no, tgt, addr, x, fld, value) \
    JR_WR_FLD_CHIP(chip_no, JR_ADDR_X(tgt, addr, x), tgt, addr, fld, value)

/* Read register bit field */
#define JR_RDXB(tgt, addr, x, fld, value) \
    JR_RD_BIT(JR_ADDR_X(tgt, addr, x), tgt, addr, fld, value)

/* Write register bit field */
#define JR_WRXB(tgt, addr, x, fld, value) \
    JR_WR_BIT(JR_ADDR_X(tgt, addr, x), tgt, addr, fld, value)

#define JR_WRXB_CHIP(chip_no, tgt, addr, x, fld, value) \
    JR_WR_BIT_CHIP(chip_no, JR_ADDR_X(tgt, addr, x), tgt, addr, fld, value)

/* Write register masked and set/clear */
#define JR_WRXM(tgt, addr, x, value, mask) JR_WR_MSK(JR_ADDR_X(tgt, addr, x), value, mask)
#define JR_WRXM_SET(tgt, addr, x, mask)    JR_WRXM(tgt, addr, x, mask, mask)
#define JR_WRXM_CLR(tgt, addr, x, mask)    JR_WRXM(tgt, addr, x, 0,    mask)

/* - Double replication (X, Y) ------------------------------------- */

/* Read register */
#define JR_RDXY(tgt, addr, x, y, value) \
    JR_RD_REG(JR_ADDR_XY(tgt, addr, x, y), value)

/* Write register */
#define JR_WRXY(tgt, addr, x, y, value) \
    JR_WR_REG(JR_ADDR_XY(tgt, addr, x, y), value)

/* Read register field */
#define JR_RDXYF(tgt, addr, x, y, fld, value) \
    JR_RD_FLD(JR_ADDR_XY(tgt, addr, x, y), tgt, addr, fld, value)

/* Write register field */
#define JR_WRXYF(tgt, addr, x, y, fld, value) \
    JR_WR_FLD(JR_ADDR_XY(tgt, addr, x, y), tgt, addr, fld, value)

/* Read register bit field */
#define JR_RDXYB(tgt, addr, x, y, fld, value) \
    JR_RD_BIT(JR_ADDR_XY(tgt, addr, x, y), tgt, addr, fld, value)

/* Write register bit field */
#define JR_WRXYB(tgt, addr, x, y, fld, value) \
    JR_WR_BIT(JR_ADDR_XY(tgt, addr, x, y), tgt, addr, fld, value)

/* Write register masked and set/clear */
#define JR_WRXYM(tgt, addr, x, y, value, mask) \
    JR_WR_MSK(JR_ADDR_XY(tgt, addr, x, y), value, mask)
#define JR_WRXYM_SET(tgt, addr, x, y, mask) JR_WRXM(tgt, addr, x, y, mask, mask)
#define JR_WRXYM_CLR(tgt, addr, x, y, mask) JR_WRXM(tgt, addr, x, y, 0,    mask)

/* ----------------------------------------------------------------- */

/* Is register given by @addr directly accessible? */
static inline BOOL jr_reg_directly_accessible(vtss_chip_no_t chip_no, u32 addr)
{
#if VTSS_OPT_VCORE_III
    /* Running on internal CPU. chip_no == 0 registers are always directly accessible. */
    return (chip_no == 0 || addr < ((VTSS_IO_ORIGIN2_OFFSET - VTSS_IO_ORIGIN1_OFFSET) >> 2));
#else
    /* Running on external CPU. Both chip_no 0 and 1 are subject to indirect access */
    return (addr < ((VTSS_IO_ORIGIN2_OFFSET - VTSS_IO_ORIGIN1_OFFSET) >> 2));
#endif
}

static vtss_rc jr_wr_chip(vtss_chip_no_t chip_no, u32 addr, u32 value);
static vtss_rc jr_rd_chip(vtss_chip_no_t chip_no, u32 addr, u32 *value);

/* Read or write register indirectly */
static vtss_rc jr_reg_indirect_access(vtss_chip_no_t chip_no, u32 addr, u32 *value, BOOL is_write)
{
    /* The following access must be executed atomically, and since this function may be called
     * without the API lock taken, we have to disable the scheduler
     */
    /*lint --e{529} */ // Avoid "Symbol 'flags' not subsequently referenced" Lint warning
    VTSS_OS_SCHEDULER_FLAGS flags = 0;
    u32 ctrl;
    vtss_rc result;

    /* The @addr is an address suitable for the read or write callout function installed by
     * the application, i.e. it's a 32-bit address suitable for presentation on a PI
     * address bus, i.e. it's not suitable for presentation on the VCore-III shared bus.
     * In order to make it suitable for presentation on the VCore-III shared bus, it must
     * be made an 8-bit address, so we multiply by 4, and it must be offset by the base
     * address of the switch core registers, so we add VTSS_IO_ORIGIN1_OFFSET.
     */
    addr <<= 2;
    addr += VTSS_IO_ORIGIN1_OFFSET;

    VTSS_OS_SCHEDULER_LOCK(flags);

    if ((result = jr_wr_chip(chip_no, VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_ADDR, addr)) != VTSS_RC_OK) {
        goto do_exit;
    }
    if (is_write) {
        if ((result = jr_wr_chip(chip_no, VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, *value)) != VTSS_RC_OK) {
            goto do_exit;
        }
        // Wait for operation to complete
        do {
            if ((result = jr_rd_chip(chip_no, VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL, &ctrl)) != VTSS_RC_OK) {
                goto do_exit;
            }
        } while (ctrl & VTSS_F_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL_VA_BUSY);
    } else {
        // Dummy read to initiate access
        if ((result = jr_rd_chip(chip_no, VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, value)) != VTSS_RC_OK) {
            goto do_exit;
        }
        // Wait for operation to complete
        do {
            if ((result = jr_rd_chip(chip_no, VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL, &ctrl)) != VTSS_RC_OK) {
                goto do_exit;
            }
        } while (ctrl & VTSS_F_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL_VA_BUSY);
        if ((result = jr_rd_chip(chip_no, VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, value)) != VTSS_RC_OK) {
            goto do_exit;
        }
    }

do_exit:
    VTSS_OS_SCHEDULER_UNLOCK(flags);
    return result;
}

/* Read register */
static vtss_rc jr_rd(u32 addr, u32 *value)
{
    if (jr_reg_directly_accessible(vtss_state->chip_no, addr)) {
        return vtss_state->init_conf.reg_read(vtss_state->chip_no, addr, value);
    } else {
        return jr_reg_indirect_access(vtss_state->chip_no, addr, value, FALSE);
    }
}

/* Read register */
static vtss_rc jr_rd_chip(vtss_chip_no_t chip_no, u32 addr, u32 *value)
{
    if (jr_reg_directly_accessible(chip_no, addr)) {
        return vtss_state->init_conf.reg_read(chip_no, addr, value);
    } else {
        return jr_reg_indirect_access(chip_no, addr, value, FALSE);
    }
}

/* Write register */
static vtss_rc jr_wr(u32 addr, u32 value)
{
    if (jr_reg_directly_accessible(vtss_state->chip_no, addr)) {
        return vtss_state->init_conf.reg_write(vtss_state->chip_no, addr, value);
    } else {
        return jr_reg_indirect_access(vtss_state->chip_no, addr, &value, TRUE);
    }
}

/* Write register */
static vtss_rc jr_wr_chip(vtss_chip_no_t chip_no, u32 addr, u32 value)
{
    if (jr_reg_directly_accessible(chip_no, addr)) {
        return vtss_state->init_conf.reg_write(chip_no, addr, value);
    } else {
        return jr_reg_indirect_access(chip_no, addr, &value, TRUE);
    }
}

/* Read, modify and write register */
static vtss_rc jr_wrm(u32 addr, u32 value, u32 mask)
{
    vtss_rc rc;
    u32     val;

    if ((rc = jr_rd(addr, &val)) == VTSS_RC_OK)
        rc = jr_wr(addr, (val & ~mask) | (value & mask));
    return rc;
}

/* Read, modify and write register */
#ifdef VTSS_FEATURE_EEE
static vtss_rc jr_wrm_chip(vtss_chip_no_t chip_no, u32 addr, u32 value, u32 mask)
{
  vtss_rc rc;
  u32     val;

  if ((rc = jr_rd_chip(chip_no, addr, &val)) == VTSS_RC_OK) {
      rc = jr_wr_chip(chip_no, addr, (val & ~mask) | (value & mask));
  }
  return rc;
}
#endif

/* Poll bit until zero */
#define JR_POLL_BIT(tgt, addr, fld)                                \
{                                                                  \
    u32 _x_, count = 0;                                            \
    do {                                                           \
        JR_RDB(tgt, addr, fld, &_x_);                              \
        VTSS_MSLEEP(1);                                            \
        count++;                                                   \
        if (count == 100) {                                        \
            VTSS_E("timeout, target address: %s::%s", #tgt, #addr); \
            return VTSS_RC_ERROR;                                  \
        }                                                          \
    } while (_x_);                                                 \
}

/* ================================================================= *
 *  Utility functions
 * ================================================================= */

static u32 jr_port_mask(const BOOL member[])
{
    vtss_port_no_t port_no;
    u32            port, mask = 0;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (member[port_no] && VTSS_PORT_CHIP_SELECTED(port_no)) {
            port = VTSS_CHIP_PORT(port_no);
            mask |= VTSS_BIT(port);
        }
    }
    return mask;
}

static u32 jr_port_mask_from_map(BOOL include_internal_ports, BOOL include_stack_ports)
{
    vtss_port_no_t port_no;
    u32            mask = 0;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (VTSS_PORT_CHIP_SELECTED(port_no) 
#if defined(VTSS_FEATURE_VSTAX_V2)
            && (vtss_state->vstax_port_enabled[port_no] == 0 || include_stack_ports)
#endif /* VSTAX_V2 */
            ) {
            mask |= VTSS_BIT(VTSS_CHIP_PORT(port_no));
        }
    }
    if (include_internal_ports && vtss_state->chip_count == 2) {
        /* Include internal ports in mask */
        mask |= vtss_state->mask_int_ports;
    }
    return mask;
}

/* Convert to PGID in chip */
static u32 jr_chip_pgid(u32 pgid)
{
    if (pgid < vtss_state->port_count) {
        return VTSS_CHIP_PORT(pgid);
    } else {
        return (pgid + VTSS_CHIP_PORTS - vtss_state->port_count);
    }
}

/* Convert from PGID in chip */
static u32 jr_vtss_pgid(const vtss_state_t *const state, u32 pgid)
{
    vtss_port_no_t        port_no;
    const vtss_port_map_t *pmap;

    if (pgid < VTSS_CHIP_PORTS) {
        for (port_no = VTSS_PORT_NO_START; port_no < state->port_count; port_no++) {
            pmap = &state->port_map[port_no];
            if (pmap->chip_port == pgid && pmap->chip_no == state->chip_no)
                break;
        }
        return port_no;
    } else {
        return (state->port_count + pgid - VTSS_CHIP_PORTS);
    }
}

/* Update MTU */
static vtss_rc jr_setup_mtu(u32 chipport, u32 mtu, BOOL front_port)
{
#if defined(VTSS_FEATURE_VSTAX_V2)
    if (!front_port)
        mtu += VTSS_VSTAX_HDR_SIZE; /* 12 bytes extra MTU */
#endif /* VTSS_FEATURE_VSTAX_V2 */
    if(VTSS_PORT_IS_1G(chipport)) {
        JR_WRX(DEV1G, MAC_CFG_STATUS_MAC_MAXLEN_CFG, VTSS_TO_DEV(chipport), mtu);
    } else {
        JR_WRX(DEV10G, MAC_CFG_STATUS_MAC_MAXLEN_CFG, VTSS_TO_DEV(chipport), mtu);
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_port_max_tags_set(vtss_port_no_t port_no)
{
    vtss_port_max_tags_t  max_tags = vtss_state->port_conf[port_no].max_tags;
    vtss_vlan_port_type_t vlan_type = vtss_state->vlan_port_conf[port_no].port_type;
    u32                   port = VTSS_CHIP_PORT(port_no);
    u32                   tgt = VTSS_TO_DEV(port);
    u32                   etype, double_ena, single_ena;

    /* S-ports and VLAN unaware ports both support 0x88a8 (in addition to 0x8100) */
    etype = (vlan_type == VTSS_VLAN_PORT_TYPE_S_CUSTOM ? vtss_state->vlan_conf.s_etype :
             vlan_type == VTSS_VLAN_PORT_TYPE_C ? 0x8100 : 0x88a8);
    single_ena = (max_tags == VTSS_PORT_MAX_TAGS_NONE ? 0 : 1);
    double_ena = (max_tags == VTSS_PORT_MAX_TAGS_TWO ? 1 : 0);
    
    if (VTSS_PORT_IS_10G(port)) {
        JR_WRX(DEV10G, MAC_CFG_STATUS_MAC_TAGS_CFG, tgt,
               JR_PUT_FLD(DEV10G, MAC_CFG_STATUS_MAC_TAGS_CFG, TAG_ID, etype) |
               JR_PUT_BIT(DEV10G, MAC_CFG_STATUS_MAC_TAGS_CFG, DOUBLE_TAG_ENA, double_ena) | 
               JR_PUT_BIT(DEV10G, MAC_CFG_STATUS_MAC_TAGS_CFG, VLAN_AWR_ENA, single_ena));
    } else {
        JR_WRX(DEV1G, MAC_CFG_STATUS_MAC_TAGS_CFG, tgt,
               JR_PUT_FLD(DEV1G, MAC_CFG_STATUS_MAC_TAGS_CFG, TAG_ID, etype) |
               JR_PUT_BIT(DEV1G, MAC_CFG_STATUS_MAC_TAGS_CFG, PB_ENA, double_ena) | 
               JR_PUT_BIT(DEV1G, MAC_CFG_STATUS_MAC_TAGS_CFG, VLAN_AWR_ENA, single_ena) |
               JR_PUT_BIT(DEV1G, MAC_CFG_STATUS_MAC_TAGS_CFG, VLAN_LEN_AWR_ENA, 1));
    }
    return VTSS_RC_OK;
}

static BOOL cemax_port_is_host(vtss_port_no_t port_no) 
{
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    return (VTSS_CHIP_PORT(port_no) == ce_mac_hmda || VTSS_CHIP_PORT(port_no) == ce_mac_hmdb);
#else
    return FALSE;
#endif
}

static u32 ce_max_hm(void) 
{
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)   
    switch (vtss_state->init_conf.host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
    case VTSS_HOST_MODE_0:
        return 0;        
    case VTSS_HOST_MODE_1:
        return 1;        
#endif /* VTSS_CHIP_CE_MAX_24 */
    case VTSS_HOST_MODE_2:
        return 2;        
    case VTSS_HOST_MODE_3:
        return 3;        
    default:
        return 0;        
    }
#else
    return 0;
#endif
}

/* Call configuration function for all devices */
static vtss_rc jr_conf_chips(vtss_rc (*func_conf)(void))
{
    vtss_chip_no_t chip_no;

    /* Setup all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(func_conf());
    }
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Port control
 * ================================================================= */

/* PHY commands */
#define PHY_CMD_ADDRESS  0 /* 10G: Address */
#define PHY_CMD_WRITE    1 /* 1G/10G: Write */
#define PHY_CMD_READ_INC 2 /* 1G: Read, 10G: Read and increment */
#define PHY_CMD_READ     3 /* 10G: Read */

static vtss_rc jr_miim_cmd(u32 cmd, u32 sof, vtss_miim_controller_t miim_controller,
                           u8 miim_addr, u8 addr, u16 *data, BOOL report_errors)
{
    u32 i, n;
    u32 value;

    switch (miim_controller) {
    case VTSS_MIIM_CONTROLLER_0:
        i = 0;
        break;
    case VTSS_MIIM_CONTROLLER_1:
        i = 1;
        break;
    default:
        VTSS_E("illegal miim_controller: %d", miim_controller);
        return VTSS_RC_ERROR;
    }

    /* Set Start of frame field */
    JR_WRX(DEVCPU_GCB, MIIM_MII_CFG, i,
           VTSS_F_DEVCPU_GCB_MIIM_MII_CFG_MIIM_CFG_PRESCALE(0x32) |
           VTSS_F_DEVCPU_GCB_MIIM_MII_CFG_MIIM_ST_CFG_FIELD(sof));

    /* Read command is different for Clause 22 */
    if (sof == 1 && cmd == PHY_CMD_READ) {
        cmd = PHY_CMD_READ_INC;
    }

    /* Start command */
    JR_WRX(DEVCPU_GCB, MIIM_MII_CMD, i,
           VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_VLD |
           VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_PHYAD(miim_addr) |
           VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_REGAD(addr) |
           VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_WRDATA(*data) |
           VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_OPR_FIELD(cmd));

    /* Wait for access to complete */
    for (n = 0; ; n++) {
        JR_RDX(DEVCPU_GCB, MIIM_MII_STATUS, i, &value);
        if (JR_GET_BIT(DEVCPU_GCB, MIIM_MII_STATUS, MIIM_STAT_PENDING_RD, value) == 0 &&
                JR_GET_BIT(DEVCPU_GCB, MIIM_MII_STATUS, MIIM_STAT_PENDING_WR, value) == 0) {
            break;
        }
        if (n == 1000) {
            goto mmd_error;
        }
    }

    /* Read data */
    if (cmd == PHY_CMD_READ || cmd == PHY_CMD_READ_INC) {
        JR_RDX(DEVCPU_GCB, MIIM_MII_DATA, i, &value);
        if (JR_GET_FLD(DEVCPU_GCB, MIIM_MII_DATA, MIIM_DATA_SUCCESS, value)) {
            goto mmd_error;
        }
        *data = JR_GET_FLD(DEVCPU_GCB, MIIM_MII_DATA, MIIM_DATA_RDDATA, value);
    }
    return VTSS_RC_OK;

mmd_error:
    if(report_errors) {
        VTSS_E("miim failed, cmd: %s, miim_addr: %u, addr: %u",
               cmd == PHY_CMD_READ ? "PHY_CMD_READ" :
               cmd == PHY_CMD_WRITE ? "PHY_CMD_WRITE" :
               cmd == PHY_CMD_ADDRESS ? "PHY_CMD_ADDRESS" : "PHY_CMD_READ_INC",
               miim_addr, addr);
    }

    return VTSS_RC_ERROR;
}

static vtss_rc jr_miim_read(vtss_miim_controller_t miim_controller,
                            u8 miim_addr,
                            u8 addr,
                            u16 *value, 
                            BOOL report_errors)
{
    return jr_miim_cmd(PHY_CMD_READ, 1, miim_controller, miim_addr, addr, value, report_errors);
}

static vtss_rc jr_miim_write(vtss_miim_controller_t miim_controller,
                             u8 miim_addr,
                             u8 addr,
                             u16 value, 
                             BOOL report_errors)
{
    return jr_miim_cmd(PHY_CMD_WRITE, 1, miim_controller, miim_addr, addr, &value, report_errors);
}

/* MMD (MDIO Management Devices (10G)) read */
static vtss_rc jr_mmd_read(vtss_miim_controller_t miim_controller,
                           u8 miim_addr, u8 mmd, u16 addr, u16 *value, BOOL report_errors)
{
    VTSS_RC(jr_miim_cmd(PHY_CMD_ADDRESS, 0, miim_controller, miim_addr, mmd, &addr, report_errors));
    return jr_miim_cmd(PHY_CMD_READ, 0, miim_controller, miim_addr, mmd, value, report_errors);
}

/* MMD (MDIO Management Devices (10G)) read-inc */
static vtss_rc jr_mmd_read_inc(vtss_miim_controller_t miim_controller,
                           u8 miim_addr, u8 mmd, u16 addr, u16 *buf, u8 count, BOOL report_errors)
{

    VTSS_RC(jr_miim_cmd(PHY_CMD_ADDRESS, 0, miim_controller, miim_addr, mmd, &addr, report_errors));
    while (count > 0) {
        VTSS_RC(jr_miim_cmd(PHY_CMD_READ_INC, 0, miim_controller, miim_addr, mmd, buf, report_errors));
        buf++;
        count--;
    }
    return VTSS_RC_OK;
}


/* MMD (MDIO Management Devices (10G)) write */
static vtss_rc jr_mmd_write(vtss_miim_controller_t miim_controller,
                            u8 miim_addr, u8 mmd, u16 addr, u16 data,  BOOL report_errors)
{
    VTSS_RC(jr_miim_cmd(PHY_CMD_ADDRESS, 0, miim_controller, miim_addr, mmd, &addr, report_errors));
    return jr_miim_cmd(PHY_CMD_WRITE, 0, miim_controller, miim_addr, mmd, &data, report_errors);
}

/* ================================================================= *
 *  SQS / WM setup
 * ================================================================= */
typedef struct {
    u32 atop;
    u32 hwm;
    u32 lwm;
    u32 swm;
    u32 fwd_press;
    BOOL cnt_on_port;
    BOOL cnt_on_prio;
    BOOL cnt_on_buf_port;
    BOOL cnt_on_buf_prio;
    BOOL rc_mode;  // 0: Normal 1: Minimum guaranteed
    BOOL fc_drop_mode;  // 0: FC     1: Drop
    BOOL rx_ena;
    u32 mtu_pre_alloc;
} sqs_qu_t;

typedef struct {
    u32 atop;
    u32 hwm;
    u32 lwm;
    u32 swm;
    u32 gwm;
    // GWM not used in SME
    u32 cmwm; // only OQS
    BOOL cnt_on_buf;
    BOOL rc_mode;  // 0: Normal 1: Minimum guaranteed
    BOOL fc_drop_mode;  // 0: FC     1: Drop
    BOOL rx_ena;
    sqs_qu_t qu[8];
} sqs_port_t;

typedef struct {
    u32 atop;
    u32 hwm;
    u32 lwm;
    BOOL fc_drop_mode;  // 0: FC     1: Drop
    BOOL rx_ena;
    u32 pre_alloc_frm_size[4];
    sqs_port_t port;
} sqs_buf_t;  // BUF-port - BUF-prio not used in this setup.

typedef struct {
    sqs_buf_t iqs;
    sqs_buf_t oqs;
} sqs_t;

#define WRX_SQS(qs, addr, x, value)                     \
{                                                      \
    vtss_rc rc = jr_wr((qs ? VTSS_OQS_##addr(x) : VTSS_IQS_##addr(x)), value); \
    if (rc != VTSS_RC_OK)                              \
        return rc;                                     \
}

#define WR_SQS(qs, addr, value)                     \
{                                                      \
    vtss_rc rc = jr_wr((qs ? VTSS_OQS_##addr : VTSS_IQS_##addr), value); \
    if (rc != VTSS_RC_OK)                              \
        return rc;                                     \
}

#define RDX_SQS(qs, addr, x, value)                \
{                                                      \
    vtss_rc rc = jr_rd((qs ? VTSS_OQS_##addr(x) : VTSS_IQS_##addr(x)), value); \
    if (rc != VTSS_RC_OK)                              \
        return rc;                                     \
}
#define RD_SQS(qs, addr, value)                \
{                                                      \
    vtss_rc rc = jr_rd((qs ? VTSS_OQS_##addr : VTSS_IQS_##addr), value); \
    if (rc != VTSS_RC_OK)                              \
        return rc;                                     \
}


#define PST_SQS(front,qs,back) (qs ? front##_OQS_##back : front##_IQS_##back)

static u32 port_prio_2_qu (u32 port, u32 prio, u32 qs) {
    if ((port < 32) || (qs == 0)) {
        return 8*port + prio;
    } else {      
        if (port == 32+48) {
            // OQS-VD
            return (port-48)*8+192+prio;
        } else if ((port > 80) && (port < 89)) {
            // OQS CPU
            return (81-48)*8 + 192 + (port-81);
        } else {
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)   
            u32 prios;
            // OQS Host ports            
            if ((ce_max_hm() == 0) ||  (ce_max_hm() == 2)) {
                prios = 8;
            } else {
                prios = 4;
            }
            return 256 + (port-32)*prios + prio;
#else
            return (81-48)*8 + 192 + (port-81);
#endif
        }
    }
}

static vtss_rc jr_sqs_rd(u32 port_no, sqs_t *sqs)
{
    u32 q, qu_id, qs, i, value, prios=8;
    sqs_buf_t *psqs;

    if ((ce_max_hm() == 1) || (ce_max_hm() == 3)) {
        prios = 4;
    } 

   /*  Queue Level  */
    for (qs = 0; qs < 2; qs++) { /* 0 = IQS, 1 = OQS */
        if ((qs == 0) && (port_no > 34)) {
            continue;
        }
        if (qs == 0) {
            psqs = &sqs->iqs;
        } else {
            psqs = &sqs->oqs;
        }

        if (port_no > 80) {
            prios = 1; /* CPU OQS qu */
        }

        for (q = 0; q < prios; q++) { 
            qu_id = port_prio_2_qu(port_no, q, qs);

            RDX_SQS(qs, QU_RAM_CFG_QU_RC_CFG, qu_id, &value);
            psqs->port.qu[q].fwd_press = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_CFG_FWD_PRESS_THRES(value));
            psqs->port.qu[q].cnt_on_port = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_PORT_LVL_ENA) & value ? 1 : 0;
            psqs->port.qu[q].cnt_on_buf_prio = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_BUF_PRIO_LVL_ENA) & value ? 1 : 0;
            psqs->port.qu[q].cnt_on_buf_port = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_BUF_PORT_LVL_ENA) & value ? 1 : 0;
            psqs->port.qu[q].rc_mode = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_RC_MODE) & value ? 1 : 0;
            psqs->port.qu[q].fc_drop_mode = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_FC_DROP_MODE) & value ? 1 : 0;
            psqs->port.qu[q].rx_ena = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_RX_QU_ENA) & value ? 1 : 0;
            RDX_SQS(qs, QU_RAM_CFG_QU_RC_HLWM_CFG, qu_id, &value);
            psqs->port.qu[q].hwm = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_HWM(value));
            psqs->port.qu[q].lwm = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_LWM(value));
            RDX_SQS(qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG, qu_id, &value);
            psqs->port.qu[q].atop = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_ATOP(value));
            psqs->port.qu[q].swm  = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_SWM(value));
            RDX_SQS(qs, QU_RAM_CFG_MTU_QU_MAP_CFG, qu_id, &psqs->port.qu[q].mtu_pre_alloc);
        }
        
        /* Port Level */                  
        RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_CFG, port_no, &value); 
        psqs->port.rx_ena       = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_RX_PORT_ENA) & value ? 1 : 0;     
        psqs->port.cnt_on_buf   = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_CNT_ON_BUF_LVL_ENA) & value ? 1 : 0;
        psqs->port.fc_drop_mode = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_FC_DROP_MODE) & value ? 1 : 0;
        psqs->port.rc_mode      = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_RC_MODE) & value ? 1 : 0;  
        RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG, port_no, &value);
        psqs->port.hwm          = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG_PORT_RC_PROFILE_HWM(value));
        psqs->port.lwm          = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG_PORT_RC_PROFILE_LWM(value));
        RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG, port_no, &value);
        psqs->port.atop          = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_ATOP(value));   
        psqs->port.swm           = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_SWM(value));   
        RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_GWM_CFG, port_no, &value);
        psqs->port.gwm           = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_SWM(value));

        /* Buffer Port Level */
        RD_SQS(qs, RESOURCE_CTRL_CFG_BUF_RC_CFG, &value);
        psqs->rx_ena       = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_RX_BUF_ENA) & value ? 1 : 0; 
        psqs->fc_drop_mode = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_BUF_PORT_FC_DROP_MODE) & value ? 1 : 0;
        psqs->atop         = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_BUF_PORT_RC_PROFILE_ATOP(value));;
        RD_SQS(qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG, &value);
        psqs->hwm          = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG_BUF_PORT_RC_PROFILE_HWM(value));
        psqs->lwm          = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG_BUF_PORT_RC_PROFILE_LWM(value));
        for (i = 0; i < 4; i++) {         
            RDX_SQS(qs, MTU_MTU_FRM_SIZE_CFG, i, &psqs->pre_alloc_frm_size[i]);
        }
    }   
    return VTSS_RC_OK;     
}

static vtss_rc jr_sqs_set(u32 port_no, sqs_t *sqs)
{
    u32 q, qu_id, qs, i, prios=8;
    sqs_buf_t *psqs;

    if ((ce_max_hm() == 1) || (ce_max_hm() == 3)) {
        prios = 4;
    } 
   /*  Queue Level  */
    for (qs = 0; qs < 2; qs++) { /* 0 = IQS, 1 = OQS */
        if ((qs == 0) && (port_no > 34)) {
            continue;
        } 
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)   
        if ((prios == 8) && (port_no > 55))
            continue; /* nore more queues */
#else
        if ((qs == 1) && (port_no > 31 && port_no < 81))
            prios = 4; /* Reserved host queues */   
#endif

        if (qs == 0) {
            psqs = &sqs->iqs;
        } else {
            psqs = &sqs->oqs;
        }

        if (port_no > 80) {
            prios = 1; /* CPU OQS qu */
        }

        for (q = 0; q < prios; q++) { 
            qu_id = port_prio_2_qu(port_no, q, qs);

            WRX_SQS(qs, QU_RAM_CFG_QU_RC_CFG, qu_id,
                    PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_FWD_PRESS_THRES(psqs->port.qu[q].fwd_press)) | VTSS_BIT(7) |
                    (psqs->port.qu[q].cnt_on_prio ? PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_PRIO_LVL_ENA) : 0) |
                    (psqs->port.qu[q].cnt_on_port ? PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_PORT_LVL_ENA) : 0) |
                    (psqs->port.qu[q].cnt_on_buf_prio ? PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_BUF_PRIO_LVL_ENA) : 0) |
                    (psqs->port.qu[q].cnt_on_buf_port ? PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_BUF_PORT_LVL_ENA) : 0) |
                    (psqs->port.qu[q].rc_mode ? PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_RC_MODE) : 0) |
                    (psqs->port.qu[q].fc_drop_mode ? PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_FC_DROP_MODE) : 0) |
                    (psqs->port.qu[q].rx_ena ? PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_RX_QU_ENA) : 0));
                
            WRX_SQS(qs, QU_RAM_CFG_QU_RC_HLWM_CFG, qu_id,
                    PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_HWM(psqs->port.qu[q].hwm)) |
                    PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_LWM(psqs->port.qu[q].lwm)));
                
            WRX_SQS(qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG, qu_id,
                    PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_ATOP(psqs->port.qu[q].atop)) |
                    PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_SWM(psqs->port.qu[q].swm)));
            
            WRX_SQS(qs, QU_RAM_CFG_MTU_QU_MAP_CFG, qu_id, psqs->port.qu[q].mtu_pre_alloc);            

        }
 
        /*  Port Level  */
        WRX_SQS(qs, PORT_RAM_CFG_PORT_RC_CFG, port_no,
                (psqs->port.rx_ena ? PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_RX_PORT_ENA) : 0) |
                (psqs->port.fc_drop_mode ? PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_FC_DROP_MODE) : 0) |
                (psqs->port.rc_mode ? PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_RC_MODE) : 0) |
                (psqs->port.cnt_on_buf ? PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_CNT_ON_BUF_LVL_ENA) : 0));
        
        WRX_SQS(qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG, port_no,
                PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG_PORT_RC_PROFILE_HWM(psqs->port.hwm)) |
                PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG_PORT_RC_PROFILE_LWM(psqs->port.lwm)));
        
        WRX_SQS(qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG, port_no,
                PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_ATOP(psqs->port.atop)) |
                PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_SWM(psqs->port.swm)));
        
        WRX_SQS(qs, PORT_RAM_CFG_PORT_RC_GWM_CFG, port_no,
                PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_GWM_CFG_PORT_RC_PROFILE_GWM(psqs->port.gwm)));

        if (qs == 1) {
            JR_WRX(OQS, PORT_RAM_CFG_CM_PORT_WM, port_no, VTSS_F_OQS_PORT_RAM_CFG_CM_PORT_WM_CM_PORT_WM(psqs->port.cmwm));
        }
       
        /*  BUF port  Level  */
        WR_SQS(qs, RESOURCE_CTRL_CFG_BUF_RC_CFG,
               PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_BUF_PORT_RC_PROFILE_ATOP(psqs->atop)) |
               (psqs->fc_drop_mode ? PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_BUF_PORT_FC_DROP_MODE) : 0) |
               (psqs->rx_ena ? PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_RX_BUF_ENA) : 0));
        
        WR_SQS(qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG,
                PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG_BUF_PORT_RC_PROFILE_HWM(psqs->hwm)) |
                PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG_BUF_PORT_RC_PROFILE_LWM(psqs->lwm)));

        for (i = 0; i < 4; i++) {         
            WRX_SQS(qs, MTU_MTU_FRM_SIZE_CFG, i, psqs->pre_alloc_frm_size[i]);
        }
    }   
    return VTSS_RC_OK;     
}


#define IQS_SET_QU(q, _atop, _hwm, _lwm, _swm, fwd, cnt_port, cnt_prio, cnt_buf_port, rc, fc, ena, mtu) \
 {           \
  psqs->iqs.port.qu[q].atop  = _atop;  \
  psqs->iqs.port.qu[q].hwm  = _hwm;  \
  psqs->iqs.port.qu[q].lwm  = _lwm; \
  psqs->iqs.port.qu[q].swm  = _swm; \
  psqs->iqs.port.qu[q].fwd_press = fwd; \
  psqs->iqs.port.qu[q].cnt_on_port = cnt_port; \
  psqs->iqs.port.qu[q].cnt_on_prio = cnt_prio; \
  psqs->iqs.port.qu[q].cnt_on_buf_port = cnt_buf_port; \
  psqs->iqs.port.qu[q].cnt_on_buf_prio = 0; \
  psqs->iqs.port.qu[q].rc_mode = rc;  \
  psqs->iqs.port.qu[q].fc_drop_mode = fc; \
  psqs->iqs.port.qu[q].rx_ena = ena; \
  psqs->iqs.port.qu[q].mtu_pre_alloc = mtu; \
}

#define OQS_SET_QU(q,  _atop, _hwm, _lwm, _swm, fwd, cnt_port, cnt_prio, cnt_buf_port, rc, fc, ena, mtu) \
 {           \
  psqs->oqs.port.qu[q].atop  = _atop;      \
  psqs->oqs.port.qu[q].hwm  = _hwm;        \
  psqs->oqs.port.qu[q].lwm  = _lwm;        \
  psqs->oqs.port.qu[q].swm  = _swm;        \
  psqs->oqs.port.qu[q].fwd_press = fwd;    \
  psqs->oqs.port.qu[q].cnt_on_port = cnt_port; \
  psqs->oqs.port.qu[q].cnt_on_prio = cnt_prio; \
  psqs->oqs.port.qu[q].cnt_on_buf_port = cnt_buf_port; \
  psqs->oqs.port.qu[q].cnt_on_buf_prio = 0; \
  psqs->oqs.port.qu[q].rc_mode = rc;        \
  psqs->oqs.port.qu[q].fc_drop_mode = fc;   \
  psqs->oqs.port.qu[q].rx_ena = ena;        \
  psqs->oqs.port.qu[q].mtu_pre_alloc = mtu; \
}

#define OQS_SET_PORT(_atop, _hwm, _lwm, _swm, _cmwm, cnt_buf, ena, rc, fc, _gwm) \
 { \
     psqs->oqs.port.atop        = _atop;  \
     psqs->oqs.port.hwm         = _hwm;   \
     psqs->oqs.port.lwm         = _lwm;   \
     psqs->oqs.port.swm         = _swm;   \
     psqs->oqs.port.gwm         = _gwm;   \
     psqs->oqs.port.cmwm        = _cmwm;  \
     psqs->oqs.port.cnt_on_buf  = cnt_buf;\
     psqs->oqs.port.rx_ena      = ena;    \
     psqs->oqs.port.rc_mode     = rc;     \
     psqs->oqs.port.fc_drop_mode= fc;     \
 }

#define IQS_SET_PORT(_atop, _hwm, _lwm, _swm, cnt_buf, ena, rc, fc, _gwm)  \
 { \
     psqs->iqs.port.atop  = _atop;        \
     psqs->iqs.port.hwm   = _hwm;         \
     psqs->iqs.port.lwm   = _lwm;         \
     psqs->iqs.port.swm   = _swm;         \
     psqs->iqs.port.gwm   = _gwm;   \
     psqs->iqs.port.cnt_on_buf  = cnt_buf;\
     psqs->iqs.port.rx_ena      = ena;    \
     psqs->iqs.port.rc_mode     = rc;     \
     psqs->iqs.port.fc_drop_mode = fc;    \
 }

#define SQS_SET_BUF_WM(fc) \
 { \
     psqs->iqs.hwm  += psqs->iqs.port.hwm * 100 / (fc ? iqs_oversub_fact_fc : iqs_oversub_fact_drop)  - psqs->iqs.port.swm; \
     psqs->iqs.lwm  += psqs->iqs.port.lwm * 100 / (fc ? iqs_oversub_fact_fc : iqs_oversub_fact_drop) - psqs->iqs.port.swm; \
     psqs->iqs.atop += psqs->iqs.port.atop * 100 / (fc ? iqs_oversub_fact_fc : iqs_oversub_fact_drop) - psqs->iqs.port.swm; \
     psqs->oqs.atop -= psqs->iqs.port.atop * 100 / (fc ? iqs_oversub_fact_fc : iqs_oversub_fact_drop); \
     psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; \
     psqs->oqs.lwm  = psqs->oqs.hwm  - 13;      \
     psqs->iqs.rx_ena = 1;                      \
     psqs->oqs.rx_ena = 1;                      \
}

#define QS_QU(x) x
#define QS_ATOP(x) x
#define QS_HWM(x) x
#define QS_LWM(x) x
#define QS_SWM(x) x
#define QS_GWM(x) x
#define QS_FWDP(x) x
#define QS_CMWM(x) x
#define QS_CPORT(x) x
#define QS_CPRIO(x) x
#define QS_CBPORT(x) x
#define QS_ENA(x) x
#define QS_RC(x) x
#define QS_FC(x) (~x&1)
#define QS_MTU_ID(x) x

/* Get target from state */
static vtss_target_type_t jr_target_get(void)
{
    vtss_target_type_t target = vtss_state->create.target;   

    /* Dual-device targets are treated like the corresponding single-device targets */
    if (target == VTSS_TARGET_E_STAX_III_24_DUAL)
        target = VTSS_TARGET_E_STAX_III_48;
    if (target == VTSS_TARGET_E_STAX_III_68_DUAL)
        target = VTSS_TARGET_E_STAX_III_68;
    if (target == VTSS_TARGET_JAGUAR_1_DUAL)
        target = VTSS_TARGET_JAGUAR_1;

    return target;
}

/* 
   For VTSS_TARGET_E_STAX_III_68 and VTSS_TARGET_E_STAX_III_48 (Enterprise):
   - Inititilization (VD and CPU WM are configured)
   - Jumbo mode On/Off
   - Flowcontrol On/Off
   - Port speed: 1G / 10G
   - Port power down / up.   
   For  VTSS_TARGET_JAGUAR_1  VTSS_TARGET_LYNX_1 (Carrier Ethernet):
   - Inititilization (VD and CPU WM are configured)
   - No jumbo and no flowontrol is assumed.
 */

static vtss_rc jr_sqs_calc_wm(u32 chipport, vtss_port_speed_t spd, BOOL jumbo, BOOL flowcontrol, BOOL power_down, BOOL init, sqs_t *psqs)
{
    u32 num_of_1G=0, num_of_10G=0, total_cells=0, oqs_ports=0, prios=0;
    BOOL one_in_drop_mode, vd_active;
    u32 max_frm_cells_normal = 10, max_frm_cells_jumbo  = 61; // Up to 9736 byte;
    u32 iqs_oversub_fact_drop, iqs_oversub_fact_fc, oversub_fact;
    u32 port, tmp, rsrv, q, p, i, oqs_total, mtu = 10;
    u32 hwm, lwm, swm, gwm, atop, cmwm, cnt_on_port, cnt_on_buf, rc;
    vtss_target_type_t target = jr_target_get();
    vtss_port_no_t port_no;
        
    if (target == VTSS_TARGET_JAGUAR_1 || target == VTSS_TARGET_LYNX_1) {
        if (!init) {
            if (flowcontrol || jumbo || power_down) { /* Not supported  */
                return VTSS_RC_ERROR;   
            }
        }
    } else if (target == VTSS_TARGET_CE_MAX_12 || target == VTSS_TARGET_CE_MAX_24) {
        if (!init) {
            if (!flowcontrol || !jumbo) {  /* Not supported  */
                return VTSS_RC_ERROR;   
            }
        }
    }
    VTSS_D("Target:%x chipport:%d spd:%d jumbo:%d fc:%d power_down:%d, init:%d",target, chipport, spd, jumbo, flowcontrol, power_down, init);
    if (spd != VTSS_SPEED_10G && spd != VTSS_SPEED_12G ) {
        spd = VTSS_SPEED_1G;
    }

    switch (target) {
    case VTSS_TARGET_E_STAX_III_48:
    case VTSS_TARGET_LYNX_1:
        total_cells = 13120;
        iqs_oversub_fact_drop = 175;
        iqs_oversub_fact_fc   = 150;
        break;
    case VTSS_TARGET_E_STAX_III_68:
    case VTSS_TARGET_JAGUAR_1:
        total_cells = 26240; /* cell=160B */
        iqs_oversub_fact_drop = 125;
        iqs_oversub_fact_fc   = 100;
        break;
    case VTSS_TARGET_CE_MAX_12:        
    case VTSS_TARGET_CE_MAX_24:
        iqs_oversub_fact_drop = 125;
        iqs_oversub_fact_fc   = 100;
        if ((ce_max_hm() == 0) || (ce_max_hm() == 2)) {
            oqs_ports = 56;
            prios = 8;
        } else {
            oqs_ports = 80;
            prios = 4;
        }
        break;
    default:
        VTSS_E("Target not supported");
        return VTSS_RC_ERROR;
    }
    
    for (p = VTSS_PORT_NO_START; p < vtss_state->port_count; p++) {
        if (!VTSS_PORT_CHIP_SELECTED(p)) {
            continue;
        }
        if (VTSS_PORT_IS_10G(VTSS_CHIP_PORT(p))) {
            num_of_10G++;
        } else {
            num_of_1G++;
        }
    }

    if (vtss_state->chip_count == 2) {
        /* Include internal ports */
        num_of_10G += 2;
    }
    vd_active = 0; /* Virtual device currently not used */
    
    if (init) {
        /* ======================== */
        /* ======== Init ========== */
        /* ======================== */
        // Initilize. Set up all the stuff which will not change based on jumbo/flowcontrol/power.             
        if (chipport == 0) {
            for (i = 0; i < 4; i++) {
                psqs->iqs.pre_alloc_frm_size[i] = (i == 1) ? max_frm_cells_jumbo : max_frm_cells_normal; 
                psqs->oqs.pre_alloc_frm_size[i] = (i == 1) ? max_frm_cells_jumbo : max_frm_cells_normal; 
            }
            if (target == VTSS_TARGET_CE_MAX_24 || target == VTSS_TARGET_CE_MAX_12) {
                /* Supporting CE-MAX WM for Jumbo / flowcontrol mode */
                psqs->iqs.rx_ena = 1;                                 
                psqs->oqs.rx_ena = 1;                                 
                psqs->iqs.atop = 7140; /* 8922 cells @ 1.25 oversubsrciption */
                psqs->iqs.hwm  = 0;
                psqs->iqs.lwm  = 0;
                psqs->oqs.atop = 12080;
                psqs->oqs.hwm  = 11969;
                psqs->oqs.lwm  = 11956;
            } else {
                psqs->iqs.atop = 0;
                psqs->iqs.hwm  = 0;
                psqs->iqs.lwm  = 0;
                psqs->iqs.fc_drop_mode  = 0;                    
                psqs->oqs.fc_drop_mode  = 0;
                psqs->oqs.atop = total_cells;
                psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
                psqs->oqs.lwm  = psqs->oqs.hwm  - 13;
            }
        } 
        psqs->iqs.port.rx_ena = 0;

        /* === Virtual device = 32 ====== */        
        if (chipport == 32 && vd_active) {
            for (q = 0; q < 8; q++) {
                IQS_SET_QU(QS_QU(q), QS_ATOP(100),  QS_HWM(32), QS_LWM(27), QS_SWM(0), QS_FWDP(100), 
                           QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
            }
            IQS_SET_PORT(QS_ATOP(100), QS_HWM(32), QS_LWM(31), QS_SWM(10), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(1), QS_GWM(0));  

            tmp = psqs->iqs.port.hwm * 100 / iqs_oversub_fact_drop; 
            psqs->iqs.hwm  += tmp - psqs->iqs.port.swm;
            tmp = psqs->iqs.port.lwm * 100 / iqs_oversub_fact_drop; 
            psqs->iqs.lwm  += tmp - psqs->iqs.port.swm;
            tmp = psqs->iqs.port.atop * 100 / iqs_oversub_fact_drop;
            psqs->iqs.atop += tmp - psqs->iqs.port.swm;
             
            psqs->oqs.atop -= tmp;
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;                        
        }
        
        /* === VD OQS === */
        if (chipport == 80 && vd_active) {
            for (q = 0; q < 8; q++) {      
                OQS_SET_QU(QS_QU(q), QS_ATOP(460), QS_HWM(400), QS_LWM(387), QS_SWM(15), QS_FWDP(0), 
                           QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                psqs->oqs.atop -= psqs->oqs.port.qu[q].swm;
            }
            
            if (target == VTSS_TARGET_E_STAX_III_48 || target == VTSS_TARGET_LYNX_1) {
                OQS_SET_PORT(QS_ATOP(660), QS_HWM(600), QS_LWM(587), QS_SWM(0), QS_CMWM(999), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));
            } else {
                OQS_SET_PORT(QS_ATOP(860), QS_HWM(800), QS_LWM(787), QS_SWM(0), QS_CMWM(999), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
            }        
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;      
        }
        /* === IQS CPU ports 33-34 ====== */            
        if (chipport >= 33 &&  chipport < 35) {
            for (q = 0; q < 8; q++) {       
                IQS_SET_QU(QS_QU(q), QS_ATOP(80),  QS_HWM(18), QS_LWM(15), QS_SWM(0), QS_FWDP(80), 
                           QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
            }
            IQS_SET_PORT(QS_ATOP(80), QS_HWM(18), QS_LWM(15), QS_SWM(10), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(1), QS_GWM(0));  

            tmp = psqs->iqs.port.hwm * 100 / iqs_oversub_fact_drop; 
            psqs->iqs.hwm  += tmp - psqs->iqs.port.swm;
            tmp = psqs->iqs.port.lwm * 100 / iqs_oversub_fact_drop;
            psqs->iqs.lwm  += tmp - psqs->iqs.port.swm;
            tmp = psqs->iqs.port.atop * 100 / iqs_oversub_fact_drop; 
            psqs->iqs.atop += tmp - psqs->iqs.port.swm;
            psqs->oqs.atop -= tmp;
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;
        }

        /* === OQS HOST ports 32-79 ====== */            
        if (target == VTSS_TARGET_CE_MAX_12 || target == VTSS_TARGET_CE_MAX_24) {
            if (chipport > 31 && chipport < oqs_ports) { 
                rsrv = 0;
                for (q = 0; q < prios; q++) {                    
                    if (q < (prios - 2)) {
                        /* Service qu WM */
                        OQS_SET_QU(QS_QU(q), QS_ATOP(131),  QS_HWM(20), QS_LWM(20), QS_SWM(20), QS_FWDP(15), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                    } else {
                        /* Control qu WM */
                        OQS_SET_QU(QS_QU(q), QS_ATOP(131),  QS_HWM(20), QS_LWM(20), QS_SWM(20), QS_FWDP(15), 
                                   QS_CPORT(0), QS_CPRIO(0), QS_CBPORT(0), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                    }
                    rsrv += 20;
                }
                OQS_SET_PORT(QS_ATOP(191), QS_HWM(80), QS_LWM(80), QS_SWM(0), QS_CMWM(75), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(200));  
                psqs->oqs.atop -= rsrv;
                psqs->oqs.hwm  -= rsrv;
                psqs->oqs.lwm  -= rsrv;
            }            
        }

        /* === OQS CPU ports 81-88 ====== */            
        if (chipport >= 81 &&  chipport < 89) {
            if (target == VTSS_TARGET_E_STAX_III_48 || target == VTSS_TARGET_LYNX_1) {
                atop = 460;
                hwm  = 400;
                lwm  = 387;
            } else {
                atop = 660;
                hwm  = 600;
                lwm  = 587;
            }
            OQS_SET_QU(QS_QU(0), QS_ATOP(atop),  QS_HWM(hwm), QS_LWM(lwm), QS_SWM(15), QS_FWDP(0), 
                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
            psqs->oqs.atop -= psqs->oqs.port.qu[0].swm;
            OQS_SET_PORT(QS_ATOP(atop), QS_HWM(hwm), QS_LWM(lwm), QS_SWM(0), QS_CMWM(999), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
        }        

        if (chipport == 88) { 
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10;
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;
        }

    } else {
        /* ====================== */
        /* =====  Port down ===== */
        /* ====================== */
        /* Power down a port. It's space goes to the shared pool */
        if (power_down) {
            psqs->iqs.port.rx_ena = 0;       
            oversub_fact = (psqs->iqs.port.fc_drop_mode == 0) ? iqs_oversub_fact_fc : iqs_oversub_fact_drop;             
            psqs->iqs.hwm  -= psqs->iqs.port.hwm * 100 / oversub_fact - psqs->iqs.port.swm; 
            psqs->iqs.lwm  -= psqs->iqs.port.lwm * 100 / oversub_fact  - psqs->iqs.port.swm;
            psqs->iqs.atop -= psqs->iqs.port.atop * 100 / oversub_fact - psqs->iqs.port.swm;
            psqs->oqs.atop += psqs->iqs.port.atop * 100 / oversub_fact;

            // Check if a port is left that runs drop mode. If not, switch buffer to FC mode
            if (psqs->iqs.port.fc_drop_mode == 1) {
                one_in_drop_mode = 0;
                for (port = 0; port < vtss_state->port_count; port++) {                    
                    if ((vtss_state->port_conf[port].power_down == 0) && vtss_state->port_conf[port].flow_control.obey == 0) {
                        one_in_drop_mode = 1;
                        break;
                    }
                }
                if (!one_in_drop_mode) {
                    psqs->iqs.fc_drop_mode = 0;
                }                
            }
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;
            for (q = 0; q < 8; q++) {
                psqs->oqs.atop += psqs->oqs.port.qu[q].swm;
            }
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;

        } else if ((jumbo) && (flowcontrol)) {
            /* ============================ */
            /* === JUMBO and FLOWONTROL === */
            /* ============================ */           
            /* ============= IQS ============== */     
            
            if (target == VTSS_TARGET_CE_MAX_24 || target == VTSS_TARGET_CE_MAX_12) {
                rsrv = 0;
                for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
                    if (vtss_state->port_map[port_no].chip_port == chipport)
                        break;

                if (cemax_port_is_host(port_no)) {  /* Host Interface port IQS/OQS */
                    /* IQS */
                    IQS_SET_PORT(QS_ATOP(417), QS_HWM(200), QS_LWM(180), QS_SWM(0), QS_CBPORT(1), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(430));  
                    for (q = 0; q < prios; q++) {
                        IQS_SET_QU(QS_QU(q), QS_ATOP(160),  QS_HWM(150), QS_LWM(150), QS_SWM(0), QS_FWDP(2), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                    }
                    hwm = 160; /* 8x10 cells */
                    lwm = 160; /* 8x10 cells */
                    /* No OQS config needed for HI */
                } else if (spd == VTSS_SPEED_1G)  { /* 1G Line port IQS/OQS */
                    hwm = 160; 
                    lwm = 140; 
                    IQS_SET_PORT(QS_ATOP(307), QS_HWM(hwm), QS_LWM(lwm), QS_SWM(0), QS_CBPORT(1), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(320));  
                    OQS_SET_PORT(QS_ATOP(421), QS_HWM(310), QS_LWM(297), QS_SWM(0), QS_CMWM(180), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(430));  
                    for (q = 0; q < prios; q++) {
                        if (q < (prios-2)) {         /* IQS Service qu WM for 1G */                    
                            IQS_SET_QU(QS_QU(q), QS_ATOP(307),  QS_HWM(160), QS_LWM(160), QS_SWM(0), QS_FWDP(0), 
                                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));

                                                     /* OQS Service qu WM for 1G */ 
                            OQS_SET_QU(QS_QU(q), QS_ATOP(211),  QS_HWM(100), QS_LWM(87), QS_SWM(15), QS_FWDP(20), 
                                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                            rsrv += 15;
                        } else {                     /* IQS Control qu WM for 1G */ 
                            IQS_SET_QU(QS_QU(q), QS_ATOP(130),  QS_HWM(65), QS_LWM(65), QS_SWM(0), QS_FWDP(0), 
                                       QS_CPORT(0), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                                                     /* OQS Control qu WM for 1G */ 
                            OQS_SET_QU(QS_QU(q), QS_ATOP(131),  QS_HWM(20), QS_LWM(7), QS_SWM(20), QS_FWDP(20), 
                                       QS_CPORT(0), QS_CPRIO(0), QS_CBPORT(0), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                            rsrv += 20;
                        }
                    }
                } else {     
                    /* 10G Line port IQS/OQS */
                    hwm = 480; 
                    lwm = 440; 
                    IQS_SET_PORT(QS_ATOP(697), QS_HWM(hwm), QS_LWM(lwm), QS_SWM(0), QS_CBPORT(1), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(720));  
                    OQS_SET_PORT(QS_ATOP(531), QS_HWM(420), QS_LWM(407), QS_SWM(0), QS_CMWM(290), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(540));  
                    for (q = 0; q < prios; q++) {
                        if (q < (prios-2)) {         /* Service qu WM */                    
                            IQS_SET_QU(QS_QU(q), QS_ATOP(697),  QS_HWM(480), QS_LWM(480), QS_SWM(0), QS_FWDP(0), 
                                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));

                            OQS_SET_QU(QS_QU(q), QS_ATOP(261),  QS_HWM(150), QS_LWM(137), QS_SWM(30), QS_FWDP(70), 
                                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));

                            rsrv += 30;
                        } else {                     /* Control qu WM */
                            IQS_SET_QU(QS_QU(q), QS_ATOP(195),  QS_HWM(130), QS_LWM(130), QS_SWM(0), QS_FWDP(0), 
                                       QS_CPORT(0), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));

                            OQS_SET_QU(QS_QU(q), QS_ATOP(151),  QS_HWM(40), QS_LWM(27), QS_SWM(40), QS_FWDP(70), 
                                       QS_CPORT(0), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                            rsrv += 40;
                        }
                    }
                } 
                psqs->iqs.hwm  += (u32)(hwm - (hwm/5)); /* To 80% - no floats */
                psqs->iqs.lwm  += (u32)(lwm - (lwm/5)); /* To 80% - no floats */

                psqs->oqs.atop -= rsrv; 
                psqs->oqs.hwm  -= rsrv; 
                psqs->oqs.lwm  -= rsrv; 
                return VTSS_RC_OK;    /* Nothing more todo for CE-MAX */
                
            } else {
                if (target == VTSS_TARGET_E_STAX_III_48) {
                    if (spd == VTSS_SPEED_1G) {
                        atop = 327;
                        hwm  = 183;
                        lwm  = 61;
                        swm  = 40;
                    } else {
                        atop = 671;
                        hwm  = 305;
                        lwm  = 122;
                        swm  = 80;
                    }
                } else {
                    if (spd == VTSS_SPEED_1G) {
                        atop = 449;
                        hwm  = 305;
                        lwm  = 183;
                        swm  = 122;
                    } else {
                        atop = 854;
                        hwm  = 488;
                        lwm  = 305;
                        swm  = 183;
                    }
                }
                IQS_SET_PORT(QS_ATOP(atop), QS_HWM(hwm), QS_LWM(lwm), QS_SWM(swm), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(1), QS_GWM(0));  
                for (q = 0; q < 8; q++) {
                    IQS_SET_QU(QS_QU(q), QS_ATOP(atop),  QS_HWM(hwm), QS_LWM(lwm), QS_SWM(0), QS_FWDP(atop), 
                               QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                }
                SQS_SET_BUF_WM(flowcontrol);
            }
            
            /* ============= OQS ============== */     
            for (q = 0; q < 8; q++) {
                if (target == VTSS_TARGET_E_STAX_III_48 || target == VTSS_TARGET_LYNX_1) {
                    if (spd == VTSS_SPEED_1G) {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(294),  QS_HWM(183), QS_LWM(170), QS_SWM(15), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                    } else {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(416),  QS_HWM(305), QS_LWM(292), QS_SWM(30), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                    }
                } else {
                    if (spd == VTSS_SPEED_1G) {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(416),  QS_HWM(305), QS_LWM(292), QS_SWM(15), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                    } else {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(904),  QS_HWM(793), QS_LWM(780), QS_SWM(30), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                    }
                }
                psqs->oqs.atop -= psqs->oqs.port.qu[q].swm;
            }
            
            if (target == VTSS_TARGET_E_STAX_III_48) {
                if (spd == VTSS_SPEED_1G) {
                    OQS_SET_PORT(QS_ATOP(355), QS_HWM(244), QS_LWM(231), QS_SWM(0), QS_CMWM(70), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                } else {
                    OQS_SET_PORT(QS_ATOP(477), QS_HWM(366), QS_LWM(353), QS_SWM(0), QS_CMWM(153), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                }
            } else {
                if (spd == VTSS_SPEED_1G) {
                    OQS_SET_PORT(QS_ATOP(599), QS_HWM(488), QS_LWM(474), QS_SWM(0), QS_CMWM(100), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                } else {
                    OQS_SET_PORT(QS_ATOP(1087), QS_HWM(976), QS_LWM(963), QS_SWM(0), QS_CMWM(250), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                }
            }          
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;
        } else if ((!jumbo) && (flowcontrol)) {
            /* ================================ */
            /* === NOT JUMBO and FLOWONTROL === */
            /* ================================ */           
                
            /* ============= IQS ============== */           
            if (spd == VTSS_SPEED_1G) {
                atop = 125;
                hwm  = 80;
                lwm  = 50;
                swm  = 20;
            } else {
                atop = 275;
                hwm  = 160;
                lwm  = 130;
                swm  = 30;
            }
            IQS_SET_PORT(QS_ATOP(atop), QS_HWM(hwm), QS_LWM(lwm), QS_SWM(swm), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(1), QS_GWM(0));  
            for (q = 0; q < 8; q++) {
                IQS_SET_QU(QS_QU(q), QS_ATOP(atop),  QS_HWM(hwm), QS_LWM(lwm), QS_SWM(0), QS_FWDP(atop), 
                           QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
            }

            SQS_SET_BUF_WM(flowcontrol);

            /* ============= OQS ============== */           
            for (q = 0; q < 8; q++) {
                if (target == VTSS_TARGET_E_STAX_III_48) {
                    if (spd == VTSS_SPEED_1G) {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(560),  QS_HWM(500), QS_LWM(487), QS_SWM(15), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                    } else {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(1060),  QS_HWM(1000), QS_LWM(987), QS_SWM(30), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                    }
                } else {
                    if (spd == VTSS_SPEED_1G) {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(660),  QS_HWM(600), QS_LWM(587), QS_SWM(15), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                    } else {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(1260),  QS_HWM(1200), QS_LWM(1187), QS_SWM(30), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                    }
                } 
                psqs->oqs.atop -= psqs->oqs.port.qu[q].swm;
            }
            if (target == VTSS_TARGET_E_STAX_III_48) {
                if (spd == VTSS_SPEED_1G) {
                    OQS_SET_PORT(QS_ATOP(760), QS_HWM(700), QS_LWM(687), QS_SWM(0), QS_CMWM(250), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                } else {
                    OQS_SET_PORT(QS_ATOP(1460), QS_HWM(1400), QS_LWM(1387), QS_SWM(0), QS_CMWM(500), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                }
            } else {
                if (spd == VTSS_SPEED_1G) {
                    OQS_SET_PORT(QS_ATOP(860), QS_HWM(800), QS_LWM(787), QS_SWM(0), QS_CMWM(300), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                } else {
                    OQS_SET_PORT(QS_ATOP(1660), QS_HWM(1600), QS_LWM(1587), QS_SWM(0), QS_CMWM(600), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                }
            }  
            
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;

        } else if ((jumbo) && (!flowcontrol)) {
            /* ================================ */
            /* === JUMBO and NOT FLOWONTROL === */
            /* ================================ */

            /* ============= IQS ============== */           
            if (target == VTSS_TARGET_E_STAX_III_48) {
                if (spd == VTSS_SPEED_1G) {
                    IQS_SET_PORT(QS_ATOP(427), QS_HWM(366), QS_LWM(366), QS_SWM(61), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(0), QS_GWM(0));  
                } else {
                    IQS_SET_PORT(QS_ATOP(671), QS_HWM(610), QS_LWM(610), QS_SWM(122), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(0), QS_GWM(0));  
                }
            } else {
                if (spd == VTSS_SPEED_1G) {
                    IQS_SET_PORT(QS_ATOP(488), QS_HWM(427), QS_LWM(427), QS_SWM(122), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(0), QS_GWM(0));  
                } else {
                    IQS_SET_PORT(QS_ATOP(793), QS_HWM(732), QS_LWM(732), QS_SWM(244), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(0), QS_GWM(0));  
                }
            }  

            for (q = 0; q < 8; q++) {
                if (target == VTSS_TARGET_E_STAX_III_48) {
                    if (spd == VTSS_SPEED_1G) {
                        IQS_SET_QU(QS_QU(q), QS_ATOP(244),  QS_HWM(184), QS_LWM(183), QS_SWM(0), QS_FWDP(45), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(0), QS_ENA(1), QS_MTU_ID(1));
                    } else {
                        IQS_SET_QU(QS_QU(q), QS_ATOP(444),  QS_HWM(427), QS_LWM(427), QS_SWM(0), QS_FWDP(60), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(0), QS_ENA(1), QS_MTU_ID(1));
                    }
                } else {
                    if (spd == VTSS_SPEED_1G) {
                        IQS_SET_QU(QS_QU(q), QS_ATOP(305),  QS_HWM(244), QS_LWM(244), QS_SWM(0), QS_FWDP(40), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(0), QS_ENA(1), QS_MTU_ID(1));
                    } else {
                        IQS_SET_QU(QS_QU(q), QS_ATOP(610),  QS_HWM(549), QS_LWM(549), QS_SWM(0), QS_FWDP(70),
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(0), QS_ENA(1), QS_MTU_ID(1));

                    }
                }                                 
            }            

            SQS_SET_BUF_WM(flowcontrol);
            psqs->iqs.fc_drop_mode = 1; /* dropmode */           
            // As soon as one port runs drop mode, set IQS buffer to drop mode
            /* ============= OQS ============== */           
            for (q = 0; q < 8; q++) {
                if (target == VTSS_TARGET_E_STAX_III_48 || target == VTSS_TARGET_LYNX_1) {
                    if (spd == VTSS_SPEED_1G) {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(460),  QS_HWM(349), QS_LWM(336), QS_SWM(15), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                    } else {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(870),  QS_HWM(759), QS_LWM(746), QS_SWM(30), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                    }
                } else {
                    if (spd == VTSS_SPEED_1G) {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(660),  QS_HWM(549), QS_LWM(536), QS_SWM(15), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                    } else {
                        OQS_SET_QU(QS_QU(q), QS_ATOP(1270),  QS_HWM(1159), QS_LWM(1146), QS_SWM(30), QS_FWDP(0), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(1));
                    }
                }                
                psqs->oqs.atop -= psqs->oqs.port.qu[q].swm;
            }
            if (target == VTSS_TARGET_E_STAX_III_48) {
                if (spd == VTSS_SPEED_1G) {
                    OQS_SET_PORT(QS_ATOP(521), QS_HWM(410), QS_LWM(397), QS_SWM(0), QS_CMWM(150), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                } else {
                    OQS_SET_PORT(QS_ATOP(931), QS_HWM(820), QS_LWM(807), QS_SWM(0), QS_CMWM(300), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                }
            } else {
                if (spd == VTSS_SPEED_1G) {
                    OQS_SET_PORT(QS_ATOP(721), QS_HWM(610), QS_LWM(597), QS_SWM(0), QS_CMWM(250), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                } else {
                    OQS_SET_PORT(QS_ATOP(1331), QS_HWM(1220), QS_LWM(1207), QS_SWM(0), QS_CMWM(500), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                }
            }  
            psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
            psqs->oqs.lwm  = psqs->oqs.hwm  - 13;

        } else  {
            /* ==================================== */
            /* === NOT JUMBO and NOT FLOWONTROL === */
            /* ==================================== */

            /* ============= IQS ============== */           
            if (target == VTSS_TARGET_JAGUAR_1 || target == VTSS_TARGET_LYNX_1) {
                hwm = (spd == VTSS_SPEED_1G) ? 30 : 40;
                for (q = 0; q < 8; q++) { 
                    /* Queues 0-5 (Service traffic) Queues 6-7 (Control traffic)*/
                    IQS_SET_QU(QS_QU(q), QS_ATOP((hwm+mtu)),  QS_HWM(hwm), QS_LWM(hwm), QS_SWM(0), QS_FWDP(2), 
                               QS_CPORT((q < 6 ? 1 : 0)), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                }
                
                hwm = (spd == VTSS_SPEED_1G) ? 180 : 240;
                gwm = hwm + mtu;
                atop = gwm + mtu;
                /* ============= IQS PORT ============== */           
                IQS_SET_PORT(QS_ATOP(atop), QS_HWM(hwm), QS_LWM(hwm), QS_SWM(0), QS_CBPORT(1), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(gwm));  

                /* ============= IQS BUF ============== */           
                psqs->iqs.rx_ena = 1;            
                psqs->iqs.fc_drop_mode = 0;            
                psqs->iqs.hwm = num_of_1G * 180 + num_of_10G * 240;
                psqs->iqs.lwm = psqs->iqs.hwm;
                psqs->iqs.atop = (num_of_1G * (180 + 2*mtu)) + (num_of_10G * (240 + 2*mtu));
        
                /* === OQS === */
                oqs_total = total_cells - psqs->iqs.atop - (num_of_1G*2*30 + num_of_10G*2*40);                     
                for (q = 0; q < 8; q++) { 
                    if (q < 6) { /* Queues 0-5 (Service traffic) */
                        rc = 1;     
                        atop = (spd == VTSS_SPEED_1G) ? 290 : 390;
                        hwm = (spd == VTSS_SPEED_1G) ? 230 : 330;
                        lwm = hwm - 13;
                        swm = (spd == VTSS_SPEED_1G) ? 15 : 30;
                        cnt_on_port = 1;           
                        cnt_on_buf = 1; 
                    } else {
                        rc = 0;     
                        atop = (spd == VTSS_SPEED_1G) ? 180 : 220;
                        hwm = (spd == VTSS_SPEED_1G) ? 12*mtu : 16*mtu;
                        lwm = hwm - 13;
                        swm = hwm;
                        cnt_on_port = 0;   
                        cnt_on_buf = 0;
                    }
                        
                    OQS_SET_QU(QS_QU(q), QS_ATOP(atop),  QS_HWM(hwm), QS_LWM(lwm), QS_SWM(swm), QS_FWDP(0), 
                               QS_CPORT(cnt_on_port), QS_CPRIO(0), QS_CBPORT(cnt_on_buf), QS_RC(rc), QS_FC(0), QS_ENA(1), QS_MTU_ID(0));
                }

                /* Buf Port level - OQS */
                /* ==================== */
                psqs->oqs.rx_ena = 1;            
                psqs->oqs.fc_drop_mode = 1; /* Flowcontrol to IQS */
                psqs->oqs.atop = oqs_total - num_of_1G*6*15 - num_of_1G*2*120 -  num_of_10G*6*30 - num_of_10G*2*160;
                psqs->oqs.hwm = psqs->oqs.atop - 50 - mtu;
                psqs->oqs.lwm = psqs->oqs.hwm - 13;
                
                /* Port level - OQS */
                /* ================ */
                hwm = (spd == VTSS_SPEED_1G) ? 600 : 800;
                atop = hwm + 50 + mtu;
                lwm = hwm - 13;
                gwm = (spd == VTSS_SPEED_1G) ? 620 : 820;
                cmwm = (spd == VTSS_SPEED_1G) ? 200 : 270;
                OQS_SET_PORT(QS_ATOP(atop), QS_HWM(hwm), QS_LWM(lwm), QS_SWM(0), QS_CMWM(cmwm), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(0), QS_GWM(gwm));  
            } else {
                if (spd == VTSS_SPEED_1G) {
                    IQS_SET_PORT(QS_ATOP(250), QS_HWM(240), QS_LWM(240), QS_SWM(20), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(0), QS_GWM(0));  
                } else {
                    IQS_SET_PORT(QS_ATOP(490), QS_HWM(480), QS_LWM(480), QS_SWM(40), QS_CBPORT(1), QS_ENA(1), QS_RC(1), QS_FC(0), QS_GWM(0));         
                }
                for (q = 0; q < 8; q++) {
                    if (spd == VTSS_SPEED_1G) {
                        IQS_SET_QU(QS_QU(q), QS_ATOP(60),  QS_HWM(50), QS_LWM(50), QS_SWM(0), QS_FWDP(30), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(0), QS_ENA(1), QS_MTU_ID(0));
                    } else {
                        IQS_SET_QU(QS_QU(q), QS_ATOP(110),  QS_HWM(100), QS_LWM(100), QS_SWM(0), QS_FWDP(60), 
                                   QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(0), QS_RC(0), QS_FC(0), QS_ENA(1), QS_MTU_ID(0));
                    }
                }
                SQS_SET_BUF_WM(flowcontrol); 
                psqs->iqs.fc_drop_mode = 1;
                // As soon as one port runs drop mode, set IQS buffer to drop mode
                /* ============= OQS ============== */           
                for (q = 0; q < 8; q++) {
                    if (target == VTSS_TARGET_E_STAX_III_48) {
                        if (spd == VTSS_SPEED_1G) {
                            OQS_SET_QU(QS_QU(q), QS_ATOP(560),  QS_HWM(500), QS_LWM(487), QS_SWM(15), QS_FWDP(0), 
                                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                        } else {
                            OQS_SET_QU(QS_QU(q), QS_ATOP(1060),  QS_HWM(1000), QS_LWM(987), QS_SWM(30), QS_FWDP(0), 
                                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                        }
                    } else {
                        if (spd == VTSS_SPEED_1G) {
                            OQS_SET_QU(QS_QU(q), QS_ATOP(660),  QS_HWM(600), QS_LWM(587), QS_SWM(15), QS_FWDP(0), 
                                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                        } else {
                            OQS_SET_QU(QS_QU(q), QS_ATOP(1260),  QS_HWM(1200), QS_LWM(1187), QS_SWM(30), QS_FWDP(0), 
                                       QS_CPORT(1), QS_CPRIO(0), QS_CBPORT(1), QS_RC(1), QS_FC(1), QS_ENA(1), QS_MTU_ID(0));
                        }
                    }  
                    psqs->oqs.atop -= psqs->oqs.port.qu[q].swm;                    
                }
                if (target == VTSS_TARGET_E_STAX_III_48) {
                    if (spd == VTSS_SPEED_1G) {
                        OQS_SET_PORT(QS_ATOP(760), QS_HWM(700), QS_LWM(687), QS_SWM(0), QS_CMWM(250), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                    } else {
                        OQS_SET_PORT(QS_ATOP(1460), QS_HWM(1400), QS_LWM(1387), QS_SWM(0), QS_CMWM(500), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                    }
                } else {
                    if (spd == VTSS_SPEED_1G) {
                        OQS_SET_PORT(QS_ATOP(860), QS_HWM(800), QS_LWM(787), QS_SWM(0), QS_CMWM(300), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                    } else {
                        OQS_SET_PORT(QS_ATOP(1660), QS_HWM(1600), QS_LWM(1587), QS_SWM(0), QS_CMWM(600), QS_CBPORT(0), QS_ENA(1), QS_RC(0), QS_FC(1), QS_GWM(0));  
                    }
                }  
                psqs->oqs.hwm  = psqs->oqs.atop - 50 - 10; 
                psqs->oqs.lwm  = psqs->oqs.hwm  - 13;        
            } // VTSS_TARGET_E_STAX_III_48 ||  VTSS_TARGET_E_STAX_III_68
        }
    }
    return VTSS_RC_OK;    
}


/* Serdes6G: Read data from MCB slave back to MCB master (CSR target) */
static vtss_rc jr_sd6g_read(u32 addr) 
{
    /* Read back data from MCB slave to MCB master (CSR target) */
    JR_WR(MACRO_CTRL, MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG,
           VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_ADDR(addr) | 
           VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_RD_ONE_SHOT);
       
    /* Wait until read operation is completed  */
    JR_POLL_BIT(MACRO_CTRL, MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG, SERDES6G_RD_ONE_SHOT);

    return VTSS_RC_OK;    
}


/* Serdes1G: Read data from  MCB slave back to MCB master (CSR target) */
static vtss_rc jr_sd1g_read(u32 addr) 
{
    /* Read back data from MCB slave to MCB master (CSR target) */
    JR_WR(MACRO_CTRL, MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG,
           VTSS_F_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_ADDR(addr) | 
           VTSS_F_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_RD_ONE_SHOT);
       
    /* Wait until read operation is completed  */
    JR_POLL_BIT(MACRO_CTRL, MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG, SERDES1G_RD_ONE_SHOT);

    return VTSS_RC_OK;
}


/* Serdes6G: Transfers data from MCB master (CSR target) to MCB slave */
static vtss_rc jr_sd6g_write(u32 addr, u32 nsec) 
{
    JR_WR(MACRO_CTRL, MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG,
          VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_ADDR(addr) |
          VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_WR_ONE_SHOT);

    /* Wait until write operation is completed  */
    JR_POLL_BIT(MACRO_CTRL, MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG, SERDES6G_WR_ONE_SHOT);
    
    if (nsec)
        VTSS_NSLEEP(nsec);

    return VTSS_RC_OK;
}

/* Serdes1G: Transfers data from MCB master (CSR target) to MCB slave */
static vtss_rc jr_sd1g_write(u32 addr, u32 nsec) 
{
    JR_WR(MACRO_CTRL, MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG,
          VTSS_F_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_ADDR(addr) |
          VTSS_F_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_WR_ONE_SHOT);

    /* Wait until write operation is completed  */
    JR_POLL_BIT(MACRO_CTRL, MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG, SERDES1G_WR_ONE_SHOT);

    if (nsec)
        VTSS_NSLEEP(nsec);

    return VTSS_RC_OK;
}

/* Serdes6G setup */
#define RCOMP_CFG0 VTSS_IOREG(VTSS_TO_HSIO,0x8)
static vtss_rc jr_sd6g_cfg(vtss_serdes_mode_t mode, u32 addr)
{
    /* Initialize parameters to default values */
    u32  ob_post0 = 0, ob_prec = 0, ob_lev = 48, ob_ena_cas = 0, ob_pol = 1;
    u32  ib_ic_ac = 0, ib_rf = 15, ib_vbac = 0, ib_vbcom = 0, ib_c = 0, ib_chf = 0;
    BOOL ob_ena1v = 0, ib_dis_eq = 1, if_100fx = 0, ser_4tab_ena = 0, ser_enali = 0, ser_alisel = 0;
    BOOL div4 = 0, ena_rot = 0, ena_lane = 1, qrate = 1, hrate = 0,  rot_dir = 0;
    u32  ctrl_data = 60, if_mode = 1, rcomp_val, des_mbtr_ctrl = 0, des_cpmd_sel = 0;

    ob_ena1v = (vtss_state->init_conf.serdes.serdes6g_vdd == VTSS_VDD_1V0) ? 1 : 0;

    /* Override default values depending on mode */
    switch (mode) {
    case VTSS_SERDES_MODE_RXAUI:
        if (ob_ena1v) {
            /* 1.0 V */
            ob_prec = 5; /* OB_CFG */
            ob_ena1v = 1;/* OB_CFG */
        } else {
            /* 1.2 V */
            ob_post0 = 11;/* OB_CFG */
            ob_prec = 8; /* OB_CFG */
            ob_lev = 63; /* OB_CFG */
        }
        ib_ic_ac = 2;    /* IB_CFG */
        ib_rf = 2;       /* IB_CFG */
        ib_vbac = 4;     /* IB_CFG */
        ib_vbcom = 5;    /* IB_CFG */
        ib_c = 6;        /* IB_CFG */
        ib_chf = 1;      /* IB_CFG */
        ib_dis_eq = 1;   /* IB_CFG1 */
        ena_rot = 1;     /* PLL_CFG */
        ctrl_data = 96;  /* PLL_CFG */
        if_mode = 3;     /* COMMON_CFG */
        qrate = 0;       /* COMMON_CFG */ 
        ser_4tab_ena = 1;/* SER_CFG */
        ser_enali = 1;   /* SER_CFG */
        ser_alisel = 1;  /* SER_CFG */
        break;
    case VTSS_SERDES_MODE_XAUI:
    case VTSS_SERDES_MODE_2G5:
        ob_post0 = 2;    /* OB_CFG */
        ob_prec = 2;     /* OB_CFG */
        ib_ic_ac = 2;    /* IB_CFG */
        ib_rf = 2;       /* IB_CFG */
        ib_vbac = 4;     /* IB_CFG */
        ib_vbcom = 5;    /* IB_CFG */
        ib_c = 6;        /* IB_CFG */
        ib_chf = 1;      /* IB_CFG */
        ib_dis_eq = 1;   /* IB_CFG1 */
        ena_rot = 1;     /* PLL_CFG */
        ctrl_data = 48;  /* PLL_CFG */
        qrate = 0;       /* COMMON_CFG */
        hrate = 1;       /* COMMON_CFG */
        ser_4tab_ena = 1;/* SER_CFG */
        ser_enali = 1;   /* SER_CFG */
        ser_alisel = 1;  /* SER_CFG */
        break;
    case VTSS_SERDES_MODE_XAUI_12G:
        ib_rf = 8;       /* IB_CFG */
        ib_c = 8;        /* IB_CFG */
        ib_dis_eq = 1;   /* IB_CFG1 */
        ena_rot = 1;     /* PLL_CFG */
        ctrl_data = 80;  /* PLL_CFG */
        div4 = 0;        /* PLL_CFG */
        rot_dir = 1;     /* PLL_CFG */
        qrate = 0;       /* COMMON_CFG */
        ser_enali = 1;   /* SER_CFG */
        ser_alisel = 1;  /* SER_CFG */
        break;
    case VTSS_SERDES_MODE_SGMII:
        ob_ena_cas = 2;  /* OB_CFG */
        ib_c = 15;       /* IB_CFG */
        ib_dis_eq = 1;   /* IB_CFG1 */
        break;
    case VTSS_SERDES_MODE_100FX:
        ib_c = 15;       /* IB_CFG */
        if_100fx = 1;    /* IB_CFG1 */
        ib_dis_eq = 1;   /* IB_CFG1 */
        des_mbtr_ctrl = 3;/* DES_CFG */
        des_cpmd_sel = 2; /* DES_CFG */
        break;
    case VTSS_SERDES_MODE_1000BaseX:
        ob_ena_cas = 2;  /* OB_CFG1 */
        ib_c = 15;       /* IB_CFG */
        ib_dis_eq = 1;   /* IB_CFG1 */
        break;
    case VTSS_SERDES_MODE_DISABLE:
        ena_lane = 0;
        ob_lev = 0;
        ib_rf = 0;
        ib_dis_eq = 0;
        ctrl_data = 0;
        if_mode = 0;
        qrate = 0;
        break;
    default:
        VTSS_E("Serdes6g mode not supported:%u",mode);
        return VTSS_RC_ERROR;
    }
    /* RCOMP_CFG0.MODE_SEL = 2 */
    VTSS_RC(jr_wr(RCOMP_CFG0,0x3<<8));
    
    /* RCOMP_CFG0.RUN_CAL = 1 */
    VTSS_RC(jr_wr(RCOMP_CFG0, 0x3<<8|1<<12));
    
    /* Wait for calibration to finish */
    JR_POLL_BIT(MACRO_CTRL, RCOMP_STATUS_RCOMP_STATUS, BUSY);  
    JR_RDF(MACRO_CTRL, RCOMP_STATUS_RCOMP_STATUS, RCOMP, &rcomp_val);

    /* 1. Configure macro, apply reset */
    /* OB_CFG */
    JR_WRM(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_OB_CFG,
           (ob_pol ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POL : 0) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POST0(ob_post0) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_PREC(ob_prec) |
           VTSS_ENCODE_BITFIELD(rcomp_val+1,4,4) | /* ob_resistor_ctrl: bit 4-7! (RCOMP readout + 1) */
           (ob_ena1v ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_ENA1V_MODE : 0),

           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POL |
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POST0 |
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_PREC |
           VTSS_ENCODE_BITMASK(4,4) | /* ob_resistor_ctrl: bit 4-7! */
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_ENA1V_MODE);
    
    /* OB_CFG1 */
    JR_WRM(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1,
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_ENA_CAS(ob_ena_cas) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_LEV(ob_lev),
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_ENA_CAS |
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_LEV);

     /* IB_CFG */      
    JR_WRM(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_IB_CFG,
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_IC_AC(ib_ic_ac) | 
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RT(15) | 
          VTSS_ENCODE_BITFIELD(ib_vbac,7,3) | 
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RESISTOR_CTRL(rcomp_val+2) |
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_VBCOM(ib_vbcom) |
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RF(ib_rf),
          VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_IC_AC |
          VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RT |
          VTSS_ENCODE_BITMASK(7,3) | 
          VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RESISTOR_CTRL |
          VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_VBCOM |
          VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RF);

    /* IB_CFG1 */      
    JR_WRM(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1,
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_C(ib_c) |
           (ib_chf ? VTSS_BIT(7) : 0) |
           (ib_dis_eq ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_DIS_EQ : 0) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSAC |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSDC |
           (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA : 0) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST,
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_C |
           VTSS_BIT(7) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_DIS_EQ |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSAC |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSDC |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST);
    
    /* DES_CFG */      
    JR_WRM(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_DES_CFG,
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_ANA(5) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL(des_cpmd_sel) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_MBTR_CTRL(des_mbtr_ctrl),
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_ANA |
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL |
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_MBTR_CTRL);

    /* SER_CFG */      
    JR_WRM(MACRO, CTRL_SERDES6G_ANA_CFG_SERDES6G_SER_CFG,
          (ser_4tab_ena ? VTSS_BIT(8) : 0) |
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_SER_CFG_SER_ALISEL(ser_alisel) |
           (ser_enali ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_SER_CFG_SER_ENALI : 0),
           VTSS_BIT(8) |
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_SER_CFG_SER_ALISEL |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_SER_CFG_SER_ENALI);

    /* PLL_CFG */      
    JR_WRM(MACRO, CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG,
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_CTRL_DATA(ctrl_data) |
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_ENA |
          VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ROT_FRQ |
          (rot_dir ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ROT_DIR : 0) |
          (div4 ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_DIV4 : 0) |
           (ena_rot ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ENA_ROT : 0),
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_CTRL_DATA |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_ENA |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ROT_FRQ |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ROT_DIR |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_DIV4 |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ENA_ROT);

    /* Write masked to avoid changing RECO_SEL_* fields used by SyncE */
    /* COMMON_CFG */      
    JR_WRM(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG,
           (ena_lane ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_ENA_LANE : 0) |
           (hrate ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_HRATE : 0) |
           (qrate ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_QRATE : 0) |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_IF_MODE(if_mode),
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SYS_RST |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_ENA_LANE |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_HRATE |
           VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_QRATE |
           VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_IF_MODE);
    
    /* MISC_CFG */      
    JR_WRM(MACRO_CTRL, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG,
           (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA : 0),
           VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA);

    JR_WRM(MACRO_CTRL, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG,
           (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA : 0) |
           VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST,
           VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA |
           VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST);
    
    VTSS_RC(jr_sd6g_write(addr, 60000));
    
    /* In RXAUI mode, keep odd lanes reset to avoid crosstalk */
    if (mode == VTSS_SERDES_MODE_RXAUI)
        addr &= 0x55555555;

    /* 2. Release PLL Reset */
    JR_WRB(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, SYS_RST, 1);
    VTSS_RC(jr_sd6g_write(addr, 130000));
    
    /* 3. Release digital reset */
    JR_WRB(MACRO_CTRL, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, LANE_RST, 0);
    JR_WRB(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, IB_RST, 0);
    VTSS_RC(jr_sd6g_write(addr, 60000));

    return VTSS_RC_OK;
}

static vtss_rc jr_sd1g_cfg (vtss_serdes_mode_t mode, u32 adr)
{
    BOOL ena_lane, if_100fx, ib_ena_dc=0;
    u32  ob_amp_ctrl, rcomp_val, resist_val, value, rev, des_bw_ana=6, cpmd_sel=0, des_mbtr_ctrl=0;
    
    switch (mode) {
    case VTSS_SERDES_MODE_SGMII:
        ena_lane = 1;
        ob_amp_ctrl = 12;
        if_100fx = 0;      
        break;
    case VTSS_SERDES_MODE_100FX:
        ena_lane = 1;
        ob_amp_ctrl = 12;
        ib_ena_dc = 1;
        des_bw_ana = 0;
        cpmd_sel = 2;
        des_mbtr_ctrl = 3;
        if_100fx = 1;
        break;
    case VTSS_SERDES_MODE_1000BaseX:
        ena_lane = 1;
        ob_amp_ctrl = 15;
        if_100fx = 0;
        break;
    case VTSS_SERDES_MODE_DISABLE:
        if_100fx = 0;
        ena_lane = 0;
        ob_amp_ctrl = 0;
        break;
    default:
        VTSS_E("Serdes1g mode not supported");
        return VTSS_RC_ERROR;
    }

    JR_RD(DEVCPU_GCB, CHIP_REGS_CHIP_ID, &value);
    rev = JR_GET_FLD(DEVCPU_GCB, CHIP_REGS_CHIP_ID, REV_ID, value);

    /* RCOMP_CFG0.MODE_SEL = 2 */
    VTSS_RC(jr_wr(RCOMP_CFG0,0x3<<8));
    
    /* RCOMP_CFG0.RUN_CAL = 1 */
    VTSS_RC(jr_wr(RCOMP_CFG0, 0x3<<8|1<<12));
    
    /* Wait for calibration to finish */
    JR_POLL_BIT(MACRO_CTRL, RCOMP_STATUS_RCOMP_STATUS, BUSY);  
    JR_RDF(MACRO_CTRL, RCOMP_STATUS_RCOMP_STATUS, RCOMP, &rcomp_val);
        
    /* 1. Enable sig_det circuitry, lane, select if mode, apply reset */

    JR_WRM(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_IB_CFG,
           (if_100fx ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_FX100_ENA : 0) |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DETLEV |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_DET_LEV(3) |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_CMV_TERM |
           (ib_ena_dc ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DC_COUPLING : 0) |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_OFFSET_COMP |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_RESISTOR_CTRL(rcomp_val-3) |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_EQ_GAIN(3),
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_FX100_ENA |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DETLEV |
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_DET_LEV |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_CMV_TERM |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DC_COUPLING |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_OFFSET_COMP |
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_RESISTOR_CTRL |
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_EQ_GAIN);

    JR_WRM(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_DES_CFG,
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_BW_ANA(des_bw_ana) |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_CPMD_SEL(cpmd_sel) |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_MBTR_CTRL(des_mbtr_ctrl),
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_BW_ANA |
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_CPMD_SEL |
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_MBTR_CTRL);


    resist_val = (vtss_state->init_conf.serdes.serdes1g_vdd == VTSS_VDD_1V0) ? rcomp_val+1 : rcomp_val-4;
    JR_WRM(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_OB_CFG,
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_RESISTOR_CTRL(resist_val) |
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_AMP_CTRL(ob_amp_ctrl),
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_RESISTOR_CTRL |
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_AMP_CTRL);

    /* Write masked to avoid changing RECO_SEL_* fields used by SyncE */
    JR_WRM(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG,
            (ena_lane ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_ENA_LANE : 0) | VTSS_BIT(0),            
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SYS_RST |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_ENA_LANE |
           (rev == 0 ? VTSS_BIT(7) : 0) | /* Rev 0 -> HRATE = 0.  Rev 1 -> HRATE = 1 (default) */
            VTSS_BIT(0)); /* IFMODE = 1 */


    JR_WRM(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_PLL_CFG,
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_ENA |
           (rev == 0 ? VTSS_BIT(21) : 0) | /* Rev 0 -> RC_DIV2 = 1. Rev 1 -> RC_DIV2 = 0 (default) */
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_CTRL_DATA(200),
           VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_ENA |
           VTSS_BIT(21) |
           VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_CTRL_DATA);
    

    JR_WRM(MACRO_CTRL, SERDES1G_DIG_CFG_SERDES1G_MISC_CFG,
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_DES_100FX_CPMD_ENA : 0) |
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_LANE_RST,
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_DES_100FX_CPMD_ENA |
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_LANE_RST);

    VTSS_RC(jr_sd1g_write(adr, 40000));
    /* 2. Release PLL Reset  */

    JR_WRB(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, SYS_RST, 1);

    VTSS_RC(jr_sd1g_write(adr, 90000));

    /* 3. Release digital reset */

    JR_WRB(MACRO_CTRL, SERDES1G_DIG_CFG_SERDES1G_MISC_CFG, LANE_RST, 0);
    VTSS_RC(jr_sd1g_write(adr, 0));
    return VTSS_RC_OK;
}


typedef enum
{
    SERDES_1G,
    SERDES_6G,
} serdes_t;

/* Convert port to serdes instance */
static vtss_rc jr_port2serdes_get(u32 port, serdes_t *type, u32 *lane, u32 *count)
{
    vtss_rc              rc = VTSS_RC_OK;
    vtss_target_type_t   target = jr_target_get();
    vtss_port_mux_mode_t mux_mode = vtss_state->mux_mode[vtss_state->chip_no];
    u32                  base_1g = 0, cnt_1g = 24;
    u32                  base_2g5 = 23, cnt_2g5 = 0;
    u32                  base_10g = VTSS_PORT_10G_START, cnt_10g = 4;
    
    /* Determine base and count for 1G/2.5G/10G ports based on target and mux mode */
    switch (target) {
    case VTSS_TARGET_JAGUAR_1:
    case VTSS_TARGET_E_STAX_III_48:
    case VTSS_TARGET_E_STAX_III_68:
    case VTSS_TARGET_CE_MAX_24:
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            /* 24x1G + 4x10G + NPI */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
            /* 23x1G + 4x2g5 + 3x10G + NPI */
            cnt_1g = 23;
            cnt_2g5 = 4;
            base_10g = 28;
            cnt_10g = 3;
        } else if (mux_mode == VTSS_PORT_MUX_MODE_7) {
            /* 19x1G + 8x2g5 + 2x10G + NPI */
            cnt_1g = 19;
            base_2g5 = 19;
            cnt_2g5 = 8;
            base_10g = 29;
            cnt_10g = 2;
        } else {
            VTSS_E("Mux mode no supported");
            rc = VTSS_RC_ERROR;
        }
        break;
    case VTSS_TARGET_LYNX_1:
    case VTSS_TARGET_CE_MAX_12:
        base_1g = 11;
        cnt_1g = 12;
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            /* 12x1G + 2x10G + NPI */
            cnt_10g = 2;
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
            /* 12x1G + 4x2g5 + 1x10g + NPI */
            cnt_2g5 = 4;
            base_10g = 28;
            cnt_10g = 1;
        } else if (mux_mode == VTSS_PORT_MUX_MODE_7) {
            /* 8x1G + 8x2g5 + NPI */
            if (target == VTSS_TARGET_CE_MAX_12)
                rc = VTSS_RC_ERROR;
            cnt_1g = 8;
            base_2g5 = 19;
            cnt_2g5 = 8;
            base_10g = 29;
            cnt_10g = 0;
        } else {
            VTSS_E("Mux mode no supported");
            rc = VTSS_RC_ERROR;
        }
        if (target == VTSS_TARGET_LYNX_1) {
            /* LynX supports two more XAUI ports */
            cnt_10g += 2;
        }
        break;
    default:
        VTSS_E("Unknown target");
        rc = VTSS_RC_ERROR;
        break;
    } /* switch */

    /* Determine SerDes information based on port information */
    if (port >= base_1g && port < (base_1g + cnt_1g)) {
        /* 1G port */
        *type = SERDES_1G;
        *lane = port;
        *count = 1;
    } else if (port >= base_2g5 && port < (base_2g5 + cnt_2g5)) {
        /* 2.5G port */
        *type = SERDES_6G;
        *lane = (port - base_2g5);
        *count = 1;
    } else if (port >= base_10g && port < (base_10g + cnt_10g)) {
        /* 10G port */
        *type = SERDES_6G;
        *lane = 4*(port - VTSS_PORT_10G_START);
        *count = 4;
    } else if (port == VTSS_PORT_NPI) {
        /* NPI port */
        *type = SERDES_1G;
        *lane = 24;
        *count = 1;
    } else {
        /* Illegal port */
        rc = VTSS_RC_ERROR;
    }

    return rc;
}

/* 10G port rate control register values */
#define JR_RATE_CTRL_GAP_10G   20
#define JR_RATE_CTRL_WM_LO_10G 639
#define JR_RATE_CTRL_WM_HI_10G 641

static vtss_rc jr_port_setup(vtss_port_no_t port_no, u32 chipport, vtss_port_conf_t *conf, BOOL front_port)
{
    const u8 *mac;
    u32      wm_low = 0, wm_high, wm_stop = 4, mac_hi, mac_lo, gap = 12, value;
    BOOL     dev1g = (chipport < 18), valid_sqs_conf = 1;
    sqs_t    sqs;
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
    u32      tmp_gap_val;
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */
#if defined(VTSS_FEATURE_VSTAX)
    u32      portmask = VTSS_BIT(chipport), prio;
#endif
    
    vtss_target_type_t target = jr_target_get();

    switch (conf->speed) {
    case VTSS_SPEED_10M:
        wm_high = (dev1g ? 7 : 4);
        break;
    case VTSS_SPEED_100M:
        wm_high = (dev1g ? 30 : 58);
        break;
    case VTSS_SPEED_1G:
        wm_low = (dev1g ? 255 : 63);
        wm_high = (dev1g ? 257 : 65);
        break;
    case VTSS_SPEED_2500M:
        wm_low = 159;
        wm_high = 161;
        break;
    case VTSS_SPEED_10G:
        wm_low = JR_RATE_CTRL_WM_LO_10G;
        wm_high = JR_RATE_CTRL_WM_HI_10G;
        break;
    case VTSS_SPEED_12G:
        wm_low = 767;
        wm_high = 769;
        break;
    default:
        VTSS_E("illegal speed");
        return VTSS_RC_ERROR;
    }

    if (dev1g) {
        /* DEV1G */
        wm_stop = 3;
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
        JR_RDX(DSM, CFG_RATE_CTRL, chipport, &tmp_gap_val);
        tmp_gap_val = VTSS_X_DSM_CFG_RATE_CTRL_FRM_GAP_COMP(tmp_gap_val);
        if (tmp_gap_val > 12) {
            gap = tmp_gap_val;
        }
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */
    } else if (VTSS_PORT_IS_10G(chipport)) {
        /* DEV10G */
        JR_RDX(DSM, HOST_IF_CFG_HIH_CFG, (chipport - 27), &value);
        if (JR_GET_BIT(DSM, HOST_IF_CFG_HIH_CFG, HIH_ENA, value)) { 
            gap = 12;  /* HIH is enabled, do not compensate */
        } else {
            gap = JR_RATE_CTRL_GAP_10G;
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
            /* TBD: Make it generic once 802.3ar is supported on host interfaces also */
            JR_RDX(DSM, CFG_RATE_CTRL, chipport,&tmp_gap_val);
            tmp_gap_val = VTSS_X_DSM_CFG_RATE_CTRL_FRM_GAP_COMP(tmp_gap_val);
            if (tmp_gap_val > 12) {
                gap = tmp_gap_val;
            }
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */
        }
        wm_stop = 5;
        JR_WRXF(DSM, CFG_RATE_CTRL_WM_DEV10G, (chipport - 27), TAXI_128_RATE_CTRL_WM, 
                conf->speed < VTSS_SPEED_10G ? 24 : 44);
    } else if (chipport != VTSS_PORT_NPI) {
        /* DEV2G5 */
        JR_WRXF(DSM, CFG_RATE_CTRL_WM_DEV2G5, (chipport - 18), TAXI_128_RATE_CTRL_WM_DEV2G5, 
                conf->speed == VTSS_SPEED_2500M ? 36 : 24);
    }

    /* Setup WM in DSM and ASM */  
    JR_WRX(DSM, CFG_SCH_STOP_WM_CFG, chipport, VTSS_F_DSM_CFG_SCH_STOP_WM_CFG_SCH_STOP_WM(wm_stop));
                                                          
    JR_WRX(DSM, CFG_RATE_CTRL, chipport,
           VTSS_F_DSM_CFG_RATE_CTRL_FRM_GAP_COMP(gap) |
           VTSS_F_DSM_CFG_RATE_CTRL_TAXI_RATE_HIGH(wm_high) |
           VTSS_F_DSM_CFG_RATE_CTRL_TAXI_RATE_LOW(wm_low));

    /* Flow Control configuration */
    JR_WRXB(DSM, CFG_RX_PAUSE_CFG, chipport, RX_PAUSE_EN, conf->flow_control.obey); 
    JR_WRXB(DSM, CFG_ETH_FC_GEN, chipport, ETH_PORT_FC_GEN, conf->flow_control.generate); 

#if defined(VTSS_FEATURE_VSTAX)
    /* Enable use of CMEF when the front port can generate flow control, otherwise disable it. */
    for (prio = 0; front_port && prio < (VTSS_PRIOS - 2); prio++) {
        JR_WRXM(ARB, CFG_STATUS_CM_CFG_VEC0, prio, conf->flow_control.generate ? portmask : 0, portmask);
    }
#endif

    /* Update policer flow control configuration */
    VTSS_RC(jr_port_policer_fc_set(port_no, chipport));

    JR_WRX(DSM, CFG_MAC_CFG, chipport, VTSS_F_DSM_CFG_MAC_CFG_TX_PAUSE_VAL(0xFFFF) |
           (conf->fdx ? 0 : VTSS_F_DSM_CFG_MAC_CFG_HDX_BACKPRESSURE) |
           VTSS_F_DSM_CFG_MAC_CFG_TX_PAUSE_XON_XOFF);

    mac = conf->flow_control.smac.addr;
    mac_hi = ((mac[0] << 16) | (mac[1] << 8) | (mac[2] << 0));
    mac_lo = ((mac[3] << 16) | (mac[4] << 8) | (mac[5] << 0));
    JR_WRXF(DSM, CFG_MAC_ADDR_BASE_HIGH_CFG, chipport, MAC_ADDR_HIGH, mac_hi);
    JR_WRXF(DSM, CFG_MAC_ADDR_BASE_LOW_CFG, chipport, MAC_ADDR_LOW, mac_lo);
    JR_WRXF(ASM, CFG_MAC_ADDR_HIGH_CFG, chipport, MAC_ADDR_HIGH, mac_hi);
    JR_WRXF(ASM, CFG_MAC_ADDR_LOW_CFG, chipport, MAC_ADDR_LOW, mac_lo);
    JR_WRX(ASM, CFG_PAUSE_CFG, chipport, 
           VTSS_F_ASM_CFG_PAUSE_CFG_ABORT_PAUSE_ENA |
           VTSS_F_ASM_CFG_PAUSE_CFG_ABORT_CTRL_ENA);

    /* FC Stacking awareness */
    JR_WRXF(ANA_CL_2, PORT_STACKING_CTRL, chipport, IGR_DROP_ENA, (conf->flow_control.obey ? 0 : 0xFF));

    memset(&sqs,0,sizeof(sqs));
    if (target == VTSS_TARGET_E_STAX_III_48 || target == VTSS_TARGET_E_STAX_III_68) {
        /* Setup SQS WMs */
        VTSS_RC(jr_sqs_rd(chipport, &sqs));

        if (sqs.iqs.port.rx_ena && conf->power_down == 0) {
            /*  Power down port in the SQS WM calculation */            
            VTSS_RC(jr_sqs_calc_wm(chipport, conf->speed, 0, 0, 1, 0, &sqs));  
        } else if ((sqs.iqs.port.rx_ena == 0) && conf->power_down) {
            valid_sqs_conf = 0;
        }        
        
        /* Everything supported for Enterprise */
        if (valid_sqs_conf) {            
            /* Setup WM based on speed/jumbo/FC/power_down */
            VTSS_RC(jr_sqs_calc_wm(chipport, conf->speed, (conf->max_frame_length>1522?1:0), conf->flow_control.obey, conf->power_down, 0, &sqs)); 
            VTSS_RC(jr_sqs_set(chipport, &sqs));
        }
        
        if (chipport == VTSS_CHIP_PORT(port_no) && VTSS_PORT_CHIP_SELECTED(port_no)) {
            VTSS_RC(jr_qos_wred_conf_set(port_no));  /* Must update RED */
        }
    }
    
    /* Setup MTU */
    VTSS_RC(jr_setup_mtu(chipport, conf->max_frame_length, front_port));

#if defined(VTSS_FEATURE_VSTAX_V2)
    VTSS_RC(jr_vstax_update_ingr_shaper(chipport, conf->speed, front_port));
#endif /* VTSS_FEATURE_VSTAX_V2 */

    return VTSS_RC_OK;
}

static vtss_rc jr_queue_flush(u32 port, BOOL flush)
{
    u32 value, q;

    if (flush) {
        /* Disable Rx and TX queues, but don't flush them (see Gnats#6936 and Bugzilla#8127) */
        for (q = 0; q < 8; q++) {
            JR_WRXB(OQS, QU_RAM_CFG_QU_RC_CFG, (port*8+q), RX_QU_ENA, 0);
            JR_WRXB(IQS, QU_RAM_CFG_QU_RC_CFG, (port*8+q), RX_QU_ENA, 0);
        }

        /* Disable FC in DSM */
        JR_WRXB(DSM, CFG_RX_PAUSE_CFG, port, RX_PAUSE_EN, 0);
        
        /* Disable Rx and Tx in MAC */
        if (VTSS_PORT_IS_1G(port)) {
            JR_WRX(DEV1G, MAC_CFG_STATUS_MAC_ENA_CFG, VTSS_TO_DEV(port), 0);
        } else {
            JR_WRX(DEV10G, MAC_CFG_STATUS_MAC_ENA_CFG, VTSS_TO_DEV(port), 0);
        }

        /*  Remove port from the the  ANA forwarding and learn masks */
        JR_RD(ANA_L3_2, COMMON_PORT_FWD_CTRL, &value);
        JR_WR(ANA_L3_2, COMMON_PORT_FWD_CTRL, (value & ~VTSS_BIT(port)));
        if (vtss_state->create.target != VTSS_TARGET_CE_MAX_12 && 
            vtss_state->create.target != VTSS_TARGET_CE_MAX_24) { 
            JR_RD(ANA_L3_2, COMMON_PORT_LRN_CTRL, &value);
            JR_WR(ANA_L3_2, COMMON_PORT_LRN_CTRL, (value & ~VTSS_BIT(port)));
        }

        /* Wait a worst case time 8ms (jumbo/10Mbit) */
        VTSS_NSLEEP(8000000);
   
        /* Clear port buffer in DSM */
        JR_WRX(DSM, CFG_CLR_BUF, 0, VTSS_BIT(port));
    } else {
        /* Enable the port queues */
        for (q = 0; q < 8; q++) {
            JR_WRXB(IQS, QU_RAM_CFG_QU_RC_CFG, (port*8+q),  RX_QU_ENA, 1);
            JR_WRXB(OQS, QU_RAM_CFG_QU_RC_CFG, (port*8+q),  RX_QU_ENA, 1);
        }
        /* Enable transmission in the scheduler */
        JR_WRXF(SCH, QSIF_QSIF_ETH_PORT_CTRL, port, ETH_TX_DIS, 0);

        /*  Add the port to the  ANA forwarding and learn masks */
        JR_RD(ANA_L3_2, COMMON_PORT_FWD_CTRL, &value);
        JR_WR(ANA_L3_2, COMMON_PORT_FWD_CTRL, (value & ~VTSS_BIT(port)) | VTSS_BIT(port));
        if (vtss_state->create.target != VTSS_TARGET_CE_MAX_12 && 
            vtss_state->create.target != VTSS_TARGET_CE_MAX_24) {  
            JR_RD(ANA_L3_2, COMMON_PORT_LRN_CTRL, &value);
            JR_WR(ANA_L3_2, COMMON_PORT_LRN_CTRL, (value & ~VTSS_BIT(port)) | VTSS_BIT(port));        
        }

    }    
    return VTSS_RC_OK;
}

/* Setup function for SGMII/SERDES/FX100/VAUI ports */
/* Chip ports 0-18:  Dev1g   -> 1G operation        */
/* Chip ports 19-26: Dev2G5g -> 1G/2g5 operation    */
static vtss_rc jr_port_conf_1g_set(vtss_port_no_t port_no, u32 port, 
                                   vtss_port_conf_t *conf, BOOL front_port)
{
    u32                giga = 0, fdx, clk, rx_ifg1, rx_ifg2, tx_ifg;
    u32                sgmii = 0, if_100fx = 0, disabled, value, count;
    u32                tgt = VTSS_TO_DEV(port);
    vtss_port_speed_t  speed;
    vtss_serdes_mode_t mode = VTSS_SERDES_MODE_SGMII;
    serdes_t           serdes_type;
    u32                serdes_instance;

    disabled = (conf->power_down ? 1 : 0);
    fdx = conf->fdx;
    speed = conf->speed;

    /* Verify port speed */
    switch (speed) {
    case VTSS_SPEED_10M:
        clk = 0;
        break;
    case VTSS_SPEED_100M:
        clk = 1;
        break;
    case VTSS_SPEED_1G:
        clk = 2;
        giga = 1;
        break;
    case VTSS_SPEED_2500M:
        if (port < 19 || port > 26) {
            VTSS_E("Illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        clk = 2;
        giga = 1;
        mode = VTSS_SERDES_MODE_2G5;
        break;
    default:
        VTSS_E("Illegal speed: %d", speed);
        return VTSS_RC_ERROR;
    }

    /* Verify Interface type */
    switch (conf->if_type) {
    case VTSS_PORT_INTERFACE_NO_CONNECTION:
        disabled = 1;
        break;
    case VTSS_PORT_INTERFACE_SGMII:
        VTSS_D("chip port: %u interface:SGMII", port);
        if (speed != VTSS_SPEED_10M && speed != VTSS_SPEED_100M && speed != VTSS_SPEED_1G) {
            VTSS_E("SGMII, illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        sgmii = 1;
        mode = VTSS_SERDES_MODE_SGMII;
        break;
    case VTSS_PORT_INTERFACE_100FX:
        VTSS_D("chip port: %u interface:100FX", port);
        if (speed != VTSS_SPEED_100M) {
            VTSS_E("100FX, illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        mode = VTSS_SERDES_MODE_100FX;
        if_100fx = 1;
        break;
    case VTSS_PORT_INTERFACE_SERDES:
        VTSS_D("chip port: %u interface:SERDES", port);
        if (speed != VTSS_SPEED_1G) {
            VTSS_E("SERDES, illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }            
        mode = VTSS_SERDES_MODE_1000BaseX;
        break;
    case VTSS_PORT_INTERFACE_SGMII_CISCO:
        if (speed != VTSS_SPEED_10M && speed != VTSS_SPEED_100M && speed != VTSS_SPEED_1G) {
            VTSS_E("SGMII_CISCO, illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        mode = VTSS_SERDES_MODE_1000BaseX;
        sgmii = 1;        
        break;
    case VTSS_PORT_INTERFACE_VAUI:
        VTSS_D("chip port: %u interface:VAUI", port);
        if (speed != VTSS_SPEED_1G && speed != VTSS_SPEED_2500M) {
            VTSS_E("VAUI, illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        mode = (speed == VTSS_SPEED_2500M ? VTSS_SERDES_MODE_2G5 : VTSS_SERDES_MODE_1000BaseX);
        sgmii = 0;
        break;
    case VTSS_PORT_INTERFACE_LOOPBACK:
        mode = VTSS_SERDES_MODE_1000BaseX;
        break;
    default:
        VTSS_E("Illegal interface type");
        return VTSS_RC_ERROR;
    }  

    /* (re-)configure the Serdes macros to 100FX / 1000BaseX */
    if (mode != VTSS_SERDES_MODE_DISABLE && mode != vtss_state->serdes_mode[port_no] &&
        ((mode == VTSS_SERDES_MODE_SGMII) || (mode == VTSS_SERDES_MODE_1000BaseX) || 
         (mode == VTSS_SERDES_MODE_100FX) || (mode == VTSS_SERDES_MODE_2G5))) {

        if ((jr_port2serdes_get(port, &serdes_type, &serdes_instance, &count) == VTSS_RC_OK)) {
            if (serdes_type == SERDES_1G) {
                VTSS_RC(jr_sd1g_read(1 << serdes_instance));
                VTSS_MSLEEP(10);
                VTSS_RC(jr_sd1g_cfg(mode, 1 << serdes_instance));
                VTSS_MSLEEP(1);        
            } else {
                VTSS_RC(jr_sd6g_read(1 << serdes_instance));
                VTSS_MSLEEP(10);
                VTSS_RC(jr_sd6g_cfg(mode, 1 << serdes_instance));
                VTSS_MSLEEP(1);        
            }
            vtss_state->serdes_mode[port_no] = mode;                
        } else {
            VTSS_E("Could not read serdes type");
            return VTSS_RC_ERROR;
        }
        vtss_state->serdes_mode[port_no] = mode;
    }

    /* Disable and flush port queues */
    VTSS_RC(jr_queue_flush(port, 1));

    /* Reset MAC */
    JR_WRX(DEV1G, DEV_CFG_STATUS_DEV_RST_CTRL, tgt,
           VTSS_F_DEV1G_DEV_CFG_STATUS_DEV_RST_CTRL_MAC_TX_RST |
           VTSS_F_DEV1G_DEV_CFG_STATUS_DEV_RST_CTRL_MAC_RX_RST);

    /* Common port setup */
    VTSS_RC(jr_port_setup(port_no, port, conf, front_port));

    /* Decide interframe gaps */
    rx_ifg1 = conf->frame_gaps.hdx_gap_1;
    if (rx_ifg1 == VTSS_FRAME_GAP_DEFAULT) {
        rx_ifg1 = (giga ? 5 : (speed == VTSS_SPEED_100M) ? 7 : 8);
    }

    rx_ifg2 = conf->frame_gaps.hdx_gap_2;
    if (rx_ifg2 == VTSS_FRAME_GAP_DEFAULT) {
        rx_ifg2 = (giga ? 1 : 8);
    }

    tx_ifg = conf->frame_gaps.fdx_gap;
    if (tx_ifg == VTSS_FRAME_GAP_DEFAULT) {
        tx_ifg = (giga ? 4 : (fdx == 0) ? 13 : 11);
    }

    /* Set interframe gaps */
    JR_WRX(DEV1G, MAC_CFG_STATUS_MAC_IFG_CFG, tgt,
           VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_IFG_CFG_TX_IFG(tx_ifg) |
           VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG1(rx_ifg1) |
           VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG2(rx_ifg2));

    /* MAC mode */
    if (fdx && ((speed == VTSS_SPEED_1G) || speed == VTSS_SPEED_2500M)) {
        value = VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_MODE_CFG_FDX_ENA | VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_MODE_CFG_GIGA_MODE_ENA;
    } else if (fdx) {
        value = VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_MODE_CFG_FDX_ENA;
    } else {
        value = 0;
    }
    /* Giga and FDX mode */
    JR_WRX(DEV1G, MAC_CFG_STATUS_MAC_MODE_CFG, tgt, value);

    /* Half duplex */
    JR_WRX(DEV1G, MAC_CFG_STATUS_MAC_HDX_CFG, tgt,
           VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_HDX_CFG_SEED(conf->flow_control.smac.addr[5]) |
           (fdx ? 0 : VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_HDX_CFG_SEED_LOAD) |
           (conf->exc_col_cont ? VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_HDX_CFG_RETRY_AFTER_EXC_COL_ENA : 0) | 
           VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_HDX_CFG_LATE_COL_POS(0x43));

    /* PCS used in SERDES or SGMII mode */
    JR_WRX(DEV1G, PCS1G_CFG_STATUS_PCS1G_MODE_CFG, tgt,
           sgmii ? VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_MODE_CFG_SGMII_MODE_ENA : 0);

    /* Signal detect */
    if (if_100fx) {  
        JR_WRXM(DEV1G, PCS_FX100_CONFIGURATION_PCS_FX100_CFG, tgt,
                (conf->sd_enable ?       VTSS_F_DEV1G_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_ENA : 0) |
                (conf->sd_internal ? 0 : VTSS_F_DEV1G_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_SEL) |
                (conf->sd_active_high ? VTSS_BIT(25) : 0),
                VTSS_F_DEV1G_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_ENA |
                VTSS_F_DEV1G_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_SEL |
                VTSS_BIT(25)); 
    } else {
        JR_WRX(DEV1G, PCS1G_CFG_STATUS_PCS1G_SD_CFG, tgt,
               (conf->sd_active_high ? VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_POL : 0) |
               (conf->sd_internal ? 0 : VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_SEL) |
               (conf->sd_enable ? VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_ENA : 0));
    }

    /* Enable/Disable 100FX PCS */
    JR_WRXB(DEV1G, PCS_FX100_CONFIGURATION_PCS_FX100_CFG, tgt, PCS_ENA, (if_100fx ? 1 : 0));
    
    /* Enable/disable Serdes/SGMII PCS */
    JR_WRXB(DEV1G, PCS1G_CFG_STATUS_PCS1G_CFG, tgt, PCS_ENA, (if_100fx ? 0 : 1));

    if (conf->if_type == VTSS_PORT_INTERFACE_SGMII) {
        JR_WRX(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, tgt, 0);
    } else if (conf->if_type == VTSS_PORT_INTERFACE_SGMII_CISCO) {
        /* Complete SGMII aneg */
        value = 0x0001;       

        JR_WRX(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, tgt,
               VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ADV_ABILITY(value) |
               VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_ENA  | 
               VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_SW_RESOLVE_ENA |
               VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_RESTART_ONE_SHOT);
        
        /* Clear the sticky bits */
        JR_RDX(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, &value);
        JR_WRX(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, value);
    }

    /* Set it in loopback mode if required */
    JR_WRXB(DEV1G, PCS1G_CFG_STATUS_PCS1G_LB_CFG, tgt, TBI_HOST_LB_ENA, conf->if_type == VTSS_PORT_INTERFACE_LOOPBACK ? 1 : 0);

    /* Set fill level before DSM is signalled */
    JR_WRXF(DEV1G, DEV_CFG_STATUS_DEV_DBG_CFG, tgt, TX_BUF_HIGH_WM, 11);

    /* Release MAC from reset */
    JR_WRX(DEV1G, DEV_CFG_STATUS_DEV_RST_CTRL, tgt,
           VTSS_F_DEV1G_DEV_CFG_STATUS_DEV_RST_CTRL_SPEED_SEL(disabled ? 3 : clk) |
           (disabled ? VTSS_F_DEV1G_DEV_CFG_STATUS_DEV_RST_CTRL_MAC_TX_RST : 0) |
           (disabled ? VTSS_F_DEV1G_DEV_CFG_STATUS_DEV_RST_CTRL_MAC_RX_RST : 0));

    /* Enable Rx and Tx in MAC */
    JR_WRX(DEV1G, MAC_CFG_STATUS_MAC_ENA_CFG, tgt,
           (disabled ? 0 : VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA) |
           (disabled ? 0 : VTSS_F_DEV1G_MAC_CFG_STATUS_MAC_ENA_CFG_TX_ENA));

    /* Enable port queues */
    if (!disabled) {
        VTSS_RC(jr_queue_flush(port, 0));
    }

    VTSS_D("chip port: %u (dev1g),is configured", port);

    return VTSS_RC_OK;
}

/* Setup function for XAUI ports                   */
/* Chip ports 27-30: Dev10G @ 10g/12g operation    */
static vtss_rc jr_port_conf_10g_set(vtss_port_no_t port_no, u32 port, 
                                    vtss_port_conf_t *conf, BOOL front_port)
{

    u32                disabled = (conf->power_down ? 1 : 0);
    u32                tgt = VTSS_TO_DEV(port);
    vtss_port_speed_t  speed = conf->speed;
    BOOL               rxaui = 0;
    BOOL               rx_flip = conf->xaui_rx_lane_flip;
    BOOL               tx_flip = conf->xaui_tx_lane_flip;
    serdes_t           serdes_type;
    u32                serdes_instance, count;
    vtss_serdes_mode_t mode = VTSS_SERDES_MODE_XAUI;
    BOOL               xaui =  (conf->if_type == VTSS_PORT_INTERFACE_XAUI || conf->if_type == VTSS_PORT_INTERFACE_RXAUI) ? 1 : 0;
    
    /* Verify speed */
    switch (speed) {
    case VTSS_SPEED_1G:
    case VTSS_SPEED_2500M:
    case VTSS_SPEED_10G:
    case VTSS_SPEED_12G:
        break;
    default:
        VTSS_E("Illegal speed: %d", speed);
        return VTSS_RC_ERROR;
    }

    switch (conf->if_type) {
    case VTSS_PORT_INTERFACE_SERDES:
        VTSS_D("chip port: %u interface:SERDES", port);
        if (speed != VTSS_SPEED_1G) {
            VTSS_E("SERDES, illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }            
        mode = VTSS_SERDES_MODE_1000BaseX;
        break;
    case VTSS_PORT_INTERFACE_XAUI:
        VTSS_D("chip port: %u interface:XAUI", port);
        if (speed == VTSS_SPEED_10G) {
            mode = VTSS_SERDES_MODE_XAUI;
        } else {
            mode = VTSS_SERDES_MODE_XAUI_12G;
        }
        break;
    case VTSS_PORT_INTERFACE_RXAUI:
        mode = VTSS_SERDES_MODE_RXAUI;
        rxaui = 1;
        rx_flip = 0;
        tx_flip = 0;
        break;
    case VTSS_PORT_INTERFACE_VAUI:
        VTSS_D("chip port: %u interface:VAUI", port);
        if (speed != VTSS_SPEED_2500M) {
            VTSS_E("VAUI, illegal speed:%d, port %u", speed, port);
            return VTSS_RC_ERROR;
        }
        mode = VTSS_SERDES_MODE_2G5;
        break;
    case VTSS_PORT_INTERFACE_LOOPBACK:
        mode = VTSS_SERDES_MODE_1000BaseX;
        break;
    default:
        VTSS_E("Illegal interface type");
        return VTSS_RC_ERROR;
    }  

    /* (re-)configure the Serdes macros to 10Gb / 12Gb */
    if (mode != VTSS_SERDES_MODE_DISABLE && (port_no == VTSS_PORT_NO_NONE || mode != vtss_state->serdes_mode[port_no])) {
        if ((jr_port2serdes_get(port, &serdes_type, &serdes_instance, &count) == VTSS_RC_OK) && (serdes_type == SERDES_6G)) {
            VTSS_RC(jr_sd6g_read(1 << serdes_instance));
            VTSS_MSLEEP(10);
            VTSS_RC(jr_sd6g_cfg(mode, 0xf << serdes_instance));
            VTSS_MSLEEP(1);        
        } else {
            VTSS_E("Illegal serdes type chipport:%d",port);
        }
        if (port_no != VTSS_PORT_NO_NONE)
            vtss_state->serdes_mode[port_no] = mode;
    }

    /* Disable and flush port queues */
    VTSS_RC(jr_queue_flush(port, 1));

    JR_WRX(DEV10G, MAC_CFG_STATUS_MAC_ENA_CFG, tgt, 0);

    /* Reset MAC */
    JR_WRX(DEV10G, DEV_CFG_STATUS_DEV_RST_CTRL, tgt,
           VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_MAC_TX_RST |
           VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_MAC_RX_RST |
           VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_PCS_TX_RST |
           VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_PCS_RX_RST);

    if (xaui) {
        /* Disable 1G PCS */
        JR_WRXB(DEV10G, PCS1G_CFG_STATUS_PCS1G_CFG, tgt, PCS_ENA, 0);
        /* Enable XAUI PCS */
        JR_WRXB(DEV10G, PCS_XAUI_CONFIGURATION_PCS_XAUI_CFG, tgt, PCS_ENA, 1);
    } else {
        /* Disable XAUI PCS */
        JR_WRXB(DEV10G, PCS_XAUI_CONFIGURATION_PCS_XAUI_CFG, tgt, PCS_ENA, 0);
        /* Enable 1G PCS */
        JR_WRXB(DEV10G, PCS1G_CFG_STATUS_PCS1G_CFG, tgt, PCS_ENA, 1);
        JR_WRXB(DEV10G, PCS1G_CFG_STATUS_PCS1G_MODE_CFG, tgt, SGMII_MODE_ENA, 0);
        /* Signal detect */
        JR_WRX(DEV10G, PCS1G_CFG_STATUS_PCS1G_SD_CFG, tgt,
               (conf->sd_active_high ? VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_POL : 0) |
               (conf->sd_internal ? 0 : VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_SEL) |
               (conf->sd_enable ? VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_ENA : 0));
    }

    /* Enable XAUI PCS 2x6G */
    //JR_WRXB(DEV10G, PCS2X6G_CONFIGURATION_PCS2X6G_CFG, tgt, PCS_ENA, 1);

    /* Common port setup */
    VTSS_RC(jr_port_setup(port_no, port, conf, front_port));

    /* Link fault signalling used for line ports */
    JR_WRXB(DEV10G, MAC_CFG_STATUS_MAC_LFS_CFG, tgt, LFS_MODE_ENA, (xaui?1:0));

    /* Write Txbuf low and high  */
    VTSS_RC(jr_wr(VTSS_IOREG(tgt,0x64), 9 | 2<<5)); 

    /* Enable Rx and Tx in MAC */
    JR_WRX(DEV10G, MAC_CFG_STATUS_MAC_ENA_CFG, tgt,
           (disabled ? 0 : VTSS_F_DEV10G_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA) |
           (disabled ? 0 : VTSS_F_DEV10G_MAC_CFG_STATUS_MAC_ENA_CFG_TX_ENA));

    if (xaui) {
        /* Setup four or two lane XAUI */
        JR_WRXB(DEV10G, PCS_XAUI_CONFIGURATION_PCS_XAUI_INTERLEAVE_MODE_CFG, 
                tgt, ILV_MODE_ENA, rxaui);
        
        /* XAUI Lane flipping  */
        JR_WRX(DEV10G, PCS_XAUI_CONFIGURATION_PCS_XAUI_EXT_CFG, tgt,
               (rx_flip ? VTSS_F_DEV10G_PCS_XAUI_CONFIGURATION_PCS_XAUI_EXT_CFG_RX_FLIP_HMBUS : 0) |
               (tx_flip ? VTSS_F_DEV10G_PCS_XAUI_CONFIGURATION_PCS_XAUI_EXT_CFG_TX_FLIP_HMBUS : 0));
    }

    /* Set it in loopback mode if required */
    JR_WRXB(DEV10G, PCS1G_CFG_STATUS_PCS1G_LB_CFG, tgt, TBI_HOST_LB_ENA, conf->if_type == VTSS_PORT_INTERFACE_LOOPBACK ? 1 : 0);
    
    /* Release MAC from reset */
    JR_WRX(DEV10G, DEV_CFG_STATUS_DEV_RST_CTRL, tgt,
           VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_SPEED_SEL(disabled ? 6 : xaui ? 4 : (speed == VTSS_SPEED_2500M ? 3 : 2)) |
           (disabled ? VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_PCS_TX_RST : 0) |
           (disabled ? VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_PCS_RX_RST : 0) |
           (disabled ? VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_MAC_TX_RST : 0) |
           (disabled ? VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL_MAC_RX_RST : 0));
    
    /* Clear the Stickies */
    JR_WRX(DEV10G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, 0xFFFF);

    /* Enable port queues */
    VTSS_RC(jr_queue_flush(port, 0));

    VTSS_D("chip port: %u (10G),is configured", port);

    return VTSS_RC_OK;
}


static vtss_rc jr_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_port_conf_t *conf = &vtss_state->port_conf[port_no];
    u32              port = VTSS_CHIP_PORT(port_no);
    BOOL             front_port = 0;
    
#if defined(VTSS_FEATURE_VSTAX_V2)
    front_port = jr_is_frontport(port_no);
#endif /* VTSS_FEATURE_VSTAX_V2 */

    /* Update maximum tags allowed */
    VTSS_RC(jr_port_max_tags_set(port_no));

    if (VTSS_PORT_IS_10G(port)) {
        return jr_port_conf_10g_set(port_no, port, conf, front_port);
    } else {
        return jr_port_conf_1g_set(port_no, port, conf, front_port);
    }
}

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)   

#define JR_CNT_HOST_SQS(tgt, xmit, cnt_id, qu, cnt, clr)                               \
{                                                                                      \
    u32 value;                                                                         \
    JR_RDXY(tgt, STAT_CNT_CFG_##xmit##_STAT_LSB_CNT, qu, JR_CNT(xmit, cnt_id), &value); \
    vtss_cmn_counter_32_update(value, cnt, clr);                                       \
}

static BOOL lport_is_valid(vtss_lport_no_t lport_no)
{
    BOOL is_valid = FALSE;

    switch(vtss_state->init_conf.host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
        case VTSS_HOST_MODE_0:
            is_valid = (lport_no > 23) ? FALSE : TRUE;
            break;
        case VTSS_HOST_MODE_1:
            is_valid = (lport_no > 47) ? FALSE : TRUE;
            break;
#endif
        case VTSS_HOST_MODE_2:
            is_valid = (lport_no > 23) ? FALSE : TRUE;
            break;
        case VTSS_HOST_MODE_3:
            is_valid = (lport_no > 47) ? FALSE : TRUE;
            break;
        default:
            break;
    }
    return is_valid;
}

static vtss_rc jr_xaui_conf_set(vtss_port_no_t port_no)
{
    vtss_xaui_conf_t  *xaui;
    u32               hih_ena, format, chip_port = vtss_state->port_map[port_no].chip_port;
    u32               tmp_val, host_mode, total_ports, tmp_port;
    vtss_rc           rc = VTSS_RC_OK;

    xaui = &vtss_state->xaui[chip_port-27];
    hih_ena = (xaui->hih.format > 1) ? 1 : 0;  
    host_mode = ce_max_hm();

    switch(xaui->hih.format) {
        case VTSS_HIH_PRE_STANDARD:
            format = 0;
            break;
        case VTSS_HIH_PRE_SHORT:
            format = 1;
            break;
        case VTSS_HIH_PRE_SFD:
            format = 2;
            break;
        case VTSS_HIH_POST_SFD:
            format = 3;
            break;
        case VTSS_HIH_IN_CRC:
            format = 4;
            break;
        default:
            return VTSS_RC_ERROR;
    }
    /* HIH position in Preamble */
    JR_WRXF(DEV10G, MAC_CFG_STATUS_MAC_MODE_CFG, VTSS_TO_DEV(chip_port), MAC_PREAMBLE_CFG, format);
    JR_WRXB(DEV10G, MAC_CFG_STATUS_MAC_MODE_CFG, VTSS_TO_DEV(chip_port), HIH_CRC_CHECK, xaui->hih.cksum_enable);   
    /* Move received HIH to IFH */
    JR_WRXB(ASM, CFG_ETH_CFG, chip_port, ETH_PRE_MODE, hih_ena);
    /* Enable / Disable HIH insertion */
    JR_WRXB(REW, COMMON_HIH_CTRL, (chip_port-27), HIH_ENA, hih_ena);
    JR_WRXB(DSM, HOST_IF_CFG_HIH_CFG, (chip_port-27), HIH_ENA, hih_ena);
    /* GAP compenstations is 12 if HIH is enabled, otherwise 20  */
    JR_WRXF(DSM, CFG_RATE_CTRL, chip_port, FRM_GAP_COMP, hih_ena ? 12 : 20);            

    if (chip_port == ce_mac_hmda || chip_port == ce_mac_hmdb) {
        /* Enable/Disable Status channel */
        rc = jr_init_cemax_oobfc_conf_set(chip_port - ce_mac_hmda, xaui->fc.channel_enable);
        if (rc != VTSS_RC_OK) {
            return rc;
        }
        /* Enable/Disable Extended reach flow control on line ports associated with this host interafce */
        switch (host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
            case VTSS_HOST_MODE_0:
                tmp_port = 0;
                if (chip_port != ce_mac_hmda) {
                    tmp_port = 12;
                }
                total_ports = 12;
                break;
            case VTSS_HOST_MODE_1:
                tmp_port = 0;
                if (chip_port != ce_mac_hmda) {
                    tmp_port = 12;
                }
                total_ports = 12;
                break;
#endif /* VTSS_CHIP_CE_MAX_24 */
            case VTSS_HOST_MODE_2:
                tmp_port = 0;
                total_ports = 24;
                break;
            case VTSS_HOST_MODE_3:
                tmp_port = 0;
                total_ports = 24;
                break;
            default:
                return VTSS_RC_ERROR;
        }
        total_ports += tmp_port;
        /* Enable extended reach on Ethernet ports to send-out pause frames */
        for (; tmp_port < total_ports; tmp_port++) {
            JR_RDX(DSM, CFG_ETH_FC_GEN, tmp_port, &tmp_val);
            tmp_val &= (~VTSS_M_DSM_CFG_ETH_FC_GEN_XAUI_EXT_REACH_FC_GEN);
            tmp_val |= VTSS_F_DSM_CFG_ETH_FC_GEN_XAUI_EXT_REACH_FC_GEN(xaui->fc.extended_reach);
            JR_WRX(DSM, CFG_ETH_FC_GEN, tmp_port, tmp_val);
        }
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_qos_lport_wred_conf_set(const vtss_lport_no_t lport_no)
{
    vtss_qos_lport_conf_t *conf = &vtss_state->qos_lport[lport_no];
    int                   queue;
    u32                   oqs_mtu[4];
    u8                    m, total_queues = 0;
    vtss_rc               rc = VTSS_RC_OK;

    for (m = 0; m < 4; m++) {
        JR_RDX(OQS, MTU_MTU_FRM_SIZE_CFG, m, &oqs_mtu[m]);
    }

    switch(vtss_state->init_conf.host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
        case VTSS_HOST_MODE_0:
            total_queues = 8;
            break;
        case VTSS_HOST_MODE_1:
            total_queues = 4;
            break;
#endif
        case VTSS_HOST_MODE_2:
            total_queues = 8;
            break;
        case VTSS_HOST_MODE_3:
            total_queues = 4;
            break;
        default:
            rc = VTSS_RC_ERROR;
            break;
    }
    if (rc != VTSS_RC_OK) {
        return rc;
    }
    for (queue = 0; queue < total_queues; queue++) {
        u32        instance = 256 + (lport_no * total_queues) + queue;
        u32        hlwm, atop_swm, map_cfg, hwm, mtu, l_th, h_th, min_th, max_th, minmax_th, gain;
        vtss_red_t *red = &conf->red[queue];

        /* Sanity check */
        if (red->max_th > 100) {
            VTSS_E("illegal max_th (%u) on lport %u, queue %d", red->max_th, lport_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->min_th > 100) {
            VTSS_E("illegal min_th (%u) on lport %u, queue %d", red->min_th, lport_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->max_prob_1 > 100) {
            VTSS_E("illegal max_prob_1 (%u) on lport %u, queue %d", red->max_prob_1, lport_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->max_prob_2 > 100) {
            VTSS_E("illegal max_prob_2 (%u) on lport %u, queue %d", red->max_prob_2, lport_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->max_prob_3 > 100) {
            VTSS_E("illegal max_prob_3 (%u) on lport %u, queue %d", red->max_prob_3, lport_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->min_th > red->max_th) {
            VTSS_E("min_th (%u) > max_th (%u) on lport %u, queue %d", red->min_th, red->max_th,
                    lport_no, queue);
            return VTSS_RC_ERROR;
        }

        /* OQS */
        JR_RDX(OQS, QU_RAM_CFG_QU_RC_HLWM_CFG, instance, &hlwm);
        JR_RDX(OQS, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG, instance, &atop_swm);
        JR_RDX(OQS, QU_RAM_CFG_MTU_QU_MAP_CFG, instance, &map_cfg);

        hwm  = VTSS_X_OQS_QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_HWM(hlwm);
        mtu  = oqs_mtu[VTSS_X_OQS_QU_RAM_CFG_MTU_QU_MAP_CFG_MTU_PRE_ALLOC(map_cfg)];
        l_th = VTSS_X_OQS_QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_SWM(atop_swm); /* l_th always == swn on OQS */
        h_th = hwm - mtu;                                                            /* h_th always == hwm - mtu on OQS */
        if (l_th >= h_th) {
            l_th = h_th / 4; /* l_th must be less than h_th */
        }
        min_th = (((h_th - l_th) * red->min_th) / 100) + l_th; /* convert min_th from pct to absolute value where 0% = l_th and 100% = h_th */
        max_th = (((h_th - l_th) * red->max_th) / 100) + l_th; /* convert max_th from pct to absolute value where 0% = l_th and 100% = h_th */
        minmax_th = max_th - min_th;
        gain = 0;
        while ((min_th > 255) || (minmax_th > 255)) {
            /* scale min_th and minmax_th down to be within 0..255 and adjust gain accordingly */
            min_th /= 2;
            minmax_th /= 2;
            gain++;
        }
        JR_WRXF(OQS, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG, instance,  RED_ECN_PROFILE_MINTH, min_th);
        JR_WRXF(OQS, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG, instance,  RED_ECN_PROFILE_MINMAXTH, minmax_th);

        JR_WRXF(OQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_ECN_PROF_MAXP,
                VTSS_ENCODE_BITFIELD((red->max_prob_1 * 255) / 100,  0, 8) |
                VTSS_ENCODE_BITFIELD((red->max_prob_2 * 255) / 100,  8, 8) |
                VTSS_ENCODE_BITFIELD((red->max_prob_3 * 255) / 100, 16, 8));
        JR_WRXF(OQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_THRES_GAIN, gain);
        JR_WRXB(OQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_ENA, red->enable);

        JR_WRX(OQS, RED_RAM_RED_ECN_WQ_CFG, instance, 3); /* Recommended values for OQS from VSC7460 Datasheet */
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_qos_lport_dwrr_conf_set(const vtss_lport_no_t lport_no)
{
    vtss_qos_lport_conf_t *tmp_lport_conf;
    u8                    lport_pct[VTSS_LPORTS];
    u8                    lport_cost[VTSS_LPORTS];
    u8                    tmp_instance = 0;
    vtss_lport_no_t       tmp_lport = 0, total_lports = 0;
    vtss_rc               rc = VTSS_RC_OK;
    u8                    tmp;
    vtss_host_mode_t      host_mode = vtss_state->init_conf.host_mode;

    /* Enable the DWRR configurations */
    switch (host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
        case VTSS_HOST_MODE_0:
            tmp_instance = (lport_no > 11) ? 1 : 0;
            tmp_lport = (tmp_instance) * 12;
            total_lports = (tmp_instance + 1) * 12;
            break;
        case VTSS_HOST_MODE_1:
            tmp_instance = (lport_no > 23) ? 1 : 0;
            tmp_lport = (tmp_instance) * 24;
            total_lports = (tmp_instance + 1) * 24;
            break;
#endif
        case VTSS_HOST_MODE_2:
            tmp_instance = 0;
            tmp_lport = (tmp_instance) * 24;
            total_lports = (tmp_instance + 1) * 24;
            break;
        case VTSS_HOST_MODE_3:
            tmp_instance = 0;
            tmp_lport = (tmp_instance) * 48;
            total_lports = (tmp_instance + 1) * 48;
            break;
        default:
            rc = VTSS_RC_ERROR;
            break;
    }
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Build lport specific DWRR costs */
    for (tmp = 0; tmp_lport < total_lports; tmp_lport++, tmp++) {
        tmp_lport_conf = &vtss_state->qos_lport[tmp_lport];
        lport_pct[tmp] = tmp_lport_conf->lport_pct;
    }

    VTSS_RC(vtss_cmn_qos_weight2cost(lport_pct, lport_cost, tmp, 7));

    switch (host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
        case VTSS_HOST_MODE_0:
            tmp_lport = (tmp_instance) * 12;
            total_lports = (tmp_instance + 1) * 12;
            break;
        case VTSS_HOST_MODE_1:
            tmp_lport = (tmp_instance) * 24;
            total_lports = (tmp_instance + 1) * 24;
            break;
#endif
        case VTSS_HOST_MODE_2:
            tmp_lport = (tmp_instance) * 24;
            total_lports = (tmp_instance + 1) * 24;
            break;
        case VTSS_HOST_MODE_3:
            tmp_lport = (tmp_instance) * 48;
            total_lports = (tmp_instance + 1) * 48;
            break;
        default:
            rc = VTSS_RC_ERROR;
            break;
    }
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Configure the HW with LPORT costs */
    for (tmp = 0; tmp_lport < total_lports; tmp_lport++, tmp++) {
        JR_WRXF(SCH, HM_HM_DWRR_COST, tmp_lport, DWRR_COST,lport_cost[tmp]);
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_qos_lport_conf_set(const vtss_lport_no_t lport_no)
{
    vtss_qos_lport_conf_t *conf;
    vtss_host_mode_t      host_mode = vtss_state->init_conf.host_mode;
    vtss_rc               rc = VTSS_RC_OK;
    u8                    queue_cnt = 0;
    u8                    queue_cost[6];

    switch(host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
        case VTSS_HOST_MODE_0:
            if (lport_no > 11) {
                rc = VTSS_RC_ERROR;
            }
            break;
        case VTSS_HOST_MODE_1:
            if (lport_no > 23) {
                rc = VTSS_RC_ERROR;
            }
            break;
#endif
        case VTSS_HOST_MODE_2:
            if (lport_no > 23) {
                rc = VTSS_RC_ERROR;
            }
            break;
        case VTSS_HOST_MODE_3:
            if (lport_no > 47) {
                rc = VTSS_RC_ERROR;
            }
            break;
        default:
            rc = VTSS_RC_ERROR;
            break;
    }
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    if (lport_no >= VTSS_LPORTS) { /* To make Lint happy, added this */
        return VTSS_RC_ERROR;
    }

    conf = &vtss_state->qos_lport[lport_no];
    JR_WRXB(SCH, HM_HM_DWRR_AND_LB, lport_no, FRM_ADJ_ENA, 1);

    if (conf->shaper.rate == VTSS_BITRATE_DISABLED) {
        // Set burst level to maximum if shaper is disabled
        JR_WRXF(SCH, HM_LB_HM_LB_THRES, ((3 * lport_no) + 2), LB_THRES,  0x3f);
    }
    else {
        JR_WRXF(SCH, HM_LB_HM_LB_THRES, ((3 * lport_no) + 2), LB_THRES,  MIN(0x3f, ((conf->shaper.level + 4095) / 4096)));
    }

    /* Enable Logical port LB Shaping */
    if (conf->shaper.rate == VTSS_BITRATE_DISABLED) {
        JR_WRXB(SCH, HM_HM_LB_CFG, (3*lport_no)+2, LB_SHAPING_ENA, 0);
    } else {
        JR_WRXB(SCH, HM_HM_LB_CFG, (3*lport_no)+2, LB_SHAPING_ENA, 1);
    }
    /* Enable Logical port Shaper */
    JR_WRXF(SCH, HM_LB_HM_LB_RATE, (3*lport_no)+2, LB_RATE, MIN(0x1ffff,
                VTSS_DIV64(((u64)conf->shaper.rate * 1000) + 100159, 100160)));

    for (queue_cnt = 0; queue_cnt < 2; queue_cnt++) {

        if (conf->scheduler.queue[queue_cnt].rate == VTSS_BITRATE_DISABLED) {
            // Set burst level to maximum if shaper is disabled
            JR_WRXF(SCH, HM_LB_HM_LB_THRES, ((3 * lport_no) + queue_cnt), LB_THRES,  0x3f);
        }
        else {
            JR_WRXF(SCH, HM_LB_HM_LB_THRES, ((3 * lport_no) + queue_cnt), LB_THRES,
                    MIN(0x3f, ((conf->scheduler.queue[queue_cnt].level + 4095) / 4096)));
        }

        /* Enable Q7, Q6 LB Shaping */
        if (conf->scheduler.queue[queue_cnt].rate == VTSS_BITRATE_DISABLED) {
            JR_WRXB(SCH, HM_HM_LB_CFG, (3*lport_no)+queue_cnt, LB_SHAPING_ENA, 0);
        } else {
            JR_WRXB(SCH, HM_HM_LB_CFG, (3*lport_no)+queue_cnt, LB_SHAPING_ENA, 1);
        }
        /* Enable Logical queue Shaper */
        JR_WRXF(SCH, HM_LB_HM_LB_RATE, (3*lport_no)+queue_cnt, LB_RATE, MIN(0x1ffff,
                    VTSS_DIV64(((u64)conf->scheduler.queue[queue_cnt].rate * 1000) + 100159, 100160)));
    }

    /* Enable the DWRR configurations */
    switch (host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
        case VTSS_HOST_MODE_0:
            VTSS_RC(vtss_cmn_qos_weight2cost(conf->scheduler.queue_pct, queue_cost, 6, 7));
            for (queue_cnt = 0; queue_cnt < 6; queue_cnt++) {
                JR_WRXF(SCH, HM_HM_DWRR_COST, (6*lport_no)+24+queue_cnt, DWRR_COST,queue_cost[queue_cnt]);
            }
            break;
        case VTSS_HOST_MODE_1:
            VTSS_RC(vtss_cmn_qos_weight2cost(conf->scheduler.queue_pct, queue_cost, 2, 7));
            for (queue_cnt = 0; queue_cnt < 2; queue_cnt++) {
                JR_WRXF(SCH, HM_HM_DWRR_COST, (2*lport_no)+48+queue_cnt, DWRR_COST,queue_cost[queue_cnt]);
            }
            break;
#endif
        case VTSS_HOST_MODE_2:
            VTSS_RC(vtss_cmn_qos_weight2cost(conf->scheduler.queue_pct, queue_cost, 6, 7));
            for (queue_cnt = 0; queue_cnt < 6; queue_cnt++) {
                JR_WRXF(SCH, HM_HM_DWRR_COST, (6*lport_no)+24+queue_cnt, DWRR_COST,queue_cost[queue_cnt]);
            }
            break;
        case VTSS_HOST_MODE_3:
            VTSS_RC(vtss_cmn_qos_weight2cost(conf->scheduler.queue_pct, queue_cost, 2, 7));
            for (queue_cnt = 0; queue_cnt < 2; queue_cnt++) {
                JR_WRXF(SCH, HM_HM_DWRR_COST, (2*lport_no)+48+queue_cnt, DWRR_COST,queue_cost[queue_cnt]);
            }
            break;
        default:
            break;
    }

    /* Configure the LPORT DWRR cost */
    rc = jr_qos_lport_dwrr_conf_set(lport_no);
    if (rc != VTSS_RC_OK) {
        return rc;
    }
    /* WRED configuration */
    rc = jr_qos_lport_wred_conf_set(lport_no);

    return rc;
}

static vtss_rc jr_qos_lport_conf_get(const vtss_lport_no_t       lport_no,
                                     const vtss_qos_lport_conf_t *const conf)
{
       /* TBD */
       return VTSS_RC_OK;
}

static vtss_rc jr_lport_counters_get(const vtss_lport_no_t lport_no,
                                     vtss_lport_counters_t *const counters)
{
    u8  i, prios;
    u32 qu;
    u32 hm = ce_max_hm();

    if (hm == 0 || hm == 2) {
        prios = 8;
    } else {
        prios = 4;
    }
    if (lport_is_valid(lport_no) == FALSE) {
        return VTSS_RC_ERROR;
    }
    if (lport_no >= VTSS_LPORTS) {
        return VTSS_RC_ERROR;
    }
    qu = 256 + (lport_no) * prios;
    for (i = 0; i < prios; i++) {
        /* Update the chip counters */
        JR_CNT_HOST_SQS(OQS, TX, GREEN, qu,  &vtss_state->lport_chip_counters[lport_no].tx_green[i], 0);
        JR_CNT_HOST_SQS(OQS, TX, YELLOW, qu, &vtss_state->lport_chip_counters[lport_no].tx_yellow[i], 0);
        JR_CNT_HOST_SQS(OQS, RX, QDROPS, qu, &vtss_state->lport_chip_counters[lport_no].rx_drops[i], 0);
        JR_CNT_HOST_SQS(OQS, RX, GREEN, qu, &vtss_state->lport_chip_counters[lport_no].rx_green[i], 0);
        JR_CNT_HOST_SQS(OQS, RX, YELLOW, qu, &vtss_state->lport_chip_counters[lport_no].rx_yellow[i], 0);

        /* Tx Counters */
        counters->tx_green[i] = vtss_state->lport_chip_counters[lport_no].tx_green[i].value;
        counters->tx_yellow[i] = vtss_state->lport_chip_counters[lport_no].tx_yellow[i].value;

        /* Rx Counters */
        counters->rx_green[i] = vtss_state->lport_chip_counters[lport_no].rx_green[i].value;
        counters->rx_yellow[i] = vtss_state->lport_chip_counters[lport_no].rx_yellow[i].value;
        counters->rx_drops[i] = vtss_state->lport_chip_counters[lport_no].rx_drops[i].value;
        qu++;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_lport_counters_clear(const vtss_lport_no_t lport_no)
{
    u8  i, prios;
    u32 qu;
    u32 hm = ce_max_hm();

    if (hm == 0 || hm == 2) {
        prios = 8;
    } else {
        prios = 4;
    }

    if (lport_is_valid(lport_no) == FALSE) {
        return VTSS_RC_ERROR;
    }
    if (lport_no >= VTSS_LPORTS) {
        return VTSS_RC_ERROR;
    }
    qu = 256 + (lport_no) * prios;
    for (i = 0; i < prios; i++) {
        /* Update the chip counters */
        JR_CNT_HOST_SQS(OQS, TX, GREEN, qu,  &vtss_state->lport_chip_counters[lport_no].tx_green[i], 1);
        JR_CNT_HOST_SQS(OQS, TX, YELLOW, qu, &vtss_state->lport_chip_counters[lport_no].tx_yellow[i], 1);
        JR_CNT_HOST_SQS(OQS, RX, QDROPS, qu, &vtss_state->lport_chip_counters[lport_no].rx_drops[i], 1);
        JR_CNT_HOST_SQS(OQS, RX, GREEN, qu, &vtss_state->lport_chip_counters[lport_no].rx_green[i], 1);
        JR_CNT_HOST_SQS(OQS, RX, YELLOW, qu, &vtss_state->lport_chip_counters[lport_no].rx_yellow[i], 1);

        qu++;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_init_cemax_oobfc_conf_set(const u8 host_instance,  const BOOL fc_enable)
{
    u32 i, host_mode;
    u32 tmp_port, tmp_prio, tmp_instance, total_queues, total_instaces, total_ports, max_queues;
    u32 total_line_queues, total_host_queues;
    u8  inc_val;
    u32 enable_mask, tmp_mask;


    host_mode = ce_max_hm();

    switch(host_mode) {
#if defined(VTSS_CHIP_CE_MAX_24)
        case VTSS_HOST_MODE_0:
            total_instaces = 1;
            total_ports = 12; /* Total ports per instance */
            total_queues = total_line_queues = total_host_queues = 8;
            inc_val = 1;
            break;
        case VTSS_HOST_MODE_1:
            total_instaces = 1;
            total_line_queues = 8;
            total_queues = total_host_queues = 4;
            total_ports = 12;
            inc_val = 2;
            break;
#endif
        case VTSS_HOST_MODE_2:
            total_instaces = 1;
            total_queues = total_line_queues = total_host_queues = 8;
            total_ports = 24;
            inc_val = 1;
#if defined(VTSS_CHIP_CE_MAX_12)
            total_ports = 12;
#endif
            break;
        case VTSS_HOST_MODE_3:
            total_instaces = 1;
            total_line_queues = 8;
            total_queues = total_host_queues = 4;
            total_ports = 24;
            inc_val = 2;
#if defined(VTSS_CHIP_CE_MAX_12)
            total_ports = 12;
#endif
            break;
        default :
            total_instaces = 0;
            total_ports = 12; /* Total ports per instance */
            total_queues = total_line_queues = total_host_queues = 8;
            inc_val = 1;
            break;
    }

    if (total_instaces == 0) {
        return VTSS_RC_ERROR;
    }

    if (host_instance > 0 && ((host_mode == VTSS_HOST_MODE_2 || host_mode == VTSS_HOST_MODE_3))) {
        return VTSS_RC_ERROR;
    }

    if (fc_enable == TRUE) {
        if (host_instance == 0) {
            if (host_mode != VTSS_HOST_MODE_2 && host_mode != VTSS_HOST_MODE_3) {
                enable_mask = 0x00000fff;
            } else {
                enable_mask = 0xffffffff;
            }
        } else {
            enable_mask = 0x00fff000;
        }
        /* Enable Status Capture on port-level for line ports */
        JR_RD(DSM, CM_CFG_CMM_MASK_CFG, &tmp_mask);
        tmp_mask |= enable_mask;
        JR_WR(DSM, CM_CFG_CMM_MASK_CFG, tmp_mask);

        /* Enable Status Capture on Queue-level for line port queues */
        for (i = 0; i < 8; ) {
            JR_RDX(DSM, CM_CFG_CMM_QU_MASK_CFG, i, &tmp_mask);
            tmp_mask |= enable_mask;
            JR_WRX(DSM, CM_CFG_CMM_QU_MASK_CFG, i, tmp_mask);
            i++;
        }
        //For the status channel registers configure which UPSID, they shall capture
        // status for.
        {
            u8 upsid;
            u8 total_upsid;
            u8 tmp;

            if (host_instance == 0) {
                upsid = 8;
                total_upsid = 8;
                if (host_mode != VTSS_HOST_MODE_2 && host_mode != VTSS_HOST_MODE_3) {
                    total_upsid = 4;
                }
            } else {
                upsid = 12;
                total_upsid = 4;
            }
            for (tmp = 0; tmp <= total_upsid; tmp++) {
                JR_WRX(DSM, HOST_IF_IBS_COMMON_CFG_CAPTURE_UPSID_CFG, tmp, upsid);
                upsid++;
            }
            JR_WRX(DSM, HOST_IF_IBS_COMMON_CFG_CAPTURE_UPSID_CFG, 8, 0);
        }

        for (tmp_instance = 0; tmp_instance < total_instaces; tmp_instance++) {
            // Select clock and release reset write
            JR_RDX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, &i);
            i &= (~VTSS_M_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CLK_SEL);
            i |= VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CLK_SEL(0);
            JR_WRX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, i);
            JR_RDX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, &i);
            i &= (~VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_RST);
            JR_WRX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, i);

            /* == Enable the Calendar entries for first 12 ports and queues == */
            tmp_port = host_instance * total_ports;
#if defined(VTSS_CHIP_CE_MAX_12)
            tmp_port = 11;
#endif
            for (i = 0; i < total_ports; i++) {
                JR_WRXY(DSM, HOST_IF_CONF_IBS_CAL_XAUI_IBS_CAL, i, host_instance, (32*total_queues)+tmp_port);
                tmp_port++;
            }
            tmp_port = host_instance * total_ports;
#if defined(VTSS_CHIP_CE_MAX_12)
            tmp_port = 11;
#endif

            tmp_prio = 0;
            max_queues = (total_ports + total_ports*total_queues);
            for (i = total_ports; i < max_queues; i++) {
                JR_WRXY(DSM, HOST_IF_CONF_IBS_CAL_XAUI_IBS_CAL, i, host_instance, tmp_port+(32*tmp_prio));
                tmp_prio = (tmp_prio+inc_val)%total_line_queues;
                if (tmp_prio == 0) {
                    tmp_port++;
                }
            }

            JR_RDX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, &i);

            /* Set Calendar length */
            i &= (~VTSS_M_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CAL_LEN);
            i |= VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CAL_LEN(total_ports*(1+total_queues));
            i |= VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CAL_M(1);
            /* Start Status Channel */
            i &= (~VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_FORCE_IDLE);
            JR_WRX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, i);

            /* == Configuration settings to receive the filling status of line port OQS == */
            JR_RDX(DSM, HOST_IF_OBS_CFG_XAUI_OBS_CFG, host_instance, &i);
            i &= ~(VTSS_F_DSM_HOST_IF_OBS_CFG_XAUI_OBS_CFG_CONF_OBS_RST);
            JR_WRX(DSM, HOST_IF_OBS_CFG_XAUI_OBS_CFG, host_instance, i);

            tmp_port = host_instance * total_ports;
#if defined(VTSS_CHIP_CE_MAX_12)
            tmp_port = 11;
#endif
            /* Setup OBS flow control Calendar */
            for (i = 0; i < total_ports; i++) {
                JR_WRXY(DSM, HOST_IF_CONF_OBS_CAL_XAUI_OBS_CAL, i, host_instance, 192+tmp_port);
                tmp_port++;
            }
            tmp_prio = 0;
            tmp_port = host_instance * total_ports;
#if defined(VTSS_CHIP_CE_MAX_12)
            tmp_port = 0; /* Host queue starts from 0 again */
#endif
            for (i = total_ports; i < total_ports*(1+total_host_queues); i++) {
                JR_WRXY(DSM, HOST_IF_CONF_OBS_CAL_XAUI_OBS_CAL, i, host_instance, tmp_port*total_host_queues+tmp_prio);
                tmp_prio = (tmp_prio+1)%total_host_queues;
                if (tmp_prio == 0) {
                    tmp_port++;
                }
            }
            JR_RDX(DSM, HOST_IF_OBS_CFG_XAUI_OBS_CFG, host_instance, &i);
            /* Set Calendar length */
            i |= VTSS_F_DSM_HOST_IF_OBS_CFG_XAUI_OBS_CFG_CONF_OBS_CAL_LEN(total_ports*(1+total_host_queues));
            i |= VTSS_F_DSM_HOST_IF_OBS_CFG_XAUI_OBS_CFG_CONF_OBS_CAL_M(1);
            JR_WRX(DSM, HOST_IF_OBS_CFG_XAUI_OBS_CFG, host_instance, i);
        }
    } else {

        if (host_instance == 0) {
            enable_mask = 0x00000000;
            if (host_mode != VTSS_HOST_MODE_2 && host_mode != VTSS_HOST_MODE_3) {
                enable_mask = 0x00fff000;
            }
        } else {
            enable_mask = 0x00000fff;
        }

        /* Enable Status Capture on port-level for all line ports */
        JR_RD(DSM, CM_CFG_CMM_MASK_CFG, &tmp_mask);
        tmp_mask &= enable_mask;
        JR_WR(DSM, CM_CFG_CMM_MASK_CFG, tmp_mask);

        /* Enable Status Capture on Queue-level for line port queues */
        for (i = 0; i < 8; ) {
            JR_RDX(DSM, CM_CFG_CMM_QU_MASK_CFG, i, &tmp_mask);
            tmp_mask &= enable_mask;
            JR_WRX(DSM, CM_CFG_CMM_QU_MASK_CFG, i, tmp_mask);
            i += inc_val;
        }

        //For the status channel registers configure which UPSID, they shall capture
        // status for.
        {
            u8 total_upsid;
            u8 tmp;

            if (host_instance == 0) {
                total_upsid = 8;
                if (host_mode != VTSS_HOST_MODE_2 && host_mode != VTSS_HOST_MODE_3) {
                    total_upsid = 4;
                }
            } else {
                total_upsid = 4;
            }
            for (tmp = 0; tmp <= total_upsid; tmp++) {
                JR_WRX(DSM, HOST_IF_IBS_COMMON_CFG_CAPTURE_UPSID_CFG, tmp, 0);
            }
            JR_WRX(DSM, HOST_IF_IBS_COMMON_CFG_CAPTURE_UPSID_CFG, 8, 0);
        }

        for (tmp_instance = 0; tmp_instance < total_instaces; tmp_instance++) {
            // Select clock and release reset write
            JR_RDX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, &i);
            i &= (~VTSS_M_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CLK_SEL);
            i |= VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CLK_SEL(3);
            JR_WRX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, i);
            JR_RDX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, &i);
            i |= (VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_RST);
            JR_WRX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, i);

            /* == Enable the Calendar entries for first 12 ports and queues == */
            tmp_port = host_instance * total_ports;
#if defined(VTSS_CHIP_CE_MAX_12)
            tmp_port = 11;
#endif
            for (i = 0; i < total_ports; i++) {
                JR_WRXY(DSM, HOST_IF_CONF_IBS_CAL_XAUI_IBS_CAL, i, host_instance, 0);
                tmp_port++;
            }
            tmp_port = host_instance * total_ports;
#if defined(VTSS_CHIP_CE_MAX_12)
            tmp_port = 11;
#endif
            tmp_prio = 0;
            max_queues = (total_ports + total_ports*total_queues);
            for (i = total_ports; i < max_queues; i++) {
                JR_WRXY(DSM, HOST_IF_CONF_IBS_CAL_XAUI_IBS_CAL, i, host_instance, 0);
                tmp_prio = (tmp_prio+inc_val)%total_line_queues;
                if (tmp_prio == 0) {
                    tmp_port++;
                }
            }

            JR_RDX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, &i);

            /* Set Calendar length */
            i &= (~VTSS_M_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CAL_LEN);
            i |= VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CAL_LEN(0);
            i |= VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_CAL_M(0);
            /* Start Status Channel */
            i |= (VTSS_F_DSM_HOST_IF_IBS_CFG_XAUI_IBS_CFG_CONF_IBS_FORCE_IDLE);
            JR_WRX(DSM, HOST_IF_IBS_CFG_XAUI_IBS_CFG, host_instance, i);

            /* == Configuration settings to receive the filling status of line port OQS == */
            JR_RDX(DSM, HOST_IF_OBS_CFG_XAUI_OBS_CFG, host_instance, &i);
            i |= (VTSS_F_DSM_HOST_IF_OBS_CFG_XAUI_OBS_CFG_CONF_OBS_RST);
            JR_WRX(DSM, HOST_IF_OBS_CFG_XAUI_OBS_CFG, host_instance, i);

            tmp_port = 0;
            /* Setup flow control Calendar */
            for (tmp_port = 0; tmp_port < total_ports ; tmp_port++) {
                JR_WRXY(DSM, HOST_IF_CONF_OBS_CAL_XAUI_OBS_CAL, tmp_port, host_instance, 0);
            }
            tmp_prio = 0;
            tmp_port = 0;
            for (i = total_ports; i < total_ports*(1+total_host_queues); i++) {
                JR_WRXY(DSM, HOST_IF_CONF_OBS_CAL_XAUI_OBS_CAL, i, host_instance, 0);
                tmp_prio = (tmp_prio+1)%total_host_queues;
                if (tmp_prio == 0) {
                    tmp_port++;
                }
            }
            JR_RDX(DSM, HOST_IF_OBS_CFG_XAUI_OBS_CFG, host_instance, &i);
            /* Set Calendar length */
            i &= (~VTSS_M_DSM_HOST_IF_OBS_CFG_XAUI_OBS_CFG_CONF_OBS_CAL_LEN);
            i |= VTSS_F_DSM_HOST_IF_OBS_CFG_XAUI_OBS_CFG_CONF_OBS_CAL_LEN(0);
            i |= VTSS_F_DSM_HOST_IF_OBS_CFG_XAUI_OBS_CFG_CONF_OBS_CAL_M(0);
            JR_WRX(DSM, HOST_IF_OBS_CFG_XAUI_OBS_CFG, host_instance, i);
        }
    }
    return VTSS_RC_OK;
}

static BOOL jr_port_is_host(const vtss_port_no_t port_no)
{
    return (VTSS_CHIP_PORT(port_no) == ce_mac_hmda || VTSS_CHIP_PORT(port_no) == ce_mac_hmdb);
}


#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

/* Set 1000Base-X Fiber Auto-negotiation (Clause 37) */
static vtss_rc jr_port_clause_37_control_set(const vtss_port_no_t port_no)
{
    vtss_port_clause_37_control_t *control = &vtss_state->port_clause_37[port_no];
    u32 value, port = VTSS_CHIP_PORT(port_no);

    /* Aneg capabilities for this port */
    VTSS_RC(vtss_cmn_port_clause_37_adv_set(&value, &control->advertisement, control->enable));

    if (VTSS_PORT_IS_1G(port)) {
        /* Set aneg capabilities, enable neg and restart */
        JR_WRX(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, VTSS_TO_DEV(port), 
               JR_PUT_FLD(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ADV_ABILITY, value) |
               JR_PUT_BIT(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ANEG_ENA, 1) | 
               JR_PUT_BIT(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ANEG_RESTART_ONE_SHOT, 1));

        if (!control->enable) {
            /* Disable Aneg */
            JR_WRX(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, VTSS_TO_DEV(port), 
                   JR_PUT_FLD(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ADV_ABILITY, 0) |
                   JR_PUT_BIT(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ANEG_ENA, 0) | 
                   JR_PUT_BIT(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ANEG_RESTART_ONE_SHOT, 1));
         }
 
    } else {
        JR_WRX(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, VTSS_TO_DEV(port), 
               JR_PUT_FLD(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ADV_ABILITY, value) |
               JR_PUT_BIT(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ANEG_ENA, 1) | 
               JR_PUT_BIT(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ANEG_RESTART_ONE_SHOT, 1)); 

        if (!control->enable) {
            JR_WRX(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, VTSS_TO_DEV(port), 
                   JR_PUT_FLD(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ADV_ABILITY, 0) |
                   JR_PUT_BIT(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ANEG_ENA, 0) | 
                   JR_PUT_BIT(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_CFG, ANEG_RESTART_ONE_SHOT, 1)); 
        }
    }
    return VTSS_RC_OK;
}

/* Get 1000Base-X Fiber Auto-negotiation status (Clause 37) */
static vtss_rc jr_port_clause_37_status_get(const vtss_port_no_t         port_no,
                                            vtss_port_clause_37_status_t *const status)
    
{

    u32 value, port = VTSS_CHIP_PORT(port_no);

    if (vtss_state->port_conf[port_no].power_down) {
        status->link = 0;
        return VTSS_RC_OK;
    }

    if (VTSS_PORT_IS_1G(port)) {
        /* Get the link state 'down' sticky bit  */
        JR_RDX(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, VTSS_TO_DEV(port), &value);
        value = VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY & value; 

        if (VTSS_BOOL(value)) {
            /* The link has been down. Clear the sticky bit and return the 'down' value  */
            JR_WRXB(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, VTSS_TO_DEV(port), LINK_DOWN_STICKY, 1);
            JR_WRXB(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, VTSS_TO_DEV(port), OUT_OF_SYNC_STICKY, 1);
            status->link = 0;
        } else {
            /*  Return the current status     */
            JR_RDX(DEV1G, PCS1G_CFG_STATUS_PCS1G_LINK_STATUS, VTSS_TO_DEV(port), &value);
            status->link = VTSS_BOOL(VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS & value) && 
                           VTSS_BOOL(VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_SYNC_STATUS & value);
        }        
        /* Get PCS ANEG status register */
        JR_RDX(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS, VTSS_TO_DEV(port), &value);           
        /* Get 'Aneg complete'   */
        status->autoneg.complete = VTSS_BOOL(value & VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE);        

        /* Workaround for a Serdes issue, when aneg completes with FDX capability=0 */
        if (vtss_state->port_conf[port_no].if_type == VTSS_PORT_INTERFACE_SERDES) {
            if (status->autoneg.complete) {
                if (((value >> 21) & 0x1) == 0) { 
                    JR_WRXB(DEV1G, PCS1G_CFG_STATUS_PCS1G_CFG, VTSS_TO_DEV(port), PCS_ENA, 0);
                    JR_WRXB(DEV1G, PCS1G_CFG_STATUS_PCS1G_CFG, VTSS_TO_DEV(port), PCS_ENA, 1);
                    (void)jr_port_clause_37_control_set(port_no); /* Restart Aneg */
                    VTSS_MSLEEP(50);
                    JR_RDX(DEV1G, PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS, VTSS_TO_DEV(port), &value);           
                    status->autoneg.complete = VTSS_BOOL(value & VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE);        
                }
            }
        }

        /* Return partner advertisement ability */
        value = VTSS_X_DEV1G_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_LP_ADV_ABILITY(value);        
    } else {
        /* 10G in 1G mode */
        JR_RDX(DEV10G, PCS1G_CFG_STATUS_PCS1G_STICKY, VTSS_TO_DEV(port), &value);
        value = (VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY & value); 
        if (VTSS_BOOL(value)) {
            /* The link has been down. Clear the sticky bit and return the 'down' value  */
            JR_WRXB(DEV10G, PCS1G_CFG_STATUS_PCS1G_STICKY, VTSS_TO_DEV(port), LINK_DOWN_STICKY, 1);
            JR_WRXB(DEV10G, PCS1G_CFG_STATUS_PCS1G_STICKY, VTSS_TO_DEV(port), OUT_OF_SYNC_STICKY, 1);
            status->link = 0;
        } else {
            /*  Return the current status     */
            JR_RDX(DEV10G, PCS1G_CFG_STATUS_PCS1G_LINK_STATUS, VTSS_TO_DEV(port), &value);
            status->link = VTSS_BOOL(VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS & value) && 
                           VTSS_BOOL(VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_SYNC_STATUS & value);        
        }        
        /* Get PCS ANEG status register */
        JR_RDX(DEV10G, PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS, VTSS_TO_DEV(port), &value);           
        /* Get 'Aneg complete'   */
        status->autoneg.complete = VTSS_BOOL(value & VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE);        
        /* Return partner advertisement ability */
        value = VTSS_X_DEV10G_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_LP_ADV_ABILITY(value);
    }

    if (vtss_state->port_conf[port_no].if_type == VTSS_PORT_INTERFACE_SGMII_CISCO) {
        status->autoneg.partner_adv_sgmii.link = ((value >> 15) == 1) ? 1 : 0;
        status->autoneg.partner_adv_sgmii.speed_10M = (((value >> 10) & 3) == 0) ? 1 : 0;
        status->autoneg.partner_adv_sgmii.speed_100M =(((value >> 10) & 3) == 1) ? 1 : 0;
        status->autoneg.partner_adv_sgmii.speed_1G =  (((value >> 10) & 3) == 2) ? 1 : 0;
        status->autoneg.partner_adv_sgmii.fdx = (((value >> 12) & 0x1) == 1) ? 1 : 0;
        status->autoneg.partner_adv_sgmii.hdx = status->autoneg.partner_advertisement.fdx ? 0 : 1;
        if (status->link) {
            /* If the SFP module does not have a link then the port does not have link */
            status->link = status->autoneg.partner_adv_sgmii.link;
        }
    } else {
        VTSS_RC(vtss_cmn_port_clause_37_adv_get(value, &status->autoneg.partner_advertisement));
    }


    return VTSS_RC_OK;
}

/* Get status of the XAUI, VAUI or 100FX ports. */
/* Status for SERDES and SGMII ports is handled elsewhere (through autonegotiation) */
static vtss_rc jr_port_status_get(const vtss_port_no_t  port_no,
                                  vtss_port_status_t    *const status)
{
    u32              addr, value;
    u32              port = VTSS_CHIP_PORT(port_no);
    u32              tgt = VTSS_TO_DEV(port);
    vtss_port_conf_t *conf = &vtss_state->port_conf[port_no];
    
    if (conf->power_down) {
        /* Disabled port is considered down */
        return VTSS_RC_OK;
    }

    switch (vtss_state->port_conf[port_no].if_type) {
    case VTSS_PORT_INTERFACE_100FX:
        /* Get the PCS status  */
        JR_RDX(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, tgt, &value);
        
        /* Link has been down if the are any error stickies */
        status->link_down = JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, SYNC_LOST_STICKY, value) ||
                            JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, SSD_ERROR_STICKY, value) ||
                            JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, FEF_FOUND_STICKY, value) ||
                            JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, PCS_ERROR_STICKY, value);

        if (status->link_down) {
            JR_WRX(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, tgt, 0xFFFF);
            JR_RDX(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, tgt, &value);
        }
        /* Link=1 if sync status=1 and no error stickies after a clear */
        status->link = JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, SYNC_STATUS, value) && 
                     (!JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, SYNC_LOST_STICKY, value) &&
                      !JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, FEF_FOUND_STICKY, value) &&
                      !JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, PCS_ERROR_STICKY, value) &&
                      !JR_GET_BIT(DEV1G, PCS_FX100_STATUS_PCS_FX100_STATUS, PCS_ERROR_STICKY, value));

        status->speed = VTSS_SPEED_100M; 
        break;
    case VTSS_PORT_INTERFACE_XAUI:
    case VTSS_PORT_INTERFACE_RXAUI:
        /* MAC10G Tx Monitor Sticky bit Register */
        addr = VTSS_IOREG(tgt, 0x1b);
        VTSS_RC(jr_rd(addr, &value));
        if (value != 2) {
            /* The link is or has been down. Clear the sticky bit */
            status->link_down = 1;
            VTSS_RC(jr_wr(addr, 0x1F));
            VTSS_RC(jr_rd(addr, &value));
        }
        /* IDLE_STATE_STICKY = 1 and no others stickys --> link on */
        status->link = (value == 2 ? 1 : 0);    
        status->speed = conf->speed;
        break;
    case VTSS_PORT_INTERFACE_VAUI:
        if (VTSS_PORT_IS_1G(port)) {
            /* Get the PCS status */
            JR_RDX(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, &value);
            if (JR_GET_BIT(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, LINK_DOWN_STICKY, value)) {
                /* The link has been down. Clear the sticky bit */
                status->link_down = 1;
                JR_WRXB(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, LINK_DOWN_STICKY, 1);
                JR_WRXB(DEV1G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, OUT_OF_SYNC_STICKY, 1);
            }
            JR_RDX(DEV1G, PCS1G_CFG_STATUS_PCS1G_LINK_STATUS, tgt, &value);
            status->link = VTSS_BOOL(VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS & value) && 
                           VTSS_BOOL(VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_SYNC_STATUS & value);        

            status->speed = VTSS_SPEED_2500M;
        } else {
            /* 10G in 2500M mode */
            JR_RDX(DEV10G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, &value);
            value = (VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY & value);        
            if (VTSS_BOOL(value)) {
                /* The link has been down. Clear the sticky bit and return the 'down' value  */
                JR_WRXB(DEV10G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, LINK_DOWN_STICKY, 1);
                JR_WRXB(DEV10G, PCS1G_CFG_STATUS_PCS1G_STICKY, tgt, OUT_OF_SYNC_STICKY, 1);
                status->link_down = 1;
            }        
            /*  Return the current status     */
            JR_RDX(DEV10G, PCS1G_CFG_STATUS_PCS1G_LINK_STATUS, tgt, &value);
            status->link = VTSS_BOOL(VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS & value) && 
                           VTSS_BOOL(VTSS_F_DEV10G_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_SYNC_STATUS & value);        
            status->speed = VTSS_SPEED_2500M;

        }
        break;
    case VTSS_PORT_INTERFACE_NO_CONNECTION:
        status->link = 0;
        status->link_down = 0;
        break;
    default:
        VTSS_E("Status not supported for port: %u", port_no);
        return VTSS_RC_ERROR;
    }
    status->fdx = conf->fdx;
    return VTSS_RC_OK;
}


#define JR_CNT_SQS(tgt, xmit, cnt_id, port, prio, cnt, clr)                            \
{                                                                                      \
    u32 qu, value;                                                                     \
    qu = (port * 8 + prio);                                                            \
    JR_RDXY(tgt, STAT_CNT_CFG_##xmit##_STAT_LSB_CNT, qu, JR_CNT(xmit, cnt_id), &value); \
    vtss_cmn_counter_32_update(value, cnt, clr);                                       \
}                                                                                      \


/* Handle 1G counter in ASM */
#define JR_CNT_1G(name, i, cnt, clr)                     \
{                                                        \
    u32 value;                                           \
    JR_RDX(ASM, DEV_STATISTICS_##name##_CNT, i, &value); \
    vtss_cmn_counter_32_update(value, cnt, clr);         \
}

/* Handle 10G counter in DEV10G */
#define JR_CNT_10G(name, i, cnt, clr)                       \
{                                                           \
    u32 value;                                              \
    JR_RDX(DEV10G, DEV_STATISTICS_##name##_CNT, i, &value); \
    vtss_cmn_counter_32_update(value, cnt, clr);            \
}


/* Update/clear/get port counters for chip */
static vtss_rc jr_port_counters_chip(u32 port, 
                                     vtss_port_jr1_counters_t *c,
                                     vtss_port_counters_t     *const counters,
                                     BOOL                      clr)
{
    u32                                i;
    vtss_port_counter_t                rx_errors;
    vtss_port_rmon_counters_t          *rmon;
    vtss_port_if_group_counters_t      *if_group;
    vtss_port_ethernet_like_counters_t *elike;
    vtss_port_proprietary_counters_t   *prop;

    if (VTSS_PORT_IS_1G(port)) {
        i = (port == VTSS_PORT_NPI ? 27 : port);
        JR_CNT_1G(RX_IN_BYTES, i, &c->rx_in_bytes, clr);
        JR_CNT_1G(RX_SYMBOL_ERR, i, &c->rx_symbol_err, clr);
        JR_CNT_1G(RX_PAUSE, i, &c->rx_pause, clr);
        JR_CNT_1G(RX_UNSUP_OPCODE, i, &c->rx_unsup_opcode, clr);
        JR_CNT_1G(RX_OK_BYTES, i, &c->rx_ok_bytes, clr);
        JR_CNT_1G(RX_BAD_BYTES, i, &c->rx_bad_bytes, clr);
        JR_CNT_1G(RX_UC, i, &c->rx_unicast, clr);
        JR_CNT_1G(RX_MC, i, &c->rx_multicast, clr);
        JR_CNT_1G(RX_BC, i, &c->rx_broadcast, clr);
        JR_CNT_1G(RX_CRC_ERR, i, &c->rx_crc_err, clr);
        JR_CNT_1G(RX_UNDERSIZE, i, &c->rx_undersize, clr);
        JR_CNT_1G(RX_FRAGMENTS, i, &c->rx_fragments, clr);
        JR_CNT_1G(RX_IN_RANGE_LEN_ERR, i, &c->rx_in_range_len_err, clr);
        JR_CNT_1G(RX_OUT_OF_RANGE_LEN_ERR, i, &c->rx_out_of_range_len_err, clr);
        JR_CNT_1G(RX_OVERSIZE, i, &c->rx_oversize, clr);
        JR_CNT_1G(RX_JABBERS, i, &c->rx_jabbers, clr);
        JR_CNT_1G(RX_SIZE64, i, &c->rx_size64, clr);
        JR_CNT_1G(RX_SIZE65TO127, i, &c->rx_size65_127, clr);
        JR_CNT_1G(RX_SIZE128TO255, i, &c->rx_size128_255, clr);
        JR_CNT_1G(RX_SIZE256TO511, i, &c->rx_size256_511, clr);
        JR_CNT_1G(RX_SIZE512TO1023, i, &c->rx_size512_1023, clr);
        JR_CNT_1G(RX_SIZE1024TO1518, i, &c->rx_size1024_1518, clr);
        JR_CNT_1G(RX_SIZE1519TOMAX, i, &c->rx_size1519_max, clr);

        JR_CNT_1G(TX_OUT_BYTES, i, &c->tx_out_bytes, clr);
        JR_CNT_1G(TX_PAUSE, i, &c->tx_pause, clr);
        JR_CNT_1G(TX_OK_BYTES, i, &c->tx_ok_bytes, clr);
        JR_CNT_1G(TX_UC, i, &c->tx_unicast, clr);
        JR_CNT_1G(TX_MC, i, &c->tx_multicast, clr);
        JR_CNT_1G(TX_BC, i, &c->tx_broadcast, clr);
        JR_CNT_1G(TX_SIZE64, i, &c->tx_size64, clr);
        JR_CNT_1G(TX_SIZE65TO127, i, &c->tx_size65_127, clr);
        JR_CNT_1G(TX_SIZE128TO255, i, &c->tx_size128_255, clr);
        JR_CNT_1G(TX_SIZE256TO511, i, &c->tx_size256_511, clr);
        JR_CNT_1G(TX_SIZE512TO1023, i, &c->tx_size512_1023, clr);
        JR_CNT_1G(TX_SIZE1024TO1518, i, &c->tx_size1024_1518, clr);
        JR_CNT_1G(TX_SIZE1519TOMAX, i, &c->tx_size1519_max, clr);
        JR_CNT_1G(TX_MULTI_COLL, i, &c->tx_multi_coll, clr);
        JR_CNT_1G(TX_LATE_COLL, i, &c->tx_late_coll, clr);
        JR_CNT_1G(TX_XCOLL, i, &c->tx_xcoll, clr);
        JR_CNT_1G(TX_DEFER, i, &c->tx_defer, clr);
        JR_CNT_1G(TX_XDEFER, i, &c->tx_xdefer, clr);
        JR_CNT_1G(TX_BACKOFF1, i, &c->tx_backoff1, clr);
    } else {
        i = VTSS_TO_DEV(port);
        JR_CNT_10G(40BIT_RX_IN_BYTES, i, &c->rx_in_bytes, clr);
        JR_CNT_10G(32BIT_RX_SYMBOL_ERR, i, &c->rx_symbol_err, clr);
        JR_CNT_10G(32BIT_RX_PAUSE, i, &c->rx_pause, clr);
        JR_CNT_10G(32BIT_RX_UNSUP_OPCODE, i, &c->rx_unsup_opcode, clr);
        JR_CNT_10G(40BIT_RX_OK_BYTES, i, &c->rx_ok_bytes, clr);
        JR_CNT_10G(40BIT_RX_BAD_BYTES, i, &c->rx_bad_bytes, clr);
        JR_CNT_10G(32BIT_RX_UC, i, &c->rx_unicast, clr);
        JR_CNT_10G(32BIT_RX_MC, i, &c->rx_multicast, clr);
        JR_CNT_10G(32BIT_RX_BC, i, &c->rx_broadcast, clr);
        JR_CNT_10G(32BIT_RX_CRC_ERR, i, &c->rx_crc_err, clr);
        JR_CNT_10G(32BIT_RX_UNDERSIZE, i, &c->rx_undersize, clr);
        JR_CNT_10G(32BIT_RX_FRAGMENTS, i, &c->rx_fragments, clr);
        JR_CNT_10G(32BIT_RX_IN_RANGE_LEN_ERR, i, &c->rx_in_range_len_err, clr);
        JR_CNT_10G(32BIT_RX_OUT_OF_RANGE_LEN_ERR, i, &c->rx_out_of_range_len_err, clr);
        JR_CNT_10G(32BIT_RX_OVERSIZE, i, &c->rx_oversize, clr);
        JR_CNT_10G(32BIT_RX_JABBERS, i, &c->rx_jabbers, clr);
        JR_CNT_10G(32BIT_RX_SIZE64, i, &c->rx_size64, clr);
        JR_CNT_10G(32BIT_RX_SIZE65TO127, i, &c->rx_size65_127, clr);
        JR_CNT_10G(32BIT_RX_SIZE128TO255, i, &c->rx_size128_255, clr);
        JR_CNT_10G(32BIT_RX_SIZE256TO511, i, &c->rx_size256_511, clr);
        JR_CNT_10G(32BIT_RX_SIZE512TO1023, i, &c->rx_size512_1023, clr);
        JR_CNT_10G(32BIT_RX_SIZE1024TO1518, i, &c->rx_size1024_1518, clr);
        JR_CNT_10G(32BIT_RX_SIZE1519TOMAX, i, &c->rx_size1519_max, clr);

        JR_CNT_10G(40BIT_TX_OUT_BYTES, i, &c->tx_out_bytes, clr);
        JR_CNT_10G(32BIT_TX_PAUSE, i, &c->tx_pause, clr);
        JR_CNT_10G(40BIT_TX_OK_BYTES, i, &c->tx_ok_bytes, clr);
        JR_CNT_10G(32BIT_TX_UC, i, &c->tx_unicast, clr);
        JR_CNT_10G(32BIT_TX_MC, i, &c->tx_multicast, clr);
        JR_CNT_10G(32BIT_TX_BC, i, &c->tx_broadcast, clr);
        JR_CNT_10G(32BIT_TX_SIZE64, i, &c->tx_size64, clr);
        JR_CNT_10G(32BIT_TX_SIZE65TO127, i, &c->tx_size65_127, clr);
        JR_CNT_10G(32BIT_TX_SIZE128TO255, i, &c->tx_size128_255, clr);
        JR_CNT_10G(32BIT_TX_SIZE256TO511, i, &c->tx_size256_511, clr);
        JR_CNT_10G(32BIT_TX_SIZE512TO1023, i, &c->tx_size512_1023, clr);
        JR_CNT_10G(32BIT_TX_SIZE1024TO1518, i, &c->tx_size1024_1518, clr);
        JR_CNT_10G(32BIT_TX_SIZE1519TOMAX, i, &c->tx_size1519_max, clr);
    }

    /* SQS counters */
    for (i = 0; i < VTSS_PRIOS; i++) {
        JR_CNT_SQS(IQS, RX, GREEN, port, i, &c->rx_green_class[i], clr);        
        JR_CNT_SQS(IQS, RX, YELLOW, port, i, &c->rx_yellow_class[i], clr);
        JR_CNT_SQS(IQS, RX, QDROPS, port, i, &c->rx_queue_drops[i], clr);
        JR_CNT_SQS(IQS, RX, PDROPS, port, i, &c->rx_policer_drops[i], clr);
        JR_CNT_SQS(IQS, TX, DROPS, port, i, &c->rx_txqueue_drops[i], clr);

        JR_CNT_SQS(OQS, RX, QDROPS, port, i, &c->tx_queue_drops[i], clr);
        JR_CNT_SQS(OQS, TX, GREEN, port, i, &c->tx_green_class[i], clr);
        JR_CNT_SQS(OQS, TX, YELLOW, port, i, &c->tx_yellow_class[i], clr);
    }

    /* ANA_AC counter */
    {
        u32 value;
        
        JR_RDXY(ANA_AC, STAT_CNT_CFG_PORT_STAT_LSB_CNT, port, JR_CNT_ANA_AC_FILTER, &value);
        vtss_cmn_counter_32_update(value, &c->rx_local_drops, clr);
    }
    
    if (counters == NULL) {
        return VTSS_RC_OK;
    }

    /* Proprietary counters */
    prop = &counters->prop;
    for (i = 0; i < VTSS_PRIOS; i++) {
        prop->rx_prio[i] = c->rx_red_class[i].value + c->rx_yellow_class[i].value + c->rx_green_class[i].value;
        prop->tx_prio[i] = c->tx_yellow_class[i].value + c->tx_green_class[i].value;
    }

    /* RMON Rx counters */
    rmon = &counters->rmon;
    rmon->rx_etherStatsDropEvents = 0;
    for (i = 0; i < VTSS_PRIOS; i++) {
        rmon->rx_etherStatsDropEvents += (c->rx_queue_drops[i].value + c->rx_txqueue_drops[i].value + c->rx_policer_drops[i].value);
    }
    rmon->rx_etherStatsOctets = (c->rx_ok_bytes.value + c->rx_bad_bytes.value);
    rx_errors = (c->rx_crc_err.value +  c->rx_undersize.value + c->rx_oversize.value +
                 c->rx_out_of_range_len_err.value + c->rx_symbol_err.value +
                 c->rx_jabbers.value + c->rx_fragments.value);
    rmon->rx_etherStatsPkts = (c->rx_unicast.value + c->rx_multicast.value +
                               c->rx_broadcast.value + rx_errors);
    rmon->rx_etherStatsBroadcastPkts = c->rx_broadcast.value;
    rmon->rx_etherStatsMulticastPkts = c->rx_multicast.value;
    rmon->rx_etherStatsCRCAlignErrors = c->rx_crc_err.value;
    rmon->rx_etherStatsUndersizePkts = c->rx_undersize.value;
    rmon->rx_etherStatsOversizePkts = c->rx_oversize.value;
    rmon->rx_etherStatsFragments = c->rx_fragments.value;
    rmon->rx_etherStatsJabbers = c->rx_jabbers.value;
    rmon->rx_etherStatsPkts64Octets = c->rx_size64.value;
    rmon->rx_etherStatsPkts65to127Octets = c->rx_size65_127.value;
    rmon->rx_etherStatsPkts128to255Octets = c->rx_size128_255.value;
    rmon->rx_etherStatsPkts256to511Octets = c->rx_size256_511.value;
    rmon->rx_etherStatsPkts512to1023Octets = c->rx_size512_1023.value;
    rmon->rx_etherStatsPkts1024to1518Octets = c->rx_size1024_1518.value;
    rmon->rx_etherStatsPkts1519toMaxOctets = c->rx_size1519_max.value;

    /* RMON Tx counters */
    rmon->tx_etherStatsDropEvents = 0;
    for (i = 0; i < VTSS_PRIOS; i++) {
        rmon->tx_etherStatsDropEvents += c->tx_queue_drops[i].value;
    }
    rmon->tx_etherStatsPkts = (c->tx_unicast.value + c->tx_multicast.value +
                               c->tx_broadcast.value + c->tx_late_coll.value);
    rmon->tx_etherStatsOctets = c->tx_ok_bytes.value;
    rmon->tx_etherStatsBroadcastPkts = c->tx_broadcast.value;
    rmon->tx_etherStatsMulticastPkts = c->tx_multicast.value;
    rmon->tx_etherStatsCollisions = (c->tx_multi_coll.value + c->tx_backoff1.value +
                                     c->tx_late_coll.value + c->tx_xcoll.value);
    rmon->tx_etherStatsPkts64Octets = c->tx_size64.value;
    rmon->tx_etherStatsPkts65to127Octets = c->tx_size65_127.value;
    rmon->tx_etherStatsPkts128to255Octets = c->tx_size128_255.value;
    rmon->tx_etherStatsPkts256to511Octets = c->tx_size256_511.value;
    rmon->tx_etherStatsPkts512to1023Octets = c->tx_size512_1023.value;
    rmon->tx_etherStatsPkts1024to1518Octets = c->tx_size1024_1518.value;
    rmon->tx_etherStatsPkts1519toMaxOctets = c->tx_size1519_max.value;

    /* Interfaces Group Rx counters */
    if_group = &counters->if_group;
    if_group->ifInOctets = rmon->rx_etherStatsOctets;
    if_group->ifInUcastPkts = c->rx_unicast.value;
    if_group->ifInMulticastPkts = c->rx_multicast.value;
    if_group->ifInBroadcastPkts = c->rx_broadcast.value;
    if_group->ifInNUcastPkts = (c->rx_multicast.value + c->rx_broadcast.value);
    if_group->ifInDiscards = rmon->rx_etherStatsDropEvents;
    if_group->ifInErrors = (rx_errors + c->rx_in_range_len_err.value);

    /* Interfaces Group Tx counters */
    if_group->ifOutOctets = rmon->tx_etherStatsOctets;
    if_group->ifOutUcastPkts = c->tx_unicast.value;
    if_group->ifOutMulticastPkts = c->tx_multicast.value;
    if_group->ifOutBroadcastPkts = c->tx_broadcast.value;
    if_group->ifOutNUcastPkts = (c->tx_multicast.value + c->tx_broadcast.value);
    if_group->ifOutDiscards = rmon->tx_etherStatsDropEvents;
    if_group->ifOutErrors = (c->tx_late_coll.value + c->tx_csense.value + c->tx_xcoll.value);

    /* Ethernet-like Rx counters */
    elike = &counters->ethernet_like;
    elike->dot3StatsAlignmentErrors = 0; /* Not supported */
    elike->dot3StatsFCSErrors = c->rx_crc_err.value;
    elike->dot3StatsFrameTooLongs = c->rx_oversize.value;
    elike->dot3StatsSymbolErrors = c->rx_symbol_err.value;
    elike->dot3ControlInUnknownOpcodes = c->rx_unsup_opcode.value;
    elike->dot3InPauseFrames = c->rx_pause.value;

    /* Ethernet-like Tx counters */
    elike->dot3StatsSingleCollisionFrames = c->tx_backoff1.value;
    elike->dot3StatsMultipleCollisionFrames = c->tx_multi_coll.value;
    elike->dot3StatsDeferredTransmissions = c->tx_defer.value;
    elike->dot3StatsLateCollisions = c->tx_late_coll.value;
    elike->dot3StatsExcessiveCollisions = c->tx_xcoll.value;
    elike->dot3StatsCarrierSenseErrors = c->tx_csense.value;
    elike->dot3OutPauseFrames = c->tx_pause.value;

    /* Bridge counters */
    /* This will include filtered frames with and without CRC error */
    counters->bridge.dot1dTpPortInDiscards = c->rx_local_drops.value;

    return VTSS_RC_OK;
}

/* Update/clear/get port counters */
static vtss_rc jr_port_counters(const vtss_port_no_t port_no,
                                vtss_port_counters_t *const counters,
                                BOOL                 clr)
{
    return jr_port_counters_chip(VTSS_CHIP_PORT(port_no),
                                 &vtss_state->port_counters[port_no].counter.jr1,
                                 counters,
                                 clr);
}


static vtss_rc jr_port_counters_update(const vtss_port_no_t port_no)
{
    return jr_port_counters(port_no, NULL, 0);
}

static vtss_rc jr_port_counters_clear(const vtss_port_no_t port_no)
{
    return jr_port_counters(port_no, NULL, 1);
}

static vtss_rc jr_port_counters_get(const vtss_port_no_t port_no,
                                    vtss_port_counters_t *const counters)
{
    return jr_port_counters(port_no, counters, 0);
}

static vtss_rc jr_port_basic_counters_get(const vtss_port_no_t port_no,
                                          vtss_basic_counters_t *const counters)
{
    u32                      i, port = VTSS_CHIP_PORT(port_no);
    vtss_port_jr1_counters_t *c = &vtss_state->port_counters[port_no].counter.jr1;

    if (VTSS_PORT_IS_1G(port)) {
        i = (port == VTSS_PORT_NPI ? 27 : port);
        JR_CNT_1G(RX_UC, i, &c->rx_unicast, 0);
        JR_CNT_1G(RX_MC, i, &c->rx_multicast, 0);
        JR_CNT_1G(RX_BC, i, &c->rx_broadcast, 0);
        JR_CNT_1G(TX_UC, i, &c->tx_unicast, 0);
        JR_CNT_1G(TX_MC, i, &c->tx_multicast, 0);
        JR_CNT_1G(TX_BC, i, &c->tx_broadcast, 0);
    } else {
        i = VTSS_TO_DEV(port);
        JR_CNT_10G(32BIT_RX_UC, i, &c->rx_unicast, 0);
        JR_CNT_10G(32BIT_RX_MC, i, &c->rx_multicast, 0);
        JR_CNT_10G(32BIT_RX_BC, i, &c->rx_broadcast, 0);
        JR_CNT_10G(32BIT_TX_UC, i, &c->tx_unicast, 0);
        JR_CNT_10G(32BIT_TX_MC, i, &c->tx_multicast, 0);
        JR_CNT_10G(32BIT_TX_BC, i, &c->tx_broadcast, 0);
    }

    counters->rx_frames = (c->rx_unicast.value + c->rx_multicast.value + c->rx_broadcast.value);
    counters->tx_frames = (c->tx_unicast.value + c->tx_multicast.value + c->tx_broadcast.value);

    return VTSS_RC_OK;
}

static vtss_rc jr_port_forward_set(const vtss_port_no_t port_no)
{
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  QoS
 * ================================================================= */
static u32 jr_chip_prio(const vtss_prio_t prio)
{
    if (prio >= JR_PRIOS) {
        VTSS_E("illegal prio: %u", prio);
    }

    switch (vtss_state->qos_conf.prios) {
    case 1:
        return 0;
    case 2:
        return (prio < 4 ? 0 : 1);
    case 4:
        return (prio < 2 ? 0 : prio < 4 ? 1 : prio < 6 ? 2 : 3);
    case 8:
        return prio;
    default:
        break;
    }
    VTSS_E("illegal prios: %u", vtss_state->qos_conf.prios);
    return 0; 
}

static vtss_rc jr_qos_wred_conf_set(const vtss_port_no_t port_no)
{
    vtss_qos_port_conf_t *conf = &vtss_state->qos_port_conf[port_no];
    u32                  port = VTSS_CHIP_PORT(port_no);
    int                  m, queue;
    u32                  iqs_mtu[4], oqs_mtu[4];

    for (m = 0; m < 4; m++) {
        JR_RDX(IQS, MTU_MTU_FRM_SIZE_CFG, m, &iqs_mtu[m]);                
        JR_RDX(OQS, MTU_MTU_FRM_SIZE_CFG, m, &oqs_mtu[m]);                
    }

    for (queue = 0; queue < 8; queue++) {
        u32  instance = (port * 8) + queue;  
        u32  rc_cfg, hlwm, atop_swm, map_cfg, hwm, atop, mtu, l_th, h_th, min_th, max_th, minmax_th, gain;
        vtss_red_t *red = &conf->red[queue];

        /* Sanity check */
        if (red->max_th > 100) {
            VTSS_E("illegal max_th (%u) on port %u, queue %d", red->max_th, port_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->min_th > 100) {
            VTSS_E("illegal min_th (%u) on port %u, queue %d", red->min_th, port_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->max_prob_1 > 100) {
            VTSS_E("illegal max_prob_1 (%u) on port %u, queue %d", red->max_prob_1, port_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->max_prob_2 > 100) {
            VTSS_E("illegal max_prob_2 (%u) on port %u, queue %d", red->max_prob_2, port_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->max_prob_3 > 100) {
            VTSS_E("illegal max_prob_3 (%u) on port %u, queue %d", red->max_prob_3, port_no, queue);
            return VTSS_RC_ERROR;
        }
        if (red->min_th > red->max_th) {
            VTSS_E("min_th (%u) > max_th (%u) on port %u, queue %d", red->min_th, red->max_th, port_no, queue);
            return VTSS_RC_ERROR;
        }

        /* IQS */
        JR_RDX(IQS, QU_RAM_CFG_QU_RC_HLWM_CFG, instance, &hlwm);
        JR_RDX(IQS, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG, instance, &atop_swm);
        JR_RDX(IQS, QU_RAM_CFG_MTU_QU_MAP_CFG, instance, &map_cfg);
        JR_RDX(IQS, QU_RAM_CFG_QU_RC_CFG, instance, &rc_cfg);

        hwm  = VTSS_X_IQS_QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_HWM(hlwm);
        atop = VTSS_X_IQS_QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_ATOP(atop_swm);
        mtu  = iqs_mtu[VTSS_X_IQS_QU_RAM_CFG_MTU_QU_MAP_CFG_MTU_PRE_ALLOC(map_cfg)];
        l_th = VTSS_X_IQS_QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_SWM(atop_swm);                    /* l_th always == swn on IQS */
        h_th = (VTSS_F_IQS_QU_RAM_CFG_QU_RC_CFG_QU_FC_DROP_MODE & rc_cfg) ? (hwm - mtu) : (atop - mtu); /* h_th depends on drop_mode on IQS */
        if (l_th >= h_th) {
            l_th = h_th / 4; /* l_th must be less than h_th */
        } 
        min_th = (((h_th - l_th) * red->min_th) / 100) + l_th; /* convert min_th from pct to absolute value where 0% = l_th and 100% = h_th */
        max_th = (((h_th - l_th) * red->max_th) / 100) + l_th; /* convert max_th from pct to absolute value where 0% = l_th and 100% = h_th */
        minmax_th = max_th - min_th;
        gain = 0;
        while ((min_th > 255) || (minmax_th > 255)) {
            /* scale min_th and minmax_th down to be within 0..255 and adjust gain accordingly */ 
            min_th /= 2;
            minmax_th /= 2;
            gain++;
        }

        JR_WRXF(IQS, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG, instance,  RED_ECN_PROFILE_MINTH, min_th);
        JR_WRXF(IQS, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG, instance,  RED_ECN_PROFILE_MINMAXTH, minmax_th);

        JR_WRXF(IQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_ECN_PROF_MAXP,
                VTSS_ENCODE_BITFIELD((red->max_prob_1 * 255) / 100,  0, 8) |
                VTSS_ENCODE_BITFIELD((red->max_prob_2 * 255) / 100,  8, 8) |
                VTSS_ENCODE_BITFIELD((red->max_prob_3 * 255) / 100, 16, 8));
        JR_WRXF(IQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_THRES_GAIN, gain);
        JR_WRXB(IQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_ENA, 0); /* Never enable WRED on IQS (Bugzilla#9523) */

        JR_WRX(IQS, RED_RAM_RED_ECN_WQ_CFG, instance, VTSS_PORT_IS_10G(port) ? 4 : 6); /* Recommended values for IQS from VSC7460 Datasheet */

        /* OQS */
        JR_RDX(OQS, QU_RAM_CFG_QU_RC_HLWM_CFG, instance, &hlwm);
        JR_RDX(OQS, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG, instance, &atop_swm);
        JR_RDX(OQS, QU_RAM_CFG_MTU_QU_MAP_CFG, instance, &map_cfg);

        hwm  = VTSS_X_OQS_QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_HWM(hlwm);
        mtu  = iqs_mtu[VTSS_X_OQS_QU_RAM_CFG_MTU_QU_MAP_CFG_MTU_PRE_ALLOC(map_cfg)];
        l_th = VTSS_X_OQS_QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_SWM(atop_swm); /* l_th always == swn on OQS */
        h_th = hwm - mtu;                                                            /* h_th always == hwm - mtu on OQS */
        if (l_th >= h_th) {
            l_th = h_th / 4; /* l_th must be less than h_th */
        } 
        min_th = (((h_th - l_th) * red->min_th) / 100) + l_th; /* convert min_th from pct to absolute value where 0% = l_th and 100% = h_th */
        max_th = (((h_th - l_th) * red->max_th) / 100) + l_th; /* convert max_th from pct to absolute value where 0% = l_th and 100% = h_th */
        minmax_th = max_th - min_th;
        gain = 0;
        while ((min_th > 255) || (minmax_th > 255)) {
            /* scale min_th and minmax_th down to be within 0..255 and adjust gain accordingly */ 
            min_th /= 2;
            minmax_th /= 2;
            gain++;
        }

        JR_WRXF(OQS, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG, instance,  RED_ECN_PROFILE_MINTH, min_th);
        JR_WRXF(OQS, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG, instance,  RED_ECN_PROFILE_MINMAXTH, minmax_th);

        JR_WRXF(OQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_ECN_PROF_MAXP,
                VTSS_ENCODE_BITFIELD((red->max_prob_1 * 255) / 100,  0, 8) |
                VTSS_ENCODE_BITFIELD((red->max_prob_2 * 255) / 100,  8, 8) |
                VTSS_ENCODE_BITFIELD((red->max_prob_3 * 255) / 100, 16, 8));
        JR_WRXF(OQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_THRES_GAIN, gain);
        JR_WRXB(OQS, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, RED_ENA, red->enable);

        JR_WRX(OQS, RED_RAM_RED_ECN_WQ_CFG, instance, 0); /* Recommended values for OQS from Bugzilla#9523 */
    }

    return VTSS_RC_OK;
}
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
static vtss_rc jr_qos_port_dot3ar_rate_limiter_conf_set(const vtss_port_no_t port_no)
{
    vtss_qos_port_conf_t                *conf = &vtss_state->qos_port_conf[port_no];
    vtss_qos_port_dot3ar_rate_limiter_t *tx_rate_limiter = &conf->tx_rate_limiter;
    u32                                 rate_limit_val;
    u32                                 rate_ctrl_val;
    u32                                 tmp_gap_val;
    u32                                 tmp_ipg_rate = 0, tmp_scale = 0;
    u32                                 frame_overhead_scale = 0;
    u8                                  mask = 0;
    u32                                 port = VTSS_CHIP_PORT(port_no);


    if(cemax_port_is_host(port_no) == TRUE) {
        /* 802.3ar is not supported on host interfaces according to the Bug# 4108 */
        if (tx_rate_limiter->frame_rate_enable == TRUE ||
            tx_rate_limiter->payload_rate_enable == TRUE ||
            tx_rate_limiter->frame_overhead_enable == TRUE) {
            return VTSS_RC_ERROR;
        } else {
            return VTSS_RC_OK; /* No need to go further */
        }
    }

    if (tx_rate_limiter->accumulate_mode_enable == TRUE && tx_rate_limiter->frame_rate_enable == TRUE) {
        /* if accumulate mode is enabled, frame rate limiting should be disabled */
        return VTSS_RC_ERROR;
    }

    tmp_gap_val = 0;
    JR_RDX(DSM, RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE, port_no, &rate_limit_val);

    /* == Calculate the Frame overhead configuration == */
    if (VTSS_PORT_IS_10G(port_no)) {
        tmp_gap_val = 12 + 8; /* Include preamble also as said in data sheet for other ports than Dev1G and Dev2.5G*/
    } else {
        tmp_gap_val = 12;
    }
    if (tx_rate_limiter->frame_overhead_enable) { /* Frame overhead is enabled */
        rate_limit_val |= VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_FRAME_OVERHEAD_ENA;
        if (tx_rate_limiter->frame_overhead > 12) {
            frame_overhead_scale = tx_rate_limiter->frame_overhead / 255;
            if (frame_overhead_scale > 1024) {
                return VTSS_RC_ERROR;
            } else if (frame_overhead_scale){
                if (frame_overhead_scale) {
                    while (frame_overhead_scale >= (1LU << mask))  {
                        mask++;
                    }
                    frame_overhead_scale = mask;
                }
                /* CEIL the gap value */
                tmp_gap_val = (tx_rate_limiter->frame_overhead / (1LU << frame_overhead_scale)) +
                    ((tx_rate_limiter->frame_overhead % (1LU << frame_overhead_scale)) ? 1 : 0);
            } else {
                tmp_gap_val = tx_rate_limiter->frame_overhead;
            }
        }
    } else {
        rate_limit_val &= (~VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_FRAME_OVERHEAD_ENA);
    }

    /* == Calculate Payload rate limiter configuration == */
    if (tx_rate_limiter->payload_rate_enable == TRUE) {
        rate_limit_val |= VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_PAYLOAD_RATE_ENA;
        /* Calculate the IPG_STRETCH_RATION based on bandwidth utilization */
        do {
            if (tx_rate_limiter->payload_rate == 100) {
                rate_limit_val &= (~VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_PAYLOAD_RATE_ENA);
                break;
            } else {
                tmp_ipg_rate = ( (1<<tmp_scale) * 256 * (tx_rate_limiter->payload_rate) ) /
                    (100-tx_rate_limiter->payload_rate);
                tmp_ipg_rate += ( (1<<tmp_scale) * 256 * (tx_rate_limiter->payload_rate) ) %
                    (100-tx_rate_limiter->payload_rate);

            }
            if (tmp_ipg_rate > 1152 && tmp_ipg_rate < 518143) { /* Range as specified in DS */
                break;
            }
            tmp_scale++;
        } while (tmp_scale <= 10);
        if (tmp_scale > 10) {
            return VTSS_RC_ERROR;
        }

    } else {
        rate_limit_val &= (~VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_PAYLOAD_RATE_ENA);
    }

    tmp_scale = (tmp_scale > frame_overhead_scale) ? tmp_scale : frame_overhead_scale;
    rate_limit_val &= (~VTSS_M_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_IPG_SCALE_VAL);
    rate_limit_val |= (VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_IPG_SCALE_VAL(tmp_scale));

    /* == Calculate Frame rate limiter configuration == */
    if (tx_rate_limiter->frame_rate_enable == TRUE) {
        rate_limit_val |= (VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_FRAME_RATE_ENA);
    } else {
        rate_limit_val &= (~VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_FRAME_RATE_ENA);
    }

    /* == Include/Exclude preamble from payload calculation == */
    if (tx_rate_limiter->preamble_in_payload == TRUE) {
        rate_limit_val |= VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_PAYLOAD_PREAM_CFG;
    } else {
        rate_limit_val &= (~VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_PAYLOAD_PREAM_CFG);
    }

    /* == Include/Exclude header size from payload calculation == */
    if (tx_rate_limiter->header_in_payload == TRUE) {
        rate_limit_val |= VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_PAYLOAD_CFG;
    } else {
        rate_limit_val &= (~VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_PAYLOAD_CFG);
    }

    /* == Enable/Disable accumulate mode == */
    if (tx_rate_limiter->accumulate_mode_enable == TRUE) {
        rate_limit_val |= VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_ACCUM_MODE_ENA;
    } else {
        rate_limit_val &= (~VTSS_F_DSM_RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE_TX_RATE_LIMIT_ACCUM_MODE_ENA);
    }

    /* == Configure Rate CTRL value  == */
    JR_RDX(DSM, CFG_RATE_CTRL, port, &rate_ctrl_val);
    rate_ctrl_val &= (~VTSS_M_DSM_CFG_RATE_CTRL_FRM_GAP_COMP);
    rate_ctrl_val |= (VTSS_F_DSM_CFG_RATE_CTRL_FRM_GAP_COMP(tmp_gap_val));
    JR_WRX(DSM, CFG_RATE_CTRL, port, rate_ctrl_val);

    /* == Configure the IPG stretch ratio == */
    JR_WRX(DSM, RATE_LIMIT_CFG_TX_IPG_STRETCH_RATIO_CFG, port, tmp_ipg_rate);

    /* == Configure the Frame rate == */
    if (tx_rate_limiter->frame_rate < 64) {
        JR_WRX(DSM, RATE_LIMIT_CFG_TX_FRAME_RATE_START_CFG, port, 64);
    } else {
        JR_WRX(DSM, RATE_LIMIT_CFG_TX_FRAME_RATE_START_CFG, port, tx_rate_limiter->frame_rate);
    }

    /* == Configure the Rate limiters == */
    JR_WRX(DSM, RATE_LIMIT_CFG_TX_RATE_LIMIT_MODE, port, rate_limit_val);

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_DOT3AR_RATE_LIMITER */

static vtss_rc jr_qos_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_qos_port_conf_t *conf = &vtss_state->qos_port_conf[port_no];
    u32 port = VTSS_CHIP_PORT(port_no);
    int pcp, dei, queue, class, policer;
    u8 cost[6];
    u32  tag_remark_mode;
    BOOL tag_default_dei;

    /* Select device based on port map */
    VTSS_SELECT_CHIP_PORT_NO(port_no);

    /* Port default PCP and DEI configuration */
    JR_WRXF(ANA_CL_2, PORT_VLAN_CTRL, port, PORT_UPRIO, conf->usr_prio);
    JR_WRXB(ANA_CL_2, PORT_VLAN_CTRL, port, PORT_CFI,   conf->default_dei);
    
    /* Port default QoS class, DP level, tagged frames mode and DSCP mode configuration */
    JR_WRXF(ANA_CL_2, PORT_QOS_CFG, port, DEFAULT_QOS_VAL,    jr_chip_prio(conf->default_prio));
    JR_WRXF(ANA_CL_2, PORT_QOS_CFG, port, DEFAULT_DP_VAL,     conf->default_dpl);
    /* If translation is enabled, we must enable QoS classification based on the DSCP value also. */
    /* All trusted DSCP values will now be used for QoS classification as a side effect of this. */
    JR_WRXB(ANA_CL_2, PORT_QOS_CFG, port, DSCP_QOS_ENA,       conf->dscp_class_enable || conf->dscp_translate);
    JR_WRXB(ANA_CL_2, PORT_QOS_CFG, port, DSCP_DP_ENA,        conf->dscp_class_enable);


    /* Map for (PCP and DEI) to (QoS class and DP level) */
    for (pcp = VTSS_PCP_START; pcp < VTSS_PCP_END; pcp++) {
        for (dei = VTSS_DEI_START; dei < VTSS_DEI_END; dei++) {
            JR_WRXYF(ANA_CL_2, PORT_UPRIO_MAP_CFG, port, (8*dei + pcp), UPRIO_CFI_TRANSLATE_QOS_VAL, jr_chip_prio(conf->qos_class_map[pcp][dei]));
            JR_WRXYF(ANA_CL_2, PORT_DP_CONFIG,     port, (8*dei + pcp), UPRIO_CFI_DP_VAL,            conf->dp_level_map[pcp][dei]);
        }
    }

    /* Update vlan port configuration in order to enable/disable tag classification */
    VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));

    /* Enable gap value adjustment */
    JR_WRXB(SCH, ETH_ETH_LB_DWRR_CFG, port, FRM_ADJ_ENA, 1);

    /* DWRR configuration */
    VTSS_RC(vtss_cmn_qos_weight2cost(conf->queue_pct, cost, 6, 5));
    JR_WRXB(SCH, ETH_ETH_DWRR_CFG, port, DWRR_MODE, conf->dwrr_enable);
    JR_WRXF(SCH, ETH_ETH_DWRR_CFG, port, COST_CFG,
            VTSS_ENCODE_BITFIELD(cost[0],  0, 5) |
            VTSS_ENCODE_BITFIELD(cost[1],  5, 5) |
            VTSS_ENCODE_BITFIELD(cost[2], 10, 5) |
            VTSS_ENCODE_BITFIELD(cost[3], 15, 5) |
            VTSS_ENCODE_BITFIELD(cost[4], 20, 5) |
            VTSS_ENCODE_BITFIELD(cost[5], 25, 5));

    /* Egress port shaper burst level configuration
     * The value is rounded up to the next possible value:
     *           0 -> 0 (Always closed)
     *    1.. 4096 -> 1 ( 4 KB)
     * 4097.. 8192 -> 2 ( 8 KB)
     * 8193..12288 -> 3 (12 KB)
     */
    if (conf->shaper_port.rate == VTSS_BITRATE_DISABLED) {
        JR_WRXF(SCH, ETH_LB_ETH_LB_THRES, ((9 * port) + 8), LB_THRES,  0x3f); // Set burst level to maximum if shaper is disabled
    }
    else {
        JR_WRXF(SCH, ETH_LB_ETH_LB_THRES, ((9 * port) + 8), LB_THRES,  MIN(0x3f, ((conf->shaper_port.level + 4095) / 4096)));
    }

    /* Egress port shaper rate configuration
     * The value (in kbps) is rounded up to the next possible value:
     *        0 -> 0 (Open until burst capacity is used, then closed)
     *   1..100 -> 1 (100160 bps)
     * 101..200 -> 2 (200320 bps)
     * 201..300 -> 3 (300480 bps)
     */
    JR_WRXF(SCH, ETH_LB_ETH_LB_RATE, ((9 * port) + 8), LB_RATE, MIN(0x1ffff, VTSS_DIV64(((u64)conf->shaper_port.rate * 1000) + 100159, 100160)));

    /* Egress queue shaper rate and burst level configuration. See documentation above */
    for (queue = 0; queue < 8; queue++) {
        if (conf->shaper_queue[queue].rate == VTSS_BITRATE_DISABLED) {
            JR_WRXF(SCH, ETH_LB_ETH_LB_THRES, ((9 * port) + queue), LB_THRES,  0x3f); // Set burst level to maximum if shaper is disabled
        }
        else {
            JR_WRXF(SCH, ETH_LB_ETH_LB_THRES, ((9 * port) + queue), LB_THRES,  MIN(0x3f, ((conf->shaper_queue[queue].level + 4095) / 4096)));
        }
        JR_WRXF(SCH, ETH_LB_ETH_LB_RATE, ((9 * port) + queue), LB_RATE, MIN(0x1ffff, VTSS_DIV64(((u64)conf->shaper_queue[queue].rate * 1000) + 100159, 100160)));
    }

    /* Egress port and queue shaper enable/disable configuration
     * In Jaguar we cannot enable/disable shapers individually
     * so we always enable shaping.
     * If an individual shaper (port or queue) is disabled, we
     * must set rate and burst level to maximum.
     * The rate is automatically configured to maximum if the
     * value for disabled is applied (0xffffffff).
     * The burst level is explicit set to maximum is the shaper
     * is disabled
     */
    JR_WRXB(SCH, ETH_ETH_SHAPING_CTRL, port, SHAPING_ENA, 1);
    JR_WRXF(SCH, ETH_ETH_SHAPING_CTRL, port, PRIO_LB_EXS_ENA,
            (conf->excess_enable[0] ? VTSS_BIT(0) : 0) |
            (conf->excess_enable[1] ? VTSS_BIT(1) : 0) |
            (conf->excess_enable[2] ? VTSS_BIT(2) : 0) |
            (conf->excess_enable[3] ? VTSS_BIT(3) : 0) |
            (conf->excess_enable[4] ? VTSS_BIT(4) : 0) |
            (conf->excess_enable[5] ? VTSS_BIT(5) : 0) |
            (conf->excess_enable[6] ? VTSS_BIT(6) : 0) |
            (conf->excess_enable[7] ? VTSS_BIT(7) : 0));

    /* Tag remarking configuration
     */
    tag_remark_mode = conf->tag_remark_mode;
    tag_default_dei = (tag_remark_mode == VTSS_TAG_REMARK_MODE_DEFAULT ? 
                       conf->tag_default_dei : 0);
#if defined(VTSS_FEATURE_EVC)
    /* Enable DEI colouring for NNI ports */
    if (vtss_state->evc_port_info[port_no].nni_count) {
        /* Allow only CLASSIFIED and MAPPED modes */
        if (tag_remark_mode == VTSS_TAG_REMARK_MODE_DEFAULT)
            tag_remark_mode = VTSS_TAG_REMARK_MODE_CLASSIFIED;
        
        /* DEI colouring is enabled using default DEI field (requires revision B) */
        tag_default_dei = vtss_state->evc_port_conf[port_no].dei_colouring;
    }
#endif /* VTSS_FEATURE_EVC */    

    JR_WRXF(REW, PORT_TAG_CTRL,         port, QOS_SEL,       tag_remark_mode);
    JR_WRXF(REW, PORT_PORT_TAG_DEFAULT, port, PORT_TCI_PRIO, conf->tag_default_pcp);
    JR_WRXB(REW, PORT_PORT_TAG_DEFAULT, port, PORT_TCI_CFI,  tag_default_dei);
    JR_WRXF(REW, PORT_PORT_DP_MAP,      port, DP,
            (conf->tag_dp_map[0] ? VTSS_BIT(0) : 0) |
            (conf->tag_dp_map[1] ? VTSS_BIT(1) : 0) |
            (conf->tag_dp_map[2] ? VTSS_BIT(2) : 0) |
            (conf->tag_dp_map[3] ? VTSS_BIT(3) : 0));

    /* Map for (QoS class and DP level) to (PCP and DEI) */
    for (class = VTSS_QUEUE_START; class < VTSS_QUEUE_END; class++) {
        JR_WRXYF(REW, PORT_PCP_MAP_DE0, port, class, PCP_DE0, conf->tag_pcp_map[class][0]);
        JR_WRXYF(REW, PORT_PCP_MAP_DE1, port, class, PCP_DE1, conf->tag_pcp_map[class][1]);
        JR_WRXYB(REW, PORT_DEI_MAP_DE0, port, class, DEI_DE0, conf->tag_dei_map[class][0]);
        JR_WRXYB(REW, PORT_DEI_MAP_DE1, port, class, DEI_DE1, conf->tag_dei_map[class][1]);
    }

    /* DSCP remarking configuration
     */
    JR_WRXF(ANA_CL_2, PORT_QOS_CFG,  port, DSCP_REWR_MODE_SEL, conf->dscp_mode);
    JR_WRXB(ANA_CL_2, PORT_QOS_CFG,  port, DSCP_TRANSLATE_ENA, conf->dscp_translate);
    JR_WRXB(REW,      PORT_DSCP_MAP, port, DSCP_UPDATE_ENA,   (conf->dscp_emode > VTSS_DSCP_EMODE_DISABLE));
    JR_WRXB(REW,      PORT_DSCP_MAP, port, DSCP_REMAP_ENA,    (conf->dscp_emode > VTSS_DSCP_EMODE_REMARK));

    /* WRED configuration
     */
    VTSS_RC(jr_qos_wred_conf_set(port_no));

    /* Port policer configuration
     */
    for (policer = 0; policer < 4; policer++) {
        VTSS_RC(jr_port_policer_set(port, policer, &conf->policer_port[policer], &conf->policer_ext_port[policer]));
    }

    /* Queue policer configuration */
    for (queue = 0; queue < 8; queue++) {
        VTSS_RC(jr_queue_policer_set(port, queue, &conf->policer_queue[queue]));
    }

    /* Update policer flow control configuration */
    VTSS_RC(jr_port_policer_fc_set(port_no, port));

#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
    VTSS_RC(jr_qos_port_dot3ar_rate_limiter_conf_set(port_no));
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */

    return VTSS_RC_OK;
}

static vtss_rc jr_qos_conf_set_chip(void) 
{
    vtss_qos_conf_t *conf = &vtss_state->qos_conf;
    int             i;

    /* Frame adjustment (gap value - number of bytes added in leaky buckets and DWRR calculations) */
    JR_WRF(SCH, ETH_ETH_LB_DWRR_FRM_ADJ, FRM_ADJ, 20); /* 20 bytes added */

    /* DSCP classification and remarking configuration
     */
    for (i = 0; i < 64; i++) {
        JR_WRXF(ANA_CL_2, COMMON_DSCP_CFG,   i, DSCP_TRANSLATE_VAL, conf->dscp_translate_map[i]);
        JR_WRXF(ANA_CL_2, COMMON_DSCP_CFG,   i, DSCP_QOS_VAL,       jr_chip_prio(conf->dscp_qos_class_map[i]));
        JR_WRXF(ANA_CL_2, COMMON_DSCP_CFG,   i, DSCP_DP_VAL,        conf->dscp_dp_level_map[i]);
        JR_WRXB(ANA_CL_2, COMMON_DSCP_CFG,   i, DSCP_REWR_ENA,      conf->dscp_remark[i]);
        JR_WRXB(ANA_CL_2, COMMON_DSCP_CFG,   i, DSCP_TRUST_ENA,     conf->dscp_trust[i]);
        JR_WRXF(REW,      COMMON_DSCP_REMAP, i, DSCP_REMAP,         conf->dscp_remap[i]);
    }

    /* DSCP classification from QoS configuration
     */
    for (i = 0; i < 8; i++) {
        JR_WRXF(ANA_CL_2, COMMON_QOS_MAP_CFG, i, DSCP_REWR_VAL, conf->dscp_qos_map[i]);
    }

#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
    JR_WR(DSM, RATE_LIMIT_CFG_TX_RATE_LIMIT_HDR_CFG, conf->header_size);
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */

    return VTSS_RC_OK;
}

static vtss_rc jr_qos_conf_set(BOOL changed)
{
    vtss_port_no_t port_no;
        
    if (changed) {
        /* Number of priorities changed, update QoS setup for all ports */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            VTSS_RC(jr_qos_port_conf_set(port_no));   
        }
    }

    /* Setup config for all devices */
    return jr_conf_chips(jr_qos_conf_set_chip);
}

#if defined(VTSS_FEATURE_TIMESTAMP)
/* ================================================================= *
*  Time stamping
* ================================================================= */


#define HW_CLK_CNT_PR_SEC 250000000L
#define HW_NS_PR_SEC 1000000000L
#define HW_NS_PR_CLK_CNT (HW_NS_PR_SEC/HW_CLK_CNT_PR_SEC)

#define HW_CLK_50_MS (HW_CLK_CNT_PR_SEC/20)
#define HW_CLK_M950_MS (-HW_CLK_CNT_PR_SEC + HW_CLK_50_MS)
#define EXT_SYNC_INPUT_LATCH_LATENCY 3  /* subtracted from EXT_SYNC_CURRENT TIME */

#define VTSS_JR_TX_TIMESTAMPS_PR_PORT 3

static vtss_rc 
    jr_ts_timeofday_get(vtss_timestamp_t               *ts,
                         u32                     *tc)
{
    u32 value;
    u32 sticky;
    i32 x;
    ts->seconds = vtss_state->ts_conf.sec_offset; /* sec counter is maintained in SW */
    ts->sec_msb = 0; /* to be maintained in one sec interrupt */
    JR_RD(DEVCPU_GCB, PTP_STAT_PTP_CURRENT_TIME_STAT, &value);
    x = VTSS_X_DEVCPU_GCB_PTP_STAT_PTP_CURRENT_TIME_STAT_PTP_CURRENT_TIME(value);
    /* DURING THE ADJUSTMENT PHASE THE ADJUST IS DONE IN sw */
    if (vtss_state->ts_conf.awaiting_adjustment) {
        x -= vtss_state->ts_conf.outstanding_corr;
        if (x < 0) {
            ts->seconds--;
            x += HW_CLK_CNT_PR_SEC;
        } else if (x >= HW_CLK_CNT_PR_SEC) {
            ts->seconds++;
            x -= HW_CLK_CNT_PR_SEC;
        }
    }
    /* if the timer has wrapped, and the onesec update has not been done, then increment ts->seconds */
    JR_RD(DEVCPU_GCB, PTP_STAT_PTP_EVT_STAT, &sticky);
    if (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_SYNC_STAT) {
        if (value < HW_CLK_50_MS) {
            ++ts->seconds;
        } else if (value < (HW_CLK_CNT_PR_SEC- HW_CLK_50_MS)){
            /* The counter may have wrapped after the readings, therefore a value close to 1 sec is ok */
            VTSS_E("too large interrupt latency: %d (clock cycles)", value);
        }
    }
        
    ts->nanoseconds = x*HW_NS_PR_CLK_CNT;
    *tc = VTSS_X_DEVCPU_GCB_PTP_STAT_PTP_CURRENT_TIME_STAT_PTP_CURRENT_TIME(value);
    VTSS_D("ts->seconds: %u, ts->nanoseconds: %u", ts->seconds, ts->nanoseconds);
    return VTSS_RC_OK;
}

static vtss_rc 
jr_ts_timeofday_next_pps_get(vtss_timestamp_t               *ts)
{
    ts->sec_msb = 0; /* to be maintained in one sec interrupt */
    ts->seconds = vtss_state->ts_conf.sec_offset+1; /* sec counter is maintained in SW */
    ts->nanoseconds = 0;
    VTSS_D("ts->seconds: %u, ts->nanoseconds: %u", ts->seconds, ts->nanoseconds);
    return VTSS_RC_OK;
}

/*
 * Subtract the offset from the actual time. 
 * offset is in the range [-1..+1] sec.
 * alignment algorithm:
 *  offset > 50 ms => 
 *    PTP_UPPER_LIMIT_1_TIME_ADJ = TOD_NANOSECS - ts.nanoseconds
 *    (TBD if this works, because the ADJ is < actual TOD_NANOSECS ??)
 *  50 ms > offset > -950 ms => 
 *    PTP_UPPER_LIMIT_1_TIME_ADJ = TOD_NANOSECS - ts.nanoseconds + 1 sec
 *  -950 ms > offset => 
 *    PTP_UPPER_LIMIT_1_TIME_ADJ = TOD_NANOSECS - ts.nanoseconds + 2 sec
 * The second offset is adjusted accordingly.
 *
 */
static vtss_rc
jr_ts_timeofday_offset_set(i32 offset)
{
    i32 corr;
    if (vtss_state->ts_conf.awaiting_adjustment) {
        return VTSS_RC_ERROR;
     }
    vtss_state->ts_conf.awaiting_adjustment = TRUE;
    vtss_state->ts_conf.outstanding_adjustment = 0;
    
    corr = (offset/HW_NS_PR_CLK_CNT);
    
    if (corr > HW_CLK_50_MS) {
        --vtss_state->ts_conf.sec_offset;
        vtss_state->ts_conf.outstanding_adjustment = corr-1;
        vtss_state->ts_conf.outstanding_corr = corr - HW_CLK_CNT_PR_SEC;
    } else if (corr < HW_CLK_M950_MS) {
        ++vtss_state->ts_conf.sec_offset;
        vtss_state->ts_conf.outstanding_adjustment = corr + 2*HW_CLK_CNT_PR_SEC-1;
        vtss_state->ts_conf.outstanding_corr = corr + HW_CLK_CNT_PR_SEC;
    } else {
        vtss_state->ts_conf.outstanding_corr = corr;
        vtss_state->ts_conf.outstanding_adjustment = corr + HW_CLK_CNT_PR_SEC-1;
    }
    /* we wait until TOD_NANOSECS wraps */
    VTSS_D("offset = %d, adjust = %u, corr: %d", offset, vtss_state->ts_conf.outstanding_adjustment, vtss_state->ts_conf.outstanding_corr);
    return VTSS_RC_OK;
}

/*
* The actual second counter is saved in SW.
* The NS counter in hw (CURRENT_TIME) has to be aligned with the ts.nanoseconds,
* this is done by using the PTP_UPPER_LIMIT_1_TIME_ADJ:
* This register can be used to change the one sec. period once, the period can 
* be set to [0..268.435.455] periods of 4 ns = [0..1.073.741.820] ns.
* alignment algorithm:
*  (CURRENT_TIME - ts.nanoseconds) > 50 ms => 
*    PTP_UPPER_LIMIT_1_TIME_ADJ = CURRENT_TIME - ts.nanoseconds
*    (This has to be delayed until CURRENT_TIME wraps, because the ADJ is < actual CURRENT_TIME ??)
*  50 ms > (TOD_NANOSECS - ts.nanoseconds) > -950 ms => 
*    PTP_UPPER_LIMIT_1_TIME_ADJ = CURRENT_TIME - ts.nanoseconds + 1 sec
*  -950 ms > (CURRENT_TIME - ts.nanoseconds) => 
*    PTP_UPPER_LIMIT_1_TIME_ADJ = CURRENT_TIME - ts.nanoseconds + 2 sec
* The second offset is adjusted accordingly.
*
*/
static vtss_rc
    jr_ts_timeofday_set(const vtss_timestamp_t         *ts)
{
    i32 corr;
    u32 value;
    if (vtss_state->ts_conf.awaiting_adjustment) {
        return VTSS_RC_ERROR;
    }
    
    vtss_state->ts_conf.sec_offset = ts->seconds;
    /* corr = PTP_NS - ts.ns */
    JR_RD(DEVCPU_GCB, PTP_STAT_PTP_CURRENT_TIME_STAT, &value);
    VTSS_D("ts->seconds: %u, ts->nanoseconds: %u", ts->seconds, ts->nanoseconds);
    VTSS_D("PTP_CURRENT_TIME: %u", value);
    
    corr = (VTSS_X_DEVCPU_GCB_PTP_STAT_PTP_CURRENT_TIME_STAT_PTP_CURRENT_TIME(value));
    VTSS_RC(jr_ts_timeofday_offset_set(corr*HW_NS_PR_CLK_CNT - ts->nanoseconds));
    /* we wait until TOD_NANOSECS wraps */
    VTSS_D("PTP_TOD_SECS: %u, PTP_TOD_NANOSECS: %u",vtss_state->ts_conf.sec_offset , value);
    return VTSS_RC_OK;
}

/*
 * When the time is set, the SW sec  counters are written. The HW ns clock is adjusted in the next 1 sec call. 
 */
static vtss_rc jr_ts_timeofday_set_delta(const vtss_timestamp_t *ts, BOOL negative)
{
    i32 corr;
    if (vtss_state->ts_conf.awaiting_adjustment) {
        return VTSS_RC_ERROR;
    }
    if (ts->nanoseconds != 0) {
        if (negative) {
            corr = -(i32)ts->nanoseconds;
        } else {
            corr = ts->nanoseconds;
        }        
        VTSS_RC(jr_ts_timeofday_offset_set(corr));
    }
    if (negative) {
        vtss_state->ts_conf.sec_offset -= ts->seconds;
    } else {
        vtss_state->ts_conf.sec_offset += ts->seconds;
    }
    
    VTSS_D("ts->sec_msb: %u, ts->seconds: %u, ts->nanoseconds: %u", ts->sec_msb, ts->seconds, ts->nanoseconds);
    return VTSS_RC_OK;
}


/*
 * This function is called from interrupt, therefore the macros using vtss_state cannot be used.
 */
static u32 jr_ts_ns_cnt_get(vtss_inst_t inst)
{
    u32 value;
    (void)inst->init_conf.reg_read(0, VTSS_DEVCPU_GCB_PTP_STAT_PTP_CURRENT_TIME_STAT, &value);
    return VTSS_X_DEVCPU_GCB_PTP_STAT_PTP_CURRENT_TIME_STAT_PTP_CURRENT_TIME(value);
}

static vtss_rc
    jr_ts_timeofday_one_sec(void)
{
    u32 value;
    ++vtss_state->ts_conf.sec_offset; /* sec counter is maintained in SW */
    if (vtss_state->ts_conf.awaiting_adjustment) {
        /* For debug: read the internal NS counter */
        JR_RD(DEVCPU_GCB, PTP_STAT_PTP_CURRENT_TIME_STAT, &value);
        VTSS_D("TOD_NANOSECS %u", value);

        if (vtss_state->ts_conf.outstanding_adjustment != 0) {
            if (value > vtss_state->ts_conf.outstanding_adjustment) {
                VTSS_E("Too large interrupt latency to adjust %u (*4ns)", value);
            } else {
                /*PTP_UPPER_LIMIT_1_TIME_ADJ = x */
                JR_WR(DEVCPU_GCB, PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG, 
                      VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ(vtss_state->ts_conf.outstanding_adjustment) |
                          VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_SHOT);
                VTSS_D("onetime adjustment done %u", vtss_state->ts_conf.outstanding_adjustment);
                vtss_state->ts_conf.outstanding_adjustment = 0;
            }
        } else {
            vtss_state->ts_conf.awaiting_adjustment = FALSE;
            VTSS_D("awaiting_adjustment cleared");
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc
    jr_ts_adjtimer_set(void)
{
    /* adj in units of 0,1 pbb */
    i32 adj = vtss_state->ts_conf.adj;
    u32 clk_adj = (u64)HW_CLK_CNT_PR_SEC;
    i32 divisor = 0;
    /* CLK_ADJ = clock_rate/|adj|
    * CLK_ADJ_DIR = adj>0 ? Positive : Negative
    */
    if (adj != 0) {
        divisor = (i32) VTSS_DIV64(HW_NS_PR_SEC*10LL, vtss_state->ts_conf.adj);
        clk_adj = VTSS_LABS(divisor)-1;
    }
    vtss_state->ts_conf.adj_divisor = divisor;
    if (clk_adj > HW_CLK_CNT_PR_SEC) clk_adj = HW_CLK_CNT_PR_SEC;
    
    if ((adj != 0)) {
        clk_adj |= VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_ENA;
        JR_WRM_SET(DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ADJ_ENA);
    } else {
        JR_WRM_CLR(DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ADJ_ENA);
    }
    if (adj < 0) {
        clk_adj |= VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_DIR;
    } 
    JR_WR(DEVCPU_GCB, PTP_CFG_CLK_ADJ_CFG, clk_adj);
    
    VTSS_D("adj: %d, CLK_ACJ_CFG: %x, divisor %d", adj, clk_adj, divisor);
    return VTSS_RC_OK;
}

static vtss_rc
    jr_ts_freq_offset_get(i32 *const adj)
{
    i32 offset; /* frequency offset in clock counts pr sec */
    u32 cu;
    JR_RD(DEVCPU_GCB, PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT,&cu);
    cu -= EXT_SYNC_INPUT_LATCH_LATENCY;
    offset = VTSS_X_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT_EXT_SYNC_CURRENT_TIME(cu) - HW_CLK_CNT_PR_SEC;
    /* convert to units of 0,1 ppb */
    *adj = HW_NS_PR_CLK_CNT*10*offset;
    
    
    VTSS_D("cu: %u adj: %d", cu, *adj);
    return VTSS_RC_OK;
}




static vtss_rc
    jr_ts_external_clock_mode_set(void)
{
    u32 dividers;
    u32 high_div;
    u32 low_div;
    if (!vtss_state->ts_conf.ext_clock_mode.enable) {
        /* disable clock output */
        JR_WRM_CLR(DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA);
        /* disable alternate 1 for GPIO_9 (clock output). */
        (void) jr_gpio_mode(0, 9, VTSS_GPIO_IN);
    } else {
        /* enable alternate 1 for GPIO_9 (clock output). */
        (void) jr_gpio_mode(0, 9, VTSS_GPIO_ALT_0);
        
        /* set dividers; enable clock output. */
        dividers = (HW_CLK_CNT_PR_SEC/(vtss_state->ts_conf.ext_clock_mode.freq));
        high_div = (dividers/2)-1;
        low_div =  ((dividers+1)/2)-1;
        JR_WR(DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG, high_div  &
               VTSS_M_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG_GEN_EXT_CLK_HIGH_PERIOD);
        JR_WR(DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG, low_div  &
               VTSS_M_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG_GEN_EXT_CLK_LOW_PERIOD);
        JR_WRM_SET(DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA);
    }
    if ((vtss_state->ts_conf.ext_clock_mode.one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_DISABLE)) {
        /* disable alternate 1 for GPIO_8 (clock output). */
        (void) jr_gpio_mode(0, 8, VTSS_GPIO_IN);
    } else {
        /* enable alternate 1 for GPIO_8 (clock output). */
        (void) jr_gpio_mode(0, 8, VTSS_GPIO_ALT_0);
    }
    if (vtss_state->ts_conf.ext_clock_mode.one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_OUTPUT) {
        /* 1 pps output enabled */
        JR_WRM(DEVCPU_GCB, PTP_CFG_PTP_MISC_CFG, 
               VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA, 
               VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_INV |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_SEL |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_INV |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA);
        
    } else if (vtss_state->ts_conf.ext_clock_mode.one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_INPUT) {
        /* 1 pps input enabled */
        JR_WRM(DEVCPU_GCB, PTP_CFG_PTP_MISC_CFG, 
               VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA, 
               VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_INV |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_SEL |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_INV |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA);
    } else {
        /* 1 pps disabled */
        JR_WRM(DEVCPU_GCB, PTP_CFG_PTP_MISC_CFG, 
               VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA, 
               VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_INV |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_SEL |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_INV |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA |
                   VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA);
    }
    return VTSS_RC_OK;
}

static vtss_rc 
    jr_ts_convert_timestamp(vtss_port_no_t port, u32 *ts)
{
    /*lint -esym(459, slave_timer_offset) */
    static u32 slave_timer_offset[] = {
        /*  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15 */
           13, 18, 19, 24, 25, 30, 14, 17, 20, 23, 26, 29, 15, 16, 21, 22,
        /* 16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31 */
           27, 28, 12, 11, 10,  9,  8,  7,  6,  5,  4, 34, 31, 33, 32,  3
    };
    /* chip port is signed, == -1 if unused port */
   *ts = *ts/HW_NS_PR_CLK_CNT; /* slave timers are in ns, we use clock cycles */
   /* clock asjustment compensation */
   if (vtss_state->ts_conf.adj_divisor) {
       *ts = *ts + (i32)*ts/vtss_state->ts_conf.adj_divisor;
   }
   if ((u32)VTSS_CHIP_PORT(port) < sizeof(slave_timer_offset)/sizeof(u32)) {
        *ts += slave_timer_offset[VTSS_CHIP_PORT(port)];
        if (*ts > HW_CLK_CNT_PR_SEC) {
            *ts -= HW_CLK_CNT_PR_SEC;
        }
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}


static vtss_rc 
    jr_ts_timestamp_get(void)
{
    u32 value;
    u32 ts_ts;
    u32 ts_id;
    u32 chip_port;
    u32 idx;
    u32 sticky_mask;
    vtss_port_no_t tx_port;
    
    for (tx_port = 0; tx_port < VTSS_PORT_ARRAY_SIZE; tx_port++) {
        chip_port = VTSS_CHIP_PORT(tx_port);

        if (VTSS_PORT_IS_1G(chip_port)) {
            JR_RDX(DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_STICKY, VTSS_TO_DEV(chip_port), &value);
        } else {
            JR_RDX(DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_STICKY, VTSS_TO_DEV(chip_port), &value);
        }
        sticky_mask = 0x1;
        for (idx = 0; idx < VTSS_JR_TX_TIMESTAMPS_PR_PORT; idx++) {
            if (value & sticky_mask) {
                if (VTSS_PORT_IS_1G(chip_port)) {
                    JR_RDXY(DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_TSTAMP, VTSS_TO_DEV(chip_port), idx, &ts_ts);
                    JR_RDXY(DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_ID, VTSS_TO_DEV(chip_port), idx, &ts_id);
                    JR_WRX(DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_STICKY, VTSS_TO_DEV(chip_port), sticky_mask);
                } else {
                    JR_RDXY(DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_TSTAMP, VTSS_TO_DEV(chip_port), idx, &ts_ts);
                    JR_RDXY(DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_ID, VTSS_TO_DEV(chip_port), idx, &ts_id);
                    JR_WRX(DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_STICKY, VTSS_TO_DEV(chip_port), sticky_mask);
                }
                (void)jr_ts_convert_timestamp(tx_port, &ts_ts);
                vtss_state->ts_status[idx].tx_tc[tx_port] = ts_ts;
                vtss_state->ts_status[idx].tx_id[tx_port] = ts_id;
                vtss_state->ts_status[idx].valid_mask |= 1LL<<tx_port;
                VTSS_D("Txstamp[%d], ts = %u, id = %u, port = %u", idx, ts_ts, ts_id, tx_port);
            }
            sticky_mask = sticky_mask<<1;
        }
    }
    return VTSS_RC_OK;
}



static vtss_rc 
    jr_ts_timestamp_id_release(u32 ts_id)
{
    return VTSS_RC_OK;
}

#endif /* VTSS_FEATURE_TIMESTAMP */

#if defined(VTSS_FEATURE_VSTAX) || defined(VTSS_FEATURE_NPI)
static vtss_rc jr_cpu_queue_redirect_chip(void)
{
    /*
     * This function performs the following CPU queue redirection actions if
     * l3_XXX_queue != VTSS_PACKET_RX_QUEUE_NONE and vstax_info.master_upsid != VTSS_VSTAX_UPSID_UNDEF:
     *   JR-24 standalone       : No redirection.
     *   JR-24 stackable, master: No redirection.
     *   JR-24 stackable, slave : l3_XXX_queue redirected to master.
     *   JR-48 standalone       : Primary unit: No redirection.                    Secondary unit: l3_XXX_queue redirected to primary unit.
     *   JR-48 stackable, master: Primary unit: No redirection.                    Secondary unit: l3_XXX_queue redirected to primary unit (master).
     *   JR-48 stackable, slave : Primary unit: l3_XXX_queue redirected to master. Secondary unit: l3_XXX_queue redirected to master.
     *
     * If user-controlled redirection of any CPU queue to an NPI-port is required, this can
     * only occur in configurations that says "no redirection" in the table above or if
     * l3_XXX_queue is set to VTSS_PACKET_RX_QUEUE_NONE.
     *
     * l3_XXX_queue redirection is required to overcome two bugs (Bugzilla#3792 and
     * Bugzilla#9547) in Jaguar.
     */
    vtss_packet_rx_conf_t *rx_conf = &vtss_state->rx_conf;
    BOOL l3_redir_ena = FALSE;
#ifdef VTSS_FEATURE_VSTAX
    vtss_vstax_upsid_t master_upsid = vtss_state->vstax_info.master_upsid;
    u32                redir_chip_port = 0xFFFFFFFFUL;
#endif

#ifdef VTSS_FEATURE_VSTAX
    {
        vtss_vstax_upsid_t my_upsid = vtss_state->vstax_info.upsid[vtss_state->chip_no];

        // We redirect l3_XXX_queue according to table above.
        if ((rx_conf->map.l3_uc_queue    != VTSS_PACKET_RX_QUEUE_NONE  ||
             rx_conf->map.l3_other_queue != VTSS_PACKET_RX_QUEUE_NONE) &&
             VTSS_VSTAX_UPSID_LEGAL(master_upsid)                      &&
             master_upsid != my_upsid) {

            vtss_vstax_route_entry_t *master_rt = &vtss_state->vstax_info.chip_info[vtss_state->chip_no].rt_table.table[master_upsid];
            vtss_vstax_chip_info_t   *chip_info = &vtss_state->vstax_info.chip_info[vtss_state->chip_no];
            u32                      mask;

            if (master_rt->stack_port_a && chip_info->mask_a) {
                mask = chip_info->mask_a;
            } else if (master_rt->stack_port_b && chip_info->mask_b) {
                mask = chip_info->mask_b;
            } else {
                mask = 0;
            }

            // Get the index of one of the bits set in mask (if any).
            if ((redir_chip_port = VTSS_OS_CTZ(mask)) >= 32) {
                // mask was 0. Master is unreachable.
                l3_redir_ena = FALSE;
            } else {
                l3_redir_ena = TRUE;
            }
        }
    }
#endif

#ifdef VTSS_FEATURE_NPI
    if (l3_redir_ena && vtss_state->npi_conf.enable) {
        VTSS_E("Unable to enable redirection to NPI port while internal L3 redirect is active");
        return VTSS_RC_ERROR;
    }
    if (l3_redir_ena == FALSE && vtss_state->npi_conf.enable) {
        u32 qmask, i;
        for (qmask = i = 0; i < vtss_state->rx_queue_count; i++) {
            if (rx_conf->queue[i].npi.enable) {
                qmask |= VTSS_BIT(i); /* NPI redirect */
            }
        }
        if (VTSS_PORT_CHIP_SELECTED(vtss_state->npi_conf.port_no)) {
            JR_WRF(ARB, CFG_STATUS_STACK_CFG, GCPU_PORT_CFG, VTSS_CHIP_PORT(vtss_state->npi_conf.port_no));
            JR_WRF(ARB, CFG_STATUS_STACK_CFG, CPU_TO_GCPU_MASK, qmask);
        } else {
            // Redirect to one of the internal ports.
            JR_WRF(ARB, CFG_STATUS_STACK_CFG, GCPU_PORT_CFG, vtss_state->port_int_0);
            JR_WRF(ARB, CFG_STATUS_STACK_CFG, CPU_TO_GCPU_MASK, qmask);
        }
    } else
#endif
#ifdef VTSS_FEATURE_VSTAX
    if (l3_redir_ena) {
        u32 mask = 0;

        VTSS_IG(VTSS_TRACE_GROUP_PACKET, "Redirecting queue #%u of chip = %u to upsid = %d", rx_conf->map.l3_uc_queue, vtss_state->chip_no, master_upsid);
        // Remote CPU is the UPSID of the master.
        JR_WRF(REW, COMMON_VSTAX_GCPU_CFG, GCPU_UPSID, master_upsid);

        // Remote CPU queue is the highest CPU queue that this revision of the chip supports
        // without putting the frame into the error queue (identified by stack_queue) on
        // reception at the master. This number is 2.
        JR_WRF(REW, COMMON_VSTAX_GCPU_CFG, GCPU_UPSPN, 2);

        // The CPU queue(s) to redirect.
        if (rx_conf->map.l3_uc_queue != VTSS_PACKET_RX_QUEUE_NONE) {
            mask |=  VTSS_BIT(rx_conf->map.l3_uc_queue);
        }
        if (rx_conf->map.l3_other_queue != VTSS_PACKET_RX_QUEUE_NONE) {
            mask |=  VTSS_BIT(rx_conf->map.l3_other_queue);
        }
        JR_WRF(ARB, CFG_STATUS_STACK_CFG, CPU_TO_GCPU_MASK, mask);

        // Send the frame in the right direction.
        JR_WRF(ARB, CFG_STATUS_STACK_CFG, GCPU_PORT_CFG, redir_chip_port);
    } else
#endif
    {
        VTSS_IG(VTSS_TRACE_GROUP_PACKET, "Redirection disabled");
        JR_WRF(ARB, CFG_STATUS_STACK_CFG, GCPU_PORT_CFG, 63); /* == disabled */
        JR_WRF(ARB, CFG_STATUS_STACK_CFG, CPU_TO_GCPU_MASK, 0); /* zero mask to be sure */
    }

    return VTSS_RC_OK;
}
#endif

/* ================================================================= *
 * NPI
 * ================================================================= */

#if defined(VTSS_FEATURE_NPI)
static vtss_rc jr_npi_mask_set_chip(void)
{
    VTSS_RC(jr_cpu_queue_redirect_chip());

    /* Control VStaX-2 awareness */
    if (vtss_state->npi_conf.port_no != VTSS_PORT_NO_NONE && VTSS_PORT_CHIP_SELECTED(vtss_state->npi_conf.port_no)) {
        u32 chip_port = VTSS_CHIP_PORT(vtss_state->npi_conf.port_no);
        JR_WRXB(REW, PHYSPORT_VSTAX_CTRL, chip_port, VSTAX_HDR_ENA, vtss_state->npi_conf.enable);
        JR_WRXB(ASM, CFG_ETH_CFG, chip_port, ETH_VSTAX2_AWR_ENA, vtss_state->npi_conf.enable);
        JR_WRXB(ANA_CL_2, PORT_STACKING_CTRL, chip_port, STACKING_AWARE_ENA, vtss_state->npi_conf.enable);
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_npi_conf_set(const vtss_npi_conf_t *const conf)
{
    /* Disable current NPI port. */
    vtss_state->npi_conf.enable = FALSE;
    VTSS_RC(jr_conf_chips(jr_npi_mask_set_chip));

    /* Set new configuration */
    vtss_state->npi_conf = *conf;
    if (!vtss_state->npi_conf.enable) {
      vtss_state->npi_conf.port_no = VTSS_PORT_NO_NONE;
    }
    VTSS_RC(jr_conf_chips(jr_npi_mask_set_chip));

    /* VStaX enabled/disabled, setup all VLANs (an NPI port should not be in normal VLANs) */
    VTSS_RC(vtss_cmn_vlan_update_all());

    // Don't forget DSM taxi rate WM
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_NPI */

/* ================================================================= *
 *  Packet control
 * ================================================================= */

/* Offsets to extra entries for any frame type rules */
#define JR_IS2_ID_ANY_LLC   0x100000000LL /* 0001 -> MAC_LLC */
#define JR_IS2_ID_ANY_SNAP  0x200000000LL /* 001X -> MAC_SNAP and ARP */
#define JR_IS2_ID_ANY_OAM   0x300000000LL /* 0111 -> MAC_OAM */
#define JR_IS2_ID_ANY_IP    0x400000000LL /* 010X -> IP_TCP_UDP and IP_OTHER */
                                          /* X000 -> MAC_ETYPE */

#define JR_IS2_ANY_ETYPE_MASK 0x8 /* X000 -> MAC_ETYPE */
#define JR_IS2_ANY_IP_MASK    0x1 /* 010X -> IP_TCP_UDP and IP_OTHER */
#define JR_IS2_ANY_SNAP_MASK  0x1 /* 001X -> MAC_SNAP and ARP */

#define JR_IS2_ID_ANY_CNT   4 /* Number of extra entries */


static vtss_rc jr_is2_del_any(vtss_vcap_user_t user, vtss_vcap_id_t id, BOOL etype)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is2.obj;
    
    if (etype) {
        VTSS_RC(vtss_vcap_del(obj, user, id));
    }
    VTSS_RC(vtss_vcap_del(obj, user, id + JR_IS2_ID_ANY_LLC));
    VTSS_RC(vtss_vcap_del(obj, user, id + JR_IS2_ID_ANY_SNAP));
    VTSS_RC(vtss_vcap_del(obj, user, id + JR_IS2_ID_ANY_OAM));
    VTSS_RC(vtss_vcap_del(obj, user, id + JR_IS2_ID_ANY_IP));
    
    return VTSS_RC_OK;
}

static vtss_rc jr_is2_add_any(vtss_vcap_user_t user, 
                              vtss_vcap_id_t   id_base,
                              vtss_vcap_id_t   id_next,
                              vtss_vcap_data_t *data,
                              BOOL             etype)
{
    vtss_is2_entry_t *entry = data->u.is2.entry;
    vtss_ace_t       *ace = &entry->ace;
    vtss_vcap_obj_t  *obj = &vtss_state->is2.obj;
    vtss_vcap_id_t   id = id_base;
    
    /* Add extra any entries */
    memset(&ace->frame, 0, sizeof(ace->frame));

    /* ANY_ETYPE */
    if (etype) {
        entry->type = IS2_ENTRY_TYPE_ETYPE;
        entry->type_mask = JR_IS2_ANY_ETYPE_MASK;
        VTSS_RC(vtss_vcap_add(obj, user, id, id_next, data, 0));
    }
    
    /* ANY_IP */
    entry->type = IS2_ENTRY_TYPE_IP_OTHER;
    entry->type_mask = JR_IS2_ANY_IP_MASK;
    id_next = id;
    id = (id_base + JR_IS2_ID_ANY_IP);
    VTSS_RC(vtss_vcap_add(obj, user, id, id_next, data, 0));

    /* ANY_OAM */
    entry->type = IS2_ENTRY_TYPE_OAM;
    entry->type_mask = 0;
    id_next = id;
    id = (id_base + JR_IS2_ID_ANY_OAM);
    VTSS_RC(vtss_vcap_add(obj, user, id, id_next, data, 0));
    
    /* ANY_SNAP */
    entry->type = IS2_ENTRY_TYPE_SNAP;
    entry->type_mask = JR_IS2_ANY_SNAP_MASK;
    id_next = id;
    id = (id_base + JR_IS2_ID_ANY_SNAP);
    VTSS_RC(vtss_vcap_add(obj, user, id, id_next, data, 0));
    
    /* ANY_LLC */
    entry->type = IS2_ENTRY_TYPE_LLC;
    entry->type_mask = 0;
    id_next = id;
    id = (id_base + JR_IS2_ID_ANY_LLC);
    VTSS_RC(vtss_vcap_add(obj, user, id, id_next, data, 0));

    return VTSS_RC_OK;
}

/* Determine if first IS2 lookup must be enabled */
static BOOL jr_is2_first_enabled(void)
{
    BOOL enabled = vtss_state->rx_conf.reg.igmp_cpu_only;

#if defined(VTSS_FEATURE_IPV4_MC_SIP)
    if (vtss_state->ipmc.obj.src_count)
        enabled = 1;
#endif /* VTSS_FEATURE_IPV4_MC_SIP */
    
    return enabled;
}

/* Enable/disable IS1 and IS2 lookup for port */
static vtss_rc jr_vcap_is1_is2_set(u32 port, BOOL is1, u32 is2)
{
    JR_WRX(ANA_CL_2, PORT_ADV_CL_CFG, port, 
           JR_PUT_BIT(ANA_CL_2, PORT_ADV_CL_CFG, ADV_USER_CL_ENA, is1) |
           JR_PUT_BIT(ANA_CL_2, PORT_ADV_CL_CFG, ADV_QOS_CL_ENA, is1) |
           JR_PUT_FLD(ANA_CL_2, PORT_ADV_CL_CFG, SEC_TYPE_MAC_SNAP_ENA, IS2_LOOKUP_BOTH) |
           JR_PUT_FLD(ANA_CL_2, PORT_ADV_CL_CFG, SEC_TYPE_IP_TCPUDP_ENA, IS2_LOOKUP_BOTH) |
           JR_PUT_FLD(ANA_CL_2, PORT_ADV_CL_CFG, SEC_TYPE_IP6_TCPUDP_OTHER_ENA, IS2_LOOKUP_BOTH) |
           JR_PUT_FLD(ANA_CL_2, PORT_ADV_CL_CFG, SEC_TYPE_MAC_LLC_ENA, IS2_LOOKUP_BOTH) |
           JR_PUT_FLD(ANA_CL_2, PORT_ADV_CL_CFG, SEC_TYPE_ARP_ENA, IS2_LOOKUP_BOTH) |
           JR_PUT_FLD(ANA_CL_2, PORT_ADV_CL_CFG, SEC_TYPE_IP_OTHER_ENA, IS2_LOOKUP_BOTH) |
           JR_PUT_FLD(ANA_CL_2, PORT_ADV_CL_CFG, SEC_ENA, is2));    
    
    return VTSS_RC_OK;
}

/* Enable/disable VCAP lookups */
static vtss_rc jr_vcap_lookup_set(const vtss_port_no_t port_no)
{
    u32  port = VTSS_CHIP_PORT(port_no);
    u32  is2 = IS2_LOOKUP_NONE;
    BOOL is0 = 1, is1 = 1;

    /* Determine if first IS2 lookup is enabled */
    if (jr_is2_first_enabled()) {
        is2 |= IS2_LOOKUP_FIRST;
    }
    
    /* Second IS2 lookup is enabled if ACLs are enabled on port */
    if (vtss_state->acl_port_conf[port_no].policy_no != VTSS_ACL_POLICY_NO_NONE) {
        is2 |= IS2_LOOKUP_SECOND;
    }
    
#if defined(VTSS_FEATURE_VSTAX_V2)
    if (!jr_is_frontport(port_no)) {
        /* Disable VCAP lookups on stack ports */
        is0 = 0;
        is1 = 0;
        is2 &= ~IS2_LOOKUP_SECOND; /* First lookup can be enabled for IGMP/SSM */
    }
#endif /* VTSS_FEATURE_VSTAX_V2 */

    VTSS_RC(jr_vcap_is1_is2_set(port, is1, is2));

    /* DBL_VID_ENA */
    JR_WRX(ANA_CL_2, PORT_IS0_CFG_1, port, 
           VTSS_BIT(1) | JR_PUT_BIT(ANA_CL_2, PORT_IS0_CFG_1, IS0_ENA, is0));

    return VTSS_RC_OK;
}

/* Advanced REDIR_PGID fields */
#define JR_IS2_REDIR_PGID_ASM ((0<<10) | (1<<3)) /* Force ASM in MAC table */

/* BZ 2902: Handle IGMP CPU redirect using IS2.
   If IGMP is enabled, both IS2 lookups are used and an IGMP IS2 rule is added 
   for the first lookup. After this rule, a block of IS2 rules are also added to 
   allow any frame. This is done to ensure that the default action is never hit for the
   first lookup (this would cause problems with counters and default actions).
   If IGMP is disabled, only the second lookup is used and the IS2 rules are deleted. */
static vtss_rc jr_igmp_cpu_copy_set(void)
{
    vtss_packet_rx_conf_t *conf = &vtss_state->rx_conf;
    vtss_vcap_obj_t       *obj = &vtss_state->is2.obj;
    vtss_vcap_data_t      data;
    vtss_vcap_id_t        id = 1;
    vtss_is2_entry_t      entry;
    vtss_ace_t            *ace = &entry.ace;
    vtss_port_no_t        port_no;
    
    /* Initialize IS2 data */
    memset(&data, 0, sizeof(data));
    memset(&entry, 0, sizeof(entry));
    data.u.is2.entry = &entry;
    entry.first = 1;
    entry.action_ext = JR_IS2_REDIR_PGID_ASM;
    entry.include_int_ports = 1;
    entry.include_stack_ports = 1;
    ace->port_no = VTSS_PORT_NO_NONE;
    ace->action.learn = 1;
    ace->action.forward = 1;

    if (jr_is2_first_enabled()) {
        /* Add block of default entries */
        VTSS_RC(jr_is2_add_any(VTSS_IS2_USER_IGMP_ANY, id, VTSS_VCAP_ID_LAST, &data, 1));
    } else {
        /* Delete block of default entries */
        VTSS_RC(jr_is2_del_any(VTSS_IS2_USER_IGMP_ANY, id, 1));
    }

    if (conf->reg.igmp_cpu_only) {
        /* Add IGMP entry */
        entry.include_int_ports = 0;
        entry.include_stack_ports = 0;
        entry.type = IS2_ENTRY_TYPE_IP_OTHER;
        entry.type_mask = 0x1;
        ace->type = VTSS_ACE_TYPE_IPV4;
        ace->frame.ipv4.proto.value = 2;
        ace->frame.ipv4.proto.mask = 0xff;
        ace->action.forward = 0;
        ace->action.cpu = 1;
        ace->action.cpu_queue = conf->map.igmp_queue;
        VTSS_RC(vtss_vcap_add(obj, VTSS_IS2_USER_IGMP, id, VTSS_VCAP_ID_LAST, &data, 0));
    } else {
        /* Delete IGMP entry */
        VTSS_RC(vtss_vcap_del(obj, VTSS_IS2_USER_IGMP, id));
    }

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_SELECT_CHIP_PORT_NO(port_no);
        VTSS_RC(jr_vcap_lookup_set(port_no));
    }

    return VTSS_RC_OK;
}

#ifdef VTSS_FEATURE_VSTAX_V2
static vtss_rc jr_cpu_pol_setup_chip(void)
{
    vtss_policer_t             pol_conf;
    vtss_policer_ext_t         pol_ext_conf;
    vtss_packet_rx_queue_map_t *map;
    u32                        q, port;

    /* Only setup internal ports if dual device target */
    if (vtss_state->chip_count != 2) {
        return VTSS_RC_OK;
    }

    map = &vtss_state->rx_conf.map;

    /* Use policers to avoid CPU copy on internal ports.
     * If we didn't do this, frames that for some reason come
     * to both chips (e.g. broadcast frames) would be copied
     * twice to the CPU - once on the primary and once on the
     * secondary chip.
     */
    memset(&pol_conf,     0, sizeof(pol_conf));
    memset(&pol_ext_conf, 0, sizeof(pol_ext_conf));

    pol_ext_conf.to_cpu = 1;
    pol_ext_conf.limit_cpu_traffic = 1;
    for (q = 0; q < 8; q++) {
        if (q != map->sflow_queue && q != map->stack_queue && q != map->l3_other_queue && q != map->l3_uc_queue) {
            /* We need multiple CPU copies for sFlow frames */
            /* In reality, we only need to allow l3_other_queue and l3_uc_queue on
               the primary chip of a JR-48 and only if it is master. */
            pol_ext_conf.cpu_queue[q] = 1;
        }
    }

    for (port = vtss_state->port_int_0; port <= vtss_state->port_int_1; port++) {
        /* Use policers to avoid CPU copy */
        VTSS_RC(jr_port_policer_set(port, 0, &pol_conf, &pol_ext_conf));
    }

    return VTSS_RC_OK;
}
#endif

#ifdef VTSS_FEATURE_VSTAX_V2
static vtss_rc jr_lrn_all_set_chip(void)
{
    /* S/W-assisted H/W learning (see Bugzilla#7730 & 7737) */
    vtss_vstax_chip_info_t *chip_info = &vtss_state->vstax_info.chip_info[vtss_state->chip_no];
    BOOL stacking = chip_info->mask_a != 0 || chip_info->mask_b != 0;

    if (stacking) {
        u32 qu;
        vtss_packet_rx_queue_map_t *map = &vtss_state->rx_conf.map;
        u32 rmask = VTSS_F_ANA_L2_COMMON_VSTAX_CTRL_VSTAX2_LCPU_CP_NEW_FRONT_ENA     |
                    VTSS_F_ANA_L2_COMMON_VSTAX_CTRL_VSTAX2_LCPU_CP_MOVE_FRONT_ENA    |
                    VTSS_F_ANA_L2_COMMON_VSTAX_CTRL_VSTAX2_LCPU_CP_REFRESH_FRONT_ENA |
                    VTSS_F_ANA_L2_COMMON_VSTAX_CTRL_VSTAX2_LCPU_CP_NXT_LRN_ALL_FRONT_ENA;

        /* Turn off CPU-copying of learn-all frames while setting up the queues. */
        JR_WRM(ANA_L2, COMMON_VSTAX_CTRL, 0, rmask);

        /* Set up the learn-all CPU queue */
        JR_WRF(ANA_L2, COMMON_VSTAX_CTRL, CPU_VSTAX_QU, map->lrn_all_queue == VTSS_PACKET_RX_QUEUE_NONE ? 0 : map->lrn_all_queue);

        if (map->lrn_all_queue == VTSS_PACKET_RX_QUEUE_NONE) {
            // Release any truncations.
            for (qu = 0; qu < vtss_state->rx_queue_count; qu++) {
                JR_WRX(DEVCPU_QS, XTR_XTR_FRM_PRUNING, qu, 0);
            }
        } else {
            /* Truncate the lrn-all queue to 64 byte packets + 24 byte IFH under the assumption
             * that this queue is *only* used for learn-all frames. This is also assumed by the FDMA code. */
            JR_WRX(DEVCPU_QS, XTR_XTR_FRM_PRUNING, map->lrn_all_queue, ((64 + 24) / sizeof(u32)) - 1);
        }

        /* Make sure that learn-all frames that go to the CPU for multiple reasons are received twice
         * (both in the learn-all queue and the most-significant other queue).
         * The reason for this is that the learn-all frames are re-injected on the stack ports
         * deep down in the FDMA driver, and never sent to the application code, so if the frame
         * at the same time is marked as CPU-copy/redirect for some other reason, we must get it
         * into that other CPU Rx queue.
         */
        JR_WR(ANA_AC, PS_COMMON_CPU_CFG, map->lrn_all_queue == VTSS_PACKET_RX_QUEUE_NONE ? VTSS_M_ANA_AC_PS_COMMON_CPU_CFG_ONE_CPU_COPY_ONLY_MASK : VTSS_M_ANA_AC_PS_COMMON_CPU_CFG_ONE_CPU_COPY_ONLY_MASK & ~(VTSS_BIT(map->lrn_all_queue)));

        /* Turn on CPU-copying of learn-all frames now that everything is in place */
        if (map->lrn_all_queue != VTSS_PACKET_RX_QUEUE_NONE) {
            JR_WRM(ANA_L2, COMMON_VSTAX_CTRL, rmask, rmask);
        }
    }
    return VTSS_RC_OK;
}
#endif

static vtss_rc jr_rx_conf_set_chip(void)
{
    vtss_packet_rx_conf_t      *conf = &vtss_state->rx_conf;
    vtss_packet_rx_reg_t       *reg = &conf->reg;
    vtss_packet_rx_queue_map_t *map = &conf->map;
    vtss_packet_rx_port_conf_t *port_conf;
    vtss_packet_reg_type_t     type;
    vtss_port_no_t             port_no;
    u32                        port, i, j, cap_cfg, garp_cfg, bpdu_cfg, val;
    BOOL                       cpu_only;
    u32                        hwm, lwm, atop, qu_id, qu;

    /* Setup Rx registrations for all ports */
    cap_cfg = (JR_PUT_BIT(ANA_CL_2, PORT_CAPTURE_CFG, CPU_MLD_REDIR_ENA, reg->mld_cpu_only) |
               JR_PUT_BIT(ANA_CL_2, PORT_CAPTURE_CFG, CPU_IP4_MC_COPY_ENA, reg->ipmc_ctrl_cpu_copy) |
               JR_PUT_BIT(ANA_CL_2, PORT_CAPTURE_CFG, CPU_IGMP_REDIR_ENA, reg->igmp_cpu_only));
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!VTSS_PORT_CHIP_SELECTED(port_no))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        port_conf = &vtss_state->rx_port_conf[port_no];
        bpdu_cfg = 0;
        garp_cfg = 0;
        for (i = 0; i < 32; i++) {
            if (i < 16) {
                /* BPDU */
                j = i;
                type = port_conf->bpdu_reg[j];
                cpu_only = reg->bpdu_cpu_only;
            } else {
                /* GARP */
                j = (i - 16);
                type = port_conf->garp_reg[j];
                cpu_only = reg->garp_cpu_only[j];
            }
            switch (type) {
            case VTSS_PACKET_REG_NORMAL:
                val = (cpu_only ? 1 : 0);
                break;
            case VTSS_PACKET_REG_FORWARD:
                val = 0;
                break;
            case VTSS_PACKET_REG_DISCARD:
                val = 3;
                break;
            case VTSS_PACKET_REG_CPU_COPY:
                val = 2;
                break;
            case VTSS_PACKET_REG_CPU_ONLY:
                val = 1;
                break;
            default:
                val = 0;
                break;
            }
            val = (val << (j * 2));
            if (i < 16)
                bpdu_cfg |= val;
            else
                garp_cfg |= val;
        }
        JR_WRX(ANA_CL_2, PORT_CAPTURE_CFG, port, cap_cfg);
        JR_WRX(ANA_CL_2, PORT_CAPTURE_GXRP_CFG, port, garp_cfg);
        JR_WRX(ANA_CL_2, PORT_CAPTURE_BPDU_CFG, port, bpdu_cfg);
    }

    /* CPU OQS ports 81-88 (ethernet->CPU). Only one queue per port. */
    for (port = 81; port < 89; port++) {
        hwm  = conf->queue[port - 81].size / 160;
        atop = (hwm == 0) ? 0 : (hwm + 60);
        lwm = hwm > 13 ? (hwm - 13) : 0;
        qu_id = port_prio_2_qu(port, 0, 1);

        JR_WRXF(OQS, QU_RAM_CFG_QU_RC_HLWM_CFG, qu_id, QU_RC_PROFILE_HWM, hwm);
        JR_WRXF(OQS, QU_RAM_CFG_QU_RC_HLWM_CFG, qu_id, QU_RC_PROFILE_LWM, lwm);
        JR_WRXF(OQS, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG, qu_id, QU_RC_PROFILE_ATOP, atop);

        JR_WRXF(OQS, PORT_RAM_CFG_PORT_RC_HLWM_CFG, port, PORT_RC_PROFILE_HWM, hwm);
        JR_WRXF(OQS, PORT_RAM_CFG_PORT_RC_HLWM_CFG, port, PORT_RC_PROFILE_LWM, lwm);
        JR_WRXF(OQS, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG, port, PORT_RC_PROFILE_ATOP, atop);              
    }
    /* CPU IQS ports 33 and 34 (CPU->ethernet). Setting FWD pressure to 1 to force IQS frames to OQS */
    for (port = 33; port < 35; port++) {
        for (qu = 0; qu < 8; qu++) {
            qu_id = port_prio_2_qu(port, qu, 0);
            JR_WRXF(IQS, QU_RAM_CFG_QU_RC_CFG, qu_id, FWD_PRESS_THRES, 1);
        }
    }

    /* Setup queue mappings */
    bpdu_cfg = (JR_PUT_FLD(ANA_CL_2, COMMON_CPU_BPDU_QU_CFG, CPU_GXRP_QU, map->garp_queue) |
                JR_PUT_FLD(ANA_CL_2, COMMON_CPU_BPDU_QU_CFG, CPU_BPDU_QU, map->bpdu_queue));
    for (i = 0; i < 16; i++) {
        JR_WRX(ANA_CL_2, COMMON_CPU_BPDU_QU_CFG, i, bpdu_cfg);
    }
    JR_WR(ANA_CL_2, COMMON_CPU_PROTO_QU_CFG,
          JR_PUT_FLD(ANA_CL_2, COMMON_CPU_PROTO_QU_CFG, CPU_MLD_QU, map->igmp_queue) |
          JR_PUT_FLD(ANA_CL_2, COMMON_CPU_PROTO_QU_CFG, CPU_IP4_MC_CTRL_QU, map->ipmc_ctrl_queue) |
          JR_PUT_FLD(ANA_CL_2, COMMON_CPU_PROTO_QU_CFG, CPU_IGMP_QU, map->igmp_queue));
    JR_WRF(ANA_L2, COMMON_LRN_CFG, CPU_LRN_QU, map->learn_queue);
    JR_WRF(ANA_L2, COMMON_LRN_CFG, CPU_SMAC_QU, map->mac_vid_queue);
    JR_WRF(ANA_L2, COMMON_FWD_CFG, CPU_DMAC_QU, map->mac_vid_queue);
    JR_WRB(ANA_L2, COMMON_FWD_CFG, CPU_DMAC_COPY_ENA, 1);

#ifdef VTSS_FEATURE_VSTAX_V2
    VTSS_RC(jr_lrn_all_set_chip());

    VTSS_RC(jr_cpu_queue_redirect_chip());

    /* Use policers to avoid CPU copy */
    VTSS_RC(jr_cpu_pol_setup_chip());
#endif

    // Configure L3 routing CPU queues
    JR_WR(ANA_L3_2, COMMON_CPU_QU_CFG,
          JR_PUT_FLD(ANA_L3_2, COMMON_CPU_QU_CFG, CPU_RLEG_QU,             map->l3_uc_queue)    |
          JR_PUT_FLD(ANA_L3_2, COMMON_CPU_QU_CFG, CPU_RLEG_IP_OPT_QU,      map->l3_other_queue) |
          JR_PUT_FLD(ANA_L3_2, COMMON_CPU_QU_CFG, CPU_RLEG_IP_HDR_FAIL_QU, map->l3_other_queue) |
          JR_PUT_FLD(ANA_L3_2, COMMON_CPU_QU_CFG, CPU_UC_QU,               map->l3_uc_queue)    |
          JR_PUT_FLD(ANA_L3_2, COMMON_CPU_QU_CFG, CPU_MC_FAIL_QU,          map->l3_other_queue) |
          JR_PUT_FLD(ANA_L3_2, COMMON_CPU_QU_CFG, CPU_UC_FAIL_QU,          map->l3_uc_queue)    |
          JR_PUT_FLD(ANA_L3_2, COMMON_CPU_QU_CFG, CPU_IP_TTL_FAIL_QU,      map->l3_other_queue));

    if (map->sflow_queue != VTSS_PACKET_RX_QUEUE_NONE) {
        JR_WRF(ANA_AC, PS_COMMON_SFLOW_CFG, SFLOW_CPU_QU, map->sflow_queue);
    }

    /* Due to a bug in JR1, CPU-generated stack traffic destined for a CPU queue with
     * a number greater than 2, will get redirected to the error queue. In order to
     * receive such frames in the stack queue number, we must configure the
     * error queue to the stack queue.
     */
    if (map->stack_queue != VTSS_PACKET_RX_QUEUE_NONE) {
        JR_WRF(ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, VSTAX2_FWD_ERR_QU, map->stack_queue);
    }

    /* Enable frame extraction queues and setup mapping to extraction groups */
    for (i = 0; i < JR_PACKET_RX_QUEUE_CNT; i++) {
        JR_WRX(DEVCPU_QS, XTR_XTR_MAP, i,
               JR_PUT_FLD(DEVCPU_QS, XTR_XTR_MAP, GRP, conf->grp_map[i]) |
               JR_PUT_BIT(DEVCPU_QS, XTR_XTR_MAP, MAP_ENA, 1));
    }

#if defined(VTSS_FEATURE_NPI)
    VTSS_RC(jr_npi_mask_set_chip());
#endif

    return VTSS_RC_OK;
}

static vtss_rc jr_rx_conf_set(void) 
{
    /* Setup config for all devices */
    VTSS_RC(jr_conf_chips(jr_rx_conf_set_chip));

    /* Register IGMP */
    VTSS_RC(jr_igmp_cpu_copy_set());
    
    return VTSS_RC_OK;
}

#define XTR_EOF_0     0x80000000U
#define XTR_EOF_1     0x80000001U
#define XTR_EOF_2     0x80000002U
#define XTR_EOF_3     0x80000003U
#define XTR_PRUNED    0x80000004U
#define XTR_ABORT     0x80000005U
#define XTR_ESCAPE    0x80000006U
#define XTR_NOT_READY 0x80000007U

static vtss_rc jr_rx_frame_discard_internal(const vtss_chip_no_t       chip_no,
                                            const vtss_packet_rx_grp_t grp,
                                                  u32                  *discarded_bytes)
{
    BOOL done = 0;
    u32  val;

    *discarded_bytes = 0;

    while (!done) {
        JR_RDX_CHIP(chip_no, DEVCPU_QS, XTR_XTR_RD, grp, &val);
        switch(val) {
        case XTR_ABORT:
        case XTR_PRUNED:
        case XTR_EOF_3:
        case XTR_EOF_2:
        case XTR_EOF_1:
        case XTR_EOF_0:
            done = 1;
            /* FALLTHROUGH */
        case XTR_ESCAPE:
            JR_RDX_CHIP(chip_no, DEVCPU_QS, XTR_XTR_RD, grp, &val);
            break;
        case XTR_NOT_READY:
            done = 1;
            break;
        default:
            (*discarded_bytes)++;
            break;
        }
    }

    *discarded_bytes *= 4;

    return VTSS_RC_OK;
}

/**
 * Return values:
 *  0 = Data OK.
 *  1 = EOF reached. Data OK. bytes_valid indicates the number of valid bytes in last word ([1; 4]).
 *  2 = Error. No data from queue system.
 */
static int jr_rx_frame_word(vtss_chip_no_t chip_no, vtss_packet_rx_grp_t grp, BOOL first_word, u32 *val, u32 *bytes_valid)
{
    JR_RDX_CHIP(chip_no, DEVCPU_QS, XTR_XTR_RD, grp, val);

    if (*val == XTR_NOT_READY) {
        /** XTR_NOT_READY means two different things depending on whether this is the first
         * word read of a frame or after at least one word has been read.
         * When the first word, the group is empty, and we return an error.
         * Otherwise we have to wait for the FIFO to have received some more data. */
        if (first_word) {
          return 2; /* Error */
        }
        do {
            JR_RDX_CHIP(chip_no, DEVCPU_QS, XTR_XTR_RD, grp, val);
        } while(*val == XTR_NOT_READY);
    }

    switch(*val) {
    case XTR_ABORT:
        /* No accompanying data. */
        VTSS_E("Aborted frame");
        return 2; /* Error */
    case XTR_EOF_0:
    case XTR_EOF_1:
    case XTR_EOF_2:
    case XTR_EOF_3:
    case XTR_PRUNED:
        *bytes_valid = 4 - ((*val) & 3);
        JR_RDX_CHIP(chip_no, DEVCPU_QS, XTR_XTR_RD, grp, val);
        if (*val == XTR_ESCAPE) {
            JR_RDX_CHIP(chip_no, DEVCPU_QS, XTR_XTR_RD, grp, val);
        }
        return 1; /* EOF */
    case XTR_ESCAPE:
        JR_RDX_CHIP(chip_no, DEVCPU_QS, XTR_XTR_RD, grp, val);
        /* FALLTHROUGH */
    default:
        *bytes_valid = 4;
        return 0;
    }
}

static vtss_rc jr_rx_frame_get_internal(vtss_chip_no_t       chip_no,
                                        vtss_packet_rx_grp_t grp,
                                        u8                   *const ifh,
                                        u8                   *const frame,
                                        const u32            buf_length,
                                        u32                  *frm_length) /* Including CRC */
{
    u32     i, val, bytes_valid, buf_len = buf_length;
    BOOL    done = 0, first_word = 1;
    u8      *buf;
    int     result;


    /* Check if data is ready for grp */
    JR_RDF_CHIP(chip_no, DEVCPU_QS, XTR_XTR_DATA_PRESENT, DATA_PRESENT_GRP, &val);
    if (!(val & VTSS_BIT(grp))) {
        return VTSS_RC_ERROR;
    }

    *frm_length = 0;
    buf = ifh;

    /* Read IFH. It consists of a 1 word queue number and 6 words of real IFH, hence 7 words */
    for (i = 0; i < 7; i++) {
        if (jr_rx_frame_word(chip_no, grp, first_word, &val, &bytes_valid) != 0) {
            /* We accept neither EOF nor ERROR when reading the IFH */
            return VTSS_RC_ERROR;
        }
        *buf++ = (u8)(val >>  0);
        *buf++ = (u8)(val >>  8);
        *buf++ = (u8)(val >> 16);
        *buf++ = (u8)(val >> 24);
        first_word = 0;
    }

    buf = frame;

    /* Read the rest of the frame */
    while (!done && buf_len >= 4) {
        result = jr_rx_frame_word(chip_no, grp, 0, &val, &bytes_valid);
        if (result == 2) {
            // Error.
            return VTSS_RC_ERROR;
        }
        // Store the data.
        *frm_length += bytes_valid;
        *buf++ = (u8)(val >>  0);
        *buf++ = (u8)(val >>  8);
        *buf++ = (u8)(val >> 16);
        *buf++ = (u8)(val >> 24);
        buf_len -= bytes_valid;
        done = result == 1;
    }

    if (!done) {
        /* Buffer overrun. Skip remainder of frame */
        u32 discarded_bytes;
        (void)jr_rx_frame_discard_internal(chip_no, grp, &discarded_bytes);
        *frm_length += discarded_bytes;
        return VTSS_RC_INCOMPLETE; // Special return value indicating that there wasn't room.
    }

    if (*frm_length < 60) {
        VTSS_E("short frame, %u bytes read", *frm_length);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}


static vtss_rc jr_rx_frame_discard(const vtss_packet_rx_queue_t queue_no)
{
    vtss_packet_rx_grp_t grp = vtss_state->rx_conf.grp_map[queue_no];
    u32 dummy;
    return jr_rx_frame_discard_internal(vtss_state->chip_no, grp, &dummy);
}

static vtss_rc jr_rx_frame_get_chip(const vtss_packet_rx_queue_t queue_no,
                                    vtss_packet_rx_header_t      *const header,
                                    u8                           *const frame,
                                    const u32                    length)
{
    vtss_rc              rc;
    vtss_packet_rx_grp_t grp = vtss_state->rx_conf.grp_map[queue_no];
    u32                  val, port;
    u8                   ifh[28];
    u64                  vstax_lo, fwd;

    /* Check if data is ready in queue */
    JR_RDF(DEVCPU_QS, XTR_XTR_DATA_PRESENT, DATA_PRESENT, &val);
    if (!(val & VTSS_BIT(queue_no))) {
        return VTSS_RC_INCOMPLETE;
    }

    if ((rc = jr_rx_frame_get_internal(vtss_state->chip_no, grp, ifh, frame, length, &header->length)) != VTSS_RC_OK) {
        return rc;
    }

    header->length -= 4; /* jr_rx_frame_get_internal() returns frame length including CRC. This function returns frame length excluding CRC. */
    vstax_lo = ((u64)ifh[14] << 56) | ((u64)ifh[15] << 48) | ((u64)ifh[16] << 40) | ((u64)ifh[17] << 32) |
               ((u64)ifh[18] << 24) | ((u64)ifh[19] << 16) | ((u64)ifh[20] <<  8) | ((u64)ifh[21] <<  0);
    fwd      =                                               ((u64)ifh[22] << 40) | ((u64)ifh[23] << 32) |
               ((u64)ifh[24] << 24) | ((u64)ifh[25] << 16) | ((u64)ifh[26] <<  8) | ((u64)ifh[27] <<  0);

    header->arrived_tagged = VTSS_EXTRACT_BITFIELD64(vstax_lo, 15,  1); /* vstax::tag::was_tagged */
    header->tag.vid        = VTSS_EXTRACT_BITFIELD64(vstax_lo, 16, 12); /* vstax::tag::cl_vid     */
    header->tag.cfi        = VTSS_EXTRACT_BITFIELD64(vstax_lo, 28,  1); /* vstax::tag::dei        */
    header->tag.tagprio    = VTSS_EXTRACT_BITFIELD64(vstax_lo, 29,  3); /* vstax::tag::pcp        */
    port                   = VTSS_EXTRACT_BITFIELD64(fwd,      39,  6); /* fwd::src_port          */
    header->learn          = 0;                                         /* TBD */
    header->port_no        = vtss_cmn_chip_to_logical_port(vtss_state, vtss_state->chip_no, port);

    return VTSS_RC_OK;
}

static vtss_rc jr_rx_frame_get(const vtss_packet_rx_queue_t queue_no,
                               vtss_packet_rx_header_t      *const header,
                               u8                           *const frame,
                               const u32                    length)
{
    vtss_chip_no_t chip_no;
    vtss_rc        rc;

    // Try all units in consecutive order.
    // If jr_rx_frame_get_chip() returns something different from
    // VTSS_RC_INCOMPLETE, then stop, because VTSS_RC_INCOMPLETE
    // indicates that the chip we're reading has no pending frames.
    // All other return codes indicate success or other kind of
    // error.
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        rc = jr_rx_frame_get_chip(queue_no, header, frame, length);
        if (rc != VTSS_RC_INCOMPLETE) {
            // Success of failure.
            return rc;
        }
    }

    // Last chip and no frames found on this one either.
    return VTSS_RC_ERROR;
}

static inline u32 BYTE_SWAP(u32 v)
{
    register u32 v1 = v;
    v1 = ((v1 >> 24) & 0x000000FF) | ((v1 >> 8) & 0x0000FF00) | ((v1 << 8) & 0x00FF0000) | ((v1 << 24) & 0xFF000000);
    return v1;
}

static vtss_rc jr_tx_frame_port(const vtss_port_no_t  port_no,
                                const u8              *const frame,
                                const u32             length,
                                const vtss_vid_t      vid)
{
    u32 value, count, w, last, ifh[6], grp = 0; /* First injection group used */
    const u8 *buf = frame;

    VTSS_N("port_no: %u, length: %u, vid: %u", port_no, length, vid);

    VTSS_SELECT_CHIP_PORT_NO(port_no);
    JR_RD(DEVCPU_QS, INJ_INJ_STATUS, &value);
    if (!(JR_GET_FLD(DEVCPU_QS, INJ_INJ_STATUS, FIFO_RDY, value) & VTSS_BIT(grp))) {
        VTSS_E("Not ready");
        return VTSS_RC_ERROR;
    }

    if (JR_GET_FLD(DEVCPU_QS, INJ_INJ_STATUS, WMARK_REACHED, value) & VTSS_BIT(grp)) {
        VTSS_E("WM reached");
        return VTSS_RC_ERROR;
    }
    
    /* Indicate SOF */
    JR_WRX(DEVCPU_QS, INJ_INJ_CTRL, grp, JR_PUT_BIT(DEVCPU_QS, INJ_INJ_CTRL, SOF, 1));

    ifh[0]  = 0;                                                   // MSW(dst)
    ifh[1]  = VTSS_ENCODE_BITFIELD(VTSS_CHIP_PORT(port_no), 0, 8); // LSW(dst).uc_inject_dst
    ifh[2]  = VTSS_ENCODE_BITFIELD(1, 79 - 48, 1);                 // vstax.resv (must be 1)
    ifh[2] |= VTSS_ENCODE_BITFIELD(1, 59 - 48, 1);                 // vstax.sp
    ifh[3]  = 0;
    ifh[4]  = VTSS_ENCODE_BITFIELD(1, 45 - 32, 1);                 // fwd.vstax_avail
    ifh[4] |= VTSS_ENCODE_BITFIELD(1, 44 - 32, 1);                 // fwd.update_fcs
    ifh[5]  = VTSS_ENCODE_BITFIELD(1, 28 -  0, 1);                 // fwd.do_not_rew
    ifh[5] |= VTSS_ENCODE_BITFIELD(2, 16 -  0, 2);                 // fwd.inject_mode = INJECT_UNICAST.

    for (w = 0; w < 6; w++) {
        JR_WRX(DEVCPU_QS, INJ_INJ_WR, grp, BYTE_SWAP(ifh[w]));
    }
            
    /* Write whole words */
    count = (length / 4);
    for (w = 0; w < count; w++, buf += 4) {
        if (w == 3 && vid != VTSS_VID_NULL) {
            /* Insert C-tag */
            JR_WRX(DEVCPU_QS, INJ_INJ_WR, grp, BYTE_SWAP((0x8100U << 16) | vid));
            w++;
        }
        JR_WRX(DEVCPU_QS, INJ_INJ_WR, grp, (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
    }

    /* Write last, partial word */
    last = (length % 4);
    if (last) {
        JR_WRX(DEVCPU_QS, INJ_INJ_WR, grp, (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
        w++;
    }
        
    /* Indicate EOF and valid bytes in last word */
    JR_WRX(DEVCPU_QS, INJ_INJ_CTRL, grp, 
           JR_PUT_FLD(DEVCPU_QS, INJ_INJ_CTRL, VLD_BYTES, length < 60 ? 0 : last) |
           JR_PUT_BIT(DEVCPU_QS, INJ_INJ_CTRL, EOF, 1));

    
    /* Add dummy CRC (Jaguar will do padding, if neccessary) */
    JR_WRX(DEVCPU_QS, INJ_INJ_WR, grp, 0);
    w++;

    VTSS_N("Wrote %u words, last: %u", w, last);
    
    return VTSS_RC_OK;
}

/* Find first port enabled in mask */
static u32 jr_first_port(u32 mask)
{
    u32 i, port = 0;

    for (i = 0; i < 32; i++) {
        if (mask & VTSS_BIT(i)) {
            port = i;
            break;
        }
    }
    return port;
}

/* Setup mirroring for chip */
static vtss_rc jr_mirror_conf_set_chip(void)
{
    vtss_mirror_conf_t     *conf = &vtss_state->mirror_conf;
    vtss_port_no_t         port_no = conf->port_no;
    u32                    mask = 0, port, tpid, value;
#if defined(VTSS_FEATURE_VSTAX_V2)
    vtss_vstax_chip_info_t *chip_info = &vtss_state->vstax_info.chip_info[vtss_state->chip_no];
        
    /* Include stack ports and mirror port in GMIRROR mask */
    if (chip_info->port_conf[0].mirror)
        mask |= chip_info->mask_a;
    if (chip_info->port_conf[1].mirror || 
        (vtss_state->chip_count == 2 && port_no != VTSS_PORT_NO_NONE && 
         !VTSS_PORT_CHIP_SELECTED(port_no))) {
        /* For dual device targets, the internal ports are STACK_B.
           Mirroring is enabled if the mirror port is on the other device */
        mask |= chip_info->mask_b;
    }
#endif /* VTSS_FEATURE_VSTAX_V2 */
    /* Include mirror port or internal ports in GMIRROR mask */
    mask |= (port_no == VTSS_PORT_NO_NONE ? 0 :
             VTSS_PORT_CHIP_SELECTED(port_no) ? VTSS_BIT(VTSS_CHIP_PORT(port_no)) :
             vtss_state->mask_int_ports);
    JR_WR(ANA_AC, PS_COMMON_VSTAX_GMIRROR_CFG, mask);
    
    /* Arbiter: The first enabled mirror port is setup */
    port = jr_first_port(mask);
    JR_WRX(ARB, CFG_STATUS_MIRROR_CFG, JR_MIRROR_PROBE_RX,
           VTSS_F_ARB_CFG_STATUS_MIRROR_CFG_MIRROR_PROBE_CFG(port));
    JR_WRX(ARB, CFG_STATUS_MIRROR_CFG, JR_MIRROR_PROBE_TX,
           VTSS_F_ARB_CFG_STATUS_MIRROR_CFG_MIRROR_PROBE_CFG(port) |
           VTSS_F_ARB_CFG_STATUS_MIRROR_CFG_MIRROR_PROBE_TYPE);

    /* Ingress mirroring, only enabled if mirror port enabled */
    JR_WRX(ANA_AC, MIRROR_PROBE_PROBE_CFG, JR_MIRROR_PROBE_RX, 
           VTSS_F_ANA_AC_MIRROR_PROBE_PROBE_CFG_PROBE_DIRECTION(2));
    JR_WRX(ANA_AC, MIRROR_PROBE_PROBE_PORT_CFG, JR_MIRROR_PROBE_RX,
           mask ? jr_port_mask(vtss_state->mirror_ingress) : 0);
    
    /* Egress mirroring, only enabled if mirror port enabled. Probe mode Tx (1) */
    if (mask != 0)
        mask = jr_port_mask(vtss_state->mirror_egress);
    JR_WRX(ANA_AC, MIRROR_PROBE_PROBE_CFG, JR_MIRROR_PROBE_TX, 
           VTSS_F_ANA_AC_MIRROR_PROBE_PROBE_CFG_PROBE_DIRECTION(1));
    JR_WRX(ANA_AC, MIRROR_PROBE_PROBE_PORT_CFG, JR_MIRROR_PROBE_TX, mask);
    
    /* Setup mirror tagging */
    tpid = (conf->tag == VTSS_MIRROR_TAG_S ? 1 : conf->tag == VTSS_MIRROR_TAG_S_CUSTOM ? 2 : 0);
    value = (VTSS_F_REW_COMMON_MIRROR_PROBE_CFG_MIRROR_VLAN_UPRIO(conf->pcp) |
             (conf->dei ? VTSS_F_REW_COMMON_MIRROR_PROBE_CFG_MIRROR_VLAN_CFI : 0) |
             VTSS_F_REW_COMMON_MIRROR_PROBE_CFG_MIRROR_VLAN_VID(conf->vid) |
             VTSS_F_REW_COMMON_MIRROR_PROBE_CFG_MIRROR_VLAN_TPI(tpid) |
             VTSS_F_REW_COMMON_MIRROR_PROBE_CFG_USE_MIRROR_UPRIO_ENA |
             (conf->tag == VTSS_MIRROR_TAG_NONE ? 0 : 
              VTSS_F_REW_COMMON_MIRROR_PROBE_CFG_REMOTE_MIRROR_ENA));
    JR_WRX(REW, COMMON_MIRROR_PROBE_CFG, JR_MIRROR_PROBE_RX, value);

    /* The first port enabled for egress mirroring is setup in Rewriter */
    port = jr_first_port(mask);
    JR_WRX(REW, COMMON_MIRROR_PROBE_CFG, JR_MIRROR_PROBE_TX,
           value | VTSS_F_REW_COMMON_MIRROR_PROBE_CFG_MIRROR_TX_PORT(port));

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  VSTAX (VSTAX2)
 * ================================================================= */

#if defined(VTSS_FEATURE_VSTAX_V2)

static inline BOOL jr_is_frontport(vtss_port_no_t port_no)
{
    return (vtss_state->vstax_port_enabled[port_no] ? 0 : 1);
}


// CMEF min interval (i.e. max rate)
// 
// Values set such that rate will not exceed 100Mbps, regardless of topology.
// Max interval (i.e. periodic rate) is set to 20*min_interval.
// 
// I.e. periodic CMEFs will consume 5Mbps.
// Triggered CMEFs may *theoritically* consume up to 100Mbps.
// But in all realistic scenarios, CMEF will consume ~5Mbps.
//
// Background for selecting these values:
// max_interval cannot be set higher than 0xffff (16 bit register).
// This means that in a max stack of 32 units, CMEFs will (on some links)
// consume ~31*250e6/16/0xffff*576=4.3Mbps.
// So to keep things simple (for marketing), numbers are set such 
// that periodic CMEFs always consume max ~5Mbps.
//
// The max rate (100Mbps) is somewhat arbitrary.
//
// Spreadsheet used to calculate values:
// KT:/Documents/TGS_TargetSpecification/TGS1000-1099/TGS1000-Jaguar/Input/Features/x_Stacking-toe.xlsx
//
// First index:  Number of UPSIDs in use.
// Second index: 0: Ring, 1: Chain
/*lint -esym(459, cmef_min_interval) */
static int cmef_min_interval[VTSS_VSTAX_UPSIDS][2] = {
    {    0,    0 },
    {   90,   90 },
    {   90,  180 },
    {  180,  270 },
    {  180,  360 },
    {  270,  450 },
    {  270,  540 },
    {  360,  630 },
    {  360,  720 },
    {  450,  810 },
    {  450,  900 },
    {  540,  990 },
    {  540, 1080 },
    {  630, 1170 },
    {  630, 1260 },
    {  720, 1350 },
    {  720, 1440 },
    {  810, 1530 },
    {  810, 1620 },
    {  900, 1710 },
    {  900, 1800 },
    {  990, 1890 },
    {  990, 1980 },
    { 1080, 2070 },
    { 1080, 2160 },
    { 1170, 2250 },
    { 1170, 2340 },
    { 1260, 2430 },
    { 1260, 2520 },
    { 1350, 2610 },
    { 1350, 2700 },
    { 1440, 2790 },
};

static vtss_rc jr_update_lport(vtss_port_no_t port_no, i32 lport)
{
#if defined(VTSS_FEATURE_AGGR_GLAG)
    if ((vtss_state->port_glag_no[port_no]) != VTSS_GLAG_NO_NONE) {
        vtss_port_no_t port_ix;
        u32 glag_no = vtss_state->port_glag_no[port_no];
        for (port_ix = VTSS_PORT_NO_START; port_ix < vtss_state->port_count; port_ix++) {
            if (vtss_state->port_glag_no[port_ix] == glag_no &&
                VTSS_CHIP_PORT(port_ix) < lport)
                lport = VTSS_CHIP_PORT(port_ix);
        }
    }
#endif    
    JR_WRX(DSM, CM_CFG_LPORT_NUM_CFG, VTSS_CHIP_PORT(port_no), lport);

    return VTSS_RC_OK;
}

#define CMEF_SHAPER_RATE_M(n)  (u32) (n * (1.5        * 1000000 / 100160))
#define CMEF_SHAPER_RATE_G(n)  (u32) (n * (1.5 * 1000 * 1000000 / 100160))

static u32 jr_cmef_shaper_rate(vtss_port_speed_t speed)
{
    switch (speed) {
    case VTSS_SPEED_10M:
        return CMEF_SHAPER_RATE_M(10);
    case VTSS_SPEED_100M:
        return CMEF_SHAPER_RATE_M(100);
    case VTSS_SPEED_1G:
        return CMEF_SHAPER_RATE_G(1);
    case VTSS_SPEED_2500M:
        return CMEF_SHAPER_RATE_G(2.5);
    case VTSS_SPEED_10G:
        return CMEF_SHAPER_RATE_G(10);
    case VTSS_SPEED_12G:
        return CMEF_SHAPER_RATE_G(12);
    default:;
    }
    return 0;
}

static vtss_rc jr_vstax_update_ingr_shaper(u32 chipport, vtss_port_speed_t speed, BOOL front_port)
{
    u32 ingr_rate = front_port ? jr_cmef_shaper_rate(speed) : 0;
    /* Set INGR_SHAPER_CFG */
    JR_WRXM(ARB, CFG_STATUS_INGR_SHAPER_CFG, chipport, 
            JR_PUT_FLD(ARB, CFG_STATUS_INGR_SHAPER_CFG, INGR_SHAPER_RATE, ingr_rate) |
            JR_PUT_FLD(ARB, CFG_STATUS_INGR_SHAPER_CFG, INGR_SHAPER_THRES, 1) | /* n * 4K */
            JR_PUT_FLD(ARB, CFG_STATUS_INGR_SHAPER_CFG, INGR_SHAPER_MODE, front_port ? 1 : 0), /* Enabled: MC once */
            JR_MSK(ARB, CFG_STATUS_INGR_SHAPER_CFG, INGR_SHAPER_RATE) |
            JR_MSK(ARB, CFG_STATUS_INGR_SHAPER_CFG, INGR_SHAPER_MODE) |
            JR_MSK(ARB, CFG_STATUS_INGR_SHAPER_CFG, INGR_SHAPER_THRES));
    return VTSS_RC_OK;
}

static vtss_rc jr_cmef_topology_change(void)
{
    JR_WR(DSM, CM_CFG_CMEF_UPSID_ACTIVE_CFG, vtss_state->vstax_conf.cmef_disable ? 0 : vtss_state->vstax_info.chip_info[vtss_state->chip_no].upsid_active);
    return VTSS_RC_OK;
}

static vtss_rc jr_vstax_topology_set_chip(void)
{
    u32                    upsid_active_mask = 0, upsid_active_count = 0;
    u32                    cmef_int_min, cmef_int_max;
    vtss_vstax_upsid_t     upsid;
    vtss_vstax_chip_info_t *chip_info = &vtss_state->vstax_info.chip_info[vtss_state->chip_no];

    for (upsid = VTSS_VSTAX_UPSID_MIN; upsid <= VTSS_VSTAX_UPSID_MAX; upsid++) {
        vtss_vstax_route_entry_t *rt = &chip_info->rt_table.table[upsid];
        u32 mask = 0;
        if(rt->stack_port_a)
            mask |= chip_info->mask_a;
        if(rt->stack_port_b)
            mask |= chip_info->mask_b;
        JR_WRX(ANA_AC, UPSID_UPSID_CFG, upsid, mask);
        /* Unit routeable both ways? */
        if (rt->stack_port_a && rt->stack_port_b &&
            chip_info->rt_table.topology_type != VTSS_VSTAX_TOPOLOGY_RING) {
            VTSS_E("topology not ring, but equal cost routing on UPSID %d, chip_no=%d", 
                   upsid, vtss_state->chip_no);
        }
        JR_WRXB(ANA_AC, UPSID_STACK_LINK_EQUAL_COST_CFG, upsid, STACK_LINK_EQUAL_ENA,
                (rt->stack_port_a && rt->stack_port_b));
        /* Track active UPSes, i.e. reachable through stack port or ourself */
        if (mask || upsid == vtss_state->vstax_info.upsid[vtss_state->chip_no]) {
            upsid_active_mask |= VTSS_BIT(upsid);
            upsid_active_count++;
        }
    }
    if (chip_info->upsid_active != upsid_active_mask) {
        chip_info->upsid_active = upsid_active_mask;
        VTSS_RC(jr_cmef_topology_change());
    }

    // A topology change may affect the redirection of CPU queues */
    VTSS_RC(jr_cpu_queue_redirect_chip());

    /* CMEF Rates */
    cmef_int_min = cmef_min_interval[upsid_active_count-1][chip_info->rt_table.topology_type == VTSS_VSTAX_TOPOLOGY_RING ? 0 : 1];
    cmef_int_max = cmef_int_min * 20;
    if (cmef_int_min > 0xffff || cmef_int_max > 0xffff) {
        VTSS_E("Invalid CMEF interval(s): %u %u (upsid_active_count=%u)",
               cmef_int_min, cmef_int_max, upsid_active_count);
    }
    JR_WR(DSM, CM_CFG_CMEF_RATE_CFG,
          JR_PUT_FLD(DSM, CM_CFG_CMEF_RATE_CFG, CMEF_MIN_INTERVAL, cmef_int_min) |
          JR_PUT_FLD(DSM, CM_CFG_CMEF_RATE_CFG, CMEF_MAX_INTERVAL, cmef_int_max));

    return VTSS_RC_OK;
}

static vtss_rc jr_vstax_conf_set_chip(void)
{
    vtss_vstax_info_t      *vstax_info = &vtss_state->vstax_info;
    vtss_vstax_chip_info_t *chip_info = &vstax_info->chip_info[vtss_state->chip_no];
    vtss_vstax_upsid_t     upsid = vstax_info->upsid[vtss_state->chip_no];
    u32                    stack_mask = (chip_info->mask_a | chip_info->mask_b);
    u32                    port, stack_port, rvalue, rmask, max_ttl = 0, ttl = 0;
    BOOL                   enable, front_port;
    BOOL                   stacking = (stack_mask ? 1 : 0);
    BOOL                   cmef_enable = (vtss_state->vstax_conf.cmef_disable ? 0 : 1);
    vtss_port_no_t         port_no;

    /* UPSID setup */
    JR_WRF(ANA_CL_2, COMMON_UPSID_CFG, UPSID_NUM, upsid);
    JR_WRF(ANA_L2, COMMON_VSTAX_CTRL, OWN_UPSID, upsid);
    JR_WRF(ASM, SP_CFG_SP_UPSID_CFG, OWN_UPSID, upsid);
    JR_WRF(ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, OWN_UPSID, upsid);
    JR_WRB(ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, VSTAX2_MC_LLOOKUP_NON_FLOOD_ENA, 1); /* Use local lookup for non-flooded MC frames */
    JR_WRB(ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, VSTAX2_MC_LLOOKUP_ENA, 0);  /* BZ 6702: Do not use local lookup for flooded MC frames */
    JR_WRB(ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, VSTAX2_GLAG_ENA, 1); /* Enable GLAGs in VSTAX */
    JR_WRF(DSM, CM_CFG_CMEF_OWN_UPSID_CFG, CMEF_OWN_UPSID, upsid);

    for (port = JR_STACK_PORT_START; port < JR_STACK_PORT_END; port++) {
        /* Disable CMEF while fiddling with Rx and Tx buffers. */
        stack_port = port - JR_STACK_PORT_START;
        JR_WRXM(DSM, CM_CFG_CMEF_GEN_CFG, stack_port, 0, VTSS_F_DSM_CM_CFG_CMEF_GEN_CFG_CMEF_RELAY_TX_ENA | VTSS_F_DSM_CM_CFG_CMEF_GEN_CFG_CMEF_GEN_ENA);
        JR_WRXB(ASM, CM_CFG_CMEF_RX_CFG, stack_port, CMEF_RX_ENA, 0);
    }

    /* Per-stackable port setup */
    for (port = JR_STACK_PORT_START; port < JR_STACK_PORT_END; port++) {
        BOOL is_stack_a, is_stack_b, is_stack_port;
        is_stack_a    = (VTSS_BIT(port) & chip_info->mask_a) ? 1 : 0;
        is_stack_b    = (VTSS_BIT(port) & chip_info->mask_b) ? 1 : 0;
        is_stack_port = is_stack_a || is_stack_b;
        stack_port    = port - JR_STACK_PORT_START;

        rmask  = VTSS_F_ASM_SP_CFG_SP_RX_CFG_SP_FWD_CELL_BUS_ENA | VTSS_F_ASM_SP_CFG_SP_RX_CFG_SP_RX_SEL | VTSS_F_ASM_SP_CFG_SP_RX_CFG_SP_RX_ENA;
        /* Enable super-prio Rx on stack ports, except for interconnect port #1 and not on rev. A chips. */
        rvalue = ((vtss_state->chip_id.revision != 0) && is_stack_port && (vtss_state->chip_count != 2 || port != vtss_state->port_int_1)) ? VTSS_F_ASM_SP_CFG_SP_RX_CFG_SP_RX_ENA : 0;
        /* SP_RX_SEL == 1 on external stack ports only in 48-port solutions, and on stack port 0 (A) in 24-port solutions. */
        rvalue |= is_stack_a ? VTSS_F_ASM_SP_CFG_SP_RX_CFG_SP_RX_SEL : 0;
        /* Due to a chip-bug in Jaguar 1 rev. A, it's not
         * recommended to enable super-priority Rx. Instead, use
         * analyzer to get CPU frames to the CPU.
         * (VTSS_F_ASM_SP_CFG_SP_RX_CFG_SP_FWD_CELL_BUS_ENA)
         */
        rvalue |= (vtss_state->chip_id.revision == 0 && is_stack_port) ? VTSS_F_ASM_SP_CFG_SP_RX_CFG_SP_FWD_CELL_BUS_ENA : 0;
        JR_WRXM(ASM, SP_CFG_SP_RX_CFG, stack_port, rvalue, rmask);

        rmask = VTSS_F_DSM_SP_CFG_SP_TX_CFG_SP_TX_SEL | VTSS_F_DSM_SP_CFG_SP_TX_CFG_SP_TX_ENA;
        /* SP_TX_SEL = 1 on interconnect ports (stack_b) and 0 on external stack ports (stack_a) in 48-port solutions.
           SP_TX_SEL = 1 on stack_b port and 0 on stack_a port in 24-port solutions. */
        rvalue = is_stack_b ? VTSS_F_DSM_SP_CFG_SP_TX_CFG_SP_TX_SEL : 0;
        /* SP_TX_ENA = 1 on all stack ports (external as well as interconnects), except for interconnect #1,
           since it's used for CMEFs. */
        rvalue |= (is_stack_port && (vtss_state->chip_count != 2 || port != vtss_state->port_int_1)) ? VTSS_F_DSM_SP_CFG_SP_TX_CFG_SP_TX_ENA : 0;
        JR_WRXM(DSM, SP_CFG_SP_TX_CFG, stack_port, rvalue, rmask);

        /* VStaX2 awareness */
        JR_WRXB(ASM, CFG_ETH_CFG, port, ETH_VSTAX2_AWR_ENA, is_stack_port);

        /* TTL */
        if (is_stack_a) {
            JR_WRXF(REW, COMMON_VSTAX_PORT_GRP_CFG, 0, VSTAX_TTL, chip_info->port_conf[0].ttl);
        }
        if (is_stack_b) {
            JR_WRXF(REW, COMMON_VSTAX_PORT_GRP_CFG, 1, VSTAX_TTL, chip_info->port_conf[1].ttl);
        }

        /* Set REW:PHYSPORT[chip_port].VSTAX_STACK_GRP_SEL */
        rmask  = VTSS_F_REW_PHYSPORT_VSTAX_CTRL_VSTAX_HDR_ENA | VTSS_F_REW_PHYSPORT_VSTAX_CTRL_VSTAX_STACK_GRP_SEL;
        rvalue = is_stack_port ? VTSS_F_REW_PHYSPORT_VSTAX_CTRL_VSTAX_HDR_ENA : 0;
        if (is_stack_b) {
            rvalue |= VTSS_F_REW_PHYSPORT_VSTAX_CTRL_VSTAX_STACK_GRP_SEL; /* Stack Port "B" */
        }
        JR_WRXM(REW, PHYSPORT_VSTAX_CTRL, port, rvalue, rmask);

        /* ANA_CL:PORT[chip_port].PORT_STACKING_CTRL */
#ifdef VTSS_FEATURE_NPI
        if (is_stack_port || !vtss_state->npi_conf.enable || !VTSS_PORT_CHIP_SELECTED(vtss_state->npi_conf.port_no) || VTSS_CHIP_PORT(vtss_state->npi_conf.port_no) != port)
#endif
        {
            rmask = VTSS_F_ANA_CL_2_PORT_STACKING_CTRL_STACKING_AWARE_ENA | VTSS_F_ANA_CL_2_PORT_STACKING_CTRL_STACKING_NON_HEADER_DISCARD_ENA;
            JR_WRXM(ANA_CL_2, PORT_STACKING_CTRL, port, is_stack_port ? rmask : 0, rmask);
        }

        if (is_stack_port) { /* Bugzilla#6918 */
            int pcp, dei;

            /* Analyzer: Always use PCP and DEI for QoS and DP level classification on stack ports */
            JR_WRXB(ANA_CL_2, PORT_QOS_CFG, port, UPRIO_QOS_ENA,      1);
            JR_WRXB(ANA_CL_2, PORT_QOS_CFG, port, UPRIO_DP_ENA,       1);
            JR_WRXB(ANA_CL_2, PORT_QOS_CFG, port, ITAG_UPRIO_QOS_ENA, 1);
            JR_WRXB(ANA_CL_2, PORT_QOS_CFG, port, ITAG_UPRIO_DP_ENA,  1);

            /* Map for (PCP and DEI) to (QoS class and DP level) */
            for (pcp = VTSS_PCP_START; pcp < VTSS_PCP_END; pcp++) {
                for (dei = VTSS_DEI_START; dei < VTSS_DEI_END; dei++) {
                    JR_WRXYF(ANA_CL_2, PORT_UPRIO_MAP_CFG, port, (8*dei + pcp), UPRIO_CFI_TRANSLATE_QOS_VAL, vtss_cmn_pcp2qos(pcp));
                    JR_WRXYF(ANA_CL_2, PORT_DP_CONFIG,     port, (8*dei + pcp), UPRIO_CFI_DP_VAL,            dei);
                }
            }
        }
    }
    VTSS_RC(jr_cmef_topology_change());

    if (cmef_enable) {
        JR_WR(DSM, CM_CFG_CMM_MASK_CFG, ~stack_mask);
    } else {
        JR_WR(DSM, CM_CFG_CMM_MASK_CFG, 0);
        /* We also must clear status when disabling (instance 0) */
        JR_WRX(DSM, CM_STATUS_CURRENT_CM_STATUS, 0, 0);
    }

    /* No ISDX in VStaX header */
    JR_WRB(ANA_CL_2, COMMON_IFH_CFG, VSTAX_ISDX_ENA, !stacking);
    JR_WRB(ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, VSTAX2_MISC_ISDX_ENA, !stacking);

    JR_WRB(ANA_CL_2, COMMON_AGGR_CFG, AGGR_USE_VSTAX_AC_ENA, stacking);

    /* Configuration of learn-all is moved to jr_lrn_all_set_chip() because
     * it's now a S/W-based learn-all frame forwarding due to a chip-bug (bugzilla#7730 and 7737).
     * However, if the stack masks change, we need to update its configuration.
     */
    VTSS_RC(jr_lrn_all_set_chip());
     
    /* Enable CM - globally */
    JR_WRB(ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, VSTAX2_USE_CM_ENA, stacking);

    /* Enable CM on non-stack ports and update MAX Frame length */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!VTSS_PORT_CHIP_SELECTED(port_no)) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        front_port = jr_is_frontport(port_no);
        VTSS_RC(jr_update_lport(port_no, port));
        VTSS_RC(jr_vstax_update_ingr_shaper(port, vtss_state->port_conf[port_no].speed, front_port));
        VTSS_RC(jr_setup_mtu(port, vtss_state->port_conf[port_no].max_frame_length, front_port));
    }

    JR_WRB(DSM, CM_CFG_CMM_TO_ARB_CFG, CMM_TO_ARB_DELAY_ENA, stacking);
    JR_WRF(DSM, CM_CFG_CMM_TO_ARB_CFG, CMM_XTRCT_ENA, stacking ? 0x3 : 0);

    /* Due to a chip-bug in Jaguar 1 rev. A, enabling super-priority Rx is not recommended.
     * Only enable SP Rx in stacking solutions, on non-rev. A chips, and on primary chip.
     * In fact, it's impossible to enable stacking in a dual-chip solution if primary chip
     * is not a non-Rev. A, because SP frames arriving on the internal ports on the primary
     * chip will have to be handled by the analyzer and due to policing, all analyzed frames
     * going to the CPU frames will be killed to avoid duplicates.
     */
    JR_WRF(DSM, SP_CFG_SP_XTRCT_CFG, SP_XTRCT_ENA, (stacking && vtss_state->chip_id.revision != 0 && vtss_state->chip_no == 0) ? 0x3 : 0);

    /* Max TTL */
    if (chip_info->mask_a) {
        ttl = chip_info->port_conf[0].ttl;
    }
    if (chip_info->mask_b) {
        max_ttl = chip_info->port_conf[1].ttl;
    }
    if (ttl > max_ttl) {
        max_ttl = ttl;
    }

    /* Max TTL for equal path routing */
    JR_WRF(ANA_AC, PS_COMMON_COMMON_EQUAL_STACK_LINK_TTL_CFG, VSTAX2_EQUAL_STACK_LINK_TTL_VAL, max_ttl);

    /* Keep TTL on secondary chip if stacking */
    JR_WRB(ASM, SP_CFG_SP_KEEP_TTL_CFG, SP_KEEP_TTL_ENA, (stacking && vtss_state->chip_no == 1) ? 1 : 0);
    JR_WRB(DSM, SP_CFG_SP_KEEP_TTL_CFG, SP_KEEP_TTL_ENA, (stacking && vtss_state->chip_no == 1) ? 1 : 0);

    /* Stack port mask registers */
    JR_WR(ANA_AC, PS_COMMON_STACK_A_CFG, chip_info->mask_a);
    JR_WR(ANA_AC, PS_COMMON_STACK_CFG, stack_mask);
    JR_WR(ANA_AC, PS_COMMON_PHYS_SRC_AGGR_CFG, stack_mask);

    /* REW:COMMON:VSTAX_PORT_GRP_CFG[0-1].VSTAX_LRN_ALL_HP_ENA */
    JR_WRXB(REW, COMMON_VSTAX_PORT_GRP_CFG, 0, VSTAX_LRN_ALL_HP_ENA, chip_info->mask_a != 0);
    JR_WRXB(REW, COMMON_VSTAX_PORT_GRP_CFG, 1, VSTAX_LRN_ALL_HP_ENA, chip_info->mask_b != 0);

    /* REW:COMMON:VSTAX_PORT_GRP_CFG[0-1].VSTAX_MODE */
    JR_WRXB(REW, COMMON_VSTAX_PORT_GRP_CFG, 0, VSTAX_MODE, chip_info->mask_a != 0);
    JR_WRXB(REW, COMMON_VSTAX_PORT_GRP_CFG, 1, VSTAX_MODE, chip_info->mask_b != 0);

    /* Setup mirroring */
    VTSS_RC(jr_mirror_conf_set_chip());

    /* Update VStaX topology */
    VTSS_RC(jr_vstax_topology_set_chip());

    for (port = JR_STACK_PORT_START; cmef_enable && port < JR_STACK_PORT_END; port++) {
        /* Enable CMEF on external stack ports and (in two-chip solutions) on internal port #0 */
        stack_port = port - JR_STACK_PORT_START;
        enable = ((VTSS_BIT(port) & stack_mask) != 0 && (vtss_state->chip_count != 2 || port != vtss_state->port_int_1));

        if (enable) {
            BOOL is_stack_a, is_stack_b;
            is_stack_a    = (VTSS_BIT(port) & chip_info->mask_a) ? 1 : 0;
            is_stack_b    = (VTSS_BIT(port) & chip_info->mask_b) ? 1 : 0;
            ttl = (is_stack_a ? chip_info->port_conf[0].ttl : is_stack_b ? chip_info->port_conf[1].ttl : 0);
            JR_WRXF(DSM, CM_CFG_CMEF_GEN_CFG, stack_port, CMEF_TTL, ttl);

            rmask = VTSS_F_DSM_CM_CFG_CMEF_GEN_CFG_CMEF_RELAY_TX_ENA | VTSS_F_DSM_CM_CFG_CMEF_GEN_CFG_CMEF_GEN_ENA;
            JR_WRXM(DSM, CM_CFG_CMEF_GEN_CFG, stack_port, rmask, rmask);
            JR_WRXB(ASM, CM_CFG_CMEF_RX_CFG, stack_port, CMEF_RX_ENA, 1);
        }
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_vstax_conf_set(void)
{
    vtss_chip_no_t         chip_no;
    u32                    mask;
    vtss_vstax_chip_info_t *chip_info;
    vtss_port_no_t         port_no;
    int                    i;

    /* Update VStaX for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        chip_info = &vtss_state->vstax_info.chip_info[chip_no];

        /* Update mask_a and mask_b */
        chip_info->mask_a = 0;
        chip_info->mask_b = (vtss_state->chip_count == 2 ? vtss_state->mask_int_ports : 0);
        for (i = 0; i < 2; i++) {
            port_no = (i == 0 ? vtss_state->vstax_conf.port_0 : vtss_state->vstax_conf.port_1);
            if (port_no != VTSS_PORT_NO_NONE && VTSS_PORT_CHIP_SELECTED(port_no)) {
                mask = VTSS_BIT(VTSS_CHIP_PORT(port_no));
                /* In two-chip solutions, mask_a are the external stack ports, and mask_b are the interconnects */
                if (i == 0 || vtss_state->chip_count == 2) {
                    /* Stack port A */
                    chip_info->mask_a |= mask;
                } else {
                    /* Stack port B */
                    chip_info->mask_b |= mask;
                }
            }
        }
        VTSS_RC(jr_vstax_conf_set_chip());
    }

    /* Update VStaX for all stackable ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (VTSS_PORT_IS_STACKABLE(VTSS_CHIP_PORT(port_no))) {
            VTSS_SELECT_CHIP_PORT_NO(port_no);

            /* Enable/disable VCAP lookups */
            VTSS_RC(jr_vcap_lookup_set(port_no));

            /* Update VLAN port configuration */
            VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
        }
    }

    /* Update IS2 rules for IGMP */
    VTSS_RC(jr_igmp_cpu_copy_set());

    return VTSS_RC_OK;
}

static vtss_rc jr_vstax_master_upsid_set(void)
{
    /* Setup config for all devices */
    VTSS_RC(jr_conf_chips(jr_cpu_queue_redirect_chip));

    return VTSS_RC_OK;
}

static vtss_rc jr_vstax_topology_set(void)
{
    return jr_vstax_topology_set_chip();
}

static vtss_rc jr_vstax_port_conf_set(const BOOL stack_port_a)
{
    /* Update VStaX configuration */
    return jr_vstax_conf_set_chip();
}

static vtss_rc jr_vstax_header2frame(const vtss_state_t            *const state,
                                     const vtss_port_no_t          port_no,
                                     const vtss_vstax_tx_header_t  *const vstax,
                                     u8                            *const frame)
{
    u32 chip_port = (vstax->port_no == VTSS_PORT_NO_NONE ? vstax->chip_port : VTSS_CHIP_PORT_FROM_STATE(state, vstax->port_no));
    u32 val;
    u32 vstax_hi = 0;
    u64 vstax_lo = 0;

    /* EtherType */
    vstax_hi |= VTSS_ENCODE_BITFIELD(VTSS_ETYPE_VTSS, 16, 16);

    /* Valid */
    vstax_hi |= VTSS_ENCODE_BITFIELD(1,               15,  1);

    /* Ingress Drop Mode == 1 => No head-of-line blocking. Not used on super-prio injection */
    vstax_lo |= VTSS_ENCODE_BITFIELD64(1, 55, 1);

    /* DP */
    if (vstax->dp) {
        vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->dp, 60, 2);
    }

    /* QoS */
    if (vstax->prio == VTSS_PRIO_SUPER) {
        vstax_lo |= VTSS_ENCODE_BITFIELD64(TRUE, 59, 1);
    } else {
        vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->prio, 56, 3);
    }

    /* TTL */
    if (vstax->ttl == VTSS_VSTAX_TTL_PORT) {
        /* In two-chip solutions that send to the secondary's stack port, we actually pick
         * the TTL for the interconnect port, because the FDMA transmits indirectly through
         * the interconnect port. Application must set keep_ttl to 0 in that case.
         */
        val = state->vstax_info.chip_info[0].port_conf[port_no == state->vstax_conf.port_0 ? 0 : 1].ttl;
    } else {
        val = vstax->ttl;
    }
    vstax_lo |= VTSS_ENCODE_BITFIELD64(val, 48, 5);

    /* LRN skip */
    vstax_lo |= VTSS_ENCODE_BITFIELD64(TRUE, 47, 1);

    /* Fwd mode and associated fields*/
    vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->fwd_mode, 44, 3);
    switch(vstax->fwd_mode) {
    case VTSS_VSTAX_FWD_MODE_LOOKUP:
        break;
    case VTSS_VSTAX_FWD_MODE_UPSID_PORT:
        /* dst_port_type=0 */
        vstax_lo |= VTSS_ENCODE_BITFIELD64((u32)vstax->upsid - VTSS_VSTAX_UPSID_START, 37, 5);
        vstax_lo |= VTSS_ENCODE_BITFIELD64(chip_port, 32, 5);
        break;
    case VTSS_VSTAX_FWD_MODE_CPU_UPSID:
        vstax_lo |= VTSS_ENCODE_BITFIELD64((u32)vstax->upsid - VTSS_VSTAX_UPSID_START, 37, 5);
        vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->queue_no - VTSS_PACKET_RX_QUEUE_START, 32, 4);
        break;
    case VTSS_VSTAX_FWD_MODE_CPU_ALL:
        vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->keep_ttl, 41, 1);
        vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->queue_no - VTSS_PACKET_RX_QUEUE_START, 32, 4);
        break;
    default:
        VTSS_E("Illegal fwd mode: %d", vstax->fwd_mode);
    }

    /* uprio */
    vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->tci.tagprio, 29, 3);

    /* cfi */
    vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->tci.cfi, 28, 1);

    /* VID */
    vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->tci.vid, 16, 12);

    /* Source */
    if (vstax->glag_no != VTSS_GLAG_NO_NONE) {
        /* src_addr_mode (bit #10) == 1, a.k.a. src_glag */
        vstax_lo |= VTSS_ENCODE_BITFIELD64(TRUE, 10, 1);
        vstax_lo |= VTSS_ENCODE_BITFIELD64(vstax->glag_no, 0, 5);
    } else {
        /* src_addr_mode (bit #10) == 0, a.k.a. src_ind_port */
        /* Pick the src_port_type (bit #11) == 1, a.k.a. port_type_intpn. */
        vstax_lo |= VTSS_ENCODE_BITFIELD64(TRUE, 11, 1);
        vstax_lo |= VTSS_ENCODE_BITFIELD64((u8)state->vstax_info.upsid[0], 5, 5); /* Source chip_no == 0, i.e. ourselves. */
        vstax_lo |= VTSS_ENCODE_BITFIELD64(0xf, 0, 5); /* intpn_dlookup */
    }

    frame[ 0] = vstax_hi >> 24;
    frame[ 1] = vstax_hi >> 16;
    frame[ 2] = vstax_hi >>  8;
    frame[ 3] = vstax_hi >>  0;
    frame[ 4] = vstax_lo >> 56;
    frame[ 5] = vstax_lo >> 48;
    frame[ 6] = vstax_lo >> 40;
    frame[ 7] = vstax_lo >> 32;
    frame[ 8] = vstax_lo >> 24;
    frame[ 9] = vstax_lo >> 16;
    frame[10] = vstax_lo >>  8;
    frame[11] = vstax_lo >>  0;

    return VTSS_RC_OK;
}

static vtss_rc jr_vstax_frame2header(const vtss_state_t      *const state,
                                     const u8                *const frame,
                                     vtss_vstax_rx_header_t  *const vstax)
{
    u32 vstax_hi = 0;
    const u8 *ifh = frame;

    /* 4 bytes to vstax_hi */
    vstax_hi <<= 8; vstax_hi += *ifh++;
    vstax_hi <<= 8; vstax_hi += *ifh++;
    vstax_hi <<= 8; vstax_hi += *ifh++;
    vstax_hi <<= 8; vstax_hi += *ifh++;

    memset(vstax, 0, sizeof(*vstax));

    /* Valid IFH */
    vstax->valid =
        (VTSS_EXTRACT_BITFIELD(vstax_hi, 16, 16) == VTSS_ETYPE_VTSS) &&
        VTSS_EXTRACT_BITFIELD(vstax_hi, 15, 1);

    /* Decode rest if OK */
    if(vstax->valid) {
        u64 vstax_lo = 0;

        vstax->isdx = VTSS_EXTRACT_BITFIELD(vstax_hi, 0, 12);

        /* 8 bytes to vstax_lo */
        vstax_lo <<= 8; vstax_lo += *ifh++;
        vstax_lo <<= 8; vstax_lo += *ifh++;
        vstax_lo <<= 8; vstax_lo += *ifh++;
        vstax_lo <<= 8; vstax_lo += *ifh++;
        vstax_lo <<= 8; vstax_lo += *ifh++;
        vstax_lo <<= 8; vstax_lo += *ifh++;
        vstax_lo <<= 8; vstax_lo += *ifh++;
        vstax_lo <<= 8; vstax_lo += *ifh++;

        vstax->sp = VTSS_EXTRACT_BITFIELD64(vstax_lo, 59, 1);

        if(VTSS_EXTRACT_BITFIELD64(vstax_lo, 10, 1)) {
            /* GLAG */
            vstax->port_no = VTSS_PORT_NO_NONE;
            vstax->glag_no = VTSS_EXTRACT_BITFIELD64(vstax_lo, 0, 5);
        } else {
            /* Port */
            vstax->port_no = jr_vtss_pgid(state, VTSS_EXTRACT_BITFIELD64(vstax_lo, 0,  5));
            vstax->upsid   = VTSS_EXTRACT_BITFIELD64(vstax_lo, 5,  5);
            vstax->glag_no = VTSS_GLAG_NO_NONE;
        }
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_VSTAX_V2 */

static vtss_rc jr_rx_hdr_decode(const vtss_state_t          *const state,
                                const vtss_packet_rx_meta_t *const meta,
                                      vtss_packet_rx_info_t *const info)
{
    u16                 vstax_hi;
    u64                 vstax_lo, fwd;
    BOOL                sflow_marked;
    vtss_phys_port_no_t chip_port;
    vtss_etype_t        etype;
    vtss_trace_group_t  trc_grp = meta->no_wait ? VTSS_TRACE_GROUP_FDMA_IRQ : VTSS_TRACE_GROUP_PACKET;

    VTSS_DG(trc_grp, "IFH (24 bytes) + bit of packet:");
    VTSS_DG_HEX(trc_grp, &meta->bin_hdr[0], 64);

    vstax_hi =                                                                   ((u16)meta->bin_hdr[ 8] <<  8) | ((u16)meta->bin_hdr[ 9] <<  0);
    vstax_lo = ((u64)meta->bin_hdr[10] << 56) | ((u64)meta->bin_hdr[11] << 48) | ((u64)meta->bin_hdr[12] << 40) | ((u64)meta->bin_hdr[13] << 32) |
               ((u64)meta->bin_hdr[14] << 24) | ((u64)meta->bin_hdr[15] << 16) | ((u64)meta->bin_hdr[16] <<  8) | ((u64)meta->bin_hdr[17] <<  0);
    fwd      =                                                                   ((u64)meta->bin_hdr[18] << 40) | ((u64)meta->bin_hdr[19] << 32) |
               ((u64)meta->bin_hdr[20] << 24) | ((u64)meta->bin_hdr[21] << 16) | ((u64)meta->bin_hdr[22] <<  8) | ((u64)meta->bin_hdr[23] <<  0);

    // The VStaX header's MSbit must be 1.
    if (VTSS_EXTRACT_BITFIELD(vstax_hi, 15, 1) != 1) {
        VTSS_EG(trc_grp, "Invalid Rx header signature");
        return VTSS_RC_ERROR;
    }

    memset(info, 0, sizeof(*info));

    info->sw_tstamp = meta->sw_tstamp;
    info->length    = meta->length;

    chip_port = VTSS_EXTRACT_BITFIELD64(fwd, 29, 6);
    info->port_no = vtss_cmn_chip_to_logical_port(state, meta->chip_no, chip_port);

//     VTSS_IG(trc_grp, "Received on xtr_qu = %u, chip_no = %d, chip_port = %u, port_no = %u", meta->xtr_qu, meta->chip_no, chip_port, info->port_no);

#if defined(VTSS_FEATURE_AGGR_GLAG)
    info->glag_no = state->port_glag_no[info->port_no];
#else
    info->glag_no = VTSS_GLAG_NO_NONE;
#endif

    // FIXME: Must support internal ports.

#ifdef VTSS_FEATURE_VSTAX
    if (VTSS_EXTRACT_BITFIELD64(fwd, 45, 1)) {
        // Received with VStaX header. Decode it.
        u8 vstax_hdr_bin[VTSS_VSTAX_HDR_SIZE];

        vstax_hdr_bin[0] = (VTSS_ETYPE_VTSS >> 8) & 0xFF;
        vstax_hdr_bin[1] = (VTSS_ETYPE_VTSS >> 0) & 0xFF;
        memcpy(&vstax_hdr_bin[2], &meta->bin_hdr[8], VTSS_VSTAX_HDR_SIZE - 2);
        (void)jr_vstax_frame2header(state, vstax_hdr_bin, &info->vstax);
        info->glag_no = info->vstax.glag_no;
    }
#endif

    sflow_marked = VTSS_EXTRACT_BITFIELD64(fwd, 40, 1);
    if (sflow_marked) {
        // This is only reliable if ANA_AC:PS_COMMON:PS_COMMON_CFG.SFLOW_SMPL_ID_IN_STAMP_ENA is set to 1.
        u32 sflow_id        = (meta->fcs >> 26) & 0x3F; // Indicates physical port number.
        info->sflow_type    = chip_port == sflow_id ? VTSS_SFLOW_TYPE_RX : VTSS_SFLOW_TYPE_TX;
        info->sflow_port_no = vtss_cmn_chip_to_logical_port(state, meta->chip_no, sflow_id);
    } else {
        // Get the timestamp from the FCS.
        info->hw_tstamp         = meta->fcs;
        info->hw_tstamp_decoded = TRUE;
    }

    if (VTSS_EXTRACT_BITFIELD64(fwd, 27, 1) && meta->xtr_qu < 8) {
        // If classification_result[9] == 1, classification_result[7:0] == xtr_qu_mask for 8 non-super-priority queues.
        info->xtr_qu_mask = VTSS_EXTRACT_BITFIELD64(fwd, 18, 8);
    } else {
        // If classification_result[9] == 0, classification_result[8:0] == acl_idx for non-default VCAP IS2 rule.
        // If the frame is received on a super-priority queue, the FDMA has created an artificial IFH, and therefore
        // we have to construct the xtr_qu_mask from that.
        info->xtr_qu_mask = 1 << meta->xtr_qu;

        if (meta->xtr_qu < 8) {
            // Hit a non-default VCAP IS2 rule.
            info->acl_hit = TRUE;
            info->acl_idx = VTSS_EXTRACT_BITFIELD64(fwd, 18, 9);

            // We may also set the sflow queue in the queue mask.
            if (sflow_marked && state->rx_conf.map.sflow_queue != VTSS_PACKET_RX_QUEUE_NONE) {
                info->xtr_qu_mask |= (1 << state->rx_conf.map.sflow_queue);
            }
        }
    }

    info->tag.pcp = VTSS_EXTRACT_BITFIELD64(vstax_lo, 29,  3);
    info->tag.dei = VTSS_EXTRACT_BITFIELD64(vstax_lo, 28,  1);
    info->tag.vid = VTSS_EXTRACT_BITFIELD64(vstax_lo, 16, 12);
    info->cos     = VTSS_EXTRACT_BITFIELD64(vstax_lo, 56,  3);

#if defined(VTSS_FEATURE_VSTAX)
    // If it's received on a stack port, we gotta look into the VStaX header to find a tag type on the original port.
    // Unfortunately, a possible tag was stripped on egress of the stack port of the originating switch, so that we
    // can't determine whether it originally arrived on an S- or an S-custom port. We classify it as an S-port in that
    // case. Also, we can't set any of the info->hints flags, because we don't have info about the original
    // ingress port's setup.
    // sFlow frames received through interconnects on secondary device will have info->port_no == VTSS_PORT_NO_NONE,
    // which is not supported by vtss_cmn_packet_hints_update(), so we captuer them below.
    if ((state->chip_count == 2 && info->port_no == VTSS_PORT_NO_NONE) || info->port_no == state->vstax_conf.port_0 || info->port_no == state->vstax_conf.port_1) {
        if (VTSS_EXTRACT_BITFIELD64(vstax_lo, 15, 1)) {
            // Frame was originally tagged. Figure out whether it was a C or an S(-custom) tag.
            info->tag_type = VTSS_EXTRACT_BITFIELD64(vstax_lo, 14, 1) ? VTSS_TAG_TYPE_S_TAGGED : VTSS_TAG_TYPE_C_TAGGED;
            info->tag.tpid = info->tag_type == VTSS_TAG_TYPE_S_TAGGED ? VTSS_ETYPE_TAG_S : VTSS_ETYPE_TAG_C; // Application shouldn't rely on this.
        }
    } else
#endif
    {
        etype = (meta->bin_hdr[24 /* xtr hdr */ + 2 * 6 /* 2 MAC addresses*/] << 8) | (meta->bin_hdr[24 + 2 * 6 + 1]);
        VTSS_RC(vtss_cmn_packet_hints_update(state, trc_grp, etype, info));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_tx_hdr_encode_internal(const vtss_state_t          *const state,
                                         const vtss_packet_tx_info_t *const info,
                                               u8                    *const bin_hdr,
                                               u32                   *const bin_hdr_len,
                                               vtss_packet_tx_grp_t  *const inj_grp)
{
    u64  dst, vstax_lo, fwd;
    u16  vstax_hi;
    BOOL contains_stack_hdr;

    if (bin_hdr == NULL) {
        // Caller wants us to return the number of bytes required to fill
        // in #bin_hdr. We need 24 bytes for the IFH.
        *bin_hdr_len = 24;
        return VTSS_RC_OK;
    } else if (*bin_hdr_len < 24) {
        return VTSS_RC_ERROR;
    }

    *bin_hdr_len = 24;

#if defined(VTSS_FEATURE_VSTAX)
   contains_stack_hdr = info->tx_vstax_hdr != VTSS_PACKET_TX_VSTAX_NONE;
#else
   contains_stack_hdr = FALSE;
#endif

    dst      = 0;
    vstax_hi = 0;
    vstax_lo = VTSS_ENCODE_BITFIELD64(1, 55, 1); // ingr_drop_mode = 1 => don't make head-of-line blocking
    fwd      = VTSS_ENCODE_BITFIELD64(1, 44, 1); // Enforce update of FCS.
    *inj_grp = 0; // Default to injecting non-super prio.

    if (!info->switch_frm) {
        // Most of the stuff that needs to be filled in is when we're not switching the frame.
        // First figure out which ports the caller wants to inject to.
        u64            chip_port_mask;
        vtss_chip_no_t chip_no;
        vtss_port_no_t stack_port_no;
        u32            port_cnt;
        vtss_port_no_t port_no; /* Only valid if port_cnt == 1 */

        VTSS_RC(vtss_cmn_logical_to_chip_port_mask(state, info->dst_port_mask, &chip_port_mask, &chip_no, &stack_port_no, &port_cnt, &port_no));

        if (contains_stack_hdr) {
#if defined(VTSS_FEATURE_VSTAX)
            // Stack header is already filled in by the user. Make sure the selected port is a stack port.
            if (stack_port_no == VTSS_PORT_NO_NONE) {
                VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Injecting with stack header to non-stack port");
                return VTSS_RC_ERROR;
            }

            if (info->tx_vstax_hdr == VTSS_PACKET_TX_VSTAX_SYM) {
                // The following function also inserts the EtherType, hence the "-2" from the correct VStaX header position within the IFH.
                VTSS_RC(jr_vstax_header2frame(state, stack_port_no, &info->vstax.sym, &bin_hdr[8 - 2]));
            } else {
                // Copy the user-defined stack header to the binary header. Skip EtherType.
                memcpy(bin_hdr + 8, info->vstax.bin + 2, VTSS_VSTAX_HDR_SIZE - 2);
            }

            fwd |= VTSS_ENCODE_BITFIELD64(1, 45, 1); // FWD.VSTAX_AVAIL = 1

            if (info->cos >= 8) {
                // Super priority injection. Modify injection group to either 2 (stack left) or 3 (stack right).
                // When doing SP injection stack port injection, most of the non-stack-hdr-part of the IFH is ignored.
                if (state->chip_count == 2) {
                    // In 48-port stackable solutions, transmit on injection group 3, which hits the interconnect
                    // port, if the frame must go out on the external stack port on the secondary device.
                    *inj_grp = state->port_map[stack_port_no].chip_no == 1 ? 3 : 2;
                } else {
                    // In 24-port stackable solutions, transmit on injection group 3
                    // if the stack port number matches stack right.
                    *inj_grp = stack_port_no == state->vstax_conf.port_1 ? 3 : 2;
                }
            } else {
                // Non-super-priority injection with a stack header. Leave the injection group at 0.
                // If the stack port is on the secondary device, and we're running on the on-chip CPU,
                // we reach the stack port by injecting to the interconnect port.
                // An external CPU may or may not want to inject through the interconnect port.
                // What it does depends on whether it has one or two NPI ports, or - if it uses the register interface -
                // has one to each of the two devices, or only one to the primary.
                fwd |= VTSS_ENCODE_BITFIELD64(3, 16, 2); // INJECT_MULTICAST (with only one port).
#if VTSS_OPT_VCORE_III
                if (chip_no == 1) {
                    // Injecting to stack port on secondary device.
                    // Do this through the interconnect port on the primary device.
                    chip_port_mask = VTSS_BIT64(state->port_int_0);
                }
#endif
                dst |= VTSS_ENCODE_BITFIELD64(chip_port_mask,         0, 32);
                fwd |= VTSS_ENCODE_BITFIELD64(info->latch_timestamp, 46,  2); // FWD.CAPTURE_TSTAMP
                fwd |= VTSS_ENCODE_BITFIELD64(1,                     28,  1); // FWD.DO_NOT_REW
            }
#endif
        } else {
            // A stack header is not already prefilled by the user.
            if (stack_port_no != VTSS_PORT_NO_NONE) {
                // When injecting to a stack port, the user must already have filled in the VStaX header.
                VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Injecting without a stack header to a stack port");
                return VTSS_RC_ERROR;
            }

            if (info->cos >= 8) {
                // User wants to inject on super-priority to a front-port.
                // As stated in the API, this only works on the primary device (if running on on-chip CPU) and
                // with a single port at a time.
                if (port_cnt != 1) {
                    VTSS_EG(VTSS_TRACE_GROUP_PACKET, "SP FP inj. only supported one port at a time");
                    return VTSS_RC_ERROR;
                }

#if VTSS_OPT_VCORE_III
                // If using the on-chip CPU, we cannot inject super-priority to a front port
                // on the secondary device, because we'd have to go through one of the interconnect ports, after
                // which SP FP injection is impossible.
                if (chip_no != 0) {
                    VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Can't do SP FP inj. on sec. device");
                    return VTSS_RC_ERROR;
                }
#endif

                fwd |= VTSS_ENCODE_BITFIELD64(2, 16, 2); // INJECT_UNICAST
                dst |= VTSS_ENCODE_BITFIELD64((u32)state->port_map[port_no].chip_port, 0, 8);
                *inj_grp = 4; // Inject on pront port, super-prio group
            } else {
                // Injecting non-super-priority to one or more front ports on either the primary or the secondary device.
                // If injecting to front ports on the secondary device, only one port is supported at a time (if running on the on-chip
                // CPU; an external may have direct access to the secondary device). In this case, it's up to e.g. the FDMA driver to inject multiple times.
#if defined(VTSS_FEATURE_VSTAX) && VTSS_OPT_VCORE_III
                if (chip_no == 1) {
                    vtss_vstax_tx_header_t vs_hdr;

                    if (port_cnt != 1) {
                        VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Can't inj. to multiple ports on the secondary device (mask = 0x%llx)", info->dst_port_mask);
                        return VTSS_RC_ERROR;
                    }

                    // Otherwise, we synthesize a VStaX header and send the frame to the interconnect port.
                    memset(&vs_hdr, 0, sizeof(vs_hdr));
                    vs_hdr.fwd_mode  = VTSS_VSTAX_FWD_MODE_UPSID_PORT;
                    vs_hdr.ttl       = 1;
                    vs_hdr.prio      = info->cos >= 8 ? 7 : info->cos;
                    vs_hdr.upsid     = state->vstax_info.upsid[chip_no];
                    vs_hdr.port_no   = port_no;
                    vs_hdr.glag_no   = VTSS_GLAG_NO_NONE; // Disable GLAG source filtering.
                    VTSS_RC(jr_vstax_header2frame(state, VTSS_PORT_NO_NONE, &vs_hdr, &bin_hdr[8 - 2])); // That function also inserts the EtherType, hence the "-2" from the correct VStaX header position within the IFH.
                    contains_stack_hdr = TRUE; // Prevent overwriting further down.
                    chip_port_mask = VTSS_BIT64(state->port_int_0); // Transmit on interconnect port.
                    // Leave injection group as 0.
                }
#endif
                fwd |= VTSS_ENCODE_BITFIELD64(3, 16, 2); // INJECT_MULTICAST
                dst |= VTSS_ENCODE_BITFIELD64((u32)chip_port_mask, 0, 32);
            }

            fwd |= VTSS_ENCODE_BITFIELD64(info->latch_timestamp, 46,  2); // FWD.CAPTURE_TSTAMP
            fwd |= VTSS_ENCODE_BITFIELD64(1,                     28,  1); // FWD.DO_NOT_REW
            fwd |= VTSS_ENCODE_BITFIELD64(1,                     45,  1); // FWD.VSTAX_AVAIL = 1

            // We can get here even with a stack header, in the case the user doesn't inject into
            // a super-priority group (i.e. inj_grp != 2 && inj_grp != 3).
            if (!contains_stack_hdr) {
                // We don't have a VLAN tag to get the switch determine the QoS class from,
                // so we must use the VStaX header (props->port_mask must not include stack ports)
                // to convey the information (this is not possible when sending switched).
                vstax_hi  = VTSS_ENCODE_BITFIELD  (1,         15, 1); // MSBit must be 1
                vstax_lo |= VTSS_ENCODE_BITFIELD64(info->cos >= 8 ? 7 : info->cos, 56, 3); // qos_class/iprio (internal priority)

            }
        }
    }

    bin_hdr[0] = dst >> 56;
    bin_hdr[1] = dst >> 48;
    bin_hdr[2] = dst >> 40;
    bin_hdr[3] = dst >> 32;
    bin_hdr[4] = dst >> 24;
    bin_hdr[5] = dst >> 16;
    bin_hdr[6] = dst >>  8;
    bin_hdr[7] = dst >>  0;
    if (!contains_stack_hdr) {
        bin_hdr[ 8] = vstax_hi >>  8;
        bin_hdr[ 9] = vstax_hi >>  0;
        bin_hdr[10] = vstax_lo >> 56;
        bin_hdr[11] = vstax_lo >> 48;
        bin_hdr[12] = vstax_lo >> 40;
        bin_hdr[13] = vstax_lo >> 32;
        bin_hdr[14] = vstax_lo >> 24;
        bin_hdr[15] = vstax_lo >> 16;
        bin_hdr[16] = vstax_lo >>  8;
        bin_hdr[17] = vstax_lo >>  0;
    }
    bin_hdr[18] = fwd >> 40;
    bin_hdr[19] = fwd >> 32;
    bin_hdr[20] = fwd >> 24;
    bin_hdr[21] = fwd >> 16;
    bin_hdr[22] = fwd >>  8;
    bin_hdr[23] = fwd >>  0;

    if (info->cos >= 8) {
        // When injecting stack-frames through the super-priority queues,
        // or frames through the front port super-priority queue,
        // the IFH checksum must be correct, because the check in the
        // chip (DSM) cannot be disabled, as it can for non-SP-transmitted
        // frames (through ASM::CPU_CFG.CPU_CHK_IFH_CHKSUM_ENA).
        // This is computed over the entire IFH as BIP-8 (BIP8).
        u8 chksum = 0;
        u8 *p = &bin_hdr[0];
        int i;
        for (i = 0; i < 24; i++) {
            chksum ^= *(p++);
        }
        bin_hdr[23] = chksum;
    }

    VTSS_IG(VTSS_TRACE_GROUP_PACKET, "IFH:");
    VTSS_IG_HEX(VTSS_TRACE_GROUP_PACKET, &bin_hdr[0], *bin_hdr_len);

    return VTSS_RC_OK;
}

static vtss_rc jr_tx_hdr_encode(      vtss_state_t          *const state,
                                const vtss_packet_tx_info_t *const info,
                                      u8                    *const bin_hdr,
                                      u32                   *const bin_hdr_len)
{
    vtss_packet_tx_grp_t inj_grp;
    return jr_tx_hdr_encode_internal(state, info, bin_hdr, bin_hdr_len, &inj_grp);
}

/* ================================================================= *
 *  TCAM control
 * ================================================================= */

enum vtss_tcam_cmd {
    VTSS_TCAM_CMD_WRITE       = 0, /* Copy from Cache to TCAM */
    VTSS_TCAM_CMD_READ        = 1, /* Copy from TCAM to Cache */
    VTSS_TCAM_CMD_MOVE_UP     = 2, /* Move <count> up */
    VTSS_TCAM_CMD_MOVE_DOWN   = 3, /* Move <count> down */
    VTSS_TCAM_CMD_INITIALIZE  = 4, /* Init entries and normal actions. Reset default actions */
};

enum vtss_tcam_bank {
    VTSS_TCAM_IS0, /* IS0 */
    VTSS_TCAM_IS1, /* IS1 */
    VTSS_TCAM_IS2, /* IS2 */
    VTSS_TCAM_ES0  /* ES0 */
};

typedef struct {
    u32 target;  /* Target offset */
    u16 entries; /* Number of entries */
    u16 actions; /* Number of actions */
} tcam_props_t;

/* Last policer used to discard frames to avoid CPU copies */
#define JR_IS2_POLICER_DISCARD 31

/* ================================================================= *
 *  VCAP control
 * ================================================================= */

/* VCAP value/mask register */
typedef struct {
    BOOL valid;
    u32  value;
    u32  mask;
} jr_vcap_reg_t;

/* VCAP MAC address registers */
typedef struct {
    jr_vcap_reg_t mach;
    jr_vcap_reg_t macl;
} jr_vcap_mac_t;

/* IS1 entry registers */
typedef struct {
    jr_vcap_reg_t entry;   /* ENTRY */
    jr_vcap_reg_t if_grp;  /* IF_GRP */
    jr_vcap_reg_t vlan;    /* VLAN */
    jr_vcap_reg_t flags;   /* FLAGS */
    jr_vcap_mac_t mac;     /* L2_MAC_ADDR_* */
    jr_vcap_reg_t sip;     /* L3_IP4_SIP */
    jr_vcap_reg_t l3_misc; /* L3_MISC */
    jr_vcap_reg_t l4_misc; /* L4_MISC */
    jr_vcap_reg_t l4_port; /* L4_PORT */
} jr_vcap_is1_regs_t;

/* IS2 entry registers */
typedef struct {
    jr_vcap_reg_t vld;        /* ACE_VLD */
    jr_vcap_reg_t entry_type; /* ACE_ENTRY_TYPE */
    jr_vcap_reg_t type;       /* ACE_TYPE */
    jr_vcap_reg_t port_mask;  /* ACE_IGR_PORT_MASK */
    jr_vcap_reg_t pag;        /* ACE_PAG */
    jr_vcap_mac_t smac;       /* ACE_L2_SMAC_* */
    jr_vcap_mac_t dmac;       /* ACE_L2_DMAC_* */
    jr_vcap_reg_t sip;        /* ACE_L3_IP4_SIP */
    jr_vcap_reg_t dip;        /* ACE_L3_IP4_DIP */
    jr_vcap_reg_t l2_misc;    /* ACE_L2_MISC */
    jr_vcap_reg_t l3_misc;    /* ACE_L3_MISC */
    jr_vcap_reg_t data[8];    /* ACE_CUSTOM_DATA_0 - ACE_CUSTOM_DATA_7 */
} jr_vcap_is2_regs_t;

/* VCAP bit */
#define JR_VCAP_BIT(val) ((val) ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_0)

/* Write VCAP value and mask */
#define JR_VCAP_WR(tgt, pre, suf, reg)        \
{                                             \
    JR_WR(tgt, pre##_ENTRY_##suf, reg.value); \
    JR_WR(tgt, pre##_MASK_##suf, ~reg.mask);  \
}

/* Put bit into VCAP value and mask */
#define JR_VCAP_PUT_BIT(tgt, pre, suf, fld, reg, val)                                     \
{                                                                                         \
    reg.value |= JR_PUT_BIT(tgt, pre##_ENTRY_##suf, fld, (val) == VTSS_VCAP_BIT_1 ? 1 : 0); \
    reg.mask |= JR_PUT_BIT(tgt, pre##_MASK_##suf, fld, (val) == VTSS_VCAP_BIT_ANY ? 0 : 1); \
}

/* Set VCAP bit to 1 */
#define JR_VCAP_SET_BIT(tgt, pre, suf, fld, reg)          \
{                                                         \
    reg.value |= JR_SET_BIT(tgt, pre##_ENTRY_##suf, fld); \
    reg.mask |= JR_SET_BIT(tgt, pre##_MASK_##suf, fld);   \
}

/* Set VCAP bit to 0 */
#define JR_VCAP_CLR_BIT(tgt, pre, suf, fld, reg) \
    reg.mask |= JR_SET_BIT(tgt, pre##_MASK_##suf, fld)

/* Put field into VCAP value and mask */
#define JR_VCAP_PUT_FLD_VM(tgt, pre, suf, fld, reg, val, msk)  \
{                                                              \
    reg.value |= JR_PUT_FLD(tgt, pre##_ENTRY_##suf, fld, val); \
    reg.mask |= JR_PUT_FLD(tgt, pre##_MASK_##suf, fld, msk);   \
}

/* Put field into VCAP value and mask */
#define JR_VCAP_PUT_FLD(tgt, pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_FLD_VM(tgt, pre, suf, fld, reg, vm.value, vm.mask)

/* Put U16 field into VCAP value and mask */
#define JR_VCAP_PUT_U16(tgt, pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_FLD_VM(tgt, pre, suf, fld, reg, (vm.value[0]<<8) | vm.value[1], (vm.mask[0]<<8) | vm.mask[1])

/* Put U32 field into VCAP value and mask based offset by index */
#define JR_VCAP_PUT_U32_NDX(reg, vm, ndx)                        \
{                                                                \
    reg.value = ((vm.value[0+ndx]<<24) | (vm.value[1+ndx]<<16) | \
                 (vm.value[2+ndx]<<8) | vm.value[3+ndx]);        \
    reg.mask = ((vm.mask[0+ndx]<<24) | (vm.mask[1+ndx]<<16) |    \
                (vm.mask[2+ndx]<<8) | vm.mask[3+ndx]);           \
}

/* Put U32 field into VCAP value and mask */
#define JR_VCAP_PUT_U32(reg, vm) JR_VCAP_PUT_U32_NDX(reg, vm, 0)

/* Put U40 field into VCAP value and mask */
#define JR_VCAP_PUT_U40(mac, vm)                                                               \
{                                                                                              \
    mac.mach.value = (vm.value[0]);                                                            \
    mac.mach.mask = (vm.mask[0]);                                                              \
    mac.macl.value = ((vm.value[1]<<24) | (vm.value[2]<<16) | (vm.value[3]<<8) | vm.value[4]); \
    mac.macl.mask = ((vm.mask[1]<<24) | (vm.mask[2]<<16) | (vm.mask[3]<<8) | vm.mask[4]);      \
}

/* Put U48 field into VCAP value and mask */
#define JR_VCAP_PUT_U48(mac, vm)                                                               \
{                                                                                              \
    mac.mach.value = ((vm.value[0]<<8) | vm.value[1]);                                         \
    mac.mach.mask = ((vm.mask[0]<<8) | vm.mask[1]);                                            \
    mac.macl.value = ((vm.value[2]<<24) | (vm.value[3]<<16) | (vm.value[4]<<8) | vm.value[5]); \
    mac.macl.mask = ((vm.mask[2]<<24) | (vm.mask[3]<<16) | (vm.mask[4]<<8) | vm.mask[5]);      \
}

/* Put U48 MAC field into VCAP value and mask (3 bytes in mach and 3 bytes in macl */
#define JR_VCAP_PUT_MAC(mac, vm)                                                               \
{                                                                                              \
    mac.mach.value = ((vm.value[0]<<16) | (vm.value[1]<<8) | vm.value[2]);                     \
    mac.mach.mask  = ((vm.mask[0] <<16) | (vm.mask[1] <<8) | vm.mask[2]);                      \
    mac.macl.value = ((vm.value[3]<<16) | (vm.value[4]<<8) | vm.value[5]);                     \
    mac.macl.mask  = ((vm.mask[3] <<16) | (vm.mask[4] <<8) | vm.mask[5]);                      \
}

/* Write VCAP MAC address value and mask */
#define JR_VCAP_WR_MAC(tgt, pre, suf, vm)            \
{                                                    \
    jr_vcap_mac_t vcap_mac;                          \
    JR_VCAP_PUT_U48(vcap_mac, vm);                   \
    JR_VCAP_WR(tgt, pre, suf##_HIGH, vcap_mac.mach); \
    JR_VCAP_WR(tgt, pre, suf##_LOW, vcap_mac.macl);  \
}

/* Read VCAP value and mask */
#define JR_VCAP_RD(tgt, pre, suf, reg)         \
{                                              \
    reg.valid = 1;                             \
    JR_RD(tgt, pre##_ENTRY_##suf, &reg.value); \
    JR_RD(tgt, pre##_MASK_##suf, &reg.mask);   \
}

/* Read VCAP MAC address */
#define JR_VCAP_RD_MAC(tgt, pre, suf, reg)       \
{                                                \
    JR_VCAP_RD(tgt, pre, suf##_HIGH, reg.mach); \
    JR_VCAP_RD(tgt, pre, suf##_LOW, reg.macl);   \
}

/* Debug VCAP bit */
#define JR_VCAP_DEBUG_BIT(pr, name, tgt, pre, suf, fld, reg) \
    jr_debug_vcap_reg(pr, name, reg, VTSS_F_##tgt##_##pre##_ENTRY_##suf##_##fld)

/* Debug VCAP field */
#define JR_VCAP_DEBUG_FLD(pr, name, tgt, pre, suf, fld, reg)             \
    jr_debug_vcap_reg(pr, name, reg, VTSS_M_##tgt##_##pre##_ENTRY_##suf##_##fld)

/* IS0 VCAP macros */
#define JR_IS0_WR(pre, suf, reg) JR_VCAP_WR(VCAP_IS0, pre, suf, reg)
#define JR_IS0_WR_MAC(pre, suf, mac) JR_VCAP_WR_MAC(VCAP_IS0, pre, suf, mac)
#define JR_IS0_PUT_BIT(pre, suf, fld, reg, val) \
    JR_VCAP_PUT_BIT(VCAP_IS0, pre, suf, fld, reg, val)
#define JR_IS0_SET_BIT(pre, suf, fld, reg) JR_VCAP_SET_BIT(VCAP_IS0, pre, suf, fld, reg)
#define JR_IS0_CLR_BIT(pre, suf, fld, reg) JR_VCAP_CLR_BIT(VCAP_IS0, pre, suf, fld, reg)
#define JR_IS0_PUT_FLD_VM(pre, suf, fld, reg, val, msk) \
    JR_VCAP_PUT_FLD_VM(VCAP_IS0, pre, suf, fld, reg, val, msk)
#define JR_IS0_PUT_FLD(pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_FLD(VCAP_IS0, pre, suf, fld, reg, vm)
#define JR_IS0_PUT_U16(pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_U16(VCAP_IS0, pre, suf, fld, reg, vm)
#define JR_IS0_RD(pre, suf, reg) JR_VCAP_RD(VCAP_IS0, pre, suf, reg)
#define JR_IS0_RD_MAC(pre, suf, reg) JR_VCAP_RD_MAC(VCAP_IS0, pre, suf, reg)
#define JR_IS0_DEBUG_BIT(pr, name, pre, suf, fld, reg) \
    JR_VCAP_DEBUG_BIT(pr, name, VCAP_IS0, pre, suf, fld, reg)
#define JR_IS0_DEBUG_FLD(pr, name, pre, suf, fld, reg) \
    JR_VCAP_DEBUG_FLD(pr, name, VCAP_IS0, pre, suf, fld, reg)

/* IS1 VCAP macros */
#define JR_IS1_WR(pre, suf, reg) JR_VCAP_WR(VCAP_IS1, pre, suf, reg)
#define JR_IS1_WR_MAC(pre, suf, mac) JR_VCAP_WR_MAC(VCAP_IS1, pre, suf, mac)
#define JR_IS1_PUT_BIT(pre, suf, fld, reg, val) \
    JR_VCAP_PUT_BIT(VCAP_IS1, pre, suf, fld, reg, val)
#define JR_IS1_SET_BIT(pre, suf, fld, reg) JR_VCAP_SET_BIT(VCAP_IS1, pre, suf, fld, reg)
#define JR_IS1_CLR_BIT(pre, suf, fld, reg) JR_VCAP_CLR_BIT(VCAP_IS1, pre, suf, fld, reg)
#define JR_IS1_PUT_FLD_VM(pre, suf, fld, reg, val, msk) \
    JR_VCAP_PUT_FLD_VM(VCAP_IS1, pre, suf, fld, reg, val, msk)
#define JR_IS1_PUT_FLD(pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_FLD(VCAP_IS1, pre, suf, fld, reg, vm)
#define JR_IS1_PUT_U16(pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_U16(VCAP_IS1, pre, suf, fld, reg, vm)
#define JR_IS1_RD(pre, suf, reg) JR_VCAP_RD(VCAP_IS1, pre, suf, reg)
#define JR_IS1_RD_MAC(pre, suf, reg) JR_VCAP_RD_MAC(VCAP_IS1, pre, suf, reg)
#define JR_IS1_DEBUG_BIT(pr, name, pre, suf, fld, reg) \
    JR_VCAP_DEBUG_BIT(pr, name, VCAP_IS1, pre, suf, fld, reg)
#define JR_IS1_DEBUG_FLD(pr, name, pre, suf, fld, reg) \
    JR_VCAP_DEBUG_FLD(pr, name, VCAP_IS1, pre, suf, fld, reg)

/* IS2 VCAP macros */
#define JR_IS2_WR(pre, suf, reg) JR_VCAP_WR(VCAP_IS2, pre, suf, reg)
#define JR_IS2_WR_MAC(pre, suf, mac) JR_VCAP_WR_MAC(VCAP_IS2, pre, suf, mac)
#define JR_IS2_PUT_BIT(pre, suf, fld, reg, val) \
    JR_VCAP_PUT_BIT(VCAP_IS2, pre, suf, fld, reg, val)
#define JR_IS2_SET_BIT(pre, suf, fld, reg) JR_VCAP_SET_BIT(VCAP_IS2, pre, suf, fld, reg)
#define JR_IS2_CLR_BIT(pre, suf, fld, reg) JR_VCAP_CLR_BIT(VCAP_IS2, pre, suf, fld, reg)
#define JR_IS2_PUT_FLD_VM(pre, suf, fld, reg, val, msk) \
    JR_VCAP_PUT_FLD_VM(VCAP_IS2, pre, suf, fld, reg, val, msk)
#define JR_IS2_PUT_FLD(pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_FLD(VCAP_IS2, pre, suf, fld, reg, vm)
#define JR_IS2_PUT_U16(pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_U16(VCAP_IS2, pre, suf, fld, reg, vm)
#define JR_IS2_RD(pre, suf, reg) JR_VCAP_RD(VCAP_IS2, pre, suf, reg)
#define JR_IS2_RD_MAC(pre, suf, reg) JR_VCAP_RD_MAC(VCAP_IS2, pre, suf, reg)
#define JR_IS2_DEBUG_BIT(pr, name, pre, suf, fld, reg) \
    JR_VCAP_DEBUG_BIT(pr, name, VCAP_IS2, pre, suf, fld, reg)
#define JR_IS2_DEBUG_FLD(pr, name, pre, suf, fld, reg) \
    JR_VCAP_DEBUG_FLD(pr, name, VCAP_IS2, pre, suf, fld, reg)

/* ES0 VCAP macros */
#define JR_ES0_WR(pre, suf, reg) JR_VCAP_WR(VCAP_ES0, pre, suf, reg)
#define JR_ES0_WR_MAC(pre, suf, mac) JR_VCAP_WR_MAC(VCAP_ES0, pre, suf, mac)
#define JR_ES0_PUT_BIT(pre, suf, fld, reg, val) \
    JR_VCAP_PUT_BIT(VCAP_ES0, pre, suf, fld, reg, val)
#define JR_ES0_SET_BIT(pre, suf, fld, reg) JR_VCAP_SET_BIT(VCAP_ES0, pre, suf, fld, reg)
#define JR_ES0_CLR_BIT(pre, suf, fld, reg) JR_VCAP_CLR_BIT(VCAP_ES0, pre, suf, fld, reg)
#define JR_ES0_PUT_FLD_VM(pre, suf, fld, reg, val, msk) \
    JR_VCAP_PUT_FLD_VM(VCAP_ES0, pre, suf, fld, reg, val, msk)
#define JR_ES0_PUT_FLD(pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_FLD(VCAP_ES0, pre, suf, fld, reg, vm)
#define JR_ES0_PUT_U16(pre, suf, fld, reg, vm) \
    JR_VCAP_PUT_U16(VCAP_ES0, pre, suf, fld, reg, vm)
#define JR_ES0_RD(pre, suf, reg) JR_VCAP_RD(VCAP_ES0, pre, suf, reg)
#define JR_ES0_RD_MAC(pre, suf, reg) JR_VCAP_RD_MAC(VCAP_ES0, pre, suf, reg)
#define JR_ES0_DEBUG_BIT(pr, name, pre, suf, fld, reg) \
    JR_VCAP_DEBUG_BIT(pr, name, VCAP_ES0, pre, suf, fld, reg)
#define JR_ES0_DEBUG_FLD(pr, name, pre, suf, fld, reg) \
    JR_VCAP_DEBUG_FLD(pr, name, VCAP_ES0, pre, suf, fld, reg)

static const tcam_props_t tcam_info[] = {
    [VTSS_TCAM_IS0] = {
        .target = VTSS_TO_VCAP_IS0,
        .entries = VTSS_JR1_IS0_CNT,
        .actions = VTSS_JR1_IS0_CNT + 35
    },
    [VTSS_TCAM_IS1] = {
        .target = VTSS_TO_VCAP_IS1,
        .entries = VTSS_JR1_IS1_CNT,
        .actions = VTSS_JR1_IS1_CNT + 2
    },
    [VTSS_TCAM_IS2] = {
        .target = VTSS_TO_VCAP_IS2,
        .entries = VTSS_JR1_IS2_CNT,
        .actions =  VTSS_JR1_IS2_CNT + 35
    },
    [VTSS_TCAM_ES0] = {
        .target = VTSS_TO_VCAP_ES0,
        .entries = VTSS_JR1_ES0_CNT,
        .actions = VTSS_JR1_ES0_CNT + 1
    }
};

static vtss_rc jr_vcap_command(int bank, u16 ix, int cmd)
{
    u32 value;

    if (bank == VTSS_TCAM_IS0) {
        JR_WR(VCAP_IS0, IS0_CONTROL_ACE_UPDATE_CTRL, 
              VTSS_F_VCAP_IS0_IS0_CONTROL_ACE_UPDATE_CTRL_ACE_INDEX(ix) |
              VTSS_F_VCAP_IS0_IS0_CONTROL_ACE_UPDATE_CTRL_ACE_UPDATE_CMD(cmd) |
              VTSS_F_VCAP_IS0_IS0_CONTROL_ACE_UPDATE_CTRL_ACE_UPDATE_SHOT);
        do {
            JR_RDB(VCAP_IS0, IS0_CONTROL_ACE_UPDATE_CTRL, ACE_UPDATE_SHOT, &value);
        } while (value);
    } else if (bank == VTSS_TCAM_IS1) {
        JR_WR(VCAP_IS1, IS1_CONTROL_ACE_UPDATE_CTRL, 
              VTSS_F_VCAP_IS1_IS1_CONTROL_ACE_UPDATE_CTRL_ACE_INDEX(ix) |
              VTSS_F_VCAP_IS1_IS1_CONTROL_ACE_UPDATE_CTRL_ACE_UPDATE_CMD(cmd) |
              VTSS_F_VCAP_IS1_IS1_CONTROL_ACE_UPDATE_CTRL_ACE_UPDATE_SHOT);
        do {
            JR_RDB(VCAP_IS1, IS1_CONTROL_ACE_UPDATE_CTRL, ACE_UPDATE_SHOT, &value);
        } while (value);
    } else if (bank == VTSS_TCAM_IS2) {
        JR_WR(VCAP_IS2, IS2_CONTROL_ACE_UPDATE_CTRL, 
              VTSS_F_VCAP_IS2_IS2_CONTROL_ACE_UPDATE_CTRL_ACE_INDEX(ix) |
              VTSS_F_VCAP_IS2_IS2_CONTROL_ACE_UPDATE_CTRL_ACE_UPDATE_CMD(cmd) |
              VTSS_F_VCAP_IS2_IS2_CONTROL_ACE_UPDATE_CTRL_ACE_UPDATE_SHOT);
        do {
            JR_RDB(VCAP_IS2, IS2_CONTROL_ACE_UPDATE_CTRL, ACE_UPDATE_SHOT, &value);
        } while (value);
    } else if (bank == VTSS_TCAM_ES0) {
        JR_WR(VCAP_ES0, ES0_CONTROL_ACE_UPDATE_CTRL, 
              VTSS_F_VCAP_ES0_ES0_CONTROL_ACE_UPDATE_CTRL_ACE_INDEX(ix) |
              VTSS_F_VCAP_ES0_ES0_CONTROL_ACE_UPDATE_CTRL_ACE_UPDATE_CMD(cmd) |
              VTSS_F_VCAP_ES0_ES0_CONTROL_ACE_UPDATE_CTRL_ACE_UPDATE_SHOT);
        do {
            JR_RDB(VCAP_ES0, ES0_CONTROL_ACE_UPDATE_CTRL, ACE_UPDATE_SHOT, &value);
        } while (value);
    } else {
        VTSS_E("illegal bank: %d", bank);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_vcap_port_command(int bank, u32 port, int cmd)
{
    return jr_vcap_command(bank, tcam_info[bank].entries + port, cmd);
}

static vtss_rc jr_vcap_index_command(int bank, u32 ix, int cmd)
{
    return jr_vcap_command(bank, tcam_info[bank].entries - ix - 1, cmd);
}

static vtss_rc jr_is2_prepare_action(vtss_acl_action_t *action, u32 action_ext, u32 counter)
{
    BOOL police = action->police;
    u32  policer_no = action->policer_no;
    BOOL rx_timestamp = 0;
    BOOL redir_ena = 0;

    if (action->cpu || action->cpu_once) {
        /* Enable Rx timestamping for all frames to CPU (for PTP) */
        rx_timestamp = 1;
    } else if (action->forward == 0) {
        /* If discarding and not forwarding to CPU, use discard policer to avoid CPU copy */
        police = 1;
        policer_no = JR_IS2_POLICER_DISCARD;
    }

    JR_WR(VCAP_IS2, BASETYPE_ACTION_A,
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_A, HIT_ME_ONCE, action->cpu_once) |
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_A, CPU_COPY_ENA, action->cpu) |
          JR_PUT_FLD(VCAP_IS2, BASETYPE_ACTION_A, CPU_QU_NUM, action->cpu_queue) |
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_A, IRQ_TRIGGER, action->irq_trigger) |
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_A, FW_ENA, action->forward) |
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_A, LRN_ENA, action->learn) |
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_A, POLICE_ENA, police) |
          JR_PUT_FLD(VCAP_IS2, BASETYPE_ACTION_A, POLICE_IDX, policer_no) |
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_A, OAM_RX_TIMESTAMP_ENA, rx_timestamp) |
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_A, RT_ENA, action->learn));

    if (action->port_forward && action->port_no < vtss_state->port_count && 
        VTSS_PORT_CHIP_SELECTED(action->port_no)) {
        /* It is only possible to redirect to ports on the same device */
        redir_ena = 1;
    }
    JR_WR(VCAP_IS2, BASETYPE_ACTION_B,
          JR_PUT_BIT(VCAP_IS2, BASETYPE_ACTION_B, REDIR_ENA, redir_ena) |
          JR_PUT_FLD(VCAP_IS2, BASETYPE_ACTION_B, REDIR_PGID, 
                     redir_ena ? jr_chip_pgid(action->port_no) : action_ext));
          
    JR_WR(VCAP_IS2, BASETYPE_ACTION_C, 0xffffffff);
    
    JR_WR(VCAP_IS2, BASETYPE_ACTION_D, counter);

    return VTSS_RC_OK;
}

/* Calculate policer rate

   If input rate is kbps (frame_rate = FALSE) then output rate is calculated like this:
   output rate = ((input rate * 1000) + 100159) / 100160
   This will round up the rate to the nearest possible value:
     0 -> 0 (Open until burst capacity is used, then closed)
     1..100 -> 1 (100160 bps)
   101..200 -> 2 (200320 bps)
   201..300 -> 3 (300480 bps)
   ...

   If input rate is frames per second (frame_rate = TRUE) then output rate is also frames per second.

   In both cases the maximum rate returned is limited to 0x1FFF.
*/
static u32 jr_calc_policer_rate(u32 rate, BOOL frame_rate)
{
    if (!frame_rate) { /* input rate is measured in kbps */
        rate = VTSS_DIV64(((u64)rate * 1000) + 100159, 100160);
    }
    return MIN(0x1ffff, rate);
}

/* Calculate policer burst level
*/
static u32 jr_calc_policer_level(u32 level, u32 rate, BOOL frame_rate)
{
    if (rate == 0) {
        return 0;                               /* Always closed */
    } else if ((rate == VTSS_BITRATE_DISABLED) || frame_rate) {
        return 0x3f;                            /* Maximum burst level */
    } else {
        return MIN(0x3f, MAX(1, level / 2048)); /* Calculated value 1..0x3f ~ 2048..129024 bytes */
    }
}

static vtss_rc jr_acl_policer_set_chip(const vtss_acl_policer_no_t policer_no)
{
    u32                     rate, i = (policer_no - VTSS_ACL_POLICER_NO_START);
    vtss_acl_policer_conf_t *conf = &vtss_state->acl_policer_conf[i];
    
    rate = MIN(0x1ffff, conf->rate);
    JR_WRX(ANA_AC, POL_ALL_CFG_POL_ACL_RATE_CFG, i, 
           JR_PUT_FLD(ANA_AC, POL_ALL_CFG_POL_ACL_RATE_CFG, ACL_RATE, rate));
    
    /* 4 kB threshold */
    JR_WRX(ANA_AC, POL_ALL_CFG_POL_ACL_THRES_CFG, i, 
           JR_PUT_FLD(ANA_AC, POL_ALL_CFG_POL_ACL_THRES_CFG, ACL_THRES, 1));
    
    /* Frame rate */
    JR_WRX(ANA_AC, POL_ALL_CFG_POL_ACL_CTRL, i, 
           JR_PUT_FLD(ANA_AC, POL_ALL_CFG_POL_ACL_CTRL, ACL_TRAFFIC_TYPE_MASK, 3) |
           JR_PUT_BIT(ANA_AC, POL_ALL_CFG_POL_ACL_CTRL, FRAME_RATE_ENA, 1));

    return VTSS_RC_OK;
}

static vtss_rc jr_acl_policer_set(const vtss_acl_policer_no_t policer_no)
{
    vtss_chip_no_t chip_no;

    /* Setup all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_acl_policer_set_chip(policer_no));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_acl_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_acl_port_conf_t *conf = &vtss_state->acl_port_conf[port_no];
    u32                  pag = 0, cnt, port = VTSS_CHIP_PORT(port_no);
    
    /* Enable/disable IS2 lookups on port */
    VTSS_RC(jr_vcap_lookup_set(port_no));

    /* Setup PAG using default actions in IS0 */
    if (conf->policy_no != VTSS_ACL_POLICY_NO_NONE)
        pag = (conf->policy_no - VTSS_ACL_POLICY_NO_START);
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS0, port, VTSS_TCAM_CMD_READ));
    JR_WRF(VCAP_IS0, BASETYPE_ACTION_B, PAG_VAL, pag);
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS0, port, VTSS_TCAM_CMD_WRITE));

    /* Setup action */
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS2, port, VTSS_TCAM_CMD_READ));
    JR_RD(VCAP_IS2, BASETYPE_ACTION_D, &cnt);
    VTSS_RC(jr_is2_prepare_action(&conf->action, 0, cnt));
    return jr_vcap_port_command(VTSS_TCAM_IS2, port, VTSS_TCAM_CMD_WRITE);
}

static vtss_rc jr_acl_port_counter_get(const vtss_port_no_t    port_no,
                                       vtss_acl_port_counter_t *const counter)
{
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS2, VTSS_CHIP_PORT(port_no), VTSS_TCAM_CMD_READ));
    JR_RD(VCAP_IS2, BASETYPE_ACTION_D, counter);
    return VTSS_RC_OK;
}

static vtss_rc jr_acl_port_counter_clear(const vtss_port_no_t port_no)
{
    u32 port = VTSS_CHIP_PORT(port_no);
    
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS2, port, VTSS_TCAM_CMD_READ));
    JR_WR(VCAP_IS2, BASETYPE_ACTION_D, 0);
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS2, port, VTSS_TCAM_CMD_WRITE));
    return VTSS_RC_OK;
}

static vtss_rc jr_ace_add(const vtss_ace_id_t ace_id, const vtss_ace_t *const ace)
{
    vtss_vcap_obj_t             *is2_obj = &vtss_state->is2.obj;
    vtss_vcap_user_t            is2_user = VTSS_IS2_USER_ACL;
    vtss_vcap_data_t            data;
    vtss_is2_data_t             *is2 = &data.u.is2;
    vtss_is2_entry_t            entry;
    vtss_ace_t                  *ace_copy = &entry.ace;
    const vtss_ace_udp_tcp_t    *sport = NULL, *dport = NULL;
    vtss_vcap_id_t              id, id_next;
    u32                         old = 0, old_any = 0, new_any = 0;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range; 
    
    /*** Step 1: Check the simple things */
    VTSS_RC(vtss_cmn_ace_add(ace_id, ace));

    /*** Step 2: Check if IS2 entries can be added */

    /* Initialize entry data */
    vtss_vcap_is2_init(&data, &entry);

    /* Check if main entry exists */
    if (vtss_vcap_lookup(is2_obj, is2_user, ace->id, &data, NULL) == VTSS_RC_OK)
        old = 1;

    /* Check if extra any entries exist */
    id = (ace->id + JR_IS2_ID_ANY_LLC);
    if (vtss_vcap_lookup(is2_obj, is2_user, id, NULL, NULL) == VTSS_RC_OK)
        old_any = JR_IS2_ID_ANY_CNT;

    /* Determine IS2 type and check if extra any entries must be added */
    switch (ace->type) {
    case VTSS_ACE_TYPE_ANY:
        entry.type = IS2_ENTRY_TYPE_ETYPE;
        entry.type_mask = JR_IS2_ANY_ETYPE_MASK;
        new_any = JR_IS2_ID_ANY_CNT;
        break;
    case VTSS_ACE_TYPE_ETYPE:
        entry.type = IS2_ENTRY_TYPE_ETYPE;
        break;
    case VTSS_ACE_TYPE_LLC:
        entry.type = IS2_ENTRY_TYPE_LLC;
        break;
    case VTSS_ACE_TYPE_SNAP:
        entry.type = IS2_ENTRY_TYPE_SNAP;
        break;
    case VTSS_ACE_TYPE_ARP:
        entry.type = IS2_ENTRY_TYPE_ARP;
        break;
    case VTSS_ACE_TYPE_IPV4:
        if (!vtss_vcap_udp_tcp_rule(&ace->frame.ipv4.proto))
            entry.type_mask = JR_IS2_ANY_IP_MASK;
        /* FALLTHROUGH */
    case VTSS_ACE_TYPE_IPV6:
        entry.type = IS2_ENTRY_TYPE_IP_OTHER;
        break;
    default:
        VTSS_E("illegal type: %d", ace->type);
        return VTSS_RC_ERROR;
    }

    if ((is2_obj->count + new_any) >= (is2_obj->max_count + old + old_any)) {
        VTSS_I("IS2 is full");
        return VTSS_RC_ERROR;
    }

    /* Calculate next ID and check that it exists */
    id = (ace_id + JR_IS2_ID_ANY_LLC);
    id_next = (vtss_vcap_lookup(is2_obj, is2_user, id, NULL, NULL) == VTSS_RC_OK ? id : ace_id);
    VTSS_RC(vtss_vcap_add(is2_obj, is2_user, ace->id, id_next, NULL, 0));

    /*** Step 3: Allocate range checkers */
    /* Free any old range checkers */
    VTSS_RC(vtss_vcap_range_free(&range_new, is2->srange));
    VTSS_RC(vtss_vcap_range_free(&range_new, is2->drange));
    is2->srange = VTSS_VCAP_RANGE_CHK_NONE;
    is2->drange = VTSS_VCAP_RANGE_CHK_NONE;

    if (ace->type == VTSS_ACE_TYPE_IPV4 && vtss_vcap_udp_tcp_rule(&ace->frame.ipv4.proto)) {
        sport = &ace->frame.ipv4.sport;
        dport = &ace->frame.ipv4.dport;
    }

    if (sport && dport) {
        /* Allocate new range checkers */
        VTSS_RC(vtss_vcap_udp_tcp_range_alloc(&range_new, &is2->srange, sport, 1));
        VTSS_RC(vtss_vcap_udp_tcp_range_alloc(&range_new, &is2->drange, dport, 0));
    }

    /* Commit range checkers */
    VTSS_RC(vtss_vcap_range_commit(&range_new));

    /*** Step 4: Add IS2 entries */

    /* Add main entry */
    *ace_copy = *ace;
    is2->entry = &entry;
    VTSS_RC(vtss_vcap_add(is2_obj, is2_user, ace->id, id_next, &data, 0));
    if (new_any) {
        /* Add block of IS2 rules matching all frame types */
        VTSS_RC(jr_is2_add_any(is2_user, ace->id, id_next, &data, 0));
    } else if (old_any) {
        /* Delete extra any entries */
        VTSS_RC(jr_is2_del_any(is2_user, ace->id, 0));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_ace_del(const vtss_ace_id_t ace_id)
{
    /* Delete main entry */
    VTSS_RC(vtss_cmn_ace_del(ace_id));
    
    /* Delete extra any entries */
    VTSS_RC(jr_is2_del_any(VTSS_IS2_USER_ACL, ace_id, 0));
    
    return VTSS_RC_OK;
}

static vtss_rc jr_ace_get(vtss_vcap_id_t id, vtss_ace_counter_t *const counter, BOOL clear)
{
    vtss_rc         rc;
    vtss_vcap_obj_t *obj = &vtss_state->is2.obj;
    u32             cnt = 0;
    vtss_vcap_idx_t idx;
    
    /* Add/clear counter from extra entry */
    if ((rc = vtss_vcap_lookup(obj, VTSS_IS2_USER_ACL, id, NULL, &idx)) == VTSS_RC_OK) {
        VTSS_RC(obj->entry_get(&idx, &cnt, clear));
        if (counter != NULL)
            *counter += cnt;
    }
    return rc;
}

static vtss_rc jr_ace_get_any(const vtss_ace_id_t ace_id, 
                              vtss_ace_counter_t *const counter, 
                              BOOL clear)
{
    if (jr_ace_get(ace_id + JR_IS2_ID_ANY_LLC, counter, clear) == VTSS_RC_OK) {
        VTSS_RC(jr_ace_get(ace_id + JR_IS2_ID_ANY_SNAP, counter, clear));
        VTSS_RC(jr_ace_get(ace_id + JR_IS2_ID_ANY_OAM, counter, clear));
        VTSS_RC(jr_ace_get(ace_id + JR_IS2_ID_ANY_IP, counter, clear));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_ace_counter_get(const vtss_ace_id_t ace_id, vtss_ace_counter_t *const counter)
{
    VTSS_RC(vtss_cmn_ace_counter_get(ace_id, counter));
    return jr_ace_get_any(ace_id, counter, 0);
}

static vtss_rc jr_ace_counter_clear(const vtss_ace_id_t ace_id)
{
    VTSS_RC(vtss_cmn_ace_counter_clear(ace_id));
    return jr_ace_get_any(ace_id, NULL, 1);
}

static vtss_rc jr_vcap_range_commit(void)
{
    vtss_chip_no_t        chip_no;
    u32                   i, type;
    vtss_vcap_range_chk_t *entry;
    
    for (i = 0; i < VTSS_VCAP_RANGE_CHK_CNT; i++) {
        entry = &vtss_state->vcap_range.entry[i];
        if (entry->count == 0)
            continue;
        switch (entry->type) {
        case VTSS_VCAP_RANGE_TYPE_SPORT:
            type = 2;
            break;
        case VTSS_VCAP_RANGE_TYPE_DPORT:
            type = 1;
            break;
        case VTSS_VCAP_RANGE_TYPE_SDPORT:
            type = 3;
            break;
        case VTSS_VCAP_RANGE_TYPE_VID:
            type = 4;
            break;
        case VTSS_VCAP_RANGE_TYPE_DSCP:
            type = 5;
            break;
        default:
            VTSS_E("illegal type: %d", entry->type);
            return VTSS_RC_ERROR;
        }
        for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
            VTSS_SELECT_CHIP(chip_no);
            JR_WRX(ANA_CL_2, COMMON_ADV_RNG_CTRL, i, 
                   JR_PUT_FLD(ANA_CL_2, COMMON_ADV_RNG_CTRL, RNG_TYPE_SEL, type));
            JR_WRX(ANA_CL_2, COMMON_ADV_RNG_VALUE_CFG, i, 
                   JR_PUT_FLD(ANA_CL_2, COMMON_ADV_RNG_VALUE_CFG, RNG_MAX_VALUE, entry->max) |
                   JR_PUT_FLD(ANA_CL_2, COMMON_ADV_RNG_VALUE_CFG, RNG_MIN_VALUE, entry->min));
        }
    }
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  VCAP functions
 * ================================================================= */

/* Get VCAP entry from all chips */
static vtss_rc jr_vcap_entry_get_chips(vtss_rc (* func_get)(u32 ix, u32 *cnt, BOOL clear),
                                       u32 ix, u32 *counter, BOOL clear)
{
    vtss_chip_no_t chip_no;
    u32            cnt;

    /* Read/clear from all devices */
    *counter = 0;
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(func_get(ix, &cnt, clear));
        *counter += cnt;
        vtss_state->vcap_counter[chip_no] = cnt;
    }
    return VTSS_RC_OK;
}

/* Add VCAP entry to all devices */
static vtss_rc jr_vcap_entry_add_chips(vtss_rc (* func_add)(u32 ix, vtss_vcap_data_t *data, u32 cnt),
                                       u32 ix, vtss_vcap_data_t *data, u32 counter)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(func_add(ix, data, counter == 0 ? 0 : vtss_state->vcap_counter[chip_no]));
    }
    return VTSS_RC_OK;
}

/* Delete VCAP entry from all devices */
static vtss_rc jr_vcap_entry_del_chips(vtss_rc (* func_del)(u32 ix), u32 ix)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(func_del(ix));
    }
    return VTSS_RC_OK;
}

/* Move VCAP entry for all devices */
static vtss_rc jr_vcap_entry_move_chips(vtss_rc (* func_move)(u32 ix, u32 count, BOOL up),
                                        u32 ix, u32 count, BOOL up)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(func_move(ix, count, up));
    }
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  IS0 functions
 * ================================================================= */

/* Get IS0 entry for chip */
static vtss_rc jr_is0_entry_get_chip(u32 ix, u32 *cnt, BOOL clear) 
{
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS0, ix, VTSS_TCAM_CMD_READ));
    JR_RD(VCAP_IS0, BASETYPE_ACTION_C, cnt);

    if (clear) {
        JR_WR(VCAP_IS0, BASETYPE_ACTION_C, 0);
        VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS0, ix, VTSS_TCAM_CMD_WRITE));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_is0_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_I("row: %u", idx->row);

    /* Read/clear from all devices */
    return jr_vcap_entry_get_chips(jr_is0_entry_get_chip, idx->row, counter, clear);
}

static vtss_rc jr_is0_entry_add_chip(u32 ix, vtss_vcap_data_t *data, u32 counter)
{
    vtss_is0_data_t   *is0 = &data->u.is0;
    vtss_is0_key_t    *key = &is0->entry->key;
    vtss_is0_action_t *action = &is0->entry->action;
    vtss_vcap_tag_t   *tag;
    vtss_is0_proto_t  proto;
    jr_vcap_reg_t     reg, reg1;
    u32               port;

    memset(&reg, 0, sizeof(reg));
    JR_IS0_SET_BIT(ISID, A, VLD, reg);
    if (key->port_no != VTSS_PORT_NO_NONE) {
        port = (VTSS_PORT_CHIP_SELECTED(key->port_no) ? VTSS_CHIP_PORT(key->port_no) : 0xff);
        JR_IS0_PUT_FLD_VM(ISID, A, VIGR_PORT, reg, port, 0xff);
    }

    switch (key->type) {
    case VTSS_IS0_TYPE_DBL_VID:
        JR_IS0_WR(DBL_VID, A, reg);

        /* DBL_VID0 and DBL_VID1 */
        memset(&reg, 0, sizeof(reg));
        memset(&reg1, 0, sizeof(reg1));
        
        /* Outer tag */
        tag = &key->data.dbl_vid.outer_tag;
        JR_IS0_PUT_BIT(DBL_VID, DBL_VID0, VLAN_TAGGED, reg, tag->tagged);
        if (tag->tagged == VTSS_VCAP_BIT_1) {
            /* Filtering on outer tag fields and inner tag only done if outer tag present */
            JR_IS0_PUT_BIT(DBL_VID, DBL_VID0, OUTER_DEI, reg, tag->dei);
            JR_IS0_PUT_FLD(DBL_VID, DBL_VID0, OUTER_PCP, reg, tag->pcp);
            JR_IS0_PUT_BIT(DBL_VID, DBL_VID0, OUTER_TPI, reg, tag->s_tag);
            JR_IS0_PUT_FLD(DBL_VID, DBL_VID1, OUTER_VID, reg1, tag->vid);

            /* Inner tag */
            tag = &key->data.dbl_vid.inner_tag;
            JR_IS0_PUT_BIT(DBL_VID, DBL_VID0, VLAN_DBL_TAGGED, reg, tag->tagged);
            if (tag->tagged == VTSS_VCAP_BIT_1) {
                JR_IS0_PUT_BIT(DBL_VID, DBL_VID0, INNER_DEI, reg, tag->dei);
                JR_IS0_PUT_FLD(DBL_VID, DBL_VID0, INNER_PCP, reg, tag->pcp);
                JR_IS0_PUT_BIT(DBL_VID, DBL_VID0, INNER_TPI, reg, tag->s_tag);
                JR_IS0_PUT_FLD(DBL_VID, DBL_VID1, INNER_VID, reg1, tag->vid);
            }
        }
        
        JR_IS0_WR(DBL_VID, DBL_VID0, reg);
        JR_IS0_WR(DBL_VID, DBL_VID1, reg1);
        
        /* DBL_VID2 */
        memset(&reg, 0, sizeof(reg));
        JR_IS0_PUT_FLD(DBL_VID, DBL_VID2, L3_DSCP, reg, key->data.dbl_vid.dscp);
        if ((proto = key->data.dbl_vid.proto) != VTSS_IS0_PROTO_ANY) {
            JR_IS0_PUT_FLD_VM(DBL_VID, DBL_VID2, PROT, reg, proto - 1, 0x3);
        }
        JR_IS0_WR(DBL_VID, DBL_VID2, reg);
        break;
    default:
        VTSS_E("illegal type: %d", key->type);
        return VTSS_RC_ERROR;
    }

    /* Action */
    JR_WR(VCAP_IS0, BASETYPE_ACTION_A,
          JR_PUT_BIT(VCAP_IS0, BASETYPE_ACTION_A, S1_DMAC_ENA, action->s1_dmac_ena) |
          JR_PUT_FLD(VCAP_IS0, BASETYPE_ACTION_A, VLAN_POP_CNT, action->vlan_pop_cnt) |
          JR_PUT_BIT(VCAP_IS0, BASETYPE_ACTION_A, VID_ENA, action->vid_ena ? 1 : 0) |
          JR_PUT_BIT(VCAP_IS0, BASETYPE_ACTION_A, DEI_VAL, action->dei) |
          JR_PUT_FLD(VCAP_IS0, BASETYPE_ACTION_A, PCP_VAL, action->pcp) |
          JR_PUT_BIT(VCAP_IS0, BASETYPE_ACTION_A, PCP_DEI_ENA, action->pcp_dei_ena));
    JR_WR(VCAP_IS0, BASETYPE_ACTION_B,
          JR_PUT_FLD(VCAP_IS0, BASETYPE_ACTION_B, ISDX_VAL, action->isdx) |
          JR_PUT_FLD(VCAP_IS0, BASETYPE_ACTION_B, VID_VAL, action->vid) |
          JR_PUT_FLD(VCAP_IS0, BASETYPE_ACTION_B, PAG_VAL, action->pag));
    JR_WR(VCAP_IS0, BASETYPE_ACTION_C, counter);

    return jr_vcap_index_command(VTSS_TCAM_IS0, ix, VTSS_TCAM_CMD_WRITE);
}

static vtss_rc jr_is0_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter)
{
    VTSS_I("row: %u", idx->row);

    /* Add to all devices */
    return jr_vcap_entry_add_chips(jr_is0_entry_add_chip, idx->row, data, counter);
}

static vtss_rc jr_is0_entry_del_chip(u32 ix)
{
    JR_WRB(VCAP_IS0, ISID_ENTRY_A, VLD, 0);
    JR_WRB(VCAP_IS0, ISID_MASK_A, VLD, 0);
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS0, ix, VTSS_TCAM_CMD_WRITE));
    return VTSS_RC_OK;
}

static vtss_rc jr_is0_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_I("row: %u", idx->row);

    /* Delete from all devices */
    return jr_vcap_entry_del_chips(jr_is0_entry_del_chip, idx->row);
}

static vtss_rc jr_is0_entry_move_chip(u32 ix, u32 count, BOOL up)
{
    JR_WR(VCAP_IS0, IS0_CONTROL_ACE_MV_CFG, 
          JR_PUT_FLD(VCAP_IS0, IS0_CONTROL_ACE_MV_CFG, ACE_MV_NUM_POS, 1) |
          JR_PUT_FLD(VCAP_IS0, IS0_CONTROL_ACE_MV_CFG, ACE_MV_SIZE, count));
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS0, ix + count - 1, 
                                  up ? VTSS_TCAM_CMD_MOVE_UP : VTSS_TCAM_CMD_MOVE_DOWN));
    return VTSS_RC_OK;
}

static vtss_rc jr_is0_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_I("row: %u, count: %u, up: %u", idx->row, count, up);

    /* Move for all devices */
    return jr_vcap_entry_move_chips(jr_is0_entry_move_chip, idx->row, count, up);
}

/* ================================================================= *
 *  IS1 functions
 * ================================================================= */
static BOOL jr_vcap_is_udp_tcp(vtss_vcap_u8_t *proto)
{
    return (proto->mask == 0xff && (proto->value == 6 || proto->value == 17));
}

static u32 jr_u8_to_u32(u8 *p)
{
    return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

/* Get IS1 entry for chip */
static vtss_rc jr_is1_entry_get_chip(u32 ix, u32 *cnt, BOOL clear) 
{
    u32 type;
    
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS1, ix, VTSS_TCAM_CMD_READ));
    JR_RDB(VCAP_IS1, IS1_CONTROL_ACE_STATUS, ACE_ACTION_TYPE, &type);

    if (type) { /* QOS_ACTION */
        JR_RDB(VCAP_IS1, QOS_ACTION_STICKY, HIT_STICKY, cnt);
    } else { /* VLAN_PAG_ACTION */
        JR_RDB(VCAP_IS1, VLAN_PAG_ACTION_STICKY, HIT_STICKY, cnt);
    }
    
    if (clear) {
        if (type) { /* QOS_ACTION */
            JR_WRB(VCAP_IS1, QOS_ACTION_STICKY, HIT_STICKY, 0);
        } else { /* VLAN_PAG_ACTION */
            JR_WRB(VCAP_IS1, VLAN_PAG_ACTION_STICKY, HIT_STICKY, 0);
        }
        VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS1, ix, VTSS_TCAM_CMD_WRITE));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_is1_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_I("row: %u", idx->row);

    /* Read/clear from all devices */
    return jr_vcap_entry_get_chips(jr_is1_entry_get_chip, idx->row, counter, clear);
}

static vtss_rc jr_is1_entry_add_chip(u32 ix, vtss_vcap_data_t *data, u32 counter)
{
    vtss_is1_data_t    *is1 = &data->u.is1;
    vtss_is1_key_t     *key = &is1->entry->key;
    vtss_is1_tag_t     *tag = &key->tag;
    vtss_is1_action_t  *action = &is1->entry->action;
    u32                etype_len = 0, ip_snap = 0, ip4 = 0, tcp_udp = 0, tcp = 0;
    vtss_vcap_u8_t     *proto = NULL;
    vtss_vcap_vr_t     *dscp = NULL, *sport = NULL, *dport = NULL;
    vtss_vcap_ip_t     sip;
    vtss_vcap_u16_t    etype;
    vtss_vcap_u8_t     range;
    jr_vcap_is1_regs_t regs;

    memset(&sip, 0, sizeof(sip));
    memset(&etype, 0, sizeof(etype));
    memset(&range, 0, sizeof(range));
    memset(&regs, 0, sizeof(regs));

    JR_IS1_SET_BIT(QOS, ENTRY, VLD, regs.entry);

    /* Enabled ports are don't cared, disabled ports must be 0 */
    regs.if_grp.mask = ~jr_port_mask(key->port_list);

    JR_IS1_PUT_BIT(QOS, VLAN, VLAN_TAGGED, regs.vlan, tag->tagged);
    if (tag->vid.type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
        JR_IS1_PUT_FLD(QOS, VLAN, VID, regs.vlan, tag->vid.vr.v);
    }
    if (is1->vid_range != VTSS_VCAP_RANGE_CHK_NONE) {
        range.mask |= (1<<is1->vid_range);
        if (tag->vid.type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
            range.value |= (1<<is1->vid_range);
    }
    JR_IS1_PUT_BIT(QOS, VLAN, VLAN_TAGGED, regs.vlan, tag->tagged);
    JR_IS1_PUT_BIT(QOS, VLAN, OUTER_DEI, regs.vlan, tag->dei);
    JR_IS1_PUT_FLD(QOS, VLAN, PCP, regs.vlan, tag->pcp);

    JR_VCAP_PUT_MAC(regs.mac, key->mac.smac);

    JR_IS1_PUT_BIT(QOS, FLAGS, L2_MC, regs.flags, key->mac.dmac_mc);
    JR_IS1_PUT_BIT(QOS, FLAGS, L2_BC, regs.flags, key->mac.dmac_bc);

    switch (key->type) {
    case VTSS_IS1_TYPE_ANY:
        /* Only common fields are valid */
        break;
    case VTSS_IS1_TYPE_ETYPE:
        etype_len = 1;
        ip_snap = 0;
        ip4 = 0;
        etype = key->frame.etype.etype;
        sip.value = jr_u8_to_u32(key->frame.etype.data.value);
        sip.mask = jr_u8_to_u32(key->frame.etype.data.mask);
        break;
    case VTSS_IS1_TYPE_LLC:
        etype_len = 0;
        ip_snap = 0;
        ip4 = 0;
        etype.value[0] = key->frame.llc.data.value[0];
        etype.mask[0] = key->frame.llc.data.mask[0];
        etype.value[1] = key->frame.llc.data.value[1];
        etype.mask[1] = key->frame.llc.data.mask[1];
        sip.value = jr_u8_to_u32(&key->frame.llc.data.value[2]);
        sip.mask = jr_u8_to_u32(&key->frame.llc.data.mask[2]);
        break;
    case VTSS_IS1_TYPE_SNAP:
        etype_len = 0;
        ip_snap = 1;
        ip4 = 0;
        etype.value[0] = key->frame.snap.data.value[3];     /* Ethertype MSB */
        etype.mask[0] = key->frame.snap.data.mask[3];
        etype.value[1] = key->frame.snap.data.value[4];     /* Ethertype LSB */
        etype.mask[1] = key->frame.snap.data.mask[4];
        sip.value = (key->frame.snap.data.value[0] << 24) | /* OUI[0] */
                    (key->frame.snap.data.value[1] << 16) | /* OUI[1] */
                    (key->frame.snap.data.value[2] << 8)  | /* OUI[2] */
                     key->frame.snap.data.value[5];         /* Payload[0] */
        sip.mask  = (key->frame.snap.data.mask[0] << 24) |
                    (key->frame.snap.data.mask[1] << 16) |
                    (key->frame.snap.data.mask[2] << 8)  |
                     key->frame.snap.data.mask[5];
        break;
    case VTSS_IS1_TYPE_IPV4:
    case VTSS_IS1_TYPE_IPV6:
        etype_len = 1;
        ip_snap = 1;
        if (key->type == VTSS_IS1_TYPE_IPV4) {
            ip4 = 1;
            proto = &key->frame.ipv4.proto;
            dscp = &key->frame.ipv4.dscp;
            sip = key->frame.ipv4.sip;
            sport = &key->frame.ipv4.sport;
            dport = &key->frame.ipv4.dport;
        } else {
            ip4 = 0;
            proto = &key->frame.ipv6.proto;
            dscp = &key->frame.ipv6.dscp;
            sip.value = jr_u8_to_u32(key->frame.ipv6.sip.value);
            sip.mask = jr_u8_to_u32(key->frame.ipv6.sip.mask);
            sport = &key->frame.ipv6.sport;
            dport = &key->frame.ipv6.dport;
        }
        if (jr_vcap_is_udp_tcp(proto)) {
            tcp_udp = 1;
            tcp = VTSS_BOOL(proto->value == 6);
            if (dport->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
                etype.value[0] = ((dport->vr.v.value >> 8) & 0xff);
                etype.value[1] = (dport->vr.v.value & 0xff);
                etype.mask[0] = ((dport->vr.v.mask >> 8) & 0xff);
                etype.mask[1] = (dport->vr.v.mask & 0xff);
            }
        } else {
            etype.value[1] = proto->value;
            etype.mask[1] = proto->mask;
        }
        break;
    default:
        VTSS_E("illegal type: %d", key->type);
        return VTSS_RC_ERROR;
    }

    if (key->type != VTSS_IS1_TYPE_ANY) {
        JR_IS1_PUT_BIT(QOS, FLAGS, ETYPE_LEN, regs.flags, JR_VCAP_BIT(etype_len));

        JR_IS1_PUT_U16(QOS, FLAGS, ETYPE, regs.flags, etype);
        JR_IS1_PUT_BIT(QOS, FLAGS, IP, regs.flags, JR_VCAP_BIT(ip_snap));
        if ((key->type == VTSS_IS1_TYPE_IPV4) || (key->type == VTSS_IS1_TYPE_IPV6)) {
            JR_IS1_PUT_BIT(QOS, FLAGS, IP4, regs.flags, JR_VCAP_BIT(ip4));
            if (ip4) {
                JR_IS1_PUT_BIT(QOS, L3_MISC, L3_FRAGMENT, regs.l3_misc, key->frame.ipv4.fragment);
            }
        }
        if (dscp != NULL) {
            if (dscp->type == VTSS_VCAP_VR_TYPE_VALUE_MASK)
                JR_IS1_PUT_FLD(QOS, L3_MISC, L3_DSCP, regs.l3_misc, dscp->vr.v);
            if (is1->dscp_range != VTSS_VCAP_RANGE_CHK_NONE) {
                range.mask |= (1<<is1->dscp_range);
                if (dscp->type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
                    range.value |= (1<<is1->dscp_range);
            }
        }
        regs.sip.value = sip.value;
        regs.sip.mask = sip.mask;
        if (tcp_udp) {
            JR_IS1_PUT_BIT(QOS, FLAGS, TCP_UDP, regs.flags, JR_VCAP_BIT(tcp_udp));
            JR_IS1_PUT_BIT(QOS, FLAGS, TCP, regs.flags, JR_VCAP_BIT(tcp));
            if (sport != NULL && sport->type == VTSS_VCAP_VR_TYPE_VALUE_MASK)
                JR_IS1_PUT_FLD(QOS, L4_PORT, L4_SPORT, regs.l4_port, sport->vr.v);
            if (is1->sport_range != VTSS_VCAP_RANGE_CHK_NONE) {
                range.mask |= (1<<is1->sport_range);
                if (sport != NULL && sport->type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
                    range.value |= (1<<is1->sport_range);
            }
            if (is1->dport_range != VTSS_VCAP_RANGE_CHK_NONE) {
                range.mask |= (1<<is1->dport_range);
                if (dport != NULL && dport->type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
                    range.value |= (1<<is1->dport_range);
            }
        }
    }
    JR_IS1_PUT_FLD(QOS, L4_MISC, L4_RNG, regs.l4_misc, range);

    if (is1->lookup == 0) { /* VLAN_PAG */
        /* Update entry */
        JR_IS1_WR(VLAN_PAG, ENTRY, regs.entry);
        JR_IS1_WR(VLAN_PAG, IF_GRP, regs.if_grp);
        JR_IS1_WR(VLAN_PAG, VLAN, regs.vlan);
        JR_IS1_WR(VLAN_PAG, FLAGS, regs.flags);
        JR_IS1_WR(VLAN_PAG, L2_MAC_ADDR_HIGH, regs.mac.mach);
        JR_IS1_WR(VLAN_PAG, L2_MAC_ADDR_LOW, regs.mac.macl);
        JR_IS1_WR(VLAN_PAG, L3_IP4_SIP, regs.sip);
        JR_IS1_WR(VLAN_PAG, L3_MISC, regs.l3_misc);
        JR_IS1_WR(VLAN_PAG, L4_MISC, regs.l4_misc);

        /* Update action */
        JR_WR(VCAP_IS1, VLAN_PAG_ACTION_PAG, 0);                                                /* TBD */
        JR_WR(VCAP_IS1, VLAN_PAG_ACTION_MISC,
              JR_PUT_BIT(VCAP_IS1, VLAN_PAG_ACTION_MISC, CUSTOM_ACE_TYPE_ENA, 0)              | /* TBD */
              JR_PUT_FLD(VCAP_IS1, VLAN_PAG_ACTION_MISC, CUSTOM_ACE_TYPE_VAL, 0)              | /* TBD */
              JR_PUT_BIT(VCAP_IS1, VLAN_PAG_ACTION_MISC, PCP_DEI_ENA, action->pcp_dei_enable) |
              JR_PUT_FLD(VCAP_IS1, VLAN_PAG_ACTION_MISC, PCP_VAL, action->pcp)                |
              JR_PUT_BIT(VCAP_IS1, VLAN_PAG_ACTION_MISC, DEI_VAL, action->dei)                |
              JR_PUT_BIT(VCAP_IS1, VLAN_PAG_ACTION_MISC, VID_REPLACE_ENA, action->vid)        | /* replace if vid != 0 */
              JR_PUT_FLD(VCAP_IS1, VLAN_PAG_ACTION_MISC, VID_ADD_VAL, action->vid));

        JR_WR(VCAP_IS1, VLAN_PAG_ACTION_CUSTOM_POS, 0);                                         /* TBD */
        JR_WR(VCAP_IS1, VLAN_PAG_ACTION_ISDX, 
              JR_PUT_FLD(VCAP_IS1, VLAN_PAG_ACTION_ISDX, ISDX_ADD_VAL, action->isdx) |
              JR_PUT_BIT(VCAP_IS1, VLAN_PAG_ACTION_ISDX, ISDX_REPLACE_ENA, action->isdx_enable));
        JR_WRB(VCAP_IS1, VLAN_PAG_ACTION_STICKY, HIT_STICKY, 0);
    }
    else { /* QOS */
        /* Update entry */
        JR_IS1_WR(QOS, ENTRY, regs.entry);
        JR_IS1_WR(QOS, IF_GRP, regs.if_grp);
        JR_IS1_WR(QOS, VLAN, regs.vlan);
        JR_IS1_WR(QOS, FLAGS, regs.flags);
        JR_IS1_WR(QOS, L2_MAC_ADDR_HIGH, regs.mac.mach);
        JR_IS1_WR(QOS, L3_IP4_SIP, regs.sip);
        JR_IS1_WR(QOS, L3_MISC, regs.l3_misc);
        JR_IS1_WR(QOS, L4_MISC, regs.l4_misc);
        JR_IS1_WR(QOS, L4_PORT, regs.l4_port);

        /* Update action */
        JR_WR(VCAP_IS1, QOS_ACTION_DSCP,
              JR_PUT_BIT(VCAP_IS1, QOS_ACTION_DSCP, DSCP_ENA, action->dscp_enable) |
              JR_PUT_FLD(VCAP_IS1, QOS_ACTION_DSCP, DSCP_VAL, action->dscp));

        JR_WR(VCAP_IS1, QOS_ACTION_QOS,
              JR_PUT_BIT(VCAP_IS1, QOS_ACTION_QOS, QOS_ENA, action->prio_enable) |
              JR_PUT_FLD(VCAP_IS1, QOS_ACTION_QOS, QOS_VAL, action->prio));

        JR_WR(VCAP_IS1, QOS_ACTION_DP,
              JR_PUT_BIT(VCAP_IS1, QOS_ACTION_DP, DP_ENA, action->dp_enable) |
              JR_PUT_FLD(VCAP_IS1, QOS_ACTION_DP, DP_VAL, action->dp));

        JR_WRB(VCAP_IS1, QOS_ACTION_STICKY, HIT_STICKY, 0);
    }
    return jr_vcap_index_command(VTSS_TCAM_IS1, ix, VTSS_TCAM_CMD_WRITE);
}

static vtss_rc jr_is1_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter)
{
    VTSS_I("row: %u", idx->row);

    /* Add to all devices */
    return jr_vcap_entry_add_chips(jr_is1_entry_add_chip, idx->row, data, counter);
}

static vtss_rc jr_is1_entry_del_chip(u32 ix)
{
    JR_WRB(VCAP_IS1, QOS_ENTRY_ENTRY, VLD, 0);
    JR_WRB(VCAP_IS1, QOS_MASK_ENTRY, VLD, 0);
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS1, ix, VTSS_TCAM_CMD_WRITE));
    return VTSS_RC_OK;
}

static vtss_rc jr_is1_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_I("row: %u", idx->row);

    /* Delete from all devices */
    return jr_vcap_entry_del_chips(jr_is1_entry_del_chip, idx->row);
}

static vtss_rc jr_is1_entry_move_chip(u32 ix, u32 count, BOOL up)
{
    JR_WR(VCAP_IS1, IS1_CONTROL_ACE_MV_CFG, 
          JR_PUT_FLD(VCAP_IS1, IS1_CONTROL_ACE_MV_CFG, ACE_MV_NUM_POS, 1) |
          JR_PUT_FLD(VCAP_IS1, IS1_CONTROL_ACE_MV_CFG, ACE_MV_SIZE, count));
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS1, ix + count - 1, 
                                  up ? VTSS_TCAM_CMD_MOVE_UP : VTSS_TCAM_CMD_MOVE_DOWN));
    return VTSS_RC_OK;
}

static vtss_rc jr_is1_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_I("row: %u, count: %u, up: %u", idx->row, count, up);

    /* Move for all devices */
    return jr_vcap_entry_move_chips(jr_is1_entry_move_chip, idx->row, count, up);
}

/* ================================================================= *
 *  IS2 functions
 * ================================================================= */

/* Get IS2 entry for chip */
static vtss_rc jr_is2_entry_get_chip(u32 ix, u32 *cnt, BOOL clear) 
{
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS2, ix, VTSS_TCAM_CMD_READ));
    JR_RD(VCAP_IS2, BASETYPE_ACTION_D, cnt);
    
    if (clear) {
        JR_WR(VCAP_IS2, BASETYPE_ACTION_D, 0);
        VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS2, ix, VTSS_TCAM_CMD_WRITE));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_is2_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_I("row: %u", idx->row);

    /* Read/clear from all devices */
    return jr_vcap_entry_get_chips(jr_is2_entry_get_chip, idx->row, counter, clear);
}

#define JR_IS2_WR_COMMON(pre, regs)                    \
{                                                      \
    JR_IS2_WR(pre, ACE_VLD, regs.vld);                 \
    JR_IS2_WR(pre, ACE_TYPE, regs.type);               \
    JR_IS2_WR(pre, ACE_IGR_PORT_MASK, regs.port_mask); \
    JR_IS2_WR(pre, ACE_PAG, regs.pag);                 \
}

#define JR_IS2_WR_IP(pre, regs)                \
{                                              \
    JR_IS2_WR(pre, ACE_L3_IP4_SIP, regs.sip);  \
    JR_IS2_WR(pre, ACE_L3_IP4_DIP, regs.dip);  \
    JR_IS2_WR(pre, ACE_L2_MISC, regs.l2_misc); \
    JR_IS2_WR(pre, ACE_L3_MISC, regs.l3_misc); \
}

static vtss_rc jr_is2_entry_add_chip(u32 ix, vtss_vcap_data_t *data, u32 counter)
{
    vtss_is2_data_t    *is2 = &data->u.is2;
    vtss_is2_entry_t   *entry = is2->entry;
    vtss_ace_t         *ace = &entry->ace;
    u32                mask, smask, dmask;
    jr_vcap_is2_regs_t regs;
    jr_vcap_reg_t      misc;
    jr_vcap_mac_t      mac;
    vtss_ace_bit_t     ttl, tcp_fin, tcp_syn, tcp_rst, tcp_psh, tcp_ack, tcp_urg;
    vtss_ace_u8_t      proto, ds;
    vtss_ace_u48_t     ip_data;
    vtss_ace_udp_tcp_t *sport, *dport, ipv6_port;
    
    mask = entry->type_mask;
    if (mask != 0) {
        /* Type mask must be updated before writing data mask */
        JR_WR(VCAP_IS2, IS2_CONTROL_ACE_UPDATE_CTRL, 
              JR_PUT_FLD(VCAP_IS2, IS2_CONTROL_ACE_UPDATE_CTRL, ACE_ENTRY_TYPE_MASK, mask));
    }
    
    memset(&regs, 0, sizeof(regs));
    JR_IS2_SET_BIT(MAC_ETYPE, ACE_VLD, VLD, regs.vld);

    /* ACE_TYPE register */
    JR_IS2_PUT_BIT(MAC_ETYPE, ACE_TYPE, FIRST, regs.type, 
                   entry->first ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_0);
    JR_IS2_PUT_BIT(MAC_ETYPE, ACE_TYPE, L2_MC, regs.type, ace->dmac_mc);
    JR_IS2_PUT_BIT(MAC_ETYPE, ACE_TYPE, L2_BC, regs.type, ace->dmac_bc);
    JR_IS2_PUT_BIT(MAC_ETYPE, ACE_TYPE, CFI, regs.type, ace->vlan.cfi);
    JR_IS2_PUT_FLD(MAC_ETYPE, ACE_TYPE, UPRIO, regs.type, ace->vlan.usr_prio);
    JR_IS2_PUT_FLD(MAC_ETYPE, ACE_TYPE, VID, regs.type, ace->vlan.vid);
    if (ace->vlan.vid.mask != 0) {
        /* VID matching used, ISDX must be zero */
        JR_IS2_CLR_BIT(MAC_ETYPE, ACE_TYPE, ISDX_NEQ0, regs.type);
    }
#if defined(VTSS_FEATURE_ACL_V2)
    JR_IS2_PUT_BIT(MAC_ETYPE, ACE_TYPE, VLAN_TAGGED, regs.type, ace->vlan.tagged);
#endif

    /* ACE_IGR_PORT_MASK */
#if defined(VTSS_FEATURE_ACL_V1)
    mask = (entry->chip_port_mask[vtss_state->chip_no] != 0 ? entry->chip_port_mask[vtss_state->chip_no] :
            ace->port_no == VTSS_PORT_NO_ANY ? jr_port_mask_from_map(entry->include_int_ports, entry->include_stack_ports) :
            ace->port_no < vtss_state->port_count && VTSS_PORT_CHIP_SELECTED(ace->port_no) ? 
            VTSS_BIT(VTSS_CHIP_PORT(ace->port_no)) : 0);
#endif
#if defined(VTSS_FEATURE_ACL_V2)
    mask = jr_port_mask(ace->port_list);
#endif
    /* Enabled ports are don't cared, disabled ports must be 0 */
    regs.port_mask.mask = ~mask;

    /* ACE_PAG */
    JR_IS2_PUT_FLD_VM(MAC_ETYPE, ACE_PAG, PAG, regs.pag, ace->policy.value, ace->policy.mask);
    
    misc.value = misc.mask = 0;
    
    switch (entry->type) {
    case IS2_ENTRY_TYPE_ETYPE:
        JR_IS2_WR_COMMON(MAC_ETYPE, regs);
        JR_IS2_WR_MAC(MAC_ETYPE, ACE_L2_SMAC, ace->frame.etype.smac);
        JR_IS2_WR_MAC(MAC_ETYPE, ACE_L2_DMAC, ace->frame.etype.dmac);
        JR_IS2_PUT_U16(MAC_ETYPE, ACE_L2_ETYPE, L2_PAYLOAD, misc, ace->frame.etype.data);
        JR_IS2_PUT_U16(MAC_ETYPE, ACE_L2_ETYPE, ETYPE, misc, ace->frame.etype.etype);
        JR_IS2_WR(MAC_ETYPE, ACE_L2_ETYPE, misc);
        break;
    case IS2_ENTRY_TYPE_LLC:
        JR_IS2_WR_COMMON(MAC_LLC, regs);
        JR_IS2_WR_MAC(MAC_LLC, ACE_L2_SMAC, ace->frame.llc.smac);
        JR_IS2_WR_MAC(MAC_LLC, ACE_L2_DMAC, ace->frame.llc.dmac);
        JR_VCAP_PUT_U32(misc, ace->frame.llc.llc);
        JR_IS2_WR(MAC_LLC, ACE_L2_LLC, misc);
        break;
    case IS2_ENTRY_TYPE_SNAP:
        JR_IS2_WR_COMMON(MAC_SNAP, regs);
        JR_VCAP_PUT_U40(mac, ace->frame.snap.snap);
        JR_IS2_WR(MAC_SNAP, ACE_L2_SNAP_HIGH, mac.mach);
        JR_IS2_WR(MAC_SNAP, ACE_L2_SNAP_LOW, mac.macl);
        break;
    case IS2_ENTRY_TYPE_ARP:
        JR_IS2_WR_COMMON(ARP, regs);
        JR_IS2_WR_MAC(ARP, ACE_L2_SMAC, ace->frame.arp.smac);
        JR_IS2_WR(ARP, ACE_L3_IP4_SIP, ace->frame.arp.sip);
        JR_IS2_WR(ARP, ACE_L3_IP4_DIP, ace->frame.arp.dip);
        JR_IS2_WR(ARP, ACE_L3_MISC, misc);
        JR_IS2_PUT_FLD_VM(ARP, ACE_L3_ARP, ARP_OPCODE, misc, 
                          ((ace->frame.arp.arp == VTSS_ACE_BIT_1 ? 0 : 1) << 1) |
                          ((ace->frame.arp.req == VTSS_ACE_BIT_1 ? 0 : 1) << 0),
                          ((ace->frame.arp.arp == VTSS_ACE_BIT_ANY ? 0 : 1) << 1) |
                          ((ace->frame.arp.req == VTSS_ACE_BIT_ANY ? 0 : 1) << 0));
        JR_IS2_PUT_BIT(ARP, ACE_L3_ARP, ARP_OPCODE_UNKNOWN, misc, ace->frame.arp.unknown);
        JR_IS2_PUT_BIT(ARP, ACE_L3_ARP, ARP_SENDER_MATCH, misc, ace->frame.arp.smac_match);
        JR_IS2_PUT_BIT(ARP, ACE_L3_ARP, ARP_TARGET_MATCH, misc, ace->frame.arp.dmac_match);
        JR_IS2_PUT_BIT(ARP, ACE_L3_ARP, ARP_LEN_OK, misc, ace->frame.arp.length);
        JR_IS2_PUT_BIT(ARP, ACE_L3_ARP, ARP_PROTO_SPACE_OK, misc, ace->frame.arp.ip);
        JR_IS2_PUT_BIT(ARP, ACE_L3_ARP, ARP_ADDR_SPACE_OK, misc, ace->frame.arp.ethernet);
        JR_IS2_WR(ARP, ACE_L3_ARP, misc);
        break;
    case IS2_ENTRY_TYPE_IP_OTHER:
        if (ace->type == VTSS_ACE_TYPE_IPV4) {
            /* IPv4 */
            regs.sip.value = ace->frame.ipv4.sip.value;
            regs.sip.mask = ace->frame.ipv4.sip.mask;
            regs.dip.value = ace->frame.ipv4.dip.value;
            regs.dip.mask = ace->frame.ipv4.dip.mask;
            JR_IS2_SET_BIT(IP_OTHER, ACE_L2_MISC, IP4, regs.l2_misc);
            proto = ace->frame.ipv4.proto;
            ds = ace->frame.ipv4.ds;
            ttl = ace->frame.ipv4.ttl;
            JR_IS2_PUT_BIT(IP_OTHER, ACE_L3_MISC, L3_FRAGMENT, regs.l3_misc, 
                           ace->frame.ipv4.fragment);
            JR_IS2_PUT_BIT(IP_OTHER, ACE_L3_MISC, L3_OPTIONS, regs.l3_misc, 
                           ace->frame.ipv4.options);
            ip_data = ace->frame.ipv4.data;
            sport = &ace->frame.ipv4.sport;
            dport = &ace->frame.ipv4.dport;
            tcp_urg = ace->frame.ipv4.tcp_urg;
            tcp_ack = ace->frame.ipv4.tcp_ack;
            tcp_psh = ace->frame.ipv4.tcp_psh;
            tcp_rst = ace->frame.ipv4.tcp_rst;
            tcp_syn = ace->frame.ipv4.tcp_syn;
            tcp_fin = ace->frame.ipv4.tcp_fin;
        } else {
            /* IPv6 or ANY_IP */
            JR_VCAP_PUT_U32_NDX(regs.sip, ace->frame.ipv6.sip, 12);        
            JR_VCAP_PUT_U32_NDX(regs.dip, ace->frame.ipv6.sip, 8);        
            if (ace->type == VTSS_ACE_TYPE_IPV6) {
                /* IPv6 (not ANY_IP) */
                JR_IS2_CLR_BIT(IP_OTHER, ACE_L2_MISC, IP4, regs.l2_misc);
            }
            proto.value = proto.mask = 0;
            ds.value = ds.mask = 0;
            ttl = VTSS_ACE_BIT_ANY;
            memset(&ip_data, 0, sizeof(ip_data));
            ipv6_port.in_range = 1;
            ipv6_port.high = 0xffff;
            ipv6_port.low = 0;
            sport = dport = &ipv6_port;
            tcp_urg = VTSS_ACE_BIT_ANY;
            tcp_ack = VTSS_ACE_BIT_ANY;
            tcp_psh = VTSS_ACE_BIT_ANY;
            tcp_rst = VTSS_ACE_BIT_ANY;
            tcp_syn = VTSS_ACE_BIT_ANY;
            tcp_fin = VTSS_ACE_BIT_ANY;
        }
        JR_IS2_PUT_FLD(IP_OTHER, ACE_L3_MISC, L3_IP_PROTO, regs.l3_misc, proto);
        JR_IS2_PUT_FLD(IP_OTHER, ACE_L3_MISC, L3_TOS, regs.l3_misc, ds);
        JR_IS2_PUT_BIT(IP_OTHER, ACE_L3_MISC, L3_TTL_GT0, regs.l3_misc, ttl);

        if (vtss_vcap_udp_tcp_rule(&proto)) {
            /* UDP/TCP */
            JR_IS2_WR_COMMON(IP_TCP_UDP, regs);
            JR_IS2_WR_IP(IP_TCP_UDP, regs);

            /* Port numbers */
            JR_IS2_PUT_FLD_VM(IP_TCP_UDP, ACE_L4_PORT, L4_SPORT, misc, 
                              sport->low, sport->low == sport->high ? 0xffff : 0);
            JR_IS2_PUT_FLD_VM(IP_TCP_UDP, ACE_L4_PORT, L4_DPORT, misc, 
                              dport->low, dport->low == dport->high ? 0xffff : 0);
            JR_IS2_WR(IP_TCP_UDP, ACE_L4_PORT, misc);

            /* Port ranges and TCP flags */
            misc.value = misc.mask = 0;
            smask = (is2->srange == VTSS_VCAP_RANGE_CHK_NONE ? 0 : (1<<is2->srange));
            dmask = (is2->drange == VTSS_VCAP_RANGE_CHK_NONE ? 0 : (1<<is2->drange));
            JR_IS2_PUT_FLD_VM(IP_TCP_UDP, ACE_L4_MISC, L4_RNG, misc, 
                              (sport->in_range ? smask : 0) | (dport->in_range ? dmask : 0), 
                              smask | dmask);
            if (proto.value == 6) {
                JR_IS2_PUT_BIT(IP_TCP_UDP, ACE_L4_MISC, L4_URG, misc, tcp_urg);
                JR_IS2_PUT_BIT(IP_TCP_UDP, ACE_L4_MISC, L4_ACK, misc, tcp_ack);
                JR_IS2_PUT_BIT(IP_TCP_UDP, ACE_L4_MISC, L4_PSH, misc, tcp_psh);
                JR_IS2_PUT_BIT(IP_TCP_UDP, ACE_L4_MISC, L4_RST, misc, tcp_rst);
                JR_IS2_PUT_BIT(IP_TCP_UDP, ACE_L4_MISC, L4_SYN, misc, tcp_syn);
                JR_IS2_PUT_BIT(IP_TCP_UDP, ACE_L4_MISC, L4_FIN, misc, tcp_fin);
            }
            JR_IS2_WR(IP_TCP_UDP, ACE_L4_MISC, misc);
        } else {
            /* Not UDP/TCP */
            JR_IS2_WR_COMMON(IP_OTHER, regs);
            JR_IS2_WR_IP(IP_OTHER, regs);
            JR_VCAP_PUT_U48(mac, ip_data);
            JR_IS2_WR(IP_OTHER, ACE_IP4_OTHER_0, mac.macl);
            JR_IS2_WR(IP_OTHER, ACE_IP4_OTHER_1, mac.mach);
        }
        break;
    case IS2_ENTRY_TYPE_OAM:
        JR_IS2_WR_COMMON(OAM, regs);
        JR_IS2_WR(OAM, ACE_OAM_0, misc);
        JR_IS2_WR(OAM, ACE_OAM_1, misc);
        break;
    default:
        VTSS_E("illegal type: %d", ace->type);
        return VTSS_RC_ERROR;
    }

    VTSS_RC(jr_is2_prepare_action(&ace->action, entry->action_ext, counter));
    return jr_vcap_index_command(VTSS_TCAM_IS2, ix, VTSS_TCAM_CMD_WRITE);
}

static vtss_rc jr_is2_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter)
{
    VTSS_I("row: %u", idx->row);

    /* Add to all devices */
    return jr_vcap_entry_add_chips(jr_is2_entry_add_chip, idx->row, data, counter);
}

static vtss_rc jr_is2_entry_del_chip(u32 ix)
{
    JR_WR(VCAP_IS2, MAC_ETYPE_ENTRY_ACE_VLD, 0);
    JR_WR(VCAP_IS2, MAC_ETYPE_MASK_ACE_VLD, 0);
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS2, ix, VTSS_TCAM_CMD_WRITE));
    return VTSS_RC_OK;
}

static vtss_rc jr_is2_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_I("row: %u", idx->row);

    /* Delete from all devices */
    return jr_vcap_entry_del_chips(jr_is2_entry_del_chip, idx->row);
}

static vtss_rc jr_is2_entry_move_chip(u32 ix, u32 count, BOOL up)
{
    JR_WR(VCAP_IS2, IS2_CONTROL_ACE_MV_CFG, 
          JR_PUT_FLD(VCAP_IS2, IS2_CONTROL_ACE_MV_CFG, ACE_MV_NUM_POS, 1) |
          JR_PUT_FLD(VCAP_IS2, IS2_CONTROL_ACE_MV_CFG, ACE_MV_SIZE, count));
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_IS2, ix + count - 1, 
                                  up ? VTSS_TCAM_CMD_MOVE_UP : VTSS_TCAM_CMD_MOVE_DOWN));
    return VTSS_RC_OK;
}

static vtss_rc jr_is2_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_I("row: %u, count: %u, up: %u", idx->row, count, up);

    /* Move for all devices */
    return jr_vcap_entry_move_chips(jr_is2_entry_move_chip, idx->row, count, up);
}

/* ================================================================= *
 *  ES0 functions
 * ================================================================= */

/* Get ES0 entry for chip */
static vtss_rc jr_es0_entry_get_chip(u32 ix, u32 *cnt, BOOL clear) 
{
    u32 type;
    
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_ES0, ix, VTSS_TCAM_CMD_READ));
    JR_RDB(VCAP_ES0, ES0_CONTROL_ACE_STATUS, ACE_ACTION_TYPE, &type);
    if (type == JR_ES0_ACTION_MACINMAC) {
        JR_RD(VCAP_ES0, MACINMAC_ACTION_C, cnt);
    } else {
        JR_RD(VCAP_ES0, TAG_ACTION_C, cnt);
    }
    
    if (clear) {
        if (type == JR_ES0_ACTION_MACINMAC) {
            JR_WR(VCAP_ES0, MACINMAC_ACTION_C, 0);
        } else {
            JR_WR(VCAP_ES0, TAG_ACTION_C, 0);
        }
        VTSS_RC(jr_vcap_index_command(VTSS_TCAM_ES0, ix, VTSS_TCAM_CMD_WRITE));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_es0_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_I("row: %u", idx->row);

    /* Read/clear from all devices */
    return jr_vcap_entry_get_chips(jr_es0_entry_get_chip, idx->row, counter, clear);
}

static vtss_rc jr_es0_entry_add_chip(u32 ix, vtss_vcap_data_t *data, u32 counter)
{
    vtss_es0_data_t   *es0 = &data->u.es0;
    vtss_es0_key_t    *key = &es0->entry->key;
    vtss_es0_action_t *action = &es0->entry->action;
    jr_vcap_reg_t     reg;
    u32               port;

    memset(&reg, 0, sizeof(reg));
    JR_ES0_SET_BIT(ISDX, A, VLD, reg);
    JR_ES0_PUT_BIT(ISDX, A, ISDX_NEQ0, reg, key->isdx_neq0);
    if (key->port_no != VTSS_PORT_NO_NONE) {
        port = (VTSS_PORT_CHIP_SELECTED(key->port_no) ? VTSS_CHIP_PORT(key->port_no) : 0xff);
        JR_ES0_PUT_FLD_VM(ISDX, A, VEGR_PORT, reg, port, 0xff);
    }
    
    switch (key->type) {
    case VTSS_ES0_TYPE_ISDX:
        JR_ES0_WR(ISDX, A, reg);
        memset(&reg, 0, sizeof(reg));
        JR_ES0_PUT_FLD_VM(ISDX, ISDX1, ISDX, reg, key->data.isdx.isdx, 0xfff);
        JR_ES0_WR(ISDX, ISDX1, reg);
        break;
    case VTSS_ES0_TYPE_VID:
        JR_ES0_WR(VID, A, reg);
        memset(&reg, 0, sizeof(reg));
        JR_ES0_PUT_FLD_VM(VID, VID1, VID, reg, key->data.vid.vid, 0xfff);
        JR_ES0_WR(VID, VID1, reg);
        break;
    default:
        VTSS_E("illegal type: %d", key->type);
        return VTSS_RC_ERROR;
    }
    
    /* Action */
    vtss_cmn_es0_action_get(es0);
    JR_WR(VCAP_ES0, TAG_ACTION_A, JR_PUT_BIT(VCAP_ES0, TAG_ACTION_A, VLD, 1));
    JR_WR(VCAP_ES0, TAG_ACTION_TAG1,
          JR_PUT_FLD(VCAP_ES0, TAG_ACTION_TAG1, VID_B_VAL, action->vid_b) |
          JR_PUT_FLD(VCAP_ES0, TAG_ACTION_TAG1, TAG_VID_SEL, 2) |
          JR_PUT_FLD(VCAP_ES0, TAG_ACTION_TAG1, TAG_TPI_SEL, action->tpid) |
          JR_PUT_FLD(VCAP_ES0, TAG_ACTION_TAG1, TAG_CTRL, action->tag));
    JR_WR(VCAP_ES0, TAG_ACTION_B,
          JR_PUT_FLD(VCAP_ES0, TAG_ACTION_B, VID_A_VAL, action->vid_a) |
          JR_PUT_FLD(VCAP_ES0, TAG_ACTION_B, ESDX_VAL, action->esdx) |
          JR_PUT_BIT(VCAP_ES0, TAG_ACTION_B, DEI_VAL, action->dei) |
          JR_PUT_FLD(VCAP_ES0, TAG_ACTION_B, PCP_VAL, action->pcp) |
          JR_PUT_FLD(VCAP_ES0, TAG_ACTION_B, QOS_SRC_SEL, action->qos));
    JR_WR(VCAP_ES0, TAG_ACTION_C, counter);

    return jr_vcap_index_command(VTSS_TCAM_ES0, ix, VTSS_TCAM_CMD_WRITE);
}

static vtss_rc jr_es0_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter)
{
    VTSS_I("row: %u", idx->row);

    /* Add to all devices */
    return jr_vcap_entry_add_chips(jr_es0_entry_add_chip, idx->row, data, counter);
}

static vtss_rc jr_es0_entry_del_chip(u32 ix)
{
    JR_WRB(VCAP_ES0, ISDX_ENTRY_A, VLD, 0);
    JR_WRB(VCAP_ES0, ISDX_MASK_A, VLD, 0);
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_ES0, ix, VTSS_TCAM_CMD_WRITE));
    return VTSS_RC_OK;
}

static vtss_rc jr_es0_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_I("row: %u", idx->row);

    /* Delete from all devices */
    return jr_vcap_entry_del_chips(jr_es0_entry_del_chip, idx->row);
}

static vtss_rc jr_es0_entry_move_chip(u32 ix, u32 count, BOOL up)
{
    JR_WR(VCAP_ES0, ES0_CONTROL_ACE_MV_CFG, 
          JR_PUT_FLD(VCAP_ES0, ES0_CONTROL_ACE_MV_CFG, ACE_MV_NUM_POS, 1) |
          JR_PUT_FLD(VCAP_ES0, ES0_CONTROL_ACE_MV_CFG, ACE_MV_SIZE, count));
    VTSS_RC(jr_vcap_index_command(VTSS_TCAM_ES0, ix + count - 1, 
                                  up ? VTSS_TCAM_CMD_MOVE_UP : VTSS_TCAM_CMD_MOVE_DOWN));
    return VTSS_RC_OK;
}

static vtss_rc jr_es0_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_I("row: %u, count: %u, up: %u", idx->row, count, up);

    /* Move for all devices */
    return jr_vcap_entry_move_chips(jr_es0_entry_move_chip, idx->row, count, up);
}

/* Update outer tag TPID for ES0 entry if VLAN port type has changed */
static vtss_rc jr_es0_entry_update(vtss_vcap_idx_t *idx, vtss_es0_data_t *es0) 
{
    vtss_chip_no_t    chip_no;
    vtss_es0_action_t *action = &es0->entry->action;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_vcap_index_command(VTSS_TCAM_ES0, idx->row, VTSS_TCAM_CMD_READ));
        if (es0->flags & VTSS_ES0_FLAG_TPID) {
            JR_WRF(VCAP_ES0, TAG_ACTION_TAG1, TAG_TPI_SEL, action->tpid);
        }
        if (es0->flags & VTSS_ES0_FLAG_QOS) {
            JR_WRF(VCAP_ES0, TAG_ACTION_B, QOS_SRC_SEL, action->qos);
            JR_WRF(VCAP_ES0, TAG_ACTION_B, PCP_VAL, action->pcp);
            JR_WRB(VCAP_ES0, TAG_ACTION_B, DEI_VAL, action->dei);
        }
        VTSS_RC(jr_vcap_index_command(VTSS_TCAM_ES0, idx->row, VTSS_TCAM_CMD_WRITE));
    }
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  LPM functions
 * ================================================================= */

enum
{
    VTSS_LPM_ACCESS_CTRL_CMD_IDLE = 0,
    VTSS_LPM_ACCESS_CTRL_CMD_READ = 1,
    VTSS_LPM_ACCESS_CTRL_CMD_WRITE = 2,
    VTSS_LPM_ACCESS_CTRL_CMD_MOVE_UP = 3,
    VTSS_LPM_ACCESS_CTRL_CMD_MOVE_DOWN = 4,
    VTSS_LPM_ACCESS_CTRL_CMD_INIT = 5,
};

enum
{
    VTSS_LPM_ACCESS_CTRL_WID_QUAD = 0,
    VTSS_LPM_ACCESS_CTRL_WID_HALF = 1,
    VTSS_LPM_ACCESS_CTRL_WID_FULL = 2,
};

enum
{
    VTSS_LPM_TYPE_INVALID  = 0,
    VTSS_LPM_TYPE_IPV4_UC = 1,
    VTSS_LPM_TYPE_IPV4_MC = 2,
    VTSS_LPM_TYPE_IPV6_UC = 3,
};

static vtss_rc jr_lpm_table_idle(void)
{
    // TODO, timeout?

    u32 cmd;
    while (1) {
        JR_RD(ANA_L3_2_LPM, ACCESS_CTRL, &cmd);
        if ((VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT & cmd) == 0)
            break;
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv4_uc_clear_chip(const u32 row)
{
    JR_WRX(ANA_L3_2_LPM, LPM_USAGE_CFG, 0,
            VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_TYPE(VTSS_LPM_TYPE_INVALID));

    JR_WR(ANA_L3_2_LPM, ACCESS_CTRL,
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(row) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_WID(
                VTSS_LPM_ACCESS_CTRL_WID_QUAD) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                VTSS_LPM_ACCESS_CTRL_CMD_WRITE) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT
         );

    VTSS_RC(jr_lpm_table_idle());

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv4_uc_clear(const u32 row)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_ipv4_uc_clear_chip(row));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv4_mc_clear_chip(const u32 row)
{
    JR_WRX(ANA_L3_2_LPM, LPM_USAGE_CFG, 0,
            VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_TYPE(VTSS_LPM_TYPE_INVALID));

    JR_WR(ANA_L3_2_LPM, ACCESS_CTRL,
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(row) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_WID(
                VTSS_LPM_ACCESS_CTRL_WID_HALF) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                VTSS_LPM_ACCESS_CTRL_CMD_WRITE) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT
         );

    VTSS_RC(jr_lpm_table_idle());

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv4_mc_clear(const u32 row)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_ipv4_mc_clear_chip(row));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv6_uc_clear_chip(const u32 row)
{
    JR_WRX(ANA_L3_2_LPM, LPM_USAGE_CFG, 0,
            VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_TYPE(VTSS_LPM_TYPE_INVALID));

    JR_WR(ANA_L3_2_LPM, ACCESS_CTRL,
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(row) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_WID(
                VTSS_LPM_ACCESS_CTRL_WID_FULL) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                VTSS_LPM_ACCESS_CTRL_CMD_WRITE) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT
         );

    VTSS_RC(jr_lpm_table_idle());

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv6_uc_clear(const u32 row)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_ipv6_uc_clear_chip(row));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv4_uc_set_chip(const u32 row, const u32 remap_ptr,
        const BOOL ecmp, const u32 address, const u32 netmask)
{
    JR_WRX(ANA_L3_2_LPM, LPM_DATA_CFG, 0, address);
    JR_WRX(ANA_L3_2_LPM, LPM_MASK_CFG, 0, ~netmask); // netmask must be inverted in the TCAM
    JR_WR(ANA_L3_2_REMAP, REMAP_CFG,
            (ecmp ? VTSS_F_ANA_L3_2_REMAP_REMAP_CFG_ECMP_CNT : 0 ) |
            VTSS_F_ANA_L3_2_REMAP_REMAP_CFG_BASE_PTR(remap_ptr)
         );
    JR_WRX(ANA_L3_2_LPM, LPM_USAGE_CFG, 0,
            VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_DST_FLAG_DATA |
            VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_TYPE(
                VTSS_LPM_TYPE_IPV4_UC)
          );
    JR_WR(ANA_L3_2_LPM, ACCESS_CTRL,
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(row) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_WID(
                VTSS_LPM_ACCESS_CTRL_WID_QUAD) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                VTSS_LPM_ACCESS_CTRL_CMD_WRITE) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT
         );

    VTSS_RC(jr_lpm_table_idle());

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv4_uc_set(const u32 row, const u32 remap_ptr,
        const BOOL ecmp, const u32 address, const u32 netmask)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_ipv4_uc_set_chip(
                    row, remap_ptr, ecmp, address, netmask));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv6_uc_set_chip(const u32 row, const u32 remap_ptr,
        const BOOL ecmp, const u32 a0, const u32 a1, const u32 a2,
        const u32 a3, const u32 n0, const u32 n1, const u32 n2, const u32 n3 )
{
    int i;

    JR_WRX(ANA_L3_2_LPM, LPM_DATA_CFG, 0, a3);
    JR_WRX(ANA_L3_2_LPM, LPM_DATA_CFG, 1, a2);
    JR_WRX(ANA_L3_2_LPM, LPM_DATA_CFG, 2, a1);
    JR_WRX(ANA_L3_2_LPM, LPM_DATA_CFG, 3, a0);
    JR_WRX(ANA_L3_2_LPM, LPM_MASK_CFG, 0, ~n3); // netmask must be inverted in the TCAM
    JR_WRX(ANA_L3_2_LPM, LPM_MASK_CFG, 1, ~n2);
    JR_WRX(ANA_L3_2_LPM, LPM_MASK_CFG, 2, ~n1);
    JR_WRX(ANA_L3_2_LPM, LPM_MASK_CFG, 3, ~n0);

    JR_WR(ANA_L3_2_REMAP, REMAP_CFG,
            (ecmp ? VTSS_F_ANA_L3_2_REMAP_REMAP_CFG_ECMP_CNT : 0 ) |
            VTSS_F_ANA_L3_2_REMAP_REMAP_CFG_BASE_PTR(remap_ptr)
         );

    for (i = 0; i < 4; i++) {
        JR_WRX(ANA_L3_2_LPM, LPM_USAGE_CFG, i,
                VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_DST_FLAG_DATA |
                VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_TYPE(
                    VTSS_LPM_TYPE_IPV6_UC)
              );
    }

    JR_WR(ANA_L3_2_LPM, ACCESS_CTRL,
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(row) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_WID(
                VTSS_LPM_ACCESS_CTRL_WID_FULL) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                VTSS_LPM_ACCESS_CTRL_CMD_WRITE) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT
         );

    VTSS_RC(jr_lpm_table_idle());

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_ipv6_uc_set(const u32 row, const u32 remap_ptr,
        const BOOL ecmp, const u32 a0, const u32 a1, const u32 a2,
        const u32 a3, const u32 n0, const u32 n1, const u32 n2, const u32 n3 )
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_ipv6_uc_set_chip(
                    row, remap_ptr, ecmp, a0, a1, a2, a3, n0, n1, n2, n3));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_lpm_move_chip(const u32 idx, const u32 size,
        const u32 distance, const BOOL down)
{
    u32 tmp = 0;

    VTSS_D("idx=%u, n=%u, distance=%u, down=%hhu", idx, size, distance, down);

    JR_WR(ANA_L3_2_LPM, ACCESS_MV_CFG,
            VTSS_F_ANA_L3_2_LPM_ACCESS_MV_CFG_MV_NUM_POS(distance) |
            VTSS_F_ANA_L3_2_LPM_ACCESS_MV_CFG_MV_SIZE(size));

    tmp = VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(idx) |
        VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT;

    /* Invert direction */
    if( down )
        tmp |= VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                VTSS_LPM_ACCESS_CTRL_CMD_MOVE_UP);
    else
        tmp |= VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                VTSS_LPM_ACCESS_CTRL_CMD_MOVE_DOWN);

    JR_WR(ANA_L3_2_LPM, ACCESS_CTRL, tmp);
    VTSS_RC(jr_lpm_table_idle());

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_lpm_move(const u32 idx, const u32 size,
        const u32 distance, const BOOL down)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_lpm_move_chip(idx, size, distance, down));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_debug_sticky_clear_chip(void)
{
    JR_WR(ANA_L3_2, LPM_REMAP_STICKY_L3_LPM_REMAP_STICKY, 0xffffffff);
    JR_WR(ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, 0xffffffff);
    JR_WR(ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, 0xffffffff);
    return VTSS_RC_OK;
}

static vtss_rc jr_l3_debug_sticky_clear(void)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_l3_debug_sticky_clear_chip());
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_arp_set_chip( const u32 row,
        const vtss_mac_t * const mac, const vtss_vid_t evid)
{
    // We can not do atomic update, we msut therefor perform the
    // following steps:
    //  - invalidate the entry by update reg 0
    //  - write valid information to reg 1
    //  - write valid information to reg 0, and enable the entry
    JR_WRX(ANA_L3_2_ARP, ARP_CFG_0, row, 0);
    JR_WRX(ANA_L3_2_ARP, ARP_CFG_1, row,
            ( (mac->addr[2] << 24) | (mac->addr[3] << 16) |
              (mac->addr[4] <<  8) |  mac->addr[5]) );

    JR_WRX(ANA_L3_2_ARP, ARP_CFG_0, row,
            VTSS_F_ANA_L3_2_ARP_ARP_CFG_0_MAC_MSB(
                (mac->addr[0] << 8) | mac->addr[1] ) |
            VTSS_F_ANA_L3_2_ARP_ARP_CFG_0_ARP_VMID(evid) |
            VTSS_F_ANA_L3_2_ARP_ARP_CFG_0_ARP_ENA);

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_arp_set( const u32 row, const vtss_mac_t * const mac,
                              const vtss_vid_t evid)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_arp_set_chip(row, mac, evid));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_arp_clear_chip( const u32 row )
{
    JR_WRX(ANA_L3_2_ARP, ARP_CFG_0, row, 0);
    JR_WRX(ANA_L3_2_ARP, ARP_CFG_1, row, 0);

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_arp_clear( const u32 row )
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_arp_clear_chip(row));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_common_set_chip(const vtss_l3_common_conf_t * const conf )
{
    u32 lsb = 0;
    u32 msb = 0;

    if( conf->rleg_mode != VTSS_ROUTING_RLEG_MAC_MODE_SINGLE ) {
        return VTSS_RC_ERROR;
    }

    // NOTE: we do not use the routing_enable flag here, as the LPM table is
    // used even when routing are disabled.

    msb <<= 8; msb |= (u32)conf->base_address.addr[0];
    msb <<= 8; msb |= (u32)conf->base_address.addr[1];
    msb <<= 8; msb |= (u32)conf->base_address.addr[2];
    lsb <<= 8; lsb |= (u32)conf->base_address.addr[3];
    lsb <<= 8; lsb |= (u32)conf->base_address.addr[4];
    lsb <<= 8; lsb |= (u32)conf->base_address.addr[5];

    JR_WR(ANA_L3_2_COMMON, RLEG_CFG_0,
          VTSS_F_ANA_L3_2_COMMON_RLEG_CFG_0_RLEG_MAC_LSB( lsb ));
    JR_WR(REW_COMMON, RLEG_CFG_0,
          VTSS_F_REW_COMMON_RLEG_CFG_0_RLEG_MAC_LSB( lsb ));

#define RLEG_MAC_TYPE_SEL 2
    JR_WR(ANA_L3_2_COMMON, RLEG_CFG_1,
          VTSS_F_ANA_L3_2_COMMON_RLEG_CFG_1_RLEG_MAC_TYPE_SEL(RLEG_MAC_TYPE_SEL) |
          VTSS_F_ANA_L3_2_COMMON_RLEG_CFG_1_RLEG_MAC_MSB(msb));
    JR_WR(REW_COMMON, RLEG_CFG_1,
          VTSS_F_REW_COMMON_RLEG_CFG_1_RLEG_MAC_TYPE_SEL(RLEG_MAC_TYPE_SEL) |
          VTSS_F_REW_COMMON_RLEG_CFG_1_RLEG_MAC_MSB(msb));
#undef RLEG_MAC_TYPE_SEL

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_common_set(const vtss_l3_common_conf_t * const conf)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_common_set_chip(conf));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_vlan_set_chip(const vtss_l3_rleg_id_t rleg_id,
                                   const vtss_vid_t        vlan,
                                   const BOOL              enable)
{
    if (enable) {
        VTSS_DG(VTSS_TRACE_GROUP_L3, "Enable rleg_id %u, vlan %u", rleg_id, vlan);
        JR_WRX(ANA_L3_2_VLAN, VMID_CFG, vlan,
                VTSS_F_ANA_L3_2_VLAN_VMID_CFG_VMID(rleg_id));
        JR_WRXM_SET(ANA_L3_2_VLAN, VLAN_CFG, vlan,
                VTSS_F_ANA_L3_2_VLAN_VLAN_CFG_VLAN_RLEG_ENA);
    } else {
        VTSS_DG(VTSS_TRACE_GROUP_L3, "disable rleg_id %u, vlan %u", rleg_id, vlan);
        JR_WRXM_CLR(ANA_L3_2_VLAN, VLAN_CFG, vlan,
                VTSS_F_ANA_L3_2_VLAN_VLAN_CFG_VLAN_RLEG_ENA);
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_vlan_set(const vtss_l3_rleg_id_t rleg_id,
                              const vtss_vid_t        vlan,
                              const BOOL              enable)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_l3_vlan_set_chip(rleg_id, vlan, enable));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_rleg_set_chip(const vtss_l3_rleg_id_t   rleg,
                                   const vtss_l3_rleg_conf_t *const conf )
{
    u32 tmp = 0;

#define COND_APPEND(x, y, z) \
    x |= conf-> y ? VTSS_F_ANA_L3_2_VMID_RLEG_CTRL_RLEG_## z : 0;
    COND_APPEND(tmp, ipv4_unicast_enable, IP4_UC_ENA);
    COND_APPEND(tmp, ipv6_unicast_enable, IP6_UC_ENA);
    COND_APPEND(tmp, ipv4_icmp_redirect_enable, IP4_ICMP_REDIR_ENA);
    COND_APPEND(tmp, ipv6_icmp_redirect_enable, IP6_ICMP_REDIR_ENA);
#undef COND_APPEND
    tmp |= VTSS_F_ANA_L3_2_VMID_RLEG_CTRL_RLEG_EVID(conf->vlan);

    JR_WRX(ANA_L3_2_VMID, RLEG_CTRL, rleg, tmp);

    JR_WRX(REW, VMID_RLEG_CTRL, rleg,
           VTSS_F_REW_VMID_RLEG_CTRL_RLEG_EVID(conf->vlan));

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_rleg_set(const vtss_l3_rleg_id_t   rleg,
                              const vtss_l3_rleg_conf_t * const conf)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_rleg_set_chip(rleg, conf));
    }

    return VTSS_RC_OK;
}

enum {
    VTSS_RLEG_EVENT_MASK_ACL_DIS = VTSS_BIT(0),
    VTSS_RLEG_EVENT_MASK_TOTAL = VTSS_BIT(1),
    VTSS_RLEG_EVENT_MASK_VLAN_DIS = VTSS_BIT(2),
    VTSS_RLEG_EVENT_MASK_IPV4_UC = VTSS_BIT(3),
    VTSS_RLEG_EVENT_MASK_IPV6_UC = VTSS_BIT(4),
    VTSS_RLEG_EVENT_MASK_IP_MC = VTSS_BIT(5),
    VTSS_RLEG_EVENT_MASK_IP_MC_DIS = VTSS_BIT(6),
    VTSS_RLEG_EVENT_MASK_TTS_DIS = VTSS_BIT(7),
};

enum {
    VTSS_RLEG_FRAME_CNT = 0,
    VTSS_RLEG_BYTE_CNT = 1,
};

/* L3 IRLEG COUNTER CONFIGURATION */
//                         IDX TRIGGER-MASK                    COUNTER TYPE         DESTINATION
#define VTSS_L3_IRLEG_CNT0 (0, VTSS_RLEG_EVENT_MASK_IPV4_UC,   VTSS_RLEG_FRAME_CNT, ipv4uc_received_frames)
#define VTSS_L3_IRLEG_CNT1 (1, VTSS_RLEG_EVENT_MASK_IPV4_UC,   VTSS_RLEG_BYTE_CNT,  ipv4uc_received_octets)
#define VTSS_L3_IRLEG_CNT2 (2, VTSS_RLEG_EVENT_MASK_IPV6_UC,   VTSS_RLEG_FRAME_CNT, ipv6uc_received_frames)
#define VTSS_L3_IRLEG_CNT3 (3, VTSS_RLEG_EVENT_MASK_IPV6_UC,   VTSS_RLEG_BYTE_CNT,  ipv6uc_received_octets)
//#define VTSS_L3_IRLEG_CNT4
//#define VTSS_L3_IRLEG_CNT5

/* L3 ERLEG COUNTER CONFIGURATION */
//                         IDX TRIGGER-MASK                    COUNTER TYPE         DESTINATION
#define VTSS_L3_ERLEG_CNT0 (0, VTSS_RLEG_EVENT_MASK_IPV4_UC,   VTSS_RLEG_FRAME_CNT, ipv4uc_transmitted_frames)
#define VTSS_L3_ERLEG_CNT1 (1, VTSS_RLEG_EVENT_MASK_IPV4_UC,   VTSS_RLEG_BYTE_CNT,  ipv4uc_transmitted_octets)
#define VTSS_L3_ERLEG_CNT2 (2, VTSS_RLEG_EVENT_MASK_IPV6_UC,   VTSS_RLEG_FRAME_CNT, ipv6uc_transmitted_frames)
#define VTSS_L3_ERLEG_CNT3 (3, VTSS_RLEG_EVENT_MASK_IPV6_UC,   VTSS_RLEG_BYTE_CNT,  ipv6uc_transmitted_octets)
//#define VTSS_L3_ERLEG_CNT4
//#define VTSS_L3_ERLEG_CNT5

static vtss_rc jr_l3_rleg_hw_stat_poll_chip(vtss_l3_rleg_id_t rleg,
                                            vtss_l3_counters_t *const stat)
{
#define CNT_POLL(idx, direction, dest)                                  \
    {                                                                   \
        u64 val = 0;                                                    \
        u32 msb = 0, msb2 = 0, lsb = 0;                                 \
        JR_RDXY(ANA_AC, STAT_CNT_CFG_##direction##_STAT_MSB_CNT,        \
                rleg, idx, &msb);                                       \
        JR_RDXY(ANA_AC, STAT_CNT_CFG_##direction##_STAT_LSB_CNT,        \
                rleg, idx, &lsb);                                       \
        JR_RDXY(ANA_AC, STAT_CNT_CFG_##direction##_STAT_MSB_CNT,        \
                rleg, idx, &msb2);                                      \
        if( msb != msb2 ) /* detect lsb wrap around */                  \
            lsb = 0;                                                    \
        val = msb2;                                                     \
        val <<= 32;                                                     \
        val |= (u64)lsb;                                                \
        stat -> dest += val;                                            \
    }

#define CNT_POLL_IRLEG(T) CNT_POLL_IRLEG_ T
#define CNT_POLL_IRLEG_(idx, mask, cnt_type, dest) \
    CNT_POLL(idx, IRLEG, dest)
    CNT_POLL_IRLEG(VTSS_L3_IRLEG_CNT0);
    CNT_POLL_IRLEG(VTSS_L3_IRLEG_CNT1);
    CNT_POLL_IRLEG(VTSS_L3_IRLEG_CNT2);
    CNT_POLL_IRLEG(VTSS_L3_IRLEG_CNT3);
    //CNT_POLL_IRLEG(VTSS_L3_IRLEG_CNT4);
    //CNT_POLL_IRLEG(VTSS_L3_IRLEG_CNT5);
#undef CNT_POLL_IRLEG
#undef CNT_POLL_IRLEG_

#define CNT_POLL_ERLEG(T) CNT_POLL_ERLEG_ T
#define CNT_POLL_ERLEG_(idx, mask, cnt_type, dest) \
    CNT_POLL(idx, ERLEG, dest)
    CNT_POLL_ERLEG(VTSS_L3_ERLEG_CNT0);
    CNT_POLL_ERLEG(VTSS_L3_ERLEG_CNT1);
    CNT_POLL_ERLEG(VTSS_L3_ERLEG_CNT2);
    CNT_POLL_ERLEG(VTSS_L3_ERLEG_CNT3);
    //CNT_POLL_ERLEG(VTSS_L3_ERLEG_CNT4);
    //CNT_POLL_ERLEG(VTSS_L3_ERLEG_CNT5);
#undef CNT_POLL_ERLEG
#undef CNT_POLL_ERLEG_

#undef CNT_POLL

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_rleg_stat_reset_chip(void)
{
    JR_WR(ANA_AC, STAT_GLOBAL_CFG_ERLEG_STAT_RESET, 1);
    JR_WR(ANA_AC, STAT_GLOBAL_CFG_IRLEG_STAT_RESET, 1);

    JR_POLL_BIT(ANA_AC, STAT_GLOBAL_CFG_ERLEG_STAT_RESET, RESET);
    JR_POLL_BIT(ANA_AC, STAT_GLOBAL_CFG_IRLEG_STAT_RESET, RESET);

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_rleg_stat_reset(void)
{
    vtss_chip_no_t chip_no;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_rleg_stat_reset_chip());
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_l3_rleg_hw_stat_poll(const vtss_l3_rleg_id_t rleg,
                                       vtss_l3_counters_t      *const stat )
{
    vtss_chip_no_t chip_no;
    memset(stat, 0, sizeof(vtss_l3_counters_t));

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        VTSS_RC(jr_l3_rleg_hw_stat_poll_chip(rleg, stat));
    }

    return VTSS_RC_OK;
}

#if defined(VTSS_SW_OPTION_L3RT)
static vtss_rc jr_l3_rleg_stat_poll(void)
{
    u32 i;
    u32 bad_cnt = 0;
    u32 good_cnt = 0;
    u32 incomplete_cnt = 0;

#define CNT_MAX 0x000000FFFFFFFFFFLLU
#define CNT_VAL(NEW_VAL, OLD_VAL)                            \
        (                                                    \
           (NEW_VAL) >= (OLD_VAL) ?                          \
              ((NEW_VAL) - (OLD_VAL))                        \
           :                                                 \
              (((CNT_MAX) - (NEW_VAL)) + (OLD_VAL) + (1LLU)) \
        )

#define CNT_UPDATE(DST, SAMPLE, SHADOW)                      \
    (DST) = CNT_VAL((SAMPLE), (SHADOW));                     \
    (SHADOW) = (SAMPLE);

#define CNT_ACC(DST_A, DST_B, SAMPLE, SHADOW, FIELD)         \
    {                                                        \
        unsigned long long d = 0;                            \
        CNT_UPDATE(d, (SAMPLE).FIELD, (SHADOW).FIELD);       \
        DST_A.FIELD += d;                                    \
        DST_B.FIELD += d;                                    \
    }

    for (i = 0; i < VTSS_RLEG_CNT; ++i) {
        vtss_l3_counters_t stat_sample;
        vtss_rc rc = jr_l3_rleg_hw_stat_poll(i, &stat_sample);

        switch( rc ) {
        case VTSS_RC_OK:
            good_cnt++;
            break;

        case VTSS_RC_ERROR:
            bad_cnt++;
            break;

        default:
            incomplete_cnt++;
            break;
        }

        if( rc != VTSS_RC_OK )
            continue;

#define ACC(I, F)                                                    \
    CNT_ACC(vtss_state->l3.statistics.interface_counter[(I)],        \
            vtss_state->l3.statistics.system_counter,                \
            stat_sample,                                             \
            vtss_state->l3.statistics.interface_shadow_counter[(I)], \
            F)
        ACC(i, ipv4uc_received_octets);
        ACC(i, ipv4uc_received_frames);
        ACC(i, ipv6uc_received_octets);
        ACC(i, ipv6uc_received_frames);
        ACC(i, ipv4uc_transmitted_octets);
        ACC(i, ipv4uc_transmitted_frames);
        ACC(i, ipv6uc_transmitted_octets);
        ACC(i, ipv6uc_transmitted_frames);
#undef ACC
    }

#undef CNT_ACC
#undef CNT_UPDATE
#undef CNT_VAL
#undef CNT_MAX

    if (bad_cnt == 0 && incomplete_cnt == 0)
        return VTSS_RC_OK;

    if (incomplete_cnt == 0 && good_cnt == 0)
        return VTSS_RC_ERROR;

    return VTSS_RC_INCOMPLETE;
}
#endif /* VTSS_SW_OPTION_L3RT) */

static vtss_rc jr_lpm_l3_chip( void )
{
    /* Start initializing of the LPM TMAM */
    JR_WR(ANA_L3_2_TCAM_BIST, TCAM_CTRL,
            VTSS_F_ANA_L3_2_TCAM_BIST_TCAM_CTRL_TCAM_BIST |
            VTSS_F_ANA_L3_2_TCAM_BIST_TCAM_CTRL_TCAM_INIT );

    /* Start initializing of arp, ipmc and vmid */
#define INIT_MASK \
    ( VTSS_F_ANA_L3_2_COMMON_TABLE_CTRL_INIT_ARP_SHOT  | \
      VTSS_F_ANA_L3_2_COMMON_TABLE_CTRL_INIT_IPMC_SHOT | \
      VTSS_F_ANA_L3_2_COMMON_TABLE_CTRL_INIT_VMID_SHOT )
    JR_WR(ANA_L3_2_COMMON, TABLE_CTRL, INIT_MASK);

    /* Configure the 6 erleg counters and the 6 irleg counters.
     *
     * To change the counter configuration, change the tuples
     * VTSS_L3_ERLEG_CNT0-5 and VTSS_L3_IRLEG_CNT0-5 instead */
#define CNT_CONF_IRLEG(T) CNT_CONF_IRLEG_ T
#define CNT_CONF_IRLEG_(idx, mask, cnt_type, dest) \
    JR_WRX(ANA_AC, STAT_GLOBAL_CFG_IRLEG_STAT_GLOBAL_CFG, idx, cnt_type); \
    JR_WRX(ANA_AC, STAT_GLOBAL_CFG_IRLEG_STAT_GLOBAL_EVENT_MASK, idx, mask);

#define CNT_CONF_ERLEG(T) CNT_CONF_ERLEG_ T
#define CNT_CONF_ERLEG_(idx, mask, cnt_type, dest) \
    JR_WRX(ANA_AC, STAT_GLOBAL_CFG_ERLEG_STAT_GLOBAL_CFG, idx, cnt_type); \
    JR_WRX(ANA_AC, STAT_GLOBAL_CFG_ERLEG_STAT_GLOBAL_EVENT_MASK, idx, mask);

    CNT_CONF_IRLEG(VTSS_L3_IRLEG_CNT0);
    CNT_CONF_IRLEG(VTSS_L3_IRLEG_CNT1);
    CNT_CONF_IRLEG(VTSS_L3_IRLEG_CNT2);
    CNT_CONF_IRLEG(VTSS_L3_IRLEG_CNT3);
    //CNT_CONF_IRLEG(VTSS_L3_IRLEG_CNT4);
    //CNT_CONF_IRLEG(VTSS_L3_IRLEG_CNT5);
    CNT_CONF_ERLEG(VTSS_L3_ERLEG_CNT0);
    CNT_CONF_ERLEG(VTSS_L3_ERLEG_CNT1);
    CNT_CONF_ERLEG(VTSS_L3_ERLEG_CNT2);
    CNT_CONF_ERLEG(VTSS_L3_ERLEG_CNT3);
    //CNT_CONF_ERLEG(VTSS_L3_ERLEG_CNT4);
    //CNT_CONF_ERLEG(VTSS_L3_ERLEG_CNT5);
    /* Counter configuration is completed */

    // Reset counters after setup
    VTSS_RC(jr_l3_rleg_stat_reset_chip());

    // Forward all router errors to CPU
    JR_WR(ANA_L3_2_COMMON, ROUTING_CFG,
          VTSS_F_ANA_L3_2_COMMON_ROUTING_CFG_ALWAYS_SWITCHED_COPY_ENA |
          VTSS_F_ANA_L3_2_COMMON_ROUTING_CFG_DIP_ADDR_VIOLATION_REDIR_ENA(15) |
          VTSS_F_ANA_L3_2_COMMON_ROUTING_CFG_SIP_ADDR_VIOLATION_REDIR_ENA(7) |
          VTSS_F_ANA_L3_2_COMMON_ROUTING_CFG_CPU_RLEG_IP_HDR_FAIL_REDIR_ENA |
          VTSS_F_ANA_L3_2_COMMON_ROUTING_CFG_CPU_IP6_HOPBYHOP_REDIR_ENA |
          VTSS_F_ANA_L3_2_COMMON_ROUTING_CFG_CPU_IP4_OPTIONS_REDIR_ENA |
          VTSS_F_ANA_L3_2_COMMON_ROUTING_CFG_IP6_HC_REDIR_ENA |
          VTSS_F_ANA_L3_2_COMMON_ROUTING_CFG_IP4_TTL_REDIR_ENA
    );

    // Routing related stacking settings. No sideeffects when not used in stack
    JR_WRM_SET(ANA_AC_PS_COMMON, COMMON_VSTAX_CFG,
               VTSS_F_ANA_AC_PS_COMMON_COMMON_VSTAX_CFG_VSTAX2_RT_MC_SRC_LRN_SKIP_ENA |
               VTSS_F_ANA_AC_PS_COMMON_COMMON_VSTAX_CFG_VSTAX2_RT_UC_SRC_LRN_SKIP_ENA |
               VTSS_F_ANA_AC_PS_COMMON_COMMON_VSTAX_CFG_VSTAX2_MC_ROUTE_TO_STACK_LINK_ENA(0)
    );

    { /* wait for TCAM init to complete */
        u32 clear_bit = 0;
        while(1) {
            JR_RD(ANA_L3_2_TCAM_BIST, TCAM_CTRL, &clear_bit);
            if( clear_bit == 0 )
                break;
        }
    }

    { /* wait for arp, ipmc and vmid */
        u32 clear_bit = 0;

        while(1){
            JR_RD(ANA_L3_2_COMMON, TABLE_CTRL, &clear_bit);
            if( (clear_bit & INIT_MASK) == 0 )
                break;
        }
    }

#undef INIT_MASK
#undef CNT_CONF_IRLEG
#undef CNT_CONF_ERLEG
#undef CNT_CONF_IRLEG_
#undef CNT_CONF_ERLEG_

    return VTSS_RC_OK;
}

static vtss_rc jr_routing_uc_global_enable_chip(void)
{
    // Enable routing on all ports, including stack ports.
    // The reason for enabling routing on stack ports as well
    // is that a bug in Jaguar causes frames that are H/W-forwarded
    // from CPU Rx queues on one unit towards CPU Rx
    // queues on another unit to otherwise hit the same CPU Rx queue,
    // making it impossible to prioritize traffic on the receiving
    // unit. By enabling routing on stack ports, frames not already
    // routed will be redirected to CPU Rx queues according to
    // the ANA_L3:COMMON:CPU_QU_CFG register. In principle,
    // we could live with only enabling routing on the master unit's
    // stack ports, but doing it on all units simplifies the implementation.
    // A draw-back of enabling routing on even only the master's stack ports
    // is if a front port is *not* enabled for routing,
    // then frames subject to routing (i.e. with DMAC = L3 base address),
    // might get forwarded to stack links as a result of normal bridging,
    // and get routed whenever it hits a stack port. But in our current
    // implementation, routing is always enabled on all front ports as well.
    // To overcome the problem, an IS2 rule could be implemented on ports
    // not enabled for routing.
    u32 portmask = 0xffffffff;
    JR_WR(ANA_L3_2_COMMON, L3_UC_ENA, portmask);
    return VTSS_RC_OK;
}

static vtss_rc jr_init_l3_chip(void)
{
    VTSS_RC(jr_lpm_l3_chip());
    VTSS_RC(jr_routing_uc_global_enable_chip());

    return VTSS_RC_OK;
}


/* ================================================================= *
 *  Layer 2
 * ================================================================= */

/* Wait until the MAC operation is finsished */
/* Polling method is used, the sticky interrupt is not used */
static vtss_rc jr_mac_table_idle(void)
{
    u32 cmd;    
    while (1) {
        JR_RD(LRN_COMMON, COMMON_ACCESS_CTRL, &cmd); 

        if ((VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_MAC_TABLE_ACCESS_SHOT & cmd) == 
            0)
            break;
    }
    return VTSS_RC_OK;
}
 
 
/*******************************************************************************
 * Add/delete/get/get_next MAC address entry. 
 *
 * involved registers:
 *  MAC_ACCESS_CFG_0
 *  MAC_ACCESS_CFG_1
 *  MAC_ACCESS_CFG_2
 *  MAC_ACCESS_CFG_3
 *  COMMON_ACCESS_CTRL
 *
 ******************************************************************************/
/*
 * Add MAC address entry. 
 *
 * \param entry [IN]  MAC address entry structure.
 * \param pgid [IN]   Multicast group ID: = VTSS_PGID_NONE if IP4/IP6 multicast address
 *                      = index into the vtss_state->pgid_table, which holds a portmask for the entry.
 *                        valid index in Jaguar: [0..511] 
 * \return Return code.
 **/
static vtss_rc jr_mac_table_add_chip(const vtss_mac_table_entry_t *const entry, u32 pgid)
{
    u32 mach, macl, mask = 0, idx = 0, aged = 0, fwd_kill = 0, cpu_copy = entry->copy_to_cpu;
    u32 entry_type = MAC_ENTRY_TYPE_STD_ENTRY;
    u32 addr_type = MAC_ENTRY_ADDR_TYPE_UPSID_PN;
    
    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
   
    if (pgid == VTSS_PGID_NONE) {
        /* IPv4/IPv6 multicast entry */
        entry_type = (entry->vid_mac.mac.addr[0] == 0x01 ? MAC_ENTRY_TYPE_IPV4_MC_ENTRY :
                      MAC_ENTRY_TYPE_IPV6_MC_ENTRY);
        
        mask = jr_port_mask(vtss_state->pgid_table[pgid].member); 
        if (vtss_state->chip_count == 2) {
            /* Include internal ports in mask */
            mask |= vtss_state->mask_int_ports;
        }
    } else {
        /* Not IP multicast entry */
        /* Set FWD_KILL to make the switch discard frames in SMAC lookup */
        fwd_kill = (cpu_copy || (pgid != vtss_state->pgid_drop) ? 0 : 1); /* TBD if needed in Jaguar */
        idx = jr_chip_pgid(pgid);

        if (idx >= VTSS_CHIP_PORTS) {
            /* Multicast PGID */
            addr_type = MAC_ENTRY_ADDR_TYPE_MC_IDX;
            idx -= VTSS_CHIP_PORTS;
        } else {
#if defined(VTSS_FEATURE_VSTAX_V2)
            idx += (vtss_state->vstax_info.upsid[VTSS_CHIP_NO(pgid)]<<5);
#endif /* VTSS_FEATURE_VSTAX_V2 */
        }
    }

    VTSS_D("address 0x%x  0x%x", mach, macl);

    /* Insert/learn new entry into the MAC table  */
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_0, mach);
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_1, macl);

    // Due to a chip-bug in Jaguar (see Bugzilla#7730), the CPU must do S/W-assisted
    // H/W-based learning in a stack. For this to work, the FDMA driver re-injects all
    // learn-frames on the stack links.
    // To overcome another chip-bug (see Bugzilla#9545), the DMAC of the re-injected
    // learn-all frame must not match an entry in the MAC table with the CPU-copy
    // flag set, because that would cause the frame do get to the CPU multiple times on
    // the unit(s) that has the CPU-copy flag set in the MAC table. The entry may, however,
    // match an entry in the MAC table with a pointer to a PGID entry that has the CPU copy
    // flag set (this will not give rise to multiple copied).
    // Therefore, the FDMA driver and this piece of code has a silent agreement on a common
    // MAC address that will never have the CPU copy flag MAC address set (but use the PGID
    // table instead if needed).
    // Any MAC address can be used, but for simplicity, we use the broadcast MAC address,
    // and rely on the fact that this gets copied to the CPU through the PGID table, if needed.
    if (cpu_copy && VTSS_MAC_BC(entry->vid_mac.mac.addr)) {
        cpu_copy = 0;
    }

    /* MAC_ENTRY_VLAN_IGNORE is set to 0 */
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_2, 
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR(idx) |
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR_TYPE(addr_type) |
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_TYPE(entry_type) |
          (fwd_kill ? VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_SRC_KILL_FWD : 0) |
          (cpu_copy ? VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_CPU_COPY : 0) |
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_AGE_FLAG(aged) |
          (entry->locked ? VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_LOCKED : 0) |
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_VLD);
    {
        u32 cfg2;
        JR_RD(LRN_COMMON, MAC_ACCESS_CFG_2, &cfg2);
        VTSS_D("cfg2: 0x%08x", cfg2);
    }

    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_3, mask);

    JR_WR(LRN_COMMON, COMMON_ACCESS_CTRL, 
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_CPU_ACCESS_CMD(MAC_CMD_LEARN) |
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_MAC_TABLE_ACCESS_SHOT);

    /* Wait until MAC operation is finished */
    return jr_mac_table_idle();
}

static vtss_rc jr_mac_table_add(const vtss_mac_table_entry_t *const entry, u32 pgid)
{
    vtss_chip_no_t chip_no;

    /* Add in all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_mac_table_add_chip(entry, pgid));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_mac_table_del_chip(const vtss_vid_mac_t *const vid_mac)
{
    u32 mach, macl;
    
    vtss_mach_macl_get(vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    
    /* Delete/unlearn the given MAC entry */
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_0, mach);
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_1, macl);


    JR_WR(LRN_COMMON, COMMON_ACCESS_CTRL, 
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_CPU_ACCESS_CMD(MAC_CMD_UNLEARN) |
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_MAC_TABLE_ACCESS_SHOT);
    
    /* Wait until MAC operation is finished */
    return jr_mac_table_idle();
}

static vtss_rc jr_mac_table_del(const vtss_vid_mac_t *const vid_mac)
{
    vtss_chip_no_t chip_no;

    /* Delete from all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_mac_table_del_chip(vid_mac));
    }
    return VTSS_RC_OK;
}

/* Analyze the result in the MAC table */
static vtss_rc jr_mac_table_result(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32                cfg2, cfg3, mach, macl, idx;
    vtss_port_no_t     port_no;
    vtss_pgid_entry_t  *pgid_entry;
#if defined(VTSS_FEATURE_VSTAX_V2)
    vtss_chip_no_t     chip_no = vtss_state->chip_no;
    u32                glag_no;
    vtss_vstax_upsid_t my_upsid, upsid;
#endif /* VTSS_FEATURE_VSTAX_V2 */
    
    JR_RD(LRN_COMMON, MAC_ACCESS_CFG_2, &cfg2);
    JR_RD(LRN_COMMON, MAC_ACCESS_CFG_3, &cfg3);
    
    VTSS_D("MAC_ACCESS_CFG_2 %x, MAC_ACCESS_CFG_3 %x",cfg2,cfg3);
    
    /* Check if entry is valid */
    if (!(cfg2 & VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_VLD)) {
        VTSS_D("not valid");
        return VTSS_RC_ERROR;
    }

#if defined(VTSS_FEATURE_VSTAX_V2)
    my_upsid = vtss_state->vstax_info.upsid[chip_no];
    entry->vstax2.remote_entry = 0;
    entry->vstax2.upsid = my_upsid;
    entry->vstax2.upspn = 0;
#endif /* VTSS_FEATURE_VSTAX_V2 */
    
    /* extract fields from Jaguar registers */
    entry->aged        = VTSS_BOOL(VTSS_X_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_AGE_FLAG(cfg2));
    entry->copy_to_cpu = VTSS_BOOL(cfg2 & VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_CPU_COPY);
    entry->locked      = VTSS_BOOL(cfg2 & VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_LOCKED);
    idx                = VTSS_X_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR(cfg2); 

    JR_RD(LRN_COMMON, MAC_ACCESS_CFG_0, &mach);
    JR_RD(LRN_COMMON, MAC_ACCESS_CFG_1, &macl);
    
    entry->vid_mac.vid = ((mach>>16) & 0xFFF);
    entry->vid_mac.mac.addr[0] = ((mach>>8)  & 0xff);
    entry->vid_mac.mac.addr[1] = ((mach>>0)  & 0xff);
    entry->vid_mac.mac.addr[2] = ((macl>>24) & 0xff);
    entry->vid_mac.mac.addr[3] = ((macl>>16) & 0xff);
    entry->vid_mac.mac.addr[4] = ((macl>>8)  & 0xff);
    entry->vid_mac.mac.addr[5] = ((macl>>0)  & 0xff);
    VTSS_D("found 0x%08x%08x, cfg2: 0x%08x, cfg3: 0x%08x", mach, macl, cfg2, cfg3);

    switch (VTSS_X_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_TYPE(cfg2)) {
    case MAC_ENTRY_TYPE_STD_ENTRY:
        /* Normal entry */
        if (VTSS_X_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR_TYPE(cfg2) == 
            MAC_ENTRY_ADDR_TYPE_MC_IDX) {
            /* Multicast PGID */
            idx += VTSS_CHIP_PORTS;
        }
        *pgid = jr_vtss_pgid(vtss_state, idx);
#if defined(VTSS_FEATURE_VSTAX_V2)

        upsid = ((idx >> 5) & 0x1F);
        switch (VTSS_X_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR_TYPE(cfg2)) {
        case  MAC_ENTRY_ADDR_TYPE_UPSID_PN:
            entry->vstax2.remote_entry = (upsid == my_upsid ? 0 : 1);
            entry->vstax2.upsid = upsid;
            entry->vstax2.upspn = idx & 0x1F;
            if (vtss_state->chip_count == 2 && 
                vtss_state->vstax_info.upsid[chip_no ? 0 : 1] == upsid) {
                /* If the address is learned on the other device, we change the context to 
                   that device to make the PGID calculation done in jr_vtss_pgid() correct */
                entry->vstax2.remote_entry = 0;
                VTSS_SELECT_CHIP(chip_no ? 0 : 1);
            }
            *pgid = jr_vtss_pgid(vtss_state, entry->vstax2.upspn);
            VTSS_SELECT_CHIP(chip_no);
            break;
        case MAC_ENTRY_ADDR_TYPE_UPSID_CPU_OR_INT:
            entry->vstax2.remote_entry = (upsid == my_upsid ? 0 : 1);
            entry->vstax2.upsid = upsid;
            entry->vstax2.upspn = VTSS_UPSPN_CPU;
            *pgid = VTSS_PGID_NONE;
            break;
        case MAC_ENTRY_ADDR_TYPE_GLAG:
            glag_no = VTSS_X_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR(cfg2) & 0x1F;
            *pgid = VTSS_PGID_NONE;
            pgid_entry = &vtss_state->pgid_table[*pgid];

            /* Find out if this we have local members of this GLAG */
            entry->vstax2.remote_entry = 1;
            for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
                if (vtss_state->port_glag_no[port_no] == glag_no) {
                    /* Return the local members of the GLAG */
                    pgid_entry->member[port_no] = 1;
                    entry->vstax2.remote_entry = 0;
                }
            }
            if (entry->vstax2.remote_entry) {
                /* No local members - return the first upsid/upspn member entry in the GLAG */
                vtss_glag_conf_t *glag_conf = &vtss_state->glag_conf[glag_no][0];

                if (glag_conf->entry.upspn == VTSS_UPSPN_NONE) {
                    /* No valid GLAG members */
                    entry->vstax2.upsid = 0;
                } else {
                    /* First GLAG member is valid */
                    entry->vstax2.upsid = glag_conf->entry.upsid;
                    entry->vstax2.upspn = glag_conf->entry.upspn;
                }
            }       
            break;
        case  MAC_ENTRY_ADDR_TYPE_MC_IDX:
            /* Already covered */
            break;
        case  MAC_ENTRY_ADDR_TYPE_CONID_FWD:
        case  MAC_ENTRY_ADDR_TYPE_PATHGRP_FWD:
        default:
            VTSS_E("unsupported addr type:%u",VTSS_X_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR_TYPE(cfg2));
            return VTSS_RC_ERROR;
        }     
#endif /* VTSS_FEATURE_VSTAX_V2 */
        break;
    case MAC_ENTRY_TYPE_IPV4_MC_ENTRY:
    case MAC_ENTRY_TYPE_IPV6_MC_ENTRY:
        /* IPv4/IPv6 multicast address */
        *pgid = VTSS_PGID_NONE;
        /* Convert port mask */
        pgid_entry = &vtss_state->pgid_table[*pgid];
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if ((cfg3 & (1 << VTSS_CHIP_PORT(port_no))) && VTSS_PORT_CHIP_SELECTED(port_no)) {
                pgid_entry->member[port_no] = 1;
            }
        }

        /* Reset the search criteria */
        JR_WRM(LRN_COMMON, MAC_ACCESS_CFG_2, 0, VTSS_M_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_TYPE);
        break;
    case MAC_ENTRY_TYPE_PATH_CCM_MEPID:
    default:
        VTSS_E("unsupported entry");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_mac_table_get_chip(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32 mach, macl, type;

    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_0, mach);
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_1, macl);

    /* BZ 10076: Setup entry type before lookup operation */
    type = (VTSS_MAC_IPV4_MC(entry->vid_mac.mac.addr) ? MAC_ENTRY_TYPE_IPV4_MC_ENTRY :
            VTSS_MAC_IPV6_MC(entry->vid_mac.mac.addr) ? MAC_ENTRY_TYPE_IPV6_MC_ENTRY :
            MAC_ENTRY_TYPE_STD_ENTRY);
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_2, VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_TYPE(type));

    /* Do a lookup */
    JR_WR(LRN_COMMON, COMMON_ACCESS_CTRL, 
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_CPU_ACCESS_CMD(MAC_CMD_LOOKUP) |
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_MAC_TABLE_ACCESS_SHOT);

    VTSS_RC(jr_mac_table_idle());

    return jr_mac_table_result(entry, pgid);
}

static vtss_rc jr_mac_table_get(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    vtss_pgid_entry_t *pgid_entry = &vtss_state->pgid_table[VTSS_PGID_NONE];
    
    /* Clear PGID entry for IPMC/GLAG entries */
    memset(pgid_entry, 0, sizeof(*pgid_entry));
    
    /* Get entry from first device */
    VTSS_RC(jr_mac_table_get_chip(entry, pgid));

    /* For IPMC entries, get entry (port set) from second device */
    if (vtss_state->chip_count == 2 && *pgid == VTSS_PGID_NONE) {
        VTSS_SELECT_CHIP(1);
        VTSS_RC(jr_mac_table_get_chip(entry, pgid));
        VTSS_SELECT_CHIP(0);
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_mac_table_get_next(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32               mach, macl;
    vtss_pgid_entry_t *pgid_entry = &vtss_state->pgid_table[VTSS_PGID_NONE];
    
    /* Clear PGID entry for IPMC/GLAG entries */
    memset(pgid_entry, 0, sizeof(*pgid_entry));

    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);

    /* Ensure that no filtering is active */
    JR_WR(LRN_COMMON, SCAN_NEXT_CFG, 
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_SCAN_TYPE_MASK(1) |
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_SCAN_NEXT_UNTIL_FOUND_ENA);
    /* Static entries are left out due to a bug */
   
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_0, mach);
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_1, macl);

    /* Do a get next lookup */
    JR_WR(LRN_COMMON, COMMON_ACCESS_CTRL, 
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_CPU_ACCESS_CMD(MAC_CMD_FIND_SMALLEST) |
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_MAC_TABLE_ACCESS_SHOT);

    VTSS_RC(jr_mac_table_idle());
    return jr_mac_table_result(entry, pgid);
}

/*******************************************************************************
 *  MAC ageing. 
 *
 * involved registers:
 *  AUTOAGE_CFG
 *  AUTOAGE_CFG_1
 *  SCAN_NEXT_CFG           The CCM bits are set to 0, 
 *  MAC_ACCESS_CFG_0
 *  MAC_ACCESS_CFG_2
 *  COMMON_ACCESS_CTRL
 *
 * Pre condition (currently these are the default values):
 *  LRN_CFG.AGE_SIZE = 0 (i.e. 1 bit age counter. 2 bit age counter requires a change
 *                       in the flush functions, i.e. flush is done by calling the
 *                       age function two times.
 *  ANA_L2:COMMON:FILTER_OTHER_CTRL = 0 I.E allow ageing for remote learned entries. (ignored)
 *  ANA_L2:COMMON:FILTER_LOCAL_CTRL = 0 I.E. allow autoaging for all local ports. (ignored)
 *
 ******************************************************************************/

/*
 * set automatic ageing time. 
 * time = 0 disables ageing
 * time = [10..100000] seconds
 * The value of PERIOD_VAL in the AUTOAGE_CFG register shall be > 10.000 therefore:
 * 20001 < time          => units = 3 (sec), PERIOD_VAL = time/2 -1
 * 201 <   time <= 20001 => units = 2(10 ms), PERIOD_VAL = time*100/2 -1
 *         time <= 201   => units = 1(100 us), PERIOD_VAL = time*10000/2 -1
 */
static vtss_rc jr_mac_table_age_time_set_chip(void)
{
    u32 time, units;
    /* Scan two times per age time */
    time = vtss_state->mac_age_time;
    if (time <= 1) {
        units = 0; /* 0: disable ageing */
        time = 0; 
    } else {
        units = 3; /* 0: disable ageing, 3: unit = 1 s. */
        //while (time < 20002) {
        //    --units;
        //    time = time * 100;
        //}
        time = (time/2)-1;           /* age period = time+1 */
    }
        
    if (time > 0x3fffff)
        time = 0x3fffff;         /* period_val is 22 bits wide */
    /* stop aging and set counters to a low value */
    JR_WR(LRN_COMMON, AUTOAGE_CFG, 
            VTSS_F_LRN_COMMON_AUTOAGE_CFG_AUTOAGE_UNIT_SIZE(1)  |
            VTSS_F_LRN_COMMON_AUTOAGE_CFG_AUTOAGE_PERIOD_VAL(1));
    JR_WR(LRN_COMMON, AUTOAGE_CFG_1, 
            VTSS_F_LRN_COMMON_AUTOAGE_CFG_1_AUTOAGE_FORCE_IDLE_ENA |
            VTSS_F_LRN_COMMON_AUTOAGE_CFG_1_AUTOAGE_FORCE_HW_SCAN_STOP_SHOT);
    

    VTSS_D("time %d, units %d", time, units);
    JR_WR(LRN_COMMON, AUTOAGE_CFG, 
            VTSS_F_LRN_COMMON_AUTOAGE_CFG_AUTOAGE_UNIT_SIZE(units)  |
            VTSS_F_LRN_COMMON_AUTOAGE_CFG_AUTOAGE_PERIOD_VAL(time));
    JR_WR(LRN_COMMON, AUTOAGE_CFG_1, 
            (units ? 0:VTSS_F_LRN_COMMON_AUTOAGE_CFG_1_AUTOAGE_FORCE_IDLE_ENA) |
            VTSS_F_LRN_COMMON_AUTOAGE_CFG_1_AUTOAGE_FORCE_HW_SCAN_STOP_SHOT);
    return VTSS_RC_OK;
}

static vtss_rc jr_mac_table_age_time_set(void)
{
    return jr_conf_chips(jr_mac_table_age_time_set_chip);
}

#if defined(VTSS_FEATURE_VSTAX_V2)
static vtss_rc jr_mac_table_upsid_upspn_flush_chip(const vtss_vstax_upsid_t upsid, 
                                                   const vtss_vstax_upspn_t upspn)
{
    u32 value;

    /* Flush all upsid entries by setting up a filter */
    JR_WR(LRN_COMMON, SCAN_NEXT_CFG, 
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_SCAN_NEXT_IGNORE_LOCKED_ENA |
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_ADDR_FILTER_ENA |
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_SCAN_TYPE_MASK(0x1) |
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_SCAN_NEXT_REMOVE_FOUND_ENA);

    /* Bits 14-12 = 110. Bits 9-5 (UPSID) = 0x1F (care). Bits 4-0 (UPSPN) = 0 */
    value = ((6 << 12) | (0x1F << 5) | (upspn == VTSS_UPSPN_NONE ? 0 : 0x1F));
    JR_WR(LRN_COMMON, SCAN_NEXT_CFG_1, 
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_1_SCAN_ENTRY_ADDR_MASK(value));
          
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_2, 
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR((upsid << 5) | (upspn & 0x1F)) | 
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR_TYPE(MAC_ENTRY_ADDR_TYPE_UPSID_PN) |
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_TYPE(MAC_ENTRY_TYPE_STD_ENTRY));

    JR_RD(LRN_COMMON, MAC_ACCESS_CFG_2, &value);

    /* Do the aging */
    JR_WR(LRN_COMMON, COMMON_ACCESS_CTRL, 
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_CPU_ACCESS_CMD(MAC_CMD_SCAN) |
            VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_MAC_TABLE_ACCESS_SHOT);

    /* Wait until MAC operation is finished */
    return jr_mac_table_idle();
}

static vtss_rc jr_mac_table_upsid_upspn_flush(const vtss_vstax_upsid_t upsid,
                                              const vtss_vstax_upspn_t upspn)
{
    vtss_chip_no_t chip_no;

    /* Flush for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_mac_table_upsid_upspn_flush_chip(upsid, upspn));
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_VSTAX_V2 */

static vtss_rc jr_mac_table_age_chip(BOOL             addr_age,
                                     u32              addr,
                                     BOOL             vid_age,
                                     const vtss_vid_t vid)
{
    VTSS_D("addr_age %u, addr %u, vid_age %u, vid %u", addr_age, addr, vid_age, vid);
    
    /* Selective aging */
    JR_WR(LRN_COMMON, SCAN_NEXT_CFG, 
          (vid_age ? VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_FID_FILTER_ENA : 0)   |
          (addr_age ? VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_ADDR_FILTER_ENA : 0) |
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_SCAN_TYPE_MASK(0x1)              |
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_SCAN_NEXT_REMOVE_FOUND_ENA       |
          VTSS_F_LRN_COMMON_SCAN_NEXT_CFG_SCAN_NEXT_INC_AGE_BITS_ENA);
            
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_0, VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_0_MAC_ENTRY_FID(vid));
    JR_WR(LRN_COMMON, MAC_ACCESS_CFG_2, 
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR(addr) |
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_ADDR_TYPE(MAC_ENTRY_ADDR_TYPE_UPSID_PN)  |
          VTSS_F_LRN_COMMON_MAC_ACCESS_CFG_2_MAC_ENTRY_TYPE(MAC_ENTRY_TYPE_STD_ENTRY));

    /* Do the aging */
    JR_WR(LRN_COMMON, COMMON_ACCESS_CTRL, 
          VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_CPU_ACCESS_CMD(MAC_CMD_SCAN) |
          VTSS_F_LRN_COMMON_COMMON_ACCESS_CTRL_MAC_TABLE_ACCESS_SHOT);

    /* Wait until MAC operation is finished */
    return jr_mac_table_idle();
}

static vtss_rc jr_mac_table_age(BOOL             pgid_age,
                                u32              pgid,
                                BOOL             vid_age,
                                const vtss_vid_t vid)
{
    u32            addr_age = 0, addr = 0;
    vtss_chip_no_t chip_no;

    if (pgid_age && pgid < vtss_state->port_count) {
        /* Port specific ageing, find UPSID and UPSPN */
        addr_age = 1;
        addr = VTSS_CHIP_PORT(pgid);
#if defined(VTSS_FEATURE_VSTAX_V2)
        addr += (vtss_state->vstax_info.upsid[vtss_state->port_map[pgid].chip_no] << 5);
#endif /* VSTAX_V2 */
    }

    /* Age for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_mac_table_age_chip(addr_age, addr, vid_age, vid));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_mac_table_status_get_chip(vtss_mac_table_status_t *status)
{
    u32 value;
    
    /* Detect port move events */
    JR_RD(ANA_L2, STICKY_STICKY, &value);
    value &= (VTSS_F_ANA_L2_STICKY_STICKY_LOCAL_TO_REMOTE_PORTMOVE_STICKY  |
              VTSS_F_ANA_L2_STICKY_STICKY_REMOTE_TO_LOCAL_PORTMOVE_STICKY  |
              VTSS_F_ANA_L2_STICKY_STICKY_REMOTE_TO_REMOTE_PORTMOVE_STICKY |
              VTSS_F_ANA_L2_STICKY_STICKY_GLOBAL_TO_GLOBAL_PORTMOVE_STICKY |
              VTSS_F_ANA_L2_STICKY_STICKY_GLOBAL_TO_LOCAL_PORTMOVE_STICKY  |
              VTSS_F_ANA_L2_STICKY_STICKY_LOCAL_TO_GLOBAL_PORTMOVE_STICKY  |
              VTSS_F_ANA_L2_STICKY_STICKY_LOCAL_TO_LOCAL_PORTMOVE_STICKY);
    JR_WR(ANA_L2, STICKY_STICKY, value);
    if (value)
        status->moved = 1;

    /* Read and clear sticky register */
    JR_RD(LRN_COMMON, EVENT_STICKY, &value);
    JR_WR(LRN_COMMON, EVENT_STICKY, value & 
          (VTSS_F_LRN_COMMON_EVENT_STICKY_AUTO_LRN_REPLACE_STICKY |
           VTSS_F_LRN_COMMON_EVENT_STICKY_AUTO_LRN_INSERT_STICKY |
           VTSS_F_LRN_COMMON_EVENT_STICKY_AUTOAGE_REMOVE_STICKY |
           VTSS_F_LRN_COMMON_EVENT_STICKY_AUTOAGE_AGED_STICKY));

    /* Detect learn events */
    if (value & VTSS_F_LRN_COMMON_EVENT_STICKY_AUTO_LRN_INSERT_STICKY)
        status->learned = 1;

    /* Detect replace events */
    if (value & VTSS_F_LRN_COMMON_EVENT_STICKY_AUTO_LRN_REPLACE_STICKY)
        status->replaced = 1;

    /* Detect age events (both aged and removed event is used */
    if (value & (VTSS_F_LRN_COMMON_EVENT_STICKY_AUTOAGE_AGED_STICKY |
                 VTSS_F_LRN_COMMON_EVENT_STICKY_AUTOAGE_REMOVE_STICKY))
        status->aged = 1;

    return VTSS_RC_OK;
}

static vtss_rc jr_mac_table_status_get(vtss_mac_table_status_t *status)
{
    vtss_chip_no_t chip_no;
    
    memset(status, 0, sizeof(*status));
    
    /* Get status from all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_mac_table_status_get_chip(status));
    }
    return VTSS_RC_OK;
}

/* Learn mode: 'Learn frames dropped' / 'Autolearning' / 'Copy learn frame to CPU' */
static vtss_rc jr_learn_port_mode_set(const vtss_port_no_t port_no)
{
    vtss_learn_mode_t *mode = &vtss_state->learn_mode[port_no];
    u32               mask = (1 << VTSS_CHIP_PORT(port_no));

    JR_WRM(ANA_L2_COMMON, LRN_SECUR_CFG,        mode->discard   ? mask : 0, mask);
    JR_WRM(ANA_L2_COMMON, LRN_SECUR_LOCKED_CFG, mode->discard   ? mask : 0, mask);
    JR_WRM(ANA_L2_COMMON, AUTO_LRN_CFG,         mode->automatic ? mask : 0, mask);
    JR_WRM(ANA_L2_COMMON, LRN_COPY_CFG,         mode->cpu       ? mask : 0, mask);

    if (!mode->automatic) {
        /* If automatic ageing is disabled, flush entries previously learned on port. 
           To avoid continuous refreshing, learning must be disabled during the operation */
        VTSS_RC(jr_mac_table_age(1, port_no, 0, 0));
        VTSS_RC(jr_mac_table_age(1, port_no, 0, 0));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_learn_state_set(const BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_chip_no_t chip_no;
    u32            mask;

    /* Setup learn mask for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        mask = jr_port_mask(member);
        if (vtss_state->chip_count == 2) {
            /* Include internal ports in learn mask */
            mask |= vtss_state->mask_int_ports;
        }
        JR_WRX(ANA_L3_2, MSTP_MSTP_LRN_CFG, 0, mask);
    }
    
    return VTSS_RC_OK;
}

static vtss_rc jr_vlan_conf_set_chip(BOOL ports)
{
    vtss_port_no_t port_no;
    
    JR_WR(ANA_CL_2, COMMON_VLAN_STAG_CFG, vtss_state->vlan_conf.s_etype);
    
    /* Update VLAN port configuration */
    for (port_no = VTSS_PORT_NO_START; ports && port_no < vtss_state->port_count; port_no++) {
        if (VTSS_PORT_CHIP_SELECTED(port_no)) {
            VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
        }
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_vlan_conf_set(void)
{
    vtss_chip_no_t chip_no;
    
    /* Setup VLAN for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        VTSS_RC(jr_vlan_conf_set_chip(1));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_vlan_mask_apply(vtss_vid_t vid, u32 mask)
{
    /* VLAN mask */
    JR_WRX(ANA_L3_2, VLAN_VLAN_MASK_CFG, vid, mask);

    /* Isolation and FID */
    JR_WRXM(ANA_L3_2, VLAN_VLAN_CFG, vid, 
            VTSS_F_ANA_L3_2_VLAN_VLAN_CFG_VLAN_FID(vid) |
            (vtss_state->vlan_table[vid].isolated ? 
             VTSS_F_ANA_L3_2_VLAN_VLAN_CFG_VLAN_PRIVATE_ENA : 0),
            VTSS_M_ANA_L3_2_VLAN_VLAN_CFG_VLAN_FID |
            VTSS_F_ANA_L3_2_VLAN_VLAN_CFG_VLAN_PRIVATE_ENA);
    
    return VTSS_RC_OK;
}

static vtss_rc jr_vlan_mask_update(vtss_vid_t vid, BOOL member[VTSS_PORT_ARRAY_SIZE]) 
{
    vtss_chip_no_t chip_no;
    u32            mask;

    /* Update VLAN mask for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        mask = jr_port_mask(member);
        if (vtss_state->chip_count == 2) {
            /* Include internal ports in all VLAN masks */
            mask |= vtss_state->mask_int_ports;
        }
        VTSS_RC(jr_vlan_mask_apply(vid, mask));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_vlan_port_conf_apply(u32 port, vtss_vlan_port_conf_t *conf, vtss_qos_port_conf_t *qp_conf)
{
    BOOL       aware = 1, c_port = 0, s_port = 0, s_custom_port = 0, tagged, untagged, vid_ena;
    vtss_vid_t uvid = conf->untagged_vid;
    u32        tag_mode, mask, value = 0, tpid = VTSS_ETYPE_TAG_C;
    u32        etype = vtss_state->vlan_conf.s_etype;


    /* Check port type */
    switch (conf->port_type) {
    case VTSS_VLAN_PORT_TYPE_UNAWARE:
        aware = 0;
        break;
    case VTSS_VLAN_PORT_TYPE_C:
        c_port = 1;
        break;
    case VTSS_VLAN_PORT_TYPE_S:
        s_port = 1;
        tpid = 1;
        break;
    case VTSS_VLAN_PORT_TYPE_S_CUSTOM:
        s_port = 1;
        s_custom_port = 1;
        tpid = 2;
        break;
    default:
        return VTSS_RC_ERROR;
    }
    vid_ena = (aware ? 0 : 1);

#if defined(VTSS_FEATURE_VSTAX_V2)
    {
        vtss_vstax_chip_info_t *chip_info = &vtss_state->vstax_info.chip_info[vtss_state->chip_no];
        if (VTSS_BIT(port) & (chip_info->mask_a | chip_info->mask_b)) {
            /* Stacking is enabled on port */
            aware = 0;           /* Avoid popping tags on ingress */
            uvid = VTSS_VID_ALL; /* Avoid pushing tags on egress */
            vid_ena = 0;         /* Avoid changing VLAN classification */
        }
    }
#endif /* VTSS_FEATURE_VSTAX_V2 */

    /* VLAN popping is done using default actions in IS0 */
    /* BZ 4301: For VLAN unaware ports, the PVID is setup in IS0 */
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS0, port, VTSS_TCAM_CMD_READ));
    JR_WRF(VCAP_IS0, BASETYPE_ACTION_A, VLAN_POP_CNT, aware);
    JR_WRB(VCAP_IS0, BASETYPE_ACTION_A, VID_ENA, vid_ena);
    JR_WRF(VCAP_IS0, BASETYPE_ACTION_B, VID_VAL, conf->pvid);
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS0, port, VTSS_TCAM_CMD_WRITE));

    /* BZ 8810: Ports are always VLAN aware */
    /* Disable tag classification in chip if configured as VLAN unaware, even if tag classification is enabled in QoS */
    if (qp_conf) {
        if (aware && qp_conf->tag_class_enable) {
            JR_WRXM(ANA_CL_2, PORT_QOS_CFG, port,
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_UPRIO_QOS_ENA      |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_UPRIO_DP_ENA       |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_ITAG_UPRIO_QOS_ENA |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_ITAG_UPRIO_DP_ENA,
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_UPRIO_QOS_ENA      |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_UPRIO_DP_ENA       |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_ITAG_UPRIO_QOS_ENA |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_ITAG_UPRIO_DP_ENA);
        } else {
            JR_WRXM(ANA_CL_2, PORT_QOS_CFG, port,
                    0,
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_UPRIO_QOS_ENA      |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_UPRIO_DP_ENA       |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_ITAG_UPRIO_QOS_ENA |
                    VTSS_F_ANA_CL_2_PORT_QOS_CFG_ITAG_UPRIO_DP_ENA);
        }
    }

    /* Port VLAN Configuration */
    /* BZ 4301: Ports are always VLAN aware to allow IS1 to operate on tags */
    JR_WRXM(ANA_CL_2, PORT_VLAN_CTRL, port,
            VTSS_F_ANA_CL_2_PORT_VLAN_CTRL_PORT_VID(conf->pvid) |
            VTSS_F_ANA_CL_2_PORT_VLAN_CTRL_VLAN_AWARE_ENA,
            VTSS_M_ANA_CL_2_PORT_VLAN_CTRL_PORT_VID |
            VTSS_F_ANA_CL_2_PORT_VLAN_CTRL_VLAN_AWARE_ENA);

    /* Drop Configuration based on port type and frame type */
    tagged = (conf->frame_type == VTSS_VLAN_FRAME_TAGGED);
    untagged = (conf->frame_type == VTSS_VLAN_FRAME_UNTAGGED);
    if (tagged && aware) {
        /* Discard untagged and priority-tagged if aware and tagged-only allowed */
        value |= VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_VLAN_UNTAG_DIS;
        value |= VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_OUT_PRIO_CTAG_DIS;
        value |= VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_OUT_PRIO_STAG_DIS;
    }
    if ((untagged && c_port) || (tagged && s_port)) {
        /* Discard C-tagged if C-port and untagged-only OR S-port and tagged-only */
        value |= VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_OUT_CTAG_DIS;
    }
    if ((untagged && s_port) || (tagged && c_port)) {
        /* Discard S-tagged if S-port and untagged-only OR C-port and tagged-only */
        value |= VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_OUT_STAG_DIS;
    }
    JR_WRXM(ANA_CL_2, PORT_FILTER_CTRL, port, value,
            VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_VLAN_UNTAG_DIS |
            VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_OUT_PRIO_CTAG_DIS |
            VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_OUT_CTAG_DIS |
            VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_OUT_PRIO_STAG_DIS |
            VTSS_F_ANA_CL_2_PORT_FILTER_CTRL_OUT_STAG_DIS);
            
    /* Custom Ethernet Type */
    mask = (1 << port);
    JR_WRXM(ANA_CL_2, COMMON_CUSTOM_STAG_ETYPE_CTRL, 0, s_custom_port ? mask : 0, mask);

    /* Ingress filtering */
    JR_WRM(ANA_L3_2, COMMON_VLAN_FILTER_CTRL, conf->ingress_filter ? mask : 0, mask);

    /* BZ 7231: For dual Jaguar systems, tagging all frames, except VID zero is supported.
       The EVC feature requires that tagging VID zero is supported */
    tag_mode = (uvid == VTSS_VID_ALL ? 0 : uvid == VTSS_VID_NULL ? 
                (vtss_state->chip_count == 2 ? 2 : 3) : 1);
    
    /* Rewriter VLAN tag configuration */
    JR_WRXM(REW, PORT_TAG_CTRL, port,
            JR_PUT_FLD(REW, PORT_TAG_CTRL, TAG_MODE, tag_mode) |
            JR_PUT_FLD(REW, PORT_TAG_CTRL, TPI_SEL, tpid) |
            JR_PUT_FLD(REW, PORT_TAG_CTRL, VID_SEL, 2),
            JR_MSK(REW, PORT_TAG_CTRL, TAG_MODE) |
            JR_MSK(REW, PORT_TAG_CTRL, TPI_SEL) |
            JR_MSK(REW, PORT_TAG_CTRL, VID_SEL));
            JR_WRXM(REW, PORT_PORT_TAG_DEFAULT, port, 
            JR_PUT_FLD(REW, PORT_PORT_TAG_DEFAULT, PORT_TCI_VID, uvid) |
            JR_PUT_FLD(REW, PORT_PORT_TAG_DEFAULT, PORT_TPI_ETYPE, etype),
            JR_MSK(REW, PORT_PORT_TAG_DEFAULT, PORT_TCI_VID) |
            JR_MSK(REW, PORT_PORT_TAG_DEFAULT, PORT_TPI_ETYPE));

    return VTSS_RC_OK;
}

static vtss_rc jr_vlan_port_conf_update(vtss_port_no_t port_no, vtss_vlan_port_conf_t *conf)
{
    /* Update maximum tags allowed */
    VTSS_RC(jr_port_max_tags_set(port_no));

    /* Update VLAN port configuration */
    return jr_vlan_port_conf_apply(VTSS_CHIP_PORT(port_no), conf, &vtss_state->qos_port_conf[port_no]);
}

static vtss_rc jr_vcl_port_conf_set(const vtss_port_no_t port_no)
{
    BOOL dmac_ena = vtss_state->vcl_port_conf[port_no].dmac_dip;
    u32  port = VTSS_CHIP_PORT(port_no);

    /* DMAC/DIP use in IS1 is enabled using default actions in IS0 */
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS0, port, VTSS_TCAM_CMD_READ));
    JR_WRB(VCAP_IS0, BASETYPE_ACTION_A, S1_DMAC_ENA, dmac_ena);
    VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS0, port, VTSS_TCAM_CMD_WRITE));
    return VTSS_RC_OK;
}

static vtss_rc jr_isolated_port_members_set_chip(void)
{
    JR_WR(ANA_L3_2, COMMON_VLAN_ISOLATED_CFG, jr_port_mask(vtss_state->isolated_port));
    return VTSS_RC_OK;
}

static vtss_rc jr_isolated_port_members_set(void)
{
    return jr_conf_chips(jr_isolated_port_members_set_chip);
}

static vtss_rc jr_aggr_mode_set_chip(void)
{
    vtss_aggr_mode_t *mode = &vtss_state->aggr_mode;

    JR_WRB(ANA_CL_2, COMMON_AGGR_CFG, AGGR_SMAC_ENA, mode->smac_enable);
    JR_WRB(ANA_CL_2, COMMON_AGGR_CFG, AGGR_DMAC_ENA, mode->dmac_enable);
    JR_WRB(ANA_CL_2, COMMON_AGGR_CFG, AGGR_IP4_SIPDIP_ENA, mode->sip_dip_enable);
    JR_WRB(ANA_CL_2, COMMON_AGGR_CFG, AGGR_IP4_TCPUDP_PORT_ENA, mode->sport_dport_enable);
    JR_WRB(ANA_CL_2, COMMON_AGGR_CFG, AGGR_IP6_TCPUDP_PORT_ENA, mode->sport_dport_enable);

    return VTSS_RC_OK;
}

static vtss_rc jr_aggr_mode_set(void)
{
    return jr_conf_chips(jr_aggr_mode_set_chip);
}

/* Based on mux mode, return unused port for EEE loopback and host mode purposes */
static u32 jr_unused_chip_port(void)
{
    return (vtss_state->mux_mode[vtss_state->chip_no] == VTSS_PORT_MUX_MODE_0 ? 26 : 27);
}

static vtss_rc jr_mirror_port_set_chip(void)
{
    /* Setup mirroring for all devices */
    return jr_mirror_conf_set_chip();
}

static vtss_rc jr_mirror_port_set(void)
{
    vtss_port_no_t port_no;

    /* Update all VLANs */
    VTSS_RC(vtss_cmn_vlan_update_all());

    /* Update VLAN port configuration for all ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_SELECT_CHIP_PORT_NO(port_no);
        VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
    }

    /* Setup mirroring for all devices */
    return jr_conf_chips(jr_mirror_port_set_chip);
}

static vtss_rc jr_mirror_ingress_set(void)
{
    /* Setup mirroring for all devices */
    return jr_conf_chips(jr_mirror_conf_set_chip);
}

static vtss_rc jr_mirror_egress_set(void)
{
    /* Setup mirroring for all devices */
    return jr_conf_chips(jr_mirror_conf_set_chip);
}

static vtss_rc jr_src_table_write_chip(u32 port, u32 mask)
{
    JR_WRX(ANA_AC, SRC_SRC_CFG, port, mask);
    return VTSS_RC_OK;
}

static vtss_rc jr_src_table_write(vtss_port_no_t port_no, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    u32            mask, port = VTSS_CHIP_PORT(port_no);
    vtss_chip_no_t chip_no;

    VTSS_SELECT_CHIP_PORT_NO(port_no);
    mask = jr_port_mask(member);
    if (vtss_state->chip_count == 2) {
        /* Include internal ports in source masks if any port on the other device is included */
        chip_no = vtss_state->chip_no;
        VTSS_SELECT_CHIP(chip_no ? 0 : 1);
        if (jr_port_mask(member)) {
            mask |= vtss_state->mask_int_ports;
        }
        VTSS_SELECT_CHIP(chip_no);
    }
    
    return jr_src_table_write_chip(port, mask);
}

static vtss_rc jr_pgid_table_write_chip(u32 pgid, u32 mask, BOOL cpu_copy, 
                                        vtss_packet_rx_queue_t cpu_queue, BOOL stack_type_ena)
{
    JR_WRX(ANA_AC, PGID_PGID_CFG_0, pgid, mask);
    JR_WRX(ANA_AC, PGID_PGID_CFG_1, pgid, 
           JR_PUT_BIT(ANA_AC, PGID_PGID_CFG_1, PGID_CPU_COPY_ENA, cpu_copy) |
           JR_PUT_BIT(ANA_AC, PGID_PGID_CFG_1, STACK_TYPE_ENA, stack_type_ena) |
           JR_PUT_FLD(ANA_AC, PGID_PGID_CFG_1, PGID_CPU_QU, cpu_queue));
    return VTSS_RC_OK;
}

static vtss_rc jr_pgid_table_write(u32 pgid, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_chip_no_t    chip_no;
    vtss_pgid_entry_t *pgid_entry = &vtss_state->pgid_table[pgid];
    u32               mask, i = jr_chip_pgid(pgid);

    /* Setup PGID masks for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        mask = jr_port_mask(member);
        if (i < VTSS_CHIP_PORTS) {
            /* UC entry */
            if (!VTSS_PORT_CHIP_SELECTED(pgid)) {
                /* Only setup UC mask if port is on selected chip */
                continue;
            }
        } else if (vtss_state->chip_count == 2) {
            /* MC entry, include internal ports in mask */
            mask |= vtss_state->mask_int_ports;
        }
        VTSS_RC(jr_pgid_table_write_chip(i, mask, pgid_entry->cpu_copy, 
                                         pgid_entry->cpu_queue, 0));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_aggr_table_write(u32 ac, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_chip_no_t chip_no;
    u32            mask;

    /* Setup aggregation mask for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        mask = jr_port_mask(member);
        if (vtss_state->chip_count == 2) {
            /* Include one internal port in each aggregation mask */
            mask |= VTSS_BIT(ac & 1 ? vtss_state->port_int_0 : vtss_state->port_int_1);
        }
        // The following is required by EEE, and doesn't harm if EEE is not enabled or not included.
        mask |= VTSS_BIT(jr_unused_chip_port());
        JR_WRX(ANA_AC, AGGR_AGGR_CFG, ac, mask);
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_pmap_table_write(vtss_port_no_t port_no, vtss_port_no_t l_port_no)
{
    BOOL glag_ena=0;
    u32 glag_no=0;
    
    VTSS_SELECT_CHIP_PORT_NO(port_no);
#if defined(VTSS_FEATURE_AGGR_GLAG )
    if ((vtss_state->port_glag_no[port_no]) != VTSS_GLAG_NO_NONE) {
        glag_ena = 1;
        glag_no = vtss_state->port_glag_no[port_no];
    }
#endif    
    JR_WRX(ANA_CL_2, PORT_PORT_ID_CFG, VTSS_CHIP_PORT(port_no),
           VTSS_F_ANA_CL_2_PORT_PORT_ID_CFG_LPORT_NUM(VTSS_CHIP_PORT(l_port_no)) |
           (glag_ena ? VTSS_F_ANA_CL_2_PORT_PORT_ID_CFG_PORT_IS_GLAG_ENA : 0) |
           VTSS_F_ANA_CL_2_PORT_PORT_ID_CFG_GLAG_NUM(glag_no));

    return VTSS_RC_OK;
}

/* Update PGID state for reserved entry and update chip */
static vtss_rc jr_pgid_update(u32 pgid, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_port_no_t    port_no;
    vtss_pgid_entry_t *pgid_entry;
    
    pgid = jr_vtss_pgid(vtss_state, pgid);
    pgid_entry = &vtss_state->pgid_table[pgid];
    pgid_entry->resv = 1;
    pgid_entry->references = 1;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        pgid_entry->member[port_no] = member[port_no];
    
    return jr_pgid_table_write(pgid, member);
}

static vtss_rc jr_flood_conf_set(void)
{
    vtss_port_no_t port_no;
    BOOL           member[VTSS_PORT_ARRAY_SIZE];

    /* Unicast flood mask */
    VTSS_RC(jr_pgid_update(PGID_UC_FLOOD, vtss_state->uc_flood));

    /* Multicast flood mask */
    VTSS_RC(jr_pgid_update(PGID_MC_FLOOD, vtss_state->mc_flood));

    /* IPv4 multicast control flood mask */
    VTSS_RC(jr_pgid_update(PGID_IPV4_MC_CTRL, vtss_state->mc_flood));

    /* IPv4 multicast data flood mask */
    VTSS_RC(jr_pgid_update(PGID_IPV4_MC_DATA, vtss_state->ipv4_mc_flood));

    /* IPv6 multicast control flood mask */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        member[port_no] = (vtss_state->ipv6_mc_scope ? vtss_state->ipv6_mc_flood[port_no] : 
                           vtss_state->mc_flood[port_no]);
    VTSS_RC(jr_pgid_update(PGID_IPV6_MC_CTRL, member));
    
    /* Setup IPv6 MC data flood mask */
    return jr_pgid_update(PGID_IPV6_MC_DATA, vtss_state->ipv6_mc_flood);
}

#if defined(VTSS_FEATURE_IPV4_MC_SIP)
/* Add/delete IS2 entry for SSM source */
static vtss_rc jr_ip_mc_is2_update(vtss_ipmc_data_t *ipmc)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is2.obj;
    vtss_vcap_data_t data;
    vtss_is2_entry_t entry;
    vtss_vcap_id_t   id;
    vtss_ace_t       *ace = &entry.ace;
    vtss_chip_no_t   chip_no;
    u32              fwd_key_sel = (vtss_state->ipmc.obj.src_count ? 2 : 0);

    if (ipmc->src.ssm) {
        /* SSM entry VCAP ID */
        id = ipmc->src.vid;
        id = ((id << 32) + ipmc->src.sip.ipv4);

        if (ipmc->src_add) {
            /* Add IS2 SSM rule */
            memset(&data, 0, sizeof(data));
            memset(&entry, 0, sizeof(entry));
            data.u.is2.entry = &entry;
            entry.first = 1;
            entry.type = IS2_ENTRY_TYPE_IP_OTHER;
            entry.type_mask = JR_IS2_ANY_IP_MASK;
            entry.include_int_ports = 1;
            entry.include_stack_ports = 1;
            ace->port_no = VTSS_PORT_NO_NONE;
            ace->action.learn = 1;
            ace->action.forward = 1;
            ace->type = VTSS_ACE_TYPE_IPV4;
            ace->vlan.vid.value = ipmc->src.vid;
            ace->vlan.vid.mask = 0xfff;
            ace->frame.ipv4.sip.value = ipmc->src.sip.ipv4;
            ace->frame.ipv4.sip.mask = 0xffffffff;
            VTSS_RC(vtss_vcap_add(obj, VTSS_IS2_USER_SSM, id, VTSS_VCAP_ID_LAST, &data, 0));
        } else if (ipmc->src_del) {
            /* Delete IS2 SSM rule */
            VTSS_RC(vtss_vcap_del(obj, VTSS_IS2_USER_SSM, id));
        }
    }

    /* Setup default forward key in MAC address table for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        JR_WRF(ANA_L2, COMMON_FWD_CFG, FWD_KEY_SEL, fwd_key_sel);
    }

    /* Call IGMP setup function, to setup default rules and VCAP lookup */
    VTSS_RC(jr_igmp_cpu_copy_set());
    
    return VTSS_RC_OK;
}

/* Convert 64-bit key to VID/MAC */
static void jr_ip_mc_mac_get(vtss_vid_mac_t *vid_mac, u64 key)
{
    u32 i;

    vid_mac->vid = (key >> 48);
    for (i = 0; i < 6; i++) {
        vid_mac->mac.addr[i] = (key >> ((5 - i)*8));
    }
}

/* SIP[31:0] and DIP[27:0] used as 60-bit (VID, DMAC) key */
static u64 jr_ip_mc_ssm_key(u32 sip, u32 dip)
{
    u64 key = sip;
    
    return ((key << 28) + (dip & 0x0fffffff));
}

static vtss_rc jr_ip_mc_update(vtss_ipmc_data_t *ipmc, vtss_ipmc_cmd_t cmd)
{
    vtss_ipmc_obj_t        *ipmc_obj = &vtss_state->ipmc.obj;
    vtss_ipmc_src_t        *src, *src_asm, *src_list = ipmc_obj->src_used[0];
    vtss_ipmc_dst_t        *dst;
    vtss_res_t             res;
    u32                    sip, dip = ipmc->dst.dip.ipv4;
    u32                    mac_count = 1, port_count = vtss_state->port_count;
    u64                    key;
    BOOL                   ssm_found = 0, ssm_del = 1;
    vtss_port_no_t         port_no;
    vtss_mac_table_entry_t mac_entry;
    vtss_vid_mac_t         *vid_mac = &mac_entry.vid_mac;

    VTSS_I("%s IPv%u %sSM, vid:%u, fid:%u, sip:0x%08x, dip:0x%08x",
           cmd == VTSS_IPMC_CMD_CHECK ? "CHK" :
           cmd == VTSS_IPMC_CMD_ADD ? "ADD" :
           cmd == VTSS_IPMC_CMD_DEL ? "DEL" : "?",
           ipmc->ipv6 ? 6 : 4, 
           ipmc->src.ssm ? "S" : "A", 
           ipmc->src.vid, 
           ipmc->src.fid,
           ipmc->src.sip.ipv4, 
           ipmc->dst.dip.ipv4);

    if (ipmc->ipv6) {
        VTSS_E("IPv6 SSM not supported");
        return VTSS_RC_ERROR;
    }
    
    if (cmd == VTSS_IPMC_CMD_CHECK) {
        /* Check that resources are available */
        
        /* New source requires IS2 entries */
        if (ipmc->src_add) {
            vtss_cmn_res_init(&res);

            /* The first added source may require extra entries */
            if (ipmc_obj->src_count == 0) 
                res.is2.add = (JR_IS2_ID_ANY_CNT + 1);

            /* New SSM source requires one IS2 entry */
            if (ipmc->src.ssm)
                res.is2.add++;
            
            VTSS_RC(vtss_cmn_res_check(&res));
        }

        /* No futher processing unless adding new destination */
        if (!ipmc->dst_add)
            return VTSS_RC_OK;

        /* ASM source may require one more entry per SSM SIP */
        for (src = src_list; ipmc->src.ssm == 0 && src != NULL; src = src->next) {
            if (!src->data.ssm)
                continue;
            
            /* SSM source, look for matching DIP */
            for (dst = src->dest; dst != NULL; dst = dst->next) {
                if (dst->data.dip.ipv4 == dip) {
                    ssm_found = 1;
                    break;
                }
            }
            
            /* Continue if the next source has the same SIP */
            if (src->next != NULL && src->next->data.sip.ipv4 == src->data.sip.ipv4)
                continue;
            
            /* If the DIP is not found, one (SIP, DIP) entry is required */
            if (!ssm_found)
                mac_count++;
            ssm_found = 0;
        }
        
        if ((vtss_state->mac_table_count + mac_count) > vtss_state->mac_table_max) {
            VTSS_I("no more MAC entries");
            return VTSS_RC_ERROR;
        }
        return VTSS_RC_OK;
    }
    
    /* We come to this point with VTSS_IPMC_CMD_ADD/DEL to add/delete resources */

    /* Update MAC table entries */
    memset(&mac_entry, 0, sizeof(mac_entry));
    mac_entry.locked = 1;
    for (src = src_list; src != NULL; src = src->next) {
        /* Skip ASM sources */
        if (!src->data.ssm)
            continue;
        
        /* SSM source, look for matching DIP */
        for (dst = src->dest; dst != NULL; dst = dst->next) {
            if (dst->data.dip.ipv4 == dip) {
                ssm_found = 1;
                break;
            }
        }
        
        /* If not found, look for DIP for the ASM source using the VID */
        for (src_asm = src_list; dst == NULL && src_asm != NULL; src_asm = src_asm->next) {
            if (src_asm->data.ssm)
                break;
            if (src_asm->data.vid != src->data.vid)
                continue;
            for (dst = src_asm->dest; dst != NULL; dst = dst->next) {
                if (dst->data.dip.ipv4 == dip)
                    break;
            }
        }
        
        /* If found, update destination set */
        if (dst != NULL) {
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (VTSS_PORT_BF_GET(dst->data.member, port_no)) {
                    mac_entry.destination[port_no] = 1;
                }
            }
        }
        
        /* Continue if the next source has the same SIP */
        sip = src->data.sip.ipv4;
        if (src->next != NULL && src->next->data.sip.ipv4 == sip)
            continue;
        
        /* Clear SSM delete flag indicating that SSM (SIP, DIP) has been added/deleted */
        if (sip == ipmc->src.sip.ipv4)
            ssm_del = 0;

        /* Add/delete (SIP, DIP) MAC table entry */
        key = jr_ip_mc_ssm_key(sip, dip);
        jr_ip_mc_mac_get(vid_mac, key);
        if (ssm_found) {
            (void)vtss_mac_add(VTSS_MAC_USER_SSM, &mac_entry);
            ssm_found = 0;
            memset(mac_entry.destination, 0, port_count);
        } else {
            (void)vtss_mac_del(VTSS_MAC_USER_SSM, vid_mac);
        }
    } /* src loop */

    if (ipmc->src.ssm) {
        /* Delete (SIP, DIP) rule if SSM (SIP, DIP) was not found */
        if (ssm_del) {
            key = jr_ip_mc_ssm_key(ipmc->src.sip.ipv4, dip);
            jr_ip_mc_mac_get(vid_mac, key);
            (void)vtss_mac_del(VTSS_MAC_USER_SSM, vid_mac);
        }
    } else {
        /* Add/delete (VID, DIP) MAC table entry */
        key = ipmc->src.vid;
        key = ((key << 16) + 0x0100);
        key = ((key << 32) + (dip & 0x0fffffff));
        jr_ip_mc_mac_get(vid_mac, key);
        if (ipmc->dst_del) {
            (void)vtss_mac_del(VTSS_MAC_USER_ASM, vid_mac);
        } else {
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (VTSS_PORT_BF_GET(ipmc->dst.member, port_no)) {
                    mac_entry.destination[port_no] = 1;
                }
            }
            (void)vtss_mac_add(VTSS_MAC_USER_ASM, &mac_entry);
        }
    }

    /* Update VCAP IS2 */
    VTSS_RC(jr_ip_mc_is2_update(ipmc));
    
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_IPV4_MC_SIP */

#if defined(VTSS_FEATURE_AGGR_GLAG)
static vtss_rc jr_glag_src_table_write(u32 glag_no, u32 port_count, 
                                       BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_chip_no_t chip_no;
    u32            mask;

    if (port_count)
        port_count--;
    
    /* Update GLAG source mask and port member count for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        mask = jr_port_mask(member);
        if (vtss_state->chip_count == 2) {
            /* Include internal ports in GLAG source mask */
            mask |= vtss_state->mask_int_ports;
        }
        JR_WRX(ANA_AC, GLAG_GLAG_CFG, glag_no, mask);   
        JR_WRXF(ANA_AC, GLAG_MBR_CNT_CFG, glag_no, GLAG_MBR_CNT, port_count); 
    }    
    return VTSS_RC_OK;
}

static vtss_rc jr_glag_port_write(u32 glag_no, u32 idx, vtss_vstax_glag_entry_t *entry)
{
    vtss_chip_no_t chip_no;
    u32            value;
    
    /* Update GLAG port entry for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        value = ((entry->upsid << 6) + entry->upspn); /* UPSID at bit 6:10, UPSPN at bit 0:4 */
        if (vtss_state->vstax_info.upsid[vtss_state->chip_no] == entry->upsid)
            value += (entry->upspn << 16); /* If local UPSID, UPSPN at bit 16:20 */
        VTSS_RC(jr_pgid_table_write_chip(PGID_GLAG_START + glag_no*VTSS_GLAG_PORTS + idx, 
                                         value, 0, 0, 1));
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_AGGR_GLAG */

#if defined(VTSS_FEATURE_VLAN_COUNTERS)
/* Read and update ANA_AC/REW counters */
#define JR_VLAN_CNT_GET(tgt, sdx, idx, id, cnt, clr)                            \
{                                                                               \
    u32 lsb = 0, msb = 0, temp_lsb, temp_msb;                                   \
    vtss_chip_no_t              chip_no;                                        \
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {            \
        VTSS_SELECT_CHIP(chip_no);                                              \
        JR_RDXY(tgt, STAT_CNT_CFG_##sdx##_STAT_LSB_CNT, idx, id, &temp_lsb);    \
        JR_RDXY(tgt, STAT_CNT_CFG_##sdx##_STAT_MSB_CNT, idx, id/2, &temp_msb);  \
        lsb += temp_lsb;                                                        \
        msb += temp_msb;                                                        \
    }                                                                           \
    vtss_cmn_counter_40_update(lsb, msb, cnt.bytes, clr);                       \
    lsb = 0;                                                                    \
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {            \
        VTSS_SELECT_CHIP(chip_no);                                              \
        JR_RDXY(tgt, STAT_CNT_CFG_##sdx##_STAT_LSB_CNT, idx, id + 1, &temp_lsb);\
        lsb += temp_lsb;                                                        \
    }                                                                           \
    vtss_cmn_counter_32_update(lsb, cnt.frames, clr);                           \
}
#define JR_VLAN_CNT_ADD(c, counters, name)          \
{                                                  \
    counters->name.frames += c->name.frames.value; \
    counters->name.bytes += c->name.bytes.value;   \
}

static vtss_rc jr_vlan_counters_update(vtss_vlan_chip_counters_t    *pcnt,
                                       u32                          idx,
                                       vtss_vlan_counters_t         *const counters,
                                       BOOL                         clear)
{

    /* Read and update counters for all the chips */
    JR_VLAN_CNT_GET(ANA_AC, ISDX, idx, 0, &pcnt->rx_vlan_unicast, clear);
    JR_VLAN_CNT_GET(ANA_AC, ISDX, idx, 2, &pcnt->rx_vlan_multicast, clear);
    JR_VLAN_CNT_GET(ANA_AC, ISDX, idx, 4, &pcnt->rx_vlan_flood, clear);

    if (counters != NULL) {         
        /* Add counters */
        JR_VLAN_CNT_ADD(pcnt, counters, rx_vlan_unicast);
        JR_VLAN_CNT_ADD(pcnt, counters, rx_vlan_multicast);
        JR_VLAN_CNT_ADD(pcnt, counters, rx_vlan_flood);
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_vlan_counters_get(vtss_vid_t     vid,
                                    vtss_vlan_counters_t *counters)
{
    vtss_rc rc;
    // #vid is in range [1; VTSS_VIDS - 1]
    rc = jr_vlan_counters_update(&vtss_state->vlan_counters_info.counters[vid - 1], vid, counters, FALSE);
    return rc;
}

static vtss_rc jr_vlan_counters_clear(vtss_vid_t vid)
{
    vtss_rc rc;
    rc = jr_vlan_counters_update(&vtss_state->vlan_counters_info.counters[vid - 1], vid, NULL, TRUE);
    return rc;
}

static vtss_rc jr_vlan_counters_poll_1sec(void)
{
    vtss_vlan_counter_info_t  *vlan_info = &vtss_state->vlan_counters_info;
    u32                       i, idx;

    /* Maximum we support 90Gbps traffic in Jaguar-48. For 100Gbps, 32-bit counter wrap time is around 26s.
       Hence, we poll 200 VLAN counters per second. This will give us approximately 20s time between the 
       fetches */
    for (i = 0; i < 200; i++) {
        // Poll in range vid == [1; VTSS_VIDS - 1], which corresponds to idx == [0; VTSS_VIDS - 2]
        idx = vlan_info->poll_idx++;
        if (vlan_info->poll_idx >= VTSS_VIDS - 1) {
            vlan_info->poll_idx = 0;
        }
        VTSS_RC(jr_vlan_counters_get(idx + 1, NULL));
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_VLAN_COUNTERS */

/* ================================================================= *
 *  Miscellaneous
 * ================================================================= */

// Function for initializing chip temperature sensor
//  
// In: enable - True if temperature sensor shall be enabled else false
//
// Return: VTSS_RC_OK if temperature sensor was initialized - else error code


static vtss_rc jr_temp_sensor_init(const BOOL enable) 
{
    if (enable) {
        JR_WR(DEVCPU_GCB, TEMP_SENSOR_TEMP_SENSOR_CTRL, 0x1);   // Enable sensor (see datasheet)
    } else {
        JR_WR(DEVCPU_GCB, TEMP_SENSOR_TEMP_SENSOR_CTRL, 0x2);    // Disable sensor (see datasheet)
    }
    return VTSS_RC_OK;
}

// Function for getting chip temperature 
//  
// In: port_no - The PHY port number starting from 0.
//
// In/Out: Temp - The chip temperature (from -46 to 135 degC)
//
// Return: VTSS_RC_OK if we got the temperature - else error code

static vtss_rc jr_temp_sensor_read(i16 *temp) 
{
    u32 reg_val;
    JR_RD(DEVCPU_GCB, TEMP_SENSOR_TEMP_SENSOR_DATA, &reg_val);
    
    // Bit 8 determines if the data is valid (see datasheet)
    if (reg_val & 0x100) {
        //135.3degC - 0.71degC*ADCOUT
        *temp = (13530 - 71 * (reg_val & 0xFF))/100;
    } else {
        *temp = 135; // Data not valid
        return VTSS_RC_ERROR;
    }
   
    return VTSS_RC_OK;
}




static vtss_rc jr_gpio_mode(const vtss_chip_no_t   chip_no,
                            const vtss_gpio_no_t   gpio_no,
                            const vtss_gpio_mode_t mode)
{
    u32 mask = VTSS_BIT(gpio_no);

    JR_WRM_CLR(DEVCPU_GCB, GPIO_GPIO_INTR_ENA, mask); /* Disable IRQ */
    switch(mode) {
    case VTSS_GPIO_OUT:
    case VTSS_GPIO_IN:
    case VTSS_GPIO_IN_INT:
        JR_WRXM_CLR(DEVCPU_GCB_GPIO_GPIO, ALT, 0, mask); /* GPIO mode 0b00 */
        JR_WRXM_CLR(DEVCPU_GCB_GPIO_GPIO, ALT, 1, mask); /* -"- */
        JR_WRM(DEVCPU_GCB_GPIO_GPIO, OE, (mode == VTSS_GPIO_OUT ? mask : 0), mask);
        if(mode == VTSS_GPIO_IN_INT)
            JR_WRM_SET(DEVCPU_GCB_GPIO_GPIO, INTR_ENA, mask);
        return VTSS_RC_OK;
    case VTSS_GPIO_ALT_0:
        JR_WRXM_SET(DEVCPU_GCB_GPIO_GPIO, ALT, 0, mask); /* GPIO mode 0b01 */
        JR_WRXM_CLR(DEVCPU_GCB_GPIO_GPIO, ALT, 1, mask); /* -"- */
        return VTSS_RC_OK;
    case VTSS_GPIO_ALT_1:
        JR_WRXM_CLR(DEVCPU_GCB_GPIO_GPIO, ALT, 0, mask); /* GPIO mode 0b10 */
        JR_WRXM_SET(DEVCPU_GCB_GPIO_GPIO, ALT, 1, mask); /* -"- */
        return VTSS_RC_OK;
    default:
        break;
    }
    return VTSS_RC_ERROR;
}


static vtss_rc jr_gpio_read(const vtss_chip_no_t  chip_no,
                             const vtss_gpio_no_t  gpio_no,
                             BOOL                  *const value)
{
    u32 val,  mask = VTSS_BIT(gpio_no);

    JR_RD(DEVCPU_GCB, GPIO_GPIO_IN, &val);
    *value = VTSS_BOOL(val & mask);
    return VTSS_RC_OK;
}

static vtss_rc jr_gpio_write(const vtss_chip_no_t  chip_no,
                             const vtss_gpio_no_t  gpio_no, 
                             const BOOL            value)
{
    u32 mask = VTSS_BIT(gpio_no);

    if(value) {
        JR_WR(DEVCPU_GCB, GPIO_GPIO_OUT_SET, mask);
    } else {
        JR_WR(DEVCPU_GCB, GPIO_GPIO_OUT_CLR, mask);
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_gpio_event_poll(const vtss_chip_no_t  chip_no,
                                  BOOL                  *const events)
{
    u32 pending, mask, i;

    JR_RD(DEVCPU_GCB, GPIO_GPIO_INTR, &pending);
    JR_RD(DEVCPU_GCB, GPIO_GPIO_INTR_ENA, &mask);
    pending &= mask;
    JR_WR(DEVCPU_GCB, GPIO_GPIO_INTR, pending);

    for (i=0; i<VTSS_GPIOS; i++) {
        events[i] = (pending & 1<<i) ? TRUE : FALSE;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_gpio_event_enable(const vtss_chip_no_t  chip_no,
                                    const vtss_gpio_no_t  gpio_no,
                                    const BOOL            enable)
{
    JR_WRM(DEVCPU_GCB, GPIO_GPIO_INTR_ENA, enable ? 1<<gpio_no : 0, 1<<gpio_no);

    return VTSS_RC_OK;
}

static vtss_rc jr_sgpio_event_poll(const vtss_chip_no_t     chip_no,
                                   const vtss_sgpio_group_t group,
                                   const u32                bit,
                                   BOOL                     *const events)
{
    u32 i, val = 0;

    JR_RDXY(DEVCPU_GCB, SIO_CTRL_SIO_INT_REG, group, bit, &val);
    JR_WRXY(DEVCPU_GCB, SIO_CTRL_SIO_INT_REG, group, bit, val);

    /* Setup serial IO port enable register */
    for (i=0; i<32; i++) {
        events[i] = (val & 1<<i) ? TRUE : FALSE;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_sgpio_event_enable(const vtss_chip_no_t     chip_no,
                                     const vtss_sgpio_group_t group,
                                     const u32                port,
                                     const u32                bit,
                                     const BOOL               enable)
{
    u32 data, pol, i;

    if (enable) {
        JR_RDXY(DEVCPU_GCB, SIO_CTRL_SIO_INPUT_DATA, group, bit, &data);
        JR_RDXY(DEVCPU_GCB, SIO_CTRL_SIO_INT_POL, group ,bit, &pol);
        pol = ~pol;     /* '0' means interrupt when input is one */
        data &= pol;    /* Now data '1' means active interrupt */
        if (!(data & 1<<port))    /* Only enable if not active interrupt - as interrupt pending cannot be cleared */
            JR_WRXM(DEVCPU_GCB, SIO_CTRL_SIO_PORT_INT_ENA, group, 1<<port, 1<<port);
        JR_WRXM(DEVCPU_GCB, SIO_CTRL_SIO_CONFIG, group, VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit), VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit));
    }
    else {
        JR_WRXM(DEVCPU_GCB, SIO_CTRL_SIO_PORT_INT_ENA, group, 0, 1<<port);
        for (i=0; i<32; ++i)
            if (vtss_state->sgpio_event_enable[group].enable[i][bit])      break;
        if (i == 32)
            JR_WRXM(DEVCPU_GCB, SIO_CTRL_SIO_CONFIG, group, 0, VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_sgpio_conf_set(const vtss_chip_no_t     chip_no,
                                 const vtss_sgpio_group_t group,
                                 const vtss_sgpio_conf_t  *const conf)
{
    u32 i, port, val = 0, bmode[2], bit_idx;

    /* Setup serial IO port enable register */
    for (port = 0; port < 32; port++) {
        if (conf->port_conf[port].enabled)
            val |= VTSS_BIT(port);
    }
    JR_WRX(DEVCPU_GCB, SIO_CTRL_SIO_PORT_ENABLE, group, val);

    /* 
     * Setup general configuration register
     *
     * The burst gap is 0x1f(33.55ms)
     * The load signal is active low
     * The auto burst repeat function is on
     * The SIO reverse output is off
     */
    for (i = 0; i < 2; i++) {
        switch (conf->bmode[i]) {
        case VTSS_SGPIO_BMODE_TOGGLE:
            if (i == 0) {
                VTSS_E("blink mode 0 does not support TOGGLE (group:%d)",group);
                return VTSS_RC_ERROR;
            }
            bmode[i] = 3;
            break;
        case VTSS_SGPIO_BMODE_0_625:
            if (i == 1) {
                VTSS_E("blink mode 1 does not support 0.625 Hz");
                return VTSS_RC_ERROR;
            }
            bmode[i] = 3;
            break;
        case VTSS_SGPIO_BMODE_1_25:
            bmode[i] = 2;
            break;
        case VTSS_SGPIO_BMODE_2_5:
            bmode[i] = 1;
            break;
        case VTSS_SGPIO_BMODE_5:
            bmode[i] = 0;
            break;
        default:
            return VTSS_RC_ERROR;
        }
    }

    val = (VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BMODE_0(bmode[0]) |
           VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BMODE_1(bmode[1]) |
           VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BURST_GAP(0x00) |
           VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_PORT_WIDTH(conf->bit_count - 1) |
           VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_AUTO_REPEAT);
    JR_WRXM(DEVCPU_GCB, SIO_CTRL_SIO_CONFIG, group, val, ~VTSS_M_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA);

    /* Setup the serial IO clock frequency - 12.5MHz (0x28) */
    JR_WRX(DEVCPU_GCB, SIO_CTRL_SIO_CLOCK, group, 0x28);

    /*
     * Configuration of output data values
     * The placement of the source select bits for each output bit in the register:
     * Output bit0 : (2 downto 0)
     * Output bit1 : (5 downto 3)
     * Output bit2 : (8 downto 6)
     * Output bit3 : (11 downto 9)
    */
    for (port = 0; port < 32; port++) {
        for (val = 0, bit_idx = 0; bit_idx < 4; bit_idx++) {
            /* Set output bit n */
            val |= VTSS_ENCODE_BITFIELD(conf->port_conf[port].mode[bit_idx], (bit_idx * 3), 3);
        }
        JR_WR(DEVCPU_GCB, SIO_CTRL_SIO_PORT_CONFIG(group, port), val);
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_sgpio_read(const vtss_chip_no_t     chip_no,
                             const vtss_sgpio_group_t group,
                             vtss_sgpio_port_data_t   data[VTSS_SGPIO_PORTS])
{
    u32 i, port, value;

    for (i = 0; i < 4; i++) {
        JR_RDXY(DEVCPU_GCB, SIO_CTRL_SIO_INPUT_DATA, group, i, &value);
        for (port = 0; port < 32; port++) {
            data[port].value[i] = VTSS_BOOL(value & (1 << port));
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_chip_id_get_chip(vtss_chip_id_t *const chip_id)
{
    u32 value;

    JR_RD(DEVCPU_GCB, CHIP_REGS_CHIP_ID, &value);

    if (value == 0 || value == 0xffffffff) {
        VTSS_E("CPU interface[%u] error, chipid: 0x%08x", vtss_state->chip_no, value);
        return VTSS_RC_ERROR;
    }

    chip_id->part_number = JR_GET_FLD(DEVCPU_GCB, CHIP_REGS_CHIP_ID, PART_ID, value);
    chip_id->revision = JR_GET_FLD(DEVCPU_GCB, CHIP_REGS_CHIP_ID, REV_ID, value);

    return VTSS_RC_OK;
}

static vtss_rc jr_chip_id_get(vtss_chip_id_t *const chip_id)
{
    return jr_chip_id_get_chip(chip_id);
}

static vtss_rc jr_intr_sticky_clear(const vtss_state_t   *const state,
                                    u32                  ext)
{
    if (state->chip_count == 2)
        JR_WR_REG_CHIP(1, VTSS_ICPU_CFG_INTR_INTR, (ext == 0) ? VTSS_F_ICPU_CFG_INTR_INTR_EXT_IRQ0_INTR : VTSS_F_ICPU_CFG_INTR_INTR_EXT_IRQ1_INTR);

    return VTSS_RC_OK;
}

static vtss_rc jr_poll_sgpio_1sec(void)
{
    vtss_rc rc = VTSS_RC_OK;
    u32     group, port, bit, enable;

    for (group=0; group<2; ++group) {   /* All SGPIO is checked for "enabled" interrupt that is actually disabled because interrupt is active - pending can not be cleared */
        JR_RDX(DEVCPU_GCB, SIO_CTRL_SIO_PORT_INT_ENA, group, &enable);
        for (port=0; port<32; ++port)
            if (!(enable & 1<<port))
                for (bit=0; bit<4; ++bit)    /* port is not enabled - check if it is configured to be */
                    if (vtss_state->sgpio_event_enable[group].enable[port][bit]) {
                        rc = jr_sgpio_event_enable(0, group, port, bit, TRUE);  /* this port,bit is configured to be enabled - try and enable */
                    }
    }
    return rc;
}

static vtss_rc jr_ptp_event_poll(vtss_ptp_event_type_t  *ev_mask)
{
    u32 sticky, mask;
    
    /* PTP events */
    *ev_mask = 0;
    JR_RD(DEVCPU_GCB, PTP_STAT_PTP_EVT_STAT, &sticky);
    JR_WR(DEVCPU_GCB, PTP_STAT_PTP_EVT_STAT, sticky);   /* Clear pending */
    JR_RD(DEVCPU_GCB, PTP_CFG_PTP_SYNC_INTR_ENA_CFG, &mask);
    mask |= VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY; /* CLK ADJ event has no enable bit - do not generate interrupt */
    sticky &= mask;      /* Only handle enabled sources */
    
    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_SYNC_STAT) ? VTSS_PTP_SYNC_EV : 0;
    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_EXT_SYNC_CURRENT_TIME_STICKY) ? VTSS_PTP_EXT_SYNC_EV : 0;
    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY) ? VTSS_PTP_CLK_ADJ_EV : 0;
    
    VTSS_D("sticky: 0x%x", sticky);
    
    return VTSS_RC_OK;
}

static vtss_rc jr_ptp_event_enable(vtss_ptp_event_type_t ev_mask, BOOL enable)
{
    
    /* PTP masks */
    if (ev_mask & VTSS_PTP_SYNC_EV) {
        JR_WRM(DEVCPU_GCB, PTP_CFG_PTP_SYNC_INTR_ENA_CFG, 
               enable ? VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_SYNC_STAT_ENA : 0, 
               VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_SYNC_STAT_ENA);
    }
    if (ev_mask & VTSS_PTP_EXT_SYNC_EV) {
        JR_WRM(DEVCPU_GCB, PTP_CFG_PTP_SYNC_INTR_ENA_CFG, 
               enable ? VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_EXT_SYNC_CURRENT_TIME_ENA : 0, 
               VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_EXT_SYNC_CURRENT_TIME_ENA);
    }
    
    return VTSS_RC_OK;
}


static vtss_rc jr_dev_all_event_poll(vtss_dev_all_event_poll_t poll_type,  vtss_dev_all_event_type_t  *ev_mask)
{
    u32 sticky=0, mask=0, ident=0, chip_port;
    vtss_port_no_t port;
    
    /* poll timestamp events */
    for (port=VTSS_PORT_NO_START; port<vtss_state->port_count; port++) {
        ev_mask[port] = 0;

        if ((poll_type == VTSS_DEV_ALL_POLL_ALL) ||
            ((poll_type == VTSS_DEV_ALL_POLL_PRIMARY) && (VTSS_CHIP_NO(port) == 0)) ||
            ((poll_type == VTSS_DEV_ALL_POLL_SECONDARY) && (VTSS_CHIP_NO(port) == 1))) { /* This chip needs to be polled */
            VTSS_SELECT_CHIP_PORT_NO(port);     /* Select the chip for this port */
            chip_port = VTSS_CHIP_PORT(port);
    
            JR_RD(ICPU_CFG, INTR_DEV_IDENT, &ident);
            if (ident & (1 << chip_port)) {
                if (VTSS_PORT_IS_1G(chip_port)) {
                    JR_RDX(DEV1G, DEV1G_INTR_CFG_STATUS_DEV1G_INTR, VTSS_TO_DEV(chip_port), &sticky);
                    JR_WRX(DEV1G, DEV1G_INTR_CFG_STATUS_DEV1G_INTR, VTSS_TO_DEV(chip_port), sticky);    /* Clear pending */
                    JR_RDX(DEV1G, DEV1G_INTR_CFG_STATUS_DEV1G_INTR_CFG, VTSS_TO_DEV(chip_port), &mask);
                    sticky &= mask;      /* Only handle enabled sources */
        
                    ev_mask[port] |= (sticky & VTSS_F_DEV1G_DEV1G_INTR_CFG_STATUS_DEV1G_INTR_PTP_UPDATE_INTR_STICKY) ? VTSS_DEV_ALL_TX_TSTAMP_EV : 0;
                    ev_mask[port] |= (sticky & (VTSS_F_DEV1G_DEV1G_INTR_CFG_STATUS_DEV1G_INTR_LINK_UP_INTR_STICKY | VTSS_F_DEV1G_DEV1G_INTR_CFG_STATUS_DEV1G_INTR_LINK_DOWN_INTR_STICKY)) ? VTSS_DEV_ALL_LINK_EV : 0;
                } else {
                    JR_RDX(DEV10G, DEV_CFG_STATUS_INTR, VTSS_TO_DEV(chip_port), &sticky);
                    JR_WRX(DEV10G, DEV_CFG_STATUS_INTR, VTSS_TO_DEV(chip_port), sticky);    /* Clear pending */
                    JR_RDX(DEV10G, DEV_CFG_STATUS_INTR_ENA, VTSS_TO_DEV(chip_port), &mask);
                    sticky &= mask;      /* Only handle enabled sources */
        
                    ev_mask[port] |= (sticky & VTSS_F_DEV10G_DEV_CFG_STATUS_INTR_PTP_UPDT_INTR) ? VTSS_DEV_ALL_TX_TSTAMP_EV : 0;
                    ev_mask[port] |= (sticky & (VTSS_F_DEV10G_DEV_CFG_STATUS_INTR_LINK_UP_INTR | VTSS_F_DEV10G_DEV_CFG_STATUS_INTR_LINK_DWN_INTR)) ? VTSS_DEV_ALL_LINK_EV : 0;
                }
            }
        }
    }
    
    VTSS_D("sticky: 0x%x", sticky);
    
    return VTSS_RC_OK;
}

static vtss_rc jr_dev_all_event_enable(vtss_port_no_t port, vtss_dev_all_event_type_t ev_mask, BOOL enable)
{
    u32 chip_port, mask=0;
    
    if (ev_mask == 0) return VTSS_RC_OK;

    VTSS_SELECT_CHIP_PORT_NO(port);     /* Select the chip for this port */
    chip_port = VTSS_CHIP_PORT(port);

    if (VTSS_PORT_IS_1G(chip_port)) {
        if (ev_mask & VTSS_DEV_ALL_TX_TSTAMP_EV)    mask |= VTSS_F_DEV1G_DEV1G_INTR_CFG_STATUS_DEV1G_INTR_PTP_UPDATE_INTR_STICKY;
        if (ev_mask & VTSS_DEV_ALL_LINK_EV)         mask |= (VTSS_F_DEV1G_DEV1G_INTR_CFG_STATUS_DEV1G_INTR_LINK_UP_INTR_STICKY | VTSS_F_DEV1G_DEV1G_INTR_CFG_STATUS_DEV1G_INTR_LINK_DOWN_INTR_STICKY);
        JR_WRXM(DEV1G, DEV1G_INTR_CFG_STATUS_DEV1G_INTR_CFG, VTSS_TO_DEV(chip_port), enable ? mask : 0, mask);
    } else {
        if (ev_mask & VTSS_DEV_ALL_TX_TSTAMP_EV)    mask |= VTSS_F_DEV10G_DEV_CFG_STATUS_INTR_PTP_UPDT_INTR;
        if (ev_mask & VTSS_DEV_ALL_LINK_EV)         mask |= (VTSS_F_DEV10G_DEV_CFG_STATUS_INTR_LINK_UP_INTR | VTSS_F_DEV10G_DEV_CFG_STATUS_INTR_LINK_DWN_INTR);
        JR_WRXM(DEV10G, DEV_CFG_STATUS_INTR_ENA, VTSS_TO_DEV(chip_port), enable ? mask : 0, mask);
    }
    
    return VTSS_RC_OK;
}

static vtss_rc jr_reg_read(const vtss_chip_no_t chip_no, const u32 addr, u32 *const value)
{
    /* Do not change this to calling jr_rd() without chip_no, since it's also called by the FDMA */
    return jr_rd_chip(chip_no, addr, value);
}

static vtss_rc jr_reg_write(const vtss_chip_no_t chip_no, const u32 addr, const u32 value)
{
    /* Do not change this to calling jr_wr() without chip_no, since it's also called by the FDMA */
    return jr_wr_chip(chip_no, addr, value);
}

/* Configures the Serdes1G/Serdes6G blocks based on mux mode and Target */
static vtss_rc serdes_macro_config (void)
{
    vtss_port_mux_mode_t mux_mode = vtss_state->mux_mode[vtss_state->chip_no];
    vtss_target_type_t   target = jr_target_get();
    u32                  mask_sgmii = 0, mask_2g5 = 0, mask_xaui = 0;

    if (mux_mode == VTSS_PORT_MUX_MODE_0) {
        /* Multiplexer for interconnection between 10G/2G5 */
        JR_WR(DEVCPU_GCB, VAUI2_MUX_VAUI2_MUX, 0);
        /* Multiplexer for interconnection between 10G/2G5: 4x2G5 */
    } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
        JR_WR(DEVCPU_GCB, VAUI2_MUX_VAUI2_MUX, 1);
        /* Disable VAUI 0 Lane Sync Mechanism for 4 independent lanes... */
        JR_WRXF(MACRO_CTRL, VAUI_CHANNEL_CFG_VAUI_CHANNEL_CFG, 0, LANE_SYNC_ENA, 0);
    } else if (mux_mode == VTSS_PORT_MUX_MODE_7) {
        /* Multiplexer for interconnection between 10G/2G5: 8x2G5 */
        JR_WR(DEVCPU_GCB, VAUI2_MUX_VAUI2_MUX, 7);
        /* Disable VAUI 0 and 1 Lane Sync Mechanism for 4 independent lanes... */
        JR_WRXF(MACRO_CTRL, VAUI_CHANNEL_CFG_VAUI_CHANNEL_CFG, 0, LANE_SYNC_ENA, 0);
        JR_WRXF(MACRO_CTRL, VAUI_CHANNEL_CFG_VAUI_CHANNEL_CFG, 1, LANE_SYNC_ENA, 0);
    }

    switch (target) {
    case VTSS_TARGET_JAGUAR_1:
    case VTSS_TARGET_CE_MAX_24:
    case VTSS_TARGET_E_STAX_III_48:
    case VTSS_TARGET_E_STAX_III_68:
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            /* 24x1G + 4x10G + NPI */            
            mask_sgmii = 0x1ffffff; /* SGMII mode (24 SGMII + 1 RGMII) */
            mask_xaui = 0xffff;     /* XAUI mode, lane 0-15 */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
            /* 23x1G + 4x2g5 + 3x10G + NPI */
            mask_sgmii = 0x17fffff; /* SGMII mode (23 SGMII + 1 RGMII) */
            mask_2g5 = 0xf;         /* 2.5G mode, lane 0-3 */
            mask_xaui = 0xfff0;     /* XAUI mode, lane 4-15 */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_7) {
            /* 19x1G + 8x2g5 + 2x10G + NPI */
            mask_sgmii = 0x107ffff; /* SGMII mode (19 SGMII + 1 RGMII) */
            mask_2g5 = 0xff;        /* 2.5G mode, lane 0-7 */
            mask_xaui = 0xff00;     /* XAUI mode, lane 8-15 */
        } else {
            VTSS_E("Mux mode no supported");
        }
        break;
    case VTSS_TARGET_LYNX_1:
    case VTSS_TARGET_CE_MAX_12:
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            /* 12x1G + 2x10G + NPI */
            mask_sgmii = 0x17ff800; /* SGMII mode (11-22 SGMII + 1 RGMII) */
            mask_xaui = 0xff;       /* XAUI mode, lane 0-7 */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
            /* 12x1G + 4x2g5 + 1x10g + NPI */
            mask_sgmii = 0x17ff800; /* SGMII mode, (11-22 SGMII + 1 RGMII) */
            mask_2g5 = 0xf;         /* 2.5G mode, lane 0-3 */
            mask_xaui = 0xf0;       /* XAUI mode, lane 4-7 */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_7) {
            if (target == VTSS_TARGET_CE_MAX_12) {
                VTSS_E("Mux mode no supported");
                return VTSS_RC_ERROR;
            }
            /* 8x1G + 8x2g5 + NPI */
            mask_sgmii = 0x107f800; /* SGMII mode, (11-18 SGMII + 1 RGMII) */
            mask_2g5 = 0xf;         /* 2.5G mode, lane 0-3 */
        } else {
            VTSS_E("Mux mode no supported");
            return VTSS_RC_ERROR;
        }
        if (target == VTSS_TARGET_LYNX_1) {
            mask_xaui |= 0xff00;    /* XAUI mode, lane 8-15 */
        }
        break;
    default:
        VTSS_E("Serdes macro - Unknown Target");
        return VTSS_RC_ERROR;
    }
    
    if (mask_sgmii) {
        VTSS_RC(jr_sd1g_cfg(VTSS_SERDES_MODE_SGMII, mask_sgmii));
    }
    if (mask_2g5) {
        VTSS_RC(jr_sd6g_cfg(VTSS_SERDES_MODE_2G5, mask_2g5));
    }
    if (mask_xaui) {
        VTSS_RC(jr_sd6g_cfg(VTSS_SERDES_MODE_XAUI, mask_xaui));
    }
    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_SYNCE)
/* ================================================================= *
 *  SYNCE (Level 1 syncronization)
 * ================================================================= */

static vtss_rc jr_synce_check_target(void)
{
    vtss_rc rc;

    switch (jr_target_get()) {
    case VTSS_TARGET_JAGUAR_1:
    case VTSS_TARGET_CE_MAX_24:
    case VTSS_TARGET_LYNX_1:
    case VTSS_TARGET_CE_MAX_12:
        rc = VTSS_RC_OK;
        break;
    default:
        rc = VTSS_RC_ERROR;
        break;
    }
    return rc;
}


static vtss_rc jr_synce_clock_out_set(const u32   clk_port)
{
    u32 eth_cfg, div_mask, en_mask;

//printf("jr_synce_clock_out_set  clk_port %X  enable %u\n", clk_port, vtss_state->synce_out_conf[clk_port].enable);
    VTSS_RC(jr_synce_check_target());

    div_mask = (clk_port ? VTSS_M_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_B : VTSS_M_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_A);
    en_mask = (clk_port ? VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_RECO_CLK_B_ENA : VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_RECO_CLK_A_ENA);

    JR_RD(MACRO_CTRL, SYNC_ETH_CFG_SYNC_ETH_CFG, &eth_cfg);
    eth_cfg &= ~(div_mask | en_mask);      /* clear the related bits for this configuration */
    switch (vtss_state->synce_out_conf[clk_port].divider) {
        case VTSS_SYNCE_DIVIDER_1: div_mask = (clk_port ? VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_B(0) : VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_A(0)); break;
        case VTSS_SYNCE_DIVIDER_4: div_mask = (clk_port ? VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_B(2) : VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_A(2)); break;
        case VTSS_SYNCE_DIVIDER_5: div_mask = (clk_port ? VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_B(1) : VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_A(1)); break;
    }
    eth_cfg = eth_cfg | div_mask | (vtss_state->synce_out_conf[clk_port].enable ? en_mask : 0);
//printf("eth_cfg %u\n", eth_cfg);
    JR_WR(MACRO_CTRL, SYNC_ETH_CFG_SYNC_ETH_CFG,  eth_cfg);  /* Set the related bits defending on configuration */

    return VTSS_RC_OK;
}

static vtss_rc jr_synce_clock_in_set(const u32   clk_port)
{
    serdes_t    serdes_type;
    u32         serdes_instance, common_cfg, sq_mask, en_mask;
    i32         new_chip_port = vtss_state->port_map[vtss_state->synce_in_conf[clk_port].port_no].chip_port;
    i32         old_chip_port = vtss_state->old_port_no[clk_port];
    u32         count;

//printf("jr_synce_clock_in_set  clk_port %X  enable %u\n", clk_port, vtss_state->synce_in_conf[clk_port].enable);
    VTSS_RC(jr_synce_check_target());

    if (!vtss_state->synce_in_conf[clk_port].enable || (new_chip_port != old_chip_port)) {
    /* Disable of this clock port or input port has changed for this clock output port - disable old input */
        if (VTSS_RC_OK == jr_port2serdes_get(old_chip_port, &serdes_type, &serdes_instance, &count)) {
            if (serdes_type == SERDES_1G) {
                VTSS_RC(jr_sd1g_read(1<<serdes_instance));                /* Readback the 1G common config register */
                JR_RD(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, &common_cfg);
                common_cfg &= ~(clk_port ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_RECO_SEL_B : VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_RECO_SEL_A);
//printf("1G  disable   instance %u  common_cfg %X\n", serdes_instance, common_cfg);
                JR_WR(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, common_cfg);   /* Clear the recovered clock enable */
                VTSS_RC(jr_sd1g_write(1<<serdes_instance, 0));
            }
            if (serdes_type == SERDES_6G) {
                VTSS_RC(jr_sd6g_read(1<<serdes_instance));
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, &common_cfg);
                common_cfg &= ~(clk_port ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_RECO_SEL_B : VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_RECO_SEL_A);
//printf("6G  disable   instance %u  common_cfg %X\n", serdes_instance, common_cfg);
                JR_WR(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, common_cfg);   /* Clear the recovered clock enable */
                VTSS_RC(jr_sd6g_write(1<<serdes_instance, 0));
            }
        }
    }

    if (vtss_state->synce_in_conf[clk_port].enable) {
    /* Enable input clock configuration - now configuring the new (or maybe the same) input port */
        if (VTSS_RC_OK == jr_port2serdes_get(new_chip_port, &serdes_type, &serdes_instance, &count)) {
            if (serdes_type == SERDES_1G) {
                sq_mask = (clk_port ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SE_AUTO_SQUELCH_B_ENA : VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SE_AUTO_SQUELCH_A_ENA);
                en_mask = (clk_port ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_RECO_SEL_B : VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_RECO_SEL_A);
                VTSS_RC(jr_sd1g_read(1<<serdes_instance));                /* Readback the 1G common config register */
                JR_RD(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, &common_cfg);
                common_cfg &= ~(sq_mask | en_mask);      /* clear the related bits for this configuration */
                common_cfg |= (vtss_state->synce_in_conf[clk_port].squelsh ? sq_mask : 0) | (vtss_state->synce_in_conf[clk_port].enable ? en_mask : 0);
//printf("1G  enable   instance %u  common_cfg %X\n", serdes_instance, common_cfg);
                JR_WR(MACRO_CTRL, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, common_cfg);   /* Set the related bits depending on configuration */
                VTSS_RC(jr_sd1g_write(1<<serdes_instance, 0));     /* transfer 1G common config register */
            }
            if (serdes_type == SERDES_6G) {
                sq_mask = (clk_port ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SE_AUTO_SQUELCH_B_ENA : VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SE_AUTO_SQUELCH_A_ENA);
                en_mask = (clk_port ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_RECO_SEL_B : VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_RECO_SEL_A);
                VTSS_RC(jr_sd6g_read(1<<serdes_instance));                /* Readback the 1G common config register */
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, &common_cfg);
                common_cfg &= ~(sq_mask | en_mask);      /* clear the related bits for this configuration */
                common_cfg |= (vtss_state->synce_in_conf[clk_port].squelsh ? sq_mask : 0) | (vtss_state->synce_in_conf[clk_port].enable ? en_mask : 0);
//printf("6G  enable   instance %u  common_cfg %X\n", serdes_instance, common_cfg);
                JR_WR(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, common_cfg);   /* Set the related bits depending on configuration */
                VTSS_RC(jr_sd6g_write(1<<serdes_instance, 0));     /* transfer 1G common config register */
            }
            vtss_state->old_port_no[clk_port] = new_chip_port;
        }
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_SYNCE */

#ifdef VTSS_FEATURE_EEE
/* =================================================================
 *  EEE - Energy Efficient Ethernet
 * =================================================================*/

// EEE needs a loopback port per chip. Get an unused port.
static vtss_rc jr_eee_loopback_port(void)
{
    u32 chip_port = jr_unused_chip_port();

    // If in mux-mode 1 and we need flow control on port 0, we cannot enable
    // EEE, because a host-mode register must be set to the loop port (ref.
    // the other function that uses jr_unused_chip_port()).

    // We need flow control when not in CE mode.
    if (vtss_state->mux_mode[vtss_state->chip_no] == VTSS_PORT_MUX_MODE_0 && vtss_state->create.target == VTSS_TARGET_E_STAX_III_24_DUAL) {
        VTSS_E("EEE cannot be enabled in mux-mode 0 on non-CE targets");
        return VTSS_RC_ERROR;
    }

    vtss_state->eee_loopback_chip_port[vtss_state->chip_no] = chip_port;
    return VTSS_RC_OK;
}

static vtss_rc jr_eee_ace_add_remove(void)
{
    vtss_vcap_data_t      data;
    vtss_is2_entry_t      entry;
    vtss_ace_t            *ace = &entry.ace;
    vtss_chip_no_t        chip_no;
    vtss_vcap_id_t        id = 1;

    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        // Enable/disable second IS2 lookup on loopback port
        VTSS_RC(jr_vcap_is1_is2_set(vtss_state->eee_loopback_chip_port[chip_no], 0, vtss_state->eee_ena_cnt ? IS2_LOOKUP_SECOND : IS2_LOOKUP_NONE));
    }

    if (vtss_state->eee_ena_cnt) {
        // Add block of default entries
        memset(&data, 0, sizeof(data));
        memset(&entry, 0, sizeof(entry));
        data.u.is2.entry = &entry;
        for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
            entry.chip_port_mask[chip_no] = VTSS_BIT(vtss_state->eee_loopback_chip_port[chip_no]);
        }
        ace->action.port_no     = VTSS_PORT_NO_NONE;
        ace->action.irq_trigger = TRUE;
        ace->id                 = id;
        VTSS_RC(jr_is2_add_any(VTSS_IS2_USER_EEE, id, VTSS_VCAP_ID_LAST, &data, 1));
    } else {
        // Delete block of default entries
        VTSS_RC(jr_is2_del_any(VTSS_IS2_USER_EEE, id, 1));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_eee_port_conf_set(const vtss_port_no_t port_no, const vtss_eee_port_conf_t *const eee_conf)
{
    vtss_chip_no_t   chip_no;
    u32              loopback_port;
    vtss_port_conf_t port_conf;
    BOOL             reconfigure;

    if (vtss_state->eee_ena[port_no] == eee_conf->eee_ena) {
        return VTSS_RC_OK;
    }

    vtss_state->eee_ena[port_no] = eee_conf->eee_ena;

    if (eee_conf->eee_ena) {
        reconfigure = vtss_state->eee_ena_cnt == 0;
        vtss_state->eee_ena_cnt++;
    } else {
        vtss_state->eee_ena_cnt--;
        reconfigure = vtss_state->eee_ena_cnt == 0;
    }

    if (!reconfigure) {
        return VTSS_RC_OK;
    }

    // If we're still here, there's a state change.
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);

        // EEE needs one loopback port per chip.
        // Search for available ports in the port map.
        VTSS_RC(jr_eee_loopback_port());

        loopback_port = vtss_state->eee_loopback_chip_port[vtss_state->chip_no];

        // For EEE, we need Tx mirroring (direction == 1).
        // Set all other fields than ANA_AC:MIRROR_PROBE:PROBE_CFG.PROBE_DIRECTION to 0.
        JR_WRX(ANA_AC, MIRROR_PROBE_PROBE_CFG, JR_MIRROR_PROBE_EEE, VTSS_F_ANA_AC_MIRROR_PROBE_PROBE_CFG_PROBE_DIRECTION(1));

        // No ports are mirrored yet.
        JR_WRX(ANA_AC, MIRROR_PROBE_PROBE_PORT_CFG, JR_MIRROR_PROBE_EEE, 0);

        // Arbiter. Configure mirror port in arbiter and set type to 'Tx'.
        JR_WRX(ARB, CFG_STATUS_MIRROR_CFG, JR_MIRROR_PROBE_EEE, VTSS_F_ARB_CFG_STATUS_MIRROR_CFG_MIRROR_PROBE_CFG(loopback_port) | VTSS_F_ARB_CFG_STATUS_MIRROR_CFG_MIRROR_PROBE_TYPE);

        // Set-up port.
        memset(&port_conf, 0, sizeof(port_conf));
        port_conf.fdx              = 1;
        port_conf.max_frame_length = VTSS_MAX_FRAME_LENGTH_MAX;
        port_conf.if_type          = VTSS_PORT_INTERFACE_LOOPBACK;
        port_conf.power_down       = vtss_state->eee_ena_cnt == 0;
        port_conf.speed            = VTSS_SPEED_100M;
        VTSS_RC(jr_port_setup(VTSS_PORT_NO_NONE, loopback_port, &port_conf, 0));

        // jr_port_setup() modifies the DSM's rate to defaults for a 100Mbps port.
        // We need to change that to approx. 100Mbps. Units are 15.626Mbps, so with 6, we get 93.75 Mbps.
        JR_WRX(DSM, CFG_RATE_CTRL, loopback_port,
           VTSS_F_DSM_CFG_RATE_CTRL_FRM_GAP_COMP(12) |
           VTSS_F_DSM_CFG_RATE_CTRL_TAXI_RATE_HIGH(6) |
           VTSS_F_DSM_CFG_RATE_CTRL_TAXI_RATE_LOW(6));

        // Loop-back on the taxi bus (the earliest possible loopback position).
        // In this way, we avoid setting up the MAC.
        if (VTSS_PORT_IS_10G(loopback_port)) {
#define VTSS_DEV10G_DEV_CFG_STATUS_DEV_LB_CFG(target)            VTSS_IOREG(target,0x62)
#define VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_LB_CFG_TAXI_HOST_LB_ENA  VTSS_BIT(0)
            JR_WRXB(DEV10G, DEV_CFG_STATUS_DEV_LB_CFG, VTSS_TO_DEV(loopback_port), TAXI_HOST_LB_ENA, vtss_state->eee_ena_cnt ? 1 : 0);
            // Set the rate ctrl watermark to minimize the latency from frame arrival to analyzer interrupt.
            JR_WRX(DSM, CFG_RATE_CTRL_WM_DEV10G, (loopback_port - 27), 0x80);
        } else {
            JR_WRXB(DEV1G,  DEV_CFG_STATUS_DEV_LB_CFG, VTSS_TO_DEV(loopback_port), TAXI_HOST_LB_ENA, vtss_state->eee_ena_cnt ? 1 : 0);
            // Set the rate ctrl watermark to minimize the latency from frame arrival to analyzer interrupt.
            JR_WRX(DSM, CFG_RATE_CTRL_WM_DEV2G5, (loopback_port - 18), 0x80);
        }

        // Setup logical port numbers
        JR_WRX(DSM, CM_CFG_LPORT_NUM_CFG, loopback_port, loopback_port);
        JR_WRX(ANA_CL_2, PORT_PORT_ID_CFG, loopback_port, JR_PUT_FLD(ANA_CL_2, PORT_PORT_ID_CFG, LPORT_NUM, loopback_port));
    }

    // Create IS2 rules on the loopback ports that kill all frames,
    // but generates an interrupt to the CPU.
    VTSS_RC(jr_eee_ace_add_remove());

    return VTSS_RC_OK;
}

static vtss_rc jr_eee_port_state_set(const vtss_state_t          *const state,
                                     const vtss_port_no_t               port_no,
                                     const vtss_eee_port_state_t *const eee_state)
{
    u32            chip_port = VTSS_CHIP_PORT_FROM_STATE(state, port_no);
    vtss_chip_no_t chip_no   = VTSS_CHIP_NO_FROM_STATE  (state, port_no);

    switch (eee_state->select) {
    case VTSS_EEE_STATE_SELECT_LPI: {
// Register defines not available in the general JR register header files.
#define VTSS_DEV1G_PCS1G_CFG_STATUS_PCS1G_LPI_CFG(target)            VTSS_IOREG(target, 0x23)
#define VTSS_F_DEV1G_PCS1G_CFG_STATUS_PCS1G_LPI_CFG_TX_ASSERT_LPIDLE VTSS_BIT(0)
        JR_WRXB_CHIP(chip_no, DEV1G, PCS1G_CFG_STATUS_PCS1G_LPI_CFG, VTSS_TO_DEV(chip_port), TX_ASSERT_LPIDLE, eee_state->val ? 1 : 0);
        break;
    }

    case VTSS_EEE_STATE_SELECT_SCH:
        JR_WRXF_CHIP(chip_no, SCH, QSIF_QSIF_ETH_PORT_CTRL, chip_port, ETH_TX_DIS, eee_state->val ? 0 : 0xFF);
        break;

    case VTSS_EEE_STATE_SELECT_FP: {
        u32 one_hot = VTSS_BIT(chip_port);
        u32 mask;
        JR_RDX_CHIP(chip_no, ANA_AC, MIRROR_PROBE_PROBE_PORT_CFG, JR_MIRROR_PROBE_EEE, &mask);
        if (eee_state->val) {
            mask |= one_hot;
        } else {
            mask &= ~one_hot;
        }
        JR_WRX_CHIP(chip_no, ANA_AC, MIRROR_PROBE_PROBE_PORT_CFG, JR_MIRROR_PROBE_EEE, mask);
        break;
    }

    case VTSS_EEE_STATE_SELECT_INTR_ACK: {
        JR_WR_CHIP(eee_state->val, ANA_L2, COMMON_INTR, VTSS_F_ANA_L2_COMMON_INTR_VCAP_S2_INTR);
        break;
    }

    case VTSS_EEE_STATE_SELECT_INTR_ENA: {
        JR_WRB_CHIP(eee_state->val, ANA_L2, COMMON_INTR_ENA, VCAP_S2_INTR_ENA, 1);
        break;
    }

    default:
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_eee_port_counter_get(const vtss_state_t        *const state,
                                       const vtss_port_no_t             port_no,
                                       vtss_eee_port_counter_t   *const eee_counter)
{
    u32            chip_port = VTSS_CHIP_PORT_FROM_STATE(state, port_no);
    vtss_chip_no_t chip_no   = VTSS_CHIP_NO_FROM_STATE(state, port_no);
    u8             qu;
    u32            cell_cnt;

// Define register not defined in header files
#define VTSS_OQS_QU_MEM_RAM_STATUS_QU_MEM_ALLOC_STATUS(ri)  VTSS_IOREG(VTSS_TO_OQS, 0x8c00 + (ri))
#define  VTSS_F_OQS_QU_MEM_RAM_STATUS_QU_MEM_ALLOC_STATUS_QU_MEM_ALLOC(x)  VTSS_ENCODE_BITFIELD(x,0,15)
#define  VTSS_M_OQS_QU_MEM_RAM_STATUS_QU_MEM_ALLOC_STATUS_QU_MEM_ALLOC     VTSS_ENCODE_BITMASK(0,15)
#define  VTSS_X_OQS_QU_MEM_RAM_STATUS_QU_MEM_ALLOC_STATUS_QU_MEM_ALLOC(x)  VTSS_EXTRACT_BITFIELD(x,0,15)

    if (eee_counter->fill_level_get) {
        eee_counter->fill_level = 0;

        for (qu = 0; qu < 8; qu++) {
            JR_RDXF_CHIP(chip_no, OQS, QU_MEM_RAM_STATUS_QU_MEM_ALLOC_STATUS, 8 * chip_port + qu, QU_MEM_ALLOC, &cell_cnt);
            eee_counter->fill_level += cell_cnt * 160;
            if (eee_counter->fill_level > eee_counter->fill_level_thres) {
                break;
            }
        }
    }

    if (eee_counter->tx_out_bytes_get) {
        JR_RDX_CHIP(chip_no, ASM, DEV_STATISTICS_TX_OUT_BYTES_CNT, chip_port, &eee_counter->tx_out_bytes);
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_EEE */

/* ================================================================= *
 *  EVC
 * ================================================================= */

#if defined(VTSS_FEATURE_EVC)

/* Read and update ANA_AC/REW counters */
#define JR_SDX_CNT_GET(tgt, sdx, idx, id, cnt, clr)                     \
{                                                                       \
    u32 lsb, msb;                                                       \
    JR_RDXY(tgt, STAT_CNT_CFG_##sdx##_STAT_LSB_CNT, idx, id, &lsb);     \
    JR_RDXY(tgt, STAT_CNT_CFG_##sdx##_STAT_MSB_CNT, idx, id/2, &msb);   \
    vtss_cmn_counter_40_update(lsb, msb, cnt.bytes, clr);               \
    JR_RDXY(tgt, STAT_CNT_CFG_##sdx##_STAT_LSB_CNT, idx, id + 1, &lsb); \
    vtss_cmn_counter_32_update(lsb, cnt.frames, clr);                   \
}

/* Read and update IQS/OQS counters */
#define JR_SQS_CNT_GET(tgt, idx, cnt, clr)                      \
{                                                               \
    u32 lsb, msb;                                               \
    JR_RDXY(tgt, STAT_CNT_CFG_ISDX_STAT_LSB_CNT, idx, 0, &lsb); \
    JR_RDX(tgt, STAT_CNT_CFG_ISDX_STAT_MSB_CNT, idx, &msb);     \
    vtss_cmn_counter_40_update(lsb, msb, cnt.bytes, clr);       \
    JR_RDXY(tgt, STAT_CNT_CFG_ISDX_STAT_LSB_CNT, idx, 1, &lsb); \
    vtss_cmn_counter_32_update(lsb, cnt.frames, clr);           \
}

#define JR_SDX_CNT_ADD(c, counters, name)          \
{                                                  \
    counters->name.frames += c->name.frames.value; \
    counters->name.bytes += c->name.bytes.value;   \
}

static vtss_rc jr_sdx_counters_update(vtss_sdx_entry_t *isdx, vtss_sdx_entry_t *esdx, 
                                      vtss_evc_counters_t *const counters, BOOL clear)
{
    u32                 idx;
    vtss_sdx_counters_t *c;
    
    /* ISDX counters */
    if (isdx != NULL && isdx->port_no != VTSS_PORT_NO_NONE) {
        idx = isdx->sdx;
        c = &vtss_state->sdx_info.sdx_table[idx];
        JR_SDX_CNT_GET(ANA_AC, ISDX, idx, 0, &c->rx_green, clear);
        JR_SDX_CNT_GET(ANA_AC, ISDX, idx, 2, &c->rx_yellow, clear);
        JR_SDX_CNT_GET(ANA_AC, ISDX, idx, 4, &c->rx_red, clear);
        JR_SQS_CNT_GET(IQS, idx, &c->rx_discard, clear);

        if (counters != NULL) {
            JR_SDX_CNT_ADD(c, counters, rx_green);
            JR_SDX_CNT_ADD(c, counters, rx_yellow);
            JR_SDX_CNT_ADD(c, counters, rx_red);
            JR_SDX_CNT_ADD(c, counters, rx_discard);
        }
    }
    

    /* ESDX counters */
    if (esdx != NULL && esdx->port_no != VTSS_PORT_NO_NONE) {
        idx = esdx->sdx;
        c = &vtss_state->sdx_info.sdx_table[idx];
        JR_SQS_CNT_GET(OQS, idx, &c->tx_discard, clear);
        JR_SDX_CNT_GET(REW, ESDX, idx, 0, &c->tx_green, clear);
        JR_SDX_CNT_GET(REW, ESDX, idx, 2, &c->tx_yellow, clear);
        
        if (counters != NULL) {
            /* Add counters */
            JR_SDX_CNT_ADD(c, counters, tx_discard);
            JR_SDX_CNT_ADD(c, counters, tx_green);
            JR_SDX_CNT_ADD(c, counters, tx_yellow);
        }
    }
    return VTSS_RC_OK;
}

/* Allocate SDX, insert in list and return pointer to new entry */
static vtss_rc jr_ece_sdx_alloc(vtss_ece_entry_t *ece, vtss_port_no_t port_no, BOOL isdx)
{
    return (vtss_cmn_ece_sdx_alloc(ece, port_no, isdx) == NULL ? VTSS_RC_ERROR : VTSS_RC_OK);
}

static vtss_rc jr_evc_port_conf_set(const vtss_port_no_t port_no)
{
    /* Override tag remark mode for NNI ports */
    return jr_qos_port_conf_set(port_no);
}

/* Map policer ID to Jaguar policer index */
static vtss_rc jr_evc_policer_idx(const vtss_evc_policer_id_t policer_id, u32 *idx)
{
    vtss_rc rc = VTSS_RC_OK;
    
    switch (policer_id) {
    case VTSS_EVC_POLICER_ID_NONE:
        *idx = JR_EVC_POLICER_NONE; 
        break;
    case VTSS_EVC_POLICER_ID_DISCARD:
        *idx = JR_EVC_POLICER_DISCARD; 
        break;
    default:
        if (policer_id < VTSS_EVC_POLICERS)
            *idx = (policer_id + JR_EVC_POLICER_RESV_CNT); /* Skip the reserved policers */
        else {
            VTSS_E("illegal policer_id: %u", policer_id);
            rc = VTSS_RC_ERROR;
        }
        break;
    }
    return rc;
}

static vtss_rc jr_evc_policer_conf_apply(const vtss_evc_policer_id_t policer_id,
                                         vtss_evc_policer_conf_t *conf)
{
    u32 i, pol_idx, scale, level, cf;
    u64 rate, interval;

    /* Map to policer index */
    VTSS_RC(jr_evc_policer_idx(policer_id, &pol_idx));

    /* Calculate scale based on maximum rate */
    cf = (conf->cm && conf->cf ? 1 : 0);
    rate = conf->eir;
    if (cf)
        rate += conf->cir;
    if (conf->cir > rate)
        rate = conf->cir;

    /* CIR/EIR are in kbps **/
    if (rate > 2097000) {
        scale = 0;
        interval = 8196721;
    } else if (rate > 262000) {
        scale = 1;
        interval = 1024590;
    } else if (rate > 32770) {
        scale = 2;
        interval = 128074;
    } else {
        scale = 3;
        interval = 16009;
    }
    
    JR_WRX(ANA_AC, SDLB_DLB_CFG, pol_idx,
           JR_PUT_BIT(ANA_AC, SDLB_DLB_CFG, COUPLING_MODE, conf->cf) |
           JR_PUT_FLD(ANA_AC, SDLB_DLB_CFG, COLOR_AWARE_LVL, conf->cm ? 0 : 3) |
           JR_PUT_FLD(ANA_AC, SDLB_DLB_CFG, CIR_INC_DP_VAL, 1) |
           JR_PUT_FLD(ANA_AC, SDLB_DLB_CFG, GAP_VAL, 20) |
           JR_PUT_FLD(ANA_AC, SDLB_DLB_CFG, TIMESCALE_VAL, scale));
    
    /* Setup CIR/PIR policers */
    for (i = 0; i < 2; i++) {
        if (i == 0) {
            rate = conf->cir;
            level = conf->cbs;
        } else{
            rate = conf->eir;
            level = conf->ebs;
            if (cf)
                rate += conf->cir;
        }
        rate = VTSS_DIV64((rate * 1000), interval);
        if (rate > 0x7ff || conf->enable == 0)
            rate = 0x7ff;
        level /= 2048;
        if (level > 0x7f)
            level = 0x7f;
        JR_WRXY(ANA_AC, SDLB_LB_CFG, pol_idx, i, 
                JR_PUT_FLD(ANA_AC, SDLB_LB_CFG, THRES_VAL, level) |
                JR_PUT_FLD(ANA_AC, SDLB_LB_CFG, RATE_VAL, rate));
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_evc_policer_conf_set(const vtss_evc_policer_id_t policer_id)
{
    return jr_evc_policer_conf_apply(policer_id, &vtss_state->evc_policer_conf[policer_id]);
}

static vtss_vcap_id_t jr_evc_vcap_id(vtss_ece_id_t ece_id, vtss_port_no_t port_no)
{
    vtss_vcap_id_t id = port_no;

    return ((id << 32) + ece_id);
}

/* Update ISDX entry for UNI/NNI */
static vtss_rc jr_ece_isdx_update(vtss_evc_entry_t *evc, vtss_ece_entry_t *ece, 
                                  vtss_sdx_entry_t *isdx)
{
    BOOL                  isdx_ena = (evc->learning ? 0 : 1);
    u32                   upspn = 0, pol_idx;
    vtss_sdx_entry_t      *esdx;
    vtss_evc_policer_id_t policer_id;

    if (isdx_ena) {
        for (esdx = ece->esdx_list; esdx != NULL; esdx = esdx->next) {
            if (esdx->port_no != isdx->port_no) {
                upspn = VTSS_CHIP_PORT(esdx->port_no);
            }
        }
    }
    
    JR_WRX(ANA_L2, ISDX_SERVICE_CTRL, isdx->sdx, 
           JR_PUT_FLD(ANA_L2, ISDX_SERVICE_CTRL, FWD_ADDR, upspn) |
           JR_PUT_BIT(ANA_L2, ISDX_SERVICE_CTRL, ES0_ISDX_KEY_ENA, isdx_ena) |
           JR_PUT_BIT(ANA_L2, ISDX_SERVICE_CTRL, CDA_FWD_ENA, 1) |
           JR_PUT_BIT(ANA_L2, ISDX_SERVICE_CTRL, ISDX_BASED_FWD_ENA, isdx_ena) |
           JR_PUT_FLD(ANA_L2, ISDX_SERVICE_CTRL, FWD_TYPE, 0));

    
    /* Map to policer index */
    if (vtss_state->evc_port_info[isdx->port_no].nni_count)
        policer_id = VTSS_EVC_POLICER_ID_NONE;
    else if ((policer_id = ece->policer_id) == VTSS_EVC_POLICER_ID_EVC)
        policer_id = evc->policer_id;
    VTSS_RC(jr_evc_policer_idx(policer_id, &pol_idx));
    JR_WRX(ANA_L2, ISDX_DLB_CFG, isdx->sdx, 
           JR_PUT_FLD(ANA_L2, ISDX_DLB_CFG, DLB_IDX, pol_idx));
    return VTSS_RC_OK;
}

/* Add IS0 entry for UNI/NNI */
static vtss_rc jr_ece_is0_add(vtss_vcap_id_t id, vtss_vcap_id_t id_next,
                              vtss_evc_entry_t *evc, vtss_ece_entry_t *ece, 
                              vtss_sdx_entry_t *isdx)
{
    vtss_is0_entry_t  is0;
    vtss_is0_key_t    *key = &is0.key;
    vtss_is0_action_t *action = &is0.action;
    vtss_vcap_data_t  data;
    vtss_vcap_tag_t   *tag;
    
    memset(&data, 0, sizeof(data));
    memset(&is0, 0, sizeof(is0));
    data.u.is0.entry = &is0; 
    key->port_no = isdx->port_no;
    key->type = VTSS_IS0_TYPE_DBL_VID;

    if (vtss_state->evc_port_info[isdx->port_no].nni_count && 
        (ece->act_flags & VTSS_ECE_ACT_DIR_ONE) == 0) {
        /* NNI port with DIR_BOTH */
        tag = &key->data.dbl_vid.outer_tag;
        tag->tagged = VTSS_VCAP_BIT_1;
        tag->vid.value = evc->vid;
        tag->vid.mask = 0xfff;
        /* NNI->UNI: Match the same PCP as pushed in the UNI->NNI direction.
           Any DEI is matched, because the DEI may be changed by policers */
        if (ece->act_flags & VTSS_ECE_ACT_OT_PCP_MODE_FIXED) {
            /* Fixed PCP/DEI */
            tag->pcp.value = ece->ot_pcp;
            tag->pcp.mask = 0x7;
        } else {
            /* Preserved PCP/DEI */
            tag->pcp = ece->pcp;
        }
        
        /* NNI->UNI: Pop the same number of tags as pushed on the UNI->NNI direction */
        tag = &key->data.dbl_vid.inner_tag;
        if (ece->act_flags & VTSS_ECE_ACT_IT_TYPE_USED) {
            /* Inner tag used */
            action->vlan_pop_cnt = 2;
            tag->tagged = VTSS_VCAP_BIT_1;
            tag->vid.value = ece->it_vid;
            tag->vid.mask = 0xfff;
            if (ece->act_flags & VTSS_ECE_ACT_IT_PCP_MODE_FIXED) {
                /* Fixed PCP/DEI */
                tag->pcp.value = ece->it_pcp;
                tag->pcp.mask = 0x7;
                tag->dei = JR_VCAP_BIT(ece->act_flags & VTSS_ECE_ACT_IT_DEI);
            } else {
                /* Preserved PCP/DEI */
                tag->pcp = ece->pcp;
                tag->dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_DEI_VLD,
                                                VTSS_ECE_KEY_TAG_DEI_1);
            }
        } else {
            /* No inner tag pushed */
            action->vlan_pop_cnt = 1;
            
            if (!(ece->act_flags & VTSS_ECE_ACT_POP_1) && 
                (ece->key_flags & VTSS_ECE_KEY_TAG_TAGGED_VLD) &&
                (ece->key_flags & VTSS_ECE_KEY_TAG_TAGGED_1)) {
                /* Tag preserved in UNI->NNI direction, match VID in NNI->UNI direction */
                tag->tagged = VTSS_VCAP_BIT_1;
                tag->s_tag = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_S_TAGGED_VLD,
                                                  VTSS_ECE_KEY_TAG_S_TAGGED_1);
                tag->vid.value = ece->vid.vr.v.value;
                tag->vid.mask = ece->vid.vr.v.mask;
                tag->pcp = ece->pcp;
                tag->dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_DEI_VLD,
                                                VTSS_ECE_KEY_TAG_DEI_1);
            }
        }
    } else {
        /* UNI port or unidirectional NNI port */
        
        /* Outer tag */
        tag = &key->data.dbl_vid.outer_tag;
        tag->tagged = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_TAGGED_VLD,
                                           VTSS_ECE_KEY_TAG_TAGGED_1);
        tag->s_tag = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_S_TAGGED_VLD,
                                          VTSS_ECE_KEY_TAG_S_TAGGED_1);
        tag->dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_DEI_VLD, VTSS_ECE_KEY_TAG_DEI_1);
        tag->vid.value = ece->vid.vr.v.value;
        tag->vid.mask = ece->vid.vr.v.mask;
        tag->pcp = ece->pcp;

        /* Inner tag */
        tag = &key->data.dbl_vid.inner_tag;
        tag->tagged = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_IN_TAG_TAGGED_VLD,
                                           VTSS_ECE_KEY_IN_TAG_TAGGED_1);
        tag->s_tag = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_IN_TAG_S_TAGGED_VLD,
                                          VTSS_ECE_KEY_IN_TAG_S_TAGGED_1);
        tag->dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_IN_TAG_DEI_VLD,
                                        VTSS_ECE_KEY_IN_TAG_DEI_1);
        tag->vid.value = ece->in_vid.vr.v.value;
        tag->vid.mask = ece->in_vid.vr.v.mask;
        tag->pcp = ece->in_pcp;
        
        if (ece->key_flags & VTSS_ECE_KEY_PROT_IPV4) {
            key->data.dbl_vid.proto = VTSS_IS0_PROTO_IPV4;
            key->data.dbl_vid.dscp.value = ece->frame.ipv4.dscp.vr.v.value;
            key->data.dbl_vid.dscp.mask = ece->frame.ipv4.dscp.vr.v.mask;
        } else if (ece->key_flags & VTSS_ECE_KEY_PROT_IPV6) {
            key->data.dbl_vid.proto = VTSS_IS0_PROTO_IPV6;
            key->data.dbl_vid.dscp.value = ece->frame.ipv6.dscp.vr.v.value;
            key->data.dbl_vid.dscp.mask = ece->frame.ipv6.dscp.vr.v.mask;
        }
        if (ece->act_flags & VTSS_ECE_ACT_POP_1)
            action->vlan_pop_cnt = 1;
        else if (ece->act_flags & VTSS_ECE_ACT_POP_2)
            action->vlan_pop_cnt = 2;
        if (ece->act_flags & VTSS_ECE_ACT_OT_PCP_MODE_FIXED) {
            /* Fixed PCP/DEI is used in outer tag ("port tag"), change classification */
            action->pcp_dei_ena = 1;
            action->pcp = ece->ot_pcp;
            action->dei = VTSS_BOOL(ece->act_flags & VTSS_ECE_ACT_OT_DEI);
        }
    }
    action->vid_ena = 1;
    action->vid = (evc->learning ? evc->ivid : 0);
    action->isdx = isdx->sdx;
    action->pag = ece->policy_no;
    return vtss_vcap_add(&vtss_state->is0.obj, VTSS_IS0_USER_EVC, id, id_next, &data, 0);
}

/* Add ES0 entry for UNI/NNI */
static vtss_rc jr_ece_es0_add(vtss_vcap_id_t id, vtss_vcap_id_t id_next,
                              vtss_evc_entry_t *evc, vtss_ece_entry_t *ece, 
                              vtss_sdx_entry_t *esdx)
{
    vtss_vcap_data_t  data;
    vtss_es0_data_t   *es0 = &data.u.es0;
    vtss_es0_entry_t  entry;
    vtss_es0_key_t    *key = &entry.key;
    vtss_es0_action_t *action = &entry.action;
    vtss_port_no_t    port_no = esdx->port_no;
    vtss_sdx_entry_t  *isdx;

    vtss_vcap_es0_init(&data, &entry);
    key->port_no = port_no;
    key->isdx_neq0 = VTSS_VCAP_BIT_1;
    action->esdx = esdx->sdx;
    if (evc->learning) {
        /* VLAN based forwarding */
        key->type = VTSS_ES0_TYPE_VID;
        key->data.vid.vid = evc->ivid;
    } else {
        /* ISDX based forwarding */
        key->type = VTSS_ES0_TYPE_ISDX;
        for (isdx = ece->isdx_list; isdx != NULL; isdx = isdx->next) {
            if (isdx->port_no != port_no) {
                key->data.isdx.isdx = isdx->sdx;
                break;
            }
        }
    } 
    if (vtss_state->evc_port_info[port_no].nni_count) {
        /* NNI port */
        action->vid_a = evc->vid;
        if (ece->act_flags & VTSS_ECE_ACT_IT_TYPE_USED) {
            /* Push two tags ("port tag" is outer and "ES0 tag" is inner) */
            action->tag = VTSS_ES0_TAG_PORT;
            action->vid_b = ece->it_vid;
            action->qos = (ece->act_flags & VTSS_ECE_ACT_IT_PCP_MODE_FIXED ?
                           VTSS_ES0_QOS_ES0 : VTSS_ES0_QOS_CLASS);
            action->pcp = ece->it_pcp;
            action->dei = VTSS_BOOL(ece->act_flags & VTSS_ECE_ACT_IT_DEI);
            action->tpid = (ece->act_flags & VTSS_ECE_ACT_IT_TYPE_C ? VTSS_ES0_TPID_C :
                            ece->act_flags & VTSS_ECE_ACT_IT_TYPE_S ? VTSS_ES0_TPID_S :
                            VTSS_ES0_TPID_PORT);
        } else {
            /* Push one tag ("port tag") */
            action->tag = VTSS_ES0_TAG_NONE;
        }
    } else {
        /* UNI port */
        if ((ece->act_flags & VTSS_ECE_ACT_OT_ENA) ||
            ((ece->act_flags & VTSS_ECE_ACT_POP_1) && 
             (ece->key_flags & VTSS_ECE_KEY_TAG_TAGGED_1) &&
             ece->vid.vr.v.mask == 0xfff && ece->vid.vr.v.value != VTSS_VID_NULL)) {
            /* NNI->UNI: Push one tag ("ES0 tag") */
            es0->port_no = port_no;
            es0->flags = VTSS_ES0_FLAG_TPID;
            action->tag = VTSS_ES0_TAG_ES0;
            action->vid_b = (ece->act_flags & VTSS_ECE_ACT_OT_ENA ? ece->ot_vid : 
                             ece->vid.vr.v.value);
            if (ece->act_flags & VTSS_ECE_ACT_OT_PCP_MODE_FIXED) {
                /* Fixed PCP/DEI */
                action->qos = VTSS_ES0_QOS_ES0;
                if (ece->act_flags & VTSS_ECE_ACT_OT_ENA) {
                    /* Direction NNI_TO_UNI */
                    action->pcp = ece->ot_pcp;
                    if (ece->act_flags & VTSS_ECE_ACT_OT_DEI)
                        action->dei = 1;
                } else {
                    /* Direction BOTH, use lowest PCP matching the ingress PCP value/mask */
                    for (action->pcp = 0; action->pcp < 8; action->pcp++)
                        if ((action->pcp & ece->pcp.mask) == (ece->pcp.value & ece->pcp.mask))
                            break;
                    if (ece->key_flags & VTSS_ECE_KEY_TAG_DEI_1)
                        action->dei = 1;
                }
            } else {
                /* Preserved PCP/DEI, use classified values */
                action->qos = VTSS_ES0_QOS_CLASS;
            }
        } else {
            /* NNI->UNI: Do not push any tag */
            action->tag = VTSS_ES0_TAG_NONE;
        }
    }
    return vtss_vcap_add(&vtss_state->es0.obj, VTSS_ES0_USER_EVC, id, id_next, &data, 0);
}

/* Determine if IS0 rule is needed for UNI/NNI port */
static BOOL jr_ece_is0_needed(BOOL nni, vtss_ece_dir_t dir) 
{
    return (nni ? 
            (dir == VTSS_ECE_DIR_UNI_TO_NNI ? 0 : 1) :
            (dir == VTSS_ECE_DIR_NNI_TO_UNI ? 0 : 1));
}

/* Determine if ES0 rule is needed for UNI/NNI port */
static BOOL jr_ece_es0_needed(BOOL nni, vtss_ece_dir_t dir) 
{
    return (nni ? 
            (dir == VTSS_ECE_DIR_NNI_TO_UNI ? 0 : 1) :
            (dir == VTSS_ECE_DIR_UNI_TO_NNI ? 0 : 1));
}

static vtss_rc jr_ece_update(vtss_ece_entry_t *ece, vtss_res_t *res, vtss_res_cmd_t cmd)
{
    vtss_evc_entry_t *evc;
    vtss_ece_id_t    ece_id;
    vtss_ece_entry_t *ece_next;
    vtss_port_no_t   port_no;
    vtss_vcap_id_t   id, id_next_is0, id_next_es0;
    vtss_sdx_entry_t *sdx, *sdx_next;
    BOOL             nni;
    vtss_ece_dir_t   dir;

    if (cmd == VTSS_RES_CMD_CALC) {
        /* Calculate resource usage */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            nni = res->port_nni[port_no];
            if (res->port_add[port_no] || res->port_chg[port_no]) {
                /* Add/change port */
                if (jr_ece_is0_needed(nni, res->dir_new)) {
                    res->is0.add++;
                    res->isdx.add++;
                }
                if (jr_ece_es0_needed(nni, res->dir_new)) {
                    res->es0.add++;
                    res->esdx.add++;
                }
            }
            
            if (res->port_del[port_no] || res->port_chg[port_no]) {
                /* Delete/change port */
                if (jr_ece_is0_needed(nni, res->dir_old)) {
                    res->is0.del++;
                    res->isdx.del++;
                }
                if (jr_ece_es0_needed(nni, res->dir_old)) {
                    res->es0.del++;
                    res->esdx.del++;
                }
            }
        }
        return VTSS_RC_OK;
    }

    evc = &vtss_state->evc_info.table[ece->evc_id];
    ece_id = ece->ece_id;
    
    if (cmd == VTSS_RES_CMD_DEL) {
        /* Delete resources */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (res->port_del[port_no] || res->port_chg[port_no]) {
                /* Port deleted or changed (e.g. direction) */
                nni = res->port_nni[port_no];
                id = jr_evc_vcap_id(ece_id, port_no);
                
                /* Delete IS0 entry and ISDX */
                if (jr_ece_is0_needed(nni, res->dir_old)) {
                    VTSS_RC(vtss_vcap_del(&vtss_state->is0.obj, VTSS_IS0_USER_EVC, id));
                    VTSS_RC(vtss_cmn_ece_sdx_free(ece, port_no, 1));
                }
                
                /* Delete ES0 entry and ESDX */
                if (jr_ece_es0_needed(nni, res->dir_old)) {
                    VTSS_RC(vtss_vcap_del(&vtss_state->es0.obj, VTSS_ES0_USER_EVC, id));
                    VTSS_RC(vtss_cmn_ece_sdx_free(ece, port_no, 0));
                }
            }
        }
    } else if (cmd == VTSS_RES_CMD_ADD) {
        /* Add resources */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (res->port_add[port_no] || res->port_chg[port_no]) {
                /* Allocate ISDX */
                nni = res->port_nni[port_no];
                if (jr_ece_is0_needed(nni, res->dir_new))
                    VTSS_RC(jr_ece_sdx_alloc(ece, port_no, 1));

                /* Allocate ESDX */
                if (jr_ece_es0_needed(nni, res->dir_new))
                    VTSS_RC(jr_ece_sdx_alloc(ece, port_no, 0));
            }
        }
        
        /* Find next VCAP IDs */
        id_next_is0 = VTSS_VCAP_ID_LAST;
        id_next_es0 = VTSS_VCAP_ID_LAST;
        for (ece_next = ece->next; ece_next != NULL; ece_next = ece_next->next) {
            dir = vtss_cmn_ece_dir_get(ece_next);
            for (sdx = ece_next->isdx_list; sdx != NULL && id_next_is0 == VTSS_VCAP_ID_LAST; 
                 sdx = sdx->next) {
                nni = (vtss_state->evc_port_info[sdx->port_no].nni_count ? 1 : 0);
                if (jr_ece_is0_needed(nni, dir))
                    id_next_is0 = jr_evc_vcap_id(ece_next->ece_id, sdx->port_no);
                
            }
            for (sdx = ece_next->esdx_list; sdx != NULL && id_next_es0 == VTSS_VCAP_ID_LAST; 
                 sdx = sdx->next) {
                nni = (vtss_state->evc_port_info[sdx->port_no].nni_count ? 1 : 0);
                if (jr_ece_es0_needed(nni, dir))
                    id_next_es0 = jr_evc_vcap_id(ece_next->ece_id, sdx->port_no);
            }
            if (id_next_is0 != VTSS_VCAP_ID_LAST && id_next_es0 != VTSS_VCAP_ID_LAST)
                break;
        }
        
        /* Add/update IS0, ES0 and ISDX entries */
        sdx_next = NULL;
        while (sdx_next != ece->isdx_list) {
            /* Start with the last ISDX */
            for (sdx = ece->isdx_list; sdx != NULL; sdx = sdx->next) {
                if (sdx->next == sdx_next) {
                    /* Update ISDX entry */
                    VTSS_RC(jr_ece_isdx_update(evc, ece, sdx));

                    /* Add IS0 entry */
                    id = jr_evc_vcap_id(ece_id, sdx->port_no);
                    VTSS_RC(jr_ece_is0_add(id, id_next_is0, evc, ece, sdx));
                    id_next_is0 = id;
                    sdx_next = sdx;
                    break;
                }
            }
        }

        sdx_next = NULL;
        while (sdx_next != ece->esdx_list) {
            /* Start with the last ESDX */
            for (sdx = ece->esdx_list; sdx != NULL; sdx = sdx->next) {
                if (sdx->next == sdx_next) {
                    /* Add ES0 entry */
                    id = jr_evc_vcap_id(ece_id, sdx->port_no);
                    VTSS_RC(jr_ece_es0_add(id, id_next_es0, evc, ece, sdx));
                    id_next_es0 = id;
                    sdx_next = sdx;
                    break;
                }
            }
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_evc_update(vtss_evc_id_t evc_id, vtss_res_t *res, vtss_res_cmd_t cmd)
{
    vtss_ece_entry_t *ece;
    
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        if (ece->evc_id == evc_id) {
            res->dir_old = vtss_cmn_ece_dir_get(ece);
            res->dir_new = res->dir_old;
            VTSS_RC(jr_ece_update(ece, res, cmd));
        }
    }
    return VTSS_RC_OK;

}

static vtss_rc jr_evc_poll_1sec(void)
{
    vtss_sdx_info_t  *sdx_info = &vtss_state->sdx_info;
    vtss_sdx_entry_t *isdx, *esdx;
    u32              i, idx;

    /* Poll counters for 20 SDX entries, giving 4095/20 = 205 seconds between each poll.
       This ensures that any counter can wrap only once between each poll.
       The worst case is a 32-bit frame counter on a 10Gbps port, which takes about
       0xffffffff/14.880.000.000 = 288 seconds to wrap. */
    for (i = 0; i < 20; i++) {
        idx = sdx_info->poll_idx;
        isdx = &sdx_info->isdx.table[idx];
        esdx = &sdx_info->esdx.table[idx];
        idx++;
        sdx_info->poll_idx = (idx < sdx_info->max_count ? idx : 0);
        VTSS_RC(jr_sdx_counters_update(isdx, esdx, NULL, 0));
    }
    return VTSS_RC_OK;
}

static void jr_evc_init(void)
{
    /* Setup number of EVCs, ECEs and SDX values*/
    vtss_state->evc_info.max_count = VTSS_EVCS;
    vtss_state->ece_info.max_count = VTSS_ECES;
    vtss_state->sdx_info.max_count = VTSS_JR1_SDX_CNT;
}
#endif /* VTSS_FEATURE_EVC */

#ifdef VTSS_FEATURE_SFLOW
/* ================================================================= *
 *  SFLOW
 * ================================================================= */

static u32 next_power_of_two(u32 x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

/**
 * jr_sflow_hw_rate()
 */
static u32 jr_sflow_hw_rate(const u32 desired_sw_rate, u32 *const realizable_sw_rate)
{
    u32 hw_rate         = desired_sw_rate ? MAX(VTSS_ROUNDING_DIVISION(JR_SFLOW_MAX_SAMPLE_RATE, desired_sw_rate), JR_SFLOW_MIN_SAMPLE_RATE) : 0;
    *realizable_sw_rate = hw_rate         ?     VTSS_ROUNDING_DIVISION(JR_SFLOW_MAX_SAMPLE_RATE, hw_rate) : 0;
    return hw_rate;
}

/**
 * jr_sflow_sampling_rate_convert()
 */
static vtss_rc jr_sflow_sampling_rate_convert(const vtss_state_t *const state, const BOOL power2, const u32 rate_in, u32 *const rate_out)
{
    u32 modified_rate_in;
    // Could happen that two threads call this function simultaneously at boot, but we take the risk.
    // Once sflow_max_power_of_two has been computed, it's no longer a problem with simultaneous access.
    /*lint -esym(459, sflow_max_power_of_two) */
    static u32 sflow_max_power_of_two;

    if (sflow_max_power_of_two == 0) {
        sflow_max_power_of_two = next_power_of_two(JR_SFLOW_MAX_SAMPLE_RATE);
        if ((JR_SFLOW_MAX_SAMPLE_RATE & sflow_max_power_of_two) == 0) {
            sflow_max_power_of_two >>= 1;
        }
    }

    // Compute the actual sampling rate given the user input.
    if (rate_in != 0 && power2) {
        // Round off to the nearest power of two.
        u32 temp1 = next_power_of_two(rate_in);
        u32 temp2 = temp1 >> 1;
        if (temp1 - rate_in < rate_in-temp2) {
            modified_rate_in = temp1;
        } else {
            modified_rate_in = temp2;
        }
        if (modified_rate_in == 0) {
            modified_rate_in = 1;
        } else if (modified_rate_in > sflow_max_power_of_two) {
            modified_rate_in = sflow_max_power_of_two;
        }
    } else {
        modified_rate_in = rate_in;
    }

    (void)jr_sflow_hw_rate(modified_rate_in, rate_out);
    return VTSS_RC_OK;
}

/**
 * jr_sflow_port_conf_set()
 */
static vtss_rc jr_sflow_port_conf_set(const vtss_port_no_t port_no, const vtss_sflow_port_conf_t *const new_conf)
{
#define JR_SFLOW_ENABLED(_conf_) ((_conf_)->sampling_rate > 0 && (_conf_)->type != VTSS_SFLOW_TYPE_NONE)
    vtss_sflow_port_conf_t *cur_conf = &vtss_state->sflow_conf[port_no];
    u32                    hw_rate, value, chip_port = VTSS_CHIP_PORT(port_no);
    BOOL                   globally_enable;

    VTSS_SELECT_CHIP_PORT_NO(port_no);

    globally_enable = vtss_state->sflow_ena_cnt[vtss_state->chip_no] > 0;

    if (JR_SFLOW_ENABLED(new_conf) && !JR_SFLOW_ENABLED(cur_conf)) {
        if (vtss_state->sflow_ena_cnt[vtss_state->chip_no] == VTSS_PORTS) {
            VTSS_E("sFlow enable counter out of sync");
        }
        if (vtss_state->sflow_ena_cnt[vtss_state->chip_no]++ == 0) {
            globally_enable = TRUE;
        }
    } else if (!JR_SFLOW_ENABLED(new_conf) && JR_SFLOW_ENABLED(cur_conf)) {
        if (vtss_state->sflow_ena_cnt[vtss_state->chip_no] == 0) {
            VTSS_E("sFlow enable counter out of sync");
        } else  if (--vtss_state->sflow_ena_cnt[vtss_state->chip_no] == 0) {
            globally_enable = FALSE;
        }
    }

    *cur_conf = *new_conf;

    JR_WRB(ANA_AC, PS_COMMON_PS_COMMON_CFG, SFLOW_ENA,                  globally_enable);
    JR_WRB(ANA_AC, PS_COMMON_PS_COMMON_CFG, SFLOW_SMPL_ID_IN_STAMP_ENA, globally_enable);

    // Compute the actual sampling rate given the user input.
    // If the user requires power-of-two sampling rates, he is supposed to have provided
    // such a sampling rate in #new_conf.
    // We must ensure that power-of-two input sampling rates gives the same power-of-two
    // output sampling rate.
    hw_rate = jr_sflow_hw_rate(new_conf->sampling_rate, &cur_conf->sampling_rate);

    JR_RDX(ANA_AC, PS_COMMON_SFLOW_CTRL, chip_port, &value);
    value &= ~VTSS_M_ANA_AC_PS_COMMON_SFLOW_CTRL_SFLOW_DIR_SEL;
    value &= ~VTSS_M_ANA_AC_PS_COMMON_SFLOW_CTRL_SFLOW_SAMPLE_RATE;
    value |= JR_PUT_FLD(ANA_AC, PS_COMMON_SFLOW_CTRL, SFLOW_DIR_SEL, new_conf->type == VTSS_SFLOW_TYPE_NONE ? 0 :
                                                                     new_conf->type == VTSS_SFLOW_TYPE_RX   ? 1 :
                                                                     new_conf->type == VTSS_SFLOW_TYPE_TX   ? 2 : 3);
    value |= JR_PUT_FLD(ANA_AC, PS_COMMON_SFLOW_CTRL, SFLOW_SAMPLE_RATE, hw_rate);

    JR_WRX(ANA_AC, PS_COMMON_SFLOW_CTRL, chip_port, value);

    return VTSS_RC_OK;
#undef JR_SFLOW_ENABLED
}
#endif /* VTSS_FEATURE_SFLOW */

/* ================================================================= *
 *  Debug print
 * ================================================================= */

static void jr_debug_reg_header(const vtss_debug_printf_t pr, const char *name) 
{
    char buf[64];
    
    sprintf(buf, "%-18s  Tgt  Addr", name);
    vtss_debug_print_reg_header(pr, buf);
}

static void jr_debug_print_mask(const vtss_debug_printf_t pr, u32 mask)
{
    u32 port;
    
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        pr("%s%s", port == 0 || (port & 7) ? "" : ".", ((1<<port) & mask) ? "1" : "0");
    }
    pr("  0x%08x\n", mask);
}


static void jr_debug_print_header(const vtss_debug_printf_t pr, const char *txt)
{
    pr("%s:\n\n", txt);
}

static void jr_debug_print_port_header(const vtss_debug_printf_t pr, const char *txt)
{
    vtss_debug_print_port_header(pr, txt, VTSS_CHIP_PORTS, 1);
}

static void jr_debug_reg_clr(const vtss_debug_printf_t pr, u32 addr, const char *name, BOOL clr)
{
    u32  value, tgt;
    char buf[64];

    if (jr_rd(addr, &value) == VTSS_RC_OK && (clr == 0 || jr_wr(addr, value) == VTSS_RC_OK)) {
        tgt = ((addr >> 14) & 0xff);
        if (tgt < 128) {
            /* 14 bit address */
            addr &= 0x3fff;
        } else {
            /* 18 bit address */
            tgt &= 0xf0;
            addr &= 0x3ffff;
        }
        sprintf(buf, "%-18s  0x%02x 0x%05x", name, tgt, addr);
        vtss_debug_print_reg(pr, buf, value);
    }
}

static void jr_debug_sticky(const vtss_debug_printf_t pr, u32 addr, const char *name)
{
    jr_debug_reg_clr(pr, addr, name, 1);
}

static void jr_debug_sticky_inst(const vtss_debug_printf_t pr, u32 addr, u32 i, 
                                 const char *name)
{
    char buf[64];

    sprintf(buf, "%s_%u", name, i);
    jr_debug_sticky(pr, addr, buf);
}

static void jr_debug_reg(const vtss_debug_printf_t pr, u32 addr, const char *name)
{
    jr_debug_reg_clr(pr, addr, name, 0);
}

static void jr_debug_reg_inst(const vtss_debug_printf_t pr, u32 addr, u32 i, const char *name)
{
    char buf[64];

    sprintf(buf, "%s_%u", name, i);
    jr_debug_reg(pr, addr, buf);
}

#define JR_DEBUG_REG(pr, tgt, addr)            jr_debug_reg(pr, JR_ADDR(tgt, addr), #addr)
#define JR_DEBUG_REG_NAME(pr, tgt, addr, name) jr_debug_reg(pr, JR_ADDR(tgt, addr), name)
#define JR_DEBUG_REGX(pr, tgt, addr, x) \
    jr_debug_reg_inst(pr, JR_ADDR_X(tgt, addr, x), x, #addr)
#define JR_DEBUG_REGX_NAME(pr, tgt, addr, x, name) \
    jr_debug_reg_inst(pr, JR_ADDR_X(tgt, addr, x), x, name)
#define JR_DEBUG_REGXY_NAME(pr, tgt, addr, x, y, name) \
    jr_debug_reg_inst(pr, JR_ADDR_XY(tgt, addr, x, y), x, name)
    

static void jr_debug_cnt(const vtss_debug_printf_t pr, const char *col1, const char *col2, 
                          vtss_chip_counter_t *c1, vtss_chip_counter_t *c2)
{
    char buf[80];
    
    if (col1 != NULL) {
        sprintf(buf, "rx_%s:", col1);
        pr("%-19s%19llu   ", buf, c1->prev);
    }
    if (col2 != NULL) {
        sprintf(buf, "tx_%s:", strlen(col2) ? col2 : col1);
        pr("%-19s%19llu", buf, c2->prev);
    }
    pr("\n");
}

#define JR_DEBUG_GPIO(pr, addr, name) JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, GPIO_GPIO_##addr, "GPIO_"name)
#define JR_DEBUG_SIO(pr, addr, name) JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, SIO_CTRL_SIO_##addr, "SIO_"name)
#define JR_DEBUG_SIO_INST(pr, addr, i, name) jr_debug_reg_inst(pr, VTSS_DEVCPU_GCB_SIO_CTRL_SIO_##addr, i, "SIO_"name)

static void jr_debug_tgt(const vtss_debug_printf_t pr, const char *name, u32 to)
{
    u32 tgt = ((to >> 16) & 0xff);
    pr("%-12s  0x%02x (%u)\n", name, tgt, tgt);
}

#define JR_DEBUG_TGT(pr, name) jr_debug_tgt(pr, #name, VTSS_TO_##name)

static vtss_rc jr_debug_init(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    jr_debug_reg_header(pr, "ICPU/PI");
#if VTSS_OPT_VCORE_III
    if (vtss_state->chip_no == 0) {
        JR_DEBUG_REG_NAME(pr, ICPU_CFG, CPU_SYSTEM_CTRL_GENERAL_CTRL, "GENERAL_CTRL");
        JR_DEBUG_REG_NAME(pr, ICPU_CFG, PI_MST_PI_MST_CFG, "PI_MST_CFG");
        JR_DEBUG_REGX_NAME(pr, ICPU_CFG, PI_MST_PI_MST_CTRL, 0, "PI_MST_CTRL");
    }
#endif /* VTSS_OPT_VCORE_III */
    JR_DEBUG_REG_NAME(pr, DEVCPU_PI, PI_PI_MODE, "PI_MODE");
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_misc(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    u32  port, i, g;
    char name[32];
    
    pr("Name          Target\n");
    JR_DEBUG_TGT(pr, DEVCPU_ORG);
    JR_DEBUG_TGT(pr, DEVCPU_GCB);
    JR_DEBUG_TGT(pr, DEVCPU_QS);
    JR_DEBUG_TGT(pr, DEVCPU_PI);
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        sprintf(name, "DEV_%s_%u",
                port == VTSS_PORT_NPI ? "RGMII" : 
                VTSS_PORT_IS_10G(port) ? "10G" : port < 18 ? "1G" : "2G5",
                port == VTSS_PORT_NPI ? 0 : VTSS_PORT_IS_10G(port) ? (port - 27) :
                port < 18 ? port : (port - 18));
        jr_debug_tgt(pr, name, VTSS_TO_DEV(port));
    }
    JR_DEBUG_TGT(pr, ASM);
    JR_DEBUG_TGT(pr, ANA_CL);
    JR_DEBUG_TGT(pr, LRN);
    JR_DEBUG_TGT(pr, VCAP_IS0);
    JR_DEBUG_TGT(pr, VCAP_IS1);
    JR_DEBUG_TGT(pr, VCAP_IS2);
    JR_DEBUG_TGT(pr, ARB);
    JR_DEBUG_TGT(pr, SCH);
    JR_DEBUG_TGT(pr, DSM);
    JR_DEBUG_TGT(pr, HSIO);
    JR_DEBUG_TGT(pr, VCAP_ES0);
    JR_DEBUG_TGT(pr, ANA_L3);
    JR_DEBUG_TGT(pr, ANA_L2);
    JR_DEBUG_TGT(pr, ANA_AC);
    JR_DEBUG_TGT(pr, IQS);
    JR_DEBUG_TGT(pr, OQS);
    JR_DEBUG_TGT(pr, REW);
    pr("\n");

    jr_debug_reg_header(pr, "GPIOs");
    JR_DEBUG_GPIO(pr, OUT, "OUT");
    JR_DEBUG_GPIO(pr, IN, "IN");
    JR_DEBUG_GPIO(pr, OE, "OE");
    JR_DEBUG_GPIO(pr, INTR, "INTR");
    JR_DEBUG_GPIO(pr, INTR_ENA, "INTR_ENA");
    JR_DEBUG_GPIO(pr, INTR_IDENT, "INTR_IDENT");
    JR_DEBUG_GPIO(pr, ALT(0), "ALT_0");
    JR_DEBUG_GPIO(pr, ALT(1), "ALT_1");
    pr("\n");

    for (g=0; g<2; g++) {
        sprintf(name, "SGPIOs Group:%u", g);
        jr_debug_reg_header(pr, name);
        for (i = 0; i < 4; i++)
            JR_DEBUG_SIO_INST(pr, INPUT_DATA(g,i), i, "INPUT_DATA");
        for (i = 0; i < 4; i++)
            JR_DEBUG_SIO_INST(pr, INT_POL(g,i), i, "INT_POL");
        JR_DEBUG_SIO(pr, PORT_INT_ENA(g), "PORT_INT_ENA");
        for (i = 0; i < 32; i++)
            JR_DEBUG_SIO_INST(pr, PORT_CONFIG(g,i), i, "PORT_CONFIG");
        JR_DEBUG_SIO(pr, PORT_ENABLE(g), "PORT_ENABLE");
        JR_DEBUG_SIO(pr, CONFIG(g), "CONFIG");
        JR_DEBUG_SIO(pr, CLOCK(g), "CLOCK");
        for (i = 0; i < 4; i++)
            JR_DEBUG_SIO_INST(pr, INT_REG(g,i), i, "INT_REG");
        pr("\n");
    }
    
    return VTSS_RC_OK;
}

static void jr_debug_fld_nl(const vtss_debug_printf_t pr, const char *name, u32 value)
{
    pr("%-20s: %u\n", name, value);
}

#define JR_DEBUG_HSIO(pr, addr, name) JR_DEBUG_REG_NAME(pr, MACRO_CTRL, addr, name)
#define JR_DEBUG_MAC(pr, addr, i, name) jr_debug_reg_inst(pr, VTSS_DEV1G_MAC_CFG_STATUS_MAC_##addr, i, "MAC_"name)
#define JR_DEBUG_PCS(pr, addr, i, name) jr_debug_reg_inst(pr, VTSS_DEV1G_PCS1G_CFG_STATUS_PCS1G_##addr, i, "PCS_"name)
#define JR_DEBUG_10G_MAC(pr, addr, i, name) jr_debug_reg_inst(pr, VTSS_DEV10G_MAC_CFG_STATUS_MAC_##addr, i, "MAC_"name)
#define JR_DEBUG_10G_PCS(pr, addr, i, name) jr_debug_reg_inst(pr, VTSS_DEV10G_PCS_XAUI_##addr, i, "PCS_"name)
#define JR_DEBUG_HSIO_BIT(pr, addr, fld, value) jr_debug_fld_nl(pr, #fld, JR_GET_BIT(MACRO_CTRL, addr, fld, x))
#define JR_DEBUG_HSIO_FLD(pr, addr, fld, value) jr_debug_fld_nl(pr, #fld, JR_GET_FLD(MACRO_CTRL, addr, fld, x))
#define JR_DEBUG_RAW(pr, offset, length, value, name) jr_debug_fld_nl(pr, name, VTSS_EXTRACT_BITFIELD(value, offset, length))


static BOOL jr_debug_port_skip(const vtss_port_no_t port_no,
                               const vtss_debug_info_t   *const info)
{
    /* Skip disabled ports and ports not on the current device */
    return (info->port_list[port_no] && VTSS_PORT_CHIP_SELECTED(port_no) ? 0 : 1);
}

static vtss_rc jr_debug_chip_port(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info,
                                  u32 port)
{
    u32      i, tgt, lane = 0, count = 0, x;
    serdes_t type = SERDES_1G;
    char     buf[32];
    
    tgt = VTSS_TO_DEV(port);
    if (VTSS_PORT_IS_10G(port)) {
        JR_DEBUG_10G_MAC(pr, ENA_CFG(tgt), port, "ENA_CFG");
        JR_DEBUG_10G_MAC(pr, MODE_CFG(tgt), port, "MODE_CFG");
        JR_DEBUG_10G_MAC(pr, MAXLEN_CFG(tgt), port, "MAXLEN_CFG");
        JR_DEBUG_10G_MAC(pr, TAGS_CFG(tgt), port, "TAGS_CFG");
        jr_debug_reg_inst(pr, VTSS_DEV10G_DEV_CFG_STATUS_DEV_RST_CTRL(tgt), port, "DEV_RST_CTRL");
        JR_DEBUG_10G_PCS(pr, CONFIGURATION_PCS_XAUI_CFG(tgt), port, "XAUI_CFG");
        JR_DEBUG_10G_PCS(pr, STATUS_PCS_XAUI_RX_STATUS(tgt), port,  "XAUI_STATUS");
        JR_DEBUG_10G_PCS(pr, STATUS_PCS_XAUI_RX_ERROR_STATUS(tgt), port, "XAUI_ERROR");
        jr_debug_sticky_inst(pr, VTSS_IOREG(tgt, 0x1b), port, "TX_MON_STICKY");
        JR_DEBUG_10G_PCS(pr, CONFIGURATION_PCS_XAUI_EXT_CFG(tgt), port, "XAUI_EXT_CFG");
        JR_DEBUG_10G_PCS(pr, CONFIGURATION_PCS_XAUI_SD_CFG(tgt), port, "XAUI_SD_CFG");
    } else {        
        JR_DEBUG_MAC(pr, ENA_CFG(tgt), port, "ENA_CFG");
        JR_DEBUG_MAC(pr, MODE_CFG(tgt), port, "MODE_CFG");
        JR_DEBUG_MAC(pr, MAXLEN_CFG(tgt), port, "MAXLEN_CFG");
        JR_DEBUG_MAC(pr, TAGS_CFG(tgt), port, "TAGS_CFG");
        JR_DEBUG_PCS(pr, CFG(tgt), port, "CFG");
        JR_DEBUG_PCS(pr, MODE_CFG(tgt), port, "MODE_CFG");
        JR_DEBUG_PCS(pr, SD_CFG(tgt), port, "SD_CFG");
        JR_DEBUG_PCS(pr, ANEG_CFG(tgt), port, "ANEG_CFG");
        JR_DEBUG_PCS(pr, ANEG_STATUS(tgt), port, "ANEG_STATUS");
        JR_DEBUG_PCS(pr, LINK_STATUS(tgt), port, "LINK_STATUS");
        jr_debug_reg_inst(pr, VTSS_DEV1G_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt), port, 
                          "PCS_FX100_CFG");
        jr_debug_reg_inst(pr, VTSS_DEV1G_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), port, 
                          "FX100_STATUS");
    }
    JR_DEBUG_REGX_NAME(pr, DSM, CFG_RX_PAUSE_CFG, port, "RX_PAUSE_CFG");
    JR_DEBUG_REGX_NAME(pr, DSM, CFG_ETH_FC_GEN, port, "ETH_FC_GEN");
    pr("\n");
    
    if (info->full && jr_port2serdes_get(port, &type, &lane, &count) == VTSS_RC_OK) {
        for (i = lane; i < (lane + count); i++) {
            if (type == SERDES_1G) {
                sprintf(buf, "SerDes1G_%u", i);
                vtss_debug_print_reg_header(pr, buf);
                VTSS_RC(jr_sd1g_read(1 << i));
                JR_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_DES_CFG, "DES_CFG");
                JR_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_IB_CFG, "IB_CFG");
                JR_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_OB_CFG, "OB_CFG");
                JR_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_SER_CFG, "SER_CFG");
                JR_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, "COMMON_CFG");
                JR_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_PLL_CFG, "PLL_CFG");
                JR_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_MISC_CFG, "100FX_MISC");
                JR_DEBUG_HSIO(pr, SERDES1G_ANA_STATUS_SERDES1G_PLL_STATUS, "PLL_STATUS");
                JR_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG0, "DFT_CFG0");
                JR_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG1, "DFT_CFG1");
                JR_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG2, "DFT_CFG2");
                JR_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_TP_CFG, "TP_CFG");
                JR_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_MISC_CFG, "MISC_CFG");
                JR_DEBUG_HSIO(pr, SERDES1G_DIG_STATUS_SERDES1G_DFT_STATUS, "DFT_STATUS");
            } else {
                sprintf(buf, "SerDes6G_%u", i);
                vtss_debug_print_reg_header(pr, buf);
                VTSS_RC(jr_sd6g_read(1 << i));
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, "DES_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, "IB_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, "IB_CFG1");
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, "OB_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, "OB_CFG1");
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, "SER_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, "COMMON_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, "PLL_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_ANA_STATUS_SERDES6G_PLL_STATUS, "PLL_STATUS");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DIG_CFG, "DIG_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG0, "DFT_CFG0");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG1, "DFT_CFG1");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG2, "DFT_CFG2");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_TP_CFG0, "TP_CFG0");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_TP_CFG1, "TP_CFG1");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, "MISC_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_OB_ANEG_CFG, "OB_ANEG_CFG");
                JR_DEBUG_HSIO(pr, SERDES6G_DIG_STATUS_SERDES6G_DFT_STATUS, "DFT_STATUS");
                
                pr("\n%s:OB_CFG:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, &x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_IDLE, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_ENA1V_MODE, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_POL, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_POST0, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_POST1, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_PREC, x);
                JR_DEBUG_RAW(pr, 12, 1, x, "OB_R_ADJ_MUX");
                JR_DEBUG_RAW(pr, 11, 1, x, "OB_R_ADJ_PDR");
                JR_DEBUG_RAW(pr, 10, 1, x, "OB_R_COR");
                JR_DEBUG_RAW(pr, 9, 1, x, "OB_SEL_RCTRL");
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_SR_H, x);
                JR_DEBUG_RAW(pr, 4, 4, x, "OB_RESISTOR_CTRL");
                JR_DEBUG_RAW(pr, 0, 4, x, "OB_SR");
                pr("\n%s:OB_CFG1:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, &x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, OB_ENA_CAS, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, OB_LEV, x);

                pr("\n%s:DES_CFG:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, &x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, DES_PHS_CTRL, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, DES_MBTR_CTRL, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, DES_CPMD_SEL, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, DES_BW_HYST, x);
                JR_DEBUG_RAW(pr, 4, 1, x, "DES_SWAP_HYST");
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, DES_BW_ANA, x);
                JR_DEBUG_RAW(pr, 0, 1, x, "DES_SWAP_ANA");

                pr("\n%s:IB_CFG:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, &x);
                JR_DEBUG_RAW(pr, 28, 3, x, "ACJTAG_HYST");
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_IC_AC, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_IC_COM, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_IC_DC, x);
                JR_DEBUG_RAW(pr, 18, 1, x, "IB_R_COR");
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_RF, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_RT, x);
                JR_DEBUG_RAW(pr, 7, 3, x, "IB_VBAC");
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_VBCOM, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_RESISTOR_CTRL, x);

                pr("\n%s:IB_CFG1:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, &x);
                JR_DEBUG_RAW(pr, 12, 2, x, "IB_C_OFF");
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, IB_C, x);
                JR_DEBUG_RAW(pr, 7, 1, x, "IB_CHF");
                JR_DEBUG_RAW(pr, 6, 1, x, "IB_ANEG_MODE");
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, IB_CTERM_ENA, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, IB_DIS_EQ, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, IB_ENA_OFFSAC, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, IB_ENA_OFFSDC, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, IB_FX100_ENA, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, IB_RST, x);
                
                pr("\n%s:SER_CFG:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, &x);
                JR_DEBUG_RAW(pr, 8, 1, x, "SER_4TAP_ENA");
                JR_DEBUG_RAW(pr, 7, 1, x, "SER_CPMD_SEL");
                JR_DEBUG_RAW(pr, 6, 1, x, "SER_SWAP_CPMD");
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, SER_ALISEL, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, SER_ENHYS, x);
                JR_DEBUG_RAW(pr, 2, 1, x, "SER_BIG_WIN");
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, SER_EN_WIN, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, SER_ENALI, x);
                
                pr("\n%s:PLL_CFG:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, &x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_DIV4, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_ENA_ROT, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_FSM_CTRL_DATA, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_FSM_ENA, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_ROT_DIR, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_ROT_FRQ, x);

                pr("\n%s:COMMOM_CFG:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, &x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, SYS_RST, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, ENA_LANE, x);
                JR_DEBUG_RAW(pr, 17, 1, x, "PWD_RX");
                JR_DEBUG_RAW(pr, 16, 1, x, "PWD_TX");
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, HRATE, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, QRATE, x);
                JR_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, IF_MODE, x);
                
                pr("\nVAUI_CHANNEL_CFG_%u:\n", lane/count);
                JR_RDX(MACRO_CTRL, VAUI_CHANNEL_CFG_VAUI_CHANNEL_CFG, lane/count, &x);
                JR_DEBUG_HSIO_FLD(pr, VAUI_CHANNEL_CFG_VAUI_CHANNEL_CFG, LANE_SYNC_ENA, x);

                pr("\n%s:MISC_CFG:\n", buf);
                JR_RD(MACRO_CTRL, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, &x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, LANE_RST, x);
                JR_DEBUG_HSIO_BIT(pr, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, DES_100FX_CPMD_ENA, x);
            }
            pr("\n");
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_port(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    vtss_port_no_t port_no;
    u32            value, port;
    char           buf[32];
    
    pr("Target  : 0x%04x/0x%04x\n", vtss_state->create.target, vtss_state->chip_id.part_number);
    JR_RD(DEVCPU_GCB, VAUI2_MUX_VAUI2_MUX, &value);
    pr("Mux Mode: %u/%u\n\n", vtss_state->mux_mode[vtss_state->chip_no], value);

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        sprintf(buf, "Port %u (%u)", port, port_no);
        jr_debug_reg_header(pr, buf);
        VTSS_RC(jr_debug_chip_port(pr, info, port));
    } /* Port loop */

#ifdef VTSS_FEATURE_EEE
    if (vtss_state->eee_ena_cnt) {
        sprintf(buf, "EEE Loopback port %u", vtss_state->eee_loopback_chip_port[vtss_state->chip_no]);
        jr_debug_reg_header(pr, buf);
        VTSS_RC(jr_debug_chip_port(pr, info, vtss_state->eee_loopback_chip_port[vtss_state->chip_no]));
    }
#endif

    if (vtss_state->chip_count == 2) {
        /* Internal ports */
        for (port = vtss_state->port_int_0; port <= vtss_state->port_int_1; port++) {
            sprintf(buf, "Internal port %u", port);
            jr_debug_reg_header(pr, buf);
            VTSS_RC(jr_debug_chip_port(pr, info, port));
        }
    }
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_port_counters(const vtss_debug_printf_t pr, 
                                   const vtss_debug_info_t   *const info, u32 port)
{
    u32                      i, prio, qu, val1;
    char                     buf[32];
    vtss_port_jr1_counters_t cnt;

    if (jr_port_counters_chip(port, &cnt, NULL, 0) != VTSS_RC_OK)
        return VTSS_RC_ERROR;

    jr_debug_cnt(pr, "ok_bytes", "out_bytes", &cnt.rx_ok_bytes, &cnt.tx_out_bytes);
    jr_debug_cnt(pr, "uc", "", &cnt.rx_unicast, &cnt.tx_unicast);
    jr_debug_cnt(pr, "mc", "", &cnt.rx_multicast, &cnt.tx_multicast);
    jr_debug_cnt(pr, "bc", "", &cnt.rx_broadcast, &cnt.tx_broadcast);

    if (!info->full) {
        pr("\n");
        return VTSS_RC_OK;
    }
    
    jr_debug_cnt(pr, "pause", "", &cnt.rx_pause, &cnt.tx_pause);
    jr_debug_cnt(pr, "64", "", &cnt.rx_size64, &cnt.tx_size64);
    jr_debug_cnt(pr, "65_127", "", &cnt.rx_size65_127, &cnt.tx_size65_127);
    jr_debug_cnt(pr, "128_255", "", &cnt.rx_size128_255, &cnt.tx_size128_255);
    jr_debug_cnt(pr, "256_511", "", &cnt.rx_size256_511, &cnt.tx_size256_511);
    jr_debug_cnt(pr, "512_1023", "", &cnt.rx_size512_1023, &cnt.tx_size512_1023);
    jr_debug_cnt(pr, "1024_1526", "", &cnt.rx_size1024_1518, &cnt.tx_size1024_1518);
    jr_debug_cnt(pr, "jumbo", "", &cnt.rx_size1519_max, &cnt.tx_size1519_max);
    jr_debug_cnt(pr, "crc", NULL, &cnt.rx_crc_err, NULL);
    jr_debug_cnt(pr, "local_drops", NULL, &cnt.rx_local_drops, NULL);
    
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "drops_%u", i);
        jr_debug_cnt(pr, buf, "", &cnt.rx_queue_drops[i], &cnt.tx_queue_drops[i]);
    }
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "tx_drops_%u", i);
        jr_debug_cnt(pr, buf, NULL, &cnt.rx_txqueue_drops[i], NULL);
    }
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "pol_drops_%u", i);
        jr_debug_cnt(pr, buf, NULL, &cnt.rx_policer_drops[i], NULL);
    }
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "green_%u", i);
        jr_debug_cnt(pr, buf, "", &cnt.rx_green_class[i], &cnt.tx_green_class[i]);
    }
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "yellow_%u", i);
        jr_debug_cnt(pr, buf, "", &cnt.rx_yellow_class[i], &cnt.tx_yellow_class[i]);
    }
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "red_%u", i);
        jr_debug_cnt(pr, buf, NULL, &cnt.rx_red_class[i], NULL);
    }
    pr("\n");
    
    pr("IQS/OQS Stickies (drops are explained):\n");
    for (prio = 0; prio < 8; prio++) {
        qu = port * 8 + prio;  
        JR_RDX(IQS, STAT_CNT_CFG_RX_STAT_EVENTS_STICKY, qu, &val1);
        if (val1 > 0) {
            pr("Port:%u Queue:%u. IQS RX Sticky:0x%x",port,prio,val1);
            if (val1>>0 & 1) 
                pr(" ->Failed frame");
            if (val1>>1 & 1) 
                pr(" ->due to buffer resource ");
            if (val1>>2 & 1) 
                pr(" ->due to prio resource ");
            if (val1>>3 & 1) 
                pr(" ->due to port resource ");
            if (val1>>4 & 1) 
                pr(" ->due to queue resource ");
            if (val1>>5 & 1) 
                pr(" ->due to RED ");
            if (val1>>6 & 1) 
                pr(" ->due to Q disabled ");
            if (val1>>7 & 1) 
                pr(" ->frame is to long ");
            pr("\n");
        }

        JR_RDX(IQS, STAT_CNT_CFG_TX_STAT_EVENTS_STICKY, qu, &val1);
        if (val1 > 0) {
            pr("Port:%u Queue:%u. IQS TX Sticky:0x%x",port,prio,val1);
            if (val1>>0 & 1) 
                pr(" ->Failed frame");
            if (val1>>1 & 1) 
                pr(" ->due to aging ");
            if (val1>>2 & 1) 
                pr(" ->due to Q flush ");
            if (val1>>3 & 1) 
                pr(" ->due to full OQS ");
            pr("\n");
        }

        JR_RDX(OQS, STAT_CNT_CFG_RX_STAT_EVENTS_STICKY, qu, &val1);
        if (val1 > 0) {
            pr("Port:%u Queue:%u. OQS RX Sticky:0x%x",port,prio,val1);
            if (val1>>0 & 1) 
                pr(" ->Failed frame");
            if (val1>>1 & 1) 
                pr(" ->due to buffer resource ");
            if (val1>>2 & 1) 
                pr(" ->due to prio resource ");
            if (val1>>3 & 1) 
                pr(" ->due to port resource ");
            if (val1>>4 & 1) 
                pr(" ->due to queue resource ");
            if (val1>>5 & 1) 
                pr(" ->due to RED ");
            if (val1>>6 & 1) 
                pr(" ->due to Q disabled ");
            if (val1>>7 & 1) 
                pr(" ->frame is to long ");
            pr("\n");
        }

        JR_RDX(OQS, STAT_CNT_CFG_TX_STAT_EVENTS_STICKY, qu, &val1);
        if (val1 > 0) {
            pr("Port:%u Queue:%u. OQS TX Sticky:0x%x",port,prio,val1);
            if (val1>>0 & 1) 
                pr(" ->Failed frame");
            if (val1>>1 & 1) 
                pr(" ->due to aging ");
            if (val1>>2 & 1) 
                pr(" ->due to Q flush ");
            if (val1>>3 & 1) 
                pr(" ->due to full OQS ");
            pr("\n");
        }

        JR_WRX(IQS, STAT_CNT_CFG_RX_STAT_EVENTS_STICKY, qu, 0xFFFFFFFF);
        JR_WRX(IQS, STAT_CNT_CFG_TX_STAT_EVENTS_STICKY, qu, 0xFFFFFFFF);
        JR_WRX(OQS, STAT_CNT_CFG_RX_STAT_EVENTS_STICKY, qu, 0xFFFFFFFF);
        JR_WRX(OQS, STAT_CNT_CFG_TX_STAT_EVENTS_STICKY, qu, 0xFFFFFFFF);


        /* jr_debug_sticky_inst(pr, VTSS_IQS_STAT_CNT_CFG_RX_STAT_EVENTS_STICKY(qu),  */
        /*                      prio, "IQS_RX_STICKY");  */
        /* jr_debug_sticky_inst(pr, VTSS_IQS_STAT_CNT_CFG_TX_STAT_EVENTS_STICKY(qu),  */
        /*                      prio, "IQS_TX_STICKY");  */
        /* jr_debug_sticky_inst(pr, VTSS_OQS_STAT_CNT_CFG_RX_STAT_EVENTS_STICKY(qu),  */
        /*                      prio, "OQS_RX_STICKY");  */
        /* jr_debug_sticky_inst(pr, VTSS_OQS_STAT_CNT_CFG_TX_STAT_EVENTS_STICKY(qu),  */
        /*                      prio, "OQS_TX_STICKY");  */
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_port_cnt(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */ // Due to VTSS_EXIT_ENTER
    vtss_port_no_t port_no;
    u32            port;
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (jr_debug_port_skip(port_no, info))
            continue;
        VTSS_EXIT_ENTER();
        port = VTSS_CHIP_PORT(port_no);
        pr("Counters for port: %u (chip_port: %u):\n\n", port_no, port);
        (void)jr_debug_port_counters(pr, info, port);
    }

#ifdef VTSS_FEATURE_EEE
    if (vtss_state->eee_ena_cnt) {
        pr("Counters for EEE Loopback port %u:\n\n", vtss_state->eee_loopback_chip_port[vtss_state->chip_no]);
        (void)jr_debug_port_counters(pr, info, vtss_state->eee_loopback_chip_port[vtss_state->chip_no]);
    }
#endif

    if (vtss_state->chip_count == 2) {
        /* Internal port counters */
        for (port = vtss_state->port_int_0; port <= vtss_state->port_int_1; port++) {
            pr("Counters for internal port %u:\n\n", port);
            (void)jr_debug_port_counters(pr, info, port);
        }
    }
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_mac_table(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */ // Due to VTSS_EXIT_ENTER
    vtss_mac_entry_t       *entry;
    vtss_mac_table_entry_t mac_entry;
    BOOL                   header = 1;
    u32                    pgid;

    /* Dump MAC address table */
    memset(&mac_entry, 0, sizeof(mac_entry));
    while (jr_mac_table_get_next(&mac_entry, &pgid) == VTSS_RC_OK) {
        vtss_debug_print_mac_entry(pr, "Dynamic Entries (GET_NEXT)", &header, &mac_entry, pgid);
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");

    /* Dump static entries not returned by the get_next function */
    header = 1;
    for (entry = vtss_state->mac_list_used; entry != NULL; entry = entry->next) {
        vtss_mach_macl_set(&mac_entry.vid_mac, entry->mach, entry->macl);
        if (jr_mac_table_get_chip(&mac_entry, &pgid) == VTSS_RC_OK) {
            vtss_debug_print_mac_entry(pr, "Static Entries (GET)", &header, &mac_entry, pgid);
        }
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");
    
    /* Read and clear sticky bits */
    if (info->full) {
        jr_debug_reg_header(pr, "STICKY");
        jr_debug_sticky(pr, VTSS_ANA_CL_2_STICKY_FILTER_STICKY, "ANA_CL:FILTER_STKY");
        jr_debug_sticky(pr, VTSS_ANA_CL_2_STICKY_CLASS_STICKY, "ANA_CL:CLASS_STCKY");
        jr_debug_sticky(pr, VTSS_ANA_CL_2_STICKY_CAT_STICKY, "ANA_CL:CAT_STICKY");
        jr_debug_sticky(pr, VTSS_ANA_CL_2_STICKY_IP_HDR_CHK_STICKY, "ANA_CL:IP_HDR_CHK");
        jr_debug_sticky(pr, VTSS_ANA_CL_2_STICKY_VQ_STICKY_REG, "ANA_CL:VQ_STICKY");
        jr_debug_sticky(pr, VTSS_ANA_CL_2_STICKY_MISC_CONF_STICKY, "ANA_CL:MISC_CONF");
        jr_debug_sticky(pr, VTSS_ANA_L2_STICKY_CCM_STICKY, "ANA_L2:CCM_STICKY");
        jr_debug_sticky(pr, VTSS_ANA_L2_STICKY_STICKY, "ANA_L2:L2_STICKY");
        jr_debug_sticky(pr, VTSS_ANA_AC_PS_STICKY_STICKY, "ANA_AC:PS_STICKY");
        pr("\n");
    }
    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_ts(const vtss_debug_printf_t pr,
                            const vtss_debug_info_t   *const info)
{
    u32  value;
    u32  port_no;
    char buf[32];
    
    /* DEVCPU_GCB: PTP_CFG */
    jr_debug_reg_header(pr, "GCB:PTP_CFG");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_CFG_PTP_MISC_CFG, "PTP_MISC_CFG");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_CFG_PTP_UPPER_LIMIT_CFG, "PTP_UPPER_LIMIT_CFG");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG, "PTP_UPPER_LIMIT_1_TIME_ADJ_CFG");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_CFG_PTP_SYNC_INTR_ENA_CFG, "PTP_SYNC_INTR_ENA_CFG");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG, "GEN_EXT_CLK_HIGH_PERIOD_CFG");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG, "GEN_EXT_CLK_LOW_PERIOD_CFG");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_CFG_GEN_EXT_CLK_CFG, "GEN_EXT_CLK_CFG");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_CFG_CLK_ADJ_CFG, "CLK_ADJ_CFG");
    /* DEVCPU_GCB: PTP_STAT */
    jr_debug_reg_header(pr, "GCB:PTP_STAT");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_STAT_PTP_CURRENT_TIME_STAT, "PTP_CURRENT_TIME_STAT");
    JR_DEBUG_REG_NAME(pr, DEVCPU_GCB, PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT, "EXT_SYNC_CURRENT_TIME_STAT");
    
    JR_RD(DEVCPU_GCB, PTP_STAT_PTP_EVT_STAT, &value);
    JR_WR(DEVCPU_GCB, PTP_STAT_PTP_EVT_STAT, value);
    vtss_debug_print_sticky(pr, "CLK_ADJ_UPD_STICKY", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY);
    vtss_debug_print_sticky(pr, "EXT_SYNC_CURRENT_TIME_STICKY", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_EXT_SYNC_CURRENT_TIME_STICKY);
    vtss_debug_print_sticky(pr, "SYNC_STAT", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_SYNC_STAT);
    
    /* SYS_PORT:PTP */
    for (port_no = 0; port_no < vtss_state->port_count; port_no++) {
        u32 chip_port = VTSS_CHIP_PORT(port_no);
        if (jr_debug_port_skip(port_no, info))
            continue;
        
        sprintf(buf, "DEV1G:PTP, CHIP PORT %d", chip_port);
        jr_debug_reg_header(pr, buf);
        
        JR_DEBUG_REGX_NAME(pr, ASM, CFG_ETH_CFG, chip_port, "ASM::ETH_CFG");
        if (VTSS_PORT_IS_1G(chip_port)) {
            JR_DEBUG_REGX_NAME(pr, DEV1G, DEV1G_INTR_CFG_STATUS_DEV1G_INTR_CFG, VTSS_TO_DEV(chip_port), "DEV1G_INTR_CFG");
            JR_DEBUG_REGX_NAME(pr, DEV1G, DEV1G_INTR_CFG_STATUS_DEV1G_INTR, VTSS_TO_DEV(chip_port), "DEV1G_INTR");
            JR_DEBUG_REGX_NAME(pr, DEV1G, DEV_CFG_STATUS_DEV_PTP_CFG, VTSS_TO_DEV(chip_port), "DEV_PTP_CFG");
            JR_DEBUG_REGX_NAME(pr, DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_STICKY, VTSS_TO_DEV(chip_port), "DEV_PTP_TX_STICKY");
            JR_DEBUG_REGXY_NAME(pr, DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_TSTAMP, VTSS_TO_DEV(chip_port), 0, "DEV_PTP_TX_TSTAMP0");
            JR_DEBUG_REGXY_NAME(pr, DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_TSTAMP, VTSS_TO_DEV(chip_port), 1, "DEV_PTP_TX_TSTAMP1");
            JR_DEBUG_REGXY_NAME(pr, DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_TSTAMP, VTSS_TO_DEV(chip_port), 2, "DEV_PTP_TX_TSTAMP2");
            JR_DEBUG_REGXY_NAME(pr, DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_ID, VTSS_TO_DEV(chip_port), 0, "DEV_PTP_TX_ID0");
            JR_DEBUG_REGXY_NAME(pr, DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_ID, VTSS_TO_DEV(chip_port), 1, "DEV_PTP_TX_ID1");
            JR_DEBUG_REGXY_NAME(pr, DEV1G, DEV_CFG_STATUS_DEV_PTP_TX_ID, VTSS_TO_DEV(chip_port), 2, "DEV_PTP_TX_ID2");
            
        } else {
            JR_DEBUG_REGX_NAME(pr, DEV10G, DEV_CFG_STATUS_DEV_PTP_CFG, VTSS_TO_DEV(chip_port), "DEV_PTP_CFG");
            JR_DEBUG_REGX_NAME(pr, DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_STICKY, VTSS_TO_DEV(chip_port), "DEV_PTP_TX_STICKY");
            JR_DEBUG_REGXY_NAME(pr, DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_TSTAMP, VTSS_TO_DEV(chip_port), 0, "DEV_PTP_TX_TSTAMP0");
            JR_DEBUG_REGXY_NAME(pr, DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_TSTAMP, VTSS_TO_DEV(chip_port), 1, "DEV_PTP_TX_TSTAMP1");
            JR_DEBUG_REGXY_NAME(pr, DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_TSTAMP, VTSS_TO_DEV(chip_port), 2, "DEV_PTP_TX_TSTAMP2");
            JR_DEBUG_REGXY_NAME(pr, DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_ID, VTSS_TO_DEV(chip_port), 0, "DEV_PTP_TX_ID0");
            JR_DEBUG_REGXY_NAME(pr, DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_ID, VTSS_TO_DEV(chip_port), 1, "DEV_PTP_TX_ID1");
            JR_DEBUG_REGXY_NAME(pr, DEV10G, DEV_CFG_STATUS_DEV_PTP_TX_ID, VTSS_TO_DEV(chip_port), 2, "DEV_PTP_TX_ID2");
            JR_DEBUG_REGX_NAME(pr, DEV10G, MAC_CFG_STATUS_MAC_MODE_CFG, VTSS_TO_DEV(chip_port), "MAC_MODE_CFG");
        }
    }
    
    /* ANA:: */
    jr_debug_reg_header(pr, "ANA::PTP");
    JR_DEBUG_REG_NAME(pr, ANA_AC, PS_COMMON_MISC_CTRL, "MISC-CTRL");
    
    
    pr("\n");
    
    return VTSS_RC_OK;
}

static void jr_debug_bit(const vtss_debug_printf_t pr, const char *name, u32 value)
{
    int  i, length = strlen(name);

    for (i = 0; i < length; i++)
        pr("%c", value ? toupper(name[i]) : tolower(name[i]));
    pr(" ");
}

static void jr_debug_fld(const vtss_debug_printf_t pr, const char *name, u32 value)
{
    pr("%s: ", name);
    if (value < 10)
        pr("%u", value);
    else
        pr("0x%x", value);
    pr(" ");
}

static void jr_debug_action(const vtss_debug_printf_t pr, const char *name, u32 ena, u32 val)
{
    int i, length = strlen(name);

    for (i = 0; i < length; i++)
        pr("%c", ena ? toupper(name[i]) : tolower(name[i]));
    pr(":%u ", val);
}

static void jr_debug_vcap_reg(const vtss_debug_printf_t pr, 
                              const char *name, jr_vcap_reg_t *reg, u32 mask)
{
    u32 i, j, m;

    if (reg->valid) {
        if (name)
            pr("%s: ", name);

        if (mask == 0) /* Show all bits by default */
            mask = 0xffffffff;

        for (i = 0, j = 0; i < 32; i++) {
            m = VTSS_BIT(31 - i);
            if (m & mask) {
                if ((name == NULL || j != 0) && (j % 8) == 0)
                    pr(".");
                j++;
                pr("%c", (reg->mask & m) ? 'X' : (reg->value & m) ? '1' : '0') ;
            }
        }
        pr(j > 23 ? "\n" : " ");
    }
}

static void jr_debug_vcap_mac(const vtss_debug_printf_t pr, 
                              const char *name, jr_vcap_mac_t *reg)
{
    jr_debug_vcap_reg(pr, name, &reg->mach, 0xffff);
    jr_debug_vcap_reg(pr, NULL, &reg->macl, 0);
}

static vtss_rc jr_debug_vcap(int bank,
                             const char *name,
                             const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info,
                             vtss_rc (* dbg)(const vtss_debug_printf_t pr, BOOL action))
{
    /*lint --e{454, 455, 456} */ // Due to VTSS_EXIT_ENTER
    const tcam_props_t *tcam = &tcam_info[bank];
    int                i, is_entry;
    
    jr_debug_print_header(pr, name);
    
    for (i = (tcam->actions - 1); i >= 0; i--) {
        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();

        /* Read entry */
        if (jr_vcap_command(bank, i, VTSS_TCAM_CMD_READ) != VTSS_RC_OK)
            continue;

        /* Print entry */
        is_entry = (i < tcam->entries);
        if (is_entry) {
            if (dbg(pr, 0) != VTSS_RC_OK)
                continue;
            pr("\n");
        }
        
        /* Print action */
        pr("%d (", i);
        if (is_entry)
            pr("rule %d", tcam->entries - i - 1);
        else if (bank == VTSS_TCAM_IS1)
            pr("%s_default", i == (tcam->actions - 1) ? "qos" : "vlan_pag");
        else
            pr("port %d", i - tcam->entries);
        pr("): ");
        if (dbg(pr, 1) == VTSS_RC_OK)
            pr("\n-------------------------------------------\n");
    }
    pr("\n");
    return VTSS_RC_OK;
}
    
static vtss_rc jr_debug_is0(const vtss_debug_printf_t pr, BOOL action)
{
    u32           value, type;
    jr_vcap_reg_t reg, reg1;

    if (action) {
        JR_RD(VCAP_IS0, BASETYPE_ACTION_A, &value);
        jr_debug_bit(pr, "s1_dmac_ena", value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_S1_DMAC_ENA);
        jr_debug_bit(pr, "l3_mc_lookup_dis", 
                     value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_L3_MC_LOOKUP_DIS);
        jr_debug_bit(pr, "oam_detect_dis", 
                     value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_OAM_DETECT_DIS);
        jr_debug_bit(pr, "second_lookup_dis", 
                     value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_SECOND_LOOKUP_DIS);
        pr("\n");
        jr_debug_bit(pr, "mac_pop_cnt", 
                     value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_MAC_POP_CNT);
        jr_debug_fld(pr, "vlan_pop_cnt", 
                     VTSS_X_VCAP_IS0_BASETYPE_ACTION_A_VLAN_POP_CNT(value));
        jr_debug_bit(pr, "bvid_ena", 
                     value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_BVID_ENA);
        jr_debug_bit(pr, "vid_ena", 
                     value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_VID_ENA);
        jr_debug_bit(pr, "dei_val", 
                     value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_DEI_VAL);
        jr_debug_fld(pr, "pcp_val", 
                     VTSS_X_VCAP_IS0_BASETYPE_ACTION_A_PCP_VAL(value));
        jr_debug_bit(pr, "pcp_dei_ena", 
                     value & VTSS_F_VCAP_IS0_BASETYPE_ACTION_A_PCP_DEI_ENA);
        pr("\n");
        JR_RD(VCAP_IS0, BASETYPE_ACTION_B, &value);
        jr_debug_fld(pr, "isdx_val", 
                     VTSS_X_VCAP_IS0_BASETYPE_ACTION_B_ISDX_VAL(value));
        jr_debug_fld(pr, "vid_val", 
                     VTSS_X_VCAP_IS0_BASETYPE_ACTION_B_VID_VAL(value));
        jr_debug_fld(pr, "pag_val", 
                     VTSS_X_VCAP_IS0_BASETYPE_ACTION_B_PAG_VAL(value));
        JR_RD(VCAP_IS0, BASETYPE_ACTION_C, &value);
        jr_debug_bit(pr, "hit", value);
        
        return VTSS_RC_OK;
    }

    JR_RDF(VCAP_IS0, IS0_CONTROL_ACE_STATUS, ACE_ENTRY_TYPE, &type);

    switch (type) {
    case JR_IS0_TYPE_ISID:
        JR_RD(VCAP_IS0, ISID_ENTRY_A, &value);
        if ((value & VTSS_F_VCAP_IS0_ISID_ENTRY_A_VLD) == 0)
            return VTSS_RC_ERROR;
        pr("type: isid");
        break;
    case JR_IS0_TYPE_DBL_VID:
        JR_IS0_RD(DBL_VID, A, reg);
        if ((reg.value & VTSS_F_VCAP_IS0_DBL_VID_ENTRY_A_VLD) == 0)
            return VTSS_RC_ERROR;
        pr("type: dbl_vid ");
        JR_IS0_DEBUG_FLD(pr, "vigr_port", DBL_VID, A, VIGR_PORT, &reg);

        /* DBL_VID0 and DBL_VID1 */
        JR_IS0_RD(DBL_VID, DBL_VID0, reg);
        JR_IS0_RD(DBL_VID, DBL_VID1, reg1);
        JR_IS0_DEBUG_BIT(pr, "tagged", DBL_VID, DBL_VID0, VLAN_TAGGED, &reg);
        JR_IS0_DEBUG_BIT(pr, "dbl_tagged", DBL_VID, DBL_VID0, VLAN_DBL_TAGGED, &reg);
        pr("\n");
        JR_IS0_DEBUG_FLD(pr, "outer_vid", DBL_VID, DBL_VID1, OUTER_VID, &reg1);
        JR_IS0_DEBUG_FLD(pr, "outer_pcp", DBL_VID, DBL_VID0, OUTER_PCP, &reg);
        JR_IS0_DEBUG_BIT(pr, "outer_dei", DBL_VID, DBL_VID0, OUTER_DEI, &reg);
        JR_IS0_DEBUG_BIT(pr, "outer_tpi", DBL_VID, DBL_VID0, OUTER_TPI, &reg);
        pr("\n");
        JR_IS0_DEBUG_FLD(pr, "inner_vid", DBL_VID, DBL_VID1, INNER_VID, &reg1);
        JR_IS0_DEBUG_FLD(pr, "inner_pcp", DBL_VID, DBL_VID0, INNER_PCP, &reg);
        JR_IS0_DEBUG_BIT(pr, "inner_dei", DBL_VID, DBL_VID0, INNER_DEI, &reg);
        JR_IS0_DEBUG_BIT(pr, "inner_tpi", DBL_VID, DBL_VID0, INNER_TPI, &reg);
        pr("\n");
        
        /* DBL_VID2 */
        JR_IS0_RD(DBL_VID, DBL_VID2, reg);
        JR_IS0_DEBUG_FLD(pr, "l3_dscp", DBL_VID, DBL_VID2, L3_DSCP, &reg);
        JR_IS0_DEBUG_FLD(pr, "prot", DBL_VID, DBL_VID2, PROT, &reg);
        pr("\n");
        break;
    case JR_IS0_TYPE_MPLS:
        JR_RD(VCAP_IS0, MPLS_ENTRY_A, &value);
        if ((value & VTSS_F_VCAP_IS0_MPLS_ENTRY_A_VLD) == 0)
            return VTSS_RC_ERROR;
        pr("type: mpls");
        break;
    case JR_IS0_TYPE_MAC_ADDR:
        JR_RD(VCAP_IS0, MAC_ADDR_ENTRY_A, &value);
        if ((value & VTSS_F_VCAP_IS0_MAC_ADDR_ENTRY_A_VLD) == 0)
            return VTSS_RC_ERROR;
        pr("type: mac_addr");
        break;
    default:
        break;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_vcap_is0(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    return jr_debug_vcap(VTSS_TCAM_IS0, "IS0", pr, info, jr_debug_is0);
}

/* Read IS1 common registers */
#define JR_IS1_RD_COMMON(pre, regs)        \
{                                          \
    JR_IS1_RD(pre, ENTRY, regs.entry);     \
    JR_IS1_RD(pre, IF_GRP, regs.if_grp);   \
    JR_IS1_RD(pre, VLAN, regs.vlan);       \
    JR_IS1_RD(pre, FLAGS, regs.flags);     \
    JR_IS1_RD(pre, L3_IP4_SIP, regs.sip);  \
    JR_IS1_RD(pre, L3_MISC, regs.l3_misc); \
    JR_IS1_RD(pre, L4_MISC, regs.l4_misc); \
}

static vtss_rc jr_debug_is1(const vtss_debug_printf_t pr, BOOL action)
{
    u32                type, x;
    jr_vcap_is1_regs_t regs;

    JR_RDB(VCAP_IS1, IS1_CONTROL_ACE_STATUS, ACE_ACTION_TYPE, &type);
    if (action) {
        if (type) {
            /* QOS_ACTION */
            JR_RD(VCAP_IS1, QOS_ACTION_DSCP, &x);
            jr_debug_action(pr, "dscp", 
                            JR_GET_BIT(VCAP_IS1, QOS_ACTION_DSCP, DSCP_ENA, x), 
                            JR_GET_FLD(VCAP_IS1, QOS_ACTION_DSCP, DSCP_VAL, x));
            JR_RD(VCAP_IS1, QOS_ACTION_QOS, &x);
            jr_debug_action(pr, "qos",
                            JR_GET_BIT(VCAP_IS1, QOS_ACTION_QOS, QOS_ENA, x),
                            JR_GET_FLD(VCAP_IS1, QOS_ACTION_QOS, QOS_VAL, x));
            JR_RD(VCAP_IS1, QOS_ACTION_DP, &x);
            jr_debug_action(pr, "dp",
                            JR_GET_BIT(VCAP_IS1, QOS_ACTION_DP, DP_ENA, x),
                            JR_GET_FLD(VCAP_IS1, QOS_ACTION_DP, DP_VAL, x));
            JR_RD(VCAP_IS1, QOS_ACTION_STICKY, &x);
        } else {
            /* VLAN_PAG_ACTION */
            JR_RD(VCAP_IS1, VLAN_PAG_ACTION_PAG, &x);
            jr_debug_fld(pr, "pag_val", 
                         JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_PAG, PAG_VAL, x));
            jr_debug_fld(pr, "pag_mask", 
                         JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_PAG, PAG_OVERRIDE_MASK, x));
            JR_RD(VCAP_IS1, VLAN_PAG_ACTION_MISC, &x);
            jr_debug_action(pr, "custom_type", 
                            JR_GET_BIT(VCAP_IS1, VLAN_PAG_ACTION_MISC, CUSTOM_ACE_TYPE_ENA, x),
                            JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_MISC, CUSTOM_ACE_TYPE_VAL, x));
            pr("\n");
            jr_debug_action(pr, "pcp", 
                            JR_GET_BIT(VCAP_IS1, VLAN_PAG_ACTION_MISC, PCP_DEI_ENA, x),
                            JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_MISC, PCP_VAL,x));
            jr_debug_fld(pr, "dei", 
                         JR_GET_BIT(VCAP_IS1, VLAN_PAG_ACTION_MISC, DEI_VAL, x));
            jr_debug_action(pr, "vid", 
                            JR_GET_BIT(VCAP_IS1, VLAN_PAG_ACTION_MISC, VID_REPLACE_ENA, x),
                            JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_MISC, VID_ADD_VAL, x));
            JR_RD(VCAP_IS1, VLAN_PAG_ACTION_CUSTOM_POS, &x);
            jr_debug_fld(pr, "pos_0", 
                         JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_CUSTOM_POS, CUSTOM_POS_VAL_0, x));
            jr_debug_fld(pr, "pos_1", 
                         JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_CUSTOM_POS, CUSTOM_POS_VAL_1, x));
            jr_debug_fld(pr, "pos_2", 
                         JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_CUSTOM_POS, CUSTOM_POS_VAL_2, x));
            jr_debug_fld(pr, "pos_3", 
                         JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_CUSTOM_POS, CUSTOM_POS_VAL_3, x));
            JR_RD(VCAP_IS1, VLAN_PAG_ACTION_ISDX, &x);
            jr_debug_action(pr, "isdx", 
                            JR_GET_BIT(VCAP_IS1, VLAN_PAG_ACTION_ISDX, ISDX_REPLACE_ENA, x),
                            JR_GET_FLD(VCAP_IS1, VLAN_PAG_ACTION_ISDX, ISDX_ADD_VAL, x));
            JR_RD(VCAP_IS1, VLAN_PAG_ACTION_STICKY, &x);
        }
        jr_debug_bit(pr, "hit", x);
        return VTSS_RC_OK;
    }

    memset(&regs, 0, sizeof(regs));
    if (type) {
        /* QOS_ENTRY */
        JR_IS1_RD_COMMON(QOS, regs);
        JR_IS1_RD(QOS, L2_MAC_ADDR_HIGH, regs.mac.mach);
        JR_IS1_RD(QOS, L4_PORT, regs.l4_port);
    } else {
        /* VLAN_PAG_ENTRY */
        JR_IS1_RD_COMMON(VLAN_PAG, regs);
        JR_IS1_RD_MAC(VLAN_PAG, L2_MAC_ADDR, regs.mac);
    }
    
    /* The register layout is the same for different entry types */
    /* Check if entry is valid */
    if (JR_GET_BIT(VCAP_IS1, QOS_ENTRY_ENTRY, VLD, regs.entry.value) == 0)
        return VTSS_RC_ERROR;

    jr_debug_fld(pr, type ? "type_qos" : "type_vlan_pag", type);    
    
    /* ENTRY */
    JR_IS1_DEBUG_FLD(pr, "pag", QOS, ENTRY, PAG, &regs.entry);
    JR_IS1_DEBUG_BIT(pr, "vport_vld", QOS, ENTRY, VPORT_VLD, &regs.entry);
    JR_IS1_DEBUG_BIT(pr, "isdx_neq0", QOS, ENTRY, ISDX_NEQ0, &regs.entry);
    pr("\n");
    
    /* IF_GRP */
    jr_debug_vcap_reg(pr, "igr_port_mask", &regs.if_grp, 0);

    /* VLAN */
    JR_IS1_DEBUG_BIT(pr, "vlan_tagged", QOS, VLAN, VLAN_TAGGED, &regs.vlan);
    JR_IS1_DEBUG_BIT(pr, "dbl_tagged", QOS, VLAN, VLAN_DBL_TAGGED, &regs.vlan);
    JR_IS1_DEBUG_BIT(pr, "in_dei", QOS, VLAN, INNER_DEI, &regs.vlan);
    JR_IS1_DEBUG_BIT(pr, "ou_dei", QOS, VLAN, OUTER_DEI, &regs.vlan);
    JR_IS1_DEBUG_FLD(pr, "pcp", QOS, VLAN, PCP, &regs.vlan);
    JR_IS1_DEBUG_FLD(pr, "vid", QOS, VLAN, VID, &regs.vlan);
    pr("\n");

    /* FLAGS */
    JR_IS1_DEBUG_BIT(pr, "l2_bc", QOS, FLAGS, L2_BC, &regs.flags);
    JR_IS1_DEBUG_BIT(pr, "l2_mc", QOS, FLAGS, L2_MC, &regs.flags);
    JR_IS1_DEBUG_BIT(pr, "etype_len", QOS, FLAGS, ETYPE_LEN, &regs.flags);
    JR_IS1_DEBUG_BIT(pr, "ip", QOS, FLAGS, IP, &regs.flags);
    JR_IS1_DEBUG_BIT(pr, "ip_mc", QOS, FLAGS, IP_MC, &regs.flags);
    JR_IS1_DEBUG_BIT(pr, "ip4", QOS, FLAGS, IP4, &regs.flags);
    JR_IS1_DEBUG_BIT(pr, "tcp_udp", QOS, FLAGS, TCP_UDP, &regs.flags);
    JR_IS1_DEBUG_BIT(pr, "tcp", QOS, FLAGS, TCP, &regs.flags);
    pr("\n");
    JR_IS1_DEBUG_FLD(pr, "etype", QOS, FLAGS, ETYPE, &regs.flags);

    /* L2_MAC_ADDR */
    JR_IS1_DEBUG_FLD(pr, "mac_hi", QOS, L2_MAC_ADDR_HIGH, L2_MAC_ADDR_HIGH, &regs.mac.mach);
    JR_IS1_DEBUG_FLD(pr, "mac_lo", VLAN_PAG, L2_MAC_ADDR_LOW, L2_MAC_ADDR_LOW, &regs.mac.macl);
    
    /* L3_IP_SIP */
    jr_debug_vcap_reg(pr, "sip", &regs.sip, 0);

    /* L3_MISC */
    JR_IS1_DEBUG_BIT(pr, "fragment", QOS, L3_MISC, L3_FRAGMENT, &regs.l3_misc);
    
    /* L4_MISC */
    JR_IS1_DEBUG_FLD(pr, "l4_rng", QOS, L4_MISC, L4_RNG, &regs.l4_misc);

    /* L4_PORT */
    JR_IS1_DEBUG_FLD(pr, "l4_sport", QOS, L4_PORT, L4_SPORT, &regs.l4_port);
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_vcap_is1(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    return jr_debug_vcap(VTSS_TCAM_IS1, "IS1", pr, info, jr_debug_is1);
}

/* The register field layout is the same for different entry types, 
   so we use a common function to extract and show fields */
static vtss_rc jr_debug_is2_regs(const vtss_debug_printf_t pr,
                                 const char *name,
                                 jr_vcap_is2_regs_t *regs)
{
    int  i;
    char buf[80];

    /* Check if entry is valid */
    if (JR_GET_BIT(VCAP_IS2, MAC_ETYPE_ENTRY_ACE_VLD, VLD, regs->vld.value) == 0)
        return VTSS_RC_ERROR;
    
    /* Type */
    sprintf(buf, "type_%s", name);
    jr_debug_vcap_reg(pr, buf, &regs->entry_type, 0xf);    

    /* ACE_TYPE */
    JR_IS2_DEBUG_BIT(pr, "first", MAC_ETYPE, ACE_TYPE, FIRST, &regs->type);
    JR_IS2_DEBUG_BIT(pr, "l2_mc", MAC_ETYPE, ACE_TYPE, L2_MC, &regs->type);
    JR_IS2_DEBUG_BIT(pr, "l2_bc", MAC_ETYPE, ACE_TYPE, L2_BC, &regs->type);
    pr("\n");
    JR_IS2_DEBUG_FLD(pr, "vid", MAC_ETYPE, ACE_TYPE, VID, &regs->type);
    JR_IS2_DEBUG_FLD(pr, "uprio", MAC_ETYPE, ACE_TYPE, UPRIO, &regs->type);
    JR_IS2_DEBUG_BIT(pr, "cfi", MAC_ETYPE, ACE_TYPE, CFI, &regs->type);
    JR_IS2_DEBUG_BIT(pr, "vlan_tagged", MAC_ETYPE, ACE_TYPE, VLAN_TAGGED, &regs->type);
    pr("\n");
    JR_IS2_DEBUG_BIT(pr, "l2_fwd", MAC_ETYPE, ACE_TYPE, L2_FWD, &regs->type);

    switch (regs->entry_type.value) {
    case IS2_ENTRY_TYPE_LLC:
    case IS2_ENTRY_TYPE_SNAP:
    case IS2_ENTRY_TYPE_ARP:
    case IS2_ENTRY_TYPE_OAM:
        break;
    default:
        /* These fields are not available for all frame types */
        JR_IS2_DEBUG_BIT(pr, "l3_rt", MAC_ETYPE, ACE_TYPE, L3_RT, &regs->type);
        JR_IS2_DEBUG_BIT(pr, "l3_smac_sip_match", MAC_ETYPE, ACE_TYPE, L3_SMAC_SIP_MATCH, 
                         &regs->type);
        JR_IS2_DEBUG_BIT(pr, "l3_dmac_dip_match", MAC_ETYPE, ACE_TYPE, L3_DMAC_DIP_MATCH, 
                         &regs->type);
        break;
    }
    JR_IS2_DEBUG_BIT(pr, "isdx_neq0", MAC_ETYPE, ACE_TYPE, ISDX_NEQ0, &regs->type);
    pr("\n");
    
    /* PAG */
    JR_IS2_DEBUG_FLD(pr, "pag", MAC_ETYPE, ACE_PAG, PAG, &regs->pag);

    /* Port mask */
    jr_debug_vcap_reg(pr, "igr_port_mask", &regs->port_mask, 0);

    /* SMAC/DMAC */
    jr_debug_vcap_mac(pr, "smac", &regs->smac);
    jr_debug_vcap_mac(pr, "dmac", &regs->dmac);

    /* SIP/DIP */
    jr_debug_vcap_reg(pr, "sip", &regs->sip, 0);
    jr_debug_vcap_reg(pr, "dip", &regs->dip, 0);
    
    /* MISC */
    JR_IS2_DEBUG_BIT(pr, "ip4", IP_OTHER, ACE_L2_MISC, IP4, &regs->l2_misc);
    JR_IS2_DEBUG_BIT(pr, "ttl", IP_OTHER, ACE_L3_MISC, L3_TTL_GT0, &regs->l3_misc);
    JR_IS2_DEBUG_BIT(pr, "frag", IP_OTHER, ACE_L3_MISC, L3_FRAGMENT, &regs->l3_misc);
    JR_IS2_DEBUG_BIT(pr, "options", IP_OTHER, ACE_L3_MISC, L3_OPTIONS, &regs->l3_misc);
    JR_IS2_DEBUG_FLD(pr, "tos", IP_OTHER, ACE_L3_MISC, L3_TOS, &regs->l3_misc);
    JR_IS2_DEBUG_FLD(pr, "proto", IP_OTHER, ACE_L3_MISC, L3_IP_PROTO, &regs->l3_misc);
    JR_IS2_DEBUG_BIT(pr, "dip_eq_sip", IP_OTHER, ACE_L3_MISC, DIP_EQ_SIP, &regs->l3_misc);
    if (regs->l3_misc.valid)
        pr("\n");

    /* CUSTOM_DATA */
    if (regs->data[0].valid) {
        for (i = 0; i < 8; i++) {
            sprintf(buf, "data_%u_%u", i, i + 1);
            jr_debug_vcap_reg(pr, buf, &regs->data[i], 0xffff);
            jr_debug_vcap_reg(pr, NULL, &regs->data[i+1], 0xffff);
            pr("\n");
        }
    }
    
    return VTSS_RC_OK;
}

/* Read IS2 common registers */
#define JR_IS2_RD_COMMON(pre, regs)                    \
{                                                      \
    JR_IS2_RD(pre, ACE_VLD, regs.vld);                 \
    JR_IS2_RD(pre, ACE_TYPE, regs.type);               \
    JR_IS2_RD(pre, ACE_IGR_PORT_MASK, regs.port_mask); \
    JR_IS2_RD(pre, ACE_PAG, regs.pag);                 \
}

/* Read IS2 L2 common registers */
#define JR_IS2_RD_L2(pre, regs)                 \
{                                               \
    JR_IS2_RD_COMMON(pre, regs);                \
    JR_IS2_RD_MAC(pre, ACE_L2_SMAC, regs.smac); \
    JR_IS2_RD_MAC(pre, ACE_L2_DMAC, regs.dmac); \
}

/* Read IS2 IP common registers */
#define JR_IS2_RD_IP(pre, regs)                \
{                                              \
    JR_IS2_RD_COMMON(pre, regs);               \
    JR_IS2_RD(pre, ACE_L3_IP4_SIP, regs.sip);  \
    JR_IS2_RD(pre, ACE_L3_IP4_DIP, regs.dip);  \
    JR_IS2_RD(pre, ACE_L2_MISC, regs.l2_misc); \
    JR_IS2_RD(pre, ACE_L3_MISC, regs.l3_misc); \
}

#define JR_IS2_RD_CUSTOM_IDX(pre, regs, ix) JR_IS2_RD(pre, ACE_CUSTOM_DATA_##ix, regs.data[ix])

/* Read IS2 CUSTOM registers */
#define JR_IS2_RD_CUSTOM(idx, regs)              \
{                                                \
    JR_IS2_RD_COMMON(CUSTOM_##idx, regs);        \
    JR_IS2_RD_CUSTOM_IDX(CUSTOM_##idx, regs, 0); \
    JR_IS2_RD_CUSTOM_IDX(CUSTOM_##idx, regs, 1); \
    JR_IS2_RD_CUSTOM_IDX(CUSTOM_##idx, regs, 2); \
    JR_IS2_RD_CUSTOM_IDX(CUSTOM_##idx, regs, 3); \
    JR_IS2_RD_CUSTOM_IDX(CUSTOM_##idx, regs, 4); \
    JR_IS2_RD_CUSTOM_IDX(CUSTOM_##idx, regs, 5); \
    JR_IS2_RD_CUSTOM_IDX(CUSTOM_##idx, regs, 6); \
    JR_IS2_RD_CUSTOM_IDX(CUSTOM_##idx, regs, 7); \
}

static vtss_rc jr_debug_is2(const vtss_debug_printf_t pr, BOOL action)
{
    u32                value;
    jr_vcap_is2_regs_t regs;
    jr_vcap_reg_t      misc;
    jr_vcap_mac_t      mac;
    
    if (action) {
        JR_RD(VCAP_IS2, BASETYPE_ACTION_A, &value);
        jr_debug_bit(pr, "hit_me", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_HIT_ME_ONCE);
        jr_debug_bit(pr, "cpu_copy", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_CPU_COPY_ENA);
        jr_debug_fld(pr, "cpu_qu", VTSS_X_VCAP_IS2_BASETYPE_ACTION_A_CPU_QU_NUM(value));
        jr_debug_bit(pr, "irq", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_IRQ_TRIGGER);
        jr_debug_bit(pr, "fwd", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_FW_ENA);
        jr_debug_bit(pr, "lrn", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_LRN_ENA);
        jr_debug_bit(pr, "rt", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_RT_ENA);
        jr_debug_bit(pr, "pol", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_POLICE_ENA);
        jr_debug_fld(pr, "pol_idx", VTSS_X_VCAP_IS2_BASETYPE_ACTION_A_POLICE_IDX(value));
        pr("\n");
        jr_debug_fld(pr, "dlb_offs", VTSS_X_VCAP_IS2_BASETYPE_ACTION_A_DLB_OFFSET(value));
        jr_debug_fld(pr, "mirror_probe", VTSS_X_VCAP_IS2_BASETYPE_ACTION_A_MIRROR_PROBE(value));
        jr_debug_fld(pr, "ccm_type", VTSS_X_VCAP_IS2_BASETYPE_ACTION_A_OAM_CCM_TYPE(value));
        jr_debug_bit(pr, "loopback", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_LOOPBACK_SEL);
        jr_debug_bit(pr, "ts_rx", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_A_OAM_RX_TIMESTAMP_ENA);
        jr_debug_fld(pr, "ts_tx", VTSS_X_VCAP_IS2_BASETYPE_ACTION_A_OAM_TX_TIMESTAMP_SEL(value));
        pr("\n");
        JR_RD(VCAP_IS2, BASETYPE_ACTION_B, &value);
        jr_debug_bit(pr, "redir", value & VTSS_F_VCAP_IS2_BASETYPE_ACTION_B_REDIR_ENA);
        jr_debug_fld(pr, "redir_pgid", VTSS_X_VCAP_IS2_BASETYPE_ACTION_B_REDIR_PGID(value));
        JR_RD(VCAP_IS2, BASETYPE_ACTION_C, &value);
        jr_debug_fld(pr, "port_mask", value);
        JR_RD(VCAP_IS2, BASETYPE_ACTION_D, &value);
        jr_debug_fld(pr, "cnt", value);
        
        return VTSS_RC_OK;
    }
    
    memset(&regs, 0, sizeof(regs));

    /* The entry type mask can not be read, so we let it be zero, never indicating don't care */
    regs.entry_type.valid = 1;
    JR_RDF(VCAP_IS2, IS2_CONTROL_ACE_STATUS, ACE_ENTRY_TYPE, &regs.entry_type.value);
    
    switch (regs.entry_type.value) {
    case IS2_ENTRY_TYPE_ETYPE:
        JR_IS2_RD_L2(MAC_ETYPE, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "etype", &regs));
        JR_IS2_RD(MAC_ETYPE, ACE_L2_ETYPE, misc);
        JR_IS2_DEBUG_FLD(pr, "l2_payload", MAC_ETYPE, ACE_L2_ETYPE, L2_PAYLOAD, &misc);
        JR_IS2_DEBUG_FLD(pr, "etype", MAC_ETYPE, ACE_L2_ETYPE, ETYPE, &misc);
        pr("\n");
        break;
    case IS2_ENTRY_TYPE_LLC:
        JR_IS2_RD_L2(MAC_LLC, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "llc", &regs));
        JR_IS2_RD(MAC_LLC, ACE_L2_LLC, misc);
        jr_debug_vcap_reg(pr, "llc", &misc, 0);
        break;
    case IS2_ENTRY_TYPE_SNAP:
        JR_IS2_RD_L2(MAC_SNAP, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "snap", &regs));
        JR_IS2_RD_MAC(MAC_SNAP, ACE_L2_SNAP, mac);
        jr_debug_vcap_reg(pr, "snap", &mac.mach, 0xff);
        jr_debug_vcap_reg(pr, NULL, &mac.macl, 0);
        break;
    case IS2_ENTRY_TYPE_ARP:
        JR_IS2_RD_COMMON(ARP, regs);
        JR_IS2_RD_MAC(ARP, ACE_L2_SMAC, regs.smac);
        JR_IS2_RD(ARP, ACE_L3_IP4_SIP, regs.sip);
        JR_IS2_RD(ARP, ACE_L3_IP4_DIP, regs.dip);
        VTSS_RC(jr_debug_is2_regs(pr, "arp", &regs));
        JR_IS2_RD(ARP, ACE_L3_MISC, misc);
        JR_IS2_DEBUG_BIT(pr, "dip_eq_sip", ARP, ACE_L3_MISC, DIP_EQ_SIP, &misc);
        JR_IS2_RD(ARP, ACE_L3_ARP, misc);
        JR_IS2_DEBUG_FLD(pr, "opcode", ARP, ACE_L3_ARP, ARP_OPCODE, &misc);
        JR_IS2_DEBUG_BIT(pr, "op_unkn", ARP, ACE_L3_ARP, ARP_OPCODE_UNKNOWN, &misc);
        JR_IS2_DEBUG_BIT(pr, "sender_match", ARP, ACE_L3_ARP, ARP_SENDER_MATCH, &misc);
        JR_IS2_DEBUG_BIT(pr, "target_match", ARP, ACE_L3_ARP, ARP_TARGET_MATCH, &misc);
        pr("\n");
        JR_IS2_DEBUG_BIT(pr, "len_ok", ARP, ACE_L3_ARP, ARP_LEN_OK, &misc);
        JR_IS2_DEBUG_BIT(pr, "proto_ok", ARP, ACE_L3_ARP, ARP_PROTO_SPACE_OK, &misc);
        JR_IS2_DEBUG_BIT(pr, "addr_ok", ARP, ACE_L3_ARP, ARP_ADDR_SPACE_OK, &misc);
        pr("\n");
        break;
    case IS2_ENTRY_TYPE_UDP_TCP:
        JR_IS2_RD_IP(IP_TCP_UDP, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "udp_tcp", &regs));
        JR_IS2_RD(IP_TCP_UDP, ACE_L4_PORT, misc);
        JR_IS2_DEBUG_FLD(pr, "sport", IP_TCP_UDP, ACE_L4_PORT, L4_SPORT, &misc);
        JR_IS2_DEBUG_FLD(pr, "dport", IP_TCP_UDP, ACE_L4_PORT, L4_DPORT, &misc);
        JR_IS2_RD(IP_TCP_UDP, ACE_L4_MISC, misc);
        JR_IS2_DEBUG_FLD(pr, "range", IP_TCP_UDP, ACE_L4_MISC, L4_RNG, &misc);
        pr("\n");
        JR_IS2_DEBUG_BIT(pr, "urg", IP_TCP_UDP, ACE_L4_MISC, L4_URG, &misc);
        JR_IS2_DEBUG_BIT(pr, "ack", IP_TCP_UDP, ACE_L4_MISC, L4_ACK, &misc);
        JR_IS2_DEBUG_BIT(pr, "psh", IP_TCP_UDP, ACE_L4_MISC, L4_PSH, &misc);
        JR_IS2_DEBUG_BIT(pr, "rst", IP_TCP_UDP, ACE_L4_MISC, L4_RST, &misc);
        JR_IS2_DEBUG_BIT(pr, "syn", IP_TCP_UDP, ACE_L4_MISC, L4_SYN, &misc);
        JR_IS2_DEBUG_BIT(pr, "fin", IP_TCP_UDP, ACE_L4_MISC, L4_FIN, &misc);
        JR_IS2_DEBUG_BIT(pr, "seq_eq0", IP_TCP_UDP, ACE_L4_MISC, SEQUENCE_EQ0, &misc);
        JR_IS2_DEBUG_BIT(pr, "sp_eq_dp", IP_TCP_UDP, ACE_L4_MISC, SPORT_EQ_DPORT, &misc);
        pr("\n");
        break;
    case IS2_ENTRY_TYPE_IP_OTHER:
        JR_IS2_RD_IP(IP_OTHER, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "ip_other", &regs));
        JR_IS2_RD(IP_OTHER, ACE_IP4_OTHER_0, mac.macl);
        JR_IS2_RD(IP_OTHER, ACE_IP4_OTHER_1, mac.mach);
        jr_debug_vcap_mac(pr, "ip_payload", &mac);
        break;
    case IS2_ENTRY_TYPE_IPV6:
        JR_IS2_RD_COMMON(IP6_STD, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "ipv6", &regs));
        JR_IS2_RD(IP6_STD, ACE_L3_MISC, misc);
        JR_IS2_DEBUG_FLD(pr, "proto", IP6_STD, ACE_L3_MISC, L3_IP_PROTO, &misc);
        pr("\n");
        JR_IS2_RD(IP6_STD, ACE_L3_IP6_SIP_3, misc);
        jr_debug_vcap_reg(pr, "sip_3", &misc, 0);
        JR_IS2_RD(IP6_STD, ACE_L3_IP6_SIP_2, misc);
        jr_debug_vcap_reg(pr, "sip_2", &misc, 0);
        JR_IS2_RD(IP6_STD, ACE_L3_IP6_SIP_1, misc);
        jr_debug_vcap_reg(pr, "sip_1", &misc, 0);
        JR_IS2_RD(IP6_STD, ACE_L3_IP6_SIP_0, misc);
        jr_debug_vcap_reg(pr, "sip_0", &misc, 0);
        break;
    case IS2_ENTRY_TYPE_OAM:
        JR_IS2_RD_L2(OAM, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "oam", &regs));
        JR_IS2_RD(OAM, ACE_OAM_0, misc);
        JR_IS2_DEBUG_BIT(pr, "mac_in_mac", OAM, ACE_OAM_0, ISMACINMACOAM, &misc);
        JR_IS2_DEBUG_FLD(pr, "mel_flags", OAM, ACE_OAM_0, OAM_MEL_FLAGS, &misc);
        JR_IS2_DEBUG_FLD(pr, "ver", OAM, ACE_OAM_0, OAM_VER, &misc);
        JR_IS2_RD(OAM, ACE_OAM_1, misc);
        pr("\n");
        JR_IS2_DEBUG_FLD(pr, "opcode", OAM, ACE_OAM_1, OAM_OPCODE, &misc);
        JR_IS2_DEBUG_FLD(pr, "flags", OAM, ACE_OAM_1, OAM_FLAGS, &misc);
        JR_IS2_DEBUG_FLD(pr, "mepid", OAM, ACE_OAM_1, OAM_MEPID, &misc);
        pr("\n");
        break;
    case IS2_ENTRY_TYPE_CUSTOM_0:
        JR_IS2_RD_CUSTOM(0, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "custom_0", &regs));
        break;
    case IS2_ENTRY_TYPE_CUSTOM_1:
        JR_IS2_RD_CUSTOM(1, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "custom_1", &regs));
        break;
    case IS2_ENTRY_TYPE_CUSTOM_2:
        JR_IS2_RD_CUSTOM(2, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "custom_2", &regs));
        break;
    case IS2_ENTRY_TYPE_CUSTOM_3:
        JR_IS2_RD_CUSTOM(3, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "custom_3", &regs));
        break;
    case IS2_ENTRY_TYPE_CUSTOM_4:
        JR_IS2_RD_CUSTOM(4, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "custom_4", &regs));
        break;
    case IS2_ENTRY_TYPE_CUSTOM_5:
        JR_IS2_RD_CUSTOM(5, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "custom_5", &regs));
        break;
    case IS2_ENTRY_TYPE_CUSTOM_6:
        JR_IS2_RD_CUSTOM(6, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "custom_6", &regs));
        break;
    case IS2_ENTRY_TYPE_CUSTOM_7:
        JR_IS2_RD_CUSTOM(7, regs);
        VTSS_RC(jr_debug_is2_regs(pr, "custom_7", &regs));
        break;
    default:
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_vcap_is2(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    return jr_debug_vcap(VTSS_TCAM_IS2, "IS2", pr, info, jr_debug_is2);
}

static vtss_rc jr_debug_es0(const vtss_debug_printf_t pr, BOOL action)
{
    u32           x, y, type;
    jr_vcap_reg_t reg;
    
    if (action) {
        JR_RDB(VCAP_ES0, ES0_CONTROL_ACE_STATUS, ACE_ACTION_TYPE, &type);
        if (type == JR_ES0_ACTION_MACINMAC) {
            /* MACINMAC action */
            JR_RD(VCAP_ES0, MACINMAC_ACTION_A, &x);
            if (JR_GET_BIT(VCAP_ES0, MACINMAC_ACTION_A, VLD, x) == 0)
                return VTSS_RC_OK;

            JR_RD(VCAP_ES0, MACINMAC_ACTION_MACINMAC1, &x);
            jr_debug_fld(pr, "isid_val", VTSS_X_VCAP_ES0_MACINMAC_ACTION_MACINMAC1_ISID_VAL(x));
            JR_RD(VCAP_ES0, MACINMAC_ACTION_MACINMAC2, &x);
            jr_debug_bit(pr, "bvid_ena", 
                         x & VTSS_F_VCAP_ES0_MACINMAC_ACTION_MACINMAC2_BVID_ENA);
            jr_debug_fld(pr, "itag_bsa_idx", 
                         VTSS_X_VCAP_ES0_MACINMAC_ACTION_MACINMAC2_ITAG_BSA_IDX(x));
            jr_debug_bit(pr, "itag_keep_ena", 
                         x & VTSS_F_VCAP_ES0_MACINMAC_ACTION_MACINMAC2_ITAG_KEEP_ENA);
            pr("\n");
            jr_debug_fld(pr, "stag_tpi_sel", 
                         VTSS_X_VCAP_ES0_MACINMAC_ACTION_MACINMAC2_STAG_TPI_SEL(x));
            jr_debug_fld(pr, "stag_qos_src_sel", 
                         VTSS_X_VCAP_ES0_MACINMAC_ACTION_MACINMAC2_STAG_QOS_SRC_SEL(x));
            jr_debug_bit(pr, "stag_bundling_ena", 
                         x & VTSS_F_VCAP_ES0_MACINMAC_ACTION_MACINMAC2_STAG_BUNDLING_ENA);
            pr("\n");
            JR_RD(VCAP_ES0, MACINMAC_ACTION_B, &x);
            jr_debug_fld(pr, "vid_a", VTSS_X_VCAP_ES0_MACINMAC_ACTION_B_VID_A_VAL(x));
            jr_debug_fld(pr, "esdx", VTSS_X_VCAP_ES0_MACINMAC_ACTION_B_ESDX_VAL(x));
            jr_debug_bit(pr, "dei", x & VTSS_F_VCAP_ES0_MACINMAC_ACTION_B_DEI_VAL);
            jr_debug_fld(pr, "pcp", VTSS_X_VCAP_ES0_MACINMAC_ACTION_B_PCP_VAL(x));
            jr_debug_fld(pr, "qos_src_sel", VTSS_X_VCAP_ES0_MACINMAC_ACTION_B_QOS_SRC_SEL(x));
            JR_RD(VCAP_ES0, MACINMAC_ACTION_C, &x);
        } else {
            /* TAG action */
            JR_RD(VCAP_ES0, TAG_ACTION_A, &x);
            if (JR_GET_BIT(VCAP_ES0, TAG_ACTION_A, VLD, x) == 0)
                return VTSS_RC_OK;
            JR_RD(VCAP_ES0, TAG_ACTION_TAG1, &x);
            jr_debug_fld(pr, "vid_b", VTSS_X_VCAP_ES0_TAG_ACTION_TAG1_VID_B_VAL(x));
            jr_debug_fld(pr, "tag_vid_sel", VTSS_X_VCAP_ES0_TAG_ACTION_TAG1_TAG_VID_SEL(x));
            y = VTSS_X_VCAP_ES0_TAG_ACTION_TAG1_TAG_TPI_SEL(x);
            jr_debug_fld(pr, "tag_tpi_sel", y);
            pr("(%s)\n", y == VTSS_ES0_TPID_C ? "c" : y == VTSS_ES0_TPID_S ? "s" : 
               y == VTSS_ES0_TPID_PORT ? "port" : "c/port");
            y = VTSS_X_VCAP_ES0_TAG_ACTION_TAG1_TAG_CTRL(x);
            jr_debug_fld(pr, "tag_ctrl", y);
            pr("(%s) ", y == VTSS_ES0_TAG_NONE ? "none" : y == VTSS_ES0_TAG_ES0 ? "es0" :
               y == VTSS_ES0_TAG_PORT ? "port" : "both");
            JR_RD(VCAP_ES0, TAG_ACTION_B, &x);
            jr_debug_fld(pr, "vid_a", VTSS_X_VCAP_ES0_TAG_ACTION_B_VID_A_VAL(x));
            jr_debug_fld(pr, "esdx", VTSS_X_VCAP_ES0_TAG_ACTION_B_ESDX_VAL(x));
            jr_debug_bit(pr, "dei", x & VTSS_F_VCAP_ES0_TAG_ACTION_B_DEI_VAL);
            jr_debug_fld(pr, "pcp", VTSS_X_VCAP_ES0_TAG_ACTION_B_PCP_VAL(x));
            y = VTSS_X_VCAP_ES0_TAG_ACTION_B_QOS_SRC_SEL(x);
            jr_debug_fld(pr, "qos_src_sel", y);
            pr("(%s) ", y == VTSS_ES0_QOS_CLASS ? "class" : y == VTSS_ES0_QOS_ES0 ? "es0" :
               y == VTSS_ES0_QOS_PORT ? "port" : "mapped");
            JR_RD(VCAP_ES0, TAG_ACTION_C, &x);
        }
        jr_debug_bit(pr, "hit", x);
        return VTSS_RC_OK;
    }

    JR_RDB(VCAP_ES0, ES0_CONTROL_ACE_STATUS, ACE_ENTRY_TYPE, &type);
    if (type == JR_ES0_TYPE_ISDX) {
        /* ISDX entry */
        JR_ES0_RD(ISDX, A, reg);
        if ((reg.value & VTSS_F_VCAP_ES0_ISDX_ENTRY_A_VLD) == 0)
            return VTSS_RC_ERROR;
        pr("type: isdx ");
        JR_ES0_DEBUG_BIT(pr, "isdx_neq0", ISDX, A, ISDX_NEQ0, &reg);
        JR_ES0_DEBUG_FLD(pr, "vegr_port", ISDX, A, VEGR_PORT, &reg);
        JR_ES0_RD(ISDX, ISDX1, reg);
        JR_ES0_DEBUG_FLD(pr, "isdx", ISDX, ISDX1, ISDX, &reg);
    } else {
        /* VID entry */
        JR_ES0_RD(VID, A, reg);
        if ((reg.value & VTSS_F_VCAP_ES0_VID_ENTRY_A_VLD) == 0)
            return VTSS_RC_ERROR;
        pr("type: vid ");
        JR_ES0_DEBUG_BIT(pr, "isdx_neq0", VID, A, ISDX_NEQ0, &reg);
        JR_ES0_DEBUG_FLD(pr, "vegr_port", VID, A, VEGR_PORT, &reg);
        JR_ES0_RD(VID, VID1, reg);
        JR_ES0_DEBUG_FLD(pr, "vid", VID, VID1, VID, &reg);
    }
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_vcap_es0(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    return jr_debug_vcap(VTSS_TCAM_ES0, "ES0", pr, info, jr_debug_es0);
}

static vtss_rc jr_debug_range_checkers(const vtss_debug_printf_t pr,
                                        const vtss_debug_info_t   *const info)
{
    u32 i;
    
    jr_debug_reg_header(pr, "Range Checkers");
    for (i = 0; i < VTSS_VCAP_RANGE_CHK_CNT; i++) {
        JR_DEBUG_REGX_NAME(pr, ANA_CL_2, COMMON_ADV_RNG_CTRL, i, "ADV_RNG_CTRL");
        JR_DEBUG_REGX_NAME(pr, ANA_CL_2, COMMON_ADV_RNG_VALUE_CFG, i, "ADV_RNG_VALUE");
        pr("\n");
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_acl(const vtss_debug_printf_t pr,
                            const vtss_debug_info_t   *const info)
{
    u32 i;

    VTSS_RC(jr_debug_vcap_is2(pr, info));
    
    if (info->full) {
        VTSS_RC(jr_debug_range_checkers(pr, info));

        jr_debug_reg_header(pr, "ACL Policers");
        for (i = 0; i < 32; i++) {
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_ALL_CFG_POL_ACL_RATE_CFG, i, "ACL_RATE_CFG");
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_ALL_CFG_POL_ACL_THRES_CFG, i, "ACL_THRES_CFG");
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_ALL_CFG_POL_ACL_CTRL, i, "ACL_CTRL");
            pr("\n");
        }
    }
    
    return VTSS_RC_OK;
}

#define JR_DEBUG_VLAN(pr, addr, name) JR_DEBUG_REG_NAME(pr, ANA_L3_2, COMMON_VLAN_##addr, "VLAN_"name);

static vtss_rc jr_debug_vlan(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */ // Due to VTSS_EXIT_ENTER
    vtss_vid_t        vid;
    vtss_vlan_entry_t *vlan_entry;
    BOOL              header = 1;
    u32               port, mask = 0;
    char              buf[32];
    
    for (port = 0; port < VTSS_CHIP_PORTS_ALL; port++) {
        sprintf(buf, "Port %u", port);
        jr_debug_reg_header(pr, buf);
        jr_debug_reg_inst(pr, VTSS_ANA_CL_2_PORT_FILTER_CTRL(port), port, "ANA:FILTER_CTRL");
        jr_debug_reg_inst(pr, VTSS_ANA_CL_2_PORT_VLAN_CTRL(port), port, "ANA:VLAN_CTRL");
        jr_debug_reg_inst(pr, VTSS_REW_PORT_PORT_TAG_DEFAULT(port), port, "REW:TAG_DEFAULT");
        jr_debug_reg_inst(pr, VTSS_REW_PORT_TAG_CTRL(port), port, "REW:TAG_CTRL");
        pr("\n");
    }
        
    jr_debug_reg_header(pr, "VLAN COMMON");
    JR_DEBUG_VLAN(pr, CTRL, "CTRL");
    JR_DEBUG_VLAN(pr, FILTER_CTRL, "FILTER_CTRL");
    JR_DEBUG_VLAN(pr, ISOLATED_CFG, "ISOLATED_CFG");
    JR_DEBUG_VLAN(pr, COMMUNITY_CFG, "COMMUNITY_CFG");
    JR_DEBUG_VLAN(pr, PORT_TYPE_MASK_CFG, "PORT_TYPE_MSK");
    JR_DEBUG_REG_NAME(pr, ANA_CL_2, COMMON_VLAN_STAG_CFG, "VLAN_STAG_CFG");
    pr("\n");

    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        vlan_entry = &vtss_state->vlan_table[vid];
        if (!vlan_entry->enabled && !info->full)
            continue;
        
        JR_RDX(ANA_L3_2, VLAN_VLAN_MASK_CFG, vid, &mask);
        
        if (header)
            jr_debug_print_port_header(pr, "VID   ");
        header = 0;

        pr("%-4u  ", vid);
        jr_debug_print_mask(pr, mask);

        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");
    
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        vlan_entry = &vtss_state->vlan_table[vid];
        if (!vlan_entry->enabled && !info->full)
            continue;
        sprintf(buf, "VID: %u", vid);
        jr_debug_reg_header(pr, buf);
        jr_debug_reg_inst(pr, VTSS_ANA_L3_2_VLAN_BVLAN_CFG(vid), vid, "BVLAN_CFG");
        jr_debug_reg_inst(pr, VTSS_ANA_L3_2_VLAN_VLAN_CFG(vid), vid, "VLAN_CFG");
        jr_debug_reg_inst(pr, VTSS_ANA_L3_2_VLAN_VLAN_MASK_CFG(vid), vid, "VLAN_MASK_CFG");
        pr("\n");
        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();
    }
    
    /* IS0 is used for basic VLAN classification */
    VTSS_RC(jr_debug_vcap_is0(pr, info));

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_src_table(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    u32 port, mask;

    jr_debug_print_header(pr, "Source Masks");
    jr_debug_print_port_header(pr, "Port  ");
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        JR_RDX(ANA_AC, SRC_SRC_CFG, port, &mask);
        pr("%-4u  ", port);
        jr_debug_print_mask(pr, mask);
    }
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_pvlan(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    jr_debug_reg_header(pr, "ANA_L3");
    jr_debug_reg(pr, VTSS_ANA_L3_2_COMMON_VLAN_ISOLATED_CFG, "ISOLATED_CFG");
    pr("\n");
    return jr_debug_src_table(pr, info);
}

static vtss_rc jr_debug_vxlat(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    VTSS_RC(jr_debug_vcap_is1(pr, info));
    VTSS_RC(jr_debug_vcap_es0(pr, info));
    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_aggr(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    u32 ac, port, pgid, value, mask;
    
    jr_debug_reg_header(pr, "Logical Ports");
    for (port = 0; port < VTSS_CHIP_PORTS_ALL; port++) {
        jr_debug_reg_inst(pr, VTSS_ANA_CL_2_PORT_PORT_ID_CFG(port), port, "PORT_ID_CFG");
    }
    pr("\n");

    VTSS_RC(jr_debug_src_table(pr, info));
    
    jr_debug_print_header(pr, "Aggregation Masks");
    jr_debug_print_port_header(pr, "AC    ");
    for (ac = 0; ac < 16; ac++) {
        JR_RDX(ANA_AC, AGGR_AGGR_CFG, ac, &mask);
        pr("%-4u  ", ac);
        jr_debug_print_mask(pr, mask);
    }
    pr("\n");
    
    jr_debug_print_header(pr, "Destination Masks");
    jr_debug_print_port_header(pr, "PGID            CPU  Queue  Stack  ");
    for (pgid = 0; pgid < VTSS_PGID_JAGUAR_1; pgid++) {
        JR_RDX(ANA_AC, PGID_PGID_CFG_0, pgid, &mask);
        JR_RDX(ANA_AC, PGID_PGID_CFG_1, pgid, &value);
        if (pgid > 50 && mask == 0 && !info->full)
            continue;
        pr("%-4u %-9s  %-3u  %-5u  %-5u  ", pgid,
           pgid == PGID_UC_FLOOD ? "UC" :
           pgid == PGID_MC_FLOOD ? "MC" :
           pgid == PGID_IPV4_MC_DATA ? "IPv4 Data" :
           pgid == PGID_IPV4_MC_CTRL ? "IPv4 Ctrl" :
           pgid == PGID_IPV6_MC_DATA ? "IPv6 Data" :
           pgid == PGID_IPV6_MC_CTRL ? "IPv6 Ctrl" :
           pgid == PGID_DROP ? "DROP" : 
           pgid == PGID_FLOOD ? "FLOOD" :
           (pgid >= PGID_GLAG_START) && (pgid < PGID_GLAG_START+VTSS_GLAGS*VTSS_GLAG_PORTS) ? "GLAG" : "",
           JR_GET_BIT(ANA_AC, PGID_PGID_CFG_1, PGID_CPU_COPY_ENA, value),
           JR_GET_FLD(ANA_AC, PGID_PGID_CFG_1, PGID_CPU_QU, value),
           JR_GET_BIT(ANA_AC, PGID_PGID_CFG_1, STACK_TYPE_ENA, value));
        jr_debug_print_mask(pr, mask);
    }
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_glag(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
#if defined(VTSS_FEATURE_AGGR_GLAG)
    u32 ac, id, port, pgid, value, mask, gl_memb[VTSS_GLAGS];

    pr("%-10s %-10s %-10s %-10s\n", "Port", "Lport","is-Glag","Glag_no");
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        JR_RDX(ANA_CL_2, PORT_PORT_ID_CFG, port, &value);
        pr("%-10u %-10u %-10d %-10u\n", port, 
           VTSS_X_ANA_CL_2_PORT_PORT_ID_CFG_LPORT_NUM(value), 
           ((VTSS_F_ANA_CL_2_PORT_PORT_ID_CFG_PORT_IS_GLAG_ENA & value) ? 1 : 0),
           VTSS_X_ANA_CL_2_PORT_PORT_ID_CFG_GLAG_NUM(value));
    }
    pr("\n");

    jr_debug_print_header(pr, "GLAG member count (1-8)");
    pr("%-8s %-8s\n","Glag","Members");
    for (id = 0; id < VTSS_GLAGS; id++) {
        JR_RDX(ANA_AC, GLAG_MBR_CNT_CFG, id, &value); 
        pr("%-8u %-8u\n", id, value+1);
        gl_memb[id] = value;
    }
    pr("\n");

    jr_debug_print_header(pr, "Aggregation code");
    JR_RD(ANA_CL_2, COMMON_AGGR_CFG, &value);
    jr_debug_print_mask(pr, value); 
    pr("\n");

    jr_debug_print_header(pr, "GLAG Source Masks");
    jr_debug_print_port_header(pr, "Glag  ");
    for (id = 0; id < VTSS_GLAGS; id++) {
        JR_RDX(ANA_AC, GLAG_GLAG_CFG, id, &mask);
        pr("%-4u  ", id);
        jr_debug_print_mask(pr, mask);
    }
    pr("\n");
    
    jr_debug_print_header(pr, "Aggregation Masks");
    jr_debug_print_port_header(pr, "AC    ");
    for (ac = 0; ac < 16; ac++) {
        JR_RDX(ANA_AC, AGGR_AGGR_CFG, ac, &mask);
        pr("%-4u  ", ac);
        jr_debug_print_mask(pr, mask);
    }
    pr("\n");

    
    jr_debug_print_header(pr, "GLAG PGID table");
    pr("%-8s %-8s %-8s %-8s %-8s %-8s %-8s\n", "PGID","GLAG","Stack","CPU","Queue","UPSID","UPSPN");
    for (pgid = PGID_GLAG_START; pgid < (PGID_GLAG_START+VTSS_GLAG_PORTS*VTSS_GLAGS); pgid++) {
        id = (pgid-PGID_GLAG_START)/VTSS_GLAG_PORTS;
        JR_RDX(ANA_AC, PGID_PGID_CFG_0, pgid, &mask);
        if (mask == 0 && gl_memb[id]==0) {
            continue;
        }
        gl_memb[id]=0;
        JR_RDX(ANA_AC, PGID_PGID_CFG_1, pgid, &value);
        pr("%-8u %-8u %-8d %-8d %-8u %-8u %-8u", pgid, id,
           JR_GET_BIT(ANA_AC, PGID_PGID_CFG_1, STACK_TYPE_ENA, value),
           JR_GET_BIT(ANA_AC, PGID_PGID_CFG_1, PGID_CPU_COPY_ENA, value),
           JR_GET_FLD(ANA_AC, PGID_PGID_CFG_1, PGID_CPU_QU, value),
           (mask >> 6 & 0x1F), (mask & 0x1F));

//        jr_debug_print_mask(pr, mask);
        pr("\n");
    }
    pr("\n");

#endif    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_stp(const vtss_debug_printf_t pr,
                            const vtss_debug_info_t   *const info)
{
    jr_debug_reg_header(pr, "MSTP_0");
    jr_debug_reg_inst(pr, VTSS_ANA_L3_2_MSTP_MSTP_LRN_CFG(0), 0, "MSTP_LRN_CFG");
    pr("\n");
    
    jr_debug_reg_header(pr, "L2_COMMON");
    jr_debug_reg(pr, VTSS_ANA_L2_COMMON_LRN_SECUR_CFG, "LRN_SECUR_CFG");
    jr_debug_reg(pr, VTSS_ANA_L2_COMMON_AUTO_LRN_CFG, "AUTO_LRN_CFG");
    jr_debug_reg(pr, VTSS_ANA_L2_COMMON_LRN_COPY_CFG, "LRN_COPY_CFG");
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_mirror(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    u32 i;

    for (i = 0; i < 3; i++) {
        jr_debug_reg_header(pr, i == JR_MIRROR_PROBE_RX ? "RX" : 
                            i == JR_MIRROR_PROBE_TX ? "TX" : "RESV");
        jr_debug_reg_inst(pr, VTSS_ARB_CFG_STATUS_MIRROR_CFG(i), i, "ARB:MIRROR_CFG");
        jr_debug_reg_inst(pr, VTSS_REW_COMMON_MIRROR_PROBE_CFG(i), i, "REW:PROBE_CFG");
        jr_debug_reg_inst(pr, VTSS_ANA_AC_MIRROR_PROBE_PROBE_CFG(i), i, "ANA_AC:PROBE_CFG");
        jr_debug_reg_inst(pr, VTSS_ANA_AC_MIRROR_PROBE_PROBE_PORT_CFG(i), i, "ANA_AC:PORT_CFG");
        pr("\n");
    }
    jr_debug_reg(pr, VTSS_ANA_AC_PS_COMMON_VSTAX_GMIRROR_CFG, "ANA_AC:GMIRROR_CFG");
    pr("\n");

    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_EVC)
static void jr_debug_sdx_cnt(const vtss_debug_printf_t pr, const char *name,  
                             vtss_chip_counter_pair_t *c1, vtss_chip_counter_pair_t *c2)
{
    char buf[80];
    int  bytes;

    for (bytes = 0; bytes < 2; bytes++) {
        sprintf(buf, "%s_%s", name, bytes ? "bytes" : "frames");
        jr_debug_cnt(pr, buf, c2 == NULL ? NULL : "", 
                     bytes ? &c1->bytes : &c1->frames, 
                     c2 == NULL ? 0 : (bytes ? &c2->bytes : &c2->frames));
    }
}

#define JR_DEBUG_SDX_NAME(pr, tgt, sdx, idx, name)                                        \
{                                                                                         \
    JR_DEBUG_REGX_NAME(pr, tgt, STAT_GLOBAL_CFG_##sdx##_STAT_GLOBAL_CFG, idx, name":CFG"); \
    JR_DEBUG_REGX_NAME(pr, tgt, STAT_GLOBAL_CFG_##sdx##_STAT_GLOBAL_EVENT_MASK, idx, name":EVENT_MASK"); \
}

static vtss_rc jr_debug_evc(const vtss_debug_printf_t pr,
                            const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */ // Due to VTSS_EXIT_ENTER
    vtss_ece_entry_t    *ece;
    vtss_sdx_entry_t    *isdx, *esdx;
    vtss_sdx_counters_t *icnt, *ecnt, zero_cnt;
    vtss_port_no_t      port_no;
    u32                 value, i, j;
    BOOL                header = 1;
    char                buf[80], *p;

    /* VCAPs */
    VTSS_RC(jr_debug_vcap_is0(pr, info));
    VTSS_RC(jr_debug_vcap_es0(pr, info));

    jr_debug_reg_header(pr, "REW");
    jr_debug_reg(pr, VTSS_REW_COMMON_SERVICE_CTRL, "SERVICE_CTRL");
    pr("\n");

    /* ISDX table */
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        for (isdx = ece->isdx_list; isdx != NULL; isdx = isdx->next) {
            if (header)
                pr("ISDX  ECE ID  Port  FWD_TYPE  ISDX_FWD  CDA_FWD  ES0_ISDX  FWD_ADDR  POL_IDX\n");
            header = 0;
            JR_RDX(ANA_L2, ISDX_SERVICE_CTRL, isdx->sdx, &value);
            pr("%-6u%-8u%-6u%-10u%-10u%-9u%-10u%-10u",
               isdx->sdx,
               ece->ece_id, 
               VTSS_CHIP_PORT(isdx->port_no), 
               JR_GET_FLD(ANA_L2, ISDX_SERVICE_CTRL, FWD_TYPE, value),
               JR_GET_BIT(ANA_L2, ISDX_SERVICE_CTRL, ISDX_BASED_FWD_ENA, value),
               JR_GET_BIT(ANA_L2, ISDX_SERVICE_CTRL, CDA_FWD_ENA, value),
               JR_GET_BIT(ANA_L2, ISDX_SERVICE_CTRL, ES0_ISDX_KEY_ENA, value),
               JR_GET_FLD(ANA_L2, ISDX_SERVICE_CTRL, FWD_ADDR, value));
            JR_RDX(ANA_L2, ISDX_DLB_CFG, isdx->sdx, &value);
            pr("%u\n", value);
            VTSS_EXIT_ENTER();
        }
    }
    if (!header)
        pr("\n");

    /* SDX counter setup */
    jr_debug_reg_header(pr, "GLOBAL_CFG");
    for (i = 0; i < 6; i++) {
        JR_DEBUG_SDX_NAME(pr, ANA_AC, ISDX, i, "ANA");
    }
    for (i = 0; i < 2; i++) {
        JR_DEBUG_SDX_NAME(pr, IQS, ISDX, i, "IQS");
    }
    for (i = 0; i < 2; i++) {
        JR_DEBUG_SDX_NAME(pr, OQS, ISDX, i, "OQS");
    }
    for (i = 0; i < 4; i++) {
        JR_DEBUG_SDX_NAME(pr, REW, ESDX, i, "REW");
    }
    pr("\n");
    
    memset(&zero_cnt, 0, sizeof(zero_cnt));
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        icnt = &zero_cnt;
        ecnt = &zero_cnt;
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            for (isdx = ece->isdx_list; isdx != NULL; isdx = isdx->next) {
                if (isdx->port_no == port_no) {
                    icnt = &vtss_state->sdx_info.sdx_table[isdx->sdx];
                    break;
                }
            }
            for (esdx = ece->esdx_list; esdx != NULL; esdx = esdx->next) {
                if (esdx->port_no == port_no) {
                    ecnt = &vtss_state->sdx_info.sdx_table[esdx->sdx];
                    break;
                }
            }
            if (isdx == NULL && esdx == NULL)
                continue;
            VTSS_RC(jr_sdx_counters_update(isdx, esdx, NULL, 0));
            pr("Counters for ECE ID %u, port %u (chip_port: %u), ISDX: %u, ESDX: %u\n", 
               ece->ece_id, port_no, VTSS_CHIP_PORT(port_no), 
               isdx == NULL ? 0 : isdx->sdx, esdx == NULL ? 0 : esdx->sdx);
            jr_debug_sdx_cnt(pr, "green", &icnt->rx_green, &ecnt->tx_green);
            jr_debug_sdx_cnt(pr, "yellow", &icnt->rx_yellow, &ecnt->tx_yellow);
            jr_debug_sdx_cnt(pr, "red", &icnt->rx_red, NULL);
            jr_debug_sdx_cnt(pr, "discard", &icnt->rx_discard, &ecnt->tx_discard);
            pr("\n");
            VTSS_EXIT_ENTER();
        }
    }

    /* Policer table */
    p = (buf + sprintf(buf, "Policer "));
    for (i = 0; i < VTSS_EVC_POLICERS + JR_EVC_POLICER_RESV_CNT; i++) {
        if (i == JR_EVC_POLICER_NONE)
            strcpy(p, "None");
        else if (i == JR_EVC_POLICER_DISCARD)
            strcpy(p, "Discard");
        else {
            j = (i - JR_EVC_POLICER_RESV_CNT);
            if (vtss_state->evc_policer_conf[j].enable)
                sprintf(p, "%u" , j);
            else
                continue;
        }
        jr_debug_reg_header(pr, buf);
        JR_DEBUG_REGX_NAME(pr, ANA_AC, SDLB_DLB_CFG, i, "DLB_CFG");
        JR_DEBUG_REGXY_NAME(pr, ANA_AC, SDLB_LB_CFG, i, 0, "LB_CFG_0");
        JR_DEBUG_REGXY_NAME(pr, ANA_AC, SDLB_LB_CFG, i, 1, "LB_CFG_1");
        pr("\n");
        VTSS_EXIT_ENTER();
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_EVC */

static vtss_rc jr_debug_pkt(const vtss_debug_printf_t pr,
                            const vtss_debug_info_t   *const info)
{
    u32                 port, i;
    char                buf[32];
    vtss_chip_counter_t cnt;

    /* Rx registration registers */
    for (port = 0; port < VTSS_CHIP_PORTS_ALL; port++) {
        sprintf(buf, "Port %u", port);
        jr_debug_reg_header(pr, buf);
        JR_DEBUG_REGX_NAME(pr, ANA_CL_2, PORT_CAPTURE_CFG, port, "CAPTURE_CFG");
        JR_DEBUG_REGX_NAME(pr, ANA_CL_2, PORT_CAPTURE_GXRP_CFG, port, "GXRP_CFG");
        JR_DEBUG_REGX_NAME(pr, ANA_CL_2, PORT_CAPTURE_BPDU_CFG, port, "BPDU_CFG");
        pr("\n");
    }

    /* Rx queue mapping registers */
    for (i = 0; i < 16; i++) {
        sprintf(buf, "GARP/BPDU %u", i);
        jr_debug_reg_header(pr, buf);
        JR_DEBUG_REGX_NAME(pr, ANA_CL_2, COMMON_CPU_BPDU_QU_CFG, i, "BPDU_QU_CFG");
        pr("\n");
    }
#if defined(VTSS_FEATURE_NPI)
    jr_debug_reg_header(pr, "NPI");
    JR_DEBUG_REG_NAME(pr, ARB, CFG_STATUS_STACK_CFG, "ARB:STACK_CFG");
    pr("\n");
#endif

    jr_debug_reg_header(pr, "COMMON");
    JR_DEBUG_REG_NAME(pr, ANA_CL_2, COMMON_CPU_PROTO_QU_CFG, "ANA_CL:PROT_QU_CFG");
    JR_DEBUG_REG_NAME(pr, ANA_L2, COMMON_LRN_CFG, "ANA_L2:LRN_CFG");
    JR_DEBUG_REG_NAME(pr, ANA_L2, COMMON_FWD_CFG, "ANA_L2:FWD_CFG");
    pr("\n");
    
    /* IQS counters */
    for (port = VTSS_CHIP_PORT_CPU_0; port <= VTSS_CHIP_PORT_CPU_1; port++) {
        pr("CPU Tx counters (IQS) for port %u:\n\n", port);
        for (i = 0; i < VTSS_PRIOS; i++) {
            sprintf(buf, "green_%u", i);
            JR_CNT_SQS(IQS, RX, GREEN, port, i, &cnt, 0); 
            jr_debug_cnt(pr, NULL, buf, NULL, &cnt);
        }
        for (i = 0; i < VTSS_PRIOS; i++) {
            sprintf(buf, "yellow_%u", i);
            JR_CNT_SQS(IQS, RX, YELLOW, port, i, &cnt, 0); 
            jr_debug_cnt(pr, NULL, buf, NULL, &cnt);
        }
        for (i = 0; i < VTSS_PRIOS; i++) {
            sprintf(buf, "drops_%u", i);
            JR_CNT_SQS(IQS, RX, QDROPS, port, i, &cnt, 0); 
            jr_debug_cnt(pr, NULL, buf, NULL, &cnt);
        }
        for (i = 0; i < VTSS_PRIOS; i++) {
            sprintf(buf, "pol_drops_%u", i);
            JR_CNT_SQS(IQS, RX, PDROPS, port, i, &cnt, 0);
            jr_debug_cnt(pr, NULL, buf, NULL, &cnt);
        }
        pr("\n");
    }

    /* OQS counters */
    port = (456/8); /* Map to queue 456-463 */
    pr("CPU Rx counters (OQS)\n\n");
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "green_%u", i);
        JR_CNT_SQS(OQS, TX, GREEN, port, i, &cnt, 0); 
        jr_debug_cnt(pr, buf, NULL, &cnt, NULL);
    }
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "yellow_%u", i);
        JR_CNT_SQS(OQS, TX, YELLOW, port, i, &cnt, 0); 
        jr_debug_cnt(pr, buf, NULL, &cnt, NULL);
    }
    for (i = 0; i < VTSS_PRIOS; i++) {
        sprintf(buf, "drops_%u", i);
        JR_CNT_SQS(OQS, RX, QDROPS, port, i, &cnt, 0); 
        jr_debug_cnt(pr, buf, NULL, &cnt, NULL);
    }
    pr("\n");
    
    /* ASM registers */
    jr_debug_reg_header(pr, "ASM");
    for (i = 0; i < 2; i++) {
        JR_DEBUG_REGX(pr, ASM, CFG_CPU_CFG, i);
    }
    for (i = 0; i < JR_STACK_PORT_CNT; i++) {
        JR_DEBUG_REGX_NAME(pr, ASM, SP_CFG_SP_RX_CFG, i, "SP_RX_CFG");
    }
    JR_DEBUG_REG_NAME(pr, ASM, SP_CFG_SP_UPSID_CFG, "SP_UPSID_CFG");
    JR_DEBUG_REG_NAME(pr, ASM, SP_CFG_SP_KEEP_TTL_CFG, "SP_KEEP_TTL_CFG");
    pr("\n");
    
    /* DSM registers */
    jr_debug_reg_header(pr, "DSM");
    JR_DEBUG_REG_NAME(pr, DSM, SP_CFG_SP_KEEP_TTL_CFG, "SP_KEEP_TTL_CFG");
    for (i = 0; i < JR_STACK_PORT_CNT; i++) {
        JR_DEBUG_REGX_NAME(pr, DSM, SP_CFG_SP_TX_CFG, i, "SP_TX_CFG");
    }    
    JR_DEBUG_REG_NAME(pr, DSM, SP_CFG_SP_XTRCT_CFG, "SP_XTRCT_CFG");
    JR_DEBUG_REG_NAME(pr, DSM, SP_CFG_SP_FRONT_PORT_INJ_CFG, "SP_PORT_INJ_CFG");
    JR_DEBUG_REG_NAME(pr, DSM, SP_CFG_SP_INJ_CFG, "SP_INJ_CFG");
    JR_DEBUG_REG_NAME(pr, DSM, SP_STATUS_SP_FRONT_PORT_INJ_STAT, "SP_PORT_INJ_STAT");
    for (i = 0; i < JR_STACK_PORT_CNT; i++) {
        JR_DEBUG_REGX_NAME(pr, DSM, SP_STATUS_SP_FRAME_CNT, i, "SP_FRAME_CNT");
    }    
    pr("\n");

    /* Packet extraction registers */
    jr_debug_reg_header(pr, "Extraction");
    for (i = 0; i < JR_PACKET_RX_QUEUE_CNT; i++)
        JR_DEBUG_REGX_NAME(pr, DEVCPU_QS, XTR_XTR_FRM_PRUNING, i, "FRM_PRUNING");
    for (i = 0; i < JR_PACKET_RX_GRP_CNT; i++)
        JR_DEBUG_REGX_NAME(pr, DEVCPU_QS, XTR_XTR_GRP_CFG, i, "GRP_CFG");
    for (i = 0; i < JR_PACKET_RX_QUEUE_CNT; i++)
        JR_DEBUG_REGX_NAME(pr, DEVCPU_QS, XTR_XTR_MAP, i, "MAP");
    JR_DEBUG_REG_NAME(pr, DEVCPU_QS, XTR_XTR_DATA_PRESENT, "DATA_PRESENT");
    JR_DEBUG_REG_NAME(pr, DEVCPU_QS, XTR_XTR_QU_DBG, "QU_DBG");
    pr("\n");

    /* Packet injection registers */
    jr_debug_reg_header(pr, "Injection");
    for (i = 0; i < JR_PACKET_TX_GRP_CNT; i++)
        JR_DEBUG_REGX_NAME(pr, DEVCPU_QS, INJ_INJ_GRP_CFG, i, "GRP_CFG");
    for (i = 0; i < JR_PACKET_TX_GRP_CNT; i++)
        JR_DEBUG_REGX_NAME(pr, DEVCPU_QS, INJ_INJ_CTRL, i, "CTRL");
    for (i = 0; i < JR_PACKET_TX_GRP_CNT; i++)
        JR_DEBUG_REGX_NAME(pr, DEVCPU_QS, INJ_INJ_ERR, i, "ERR");
    JR_DEBUG_REG_NAME(pr, DEVCPU_QS, INJ_INJ_STATUS, "STATUS");
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_lrn_table(const vtss_debug_printf_t pr,
                                   const vtss_debug_info_t   *const info)
{
    u32 value;
    
    vtss_debug_print_reg_header(pr, "LRN:COMMON:");

    jr_debug_reg(pr, VTSS_LRN_COMMON_COMMON_ACCESS_CTRL, "COMMON_ACCESS_CTRL");
    jr_debug_reg(pr, VTSS_LRN_COMMON_MAC_ACCESS_CFG_0, "MAC_ACCESS_CFG_0");
    jr_debug_reg(pr, VTSS_LRN_COMMON_MAC_ACCESS_CFG_1, "MAC_ACCESS_CFG_1");
    jr_debug_reg(pr, VTSS_LRN_COMMON_MAC_ACCESS_CFG_2, "MAC_ACCESS_CFG_2");
    jr_debug_reg(pr, VTSS_LRN_COMMON_MAC_ACCESS_CFG_3, "MAC_ACCESS_CFG_3");
    jr_debug_reg(pr, VTSS_LRN_COMMON_MAC_ACCESS_CFG_4, "MAC_ACCESS_CFG_4");
    jr_debug_reg(pr, VTSS_LRN_COMMON_SCAN_NEXT_CFG, "SCAN_NEXT_CFG");
    jr_debug_reg(pr, VTSS_LRN_COMMON_SCAN_NEXT_CFG_1, "SCAN_NEXT_CFG_1");
    //jr_debug_reg(pr, VTSS_LRN_COMMON_SCAN_LAST_ROW_CFG, "SCAN_LAST_ROW_CFG");
    jr_debug_reg(pr, VTSS_LRN_COMMON_SCAN_NEXT_CNT, "SCAN_NEXT_CNT");
    jr_debug_reg(pr, VTSS_LRN_COMMON_AUTOAGE_CFG, "AUTOAGE_CFG");
    jr_debug_reg(pr, VTSS_LRN_COMMON_AUTOAGE_CFG_1, "AUTOAGE_CFG_1");
    jr_debug_reg(pr, VTSS_LRN_COMMON_AUTO_LRN_CFG, "AUTO_LRN_CFG");
    jr_debug_reg(pr, VTSS_LRN_COMMON_CCM_CTRL, "CCM_CTRL");
    //jr_debug_reg(pr, VTSS_LRN_COMMON_CCM_TYPE_CFG(0), "CCM_TYPE_CFG(0)");
    jr_debug_reg(pr, VTSS_LRN_COMMON_CCM_STATUS, "CCM_STATUS");


    /* Read and clear LRN sticky bits */
    JR_RD(LRN_COMMON, EVENT_STICKY, &value); 
    JR_WR(LRN_COMMON, EVENT_STICKY, value); 
    vtss_debug_print_reg(pr, "EVENT_STICKY", value);

    jr_debug_reg(pr, VTSS_LRN_COMMON_LATEST_POS_STATUS, "LATEST_POS_STATUS");
    /* Read and clear RAM_INTGR_ERR_ITENT sticky bits */
    JR_RD(LRN_COMMON, RAM_INTGR_ERR_IDENT, &value); 
    JR_WR(LRN_COMMON, RAM_INTGR_ERR_IDENT, value); 
    vtss_debug_print_reg(pr, "RAM_INTGR_ERR_IDENT", value);
    
    pr("\n");

    return VTSS_RC_OK;
}

static void jr_debug_wm_dump (const vtss_debug_printf_t pr, const char *reg_name, u32 *value, u32 i) {
    u32 q;
    pr("%-25s",reg_name);
    for (q = 0; q <i; q++) {
        pr("%6u",value[q]);
    }
    pr("\n");
}
static vtss_rc jr_debug_wm(const vtss_debug_printf_t pr,
                           const vtss_debug_info_t   *const info)
{
    u32 rc_cfg[8], rc_hlwm_cfg[8], rc_atop_cfg[8], rc_gwm_cfg[8];
    u32 fwd_press[8], cnt_on_prio_lvl[8], cnt_on_port_lvl[8],  cnt_on_buf_prio_lvl[8];
    u32 cnt_on_buf_port_lvl[8], cnt_on_buf_lvl[8];
    u32 rc_mode[8], drop_mode[8], enabled[8], id[8], q, qs;
    u32 swm[8], gwm[8], hwm[8], lwm[8], atop[8], mtu_map[8], mtu[4], cmwm[8];
    u32 wm_stop, rate_ctrl, rx_pause, pause_gen, port_no, gap, wm_high, wm_low, qu, port_count, queues, chipport;
    BOOL is_mac;

    /* Run through IQS and OQS, and dump the register fields */
    is_mac = (vtss_state->create.target == VTSS_TARGET_CE_MAX_24 ||
              vtss_state->create.target == VTSS_TARGET_CE_MAX_12);

    port_count = (is_mac ? 56 : (info->full ? 89 : VTSS_CHIP_PORTS));
    for (chipport = 0; chipport < port_count; chipport++) {
        queues = 8;

        if (is_mac) {
            pr("WM for port %u:", chipport);
        } else {
            if (vtss_state->chip_count == 2 && 
                (chipport == vtss_state->port_int_0 || chipport == vtss_state->port_int_1)) {
                pr("WM for internal port: %u", chipport);
            } else if (chipport < VTSS_CHIP_PORTS) {
                /* Normal port, check if port is enabled */
                for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
                    if (VTSS_CHIP_PORT(port_no) == chipport &&
                        VTSS_PORT_CHIP_SELECTED(port_no) &&
                        info->port_list[port_no]) {
                        break;
                    }
                }
                if (port_no == vtss_state->port_count)
                    continue;
                pr("WM for port %u: (%u)", chipport, port_no);
            } else if (chipport == 32) {
                pr("WM for IQS Virtual device:%u", chipport);                
            } else if (chipport == 33) {
                pr("WM for IQS CPU port :%u", chipport);
            } else if (chipport == 34) {
                pr("WM for IQS CPU port :%u", chipport);
            } else if (chipport == 80) {
                pr("WM for OQS Virtual device:%u", chipport);                
            } else if (chipport > 80) {
                pr("WM for OQS CPU port :%u", chipport);
                queues = 1;
            } else {
                continue;
            } 
        }
        pr("\n==========================================================================\n");  

        for (qs = 0; qs<2; qs++) { /* qs=0 -> IQS  qs=1 -> OQS   */
            if (is_mac) {
                if ((qs == 0) && (chipport > 31)) {
                    continue;
                }
            } else {
                if ((qs == 0) && (chipport > 34)) {
                    continue;
                }
            }
            /* Queue Level */    
            for(q=0;q<4;q++) {
                id[q] = q;
                RDX_SQS(qs, MTU_MTU_FRM_SIZE_CFG, q, &mtu[q]);                
            }
            pr("\nMTU::%s:\n",qs ? "OQS" : "IQS" );  
            pr("-------\n");          
            jr_debug_wm_dump(pr, "MTU id:", id, 4);
            jr_debug_wm_dump(pr, "MTU size:",  mtu, 4);
            
            for (q = 0; q <queues; q++) {
                qu = port_prio_2_qu(chipport, q, qs);
                RDX_SQS(qs, QU_RAM_CFG_QU_RC_CFG, qu, &rc_cfg[q]);
                RDX_SQS(qs, QU_RAM_CFG_QU_RC_HLWM_CFG, qu, &rc_hlwm_cfg[q]);
                RDX_SQS(qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG, qu, &rc_atop_cfg[q]);
                RDX_SQS(qs, QU_RAM_CFG_MTU_QU_MAP_CFG, qu, &mtu_map[q]);
            }
        
            for (q = 0; q <queues; q++) {
                id[q] = q;
                fwd_press[q]           = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_CFG_FWD_PRESS_THRES(rc_cfg[q]));
                cnt_on_prio_lvl[q]     = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_PRIO_LVL_ENA) & rc_cfg[q] ? 1 : 0;
                cnt_on_port_lvl[q]     = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_PORT_LVL_ENA) & rc_cfg[q] ? 1 : 0;
                cnt_on_buf_prio_lvl[q] = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_BUF_PRIO_LVL_ENA) & rc_cfg[q] ? 1 : 0;
                cnt_on_buf_port_lvl[q] = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_CNT_ON_BUF_PORT_LVL_ENA) & rc_cfg[q] ? 1 : 0;
                rc_mode[q]             = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_RC_MODE) & rc_cfg[q] ? 1 : 0;
                drop_mode[q]           = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_QU_FC_DROP_MODE) & rc_cfg[q] ? 1 : 0;
                enabled[q]             = PST_SQS(VTSS_F, qs, QU_RAM_CFG_QU_RC_CFG_RX_QU_ENA) & rc_cfg[q] ? 1 : 0;
                hwm[q]                 = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_HWM(rc_hlwm_cfg[q]));
                lwm[q]                 = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_LWM(rc_hlwm_cfg[q]));
                atop[q]                = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_ATOP(rc_atop_cfg[q]));
                swm[q]                 = PST_SQS(VTSS_X, qs, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_SWM(rc_atop_cfg[q]));
            }
            pr("\nQueue level::%s:\n",qs ? "OQS" : "IQS" );  
            pr("--------------\n");  
            jr_debug_wm_dump(pr, "Queue:", id, queues);
            jr_debug_wm_dump(pr, "QU_ENA:",  enabled, queues);
            jr_debug_wm_dump(pr, "FWD_PRESS_THRES:", fwd_press, queues);
            jr_debug_wm_dump(pr, "CNT_ON_PRIO_LVL_ENA:", cnt_on_prio_lvl, queues);
            jr_debug_wm_dump(pr, "CNT_ON_PORT_LVL_ENA:", cnt_on_port_lvl, queues);
            jr_debug_wm_dump(pr, "CNT_ON_BUF_PRIO_LVL_ENA:",  cnt_on_buf_prio_lvl, queues);
            jr_debug_wm_dump(pr, "CNT_ON_BUF_PORT_LVL_ENA:",  cnt_on_buf_port_lvl, queues);
            jr_debug_wm_dump(pr, "RC_MODE:",  rc_mode, queues);
            jr_debug_wm_dump(pr, "DROP_MODE:",  drop_mode, queues);
            jr_debug_wm_dump(pr, "ATOP:",  atop, queues);
            jr_debug_wm_dump(pr, "HWM:",  hwm, queues);
            jr_debug_wm_dump(pr, "LWM:",  lwm, queues);
            jr_debug_wm_dump(pr, "SWM:",  swm, queues);
            jr_debug_wm_dump(pr, "MTU map:",  mtu_map, queues);

            /* Port Level */      
            for (q = 0; q <queues; q++) {
                if (chipport+q > port_count)
                    break;
                RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_CFG, chipport+q, &rc_cfg[q]); 
                RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG, chipport+q, &rc_hlwm_cfg[q]);
                RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG, chipport+q, &rc_atop_cfg[q]);
                RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_GWM_CFG, chipport+q, &rc_gwm_cfg[q]);
                if (qs == 1) {
                    JR_RDX(OQS, PORT_RAM_CFG_CM_PORT_WM, chipport, &cmwm[q]);
                } 
            }
        
            for (q = 0; q <queues; q++) {
                if (chipport+q > port_count)
                    break;
                id[q] = chipport+q;
                enabled[q]        = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_RX_PORT_ENA) & rc_cfg[q] ? 1 : 0;       
                cnt_on_buf_lvl[q] = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_CNT_ON_BUF_LVL_ENA) & rc_cfg[q] ? 1 : 0;
                drop_mode[q]      = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_FC_DROP_MODE) & rc_cfg[q] ? 1 : 0;
                rc_mode[q]        = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_RC_MODE) & rc_cfg[q] ? 1 : 0;
                hwm[q]            = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG_PORT_RC_PROFILE_HWM(rc_hlwm_cfg[q]));
                lwm[q]            = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG_PORT_RC_PROFILE_LWM(rc_hlwm_cfg[q]));
                atop[q]           = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_ATOP(rc_atop_cfg[q]));   
                swm[q]            = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_SWM(rc_atop_cfg[q]));   
                gwm[q]            = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_SWM(rc_gwm_cfg[q]));   
            }
            
        
            pr("\nPort Level::%s:\n",qs ? "OQS" : "IQS" );  
            pr("--------------\n");  
            jr_debug_wm_dump(pr, "QQS Port:", id, queues);
            jr_debug_wm_dump(pr, "PORT_ENA:", enabled, queues);
            jr_debug_wm_dump(pr, "CNT_ON_BUF_LVL:", cnt_on_buf_lvl, queues);
            jr_debug_wm_dump(pr, "DROP_MODE:", drop_mode, queues);
            jr_debug_wm_dump(pr, "RC_MODE:", rc_mode, queues);
            jr_debug_wm_dump(pr, "GWM:", gwm, queues);
            jr_debug_wm_dump(pr, "ATOP:", atop, queues);
            jr_debug_wm_dump(pr, "HWM:", hwm, queues);
            jr_debug_wm_dump(pr, "LWM:", lwm, queues);
            jr_debug_wm_dump(pr, "SWM:", swm, queues);
            if (qs == 1) {
                jr_debug_wm_dump(pr, "CMWM:", cmwm, queues);
            }
               
            /* Priority Level */
            for (q = 0; q <queues; q++) {
                RDX_SQS(qs, RESOURCE_CTRL_CFG_PRIO_RC_CFG, q, &rc_cfg[q]);
                RDX_SQS(qs, RESOURCE_CTRL_CFG_PRIO_RC_HLWM_CFG, q, &rc_hlwm_cfg[q]);
                RDX_SQS(qs, RESOURCE_CTRL_CFG_PRIO_RC_ATOP_SWM_CFG, q, &rc_atop_cfg[q]);
            }
            
            for (q = 0; q <queues; q++) {
                id[q] = q;
                enabled[q]        = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_PRIO_RC_CFG_RX_PRIO_ENA) & rc_cfg[q] ? 1 : 0;
                drop_mode[q]      = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_PRIO_RC_CFG_PRIO_FC_DROP_MODE) & rc_cfg[q] ? 1 : 0;
                rc_mode[q]        = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_PRIO_RC_CFG_PRIO_RC_MODE) & rc_cfg[q] ? 1 : 0;
                cnt_on_buf_lvl[q] = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_PRIO_RC_CFG_PRIO_CNT_ON_BUF_LVL_ENA) & rc_cfg[q] ? 1 : 0;
                hwm[q]            = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_PRIO_RC_HLWM_CFG_PRIO_RC_PROFILE_HWM(rc_hlwm_cfg[q]));
                lwm[q]            = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_PRIO_RC_HLWM_CFG_PRIO_RC_PROFILE_LWM(rc_hlwm_cfg[q]));
                atop[q]           = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_PRIO_RC_ATOP_SWM_CFG_PRIO_RC_PROFILE_ATOP(rc_atop_cfg[q]));
                swm[q]            = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_PRIO_RC_ATOP_SWM_CFG_PRIO_RC_PROFILE_SWM(rc_atop_cfg[q]));
            }
            
            /* pr("\nPrio level::%s:\n",qs ? "OQS" : "IQS" );   */
            /* pr("--------------\n");   */
            /* jr_debug_wm_dump(pr, "Priority:", id, 8); */
            /* jr_debug_wm_dump(pr, "RX_PRIO_ENA:", enabled, 8); */
            /* jr_debug_wm_dump(pr, "CNT_ON_BUF_LVL_ENA:",  cnt_on_buf_lvl, 8); */
            /* jr_debug_wm_dump(pr, "DROP_MODE:",  drop_mode, 8); */
            /* jr_debug_wm_dump(pr, "RC_MODE:",  rc_mode, 8); */
            /* jr_debug_wm_dump(pr, "ATOP:", atop, 8); */
            /* jr_debug_wm_dump(pr, "HWM:", hwm, 8); */
            /* jr_debug_wm_dump(pr, "LWM:", lwm, 8); */
            /* jr_debug_wm_dump(pr, "SWM:", swm, 8); */
            
            /* Buffer Port Level */
            RD_SQS(qs, RESOURCE_CTRL_CFG_BUF_RC_CFG, &rc_cfg[0]);
            RD_SQS(qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG, &rc_hlwm_cfg[0]);
            
            enabled[0]   = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_RX_BUF_ENA) & rc_cfg[0] ? 1 : 0;
            drop_mode[0] = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_BUF_PORT_FC_DROP_MODE) & rc_cfg[0] ? 1 : 0;
            atop[0]      = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_RC_CFG_BUF_PORT_RC_PROFILE_ATOP(rc_cfg[0]));
            hwm[0]       = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG_BUF_PORT_RC_PROFILE_HWM(rc_hlwm_cfg[0]));
            lwm[0]       = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_PORT_RC_HLWM_CFG_BUF_PORT_RC_PROFILE_LWM(rc_hlwm_cfg[0]));
            
            pr("\nBuf port level::%s:\n",qs ? "OQS" : "IQS" );  
            pr("-------------------\n");  
            jr_debug_wm_dump(pr, "RX_BUF_ENA:", enabled, 1);
            jr_debug_wm_dump(pr, "DROP_MODE:", drop_mode, 1);
            jr_debug_wm_dump(pr, "ATOP:", atop, 1);
            jr_debug_wm_dump(pr, "HWM:", hwm, 1);
            jr_debug_wm_dump(pr, "LWM:", lwm, 1);
            
            /* Buffer Prio Level */
            RD_SQS(qs, RESOURCE_CTRL_CFG_BUF_PRIO_RC_CFG, &rc_cfg[0]);
            RD_SQS(qs, RESOURCE_CTRL_CFG_BUF_PRIO_RC_HLWM_CFG, &rc_hlwm_cfg[0]);
            
            drop_mode[0] = PST_SQS(VTSS_F, qs, RESOURCE_CTRL_CFG_BUF_PRIO_RC_CFG_BUF_PRIO_FC_DROP_MODE) & rc_cfg[0] ? 1 : 0;
            atop[0]      = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_PRIO_RC_CFG_BUF_PRIO_RC_PROFILE_ATOP(rc_cfg[0]));
            hwm[0]       = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_PRIO_RC_HLWM_CFG_BUF_PRIO_RC_PROFILE_HWM(rc_hlwm_cfg[0]));
            lwm[0]       = PST_SQS(VTSS_X, qs, RESOURCE_CTRL_CFG_BUF_PRIO_RC_HLWM_CFG_BUF_PRIO_RC_PROFILE_LWM(rc_hlwm_cfg[0]));
        
            /* pr("\nBuf prio level::%s:\n",qs ? "OQS" : "IQS" );   */
            /* pr("-------------------\n");   */
            /* jr_debug_wm_dump(pr, "DROP_MODE:", drop_mode, 1); */
            /* jr_debug_wm_dump(pr, "ATOP:", atop, 1); */
            /* jr_debug_wm_dump(pr, "HWM:", hwm, 1); */
            /* jr_debug_wm_dump(pr, "LWM:", lwm, 1); */
            
            
            RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_CFG, chipport, &rc_cfg[0]); 
            RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG, chipport, &rc_hlwm_cfg[0]);
            RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG, chipport, &rc_atop_cfg[0]);
            RDX_SQS(qs, PORT_RAM_CFG_PORT_RC_GWM_CFG, chipport, &rc_gwm_cfg[0]);
            
            id[0] = chipport;
            enabled[0]        = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_RX_PORT_ENA) & rc_cfg[0] ? 1 : 0;       
            cnt_on_buf_lvl[0] = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_CNT_ON_BUF_LVL_ENA) & rc_cfg[0] ? 1 : 0;
            drop_mode[0]      = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_FC_DROP_MODE) & rc_cfg[0] ? 1 : 0;
            rc_mode[0]        = PST_SQS(VTSS_F, qs, PORT_RAM_CFG_PORT_RC_CFG_PORT_RC_MODE) & rc_cfg[0] ? 1 : 0;
            hwm[0]            = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG_PORT_RC_PROFILE_HWM(rc_hlwm_cfg[0]));
            lwm[0]            = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_HLWM_CFG_PORT_RC_PROFILE_LWM(rc_hlwm_cfg[0]));
            atop[0]           = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_ATOP(rc_atop_cfg[0]));   
            swm[0]            = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_SWM(rc_atop_cfg[0]));   
            gwm[0]            = PST_SQS(VTSS_X, qs, PORT_RAM_CFG_PORT_RC_ATOP_SWM_CFG_PORT_RC_PROFILE_SWM(rc_gwm_cfg[0]));   
        } /* for qs=0 (IQS) and qs=1 (OQS) */        

        if (chipport < 32) {
            /* Flowcontrol Settings */
            JR_RDX(DSM, CFG_SCH_STOP_WM_CFG, chipport, &wm_stop);
            JR_RDX(DSM, CFG_RATE_CTRL, chipport, &rate_ctrl);
            JR_RDX(DSM, CFG_RX_PAUSE_CFG, chipport, &rx_pause);
            JR_RDX(DSM, CFG_ETH_FC_GEN, chipport, &pause_gen);
            
            wm_stop = JR_GET_FLD(DSM, CFG_SCH_STOP_WM_CFG, SCH_STOP_WM, wm_stop);
            gap = JR_GET_FLD(DSM, CFG_RATE_CTRL, FRM_GAP_COMP, rate_ctrl);
            wm_high = JR_GET_FLD(DSM, CFG_RATE_CTRL, TAXI_RATE_HIGH, rate_ctrl);
            wm_low = JR_GET_FLD(DSM, CFG_RATE_CTRL, TAXI_RATE_LOW, rate_ctrl);
            rx_pause = VTSS_F_DSM_CFG_RX_PAUSE_CFG_RX_PAUSE_EN & rx_pause ? 1 : 0;
            pause_gen = VTSS_F_DSM_CFG_ETH_FC_GEN_ETH_PORT_FC_GEN & pause_gen ? 1 : 0;
        
            pr("\nFC settings in DSM:\n");      
            pr("-------------------\n");      
            jr_debug_wm_dump(pr, "SCH_STOP_WM:", &wm_stop, 1);
            jr_debug_wm_dump(pr, "GAP:", &gap, 1);
            jr_debug_wm_dump(pr, "WM_HIGH:", &wm_high, 1);
            jr_debug_wm_dump(pr, "WM_LOW:", &wm_low, 1);
            jr_debug_wm_dump(pr, "RX_PAUSE_EN:", &rx_pause, 1);
            jr_debug_wm_dump(pr, "ETH_PAUSE_GEN:", &pause_gen, 1);
            pr("\n");
        }
    } /* For each port in the port list */
    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_qos(const vtss_debug_printf_t pr,
                            const vtss_debug_info_t   *const info)
{
    u32            value, port;
    vtss_port_no_t port_no;
    int            i;
    u32            iqs_mtu[4], oqs_mtu[4];
    char           buf[32];

    jr_debug_print_header(pr, "QoS Port Classification Config");

    pr("Port PCP CLS DEI DPL D_EN Q_EN DD_EN DQ_EN\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 vlan, qos;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        JR_RDX(ANA_CL_2, PORT_VLAN_CTRL, port, &vlan);
        JR_RDX(ANA_CL_2, PORT_QOS_CFG, port, &qos);
        pr("%4u %3u %3u %3u %3u %4u %4u %5u %5u\n",
           port_no,
           VTSS_X_ANA_CL_2_PORT_VLAN_CTRL_PORT_UPRIO(vlan),
           VTSS_X_ANA_CL_2_PORT_QOS_CFG_DEFAULT_QOS_VAL(qos),
           VTSS_BOOL(vlan & VTSS_F_ANA_CL_2_PORT_VLAN_CTRL_PORT_CFI),
           VTSS_X_ANA_CL_2_PORT_QOS_CFG_DEFAULT_DP_VAL(qos),
           VTSS_BOOL(qos & VTSS_F_ANA_CL_2_PORT_QOS_CFG_UPRIO_DP_ENA),
           VTSS_BOOL(qos & VTSS_F_ANA_CL_2_PORT_QOS_CFG_UPRIO_QOS_ENA),
           VTSS_BOOL(qos & VTSS_F_ANA_CL_2_PORT_QOS_CFG_DSCP_DP_ENA),
           VTSS_BOOL(qos & VTSS_F_ANA_CL_2_PORT_QOS_CFG_DSCP_QOS_ENA));
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS Port Classification PCP, DEI to QoS class, DP level Mapping");

    pr("Port QoS class (8*DEI+PCP)           DP level (8*DEI+PCP)\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int pcp, dei, class_ct = 0, dpl_ct = 0;
        u32 class, dpl;
        char class_buf[40], dpl_buf[40];
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        for (dei = VTSS_DEI_START; dei < VTSS_DEI_END; dei++) {
            for (pcp = VTSS_PCP_START; pcp < VTSS_PCP_END; pcp++) {
                const char *delim = ((pcp == VTSS_PCP_START) && (dei == VTSS_DEI_START)) ? "" : ",";
                JR_RDXY(ANA_CL_2, PORT_UPRIO_MAP_CFG, port, (8*dei + pcp), &class);
                JR_RDXY(ANA_CL_2, PORT_DP_CONFIG,     port, (8*dei + pcp), &dpl);
                class_ct += snprintf(class_buf + class_ct, sizeof(class_buf) - class_ct, "%s%u", delim,
                                     VTSS_X_ANA_CL_2_PORT_UPRIO_MAP_CFG_UPRIO_CFI_TRANSLATE_QOS_VAL(class));
                dpl_ct   += snprintf(dpl_buf   + dpl_ct,   sizeof(dpl_buf)   - dpl_ct,   "%s%u",  delim,
                                     VTSS_X_ANA_CL_2_PORT_DP_CONFIG_UPRIO_CFI_DP_VAL(dpl));
            }
        }
        pr("%4u %s %s\n", port_no, class_buf, dpl_buf);
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS Port Leaky Bucket and Scheduler Config");

    JR_RD(SCH, ETH_ETH_LB_DWRR_FRM_ADJ, &value);
    pr("Frame Adjustment (gap value): %u bytes\n", VTSS_X_SCH_ETH_ETH_LB_DWRR_FRM_ADJ_FRM_ADJ(value));
    pr("Port F_EN Mode C0 C1 C2 C3 C4 C5\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 frm_adj_ena, dwrr_cfg, cost;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        JR_RDX(SCH, ETH_ETH_LB_DWRR_CFG, port, &frm_adj_ena);
        JR_RDX(SCH, ETH_ETH_DWRR_CFG, port, &dwrr_cfg);
        cost = VTSS_X_SCH_ETH_ETH_DWRR_CFG_COST_CFG(dwrr_cfg);
        pr("%4u %4u %4u %2u %2u %2u %2u %2u %2u\n",
           port_no,
           VTSS_BOOL(frm_adj_ena & VTSS_F_SCH_ETH_ETH_LB_DWRR_CFG_FRM_ADJ_ENA),
           VTSS_BOOL(dwrr_cfg & VTSS_F_SCH_ETH_ETH_DWRR_CFG_DWRR_MODE),
           VTSS_EXTRACT_BITFIELD(cost,  0, 5),
           VTSS_EXTRACT_BITFIELD(cost,  5, 5),
           VTSS_EXTRACT_BITFIELD(cost, 10, 5),
           VTSS_EXTRACT_BITFIELD(cost, 15, 5),
           VTSS_EXTRACT_BITFIELD(cost, 20, 5),
           VTSS_EXTRACT_BITFIELD(cost, 25, 5));
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS Port and Queue Shaper enable/disable Config");

    pr("Port S Ex Q0-Q7\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 queue_excess;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        JR_RDX(SCH, ETH_ETH_SHAPING_CTRL, port, &value);
        queue_excess = VTSS_X_SCH_ETH_ETH_SHAPING_CTRL_PRIO_LB_EXS_ENA(value);
        pr("%4u %u %u%u%u%u%u%u%u%u\n",
           port_no,
           VTSS_BOOL(value & VTSS_F_SCH_ETH_ETH_SHAPING_CTRL_SHAPING_ENA),
           VTSS_BOOL(queue_excess & VTSS_BIT(0)),
           VTSS_BOOL(queue_excess & VTSS_BIT(1)),
           VTSS_BOOL(queue_excess & VTSS_BIT(2)),
           VTSS_BOOL(queue_excess & VTSS_BIT(3)),
           VTSS_BOOL(queue_excess & VTSS_BIT(4)),
           VTSS_BOOL(queue_excess & VTSS_BIT(5)),
           VTSS_BOOL(queue_excess & VTSS_BIT(6)),
           VTSS_BOOL(queue_excess & VTSS_BIT(7)));
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS Port and Queue Shaper Burst and Rate Config");

    pr("Port Queue Burst Rate\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 burst, rate;
        int queue;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        JR_RDX(SCH, ETH_LB_ETH_LB_THRES, ((9 * port) + 8), &burst);
        JR_RDX(SCH, ETH_LB_ETH_LB_RATE,  ((9 * port) + 8), &rate);
        
        pr("%4u     - 0x%02x  0x%04x\n     ",
           port_no,
           VTSS_X_SCH_ETH_LB_ETH_LB_THRES_LB_THRES(burst),
           VTSS_X_SCH_ETH_LB_ETH_LB_RATE_LB_RATE(rate));
        for (queue = 0; queue < 8; queue++) {
            JR_RDX(SCH, ETH_LB_ETH_LB_THRES, ((9 * port) + queue), &burst);
            JR_RDX(SCH, ETH_LB_ETH_LB_RATE,  ((9 * port) + queue), &rate);
            pr("%5d 0x%02x  0x%04x\n     ",
               queue,
               VTSS_X_SCH_ETH_LB_ETH_LB_THRES_LB_THRES(burst),
               VTSS_X_SCH_ETH_LB_ETH_LB_RATE_LB_RATE(rate));
        }
        pr("\r");
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS Port Tag Remarking Config");

    pr("Port Mode PCP DEI DPL mapping\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 tag_default, dp_map, tag_ctrl;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        JR_RDX(REW, PORT_PORT_TAG_DEFAULT, port, &tag_default);
        JR_RDX(REW, PORT_PORT_DP_MAP,      port, &dp_map);
        JR_RDX(REW, PORT_TAG_CTRL,         port, &tag_ctrl);
        pr("%4u %4x %3d %3d   %d %d %d %d\n",
           port_no,
           VTSS_X_REW_PORT_TAG_CTRL_QOS_SEL(tag_ctrl),
           VTSS_X_REW_PORT_PORT_TAG_DEFAULT_PORT_TCI_PRIO(tag_default),
           VTSS_BOOL(tag_default & VTSS_F_REW_PORT_PORT_TAG_DEFAULT_PORT_TCI_CFI),
           VTSS_BOOL(dp_map & VTSS_BIT(0)),
           VTSS_BOOL(dp_map & VTSS_BIT(1)),
           VTSS_BOOL(dp_map & VTSS_BIT(2)),
           VTSS_BOOL(dp_map & VTSS_BIT(3)));
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS Port Tag Remarking Map");

    pr("Port PCP (2*QoS class+DPL)           DEI (2*QoS class+DPL)\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int class, pcp_ct = 0, dei_ct = 0;
        u32 pcp_dp0, pcp_dp1, dei_dp0, dei_dp1;
        char pcp_buf[40], dei_buf[40];
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        for (class = VTSS_QUEUE_START; class < VTSS_QUEUE_END; class++) {
            const char *delim = (class == VTSS_QUEUE_START) ? "" : ",";
            JR_RDXY(REW, PORT_PCP_MAP_DE0, port, class, &pcp_dp0);
            JR_RDXY(REW, PORT_PCP_MAP_DE1, port, class, &pcp_dp1);
            JR_RDXY(REW, PORT_DEI_MAP_DE0, port, class, &dei_dp0);
            JR_RDXY(REW, PORT_DEI_MAP_DE1, port, class, &dei_dp1);
            pcp_ct += snprintf(pcp_buf + pcp_ct, sizeof(pcp_buf) - pcp_ct, "%s%u,%u",
                               delim,
                               VTSS_X_REW_PORT_PCP_MAP_DE0_PCP_DE0(pcp_dp0),
                               VTSS_X_REW_PORT_PCP_MAP_DE1_PCP_DE1(pcp_dp1));
            dei_ct += snprintf(dei_buf + dei_ct, sizeof(dei_buf) - dei_ct, "%s%u,%u",
                               delim,
                               VTSS_BOOL(dei_dp0 & VTSS_F_REW_PORT_DEI_MAP_DE0_DEI_DE0),
                               VTSS_BOOL(dei_dp1 & VTSS_F_REW_PORT_DEI_MAP_DE1_DEI_DE1));
        }
        pr("%4u %s %s\n", port_no, pcp_buf, dei_buf);
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS Port DSCP Remarking Config");

    pr("Port I_Mode Keep Trans Update Remap\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 qos_cfg, dscp_map;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        JR_RDX(ANA_CL_2, PORT_QOS_CFG, port, &qos_cfg);
        JR_RDX(REW, PORT_DSCP_MAP, port, &dscp_map);
        pr("%4u %6u %4u %5u %6u %5u\n",
           port_no,
           VTSS_X_ANA_CL_2_PORT_QOS_CFG_DSCP_REWR_MODE_SEL(qos_cfg),
           VTSS_BOOL(qos_cfg & VTSS_F_ANA_CL_2_PORT_QOS_CFG_DSCP_KEEP_ENA),
           VTSS_BOOL(qos_cfg & VTSS_F_ANA_CL_2_PORT_QOS_CFG_DSCP_TRANSLATE_ENA),
           VTSS_BOOL(dscp_map & VTSS_F_REW_PORT_DSCP_MAP_DSCP_UPDATE_ENA),
           VTSS_BOOL(dscp_map & VTSS_F_REW_PORT_DSCP_MAP_DSCP_REMAP_ENA));
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS DSCP Config");

    pr("DSCP Trans CLS DPL Rewr Trust Remap\n");
    for (i = 0; i < 64; i++) {
        u32 dscp_cfg, dscp_remap;
        JR_RDX(ANA_CL_2, COMMON_DSCP_CFG, i, &dscp_cfg);
        JR_RDX(REW, COMMON_DSCP_REMAP, i, &dscp_remap);
        pr("%4u %5u %3u %3u %4u %5u %5u\n",
           i,
           VTSS_X_ANA_CL_2_COMMON_DSCP_CFG_DSCP_TRANSLATE_VAL(dscp_cfg),
           VTSS_X_ANA_CL_2_COMMON_DSCP_CFG_DSCP_QOS_VAL(dscp_cfg),
           VTSS_X_ANA_CL_2_COMMON_DSCP_CFG_DSCP_DP_VAL(dscp_cfg),
           VTSS_BOOL(dscp_cfg & VTSS_F_ANA_CL_2_COMMON_DSCP_CFG_DSCP_REWR_ENA),
           VTSS_BOOL(dscp_cfg & VTSS_F_ANA_CL_2_COMMON_DSCP_CFG_DSCP_TRUST_ENA),
           VTSS_X_REW_COMMON_DSCP_REMAP_DSCP_REMAP(dscp_remap));
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS DSCP Classification from QoS Config");

    pr("QoS DSCP\n");
    for (i = 0; i < 8; i++) {
        u32 qos;
        JR_RDX(ANA_CL_2, COMMON_QOS_MAP_CFG, i, &qos);
        pr("%3u %4u\n",
           i,
           VTSS_X_ANA_CL_2_COMMON_QOS_MAP_CFG_DSCP_REWR_VAL(qos));
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS WRED Config");

    for (i = 0; i < 4; i++) {
        RDX_SQS(0, MTU_MTU_FRM_SIZE_CFG, i, &iqs_mtu[i]);                
        RDX_SQS(1, MTU_MTU_FRM_SIZE_CFG, i, &oqs_mtu[i]);                
    }

    pr("Port Q I/O E G Min MMx Dp1 Dp2 Dp3 Wq D   SWM   HWM  ATOP  MTU\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int queue, output;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        pr("%4u ", port_no);
        for (queue = 0; queue < 8; queue++) {
            u32 instance = (port * 8) + queue;  
            for (output = 0; output < 2; output++) {
                u32 red_min_max, red_misc, red_wq, maxp, rc_cfg, hlwm, atop_swm, map_cfg, mtu_ix;
                RDX_SQS(output, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG, instance, &red_min_max);
                RDX_SQS(output, QU_RAM_CFG_RED_ECN_MISC_CFG, instance, &red_misc);
                maxp = PST_SQS(VTSS_X, output, QU_RAM_CFG_RED_ECN_MISC_CFG_RED_ECN_PROF_MAXP(red_misc));
                RDX_SQS(output, RED_RAM_RED_ECN_WQ_CFG, instance, &red_wq);

                RDX_SQS(output, QU_RAM_CFG_QU_RC_CFG, instance, &rc_cfg);
                RDX_SQS(output, QU_RAM_CFG_QU_RC_HLWM_CFG, instance, &hlwm);
                RDX_SQS(output, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG, instance, &atop_swm);
                RDX_SQS(output, QU_RAM_CFG_MTU_QU_MAP_CFG, instance, &map_cfg);
                mtu_ix = PST_SQS(VTSS_X, output, QU_RAM_CFG_MTU_QU_MAP_CFG_MTU_PRE_ALLOC(map_cfg));

                pr("%1d  %c  %1d %1d %3d %3d %3d %3d %3d %2d %1d %5d %5d %5d %1d:%2d\n     ",
                   queue,
                   output ? 'O' : 'I',
                   PST_SQS(VTSS_F, output, QU_RAM_CFG_RED_ECN_MISC_CFG_RED_ENA) & red_misc ? 1 : 0,
                   PST_SQS(VTSS_X, output, QU_RAM_CFG_RED_ECN_MISC_CFG_RED_THRES_GAIN(red_misc)),
                   PST_SQS(VTSS_X, output, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG_RED_ECN_PROFILE_MINTH(red_min_max)),
                   PST_SQS(VTSS_X, output, QU_RAM_CFG_RED_ECN_MINTH_MAXTH_CFG_RED_ECN_PROFILE_MINMAXTH(red_min_max)),
                   VTSS_EXTRACT_BITFIELD(maxp,  0, 8),
                   VTSS_EXTRACT_BITFIELD(maxp,  8, 8),
                   VTSS_EXTRACT_BITFIELD(maxp,  16, 8),
                   PST_SQS(VTSS_X, output, RED_RAM_RED_ECN_WQ_CFG_RED_ECN_PROFILE_WQ(red_wq)),
                   PST_SQS(VTSS_F, output, QU_RAM_CFG_QU_RC_CFG_QU_FC_DROP_MODE) & rc_cfg ? 1 : 0,
                   PST_SQS(VTSS_X, output, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_SWM(atop_swm)),
                   PST_SQS(VTSS_X, output, QU_RAM_CFG_QU_RC_HLWM_CFG_QU_RC_PROFILE_HWM(hlwm)),
                   PST_SQS(VTSS_X, output, QU_RAM_CFG_QU_RC_ATOP_SWM_CFG_QU_RC_PROFILE_ATOP(atop_swm)),
                   mtu_ix,
                   output ? oqs_mtu[mtu_ix] : iqs_mtu[mtu_ix]);
            }
        }
        pr("\r");
    }
    pr("\n");

    VTSS_RC(jr_debug_range_checkers(pr, info));
    VTSS_RC(jr_debug_vcap_is1(pr, info));

    jr_debug_print_header(pr, "QoS Port Policers");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int policer;
        u32 pol_idx;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        sprintf(buf, "Port %u (%u)", port, port_no);
        jr_debug_reg_header(pr, buf);
        for (policer = 0; policer < 4; policer++) {
            pol_idx = (port * 4 + policer);
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_PORT_CFG_POL_PORT_THRES_CFG_0, pol_idx, "THRES_CFG_0");
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_PORT_CFG_POL_PORT_THRES_CFG_1, pol_idx, "THRES_CFG_1");
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_PORT_CFG_POL_PORT_RATE_CFG,    pol_idx, "RATE_CFG");
            JR_DEBUG_REGXY_NAME(pr, ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, port, policer, "POL_PORT_CFG");
        }
        JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_PORT_CTRL_POL_PORT_GAP,       port, "GAP");
        JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_ALL_CFG_POL_PORT_FC_CFG,      port, "POL_PORT_FC_CFG");
    }
    pr("\n");

    jr_debug_print_header(pr, "QoS Queue Policers");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int queue;
        u32 pol_idx;
        if (jr_debug_port_skip(port_no, info))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        sprintf(buf, "Port %u (%u)", port, port_no);
        jr_debug_reg_header(pr, buf);
        for (queue = 0; queue < 8; queue++) {
            pol_idx = (port * 8 + queue);
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_PRIO_CFG_POL_PRIO_THRES_CFG, pol_idx, "THRES_CFG");
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_PRIO_CFG_POL_PRIO_RATE_CFG,  pol_idx, "RATE_CFG");
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_PRIO_CTRL_POL_PRIO_GAP,      pol_idx, "PRIO_GAP");
            JR_DEBUG_REGX_NAME(pr, ANA_AC, POL_PRIO_CTRL_POL_PRIO_CFG,      pol_idx, "PRIO_CFG");
        }
    }
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_ip_mc(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    jr_debug_reg_header(pr, "FWD_CFG");
    JR_DEBUG_REG(pr, ANA_L2, COMMON_FWD_CFG);
    pr("\n");
    
    VTSS_RC(jr_debug_mac_table(pr, info));

    VTSS_RC(jr_debug_vcap_is2(pr, info));
    
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_stack(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
#if defined(VTSS_FEATURE_VSTAX_V2)
    vtss_vstax_upsid_t       upsid;

    jr_debug_reg_header(pr, "ANA_L2/ANA_CL");
    JR_DEBUG_REG(pr, ANA_L2, COMMON_VSTAX_CTRL);
    JR_DEBUG_REG(pr, ANA_CL_2, COMMON_UPSID_CFG);
    pr("\n");

    jr_debug_reg_header(pr, "REW");
    JR_DEBUG_REGX_NAME(pr, REW, COMMON_VSTAX_PORT_GRP_CFG, 0, "PORT_GRP_CFG");
    JR_DEBUG_REGX_NAME(pr, REW, COMMON_VSTAX_PORT_GRP_CFG, 1, "PORT_GRP_CFG");
    pr("\n");
    
    jr_debug_reg_header(pr, "ANA_AC");
    jr_debug_reg(pr, VTSS_ANA_AC_PS_COMMON_STACK_A_CFG, "STACK_A_CFG");
    jr_debug_reg(pr, VTSS_ANA_AC_PS_COMMON_STACK_CFG, "STACK_CFG");
    jr_debug_reg(pr, VTSS_ANA_AC_PS_COMMON_COMMON_EQUAL_STACK_LINK_TTL_CFG, "LINK_TTL_CFG");
    for (upsid = VTSS_VSTAX_UPSID_MIN; upsid <= VTSS_VSTAX_UPSID_MAX; upsid++) {
        JR_DEBUG_REGX(pr, ANA_AC, UPSID_UPSID_CFG, upsid);
    }
    pr("\n");
#endif  /* VTSS_FEATURE_VSTAX_V2 */
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_cmef(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
#if defined(VTSS_FEATURE_VSTAX_V2)
    vtss_port_no_t           chip_port;
    int                      i, prio;
    vtss_vstax_upsid_t       upsid;
    vtss_chip_no_t           chip_no = vtss_state->chip_no;
    vtss_vstax_route_entry_t *rt;

    jr_debug_reg_header(pr, "DSM/ANA_AC General");
    JR_DEBUG_REG_NAME(pr, DSM, CM_CFG_CMM_MASK_CFG, "CMM_MASK_CFG");
    JR_DEBUG_REG_NAME(pr, ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, "COMMON_VSTAX_CFG");
    JR_DEBUG_REG_NAME(pr, DSM, CM_CFG_CMM_TO_ARB_CFG, "CMM_TO_ARB_CFG");
    JR_DEBUG_REG_NAME(pr, DSM, CM_CFG_CMEF_UPSID_ACTIVE_CFG, "CMEF_UPSID_ACTIVE");
    JR_DEBUG_REG_NAME(pr, DSM, CM_CFG_CMEF_RATE_CFG, "CMEF_RATE_CFG");
    pr("\n");

    jr_debug_reg_header(pr, "ARB Port CFG");
    for (chip_port = 0; chip_port < VTSS_CHIP_PORTS; chip_port++) {
        JR_DEBUG_REGX_NAME(pr, ARB, CFG_STATUS_INGR_SHAPER_CFG, chip_port, "INGR_SHAPER_CFG");
    }
    pr("\n");
    
    jr_debug_reg_header(pr, "DSM Port CFG");
    for (chip_port = 0; chip_port < VTSS_CHIP_PORTS; chip_port++) {
        JR_DEBUG_REGX_NAME(pr, DSM, CM_CFG_LPORT_NUM_CFG, chip_port, "LPORT_NUM_CFG");
    }
    pr("\n");

    jr_debug_reg_header(pr, "OQS Port CFG");
    for (chip_port = 0; chip_port < VTSS_CHIP_PORTS; chip_port++) {
        JR_DEBUG_REGX_NAME(pr, OQS, PORT_RAM_CFG_CM_PORT_WM, chip_port, "CM_PORT_WM");
    }
    pr("\n");

    jr_debug_reg_header(pr, "ASM/DSM");
    for (chip_port = 0; chip_port < VTSS_CHIP_PORTS; chip_port++) {
        if (VTSS_PORT_IS_STACKABLE(chip_port)) {
            u32 stackport = chip_port - JR_STACK_PORT_START; /* Stack port no */
            jr_debug_reg_inst(pr, VTSS_DSM_CM_CFG_CMEF_GEN_CFG(stackport), stackport, "DSM:CM_GEN_CFG");
            jr_debug_reg_inst(pr, VTSS_ASM_CM_CFG_CMEF_RX_CFG(stackport),  stackport, "ASM:CM_RX_CFG ");
        }
    }
    pr("\n");

    jr_debug_reg_header(pr, "ARB Queue Mask");
    for (prio = 0; prio < VTSS_PRIOS; prio++) {
        JR_DEBUG_REGX_NAME(pr, ARB, CFG_STATUS_CM_CFG_VEC0, prio, "CM_CFG_VEC0");
    }
    pr("\n");

    jr_debug_reg_header(pr, "ASM/DSM Status");
    for (i = 0; i < 2; i++) {
        JR_DEBUG_REGX_NAME(pr, ASM, CM_STATUS_CMEF_CNT, i, "CMEF_CNT");
        JR_DEBUG_REGX_NAME(pr, DSM, CM_STATUS_CMEF_GENERATED_STATUS, i, "CMEF_GENERATED");
        JR_DEBUG_REGX_NAME(pr, DSM, CM_STATUS_CMEF_RELAYED_STATUS, i, "CMEF_RELAYED");
        JR_DEBUG_REGX_NAME(pr, DSM, CM_STATUS_CMM_XTRCT_CNT, i, "CMM_XTRCT_CNT");
    }
    JR_DEBUG_REG_NAME(pr, ASM, CM_STATUS_CMEF_DISCARD_STICKY, "CMEF_DISCARD");
    pr("\n");
    
    jr_debug_reg_header(pr, "ARB Status");
    for (upsid = VTSS_VSTAX_UPSID_MIN; upsid <= VTSS_VSTAX_UPSID_MAX; upsid++) {
        rt = &vtss_state->vstax_info.chip_info[chip_no].rt_table.table[upsid];
        if (vtss_state->vstax_info.upsid[chip_no] == upsid || rt->stack_port_a || rt->stack_port_b) {
            i8 regname[64];
            JR_WRF(ARB, CFG_STATUS_CM_ACCESS, CM_UPSID, upsid);
            JR_WRB(ARB, CFG_STATUS_CM_ACCESS, CM_START_ACCESS, 1);
            JR_POLL_BIT(ARB, CFG_STATUS_CM_ACCESS, CM_START_ACCESS);
            (void)snprintf(regname, sizeof(regname), "CM_ACCESS(%d)", upsid);
            JR_DEBUG_REG_NAME(pr, ARB, CFG_STATUS_CM_ACCESS, regname);
            (void)snprintf(regname, sizeof(regname), "CM_DATA(%d)", upsid);
            JR_DEBUG_REG_NAME(pr, ARB, CFG_STATUS_CM_DATA, regname);
        }
    }
    pr("\n");
#endif  /* VTSS_FEATURE_VSTAX_V2 */
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_host(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    u32                      hmda,hmdb,mask,val,pgid,prios,val1,val2;
    u32                      hm = ce_max_hm();
    vtss_port_no_t           port_no;
    vtss_lport_no_t          lport_no;
    u32                      i, oqs_ports, id, hport, qu;
    u64                      green, yellow, drops;
    
    pr("Host mode:%u (%s)\n",hm, (hm==0)?"12x8x2":(hm==1)?"24x4x2":(hm==2)?"24x8x1":(hm==3)?"48x4x1":"illegal");

    if (hm == 0 || hm == 2)
        prios = 8;
    else
        prios = 4;

    JR_RDF(ARB, CFG_STATUS_HM_CFG, ETH_DEV_HMDA, &hmda);
    JR_RDF(ARB, CFG_STATUS_HM_CFG, ETH_DEV_HMDB, &hmdb);
    pr("Host Interfaces HMDA:%u  HMDB:%u\n", hmda, hmdb);

    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        lport_no = vtss_state->port_map[port_no].lport_no;
        if ((lport_no == VTSS_LPORT_MAP_NONE) || (lport_no == VTSS_LPORT_MAP_DEFAULT)) { /* No mapping - skip */
            continue;
        } 
        pr("Line port %u maps to Host port %u\n", port_no, lport_no);
    }

    jr_debug_print_header(pr, "Line Port PGID table");
    pr("%-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s %-8s\n", "PGID","Stack","CPU","Queue","UPSID","UPSPN","HQ","HP","Mask");
    for (pgid = PGID_HQ_START; pgid < (PGID_HQ_START+(VTSS_PORTS*8)); pgid++) {
        if ((prios == 4) && (pgid % 2))
            continue;
            
        JR_RDX(ANA_AC, PGID_PGID_CFG_0, pgid, &mask);
        if (mask == 0)
            continue;

        JR_RDX(ANA_AC, PGID_PGID_CFG_1, pgid, &val);
        pr("%-8u %-8d %-8d %-8u %-8u %-8u %-8u %-8u 0x%-8x", pgid,
           JR_GET_BIT(ANA_AC, PGID_PGID_CFG_1, STACK_TYPE_ENA, val),
           JR_GET_BIT(ANA_AC, PGID_PGID_CFG_1, PGID_CPU_COPY_ENA, val),
           JR_GET_FLD(ANA_AC, PGID_PGID_CFG_1, PGID_CPU_QU, val),
           (mask >> 6 & 0x1F), (mask & 0x1F), ((mask >> 6 & 0x1F)*32 + (mask & 0x1F)), (((mask >> 6 & 0x1F)*32 + (mask & 0x1F))-32)/prios, mask);        
        pr("\n");
    }


    /* for (port_no = 0; port_no < 32; port_no++) { */

    /*     for (i = 0; i < prios; i++) { */
    /*         qu = port_prio_2_qu(port_no, i, 0); */

    /*         JR_CNT_HOST_SQS(IQS, RX, GREEN, qu, green);    /\* Green *\/ */
    /*         JR_CNT_HOST_SQS(IQS, RX, YELLOW , qu, yellow);  /\* Yellow *\/ */
    /*         JR_CNT_HOST_SQS(IQS, RX, QDROPS, qu, qdrops);   /\* SQS and RED drops *\/ */
    /*         JR_CNT_HOST_SQS(IQS, RX, PDROPS, qu, pdrops); /\* Policer drops *\/ */
    /*         JR_CNT_HOST_SQS(IQS, TX, TXDROPS, qu, txdrops);   /\* Full OQS drops *\/ */
        
    /*         if ((yellow+green+qdrops+txdrops+pdrops) > 0) { */
    /*             pr("IQS: Port %u Queue:%u RxFrames:%u Qdrops:%u, Pdrops:%u, Txdrops:%u\n",port_no,i,yellow+green,qdrops,pdrops,txdrops);  */
    /*         } */
    /*         if (qdrops+pdrops+txdrops+yellow+green > 0) { */
    /*             JR_RDX(IQS, STAT_CNT_CFG_RX_STAT_EVENTS_STICKY, qu, &val1); */
    /*             JR_RDX(IQS, STAT_CNT_CFG_TX_STAT_EVENTS_STICKY, qu, &val2); */
    /*             pr("IQS:(Qu:%u) Rx Sticky:0x%x   Tx Sticky:0x%x\n",qu, val1, val2);  */
    /*         } */
    /*         id++; */
    /*     } */
    /* }    */

    /* OQS counters */
    if (prios == 8)
        oqs_ports = 56;
    else
        oqs_ports = 80;
        
    id = 0;
    for (hport = 0; hport < oqs_ports-32; hport++) {
        for (i = 0; i < prios; i++) {
            qu = 256 + id;
            JR_CNT_HOST_SQS(OQS, TX, GREEN, qu,  &vtss_state->lport_chip_counters[hport].tx_green[i], 0);
            JR_CNT_HOST_SQS(OQS, TX, YELLOW, qu, &vtss_state->lport_chip_counters[hport].tx_yellow[i], 0);
            JR_CNT_HOST_SQS(OQS, RX, QDROPS, qu, &vtss_state->lport_chip_counters[hport].rx_drops[i], 0);
            green = vtss_state->lport_chip_counters[hport].tx_green[i].value;
            yellow = vtss_state->lport_chip_counters[hport].tx_yellow[i].value;
            drops = vtss_state->lport_chip_counters[hport].rx_drops[i].value;
            if ((yellow+green) > 0 || drops > 0) {
                pr("OQS: Host port %u (HQ:%u) Queue:%u TxFrames:%llu Drops:%llu\n",hport,qu,i,yellow+green,drops);
            }
            if (drops>0) {
                JR_RDX(OQS, STAT_CNT_CFG_RX_STAT_EVENTS_STICKY, qu, &val1);
                JR_RDX(OQS, STAT_CNT_CFG_TX_STAT_EVENTS_STICKY, qu, &val2);
                pr("OQS:(Qu:%u) Rx Sticky:0x%x   Tx Sticky:0x%x\n",qu, val1, val2);
            }
            id++;
        }
    }   
    pr("\n");

#endif  /* VTSS_ARCH_JAGUAR_1_CE_MAC */
    return VTSS_RC_OK;
}

static vtss_rc jr_debug_routing(const vtss_debug_printf_t pr,
                                const vtss_debug_info_t   *const info)
{
    u32 tmp, tmp0, tmp1, mac_lsb, mac_msb;
    u32 row;
    u32 data, mask, conf, remap, arp0, arp1;
    vtss_mac_t mac;

#define FLAG(X, Y) ((Y) & VTSS_F_ANA_L3_2_ ## X )
#define ENA_DIS(X, Y) (FLAG(X, Y) ? "ENA":"DIS")
#define RAW_REG(var, fam, reg)                              \
    {                                                       \
        var = 0;                                            \
        JR_RD(fam, reg, &var);                              \
        pr("%-35s: 0x%08x\n", "VTSS_" #fam "_" #reg, var); \
    }
#define REG_BIT_DESC(var, desc, mask)      \
    {                                      \
        if( var & mask)                    \
            pr("  %-35s: ON\n", desc);     \
        else                               \
            pr("  %-35s: OFF\n", desc);    \
    }
#define REG_BIT(var, fam, reg, mask) \
    REG_BIT_DESC(var, #mask, VTSS_F_##fam##_##reg##_##mask)
#define MAC_FORMAT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARGS(X) \
    X.addr[0], X.addr[1], X.addr[2], X.addr[3], X.addr[4], X.addr[5]

    // TODO, extract and print individual bit fields
    pr("Raw dump of global routing related registers:\n");
    pr("=============================================\n");
    RAW_REG(tmp, ANA_L3_2_COMMON, L3_UC_ENA);
    RAW_REG(tmp, ANA_L3_2_COMMON, L3_MC_ENA);

    RAW_REG(tmp, ANA_L3_2_COMMON, ROUTING_CFG);
    REG_BIT(tmp, ANA_L3_2_COMMON, ROUTING_CFG, ALWAYS_SWITCHED_COPY_ENA);
    pr("  DIP_ADDR_VIOLATION_REDIR_ENA:\n");
    REG_BIT_DESC(tmp, "    0.0.0.0-0.255.255.255 ", VTSS_BIT(8));
    REG_BIT_DESC(tmp, "    127.0.0.0-127.255.255.255 ", VTSS_BIT(9));
    REG_BIT_DESC(tmp, "    240.0.0.0-255.255.255.254 ", VTSS_BIT(10));
    REG_BIT_DESC(tmp, "    224.0.0.0-239.255.255.255 ", VTSS_BIT(11));
    pr("  SIP_ADDR_VIOLATION_REDIR_ENA :\n");
    REG_BIT_DESC(tmp, "    0.0.0.0-0.255.255.255 ", VTSS_BIT(8));
    REG_BIT_DESC(tmp, "    127.0.0.0-127.255.255.255 ", VTSS_BIT(9));
    REG_BIT_DESC(tmp, "    224.0.0.0-255.255.255.255 ", VTSS_BIT(11));
    REG_BIT(tmp, ANA_L3_2_COMMON, ROUTING_CFG, CPU_RLEG_IP_HDR_FAIL_REDIR_ENA);
    REG_BIT(tmp, ANA_L3_2_COMMON, ROUTING_CFG, CPU_IP6_HOPBYHOP_REDIR_ENA);
    REG_BIT(tmp, ANA_L3_2_COMMON, ROUTING_CFG, CPU_IP4_OPTIONS_REDIR_ENA);
    REG_BIT(tmp, ANA_L3_2_COMMON, ROUTING_CFG, IP6_HC_REDIR_ENA);
    REG_BIT(tmp, ANA_L3_2_COMMON, ROUTING_CFG, IP4_TTL_REDIR_ENA);

    RAW_REG(tmp0, ANA_L3_2_COMMON, RLEG_CFG_0);
    RAW_REG(tmp1, ANA_L3_2_COMMON, RLEG_CFG_1);

    mac_lsb = VTSS_X_ANA_L3_2_COMMON_RLEG_CFG_0_RLEG_MAC_LSB(tmp0);
    mac.addr[5] = (mac_lsb) & 0xff;
    mac.addr[4] = (mac_lsb >> 8) & 0xff;
    mac.addr[3] = (mac_lsb >> 16) & 0xff;

    mac_msb = VTSS_X_ANA_L3_2_COMMON_RLEG_CFG_1_RLEG_MAC_MSB(tmp1);
    mac.addr[2] = (mac_msb) & 0xff;
    mac.addr[1] = (mac_msb >> 8) & 0xff;
    mac.addr[0] = (mac_msb >> 16) & 0xff;

    pr("  RLEG_MAC_TYPE_SEL (%u):\n",
            VTSS_X_ANA_L3_2_COMMON_RLEG_CFG_1_RLEG_MAC_TYPE_SEL(tmp1) );
    switch( VTSS_X_ANA_L3_2_COMMON_RLEG_CFG_1_RLEG_MAC_TYPE_SEL(tmp1) ) {
        case 0:
            pr("    RLEG_MAC_ADDR + VIMD\n");
            break;
        case 1:
            pr("    RLEG_MAC_ADDR + VID\n");
            break;
        default:
            pr("    COMMON MAC ADDRESS FOR ALL LEGS\n");
            break;
    }
    pr("  RLEG_MAC_ADDR: " MAC_FORMAT "\n", MAC_ARGS(mac));
    RAW_REG(tmp, ANA_L3_2_COMMON, VRRP_CFG_0);
    RAW_REG(tmp, ANA_L3_2_COMMON, VRRP_CFG_1);


    pr(" \n");
    pr("VLAN to rleg association\n");
    pr("========================\n");
    pr("VLAN ID   ->     RLEG ID\n");
    pr("------------------------\n");
    //("0000      ->     0000\n");
    for( row = 0; row < VTSS_VIDS; ++row ) {
        JR_RDX(ANA_L3_2_VLAN, VLAN_CFG, row, &tmp);

        if( tmp & VTSS_F_ANA_L3_2_VLAN_VLAN_CFG_VLAN_RLEG_ENA ) {
            u32 vid;

            JR_RDX(ANA_L3_2_VLAN, VMID_CFG, row, &vid);
            pr("%4u      ->     %4u\n",
                    row, VTSS_X_ANA_L3_2_VLAN_VMID_CFG_VMID(vid));
        }
    }
    pr(" \n");

    pr("Routing leg configurations\n");
    pr("==========================\n");
    pr("ID   EVID IP4_MC_TTL VRID IP6_ICMP_REDIR IP6_UC IP4_MC IP4_ICMP_REDIR IP4_UC VRID-0 VRID-1\n");
    pr("------------------------------------------------------------------------------------------\n");
    //("0000 0000 000        DIS  DIS            DIS    DIS    DIS            DIS       000    000\n");
    // TODO, MAGIC!
    for( row = 0; row < 128; ++row ) {

        JR_RDX(ANA_L3_2_VMID, RLEG_CTRL, row, &tmp);

        if( ! ((tmp & VTSS_F_ANA_L3_2_VMID_RLEG_CTRL_RLEG_VRID_ENA) ||
                (tmp & VTSS_F_ANA_L3_2_VMID_RLEG_CTRL_RLEG_IP6_ICMP_REDIR_ENA) ||
                (tmp & VTSS_F_ANA_L3_2_VMID_RLEG_CTRL_RLEG_IP6_UC_ENA) ||
                (tmp & VTSS_F_ANA_L3_2_VMID_RLEG_CTRL_RLEG_IP4_MC_ENA) ||
                (tmp & VTSS_F_ANA_L3_2_VMID_RLEG_CTRL_RLEG_IP4_ICMP_REDIR_ENA) ||
                (tmp & VTSS_F_ANA_L3_2_VMID_RLEG_CTRL_RLEG_IP4_UC_ENA)) )
            continue;

        pr("%4u ", row); // ID
        pr("%4u ", VTSS_X_ANA_L3_2_VMID_RLEG_CTRL_RLEG_EVID(tmp)); // EVID
        pr("%3u        ", // IP4_MC_TTL
                VTSS_X_ANA_L3_2_VMID_RLEG_CTRL_RLEG_IP4_MC_TTL(tmp));

        pr("%s  ", ENA_DIS(VMID_RLEG_CTRL_RLEG_VRID_ENA, tmp)); // VRID
        pr("%s            ", // IP6_ICMP_REDIR
                ENA_DIS(VMID_RLEG_CTRL_RLEG_IP6_ICMP_REDIR_ENA, tmp));
        pr("%s    ", ENA_DIS(VMID_RLEG_CTRL_RLEG_IP6_UC_ENA, tmp)); // IP6_UC
        pr("%s    ", ENA_DIS(VMID_RLEG_CTRL_RLEG_IP4_MC_ENA, tmp)); // IP4_MC
        pr("%s            ", // IP4_ICMP_REDIR
                ENA_DIS(VMID_RLEG_CTRL_RLEG_IP4_ICMP_REDIR_ENA, tmp));
        pr("%s       ", ENA_DIS(VMID_RLEG_CTRL_RLEG_IP4_UC_ENA, tmp)); // IP4_UC

        JR_RDXY(ANA_L3_2_VMID, VRRP_CFG, row, 0, &tmp);
        pr("%3u    ", VTSS_X_ANA_L3_2_VMID_VRRP_CFG_RLEG_VRID(tmp)); // VRID-0

        JR_RDXY(ANA_L3_2_VMID, VRRP_CFG, row, 1, &tmp);
        pr("%3u", VTSS_X_ANA_L3_2_VMID_VRRP_CFG_RLEG_VRID(tmp)); // VRID-1

        pr("\n");
    }
    pr(" \n");


    pr("Routing table\n");
    pr("=============\n");
    pr("                LPM TABLE                             |             ARP TABLE                   \n");
    pr("row  match    type    data       mask        eq  Ptr  | EVMID  secure secure arp    mac         \n");
    pr("                                                      |        vmid   mac    enable address     \n");
    pr("------------------------------------------------------|-----------------------------------------\n");
    // "0000 SECURITY INVALID 0x00000000 NO-READBACK Yes 0000 | 0000   No     No     Yes    000000000000

    for( row = 0; row < VTSS_JR1_LPM_CNT; ++row ) {
        // just assume that this is IPv4
        JR_WR(ANA_L3_2_LPM,
              ACCESS_CTRL,
              VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(row) |
              VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_WID(
                  VTSS_LPM_ACCESS_CTRL_WID_QUAD) |
              VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                  VTSS_LPM_ACCESS_CTRL_CMD_READ) |
              VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT
        );

        VTSS_RC(jr_lpm_table_idle());

        JR_RDX(ANA_L3_2_LPM, LPM_USAGE_CFG, 0, &conf);

        // don't print invalid entries
        if( VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_TYPE(conf) ==
                VTSS_LPM_TYPE_INVALID)
            continue;

        JR_RDX(ANA_L3_2_LPM, LPM_DATA_CFG, 0, &data);
        JR_RDX(ANA_L3_2_LPM, LPM_DATA_CFG, 0, &mask);
        JR_RD(ANA_L3_2_REMAP, REMAP_CFG, &remap);

        // print row
        pr("%4u ", row);

        // print match
        if( VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_DST_FLAG_MASK & conf ) {
            pr("BOTH     ");
        } else {
            if( VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_DST_FLAG_DATA & conf )
                pr("LPM      ");
            else
                pr("SECURITY ");
        }

        // print type
        switch (VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_TYPE(conf)) {
            case VTSS_LPM_TYPE_INVALID:
                pr("INVALID ");
                break;
            case VTSS_LPM_TYPE_IPV4_UC:
                pr("IPv4 UC ");
                break;
            case VTSS_LPM_TYPE_IPV4_MC:
                // Re-read as we before assumed this was IPv4uc
                JR_WR(ANA_L3_2_LPM,
                      ACCESS_CTRL,
                      VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(row) |
                      VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_WID(
                          VTSS_LPM_ACCESS_CTRL_WID_HALF) |
                      VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                          VTSS_LPM_ACCESS_CTRL_CMD_READ) |
                      VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT
                );
                row += 1;
                VTSS_RC(jr_lpm_table_idle());
                pr("IPv4 MC ");
                break;
            case VTSS_LPM_TYPE_IPV6_UC:
                // Re-read as we before assumed this was IPv4uc
                JR_WR(ANA_L3_2_LPM,
                      ACCESS_CTRL,
                      VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_IDX(row) |
                      VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_WID(
                          VTSS_LPM_ACCESS_CTRL_WID_FULL) |
                      VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_CMD(
                          VTSS_LPM_ACCESS_CTRL_CMD_READ) |
                      VTSS_F_ANA_L3_2_LPM_ACCESS_CTRL_ACCESS_SHOT
                );
                row += 3;
                VTSS_RC(jr_lpm_table_idle());
                pr("IPv6 UC ");
                break;
            default:
                pr("UNKNOWN ");
        };

        // print data and mask
        pr("0x%08x NO-READBACK ", data);
        pr("%s ", ENA_DIS(REMAP_REMAP_CFG_ECMP_CNT, remap));

        // print pointer
        pr("%4u | ", VTSS_X_ANA_L3_2_REMAP_REMAP_CFG_BASE_PTR(remap));

        // Fetch ARP entry
        JR_RDX(ANA_L3_2_ARP, ARP_CFG_0,
                VTSS_X_ANA_L3_2_REMAP_REMAP_CFG_BASE_PTR(remap), &arp0);
        JR_RDX(ANA_L3_2_ARP, ARP_CFG_1,
                VTSS_X_ANA_L3_2_REMAP_REMAP_CFG_BASE_PTR(remap), &arp1);

        pr("%4u   ", VTSS_X_ANA_L3_2_ARP_ARP_CFG_0_ARP_VMID(arp0));
        pr("%s    ", ENA_DIS(ARP_ARP_CFG_0_SECUR_MATCH_VMID_ENA, arp0));
        pr("%s    ", ENA_DIS(ARP_ARP_CFG_0_SECUR_MATCH_MAC_ENA, arp0));
        pr("%s    ", ENA_DIS(ARP_ARP_CFG_0_ARP_ENA, arp0));
        pr("%04x%08x",VTSS_X_ANA_L3_2_ARP_ARP_CFG_0_MAC_MSB(arp0), arp1);

        // fetch and display multi row entries
        switch (VTSS_F_ANA_L3_2_LPM_LPM_USAGE_CFG_LPM_TYPE(conf)) {
            case VTSS_LPM_TYPE_IPV6_UC:
                JR_RDX(ANA_L3_2_LPM, LPM_DATA_CFG, 1, &data);
                pr("\n                      0x%08x NO-READBACK", data);
                JR_RDX(ANA_L3_2_LPM, LPM_DATA_CFG, 2, &data);
                pr("\n                      0x%08x NO-READBACK", data);
                JR_RDX(ANA_L3_2_LPM, LPM_DATA_CFG, 3, &data);
                pr("\n                      0x%08x NO-READBACK", data);
                break;

            case VTSS_LPM_TYPE_IPV4_MC:
                JR_RDX(ANA_L3_2_LPM, LPM_DATA_CFG, 1, &data);
                pr("\n                      0x%08x NO-READBACK", data);
                break;

            default:
                ;
        };

        pr("\n");
    }
    pr("\n");


    pr("ARP table\n");
    pr("=========\n");
    pr("row   EVMID  secure secure arp    mac         \n");
    pr("             vmid   mac    enable address     \n");
    pr("----------------------------------------------\n");
    // "0000  0000   No     No     Yes    000000000000
    for (row = 0; row < VTSS_JR1_ARP_CNT; ++row) {
        // Fetch ARP entry
        JR_RDX(ANA_L3_2_ARP, ARP_CFG_0,
                VTSS_X_ANA_L3_2_REMAP_REMAP_CFG_BASE_PTR(row), &arp0);
        JR_RDX(ANA_L3_2_ARP, ARP_CFG_1,
                VTSS_X_ANA_L3_2_REMAP_REMAP_CFG_BASE_PTR(row), &arp1);

        if ((!FLAG(ARP_ARP_CFG_0_SECUR_MATCH_VMID_ENA, arp0)) &&
            (!FLAG(ARP_ARP_CFG_0_SECUR_MATCH_MAC_ENA, arp0)) &&
            (!FLAG(ARP_ARP_CFG_0_ARP_ENA, arp0))) {
            continue;
        }

        pr("%4u ", row);
        pr("%4u   ", VTSS_X_ANA_L3_2_ARP_ARP_CFG_0_ARP_VMID(arp0));
        pr("%s    ", ENA_DIS(ARP_ARP_CFG_0_SECUR_MATCH_VMID_ENA, arp0));
        pr("%s    ", ENA_DIS(ARP_ARP_CFG_0_SECUR_MATCH_MAC_ENA, arp0));
        pr("%s    ", ENA_DIS(ARP_ARP_CFG_0_ARP_ENA, arp0));
        pr("%04x%08x",VTSS_X_ANA_L3_2_ARP_ARP_CFG_0_MAC_MSB(arp0), arp1);
        pr("\n");
    }
    pr("\n");
    pr("\n");


    // dump sticky bits
    pr("Debug sticky bits\n");
    pr("=================\n");
    RAW_REG(tmp, ANA_L3_2, LPM_REMAP_STICKY_L3_LPM_REMAP_STICKY);
    REG_BIT(tmp, ANA_L3_2, LPM_REMAP_STICKY_L3_LPM_REMAP_STICKY, SECUR_IP6_LPM_FOUND_STICKY);
    REG_BIT(tmp, ANA_L3_2, LPM_REMAP_STICKY_L3_LPM_REMAP_STICKY, SECUR_IP4_LPM_FOUND_STICKY);
    REG_BIT(tmp, ANA_L3_2, LPM_REMAP_STICKY_L3_LPM_REMAP_STICKY, LPM_IP6UC_FOUND_STICKY);
    REG_BIT(tmp, ANA_L3_2, LPM_REMAP_STICKY_L3_LPM_REMAP_STICKY, LPM_IP4UC_FOUND_STICKY);
    REG_BIT(tmp, ANA_L3_2, LPM_REMAP_STICKY_L3_LPM_REMAP_STICKY, LPM_IP4MC_FOUND_STICKY);
    pr("\n");
    RAW_REG(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, BMSTP_FWD_ALLOWED_STICKY );
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, BMSTP_DISCARD_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, BVLAN_LRN_DENY_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, BMSTP_LRN_DENY_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, BMSTP_LRN_ALLOWED_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, BVLAN_LOOKUP_INVLD_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, BVLAN_IGR_FILTER_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, MSTP_FWD_ALLOWED_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, MSTP_DISCARD_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, VLAN_LRN_DENY_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, MSTP_LRN_DENY_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, MSTP_LRN_ALLOWED_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, VLAN_LOOKUP_INVLD_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_VLAN_STICKY, VLAN_IGR_FILTER_STICKY);

    RAW_REG(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, NO_MC_VMID_AVAIL_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, MC_ENTRY_NOT_FOUND_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, ENTRY_NOT_FOUND_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, SECUR_DIP_FAIL_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, MC_LOOPED_CP_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, NO_MC_FWD_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, L3_MC_FWD_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, MC_RPF_FILTER_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, L2_MC_FWD_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, UC_ICMP_REDIR_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, UC_ZERO_DMAC_FOUND_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, UC_TTL_FILTERING_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, UC_ENTRY_FOUND_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, RLEG_MC_HIT_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, RLEG_MC_TTL_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, RLEG_MC_IP_OPT_REDIR_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, SECUR_IP4_SIP_MATCH_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, SECUR_IP6_DIP_MATCH_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, SECUR_IP4_DIP_MATCH_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, RLEG_MC_HDR_ERR_REDIR_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, SECUR_SIP_FAIL_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, SECUR_IP6_SIP_MATCH_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, RLEG_UC_HIT_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, RLEG_UC_IP_OPT_REDIR_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, RLEG_UC_HDR_ERR_REDIR_STICKY);
    REG_BIT(tmp, ANA_L3_2, VLAN_ARP_L3MC_STICKY_L3_ARP_IPMC_STICKY, RLEG_UC_NONIP_REDIR_STICKY);

#undef ENA_DIS
#undef RAW_REG_DUMP
#undef REG_BIT_DESC
#undef REG_BIT
#undef MAC_FORMAT
#undef MAC_ARGS
#undef FLAG

    return VTSS_RC_OK;
}

static vtss_rc jr_debug_info_print(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_INIT, jr_debug_init, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MISC, jr_debug_misc, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PORT, jr_debug_port, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PORT_CNT, jr_debug_port_cnt, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_VLAN, jr_debug_vlan, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PVLAN, jr_debug_pvlan, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MAC_TABLE, jr_debug_mac_table, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_ACL, jr_debug_acl, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_VXLAT, jr_debug_vxlat, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_AGGR, jr_debug_aggr, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_GLAG, jr_debug_glag, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_LRN,  jr_debug_lrn_table, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_STP,  jr_debug_stp, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MIRROR, jr_debug_mirror, pr, info));
#if defined(VTSS_FEATURE_EVC)
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_EVC, jr_debug_evc, pr, info));
#endif /* VTSS_FEATURE_EVC */
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PACKET, jr_debug_pkt, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_WM,   jr_debug_wm, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_QOS,  jr_debug_qos, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_IPMC, jr_debug_ip_mc, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_TS,   jr_debug_ts, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_STACK, jr_debug_stack, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_CMEF, jr_debug_cmef, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_HOST, jr_debug_host, pr, info));
#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_FDMA)) {
        if (vtss_state->fdma_state.fdma_func.fdma_debug_print != NULL) {
            return vtss_state->fdma_state.fdma_func.fdma_debug_print(vtss_state, pr, info);
        } else {
            return VTSS_RC_ERROR;
        }
    }
#endif 
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_L3, jr_debug_routing, pr, info));
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Initialization
 * ================================================================= */

static vtss_rc jr_restart_conf_set(void)
{
    JR_WR(DEVCPU_GCB, CHIP_REGS_GENERAL_PURPOSE, vtss_cmn_restart_value_get());

    return VTSS_RC_OK;
}

static vtss_rc jr_init_hsio(void)
{
    VTSS_RC(serdes_macro_config());
    return VTSS_RC_OK;
}

/* In general the role of the assembler is:                                        */
/* - Store frames from the taxi busses as cells and send them to the Analyzer      */
/* - Detect pause frames and signal to the DSM to pause further transmission       */
/* - Detect super priority frames and send them to the DSM for direct transmission */
/* - Flowcontrol the CPU                                                           */
/* - The calender must be formated with taxi buses based on port speeds */
static vtss_rc jr_init_asm(void)
{
    u32 cbc_len = 48, i;
    vtss_port_mux_mode_t mux_mode = vtss_state->mux_mode[vtss_state->chip_no];
    int taxi_c1[] = {5,4,3,5,4,0,5,4,1,5,3,4,5,0,4,5,1,2,5,4,3,5,4,2,
                     5,4,0,5,3,4,5,1,2,5,4,6,5,4,3,5,4,0,5,1,4,5,2,15};
    int taxi_c2[] = {5,3,4,5,0,3,5,4,1,5,3,2,5,4,3,5,0,1,5,4,3,5,2,6,
                     5,3,4,5,0,3,5,6,1,5,2,3,5,4,3,5,0,1,5,3,2,5,4,15};
    int taxi_c3[] = {5,3,0,5,3,1,5,3,2,5,3,6,5,3,0,5,3,1,5,3,2,5,3,6,
                     5,3,0,5,3,1,5,2,3,5,6,3,5,0,3,5,1,3,5,2,3,5,4/* <= EEE port 27 */,15};
    int taxi_c4[] = {4,5,3,4,5,0,4,5,1,4,3,5,4,0,5,4,1,2,4,5,3,4,5,2,
                     4,5,0,4,3,5,4,1,2,4,5,6,4,5,3,4,5,0,4,1,5,4,2,15};
    int taxi_c5[] = {15,3,4,15,15,3,4,15,1,3,15,2,4,15,3,15,15,1,4,15,3,2,15,6,
                     3,15,4,15,15,3,4,15,1,2,15,3,4,15,3,15,15,1,4,3,15,2,6,15};
    int taxi_ac3[] = {4,5,3,4,5,0,4,5,1,4,5,3,4,5,0,1,4,5,2,4,5,3,4,2,
                      5,4,0,5,4,3,5,1,4,2,5,4,0,5,4,3,1,4,5,2,4,5,6,15};
    int taxi_ac5[] = {5,3,4,5,3,0,5,4,1,5,3,2,5,4,3,0,5,1,2,5,3,4,5,6,
                      3,5,0,4,5,3,1,2,5,3,4,5,0,1,5,3,2,5,4,3,5,6,9,15};
    int taxi_ac6[] = {5,3,0,5,3,1,5,3,2,5,3,6,5,3,0,1,5,3,2,5,3,6,5,9,
                      3,5,0,3,5,1,3,2,5,6,3,5,0,3,5,1,2,5,3,6,5,3,4/* <= EEE port 27 */,15};
    int *taxi_p;

    /* Determine the correct cell bus calender configuration (based on target and mux mode) */
    switch (vtss_state->create.target) {
    case VTSS_TARGET_LYNX_1:
    case VTSS_TARGET_JAGUAR_1:
    case VTSS_TARGET_CE_MAX_24:
    case VTSS_TARGET_E_STAX_III_48:
    case VTSS_TARGET_E_STAX_III_68:
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            taxi_p = taxi_c1; /* 24x1G + 4x10G (or 12G) */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
            taxi_p = taxi_c2; /* 20x1G + 4x2G5 + 3x10G (or 12G) */
        } else {
            taxi_p = taxi_c3; /* 16x1G + 8x2G5 + 2x10G (or 12G) */
        }
        break;
    case VTSS_TARGET_JAGUAR_1_DUAL:
    case VTSS_TARGET_E_STAX_III_24_DUAL:
    case VTSS_TARGET_E_STAX_III_68_DUAL:
        if (vtss_state->port_int_0 == 27) {
            /* XAUI_0 and XAUI_1 are 12G interconnects */
            taxi_p = taxi_ac3;
        } else if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            taxi_p = taxi_c1;
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
            taxi_p = taxi_ac5;
        } else {
            taxi_p = taxi_ac6;
        }
        break;
    case VTSS_TARGET_CE_MAX_12:
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            taxi_p = taxi_c4; /* 12x1G + 2x10G (or 12G)  */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
            taxi_p = taxi_c5; /* 12x1G + 4x2G5 + 1x10G (or 12G)  */
        } else {
            return VTSS_RC_ERROR;
        }
        break;
    default:
        VTSS_E("Assembler - Unknown Target");
        return VTSS_RC_ERROR;
    }
  
    /* Initilize  counters */
    JR_WR(ASM, CFG_STAT_CFG, VTSS_F_ASM_CFG_STAT_CFG_STAT_CNT_CLR_SHOT);

    /* Disable calender while setting it up */
    JR_WR(ASM, CFG_CBC_LEN_CFG, VTSS_F_ASM_CFG_CBC_LEN_CFG_CBC_LEN(0));
     
    /* Format the  the bus calender with Taxi's */
    for (i=0; i<cbc_len; i++) {
        JR_WRXF(ASM, CFG_CBC_CFG, i, DEV_GRP_NUM, taxi_p[i]);
    }

    /* Set Cell Bus Calendar length */
    JR_WR(ASM, CFG_CBC_LEN_CFG, VTSS_F_ASM_CFG_CBC_LEN_CFG_CBC_LEN(cbc_len));

    /* Wait until csc init is complete  */
    JR_POLL_BIT(ASM, CFG_STAT_CFG, STAT_CNT_CLR_SHOT);

    /* Disable Vstax2 awareness (enabled by default, TN0613) */
    for (i=0; i<32; i++) {
        JR_WRXB(ASM, CFG_ETH_CFG, i, ETH_VSTAX2_AWR_ENA, 0);
    }

    return VTSS_RC_OK;
}

static vtss_rc jr_init_ana(void)
{
    u32 value, i;    
    /* Initilize multiple RAMs in various blocks of the analyzer */

    /* ANA_CL RAM init */
    JR_WRB(ANA_CL_2, COMMON_COMMON_CTRL, TABLE_INIT_SHOT, 1);
    
    /* Initiatate MAC table clear */
    JR_WRB(ANA_L2, COMMON_TABLE_CLR_CFG, MAC_TBL_INIT_SHOT, 1);
    
    /* Initilizing ANA_L2 RAMs */
    JR_WRB(ANA_L2, COMMON_TABLE_CLR_CFG, ISDX_INIT_SHOT, 1);
    JR_WRB(ANA_L2, COMMON_TABLE_CLR_CFG, PATHGRP_INIT_SHOT, 1);

    /* Initilizing ANA_L3 RAMs */
    JR_WR(ANA_L3_2, COMMON_TABLE_CTRL, 0x1F);

    /* Clearing RAMs */
    JR_WRB(ANA_AC, PS_COMMON_PS_TABLE_CLR_CFG, PGID_TABLE_INIT_SHOT, 1);
    JR_WRB(ANA_AC, PS_COMMON_PS_TABLE_CLR_CFG, SRC_TABLE_INIT_SHOT, 1);

    /* Clear policers */
    JR_WRB(ANA_AC, POL_ALL_CFG_POL_ALL_CFG, PRIO_FORCE_INIT, 1);
    JR_WRB(ANA_AC, POL_ALL_CFG_POL_ALL_CFG, ACL_FORCE_INIT, 1);
    JR_WRB(ANA_AC, POL_ALL_CFG_POL_ALL_CFG, FORCE_INIT, 1);

    /* Enable overall signaling of flow control */
    JR_WRB(ANA_AC, POL_ALL_CFG_POL_ALL_CFG, PORT_FC_ENA, 1);

    /* Clearing Port Stat RAMs */
    JR_WRB(ANA_AC, STAT_GLOBAL_CFG_PORT_STAT_RESET, RESET, 1);

    /* Clearing Queue Stat RAMs ?? */

    /* Clearing ISDX Stat RAMs */
    JR_WRB(ANA_AC, STAT_GLOBAL_CFG_ISDX_STAT_RESET, RESET, 1);

#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    /* Enable VID counters */
    JR_WRB(ANA_AC, PS_COMMON_MISC_CTRL, USE_VID_AS_ISDX_ENA, 1);
#endif /* VTSS_FEATURE_VLAN_COUNTERS */

    /* Clearing Tunnel Stat RAMs */
    JR_WRB(ANA_AC, STAT_GLOBAL_CFG_TUNNEL_STAT_RESET, RESET, 1);

    /* Clearing Rleg Stat RAMs */
    JR_WRB(ANA_AC, STAT_GLOBAL_CFG_IRLEG_STAT_RESET, RESET, 1);
    JR_WRB(ANA_AC, STAT_GLOBAL_CFG_ERLEG_STAT_RESET, RESET, 1);

    /* Clearing service DLB RAMs */
    JR_WRB(ANA_AC, COMMON_SDLB_DLB_CTRL, DLB_INIT_SHOT, 1);
    
    /* Clearing tunnel DLB RAMs */
    JR_WRB(ANA_AC, COMMON_TDLB_DLB_CTRL, DLB_INIT_SHOT, 1);

    /* Clearing the sFlow cnt register */
    JR_WRM_SET(ANA_AC, PS_COMMON_SFLOW_RESET_CTRL, 0xFFFFFFFF);

    /* Initilizing LPM */
    JR_WRF(ANA_L3_2, LPM_ACCESS_CTRL, ACCESS_CMD, 6);
    JR_WRB(ANA_L3_2, LPM_ACCESS_CTRL, ACCESS_SHOT, 1);

    /* Enable usage of DBL_VID_PCP for lookup (default disabled, TN0613) */
    for (i=0; i<35; i++) {
        JR_RDX(ANA_CL_2, PORT_IS0_CFG_1, i,  &value);
        JR_WRX(ANA_CL_2, PORT_IS0_CFG_1, i, (value | VTSS_BIT(1)));
    }
    /* Must be set to 1 to avoid bad pointers from C-domain into B-domain (TN0613) */
    JR_RD(ANA_L2, COMMON_BLRN_CFG, &value);

    JR_WR(ANA_L2, COMMON_BLRN_CFG, (value | VTSS_BIT(13)));

    /* Enable CPU-copying of port-move detects of statically learned MAC addresses */
    JR_WRB(ANA_L2, COMMON_LRN_CFG, LOCKED_PORTMOVE_COPY_ENA, 1);

    /* Now, wait until initilization is complete */

    /* Await MAC table clear complete */
    JR_POLL_BIT(ANA_L2, COMMON_TABLE_CLR_CFG, MAC_TBL_INIT_SHOT);

    /* Await ANA_CL RAM init */
    JR_POLL_BIT(ANA_CL_2, COMMON_COMMON_CTRL, TABLE_INIT_SHOT);
    
    /* Await MAC table clear */
    JR_POLL_BIT(ANA_L2, COMMON_TABLE_CLR_CFG, MAC_TBL_INIT_SHOT);
    
    /* Await ANA_L2 RAMs */
    JR_POLL_BIT(ANA_L2, COMMON_TABLE_CLR_CFG, ISDX_INIT_SHOT);
    JR_POLL_BIT(ANA_L2, COMMON_TABLE_CLR_CFG, PATHGRP_INIT_SHOT);

    /* Await ANA_L3 RAMs */
    JR_POLL_BIT(ANA_L3_2, COMMON_TABLE_CTRL, INIT_IPMC_SHOT);
    JR_POLL_BIT(ANA_L3_2, COMMON_TABLE_CTRL, INIT_ARP_SHOT);
    JR_POLL_BIT(ANA_L3_2, COMMON_TABLE_CTRL, INIT_VLAN_SHOT);

    /* Await clearing RAMs */
    JR_POLL_BIT(ANA_AC, PS_COMMON_PS_TABLE_CLR_CFG, PGID_TABLE_INIT_SHOT);
    JR_POLL_BIT(ANA_AC, PS_COMMON_PS_TABLE_CLR_CFG, SRC_TABLE_INIT_SHOT);

    /* Await policer init */
    JR_POLL_BIT(ANA_AC, POL_ALL_CFG_POL_ALL_CFG, PRIO_FORCE_INIT);
    JR_POLL_BIT(ANA_AC, POL_ALL_CFG_POL_ALL_CFG, ACL_FORCE_INIT);
    JR_POLL_BIT(ANA_AC, POL_ALL_CFG_POL_ALL_CFG, FORCE_INIT);

    /* Await Clearing Port Stat RAMs */
    JR_POLL_BIT(ANA_AC, STAT_GLOBAL_CFG_PORT_STAT_RESET, RESET);

    /* Await Clearing Queue Stat RAMs ?? */

    /* Await Clearing ISDX Stat RAMs */
    JR_POLL_BIT(ANA_AC, STAT_GLOBAL_CFG_ISDX_STAT_RESET, RESET);

#if 0 //defined(VTSS_FEATURE_VLAN_COUNTERS)
    /* Await USE_VID_AS_ISDX_ENA bit */
    JR_POLL_BIT(ANA_AC, PS_COMMON_MISC_CTRL, USE_VID_AS_ISDX_ENA);
#endif

    /* Await Clearing Tunnel Stat RAMs */
    JR_POLL_BIT(ANA_AC, STAT_GLOBAL_CFG_TUNNEL_STAT_RESET, RESET);

    /* Await Clearing Rleg Stat RAMs */
    JR_POLL_BIT(ANA_AC, STAT_GLOBAL_CFG_IRLEG_STAT_RESET, RESET);
    JR_POLL_BIT(ANA_AC, STAT_GLOBAL_CFG_ERLEG_STAT_RESET, RESET);

    /* Await Clearing service DLB RAMs */
    JR_POLL_BIT(ANA_AC, COMMON_SDLB_DLB_CTRL, DLB_INIT_SHOT);
    
    /* Await Clearing tunnel DLB RAMs */
    JR_POLL_BIT(ANA_AC, COMMON_TDLB_DLB_CTRL, DLB_INIT_SHOT);

    /* Await LPM clear */
    JR_POLL_BIT(ANA_L3_2, LPM_ACCESS_CTRL, ACCESS_SHOT);

    /* Resetting the sFlow cnt reg */
    JR_WRM_SET(ANA_AC, PS_COMMON_SFLOW_RESET_CTRL, 0x0); 

    return VTSS_RC_OK; 
}

static vtss_rc jr_init_vcap_is0(void)
{
    JR_WR(VCAP_IS0, TCAM_BIST_TCAM_CTRL, VTSS_F_VCAP_IS0_TCAM_BIST_TCAM_CTRL_TCAM_INIT);
    JR_POLL_BIT(VCAP_IS0, TCAM_BIST_TCAM_CTRL, TCAM_INIT);

    VTSS_RC(jr_vcap_command(VTSS_TCAM_IS0, 0, VTSS_TCAM_CMD_INITIALIZE));
    JR_WR(VCAP_IS0, IS0_CONTROL_ACL_CFG, VTSS_F_VCAP_IS0_IS0_CONTROL_ACL_CFG_ACL_ENA);

    return VTSS_RC_OK;
}

static vtss_rc jr_init_vcap_is1(void)
{
    JR_WR(VCAP_IS1, TCAM_BIST_TCAM_CTRL, VTSS_F_VCAP_IS1_TCAM_BIST_TCAM_CTRL_TCAM_INIT);
    JR_POLL_BIT(VCAP_IS1, TCAM_BIST_TCAM_CTRL, TCAM_INIT);

    VTSS_RC(jr_vcap_command(VTSS_TCAM_IS1, 0, VTSS_TCAM_CMD_INITIALIZE));
    JR_WR(VCAP_IS1, IS1_CONTROL_ACL_CFG, VTSS_F_VCAP_IS1_IS1_CONTROL_ACL_CFG_ACL_ENA);

    return VTSS_RC_OK;
}

static vtss_rc jr_init_vcap_is2(void)
{
    JR_WR(VCAP_IS2, TCAM_BIST_TCAM_CTRL, VTSS_F_VCAP_IS2_TCAM_BIST_TCAM_CTRL_TCAM_INIT);
    JR_POLL_BIT(VCAP_IS2, TCAM_BIST_TCAM_CTRL, TCAM_INIT);

    VTSS_RC(jr_vcap_command(VTSS_TCAM_IS2, 0, VTSS_TCAM_CMD_INITIALIZE));
    JR_WR(VCAP_IS2, IS2_CONTROL_ACL_CFG, VTSS_F_VCAP_IS2_IS2_CONTROL_ACL_CFG_ACL_ENA);

    return VTSS_RC_OK;
}

static vtss_rc jr_init_vcap_es0(void)
{
    JR_WR(VCAP_ES0, TCAM_BIST_TCAM_CTRL, VTSS_F_VCAP_ES0_TCAM_BIST_TCAM_CTRL_TCAM_INIT);
    JR_POLL_BIT(VCAP_ES0, TCAM_BIST_TCAM_CTRL, TCAM_INIT);

    VTSS_RC(jr_vcap_command(VTSS_TCAM_ES0, 0, VTSS_TCAM_CMD_INITIALIZE));
    JR_WR(VCAP_ES0, ES0_CONTROL_ACL_CFG, VTSS_F_VCAP_ES0_ES0_CONTROL_ACL_CFG_ACL_ENA);

    return VTSS_RC_OK;
}

#define JR_SQS_CNT_SET(tgt, xmit, cnt_id, qu, frm_type, event_mask)                       \
{                                                                                         \
    JR_WRXY(tgt, STAT_CNT_CFG_##xmit##_STAT_CFG, qu, JR_CNT(xmit, cnt_id),                 \
            JR_PUT_FLD(tgt, STAT_CNT_CFG_##xmit##_STAT_CFG, CFG_CNT_FRM_TYPE, frm_type)); \
    JR_WRX(tgt, STAT_GLOBAL_CFG_##xmit##_STAT_GLOBAL_EVENT_MASK, JR_CNT(xmit, cnt_id),     \
           JR_PUT_FLD(tgt, STAT_GLOBAL_CFG_##xmit##_STAT_GLOBAL_EVENT_MASK,               \
                      GLOBAL_EVENT_MASK, event_mask));                  \
}                                                                                         


static vtss_rc jr_init_sqs(void)
{
    u32 qu, p;
    sqs_t   sqs;
    vtss_target_type_t target = jr_target_get();

    /* Start initilization */
    JR_WRB(OQS, MAIN_MAIN_REINIT_1SHOT, MAIN_REINIT, 1);
    JR_WRB(IQS, MAIN_MAIN_REINIT_1SHOT, MAIN_REINIT, 1);

    JR_WRB(IQS, STAT_GLOBAL_CFG_ISDX_STAT_RESET, RESET, 1);
    JR_WRB(OQS, STAT_GLOBAL_CFG_ISDX_STAT_RESET, RESET, 1);

    JR_WRB(IQS, STAT_GLOBAL_CFG_RX_STAT_RESET, RESET, 1);
    JR_WRB(OQS, STAT_GLOBAL_CFG_RX_STAT_RESET, RESET, 1);

    JR_WRB(IQS, STAT_GLOBAL_CFG_TX_STAT_RESET, RESET, 1);
    JR_WRB(OQS, STAT_GLOBAL_CFG_TX_STAT_RESET, RESET, 1);

    /* Wait until completed */
    JR_POLL_BIT(OQS, MAIN_MAIN_REINIT_1SHOT, MAIN_REINIT);
    JR_POLL_BIT(IQS, MAIN_MAIN_REINIT_1SHOT, MAIN_REINIT);

    JR_POLL_BIT(IQS, STAT_GLOBAL_CFG_ISDX_STAT_RESET, RESET);
    JR_POLL_BIT(OQS, STAT_GLOBAL_CFG_ISDX_STAT_RESET, RESET);

    JR_POLL_BIT(IQS, STAT_GLOBAL_CFG_RX_STAT_RESET, RESET);
    JR_POLL_BIT(OQS, STAT_GLOBAL_CFG_RX_STAT_RESET, RESET);

    JR_POLL_BIT(IQS, STAT_GLOBAL_CFG_TX_STAT_RESET, RESET);
    JR_POLL_BIT(OQS, STAT_GLOBAL_CFG_TX_STAT_RESET, RESET);

    /* Setup SQS counters. */
    /* Counters can be extracted from IQS and OQS for RX (4 sets) and TX (2 sets)  */

    /*  ##### IQS RX counters #####  */
    for (qu = 0; qu < 280; qu++) { 
        /* 4 sets of RX counters */
        JR_SQS_CNT_SET(IQS, RX, GREEN, qu, 1, 1<<8);    /* Green */
        JR_SQS_CNT_SET(IQS, RX, YELLOW , qu, 1, 1<<9);  /* Yellow */
        JR_SQS_CNT_SET(IQS, RX, QDROPS, qu, 3, 0x3E);   /* SQS and RED drops */
        JR_SQS_CNT_SET(IQS, RX, PDROPS, qu, 2, 0x7C00); /* Policer drops */

        /* 2 sets of TX counters (using 1) */
        JR_SQS_CNT_SET(IQS, TX, DROPS, qu, 1, 0x8);     /* Full OQS Tx drops */
    }
    
    /*  ##### OQS counters #####  */
    for (qu = 0; qu < 464; qu++) { 
        /* 4 sets of RX counters (using 3) */
        JR_SQS_CNT_SET(OQS, RX, GREEN, qu, 1, 1<<8);    /* Green */
        JR_SQS_CNT_SET(OQS, RX, YELLOW , qu, 1, 1<<9);  /* Yellow */
        JR_SQS_CNT_SET(OQS, RX, QDROPS, qu, 1, 0x3E);   /* SQS drops */

        /* 2 sets of TX counters (using 2) */
        JR_SQS_CNT_SET(OQS, TX, GREEN, qu, 1, 1<<4);  /* Green */
        JR_SQS_CNT_SET(OQS, TX, YELLOW, qu, 1, 1<<5); /* Yellow */
    }

    /* IQS and OQS reserved fields that must be set to 0 during initialization */
    JR_WR(IQS, RESERVED_RESERVED, 0)
    JR_WR(OQS, RESERVED_RESERVED, 0);

    /* Enterprise must react on congestion for all DP levels */
    if (target == VTSS_TARGET_E_STAX_III_48 || 
        target == VTSS_TARGET_E_STAX_III_68) {
        JR_WRF(IQS, CONG_CTRL_CH_CFG, DE_ENA, 0xF); 
        JR_WRF(ARB, CFG_STATUS_CH_CFG, DE_ENA, 0xF);
        JR_WRF(OQS, CONG_CTRL_CH_CFG, DE_ENA, 0xF); 
    } else {
        JR_WRF(IQS, CONG_CTRL_CH_CFG, DE_ENA, 0xE); 
        JR_WRF(ARB, CFG_STATUS_CH_CFG, DE_ENA, 0xE);
        JR_WRF(OQS, CONG_CTRL_CH_CFG, DE_ENA, 0xE); 
    }
    
    /* Initilize SQS WMs */
    for(p = 0; p < 89; p++) {
        /*                   chipport, spd, jumbo, fc, power, init   */ 
        VTSS_RC(jr_sqs_calc_wm(p, VTSS_SPEED_1G, 0, 0, 0, 1, &sqs));
        VTSS_RC(jr_sqs_set(p, &sqs));
    }
    
    return VTSS_RC_OK;
}

static vtss_rc jr_init_arb(void)
{
    u8 i;
    
    /* ARB: disable arbitration randomness */
    JR_WR(ARB, CFG_STATUS_RND_GEN_CFG, 1);

    /* ARB: disable force drop mode except for the CPU */
    for(i = 0; i < 8; i++) {
        JR_WRX(ARB, DROP_MODE_CFG_OUTB_ETH_DROP_MODE_CFG, i, 0x00);
        JR_WRX(ARB, DROP_MODE_CFG_OUTB_CPU_DROP_MODE_CFG, i, 0xff); /* Bugzilla#7548 */
        JR_WRX(ARB, DROP_MODE_CFG_OUTB_VD_DROP_MODE_CFG,  i, 0x00);
    }
    
    /* ARB: enable QS spill over watermarks (DBG-reg ARB::SWM_CFG) */
    VTSS_RC(jr_wr(VTSS_IOREG(VTSS_TO_ARB, 0x53), 1));

    return VTSS_RC_OK;
}

/* Initilization of the scheduler. */
/* TX cell bus calender is configured based on the Mux mode */
static vtss_rc jr_init_sch(void)
{
    /* Jaguar MUX:0, 10G:27-28, 12G:29-30 */
    u8 cal_s1[] = {27,29,28,0,30,6,29,27,12,28,30,18,29,1,27,30,28,7,29,13,19,27,30,28,
                   29,2,8,30,27,29,28,14,20,30,3,63,27,29,28,9,30,15,29,27,21,28,30,4,
                   29,10,27,30,28,16,29,22,5,27,30,28,29,11,17,30,27,29,28,23,33,30,31,63}; 

    /* CEMAX-24 MUX:0 HMD:30 */
    u8 cal_m1[] = {34,28,29,27,63,33,31,34,28,29,27,23,17,22,34,28,29,27,21,16,20,34,28,29,
                   27,19,15,18,34,28,29,27,14,11,13,10,34,28,29,27,12,9,5,34,28,29,27,8,
                   4,7,34,28,29,27,6,3,63,34,28,29,27,2,63,1,34,28,29,27,0,63,63,63};

    /* CEMAX-24 MUX:0 HMD:29,30 */
    u8 cal_m2[] = {35,28,34,27,63,33,31,35,28,34,27,23,17,22,35,28,34,27,21,16,20,35,28,34,
                   27,19,15,18,35,28,34,27,14,11,13,10,35,28,34,27,12,9,5,35,28,34,27,8,
                   4,7,35,28,34,27,6,3,63,35,28,34,27,2,63,1,35,28,34,27,0,63,63,63};

    /* CEMAX-12 MUX:0 HMD:28 */
    u8 cal_m13[] = {34,63,27,33,31,17,22,34,21,27,20,16,19,15,34,18,27,14,11,13,63,34,12,27,
                    63,63,63,63,34,63,27,63,63,63,63,63,34,63,27,63,63,63,63,34,63,27,63,63,
                    63,63,34,63,27,63,63,63,63,34,63,27,63,63,63,63,34,63,27,63,63,63,63,63};

    u8 cal_ac3[] = {28,29,27,0,30,6,28,29,27,12,18,30,28,29,27,1,7,30,28,29,27,13,19,2,
                    28,29,27,30,8,14,28,29,27,30,20,3,28,29,27,9,30,15,28,29,27,21,4,30,
                    28,29,27,10,16,30,28,29,27,22,5,11,28,29,27,30,17,23,28,29,27,30,33,31,
                    28,29,27,0,30,6,28,29,27,12,18,30,28,29,27,1,7,30,28,29,27,13,19,2,
                    28,29,27,30,8,14,8,29,27,30,20,3,28,29,27,9,30,15,28,29,27,21,4,30,
                    28,29,27,10,16,30,28,29,27,22,5,11,28,29,27,30,17,23,28,29,27,30,63,31};       

    u8 cal_ac5[] = {29,28,23,30,32,24,29,25,28,30,26,0,29,18,6,30,28,19,29,12,1,30,28,7,
                    29,13,2,30,8,28,29,14,23,30,24,3,29,28,25,30,26,32,29,9,28,30,15,4,
                    29,10,16,30,28,5,29,11,31,30,28,22,29,23,17,30,24,28,29,25,33,30,26,63,
                    29,28,20,30,21,32,29,0,6,30,28,19,29,18,12,30,23,28,29,24,1,30,25,7,
                    29,28,26,30,13,2,29,28,8,30,14,3,29,9,15,30,28,32,29,4,10,30,28,23,
                    29,24,16,30,25,28,29,26,5,30,31,11,29,28,17,30,33,22,29,28,21,30,63,20};

    u8 cal_ac6[] = {29,32,23,30,0,24,29,25,6,30,26,12,29,19,32,30,20,1,29,21,7,30,22,13,
                    29,31,32,30,2,8,29,14,23,30,24,3,29,32,25,30,26,9,29,19,15,30,20,4,
                    29,21,32,30,22,10,29,16,5,30,11,18,29,23,32,30,24,17,29,25,63,30,26,33,
                    29,19,32,30,20,0,29,21,6,30,22,12,29,32,1,30,23,7,29,24,13,30,25,2,
                    29,32,26,30,19,8,29,20,14,30,21,3,29,22,32,30,31,9,29,15,4,30,10,23,
                    29,24,32,30,25,16,29,26,5,30,19,11,29,20,32,30,21,17,29,22,63,30,18,33};

    u8 *cal_p;
    u32 cbc_len = 144, i, len, port, port_unused;
    u32 hm = ce_max_hm();
    vtss_target_type_t target = vtss_state->create.target;   
    
    /* Determine the correct cell bus calender configuration (based on target and mux mode, i.e. port speeds) */
    switch (vtss_state->mux_mode[vtss_state->chip_no]) {
    case VTSS_PORT_MUX_MODE_0:
        if (target == VTSS_TARGET_CE_MAX_24 || target == VTSS_TARGET_CE_MAX_12) {
            cbc_len = 72;
            cal_p = (hm == 0 || hm == 1 ? cal_m2 :
                     target == VTSS_TARGET_CE_MAX_24 ? cal_m1 : cal_m13);
        } else if (target != jr_target_get() && (vtss_state->port_int_0 == 27)) {
            /* Dual Jaguar target using XAUI_0 and XAUI_1 interconnects */
            cal_p = cal_ac3; /* Jaguar1 10G:29-30 12G:27,28 */
        } else {
            cbc_len = 72;
            cal_p = cal_s1; /* Jaguar1 10G:27-28, 12G:29-30 */
        }
        break;
    case VTSS_PORT_MUX_MODE_1:
        cal_p = cal_ac5;
        break;
    default:
        cal_p = cal_ac6;
        break;
    }

    /* Init leaky bucket */
    JR_WRB(SCH, ETH_ETH_LB_CTRL, LB_INIT, 1);
    JR_WRB(SCH, HM_HM_SCH_CTRL, LB_INIT, 1);
    /* Init MCTX RAM */
    JR_WR(SCH, QSIF_QSIF_CTRL, (1<<2)); 

    /* Disable calender while setting it up */
    JR_WR(SCH, CBC_CBC_LEN_CFG, 0); 

    /* Setup default host mode devices to unused port */
    port_unused = jr_unused_chip_port();
    JR_WRF(SCH, QSIF_QSIF_HM_CTRL, ETH_DEV_HMDA, port_unused);
    JR_WRF(SCH, QSIF_QSIF_HM_CTRL, ETH_DEV_HMDB, port_unused);

    /* The calendar is repeated 4 times */
    len = (cbc_len * 4);
    for (i = 0; i < len; i++) {
        port = cal_p[i % cbc_len];
        if (port == 63 && port_unused != VTSS_PORT_NO_NONE) {
            /* If the calendar is replicated, the first occurrence of the IDLE port
               is replaced by the unused port for EEE */
            port = port_unused;
            port_unused = VTSS_PORT_NO_NONE;
        }
        JR_WRX(SCH, CBC_DEV_ID_CFG_CBC_DEV_ID, i, port);
    }
    
    JR_POLL_BIT(SCH, ETH_ETH_LB_CTRL, LB_INIT);

    /* Enable calender  */
    JR_WR(SCH, CBC_CBC_LEN_CFG, len);
    
    return VTSS_RC_OK;
}

static vtss_rc jr_init_rew(void)
{
    /* Start Ram init */
    JR_WRB(REW, COMMON_COMMON_CTRL, FORCE_PORT_CFG_INIT, 1); 
    JR_WR(REW, STAT_GLOBAL_CFG_ESDX_STAT_RESET, 1); 

    JR_POLL_BIT(REW, COMMON_COMMON_CTRL, FORCE_PORT_CFG_INIT);
    JR_POLL_BIT(REW, STAT_GLOBAL_CFG_ESDX_STAT_RESET, RESET);

    /* Count bytes */
    JR_WRXB(REW, STAT_GLOBAL_CFG_ESDX_STAT_GLOBAL_CFG, 0, GLOBAL_CFG_CNT_BYTE, 1); 

    /* Better default (TN0613) */
    /* REW:COMMON:VSTAX_PORT_GRP_CFG[0-1].VSTAX_LRN_ALL_HP_ENA */
    JR_WRXB(REW, COMMON_VSTAX_PORT_GRP_CFG, 0, VSTAX_LRN_ALL_HP_ENA, 1);
    JR_WRXB(REW, COMMON_VSTAX_PORT_GRP_CFG, 1, VSTAX_LRN_ALL_HP_ENA, 1);
    
    /* REW:COMMON:VSTAX_PORT_GRP_CFG[0-1].VSTAX_MODE */
    JR_WRXB(REW, COMMON_VSTAX_PORT_GRP_CFG, 0, VSTAX_MODE, 1);
    JR_WRXB(REW, COMMON_VSTAX_PORT_GRP_CFG, 1, VSTAX_MODE, 1);

    return VTSS_RC_OK;
}

static vtss_rc jr_init_dsm(void)
{    
    // MAC mode LPORT number mapping init TBD      

    /* Fixed water marks for DEV1G and DEVNPI */
    JR_WR(DSM, CFG_RATE_CTRL_WM,
          JR_PUT_FLD(DSM, CFG_RATE_CTRL_WM, TAXI_128_RATE_CTRL_WM_DEVNPI, 20) |
          JR_PUT_FLD(DSM, CFG_RATE_CTRL_WM, TAXI_32_RATE_CTRL_WM, 36));

    return VTSS_RC_OK;
}

/* Enable first IS2 lookup and setup default actions for CPU ports */
static vtss_rc jr_is2_cpu_port_setup(void)
{
    u32               port, cnt;
    vtss_acl_action_t action;

    /* Initialize IS2 action */
    memset(&action, 0, sizeof(action));
    action.learn = 1;
    action.forward = 1;

    for (port = VTSS_CHIP_PORT_CPU_0; port <= VTSS_CHIP_PORT_CPU_1; port++) {
        VTSS_RC(jr_vcap_is1_is2_set(port, 0, IS2_LOOKUP_FIRST));
        VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS2, port, VTSS_TCAM_CMD_READ));
        JR_RD(VCAP_IS2, BASETYPE_ACTION_D, &cnt);
        VTSS_RC(jr_is2_prepare_action(&action, JR_IS2_REDIR_PGID_ASM, cnt));
        VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS2, port, VTSS_TCAM_CMD_WRITE));
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_init_packet(void)
{
    u32 i;
    int pcp, dei;
    
    /* Default extraction mode is little endian and status word before last data. Include queue number before IFH */
    for (i = 0; i < JR_PACKET_RX_GRP_CNT; i++) {
        JR_WRX(DEVCPU_QS, XTR_XTR_GRP_CFG, i, 
               JR_PUT_BIT(DEVCPU_QS, XTR_XTR_GRP_CFG, BYTE_SWAP, 1) |
               JR_PUT_BIT(DEVCPU_QS, XTR_XTR_GRP_CFG, STATUS_WORD_POS, 0) |
               JR_PUT_BIT(DEVCPU_QS, XTR_XTR_GRP_CFG, XTR_HDR_ENA, 1));
        /* By default CPU Rx queues are extracted in a round-robin fashion.
         * We need to change that to strict priority, so that higher queue
         * numbers are extracted prior to lower queue numbers.
         */
        JR_WRX(DEVCPU_QS, XTR_XTR_QU_SEL, i, VTSS_F_DEVCPU_QS_XTR_XTR_QU_SEL_QU_SCH(1));
    }

    /* Make the chip store the CPU Rx queue mask (i.e. the reasons for forwarding the frame to the CPU) in the extraction IFH */
    JR_WRM_SET(ANA_AC, PS_COMMON_MISC_CTRL, VTSS_F_ANA_AC_PS_COMMON_MISC_CTRL_CPU_DEST_MASK_IN_CL_RES_ENA);

    /* Enable ability to perform super priority injection on front ports (supported by FDMA driver)
     * Disobey flow control, since that will cause the whole system to stop working if it happens. */
    JR_WR(DSM, SP_CFG_SP_FRONT_PORT_INJ_CFG,
          JR_PUT_BIT(DSM, SP_CFG_SP_FRONT_PORT_INJ_CFG, SP_FRONT_PORT_REGARD_LLFC, 0) |
          JR_PUT_BIT(DSM, SP_CFG_SP_FRONT_PORT_INJ_CFG, SP_FRONT_PORT_TX_ENA, 1));

    /* The super priority queue watermarks must be set to CPU MTU + various header sizes +
     * latency from flow control assertion until FDMA reaction (register watermark
     * granularity is 16 bytes). The maximum is 99.
     */
    JR_WR(DSM, SP_CFG_SP_INJ_CFG, 95);

    /* Disable CPU frame aging in ASM */
    for (i = 0; i < 2; i++) {
        JR_WRXB(ASM, CFG_CPU_CFG, i, CPU_FRM_AGING_DIS, 1);
    }

    for (i = VTSS_CHIP_PORT_CPU_0; i <= VTSS_CHIP_PORT_CPU_1; i++) {
        /* Enable stacking on CPU ports for VLAN classification purposes */
        JR_WRXB(ANA_CL_2, PORT_STACKING_CTRL, i, STACKING_AWARE_ENA, 1);

        /* VLAN popping is done using default actions in IS0 */
        VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS0, i, VTSS_TCAM_CMD_READ));
        JR_WRF(VCAP_IS0, BASETYPE_ACTION_A, VLAN_POP_CNT, 1);
        VTSS_RC(jr_vcap_port_command(VTSS_TCAM_IS0, i, VTSS_TCAM_CMD_WRITE));

        /* Set CPU ports to be VLAN aware, since frames that we send switched
         * must contain a VLAN tag for correct classification. One could use
         * the frame's VStaX header, but that won't work for stacking solutions */
        JR_WRXM(ANA_CL_2, PORT_VLAN_CTRL, i,
                VTSS_F_ANA_CL_2_PORT_VLAN_CTRL_PORT_VID(0) |
                VTSS_F_ANA_CL_2_PORT_VLAN_CTRL_VLAN_AWARE_ENA,
                VTSS_M_ANA_CL_2_PORT_VLAN_CTRL_PORT_VID |
                VTSS_F_ANA_CL_2_PORT_VLAN_CTRL_VLAN_AWARE_ENA);

        /* In order for switched frames to be classified to a user-module-specified
         * QoS class, we use the VLAN tag's PCP and DEI bits to get it classified.
         * For this to work, the CPU port(s) must be enabled for PCP/DEI-to-QoS
         * class classification. Below, we set-up a one-to-one-mapping between
         * PCP value and QoS class independent of value of DEI.
         */
        JR_WRXB(ANA_CL_2, PORT_QOS_CFG, i, UPRIO_QOS_ENA,      1);
        JR_WRXB(ANA_CL_2, PORT_QOS_CFG, i, UPRIO_DP_ENA,       1);
        for (pcp = 0; pcp < VTSS_PCP_END - VTSS_PCP_START; pcp++) {
            for (dei = 0; dei < VTSS_DEI_END - VTSS_DEI_START; dei++) {
                JR_WRXYF(ANA_CL_2, PORT_UPRIO_MAP_CFG, i, (8 * dei + pcp), UPRIO_CFI_TRANSLATE_QOS_VAL, vtss_cmn_pcp2qos(pcp));
                JR_WRXYF(ANA_CL_2, PORT_DP_CONFIG,     i, (8 * dei + pcp), UPRIO_CFI_DP_VAL,            0);
            }
        }
    }

    VTSS_RC(jr_is2_cpu_port_setup());

    return VTSS_RC_OK;
}

#define JR_SDX_CNT_SET(tgt, sdx, idx, frames, mask)                        \
{                                                                          \
    JR_WRX(tgt, STAT_GLOBAL_CFG_##sdx##_STAT_GLOBAL_CFG, idx,              \
           JR_PUT_BIT(tgt, STAT_GLOBAL_CFG_##sdx##_STAT_GLOBAL_CFG,        \
                      GLOBAL_CFG_CNT_BYTE, (frames) ? 0 : 1));          \
    JR_WRX(tgt, STAT_GLOBAL_CFG_##sdx##_STAT_GLOBAL_EVENT_MASK, idx,       \
           JR_PUT_FLD(tgt, STAT_GLOBAL_CFG_##sdx##_STAT_GLOBAL_EVENT_MASK, \
                      GLOBAL_EVENT_MASK, mask));                           \
}                                                                                         

static vtss_rc jr_port_policer_fc_set(const vtss_port_no_t port_no, u32 chipport)
{
    u32 i, fc_ena = 0;
    vtss_port_conf_t *port_conf = &vtss_state->port_conf[port_no];
    vtss_qos_port_conf_t *qos_conf = &vtss_state->qos_port_conf[port_no];

    if (port_conf->flow_control.generate) {
        for (i = 0; i < 4; i++) {
            if ((qos_conf->policer_port[i].rate != VTSS_BITRATE_DISABLED) && qos_conf->policer_ext_port[i].flow_control) {
                fc_ena |= 1 << i;
            }
        }
    }
    JR_WRX(ANA_AC, POL_ALL_CFG_POL_PORT_FC_CFG, chipport, fc_ena);
    JR_WRXB(DSM, CFG_ETH_FC_GEN, chipport, ETH_POLICED_PORT_FC_GEN, fc_ena ? 1 : 0); 

    return VTSS_RC_OK;
}

static vtss_rc jr_port_policer_set(u32 port, u32 idx, vtss_policer_t *conf, vtss_policer_ext_t *conf_ext)
{
    u32 pol_idx = (port * 4 + idx);
    u32 mask = 0, q, type = 0;
    
    /* Burst size and rate */
    JR_WRX(ANA_AC, POL_PORT_CFG_POL_PORT_THRES_CFG_0, pol_idx, jr_calc_policer_level(conf->level, conf->rate, conf_ext->frame_rate));
    JR_WRX(ANA_AC, POL_PORT_CFG_POL_PORT_THRES_CFG_1, pol_idx, 1); /* 2048 bytes flow control deassert threshold */
    JR_WRX(ANA_AC, POL_PORT_CFG_POL_PORT_RATE_CFG, pol_idx, jr_calc_policer_rate(conf->rate, conf_ext->frame_rate));

    /* GAP */
    JR_WRX(ANA_AC, POL_PORT_CTRL_POL_PORT_GAP, port, 0);

    /* CPU queue mask */
    for (q = 0; q < 8; q++) {
        if (conf_ext->cpu_queue[q])
            mask |= VTSS_BIT(q);
    }
    
    /* Traffic type mask */
    if (conf->rate != VTSS_BITRATE_DISABLED) {
        if (conf_ext->multicast)
            type |= VTSS_BIT(0);
        if (conf_ext->broadcast)
            type |= VTSS_BIT(1);
        if (conf_ext->unicast)
            type |= VTSS_BIT(2);
        if (conf_ext->flooded)
            type |= VTSS_BIT(3);
        if (conf_ext->to_cpu)
            type |= VTSS_BIT(4);
        if (conf_ext->learning)
            type |= VTSS_BIT(5);
    }
    
    JR_WRXY(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, port, idx,
            JR_PUT_FLD(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, CPU_QU_MASK, mask) |
            JR_PUT_FLD(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, 
                       DP_BYPASS_LVL, conf_ext->dp_bypass_level) |
            JR_PUT_BIT(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, FRAME_RATE_ENA, conf_ext->frame_rate) |
            JR_PUT_BIT(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, 
                       LIMIT_NONCPU_TRAFFIC_ENA, conf_ext->limit_noncpu_traffic) | 
            JR_PUT_BIT(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, 
                       LIMIT_CPU_TRAFFIC_ENA, conf_ext->limit_cpu_traffic) | 
            JR_PUT_BIT(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, 
                       MC_WITHOUT_FLOOD_ENA, conf_ext->mc_no_flood) |
            JR_PUT_BIT(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, 
                       UC_WITHOUT_FLOOD_ENA, conf_ext->uc_no_flood) |
            JR_PUT_FLD(ANA_AC, POL_PORT_CTRL_POL_PORT_CFG, TRAFFIC_TYPE_MASK, type));
    
    return VTSS_RC_OK;
}

static vtss_rc jr_queue_policer_set(u32 port, u32 queue, vtss_policer_t *conf)
{
    u32 pol_idx = (port * 8 + queue);

    JR_WRX(ANA_AC, POL_PRIO_CFG_POL_PRIO_RATE_CFG, pol_idx, jr_calc_policer_rate(conf->rate, FALSE));
    JR_WRX(ANA_AC, POL_PRIO_CFG_POL_PRIO_THRES_CFG, pol_idx, jr_calc_policer_level(conf->level, conf->rate, FALSE));
    JR_WRX(ANA_AC, POL_PRIO_CTRL_POL_PRIO_GAP, pol_idx, 0);
    JR_WRX(ANA_AC, POL_PRIO_CTRL_POL_PRIO_CFG, pol_idx, 0);

    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_VSTAX_V2)
static vtss_rc jr_init_ports_int(void)
{
    vtss_chip_no_t         chip_no;
    u32                    port;
    vtss_port_conf_t       conf;
    vtss_vstax_info_t      *vstax_info;
    vtss_vstax_chip_info_t *chip_info;

#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
    /* The FDMA needs a function pointer to a function that can be used to manually read the secondary chip. */
    vtss_state->fdma_state.rx_frame_get_internal     = jr_rx_frame_get_internal;
    /* Also needs a function pointer to a function that can be used to discard frames on the secondary chip. */
    vtss_state->fdma_state.rx_frame_discard_internal = jr_rx_frame_discard_internal;
#endif

    /* Only setup internal ports if dual device target */
    if (vtss_state->chip_count != 2)
        return VTSS_RC_OK;
    
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        VTSS_SELECT_CHIP(chip_no);
        /* Default VStaX info: UPSID 0/1 and internal ports are STACK_B */
        vstax_info = &vtss_state->vstax_info;
        vstax_info->upsid[chip_no] = chip_no;

        if (chip_no == 0)
            vtss_state->vstax_conf.upsid_0 = chip_no;
        else
            vtss_state->vstax_conf.upsid_1 = chip_no;
        chip_info = &vstax_info->chip_info[chip_no];
        chip_info->mask_b = vtss_state->mask_int_ports;
        chip_info->rt_table.topology_type = VTSS_VSTAX_TOPOLOGY_CHAIN;
        chip_info->rt_table.table[chip_no == 0 ? 1 : 0].stack_port_b = 1;
        chip_info->port_conf[1].ttl = 1;

        /* We are using short preamble and shrinked IPG */
        JR_WRF(DSM, CFG_IPG_SHRINK_CFG, IPG_PREAM_SHRINK_ENA, (1<<(vtss_state->port_int_0-27)) | (1<<(vtss_state->port_int_1-27)));
        JR_WRF(DSM, CFG_IPG_SHRINK_CFG, IPG_SHRINK_ENA, (1<<(vtss_state->port_int_0-27)) | (1<<(vtss_state->port_int_1-27)));
        
        for (port = vtss_state->port_int_0; port <= vtss_state->port_int_1; port++) {
            /* Port configuration */
            memset(&conf, 0, sizeof(conf));
            conf.if_type = VTSS_PORT_INTERFACE_XAUI;
            conf.speed = VTSS_SPEED_12G;
            conf.fdx = 1;
            conf.max_frame_length = VTSS_MAX_FRAME_LENGTH_MAX;
            VTSS_RC(jr_port_conf_10g_set(VTSS_PORT_NO_NONE, port, &conf, 0));
            
            /* Shrink preamble to 1 byte */
            JR_WRXF(DEV10G, MAC_CFG_STATUS_MAC_MODE_CFG, VTSS_TO_DEV(port), MAC_PREAMBLE_CFG, 1);

            /* Shrink the IPG to 4-7 bytes */
            JR_WRXB(DEV10G, MAC_CFG_STATUS_MAC_MODE_CFG, VTSS_TO_DEV(port), MAC_IPG_CFG, 1);
           
            /* Source masks: Exclude internal ports */
            VTSS_RC(jr_src_table_write_chip(port, 0xffffffff - vtss_state->mask_int_ports));
            
            /* Setup logical port numbers */
            JR_WRX(DSM, CM_CFG_LPORT_NUM_CFG, port, port);
            JR_WRX(ANA_CL_2, PORT_PORT_ID_CFG, port,
                   JR_PUT_FLD(ANA_CL_2, PORT_PORT_ID_CFG, LPORT_NUM, port));
            
            /* Destination masks: Include internal ports */
            VTSS_RC(jr_pgid_table_write_chip(port, vtss_state->mask_int_ports, 0, 0, 0));

            /* VLAN port configuration */
            VTSS_RC(jr_vlan_port_conf_apply(port, &vtss_state->vlan_port_conf[VTSS_PORT_NO_START], NULL));
            
            /* Enable first IS2 lookup on internal ports for SSM */
            VTSS_RC(jr_vcap_is1_is2_set(port, 0, IS2_LOOKUP_FIRST));
        }

        /* Use policers to avoid CPU copy */
        VTSS_RC(jr_cpu_pol_setup_chip());

        /* Update VStaX configuration */
        VTSS_RC(jr_vstax_conf_set_chip());
    }
    
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_VSTAX_V2 */

static vtss_rc jr_update_sqs(void)
{
    vtss_port_no_t port_no;
    vtss_target_type_t target = jr_target_get();
    u32 chipport, startport=0;
    sqs_t sqs;
    vtss_port_speed_t speed;
    vtss_port_mux_mode_t mux_mode = vtss_state->mux_mode[vtss_state->chip_no];

    memset(&sqs,0,sizeof(sqs));
    /* The WM is only setup once for these targets */
    if (target == VTSS_TARGET_CE_MAX_12 || target == VTSS_TARGET_CE_MAX_24) {
        /* if (target == VTSS_TARGET_CE_MAX_12) */
        /*     startport = 12; */
        for (chipport = startport; chipport < VTSS_CHIP_PORTS; chipport++) {
            if (mux_mode == VTSS_PORT_MUX_MODE_0) {
                if (chipport > 23 && chipport < 27) 
                    continue;
            } else if (mux_mode == VTSS_PORT_MUX_MODE_1) {
                if ((chipport > 19 && chipport < 23) || (chipport == 27)) 
                    continue;                
            } else if (mux_mode == VTSS_PORT_MUX_MODE_7) {
                if ((chipport > 16 && chipport < 19) || (chipport == 27)) 
                    continue;                
            }               
            if (VTSS_PORT_IS_1G(chipport)) {
                speed = VTSS_SPEED_1G;
            } else {
                speed = VTSS_SPEED_10G;
            }
            VTSS_RC(jr_sqs_rd(chipport, &sqs));
            VTSS_RC(jr_sqs_calc_wm(chipport, speed, 1, 1, 0, 0, &sqs)); /* FC=1, Jumbo=1 */
            VTSS_RC(jr_sqs_set(chipport, &sqs));
        }
    } else if (target == VTSS_TARGET_JAGUAR_1 || target == VTSS_TARGET_LYNX_1) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            speed = vtss_state->port_conf[port_no].speed;
            chipport = VTSS_CHIP_PORT(port_no);
            VTSS_RC(jr_sqs_rd(chipport, &sqs));
            VTSS_RC(jr_sqs_calc_wm(chipport, speed, 0, 0, 0, 0, &sqs)); /* FC=0, Jumbo=0 */
            VTSS_RC(jr_sqs_set(chipport, &sqs));
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc jr_port_map_set(void)
{
    vtss_port_no_t    port_no;
    u32               pgid;
    BOOL              member[VTSS_PORT_ARRAY_SIZE];
    vtss_pgid_entry_t *pgid_entry;

    /* We only need to setup the no of avail pgids */
    vtss_state->pgid_count = (VTSS_PGID_JAGUAR_1 - VTSS_CHIP_PORTS + vtss_state->port_count);
    
    /* For dual chip targets, the calculation above must be limited */
    if (vtss_state->pgid_count > VTSS_PGID_JAGUAR_1)
        vtss_state->pgid_count = VTSS_PGID_JAGUAR_1;

    /* And then reserve PGIDs for flood masks */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        member[port_no] = cemax_port_is_host(port_no) ? 0 : 1;
    for (pgid = PGID_UC_FLOOD; pgid <= PGID_IPV6_MC_CTRL; pgid++) {
        VTSS_RC(jr_pgid_update(pgid, member));
    }
    
    /* Drop entry */
    pgid = jr_vtss_pgid(vtss_state, PGID_DROP);
    vtss_state->pgid_drop = pgid;
    vtss_state->pgid_table[pgid].references = 1;
    
    /* Flood entry */
    pgid = jr_vtss_pgid(vtss_state, PGID_FLOOD);
    vtss_state->pgid_flood = pgid;
    pgid_entry = &vtss_state->pgid_table[pgid];
    pgid_entry->references = 1;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) 
        pgid_entry->member[port_no] = cemax_port_is_host(port_no) ? 0 : 1;

#if !defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    /* GLAG entries */
    vtss_state->pgid_glag_dest = jr_vtss_pgid(vtss_state, PGID_GLAG_START);     

    /* Reserve entries for GLAG destinations */
    for (pgid = PGID_GLAG_START; pgid < (PGID_GLAG_START+VTSS_GLAG_PORTS*VTSS_GLAGS); pgid++) {
        vtss_state->pgid_table[jr_vtss_pgid(vtss_state, pgid)].resv = 1;
    }
#endif /* !defined (VTSS_ARCH_JAGUAR_1_CE_MAC) */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    {
        /* Do the Line Port to Host Port mapping */
        u32 host_mode = ce_max_hm();
        u32 max_lport, prio, lprio, hmq, hmdx, upsid, upspn, ups_mask, prios, mask, pgid_offset=64;
        vtss_lport_no_t lport_no;

        if (host_mode == 0 || host_mode == 2) { 
            prios = 8;
            max_lport = 24;
        } else {
            max_lport = 48;
            prios = 4;
        }
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {

            if (cemax_port_is_host(port_no)) {
                continue; /* HI do not map to Lports */
            }
  
            lport_no = vtss_state->port_map[port_no].lport_no;

            if (lport_no == VTSS_LPORT_MAP_NONE) { /* No mapping - skip */
                continue;
            } else if (lport_no == VTSS_LPORT_MAP_DEFAULT) {  
                lport_no = (port_no - VTSS_PORT_NO_START); /* Default mapping: API port to Lport, starting from 0. */ 
                if (lport_no >= max_lport) {
                    break;
                }
            }
            if (lport_no >= max_lport) {
                VTSS_E("illegal lport_no: %d for port_no: %d, host mode: %u", lport_no, port_no, host_mode);
                return VTSS_RC_ERROR;
            }
            /* Update the API map with the lport_no (only meaningful if VTSS_LPORT_MAP_NONE was used) */
            vtss_state->port_map[port_no].lport_no = lport_no;
           
            if (host_mode == 0) { 
                hmdx = (lport_no < 12) ? 29 : 30;
            } else if (host_mode == 1) { 
                hmdx = (lport_no < 24) ? 29 : 30;
            } else { 
                if (vtss_state->create.target == VTSS_TARGET_CE_MAX_24) {
                    hmdx = 30;
                } else {
                    hmdx = 28;
                }
            }
            
            /* Apply the mappings to the PGID table. The PGID HQ index starts at 1024+32 */
            for (prio = 0, lprio = 0; prio < 8; prio++) {
                if (host_mode == 0 || host_mode == 2) {
                    if (prio != 0)
                        lprio++; /* 8 prios maps to 8 host queues */
                } else {
                    /* 8 prios map to 4 host queues 0-1 -> 0, ... 6,7 -> 3 */
                    if ((prio % 2 == 0) && (prio != 0)) {
                        lprio++;
                    } 
                }                
                pgid = PGID_HQ_START + (VTSS_CHIP_PORT(port_no) * 8) + prio;
                hmq = (lport_no * prios) + lprio;
                upsid = (1 + (hmq / 32));
                upspn = hmq % 32;
                ups_mask = hmdx<<16 | upsid<<6 | upspn;

                vtss_state->pgid_table[jr_vtss_pgid(vtss_state, pgid)].resv = 1; /* Reserve PGID in AIL */
                VTSS_RC(jr_pgid_table_write_chip(pgid, ups_mask, 0, 0, 1)); /* Write PGID to chip. CPU copy=0, cpu qu=0, stack=1 */
            }

            /* Host to line forwarding. Lport maps back to API port */
            pgid = lport_no;
            mask = 1 << VTSS_CHIP_PORT(port_no - VTSS_PORT_NO_START); 
            vtss_state->pgid_table[jr_vtss_pgid(vtss_state, pgid)].resv = 1; /* Reserve PGID in AIL */
            VTSS_RC(jr_pgid_table_write_chip(pgid, mask, 0, 0, 0)); /* Write PGID to chip. CPU copy=0, cpu qu=0, stack=0 */

            /* Host to line forwarding from HMDB. PGID offset:64 */
            if (host_mode == 0 && lport_no > 11) {
                mask = 1 << VTSS_CHIP_PORT(port_no - VTSS_PORT_NO_START); 
                vtss_state->pgid_table[jr_vtss_pgid(vtss_state, pgid_offset)].resv = 1; /* Reserve PGID in AIL */
                VTSS_RC(jr_pgid_table_write_chip(pgid_offset, mask, 0, 0, 0)); /* Write PGID to chip. CPU copy=0, cpu qu=0, stack=0 */
                pgid_offset++; 
            }
        }
    }
#endif /* defined(VTSS_ARCH_JAGUAR_1_CE_MAC) */

    /* Setup Rx registrations and queue mappings */
    VTSS_RC(jr_rx_conf_set());

#if defined(VTSS_OPT_EVC_OAM_SDX_ZERO)
    {
        /* Ensure that all OAM frames get SDX zero */
        vtss_vcap_data_t  data;
        vtss_is1_data_t   *is1 = &data.is1;
        vtss_is1_entry_t  entry;
        vtss_is1_action_t *action = &entry.action;
        vtss_is1_key_t    *key = &entry.key;

        memset(&data, 0, sizeof(data));
        memset(&entry, 0, sizeof(entry));
        is1->vid_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->dscp_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->sport_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->dport_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->entry = &entry;
        is1->first = 1;
        action->isdx_enable = 1;
        key->type = VTSS_IS1_TYPE_ETYPE;
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            key->port_list[port_no] = 1;
        key->frame.etype.etype.value[0] = 0x89;
        key->frame.etype.etype.value[1] = 0x02;
        key->frame.etype.etype.mask[0] = 0xff;
        key->frame.etype.etype.mask[1] = 0xff;
        VTSS_RC(vtss_vcap_add(&vtss_state->is1.obj, VTSS_IS1_USER_EVC, 
                              1, VTSS_VCAP_ID_LAST, &data, 0));
    }
#endif /* VTSS_OPT_EVC_OAM_SDX_ZERO */

#if defined(VTSS_FEATURE_VSTAX_V2)
    /* Initialize internal ports */
    VTSS_RC(jr_init_ports_int());
#endif /* VTSS_FEATURE_VSTAX_V2 */

    /* Setup WM for chip ports (for some targets) */
    VTSS_RC(jr_update_sqs());

    return VTSS_RC_OK;
}


#if defined (VTSS_ARCH_JAGUAR_1_CE_MAC)
/* Configure Jaguar to operate as a MAC */
static vtss_rc jr_init_cemax(void)
{
    u32 hmda, hmdb, hmd_mask=0, flood_mask, pgid, i, prio, host_mode;

    host_mode = ce_max_hm();    

    if (host_mode == 0 || host_mode == 1) {
        ce_mac_hmda = VTSS_CE_MAC_HMDA;
        ce_mac_hmdb = VTSS_CE_MAC_HMDB;
    } else { /* host_mode == VTSS_HOST_MODE_2 || host_mode == VTSS_HOST_MODE_3 */
        if (vtss_state->create.target == VTSS_TARGET_CE_MAX_24) {
            ce_mac_hmda = VTSS_CE_MAC_HMDB;
            ce_mac_hmdb = VTSS_CE_MAC_HMDB;
        } else {
            ce_mac_hmda = VTSS_CE_MAC_HMDA;
            ce_mac_hmdb = VTSS_CE_MAC_HMDA;
        }
    }
    hmda = ce_mac_hmda;
    hmdb = ce_mac_hmdb;

    /* == Analyzer == */
    for (i = 0; i <= (hmdb-hmda); i++) {
        JR_WRXB(ANA_CL_2, COMMON_HM_CFG, i, HMD_PORT_VLD, 1);
        JR_WRXF(ANA_CL_2, COMMON_HM_CFG, i, HMD_PORT, (i==0?hmda:hmdb));            
        JR_WRXB(ANA_AC, PS_COMMON_HM_CFG, i, HMD_PORT_VLD, 1);
        JR_WRXB(ANA_AC, PS_COMMON_HM_CFG, i, HMD_HOST_ENA, 1);
        JR_WRXF(ANA_AC, PS_COMMON_HM_CFG, i, HMD_PORT, (i==0?hmda:hmdb));
        if (i == 1) {
            /* Enable PGID offset for HMDB */
            JR_WRXF(ANA_AC, PS_COMMON_HM_CFG, i, HMD_ADDR_IDX, 1);            
            JR_WRF(ANA_AC, PS_COMMON_PS_COMMON_CFG, HMD_ADDR_OFFSET, 1);
        }
        hmd_mask |= (1<<(i==0?hmda:hmdb));       
    }

    /* Enable MAC based forwarding in ANA_AC */
    JR_WRF(ANA_L2, COMMON_VSTAX_CTRL, OWN_UPSID, 0);
    JR_WRF(ANA_AC, PS_COMMON_COMMON_VSTAX_CFG, OWN_UPSID, 0);
    JR_WRB(ANA_AC, PS_COMMON_PS_COMMON_CFG, HM_FWD_ENA, 1);

    /* Disable learning */
    JR_WR(ANA_L3_2, COMMON_PORT_LRN_CTRL, 0);

    /* No flooding to HMDs */
    flood_mask = 0xffffffff ^ hmd_mask;
    for (pgid = 32; pgid < 38; pgid++) {
        JR_WRX(ANA_AC, PGID_PGID_CFG_0, pgid, flood_mask);        
    }

    /* == Arbiter == */    
    /* Use UPSID 0-7 */
    JR_WRF(ARB, CFG_STATUS_HM_CFG, HM_UPSID_RANGE, 0);    
    JR_WRF(ARB, CFG_STATUS_HM_CFG, ETH_DEV_HMDA, hmda);
    JR_WRF(ARB, CFG_STATUS_HM_CFG, ETH_DEV_HMDB, hmdb);

    /* Enable use of CM for arbitrating traffic from line ports */
    for (prio = 0; prio < 8; prio++) {
        JR_WRX(ARB, CFG_STATUS_CM_CFG_VEC0, prio, 0xffffffff); 
    }
    /* Enable Host mode operation */
    JR_WRB(ARB, CFG_STATUS_HM_CFG, HM_ENA, 1);

    /* == SQS == */
    /* Enable WM for host queue */
    JR_WRB(OQS, CE_HOST_MODE_CFG_HOST_MODE_CM_CFG, HOST_MODE_CM_SEL, 1);
    
    /* 4 or 8 priorities per HP */
    JR_WRB(OQS, CE_HOST_MODE_CFG_HOST_PORT_RESRC_CTL_CFG, HOST_PORT_PRIO_CFG, 
           (host_mode == 0 || host_mode == 2) ? 1 : 0);

    /* == Scheduler == */
    JR_WRF(SCH, QSIF_QSIF_HM_CTRL, HM_MODE, host_mode);
    JR_WRF(SCH, QSIF_QSIF_HM_CTRL, ETH_DEV_HMDA, hmda);
    JR_WRF(SCH, QSIF_QSIF_HM_CTRL, ETH_DEV_HMDB, (hmda == hmdb) ? 0 : hmdb);
    JR_WRF(SCH, HM_HM_SCH_CTRL, HM_MODE, host_mode);

    /* == Rewriter == */    
    JR_WRF(REW, COMMON_HM_CTRL, HM_MODE, host_mode);    
    JR_WRF(REW, COMMON_HM_CTRL, ETH_DEV_HMDA, hmda);    
    JR_WRF(REW, COMMON_HM_CTRL, ETH_DEV_HMDB, (hmda == hmdb) ? 0 : hmdb);

    /* == Disassembler == */    
    /* Select 4 or 8 priorities */
    JR_WRB(DSM, CM_CFG_HOST_PORT_RESRC_CTL_CFG, HOST_PORT_PRIO_CFG, 
           (host_mode == 0 || host_mode == 2) ? 1 : 0);
    JR_WRF(DSM, CM_CFG_CMEF_OWN_UPSID_CFG, CMEF_OWN_UPSID, 0);
    JR_WRF(DSM, CM_CFG_CMEF_OWN_UPSID_CFG, HM_OWN_UPSID_MASK, 3);
    JR_WR(DSM, CM_CFG_CMEF_UPSID_ACTIVE_CFG, 0x7f);

    /* Enable CM generation for HM queues */
    for(i = 0; i < 6; i++) {
        JR_WRX(DSM, CM_CFG_CMM_HOST_QU_MASK_CFG, i, 0xffffffff);
    }
    /* Enable internal FC for HPs */
    if (host_mode == 0 || host_mode == 2) {
        JR_WRX(DSM, CM_CFG_CMM_HOST_LPORT_MASK_CFG, 0, 0xffffffff);  /* 24 HPs */
        JR_WRX(DSM, CM_CFG_CMM_HOST_LPORT_MASK_CFG, 1, 0);
    } else {
        /* 48 HPs */
        JR_WRX(DSM, CM_CFG_CMM_HOST_LPORT_MASK_CFG, 0, 0xffffffff);  /* 48 HPs */
        JR_WRX(DSM, CM_CFG_CMM_HOST_LPORT_MASK_CFG, 1, 0xffffffff);
    }

    /* == OOBFC == */
    JR_WRF(DSM, CM_CFG_CMEF_OWN_UPSID_CFG, HM_OWN_UPSID_MASK, 4);
    JR_WR(DSM, CM_CFG_CMEF_UPSID_ACTIVE_CFG, 0xff7f);

    /* Initialize the LPORT_NUM table */
    JR_WR(DSM, CM_CFG_LPORT_NUM_INIT, 1);
    JR_POLL_BIT(DSM, CM_CFG_LPORT_NUM_INIT, LPORT_NUM_INIT_SHOT);

    // For the status channel registers configure which UPSID, they shall capture
    // status for.
    {
        u8 upsid = 8;
        u8 tmp;
        for (tmp = 0; tmp <= 7; tmp++) {
            JR_WRX(DSM, HOST_IF_IBS_COMMON_CFG_CAPTURE_UPSID_CFG, tmp, upsid);
            upsid++;
        }
        JR_WRX(DSM, HOST_IF_IBS_COMMON_CFG_CAPTURE_UPSID_CFG, 8, 0);
    }



    return VTSS_RC_OK;
}

#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

static vtss_rc jr_init_conf_set_chip(void)
{
    u32 value, port, i;

    /* Read chip ID to check CPU interface */
    VTSS_RC(jr_chip_id_get_chip(&vtss_state->chip_id));
    VTSS_I("chip_id[%u]: 0x%04x, revision: 0x%04x",
           vtss_state->chip_no, vtss_state->chip_id.part_number, vtss_state->chip_id.revision);

    /* Check chip ID against created target */
    if ((vtss_state->chip_id.part_number & 0xfff0) == 0x7430 && 
        (vtss_state->create.target & 0xfff0) == 0x7460) {
        /* SMB devices (VSC743x) do not support CE targets (VSC746x) */
        VTSS_E("device 0x%04x does not support CE software", vtss_state->chip_id.part_number);
        return VTSS_RC_ERROR;
    }

    /* Read restart type */
    if (vtss_state->chip_no == 0) {
        JR_RD(DEVCPU_GCB, CHIP_REGS_GENERAL_PURPOSE, &value);
        vtss_state->init_conf.warm_start_enable = 0; /* Warm start not supported */
        VTSS_RC(vtss_cmn_restart_update(value));
    }

    /* ----------- Initalize blocks ------------ */
    VTSS_RC(jr_init_hsio());
    VTSS_RC(jr_init_asm());
    VTSS_RC(jr_init_ana());
    VTSS_RC(jr_init_vcap_is0());
    VTSS_RC(jr_init_vcap_is1());
    VTSS_RC(jr_init_vcap_is2());
    VTSS_RC(jr_init_vcap_es0());
    VTSS_RC(jr_init_sqs());
    VTSS_RC(jr_init_arb());
    VTSS_RC(jr_init_sch());
    VTSS_RC(jr_init_rew());
    VTSS_RC(jr_init_dsm());
    VTSS_RC(jr_init_l3_chip());
    /* ----------- CE-MAX special ---------- */
#if defined (VTSS_ARCH_JAGUAR_1_CE_MAC)
    VTSS_RC(jr_init_cemax());
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

    /* ----------- Initalize features ---------- */

    /* Disable learning */
    JR_WRX(ANA_L3_2, MSTP_MSTP_LRN_CFG, 0, 0);

    /* Initialize packet control */
    VTSS_RC(jr_init_packet()); 

    /* VLAN: Clear VID 4095 mask, enable VLAN and use default port config */
    JR_WR(ANA_L3_2, COMMON_VLAN_CTRL, VTSS_F_ANA_L3_2_COMMON_VLAN_CTRL_VLAN_ENA);
    JR_WRX(ANA_L3_2, VLAN_VLAN_MASK_CFG, 4095, 0);
    VTSS_RC(jr_vlan_mask_apply(1, 0xffffffff));
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        VTSS_RC(jr_vlan_port_conf_apply(port, &vtss_state->vlan_port_conf[VTSS_PORT_NO_START], NULL));
    }

    /* Disable learning in VLAN 0 (used for EVCs) */
    JR_WRXB(ANA_L3_2, VLAN_VLAN_CFG, 0, VLAN_LRN_DIS, 1);

    /* Setup VLAN configuration */
    VTSS_RC(jr_vlan_conf_set_chip(0));

    /* Disable policers */
    {
        vtss_policer_t     pol_conf;
        vtss_policer_ext_t pol_ext_conf;

        memset(&pol_conf, 0, sizeof(pol_conf));
        memset(&pol_ext_conf, 0, sizeof(pol_ext_conf));
        pol_conf.rate = VTSS_BITRATE_DISABLED;
        for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
            for (i = 0; i < 4; i++) {
                VTSS_RC(jr_port_policer_set(port, i, &pol_conf, &pol_ext_conf));
            }
            for (i = 0; i < 8; i++) {
                VTSS_RC(jr_queue_policer_set(port, i, &pol_conf));
            }
        }
    }

    /* Set MAC age time to default value */
    VTSS_RC(jr_mac_table_age_time_set_chip());

#if !defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    /* Enable frame aging */
    JR_WR(ARB, AGING_CFG_ERA_CTRL, 0x41); /* Set tick to 0.5 sec and enable era generator */
    JR_WR(SCH, MISC_ERA_CTRL, 0x41);      /* Set tick to 0.5 sec and enable era generator */
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        JR_WRXB(DSM, CFG_BUF_CFG, port, AGING_ENA, 1); /* Enable aging in disassembler for each port */
    }
#endif /* !defined(VTSS_ARCH_JAGUAR_1_CE_MAC) */

    /* Setup ANA_AC to count local drops per port */
    JR_WRX(ANA_AC, PS_STICKY_MASK_STICKY_MASK, 0, 
           VTSS_F_ANA_AC_PS_STICKY_MASK_STICKY_MASK_ZERO_DST_STICKY_MASK);
    JR_WRX(ANA_AC, STAT_GLOBAL_CFG_PORT_STAT_GLOBAL_EVENT_MASK, 0,
           VTSS_F_ANA_AC_STAT_GLOBAL_CFG_PORT_STAT_GLOBAL_EVENT_MASK_GLOBAL_EVENT_MASK(1<<0));
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        JR_WRXY(ANA_AC, STAT_CNT_CFG_PORT_STAT_CFG, port, JR_CNT_ANA_AC_FILTER,
                VTSS_F_ANA_AC_STAT_CNT_CFG_PORT_STAT_CFG_CFG_PRIO_MASK(0xff) |
                VTSS_F_ANA_AC_STAT_CNT_CFG_PORT_STAT_CFG_CFG_CNT_FRM_TYPE(3));
    }
    
    /* Enable ES0 lookup (SERVICE_LU_ENA) */
    JR_WR(REW, COMMON_SERVICE_CTRL, 1<<0);

    /* Enable ISDX table */
    JR_WRB(ANA_L2, COMMON_FWD_CFG, ISDX_LOOKUP_ENA, 1);

    /* Setup IS2 discard policer */
    JR_WRX(ANA_AC, POL_ALL_CFG_POL_ACL_CTRL, JR_IS2_POLICER_DISCARD,
           JR_PUT_FLD(ANA_AC, POL_ALL_CFG_POL_ACL_CTRL, ACL_TRAFFIC_TYPE_MASK, 2));

#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    /* Setup VLAN statistics:
       - Even counters (0,2,4) are byte counters
       - Odd counters (1,3,5) are frame counters */
    for (i = 0; i < 6; i++) {
        /* ANA_AC */
        JR_SDX_CNT_SET(ANA_AC, ISDX, i, i & 1, 1<<((i/2)+3));
    }
#else /* VTSS_FEATURE_VLAN_COUNTERS */
    /* Setup SDX statistics:
       - Even counters (0,2,4) are byte counters
       - Odd counters (1,3,5) are frame counters */
    for (i = 0; i < 6; i++) {
        /* ANA_AC */
        JR_SDX_CNT_SET(ANA_AC, ISDX, i, i & 1, 1<<(i/2));
    }
#endif /* VTSS_FEATURE_VLAN_COUNTERS */

    for (i = 0; i < 2; i++) {
        /* IQS/OQS */
        JR_SDX_CNT_SET(IQS, ISDX, i, i & 1, (1<<10) | (1<<11));
        JR_SDX_CNT_SET(OQS, ISDX, i, i & 1, (1<<10) | (1<<11));
    }
    for (i = 0; i < 4; i++) {
        /* REW */
        JR_SDX_CNT_SET(REW, ESDX, i, i & 1, i < 2 ? 0x1 : 0xe);
    }

#if defined(VTSS_FEATURE_EVC)
    /* Setup EVC open and discard policer */
    {
        vtss_evc_policer_conf_t pol_conf;
        
        memset(&pol_conf, 0, sizeof(pol_conf));
        pol_conf.cm = 1; /* Colour aware to make statistics show incoming colour */
        VTSS_RC(jr_evc_policer_conf_apply(VTSS_EVC_POLICER_ID_NONE, &pol_conf));
        pol_conf.enable = 1;
        VTSS_RC(jr_evc_policer_conf_apply(VTSS_EVC_POLICER_ID_DISCARD, &pol_conf));
    }
#endif /* VTSS_FEATURE_EVC */

    /* CMEF DMAC address */
    JR_WRF(DSM, CM_CFG_CMEF_MAC_ADDR_HIGH_CFG, CMEF_MAC_ADDR_HIGH, 0xFFFFFF);
    JR_WRF(DSM, CM_CFG_CMEF_MAC_ADDR_LOW_CFG, CMEF_MAC_ADDR_LOW, 0xFFFFFF);

#if defined(VTSS_FEATURE_TIMESTAMP)
    /* enable ptp master counter and clock adjustment */
    JR_WR(DEVCPU_GCB, PTP_CFG_PTP_MISC_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_PTP_ENA);
    JR_WR(DEVCPU_GCB, PTP_CFG_CLK_ADJ_CFG,  HW_CLK_CNT_PR_SEC |
           VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_UPD);
    /* configure RX timestamping config: Overwriting the frame's FCS field with the Rx timestamp value*/
    JR_WRM_SET(ANA_AC, PS_COMMON_MISC_CTRL, VTSS_F_ANA_AC_PS_COMMON_MISC_CTRL_OAM_RX_TSTAMP_IN_FCS_ENA);
    /* enable timestamping for all ports. Note: Vstax2 awareness is disabled. */
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        JR_WRXM_SET(ASM, CFG_ETH_CFG, port, VTSS_F_ASM_CFG_ETH_CFG_ETH_PRE_MODE);
        if (VTSS_PORT_IS_1G(port)) {
            JR_WRXM_SET(DEV1G, DEV_CFG_STATUS_DEV_PTP_CFG, VTSS_TO_DEV(port), VTSS_F_DEV1G_DEV_CFG_STATUS_DEV_PTP_CFG_PTP_ENA);
        } else {
            JR_WRXM_SET(DEV10G, DEV_CFG_STATUS_DEV_PTP_CFG, VTSS_TO_DEV(port), VTSS_F_DEV10G_DEV_CFG_STATUS_DEV_PTP_CFG_PTP_CFG_ENA);
        }
    }
#endif    

    /* Setup all 10G ports to avoid delayed SP injection */
    {
        u32 no_of_10g = (jr_target_get() == VTSS_TARGET_CE_MAX_12 ? 2 : 4);
        vtss_port_mux_mode_t mux_mode = vtss_state->mux_mode[vtss_state->chip_no];

        
        for (i = 0; i < no_of_10g; i++) {
            if (mux_mode == VTSS_PORT_MUX_MODE_1) {
                if (i + VTSS_PORT_10G_START == 27) 
                    continue; /* Port 27 is not active */
            } else if (mux_mode == VTSS_PORT_MUX_MODE_7) {
                if (i + VTSS_PORT_10G_START < 29)
                    continue; /* Port 27,28 are not active */
            }
                
            JR_WRX(DSM, CFG_RATE_CTRL, i + VTSS_PORT_10G_START,
                   VTSS_F_DSM_CFG_RATE_CTRL_FRM_GAP_COMP(JR_RATE_CTRL_GAP_10G) |
                   VTSS_F_DSM_CFG_RATE_CTRL_TAXI_RATE_HIGH(JR_RATE_CTRL_WM_HI_10G) |
                   VTSS_F_DSM_CFG_RATE_CTRL_TAXI_RATE_LOW(JR_RATE_CTRL_WM_LO_10G));
        }
    }
    
    return VTSS_RC_OK;
}

static vtss_rc jr_init_conf_set(void)
{
    /* Store mux modes per device */
    vtss_state->mux_mode[0] = vtss_state->init_conf.mux_mode;
#if defined(VTSS_ARCH_JAGUAR_1_DUAL)
    vtss_state->mux_mode[1] = vtss_state->init_conf.mux_mode_2;

    /* Select internal ports for dual Jaguar targets */
    switch (vtss_state->init_conf.dual_connect_mode) {
    case VTSS_DUAL_CONNECT_MODE_0: /* XAUI_0 and XAUI_1 */
        vtss_state->port_int_0 = 27;
        vtss_state->port_int_1 = 28;
        break;
    case VTSS_DUAL_CONNECT_MODE_1: /* XAUI_2 and XAUI_3 */
        vtss_state->port_int_0 = 29;
        vtss_state->port_int_1 = 30;
        break;
    default:
        VTSS_E("illegal connect mode");
        return VTSS_RC_ERROR;
    }
    vtss_state->mask_int_ports = (VTSS_BIT(vtss_state->port_int_0) | 
                                  VTSS_BIT(vtss_state->port_int_1));
#endif /* VTSS_ARCH_JAGUAR_1_DUAL */

    // For register access checking.
    // Avoid Lint Warning 572: Excessive shift value (precision 1 shifted right by 2),
    // which is due to use of the first target in the range (VTSS_DEVCPU_ORG).
    /*lint --e{572} */
    vtss_state->reg_check.addr = VTSS_DEVCPU_ORG_ORG_ERR_CNTS;

    /* Initialize all devices */
    return jr_conf_chips(jr_init_conf_set_chip);
}

static vtss_rc jr_poll_1sec(void)
{
    vtss_rc rc;

    rc = jr_poll_sgpio_1sec();
    
#if defined(VTSS_FEATURE_EVC)
    {
        vtss_rc rc2 = jr_evc_poll_1sec();
        if (rc == VTSS_RC_OK)
            rc = rc2;
    }
#endif /* VTSS_FEATURE_EVC */

#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    {
        vtss_rc rc2 = jr_vlan_counters_poll_1sec();
        if (rc == VTSS_RC_OK)
            rc = rc2;
    }
#endif /* VTSS_FEATURE_VLAN_COUNTERS */

#if defined(VTSS_SW_OPTION_L3RT)
    {
        vtss_rc rc2 = jr_l3_rleg_stat_poll();
        if (rc == VTSS_RC_OK)
            rc = rc2;
    }
#endif /* VTSS_SW_OPTION_L3RT */

    return rc;
}

vtss_rc vtss_jaguar1_inst_create(void)
{
    vtss_cil_func_t *func = &vtss_state->cil_func;
    vtss_vcap_obj_t *is0 = &vtss_state->is0.obj;
    vtss_vcap_obj_t *is1 = &vtss_state->is1.obj;
    vtss_vcap_obj_t *is2 = &vtss_state->is2.obj;
    vtss_vcap_obj_t *es0 = &vtss_state->es0.obj;

    /* Initialization */
    func->init_conf_set = jr_init_conf_set;
    func->restart_conf_set = jr_restart_conf_set;
    func->reg_read = jr_reg_read;
    func->reg_write = jr_reg_write;

    /* Miscellaneous */
    func->chip_temp_get = jr_temp_sensor_read;
    func->chip_temp_init = jr_temp_sensor_init;
    func->gpio_mode = jr_gpio_mode;
    func->gpio_read = jr_gpio_read;
    func->gpio_write = jr_gpio_write;
    func->gpio_event_poll = jr_gpio_event_poll;
    func->gpio_event_enable = jr_gpio_event_enable;
    func->chip_id_get = jr_chip_id_get;
    func->intr_sticky_clear = jr_intr_sticky_clear;
    func->poll_1sec = jr_poll_1sec;
    func->sgpio_conf_set = jr_sgpio_conf_set;
    func->sgpio_read = jr_sgpio_read;
    func->sgpio_event_enable = jr_sgpio_event_enable;
    func->sgpio_event_poll = jr_sgpio_event_poll;
    func->debug_info_print = jr_debug_info_print;
    func->ptp_event_poll = jr_ptp_event_poll;
    func->ptp_event_enable = jr_ptp_event_enable;
    func->dev_all_event_poll = jr_dev_all_event_poll;
    func->dev_all_event_enable = jr_dev_all_event_enable;

    /* Port control */
    func->miim_read = jr_miim_read;
    func->miim_write = jr_miim_write;
    func->mmd_read = jr_mmd_read;
    func->mmd_read_inc = jr_mmd_read_inc;
    func->mmd_write = jr_mmd_write;
    func->port_map_set = jr_port_map_set;
    func->port_conf_set = jr_port_conf_set;
    func->port_clause_37_status_get = jr_port_clause_37_status_get;
    func->port_clause_37_control_set = jr_port_clause_37_control_set;
    func->port_status_get = jr_port_status_get;
    func->port_counters_update = jr_port_counters_update;
    func->port_counters_clear = jr_port_counters_clear;
    func->port_counters_get = jr_port_counters_get;
    func->port_basic_counters_get = jr_port_basic_counters_get;
    func->port_forward_set = jr_port_forward_set;
#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    func->host_conf_set = jr_xaui_conf_set;
    func->qos_lport_conf_set = jr_qos_lport_conf_set;
    func->qos_lport_conf_get = jr_qos_lport_conf_get;
    func->port_is_host       = jr_port_is_host;
    func->lport_counters_get = jr_lport_counters_get;
    func->lport_counters_clear = jr_lport_counters_clear;
#endif

    /* QoS */
    func->qos_conf_set = jr_qos_conf_set;
    func->qos_port_conf_set = vtss_cmn_qos_port_conf_set;
    func->qos_port_conf_update = jr_qos_port_conf_set;
    func->qce_add = vtss_cmn_qce_add;
    func->qce_del = vtss_cmn_qce_del;

    /* Packet control */
    func->rx_conf_set      = jr_rx_conf_set;
    func->rx_frame_get     = jr_rx_frame_get;
    func->rx_frame_discard = jr_rx_frame_discard;
    func->tx_frame_port    = jr_tx_frame_port;
    func->rx_hdr_decode    = jr_rx_hdr_decode;
    func->tx_hdr_encode    = jr_tx_hdr_encode;
#if defined(VTSS_FEATURE_VSTAX_V2)
    func->vstax_header2frame = jr_vstax_header2frame;
    func->vstax_frame2header = jr_vstax_frame2header;
#endif /* VTSS_FEATURE_VSTAX_V2 */
#if defined(VTSS_FEATURE_NPI)
    func->npi_conf_set = jr_npi_conf_set;
#endif  /* VTSS_FEATURE_NPI */

    /* Security */
    func->acl_policer_set = jr_acl_policer_set;
    func->acl_port_set = jr_acl_port_conf_set;
    func->acl_port_counter_get = jr_acl_port_counter_get;
    func->acl_port_counter_clear = jr_acl_port_counter_clear;
    func->acl_ace_add = jr_ace_add;
    func->acl_ace_del = jr_ace_del;
    func->acl_ace_counter_get = jr_ace_counter_get;
    func->acl_ace_counter_clear = jr_ace_counter_clear;
    func->vcap_range_commit = jr_vcap_range_commit;

    /* IS0 */
    is0->max_count = VTSS_JR1_IS0_CNT;
    is0->entry_get = jr_is0_entry_get;
    is0->entry_add = jr_is0_entry_add;
    is0->entry_del = jr_is0_entry_del;
    is0->entry_move = jr_is0_entry_move;

    /* IS1 */
    is1->max_count = VTSS_JR1_IS1_CNT;
    is1->entry_get = jr_is1_entry_get;
    is1->entry_add = jr_is1_entry_add;
    is1->entry_del = jr_is1_entry_del;
    is1->entry_move = jr_is1_entry_move;

    /* IS2 */
    is2->max_count = VTSS_JR1_IS2_CNT;
    is2->entry_get = jr_is2_entry_get;
    is2->entry_add = jr_is2_entry_add;
    is2->entry_del = jr_is2_entry_del;
    is2->entry_move = jr_is2_entry_move;

    /* ES0 */
    es0->max_count = VTSS_JR1_ES0_CNT;
    es0->entry_get = jr_es0_entry_get;
    es0->entry_add = jr_es0_entry_add;
    es0->entry_del = jr_es0_entry_del;
    es0->entry_move = jr_es0_entry_move;
    func->es0_entry_update = jr_es0_entry_update;

#if defined(VTSS_FEATURE_EVC)
    jr_evc_init();
#endif /* VTSS_FEATURE_EVC */

    /* Layer 2 */
    func->mac_table_add  = jr_mac_table_add;
    func->mac_table_del  = jr_mac_table_del;
    func->mac_table_get = jr_mac_table_get;
    func->mac_table_get_next = jr_mac_table_get_next;
    func->mac_table_age_time_set = jr_mac_table_age_time_set;
    func->mac_table_age = jr_mac_table_age;
#if defined(VTSS_FEATURE_VSTAX_V2)
    func->mac_table_upsid_upspn_flush = jr_mac_table_upsid_upspn_flush;
#endif /* VTSS_FEATURE_VSTAX_V2 */
    func->mac_table_status_get = jr_mac_table_status_get;
    func->learn_port_mode_set = jr_learn_port_mode_set;
    func->learn_state_set = jr_learn_state_set;
    func->mstp_state_set = vtss_cmn_mstp_state_set;
    func->mstp_vlan_msti_set = vtss_cmn_vlan_members_set;
    func->erps_vlan_member_set = vtss_cmn_erps_vlan_member_set;
    func->erps_port_state_set = vtss_cmn_erps_port_state_set;
    func->vlan_conf_set = jr_vlan_conf_set;
    func->vlan_port_conf_set = vtss_cmn_vlan_port_conf_set;
    func->vlan_port_conf_update = jr_vlan_port_conf_update;
    func->vlan_port_members_set = vtss_cmn_vlan_members_set;
    func->vlan_mask_update = jr_vlan_mask_update;
    func->vlan_tx_tag_set = vtss_cmn_vlan_tx_tag_set;
    func->isolated_vlan_set = vtss_cmn_vlan_members_set;
    func->isolated_port_members_set = jr_isolated_port_members_set;
    func->aggr_mode_set = jr_aggr_mode_set;
    func->mirror_port_set = jr_mirror_port_set;
    func->mirror_ingress_set = jr_mirror_ingress_set;
    func->mirror_egress_set = jr_mirror_egress_set;
    func->flood_conf_set = jr_flood_conf_set;
#if defined(VTSS_FEATURE_IPV4_MC_SIP)
    func->ipv4_mc_add = vtss_cmn_ipv4_mc_add;
    func->ipv4_mc_del = vtss_cmn_ipv4_mc_del;
    func->ip_mc_update = jr_ip_mc_update;
#endif /* VTSS_FEATURE_IPV4_MC_SIP */
#if defined(VTSS_FEATURE_VSTAX_V2)
    func->vstax_conf_set = jr_vstax_conf_set;
    func->vstax_master_upsid_set = jr_vstax_master_upsid_set;
    func->vstax_port_conf_set = jr_vstax_port_conf_set;
    func->vstax_topology_set = jr_vstax_topology_set;
#endif /* VTSS_FEATURE_VSTAX_V2 */
#if defined(VTSS_FEATURE_AGGR_GLAG)
    func->glag_src_table_write = jr_glag_src_table_write;
    func->glag_port_write = jr_glag_port_write;
#endif /*VTSS_FEATURE_AGGR_GLAG */
    func->src_table_write = jr_src_table_write;
    func->pgid_table_write = jr_pgid_table_write;
    func->aggr_table_write = jr_aggr_table_write;
    func->pmap_table_write = jr_pmap_table_write;
    func->eps_port_set = vtss_cmn_eps_port_set;
#if defined(VTSS_FEATURE_VCL)
    func->vcl_port_conf_set = jr_vcl_port_conf_set;
    func->vce_add = vtss_cmn_vce_add;
    func->vce_del = vtss_cmn_vce_del;
#endif /* VTSS_FEATURE_VCL */
#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
    func->vlan_trans_group_add = vtss_cmn_vlan_trans_group_add;
    func->vlan_trans_group_del = vtss_cmn_vlan_trans_group_del;
    func->vlan_trans_group_get = vtss_cmn_vlan_trans_group_get;
    func->vlan_trans_port_conf_set  = vtss_cmn_vlan_trans_port_conf_set;
    func->vlan_trans_port_conf_get  = vtss_cmn_vlan_trans_port_conf_get;
#endif /* VTSS_FEATURE_VLAN_TRANSLATION */
#ifdef VTSS_FEATURE_EEE
    /* EEE */
    func->eee_port_conf_set    = jr_eee_port_conf_set;
    func->eee_port_state_set   = jr_eee_port_state_set;
    func->eee_port_counter_get = jr_eee_port_counter_get;
#endif
#if defined(VTSS_FEATURE_EVC)
    func->evc_port_conf_set = jr_evc_port_conf_set;
    func->evc_policer_conf_set = jr_evc_policer_conf_set;
    func->evc_add = vtss_cmn_evc_add;
    func->evc_del = vtss_cmn_evc_del;
    func->ece_add = vtss_cmn_ece_add;
    func->ece_del= vtss_cmn_ece_del;
    func->ece_counters_get = vtss_cmn_ece_counters_get;
    func->ece_counters_clear = vtss_cmn_ece_counters_clear;
    func->evc_counters_get = vtss_cmn_evc_counters_get;
    func->evc_counters_clear = vtss_cmn_evc_counters_clear;
    func->sdx_counters_update = jr_sdx_counters_update;
    func->ece_update = jr_ece_update;
    func->evc_update = jr_evc_update;
#endif /* VTSS_FEATURE_EVC */

#if defined (VTSS_FEATURE_SFLOW)
    func->sflow_port_conf_set         = jr_sflow_port_conf_set;
    func->sflow_sampling_rate_convert = jr_sflow_sampling_rate_convert;
#endif /* VTSS_FEATURE_SFLOW */

#if defined(VTSS_FEATURE_TIMESTAMP)
    /* Time stamping features */
    func->ts_timeofday_get = jr_ts_timeofday_get;
    func->ts_timeofday_next_pps_get = jr_ts_timeofday_next_pps_get;
    func->ts_timeofday_set = jr_ts_timeofday_set;
    func->ts_timeofday_set_delta = jr_ts_timeofday_set_delta;
    func->ts_timeofday_offset_set = jr_ts_timeofday_offset_set;
    func->ts_ns_cnt_get = jr_ts_ns_cnt_get;
    func->ts_timeofday_one_sec = jr_ts_timeofday_one_sec;
    func->ts_adjtimer_set = jr_ts_adjtimer_set;
    func->ts_freq_offset_get = jr_ts_freq_offset_get;
    func->ts_external_clock_mode_set = jr_ts_external_clock_mode_set;
    func->ts_timestamp_get = jr_ts_timestamp_get;
    func->ts_timestamp_convert = jr_ts_convert_timestamp;
    func->ts_timestamp_id_release = jr_ts_timestamp_id_release;
    
#endif /* VTSS_FEATURE_TIMESTAMP */
    
    /* SYNCE features */
#if defined(VTSS_FEATURE_SYNCE)
    func->synce_clock_out_set = jr_synce_clock_out_set;
    func->synce_clock_in_set = jr_synce_clock_in_set;
#endif /* VTSS_FEATURE_SYNCE */

#if defined(VTSS_FEATURE_VLAN_COUNTERS)
    func->vlan_counters_get = jr_vlan_counters_get;
    func->vlan_counters_clear = jr_vlan_counters_clear;
#endif /* VTSS_FEATURE_VLAN_COUNTERS */

    /* State data */
    vtss_state->gpio_count = JR_GPIOS;
    vtss_state->sgpio_group_count = JR_SGPIO_GROUPS;
    vtss_state->prio_count = JR_PRIOS;
    vtss_state->rx_queue_count = JR_PACKET_RX_QUEUE_CNT;
    vtss_state->ac_count = JR_ACS;

#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
#if VTSS_OPT_FDMA_VER > 2
    /* The FDMA needs a special version of the tx_hdr_encode() function,
     * which can also return the injection group. */
    vtss_state->fdma_state.tx_hdr_encode_internal = jr_tx_hdr_encode_internal;
#endif
    vcoreiii_fdma_func_init();
#endif

#if defined(VTSS_FEATURE_WIS)
    if (vtss_phy_inst_ewis_create() != VTSS_RC_OK) {
    }
#endif

    func->l3_common_set = jr_l3_common_set;
    func->l3_rleg_counters_get = jr_l3_rleg_hw_stat_poll;
    func->l3_rleg_counters_reset = jr_l3_rleg_stat_reset;
    func->l3_rleg_set = jr_l3_rleg_set;
    func->l3_vlan_set = jr_l3_vlan_set;
    func->l3_arp_set = jr_l3_arp_set;
    func->l3_arp_clear = jr_l3_arp_clear;
    func->l3_ipv4_uc_set = jr_l3_ipv4_uc_set;
    func->l3_ipv6_uc_set = jr_l3_ipv6_uc_set;
    func->l3_ipv4_uc_clear = jr_l3_ipv4_uc_clear;
    func->l3_ipv4_mc_clear = jr_l3_ipv4_mc_clear;
    func->l3_ipv6_uc_clear = jr_l3_ipv6_uc_clear;
    func->l3_lpm_move = jr_l3_lpm_move;
    func->l3_debug_sticky_clear = jr_l3_debug_sticky_clear;

    return VTSS_RC_OK;
}

#endif /* VTSS_ARCH_JAGUAR_1 */

