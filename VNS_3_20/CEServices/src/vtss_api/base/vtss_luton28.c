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

#define VTSS_TRACE_LAYER VTSS_TRACE_LAYER_CIL

// Avoid "vtss_api.h not used in module vtss_luton28.c"
/*lint --e{766} */

#include "vtss_api.h"
#include "vtss_state.h"
#include "vtss_common.h"
#include "vtss_luton28_reg.h"
#include "vtss_luton28_qs.h"
#include "vtss_luton28_fdma.h"
#include "vtss_luton28.h"
#include "vtss_util.h"
#if defined(VTSS_ARCH_LUTON28)

/* Chip ports */
#define VTSS_CHIP_PORTS           28
#define VTSS_CHIP_PORT_CPU        VTSS_CHIP_PORTS
#define VTSS_CHIP_PORT_MASK       ((1U<<VTSS_CHIP_PORTS)-1)
#define VTSS_CHIP_PORT_AGGR_0     24 /* Chip port 24 and 25 are paired */
#define VTSS_CHIP_PORT_AGGR_1     26 /* Chip port 26 and 27 are paired */
#define VTSS_CHIP_PORT_VAUI_START 24 /* First VAUI port */
#define VTSS_CHIP_MASK_UPDATE     (1U<<31) /* Illegal mask, used for forcing VLAN update */

/* NPI port */
#if VTSS_OPT_INT_AGGR
#define VTSS_CHIP_PORT_NPI VTSS_CHIP_PORT_AGGR_1
#else
#define VTSS_CHIP_PORT_NPI (VTSS_CHIP_PORT_AGGR_1 + 1)
#endif

#define L28_ACS   16  /* Number of aggregation codes */
#define L28_PRIOS 4   /* Number of priorities */
#define L28_PRIO_END (VTSS_PRIO_START + L28_PRIOS) /* Luton 28 priority end number */

/* Frame ageing timer */
#define HT_TIMECMP_DEFAULT 0x3ffffff

