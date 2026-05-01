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

/* Use relative DWORD addresses for registers - must be first */
#define VTSS_IOADDR(t,o) ((((t) - VTSS_TO_DEVCPU_ORG) >> 2) + (o))
#define VTSS_IOREG(t,o)  (VTSS_IOADDR(t,o))

#define VTSS_TRACE_LAYER VTSS_TRACE_LAYER_CIL

// Avoid "vtss_api.h not used in module vtss_luton26.c"
/*lint --e{766} */

#include "vtss_api.h"
#include "vtss_state.h"

#if defined(VTSS_ARCH_LUTON26)
#include "vtss_common.h"
#include "vtss_init_api.h"
#include "vtss_luton26.h"
#include "vtss_luton26_reg.h"
#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
#include "vtss_vcoreiii_fdma.h"
#endif
#include "vtss_util.h"

/* Chip ports - See PD0071 Sect 4.1 */
#define VTSS_CHIP_PORTS           26
/* On the switch core side, the CPU port is one single port */
#define VTSS_CHIP_PORT_CPU        VTSS_CHIP_PORTS
/* On the CPU Port module side, the switch CPU port is split into two "CPU ports" */
#define VTSS_CHIP_PORT_CPU_0      (VTSS_CHIP_PORT_CPU + 0) /* A.k.a. CPU Port 26 */
#define VTSS_CHIP_PORT_CPU_1      (VTSS_CHIP_PORT_CPU + 1) /* A.k.a. CPU Port 27 */
#define VTSS_CHIP_PORT_MASK       ((1U<<VTSS_CHIP_PORTS)-1)

#define CPU_INJ_REG     0 /* CPU Injection GRP - register based */
#define CPU_INJ_DMA     1 /* CPU Injection GRP - DMA based */

#define L26_ACS   16  /* Number of aggregation masks */
#define L26_PRIOS 8   /* Number of priorities */
#define L26_PRIO_END (VTSS_PRIO_START + L28_PRIOS) /* Luton 26 priority end number */

/* Buffer constants */
#define L26_BUFFER_MEMORY 512000
#define L26_BUFFER_REFERENCE 5500
#define L26_BUFFER_CELL_SZ 48

#define L26_GPIOS        32
#define L26_SGPIO_GROUPS 1

/* Reserved PGIDs */
#define PGID_UC      (VTSS_PGID_LUTON26-4)
#define PGID_MC      (VTSS_PGID_LUTON26-3)
#define PGID_MCIPV4  (VTSS_PGID_LUTON26-2)
#define PGID_MCIPV6  (VTSS_PGID_LUTON26-1)
#define PGID_AGGR    (VTSS_PGID_LUTON26)
#define PGID_SRC     (PGID_AGGR + L26_ACS)

/* Policers, last policers used for ACLs */
#define L26_ACL_POLICER_DISC   26 /* Reserved for ACL discarding */
#define L26_ACL_POLICER_OFFSET (VTSS_L26_POLICER_CNT - VTSS_ACL_POLICERS)

/* sFlow H/W-related min/max */
#define LU26_SFLOW_MIN_SAMPLE_RATE    0 /**< Minimum allowable sampling rate for sFlow */
#define LU26_SFLOW_MAX_SAMPLE_RATE 4095 /**< Maximum allowable sampling rate for sFlow */

/* TCAM entries */
enum vtss_tcam_sel {
    VTSS_TCAM_SEL_ENTRY   = VTSS_BIT(0),
    VTSS_TCAM_SEL_ACTION  = VTSS_BIT(1),
    VTSS_TCAM_SEL_COUNTER = VTSS_BIT(2),
    VTSS_TCAM_SEL_ALL     = VTSS_BITMASK(3),
}; 

enum vtss_tcam_cmd {
    VTSS_TCAM_CMD_WRITE       = 0, /* Copy from Cache to TCAM */
    VTSS_TCAM_CMD_READ        = 1, /* Copy from TCAM to Cache */
    VTSS_TCAM_CMD_MOVE_UP     = 2, /* Move <count> up */
    VTSS_TCAM_CMD_MOVE_DOWN   = 3, /* Move <count> down */
    VTSS_TCAM_CMD_INITIALIZE  = 4, /* Write all (from cache) */
};

enum vtss_tcam_bank {
    VTSS_TCAM_S1 = 0,           /* S1 */
    VTSS_TCAM_S2,               /* S2 */
    VTSS_TCAM_ES0               /* ES0 */
};

typedef struct {
    u32 target;                 /* Target offset */
    u16 entries;                /* # of entries */
    u16 actions;                /* # of actions */
    u8  entry_width;            /* Width of entry (in dwords) */
    u8  action_width;           /* Width of entry (in dwords) */
} tcam_props_t;

#define BITS_TO_DWORD(x) (1+((x-1)/32))

static const tcam_props_t tcam_info[] = {
    [VTSS_TCAM_S1] = {
        .target = VTSS_TO_S1,
        .entries = VTSS_L26_IS1_CNT,
        .actions = VTSS_L26_IS1_CNT + 1,
        .entry_width = BITS_TO_DWORD(184), /* 188 - 2 from TG_DAT */
        .action_width = BITS_TO_DWORD(60),
    },
    [VTSS_TCAM_S2] = {
        .target = VTSS_TO_S2,
        .entries = VTSS_L26_IS2_CNT,
        .actions =  VTSS_L26_IS2_CNT + 28,
        .entry_width = BITS_TO_DWORD(195), /* 196 - 1 from TG_DAT */
        .action_width = BITS_TO_DWORD(46),
    },
    [VTSS_TCAM_ES0] = {
        .target = VTSS_TO_ES0,
        .entries = VTSS_L26_ES0_CNT,
        .actions = VTSS_L26_ES0_CNT + 26,
        .entry_width = BITS_TO_DWORD(28), /* 29 - 1 from TG_DAT */
        .action_width = BITS_TO_DWORD(37),
    },
};

#define VTSS_TCAM_ENTRY_WIDTH 8 /* Max entry width (32bit words) */

#define L26_ACL_FIELD(off, wid, val, msk, ent_bits, msk_bits)          \
    {                                                                  \
        vtss_bs_set(ent_bits, off, wid, val);                          \
        vtss_bs_set(msk_bits, off, wid, msk);                          \
    }

#define L26_ACL_CFIELD(off, wid, val, msk, ent_bits, msk_bits)         \
    {                                                                  \
        if(msk)                                                        \
            L26_ACL_FIELD(off, wid, val, msk, ent_bits, msk_bits);     \
    }

#define L26_ACL_BITSET(off, val, ent_bits, msk_bits)                   \
    {                                                                  \
        vtss_bs_set(ent_bits, off, 1, val);                            \
        vtss_bs_set(msk_bits, off, 1, 1);                              \
    }

#define L26_ACL_CBITSET(off, bval, ent_bits, msk_bits)                  \
    {                                                                   \
        if(bval != VTSS_ACE_BIT_ANY)                                    \
            L26_ACL_BITSET(off, bval == VTSS_ACE_BIT_1, ent_bits, msk_bits); \
    }

#define L26_ACL_CBITSET_INV(off, bval, ent_bits, msk_bits)                  \
    {                                                                   \
        if(bval != VTSS_ACE_BIT_ANY)                                    \
            L26_ACL_BITSET(off, bval == VTSS_ACE_BIT_0, ent_bits, msk_bits); \
    }

static u32
l26_get_bytes(u8 *data, int nbytes)
{
    u32 val = 0;
    int i;
    for(i = 0; i < nbytes; i++)
        val = (data[i] + (val << 8));
    return val;
}

static void
l26_acl_bytes(int off, int bytes, 
              u8 *val, u8 *msk,
              void *ent_val, void *ent_msk)
{
    while (bytes) {
        u8 nbytes = MIN(4, bytes); /* At most 4 bytes a time */
        u32 wd = l26_get_bytes(msk, nbytes);
        if (wd) {                    /* Something set in mask? */
            vtss_bs_set(ent_msk, off, nbytes*8, wd);
            /* Add value part to look for */
            wd = l26_get_bytes(val, nbytes);
            vtss_bs_set(ent_val, off, nbytes*8, wd);
        }
        bytes -= nbytes;
    }
}

#define L26_ACE_MAC(off, mac, ent_bits, msk_bits)                       \
    /* MAC_HIGH */                                                        \
    l26_acl_bytes(off, 2, mac.value, mac.mask, ent_bits, msk_bits); \
    /* MAC_LOW */                                                        \
    l26_acl_bytes(off + 16, 4, mac.value + 2, mac.mask + 2, ent_bits, msk_bits);

/* ================================================================= *
 *  Function declarations
 * ================================================================= */
static vtss_rc l26_vcap_range_commit(void);
static vtss_rc l26_acl_policer_move(u32 policer);
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
static vtss_rc l26_evc_policer_move(u32 policer);
static vtss_rc l26_evc_policer_conf_set(const vtss_evc_policer_id_t policer_id);
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
static vtss_rc l26_port_policer_fc_set(const vtss_port_no_t port_no, u32 chipport);

/* ================================================================= *
 *  Register acces
 * ================================================================= */

#define L26_RD(p, value)                              \
    {                                                 \
        vtss_rc __rc = l26_rd(p, value);              \
        if (__rc != VTSS_RC_OK)                       \
            return __rc;                              \
    }

#define L26_WR(p, value)                              \
    {                                                 \
        vtss_rc __rc = l26_wr(p, value);              \
        if (__rc != VTSS_RC_OK)                       \
            return __rc;                              \
    }

// Read modify write.
//
// p     : Register to modify
// value : New value
// mask  : Bits to be affected.
#define L26_WRM(p, value, mask)                       \
    {                                                 \
        vtss_rc __rc = l26_wrm(p, value, mask);       \
        if (__rc != VTSS_RC_OK)                       \
            return __rc;                              \
    }

#define L26_WRM_SET(p, mask) L26_WRM(p, mask, mask)
#define L26_WRM_CLR(p, mask) L26_WRM(p, 0,    mask)
#define L26_WRM_CTL(p, _cond_, mask) L26_WRM(p, (_cond_ ? mask : 0), mask)

/* Decode register bit field */
#define L26_BF(name, value) ((VTSS_F_##name & value) ? 1 : 0)

static vtss_rc l26_wr(u32 addr, u32 value);
static vtss_rc l26_rd(u32 addr, u32 *value);

#if !VTSS_OPT_VCORE_III
static inline BOOL l26_reg_directly_accessible(u32 addr)
{
    /* Running on external CPU. VCoreIII registers require indirect access. */
    /* On internal CPU, all registers are always directly accessible. */
    return (addr < ((VTSS_IO_ORIGIN2_OFFSET - VTSS_IO_ORIGIN1_OFFSET) >> 2));
}
#endif /* !VTSS_OPT_VCORE_III */

#if !VTSS_OPT_VCORE_III
/* Read or write register indirectly */
static vtss_rc l26_reg_indirect_access(u32 addr, u32 *value, BOOL is_write)
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

    if ((result = l26_wr(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_ADDR, addr)) != VTSS_RC_OK) {
        goto do_exit;
    }
    if (is_write) {
        if ((result = l26_wr(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, *value)) != VTSS_RC_OK) {
            goto do_exit;
        }
        // Wait for operation to complete
        do {
            if ((result = l26_rd(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL, &ctrl)) != VTSS_RC_OK) {
                goto do_exit;
            }
        } while (ctrl & VTSS_F_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL_VA_BUSY);
    } else {
        // Dummy read to initiate access
        if ((result = l26_rd(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, value)) != VTSS_RC_OK) {
            goto do_exit;
        }
        // Wait for operation to complete
        do {
            if ((result = l26_rd(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL, &ctrl)) != VTSS_RC_OK) {
                goto do_exit;
            }
        } while (ctrl & VTSS_F_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL_VA_BUSY);
        if ((result = l26_rd(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, value)) != VTSS_RC_OK) {
            goto do_exit;
        }
    }

do_exit:
    VTSS_OS_SCHEDULER_UNLOCK(flags);
    return result;
}
#endif /* !VTSS_OPT_VCORE_III */

/* Read target register using current CPU interface */
static vtss_rc l26_rd(u32 reg, u32 *value)
{
#if VTSS_OPT_VCORE_III
    return vtss_state->init_conf.reg_read(0, reg, value);
#else
    if (l26_reg_directly_accessible(reg)) {
        return vtss_state->init_conf.reg_read(0, reg, value);
    } else {
        return l26_reg_indirect_access(reg, value, FALSE);
    }
#endif
}

/* Write target register using current CPU interface */
static vtss_rc l26_wr(u32 reg, u32 value)
{
#if VTSS_OPT_VCORE_III
    return vtss_state->init_conf.reg_write(0, reg, value);
#else
    if (l26_reg_directly_accessible(reg)) {
        return vtss_state->init_conf.reg_write(0, reg, value);
    } else {
        return l26_reg_indirect_access(reg, &value, TRUE);
    }
#endif
}

/* Read-modify-write target register using current CPU interface */
static vtss_rc l26_wrm(u32 reg, u32 value, u32 mask)
{
    vtss_rc rc;
    u32     val;

    if ((rc = l26_rd(reg, &val)) == VTSS_RC_OK) {
        val = ((val & ~mask) | (value & mask));
        rc = l26_wr(reg, val);
    }
    return rc;
}

static vtss_rc l26_reg_read(const vtss_chip_no_t chip_no, const u32 addr, u32 * const value)
{
    return l26_rd(addr, value);
}

static vtss_rc l26_reg_write(const vtss_chip_no_t chip_no, const u32 addr, const u32 value)
{
    return l26_wr(addr, value);
}

/* ================================================================= *
 *  Utility functions
 * ================================================================= */

static u32 l26_port_mask(const BOOL member[])
{
    vtss_port_no_t port_no;
    u32            port, mask = 0;
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (member[port_no]) {
            port = VTSS_CHIP_PORT(port_no);
            mask |= VTSS_BIT(port);
        }
    }
    return mask;
}

/* Convert to PGID in chip */
static u32 l26_chip_pgid(u32 pgid)
{
    if (pgid < vtss_state->port_count) {
        return VTSS_CHIP_PORT(pgid);
    } else {
        return (pgid + VTSS_CHIP_PORTS - vtss_state->port_count);
    }
}

/* Convert from PGID in chip */
static u32 l26_vtss_pgid(u32 pgid)
{
    vtss_port_no_t port_no;
    
    if (pgid < VTSS_CHIP_PORTS) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            if (VTSS_CHIP_PORT(port_no) == pgid)
                break;
        return port_no;
    } else {
        return (vtss_state->port_count + pgid - VTSS_CHIP_PORTS);
    }
}

/* ================================================================= *
 *  TCAM functions
 * ================================================================= */

static vtss_rc l26_vcap_command(const tcam_props_t *tcam,
                                u16 ix, 
                                int cmd, 
                                int sel)
{
    u32 value = 
        VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_CMD(cmd) |
        VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_ADDR(ix) |
        VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_SHOT;

    if((sel & VTSS_TCAM_SEL_ENTRY) && ix >= tcam->entries)
        return VTSS_RC_ERROR;

    if(!(sel & VTSS_TCAM_SEL_ENTRY))
        value |= VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_ENTRY_DIS;

    if(!(sel & VTSS_TCAM_SEL_ACTION))
        value |= VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_ACTION_DIS;

    if(!(sel & VTSS_TCAM_SEL_COUNTER))
        value |= VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_CNT_DIS;

    L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL(tcam->target), value);           

    do {
        L26_RD(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL(tcam->target), &value);
    } while(value & VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_SHOT);

    return VTSS_RC_OK;
}

static vtss_rc l26_vcap_entry2cache(const tcam_props_t *tcam,
                                    u32 typegroup,
                                    u32 *entry,
                                    u32 *mask)
{
    int i;
    L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_TG_DAT(tcam->target), typegroup);
    for(i = 0; i < tcam->entry_width; i++) {
        L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ENTRY_DAT(tcam->target, i), entry ? entry[i] : 0);
        L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_MASK_DAT(tcam->target, i), mask ? ~mask[i] : ~0);
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_vcap_cache2entry(const tcam_props_t *tcam,
                                    u32 *entry,
                                    u32 *mask)
{
    int i;
    for(i = 0; i < tcam->entry_width; i++) {
        u32 m;
        L26_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ENTRY_DAT(tcam->target, i), &entry[i]);
        L26_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_MASK_DAT(tcam->target, i), &m);
        mask[i] = ~m;           /* Invert mask */
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_vcap_action2cache(const tcam_props_t *tcam,
                                     const u32 *entry,
                                     u32 counter)
{
    int i;
    for(i = 0; i < tcam->action_width; i++)
        L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ACTION_DAT(tcam->target, i), entry ? entry[i] : 0);
    L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tcam->target, 0), counter);
    return VTSS_RC_OK;
}

static vtss_rc l26_vcap_cache2action(const tcam_props_t *tcam,
                                     u32 *entry)
{
    int i;
    for(i = 0; i < tcam->action_width; i++)
        L26_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ACTION_DAT(tcam->target, i), &entry[i]);
    return VTSS_RC_OK;
}

static vtss_rc l26_vcap_initialize(const tcam_props_t *tcam)
{
    /* First write entries */
    VTSS_RC(l26_vcap_entry2cache(tcam, 0, NULL, NULL));
    L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG(tcam->target),
           VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_SIZE(tcam->entries));
    VTSS_RC(l26_vcap_command(tcam, 0, VTSS_TCAM_CMD_INITIALIZE, VTSS_TCAM_SEL_ENTRY));

    /* Then actions and counters */
    VTSS_RC(l26_vcap_action2cache(tcam, NULL, 0));
    L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG(tcam->target),
           VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_SIZE(tcam->actions));
    VTSS_RC(l26_vcap_command(tcam, 0, VTSS_TCAM_CMD_INITIALIZE, VTSS_TCAM_SEL_ACTION|VTSS_TCAM_SEL_COUNTER));

    return VTSS_RC_OK;
}

/* Convert from 0-based index to VCAP entry index and run command */
static vtss_rc l26_vcap_index_command(const tcam_props_t *tcam,
                                      u16 ix, 
                                      int cmd, 
                                      int sel)
{
    return l26_vcap_command(tcam, tcam->entries - ix - 1, cmd, sel);
}

/* Convert from 0-based port to VCAP entry index and run command */
static vtss_rc l26_vcap_port_command(const tcam_props_t *tcam,
                                     u16 port, 
                                     int cmd, 
                                     int sel)
{
    return l26_vcap_command(tcam, tcam->entries + port, cmd, sel);
}

static vtss_rc l26_vcap_entry_get(int bank, u32 ix, u32 *counter, BOOL clear) 
{
    const tcam_props_t *tcam = &tcam_info[bank];
    
    /* Read counter at index */
    VTSS_RC(l26_vcap_index_command(tcam, ix, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_COUNTER));
    L26_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tcam->target, 0), counter);
    if (!clear)
        return VTSS_RC_OK;

    /* Clear counter at index */
    L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tcam->target, 0), 0);
    return l26_vcap_index_command(tcam, ix, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_COUNTER);
}

static vtss_rc l26_vcap_port_get(int bank, u32 port, u32 *counter, BOOL clear) 
{
    const tcam_props_t *tcam = &tcam_info[bank];
    
    /* Read counter at index */
    VTSS_RC(l26_vcap_port_command(tcam, port, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_COUNTER));
    L26_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tcam->target, 0), counter);
    if (!clear)
        return VTSS_RC_OK;

    /* Clear counter at index */
    L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tcam->target, 0), 0);
    return l26_vcap_port_command(tcam, port, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_COUNTER);
}

static vtss_rc l26_vcap_entry_del(int bank, u32 ix)
{
    const tcam_props_t *tcam = &tcam_info[bank];

    /* Delete entry at index */
    VTSS_RC(l26_vcap_entry2cache(tcam, 0, NULL, NULL));
    VTSS_RC(l26_vcap_action2cache(tcam, NULL, 0));
    return l26_vcap_index_command(tcam, ix, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ALL);
}

static vtss_rc l26_vcap_entry_move(int bank, u32 ix, u32 count, BOOL up)
{
    const tcam_props_t *tcam = &tcam_info[bank];
    u32                i, cmd;

    if (up) {
        /* Moving up corresponds to move down command and must be done one entry at a time */
        cmd = VTSS_TCAM_CMD_MOVE_DOWN;
        L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG(tcam->target),
               VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_NUM_POS(1) |
               VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_SIZE(0));
        for (i = 0; i < count; i++) {
            VTSS_RC(l26_vcap_index_command(tcam, ix + i, cmd, VTSS_TCAM_SEL_ALL));
        }
    } else {
        /* Moving down corresponds to move up command */
        cmd = VTSS_TCAM_CMD_MOVE_UP;
        L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG(tcam->target),
               VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_NUM_POS(1) |
               VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_SIZE(count - 1));
        VTSS_RC(l26_vcap_index_command(tcam, ix + count - 1, cmd, VTSS_TCAM_SEL_ALL));
    }

    /* Move block starting at index */
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  IS1 functions
 * ================================================================= */

static BOOL l26_vcap_is_udp_tcp(vtss_vcap_u8_t *proto)
{
    return (proto->mask == 0xff && (proto->value == 6 || proto->value == 17));
}

static u32 l26_u8_to_u32(u8 *p)
{
    return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static vtss_rc l26_is1_prepare_action(const tcam_props_t *tcam,
                                      vtss_is1_data_t *is1,
                                      u32 counter)
{
    u32               entry[VTSS_TCAM_ENTRY_WIDTH];
    vtss_is1_action_t *action = &is1->entry->action;
    
    memset(entry, 0, sizeof(entry));
    if (is1->entry->key.type == VTSS_IS1_TYPE_SMAC_SIP) {
        vtss_bs_set(entry, 0, 2, IS1_ACTION_TYPE_SMAC_SIP4); /* Type */
        /* SIP/SMAC action */
        if (action->host_match) 
            vtss_bs_bit_set(entry, 2, 1); /* HOST_MATCH */
    } else {
        vtss_bs_set(entry, 0, 2, IS1_ACTION_TYPE_NORMAL); /* Type */

        /* Normal action */
        if (action->dscp_enable) {
            vtss_bs_bit_set(entry, 2, 1); /* DSCP_ENA */
            vtss_bs_set(entry, 3, 6, action->dscp);
        }
        if (action->dp_enable) {
            vtss_bs_bit_set(entry, 9, 1); /* DP_ENA */
            vtss_bs_bit_set(entry, 10, action->dp); /* DP_VAL */
        }
        if (action->prio_enable) {
            vtss_bs_bit_set(entry, 11, 1); /* QOS_ENA */
            vtss_bs_set(entry, 12, 3, action->prio); /* QOS_VAL */
        }
        if (action->pag_enable) {
            vtss_bs_bit_set(entry, 15, 1); /* PAG_ENA */
            vtss_bs_set(entry, 16, 8, action->pag); /* PAG_VAL */
        }
        if (action->vid) {
            vtss_bs_bit_set(entry, 24, 1); /* VID_REPLACE_ENA */
            vtss_bs_set(entry, 25, 12, action->vid); /* VID_ADD_VAL */
        }
        if (action->pcp_dei_enable) {
            vtss_bs_bit_set(entry, 51, 1); /* PCP_DEI_ENA */
            vtss_bs_set(entry, 52, 3, action->pcp); /* PCP_VAL */
            vtss_bs_bit_set(entry, 55, action->dei); /* DEI_VAL */
        }
        if (action->pop_enable) {
            vtss_bs_bit_set(entry, 56, 1); /* VLAN_POP_CNT_ENA */
            vtss_bs_set(entry, 57, 2, action->pop); /* VLAN_POP_CNT */
        }
        if (action->host_match)
            vtss_bs_bit_set(entry, 59, 1); /* HOST_MATCH */
        if (action->fid_sel) {
            vtss_bs_set(entry, 37, 2, action->fid_sel);     /* FID_SEL */
            vtss_bs_set(entry, 39, 12, action->fid_val);    /* FID_VAL */
        }
    }
    
    return l26_vcap_action2cache(tcam, entry, counter);
}

static vtss_rc l26_is1_prepare_key(const tcam_props_t *tcam, vtss_is1_data_t *is1) 
{
    u32                 entry[VTSS_TCAM_ENTRY_WIDTH];
    u32                 mask[VTSS_TCAM_ENTRY_WIDTH];
    vtss_is1_key_t      *key = &is1->entry->key;
    vtss_is1_tag_t      *tag = &key->tag;
    u32                 port, port_mask, etype_len, ip_snap, ip4 = 0, tcp_udp = 0, tcp = 0;
    u32                 l4_used = 0, msk;
    vtss_vcap_ip_t      sip;
    vtss_vcap_u8_t      *proto = NULL;
    vtss_vcap_vr_t      *dscp = NULL, *sport = NULL, *dport = NULL;
    vtss_vcap_u16_t     etype;
    u8                  range_value = 0, range_mask = 0;
    vtss_vcap_u48_t     smac;
    int                 i;
    vtss_vcap_bit_t     vcap_bit;

    memset(entry, 0, sizeof(entry));
    memset(mask, 0, sizeof(mask));
    memset(&etype, 0, sizeof(etype));
    
    if (key->type == VTSS_IS1_TYPE_SMAC_SIP) {
        /* SMAC/SIP entry */
        if (key->frame.smac_sip.port_no != VTSS_PORT_NO_NONE) {
            port = VTSS_CHIP_PORT(key->frame.smac_sip.port_no);
            L26_ACL_FIELD(0, 5, port, ACL_MASK_ONES, entry, mask); /* IGR_PORT */
        }
        for (i = 0; i < 6; i++) {
            smac.value[i] = key->frame.smac_sip.smac.addr[i];
            smac.mask[i] = 0xff;
        }
        L26_ACE_MAC(5, smac, entry, mask);     /* L2_SMAC */
        L26_ACL_FIELD(53, 32, key->frame.smac_sip.sip, ACL_MASK_ONES, entry, mask); /* L3_IP4_SIP */
        return l26_vcap_entry2cache(tcam, VCAP_TG_VAL_IS1_IP4, entry, mask);
    }
    
    /* Common fields */
    L26_ACL_BITSET(0, 0, entry, mask);                  /* IS1_TYPE */
    L26_ACL_BITSET(1, is1->lookup ? 0 : 1, entry, mask);         /* FIRST */
    port_mask = l26_port_mask(key->port_list);
    /* Inverse match - Match *zeroes* in port mask - enables multiple ports */
    L26_ACL_FIELD(2, 27, 0, ~port_mask, entry, mask);   /* IGR_PORT_MASK */
    L26_ACL_CBITSET(29, tag->tagged, entry, mask); /* VLAN_TAGGED */
    L26_ACL_CBITSET(30, key->inner_tag.tagged, entry, mask); /* VLAN_DBL_TAGGED */
    L26_ACL_CBITSET(31, tag->s_tag, entry, mask);  /* TPID */
    if (key->tag.vid.type == VTSS_VCAP_VR_TYPE_VALUE_MASK)
        L26_ACL_CFIELD(32, 12, tag->vid.vr.v.value, tag->vid.vr.v.mask, entry, mask); /* VID */
    if (is1->vid_range != VTSS_VCAP_RANGE_CHK_NONE) {
        range_mask |= (1<<is1->vid_range);
        if (tag->vid.type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
            range_value |= (1<<is1->vid_range);
    }
    L26_ACL_CBITSET(44, tag->dei, entry, mask);    /* DEI */
    L26_ACL_CFIELD(45, 3, tag->pcp.value, tag->pcp.mask, entry, mask);  /* PCP */
    L26_ACE_MAC(48, key->mac.smac, entry, mask);        /* L2_SMAC */
    L26_ACL_CBITSET(96, key->mac.dmac_mc, entry, mask); /* L2_MC */
    L26_ACL_CBITSET(97, key->mac.dmac_bc, entry, mask); /* L2_BC */

    switch (key->type) {
    case VTSS_IS1_TYPE_ANY:
        /* Only common fields and range are valid */
        L26_ACL_CFIELD(176, 8, range_value, range_mask, entry, mask);                      /* L4_RNG */
        return l26_vcap_entry2cache(tcam, VCAP_TG_VAL_IS1_IS1, entry, mask);
    case VTSS_IS1_TYPE_ETYPE:
        etype_len = 1;
        ip_snap = 0;
        ip4 = 0;
        etype = key->frame.etype.etype;
        sip.value = l26_u8_to_u32(key->frame.etype.data.value);
        sip.mask = l26_u8_to_u32(key->frame.etype.data.mask);
        break;
    case VTSS_IS1_TYPE_LLC:
        etype_len = 0;
        ip_snap = 0;
        ip4 = 0;
        etype.value[0] = key->frame.llc.data.value[0];
        etype.mask[0] = key->frame.llc.data.mask[0];
        etype.value[1] = key->frame.llc.data.value[1];
        etype.mask[1] = key->frame.llc.data.mask[1];
        sip.value = l26_u8_to_u32(&key->frame.llc.data.value[2]);
        sip.mask = l26_u8_to_u32(&key->frame.llc.data.mask[2]);
        break;
    case VTSS_IS1_TYPE_SNAP:
        etype_len = 0;
        ip_snap = 1;
        ip4 = 0;
        etype.value[0] = key->frame.snap.data.value[0];
        etype.mask[0] = key->frame.snap.data.mask[0];
        etype.value[1] = key->frame.snap.data.value[1];
        etype.mask[1] = key->frame.snap.data.mask[1];
        sip.value = l26_u8_to_u32(&key->frame.snap.data.value[2]);
        sip.mask = l26_u8_to_u32(&key->frame.snap.data.mask[2]);
        break;
    case VTSS_IS1_TYPE_IPV4:
    case VTSS_IS1_TYPE_IPV6:
        etype_len = 1;
        ip_snap = 1;
        if (key->type == VTSS_IS1_TYPE_IPV4) {
            ip4 = 1;
            vcap_bit = key->frame.ipv4.ip_mc;
            proto = &key->frame.ipv4.proto;
            dscp = &key->frame.ipv4.dscp;
            sip = key->frame.ipv4.sip;
            sport = &key->frame.ipv4.sport;
            dport = &key->frame.ipv4.dport;
        } else {
            ip4 = 0;
            vcap_bit = key->frame.ipv6.ip_mc;
            proto = &key->frame.ipv6.proto;
            dscp = &key->frame.ipv6.dscp;
            sip.value = l26_u8_to_u32(&key->frame.ipv6.sip.value[12]);
            sip.mask = l26_u8_to_u32(&key->frame.ipv6.sip.mask[12]);
            sport = &key->frame.ipv6.sport;
            dport = &key->frame.ipv6.dport;
        }
        L26_ACL_CBITSET(98, vcap_bit, entry, mask); /* IP_MC */
        if (l26_vcap_is_udp_tcp(proto)) {
            tcp_udp = 1;
            tcp = VTSS_BOOL(proto->value == 6);
            if (dport->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
                msk = dport->vr.v.mask;
                etype.value[0] = ((dport->vr.v.value >> 8) & 0xff);
                etype.value[1] = (dport->vr.v.value & 0xff);
                etype.mask[0] = ((msk >> 8) & 0xff);
                etype.mask[1] = (msk & 0xff);
                if (msk)
                    l4_used = 1;
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

    L26_ACL_BITSET(99, etype_len, entry, mask);                                        /* ETYPE_LEN */
    l26_acl_bytes(100, 2, etype.value, etype.mask, entry, mask);                       /* ETYPE */
    L26_ACL_BITSET(116, ip_snap, entry, mask);                                         /* IP_SNAP */

    if (dscp != NULL) {
        if (dscp->type == VTSS_VCAP_VR_TYPE_VALUE_MASK)
            L26_ACL_CFIELD(120, 6, dscp->vr.v.value, dscp->vr.v.mask, entry, mask);    /* L3_DSCP */
        if (is1->dscp_range != VTSS_VCAP_RANGE_CHK_NONE) {
            range_mask |= (1<<is1->dscp_range);
            if (dscp->type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
                range_value |= (1<<is1->dscp_range);
        }
    }
    L26_ACL_CFIELD(126, 32, sip.value, sip.mask, entry, mask);                         /* L3_SIP */
    if (tcp_udp) {
        L26_ACL_BITSET(158, tcp_udp, entry, mask);                                     /* TCP_UDP */
        L26_ACL_BITSET(159, tcp, entry, mask);                                         /* TCP */
        if (sport != NULL && sport->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
            msk = sport->vr.v.mask;
            L26_ACL_CFIELD(160, 16, sport->vr.v.value, msk, entry, mask); /* L4_SPORT */
            if (msk)
                l4_used = 1;
        }
        if (is1->sport_range != VTSS_VCAP_RANGE_CHK_NONE) {
            range_mask |= (1<<is1->sport_range);
            if (sport != NULL && sport->type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
                range_value |= (1<<is1->sport_range);
            l4_used = 1;
        }
        if (is1->dport_range != VTSS_VCAP_RANGE_CHK_NONE) {
            range_mask |= (1<<is1->dport_range);
            if (dport != NULL && dport->type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
                range_value |= (1<<is1->dport_range);
            l4_used = 1;
        }
    }
    if ((key->type == VTSS_IS1_TYPE_IPV4) || (key->type == VTSS_IS1_TYPE_IPV6)) {
        L26_ACL_BITSET(117, ip4, entry, mask);                                         /* IP4 */
        if (ip4) {
            /* If L4 filtering is used, IPv4 frame must not be fragment or have options */
            vcap_bit = (l4_used ? VTSS_VCAP_BIT_0 : key->frame.ipv4.fragment);
            L26_ACL_CBITSET(118, vcap_bit, entry, mask);               /* L3_FRAGMENT */
            vcap_bit = (l4_used ? VTSS_VCAP_BIT_0 : key->frame.ipv4.options);
            L26_ACL_CBITSET(119, vcap_bit, entry, mask);                /* L3_OPTIONS */
        }
    }
    L26_ACL_CFIELD(176, 8, range_value, range_mask, entry, mask);                      /* L4_RNG */
    
    return l26_vcap_entry2cache(tcam, VCAP_TG_VAL_IS1_IS1, entry, mask);
}

static vtss_rc l26_is1_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_I("row: %u", idx->row);

    return l26_vcap_entry_get(VTSS_TCAM_S1, idx->row, counter, clear);
}

static vtss_rc l26_is1_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter)
{
    const tcam_props_t *tcam = &tcam_info[VTSS_TCAM_S1];
    
    VTSS_I("row: %u", idx->row);
    
    /* Write entry at index */
    VTSS_RC(l26_is1_prepare_key(tcam, &data->u.is1));
    VTSS_RC(l26_is1_prepare_action(tcam, &data->u.is1, counter));

    return l26_vcap_index_command(tcam, idx->row, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ALL);
}

static vtss_rc l26_is1_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_I("row: %u", idx->row);
    
    return l26_vcap_entry_del(VTSS_TCAM_S1, idx->row);
}

static vtss_rc l26_is1_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_I("row: %u, count: %u, up: %u", idx->row, count, up);
    
    return l26_vcap_entry_move(VTSS_TCAM_S1, idx->row, count, up);
}

/* ================================================================= *
 *  IS2 functions
 * ================================================================= */

/* Setup action */
static vtss_rc l26_is2_prepare_action(const tcam_props_t *tcam,
                                      vtss_acl_action_t *action,
                                      u32 counter)
{
    u32 entry[VTSS_TCAM_ENTRY_WIDTH];
    u32 mode = 0, policer_ena = 0, policer = 0, mask;
    
    memset(entry, 0, sizeof(entry));

    if (action->cpu_once)
        entry[0] |= VTSS_BIT(0); /* HIT_ME_ONCE */
    if (action->cpu)
        entry[0] |= VTSS_BIT(1); /* CPU_COPY_ENA */
    if (action->cpu_once || action->cpu) /* CPU_QU_NUM */
        entry[0] |= VTSS_ENCODE_BITFIELD(action->cpu_queue - VTSS_PACKET_RX_QUEUE_START, 2, 3);

    /* MASK_MODE */
    switch (action->port_action) {
    case VTSS_ACL_PORT_ACTION_NONE:
        mode = 0;
        break;
    case VTSS_ACL_PORT_ACTION_FILTER:
        mode = 1;
        break;
    case VTSS_ACL_PORT_ACTION_REDIR:
        mode = 3;
        break;
    default:
        VTSS_E("unknown port_action");
        return VTSS_RC_ERROR;
    }
    entry[0] |= VTSS_ENCODE_BITFIELD(mode, 5, 2); 
    
    if (action->mirror)
        entry[0] |= VTSS_BIT(7); /* MIRROR_ENA */
        
    if (!action->learn)
        entry[0] |= VTSS_BIT(8); /* LRN_DIS */

    mask = l26_port_mask(action->port_list);
    if (mode != 0 && mask == 0 && action->cpu_once == 0 && action->cpu == 0) {
        /* Forwarding and CPU copy disabled, discard using policer to avoid CPU copy */
        policer_ena = 1;
        policer = L26_ACL_POLICER_DISC;
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    } else if (action->evc_police) {
        policer_ena = 1;
        policer = vtss_state->evc_policer_alloc[action->evc_policer_id].policer;
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
    } else if (action->police) {
        policer_ena = 1;
        policer = vtss_state->acl_policer_alloc[action->policer_no].policer;
    }
    if (policer_ena)
        entry[0] |= (VTSS_BIT(9) | /* POLICE_ENA */
                     VTSS_ENCODE_BITFIELD(policer, 10, 8)); /* POLICE_IDX */
    /* PORT_MASK */
    if (mode)
        vtss_bs_set(entry, 18, 26, mask);

    /* PTP_ENA */
    switch (action->ptp_action) {
    case VTSS_ACL_PTP_ACTION_NONE:
        mode = 0;
        break;
    case VTSS_ACL_PTP_ACTION_ONE_STEP:
        mode = 1;
        break;
    case VTSS_ACL_PTP_ACTION_TWO_STEP:
        mode = 2;
        break;
    case VTSS_ACL_PTP_ACTION_ONE_AND_TWO_STEP:
        mode = 3;
        break;
    default:
        VTSS_E("unknown ptp_action");
        return VTSS_RC_ERROR;
    }
    vtss_bs_set(entry, 44, 2, mode);

    return l26_vcap_action2cache(tcam, entry, counter);
}

static vtss_rc l26_is2_prepare_key(const tcam_props_t *tcam, vtss_is2_data_t *is2) 
{
    u32                entry[VTSS_TCAM_ENTRY_WIDTH];
    u32                mask[VTSS_TCAM_ENTRY_WIDTH], port_mask;
    BOOL               ipv4 = FALSE;
    int                type = ACL_TYPE_ETYPE;
    vtss_ace_t         *ace = &is2->entry->ace;
    vtss_ace_udp_tcp_t *sport, *dport;
    u8                 range = 0;
    vtss_ace_u48_t     smac, dmac;
    vtss_ace_u16_t     mac_data;
    vtss_ace_u8_t      *proto;
    vtss_ace_bit_t     *ttl, *tcp_fin, *tcp_syn, *tcp_rst, *tcp_psh, *tcp_ack, *tcp_urg;
    vtss_ace_bit_t     *sip_eq_dip, *sport_eq_dport, *seq_zero;
    vtss_ace_u8_t      *ds;
    vtss_ace_ip_t      sip, dip;
    vtss_ace_u48_t     ip_data;
    vtss_ace_ptp_t     *ptp;
    vtss_ace_u128_t    *sipv6;
    
    memset(entry, 0, sizeof(entry));
    memset(mask, 0, sizeof(mask));

    switch (ace->type) {
    case VTSS_ACE_TYPE_ANY:
        type = ACL_TYPE_ANY;
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
    case VTSS_ACE_TYPE_IPV6:
        if (ace->type == VTSS_ACE_TYPE_IPV4) {
            ipv4 = TRUE;
            proto = &ace->frame.ipv4.proto;
        } else 
            proto = &ace->frame.ipv6.proto;
        if (l26_vcap_is_udp_tcp(proto)) {
            /* UDP/TCP */
            type = ACL_TYPE_UDP_TCP;
        } else {
            /* IPv4 */
            type = ACL_TYPE_IPV4;
        }
        break;
    default:
        VTSS_E("illegal type: %d", ace->type);
        return VTSS_RC_ERROR;
    }

    /* Common Fields */
    if (type != ACL_TYPE_ANY)
        L26_ACL_FIELD(0, 3, type, ACL_MASK_ONES, entry, mask); /* IS2_TYPE */
    L26_ACL_BITSET(3, is2->entry->first, entry, mask); /* FIRST */
    
    L26_ACL_FIELD(4, 8, ace->policy.value, ace->policy.mask, entry, mask); /* PAG */

    /* Inverse match - Match *zeroes* in port mask - enables multiple ports */
    port_mask = l26_port_mask(ace->port_list);
    L26_ACL_FIELD(12, 27, 0, ~port_mask, entry, mask); /* IGR_PORT_MASK */

    L26_ACL_CBITSET(39, ace->vlan.tagged, entry, mask); /* VLAN_TAGGED */
    if (is2->entry->host_match)
        L26_ACL_BITSET(40, TRUE, entry, mask); /* HOST_MATCH */
    /* VID */
    L26_ACL_CFIELD(41, 12, ace->vlan.vid.value, ace->vlan.vid.mask, entry, mask);
    /* DEI (formerly CFI) */
    L26_ACL_CBITSET(53, ace->vlan.cfi, entry, mask); 
    /* PCP (formerly UPRIO) */
    L26_ACL_CFIELD(54, 3, ace->vlan.usr_prio.value, ace->vlan.usr_prio.mask, entry, mask);
    switch (type) {
    case ACL_TYPE_ETYPE:
    case ACL_TYPE_LLC:
    case ACL_TYPE_SNAP:
        /* Common format */
        L26_ACL_CBITSET(153, ace->dmac_mc, entry, mask); /* L2_MC */
        L26_ACL_CBITSET(154, ace->dmac_bc, entry, mask); /* L2_BC */
        /* Specific format */
        switch (type) {
        case ACL_TYPE_ETYPE:
            dmac = ace->frame.etype.dmac;
            smac = ace->frame.etype.smac;
            mac_data = ace->frame.etype.data;
            ptp = &ace->frame.etype.ptp;
            if (ptp->enable) {
                /* PTP header filtering, byte 0, 1, 4 and 6 */
                mac_data.value[0] = ptp->header.value[0]; /* PTP byte 0 */
                mac_data.mask[0] = ptp->header.mask[0];   /* PTP byte 0 */
                mac_data.value[1] = ptp->header.value[1]; /* PTP byte 1 */
                mac_data.mask[1] = ptp->header.mask[1];   /* PTP byte 1 */
                if (!is2->entry->first) {
                    /* Override SMAC byte 2 and 4 in second lookup */
                    memset(&smac, 0, sizeof(smac));
                    smac.value[2] = ptp->header.value[2]; /* PTP byte 4 */
                    smac.mask[2] = ptp->header.mask[2];   /* PTP byte 4 */
                    smac.value[4] = ptp->header.value[3]; /* PTP byte 6 */
                    smac.mask[4] = ptp->header.mask[3];   /* PTP byte 6 */
                }
            }
            
            /* ETYPE */
            l26_acl_bytes(155, 2, ace->frame.etype.etype.value, ace->frame.etype.etype.mask, entry, mask);
            /* L2_PAYLOAD */
            l26_acl_bytes(171, 2, mac_data.value, mac_data.mask, entry, mask);
            break;
        case ACL_TYPE_LLC:
            dmac = ace->frame.llc.dmac;
            smac = ace->frame.llc.smac;
            /* L2_LLC */
            l26_acl_bytes(155, 4, ace->frame.llc.llc.value, ace->frame.llc.llc.mask, entry, mask);
            break;
        case ACL_TYPE_SNAP:
            dmac = ace->frame.snap.dmac;
            smac = ace->frame.snap.smac;
            /* L2_SNAP bytes 0-3 */
            l26_acl_bytes(155, 4, ace->frame.snap.snap.value, ace->frame.snap.snap.mask, entry, mask);
            /* L2_SNAP bytes 4 */
            l26_acl_bytes(187, 1, ace->frame.snap.snap.value+4, ace->frame.snap.snap.mask+4, entry, mask);
            break;
        }
        L26_ACE_MAC(57, dmac, entry, mask); /* L2_DMAC */
        L26_ACE_MAC(105, smac, entry, mask); /* L2_SMAC */
        break;
    case ACL_TYPE_ARP:
        L26_ACE_MAC(57, ace->frame.arp.smac, entry, mask); /* L2_SMAC */
        L26_ACL_CBITSET(105, ace->dmac_mc, entry, mask); /* L2_MC */
        L26_ACL_CBITSET(106, ace->dmac_bc, entry, mask); /* L2_BC */
        L26_ACL_CBITSET(107, ace->frame.arp.ethernet, entry, mask); /* ARP_ADDR_SPACE_OK */
        L26_ACL_CBITSET(108, ace->frame.arp.ip, entry, mask); /* ARP_PROTO_SPACE_OK */
        L26_ACL_CBITSET(109, ace->frame.arp.length, entry, mask); /* ARP_LEN_OK */
        L26_ACL_CBITSET(110, ace->frame.arp.dmac_match, entry, mask); /* ARP_TARGET_MATCH */
        L26_ACL_CBITSET(111, ace->frame.arp.smac_match, entry, mask); /* ARP_SENDER_MATCH */
        L26_ACL_CBITSET(112, ace->frame.arp.unknown, entry, mask); /* ARP_UNKNOWN_OPCODE */
        L26_ACL_CBITSET_INV(113, ace->frame.arp.req, entry, mask); /* ARP_OPCODE */
        L26_ACL_CBITSET_INV(114, ace->frame.arp.arp, entry, mask); /* ARP_OPCODE */
        L26_ACL_CFIELD(115, 32, ace->frame.arp.dip.value, ace->frame.arp.dip.mask, entry, mask);
        L26_ACL_CFIELD(147, 32, ace->frame.arp.sip.value, ace->frame.arp.sip.mask, entry, mask);
        /* ARP_DIP_EQ_SIP (bit 179): Not supported */
        break;
    case ACL_TYPE_UDP_TCP:
    case ACL_TYPE_IPV4:
        if (ipv4) {
            /* IPv4 */
            ttl = &ace->frame.ipv4.ttl;
            ds = &ace->frame.ipv4.ds;
            proto = &ace->frame.ipv4.proto;
            sip = ace->frame.ipv4.sip;
            dip = ace->frame.ipv4.dip;
            ip_data = ace->frame.ipv4.data;
            sport = &ace->frame.ipv4.sport;
            dport = &ace->frame.ipv4.dport;
            tcp_fin = &ace->frame.ipv4.tcp_fin;
            tcp_syn = &ace->frame.ipv4.tcp_syn;
            tcp_rst = &ace->frame.ipv4.tcp_rst;
            tcp_psh = &ace->frame.ipv4.tcp_psh;
            tcp_ack = &ace->frame.ipv4.tcp_ack;
            tcp_urg = &ace->frame.ipv4.tcp_urg;
            sip_eq_dip = &ace->frame.ipv4.sip_eq_dip;
            sport_eq_dport = &ace->frame.ipv4.sport_eq_dport;
            seq_zero = &ace->frame.ipv4.seq_zero;
            ptp = &ace->frame.ipv4.ptp;
            if (is2->entry->host_match) {
                sip.value = ace->frame.ipv4.sip_smac.sip;
                sip.mask = ACL_MASK_ONES;
            }
        } else {
            /* IPv6 */
            proto = &ace->frame.ipv6.proto;
            ttl = &ace->frame.ipv6.ttl;
            ds = &ace->frame.ipv6.ds;
            sipv6 = &ace->frame.ipv6.sip;
            sip.value = l26_u8_to_u32(&sipv6->value[12]);
            sip.mask = l26_u8_to_u32(&sipv6->mask[12]);
            dip.value = l26_u8_to_u32(&sipv6->value[8]);
            dip.mask = l26_u8_to_u32(&sipv6->mask[8]);
            ip_data = ace->frame.ipv6.data;
            sport = &ace->frame.ipv6.sport;
            dport = &ace->frame.ipv6.dport;
            tcp_fin = &ace->frame.ipv6.tcp_fin;
            tcp_syn = &ace->frame.ipv6.tcp_syn;
            tcp_rst = &ace->frame.ipv6.tcp_rst;
            tcp_psh = &ace->frame.ipv6.tcp_psh;
            tcp_ack = &ace->frame.ipv6.tcp_ack;
            tcp_urg = &ace->frame.ipv6.tcp_urg;
            sip_eq_dip = &ace->frame.ipv6.sip_eq_dip;
            sport_eq_dport = &ace->frame.ipv6.sport_eq_dport;
            seq_zero = &ace->frame.ipv6.seq_zero;
            ptp = &ace->frame.ipv6.ptp;
        }
        
        /* Common format */
        L26_ACL_CBITSET(57, ace->dmac_mc, entry, mask); /* L2_MC */
        L26_ACL_CBITSET(58, ace->dmac_bc, entry, mask); /* L2_BC */
        L26_ACL_BITSET(59, ipv4, entry, mask); /* IP4 */
        if (ipv4) {
            L26_ACL_CBITSET(60, ace->frame.ipv4.fragment, entry, mask); /* L3_FRAGMENT */
            /* L3_FRAG_OFS_GT0 (bit 61): Not supported */
            L26_ACL_CBITSET(62, ace->frame.ipv4.options, entry, mask); /* L3_OPTIONS */
        }
        L26_ACL_CBITSET(63, *ttl, entry, mask); /* L3_TTL_GT0 */
        /* L3_TOS */
        L26_ACL_CFIELD(64, 8, ds->value, ds->mask, entry, mask);
        if(type == ACL_TYPE_UDP_TCP) {
            /* Specific format - Bit 72 FF */
            if (!is2->entry->udp_tcp_any)
                L26_ACL_BITSET(72, proto->value == 6, entry, mask); /* TCP */
            /* L4_1588_DOM (bit 73): Not supported */

            /* L3_IP4_DIP */
            L26_ACL_CFIELD(81, 32, dip.value, dip.mask, entry, mask);
            /* L3_IP4_SIP */
            if (ptp->enable && !is2->entry->first) {
                /* Override SIP in second lookup */
                l26_acl_bytes(113, 4, ptp->header.value, ptp->header.mask, entry, mask);
            } else {
                L26_ACL_CFIELD(113, 32, sip.value, sip.mask, entry, mask);
            }
            L26_ACL_CBITSET(145, *sip_eq_dip, entry, mask); /* DIP_EQ_SIP */
            
            /* L3_DPORT */
            if (dport->in_range && dport->low == dport->high)
                L26_ACL_FIELD(146, 16, dport->low, ACL_MASK_ONES, entry, mask);
            /* L3_SPORT */
            if(sport->in_range && sport->low == sport->high)
                L26_ACL_FIELD(162, 16, sport->low, ACL_MASK_ONES, entry, mask);
            /* L4_RNG */
            if (is2->srange != VTSS_VCAP_RANGE_CHK_NONE)
                range |= (1<<is2->srange);
            if (is2->drange != VTSS_VCAP_RANGE_CHK_NONE)
                range |= (1<<is2->drange);
            L26_ACL_CFIELD(178, 8, range, range, entry, mask);
            L26_ACL_CBITSET(186, *sport_eq_dport, entry, mask); /* SPORT_EQ_DPORT */
            L26_ACL_CBITSET(187, *seq_zero, entry, mask); /* SEQUENCE_EQ0 */
            L26_ACL_CBITSET(188, *tcp_fin, entry, mask); /* L4_FIN */
            L26_ACL_CBITSET(189, *tcp_syn, entry, mask); /* L4_SYN */
            L26_ACL_CBITSET(190, *tcp_rst, entry, mask); /* L4_RST */
            L26_ACL_CBITSET(191, *tcp_psh, entry, mask); /* L4_PSH */
            L26_ACL_CBITSET(192, *tcp_ack, entry, mask); /* L4_ACK */
            L26_ACL_CBITSET(193, *tcp_urg, entry, mask); /* L4_URG */
        } else {            /* ACL_TYPE_IPV4 */
            /* Specific format - Bit 72 FF */
            /* L3_PROTO */
            L26_ACL_CFIELD(72, 8, proto->value, proto->mask, entry, mask);
            /* L3_IP4_DIP */
            L26_ACL_CFIELD(80, 32, dip.value, dip.mask, entry, mask);
            /* L3_IP4_SIP */
            L26_ACL_CFIELD(112, 32, sip.value, sip.mask, entry, mask);
            L26_ACL_CBITSET(144, *sip_eq_dip, entry, mask); /* DIP_EQ_SIP */

            /* L3_PAYLOAD */
            L26_ACE_MAC(145, ip_data, entry, mask);
        }
        break;
    case ACL_TYPE_ANY:
        break;
    default:
        VTSS_E("illegal type: %d", type);
        return VTSS_RC_ERROR;
    }

    return l26_vcap_entry2cache(tcam, VCAP_TG_VAL_IS2, entry, mask);
}

static vtss_rc l26_is2_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_I("row: %u", idx->row);

    return l26_vcap_entry_get(VTSS_TCAM_S2, idx->row, counter, clear);
}

static vtss_rc l26_is2_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter)
{
    const tcam_props_t *tcam = &tcam_info[VTSS_TCAM_S2];
    
    VTSS_I("row: %u", idx->row);
    
    /* Write entry at index */
    VTSS_RC(l26_is2_prepare_key(tcam, &data->u.is2));
    VTSS_RC(l26_is2_prepare_action(tcam, &data->u.is2.entry->ace.action, counter));
    return l26_vcap_index_command(tcam, idx->row, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ALL);
}

static vtss_rc l26_is2_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_I("row: %u", idx->row);
    
    return l26_vcap_entry_del(VTSS_TCAM_S2, idx->row);
}

static vtss_rc l26_is2_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_I("row: %u, count: %u, up: %u", idx->row, count, up);
    
    return l26_vcap_entry_move(VTSS_TCAM_S2, idx->row, count, up);
}

static vtss_rc l26_is2_port_get(u32 port, u32 *counter, BOOL clear)
{
    return l26_vcap_port_get(VTSS_TCAM_S2, port, counter, clear);
}

/* ================================================================= *
 *  ES0 functions
 * ================================================================= */

/* Setup action */
static vtss_rc l26_es0_prepare_action(const tcam_props_t *tcam,
                                      vtss_es0_action_t *action,
                                      u32 counter)
{
    u32 entry[VTSS_TCAM_ENTRY_WIDTH];
    
    memset(entry, 0, sizeof(entry));
    vtss_bs_set(entry, 0, 1, 1);               /* VLD */
    vtss_bs_set(entry, 1, 2, action->tag);     /* TAG_ES0 */
    vtss_bs_set(entry, 3, 2, action->tpid);    /* TAG_TPID_SEL */
    vtss_bs_set(entry, 5, 2, 2);               /* TAG_VID_SEL */
    vtss_bs_set(entry, 7, 12, action->vid_a);  /* VID_A_VAL */
    vtss_bs_set(entry, 19, 12, action->vid_b); /* VID_B_VAL */
    vtss_bs_set(entry, 31, 2, action->qos);    /* QOS_SRC_SEL */
    vtss_bs_set(entry, 33, 3, action->pcp);    /* PCP_VAL */
    vtss_bs_set(entry, 36, 1, action->dei);    /* DEI_VAL */
    
    return l26_vcap_action2cache(tcam, entry, counter);
}

static vtss_rc l26_es0_prepare_key(const tcam_props_t *tcam, vtss_es0_key_t *key) 
{
    u32 entry[VTSS_TCAM_ENTRY_WIDTH];
    u32 mask[VTSS_TCAM_ENTRY_WIDTH];
    u32 port;

    memset(entry, 0, sizeof(entry));
    memset(mask, 0, sizeof(mask));
    
    if (key->port_no != VTSS_PORT_NO_NONE) {
        port = VTSS_CHIP_PORT(key->port_no);
        L26_ACL_FIELD(0, 5, port, ACL_MASK_ONES, entry, mask); /* EGR_PORT */
    }
    L26_ACL_FIELD(10, 12, key->data.vid.vid, ACL_MASK_ONES, entry, mask); /* VID */
    return l26_vcap_entry2cache(tcam, VCAP_TG_VAL_ES0, entry, mask);
}

static vtss_rc l26_es0_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *data, u32 counter)
{
    const tcam_props_t *tcam = &tcam_info[VTSS_TCAM_ES0];
    vtss_es0_data_t    *es0 = &data->u.es0;
    vtss_es0_action_t  *action = &es0->entry->action;

    VTSS_I("row: %u", idx->row);
    
    /* Write entry at index */
    vtss_cmn_es0_action_get(es0);
    VTSS_RC(l26_es0_prepare_key(tcam, &es0->entry->key));
    VTSS_RC(l26_es0_prepare_action(tcam, action, counter));
    return l26_vcap_index_command(tcam, idx->row, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ALL);
}

static vtss_rc l26_es0_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_I("row: %u", idx->row);

    return l26_vcap_entry_get(VTSS_TCAM_ES0, idx->row, counter, clear);
}

static vtss_rc l26_es0_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_I("row: %u", idx->row);
    
    return l26_vcap_entry_del(VTSS_TCAM_ES0, idx->row);
}

static vtss_rc l26_es0_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_I("row: %u, count: %u, up: %u", idx->row, count, up);
    
    return l26_vcap_entry_move(VTSS_TCAM_ES0, idx->row, count, up);
}

/* Update outer tag TPID for ES0 entry if VLAN port type has changed */
static vtss_rc l26_es0_entry_update(vtss_vcap_idx_t *idx, vtss_es0_data_t *es0) 
{
    const tcam_props_t *tcam = &tcam_info[VTSS_TCAM_ES0];
    u32                entry[VTSS_TCAM_ENTRY_WIDTH];
    vtss_es0_action_t  *action = &es0->entry->action;

    VTSS_RC(l26_vcap_index_command(tcam, idx->row, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_ACTION));
    VTSS_RC(l26_vcap_cache2action(tcam, entry));
    if (es0->flags & VTSS_ES0_FLAG_TPID) {
        vtss_bs_set(entry, 3, 2, action->tpid); /* TAG_TPID_SEL */
    }
    if (es0->flags & VTSS_ES0_FLAG_QOS) {
        vtss_bs_set(entry, 31, 2, action->qos); /* QOS_SRC_SEL */
        vtss_bs_set(entry, 33, 3, action->pcp); /* PCP_VAL */
        vtss_bs_set(entry, 36, 1, action->dei); /* DEI_VAL */
    }
    VTSS_RC(l26_vcap_action2cache(tcam, entry, 0));
    VTSS_RC(l26_vcap_index_command(tcam, idx->row, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ACTION));

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Miscellaneous
 * ================================================================= */

static vtss_rc l26_chip_id_get(vtss_chip_id_t *const chip_id)
{
    u32 value;
    
    L26_RD(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID, &value);
    
    if (value == 0 || value == 0xffffffff) {
        VTSS_E("CPU interface error, chipid: 0x%08x", value);
        return VTSS_RC_ERROR;
    }
    
    chip_id->part_number = VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_PART_ID(value);
    chip_id->revision = VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_REV_ID(value);
    
    return VTSS_RC_OK;
}


static vtss_rc l26_ptp_event_poll(vtss_ptp_event_type_t *ev_mask)
{
    u32 sticky, mask;
    
    /* PTP events */
    *ev_mask = 0;
    L26_RD(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, &sticky);
    L26_WR(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, sticky);
    L26_RD(VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG, &mask);
    mask |= VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY; /* CLK ADJ event has no enable bit - do not generate interrupt */
    sticky &= mask;      /* Only handle enabled sources */

    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_SYNC_STAT) ? VTSS_PTP_SYNC_EV : 0;
    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_EXT_SYNC_CURRENT_TIME_STICKY) ? VTSS_PTP_EXT_SYNC_EV : 0;
    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY) ? VTSS_PTP_CLK_ADJ_EV : 0;
    VTSS_I("sticky: 0x%x, ev_mask 0x%x", sticky, *ev_mask);

    return VTSS_RC_OK;
}

static vtss_rc l26_ptp_event_enable(vtss_ptp_event_type_t ev_mask, BOOL enable)
{
    
    /* PTP masks */
    
    if (ev_mask & VTSS_PTP_SYNC_EV) {
        L26_WRM(VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG, 
            enable ? VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_SYNC_STAT_ENA : 0, 
            VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_SYNC_STAT_ENA);
    }
    if (ev_mask & VTSS_PTP_EXT_SYNC_EV) {
        L26_WRM(VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG, 
            enable ? VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_EXT_SYNC_CURRENT_TIME_ENA : 0, 
            VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_EXT_SYNC_CURRENT_TIME_ENA);
    }
    return VTSS_RC_OK;
}


// Function for setup interrupt
// In: intr_mask - The interrupt bit mask bit.
//     polarity  - The interrupt polarity.
//     enable    - Set to enable the interrupts        
static vtss_rc l26_intr_cfg(const u32                intr_mask,
                            const BOOL               polarity,
                            const BOOL               enable)
{
    u32 intr_bit;
    
    // Loop through the whole interrupt vector and setup the bits in the mask.
    for (intr_bit = 0; intr_bit < 32; intr_bit++) {
        // If the bit is set in the mask then setup interrupt
        if (intr_mask & (1 << intr_bit)) {
            
            // Setup interrupt polarity.
            L26_WRM(VTSS_ICPU_CFG_INTR_DEV_POL, polarity << intr_bit, 1 << intr_bit);
                
            // Enable / Disable the interrupt
            if (vtss_state->chip_id.revision == 0x1 && intr_bit == 28) {
                // Bug in Rev. B (bugzilla#3995) PHY interrupt is stocked to 0, and should not be enabled.
                L26_WRM(VTSS_ICPU_CFG_INTR_DEV_ENA, 0, 1 << 28);
            } else {
                L26_WRM(VTSS_ICPU_CFG_INTR_DEV_ENA, enable << intr_bit, 1 << intr_bit);
                VTSS_D("enable:%d - 0x%X, intr_bit:%d, mask:0x%X, polarity:%d", enable, enable << intr_bit, intr_bit, intr_mask, polarity);
            }
        }
    }
    return VTSS_RC_OK;
}


static vtss_rc l26_intr_pol_negation(void)
{
    u32 ident, polarity;
    
    L26_RD(VTSS_ICPU_CFG_INTR_DEV_IDENT, &ident);           /* Get active interrupt indications on Fast Link Fail */
    ident &= 0xFFF;

    L26_RD(VTSS_ICPU_CFG_INTR_DEV_POL, &polarity);          /* Get polarity and negate the active ones */
    L26_WR(VTSS_ICPU_CFG_INTR_DEV_POL, polarity ^ ident );

    return VTSS_RC_OK;
}


/* =================================================================
 *  Miscellaneous - GPIO
 * =================================================================*/

static vtss_rc l26_gpio_mode(const vtss_chip_no_t   chip_no,
                             const vtss_gpio_no_t   gpio_no,
                             const vtss_gpio_mode_t mode)
{
    u32 mask = VTSS_BIT(gpio_no);
    L26_WRM_CLR(VTSS_DEVCPU_GCB_GPIO_GPIO_INTR_ENA, mask); /* Disable IRQ */
    switch(mode) {
    case VTSS_GPIO_OUT:
    case VTSS_GPIO_IN:
    case VTSS_GPIO_IN_INT:
        L26_WRM_CLR(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), mask); /* GPIO mode 0b00 */
        L26_WRM_CLR(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(1), mask); /* -"- */
        L26_WRM(VTSS_DEVCPU_GCB_GPIO_GPIO_OE, mode == VTSS_GPIO_OUT ? mask : 0, mask);
        if(mode == VTSS_GPIO_IN_INT)
            L26_WRM_SET(VTSS_DEVCPU_GCB_GPIO_GPIO_INTR_ENA, mask);
        return VTSS_RC_OK;
    case VTSS_GPIO_ALT_0:
        L26_WRM_SET(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), mask); /* GPIO mode 0b01 */
        L26_WRM_CLR(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(1), mask); /* -"- */
        return VTSS_RC_OK;
    case VTSS_GPIO_ALT_1:
        L26_WRM_CLR(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), mask); /* GPIO mode 0b10 */
        L26_WRM_SET(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(1), mask); /* -"- */
        return VTSS_RC_OK;
    default:
        break;
    }
    return VTSS_RC_ERROR;
}

static vtss_rc l26_gpio_read(const vtss_chip_no_t  chip_no,
                             const vtss_gpio_no_t  gpio_no,
                             BOOL                  *const value)
{
    u32 val,  mask = VTSS_BIT(gpio_no);
    L26_RD(VTSS_DEVCPU_GCB_GPIO_GPIO_IN, &val);
    *value = VTSS_BOOL(val & mask);
    return VTSS_RC_OK;
}

static vtss_rc l26_gpio_write(const vtss_chip_no_t  chip_no,
                              const vtss_gpio_no_t  gpio_no, 
                              const BOOL            value)
{
    u32 mask = VTSS_BIT(gpio_no);
    if(value) {
        L26_WR(VTSS_DEVCPU_GCB_GPIO_GPIO_OUT_SET, mask);
    } else {
        L26_WR(VTSS_DEVCPU_GCB_GPIO_GPIO_OUT_CLR, mask);
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_sgpio_event_poll(const vtss_chip_no_t     chip_no,
                                    const vtss_sgpio_group_t group,
                                    const u32                bit,
                                    BOOL                     *const events)
{
    u32 i, val = 0;

    L26_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INT_REG(bit), &val);
    L26_WR(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INT_REG(bit), val);     /* Clear pending */

    /* Setup serial IO port enable register */
    for (i=0; i<32; i++) {
        events[i] = (val & 1<<i) ? TRUE : FALSE;
    }

    return VTSS_RC_OK;
}


static vtss_rc l26_sgpio_event_enable(const vtss_chip_no_t     chip_no,
                                      const vtss_sgpio_group_t group,
                                      const u32                port,
                                      const u32                bit,
                                      const BOOL               enable)
{
    u32 data, pol, i;

    if (enable) {
        L26_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INPUT_DATA(bit), &data);
        L26_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INT_POL(bit), &pol);
        pol = ~pol;     /* '0' means interrupt when input is one */
        data &= pol;    /* Now data '1' means active interrupt */
        if (!(data & 1<<port))    /* Only enable if not active interrupt - as interrupt pending cannot be cleared */
            L26_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_INT_ENA, 1<<port, 1<<port);
        L26_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG, VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit), VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit));
    }
    else {
        L26_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_INT_ENA, 0, 1<<port);
        for (i=0; i<32; ++i)
            if (vtss_state->sgpio_event_enable[group].enable[i][bit])      break;
        if (i == 32)
            L26_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG, 0, VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit));
    }

    return VTSS_RC_OK;
}

static vtss_rc l26_sgpio_conf_set(const vtss_chip_no_t     chip_no,
                                  const vtss_sgpio_group_t group,
                                  const vtss_sgpio_conf_t  *const conf)
{
    u32 i, port, val = 0, bmode[2], bit_idx;

    /* Setup serial IO port enable register */
    for (port = 0; port < 32; port++) {
        if (conf->port_conf[port].enabled)
            val |= VTSS_BIT(port);
    }
    L26_WR(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_ENABLE, val);

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
                VTSS_E("blink mode 0 does not support TOGGLE");
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

    /* Configure "LD" polarity signal to 0 (active low) for input SGPIO */
    val = (VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BMODE_0(bmode[0]) |
           VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BMODE_1(bmode[1]) |
           VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BURST_GAP(0x1F) |
           VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_PORT_WIDTH(conf->bit_count - 1) |
           VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_AUTO_REPEAT);
    L26_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG, val, ~VTSS_M_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA);

    /* Setup the serial IO clock frequency - 12.5MHz (0x14) */
    L26_WR(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CLOCK, 0x14);

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
        L26_WR(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_CONFIG(port), val);
    }

    return VTSS_RC_OK;
}

static vtss_rc l26_sgpio_read(const vtss_chip_no_t     chip_no,
                              const vtss_sgpio_group_t group,
                              vtss_sgpio_port_data_t   data[VTSS_SGPIO_PORTS])
{
    u32 i, port, value;

    for (i = 0; i < 4; i++) {
        L26_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INPUT_DATA(i), &value);
        for (port = 0; port < 32; port++) {
            data[port].value[i] = VTSS_BOOL(value & (1 << port));
        }
    }

    return VTSS_RC_OK;
}

/* =================================================================
 *  Serdes configuration
 * =================================================================*/

static const char *l26_serdes_mode_txt(vtss_serdes_mode_t mode)
{
    return (mode == VTSS_SERDES_MODE_DISABLE ? "DISABLE" : 
            mode == VTSS_SERDES_MODE_2G5 ? "2G5" : 
            mode == VTSS_SERDES_MODE_QSGMII ? "QSGMII" : 
            mode == VTSS_SERDES_MODE_SGMII ? "SGMII" : "?");
}

/* Serdes1G: Read/write data */
static vtss_rc l26_sd1g_read_write(u32 addr, BOOL write, u32 nsec)
{
    u32 data, mask;
    
    if (write)
        mask = VTSS_F_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_WR_ONE_SHOT;
    else
        mask = VTSS_F_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_RD_ONE_SHOT;

    L26_WR(VTSS_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG,
           VTSS_F_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_ADDR(addr) | mask);
    
    do { /* Wait until operation is completed  */
        L26_RD(VTSS_MACRO_CTRL_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG, &data);
    } while (data & mask);

    if (nsec)
        VTSS_NSLEEP(nsec);
    
    return VTSS_RC_OK;
}

/* Serdes1G: Read data */
static vtss_rc l26_sd1g_read(u32 addr) 
{
    return l26_sd1g_read_write(addr, 0, 0);
}

/* Serdes1G: Write data */
static vtss_rc l26_sd1g_write(u32 addr, u32 nsec) 
{
    return l26_sd1g_read_write(addr, 1, nsec);
}

/* Wait 100 usec after some SerDes operations */
#define L26_SERDES_WAIT 100000
#define RCOMP_CFG0 VTSS_IOREG(VTSS_TO_MACRO_CTRL,0x8)
static vtss_rc l26_sd1g_cfg(vtss_serdes_mode_t mode, u32 addr) 
{
    BOOL ena_lane, if_100fx, ib_ena_dc_coupling=0;
    u32  ob_amp_ctrl, rcomp_val, resist_val, value, rev;

    VTSS_D("addr: 0x%x, mode: %s", addr, l26_serdes_mode_txt(mode));
    
    switch (mode) {
    case VTSS_SERDES_MODE_SGMII:
        ena_lane = 1;
        ob_amp_ctrl = 12;
        if_100fx = 0;
        break;
    case VTSS_SERDES_MODE_100FX:
        ena_lane = 1;
        ob_amp_ctrl = 12;
        if_100fx = 1;
        ib_ena_dc_coupling = 1;
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
        VTSS_E("Serdes1g mode %s not supported", l26_serdes_mode_txt(mode));
        return VTSS_RC_ERROR;
    }

    L26_RD(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID, &value);
    rev = VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_REV_ID(value);

    /* RCOMP_CFG0.MODE_SEL = 2 */
    VTSS_RC(l26_wr(RCOMP_CFG0,0x3<<8));
    
    /* RCOMP_CFG0.RUN_CAL = 1 */
    VTSS_RC(l26_wr(RCOMP_CFG0, 0x3<<8|1<<12));
    
    do { /* Wait for calibration to finish */
        L26_RD(VTSS_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS, &rcomp_val);
    } while(rcomp_val & VTSS_F_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS_BUSY);

    L26_RD(VTSS_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS, &rcomp_val);
    rcomp_val = VTSS_X_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS_RCOMP(rcomp_val);

    /* 1. Configure macro, apply reset */
    L26_WRM(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG,
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_FX100_ENA : 0) |
            (ib_ena_dc_coupling ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DC_COUPLING : 0) |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_CMV_TERM |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DETLEV |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_DET_LEV(3) |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_OFFSET_COMP |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_RESISTOR_CTRL(rcomp_val-3) |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_EQ_GAIN(3),
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_FX100_ENA |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DC_COUPLING |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_CMV_TERM |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DETLEV |
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_DET_LEV |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_OFFSET_COMP |
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_RESISTOR_CTRL |
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_EQ_GAIN);
    

    L26_WRM(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG,
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_CPMD_SEL(2) : 0) |
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_MBTR_CTRL(3) : 0) |
            (if_100fx ? 0 : VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_BW_ANA(6)),
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_CPMD_SEL |
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_MBTR_CTRL |
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_BW_ANA);
    
    resist_val = (vtss_state->init_conf.serdes.serdes1g_vdd == VTSS_VDD_1V0) ? rcomp_val+1 : rcomp_val-4;
    L26_WRM(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG,
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_RESISTOR_CTRL(resist_val) |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_AMP_CTRL(ob_amp_ctrl),
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_RESISTOR_CTRL |
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_AMP_CTRL);
    
    /* Write masked to avoid changing RECO_SEL_* fields used by SyncE */
    L26_WRM(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG,
            (ena_lane ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_ENA_LANE : 0) | VTSS_BIT(0),            
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SYS_RST |
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_ENA_LANE |
           (rev == 0 ? VTSS_BIT(7) : 0) | /* Rev 0 -> HRATE = 0.  Rev 1 -> HRATE = 1 (default) */
            VTSS_BIT(0)); /* IFMODE = 1 */

    L26_WRM(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG,
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_ENA |
            (rev == 0 ? VTSS_BIT(21) : 0) | /* Rev 0 -> RC_DIV2 = 1. Rev 1 -> RC_DIV2 = 0 (default) */
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_CTRL_DATA(200),
            VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_ENA |
            VTSS_BIT(21) |
            VTSS_M_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_CTRL_DATA);   

    L26_WRM(VTSS_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG,
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_DES_100FX_CPMD_ENA : 0) |
            #if (defined(BOARD_VNS_12_REF) || defined(BOARD_VNS_16_REF))
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_RX_DATA_INV_ENA |
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_TX_DATA_INV_ENA |
            #endif
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_LANE_RST,
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_DES_100FX_CPMD_ENA |
            #if (defined(BOARD_VNS_12_REF) || defined(BOARD_VNS_16_REF))
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_RX_DATA_INV_ENA |
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_TX_DATA_INV_ENA |
            #endif
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_LANE_RST);
           
    VTSS_RC(l26_sd1g_write(addr, L26_SERDES_WAIT));

    /* 2. Release PLL reset */
    L26_WRM_SET(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG,
                VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SYS_RST);
    VTSS_RC(l26_sd1g_write(addr, L26_SERDES_WAIT));

    /* 3. Release digital reset */
    L26_WRM(VTSS_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG, 0,
            VTSS_F_MACRO_CTRL_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_LANE_RST);
    VTSS_RC(l26_sd1g_write(addr, 0));

    return VTSS_RC_OK;
}


/* Serdes6G: Read/write data */
static vtss_rc l26_sd6g_read_write(u32 addr, BOOL write, u32 nsec)
{
    u32 data, mask;

    if (write)
        mask = VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_WR_ONE_SHOT;
    else
        mask = VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_RD_ONE_SHOT;

    L26_WR(VTSS_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG,
           VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_ADDR(addr) | mask);
    
    do { /* Wait until operation is completed  */
        L26_RD(VTSS_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG, &data);
    } while (data & mask);

    if (nsec)
        VTSS_NSLEEP(nsec);
    
    return VTSS_RC_OK;
}

/* Serdes6G: Read data */
static vtss_rc l26_sd6g_read(u32 addr) 
{
    return l26_sd6g_read_write(addr, 0, 0);
}

/* Serdes6G: Write data */
static vtss_rc l26_sd6g_write(u32 addr, u32 nsec) 
{
    return l26_sd6g_read_write(addr, 1, nsec);
}

/* Serdes6G setup (Disable/2G5/QSGMII/SGMII) */
static vtss_rc l26_sd6g_cfg(vtss_serdes_mode_t mode, u32 addr)
{
    u32 ib_rf, ctrl_data, if_mode=1, ob_ena_cas, ob_lev, ib_vbac=5, ib_vbcom=4, rcomp_val, ob_post_0=0, ib_ic_ac=0, ib_c=15, ib_chf=0;
    u32 ob_sr = 7;
    BOOL ena_lane=1, ena_rot=0, qrate, hrate, ob_ena1v, if_100fx=0, ib_cterm_ena;


    VTSS_D("addr: 0x%x, mode: %s", addr, l26_serdes_mode_txt(mode));
    ob_ena1v = (vtss_state->init_conf.serdes.serdes6g_vdd == VTSS_VDD_1V0) ? 1 : 0;
    ib_cterm_ena = (vtss_state->init_conf.serdes.ib_cterm_ena);

    switch (mode) {
    case VTSS_SERDES_MODE_2G5:
        /* Seredes6g_ob_cfg  */
        ob_post_0 = 2;
        /* Seredes6g_ob_cfg1 */
        ob_ena_cas = 1;
        ob_lev = ob_ena1v ? 48 : 63;
        /* Seredes6g_des_cfg --> see code */        
        /* Seredes6g_ib_cfg */        
        ib_ic_ac = ob_ena1v ? 2 : 0;
        ib_vbac  = ob_ena1v ? 4 : 5;
        ib_rf    = ob_ena1v ? 2 : 10;
        ib_vbcom = ob_ena1v ? 4 : 5;
        /* Seredes6g_ib_cfg1 */        
        ib_c = ob_ena1v ? 6 : 10;
        ib_chf = ob_ena1v ? 1 : 0;
        /* Seredes6g_pll_cfg */        
        ena_rot = 1;
        ctrl_data = 48;
        /* Seredes6g_common_cfg */        
        qrate = 0;
        hrate = 1;
        break;
    case VTSS_SERDES_MODE_QSGMII: 
        /* Seredes6g_ob_cfg  */
        ob_sr = vtss_state->init_conf.serdes.qsgmii.ob_sr;
        ob_post_0 = vtss_state->init_conf.serdes.qsgmii.ob_post0;
        /* Seredes6g_ob_cfg1 */
        ob_ena_cas = 1;
        ob_lev = 24;
        /* Seredes6g_ib_cfg */        
        ib_rf = 4;
        /* Seredes6g_ib_cfg1 */        
        ib_c = 4;
        /* Seredes6g_pll_cfg */        
        /* Seredes6g_pll_cfg */ 
        ctrl_data = 120;
        if_mode = 3;
        qrate = 0;
        hrate = 0;
        break;
    case VTSS_SERDES_MODE_SGMII:
        ob_lev = 48;
        ob_ena_cas = 2;
        ib_rf = 15;
        ctrl_data = 60;
        qrate = 1;
        hrate = 0;
        break;
    case VTSS_SERDES_MODE_100FX:
        ob_lev = 48;
        ob_ena_cas = 1;
        ib_rf = 15;
        ctrl_data = 60;
        qrate = 1;
        hrate = 0;
        if_100fx = 1;
        break;
    case VTSS_SERDES_MODE_1000BaseX:
        ob_lev = 48;
        ob_ena_cas = 2;
        ib_rf = 15;
        ctrl_data = 60;
        qrate = 1;
        hrate = 0;
        break;
    case VTSS_SERDES_MODE_DISABLE:
        ob_lev = 0;
        ob_ena_cas = 0;
        ib_rf = 0;
        ib_vbcom = 0;
        ena_rot = 0;
        ctrl_data = 0;
        qrate = 0;
        hrate = 0;
        break;
    default:
        VTSS_E("Serdes6g mode %s not supported", l26_serdes_mode_txt(mode));
        return VTSS_RC_ERROR;
    }
    /* RCOMP_CFG0.MODE_SEL = 2 */
    VTSS_RC(l26_wr(RCOMP_CFG0,0x3<<8));
    
    /* RCOMP_CFG0.RUN_CAL = 1 */
    VTSS_RC(l26_wr(RCOMP_CFG0, 0x3<<8|1<<12));
    
    do { /* Wait for calibration to finish */
        L26_RD(VTSS_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS, &rcomp_val);
    } while(rcomp_val & VTSS_F_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS_BUSY);

    L26_RD(VTSS_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS, &rcomp_val);
    rcomp_val = VTSS_X_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS_RCOMP(rcomp_val);

    /* 1. Configure macro, apply reset */
    /* OB_CFG  */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POL |
            VTSS_ENCODE_BITFIELD(rcomp_val+1,4,4) | /* RCOMP: bit 4-7 */
            VTSS_ENCODE_BITFIELD(ob_sr,0,4) |       /* SR:    bit 0-3 */
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POST0(ob_post_0) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_SR_H |
            (ob_ena1v ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_ENA1V_MODE : 0),
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POL |
            VTSS_ENCODE_BITMASK(4,4) | /* RCOMP: bit 4-7 */
            VTSS_ENCODE_BITMASK(0,4) | /* SR:    bit 0-3 */
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POST0 |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_SR_H |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_ENA1V_MODE);

    /* OB_CFG1 */      
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_ENA_CAS(ob_ena_cas) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_LEV(ob_lev),
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_ENA_CAS |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_LEV);

     /* IB_CFG */      
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG,
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
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1,
            (ib_cterm_ena ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_CTERM_ENA : 0) |
            (ib_chf ? VTSS_BIT(7) : 0 ) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_C(ib_c) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_DIS_EQ |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSAC |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSDC |
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA : 0) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST,
            VTSS_BIT(7) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_CTERM_ENA |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_C |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_DIS_EQ |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSAC |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSDC |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST);

    /* DES_CFG */      
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_PHS_CTRL(6) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_MBTR_CTRL(2) |
           (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL(2) : 0) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_HYST(5) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_ANA(5),
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_PHS_CTRL |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_MBTR_CTRL |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_HYST |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_ANA);

    /* PLL_CFG */      
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_CTRL_DATA(ctrl_data) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_ENA |
            (ena_rot ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ENA_ROT : 0),
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_CTRL_DATA |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_ENA |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ENA_ROT);

    /* Write masked to avoid changing RECO_SEL_* fields used by SyncE */
    /* COMMON_CFG */      
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG,
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
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG,
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA : 0),
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA);

    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG,
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA : 0) |
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST,
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA |
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST);

    VTSS_RC(l26_sd6g_write(addr, L26_SERDES_WAIT));

    /* 2. Release PLL reset */
    L26_WRM_SET(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, 
                VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SYS_RST);
    VTSS_RC(l26_sd6g_write(addr, L26_SERDES_WAIT));

    /* 3. Release digital reset */
    L26_WRM_CLR(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1,
                VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST);

    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, 0,
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST);
    VTSS_RC(l26_sd6g_write(addr, 0));

    return VTSS_RC_OK;
}


/* Configures the Serdes1G/Serdes6G blocks based on mux mode and Target */
static vtss_rc l26_serdes_macro_config(void) 
{
    vtss_port_mux_mode_t mux_mode = vtss_state->init_conf.mux_mode;
    u32                  sw_mode;

    /* Setup mux mode */
    switch (mux_mode) {
    case VTSS_PORT_MUX_MODE_0:
        sw_mode = 0;
        break;
    case VTSS_PORT_MUX_MODE_1:
        sw_mode = 1;
        break;
    case VTSS_PORT_MUX_MODE_2:
        sw_mode = 2;

        /* In mode 2, ports 10 and 11 are connected to Serdes1G Macro */
        L26_WR(VTSS_DEV_DEV_CFG_STATUS_DEV_IF_CFG(VTSS_TO_DEV(10)), 1);
        L26_WR(VTSS_DEV_DEV_CFG_STATUS_DEV_IF_CFG(VTSS_TO_DEV(11)), 1);
        break;
    default:
        VTSS_E("port mux mode not supported");
        return VTSS_RC_ERROR;
    }

    L26_WRM(VTSS_DEVCPU_GCB_MISC_MISC_CFG, 
            VTSS_F_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE(sw_mode), 
            VTSS_M_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE);

    /* Setup macros */
    switch (vtss_state->create.target) {
    case VTSS_TARGET_SPARX_III_10:
    case VTSS_TARGET_SPARX_III_10_01:
        if (mux_mode != VTSS_PORT_MUX_MODE_2) {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }
        
        VTSS_RC(l26_sd1g_cfg(VTSS_SERDES_MODE_SGMII, 0x3));       /* SGMII mode, Serdes1g (1-0) */
        break;
    case VTSS_TARGET_SPARX_III_18: 
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0x2));  /* QSGMII mode, Serdes6g (1) */
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x1));   /* SGMII mode, Serdes6g (0) */
            VTSS_RC(l26_sd1g_cfg(VTSS_SERDES_MODE_SGMII, 0x1));   /* SGMII mode, Serdes1g (0) */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_2) {
            VTSS_RC(l26_sd1g_cfg(VTSS_SERDES_MODE_SGMII, 0x3F));  /* Enable SGMII, Serdes1g (5-0)   */
        } else {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }    
        break;
    case VTSS_TARGET_SPARX_III_24: 
        if (mux_mode != VTSS_PORT_MUX_MODE_0) {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }
        VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0xE));     /* Enable QSGMII, Serdes6g (3-1) */
        break;
    case VTSS_TARGET_SPARX_III_26: 
        if (mux_mode != VTSS_PORT_MUX_MODE_0) {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }
        VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0xE));     /* Enable QSGMII, Serdes6g (3-1) */
        VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x1));      /* Enable SGMII, Serdes6g (0) */
        VTSS_RC(l26_sd1g_cfg(VTSS_SERDES_MODE_SGMII, 0x1));      /* Enable SGMII, Serdes1g (0) */
        break;
    case VTSS_TARGET_CARACAL_1: 
        if (mux_mode != VTSS_PORT_MUX_MODE_1) {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }
        VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_2G5, 0x3));        /* Enable 2.5G, Serdes6g (1-0) */
        VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x4));      /* Enable SGMII, Serdes6g (2) */
        VTSS_RC(l26_sd1g_cfg(VTSS_SERDES_MODE_SGMII, 0xFF));     /* Enable SGMII, Serdes1g (7-0) */
        break;
    case VTSS_TARGET_CARACAL_2: 
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_2G5, 0x1));    /* Enable 2.5G, Serdes6g (0) */
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0xE)); /* Enable QSGMII, Serdes6g (3-1) */
            VTSS_RC(l26_sd1g_cfg(VTSS_SERDES_MODE_SGMII, 1));    /* Enable SGMII, Serdes1g (0) */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) { 
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_2G5, 0x3));    /* Enable 2.5G, Serdes6g (1-0) */
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0xC));  /* Enable SGMII, Serdes6g (3-2) */
            VTSS_RC(l26_sd1g_cfg(VTSS_SERDES_MODE_SGMII, 0xFF)); /* Enable SGMII, Serdes1g (7-0) */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_2) {
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0xC)); /* Enable QSGMII, Serdes6g (3-2) */
            VTSS_RC(l26_sd1g_cfg(VTSS_SERDES_MODE_SGMII, 0xFF)); /* Enable SGMII, Serdes1g (7-0) */
        } else {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }
        break;
    case VTSS_TARGET_SPARX_III_10_UM: 
        if (mux_mode == VTSS_PORT_MUX_MODE_1) {
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x3));  /* Enable SGMII, Serdes6g (1-0) */
        } else {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }
        break;
    case VTSS_TARGET_SPARX_III_17_UM: 
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x1));  /* Enable SGMII, Serdes6g (0) */
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0x8)); /* Enable QSGMII, Serdes6g (3) */
        } else if (mux_mode == VTSS_PORT_MUX_MODE_1) { 
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0xF));  /* Enable SGMII, Serdes6g (3-0) */
        } else {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }
        break;
    case VTSS_TARGET_SPARX_III_25_UM: 
        if (mux_mode == VTSS_PORT_MUX_MODE_0) {
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0xE)); /* Enable QSGMII, Serdes6g (3-1) */
            VTSS_RC(l26_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x1));  /* Enable SGMII, Serdes6g (0) */
        } else {
            VTSS_E("Mux mode not supported for this Target");
            return VTSS_RC_ERROR;
        }
        break;
    default:
        VTSS_E("Unknown Target");
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}


#if defined(VTSS_FEATURE_SYNCE)
/* ================================================================= *
 *  SYNCE (Level 1 syncronization)
 * ================================================================= */

typedef enum
{
    SERDES_1G,
    SERDES_6G,
} serdes_t;

static vtss_rc synce_chip_port_to_serdes(vtss_target_type_t target,   vtss_port_mux_mode_t mux_mode,   i32 chip_port,   serdes_t *type,   u32 *instance)
{ /* This function is able to calculate the SERDES type and instance of all chip ports that can possible be a synce source */
    switch (target) {
        case VTSS_TARGET_SPARX_III_10:     /* VSC7424 */
        case VTSS_TARGET_SPARX_III_10_01:  /* VSC7424-01 */
            if (mux_mode != VTSS_PORT_MUX_MODE_2)   return VTSS_RC_ERROR;

            *type = SERDES_1G;
            if (chip_port == 24)     *instance = 0;
            else
            if (chip_port == 25)     *instance = 1;
            else                     return VTSS_RC_ERROR;
            break;
        case VTSS_TARGET_SPARX_III_18:  /* VSC7425 */ 
            if (mux_mode == VTSS_PORT_MUX_MODE_0) {
                *instance = 0;
                if (chip_port == 24)  *type = SERDES_1G;
                else
                if (chip_port == 25)  *type = SERDES_6G;
                else                  return VTSS_RC_ERROR;
            } else
            if (mux_mode == VTSS_PORT_MUX_MODE_2) {
                *type = SERDES_1G;
                switch (chip_port) {
                    case 20: case 21: case 22: case 23: *instance = 5 - (chip_port-20); break;
                    case 24: case 25: *instance = chip_port-24; break;
                    default:    return VTSS_RC_ERROR;
                }
            }    
            else   return VTSS_RC_ERROR;
            break;
        case VTSS_TARGET_SPARX_III_24:  /* VSC7426 */
            return VTSS_RC_ERROR;
        case VTSS_TARGET_SPARX_III_26:  /* VSC7427 */
            if (mux_mode != VTSS_PORT_MUX_MODE_0)      return VTSS_RC_ERROR;
            *instance = 0;
            if (chip_port == 24)  *type = SERDES_1G;
            else
            if (chip_port == 25)  *type = SERDES_6G;
            else                  return VTSS_RC_ERROR;
            break;
        case VTSS_TARGET_CARACAL_1:  /* VSC7428 */
            if (mux_mode != VTSS_PORT_MUX_MODE_1)      return VTSS_RC_ERROR;
            switch (chip_port) {
                case 14: case 15: *type = SERDES_1G; *instance = 7 - (chip_port-14); break;
                case 17: case 18: *type = SERDES_1G; *instance = 5 - (chip_port-17); break;
                case 19: *type = SERDES_6G; *instance = 2; break;
                case 20: case 21: case 22: case 23: *type = SERDES_1G; *instance = 3 - (chip_port-20); break;
                case 24: case 25: *type = SERDES_6G; *instance = 1 - (chip_port-24); break;
                default:    return VTSS_RC_ERROR;
            }
            break;
        case VTSS_TARGET_CARACAL_2:  /* VSC7429 */
            if (mux_mode == VTSS_PORT_MUX_MODE_0) {
                *instance = 0;
                if (chip_port == 24)  *type = SERDES_1G;
                else
                if (chip_port == 25)  *type = SERDES_6G;
                else                  return VTSS_RC_ERROR;
            } else
            if (mux_mode == VTSS_PORT_MUX_MODE_1) { 
                switch (chip_port) {
                    case 14: case 15: *type = SERDES_1G; *instance = 7 - (chip_port-14); break;
                    case 16: *type = SERDES_6G; *instance = 3; break;
                    case 17: case 18: *type = SERDES_1G; *instance = 5 - (chip_port-17); break;
                    case 19: *type = SERDES_6G; *instance = 2; break;
                    case 20: case 21: case 22: case 23: *type = SERDES_1G; *instance = 3 - (chip_port-20); break;
                    case 24: case 25: *type = SERDES_6G; *instance = 1 - (chip_port-24); break;
                    default:    return VTSS_RC_ERROR;
                }
            } else
            if (mux_mode == VTSS_PORT_MUX_MODE_2) {
                *type = SERDES_1G;
                switch (chip_port) {
                    case 10: case 11: *instance = 7 - (chip_port-10); break;
                    case 20: case 21: case 22: case 23: *instance = 5 - (chip_port-20); break;
                    case 24: case 25: *instance = chip_port-24; break;
                    default:    return VTSS_RC_ERROR;
                }
            } else   return VTSS_RC_ERROR;
            break;
        default:
            return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_synce_clock_out_set(const u32   clk_port)
{
    u32 eth_cfg, div_mask, en_mask;

    if ((vtss_state->create.target != VTSS_TARGET_CARACAL_1) && (vtss_state->create.target != VTSS_TARGET_CARACAL_2) && (vtss_state->create.target != VTSS_TARGET_SPARX_III_26))   return VTSS_RC_ERROR;

    div_mask = (clk_port ? VTSS_M_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_B : VTSS_M_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_A);
    en_mask = (clk_port ? VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_RECO_CLK_B_ENA : VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_RECO_CLK_A_ENA);

    L26_RD(VTSS_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG, &eth_cfg);
    eth_cfg &= ~(div_mask | en_mask);      /* clear the related bits for this configuration */
    switch (vtss_state->synce_out_conf[clk_port].divider) {
        case VTSS_SYNCE_DIVIDER_1: div_mask = (clk_port ? VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_B(0) : VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_A(0)); break;
        case VTSS_SYNCE_DIVIDER_4: div_mask = (clk_port ? VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_B(2) : VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_A(2)); break;
        case VTSS_SYNCE_DIVIDER_5: div_mask = (clk_port ? VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_B(1) : VTSS_F_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG_SEL_RECO_CLK_A(1)); break;
    }
    eth_cfg = eth_cfg | div_mask | (vtss_state->synce_out_conf[clk_port].enable ? en_mask : 0);
//printf("l26_synce_clock_out_set  eth_cfg %X\n", eth_cfg);
    L26_WR(VTSS_MACRO_CTRL_SYNC_ETH_CFG_SYNC_ETH_CFG,  eth_cfg);  /* Set the related bits defending on configuration */

    return VTSS_RC_OK;
}

static vtss_rc l26_synce_clock_in_set(const u32   clk_port)
{
    serdes_t    serdes_type;
    u32         serdes_instance, common_cfg, sq_mask, en_mask;
    i32         new_chip_port = vtss_state->port_map[vtss_state->synce_in_conf[clk_port].port_no].chip_port;
    i32         old_chip_port = vtss_state->old_port_no[clk_port];

    if ((vtss_state->create.target != VTSS_TARGET_CARACAL_1) && (vtss_state->create.target != VTSS_TARGET_CARACAL_2) && (vtss_state->create.target != VTSS_TARGET_SPARX_III_26))   return VTSS_RC_ERROR;

    if (!vtss_state->synce_in_conf[clk_port].enable || (new_chip_port != old_chip_port)) {
    /* Disable of this clock port or input port has changed for this clock output port - disable old input */
        if (VTSS_RC_OK == synce_chip_port_to_serdes(vtss_state->create.target,   vtss_state->init_conf.mux_mode,   old_chip_port,   &serdes_type,   &serdes_instance)) {
            if (serdes_type == SERDES_1G) {
                VTSS_RC(l26_sd1g_read(1<<serdes_instance));                /* Readback the 1G common config register */
                L26_RD(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, &common_cfg);
                common_cfg &= ~(clk_port ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_RECO_SEL_B : VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_RECO_SEL_A);
//printf("l26_synce_clock_in_set disable 1G   instance %u  common_cfg %X\n", serdes_instance, common_cfg);
                L26_WR(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, common_cfg);   /* Clear the recovered clock enable */
                VTSS_RC(l26_sd1g_write(1<<serdes_instance, 0));
            }
            if (serdes_type == SERDES_6G) {
                VTSS_RC(l26_sd6g_read(1<<serdes_instance));
                L26_RD(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, &common_cfg);
                common_cfg &= ~(clk_port ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_RECO_SEL_B : VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_RECO_SEL_A);
//printf("l26_synce_clock_in_set disable 6G   instance %u  common_cfg %X\n", serdes_instance, common_cfg);
                L26_WR(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, common_cfg);   /* Clear the recovered clock enable */
                VTSS_RC(l26_sd6g_write(1<<serdes_instance, 0));
            }
        }
    }

    if (vtss_state->synce_in_conf[clk_port].enable) {
    /* Enable input clock configuration - now configuring the new (or maybe the same) input port */
        if (VTSS_RC_OK == synce_chip_port_to_serdes(vtss_state->create.target,   vtss_state->init_conf.mux_mode,   new_chip_port,   &serdes_type,   &serdes_instance)) {
            if (serdes_type == SERDES_1G) {
                sq_mask = (clk_port ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SE_AUTO_SQUELCH_B_ENA : VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SE_AUTO_SQUELCH_A_ENA);
                en_mask = (clk_port ? VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_RECO_SEL_B : VTSS_F_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_RECO_SEL_A);
                VTSS_RC(l26_sd1g_read(1<<serdes_instance));                /* Readback the 1G common config register */
                L26_RD(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, &common_cfg);
                common_cfg &= ~(sq_mask | en_mask);      /* clear the related bits for this configuration */
                common_cfg |= (vtss_state->synce_in_conf[clk_port].squelsh ? sq_mask : 0) | (vtss_state->synce_in_conf[clk_port].enable ? en_mask : 0);
//printf("l26_synce_clock_in_set enable 1G   instance %u  common_cfg %X\n", serdes_instance, common_cfg);
                L26_WR(VTSS_MACRO_CTRL_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, common_cfg);   /* Set the related bits depending on configuration */
                VTSS_RC(l26_sd1g_write(1<<serdes_instance, 0));     /* transfer 1G common config register */
            }
            if (serdes_type == SERDES_6G) {
                sq_mask = (clk_port ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SE_AUTO_SQUELCH_B_ENA : VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SE_AUTO_SQUELCH_A_ENA);
                en_mask = (clk_port ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_RECO_SEL_B : VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_RECO_SEL_A);
                VTSS_RC(l26_sd6g_read(1<<serdes_instance));                /* Readback the 1G common config register */
                L26_RD(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, &common_cfg);
                common_cfg &= ~(sq_mask | en_mask);      /* clear the related bits for this configuration */
                common_cfg |= (vtss_state->synce_in_conf[clk_port].squelsh ? sq_mask : 0) | (vtss_state->synce_in_conf[clk_port].enable ? en_mask : 0);
//printf("l26_synce_clock_in_set enable 6G   instance %u  common_cfg %X\n", serdes_instance, common_cfg);
                L26_WR(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, common_cfg);   /* Set the related bits depending on configuration */
                VTSS_RC(l26_sd6g_write(1<<serdes_instance, 0));     /* transfer 1G common config register */
            }
            vtss_state->old_port_no[clk_port] = new_chip_port;
        }
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_SYNCE */


#if defined(VTSS_FEATURE_EEE)
/* =================================================================
 *  EEE - Energy Efficient Ethernet
 * =================================================================*/

/*
 * Set EEE configuration
 *
 * In :  port_no - The port number for which to set new configuration.
 *       conf    - New configuration for the port
 *
 * Return : VTSS_RC_OK if configuration done else error code.
 */
static vtss_rc l26_eee_port_conf_set(const vtss_port_no_t              port_no,
                                     const vtss_eee_port_conf_t *const conf)
{
    /*lint -esym(459, eee_timer_table, eee_timer_table_initialized) */
    static u32     eee_timer_table[128];
    static BOOL    eee_timer_table_initialized = FALSE;
    u32            closest_match_index, closest_match, i, requested_time;
    u32            eee_cfg_reg = 0x0; // SYS::EEE_CFG register value.
    vtss_port_no_t chip_port = VTSS_CHIP_PORT(port_no);
    u16            eee_timer_age;

    // The formula for the actual wake-up time given a
    // register value is non-linear (though periodic with
    // increasing base values).
    // The easiest way to find the closest match to a user-specified
    // value is to create a static lookup table that will have to be
    // traversed everytime.
    if (!eee_timer_table_initialized) {
        eee_timer_table_initialized = TRUE;
        for (i = 0; i < 128; i++) {
            eee_timer_table[i] = (1 << (2 * (i / 16UL))) * (i % 16UL);
        }
    }

    if ((vtss_state->chip_id.revision == VTSS_PHY_ATOM_REV_B) && (chip_port < 12) && (chip_port < VTSS_PORT_ARRAY_SIZE) && (vtss_state->eee_ena[port_no] != conf->eee_ena)) {
        vtss_state->eee_ena[port_no] = conf->eee_ena; // On RevB the Fast Link Fail signal interrupt from internal PHY (12 ports) must be disabled when EEE is enabled
        VTSS_N("conf->eee_ena:%d", conf->eee_ena);
        VTSS_RC(l26_intr_cfg((0x01 << chip_port), 0, !conf->eee_ena));
    }


    // Make sure that we don't get out of bound
    if (port_no >= VTSS_PORT_ARRAY_SIZE) {
        VTSS_E("Out of bounds: Port:%u, VTSS_PORT_ARRAY_SIZE:%d", port_no, VTSS_PORT_ARRAY_SIZE);
        return VTSS_RC_ERROR;
    }

    // EEE enable
    if (conf->eee_ena) {
        //LPI is really an old error code redefined to tell the link partner to go to
        // lower power or when removed to wakeup.
        // Some devices are seeing the error code and assuming there is a
        // problem causing the link to go down. A work around is to only enable EEE in the MAC (No LPI signal to the PHY)
        // when the PHY has auto negotiated and have found that the link partner supports EEE.
        if (conf->lp_advertisement == 0) {
            VTSS_D("Link partner doesn't support EEE - Keeping EEE disabled. Port:%u", chip_port);
        } else if (!(vtss_state->phy_state[port_no].status.fdx)) {
            // EEE and Half duplex are not supposed to work together, so we disables EEE in the case where the port is in HDX mode.
            VTSS_D("EEE disabled due to that port is in HDX mode, port:%u, fdx:%u", chip_port, vtss_state->phy_state[port_no].status.fdx);

        } else if ((vtss_state->phy_state[port_no].status.aneg.obey_pause || vtss_state->phy_state[port_no].status.aneg.generate_pause) &&
                   (vtss_state->chip_id.revision == 0 ||  vtss_state->chip_id.revision == 1)) {
            // For chip revision A and B Flow control and EEE doesn't work together.
            VTSS_D("EEE disabled due to bugzilla#4235, port:%d", chip_port);
        } else {
            eee_cfg_reg |= VTSS_F_SYS_SYSTEM_EEE_CFG_EEE_ENA;
        }
    }

    // EEE wakeup timer (See datasheet for understanding calculation)
    closest_match_index = 0;
    closest_match       = 0xFFFFFFFFUL;
    requested_time      = conf->tx_tw;
    for (i = 0; i< 128; i++) {
        u32 table_val = eee_timer_table[i];
        if (table_val >= requested_time) {
            u32 diff = table_val - requested_time;
            if (diff < closest_match) {
                closest_match       = diff;
                closest_match_index = i;
                if (closest_match == 0) {
                    break;
                }
            }
        }
    }

    if (closest_match == 0xFFFFFFFFUL) {
        closest_match_index = 127;
    }

    eee_cfg_reg |= VTSS_F_SYS_SYSTEM_EEE_CFG_EEE_TIMER_WAKEUP(closest_match_index);

    
    // Set the latency depending upon what the user prefer (power saving vs. low traffic latency)
    if (conf->optimized_for_power) {
        eee_timer_age = 82; // Set timer age to 263 mSec.
    } else {
        eee_timer_age = 0x23; // Set timer age to 48 uSec.
    }

    // EEE holdoff timer
    eee_cfg_reg |= VTSS_F_SYS_SYSTEM_EEE_CFG_EEE_TIMER_HOLDOFF(0x5) | VTSS_F_SYS_SYSTEM_EEE_CFG_EEE_TIMER_AGE(eee_timer_age);

    // EEE fast queues
    eee_cfg_reg |= VTSS_F_SYS_SYSTEM_EEE_CFG_EEE_FAST_QUEUES(conf->eee_fast_queues);

    // Registers write
    L26_WR(VTSS_SYS_SYSTEM_EEE_CFG(chip_port), eee_cfg_reg);

    VTSS_I("chip_port:%u, eee_cfg_reg = 0x%X, conf->tx_tw = %d, eee_timer_age:%d", chip_port, eee_cfg_reg, conf->tx_tw, eee_timer_age);

    // Setting Buffer size to 12.2 Kbyte & 255 frames.
    L26_WR(VTSS_SYS_SYSTEM_EEE_THRES, 0xFFFF);

    return VTSS_RC_OK;
}
#endif

/* =================================================================
 * FAN speed control
* =================================================================*/
#if defined(VTSS_FEATURE_FAN) 

/*
 * Initialise FAN controller 
 *
 * In :  spec  - Fan specifications
 *
 */
static vtss_rc l26_fan_controller_init(const vtss_fan_conf_t * const spec)
{
    // Set GPIO alternate functions. PWM is bit 29.
    (void) l26_gpio_mode(0, 29, VTSS_GPIO_ALT_0);

    // Set PWM frequency 
    L26_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
           VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_PWM_FREQ(spec->fan_pwm_freq),
           VTSS_M_DEVCPU_GCB_FAN_CFG_FAN_CFG_PWM_FREQ);

    // Set PWM polarity 
    L26_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
           spec->fan_low_pol ? VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_INV_POL : 0,
           VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_INV_POL);

    // Set PWM open collector 
    L26_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
           spec->fan_open_col ? VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_INV_POL : 0,
           VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_PWM_OPEN_COL_ENA);
           
    // Set fan speed measurement
    if (spec->type == VTSS_FAN_3_WIRE_TYPE) {
        // Enable gating for 3-wire fan types.
        L26_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
               1,
               VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_GATE_ENA);
    } else {
        //  For 4-wire fan types we need to disable gating (2-wire types doesn't matter)
        L26_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
                0,
                VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_GATE_ENA);
    }


    // Set GPIO alternate functions. ROTA is bit 4.
    (void) l26_gpio_mode(0, 4, VTSS_GPIO_ALT_0);
 
    return VTSS_RC_OK;
}




/*
 * Set FAN cooling level
 *
 * In :  lvl - New fan cooling level, 0 = Fan is OFF, 0xFF = Fan constant ON.
 *
 * return : VTSS_RC_OK when configuration done else error code.
 */

static vtss_rc l26_fan_cool_lvl_set(u8 lvl)
{
    // Set PWM duty cycle (fan speed) 
    L26_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
           VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_DUTY_CYCLE(lvl),
           VTSS_M_DEVCPU_GCB_FAN_CFG_FAN_CFG_DUTY_CYCLE);

    return VTSS_RC_OK;
}

/*
 * Get FAN cooling level
 *
 * Return : Fan cooling level, 0 = Fan is OFF, 0xFF = Fan constant ON.
 *
 */

static vtss_rc l26_fan_cool_lvl_get(u8 *duty_cycle)
{
    u32 fan_cfg_reg;

    // Read the register 
    L26_RD(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,&fan_cfg_reg);

    // Get PWM duty cycle
    *duty_cycle = VTSS_X_DEVCPU_GCB_FAN_CFG_FAN_CFG_DUTY_CYCLE(fan_cfg_reg);
    
    return VTSS_RC_OK;
}


/*
 * Get FAN rotation count. 
 *
 * In : fan_spec - Pointer to fan specification, e.g. type of fan.
 * 
 * In/Out - rotation_count - Pointer to where to put the rotation count.
 *
 * Return : VTSS_OK if rotation was found else error code.
 */
static vtss_rc l26_fan_rotation_get(vtss_fan_conf_t *fan_spec, u32 *rotation_count)
{
    // Read the register 
    L26_RD(VTSS_DEVCPU_GCB_FAN_STAT_FAN_CNT,rotation_count);
   
     
    return VTSS_RC_OK;
}

#endif
/* ================================================================= *
 *  Port control
 * ================================================================= */

static vtss_rc l26_miim_read_write(BOOL read, 
                                   u32 miim_controller, 
                                   u8 miim_addr, 
                                   u8 addr, 
                                   u16 *value, 
                                   BOOL report_errors)
{
    u32 data;

    /* Command part */
    data = (read ? VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_OPR_FIELD(2) : /* Read op */
            VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_OPR_FIELD(1) | /* Write op */
            VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_WRDATA(*value)); /* value */

    /* Addressing part */
    data |= 
        VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_VLD | /* Valid command */
        VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_REGAD(addr) | /* Register address */
        VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_PHYAD(miim_addr); /* Phy/port address */
    
    /* Enqueue MIIM operation to be executed */
    L26_WR(VTSS_DEVCPU_GCB_MIIM_MII_CMD(miim_controller), data);

    /* Wait for MIIM operation to finish */
    do {
        L26_RD(VTSS_DEVCPU_GCB_MIIM_MII_STATUS(miim_controller), &data);
    } while(data & VTSS_F_DEVCPU_GCB_MIIM_MII_STATUS_MIIM_STAT_BUSY);
    
    if (read) {
        L26_RD(VTSS_DEVCPU_GCB_MIIM_MII_DATA(miim_controller), &data);
        if (data & VTSS_F_DEVCPU_GCB_MIIM_MII_DATA_MIIM_DATA_SUCCESS(3))
            return VTSS_RC_ERROR;
        *value = VTSS_X_DEVCPU_GCB_MIIM_MII_DATA_MIIM_DATA_RDDATA(data);
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_miim_read(vtss_miim_controller_t miim_controller, 
                             u8 miim_addr, 
                             u8 addr, 
                             u16 *value, 
                             BOOL report_errors)
{
    return l26_miim_read_write(TRUE, miim_controller, miim_addr, addr, value, report_errors);
}

static vtss_rc l26_miim_write(vtss_miim_controller_t miim_controller, 
                              u8 miim_addr, 
                              u8 addr, 
                              u16 value, 
                              BOOL report_errors)
{
    return l26_miim_read_write(FALSE, miim_controller, miim_addr, addr, &value, report_errors);
}

static vtss_rc l26_pgid_mask_write(u32 pgid, BOOL member[],
                                   BOOL cpu_copy, vtss_packet_rx_queue_t cpu_queue)
{
    u32 mask = l26_port_mask(member);
    u32 queue = 0;
    
    if (cpu_copy) {
        mask |= VTSS_BIT(VTSS_CHIP_PORT_CPU);
        queue = cpu_queue;
    }
    L26_WR(VTSS_ANA_ANA_TABLES_PGID(pgid),
           VTSS_F_ANA_ANA_TABLES_PGID_PGID(mask) |
           VTSS_F_ANA_ANA_TABLES_PGID_CPUQ_DST_PGID(queue));
    return VTSS_RC_OK;
}

/* Write PGID entry */
static vtss_rc l26_pgid_table_write(u32 pgid, BOOL member[])
{
    vtss_pgid_entry_t *pgid_entry = &vtss_state->pgid_table[pgid];

    return l26_pgid_mask_write(l26_chip_pgid(pgid), member, 
                               pgid_entry->cpu_copy, pgid_entry->cpu_queue);
}

/* Update PGID table for reserved entry */
static vtss_rc l26_pgid_update(u32 pgid, BOOL member[])
{
    vtss_port_no_t    port_no;
    vtss_pgid_entry_t *pgid_entry;
    
    pgid = l26_vtss_pgid(pgid);
    pgid_entry = &vtss_state->pgid_table[pgid];
    pgid_entry->resv = 1;
    pgid_entry->references = 1;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        pgid_entry->member[port_no] = member[port_no];
    
    return l26_pgid_table_write(pgid, member);
}

/* Write the aggregation masks  */
static vtss_rc l26_aggr_table_write(u32 ac, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    return l26_pgid_mask_write(PGID_AGGR + ac, member, 0, 0);
}

/* Write source mask */
static vtss_rc l26_src_table_write(vtss_port_no_t port_no, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    return l26_pgid_mask_write(PGID_SRC + VTSS_CHIP_PORT(port_no), member, 0, 0);
}

/* Write the aggregation mode  */
static vtss_rc l26_aggr_mode_set(void)
{
    vtss_aggr_mode_t *mode = &vtss_state->aggr_mode;

    L26_WR(VTSS_ANA_COMMON_AGGR_CFG,
           (mode->sip_dip_enable     ? VTSS_F_ANA_COMMON_AGGR_CFG_AC_IP4_SIPDIP_ENA : 0) |
           (mode->sport_dport_enable ? VTSS_F_ANA_COMMON_AGGR_CFG_AC_IP4_TCPUDP_ENA : 0) |
           (mode->dmac_enable        ? VTSS_F_ANA_COMMON_AGGR_CFG_AC_DMAC_ENA       : 0) |
           (mode->smac_enable        ? VTSS_F_ANA_COMMON_AGGR_CFG_AC_SMAC_ENA       : 0));
    return VTSS_RC_OK;
}

/* Write the common logical port of the aggr. group (lowest port no. of the group) */
static vtss_rc l26_pmap_table_write(vtss_port_no_t port_no, vtss_port_no_t l_port_no)
{
    L26_WRM(VTSS_ANA_PORT_PORT_CFG(VTSS_CHIP_PORT(port_no)), 
            VTSS_F_ANA_PORT_PORT_CFG_PORTID_VAL(VTSS_CHIP_PORT(l_port_no)), 
            VTSS_M_ANA_PORT_PORT_CFG_PORTID_VAL);

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  IP Multicast
 * ================================================================= */

/* Allocate FID for IPMC SSM */
static vtss_rc l26_ip_mc_fid_alloc(vtss_vid_t *fid, BOOL ipv6, BOOL add)
{
    u8                mask = (ipv6 ? IPMC_USED_IPV6 : IPMC_USED_IPV4);
    vtss_vid_t        vid, fid_free = VTSS_VID_NULL;
    vtss_vlan_entry_t *vlan_entry;

    /* Search for free VID from 4094 -> 2 */
    for (vid = (VTSS_VID_RESERVED - 1); vid > VTSS_VID_DEFAULT; vid--) {
        vlan_entry = &vtss_state->vlan_table[vid];
        if (vlan_entry->enabled || (vlan_entry->ipmc_used & mask) != 0)
            continue;
        
        if (vlan_entry->ipmc_used == IPMC_USED_NONE) {
            /* First completely free entry may be used */
            if (fid_free == VTSS_VID_NULL)
                fid_free = vid;
        } else {
            /* First entry used by the other IP version is preferred */
            fid_free = vid;
            break;
        }
    }

    if (fid_free != VTSS_VID_NULL) {
        /* Free FID was found */
        if (add)
            vtss_state->vlan_table[fid_free].ipmc_used |= mask;
        *fid = fid_free;
        return VTSS_RC_OK;
    }

    VTSS_I("no more FIDs");
    return VTSS_RC_ERROR;
}

/* Free FID for IPMC SSM */
static vtss_rc l26_ip_mc_fid_free(vtss_vid_t fid, BOOL ipv6)
{
    u8                mask = (ipv6 ? IPMC_USED_IPV6 : IPMC_USED_IPV4);
    vtss_vlan_entry_t *vlan_entry = &vtss_state->vlan_table[fid];

    if (vlan_entry->ipmc_used & mask) {
        vlan_entry->ipmc_used -= mask;
        return VTSS_RC_OK;
    }
    VTSS_E("FID %u already free for IPv%u", fid, ipv6 ? 6 : 4);
    return VTSS_RC_ERROR;
}

/* Convert DIP to MAC address */
static void l26_ip_mc_mac_get(vtss_vid_mac_t *vid_mac, vtss_ipmc_dst_data_t *dst, BOOL ipv6)
{
    u8 i, *mac_addr = &vid_mac->mac.addr[0];
    
    if (ipv6) {
        for (i = 0; i < 6; i++) {
            mac_addr[i] = (i < 2 ? 0x33 : dst->dip.ipv6.addr[10 + i]);
        }
    } else {
        mac_addr[0] = 0x01;
        mac_addr[1] = 0x00;
        mac_addr[2] = 0x5e;
        mac_addr[3] = ((dst->dip.ipv4 >> 16) & 0x7f);
        mac_addr[4] = ((dst->dip.ipv4 >> 8) & 0xff);
        mac_addr[5] = (dst->dip.ipv4 & 0xff);
    }
}

/* Calculate IS1 VCAP ID for source */
static vtss_vcap_id_t l26_ip_mc_vcap_id(vtss_ipmc_src_data_t *src, BOOL ipv6)
{
    vtss_vcap_id_t id = ipv6;
    
    id = ((id << 31) + src->vid);
    id = ((id << 32) + vtss_cmn_ip2u32(&src->sip, ipv6));
    return id;
}

/* Add IS1 entry for source */
static vtss_rc l26_ip_mc_is1_add(vtss_ipmc_src_data_t *src, BOOL ipv6)
{
    vtss_vcap_obj_t   *obj = &vtss_state->is1.obj;
    vtss_vcap_data_t  data;
    vtss_is1_entry_t  entry;
    vtss_is1_action_t *action = &entry.action;
    vtss_is1_key_t    *key = &entry.key;
    vtss_vcap_id_t    id = l26_ip_mc_vcap_id(src, ipv6);
    vtss_port_no_t    port_no;
    int               i;

    /* Initialize IS1 entry data */
    vtss_vcap_is1_init(&data, &entry);
    
    /* First Lookup */
    data.u.is1.lookup = 0;
    
    /* Action data */
    action->fid_sel = VTSS_FID_SEL_DMAC;
    action->fid_val = src->fid;
    
    /* Key data */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        key->port_list[port_no] = TRUE;
    }
    
    key->tag.vid.type = VTSS_VCAP_VR_TYPE_VALUE_MASK;
    key->tag.vid.vr.v.value = src->vid;
    key->tag.vid.vr.v.mask = 0xffff;
    
    if (ipv6) {
        key->type = VTSS_IS1_TYPE_IPV6;
        for (i = 0; i < 4; i++) {
            key->frame.ipv6.sip.value[i] = src->sip.ipv6.addr[12 + i];
            key->frame.ipv6.sip.mask[i] = 0xff;
        }
        key->frame.ipv6.ip_mc = VTSS_VCAP_BIT_1;
    } else {
        key->type = VTSS_IS1_TYPE_IPV4;
        key->frame.ipv4.sip.value = src->sip.ipv4;
        key->frame.ipv4.sip.mask = 0xffffffff;
        key->frame.ipv4.ip_mc = VTSS_VCAP_BIT_1;
    }
    
    /* Add IS1 entry */
    VTSS_RC(vtss_vcap_add(obj, VTSS_IS1_USER_SSM, id, VTSS_VCAP_ID_LAST, &data, 0));    
    
    return VTSS_RC_OK;
}

/* Update IP multicast entry */
static vtss_rc l26_ip_mc_update(vtss_ipmc_data_t *ipmc, vtss_ipmc_cmd_t cmd)
{
    vtss_res_t             res;
    vtss_ipmc_obj_t        *ipmc_obj = &vtss_state->ipmc.obj;
    vtss_vcap_obj_t        *is1_obj = &vtss_state->is1.obj;
    vtss_ipmc_src_t        *src, *src_asm = NULL;
    vtss_ipmc_dst_t        *dst, *dst_asm, *dst_asm_first = NULL, *dst_asm_last = NULL;
    vtss_vcap_id_t         id;
    u32                    mask, dip, dip_next, mac_count;
    u32                    port_count = vtss_state->port_count;
    BOOL                   src_found = 0, fid_add, dmac_found = 0;
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
           vtss_cmn_ip2u32(&ipmc->src.sip, ipmc->ipv6), 
           vtss_cmn_ip2u32(&ipmc->dst.dip, ipmc->ipv6));
        
    /* VCAP ID */
    id = l26_ip_mc_vcap_id(&ipmc->src, ipmc->ipv6);

    /* MAC address */
    memset(&mac_entry, 0, sizeof(mac_entry));
    mac_entry.locked = 1;
    l26_ip_mc_mac_get(vid_mac, &ipmc->dst, ipmc->ipv6);
    
    fid_add = (ipmc->src.ssm && ipmc->src.fid == 0 ? 1 : 0);
    if (cmd == VTSS_IPMC_CMD_CHECK) {
        /* Check that resources are available */

        /* Check source resources, if new SSM */
        if (fid_add) {
            /* Check IS1 resources */
            vtss_cmn_res_init(&res);
            res.is1.add = 1;
            VTSS_RC(vtss_cmn_res_check(&res));
            
            /* Check FID allocation */
            VTSS_RC(l26_ip_mc_fid_alloc(&ipmc->src.fid, ipmc->ipv6, 0));
        }

        /* Check MAC table resources */
        if (!ipmc->dst_add)
            return VTSS_RC_OK;

        mac_count = 1; /* One entry must always be added for ASM/SSM */
        for (src = ipmc_obj->src_used[ipmc->ipv6]; src != NULL; src = src->next) {
            if (ipmc->src.vid != src->data.vid)
                continue;
            
            if (ipmc->src.ssm) {
                /* Adding SSM destination */
                if (!src->data.ssm && fid_add) {
                    /* Up to one entry for each ASM DIP */
                    for (dst = src->dest; dst != NULL; dst = dst->next) {
                        mac_count++;
                    }
                }
                break;
            } 
            
            /* Adding ASM destination */
            if (src->data.ssm && 
                (src->next == NULL || src->next->data.fid != src->data.fid)) {
                /* Up to one entry for each SSM FID */
                mac_count++;
            }
        }
        VTSS_I("new dest needs %u, current: %u, max: %u",
               mac_count, vtss_state->mac_table_count, vtss_state->mac_table_max);
        
        if ((vtss_state->mac_table_count + mac_count) > vtss_state->mac_table_max) {
            VTSS_I("no more MAC entries");
            return VTSS_RC_ERROR;
        }
        return VTSS_RC_OK;
    } 

    if (cmd == VTSS_IPMC_CMD_ADD && fid_add) {
        /* Allocate FID  */
        VTSS_RC(l26_ip_mc_fid_alloc(&ipmc->src.fid, ipmc->ipv6, 1));

        /* Add IS1 entry */
        VTSS_RC(l26_ip_mc_is1_add(&ipmc->src, ipmc->ipv6));
    }

    /* We come to this point with VTSS_IPMC_CMD_ADD/DEL to add/delete resources */
    
    /* Add/delete MAC address entries */
    mask = (ipmc->ipv6 ? 0xffffffff : 0x007fffff);
    dip = (vtss_cmn_ip2u32(&ipmc->dst.dip, ipmc->ipv6) & mask);
    for (src = ipmc_obj->src_used[ipmc->ipv6]; src != NULL; src = src->next) {
        /* Skip sources not matching the VID */
        if (ipmc->src.vid != src->data.vid)
            continue;

        if (src->data.ssm == ipmc->src.ssm) {
            /* If adding/deleting SSM entry the FID must match */
            if (ipmc->src.ssm && src->data.fid != ipmc->src.fid)
                continue;
            src_found = 1;
        }

        /* Look for all DIPs mapping to the same DMAC */
        for (dst = src->dest; dst != NULL; dst = dst->next) {
            if ((vtss_cmn_ip2u32(&dst->data.dip, ipmc->ipv6) & mask) != dip)
                continue;
            dmac_found = 1;
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                if (VTSS_PORT_BF_GET(dst->data.member, port_no)) {
                    mac_entry.destination[port_no] = 1;
                }
            }
            if (src->data.ssm) {
                /* SSM entry, clear ASM add flag if DIP matches */
                for (dst_asm = dst_asm_first; dst_asm != NULL; dst_asm = dst_asm->next) {
                    if (dst_asm->add && 
                        memcmp(&dst->data.dip, &dst_asm->data.dip, sizeof(vtss_ip_addr_internal_t)) == 0)
                        dst_asm->add = 0;
                    if (dst_asm == dst_asm_last)
                        break;
                }
            } else {
                /* ASM entry, store pointers and set add flag */
                if (dst_asm_first == NULL)
                    dst_asm_first = dst;
                dst_asm_last = dst;
                dst->add = 1;
            }
        }

        /* Continue if the next source is using the same FID */
        if (src->next != NULL && src->next->data.fid == src->data.fid)
            continue;
        
        VTSS_I("%sSM, fid:%u, sip:0x%08x, dmac_found:%u",
               src->data.ssm ? "S" : "A", 
               src->data.fid, 
               vtss_cmn_ip2u32(&src->data.sip, ipmc->ipv6), 
               dmac_found);
        
        if (src->data.ssm) {
            /* For SSM entry, merge with the ASM destination set */
            for (dst = dst_asm_first; dst != NULL; dst = dst->next) {
                if (dst->add) {
                    dmac_found = 1;
                    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                        if (VTSS_PORT_BF_GET(dst->data.member, port_no)) {
                            mac_entry.destination[port_no] = 1;
                        }
                    }
                }
                dst->add = 1;
                if (dst == dst_asm_last)
                    break;
            }
        } else {
            /* Store ASM information */
            src_asm = src;
        }
        
        /* Add/delete MAC address entry (ignoring return codes) */
        vid_mac->vid = src->data.fid;
        if (dmac_found) {
            (void)vtss_mac_add(VTSS_MAC_USER_SSM, &mac_entry);
        } else {
            (void)vtss_mac_del(VTSS_MAC_USER_SSM, vid_mac);
        }
        
        /* Clear destination set */
        memset(mac_entry.destination, 0, port_count);
        dmac_found = 0;
    }
    
    /* No further processing if the source was found */
    if (src_found)
        return VTSS_RC_OK;

    /* If the source is deleted, the MAC entry must be deleted */
    vid_mac->vid = ipmc->src.fid;
    (void)vtss_mac_del(VTSS_MAC_USER_SSM, vid_mac);

    /* No further processing for ASM sources */
    if (!ipmc->src.ssm)
        return VTSS_RC_OK;

    /* Free FID */
    VTSS_RC(l26_ip_mc_fid_free(ipmc->src.fid, ipmc->ipv6));
        
    /* Delete IS1 entry */
    VTSS_RC(vtss_vcap_del(is1_obj, VTSS_IS1_USER_SSM, id));

    /* If SSM source is deleted, all ASM destinations must be deleted for SSM FID */
    if (src_asm != NULL && (dst = src_asm->dest) != NULL) {
        dip = (vtss_cmn_ip2u32(&dst->data.dip, ipmc->ipv6) & mask);
        while (dst != NULL) {
            /* Delete MAC address entry */
            l26_ip_mc_mac_get(vid_mac, &dst->data, ipmc->ipv6);
            (void)vtss_mac_del(VTSS_MAC_USER_SSM, vid_mac);

            /* Find next DIP mapping to another DMAC */
            for (dst = dst->next; dst != NULL; dst = dst->next) {
                dip_next = (vtss_cmn_ip2u32(&dst->data.dip, ipmc->ipv6) & mask);
                if (dip_next != dip) {
                    dip = dip_next;
                    break;
                }
            }
        }
    }
    return VTSS_RC_OK;
}

/* Update MAC address entries for source to a new FID */
static vtss_rc l26_ip_mc_mac_update(vtss_ipmc_src_t *src, vtss_vid_t old_fid, 
                                    vtss_vid_t new_fid, BOOL ipv6)
{
    vtss_mac_table_entry_t mac_entry;
    vtss_vid_mac_t         *vid_mac = &mac_entry.vid_mac;
    vtss_ipmc_dst_t        *dst;

    /* Update MAC address entries for all destinations */
    for (dst = src->dest; dst != NULL; dst = dst->next) {
        l26_ip_mc_mac_get(vid_mac, &dst->data, ipv6);
        vid_mac->vid = old_fid;
        if (vtss_mac_get(vid_mac, &mac_entry) == VTSS_RC_OK &&
            vtss_mac_del(VTSS_MAC_USER_SSM, vid_mac) == VTSS_RC_OK) {
            vid_mac->vid = new_fid;
            VTSS_RC(vtss_mac_add(VTSS_MAC_USER_SSM, &mac_entry));
        }
    }
    
    return VTSS_RC_OK;
}

/* Reallocate FID and update IP multicast entries accordingly */
static vtss_rc l26_ip_mc_fid_adjust(vtss_vid_t fid)
{
    vtss_ipmc_obj_t *obj = &vtss_state->ipmc.obj;
    vtss_ipmc_src_t *src;
    vtss_vid_t      ipv4_fid = 0, ipv6_fid = 0, new_fid, asm_vid;
    int             add, ipv6;
    u8              ipmc_used = vtss_state->vlan_table[fid].ipmc_used;
    
    /* Reallocate FIDs in two steps:
       1: Try to allocate IPv4/IPv6 FID
       2: Allocate and free IPv4/IPv6 FID */
    for (add = 0; add < 2; add++) {
        for (ipv6 = 0; ipv6 < 2; ipv6++) {
            if (ipmc_used & (ipv6 ? IPMC_USED_IPV6 : IPMC_USED_IPV4)) {
                /* Allocate new FID */
                VTSS_RC(l26_ip_mc_fid_alloc(ipv6 ? &ipv6_fid : &ipv4_fid, ipv6, add));
                
                if (add) {
                    /* Free old FID */
                    VTSS_RC(l26_ip_mc_fid_free(fid, ipv6));
                }
            }
        }
    }
    
    /* Update IS1 and MAC table entries */
    for (ipv6 = 0; ipv6 < 2; ipv6++) {
        if ((new_fid = (ipv6 ? ipv6_fid : ipv4_fid)) == 0)
            continue;

        /* SSM entries */
        asm_vid = VTSS_VID_NULL;
        for (src = obj->src_used[ipv6]; src != NULL; src = src->next) {
            if (src->data.ssm && src->data.fid == fid) {
                /* Update FID and add IS1 entry for source */
                asm_vid = src->data.vid;
                src->data.fid = new_fid;
                VTSS_RC(l26_ip_mc_is1_add(&src->data, ipv6));
                
                /* Update MAC address entries for all destinations */
                VTSS_RC(l26_ip_mc_mac_update(src, fid, new_fid, ipv6));
            }
        }
        

        /* ASM entries */
        if (asm_vid == VTSS_VID_NULL)
            continue;
        for (src = obj->src_used[ipv6]; src != NULL; src = src->next) {
            if (!src->data.ssm && src->data.vid == asm_vid) {
                /* Update MAC address entries for all destinations */
                VTSS_RC(l26_ip_mc_mac_update(src, fid, new_fid, ipv6));
                break;
            }
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_flood_conf_set(void)
{
    /* Unicast flood mask */
    VTSS_RC(l26_pgid_update(PGID_UC, vtss_state->uc_flood));
    
    /* Multicast flood mask */
    VTSS_RC(l26_pgid_update(PGID_MC, vtss_state->mc_flood));
    
    /* IPv4 flood mask */
    VTSS_RC(l26_pgid_update(PGID_MCIPV4, vtss_state->ipv4_mc_flood));

    /* IPv6 flood mask */
    VTSS_RC(l26_pgid_update(PGID_MCIPV6, vtss_state->ipv6_mc_flood));

    /* IPv6 flood scope */
    L26_WRM(VTSS_ANA_ANA_FLOODING_IPMC, 
            VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC6_CTRL(vtss_state->ipv6_mc_scope ? PGID_MCIPV6 : PGID_MC),
            VTSS_M_ANA_ANA_FLOODING_IPMC_FLD_MC6_CTRL);
    
    return VTSS_RC_OK;
}

/* Setup water marks, drop levels, etc for port */
static vtss_rc l26_port_fc_setup(u32 port,  
                                 const vtss_port_conf_t *const conf)
{
    const u8 *mac;
    u32 pause_start, pause_stop, rsrv_raw, rsrv_total=0, atop_wm;
    vtss_port_conf_t *conf_tmp;
    vtss_port_no_t port_no;


    pause_stop  = 0x7ff;
    pause_start = 0x7ff;
    rsrv_raw = 0;

    if (conf->flow_control.generate) {
        if (conf->max_frame_length <= VTSS_MAX_FRAME_LENGTH_STANDARD) {
            pause_start = 190;    /* 6 x 1518 / 48 */
        } else {
            pause_start = 221;    /* 7 x 1518 / 48 */
        }
    }

    if (conf->flow_control.generate && conf->max_frame_length <= VTSS_MAX_FRAME_LENGTH_STANDARD) {
        /* FC enabled and no jumbo */
        pause_stop  = 127;    /* 4 x 1518 / 48 */
        rsrv_raw  =  253;     /* 8 x 1518 / 48 */
    } else if (conf->flow_control.generate && conf->max_frame_length > VTSS_MAX_FRAME_LENGTH_STANDARD) {
        /* FC enabled and jumbo */
        pause_stop = 158;     /* 5 x 1518 / 48 */
        rsrv_raw =  284;      /* 9 x 1518 / 48 */        
    } else if (!conf->flow_control.generate && conf->max_frame_length > VTSS_MAX_FRAME_LENGTH_STANDARD) {
        /* FC disabled and jumbo */
        rsrv_raw  = 250;      /* 12000 / 48 */
    }

    /* Calculate the total reserved space for all ports*/
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        conf_tmp = &vtss_state->port_conf[port_no];
        if (conf_tmp->flow_control.generate && conf_tmp->max_frame_length <= VTSS_MAX_FRAME_LENGTH_STANDARD) {
            rsrv_total +=  12144; /* 8*1518 */
        } else if (conf_tmp->flow_control.generate && conf_tmp->max_frame_length > VTSS_MAX_FRAME_LENGTH_STANDARD) {
            rsrv_total +=  13662; /* 9*1518 */
        } else if (!conf_tmp->flow_control.generate && conf_tmp->max_frame_length > VTSS_MAX_FRAME_LENGTH_STANDARD) {
            rsrv_total  += 12000;
        }        
    }
    
    /* Set Pause WM hysteresis*/
    L26_WR(VTSS_SYS_PAUSE_CFG_PAUSE_CFG(port),
           VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_START(pause_start) |
           VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_STOP(pause_stop) |
           (conf->flow_control.generate ? VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_ENA : 0));

    atop_wm = (L26_BUFFER_MEMORY - rsrv_total)/L26_BUFFER_CELL_SZ;
    if (atop_wm >= 1024) {
        atop_wm = 1024 + atop_wm/16;
    }

    /*  When 'port ATOP' and 'common ATOP_TOT' are exceeded, tail dropping is activated on a port */
    L26_WR(VTSS_SYS_PAUSE_CFG_ATOP_TOT_CFG, atop_wm);
    L26_WR(VTSS_SYS_PAUSE_CFG_ATOP(port), rsrv_raw);

    /* Set SMAC of Pause frame */
    mac = &conf->flow_control.smac.addr[0];
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_MAC_HIGH_CFG(VTSS_TO_DEV(port)),(mac[0]<<16) | (mac[1]<<8) | mac[2]);
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_MAC_LOW_CFG(VTSS_TO_DEV(port)), (mac[3]<<16) | (mac[4]<<8) | mac[5]);

    /* Enable/disable FC incl. pause value and zero pause */
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_CFG(VTSS_TO_DEV(port)),
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_ZERO_PAUSE_ENA |
           (conf->flow_control.obey ? VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_RX_FC_ENA  : 0) |
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_PAUSE_VAL_CFG(0xff) |
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_FC_LATENCY_CFG(63));  /* changed from 7 to 63 (recommended by MOT) */
  
    return VTSS_RC_OK;
}

/* Returns the serdes instance and if it is a serdes6g or serdes1g */
static vtss_rc serdes_instance_get(u32 port, u32 *instance, BOOL *serdes6g) 
{
    vtss_port_mux_mode_t mode = vtss_state->init_conf.mux_mode;

    int mode_0_serd6[] =  {-1,-1,1,1,1,1,1,1,1,1,1,1,1,1,0,1};
    int mode_0_inst[] =   {-1,-1,3,3,3,3,2,2,2,2,1,1,1,1,0,0};

    int mode_1_serd6[] =  {-1,-1,-1,-1,0,0,1,0,0,1,0,0,0,0,1,1};
    int mode_1_inst[] =   {-1,-1,-1,-1,7,6,3,5,4,2,3,2,1,0,1,0};

    int mode_2_serd6[] =  {0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0};
    int mode_2_inst[] =   {7,6,3,3,3,3,2,2,2,2,5,4,3,2,0,1};

    if (port < 10) {
        return VTSS_RC_ERROR;
    }
    
    if (mode == 0) {
        if (mode_0_inst[port-10] == -1 || mode_0_inst[port-10] == -1) {
            return VTSS_RC_ERROR;
        }
        *instance = mode_0_inst[port-10];
        *serdes6g = mode_0_serd6[port-10];
    } else if (mode == 1) {
        if (mode_1_inst[port-10] == -1 || mode_1_inst[port-10] == -1) {
            return VTSS_RC_ERROR;
        }
        *instance = mode_1_inst[port-10];
        *serdes6g = mode_1_serd6[port-10];        
    } else {
        *instance = mode_2_inst[port-10];
        *serdes6g = mode_2_serd6[port-10];        
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_port_max_tags_set(vtss_port_no_t port_no)
{
    vtss_port_max_tags_t  max_tags = vtss_state->port_conf[port_no].max_tags;
    vtss_vlan_port_type_t vlan_type = vtss_state->vlan_port_conf[port_no].port_type;
    u32                   port = VTSS_CHIP_PORT(port_no);
    u32                   tgt = VTSS_TO_DEV(port);
    u32                   etype, double_ena, single_ena;

    /* S-ports and VLAN unaware ports both support 0x88a8 (in addition to 0x8100) */
    etype = (vlan_type == VTSS_VLAN_PORT_TYPE_S_CUSTOM ? vtss_state->vlan_conf.s_etype :
             vlan_type == VTSS_VLAN_PORT_TYPE_C ? VTSS_ETYPE_TAG_C : VTSS_ETYPE_TAG_S);
    single_ena = (max_tags == VTSS_PORT_MAX_TAGS_NONE ? 0 : 1);
    double_ena = (max_tags == VTSS_PORT_MAX_TAGS_TWO ? 1 : 0);
    
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_TAGS_CFG(tgt), 
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG_TAG_ID(etype) |
           (double_ena ? VTSS_F_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG_VLAN_DBL_AWR_ENA : 0) |
           (single_ena ? VTSS_F_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG_VLAN_AWR_ENA : 0) |
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG_VLAN_LEN_AWR_ENA);
    return VTSS_RC_OK;
}

/* MAC config for internal(0-11), SGMII, Serdes, 100fx and VAUI ports.        */
/* Ports 10 and 11 can connect to internal Phys (GMII) or to a Serdes1g macro */
static vtss_rc l26_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_port_conf_t       *conf = &vtss_state->port_conf[port_no];
    u32                    port = VTSS_CHIP_PORT(port_no);
    u32                    link_speed, value, tgt = VTSS_TO_DEV(port), instance;
    vtss_port_interface_t  if_type = conf->if_type;
    vtss_port_frame_gaps_t gaps;
    vtss_port_speed_t      speed = conf->speed;
    BOOL                   fdx=conf->fdx, disable=conf->power_down;
    BOOL                   sgmii=0, if_100fx=0, serdes6g;
    vtss_serdes_mode_t     mode = VTSS_SERDES_MODE_SGMII;   
    u16                    delay=0; 
   
    gaps.hdx_gap_1   = 0;
    gaps.hdx_gap_2   = 0;
    gaps.fdx_gap     = 0;

    /* Verify speed and interface type */
    switch (speed) {
    case VTSS_SPEED_10M:
        link_speed = 3;
        break;
    case VTSS_SPEED_100M:
        link_speed = 2;
        break;
    case VTSS_SPEED_1G:
    case VTSS_SPEED_2500M:
        link_speed = 1;
        break;
    default:
        VTSS_E("illegal speed, port %u", port);
        return VTSS_RC_ERROR;
    }

    switch (if_type) {
    case VTSS_PORT_INTERFACE_NO_CONNECTION:
        disable = 1;
        break;
    case VTSS_PORT_INTERFACE_INTERNAL:
    case VTSS_PORT_INTERFACE_SGMII:
    case VTSS_PORT_INTERFACE_QSGMII:
        sgmii = 1;
        break;
    case VTSS_PORT_INTERFACE_SERDES:
        if (speed != VTSS_SPEED_1G) {
            VTSS_E("illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        mode = VTSS_SERDES_MODE_1000BaseX;
        break;
    case VTSS_PORT_INTERFACE_100FX:
        if (speed != VTSS_SPEED_100M) {
            VTSS_E("illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        mode = VTSS_SERDES_MODE_100FX;
        if_100fx = 1;      
        break;
    case VTSS_PORT_INTERFACE_SGMII_CISCO:
        if (speed != VTSS_SPEED_10M && speed != VTSS_SPEED_100M && speed != VTSS_SPEED_1G) {
            VTSS_E("SFP_CU, illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        mode = VTSS_SERDES_MODE_1000BaseX;
        sgmii = 1;        
        break;
    case VTSS_PORT_INTERFACE_VAUI:
        if (speed != VTSS_SPEED_1G && speed != VTSS_SPEED_2500M) {
            VTSS_E("illegal speed, port %u", port);
            return VTSS_RC_ERROR;
        }
        mode = (speed == VTSS_SPEED_2500M ? VTSS_SERDES_MODE_2G5 : VTSS_SERDES_MODE_1000BaseX);
        break;
    case VTSS_PORT_INTERFACE_LOOPBACK:
        if (port < 12) {
            sgmii = 1;
        } else {
            mode = VTSS_SERDES_MODE_1000BaseX;
        }
        break;
    default:
        VTSS_E("illegal interface, port %u", port);
        return VTSS_RC_ERROR;
    }
    /* (re-)configure the Serdes macros to 100FX / 1000BaseX / 2500 */
    if (mode != vtss_state->serdes_mode[port_no] && mode != VTSS_SERDES_MODE_SGMII) {
        VTSS_RC(serdes_instance_get(port, &instance, &serdes6g));
        if (mode == VTSS_SERDES_MODE_2G5 && !serdes6g) {
            VTSS_E("illegal serdes settings, port %u", port);
            return VTSS_RC_ERROR;
        }          
        if (serdes6g) {
            VTSS_RC(l26_sd6g_read(1<<instance));
            VTSS_MSLEEP(10);
            VTSS_RC(l26_sd6g_cfg(mode, 1<<instance));
            VTSS_RC(l26_sd6g_write(1<<instance, L26_SERDES_WAIT));
        } else {
            VTSS_RC(l26_sd1g_read(1<<instance));
            VTSS_MSLEEP(10);
            VTSS_RC(l26_sd1g_cfg(mode, 1<<instance));
            VTSS_RC(l26_sd1g_write(1<<instance, L26_SERDES_WAIT));
        }
        VTSS_MSLEEP(1);          
        vtss_state->serdes_mode[port_no] = mode;
    }
    /* Port disable and flush procedure: */
    /* 0.1: Reset the PCS */   
    if (port > 9) {    
        L26_WRM_SET(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt),
                    VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PCS_RX_RST);
    }
    /* 1: Disable MAC frame reception */
    L26_WRM_CLR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_ENA_CFG(tgt),
                VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA);

    /* 2: Disable traffic being sent to or from switch port */
    L26_WRM_CLR(VTSS_SYS_SYSTEM_SWITCH_PORT_MODE(port),
                VTSS_F_SYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA);

    /* 3: Disable dequeuing from the egress queues *\/ */
    L26_WRM_SET(VTSS_SYS_SYSTEM_PORT_MODE(port),
                VTSS_F_SYS_SYSTEM_PORT_MODE_DEQUEUE_DIS);

    /* 4: Wait a worst case time 8ms (jumbo/10Mbit) *\/ */
    VTSS_MSLEEP(8);

    /* 5: Disable HDX backpressure (Bugzilla 3203) */
    L26_WRM_CLR(VTSS_SYS_SYSTEM_FRONT_PORT_MODE(port), 
                VTSS_F_SYS_SYSTEM_FRONT_PORT_MODE_HDX_MODE);
   
    /* 6: Flush the queues accociated with the port */
    L26_WRM_SET(VTSS_REW_PORT_PORT_CFG(port), 
                VTSS_F_REW_PORT_PORT_CFG_FLUSH_ENA);

    /* 7: Enable dequeuing from the egress queues */
    L26_WRM_CLR(VTSS_SYS_SYSTEM_PORT_MODE(port),
                VTSS_F_SYS_SYSTEM_PORT_MODE_DEQUEUE_DIS);

    /* 9: Reset the clock */
    if (port > 9) {
        value = 
            VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_TX_RST |
            VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_RX_RST |
            VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PORT_RST;
        L26_WR(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), value);
    } else {
        value = 
            VTSS_F_DEV_GMII_PORT_MODE_CLOCK_CFG_MAC_TX_RST |
            VTSS_F_DEV_GMII_PORT_MODE_CLOCK_CFG_MAC_RX_RST |
            VTSS_F_DEV_GMII_PORT_MODE_CLOCK_CFG_PORT_RST;
        L26_WR(VTSS_DEV_GMII_PORT_MODE_CLOCK_CFG(tgt), value);
    }


    do { /* 8: Wait until flushing is complete */
        L26_RD(VTSS_SYS_SYSTEM_SW_STATUS(port), &value);
        VTSS_MSLEEP(1);            
        delay++;
        if (delay == 2000) {
            VTSS_E("Flush timeout port %u, delay:%d\n",port, delay);
            break;
        }
    } while(value & VTSS_M_SYS_SYSTEM_SW_STATUS_EQ_AVAIL);
    VTSS_I("port:%d, Flush delay:%d", port, delay);


    /* 10: Clear flushing */
    L26_WRM_CLR(VTSS_REW_PORT_PORT_CFG(port), VTSS_F_REW_PORT_PORT_CFG_FLUSH_ENA);

    /* The port is disabled and flushed, now set up the port in the new operating mode */

    /* Bugzilla 4388: disabling frame aging when in HDX */
    L26_WRM(VTSS_REW_PORT_PORT_CFG(port), (fdx ? 0 : VTSS_F_REW_PORT_PORT_CFG_AGE_DIS), VTSS_F_REW_PORT_PORT_CFG_AGE_DIS);

    /* GIG/FDX mode */
    if (fdx && ((speed == VTSS_SPEED_1G) || speed == VTSS_SPEED_2500M)) {
        value = VTSS_F_DEV_MAC_CFG_STATUS_MAC_MODE_CFG_FDX_ENA | VTSS_F_DEV_MAC_CFG_STATUS_MAC_MODE_CFG_GIGA_MODE_ENA;
    } else if (fdx) {
        value = VTSS_F_DEV_MAC_CFG_STATUS_MAC_MODE_CFG_FDX_ENA;
    } else {        
        L26_WRM(VTSS_SYS_SYSTEM_FRONT_PORT_MODE(port), 1, VTSS_F_SYS_SYSTEM_FRONT_PORT_MODE_HDX_MODE);
        value = 0;
    }
    /* Set GIG/FDX mode */      
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), value)
    

    /* Default FDX gaps */
    if ((speed==VTSS_SPEED_1G)||(speed==VTSS_SPEED_2500M)) {
        gaps.fdx_gap = 5;
    } else if (port < 10) {
        gaps.fdx_gap = 16;
    } else if ((port < 12) && (vtss_state->init_conf.mux_mode != VTSS_PORT_MUX_MODE_2))  {
        if (!fdx) {
            gaps.fdx_gap = 14;
        } else {
            gaps.fdx_gap = 16;
        }
    } else {
        gaps.fdx_gap = 15;
    }

    /* Default HDX gaps */
    if (speed == VTSS_SPEED_10M && !fdx) {
        gaps.hdx_gap_1 = 11;
        gaps.hdx_gap_2 = 9;
    } else if (speed == VTSS_SPEED_100M && !fdx) {
        gaps.hdx_gap_1 = 7;
        gaps.hdx_gap_2 = 9;
    }
    /* Non default gaps */
    if (conf->frame_gaps.hdx_gap_1 != VTSS_FRAME_GAP_DEFAULT) 
        gaps.hdx_gap_1 = conf->frame_gaps.hdx_gap_1;
    if (conf->frame_gaps.hdx_gap_2 != VTSS_FRAME_GAP_DEFAULT)
        gaps.hdx_gap_2 = conf->frame_gaps.hdx_gap_2;
    if (conf->frame_gaps.fdx_gap != VTSS_FRAME_GAP_DEFAULT)
        gaps.fdx_gap = conf->frame_gaps.fdx_gap;
    
    /* Set MAC IFG Gaps */
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_IFG_CFG(tgt), 
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_TX_IFG(gaps.fdx_gap) |
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG1(gaps.hdx_gap_1) |
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG2(gaps.hdx_gap_2));

    /* Set MAC HDX Late collision and assert seed load. Seed load must be de-asserted after min 1us */
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_HDX_CFG(tgt), 
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_LATE_COL_POS(port<10?64:67) |
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED_LOAD);
    VTSS_NSLEEP(1000);
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_HDX_CFG(tgt),
           (conf->exc_col_cont ? VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_RETRY_AFTER_EXC_COL_ENA : 0) | 
           VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_LATE_COL_POS(port<10?64:67));
  

    /* PSC settings for 100fx/SGMII/SERDES */
    if (port > 9) {
        if (if_100fx) {
            /* VTSS_MSLEEP(1);            */
            value = (disable ? 0 : VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_PCS_ENA) |
                (conf->sd_internal    ? 0 : VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_SEL) |
                (conf->sd_active_high ? 1<<25 : 0)  |  /* VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_SD_POL [DBG] */
                (conf->sd_enable      ? VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_ENA : 0);

            /* 100FX PCS */                    
            L26_WRM(VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt),value,
                    VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_PCS_ENA | 
                    VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_SEL |
                    (1<<25) | /* VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_POL [DBG] */
                    VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_ENA)                                                 
        } else {
            /* Disable 100FX */
            L26_WRM(VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt),0,
                    VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_PCS_ENA)

            /* Choose SGMII or Serdes PCS mode */
            L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG(tgt), 
                   (sgmii ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG_SGMII_MODE_ENA : 0));

            if (sgmii) {
                L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt),
                       VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_SW_RESOLVE_ENA); /* Set whole register */
            } else {
                L26_WRM_CLR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt),
                            VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_SW_RESOLVE_ENA); /* Clear specific bit only */
            }
            
            L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG(tgt),
                  (conf->sd_active_high ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_POL : 0) |
                   (conf->sd_enable ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_ENA : 0) |
                   (conf->sd_internal ? 0 : VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_SEL));
        
            /* Enable/disable PCS */
            L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt), 
                   disable ? 0 : VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);

            if (conf->if_type == VTSS_PORT_INTERFACE_SGMII) {
                L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 0);
            } else if (conf->if_type == VTSS_PORT_INTERFACE_SGMII_CISCO) {
                /* Complete SGMII aneg */
                value = 0x0001;       
                
                L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt),
                       VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ADV_ABILITY(value)    |
                       VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_SW_RESOLVE_ENA        |
                       VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_ENA              |
                       VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_RESTART_ONE_SHOT);
                
                /* Clear the sticky bits */
                L26_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), &value);
                L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), value);
            }           
        }
    }
  
    /* Disable loopback */
    if (port < 10) {
        L26_WRM_CLR(VTSS_DEV_GMII_PORT_MODE_PORT_MISC(tgt), VTSS_F_DEV_GMII_PORT_MODE_PORT_MISC_GMII_LOOP_ENA); 
    } else if (port > 9 && port < 12) {
        L26_WRM_CLR(VTSS_DEV_PORT_MODE_PORT_MISC(tgt), VTSS_F_DEV_PORT_MODE_PORT_MISC_DEV_LOOP_ENA); 
    } else {
        L26_WRM_CLR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LB_CFG(tgt), VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_LB_CFG_TBI_HOST_LB_ENA);
    }
    /* Enable loopback if requested */
    if (conf->if_type == VTSS_PORT_INTERFACE_LOOPBACK || conf->loop == VTSS_PORT_LOOP_PCS_HOST) {
        if (conf->loop == VTSS_PORT_LOOP_PCS_HOST && port < 12) {
            VTSS_E("PCS loopback not supported for port:%u\n",port_no);
            return VTSS_RC_ERROR;
        }
        if (port < 10) {
            L26_WRM_SET(VTSS_DEV_GMII_PORT_MODE_PORT_MISC(tgt), VTSS_F_DEV_GMII_PORT_MODE_PORT_MISC_GMII_LOOP_ENA); 
        } else if (port > 9 && port < 12) {
            L26_WRM_SET(VTSS_DEV_PORT_MODE_PORT_MISC(tgt), VTSS_F_DEV_PORT_MODE_PORT_MISC_DEV_LOOP_ENA); 
        } else {
            L26_WRM_SET(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LB_CFG(tgt), VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_LB_CFG_TBI_HOST_LB_ENA);
        }
    }

    /* Set Max Length and maximum tags allowed */
    L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MAXLEN_CFG(tgt), conf->max_frame_length);
    VTSS_RC(l26_port_max_tags_set(port_no));

    if (!disable) {
        /* Configure flow control */
        if (l26_port_fc_setup(port, conf) != VTSS_RC_OK) {
            VTSS_E("Flow control setup issue, port:%u\n", port_no);
            return VTSS_RC_ERROR;
        }
       
        /* Update policer flow control configuration */
        VTSS_RC(l26_port_policer_fc_set(port_no, port));

        /* Enable MAC module */
        L26_WR(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_ENA_CFG(tgt), 
               VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA |
               VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_TX_ENA);
        
        /* Take MAC, Port, Phy (intern) and PCS (SGMII/Serdes) clock out of reset */
        if (port > 9) { 
            L26_WR(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), VTSS_F_DEV_PORT_MODE_CLOCK_CFG_LINK_SPEED(link_speed));
        } else {
            L26_WR(VTSS_DEV_GMII_PORT_MODE_CLOCK_CFG(tgt), 0);
        }
      
        /* Clear the PCS stickys */
        L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), 
               VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY |
               VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_OUT_OF_SYNC_STICKY);

        /* Core: Enable port for frame transfer */
        L26_WRM_SET(VTSS_SYS_SYSTEM_SWITCH_PORT_MODE(port), 
                    VTSS_F_SYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA);

        /* Enable flowcontrol - must be done after flushing */
        if (conf->flow_control.generate) {   
            L26_WRM_SET(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_CFG(VTSS_TO_DEV(port)), VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_TX_FC_ENA);
        }

    }                 
    return VTSS_RC_OK;
}

/* Set 1000Base-X Fiber Auto-negotiation (Clause 37) */
static vtss_rc l26_port_clause_37_control_set(const vtss_port_no_t port_no)
{
    vtss_port_clause_37_control_t *control = &vtss_state->port_clause_37[port_no];
    u32 value, tgt = VTSS_TO_DEV(vtss_state->port_map[port_no].chip_port);
       
    /* Aneg capabilities for this port */
    VTSS_RC(vtss_cmn_port_clause_37_adv_set(&value, &control->advertisement, control->enable));

    /* Set aneg capabilities and restart */
    L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 
           VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ADV_ABILITY(value) |
           VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_RESTART_ONE_SHOT |
           VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_ENA);
    
    if (!control->enable) {
        /* Disable Aneg */
        L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 
               VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ADV_ABILITY(0) |
               VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_RESTART_ONE_SHOT);
        
    }
    return VTSS_RC_OK;
}

/* Get 1000Base-X Fiber Auto-negotiation status (Clause 37) */
static vtss_rc l26_port_clause_37_status_get(const vtss_port_no_t         port_no, 
                                             vtss_port_clause_37_status_t *const status)

{
    u32 value, tgt = VTSS_TO_DEV(vtss_state->port_map[port_no].chip_port);

    if (vtss_state->port_conf[port_no].power_down) {
        status->link = 0;
        return VTSS_RC_OK;
    }

    /* Get the link state 'down' sticky bit  */
    L26_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), &value);
    status->link = L26_BF(DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY, value) ? 0 : 1;
    if (status->link == 0) {
        /* The link has been down. Clear the sticky bit and return the 'down' value  */
        L26_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), 
               VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY |
               VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_OUT_OF_SYNC_STICKY);        
    } else {
        L26_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS(tgt), &value);
        status->link = L26_BF(DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS, value) &&
                       L26_BF(DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_SYNC_STATUS, value);        
    }

    /* Get PCS ANEG status register */
    L26_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS(tgt), &value);

    /* Get 'Aneg complete'   */
    status->autoneg.complete = L26_BF(DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE, value);

    /* Workaround for a Serdes issue, when aneg completes with FDX capability=0 */
    if (vtss_state->port_conf[port_no].if_type == VTSS_PORT_INTERFACE_SERDES) {
        if (status->autoneg.complete) {
            if (((value >> 21) & 0x1) == 0) { 
                L26_WRM_CLR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt), VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);
                L26_WRM_SET(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt), VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);
                (void)l26_port_clause_37_control_set(port_no); /* Restart Aneg */
                VTSS_MSLEEP(50);
                L26_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS(tgt), &value);
                status->autoneg.complete = L26_BF(DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE, value);
            }
        }
    }

    /* Return partner advertisement ability */
    value = VTSS_X_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_LP_ADV_ABILITY(value);

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

/* Get status of the VAUI or 100FX ports.  Internal/SGMII/Serdes ports are covered elsewhere */
static vtss_rc l26_port_status_get(const vtss_port_no_t  port_no, 
                                   vtss_port_status_t    *const status)
{
    vtss_port_conf_t *conf = &vtss_state->port_conf[port_no];
    u32              tgt = VTSS_TO_DEV(vtss_state->port_map[port_no].chip_port);
    u32              value;
    
    if (conf->power_down) {
        memset(status, 0, sizeof(*status));
        return VTSS_RC_OK;
    }

    if (conf->if_type == VTSS_PORT_INTERFACE_100FX) {
        /* Get the PCS status  */
        L26_RD(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), &value);
        
        /* Link has been down if the are any error stickies */
        status->link_down = L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_LOST_STICKY, value) ||
                            L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SSD_ERROR_STICKY, value) ||
                            L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_FEF_FOUND_STICKY, value) ||
                            L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_PCS_ERROR_STICKY, value);

        if (status->link_down) {
            L26_WR(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), 0xFFFF);
            L26_RD(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), &value);
        }
        /* Link=1 if sync status=1 and no error stickies after a clear */
        status->link = L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_STATUS, value) && 
                     (!L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_LOST_STICKY, value) &&
                      !L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SSD_ERROR_STICKY, value) &&
                      !L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_FEF_FOUND_STICKY, value) &&
                      !L26_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_PCS_ERROR_STICKY, value));

        status->speed = VTSS_SPEED_100M; 
        
        // 100FX SFP can be auto detected and then the aneg status must be updated to the current flow control .
        status->aneg.obey_pause = conf->flow_control.obey;
        status->aneg.generate_pause = conf->flow_control.generate;

    }  else if (conf->if_type == VTSS_PORT_INTERFACE_VAUI) {
        /* Get the PCS status */
        L26_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS(tgt), &value);
        status->link = L26_BF(DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS, value) &&
                       L26_BF(DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_SYNC_STATUS, value);  

        L26_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), &value);

        status->link_down = L26_BF(DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY, value);
        
        if (status->link_down) {
            /* The link has been down. Clear the sticky bit */
            L26_WRM_SET(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt),
                        VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY |
                        VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_OUT_OF_SYNC_STICKY);
        }
        status->speed = VTSS_SPEED_2500M; 
    }  else if (conf->if_type == VTSS_PORT_INTERFACE_NO_CONNECTION) {
        status->link = 0;
        status->link_down = 0;
    } else {
        // VTSS_E("Status not supported for port: %u",port_no);
        return VTSS_RC_ERROR;
    }        
    status->fdx = 1;
    return VTSS_RC_OK;
}

/* Read a counter and return the value */
static vtss_rc l26_port_counter(u32 *cnt_addr, u32 port, vtss_chip_counter_t *counter, BOOL clear)
{
    u32 value, addr;

    addr = *cnt_addr;
    *cnt_addr = (addr + 1); /* Next counter */
    if (addr >= 0xC00) {     
        addr += 18*port; /* Drop cnt */
    } else if (addr >= 0x800) {
        addr += 31*port; /* Tx cnt */
    } else {
        addr += 43*port; /* Rx cnt */
    }

    L26_RD(VTSS_SYS_STAT_CNT(addr), &value);
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

/* Read the chip counters and store them in a different counter groups */
static vtss_rc l26_port_counters_read(u32                          port, 
                                      vtss_port_luton26_counters_t *c,
                                      vtss_port_counters_t *const  counters, 
                                      BOOL                         clear)
{
    u32                                i, base, *p = &base;
    vtss_port_rmon_counters_t          *rmon;
    vtss_port_if_group_counters_t      *if_group;
    vtss_port_ethernet_like_counters_t *elike;
    vtss_port_proprietary_counters_t   *prop;

    /* 32-bit Rx chip counters */
    base = 0x000;
    VTSS_RC(l26_port_counter(p, port, &c->rx_octets, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_unicast, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_multicast, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_broadcast, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_shorts, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_fragments, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_jabbers, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_crc_align_errors, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_64, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_65_127, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_128_255, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_256_511, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_512_1023, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_1024_1526, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_1527_max, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_pause, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_control, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_longs, clear));
    VTSS_RC(l26_port_counter(p, port, &c->rx_classified_drops, clear));
    for (i = 0; i < VTSS_PRIOS; i++) 
        VTSS_RC(l26_port_counter(p, port, &c->rx_red_class[i], clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(l26_port_counter(p, port, &c->rx_yellow_class[i], clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(l26_port_counter(p, port, &c->rx_green_class[i], clear));

    /* 32-bit Drop chip counters */
    base = 0xC00;
    VTSS_RC(l26_port_counter(p, port, &c->dr_local, clear));
    VTSS_RC(l26_port_counter(p, port, &c->dr_tail, clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(l26_port_counter(p, port, &c->dr_yellow_class[i], clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(l26_port_counter(p, port, &c->dr_green_class[i], clear));

    /* 32-bit Tx chip counters */
    base = 0x800;
    VTSS_RC(l26_port_counter(p, port, &c->tx_octets, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_unicast, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_multicast, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_broadcast, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_collision, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_drops, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_pause, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_64, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_65_127, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_128_255, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_256_511, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_512_1023, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_1024_1526, clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_1527_max, clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(l26_port_counter(p, port, &c->tx_yellow_class[i], clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(l26_port_counter(p, port, &c->tx_green_class[i], clear));
    VTSS_RC(l26_port_counter(p, port, &c->tx_aging, clear));

    if (counters == NULL) {
        return VTSS_RC_OK;
    }

    /* Calculate API counters based on chip counters */
    rmon = &counters->rmon;
    if_group = &counters->if_group;
    elike = &counters->ethernet_like;
    prop = &counters->prop;

    /* Proprietary counters */
    for (i = 0; i < VTSS_PRIOS; i++) {
        prop->rx_prio[i] = c->rx_red_class[i].value + c->rx_yellow_class[i].value + c->rx_green_class[i].value;
        prop->tx_prio[i] = c->tx_yellow_class[i].value + c->tx_green_class[i].value;
    }

    /* RMON Rx counters */
    rmon->rx_etherStatsDropEvents = c->rx_classified_drops.value + c->dr_tail.value;
    for (i = 0; i < VTSS_PRIOS; i++) {
        rmon->rx_etherStatsDropEvents += (c->dr_yellow_class[i].value + c->dr_green_class[i].value);
    }

    rmon->rx_etherStatsOctets = c->rx_octets.value;
    rmon->rx_etherStatsPkts = 
        (c->rx_shorts.value + c->rx_fragments.value + c->rx_jabbers.value + c->rx_longs.value + 
         c->rx_64.value + c->rx_65_127.value + c->rx_128_255.value + c->rx_256_511.value + 
         c->rx_512_1023.value + c->rx_1024_1526.value + c->rx_1527_max.value);
    rmon->rx_etherStatsBroadcastPkts = c->rx_broadcast.value;
    rmon->rx_etherStatsMulticastPkts = c->rx_multicast.value;
    rmon->rx_etherStatsCRCAlignErrors = c->rx_crc_align_errors.value;
    rmon->rx_etherStatsUndersizePkts = c->rx_shorts.value;
    rmon->rx_etherStatsOversizePkts = c->rx_longs.value;
    rmon->rx_etherStatsFragments = c->rx_fragments.value;
    rmon->rx_etherStatsJabbers = c->rx_jabbers.value;
    rmon->rx_etherStatsPkts64Octets = c->rx_64.value;
    rmon->rx_etherStatsPkts65to127Octets = c->rx_65_127.value;
    rmon->rx_etherStatsPkts128to255Octets = c->rx_128_255.value;
    rmon->rx_etherStatsPkts256to511Octets = c->rx_256_511.value;
    rmon->rx_etherStatsPkts512to1023Octets = c->rx_512_1023.value;
    rmon->rx_etherStatsPkts1024to1518Octets = c->rx_1024_1526.value;
    rmon->rx_etherStatsPkts1519toMaxOctets = c->rx_1527_max.value;

    /* RMON Tx counters */
    rmon->tx_etherStatsDropEvents = (c->tx_drops.value + c->tx_aging.value);
    rmon->tx_etherStatsOctets = c->tx_octets.value;
    rmon->tx_etherStatsPkts = c->tx_64.value + c->tx_65_127.value + c->tx_128_255.value + c->tx_256_511.value + 
                              c->tx_512_1023.value + c->tx_1024_1526.value + c->tx_1527_max.value;
    rmon->tx_etherStatsBroadcastPkts = c->tx_broadcast.value;
    rmon->tx_etherStatsMulticastPkts = c->tx_multicast.value;
    rmon->tx_etherStatsCollisions = c->tx_collision.value;
    rmon->tx_etherStatsPkts64Octets = c->tx_64.value;
    rmon->tx_etherStatsPkts65to127Octets = c->tx_65_127.value;
    rmon->tx_etherStatsPkts128to255Octets = c->tx_128_255.value;
    rmon->tx_etherStatsPkts256to511Octets = c->tx_256_511.value;
    rmon->tx_etherStatsPkts512to1023Octets = c->tx_512_1023.value;
    rmon->tx_etherStatsPkts1024to1518Octets = c->tx_1024_1526.value;
    rmon->tx_etherStatsPkts1519toMaxOctets = c->tx_1527_max.value;
    
    /* Interfaces Group Rx counters */
    if_group->ifInOctets = c->rx_octets.value;
    if_group->ifInUcastPkts = c->rx_unicast.value;
    if_group->ifInMulticastPkts = c->rx_multicast.value;
    if_group->ifInBroadcastPkts = c->rx_broadcast.value;
    if_group->ifInNUcastPkts = c->rx_multicast.value + c->rx_broadcast.value;
    if_group->ifInDiscards = rmon->rx_etherStatsDropEvents;
    if_group->ifInErrors = c->rx_crc_align_errors.value + c->rx_shorts.value + c->rx_fragments.value +
                           c->rx_jabbers.value + c->rx_longs.value;
  
    /* Interfaces Group Tx counters */
    if_group->ifOutOctets = c->tx_octets.value;
    if_group->ifOutUcastPkts = c->tx_unicast.value;
    if_group->ifOutMulticastPkts = c->tx_multicast.value;
    if_group->ifOutBroadcastPkts = c->tx_broadcast.value;
    if_group->ifOutNUcastPkts = c->tx_multicast.value + c->tx_broadcast.value;
    if_group->ifOutErrors = c->tx_drops.value + c->tx_aging.value;

    /* Ethernet-like counters */
    elike->dot3InPauseFrames = c->rx_pause.value;
    elike->dot3OutPauseFrames = c->tx_pause.value;     

    /* Bridge counters */
    counters->bridge.dot1dTpPortInDiscards = c->dr_local.value;

#if defined(VTSS_ARCH_CARACAL)
    /* EVC counters */
    {
        vtss_port_evc_counters_t *evc = &counters->evc;

        for (i = 0; i < VTSS_PRIOS; i++) {
            evc->rx_green[i] = c->rx_green_class[i].value;
            evc->rx_yellow[i] = c->rx_yellow_class[i].value;
            evc->rx_red[i] = c->rx_red_class[i].value;
            evc->rx_green_discard[i] = c->dr_green_class[i].value;
            evc->rx_yellow_discard[i] = c->dr_yellow_class[i].value;
            evc->tx_green[i] = c->tx_green_class[i].value;
            evc->tx_yellow[i] = c->tx_yellow_class[i].value;
        }
    }
#endif /* VTSS_ARCH_CARACAL */

    return VTSS_RC_OK;
}

/* Get Rx and Tx packets */
static vtss_rc l26_port_basic_counters_get(const vtss_port_no_t port_no,
                                           vtss_basic_counters_t *const counters)
{
    u32                          base, *p = &base, port = VTSS_CHIP_PORT(port_no);
    vtss_port_luton26_counters_t *c = &vtss_state->port_counters[port_no].counter.luton26;

    /* Rx Counters */
    base = 0x008; /* rx_64 */
    VTSS_RC(l26_port_counter(p, port, &c->rx_64, 0));
    VTSS_RC(l26_port_counter(p, port, &c->rx_65_127, 0));
    VTSS_RC(l26_port_counter(p, port, &c->rx_128_255, 0));
    VTSS_RC(l26_port_counter(p, port, &c->rx_256_511, 0));
    VTSS_RC(l26_port_counter(p, port, &c->rx_512_1023, 0));
    VTSS_RC(l26_port_counter(p, port, &c->rx_1024_1526, 0));
    VTSS_RC(l26_port_counter(p, port, &c->rx_1527_max, 0));

    /* Tx Counters */
    base = 0x807; /* tx_64 */
    VTSS_RC(l26_port_counter(p, port, &c->tx_64, 0));
    VTSS_RC(l26_port_counter(p, port, &c->tx_65_127, 0));
    VTSS_RC(l26_port_counter(p, port, &c->tx_128_255, 0));
    VTSS_RC(l26_port_counter(p, port, &c->tx_256_511, 0));
    VTSS_RC(l26_port_counter(p, port, &c->tx_512_1023, 0));
    VTSS_RC(l26_port_counter(p, port, &c->tx_1024_1526, 0));
    VTSS_RC(l26_port_counter(p, port, &c->tx_1527_max, 0));

    /* Rx frames */
    counters->rx_frames = (c->rx_64.value + c->rx_65_127.value + c->rx_128_255.value + c->rx_256_511.value + 
                           c->rx_512_1023.value + c->rx_1024_1526.value + c->rx_1527_max.value);

    /* Tx frames */
    counters->tx_frames = (c->tx_64.value + c->tx_65_127.value + c->tx_128_255.value + c->tx_256_511.value + 
                           c->tx_512_1023.value + c->tx_1024_1526.value + c->tx_1527_max.value);
    return VTSS_RC_OK;
}


static vtss_rc l26_port_counters_cmd(const vtss_port_no_t        port_no, 
                                     vtss_port_counters_t *const counters, 
                                     BOOL                        clear)
{
    return l26_port_counters_read(VTSS_CHIP_PORT(port_no),
                                  &vtss_state->port_counters[port_no].counter.luton26,
                                  counters,
                                  clear);
}

static vtss_rc l26_port_counters_update(const vtss_port_no_t port_no)
{
    return l26_port_counters_cmd(port_no, NULL, 0);
}

static vtss_rc l26_port_counters_clear(const vtss_port_no_t port_no)
{
    return l26_port_counters_cmd(port_no, NULL, 1);
}

static vtss_rc l26_port_counters_get(const vtss_port_no_t port_no,
                                     vtss_port_counters_t *const counters)
{
    memset(counters, 0, sizeof(*counters)); 
    return l26_port_counters_cmd(port_no, counters, 0);
}

/* ================================================================= *
 *  QoS
 * ================================================================= */

typedef struct {
    BOOL frame_rate; /* Enable frame rate policing (always single bucket) */
    BOOL dual;       /* Enable dual leaky bucket mode */
    u32  cir;        /* CIR in kbps/fps (ignored in single bucket mode) */
    u32  cbs;        /* CBS in bytes/frames (ignored in single bucket mode) */
    u32  eir;        /* EIR (PIR) in kbps/fps */
    u32  ebs;        /* EBS (PBS) in bytes/frames */
    BOOL cf;         /* Coupling flag (ignored in single bucket mode) */
} vtss_l26_policer_conf_t;

/* 
   Configure one of the shared 256 policers.
   A policer can be configured if it is free or owned by the same user.
   A special case is when the policer is owned by ACL or EVC and another user wants to allocate it.
   In this case the ACL or the EVE subsystem is asked to use another policer.
   If this operation succeeds, the policer is free and can be allocated as normal.

   This function is reentrant and can be used directly inside the ACL/EVC move function
   in order to deallocate the policer.
*/
static vtss_rc l26_policer_conf_set(vtss_policer_user_t user, u32 policer, BOOL enable, vtss_l26_policer_conf_t *conf)
{
    vtss_policer_user_t owner;
    u32 cir = 0;
    u32 cbs = 0;
    u32 pir = 0;
    u32 pbs = 0;
    u32 mode = POL_MODE_LINERATE;
    u32 cir_ena = 0;
    u32 cf = 0;
    u32 value;
    
    if ((user == VTSS_POLICER_USER_NONE) || (user >= VTSS_POLICER_USER_CNT)) {
        VTSS_E("illegal user: %d", user);
        return VTSS_RC_ERROR;
    }

    if (policer >= VTSS_L26_POLICER_CNT) {
        VTSS_E("illegal policer: %u", policer);
        return VTSS_RC_ERROR;
    }

    owner = vtss_state->policer_user[policer];

    if (enable) {
        if (conf == NULL) {
            VTSS_E("conf == NULL");
            return VTSS_RC_ERROR;
        }

        /* Verify user versus owner */
        if (owner != user) {
            /* Policer is occupied by someone else */
            switch (owner) {
            case VTSS_POLICER_USER_NONE:
                /* Free entry, no problem */
                break;
            case VTSS_POLICER_USER_ACL:
                if (l26_acl_policer_move(policer) != VTSS_RC_OK) {
                    VTSS_I("unable to move ACL policer %u", policer);
                    return VTSS_RC_ERROR;
                }
                break;
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
            case VTSS_POLICER_USER_EVC:
                if (l26_evc_policer_move(policer) != VTSS_RC_OK) {
                    VTSS_I("unable to move EVC policer %u", policer);
                    return VTSS_RC_ERROR;
                } 
                break;
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
            default:
                VTSS_I("Unmovable policer %u occupied by user %d", policer, owner);
                return VTSS_RC_ERROR;
            }
        }

        /* Calculate register fields */
        pir = conf->eir;
        pbs = conf->ebs;
        if (conf->frame_rate) {
            /* Frame rate policing (single leaky bucket) */
            if (pir >= 100) {
                mode = POL_MODE_FRMRATE_100FPS;
                pir  = (pir + 99) / 100; /* Resolution is in steps of 100 fps */
                pbs  = (pbs * 10 / 328); /* Burst unit is 32.8 frames */
                pbs++; /* Round up burst size */
                pbs  = MIN(VTSS_BITMASK(6),  pbs); /* Limit burst to the maximum value */
            } else {
                mode = POL_MODE_FRMRATE_1FPS;
                pbs  = (pbs * 10 / 3); /* Burst unit is 0.3 frames */
                pbs++; /* Round up burst size */
                pbs  = MIN(60, pbs); /* See Bugzilla#4944, comment#2 */
            }
        } else {
            /* Bit rate policing */
            if (conf->dual) {
                /* Dual leaky bucket mode */
                cir = (conf->cir + 99) / 100;  /* Rate unit is 100 kbps. Round up to next possible value */
                cbs = (conf->cbs ? conf->cbs : 1); /* BZ 9813: Avoid using zero burst size */
                cbs = (cbs + 4095) / 4096; /* Burst unit is 4kB. Round up to next possible value */
                cbs  = MIN(61, cbs); /* See Bugzilla#4944, comment#2  */
                cir_ena = 1;
                cf = conf->cf;
                if (cf)
                    pir += conf->cir;
            }
            pir  = (pir + 99) / 100;  /* Rate unit is 100 kbps. Round up to next possible value */
            pbs = (pbs ? pbs : 1);    /* BZ 9813: Avoid using zero burst size */
            pbs  = (pbs + 4095) / 4096; /* Burst unit is 4kB. Round up to next possible value */
            pbs  = MIN(61, pbs); /* See Bugzilla#4944, comment#2  */
        }
    
        /* Limit rate to the maximum value */
        pir  = MIN(VTSS_BITMASK(15), pir);
        cir  = MIN(VTSS_BITMASK(15), cir);

        /* Set current user */
        vtss_state->policer_user[policer] = user;
    } else { /* Disable policer */
        /* Verify user versus owner */
        if (owner != user) {
            /* Silently ignore if a user (e.g. QoS) disables a policer it has not allocated */
            return VTSS_RC_OK;
        }
        /* Clear current user */
        vtss_state->policer_user[policer] = VTSS_POLICER_USER_NONE;
    }
    
    /* Begin of work around part 1 for Bugzilla#3253 Comment#2:
     * ==================================================================
     * FYI;
     * It was not possible to make an ECO directly accessing the bit, so:
     * To set overshoot_ena, set cir_ena=0, dlb_coupled=1
     * To clr overshoot_ena, set cir_ena=0, dlb_coupled=0
     * To keep overshoot_ena, set cir_ena=1, dlb_coupled=X
     *
     * To get overshoot_ena, read the field as for any other field.
     * ==================================================================
     *
     * The following code enables OVERSHOOT_ENA
     */
    value = (VTSS_F_SYS_POL_POL_MODE_CFG_IPG_SIZE(20) |
             VTSS_F_SYS_POL_POL_MODE_CFG_FRM_MODE(mode) |
             VTSS_F_SYS_POL_POL_MODE_CFG_DLB_COUPLED |
             VTSS_F_SYS_POL_POL_MODE_CFG_OVERSHOOT_ENA);
    L26_WR(VTSS_SYS_POL_POL_MODE_CFG(policer), value);
    /* End of work around part 1 for Bugzilla#3253 Comment#2 */

    /* Setup policer registers */
    value = (VTSS_F_SYS_POL_POL_MODE_CFG_IPG_SIZE(20) |
             VTSS_F_SYS_POL_POL_MODE_CFG_FRM_MODE(mode) |
             VTSS_F_SYS_POL_POL_MODE_CFG_OVERSHOOT_ENA);
    if (cir_ena) {
        value |= VTSS_F_SYS_POL_POL_MODE_CFG_CIR_ENA;
        if (cf) {
            value |= VTSS_F_SYS_POL_POL_MODE_CFG_DLB_COUPLED;
        }
    } else {
        /* Begin of work around part 2 for Bugzilla#3253 Comment#2: */
        value |= VTSS_F_SYS_POL_POL_MODE_CFG_DLB_COUPLED; /* DBL_COUPLED controls OVERSHOOT_ENA when CIR_ENA is 0. */
        /* End of work around part 2 for Bugzilla#3253 Comment#2: */
    }
    L26_WR(VTSS_SYS_POL_POL_MODE_CFG(policer), value);
    L26_WR(VTSS_SYS_POL_POL_PIR_CFG(policer), 
           VTSS_F_SYS_POL_POL_PIR_CFG_PIR_RATE(pir) |
           VTSS_F_SYS_POL_POL_PIR_CFG_PIR_BURST(pbs));
    L26_WR(VTSS_SYS_POL_POL_CIR_CFG(policer), 
           VTSS_F_SYS_POL_POL_CIR_CFG_CIR_RATE(cir) |
           VTSS_F_SYS_POL_POL_CIR_CFG_CIR_BURST(cbs));
    L26_WR(VTSS_SYS_POL_POL_PIR_STATE(policer), 0); /* Reset current fill level */
    L26_WR(VTSS_SYS_POL_POL_CIR_STATE(policer), 0); /* Reset current fill level */

    return VTSS_RC_OK;
}

/* Get first free policer */
static vtss_rc l26_policer_free_get(u16 *new)
{
    u16 policer;

    /* Look for free policer starting from above */
    for (policer = 0; policer < VTSS_L26_POLICER_CNT; policer++) {
        if (vtss_state->policer_user[policer] == VTSS_POLICER_USER_NONE) {
            *new = policer;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

#define L26_DEFAULT_POL_ORDER 0x1d3 /* Luton 26 policer order: Serial (QoS -> Port -> ACL) */

static vtss_rc l26_port_policer_fc_set(const vtss_port_no_t port_no, u32 chipport)
{
    vtss_port_conf_t *port_conf = &vtss_state->port_conf[port_no];
    vtss_qos_port_conf_t *qos_conf = &vtss_state->qos_port_conf[port_no];

    L26_WR(VTSS_SYS_POL_MISC_POL_FLOWC(chipport),
           (port_conf->flow_control.generate &&
            (qos_conf->policer_port[0].rate != VTSS_BITRATE_DISABLED) &&
            qos_conf->policer_ext_port[0].flow_control) ? 1 : 0);

    return VTSS_RC_OK;
}

static vtss_rc l26_port_policer_set(u32 port, BOOL enable, vtss_l26_policer_conf_t *conf)
{
    BOOL redir_8021 = FALSE;
    BOOL redir_ip   = FALSE;
    u32  order      = L26_DEFAULT_POL_ORDER;

    VTSS_RC(l26_policer_conf_set(VTSS_POLICER_USER_PORT, port, enable, conf));

    L26_WRM(VTSS_ANA_PORT_POL_CFG(port), 
            (redir_8021   ? VTSS_F_ANA_PORT_POL_CFG_POL_CPU_REDIR_8021 : 0) |
            (redir_ip     ? VTSS_F_ANA_PORT_POL_CFG_POL_CPU_REDIR_IP   : 0) |
            (enable       ? VTSS_F_ANA_PORT_POL_CFG_PORT_POL_ENA       : 0) |
            VTSS_F_ANA_PORT_POL_CFG_POL_ORDER(order),
            VTSS_F_ANA_PORT_POL_CFG_POL_CPU_REDIR_8021 |
            VTSS_F_ANA_PORT_POL_CFG_POL_CPU_REDIR_IP   |
            VTSS_F_ANA_PORT_POL_CFG_PORT_POL_ENA       |
            VTSS_M_ANA_PORT_POL_CFG_POL_ORDER);

    return VTSS_RC_OK;
}

static vtss_rc l26_queue_policer_set(u32 port, u32 queue, BOOL enable, vtss_l26_policer_conf_t *conf)
{
    VTSS_RC(l26_policer_conf_set(VTSS_POLICER_USER_QUEUE, 32 + port * 8 + queue, enable, conf));

    L26_WRM(VTSS_ANA_PORT_POL_CFG(port), 
            (enable       ? VTSS_F_ANA_PORT_POL_CFG_QUEUE_POL_ENA(VTSS_BIT(queue)): 0),
            VTSS_F_ANA_PORT_POL_CFG_QUEUE_POL_ENA(VTSS_BIT(queue)));

    return VTSS_RC_OK;
}

static u32 l26_packet_rate(vtss_packet_rate_t rate, u32 *unit)
{
    u32 value, max;

    /* If the rate is greater than 1000 pps, the unit is kpps */
    max = (rate >= 1000 ? (rate/1000) : rate);
    *unit = (rate >= 1000 ? 0 : 1);
    for (value = 15; value != 0; value--) {
        if (max >= (u32)(1<<value))
            break;
    }
    return value;
}

static u32 l26_chip_prio(const vtss_prio_t prio)
{
    if (prio >= L26_PRIOS) {
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

static vtss_rc l26_qos_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_qos_port_conf_t *conf = &vtss_state->qos_port_conf[port_no];
    u32 port = VTSS_CHIP_PORT(port_no);
    int pcp, dei, queue, class, dpl;
    u8 cost[6];
    vtss_l26_policer_conf_t pol_cfg;
    u32  tag_remark_mode;
    BOOL tag_default_dei;

    /* Port default PCP and DEI configuration */
    L26_WRM(VTSS_ANA_PORT_VLAN_CFG(port),
            (conf->default_dei ? VTSS_F_ANA_PORT_VLAN_CFG_VLAN_DEI : 0) |
            VTSS_F_ANA_PORT_VLAN_CFG_VLAN_PCP(conf->usr_prio),
            VTSS_F_ANA_PORT_VLAN_CFG_VLAN_DEI                           |
            VTSS_M_ANA_PORT_VLAN_CFG_VLAN_PCP);
    
    /* Port default QoS class, DP level, tagged frames mode, DSCP mode and DSCP remarking configuration */
    L26_WR(VTSS_ANA_PORT_QOS_CFG(port),
           (conf->default_dpl ? VTSS_F_ANA_PORT_QOS_CFG_DP_DEFAULT_VAL : 0)           |
           VTSS_F_ANA_PORT_QOS_CFG_QOS_DEFAULT_VAL(l26_chip_prio(conf->default_prio)) |
           (conf->tag_class_enable ? VTSS_F_ANA_PORT_QOS_CFG_QOS_PCP_ENA : 0)         |
           (conf->dscp_class_enable ? VTSS_F_ANA_PORT_QOS_CFG_QOS_DSCP_ENA : 0)       |
           (conf->dscp_translate ? VTSS_F_ANA_PORT_QOS_CFG_DSCP_TRANSLATE_ENA : 0)    |
           VTSS_F_ANA_PORT_QOS_CFG_DSCP_REWR_CFG(conf->dscp_mode));

    /* Egress DSCP remarking configuration */
    L26_WR(VTSS_REW_PORT_DSCP_CFG(port), VTSS_F_REW_PORT_DSCP_CFG_DSCP_REWR_CFG(conf->dscp_emode));

    /* Map for (PCP and DEI) to (QoS class and DP level */
    for (pcp = VTSS_PCP_START; pcp < VTSS_PCP_END; pcp++) {
        for (dei = VTSS_DEI_START; dei < VTSS_DEI_END; dei++) {
            L26_WR(VTSS_ANA_PORT_QOS_PCP_DEI_MAP_CFG(port, (8*dei + pcp)),
                   (conf->dp_level_map[pcp][dei] ? VTSS_F_ANA_PORT_QOS_PCP_DEI_MAP_CFG_DP_PCP_DEI_VAL : 0) |
                   VTSS_F_ANA_PORT_QOS_PCP_DEI_MAP_CFG_QOS_PCP_DEI_VAL(l26_chip_prio(conf->qos_class_map[pcp][dei])));
        }
    }

    /* Enable gap value adjustment */
    L26_WR(VTSS_SYS_SCH_LB_DWRR_CFG(port), VTSS_F_SYS_SCH_LB_DWRR_CFG_FRM_ADJ_ENA);

    /* DWRR configuration */
    VTSS_RC(vtss_cmn_qos_weight2cost(conf->queue_pct, cost, 6, 5));
    L26_WR(VTSS_SYS_SCH_SCH_DWRR_CFG(port),
           (conf->dwrr_enable ? VTSS_F_SYS_SCH_SCH_DWRR_CFG_DWRR_MODE : 0) |
           VTSS_X_SYS_SCH_SCH_DWRR_CFG_COST_CFG(
               VTSS_ENCODE_BITFIELD(cost[0],  0, 5) |
               VTSS_ENCODE_BITFIELD(cost[1],  5, 5) |
               VTSS_ENCODE_BITFIELD(cost[2], 10, 5) |
               VTSS_ENCODE_BITFIELD(cost[3], 15, 5) |
               VTSS_ENCODE_BITFIELD(cost[4], 20, 5) |
               VTSS_ENCODE_BITFIELD(cost[5], 25, 5)));

    /* Egress port shaper burst level configuration
     * The value is rounded up to the next possible value:
     *           0 -> 0 (Always closed)
     *    1.. 4096 -> 1 ( 4 KB)
     * 4097.. 8192 -> 2 ( 8 KB)
     * 8193..12288 -> 3 (12 KB)
     */
    L26_WR(VTSS_SYS_SCH_LB_LB_THRES(((9 * port) + 8)),
           VTSS_F_SYS_SCH_LB_LB_THRES_LB_THRES(MIN(0x3f, ((conf->shaper_port.level + 4095) / 4096))));

    /* Egress port shaper rate configuration
     * The value (in kbps) is rounded up to the next possible value:
     *        0 -> 0 (Open until burst capacity is used, then closed)
     *   1..100 -> 1 (100160 bps)
     * 101..200 -> 2 (200320 bps)
     * 201..300 -> 3 (300480 bps)
     */
    L26_WR(VTSS_SYS_SCH_LB_LB_RATE(((9 * port) + 8)),
           VTSS_F_SYS_SCH_LB_LB_RATE_LB_RATE(MIN(0x7fff, VTSS_DIV64(((u64)conf->shaper_port.rate * 1000) + 100159, 100160))));

    /* Egress queue shaper rate and burst level configuration. See documentation above */
    for (queue = 0; queue < 8; queue++) {
        L26_WR(VTSS_SYS_SCH_LB_LB_THRES(((9 * port) + queue)),
               VTSS_F_SYS_SCH_LB_LB_THRES_LB_THRES(MIN(0x3f, ((conf->shaper_queue[queue].level + 4095) / 4096))));

        L26_WR(VTSS_SYS_SCH_LB_LB_RATE(((9 * port) + queue)),
               VTSS_F_SYS_SCH_LB_LB_RATE_LB_RATE(MIN(0x7fff, VTSS_DIV64(((u64)conf->shaper_queue[queue].rate * 1000) + 100159, 100160))));
    }

    /* Egress port and queue shaper enable/disable configuration */
    L26_WR(VTSS_SYS_SCH_SCH_SHAPING_CTRL(port),
           VTSS_F_SYS_SCH_SCH_SHAPING_CTRL_PRIO_SHAPING_ENA(
               (conf->shaper_queue[0].rate != VTSS_BITRATE_DISABLED ? VTSS_BIT(0) : 0) |
               (conf->shaper_queue[1].rate != VTSS_BITRATE_DISABLED ? VTSS_BIT(1) : 0) |
               (conf->shaper_queue[2].rate != VTSS_BITRATE_DISABLED ? VTSS_BIT(2) : 0) |
               (conf->shaper_queue[3].rate != VTSS_BITRATE_DISABLED ? VTSS_BIT(3) : 0) |
               (conf->shaper_queue[4].rate != VTSS_BITRATE_DISABLED ? VTSS_BIT(4) : 0) |
               (conf->shaper_queue[5].rate != VTSS_BITRATE_DISABLED ? VTSS_BIT(5) : 0) |
               (conf->shaper_queue[6].rate != VTSS_BITRATE_DISABLED ? VTSS_BIT(6) : 0) |
               (conf->shaper_queue[7].rate != VTSS_BITRATE_DISABLED ? VTSS_BIT(7) : 0)) |
           (conf->shaper_port.rate != VTSS_BITRATE_DISABLED ? VTSS_F_SYS_SCH_SCH_SHAPING_CTRL_PORT_SHAPING_ENA : 0) |
           VTSS_F_SYS_SCH_SCH_SHAPING_CTRL_PRIO_LB_EXS_ENA(
               (conf->excess_enable[0] ? VTSS_BIT(0) : 0) |
               (conf->excess_enable[1] ? VTSS_BIT(1) : 0) |
               (conf->excess_enable[2] ? VTSS_BIT(2) : 0) |
               (conf->excess_enable[3] ? VTSS_BIT(3) : 0) |
               (conf->excess_enable[4] ? VTSS_BIT(4) : 0) |
               (conf->excess_enable[5] ? VTSS_BIT(5) : 0) |
               (conf->excess_enable[6] ? VTSS_BIT(6) : 0) |
               (conf->excess_enable[7] ? VTSS_BIT(7) : 0)));

    tag_remark_mode = conf->tag_remark_mode;
    tag_default_dei = (tag_remark_mode == VTSS_TAG_REMARK_MODE_DEFAULT ? 
                       conf->tag_default_dei : 0);
#if defined(VTSS_ARCH_CARACAL)
    /* Enable DEI colouring for NNI ports */
    if (vtss_state->evc_port_info[port_no].nni_count) {
        /* Allow only CLASSIFIED and MAPPED modes */
        if (tag_remark_mode == VTSS_TAG_REMARK_MODE_DEFAULT)
            tag_remark_mode = VTSS_TAG_REMARK_MODE_CLASSIFIED;
        
        /* DEI colouring is enabled using default DEI field (requires revision B) */
        tag_default_dei = vtss_state->evc_port_conf[port_no].dei_colouring;
    }
#endif /* VTSS_ARCH_CARACAL */    

    /* Tag remarking configuration */
    L26_WRM(VTSS_REW_PORT_PORT_VLAN_CFG(port),
            (tag_default_dei ? VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_DEI : 0) |
            VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_PCP(conf->tag_default_pcp),
            VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_DEI                               |
            VTSS_M_REW_PORT_PORT_VLAN_CFG_PORT_PCP);
    
    L26_WRM(VTSS_REW_PORT_TAG_CFG(port),
            VTSS_F_REW_PORT_TAG_CFG_TAG_QOS_CFG(tag_remark_mode),
            VTSS_M_REW_PORT_TAG_CFG_TAG_QOS_CFG);

    /* Map for (QoS class and DP level) to (PCP and DEI) */
    for (class = VTSS_QUEUE_START; class < VTSS_QUEUE_END; class++) {
        for (dpl = 0; dpl < 2; dpl++) {
            L26_WRM(VTSS_REW_PORT_PCP_DEI_QOS_MAP_CFG(port, (8*dpl + class)),
                    (conf->tag_dei_map[class][dpl] ? VTSS_F_REW_PORT_PCP_DEI_QOS_MAP_CFG_DEI_QOS_VAL : 0) |
                    VTSS_F_REW_PORT_PCP_DEI_QOS_MAP_CFG_PCP_QOS_VAL(conf->tag_pcp_map[class][dpl]),
                    VTSS_F_REW_PORT_PCP_DEI_QOS_MAP_CFG_DEI_QOS_VAL                                       |
                    VTSS_M_REW_PORT_PCP_DEI_QOS_MAP_CFG_PCP_QOS_VAL);
        }
    }

    /* Port policer configuration */
    memset(&pol_cfg, 0, sizeof(pol_cfg));
    if (conf->policer_port[0].rate != VTSS_BITRATE_DISABLED) {
        pol_cfg.frame_rate = conf->policer_ext_port[0].frame_rate;
        pol_cfg.eir = conf->policer_port[0].rate;
        pol_cfg.ebs = pol_cfg.frame_rate ? 64 : conf->policer_port[0].level; /* If frame_rate we always use 64 frames as burst value */
    }
    VTSS_RC(l26_port_policer_set(port, conf->policer_port[0].rate != VTSS_BITRATE_DISABLED, &pol_cfg));

    /* Queue policer configuration */
    for (queue = 0; queue < 8; queue++) {
        memset(&pol_cfg, 0, sizeof(pol_cfg));
        if (conf->policer_queue[queue].rate != VTSS_BITRATE_DISABLED) {
            pol_cfg.eir = conf->policer_queue[queue].rate;
            pol_cfg.ebs = conf->policer_queue[queue].level;
        }
        VTSS_RC(l26_queue_policer_set(port, queue, conf->policer_queue[queue].rate != VTSS_BITRATE_DISABLED, &pol_cfg));
    }

    /* Update policer flow control configuration */
    VTSS_RC(l26_port_policer_fc_set(port_no, port));

    return VTSS_RC_OK;
}

static vtss_rc l26_qos_conf_set(BOOL changed)
{
    vtss_qos_conf_t    *conf = &vtss_state->qos_conf;
    vtss_port_no_t     port_no;
    u32                i, unit;
    vtss_packet_rate_t rate;
        
    if (changed) {
        /* Number of priorities changed, update QoS setup for all ports */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            VTSS_RC(l26_qos_port_conf_set(port_no));   
        }
    }
    /* Storm control */
    L26_WR(VTSS_ANA_ANA_STORMLIMIT_BURST, VTSS_F_ANA_ANA_STORMLIMIT_BURST_STORM_BURST(6)); /* Burst of 64 frames allowed */
    for (i = 0; i < 3; i++) {
        rate = (i == 0 ? conf->policer_uc : i == 1 ? conf->policer_bc : conf->policer_mc);
        L26_WR(VTSS_ANA_ANA_STORMLIMIT_CFG(i),
               VTSS_F_ANA_ANA_STORMLIMIT_CFG_STORM_RATE(l26_packet_rate(rate, &unit)) |
               (unit ? VTSS_F_ANA_ANA_STORMLIMIT_CFG_STORM_UNIT : 0) |
               VTSS_F_ANA_ANA_STORMLIMIT_CFG_STORM_MODE(rate == VTSS_PACKET_RATE_DISABLED ? 0 : 3)); /* Disabled or police both CPU and front port destinations */
    }

    /* Frame adjustment (gap value - number of bytes added in leaky buckets and DWRR calculations) */
    L26_WR(VTSS_SYS_SCH_LB_DWRR_FRM_ADJ, VTSS_F_SYS_SCH_LB_DWRR_FRM_ADJ_FRM_ADJ(20)); /* 20 bytes added */

    /* DSCP classification and remarking configuration
     */
    for (i = 0; i < 64; i++) {
        L26_WR(VTSS_ANA_COMMON_DSCP_CFG(i),
               (conf->dscp_dp_level_map[i] ? VTSS_F_ANA_COMMON_DSCP_CFG_DP_DSCP_VAL : 0)           |
               VTSS_F_ANA_COMMON_DSCP_CFG_QOS_DSCP_VAL(l26_chip_prio(conf->dscp_qos_class_map[i])) |
               VTSS_F_ANA_COMMON_DSCP_CFG_DSCP_TRANSLATE_VAL(conf->dscp_translate_map[i])          |
               (conf->dscp_trust[i] ? VTSS_F_ANA_COMMON_DSCP_CFG_DSCP_TRUST_ENA : 0)               |
               (conf->dscp_remark[i] ? VTSS_F_ANA_COMMON_DSCP_CFG_DSCP_REWR_ENA : 0));

        L26_WR(VTSS_REW_COMMON_DSCP_REMAP_CFG(i),
               VTSS_F_REW_COMMON_DSCP_REMAP_CFG_DSCP_REMAP_VAL(conf->dscp_remap[i]));

        L26_WR(VTSS_REW_COMMON_DSCP_REMAP_DP1_CFG(i),
               VTSS_F_REW_COMMON_DSCP_REMAP_DP1_CFG_DSCP_REMAP_DP1_VAL(conf->dscp_remap_dp1[i]));
    }

    /* DSCP classification from QoS configuration
     */
    for (i = 0; i < 8; i++) {
        L26_WR(VTSS_ANA_COMMON_DSCP_REWR_CFG(i),
               VTSS_F_ANA_COMMON_DSCP_REWR_CFG_DSCP_QOS_REWR_VAL(conf->dscp_qos_map[i]));

        L26_WR(VTSS_ANA_COMMON_DSCP_REWR_CFG(i + 8),
               VTSS_F_ANA_COMMON_DSCP_REWR_CFG_DSCP_QOS_REWR_VAL(conf->dscp_qos_map_dp1[i]));
    }

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Layer 2
 * ================================================================= */

/* Set learn mask */
static vtss_rc l26_learn_state_set(const BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_port_no_t port_no;
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        L26_WRM_CTL(VTSS_ANA_PORT_PORT_CFG(VTSS_CHIP_PORT(port_no)), 
                    member[port_no], VTSS_F_ANA_PORT_PORT_CFG_LEARN_ENA);
    }
    return VTSS_RC_OK;
}

/* Set forwarding state (already done by vtss_update_masks) */
static vtss_rc l26_port_forward_set(const vtss_port_no_t port_no)
{
    return VTSS_RC_OK;
}

/* Wait until the MAC operation is finsished */
static vtss_rc l26_mac_table_idle(void)
{
    u32 cmd;    

    do {
        L26_RD(VTSS_ANA_ANA_TABLES_MACACCESS, &cmd);        
    } while (VTSS_X_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(cmd) != MAC_CMD_IDLE);

    return VTSS_RC_OK;
}

/* Add a given MAC entry */
static vtss_rc l26_mac_table_add(const vtss_mac_table_entry_t *const entry, u32 pgid)
{
    u32 mach, macl, mask, idx, aged = 0, fwd_kill = 0, ipv6_mask = 0, type;
    BOOL copy_to_cpu = FALSE;
    
    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    
    if (pgid == VTSS_PGID_NONE) {
        /* IPv4/IPv6 multicast entry */

        /* IP multicast entries use the MAC table to get frames to the CPU, whereas
           other entries use the PGID table, which makes it possible to direct to
           different CPU queues. */
        copy_to_cpu = entry->copy_to_cpu;

        mask = l26_port_mask(vtss_state->pgid_table[pgid].member); 
        if (entry->vid_mac.mac.addr[0] == 0x01) {
            /* IPv4 multicast entry */
            type = MAC_TYPE_IPV4_MC;
            /* Encode port mask directly */
            macl = ((macl & 0x00FFFFFF) | ((mask<<24) & 0xFF000000));
            mach = ((mach & 0xFFFF0000) | ((mask>>8) & 0x0000FFFF));
            idx = ((mask>>24) & 0x3); /* Ports 24-25 */

        } else {
            /* IPv6 multicast entry */
            type = MAC_TYPE_IPV6_MC;
            /* Encode port mask directly */
            mach = ((mach & 0xFFFF0000) | (mask & 0x0000FFFF)); /* ports 0-15  */
            idx = ((mask>>16) & 0x3F);                          /* ports 16-21 */
            ipv6_mask = ((mask>>22) & 0x3);                     /* ports 22-24 */
            aged = ((mask>>25) & 1);                            /* port 25     */

        }
    } else {
        /* Not IP multicast entry */
        /* Set FWD_KILL to make the switch discard frames in SMAC lookup */
        fwd_kill = (entry->copy_to_cpu || pgid != vtss_state->pgid_drop ? 0 : 1);
        idx = l26_chip_pgid(pgid);
        type = (entry->locked ? MAC_TYPE_LOCKED : MAC_TYPE_NORMAL);
        aged = 0;
    }

    /* Insert/learn new entry into the MAC table  */
    L26_WR(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
    L26_WR(VTSS_ANA_ANA_TABLES_MACLDATA, macl);    

    L26_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
           VTSS_F_ANA_ANA_TABLES_MACACCESS_IP6_MASK(ipv6_mask)                     |
           (copy_to_cpu        ? VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_CPU_COPY : 0) |
           (fwd_kill           ? VTSS_F_ANA_ANA_TABLES_MACACCESS_SRC_KILL     : 0) |
           (aged               ? VTSS_F_ANA_ANA_TABLES_MACACCESS_AGED_FLAG    : 0) |
           VTSS_F_ANA_ANA_TABLES_MACACCESS_VALID                                   |
           VTSS_F_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(type)                        |
           VTSS_F_ANA_ANA_TABLES_MACACCESS_DEST_IDX(idx)                           |
           VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_LEARN));

    /* Wait until MAC operation is finished */
    return l26_mac_table_idle();
}

static u32 l26_mac_type(const vtss_vid_mac_t *vid_mac)
{
    return (VTSS_MAC_IPV4_MC(vid_mac->mac.addr) ? MAC_TYPE_IPV4_MC :
            VTSS_MAC_IPV6_MC(vid_mac->mac.addr) ? MAC_TYPE_IPV6_MC : MAC_TYPE_NORMAL);
}

/* Delete/unlearn a given MAC entry */
static vtss_rc l26_mac_table_del(const vtss_vid_mac_t *const vid_mac)
{
    u32 mach, macl;
    
    vtss_mach_macl_get(vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    
    /* Delete/unlearn the given MAC entry */
    L26_WR(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
    L26_WR(VTSS_ANA_ANA_TABLES_MACLDATA, macl);    
    L26_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
           VTSS_F_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(l26_mac_type(vid_mac)) |
           VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_FORGET));
    
    /* Wait until MAC operation is finished */
    return l26_mac_table_idle();
}

/* Analyze the result in the MAC table */
static vtss_rc l26_mac_table_result(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32               value, mask, mach, macl, idx, type, aged;
    vtss_port_no_t    port_no;
    vtss_pgid_entry_t *pgid_entry;
    u8                *mac = &entry->vid_mac.mac.addr[0];
    
    L26_RD(VTSS_ANA_ANA_TABLES_MACACCESS, &value);        

    /* Check if entry is valid */
    if (!(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_VALID)) {
        VTSS_D("not valid");
        return VTSS_RC_ERROR;
    }

    type               = VTSS_X_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(value);
    idx                = VTSS_X_ANA_ANA_TABLES_MACACCESS_DEST_IDX(value); 
    aged = VTSS_BOOL(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_AGED_FLAG);
    entry->aged        = aged;
    entry->copy_to_cpu = VTSS_BOOL(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_CPU_COPY);
    entry->locked      = (type == MAC_TYPE_NORMAL ? 0 : 1);

    L26_RD(VTSS_ANA_ANA_TABLES_MACHDATA, &mach);
    L26_RD(VTSS_ANA_ANA_TABLES_MACLDATA, &macl);

    if (type == MAC_TYPE_IPV4_MC || type == MAC_TYPE_IPV6_MC) {
        /* IPv4/IPv6 multicast address */
        *pgid = VTSS_PGID_NONE;

        /* Read encoded port mask and update address registers */
        if (type == MAC_TYPE_IPV6_MC) {
            /* IPv6 entry  */
            /* Portmask:  25               24-22                21-16         15-0    */
            mask = (aged<<25  | (((value>>16) & 0x7)<<22) | (idx<<16) | (mach & 0xffff));
            mach = ((mach & 0xffff0000) | 0x00003333);
        } else {
            /* IPv4 entry */
            /* Portmask:25-24        23-0  */
            mask = ((idx<<24) | ((mach<<8) & 0xFFFF00) | ((macl>>24) & 0xff));
            mach = ((mach & 0xffff0000) | 0x00000100);
            macl = ((macl & 0x00ffffff) | 0x5e000000);
        }    

        /* Convert port mask */
        pgid_entry = &vtss_state->pgid_table[*pgid];
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            pgid_entry->member[port_no] = VTSS_BOOL(mask & (1 << VTSS_CHIP_PORT(port_no)));
    } else {
        *pgid = l26_vtss_pgid(idx);
    }
    entry->vid_mac.vid = ((mach>>16) & 0xFFF);
    mac[0] = ((mach>>8)  & 0xff);
    mac[1] = ((mach>>0)  & 0xff);
    mac[2] = ((macl>>24) & 0xff);
    mac[3] = ((macl>>16) & 0xff);
    mac[4] = ((macl>>8)  & 0xff);
    mac[5] = ((macl>>0)  & 0xff);
    VTSS_D("found 0x%08x%08x", mach, macl);

    return VTSS_RC_OK;
}

static vtss_rc l26_mac_table_get(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32 mach, macl;

    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    L26_WR(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
    L26_WR(VTSS_ANA_ANA_TABLES_MACLDATA, macl);    

    /* Do a lookup */
    L26_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
           VTSS_F_ANA_ANA_TABLES_MACACCESS_VALID |
           VTSS_F_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(l26_mac_type(&entry->vid_mac)) |
           VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_READ));

    VTSS_RC(l26_mac_table_idle());
    return l26_mac_table_result(entry, pgid);
}

static vtss_rc l26_mac_table_get_next(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32 mach, macl;

    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);

    L26_WR(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
    L26_WR(VTSS_ANA_ANA_TABLES_MACLDATA, macl);    

    /* Do a get next lookup */
    L26_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
           VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_GET_NEXT));

    VTSS_RC(l26_mac_table_idle());
    return l26_mac_table_result(entry, pgid);
}

/* Normal auto aging in seconds */
static vtss_rc l26_mac_table_age_time_set(void)
{
    u32 time;
    
    /* Scan two times per age time */
    time = (vtss_state->mac_age_time/2);
    if (time > 0xfffff)
        time = 0xfffff;
    L26_WR(VTSS_ANA_ANA_AUTOAGE, VTSS_F_ANA_ANA_AUTOAGE_AGE_PERIOD(time));
    return VTSS_RC_OK;
}

/* Selective aging based on port(s) and/or VLAN */
static vtss_rc l26_mac_table_age(BOOL             pgid_age, 
                                 u32              pgid,
                                 BOOL             vid_age,
                                 const vtss_vid_t vid)
{
    /* Selective aging */
    L26_WR(VTSS_ANA_ANA_ANAGEFIL, 
           (pgid_age ? VTSS_F_ANA_ANA_ANAGEFIL_PID_EN : 0) |
           (vid_age ? VTSS_F_ANA_ANA_ANAGEFIL_VID_EN  : 0) |
           VTSS_F_ANA_ANA_ANAGEFIL_PID_VAL(l26_chip_pgid(pgid)) |
           VTSS_F_ANA_ANA_ANAGEFIL_VID_VAL(vid));

    /* Do the aging */
    L26_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
           VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_TABLE_AGE));

    VTSS_RC(l26_mac_table_idle());

    /* Clear age filter again to avoid affecting automatic ageing */
    L26_WR(VTSS_ANA_ANA_ANAGEFIL, 0);

    return VTSS_RC_OK;
}

static vtss_rc l26_mac_table_status_get(vtss_mac_table_status_t *status) 
{
    u32 value;

    /* Read and clear sticky register */
    L26_RD(VTSS_ANA_ANA_ANEVENTS, &value);
    L26_WR(VTSS_ANA_ANA_ANEVENTS, value & 
          (VTSS_F_ANA_ANA_ANEVENTS_AUTO_MOVED   |
           VTSS_F_ANA_ANA_ANEVENTS_AUTO_LEARNED |
           VTSS_F_ANA_ANA_ANEVENTS_LEARN_REMOVE |
           VTSS_F_ANA_ANA_ANEVENTS_AGED_ENTRY));

   
    /* Detect learn events */
    status->learned = VTSS_BOOL(value & VTSS_F_ANA_ANA_ANEVENTS_AUTO_LEARNED);
    
    /* Detect replace events */
    status->replaced = VTSS_BOOL(value & VTSS_F_ANA_ANA_ANEVENTS_LEARN_REMOVE);

    /* Detect port move events */
    status->moved = VTSS_BOOL(value & (VTSS_F_ANA_ANA_ANEVENTS_AUTO_MOVED));
    
    /* Detect age events */
    status->aged = VTSS_BOOL(value & VTSS_F_ANA_ANA_ANEVENTS_AGED_ENTRY);

    return VTSS_RC_OK;
}

static vtss_rc l26_learn_mode_set(u32 port, vtss_learn_mode_t *mode)
{
    L26_WRM(VTSS_ANA_PORT_PORT_CFG(port), 
            (mode->discard   ? VTSS_F_ANA_PORT_PORT_CFG_LEARNDROP : 0) |
            (mode->automatic ? VTSS_F_ANA_PORT_PORT_CFG_LEARNAUTO : 0) |
            (mode->cpu       ? VTSS_F_ANA_PORT_PORT_CFG_LEARNCPU  : 0),
            (VTSS_F_ANA_PORT_PORT_CFG_LEARNDROP |
             VTSS_F_ANA_PORT_PORT_CFG_LEARNAUTO |
             VTSS_F_ANA_PORT_PORT_CFG_LEARNCPU));    
    return VTSS_RC_OK;
}

/* Learn mode: 'Learn frames dropped' / 'Autolearning' / 'Copy learn frame to CPU' */
static vtss_rc l26_learn_port_mode_set(const vtss_port_no_t port_no)
{
    vtss_learn_mode_t *mode = &vtss_state->learn_mode[port_no];
    u32               value, port = VTSS_CHIP_PORT(port_no);

    VTSS_RC(l26_learn_mode_set(port, mode));
       
    if (!mode->automatic) {
        /* Flush entries previously learned on port to avoid continuous refreshing */
        L26_RD(VTSS_ANA_PORT_PORT_CFG(port), &value);
        L26_WRM_CLR(VTSS_ANA_PORT_PORT_CFG(port), VTSS_F_ANA_PORT_PORT_CFG_LEARN_ENA);
        VTSS_RC(l26_mac_table_age(1, port_no, 0, 0));
        VTSS_RC(l26_mac_table_age(1, port_no, 0, 0));
        L26_WR(VTSS_ANA_PORT_PORT_CFG(port), value);
    }
    return VTSS_RC_OK;
}


/* ================================================================= *
 *  Mirror
 * ================================================================= */

/* Mirror port, where a selected traffic is copied to */
static vtss_rc l26_mirror_port_set(void)
{
    BOOL                   member[VTSS_PORT_ARRAY_SIZE];
    vtss_port_no_t         port_no;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        member[port_no] = (port_no == vtss_state->mirror_conf.port_no);
    }
    L26_WR(VTSS_ANA_ANA_MIRRORPORTS, l26_port_mask(member));    

    /* Update VLAN port configuration for all ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
    }

    /* Update all VLANs */
    return vtss_cmn_vlan_update_all();
}

/* Ingress ports subjects for mirroring */
static vtss_rc l26_mirror_ingress_set(void)
{
    vtss_port_no_t port_no;
    BOOL           enabled;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        enabled = vtss_state->mirror_ingress[port_no];
        L26_WRM(VTSS_ANA_PORT_PORT_CFG(VTSS_CHIP_PORT(port_no)), 
                (enabled ? VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA : 0), VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA);
    }
    return VTSS_RC_OK;
}

/* Egress ports subjects for mirroring */
static vtss_rc l26_mirror_egress_set(void)
{
    L26_WR(VTSS_ANA_ANA_EMIRRORPORTS, l26_port_mask(vtss_state->mirror_egress));
    return VTSS_RC_OK;
}

/* CPU Ingress ports subjects for mirroring */
static vtss_rc l26_mirror_cpu_ingress_set(void)
{
    BOOL           enabled = vtss_state->mirror_cpu_ingress;

    L26_WRM(VTSS_ANA_PORT_PORT_CFG(26), 
            (enabled ? VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA : 0), VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA); // CPU port is port 26, 6See Table 98 in data sheet
    return VTSS_RC_OK;
}

/* CPU Egress ports subjects for mirroring */
static vtss_rc l26_mirror_cpu_egress_set(void)
{
    BOOL           enabled = vtss_state->mirror_cpu_egress;

    L26_WRM(VTSS_ANA_ANA_AGENCTRL, enabled ? VTSS_F_ANA_ANA_AGENCTRL_MIRROR_CPU : 0 , VTSS_F_ANA_ANA_AGENCTRL_MIRROR_CPU);
    return VTSS_RC_OK;
}

/* ================================================================= *
 * NPI
 * ================================================================= */

#if defined(VTSS_FEATURE_NPI)
static vtss_rc l26_npi_mask_set(void)
{
    vtss_packet_rx_conf_t *conf = &vtss_state->rx_conf;

    if (vtss_state->npi_conf.enable) {
        u32 val, qmask, i;
        for (qmask = i = 0; i < vtss_state->rx_queue_count; i++) {
            if (conf->queue[i].npi.enable) {
                qmask |= VTSS_BIT(i); /* NPI redirect */
            }
        }
        val = VTSS_F_SYS_SYSTEM_EXT_CPU_CFG_EXT_CPU_PORT(VTSS_CHIP_PORT(vtss_state->npi_conf.port_no)) | VTSS_F_SYS_SYSTEM_EXT_CPU_CFG_EXT_CPUQ_MSK(qmask);
        L26_WR(VTSS_SYS_SYSTEM_EXT_CPU_CFG, val);
    } else {
        L26_WR(VTSS_SYS_SYSTEM_EXT_CPU_CFG, 0); /* No redirect by default */
    }

    return VTSS_RC_OK;
}

static vtss_rc l26_npi_update(void)
{
    VTSS_RC(l26_npi_mask_set());

    if (vtss_state->npi_conf.port_no != VTSS_PORT_NO_NONE) {
        /* Control IFH insertion */
        L26_WRM_CTL(VTSS_REW_PORT_PORT_CFG(VTSS_CHIP_PORT(vtss_state->npi_conf.port_no)), vtss_state->npi_conf.enable, VTSS_F_REW_PORT_PORT_CFG_IFH_INSERT_ENA);

        /* Control IFH parsing  */
        L26_WRM_CTL(VTSS_SYS_SYSTEM_PORT_MODE(VTSS_CHIP_PORT(vtss_state->npi_conf.port_no)), vtss_state->npi_conf.enable, VTSS_F_SYS_SYSTEM_PORT_MODE_INCL_INJ_HDR);
    }

    return VTSS_RC_OK;
}

static vtss_rc l26_npi_conf_set(const vtss_npi_conf_t *const conf)
{
    /* Disable current NPI port */
    vtss_state->npi_conf.enable = FALSE;
    VTSS_RC(l26_npi_update());

    /* Set new NPI port */
    vtss_state->npi_conf = *conf;
    if (!conf->enable) {
      vtss_state->npi_conf.port_no = VTSS_PORT_NO_NONE;
    }
    VTSS_RC(l26_npi_update());

    return vtss_cmn_vlan_update_all();
}

#endif /* VTSS_FEATURE_NPI */

/* ================================================================= *
 *  Initialization 
 * ================================================================= */

static vtss_rc l26_setup_cpu_if(const vtss_init_conf_t * const conf)
{
    const vtss_pi_conf_t *pi;
    u8                   b;
    u32                  value, wait;
    
    VTSS_D("enter");
    
    /* Setup PI width */
    pi = &conf->pi;
    switch (pi->width) {
    case VTSS_PI_WIDTH_8:  
        b = 0;
        break;
    case VTSS_PI_WIDTH_16: 
        b = VTSS_F_DEVCPU_PI_PI_PI_MODE_DATA_BUS_WID;
        break;
    default:
        VTSS_E("unknown pi->width");
        return VTSS_RC_ERROR;
    }

    /* Endianess */
    value = 0x01020304; /* Endianness test value used in next line */
    if (*((u8 *)&value) == 0x01)
        b |= VTSS_F_DEVCPU_PI_PI_PI_MODE_ENDIAN; /* Big endian */

    /* Mirror settings for all bytes */
    value = ((b<<24) | (b<<16) | (b<<8) | b);
    L26_WR(VTSS_DEVCPU_PI_PI_PI_MODE, value);
    
    /* PI bus cycle configuration */
    wait = pi->cs_wait_ns / 8; /* Wait unit is 8 nsec */
    if (wait > 15)
        wait = 15;
    value = VTSS_F_DEVCPU_PI_PI_PI_CFG_PI_WAIT(wait);
    if(pi->use_extended_bus_cycle)
        value |= VTSS_F_DEVCPU_PI_PI_PI_CFG_BUSY_FEEDBACK_ENA;
    L26_WR(VTSS_DEVCPU_PI_PI_PI_CFG, value);
    
    return VTSS_RC_OK;
}

static vtss_rc l26_vlan_conf_apply(BOOL ports)
{
    u32            etype = vtss_state->vlan_conf.s_etype;
    vtss_port_no_t port_no;

    /* BZ 4513: Type 0x8100 can not be used so we use 0x88a8 */
    if (etype == VTSS_ETYPE_TAG_C)
        etype = VTSS_ETYPE_TAG_S;

    L26_WR(VTSS_SYS_SYSTEM_VLAN_ETYPE_CFG, etype);
    
    /* Update ports */
    for (port_no = VTSS_PORT_NO_START; ports && port_no < vtss_state->port_count; port_no++) {
        VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
    }

    return VTSS_RC_OK;
}

static vtss_rc l26_vlan_conf_set(void)
{
    return l26_vlan_conf_apply(1);
}

static vtss_rc l26_vlan_port_conf_apply(u32                   port, 
                                        vtss_vlan_port_conf_t *conf)
{
    u32                   value, tpid = TAG_TPID_CFG_0x8100;
    u32                   etype = vtss_state->vlan_conf.s_etype;
    BOOL                  tagged, untagged, aware = 1, c_port = 0, s_port = 0;
    vtss_vlan_port_type_t type = conf->port_type;
    
    /* BZ4513: If the custom TPID is 0x8100, we treat S-custom ports as C-ports */
    if (etype == VTSS_ETYPE_TAG_C && type == VTSS_VLAN_PORT_TYPE_S_CUSTOM)
        type = VTSS_VLAN_PORT_TYPE_C;

    /* Check port type */
    switch (type) {
    case VTSS_VLAN_PORT_TYPE_UNAWARE:
        aware = 0;
        break;
    case VTSS_VLAN_PORT_TYPE_C:
        c_port = 1;
        break;
    case VTSS_VLAN_PORT_TYPE_S:
        s_port = 1;
        tpid = TAG_TPID_CFG_0x88A8;
        break;
    case VTSS_VLAN_PORT_TYPE_S_CUSTOM:
        s_port = 1;
        tpid = TAG_TPID_CFG_PTPID;
        break;
    default:
        return VTSS_RC_ERROR;
    }

    /* Port VLAN Configuration */
    value = VTSS_F_ANA_PORT_VLAN_CFG_VLAN_VID(conf->pvid); /* Port VLAN */
    if (aware) {
        /* VLAN aware, pop outer tag */
        value |= VTSS_F_ANA_PORT_VLAN_CFG_VLAN_AWARE_ENA; 
        value |= VTSS_F_ANA_PORT_VLAN_CFG_VLAN_POP_CNT(1);
    }
    L26_WRM(VTSS_ANA_PORT_VLAN_CFG(port), value,
            VTSS_M_ANA_PORT_VLAN_CFG_VLAN_VID |
            VTSS_F_ANA_PORT_VLAN_CFG_VLAN_AWARE_ENA |
            VTSS_M_ANA_PORT_VLAN_CFG_VLAN_POP_CNT);
    
    /* Drop Configuration based on port type and frame type */
    tagged = (conf->frame_type == VTSS_VLAN_FRAME_TAGGED);
    untagged = (conf->frame_type == VTSS_VLAN_FRAME_UNTAGGED);
    value = VTSS_F_ANA_PORT_DROP_CFG_DROP_MC_SMAC_ENA;
    if (tagged && aware) {
        /* Discard untagged and priority-tagged if aware and tagged-only allowed */
        value |= VTSS_F_ANA_PORT_DROP_CFG_DROP_UNTAGGED_ENA;
        value |= VTSS_F_ANA_PORT_DROP_CFG_DROP_PRIO_C_TAGGED_ENA;
        value |= VTSS_F_ANA_PORT_DROP_CFG_DROP_PRIO_S_TAGGED_ENA;
    }
    if ((untagged && c_port) || (tagged && s_port)) {
        /* Discard C-tagged if C-port and untagged-only OR S-port and tagged-only */
        value |= VTSS_F_ANA_PORT_DROP_CFG_DROP_C_TAGGED_ENA;
    }
    if ((untagged && s_port) || (tagged && c_port)) {
        /* Discard S-tagged if S-port and untagged-only OR C-port and tagged-only */
        value |= VTSS_F_ANA_PORT_DROP_CFG_DROP_S_TAGGED_ENA;
    }
    L26_WR(VTSS_ANA_PORT_DROP_CFG(port), value);

    /* Ingress filtering */
    L26_WRM(VTSS_ANA_ANA_VLANMASK, (conf->ingress_filter ? 1 : 0) << port, 1 << port);

    /* Rewriter VLAN tag configuration */
    value = (VTSS_F_REW_PORT_TAG_CFG_TAG_TPID_CFG(tpid) |
             VTSS_F_REW_PORT_TAG_CFG_TAG_VID_CFG |
             VTSS_F_REW_PORT_TAG_CFG_TAG_CFG(
                 conf->untagged_vid == VTSS_VID_ALL ? TAG_CFG_DISABLE :
                 conf->untagged_vid == VTSS_VID_NULL ? TAG_CFG_ALL : TAG_CFG_ALL_NPV_NNUL));
    L26_WRM(VTSS_REW_PORT_TAG_CFG(port), value, 
            VTSS_M_REW_PORT_TAG_CFG_TAG_TPID_CFG | 
            VTSS_F_REW_PORT_TAG_CFG_TAG_VID_CFG |
            VTSS_M_REW_PORT_TAG_CFG_TAG_CFG);
    L26_WRM(VTSS_REW_PORT_PORT_VLAN_CFG(port), 
            VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_TPID(etype) |
            VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_VID(conf->untagged_vid),
            VTSS_M_REW_PORT_PORT_VLAN_CFG_PORT_TPID |
            VTSS_M_REW_PORT_PORT_VLAN_CFG_PORT_VID);
    
    return VTSS_RC_OK;
}

static vtss_rc l26_vlan_port_conf_update(vtss_port_no_t port_no, vtss_vlan_port_conf_t *conf)
{
    /* Update maximum tags allowed */
    VTSS_RC(l26_port_max_tags_set(port_no));

    return l26_vlan_port_conf_apply(VTSS_CHIP_PORT(port_no), conf);
}

static vtss_rc l26_vlan_table_idle(void)
{
    u32 cmd;
    
    do {
        L26_RD(VTSS_ANA_ANA_TABLES_VLANACCESS, &cmd);
    } while (VTSS_X_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(cmd) != VLAN_CMD_IDLE);

    return VTSS_RC_OK;
}

static vtss_rc l26_vlan_mask_update(vtss_vid_t vid, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_vlan_entry_t *vlan_entry = &vtss_state->vlan_table[vid];
    u32               value;

    /* Index and properties */
    value = VTSS_F_ANA_ANA_TABLES_VLANTIDX_V_INDEX(vid);
    if(vlan_entry->isolated)
        value |= VTSS_F_ANA_ANA_TABLES_VLANTIDX_VLAN_PRIV_VLAN;
    if (vlan_entry->learn_disable)
        value |= VTSS_F_ANA_ANA_TABLES_VLANTIDX_VLAN_LEARN_DISABLED;
    L26_WR(VTSS_ANA_ANA_TABLES_VLANTIDX, value);

    /* VLAN mask */
    value = VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_PORT_MASK(l26_port_mask(member));
    value |= VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(VLAN_CMD_WRITE);
    L26_WR(VTSS_ANA_ANA_TABLES_VLANACCESS, value);

    /* Adjust IP multicast entries if neccessary */
    if (vlan_entry->enabled && vlan_entry->ipmc_used) {
        VTSS_RC(l26_ip_mc_fid_adjust(vid));
    }

    return l26_vlan_table_idle();
}

/* ================================================================= *
 *  VCL
 * ================================================================= */

static vtss_rc l26_vcl_port_conf_set(const vtss_port_no_t port_no)
{
    BOOL dmac_dip = vtss_state->vcl_port_conf[port_no].dmac_dip;
    u32  mask = VTSS_F_ANA_PORT_VCAP_CFG_S1_DMAC_DIP_ENA(1);
    u32  port = VTSS_CHIP_PORT(port_no);
    
#if defined(VTSS_ARCH_CARACAL)
    /* DMAC/DIP may be enabled by VCL API or EVC API */
    if (vtss_state->evc_port_conf[port_no].dmac_dip)
        dmac_dip = 1;
#endif /* VTSS_ARCH_CARACAL */

    /* Enable/disable DMAC/DIP match in first IS1 lookup */
    L26_WRM(VTSS_ANA_PORT_VCAP_CFG(port), dmac_dip ? mask : 0, mask);

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  PVLAN / Isolated ports
 * ================================================================= */

static vtss_rc l26_isolated_port_members_set(void)
{
    u32 mask = l26_port_mask(vtss_state->isolated_port);

    /* Isolated ports: cleared (add CPU as not isolated) */
    mask = ((~mask) & VTSS_CHIP_PORT_MASK) | VTSS_BIT(VTSS_CHIP_PORT_CPU);
    L26_WR(VTSS_ANA_ANA_ISOLATED_PORTS, mask);

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  ACL
 * ================================================================= */

static vtss_rc l26_acl_policer_set(const vtss_acl_policer_no_t policer_no)
{
    u32                     policer = (policer_no - VTSS_ACL_POLICER_NO_START);
    vtss_acl_policer_conf_t *conf = &vtss_state->acl_policer_conf[policer];
    vtss_policer_alloc_t    *pol_alloc = &vtss_state->acl_policer_alloc[policer];
    vtss_l26_policer_conf_t cfg;
    
    /* Only setup policer if allocated */
    if (pol_alloc->count == 0)
        return VTSS_RC_OK;

    memset(&cfg, 0, sizeof(cfg));
    if (conf->bit_rate_enable) {
        cfg.eir = conf->bit_rate;
        cfg.ebs = 4*1024; /* 4 kB burst size */
    } else {
        cfg.frame_rate = 1;
        cfg.eir = conf->rate;
    }
    
    return l26_policer_conf_set(VTSS_POLICER_USER_ACL, pol_alloc->policer, 1, &cfg);
}

static vtss_rc l26_is2_port_action_update(const vtss_port_no_t port_no)
{
    const tcam_props_t   *tcam = &tcam_info[VTSS_TCAM_S2];
    vtss_acl_port_conf_t *conf = &vtss_state->acl_port_conf[port_no];
    u32                  port = VTSS_CHIP_PORT(port_no);

    VTSS_RC(l26_is2_prepare_action(tcam, &conf->action, 0));
    return l26_vcap_port_command(tcam, port, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ACTION);
}

/* Move references to new policer */
static vtss_rc l26_acl_evc_policer_move(vtss_policer_user_t user, u32 policer_id,
                                        u16 policer_old, u16 policer_new)
{
    const tcam_props_t *tcam = &tcam_info[VTSS_TCAM_S2];
    u32                entry[VTSS_TCAM_ENTRY_WIDTH];
    vtss_port_no_t     port_no;
    vtss_vcap_obj_t    *obj = &vtss_state->is2.obj;
    vtss_vcap_entry_t  *cur;
    vtss_is2_data_t    *is2;
    u32                i;
    
    VTSS_I("user: %u, old: %u, new: %u", user, policer_old, policer_new);

    /* Update default port actions for all ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_RC(l26_is2_port_action_update(port_no));
    }

    /* Update IS2 entries using old policer */
    for (cur = obj->used, i = 0; cur != NULL; cur = cur->next, i++) {
        is2 = &cur->data.u.is2;
        if (cur->user == VTSS_IS2_USER_ACL && is2->policer_type != VTSS_L26_POLICER_NONE &&
            is2->policer == policer_id) {
            VTSS_I("move IS2 index %u", i);
            VTSS_RC(l26_vcap_index_command(tcam, i, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_ACTION));
            VTSS_RC(l26_vcap_cache2action(tcam, entry));
            vtss_bs_set(entry, 10, 8, policer_new);
            VTSS_RC(l26_vcap_action2cache(tcam, entry, 0));
            VTSS_RC(l26_vcap_index_command(tcam, i, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ACTION));
        }
    }

    /* Free old policer */
    return l26_policer_conf_set(user, policer_old, 0, NULL);
}

static vtss_rc l26_acl_policer_move(u32 policer)
{
    vtss_policer_alloc_t *pol_alloc;
    u32                  policer_id;
    
    VTSS_I("policer: %u", policer);

    /* Look for the ACL policer mapping to this Luton26 policer */
    for (policer_id = 0; policer_id < VTSS_ACL_POLICERS; policer_id++) {
        pol_alloc = &vtss_state->acl_policer_alloc[policer_id];
        if (pol_alloc->count && pol_alloc->policer == policer) {
            VTSS_I("ACL policer: %u", policer_id);
            
            /* Find new free policer */
            VTSS_RC(l26_policer_free_get(&pol_alloc->policer));

            /* Update new ACL policer */
            VTSS_RC(l26_acl_policer_set(policer_id));

            /* Update rules to point to new policer */
            VTSS_RC(l26_acl_evc_policer_move(VTSS_POLICER_USER_ACL, policer_id,
                                             policer, pol_alloc->policer));
            break;
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_action_check(const vtss_acl_action_t *action)
{
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    if (action->police && action->evc_police) {
        VTSS_E("ACL policer and EVC policer can not both be enabled");
        return VTSS_RC_ERROR;
    }
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
    return VTSS_RC_OK;
}

static vtss_policer_alloc_t *l26_acl_alloc_get(const vtss_acl_action_t *action,
                                               vtss_policer_user_t *user)
{
    if (action->police) {
        *user = VTSS_POLICER_USER_ACL;
        return &vtss_state->acl_policer_alloc[action->policer_no];
    }
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    if (action->evc_police) {
        *user = VTSS_POLICER_USER_EVC;
        return &vtss_state->evc_policer_alloc[action->evc_policer_id];
    }
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
    return NULL;
}

static vtss_rc l26_acl_policer_free(vtss_acl_action_t *action)
{
    vtss_policer_alloc_t *pol_alloc;
    vtss_policer_user_t  user;

    /* If policer is disabled, nothing is done */
    if ((pol_alloc = l26_acl_alloc_get(action, &user)) == NULL)
        return VTSS_RC_OK;
    
    if (pol_alloc->count == 0) {
        VTSS_E("policer %u already free", pol_alloc->policer);
        return VTSS_RC_ERROR;
    }
    pol_alloc->count--;
    if (pol_alloc->count == 0) {
        VTSS_RC(l26_policer_conf_set(user, pol_alloc->policer, 0, NULL));
    }
    return VTSS_RC_OK;
}

/* Allocate new ACL policer */
static vtss_rc l26_acl_policer_alloc(const vtss_acl_action_t *action)
{
    vtss_policer_alloc_t *pol_alloc;
    vtss_policer_user_t  user;

    /* If policer is disabled, nothing is done */
    if ((pol_alloc = l26_acl_alloc_get(action, &user)) == NULL)
        return VTSS_RC_OK;

    /* If policer is already allocated, just increment reference count */
    if (pol_alloc->count != 0) {
        pol_alloc->count++;
        return VTSS_RC_OK;
    }

    /* First reference, look for free policer */
    if (l26_policer_free_get(&pol_alloc->policer) == VTSS_RC_OK) {
        pol_alloc->count++;
        if (user == VTSS_POLICER_USER_ACL)
            return l26_acl_policer_set(action->policer_no);
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
        if (user == VTSS_POLICER_USER_EVC)
            return l26_evc_policer_conf_set(action->evc_policer_id);
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
    }
    VTSS_I("no more policers");
    return VTSS_RC_ERROR;
}

static vtss_rc l26_acl_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_acl_port_conf_t *conf = &vtss_state->acl_port_conf[port_no];
    u32                  port = VTSS_CHIP_PORT(port_no);
    
    /* Check if action is valid */
    VTSS_RC(l26_action_check(&conf->action));

    /* Free old policer and allocate new policer */
    VTSS_RC(l26_acl_policer_free(&vtss_state->acl_old_port_conf.action));
    VTSS_RC(l26_acl_policer_alloc(&conf->action));

    if (conf->policy_no == VTSS_ACL_POLICY_NO_NONE) {
        /* Disable S2 */
        L26_WRM_CLR(VTSS_ANA_PORT_VCAP_CFG(port), VTSS_F_ANA_PORT_VCAP_CFG_S2_ENA);
    } else {
        /* Enable S2, set policy # */
        L26_WRM(VTSS_ANA_PORT_VCAP_CFG(port), 
                VTSS_F_ANA_PORT_VCAP_CFG_S2_ENA | 
                VTSS_F_ANA_PORT_VCAP_CFG_PAG_VAL(conf->policy_no - VTSS_ACL_POLICY_NO_START),
                VTSS_F_ANA_PORT_VCAP_CFG_S2_ENA | VTSS_M_ANA_PORT_VCAP_CFG_PAG_VAL);
    }
    return l26_is2_port_action_update(port_no);
}

static vtss_rc l26_acl_port_counter_get(const vtss_port_no_t    port_no,
                                        vtss_acl_port_counter_t *const counter)
{
    return l26_is2_port_get(VTSS_CHIP_PORT(port_no), counter, 0);
}

static vtss_rc l26_acl_port_counter_clear(const vtss_port_no_t port_no)
{
    u32 counter;
    
    return l26_is2_port_get(VTSS_CHIP_PORT(port_no), &counter, 1);
}

/* Offset added to extra IP entry */
#define VTSS_ACE_ID_IP 0x100000000LL

static vtss_rc l26_is2_policer_free(vtss_is2_data_t *is2)
{
    vtss_acl_action_t action;
    
    if (is2->policer_type == VTSS_L26_POLICER_NONE)
        return VTSS_RC_OK;
    
    /* Convert IS2 policer data to ACL action */
    memset(&action, 0, sizeof(action));
    if (is2->policer_type == VTSS_L26_POLICER_ACL) {
        action.police = 1;
        action.policer_no = is2->policer;
    } else {
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
        action.evc_police = 1;
        action.evc_policer_id = is2->policer;
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
    }
    return l26_acl_policer_free(&action);
}

/* Add ACE */
static vtss_rc l26_ace_add(const vtss_ace_id_t ace_id, const vtss_ace_t *const ace)
{
    vtss_vcap_obj_t             *is1_obj = &vtss_state->is1.obj;
    vtss_vcap_user_t            is1_user = VTSS_IS1_USER_ACL;
    vtss_vcap_obj_t             *is2_obj = &vtss_state->is2.obj;
    vtss_vcap_user_t            is2_user = VTSS_IS2_USER_ACL;
    vtss_vcap_data_t            data;
    vtss_is2_data_t             *is2 = &data.u.is2;
    vtss_is2_entry_t            entry;
    vtss_ace_t                  *ace_copy = &entry.ace;
    const vtss_ace_udp_tcp_t    *sport = NULL, *dport = NULL;
    vtss_vcap_id_t              id, id_next;
    u32                         old = 0, old_ptp = 0, old_ip = 0, new_ptp = 0, new_ip = 0;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range; 
    u8                          policer_type;
    
    /*** Step 1: Check the simple things */
    VTSS_RC(l26_action_check(&ace->action));
    VTSS_RC(vtss_cmn_ace_add(ace_id, ace));

    /*** Step 2: Check if IS2 entries can be added */

    /* Initialize entry data */
    vtss_vcap_is2_init(&data, &entry);
    
    /* Check if main entry exists */
    if (vtss_vcap_lookup(is2_obj, is2_user, ace->id, &data, NULL) == VTSS_RC_OK)
        old = 1;

    /* Check if PTP entry exists */
    if (vtss_vcap_lookup(is2_obj, VTSS_IS2_USER_ACL_PTP, ace->id, NULL, NULL) == VTSS_RC_OK)
        old_ptp = 1;

    /* Check if extra IP entry exists */
    id = (ace->id + VTSS_ACE_ID_IP);
    if (vtss_vcap_lookup(is2_obj, is2_user, id, NULL, NULL) == VTSS_RC_OK)
        old_ip = 1;
    
    /* Check if PTP entry must be added */
    if ((ace->type == VTSS_ACE_TYPE_ETYPE && ace->frame.etype.ptp.enable) ||
        (ace->type == VTSS_ACE_TYPE_IPV4 && ace->frame.ipv4.ptp.enable) ||
        (ace->type == VTSS_ACE_TYPE_IPV6 && ace->frame.ipv6.ptp.enable))
        new_ptp = 1;
    
    /* Check if extra IP entry must be added */
    if ((ace->type == VTSS_ACE_TYPE_IPV4 && ace->frame.ipv4.proto.mask != 0xff) ||
        (ace->type == VTSS_ACE_TYPE_IPV6 && ace->frame.ipv6.proto.mask != 0xff))
        new_ip = 1;

    if ((is2_obj->count + new_ptp + new_ip) >= (is2_obj->max_count + old + old_ptp + old_ip)) {
        VTSS_I("IS2 is full");
        return VTSS_RC_ERROR;
    }

    /* Calculate next ID and check that it exists */
    id = (ace_id + VTSS_ACE_ID_IP);
    id_next = (vtss_vcap_lookup(is2_obj, is2_user, id, NULL, NULL) == VTSS_RC_OK ? id : ace_id);
    VTSS_RC(vtss_vcap_add(is2_obj, is2_user, ace->id, id_next, NULL, 0));
    
    /*** Step 3: Check if IS1 entry can be added */
    if (ace->type == VTSS_ACE_TYPE_IPV4 && ace->frame.ipv4.sip_smac.enable) {
        entry.host_match = 1;
        VTSS_RC(vtss_vcap_add(is1_obj, is1_user, ace->id, VTSS_ACE_ID_LAST, NULL, 0));
    }

    /*** Step 4: Allocate range checkers */
    /* Free any old range checkers */
    VTSS_RC(vtss_vcap_range_free(&range_new, is2->srange));
    VTSS_RC(vtss_vcap_range_free(&range_new, is2->drange));
    is2->srange = VTSS_VCAP_RANGE_CHK_NONE;
    is2->drange = VTSS_VCAP_RANGE_CHK_NONE;

    if (ace->type == VTSS_ACE_TYPE_IPV4 && vtss_vcap_udp_tcp_rule(&ace->frame.ipv4.proto)) {
        sport = &ace->frame.ipv4.sport;
        dport = &ace->frame.ipv4.dport;
    } 
    if (ace->type == VTSS_ACE_TYPE_IPV6 && vtss_vcap_udp_tcp_rule(&ace->frame.ipv6.proto)) {
        sport = &ace->frame.ipv6.sport;
        dport = &ace->frame.ipv6.dport;
    }

    if (sport && dport) {
        /* Allocate new range checkers */
        VTSS_RC(vtss_vcap_udp_tcp_range_alloc(&range_new, &is2->srange, sport, 1));
        VTSS_RC(vtss_vcap_udp_tcp_range_alloc(&range_new, &is2->drange, dport, 0));
    }

    /* Free old policer and allocate new policer */
    VTSS_RC(l26_is2_policer_free(is2));
    VTSS_RC(l26_acl_policer_alloc(&ace->action));

    /* Convert action to IS2 policer data */
    if (ace->action.police) {
        is2->policer_type = VTSS_L26_POLICER_ACL;
        is2->policer = ace->action.policer_no;
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    } else if (ace->action.evc_police) {
        is2->policer_type = VTSS_L26_POLICER_EVC;
        is2->policer = ace->action.evc_policer_id;
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
    } else
        is2->policer_type = VTSS_L26_POLICER_NONE;
    policer_type = is2->policer_type;

    /* Commit range checkers */
    VTSS_RC(vtss_vcap_range_commit(&range_new));

    /*** Step 5: Add IS2 entries */

    /* Add main entry */
    *ace_copy = *ace;
    is2->entry = &entry;
    entry.first = 1;
    if (new_ptp) {
        /* Neutral actions for first PTP lookup */
        is2->policer_type = VTSS_L26_POLICER_NONE;
        memset(&ace_copy->action, 0, sizeof(vtss_acl_action_t));
        ace_copy->action.learn = 1;
    }
    VTSS_RC(vtss_vcap_add(is2_obj, is2_user, ace->id, id_next, &data, 0));

    /* Check if PTP entry must be added */
    if (new_ptp) {
        /* Find next ID value */
        VTSS_RC(vtss_vcap_get_next_id(is2_obj, is2_user, VTSS_IS2_USER_ACL_PTP, ace->id, &id));
        
        /* Restore actions and add PTP entry */
        *ace_copy = *ace;
        entry.first = 0;
        is2->policer_type = policer_type;
        VTSS_RC(vtss_vcap_add(is2_obj, VTSS_IS2_USER_ACL_PTP, ace->id, id, &data, 0));
    } else if (old_ptp) {
        /* Delete old PTP entry */
        VTSS_RC(vtss_vcap_del(is2_obj, VTSS_IS2_USER_ACL_PTP, ace->id));
    }

    /* Check if extra IP entry must be added */
    id = (ace->id + VTSS_ACE_ID_IP);
    if (new_ip) {
        vtss_ace_u8_t *proto = (ace->type == VTSS_ACE_TYPE_IPV4 ? 
                                &ace_copy->frame.ipv4.proto : &ace_copy->frame.ipv6.proto);
        entry.first = 1;
        entry.udp_tcp_any = 1;
        proto->mask = 0xff;
        proto->value = 6;
        VTSS_RC(vtss_vcap_add(is2_obj, is2_user, id, ace->id, &data, 0));
    } else if (old_ip) {
        /* Delete old IP entry */
        VTSS_RC(vtss_vcap_del(is2_obj, is2_user, id));
    }
    
    /*** Step 6: Add IS1 entry */
    if (entry.host_match) {
        vtss_is1_entry_t is1_entry;

        vtss_vcap_is1_init(&data, &is1_entry);
        is1_entry.key.type = VTSS_IS1_TYPE_SMAC_SIP;
        is1_entry.key.frame.smac_sip.port_no = VTSS_PORT_NO_NONE;
        is1_entry.key.frame.smac_sip.smac = ace->frame.ipv4.sip_smac.smac;
        is1_entry.key.frame.smac_sip.sip = ace->frame.ipv4.sip_smac.sip;
        is1_entry.action.host_match = 1;
        VTSS_RC(vtss_vcap_add(is1_obj, is1_user, ace->id, VTSS_ACE_ID_LAST, &data, 0));
    }   

    return VTSS_RC_OK;
}

/* Delete ACE */
static vtss_rc l26_ace_del(const vtss_ace_id_t ace_id)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is2.obj;
    vtss_vcap_data_t data;
    
    /* Lookup main entry and free old policer */
    VTSS_RC(vtss_vcap_lookup(obj, VTSS_IS2_USER_ACL, ace_id, &data, NULL));
    VTSS_RC(l26_is2_policer_free(&data.u.is2));

    /* Delete main entry */
    VTSS_RC(vtss_cmn_ace_del(ace_id));

    /* Delete PTP entry */
    VTSS_RC(vtss_vcap_del(obj, VTSS_IS2_USER_ACL_PTP, ace_id));

    /* Delete extra IP entry */
    VTSS_RC(vtss_vcap_del(obj, VTSS_IS2_USER_ACL, ace_id + VTSS_ACE_ID_IP));

    /* Delete SMAC/SIP entry in IS1 */
    VTSS_RC(vtss_vcap_del(&vtss_state->is1.obj, VTSS_IS1_USER_ACL, ace_id));

    return VTSS_RC_OK;
}

/* Get/clear counter for extra IP ACE */
static vtss_rc l26_ace_get(const vtss_ace_id_t ace_id, 
                           vtss_ace_counter_t *const counter, 
                           BOOL clear)
{
    vtss_vcap_obj_t *obj = &vtss_state->is2.obj;
    u32             cnt = 0;
    vtss_vcap_idx_t idx;

    /* Add/clear counter from extra IP entry */
    if (vtss_vcap_lookup(obj, VTSS_IS2_USER_ACL, ace_id + VTSS_ACE_ID_IP, NULL, &idx) == 
        VTSS_RC_OK) {
        VTSS_RC(obj->entry_get(&idx, &cnt, clear));
        if (counter != NULL)
            *counter += cnt;
    }

    return VTSS_RC_OK;
}

/* Get ACE counter */
static vtss_rc l26_ace_counter_get(const vtss_ace_id_t ace_id, vtss_ace_counter_t *const counter)
{
    VTSS_RC(vtss_cmn_ace_counter_get(ace_id, counter));
    return l26_ace_get(ace_id, counter, 0);
}

/* Clear ACE counter */
static vtss_rc l26_ace_counter_clear(const vtss_ace_id_t ace_id)
{
    VTSS_RC(vtss_cmn_ace_counter_clear(ace_id));
    return l26_ace_get(ace_id, NULL, 1);
}

/* Commit VCAP range checkers */
static vtss_rc l26_vcap_range_commit(void)
{
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
        L26_WR(VTSS_ANA_COMMON_VCAP_RNG_TYPE_CFG(i),
               VTSS_F_ANA_COMMON_VCAP_RNG_TYPE_CFG_VCAP_RNG_CFG(type));
        L26_WR(VTSS_ANA_COMMON_VCAP_RNG_VAL_CFG(i),
               VTSS_F_ANA_COMMON_VCAP_RNG_VAL_CFG_VCAP_RNG_MIN_VAL(entry->min) |
               VTSS_F_ANA_COMMON_VCAP_RNG_VAL_CFG_VCAP_RNG_MAX_VAL(entry->max))
    }
    return VTSS_RC_OK;
}

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
 * l26_sflow_hw_rate()
 */
static u32 l26_sflow_hw_rate(const u32 desired_sw_rate, u32 *const realizable_sw_rate)
{
    u32 hw_rate         = desired_sw_rate ? VTSS_ROUNDING_DIVISION(LU26_SFLOW_MAX_SAMPLE_RATE + 1, desired_sw_rate) : 0;
    hw_rate             = hw_rate > LU26_SFLOW_MIN_SAMPLE_RATE ? hw_rate - 1 : hw_rate;
    *realizable_sw_rate = VTSS_ROUNDING_DIVISION(LU26_SFLOW_MAX_SAMPLE_RATE + 1, hw_rate + 1);
    return hw_rate;
}

/**
 * l26_sflow_sampling_rate_convert()
 */
static vtss_rc l26_sflow_sampling_rate_convert(const struct vtss_state_s *const state, const BOOL power2, const u32 rate_in, u32 *const rate_out)
{
    u32 modified_rate_in;
    // Could happen that two threads call this function simultaneously at boot, but we take the risk.
    // Once sflow_max_power_of_two has been computed, it's no longer a problem with simultaneous access.
    /*lint -esym(459, sflow_max_power_of_two) */
    static u32 sflow_max_power_of_two;

    if (sflow_max_power_of_two == 0) {
        sflow_max_power_of_two = next_power_of_two(LU26_SFLOW_MAX_SAMPLE_RATE);
        if ((LU26_SFLOW_MAX_SAMPLE_RATE & sflow_max_power_of_two) == 0) {
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

    (void)l26_sflow_hw_rate(modified_rate_in, rate_out);
    return VTSS_RC_OK;
}

/**
 * l26_sflow_port_conf_set()
 */
static vtss_rc l26_sflow_port_conf_set(const vtss_port_no_t port_no, const vtss_sflow_port_conf_t *const new_conf)
{
    u32 hw_rate, value;
    vtss_sflow_port_conf_t *cur_conf = &vtss_state->sflow_conf[port_no];

    *cur_conf = *new_conf;
    hw_rate = l26_sflow_hw_rate(new_conf->sampling_rate, &cur_conf->sampling_rate);

    value  = VTSS_F_ANA_ANA_SFLOW_CFG_SF_RATE(hw_rate);
    value |= new_conf->sampling_rate != 0 && (new_conf->type == VTSS_SFLOW_TYPE_ALL || new_conf->type == VTSS_SFLOW_TYPE_RX) ? VTSS_F_ANA_ANA_SFLOW_CFG_SF_SAMPLE_RX : 0;
    value |= new_conf->sampling_rate != 0 && (new_conf->type == VTSS_SFLOW_TYPE_ALL || new_conf->type == VTSS_SFLOW_TYPE_TX) ? VTSS_F_ANA_ANA_SFLOW_CFG_SF_SAMPLE_TX : 0;
    L26_WR(VTSS_ANA_ANA_SFLOW_CFG(VTSS_CHIP_PORT(port_no)), value);

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_SFLOW */

#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
static vtss_rc l26_evc_policer_move(u32 policer)
{
    vtss_policer_alloc_t *pol_alloc;
    u32                  policer_id;

    VTSS_I("policer: %u", policer);

    /* Look for the ACL policer mapping to this Luton26 policer */
    for (policer_id = 0; policer_id < VTSS_EVC_POLICERS; policer_id++) {
        pol_alloc = &vtss_state->evc_policer_alloc[policer_id];
        if (pol_alloc->count && pol_alloc->policer == policer) {
            VTSS_I("EVC policer: %u", policer_id);

            /* Find new free policer */
            VTSS_RC(l26_policer_free_get(&pol_alloc->policer));

            /* Update new EVC policer */
            VTSS_RC(l26_evc_policer_conf_set(policer_id));

            /* Update rules to point to new policer */
            VTSS_RC(l26_acl_evc_policer_move(VTSS_POLICER_USER_EVC, policer_id, 
                                             policer, pol_alloc->policer));
            break;
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_evc_policer_conf_set(const vtss_evc_policer_id_t policer_id)
{
    vtss_evc_policer_conf_t *conf = &vtss_state->evc_policer_conf[policer_id];
    vtss_policer_alloc_t    *pol_alloc = &vtss_state->evc_policer_alloc[policer_id];
    vtss_l26_policer_conf_t cfg;

    /* Only setup policer if allocated */
    if (pol_alloc->count == 0)
        return VTSS_RC_OK;

    /* Convert to Luton26 policer configuration */
    memset(&cfg, 0, sizeof(cfg));
    cfg.dual = 1;
    if (conf->enable) {
        /* Use configured values if policer enabled */
        if (conf->eir == 0) {
            /* Use single leaky bucket if EIR is zero */
            cfg.dual = 0;
            cfg.eir = conf->cir;
            cfg.ebs = conf->cbs;
        } else {
            cfg.cir = conf->cir;
            cfg.cbs = conf->cbs;
            cfg.eir = conf->eir;
            cfg.ebs = conf->ebs;
            cfg.cf = conf->cf;
        }
    } else {
        /* Use maximum rates if policer disabled */
        cfg.cir = 100000000; /* 100 Gbps, will be rounded down */
        cfg.eir = 100000000; /* 100 Gbps, will be rounded down */
        cfg.cbs = 4*1024; /* 4 kB burst size */
        cfg.ebs = 4*1024; /* 4 kB burst size */
    }
    return l26_policer_conf_set(VTSS_POLICER_USER_EVC, pol_alloc->policer, 1, &cfg);
}
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */

#if defined(VTSS_ARCH_CARACAL)
/* VCAP ID for EVC entries in ES0 */
static vtss_vcap_id_t l26_evc_es0_id(vtss_evc_id_t evc_id, vtss_port_no_t port_no)
{
    vtss_vcap_id_t id = port_no;

    return ((id << 32) + evc_id);
}

static vtss_rc l26_evc_es0_add(vtss_evc_id_t evc_id, vtss_ece_entry_t *ece, 
                               vtss_port_no_t port_no)
{
    vtss_vcap_id_t       id = l26_evc_es0_id(evc_id, port_no);
    vtss_evc_entry_t     *evc = &vtss_state->evc_info.table[evc_id];
    vtss_evc_inner_tag_t *it = &evc->inner_tag;
    vtss_vcap_data_t     data;
    vtss_es0_data_t      *es0 = &data.u.es0;
    vtss_es0_entry_t     entry;
    vtss_es0_key_t       *key = &entry.key;
    vtss_es0_action_t    *action = &entry.action;
    BOOL                 normal;
    
    vtss_vcap_es0_init(&data, &entry);
    key->port_no = port_no;
    key->data.vid.vid = evc->ivid;
    if (ece == NULL) {
        /* NNI port */
        if (it->type == VTSS_EVC_INNER_TAG_NONE) {
            /* Push one tag ("port tag") */
            action->vid_a = evc->vid;
            action->tag = VTSS_ES0_TAG_NONE;
        } else {
            /* Push two tags ("port tag" is outer and "ES0 tag" is inner) */
            action->tag = VTSS_ES0_TAG_PORT;
            normal = VTSS_BOOL(it->vid_mode == VTSS_EVC_VID_MODE_NORMAL);
            action->vid_a = (normal ? evc->vid : it->vid);
            action->vid_b = (normal ? it->vid : evc->vid);
            action->qos = (it->pcp_dei_preserve ? VTSS_ES0_QOS_CLASS : VTSS_ES0_QOS_ES0);
            action->pcp = it->pcp;
            action->dei = it->dei;
            action->tpid = (it->type == VTSS_EVC_INNER_TAG_C ? VTSS_ES0_TPID_C :
                            it->type == VTSS_EVC_INNER_TAG_S ? VTSS_ES0_TPID_S :
                            VTSS_ES0_TPID_PORT);
        }
    } else {
        /* UNI port: Push one tag ("ES0 tag") */
        es0->port_no = port_no;
        es0->flags = VTSS_ES0_FLAG_TPID;
        action->tag = VTSS_ES0_TAG_ES0;
        action->vid_b = ((ece->act_flags & VTSS_ECE_ACT_OT_ENA) ? evc->uvid: ece->vid.vr.v.value);
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
    }
    return vtss_vcap_add(&vtss_state->es0.obj, VTSS_ES0_USER_EVC, 
                         id, VTSS_VCAP_ID_LAST, &data, 0);
}

/* VCAP ID for EVC entries in IS1 */
static vtss_vcap_id_t l26_evc_is1_id(vtss_ece_entry_t *ece, BOOL nni)
{
    vtss_vcap_id_t id = (nni ? 1 : 0);

    return ((id << 32) + ece->ece_id);
}

static vtss_rc l26_evc_range_alloc(vtss_vcap_range_chk_table_t *range, vtss_ece_entry_t *ece,
                                   vtss_is1_data_t *is1)
{
    vtss_vcap_vr_t *dscp, *sport, *dport;

    VTSS_RC(vtss_vcap_vr_alloc(range, &is1->vid_range, VTSS_VCAP_RANGE_TYPE_VID, &ece->vid));
    if (ece->key_flags & (VTSS_ECE_KEY_PROT_IPV4 | VTSS_ECE_KEY_PROT_IPV6)) {
        if (ece->key_flags & VTSS_ECE_KEY_PROT_IPV4) {
            dscp = &ece->frame.ipv4.dscp;
            sport = &ece->frame.ipv4.sport; 
            dport = &ece->frame.ipv4.dport; 
        } else {
            dscp = &ece->frame.ipv6.dscp;
            sport = &ece->frame.ipv6.sport; 
            dport = &ece->frame.ipv6.dport; 
        }
        VTSS_RC(vtss_vcap_vr_alloc(range, &is1->dscp_range, VTSS_VCAP_RANGE_TYPE_DSCP, dscp));
        VTSS_RC(vtss_vcap_vr_alloc(range, &is1->sport_range, VTSS_VCAP_RANGE_TYPE_SPORT, sport));
        VTSS_RC(vtss_vcap_vr_alloc(range, &is1->dport_range, VTSS_VCAP_RANGE_TYPE_DPORT, dport));
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_evc_range_free(vtss_vcap_range_chk_table_t *range, vtss_vcap_id_t id)
{
    vtss_vcap_data_t data;
    vtss_is1_data_t  *is1 = &data.u.is1;
    
    if (vtss_vcap_lookup(&vtss_state->is1.obj, VTSS_IS1_USER_EVC, id, &data, NULL) == 
        VTSS_RC_OK) {
        VTSS_RC(vtss_vcap_range_free(range, is1->vid_range));
        VTSS_RC(vtss_vcap_range_free(range, is1->dscp_range));
        VTSS_RC(vtss_vcap_range_free(range, is1->sport_range));
        VTSS_RC(vtss_vcap_range_free(range, is1->dport_range));
    }
    return VTSS_RC_OK;
}

static BOOL l26_evc_id_valid(vtss_evc_id_t evc_id)
{
    return (evc_id != VTSS_EVC_ID_NONE && vtss_state->evc_info.table[evc_id].enable ? 1 : 0);
}

static vtss_rc l26_evc_is1_add(vtss_ece_entry_t *ece, BOOL nni)
{
    u32                         port_count = vtss_state->port_count;
    vtss_vcap_id_t              id_next, id = l26_evc_is1_id(ece, nni);
    vtss_is1_entry_t            entry;
    vtss_is1_key_t              *key = &entry.key;
    vtss_is1_action_t           *action = &entry.action;
    vtss_vcap_data_t            data;
    vtss_is1_data_t             *is1 = &data.u.is1;
    vtss_evc_entry_t            *evc = &vtss_state->evc_info.table[ece->evc_id];
    vtss_evc_inner_tag_t        *it = &evc->inner_tag;
    vtss_port_no_t              port_no;
    BOOL                        ece_pcp, nni_bidir = 0;
    BOOL                        inner_tag = 0;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range;

    /* Free old range checkers */
    VTSS_RC(l26_evc_range_free(&range_new, id));

    vtss_vcap_is1_init(&data, &entry);
    is1->lookup = 0; /* First lookup */

    if (nni) {
        /* NNI rule, not added for DIR_UNI_TO_NNI */
        if (ece->act_flags & VTSS_ECE_ACT_DIR_UNI_TO_NNI) 
            return VTSS_RC_OK;

        /* For DIR_BOTH, bidirectional rules are used for NNI ports */
        if ((ece->act_flags & VTSS_ECE_ACT_DIR_NNI_TO_UNI) == 0) 
            nni_bidir = 1;
        
        /* NNI ports */
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (VTSS_PORT_BF_GET(evc->ports, port_no)) {
                key->port_list[port_no] = 1;
                if (vtss_state->evc_port_conf[port_no].inner_tag)
                    inner_tag = 1;
            }
        }
    } else {
        /* UNI rule, not added for DIR_NNI_TO_UNI */
        if (ece->act_flags & VTSS_ECE_ACT_DIR_NNI_TO_UNI) 
            return VTSS_RC_OK;

        /* UNI ports */
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (VTSS_PORT_BF_GET(ece->ports, port_no)) {
                key->port_list[port_no] = 1;
                if (vtss_state->evc_port_conf[port_no].inner_tag)
                    inner_tag = 1;
            }
        }
    }

    /* Match only double tagged frames if inner tag is enabled on one of the ports */
    if (inner_tag)
        key->inner_tag.tagged = VTSS_VCAP_BIT_1;

    if (nni_bidir) {
        /* NNI bidirectional rule */

        /* Match EVC VID by default */
        key->tag.vid.vr.v.value = evc->vid;
        key->tag.vid.vr.v.mask = 0xfff;
        key->tag.tagged = VTSS_VCAP_BIT_1;

        /* VID and PCP matching */
        ece_pcp = 0;
        if (inner_tag) {
            /* Matching on inner tag is enabled */
            if (it->type != VTSS_EVC_INNER_TAG_NONE) {
                /* Inner tag added */

                if (it->vid_mode == VTSS_EVC_VID_MODE_NORMAL)
                    key->tag.vid.vr.v.value = it->vid; /* Match inner tag in Normal mode */

                if (it->pcp_dei_preserve) {
                    /* Preserved PCP/DEI */
                    ece_pcp = 1;
                } else {
                    /* Fixed PCP/DEI */
                    key->tag.pcp.value = it->pcp;
                    key->tag.pcp.mask = 0x7;
                }
            }
        } else {
            /* Matching on outer tag is enabled */
            if (it->type != VTSS_EVC_INNER_TAG_NONE && it->vid_mode == VTSS_EVC_VID_MODE_TUNNEL)
                key->tag.vid.vr.v.value = it->vid; /* Match inner tag in Tunnel mode */
            ece_pcp = 1;
        }

        if (ece_pcp) {
            /* The ECE determines the PCP matching */ 
            if (ece->act_flags & VTSS_ECE_ACT_OT_PCP_MODE_FIXED) {
                /* Fixed PCP/DEI */
                key->tag.pcp.value = ece->ot_pcp;
                key->tag.pcp.mask = 0x7;
            } else {
                /* Preserved PCP/DEI */
                key->tag.pcp = ece->pcp;
            }
        }

        action->pop = (it->type == VTSS_EVC_INNER_TAG_NONE ? 1 : 2);
    } else {
        /* UNI rule or unidirectional NNI rule */

        /* Allocate range checkers for UNI/NNIs */
        VTSS_RC(l26_evc_range_alloc(&range_new, ece, is1));

        /* MAC header matching */
        key->mac.dmac_mc = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_DMAC_MC_VLD,
                                                VTSS_ECE_KEY_DMAC_MC_1);
        key->mac.dmac_bc = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_DMAC_BC_VLD,
                                                VTSS_ECE_KEY_DMAC_BC_1);
        key->mac.smac = ece->smac;
        
        /* VLAN tag matching */
        key->tag.vid = ece->vid;
        key->tag.pcp = ece->pcp;
        key->tag.dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_DEI_VLD, VTSS_ECE_KEY_TAG_DEI_1);
        key->tag.tagged = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_TAGGED_VLD,
                                               VTSS_ECE_KEY_TAG_TAGGED_1);
        key->tag.s_tag = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_S_TAGGED_VLD,
                                              VTSS_ECE_KEY_TAG_S_TAGGED_1);

        /* IP header matching */
        if (ece->key_flags & VTSS_ECE_KEY_PROT_IPV4) {
            key->type = VTSS_IS1_TYPE_IPV4;
            key->frame.ipv4.fragment = ece->frame.ipv4.fragment;
            key->frame.ipv4.proto = ece->frame.ipv4.proto;
            key->frame.ipv4.dscp = ece->frame.ipv4.dscp;
            key->frame.ipv4.sip = ece->frame.ipv4.sip;
            key->frame.ipv4.sport = ece->frame.ipv4.sport;
            key->frame.ipv4.dport = ece->frame.ipv4.dport;
        } else if (ece->key_flags & VTSS_ECE_KEY_PROT_IPV6) {
            key->type = VTSS_IS1_TYPE_IPV6;
            key->frame.ipv6.proto = ece->frame.ipv6.proto;
            key->frame.ipv6.dscp = ece->frame.ipv6.dscp;
            key->frame.ipv6.sip = ece->frame.ipv6.sip;
            key->frame.ipv6.sport = ece->frame.ipv6.sport;
            key->frame.ipv6.dport = ece->frame.ipv6.dport;
        }

        if (ece->act_flags & VTSS_ECE_ACT_POP_1)
            action->pop = 1;
        else if (ece->act_flags & VTSS_ECE_ACT_POP_2)
            action->pop = 2;
        if (ece->act_flags & VTSS_ECE_ACT_OT_PCP_MODE_FIXED) {
            /* Fixed PCP/DEI is used in outer tag ("port tag"), change classification */
            action->pcp_dei_enable = 1;
            action->pcp = ece->ot_pcp;
            action->dei = VTSS_BOOL(ece->act_flags & VTSS_ECE_ACT_OT_DEI);
        }
    }

    /* Action fields common for both directions */
    action->vid = evc->ivid;
    action->pop_enable = 1;
    if (!(ece->act_flags & VTSS_ECE_ACT_POLICY_NONE)) {
        action->pag_enable = 1;
        action->pag = ece->policy_no;
    }
    if (ece->act_flags & VTSS_ECE_ACT_PRIO_ENA) {
        action->prio_enable = 1;
        action->prio = ece->prio;
    }

    /* Commit range checkers */
    VTSS_RC(vtss_vcap_range_commit(&range_new));

    /* Find next ID with valid EVC */
    id_next = VTSS_VCAP_ID_LAST;
    for (ece = ece->next ; ece != NULL; ece = ece->next) {
        if (l26_evc_id_valid(ece->evc_id)) {
            id_next = l26_evc_is1_id(ece, (ece->act_flags & VTSS_ECE_ACT_DIR_NNI_TO_UNI) ? 1 : 0);
            break;
        }
    } 
    return vtss_vcap_add(&vtss_state->is1.obj, VTSS_IS1_USER_EVC, id, id_next, &data, 0);
}

static vtss_rc l26_evc_is1_del(vtss_ece_entry_t *ece, BOOL nni)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is1.obj;
    vtss_vcap_user_t user = VTSS_IS1_USER_EVC;
    vtss_vcap_id_t   id = l26_evc_is1_id(ece, nni);

    if (vtss_vcap_lookup(obj, user, id, NULL, NULL) == VTSS_RC_OK) {
        /* Free range checkers */
        VTSS_RC(l26_evc_range_free(&vtss_state->vcap_range, id));
        
        /* Delete IS1 entry */
        VTSS_RC(vtss_vcap_del(obj, user, id));
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_ece_update(vtss_ece_entry_t *ece, vtss_res_t *res, vtss_res_cmd_t cmd)
{
    u32                         port_count = vtss_state->port_count;
    vtss_port_no_t              port_no;
    vtss_vcap_id_t              id;
    vtss_vcap_data_t            data;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range;

    if (cmd == VTSS_RES_CMD_CALC) {
        /* Calculate resource usage */

        /* One IS1 rule for UNIs and one IS1 entry for NNIs */
        if (res->ece_del) {
            res->is1.del += (res->dir_old == VTSS_ECE_DIR_BOTH ? 2 : 1);

            if (res->dir_old == VTSS_ECE_DIR_NNI_TO_UNI) {
                /* Free old range checkers for NNIs */
                id = l26_evc_is1_id(ece, 1);
                VTSS_RC(l26_evc_range_free(&range_new, id));
            } else {
                /* Free old range checkers for UNIs */
                id = l26_evc_is1_id(ece, 0);
                VTSS_RC(l26_evc_range_free(&range_new, id));
            }
        }
        
        if (res->ece_add) {
            res->is1.add += (res->dir_new == VTSS_ECE_DIR_BOTH ? 2 : 1);

            /* Allocate range checkers for UNI/NNIs */
            VTSS_RC(l26_evc_range_alloc(&range_new, ece, &data.u.is1));
        }
        
        /* ES0 rules per UNI */
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (res->es0_add[port_no])
                res->es0.add++;
            if (res->es0_del[port_no])
                res->es0.del++;
        }
    } else if (cmd == VTSS_RES_CMD_DEL) {
        /* Delete resources */
        if (res->ece_del) {
            /* IS1 rule for UNIs */
            VTSS_RC(l26_evc_is1_del(ece, 0));

            /* IS1 rule for NNIs */
            VTSS_RC(l26_evc_is1_del(ece, 1));
        }

        /* ES0 rules for UNIs */
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (res->es0_del[port_no]) {
                id = l26_evc_es0_id(ece->evc_id, port_no);
                VTSS_RC(vtss_vcap_del(&vtss_state->es0.obj, VTSS_ES0_USER_EVC, id));
            }
        }
    } else if (cmd == VTSS_RES_CMD_ADD) {
        /* Add/update resources */
        if (res->ece_add) {
            /* IS1 rule for UNIs */
            VTSS_RC(l26_evc_is1_add(ece, 0));

            /* IS1 rule for NNIs */
            VTSS_RC(l26_evc_is1_add(ece, 1));

            /* ES0 rules for UNIs */
            if (vtss_cmn_ece_es0_needed(ece)) {
                for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                    if (VTSS_PORT_BF_GET(ece->ports, port_no)) {
                        VTSS_RC(l26_evc_es0_add(ece->evc_id, ece, port_no));
                    }
                }
            }
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_evc_update(vtss_evc_id_t evc_id, vtss_res_t *res, vtss_res_cmd_t cmd)
{
    vtss_evc_entry_t *evc = &vtss_state->evc_info.table[evc_id];
    vtss_port_no_t   port_no;
    vtss_vcap_id_t   id;
    vtss_ece_entry_t *ece;

    if (cmd == VTSS_RES_CMD_CALC) {
        /* Calculate resource usage */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            /* One ES0 rule per (EVC, NNI) */
            if (res->port_add[port_no])
                res->es0.add++;
            if (res->port_del[port_no])
                res->es0.del++;
        }
    } else if (cmd == VTSS_RES_CMD_DEL) {
        /* Delete resources */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (res->port_del[port_no]) {
                id = l26_evc_es0_id(evc_id, port_no);
                VTSS_RC(vtss_vcap_del(&vtss_state->es0.obj, VTSS_ES0_USER_EVC, id));
            }
        }
    } else if (cmd == VTSS_RES_CMD_ADD) {
        /* Add/update resources */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (VTSS_PORT_BF_GET(evc->ports, port_no)) {
                /* One ES0 rule per (EVC, NNI) */
                VTSS_RC(l26_evc_es0_add(evc_id, NULL, port_no));
            }
        }

        /* Add/update IS1 rules for UNI/NNIs */
        res->ece_add = 1;
        for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
            if (ece->evc_id == evc_id) {
                VTSS_RC(l26_ece_update(ece, res, cmd));
            }
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc l26_evc_port_conf_set(const vtss_port_no_t port_no)
{
    u32              mask, port = VTSS_CHIP_PORT(port_no);
    BOOL             inner_tag = vtss_state->evc_port_conf[port_no].inner_tag;
    vtss_ece_entry_t *ece;
    
    /* Enable/disable inner tag classification in first IS1 lookup */
    mask = VTSS_F_ANA_PORT_VCAP_CFG_S1_VLAN_INNER_TAG_ENA(1);
    L26_WRM(VTSS_ANA_PORT_VCAP_CFG(port), inner_tag ? mask : 0, mask);

    /* Enable/disable DMAC/DIP match in first IS1 lookup */
    VTSS_RC(l26_vcl_port_conf_set(port_no));
    
    /* Update IS1 rule for NNIs */
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        if (l26_evc_id_valid(ece->evc_id)) {
            VTSS_RC(l26_evc_is1_add(ece, 1));
        }
    }
    
    /* Override tag remark mode for NNI ports */
    return l26_qos_port_conf_set(port_no);
}
#endif /* VTSS_ARCH_CARACAL */

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

static vtss_rc 
l26_ts_timeofday_get(vtss_timestamp_t               *ts,
                                 u32                     *tc)
{
    u32 value;
    i32 x;
    /* Latch PTP internal timers */
    L26_WR(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL, 
                VTSS_F_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL_PTP_LATCH |
                VTSS_F_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL_PTP_TIMER_ENA);
    L26_RD(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_SECS, &value);
    ts->sec_msb = 0; /* to be maintained in one sec interrupt */
    ts->seconds = vtss_state->ts_conf.sec_offset + value;
    L26_RD(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_NANOSECS, &value);
    /* DURING THE ADJUSTMENT PHASE THE ADJUST IS DONE IN sw */
    x = VTSS_X_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_NANOSECS_PTP_TOD_NANOSECS(value);
    if (value == 0) {
        ++ts->seconds; /* HW bug: the seconds counter is incremented one clock cycle after the ns counter wraps */
    }
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
    ts->nanoseconds = x*HW_NS_PR_CLK_CNT;

    L26_RD(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_DELAY, &value);
    *tc = value;
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
l26_ts_timeofday_offset_set(i32 offset)
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
 * Latch and reset the PTP timers in HW (the sec counter is reset)
 * The actual second counter is saved in SW.
 * The NS counter in hw (TOD_NANOSECS) has to be aligned with the ts.nanoseconds,
 * this is done by using the PTP_UPPER_LIMIT_1_TIME_ADJ:
 * This register can be used to change the one sec. period once, the period can 
 * be set to [0..268.435.455] periods of 4 ns = [0..1.073.741.820] ns.
 * alignment algorithm:
 *  (TOD_NANOSECS - ts.nanoseconds) > 50 ms => 
 *    PTP_UPPER_LIMIT_1_TIME_ADJ = TOD_NANOSECS - ts.nanoseconds
 *    (TBD if this works, because the ADJ is < actual TOD_NANOSECS ??)
 *  50 ms > (TOD_NANOSECS - ts.nanoseconds) > -950 ms => 
 *    PTP_UPPER_LIMIT_1_TIME_ADJ = TOD_NANOSECS - ts.nanoseconds + 1 sec
 *  -950 ms > (TOD_NANOSECS - ts.nanoseconds) => 
 *    PTP_UPPER_LIMIT_1_TIME_ADJ = TOD_NANOSECS - ts.nanoseconds + 2 sec
 * The second offset is adjusted accordingly.
 *
 */
static vtss_rc
l26_ts_timeofday_set(const vtss_timestamp_t         *ts)
{
    i32 corr;
    u32 value;
    u32 value1;
    u32 value2;
    if (vtss_state->ts_conf.awaiting_adjustment) {
        return VTSS_RC_ERROR;
     }
    /* Latch and reset PTP internal timers */
    L26_WR(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL, 
                VTSS_F_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL_PTP_LATCH |
                VTSS_F_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL_PTP_TIMER_ENA |
                VTSS_F_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL_PTP_TOD_RST);
    
    vtss_state->ts_conf.sec_offset = ts->seconds;
    /* corr = PTP_NS - ts.ns */
    L26_RD(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_NANOSECS, &value);
    L26_RD(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_SECS, &value1);
    L26_RD(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_DELAY, &value2);
    VTSS_D("ts->seconds: %u, ts->nanoseconds: %u", ts->seconds, ts->nanoseconds);
    
    corr = value & VTSS_M_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_NANOSECS_PTP_TOD_NANOSECS;
    VTSS_RC(l26_ts_timeofday_offset_set(corr*HW_NS_PR_CLK_CNT - ts->nanoseconds));
    VTSS_D("PTP_TOD_SECS: %u, PTP_TOD_NANOSECS: %u, PTP_DELAY: %u",vtss_state->ts_conf.sec_offset , value, value2);
    return VTSS_RC_OK;
}

/*
 * When the time is set, the SW sec  counters are written. The HW ns clock is adjusted in the next 1 sec call. 
 */
static vtss_rc l26_ts_timeofday_set_delta(const vtss_timestamp_t *ts, BOOL negative)
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
        VTSS_RC(l26_ts_timeofday_offset_set(corr));
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
static u32 l26_ts_ns_cnt_get(vtss_inst_t inst)
{
    u32 value;
    (void)inst->init_conf.reg_read(0, VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_DELAY, &value);
    return value;
}


static vtss_rc
l26_ts_timeofday_one_sec(void)
{
    u32 value, value1;
    if (vtss_state->ts_conf.awaiting_adjustment) {
        /* For debug: read the internal NS counter */
        L26_WR(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL, 
                    VTSS_F_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL_PTP_LATCH |
                    VTSS_F_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL_PTP_TIMER_ENA);
        L26_RD(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_NANOSECS, &value);
        L26_RD(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_DELAY, &value1);
        VTSS_D("TOD_NANOSECS %u, PTP_DELAY %u", value, value1);

        if (vtss_state->ts_conf.outstanding_adjustment != 0) {
            if (value > vtss_state->ts_conf.outstanding_adjustment) {
                VTSS_E("Too large interrupt latency to adjust %u (*4ns)", value);
            } else {
                /*PTP_UPPER_LIMIT_1_TIME_ADJ = x */
                L26_WR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG, 
                    vtss_state->ts_conf.outstanding_adjustment |
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
    l26_ts_adjtimer_set(void)
{
    /* adj in units of 0,1 pbb */
    i32 adj = vtss_state->ts_conf.adj/10L;
    u32 clk_adj = HW_CLK_CNT_PR_SEC;
    u32 sticky;
    /* CLK_ADJ = clock_rate/|adj|
    * CLK_ADJ_DIR = adj>0 ? Positive : Negative
    */
    if (adj != 0) {
        clk_adj = VTSS_LABS((HW_NS_PR_SEC)/adj)-1;
    }
    if (clk_adj > HW_CLK_CNT_PR_SEC) clk_adj = HW_CLK_CNT_PR_SEC;
    
    if ((adj != 0)) {
        clk_adj |= VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_ENA;
        L26_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ADJ_ENA);
    } else {
        L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ADJ_ENA);
    }
    if (adj < 0) {
        clk_adj |= VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_DIR;
    } 
    //clk_adj |= VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_UPD;
    L26_RD(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT,&sticky);
    //if (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY) {
    //    L26_WR(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY);
    //} else {
    //    VTSS_E("more than one update pr sec is performed");
    //}
    L26_WR(VTSS_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG, clk_adj);
    
    VTSS_D("adj: %d, CLK_ACJ_CFG: %x", adj, clk_adj);
    return VTSS_RC_OK;
}

static vtss_rc
    l26_ts_freq_offset_get(i32 *const adj)
{
    i32 offset; /* frequency offset in clock counts pr sec */
    u32 cu;
    L26_RD(VTSS_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT,&cu);
    cu -= EXT_SYNC_INPUT_LATCH_LATENCY;
    offset = VTSS_X_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT_EXT_SYNC_CURRENT_TIME(cu) - HW_CLK_CNT_PR_SEC;
    /* convert to units of 0,1 ppb */
    *adj = HW_NS_PR_CLK_CNT*10*offset;
    
    
    VTSS_D("cu: %u adj: %d", cu, *adj);
    return VTSS_RC_OK;
}


static vtss_rc
l26_ts_external_clock_mode_set(void)
{
    u32 dividers;
    u32 high_div;
    u32 low_div;
    if (!vtss_state->ts_conf.ext_clock_mode.enable && 
            (vtss_state->ts_conf.ext_clock_mode.one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_DISABLE)) {
        L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA);
        /* disable clock output */
        L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA);
        L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, 
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_SEL |
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA |
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA);
        L26_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, 
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA);
        /* disable alternate 1 for GPIO_7 (clock output). */
        (void) l26_gpio_mode(0, 7, VTSS_GPIO_IN);
    } else {
        /* enable alternate 1 for GPIO_7 (clock output). */
        (void) l26_gpio_mode(0, 7, VTSS_GPIO_ALT_0);

        if (vtss_state->ts_conf.ext_clock_mode.one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_OUTPUT) {
            /* 1 pps output enabled */
            L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA);
            L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, 
                        VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_SEL |
                            VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA |
                            VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA);
            L26_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, 
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL |
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA);
        } else if (vtss_state->ts_conf.ext_clock_mode.one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_INPUT) {
            /* 1 pps input enabled */
            L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA);
            L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, 
                        VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL |
                            VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA);
            L26_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, 
                        VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_SEL |
                            VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA |
                            VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA);
        } else {
            /* clock frequency output */
            L26_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, 
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_SEL |
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA |
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA |
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL |
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA);
            /* set dividers; enable clock output. */
            dividers = (HW_CLK_CNT_PR_SEC/(vtss_state->ts_conf.ext_clock_mode.freq));
            high_div = (dividers/2)-1;
            low_div =  ((dividers+1)/2)-1;
            L26_WR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG, high_div  &
                        VTSS_M_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG_GEN_EXT_CLK_HIGH_PERIOD);
            L26_WR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG, low_div  &
                        VTSS_M_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG_GEN_EXT_CLK_LOW_PERIOD);
            L26_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA);
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc
l26_ts_ingress_latency_set(vtss_port_no_t    port_no)
{
    vtss_ts_port_conf_t       *conf = &vtss_state->ts_port_conf[port_no];
    i32 rx_delay = VTSS_INTERVAL_NS(conf->ingress_latency)/HW_NS_PR_CLK_CNT;
    L26_WRM(VTSS_SYS_PTP_PTP_CFG(VTSS_CHIP_PORT(port_no)), 
            VTSS_F_SYS_PTP_PTP_CFG_IO_RX_DELAY(rx_delay), 
            VTSS_M_SYS_PTP_PTP_CFG_IO_RX_DELAY);
    return VTSS_RC_OK;
}

static vtss_rc
l26_ts_p2p_delay_set(vtss_port_no_t    port_no)
{
    VTSS_D("Not to be used,a d L26 only supports up to 1 us p2pdelay");
    return VTSS_RC_OK;
}

static vtss_rc
l26_ts_egress_latency_set(vtss_port_no_t    port_no)
{
    vtss_ts_port_conf_t       *conf = &vtss_state->ts_port_conf[port_no];
    i32 tx_delay = VTSS_INTERVAL_NS(conf->egress_latency)/HW_NS_PR_CLK_CNT;
    L26_WRM(VTSS_SYS_PTP_PTP_CFG(VTSS_CHIP_PORT(port_no)), 
            VTSS_F_SYS_PTP_PTP_CFG_IO_TX_DELAY(tx_delay), 
            VTSS_M_SYS_PTP_PTP_CFG_IO_TX_DELAY);
    return VTSS_RC_OK;
}

static vtss_rc
l26_ts_operation_mode_set(vtss_port_no_t    port_no)
{
    return VTSS_RC_ERROR;
}

static u32 api_port(u32 chip_port)
{
    u32 port_no;
    int found = 0;
/* Map from chip port to API port */
    if (chip_port == VTSS_CHIP_PORT_CPU) {
        port_no = VTSS_CHIP_PORT_CPU;
    } else {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (VTSS_CHIP_PORT(port_no) == chip_port) {
                found = 1;
                break;
            }
        }
        if (!found) {
            VTSS_E("unknown chip port: %u, port_no: %d", chip_port, port_no);
        }
    }
    return port_no;
}

static vtss_rc 
l26_ts_timestamp_get(void)
{
    u32 value;
    u32 delay;
    u32 tx_port;
    u32 mess_id;
    /* PTP_STATUS */
    L26_RD(VTSS_SYS_PTP_PTP_STATUS, &value);
    while (value & VTSS_F_SYS_PTP_PTP_STATUS_PTP_MESS_VLD) {
        L26_RD(VTSS_SYS_PTP_PTP_DELAY, &delay);
        tx_port = VTSS_X_SYS_PTP_PTP_STATUS_PTP_MESS_TXPORT(value);
        tx_port = api_port(tx_port);
        mess_id = VTSS_X_SYS_PTP_PTP_STATUS_PTP_MESS_ID(value);
        if (mess_id >= VTSS_TS_ID_SIZE) {
            VTSS_D("invalid mess_id (%u)", mess_id);
        } else {
            if (tx_port < VTSS_PORT_ARRAY_SIZE) {
                vtss_state->ts_status[mess_id].tx_tc[tx_port] = delay;
                vtss_state->ts_status[mess_id].tx_id[tx_port] = mess_id;
                vtss_state->ts_status[mess_id].valid_mask |= 1LL<<tx_port;
            } else if (tx_port == VTSS_CHIP_PORT_CPU) {
                vtss_state->ts_status[mess_id].rx_tc = delay;
                vtss_state->ts_status[mess_id].rx_tc_valid = TRUE;
            } else {
                VTSS_I("invalid port (%u)", tx_port);
            }
        }
        VTSS_D("value %u, delay %u, tx_port %u, mess_id %u", value, delay, tx_port, mess_id);
        L26_WR(VTSS_SYS_PTP_PTP_NXT, VTSS_F_SYS_PTP_PTP_NXT_PTP_NXT);
        L26_RD(VTSS_SYS_PTP_PTP_STATUS, &value);
    }
    return VTSS_RC_OK;
}



static vtss_rc 
l26_ts_timestamp_id_release(u32 ts_id)
{
    /* Clear bit corresponding to ts_id */
    if (ts_id <TS_IDS_RESERVED_FOR_SW) {
        vtss_state->ts_conf.sw_ts_id_res[ts_id] = 0;
    } else if (ts_id <=31) {
        L26_WR(VTSS_ANA_ANA_TABLES_PTP_ID_LOW, VTSS_BIT(ts_id));
    } else if (ts_id <=63) {
        L26_WR(VTSS_ANA_ANA_TABLES_PTP_ID_HIGH, VTSS_BIT(ts_id-32));
    } else {
        VTSS_D("invalid ts_id %d", ts_id);
    }
    return VTSS_RC_OK;
}


#endif /* VTSS_FEATURE_TIMESTAMP */

/* ================================================================= *
 *  Packet control
 * ================================================================= */

static vtss_rc l26_rx_conf_set(void)
{
    vtss_packet_rx_conf_t      *conf = &vtss_state->rx_conf;
    vtss_packet_rx_reg_t       *reg = &conf->reg;
    vtss_packet_rx_queue_map_t *map = &conf->map;
    u32                        queue, i, value, port, bpdu, garp;
    vtss_port_no_t             port_no;
    vtss_packet_rx_port_conf_t *port_conf;
    vtss_packet_reg_type_t     type;
    
    /* Each CPU queue get resverved extraction buffer space. No sharing at port or buffer level */
    for (queue = 0; queue < vtss_state->rx_queue_count; queue++) {
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(VTSS_CHIP_PORT_CPU * VTSS_PRIOS + queue + 512), conf->queue[queue].size/L26_BUFFER_CELL_SZ);
    }
    L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(VTSS_CHIP_PORT_CPU + 224 + 512), 0); /* No extra shared space at port level */
    
    /* Rx IPMC registration */
    value = 
        (reg->ipmc_ctrl_cpu_copy ? VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_IPMC_CTRL_COPY_ENA  : 0) |
        (reg->igmp_cpu_only      ? VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_IGMP_REDIR_ENA      : 0) |
        (reg->mld_cpu_only       ? VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_MLD_REDIR_ENA       : 0);
    for (port = 0; port < VTSS_CHIP_PORTS; port++) 
        L26_WR(VTSS_ANA_PORT_CPU_FWD_CFG(port), value);

    /* Rx BPDU and GARP registrations */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!VTSS_PORT_CHIP_SELECTED(port_no))
            continue;
        port = VTSS_CHIP_PORT(port_no);
        port_conf = &vtss_state->rx_port_conf[port_no];
        for (i = 0, bpdu = 0, garp = 0; i < 16; i++) {
            type = port_conf->bpdu_reg[i];
            if ((type == VTSS_PACKET_REG_NORMAL && reg->bpdu_cpu_only) ||
                type == VTSS_PACKET_REG_CPU_ONLY)
                bpdu |= VTSS_BIT(i);
            type = port_conf->garp_reg[i];
            if ((type == VTSS_PACKET_REG_NORMAL && reg->garp_cpu_only[i]) ||
                type == VTSS_PACKET_REG_CPU_ONLY)
                garp |= VTSS_BIT(i);
        }
        L26_WR(VTSS_ANA_PORT_CPU_FWD_BPDU_CFG(port), bpdu);
        L26_WR(VTSS_ANA_PORT_CPU_FWD_GARP_CFG(port), garp);
    }

    /* Fixme - chipset has more queues than the classification the API expose */
    value = 
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_SFLOW(map->sflow_queue != VTSS_PACKET_RX_QUEUE_NONE ? map->sflow_queue - VTSS_PACKET_RX_QUEUE_START : VTSS_PACKET_RX_QUEUE_START) |
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_MIRROR(map->mac_vid_queue-VTSS_PACKET_RX_QUEUE_START) |
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_LRN(map->learn_queue-VTSS_PACKET_RX_QUEUE_START) |
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_MAC_COPY(map->mac_vid_queue-VTSS_PACKET_RX_QUEUE_START) |
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_SRC_COPY(map->mac_vid_queue-VTSS_PACKET_RX_QUEUE_START) |
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_LOCKED_PORTMOVE(map->mac_vid_queue-VTSS_PACKET_RX_QUEUE_START) |
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_ALLBRIDGE(map->bpdu_queue-VTSS_PACKET_RX_QUEUE_START) |
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_IPMC_CTRL(map->ipmc_ctrl_queue-VTSS_PACKET_RX_QUEUE_START) |
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_IGMP(map->igmp_queue-VTSS_PACKET_RX_QUEUE_START) | 
        VTSS_F_ANA_COMMON_CPUQ_CFG_CPUQ_MLD(map->igmp_queue-VTSS_PACKET_RX_QUEUE_START);
    L26_WR(VTSS_ANA_COMMON_CPUQ_CFG, value);

    /* Setup each of the BPDU, GARP and CCM 'address' extraction queues */
    for (i = 0; i < 16; i++) {
        value = 
            VTSS_F_ANA_COMMON_CPUQ_8021_CFG_CPUQ_BPDU_VAL(map->bpdu_queue-VTSS_PACKET_RX_QUEUE_START) | 
            VTSS_F_ANA_COMMON_CPUQ_8021_CFG_CPUQ_GARP_VAL(map->garp_queue-VTSS_PACKET_RX_QUEUE_START) |
            VTSS_F_ANA_COMMON_CPUQ_8021_CFG_CPUQ_CCM_VAL(VTSS_PACKET_RX_QUEUE_START); /* Fixme */
        L26_WR(VTSS_ANA_COMMON_CPUQ_8021_CFG(i), value);
    }

    /* Configure Rx Queue #i to map to an Rx group. */
    for (value = i = 0; i < vtss_state->rx_queue_count; i++) {
        if(conf->grp_map[i]) {
            value |= VTSS_BIT(i);
        }
    }
    L26_WRM(VTSS_SYS_SCH_SCH_CPU, VTSS_F_SYS_SCH_SCH_CPU_SCH_CPU_MAP(value), VTSS_M_SYS_SCH_SCH_CPU_SCH_CPU_MAP);

    for (i = 0; i < VTSS_PACKET_RX_GRP_CNT; i++) {
        /* One-to-one mapping from CPU Queue number to Extraction Group number. Enable both. */
        /* Note: On Luton26, CPU Queue is not to be confused with CPU Extraction Queue. */
        L26_WR(VTSS_DEVCPU_QS_XTR_XTR_MAP(i), (i * VTSS_F_DEVCPU_QS_XTR_XTR_MAP_GRP) | VTSS_F_DEVCPU_QS_XTR_XTR_MAP_MAP_ENA);
    }

#if defined(VTSS_FEATURE_NPI)
    VTSS_RC(l26_npi_mask_set());
#endif /* VTSS_FEATURE_NPI */

    /* Setup CPU port 0 and 1 */
    for (i = VTSS_CHIP_PORT_CPU_0; i <= VTSS_CHIP_PORT_CPU_1; i++) {
        /* Enable IFH insertion */
        L26_WRM_SET(VTSS_REW_PORT_PORT_CFG(i), VTSS_F_REW_PORT_PORT_CFG_IFH_INSERT_ENA);

        /* Enable IFH parsing on CPU port 0 and 1 */
        L26_WRM_SET(VTSS_SYS_SYSTEM_PORT_MODE(i), VTSS_F_SYS_SYSTEM_PORT_MODE_INCL_INJ_HDR);
    }

    /* Enable CPU port */
    L26_WRM_SET(VTSS_SYS_SYSTEM_SWITCH_PORT_MODE(VTSS_CHIP_PORT_CPU), 
                VTSS_F_SYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA);
    
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

static vtss_rc l26_rx_frame_discard(const vtss_packet_rx_queue_t queue_no)
{
    BOOL done = FALSE;
    vtss_packet_rx_grp_t xtr_grp = vtss_state->rx_conf.grp_map[queue_no];
    
    while(!done) {
        u32 val;
        L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val);
        switch(val) {
        case XTR_ABORT:
        case XTR_PRUNED:
        case XTR_EOF_3:
        case XTR_EOF_2:
        case XTR_EOF_1:
        case XTR_EOF_0:
            L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val); /* Last data */
            done = TRUE;        /* Last 1-4 bytes */
            break;
        case XTR_ESCAPE:
            L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val); /* Escaped data */
            break;
        case XTR_NOT_READY:
        default:
            ;
        }
    }
    return VTSS_RC_OK;
}


static vtss_rc l26_rx_frame_rd(const vtss_packet_rx_queue_t queue_no,
                               u8 *frame,
                               int buflen,
                               u32 *bytes_read)
{
    u32 read = 0;
    u8 *buf = frame;
    BOOL done = FALSE;
    vtss_rc rc = VTSS_RC_OK;
    vtss_packet_rx_grp_t xtr_grp = vtss_state->rx_conf.grp_map[queue_no];

    while(!done && buflen > 4) {
        u32 val;

        if((rc = l26_rd(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val) != VTSS_RC_OK)) {
            return rc;
        }

        switch(val) {
        case XTR_NOT_READY:
            break;              /* Try again... */
        case XTR_ABORT:
            L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val); /* Unused */
            rc = VTSS_RC_ERROR;
            done = TRUE;
            break;
        case XTR_EOF_3:
            L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val);
            *buf++ = (u8)(val >> 0);
            read += 1;
            done = TRUE;
            break;
        case XTR_EOF_2:
            L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val);
            *buf++ = (u8)(val >> 0);
            *buf++ = (u8)(val >> 8);
            read += 2;
            done = TRUE;
            break;
        case XTR_EOF_1:
            L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val);
            *buf++ = (u8)(val >>  0);
            *buf++ = (u8)(val >>  8);
            *buf++ = (u8)(val >> 16);
            read += 3;
            done = TRUE;
            break;
        case XTR_PRUNED:
            rc = VTSS_RC_INCOMPLETE; /* But get the last 4 bytes as well */
            /* FALLTHROUGH */
        case XTR_EOF_0:
            done = TRUE;
            /* FALLTHROUGH */
        case XTR_ESCAPE:
            L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val);
            /* FALLTHROUGH */
        default:
            *buf++ = (u8)(val >>  0);
            *buf++ = (u8)(val >>  8);
            *buf++ = (u8)(val >> 16);
            *buf++ = (u8)(val >> 24);
            buflen -= 4, read += 4;
        }
    }
    
    if(!done) {                 /* Buffer overrun */
        (void) l26_rx_frame_discard(queue_no);
        rc = VTSS_RC_INCOMPLETE; /* ??? */
    }

    if(rc != VTSS_RC_ERROR)
        *bytes_read = read;

    return rc;
}

static inline u32 BYTE_SWAP(u32 v)
{
    register u32 v1 = v;
    v1 = ((v1 >> 24) & 0x000000FF) | ((v1 >> 8) & 0x0000FF00) | ((v1 << 8) & 0x00FF0000) | ((v1 << 24) & 0xFF000000);
    return v1;
}

static vtss_rc l26_rx_frame_get(const vtss_packet_rx_queue_t  queue_no,
                                vtss_packet_rx_header_t       *const header,
                                u8                            *const frame,
                                const u32                     length)
{
    u32                  val, port, found = 0;
    vtss_port_no_t       port_no;
    vtss_packet_rx_grp_t xtr_grp = vtss_state->rx_conf.grp_map[queue_no];
    
    L26_RD(VTSS_DEVCPU_QS_XTR_XTR_DATA_PRESENT, &val);
    /* CPUQ0 Got data ? */
    if(val & VTSS_F_DEVCPU_QS_XTR_XTR_DATA_PRESENT_DATA_PRESENT_GRP(VTSS_BIT(xtr_grp))) { 
        u32 ifh[2], i;
        u16 ethtype;
 
        /* Read IFH */
        for (i = 0; i < 2; i++) {
            L26_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &ifh[i]);
            ifh[i] = BYTE_SWAP(ifh[i]);
        }

        if (VTSS_EXTRACT_BITFIELD(ifh[0], 56 - 32, 8) != 0xFF) {
            VTSS_E("Invalid signature");
            return VTSS_RC_ERROR;
        }

        /* Note - VLAN tags are *not* stripped on ingress */
        header->tag.vid     = VTSS_EXTRACT_BITFIELD(ifh[1],  0, 12);
        header->tag.cfi     = VTSS_EXTRACT_BITFIELD(ifh[1], 12,  1);
        header->tag.tagprio = VTSS_EXTRACT_BITFIELD(ifh[1], 13,  3);
        header->queue_mask  = VTSS_EXTRACT_BITFIELD(ifh[1], 20,  8);
        header->learn      = (VTSS_EXTRACT_BITFIELD(ifh[1], 28,  2) ? 1 : 0);

        /* Map from chip port to API port */
        port = VTSS_EXTRACT_BITFIELD(ifh[0], 51 - 32, 5);
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (VTSS_CHIP_PORT(port_no) == port) {
                header->port_no = port_no;
                found = 1;
                break;
            }
        }
        if (!found) {
            VTSS_E("unknown chip port: %u", port);
            return VTSS_RC_ERROR;
        }
            
        VTSS_RC(l26_rx_frame_rd(queue_no, frame, length, &header->length));

        ethtype = (frame[12] << 8) + frame[13];
        header->arrived_tagged = (ethtype == VTSS_ETYPE_TAG_C || ethtype == VTSS_ETYPE_TAG_S); /* Emulated */

        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;       /* No data available */
}

static vtss_rc l26_tx_frame_port(const vtss_port_no_t  port_no,
                                 const u8              *const frame,
                                 const u32             length,
                                 const vtss_vid_t      vid)
{
    u32 status, w, count, last, port = VTSS_CHIP_PORT(port_no);
    const u8 *buf = frame;
    u32 ifh0, ifh1;

    VTSS_N("port_no: %u, length: %u, vid: %u", port_no, length, vid);

    L26_RD(VTSS_DEVCPU_QS_INJ_INJ_STATUS, &status);
    if (!(VTSS_X_DEVCPU_QS_INJ_INJ_STATUS_FIFO_RDY(status) & VTSS_BIT(CPU_INJ_REG))) {
        VTSS_E("not ready");
        return VTSS_RC_ERROR;
    }

    if (VTSS_X_DEVCPU_QS_INJ_INJ_STATUS_WMARK_REACHED(status) & VTSS_BIT(CPU_INJ_REG)) {
        VTSS_E("wm reached");
        return VTSS_RC_ERROR;
    }

    /* Indicate SOF */
    L26_WR(VTSS_DEVCPU_QS_INJ_INJ_CTRL(CPU_INJ_REG), VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_SOF);

    ifh0  = VTSS_ENCODE_BITFIELD(1,              63 - 32,  1); // BYPASS
    ifh0 |= VTSS_ENCODE_BITFIELD(VTSS_BIT(port), 32 - 32, 26); // Ignore the CPU port
    ifh1  = VTSS_ENCODE_BITFIELD(0x3,            28 -  0,  2); // Disable rewriter (this is the POP_CNT field of the injection header).

    /* IFH_0 */
    L26_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(CPU_INJ_REG), BYTE_SWAP(ifh0));
    
    /* IFH_1 */       
    L26_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(CPU_INJ_REG), BYTE_SWAP(ifh1));

    /* Write whole words */
    count = (length / 4);
    for (w = 0; w < count; w++, buf += 4) {
        if (w == 3 && vid != VTSS_VID_NULL) {
            /* Insert C-tag */
            L26_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(CPU_INJ_REG), BYTE_SWAP((0x8100U << 16) | vid));
            w++;
        }
        L26_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(CPU_INJ_REG), (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
    }

    /* Write last, partial word */
    last = (length % 4);
    if (last) {
        L26_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(CPU_INJ_REG), (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
        w++;
    }
        
    /* Add padding */
    while (w < (60 / 4)) {
        L26_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(CPU_INJ_REG), 0);
        w++;
    }
    
    /* Indicate EOF and valid bytes in last word */
    L26_WR(VTSS_DEVCPU_QS_INJ_INJ_CTRL(CPU_INJ_REG), 
           VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_VLD_BYTES(length < 60 ? 0 : last) |
           VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_EOF);

    /* Add dummy CRC */
    L26_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(CPU_INJ_REG), 0);
    w++;

    VTSS_N("wrote %u words, last: %u", w, last);

    return VTSS_RC_OK;
}

static vtss_rc l26_rx_hdr_decode(const vtss_state_t          *const state,
                                 const vtss_packet_rx_meta_t *const meta,
                                       vtss_packet_rx_info_t *const info)
{
    u64                 ifh;
    u32                 sflow_id;
    u8                  signature;
    vtss_phys_port_no_t chip_port;
    vtss_etype_t        etype;
    vtss_trace_group_t  trc_grp = meta->no_wait ? VTSS_TRACE_GROUP_FDMA_IRQ : VTSS_TRACE_GROUP_PACKET;

    VTSS_DG(trc_grp, "IFH:");
    VTSS_DG_HEX(trc_grp, &meta->bin_hdr[0], 8);

    ifh = ((u64)meta->bin_hdr[0] << 56) | ((u64)meta->bin_hdr[1] << 48) | ((u64)meta->bin_hdr[2] << 40) | ((u64)meta->bin_hdr[3] << 32) |
          ((u64)meta->bin_hdr[4] << 24) | ((u64)meta->bin_hdr[5] << 16) | ((u64)meta->bin_hdr[6] <<  8) | ((u64)meta->bin_hdr[7] <<  0);

    // Signature must be 0xFF, otherwise it's not a valid extraction header.
    signature = VTSS_EXTRACT_BITFIELD64(ifh, 56, 8);
    if (signature != 0xFF) {
        VTSS_EG(trc_grp, "Invalid Rx header signature. Expected 0xFF got 0x%02x", signature);
        return VTSS_RC_ERROR;
    }

    memset(info, 0, sizeof(*info));

    info->sw_tstamp = meta->sw_tstamp;
    info->length    = meta->length;
    info->glag_no   = VTSS_GLAG_NO_NONE;

    // Map from chip port to API port
    chip_port = VTSS_EXTRACT_BITFIELD64(ifh, 51, 5);
    info->port_no = vtss_cmn_chip_to_logical_port(state, 0, chip_port);
    if (info->port_no == VTSS_PORT_NO_NONE) {
        VTSS_EG(trc_grp, "Unknown chip port: %u", chip_port);
        return VTSS_RC_ERROR;
    }

    info->tag.pcp     = VTSS_EXTRACT_BITFIELD64(ifh, 13,  3);
    info->tag.dei     = VTSS_EXTRACT_BITFIELD64(ifh, 12,  1);
    info->tag.vid     = VTSS_EXTRACT_BITFIELD64(ifh,  0, 12);
    info->xtr_qu_mask = VTSS_EXTRACT_BITFIELD64(ifh, 20,  8);
    info->cos         = VTSS_EXTRACT_BITFIELD64(ifh, 17,  3);

    etype = (meta->bin_hdr[8 /* xtr info */ + 2 * 6 /* 2 MAC addresses*/] << 8) | (meta->bin_hdr[8 + 2 * 6 + 1]);
    VTSS_RC(vtss_cmn_packet_hints_update(state, trc_grp, etype, info));

    info->acl_hit = VTSS_EXTRACT_BITFIELD64(ifh, 31, 1);
    if (info->acl_hit) {
        // An additional check for PTP frame would have been better,
        // since tstamp_id_decoded = TRUE gets set for any frame that
        // comes to the CPU due to an IS2 rule.
        info->tstamp_id = VTSS_EXTRACT_BITFIELD64(ifh, 45, 6);
        info->tstamp_id_decoded = TRUE;

        info->acl_idx = VTSS_EXTRACT_BITFIELD64(ifh, 37, 8);
    }

    // sflow_id:
    // 0-26 : Frame has been SFlow sampled by a Tx sampler on port given by #sflow_id.
    // 27   : Frame has been SFlow sampled by an Rx sampler on port given by #port_no.
    // 28-30: Reserved.
    // 31   : Frame has not been SFlow sampled.
    sflow_id = VTSS_EXTRACT_BITFIELD64(ifh, 32, 5);
    if (sflow_id == 27) {
        info->sflow_type = VTSS_SFLOW_TYPE_RX;
        info->sflow_port_no = info->port_no;
    } else if (sflow_id < 27) {
        info->sflow_type = VTSS_SFLOW_TYPE_TX;
        info->sflow_port_no = vtss_cmn_chip_to_logical_port(state, 0, sflow_id);
    }
    return VTSS_RC_OK;
}

/*****************************************************************************/
// l26_ptp_action_to_ifh()
/*****************************************************************************/
static vtss_rc l26_ptp_action_to_ifh(vtss_packet_ptp_action_t ptp_action, u32 *result)
{
    vtss_rc rc = VTSS_RC_OK;

    switch (ptp_action) {
    case VTSS_PACKET_PTP_ACTION_NONE:
        *result = 0;
        break;
    case VTSS_PACKET_PTP_ACTION_ONE_STEP:
        *result = 1;
        break;
    case VTSS_PACKET_PTP_ACTION_TWO_STEP:
        *result = 2;
        break;
    case VTSS_PACKET_PTP_ACTION_ONE_AND_TWO_STEP:
        *result = 3;
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Invalid PTP action (%d)", ptp_action);
        rc = VTSS_RC_ERROR;
        break;
    }
    return rc;
}

static vtss_rc l26_tx_hdr_encode(      vtss_state_t          *const state,
                                 const vtss_packet_tx_info_t *const info,
                                       u8                    *const bin_hdr,
                                       u32                   *const bin_hdr_len)
{
    u64 inj_hdr;
    u32 required_ifh_size = info->ptp_action == 0 ? 8 : 8 + 4;

    if (bin_hdr == NULL) {
        // Caller wants us to return the number of bytes required to fill
        // in #bin_hdr. We need 8 or 12 bytes for the IFH depending on
        // whether we need to timestamp the frame.
        *bin_hdr_len = required_ifh_size;
        return VTSS_RC_OK;
    } else if (*bin_hdr_len < required_ifh_size) {
        return VTSS_RC_ERROR;
    }

    *bin_hdr_len = required_ifh_size;

    inj_hdr  = VTSS_ENCODE_BITFIELD64(!info->switch_frm,               63,  1); // BYPASS
    inj_hdr |= VTSS_ENCODE_BITFIELD64( info->cos >= 8 ? 7 : info->cos, 17,  3); // QoS Class.

    if (info->switch_frm) {
        if (info->tag.tpid != 0) {
            // If sending the frame switched, the caller should have inserted a VLAN tag in the packet
            // to get it classified to a particular VID. This is flagged to this function through
            // the tag.tpid member, which is non-zero if a tag is added. When a tag is added,
            // the frame still contains the tag at egress, so we have to remove it by setting POP_CNT to 1.
            inj_hdr |= VTSS_ENCODE_BITFIELD64(0x1, 28, 2);
        }
    } else {
        u64            chip_port_mask;
        vtss_chip_no_t chip_no;
        vtss_port_no_t stack_port_no, port_no;
        u32            port_cnt, ptp_action;

        VTSS_RC(vtss_cmn_logical_to_chip_port_mask(state, info->dst_port_mask, &chip_port_mask, &chip_no, &stack_port_no, &port_cnt, &port_no));

#ifdef VTSS_FEATURE_MIRROR_CPU
        // Add mirror port if enabled. Mirroring of directed frames must occur through the port mask.
        if (state->mirror_conf.port_no != VTSS_PORT_NO_NONE && state->mirror_cpu_ingress) {
            chip_port_mask |= VTSS_BIT64(VTSS_CHIP_PORT_FROM_STATE(state, state->mirror_conf.port_no));
        }
#endif

        // Destination port set.
        inj_hdr |= VTSS_ENCODE_BITFIELD64(chip_port_mask, 32, 26); // Ignore the CPU port

        // Inject frame as is (backwards compatibility with Luton28), i.e. disable rewriter.
        inj_hdr |= VTSS_ENCODE_BITFIELD64(0x3, 28, 2); // Disable rewriter (this is the POP_CNT field of the injection header).

        VTSS_RC(l26_ptp_action_to_ifh(info->ptp_action, &ptp_action));
        inj_hdr |= VTSS_ENCODE_BITFIELD64(ptp_action,   61, 2); // PTP_ACTION
        inj_hdr |= VTSS_ENCODE_BITFIELD64(info->ptp_id, 59, 2); // PTP_ID
    }

    // Time to store it as binary.
    bin_hdr[0] = inj_hdr >> 56;
    bin_hdr[1] = inj_hdr >> 48;
    bin_hdr[2] = inj_hdr >> 40;
    bin_hdr[3] = inj_hdr >> 32;
    bin_hdr[4] = inj_hdr >> 24;
    bin_hdr[5] = inj_hdr >> 16;
    bin_hdr[6] = inj_hdr >>  8;
    bin_hdr[7] = inj_hdr >>  0;

    // Lu26 has variable-length IFH. If PTP_ACTION != VTSS_PACKET_PTP_ACTION_NONE,  a 32-bit timestamp
    // is inserted in between the normal injection header and the first
    // byte of the DMAC.
    if (info->ptp_action != VTSS_PACKET_PTP_ACTION_NONE) {
        // Store the timestamp.
        bin_hdr[8 + 0] = info->ptp_timestamp >> 24;
        bin_hdr[8 + 1] = info->ptp_timestamp >> 16;
        bin_hdr[8 + 2] = info->ptp_timestamp >>  8;
        bin_hdr[8 + 3] = info->ptp_timestamp >>  0;
    }

    VTSS_IG(VTSS_TRACE_GROUP_PACKET, "IFH:");
    VTSS_IG_HEX(VTSS_TRACE_GROUP_PACKET, &bin_hdr[0], *bin_hdr_len);

    return VTSS_RC_OK;
}

static vtss_rc l26_poll_1sec(void)
{
    u32 port, bit, enable, rc=VTSS_RC_OK;

    L26_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_INT_ENA, &enable);
    for (port=0; port<32; ++port)
        if (!(enable & 1<<port))
            for (bit=0; bit<4; ++bit)    /* port is not enabled - check if it is configured to be */
                if (vtss_state->sgpio_event_enable[0].enable[port][bit]) {
                    rc = l26_sgpio_event_enable(0, 0, port, bit, TRUE);  /* this port,bit is configured to be enabled - try and enable */
                }
    return rc;
}


/* ================================================================= *
 *  Debug print
 * ================================================================= */

static void l26_debug_reg_header(const vtss_debug_printf_t pr, const char *name) 
{
    char buf[64];
    
    sprintf(buf, "%-18s  Tgt   Addr", name);
    vtss_debug_print_reg_header(pr, buf);
}

static void l26_debug_reg(const vtss_debug_printf_t pr, u32 addr, const char *name)
{
    u32 value;
    char buf[64];

    if (l26_rd(addr, &value) == VTSS_RC_OK) {
        sprintf(buf, "%-18s  0x%02x  0x%04x", name, (addr >> 14) & 0x3f, addr & 0x3fff);
        vtss_debug_print_reg(pr, buf, value);
    }
}

static void l26_debug_reg_inst(const vtss_debug_printf_t pr, u32 addr, u32 i, const char *name)
{
    char buf[64];

    sprintf(buf, "%s_%u", name, i);
    l26_debug_reg(pr, addr, buf);
}

static void l26_debug_print_mask(const vtss_debug_printf_t pr, u32 mask)
{
    u32 port;
    
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        pr("%s%s", port == 0 || (port & 7) ? "" : ".", ((1<<port) & mask) ? "1" : "0");
    }
    pr("  0x%08x\n", mask);
}

static void l26_debug_print_header(const vtss_debug_printf_t pr, const char *txt)
{
    pr("%s:\n\n", txt);
}

static void l26_debug_print_port_header(const vtss_debug_printf_t pr, const char *txt)
{
    vtss_debug_print_port_header(pr, txt, VTSS_CHIP_PORTS + 1, 1);
}

static void l26_debug_bits(const vtss_debug_printf_t pr, 
                           const char *name, u32 *entry, u32 *mask, u32 offset, u32 len)
{
    u32 i,j;

    if (name)
        pr("%s: ", name);
    for (i = 0; i < len; i++) {
        j = (len - 1 - i);
        if (i != 0 && (j % 8) == 7)
            pr(".");
        j += offset;
        pr("%c", mask == NULL || vtss_bs_bit_get(mask, j) ? 
           (vtss_bs_bit_get(entry, j) ? '1' : '0') : 'X');
    }
    pr(len > 23 ? "\n" : " ");
}

static void l26_debug_bit(const vtss_debug_printf_t pr, 
                          const char *name, u32 *entry, u32 *mask, u32 offset)
{
    l26_debug_bits(pr, name, entry, mask, offset, 1);
}

static void l26_debug_mac_bits(const vtss_debug_printf_t pr, 
                               const char *name, u32 *entry, u32 *mask, u32 offset)
{
    l26_debug_bits(pr, name, entry, mask, offset, 16);      /* MAC_HIGH */
    l26_debug_bits(pr, NULL, entry, mask, offset + 16, 32); /* MAC_LOW */
}

static void l26_debug_action(const vtss_debug_printf_t pr, 
                             const char *name, u32 *entry, u32 offs, u32 offs_val, u32 len)
{
    BOOL enable, multi = 0;
    u32  num = 0;
    int  i, length = strlen(name);

    if (offs_val && (offs_val - offs) != 1) {
        /* 'Enable' field consists of multiple bits */
        multi = 1;
        num = vtss_bs_get(entry, offs, offs_val - offs);
        enable = (num ? 1 : 0);
    } else
        enable = vtss_bs_bit_get(entry, offs);
    
    for (i = 0; i < length; i++)
        pr("%c", enable ? toupper(name[i]) : tolower(name[i]));

    if (len) {
        pr(":");
        if (multi)
            pr("%u/", num);
        pr("%u", vtss_bs_get(entry, offs_val, len));
    }
    pr(" ");
}

static void l26_debug_fld(const vtss_debug_printf_t pr, 
                          const char *name, u32 *entry, u32 offs, u32 len)
{
    pr("%s:%u ", name, vtss_bs_get(entry, offs, len));
}

static vtss_rc l26_debug_is1(u32 *entry, u32 *mask, u32 typegroup, u32 cnt, 
                             const vtss_debug_printf_t pr)
{
    if (mask == NULL) {
        /* Print action */
        if (vtss_bs_get(entry, 0, 2) == IS1_ACTION_TYPE_NORMAL) {
            /* Normal IS1 action */
            l26_debug_action(pr, "dscp", entry, 2, 3, 6);   /* DSCP_ENA/DSCP_VAL */
            l26_debug_action(pr, "dp", entry, 9, 10, 1);    /* DP_ENA/DP_VAL */
            l26_debug_action(pr, "qos", entry, 11, 12, 3);  /* QOS_ENA/QOS_VAL */
            l26_debug_action(pr, "pag", entry, 15, 16, 8);  /* PAG_ENA/PAG_VAL */
            l26_debug_action(pr, "vid", entry, 24, 25, 12); /* VID_ENA/VID_VAL */
            l26_debug_action(pr, "pcp", entry, 51, 52, 3);  /* PCP_DEI_ENA/PCP_VAL */
            l26_debug_action(pr, "dei", entry, 55, 0, 0);   /* DEI_VAL */
            l26_debug_action(pr, "pop", entry, 56, 57, 2);  /* POP_ENA/POP_VAL */
            l26_debug_action(pr, "hm", entry, 59, 0, 0);    /* HOST_MATCH */
            l26_debug_action(pr, "fid", entry, 37, 39, 12); /* FID_SEL & FID_VAL */
        } else {
            /* SMAC/SIP action */
            l26_debug_action(pr, "hm", entry, 2, 0, 0); /* HOST_MATCH */
        }
        pr("cnt:%u", cnt);                             /* HIT_STICKY */
        return VTSS_RC_OK;
    }
    
    /* Print entry */
    if (typegroup == VCAP_TG_VAL_IS1_IP4) {
        /* SMAC/SIP4 entry */
        l26_debug_bits(pr, "igr_port", entry, mask, 0, 5);
        pr("\n");
        l26_debug_mac_bits(pr, "l2_smac", entry, mask, 5);
        l26_debug_bits(pr, "l3_ip_sip", entry, mask, 53, 32);
        return VTSS_RC_OK;
    }

    l26_debug_bit(pr, "is1_type", entry, mask, 0);
    if (vtss_bs_get(entry, 0, 1)) {
        /* SMAC/SIP6 entry */
        l26_debug_bits(pr, "igr_port", entry, mask, 1, 5);
        pr("\n");
        l26_debug_mac_bits(pr, "l2_smac", entry, mask, 6);
        l26_debug_bits(pr, "sip_3", entry, mask, 54, 32);
        l26_debug_bits(pr, "sip_2", entry, mask, 86, 32);
        l26_debug_bits(pr, "sip_1", entry, mask, 118, 32);
        l26_debug_bits(pr, "sip_0", entry, mask, 150, 32);
        return VTSS_RC_OK;
    }

    l26_debug_bit(pr, "first", entry, mask, 1);
    l26_debug_bits(pr, "igr_port_mask", entry, mask, 2, 27);
    l26_debug_bit(pr, "vlan_tagged", entry, mask, 29);
    l26_debug_bit(pr, "vlan_dbl_tagged", entry, mask, 30);
    l26_debug_bit(pr, "tpid", entry, mask, 31);
    pr("\n");
    l26_debug_bits(pr, "vid", entry, mask, 32, 12);
    l26_debug_bit(pr, "dei", entry, mask, 44);
    l26_debug_bits(pr, "pcp", entry, mask, 45, 3);
    pr("\n");
    l26_debug_mac_bits(pr, "l2_smac", entry, mask, 48);
    l26_debug_bit(pr, "l2_mc", entry, mask, 96);
    l26_debug_bit(pr, "l2_bc", entry, mask, 97);
    l26_debug_bit(pr, "ip_mc", entry, mask, 98);
    pr("\n");
    l26_debug_bits(pr, "etype", entry, mask, 100, 16);
    pr("\n");
    l26_debug_bit(pr, "etype_len", entry, mask, 99);
    l26_debug_bit(pr, "ip_snap", entry, mask, 116);
    l26_debug_bit(pr, "ip4", entry, mask, 117);
    pr("\n");
    l26_debug_bit(pr, "l3_fragment", entry, mask, 118);
    l26_debug_bit(pr, "l3_options", entry, mask, 119);
    l26_debug_bits(pr, "l3_dscp", entry, mask, 120, 6);
    pr("\n");
    l26_debug_bits(pr, "l3_ip4_sip", entry, mask, 126, 32);
    l26_debug_bit(pr, "tcp_udp", entry, mask, 158);
    l26_debug_bit(pr, "tcp", entry, mask, 159);
    l26_debug_bits(pr, "l4_sport", entry, mask, 160, 16);
    l26_debug_bits(pr, "l4_rng", entry, mask, 176, 8);
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc l26_debug_is2(u32 *entry, u32 *mask, u32 typegroup, u32 cnt, 
                             const vtss_debug_printf_t pr)
{
    u32 is2type, mode;

    if (mask == NULL) {
        /* Print action */
        mode = vtss_bs_get(entry, 5, 2);              /* MASK_MODE */
        pr("mode:%u (%s) ", 
           mode, 
           mode == 0 ? "none" : mode == 1 ? "filter" : mode == 2 ? "policy" : "redir");
        l26_debug_bits(pr, "mask", entry, NULL, 18, 26); /* PORT_MASK */
        l26_debug_action(pr, "hit", entry, 0, 0, 0);  /* HIT_ME_ONCE */
        l26_debug_action(pr, "cpu", entry, 1, 2, 3);  /* CPU_COPY_ENA/CPU_QU_NUM */
        l26_debug_action(pr, "mir", entry, 7, 0, 0);  /* MIRROR_ENA */
        l26_debug_action(pr, "lrn", entry, 8, 0, 0);  /* LRN_DIS */
        l26_debug_action(pr, "pol", entry, 9, 10, 8); /* POLICER_ENA/POLICE_IDX */
        l26_debug_action(pr, "one", entry, 44, 0, 0); /* PTP_ENA[0] */
        l26_debug_action(pr, "two", entry, 45, 0, 0); /* PTP_ENA[1] */
        pr("cnt:%u ", cnt);                          /* HIT_CNT */
        return VTSS_RC_OK;
    }
    
    /* Print entry */
    l26_debug_bits(pr, "type", entry, mask, 0, 3);
    is2type = vtss_bs_get(entry, 0, 3);
    pr("(%s) ", 
       vtss_bs_get(mask, 0, 3) == 0 ? "any" :
       is2type == ACL_TYPE_ETYPE ? "etype" :
       is2type == ACL_TYPE_LLC ? "llc" :
       is2type == ACL_TYPE_SNAP ? "snap" :
       is2type == ACL_TYPE_ARP ? "arp" :
       is2type == ACL_TYPE_UDP_TCP ? "udp_tcp" :
       is2type == ACL_TYPE_IPV4 ? "ipv4" :
       is2type == ACL_TYPE_IPV6 ? "ipv6" : "?");
    l26_debug_bit(pr, "first", entry, mask, 3);
    l26_debug_bits(pr, "pag", entry, mask, 4, 8);
    l26_debug_bits(pr, "igr", entry, mask, 12, 27);
    l26_debug_bit(pr, "vlan", entry, mask, 39);
    l26_debug_bit(pr, "host", entry, mask, 40);
    l26_debug_bits(pr, "vid", entry, mask, 41, 12);
    l26_debug_bit(pr, "dei", entry, mask, 53);
    l26_debug_bits(pr, "pcp", entry, mask, 54, 3);
    pr("\n");
    
    switch(is2type) {
    case ACL_TYPE_ETYPE:
    case ACL_TYPE_LLC:
    case ACL_TYPE_SNAP:
        /* Common format */
        l26_debug_mac_bits(pr, "l2_dmac", entry, mask, 57);
        l26_debug_mac_bits(pr, "l2_smac", entry, mask, 105);
        l26_debug_bit(pr, "l2_mc", entry, mask, 153);
        l26_debug_bit(pr, "l2_bc", entry, mask, 154);
        pr("\n");
        
        /* Specific format */
        switch (is2type) {
        case ACL_TYPE_ETYPE:
            l26_debug_bits(pr, "etype", entry, mask, 155, 16);
            l26_debug_bits(pr, "l2_payload", entry, mask, 171, 16);
            pr("\n");
            break;
        case ACL_TYPE_LLC:
            l26_debug_bits(pr, "l2_llc", entry, mask, 155, 32);
            break;
        case ACL_TYPE_SNAP:
            l26_debug_bits(pr, "l2_snap", entry, mask, 155, 40);
            break;
        }
        break;
    case ACL_TYPE_ARP:
        l26_debug_mac_bits(pr, "l2_smac", entry, mask, 57);
        l26_debug_bit(pr, "l2_mc", entry, mask, 105);
        l26_debug_bit(pr, "l2_bc", entry, mask, 106);
        l26_debug_bit(pr, "addr_ok", entry, mask, 107);
        l26_debug_bit(pr, "proto_ok", entry, mask, 108);
        l26_debug_bit(pr, "len_ok", entry, mask, 109);
        pr("\n");
        l26_debug_bit(pr, "t_match", entry, mask, 110);
        l26_debug_bit(pr, "s_match", entry, mask, 111);
        l26_debug_bit(pr, "op_unk", entry, mask, 112);
        l26_debug_bits(pr, "op", entry, mask, 113, 2);
        l26_debug_bit(pr, "dip_eq_sip", entry, mask, 179);
        pr("\n");
        l26_debug_bits(pr, "l3_ip4_dip", entry, mask, 115, 32);
        l26_debug_bits(pr, "l3_ip4_sip", entry, mask, 147, 32);
        break;
    case ACL_TYPE_UDP_TCP:
    case ACL_TYPE_IPV4:
        /* Common format */
        l26_debug_bit(pr, "l2_mc", entry, mask, 57);
        l26_debug_bit(pr, "l2_bc", entry, mask, 58);
        l26_debug_bit(pr, "ip4", entry, mask, 59);
        l26_debug_bit(pr, "l3_fragment", entry, mask, 60);
        l26_debug_bit(pr, "l3_fragoffs", entry, mask, 61);
        l26_debug_bit(pr, "l3_options", entry, mask, 62);
        l26_debug_bit(pr, "l3_ttl", entry, mask, 63);
        pr("\n");
        l26_debug_bits(pr, "l3_tos", entry, mask, 64, 8);
        
        /* Specific format */
        if (is2type == ACL_TYPE_UDP_TCP) {
            pr("\n");
            l26_debug_bits(pr, "l3_ip4_dip", entry, mask, 81, 32);
            l26_debug_bits(pr, "l3_ip4_sip", entry, mask, 113, 32);
            l26_debug_bits(pr, "l4_dport", entry, mask, 146, 16);
            l26_debug_bits(pr, "l4_sport", entry, mask, 162, 16);
            l26_debug_bits(pr, "l4_rng", entry, mask, 178, 8);
            pr("\n");
            l26_debug_bit(pr, "tcp", entry, mask, 72);
            l26_debug_bits(pr, "1588_dom", entry, mask, 73, 8);
            l26_debug_bit(pr, "dip_eq_sip", entry, mask, 145);
            l26_debug_bit(pr, "sp_eq_dp", entry, mask, 186);
            pr("\n");
            l26_debug_bit(pr, "seq_eq0", entry, mask, 187);
            l26_debug_bit(pr, "fin", entry, mask, 188);
            l26_debug_bit(pr, "syn", entry, mask, 189);
            l26_debug_bit(pr, "rst", entry, mask, 190);
            l26_debug_bit(pr, "psh", entry, mask, 191);
            l26_debug_bit(pr, "ack", entry, mask, 192);
            l26_debug_bit(pr, "urg", entry, mask, 193);
            pr("\n");
        } else {
            l26_debug_bits(pr, "proto", entry, mask, 72, 8);
            l26_debug_bit(pr, "dip_eq_sip", entry, mask, 144);
            pr("\n");
            l26_debug_bits(pr, "l3_ip4_dip", entry, mask, 80, 32);
            l26_debug_bits(pr, "l3_ip4_sip", entry, mask, 112, 32);
            l26_debug_mac_bits(pr, "l3_payload", entry, mask, 145);
        }
        break;
    case ACL_TYPE_IPV6:
        l26_debug_bit(pr, "l2_mc", entry, mask, 57);
        l26_debug_bit(pr, "l2_bc", entry, mask, 58);
        l26_debug_bits(pr, "proto", entry, mask, 59, 8);
        pr("\n");
        l26_debug_bits(pr, "sip_3", entry, mask, 67, 32);
        l26_debug_bits(pr, "sip_2", entry, mask, 99, 32);
        l26_debug_bits(pr, "sip_1", entry, mask, 131, 32);
        l26_debug_bits(pr, "sip_0", entry, mask, 163, 32);
        break;
    default:
        VTSS_E("Invalid IS2 type: %d", is2type);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}
    
static vtss_rc l26_debug_es0(u32 *entry, u32 *mask, u32 typegroup, u32 cnt, 
                             const vtss_debug_printf_t pr)
{
    u32 val;

    if (mask == NULL) {
        /* Print action */
        l26_debug_bit(pr, "vld", entry, NULL, 0); /* VLD */
        val = vtss_bs_get(entry, 1, 2);           /* TAG_ES0 */
        pr("tag_es0:%u (%s) ", 
           val, 
           val == VTSS_ES0_TAG_NONE ? "none" : val == VTSS_ES0_TAG_ES0 ? "es0" : 
           val == VTSS_ES0_TAG_PORT ? "port" : "both");
        val = vtss_bs_get(entry, 3, 2);               /* TAG_TPID_SEL */
        pr("tag_tpid_sel:%u (%s) ",
           val,
           val == VTSS_ES0_TPID_C ? "c" : val == VTSS_ES0_TPID_S ? "s" : 
           val == VTSS_ES0_TPID_PORT ? "port" : "c/port");
        l26_debug_fld(pr, "tag_vid_sel", entry, 5, 2); /* TAG_VID_SEL */
        pr("\n");
        l26_debug_fld(pr, "vid_a", entry, 7, 12);      /* VID_A_VAL */
        l26_debug_fld(pr, "vid_b", entry, 19, 12);     /* VID_B_VAL */
        val = vtss_bs_get(entry, 31, 2);               /* QOS_SRC_SEL */
        pr("qos_src_sel:%u (%s) ",
           val,
           val == VTSS_ES0_QOS_CLASS ? "class" : val == VTSS_ES0_QOS_ES0 ? "es0" :
           val == VTSS_ES0_QOS_PORT ? "port" : "mapped");
        l26_debug_fld(pr, "pcp", entry, 33, 3);    /* PCP_VAL */
        l26_debug_bit(pr, "dei", entry, NULL, 36); /* DEI_VAL */
        pr("cnt:%u ", cnt);                       /* HIT_CNT */
        return VTSS_RC_OK;
    }
    
    /* Print entry */
    l26_debug_bits(pr, "egr_port", entry, mask, 0, 5);
    l26_debug_bits(pr, "igr_port", entry, mask, 5, 5);
    pr("\n");
    l26_debug_bits(pr, "vid", entry, mask, 10, 12);
    l26_debug_bit(pr, "dei", entry, mask, 22);
    l26_debug_bits(pr, "pcp", entry, mask, 23, 3);
    l26_debug_bit(pr, "l2_mc", entry, mask, 26);
    l26_debug_bit(pr, "l2_bc", entry, mask, 27);
    pr("\n");
    
    return VTSS_RC_OK;
}
    
#define L26_DEBUG_VCAP(pr, name, tgt) l26_debug_reg(pr, VTSS_VCAP_CORE_VCAP_CONST_##name(tgt), #name)

static vtss_rc l26_debug_vcap(int bank,
                              const char *name,
                              const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info,
                              vtss_rc (* dbg)(u32 *entry, u32 *mask, u32 typegroup, u32 cnt,
                                              const vtss_debug_printf_t pr))
{
    /*lint --e{454, 455, 456) */ // Due to VTSS_EXIT_ENTER
    const tcam_props_t *tcam = &tcam_info[bank];
    u32                entry[VTSS_TCAM_ENTRY_WIDTH];
    u32                mask[VTSS_TCAM_ENTRY_WIDTH];
    int                i, j, sel;
    u32                cnt, typegroup, tgt;
    BOOL               is_entry;
    
    l26_debug_print_header(pr, name);
    
    l26_debug_reg_header(pr, "VCAP_CONST");
    tgt = tcam->target;
    L26_DEBUG_VCAP(pr, ENTRY_WIDTH, tgt);
    L26_DEBUG_VCAP(pr, ENTRY_CNT, tgt);
    L26_DEBUG_VCAP(pr, ENTRY_SWCNT, tgt);
    L26_DEBUG_VCAP(pr, ENTRY_TG_WIDTH, tgt);
    L26_DEBUG_VCAP(pr, ACTION_DEF_CNT, tgt);
    L26_DEBUG_VCAP(pr, ACTION_WIDTH, tgt);
    L26_DEBUG_VCAP(pr, CNT_WIDTH, tgt);
    pr("\n");
    
    for (i = (tcam->actions - 1); i >= 0; i--) {
        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();

        /* Read entry */
        is_entry = (i < tcam->entries);
        if (is_entry &&
            l26_vcap_command(tcam, i, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_ENTRY) == VTSS_RC_OK &&
            l26_vcap_cache2entry(tcam, entry, mask) == VTSS_RC_OK) {
            L26_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_TG_DAT(tcam->target), &typegroup);
            
            /* Skip invalid entries */
            if (typegroup == 0)
                continue;

            /* Print entry */
            VTSS_RC(dbg(entry, mask, typegroup, 0, pr));
            pr("\n");

            if (info->full) {
                l26_debug_reg_header(pr, "VCAP");
                l26_debug_reg(pr, VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_TG_DAT(tcam->target), "TG_DAT");
                for (j = 0; j < tcam->entry_width; j++) {
                    l26_debug_reg_inst(pr, VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ENTRY_DAT(tcam->target, j),
                                       j, "ENTRY_DAT");
                    l26_debug_reg_inst(pr, VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_MASK_DAT(tcam->target, j),
                                       j, "ENTRY_MASK");
                }
                pr("\n");
            }
        } else
            typegroup = 0;
        
        /* Read action and counter */
        sel = (VTSS_TCAM_SEL_ACTION | VTSS_TCAM_SEL_COUNTER);
        if (l26_vcap_command(tcam, i, VTSS_TCAM_CMD_READ, sel) != VTSS_RC_OK ||
            l26_vcap_cache2action(tcam, entry) != VTSS_RC_OK)
            continue;
        L26_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tcam->target, 0), &cnt);

        /* Clear counter and write back */
        if (info->full) {
            L26_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tcam->target, 0), 0);
            if (l26_vcap_command(tcam, i, VTSS_TCAM_CMD_WRITE, sel) != VTSS_RC_OK)
                continue;
        }
        
        /* Print action */
        pr("%d (%s %d): ", i, is_entry ? "rule" : "port", 
           is_entry ? (tcam->entries - i - 1) : (i - tcam->entries));
        VTSS_RC(dbg(entry, NULL, typegroup, cnt, pr));
        pr("\n-------------------------------------------\n");
    }
    pr("\n");
    return VTSS_RC_OK;
}
    
static vtss_rc l26_debug_vcap_is1(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    return l26_debug_vcap(VTSS_TCAM_S1, "IS1", pr, info, l26_debug_is1);
}

static vtss_rc l26_debug_vcap_es0(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    return l26_debug_vcap(VTSS_TCAM_ES0, "ES0", pr, info, l26_debug_es0);
}

static vtss_rc l26_debug_range_checkers(const vtss_debug_printf_t pr,
                                        const vtss_debug_info_t   *const info)
{
    u32 i;
    
    l26_debug_reg_header(pr, "Range Checkers");
    for (i = 0; i < VTSS_VCAP_RANGE_CHK_CNT; i++) {
        l26_debug_reg_inst(pr, VTSS_ANA_COMMON_VCAP_RNG_TYPE_CFG(i), i, "RNG_TYPE_CFG");
        l26_debug_reg_inst(pr, VTSS_ANA_COMMON_VCAP_RNG_VAL_CFG(i), i, "RNG_VAL_CFG");
    }
    pr("\n");
    return VTSS_RC_OK;
}

static vtss_rc l26_debug_vcap_port(const vtss_debug_printf_t pr,
                                   const vtss_debug_info_t   *const info)
{
    u32 port;
    
    l26_debug_reg_header(pr, "ANA:VCAP_CFG");
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        l26_debug_reg_inst(pr, VTSS_ANA_PORT_VCAP_CFG(port), port, "VCAP_CFG");
    }
    pr("\n");
    return VTSS_RC_OK;
}

static vtss_rc l26_debug_acl(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    VTSS_RC(l26_debug_range_checkers(pr, info));
    VTSS_RC(l26_debug_vcap_port(pr, info));
    VTSS_RC(l26_debug_vcap_is1(pr, info));
    VTSS_RC(l26_debug_vcap(VTSS_TCAM_S2, "IS2", pr, info, l26_debug_is2));
    
    return VTSS_RC_OK;
}

static vtss_rc l26_debug_ipmc(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455) */ // Due to VTSS_EXIT_ENTER
    vtss_mac_entry_t        *entry;
    vtss_vid_mac_t          vid_mac;
    vtss_mac_table_entry_t  mac_entry;
    u8                      *p = &vid_mac.mac.addr[0];
    BOOL                    header = TRUE;
    u32                     pgid;

    VTSS_RC(l26_debug_range_checkers(pr, info));
    VTSS_RC(l26_debug_vcap_port(pr, info));
    VTSS_RC(l26_debug_vcap_is1(pr, info));

    /* MAC address table in state */
    for (entry = vtss_state->mac_list_used; entry != NULL; entry = entry->next) {
        if (header)
            vtss_debug_print_port_header(pr, "VID   MAC                CPU  User  ", 0, 1);
        header = FALSE;
        vtss_mach_macl_set(&vid_mac, entry->mach, entry->macl);
        pr("%-4u  %02x-%02x-%02x-%02x-%02x-%02x  %-3u  %-4x  ",
           vid_mac.vid, p[0], p[1], p[2], p[3], p[4], p[5], entry->cpu_copy, entry->user);
        vtss_debug_print_ports(pr, entry->member, 1);
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");

    /* MAC address table in chip */
    header = TRUE;
    memset(&mac_entry, 0, sizeof(mac_entry));
    p = &mac_entry.vid_mac.mac.addr[0];
    while (vtss_state->cil_func.mac_table_get_next != NULL &&
           vtss_state->cil_func.mac_table_get_next(&mac_entry, &pgid) == VTSS_RC_OK) {
        if (header)
            pr("VID   MAC                PGID  CPU  Locked\n");
        header = FALSE;
        pr("%-4u  %02x-%02x-%02x-%02x-%02x-%02x  %-4u  %-3u  %u\n",
           mac_entry.vid_mac.vid, p[0], p[1], p[2], p[3], p[4], p[5],
           pgid, mac_entry.copy_to_cpu, mac_entry.locked);
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc l26_debug_vxlat(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    VTSS_RC(l26_debug_vcap_is1(pr, info));
    VTSS_RC(l26_debug_vcap_es0(pr, info));
    
    return VTSS_RC_OK;
}

static void l26_debug_tgt(const vtss_debug_printf_t pr, const char *name, u32 to)
{
    u32 tgt = ((to >> 16) & 0x3f);
    pr("%-10s  0x%02x (%u)\n", name, tgt, tgt);
}

#define L26_DEBUG_TGT(pr, name) l26_debug_tgt(pr, #name, VTSS_TO_##name)
#define L26_DEBUG_GPIO(pr, addr, name) l26_debug_reg(pr, VTSS_DEVCPU_GCB_GPIO_GPIO_##addr, "GPIO_"name)
#define L26_DEBUG_SIO(pr, addr, name) l26_debug_reg(pr, VTSS_DEVCPU_GCB_SIO_CTRL_SIO_##addr, "SIO_"name)
#define L26_DEBUG_SIO_INST(pr, addr, i, name) l26_debug_reg_inst(pr, VTSS_DEVCPU_GCB_SIO_CTRL_SIO_##addr, i, "SIO_"name)

static vtss_rc l26_debug_misc(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    u32  port, i;
    char buf[32];
    
    pr("Name        Target\n");
    L26_DEBUG_TGT(pr, DEVCPU_ORG);
    L26_DEBUG_TGT(pr, SYS);
    L26_DEBUG_TGT(pr, ANA);
    L26_DEBUG_TGT(pr, REW);
    L26_DEBUG_TGT(pr, ES0);
    L26_DEBUG_TGT(pr, S1);
    L26_DEBUG_TGT(pr, S2);
    L26_DEBUG_TGT(pr, DEVCPU_GCB);
    L26_DEBUG_TGT(pr, DEVCPU_QS);
    L26_DEBUG_TGT(pr, DEVCPU_PI);
    L26_DEBUG_TGT(pr, MACRO_CTRL);
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        sprintf(buf, "DEV_%u", port);
        l26_debug_tgt(pr, buf, VTSS_TO_DEV(port));
    }
    pr("\n");

    l26_debug_reg_header(pr, "GPIOs");
    L26_DEBUG_GPIO(pr, OUT, "OUT");
    L26_DEBUG_GPIO(pr, IN, "IN");
    L26_DEBUG_GPIO(pr, OE, "OE");
    L26_DEBUG_GPIO(pr, INTR, "INTR");
    L26_DEBUG_GPIO(pr, INTR_ENA, "INTR_ENA");
    L26_DEBUG_GPIO(pr, INTR_IDENT, "INTR_IDENT");
    L26_DEBUG_GPIO(pr, ALT(0), "ALT_0");
    L26_DEBUG_GPIO(pr, ALT(1), "ALT_1");
    pr("\n");
    
    l26_debug_reg_header(pr, "SGPIO");
    for (i = 0; i < 4; i++)
        L26_DEBUG_SIO_INST(pr, INPUT_DATA(i), i, "INPUT_DATA");
    for (i = 0; i < 4; i++)
        L26_DEBUG_SIO_INST(pr, INT_POL(i), i, "INT_POL");
    L26_DEBUG_SIO(pr, PORT_INT_ENA, "PORT_INT_ENA");
    for (i = 0; i < 32; i++)
        L26_DEBUG_SIO_INST(pr, PORT_CONFIG(i), i, "PORT_CONFIG");
    L26_DEBUG_SIO(pr, PORT_ENABLE, "PORT_ENABLE");
    L26_DEBUG_SIO(pr, CONFIG, "CONFIG");
    L26_DEBUG_SIO(pr, CLOCK, "CLOCK");
    for (i = 0; i < 4; i++)
        L26_DEBUG_SIO_INST(pr, INT_REG(i), i, "INT_REG");
    pr("\n");
    
    return VTSS_RC_OK;
}

#define L26_DEBUG_HSIO(pr, addr, name) l26_debug_reg(pr, VTSS_MACRO_CTRL_##addr, name)
#define L26_DEBUG_MAC(pr, addr, i, name) l26_debug_reg_inst(pr, VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_##addr, i, "MAC_"name)
#define L26_DEBUG_PCS(pr, addr, i, name) l26_debug_reg_inst(pr, VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_##addr, i, "PCS_"name)

static vtss_rc l26_debug_port(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    u32                          i, value, port, tgt;
    vtss_port_no_t               port_no;
    char                         buf[32];
    
    L26_RD(VTSS_DEVCPU_GCB_MISC_MISC_CFG, &value);
    pr("Target  : 0x%04x/0x%04x\n", vtss_state->create.target, vtss_state->chip_id.part_number);
    pr("Mux Mode: %u/%u\n\n", 
       vtss_state->init_conf.mux_mode, VTSS_X_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE(value));

    for (i = 0; info->full && i < 8; i++) {
        sprintf(buf, "SerDes1G_%u", i);
        l26_debug_reg_header(pr, buf);
        VTSS_RC(l26_sd1g_read(1 << i));
        L26_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_DES_CFG, "DES_CFG");
        L26_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_IB_CFG, "IB_CFG");
        L26_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_OB_CFG, "OB_CFG");
        L26_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_SER_CFG, "SER_CFG");
        L26_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, "COMMON_CFG");
        L26_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_PLL_CFG, "PLL_CFG");
        L26_DEBUG_HSIO(pr, SERDES1G_ANA_STATUS_SERDES1G_PLL_STATUS, "PLL_STATUS");
        L26_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG0, "DFT_CFG0");
        L26_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG1, "DFT_CFG1");
        L26_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG2, "DFT_CFG2");
        L26_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_TP_CFG, "TP_CFG");
        L26_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_MISC_CFG, "MISC_CFG");
        L26_DEBUG_HSIO(pr, SERDES1G_DIG_STATUS_SERDES1G_DFT_STATUS, "DFT_STATUS");
        pr("\n");
    }

    for (i = 0; info->full && i < 4; i++) {
        sprintf(buf, "SerDes6G_%u", i);
        l26_debug_reg_header(pr, buf);
        VTSS_RC(l26_sd6g_read(1 << i));
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, "DES_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, "IB_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, "IB_CFG1");
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, "OB_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, "OB_CFG1");
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, "SER_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, "COMMON_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, "PLL_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_ANA_STATUS_SERDES6G_PLL_STATUS, "PLL_STATUS");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DIG_CFG, "DIG_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG0, "DFT_CFG0");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG1, "DFT_CFG1");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG2, "DFT_CFG2");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_TP_CFG0, "TP_CFG0");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_TP_CFG1, "TP_CFG1");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, "MISC_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_OB_ANEG_CFG, "OB_ANEG_CFG");
        L26_DEBUG_HSIO(pr, SERDES6G_DIG_STATUS_SERDES6G_DFT_STATUS, "DFT_STATUS");
        pr("\n");
    }
    pr("\n");

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (info->port_list[port_no] == 0)
            continue;
        port = VTSS_CHIP_PORT(port_no);
        sprintf(buf, "Port %u", port);
        l26_debug_reg_header(pr, buf);
        tgt = VTSS_TO_DEV(port);
        l26_debug_reg_inst(pr, 
                           port > 9 ? VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt) :
                           VTSS_DEV_GMII_PORT_MODE_CLOCK_CFG(tgt), 
                           port, "CLOCK_CFG");
        L26_DEBUG_MAC(pr, ENA_CFG(tgt), port, "ENA_CFG");
        L26_DEBUG_MAC(pr, MODE_CFG(tgt), port, "MODE_CFG");
        L26_DEBUG_MAC(pr, MAXLEN_CFG(tgt), port, "MAXLEN_CFG");
        L26_DEBUG_MAC(pr, TAGS_CFG(tgt), port, "TAGS_CFG");
        L26_DEBUG_MAC(pr, FC_CFG(tgt), port, "FC_CFG");
        L26_DEBUG_PCS(pr, CFG(tgt), port, "CFG");
        L26_DEBUG_PCS(pr, MODE_CFG(tgt), port, "MODE_CFG");
        L26_DEBUG_PCS(pr, SD_CFG(tgt), port, "SD_CFG");
        L26_DEBUG_PCS(pr, ANEG_CFG(tgt), port, "ANEG_CFG");
        L26_DEBUG_PCS(pr, ANEG_STATUS(tgt), port, "ANEG_STATUS");
        L26_DEBUG_PCS(pr, LINK_STATUS(tgt), port, "LINK_STATUS");
        l26_debug_reg_inst(pr, VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt), port, 
                           "PCS_FX100_CFG");
        l26_debug_reg_inst(pr, VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), port, 
                           "FX100_STATUS");
        pr("\n");
    }
    pr("\n");

    return VTSS_RC_OK;
}

static void l26_debug_cnt(const vtss_debug_printf_t pr, const char *col1, const char *col2, 
                          vtss_chip_counter_t *c1, vtss_chip_counter_t *c2)
{
    char buf[80];
    
    sprintf(buf, "rx_%s:", col1);
    pr("%-19s%19llu   ", buf, c1->prev);
    if (col2 != NULL) {
        sprintf(buf, "tx_%s:", strlen(col2) ? col2 : col1);
        pr("%-19s%19llu", buf, c2->prev);
    }
    pr("\n");
}

static void l26_debug_cnt_inst(const vtss_debug_printf_t pr, u32 i,
                               const char *col1, const char *col2, 
                               vtss_chip_counter_t *c1, vtss_chip_counter_t *c2)
{
    char buf[80];
    
    sprintf(buf, "%s_%u", col1, i);
    l26_debug_cnt(pr, buf, col2, c1, c2);
}

static vtss_rc l26_debug_port_cnt(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455) */ // Due to VTSS_EXIT_ENTER
    vtss_port_no_t               port_no;
    u32                          i, port;
    vtss_port_luton26_counters_t *cnt;
    BOOL                         cpu_port;
    
    for (port_no = VTSS_PORT_NO_START; port_no <= vtss_state->port_count; port_no++) {
        cpu_port = (port_no == vtss_state->port_count);
        if (cpu_port) {
            /* CPU port */
            if (!info->full)
                continue;
            port = VTSS_CHIP_PORT_CPU;
            cnt = &vtss_state->cpu_counters.counter.luton26;
        } else {
            /* Normal port */
            if (info->port_list[port_no] == 0)
                continue;
            port = VTSS_CHIP_PORT(port_no);
            cnt = &vtss_state->port_counters[port_no].counter.luton26;
        }
        VTSS_RC(l26_port_counters_read(port, cnt, NULL, 0));
        VTSS_EXIT_ENTER();

        /* Basic counters */
        if (cpu_port) {
            pr("Counters CPU port: %u\n\n", port);
        } else {
            pr("Counters for port: %u (port_no %u):\n\n", port, port_no);
            l26_debug_cnt(pr, "oct", "", &cnt->rx_octets, &cnt->tx_octets);
            l26_debug_cnt(pr, "uc", "", &cnt->rx_unicast, &cnt->tx_unicast);
            l26_debug_cnt(pr, "mc", "", &cnt->rx_multicast, &cnt->tx_multicast);
            l26_debug_cnt(pr, "bc", "", &cnt->rx_broadcast, &cnt->tx_broadcast);
        }

        /* Detailed counters */
        if (info->full) {
            if (!cpu_port) {
                l26_debug_cnt(pr, "pause", "", &cnt->rx_pause, &cnt->tx_pause);
                l26_debug_cnt(pr, "64", "", &cnt->rx_64, &cnt->tx_64);
                l26_debug_cnt(pr, "65_127", "", &cnt->rx_65_127, &cnt->tx_65_127);
                l26_debug_cnt(pr, "128_255", "", &cnt->rx_128_255, &cnt->tx_128_255);
                l26_debug_cnt(pr, "256_511", "", &cnt->rx_256_511, &cnt->tx_256_511);
                l26_debug_cnt(pr, "512_1023", "", &cnt->rx_512_1023, &cnt->tx_512_1023);
                l26_debug_cnt(pr, "1024_1526", "", &cnt->rx_1024_1526, &cnt->tx_1024_1526);
                l26_debug_cnt(pr, "jumbo", "", &cnt->rx_1527_max, &cnt->tx_1527_max);
            }
            l26_debug_cnt(pr, "cat_drop", cpu_port ? NULL : "drops", 
                          &cnt->rx_classified_drops, &cnt->tx_drops);
            l26_debug_cnt(pr, "dr_local", cpu_port ? NULL : "aged", 
                          &cnt->dr_local, &cnt->tx_aging);
            l26_debug_cnt(pr, "dr_tail", NULL, &cnt->dr_tail, NULL);
            if (!cpu_port) {
                l26_debug_cnt(pr, "crc", NULL, &cnt->rx_crc_align_errors, NULL);
                l26_debug_cnt(pr, "short", NULL, &cnt->rx_shorts, NULL);
                l26_debug_cnt(pr, "long", NULL, &cnt->rx_longs, NULL);
                l26_debug_cnt(pr, "frag", NULL, &cnt->rx_fragments, NULL);
                l26_debug_cnt(pr, "jabber", NULL, &cnt->rx_jabbers, NULL);
                l26_debug_cnt(pr, "control", NULL, &cnt->rx_control, NULL);
            }
            for (i = 0; i < VTSS_PRIOS; i++)
                l26_debug_cnt_inst(pr, i, "green", "", 
                                   &cnt->rx_green_class[i], &cnt->tx_green_class[i]);
            for (i = 0; i < VTSS_PRIOS; i++)
                l26_debug_cnt_inst(pr, i, "yellow", "", 
                                   &cnt->rx_yellow_class[i], &cnt->tx_yellow_class[i]);
            for (i = 0; i < VTSS_PRIOS; i++) 
                l26_debug_cnt_inst(pr, i, "red", NULL, &cnt->rx_red_class[i], NULL);
            for (i = 0; i < VTSS_PRIOS; i++)
                l26_debug_cnt_inst(pr, i, "dr_green", NULL, &cnt->dr_green_class[i], NULL);
            for (i = 0; i < VTSS_PRIOS; i++)
                l26_debug_cnt_inst(pr, i, "dr_yellow", NULL, &cnt->dr_yellow_class[i], NULL);
        }
        pr("\n");
    }

    return VTSS_RC_OK;
}

static vtss_rc l26_debug_vlan(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455) */ // Due to VTSS_EXIT_ENTER
    vtss_vid_t        vid;
    vtss_vlan_entry_t *vlan_entry;
    BOOL              header = 1;
    u32               port, mask = 0;
    char              buf[32];

    for (port = 0; port <= VTSS_CHIP_PORT_CPU_1; port++) {
        sprintf(buf, "Port %u", port);
        l26_debug_reg_header(pr, buf);
        if (port != VTSS_CHIP_PORT_CPU_1) {
            l26_debug_reg_inst(pr, VTSS_ANA_PORT_VLAN_CFG(port), port, "ANA:VLAN_CFG");
            l26_debug_reg_inst(pr, VTSS_ANA_PORT_DROP_CFG(port), port, "ANA:DROP_CFG");
        }
        l26_debug_reg_inst(pr, VTSS_REW_PORT_PORT_VLAN_CFG(port), port, "REW:VLAN_CFG");
        l26_debug_reg_inst(pr, VTSS_REW_PORT_TAG_CFG(port), port, "REW:TAG_CFG");
        pr("\n");
    }
    l26_debug_reg(pr, VTSS_SYS_SYSTEM_VLAN_ETYPE_CFG, "ETYPE_CFG");
    l26_debug_reg(pr, VTSS_ANA_ANA_ADVLEARN, "ADVLEARN");
    l26_debug_reg(pr, VTSS_ANA_ANA_VLANMASK, "VLANMASK");
    pr("\n");
    
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        vlan_entry = &vtss_state->vlan_table[vid];
        if (!vlan_entry->enabled && !info->full)
            continue;

        L26_WR(VTSS_ANA_ANA_TABLES_VLANTIDX, VTSS_F_ANA_ANA_TABLES_VLANTIDX_V_INDEX(vid));
        L26_WR(VTSS_ANA_ANA_TABLES_VLANACCESS, 
               VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(VLAN_CMD_READ));
        if (l26_vlan_table_idle() != VTSS_RC_OK)
            continue;
        L26_RD(VTSS_ANA_ANA_TABLES_VLANACCESS, &mask);
        mask = VTSS_X_ANA_ANA_TABLES_VLANACCESS_VLAN_PORT_MASK(mask);

        if (header)
            l26_debug_print_port_header(pr, "VID   ");
        header = 0;

        pr("%-4u  ", vid);
        l26_debug_print_mask(pr, mask);

        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc l26_debug_src_table(const vtss_debug_printf_t pr,
                                   const vtss_debug_info_t   *const info)
{
    u32 port, mask;

    l26_debug_print_header(pr, "Source Masks");
    l26_debug_print_port_header(pr, "Port  ");
    for (port = 0; port <= VTSS_CHIP_PORTS;  port++) {
        L26_RD(VTSS_ANA_ANA_TABLES_PGID(PGID_SRC + port), &mask);
        mask = VTSS_X_ANA_ANA_TABLES_PGID_PGID(mask);
        pr("%-4u  ", port);
        l26_debug_print_mask(pr, mask);
    }
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc l26_debug_pvlan(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    l26_debug_reg_header(pr, "ANA");
    l26_debug_reg(pr, VTSS_ANA_ANA_ISOLATED_PORTS, "ISOLATED_PORTS");
    pr("\n");

    return l26_debug_src_table(pr, info);
}

static vtss_rc l26_debug_mac_table(const vtss_debug_printf_t pr,
                                   const vtss_debug_info_t   *const info)
{
    u32 value;
    
    /* Read and clear analyzer sticky bits */
    L26_RD(VTSS_ANA_ANA_ANEVENTS, &value);
    L26_WR(VTSS_ANA_ANA_ANEVENTS, value);
    
    vtss_debug_print_sticky(pr, "AUTOAGE", value, VTSS_F_ANA_ANA_ANEVENTS_AUTOAGE);
    vtss_debug_print_sticky(pr, "STORM_DROP", value, VTSS_F_ANA_ANA_ANEVENTS_STORM_DROP);
    vtss_debug_print_sticky(pr, "LEARN_DROP", value, VTSS_F_ANA_ANA_ANEVENTS_LEARN_DROP);
    vtss_debug_print_sticky(pr, "AGED_ENTRY", value, VTSS_F_ANA_ANA_ANEVENTS_AGED_ENTRY);
    vtss_debug_print_sticky(pr, "CPU_LEARN_FAILED", value, VTSS_F_ANA_ANA_ANEVENTS_CPU_LEARN_FAILED);
    vtss_debug_print_sticky(pr, "AUTO_LEARN_FAILED", value, VTSS_F_ANA_ANA_ANEVENTS_AUTO_LEARN_FAILED);
    vtss_debug_print_sticky(pr, "LEARN_REMOVE", value, VTSS_F_ANA_ANA_ANEVENTS_LEARN_REMOVE);
    vtss_debug_print_sticky(pr, "AUTO_LEARNED", value, VTSS_F_ANA_ANA_ANEVENTS_AUTO_LEARNED);
    vtss_debug_print_sticky(pr, "AUTO_MOVED", value, VTSS_F_ANA_ANA_ANEVENTS_AUTO_MOVED);
    vtss_debug_print_sticky(pr, "CLASSIFIED_DROP", value, VTSS_F_ANA_ANA_ANEVENTS_CLASSIFIED_DROP);
    vtss_debug_print_sticky(pr, "CLASSIFIED_COPY", value, VTSS_F_ANA_ANA_ANEVENTS_CLASSIFIED_COPY);
    vtss_debug_print_sticky(pr, "VLAN_DISCARD", value, VTSS_F_ANA_ANA_ANEVENTS_VLAN_DISCARD);
    vtss_debug_print_sticky(pr, "FWD_DISCARD", value, VTSS_F_ANA_ANA_ANEVENTS_FWD_DISCARD);
    vtss_debug_print_sticky(pr, "MULTICAST_FLOOD", value, VTSS_F_ANA_ANA_ANEVENTS_MULTICAST_FLOOD);
    vtss_debug_print_sticky(pr, "UNICAST_FLOOD", value, VTSS_F_ANA_ANA_ANEVENTS_UNICAST_FLOOD);
    vtss_debug_print_sticky(pr, "DEST_KNOWN", value, VTSS_F_ANA_ANA_ANEVENTS_DEST_KNOWN);
    vtss_debug_print_sticky(pr, "BUCKET3_MATCH", value, VTSS_F_ANA_ANA_ANEVENTS_BUCKET3_MATCH);
    vtss_debug_print_sticky(pr, "BUCKET2_MATCH", value, VTSS_F_ANA_ANA_ANEVENTS_BUCKET2_MATCH);
    vtss_debug_print_sticky(pr, "BUCKET1_MATCH", value, VTSS_F_ANA_ANA_ANEVENTS_BUCKET1_MATCH);
    vtss_debug_print_sticky(pr, "BUCKET0_MATCH", value, VTSS_F_ANA_ANA_ANEVENTS_BUCKET0_MATCH);
    vtss_debug_print_sticky(pr, "CPU_OPERATION", value, VTSS_F_ANA_ANA_ANEVENTS_CPU_OPERATION);
    vtss_debug_print_sticky(pr, "DMAC_LOOKUP", value, VTSS_F_ANA_ANA_ANEVENTS_DMAC_LOOKUP);
    vtss_debug_print_sticky(pr, "SMAC_LOOKUP", value, VTSS_F_ANA_ANA_ANEVENTS_SMAC_LOOKUP);
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc l26_debug_qos(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    u32            i, value, port;
    vtss_port_no_t port_no;

    l26_debug_print_header(pr, "QoS Storm Control");

    L26_RD(VTSS_ANA_ANA_STORMLIMIT_BURST, &value);
    pr("Burst: %u\n", VTSS_X_ANA_ANA_STORMLIMIT_BURST_STORM_BURST(value));
    for (i = 0; i < 3; i++) {
        const char *name = (i == 0 ? "UC" : i == 1 ? "BC" : "MC");
        L26_RD(VTSS_ANA_ANA_STORMLIMIT_CFG(i), &value);
        pr("%s   : Rate %2u, Unit %u, Mode %u\n", name,
           VTSS_X_ANA_ANA_STORMLIMIT_CFG_STORM_RATE(value),
           VTSS_BOOL(value & VTSS_F_ANA_ANA_STORMLIMIT_CFG_STORM_UNIT),
           VTSS_X_ANA_ANA_STORMLIMIT_CFG_STORM_MODE(value));
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS Port Classification Config");

    pr("Port PCP CLS DEI DPL TC_EN DC_EN\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 vlan, qos;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        L26_RD(VTSS_ANA_PORT_VLAN_CFG(port), &vlan);
        L26_RD(VTSS_ANA_PORT_QOS_CFG(port), &qos);
        pr("%4u %3u %3u %3u %3u %5u %5u\n",
           port_no,
           VTSS_X_ANA_PORT_VLAN_CFG_VLAN_PCP(vlan),
           VTSS_X_ANA_PORT_QOS_CFG_QOS_DEFAULT_VAL(qos),
           VTSS_BOOL(vlan & VTSS_F_ANA_PORT_VLAN_CFG_VLAN_DEI),
           VTSS_BOOL(qos & VTSS_F_ANA_PORT_QOS_CFG_DP_DEFAULT_VAL),
           VTSS_BOOL(qos & VTSS_F_ANA_PORT_QOS_CFG_QOS_PCP_ENA),
           VTSS_BOOL(qos & VTSS_F_ANA_PORT_QOS_CFG_QOS_DSCP_ENA));
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS Port Classification PCP, DEI to QoS class, DP level Mapping");

    pr("Port QoS class (8*DEI+PCP)           DP level (8*DEI+PCP)\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int pcp, dei, class_ct = 0, dpl_ct = 0;
        char class_buf[40], dpl_buf[40];
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        for (dei = VTSS_DEI_START; dei < VTSS_DEI_END; dei++) {
            for (pcp = VTSS_PCP_START; pcp < VTSS_PCP_END; pcp++) {
                const char *delim = ((pcp == VTSS_PCP_START) && (dei == VTSS_DEI_START)) ? "" : ",";
                L26_RD(VTSS_ANA_PORT_QOS_PCP_DEI_MAP_CFG(port, (8*dei + pcp)), &value);
                class_ct += snprintf(class_buf + class_ct, sizeof(class_buf) - class_ct, "%s%u", delim,
                                     VTSS_X_ANA_PORT_QOS_PCP_DEI_MAP_CFG_QOS_PCP_DEI_VAL(value));
                dpl_ct   += snprintf(dpl_buf   + dpl_ct,   sizeof(dpl_buf)   - dpl_ct,   "%s%u",  delim,
                                     VTSS_BOOL(value & VTSS_F_ANA_PORT_QOS_PCP_DEI_MAP_CFG_DP_PCP_DEI_VAL));
            }
        }
        pr("%4u %s %s\n", port_no, class_buf, dpl_buf);
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS Port Leaky Bucket and Scheduler Config");

    L26_RD(VTSS_SYS_SCH_LB_DWRR_FRM_ADJ, &value);
    pr("Frame Adjustment (gap value): %u bytes\n", VTSS_X_SYS_SCH_LB_DWRR_FRM_ADJ_FRM_ADJ(value));
    pr("Port F_EN Mode C0 C1 C2 C3 C4 C5\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 frm_adj_ena, dwrr_cfg, cost;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        L26_RD(VTSS_SYS_SCH_LB_DWRR_CFG(port), &frm_adj_ena);
        L26_RD(VTSS_SYS_SCH_SCH_DWRR_CFG(port), &dwrr_cfg);
        cost = VTSS_X_SYS_SCH_SCH_DWRR_CFG_COST_CFG(dwrr_cfg);
        pr("%4u %4u %4u %2u %2u %2u %2u %2u %2u\n",
           port_no,
           VTSS_BOOL(frm_adj_ena & VTSS_F_SYS_SCH_LB_DWRR_CFG_FRM_ADJ_ENA),
           VTSS_BOOL(dwrr_cfg & VTSS_F_SYS_SCH_SCH_DWRR_CFG_DWRR_MODE),
           VTSS_EXTRACT_BITFIELD(cost,  0, 5),
           VTSS_EXTRACT_BITFIELD(cost,  5, 5),
           VTSS_EXTRACT_BITFIELD(cost, 10, 5),
           VTSS_EXTRACT_BITFIELD(cost, 15, 5),
           VTSS_EXTRACT_BITFIELD(cost, 20, 5),
           VTSS_EXTRACT_BITFIELD(cost, 25, 5));
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS Port and Queue Shaper enable/disable Config");

    pr("Port P Q0-Q7    Ex Q0-Q7\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 queue_shaper, queue_excess;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        L26_RD(VTSS_SYS_SCH_SCH_SHAPING_CTRL(port), &value);
        queue_shaper = VTSS_X_SYS_SCH_SCH_SHAPING_CTRL_PRIO_SHAPING_ENA(value);
        queue_excess = VTSS_X_SYS_SCH_SCH_SHAPING_CTRL_PRIO_LB_EXS_ENA(value);
        pr("%4u %u %u%u%u%u%u%u%u%u %u%u%u%u%u%u%u%u\n",
           port_no,
           VTSS_BOOL(value & VTSS_F_SYS_SCH_SCH_SHAPING_CTRL_PORT_SHAPING_ENA),
           VTSS_BOOL(queue_shaper & VTSS_BIT(0)),
           VTSS_BOOL(queue_shaper & VTSS_BIT(1)),
           VTSS_BOOL(queue_shaper & VTSS_BIT(2)),
           VTSS_BOOL(queue_shaper & VTSS_BIT(3)),
           VTSS_BOOL(queue_shaper & VTSS_BIT(4)),
           VTSS_BOOL(queue_shaper & VTSS_BIT(5)),
           VTSS_BOOL(queue_shaper & VTSS_BIT(6)),
           VTSS_BOOL(queue_shaper & VTSS_BIT(7)),
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

    l26_debug_print_header(pr, "QoS Port and Queue Shaper Burst and Rate Config");

    pr("Port Queue Burst Rate\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int queue;
        u32 burst, rate;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        L26_RD(VTSS_SYS_SCH_LB_LB_THRES(((9 * port) + 8)), &burst);
        L26_RD(VTSS_SYS_SCH_LB_LB_RATE(((9 * port) + 8)), &rate);
        pr("%4u     - 0x%02x  0x%04x\n     ",
           port_no,
           VTSS_X_SYS_SCH_LB_LB_THRES_LB_THRES(burst),
           VTSS_X_SYS_SCH_LB_LB_RATE_LB_RATE(rate));
        for (queue = 0; queue < 8; queue++) {
            L26_RD(VTSS_SYS_SCH_LB_LB_THRES(((9 * port) + queue)), &burst);
            L26_RD(VTSS_SYS_SCH_LB_LB_RATE(((9 * port) + queue)), &rate);
            pr("%5d 0x%02x  0x%04x\n     ",
               queue,
               VTSS_X_SYS_SCH_LB_LB_THRES_LB_THRES(burst),
               VTSS_X_SYS_SCH_LB_LB_RATE_LB_RATE(rate));
        }
        pr("\r");
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS Port Tag Remarking Config");

    pr("Port Mode PCP DEI\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 tag_default, tag_ctrl;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        L26_RD(VTSS_REW_PORT_PORT_VLAN_CFG(port), &tag_default);
        L26_RD(VTSS_REW_PORT_TAG_CFG(port), &tag_ctrl);
        pr("%4u %4x %3d %3d\n",
           port_no,
           VTSS_X_REW_PORT_TAG_CFG_TAG_QOS_CFG(tag_ctrl),
           VTSS_X_REW_PORT_PORT_VLAN_CFG_PORT_PCP(tag_default),
           VTSS_BOOL(tag_default & VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_DEI));
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS Port Tag Remarking Map");

    pr("Port PCP (2*QoS class+DPL)           DEI (2*QoS class+DPL)\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int class, dpl, pcp_ct = 0, dei_ct = 0;
        char pcp_buf[40], dei_buf[40];
        if (info->port_list[port_no] == 0)
            continue;
        port = VTSS_CHIP_PORT(port_no);
        for (class = VTSS_QUEUE_START; class < VTSS_QUEUE_END; class++) {
            for (dpl = 0; dpl < 2; dpl++) {
                const char *delim = ((class == VTSS_QUEUE_START) && (dpl == 0)) ? "" : ",";
                L26_RD(VTSS_REW_PORT_PCP_DEI_QOS_MAP_CFG(port, (8*dpl + class)), &value);
                pcp_ct += snprintf(pcp_buf + pcp_ct, sizeof(pcp_buf) - pcp_ct, "%s%u", delim,
                                   VTSS_X_REW_PORT_PCP_DEI_QOS_MAP_CFG_PCP_QOS_VAL(value));
                dei_ct += snprintf(dei_buf + dei_ct, sizeof(dei_buf) - dei_ct, "%s%u",  delim,
                                   VTSS_BOOL(value & VTSS_F_REW_PORT_PCP_DEI_QOS_MAP_CFG_DEI_QOS_VAL));
            }
        }
        pr("%4u %s %s\n", port_no, pcp_buf, dei_buf);
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS Port DSCP Remarking Config");

    pr("Port I_Mode Trans E_Mode\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 qos_cfg, dscp_cfg;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        L26_RD(VTSS_ANA_PORT_QOS_CFG(port), &qos_cfg);
        L26_RD(VTSS_REW_PORT_DSCP_CFG(port), &dscp_cfg);
        pr("%4u %6u %5u %6u\n",
           port_no,
           VTSS_X_ANA_PORT_QOS_CFG_DSCP_REWR_CFG(qos_cfg),
           VTSS_BOOL(qos_cfg & VTSS_F_ANA_PORT_QOS_CFG_DSCP_TRANSLATE_ENA),
           VTSS_X_REW_PORT_DSCP_CFG_DSCP_REWR_CFG(dscp_cfg));
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS DSCP Config");

    pr("DSCP Trans CLS DPL Rewr Trust Remap_DP0 Remap_DP1\n");
    for (i = 0; i < 64; i++) {
        u32 dscp_cfg, dscp_remap, dscp_remap_dp1;
        L26_RD(VTSS_ANA_COMMON_DSCP_CFG(i), &dscp_cfg);
        L26_RD(VTSS_REW_COMMON_DSCP_REMAP_CFG(i), &dscp_remap);
        L26_RD(VTSS_REW_COMMON_DSCP_REMAP_DP1_CFG(i), &dscp_remap_dp1);

        pr("%4u %5u %3u %3u %4u %5u %5u     %5u\n",
           i,
           VTSS_X_ANA_COMMON_DSCP_CFG_DSCP_TRANSLATE_VAL(dscp_cfg),
           VTSS_X_ANA_COMMON_DSCP_CFG_QOS_DSCP_VAL(dscp_cfg),
           VTSS_BOOL(dscp_cfg & VTSS_F_ANA_COMMON_DSCP_CFG_DP_DSCP_VAL),
           VTSS_BOOL(dscp_cfg & VTSS_F_ANA_COMMON_DSCP_CFG_DSCP_REWR_ENA),
           VTSS_BOOL(dscp_cfg & VTSS_F_ANA_COMMON_DSCP_CFG_DSCP_TRUST_ENA),
           VTSS_X_REW_COMMON_DSCP_REMAP_CFG_DSCP_REMAP_VAL(dscp_remap),
           VTSS_X_REW_COMMON_DSCP_REMAP_DP1_CFG_DSCP_REMAP_DP1_VAL(dscp_remap_dp1));
    }
    pr("\n");

    l26_debug_print_header(pr, "QoS DSCP Classification from QoS Config");

    pr("QoS DSCP_DP0 DSCP_DP1\n");
    for (i = 0; i < 8; i++) {
        u32 qos_dp0, qos_dp1;
        L26_RD(VTSS_ANA_COMMON_DSCP_REWR_CFG(i), &qos_dp0);
        L26_RD(VTSS_ANA_COMMON_DSCP_REWR_CFG(i + 8), &qos_dp1);
        pr("%3u %4u     %4u\n",
           i,
           VTSS_X_ANA_COMMON_DSCP_REWR_CFG_DSCP_QOS_REWR_VAL(qos_dp0),
           VTSS_X_ANA_COMMON_DSCP_REWR_CFG_DSCP_QOS_REWR_VAL(qos_dp1));
    }
    pr("\n");

    VTSS_RC(l26_debug_range_checkers(pr, info));
    VTSS_RC(l26_debug_vcap_is1(pr, info));

    l26_debug_print_header(pr, "QoS Port and Queue Policer");

    l26_debug_reg_header(pr, "Policer Config");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        l26_debug_reg_inst(pr, VTSS_ANA_PORT_POL_CFG(port), port, "POL_CFG");
        l26_debug_reg_inst(pr, VTSS_SYS_POL_MISC_POL_FLOWC(port), port, "POL_FLOWC");
    }
    pr("\n");

    pr("Policers:\n");
    for (i = 0; i < VTSS_L26_POLICER_CNT; i++) {
        u32                 mode;
        vtss_policer_user_t user;

        if (!info->full && i == vtss_state->port_count) {
            pr("Use 'full' to see all of the %d policers!", VTSS_L26_POLICER_CNT);
            break;
        }

        if ((i % 32) == 0) {
            pr("\nIndex  User   Mode  Dual  IFG  CF  OS  PIR    PBS  CIR    CBS\n");
        }

        L26_RD(VTSS_SYS_POL_POL_MODE_CFG(i), &value);
        mode = VTSS_X_SYS_POL_POL_MODE_CFG_FRM_MODE(value);
        user = vtss_state->policer_user[i];
        pr("%-5u  %-5s  %-4s  %-4u  %-3u  %-2u  %-2u  ",
           i,
           user == VTSS_POLICER_USER_NONE ? "None" :
           user == VTSS_POLICER_USER_DISCARD ? "Disc" :
           user == VTSS_POLICER_USER_PORT ? "Port" :
           user == VTSS_POLICER_USER_QUEUE ? "Queue" :
           user == VTSS_POLICER_USER_ACL ? "ACL" :
           user == VTSS_POLICER_USER_EVC ? "EVC" :
           user == VTSS_POLICER_USER_MEP ? "MEP" : "?",
           mode == POL_MODE_LINERATE ? "Line" : mode == POL_MODE_DATARATE ? "Data" :
           mode == POL_MODE_FRMRATE_100FPS ? "F100" : "F1",
           L26_BF(SYS_POL_POL_MODE_CFG_CIR_ENA, value),
           VTSS_X_SYS_POL_POL_MODE_CFG_IPG_SIZE(value),
           L26_BF(SYS_POL_POL_MODE_CFG_DLB_COUPLED, value),
           L26_BF(SYS_POL_POL_MODE_CFG_OVERSHOOT_ENA, value));

        L26_RD(VTSS_SYS_POL_POL_PIR_CFG(i), &value);
        pr("%-5u  %-3u  ",
           VTSS_X_SYS_POL_POL_PIR_CFG_PIR_RATE(value),
           VTSS_X_SYS_POL_POL_PIR_CFG_PIR_BURST(value));

        L26_RD(VTSS_SYS_POL_POL_CIR_CFG(i), &value);
        pr("%-5u  %-3u\n",
           VTSS_X_SYS_POL_POL_CIR_CFG_CIR_RATE(value),
           VTSS_X_SYS_POL_POL_CIR_CFG_CIR_BURST(value));
    }
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc l26_debug_aggr(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    u32 ac, pgid, mask, value;
    
    VTSS_RC(l26_debug_src_table(pr, info));

    l26_debug_print_header(pr, "Aggregation Masks");
    l26_debug_print_port_header(pr, "AC    ");
    for (ac = 0; ac < L26_ACS; ac++) {
        L26_RD(VTSS_ANA_ANA_TABLES_PGID(PGID_AGGR + ac), &mask);
        mask = VTSS_X_ANA_ANA_TABLES_PGID_PGID(mask);
        pr("%-4u  ", ac);
        l26_debug_print_mask(pr, mask);
    }
    pr("\n");
    
    l26_debug_print_header(pr, "Destination Masks");
    l26_debug_print_port_header(pr, "PGID  CPU  Queue  ");
    for (pgid = 0; pgid < VTSS_PGID_LUTON26; pgid++) {
        L26_RD(VTSS_ANA_ANA_TABLES_PGID(pgid), &value);
        mask = VTSS_X_ANA_ANA_TABLES_PGID_PGID(value);
        pr("%-4u  %-3u  %-5u  ", pgid, mask & VTSS_BIT(VTSS_CHIP_PORT_CPU) ? 1 : 0,
           VTSS_X_ANA_ANA_TABLES_PGID_CPUQ_DST_PGID(value));
        l26_debug_print_mask(pr, mask);
    }
    pr("\n");
    

    l26_debug_print_header(pr, "Flooding PGIDs");
    L26_RD(VTSS_ANA_ANA_FLOODING, &value);
    pr("UNICAST  : %u\n", VTSS_X_ANA_ANA_FLOODING_FLD_UNICAST(value));
    pr("MULTICAST: %u\n", VTSS_X_ANA_ANA_FLOODING_FLD_MULTICAST(value));
    L26_RD(VTSS_ANA_ANA_FLOODING_IPMC, &value);
    pr("MC4_CTRL : %u\n", VTSS_X_ANA_ANA_FLOODING_IPMC_FLD_MC4_CTRL(value));
    pr("MC4_DATA : %u\n", VTSS_X_ANA_ANA_FLOODING_IPMC_FLD_MC4_DATA(value));
    pr("MC6_CTRL : %u\n", VTSS_X_ANA_ANA_FLOODING_IPMC_FLD_MC6_CTRL(value));
    pr("MC6_DATA : %u\n", VTSS_X_ANA_ANA_FLOODING_IPMC_FLD_MC6_DATA(value));
    pr("\n");

    l26_debug_reg_header(pr, "Aggr. Mode");
    l26_debug_reg(pr, VTSS_ANA_COMMON_AGGR_CFG, "AGGR_CFG");
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc l26_debug_stp(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    u32 value, port;

    pr("Port  ID  Learn  L_Auto  L_CPU  L_DROP  Mirror\n");
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        L26_RD(VTSS_ANA_PORT_PORT_CFG(port), &value);
        pr("%-4u  %-2u  %-5u  %-6u  %-5u  %-6u  %u\n",
           port, 
           VTSS_X_ANA_PORT_PORT_CFG_PORTID_VAL(value),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_LEARN_ENA),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_LEARNAUTO),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_LEARNCPU),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_LEARNDROP),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA));
    }
    pr("\n");
    
    for (port = 0; port <= VTSS_CHIP_PORTS; port++)
        l26_debug_reg_inst(pr, VTSS_ANA_PORT_PORT_CFG(port), port, "PORT_CFG");
    pr("\n");
    return VTSS_RC_OK;
}

static vtss_rc l26_debug_mirror(const vtss_debug_printf_t pr,
                                const vtss_debug_info_t   *const info)
{
    u32  port, value, mask = 0;
    
    /* Calculate ingress mirror mask */
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        L26_RD(VTSS_ANA_PORT_PORT_CFG(port), &value);
        if (value & VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA)
            mask |= VTSS_BIT(port);
    }
    l26_debug_print_port_header(pr, "Mirror   ");
    pr("Ingress  ");
    l26_debug_print_mask(pr, mask);
    L26_RD(VTSS_ANA_ANA_EMIRRORPORTS, &mask);
    pr("Egress   ");
    l26_debug_print_mask(pr, mask);
    L26_RD(VTSS_ANA_ANA_MIRRORPORTS, &mask);
    pr("Ports    ");
    l26_debug_print_mask(pr, mask);
    pr("\n");

    return VTSS_RC_OK;
}

#if defined(VTSS_ARCH_CARACAL)
static vtss_rc l26_debug_evc(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    VTSS_RC(l26_debug_range_checkers(pr, info));
    VTSS_RC(l26_debug_vcap_port(pr, info));
    VTSS_RC(l26_debug_vcap_is1(pr, info));
    VTSS_RC(l26_debug_vcap_es0(pr, info));
    
    return VTSS_RC_OK;
}
#endif /* VTSS_ARCH_CARACAL */

#define L26_DEBUG_CPU_FWD(pr, addr, i, name) l26_debug_reg_inst(pr, VTSS_ANA_PORT_CPU_FWD_##addr, i, "FWD_"name)
#define L26_DEBUG_XTR(pr, addr, name) l26_debug_reg(pr, VTSS_DEVCPU_QS_XTR_XTR_##addr, "XTR_"name)
#define L26_DEBUG_XTR_INST(pr, addr, i, name) l26_debug_reg_inst(pr, VTSS_DEVCPU_QS_XTR_XTR_##addr, i, "XTR_"name)
#define L26_DEBUG_INJ(pr, addr, name) l26_debug_reg(pr, VTSS_DEVCPU_QS_INJ_INJ_##addr, "INJ_"name)
#define L26_DEBUG_INJ_INST(pr, addr, i, name) l26_debug_reg_inst(pr, VTSS_DEVCPU_QS_INJ_INJ_##addr, i, "INJ_"name)

static vtss_rc l26_debug_pkt(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    u32  i, port;
    char buf[32];

    /* Analyzer CPU forwarding registers */
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        sprintf(buf, "Port %u", port);
        l26_debug_reg_header(pr, buf);
        L26_DEBUG_CPU_FWD(pr, CFG(port), port, "CFG");
        L26_DEBUG_CPU_FWD(pr, BPDU_CFG(port), port, "BPDU_CFG");
        L26_DEBUG_CPU_FWD(pr, GARP_CFG(port), port, "GARP_CFG");
        L26_DEBUG_CPU_FWD(pr, CCM_CFG(port), port, "CCM_CFG");
        pr("\n");
    }
    
    /* Analyzer CPU queue mappings */
    l26_debug_reg_header(pr, "CPU Queues");
    l26_debug_reg(pr, VTSS_ANA_COMMON_CPUQ_CFG, "CPUQ_CFG");
    for (i = 0; i < 16; i++)
        l26_debug_reg_inst(pr, VTSS_ANA_COMMON_CPUQ_8021_CFG(i), i, "CPUQ_8021_CFG");
    pr("\n");

    /* Packet extraction registers */
    l26_debug_reg_header(pr, "Extraction");
    for (i = 0; i < VTSS_PACKET_RX_GRP_CNT; i++)
        L26_DEBUG_XTR_INST(pr, FRM_PRUNING(i), i, "FRM_PRUNING");
    for (i = 0; i < VTSS_PACKET_RX_GRP_CNT; i++)
        L26_DEBUG_XTR_INST(pr, GRP_CFG(i), i, "GRP_CFG");
    for (i = 0; i < VTSS_PACKET_RX_GRP_CNT; i++)
        L26_DEBUG_XTR_INST(pr, MAP(i), i, "MAP");
    L26_DEBUG_XTR(pr, DATA_PRESENT, "DATA_PRESENT");
    L26_DEBUG_XTR(pr, QU_DBG, "QU_DBG");
    l26_debug_reg(pr, VTSS_SYS_SCH_SCH_CPU, "SCH_CPU");
    pr("\n");
    
    /* Packet injection registers */
    l26_debug_reg_header(pr, "Injection");
    for (i = 0; i < VTSS_PACKET_TX_GRP_CNT; i++)
        L26_DEBUG_INJ_INST(pr, GRP_CFG(i), i, "GRP_CFG");
    for (i = 0; i < VTSS_PACKET_TX_GRP_CNT; i++)
        L26_DEBUG_INJ_INST(pr, CTRL(i), i, "CTRL");
    for (i = 0; i < VTSS_PACKET_TX_GRP_CNT; i++)
        L26_DEBUG_INJ_INST(pr, ERR(i), i, "ERR");
    L26_DEBUG_INJ(pr, STATUS, "STATUS");
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc l26_debug_ts(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    u32  value;
    u32  port;
    char buf[32];

    /* DEVCPU_GCB: PTP_TIMER */
    l26_debug_reg_header(pr, "GCB:PTP_TIMER");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_SECS, "PTP_TOD_SECS");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TOD_NANOSECS, "PTP_TOD_NANOSECS");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_DELAY, "PTP_DELAY");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL, "PTP_TIMER_CTRL");
    /* DEVCPU_GCB: PTP_CFG */
    l26_debug_reg_header(pr, "GCB:PTP_CFG");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, "PTP_MISC_CFG");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_CFG, "PTP_UPPER_LIMIT_CFG");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG, "PTP_UPPER_LIMIT_1_TIME_ADJ_CFG");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG, "PTP_SYNC_INTR_ENA_CFG");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG, "GEN_EXT_CLK_HIGH_PERIOD_CFG");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG, "GEN_EXT_CLK_LOW_PERIOD_CFG");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG, "GEN_EXT_CLK_CFG");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG, "CLK_ADJ_CFG");
    /* DEVCPU_GCB: PTP_STAT */
    l26_debug_reg_header(pr, "GCB:PTP_STAT");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_STAT_PTP_CURRENT_TIME_STAT, "PTP_CURRENT_TIME_STAT");
    l26_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT, "EXT_SYNC_CURRENT_TIME_STAT");

    L26_RD(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, &value);
    L26_WR(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, value);
    vtss_debug_print_sticky(pr, "CLK_ADJ_UPD_STICKY", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY);
    vtss_debug_print_sticky(pr, "EXT_SYNC_CURRENT_TIME_STICKY", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_EXT_SYNC_CURRENT_TIME_STICKY);
    vtss_debug_print_sticky(pr, "SYNC_STAT", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_SYNC_STAT);
    
    /* SYS_PORT:PTP */
    l26_debug_reg_header(pr, "SYS:PORT:PTP");
    l26_debug_reg(pr, VTSS_SYS_PTP_PTP_STATUS, "PTP_STATUS");
    l26_debug_reg(pr, VTSS_SYS_PTP_PTP_DELAY, "PTP_DELAY");
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        sprintf(buf, "PTP_CFG(%u)", port);
        l26_debug_reg(pr, VTSS_SYS_PTP_PTP_CFG(port), buf);
    }

    /* ANA:: */
    l26_debug_reg_header(pr, "ANA::PTP");
    l26_debug_reg(pr, VTSS_ANA_ANA_TABLES_PTP_ID_HIGH, "PTP_ID_HIGH");
    l26_debug_reg(pr, VTSS_ANA_ANA_TABLES_PTP_ID_LOW, "PTP_ID_LOW");
    
    
    pr("\n");

    return VTSS_RC_OK;
}

static u32 WmDec(u32 value) {
    if (value > 1024) { 
        return (value - 1024) * 16;
    }        
    return value;
}

static void l26_debug_wm_dump (const vtss_debug_printf_t pr, const char *reg_name, u32 *value, u32 i, BOOL decode) {
    u32 q;
    pr("%-25s",reg_name);
    for (q = 0; q <i; q++) {
            pr("%6u ",decode?WmDec(value[q])*L26_BUFFER_CELL_SZ:value[q]);
    }
    pr("\n");
}

static vtss_rc l26_debug_wm(const vtss_debug_printf_t pr,
                            const vtss_debug_info_t   *const info)

{
    u32 port_no, value, q, dp, cpu_port, port;
    u32 id[8], val1[8], val2[8], val3[8], val4[8];
    

    for (port_no = VTSS_PORT_NO_START; port_no <= vtss_state->port_count; port_no++) {
        cpu_port = (port_no == vtss_state->port_count);
        if (cpu_port) {
            /* CPU port */
            if (!info->full)
                continue;
            port = VTSS_CHIP_PORT_CPU;
        } else {
            if (!info->port_list[port_no]) {
                continue;
            }
            port = VTSS_CHIP_PORT(port_no);
        }
        if (cpu_port) 
            pr("CPU_Port          : %u\n\n",port_no);
        else
            pr("Port          : %u\n\n",port_no);
        if (!cpu_port) {
            L26_RD(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_CFG(VTSS_TO_DEV(VTSS_CHIP_PORT(port_no))), &value);
            pr("FC Tx ena     : %d\n", (value & VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_TX_FC_ENA) ? 1 : 0);
            pr("FC Rx ena     : %d\n", (value & VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_RX_FC_ENA) ? 1 : 0);
            pr("FC Value      : 0x%x\n", VTSS_X_DEV_MAC_CFG_STATUS_MAC_FC_CFG_PAUSE_VAL_CFG(value));
            pr("FC Latency    : %u\n", VTSS_X_DEV_MAC_CFG_STATUS_MAC_FC_CFG_FC_LATENCY_CFG(value));
            pr("FC Zero pause : %d\n", (value & VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_ZERO_PAUSE_ENA) ? 1 : 0);
            
            L26_RD(VTSS_SYS_PAUSE_CFG_PAUSE_CFG(VTSS_CHIP_PORT(port_no)), &value);
            pr("FC Start      : 0x%x\n",  VTSS_X_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_START(value));
            pr("FC Stop       : 0x%x\n", VTSS_X_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_STOP(value));
            pr("FC Ena        : %d\n", (value & VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_ENA) ? 1 : 0);
            L26_RD(VTSS_SYS_PAUSE_CFG_ATOP(VTSS_CHIP_PORT(port_no)), &value);
            pr("FC Atop       : 0x%x\n", VTSS_F_SYS_PAUSE_CFG_ATOP_ATOP(value));      
            
            L26_RD(VTSS_SYS_PAUSE_CFG_PAUSE_TOT_CFG, &value);
            pr("FC TOT_START  : 0x%x\n", VTSS_X_SYS_PAUSE_CFG_PAUSE_TOT_CFG_PAUSE_TOT_START(value));
            pr("FC TOT_STOP   : 0x%x\n", VTSS_X_SYS_PAUSE_CFG_PAUSE_TOT_CFG_PAUSE_TOT_STOP(value));
            L26_RD(VTSS_SYS_PAUSE_CFG_ATOP_TOT_CFG, &value);
            pr("FC ATOP_TOT   : 0x%x\n", VTSS_F_SYS_PAUSE_CFG_ATOP_TOT_CFG_ATOP_TOT(value));
            pr("\n");
        }
        
        for (q = 0; q < VTSS_PRIOS; q++) {
            id[q] = q;
            L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((port * VTSS_PRIOS + q + 0)),   &val1[q]);
            L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((port * VTSS_PRIOS + q + 256)), &val2[q]);
            L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((port * VTSS_PRIOS + q + 512)), &val3[q]);
            L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((port * VTSS_PRIOS + q + 768)), &val4[q]);
        }
        
        l26_debug_wm_dump(pr, "Queue level:", id, 8, 0);
        l26_debug_wm_dump(pr, "Qu Ingr Buf Rsrv (Bytes) :", val1, 8, 1);
        l26_debug_wm_dump(pr, "Qu Ingr Ref Rsrv (Frames):", val2, 8, 0);
        l26_debug_wm_dump(pr, "Qu Egr Buf Rsrv  (Bytes) :", val3, 8, 1);
        l26_debug_wm_dump(pr, "Qu Egr Ref Rsrv  (Frames):", val4, 8, 0);
        pr("\n");
            
        /* Shared space for all QoS classes */
        for (q = 0; q < VTSS_PRIOS; q++) {
            L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 0)),   &val1[q]);
            L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 256)), &val2[q]);
            L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 512)), &val3[q]);
            L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 768)), &val4[q]);
        }
        l26_debug_wm_dump(pr, "QoS level:", id, 8, 0);
        l26_debug_wm_dump(pr, "QoS Ingr Buf Rsrv (Bytes) :", val1, 8, 1);
        l26_debug_wm_dump(pr, "QoS Ingr Ref Rsrv (Frames):", val2, 8, 0);
        l26_debug_wm_dump(pr, "QoS Egr Buf Rsrv  (Bytes) :", val3, 8, 1);
        l26_debug_wm_dump(pr, "QoS Egr Ref Rsrv  (Frames):", val4, 8, 0);
        pr("\n");
        /* Configure reserved space for port */
        L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((port + 224 +   0)), &val1[0]);
        L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((port + 224 + 256)), &val2[0]);
        L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((port + 224 + 512)), &val3[0]);
        L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((port + 224 + 768)), &val4[0]);
        pr("Port level:\n");
        pr("Port Ingress Buf Rsrv: %u Bytes\n", WmDec(val1[0])*L26_BUFFER_CELL_SZ);
        pr("Port Ingress Ref Rsrv: %u Frames\n", val2[0]);
        pr("Port Egress  Buf Rsrv: %u Bytes\n", WmDec(val3[0])*L26_BUFFER_CELL_SZ);
        pr("Port Egress  Ref Rsrv: %u Frames\n", val4[0]);
        pr("\n");
        
        pr("Color level:\n");
        /* Configure shared space for  both DP levels (green:0 yellow:1) */
            for (dp = 0; dp < 2; dp++) {
                L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((dp + 254 +   0)), &val1[0]);
                L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((dp + 254 + 256)), &val2[0]);
                L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((dp + 254 + 512)), &val3[0]);
                L26_RD(VTSS_SYS_RES_CTRL_RES_CFG((dp + 254 + 768)), &val4[0]);
                pr("Port DP:%6s Ingress Buf Rsrv: 0x%x\n",dp?"Green":"Yellow",val1[0]);
                pr("Port DP:%6s Ingress Ref Rsrv: 0x%x\n",dp?"Green":"Yellow",val2[0]);
                pr("Port DP:%6s Egress  Buf Rsrv: 0x%x\n",dp?"Green":"Yellow",val3[0]);
                pr("Port DP:%6s Egress  Ref Rsrv: 0x%x\n",dp?"Green":"Yellow",val4[0]);
            }
            pr("\n");
    }

    return VTSS_RC_OK;
}

static vtss_rc l26_debug_info_print(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MISC, l26_debug_misc, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PORT, l26_debug_port, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PORT_CNT, l26_debug_port_cnt, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_VLAN, l26_debug_vlan, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PVLAN, l26_debug_pvlan, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MAC_TABLE, l26_debug_mac_table, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_ACL, l26_debug_acl, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_VXLAT, l26_debug_vxlat, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_QOS, l26_debug_qos, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_AGGR, l26_debug_aggr, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_STP, l26_debug_stp, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MIRROR, l26_debug_mirror, pr, info));
#if defined(VTSS_ARCH_CARACAL)
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_EVC, l26_debug_evc, pr, info));
#endif /* VTSS_ARCH_CARACAL */
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PACKET, l26_debug_pkt, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_TS, l26_debug_ts, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_WM,   l26_debug_wm, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_IPMC, l26_debug_ipmc, pr, info));
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
// Watermark values encoded at little odd for values larger than 1024
static int WmEnc(int value) {
    if (value >= 1024) { 
        return 1024 + value/16;
    }        
    return value;
}

static vtss_rc l26_buf_conf_set(void)
{
    u32 port_no, port, q, dp, i = 0;
    u32 buf_q_rsrv_i, buf_q_rsrv_e, ref_q_rsrv_i, ref_q_rsrv_e, buf_prio_shr_i[8], buf_prio_shr_e[8], ref_prio_shr_i[8], ref_prio_shr_e[8];
    u32 buf_p_rsrv_i, buf_p_rsrv_e, ref_p_rsrv_i, ref_p_rsrv_e, buf_col_shr_i, buf_col_shr_e, ref_col_shr_i, ref_col_shr_e;
    u32 prio_mem_rsrv, prio_ref_rsrv, guaranteed, q_rsrv_mask, prio_mem, prio_ref, oversubscription_factor;

    /*  SYS::RES_CFG : 1024 watermarks for 512 kB shared buffer, unit is 48 byte */
    /*  Is divided into 4 resource consumptions, ingress and egress memory (BUF) and ingress and egress frame reference (REF) blocks */

    /* BUF_xxx_Ingress starts @ offset 0   */
    /* REF_xxx_Ingress starts @ offset 256 */
    /* BUF_xxx_Egress  starts @ offset 512 */
    /* REF_xxx_Egress  starts @ offset 768 */
    /* xxx = q_rsrv, prio_shr, p_rsrv, col_shr */
    
    /* Queue reserved (q_rsrv) : starts @ offset 0 within in each BUF and REF */
    /* Prio shared (prio_shr)  : starts @ offset 216 within in each BUF and REF */
    /* Port reserved (p_rsrv)  : starts @ offset 224 within in each BUF and REF */
    /* Colour shared (col_shr) : starts @ offset 254 within in each BUF and REF */

    /* Buffer values are in BYTES */ 
    buf_q_rsrv_i = 0;     /* Space per queue is not  guaranteed                  */
    buf_q_rsrv_e = 1519;  /* Guarantees all priorities to non-congested traffic stream */
    buf_p_rsrv_i = 512;   /* Guarantees reception of 1 MTUs per ingress port      */
    buf_p_rsrv_e = 4000;  /* Memory shared between priorities per egress port       */
    buf_col_shr_i = L26_BUFFER_MEMORY;    /* Coloring - disabled  */
    buf_col_shr_e = L26_BUFFER_MEMORY;    /* Coloring - disabled  */

    /* Reference values in NUMBER of FRAMES */
    ref_q_rsrv_i = 10;    /* Pending frames per ingress queue              */
    ref_q_rsrv_e = 4;     /* Frames can be pending to each egress queue    */
    ref_p_rsrv_i = 10;    /* Pending frames per ingress port               */
    ref_p_rsrv_e = 10;    /* Frames can be pending for each egress port    */
    ref_col_shr_i = L26_BUFFER_REFERENCE; /* Coloring - disabled           */
    ref_col_shr_e = L26_BUFFER_REFERENCE; /* Coloring - disabled           */

    prio_mem_rsrv = 2000;  /* In the shared area, each priority is cut off 2kB before the others */
    prio_ref_rsrv = 50;    /* And 50 references ... */

    /* The number of supported queues is given through the state structure                           */
    /* The supported queues (lowest to higest) are givin reserved buffer space as specified above.   */
    /* Frames in remaining queues (if any) are not getting any reserved space - but are allowed in the system.*/
    q_rsrv_mask = 0xff >> (8 - vtss_state->qos_conf.prios);

    /* The memory is oversubsrcribed by this factor (factor 1 = 100) */
    if (vtss_state->port_count > 12) {
        oversubscription_factor = 130; 
    } else {
        oversubscription_factor = 100;
    }

    /* **************************************************  */
    /* BELOW, everything is calculated based on the above. */
    /* **************************************************  */

    /* Find the amount of guaranteeed space per port */
    guaranteed = buf_p_rsrv_i+buf_p_rsrv_e;
    for (q=0; q<VTSS_PRIOS; q++) {
        if (q_rsrv_mask & (1<<q)) 
            guaranteed+=(buf_q_rsrv_i+buf_q_rsrv_e);
    }
    prio_mem = (oversubscription_factor * L26_BUFFER_MEMORY)/100 - vtss_state->port_count*guaranteed;
    /* Find the amount of guaranteeed frame references */
    guaranteed = ref_p_rsrv_i+ref_p_rsrv_e;
    for (q=0; q<VTSS_PRIOS; q++) {
        if (q_rsrv_mask & (1<<q)) {
            guaranteed+=(ref_q_rsrv_i+ref_q_rsrv_e);
        }
    }
    prio_ref = L26_BUFFER_REFERENCE - vtss_state->port_count*guaranteed;

    for (q = VTSS_PRIOS-1; ; q--) {
        buf_prio_shr_i[q] = prio_mem;
        ref_prio_shr_i[q] = prio_ref;
        buf_prio_shr_e[q] = prio_mem;
        ref_prio_shr_e[q] = prio_ref;

        if (q_rsrv_mask & (1<<q)) {
            prio_mem -= prio_mem_rsrv;
            prio_ref -= prio_ref_rsrv;
        }
        if (q==0) break;
    }
   
    do { /* Reset default WM */
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(i), 0);
        i++;
    } while (i<1024); 
  
    /* Configure reserved space for all QoS classes per port */
    for (port_no = 0; port_no <= vtss_state->port_count; port_no++) {
        if (port_no == vtss_state->port_count) {
            port = VTSS_CHIP_PORT_CPU;
        } else {
            port = VTSS_CHIP_PORT(port_no);
        }
        for (q = 0; q < VTSS_PRIOS; q++) {
            if (q_rsrv_mask&(1<<q)) {
                L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(port * VTSS_PRIOS + q + 0),   WmEnc(buf_q_rsrv_i/L26_BUFFER_CELL_SZ));
                L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(port * VTSS_PRIOS + q + 256), WmEnc(ref_q_rsrv_i));
                L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(port * VTSS_PRIOS + q + 512), WmEnc(buf_q_rsrv_e/L26_BUFFER_CELL_SZ));
                L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(port * VTSS_PRIOS + q + 768), WmEnc(ref_q_rsrv_e));            
            }
        }
    }

    /* Configure shared space for all QoS classes */
    for (q = 0; q < VTSS_PRIOS; q++) {
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 0)),   WmEnc(buf_prio_shr_i[q]/L26_BUFFER_CELL_SZ));
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 256)), WmEnc(ref_prio_shr_i[q]));
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 512)), WmEnc(buf_prio_shr_e[q]/L26_BUFFER_CELL_SZ));
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 768)), WmEnc(ref_prio_shr_e[q]));
    }

    /* Configure reserved space for all ports */
    for (port_no = 0; port_no <= vtss_state->port_count; port_no++) {
        if (port_no == vtss_state->port_count) {
            port = VTSS_CHIP_PORT_CPU;
        } else {
            port = VTSS_CHIP_PORT(port_no);
        }
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(port + 224 +   0), WmEnc(buf_p_rsrv_i/L26_BUFFER_CELL_SZ));
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(port + 224 + 256), WmEnc(ref_p_rsrv_i));
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(port + 224 + 512), WmEnc(buf_p_rsrv_e/L26_BUFFER_CELL_SZ));
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(port + 224 + 768), WmEnc(ref_p_rsrv_e));
    }

    /* Configure shared space for  both DP levels (green:0 yellow:1) */
    for (dp = 0; dp < 2; dp++) {
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(dp + 254 +   0), WmEnc(buf_col_shr_i/L26_BUFFER_CELL_SZ));
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(dp + 254 + 256), ref_col_shr_i);
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(dp + 254 + 512), WmEnc(buf_col_shr_e/L26_BUFFER_CELL_SZ));
        L26_WR(VTSS_SYS_RES_CTRL_RES_CFG(dp + 254 + 768), ref_col_shr_e);
    }
    return VTSS_RC_OK;
}


static vtss_rc l26_port_map_set(void)
{
    /* We only need to setup the no of avail pgids */
    vtss_state->pgid_count = (VTSS_PGID_LUTON26 - VTSS_CHIP_PORTS + vtss_state->port_count);

    /* And then claim some for flooding */
    VTSS_RC(l26_flood_conf_set());

    /* Setup flooding PGIDs */
    L26_WR(VTSS_ANA_ANA_FLOODING, 
           VTSS_F_ANA_ANA_FLOODING_FLD_UNICAST(PGID_UC) |
           VTSS_F_ANA_ANA_FLOODING_FLD_BROADCAST(PGID_MC) |
           VTSS_F_ANA_ANA_FLOODING_FLD_MULTICAST(PGID_MC));
    
    L26_WR(VTSS_ANA_ANA_FLOODING_IPMC, 
           VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC4_CTRL(PGID_MC) |
           VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC4_DATA(PGID_MCIPV4) |
           VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC6_CTRL(PGID_MC) |
           VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC6_DATA(PGID_MCIPV6));

    /* Setup WM (depends on the port map)  */
    VTSS_RC(l26_buf_conf_set());
       
    /* Setup Rx CPU queue system */
    VTSS_RC(l26_rx_conf_set()); 

    return VTSS_RC_OK;
}   

static vtss_rc l26_restart_conf_set(void)
{
    L26_WR(VTSS_DEVCPU_GCB_CHIP_REGS_GENERAL_PURPOSE, vtss_cmn_restart_value_get());

    return VTSS_RC_OK;
}

static vtss_rc l26_init_conf_set(void)
{
    vtss_init_conf_t *conf = &vtss_state->init_conf;
    vtss_vid_t       vid;
    u32              value, port;
    int              i, pcp, dei;

    VTSS_D("enter");

    // For register access checking.
    // Avoid Lint Warning 572: Excessive shift value (precision 1 shifted right by 2),
    // which is due to use of the first target in the range (VTSS_DEVCPU_ORG).
    /*lint --e{572} */
    vtss_state->reg_check.addr = VTSS_DEVCPU_ORG_ORG_ERR_CNTS;

    VTSS_RC(l26_setup_cpu_if(conf));

    /* Unfortunately a switch-core-only reset also affects the VCore-III
       bus and MIPS frequency, and thereby also the DDR SDRAM controller,
       so until we have a work-around, we skip resetting the switch core. */
#if 0
    {
        u32 gpio_alt[2], i
        /* Before we reset the switch core below, save the current
           GPIO alternate function settings, so that we can restore
           them after the reset. These two registers are reset to their
           defaults with a switch core reset, and are used to select
           a.o. UART operation on two GPIOs. */
        for (i = 0; i < 2; i++) {
            L26_RD(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(i), &gpio_alt[i]);
        }

        /* Top-level switch core reset */
        /* Prevent VCore-III from being reset with a chip soft reset */
        L26_WRM_SET(VTSS_ICPU_CFG_CPU_SYSTEM_CTRL_RESET, VTSS_F_ICPU_CFG_CPU_SYSTEM_CTRL_RESET_CORE_RST_PROTECT);
        /* Then reset switch core + internal PHYs. */
        L26_WR(VTSS_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST,
               VTSS_F_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST_SOFT_PHY_RST |
               VTSS_F_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST_SOFT_CHIP_RST);
        /* Unprevent VCore-III from being reset with a chip soft reset */
        L26_WRM_CLR(VTSS_ICPU_CFG_CPU_SYSTEM_CTRL_RESET, VTSS_F_ICPU_CFG_CPU_SYSTEM_CTRL_RESET_CORE_RST_PROTECT);

        /* Time to restore GPIO alternate function control */
        for (i = 0; i < 2; i++) {
            L26_WR(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(i), gpio_alt[i]);
        }

        /* After the reset, re-setup the CPU I/F */
        VTSS_RC(l26_setup_cpu_if(conf));
    }
#endif

    /* Read chip ID to check CPU interface */
    VTSS_RC(l26_chip_id_get(&vtss_state->chip_id));
    VTSS_I("chip_id: 0x%04x, revision: 0x%04x", 
           vtss_state->chip_id.part_number, vtss_state->chip_id.revision);

    /* Read restart type */
    L26_RD(VTSS_DEVCPU_GCB_CHIP_REGS_GENERAL_PURPOSE, &value);
    vtss_state->init_conf.warm_start_enable = 0; /* Warm start not supported */
    VTSS_RC(vtss_cmn_restart_update(value));

    /* Set up the Serdes1g/Serdes6g macros incl. switch_mode. */
    VTSS_RC(l26_serdes_macro_config());

    /* Initialize memories */
    L26_WR(VTSS_SYS_SYSTEM_RESET_CFG,
           VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_ENA|VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_INIT);
    i = 0;
    do {
        VTSS_MSLEEP(1); /* MEM_INIT should clear after appx. 22us */
        L26_RD(VTSS_SYS_SYSTEM_RESET_CFG, &value);
    } while(i++ < 10 && (value & VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_INIT));
    if(value & VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_INIT) {
        VTSS_E("Memory initialization error, SYS::RESET_CFG: 0x%08x", value);
        return VTSS_RC_ERROR;
    }

    /* Enable switch core */
    L26_WRM_SET(VTSS_SYS_SYSTEM_RESET_CFG, VTSS_F_SYS_SYSTEM_RESET_CFG_CORE_ENA);

    /* Release reset of internal Phys */
    L26_WRM_CLR(VTSS_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST, VTSS_F_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST_SOFT_PHY_RST);
    for (port = 0; port < 12; port++) { /* VTSS_CHIP_PORTS */
        if (port > 9) {
            L26_WRM_CLR(VTSS_DEV_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port)), 
                        VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PHY_RST);
        } else {
            L26_WRM_CLR(VTSS_DEV_GMII_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port)), 
                        VTSS_F_DEV_GMII_PORT_MODE_CLOCK_CFG_PHY_RST);
        }
    }

    /* Clear MAC table */
    L26_WR(VTSS_ANA_ANA_TABLES_MACACCESS,
           VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_TABLE_CLEAR));

    /* Clear VLAN table */
    L26_WR(VTSS_ANA_ANA_TABLES_VLANACCESS,
           VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(VLAN_CMD_TABLE_CLEAR));

    /* Disable Port ACLs */
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        L26_WRM_CLR(VTSS_ANA_PORT_VCAP_CFG(port), 
                    VTSS_F_ANA_PORT_VCAP_CFG_S1_ENA|
                    VTSS_F_ANA_PORT_VCAP_CFG_S2_ENA); /* Disable S1/S2 VCAP */
        L26_WRM_CLR(VTSS_REW_PORT_PORT_CFG(port),
                    VTSS_F_REW_PORT_PORT_CFG_ES0_ENA); /* Disable ES0 */
    }
    /* Clear ACL table */
    VTSS_RC(l26_vcap_initialize(&tcam_info[VTSS_TCAM_S1]));
    VTSS_RC(l26_vcap_initialize(&tcam_info[VTSS_TCAM_S2]));
    VTSS_RC(l26_vcap_initialize(&tcam_info[VTSS_TCAM_ES0]));

    /* Enable Port ACLs (no entries) */
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        /* Enable S1 and S2, PTP in S2 second lookup and treat IPv6 as IPv4 in both S2 lookups */
        L26_WR(VTSS_ANA_PORT_VCAP_CFG(port), 
               VTSS_F_ANA_PORT_VCAP_CFG_S1_ENA |
               VTSS_F_ANA_PORT_VCAP_CFG_S2_UDP_PAYLOAD_ENA(2) |
               VTSS_F_ANA_PORT_VCAP_CFG_S2_ETYPE_PAYLOAD_ENA(2) |
               VTSS_F_ANA_PORT_VCAP_CFG_S2_ENA |
               VTSS_F_ANA_PORT_VCAP_CFG_S2_IP6_STD_DIS(3));
        L26_WRM_SET(VTSS_REW_PORT_PORT_CFG(port),
                    VTSS_F_REW_PORT_PORT_CFG_ES0_ENA); /* Enable ES0 */
    }

    /* Clear VLAN table masks */
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        if (vid == VTSS_VID_DEFAULT) /* Default VLAN includes all ports */
            continue;
        L26_WR(VTSS_ANA_ANA_TABLES_VLANTIDX,
               VTSS_F_ANA_ANA_TABLES_VLANTIDX_V_INDEX(vid));
        L26_WR(VTSS_ANA_ANA_TABLES_VLANACCESS,
               /* Zero port mask */
               VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(VLAN_CMD_WRITE));
        VTSS_RC(l26_vlan_table_idle());
    }

    /* Setup VLAN configuration */
    VTSS_RC(l26_vlan_conf_apply(0));

    /* Setup chip ports */
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        /* Clear counters */
        L26_WRM(VTSS_SYS_SYSTEM_STAT_CFG,
                VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_CLEAR_PORT(port) |
                VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_CLEAR_SHOT,
                VTSS_M_SYS_SYSTEM_STAT_CFG_STAT_CLEAR_PORT);

        if (port == VTSS_CHIP_PORT_CPU) {
            /* Setup the CPU port as VLAN aware to support switching frames based on tags */
            L26_WR(VTSS_ANA_PORT_VLAN_CFG(port), 
                   VTSS_F_ANA_PORT_VLAN_CFG_VLAN_AWARE_ENA  |
                   VTSS_F_ANA_PORT_VLAN_CFG_VLAN_POP_CNT(1) |
                   VTSS_F_ANA_PORT_VLAN_CFG_VLAN_VID(1));

            /* Disable learning (only RECV_ENA must be set) */
            L26_WR(VTSS_ANA_PORT_PORT_CFG(port), VTSS_F_ANA_PORT_PORT_CFG_RECV_ENA);
        } else {
            /* Default VLAN port configuration */
            VTSS_RC(l26_vlan_port_conf_apply(port, 
                                             &vtss_state->vlan_port_conf[VTSS_PORT_NO_START]));
        }
    } /* Port loop */

    /* Set-up default packet Rx endianness and position of status word. */
    for (i = 0; i < VTSS_PACKET_RX_GRP_CNT; i++) {
        /* Little-endian and status word before last data */
        L26_WR(VTSS_DEVCPU_QS_XTR_XTR_GRP_CFG(i), VTSS_F_DEVCPU_QS_XTR_XTR_GRP_CFG_BYTE_SWAP);
    }

    /* Setup CPU port 0 and 1 to allow for classification of transmission of
     * switched frames into a user-module-specifiable QoS class.
     * For the two CPU ports, we set a one-to-one mapping between a VLAN tag's
     * PCP and a QoS class. When transmitting switched frames, the PCP value
     * of the VLAN tag (which is always inserted to get it switched on a given
     * VID), then controls the priority. */
    /* Enable looking into PCP bits */
    L26_WR(VTSS_ANA_PORT_QOS_CFG(VTSS_CHIP_PORT_CPU), VTSS_F_ANA_PORT_QOS_CFG_QOS_PCP_ENA);

    /* Disable aging of Rx CPU queues to allow the frames to stay there longer than
     * on normal front ports. */
    L26_WRM_SET(VTSS_REW_PORT_PORT_CFG(VTSS_CHIP_PORT_CPU), VTSS_F_REW_PORT_PORT_CFG_AGE_DIS);

    /* Disallow the CPU Rx queues to use shared memory. */
    L26_WRM_SET(VTSS_SYS_SYSTEM_EGR_NO_SHARING, VTSS_BIT(VTSS_CHIP_PORT_CPU));

    /* Set-up the one-to-one mapping */
    for (pcp = 0; pcp < VTSS_PCP_END - VTSS_PCP_START; pcp++) {
        for (dei = 0; dei < VTSS_DEI_END - VTSS_DEI_START; dei++) {
            L26_WR(VTSS_ANA_PORT_QOS_PCP_DEI_MAP_CFG(VTSS_CHIP_PORT_CPU, (8 * dei + pcp)), VTSS_F_ANA_PORT_QOS_PCP_DEI_MAP_CFG_QOS_PCP_DEI_VAL(pcp));
        }
    }

    /* Setup aggregation mode */
    VTSS_RC(l26_aggr_mode_set());
    
    /* Set MAC age time to default value */
    VTSS_RC(l26_mac_table_age_time_set());

    /* Disable learning for frames discarded by VLAN ingress filtering */
    L26_WR(VTSS_ANA_ANA_ADVLEARN, VTSS_F_ANA_ANA_ADVLEARN_VLAN_CHK);

    /* Initialize leaky buckets */
    L26_WR(VTSS_SYS_SCH_SCH_LB_CTRL, VTSS_F_SYS_SCH_SCH_LB_CTRL_LB_INIT);

    /* Setup frame ageing - fixed value "2 sec" - in 4ns units */
    L26_WR(VTSS_SYS_SYSTEM_FRM_AGING, (2 * 1000000 * 1000 / 4));

    /* Wait for internal PHYs to be ready */
    VTSS_MSLEEP(500);

    // Power down all PHY ports as default in order to save power
    for (port = 0; port < 12; port++) { /* VTSS_CHIP_PORTS */
        (void) l26_miim_write(VTSS_MIIM_CONTROLLER_0, port, 0, 1<<11, FALSE);
    }
            

    do { /* Wait until leaky buckets initialization is completed  */
        L26_RD(VTSS_SYS_SCH_SCH_LB_CTRL, &value);                       
    } while(value & VTSS_F_SYS_SCH_SCH_LB_CTRL_LB_INIT);

#if defined(VTSS_FEATURE_TIMESTAMP)
    L26_WR(VTSS_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL, 
           VTSS_F_DEVCPU_GCB_PTP_TIMERS_PTP_TIMER_CTRL_PTP_TIMER_ENA);
    L26_WR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_PTP_ENA);
    L26_WR(VTSS_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG,  HW_CLK_CNT_PR_SEC |
            VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_UPD);
    /* release all timestamp id's except those that are reserved for SW*/
    L26_WR(VTSS_ANA_ANA_TABLES_PTP_ID_LOW, VTSS_ENCODE_BITMASK(TS_IDS_RESERVED_FOR_SW,32-TS_IDS_RESERVED_FOR_SW));
    L26_WR(VTSS_ANA_ANA_TABLES_PTP_ID_HIGH, 0xffffffff); /* assuming <32 are reserved for SW */
    

#endif    
    return VTSS_RC_OK;
}

vtss_rc vtss_luton26_inst_create(void)
{
    vtss_cil_func_t *func = &vtss_state->cil_func;
    vtss_vcap_obj_t *is1 = &vtss_state->is1.obj;
    vtss_vcap_obj_t *is2 = &vtss_state->is2.obj;
    vtss_vcap_obj_t *es0 = &vtss_state->es0.obj;

    /* Initialization */
    func->init_conf_set = l26_init_conf_set;
    func->miim_read = l26_miim_read;
    func->miim_write = l26_miim_write;
    func->restart_conf_set = l26_restart_conf_set;

    /* Miscellaneous */
    func->reg_read = l26_reg_read;
    func->reg_write = l26_reg_write;
    func->chip_id_get = l26_chip_id_get;
    func->poll_1sec = l26_poll_1sec;
    func->gpio_mode = l26_gpio_mode;
    func->gpio_read = l26_gpio_read;
    func->gpio_write = l26_gpio_write;
    func->sgpio_conf_set = l26_sgpio_conf_set;
    func->sgpio_read = l26_sgpio_read;
    func->sgpio_event_enable = l26_sgpio_event_enable;
    func->sgpio_event_poll = l26_sgpio_event_poll;
    func->debug_info_print = l26_debug_info_print;
    func->ptp_event_poll = l26_ptp_event_poll;
    func->ptp_event_enable = l26_ptp_event_enable;
    func->intr_cfg = l26_intr_cfg;
    func->intr_pol_negation = l26_intr_pol_negation;


    /* Port control */
    func->port_map_set = l26_port_map_set;
    func->port_conf_set = l26_port_conf_set;
    func->port_clause_37_status_get = l26_port_clause_37_status_get;
    func->port_clause_37_control_set = l26_port_clause_37_control_set;
    func->port_status_get = l26_port_status_get;
    func->port_counters_update = l26_port_counters_update;
    func->port_counters_clear = l26_port_counters_clear;
    func->port_counters_get = l26_port_counters_get;
    func->port_basic_counters_get = l26_port_basic_counters_get;

    /* QoS */
    func->qos_conf_set = l26_qos_conf_set;
    func->qos_port_conf_set = vtss_cmn_qos_port_conf_set;
    func->qos_port_conf_update = l26_qos_port_conf_set;
    func->qce_add = vtss_cmn_qce_add;
    func->qce_del = vtss_cmn_qce_del;

    /* Layer 2 */
    func->mac_table_add = l26_mac_table_add;
    func->mac_table_del = l26_mac_table_del;
    func->mac_table_get = l26_mac_table_get;
    func->mac_table_get_next = l26_mac_table_get_next;
    func->mac_table_age_time_set = l26_mac_table_age_time_set;
    func->mac_table_age = l26_mac_table_age;
    func->mac_table_status_get = l26_mac_table_status_get;
    func->learn_port_mode_set = l26_learn_port_mode_set;
    func->learn_state_set = l26_learn_state_set;
    func->port_forward_set = l26_port_forward_set;
    func->mstp_state_set = vtss_cmn_mstp_state_set;
    func->mstp_vlan_msti_set = vtss_cmn_vlan_members_set;
    func->erps_vlan_member_set = vtss_cmn_erps_vlan_member_set;
    func->erps_port_state_set = vtss_cmn_erps_port_state_set;
    func->pgid_table_write = l26_pgid_table_write;
    func->src_table_write = l26_src_table_write;
    func->aggr_table_write = l26_aggr_table_write;
    func->aggr_mode_set = l26_aggr_mode_set;
    func->pmap_table_write = l26_pmap_table_write;
    func->vlan_conf_set = l26_vlan_conf_set;
    func->vlan_port_conf_set = vtss_cmn_vlan_port_conf_set;
    func->vlan_port_conf_update = l26_vlan_port_conf_update;
    func->vlan_port_members_set = vtss_cmn_vlan_members_set;
    func->vlan_mask_update = l26_vlan_mask_update;
    func->vlan_tx_tag_set = vtss_cmn_vlan_tx_tag_set;
    func->isolated_vlan_set = vtss_cmn_vlan_members_set;
    func->isolated_port_members_set = l26_isolated_port_members_set;
    func->flood_conf_set = l26_flood_conf_set;
    func->ipv4_mc_add = vtss_cmn_ipv4_mc_add;
    func->ipv4_mc_del = vtss_cmn_ipv4_mc_del;
    func->ipv6_mc_add = vtss_cmn_ipv6_mc_add;
    func->ipv6_mc_del = vtss_cmn_ipv6_mc_del;
    func->ip_mc_update = l26_ip_mc_update;
    func->mirror_port_set = l26_mirror_port_set;
    func->mirror_ingress_set = l26_mirror_ingress_set;
    func->mirror_egress_set = l26_mirror_egress_set;
    func->mirror_cpu_ingress_set = l26_mirror_cpu_ingress_set;
    func->mirror_cpu_egress_set = l26_mirror_cpu_egress_set;
    func->eps_port_set = vtss_cmn_eps_port_set;
#ifdef VTSS_FEATURE_SFLOW
    func->sflow_port_conf_set = l26_sflow_port_conf_set;
    func->sflow_sampling_rate_convert = l26_sflow_sampling_rate_convert;
#endif /* VTSS_FEATURE_SFLOW */
#if defined(VTSS_FEATURE_VCL) 
    func->vcl_port_conf_set = l26_vcl_port_conf_set;
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
    /* Packet control */
    func->rx_conf_set      = l26_rx_conf_set;
    func->rx_frame_get     = l26_rx_frame_get;
    func->rx_frame_discard = l26_rx_frame_discard;
    func->tx_frame_port    = l26_tx_frame_port;
    func->rx_hdr_decode    = l26_rx_hdr_decode;
    func->tx_hdr_encode    = l26_tx_hdr_encode;
#if defined(VTSS_FEATURE_NPI)
    func->npi_conf_set = l26_npi_conf_set;
#endif /* VTSS_FEATURE_NPI */

#if defined(VTSS_FEATURE_EEE)
    /* EEE */
    func->eee_port_conf_set = l26_eee_port_conf_set;
#endif    
    func->fan_controller_init = l26_fan_controller_init;
    func->fan_cool_lvl_get    = l26_fan_cool_lvl_get;
    func->fan_cool_lvl_set    = l26_fan_cool_lvl_set;
    func->fan_rotation_get    = l26_fan_rotation_get;

#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    func->evc_policer_conf_set = l26_evc_policer_conf_set;
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */
#if defined(VTSS_ARCH_CARACAL)
    func->evc_port_conf_set = l26_evc_port_conf_set;
    func->evc_add = vtss_cmn_evc_add;
    func->evc_del = vtss_cmn_evc_del;
    func->ece_add = vtss_cmn_ece_add;
    func->ece_del = vtss_cmn_ece_del;
    func->ece_update = l26_ece_update;
    func->evc_update = l26_evc_update;
    vtss_state->evc_info.max_count = 256;
    vtss_state->ece_info.max_count = 256;
    func->mce_add = vtss_cmn_mce_add;
    func->mce_del = vtss_cmn_mce_del;
#endif /* VTSS_ARCH_CARACAL */

    /* ACL */
    func->acl_policer_set = l26_acl_policer_set;
    func->acl_port_set = l26_acl_port_conf_set;
    func->acl_port_counter_get = l26_acl_port_counter_get;
    func->acl_port_counter_clear = l26_acl_port_counter_clear;
    func->acl_ace_add = l26_ace_add;
    func->acl_ace_del = l26_ace_del;
    func->acl_ace_counter_get = l26_ace_counter_get;
    func->acl_ace_counter_clear = l26_ace_counter_clear;
    func->vcap_range_commit = l26_vcap_range_commit;

    /* IS1 */
    is1->max_count = VTSS_L26_IS1_CNT;
    is1->entry_get = l26_is1_entry_get;
    is1->entry_add = l26_is1_entry_add;
    is1->entry_del = l26_is1_entry_del;
    is1->entry_move = l26_is1_entry_move;

    /* IS2 */
    is2->max_count = VTSS_L26_IS2_CNT;
    is2->entry_get = l26_is2_entry_get;
    is2->entry_add = l26_is2_entry_add;
    is2->entry_del = l26_is2_entry_del;
    is2->entry_move = l26_is2_entry_move;

    /* ES0 */
    es0->max_count = VTSS_L26_ES0_CNT;
    es0->entry_get = l26_es0_entry_get;
    es0->entry_add = l26_es0_entry_add;
    es0->entry_del = l26_es0_entry_del;
    es0->entry_move = l26_es0_entry_move;
    func->es0_entry_update = l26_es0_entry_update;

    /* Time stamping features */
#if defined(VTSS_FEATURE_TIMESTAMP)
    func->ts_timeofday_get = l26_ts_timeofday_get;
    func->ts_timeofday_set = l26_ts_timeofday_set;
    func->ts_timeofday_set_delta = l26_ts_timeofday_set_delta;
    func->ts_timeofday_offset_set = l26_ts_timeofday_offset_set;
    func->ts_ns_cnt_get = l26_ts_ns_cnt_get;
    func->ts_timeofday_one_sec = l26_ts_timeofday_one_sec;
    func->ts_adjtimer_set = l26_ts_adjtimer_set;
    func->ts_freq_offset_get = l26_ts_freq_offset_get;
    func->ts_external_clock_mode_set = l26_ts_external_clock_mode_set;
    func->ts_ingress_latency_set = l26_ts_ingress_latency_set;
    func->ts_p2p_delay_set = l26_ts_p2p_delay_set;
    func->ts_egress_latency_set = l26_ts_egress_latency_set;
    func->ts_operation_mode_set = l26_ts_operation_mode_set;
    func->ts_timestamp_get = l26_ts_timestamp_get;
    func->ts_timestamp_id_release = l26_ts_timestamp_id_release;
    
#endif /* VTSS_FEATURE_TIMESTAMP */
    
    /* SYNCE features */
#if defined(VTSS_FEATURE_SYNCE)
    func->synce_clock_out_set = l26_synce_clock_out_set;
    func->synce_clock_in_set = l26_synce_clock_in_set;
#endif /* VTSS_FEATURE_SYNCE */

    /* State data */
    vtss_state->gpio_count = L26_GPIOS;
    vtss_state->sgpio_group_count = L26_SGPIO_GROUPS;
    vtss_state->prio_count = L26_PRIOS;
    vtss_state->ac_count = L26_ACS;
    vtss_state->rx_queue_count = VTSS_PACKET_RX_QUEUE_CNT;
    /* Reserve policer for ACL discarding) */
    vtss_state->policer_user[L26_ACL_POLICER_DISC] = VTSS_POLICER_USER_DISCARD;

#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
    vcoreiii_fdma_func_init();
#endif

    return VTSS_RC_OK;
}

#endif /* VTSS_ARCH_LUTON26 */