#define HT_RD(blk, sub, reg, value) \
{ \
    vtss_rc rc; \
    if ((rc = ht_rd_wr(B_##blk, sub, R_##blk##_##reg, value, 0)) != VTSS_RC_OK) \
        return rc; \
}

#define HT_WR(blk, sub, reg, value) \
{ \
    vtss_rc rc; \
    u32     val = value; \
    if ((rc = ht_rd_wr(B_##blk, sub, R_##blk##_##reg, &val, 1)) != VTSS_RC_OK) \
        return rc; \
}

#define HT_WRM(blk, sub, reg, value, mask) \
{ \
    vtss_rc rc; \
    if ((rc = ht_wrm(B_##blk, sub, R_##blk##_##reg, value, mask)) != VTSS_RC_OK) \
        return rc; \
}

#define HT_RDF(blk, sub, reg, offset, mask, value) \
{ \
    vtss_rc rc; \
    if ((rc = ht_rdf(B_##blk, sub, R_##blk##_##reg, offset, mask, value)) != VTSS_RC_OK) \
        return rc; \
}

#define HT_WRF(blk, sub, reg, offset, mask, value) \
{ \
    vtss_rc rc; \
    if ((rc = ht_wrf(B_##blk, sub, R_##blk##_##reg, offset, mask, value)) != VTSS_RC_OK) \
        return rc; \
}

/* A few forward declarations are needed */
static vtss_rc l28_vcap_range_commit(void);
static vtss_rc l28_port_conf_set(const vtss_port_no_t port_no);
static vtss_rc l28_qos_port_conf_set(const vtss_port_no_t port_no);

/* ================================================================= *
 *  Register access and utilitties
 * ================================================================= */

/* Read target register using current CPU interface */
static vtss_rc ht_rd_wr(u32 blk, u32 sub, u32 reg, u32 *value, u8 write)
{
    vtss_rc         rc;
    u8              error;
    u32             addr, val, i;
    vtss_reg_read_t read_func;

    switch (blk) {
    case B_PORT:
        /* By default, it is an error if the sub block is not included in chip port mask */
        error = (sub > VTSS_CHIP_PORT_CPU);
        if (error)
            break;
        if (sub >= 16) {
            blk = B_PORT_HI;
            sub -= 16;
        }
        break;
    case B_MIIM: /* B_MEMINIT/B_ACL/B_VAUI */
        error = (sub > S_VAUI);
        break;
    case B_CAPTURE:
        error = 0;
        break;
    case B_ANALYZER:
    case B_ARBITER:
    case B_SYSTEM:
        error = (sub != 0);
        break;
    case B_PORT_HI:
        error = 0;
        break;
    default:
        error = 1;
        break;
    }

    if (error) {
        VTSS_E("illegal access, blk: 0x%02x, sub: 0x%02x, reg: 0x%02x", blk, sub, reg);
        return VTSS_RC_ERROR;
    }

    addr = ((blk<<12) | (sub<<8) | reg);
    if (write) {
        /* Write operation */
        rc = vtss_state->init_conf.reg_write(0, addr, *value);
    } else {
        read_func = vtss_state->init_conf.reg_read;
        if (vtss_state->init_conf.pi.use_extended_bus_cycle ||
            blk == B_CAPTURE || blk == B_SYSTEM) {
            /* Direct read operation */
            rc = read_func(0, addr, value);
        } else {
            /* Indirect read operation */
            VTSS_RC(read_func(0, addr, &val));
            for (i = 0; ; i++) {
                VTSS_RC(read_func(0, (B_SYSTEM<<12) | R_SYSTEM_CPUCTRL, &val));
                if (val & (1<<4)) {
                    rc = read_func(0, (B_SYSTEM<<12) | R_SYSTEM_SLOWDATA, value);
                    break;
                }
                if (i == 25) {
                    VTSS_E("blk: 0x%02x, sub: 0x%02x, reg: 0x%02x, failed", blk, sub, reg);
                    rc = VTSS_RC_ERROR;
                    break;
                }
            }
        }
    }
    return rc;
}

#if 0
/* Read target register using current CPU interface */
static vtss_rc ht_rd(u32 blk, u32 sub, u32 reg, u32 *value)
{
    return ht_rd_wr(blk, sub, reg, value, 0);
}

/* Write target register using current CPU interface */
static vtss_rc ht_wr(u32 blk, u32 sub, u32 reg, u32 value)
{
    return ht_rd_wr(blk, sub, reg, &value, 1);
}
#endif

/* Read-modify-write target register using current CPU interface */
static vtss_rc ht_wrm(u32 blk, u32 sub, u32 reg, u32 value, u32 mask)
{
    vtss_rc rc;
    u32     val;

    if ((rc = ht_rd_wr(blk, sub, reg, &val, 0))>=0) {
        val = ((val & ~mask) | (value & mask));
        rc = ht_rd_wr(blk, sub, reg, &val, 1);
    }
    return rc;
}

/* Read target register field using current CPU interface */
static vtss_rc ht_rdf(u32 blk, u32 sub, u32 reg, u32 offset, u32 mask, u32 *value)
{
    vtss_rc rc;
    
    rc = ht_rd_wr(blk, sub, reg, value, 0);
    *value = ((*value >> offset) & mask);
    return rc;
}

/* Read-modify-write target register field using current CPU interface */
static vtss_rc ht_wrf(u32 blk, u32 sub, u32 reg, u32 offset, u32 mask, u32 value)
{
    return ht_wrm(blk, sub, reg, value<<offset, mask<<offset);
}


/* Determine if chip port is internally aggregated */
static u32 l28_int_aggr_port(u32 port)
{
#if VTSS_OPT_INT_AGGR
    return (port == VTSS_CHIP_PORT_AGGR_0 || port == VTSS_CHIP_PORT_AGGR_1 ? (port + 1) : 0);
#else
    return 0;
#endif /* VTSS_OPT_INT_AGGR */
}

static u32 l28_port_mask(const BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_port_no_t port_no;
    u32            port, mask = 0;
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (member[port_no]) {
            port = VTSS_CHIP_PORT(port_no);
            mask |= (1<<port);
            /* Enable internally aggregated port */
            if ((port = l28_int_aggr_port(port)) != 0)
                mask |= (1<<port);
        }
    }
    return mask;
}

/* Convert to PGID in chip */
static u32 l28_chip_pgid(u32 pgid)
{
    if (pgid < vtss_state->port_count) {
        return VTSS_CHIP_PORT(pgid);
    } else {
        return (pgid + VTSS_CHIP_PORTS - vtss_state->port_count);
    }
}

/* Convert from PGID in chip */
static u32 l28_vtss_pgid(const vtss_state_t *const state, u32 pgid)
{
    vtss_port_no_t port_no;
    
    if (pgid < VTSS_CHIP_PORTS) {
        for (port_no = VTSS_PORT_NO_START; port_no < state->port_count; port_no++)
            if (VTSS_CHIP_PORT_FROM_STATE(state, port_no) == pgid)
                break;
        return port_no;
    } else {
        return (state->port_count + pgid - VTSS_CHIP_PORTS);
    }
}

/* ================================================================= *
 *  Miscellaneous 
 * ================================================================= */

static vtss_rc l28_rd_wr(u32 addr, u32 *value, u8 write)
{
    return ht_rd_wr(addr>>12, (addr>>8) & 0xf, addr & 0xff, value, write);
}

static vtss_rc l28_reg_read(const vtss_chip_no_t chip_no, const u32 addr, u32 * const value)
{
    return l28_rd_wr(addr, value, 0);
}

static vtss_rc l28_reg_write(const vtss_chip_no_t chip_no, const u32 addr, const u32 value)
{
    u32 val = value;
    return l28_rd_wr(addr, &val, 1);
}

static vtss_rc l28_chip_id_get(vtss_chip_id_t *const chip_id)
{
    u32 value;
    
    HT_RD(SYSTEM, 0, CHIPID, &value);

    if (value == 0 || value == 0xffffffff) {
        VTSS_E("CPU interface error, chipid: 0x%08x", value);
        return VTSS_RC_ERROR;
    }
    
    chip_id->part_number = ((value>>12) & 0xffff);
    chip_id->revision = ((value>>28) & 0xf);
    
    return VTSS_RC_OK;
}

static vtss_rc l28_poll_1sec(void)
{
    vtss_port_no_t   port_no;
    vtss_port_conf_t *conf;
    u32              port, value;
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        /* Skip ports not running 10fdx */
        conf = &vtss_state->port_conf[port_no];
        if (conf->fdx || conf->speed != VTSS_SPEED_10M)
            continue;
        
        /* Read Tx packet counter, skip port if changed */
        port = VTSS_CHIP_PORT(port_no);
        HT_WR(PORT, port, C_CNTADDR, (1<<31) | (0x3042<<0));
        HT_RD(PORT, port, C_CNTDATA, &value);
        if (vtss_state->port_tx_packets[port_no] != value) {
            vtss_state->port_tx_packets[port_no] = value;
            continue;
        }
        
        /* Read egress FIFO used slices */
        HT_RD(PORT, port, FREEPOOL, &value);
        HT_RDF(PORT, port, FREEPOOL, 0, 0x1f, &value);
        if (value == 0)
            continue;

        /* No Tx activity and FIFO not empty, restart port */
        VTSS_D("optimizing port %u", port_no);
        VTSS_RC(l28_port_conf_set(port_no));
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_gpio_mode(const vtss_chip_no_t   chip_no,
                             const vtss_gpio_no_t   gpio_no,
                             const vtss_gpio_mode_t mode)
{
    switch(mode) {
    case VTSS_GPIO_OUT:
    case VTSS_GPIO_IN:
        HT_WRF(SYSTEM, 0, GPIO, 16 + gpio_no, 0x1, mode == VTSS_GPIO_OUT ? 1 : 0);
        return VTSS_RC_OK;
    case VTSS_GPIO_ALT_0:
        HT_WRF(SYSTEM, 0, GPIOCTRL, gpio_no, 0x1, 0x1);
        return VTSS_RC_OK;
    default:
        return VTSS_RC_ERROR;       /* Other modes not supported */
    }
}

static vtss_rc l28_gpio_read(const vtss_chip_no_t  chip_no,
                             const vtss_gpio_no_t  gpio_no,
                             BOOL                  *const value)
{
    u32 val;

    HT_RDF(SYSTEM, 0, GPIO, gpio_no, 0x1, &val);
    *value = VTSS_BOOL(val);
    return VTSS_RC_OK;
}

static vtss_rc l28_gpio_write(const vtss_chip_no_t  chip_no,
                              const vtss_gpio_no_t  gpio_no, 
                              const BOOL            value)
{
    HT_WRF(SYSTEM, 0, GPIO, gpio_no, 0x1, value ? 1 : 0);
    return VTSS_RC_OK;
}

static vtss_rc l28_serial_led_set(const vtss_led_port_t port, 
                                  const vtss_led_mode_t mode[3])
{
    u32 value, val, i;
    
    if (port > 29) {
        VTSS_E("illegal port: %d", port);
        return VTSS_RC_ERROR;
    }

    /* Set shared LED/GPIO pins to LED mode */
    HT_WRM(SYSTEM, 0, GPIOCTRL, (0<<11) | (0<<10), (1<<11) | (1<<10));
    
    /* Select port */
    HT_WR(SYSTEM, 0, LEDTIMER, (0<<31) | (port<<26) | (178<<16) | (25000<<0));
    
    /* Setup LED mode */
    HT_RD(SYSTEM, 0, LEDMODES, &value);
    for (i = 0; i < 3; i++) {
        switch (mode[i]) {
        case VTSS_LED_MODE_IGNORE:
            continue;
            break;
        case VTSS_LED_MODE_DISABLED:
            val = 7;
            break;
        case VTSS_LED_MODE_OFF:
            val = 0;
            break;
        case VTSS_LED_MODE_ON:
            val = 1;
            break;
        case VTSS_LED_MODE_2_5:
            val = 2;
            break;
        case VTSS_LED_MODE_5:
            val = 3;
            break;
        case VTSS_LED_MODE_10:
            val = 4;
            break;
        case VTSS_LED_MODE_20:
            val = 5;
            break;
        default:
            val = 0;
            break;
        }
        value &= ~(1<<(3*i));  // Clear the bit in question
        value |= (val<<(3*i)); // Set the bit in question according to val.

    }
    HT_WR(SYSTEM, 0, LEDMODES, value);

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Port control
 * ================================================================= */

static vtss_rc ht_miim_read_write(BOOL read, u32 miim_controller, 
                                  u8 miim_addr, u8 addr, u16 *value)
{
    u32 data;

    /* Enqueue MIIM operation to be executed */
    HT_WR(MIIM, miim_controller, MIIMCMD,
          ((read ? 1 : 0)<<26) | (miim_addr<<21) | (addr<<16) | *value);

    /* Wait for MIIM operation to finish */
    while (1) {
        HT_RD(MIIM, miim_controller, MIIMSTAT, &data);
        if (!(data & 0xF)) /* Include undocumented "pending" bit */
            break;
    }
    if (read) {
        HT_RD(MIIM, miim_controller, MIIMDATA, &data);
        if (data & (1<<16))
            return VTSS_RC_ERROR;
        *value = (data & 0xFFFF);
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_miim_read(vtss_miim_controller_t miim_controller, 
                             u8 miim_addr, 
                             u8 addr, 
                             u16 *value, 
                             BOOL report_errors)
{
    return ht_miim_read_write(1, miim_controller, miim_addr, addr, value);
}

static vtss_rc l28_miim_write(vtss_miim_controller_t miim_controller, 
                              u8 miim_addr, 
                              u8 addr, 
                              u16 value, 
                              BOOL report_errors)
{
    return ht_miim_read_write(0, miim_controller, miim_addr, addr, &value);
}

static vtss_rc l28_port_map_set(void)
{
    u32               pgid, pgid_glag_dest = (vtss_state->pgid_flood + 1);
    vtss_pgid_entry_t *pgid_entry;

    vtss_state->pgid_count = (VTSS_PGID_LUTON28 - VTSS_CHIP_PORTS + vtss_state->port_count);
    vtss_state->pgid_glag_dest   = pgid_glag_dest;       /* 30-31: Dest. masks */
    vtss_state->pgid_glag_src    = (pgid_glag_dest + 2); /* 32-33: Src. masks */
    vtss_state->pgid_glag_aggr_a = (pgid_glag_dest + 4); /* 34-35: Aggr. A masks */
    vtss_state->pgid_glag_aggr_b = (pgid_glag_dest + 6); /* 36-37: Aggr. B masks */

    /* Reserve 8 entries for GLAG support */
    for (pgid = pgid_glag_dest; pgid < (pgid_glag_dest + 8); pgid++) {
        pgid_entry = &vtss_state->pgid_table[pgid];
        pgid_entry->resv = 1;
        pgid_entry->references = 1;
    }
    
    return VTSS_RC_OK;
}

static vtss_rc ht_pcs_ctrl_wrm(u32 port, u32 value, u32 mask)
{
    HT_WRM(PORT, port, PCSCTRL, value, mask);

    if (value & mask & (1<<16)) {
        /* Disable ANEG restart bit again */
        HT_WRM(PORT, port, PCSCTRL, 0, 1<<16);
    }

    return VTSS_RC_OK;
}

static vtss_rc ht_port_clause_37_control_set(
    u32                                 port,
    const vtss_port_clause_37_control_t *const control)
{
    u32                             value;
    const vtss_port_clause_37_adv_t *adv = &control->advertisement;
    
    value = ((1<<16) | /* Always restart autonegotiation */
             ((adv->next_page ? 1 : 0)<<15) |
             ((adv->acknowledge ? 1 : 0)<<14) |
             (adv->remote_fault<<12) |
             ((adv->asymmetric_pause ? 1 : 0)<<8) |
             ((adv->symmetric_pause ? 1 : 0)<<7) |
             ((adv->hdx ? 1 : 0)<<6) |
             ((adv->fdx ? 1 : 0)<<5));
    
    if (!control->enable) {
        /* Restart aneg before disabling it */
        VTSS_RC(ht_pcs_ctrl_wrm(port, (1<<17) | value, 0x3ffff));
    }
    return ht_pcs_ctrl_wrm(port, ((control->enable ? 1 : 0)<<17) | value, 0x3ffff);
}

static vtss_rc l28_port_clause_37_control_get(
    const vtss_port_no_t          port_no, 
    vtss_port_clause_37_control_t *const control)
{
    u32                       value, port = VTSS_CHIP_PORT(port_no);
    vtss_port_clause_37_adv_t *adv = &control->advertisement;
    
    HT_RD(PORT, port, PCSCTRL, &value);
    control->enable = (value & (1<<17) ? 1 : 0);
    adv->next_page = (value & (1<<15) ? 1 : 0);
    adv->acknowledge = (value & (1<<14) ? 1 : 0);
    adv->remote_fault = ((value >> 12) & 0x3);
    adv->asymmetric_pause = (value & (1<<8) ? 1 : 0);
    adv->symmetric_pause = (value & (1<<7) ? 1 : 0);
    adv->hdx = (value & (1<<6) ? 1 : 0);
    adv->fdx = (value & (1<<5) ? 1 : 0);

    return VTSS_RC_OK;
}

static vtss_rc l28_port_clause_37_status_get(const vtss_port_no_t         port_no, 
                                             vtss_port_clause_37_status_t *const status)

{
    u32                       value, port = VTSS_CHIP_PORT(port_no);
    vtss_port_clause_37_adv_t *lp = &status->autoneg.partner_advertisement;

    if (vtss_state->port_conf[port_no].power_down) {
        status->link = 0;
        return VTSS_RC_OK;
    }

    HT_RD(PORT, port, PCSSTAT, &value);
    status->link = (((value>>18) & 0x3) < 2 ? 0 : VTSS_BOOL(value & (1<<20)));
    status->autoneg.complete = VTSS_BOOL(value & (1<<16));
    lp->fdx = VTSS_BOOL(value & (1<<5));
    lp->hdx = VTSS_BOOL(value & (1<<6));
    lp->symmetric_pause = VTSS_BOOL(value & (1<<7));
    lp->asymmetric_pause = VTSS_BOOL(value & (1<<8));
    lp->remote_fault = (value>>12) & 0x3;
    lp->acknowledge = VTSS_BOOL(value & (1<<14));
    lp->next_page = VTSS_BOOL(value & (1<<15));

    return VTSS_RC_OK;
}

static vtss_rc l28_port_clause_37_control_set(const vtss_port_no_t port_no)
{
    return ht_port_clause_37_control_set(VTSS_CHIP_PORT(port_no), 
                                         &vtss_state->port_clause_37[port_no]);
}

static vtss_rc l28_port_status_get(const vtss_port_no_t  port_no, 
                                   vtss_port_status_t    *const status)
{
    u32              value, port = VTSS_CHIP_PORT(port_no);
    vtss_port_conf_t *conf = &vtss_state->port_conf[port_no];
    
    /* Only VAUI port status supported */
    if (conf->if_type != VTSS_PORT_INTERFACE_VAUI || port < VTSS_CHIP_PORT_VAUI_START) {
        return VTSS_RC_ERROR;
    }
    
    /* Read link status */
    HT_RDF(PORT, port, PCSSTAT, 20, 0x1, &value);
    status->link_down = (value ? 0 : 1);
    if (status->link_down) {
        HT_RDF(PORT, port, PCSSTAT, 20, 0x1, &value);
        status->link = (value ? 1 : 0);
    } else {
        status->link = 1;
    }

    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Read aggregated port status */
        HT_RDF(PORT, port, PCSSTAT, 20, 0x1, &value);
        if (!value) {
            status->link_down = 1;
            HT_RDF(PORT, port, PCSSTAT, 20, 0x1, &value);
            if (!value)
                status->link = 0;
        } 
    }
    
    /* Read configured speed */
    status->speed = conf->speed;
    status->fdx = conf->fdx;

    return VTSS_RC_OK;
}
                                   

#define MCNF(mode) \
((VTSS_NORM_##mode##_IMIN<<16) | \
 (VTSS_NORM_##mode##_EMIN<<8) | \
 (VTSS_NORM_##mode##_EARLY_TX<<1))

#define EWM(mode) \
((VTSS_NORM_##mode##_EMAX3<<24) | \
 (VTSS_NORM_##mode##_EMAX2<<16) | \
 (VTSS_NORM_##mode##_EMAX1<<8) | \
 (VTSS_NORM_##mode##_EMAX0<<0))

#define IWM(mode, i) \
((VTSS_NORM_##mode##_LOWONLY_##i<<18) | \
 (VTSS_NORM_##mode##_QMASK_##i<<13) | \
 (VTSS_NORM_##mode##_ACTION_##i<<11) | \
 (VTSS_NORM_##mode##_CHKMIN_##i<<10) | \
 (VTSS_NORM_##mode##_CMPWITH_##i<<8) | \
 (VTSS_NORM_##mode##_LEVEL_##i<<0))

/* Setup water marks, drop levels, etc for port */
static vtss_rc ht_port_queue_setup(u32                         port,
                            const vtss_port_conf_t      *const conf,
                            const vtss_qos_port_conf_t  *const qos)
{
    u32      zero_pause, pause_value, mcnf, ewm, iwm[6], i;
    const u8 *mac;
    BOOL     policer = (qos->policer_port[0].rate != VTSS_BITRATE_DISABLED);

    if (conf->flow_control.generate) {
        /* Flow control mode */
        zero_pause = VTSS_NORM_4S_FC_ZEROPAUSE;   
        pause_value = VTSS_NORM_4S_FC_PAUSEVALUE;   
        mcnf = MCNF(4S_FC);
        ewm = EWM(4S_FC);
        iwm[0] = IWM(4S_FC, 0);
        iwm[1] = IWM(4S_FC, 1);
        iwm[2] = IWM(4S_FC, 2);
        iwm[3] = IWM(4S_FC, 3);
        iwm[4] = IWM(4S_FC, 4);
        iwm[5] = IWM(4S_FC, 5);
    } else if (vtss_state->wfq) {
        /* Weighted drop mode */
        zero_pause = VTSS_NORM_4W_DROP_ZEROPAUSE; 
        pause_value = VTSS_NORM_4W_DROP_PAUSEVALUE; 
        mcnf = MCNF(4W_DROP);
        ewm = EWM(4W_DROP);
        iwm[0] = IWM(4W_DROP, 0);
        iwm[1] = IWM(4W_DROP, 1);
        iwm[2] = IWM(4W_DROP, 2);
        iwm[3] = IWM(4W_DROP, 3);
        iwm[4] = IWM(4W_DROP, 4);
        iwm[5] = IWM(4W_DROP, 5);
        if (policer) {
            /* Special settings if policer enabled */
            iwm[0] = ((iwm[0] & 0xffffffe0) | (VTSS_NORM_4W_POLICER_LEVEL_0<<0));
            iwm[5] = ((iwm[5] & 0xffffffe0) | (VTSS_NORM_4W_POLICER_LEVEL_5<<0));  
        }
    } else {
        /* Normal drop mode */
        zero_pause = VTSS_NORM_4S_DROP_ZEROPAUSE; 
        pause_value = VTSS_NORM_4S_DROP_PAUSEVALUE; 
        mcnf = MCNF(4S_DROP);
        ewm = EWM(4S_DROP);
        iwm[0] = IWM(4S_DROP, 0);
        iwm[1] = IWM(4S_DROP, 1);
        iwm[2] = IWM(4S_DROP, 2);
        iwm[3] = IWM(4S_DROP, 3);
        iwm[4] = IWM(4S_DROP, 4);
        iwm[5] = IWM(4S_DROP, 5);
        if (policer) {
            /* Special settings if policer enabled */
            iwm[0] = ((iwm[0] & 0xffffffe0) | (VTSS_NORM_4S_POLICER_LEVEL_0<<0));
            iwm[5] = ((iwm[5] & 0xffffffe0) | (VTSS_NORM_4S_POLICER_LEVEL_5<<0));  
        }
    }

    if (conf->speed == VTSS_SPEED_10M && conf->fdx == 0) {
        /* Special settings for 10 Mbps half duplex operation */
        mcnf = ((mcnf & ((0x1f<<16) | (0xf<<1))) | (VTSS_NORM_10_HDX_EMIN<<8));
        ewm = EWM(10_HDX);
    }

    /* Setup flow control registers */
    mac = &conf->flow_control.smac.addr[0];
    HT_WR(PORT, port, FCMACHI, (mac[0]<<16) | (mac[1]<<8) | mac[2]);
    HT_WR(PORT, port, FCMACLO, (mac[3]<<16) | (mac[4]<<8) | mac[5]);
    HT_WR(PORT, port, FCCONF,
          ((zero_pause ? 1 : 0)<<17) | 
          ((conf->flow_control.obey ? 1 : 0)<<16) | 
          (pause_value<<0));
    
    /* Setup queue system drop levels and water marks */
    HT_WR(PORT, port, Q_MISC_CONF, mcnf);
    HT_WR(PORT, port, Q_EGRESS_WM, ewm);
        
    /* Ingress watermarks */
    for (i = 0; i < 6; i++) {
        HT_WR(PORT, port, Q_INGRESS_WM + i, iwm[i]);
    }
    return VTSS_RC_OK;
}

static vtss_rc ht_port_queue_enable(u32 port, BOOL enable)
{
    u32 value;

    VTSS_D("port: %u, enable: %d", port, enable);

    /* Enable/disable Arbiter discard */
    HT_WRF(ARBITER, 0, ARBDISC, port, 0x1, enable ? 0 : 1);

    /* Enable/disable MAC Rx/Tx */
    value = ((1<<28) | (1<<16));
    HT_WRM(PORT, port, MAC_CFG, enable ? value : 0, value);

    return VTSS_RC_OK;
}

static vtss_rc ht_port_conf_set(u32                   port, 
                                vtss_port_conf_t      *conf, 
                                vtss_qos_port_conf_t  *qos,
                                BOOL                  vstax)
{
    u32                    tx_clock, mac_cfg, value, ac, aggr_masks[L28_ACS];
    u32                    hdx_late_col_pos, count;
    vtss_port_interface_t  if_type = conf->if_type;
    BOOL                   loopback = (if_type == VTSS_PORT_INTERFACE_LOOPBACK);
    BOOL                   serdes = (if_type == VTSS_PORT_INTERFACE_SERDES);
    BOOL                   sgmii = (if_type == VTSS_PORT_INTERFACE_SGMII);
    vtss_port_frame_gaps_t gaps;
    vtss_port_speed_t      speed = conf->speed;
    
    /* Determine interframe gaps and default clock selection based on speed only */
    gaps.hdx_gap_1   = 0;
    gaps.hdx_gap_2   = 0;
    gaps.fdx_gap     = 0;
    hdx_late_col_pos = 0;

    switch (speed) {
    case VTSS_SPEED_10M:
    case VTSS_SPEED_100M:
        tx_clock = (speed == VTSS_SPEED_10M ? 3 : 2);
        gaps.hdx_gap_1   = 6;
        gaps.hdx_gap_2   = 8;
        gaps.fdx_gap     = 17;
        hdx_late_col_pos = 2;
        break;
    case VTSS_SPEED_1G:
    case VTSS_SPEED_2500M:
    case VTSS_SPEED_5G:
        tx_clock     = 1;
        gaps.fdx_gap = (speed == VTSS_SPEED_1G ? 6 : 2); /* VAUI IFG of 8 bytes */
        break;
    default:
        VTSS_E("illegal speed, port %u", port);
        return VTSS_RC_ERROR;
    }

    /* Check interface type and determine clock selection */
    switch (if_type) {
    case VTSS_PORT_INTERFACE_NO_CONNECTION:
        tx_clock = 0;
        break;
    case VTSS_PORT_INTERFACE_SGMII:
    case VTSS_PORT_INTERFACE_LOOPBACK:
        if (speed != VTSS_SPEED_10M && speed != VTSS_SPEED_100M && speed != VTSS_SPEED_1G) {
            VTSS_E("illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        break;
    case VTSS_PORT_INTERFACE_SERDES:
        if (speed != VTSS_SPEED_1G) {
            VTSS_E("illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        break;
    case VTSS_PORT_INTERFACE_VAUI:
        if (speed != VTSS_SPEED_1G && speed != VTSS_SPEED_2500M && speed != VTSS_SPEED_5G) {
            VTSS_E("illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        break;
    default:
        VTSS_E("illegal interface, port %u", port);
        return VTSS_RC_ERROR;
    }

    /* Disable MAC Rx/Tx and let Arbiter discard frames from port */
    VTSS_RC(ht_port_queue_enable(port, 0));

    /* Read and remove port from aggregation masks to avoid forwarding to port */
    for (ac = 0; ac < L28_ACS; ac++) {
        HT_RD(ANALYZER, 0, AGGRMSKS + ac, &value);
        aggr_masks[ac] = value;
        HT_WR(ANALYZER, 0, AGGRMSKS + ac, value & ~(1<<port));
    }
    
    /* Wait until ingress queues are flushed */
    for (count = 1000000; count != 0; count--) {
        HT_RD(ARBITER, 0, ARBEMPTY, &value);
        if (value & (1<<port))
            break;
    }
    if (count == 0) {
        VTSS_E("arbiter not empty, port: %u", port);
        return VTSS_RC_ERROR;
    }
    
    /* Setup half duplex gaps */
    if (conf->frame_gaps.hdx_gap_1 != VTSS_FRAME_GAP_DEFAULT)
        gaps.hdx_gap_1 = conf->frame_gaps.hdx_gap_1;
    if (conf->frame_gaps.hdx_gap_2 != VTSS_FRAME_GAP_DEFAULT)
        gaps.hdx_gap_2 = conf->frame_gaps.hdx_gap_2;
    if (conf->frame_gaps.fdx_gap != VTSS_FRAME_GAP_DEFAULT)
        gaps.fdx_gap = conf->frame_gaps.fdx_gap;
    HT_WR(PORT, port, MACHDXGAP,
          (7<<16) |
          (8<<12) | 
          (hdx_late_col_pos<<8) |
          (gaps.hdx_gap_2<<4) |
          (gaps.hdx_gap_1<<0));

    mac_cfg = ((1<<27) |
               (conf->flow_control.smac.addr[5]<<19) |
               ((conf->fdx ? 1 : 0)<<18) |
               ((speed == VTSS_SPEED_10M || speed == VTSS_SPEED_100M ? 0 : 1)<<17) |
               ((conf->max_tags == VTSS_PORT_MAX_TAGS_TWO ? 1 : 0)<<15) | 
               ((conf->max_tags == VTSS_PORT_MAX_TAGS_NONE ? 0 : 1)<<14) |
               (gaps.fdx_gap<<6) |
               (tx_clock<<0));
    
    /* Reset MAC, but not PCS (could mess up auto-negotiation) */
    HT_WR(PORT, port, MAC_CFG, mac_cfg | (1<<29) | (1<<5) | (1<<4));
        
    /* Release MAC and PCS from Reset and set Seed_Load */
    HT_WR(PORT, port, MAC_CFG, mac_cfg);
    VTSS_NSLEEP(1000);

    /* Clear Seed_Load bit */
    HT_WR(PORT, port, MAC_CFG, mac_cfg & ~(1<<27));
    
    /* Restore aggregation masks */
    for (ac = 0; ac < L28_ACS; ac++) {
        HT_WR(ANALYZER, 0, AGGRMSKS + ac, aggr_masks[ac]);
    }

    /* Set Advanced Port Mode */
    HT_WR(PORT, port, ADVPORTM, (conf->exc_col_cont ? 1 : 0)<<6);
    
    /* Set PCS Control */
    VTSS_RC(ht_pcs_ctrl_wrm(port, 
                            ((conf->sd_internal ? 1 : 0)<<28) |
                            ((sgmii ? 1 : 0)<<25) |
                            ((sgmii ? 1 : 0)<<24) |
                            ((conf->sd_active_high ? 1 : 0)<<23) |
                            ((conf->sd_enable ? 1 : 0)<<22),
                            (1<<28) | (1<<25) | (1<<24) | (1<<23) | (1<<22)));
    
    /* Set SGMII_CFG */
    HT_WRM(PORT, port, SGMII_CFG,
           (1<<28) |
           ((loopback ? 1 : 0)<<27) |
           ((sgmii ? 1 : 0)<<26) | 
           (1<<25) |
           ((sgmii ? 0 : 1)<<23) |
           (0<<22) |
           ((loopback ? 1 : 0)<<9) |
           (1<<8) |
           (1<<0),
           (1<<28) | (1<<27) | (1<<26) | (1<<25) | (1<<23) | 
           (1<<22) | (1<<9) | (1<<8) | (1<<0));
    
    /* Setup VAUI lane */
    if (port >= VTSS_CHIP_PORT_VAUI_START) {
        HT_WR(VAUI, S_VAUI, LANE_CFG + (port - VTSS_CHIP_PORT_VAUI_START)*4,
              (2<<18) | (2<<10) |
              ((speed == VTSS_SPEED_2500M || speed == VTSS_SPEED_5G ? 0 : 1)<<2) |
              ((speed == VTSS_SPEED_2500M || speed == VTSS_SPEED_5G ? 0 : 1)<<1) |
              ((conf->power_down ? 0 : 1)<<0));
        HT_WR(VAUI, S_VAUI, IB_CFG + (port - VTSS_CHIP_PORT_VAUI_START)*4,
              (1<<7) | (0<<6) | (1<<4) | (VTSS_OPT_VAUI_EQ_CTRL<<0));
    }

    if (conf->power_down) {
        if (serdes || loopback) {
            /* Enable (and restart) autonegotiation.
               This will send a Link Break signal before autonegotiation restarts. */
            vtss_port_clause_37_control_t control;

            memset(&control, 0, sizeof(control));
            control.enable = 1;
            VTSS_RC(ht_port_clause_37_control_set(port, &control));
            /* Abort the autonegotiation before it completes */
            control.enable = 0;
            VTSS_RC(ht_port_clause_37_control_set(port, &control));
        }
            
        /* Power down MAC and PCS */
        value = ((1<<5) | (1<<4) | (1<<3) | (1<<2));
        HT_WRM(PORT, port, MAC_CFG, value, value);
        
        /* Power down SGMII SerDes */
        HT_WRM(PORT, port, SGMII_CFG, 0, (1<<28) | (1<<25) | (1<<8) | (1<<0));
    }

    HT_WR(PORT, port, MAXLEN, 
          vstax ? (VTSS_MAX_FRAME_LENGTH_MAX + VTSS_VSTAX_HDR_SIZE) : conf->max_frame_length);

    /* Enable MAC */
    VTSS_RC(ht_port_queue_enable(port, 1));

    return VTSS_RC_OK;
}

static vtss_rc l28_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_port_conf_t     *conf = &vtss_state->port_conf[port_no];
    u32                  timecmp, holb, port = VTSS_CHIP_PORT(port_no);
    BOOL                 vstax = vtss_state->vstax_port_enabled[port_no];
    vtss_qos_port_conf_t *qos = &vtss_state->qos_port_conf[port_no];
    vtss_port_conf_t     *pc;

    /* Setup port */
    VTSS_RC(ht_port_conf_set(port, conf, qos, vstax));

    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Setup aggregated port */
        VTSS_RC(ht_port_conf_set(port, conf, qos, vstax));
    }

    /* Setup QoS as policer/shaper rate granularity depend on port speed */
    VTSS_RC(l28_qos_port_conf_set(port_no));

    /* Setup frame ageing and HOLB */
    timecmp = HT_TIMECMP_DEFAULT;
    holb = VTSS_CHIP_PORT_MASK;
    for (port = VTSS_PORT_NO_START; port < vtss_state->port_count; port++) {
        pc = &vtss_state->port_conf[port];

        /* Disable ageing if any half duplex port is using excessive collision continuation */
        if (pc->exc_col_cont && !pc->fdx)
            timecmp = 0;

        /* Disable HOLB if any port is using flow control */
        if (pc->flow_control.obey || pc->flow_control.generate)
            holb = 0;
    }
    HT_WR(SYSTEM, 0, TIMECMP, timecmp);
    HT_WR(ARBITER, 0, ARBHOLBPENAB, holb);

    return VTSS_RC_OK;
}

static vtss_rc l28_port_conf_get(const vtss_port_no_t port_no,
                                 vtss_port_conf_t *const conf)
{
    u32 port = VTSS_CHIP_PORT(port_no);
    u32 value;

    memset(conf, 0, sizeof(*conf));

    /* Get speed, duplex and power_down */
    HT_RD(PORT, port, MAC_CFG, &value);
    conf->power_down = (value & (1<<4) ? 1 : 0);
    conf->fdx = (value & (1<<18) ? 1 : 0);
    if (value & (1<<1)) {
        conf->speed = (value & (1<<0) ? VTSS_SPEED_10M : VTSS_SPEED_100M);
    } else if (value & (1<<0)) {
        conf->speed = VTSS_SPEED_1G;
        if (port >= VTSS_CHIP_PORT_VAUI_START) {
            HT_RD(VAUI, S_VAUI, LANE_CFG + (port - VTSS_CHIP_PORT_VAUI_START)*4, &value);
            if ((value & (1<<1)) == 0) {
#if VTSS_OPT_INT_AGGR
                conf->speed = VTSS_SPEED_5G;
#else
                conf->speed = VTSS_SPEED_2500M;
#endif /* VTSS_OPT_INT_AGGR */
            }
        }
    }

    /* Get flow control */
    HT_RD(PORT, port, FCCONF, &value);
    conf->flow_control.obey = (value & (1<<16) ? 1 : 0);
    conf->flow_control.generate = (value & 0xff ? 1 : 0);
    
    return VTSS_RC_OK;
}

static vtss_rc ht_port_counter(u32 port, vtss_chip_counter_t *counter, BOOL clear)
{
    u32 value;

    HT_RD(PORT, port, C_CNTDATA, &value);
    if (clear) {
        /* Clear counter */
        counter->value = 0;
    } else {
        /* Accumulate counter */
        if (value >= counter->prev) {
            /* Not wrapped */
            counter->value += (value - counter->prev);
        } else {
            /* Wrapped */
            counter->value += (0xffffffff - counter->prev);
            counter->value += (value + 1);
        }
    }
    counter->prev = value;
    return VTSS_RC_OK;
}

static vtss_rc ht_port_counters(u32                          port, 
                                vtss_port_luton28_counters_t *c, 
                                vtss_port_counters_t         *const counters, 
                                BOOL                         clear)
{
    vtss_chip_counter_t                dummy;
    vtss_port_rmon_counters_t          *rmon;
    vtss_port_if_group_counters_t      *if_group;
    vtss_port_ethernet_like_counters_t *elike;
    vtss_port_proprietary_counters_t   *prop;
    u32                                i;

    /* Read Rx counters in 32-bit mode */
    HT_WR(PORT, port, C_CNTADDR, (1<<31) | (0x3000<<0));
    VTSS_RC(ht_port_counter(port, &c->rx_octets, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_packets, clear));
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_rxbcmc */
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_rxbadpkt */
    VTSS_RC(ht_port_counter(port, &c->rx_broadcasts, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_multicasts, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_64, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_65_127, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_128_255, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_256_511, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_512_1023, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_1024_1526, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_1527_max, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_pauses, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_drops, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_local_drops, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_classified_drops, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_crc_align_errors, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_shorts, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_longs, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_fragments, clear));
    VTSS_RC(ht_port_counter(port, &c->rx_jabbers, clear));
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_rxctrl */
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_rxgoodpkt */
    VTSS_RC(ht_port_counter(port, &c->rx_class[0], clear));
    VTSS_RC(ht_port_counter(port, &c->rx_class[1], clear));
    VTSS_RC(ht_port_counter(port, &c->rx_class[2], clear));
    VTSS_RC(ht_port_counter(port, &c->rx_class[3], clear));
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_rxtotdrop */
    VTSS_RC(ht_port_counter(port, &c->rx_unicast, clear));

    /* Read Tx c in 32-bit mode */
    HT_WR(PORT, port, C_CNTADDR, (1<<31) | (0x3040<<0));
    VTSS_RC(ht_port_counter(port, &c->tx_octets, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_packets, clear));
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_txbcmc */
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_txbadpkt */
    VTSS_RC(ht_port_counter(port, &c->tx_broadcasts, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_multicasts, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_64, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_65_127, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_128_255, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_256_511, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_512_1023, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_1024_1526, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_1527_max, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_pauses, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_fifo_drops, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_drops, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_collisions, clear));
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_txcfidrop */
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_txgoodpkt */
    VTSS_RC(ht_port_counter(port, &c->tx_class[0], clear));
    VTSS_RC(ht_port_counter(port, &c->tx_class[1], clear));
    VTSS_RC(ht_port_counter(port, &c->tx_class[2], clear));
    VTSS_RC(ht_port_counter(port, &c->tx_class[3], clear));
    VTSS_RC(ht_port_counter(port, &dummy, clear)); /* c_txtotdrop */
    VTSS_RC(ht_port_counter(port, &c->tx_unicast, clear));
    VTSS_RC(ht_port_counter(port, &c->tx_aging, clear));

    if (counters == NULL)
        return VTSS_RC_OK;

    rmon = &counters->rmon;
    if_group = &counters->if_group;
    elike = &counters->ethernet_like;
    prop = &counters->prop;

    /* RMON Rx counters */
    rmon->rx_etherStatsDropEvents += (c->rx_drops.value + c->rx_classified_drops.value);
    rmon->rx_etherStatsOctets += c->rx_octets.value;
    rmon->rx_etherStatsPkts += c->rx_packets.value;
    rmon->rx_etherStatsBroadcastPkts += c->rx_broadcasts.value;
    rmon->rx_etherStatsMulticastPkts += c->rx_multicasts.value;
    rmon->rx_etherStatsCRCAlignErrors += c->rx_crc_align_errors.value;
    rmon->rx_etherStatsUndersizePkts += c->rx_shorts.value;
    rmon->rx_etherStatsOversizePkts += c->rx_longs.value;
    rmon->rx_etherStatsFragments += c->rx_fragments.value;
    rmon->rx_etherStatsJabbers += c->rx_jabbers.value;
    rmon->rx_etherStatsPkts64Octets += c->rx_64.value;
    rmon->rx_etherStatsPkts65to127Octets += c->rx_65_127.value;
    rmon->rx_etherStatsPkts128to255Octets += c->rx_128_255.value;
    rmon->rx_etherStatsPkts256to511Octets += c->rx_256_511.value;
    rmon->rx_etherStatsPkts512to1023Octets += c->rx_512_1023.value;
    rmon->rx_etherStatsPkts1024to1518Octets += c->rx_1024_1526.value;
    rmon->rx_etherStatsPkts1519toMaxOctets += c->rx_1527_max.value;
    
    /* RMON Tx counters */
    rmon->tx_etherStatsDropEvents = (c->tx_fifo_drops.value + c->tx_aging.value);
    rmon->tx_etherStatsOctets += c->tx_octets.value;
    rmon->tx_etherStatsPkts += c->tx_packets.value;
    rmon->tx_etherStatsBroadcastPkts += c->tx_broadcasts.value;
    rmon->tx_etherStatsMulticastPkts += c->tx_multicasts.value;
    rmon->tx_etherStatsCollisions += c->tx_collisions.value;
    rmon->tx_etherStatsPkts64Octets += c->tx_64.value;
    rmon->tx_etherStatsPkts65to127Octets += c->tx_65_127.value;
    rmon->tx_etherStatsPkts128to255Octets += c->tx_128_255.value;
    rmon->tx_etherStatsPkts256to511Octets += c->tx_256_511.value;
    rmon->tx_etherStatsPkts512to1023Octets += c->tx_512_1023.value;
    rmon->tx_etherStatsPkts1024to1518Octets += c->tx_1024_1526.value;
    rmon->tx_etherStatsPkts1519toMaxOctets += c->tx_1527_max.value;
    
    /* Interfaces Group Rx counters */
    /* ifInOctets includes bytes in bad frames */
    if_group->ifInOctets += c->rx_octets.value;
    if_group->ifInUcastPkts += c->rx_unicast.value;
    if_group->ifInMulticastPkts += c->rx_multicasts.value;
    if_group->ifInBroadcastPkts += c->rx_broadcasts.value;
    if_group->ifInNUcastPkts = (c->rx_multicasts.value + c->rx_broadcasts.value);
    if_group->ifInDiscards = rmon->rx_etherStatsDropEvents;
    if_group->ifInErrors = (c->rx_crc_align_errors.value + 
                            c->rx_shorts.value + 
                            c->rx_longs.value +
                            c->rx_fragments.value +
                            c->rx_jabbers.value);
    
    /* Interfaces Group Tx counters */
    if_group->ifOutOctets += c->tx_octets.value;
    if_group->ifOutUcastPkts += c->tx_unicast.value;
    if_group->ifOutMulticastPkts += c->tx_multicasts.value;
    if_group->ifOutBroadcastPkts += c->tx_broadcasts.value;
    if_group->ifOutNUcastPkts = (c->tx_multicasts.value + c->tx_broadcasts.value);
    if_group->ifOutDiscards = rmon->tx_etherStatsDropEvents;
    /* Late/excessive collisions and aged frames */
    if_group->ifOutErrors += c->tx_drops.value; 
    
    /* Ethernet-like counters */
    elike->dot3InPauseFrames += c->rx_pauses.value;
    elike->dot3OutPauseFrames += c->tx_pauses.value;
    
    /* Bridge counters */
    counters->bridge.dot1dTpPortInDiscards += c->rx_local_drops.value;

    /* Proprietary counters */
    for (i = 0; i < L28_PRIOS; i++) {
        prop->rx_prio[i] += c->rx_class[i].value;
        prop->tx_prio[i] += c->tx_class[i].value;
    }

    return VTSS_RC_OK;
}

/* Update/clear/get counters for port */
static vtss_rc ht_port_counters_cmd(const vtss_port_no_t port_no,
                                    vtss_port_counters_t *const counters,
                                    BOOL                 clear)
{
    u32                          port = VTSS_CHIP_PORT(port_no);
    vtss_port_luton28_counters_t *c = &vtss_state->port_counters[port_no].counter.luton28;
    
    VTSS_RC(ht_port_counters(port, c, counters, clear));

#if VTSS_OPT_INT_AGGR
    c = &vtss_state->port_int_counters[port == VTSS_CHIP_PORT_AGGR_0 ? 0 : 1].counter.luton28;
#endif /* VTSS_OPT_INT_AGGR */
    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Process counters for aggregated port */
        VTSS_RC(ht_port_counters(port, c, counters, clear));
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_port_counters_update(const vtss_port_no_t port_no)
{
    return ht_port_counters_cmd(port_no, NULL, 0);
}

static vtss_rc l28_port_counters_clear(const vtss_port_no_t port_no)
{
    return ht_port_counters_cmd(port_no, NULL, 1);
}

static vtss_rc l28_port_counters_get(const vtss_port_no_t port_no,
                                     vtss_port_counters_t *const counters)
{
    memset(counters, 0, sizeof(*counters));
    return ht_port_counters_cmd(port_no, counters, 0);
}

static vtss_rc ht_basic_counters_get(u32 port, vtss_basic_counters_t *const counters)
{
    u32 value;

    /* Rx frames */
    HT_WR(PORT, port, C_CNTADDR, (1<<31) | (0x3002<<0));
    HT_RD(PORT, port, C_CNTDATA, &value);
    counters->rx_frames += value;

    /* Tx frames */
    HT_WR(PORT, port, C_CNTADDR, (1<<31) | (0x3042<<0));
    HT_RD(PORT, port, C_CNTDATA, &value);
    counters->tx_frames += value;
    
    return VTSS_RC_OK;
}

static vtss_rc l28_port_basic_counters_get(const vtss_port_no_t port_no,
                                           vtss_basic_counters_t *const counters)
{
    u32 port = VTSS_CHIP_PORT(port_no);
    
    memset(counters, 0, sizeof(*counters));
    VTSS_RC(ht_basic_counters_get(port, counters));

    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Process counters for aggregated port */
        VTSS_RC(ht_basic_counters_get(port, counters));
    }
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  QoS
 * ================================================================= */

static u32 l28_packet_rate(vtss_packet_rate_t rate, u32 *unit)
{
    u32 value, max;

    /* If the rate is greater than 1000 pps, the unit is kpps */
    max = (rate >= 1000 ? (rate/1000) : rate);
    *unit = (rate >= 1000 ? 0 : 1);
    for (value = 15; value != 0; value--) {
        if (max >= (1<<value))
            break;
    }
    return value;
}

static vtss_rc l28_port_queue_update(void)
{
    vtss_port_no_t       port_no;
    u32                  port;
    BOOL                 wfq_old;
    vtss_port_conf_t     *conf;
    vtss_qos_port_conf_t *qos;

    wfq_old = vtss_state->wfq;
    vtss_state->wfq = 0;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        if (vtss_state->qos_port_conf[port_no].wfq_enable)
            vtss_state->wfq = 1;

    /* WFQ not changed */
    if (vtss_state->wfq == wfq_old)
        return VTSS_RC_OK;
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        port = VTSS_CHIP_PORT(port_no);
        conf = &vtss_state->port_conf[port_no];
        qos = &vtss_state->qos_port_conf[port_no];
        VTSS_RC(ht_port_queue_setup(port, conf, qos));
        if ((port = l28_int_aggr_port(port)) != 0) {
            /* Setup aggregated port */
            VTSS_RC(ht_port_queue_setup(port, conf, qos));
        }
    }
    return VTSS_RC_OK;
}

static u32 l28_chip_prio(const vtss_state_t *const state, const vtss_prio_t prio)
{
    vtss_prio_t maximumNumberOfPrio    = state->prio_count;
    vtss_prio_t configuredNumberOfPrio = state->qos_conf.prios;
    vtss_prio_t newPrio                = prio;

    // Scale down the new priority to a value between 0 and 3
    while (maximumNumberOfPrio > L28_PRIOS) {
        maximumNumberOfPrio >>= 1;
        newPrio >>= 1;
    }
    // Reduce the configured number of queues to the maximum supported value on luton 28
    if (configuredNumberOfPrio > L28_PRIOS) {
        configuredNumberOfPrio = L28_PRIOS;
    }

    switch (configuredNumberOfPrio) {
    case 1: 
        return 3;
    case 2: 
        return (newPrio == VTSS_PRIO_START ? 1 : 3);
    case 4: 
        return (newPrio - VTSS_PRIO_START);
    default:
        break;
    }
    VTSS_E("illegal prios: %u", state->qos_conf.prios);
    return 0; 
}

#define QCL_PORT_MAX 12 /* Number of QCLs per port */
#define QCE_TYPE_FREE      (0<<24)
#define QCE_TYPE_ETYPE     (1<<24)
#define QCE_TYPE_VID       (2<<24)
#define QCE_TYPE_UDP_TCP   (3<<24)
#define QCE_TYPE_DSCP      (4<<24)
#define QCE_TYPE_TOS       (5<<24)
#define QCE_TYPE_TAG       (6<<24)

static BOOL l28_qcl_full(const vtss_qcl_id_t qcl_id)
{
    int              i;
    vtss_qcl_entry_t *entry;

    for (i = 0, entry = vtss_state->qcl[qcl_id].qcl_list_used;
         entry != NULL; i++, entry = entry->next) {
        
        switch (entry->qce.type) {
        case VTSS_QCE_TYPE_ETYPE:
        case VTSS_QCE_TYPE_VLAN:
        case VTSS_QCE_TYPE_TOS:
        case VTSS_QCE_TYPE_TAG:
            /* These entries take up a single QCL entry */
            break;
        case VTSS_QCE_TYPE_UDP_TCP:
            if (entry->qce.frame.udp_tcp.low != entry->qce.frame.udp_tcp.high) {
                /* Range, at least two entries required */
                if (i & 1) /* Odd address, three entries requried */
                    i++;
                i++;
            }
            break;
        case VTSS_QCE_TYPE_DSCP:
            if (entry->next != NULL && entry->next->qce.type == VTSS_QCE_TYPE_DSCP) {
                /* If next entry is DSCP, merge */
                entry = entry->next;
            }
            break;
        default: 
            VTSS_E("Unknown QCE type (%d)", entry->qce.type); 
            return 0;
        }
    }
    return (i > QCL_PORT_MAX);
}

static vtss_rc l28_qcl_add(u32 port, const vtss_qcl_id_t qcl_id)
{
    u32              value, i, j;
    vtss_qcl_entry_t *entry;

    /* Clear range table */
    HT_WR(PORT, port, CAT_QCL_RANGE_CFG, 0);
    
    entry = (qcl_id == VTSS_QCL_ID_NONE ? NULL : vtss_state->qcl[qcl_id].qcl_list_used);
    for (i = 0; i < QCL_PORT_MAX && entry != NULL; i++, entry = entry->next) {
        switch(entry->qce.type) {
        case VTSS_QCE_TYPE_ETYPE:
            value = (QCE_TYPE_ETYPE | 
                     (l28_chip_prio(vtss_state, entry->qce.frame.etype.prio)<<16) | 
                     (entry->qce.frame.etype.val<<0)); 
            VTSS_D("adding ETYPE QCE[%u]: 0x%08x", i, value);
            HT_WR(PORT, port, CAT_QCE0 + i, value);
            break;
        case VTSS_QCE_TYPE_VLAN:
            value = (QCE_TYPE_VID | 
                     (l28_chip_prio(vtss_state, entry->qce.frame.vlan.prio)<<16) | 
                     (entry->qce.frame.vlan.vid<<0));
            VTSS_D("adding VLAN QCE[%u]: 0x%08x", i, value);
            HT_WR(PORT, port, CAT_QCE0 + i, value);
            break;
        case VTSS_QCE_TYPE_UDP_TCP:
            if (entry->qce.frame.udp_tcp.low != entry->qce.frame.udp_tcp.high) {
                if (i & 1) {
                    VTSS_D("adding free QCE[%u] for UDP/TCP range", i);
                    HT_WR(PORT, port, CAT_QCE0 + i, QCE_TYPE_FREE);
                    i++;
                    if (i >= QCL_PORT_MAX)
                        break;
                }
                VTSS_D("setting bit #%u of QCL range cfg", i/2);  
                HT_WRF(PORT, port, CAT_QCL_RANGE_CFG, i/2, 0x1, 1);
            }
            value = (QCE_TYPE_UDP_TCP | 
                     (l28_chip_prio(vtss_state, entry->qce.frame.udp_tcp.prio)<<16) | 
                     (entry->qce.frame.udp_tcp.low<<0)); 
            VTSS_D("adding UDP/TCP QCE[%u]: 0x%08x", i, value);
            HT_WR(PORT, port, CAT_QCE0 + i, value);
            if (entry->qce.frame.udp_tcp.low != entry->qce.frame.udp_tcp.high) {
                i++;
                if (i >= QCL_PORT_MAX)
                    break;
                value = (QCE_TYPE_UDP_TCP | 
                         (l28_chip_prio(vtss_state, entry->qce.frame.udp_tcp.prio)<<16) | 
                         (entry->qce.frame.udp_tcp.high<<0)); 
                VTSS_D("adding UDP/TCP QCE[%u]: 0x%08x", i, value);
                HT_WR(PORT, port, CAT_QCE0 + i, value);
            }
            break;
        case VTSS_QCE_TYPE_DSCP:
            value = (QCE_TYPE_DSCP | 
                     (entry->qce.frame.dscp.dscp_val <<  2) | 
                     (l28_chip_prio(vtss_state, entry->qce.frame.dscp.prio) << 0));  /* DSCP0 */ 
            if (entry->next != NULL && entry->next->qce.type == VTSS_QCE_TYPE_DSCP) {
                /* If next entry is DSCP, merge */
                entry = entry->next;
            }
            value |= ((entry->qce.frame.dscp.dscp_val << 10) | 
                      (l28_chip_prio(vtss_state, entry->qce.frame.dscp.prio) << 8)); /* DSCP1 */ 
            VTSS_D("adding DSCP QCE[%u]: 0x%08x", i, value); 
            HT_WR(PORT, port, CAT_QCE0 + i, value);
            break;
        case VTSS_QCE_TYPE_TOS:
            value = QCE_TYPE_TOS;
            for (j = 0; j < 8; j++) {
                value |= (l28_chip_prio(vtss_state, entry->qce.frame.tos_prio[j])<<(j*2));
            }
            VTSS_D("adding TOS QCE[%u]: 0x%08x", i, value); 
            HT_WR(PORT, port, CAT_QCE0 + i, value);
            break;
        case VTSS_QCE_TYPE_TAG:
            value = QCE_TYPE_TAG;
            for (j = 0; j < 8; j++) {
                value |= (l28_chip_prio(vtss_state, entry->qce.frame.tag_prio[j])<<(j*2));
            }
            VTSS_D("adding TAG QCE[%u]: 0x%08x", i, value); 
            HT_WR(PORT, port, CAT_QCE0 + i, value);
            break;
        default: 
            VTSS_E("unknown QCE type (%d)", entry->qce.type); 
            return VTSS_RC_ERROR;
        }
    }
        
    /* Clear unused entries */
    while (i < QCL_PORT_MAX) {
        HT_WR(PORT, port, CAT_QCE0 + i, QCE_TYPE_FREE);
        i++;
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_qos_port_set(u32 port, vtss_qos_port_conf_t *conf, BOOL vstax)
{
    u32              i, value;
    vtss_prio_t      prio;
    
    if (vstax) {
        /* Mapping from internal priority to VStaX priority:
           0->1, 1->3, 2->5, 3->7 */
        HT_WR(PORT, port, CAT_GEN_PRIO_REMAP, (7<<12) | (5<<8) | (3<<4) | (1<<0));

        /* Mapping from VStaX priority to internal priority:
           0->0, 1->0, 2->1, 3->1, 4->2, 5->2, 6->3, 7->3 */
        for (i = 0; i < QCL_PORT_MAX; i++) {
            HT_WR(PORT, port, CAT_QCE0 + i, QCE_TYPE_FREE | 0xfa50);
        }
    } else {
        /* Add QCL */
        VTSS_RC(l28_qcl_add(port, conf->qcl_id));
    }

    /* Default ingress VLAN tag priority */
    HT_WRF(PORT, port, CAT_PORT_VLAN, 12, 0x7, conf->usr_prio);

    /* Default priority */
    HT_WR(PORT, port, CAT_QCL_DEFAULT_CFG, l28_chip_prio(vtss_state, conf->default_prio));

    /* DSCP remarking (ingress disabled on stack ports) */
    value = (vstax ? VTSS_DSCP_MODE_NONE : conf->dscp_mode);
    if (port < 16) {
        HT_WRF(ANALYZER, 0, DSCPMODELOW, port*2, 0x3, value);
    } else {
        HT_WRF(ANALYZER, 0, DSCPMODEHIGH, (port - 16)*2, 0x3, value);
    }
    HT_WRF(PORT, port, TXUPDCFG, 18, 0x1, conf->dscp_remark ? 1 : 0);

    value = 0;
    for (prio = VTSS_PRIO_START; prio < L28_PRIO_END; prio++)
        value |= (conf->dscp_map[prio]<<((prio - VTSS_PRIO_START)*8));
    HT_WR(PORT, port, CAT_PR_DSCP_VAL_0_3, value);
   
    /* Tag remarking */
    HT_WRF(PORT, port, TXUPDCFG, 17, 0x1, conf->tag_remark ? 1 : 0);
    HT_WRF(PORT, port, TXUPDCFG, 2, 0x1, conf->tag_remark ? 1 : 0);
    
    if (!vstax) {
        /* Tag remarking can not be used on stack ports */
        value = 0;
        for (prio = VTSS_PRIO_START; prio < L28_PRIO_END; prio++)
            value |= (conf->tag_map[prio]<<((prio - VTSS_PRIO_START)*4));
        HT_WR(PORT, port, CAT_GEN_PRIO_REMAP, value);
    }

    return VTSS_RC_OK;
}

#define RATE_FACTOR        1666667 /* 8000000/4.8 */
#define RATE_MAX           6510    /* 0xFFF*RATE_FACTOR/0xFFFFF */
#define BURST_LEVEL(level) (level >= 32000 ? 2 : level >= 2000 ? 1 : 0)

static vtss_rc l28_policer_shaper_update(void)
{
    u32                  i, port, j, weight, w[L28_PRIOS];
    vtss_port_no_t       port_no;
    u32                  max_rate = 0, policer_rate, shaper_rate, max_rate_burst = 0;
    vtss_policer_t       *policer;
    vtss_shaper_t        *shaper;
    vtss_qos_port_conf_t *conf;
    
    for (i = 0; i < 2; i++) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            conf = &vtss_state->qos_port_conf[port_no];
            policer = &conf->policer_port[0];
            policer_rate = policer->rate;
            if (policer_rate == VTSS_BITRATE_DISABLED)
                policer_rate = 0;
            
            shaper = &conf->shaper_port;
            shaper_rate = shaper->rate;
            if (shaper_rate == VTSS_BITRATE_DISABLED)
                shaper_rate = 0;
                
            if (i == 0) {
                /* First round: calculate maximum rate */
                if (policer_rate > max_rate)
                    max_rate = policer_rate;

                if (shaper_rate > max_rate)
                    max_rate = shaper_rate;

                /* Find the maximum rate of policers/shapers with minimum burst level */
                if (policer->level < 4000 && policer_rate > max_rate_burst)
                    max_rate_burst = policer_rate;

                if (shaper->level < 4000 && shaper_rate > max_rate_burst)
                    max_rate_burst = shaper_rate;
            } else {
                /* Second round: Setup policer and shaper */
                port = VTSS_CHIP_PORT(port_no);
                for (j = 0; j < L28_PRIOS; j++) {
                    weight = conf->weight[j + VTSS_QUEUE_START];
                    w[j] = (conf->wfq_enable ? 
                            (weight == VTSS_WEIGHT_1 ? 3 :
                             weight == VTSS_WEIGHT_2 ? 2 :
                             weight == VTSS_WEIGHT_4 ? 1 :
                             weight == VTSS_WEIGHT_8 ? 0 : 4) : 0);
                    if (w[j] > 3) {
                        VTSS_E("illegal weight: %d", weight);
                        w[j] = 0;
                    }
                }
                HT_WR(PORT, port, SHAPER_CFG,
                      (20<<24) | /* GAP_VALUE */
                      (w[0]<<22) | (w[1]<<20) | (w[2]<<18) | (w[3]<<16) | 
                      ((conf->wfq_enable ? 1 : 0)<<15) |
                      (BURST_LEVEL(shaper->level)<<12) |  /* EGRESS_BURST */
                      (shaper_rate*0xFFF/max_rate)<<0);   /* EGRESS_RATE */
                HT_WR(PORT, port, POLICER_CFG,
                      (1<<15) | /* POLICE_FWD */
                      (0<<14) | /* POLICE_FC */
                      (BURST_LEVEL(policer->level)<<12) | /* INGRESS_BURST */
                      (policer_rate*0xFFF/max_rate)<<0);  /* INGRESS RATE */
            }
        }
        if (i == 0) {
            /* Calculate maximum rate */
            if (max_rate > 1000000 || max_rate == 0) /* Maximum 1G */
                max_rate = 1000000;
            
            /* Setup rate unit */
            /* The rate of a policer/shaper with minimum burst level must not exceed 256 */
            max_rate_burst *= (4096/256);
            if (max_rate_burst != 0 && max_rate_burst > max_rate)
                max_rate = max_rate_burst;
            
            if (max_rate < RATE_MAX)
                max_rate = RATE_MAX; 
            HT_WR(SYSTEM, 0, RATEUNIT, 5*(((0xFFF/5)*RATE_FACTOR)/max_rate));
        }
    }
    
    return VTSS_RC_OK;
}

static vtss_rc l28_qos_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_qos_port_conf_t *conf = &vtss_state->qos_port_conf[port_no];
    u32                  port = VTSS_CHIP_PORT(port_no);
    vtss_port_conf_t     *port_conf = &vtss_state->port_conf[port_no];
    BOOL                 vstax = vtss_state->vstax_port_enabled[port_no];
    
    /* Update port queues if WFQ changed */
    VTSS_RC(l28_port_queue_update());
    
    /* Setup QoS and queues (depend on policer setup) */
    VTSS_RC(l28_qos_port_set(port, conf, vstax));
    VTSS_RC(ht_port_queue_setup(port, port_conf, conf));
    
    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Setup aggregated port */
        VTSS_RC(l28_qos_port_set(port, conf, vstax));
        VTSS_RC(ht_port_queue_setup(port, port_conf, conf));
    }

    /* Policers and shapers */
    VTSS_RC(l28_policer_shaper_update());

    return VTSS_RC_OK;
}

static vtss_rc l28_qos_conf_set(BOOL changed)
{
    vtss_qos_conf_t    *conf = &vtss_state->qos_conf;
    vtss_port_no_t     port_no;
    u32                i, low = 0, high = 0, unit;
    vtss_packet_rate_t rate;
        
    if (changed) {
        /* Number of priorities changed, update QoS setup for all ports */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            VTSS_RC(l28_qos_port_conf_set(port_no));   
        }
    }

    /* DHCP remarking */
    for (i = 0; i < 64; i++) {
        if (conf->dscp_remark[i]) {
            if (i < 32)
                low |= (1<<i);
            else
                high |= (1<<(i-32));
        }
    }
    HT_WR(ANALYZER, 0, DSCPSELLOW, low);
    HT_WR(ANALYZER, 0, DSCPSELHIGH, high);

    /* Packet policers */
    HT_WRF(ANALYZER, 0, STORMLIMIT, 28, 0xf, 6); /* Burst of 64 frames allowed */
    for (i = 0; i < 6; i++) {
        rate = (i == 0 ? conf->policer_uc : i == 1 ? conf->policer_mc :
                i == 2 ? conf->policer_bc : i == 3 ? conf->policer_learn : 
                i == 4 ? conf->policer_cat : conf->policer_mac);
        HT_WRF(ANALYZER, 0, STORMLIMIT, 4*i, 0xf, l28_packet_rate(rate, &unit));
        HT_WRF(ANALYZER, 0, STORMPOLUNIT, i, 0x1, unit);
        HT_WRF(ANALYZER, 0, STORMLIMIT_ENA, i, 0x1, rate == VTSS_PACKET_RATE_DISABLED ? 0 : 1);
    }
    
    return VTSS_RC_OK;
}

static vtss_rc l28_qcl_commit(const vtss_qcl_id_t qcl_id)
{
    vtss_port_no_t       port_no;
    vtss_qos_port_conf_t *conf;

    if (l28_qcl_full(qcl_id)) {
        VTSS_D("Warning: QCL %u will be truncated when applied", qcl_id);
    }

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        conf = &vtss_state->qos_port_conf[port_no];
        if (conf->qcl_id == qcl_id) {
            /* Reapply QoS settings */
            VTSS_RC(l28_qos_port_conf_set(port_no));
        }
    }

    return VTSS_RC_OK;
}

static vtss_rc l28_qce_add(const vtss_qcl_id_t  qcl_id,
                           const vtss_qce_id_t  qce_id,
                           const vtss_qce_t     *const qce)
{
    vtss_qcl_t       *qcl;
    vtss_qcl_entry_t *cur, *prev = NULL;
    vtss_qcl_entry_t *new = NULL, *new_prev = NULL, *ins = NULL, *ins_prev = NULL;
    
    VTSS_D("enter, qcl_id: %u, qce_id: %u", qcl_id, qce_id);
    
    /* QCE ID 0 is reserved for insertion at end. Also check for same id */
    if (qce->id == VTSS_QCE_ID_LAST || qce->id == qce_id) {
        VTSS_E("illegal qce id: %u", qce->id);
        return VTSS_RC_ERROR;
    }

    /* Check that the QCL ID is valid */
    if (qcl_id < VTSS_QCL_ID_START || qcl_id >= VTSS_QCL_ID_END) {
        VTSS_E("illegal qcl_id: %u", qcl_id);
        return VTSS_RC_ERROR;
    }
    
    /* Search for existing entry and place to add */
    qcl = &vtss_state->qcl[qcl_id];
    for (cur = qcl->qcl_list_used; cur != NULL; prev = cur, cur = cur->next) {
        if (cur->qce.id == qce->id) {
            /* Entry already exists */
            new_prev = prev;
            new = cur;
        }
        
        if (cur->qce.id == qce_id) {
            /* Found insertion point */
            ins_prev = prev;
            ins = cur;
        }
    }

    if (qce_id == VTSS_QCE_ID_LAST)
        ins_prev = prev;

    /* Check if the place to insert was found */
    if (ins == NULL && qce_id != VTSS_QCE_ID_LAST) {
        VTSS_E("could not find qce_id: %u", qce_id);
        return VTSS_RC_ERROR;
    }
    
    if (new == NULL) {
        /* Take new entry from free list */
        if ((new = qcl->qcl_list_free) == NULL) {
            VTSS_E("QCL is full");
            return VTSS_RC_ERROR;
        }
        qcl->qcl_list_free = new->next;
    } else {
        /* Take existing entry out of list */
        if (ins_prev == new)
            ins_prev = new_prev;
        if (new_prev == NULL)
            qcl->qcl_list_used = new->next;
        else
            new_prev->next = new->next;
    }

    /* Insert new entry in list */
    if (ins_prev == NULL) {
        new->next = qcl->qcl_list_used;
        qcl->qcl_list_used = new;
    } else {
        new->next = ins_prev->next;
        ins_prev->next = new;
    }
    new->qce = *qce;
    
    return l28_qcl_commit(qcl_id);
}

static vtss_rc l28_qce_del(const vtss_qcl_id_t  qcl_id,
                           const vtss_qce_id_t  qce_id)
{
    vtss_qcl_t       *qcl;
    vtss_qcl_entry_t *cur, *prev;
    
    VTSS_D("enter, qcl_id: %u, qce_id: %u", qcl_id, qce_id);

    /* Check that the QCL ID is valid */
    if (qcl_id < VTSS_QCL_ID_START || qcl_id >= VTSS_QCL_ID_END) {
        VTSS_E("illegal QCL ID: %u", qcl_id);
        return VTSS_RC_ERROR;
    }
  
    qcl = &vtss_state->qcl[qcl_id];
    
    for (cur = qcl->qcl_list_used, prev = NULL; cur != NULL; prev = cur, cur = cur->next) {
        if (cur->qce.id == qce_id) {
            VTSS_D("Found existing ID %u", qce_id);
            if (prev == NULL)
                qcl->qcl_list_used = cur->next;
            else
                prev->next = cur->next;
            cur->next = qcl->qcl_list_free;
            qcl->qcl_list_free = cur;
            break;
        }
    }
    return (cur == NULL ? VTSS_RC_ERROR : l28_qcl_commit(qcl_id));
}

/* ================================================================= *
 *  Packet control
 * ================================================================= */

static vtss_rc l28_rx_conf_set(void)
{
    vtss_packet_rx_conf_t      *conf = &vtss_state->rx_conf;
    vtss_packet_rx_reg_t       *reg = &conf->reg;
    vtss_packet_rx_queue_map_t *map = &conf->map;
    u32                        queue, i, value;
    
    /* Rx queue */
    for (queue = 0; queue < vtss_state->rx_queue_count; queue++) {
        HT_WRF(PORT, VTSS_CHIP_PORT_CPU, Q_EGRESS_WM, 
               queue*8, 0x1F, conf->queue[queue].size/2048);
    }

    /* Rx registration */
    value = ((VTSS_BOOL(reg->bpdu_cpu_only)<<16) |
             (VTSS_BOOL(reg->igmp_cpu_only)<<18) |
             (VTSS_BOOL(reg->ipmc_ctrl_cpu_copy)<<19) |
             (VTSS_BOOL(reg->mld_cpu_only)<<20));
    for (i = 0; i < 16; i++)
        value |= (VTSS_BOOL(reg->garp_cpu_only[i])<<i);
    HT_WR(ANALYZER, 0, CAPENAB, value);

    /* Rx map */
    HT_WR(ANALYZER, 0, CAPQUEUE,
          ((map->learn_queue-VTSS_PACKET_RX_QUEUE_START)<<12) |
          ((map->mac_vid_queue-VTSS_PACKET_RX_QUEUE_START)<<10) |
          ((map->igmp_queue-VTSS_PACKET_RX_QUEUE_START)<<8) |
          ((map->ipmc_ctrl_queue-VTSS_PACKET_RX_QUEUE_START)<<6) |
          ((map->igmp_queue-VTSS_PACKET_RX_QUEUE_START)<<4) |
          (0<<2) | /* 01-80-C2-00-00-10 */
          ((map->bpdu_queue-VTSS_PACKET_RX_QUEUE_START)<<0));

    value = 0;
    for (i = 0; i < 16; i++) {
        value |= ((map->garp_queue-VTSS_PACKET_RX_QUEUE_START)<<(i*2));
    }
    HT_WR(ANALYZER, 0, CAPQUEUEGARP, value);
    
    return VTSS_RC_OK;
}

static vtss_rc l28_rx_word(u32 queue, u32 *value)
{
    HT_RD(CAPTURE, S_CAPTURE_DATA + queue*2, FRAME_DATA, value);
    return VTSS_RC_OK;
}

static vtss_rc l28_vstax_header_get(const vtss_state_t           *const state,
                                    vtss_port_no_t               port_no,
                                    const vtss_vstax_tx_header_t *const vstax,
                                    u32                          *vsh0, 
                                    u32                          *vsh1, 
                                    u32                          *vsh2)
{
    BOOL super = (vstax->prio == VTSS_PRIO_SUPER ? 1 : 0);
    u32  port = (vstax->port_no == VTSS_PORT_NO_NONE ? 0 : VTSS_CHIP_PORT_FROM_STATE(state, vstax->port_no));
    u32  w1 = 0, w2 = 0;
    
    *vsh0 = ((VTSS_ETYPE_VTSS<<16) | EPID_VSTAX);
    VSH_PUT(w1, w2, SPRIO, super);
    VSH_PUT(w1, w2, IPRIO, 2 * l28_chip_prio(state, super ? (L28_PRIO_END - 1) : vstax->prio) + 1);
    VSH_PUT(w1, w2, TTL, vstax->ttl == VTSS_VSTAX_TTL_PORT ? state->vstax_info.chip_info[0].port_conf[port_no == state->vstax_conf.port_0 ? 0 : 1].ttl : vstax->ttl);
    VSH_PUT(w1, w2, LRN_SKIP, 1);
    VSH_PUT(w1, w2, FWD_MODE, vstax->fwd_mode);
    if (vstax->fwd_mode == VTSS_VSTAX_FWD_MODE_UPSID_PORT ||
        vstax->fwd_mode == VTSS_VSTAX_FWD_MODE_CPU_UPSID)
        VSH_PUT(w1, w2, DST_UPSID, vstax->upsid - VTSS_VSTAX_UPSID_START);
    if (vstax->fwd_mode == VTSS_VSTAX_FWD_MODE_UPSID_PORT)
        VSH_PUT(w1, w2, DST_PORT, port);
    if (vstax->fwd_mode == VTSS_VSTAX_FWD_MODE_CPU_UPSID ||
        vstax->fwd_mode == VTSS_VSTAX_FWD_MODE_CPU_ALL)
        VSH_PUT(w1, w2, DST_PORT, vstax->queue_no - VTSS_PACKET_RX_QUEUE_START);
    VSH_PUT(w1, w2, VID, vstax->tci.vid);
    VSH_PUT(w1, w2, CFI, vstax->tci.cfi);
    VSH_PUT(w1, w2, UPRIO, vstax->tci.tagprio);
    VSH_PUT(w1, w2, SRC_UPSID, state->vstax_conf.upsid_0);
    if (vstax->fwd_mode == VTSS_VSTAX_FWD_MODE_CPU_UPSID ||
        vstax->fwd_mode == VTSS_VSTAX_FWD_MODE_CPU_ALL) {
        if (vstax->glag_no != VTSS_GLAG_NO_NONE) {
            VSH_PUT(w1, w2, SRC_MODE, 1);
            port = (vstax->glag_no - VTSS_GLAG_NO_START);
        }
        VSH_PUT(w1, w2, SRC_PORT, port);
    }
    
    *vsh1 = w1;
    *vsh2 = w2;

    return VTSS_RC_OK;
}

static vtss_rc l28_vstax_header_put(const vtss_state_t *const state, u32 vsh0, u32 vsh1, u32 vsh2,
                                    vtss_vstax_rx_header_t *vstax_header)
{
    u32 src_port;

    memset(vstax_header, 0, sizeof(*vstax_header));
    
    vstax_header->valid = (vsh0 == ((VTSS_ETYPE_VTSS<<16) | EPID_VSTAX) ? 1 : 0);
    vstax_header->upsid = (VSH_GET(vsh1, vsh2, SRC_UPSID) + VTSS_VSTAX_UPSID_START);
    src_port = VSH_GET(vsh1, vsh2, SRC_PORT);
    vstax_header->sp = VSH_GET(vsh1, vsh2, SPRIO);
    if (VSH_GET(vsh1, vsh2, SRC_MODE)) {
        /* GLAG */
        vstax_header->port_no = VTSS_PORT_NO_NONE;
        vstax_header->glag_no = (src_port + VTSS_GLAG_NO_START);
    } else {
        /* Port */
        vstax_header->port_no = l28_vtss_pgid(state, src_port);
        vstax_header->glag_no = VTSS_GLAG_NO_NONE;
    }

    return VTSS_RC_OK;
}

static vtss_rc l28_rx_frame_discard(const vtss_packet_rx_queue_t  queue_no)
{
    vtss_packet_rx_t *packet;
    u32              queue, w, w_max, data;

    queue = (queue_no - VTSS_PACKET_RX_QUEUE_START);
    packet = &vtss_state->rx_packet[queue];
    if (packet->used) {
        
        /* Calculate number of words to read */
        w_max = (packet->header.length + 7)/4; /* Include FCS */
        if (packet->header.vstax.valid)
            w_max += (VTSS_VSTAX_HDR_SIZE/4); /* Include VStaX header */
        if (w_max & 1)
            w_max++; /* Read even number of words */
        
        /* Read out rest of frame */
        for (w = packet->words_read; w < w_max; w++) {
            VTSS_RC(l28_rx_word(queue, &data));
        }
        packet->used = 0;
        packet->words_read = 0;
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_rx_frame_get(const vtss_packet_rx_queue_t  queue_no,
                                vtss_packet_rx_header_t       *const header,
                                u8                            *const frame,
                                const u32                     length)
{
    u32              value, queue, w, len, data, ifh0, ifh1, vsh1, vsh2, pgid;
    vtss_port_no_t   port_no;
    vtss_glag_no_t   glag_no;
    vtss_packet_rx_t *packet;
    
    queue = (queue_no - VTSS_PACKET_RX_QUEUE_START);
    packet = &vtss_state->rx_packet[queue];
    if (packet->used) {
        /* Header has already been read, copy from API state */
        *header = packet->header;
    } else {
        /* Check if frame is ready */
        HT_RDF(SYSTEM, 0, CAPCTRL, 28 + queue, 0x1, &value);
        if (value == 0) {
            VTSS_N("no frame ready in queue %u", queue_no);
            return VTSS_RC_ERROR;
        }
        
        /* Read IFH */
        VTSS_RC(l28_rx_word(queue, &ifh0));
        VTSS_RC(l28_rx_word(queue, &ifh1));

        /* Extract IFH fields */
        header->length = (IFH_GET(ifh0, ifh1, LENGTH) - 4); /* Exclude FCS */
        pgid = l28_vtss_pgid(vtss_state, IFH_GET(ifh0, ifh1, PORT));
        if (pgid == vtss_state->pgid_glag_dest || pgid == (vtss_state->pgid_glag_dest + 1)) {
            /* GLAG, return first member port or VTSS_PORT_NO_NONE */
            glag_no = (pgid - vtss_state->pgid_glag_dest + VTSS_GLAG_NO_START);
            pgid = VTSS_PORT_NO_NONE;
            for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
                if (vtss_state->port_glag_no[port_no] == glag_no) {
                    pgid = port_no;
                    break;
                }
        }
        header->port_no = pgid;
        header->queue_mask = (1 << queue); /* Backwards compatible */
        header->arrived_tagged = IFH_GET(ifh0, ifh1, TAGGED);
        header->tag.tagprio = IFH_GET(ifh0, ifh1, UPRIO);
        header->tag.cfi = IFH_GET(ifh0, ifh1, CFI);
        header->tag.vid = IFH_GET(ifh0, ifh1, VID);
        header->learn = IFH_GET(ifh0, ifh1, LEARN);

        /* VStaX header is stripped, if present */
        header->vstax.valid = IFH_GET(ifh0, ifh1, VSTAX);
        if (header->vstax.valid && (header->length >= VTSS_VSTAX_HDR_SIZE))
            header->length -= VTSS_VSTAX_HDR_SIZE;
        
        /* Store header */
        packet->used = 1;
        packet->header = *header;
    }

    len = (header->length < length ? header->length : length);
    for (w = 0; w < (len+3)/4; w++) {
        if (w == 3 && header->vstax.valid) {
            /* Strip VStaX header */
            VTSS_RC(l28_rx_word(queue, &data)); /* VSH0 (ignored) */
            VTSS_RC(l28_rx_word(queue, &vsh1)); /* VSH1 */
            VTSS_RC(l28_rx_word(queue, &vsh2)); /* VSH2 */
            VTSS_RC(l28_vstax_header_put(vtss_state, data, vsh1, vsh2, &header->vstax));
        }
        VTSS_RC(l28_rx_word(queue, &data));
        frame[w*4+0]=((data>>24) & 0xff);
        frame[w*4+1]=((data>>16) & 0xff);
        frame[w*4+2]=((data>>8) & 0xff);
        frame[w*4+3]=(data & 0xff);
    }

    if (length < header->length)
        return VTSS_RC_ERROR; /* Buffer too small */
    
    if (header->vstax.valid)
        w += (VTSS_VSTAX_HDR_SIZE/4);
    packet->words_read = w;
    return l28_rx_frame_discard(queue_no);
}

static vtss_rc l28_tx_word(u32 port, u32 value)
{
    u32 i, pending;

    HT_WR(PORT, port, CPUTXDAT, value);
    for (i = 0; i < 1000000; i++) {
        HT_RDF(PORT, port, MISCSTAT, 8, 0x1, &pending);
        if (!pending)
            return VTSS_RC_OK; /* Done */
    }

    /* Timeout, cancel transmission */
    HT_WR(PORT, port, MISCFIFO, (1<<1));

    return VTSS_RC_ERROR;
}

static void l28_word2frame(u32 data, u8 *frame)
{ 
    frame[0] = ((data>>24) & 0xff); 
    frame[1] = ((data>>16) & 0xff);
    frame[2] = ((data>>8) & 0xff);
    frame[3] = (data & 0xff);
}

static u32 l28_frame2word(const u8 *frame)
{ 
    return ((frame[0] << 24) | (frame[1] << 16) | (frame[2] << 8) | (frame[3] << 0));
}

static vtss_rc l28_tx_frame_port(const vtss_port_no_t  port_no,
                                 const u8              *const frame,
                                 const u32             length,
                                 const vtss_vid_t      vid)
{
    u32                          port = VTSS_CHIP_PORT(port_no);
    u32                          w, addw, len, ifh0 = 0, ifh1 = 0, vsh0, vsh1, vsh2;
    const vtss_vstax_tx_header_t *vstax;
    vtss_vid_t                   tx_vid = vid;
    
    vstax = (vtss_state->vstax_port_enabled[port_no] ? vtss_state->vstax_tx_header : NULL);

    /* Calculate and send IFH0 and IFH1 */
    addw = 1; /* CRC */
    if (vstax != NULL) {
        addw += (VTSS_VSTAX_HDR_SIZE/4); /* VStaX header */
        tx_vid = VTSS_VID_NULL;          /* Never insert VLAN tag */

        IFH_PUT(ifh0, ifh1, VSTAX, 1);
        if (port >= VTSS_CHIP_PORT_AGGR_1) {
            IFH_PUT(ifh0, ifh1, STACK_B, 1);
        } else {
            IFH_PUT(ifh0, ifh1, STACK_A, 1);
        }
    }
    if (tx_vid != VTSS_VID_NULL)
        addw++; /* VLAN tag */

    len = (length + addw*4);
    IFH_PUT(ifh0, ifh1, LENGTH, len < 64 ? 64 : len);

    VTSS_RC(l28_tx_word(port, ifh0));
    VTSS_RC(l28_tx_word(port, ifh1));

    /* Send frame data */
    for (w = 0; w <= (length-1)/4; w++) {
        if (w == 3) {
            if (vstax != NULL) {
                /* Insert VStaX header */
                VTSS_RC(l28_vstax_header_get(vtss_state, port_no, vstax, &vsh0, &vsh1, &vsh2));
                VTSS_RC(l28_tx_word(port, vsh0));
                VTSS_RC(l28_tx_word(port, vsh1));
                VTSS_RC(l28_tx_word(port, vsh2));
            }
            if (tx_vid != VTSS_VID_NULL) {
                /* Insert tag */
                VTSS_RC(l28_tx_word(port, (0x8100<<16) | (0<<13) | (0<<12) | vid));
            }
        }
        VTSS_RC(l28_tx_word(port, l28_frame2word(&frame[w*4])));
    }

    /* Add padding when the frame is too short */
    w += addw;
    while (w < (64/4)) {
        VTSS_RC(l28_tx_word(port, 0));
        w++;
    }

    /* Add dummy CRC */
    VTSS_RC(l28_tx_word(port, 0));

    /* We must write an even number of longs, so write an extra */
    if (w & 1) {
        VTSS_RC(l28_tx_word(port, 0));
    }
    
    /* Transmit data */
    HT_WR(PORT, port, MISCFIFO, (1<<0));

    return VTSS_RC_OK;
}

static vtss_rc l28_vstax_header2frame(const vtss_state_t            *const state,
                                      const vtss_port_no_t          port_no,
                                      const vtss_vstax_tx_header_t  *const header,
                                      u8                            *const frame)
{
    u32 vsh0, vsh1, vsh2;
    
    VTSS_RC(l28_vstax_header_get(state, port_no, header, &vsh0, &vsh1, &vsh2));
    l28_word2frame(vsh0, frame);
    l28_word2frame(vsh1, frame + 4);
    l28_word2frame(vsh2, frame + 8);

    return VTSS_RC_OK;
}

static vtss_rc l28_vstax_frame2header(const vtss_state_t      *const state,
                                      const u8                *const frame,
                                      vtss_vstax_rx_header_t  *const header)
{
    u32 vsh0, vsh1, vsh2;
    
    vsh0 = l28_frame2word(frame);
    vsh1 = l28_frame2word(frame + 4);
    vsh2 = l28_frame2word(frame + 8);
    VTSS_RC(l28_vstax_header_put(state, vsh0, vsh1, vsh2, header));
    
    return VTSS_RC_OK;
}

static vtss_rc l28_rx_hdr_decode(const vtss_state_t          *const state,
                                 const vtss_packet_rx_meta_t *const meta,
                                       vtss_packet_rx_info_t *const info)
{
    u32                 ifh0, ifh1, pgid, save_pgid;
    vtss_port_no_t      port_no;
    vtss_etype_t        etype = (meta->bin_hdr[8 /* xtr hdr */ + 2 * 6 /* 2 MAC addresses*/] << 8) | (meta->bin_hdr[8 + 2 * 6 + 1]);
    BOOL                was_tagged, s_tagged;
    vtss_trace_group_t  trc_grp = meta->no_wait ? VTSS_TRACE_GROUP_FDMA_IRQ : VTSS_TRACE_GROUP_PACKET;

    VTSS_DG(trc_grp, "IFH:");
    VTSS_DG_HEX(trc_grp, &meta->bin_hdr[0], 8);

    ifh0 = ((u32)meta->bin_hdr[0] << 0) | ((u32)meta->bin_hdr[1] << 8) | ((u32)meta->bin_hdr[2] << 16) | ((u32)meta->bin_hdr[3] << 24);
    ifh1 = ((u32)meta->bin_hdr[4] << 0) | ((u32)meta->bin_hdr[5] << 8) | ((u32)meta->bin_hdr[6] << 16) | ((u32)meta->bin_hdr[7] << 24);

    memset(info, 0, sizeof(*info));

    info->sw_tstamp = meta->sw_tstamp;
    info->length    = meta->length;

#if defined(VTSS_FEATURE_VSTAX)
    {
        if (etype == VTSS_ETYPE_VTSS && meta->bin_hdr[8 + 2 * 6 + 2] & 0x80) {
            (void)l28_vstax_frame2header(state, &meta->bin_hdr[8 + 2 * 6], &info->vstax);
            // Compose a new etype for the tag_type code below.
            etype = (meta->bin_hdr[8 /* xtr hdr */ + 2 * 6 /* 2 MAC addresses*/ + VTSS_VSTAX_HDR_SIZE] << 8) | (meta->bin_hdr[8 + 2 * 6 + VTSS_VSTAX_HDR_SIZE + 1]);
        }
    }
#endif

    info->glag_no = VTSS_GLAG_NO_NONE;
    pgid = l28_vtss_pgid(state, IFH_GET(ifh0, ifh1, PORT));
    save_pgid = pgid;
    if (pgid == state->pgid_glag_dest || pgid == (state->pgid_glag_dest + 1)) {
        /* GLAG. Return first member port or VTSS_PORT_NO_NONE */
        info->glag_no = (pgid - state->pgid_glag_dest + VTSS_GLAG_NO_START);
        pgid = VTSS_PORT_NO_NONE;
        for (port_no = VTSS_PORT_NO_START; port_no < state->port_count; port_no++)
            if (state->port_glag_no[port_no] == info->glag_no) {
                pgid = port_no;
                break;
            }
    }
    info->port_no = pgid;

    if (info->port_no == VTSS_PORT_NO_NONE) {
        VTSS_EG(trc_grp, "Unknown chip port: %u", save_pgid);
        return VTSS_RC_ERROR;
    }

    info->tag.vid     = IFH_GET(ifh0, ifh1, VID);
    info->tag.dei     = IFH_GET(ifh0, ifh1, CFI);
    info->tag.pcp     = IFH_GET(ifh0, ifh1, UPRIO);
    info->xtr_qu_mask = 1 << IFH_GET(ifh0, ifh1, XTR_QU);

    if (IFH_GET(ifh0, ifh1, FRM_TYPE) == 0x7) {
        // Frame hit an ACL rule.
        info->acl_hit = TRUE;
    }

    info->cos  = IFH_GET(ifh0, ifh1, COS);
    was_tagged = IFH_GET(ifh0, ifh1, TAGGED);
    s_tagged   = IFH_GET(ifh0, ifh1, TAG_TYPE);
    if (was_tagged) {
        // Frame was received tagged. Notice that the H/W strips both C-tags and S-tags
        // when the port is configured as VLAN-aware (there is no support for S-custom tags).
        etype = s_tagged ? VTSS_ETYPE_TAG_S : VTSS_ETYPE_TAG_C;
    }

#if defined(VTSS_FEATURE_VSTAX)
    // If it's received on a stack port, we gotta look into the VStaX header to find a tag type on the original port.
    // We cannot set some of the info->hints flag values in that case because we don't have info about the original port's setup.
    if (info->port_no == state->vstax_conf.port_0 || info->port_no == state->vstax_conf.port_1) {
        info->tag.tpid = etype; // Save it for the FDMA driver to be able to possibly re-insert a tag.
        if (was_tagged) {
            if (!s_tagged) {
                // C-tagged.
                if (etype == VTSS_ETYPE_TAG_C) {
                    info->tag_type = VTSS_TAG_TYPE_C_TAGGED;
                } else {
                    // Should be impossible
                    info->hints |= VTSS_PACKET_RX_HINTS_VLAN_TAG_MISMATCH;
                }
            } else {
                // S-tagged (S-custom not supported).
                if (etype == VTSS_ETYPE_TAG_S) {
                    info->tag_type = VTSS_TAG_TYPE_S_TAGGED;
                } else {
                    // Should only be possible transiently.
                    info->hints |= VTSS_PACKET_RX_HINTS_VLAN_TAG_MISMATCH;
                }
            }
        }
    } else
#endif
    {
        VTSS_RC(vtss_cmn_packet_hints_update(state, trc_grp, etype, info));
    }

    return VTSS_RC_OK;
}

static vtss_rc l28_tx_hdr_encode(      vtss_state_t          *const state,
                                 const vtss_packet_tx_info_t *const info,
                                       u8                    *const bin_hdr,
                                       u32                   *const bin_hdr_len)
{
    u32 ifh0, ifh1, len;

    if (bin_hdr == NULL) {
        // Caller wants us to return the number of bytes required to fill
        // in #bin_hdr. We need 8 bytes for the IFH (the CMD field is a separate word required
        // by FDMA-based injection).
        *bin_hdr_len = 8;
        return VTSS_RC_OK;
    } else if (*bin_hdr_len < 8) {
        return VTSS_RC_ERROR;
    }

    *bin_hdr_len = 8;

    ifh0 = 0;
    ifh1 = 0;

    len = info->frm_len + 4 /* For FCS */;
    IFH_PUT(ifh0, ifh1, LENGTH, len < 64 ? 64 : len);

    if (info->switch_frm) {
        // When using the CPU PM to inject the frame, the source_port field of the IFH must be set or the frame will be aged, because we don't set the timestamp in the IFH.
        vtss_prio_t cos = info->cos;
        if (cos >= 8) {
            cos = 7;
        }

        IFH_PUT(ifh0, ifh1, PORT,      VTSS_CHIP_PORT_CPU);
        IFH_PUT(ifh0, ifh1, TAG,       ((info->tag.pcp & 0x7) << 13) | ((info->tag.dei & 0x1) << 12) | (info->tag.vid & 0xFFF));
        // cos is specified as a number in range [0; 7], but Lu28 only accepts [0; 3], so here we simply right shift by one to let the three MSbits rule.
        IFH_PUT(ifh0, ifh1, COS,       cos >> 1);
        IFH_PUT(ifh0, ifh1, AGGR_CODE, info->aggr_code);
    }

#ifdef VTSS_FEATURE_VSTAX
    else if (info->tx_vstax_hdr != VTSS_PACKET_TX_VSTAX_NONE) {
        vtss_port_no_t port_no;
        u32            chip_port, chip_port_mask;

        VTSS_RC(vtss_cmn_bit_from_one_hot_mask64(info->dst_port_mask, &port_no));
        if (port_no >= state->port_count) {
            return VTSS_RC_ERROR;
        }
        IFH_PUT(ifh0, ifh1, VSTAX, 1);

        chip_port = state->port_map[port_no].chip_port;
        chip_port_mask = 1UL << chip_port;
        if (chip_port_mask & state->vstax_info.chip_info[0].mask_a) {
            IFH_PUT(ifh0, ifh1, STACK_A, 1);
        } else if (chip_port_mask & state->vstax_info.chip_info[0].mask_b) {
            IFH_PUT(ifh0, ifh1, STACK_B, 1);
        } else {
            // Not a stack port.
            return VTSS_RC_ERROR;
        }
    }
#endif

    bin_hdr[0] = ifh0 >>  0;
    bin_hdr[1] = ifh0 >>  8;
    bin_hdr[2] = ifh0 >> 16;
    bin_hdr[3] = ifh0 >> 24;
    bin_hdr[4] = ifh1 >>  0;
    bin_hdr[5] = ifh1 >>  8;
    bin_hdr[6] = ifh1 >> 16;
    bin_hdr[7] = ifh1 >> 24;

    VTSS_IG(VTSS_TRACE_GROUP_PACKET, "IFH:");
    VTSS_IG_HEX(VTSS_TRACE_GROUP_PACKET, &bin_hdr[0], *bin_hdr_len);

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Security
 * ================================================================= */

static vtss_rc l28_acl_cmd(u32 cmd, u32 index)
{
    u32 busy;

    HT_WR(ACL, S_ACL, UPDATE_CTRL, 
          (0<<20) | 
          (1<<17) |
          ((index < VTSS_L28_IS2_CNT ? 1 : 0)<<16) | 
          (index<<4) | 
          (cmd<<1) | 
          (1<<0));
    
    /* Wait until idle */
    while (1) {
        HT_RDF(ACL, S_ACL, UPDATE_CTRL, 0, 0x1, &busy);
        if (!busy)
            break;
    }
    return VTSS_RC_OK;
}

/* Setup action */
static vtss_rc l28_acl_action_set(const vtss_acl_action_t *action, u32 counter)
{
    HT_WR(ACL, S_ACL, IN_VLD, 1<<0);
    HT_WR(ACL, S_ACL, IN_MISC_CFG,
          ((action->cpu_once ? 1 : 0)<<20) |
          (((action->cpu || action->cpu_once) ? 
            (action->cpu_queue - VTSS_PACKET_RX_QUEUE_START) : 0)<<16) | 
          ((action->cpu ? 1 : 0)<<12) |
          ((action->police ? (action->policer_no - VTSS_ACL_POLICER_NO_START) : 0)<<4) |
          ((action->police ? 1 : 0)<<3) |
          ((action->learn ? 1 : 0)<<1) |
          ((action->forward ? 1 : 0)<<0));
    HT_WR(ACL, S_ACL, IN_REDIR_CFG,
          ((action->port_forward ? l28_chip_pgid(action->port_no) : 0)<<4) |
          ((action->port_forward ? 1 : 0)<<0));
    HT_WR(ACL, S_ACL, IN_CNT, counter);

    return VTSS_RC_OK;
}

static vtss_rc l28_acl_policer_set(const vtss_acl_policer_no_t policer_no)
{
    u32 unit, policer, rate;

    policer = (policer_no - VTSS_ACL_POLICER_NO_START);
    rate = vtss_state->acl_policer_conf[policer].rate;
    HT_WR(ANALYZER, 0, ACLPOLIDX, policer);
    HT_WRF(ANALYZER, 0, STORMLIMIT, 24, 0xf, l28_packet_rate(rate, &unit));
    HT_WRF(ANALYZER, 0, STORMLIMIT_ENA, 6, 0x1, 1);
    HT_WRF(ANALYZER, 0, STORMPOLUNIT, 6 + policer, 0x1, unit);
    
    return VTSS_RC_OK;
}

static vtss_rc l28_acl_port_conf_set(u32                  port,
                                     vtss_acl_port_conf_t *conf)
{
    u32 enabled = VTSS_BOOL(conf->policy_no != VTSS_ACL_POLICY_NO_NONE);
    u32 counter;

    /* Enable/disable ACL on port */
    HT_WRF(PORT, port, MISCCFG, 2, 0x1, enabled);

    /* Policy */
    if (enabled) {
        HT_WR(ACL, S_ACL, PAG_CFG + port, 
              ((conf->policy_no - VTSS_ACL_POLICY_NO_START)<<(ACL_PAG_WIDTH - 3)) | 
              (port<<0));
    }

    /* Action */
    VTSS_RC(l28_acl_cmd(ACL_CMD_READ, VTSS_L28_IS2_CNT + port));
    HT_RD(ACL, S_ACL, IN_CNT, &counter);
    VTSS_RC(l28_acl_action_set(&conf->action, counter));
    VTSS_RC(l28_acl_cmd(ACL_CMD_WRITE, VTSS_L28_IS2_CNT + port));
    
    return VTSS_RC_OK;
}

static vtss_rc l28_acl_port_set(const vtss_port_no_t port_no)
{
    vtss_acl_port_conf_t *conf = &vtss_state->acl_port_conf[port_no];
    u32                  port = VTSS_CHIP_PORT(port_no);

    VTSS_RC(l28_acl_port_conf_set(port, conf));
    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Setup aggregated port */
        VTSS_RC(l28_acl_port_conf_set(port, conf));
    }    
    return VTSS_RC_OK;
}

static vtss_rc l28_acl_port_cnt_get(u32 port, u32 *value)
{
    VTSS_RC(l28_acl_cmd(ACL_CMD_READ, VTSS_L28_IS2_CNT + port));
    HT_RD(ACL, S_ACL, IN_CNT, value);
    return VTSS_RC_OK;
}

static vtss_rc l28_acl_port_counter_get(const vtss_port_no_t    port_no,
                                        vtss_acl_port_counter_t *const counter)
{
    u32 value, port = VTSS_CHIP_PORT(port_no);

    VTSS_RC(l28_acl_port_cnt_get(port, &value));
    *counter = value;
    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Read aggregated port */
        VTSS_RC(l28_acl_port_cnt_get(port, &value));
        *counter += value;
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_acl_port_cnt_clear(u32 port)
{
    VTSS_RC(l28_acl_cmd(ACL_CMD_READ, VTSS_L28_IS2_CNT + port));
    HT_WR(ACL, S_ACL, IN_CNT, 0);
    VTSS_RC(l28_acl_cmd(ACL_CMD_WRITE, VTSS_L28_IS2_CNT + port));
    return VTSS_RC_OK;
}

static vtss_rc l28_acl_port_counter_clear(const vtss_port_no_t port_no)
{
    u32 port = VTSS_CHIP_PORT(port_no);

    VTSS_RC(l28_acl_port_cnt_clear(port));
    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Clear aggregated port */
        VTSS_RC(l28_acl_port_cnt_clear(port));
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_ace_add(const vtss_ace_id_t ace_id, const vtss_ace_t *const ace)
{
    vtss_vcap_obj_t          *is2_obj = &vtss_state->is2.obj;
    vtss_vcap_user_t         is2_user = VTSS_IS2_USER_ACL;
    vtss_vcap_data_t         data;
    vtss_is2_data_t          *is2 = &data.u.is2;
    vtss_is2_entry_t         entry;
    vtss_ace_t               *ace_copy = &entry.ace;
    const vtss_ace_udp_tcp_t *sport = NULL, *dport = NULL;
    
    /*** Step 1: Check the simple things */
    VTSS_RC(vtss_cmn_ace_add(ace_id, ace));

    /*** Step 2: Check if IS2 entries can be added */
    memset(&data, 0, sizeof(data));
    is2->srange = VTSS_VCAP_RANGE_CHK_NONE;
    is2->drange = VTSS_VCAP_RANGE_CHK_NONE;
    VTSS_RC(vtss_vcap_add(is2_obj, is2_user, ace->id, ace_id, &data, 1));
    
    /*** Step 3: Allocate range checkers */
    /* Free eventually old range checkers - There is no need to commit when freeing a range checker */
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is2->srange));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is2->drange));

    if (ace->type == VTSS_ACE_TYPE_IPV4 && vtss_vcap_udp_tcp_rule(&ace->frame.ipv4.proto)) {
        sport = &ace->frame.ipv4.sport;
        dport = &ace->frame.ipv4.dport;
    } 

    if (sport && dport) {
        /* Make a temporary working copy of the range table */
        vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range; 
        VTSS_RC(vtss_vcap_udp_tcp_range_alloc(&range_new, &is2->srange, sport, 1));
        VTSS_RC(vtss_vcap_udp_tcp_range_alloc(&range_new, &is2->drange, dport, 0));
        if (memcmp(&vtss_state->vcap_range, &range_new, sizeof(range_new))) {
            /* The temporary working copy has changed - Save it and commit */
            vtss_state->vcap_range = range_new;
            VTSS_RC(l28_vcap_range_commit());
        }
    }

    /*** Step 4: Add IS2 entry */
    *ace_copy = *ace;
    is2->entry = &entry;
    VTSS_RC(vtss_vcap_add(is2_obj, is2_user, ace->id, ace_id, &data, 0));

    return VTSS_RC_OK;
}

/* Write ACE data and mask */
#define L28_ACE_WR(reg, data, mask) \
{ \
    vtss_rc rc; \
    u32 val = data; \
    u32 msk = ~(mask); \
    if ((rc = ht_rd_wr(B_ACL, S_ACL, R_ACL_##reg + ACE_DATA_OFFS, &val, 1)) != VTSS_RC_OK) \
        return rc; \
    if ((rc = ht_rd_wr(B_ACL, S_ACL, R_ACL_##reg + ACE_MASK_OFFS, &msk, 1)) != VTSS_RC_OK) \
        return rc; \
}

/* Convert to 32-bit MAC data/mask register values */
#define L28_ACE_MAC(mac_hi, mac_lo, mac) \
 mac_hi[0] = ((mac.value[0]<<8) | mac.value[1]); \
 mac_hi[1] = ((mac.mask[0]<<8) | mac.mask[1]); \
 mac_lo[0] = ((mac.value[2]<<24) | (mac.value[3]<<16) | (mac.value[4]<<8) | mac.value[5]); \
 mac_lo[1] = ((mac.mask[2]<<24) | (mac.mask[3]<<16) | (mac.mask[4]<<8) | mac.mask[5]);

/* Convert to 32-bit IPv6 data/mask registers */
#define L28_ACE_IPV6(ipv6, addr, i) \
 ipv6[0] = ((addr.value[i+0]<<24) | (addr.value[i+1]<<16) | \
            (addr.value[i+2]<<8) | addr.value[i+3]); \
 ipv6[1] = ((addr.mask[i+0]<<24) | (addr.mask[i+1]<<16) | \
            (addr.mask[i+2]<<8) | addr.mask[i+3]);

/* Determine if ACE is UDP/TCP entry */
#define L28_ACE_UDP_TCP(ace) \
 ((ace)->type == VTSS_ACE_TYPE_IPV4 && (ace)->frame.ipv4.proto.mask == 0xff && \
  ((ace)->frame.ipv4.proto.value == 6 || (ace)->frame.ipv4.proto.value == 17) ? 1 : 0)

/* Commit VCAP range checkers */
static vtss_rc l28_vcap_range_commit(void)
{
    u32                   i, sport, dport;
    vtss_vcap_range_chk_t *entry;
    
    for (i = 0; i < VTSS_VCAP_RANGE_CHK_CNT; i++) {
        entry = &vtss_state->vcap_range.entry[i];
        if (entry->count == 0)
            continue;
        switch (entry->type) {
        case VTSS_VCAP_RANGE_TYPE_SPORT:
            sport = 1;
            dport = 0;
            break;
        case VTSS_VCAP_RANGE_TYPE_DPORT:
            sport = 0;
            dport = 1;
            break;
        case VTSS_VCAP_RANGE_TYPE_SDPORT:
            sport = 1;
            dport = 1;
            break;
        default:
            VTSS_E("illegal type: %d", entry->type);
            return VTSS_RC_ERROR;
        }
        HT_WR(ACL, S_ACL, TCP_RNG_ENA_CFG_0 + 2*i, (sport<<1) | (dport<<0));
        HT_WR(ACL, S_ACL, TCP_RNG_VALUE_CFG_0 + 2*i, (entry->max<<16) | entry->min);
    }
    return VTSS_RC_OK;
}

/* Convert to table index */
static u32 l28_is2_index(u32 index)
{
    return (VTSS_L28_IS2_CNT - index - 1);
}

static vtss_rc l28_is2_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    u32 i = l28_is2_index(idx->row);

    VTSS_RC(l28_acl_cmd(ACL_CMD_READ, i));;
    HT_RD(ACL, S_ACL, IN_CNT, counter);
    if (clear) {
        HT_WR(ACL, S_ACL, IN_CNT, 0);
        VTSS_RC(l28_acl_cmd(ACL_CMD_WRITE, i));
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_is2_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter)
{
    u32                misc[2], smac_hi[2], smac_lo[2], dmac_hi[2], dmac_lo[2];
    u32                l3_misc[2], l3_hi[2], l3_lo[2], ipv6[2];
    u32                type, type_mask, srange, drange;
    BOOL               udp_tcp = 0, tcp = 0;
    vtss_ace_udp_tcp_t *sport, *dport;
    u8                 smask, dmask;
    vtss_ace_t         *ace = &data->u.is2.entry->ace; 

    VTSS_I("row: %u", idx->row);
    
    /* Determine frame type and mask */
    type_mask = 0x0;
    l3_misc[0] = 0; /* Avoid compiler warning */
    l3_misc[1] = 0; /* Avoid compiler warning */
    switch (ace->type) {
    case VTSS_ACE_TYPE_ANY:
        type = ACL_TYPE_IPV6;
        type_mask = 0x7;
        memset(&ace->frame, 0, sizeof(ace->frame));
        break;
    case VTSS_ACE_TYPE_ETYPE:
        type = ACL_TYPE_ETYPE;
        break;
    case VTSS_ACE_TYPE_LLC:
        type = ACL_TYPE_LLC;
        break;
    case VTSS_ACE_TYPE_SNAP:
        type = ACL_TYPE_SNAP;
        break;
    case VTSS_ACE_TYPE_ARP:
        type = ACL_TYPE_ARP;
        break;
    case VTSS_ACE_TYPE_IPV4:
        udp_tcp = L28_ACE_UDP_TCP(ace);
        tcp = (udp_tcp && ace->frame.ipv4.proto.value == 6);
        if (udp_tcp) { 
            /* UDP/TCP */
            type = ACL_TYPE_UDP_TCP;
        } else {
            /* IPv4 */
            type = ACL_TYPE_IPV4;
            type_mask = 0x1;
        }
        l3_misc[0] = ((ace->frame.ipv4.proto.value<<12) | 
                      (ace->frame.ipv4.ds.value<<4) |
                      ((ace->frame.ipv4.options == VTSS_ACE_BIT_1 ? 1 : 0)<<2) |
                      ((ace->frame.ipv4.fragment == VTSS_ACE_BIT_1 ? 1 : 0)<<1) |
                      ((ace->frame.ipv4.ttl == VTSS_ACE_BIT_1 ? 1 : 0)<<0));
        l3_misc[1] = ((ace->frame.ipv4.proto.mask<<12) | 
                      (ace->frame.ipv4.ds.mask<<4) |
                      ((ace->frame.ipv4.options == VTSS_ACE_BIT_ANY ? 0 : 1)<<2) |
                      ((ace->frame.ipv4.fragment == VTSS_ACE_BIT_ANY ? 0 : 1)<<1) |
                      ((ace->frame.ipv4.ttl == VTSS_ACE_BIT_ANY ? 0 : 1)<<0));
        break;
    case VTSS_ACE_TYPE_IPV6:
        type = ACL_TYPE_IPV6;
        break;
    default:
        VTSS_E("illegal type: %d", ace->type);
        return VTSS_RC_ERROR;
    }
    
    misc[0] = (((ace->dmac_bc == VTSS_ACE_BIT_1 ? 1 : 0)<<29) |
               ((ace->dmac_mc == VTSS_ACE_BIT_1 ? 1 : 0)<<28) |
               ((ace->vlan.usr_prio.value & 0x7)<<24) |
               ((ace->vlan.vid.value & 0xfff)<<12) |
               ((ace->policy.value & 0x7)<<(ACL_PAG_WIDTH + 1)) | 
               ((ace->port_no == VTSS_PORT_NO_ANY ? 0 : VTSS_CHIP_PORT(ace->port_no))<<4) |
               ((ace->vlan.cfi == VTSS_ACE_BIT_1 ? 1 : 0)<<2) |
               (1<<1) |
               (1<<0));
    misc[1] = (((ace->dmac_bc == VTSS_ACE_BIT_ANY ? 0 : 1)<<29) |
               ((ace->dmac_mc == VTSS_ACE_BIT_ANY ? 0 : 1)<<28) |
               ((ace->vlan.usr_prio.mask & 0x7)<<24) |
               ((ace->vlan.vid.mask & 0xfff)<<12) |
               ((ace->policy.mask & 0x7)<<(ACL_PAG_WIDTH + 1)) | 
               (((ace->port_no == VTSS_PORT_NO_ANY ? 0 : ((1<<(ACL_PAG_WIDTH - 3)) - 1)))<<4) |
               ((ace->vlan.cfi == VTSS_ACE_BIT_ANY ? 0 : 1)<<2) |
               (1<<1) |
               (1<<0));
    
    if (type_mask != 0) {
        /* Update type mask before writing data mask */
        HT_WR(ACL, S_ACL, UPDATE_CTRL, 
              (type_mask<<20) | 
              (0<<17) |
              (0<<16) | 
              (0<<4) | 
              (0<<1) | 
              (0<<0));
    }
    
    switch (type) {
    case ACL_TYPE_ETYPE:
        L28_ACE_WR(ETYPE_TYPE, misc[0], misc[1]);
        L28_ACE_MAC(smac_hi, smac_lo, ace->frame.etype.smac);
        L28_ACE_MAC(dmac_hi, dmac_lo, ace->frame.etype.dmac);
        L28_ACE_WR(ETYPE_L2_SMAC_HIGH, smac_hi[0], smac_hi[1]);
        L28_ACE_WR(ETYPE_L2_SMAC_LOW, smac_lo[0], smac_lo[1]);
        L28_ACE_WR(ETYPE_L2_DMAC_HIGH, dmac_hi[0], dmac_hi[1]);
        L28_ACE_WR(ETYPE_L2_DMAC_LOW, dmac_lo[0], dmac_lo[1]);
        L28_ACE_WR(ETYPE_L2_ETYPE, 
                   (ace->frame.etype.data.value[0]<<24) |
                   (ace->frame.etype.data.value[1]<<16) |
                   (ace->frame.etype.etype.value[0]<<8) |
                   (ace->frame.etype.etype.value[1]),
                   ((ace->frame.etype.data.mask[0]<<24) |
                    (ace->frame.etype.data.mask[1]<<16) |
                    (ace->frame.etype.etype.mask[0]<<8) |
                    (ace->frame.etype.etype.mask[1])));
        break;
    case ACL_TYPE_LLC:
        L28_ACE_WR(LLC_TYPE, misc[0], misc[1]);
        L28_ACE_MAC(smac_hi, smac_lo, ace->frame.llc.smac);
        L28_ACE_MAC(dmac_hi, dmac_lo, ace->frame.llc.dmac);
        L28_ACE_WR(LLC_L2_SMAC_HIGH, smac_hi[0], smac_hi[1]);
        L28_ACE_WR(LLC_L2_SMAC_LOW, smac_lo[0], smac_lo[1]);
        L28_ACE_WR(LLC_L2_DMAC_HIGH, dmac_hi[0], dmac_hi[1]);
        L28_ACE_WR(LLC_L2_DMAC_LOW, dmac_lo[0], dmac_lo[1]);
        L28_ACE_WR(LLC_L2_LLC, 
                   (ace->frame.llc.llc.value[0]<<24) |
                   (ace->frame.llc.llc.value[1]<<16) |
                   (ace->frame.llc.llc.value[2]<<8) |
                   ace->frame.llc.llc.value[3],
                   (ace->frame.llc.llc.mask[0]<<24) |
                   (ace->frame.llc.llc.mask[1]<<16) |
                   (ace->frame.llc.llc.mask[2]<<8) |
                   ace->frame.llc.llc.mask[3]);
        break;
    case ACL_TYPE_SNAP:
        L28_ACE_WR(SNAP_TYPE, misc[0], misc[1]);
        L28_ACE_MAC(smac_hi, smac_lo, ace->frame.snap.smac);
        L28_ACE_MAC(dmac_hi, dmac_lo, ace->frame.snap.dmac);
        L28_ACE_WR(SNAP_L2_SMAC_HIGH, smac_hi[0], smac_hi[1]);
        L28_ACE_WR(SNAP_L2_SMAC_LOW, smac_lo[0], smac_lo[1]);
        L28_ACE_WR(SNAP_L2_DMAC_HIGH, dmac_hi[0], dmac_hi[1]);
        L28_ACE_WR(SNAP_L2_DMAC_LOW, dmac_lo[0], dmac_lo[1]);
        L28_ACE_WR(SNAP_L2_SNAP_HIGH, 
                   ace->frame.snap.snap.value[0], ace->frame.snap.snap.mask[0]);
        L28_ACE_WR(SNAP_L2_SNAP_LOW, 
                   (ace->frame.snap.snap.value[1]<<24) |
                   (ace->frame.snap.snap.value[2]<<16) |
                   (ace->frame.snap.snap.value[3]<<8) |
                   ace->frame.snap.snap.value[4],
                   (ace->frame.snap.snap.mask[1]<<24) |
                   (ace->frame.snap.snap.mask[2]<<16) |
                   (ace->frame.snap.snap.mask[3]<<8) |
                   ace->frame.snap.snap.mask[4]);
        break;
    case ACL_TYPE_ARP:
        L28_ACE_WR(ARP_TYPE, misc[0], misc[1]);
        L28_ACE_MAC(smac_hi, smac_lo, ace->frame.arp.smac);
        L28_ACE_WR(ARP_L2_SMAC_HIGH, smac_hi[0], smac_hi[1]);
        L28_ACE_WR(ARP_L2_SMAC_LOW, smac_lo[0], smac_lo[1]);
        L28_ACE_WR(ARP_L3_ARP, 
                   ((ace->frame.arp.ethernet == VTSS_ACE_BIT_1 ? 1 : 0)<<7) |
                   ((ace->frame.arp.ip == VTSS_ACE_BIT_1 ? 1 : 0)<<6) |
                   ((ace->frame.arp.length == VTSS_ACE_BIT_1 ? 1 : 0)<<5) |
                   ((ace->frame.arp.dmac_match == VTSS_ACE_BIT_1 ? 1 : 0)<<4) |
                   ((ace->frame.arp.smac_match == VTSS_ACE_BIT_1 ? 1 : 0)<<3) |
                   ((ace->frame.arp.unknown == VTSS_ACE_BIT_1 ? 1 : 0)<<2) |
                   ((ace->frame.arp.arp == VTSS_ACE_BIT_1 ? 0 : 1)<<1) |
                   ((ace->frame.arp.req == VTSS_ACE_BIT_1 ? 0 : 1)<<0),
                   ((ace->frame.arp.ethernet == VTSS_ACE_BIT_ANY ? 0 : 1)<<7) |
                   ((ace->frame.arp.ip == VTSS_ACE_BIT_ANY ? 0 : 1)<<6) |
                   ((ace->frame.arp.length == VTSS_ACE_BIT_ANY ? 0 : 1)<<5) |
                   ((ace->frame.arp.dmac_match == VTSS_ACE_BIT_ANY ? 0 : 1)<<4) |
                   ((ace->frame.arp.smac_match == VTSS_ACE_BIT_ANY ? 0 : 1)<<3) |
                   ((ace->frame.arp.unknown == VTSS_ACE_BIT_ANY ? 0 : 1)<<2) |
                   ((ace->frame.arp.arp == VTSS_ACE_BIT_ANY ? 0 : 1)<<1) |
                   ((ace->frame.arp.req == VTSS_ACE_BIT_ANY ? 0 : 1)<<0));
        L28_ACE_WR(ARP_L3_IPV4_DIP, ace->frame.arp.dip.value, ace->frame.arp.dip.mask);
        L28_ACE_WR(ARP_L3_IPV4_SIP, ace->frame.arp.sip.value, ace->frame.arp.sip.mask);
        break;
    case ACL_TYPE_UDP_TCP:
        L28_ACE_WR(UDP_TCP_TYPE, misc[0], misc[1]);
        L28_ACE_WR(UDP_TCP_L3_MISC_CFG, l3_misc[0], l3_misc[1]); 
        L28_ACE_WR(UDP_TCP_L3_IPV4_DIP, ace->frame.ipv4.dip.value, ace->frame.ipv4.dip.mask);
        L28_ACE_WR(UDP_TCP_L3_IPV4_SIP, ace->frame.ipv4.sip.value, ace->frame.ipv4.sip.mask);
        sport = &ace->frame.ipv4.sport;
        dport = &ace->frame.ipv4.dport;
        srange = data->u.is2.srange;
        drange = data->u.is2.drange;
        smask = (srange == VTSS_VCAP_RANGE_CHK_NONE ? 0 : (1<<srange));
        dmask = (drange == VTSS_VCAP_RANGE_CHK_NONE ? 0 : (1<<drange));
        VTSS_I("smask: 0x%02x, dmask: 0x%02x", smask, dmask); 
        L28_ACE_WR(UDP_TCP_L4_PORT, 
                   (sport->low<<16) | dport->low,
                   ((smask == 0 && sport->low == sport->high ? 0xffff : 0)<<16) |
                   (dmask == 0 && dport->low == dport->high ? 0xffff : 0));
        L28_ACE_WR(UDP_TCP_L4_MISC,
                   ((ace->frame.ipv4.tcp_fin == VTSS_ACE_BIT_1 ? 1 : 0)<<13) |
                   ((ace->frame.ipv4.tcp_syn == VTSS_ACE_BIT_1 ? 1 : 0)<<12) |
                   ((ace->frame.ipv4.tcp_rst == VTSS_ACE_BIT_1 ? 1 : 0)<<11) |
                   ((ace->frame.ipv4.tcp_psh == VTSS_ACE_BIT_1 ? 1 : 0)<<10) |
                   ((ace->frame.ipv4.tcp_ack == VTSS_ACE_BIT_1 ? 1 : 0)<<9) |
                   ((ace->frame.ipv4.tcp_urg == VTSS_ACE_BIT_1 ? 1 : 0)<<8) |
                   (((sport->in_range ? smask : 0) | (dport->in_range ? dmask : 0))<<0),
                   ((tcp && ace->frame.ipv4.tcp_fin != VTSS_ACE_BIT_ANY ? 1 : 0)<<13) |
                   ((tcp && ace->frame.ipv4.tcp_syn != VTSS_ACE_BIT_ANY ? 1 : 0)<<12) |
                   ((tcp && ace->frame.ipv4.tcp_rst != VTSS_ACE_BIT_ANY ? 1 : 0)<<11) |
                   ((tcp && ace->frame.ipv4.tcp_psh != VTSS_ACE_BIT_ANY ? 1 : 0)<<10) |
                   ((tcp && ace->frame.ipv4.tcp_ack != VTSS_ACE_BIT_ANY ? 1 : 0)<<9) |
                   ((tcp && ace->frame.ipv4.tcp_urg != VTSS_ACE_BIT_ANY ? 1 : 0)<<8) |
                   ((smask | dmask)<<0));
        break;
    case ACL_TYPE_IPV4:
        L28_ACE_WR(IPV4_TYPE, misc[0], misc[1]);
        L28_ACE_WR(IPV4_L3_MISC_CFG, l3_misc[0], l3_misc[1]); 
        L28_ACE_WR(IPV4_L3_IPV4_DIP, ace->frame.ipv4.dip.value, ace->frame.ipv4.dip.mask);
        L28_ACE_WR(IPV4_L3_IPV4_SIP, ace->frame.ipv4.sip.value, ace->frame.ipv4.sip.mask);
        L28_ACE_MAC(l3_hi, l3_lo, ace->frame.ipv4.data);
        L28_ACE_WR(IPV4_DATA_1, l3_hi[0], l3_hi[1]);
        L28_ACE_WR(IPV4_DATA_0, l3_lo[0], l3_lo[1]);
        break;
    case ACL_TYPE_IPV6:
        L28_ACE_WR(IPV6_TYPE, misc[0], misc[1]);
        L28_ACE_WR(IPV6_L3_MISC_CFG, 
                   ace->frame.ipv6.proto.value<<8, ace->frame.ipv6.proto.mask<<8);
        L28_ACE_IPV6(ipv6, ace->frame.ipv6.sip, 0);
        L28_ACE_WR(IPV6_L3_IPV6_SIP_3, ipv6[0], ipv6[1]); 
        L28_ACE_IPV6(ipv6, ace->frame.ipv6.sip, 4);
        L28_ACE_WR(IPV6_L3_IPV6_SIP_2, ipv6[0], ipv6[1]); 
        L28_ACE_IPV6(ipv6, ace->frame.ipv6.sip, 8);
        L28_ACE_WR(IPV6_L3_IPV6_SIP_1, ipv6[0], ipv6[1]); 
        L28_ACE_IPV6(ipv6, ace->frame.ipv6.sip, 12);
        L28_ACE_WR(IPV6_L3_IPV6_SIP_0, ipv6[0], ipv6[1]); 
        break;
    default:
        VTSS_E("illegal type: %d", type);
        return VTSS_RC_ERROR;
    }
    
    /* Action */
    VTSS_RC(l28_acl_action_set(&ace->action, counter));
        
    /* Write entry */
    return l28_acl_cmd(ACL_CMD_WRITE, l28_is2_index(idx->row));
}

static vtss_rc l28_is2_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_I("row: %u", idx->row);

    /* Delete entry at index */
    HT_WR(ACL, S_ACL, ETYPE_TYPE + ACE_DATA_OFFS, 0<<0);
    HT_WR(ACL, S_ACL, ETYPE_TYPE + ACE_MASK_OFFS, 0<<0);
    HT_WR(ACL, S_ACL, IN_VLD, 0<<0);
    return l28_acl_cmd(ACL_CMD_WRITE, l28_is2_index(idx->row));
}

static vtss_rc l28_is2_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_I("row: %u, count: %u, up: %u", idx->row, count, up);

    HT_WR(ACL, S_ACL, MV_CFG, (1<<16) | (count<<0));
    return l28_acl_cmd(up ? ACL_CMD_MOVE_UP : ACL_CMD_MOVE_DOWN, 
                       l28_is2_index(idx->row + count - 1));
}

/* ================================================================= *
 *  Layer 2
 * ================================================================= */

/* Set learn mask */
static vtss_rc l28_learn_state_set(const BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    HT_WR(ANALYZER, 0, LERNMASK, l28_port_mask(member));
    return VTSS_RC_OK;
}

/* Set forwarding state (already done by vtss_update_masks) */
static vtss_rc l28_port_forward_set(const vtss_port_no_t port_no)
{
    return VTSS_RC_OK;
}

static vtss_rc l28_src_table_write(vtss_port_no_t port_no, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    u32 mask, port = VTSS_CHIP_PORT(port_no);

    mask = l28_port_mask(member);
    if (vtss_state->mirror_ingress[port_no])
        mask |= (1<<29);
    HT_WR(ANALYZER, 0, SRCMASKS + port, mask);
    
    /* Update mask for internally aggregated chip port */
    if ((port = l28_int_aggr_port(port)) != 0) {
        HT_WR(ANALYZER, 0, SRCMASKS + port, mask);
    }
    
    return VTSS_RC_OK;
}

/* Write PGID entry */
static vtss_rc l28_pgid_table_write(u32 pgid, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_port_no_t port_no;
    u32            mask;

    mask = l28_port_mask(member);

    if (pgid == vtss_state->pgid_glag_src || pgid == (vtss_state->pgid_glag_src + 1)) {
        /* Include CPU port in GLAG source masks */
        mask |= (1<<VTSS_CHIP_PORT_CPU);
    } else if (pgid == vtss_state->pgid_glag_aggr_a || pgid == vtss_state->pgid_glag_aggr_b) {
        /* GLAG aggregation masks */
        mask = 0;
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            if (member[port_no])
                mask |= (1 << (port_no - VTSS_PORT_NO_START));
    }

    pgid = l28_chip_pgid(pgid);
    HT_WR(ANALYZER, 0, DSTMASKS + pgid, mask);

    /* Update mask for internally aggregated chip port */
    if ((pgid = l28_int_aggr_port(pgid)) != 0) {
        HT_WR(ANALYZER, 0, DSTMASKS + pgid, mask);
    }

    return VTSS_RC_OK;
}

static vtss_rc l28_aggr_table_write(u32 ac, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    u32 mask = l28_port_mask(member);
#if VTSS_OPT_INT_AGGR
    u32 i, port_base, *port_next;

    for (i = 0; i < 2; i++) {
        port_base = (i == 0 ? VTSS_CHIP_PORT_AGGR_0 : VTSS_CHIP_PORT_AGGR_1);
        port_next = &vtss_state->aggr_chip_port_next[i];
        if (ac == 0)
            *port_next = port_base;
    
        /* Exclude one of the internally aggregated ports.
           It is supported that the two 5G ports can be aggregated. */
        if (mask & (1<<port_base)) {
            mask &= ~(1<<(*port_next));
            *port_next = (*port_next == port_base ? (*port_next + 1) : (*port_next - 1));
        }
    }
#endif /* VTSS_OPT_INT_AGGR */
    HT_WR(ANALYZER, 0, AGGRMSKS + ac, mask);

    return VTSS_RC_OK;
}

static vtss_rc l28_pmap_table_write(vtss_port_no_t port_no, vtss_port_no_t l_port_no)
{
    u32            port, l_port;
    vtss_glag_no_t glag_no;
    
#if VTSS_OPT_STACK_PORT_ID_INDIVIDUAL
    /* Aggregated VStaX ports use individual port numbers */
    if (vtss_state->vstax_port_enabled[port_no])
        l_port_no = port_no;
#endif /* VTSS_OPT_STACK_PORT_ID_INDIVIDUAL */
    port = VTSS_CHIP_PORT(port_no);
    l_port = VTSS_CHIP_PORT(l_port_no);

    /* Setup logical port for GLAG member */
    if ((glag_no = vtss_state->port_glag_no[port_no]) != VTSS_GLAG_NO_NONE &&
        vtss_state->port_state[port_no])
        l_port = (30 + glag_no - VTSS_GLAG_NO_START);

    HT_WRF(PORT, port, STACKCFG, 0, 0x1F, l_port);
    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Setup aggregated port */
        HT_WRF(PORT, port, STACKCFG, 0, 0x1F, l_port);
    }

    return VTSS_RC_OK;
}

static vtss_rc ht_vlan_port_conf_set(u32                         port, 
                                     BOOL                        vstax,
                                     const vtss_vlan_port_conf_t *const conf)
{
    vtss_vid_t             uvid;
    vtss_vstax_chip_info_t *chip_info = &vtss_state->vstax_info.chip_info[0];
    u32                    ttl;

    ttl = chip_info->port_conf[(VTSS_BIT(port) & chip_info->mask_a) ? 0 : 1].ttl;
    /* If VStaX is enabled, the PVID field is used for this purpose */
    HT_WRF(PORT, port, CAT_PORT_VLAN, 0, 0xFFF, 
           vstax ? (((vtss_state->vstax_conf.upsid_0)<<0) | (ttl<<5)) : conf->pvid);
    
    HT_WR(PORT, port, CAT_VLAN_MISC, 
          ((conf->aware ? 0 : 1)<<8) | 
          ((conf->aware ? 0 : 1)<<7) | 
          ((conf->stag ? 1 : 0)<<5));

    HT_WR(PORT, port, CAT_DROP, 
          (1<<6) |
          ((conf->frame_type == VTSS_VLAN_FRAME_TAGGED ? 1 : 0)<<2) |
          ((conf->frame_type == VTSS_VLAN_FRAME_UNTAGGED ? 1 : 0)<<1));

    HT_WRF(ANALYZER, 0, VLANMASK, port, 0x1, conf->ingress_filter ? 1 : 0);

    /* If VStaX is enabled, everything is sent untagged */
    uvid = (vstax ? VTSS_VID_ALL : conf->untagged_vid);
    HT_WRM(PORT, port, TXUPDCFG,
           (1<<20) |
           ((conf->untagged_vid & 0xFFF)<<4) |
           ((uvid == VTSS_VID_ALL || uvid == VTSS_VID_NULL ? 0 : 1)<<3) |
           (1<<1) |
           ((uvid == VTSS_VID_ALL ? 0 : 1)<<0),
           (1<<20) | (0xFFF<<4) | (1<<3) | (1<<1) | (1<<0));
    
    return VTSS_RC_OK;
}

static vtss_rc l28_mac_table_idle(void)
{
    u32 cmd;
    
    while (1) {
        HT_RDF(ANALYZER, 0, MACACCES, 0, 0x7, &cmd);
        if (cmd == MAC_CMD_IDLE)
            break;
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_mac_table_add(const vtss_mac_table_entry_t *const entry, u32 pgid)
{
    u32 mach, macl, mask, idx, locked = 1, aged = 1, fwd_kill = 0, ipv6_mask = 0;
    
    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    
    if (pgid == VTSS_PGID_NONE) {
        /* IPv4/IPv6 multicast entry */
        mask = l28_port_mask(vtss_state->pgid_table[pgid].member); 
        if (entry->vid_mac.mac.addr[0] == 0x01) {
            /* IPv4 multicast entry */
            /* Encode port mask directly */
            macl = ((macl & 0x00FFFFFF) | ((mask<<24) & 0xFF000000));
            mach = ((mach & 0xFFFF0000) | ((mask>>8) & 0x0000FFFF));
            idx = ((mask>>24) & 0xF);
        } else {
            /* IPv6 multicast entry */
            fwd_kill = 1;
            /* Encode port mask directly */
            mach = ((mach & 0xFFFF0000) | (mask & 0x0000FFFF));
            idx = ((mask>>16) & 0x3F);
            ipv6_mask = ((mask>>22) & 0x3F);
        }
    } else {
        /* Not IP multicast entry */
        /* Set FWD_KILL to make the switch discard frames in SMAC lookup */
        fwd_kill = (entry->copy_to_cpu || pgid != vtss_state->pgid_drop ? 0 : 1);
        idx = l28_chip_pgid(pgid);
        locked = entry->locked;
        aged = 0;
    }
    HT_WR(ANALYZER, 0, MACHDATA, mach);
    HT_WR(ANALYZER, 0, MACLDATA, macl);
    HT_WR(ANALYZER, 0, MACACCES,
          (ipv6_mask<<16) |
          (VTSS_BOOL(entry->copy_to_cpu)<<14) |
          (fwd_kill<<13) |      /* forward kill */
          (0<<12) |             /* Ignore VLAN */
          (aged<<11) |
          (1<<10) |             /* Valid */
          (locked<<9) |
          (idx<<3) |
          (MAC_CMD_LEARN<<0));
    
    return l28_mac_table_idle();
}

static vtss_rc l28_mac_table_del(const vtss_vid_mac_t *const vid_mac)
{
    u32 locked, aged, fwd_kill, mach, macl;
    
    vtss_mach_macl_get(vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    
    if (VTSS_MAC_IPV4_MC(vid_mac->mac.addr)) {
        /* IPv4 multicast */
        locked = 1;
        fwd_kill = 0;
    } else if (VTSS_MAC_IPV6_MC(vid_mac->mac.addr)) {
        /* IPv6 multicast */
        locked = 1;
        fwd_kill = 1;
    } else {
        locked = 0;
        fwd_kill = 0;
    }
    aged = locked;

    HT_WR(ANALYZER, 0, MACHDATA, mach);
    HT_WR(ANALYZER, 0, MACLDATA, macl);
    HT_WR(ANALYZER, 0, MACACCES, 
          (fwd_kill<<13) |
          (aged<<11) |
          (locked<<9) |
          (MAC_CMD_FORGET<<0));
    return l28_mac_table_idle();
}

static vtss_rc l28_mac_table_result(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32               value, mask, mach, macl, idx;
    vtss_port_no_t    port_no;
    vtss_pgid_entry_t *pgid_entry;
    u8                *mac = &entry->vid_mac.mac.addr[0];
    
    HT_RD(ANALYZER, 0, MACACCES, &value);

    /* Check if entry is valid */
    if (!(value & (1<<10))) {
        VTSS_D("not valid");
        return VTSS_RC_ERROR;
    }

    entry->copy_to_cpu = VTSS_BOOL(value & (1<<14));
    entry->aged        = VTSS_BOOL(value & (1<<11));
    entry->locked      = VTSS_BOOL(value & (1<<9));
    idx = ((value>>3) & 0x3F);

    HT_RD(ANALYZER, 0, MACHDATA, &mach);
    HT_RD(ANALYZER, 0, MACLDATA, &macl);

    if (entry->locked && entry->aged) {
        /* IPv4/IPv6 multicast address */
        *pgid = VTSS_PGID_NONE;

        /* Read encoded port mask and update address registers */
        if (value & (1<<13)) {
            /* IPv6 entry (FWD_KILL set) */
            mask = ((((value>>16) & 0x3F)<<22) | (idx<<16) | (mach & 0xffff));
            mach = ((mach & 0xffff0000) | 0x00003333);
        } else {
            /* IPv4 entry */
            mask = ((idx<<24) | ((mach<<8) & 0xFFFF00) | ((macl>>24) & 0xff));
            mach = ((mach & 0xffff0000) | 0x00000100);
            macl = ((macl & 0x00ffffff) | 0x5e000000);
        }    

        /* Convert port mask */
        pgid_entry = &vtss_state->pgid_table[*pgid];
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            pgid_entry->member[port_no] = VTSS_BOOL(mask & (1 << VTSS_CHIP_PORT(port_no)));
    } else {
        *pgid = l28_vtss_pgid(vtss_state, idx);
    }
    entry->vid_mac.vid = ((mach>>16) & 0xFFF);
    mac[0] = ((mach>>8) & 0xff);
    mac[1] = ((mach>>0) & 0xff);
    mac[2] = ((macl>>24) & 0xff);
    mac[3] = ((macl>>16) & 0xff);
    mac[4] = ((macl>>8) & 0xff);
    mac[5] = ((macl>>0) & 0xff);
    VTSS_D("found 0x%08x%08x", mach, macl);

    return VTSS_RC_OK;
}

static vtss_rc l28_mac_table_get(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32 mach, macl;

    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    HT_WR(ANALYZER, 0, MACHDATA, mach);
    HT_WR(ANALYZER, 0, MACLDATA, macl);
    HT_WR(ANALYZER, 0, MACACCES, 
          (1<<10) | /* Valid set means LOOKUP */
          (MAC_CMD_READ<<0));
    VTSS_RC(l28_mac_table_idle());
    return l28_mac_table_result(entry, pgid);
}

static vtss_rc l28_mac_table_get_next(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32 mach, macl;

    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    HT_WR(ANALYZER, 0, ANAGEFIL, 0);
    HT_WR(ANALYZER, 0, MACHDATA, mach);
    HT_WR(ANALYZER, 0, MACLDATA, macl);
    HT_WR(ANALYZER, 0, MACACCES, MAC_CMD_GET_NEXT<<0);
    VTSS_RC(l28_mac_table_idle());
    return l28_mac_table_result(entry, pgid);
}

static vtss_rc l28_mac_table_age_time_set(void)
{
    u32 time;
    
    /* Scan two times per age time */
    time = (vtss_state->mac_age_time/2);
    if (time > 0xfffff)
        time = 0xfffff;
    HT_WR(ANALYZER, 0, AUTOAGE, time);
    return VTSS_RC_OK;
}

static vtss_rc l28_mac_table_age(BOOL             pgid_age, 
                                 u32              pgid,
                                 BOOL             vid_age,
                                 const vtss_vid_t vid)
{
    HT_WR(ANALYZER, 0, ANAGEFIL, 
          (pgid_age<<31) | 
          (l28_chip_pgid(pgid)<<16) | 
          (vid_age<<15) | 
          (vid<<0));
    HT_WR(ANALYZER, 0, MACACCES, MAC_CMD_TABLE_AGE<<0);
    return l28_mac_table_idle();
}

static vtss_rc l28_mac_table_status_get(vtss_mac_table_status_t *status) 
{
    u32 value;

    /* Read and clear sticky register */
    HT_RD(ANALYZER, 0, ANEVENTS, &value);
    HT_WR(ANALYZER, 0, ANEVENTS, value & ((1<<15) | (1<<16) | (1<<17) | (1<<20)));
    
    /* Detect learn events */
    status->learned = VTSS_BOOL(value & (1<<16));
    
    /* Detect replace events */
    status->replaced = VTSS_BOOL(value & (1<<17));

    /* Detect port move events */
    status->moved = VTSS_BOOL(value & (1<<15));
    
    /* Detect age events */
    status->aged = VTSS_BOOL(value & (1<<20));

    return VTSS_RC_OK;
}

static vtss_rc l28_learn_mode_set(u32 port, vtss_learn_mode_t *mode)
{
    HT_WRF(ANALYZER, 0, LEARNDROP, port, 0x1, mode->discard);
    HT_WRF(ANALYZER, 0, LEARNAUTO, port, 0x1, mode->automatic);
    HT_WRF(ANALYZER, 0, LEARNCPU, port, 0x1, mode->cpu);
    
    return VTSS_RC_OK;
}

static vtss_rc l28_learn_port_mode_set(const vtss_port_no_t port_no)
{
    vtss_learn_mode_t *mode = &vtss_state->learn_mode[port_no];
    u32               learn_mask, mask, port = VTSS_CHIP_PORT(port_no);

    mask = (1 << port);
    VTSS_RC(l28_learn_mode_set(port, mode));
    
    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Setup aggregated port */
        mask |= (1 << port);
        VTSS_RC(l28_learn_mode_set(port, mode));
    }
    
    if (!mode->automatic) {
        /* Flush entries previously learned on port to avoid continuous refreshing */
        HT_RD(ANALYZER, 0, LERNMASK, &learn_mask);
        HT_WR(ANALYZER, 0, LERNMASK, learn_mask & ~mask);
        l28_mac_table_age(1, port_no, 0, 0);
        l28_mac_table_age(1, port_no, 0, 0);
        HT_WR(ANALYZER, 0, LERNMASK, learn_mask);
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_vlan_port_conf_update(vtss_port_no_t port_no, vtss_vlan_port_conf_t  *conf)
{
    u32  port = VTSS_CHIP_PORT(port_no);
    BOOL vstax = vtss_state->vstax_port_enabled[port_no];
    
    VTSS_RC(ht_vlan_port_conf_set(port, vstax, conf));
    
    if ((port = l28_int_aggr_port(port)) != 0) {
        /* Setup aggregated port */
        VTSS_RC(ht_vlan_port_conf_set(port, vstax, conf));
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_vlan_table_idle(void)
{
    u32 cmd;

    while (1) {
        HT_RDF(ANALYZER, 0, VLANACES, 0, 0x3, &cmd);
        if (cmd == VLAN_CMD_IDLE)
            break;
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_vlan_mask_update(vtss_vid_t vid, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_vlan_entry_t *vlan_entry = &vtss_state->vlan_table[vid];
    u32               mask;

    /* Check if mask has changed */
    if ((mask = l28_port_mask(member)) == vlan_entry->mask)
        return VTSS_RC_OK;

    /* Update state */
    VTSS_N("vid %d: mask 0x%08x => 0x%08x", vid, vlan_entry->mask, mask);
    vlan_entry->mask = mask;

    /* Update hardware */
    HT_WR(ANALYZER, 0, VLANTINDX, 
          (vid<<0) |
          (VTSS_BOOL(vlan_entry->isolated)<<15));
    HT_WR(ANALYZER, 0, VLANACES, 
          (mask<<2) |
          (VLAN_CMD_WRITE<<0));

    return l28_vlan_table_idle();
}

static vtss_rc l28_isolated_vlan_set(const vtss_vid_t vid)
{
    /* Force VLAN update */
    vtss_state->vlan_table[vid].mask = VTSS_CHIP_MASK_UPDATE;
    return vtss_cmn_vlan_members_set(vid);
}

static vtss_rc l28_isolated_port_members_set(void)
{
    vtss_port_no_t port_no;
    BOOL           port_member[VTSS_PORT_ARRAY_SIZE];

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        port_member[port_no] = (vtss_state->isolated_port[port_no] ? 0 : 1);
    
    HT_WR(ANALYZER, 0, PVLAN_MASK, l28_port_mask(port_member) | (1<<VTSS_CHIP_PORT_CPU));

    return VTSS_RC_OK;
}

static vtss_rc l28_aggr_mode_set(void)
{
    vtss_aggr_mode_t *mode = &vtss_state->aggr_mode;
    u32              value;
    vtss_port_no_t   port_no;
    u32              port;

    /* Setup analyzer */
    HT_WR(ANALYZER, 0, AGGRCNTL,
          ((mode->sip_dip_enable || mode->sport_dport_enable ? 1 : 0)<<2) |
          ((mode->dmac_enable ? 1 : 0)<<1) |
          ((mode->smac_enable ? 1 : 0)<<0));

    /* Setup categorizer */
    value = (((mode->sip_dip_enable ? 1 : 0)<<1) |
             ((mode->sport_dport_enable ? 1 : 0)<<0));
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        port = VTSS_CHIP_PORT(port_no);
        HT_WRF(PORT, port, CAT_OTHER_CFG, 0, 0x3, value);
        if ((port = l28_int_aggr_port(port)) != 0) {
            /* Setup aggregated port */
            HT_WRF(PORT, port, CAT_OTHER_CFG, 0, 0x3, value);
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_mirror_port_set(void)
{
    BOOL                   member[VTSS_PORT_ARRAY_SIZE];
    vtss_port_no_t         port_no;
    vtss_vstax_chip_info_t *chip_info = &vtss_state->vstax_info.chip_info[0];

    /* Calculate mirror ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        member[port_no] = (port_no == vtss_state->mirror_conf.port_no);
        
        /* Include VStaX ports enabled for mirroring */
        if (vtss_state->vstax_port_enabled[port_no] &&
            chip_info->port_conf[port_no == vtss_state->vstax_conf.port_0 ? 0 : 1].mirror)
            member[port_no] = 1;
    }
    HT_WR(ANALYZER, 0, MIRRORPORTS, l28_port_mask(member));
    
#if defined(VTSS_FEATURE_NPI)
    /* If NPI is enabled and the mirror port is disabled, frames to CPU are mirrored */
    HT_WRF(ANALYZER, 0, AGENCNTL, 8, 0x1, 
           vtss_state->npi_conf.enable && 
           vtss_state->mirror_conf.port_no == VTSS_PORT_NO_NONE ? 1 : 0);
#endif /* VTSS_FEATURE_NPI */

    /* Update VLAN port configuration for all ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
    }

    /* Update all VLANs */
    return vtss_cmn_vlan_update_all();
}

static vtss_rc l28_mirror_ingress_set(void)
{
    vtss_port_no_t port_no;
    u32            port, enabled;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        enabled = vtss_state->mirror_ingress[port_no];
        port = VTSS_CHIP_PORT(port_no);
        HT_WRF(ANALYZER, 0, SRCMASKS + port, 29, 0x1, enabled);
        if ((port = l28_int_aggr_port(port)) != 0) {
            /* Setup aggregated port */
            HT_WRF(ANALYZER, 0, SRCMASKS + port, 29, 0x1, enabled);
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_mirror_egress_set(void)
{
    HT_WR(ANALYZER, 0, EMIRRORMASK, l28_port_mask(vtss_state->mirror_egress));
    return VTSS_RC_OK;
}

static vtss_rc l28_flood_conf_set(void)
{
    vtss_port_no_t port_no;
    u32            port, scope = vtss_state->ipv6_mc_scope;

    /* Unicast flood mask */
    HT_WR(ANALYZER, 0, UFLODMSK, l28_port_mask(vtss_state->uc_flood));
    
    /* Multicast flood mask */
    HT_WR(ANALYZER, 0, MFLODMSK, l28_port_mask(vtss_state->mc_flood));

    /* IPv4 MC flood mask */
    HT_WR(ANALYZER, 0, IFLODMSK, l28_port_mask(vtss_state->ipv4_mc_flood));

    /* IPv6 MC flood mask */
    HT_WR(ANALYZER, 0, I6FLODMSK, l28_port_mask(vtss_state->ipv6_mc_flood));
    
    /* IPv6 flood scope */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        port = VTSS_CHIP_PORT(port_no);
        HT_WRF(PORT, port, CAT_OTHER_CFG, 3, 0x1, scope);
        if ((port = l28_int_aggr_port(port)) != 0) {
            /* Setup aggregated port */
            HT_WRF(PORT, port, CAT_OTHER_CFG, 3, 0x1, scope);
        }
    }

    return VTSS_RC_OK;
}

static vtss_rc l28_vstax_port_set(u32 port)
{
    vtss_vstax_chip_info_t *chip_info = &vtss_state->vstax_info.chip_info[0];
    u32                    mask, enabled, stack_port_b;

    mask = (1 << port);
    enabled = VTSS_BOOL(mask & (chip_info->mask_a | chip_info->mask_b));
    stack_port_b = VTSS_BOOL(mask & chip_info->mask_b);

    HT_WRM(PORT, port, STACKCFG, 
           (1<<11) | (1<<10) | (1<<9) | (1<<8) | (1<<6) | (1<<5),
           (0<<11) |
           (1<<10) |
           ((enabled && chip_info->port_conf[stack_port_b].mirror ? 1 : 0)<<9) |
           (stack_port_b<<8) |
           (enabled<<6) |
           (enabled<<5));
    
    HT_WR(PORT, port, MAXLEN, enabled ? (VTSS_MAX_FRAME_LENGTH_MAX + VTSS_VSTAX_HDR_SIZE) :
          vtss_state->port_conf[port].max_frame_length);
    
    return VTSS_RC_OK;
}

static vtss_rc l28_vstax_conf_set(void)
{
    vtss_port_no_t         port_no;
    vtss_vstax_chip_info_t *chip_info = &vtss_state->vstax_info.chip_info[0];
    u32                    port, mask;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
    }

    /* Calculate and setup mask_a and mask_b */
    chip_info->mask_a = 0;
    chip_info->mask_b = 0;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (vtss_state->vstax_port_enabled[port_no]) {
            port = VTSS_CHIP_PORT(port_no);
            mask = VTSS_BIT(port);
            if ((port = l28_int_aggr_port(port)) != 0)
                mask |= VTSS_BIT(port);
            if (port_no == vtss_state->vstax_conf.port_0)
                chip_info->mask_a |= mask;
            else
                chip_info->mask_b |= mask;
        }
    }    
    HT_WR(ANALYZER, 0, STACKPORTSA, chip_info->mask_a);
    HT_WR(ANALYZER, 0, STACKPORTSB, chip_info->mask_b);

    /* Setup VStaX for all stackable ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        port = VTSS_CHIP_PORT(port_no);
        if (port < VTSS_CHIP_PORT_AGGR_0)
            continue;

        VTSS_RC(l28_vstax_port_set(port));
        if ((port = l28_int_aggr_port(port)) != 0) {
            /* Setup aggregated port */
            VTSS_RC(l28_vstax_port_set(port));
        }
        
        /* Setup QoS for port (including aggregated port) */
        VTSS_RC(l28_qos_port_conf_set(port_no));
        
        /* Setup VLAN for port (including aggregated port) */
        VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
        
        /* Setup mirror ports (including aggregated port) */
        VTSS_RC(l28_mirror_port_set());
    }
    return VTSS_RC_OK;
}

static vtss_rc l28_vstax_port_conf_set(const BOOL stack_port_a)
{
    return l28_vstax_conf_set();
}

static void l28_debug_print_mask(const vtss_debug_printf_t pr,
                                 u32                       mask)
{
    u32 port;
    
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        pr("%s%s", port == 0 || (port & 7) ? "" : ".", ((1<<port) & mask) ? "1" : "0");
    }
    pr("  0x%08x\n", mask);
}

static void l28_debug_print_header(const vtss_debug_printf_t pr, const char *txt)
{
    pr("%s:\n\n", txt);
}
static void l28_debug_print_port_header(const vtss_debug_printf_t pr, const char *txt)
{
    vtss_debug_print_port_header(pr, txt, VTSS_CHIP_PORTS + 1, 1);
}

static vtss_rc l28_debug_vlan(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    vtss_vid_t        vid;
    vtss_vlan_entry_t *vlan_entry;
    BOOL              header = 1;
    u32               mask = 0;

    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        vlan_entry = &vtss_state->vlan_table[vid];
        if (!vlan_entry->enabled && !info->full)
            continue;
        HT_WR(ANALYZER, 0, VLANTINDX, vid);
        HT_WR(ANALYZER, 0, VLANACES, VLAN_CMD_READ);
        if (l28_vlan_table_idle() == VTSS_RC_OK) {
            HT_RD(ANALYZER, 0, VLANACES, &mask);
            mask = ((mask >> 2) & VTSS_CHIP_PORT_MASK);
        }
        if (header) {
            header = 0;
            l28_debug_print_port_header(pr, "VID   ");
        }
        pr("%-4u  ", vid);
        l28_debug_print_mask(pr, mask);

        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");

    return VTSS_RC_OK;
}

static void l28_acl_bits(const vtss_debug_printf_t pr, 
                         const char *txt, u32*data, int offset, int len, int nl)
{
    int   i, count = 0;
    u32   mask;

    if (txt == NULL)
        pr(".");
    else
        pr(" %s: ", txt);
    for (i = (len - 1); i >= 0; i--) {
        if (count && (count % 8) == 0)
            pr(".");
        count++;

        mask = (1<<(offset+i));
        pr((data[1] & mask) ? "X" : (data[0] & mask) ? "1" : "0"); 
    }
    if (nl)
        pr("\n");
}

static vtss_rc l28_debug_acl(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    int  i;
    u32  valid, misc, redir, cnt, mask, status;
    char buf[32];
    
    for (i = (VTSS_L28_IS2_CNT + VTSS_CHIP_PORTS); i >= 0; i--) {
        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();
        
        /* Read entry */
        VTSS_RC(l28_acl_cmd(ACL_CMD_READ, i));
        HT_RD(ACL, S_ACL, STATUS, &status);
        
        /* Frame info */
        if (i < VTSS_L28_IS2_CNT) {
            int   j;
            u32   type[2], offset;
            u32   misc[2], smac_hi[2], smac_lo[2], dmac_hi[2], dmac_lo[2];
            u32   etype[2], llc[2], snap_hi[2], snap_lo[2], l3_arp[2], l3_sip[8], l3_dip[2];
            u32   l3_misc[2], l4_port[2], l4_misc[2], l3_ip_hi[2], l3_ip_lo[2];

            /* Type value and mask (mask can not be read back - always zero) */
            type[0] = (status & 0x7);
            HT_RDF(ACL, S_ACL, UPDATE_CTRL, 20, 0x7, &type[1]);
            
            /* Read data and mask */
            for (j = 0; j < 2; j++) {
                offset = (j == 0 ? ACE_DATA_OFFS : ACE_MASK_OFFS);
                switch (type[0]) {
                case ACL_TYPE_ETYPE:
                    strcpy(buf, "ETYPE/ANY");
                    HT_RD(ACL, S_ACL, ETYPE_TYPE + offset, &misc[j]);
                    HT_RD(ACL, S_ACL, ETYPE_L2_SMAC_HIGH + offset, &smac_hi[j]);
                    HT_RD(ACL, S_ACL, ETYPE_L2_SMAC_LOW + offset, &smac_lo[j]);
                    HT_RD(ACL, S_ACL, ETYPE_L2_DMAC_HIGH + offset, &dmac_hi[j]);
                    HT_RD(ACL, S_ACL, ETYPE_L2_DMAC_LOW + offset, &dmac_lo[j]);
                    HT_RD(ACL, S_ACL, ETYPE_L2_ETYPE + offset, &etype[j]);
                    break;
                case ACL_TYPE_LLC:
                    strcpy(buf, "LLC");
                    HT_RD(ACL, S_ACL, LLC_TYPE + offset, &misc[j]);
                    HT_RD(ACL, S_ACL, LLC_L2_SMAC_HIGH + offset, &smac_hi[j]);
                    HT_RD(ACL, S_ACL, LLC_L2_SMAC_LOW + offset, &smac_lo[j]);
                    HT_RD(ACL, S_ACL, LLC_L2_DMAC_HIGH + offset, &dmac_hi[j]);
                    HT_RD(ACL, S_ACL, LLC_L2_DMAC_LOW + offset, &dmac_lo[j]);
                    HT_RD(ACL, S_ACL, LLC_L2_LLC + offset, &llc[j]);
                    break;
                case ACL_TYPE_SNAP:
                    strcpy(buf, "SNAP");
                    HT_RD(ACL, S_ACL, SNAP_TYPE + offset, &misc[j]);
                    HT_RD(ACL, S_ACL, SNAP_L2_SMAC_HIGH + offset, &smac_hi[j]);
                    HT_RD(ACL, S_ACL, SNAP_L2_SMAC_LOW + offset, &smac_lo[j]);
                    HT_RD(ACL, S_ACL, SNAP_L2_DMAC_HIGH + offset, &dmac_hi[j]);
                    HT_RD(ACL, S_ACL, SNAP_L2_DMAC_LOW + offset, &dmac_lo[j]);
                    HT_RD(ACL, S_ACL, SNAP_L2_SNAP_HIGH + offset, &snap_hi[j]);
                    HT_RD(ACL, S_ACL, SNAP_L2_SNAP_LOW + offset, &snap_lo[j]);
                    break;
                case ACL_TYPE_ARP:
                    strcpy(buf, "ARP");
                    HT_RD(ACL, S_ACL, ARP_TYPE + offset, &misc[j]);
                    HT_RD(ACL, S_ACL, ARP_L2_SMAC_HIGH + offset, &smac_hi[j]);
                    HT_RD(ACL, S_ACL, ARP_L2_SMAC_LOW + offset, &smac_lo[j]);
                    HT_RD(ACL, S_ACL, ARP_L3_ARP + offset, &l3_arp[j]);
                    HT_RD(ACL, S_ACL, ARP_L3_IPV4_DIP + offset, &l3_dip[j]);
                    HT_RD(ACL, S_ACL, ARP_L3_IPV4_SIP + offset, &l3_sip[j]);
                    break;
                case ACL_TYPE_UDP_TCP:
                    strcpy(buf, "UDP/TCP");
                    HT_RD(ACL, S_ACL, UDP_TCP_TYPE + offset, &misc[j]);
                    HT_RD(ACL, S_ACL, UDP_TCP_L3_MISC_CFG + offset, &l3_misc[j]);
                    HT_RD(ACL, S_ACL, UDP_TCP_L4_PORT + offset, &l4_port[j]);
                    HT_RD(ACL, S_ACL, UDP_TCP_L4_MISC + offset, &l4_misc[j]);
                    HT_RD(ACL, S_ACL, UDP_TCP_L3_IPV4_DIP + offset, &l3_dip[j]);
                    HT_RD(ACL, S_ACL, UDP_TCP_L3_IPV4_SIP + offset, &l3_sip[j]);
                    break;
                case ACL_TYPE_IPV4:
                    strcpy(buf, "IPv4");
                    HT_RD(ACL, S_ACL, IPV4_TYPE + offset, &misc[j]);
                    HT_RD(ACL, S_ACL, IPV4_L3_MISC_CFG + offset, &l3_misc[j]);
                    HT_RD(ACL, S_ACL, IPV4_DATA_0 + offset, &l3_ip_lo[j]);
                    HT_RD(ACL, S_ACL, IPV4_DATA_1 + offset, &l3_ip_hi[j]);
                    HT_RD(ACL, S_ACL, IPV4_L3_IPV4_DIP + offset, &l3_dip[j]);
                    HT_RD(ACL, S_ACL, IPV4_L3_IPV4_SIP + offset, &l3_sip[j]);
                    break;
                case ACL_TYPE_IPV6:
                    strcpy(buf, "IPv6");
                    HT_RD(ACL, S_ACL, IPV6_TYPE + offset, &misc[j]);
                    HT_RD(ACL, S_ACL, IPV6_L3_MISC_CFG + offset, &l3_misc[j]);
                    HT_RD(ACL, S_ACL, IPV6_L3_IPV6_SIP_0 + offset, &l3_sip[0+j]);
                    HT_RD(ACL, S_ACL, IPV6_L3_IPV6_SIP_1 + offset, &l3_sip[2+j]);
                    HT_RD(ACL, S_ACL, IPV6_L3_IPV6_SIP_2 + offset, &l3_sip[4+j]);
                    HT_RD(ACL, S_ACL, IPV6_L3_IPV6_SIP_3 + offset, &l3_sip[6+j]);
                    break;
                default:
                    VTSS_E("illegal type: %u", type[0]);
                    misc[0] = 0; /* Treat as invalid entry */
                    misc[1] = 0;
                    break;
                }
            }
            
            /* Skip invalid entries */
            if ((misc[0] & (1<<0)) == 0 && info->full == 0)
                continue;
            
            /* Type */
            pr(buf);
            l28_acl_bits(pr, "type", type, 0, 3, 1);
            
            /* Common fields */
            l28_acl_bits(pr, "bc", misc, 29, 1, 0);
            l28_acl_bits(pr, "mc", misc, 28, 1, 0);
            l28_acl_bits(pr, "vid", misc, 12, 12, 0);
            l28_acl_bits(pr, "uprio", misc, 24, 3, 0);
            l28_acl_bits(pr, "cfi", misc, 2, 1, 0);
            l28_acl_bits(pr, "pag", misc, 4, ACL_PAG_WIDTH, 0);
            l28_acl_bits(pr, "igr", misc, 1, 1, 0);
            l28_acl_bits(pr, "vld", misc, 0, 1, 1);

            if (type[0] <= ACL_TYPE_ARP) {
                l28_acl_bits(pr, "smac", smac_hi, 0, 16, 0);
                l28_acl_bits(pr, NULL, smac_lo, 0, 32, 1);
            }
            if (type[0] <= ACL_TYPE_SNAP) {
                l28_acl_bits(pr, "dmac", dmac_hi, 0, 16, 0);
                l28_acl_bits(pr, NULL, dmac_lo, 0, 32, 1);
            }
            if (type[0] == ACL_TYPE_ETYPE) {
                l28_acl_bits(pr, "etype", etype, 0, 16, 0);
                l28_acl_bits(pr, "data", etype, 16, 16, 1);
            }
            if (type[0] == ACL_TYPE_LLC) {
                l28_acl_bits(pr, "llc", etype, 0, 32, 1);
            }
            if (type[0] == ACL_TYPE_SNAP) {
                l28_acl_bits(pr, "snap", snap_hi, 0, 8, 0);
                l28_acl_bits(pr, NULL, snap_hi, 0, 32, 1);
            }
            if (type[0] == ACL_TYPE_ARP) {
                l28_acl_bits(pr, "arp", l3_arp, 1, 1, 0);
                l28_acl_bits(pr, "req", l3_arp, 0, 1, 0);
                l28_acl_bits(pr, "unkn", l3_arp, 2, 1, 0);
                l28_acl_bits(pr, "s_match", l3_arp, 3, 1, 0);
                l28_acl_bits(pr, "d_match", l3_arp, 4, 1, 0);
                l28_acl_bits(pr, "len", l3_arp, 5, 1, 0);
                l28_acl_bits(pr, "ip", l3_arp, 6, 1, 0);
                l28_acl_bits(pr, "eth", l3_arp, 7, 1, 1);
            }
            if (type[0] == ACL_TYPE_UDP_TCP || type[0] == ACL_TYPE_IPV4) {
                l28_acl_bits(pr, "proto", l3_misc, 12, 8, 0);
                l28_acl_bits(pr, "ds", l3_misc, 4, 8, 0);
                l28_acl_bits(pr, "options", l3_misc, 2, 1, 0);
                l28_acl_bits(pr, "fragment", l3_misc, 1, 1, 0);
                l28_acl_bits(pr, "ttl", l3_misc, 0, 1, 1);
            }
            if (type[0] >= ACL_TYPE_ARP && type[0] <= ACL_TYPE_IPV4) {
                l28_acl_bits(pr, "sip", l3_sip, 0, 32, 1);
                l28_acl_bits(pr, "dip", l3_dip, 0, 32, 1);
            }
            if (type[0] == ACL_TYPE_UDP_TCP) {
                l28_acl_bits(pr, "sport", l4_port, 16, 16, 0);
                l28_acl_bits(pr, "dport", l4_port, 0, 16, 1);
                l28_acl_bits(pr, "fin", l4_misc, 13, 1, 0);
                l28_acl_bits(pr, "syn", l4_misc, 12, 1, 0);
                l28_acl_bits(pr, "rst", l4_misc, 11, 1, 0);
                l28_acl_bits(pr, "psh", l4_misc, 10, 1, 0);
                l28_acl_bits(pr, "ack", l4_misc, 9, 1, 0);
                l28_acl_bits(pr, "urg", l4_misc, 8, 1, 0);
                l28_acl_bits(pr, "range", l4_misc, 0, 8, 1);
            }
            if (type[0] == ACL_TYPE_IPV4) {
                l28_acl_bits(pr, "data", l3_ip_hi, 0, 16, 0);
                l28_acl_bits(pr, NULL, l3_ip_lo, 0, 32, 1);
            }
            if (type[0] == ACL_TYPE_IPV6) {
                l28_acl_bits(pr, "sip", &l3_sip[6], 0, 32, 1);
                l28_acl_bits(pr, "sip", &l3_sip[4], 0, 32, 1);
                l28_acl_bits(pr, "sip", &l3_sip[2], 0, 32, 1);
                l28_acl_bits(pr, "sip", &l3_sip[0], 0, 32, 1);
            }
        } /* Frame info */
        
        if (i == (VTSS_L28_IS2_CNT + VTSS_CHIP_PORTS))
            pr("%3d (egress)", i);
        else if (i < VTSS_L28_IS2_CNT)
            pr("%3d (ace %d)", i, VTSS_L28_IS2_CNT - i - 1);
        else
            pr("%3d (port %2d)", i, i - VTSS_L28_IS2_CNT);
        pr(" :");
        if (status & (1<<4)) {
            /* Egress action */
            HT_RD(ACL, S_ACL, EG_VLD, &valid);
            HT_RD(ACL, S_ACL, EG_PORT_MASK, &mask);
            HT_RD(ACL, S_ACL, EG_CNT, &cnt);
            pr("%s 0x%08x %d\n", valid ? "V" : "v", mask, cnt);
        } else {
            /* Ingress action */
            HT_RD(ACL, S_ACL, IN_VLD, &valid);
            HT_RD(ACL, S_ACL, IN_MISC_CFG, &misc);
            HT_RD(ACL, S_ACL, IN_REDIR_CFG, &redir);
            HT_RD(ACL, S_ACL, IN_CNT, &cnt);
            pr("%s %s %s (%d) %s (%d) %s %s, %s (%d) %d\n", 
               valid ? "V" : "v", 
               misc & (1<<20) ? "H" : "h",
               misc & (1<<12) ? "C" : "c",
               (misc>>16) & 0x3,
               misc & (1<<3) ? "P" : "p",
               (misc>>4) & 0xf,
               misc & (1<<1) ? "L" : "l",
               misc & (1<<0) ? "F" : "f",
               redir & (1<<0) ? "R" : "r",
               (redir>>4) & 0x3f,
               cnt);
        }
    }
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc l28_debug_aggr(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    u32 ac, port, pgid, mask;
    
    l28_debug_print_header(pr, "Source Masks");
    l28_debug_print_port_header(pr, "Port  ");
    for (port = 0; port <= VTSS_CHIP_PORTS;  port++) {
        HT_RD(ANALYZER, 0, SRCMASKS + port, &mask);
        pr("%-4u  ", port);
        l28_debug_print_mask(pr, mask);
    }
    pr("\n");
    
    l28_debug_print_header(pr, "Aggregation Masks");
    l28_debug_print_port_header(pr, "AC    ");
    for (ac = 0; ac < L28_ACS; ac++) {
        HT_RD(ANALYZER, 0, AGGRMSKS + ac, &mask);
        pr("%-4u  ", ac);
        l28_debug_print_mask(pr, mask);
    }
    pr("\n");
    
    l28_debug_print_header(pr, "Destination Masks");
    l28_debug_print_port_header(pr, "PGID  ");
    for (pgid = 0; pgid < VTSS_PGID_LUTON28; pgid++) {
        HT_RD(ANALYZER, 0, DSTMASKS + pgid, &mask);
        pr("%-4u  ", pgid);
        l28_debug_print_mask(pr, mask);
    }
    pr("\n");
    return VTSS_RC_OK;
}

static vtss_rc l28_debug_stp(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    u32 mask;
    
    HT_RD(ANALYZER, 0, LERNMASK, &mask);
    l28_debug_print_header(pr, "Learn Mask");
    l28_debug_print_port_header(pr, NULL);
    l28_debug_print_mask(pr, mask);
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc l28_debug_info_print(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_VLAN, l28_debug_vlan, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_ACL, l28_debug_acl, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_AGGR, l28_debug_aggr, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_STP, l28_debug_stp, pr, info));
#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_FDMA)) {
        if (vtss_state->fdma_state.fdma_func.fdma_debug_print != NULL) {
            return vtss_state->fdma_state.fdma_func.fdma_debug_print(vtss_state, pr, info);
        } else {
            return VTSS_RC_ERROR;
        }
    }
#endif    
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Initialization 
 * ================================================================= */

#if defined(VTSS_FEATURE_WARM_START)

static vtss_rc l28_restart_conf_set(void)
{
    u32 value, mask;

    value = vtss_cmn_restart_value_get();
    mask = ((VTSS_BITMASK(VTSS_RESTART_VERSION_WIDTH) << VTSS_RESTART_VERSION_OFFSET) | 
            (VTSS_BITMASK(VTSS_RESTART_TYPE_WIDTH)    << VTSS_RESTART_TYPE_OFFSET));
    
    HT_WR(SYSTEM, 0, ICPU_MBOX_CLR, mask);
    HT_WR(SYSTEM, 0, ICPU_MBOX_SET, value);
   
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_WARM_START */

static vtss_rc ht_setup_cpu_if(const vtss_init_conf_t * const conf)
{
    const vtss_pi_conf_t *pi;
    u8                   b;
    u32                  value, wait;
    
    VTSS_D("enter");
    
    /* Setup PI width */
    pi = &conf->pi;
    switch (pi->width) {
    case VTSS_PI_WIDTH_8:  
        b = (1<<3 | 1<<2 | 1<<0); 
        break;
    case VTSS_PI_WIDTH_16: 
        b = (1<<3 | 1<<2 | 0<<0); 
        break;
    default:
        VTSS_E("unknown pi->width");
        return VTSS_RC_ERROR;
    }
    value = ((b<<24) | (b<<16) | (b<<8) | b);
    HT_WR(SYSTEM, 0, PICONF, value);
    
    value = 0x01020304; /* Endianness test value used in next line */
    value = (*((u8 *)&value) == 0x01 ? 0x99999999 : 0x81818181);
    HT_WR(SYSTEM, 0, CPUMODE, value);

    HT_WR(SYSTEM, 0, SIPAD, 0);
    
    /* Wait unit is 9.6 nsec, maximum 144 nsec */
    wait = (pi->cs_wait_ns > 144 ? 144 : (pi->cs_wait_ns*10/96));
    HT_WR(SYSTEM, 0, CPUCTRL, wait<<8 | (pi->use_extended_bus_cycle ? 0 : 1)<<5);
    
    return VTSS_RC_OK;
}

static vtss_rc l28_init_conf_set(void)
{
    vtss_init_conf_t *conf = &vtss_state->init_conf;
    u32              port, value, ac, memid;
    vtss_vid_t       vid;

    VTSS_D("enter");

    VTSS_RC(ht_setup_cpu_if(conf));

#if defined(VTSS_FEATURE_WARM_START)
    /* Check for warm restart */
    HT_RD(SYSTEM, 0, ICPU_MBOX_VAL, &value);
    VTSS_RC(vtss_cmn_restart_update(value));
    if (vtss_state->warm_start_cur)
        return VTSS_RC_OK;
#endif /* VTSS_FEATURE_WARM_START */
    VTSS_I("cold/cool starting");

#if VTSS_OPT_VCORE_II
    /* Cannot reset reliably */
#else
    /* Write to RESET register */
    HT_WR(SYSTEM, 0, GLORESET, 1<<0);
    VTSS_NSLEEP(VTSS_T_RESET);
    VTSS_RC(ht_setup_cpu_if(conf));
#endif /* VTSS_OPT_VCORE_II */

    HT_WR(SYSTEM, 0, TIMECMP, HT_TIMECMP_DEFAULT);
#if VTSS_OPT_UART
    /* Set shared UART/GPIO pins to UART mode */
    HT_WRM(SYSTEM, 0, GPIOCTRL, (1<<3)|(1<<2)|(0<<1)|(0<<0), (1<<3)|(1<<2)|(1<<1)|(1<<0));
#endif /* VTSS_OPT_UART */

#if VTSS_OPT_REF_CLK_SINGLE_ENDED
    /* Improve clock for single ended ref clock mode */
    HT_WRM(SYSTEM, 0, MACRO_CTRL, (1 << 3), (1 << 3));
#endif /* VTSS_OPT_REF_CLK_SINGLE_ENDED */

    /* Clear memories */
    for (memid = 0; memid <= MEMINIT_MAXID; memid++) {
        /* Skip certain blocks */
        if (memid >= MEMID_SKIP_MIN && memid <= MEMID_SKIP_MAX) 
            continue; 
        HT_WR(MEMINIT, S_MEMINIT, MEMINIT,(MEMINIT_CMD_INIT<<8) | memid);
        VTSS_MSLEEP(1);
    }
    VTSS_MSLEEP(30);

    /* Clear MAC and VLAN table */
    HT_WR(ANALYZER, 0, MACACCES, MAC_CMD_TABLE_CLEAR<<0);
    HT_WR(ANALYZER, 0, VLANACES, VLAN_CMD_TABLE_CLEAR<<0);

    /* Clear ACL table and enable ACLs */
    HT_WR(ACL, S_ACL, ACL_CFG, 1<<0);
    HT_WR(ACL, S_ACL, ETYPE_TYPE + ACE_DATA_OFFS, 0<<0);
    HT_WR(ACL, S_ACL, ETYPE_TYPE + ACE_MASK_OFFS, 0<<0);
    VTSS_RC(l28_acl_cmd(ACL_CMD_INIT, 0));
    HT_WR(ACL, S_ACL, EG_VLD, 1<<0);
    HT_WR(ACL, S_ACL, EG_PORT_MASK, VTSS_CHIP_PORT_MASK + (1<<VTSS_CHIP_PORT_CPU));
    VTSS_RC(l28_acl_cmd(ACL_CMD_WRITE, VTSS_L28_IS2_CNT + VTSS_CHIP_PORTS));
    VTSS_MSLEEP(40);

    /* Clear VLAN table masks */
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        if (vid == VTSS_VID_DEFAULT) {
            /* Default VLAN includes all ports */
            vtss_state->vlan_table[vid].mask = VTSS_CHIP_MASK_UPDATE; /* Ensure update next time */
            continue;
        }
        HT_WR(ANALYZER, 0, VLANTINDX, vid<<0);
        HT_WR(ANALYZER, 0, VLANACES, VLAN_CMD_WRITE<<0);
        VTSS_RC(l28_vlan_table_idle());
    }
        
    /* Setup chip ports */
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        /* Default VLAN port configuration */
        VTSS_RC(ht_vlan_port_conf_set(port, 0, 
                                      &vtss_state->vlan_port_conf[VTSS_PORT_NO_START]));
        
        /* Clear counters */
        HT_WR(PORT, port, C_CLEAR, 0);
                
        /* Enable ACLs and physical port in IFH for LACP frames */
        HT_WR(PORT, port, MISCCFG, (1<<3) | (1<<2));

        /* Setup one-to-one PAG mapping taking internal aggregations into account */
        value = port;
#if VTSS_OPT_INT_AGGR
        if (l28_int_aggr_port(port - 1))
            value--;
#endif /* VTSS_OPT_INT_AGGR */
        HT_WR(ACL, S_ACL, PAG_CFG + port, value);
    } /* Port loop */

    /* Always enable VStaX in Analyzer */
    HT_WR(ANALYZER, 0, STACKCFG, 
          (1<<17) | (1<<16) | (1<<15) | (1<<7) | (1<<6) | (1<<5) | (1<<4) | (1<<3));

    /* Rx queue system */
    VTSS_RC(l28_rx_conf_set());
    HT_WR(PORT, VTSS_CHIP_PORT_CPU, MISCCFG, 1<<1); /* Enable dropping */
    
    /* Enable all ports in IP MC flood mask */
    value = VTSS_CHIP_PORT_MASK;
    HT_WR(ANALYZER, 0, IFLODMSK, value);
    
    value |= (1<<VTSS_CHIP_PORT_CPU);
    
    /* Enable all ports in RECVMASK */
    HT_WR(ANALYZER, 0, RECVMASK, value);
    
    /* Clear aggregation masks */
    for (ac = 0; ac < L28_ACS; ac++) {
        HT_WR(ANALYZER, 0, AGGRMSKS + ac, 0);
    }

    /* Disable learning for frames discarded by VLAN ingress filtering */
    HT_WRF(ANALYZER, 0, ADVLEARN, 29, 0x1, 0x1);
    
    /* Disable learning */
    HT_WR(ANALYZER, 0, LERNMASK, 0);
    
    /* Set MAC age time to default value */
    VTSS_RC(l28_mac_table_age_time_set());

    return VTSS_RC_OK;
}

vtss_rc vtss_luton28_inst_create(void)
{
    vtss_cil_func_t *func = &vtss_state->cil_func;
    vtss_vcap_obj_t *is2;
    
    /* Initialization */
    func->init_conf_set = l28_init_conf_set;
    func->miim_read = l28_miim_read;
    func->miim_write = l28_miim_write;
#if defined(VTSS_FEATURE_WARM_START)
    func->restart_conf_set = l28_restart_conf_set;
#endif /* VTSS_FEATURE_WARM_START */

    /* Miscellaneous */
    func->reg_read = l28_reg_read;
    func->reg_write = l28_reg_write;
    func->chip_id_get = l28_chip_id_get;
    func->poll_1sec = l28_poll_1sec;
    func->gpio_mode = l28_gpio_mode;
    func->gpio_read = l28_gpio_read;
    func->gpio_write = l28_gpio_write;
    func->serial_led_set = l28_serial_led_set;
    func->debug_info_print = l28_debug_info_print;

    /* Port control */
    func->port_map_set = l28_port_map_set;
    func->port_conf_set = l28_port_conf_set;
    func->port_conf_get = l28_port_conf_get;
    func->port_clause_37_status_get = l28_port_clause_37_status_get;
    func->port_clause_37_control_get = l28_port_clause_37_control_get;
    func->port_clause_37_control_set = l28_port_clause_37_control_set;
    func->port_status_get = l28_port_status_get;
    func->port_counters_update = l28_port_counters_update;
    func->port_counters_clear = l28_port_counters_clear;
    func->port_counters_get = l28_port_counters_get;
    func->port_basic_counters_get = l28_port_basic_counters_get;
    
    /* QoS */
    func->qos_conf_set = l28_qos_conf_set;
    func->qos_port_conf_set = l28_qos_port_conf_set;
    func->qce_add = l28_qce_add;
    func->qce_del = l28_qce_del;
    
    /* Layer 2 */
    func->mac_table_add  = l28_mac_table_add;
    func->mac_table_del  = l28_mac_table_del;
    func->mac_table_get = l28_mac_table_get;
    func->mac_table_get_next = l28_mac_table_get_next;
    func->mac_table_age_time_set = l28_mac_table_age_time_set;
    func->mac_table_age = l28_mac_table_age;
    func->mac_table_status_get = l28_mac_table_status_get;
    func->learn_port_mode_set = l28_learn_port_mode_set;
    func->learn_state_set = l28_learn_state_set;
    func->mstp_vlan_msti_set = vtss_cmn_vlan_members_set;
    func->mstp_state_set = vtss_cmn_mstp_state_set;
    func->erps_vlan_member_set = vtss_cmn_erps_vlan_member_set;
    func->erps_port_state_set = vtss_cmn_erps_port_state_set;
    func->vlan_port_conf_set = vtss_cmn_vlan_port_conf_set;
    func->vlan_port_conf_update = l28_vlan_port_conf_update;
    func->vlan_port_members_set = vtss_cmn_vlan_members_set;
    func->vlan_mask_update = l28_vlan_mask_update;
    func->isolated_vlan_set = l28_isolated_vlan_set;
    func->isolated_port_members_set = l28_isolated_port_members_set;
    func->aggr_mode_set = l28_aggr_mode_set;
    func->mirror_port_set = l28_mirror_port_set;
    func->mirror_ingress_set = l28_mirror_ingress_set;
    func->mirror_egress_set = l28_mirror_egress_set;
    func->eps_port_set = vtss_cmn_eps_port_set;
    func->flood_conf_set = l28_flood_conf_set;
    func->port_forward_set = l28_port_forward_set;
    func->vstax_conf_set = l28_vstax_conf_set;
    func->vstax_port_conf_set = l28_vstax_port_conf_set;
    func->src_table_write = l28_src_table_write;
    func->pgid_table_write = l28_pgid_table_write;
    func->aggr_table_write = l28_aggr_table_write;
    func->pmap_table_write = l28_pmap_table_write;

    /* Packet control */
    func->rx_conf_set        = l28_rx_conf_set;
    func->rx_frame_get       = l28_rx_frame_get;
    func->rx_frame_discard   = l28_rx_frame_discard;
    func->tx_frame_port      = l28_tx_frame_port;
    func->rx_hdr_decode      = l28_rx_hdr_decode;
    func->tx_hdr_encode      = l28_tx_hdr_encode;
    func->vstax_header2frame = l28_vstax_header2frame;
    func->vstax_frame2header = l28_vstax_frame2header;

    /* ACL */
    func->acl_policer_set = l28_acl_policer_set;
    func->acl_port_set = l28_acl_port_set;
    func->acl_port_counter_get = l28_acl_port_counter_get;
    func->acl_port_counter_clear = l28_acl_port_counter_clear;
    func->acl_ace_add = l28_ace_add;
    func->acl_ace_del = vtss_cmn_ace_del;
    func->acl_ace_counter_get = vtss_cmn_ace_counter_get;
    func->acl_ace_counter_clear = vtss_cmn_ace_counter_clear;
    func->vcap_range_commit = l28_vcap_range_commit;  	 
 
    /* IS2 */
    is2 = &vtss_state->is2.obj;
    is2->max_count = VTSS_L28_IS2_CNT;
    is2->entry_get = l28_is2_entry_get;
    is2->entry_add = l28_is2_entry_add;
    is2->entry_del = l28_is2_entry_del;
    is2->entry_move = l28_is2_entry_move;

    /* State data */
    vtss_state->gpio_count = 16;
    vtss_state->prio_count = L28_PRIOS;
    vtss_state->rx_queue_count = 4;
    vtss_state->rx_conf.queue[0].size = 32*1024;
    vtss_state->rx_conf.queue[1].size = 16*1024;
    vtss_state->ac_count = L28_ACS;

#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
    l28_fdma_func_init();
#endif

    return VTSS_RC_OK;
}

#endif /* VTSS_ARCH_LUTON28 */
