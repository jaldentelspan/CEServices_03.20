/*

 Vitesse API software.

 Copyright (c) 2002-2011 Vitesse Semiconductor Corporation "Vitesse". All
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
#define VTSS_IOADDR(t,o) ((((t) - VTSS_IO_ORIGIN1_OFFSET) >> 2) + (o))
#define VTSS_IOREG(t,o)  (VTSS_IOADDR(t,o))

#define VTSS_TRACE_LAYER VTSS_TRACE_LAYER_CIL

// Avoid "vtss_api.h not used in module vtss_serval.c"
/*lint --e{766} */

#include "vtss_api.h"
#include "vtss_state.h"

#if defined(VTSS_ARCH_SERVAL)
#include "vtss_common.h"
#include "vtss_serval.h"
#include "vtss_serval_reg.h"
#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
#include "vtss_serval_fdma.h"
#endif
#include "vtss_util.h"

#if defined(VTSS_ARCH_SEVILLE)
#define VTSS_CHIP_PORTS      10    /* Port 0-9 */
#else
#define VTSS_CHIP_PORTS      11    /* Port 0-10 */
#endif /* VTSS_ARCH_SEVILLE */
#define VTSS_CHIP_PORT_CPU   VTSS_CHIP_PORTS /* Next port is CPU port */
#define VTSS_CHIP_PORT_CPU_0 (VTSS_CHIP_PORT_CPU + 0) /* Aka. CPU Port 11 */
#define VTSS_CHIP_PORT_CPU_1 (VTSS_CHIP_PORT_CPU + 1) /* Aka. CPU Port 12 */
#define VTSS_CHIP_PORT_MASK  VTSS_BITMASK(VTSS_CHIP_PORTS) /* Chip port mask */

#define SRVL_ACS          16  /* Number of aggregation masks */
#define SRVL_PRIOS        8   /* Number of priorities */
#define SRVL_GPIOS        32  /* Number of GPIOs */
#define SRVL_SGPIO_GROUPS 1   /* Number of SGPIO groups */

/* Reserved PGIDs */
#define PGID_UC      (VTSS_PGID_LUTON26 - 4)
#define PGID_MC      (VTSS_PGID_LUTON26 - 3)
#define PGID_MCIPV4  (VTSS_PGID_LUTON26 - 2)
#define PGID_MCIPV6  (VTSS_PGID_LUTON26 - 1)
#define PGID_AGGR    (VTSS_PGID_LUTON26)
#define PGID_SRC     (PGID_AGGR + SRVL_ACS)

/* Policers */
#define SRVL_POLICER_PORT    0    /* 0-11    : Port policers (0-10 used, 11 unused) */
#define SRVL_POLICER_ACL     12   /* 12-31   : ACL policers (12-27 used, 28-31 unused) */
#define SRVL_POLICER_QUEUE   32   /* 32-127  : Queue policers (32-119 used, 120-127 unused) */ 
#if defined(VTSS_CHIP_SERVAL)
#define SRVL_POLICER_EVC     129  /* 129-1150: EVC policers (128 unused) */
#define SRVL_POLICER_DISCARD 1151 /* 1151    : Discard policer */
#define SRVL_POLICER_CNT     1152 /* Total number of policers */
#elif defined(VTSS_CHIP_SERVAL_LITE)
#define SRVL_POLICER_EVC     129  /* 129-384 : EVC policers (128 unused) */
#define SRVL_POLICER_DISCARD 385  /* 385     : Discard policer */
#define SRVL_POLICER_CNT     386  /* Total number of policers */
#else
#define SRVL_POLICER_EVC     129  /* 129-192 : EVC policers (128 unused) */
#define SRVL_POLICER_DISCARD 193  /* 193     : Discard policer */
#define SRVL_POLICER_CNT     194  /* Total number of policers */
#endif /* VTSS_CHIP_SERVAL */

#if defined(VTSS_CHIP_SERVAL)
#define SRVL_EVC_CNT 1024
#else
#define SRVL_EVC_CNT 256
#endif /* VTSS_CHIP_SERVAL */

/* Buffer constants */
#define SRVL_BUFFER_MEMORY 1024000
#define SRVL_BUFFER_REFERENCE 11000
#define SRVL_BUFFER_CELL_SZ 60


#define SRVL_DIV_ROUND_UP(x, base) ((x + base - 1)/base)

/* ================================================================= *
 *  Register access
 * ================================================================= */
#define SRVL_RD(p, value)                 \
    {                                     \
        vtss_rc __rc = srvl_rd(p, value); \
        if (__rc != VTSS_RC_OK)           \
            return __rc;                  \
    }

#define SRVL_WR(p, value)                 \
    {                                     \
        vtss_rc __rc = srvl_wr(p, value); \
        if (__rc != VTSS_RC_OK)           \
            return __rc;                  \
    }

#define SRVL_WRM(p, value, mask)                 \
    {                                            \
        vtss_rc __rc = srvl_wrm(p, value, mask); \
        if (__rc != VTSS_RC_OK)                  \
            return __rc;                         \
    }

#define SRVL_WRM_SET(p, mask) SRVL_WRM(p, mask, mask)
#define SRVL_WRM_CLR(p, mask) SRVL_WRM(p, 0,    mask)
#define SRVL_WRM_CTL(p, _cond_, mask) SRVL_WRM(p, (_cond_) ? mask : 0, mask)

/* Decode register bit field */
#define SRVL_BF(name, value) ((VTSS_F_##name & value) ? 1 : 0)

static vtss_rc srvl_wr(u32 addr, u32 value);
static vtss_rc srvl_rd(u32 addr, u32 *value);

#if !VTSS_OPT_VCORE_III
static inline BOOL srvl_reg_directly_accessible(u32 addr)
{
    /* Running on external CPU. VCoreIII registers require indirect access. */
    /* On internal CPU, all registers are always directly accessible. */
    return (addr >= ((VTSS_IO_ORIGIN2_OFFSET - VTSS_IO_ORIGIN1_OFFSET) >> 2));
}
#endif /* !VTSS_OPT_VCORE_III */

#if !VTSS_OPT_VCORE_III
/* Read or write register indirectly */
static vtss_rc srvl_reg_indirect_access(u32 addr, u32 *value, BOOL is_write)
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

    if ((result = srvl_wr(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_ADDR, addr)) != VTSS_RC_OK) {
        goto do_exit;
    }
    if (is_write) {
        if ((result = srvl_wr(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, *value)) != VTSS_RC_OK) {
            goto do_exit;
        }
        // Wait for operation to complete
        do {
            if ((result = srvl_rd(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL, &ctrl)) != VTSS_RC_OK) {
                goto do_exit;
            }
        } while (ctrl & VTSS_F_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL_VA_BUSY);
    } else {
        // Dummy read to initiate access
        if ((result = srvl_rd(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, value)) != VTSS_RC_OK) {
            goto do_exit;
        }
        // Wait for operation to complete
        do {
            if ((result = srvl_rd(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL, &ctrl)) != VTSS_RC_OK) {
                goto do_exit;
            }
        } while (ctrl & VTSS_F_DEVCPU_GCB_VCORE_ACCESS_VA_CTRL_VA_BUSY);
        if ((result = srvl_rd(VTSS_DEVCPU_GCB_VCORE_ACCESS_VA_DATA, value)) != VTSS_RC_OK) {
            goto do_exit;
        }
    }

do_exit:
    VTSS_OS_SCHEDULER_UNLOCK(flags);
    return result;
}
#endif /* !VTSS_OPT_VCORE_III */

/* Read target register using current CPU interface */
static vtss_rc srvl_rd(u32 reg, u32 *value)
{
#if VTSS_OPT_VCORE_III
    return vtss_state->init_conf.reg_read(0, reg, value);
#else
    if (srvl_reg_directly_accessible(reg)) {
        return vtss_state->init_conf.reg_read(0, reg, value);
    } else {
        return srvl_reg_indirect_access(reg, value, FALSE);
    }
#endif
}

/* Write target register using current CPU interface */
static vtss_rc srvl_wr(u32 reg, u32 value)
{
#if VTSS_OPT_VCORE_III
    return vtss_state->init_conf.reg_write(0, reg, value);
#else
    if (srvl_reg_directly_accessible(reg)) {
        return vtss_state->init_conf.reg_write(0, reg, value);
    } else {
        return srvl_reg_indirect_access(reg, &value, TRUE);
    }
#endif
}

/* Read-modify-write target register using current CPU interface */
static vtss_rc srvl_wrm(u32 reg, u32 value, u32 mask)
{
    vtss_rc rc;
    u32     val;

    if ((rc = srvl_rd(reg, &val)) == VTSS_RC_OK) {
        val = ((val & ~mask) | (value & mask));
        rc = srvl_wr(reg, val);
    }
    return rc;
}

/* ================================================================= *
 *  Forward declarations
 * ================================================================= */
static vtss_rc srvl_gpio_mode(const vtss_chip_no_t   chip_no,
                              const vtss_gpio_no_t   gpio_no,
                              const vtss_gpio_mode_t mode);

static vtss_rc srvl_gpio_write(const vtss_chip_no_t  chip_no,
                               const vtss_gpio_no_t  gpio_no, 
                               const BOOL            value);

static vtss_rc srvl_port_policer_fc_set(const vtss_port_no_t port_no, u32 chipport);
static vtss_rc srvl_debug_range_checkers(const vtss_debug_printf_t pr,
                                         const vtss_debug_info_t   *const info);
static vtss_rc srvl_debug_is1_all(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info);
/* ================================================================= *
 *  Utility functions
 * ================================================================= */

static u32 srvl_port_mask(const BOOL member[])
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
static u32 srvl_chip_pgid(u32 pgid)
{
    if (pgid < vtss_state->port_count) {
        return VTSS_CHIP_PORT(pgid);
    } else {
        return (pgid + VTSS_CHIP_PORTS - vtss_state->port_count);
    }
}

/* Convert from PGID in chip */
static u32 srvl_vtss_pgid(u32 pgid)
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
 *  Debug print utility functions
 * ================================================================= */

static void srvl_debug_print_header(const vtss_debug_printf_t pr, const char *txt)
{
    pr("%s:\n\n", txt);
}

static void srvl_debug_print_port_header(const vtss_debug_printf_t pr, const char *txt)
{
    vtss_debug_print_port_header(pr, txt, VTSS_CHIP_PORTS + 1, 1);
}

static void srvl_debug_print_mask(const vtss_debug_printf_t pr, u32 mask)
{
    u32 port;
    
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        pr("%s%s", port == 0 || (port & 7) ? "" : ".", ((1<<port) & mask) ? "1" : "0");
    }
    pr("  0x%08x\n", mask);
}

static void srvl_debug_reg_header(const vtss_debug_printf_t pr, const char *name) 
{
    char buf[64];
    
    sprintf(buf, "%-32s  Tgt   Addr", name);
    vtss_debug_print_reg_header(pr, buf);
}

static void srvl_debug_reg(const vtss_debug_printf_t pr, u32 addr, const char *name)
{
    u32 value;
    char buf[64];

    if (srvl_rd(addr, &value) == VTSS_RC_OK) {
        sprintf(buf, "%-32s  0x%02x  0x%04x", name, (addr >> 14) & 0x3f, addr & 0x3fff);
        vtss_debug_print_reg(pr, buf, value);
    }
}

static void srvl_debug_reg_inst(const vtss_debug_printf_t pr, u32 addr, u32 i, const char *name)
{
    char buf[64];

    sprintf(buf, "%s_%u", name, i);
    srvl_debug_reg(pr, addr, buf);
}

#define VTSS_TCAM_ENTRY_WIDTH   12 /* Max entry width (32bit words) */
#define VTSS_TCAM_COUNTER_WIDTH 4  /* Max counter width (32bit words) */

typedef struct {
    u32 entry[VTSS_TCAM_ENTRY_WIDTH];     /* ENTRY_DAT */
    u32 mask[VTSS_TCAM_ENTRY_WIDTH];      /* MASK_DAT */
    u32 action[VTSS_TCAM_ENTRY_WIDTH];    /* ACTION_DAT */
    u32 counter[VTSS_TCAM_COUNTER_WIDTH]; /* CNT_DAT */
    u32 tg;                               /* TG_DAT */
    u32 type;                             /* Action type */
    u32 tg_sw;                            /* Current type-group */
    u32 cnt;                              /* Current counter */
    u32 key_offset;                       /* Current entry offset */
    u32 action_offset;                    /* Current action offset */
    u32 counter_offset;                   /* Current counter offset */
    u32 tg_value;                         /* Current type-group value */
    u32 tg_mask;                          /* Current type-group mask */
} srvl_tcam_data_t;

typedef struct {
    BOOL                is_action;
    vtss_debug_printf_t pr;
    srvl_tcam_data_t    data;
} srvl_debug_info_t;

static void srvl_debug_bits(srvl_debug_info_t *info, const char *name, u32 offset, u32 len)
{
    vtss_debug_printf_t pr = info->pr;
    srvl_tcam_data_t    *data = &info->data;
    u32                 i,j;

    if (name)
        pr("%s: ", name);
    for (i = 0; i < len; i++) {
        j = (len - 1 - i);
        if (i != 0 && (j % 8) == 7)
            pr(".");
        j += (offset + (info->is_action ? data->action_offset : data->key_offset));
        pr("%c", info->is_action ? vtss_bs_bit_get(data->action, j) ? '1' : '0' :
           vtss_bs_bit_get(data->mask, j) ? 
           (vtss_bs_bit_get(data->entry, j) ? '1' : '0') : 'X');
    }
    pr(len > 24 ? "\n" : " ");
}

static void srvl_debug_bit(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bits(info, name, offset, 1);
}

/* Debug bytes in chunks of 32-bit data */
static void srvl_debug_bytes(srvl_debug_info_t *info, const char *name, u32 offset, u32 len)
{
    u32  i, n, count;
    char buf[64];
    
    n = (len % 32);
    if (n == 0)
        n = 32;
    if (len <= 32) {
        /* One chunk */
        srvl_debug_bits(info, name, offset, n);
    } else if (len <= 56) {
        /* Two chunks, use one output line */
        srvl_debug_bits(info, name, offset, n);
        srvl_debug_bits(info, NULL, offset + n, 32);
    } else {
        /* Three or more chunks, use one output line for each */
        count = SRVL_DIV_ROUND_UP(len, 32);
        for (i = 0; i < count; i++) {
            sprintf(buf, "%s_%u", name, count - i - 1);
            srvl_debug_bits(info, buf, offset, n);
            if (n <= 24)
                info->pr("\n");
            offset += n;
            n = 32;
        }
    }
}

static void srvl_debug_u48(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bytes(info, name, offset, 48);
}

static void srvl_debug_u32(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bits(info, name, offset, 32);
}

static void srvl_debug_u128(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bytes(info, name, offset, 128);
}

static void srvl_debug_u12(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bits(info, name, offset, 12);
}

static void srvl_debug_u16(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bits(info, name, offset, 16);
}

static void srvl_debug_u8(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bits(info, name, offset, 8);
}

static void srvl_debug_u6(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bits(info, name, offset, 6);
}

static void srvl_debug_u3(srvl_debug_info_t *info, const char *name, u32 offset)
{
    srvl_debug_bits(info, name, offset, 3);
}

static u32 srvl_act_bs_get(srvl_debug_info_t *info, u32 offs, u32 len)
{
    return vtss_bs_get(info->data.action, offs + info->data.action_offset, len);
}

static u32 srvl_entry_bs_get(srvl_debug_info_t *info, u32 offs, u32 len)
{
    return vtss_bs_get(info->data.entry, offs + info->data.key_offset, len);
}

static void srvl_debug_action(srvl_debug_info_t *info, 
                              const char *name, u32 offs, u32 offs_val, u32 len)
{
    vtss_debug_printf_t pr = info->pr;
    srvl_tcam_data_t    *data = &info->data;
    BOOL                enable, multi = 0;
    u32                 num = 0;
    int                 i, length = strlen(name);

    if (offs_val && (offs_val - offs) != 1) {
        /* 'Enable' field consists of multiple bits */
        multi = 1;
        num = srvl_act_bs_get(info, offs, offs_val - offs);
        enable = (num ? 1 : 0);
    } else
        enable = vtss_bs_bit_get(data->action, offs + data->action_offset);
    
    for (i = 0; i < length; i++)
        pr("%c", enable ? toupper(name[i]) : tolower(name[i]));

    if (len) {
        pr(":");
        if (multi)
            pr("%u/", num);
        pr("%u", srvl_act_bs_get(info, offs_val, len));
    }
    pr(" ");
}

static void srvl_debug_fld(srvl_debug_info_t *info, const char *name, u32 offs, u32 len)
{
    info->pr("%s:%u ", name, srvl_act_bs_get(info, offs, len));
}

/* ================================================================= *
 *  TCAM properties
 * ================================================================= */

/* TCAM entries */
enum vtss_tcam_sel {
    VTSS_TCAM_SEL_ENTRY   = VTSS_BIT(0),
    VTSS_TCAM_SEL_ACTION  = VTSS_BIT(1),
    VTSS_TCAM_SEL_COUNTER = VTSS_BIT(2),
    VTSS_TCAM_SEL_ALL     = VTSS_BITMASK(3),
};

enum vtss_tcam_cmd {
    VTSS_TCAM_CMD_WRITE      = 0, /* Copy from Cache to TCAM */
    VTSS_TCAM_CMD_READ       = 1, /* Copy from TCAM to Cache */
    VTSS_TCAM_CMD_MOVE_UP    = 2, /* Move <count> up */
    VTSS_TCAM_CMD_MOVE_DOWN  = 3, /* Move <count> down */
    VTSS_TCAM_CMD_INITIALIZE = 4, /* Write all (from cache) */
};

enum vtss_tcam_bank {
    VTSS_TCAM_IS0,
    VTSS_TCAM_IS1,
    VTSS_TCAM_IS2,
    VTSS_TCAM_ES0
};

typedef struct {
    u32 target;              /* Target offset */
    u16 tg_width;            /* Type-group width (in bits) */
    u16 sw_count;            /* Sub word count */
    u16 entry_count;         /* Entry count */
    u16 entry_words;         /* Number of entry words */
    u16 entry_width;         /* Entry width (in bits) */
    u16 action_count;        /* Action count */
    u16 action_words;        /* Number of action words */
    u16 action_width;        /* Action width (in bits) */
    u16 action_type_width;   /* Action type width (in bits) */
    u16 action_type_0_width; /* Action type 0 width (in bits) */
    u16 action_type_1_width; /* Action type 1 width (in bits) */
    u16 action_type_0_count; /* Action type 0 sub word count */
    u16 action_type_1_count; /* Action type 1 sub word count */
    u16 counter_words;       /* Number of counter words */
    u16 counter_width;       /* Counter width (in bits) */
} tcam_props_t;

#define BITS_TO_DWORD(x) (1+(((x)-1)/32))

/* Number of full entries */
#define SRVL_IS0_CNT 384
#define SRVL_IS1_CNT 256
#define SRVL_IS2_CNT 256
#define SRVL_ES0_CNT 1024

static const tcam_props_t tcam_info[] = {
    [VTSS_TCAM_IS0] = {
        .target = VTSS_TO_IS0,
        .tg_width = 2,
        .sw_count = 2,
        .entry_count = SRVL_IS0_CNT,
        .entry_words = BITS_TO_DWORD(152),      // 2*76 (MLBS); longest of MLBS and MLL
        .entry_width = 152,
        .action_count = SRVL_IS0_CNT + 2,
        .action_words = BITS_TO_DWORD(178),
        .action_width = 178,
        .action_type_width = 1,
        .action_type_0_width = 67,
        .action_type_1_width = 89,
        .action_type_0_count = 1,
        .action_type_1_count = 2,
        .counter_words = BITS_TO_DWORD(2),
        .counter_width = 1,
    },
    [VTSS_TCAM_IS1] = {
        .target = VTSS_TO_S1,
        .tg_width = 2,
        .sw_count = 4,
        .entry_count = SRVL_IS1_CNT,
        .entry_words = BITS_TO_DWORD(376),
        .entry_width = 376,
        .action_count = SRVL_IS1_CNT + 1,
        .action_words = BITS_TO_DWORD(320),
        .action_width = 320,
        .action_type_width = 0,
        .action_type_0_width = 80,
        .action_type_1_width = 0,
        .action_type_0_count = 4,
        .action_type_1_count = 0,
        .counter_words = BITS_TO_DWORD(4),
        .counter_width = 1,
    },
    [VTSS_TCAM_IS2] = {
        .target = VTSS_TO_S2,
        .tg_width = 2,
        .sw_count = 4,
        .entry_count = SRVL_IS2_CNT,
        .entry_words = BITS_TO_DWORD(376),
        .entry_width = 376,
        .action_count = SRVL_IS2_CNT + 13,
        .action_words = BITS_TO_DWORD(103),
        .action_width = 103,
        .action_type_width = 1,
        .action_type_0_width = 51,
        .action_type_1_width = 6,
        .action_type_0_count = 2,
        .action_type_1_count = 4,
        .counter_words = BITS_TO_DWORD(4*32),
        .counter_width = 32,
    },
    [VTSS_TCAM_ES0] = {
        .target = VTSS_TO_ES0,
        .tg_width = 1,
        .sw_count = 1,
        .entry_count = SRVL_ES0_CNT,
        .entry_words = BITS_TO_DWORD(29),
        .entry_width = 29,
        .action_count = SRVL_ES0_CNT + 11,
        .action_words = BITS_TO_DWORD(91),
        .action_width = 91,
        .action_type_width = 0,
        .action_type_0_width = 91,
        .action_type_1_width = 0,
        .action_type_0_count = 1,
        .action_type_1_count = 0,
        .counter_words = BITS_TO_DWORD(1),
        .counter_width = 1,
    }
};

/* ================================================================= *
 *  VCAP key processing
 * ================================================================= */

static void srvl_vcap_key_set(srvl_tcam_data_t *data, u32 offset, u32 width, 
                              u32 value, u32 mask)
{
    VTSS_N("offset: %u, key_offset: %u, width: %u, value: 0x%08x, mask: 0x%08x", 
           offset, data->key_offset, width, value, mask);

    if (width > 32) {
        VTSS_E("illegal width: %u, offset: %u", width, offset);
    } else { 
        vtss_bs_set(data->entry, offset + data->key_offset, width, value);
        vtss_bs_set(data->mask, offset + data->key_offset, width, mask);
    }
}

static void srvl_vcap_key_bit_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_bit_t fld)
{
    srvl_vcap_key_set(data, offset, 1, 
                      fld == VTSS_VCAP_BIT_1 ? 1 : 0, fld == VTSS_VCAP_BIT_ANY ? 0 : 1);  
}

static void srvl_vcap_key_bit_inv_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_bit_t fld)
{
    srvl_vcap_key_set(data, offset, 1, 
                      fld == VTSS_VCAP_BIT_0 ? 1 : 0, fld == VTSS_VCAP_BIT_ANY ? 0 : 1);  
}

static void srvl_vcap_key_ipv4_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_ip_t *fld)
{
    srvl_vcap_key_set(data, offset, 32, fld->value, fld->mask);  
}

static void srvl_vcap_key_u3_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_u8_t *fld)
{
    srvl_vcap_key_set(data, offset, 3, fld->value, fld->mask);  
}

static void srvl_vcap_key_u6_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_u8_t *fld)
{
    srvl_vcap_key_set(data, offset, 6, fld->value, fld->mask);  
}

static void srvl_vcap_key_u8_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_u8_t *fld)
{
    srvl_vcap_key_set(data, offset, 8, fld->value, fld->mask);  
}

static void srvl_vcap_key_bytes_set(srvl_tcam_data_t *data, u32 offset, 
                                    u8 *val, u8 *msk, u32 count)
{
    u32 i, j, n = 0, value = 0, mask = 0;
    
    /* Data wider than 32 bits are split up in chunks of maximum 32 bits.
       The 32 LSB of the data are written to the 32 MSB of the TCAM. */
    offset += (count * 8);
    for (i = 0; i < count; i++) {
        j = (count - i - 1);
        value += (val[j] << n);
        mask += (msk[j] << n);
        n += 8;
        if (n == 32 || (i + 1) == count) {
            offset -= n;
            srvl_vcap_key_set(data, offset, n, value, mask);  
            n = 0;
            value = 0;
            mask = 0;
        }
    }
}

static void srvl_vcap_key_u16_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_u16_t *fld)
{
    srvl_vcap_key_bytes_set(data, offset, fld->value, fld->mask, 2);
}

static void srvl_vcap_key_u40_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_u40_t *fld)
{
    srvl_vcap_key_bytes_set(data, offset, fld->value, fld->mask, 5);
}

static void srvl_vcap_key_u48_set(srvl_tcam_data_t *data, u32 offset, vtss_vcap_u48_t *fld)
{
    srvl_vcap_key_bytes_set(data, offset, fld->value, fld->mask, 6);
}

/* ================================================================= *
 *  VCAP action processing
 * ================================================================= */

static void srvl_vcap_action_set(srvl_tcam_data_t *data, u32 offset, u32 width, u32 value)
{
    VTSS_N("offset: %u, width: %u, value: 0x%08x", offset, width, value);
    
    if (width > 32) {
        VTSS_E("illegal width: %u, offset: %u", width, offset);
    } else { 
        vtss_bs_set(data->action, offset + data->action_offset, width, value);
    }
}

static void srvl_vcap_action_bit_set(srvl_tcam_data_t *data, u32 offset, u32 value)
{
    srvl_vcap_action_set(data, offset, 1, value ? 1 : 0);
}

/* ================================================================= *
 *  VCAP cache copy operations
 * ================================================================= */

static vtss_rc srvl_vcap_cmd(const tcam_props_t *tcam, u16 ix, int cmd, int sel)
{
    u32 tgt = tcam->target;
    u32 value = (VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_CMD(cmd) |
                 VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_ADDR(ix) |
                 VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_SHOT);

    if ((sel & VTSS_TCAM_SEL_ENTRY) && ix >= tcam->entry_count)
        return VTSS_RC_ERROR;

    if (!(sel & VTSS_TCAM_SEL_ENTRY))
        value |= VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_ENTRY_DIS;

    if (!(sel & VTSS_TCAM_SEL_ACTION))
        value |= VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_ACTION_DIS;

    if (!(sel & VTSS_TCAM_SEL_COUNTER))
        value |= VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_CNT_DIS;

    SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL(tgt), value);

    do {
        SRVL_RD(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL(tgt), &value);
    } while (value & VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_UPDATE_CTRL_UPDATE_SHOT);

    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_entry2cache(const tcam_props_t *tcam, srvl_tcam_data_t *data)
{
    u32 i, tgt = tcam->target;

    for (i = 0; i < tcam->entry_words; i++) {
        SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ENTRY_DAT(tgt, i), data->entry[i]);
        SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_MASK_DAT(tgt, i), ~data->mask[i]);
    }
    SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_TG_DAT(tgt), data->tg);
    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_cache2entry(const tcam_props_t *tcam, srvl_tcam_data_t *data)
{
    u32 i, m, tgt = tcam->target;

    for (i = 0; i < tcam->entry_words; i++) {
        SRVL_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ENTRY_DAT(tgt, i), &data->entry[i]);
        SRVL_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_MASK_DAT(tgt, i), &m);
        data->mask[i] = ~m;           /* Invert mask */
    }
    SRVL_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_TG_DAT(tgt), &data->tg);
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_action2cache(const tcam_props_t *tcam, srvl_tcam_data_t *data)
{
    u32 i, width, mask, tgt = tcam->target;
    
    /* Encode action type */
    width = tcam->action_type_width;
    if (width) {
        mask = VTSS_BITMASK(width);
        data->action[0] = ((data->action[0] & ~mask) | data->type);
    }

    for (i = 0; i < tcam->action_words; i++)
        SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ACTION_DAT(tgt, i), data->action[i]);
    for (i = 0; i < tcam->counter_words; i++)
        SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tgt, i), data->counter[i]);

    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_cache2action(const tcam_props_t *tcam, srvl_tcam_data_t *data)
{
    u32 i, width, tgt = tcam->target;

    for (i = 0; i < tcam->action_words; i++)
        SRVL_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ACTION_DAT(tgt, i), &data->action[i]);
    for (i = 0; i < tcam->counter_words; i++)
        SRVL_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tgt, i), &data->counter[i]);

    /* Extract action type */
    width = tcam->action_type_width;
    data->type = (width ? VTSS_EXTRACT_BITFIELD(data->action[0], 0, width) : 0);
        
    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_initialize(int bank)
{
    const tcam_props_t *tcam = &tcam_info[bank];
    srvl_tcam_data_t   data;
    u32                tgt = tcam->target;

    memset(&data, 0, sizeof(data));
    
    /* First write entries */
    VTSS_RC(srvl_vcap_entry2cache(tcam, &data));
    SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG(tgt),
            VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_SIZE(tcam->entry_count));
    VTSS_RC(srvl_vcap_cmd(tcam, 0, VTSS_TCAM_CMD_INITIALIZE, VTSS_TCAM_SEL_ENTRY));

    /* Then actions and counters */
    VTSS_RC(srvl_vcap_action2cache(tcam, &data));
    SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG(tgt),
            VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_SIZE(tcam->action_count));
    VTSS_RC(srvl_vcap_cmd(tcam, 0, VTSS_TCAM_CMD_INITIALIZE, 
                              VTSS_TCAM_SEL_ACTION | VTSS_TCAM_SEL_COUNTER));

    return VTSS_RC_OK;
}

/* Convert from 0-based row to VCAP entry row and run command */
static vtss_rc srvl_vcap_row_cmd(const tcam_props_t *tcam, u32 row, int cmd, int sel)
{
    return srvl_vcap_cmd(tcam, tcam->entry_count - row - 1, cmd, sel);
}

/* Convert from 0-based port to VCAP entry row and run command */
static vtss_rc srvl_vcap_port_cmd(const tcam_props_t *tcam, u32 port, int cmd, int sel)
{
    return srvl_vcap_cmd(tcam, tcam->entry_count + port, cmd, sel);
}

/* ================================================================= *
 *  VCAP get/set/add/del/move utilities
 * ================================================================= */

/* Calculate offsets for entry */
static vtss_rc srvl_vcap_data_get(const tcam_props_t *tcam, vtss_vcap_idx_t *idx, 
                                  srvl_tcam_data_t *data)
{
    u32 i, col, offset, count, cnt, base, width = tcam->tg_width;
    
    VTSS_NG(VTSS_TRACE_GROUP_EVC, "row: %u, col: %u, key_size: %s",
            idx->row, idx->col, vtss_vcap_key_size_txt(idx->key_size));

    /* Calculate type-group value/mask */
    switch (idx->key_size) {
    case VTSS_VCAP_KEY_SIZE_FULL:
        count = 1;
        data->tg_sw = VCAP_TG_FULL;
        break;
    case VTSS_VCAP_KEY_SIZE_HALF:
        count = 2;
        data->tg_sw = VCAP_TG_HALF;
        break;
    case VTSS_VCAP_KEY_SIZE_QUARTER:
        count = 4;
        data->tg_sw = VCAP_TG_QUARTER;
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal key size");
        return VTSS_RC_ERROR;
    }
    col = (count - idx->col - 1);
    cnt = (tcam->sw_count / count);
    base = (tcam->sw_count - idx->col * cnt - cnt);
    data->tg_value = 0;
    data->tg_mask = 0;
    for (i = 0; i < cnt; i++) {
        offset = ((base + i) * width);
        data->tg_value |= VTSS_ENCODE_BITFIELD(data->tg_sw, offset, width);
        data->tg_mask |= VTSS_ENCODE_BITMASK(offset, width);
    }
    
    /* Calculate key/action/counter offsets */
    data->key_offset = (base * tcam->entry_width)/tcam->sw_count;
    data->counter_offset = (cnt * col * tcam->counter_width);
    if (data->type) {
        width = tcam->action_type_1_width;
        cnt = tcam->action_type_1_count;
    } else {
        width = tcam->action_type_0_width;
        cnt = tcam->action_type_0_count;
    }
    data->action_offset = (((cnt * col * width) / count) + tcam->action_type_width);

    VTSS_NG(VTSS_TRACE_GROUP_EVC, "tg_value: 0x%08x, tg_mask: 0x%08x",
            data->tg_value, data->tg_mask);
    VTSS_NG(VTSS_TRACE_GROUP_EVC, "key_offset: %u, action_offset: %u, counter_offset: %u",
            data->key_offset, data->action_offset, data->counter_offset);
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_entry_get(const tcam_props_t *tcam, vtss_vcap_idx_t *idx, 
                                   srvl_tcam_data_t *data)
{
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "row: %u", idx->row);

    /* Read row */
    memset(data, 0, sizeof(*data));
    VTSS_RC(srvl_vcap_row_cmd(tcam, idx->row, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_ALL));
    VTSS_RC(srvl_vcap_cache2action(tcam, data));
    VTSS_RC(srvl_vcap_cache2entry(tcam, data));

    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_entry_data_get(const tcam_props_t *tcam, vtss_vcap_idx_t *idx, 
                                        srvl_tcam_data_t *data)
{
    /* Read row and calculate offsets */
    VTSS_RC(srvl_vcap_entry_get(tcam, idx, data));
    VTSS_RC(srvl_vcap_data_get(tcam, idx, data)); 

    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_entry_set(const tcam_props_t *tcam, vtss_vcap_idx_t *idx, 
                                   srvl_tcam_data_t *data)
{
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "row: %u", idx->row);

    /* Write row */
    VTSS_RC(srvl_vcap_entry2cache(tcam, data));
    VTSS_RC(srvl_vcap_action2cache(tcam, data));
    VTSS_RC(srvl_vcap_row_cmd(tcam, idx->row, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ALL));

    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_counter_get(int bank, vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    const tcam_props_t *tcam = &tcam_info[bank];
    srvl_tcam_data_t   tcam_data, *data = &tcam_data;
    
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "row: %u, col: %u, key_size: %s, clear: %u",
            idx->row, idx->col, vtss_vcap_key_size_txt(idx->key_size), clear);

    /* Read counter at row */
    VTSS_RC(srvl_vcap_row_cmd(tcam, idx->row, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_COUNTER));
    VTSS_RC(srvl_vcap_cache2action(tcam, data));
    VTSS_RC(srvl_vcap_data_get(tcam, idx, data));
    *counter = vtss_bs_get(data->counter, data->counter_offset, tcam->counter_width);

    if (clear) {
        /* Clear counter at row */
        vtss_bs_set(data->counter, data->counter_offset, tcam->counter_width, 0);
        VTSS_RC(srvl_vcap_action2cache(tcam, data));
        VTSS_RC(srvl_vcap_row_cmd(tcam, idx->row, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_COUNTER));
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_entry_add(const tcam_props_t *tcam, vtss_vcap_idx_t *idx, 
                                   srvl_tcam_data_t *data)
{
    u32 type = data->type, cnt = data->cnt;

    VTSS_IG(VTSS_TRACE_GROUP_EVC, "row: %u, col: %u, key_size: %s, counter: %u",
            idx->row, idx->col, vtss_vcap_key_size_txt(idx->key_size), cnt);

    /* Read row */
    VTSS_RC(srvl_vcap_entry_get(tcam, idx, data));
    data->type = type;

    /* Encode type-group and counter */
    VTSS_RC(srvl_vcap_data_get(tcam, idx, data));
    data->tg = ((data->tg & ~data->tg_mask) | data->tg_value);
    vtss_bs_set(data->counter, data->counter_offset,  tcam->counter_width, cnt);
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_entry_del(int bank, vtss_vcap_idx_t *idx)
{
    const tcam_props_t *tcam = &tcam_info[bank];
    srvl_tcam_data_t   tcam_data, *data = &tcam_data;

    VTSS_IG(VTSS_TRACE_GROUP_EVC, "row: %u, col: %u, key_size: %s",
            idx->row, idx->col, vtss_vcap_key_size_txt(idx->key_size));

    /* Read row */
    VTSS_RC(srvl_vcap_entry_get(tcam, idx, data));

    /* Encode type-group */
    VTSS_RC(srvl_vcap_data_get(tcam, idx, data));
    data->tg = (data->tg & ~data->tg_mask);
    
    /* Write row */
    return srvl_vcap_entry_set(tcam, idx, data);
}

static void srvl_vcap_copy(u32 *src, u32 *dst, u32 src_offs, u32 dst_offs, u32 copy_len)
{
    u32 i, count, len, value;
    
    count = (copy_len / 32);
    len = 32;
    for (i = 0; i <= count; i++, src_offs += 32, dst_offs += 32) {
        if (i == count && (len = (copy_len % 32)) == 0)
            break;

        value = vtss_bs_get(src, src_offs, len);
        vtss_bs_set(dst, dst_offs, len, value);
    }
}

static void srvl_vcap_data_copy(srvl_tcam_data_t *src, srvl_tcam_data_t *dst, 
                                u32 key_len, u32 action_len, u32 counter_len)
{
    VTSS_NG(VTSS_TRACE_GROUP_EVC,
            "src->key_offset: %u, key_len: %u, dst->key_offset: %u",
            src->key_offset, key_len, dst->key_offset);
    VTSS_NG(VTSS_TRACE_GROUP_EVC,
            "src->action_offset: %u, action_len: %u, dst->action_offset: %u",
            src->action_offset, action_len, dst->action_offset);
    VTSS_NG(VTSS_TRACE_GROUP_EVC,
            "src->counter_offset: %u, counter_len: %u, dst->counter_offset: %u",
            src->counter_offset, counter_len, dst->counter_offset);
    
    /* Copy key data */
    srvl_vcap_copy(src->entry, dst->entry, src->key_offset, dst->key_offset, key_len);
    srvl_vcap_copy(src->mask, dst->mask, src->key_offset, dst->key_offset, key_len);

    /* Copy action data */
    srvl_vcap_copy(src->action, dst->action, src->action_offset, dst->action_offset, action_len);

    /* Copy counter data */
    srvl_vcap_copy(src->counter, dst->counter, src->counter_offset, dst->counter_offset, 
                   counter_len);
}

static vtss_rc srvl_vcap_entry_move(int bank, vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    const tcam_props_t *tcam = &tcam_info[bank];
    srvl_tcam_data_t   data[2], *src = &data[0], *dst = &data[1], *tmp;
    vtss_vcap_idx_t    cur_idx;
    u32                col_count, rule_count = 0, tgw, key_len, action_len, counter_len, col;

    VTSS_IG(VTSS_TRACE_GROUP_EVC, "row: %u, col: %u, key_size: %s, count: %u, up: %u",
            idx->row, idx->col, vtss_vcap_key_size_txt(idx->key_size), count, up);

    /* Check key size and count */
    if (idx->key_size > VTSS_VCAP_KEY_SIZE_QUARTER || count == 0) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal key_size: %s or count: %u",
                vtss_vcap_key_size_txt(idx->key_size), count);
        return VTSS_RC_ERROR;
    }

    /* Move FULL entries with hardware support. 
       MOVE_DOWN corresponds to 'up' seen from VCAP library */
    if (idx->key_size == VTSS_VCAP_KEY_SIZE_FULL) {
        SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG(tcam->target),
                VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_NUM_POS(1) |
                VTSS_F_VCAP_CORE_VCAP_CORE_CFG_VCAP_MV_CFG_MV_SIZE(count - 1));
        return srvl_vcap_row_cmd(tcam, 
                                 idx->row + count - 1, 
                                 up ? VTSS_TCAM_CMD_MOVE_DOWN : VTSS_TCAM_CMD_MOVE_UP,
                                 VTSS_TCAM_SEL_ALL);
    }
    
    /* Move HALF/QUARTER rules manually */
    cur_idx = *idx;
    idx = &cur_idx;
    col_count = vtss_vcap_key_rule_count(idx->key_size);
    key_len = (tcam->entry_width / col_count);
    action_len = (tcam->action_width / col_count);
    tgw = ((tcam->tg_width * tcam->sw_count) / col_count);
    counter_len = tcam->counter_width;

    VTSS_DG(VTSS_TRACE_GROUP_EVC,
            "col_count: %u, key_len: %u, action_len: %u, tgw: %u, counter_len: %u",
            col_count, key_len, action_len, tgw, counter_len);

    if (up) {
        /* Move HALF/QUARTER rules up */

        /* Read source row */
        VTSS_RC(srvl_vcap_entry_get(tcam, idx, src));

        while (1) {
            if (idx->col == 0) {
                /* Destination is previous row */
                VTSS_DG(VTSS_TRACE_GROUP_EVC, "previous, row: %u, col: %u", idx->row, idx->col);
                idx->row--;
                VTSS_RC(srvl_vcap_entry_get(tcam, idx, dst));
            } else {
                /* Move rules in current row up */
                col = MIN(col_count - idx->col, count - rule_count);
                VTSS_DG(VTSS_TRACE_GROUP_EVC, "current, row: %u, col: %u, cnt: %u", 
                        idx->row, idx->col, col);
                idx->col += (col - 1);
                VTSS_RC(srvl_vcap_data_get(tcam, idx, src)); 
                *dst = *src;
                idx->col--;
                VTSS_RC(srvl_vcap_data_get(tcam, idx, dst)); 
                srvl_vcap_data_copy(src, dst, col*key_len, col*action_len, col*counter_len);
                VTSS_DG(VTSS_TRACE_GROUP_EVC, "tg_pre: 0x%08x", dst->tg);
                dst->tg |= (dst->tg << tgw);
                dst->tg &= ~src->tg_mask;
                VTSS_DG(VTSS_TRACE_GROUP_EVC, "tg_post: 0x%08x", dst->tg);
                rule_count += col;
                if (rule_count >= count)
                    return srvl_vcap_entry_set(tcam, idx, dst);
            }

            /* Copy first rule on next row to last rule in destination row */
            idx->row++;
            idx->col = 0;
            VTSS_RC(srvl_vcap_entry_data_get(tcam, idx, src));
            idx->row--;
            idx->col = (col_count - 1);
            VTSS_RC(srvl_vcap_data_get(tcam, idx, dst)); 
            srvl_vcap_data_copy(src, dst, key_len, action_len, counter_len);
            VTSS_DG(VTSS_TRACE_GROUP_EVC, "tg_pre: 0x%08x", dst->tg);
            dst->tg = ((dst->tg & ~dst->tg_mask) | dst->tg_value);
            VTSS_DG(VTSS_TRACE_GROUP_EVC, "tg_post: 0x%08x", dst->tg);
            VTSS_RC(srvl_vcap_entry_set(tcam, idx, dst));
            
            /* Step forward one row */
            idx->row++;
            idx->col = 1;
            rule_count++;
            if (rule_count >= count) {
                /* Delete last row */
                src->tg = VCAP_TG_NONE;
                return srvl_vcap_entry_set(tcam, idx, src);
            }
        }
    }

    /* Move HALF/QUARTER rules down */
    
    /* Read destination data at the end of the rule block */
    idx->row += (count / col_count);
    idx->col += (count % col_count);
    if (idx->col >= col_count) {
        idx->row++;
        idx->col -= col_count;
    }
    VTSS_RC(srvl_vcap_entry_get(tcam, idx, dst));
    if (bank == VTSS_TCAM_IS0) {
        /* MLBS entry, action type 1 */
        dst->type = 1;
    }

    for (rule_count = 0; rule_count < count; rule_count++) {
        if (idx->col != 0) {
            /* Move rules in current row down */
            col = MIN(idx->col, count - rule_count);
            VTSS_DG(VTSS_TRACE_GROUP_EVC,
                    "current, row: %u, col: %u, cnt: %u, count: %u, rule_count: %u",
                    idx->row, idx->col, col, count, rule_count);
            VTSS_RC(srvl_vcap_data_get(tcam, idx, dst)); 
            *src = *dst;
            idx->col--;
            VTSS_RC(srvl_vcap_data_get(tcam, idx, src)); 
            srvl_vcap_data_copy(src, dst, col*key_len, col*action_len, col*counter_len);
            VTSS_DG(VTSS_TRACE_GROUP_EVC, "tg_pre: 0x%08x", dst->tg);
            dst->tg |= (dst->tg >> tgw);
            VTSS_DG(VTSS_TRACE_GROUP_EVC, "tg_post: 0x%08x", dst->tg);
            rule_count += col;
            if (rule_count >= count)
                return srvl_vcap_entry_set(tcam, idx, dst);
        }

        /* Copy last rule from previous row to first rule in destination row */
        VTSS_DG(VTSS_TRACE_GROUP_EVC, "previous, row: %u, col: %u", idx->row, idx->col);
        idx->col = 0;
        VTSS_RC(srvl_vcap_data_get(tcam, idx, dst));
        idx->row--;
        idx->col = (col_count - 1);
        VTSS_RC(srvl_vcap_entry_data_get(tcam, idx, src));
        srvl_vcap_data_copy(src, dst, key_len, action_len, counter_len);
        dst->tg = ((dst->tg & ~dst->tg_mask) | dst->tg_value);
        idx->row++;
        VTSS_RC(srvl_vcap_entry_set(tcam, idx, dst));
        
        /* Swap source and destination pointers and step back one row */
        tmp = dst;
        dst = src;
        src = tmp;
        idx->row--;
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_port_get(int bank, u32 port, u32 *counter, BOOL clear) 
{
    const tcam_props_t *tcam = &tcam_info[bank];
    u32                tgt = tcam->target;

    /* Read counter at index */
    VTSS_RC(srvl_vcap_port_cmd(tcam, port, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_COUNTER));
    SRVL_RD(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tgt, 0), counter);

    if (clear) {
        /* Clear counter at index */
        SRVL_WR(VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tgt, 0), 0);
        VTSS_RC(srvl_vcap_port_cmd(tcam, port, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_COUNTER));
    }
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  VCAP debug print utilities
 * ================================================================= */

#define SRVL_DEBUG_VCAP(pr, name, tgt) srvl_debug_reg(pr, VTSS_VCAP_CORE_VCAP_CONST_##name(tgt), #name)

static vtss_rc srvl_debug_tcam(const vtss_debug_printf_t pr,
                               const tcam_props_t *tcam, BOOL entry)
{
    u32 i, tgt = tcam->target;

    srvl_debug_reg_header(pr, "VCAP");

    /* Entry/mask */
    if (entry) {
        srvl_debug_reg(pr, VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_TG_DAT(tgt), "TG_DAT");
        for (i = 0; i < tcam->entry_words; i++) {
            srvl_debug_reg_inst(pr, VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ENTRY_DAT(tgt, i),
                                i, "ENTRY_DAT");
            srvl_debug_reg_inst(pr, VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_MASK_DAT(tgt, i),
                                i, "MASK_DAT");
        }
        pr("\n");
    }

    /* Action/counter */
    for (i = 0; i < tcam->action_words; i++)
        srvl_debug_reg_inst(pr, VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_ACTION_DAT(tgt, i),
                            i, "ACTION_DAT");
    for (i = 0; i < tcam->counter_words; i++)
        srvl_debug_reg_inst(pr, VTSS_VCAP_CORE_VCAP_CORE_CACHE_VCAP_CNT_DAT(tgt, i),
                            i, "CNT_DAT");
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_vcap(int bank,
                               const char *name,
                               const vtss_debug_printf_t pr,
                               const vtss_debug_info_t *const debug_info,
                               vtss_rc (* dbg)(srvl_debug_info_t *dbg_info))
{
    /*lint --e{454, 455, 456} */ // Due to VTSS_EXIT_ENTER
    const tcam_props_t *tcam = &tcam_info[bank];
    srvl_debug_info_t  info;
    srvl_tcam_data_t   *data = &info.data;
    int                i, j, k, sel, rule_index = 0;
    u32                tgt = tcam->target, action_width;
    const char         *txt;
    
    info.pr = pr;
    srvl_debug_print_header(pr, name);
    
    pr("tg_width           : %u\n", tcam->tg_width);
    pr("sw_count           : %u\n", tcam->sw_count);
    pr("entry_count        : %u\n", tcam->entry_count);
    pr("entry_words        : %u\n", tcam->entry_words);
    pr("entry_width        : %u\n", tcam->entry_width);
    pr("action_count       : %u = %u + %u\n", tcam->action_count, tcam->entry_count, 
       tcam->action_count - tcam->entry_count);
    pr("action_words       : %u\n", tcam->action_words);
    pr("action_width       : %u\n", tcam->action_width);
    pr("action_type_width  : %u\n", tcam->action_type_width);
    pr("action_type_0_width: %u\n", tcam->action_type_0_width);
    pr("action_type_1_width: %u\n", tcam->action_type_1_width);
    pr("action_type_0_count: %u\n", tcam->action_type_0_count);
    pr("action_type_1_count: %u\n", tcam->action_type_1_count);
    pr("counter_words      : %u\n", tcam->counter_words);
    pr("counter_width      : %u\n", tcam->counter_width);
    pr("\n");
    
    srvl_debug_reg_header(pr, "VCAP_CONST");
    SRVL_DEBUG_VCAP(pr, ENTRY_WIDTH, tgt);
    SRVL_DEBUG_VCAP(pr, ENTRY_CNT, tgt);
    SRVL_DEBUG_VCAP(pr, ENTRY_SWCNT, tgt);
    SRVL_DEBUG_VCAP(pr, ENTRY_TG_WIDTH, tgt);
    SRVL_DEBUG_VCAP(pr, ACTION_DEF_CNT, tgt);
    SRVL_DEBUG_VCAP(pr, ACTION_WIDTH, tgt);
    SRVL_DEBUG_VCAP(pr, CNT_WIDTH, tgt);
    pr("\n");
    
    for (i = (tcam->action_count - 1); i >= 0; i--) {
        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();

        /* Read action and counter */
        sel = (VTSS_TCAM_SEL_ACTION | VTSS_TCAM_SEL_COUNTER);
        if (srvl_vcap_cmd(tcam, i, VTSS_TCAM_CMD_READ, sel) != VTSS_RC_OK ||
            srvl_vcap_cache2action(tcam, data) != VTSS_RC_OK)
            continue;
        
        /* Decode action type */
        data->action_offset = tcam->action_type_width;
        data->type = (data->action_offset ? 
                      VTSS_EXTRACT_BITFIELD(data->action[0], 0, data->action_offset) : 0);
        
        if (i >= tcam->entry_count) {
            /* Raw TCAM action/counter dump */
            if (debug_info->full)
                VTSS_RC(srvl_debug_tcam(pr, tcam, 0));

            /* Print default action */
            info.is_action = 1;
            data->cnt = vtss_bs_get(data->counter, 0, tcam->counter_width);
            pr("%d (port %d): ", i, i - tcam->entry_count);
            VTSS_RC(dbg(&info));
            pr("\n-------------------------------------------\n");
            continue;
        }
        
        /* Read entry */
        if (srvl_vcap_cmd(tcam, i, VTSS_TCAM_CMD_READ, VTSS_TCAM_SEL_ENTRY) != VTSS_RC_OK ||
            srvl_vcap_cache2entry(tcam, data) != VTSS_RC_OK ||
            data->tg == VCAP_TG_NONE)
            continue;

        /* Raw TCAM entry/mask dump */
        if (debug_info->full)
            VTSS_RC(srvl_debug_tcam(pr, tcam, 1));

        for (j = (tcam->sw_count - 1); j >= 0; j--) {
            data->tg_sw = VTSS_EXTRACT_BITFIELD(data->tg, j*tcam->tg_width, tcam->tg_width);
            k = j;
            switch (data->tg_sw) {
            case VCAP_TG_FULL:
                txt = (j ? NULL : "FULL");
                break;
            case VCAP_TG_HALF:
                txt = (j == 0 || j == (tcam->sw_count/2) ? "HALF" : NULL);
                if (j)
                    k = ((data->type ? tcam->action_type_1_count : tcam->action_type_0_count)/2);
                break;
            case VCAP_TG_QUARTER:
                txt = "QUARTER";
                break;
            default:
                txt = NULL;
                break;
            }
            if (txt == NULL)
                continue;

            /* Print entry */
            info.is_action = 0;
            data->key_offset = (j*tcam->entry_width/tcam->sw_count);
            pr("%s ", txt);
            VTSS_RC(dbg(&info));
            pr("\n");
            
            /* Print action */
            info.is_action = 1;
            data->cnt = vtss_bs_get(data->counter, j*tcam->counter_width, tcam->counter_width);
            action_width = (data->type ? tcam->action_type_1_width : tcam->action_type_0_width);
            data->action_offset = (tcam->action_type_width + k*action_width);
            pr("%d-%d (rule %d): ", i, j, rule_index);
            VTSS_RC(dbg(&info));
            pr("\n-------------------------------------------\n");
            rule_index++;
        }
    }
    pr("\n");
    return VTSS_RC_OK;
}

#if defined (VTSS_FEATURE_IS0)
/* ================================================================= *
 *  IS0 functions
 * ================================================================= */

static vtss_rc srvl_is0_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear)
{
    return srvl_vcap_counter_get(VTSS_TCAM_IS0, idx, counter, clear);
}

static vtss_rc srvl_is0_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *vcap_data, u32 counter)
{
    const tcam_props_t     *tcam   = &tcam_info[VTSS_TCAM_IS0];
    srvl_tcam_data_t       tcam_data;
    srvl_tcam_data_t       *data   = &tcam_data;
    vtss_is0_data_t        *is0    = &vcap_data->u.is0;
    vtss_is0_entry_t       *entry  = is0->entry;
    vtss_is0_key_t         *key    = &entry->key;
    vtss_is0_action_t      *action = &entry->action;
    vtss_vcap_key_size_t   key_size;

    /* Check key size and type */
    switch (entry->type) {
    case VTSS_IS0_TYPE_MLL:
        data->type = 0;
        key_size = VTSS_VCAP_KEY_SIZE_FULL;
        break;
    case VTSS_IS0_TYPE_MLBS:
        data->type = 1;
        key_size = VTSS_VCAP_KEY_SIZE_HALF;
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_MPLS, "illegal key type 0x%x", entry->type);
        return VTSS_RC_ERROR;
    }

    if (idx->key_size != key_size) {
        VTSS_EG(VTSS_TRACE_GROUP_MPLS, "illegal key_size: %s, expected: %s",
                vtss_vcap_key_size_txt(idx->key_size), vtss_vcap_key_size_txt(key_size));
        return VTSS_RC_ERROR;
    }

    /* Get TCAM data */
    data->cnt = counter;
    VTSS_RC(srvl_vcap_entry_add(tcam, idx, data));

    switch (entry->type) {
    case VTSS_IS0_TYPE_MLL:
        {
            vtss_vcap_u48_t mac = { .mask = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
            u32             mask;
            // Key
            srvl_vcap_key_set(data, IS0_FKO_MLL_IGR_PORT, IS0_FKL_MLL_IGR_PORT, VTSS_CHIP_PORT(key->mll.ingress_port), VTSS_BITMASK(IS0_FKL_MLL_IGR_PORT));
            srvl_vcap_key_set(data, IS0_FKO_MLL_TTYPE, IS0_FKL_MLL_TTYPE,       key->mll.tag_type,     VTSS_BITMASK(IS0_FKL_MLL_TTYPE));
            srvl_vcap_key_set(data, IS0_FKO_MLL_BVID, IS0_FKL_MLL_BVID,         key->mll.b_vid,        VTSS_BITMASK(IS0_FKL_MLL_BVID));
            srvl_vcap_key_set(data, IS0_FKO_MLL_E_TYPE, IS0_FKL_MLL_E_TYPE,     key->mll.ether_type,   VTSS_BITMASK(IS0_FKL_MLL_E_TYPE));
            memcpy(mac.value, &key->mll.dmac, 6);
            srvl_vcap_key_u48_set(data, IS0_FKO_MLL_M_DMAC, &mac);
            memcpy(mac.value, &key->mll.smac, 6);
            srvl_vcap_key_u48_set(data, IS0_FKO_MLL_M_SMAC, &mac);

            // Action
            mask = srvl_port_mask(action->mll.b_portlist.physical) | (action->mll.b_portlist.cpu ? (1<<11) : 0);

            srvl_vcap_action_set    (data, IS0_AO_MLL_LL_IDX,          IS0_AL_MLL_LL_IDX,          action->mll.linklayer_index);
            srvl_vcap_action_bit_set(data, IS0_AO_MLL_MPLS_FWD,                                    action->mll.mpls_forwarding);
            srvl_vcap_action_set    (data, IS0_AO_MLL_B_PM,            IS0_AL_MLL_B_PM,            mask);
            srvl_vcap_action_set    (data, IS0_AO_MLL_CPU_Q,           IS0_AL_MLL_CPU_Q,           action->mll.cpu_queue);
            srvl_vcap_action_set    (data, IS0_AO_MLL_ISDX,            IS0_AL_MLL_ISDX,            action->mll.isdx);
            srvl_vcap_action_set    (data, IS0_AO_MLL_VPROFILE_IDX,    IS0_AL_MLL_VPROFILE_IDX,    action->mll.vprofile_index);
            srvl_vcap_action_bit_set(data, IS0_AO_MLL_SCONFIG_ENA,                                 action->mll.use_service_config);
            srvl_vcap_action_set    (data, IS0_AO_MLL_CL_VID,          IS0_AL_MLL_CL_VID,          action->mll.classified_vid);
            srvl_vcap_action_set    (data, IS0_AO_MLL_QOS_DEFAULT_VAL, IS0_AL_MLL_QOS_DEFAULT_VAL, action->mll.qos);
            srvl_vcap_action_set    (data, IS0_AO_MLL_DP_DEFAULT_VAL,  IS0_AL_MLL_DP_DEFAULT_VAL,  action->mll.dp);
            srvl_vcap_action_set    (data, IS0_AO_MLL_OAM_ISDX,        IS0_AL_MLL_OAM_ISDX,        action->mll.oam_isdx);
            srvl_vcap_action_bit_set(data, IS0_AO_MLL_ISDX_ADD_REPL,                               action->mll.oam_isdx_add_replace);
        }
        break;
    case VTSS_IS0_TYPE_MLBS:
        {
            u32 mask;
#define SET_KEY_FIELD(offset, length, value, dontcare) \
    if ((value) == dontcare) { \
        srvl_vcap_key_set(data, offset, length, 0, 0); \
    } else { \
        srvl_vcap_key_set(data, offset, length, value, VTSS_BITMASK(length)); \
    }

            // Key
            srvl_vcap_key_set(data, IS0_HKO_MLBS_LL_IDX, IS0_HKL_MLBS_LL_IDX, key->mlbs.linklayer_index, VTSS_BITMASK(IS0_HKL_MLBS_LL_IDX));
            SET_KEY_FIELD(IS0_HKO_MLBS_LBL_0_VAL, IS0_HKL_MLBS_LBL_0_VAL, key->mlbs.label_stack[0].value, VTSS_MPLS_LABEL_VALUE_DONTCARE)
            SET_KEY_FIELD(IS0_HKO_MLBS_LBL_1_VAL, IS0_HKL_MLBS_LBL_1_VAL, key->mlbs.label_stack[1].value, VTSS_MPLS_LABEL_VALUE_DONTCARE)
            SET_KEY_FIELD(IS0_HKO_MLBS_LBL_2_VAL, IS0_HKL_MLBS_LBL_2_VAL, key->mlbs.label_stack[2].value, VTSS_MPLS_LABEL_VALUE_DONTCARE)
            SET_KEY_FIELD(IS0_HKO_MLBS_LBL_0_TC,  IS0_HKL_MLBS_LBL_0_TC,  key->mlbs.label_stack[0].tc,    VTSS_MPLS_TC_VALUE_DONTCARE)
            SET_KEY_FIELD(IS0_HKO_MLBS_LBL_1_TC,  IS0_HKL_MLBS_LBL_1_TC,  key->mlbs.label_stack[1].tc,    VTSS_MPLS_TC_VALUE_DONTCARE)
            SET_KEY_FIELD(IS0_HKO_MLBS_LBL_2_TC,  IS0_HKL_MLBS_LBL_2_TC,  key->mlbs.label_stack[2].tc,    VTSS_MPLS_TC_VALUE_DONTCARE)
#undef SET_KEY_FIELD

            // Action
            mask = srvl_port_mask(action->mlbs.b_portlist.physical) | (action->mlbs.b_portlist.cpu ? (1<<11) : 0);

            srvl_vcap_action_set    (data, IS0_AO_MLBS_ISDX,         IS0_AL_MLBS_ISDX,         action->mlbs.isdx);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_B_PM,         IS0_AL_MLBS_B_PM,         mask);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_CPU_Q,        IS0_AL_MLBS_CPU_Q,        action->mlbs.cpu_queue);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_OAM_ENA,      IS0_AL_MLBS_OAM_ENA,      action->mlbs.oam);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_BURIED_MIP,                             action->mlbs.oam_buried_mip);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_RSVD_LBL_VAL, IS0_AL_MLBS_RSVD_LBL_VAL, action->mlbs.oam_reserved_label_value);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_RSVD_LBL_BOS,                           action->mlbs.oam_reserved_label_bottom_of_stack);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_CW_ENA,                                 action->mlbs.cw_enable);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_TC_LABEL,     IS0_AL_MLBS_TC_LABEL,     action->mlbs.tc_label_index);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_TTL_LABEL,    IS0_AL_MLBS_TTL_LABEL,    action->mlbs.ttl_label_index);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_SWAP_LABEL,   IS0_AL_MLBS_SWAP_LABEL,   action->mlbs.swap_label_index);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_TERMINATE_PW,                           action->mlbs.terminate_pw);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_POP_CNT,      IS0_AL_MLBS_POP_CNT,      action->mlbs.pop_count);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_E_LSP,                                  action->mlbs.e_lsp);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_TC_MAP,       IS0_AL_MLBS_TC_MAP,       action->mlbs.tc_maptable_index);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_L_LSP_COS,    IS0_AL_MLBS_L_LSP_COS,    action->mlbs.l_lsp_qos_class);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_INC_ISDX,                               action->mlbs.add_tc_to_isdx);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_OAM_ISDX,     IS0_AL_MLBS_OAM_ISDX,     action->mlbs.oam_isdx);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_ISDX_ADD_REPL,                          action->mlbs.oam_isdx_add_replace);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_SWAP_IS_BOS,                            action->mlbs.swap_is_bottom_of_stack);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_VPROFILE_IDX, IS0_AL_MLBS_VPROFILE_IDX, action->mlbs.vprofile_index);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_SCONFIG_ENA,                            action->mlbs.use_service_config);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_CL_VID,       IS0_AL_MLBS_CL_VID,       action->mlbs.classified_vid);
            srvl_vcap_action_set    (data, IS0_AO_MLBS_VLAN_PCP,     IS0_AL_MLBS_VLAN_PCP,     action->mlbs.pcp);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_VLAN_STAGD,                             action->mlbs.s_tag);
            srvl_vcap_action_bit_set(data, IS0_AO_MLBS_VLAN_DEI,                               action->mlbs.dei);
        }
        break;
    }

    /* Set TCAM data */
    return srvl_vcap_entry_set(tcam, idx, data);
}

static vtss_rc srvl_is0_entry_del(vtss_vcap_idx_t *idx)
{
    return srvl_vcap_entry_del(VTSS_TCAM_IS0, idx);
}

static vtss_rc srvl_is0_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    return srvl_vcap_entry_move(VTSS_TCAM_IS0, idx, count, up);
}

#if defined (VTSS_FEATURE_MPLS)
static vtss_rc srvl_debug_is0(srvl_debug_info_t *info)
{
    vtss_debug_printf_t pr    = info->pr;
    srvl_tcam_data_t    *data = &info->data;

    if (data->tg_sw == VCAP_TG_FULL) {
        if (!info->is_action) {
            /* Print key */
            srvl_debug_bits(info, "MLL Key: Port",      IS0_FKO_MLL_IGR_PORT,       IS0_FKL_MLL_IGR_PORT);
            srvl_debug_bits(info, "TagType",            IS0_FKO_MLL_TTYPE,          IS0_FKL_MLL_TTYPE);
            srvl_debug_bits(info, "B_VID",              IS0_FKO_MLL_BVID,           IS0_FKL_MLL_BVID);
            srvl_debug_bits(info, "EType",              IS0_FKO_MLL_E_TYPE,         IS0_FKL_MLL_E_TYPE);
            pr("\n");
            srvl_debug_u48 (info, "DMAC",               IS0_FKO_MLL_M_DMAC);
            srvl_debug_u48 (info, "SMAC",               IS0_FKO_MLL_M_SMAC);
        }
        else {
            /* Print action */
            srvl_debug_bits(info, "MLL Action: LL_idx", IS0_AO_MLL_LL_IDX,          IS0_AL_MLL_LL_IDX);
            srvl_debug_bit (info, "MPLS_fwd?",          IS0_AO_MLL_MPLS_FWD);
            srvl_debug_bits(info, "B_portmask",         IS0_AO_MLL_B_PM,            IS0_AL_MLL_B_PM);
            srvl_debug_bits(info, "CPU_q",              IS0_AO_MLL_CPU_Q,           IS0_AL_MLL_CPU_Q);
            srvl_debug_bits(info, "ISDX",               IS0_AO_MLL_ISDX,            IS0_AL_MLL_ISDX);
            pr("\n");
            srvl_debug_bits(info, "VProfile_idx",       IS0_AO_MLL_VPROFILE_IDX,    IS0_AL_MLL_VPROFILE_IDX);
            srvl_debug_bit (info, "Service_cfg?",       IS0_AO_MLL_SCONFIG_ENA);
            srvl_debug_bits(info, "Clas._VID",          IS0_AO_MLL_CL_VID,          IS0_AL_MLL_CL_VID);
            srvl_debug_bits(info, "Def._QoS",           IS0_AO_MLL_QOS_DEFAULT_VAL, IS0_AL_MLL_QOS_DEFAULT_VAL);
            pr("\n");
            srvl_debug_bits(info, "Def._DP",            IS0_AO_MLL_DP_DEFAULT_VAL,  IS0_AL_MLL_DP_DEFAULT_VAL);
            srvl_debug_bits(info, "OAM_ISDX",           IS0_AO_MLL_OAM_ISDX,        IS0_AL_MLL_OAM_ISDX);
            srvl_debug_bit (info, "OAM_ISDX_add/rep?",  IS0_AO_MLL_ISDX_ADD_REPL);
            pr("hit:%u ", info->data.cnt);
        }
    }
    else if (data->tg_sw == VCAP_TG_HALF) {
        if (!info->is_action) {
            /* Print key */
            srvl_debug_bits(info, "MLBS Key: LL_idx",  IS0_HKO_MLBS_LL_IDX,      IS0_HKL_MLBS_LL_IDX);
            srvl_debug_bits(info, "L0",                IS0_HKO_MLBS_LBL_0_VAL,   IS0_HKL_MLBS_LBL_0_VAL);
            srvl_debug_bits(info, "TC0",               IS0_HKO_MLBS_LBL_0_TC,    IS0_HKL_MLBS_LBL_0_TC);
            pr("\n");
            srvl_debug_bits(info, "L1",                IS0_HKO_MLBS_LBL_1_VAL,   IS0_HKL_MLBS_LBL_1_VAL);
            srvl_debug_bits(info, "TC1",               IS0_HKO_MLBS_LBL_1_TC,    IS0_HKL_MLBS_LBL_1_TC);
            srvl_debug_bits(info, "L2",                IS0_HKO_MLBS_LBL_2_VAL,   IS0_HKL_MLBS_LBL_2_VAL);
            srvl_debug_bits(info, "TC2",               IS0_HKO_MLBS_LBL_2_TC,    IS0_HKL_MLBS_LBL_2_TC);
        }
        else {
            /* Print action */
            srvl_debug_bits(info, "MLBS Action: ISDX", IS0_AO_MLBS_ISDX,         IS0_AL_MLBS_ISDX);
            srvl_debug_bits(info, "B_portmask",        IS0_AO_MLBS_B_PM,         IS0_AL_MLBS_B_PM);
            srvl_debug_bits(info, "CPU_q",             IS0_AO_MLBS_CPU_Q,        IS0_AL_MLBS_CPU_Q);
            srvl_debug_bits(info, "OAM?",              IS0_AO_MLBS_OAM_ENA,      IS0_AL_MLBS_OAM_ENA);
            srvl_debug_bit (info, "Buried_MIP?",       IS0_AO_MLBS_BURIED_MIP);
            pr("\n");
            srvl_debug_bits(info, "Res_lbl",           IS0_AO_MLBS_RSVD_LBL_VAL, IS0_AL_MLBS_RSVD_LBL_VAL);
            srvl_debug_bit (info, "Res_lbl_BOS?",      IS0_AO_MLBS_RSVD_LBL_BOS);
            srvl_debug_bit (info, "CW?",               IS0_AO_MLBS_CW_ENA);
            pr("\n");
            srvl_debug_bits(info, "TC_lbl_idx",        IS0_AO_MLBS_TC_LABEL,     IS0_AL_MLBS_TC_LABEL);
            srvl_debug_bits(info, "TTL_lbl_idx",       IS0_AO_MLBS_TTL_LABEL,    IS0_AL_MLBS_TTL_LABEL);
            srvl_debug_bits(info, "Swap_lbl_idx",      IS0_AO_MLBS_SWAP_LABEL,   IS0_AL_MLBS_SWAP_LABEL);
            srvl_debug_bit (info, "Swap_is_BOS?",      IS0_AO_MLBS_SWAP_IS_BOS);
            pr("\n");
            srvl_debug_bit (info, "Term_PW?",          IS0_AO_MLBS_TERMINATE_PW);
            srvl_debug_bits(info, "Pop_cnt",           IS0_AO_MLBS_POP_CNT,      IS0_AL_MLBS_POP_CNT);
            srvl_debug_bit (info, "E_LSP?",            IS0_AO_MLBS_E_LSP);
            srvl_debug_bits(info, "TC_map_idx",        IS0_AO_MLBS_TC_MAP,       IS0_AL_MLBS_TC_MAP);
            srvl_debug_bits(info, "L_LSP_CoS",         IS0_AO_MLBS_L_LSP_COS,    IS0_AL_MLBS_L_LSP_COS);
            srvl_debug_bit (info, "Inc_ISDX_w_TC?",    IS0_AO_MLBS_INC_ISDX);
            pr("\n");
            srvl_debug_bits(info, "OAM_ISDX",          IS0_AO_MLBS_OAM_ISDX,     IS0_AL_MLBS_OAM_ISDX);
            srvl_debug_bit (info, "OAM_ISDX_add/rep?", IS0_AO_MLBS_ISDX_ADD_REPL);
            srvl_debug_bits(info, "VProfile_idx",      IS0_AO_MLBS_VPROFILE_IDX, IS0_AL_MLBS_VPROFILE_IDX);
            srvl_debug_bit (info, "Service_cfg?",      IS0_AO_MLBS_SCONFIG_ENA);
            pr("\n");
            srvl_debug_bits(info, "Clas._VID",         IS0_AO_MLBS_CL_VID,       IS0_AL_MLBS_CL_VID);
            srvl_debug_bit (info, "VLAN_S_tag?",       IS0_AO_MLBS_VLAN_STAGD);
            srvl_debug_bits(info, "VLAN_PCP",          IS0_AO_MLBS_VLAN_PCP,     IS0_AL_MLBS_VLAN_PCP);
            srvl_debug_bit (info, "VLAN_DEI?",         IS0_AO_MLBS_VLAN_DEI);
            pr("hit:%u ", info->data.cnt);
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_is0_all(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    return srvl_debug_vcap(VTSS_TCAM_IS0, "IS0", pr, info, srvl_debug_is0);
}
#endif

#endif  /* VTSS_FEATURE_IS0 */

/* ================================================================= *
 *  IS1 functions
 * ================================================================= */

static vtss_rc srvl_is1_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "enter");
    
    return srvl_vcap_counter_get(VTSS_TCAM_IS1, idx, counter, clear);
}

/* IS1 key information common for some key types */
typedef struct {
    vtss_vcap_vid_t  vid;
    vtss_vcap_u16_t  etype;
    vtss_vcap_bit_t  etype_len;
    vtss_vcap_bit_t  ip_snap;
    vtss_vcap_bit_t  ip4;
    vtss_vcap_bit_t  ip_mc;
    vtss_vcap_bit_t  l3_fragment;
    vtss_vcap_bit_t  l3_options;
    vtss_vcap_u8_t   l3_dscp;
    vtss_vcap_ip_t   l3_ip4_sip;
    vtss_vcap_ip_t   l3_ip4_dip;
    vtss_vcap_bit_t  tcp_udp;
    vtss_vcap_bit_t  tcp;
    vtss_vcap_u16_t  l4_sport;
    vtss_vcap_u8_t   l4_range;
    vtss_vcap_u128_t l3_ip6_sip;
    vtss_vcap_u128_t l3_ip6_dip;
} srvl_is1_key_info_t;

static void srvl_is1_base_key_set(srvl_tcam_data_t *data, vtss_is1_data_t *is1, 
                                  srvl_is1_key_info_t *info, u32 offset)
{
    u32            mask, old_offset = data->key_offset;
    vtss_is1_key_t *key = &is1->entry->key;
    vtss_is1_tag_t *tag = &key->tag;
    
    data->key_offset += (offset - IS1_QKO_LOOKUP); /* Adjust offset */
    srvl_vcap_key_set(data, IS1_QKO_LOOKUP, IS1_QKL_LOOKUP, 
                      is1->lookup, VTSS_BITMASK(IS1_QKL_LOOKUP));
    mask = srvl_port_mask(key->port_list);
    srvl_vcap_key_set(data, IS1_QKO_IGR_PORT_MASK, IS1_QKL_IGR_PORT_MASK, 0, ~mask);
    srvl_vcap_key_set(data, IS1_QKO_ISDX, IS1_QKL_ISDX, 
                      (key->isdx.value[0] << 8) + key->isdx.value[1],
                      (key->isdx.mask[0] << 8) + key->isdx.mask[1]);
    srvl_vcap_key_bit_set(data, IS1_QKO_L2_MC, key->mac.dmac_mc);
    srvl_vcap_key_bit_set(data, IS1_QKO_L2_BC, key->mac.dmac_bc);
    srvl_vcap_key_bit_set(data, IS1_QKO_IP_MC, info->ip_mc);
    srvl_vcap_key_bit_set(data, IS1_QKO_VLAN_TAGGED, tag->tagged);
    srvl_vcap_key_bit_set(data, IS1_QKO_VLAN_DBL_TAGGED, key->inner_tag.tagged);
    srvl_vcap_key_bit_set(data, IS1_QKO_TPID, tag->s_tag);
    srvl_vcap_key_set(data, IS1_QKO_VID, IS1_QKL_VID, info->vid.value, info->vid.mask);
    srvl_vcap_key_bit_set(data, IS1_QKO_DEI, tag->dei);
    srvl_vcap_key_u3_set(data, IS1_QKO_PCP, &tag->pcp);
    data->key_offset = old_offset; /* Restore offset */
}

static void srvl_is1_inner_key_set(srvl_tcam_data_t *data, vtss_is1_data_t *is1, u32 offset)
{
    u32            old_offset = data->key_offset;
    vtss_is1_tag_t *tag = &is1->entry->key.inner_tag;
    BOOL           range = (tag->vid.type == VTSS_VCAP_VR_TYPE_VALUE_MASK ? 0 : 1);

    data->key_offset += (offset - IS1_QKO_INNER_TPID); /* Adjust offset */
    srvl_vcap_key_bit_set(data, IS1_QKO_INNER_TPID, tag->s_tag);
    srvl_vcap_key_set(data, IS1_QKO_INNER_VID, IS1_QKL_INNER_VID, 
                      range ? 0 : tag->vid.vr.v.value, range ? 0 : tag->vid.vr.v.mask);
    srvl_vcap_key_bit_set(data, IS1_QKO_INNER_DEI, tag->dei);
    srvl_vcap_key_u3_set(data, IS1_QKO_INNER_PCP, &tag->pcp);
    data->key_offset = old_offset; /* Restore offset */
}

static u32 srvl_u8_to_u32(u8 *p)
{
    return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static void srvl_is1_misc_key_set(srvl_tcam_data_t *data, srvl_is1_key_info_t *info, u32 offset) 
{
    u32 old_offset = data->key_offset;
    
    data->key_offset += (offset - IS1_QKO_ETYPE_LEN); /* Adjust offset */
    srvl_vcap_key_bit_set(data, IS1_QKO_ETYPE_LEN, info->etype_len);
    srvl_vcap_key_u16_set(data, IS1_QKO_ETYPE, &info->etype);
    srvl_vcap_key_bit_set(data, IS1_QKO_IP_SNAP, info->ip_snap);
    srvl_vcap_key_bit_set(data, IS1_QKO_IP4, info->ip4);
    srvl_vcap_key_bit_set(data, IS1_QKO_L3_FRAGMENT, info->l3_fragment);
    srvl_vcap_key_bit_set(data, IS1_QKO_L3_FRAG_OFS_GT0, VTSS_VCAP_BIT_ANY);
    srvl_vcap_key_bit_set(data, IS1_QKO_L3_OPTIONS, info->l3_options);
    srvl_vcap_key_u6_set(data, IS1_QKO_L3_DSCP, &info->l3_dscp);
    data->key_offset = old_offset; /* Restore offset */
}

static void srvl_is1_l4_key_set(srvl_tcam_data_t *data, srvl_is1_key_info_t *info, u32 offset) 
{
    u32 old_offset = data->key_offset;
    
    data->key_offset += (offset - IS1_HKO_NORMAL_TCP_UDP); /* Adjust offset */
    srvl_vcap_key_bit_set(data, IS1_HKO_NORMAL_TCP_UDP, info->tcp_udp);
    srvl_vcap_key_bit_set(data, IS1_HKO_NORMAL_TCP, info->tcp);
    srvl_vcap_key_u16_set(data, IS1_HKO_NORMAL_L4_SPORT, &info->l4_sport);
    srvl_vcap_key_u8_set(data, IS1_HKO_NORMAL_L4_RNG, &info->l4_range);
    data->key_offset = old_offset; /* Restore offset */
}

static void srvl_is1_range_update(vtss_vcap_vr_type_t type, u32 range, srvl_is1_key_info_t *info)
{
    u32 mask;

    if (type != VTSS_VCAP_VR_TYPE_VALUE_MASK && range != VTSS_VCAP_RANGE_CHK_NONE) {
        /* Range checker has been allocated */
        mask = (1 << range);
        info->l4_range.mask |= mask;
        if (type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE)
            info->l4_range.value |= mask;
    }
}

static void srvl_is1_l4port_update(vtss_vcap_vr_t *vr, u32 range, 
                                   vtss_vcap_u16_t *val, srvl_is1_key_info_t *info, BOOL *l4_used)
{
    u32 mask, value;
    
    if (vr->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
        /* Value/mask */
        mask = vr->vr.v.mask;
        value = vr->vr.v.value;
        if (mask != 0) {
            *l4_used = 1;
            val->value[0] = ((value >> 8) & 0xff);
            val->value[1] = (value & 0xff);
            val->mask[0] = ((mask >> 8) & 0xff);
            val->mask[1] = (mask & 0xff);
        }
    } else if (range != VTSS_VCAP_RANGE_CHK_NONE) {
        /* Range */
        *l4_used = 1;
        srvl_is1_range_update(vr->type, range, info);
    }
}

static vtss_rc srvl_is1_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *vcap_data, u32 counter)
{
    const tcam_props_t     *tcam = &tcam_info[VTSS_TCAM_IS1];
    srvl_tcam_data_t       tcam_data, *data = &tcam_data;
    vtss_is1_data_t        *is1 = &vcap_data->u.is1;
    vtss_is1_key_t         *key = &is1->entry->key;
    vtss_is1_action_t      *action = &is1->entry->action;
    vtss_is1_frame_etype_t *etype = &key->frame.etype;
    vtss_is1_frame_llc_t   *llc = &key->frame.llc;
    vtss_is1_frame_snap_t  *snap = &key->frame.snap;
    vtss_is1_frame_ipv4_t  *ipv4 = &key->frame.ipv4;
    vtss_is1_frame_ipv6_t  *ipv6 = &key->frame.ipv6;
    vtss_vcap_u8_t         *proto = NULL;
    vtss_vcap_vr_t         *dscp, *sport, *dport, *vid;
    vtss_vcap_key_size_t   key_size;
    u32                    oam = 0, i, n, type = IS1_TYPE_NORMAL;
    srvl_is1_key_info_t    info;
    BOOL                   l4_used = 0;

    VTSS_IG(VTSS_TRACE_GROUP_EVC, "enter");
    
    /* Check key size and type */
    switch (key->key_type) {
    case VTSS_VCAP_KEY_TYPE_DOUBLE_TAG:
        key_size = VTSS_VCAP_KEY_SIZE_QUARTER;
        break;
    case VTSS_VCAP_KEY_TYPE_NORMAL:
    case VTSS_VCAP_KEY_TYPE_IP_ADDR:
        key_size = VTSS_VCAP_KEY_SIZE_HALF;
        break;
    case VTSS_VCAP_KEY_TYPE_MAC_IP_ADDR:
        key_size = VTSS_VCAP_KEY_SIZE_FULL;
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal key type");
        return VTSS_RC_ERROR;
    }

    if (idx->key_size != key_size) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal key_size: %s, expected: %s",
                vtss_vcap_key_size_txt(idx->key_size), vtss_vcap_key_size_txt(key_size));
        return VTSS_RC_ERROR;
    }

    /* Initialize IS1 info */
    memset(&info, 0, sizeof(info));

    /* VLAN value/range */
    vid = &key->tag.vid;
    if (vid->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
        info.vid.value = vid->vr.v.value;
        info.vid.mask = vid->vr.v.mask;
    } 
    srvl_is1_range_update(vid->type, is1->vid_range, &info);

    /* Frame type specific processing */
    switch (key->type) {
    case VTSS_IS1_TYPE_ANY:
        break;
    case VTSS_IS1_TYPE_ETYPE:
        info.etype_len = VTSS_VCAP_BIT_1;
        info.ip_snap = VTSS_VCAP_BIT_0;
        info.ip4 = VTSS_VCAP_BIT_0;
        info.etype = etype->etype;
        info.l3_ip4_sip.value = srvl_u8_to_u32(etype->data.value);
        info.l3_ip4_sip.mask = srvl_u8_to_u32(etype->data.mask);
        for (i = 0; i < 4; i++) {
            info.l3_ip6_sip.value[i + 8] = etype->data.value[i];
            info.l3_ip6_sip.mask[i + 8] = etype->data.mask[i];
        }
        break;
    case VTSS_IS1_TYPE_LLC:
        info.etype_len = VTSS_VCAP_BIT_0;
        info.ip_snap = VTSS_VCAP_BIT_0;
        info.ip4 = VTSS_VCAP_BIT_0;
        info.etype.value[0] = llc->data.value[0];
        info.etype.mask[0] = llc->data.mask[0];
        info.etype.value[1] = llc->data.value[1];
        info.etype.mask[1] = llc->data.mask[1];
        info.l3_ip4_sip.value = srvl_u8_to_u32(&llc->data.value[2]);
        info.l3_ip4_sip.mask = srvl_u8_to_u32(&llc->data.mask[2]);
        for (i = 2; i < 6; i++) {
            info.l3_ip6_sip.value[i + 6] = llc->data.value[i];
            info.l3_ip6_sip.mask[i + 6] = llc->data.mask[i];
        }
        break;
    case VTSS_IS1_TYPE_SNAP:
        info.etype_len = VTSS_VCAP_BIT_0;
        info.ip_snap = VTSS_VCAP_BIT_1;
        info.ip4 = VTSS_VCAP_BIT_0;
        info.etype.value[0] = snap->data.value[0];
        info.etype.mask[0] = snap->data.mask[0];
        info.etype.value[1] = snap->data.value[1];
        info.etype.mask[1] = snap->data.mask[1];
        info.l3_ip4_sip.value = srvl_u8_to_u32(&snap->data.value[2]);
        info.l3_ip4_sip.mask = srvl_u8_to_u32(&snap->data.mask[2]);
        for (i = 2; i < 6; i++) {
            info.l3_ip6_dip.value[i + 6] = snap->data.value[i];
            info.l3_ip6_dip.mask[i + 6] = snap->data.mask[i];
        }
        break;
    case VTSS_IS1_TYPE_IPV4:
    case VTSS_IS1_TYPE_IPV6:
        if (key->key_type == VTSS_VCAP_KEY_TYPE_IP_ADDR)
            type = IS1_TYPE_5TUPLE_IP4;
        info.etype_len = VTSS_VCAP_BIT_1;
        info.ip_snap = VTSS_VCAP_BIT_1;
        if (key->type == VTSS_IS1_TYPE_IPV4) {
            info.ip4 = VTSS_VCAP_BIT_1;
            info.ip_mc = ipv4->ip_mc;
            info.l3_fragment = ipv4->fragment;
            info.l3_options = ipv4->options;
            proto = &ipv4->proto;
            dscp = &ipv4->dscp;
            info.l3_ip4_sip = ipv4->sip;
            info.l3_ip4_dip = ipv4->dip;
            for (i = 12; i < 16; i++) {
                n = (15 - i)*8;
                info.l3_ip6_sip.value[i] = ((ipv4->sip.value >> n) & 0xff);
                info.l3_ip6_sip.mask[i] = ((ipv4->sip.mask >> n) & 0xff);
                info.l3_ip6_dip.value[i] = ((ipv4->dip.value >> n) & 0xff);
                info.l3_ip6_dip.mask[i] = ((ipv4->dip.mask >> n) & 0xff);
            }
            sport = &ipv4->sport;
            dport = &ipv4->dport;
        } else {
            info.ip4 = VTSS_VCAP_BIT_0;
            info.ip_mc = ipv6->ip_mc;
            proto = &ipv6->proto;
            dscp = &ipv6->dscp;
            info.l3_ip4_sip.value = srvl_u8_to_u32(ipv6->sip.value);
            info.l3_ip4_sip.mask = srvl_u8_to_u32(ipv6->sip.mask);
            info.l3_ip4_dip.value = srvl_u8_to_u32(&ipv6->dip.value[12]);
            info.l3_ip4_dip.mask = srvl_u8_to_u32(&ipv6->dip.mask[12]);
            info.l3_ip6_sip = ipv6->sip;
            info.l3_ip6_dip = ipv6->dip;
            sport = &ipv6->sport;
            dport = &ipv6->dport;
        }
        if (dscp->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
            info.l3_dscp.value = dscp->vr.v.value;
            info.l3_dscp.mask = dscp->vr.v.mask;
        }
        srvl_is1_range_update(dscp->type, is1->dscp_range, &info);
        if (vtss_vcap_udp_tcp_rule(proto)) {
            info.tcp_udp = VTSS_VCAP_BIT_1;
            info.tcp = (proto->value == 6 ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_0);

            /* Destination port/range */
            srvl_is1_l4port_update(dport, is1->dport_range, &info.etype, &info, &l4_used);

            /* Source port/range */
            srvl_is1_l4port_update(sport, is1->sport_range, &info.l4_sport, &info, &l4_used);
            
            if (l4_used) {
                /* L4 used, ignore fragments and options */
                info.l3_fragment = VTSS_VCAP_BIT_0;
                info.l3_options = VTSS_VCAP_BIT_0;
            }
        } else {
            /* Not UDP/TCP */
            info.tcp_udp = VTSS_VCAP_BIT_0;
            info.etype.value[1] = proto->value;
            info.etype.mask[1] = proto->mask;
        }
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal frame type");
        return VTSS_RC_ERROR;
    }

    /* Get TCAM data */
    data->type = 0;
    data->cnt = counter;
    VTSS_RC(srvl_vcap_entry_add(tcam, idx, data));
    
    /* Setup key fields */
    if (idx->key_size == VTSS_VCAP_KEY_SIZE_QUARTER) {
        /* Quarter key */
        srvl_is1_base_key_set(data, is1, &info, IS1_QKO_LOOKUP);
        srvl_is1_inner_key_set(data, is1, IS1_QKO_INNER_TPID);
        srvl_is1_misc_key_set(data, &info, IS1_QKO_ETYPE_LEN);
        srvl_vcap_key_bit_set(data, IS1_QKO_TCP_UDP, info.tcp_udp);
        srvl_vcap_key_bit_set(data, IS1_QKO_TCP, info.tcp);
    } else if (idx->key_size == VTSS_VCAP_KEY_SIZE_HALF) {
        /* Half key */
        srvl_vcap_key_set(data, IS1_HKO_TYPE, IS1_HKL_TYPE, type, VTSS_BITMASK(IS1_HKL_TYPE));
        srvl_is1_base_key_set(data, is1, &info, IS1_HKO_LOOKUP);
        if (type == IS1_TYPE_NORMAL) {
            srvl_vcap_key_u48_set(data, IS1_HKO_NORMAL_L2_SMAC, &key->mac.smac);
            srvl_vcap_key_ipv4_set(data, IS1_HKO_NORMAL_L3_IP4_SIP, &info.l3_ip4_sip);
            srvl_is1_misc_key_set(data, &info, IS1_HKO_NORMAL_ETYPE_LEN);
            srvl_is1_l4_key_set(data, &info, IS1_HKO_NORMAL_TCP_UDP);
        } else {
            /* IS1_TYPE_5TUPLE_IP4 */
            srvl_is1_inner_key_set(data, is1, IS1_HKO_5TUPLE_IP4_INNER_TPID);
            srvl_vcap_key_bit_set(data, IS1_HKO_5TUPLE_IP4_IP4, info.ip4);
            srvl_vcap_key_bit_set(data, IS1_HKO_5TUPLE_IP4_L3_FRAGMENT, info.l3_fragment);
            srvl_vcap_key_bit_set(data, IS1_HKO_5TUPLE_IP4_L3_FRAG_OFS_GT0, VTSS_VCAP_BIT_ANY);
            srvl_vcap_key_bit_set(data, IS1_HKO_5TUPLE_IP4_L3_OPTIONS, info.l3_options);
            srvl_vcap_key_u6_set(data, IS1_HKO_5TUPLE_IP4_L3_DSCP, &info.l3_dscp);
            srvl_vcap_key_ipv4_set(data, IS1_HKO_5TUPLE_IP4_L3_IP4_DIP, &info.l3_ip4_dip);
            srvl_vcap_key_ipv4_set(data, IS1_HKO_5TUPLE_IP4_L3_IP4_SIP, &info.l3_ip4_sip);
            srvl_vcap_key_u8_set(data, IS1_HKO_5TUPLE_IP4_L3_IP_PROTO, proto);
            srvl_vcap_key_bit_set(data, IS1_HKO_5TUPLE_IP4_TCP_UDP, info.tcp_udp);
            srvl_vcap_key_bit_set(data, IS1_HKO_5TUPLE_IP4_TCP, info.tcp);
            srvl_vcap_key_u8_set(data, IS1_HKO_5TUPLE_IP4_L4_RNG, &info.l4_range);
            srvl_vcap_key_set(data, IS1_HKO_5TUPLE_IP4_IP_PAYLOAD, IS1_HKL_5TUPLE_IP4_IP_PAYLOAD,
                              0, 0);
        }
    } else {
        /* Full key */
        srvl_vcap_key_set(data, IS1_FKO_TYPE, IS1_FKL_TYPE, 
                          IS1_TYPE_7TUPLE, VTSS_BITMASK(IS1_FKL_TYPE));
        srvl_is1_base_key_set(data, is1, &info, IS1_FKO_LOOKUP);
        srvl_is1_inner_key_set(data, is1, IS1_FKO_INNER_TPID);
        srvl_vcap_key_u48_set(data, IS1_FKO_7TUPLE_L2_DMAC, &key->mac.dmac);
        srvl_vcap_key_u48_set(data, IS1_FKO_7TUPLE_L2_SMAC, &key->mac.smac);
        srvl_is1_misc_key_set(data, &info, IS1_FKO_7TUPLE_ETYPE_LEN);
        srvl_vcap_key_bytes_set(data, IS1_FKO_7TUPLE_L3_IP6_DIP_MSB,
                                info.l3_ip6_dip.value, info.l3_ip6_dip.mask, 4);
        srvl_vcap_key_bytes_set(data, IS1_FKO_7TUPLE_L3_IP6_DIP,
                                &info.l3_ip6_dip.value[8], &info.l3_ip6_dip.mask[8], 8);
        srvl_vcap_key_bytes_set(data, IS1_FKO_7TUPLE_L3_IP6_SIP_MSB,
                                info.l3_ip6_sip.value, info.l3_ip6_sip.mask, 4);
        srvl_vcap_key_bytes_set(data, IS1_FKO_7TUPLE_L3_IP6_SIP,
                                &info.l3_ip6_sip.value[8], &info.l3_ip6_sip.mask[8], 8);
        srvl_is1_l4_key_set(data, &info, IS1_FKO_7TUPLE_TCP_UDP);
    }

    /* Setup action fields */
    srvl_vcap_action_bit_set(data, IS1_AO_DSCP_ENA, action->dscp_enable);
    srvl_vcap_action_set(data, IS1_AO_DSCP_VAL, IS1_AL_DSCP_VAL, action->dscp);
    srvl_vcap_action_bit_set(data, IS1_AO_QOS_ENA, action->prio_enable);
    srvl_vcap_action_set(data, IS1_AO_QOS_VAL, IS1_AL_QOS_VAL, action->prio);
    srvl_vcap_action_bit_set(data, IS1_AO_DP_ENA, action->dp_enable);
    srvl_vcap_action_bit_set(data, IS1_AO_DP_VAL, action->dp);
    srvl_vcap_action_set(data, IS1_AO_PAG_OVERRIDE_MASK, IS1_AL_PAG_OVERRIDE_MASK, 
                         0xc0 + (action->pag_enable ? 0x3f : 0));
#if defined(VTSS_FEATURE_EVC)
    oam = (action->oam_detect == VTSS_MCE_OAM_DETECT_UNTAGGED ? 1 :
           action->oam_detect == VTSS_MCE_OAM_DETECT_SINGLE_TAGGED ? 2 :
           action->oam_detect == VTSS_MCE_OAM_DETECT_DOUBLE_TAGGED ? 3 : 0);
#endif /* VTSS_FEATURE_EVC */
    srvl_vcap_action_set(data, IS1_AO_PAG_VAL, IS1_AL_PAG_VAL, (oam << 6) + (action->pag & 0x3f));
    srvl_vcap_action_bit_set(data, IS1_AO_ISDX_REPLACE_ENA, action->isdx_enable);
    srvl_vcap_action_set(data, IS1_AO_ISDX_ADD_VAL, IS1_AL_ISDX_ADD_VAL, action->isdx);
    srvl_vcap_action_bit_set(data, IS1_AO_VID_REPLACE_ENA, 
                             action->vid_enable || action->vid != VTSS_VID_NULL ? 1 : 0);
    srvl_vcap_action_set(data, IS1_AO_VID_ADD_VAL, IS1_AL_VID_ADD_VAL, action->vid);
    srvl_vcap_action_set(data, IS1_AO_FID_SEL, IS1_AL_FID_SEL, action->fid_sel);
    srvl_vcap_action_set(data, IS1_AO_FID_VAL, IS1_AL_FID_VAL, action->fid_val);
    srvl_vcap_action_bit_set(data, IS1_AO_PCP_DEI_ENA, action->pcp_dei_enable);
    srvl_vcap_action_set(data, IS1_AO_PCP_VAL, IS1_AL_PCP_VAL, action->pcp);
    srvl_vcap_action_bit_set(data, IS1_AO_DEI_VAL, action->dei);
    srvl_vcap_action_bit_set(data, IS1_AO_VLAN_POP_CNT_ENA, action->pop_enable);
    srvl_vcap_action_set(data, IS1_AO_VLAN_POP_CNT, IS1_AL_VLAN_POP_CNT, action->pop);
    srvl_vcap_action_set(data, IS1_AO_CUSTOM_ACE_TYPE_ENA, IS1_AL_CUSTOM_ACE_TYPE_ENA, 0);
    
    /* Set TCAM data */
    return srvl_vcap_entry_set(tcam, idx, data);
}

static vtss_rc srvl_is1_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "enter");

    return srvl_vcap_entry_del(VTSS_TCAM_IS1, idx);
}

static vtss_rc srvl_is1_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "enter");

    return srvl_vcap_entry_move(VTSS_TCAM_IS1, idx, count, up);
}

static void srvl_debug_is1_base(srvl_debug_info_t *info, u32 offset)
{
    u32                 old_offset = info->data.key_offset;
    vtss_debug_printf_t pr = info->pr;

    info->data.key_offset += (offset - IS1_QKO_LOOKUP); /* Adjust offset */
    srvl_debug_bits(info, "lookup", IS1_QKO_LOOKUP, IS1_QKL_LOOKUP);
    srvl_debug_bits(info, "igr_port_mask", IS1_QKO_IGR_PORT_MASK, IS1_QKL_IGR_PORT_MASK);
    srvl_debug_bits(info, "isdx", IS1_QKO_ISDX, IS1_QKL_ISDX);
    pr("\n");
    srvl_debug_bit(info, "l2_mc", IS1_QKO_L2_MC);
    srvl_debug_bit(info, "l2_bc", IS1_QKO_L2_BC);
    srvl_debug_bit(info, "ip_mc", IS1_QKO_IP_MC);
    srvl_debug_bit(info, "tagged", IS1_QKO_VLAN_TAGGED);
    srvl_debug_bit(info, "dbl_tagged", IS1_QKO_VLAN_DBL_TAGGED);
    pr("\n");
    srvl_debug_bit(info, "tpid", IS1_QKO_TPID);
    srvl_debug_u12(info, "vid", IS1_QKO_VID);
    srvl_debug_bit(info, "dei", IS1_QKO_DEI);
    srvl_debug_u3(info, "pcp", IS1_QKO_PCP);
    pr("\n");
    info->data.key_offset = old_offset; /* Restore offset */
}

static void srvl_debug_is1_inner(srvl_debug_info_t *info, u32 offset)
{
    u32 old_offset = info->data.key_offset;

    info->data.key_offset += (offset - IS1_QKO_INNER_TPID); /* Adjust offset */
    srvl_debug_bit(info, "in_tpid", IS1_QKO_INNER_TPID);
    srvl_debug_u12(info, "in_vid", IS1_QKO_INNER_VID);
    srvl_debug_bit(info, "in_dei", IS1_QKO_INNER_DEI);
    srvl_debug_u3(info, "in_pcp", IS1_QKO_INNER_PCP);
    info->pr("\n");
    info->data.key_offset = old_offset; /* Restore offset */
}

static void srvl_debug_is1_misc(srvl_debug_info_t *info, u32 offset)
{
    u32                 old_offset = info->data.key_offset;
    vtss_debug_printf_t pr = info->pr;

    info->data.key_offset += (offset - IS1_QKO_ETYPE_LEN); /* Adjust offset */
    srvl_debug_bit(info, "etype_len", IS1_QKO_ETYPE_LEN);
    srvl_debug_u16(info, "etype", IS1_QKO_ETYPE);
    srvl_debug_bit(info, "snap", IS1_QKO_IP_SNAP);
    srvl_debug_bit(info, "ip4", IS1_QKO_IP4);
    pr("\n");
    srvl_debug_bit(info, "l3_fragment", IS1_QKO_L3_FRAGMENT);
    srvl_debug_bit(info, "l3_fragoffs", IS1_QKO_L3_FRAG_OFS_GT0);
    srvl_debug_bit(info, "l3_options", IS1_QKO_L3_OPTIONS);
    srvl_debug_u6(info, "l3_dscp", IS1_QKO_L3_DSCP);
    pr("\n");
    info->data.key_offset = old_offset; /* Restore offset */
}

static void srvl_debug_is1_l4(srvl_debug_info_t *info, u32 offset)
{
    u32 old_offset = info->data.key_offset;

    info->data.key_offset += (offset - IS1_HKO_NORMAL_TCP_UDP); /* Adjust offset */
    srvl_debug_bit(info, "tcp_udp", IS1_HKO_NORMAL_TCP_UDP);
    srvl_debug_bit(info, "tcp", IS1_HKO_NORMAL_TCP);
    srvl_debug_u16(info, "l4_sport", IS1_HKO_NORMAL_L4_SPORT);
    srvl_debug_u8(info, "l4_rng", IS1_HKO_NORMAL_L4_RNG);
    info->pr("\n");
    info->data.key_offset = old_offset; /* Restore offset */
}

static vtss_rc srvl_debug_is1(srvl_debug_info_t *info)
{
    vtss_debug_printf_t pr = info->pr;
    srvl_tcam_data_t    *data = &info->data;
    u32                 type;

    if (info->is_action) {
        /* Print action */
        srvl_debug_action(info, "dscp", IS1_AO_DSCP_ENA, IS1_AO_DSCP_VAL, IS1_AL_DSCP_VAL);
        srvl_debug_action(info, "qos", IS1_AO_QOS_ENA, IS1_AO_QOS_VAL, IS1_AL_QOS_VAL);
        srvl_debug_action(info, "dp", IS1_AO_DP_ENA, IS1_AO_DP_VAL, IS1_AL_DP_VAL);
        srvl_debug_u8(info, "pag_mask", IS1_AO_PAG_OVERRIDE_MASK);
        srvl_debug_u8(info, "pag_val", IS1_AO_PAG_VAL);
        srvl_debug_action(info, "isdx", IS1_AO_ISDX_REPLACE_ENA, 
                          IS1_AO_ISDX_ADD_VAL, IS1_AL_ISDX_ADD_VAL);
        pr("\n");
        srvl_debug_action(info, "vid", IS1_AO_VID_REPLACE_ENA, 
                          IS1_AO_VID_ADD_VAL, IS1_AL_VID_ADD_VAL);
        srvl_debug_action(info, "pcp", IS1_AO_PCP_DEI_ENA, 
                          IS1_AO_PCP_VAL, IS1_AL_PCP_VAL);
        srvl_debug_action(info, "dei", IS1_AO_DEI_VAL, 0, 0);
        srvl_debug_action(info, "pop", IS1_AO_VLAN_POP_CNT_ENA, 
                          IS1_AO_VLAN_POP_CNT, IS1_AL_VLAN_POP_CNT);
        srvl_debug_action(info, "fid", IS1_AO_FID_SEL, IS1_AO_FID_VAL, IS1_AL_FID_VAL);
        srvl_debug_bits(info, "custom_ace_ena", 
                        IS1_AO_CUSTOM_ACE_TYPE_ENA, IS1_AL_CUSTOM_ACE_TYPE_ENA);
        pr("hit:%u ", info->data.cnt);
        return VTSS_RC_OK;
    }

    /* Print entry */
    if (data->tg_sw == VCAP_TG_QUARTER) {
        /* DBL_VID */
        srvl_debug_is1_base(info, IS1_QKO_LOOKUP);
        srvl_debug_is1_inner(info, IS1_QKO_INNER_TPID);
        srvl_debug_is1_misc(info, IS1_QKO_ETYPE_LEN);
        srvl_debug_bit(info, "tcp_udp", IS1_QKO_TCP_UDP);
        srvl_debug_bit(info, "tcp", IS1_QKO_TCP);
        pr("\n");
        return VTSS_RC_OK;
    } else if (data->tg_sw == VCAP_TG_HALF) {
        /* Common fields for half keys */
        srvl_debug_bits(info, "type", IS1_HKO_TYPE, IS1_HKL_TYPE);
        type = srvl_entry_bs_get(info, IS1_HKO_TYPE, IS1_HKL_TYPE);
        pr("(%s) ",
           type == IS1_TYPE_NORMAL ? "normal" :
           type == IS1_TYPE_5TUPLE_IP4 ? "5tuple_ip4" : "?");
        srvl_debug_is1_base(info, IS1_HKO_LOOKUP);

        /* Specific format for half keys */
        switch (type) {
        case IS1_TYPE_NORMAL:
            srvl_debug_u48(info, "l2_smac", IS1_HKO_NORMAL_L2_SMAC);
            srvl_debug_is1_misc(info, IS1_HKO_NORMAL_ETYPE_LEN);
            srvl_debug_u32(info, "l3_ip4_sip", IS1_HKO_NORMAL_L3_IP4_SIP);
            srvl_debug_is1_l4(info, IS1_HKO_NORMAL_TCP_UDP);
            break;
        case IS1_TYPE_5TUPLE_IP4:
            srvl_debug_is1_inner(info, IS1_HKO_5TUPLE_IP4_INNER_TPID);
            srvl_debug_bit(info, "ip4", IS1_HKO_5TUPLE_IP4_IP4);
            srvl_debug_bit(info, "l3_fragment", IS1_HKO_5TUPLE_IP4_L3_FRAGMENT);
            srvl_debug_bit(info, "l3_fragoffs", IS1_HKO_5TUPLE_IP4_L3_FRAG_OFS_GT0);
            srvl_debug_bit(info, "l3_options", IS1_HKO_5TUPLE_IP4_L3_OPTIONS);
            srvl_debug_u6(info, "l3_dscp", IS1_HKO_5TUPLE_IP4_L3_DSCP);
            pr("\n");
            srvl_debug_u32(info, "l3_ip4_dip", IS1_HKO_5TUPLE_IP4_L3_IP4_DIP);
            srvl_debug_u32(info, "l3_ip4_sip", IS1_HKO_5TUPLE_IP4_L3_IP4_SIP);
            srvl_debug_u8(info, "l3_proto", IS1_HKO_5TUPLE_IP4_L3_IP_PROTO);
            srvl_debug_bit(info, "tcp_udp", IS1_HKO_5TUPLE_IP4_TCP_UDP);
            srvl_debug_bit(info, "tcp", IS1_HKO_5TUPLE_IP4_TCP);
            srvl_debug_u8(info, "l4_rng", IS1_HKO_5TUPLE_IP4_L4_RNG);
            pr("\n");
            srvl_debug_u32(info, "ip_payload", IS1_HKO_5TUPLE_IP4_IP_PAYLOAD);
            break;
        default:
            break;
        }
        return VTSS_RC_OK;
    }

    /* VCAP_TG_FULL */
    srvl_debug_bits(info, "type", IS1_FKO_TYPE, IS1_FKL_TYPE);
    type = srvl_entry_bs_get(info, IS1_FKO_TYPE, IS1_FKL_TYPE);
    pr("(%s) ",
       type == IS1_TYPE_NORMAL_IP6 ? "normal_ip6" :
       type == IS1_TYPE_7TUPLE ? "7tuple" :
       type == IS1_TYPE_5TUPLE_IP6 ? "5tuple_ip6" : "?");

    /* Common format for full keys */
    srvl_debug_is1_base(info, IS1_FKO_LOOKUP);
    srvl_debug_is1_inner(info, IS1_FKO_INNER_TPID);

    /* Specific format for full keys */
    switch (type) {
    case IS1_TYPE_NORMAL_IP6:
        srvl_debug_u48(info, "l2_smac", IS1_FKO_NORMAL_IP6_L2_SMAC);
        srvl_debug_u6(info, "l3_dscp", IS1_FKO_NORMAL_IP6_L3_DSCP);
        srvl_debug_u128(info, "l3_ip6_sip", IS1_FKO_NORMAL_IP6_L3_IP6_SIP);
        srvl_debug_u8(info, "l3_proto", IS1_FKO_NORMAL_IP6_L3_IP_PROTO);
        srvl_debug_bit(info, "tcp_udp", IS1_FKO_NORMAL_IP6_TCP_UDP);
        srvl_debug_u8(info, "l4_rng", IS1_FKO_NORMAL_IP6_L4_RNG);
        srvl_debug_bytes(info, "ip_payload", IS1_FKO_NORMAL_IP6_IP_PAYLOAD, 
                         IS1_FKL_NORMAL_IP6_IP_PAYLOAD);
        break;
    case IS1_TYPE_7TUPLE:
        srvl_debug_u48(info, "l2_dmac", IS1_FKO_7TUPLE_L2_DMAC);
        srvl_debug_u48(info, "l2_smac", IS1_FKO_7TUPLE_L2_SMAC);
        srvl_debug_is1_misc(info, IS1_FKO_7TUPLE_ETYPE_LEN);
        srvl_debug_u16(info, "l3_ip6_dip_msb", IS1_FKO_7TUPLE_L3_IP6_DIP_MSB);
        pr("\n");
        srvl_debug_bytes(info, "l3_ip6_dip", IS1_FKO_7TUPLE_L3_IP6_DIP, IS1_FKL_7TUPLE_L3_IP6_DIP);
        srvl_debug_u16(info, "l3_ip6_sip_msb", IS1_FKO_7TUPLE_L3_IP6_SIP_MSB);
        pr("\n");
        srvl_debug_bytes(info, "l3_ip6_sip", IS1_FKO_7TUPLE_L3_IP6_SIP, IS1_FKL_7TUPLE_L3_IP6_SIP);
        srvl_debug_is1_l4(info, IS1_FKO_7TUPLE_TCP_UDP);
        break;
    case IS1_TYPE_5TUPLE_IP6:
        srvl_debug_u6(info, "l3_dscp", IS1_FKO_5TUPLE_IP6_L3_DSCP);
        srvl_debug_u128(info, "l3_ip6_dip", IS1_FKO_5TUPLE_IP6_L3_IP6_DIP);
        srvl_debug_u128(info, "l3_ip6_sip", IS1_FKO_5TUPLE_IP6_L3_IP6_SIP);
        srvl_debug_u8(info, "l3_proto", IS1_FKO_5TUPLE_IP6_L3_IP_PROTO);
        srvl_debug_bit(info, "tcp_udp", IS1_FKO_5TUPLE_IP6_TCP_UDP);
        srvl_debug_u8(info, "l4_rng", IS1_FKO_5TUPLE_IP6_L4_RNG);
        srvl_debug_u32(info, "ip_payload", IS1_FKO_5TUPLE_IP6_IP_PAYLOAD);
        break;
    default:
        break;
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_is1_all(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    u32  port, i;
    BOOL header = 1;
    char buf[32];
    
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        if (vtss_cmn_port2port_no(info, port) == VTSS_PORT_NO_NONE)
            continue;
        if (header)
            srvl_debug_reg_header(pr, "ANA:PORT");
        header = 0;
        srvl_debug_reg_inst(pr, VTSS_ANA_PORT_VCAP_CFG(port), port, "VCAP_CFG");
        for (i = 0; i < 3; i++) {
            sprintf(buf, "VCAP_S1_CFG_%u", port);
            srvl_debug_reg_inst(pr, VTSS_ANA_PORT_VCAP_S1_KEY_CFG(port, i), i, buf);
        }
    }
    if (!header)
        pr("\n");
    return srvl_debug_vcap(VTSS_TCAM_IS1, "IS1", pr, info, srvl_debug_is1);
}

/* ================================================================= *
 *  IS2 functions
 * ================================================================= */

static vtss_rc srvl_is2_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_IG(VTSS_TRACE_GROUP_SECURITY, "enter");
    
    return srvl_vcap_counter_get(VTSS_TCAM_IS2, idx, counter, clear);
}

static vtss_rc srvl_is2_action_set(srvl_tcam_data_t *data, vtss_acl_action_t *action, u32 counter)
{
    vtss_acl_port_action_t act = action->port_action;
    vtss_acl_ptp_action_t  ptp = action->ptp_action;
    u32                    pol_idx, mask;
    u32                    action_add_sub = 0;
    
    if (data->type == IS2_ACTION_TYPE_SMAC_SIP) {
        srvl_vcap_action_bit_set(data, IS2_AO_SMAC_SIP_CPU_COPY_ENA, 0);
        srvl_vcap_action_set(data, IS2_AO_SMAC_SIP_CPU_QU_NUM, IS2_AL_SMAC_SIP_CPU_QU_NUM, 0);
        srvl_vcap_action_bit_set(data, IS2_AO_SMAC_SIP_FWD_KILL_ENA, 0);
        srvl_vcap_action_bit_set(data, IS2_AO_SMAC_SIP_HOST_MATCH, 1);
        return VTSS_RC_OK;
    }

    srvl_vcap_action_bit_set(data, IS2_AO_HIT_ME_ONCE, action->cpu_once);
    srvl_vcap_action_bit_set(data, IS2_AO_CPU_COPY_ENA, action->cpu);
    srvl_vcap_action_set(data, IS2_AO_CPU_QU_NUM, IS2_AL_CPU_QU_NUM, action->cpu_queue);
    srvl_vcap_action_set(data, IS2_AO_MASK_MODE, IS2_AL_MASK_MODE,
                         act == VTSS_ACL_PORT_ACTION_NONE ? IS2_ACT_MASK_MODE_NONE :
                         act == VTSS_ACL_PORT_ACTION_FILTER ? IS2_ACT_MASK_MODE_FILTER :
                         IS2_ACT_MASK_MODE_REDIR);
    srvl_vcap_action_bit_set(data, IS2_AO_MIRROR_ENA, action->mirror);
    srvl_vcap_action_bit_set(data, IS2_AO_LRN_DIS, 0);
    mask = srvl_port_mask(action->port_list);
    if (act != VTSS_ACL_PORT_ACTION_NONE && mask == 0 && 
        action->cpu_once == 0 && action->cpu == 0) {
        /* Forwarding and CPU copy disabled, discard using policer to avoid CPU copy */
        pol_idx = SRVL_POLICER_DISCARD;
    } else if (action->police) {
        pol_idx = (SRVL_POLICER_ACL + action->policer_no);
    } else if (action->evc_police) {
        pol_idx = (SRVL_POLICER_EVC + action->evc_policer_id);
    } else {
        pol_idx = 0;
    }
    srvl_vcap_action_bit_set(data, IS2_AO_POLICE_ENA, pol_idx ? 1 : 0);
    srvl_vcap_action_set(data, IS2_AO_POLICE_IDX, IS2_AL_POLICE_IDX, pol_idx);
    srvl_vcap_action_bit_set(data, IS2_AO_POLICE_VCAP_ONLY, 0);
    srvl_vcap_action_set(data, IS2_AO_PORT_MASK, IS2_AL_PORT_MASK, mask);

#if defined(VTSS_FEATURE_TIMESTAMP)
    if (vtss_state->ts_add_sub_option && (ptp == VTSS_ACL_PTP_ACTION_ONE_STEP ||
                                          ptp == VTSS_ACL_PTP_ACTION_ONE_STEP_ADD_DELAY ||
                                          ptp == VTSS_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1 ||
                                          ptp == VTSS_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2)) {
        action_add_sub = IS2_ACT_REW_OP_PTP_ONE_ADD_SUB;
        VTSS_D("Setting option add_sub %x in ptp_action %d",action_add_sub, ptp);
    }
#endif /* VTSS_FEATURE_TIMESTAMP */
    srvl_vcap_action_set(
        data, IS2_AO_REW_OP, IS2_AL_REW_OP, 
        action_add_sub |
        (ptp == VTSS_ACL_PTP_ACTION_ONE_STEP ? IS2_ACT_REW_OP_PTP_ONE :
        ptp == VTSS_ACL_PTP_ACTION_TWO_STEP ? IS2_ACT_REW_OP_PTP_TWO :
        ptp == VTSS_ACL_PTP_ACTION_ONE_STEP_ADD_DELAY ? IS2_ACT_REW_OP_PTP_ONE_ADD_DELAY :
        ptp == VTSS_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1 ? IS2_ACT_REW_OP_PTP_ONE_SUB_DELAY_1 :
        ptp == VTSS_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2 ? IS2_ACT_REW_OP_PTP_ONE_SUB_DELAY_2 :
        IS2_ACT_REW_OP_NONE));
    srvl_vcap_action_bit_set(data, IS2_AO_ISDX_ENA, 0);
    srvl_vcap_action_bit_set(data, IS2_AO_LM_CNT_DIS, action->lm_cnt_disable ? 1 : 0);
    srvl_vcap_action_set(data, IS2_AO_ACL_ID, IS2_AL_ACL_ID, 0);
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_is2_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *vcap_data, u32 counter)
{
    const tcam_props_t    *tcam = &tcam_info[VTSS_TCAM_IS2];
    srvl_tcam_data_t      tcam_data, *data = &tcam_data;
    u32                   type, type_mask, i, x, value = 0, mask, count;
    vtss_is2_data_t       *is2 = &vcap_data->u.is2;
    vtss_ace_t            *ace = &is2->entry->ace;
    vtss_ace_frame_ipv4_t *ipv4 = NULL;
    vtss_ace_frame_ipv6_t *ipv6 = NULL;
    vtss_vcap_bit_t       ttl, tcp_fin, tcp_syn, tcp_rst, tcp_psh, tcp_ack, tcp_urg;
    vtss_vcap_bit_t       fragment, options, sip_eq_dip, sport_eq_dport, seq_zero;
    vtss_vcap_u8_t        *proto = NULL, *ds;
    vtss_vcap_u48_t       *ip_data, smac;
    vtss_vcap_udp_tcp_t   *sport, *dport;
    vtss_ace_ptp_t        *ptp;
    u8                    *p;
    vtss_vcap_ip_t        sip;
    vtss_ace_u40_t        llc;
    
    VTSS_IG(VTSS_TRACE_GROUP_SECURITY, "enter");
    
    data->type = IS2_ACTION_TYPE_NORMAL;
    if (idx->key_size == VTSS_VCAP_KEY_SIZE_QUARTER) {
        /* SMAC_SIP4 rule */
        data->type = IS2_ACTION_TYPE_SMAC_SIP;
    } else if (idx->key_size != VTSS_VCAP_KEY_SIZE_HALF) {
        VTSS_EG(VTSS_TRACE_GROUP_SECURITY, "unsupported key_size: %s",
                vtss_vcap_key_size_txt(idx->key_size));
        return VTSS_RC_OK;
    }

    /* Get TCAM data */
    data->cnt = counter;
    VTSS_RC(srvl_vcap_entry_add(tcam, idx, data));

    if (idx->key_size == VTSS_VCAP_KEY_SIZE_QUARTER) {
        /* Setup quarter key fields */
        srvl_vcap_key_set(data, IS2_QKO_IGR_PORT, IS2_QKL_IGR_PORT, 0, 0);
        for (i = 0; i < 6; i++) {
            smac.value[i] = ace->frame.ipv4.sip_smac.smac.addr[i];
            smac.mask[i] = 0xff;
        }
        srvl_vcap_key_u48_set(data, IS2_QKO_L2_SMAC, &smac);
        sip.value = ace->frame.ipv4.sip_smac.sip;
        sip.mask = 0xffffffff;
        srvl_vcap_key_ipv4_set(data, IS2_QKO_L3_IP4_SIP, &sip);

        /* Setup action */
        VTSS_RC(srvl_is2_action_set(data, &ace->action, counter));
        
        /* Set TCAM data */
        return srvl_vcap_entry_set(tcam, idx, data);
    }

    /* Common half key fields */
    srvl_vcap_key_bit_set(data, IS2_HKO_FIRST, ace->lookup ? VTSS_VCAP_BIT_0 : VTSS_VCAP_BIT_1);
    srvl_vcap_key_u8_set(data, IS2_HKO_PAG, &ace->policy);
    mask = srvl_port_mask(ace->port_list);
    srvl_vcap_key_set(data, IS2_HKO_IGR_PORT_MASK, IS2_HKL_IGR_PORT_MASK, 0, ~mask);
    srvl_vcap_key_bit_set(data, IS2_HKO_SERVICE_FRM, 
                          ace->isdx_enable ? VTSS_VCAP_BIT_1 :
                          ace->vlan.vid.mask ? VTSS_VCAP_BIT_0 : VTSS_VCAP_BIT_ANY);
    srvl_vcap_key_bit_set(data, IS2_HKO_HOST_MATCH, 
                          is2->entry->host_match ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_ANY);
    srvl_vcap_key_bit_set(data, IS2_HKO_L2_MC, ace->dmac_mc);
    srvl_vcap_key_bit_set(data, IS2_HKO_L2_BC, ace->dmac_bc);
    srvl_vcap_key_bit_set(data, IS2_HKO_VLAN_TAGGED, ace->vlan.tagged);
    srvl_vcap_key_set(data, IS2_HKO_VID, IS2_HKL_VID,
                      ace->vlan.vid.value, ace->isdx_enable ? 0xfff : ace->vlan.vid.mask);
    srvl_vcap_key_bit_set(data, IS2_HKO_DEI, ace->vlan.cfi);
    srvl_vcap_key_u3_set(data, IS2_HKO_PCP, &ace->vlan.usr_prio);

    /* Type specific fields */
    type_mask = VTSS_BITMASK(IS2_HKL_TYPE);
    switch (ace->type) {
    case VTSS_ACE_TYPE_ANY:
        type = IS2_TYPE_ANY;
        type_mask = 0;
        count = (tcam->entry_width / 2);
        for (i = IS2_HKO_L2_DMAC; i < count; i += 32) {
            /* Clear entry data */
            srvl_vcap_key_set(data, i, MIN(32, count - i), 0, 0);
        }
        break;
    case VTSS_ACE_TYPE_ETYPE:
        type = IS2_TYPE_ETYPE;
        srvl_vcap_key_u48_set(data, IS2_HKO_L2_DMAC, &ace->frame.etype.dmac);
        srvl_vcap_key_u48_set(data, IS2_HKO_L2_SMAC, &ace->frame.etype.smac);
        srvl_vcap_key_u16_set(data, IS2_HKO_MAC_ETYPE_ETYPE, &ace->frame.etype.etype);
        ptp = &ace->frame.etype.ptp;
        if (ptp->enable) {
            /* Encode PTP byte 0,1,4 and 6 in value/mask */
            for (i = 0; i < 2; i++) {
                p = (i ? &ptp->header.mask[0] : &ptp->header.value[0]);
                x = (((p[3] & 0x80) >> 5) | ((p[3] & 0x6) >> 1)); /* Bit 7,2,1 */
                x = (p[1] +         /* Byte 1 (versionPTP) */
                     (p[0] << 8) +  /* Byte 0 (messageType) */
                     (p[2] << 16) + /* Byte 4 (domainNumber) */
                     (x << 24));    /* Byte 6 (flagField, bit 7,2,1) */
                if (i)
                    mask = x;
                else
                    value = x;
            }
            srvl_vcap_key_set(data, IS2_HKO_MAC_ETYPE_L2_PAYLOAD, IS2_HKL_MAC_ETYPE_L2_PAYLOAD, 
                              value, mask);
        } else {
            srvl_vcap_key_u16_set(data, IS2_HKO_MAC_ETYPE_L2_PAYLOAD, &ace->frame.etype.data);
            srvl_vcap_key_set(data, IS2_HKO_MAC_ETYPE_L2_PAYLOAD + 16, 
                              IS2_HKL_MAC_ETYPE_L2_PAYLOAD - 16, 0, 0);
        }
        break;
    case VTSS_ACE_TYPE_LLC:
        type = IS2_TYPE_LLC;
        srvl_vcap_key_u48_set(data, IS2_HKO_L2_DMAC, &ace->frame.llc.dmac);
        srvl_vcap_key_u48_set(data, IS2_HKO_L2_SMAC, &ace->frame.llc.smac);
        for (i = 0; i < 4; i++) {
            llc.value[i] = ace->frame.llc.llc.value[i];
            llc.mask[i] = ace->frame.llc.llc.mask[i];
        }
        llc.value[4] = 0;
        llc.mask[4] = 0;
        srvl_vcap_key_u40_set(data, IS2_HKO_MAC_LLC_L2_LLC, &llc);
        break;
    case VTSS_ACE_TYPE_SNAP:
        type = IS2_TYPE_SNAP;
        srvl_vcap_key_u48_set(data, IS2_HKO_L2_DMAC, &ace->frame.snap.dmac);
        srvl_vcap_key_u48_set(data, IS2_HKO_L2_SMAC, &ace->frame.snap.smac);
        srvl_vcap_key_u40_set(data, IS2_HKO_MAC_SNAP_L2_SNAP, &ace->frame.snap.snap);
        break;
    case VTSS_ACE_TYPE_ARP:
        type = IS2_TYPE_ARP;
        srvl_vcap_key_u48_set(data, IS2_HKO_MAC_ARP_L2_SMAC, &ace->frame.arp.smac);
        srvl_vcap_key_bit_set(data, IS2_HKO_MAC_ARP_ARP_ADDR_SPACE_OK, ace->frame.arp.ethernet);
        srvl_vcap_key_bit_set(data, IS2_HKO_MAC_ARP_ARP_PROTO_SPACE_OK, ace->frame.arp.ip);
        srvl_vcap_key_bit_set(data, IS2_HKO_MAC_ARP_ARP_LEN_OK, ace->frame.arp.length);
        srvl_vcap_key_bit_set(data, IS2_HKO_MAC_ARP_ARP_TGT_MATCH, ace->frame.arp.dmac_match);
        srvl_vcap_key_bit_set(data, IS2_HKO_MAC_ARP_ARP_SENDER_MATCH, ace->frame.arp.smac_match);
        srvl_vcap_key_bit_set(data, IS2_HKO_MAC_ARP_ARP_OPCODE_UNKNOWN, ace->frame.arp.unknown);
        srvl_vcap_key_bit_inv_set(data, IS2_HKO_MAC_ARP_ARP_OPCODE, ace->frame.arp.req);
        srvl_vcap_key_bit_inv_set(data, IS2_HKO_MAC_ARP_ARP_OPCODE + 1, ace->frame.arp.arp);
        srvl_vcap_key_ipv4_set(data, IS2_HKO_MAC_ARP_L3_IP4_DIP, &ace->frame.arp.dip);
        srvl_vcap_key_ipv4_set(data, IS2_HKO_MAC_ARP_L3_IP4_SIP, &ace->frame.arp.sip);
        srvl_vcap_key_bit_set(data, IS2_HKO_MAC_ARP_DIP_EQ_SIP, VTSS_VCAP_BIT_ANY);
        break;
    case VTSS_ACE_TYPE_IPV4:
    case VTSS_ACE_TYPE_IPV6:
        if (ace->type == VTSS_ACE_TYPE_IPV4) {
            /* IPv4 */
            ipv4 = &ace->frame.ipv4;
            proto = &ipv4->proto;
            ttl = ipv4->ttl;
            fragment = ipv4->fragment;
            options = ipv4->options;
            ds = &ipv4->ds;
            ip_data = &ipv4->data;
            sport = &ipv4->sport;
            dport = &ipv4->dport;
            tcp_fin = ipv4->tcp_fin;
            tcp_syn = ipv4->tcp_syn;
            tcp_rst = ipv4->tcp_rst;
            tcp_psh = ipv4->tcp_psh;
            tcp_ack = ipv4->tcp_ack;
            tcp_urg = ipv4->tcp_urg;
            sip_eq_dip = ipv4->sip_eq_dip;
            sport_eq_dport = ipv4->sport_eq_dport;
            seq_zero = ipv4->seq_zero;
            ptp = &ipv4->ptp;
        } else {
            /* IPv6 */
            ipv6 = &ace->frame.ipv6;
            proto = &ipv6->proto;
            ttl = ipv6->ttl;
            fragment = VTSS_VCAP_BIT_ANY;
            options = VTSS_VCAP_BIT_ANY;
            ds = &ipv6->ds;
            ip_data = &ipv6->data;
            sport = &ipv6->sport;
            dport = &ipv6->dport;
            tcp_fin = ipv6->tcp_fin;
            tcp_syn = ipv6->tcp_syn;
            tcp_rst = ipv6->tcp_rst;
            tcp_psh = ipv6->tcp_psh;
            tcp_ack = ipv6->tcp_ack;
            tcp_urg = ipv6->tcp_urg;
            sip_eq_dip = ipv6->sip_eq_dip;
            sport_eq_dport = ipv6->sport_eq_dport;
            seq_zero = ipv6->seq_zero;
            ptp = &ipv6->ptp;
        }
        srvl_vcap_key_bit_set(data, IS2_HKO_IP4, ipv4 ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_0);
        srvl_vcap_key_bit_set(data, IS2_HKO_L3_FRAGMENT, fragment);
        srvl_vcap_key_bit_set(data, IS2_HKO_L3_FRAG_OFS_GT0, VTSS_VCAP_BIT_ANY);
        srvl_vcap_key_bit_set(data, IS2_HKO_L3_OPTIONS, options);
        srvl_vcap_key_bit_set(data, IS2_HKO_L3_TTL_GT0, ttl);
        srvl_vcap_key_u8_set(data, IS2_HKO_L3_TOS, ds);
        if (ipv4) {
            srvl_vcap_key_ipv4_set(data, IS2_HKO_L3_IP4_DIP, &ipv4->dip);
            srvl_vcap_key_ipv4_set(data, IS2_HKO_L3_IP4_SIP, &ipv4->sip);
        } else if (ipv6) {
            srvl_vcap_key_bytes_set(data, IS2_HKO_L3_IP4_DIP, &ipv6->sip.value[8], 
                                    &ipv6->sip.mask[8], 8);
        }
        srvl_vcap_key_bit_set(data, IS2_HKO_DIP_EQ_SIP, sip_eq_dip);
        
        if (vtss_vcap_udp_tcp_rule(proto)) {
            /* UDP/TCP protocol match */
            type = IS2_TYPE_IP_UDP_TCP;
            srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_TCP, 
                                  proto->value == 6 ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_0);
            mask = (dport->in_range && dport->low == dport->high ? 0xffff : 0);
            srvl_vcap_key_set(data, IS2_HKO_IP4_TCP_UDP_L4_DPORT, IS2_HKL_IP4_TCP_UDP_L4_DPORT, 
                              dport->low, mask);
            mask = (sport->in_range && sport->low == sport->high ? 0xffff : 0);
            srvl_vcap_key_set(data, IS2_HKO_IP4_TCP_UDP_L4_SPORT, IS2_HKL_IP4_TCP_UDP_L4_SPORT, 
                              sport->low, mask);
            value = (is2->srange == VTSS_VCAP_RANGE_CHK_NONE ? 0 : (1 << is2->srange));
            if (is2->drange != VTSS_VCAP_RANGE_CHK_NONE)
                value |= (1<<is2->drange);
            srvl_vcap_key_set(data, IS2_HKO_IP4_TCP_UDP_L4_RNG, IS2_HKL_IP4_TCP_UDP_L4_RNG, 
                              value, value);
            srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_SPORT_EQ_DPORT, sport_eq_dport);
            
            if (ptp->enable) {
                /* Encode PTP byte 0,1,4 and 6 in value/mask */
                for (i = 0; i < 2; i++) {
                    p = (i ? &ptp->header.mask[0] : &ptp->header.value[0]);
                    x = (((p[3] & 0x80) >> 5) | ((p[3] & 0x6) >> 1)); /* Bit 7,2,1 */
                    x = ((p[0] & 0xf) +         /* messageType */
                         (x << 4) +             /* flagField, bit 7,2,1 */
                         (p[2] << 7) +          /* domainNumber */
                         ((p[1] & 0xf) << 15)); /* versionPTP */
                    if (i)
                        mask = x;
                    else
                        value = x;
                }
                srvl_vcap_key_set(data, IS2_HKO_IP4_TCP_UDP_SEQUENCE_EQ0, 19, value, mask);
            } else {
                srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_SEQUENCE_EQ0, seq_zero);
                srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_L4_FIN, tcp_fin);
                srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_L4_SYN, tcp_syn);
                srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_L4_RST, tcp_rst);
                srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_L4_PSH, tcp_psh);
                srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_L4_ACK, tcp_ack);
                srvl_vcap_key_bit_set(data, IS2_HKO_IP4_TCP_UDP_L4_URG, tcp_urg);
            }
        } else if (proto->mask == 0) {
            /* Any IP protocol match */
            type = IS2_TYPE_IP_UDP_TCP;
            type_mask = IS2_TYPE_MASK_IP_ANY;
            srvl_vcap_key_set(data, IS2_HKO_IP4_OTHER_L3_PROTO, 32, 0, 0);
            srvl_vcap_key_set(data, IS2_HKO_IP4_OTHER_L3_PROTO + 32, 32, 0, 0);
        } else {
            /* Non-UDP/TCP protocol match */
            type = IS2_TYPE_IP_OTHER;
            srvl_vcap_key_u8_set(data, IS2_HKO_IP4_OTHER_L3_PROTO, proto);
            srvl_vcap_key_set(data, IS2_HKO_IP4_OTHER_L3_PAYLOAD, 8, 0, 0);
            srvl_vcap_key_u48_set(data, IS2_HKO_IP4_OTHER_L3_PAYLOAD + 8, ip_data);
        }
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_SECURITY, "illegal type: %d", ace->type);
        return VTSS_RC_ERROR;
    }
    
    /* Setup entry type */
    srvl_vcap_key_set(data, IS2_HKO_TYPE, IS2_HKL_TYPE, type, type_mask);

    /* Setup action */
    VTSS_RC(srvl_is2_action_set(data, &ace->action, counter));

    /* Set TCAM data */
    return srvl_vcap_entry_set(tcam, idx, data);
}

static vtss_rc srvl_is2_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_IG(VTSS_TRACE_GROUP_SECURITY, "enter");

    return srvl_vcap_entry_del(VTSS_TCAM_IS2, idx);
}

static vtss_rc srvl_is2_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_IG(VTSS_TRACE_GROUP_SECURITY, "enter");

    return srvl_vcap_entry_move(VTSS_TCAM_IS2, idx, count, up);
}

static void srvl_debug_is2_base(srvl_debug_info_t *info, u32 offset)
{
    vtss_debug_printf_t pr = info->pr;
    u32                 old_offset = info->data.key_offset;

    info->data.key_offset += (offset - IS2_FKO_PAG); /* Adjust offset */
    srvl_debug_bits(info, "pag", IS2_FKO_PAG, IS2_FKL_PAG);
    srvl_debug_bits(info, "igr_port_mask", IS2_FKO_IGR_PORT_MASK, IS2_FKL_IGR_PORT_MASK);
    pr("\n");
    srvl_debug_bit(info, "isdx_neq0", IS2_FKO_SERVICE_FRM);
    srvl_debug_bit(info, "host", IS2_FKO_HOST_MATCH);
    srvl_debug_bit(info, "l2_mc", IS2_FKO_L2_MC);
    srvl_debug_bit(info, "l2_bc", IS2_FKO_L2_BC);
    pr("\n");
    srvl_debug_bit(info, "tagged", IS2_FKO_VLAN_TAGGED);
    srvl_debug_u12(info, "vid", IS2_FKO_VID);
    srvl_debug_bit(info, "dei", IS2_FKO_DEI);
    srvl_debug_u3(info, "pcp", IS2_FKO_PCP);
    pr("\n");
    info->data.key_offset = old_offset; /* Restore offset */
}

static void srvl_debug_is2_l3(srvl_debug_info_t *info, u32 offset)
{
    u32 old_offset = info->data.key_offset;

    info->data.key_offset += (offset - IS2_HKO_IP4_OTHER_L3_PROTO); /* Adjust offset */
    srvl_debug_u8(info, "proto", IS2_HKO_IP4_OTHER_L3_PROTO);
    info->pr("\n");
    srvl_debug_bytes(info, "l3_payload",
                     IS2_HKO_IP4_OTHER_L3_PAYLOAD, IS2_HKL_IP4_OTHER_L3_PAYLOAD);
    info->data.key_offset = old_offset; /* Restore offset */
}

static void srvl_debug_is2_l4(srvl_debug_info_t *info, u32 offset)
{
    vtss_debug_printf_t pr = info->pr;
    u32                 old_offset = info->data.key_offset;

    info->data.key_offset += (offset - IS2_HKO_IP4_TCP_UDP_TCP); /* Adjust offset */
    srvl_debug_u16(info, "l4_dport", IS2_HKO_IP4_TCP_UDP_L4_DPORT);
    srvl_debug_u16(info, "l4_sport", IS2_HKO_IP4_TCP_UDP_L4_SPORT);
    srvl_debug_u8(info, "l4_rng", IS2_HKO_IP4_TCP_UDP_L4_RNG);
    pr("\n");
    srvl_debug_bit(info, "tcp", IS2_HKO_IP4_TCP_UDP_TCP);
    srvl_debug_u8(info, "1588_dom", IS2_HKO_IP4_TCP_UDP_L4_1588_DOM);
    srvl_debug_bits(info, "1588_ver", 
                    IS2_HKO_IP4_TCP_UDP_L4_1588_VER, IS2_HKL_IP4_TCP_UDP_L4_1588_VER);
    srvl_debug_bit(info, "sp_eq_dp", IS2_HKO_IP4_TCP_UDP_SPORT_EQ_DPORT);
    pr("\n");
    srvl_debug_bit(info, "seq_eq0", IS2_HKO_IP4_TCP_UDP_SEQUENCE_EQ0);
    srvl_debug_bit(info, "fin", IS2_HKO_IP4_TCP_UDP_L4_FIN);
    srvl_debug_bit(info, "syn", IS2_HKO_IP4_TCP_UDP_L4_SYN);
    srvl_debug_bit(info, "rst", IS2_HKO_IP4_TCP_UDP_L4_RST);
    srvl_debug_bit(info, "psh", IS2_HKO_IP4_TCP_UDP_L4_PSH);
    srvl_debug_bit(info, "ack", IS2_HKO_IP4_TCP_UDP_L4_ACK);
    srvl_debug_bit(info, "urg", IS2_HKO_IP4_TCP_UDP_L4_URG);
    pr("\n");
    info->data.key_offset = old_offset; /* Restore offset */
}

static vtss_rc srvl_debug_is2(srvl_debug_info_t *info)
{
    vtss_debug_printf_t pr = info->pr;
    srvl_tcam_data_t    *data = &info->data;
    u32                 type, x;
    
    if (info->is_action) {
        /* Print action */
        if (data->type == IS2_ACTION_TYPE_SMAC_SIP) {
            srvl_debug_action(info, "cpu", IS2_AO_SMAC_SIP_CPU_COPY_ENA, 
                              IS2_AO_SMAC_SIP_CPU_QU_NUM, IS2_AL_SMAC_SIP_CPU_QU_NUM);
            srvl_debug_action(info, "fwd_kill", IS2_AO_SMAC_SIP_FWD_KILL_ENA, 0, 0);
            srvl_debug_action(info, "host", IS2_AO_SMAC_SIP_HOST_MATCH, 0, 0);
        } else {
            /* IS2_ACTION_TYPE_NORMAL */
            x = srvl_act_bs_get(info, IS2_AO_MASK_MODE, IS2_AL_MASK_MODE);
            pr("mode:%u (%s) ",
               x,
               x == IS2_ACT_MASK_MODE_NONE ? "none" :
               x == IS2_ACT_MASK_MODE_FILTER ? "filter" :
               x == IS2_ACT_MASK_MODE_POLICY ? "policy" :
               x == IS2_ACT_MASK_MODE_REDIR ? "redir" : "?");
            srvl_debug_bits(info, "mask", IS2_AO_PORT_MASK, IS2_AL_PORT_MASK);
            srvl_debug_action(info, "hit", IS2_AO_HIT_ME_ONCE, 0, 0);
            srvl_debug_action(info, "cpu", IS2_AO_CPU_COPY_ENA, 
                              IS2_AO_CPU_QU_NUM, IS2_AL_CPU_QU_NUM);
            srvl_debug_action(info, "mir", IS2_AO_MIRROR_ENA, 0, 0);
            srvl_debug_action(info, "lrn", IS2_AO_LRN_DIS, 0, 0);
            pr("\n");
            srvl_debug_action(info, "pol", IS2_AO_POLICE_ENA, 
                              IS2_AO_POLICE_IDX, IS2_AL_POLICE_IDX);
            srvl_debug_action(info, "pol_vcap", IS2_AO_POLICE_VCAP_ONLY, 0, 0);
            x = srvl_act_bs_get(info, IS2_AO_REW_OP, 3); /* REW_OP[2:0] */
            pr("rew_op:%u (%s) ", 
               x,
               x == IS2_ACT_REW_OP_NONE ? "none" :
               x == IS2_ACT_REW_OP_PTP_ONE ? "ptp_one" :
               x == IS2_ACT_REW_OP_PTP_TWO ? "ptp_two" :
               x == IS2_ACT_REW_OP_PTP_ORG ? "ptp_org" :
               x == IS2_ACT_REW_OP_SPECIAL ? "special" : "?");
            srvl_debug_bits(info, "rew_sub", IS2_AO_REW_OP + 3, 6); /* REW_OP[8:3] */
            srvl_debug_action(info, "isdx", IS2_AO_ISDX_ENA, 0, 0);
            srvl_debug_action(info, "lm_dis", IS2_AO_LM_CNT_DIS, 0, 0);
            srvl_debug_fld(info, "acl_id", IS2_AO_ACL_ID, IS2_AL_ACL_ID);
        }
        pr("cnt:%u ", data->cnt);
        return VTSS_RC_OK;
    }
        
    /* Print entry */
    if (data->tg_sw == VCAP_TG_QUARTER) {
        /* SMAC_SIP4 */
        srvl_debug_bits(info, "igr_port", IS2_QKO_IGR_PORT, IS2_QKL_IGR_PORT);
        srvl_debug_u32(info, "l3_ip4_sip", IS2_QKO_L3_IP4_SIP);
        srvl_debug_u48(info, "l2_smac", IS2_QKO_L2_SMAC);
        return VTSS_RC_OK;
    } else if (data->tg_sw == VCAP_TG_FULL) {
        /* Common format for full keys */
        srvl_debug_bits(info, "type", IS2_FKO_TYPE, IS2_FKL_TYPE);
        type = srvl_entry_bs_get(info, IS2_FKO_TYPE, IS2_FKL_TYPE);
        pr("(%s) ",
           type == IS2_TYPE_IP6_TCP_UDP ? "ip6_tcp_udp" :
           type == IS2_TYPE_IP6_OTHER ? "ip6_other" :
           type == IS2_TYPE_CUSTOM ? "custom" : "?");
        srvl_debug_bit(info, "first", IS2_FKO_FIRST);
        srvl_debug_is2_base(info, IS2_FKO_PAG);

        /* Specific format for full keys */
        switch (type) {
        case IS2_TYPE_IP6_TCP_UDP:
        case IS2_TYPE_IP6_OTHER:
            /* Common format for IP6 keys */
            srvl_debug_bit(info, "ttl",  IS2_FKO_L3_TTL_GT0);
            srvl_debug_u8(info, "l3_tos", IS2_FKO_L3_TOS);
            srvl_debug_bit(info, "dip_eq_sip", IS2_FKO_DIP_EQ_SIP);
            pr("\n");
            srvl_debug_u128(info, "l3_ip6_dip", IS2_FKO_L3_IP6_DIP);
            srvl_debug_u128(info, "l3_ip6_sip", IS2_FKO_L3_IP6_SIP);

            if (type == IS2_TYPE_IP6_TCP_UDP) {
                srvl_debug_is2_l4(info, IS2_FKO_IP6_TCP_UDP_TCP);
            } else {
                srvl_debug_is2_l3(info, IS2_FKO_IP6_OTHER_L3_PROTO);
            }
            break;
        case IS2_TYPE_CUSTOM:
            srvl_debug_bit(info, "custom_type", IS2_FKO_CUSTOM_CUSTOM_TYPE);
            srvl_debug_u128(info, "data", IS2_FKO_CUSTOM_CUSTOM); /* 128 bits for now */
            break;
        default:
            break;
        }
        return VTSS_RC_OK;
    }

    /* VCAP_TG_HALF */
    type = srvl_entry_bs_get(info, IS2_HKO_TYPE, IS2_HKL_TYPE);
    srvl_debug_bits(info, "type", IS2_HKO_TYPE, IS2_HKL_TYPE);
    pr("(%s) ", 
       vtss_bs_get(data->mask, IS2_HKO_TYPE + data->key_offset, IS2_HKL_TYPE) == 0 ? "any" :
       type == IS2_TYPE_ETYPE ? "etype" :
       type == IS2_TYPE_LLC ? "llc" :
       type == IS2_TYPE_SNAP ? "snap" :
       type == IS2_TYPE_ARP ? "arp" :
       type == IS2_TYPE_IP_UDP_TCP ? "udp_tcp" :
       type == IS2_TYPE_IP_OTHER ? "ip_other" :
       type == IS2_TYPE_IPV6 ? "ipv6" :
       type == IS2_TYPE_OAM ? "oam" : 
       type == IS2_TYPE_SMAC_SIP6 ? "smac_sip6" : "?");
    
    if (type == IS2_TYPE_SMAC_SIP6) {
        srvl_debug_bits(info, "igr_port", IS2_HKO_SMAC_SIP6_IGR_PORT, IS2_HKL_SMAC_SIP6_IGR_PORT);
        srvl_debug_u48(info, "l2_smac", IS2_HKO_SMAC_SIP6_L2_SMAC);
        srvl_debug_u128(info, "l3_ip6_sip", IS2_HKO_SMAC_SIP6_L3_IP6_SIP);
        return VTSS_RC_OK;
    }

    /* Common format for half keys */
    srvl_debug_bit(info, "first", IS2_HKO_FIRST);
    srvl_debug_is2_base(info, IS2_HKO_PAG);
    
    switch (type) {
    case IS2_TYPE_ETYPE:
    case IS2_TYPE_LLC:
    case IS2_TYPE_SNAP:
    case IS2_TYPE_OAM:
        /* Common format */
        srvl_debug_u48(info, "l2_dmac", IS2_HKO_L2_DMAC);
        srvl_debug_u48(info, "l2_smac", IS2_HKO_L2_SMAC);

        /* Specific format */
        if (type == IS2_TYPE_ETYPE) {
            srvl_debug_u16(info, "etype", IS2_HKO_MAC_ETYPE_ETYPE);
            srvl_debug_bits(info, "l2_payload", 
                            IS2_HKO_MAC_ETYPE_L2_PAYLOAD, IS2_HKL_MAC_ETYPE_L2_PAYLOAD);
        } else if (type == IS2_TYPE_LLC) {
            srvl_debug_bytes(info, "l2_llc", IS2_HKO_MAC_LLC_L2_LLC, IS2_HKL_MAC_LLC_L2_LLC);
        } else if (type == IS2_TYPE_SNAP) {
            srvl_debug_bytes(info, "l2_snap", IS2_HKO_MAC_SNAP_L2_SNAP, IS2_HKL_MAC_SNAP_L2_SNAP);
        } else if (type == IS2_TYPE_OAM) {
            srvl_debug_bits(info, "mel_flags", 
                            IS2_HKO_OAM_OAM_MEL_FLAGS, IS2_HKL_OAM_OAM_MEL_FLAGS);
            srvl_debug_bits(info, "ver", IS2_HKO_OAM_OAM_VER, IS2_HKL_OAM_OAM_VER);
            srvl_debug_u8(info, "opcode", IS2_HKO_OAM_OAM_OPCODE);
            srvl_debug_u8(info, "flags", IS2_HKO_OAM_OAM_FLAGS);
            pr("\n");
            srvl_debug_u16(info, "mepid", IS2_HKO_OAM_OAM_MEPID);
            srvl_debug_bit(info, "ccm_cnts_eq0", IS2_HKO_OAM_OAM_CCM_CNTS_EQ0);
        }
        break;
    case IS2_TYPE_ARP:
        srvl_debug_u48(info, "l2_smac", IS2_HKO_MAC_ARP_L2_SMAC);
        srvl_debug_bit(info, "addr_ok", IS2_HKO_MAC_ARP_ARP_ADDR_SPACE_OK);
        srvl_debug_bit(info, "proto_ok", IS2_HKO_MAC_ARP_ARP_PROTO_SPACE_OK);
        srvl_debug_bit(info, "len_ok", IS2_HKO_MAC_ARP_ARP_LEN_OK);
        pr("\n");
        srvl_debug_bit(info, "t_match", IS2_HKO_MAC_ARP_ARP_TGT_MATCH);
        srvl_debug_bit(info, "s_match", IS2_HKO_MAC_ARP_ARP_SENDER_MATCH);
        srvl_debug_bit(info, "op_unk", IS2_HKO_MAC_ARP_ARP_OPCODE_UNKNOWN);
        srvl_debug_bits(info, "op", IS2_HKO_MAC_ARP_ARP_OPCODE, IS2_HKL_MAC_ARP_ARP_OPCODE);
        srvl_debug_bit(info, "dip_eq_sip", IS2_HKO_MAC_ARP_DIP_EQ_SIP);
        pr("\n");
        srvl_debug_u32(info, "l3_ip4_dip", IS2_HKO_MAC_ARP_L3_IP4_DIP);
        srvl_debug_u32(info, "l3_ip4_sip", IS2_HKO_MAC_ARP_L3_IP4_SIP);
        break;
    case IS2_TYPE_IP_UDP_TCP:
    case IS2_TYPE_IP_OTHER:
        /* Common format for IP keys */
        srvl_debug_bit(info, "ip4", IS2_HKO_IP4);
        srvl_debug_bit(info, "l3_fragment", IS2_HKO_L3_FRAGMENT);
        srvl_debug_bit(info, "l3_fragoffs", IS2_HKO_L3_FRAG_OFS_GT0);
        srvl_debug_bit(info, "l3_options", IS2_HKO_L3_OPTIONS);
        srvl_debug_bit(info, "l3_ttl", IS2_HKO_L3_TTL_GT0);
        pr("\n");
        srvl_debug_u8(info, "l3_tos", IS2_HKO_L3_TOS);
        srvl_debug_bit(info, "dip_eq_sip", IS2_HKO_DIP_EQ_SIP);
        pr("\n");
        srvl_debug_u32(info, "l3_ip4_dip", IS2_HKO_L3_IP4_DIP);
        srvl_debug_u32(info, "l3_ip4_sip", IS2_HKO_L3_IP4_SIP);
        
        /* Specific format */
        if (type == IS2_TYPE_IP_UDP_TCP) {
            srvl_debug_is2_l4(info, IS2_HKO_IP4_TCP_UDP_TCP);
        } else {
            srvl_debug_is2_l3(info, IS2_HKO_IP4_OTHER_L3_PROTO);
        }
        break;
    case IS2_TYPE_IPV6:
        srvl_debug_u8(info, "proto", IS2_HKO_IP6_STD_L3_PROTO);
        srvl_debug_bit(info, "l3_ttl", IS2_HKO_IP6_STD_L3_TTL_GT0);
        pr("\n");
        srvl_debug_u128(info, "l3_ip6_sip", IS2_HKO_IP6_STD_L3_IP6_SIP);
        break;
    default:
        break;
    }
    return VTSS_RC_OK;
}
    
static vtss_rc srvl_debug_is2_all(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    u32  port;
    BOOL header = 1;

    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        if (vtss_cmn_port2port_no(info, port) == VTSS_PORT_NO_NONE)
            continue;
        if (header)
            srvl_debug_reg_header(pr, "ANA:PORT");
        header = 0;
        srvl_debug_reg_inst(pr, VTSS_ANA_PORT_VCAP_CFG(port), port, "VCAP_CFG");
        srvl_debug_reg_inst(pr, VTSS_ANA_PORT_VCAP_S2_CFG(port), port, "VCAP_S2_CFG");
    }
    if (!header)
        pr("\n");
    return srvl_debug_vcap(VTSS_TCAM_IS2, "IS2", pr, info, srvl_debug_is2);
}

/* ================================================================= *
 *  ES0 functions
 * ================================================================= */

static vtss_rc srvl_es0_entry_get(vtss_vcap_idx_t *idx, u32 *counter, BOOL clear) 
{
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "enter");
    
    return srvl_vcap_counter_get(VTSS_TCAM_ES0, idx, counter, clear);
}

/* (Selection, value) pair */
typedef struct {
    u32 sel;
    u32 val;
} srvl_es0_sel_t;

/* ES0 tag related fields */
typedef struct {
    u32            tag_sel;
    u32            tpid_sel;
    srvl_es0_sel_t vid;
    srvl_es0_sel_t pcp;
    srvl_es0_sel_t dei;
} srvl_es0_tag_t;

static vtss_rc srvl_es0_tag_get(vtss_es0_action_t *action, BOOL inner, srvl_es0_tag_t *tag)
{
    vtss_es0_tag_conf_t *conf;

    if (inner) {
        conf = &action->inner_tag;
        tag->tag_sel = (conf->tag == VTSS_ES0_TAG_NONE ? 0 : 
                        conf->tag == VTSS_ES0_TAG_ES0 ? 1 : 2);
        if (tag->tag_sel > 1) {
            VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal inner_tag");
            return VTSS_RC_ERROR;
        }
    } else {
        conf = &action->outer_tag;
        tag->tag_sel = (conf->tag == VTSS_ES0_TAG_NONE ? ES0_ACT_PUSH_OT_NONE :
                        conf->tag == VTSS_ES0_TAG_ES0 ? ES0_ACT_PUSH_OT_ES0 :
                        conf->tag == VTSS_ES0_TAG_PORT ? ES0_ACT_PUSH_OT_PORT_ENA :
                        ES0_ACT_PUSH_OT_PORT);
        if (tag->tag_sel == ES0_ACT_PUSH_OT_PORT) {
            VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal outer_tag");
            return VTSS_RC_ERROR;
        }
    }
    tag->tpid_sel = conf->tpid;
    tag->vid.sel = (conf->vid.sel ? 1 : 0);
    tag->vid.val = (conf->vid.sel ? conf->vid.val : 0);
    tag->pcp.sel = (conf->pcp.sel == VTSS_ES0_PCP_CLASS ? ES0_ACT_PCP_SEL_CL_PCP :
                    conf->pcp.sel == VTSS_ES0_PCP_ES0 ? ES0_ACT_PCP_SEL_PCP_ES0 :
                    conf->pcp.sel == VTSS_ES0_PCP_MAPPED ? ES0_ACT_PCP_SEL_MAPPED :
                    ES0_ACT_PCP_SEL_QOS);
    tag->pcp.val = conf->pcp.val;
    tag->dei.sel = (conf->dei.sel == VTSS_ES0_DEI_CLASS ? ES0_ACT_DEI_SEL_CL_DEI :
                    conf->dei.sel == VTSS_ES0_DEI_ES0 ? ES0_ACT_DEI_SEL_DEI_ES0 :
                    conf->dei.sel == VTSS_ES0_DEI_MAPPED ? ES0_ACT_DEI_SEL_MAPPED :
                    ES0_ACT_DEI_SEL_DP);
    tag->dei.val = (conf->dei.val ? 1 : 0);
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_es0_entry_add(vtss_vcap_idx_t *idx, vtss_vcap_data_t *vcap_data, u32 counter)
{
    const tcam_props_t *tcam = &tcam_info[VTSS_TCAM_ES0];
    srvl_tcam_data_t   tcam_data, *data = &tcam_data;
    vtss_es0_data_t    *es0 = &vcap_data->u.es0;
    vtss_es0_key_t     *key = &es0->entry->key;
    vtss_es0_action_t  *action = &es0->entry->action;
    u32                port = 0, mask = 0;
    srvl_es0_tag_t     tag;
    BOOL               key_isdx = (key->type == VTSS_ES0_TYPE_ISDX ? 1 : 0);
    
    /* Check key size */
    if (idx->key_size != VTSS_VCAP_KEY_SIZE_FULL) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "unsupported key_size: %s",
                vtss_vcap_key_size_txt(idx->key_size));
        return VTSS_RC_OK;
    }
    
    /* Get TCAM data */
    data->type = 0;
    data->cnt = counter;
    VTSS_RC(srvl_vcap_entry_add(tcam, idx, data));
    
    /* Setup key fields */
    if (key->port_no != VTSS_PORT_NO_NONE) {
        port = VTSS_CHIP_PORT(key->port_no);
        mask = VTSS_BITMASK(ES0_FKL_EGR_PORT);
    }
    srvl_vcap_key_set(data, ES0_FKO_EGR_PORT, ES0_FKL_EGR_PORT, port, mask);
    srvl_vcap_key_set(data, ES0_FKO_IGR_PORT, ES0_FKL_IGR_PORT, 0, 0);
    srvl_vcap_key_bit_set(data, ES0_FKO_SERVICE_FRM, key->isdx_neq0);
    srvl_vcap_key_bit_set(data, ES0_FKO_KEY_ISDX, key_isdx ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_0);
    srvl_vcap_key_bit_set(data, ES0_FKO_L2_MC, VTSS_VCAP_BIT_ANY);
    srvl_vcap_key_bit_set(data, ES0_FKO_L2_BC, VTSS_VCAP_BIT_ANY);
    srvl_vcap_key_set(data, ES0_FKO_VID, ES0_FKL_VID, 
                      key_isdx ? key->data.isdx.isdx : key->data.vid.vid,
                      VTSS_BITMASK(ES0_FKL_VID));
    srvl_vcap_key_bit_set(data, ES0_FKO_DEI, VTSS_VCAP_BIT_ANY);
    srvl_vcap_key_set(data, ES0_FKO_PCP, ES0_FKL_PCP, 
                      key_isdx ? key->data.isdx.pcp.value : key->data.vid.pcp.value,
                      key_isdx ? key->data.isdx.pcp.mask : key->data.vid.pcp.mask);

    /* Setup action fields - outer tag */
    vtss_cmn_es0_action_get(es0);
    VTSS_RC(srvl_es0_tag_get(action, 0, &tag));
    srvl_vcap_action_set(data, ES0_AO_PUSH_OUTER_TAG, ES0_AL_PUSH_OUTER_TAG, tag.tag_sel);
    srvl_vcap_action_set(data, ES0_AO_TAG_A_TPID_SEL, ES0_AL_TAG_A_TPID_SEL, tag.tpid_sel);
    srvl_vcap_action_bit_set(data, ES0_AO_TAG_A_VID_SEL, tag.vid.sel);
    srvl_vcap_action_set(data, ES0_AO_TAG_A_PCP_SEL, ES0_AL_TAG_A_PCP_SEL, tag.pcp.sel);
    srvl_vcap_action_set(data, ES0_AO_TAG_A_DEI_SEL, ES0_AL_TAG_A_DEI_SEL, tag.dei.sel);
    srvl_vcap_action_set(data, ES0_AO_VID_A_VAL, ES0_AL_VID_A_VAL, tag.vid.val);
    srvl_vcap_action_set(data, ES0_AO_PCP_A_VAL, ES0_AL_PCP_A_VAL, tag.pcp.val);
    srvl_vcap_action_bit_set(data, ES0_AO_DEI_A_VAL, tag.dei.val);
    
    /* Setup action fields - inner tag */
    VTSS_RC(srvl_es0_tag_get(action, 1, &tag));
    srvl_vcap_action_bit_set(data, ES0_AO_PUSH_INNER_TAG, tag.tag_sel);
    srvl_vcap_action_set(data, ES0_AO_TAG_B_TPID_SEL, ES0_AL_TAG_B_TPID_SEL, tag.tpid_sel);
    srvl_vcap_action_bit_set(data, ES0_AO_TAG_B_VID_SEL, tag.vid.sel);
    srvl_vcap_action_set(data, ES0_AO_TAG_B_PCP_SEL, ES0_AL_TAG_B_PCP_SEL, tag.pcp.sel);
    srvl_vcap_action_set(data, ES0_AO_TAG_B_DEI_SEL, ES0_AL_TAG_B_DEI_SEL, tag.dei.sel);
    srvl_vcap_action_set(data, ES0_AO_VID_B_VAL, ES0_AL_VID_B_VAL, tag.vid.val);
    srvl_vcap_action_set(data, ES0_AO_PCP_B_VAL, ES0_AL_PCP_B_VAL, tag.pcp.val);
    srvl_vcap_action_bit_set(data, ES0_AO_DEI_B_VAL, tag.dei.val);
    
    /* Setup remaining action fields */
#if defined(VTSS_FEATURE_MPLS)
    srvl_vcap_action_set(data, ES0_AO_ENCAP_ID, ES0_AL_ENCAP_ID, action->mpls_encap_idx);
    srvl_vcap_action_set(data, ES0_AO_ENCAP_LEN, ES0_AL_ENCAP_LEN, action->mpls_encap_len);
#else
    srvl_vcap_action_set(data, ES0_AO_ENCAP_ID, ES0_AL_ENCAP_ID, 0);
    srvl_vcap_action_set(data, ES0_AO_ENCAP_LEN, ES0_AL_ENCAP_LEN, 0);
#endif
    srvl_vcap_action_set(data, ES0_AO_MPOP_CNT, ES0_AL_MPOP_CNT, 0);
    srvl_vcap_action_set(data, ES0_AO_ESDX, ES0_AL_ESDX, action->esdx);
    srvl_vcap_action_bit_set(data, ES0_AO_OAM_MEP_IDX_VLD, action->mep_idx_enable);
    srvl_vcap_action_set(data, ES0_AO_OAM_MEP_IDX, ES0_AL_OAM_MEP_IDX, action->mep_idx);
    srvl_vcap_action_set(data, ES0_AO_IPT_CFG, ES0_AL_IPT_CFG, 0);
    srvl_vcap_action_set(data, ES0_AO_PPT_IDX, ES0_AL_PPT_IDX, 0);

    /* Set TCAM data */
    return srvl_vcap_entry_set(tcam, idx, data);
}

static vtss_rc srvl_es0_entry_del(vtss_vcap_idx_t *idx)
{
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "enter");
    
    return srvl_vcap_entry_del(VTSS_TCAM_ES0, idx);
}

static vtss_rc srvl_es0_entry_move(vtss_vcap_idx_t *idx, u32 count, BOOL up)
{
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "enter");

    return srvl_vcap_entry_move(VTSS_TCAM_ES0, idx, count, up);
}

/* Update outer tag TPID for ES0 entry if VLAN port type has changed */
static vtss_rc srvl_es0_entry_update(vtss_vcap_idx_t *idx, vtss_es0_data_t *es0) 
{
    const tcam_props_t  *tcam = &tcam_info[VTSS_TCAM_ES0];
    srvl_tcam_data_t    tcam_data, *data = &tcam_data;
    vtss_es0_tag_conf_t *tag = &es0->entry->action.outer_tag;
    u8                  pcp, pcp_value = 0, pcp_mask = 0;
    
    VTSS_IG(VTSS_TRACE_GROUP_EVC, "row: %u, col: %u, port_no: %u, flags: 0x%x",
            idx->row, idx->col, es0->port_no, es0->flags);
    VTSS_RC(srvl_vcap_entry_data_get(tcam, idx, data));
    if (es0->flags & VTSS_ES0_FLAG_TPID) {
        srvl_vcap_action_set(data, ES0_AO_TAG_A_TPID_SEL, ES0_AL_TAG_A_TPID_SEL, tag->tpid);
    }
    if (es0->flags & VTSS_ES0_FLAG_QOS) {
        srvl_vcap_action_set(data, ES0_AO_TAG_A_PCP_SEL, ES0_AL_TAG_A_PCP_SEL, tag->pcp.sel);
        srvl_vcap_action_set(data, ES0_AO_TAG_A_DEI_SEL, ES0_AL_TAG_A_DEI_SEL, tag->dei.sel);
        srvl_vcap_action_set(data, ES0_AO_PCP_A_VAL, ES0_AL_PCP_A_VAL, tag->pcp.val);
        srvl_vcap_action_bit_set(data, ES0_AO_DEI_A_VAL, tag->dei.val);
    }
    if (es0->flags & VTSS_ES0_FLAG_PCP_MAP) {
        /* Find PCP value mapping to ECE priority */
        for (pcp = 0; pcp < 8; pcp++) {
            if (vtss_state->qos_port_conf[es0->nni].qos_class_map[pcp][0] == es0->prio) {
                pcp_value = pcp;
                pcp_mask = 0x7;
                break;
            }
        }
        srvl_vcap_key_set(data, ES0_FKO_PCP, ES0_FKL_PCP, pcp_value, pcp_mask);
    }
    VTSS_RC(srvl_vcap_entry_set(tcam, idx, data));

    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_es0(srvl_debug_info_t *info)
{
    vtss_debug_printf_t pr = info->pr;
    u32                 i, x, offs;
    char                buf[16], buf_1[16];
    
    if (info->is_action) {
        /* Print action */
        x = srvl_act_bs_get(info, ES0_AO_PUSH_OUTER_TAG, ES0_AL_PUSH_OUTER_TAG);
        pr("outer:%u (%s) ", x,
           x == ES0_ACT_PUSH_OT_PORT_ENA ? "port_ena" :
           x == ES0_ACT_PUSH_OT_ES0 ? "es0" :
           x == ES0_ACT_PUSH_OT_PORT ? "port" :
           x == ES0_ACT_PUSH_OT_NONE ? "none" : "?");
        srvl_debug_bit(info, "inner", ES0_AO_PUSH_INNER_TAG);
        pr("\n");
        
        /* Loop over TAG_A/TAG_B fields */
        for (i = 0; i < 2; i++) {
            sprintf(buf, "_%s", i ? "b" : "a");
            offs = (i ? (ES0_AO_TAG_B_TPID_SEL - ES0_AO_TAG_A_TPID_SEL) : 0);
            x = srvl_act_bs_get(info, ES0_AO_TAG_A_TPID_SEL + offs, ES0_AL_TAG_A_TPID_SEL);
            pr("tpid%s:%u (%s) ", buf, x,
               x == TAG_TPID_CFG_0x8100 ? "c" :
               x == TAG_TPID_CFG_0x88A8 ? "s" :
               x == TAG_TPID_CFG_PTPID ? "port" :
               x == TAG_TPID_CFG_PTPID_NC ? "port-c" : "?");

            x = srvl_act_bs_get(info, ES0_AO_TAG_A_VID_SEL + offs, ES0_AL_TAG_A_VID_SEL);
            pr("vid%s:%u (%svid%s_val) ", buf, x, x ? "" : "cl_vid+", buf);

            x = srvl_act_bs_get(info, ES0_AO_TAG_A_PCP_SEL + offs, ES0_AL_TAG_A_PCP_SEL);
            pr("pcp%s:%u (%s) ", buf, x,
               x == ES0_ACT_PCP_SEL_CL_PCP ? "cl_pcp" :
               x == ES0_ACT_PCP_SEL_PCP_ES0 ? "pcp_es0" :
               x == ES0_ACT_PCP_SEL_MAPPED ? "mapped" :
               x == ES0_ACT_PCP_SEL_QOS ? "qos" : "?");

            x = srvl_act_bs_get(info, ES0_AO_TAG_A_DEI_SEL + offs, ES0_AL_TAG_A_DEI_SEL);
            pr("dei%s:%u (%s)\n", buf, x,
               x == ES0_ACT_DEI_SEL_CL_DEI ? "cl_dei" :
               x == ES0_ACT_DEI_SEL_DEI_ES0 ? "dei_es0" :
               x == ES0_ACT_DEI_SEL_MAPPED ? "mapped" :
               x == ES0_ACT_DEI_SEL_DP ? "dp" : "?");
            
            offs = (i ? (ES0_AO_VID_B_VAL - ES0_AO_VID_A_VAL) : 0);
            sprintf(buf_1, "_%s_val", i ? "b" : "a");
            sprintf(buf, "vid%s", buf_1);
            srvl_debug_fld(info, buf, ES0_AO_VID_A_VAL + offs, ES0_AL_VID_A_VAL);
            sprintf(buf, "pcp%s", buf_1);
            srvl_debug_fld(info, buf, ES0_AO_PCP_A_VAL + offs, ES0_AL_PCP_A_VAL);
            sprintf(buf, "dei%s", buf_1);
            srvl_debug_fld(info, buf, ES0_AO_DEI_A_VAL + offs, ES0_AL_DEI_A_VAL);
            pr("\n");
        }
        srvl_debug_fld(info, "encap_id", ES0_AO_ENCAP_ID, ES0_AL_ENCAP_ID);
        srvl_debug_fld(info, "encap_len", ES0_AO_ENCAP_LEN, ES0_AL_ENCAP_LEN);
        srvl_debug_fld(info, "mpop_cnt", ES0_AO_MPOP_CNT, ES0_AL_MPOP_CNT);
        srvl_debug_fld(info, "esdx", ES0_AO_ESDX, ES0_AL_ESDX);
        srvl_debug_action(info, "oam_mep_idx", ES0_AO_OAM_MEP_IDX_VLD,
                          ES0_AO_OAM_MEP_IDX, ES0_AL_OAM_MEP_IDX);
        x = srvl_act_bs_get(info, ES0_AO_IPT_CFG, ES0_AL_IPT_CFG);
        pr("\n");
        pr("ipt_cfg:%u (%s) ", x, 
           x == ES0_ACT_IPT_CFG_NONE ? "none" :
           x == ES0_ACT_IPT_CFG_WORKING ? "working" :
           x == ES0_ACT_IPT_CFG_PROTECTION ? "protection" : "?");
        pr("hit:%u ", info->data.cnt);
        return VTSS_RC_OK;
    }

    /* Print entry */
    srvl_debug_bits(info, "egr_port", ES0_FKO_EGR_PORT, ES0_FKL_EGR_PORT);
    srvl_debug_bits(info, "igr_port", ES0_FKO_IGR_PORT, ES0_FKL_IGR_PORT);
    pr("\n");
    srvl_debug_bit(info, "service_frm", ES0_FKO_SERVICE_FRM);
    srvl_debug_bit(info, "key_isdx", ES0_FKO_KEY_ISDX);
    srvl_debug_bit(info, "l2_mc", ES0_FKO_L2_MC);
    srvl_debug_bit(info, "l2_bc", ES0_FKO_L2_BC);
    srvl_debug_u12(info, "vid", ES0_FKO_VID);
    srvl_debug_bit(info, "dei", ES0_FKO_DEI);
    srvl_debug_u3(info, "pcp", ES0_FKO_PCP);
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_es0_all(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    u32  port;
    BOOL header = 1;

    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        if (vtss_cmn_port2port_no(info, port) == VTSS_PORT_NO_NONE)
            continue;
        if (header)
            srvl_debug_reg_header(pr, "REW:PORT");
        header = 0;
        srvl_debug_reg_inst(pr, VTSS_REW_PORT_PORT_CFG(port), port, "PORT_CFG");
    }
    if (!header)
        pr("\n");
    return srvl_debug_vcap(VTSS_TCAM_ES0, "ES0", pr, info, srvl_debug_es0);
}

#if defined (VTSS_FEATURE_MPLS)
/* ================================================================= *
 *  MPLS
 * ================================================================= */

/*
 * Internal MPLS utilities
 */

#define VTSS_MPLS_CHECK(chk)    do { if (!(chk)) { VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Check failed: " #chk); return VTSS_RC_ERROR; } } while (0)
#define VTSS_MPLS_CHECK_RC(chk) do { if (VTSS_RC_OK != (chk)) { VTSS_DG(VTSS_TRACE_GROUP_MPLS, "RC check failed: " #chk); return VTSS_RC_ERROR; } } while (0)

#define SEG_I(idx) vtss_state->mpls_segment_conf[idx]
#define XC_I(idx)  vtss_state->mpls_xc_conf[idx]
#define VP_I(idx)  vtss_state->mpls_vprofile_conf[idx]
#define L2_I(idx)  vtss_state->mpls_l2_conf[idx]

#define SEG_P(idx) SEG_I(idx).pub
#define XC_P(idx)  XC_I(idx).pub
#define VP_P(idx)  VP_I(idx).pub
#define L2_P(idx)  L2_I(idx).pub

#define VTSS_MPLS_TAKE_FROM_FREELIST(freelist, table, idx) \
    do { \
        (idx) = vtss_state->freelist; \
        if (VTSS_MPLS_IDX_IS_UNDEF((idx))) { \
            VTSS_DG(VTSS_TRACE_GROUP_MPLS, "No free " #table " entries"); \
            VTSS_MPLS_IDX_UNDEF((idx)); \
            return VTSS_RC_ERROR; \
        } \
        vtss_state->freelist = vtss_state->table[idx].next_free; \
        VTSS_MPLS_IDX_UNDEF(vtss_state->table[idx].next_free); \
    } while (0)

#define VTSS_MPLS_RETURN_TO_FREELIST(freelist, table, idx) \
    vtss_state->table[idx].next_free = vtss_state->freelist; \
    vtss_state->freelist = (idx);

#if 0
// Not used yet, so disabled due to lint
static BOOL srvl_mpls_hw_avail_mll(u32 cnt)
{
    return (vtss_state->mpls_is0_mll_cnt + cnt <= VTSS_MPLS_L2_CNT)  &&
           (vtss_state->mpls_is0_mll_cnt + cnt + vtss_state->mpls_is0_mlbs_cnt / 2 <= SRVL_IS0_CNT);
}

static BOOL srvl_mpls_hw_avail_mlbs(u32 cnt)
{
    return (vtss_state->mpls_is0_mlbs_cnt + cnt <= VTSS_MPLS_LSP_CNT)  &&
           (vtss_state->mpls_is0_mll_cnt + (vtss_state->mpls_is0_mlbs_cnt + cnt) / 2 <= SRVL_IS0_CNT);
}

static BOOL srvl_mpls_hw_avail_encap(u32 cnt)
{
    return (vtss_state->mpls_encap_cnt + cnt) <= VTSS_MPLS_OUT_ENCAP_CNT;
}

static BOOL srvl_mpls_hw_avail_vprofile(u32 cnt)
{
    return (vtss_state->mpls_vprofile_cnt + cnt) <= VTSS_MPLS_VPROFILE_CNT;
}
#endif

static vtss_rc srvl_mpls_sdx_alloc(BOOL isdx, vtss_port_no_t port_no, u32 cnt, vtss_sdx_entry_t **first)
{
    *first = vtss_cmn_sdx_alloc(port_no, isdx);
    if (*first) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "SDX alloc success: %d entries starting at %d", cnt, (*first)->sdx);
        return VTSS_RC_OK;
    }
    else {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "SDX alloc FAILURE: %d entries", cnt);
        return VTSS_RC_ERROR;
    }
}

static vtss_rc srvl_mpls_sdx_free(BOOL isdx, u32 cnt, vtss_sdx_entry_t *first)
{
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "SDX free: %d entries starting at %d", cnt, first->sdx);
    vtss_cmn_sdx_free(first, isdx);
    return VTSS_RC_OK;
}

/* ----------------------------------------------------------------- *
 *  VProfile functions (with regards to MPLS processing)
 * ----------------------------------------------------------------- */

static vtss_rc srvl_mpls_vprofile_hw_update(vtss_mpls_vprofile_idx_t idx)
{
    u32                  v, m;
    vtss_mpls_vprofile_t *vp;

    if (idx >= VTSS_MPLS_VPROFILE_CNT) {
        return VTSS_RC_ERROR;
    }

    vp = &VP_P(idx);

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "VProfile update, entry %d (port %d)", idx, vp->port);

    SRVL_WRM(VTSS_ANA_PORT_VCAP_CFG(idx),    vp->s1_s2_enable ? VTSS_F_ANA_PORT_VCAP_CFG_S1_ENA    : 0, VTSS_F_ANA_PORT_VCAP_CFG_S1_ENA);
    SRVL_WRM(VTSS_ANA_PORT_VCAP_S2_CFG(idx), vp->s1_s2_enable ? VTSS_F_ANA_PORT_VCAP_S2_CFG_S2_ENA : 0, VTSS_F_ANA_PORT_VCAP_S2_CFG_S2_ENA);

    v = (vp->recv_enable       ? VTSS_F_ANA_PORT_PORT_CFG_RECV_ENA  : 0)      |
        (vp->learn_enable      ? VTSS_F_ANA_PORT_PORT_CFG_LEARN_ENA : 0)      |
        (vp->src_mirror_enable ? VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA : 0) |
        VTSS_F_ANA_PORT_PORT_CFG_PORTID_VAL(VTSS_CHIP_PORT(vp->port));
    m = VTSS_F_ANA_PORT_PORT_CFG_RECV_ENA       |
        VTSS_F_ANA_PORT_PORT_CFG_LEARN_ENA      |
        VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA |
        VTSS_M_ANA_PORT_PORT_CFG_PORTID_VAL;
    SRVL_WRM(VTSS_ANA_PORT_PORT_CFG(idx), v, m);

    SRVL_WRM(VTSS_ANA_PORT_VLAN_CFG(idx), vp->vlan_aware ? VTSS_F_ANA_PORT_VLAN_CFG_VLAN_AWARE_ENA : 0, VTSS_F_ANA_PORT_VLAN_CFG_VLAN_AWARE_ENA);

    v = (vp->map_dscp_cos_enable ? VTSS_F_ANA_PORT_QOS_CFG_QOS_DSCP_ENA : 0) |
        (vp->map_eth_cos_enable  ? VTSS_F_ANA_PORT_QOS_CFG_QOS_PCP_ENA  : 0);
    m = VTSS_F_ANA_PORT_QOS_CFG_QOS_DSCP_ENA |
        VTSS_F_ANA_PORT_QOS_CFG_QOS_PCP_ENA;
    SRVL_WRM(VTSS_ANA_PORT_QOS_CFG(idx), v, m);

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_vprofile_init(void)
{
    vtss_mpls_vprofile_idx_t i;
    vtss_mpls_vprofile_t     *vp;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Attempting static VProfile config for LSR (entry %u) and OAM (entry %u)", VTSS_MPLS_VPROFILE_LSR_IDX, VTSS_MPLS_VPROFILE_OAM_IDX);

    // TN1135/MPLS, sect. 3.3.3, 3.3.4: Static LSR, OAM entries:

    vp = &VP_P(VTSS_MPLS_VPROFILE_LSR_IDX);
    vp->s1_s2_enable        = FALSE;
    vp->recv_enable         = TRUE;
    vp->ptp_dly1_enable     = FALSE;
    vp->vlan_aware          = FALSE;
    vp->map_dscp_cos_enable = FALSE;
    vp->map_eth_cos_enable  = FALSE;
    (void) srvl_mpls_vprofile_hw_update(VTSS_MPLS_VPROFILE_LSR_IDX);

    vp = &VP_P(VTSS_MPLS_VPROFILE_OAM_IDX);
    vp->s1_s2_enable        = FALSE;
    vp->recv_enable         = FALSE;
    vp->ptp_dly1_enable     = FALSE;
    vp->vlan_aware          = FALSE;
    vp->map_dscp_cos_enable = FALSE;
    vp->map_eth_cos_enable  = FALSE;
    (void) srvl_mpls_vprofile_hw_update(VTSS_MPLS_VPROFILE_OAM_IDX);

    // Set up VProfile free-list. The reserved entries, of course, aren't in:
    for (i = VTSS_MPLS_VPROFILE_RESERVED_CNT; i < VTSS_MPLS_VPROFILE_CNT - 1; i++) {
        vtss_state->mpls_vprofile_conf[i].next_free = i + 1;
    }
    vtss_state->mpls_vprofile_conf[VTSS_MPLS_VPROFILE_CNT - 1].next_free = VTSS_MPLS_IDX_UNDEFINED;
    vtss_state->mpls_vprofile_free_list = VTSS_MPLS_VPROFILE_RESERVED_CNT;

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_vprofile_alloc(vtss_mpls_vprofile_idx_t * const idx)
{
    VTSS_MPLS_TAKE_FROM_FREELIST(mpls_vprofile_free_list, mpls_vprofile_conf, *idx);
    vtss_state->mpls_vprofile_cnt++;
    VTSS_MPLS_CHECK(vtss_state->mpls_vprofile_cnt <= (VTSS_MPLS_VPROFILE_CNT - VTSS_MPLS_VPROFILE_RESERVED_CNT));

    memset(&vtss_state->mpls_vprofile_conf[*idx].pub, 0, sizeof(vtss_state->mpls_vprofile_conf[*idx].pub));

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Allocated VProfile entry %d (%d allocated)", *idx, vtss_state->mpls_vprofile_cnt);
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_vprofile_free(const vtss_mpls_vprofile_idx_t idx)
{
    if (idx < VTSS_MPLS_VPROFILE_RESERVED_CNT) {
        return VTSS_RC_ERROR;
    }
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Freeing VProfile entry %d (%d allocated afterwards)", idx, vtss_state->mpls_vprofile_cnt - 1);
    VTSS_MPLS_RETURN_TO_FREELIST(mpls_vprofile_free_list, mpls_vprofile_conf, idx);
    VTSS_MPLS_CHECK(vtss_state->mpls_vprofile_cnt > 0);
    vtss_state->mpls_vprofile_cnt--;
    return VTSS_RC_OK;
}

/* ----------------------------------------------------------------- *
 *  MPLS Out-segment (Egress) Encapsulation
 * ----------------------------------------------------------------- */

/* The MPLS egress encapsulation data consists of 320 bits; the first 48 bits
 * contain remarking information, and the subsequent bits consist of the
 * MLL + optional MLBS configuration prepended to the payload.
 */

static inline void srvl_mpls_out_encap_set_bit(u32 *bits, u32 offset, u8 val)
{
    u8 *data = (u8*)bits;
    u8 shift = offset % 8;
    data += (320-offset-1)/8;
    *data = (*data & ~VTSS_BIT(shift)) | (val ? VTSS_BIT(shift) : 0);
}

static void srvl_mpls_out_encap_set_bits(u32 *bits, u32 offset, u32 width, u32 value)
{
    while (width > 0) {
        srvl_mpls_out_encap_set_bit(bits, offset, value & 0x01);
        width--;
        offset++;
        value >>= 1;
    }
}

static void srvl_mpls_out_encap_set_mac(u32 *bits, u32 offset, const vtss_mac_t *mac)
{
    int i;
    for (i = 5; i >= 0; i--) {
        srvl_mpls_out_encap_set_bits(bits, offset, 8, mac->addr[i]);
        offset += 8;
    }
}

static u32 srvl_ntohl(u32 n)
{
#ifndef VTSS_OS_BIG_ENDIAN
    n = ((n & 0x000000ff) << 24) |
        ((n & 0x0000ff00) <<  8) |
        ((n & 0x00ff0000) >>  8) |
        ((n & 0xff000000) >> 24);
#endif
    return n;
}

typedef struct {
    struct {
        BOOL use_cls_vid;
        BOOL use_cls_dp;
        BOOL use_cls_pcp;
        BOOL use_cls_dei;
        struct {
            BOOL use_ttl;
            BOOL use_s_bit;
            BOOL use_qos_to_tc_map;
            u8   qos_to_tc_map_idx;
        } label[VTSS_MPLS_OUT_ENCAP_LABEL_CNT];
    } remark;
    vtss_mac_t                   dmac;
    vtss_mac_t                   smac;
    vtss_mll_tagtype_t           tag_type;
    vtss_vid_t                   vid;                   /**< C or B VID */
    vtss_tagprio_t               pcp;                   /**< PCP value */
    vtss_dei_t                   dei;                   /**< DEI value */
    vtss_mll_ethertype_t         ether_type;
    u8                           label_stack_depth;     /**< Range [0..VTSS_MPLS_ENCAP_LABEL_CNT] */
    vtss_mpls_label_t            label_stack[VTSS_MPLS_OUT_ENCAP_LABEL_CNT];
    BOOL                         use_cw;
    u32                          cw;
} srvl_mpls_out_encap_t;

static vtss_rc srvl_mpls_out_encap_set(u32 idx, const srvl_mpls_out_encap_t *const entry, u32 *length)
{
    const int word_cnt = 320/32;

    u32  bits[word_cnt];    // MSB is bit 31 in [0]
    int  i;
    u32  offset;
    u32  offset_base;

    if (idx == 0  ||  idx >= VTSS_MPLS_OUT_ENCAP_CNT) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Invalid MPLS out-segment encap idx (%u)", idx);
        return VTSS_RC_ERROR;
    }

    *length = 0;

    memset(bits, 0, sizeof(bits));

#define SET_BOOL(offset, boolean)      srvl_mpls_out_encap_set_bit(bits, (offset), (boolean) ? 1 : 0)
#define SET_INT(offset, width, value)  srvl_mpls_out_encap_set_bits(bits, (offset), (width), (value))
#define SET_INT_O(width, value)        offset -= (width); srvl_mpls_out_encap_set_bits(bits, offset, (width), (value))

    SET_BOOL(298, entry->remark.use_cls_vid);
    SET_BOOL(297, entry->remark.use_cls_dp);
    SET_BOOL(296, entry->remark.use_cls_pcp);
    SET_BOOL(295, entry->remark.use_cls_dei);
    SET_BOOL(294, entry->tag_type != VTSS_MPLS_TAGTYPE_UNTAGGED);
    for (i = 0; i < 3; i++) {
        SET_INT (272 + i*4, 3, entry->remark.label[i].qos_to_tc_map_idx);
        SET_BOOL(275 + i*4,    entry->remark.label[i].use_qos_to_tc_map);
        SET_BOOL(288 + i,      entry->remark.label[i].use_s_bit);
        SET_BOOL(291 + i,      entry->remark.label[i].use_ttl);
    }

    offset_base = offset = 272;

    offset -= 6*8;
    srvl_mpls_out_encap_set_mac(bits, offset, &entry->dmac);
    offset -= 6*8;
    srvl_mpls_out_encap_set_mac(bits, offset, &entry->smac);

    switch (entry->tag_type) {
    case VTSS_MPLS_TAGTYPE_UNTAGGED:
        break;
    case VTSS_MPLS_TAGTYPE_CTAGGED:
        SET_INT_O(16, 0x8100);
        SET_INT_O(3, entry->pcp);
        SET_INT_O(1, entry->dei);
        SET_INT_O(12, entry->vid);
        break;
    case VTSS_MPLS_TAGTYPE_STAGGED:
        SET_INT_O(16, 0x88A8);              // TBD: Use global value if a such is configured?
        SET_INT_O(3, entry->pcp);
        SET_INT_O(1, entry->dei);
        SET_INT_O(12, entry->vid);
        break;
    }

    if (entry->ether_type == VTSS_MLL_ETHERTYPE_DOWNSTREAM_ASSIGNED) {
        SET_INT_O(16, 0x8847);
    }
    else {
        SET_INT_O(16, 0x8848);
    }

    for (i = 0; i < (int)entry->label_stack_depth; i++) {
        SET_INT_O(20, entry->label_stack[i].value);
        SET_INT_O(3,  entry->label_stack[i].tc);
        SET_INT_O(1,  (i == (int)entry->label_stack_depth - 1) ? 1 : 0);
        SET_INT_O(8,  entry->label_stack[i].ttl);
    }
    if (entry->use_cw) {
        SET_INT_O(32, entry->cw);
    }

    *length = (offset_base - offset + 7) / 8;

#undef SET_BOOL
#undef SET_INT
#undef SET_INT_O

    for (i = 0; i < word_cnt; i++) {
        u32 val = srvl_ntohl(bits[i]);
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "%2d   0x%08x   0x%08x", i, val, bits[i]);
        SRVL_WR(VTSS_SYS_ENCAPSULATIONS_ENCAP_DATA(i), val);
    }

    SRVL_WR(VTSS_SYS_ENCAPSULATIONS_ENCAP_CTRL, VTSS_F_SYS_ENCAPSULATIONS_ENCAP_CTRL_ENCAP_ID(idx) | VTSS_F_SYS_ENCAPSULATIONS_ENCAP_CTRL_ENCAP_WR);

    return VTSS_RC_OK;
}

/* ----------------------------------------------------------------- *
 *  Segment utilities
 * ----------------------------------------------------------------- */

static vtss_rc srvl_mpls_segment_state_get(const vtss_mpls_segment_idx_t     idx,
                                           vtss_mpls_segment_state_t * const state)
{
#define SET(x) *state = VTSS_MPLS_SEGMENT_STATE_##x
#define SET_UP(tst) do { if (!seg->need_hw_alloc && (tst)) SET(UP); } while (0)

    vtss_mpls_segment_internal_t *seg;
    vtss_mpls_xc_internal_t      *xc;
    BOOL                         has_l2, has_xc, has_isdx, has_srv, has_hw;
    BOOL                         is_lsr, is_pw, srv_up, has_esdx;
    vtss_mpls_segment_state_t    srv_state;

    SET(UNCONF);

    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);

    seg      = &SEG_I(idx);
    has_l2   = VTSS_MPLS_IDX_IS_DEF(seg->pub.l2_idx);
    has_xc   = VTSS_MPLS_IDX_IS_DEF(seg->pub.xc_idx);
    has_srv  = VTSS_MPLS_IDX_IS_DEF(seg->pub.server_idx);
    has_hw   = FALSE;
    srv_up   = FALSE;

    /* In all of the below, we cannot enter UP when seg->need_hw_alloc == TRUE.
     *
     * In-segment with L2:
     *
     *     UNCONF  = !XC || !LABEL
     *     CONF    = !UNCONF
     *     UP[LSR] = CONF && xc.ISDX && MLL && MLBS
     *     UP[LER] = if seg.PW
     *                   CONF && xc.ISDX && MLL && MLBS
     *               else
     *                   CONF
     *
     * In-segment without L2:
     *
     *     UNCONF  = !XC || !LABEL
     *     CONF    = !UNCONF  &&  has_server
     *     UP[LSR] = CONF && xc.ISDX && MLL && MLBS && server.UP
     *     UP[LER] = if seg.PW
     *                   CONF && xc.ISDX && MLL && MLBS && server.UP
     *               else
     *                   CONF
     *
     * Out-segment with L2:
     *
     *     UNCONF  = !XC || !LABEL || !TTL
     *     CONF    = !UNCONF
     *     UP[LSR] = CONF && xc.ISDX && ESDX && ENCAP && ES0
     *     UP[LER] = if seg.PW
     *                   CONF && ENCAP
     *               else
     *                   CONF
     *
     * Out-segment without L2:
     *
     *     UNCONF  = !XC || !LABEL || !TTL
     *     CONF    = !UNCONF  &&  has_server
     *     UP[LSR] = CONF && xc.ISDX && ESDX && ENCAP && ES0 && server.UP
     *     UP[LER] = if seg.PW
     *                   CONF && ENCAP && server.UP
     *               else
     *                   CONF && server.UP
     *
     */

    if (!has_xc  ||  (seg->pub.label.value == VTSS_MPLS_LABEL_VALUE_DONTCARE)  ||
        (!seg->pub.is_in  &&  (seg->pub.label.ttl == 0))) {
        *state = VTSS_MPLS_SEGMENT_STATE_UNCONF;
        return VTSS_RC_OK;
    }

    xc       = &XC_I(seg->pub.xc_idx);
    has_isdx = xc->isdx != NULL;
    is_lsr   = xc->pub.type == VTSS_MPLS_XC_TYPE_LSR;
    is_pw    = xc->pub.type == VTSS_MPLS_XC_TYPE_LER  &&  seg->pub.pw_conf.is_pw;

    if (seg->pub.is_in) {
        if (has_l2) {
            SET(CONF);
            if (is_lsr  ||  is_pw) {
                has_hw = has_isdx  &&  seg->u.in.has_mll  &&  seg->u.in.has_mlbs;
                SET_UP(has_hw);
            }
            else {
                SET(UP);
            }
        }
        else {
            if (has_srv) {
                SET(CONF);
                (void) srvl_mpls_segment_state_get(seg->pub.server_idx, &srv_state);
                srv_up = srv_state == VTSS_MPLS_SEGMENT_STATE_UP;
                if (is_lsr  ||  is_pw) {
                    has_hw = has_isdx  &&  seg->u.in.has_mlbs;
                    SET_UP(has_hw && srv_up);
                }
                else {
                    SET(UP);
                }
            }
        }
    }
    else {  // Out-segment
        if (has_l2) {
            SET(CONF);
            srv_up = TRUE;   // Or so we pretend.
        }
        else if (has_srv) {
            SET(CONF);
            (void) srvl_mpls_segment_state_get(seg->pub.server_idx, &srv_state);
            srv_up = srv_state == VTSS_MPLS_SEGMENT_STATE_UP;
        }
        if (is_lsr) {
            has_esdx = seg->u.out.esdx != NULL;
            has_hw = has_isdx  &&  has_esdx  &&  seg->u.out.has_encap  &&  seg->u.out.has_es0;
            SET_UP(has_hw  && srv_up);
        }
        else if (is_pw) {
            SET_UP(seg->u.out.has_encap && srv_up);
        }
        else {
            SET_UP(srv_up);
        }
    }

    return VTSS_RC_OK;
#undef SET
#undef SET_UP
}

static const char *vtss_mpls_segment_state_to_str(const vtss_mpls_segment_state_t s)
{
    switch (s) {
    case VTSS_MPLS_SEGMENT_STATE_UNCONF:
        return "UNCONF";
    case VTSS_MPLS_SEGMENT_STATE_CONF:
        return "CONF";
    case VTSS_MPLS_SEGMENT_STATE_UP:
        return "UP";
    default:
        return "(unknown)";
    }
}

static vtss_mpls_segment_idx_t srvl_mpls_find_ultimate_server(vtss_mpls_segment_idx_t seg, u8 *depth)
{
    *depth = 1;
    if (VTSS_MPLS_IDX_IS_UNDEF(seg)) {
        VTSS_EG(VTSS_TRACE_GROUP_MPLS, "Segment idx is undefined");
    }
    else {
        while (VTSS_MPLS_IDX_IS_DEF(SEG_P(seg).server_idx)) {
            seg = SEG_P(seg).server_idx;
            (*depth)++;
        }
        if (*depth > VTSS_MPLS_IN_ENCAP_LABEL_CNT) {
            VTSS_EG(VTSS_TRACE_GROUP_MPLS, "Label stack is too deep");
        }
    }
    return seg;
}

static vtss_port_no_t srvl_mpls_port_no_get(vtss_mpls_segment_idx_t idx)
{
    u8                      depth;
    vtss_mpls_segment_idx_t ultimate_server = srvl_mpls_find_ultimate_server(idx, &depth);
    vtss_mpls_l2_idx_t      l2_idx          = SEG_P(ultimate_server).l2_idx;
    return VTSS_MPLS_IDX_IS_DEF(l2_idx) ? L2_P(l2_idx).port : 0;
}

/* ----------------------------------------------------------------- *
 *  IS0 MLL entries
 * ----------------------------------------------------------------- */

static vtss_rc srvl_mpls_is0_mll_update(vtss_mpls_l2_internal_t *l2, BOOL upstream);

static vtss_rc srvl_mpls_is0_mll_alloc(vtss_mpls_l2_internal_t *l2, BOOL upstream)
{
    i16                       *ll_idx = upstream ? &l2->ll_idx_upstream : &l2->ll_idx_downstream;
    vtss_mpls_idxchain_iter_t dummy;
    vtss_mpls_idxchain_user_t user;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Allocating new MLL %sstream entry id", upstream ? "up" : "down");
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(*ll_idx));

    VTSS_MPLS_CHECK(vtss_mpls_idxchain_get_first(&vtss_state->mpls_is0_mll_free_chain, &dummy, &user));
    VTSS_MPLS_CHECK_RC(vtss_mpls_idxchain_remove_first(&vtss_state->mpls_is0_mll_free_chain, user));
    *ll_idx = user;

    vtss_state->mpls_is0_mll_cnt++;
    VTSS_MPLS_CHECK(vtss_state->mpls_is0_mll_cnt <= VTSS_MPLS_L2_CNT  &&
                    vtss_state->mpls_is0_mll_cnt + vtss_state->mpls_is0_mlbs_cnt / 2 <= SRVL_IS0_CNT);
    
    return srvl_mpls_is0_mll_update(l2, upstream);
}

static vtss_rc srvl_mpls_is0_mll_free(vtss_mpls_l2_internal_t *l2, BOOL upstream)
{
    vtss_rc rc;
    i16 *ll_idx = upstream ? &l2->ll_idx_upstream : &l2->ll_idx_downstream;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Removing IS0 MLL entry %d", *ll_idx);
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(*ll_idx));

    // Remove VCAP entry and put its index back on the free chain
    rc = vtss_vcap_del(&vtss_state->is0.obj, VTSS_IS0_USER_MPLS_LL, *ll_idx);
    VTSS_MPLS_CHECK_RC(vtss_mpls_idxchain_insert_at_head(&vtss_state->mpls_is0_mll_free_chain, *ll_idx));
    VTSS_MPLS_IDX_UNDEF(*ll_idx);

    VTSS_MPLS_CHECK(vtss_state->mpls_is0_mll_cnt > 0);
    vtss_state->mpls_is0_mll_cnt--;

    return rc;
}

static vtss_rc srvl_mpls_is0_mll_update(vtss_mpls_l2_internal_t *l2, BOOL upstream)
{
    vtss_rc                   rc;
    vtss_is0_entry_t          entry;
    vtss_vcap_data_t          data;
    i16                       ll_idx = upstream ? l2->ll_idx_upstream : l2->ll_idx_downstream;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Updating MLL entry id %d", ll_idx);
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(ll_idx));

    memset(&entry, 0, sizeof(entry));

    entry.type = VTSS_IS0_TYPE_MLL;

    entry.key.mll.ingress_port = l2->pub.port;
    entry.key.mll.tag_type     = l2->pub.tag_type;
    entry.key.mll.b_vid        = l2->pub.vid;
    entry.key.mll.ether_type   = upstream ? VTSS_MLL_ETHERTYPE_UPSTREAM_ASSIGNED : VTSS_MLL_ETHERTYPE_DOWNSTREAM_ASSIGNED;
    memcpy(&entry.key.mll.smac, &l2->pub.peer_mac, sizeof(entry.key.mll.smac));
    memcpy(&entry.key.mll.dmac, &l2->pub.self_mac, sizeof(entry.key.mll.dmac));

    entry.action.mll.linklayer_index = ll_idx;
    entry.action.mll.mpls_forwarding = TRUE;

    data.key_size    = VTSS_VCAP_KEY_SIZE_FULL;
    data.u.is0.entry = &entry;

    rc = vtss_vcap_add(&vtss_state->is0.obj, VTSS_IS0_USER_MPLS_LL, ll_idx,
                       VTSS_VCAP_ID_LAST, &data, FALSE);
    
    if (rc != VTSS_RC_OK) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "MLL entry update FAILED, rc = %d. Attempting to delete entry.", rc);
        VTSS_MPLS_CHECK_RC(srvl_mpls_is0_mll_free(l2, upstream));
    }

    return rc;
 }



/* ----------------------------------------------------------------- *
 *  IS0 MLBS entries
 * ----------------------------------------------------------------- */

static vtss_vcap_user_t srvl_mpls_depth_to_is0_user(u8 depth)
{
#if (VTSS_MPLS_IN_ENCAP_LABEL_CNT != 3)
#error "This code assumes VTSS_MPLS_IN_ENCAP_LABEL_CNT == 3"
#endif
    switch (depth) {
    case 1:  return VTSS_IS0_USER_MPLS_MLBS_1;
    case 2:  return VTSS_IS0_USER_MPLS_MLBS_2;
    case 3:  return VTSS_IS0_USER_MPLS_MLBS_3;
    default:
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Label stack too deep: %d", depth);
        return VTSS_IS0_USER_MPLS_MLBS_1;
    }
}

static vtss_rc srvl_mpls_is0_mlbs_free(vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_internal_t *seg = &SEG_I(idx);
    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_CHECK(seg->pub.is_in);

    if (seg->u.in.has_mlbs) {
        u8 depth;

        (void) srvl_mpls_find_ultimate_server(idx, &depth);

        // Remove VCAP entry
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Removing IS0 MLBS entry %d", idx);
        VTSS_MPLS_CHECK_RC(vtss_vcap_del(&vtss_state->is0.obj, srvl_mpls_depth_to_is0_user(depth), idx));
        seg->u.in.has_mlbs = FALSE;

        VTSS_MPLS_CHECK(vtss_state->mpls_is0_mlbs_cnt > 0);
        vtss_state->mpls_is0_mlbs_cnt--;
    }
    return VTSS_RC_OK;
}

/* Update IS0 MLBS entry for in-segment. If this segment is itself a server then
 * the clients must be updated as well, but that's outside the scope of this
 * function.
 */
static vtss_rc srvl_mpls_is0_mlbs_update(vtss_mpls_segment_idx_t idx)
{
#if (VTSS_MPLS_IN_ENCAP_LABEL_CNT != 3)
#error "This code assumes VTSS_MPLS_IN_ENCAP_LABEL_CNT == 3"
#endif
    vtss_mpls_segment_internal_t *seg        = &SEG_I(idx);
    vtss_mpls_segment_internal_t *srv1       = VTSS_MPLS_IDX_IS_DEF(seg->pub.server_idx) ? &SEG_I(seg->pub.server_idx) : NULL;
    vtss_mpls_segment_internal_t *srv2       = srv1 && VTSS_MPLS_IDX_IS_DEF(srv1->pub.server_idx) ? &SEG_I(srv1->pub.server_idx) : NULL;
    vtss_mpls_l2_internal_t      *l2         = NULL;
    vtss_mpls_segment_idx_t      seg_with_l2 = idx;      // Just an assumption as this point
    u8                           depth       = 0;
    u8                           d;
    vtss_mpls_xc_internal_t      *xc;
    i16                          i;
    vtss_rc                      rc;
    vtss_is0_entry_t             entry;
    vtss_vcap_data_t             data;
    BOOL                         already_has_mlbs = seg->u.in.has_mlbs;

    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry, segment %d", idx);

    VTSS_MPLS_IDX_CHECK_XC(seg->pub.xc_idx);
    xc = &XC_I(seg->pub.xc_idx);

    VTSS_MPLS_CHECK(xc->isdx);

    memset(&entry, 0, sizeof(entry));
    entry.type = VTSS_IS0_TYPE_MLBS;

    // Configure label stack for MLBS key

    entry.action.mlbs.pop_count = VTSS_IS0_MLBS_POPCOUNT_18;  // DMAC, SMAC, eth-type, 1 label

    if (srv1) {
        seg_with_l2 = seg->pub.server_idx;
        entry.action.mlbs.pop_count++;           // One more label
        if (srv2) {
            seg_with_l2 = srv1->pub.server_idx;
            entry.action.mlbs.pop_count++;       // One more label
            entry.key.mlbs.label_stack[depth].value = srv2->pub.label.value;
            entry.key.mlbs.label_stack[depth].tc    = srv2->pub.label.tc;
            depth++;
        }
        entry.key.mlbs.label_stack[depth].value = srv1->pub.label.value;
        entry.key.mlbs.label_stack[depth].tc    = srv1->pub.label.tc;
        depth++;
    }
    entry.key.mlbs.label_stack[depth].value = seg->pub.label.value;
    entry.key.mlbs.label_stack[depth].tc    = seg->pub.label.tc;
    depth++;

    // Don't-care remainder of MLBS key

    for (d = depth; d < VTSS_MPLS_IN_ENCAP_LABEL_CNT; d++) {
        entry.key.mlbs.label_stack[d].value = VTSS_MPLS_LABEL_VALUE_DONTCARE;
        entry.key.mlbs.label_stack[d].tc    = VTSS_MPLS_TC_VALUE_DONTCARE;
    }

    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(SEG_P(seg_with_l2).l2_idx));
    l2 = &L2_I(SEG_P(seg_with_l2).l2_idx);

    // Pop VID if L2 has it

    entry.action.mlbs.pop_count += (l2->pub.vid != 0) ? 1 : 0;

    // Get MLL:LL_IDX

    i = seg->pub.upstream ? l2->ll_idx_upstream : l2->ll_idx_downstream;
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(i));
    entry.key.mlbs.linklayer_index = i;

    // Choose label for swap, tc, ttl.
    // FIXME: More to come
    entry.action.mlbs.tc_label_index          = depth - 1;
    entry.action.mlbs.ttl_label_index         = depth - 1;
    entry.action.mlbs.swap_label_index        = depth - 1;

    // VProfile, classification output setup
    entry.action.mlbs.vprofile_index     = seg->u.in.vprofile_idx;
    entry.action.mlbs.use_service_config = TRUE;
    entry.action.mlbs.classified_vid     = 0;
    entry.action.mlbs.s_tag              = 0;
    entry.action.mlbs.pcp                = 0;
    entry.action.mlbs.dei                = 0;

    switch (xc->pub.type) {
    case VTSS_MPLS_XC_TYPE_LER:
        entry.action.mlbs.cw_enable    = seg->pub.pw_conf.process_cw;
        entry.action.mlbs.terminate_pw = TRUE;
        entry.action.mlbs.pop_count    += seg->pub.pw_conf.process_cw ? 1 : 0;
        break;
    case VTSS_MPLS_XC_TYPE_LSR:
        break;
    }

    entry.action.mlbs.isdx              = xc->isdx->sdx;
    entry.action.mlbs.e_lsp             = seg->pub.e_lsp;
    entry.action.mlbs.add_tc_to_isdx    = FALSE;            // Depends on statistics: Per service or per COS per service
    entry.action.mlbs.tc_maptable_index = VTSS_MPLS_IDX_IS_DEF(seg->pub.tc_qos_map_idx) ? seg->pub.tc_qos_map_idx : 0;
    entry.action.mlbs.l_lsp_qos_class   = seg->pub.l_lsp_cos;

#if 0
    /* Fields with default zero. Just kept here for reference */
    entry.action.mlbs.b_portlist
    entry.action.mlbs.cpu_queue = ;
    entry.action.mlbs.oam = ;
    entry.action.mlbs.oam_buried_mip = ;
    entry.action.mlbs.oam_reserved_label_value = ;
    entry.action.mlbs.oam_reserved_label_bottom_of_stack = ;
    entry.action.mlbs.oam_isdx = ;
    entry.action.mlbs.oam_isdx_add_replace = ;
    entry.action.mlbs.classified_vid = ;
    entry.action.mlbs.s_tag = ;
    entry.action.mlbs.pcp = ;
    entry.action.mlbs.dei = ;
#endif

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Updating MLBS %d: ISDX %d, label stack  %d/%d  %d/%d  %d/%d",
             idx, xc->isdx->sdx,
             entry.key.mlbs.label_stack[0].value,
             entry.key.mlbs.label_stack[0].tc,
             entry.key.mlbs.label_stack[1].value,
             entry.key.mlbs.label_stack[1].tc,
             entry.key.mlbs.label_stack[2].value,
             entry.key.mlbs.label_stack[2].tc);

    data.key_size    = VTSS_VCAP_KEY_SIZE_HALF;
    data.u.is0.entry = &entry;

    rc = vtss_vcap_add(&vtss_state->is0.obj, srvl_mpls_depth_to_is0_user(depth),
                       idx, VTSS_VCAP_ID_LAST,
                       &data, FALSE);

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "MLBS Done, rc = %d", rc);

    seg->u.in.has_mlbs = (rc == VTSS_RC_OK);

    if (!already_has_mlbs  &&  seg->u.in.has_mlbs) {
        vtss_state->mpls_is0_mlbs_cnt++;
        VTSS_MPLS_CHECK(vtss_state->mpls_is0_mlbs_cnt <= VTSS_MPLS_LSP_CNT &&
                        vtss_state->mpls_is0_mll_cnt + vtss_state->mpls_is0_mlbs_cnt / 2 <= SRVL_IS0_CNT);
    }

    return rc;
}

static vtss_rc srvl_mpls_is0_mlbs_tear_all_recursive(vtss_mpls_segment_idx_t idx, u8 depth)
{
    vtss_mpls_idxchain_iter_t    iter;
    vtss_mpls_idxchain_user_t    user;
    BOOL                         more;
    vtss_mpls_segment_internal_t *seg;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry, segment %d, depth = %u", idx, depth);
    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);
    VTSS_MPLS_CHECK(depth <= VTSS_MPLS_IN_ENCAP_LABEL_CNT);

    seg = &SEG_I(idx);
    VTSS_MPLS_CHECK_RC(srvl_mpls_is0_mlbs_free(idx));

    more = vtss_mpls_idxchain_get_first(&seg->pub.clients, &iter, &user);
    while (more) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_is0_mlbs_tear_all_recursive(user, depth + 1));
        more = vtss_mpls_idxchain_get_next(&iter, &user);
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_is0_mlbs_tear_all(vtss_mpls_segment_idx_t idx)
{
    u8 depth;

    (void) srvl_mpls_find_ultimate_server(idx, &depth);
    return srvl_mpls_is0_mlbs_tear_all_recursive(idx, depth);
}



/* ----------------------------------------------------------------- *
 * MPLS Egress encapsulation entries
 * ----------------------------------------------------------------- */

static vtss_rc srvl_mpls_encap_update(vtss_mpls_segment_idx_t idx);

static vtss_rc srvl_mpls_encap_alloc(vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_internal_t *seg = &SEG_I(idx);
    vtss_mpls_idxchain_iter_t    dummy;
    vtss_mpls_idxchain_user_t    user;
    
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->u.out.encap_idx));

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Allocating new MPLS encap entry id for out-seg %d", idx);
    VTSS_MPLS_CHECK(vtss_mpls_idxchain_get_first(&vtss_state->mpls_encap_free_chain, &dummy, &user));
    VTSS_MPLS_CHECK_RC(vtss_mpls_idxchain_remove_first(&vtss_state->mpls_encap_free_chain, user));
    vtss_state->mpls_encap_cnt++;
    VTSS_MPLS_CHECK(vtss_state->mpls_encap_cnt <= VTSS_MPLS_OUT_ENCAP_CNT);
    seg->u.out.encap_idx = user;

    return srvl_mpls_encap_update(idx);
}

static vtss_rc srvl_mpls_encap_free(vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_internal_t *seg = &SEG_I(idx);
    
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Removing encap entry %d", seg->u.out.encap_idx);
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(seg->u.out.encap_idx));
    VTSS_MPLS_CHECK(seg->u.out.has_encap);

    // Put index back on the free chain
    VTSS_MPLS_CHECK_RC(vtss_mpls_idxchain_insert_at_head(&vtss_state->mpls_encap_free_chain, seg->u.out.encap_idx));
    VTSS_MPLS_IDX_UNDEF(seg->u.out.encap_idx);
    seg->u.out.has_encap = FALSE;

    VTSS_MPLS_CHECK(vtss_state->mpls_encap_cnt > 0);
    vtss_state->mpls_encap_cnt--;

    /* There may still be an ES0 entry that uses this encap; the caller must
     * clean it up
     */

    return VTSS_RC_OK;
}

/* Fill in segment pointer array. First item must be outer-most label, so we
 * use recursion down the server list to get to the ultimate server before
 * beginning to fill in the array.
 */
static void srvl_mpls_encap_make_seg_array(vtss_mpls_segment_idx_t idx,
                                           vtss_mpls_segment_internal_t *ppseg[],
                                           i8 *depth,
                                           i8 *i)
{
    vtss_mpls_segment_internal_t *s = &SEG_I(idx);
    if (*depth > VTSS_MPLS_IN_ENCAP_LABEL_CNT) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Recursion level too deep");
        return;
    }
    if (VTSS_MPLS_IDX_IS_DEF(s->pub.server_idx)) {
        (*depth)++;
        srvl_mpls_encap_make_seg_array(s->pub.server_idx, ppseg, depth, i);
    }
    ppseg[(*i)++] = s;
}

static vtss_rc srvl_mpls_encap_update(vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_internal_t *seg[VTSS_MPLS_IN_ENCAP_LABEL_CNT] = { 0 };
    vtss_mpls_xc_internal_t      *xc [VTSS_MPLS_IN_ENCAP_LABEL_CNT] = { 0 };
    vtss_mpls_l2_internal_t      *l2;
    i8                           i;
    i8                           depth;
    vtss_rc                      rc;
    srvl_mpls_out_encap_t        encap;
    u32                          length;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "MPLS encap table setup of idx %d for segment %d", SEG_I(idx).u.out.encap_idx, idx);
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(SEG_I(idx).u.out.encap_idx));

    depth = 1;
    i = 0;
    srvl_mpls_encap_make_seg_array(idx, seg, &depth, &i);
    
    memset(&encap, 0, sizeof(encap));
    seg[depth-1]->u.out.encap_bytes = 0;

    l2 = &L2_I(seg[0]->pub.l2_idx);

    for (i = 0; i < depth; i++) {
        xc[i]                = &XC_I(seg[i]->pub.xc_idx);
        encap.label_stack[i] = seg[i]->pub.label;
    }
    encap.label_stack_depth = depth;

    // For LSR, set innermost label's S-bit, TC and TTL from incoming frame's
    // classified values.
    //
    // For MPLS LER, S-bit is set for the innermost label; TC and TTL are
    // taken from the tunneled label in a way depending on the DiffServ
    // tunneling mode:
    //   Pipe + Short Pipe = push new TC / TTL
    //   Uniform           = use inner (tunneled) TC / TTL in tunnel label

    switch (xc[depth-1]->pub.type) {
    case VTSS_MPLS_XC_TYPE_LSR:
        encap.remark.label[depth-1].use_qos_to_tc_map = TRUE;
        encap.remark.label[depth-1].qos_to_tc_map_idx = VTSS_MPLS_IDX_IS_DEF(seg[depth-1]->pub.tc_qos_map_idx) ? seg[depth-1]->pub.tc_qos_map_idx : 0;  // 0 == Reserved entry; 1:1 QoS => TC
        encap.remark.label[depth-1].use_ttl           = TRUE;
        encap.remark.label[depth-1].use_s_bit         = TRUE;

        // Handle TC/TTL usage for Uniform mode for the (LER) tunnel hierarchy (if any)
        for (i = depth - 2; i >= 0; i--) {
            if(xc[i]->pub.tc_tunnel_mode == VTSS_MPLS_TUNNEL_MODE_UNIFORM) {
                encap.remark.label[i].use_qos_to_tc_map = encap.remark.label[i + 1].use_qos_to_tc_map;
                encap.remark.label[i].qos_to_tc_map_idx = encap.remark.label[i + 1].qos_to_tc_map_idx & 0x07;
                encap.label_stack[i].tc                 = encap.label_stack[i + 1].tc;
            }
            else {
                encap.remark.label[i].use_qos_to_tc_map = FALSE;
            }
            if(xc[i]->pub.ttl_tunnel_mode == VTSS_MPLS_TUNNEL_MODE_UNIFORM) {
                encap.remark.label[i].use_ttl = encap.remark.label[i + 1].use_ttl;
                encap.label_stack[i].ttl      = encap.label_stack[i + 1].ttl;
            }
        }

        for (i = depth-1; i >= 0; i--) {
            VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Level %d, Lbl %d/%d/%d, qos2tc %s/%d, use TTL %s, use S %s", i, encap.label_stack[i].value, encap.label_stack[i].tc, encap.label_stack[i].ttl,
            encap.remark.label[i].use_qos_to_tc_map ? "Y":"N", encap.remark.label[i].qos_to_tc_map_idx, encap.remark.label[i].use_ttl?"Y":"N", encap.remark.label[i].use_s_bit?"Y":"N");
        }
        break;
    case VTSS_MPLS_XC_TYPE_LER:
        break;
    }

    memcpy(&encap.dmac, &l2->pub.peer_mac, sizeof(encap.dmac));
    memcpy(&encap.smac, &l2->pub.self_mac, sizeof(encap.smac));

    encap.tag_type   = l2->pub.tag_type;
    encap.vid        = l2->pub.vid;
    encap.pcp        = l2->pub.pcp;
    encap.dei        = l2->pub.dei;
    encap.ether_type = seg[depth-1]->pub.upstream ? VTSS_MLL_ETHERTYPE_UPSTREAM_ASSIGNED : VTSS_MLL_ETHERTYPE_DOWNSTREAM_ASSIGNED;
    encap.use_cw     = seg[depth-1]->pub.pw_conf.process_cw;
    encap.cw         = seg[depth-1]->pub.pw_conf.cw;

    rc = srvl_mpls_out_encap_set(seg[depth-1]->u.out.encap_idx, &encap, &length);
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Encap entry done, %u bytes, rc=%d", length, rc);

    seg[depth-1]->u.out.encap_bytes = length & 0xffff;
    seg[depth-1]->u.out.has_encap = TRUE;

    if (rc != VTSS_RC_OK) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Encap entry update FAILED, rc = %d. Attempting to delete entry.", rc);
        VTSS_MPLS_CHECK_RC(srvl_mpls_encap_free(idx));
    }
    
    return rc;
}



/* ----------------------------------------------------------------- *
 * ES0 entries
 * ----------------------------------------------------------------- */

static vtss_rc srvl_isdx_update_es0(BOOL isdx_ena, u32 isdx, u32 isdx_mask);

static vtss_rc srvl_mpls_es0_free(vtss_mpls_segment_idx_t idx)
{
    vtss_rc                      rc;
    vtss_mpls_segment_internal_t *seg = &SEG_I(idx);
    vtss_mpls_xc_internal_t      *xc  = &XC_I(seg->pub.xc_idx);

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Removing ES0 entry %d", idx);
    VTSS_MPLS_CHECK(seg->u.out.has_es0);
    rc = vtss_vcap_del(&vtss_state->es0.obj, VTSS_ES0_USER_MPLS, idx);
    VTSS_MPLS_CHECK_RC(srvl_isdx_update_es0(FALSE, xc->isdx->sdx, 0));
    seg->u.out.has_es0 = FALSE;
    return rc;
}

static vtss_es0_mpls_encap_len_t srvl_bytes_to_encap_len(u16 bytes)
{
    switch (bytes) {
    case 0:
        return VTSS_ES0_MPLS_ENCAP_LEN_NONE;
    case 14:
        return VTSS_ES0_MPLS_ENCAP_LEN_14;
    case 18:
        return VTSS_ES0_MPLS_ENCAP_LEN_18;
    case 22:
        return VTSS_ES0_MPLS_ENCAP_LEN_22;
    case 26:
        return VTSS_ES0_MPLS_ENCAP_LEN_26;
    case 30:
        return VTSS_ES0_MPLS_ENCAP_LEN_30;
    case 34:
        return VTSS_ES0_MPLS_ENCAP_LEN_34;
    default:
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Unmatched encap byte count: %u", bytes);
        return VTSS_ES0_MPLS_ENCAP_LEN_NONE;
    }    
}

static vtss_rc srvl_mpls_es0_update(vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_internal_t *seg = &SEG_I(idx);
    vtss_mpls_xc_internal_t      *xc  = &XC_I(seg->pub.xc_idx);
    vtss_rc                      rc;
    vtss_es0_entry_t             entry;
    vtss_vcap_data_t             data;
    u32                          isdx_mask;
    vtss_port_no_t               port;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry, segment %d", idx);
    VTSS_MPLS_CHECK(!seg->pub.is_in);
    VTSS_MPLS_CHECK(seg->u.out.esdx != NULL);
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(seg->u.out.encap_idx));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(seg->pub.xc_idx));

    port = srvl_mpls_port_no_get(idx);

    memset(&entry, 0, sizeof(entry));

    entry.key.port_no          = port;
    entry.key.type             = VTSS_ES0_TYPE_ISDX;
    entry.key.isdx_neq0        = VTSS_VCAP_BIT_1;
    entry.key.data.isdx.pcp.value = 0;  // FIXME
    entry.key.data.isdx.pcp.mask  = 0;  // FIXME -- wildcard

    entry.key.data.isdx.isdx = xc->isdx->sdx;

    entry.action.mpls_encap_idx = seg->u.out.encap_idx;
    entry.action.mpls_encap_len = srvl_bytes_to_encap_len(seg->u.out.encap_bytes);
    entry.action.esdx           = seg->u.out.esdx->sdx;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Attempting ES0 config, entry %d, ISDX %d, ESDX %d, encap %d",
            idx, xc->isdx->sdx, seg->u.out.esdx->sdx, seg->u.out.encap_idx);

    data.key_size    = VTSS_VCAP_KEY_SIZE_FULL;
    data.u.es0.flags = 0;
    data.u.es0.entry = &entry;

    rc = vtss_vcap_add(&vtss_state->es0.obj, VTSS_ES0_USER_MPLS,
                       idx, VTSS_VCAP_ID_LAST, &data, FALSE);

    seg->u.out.has_es0 = TRUE;

    isdx_mask = VTSS_BIT(VTSS_CHIP_PORT(port));
    VTSS_MPLS_CHECK_RC(srvl_isdx_update_es0(TRUE, xc->isdx->sdx, isdx_mask));
    
    if (rc != VTSS_RC_OK) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "ES0 update FAILED, rc = %d. Attempting to delete entry.", rc);
        VTSS_MPLS_CHECK_RC(srvl_mpls_es0_free(idx));           // Clears seg->u.out.has_es0
    }

    return rc;
}



/* ----------------------------------------------------------------- *
 * Teardown of both ES0 and MPLS encapsulation entries
 * ----------------------------------------------------------------- */

static vtss_rc srvl_mpls_es0_encap_tear_all_recursive(vtss_mpls_segment_idx_t idx, u8 depth)
{
    vtss_mpls_idxchain_iter_t    iter;
    vtss_mpls_idxchain_user_t    user;
    BOOL                         more;
    vtss_mpls_segment_internal_t *seg;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry, segment %d, depth = %u", idx, depth);
    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);
    VTSS_MPLS_CHECK(depth <= VTSS_MPLS_OUT_ENCAP_LABEL_CNT);

    seg = &SEG_I(idx);
    if (seg->u.out.has_es0) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_es0_free(idx));
    }
    if (seg->u.out.has_encap) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_encap_free(idx));
    }
    (void) srvl_mpls_sdx_free(FALSE, 1, seg->u.out.esdx);

    more = vtss_mpls_idxchain_get_first(&seg->pub.clients, &iter, &user);
    while (more) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_es0_encap_tear_all_recursive(user, depth + 1));
        more = vtss_mpls_idxchain_get_next(&iter, &user);
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_es0_encap_tear_all(vtss_mpls_segment_idx_t idx)
{
    u8 depth;

    (void)srvl_mpls_find_ultimate_server(idx, &depth);
    return srvl_mpls_es0_encap_tear_all_recursive(idx, depth);
}



/* ----------------------------------------------------------------- *
 *  Global TC config
 * ----------------------------------------------------------------- */

static vtss_rc srvl_mpls_tc_conf_set(const vtss_mpls_tc_conf_t * const new_map)
{
    u8  map, qos, tc;
    u32 dp0, dp1;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry");

    // QOS => TC: We never update entry 0; it's a static 1-1 mapping necessary
    // to carry ingress TC to egress on Serval-1.
    for (map = 1; map < VTSS_MPLS_QOS_TO_TC_MAP_CNT; map++) {
        for (qos = 0; qos < VTSS_MPLS_QOS_TO_TC_ENTRY_CNT; qos++) {
            vtss_state->mpls_tc_conf.qos_to_tc_map[map][qos].dp0_tc = new_map->qos_to_tc_map[map][qos].dp0_tc & 0x07;
            vtss_state->mpls_tc_conf.qos_to_tc_map[map][qos].dp1_tc = new_map->qos_to_tc_map[map][qos].dp1_tc & 0x07;
        }
    }

    // Commit each map to chip (including map 0)
    for (map = 0; map < VTSS_MPLS_QOS_TO_TC_MAP_CNT; map++) {
        dp0 = dp1 = 0;
        for (qos = VTSS_MPLS_QOS_TO_TC_MAP_CNT; qos > 0; qos--) {
            dp0 = (dp0 << 3) | vtss_state->mpls_tc_conf.qos_to_tc_map[map][qos - 1].dp0_tc;
            dp1 = (dp1 << 3) | vtss_state->mpls_tc_conf.qos_to_tc_map[map][qos - 1].dp1_tc;
        }
        SRVL_WR(VTSS_SYS_SYSTEM_MPLS_QOS_MAP_CFG(map),     dp0);
        SRVL_WR(VTSS_SYS_SYSTEM_MPLS_QOS_MAP_CFG(map + 8), dp1);
    }

    // TC => QOS: Here we start at zero
    for (map = 0; map < VTSS_MPLS_TC_TO_QOS_MAP_CNT; map++) {
        for (tc = 0; tc < VTSS_MPLS_TC_TO_QOS_ENTRY_CNT; tc++) {
            vtss_state->mpls_tc_conf.tc_to_qos_map[map][tc].qos = new_map->tc_to_qos_map[map][tc].qos & 0x07;
            vtss_state->mpls_tc_conf.tc_to_qos_map[map][tc].dp  = new_map->tc_to_qos_map[map][tc].dp  & 0x01;
            SRVL_WR(VTSS_ANA_MPLS_TC_MAP_TC_MAP_TBL(map, tc),
                    vtss_state->mpls_tc_conf.tc_to_qos_map[map][tc].qos << 1 |
                    vtss_state->mpls_tc_conf.tc_to_qos_map[map][tc].dp);
        }
    }

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "exit");

    return VTSS_RC_OK;
}


/* ----------------------------------------------------------------- *
 *  L2 entries
 * ----------------------------------------------------------------- */

static vtss_rc srvl_mpls_segment_hw_update(vtss_mpls_segment_idx_t idx);

static BOOL vtss_mpls_l2_equal(const vtss_mpls_l2_t * const a,
                               const vtss_mpls_l2_t * const b)
{
    return
        (a->port == b->port) &&
        (a->ring_port == b->ring_port) &&
        (a->tag_type == b->tag_type) &&
        (a->vid == b->vid) &&
        (memcmp(a->peer_mac.addr, b->peer_mac.addr, sizeof(a->peer_mac.addr)) == 0) &&
        (memcmp(a->self_mac.addr, b->self_mac.addr, sizeof(a->self_mac.addr)) == 0);
}

static vtss_rc srvl_mpls_l2_alloc(vtss_mpls_l2_idx_t * const idx)
{
    vtss_mpls_l2_internal_t *l2;
    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_TAKE_FROM_FREELIST(mpls_l2_free_list, mpls_l2_conf, *idx);
    l2 = &L2_I(*idx);
    memset(&l2->pub, 0, sizeof(l2->pub));
    VTSS_MPLS_IDXCHAIN_UNDEF(l2->pub.users);
    VTSS_MPLS_IDX_UNDEF(l2->ll_idx_upstream);
    VTSS_MPLS_IDX_UNDEF(l2->ll_idx_downstream);
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_l2_free(vtss_mpls_l2_idx_t idx)
{
    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_IDX_CHECK_L2(idx);

    // No segments must be using this index
    VTSS_MPLS_CHECK(VTSS_MPLS_IDXCHAIN_END(L2_P(idx).users));

    VTSS_MPLS_RETURN_TO_FREELIST(mpls_l2_free_list, mpls_l2_conf, idx);
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_l2_set(const vtss_mpls_l2_idx_t     idx,
                                const vtss_mpls_l2_t * const l2)
{
    vtss_mpls_l2_internal_t   *l2_i;
    vtss_mpls_idxchain_iter_t iter;
    vtss_mpls_idxchain_user_t user;
    BOOL                      more;

    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_IDX_CHECK_L2(idx);
    l2_i = &L2_I(idx);

    if (!vtss_mpls_l2_equal(l2, &l2_i->pub)) {
        memcpy(&l2_i->pub, l2, sizeof(l2_i->pub));

        if (VTSS_MPLS_IDX_IS_DEF(l2_i->ll_idx_upstream)) {
            VTSS_MPLS_CHECK_RC(srvl_mpls_is0_mll_update(l2_i, TRUE));
        }
        if (VTSS_MPLS_IDX_IS_DEF(l2_i->ll_idx_downstream)) {
            VTSS_MPLS_CHECK_RC(srvl_mpls_is0_mll_update(l2_i, FALSE));
        }
    }

    // In-segment users are easy; they don't need to know about the change;
    // out-segment users need to rebuild their encapsulations and ES0
    // entries

    more = vtss_mpls_idxchain_get_first(&l2_i->pub.users, &iter, &user);
    while (more) {
        (void) srvl_mpls_segment_hw_update(user);
        more = vtss_mpls_idxchain_get_next(&iter, &user);
    }

    return VTSS_RC_OK;
}

// Return: VTSS_RC_OK if L2 entry's HW allocation is OK
static vtss_rc srvl_mpls_l2_hw_alloc(const vtss_mpls_l2_idx_t      l2_idx,
                                     const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_rc                      rc   = VTSS_RC_OK;
    vtss_mpls_l2_internal_t      *l2  = &L2_I(l2_idx);
    vtss_mpls_segment_internal_t *seg = &SEG_I(seg_idx);

    if (seg->pub.is_in) {
        if ( ( seg->pub.upstream  &&  VTSS_MPLS_IDX_IS_UNDEF(l2->ll_idx_upstream))  ||
             (!seg->pub.upstream  &&  VTSS_MPLS_IDX_IS_UNDEF(l2->ll_idx_downstream)) ) {
            rc = srvl_mpls_is0_mll_alloc(l2, seg->pub.upstream);
        }
        seg->u.in.has_mll = (rc == VTSS_RC_OK);
    }
    
    return rc;
}


/* As soon as an in-segment is attached, an MLL will be allocated (if HW so
 * permits).
 */
static vtss_rc srvl_mpls_l2_segment_attach(const vtss_mpls_l2_idx_t      l2_idx,
                                           const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_mpls_l2_internal_t      *l2;
    vtss_mpls_segment_internal_t *seg;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry");

    // Rules:
    //   - Segment must not already be attached to an L2 entry
    //   - Segment must not be a client

    VTSS_MPLS_IDX_CHECK_L2(l2_idx);
    VTSS_MPLS_IDX_CHECK_SEGMENT(seg_idx);

    l2  = &L2_I(l2_idx);
    seg = &SEG_I(seg_idx);
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.l2_idx));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.server_idx));
    VTSS_MPLS_CHECK(!seg->u.in.has_mll);    // Consistency check

    if (vtss_mpls_idxchain_insert_at_head(&l2->pub.users, seg_idx) != VTSS_RC_OK) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Couldn't insert segment %u in L2 idx %u user chain", seg_idx, l2_idx);
        return VTSS_RC_ERROR;
    }
    seg->pub.l2_idx = l2_idx;

    VTSS_MPLS_CHECK_RC(srvl_mpls_l2_hw_alloc(l2_idx, seg_idx));

    if (++vtss_state->mpls_port_state[l2->pub.port].l2_refcnt == 1) {
        // First segment attached -- make the port an LSR port and enable MPLS
        SRVL_WRM(VTSS_ANA_PORT_PORT_CFG(VTSS_CHIP_PORT(l2->pub.port)),
                 VTSS_F_ANA_PORT_PORT_CFG_LSR_MODE, VTSS_F_ANA_PORT_PORT_CFG_LSR_MODE);
        SRVL_WRM(VTSS_SYS_SYSTEM_PORT_MODE(VTSS_CHIP_PORT(l2->pub.port)),
                 VTSS_F_SYS_SYSTEM_PORT_MODE_MPLS_ENA, VTSS_F_SYS_SYSTEM_PORT_MODE_MPLS_ENA);
    }

    return srvl_mpls_segment_hw_update(seg_idx);
}

static vtss_rc srvl_mpls_l2_segment_detach(const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_mpls_l2_idx_t           l2_idx;
    vtss_mpls_l2_internal_t      *l2;
    vtss_mpls_segment_internal_t *seg;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry");

    // Rules:
    //   - Segment must be attached to an L2 entry

    VTSS_MPLS_IDX_CHECK_SEGMENT(seg_idx);
    seg    = &SEG_I(seg_idx);
    l2_idx = seg->pub.l2_idx;
    VTSS_MPLS_IDX_CHECK_L2(l2_idx);
    l2     = &L2_I(l2_idx);

    if (vtss_mpls_idxchain_remove_first(&l2->pub.users, seg_idx) != VTSS_RC_OK) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Couldn't remove segment %u from L2 idx %u user chain", seg_idx, l2_idx);
        return VTSS_RC_ERROR;
    }
    VTSS_MPLS_IDX_UNDEF(seg->pub.l2_idx);
    VTSS_MPLS_CHECK(vtss_state->mpls_port_state[l2->pub.port].l2_refcnt > 0);
    if (--vtss_state->mpls_port_state[l2->pub.port].l2_refcnt == 0) {
        // Last segment detached --  remove LSR and MPLS state from port
        SRVL_WRM(VTSS_ANA_PORT_PORT_CFG(VTSS_CHIP_PORT(l2->pub.port)),
                 0, VTSS_F_ANA_PORT_PORT_CFG_LSR_MODE);
        SRVL_WRM(VTSS_SYS_SYSTEM_PORT_MODE(VTSS_CHIP_PORT(l2->pub.port)),
                 0, VTSS_F_SYS_SYSTEM_PORT_MODE_MPLS_ENA);
    }


    if (seg->pub.is_in) {
        seg->u.in.has_mll = FALSE;
    }
    
    (void) srvl_mpls_segment_hw_update(seg_idx);
    
    if (seg->pub.is_in) {
        // If no other in-segment users exist with the same upstream/downstream
        // setting as the just-removed segment, free the HW resource

        vtss_mpls_idxchain_iter_t    iter;
        vtss_mpls_idxchain_user_t    user;
        BOOL                         more;
        BOOL                         found_one = FALSE;
        
        more = vtss_mpls_idxchain_get_first(&l2->pub.users, &iter, &user);
        while (more  &&  !found_one) {
            found_one = SEG_P(user).is_in  &&  SEG_P(user).upstream == seg->pub.upstream;
            more      = vtss_mpls_idxchain_get_next(&iter, &user);
        }
        if (! found_one) {
            VTSS_MPLS_CHECK_RC(srvl_mpls_is0_mll_free(l2, seg->pub.upstream));
        }
    }

    return VTSS_RC_OK;
}



/* ----------------------------------------------------------------- *
 *  MPLS Segments
 * ----------------------------------------------------------------- */

static void srvl_mpls_segment_internal_init(vtss_mpls_segment_internal_t *s, BOOL in)
{
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Segment init, type %s", in ? "in" : "out");

    memset(s, 0, sizeof(*s));

    s->pub.is_in       = in;
    s->pub.label.value = VTSS_MPLS_LABEL_VALUE_DONTCARE;
    s->pub.label.tc    = VTSS_MPLS_TC_VALUE_DONTCARE;
    s->pub.label.ttl   = 255;
    VTSS_MPLS_IDX_UNDEF(s->pub.l2_idx);
    VTSS_MPLS_IDX_UNDEF(s->pub.policer_idx);
    VTSS_MPLS_IDX_UNDEF(s->pub.server_idx);
    VTSS_MPLS_IDX_UNDEF(s->pub.xc_idx);
    VTSS_MPLS_IDXCHAIN_UNDEF(s->pub.clients);

    VTSS_MPLS_IDX_UNDEF(s->next_free);
    s->need_hw_alloc = TRUE;

    if (in) {
        s->u.in.has_mll  = FALSE;
        s->u.in.has_mlbs = FALSE;
        VTSS_MPLS_IDX_UNDEF(s->u.in.vprofile_idx);
    }
    else {
        s->u.out.has_encap = FALSE;
        s->u.out.has_es0   = FALSE;
        s->u.out.esdx      = NULL;
        VTSS_MPLS_IDX_UNDEF(s->u.out.encap_idx);
        s->u.out.encap_bytes = 0;
    }
}

static vtss_rc srvl_mpls_segment_alloc(BOOL                            in_seg,
                                       vtss_mpls_segment_idx_t * const idx)
{
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_TAKE_FROM_FREELIST(mpls_segment_free_list, mpls_segment_conf, *idx);
    srvl_mpls_segment_internal_init(&vtss_state->mpls_segment_conf[*idx], in_seg);
    vtss_state->mpls_segment_conf[*idx].pub.is_in = in_seg;
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_segment_free(vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_internal_t *seg;
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry");

    if (VTSS_MPLS_IDX_IS_UNDEF(idx)) {
        return VTSS_RC_OK;
    }
    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);

    // Rules:
    //   - Segment must not be attached to an L2 entry
    //   - Segment must not be attached to an XC
    //   - Segment must not be a server
    //   - Segment must not be attached to a server

    seg = &SEG_I(idx);

    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.l2_idx));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.xc_idx));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDXCHAIN_END(seg->pub.clients));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.server_idx));

    VTSS_MPLS_RETURN_TO_FREELIST(mpls_segment_free_list, mpls_segment_conf, idx);
    return VTSS_RC_OK;
}

/** \brief Add/update label stacks for segment and any clients it has.
 *
 * Does not affect IS0 VCAP ordering.
 */
static vtss_rc srvl_mpls_segment_hw_label_stack_refresh_recursive(vtss_mpls_segment_idx_t idx, u8 depth)
{
    vtss_mpls_segment_internal_t *seg = &SEG_I(idx);
    vtss_mpls_idxchain_iter_t    iter;
    vtss_mpls_idxchain_user_t    user;
    BOOL                         more;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Refreshing %s-segment %d, depth = %u", seg->pub.is_in ? "in" : "out", idx, depth);

    VTSS_MPLS_CHECK(depth <= (seg->pub.is_in ? VTSS_MPLS_IN_ENCAP_LABEL_CNT : VTSS_MPLS_OUT_ENCAP_LABEL_CNT));

    if (VTSS_MPLS_IDX_IS_UNDEF(seg->pub.xc_idx)) {
        VTSS_NG(VTSS_TRACE_GROUP_MPLS, "Segment %d has no XC, cannot update label stack", idx);
        return VTSS_RC_OK;
    }

    // Update labels for this segment
    
    if (seg->pub.is_in) {
        if (seg->u.in.has_mlbs) {
            VTSS_MPLS_CHECK_RC(srvl_mpls_is0_mlbs_update(idx));
        }
    }
    else {
        if (seg->u.out.has_encap) {
            VTSS_MPLS_CHECK_RC(srvl_mpls_encap_update(idx));
        }
        if (seg->u.out.has_es0) {
            VTSS_MPLS_CHECK_RC(srvl_mpls_es0_update(idx));
        }
    }

    // Update clients

    more = vtss_mpls_idxchain_get_first(&seg->pub.clients, &iter, &user);
    while (more) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_segment_hw_label_stack_refresh_recursive(user, depth + 1));
        more = vtss_mpls_idxchain_get_next(&iter, &user);
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_hw_alloc(vtss_mpls_xc_idx_t idx)
{
    vtss_mpls_xc_internal_t      *xc = &XC_I(idx);
    vtss_mpls_segment_state_t    in_state, out_state;
    BOOL                         need_isdx = FALSE;
    vtss_port_no_t               port_no   = 0;       // Quiet lint

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "XC %d, HW alloc", idx);

    if (xc->isdx) {
        return VTSS_RC_OK;
    }

    switch (xc->pub.type) {
    case VTSS_MPLS_XC_TYPE_LER:
        if (VTSS_MPLS_IDX_IS_DEF(xc->pub.in_seg_idx)) {
            need_isdx = SEG_P(xc->pub.in_seg_idx).pw_conf.is_pw;
            port_no   = srvl_mpls_port_no_get(xc->pub.in_seg_idx);
        }
        break;
    case VTSS_MPLS_XC_TYPE_LSR:
        if (VTSS_MPLS_IDX_IS_DEF(xc->pub.in_seg_idx)  &&  VTSS_MPLS_IDX_IS_DEF(xc->pub.out_seg_idx)) {
            need_isdx = SEG_P(xc->pub.in_seg_idx).pw_conf.is_pw;
            port_no   = srvl_mpls_port_no_get(xc->pub.in_seg_idx);

            (void) srvl_mpls_segment_state_get(xc->pub.in_seg_idx,  &in_state);
            (void) srvl_mpls_segment_state_get(xc->pub.out_seg_idx, &out_state);
            need_isdx = in_state == VTSS_MPLS_SEGMENT_STATE_CONF  ||  out_state == VTSS_MPLS_SEGMENT_STATE_CONF;
        }
        break;
    }

    if (need_isdx) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_sdx_alloc(TRUE, port_no, 1, &xc->isdx));
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_hw_free(vtss_mpls_xc_idx_t idx)
{
    vtss_mpls_xc_internal_t   *xc = &XC_I(idx);
    vtss_mpls_segment_state_t in_state, out_state;
    BOOL                      free_isdx = TRUE;

    if (! xc->isdx) {
        return VTSS_RC_OK;
    }

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "XC %d, HW free. ISDX %d", idx, xc->isdx->sdx);

    switch (xc->pub.type) {
    case VTSS_MPLS_XC_TYPE_LER:
        break;
    case VTSS_MPLS_XC_TYPE_LSR:
        if (VTSS_MPLS_IDX_IS_DEF(xc->pub.in_seg_idx)  &&  VTSS_MPLS_IDX_IS_DEF(xc->pub.out_seg_idx)) {
            (void) srvl_mpls_segment_state_get(xc->pub.in_seg_idx,  &in_state);
            (void) srvl_mpls_segment_state_get(xc->pub.out_seg_idx, &out_state);
            free_isdx = in_state != VTSS_MPLS_SEGMENT_STATE_UP  &&  out_state != VTSS_MPLS_SEGMENT_STATE_UP;
        }
        break;
    }

    if (free_isdx) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_sdx_free(TRUE, 1, xc->isdx));
        xc->isdx = NULL;
    }

    return VTSS_RC_OK;

}

// Helper function for @srvl_mpls_segment_hw_update() and friends, don't call directly
static vtss_rc srvl_mpls_segment_hw_free_recursive(vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_internal_t *seg = &SEG_I(idx);
    vtss_mpls_idxchain_iter_t    iter;
    vtss_mpls_idxchain_user_t    user;
    BOOL                         more;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry, segment %d", idx);

    if (seg->pub.is_in) {
        (void) srvl_mpls_vprofile_free(seg->u.in.vprofile_idx);
    }

    (void) srvl_mpls_xc_hw_free(seg->pub.xc_idx);

    more = vtss_mpls_idxchain_get_first(&seg->pub.clients, &iter, &user);
    while (more) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_segment_hw_free_recursive(user));
        more = vtss_mpls_idxchain_get_next(&iter, &user);
    }

    return VTSS_RC_OK;
}

// Helper function for @srvl_mpls_segment_hw_update() and friends, don't call directly
static vtss_rc srvl_mpls_segment_hw_free(vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_internal_t *seg = &SEG_I(idx);

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry, segment %d", idx);

    if (seg->pub.is_in) {
        // We never free the MLL since it's owned by the L2 entry
        if (seg->u.in.has_mlbs) {
            (void) srvl_mpls_is0_mlbs_tear_all(idx);
        }
    }
    else {
        if (seg->u.out.has_encap  ||  seg->u.out.has_es0) {
            (void) srvl_mpls_es0_encap_tear_all(idx);
        }
    }

    return srvl_mpls_segment_hw_free_recursive(idx);
}

// Helper function for @srvl_mpls_segment_hw_update(), don't call directly
static vtss_rc srvl_mpls_segment_hw_alloc_recursive(vtss_mpls_segment_idx_t idx, u8 depth)
{
    vtss_mpls_idxchain_iter_t    iter;
    vtss_mpls_idxchain_user_t    user;
    BOOL                         more;
    vtss_mpls_segment_state_t    state;
    vtss_mpls_segment_internal_t *seg    = &SEG_I(idx);
    vtss_mpls_xc_internal_t      *xc     = &XC_I(seg->pub.xc_idx);
    BOOL                         is_lsr  = FALSE;
    BOOL                         is_pw   = FALSE;
    vtss_port_no_t               l2_port = srvl_mpls_port_no_get(idx);

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry: container = %d, depth %d, alloc needed: %s", idx, depth, seg->need_hw_alloc ? "Yes" : "no");
    VTSS_MPLS_CHECK(depth <= VTSS_MPLS_IN_ENCAP_LABEL_CNT);

    (void) srvl_mpls_segment_state_get(idx, &state);
    if (state != VTSS_MPLS_SEGMENT_STATE_CONF) {
        VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Segment %d is not CONF but %s; cannot allocate HW", idx, vtss_mpls_segment_state_to_str(state));
        return VTSS_RC_ERROR;
    }

    seg->need_hw_alloc = FALSE;

    if (VTSS_MPLS_IDX_IS_DEF(seg->pub.server_idx)) {
        (void) srvl_mpls_segment_state_get(seg->pub.server_idx, &state);
        if (state != VTSS_MPLS_SEGMENT_STATE_UP) {
            VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Server %d is not UP but %s; cannot allocate HW", seg->pub.server_idx, vtss_mpls_segment_state_to_str(state));
            return VTSS_RC_ERROR;
        }
    }

    // Allocate XC HW resources or bail out
    if (srvl_mpls_xc_hw_alloc(seg->pub.xc_idx) != VTSS_RC_OK) {
        goto err_out;
    }

    is_lsr = xc->pub.type == VTSS_MPLS_XC_TYPE_LSR;
    is_pw  = xc->pub.type == VTSS_MPLS_XC_TYPE_LER  &&  seg->pub.pw_conf.is_pw;

    if (seg->pub.is_in) {
        // For LSR: use fixed VProfile;
        // For LER PW: allocate VProfile or bail out
        if (is_lsr) {
            seg->u.in.vprofile_idx = VTSS_MPLS_VPROFILE_LSR_IDX;
        }
        else if (is_pw) {
            vtss_mpls_vprofile_t *vp;

            (void) srvl_mpls_vprofile_alloc(&seg->u.in.vprofile_idx);
            if (VTSS_MPLS_IDX_IS_UNDEF(seg->u.in.vprofile_idx)) {
                goto err_out;
            }
            vp               = &VP_P(seg->u.in.vprofile_idx);
            vp->s1_s2_enable = TRUE;
            vp->port         = l2_port;
            vp->recv_enable  = TRUE;

            // FIXME: Raw/tagged mode setup
            vp->vlan_aware          = TRUE;
            vp->map_dscp_cos_enable = FALSE;
            vp->map_eth_cos_enable  = FALSE;

            (void) srvl_mpls_vprofile_hw_update(seg->u.in.vprofile_idx);
        }

        // LSR, LER PW: Build label stack for ourself or bail out
        if (is_lsr  ||  is_pw) {
            (void) srvl_mpls_is0_mlbs_update(idx);
            if (!seg->u.in.has_mlbs) {
                goto err_out;
            }
        }
    }
    else {
        // Allocate entries or bail out.
        // Note order: ES0 depends on encap and ESDX

        if ((is_lsr || is_pw)  &&  !seg->u.out.has_encap) {
            (void) srvl_mpls_encap_alloc(idx);
            if (!seg->u.out.has_encap) {
                goto err_out;
            }
        }

        if (is_lsr  &&  seg->u.out.esdx == NULL) {
            (void) srvl_mpls_sdx_alloc(FALSE, l2_port, 1, &seg->u.out.esdx);
            if (seg->u.out.esdx == NULL) {
                goto err_out;
            }
        }

        if (is_lsr  &&  !seg->u.out.has_es0) {
            (void) srvl_mpls_es0_update(idx);
            if (!seg->u.out.has_es0) {
                goto err_out;
            }
        }
    }

    // Go through all clients
    more = vtss_mpls_idxchain_get_first(&seg->pub.clients, &iter, &user);
    while (more) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_segment_hw_alloc_recursive(user, depth + 1));
        more = vtss_mpls_idxchain_get_next(&iter, &user);
    }

    return VTSS_RC_OK;    

err_out:
    (void) srvl_mpls_segment_hw_free(idx);
    return VTSS_RC_ERROR;
}

// Helper function for @srvl_mpls_segment_hw_update(), don't call directly
static vtss_rc srvl_mpls_segment_hw_alloc(vtss_mpls_segment_idx_t idx)
{
    u8 depth;
    (void) srvl_mpls_find_ultimate_server(idx, &depth);
    return srvl_mpls_segment_hw_alloc_recursive(idx, depth);
}

// Helper function for @srvl_mpls_segment_hw_update(), don't call directly
static vtss_rc srvl_mpls_segment_hw_refresh(vtss_mpls_segment_idx_t idx)
{
    u8 depth;
    (void) srvl_mpls_find_ultimate_server(idx, &depth);
    return srvl_mpls_segment_hw_label_stack_refresh_recursive(idx, depth);
}

static vtss_rc srvl_mpls_segment_hw_update(vtss_mpls_segment_idx_t idx)
{
    vtss_rc                   rc;
    vtss_mpls_segment_state_t state;

    (void) srvl_mpls_segment_state_get(idx, &state);
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Update of segment %d, entry state %s", idx,
            vtss_mpls_segment_state_to_str(state));

    switch (state) {
    case VTSS_MPLS_SEGMENT_STATE_UNCONF:
        SEG_I(idx).need_hw_alloc = TRUE;
        rc = srvl_mpls_segment_hw_free(idx);
        break;
    case VTSS_MPLS_SEGMENT_STATE_CONF:
        SEG_I(idx).need_hw_alloc = TRUE;
        rc = srvl_mpls_segment_hw_alloc(idx);
        break;
    case VTSS_MPLS_SEGMENT_STATE_UP:
        rc = srvl_mpls_segment_hw_refresh(idx);
        break;
    default:
        return VTSS_RC_ERROR;
    }

    (void) srvl_mpls_segment_state_get(idx, &state);
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "Update of segment %d done, exit state %s, rc = %d", idx,
            vtss_mpls_segment_state_to_str(state), rc);
    
    return rc;
}

static vtss_rc srvl_mpls_segment_set(const vtss_mpls_segment_idx_t     idx,
                                     const vtss_mpls_segment_t * const seg)
{
    vtss_mpls_segment_internal_t *seg_i;
    BOOL                         update;
    vtss_mpls_segment_state_t    state;

    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);
    seg_i = &SEG_I(idx);

    // Rules:
    //   - Can't change L2 index once set (but can detach with another func call)
    //   - Can't change server index once set (but can detach with another func call)
    //   - Can't change XC index once set (but can detach with another func call)
    //   - Can't change in/out type
    //   - Can't change upstream flag if attached to L2
    //   - Can't change between PW/LSP type if attached to XC

#define TST_IDX(x)     VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg_i->pub.x)   || (seg->x == seg_i->pub.x))
#define TST_DEP(idx,x) VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg_i->pub.idx) || (seg->x == seg_i->pub.x))

    TST_IDX(l2_idx);
    TST_IDX(server_idx);
    TST_IDX(xc_idx);
    VTSS_MPLS_CHECK(seg->is_in == seg_i->pub.is_in);
    TST_DEP(l2_idx, upstream);
    TST_DEP(xc_idx, pw_conf.is_pw);

#undef TST_IDX
#undef TST_DEP

    // Can change label, E-/L-LSP type, TC, policer

    update = (seg->label.value != seg_i->pub.label.value)  ||
             (seg->label.tc    != seg_i->pub.label.tc)     ||
             (seg->e_lsp       != seg_i->pub.e_lsp);

    if (seg->is_in) {
        update = update ||
            (seg->label.ttl      != seg_i->pub.label.ttl) ||
            (seg->l_lsp_cos      != seg_i->pub.l_lsp_cos)  ||
            (seg->tc_qos_map_idx != seg_i->pub.tc_qos_map_idx);
    }
    else {
        update = update ||
            (seg->tc_qos_map_idx != seg_i->pub.tc_qos_map_idx);
    }

    seg_i->pub = *seg;

    (void) srvl_mpls_segment_state_get(idx, &state);
    if (state != VTSS_MPLS_SEGMENT_STATE_UP  ||  update) {
        VTSS_MPLS_CHECK_RC(srvl_mpls_segment_hw_update(idx));
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_segment_server_attach(const vtss_mpls_segment_idx_t idx,
                                               const vtss_mpls_segment_idx_t srv_idx)
{
    vtss_mpls_segment_t *seg;
    vtss_mpls_segment_t *srv_seg;
    u8                  depth;

    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);
    VTSS_MPLS_IDX_CHECK_SEGMENT(srv_idx);

    seg     = &SEG_P(idx);
    srv_seg = &SEG_P(srv_idx);

    // Rules:
    //   - Segment must not already be attached to a server
    //   - Both segments must be attached to XCs
    //   - The XCs can't be the same
    //   - The segments must be different
    //   - Our HW can handle a limited label stack depth; don't allow more
    //   - We can't use an LSR XC as server
    //  (- The segment can't already be in server's client list. Invariant
    //     check; shouldn't be possible since seg can't be attached to server in
    //     the first place)

    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->server_idx));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(seg->xc_idx));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(srv_seg->xc_idx));
    VTSS_MPLS_CHECK(XC_P(srv_seg->xc_idx).type != VTSS_MPLS_XC_TYPE_LSR);
    VTSS_MPLS_CHECK(seg->xc_idx != srv_seg->xc_idx);
    VTSS_MPLS_CHECK(idx != srv_idx);
    (void) srvl_mpls_find_ultimate_server(srv_idx, &depth);
    VTSS_MPLS_CHECK(depth <= (seg->is_in ? VTSS_MPLS_IN_ENCAP_LABEL_CNT : VTSS_MPLS_OUT_ENCAP_LABEL_CNT));
    VTSS_MPLS_CHECK(!vtss_mpls_idxchain_find(&SEG_P(srv_idx).clients, idx));

    VTSS_MPLS_CHECK_RC(vtss_mpls_idxchain_append_to_tail(&SEG_P(srv_idx).clients, idx));
    seg->server_idx = srv_idx;

    (void) srvl_mpls_segment_hw_update(idx);

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_segment_server_detach(const vtss_mpls_segment_idx_t idx)
{
    vtss_mpls_segment_t *seg;

    // Rules:
    //   - Segment must be attached to a server
    //  (- Segment must be in server's client list -- consistency check)

    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);
    seg = &SEG_P(idx);
    VTSS_MPLS_IDX_CHECK_SEGMENT(seg->server_idx);

    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "Detaching segment %d from server %d", idx, seg->server_idx);
    VTSS_MPLS_CHECK_RC(vtss_mpls_idxchain_remove_first(&SEG_P(seg->server_idx).clients, idx));
    VTSS_MPLS_IDX_UNDEF(seg->server_idx);

    (void) srvl_mpls_segment_hw_update(idx);

    return VTSS_RC_OK;
}



/* ----------------------------------------------------------------- *
 *  XC entries
 * ----------------------------------------------------------------- */

static vtss_rc srvl_mpls_xc_alloc(vtss_mpls_xc_type_t type,
                                  vtss_mpls_xc_idx_t * const idx)
{
    vtss_mpls_xc_internal_t *xc;
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_TAKE_FROM_FREELIST(mpls_xc_free_list, mpls_xc_conf, *idx);

    xc = &vtss_state->mpls_xc_conf[*idx];
    xc->isdx = NULL;
    VTSS_MPLS_IDX_UNDEF(xc->next_free);

    xc->pub.type = type;
    VTSS_MPLS_IDX_UNDEF(xc->pub.in_seg_idx);
    VTSS_MPLS_IDX_UNDEF(xc->pub.out_seg_idx);
    VTSS_MPLS_IDXCHAIN_UNDEF(xc->pub.mc_chain);
#if 0
    VTSS_MPLS_IDX_UNDEF(xc->pub.prot_linear_w_idx);
    VTSS_MPLS_IDX_UNDEF(xc->pub.prot_linear_p_idx);
    VTSS_MPLS_IDX_UNDEF(xc->pub.prot_fb_idx);
#endif

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_free(vtss_mpls_xc_idx_t idx)
{
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry");

    if (VTSS_MPLS_IDX_IS_UNDEF(idx)) {
        return VTSS_RC_OK;
    }
    VTSS_MPLS_IDX_CHECK_XC(idx);
    VTSS_MPLS_RETURN_TO_FREELIST(mpls_xc_free_list, mpls_xc_conf, idx);
    return VTSS_RC_OK;
}


static vtss_rc srvl_mpls_xc_set(const vtss_mpls_xc_idx_t     idx,
                                const vtss_mpls_xc_t * const xc)
{
    vtss_mpls_xc_t *old_xc;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry, XC=%d", idx);
    VTSS_MPLS_IDX_CHECK_XC(idx);
    old_xc = &XC_P(idx);

    // Rules:
    //   - Can't change indices once set
    //   - Can't change type

#define TST_IDX(x)     VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(old_xc->x)   || (xc->x == old_xc->x))
#define TST_DEP(idx,x) VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(old_xc->idx) || (xc->x == old_xc->x))

    VTSS_MPLS_CHECK(old_xc->type == xc->type);
    TST_IDX(in_seg_idx);
    TST_IDX(out_seg_idx);

    // FIXME: More tests once protection is integrated

#undef TST_IDX
#undef TST_DEP

    if (xc->tc_tunnel_mode != old_xc->tc_tunnel_mode  ||  xc->ttl_tunnel_mode != old_xc->ttl_tunnel_mode) {
        old_xc->tc_tunnel_mode  = xc->tc_tunnel_mode;
        old_xc->ttl_tunnel_mode = xc->ttl_tunnel_mode;
        if (VTSS_MPLS_IDX_IS_DEF(old_xc->out_seg_idx)) {
            (void) srvl_mpls_segment_hw_update(old_xc->out_seg_idx);
        }
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_segment_attach(const vtss_mpls_xc_idx_t      xc_idx,
                                           const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_mpls_xc_t               *xc;
    vtss_mpls_segment_internal_t *seg;

    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_IDX_CHECK_XC(xc_idx);
    VTSS_MPLS_IDX_CHECK_SEGMENT(seg_idx);

    xc  = &XC_P(xc_idx);
    seg = &SEG_I(seg_idx);

    // Rules:
    //   - Segment must not already be attached to an XC
    //   - Can't overwrite an already attached segment of same kind
    //   - Segment must match XC type (e.g. "only one segment for LERs")
    //  (- Segment can't have clients yet. Invariant check)
    //  (- Segment can't be attached to a server segment yet. Invariant check)

    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.xc_idx));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDXCHAIN_END(seg->pub.clients));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.server_idx));

    switch (xc->type) {
    case VTSS_MPLS_XC_TYPE_LSR:
        // No check necessary
        break;
    case VTSS_MPLS_XC_TYPE_LER:
        // Either an in- or an out-segment, but not both:
        VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.is_in ? xc->out_seg_idx : xc->in_seg_idx));
        break;
    }

    if (seg->pub.is_in) {
        VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(xc->in_seg_idx));
        xc->in_seg_idx     = seg_idx;
        seg->pub.xc_idx    = xc_idx;
    }
    else {
        VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(xc->out_seg_idx));
        xc->out_seg_idx     = seg_idx;
        seg->pub.xc_idx     = xc_idx;
    }

    (void) srvl_mpls_segment_hw_update(seg_idx);

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_segment_detach(const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_mpls_xc_idx_t           xc_idx;
    vtss_mpls_xc_t               *xc;
    vtss_mpls_segment_internal_t *seg;

    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "entry: Detach seg %d", seg_idx);

    VTSS_MPLS_IDX_CHECK_SEGMENT(seg_idx);
    seg = &SEG_I(seg_idx);
    xc_idx = seg->pub.xc_idx;

    // Rules:
    //   - Segment must be attached to an XC (and XC to segment)
    //   - Segment can't be server to other segments  -- FIXME: Change?
    //   - Segment can't be attached to a server      -- FIXME: Change?

    VTSS_MPLS_IDX_CHECK_XC(xc_idx);
    VTSS_MPLS_CHECK(VTSS_MPLS_IDXCHAIN_END(seg->pub.clients));
    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_UNDEF(seg->pub.server_idx));

    xc = &XC_P(xc_idx);

    VTSS_MPLS_CHECK((seg_idx == xc->in_seg_idx) || (seg_idx == xc->out_seg_idx));  // Consistency check

    if (seg->pub.is_in) {
        VTSS_MPLS_IDX_UNDEF(xc->in_seg_idx);
        VTSS_MPLS_IDX_UNDEF(seg->pub.xc_idx);
    }
    else {
        VTSS_MPLS_IDX_UNDEF(xc->out_seg_idx);
        VTSS_MPLS_IDX_UNDEF(seg->pub.xc_idx);
    }

    (void) srvl_mpls_segment_hw_update(seg_idx);
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_mc_segment_attach(const vtss_mpls_xc_idx_t      xc_idx,
                                              const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_mpls_xc_t      *xc;
    vtss_mpls_segment_t *seg;

    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_IDX_CHECK_XC(xc_idx);
    VTSS_MPLS_IDX_CHECK_SEGMENT(seg_idx);

    xc  = &XC_P(xc_idx);
    seg = &SEG_P(seg_idx);

    // Rules:
    //   - Segment must be attached to an XC (and not this one, @xc_idx)
    //   - Must be out-segment
    //   - Must not already be in chain

    VTSS_MPLS_CHECK(VTSS_MPLS_IDX_IS_DEF(seg->xc_idx));
    VTSS_MPLS_CHECK(seg->xc_idx != xc_idx);
    VTSS_MPLS_CHECK(!seg->is_in);
    VTSS_MPLS_CHECK(!vtss_mpls_idxchain_find(&xc->mc_chain, seg_idx));

    // Add to chain

    VTSS_MPLS_CHECK_RC(vtss_mpls_idxchain_insert_at_head(&xc->mc_chain, seg_idx));

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_mc_segment_detach(const vtss_mpls_xc_idx_t      xc_idx,
                                              const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_mpls_xc_t      *xc;
    vtss_mpls_segment_t *seg;

    VTSS_NG(VTSS_TRACE_GROUP_MPLS, "entry");

    VTSS_MPLS_IDX_CHECK_XC(xc_idx);
    VTSS_MPLS_IDX_CHECK_SEGMENT(seg_idx);

    xc  = &XC_P(xc_idx);
    seg = &SEG_P(seg_idx);

    VTSS_MPLS_CHECK_RC(vtss_mpls_idxchain_remove_first(&xc->mc_chain, seg_idx));
    VTSS_MPLS_IDX_UNDEF(seg->xc_idx);

    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_evc_attach_calc(vtss_evc_id_t evc_id, vtss_res_t *res)
{
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_evc_detach(vtss_evc_id_t evc_id)
{
    return VTSS_RC_OK;
}

static vtss_rc srvl_mpls_xc_evc_attach(vtss_evc_id_t evc_id)
{
    return VTSS_RC_OK;
}

#endif /* VTSS_FEATURE_MPLS */

#if defined(VTSS_FEATURE_AFI_SWC)
/* ================================================================= *
 *  AFI - Automatic Frame Injector
 * ================================================================= */

/*
 * srvl_afi_cancel()
 * Cancels a frame by writing to the AFI.
 */
static vtss_rc srvl_afi_cancel(vtss_afi_id_t afi_id)
{
    u32 val;

    // Do cancel frame
    SRVL_WRM(VTSS_QSYS_TIMED_FRAME_CFG_TFRM_MISC,
             VTSS_F_QSYS_TIMED_FRAME_CFG_TFRM_MISC_TIMED_CANCEL_SLOT(afi_id) | VTSS_F_QSYS_TIMED_FRAME_CFG_TFRM_MISC_TIMED_CANCEL_1SHOT,
             VTSS_M_QSYS_TIMED_FRAME_CFG_TFRM_MISC_TIMED_CANCEL_SLOT         | VTSS_F_QSYS_TIMED_FRAME_CFG_TFRM_MISC_TIMED_CANCEL_1SHOT);

    // Wait for the one-shot bit to clear.
    do {
        SRVL_RD(VTSS_QSYS_TIMED_FRAME_CFG_TFRM_MISC, &val);
    } while (val & VTSS_F_QSYS_TIMED_FRAME_CFG_TFRM_MISC_TIMED_CANCEL_1SHOT);

    return VTSS_RC_OK;
}

/*
 * srvl_afi_pause_resume()
 * Called internally on link-up/down events to resume/pause AFI frames.
 */
static vtss_rc srvl_afi_pause_resume(vtss_port_no_t port_no, BOOL resume)
{
    vtss_afi_id_t afi_id;

#if !VTSS_OPT_FDMA
    if (resume) {
        // Resuming paused AFI frames is only possible when using the FDMA driver.
        // If using an external CPU, that one will have to re-inject the frame using
        // the AFI ID originally obtained with a call to vtss_afi_alloc().
        return VTSS_RC_OK; // So here, we just return VTSS_RC_OK to keep the caller happy.
    }
#endif

    // Search through all AFI slots to see if we send frames to #port_no
    // through this slot.
    for (afi_id = 0; afi_id < VTSS_ARRSZ(vtss_state->afi_slots); afi_id++) {
        vtss_afi_slot_conf_t *slot_conf = &vtss_state->afi_slots[afi_id];

        if (resume && slot_conf->state == VTSS_AFI_SLOT_STATE_PAUSED  && slot_conf->port_no == port_no) {
            // Resuming a paused frame. Only possible with FDMA driver.
#if VTSS_OPT_FDMA
            slot_conf->state = VTSS_AFI_SLOT_STATE_ENABLED;
            VTSS_RC(vtss_state->fdma_state.fdma_func.fdma_afi_pause_resume(vtss_state, afi_id, TRUE));
#endif
        } else if (!resume && slot_conf->state == VTSS_AFI_SLOT_STATE_ENABLED && slot_conf->port_no == port_no) {
            // Pausing an enabled frame. This will remove the frame from the switch core, but keep the software state.
            slot_conf->state = VTSS_AFI_SLOT_STATE_PAUSED;

            VTSS_RC(srvl_afi_cancel(afi_id));

#if VTSS_OPT_FDMA
            // Also notify the FDMA driver about this.
            VTSS_RC(vtss_state->fdma_state.fdma_func.fdma_afi_pause_resume(vtss_state, afi_id, FALSE));
#endif
        }
    }

    return VTSS_RC_OK;
}

/*
 * srvl_afi_alloc()
 * Attempts to find a suitable AFI timer and AFI slot for a given frame to be
 * periodically injected.
 */
static vtss_rc srvl_afi_alloc(const vtss_afi_frm_dscr_t *const dscr, vtss_afi_id_t *const id)
{
    u32                   timer, slot, min_slot, max_slot;
    vtss_afi_timer_conf_t *timer_conf;
    vtss_afi_slot_conf_t  *slot_conf;

    // If we get here, the user wants to add a new flow. Find a suitable timer.
    for (timer = 0; timer < VTSS_ARRSZ(vtss_state->afi_timers); timer++) {
        // First see if we can reuse one that is already in use.
        timer_conf = &vtss_state->afi_timers[timer];

        if (timer_conf->ref_cnt > 0 && timer_conf->fps == dscr->fps)  {
            // Found a suitable timer.
            break;
        }
    }

    if (timer == VTSS_ARRSZ(vtss_state->afi_timers)) {
        // Didn't find a suitable already-configured timer. See if there are any free timers.
        for (timer = 0; timer < VTSS_ARRSZ(vtss_state->afi_timers); timer++) {

            if (vtss_state->afi_timers[timer].ref_cnt == 0)  {
                // Found a suitable timer
                break;
            }
        }
    }

    if (timer == VTSS_ARRSZ(vtss_state->afi_timers)) {
        // No free timers.
        VTSS_E("Unable to find a usable AFI timer (fps = %u)", dscr->fps);
        return VTSS_RC_ERROR;
    }

    // Find a free slot. The first 16 slots are reserved for frames faster than 60 us.
    if (dscr->fps > 16666) {
        min_slot = 0;
        max_slot = 15;
    } else {
        min_slot = 16;
        max_slot = VTSS_ARRSZ(vtss_state->afi_slots) - 1;
    }

    for (slot = min_slot; slot <= max_slot; slot++) {
        if (vtss_state->afi_slots[slot].state == VTSS_AFI_SLOT_STATE_FREE) {
            break;
        }
    }

    if (slot == max_slot + 1) {
        // Out of slots.
        VTSS_E("Unable to find a usable AFI slot (fps = %u)", dscr->fps);
        return VTSS_RC_ERROR;
    }

    // Unfortunately, the timers aren't defined as a replication (due to distinct defaults),
    // so we have to define our own address.
    #define VTSS_QSYS_TIMED_FRAME_CFG_TFRM_TIMER_CFG(x) VTSS_IOREG(VTSS_TO_QSYS,0x56c6 + (x))

    // Also define the base period in picoseconds.
    #define VTSS_QSYS_TIMER_UNIT_PS 198200ULL

    timer_conf = &vtss_state->afi_timers[timer];
    slot_conf  = &vtss_state->afi_slots[slot];

    // If we get here, we both have a valid timer and a valid slot.
    if (timer_conf->ref_cnt == 0) {
        // Timer is not yet referenced by anyone. Configure it.
        u64 timer_reload_val_ps = VTSS_DIV64(1000000000000ULL, dscr->fps);
        u64 timer_reload_val = VTSS_DIV64(timer_reload_val_ps, VTSS_QSYS_TIMER_UNIT_PS);
        if (timer_reload_val <= 0 || timer_reload_val > 0xFFFFFFFFULL) {
            VTSS_E("Unable to fulfil user request for fps = %u", dscr->fps);
            return VTSS_RC_ERROR;
        }
        SRVL_WR(VTSS_QSYS_TIMED_FRAME_CFG_TFRM_TIMER_CFG(timer), timer_reload_val);
    }

    // Update the S/W state
    timer_conf->ref_cnt++;
    timer_conf->fps      = dscr->fps;
    slot_conf->timer_idx = timer;
    slot_conf->state     = VTSS_AFI_SLOT_STATE_RESERVED; // Only reserved. We don't know the destination port yet.

    *id = slot;
    return VTSS_RC_OK;
}

/*
 * srvl_afi_free()
 * Cancels a periodically injected frame and frees up the
 * resources allocated for it.
 */
static vtss_rc srvl_afi_free(vtss_afi_id_t id)
{
    vtss_afi_slot_conf_t *slot_conf;

    if (id >= VTSS_ARRSZ(vtss_state->afi_slots)) {
        VTSS_E("Invalid AFI ID (%u)", id);
        return VTSS_RC_ERROR;
    }

    slot_conf = &vtss_state->afi_slots[id];

    if (slot_conf->state != VTSS_AFI_SLOT_STATE_FREE) {

        // Ask the switch core to stop it. It doesn't harm to stop
        // a slot that is just reserved and not active.
        VTSS_RC(srvl_afi_cancel(id));

        slot_conf->state = VTSS_AFI_SLOT_STATE_FREE;

        // One less frame referencing this timer.
        vtss_state->afi_timers[slot_conf->timer_idx].ref_cnt--;
    }

    return VTSS_RC_OK;
}

#endif /* defined(VTSS_FEATURE_AFI_SWC) */

/* ================================================================= *
 *  SYNCE (Level 1 syncronization)
 * ================================================================= */

static vtss_rc srvl_synce_clock_out_set(const u32 clk_port)
{
    return VTSS_RC_OK;
}

static vtss_rc srvl_synce_clock_in_set(const u32 clk_port)
{
    return VTSS_RC_OK;
}

/* =================================================================
 *  EEE - Energy Efficient Ethernet
 * =================================================================*/
static vtss_rc srvl_eee_port_conf_set(const vtss_port_no_t       port_no, 
                                      const vtss_eee_port_conf_t *const conf)
{
     /*lint -esym(459, eee_timer_table, eee_timer_table_initialized) */
    static u32     eee_timer_table[128];
    static BOOL    eee_timer_table_initialized = FALSE;
    u32            closest_match_index, closest_match, i, requested_time;
    u32            eee_cfg_reg = 0x0; // SYS::EEE_CFG register value.
    vtss_port_no_t chip_port = VTSS_CHIP_PORT(port_no);

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

    // Make sure that we don't get out of bound
    if (port_no >= VTSS_PORT_ARRAY_SIZE) {
        VTSS_E("Out of bounds: chip_port:%d, VTSS_PORT_ARRAY_SIZE:%d", port_no, VTSS_PORT_ARRAY_SIZE);
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
            VTSS_I("Link partner doesn't support EEE - Keeping EEE disabled. Port:%d", chip_port);
        } else if (!(vtss_state->phy_state[port_no].status.fdx)) {
            // EEE and Half duplex are not supposed to work together, so we disables EEE in the case where the port is in HDX mode.
            VTSS_I("EEE disabled due to that port is in HDX mode, port:%d, fdx:%d", chip_port, vtss_state->phy_state[port_no].status.fdx);
        } else {
            eee_cfg_reg |= VTSS_F_DEV_PORT_MODE_EEE_CFG_EEE_ENA;
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

    eee_cfg_reg |= VTSS_F_DEV_PORT_MODE_EEE_CFG_EEE_TIMER_WAKEUP(closest_match_index);


    // EEE holdoff timer
    eee_cfg_reg |= VTSS_F_DEV_PORT_MODE_EEE_CFG_EEE_TIMER_HOLDOFF(0x5) | VTSS_F_DEV_PORT_MODE_EEE_CFG_EEE_TIMER_AGE(0x23);


    // Registers write
    SRVL_WR(VTSS_DEV_PORT_MODE_EEE_CFG(VTSS_TO_DEV(chip_port)), eee_cfg_reg);

    // EEE fast queues
    eee_cfg_reg = VTSS_F_QSYS_SYSTEM_EEE_CFG_EEE_FAST_QUEUES(conf->eee_fast_queues);

    VTSS_N("eee_cfg_reg = 0x%X, conf->tx_tw = %d", eee_cfg_reg, conf->tx_tw);
    SRVL_WR(VTSS_QSYS_SYSTEM_EEE_CFG(chip_port), eee_cfg_reg);

    // Setting Buffer size to 3 Kbyte & 255 frames.
    SRVL_WR(VTSS_QSYS_SYSTEM_EEE_THRES, 0x3EFF);

    return VTSS_RC_OK;
}

/* =================================================================
 * FAN speed control
 * =================================================================*/
static vtss_rc srvl_fan_controller_init(const vtss_fan_conf_t *const spec)
{
    // Set GPIO alternate functions. PWM is bit 5.
    (void) srvl_gpio_mode(0, 5, VTSS_GPIO_ALT_0);

    // Set PWM frequency 
    SRVL_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
             VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_PWM_FREQ(spec->fan_pwm_freq),
             VTSS_M_DEVCPU_GCB_FAN_CFG_FAN_CFG_PWM_FREQ);

    // Set PWM polarity 
    SRVL_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
             spec->fan_low_pol ? VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_INV_POL : 0,
             VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_INV_POL);
    
    // Set PWM open collector 
    SRVL_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
             spec->fan_open_col ? VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_INV_POL : 0,
             VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_PWM_OPEN_COL_ENA);
    
    // Set fan speed measurement
    if (spec->type == VTSS_FAN_3_WIRE_TYPE) {
        // Enable gating for 3-wire fan types.
        SRVL_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
                 1,
                 VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_GATE_ENA);
    } else {
        //  For 4-wire fan types we need to disable gating (2-wire types doesn't matter)
        SRVL_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
                 0,
                 VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_GATE_ENA);
    }
    

    // Set GPIO alternate functions. ROTA is bit 4.
    (void) srvl_gpio_mode(0, 4, VTSS_GPIO_ALT_0);
 
    return VTSS_RC_OK;
}

static vtss_rc srvl_fan_cool_lvl_set(u8 lvl)
{
    SRVL_WRM(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,
             VTSS_F_DEVCPU_GCB_FAN_CFG_FAN_CFG_DUTY_CYCLE(lvl),
             VTSS_M_DEVCPU_GCB_FAN_CFG_FAN_CFG_DUTY_CYCLE);

    return VTSS_RC_OK;
}

static vtss_rc srvl_fan_cool_lvl_get(u8 *duty_cycle)
{
    u32 fan_cfg_reg;

    // Read the register 
    SRVL_RD(VTSS_DEVCPU_GCB_FAN_CFG_FAN_CFG,&fan_cfg_reg);

    // Get PWM duty cycle
    *duty_cycle = VTSS_X_DEVCPU_GCB_FAN_CFG_FAN_CFG_DUTY_CYCLE(fan_cfg_reg);

    return VTSS_RC_OK;
}

static vtss_rc srvl_fan_rotation_get(vtss_fan_conf_t *fan_spec, u32 *rotation_count)
{
    // Read the register 
    SRVL_RD(VTSS_DEVCPU_GCB_FAN_STAT_FAN_CNT,rotation_count);
    return VTSS_RC_OK;
}

/* =================================================================
 *  Serdes configuration
 * =================================================================*/

#define SRVL_SERDES_WAIT 100000

static const char *srvl_serdes_mode_txt(vtss_serdes_mode_t mode)
{
    return (mode == VTSS_SERDES_MODE_DISABLE ? "DISABLE" : 
            mode == VTSS_SERDES_MODE_2G5 ? "2G5" : 
            mode == VTSS_SERDES_MODE_SGMII ? "SGMII" :
            mode == VTSS_SERDES_MODE_100FX ? "100FX" :
            mode == VTSS_SERDES_MODE_1000BaseX ? "1000BX" : "?");
}

/* Serdes1G: Read/write data */
static vtss_rc srvl_sd1g_read_write(u32 addr, BOOL write, u32 nsec)
{
    u32 data, mask;
    
    if (write)
        mask = VTSS_F_HSIO_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_WR_ONE_SHOT;
    else
        mask = VTSS_F_HSIO_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_RD_ONE_SHOT;

    SRVL_WR(VTSS_HSIO_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG,
           VTSS_F_HSIO_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG_SERDES1G_ADDR(addr) | mask);
    
    do { /* Wait until operation is completed  */
        SRVL_RD(VTSS_HSIO_MCB_SERDES1G_CFG_MCB_SERDES1G_ADDR_CFG, &data);
    } while (data & mask);

    if (nsec)
        VTSS_NSLEEP(nsec);
    
    return VTSS_RC_OK;
}

/* Serdes1G: Read data */
static vtss_rc srvl_sd1g_read(u32 addr) 
{
    return srvl_sd1g_read_write(addr, 0, 0);
}

/* Serdes1G: Write data */
static vtss_rc srvl_sd1g_write(u32 addr, u32 nsec) 
{
    return srvl_sd1g_read_write(addr, 1, nsec);
}

/* Wait 100 usec after some SerDes operations */
static vtss_rc srvl_sd1g_cfg(vtss_serdes_mode_t mode, u32 addr) 
{
    BOOL ena_lane = 1, if_100fx = 0, ena_dc_coupling = 0;
    u32  ob_amp_ctrl, cpmd_sel=0, mbtr_ctrl=2, des_bw_ana=6;

    VTSS_D("addr: 0x%x, mode: %s", addr, srvl_serdes_mode_txt(mode));
    
    switch (mode) {
    case VTSS_SERDES_MODE_SGMII:
        ob_amp_ctrl = 12;
        break;
    case VTSS_SERDES_MODE_100FX:
        ob_amp_ctrl = 12;
        if_100fx = 1;
        cpmd_sel = 2;
        mbtr_ctrl = 3;
        ena_dc_coupling = 1;
        des_bw_ana = 0;
        break;
    case VTSS_SERDES_MODE_1000BaseX:
        ob_amp_ctrl = 15;
        break;
    case VTSS_SERDES_MODE_DISABLE:
        ena_lane = 0;
        ob_amp_ctrl = 0;
        break;
    default:
        VTSS_E("Serdes1g mode %s not supported", srvl_serdes_mode_txt(mode));
        return VTSS_RC_ERROR;
    }
  
    /* 1. Configure macro, apply reset */
    /* IB_CFG */
    SRVL_WRM(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG,
             (if_100fx ? VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_FX100_ENA : 0) |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DETLEV |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_DET_LEV(3) |
             (ena_dc_coupling ? VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DC_COUPLING : 0) |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_EQ_GAIN(2),
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_FX100_ENA |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DETLEV |
             VTSS_M_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_DET_LEV |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_ENA_DC_COUPLING |
             VTSS_M_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG_IB_EQ_GAIN);

    /* DES_CFG */
    SRVL_WRM(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_DES_CFG,
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_CPMD_SEL(cpmd_sel) |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_MBTR_CTRL(mbtr_ctrl) |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_BW_ANA(des_bw_ana),
             VTSS_M_HSIO_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_CPMD_SEL |
             VTSS_M_HSIO_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_MBTR_CTRL |
             VTSS_M_HSIO_SERDES1G_ANA_CFG_SERDES1G_DES_CFG_DES_BW_ANA);
    
    /* OB_CFG */
    SRVL_WRM(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_OB_CFG,
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_AMP_CTRL(ob_amp_ctrl),
             VTSS_M_HSIO_SERDES1G_ANA_CFG_SERDES1G_OB_CFG_OB_AMP_CTRL);
    
    /* COMMON_CFG */
    SRVL_WRM(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG,
             (ena_lane ? VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_ENA_LANE : 0), 
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SYS_RST |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_ENA_LANE);

    /* PLL_CFG */
    SRVL_WRM(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG,
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_ENA |
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_CTRL_DATA(200),
             VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_ENA |
             VTSS_M_HSIO_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG_PLL_FSM_CTRL_DATA);   
    
    /* MISC_CFG */
    SRVL_WRM(VTSS_HSIO_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG,
             (if_100fx ? VTSS_F_HSIO_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_DES_100FX_CPMD_ENA : 0) |
             VTSS_F_HSIO_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_LANE_RST,
             VTSS_F_HSIO_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_DES_100FX_CPMD_ENA |
             VTSS_F_HSIO_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_LANE_RST);
    
    VTSS_RC(srvl_sd1g_write(addr, SRVL_SERDES_WAIT));
    
    /* 2. Release PLL reset */
    SRVL_WRM_SET(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG,
                 VTSS_F_HSIO_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG_SYS_RST);
    VTSS_RC(srvl_sd1g_write(addr, SRVL_SERDES_WAIT));
    
    /* 3. Release digital reset */
    SRVL_WRM(VTSS_HSIO_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG, 0,
             VTSS_F_HSIO_SERDES1G_DIG_CFG_SERDES1G_MISC_CFG_LANE_RST);
    VTSS_RC(srvl_sd1g_write(addr, 0));
    
    return VTSS_RC_OK;
}


/* Serdes6G: Read/write data */
static vtss_rc srvl_sd6g_read_write(u32 addr, BOOL write, u32 nsec)
{
    u32 data, mask;

    if (write)
        mask = VTSS_F_HSIO_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_WR_ONE_SHOT;
    else
        mask = VTSS_F_HSIO_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_RD_ONE_SHOT;

    SRVL_WR(VTSS_HSIO_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG,
            VTSS_F_HSIO_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_ADDR(addr) | mask);
    
    do { /* Wait until operation is completed  */
        SRVL_RD(VTSS_HSIO_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG, &data);
    } while (data & mask);

    if (nsec)
        VTSS_NSLEEP(nsec);
    
    return VTSS_RC_OK;
}

/* Serdes6G: Read data */
static vtss_rc srvl_sd6g_read(u32 addr) 
{
    return srvl_sd6g_read_write(addr, 0, 0);
}

/* Serdes6G: Write data */
static vtss_rc srvl_sd6g_write(u32 addr, u32 nsec) 
{
    return srvl_sd6g_read_write(addr, 1, nsec);
}

/* Serdes6G setup (Disable/2G5/QSGMII/SGMII) */
static vtss_rc srvl_sd6g_cfg(vtss_serdes_mode_t mode, u32 addr)
{
    u32  ib_rf = 15, ctrl_data = 60, if_mode = 1, ob_ena_cas = 0, ob_lev = 48;
    u32  ib_vbac = 0, ib_vbcom = 0, ib_ic_ac=0, ib_c=15, ser_alisel=0, ob_post0 = 0;
    BOOL ena_lane = 1, rot_frq = 1, ena_rot = 0, qrate = 1, hrate = 0;
    BOOL ob_ena1v, if_100fx = 0, ser_enali = 0, ser_4tap_ena = 0, rot_dir = 0, ob_pol = 1, ib_chf=0;               ;

    VTSS_D("addr: 0x%x, mode: %s", addr, srvl_serdes_mode_txt(mode));
    ob_ena1v = (vtss_state->init_conf.serdes.serdes6g_vdd == VTSS_VDD_1V0) ? 1 : 0;

    switch (mode) {
    case VTSS_SERDES_MODE_2G5:
        ib_rf = ob_ena1v ? 2 : 10;   /* IB_CFG */
        ib_vbac = ob_ena1v ? 4 : 5;  /* IB_CFG */
        ib_ic_ac = ob_ena1v ? 2 : 0; /* IB_CFG */
        ib_vbcom = ob_ena1v ? 5 : 0; /* IB_CFG */
        ib_chf = ob_ena1v ? 1 : 0;   /* IB_CFG */
        ib_c = 10;                   /* IB_CFG */
        ena_rot = 1;                 /* PLL_CFG */
        ctrl_data = 48;              /* PLL_CFG */
        qrate = 0;                   /* COMMON_CFG */ 
        hrate = 1;                   /* COMMON_CFG */
        ob_lev = ob_ena1v ? 48 : 63; /* OB_CFG1 */
        ser_4tap_ena = ob_ena1v ? 1 : 0; /* SER_CFG */
        ser_enali = 1;                /* SER_CFG */
        ser_alisel = 1;               /* SER_CFG */
        break;
    case VTSS_SERDES_MODE_SGMII:
        ob_ena_cas = 2;
        break;
    case VTSS_SERDES_MODE_100FX:
        if_100fx = 1;
        break;
    case VTSS_SERDES_MODE_1000BaseX:
        ob_ena_cas = 2;
        break;
    case VTSS_SERDES_MODE_DISABLE:
        ena_lane = 0;
        break;
    default:
        VTSS_E("Serdes6g mode %s not supported", srvl_serdes_mode_txt(mode));
        return VTSS_RC_ERROR;
    }

    /* 1. Configure macro, apply reset */

    /* OB_CFG */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG,
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POST0(ob_post0) |
             (ob_pol ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POL : 0) |
             (ob_ena1v ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_ENA1V_MODE : 0),
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POST0 |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POL |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_ENA1V_MODE);

    /* OB_CFG1 */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1,
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_ENA_CAS(ob_ena_cas) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_LEV(ob_lev),
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_ENA_CAS |
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_LEV);

    /* IB_CFG */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG,
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RT(15) | 
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_IC_AC(ib_ic_ac) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_VBAC(ib_vbac) | 
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_VBCOM(ib_vbcom) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RF(ib_rf),
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RT |
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_IC_AC |
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_VBAC |
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_VBCOM |
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RF);

    /* IB_CFG1 */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1,
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_C(ib_c) |
             (ib_chf ? VTSS_BIT(7) : 0 ) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_DIS_EQ |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSAC |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSDC |
             (if_100fx ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA : 0) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST,
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_C |
             VTSS_BIT(7) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_DIS_EQ |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSAC |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSDC |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST);
    
    /* DES_CFG */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_DES_CFG,
             (if_100fx ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL(2) : 0) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_ANA(5),
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL |
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_ANA);
    
    /* SER_CFG */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_SER_CFG,
             (ser_4tap_ena ? VTSS_BIT(8) : 0) |
             VTSS_ENCODE_BITFIELD(ser_alisel,4,2) |
             (ser_enali ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_SER_CFG_SER_ENALI : 0),
             VTSS_BIT(8) |
             VTSS_ENCODE_BITMASK(4,2) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_SER_CFG_SER_ENALI);
    
    /* PLL_CFG */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG,
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_CTRL_DATA(ctrl_data) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_ENA |
             (rot_dir ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ROT_DIR : 0) |
             (rot_frq ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ROT_FRQ : 0) |
             (ena_rot ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ENA_ROT : 0),
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_CTRL_DATA |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ROT_DIR |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_ENA |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ROT_FRQ |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ENA_ROT);

    /* Write masked to avoid changing RECO_SEL_* fields used by SyncE */
    /* COMMON_CFG */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG,
             (ena_lane ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_ENA_LANE : 0) |
             (hrate ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_HRATE : 0) |
             (qrate ? VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_QRATE : 0) |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_IF_MODE(if_mode),
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SYS_RST |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_ENA_LANE |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_HRATE |
             VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_QRATE |
             VTSS_M_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_IF_MODE);
    
    /* MISC_CFG */      
    SRVL_WRM(VTSS_HSIO_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG,
             (if_100fx ? VTSS_F_HSIO_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA : 0) |
             VTSS_F_HSIO_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST,
             VTSS_F_HSIO_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA |
             VTSS_F_HSIO_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST);

    VTSS_RC(srvl_sd6g_write(addr, SRVL_SERDES_WAIT));

    /* 2. Release PLL reset */
    SRVL_WRM_SET(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, 
                 VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SYS_RST);
    VTSS_RC(srvl_sd6g_write(addr, SRVL_SERDES_WAIT));
    
    /* 3. Release digital reset */
    SRVL_WRM_CLR(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1,
                 VTSS_F_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST);

    SRVL_WRM(VTSS_HSIO_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, 0,
             VTSS_F_HSIO_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST);
    VTSS_RC(srvl_sd6g_write(addr, 0));

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Port control
 * ================================================================= */

static vtss_rc srvl_port_max_tags_set(vtss_port_no_t port_no)
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
    
    SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG(tgt), 
            VTSS_F_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG_TAG_ID(etype) |
            (double_ena ? VTSS_F_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG_VLAN_DBL_AWR_ENA : 0) |
            (single_ena ? VTSS_F_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG_VLAN_AWR_ENA : 0) |
            VTSS_F_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG_VLAN_LEN_AWR_ENA);
    return VTSS_RC_OK;
}

static vtss_rc srvl_miim_read_write(BOOL read, 
                                    u32 miim_controller, 
                                    u8 miim_addr, 
                                    u8 addr, 
                                    u16 *value, 
                                    BOOL report_errors)
{
    u32 data;

    /* Enqueue MIIM operation to be executed */
    SRVL_WR(VTSS_DEVCPU_GCB_MIIM_MII_CMD(miim_controller), 
            VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_VLD |
            VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_PHYAD(miim_addr) |
            VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_REGAD(addr) |
            VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_WRDATA(*value) |
            VTSS_F_DEVCPU_GCB_MIIM_MII_CMD_MIIM_CMD_OPR_FIELD(read ? 2 : 1));
    
    /* Wait for MIIM operation to finish */
    do {
        SRVL_RD(VTSS_DEVCPU_GCB_MIIM_MII_STATUS(miim_controller), &data);
    } while (data & VTSS_F_DEVCPU_GCB_MIIM_MII_STATUS_MIIM_STAT_BUSY);
    
    if (read) {
        SRVL_RD(VTSS_DEVCPU_GCB_MIIM_MII_DATA(miim_controller), &data);
        if (data & VTSS_F_DEVCPU_GCB_MIIM_MII_DATA_MIIM_DATA_SUCCESS(3))
            return VTSS_RC_ERROR;
        *value = VTSS_X_DEVCPU_GCB_MIIM_MII_DATA_MIIM_DATA_RDDATA(data);
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_miim_read(vtss_miim_controller_t miim_controller, 
                              u8 miim_addr, 
                              u8 addr, 
                              u16 *value, 
                              BOOL report_errors)
{
    return srvl_miim_read_write(TRUE, miim_controller, miim_addr, addr, value, report_errors);
}

static vtss_rc srvl_miim_write(vtss_miim_controller_t miim_controller, 
                               u8 miim_addr, 
                               u8 addr, 
                               u16 value, 
                               BOOL report_errors)
{
    return srvl_miim_read_write(FALSE, miim_controller, miim_addr, addr, &value, report_errors);
}

static vtss_rc srvl_serdes_inst_get(u32 port, u32 *inst, BOOL *serdes6g)
{
    if (port >= VTSS_CHIP_PORTS) {
        VTSS_E("illegal port: %u", port);
        return VTSS_RC_ERROR;
    }

    if (port == 8 || port == 9) {
        /* SERDES6G, instance 0-1 */
        *serdes6g = 1;
        *inst = (port - 8);
    } else {
        /* SERDES1G, instance 0-8 */
        *serdes6g = 0;
        *inst = (port == 10 ? 8 : port);
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_port_fc_setup(u32 port, vtss_port_conf_t *conf)
{
    u8                *mac;
    u32               pause_start, pause_stop, rsrv_raw, rsrv_total = 0, atop_wm, link_speed;
    u32               tgt = VTSS_TO_DEV(port), rsrv_raw_fc_jumbo, rsrv_raw_no_fc_jumbo, rsrv_raw_fc_no_jumbo;
    BOOL              fc = conf->flow_control.generate;
    vtss_port_speed_t speed = conf->speed;
    vtss_port_no_t    port_no;

    pause_start = 0x7ff;
    pause_stop  = 0x7ff;
    rsrv_raw_fc_jumbo = 40000;
    rsrv_raw_no_fc_jumbo = 12000;
    rsrv_raw_fc_no_jumbo = 13662; /* 9 x 1518 */

    if (conf->max_frame_length > VTSS_MAX_FRAME_LENGTH_STANDARD) {
        pause_start = 228;     /* 9 x 1518 / 60 */
        if (fc) {
            /* FC and jumbo enabled*/
            pause_stop  = 177;     /* 7 x 1518 / 60 */
            rsrv_raw    = rsrv_raw_fc_jumbo;   
        } else {
            /* FC disabled, jumbo enabled */
            rsrv_raw = rsrv_raw_no_fc_jumbo;
        }
    } else {
        pause_start = 152;    /* 6 x 1518 / 60 */
        if (fc) {
        /* FC enabled, jumbo disabled */
            pause_stop  = 101;    /* 4 x 1518 / 60 */
            rsrv_raw    = rsrv_raw_fc_no_jumbo;  
        } else {
            rsrv_raw    = 0;
        }
    } 

    /* Set Pause WM hysteresis*/
    SRVL_WR(VTSS_SYS_PAUSE_CFG_PAUSE_CFG(port),
            VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_START(pause_start) |
            VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_STOP(pause_stop) |
            (fc ? VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_ENA : 0));
    
    /* Set SMAC of Pause frame */
    mac = &conf->flow_control.smac.addr[0];
    SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_FC_MAC_HIGH_CFG(tgt), (mac[0]<<16) | (mac[1]<<8) | mac[2]);
    SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_FC_MAC_LOW_CFG(tgt), (mac[3]<<16) | (mac[4]<<8) | mac[5]);

    /* Enable/disable FC incl. pause value and zero pause */
    link_speed = (speed == VTSS_SPEED_10M ? 3 :
                  speed == VTSS_SPEED_100M ? 2 :
                  speed == VTSS_SPEED_1G ? 1 : 0);
    SRVL_WR(VTSS_SYS_PAUSE_CFG_MAC_FC_CFG(port),
            VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_FC_LINK_SPEED(link_speed) |
            VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_FC_LATENCY_CFG(7) |
            VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_ZERO_PAUSE_ENA |
            (conf->flow_control.obey ? VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_RX_FC_ENA : 0) |
            VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_PAUSE_VAL_CFG(0xffff));


    SRVL_WRM(VTSS_QSYS_SYSTEM_SWITCH_PORT_MODE(port),
             fc ? 0 : VTSS_F_QSYS_SYSTEM_SWITCH_PORT_MODE_INGRESS_DROP_MODE,
             VTSS_F_QSYS_SYSTEM_SWITCH_PORT_MODE_INGRESS_DROP_MODE);

  
    /* Calculate the total reserved space for all ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        conf = &vtss_state->port_conf[port_no];
        fc = conf->flow_control.generate;
        if (conf->max_frame_length > VTSS_MAX_FRAME_LENGTH_STANDARD) {
            if (fc) {
                rsrv_total += rsrv_raw_fc_jumbo; 
            } else {
                rsrv_total += rsrv_raw_no_fc_jumbo;
            }
        } else if (fc) {
            rsrv_total += rsrv_raw_fc_no_jumbo; 
        }
    }
 
    atop_wm = (SRVL_BUFFER_MEMORY - rsrv_total)/SRVL_BUFFER_CELL_SZ;
    if (atop_wm >= 2048) {
        atop_wm = 2048 + atop_wm/16;
    }

    /*  When 'port ATOP' and 'common ATOP_TOT' are exceeded, tail dropping is activated on port */
    SRVL_WR(VTSS_SYS_PAUSE_CFG_ATOP_TOT_CFG, atop_wm);
    SRVL_WR(VTSS_SYS_PAUSE_CFG_ATOP(port), rsrv_raw/SRVL_BUFFER_CELL_SZ);

    return VTSS_RC_OK;
}

static vtss_rc srvl_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_port_conf_t       *conf = &vtss_state->port_conf[port_no];
    u32                    port = VTSS_CHIP_PORT(port_no);
    u32                    link_speed, value, tgt = VTSS_TO_DEV(port), inst, delay = 0, addr;
    vtss_port_frame_gaps_t gaps;
    vtss_port_speed_t      speed = conf->speed;
    BOOL                   fdx = conf->fdx, disable = conf->power_down;
    BOOL                   sgmii = 0, if_100fx = 0, serdes6g;
    vtss_serdes_mode_t     mode = VTSS_SERDES_MODE_SGMII;   
   
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
        VTSS_E("illegal speed:%d, port %u", speed, port);
        return VTSS_RC_ERROR;
    }

    switch (conf->if_type) {
    case VTSS_PORT_INTERFACE_NO_CONNECTION:
        disable = 1;
        break;
    case VTSS_PORT_INTERFACE_INTERNAL:
    case VTSS_PORT_INTERFACE_SGMII:
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
    default:
        VTSS_E("illegal interface, port %u", port);
        return VTSS_RC_ERROR;
    }

    /* Configure the Serdes macros to 100FX / 1000BaseX / 2500 */
    if (mode != vtss_state->serdes_mode[port_no]) {
        vtss_state->serdes_mode[port_no] = mode;
        VTSS_RC(srvl_serdes_inst_get(port, &inst, &serdes6g));
        addr = (1 << inst);
        if (serdes6g) {
            VTSS_RC(srvl_sd6g_read(addr));
            VTSS_MSLEEP(10);
            VTSS_RC(srvl_sd6g_cfg(mode, addr));
            VTSS_RC(srvl_sd6g_write(addr, SRVL_SERDES_WAIT));
        } else {
            VTSS_RC(srvl_sd1g_read(addr));
            VTSS_MSLEEP(10);
            VTSS_RC(srvl_sd1g_cfg(mode, addr));
            VTSS_RC(srvl_sd1g_write(addr, SRVL_SERDES_WAIT));
        }
        VTSS_MSLEEP(1);          
    }

    /* Port disable and flush procedure: */
   
    /* 0.1: Reset the PCS */
    SRVL_WRM_SET(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), 
				 VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PCS_RX_RST);
    /* 1: Disable MAC frame reception */
    SRVL_WRM_CLR(VTSS_DEV_MAC_CFG_STATUS_MAC_ENA_CFG(tgt),
                VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA);

#if defined(VTSS_FEATURE_AFI_SWC)
    /* Pause all AFI-generated frames to this port. */
    VTSS_RC(srvl_afi_pause_resume(port_no, FALSE));
#endif /* VTSS_FEATURE_AFI_SWC */

    /* 2: Disable traffic being sent to or from switch port */
    SRVL_WRM_CLR(VTSS_QSYS_SYSTEM_SWITCH_PORT_MODE(port),
                 VTSS_F_QSYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA);

    /* 3: Disable dequeuing from the egress queues *\/ */
    SRVL_WRM_SET(VTSS_QSYS_SYSTEM_PORT_MODE(port),
                VTSS_F_QSYS_SYSTEM_PORT_MODE_DEQUEUE_DIS);

    /* 4: Wait a worst case time 8ms (jumbo/10Mbit) *\/ */
    VTSS_MSLEEP(8);

    /* 5: Disable HDX backpressure (Bugzilla 3203) */
    SRVL_WRM_CLR(VTSS_SYS_SYSTEM_FRONT_PORT_MODE(port), 
                 VTSS_F_SYS_SYSTEM_FRONT_PORT_MODE_HDX_MODE);
   
    /* 6: Flush the queues accociated with the port */
    SRVL_WRM_SET(VTSS_REW_PORT_PORT_CFG(port), 
                 VTSS_F_REW_PORT_PORT_CFG_FLUSH_ENA);

    /* 7: Enable dequeuing from the egress queues */
    SRVL_WRM_CLR(VTSS_QSYS_SYSTEM_PORT_MODE(port),
                VTSS_F_QSYS_SYSTEM_PORT_MODE_DEQUEUE_DIS);

    do { /* 8: Wait until flushing is complete */
        SRVL_RD(VTSS_QSYS_SYSTEM_SW_STATUS(port), &value);
        VTSS_MSLEEP(1);            
        delay++;
        if (delay == 2000) {
            VTSS_E("Flush timeout port %u\n",port);
            break;
        }
    } while (value & VTSS_M_QSYS_SYSTEM_SW_STATUS_EQ_AVAIL);

    /* 9: Reset the clock */
    SRVL_WR(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), 
            VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_TX_RST |
            VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_RX_RST |
            VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PORT_RST);

    /* 10: Clear flushing */
    SRVL_WRM_CLR(VTSS_REW_PORT_PORT_CFG(port), VTSS_F_REW_PORT_PORT_CFG_FLUSH_ENA);

    /* The port is disabled and flushed, now set up the port in the new operating mode */

    /* Bugzilla 4388: disabling frame aging when in HDX */
    SRVL_WRM_CTL(VTSS_REW_PORT_PORT_CFG(port), fdx, VTSS_F_REW_PORT_PORT_CFG_AGE_DIS);

    /* GIG/FDX mode */
    if (fdx) {
        value = VTSS_F_DEV_MAC_CFG_STATUS_MAC_MODE_CFG_FDX_ENA;
        if (speed == VTSS_SPEED_1G || speed == VTSS_SPEED_2500M) {
            value |= VTSS_F_DEV_MAC_CFG_STATUS_MAC_MODE_CFG_GIGA_MODE_ENA;
        }
    } else {        
        SRVL_WRM_SET(VTSS_SYS_SYSTEM_FRONT_PORT_MODE(port), 
                     VTSS_F_SYS_SYSTEM_FRONT_PORT_MODE_HDX_MODE);
        value = conf->flow_control.obey ? 0x100 : 0; /* FC_WORD_SYNC_ENA=1 for HDX/FC for Rev B chip */
    }
    SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), value);
    

    /* Default gaps */
    gaps.fdx_gap = 15;
    gaps.hdx_gap_1 = 0;
    gaps.hdx_gap_2 = 0;
    if (speed == VTSS_SPEED_1G || speed == VTSS_SPEED_2500M) {
        gaps.fdx_gap = 5;
    } else if (!fdx) {
        gaps.hdx_gap_1 = (speed == VTSS_SPEED_100M ? 7 : 11);
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
    SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_IFG_CFG(tgt), 
            VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_TX_IFG(gaps.fdx_gap) |
            VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG1(gaps.hdx_gap_1) |
            VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG2(gaps.hdx_gap_2));
    
    /* Load seed and set MAC HDX late collision */
    value = ((conf->exc_col_cont ? 
              VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_RETRY_AFTER_EXC_COL_ENA : 0) | 
             VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_LATE_COL_POS(67));
    SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_HDX_CFG(tgt), 
            value | VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED_LOAD);
    VTSS_NSLEEP(1000);
    SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_HDX_CFG(tgt), value);

    /* PCS settings for 100fx/SGMII/SERDES */
    if (if_100fx) {
        /* 100FX PCS */                    
        SRVL_WRM(VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt),
                 (disable ? 0 : VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_PCS_ENA) |
                 (conf->sd_internal ? 0 : VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_SEL) |
                 (conf->sd_active_high ? VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_POL : 0) |
                 (conf->sd_enable ? VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_ENA : 0),
                 VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_PCS_ENA | 
                 VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_SEL |
                 VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_POL |
                 VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_ENA);
    } else {
        /* Disable 100FX */
        SRVL_WRM_CLR(VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt),
                     VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_PCS_ENA);

        /* Choose SGMII or Serdes PCS mode */
        SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG(tgt), 
                (sgmii ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG_SGMII_MODE_ENA : 0));
        
        if (sgmii) {
            /* Set whole register */
            SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt),
                    VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_SW_RESOLVE_ENA);
        } else {
            /* Clear specific bit only */
            SRVL_WRM_CLR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt),
                         VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_SW_RESOLVE_ENA);
        }
        
        SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG(tgt),
                (conf->sd_active_high ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_POL : 0) |
                (conf->sd_enable ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_ENA : 0) |
                (conf->sd_internal ? 0 : VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_SEL));
        
        /* Enable/disable PCS */
        SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt), 
                disable ? 0 : VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);
        
        if (conf->if_type == VTSS_PORT_INTERFACE_SGMII) {
            SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 0);
        } else if (conf->if_type == VTSS_PORT_INTERFACE_SGMII_CISCO) {
            /* Complete SGMII aneg */
            SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt),
                    VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ADV_ABILITY(0x0001) |
                    VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_SW_RESOLVE_ENA |
                    VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_ENA |
                    VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_RESTART_ONE_SHOT);
            
            /* Clear the sticky bits */
            SRVL_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), &value);
            SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), value);
        }           
    }

    SRVL_WRM_CTL(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LB_CFG(tgt), 
                 conf->loop == VTSS_PORT_LOOP_PCS_HOST, 
                 VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_LB_CFG_TBI_HOST_LB_ENA);

    /* Set Max Length and maximum tags allowed */
    SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_MAXLEN_CFG(tgt), conf->max_frame_length);
    VTSS_RC(srvl_port_max_tags_set(port_no));

    if (!disable) {
        /* Configure flow control */
        if (srvl_port_fc_setup(port, conf) != VTSS_RC_OK)
            return VTSS_RC_ERROR;
       
        /* Update policer flow control configuration */
        VTSS_RC(srvl_port_policer_fc_set(port_no, port));

        /* Enable MAC module */
        SRVL_WR(VTSS_DEV_MAC_CFG_STATUS_MAC_ENA_CFG(tgt), 
                VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA |
                VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_TX_ENA);
        
        /* Take MAC, Port, Phy (intern) and PCS (SGMII/Serdes) clock out of reset */
        SRVL_WR(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), 
                VTSS_F_DEV_PORT_MODE_CLOCK_CFG_LINK_SPEED(link_speed));
		
        /* Clear the PCS stickys */
        SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt),
                VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY |
                VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_OUT_OF_SYNC_STICKY);

        /* Core: Enable port for frame transfer */
        SRVL_WRM_SET(VTSS_QSYS_SYSTEM_SWITCH_PORT_MODE(port),
                     VTSS_F_QSYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA);

#if defined(VTSS_FEATURE_AFI_SWC)
        /* Resume any AFI-generated frames to this port. This will only happen if using the
         * FDMA Driver to inject frames. An external CPU will have to make sure that
         * the frame is re-injected once link-up event is seen.
         */
        VTSS_RC(srvl_afi_pause_resume(port_no, TRUE));
#endif /*V TSS_FEATURE_AFI */

        /* Enable flowcontrol - must be done after flushing */
        if (conf->flow_control.generate) {   
            SRVL_WRM_SET(VTSS_SYS_PAUSE_CFG_MAC_FC_CFG(port), VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_TX_FC_ENA);
        }

    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_port_ifh_set(const vtss_port_no_t port_no)
{
    u32 port               = VTSS_CHIP_PORT(port_no);
    vtss_port_ifh_t *ifh = &vtss_state->port_ifh_conf[port_no];

     /* Enable/Disable IFH parsing upon injection - expecting to see No Prefix */
    SRVL_WRM_SET(VTSS_SYS_SYSTEM_PORT_MODE(port), VTSS_F_SYS_SYSTEM_PORT_MODE_INCL_INJ_HDR(ifh->ena_inj_header ? 1 : 0));
     /* Enable/Disable IFH parsing upon extraction - add long prefix */
    SRVL_WRM_SET(VTSS_SYS_SYSTEM_PORT_MODE(port), VTSS_F_SYS_SYSTEM_PORT_MODE_INCL_XTR_HDR(ifh->ena_xtr_header ? 3 : 0));
 
    return VTSS_RC_OK;
}

static vtss_rc srvl_port_clause_37_control_set(const vtss_port_no_t port_no)
{
    vtss_port_clause_37_control_t *control = &vtss_state->port_clause_37[port_no];
    u32                           value, port = VTSS_CHIP_PORT(port_no);
       
    /* Aneg capabilities for this port */
    VTSS_RC(vtss_cmn_port_clause_37_adv_set(&value, &control->advertisement, control->enable));

    /* Set aneg capabilities */
    SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(VTSS_TO_DEV(port)), 
           VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ADV_ABILITY(value) |
           VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_RESTART_ONE_SHOT |
           (control->enable ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_ANEG_ENA : 0));
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_port_clause_37_status_get(const vtss_port_no_t         port_no, 
                                              vtss_port_clause_37_status_t *const status)
    
{
    u32                    value, tgt = VTSS_TO_DEV(vtss_state->port_map[port_no].chip_port);
    vtss_port_sgmii_aneg_t *sgmii_adv = &status->autoneg.partner_adv_sgmii;
        
    if (vtss_state->port_conf[port_no].power_down) {
        status->link = 0;
        return VTSS_RC_OK;
    }

    /* Get the link state 'down' sticky bit  */
    SRVL_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), &value);
    status->link = (SRVL_BF(DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY, value) ? 0 : 1);
    if (status->link == 0) {
        /* The link has been down. Clear the sticky bit and return the 'down' value  */
        SRVL_WR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), 
                VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY);        
    } else {
        SRVL_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS(tgt), &value);
        status->link = SRVL_BF(DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS, value);        
    }

    /* Get PCS ANEG status register */
    SRVL_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS(tgt), &value);

    /* Get 'Aneg complete'   */
    status->autoneg.complete = SRVL_BF(DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE, value);

   /* Workaround for a Serdes issue, when aneg completes with FDX capability=0 */
    if (vtss_state->port_conf[port_no].if_type == VTSS_PORT_INTERFACE_SERDES) {
        if (status->autoneg.complete) {
            if (((value >> 21) & 0x1) == 0) { 
                SRVL_WRM_CLR(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt), VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);
                SRVL_WRM_SET(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt), VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);
                (void)srvl_port_clause_37_control_set(port_no); /* Restart Aneg */
                VTSS_MSLEEP(50);
                SRVL_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS(tgt), &value);
                status->autoneg.complete = SRVL_BF(DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE, value);
            }
        }
    }

    /* Return partner advertisement ability */
    value = VTSS_X_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_LP_ADV_ABILITY(value);

    if (vtss_state->port_conf[port_no].if_type == VTSS_PORT_INTERFACE_SGMII_CISCO) {
        sgmii_adv->link = ((value >> 15) == 1) ? 1 : 0;
        sgmii_adv->fdx = (((value >> 12) & 0x1) == 1) ? 1 : 0;
        value = ((value >> 10) & 3);
        sgmii_adv->speed_10M = (value == 0 ? 1 : 0);
        sgmii_adv->speed_100M = (value == 1 ? 1 : 0);
        sgmii_adv->speed_1G = (value == 2 ? 1 : 0);
        sgmii_adv->hdx = (status->autoneg.partner_advertisement.fdx ? 0 : 1);
        if (status->link) {
            /* If the SFP module does not have a link then the port does not have link */
            status->link = sgmii_adv->link;
        }
    } else {
        VTSS_RC(vtss_cmn_port_clause_37_adv_get(value, &status->autoneg.partner_advertisement));
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_port_status_get(const vtss_port_no_t  port_no, 
                                    vtss_port_status_t    *const status)
{
    vtss_port_conf_t *conf = &vtss_state->port_conf[port_no];
    u32              tgt = VTSS_TO_DEV(vtss_state->port_map[port_no].chip_port);
    u32              value;
    
    if (conf->power_down) {
        memset(status, 0, sizeof(*status));
        return VTSS_RC_OK;
    }

    switch (conf->if_type) {
    case VTSS_PORT_INTERFACE_100FX:
        /* Get the PCS status  */
        SRVL_RD(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), &value);
        
        /* Link has been down if the are any error stickies */
        status->link_down = SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_LOST_STICKY, value) ||
                            SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SSD_ERROR_STICKY, value) ||
                            SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_FEF_FOUND_STICKY, value) ||
                            SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_PCS_ERROR_STICKY, value) ||
                            SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_FEF_STATUS, value);
        if (status->link_down) {
            SRVL_WR(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), 0xFFFF);
            SRVL_RD(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), &value);
        }
        /* Link=1 if sync status=1 and no error stickies after a clear */
        status->link = SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_STATUS, value) && 
                     (!SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_LOST_STICKY, value) &&
                      !SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SSD_ERROR_STICKY, value) &&
                      !SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_FEF_FOUND_STICKY, value) &&
                      !SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_PCS_ERROR_STICKY, value) &&
                      !SRVL_BF(DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_FEF_STATUS, value));
        status->speed = VTSS_SPEED_100M; 
        break;
    case VTSS_PORT_INTERFACE_VAUI:
        /* Get the PCS status */
        SRVL_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS(tgt), &value);
        status->link = SRVL_BF(DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS, value);
        SRVL_RD(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), &value);
        VTSS_I("status->link:%d, status->link_down:%d", status->link, status->link_down);
        status->link_down = SRVL_BF(DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY, value);
        if (status->link_down) {
            /* The link has been down. Clear the sticky bit */
            SRVL_WRM_SET(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt),
                         VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY);
        }
        status->speed = VTSS_SPEED_2500M; 
        break;
    case VTSS_PORT_INTERFACE_NO_CONNECTION:
        status->link = 0;
        status->link_down = 0;
        break;
    default:
        return VTSS_RC_ERROR;
    }        
    status->fdx = 1;    
    return VTSS_RC_OK;
}

static vtss_rc srvl_counter_update(u32 *addr, vtss_chip_counter_t *counter, BOOL clear)
{
    u32 value;

    SRVL_RD(VTSS_SYS_STAT_CNT(*addr), &value);
    *addr = (*addr + 1); /* Next counter address */
    vtss_cmn_counter_32_update(value, counter, clear);

    return VTSS_RC_OK;
}

static vtss_rc srvl_port_counters_read(vtss_port_no_t               port_no,
                                       u32                          port,
                                       vtss_port_luton26_counters_t *c,
                                       vtss_port_counters_t *const  counters, 
                                       BOOL                         clear)
{
    u32                                i, base, *p = &base;
    vtss_port_rmon_counters_t          *rmon;
    vtss_port_if_group_counters_t      *if_group;
    vtss_port_ethernet_like_counters_t *elike;
    vtss_port_proprietary_counters_t   *prop;
    BOOL                               tx_aging_clear = clear;

    /* Setup counter view */
    SRVL_WR(VTSS_SYS_SYSTEM_STAT_CFG, VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_VIEW(port));

    base = 0x00;
    VTSS_RC(srvl_counter_update(p, &c->rx_octets, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_unicast, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_multicast, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_broadcast, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_shorts, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_fragments, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_jabbers, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_crc_align_errors, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_symbol_errors, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_64, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_65_127, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_128_255, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_256_511, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_512_1023, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_1024_1526, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_1527_max, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_pause, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_control, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_longs, clear));
    VTSS_RC(srvl_counter_update(p, &c->rx_classified_drops, clear));
    for (i = 0; i < VTSS_PRIOS; i++) 
        VTSS_RC(srvl_counter_update(p, &c->rx_red_class[i], clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(srvl_counter_update(p, &c->rx_yellow_class[i], clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(srvl_counter_update(p, &c->rx_green_class[i], clear));
    
    base = 0x40;
    VTSS_RC(srvl_counter_update(p, &c->tx_octets, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_unicast, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_multicast, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_broadcast, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_collision, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_drops, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_pause, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_64, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_65_127, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_128_255, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_256_511, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_512_1023, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_1024_1526, clear));
    VTSS_RC(srvl_counter_update(p, &c->tx_1527_max, clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(srvl_counter_update(p, &c->tx_yellow_class[i], clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(srvl_counter_update(p, &c->tx_green_class[i], clear));
#if defined(VTSS_FEATURE_EVC)
    if (port_no != VTSS_PORT_NO_NONE && vtss_state->evc_port_info[port_no].uni_count) {
        /* BZ 10411: Clear Tx age counter to ignore Up MEP discarded frames on UNI */
        tx_aging_clear = 1;
    }
#endif /* VTSS_FEATURE_EVC */
    VTSS_RC(srvl_counter_update(p, &c->tx_aging, tx_aging_clear));

    /* 32-bit Drop chip counters */
    base = 0x80;
    VTSS_RC(srvl_counter_update(p, &c->dr_local, clear));
    VTSS_RC(srvl_counter_update(p, &c->dr_tail, clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(srvl_counter_update(p, &c->dr_yellow_class[i], clear));
    for (i = 0; i < VTSS_PRIOS; i++)
        VTSS_RC(srvl_counter_update(p, &c->dr_green_class[i], clear));
   
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
        prop->rx_prio[i] = (c->rx_red_class[i].value + c->rx_yellow_class[i].value + c->rx_green_class[i].value);
        prop->tx_prio[i] = (c->tx_yellow_class[i].value + c->tx_green_class[i].value);
    }

    /* RMON Rx counters */
    rmon->rx_etherStatsDropEvents = c->dr_tail.value;
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
    rmon->tx_etherStatsPkts = 
        (c->tx_64.value + c->tx_65_127.value + c->tx_128_255.value + c->tx_256_511.value + 
         c->tx_512_1023.value + c->tx_1024_1526.value + c->tx_1527_max.value);
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
    if_group->ifInErrors = 
        (c->rx_crc_align_errors.value + c->rx_shorts.value + c->rx_fragments.value +
         c->rx_jabbers.value + c->rx_longs.value);
  
    /* Interfaces Group Tx counters */
    if_group->ifOutOctets = c->tx_octets.value;
    if_group->ifOutUcastPkts = c->tx_unicast.value;
    if_group->ifOutMulticastPkts = c->tx_multicast.value;
    if_group->ifOutBroadcastPkts = c->tx_broadcast.value;
    if_group->ifOutNUcastPkts = (c->tx_multicast.value + c->tx_broadcast.value);
    if_group->ifOutErrors = (c->tx_drops.value + c->tx_aging.value);

    /* Ethernet-like counters */
    elike->dot3InPauseFrames = c->rx_pause.value;
    elike->dot3OutPauseFrames = c->tx_pause.value;     

    /* Bridge counters */
    counters->bridge.dot1dTpPortInDiscards = (c->rx_classified_drops.value + c->dr_local.value);

    return VTSS_RC_OK;
}

static vtss_rc srvl_port_basic_counters_get(const vtss_port_no_t port_no,
                                            vtss_basic_counters_t *const counters)
{
    u32                          base, *p = &base, port = VTSS_CHIP_PORT(port_no);
    vtss_port_luton26_counters_t *c = &vtss_state->port_counters[port_no].counter.luton26;

    /* Setup counter view */
    SRVL_WR(VTSS_SYS_SYSTEM_STAT_CFG, VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_VIEW(port));
    
    /* Rx Counters */
    base = 0x09; /* rx_64 */
    VTSS_RC(srvl_counter_update(p, &c->rx_64, 0));
    VTSS_RC(srvl_counter_update(p, &c->rx_65_127, 0));
    VTSS_RC(srvl_counter_update(p, &c->rx_128_255, 0));
    VTSS_RC(srvl_counter_update(p, &c->rx_256_511, 0));
    VTSS_RC(srvl_counter_update(p, &c->rx_512_1023, 0));
    VTSS_RC(srvl_counter_update(p, &c->rx_1024_1526, 0));
    VTSS_RC(srvl_counter_update(p, &c->rx_1527_max, 0));

    /* Tx Counters */
    base = 0x47; /* tx_64 */
    VTSS_RC(srvl_counter_update(p, &c->tx_64, 0));
    VTSS_RC(srvl_counter_update(p, &c->tx_65_127, 0));
    VTSS_RC(srvl_counter_update(p, &c->tx_128_255, 0));
    VTSS_RC(srvl_counter_update(p, &c->tx_256_511, 0));
    VTSS_RC(srvl_counter_update(p, &c->tx_512_1023, 0));
    VTSS_RC(srvl_counter_update(p, &c->tx_1024_1526, 0));
    VTSS_RC(srvl_counter_update(p, &c->tx_1527_max, 0));

    /* Rx frames */
    counters->rx_frames = 
        (c->rx_64.value + c->rx_65_127.value + c->rx_128_255.value + c->rx_256_511.value + 
         c->rx_512_1023.value + c->rx_1024_1526.value + c->rx_1527_max.value);
    
    /* Tx frames */
    counters->tx_frames = 
        (c->tx_64.value + c->tx_65_127.value + c->tx_128_255.value + c->tx_256_511.value + 
         c->tx_512_1023.value + c->tx_1024_1526.value + c->tx_1527_max.value);

    return VTSS_RC_OK;
}

static vtss_rc srvl_port_counters_cmd(const vtss_port_no_t        port_no, 
                                      vtss_port_counters_t *const counters, 
                                      BOOL                        clear)
{
    return srvl_port_counters_read(port_no,
                                   VTSS_CHIP_PORT(port_no),
                                   &vtss_state->port_counters[port_no].counter.luton26,
                                   counters,
                                   clear);
}

static vtss_rc srvl_port_counters_update(const vtss_port_no_t port_no)
{
    return srvl_port_counters_cmd(port_no, NULL, 0);
}

static vtss_rc srvl_port_counters_clear(const vtss_port_no_t port_no)
{
    return srvl_port_counters_cmd(port_no, NULL, 1);
}

static vtss_rc srvl_port_counters_get(const vtss_port_no_t port_no,
                                      vtss_port_counters_t *const counters)
{
    memset(counters, 0, sizeof(*counters)); 
    return srvl_port_counters_cmd(port_no, counters, 0);
}

static void srvl_debug_fld_nl(const vtss_debug_printf_t pr, const char *name, u32 value)
{
    pr("%-20s: %u\n", name, value);
}

#define SRVL_GET_FLD(tgt, addr, fld, value)  VTSS_X_##tgt##_##addr##_##fld(value)
#define SRVL_GET_BIT(tgt, addr, fld, value)  (VTSS_F_##tgt##_##addr##_##fld & (value) ? 1 : 0)

#define SRVL_DEBUG_HSIO(pr, addr, name) srvl_debug_reg(pr, VTSS_HSIO_##addr, name)
#define SRVL_DEBUG_MAC(pr, addr, i, name) srvl_debug_reg_inst(pr, VTSS_DEV_MAC_CFG_STATUS_MAC_##addr, i, "MAC_"name)
#define SRVL_DEBUG_PCS(pr, addr, i, name) srvl_debug_reg_inst(pr, VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_##addr, i, "PCS_"name)

#define SRVL_DEBUG_HSIO_BIT(pr, addr, fld, value) srvl_debug_fld_nl(pr, #fld, SRVL_GET_BIT(HSIO, addr, fld, x))
#define SRVL_DEBUG_HSIO_FLD(pr, addr, fld, value) srvl_debug_fld_nl(pr, #fld, SRVL_GET_FLD(HSIO, addr, fld, x))
#define SRVL_DEBUG_RAW(pr, offset, length, value, name) srvl_debug_fld_nl(pr, name, VTSS_EXTRACT_BITFIELD(value, offset, length))

static vtss_rc srvl_debug_port(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    u32            port, tgt, inst, x;
    vtss_port_no_t port_no;
    char           buf[32];
    BOOL           serdes6g;
    
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        if ((port_no = vtss_cmn_port2port_no(info, port)) == VTSS_PORT_NO_NONE)
            continue;
        sprintf(buf, "Port %u (%u)", port, port_no);
        srvl_debug_reg_header(pr, buf);
        tgt = VTSS_TO_DEV(port);
        srvl_debug_reg_inst(pr, VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), port, "CLOCK_CFG");
        SRVL_DEBUG_MAC(pr, ENA_CFG(tgt), port, "ENA_CFG");
        SRVL_DEBUG_MAC(pr, MODE_CFG(tgt), port, "MODE_CFG");
        SRVL_DEBUG_MAC(pr, MAXLEN_CFG(tgt), port, "MAXLEN_CFG");
        SRVL_DEBUG_MAC(pr, TAGS_CFG(tgt), port, "TAGS_CFG");
        srvl_debug_reg_inst(pr, VTSS_SYS_PAUSE_CFG_MAC_FC_CFG(port), port, "FC_CFG");
        SRVL_DEBUG_PCS(pr, CFG(tgt), port, "CFG");
        SRVL_DEBUG_PCS(pr, MODE_CFG(tgt), port, "MODE_CFG");
        SRVL_DEBUG_PCS(pr, SD_CFG(tgt), port, "SD_CFG");
        SRVL_DEBUG_PCS(pr, ANEG_CFG(tgt), port, "ANEG_CFG");
        SRVL_DEBUG_PCS(pr, ANEG_STATUS(tgt), port, "ANEG_STATUS");
        SRVL_DEBUG_PCS(pr, LINK_STATUS(tgt), port, "LINK_STATUS");
        srvl_debug_reg_inst(pr, VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt), port, 
                           "PCS_FX100_CFG");
        srvl_debug_reg_inst(pr, VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), port, 
                           "FX100_STATUS");
        pr("\n");

        if (!info->full || srvl_serdes_inst_get(port, &inst, &serdes6g) != VTSS_RC_OK)
            continue;
        
        if (serdes6g) {
            sprintf(buf, "SerDes6G_%u", inst);
            srvl_debug_reg_header(pr, buf);
            VTSS_RC(srvl_sd6g_read(1 << inst));
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, "DES_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, "IB_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG1, "IB_CFG1");
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, "OB_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, "OB_CFG1");
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, "SER_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, "COMMON_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, "PLL_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_ANA_STATUS_SERDES6G_PLL_STATUS, "PLL_STATUS");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DIG_CFG, "DIG_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG0, "DFT_CFG0");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG1, "DFT_CFG1");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_DFT_CFG2, "DFT_CFG2");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_TP_CFG0, "TP_CFG0");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_TP_CFG1, "TP_CFG1");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, "MISC_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_CFG_SERDES6G_OB_ANEG_CFG, "OB_ANEG_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES6G_DIG_STATUS_SERDES6G_DFT_STATUS, "DFT_STATUS");

            pr("\n%s:OB_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG, &x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_ENA1V_MODE, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_POL, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_POST0, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_PREC, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_SR_H, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG, OB_SR, x);

            pr("\n%s:OB_CFG1:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, &x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, OB_ENA_CAS, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_OB_CFG1, OB_LEV, x);

            pr("\n%s:DES_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_DES_CFG, &x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, DES_MBTR_CTRL, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, DES_CPMD_SEL, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_DES_CFG, DES_BW_ANA, x);

            pr("\n%s:IB_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_IB_CFG, &x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_IC_AC, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_RF, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_RT, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_IB_CFG, IB_VBAC, x);
            
            pr("\n%s:SER_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_SER_CFG, &x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_SER_CFG, SER_ENALI, x);

            pr("\n%s:PLL_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, &x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_DIV4, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_ENA_ROT, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_FSM_CTRL_DATA, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_FSM_ENA, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_ROT_DIR, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_PLL_CFG, PLL_ROT_FRQ, x);

            pr("\n%s:COMMON_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, &x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, ENA_LANE, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, HRATE, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, QRATE, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG, IF_MODE, x);
        } else {
            sprintf(buf, "SerDes1G_%u", inst);
            srvl_debug_reg_header(pr, buf);
            VTSS_RC(srvl_sd1g_read(1 << inst));
            SRVL_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_DES_CFG, "DES_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_IB_CFG, "IB_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_OB_CFG, "OB_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_SER_CFG, "SER_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, "COMMON_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES1G_ANA_CFG_SERDES1G_PLL_CFG, "PLL_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES1G_ANA_STATUS_SERDES1G_PLL_STATUS, "PLL_STATUS");
            SRVL_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG0, "DFT_CFG0");
            SRVL_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG1, "DFT_CFG1");
            SRVL_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_DFT_CFG2, "DFT_CFG2");
            SRVL_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_TP_CFG, "TP_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES1G_DIG_CFG_SERDES1G_MISC_CFG, "MISC_CFG");
            SRVL_DEBUG_HSIO(pr, SERDES1G_DIG_STATUS_SERDES1G_DFT_STATUS, "DFT_STATUS");


            pr("\n%s:IB_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_IB_CFG, &x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES1G_ANA_CFG_SERDES1G_IB_CFG, IB_DET_LEV, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES1G_ANA_CFG_SERDES1G_IB_CFG, IB_ENA_DC_COUPLING, x);
            SRVL_DEBUG_HSIO_BIT(pr, SERDES1G_ANA_CFG_SERDES1G_IB_CFG, IB_ENA_DETLEV, x);

            pr("\n%s:DES_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_DES_CFG, &x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES1G_ANA_CFG_SERDES1G_DES_CFG, DES_MBTR_CTRL, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES1G_ANA_CFG_SERDES1G_DES_CFG, DES_CPMD_SEL, x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES1G_ANA_CFG_SERDES1G_DES_CFG, DES_BW_ANA, x);

            pr("\n%s:OB_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_OB_CFG, &x);
            SRVL_DEBUG_HSIO_FLD(pr, SERDES1G_ANA_CFG_SERDES1G_OB_CFG, OB_AMP_CTRL, x);

            pr("\n%s:COMMON_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, &x);
            SRVL_DEBUG_RAW(pr, 7, 1, x, "HRATE");
            SRVL_DEBUG_HSIO_BIT(pr, SERDES1G_ANA_CFG_SERDES1G_COMMON_CFG, ENA_LANE, x);
             
            pr("\n%s:PLL_CFG:\n", buf);
            SRVL_RD(VTSS_HSIO_SERDES1G_ANA_CFG_SERDES1G_PLL_CFG, &x);
            SRVL_DEBUG_RAW(pr, 21, 1, x, "PLL_FSM_RC_DIV2");
            SRVL_DEBUG_HSIO_FLD(pr, SERDES1G_ANA_CFG_SERDES1G_PLL_CFG, PLL_FSM_CTRL_DATA, x);
                      
        }
        pr("\n");
    }
    pr("\n");

    return VTSS_RC_OK;
}

static void srvl_debug_cnt(const vtss_debug_printf_t pr, const char *col1, const char *col2, 
                           vtss_chip_counter_t *c1, vtss_chip_counter_t *c2)
{
    char buf[80];
    
    sprintf(buf, "rx_%s:", col1);
    pr("%-28s%10llu   ", buf, c1->prev);
    if (col2 != NULL) {
        sprintf(buf, "tx_%s:", strlen(col2) ? col2 : col1);
        pr("%-28s%10llu", buf, c2->prev);
    }
    pr("\n");
}

static void srvl_debug_cnt_inst(const vtss_debug_printf_t pr, u32 i,
                                const char *col1, const char *col2, 
                                vtss_chip_counter_t *c1, vtss_chip_counter_t *c2)
{
    char buf[80];
    
    sprintf(buf, "%s_%u", col1, i);
    srvl_debug_cnt(pr, buf, col2, c1, c2);
}

static vtss_rc srvl_debug_port_cnt(const vtss_debug_printf_t pr,
                                   const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */ // Due to VTSS_EXIT_ENTER
    vtss_port_no_t               port_no = 0;
    u32                          i, port;
    vtss_port_luton26_counters_t *cnt;
    BOOL                         cpu_port;
    
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        cpu_port = (port == VTSS_CHIP_PORT_CPU);
        if (cpu_port) {
            /* CPU port */
            if (!info->full)
                continue;
            cnt = &vtss_state->cpu_counters.counter.luton26;
            port_no = VTSS_PORT_NO_NONE;
        } else {
            /* Normal port */
            if ((port_no = vtss_cmn_port2port_no(info, port)) == VTSS_PORT_NO_NONE)
                continue;
            cnt = &vtss_state->port_counters[port_no].counter.luton26;
        }
        VTSS_RC(srvl_port_counters_read(port_no, port, cnt, NULL, 0));
        VTSS_EXIT_ENTER();

        /* Basic counters */
        if (cpu_port) {
            pr("Counters CPU port: %u\n\n", port);
        } else {
            pr("Counters for port: %u (%u):\n\n", port, port_no);
            srvl_debug_cnt(pr, "oct", "", &cnt->rx_octets, &cnt->tx_octets);
            srvl_debug_cnt(pr, "uc", "", &cnt->rx_unicast, &cnt->tx_unicast);
            srvl_debug_cnt(pr, "mc", "", &cnt->rx_multicast, &cnt->tx_multicast);
            srvl_debug_cnt(pr, "bc", "", &cnt->rx_broadcast, &cnt->tx_broadcast);
        }

        /* Detailed counters */
        if (info->full) {
            if (!cpu_port) {
                srvl_debug_cnt(pr, "pause", "", &cnt->rx_pause, &cnt->tx_pause);
                srvl_debug_cnt(pr, "64", "", &cnt->rx_64, &cnt->tx_64);
                srvl_debug_cnt(pr, "65_127", "", &cnt->rx_65_127, &cnt->tx_65_127);
                srvl_debug_cnt(pr, "128_255", "", &cnt->rx_128_255, &cnt->tx_128_255);
                srvl_debug_cnt(pr, "256_511", "", &cnt->rx_256_511, &cnt->tx_256_511);
                srvl_debug_cnt(pr, "512_1023", "", &cnt->rx_512_1023, &cnt->tx_512_1023);
                srvl_debug_cnt(pr, "1024_1526", "", &cnt->rx_1024_1526, &cnt->tx_1024_1526);
                srvl_debug_cnt(pr, "jumbo", "", &cnt->rx_1527_max, &cnt->tx_1527_max);
            }
            srvl_debug_cnt(pr, "cat_drop", cpu_port ? NULL : "drops", 
                           &cnt->rx_classified_drops, &cnt->tx_drops);
            srvl_debug_cnt(pr, "dr_local", cpu_port ? NULL : "aged", 
                           &cnt->dr_local, &cnt->tx_aging);
            srvl_debug_cnt(pr, "dr_tail", NULL, &cnt->dr_tail, NULL);
            if (!cpu_port) {
                srvl_debug_cnt(pr, "crc", NULL, &cnt->rx_crc_align_errors, NULL);
                srvl_debug_cnt(pr, "symbol", NULL, &cnt->rx_symbol_errors, NULL);
                srvl_debug_cnt(pr, "short", NULL, &cnt->rx_shorts, NULL);
                srvl_debug_cnt(pr, "long", NULL, &cnt->rx_longs, NULL);
                srvl_debug_cnt(pr, "frag", NULL, &cnt->rx_fragments, NULL);
                srvl_debug_cnt(pr, "jabber", NULL, &cnt->rx_jabbers, NULL);
                srvl_debug_cnt(pr, "control", NULL, &cnt->rx_control, NULL);
            }
            for (i = 0; i < VTSS_PRIOS; i++)
                srvl_debug_cnt_inst(pr, i, "green", "", 
                                    &cnt->rx_green_class[i], &cnt->tx_green_class[i]);
            for (i = 0; i < VTSS_PRIOS; i++)
                srvl_debug_cnt_inst(pr, i, "yellow", "", 
                                    &cnt->rx_yellow_class[i], &cnt->tx_yellow_class[i]);
            for (i = 0; i < VTSS_PRIOS; i++) 
                srvl_debug_cnt_inst(pr, i, "red", NULL, &cnt->rx_red_class[i], NULL);
            for (i = 0; i < VTSS_PRIOS; i++)
                srvl_debug_cnt_inst(pr, i, "dr_green", NULL, &cnt->dr_green_class[i], NULL);
            for (i = 0; i < VTSS_PRIOS; i++)
                srvl_debug_cnt_inst(pr, i, "dr_yellow", NULL, &cnt->dr_yellow_class[i], NULL);
        }
        pr("\n");
    }

    return VTSS_RC_OK;
}

static u32 WmDec(u32 value) {
    if (value > 2048) { 
        return (value - 2048) * 16;
    }        
    return value;
}


static void srvl_debug_wm_dump (const vtss_debug_printf_t pr, const char *reg_name, u32 *value, u32 i, BOOL decode) {
    u32 q;
    pr("%-25s",reg_name);
    for (q = 0; q <i; q++) {
            pr("%6u ",decode?WmDec(value[q])*SRVL_BUFFER_CELL_SZ:value[q]);
    }
    pr("\n");
}

static vtss_rc srvl_debug_wm(const vtss_debug_printf_t pr,
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
            if ((port_no = vtss_cmn_port2port_no(info, port)) == VTSS_PORT_NO_NONE)
                continue;

            SRVL_RD(VTSS_SYS_PAUSE_CFG_MAC_FC_CFG(VTSS_CHIP_PORT(port_no)), &value);
            pr("FC Tx ena     : %d\n", (value & VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_TX_FC_ENA) ? 1 : 0);
            pr("FC Rx ena     : %d\n", (value & VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_RX_FC_ENA) ? 1 : 0);
            pr("FC Value      : 0x%x\n", VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_PAUSE_VAL_CFG(value));
            pr("FC Zero pause : %d\n", (value & VTSS_F_SYS_PAUSE_CFG_MAC_FC_CFG_ZERO_PAUSE_ENA) ? 1 : 0);          
            SRVL_RD(VTSS_SYS_PAUSE_CFG_PAUSE_CFG(VTSS_CHIP_PORT(port_no)), &value);
            pr("FC Start      : 0x%x\n",  VTSS_X_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_START(value));
            pr("FC Stop       : 0x%x\n", VTSS_X_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_STOP(value));
            pr("FC Ena        : %d\n", (value & VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_ENA) ? 1 : 0);
            SRVL_RD(VTSS_SYS_PAUSE_CFG_ATOP(VTSS_CHIP_PORT(port_no)), &value);
            pr("FC Atop       : 0x%x\n", VTSS_F_SYS_PAUSE_CFG_ATOP_ATOP(value));                  
            SRVL_RD(VTSS_SYS_PAUSE_CFG_PAUSE_TOT_CFG, &value);
            pr("FC TOT_START  : 0x%x\n", VTSS_X_SYS_PAUSE_CFG_PAUSE_TOT_CFG_PAUSE_TOT_START(value));
            pr("FC TOT_STOP   : 0x%x\n", VTSS_X_SYS_PAUSE_CFG_PAUSE_TOT_CFG_PAUSE_TOT_STOP(value));
            SRVL_RD(VTSS_SYS_PAUSE_CFG_ATOP_TOT_CFG, &value);
            pr("FC ATOP_TOT   : 0x%x\n", VTSS_F_SYS_PAUSE_CFG_ATOP_TOT_CFG_ATOP_TOT(value));
            pr("\n");
        }
        
        for (q = 0; q < VTSS_PRIOS; q++) {
            id[q] = q;
            SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((port * VTSS_PRIOS + q + 0)),   &val1[q]);
            SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((port * VTSS_PRIOS + q + 256)), &val2[q]);
            SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((port * VTSS_PRIOS + q + 512)), &val3[q]);
            SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((port * VTSS_PRIOS + q + 768)), &val4[q]);
        }
        
        srvl_debug_wm_dump(pr, "Queue level:", id, 8, 0);
        srvl_debug_wm_dump(pr, "Qu Ingr Buf Rsrv (Bytes) :", val1, 8, 1);
        srvl_debug_wm_dump(pr, "Qu Ingr Ref Rsrv (Frames):", val2, 8, 0);
        srvl_debug_wm_dump(pr, "Qu Egr Buf Rsrv  (Bytes) :", val3, 8, 1);
        srvl_debug_wm_dump(pr, "Qu Egr Ref Rsrv  (Frames):", val4, 8, 0);
        pr("\n");
            
        /* Shared space for all QoS classes */
        for (q = 0; q < VTSS_PRIOS; q++) {
            SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((q + 216 + 0)),   &val1[q]);
            SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((q + 216 + 256)), &val2[q]);
            SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((q + 216 + 512)), &val3[q]);
            SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((q + 216 + 768)), &val4[q]);
        }
        srvl_debug_wm_dump(pr, "QoS level:", id, 8, 0);
        srvl_debug_wm_dump(pr, "QoS Ingr Buf Rsrv (Bytes) :", val1, 8, 1);
        srvl_debug_wm_dump(pr, "QoS Ingr Ref Rsrv (Frames):", val2, 8, 0);
        srvl_debug_wm_dump(pr, "QoS Egr Buf Rsrv  (Bytes) :", val3, 8, 1);
        srvl_debug_wm_dump(pr, "QoS Egr Ref Rsrv  (Frames):", val4, 8, 0);
        pr("\n");
        /* Configure reserved space for port */
        SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((port + 224 +   0)), &val1[0]);
        SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((port + 224 + 256)), &val2[0]);
        SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((port + 224 + 512)), &val3[0]);
        SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((port + 224 + 768)), &val4[0]);
        pr("Port level:\n");
        pr("Port Ingress Buf Rsrv: %u Bytes\n", WmDec(val1[0])*SRVL_BUFFER_CELL_SZ);
        pr("Port Ingress Ref Rsrv: %u Frames\n", val2[0]);
        pr("Port Egress  Buf Rsrv: %u Bytes\n", WmDec(val3[0])*SRVL_BUFFER_CELL_SZ);
        pr("Port Egress  Ref Rsrv: %u Frames\n", val4[0]);
        pr("\n");
        
        pr("Color level:\n");
        /* Configure shared space for  both DP levels (green:0 yellow:1) */
            for (dp = 0; dp < 2; dp++) {
                SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((dp + 254 +   0)), &val1[0]);
                SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((dp + 254 + 256)), &val2[0]);
                SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((dp + 254 + 512)), &val3[0]);
                SRVL_RD(VTSS_QSYS_RES_CTRL_RES_CFG((dp + 254 + 768)), &val4[0]);
                pr("Port DP:%6s Ingress Buf Rsrv: 0x%x\n",dp?"Green":"Yellow",val1[0]);
                pr("Port DP:%6s Ingress Ref Rsrv: 0x%x\n",dp?"Green":"Yellow",val2[0]);
                pr("Port DP:%6s Egress  Buf Rsrv: 0x%x\n",dp?"Green":"Yellow",val3[0]);
                pr("Port DP:%6s Egress  Ref Rsrv: 0x%x\n",dp?"Green":"Yellow",val4[0]);
            }
            pr("\n");
    }

    return VTSS_RC_OK;
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
} srvl_policer_conf_t;

static vtss_rc srvl_policer_conf_set(u32 policer, srvl_policer_conf_t *conf)
{
    u32 cir = 0, cbs = 0, pir, pbs, mode = POL_MODE_LINERATE;
    u32 cir_ena = 0, cf = 0, pbs_max, cbs_max = 0, lvl = 0;

    pir = conf->eir;
    pbs = conf->ebs;
    if (conf->frame_rate) {
        /* Frame rate policing (single leaky bucket) */
        if (pir >= 100) {
            mode = POL_MODE_FRMRATE_100FPS;
            pir = SRVL_DIV_ROUND_UP(pir, 100); /* Resolution is in steps of 100 fps */
            pbs = (pbs * 10 / 328);            /* Burst unit is 32.8 frames */
            pbs_max = VTSS_BITMASK(6);         /* Limit burst to the maximum value */
        } else {
            mode = POL_MODE_FRMRATE_1FPS;
            pbs = (pbs * 10 / 3); /* Burst unit is 0.3 frames */
            pbs_max = 60;         /* See Bugzilla#4944, comment#2 */
            if (pir == 0 && pbs == 0) {
                /* Discard all frames */
                lvl = VTSS_M_ANA_POL_POL_PIR_STATE_PIR_LVL;
            }
        }
        pbs++; /* Round up burst size */
    } else {
        /* Bit rate policing */
        if (conf->dual) {
            /* Dual leaky bucket mode */
            cir = SRVL_DIV_ROUND_UP(conf->cir, 100); /* Rate unit is 100 kbps, round up */
            cbs = (conf->cbs ? conf->cbs : 1);       /* BZ 9813: Avoid using zero burst size */
            cbs = SRVL_DIV_ROUND_UP(cbs, 4096);      /* Burst unit is 4kB, round up */
            cbs_max = 61;                            /* See Bugzilla#4944, comment#2  */
            cir_ena = 1;
            cf = conf->cf;
            if (cf)
                pir += conf->cir;
        }
        pir = SRVL_DIV_ROUND_UP(pir, 100);  /* Rate unit is 100 kbps, round up */
        pbs = (pbs ? pbs : 1);              /* BZ 9813: Avoid using zero burst size */
        pbs = SRVL_DIV_ROUND_UP(pbs, 4096); /* Burst unit is 4kB, round up */
        pbs_max = 61;                       /* See Bugzilla#4944, comment#2  */
    }

    /* Policer rate fix for Revision B or later. See Bugzilla#8648 */
    if (vtss_state->chip_id.revision > 0) {
        pir *= 3;
        cir *= 3;
    }

    /* Limit to maximum values */
    pir = MIN(VTSS_BITMASK(15), pir);
    cir = MIN(VTSS_BITMASK(15), cir);
    pbs = MIN(pbs_max, pbs);
    cbs = MIN(cbs_max, cbs);

    SRVL_WR(VTSS_ANA_POL_POL_MODE_CFG(policer),
            VTSS_F_ANA_POL_POL_MODE_CFG_IPG_SIZE(20) |
            VTSS_F_ANA_POL_POL_MODE_CFG_FRM_MODE(mode) |
            (cf ? VTSS_F_ANA_POL_POL_MODE_CFG_DLB_COUPLED : 0) |
            (cir_ena ? VTSS_F_ANA_POL_POL_MODE_CFG_CIR_ENA : 0) |
            VTSS_F_ANA_POL_POL_MODE_CFG_OVERSHOOT_ENA);
    SRVL_WR(VTSS_ANA_POL_POL_PIR_CFG(policer), 
            VTSS_F_ANA_POL_POL_PIR_CFG_PIR_RATE(pir) |
            VTSS_F_ANA_POL_POL_PIR_CFG_PIR_BURST(pbs));
    SRVL_WR(VTSS_ANA_POL_POL_CIR_CFG(policer), 
            VTSS_F_ANA_POL_POL_CIR_CFG_CIR_RATE(cir) |
            VTSS_F_ANA_POL_POL_CIR_CFG_CIR_BURST(cbs));
    SRVL_WR(VTSS_ANA_POL_POL_PIR_STATE(policer), lvl); /* Reset current fill level */
    SRVL_WR(VTSS_ANA_POL_POL_CIR_STATE(policer), lvl); /* Reset current fill level */

    return VTSS_RC_OK;
}

#define SRVL_DEFAULT_POL_ORDER 0x1d3 /* Serval policer order: Serial (QoS -> Port -> VCAP) */

static vtss_rc srvl_port_policer_fc_set(const vtss_port_no_t port_no, u32 chipport)
{
    vtss_port_conf_t *port_conf = &vtss_state->port_conf[port_no];
    vtss_qos_port_conf_t *qos_conf = &vtss_state->qos_port_conf[port_no];

    SRVL_WR(VTSS_ANA_POL_MISC_POL_FLOWC(chipport),
            (port_conf->flow_control.generate &&
             (qos_conf->policer_port[0].rate != VTSS_BITRATE_DISABLED) &&
             qos_conf->policer_ext_port[0].flow_control) ? 1 : 0);

    return VTSS_RC_OK;
}

static vtss_rc srvl_port_policer_set(u32 port, BOOL enable, srvl_policer_conf_t *conf)
{
    u32  order      = SRVL_DEFAULT_POL_ORDER;

    VTSS_RC(srvl_policer_conf_set(SRVL_POLICER_PORT + port, conf));

    SRVL_WRM(VTSS_ANA_PORT_POL_CFG(port),
             (enable ? VTSS_F_ANA_PORT_POL_CFG_PORT_POL_ENA : 0) |
             VTSS_F_ANA_PORT_POL_CFG_POL_ORDER(order),
             VTSS_F_ANA_PORT_POL_CFG_PORT_POL_ENA |
             VTSS_M_ANA_PORT_POL_CFG_POL_ORDER);

    return VTSS_RC_OK;
}

static vtss_rc srvl_queue_policer_set(u32 port, u32 queue, BOOL enable, srvl_policer_conf_t *conf)
{
    VTSS_RC(srvl_policer_conf_set(SRVL_POLICER_QUEUE + port * 8 + queue, conf));

    SRVL_WRM(VTSS_ANA_PORT_POL_CFG(port),
             (enable       ? VTSS_F_ANA_PORT_POL_CFG_QUEUE_POL_ENA(VTSS_BIT(queue)): 0),
             VTSS_F_ANA_PORT_POL_CFG_QUEUE_POL_ENA(VTSS_BIT(queue)));

    return VTSS_RC_OK;
}

static u32 srvl_packet_rate(vtss_packet_rate_t rate, u32 *unit)
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

static u32 srvl_chip_prio(const vtss_prio_t prio)
{
    if (prio >= SRVL_PRIOS) {
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

static vtss_rc srvl_qos_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_qos_port_conf_t *conf = &vtss_state->qos_port_conf[port_no];
    u32                  port = VTSS_CHIP_PORT(port_no);
    int                  pcp, dei, queue, class, dpl;
    u8                   cost[6];
    srvl_policer_conf_t  pol_cfg;
    u32                  tag_remark_mode, value;
    BOOL                 tag_default_dei;

    /* Port default PCP and DEI configuration */
    SRVL_WRM(VTSS_ANA_PORT_VLAN_CFG(port),
             (conf->default_dei ? VTSS_F_ANA_PORT_VLAN_CFG_VLAN_DEI : 0) |
             VTSS_F_ANA_PORT_VLAN_CFG_VLAN_PCP(conf->usr_prio),
             VTSS_F_ANA_PORT_VLAN_CFG_VLAN_DEI                           |
             VTSS_M_ANA_PORT_VLAN_CFG_VLAN_PCP);

    /* Port default QoS class, DP level, tagged frames mode, DSCP mode and DSCP remarking configuration */
    SRVL_WR(VTSS_ANA_PORT_QOS_CFG(port),
            (conf->default_dpl ? VTSS_F_ANA_PORT_QOS_CFG_DP_DEFAULT_VAL : 0)            |
            VTSS_F_ANA_PORT_QOS_CFG_QOS_DEFAULT_VAL(srvl_chip_prio(conf->default_prio)) |
            (conf->tag_class_enable ? VTSS_F_ANA_PORT_QOS_CFG_QOS_PCP_ENA : 0)          |
            (conf->dscp_class_enable ? VTSS_F_ANA_PORT_QOS_CFG_QOS_DSCP_ENA : 0)        |
            (conf->dscp_translate ? VTSS_F_ANA_PORT_QOS_CFG_DSCP_TRANSLATE_ENA : 0)     |
            VTSS_F_ANA_PORT_QOS_CFG_DSCP_REWR_CFG(conf->dscp_mode));

    /* Egress DSCP remarking configuration */
    SRVL_WR(VTSS_REW_PORT_DSCP_CFG(port), VTSS_F_REW_PORT_DSCP_CFG_DSCP_REWR_CFG(conf->dscp_emode));

    /* Map for (PCP and DEI) to (QoS class and DP level */
    for (pcp = VTSS_PCP_START; pcp < VTSS_PCP_END; pcp++) {
        for (dei = VTSS_DEI_START; dei < VTSS_DEI_END; dei++) {
            SRVL_WR(VTSS_ANA_PORT_QOS_PCP_DEI_MAP_CFG(port, (8*dei + pcp)),
                    (conf->dp_level_map[pcp][dei] ? VTSS_F_ANA_PORT_QOS_PCP_DEI_MAP_CFG_DP_PCP_DEI_VAL : 0) |
                    VTSS_F_ANA_PORT_QOS_PCP_DEI_MAP_CFG_QOS_PCP_DEI_VAL(srvl_chip_prio(conf->qos_class_map[pcp][dei])));
        }
    }

    /*
     * Verify that the default scheduler configuration is used.
     * If this is not the case then this code must be modified
     * to use actual scheduler configuration.
     * See VSC7418 Target Specification, section 3.11.2, Queue Mapping, Figure 42 for details.
     *
     * P is port number (0..11)
     * Q is QoS class (0..7)
     * Level 1: SE# = 218 + P
     * Level 2: SE# = Q + (P * 8)
     */

    SRVL_RD(VTSS_QSYS_QMAP_QMAP(port), &value);
    if (value) {
        VTSS_E("Unsupported scheduler configuration (0x%x)", value);
    } else {
        u32 level_1_se = 218 + port;

        /* DWRR configuration */
        SRVL_WRM(VTSS_QSYS_HSCH_SE_CFG(level_1_se),
                 VTSS_F_QSYS_HSCH_SE_CFG_SE_DWRR_CNT((conf->dwrr_enable ? 6 : 0)), /* 6 DWRR inputs if enabled, otherwise 0 inputs (all strict) */
                 VTSS_M_QSYS_HSCH_SE_CFG_SE_DWRR_CNT);

        VTSS_RC(vtss_cmn_qos_weight2cost(conf->queue_pct, cost, 6, 5));
        for (queue = 0; queue < 6; queue++) {
            SRVL_WR(VTSS_QSYS_HSCH_SE_DWRR_CFG(level_1_se, queue), cost[queue]);
        }

        /* Egress port/queue shaper rate configuration
         * The value (in kbps) is rounded up to the next possible value:
         *        0 -> 0 (Open until burst capacity is used, then closed)
         *   1..100 -> 1 (100 kbps)
         * 101..200 -> 2 (200 kbps)
         * 201..300 -> 3 (300 kbps)
         */

        /* Egress port/queue shaper burst level configuration
         * The value is rounded up to the next possible value:
         *           0 -> 0 (Shaper disabled)
         *    1.. 4096 -> 1 ( 4 KB)
         * 4097.. 8192 -> 2 ( 8 KB)
         * 8193..12288 -> 3 (12 KB)
         */

        /* Egress port shaper configuration */
        if (conf->shaper_port.rate != VTSS_BITRATE_DISABLED) {
            SRVL_WR(VTSS_QSYS_HSCH_CIR_CFG(level_1_se),
                    VTSS_F_QSYS_HSCH_CIR_CFG_CIR_RATE(SRVL_DIV_ROUND_UP(conf->shaper_port.rate, 100)) |
                    VTSS_F_QSYS_HSCH_CIR_CFG_CIR_BURST(SRVL_DIV_ROUND_UP(conf->shaper_port.level, 4096)));
        } else {
            SRVL_WR(VTSS_QSYS_HSCH_CIR_CFG(level_1_se), 0); /* Disable shaper */
        }

        /* Egress queue shaper configuration */
        for (queue = 0; queue < 8; queue++) {
            if (conf->shaper_queue[queue].rate != VTSS_BITRATE_DISABLED) {
                SRVL_WR(VTSS_QSYS_HSCH_CIR_CFG(queue + (port * 8)),
                        VTSS_F_QSYS_HSCH_CIR_CFG_CIR_RATE(SRVL_DIV_ROUND_UP(conf->shaper_queue[queue].rate, 100)) |
                        VTSS_F_QSYS_HSCH_CIR_CFG_CIR_BURST(SRVL_DIV_ROUND_UP(conf->shaper_queue[queue].level, 4096)));
            } else {
                SRVL_WR(VTSS_QSYS_HSCH_CIR_CFG(queue + (port * 8)), 0); /* Disable shaper */
            }
            /* Excess configuration */
            SRVL_WRM(VTSS_QSYS_HSCH_SE_CFG(queue + (port * 8)),
                     conf->excess_enable[queue] ? VTSS_F_QSYS_HSCH_SE_CFG_SE_EXC_ENA : 0,
                     VTSS_F_QSYS_HSCH_SE_CFG_SE_EXC_ENA);
        }
    }

    tag_remark_mode = conf->tag_remark_mode;
    tag_default_dei = (tag_remark_mode == VTSS_TAG_REMARK_MODE_DEFAULT ? conf->tag_default_dei : 0);

    /* Tag remarking configuration */
    SRVL_WRM(VTSS_REW_PORT_PORT_VLAN_CFG(port),
             (tag_default_dei ? VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_DEI : 0) |
             VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_PCP(conf->tag_default_pcp),
             VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_DEI                               |
             VTSS_M_REW_PORT_PORT_VLAN_CFG_PORT_PCP);

    /* Map remark mode */
    switch (tag_remark_mode) {
    case VTSS_TAG_REMARK_MODE_DEFAULT:
        tag_remark_mode = 1; /* PORT_PCP/PORT_DEI */
        break;
    case VTSS_TAG_REMARK_MODE_MAPPED:
        tag_remark_mode = 2; /* MAPPED */
        break;
    default:
        tag_remark_mode = 0; /* Classified PCP/DEI */
        break;
    }

    SRVL_WRM(VTSS_REW_PORT_TAG_CFG(port),
             VTSS_F_REW_PORT_TAG_CFG_TAG_PCP_CFG(tag_remark_mode) |
             VTSS_F_REW_PORT_TAG_CFG_TAG_DEI_CFG(tag_remark_mode),
             VTSS_M_REW_PORT_TAG_CFG_TAG_PCP_CFG |
             VTSS_M_REW_PORT_TAG_CFG_TAG_DEI_CFG);

    /* Map for (QoS class and DP level) to (PCP and DEI) */
    for (class = VTSS_QUEUE_START; class < VTSS_QUEUE_END; class++) {
        for (dpl = 0; dpl < 2; dpl++) {
            SRVL_WRM(VTSS_REW_PORT_PCP_DEI_QOS_MAP_CFG(port, (8 * dpl + class)),
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
    VTSS_RC(srvl_port_policer_set(port, conf->policer_port[0].rate != VTSS_BITRATE_DISABLED, &pol_cfg));

    /* Queue policer configuration */
    for (queue = 0; queue < 8; queue++) {
        memset(&pol_cfg, 0, sizeof(pol_cfg));
        if (conf->policer_queue[queue].rate != VTSS_BITRATE_DISABLED) {
            pol_cfg.eir = conf->policer_queue[queue].rate;
            pol_cfg.ebs = conf->policer_queue[queue].level;
        }
        VTSS_RC(srvl_queue_policer_set(port, queue, conf->policer_queue[queue].rate != VTSS_BITRATE_DISABLED, &pol_cfg));
    }

    /* Update policer flow control configuration */
    VTSS_RC(srvl_port_policer_fc_set(port_no, port));

    return VTSS_RC_OK;
}

static vtss_rc srvl_qos_conf_set(BOOL changed)
{
    vtss_qos_conf_t    *conf = &vtss_state->qos_conf;
    vtss_port_no_t     port_no;
    u32                i, unit;
    vtss_packet_rate_t rate;

    if (changed) {
        /* Number of priorities changed, update QoS setup for all ports */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            VTSS_RC(srvl_qos_port_conf_set(port_no));
        }
    }
    /* Storm control */
    SRVL_WR(VTSS_ANA_ANA_STORMLIMIT_BURST, VTSS_F_ANA_ANA_STORMLIMIT_BURST_STORM_BURST(6)); /* Burst of 64 frames allowed */
    for (i = 0; i < 3; i++) {
        rate = (i == 0 ? conf->policer_uc : i == 1 ? conf->policer_bc : conf->policer_mc);
        SRVL_WR(VTSS_ANA_ANA_STORMLIMIT_CFG(i),
                VTSS_F_ANA_ANA_STORMLIMIT_CFG_STORM_RATE(srvl_packet_rate(rate, &unit)) |
                (unit ? VTSS_F_ANA_ANA_STORMLIMIT_CFG_STORM_UNIT : 0) |
                VTSS_F_ANA_ANA_STORMLIMIT_CFG_STORM_MODE(rate == VTSS_PACKET_RATE_DISABLED ? 0 : 3)); /* Disabled or police both CPU and front port destinations */
    }

    /* DSCP classification and remarking configuration
     */
    for (i = 0; i < 64; i++) {
        SRVL_WR(VTSS_ANA_COMMON_DSCP_CFG(i),
                (conf->dscp_dp_level_map[i] ? VTSS_F_ANA_COMMON_DSCP_CFG_DP_DSCP_VAL : 0)           |
                VTSS_F_ANA_COMMON_DSCP_CFG_QOS_DSCP_VAL(srvl_chip_prio(conf->dscp_qos_class_map[i])) |
                VTSS_F_ANA_COMMON_DSCP_CFG_DSCP_TRANSLATE_VAL(conf->dscp_translate_map[i])          |
                (conf->dscp_trust[i] ? VTSS_F_ANA_COMMON_DSCP_CFG_DSCP_TRUST_ENA : 0)               |
                (conf->dscp_remark[i] ? VTSS_F_ANA_COMMON_DSCP_CFG_DSCP_REWR_ENA : 0));

        SRVL_WR(VTSS_REW_COMMON_DSCP_REMAP_CFG(i),
                VTSS_F_REW_COMMON_DSCP_REMAP_CFG_DSCP_REMAP_VAL(conf->dscp_remap[i]));

        SRVL_WR(VTSS_REW_COMMON_DSCP_REMAP_DP1_CFG(i),
                VTSS_F_REW_COMMON_DSCP_REMAP_DP1_CFG_DSCP_REMAP_DP1_VAL(conf->dscp_remap_dp1[i]));
    }

    /* DSCP classification from QoS configuration
     */
    for (i = 0; i < 8; i++) {
        SRVL_WR(VTSS_ANA_COMMON_DSCP_REWR_CFG(i),
                VTSS_F_ANA_COMMON_DSCP_REWR_CFG_DSCP_QOS_REWR_VAL(conf->dscp_qos_map[i]));

        SRVL_WR(VTSS_ANA_COMMON_DSCP_REWR_CFG(i + 8),
                VTSS_F_ANA_COMMON_DSCP_REWR_CFG_DSCP_QOS_REWR_VAL(conf->dscp_qos_map_dp1[i]));
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_qos(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    u32            i, port, pir, cir, value, mode, level_1_se;
    int            queue;
    BOOL           header = 1;
    vtss_port_no_t port_no;

    /* Global configuration starts here */

    srvl_debug_print_header(pr, "QoS Storm Control");

    SRVL_RD(VTSS_ANA_ANA_STORMLIMIT_BURST, &value);
    pr("Burst: %u\n", VTSS_X_ANA_ANA_STORMLIMIT_BURST_STORM_BURST(value));
    for (i = 0; i < 3; i++) {
        const char *name = (i == 0 ? "UC" : i == 1 ? "BC" : "MC");
        SRVL_RD(VTSS_ANA_ANA_STORMLIMIT_CFG(i), &value);
        pr("%s   : Rate %2u, Unit %u, Mode %u\n", name,
           VTSS_X_ANA_ANA_STORMLIMIT_CFG_STORM_RATE(value),
           VTSS_BOOL(value & VTSS_F_ANA_ANA_STORMLIMIT_CFG_STORM_UNIT),
           VTSS_X_ANA_ANA_STORMLIMIT_CFG_STORM_MODE(value));
    }
    pr("\n");

    srvl_debug_print_header(pr, "QoS DSCP Config");

    pr("DSCP Trans CLS DPL Rewr Trust Remap_DP0 Remap_DP1\n");
    for (i = 0; i < 64; i++) {
        u32 dscp_cfg, dscp_remap, dscp_remap_dp1;
        SRVL_RD(VTSS_ANA_COMMON_DSCP_CFG(i), &dscp_cfg);
        SRVL_RD(VTSS_REW_COMMON_DSCP_REMAP_CFG(i), &dscp_remap);
        SRVL_RD(VTSS_REW_COMMON_DSCP_REMAP_DP1_CFG(i), &dscp_remap_dp1);

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

    srvl_debug_print_header(pr, "QoS DSCP Classification from QoS Config");

    pr("QoS DSCP_DP0 DSCP_DP1\n");
    for (i = 0; i < 8; i++) {
        u32 qos_dp0, qos_dp1;
        SRVL_RD(VTSS_ANA_COMMON_DSCP_REWR_CFG(i), &qos_dp0);
        SRVL_RD(VTSS_ANA_COMMON_DSCP_REWR_CFG(i + 8), &qos_dp1);
        pr("%3u %4u     %4u\n",
           i,
           VTSS_X_ANA_COMMON_DSCP_REWR_CFG_DSCP_QOS_REWR_VAL(qos_dp0),
           VTSS_X_ANA_COMMON_DSCP_REWR_CFG_DSCP_QOS_REWR_VAL(qos_dp1));
    }
    pr("\n");

    /* Per port configuration starts here */

    srvl_debug_print_header(pr, "QoS Port Classification Config");

    pr("LP CP PCP CLS DEI DPL TC_EN DC_EN\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 vlan, qos;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        SRVL_RD(VTSS_ANA_PORT_VLAN_CFG(port), &vlan);
        SRVL_RD(VTSS_ANA_PORT_QOS_CFG(port), &qos);
        pr("%2u %2u %3u %3u %3u %3u %5u %5u\n",
           port_no, // Logical port
           port,    // Chip port
           VTSS_X_ANA_PORT_VLAN_CFG_VLAN_PCP(vlan),
           VTSS_X_ANA_PORT_QOS_CFG_QOS_DEFAULT_VAL(qos),
           VTSS_BOOL(vlan & VTSS_F_ANA_PORT_VLAN_CFG_VLAN_DEI),
           VTSS_BOOL(qos & VTSS_F_ANA_PORT_QOS_CFG_DP_DEFAULT_VAL),
           VTSS_BOOL(qos & VTSS_F_ANA_PORT_QOS_CFG_QOS_PCP_ENA),
           VTSS_BOOL(qos & VTSS_F_ANA_PORT_QOS_CFG_QOS_DSCP_ENA));
    }
    pr("\n");

    srvl_debug_print_header(pr, "QoS Port Classification PCP, DEI to QoS class, DP level Mapping");

    pr("LP CP QoS class (8*DEI+PCP)           DP level (8*DEI+PCP)\n");
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
                SRVL_RD(VTSS_ANA_PORT_QOS_PCP_DEI_MAP_CFG(port, (8*dei + pcp)), &value);
                class_ct += snprintf(class_buf + class_ct, sizeof(class_buf) - class_ct, "%s%u", delim,
                                     VTSS_X_ANA_PORT_QOS_PCP_DEI_MAP_CFG_QOS_PCP_DEI_VAL(value));
                dpl_ct   += snprintf(dpl_buf   + dpl_ct,   sizeof(dpl_buf)   - dpl_ct,   "%s%u",  delim,
                                     VTSS_BOOL(value & VTSS_F_ANA_PORT_QOS_PCP_DEI_MAP_CFG_DP_PCP_DEI_VAL));
            }
        }
        pr("%2u %2u %s %s\n", port_no, port, class_buf, dpl_buf);
    }
    pr("\n");

    srvl_debug_print_header(pr, "QoS Scheduler Config");

    pr("LP CP Base IdxSel InpSel DWRR C0 C1 C2 C3 C4 C5\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 qmap;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        level_1_se = 218 + port;
        SRVL_RD(VTSS_QSYS_QMAP_QMAP(port), &qmap);
        SRVL_RD(VTSS_QSYS_HSCH_SE_CFG(level_1_se), &value);
        pr("%2u %2u %4u %6u %6u %4u",
           port_no, // Logical port
           port,    // Chip port
           VTSS_X_QSYS_QMAP_QMAP_SE_BASE(qmap),
           VTSS_X_QSYS_QMAP_QMAP_SE_IDX_SEL(qmap),
           VTSS_X_QSYS_QMAP_QMAP_SE_INP_SEL(qmap),
           VTSS_X_QSYS_HSCH_SE_CFG_SE_DWRR_CNT(value));
        for (queue = 0; queue < 6; queue++) {
            SRVL_RD(VTSS_QSYS_HSCH_SE_DWRR_CFG(level_1_se, queue), &value);
            pr(" %2u", value);
        }
        pr("\n");
    }
    pr("\n");

    srvl_debug_print_header(pr, "QoS Port and Queue Shaper Burst and Rate Config");

    pr("LP CP Queue Burst Rate   Excess\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 excess;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        level_1_se = 218 + port;
        SRVL_RD(VTSS_QSYS_HSCH_CIR_CFG(level_1_se), &value);
        pr("%2u %2u     - 0x%02x  0x%04x      -\n      ",
           port_no,
           port,
           VTSS_X_QSYS_HSCH_CIR_CFG_CIR_BURST(value),
           VTSS_X_QSYS_HSCH_CIR_CFG_CIR_RATE(value));
        for (queue = 0; queue < 8; queue++) {
            SRVL_RD(VTSS_QSYS_HSCH_CIR_CFG(queue + (port * 8)), &value);
            SRVL_RD(VTSS_QSYS_HSCH_SE_CFG(queue + (port * 8)), &excess);
            pr("%5d 0x%02x  0x%04x %6d\n      ",
               queue,
               VTSS_X_QSYS_HSCH_CIR_CFG_CIR_BURST(value),
               VTSS_X_QSYS_HSCH_CIR_CFG_CIR_RATE(value),
               VTSS_BOOL(excess & VTSS_F_QSYS_HSCH_SE_CFG_SE_EXC_ENA));
        }
        pr("\r");
    }
    pr("\n");

    srvl_debug_print_header(pr, "QoS Port Tag Remarking Config");

    pr("LP CP MPCP MDEI PCP DEI\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 tag_default, tag_ctrl;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        SRVL_RD(VTSS_REW_PORT_PORT_VLAN_CFG(port), &tag_default);
        SRVL_RD(VTSS_REW_PORT_TAG_CFG(port), &tag_ctrl);
        pr("%2u %2u %4x %4x %3d %3d\n",
           port_no,
           port,
           VTSS_X_REW_PORT_TAG_CFG_TAG_PCP_CFG(tag_ctrl),
           VTSS_X_REW_PORT_TAG_CFG_TAG_DEI_CFG(tag_ctrl),
           VTSS_X_REW_PORT_PORT_VLAN_CFG_PORT_PCP(tag_default),
           VTSS_BOOL(tag_default & VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_DEI));
    }
    pr("\n");

    srvl_debug_print_header(pr, "QoS Port Tag Remarking Map");

    pr("LP CP PCP (2*QoS class+DPL)           DEI (2*QoS class+DPL)\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int class, dpl, pcp_ct = 0, dei_ct = 0;
        char pcp_buf[40], dei_buf[40];
        if (info->port_list[port_no] == 0)
            continue;
        port = VTSS_CHIP_PORT(port_no);
        for (class = VTSS_QUEUE_START; class < VTSS_QUEUE_END; class++) {
            for (dpl = 0; dpl < 2; dpl++) {
                const char *delim = ((class == VTSS_QUEUE_START) && (dpl == 0)) ? "" : ",";
                SRVL_RD(VTSS_REW_PORT_PCP_DEI_QOS_MAP_CFG(port, (8*dpl + class)), &value);
                pcp_ct += snprintf(pcp_buf + pcp_ct, sizeof(pcp_buf) - pcp_ct, "%s%u", delim,
                                   VTSS_X_REW_PORT_PCP_DEI_QOS_MAP_CFG_PCP_QOS_VAL(value));
                dei_ct += snprintf(dei_buf + dei_ct, sizeof(dei_buf) - dei_ct, "%s%u",  delim,
                                   VTSS_BOOL(value & VTSS_F_REW_PORT_PCP_DEI_QOS_MAP_CFG_DEI_QOS_VAL));
            }
        }
        pr("%2u %2u %s %s\n", port_no, port, pcp_buf, dei_buf);
    }
    pr("\n");

    srvl_debug_print_header(pr, "QoS Port DSCP Remarking Config");

    pr("LP CP I_Mode Trans E_Mode\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        u32 qos_cfg, dscp_cfg;
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        SRVL_RD(VTSS_ANA_PORT_QOS_CFG(port), &qos_cfg);
        SRVL_RD(VTSS_REW_PORT_DSCP_CFG(port), &dscp_cfg);
        pr("%2u %2u %6u %5u %6u\n",
           port_no,
           port,
           VTSS_X_ANA_PORT_QOS_CFG_DSCP_REWR_CFG(qos_cfg),
           VTSS_BOOL(qos_cfg & VTSS_F_ANA_PORT_QOS_CFG_DSCP_TRANSLATE_ENA),
           VTSS_X_REW_PORT_DSCP_CFG_DSCP_REWR_CFG(dscp_cfg));
    }
    pr("\n");

    VTSS_RC(srvl_debug_range_checkers(pr, info));
    VTSS_RC(srvl_debug_is1_all(pr, info));

    srvl_debug_print_header(pr, "QoS Port and Queue Policer");

    srvl_debug_reg_header(pr, "Policer Config (chip ports)");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no]) {
            continue;
        }
        port = VTSS_CHIP_PORT(port_no);
        srvl_debug_reg_inst(pr, VTSS_ANA_PORT_POL_CFG(port), port, "POL_CFG");
        srvl_debug_reg_inst(pr, VTSS_ANA_POL_MISC_POL_FLOWC(port), port, "POL_FLOWC");
    }
    pr("\n");

    srvl_debug_print_header(pr, "Policers");

    for (i = 0; i < SRVL_POLICER_CNT; i++) {
        SRVL_RD(VTSS_ANA_POL_POL_PIR_CFG(i), &pir);
        if (pir == 0)
            continue;

        if (header)
            pr("Index  Mode  Dual  IFG  CF  OS  PIR    PBS  CIR    CBS\n");
        header = 0;

        SRVL_RD(VTSS_ANA_POL_POL_MODE_CFG(i), &value);
        SRVL_RD(VTSS_ANA_POL_POL_CIR_CFG(i), &cir);
        mode = VTSS_X_ANA_POL_POL_MODE_CFG_FRM_MODE(value);
        pr("%-7u%-6s%-6u%-5u%-4u%-4u%-7u%-5u%-7u%-5u\n",
           i,
           mode == POL_MODE_LINERATE ? "Line" : mode == POL_MODE_DATARATE ? "Data" :
           mode == POL_MODE_FRMRATE_100FPS ? "F100" : "F1",
           SRVL_BF(ANA_POL_POL_MODE_CFG_CIR_ENA, value),
           VTSS_X_ANA_POL_POL_MODE_CFG_IPG_SIZE(value),
           SRVL_BF(ANA_POL_POL_MODE_CFG_DLB_COUPLED, value),
           SRVL_BF(ANA_POL_POL_MODE_CFG_OVERSHOOT_ENA, value),
           VTSS_X_ANA_POL_POL_PIR_CFG_PIR_RATE(pir),
           VTSS_X_ANA_POL_POL_PIR_CFG_PIR_BURST(pir),
           VTSS_X_ANA_POL_POL_CIR_CFG_CIR_RATE(cir),
           VTSS_X_ANA_POL_POL_CIR_CFG_CIR_BURST(cir));
    }
    if (!header)
        pr("\n");

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Layer 2
 * ================================================================= */

static vtss_rc srvl_pgid_mask_write(u32 pgid, BOOL member[VTSS_PORT_ARRAY_SIZE],
                                   BOOL cpu_copy, vtss_packet_rx_queue_t cpu_queue)
{
    u32 mask = srvl_port_mask(member);
    u32 queue = 0;
    
    if (cpu_copy) {
        mask |= VTSS_BIT(VTSS_CHIP_PORT_CPU);
        queue = cpu_queue;
    }
    SRVL_WR(VTSS_ANA_PGID_PGID(pgid),
            VTSS_F_ANA_PGID_PGID_PGID(mask) |
            VTSS_F_ANA_PGID_PGID_CPUQ_DST_PGID(queue));

    return VTSS_RC_OK;
}

static vtss_rc srvl_pgid_table_write(u32 pgid, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_pgid_entry_t *pgid_entry = &vtss_state->pgid_table[pgid];

    return srvl_pgid_mask_write(srvl_chip_pgid(pgid), member, 
                                pgid_entry->cpu_copy, pgid_entry->cpu_queue);
}

static vtss_rc srvl_pgid_update(u32 pgid, BOOL member[])
{
    vtss_port_no_t    port_no;
    vtss_pgid_entry_t *pgid_entry;
    
    pgid = srvl_vtss_pgid(pgid);
    pgid_entry = &vtss_state->pgid_table[pgid];
    pgid_entry->resv = 1;
    pgid_entry->references = 1;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        pgid_entry->member[port_no] = member[port_no];
    
    return srvl_pgid_table_write(pgid, member);
}

static vtss_rc srvl_aggr_table_write(u32 ac, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    return srvl_pgid_mask_write(PGID_AGGR + ac, member, 0, 0);
}

static vtss_rc srvl_src_table_write(vtss_port_no_t port_no, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    return srvl_pgid_mask_write(PGID_SRC + VTSS_CHIP_PORT(port_no), member, 0, 0);
}

static vtss_rc srvl_aggr_mode_set(void)
{
    vtss_aggr_mode_t *mode = &vtss_state->aggr_mode;

    SRVL_WR(VTSS_ANA_COMMON_AGGR_CFG,
            (mode->sip_dip_enable     ? VTSS_F_ANA_COMMON_AGGR_CFG_AC_IP4_SIPDIP_ENA : 0) |
            (mode->sport_dport_enable ? VTSS_F_ANA_COMMON_AGGR_CFG_AC_IP4_TCPUDP_ENA : 0) |
            (mode->dmac_enable        ? VTSS_F_ANA_COMMON_AGGR_CFG_AC_DMAC_ENA       : 0) |
            (mode->smac_enable        ? VTSS_F_ANA_COMMON_AGGR_CFG_AC_SMAC_ENA       : 0));
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_pmap_table_write(vtss_port_no_t port_no, vtss_port_no_t l_port_no)
{
    SRVL_WRM(VTSS_ANA_PORT_PORT_CFG(VTSS_CHIP_PORT(port_no)), 
             VTSS_F_ANA_PORT_PORT_CFG_PORTID_VAL(VTSS_CHIP_PORT(l_port_no)), 
             VTSS_M_ANA_PORT_PORT_CFG_PORTID_VAL);

    return VTSS_RC_OK;
}

static vtss_rc srvl_learn_state_set(const BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_port_no_t port_no;
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        SRVL_WRM_CTL(VTSS_ANA_PORT_PORT_CFG(VTSS_CHIP_PORT(port_no)), 
                     member[port_no], VTSS_F_ANA_PORT_PORT_CFG_LEARN_ENA);
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_port_forward_set(const vtss_port_no_t port_no)
{
    return VTSS_RC_OK;
}

static vtss_rc srvl_mac_table_idle(void)
{
    u32 cmd;    

    do {
        SRVL_RD(VTSS_ANA_ANA_TABLES_MACACCESS, &cmd);        
    } while (VTSS_X_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(cmd) != MAC_CMD_IDLE);

    return VTSS_RC_OK;
}

static vtss_rc srvl_mac_table_add(const vtss_mac_table_entry_t *const entry, u32 pgid)
{
    u32  mach, macl, mask, idx = 0, aged = 0, fwd_kill = 0, type;
    BOOL copy_to_cpu = 0;
    
    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    
    if (pgid == VTSS_PGID_NONE) {
        /* IPv4/IPv6 MC entry, encode port mask directly */
        copy_to_cpu = entry->copy_to_cpu;
        mask = srvl_port_mask(vtss_state->pgid_table[pgid].member); 
        if (entry->vid_mac.mac.addr[0] == 0x01) {
            type = MAC_TYPE_IPV4_MC;
            macl = ((macl & 0x00ffffff) | ((mask << 24) & 0xff000000));
            mach = ((mach & 0xffff0000) | ((mask >> 8) & 0x0000ffff));
        } else {
            type = MAC_TYPE_IPV6_MC;
            mach = ((mach & 0xffff0000) | (mask & 0x0000ffff));
        }
    } else {
        /* Not IP MC entry */
        /* Set FWD_KILL to make the switch discard frames in SMAC lookup */
        fwd_kill = (entry->copy_to_cpu || pgid != vtss_state->pgid_drop ? 0 : 1);
        idx = srvl_chip_pgid(pgid);
        type = (entry->locked ? MAC_TYPE_LOCKED : MAC_TYPE_NORMAL);
    }

    /* Insert/learn new entry into the MAC table  */
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACLDATA, macl);    

    SRVL_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
            (copy_to_cpu        ? VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_CPU_COPY : 0) |
            (fwd_kill           ? VTSS_F_ANA_ANA_TABLES_MACACCESS_SRC_KILL     : 0) |
            (aged               ? VTSS_F_ANA_ANA_TABLES_MACACCESS_AGED_FLAG    : 0) |
            VTSS_F_ANA_ANA_TABLES_MACACCESS_VALID                                   |
            VTSS_F_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(type)                        |
            VTSS_F_ANA_ANA_TABLES_MACACCESS_DEST_IDX(idx)                           |
            VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_LEARN));
    
    /* Wait until MAC operation is finished */
    return srvl_mac_table_idle();
}

static u32 srvl_mac_type(const vtss_vid_mac_t *vid_mac)
{
    return (VTSS_MAC_IPV4_MC(vid_mac->mac.addr) ? MAC_TYPE_IPV4_MC :
            VTSS_MAC_IPV6_MC(vid_mac->mac.addr) ? MAC_TYPE_IPV6_MC : MAC_TYPE_NORMAL);
}

static vtss_rc srvl_mac_table_del(const vtss_vid_mac_t *const vid_mac)
{
    u32 mach, macl;
    
    vtss_mach_macl_get(vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    
    /* Delete/unlearn the given MAC entry */
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACLDATA, macl);    
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
            VTSS_F_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(srvl_mac_type(vid_mac)) |
            VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_FORGET));
    
    /* Wait until MAC operation is finished */
    return srvl_mac_table_idle();
}

static vtss_rc srvl_mac_table_result(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32               value, mask, mach, macl, idx, type;
    vtss_port_no_t    port_no;
    vtss_pgid_entry_t *pgid_entry;
    u8                *mac = &entry->vid_mac.mac.addr[0];
    
    SRVL_RD(VTSS_ANA_ANA_TABLES_MACACCESS, &value);        

    /* Check if entry is valid */
    if (!(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_VALID)) {
        VTSS_D("not valid");
        return VTSS_RC_ERROR;
    }

    type               = VTSS_X_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(value);
    idx                = VTSS_X_ANA_ANA_TABLES_MACACCESS_DEST_IDX(value); 
    entry->aged        = VTSS_BOOL(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_AGED_FLAG);
    entry->copy_to_cpu = VTSS_BOOL(value & VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_CPU_COPY);
    entry->locked      = (type == MAC_TYPE_NORMAL ? 0 : 1);

    SRVL_RD(VTSS_ANA_ANA_TABLES_MACHDATA, &mach);
    SRVL_RD(VTSS_ANA_ANA_TABLES_MACLDATA, &macl);

    if (type == MAC_TYPE_IPV4_MC || type == MAC_TYPE_IPV6_MC) {
        /* IPv4/IPv6 MC entry, decode port mask from entry */
        *pgid = VTSS_PGID_NONE;
        if (type == MAC_TYPE_IPV6_MC) {
            mask = (mach & 0xffff);
            mach = ((mach & 0xffff0000) | 0x00003333);
        } else {
            mask = (((mach << 8) & 0xffff00) | ((macl >> 24) & 0xff));
            mach = ((mach & 0xffff0000) | 0x00000100);
            macl = ((macl & 0x00ffffff) | 0x5e000000);
        }    

        /* Convert port mask */
        pgid_entry = &vtss_state->pgid_table[*pgid];
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            pgid_entry->member[port_no] = VTSS_BOOL(mask & (1 << VTSS_CHIP_PORT(port_no)));
    } else {
        *pgid = srvl_vtss_pgid(idx);
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

static vtss_rc srvl_mac_table_get(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32 mach, macl;

    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACLDATA, macl);    

    /* Do a lookup */
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
            VTSS_F_ANA_ANA_TABLES_MACACCESS_VALID |
            VTSS_F_ANA_ANA_TABLES_MACACCESS_ENTRY_TYPE(srvl_mac_type(&entry->vid_mac)) |
            VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_READ));

    VTSS_RC(srvl_mac_table_idle());
    return srvl_mac_table_result(entry, pgid);
}

static vtss_rc srvl_mac_table_get_next(vtss_mac_table_entry_t *const entry, u32 *pgid)
{
    u32 mach, macl;

    vtss_mach_macl_get(&entry->vid_mac, &mach, &macl);
    VTSS_D("address 0x%08x%08x", mach, macl);

    SRVL_WR(VTSS_ANA_ANA_TABLES_MACHDATA, mach);
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACLDATA, macl);    

    /* Do a get next lookup */
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
            VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_GET_NEXT));

    VTSS_RC(srvl_mac_table_idle());
    return srvl_mac_table_result(entry, pgid);
}

static vtss_rc srvl_mac_table_age_time_set(void)
{
    u32 time;
    
    /* Scan two times per age time */
    time = (vtss_state->mac_age_time/2);
    if (time > 0xfffff)
        time = 0xfffff;
    SRVL_WR(VTSS_ANA_ANA_AUTOAGE, VTSS_F_ANA_ANA_AUTOAGE_AGE_PERIOD(time));
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_mac_table_age(BOOL             pgid_age, 
                                  u32              pgid,
                                  BOOL             vid_age,
                                  const vtss_vid_t vid)
{
    /* Selective aging */
    SRVL_WR(VTSS_ANA_ANA_ANAGEFIL, 
            (pgid_age ? VTSS_F_ANA_ANA_ANAGEFIL_PID_EN : 0) |
            (vid_age ? VTSS_F_ANA_ANA_ANAGEFIL_VID_EN  : 0) |
            VTSS_F_ANA_ANA_ANAGEFIL_PID_VAL(srvl_chip_pgid(pgid)) |
            VTSS_F_ANA_ANA_ANAGEFIL_VID_VAL(vid));
    
    /* Do the aging */
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACACCESS, 
            VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_TABLE_AGE));
    
    VTSS_RC(srvl_mac_table_idle());

    /* Clear age filter again to avoid affecting automatic ageing */
    SRVL_WR(VTSS_ANA_ANA_ANAGEFIL, 0);

    return VTSS_RC_OK;
}

static vtss_rc srvl_mac_table_status_get(vtss_mac_table_status_t *status) 
{
    u32 value;

    /* Read and clear sticky register */
    SRVL_RD(VTSS_ANA_ANA_ANEVENTS, &value);
    SRVL_WR(VTSS_ANA_ANA_ANEVENTS, value & 
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

static vtss_rc srvl_learn_port_mode_set(const vtss_port_no_t port_no)
{
    vtss_learn_mode_t *mode = &vtss_state->learn_mode[port_no];
    u32               value, port = VTSS_CHIP_PORT(port_no);

    SRVL_WRM(VTSS_ANA_PORT_PORT_CFG(port), 
             (mode->discard   ? VTSS_F_ANA_PORT_PORT_CFG_LEARNDROP : 0) |
             (mode->automatic ? VTSS_F_ANA_PORT_PORT_CFG_LEARNAUTO : 0) |
             (mode->cpu       ? VTSS_F_ANA_PORT_PORT_CFG_LEARNCPU  : 0),
             (VTSS_F_ANA_PORT_PORT_CFG_LEARNDROP |
              VTSS_F_ANA_PORT_PORT_CFG_LEARNAUTO |
              VTSS_F_ANA_PORT_PORT_CFG_LEARNCPU));    
    
    if (!mode->automatic) {
        /* Flush entries previously learned on port to avoid continuous refreshing */
        SRVL_RD(VTSS_ANA_PORT_PORT_CFG(port), &value);
        SRVL_WRM_CLR(VTSS_ANA_PORT_PORT_CFG(port), VTSS_F_ANA_PORT_PORT_CFG_LEARN_ENA);
        VTSS_RC(srvl_mac_table_age(1, port_no, 0, 0));
        VTSS_RC(srvl_mac_table_age(1, port_no, 0, 0));
        SRVL_WR(VTSS_ANA_PORT_PORT_CFG(port), value);
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_mac_table(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    u32 value;
    
    /* Read and clear analyzer sticky bits */
    SRVL_RD(VTSS_ANA_ANA_ANEVENTS, &value);
    SRVL_WR(VTSS_ANA_ANA_ANEVENTS, value);
    
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

static vtss_rc srvl_debug_src_table(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    u32 port, mask;

    srvl_debug_print_header(pr, "Source Masks");
    srvl_debug_print_port_header(pr, "Port  ");
    for (port = 0; port <= VTSS_CHIP_PORTS;  port++) {
        SRVL_RD(VTSS_ANA_PGID_PGID(PGID_SRC + port), &mask);
        mask = VTSS_X_ANA_PGID_PGID_PGID(mask);
        pr("%-4u  ", port);
        srvl_debug_print_mask(pr, mask);
    }
    pr("\n");

    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_aggr(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    u32 ac, pgid, mask, value;
    
    VTSS_RC(srvl_debug_src_table(pr, info));
    
    srvl_debug_print_header(pr, "Aggregation Masks");
    srvl_debug_print_port_header(pr, "AC    ");
    for (ac = 0; ac < SRVL_ACS; ac++) {
        SRVL_RD(VTSS_ANA_PGID_PGID(PGID_AGGR + ac), &mask);
        mask = VTSS_X_ANA_PGID_PGID_PGID(mask);
        pr("%-4u  ", ac);
        srvl_debug_print_mask(pr, mask);
    }
    pr("\n");
    
    srvl_debug_print_header(pr, "Destination Masks");
    srvl_debug_print_port_header(pr, "PGID  CPU  Queue  ");
    for (pgid = 0; pgid < VTSS_PGID_LUTON26; pgid++) {
        SRVL_RD(VTSS_ANA_PGID_PGID(pgid), &value);
        mask = VTSS_X_ANA_PGID_PGID_PGID(value);
        pr("%-4u  %-3u  %-5u  ", pgid, mask & VTSS_BIT(VTSS_CHIP_PORT_CPU) ? 1 : 0,
           VTSS_X_ANA_PGID_PGID_CPUQ_DST_PGID(value));
        srvl_debug_print_mask(pr, mask);
    }
    pr("\n");
    
    srvl_debug_print_header(pr, "Flooding PGIDs");
    SRVL_RD(VTSS_ANA_ANA_FLOODING, &value);
    pr("UNICAST  : %u\n", VTSS_X_ANA_ANA_FLOODING_FLD_UNICAST(value));
    pr("MULTICAST: %u\n", VTSS_X_ANA_ANA_FLOODING_FLD_MULTICAST(value));
    SRVL_RD(VTSS_ANA_ANA_FLOODING_IPMC, &value);
    pr("MC4_CTRL : %u\n", VTSS_X_ANA_ANA_FLOODING_IPMC_FLD_MC4_CTRL(value));
    pr("MC4_DATA : %u\n", VTSS_X_ANA_ANA_FLOODING_IPMC_FLD_MC4_DATA(value));
    pr("MC6_CTRL : %u\n", VTSS_X_ANA_ANA_FLOODING_IPMC_FLD_MC6_CTRL(value));
    pr("MC6_DATA : %u\n", VTSS_X_ANA_ANA_FLOODING_IPMC_FLD_MC6_DATA(value));
    pr("\n");

    srvl_debug_reg_header(pr, "Aggr. Mode");
    srvl_debug_reg(pr, VTSS_ANA_COMMON_AGGR_CFG, "AGGR_CFG");
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_stp(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    u32  value, port;
    BOOL header = 1;

    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        if (vtss_cmn_port2port_no(info, port) == VTSS_PORT_NO_NONE)
            continue;
        if (header)
            pr("Port  ID  Learn  L_Auto  L_CPU  L_DROP  Mirror\n");
        header = 0;
        
        SRVL_RD(VTSS_ANA_PORT_PORT_CFG(port), &value);
        pr("%-4u  %-2u  %-5u  %-6u  %-5u  %-6u  %u\n",
           port, 
           VTSS_X_ANA_PORT_PORT_CFG_PORTID_VAL(value),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_LEARN_ENA),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_LEARNAUTO),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_LEARNCPU),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_LEARNDROP),
           VTSS_BOOL(value & VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA));
    }
    if (!header)
        pr("\n");

    header = 1;
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        if (vtss_cmn_port2port_no(info, port) == VTSS_PORT_NO_NONE)
            continue;
        if (header)
            srvl_debug_reg_header(pr, "ANA");
        header = 0;
        srvl_debug_reg_inst(pr, VTSS_ANA_PORT_PORT_CFG(port), port, "PORT_CFG");
    }
    if (!header)
        pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_vxlat(const vtss_debug_printf_t pr,
                                const vtss_debug_info_t   *const info)
{
    VTSS_RC(srvl_debug_is1_all(pr, info));
    VTSS_RC(srvl_debug_es0_all(pr, info));

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Layer 2 - VLAN 
 * ================================================================= */

static vtss_rc srvl_vlan_conf_apply(BOOL ports)
{
    u32            etype = vtss_state->vlan_conf.s_etype;
    vtss_port_no_t port_no;

    /* BZ 4513: Type 0x8100 can not be used so we use 0x88a8 */
    if (etype == VTSS_ETYPE_TAG_C)
        etype = VTSS_ETYPE_TAG_S;

    SRVL_WR(VTSS_SYS_SYSTEM_VLAN_ETYPE_CFG, etype);

    /* Update ports */
    for (port_no = VTSS_PORT_NO_START; ports && port_no < vtss_state->port_count; port_no++) {
        VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_vlan_conf_set(void)
{
    return srvl_vlan_conf_apply(1);
}

static vtss_rc srvl_vlan_port_conf_apply(u32                   port, 
                                         vtss_vlan_port_conf_t *conf)
{
    u32                   tpid = TAG_TPID_CFG_0x8100;
    u32                   value, etype = vtss_state->vlan_conf.s_etype;
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
    SRVL_WRM(VTSS_ANA_PORT_VLAN_CFG(port), value,
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
    SRVL_WR(VTSS_ANA_PORT_DROP_CFG(port), value);
    
    /* Ingress filtering */
    SRVL_WRM(VTSS_ANA_ANA_VLANMASK, (conf->ingress_filter ? 1 : 0) << port, 1 << port);

    /* Rewriter VLAN tag configuration */
    value = (VTSS_F_REW_PORT_TAG_CFG_TAG_TPID_CFG(tpid) |
             VTSS_F_REW_PORT_TAG_CFG_TAG_CFG(
                 conf->untagged_vid == VTSS_VID_ALL ? TAG_CFG_DISABLE :
                 conf->untagged_vid == VTSS_VID_NULL ? TAG_CFG_ALL : TAG_CFG_ALL_NPV_NNUL));
    SRVL_WRM(VTSS_REW_PORT_TAG_CFG(port), value, 
             VTSS_M_REW_PORT_TAG_CFG_TAG_TPID_CFG | 
             VTSS_M_REW_PORT_TAG_CFG_TAG_CFG);
    SRVL_WRM(VTSS_REW_PORT_PORT_VLAN_CFG(port), 
             VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_TPID(etype) |
             VTSS_F_REW_PORT_PORT_VLAN_CFG_PORT_VID(conf->untagged_vid),
             VTSS_M_REW_PORT_PORT_VLAN_CFG_PORT_TPID |
             VTSS_M_REW_PORT_PORT_VLAN_CFG_PORT_VID);

    return VTSS_RC_OK;
}

static vtss_rc srvl_vlan_port_conf_update(vtss_port_no_t port_no, vtss_vlan_port_conf_t *conf)
{
    /* Update maximum tags allowed */
    VTSS_RC(srvl_port_max_tags_set(port_no));

    return srvl_vlan_port_conf_apply(VTSS_CHIP_PORT(port_no), conf);
}

static vtss_rc srvl_vlan_table_idle(void)
{
    u32 value;
    
    do {
        SRVL_RD(VTSS_ANA_ANA_TABLES_VLANACCESS, &value);
    } while (VTSS_X_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(value) != VLAN_CMD_IDLE);

    return VTSS_RC_OK;
}

static vtss_rc srvl_vlan_mask_update(vtss_vid_t vid, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_vlan_entry_t *vlan_entry = &vtss_state->vlan_table[vid];

    /* Index and properties */
    SRVL_WR(VTSS_ANA_ANA_TABLES_VLANTIDX, 
            VTSS_F_ANA_ANA_TABLES_VLANTIDX_V_INDEX(vid) |
            (vlan_entry->isolated ? VTSS_F_ANA_ANA_TABLES_VLANTIDX_VLAN_PRIV_VLAN : 0) |
            (vlan_entry->learn_disable ? VTSS_F_ANA_ANA_TABLES_VLANTIDX_VLAN_LEARN_DISABLED : 0));

    /* VLAN mask */
    SRVL_WR(VTSS_ANA_ANA_TABLES_VLANACCESS,
            VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_PORT_MASK(srvl_port_mask(member)) |
            VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(VLAN_CMD_WRITE));

    return srvl_vlan_table_idle();
}

static vtss_rc srvl_debug_vlan(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */ // Due to VTSS_EXIT_ENTER
    vtss_vid_t        vid;
    vtss_vlan_entry_t *vlan_entry;
    BOOL              header = 1;
    vtss_port_no_t    port_no;
    u32               port, mask = 0;
    char              buf[32];

    for (port = 0; port < (VTSS_CHIP_PORTS + 2); port++) {
        if (port < VTSS_CHIP_PORTS) {
            /* Normal ports */
            if ((port_no = vtss_cmn_port2port_no(info, port)) == VTSS_PORT_NO_NONE)
                continue;
            sprintf(buf, "Port %u (%u)", port, port_no);
        } else {
            /* CPU ports */
            if (!info->full)
                continue;
            sprintf(buf, "Port %u (CPU)", port);
        }

        srvl_debug_reg_header(pr, buf);
        if (port != VTSS_CHIP_PORT_CPU_1) {
            srvl_debug_reg_inst(pr, VTSS_ANA_PORT_VLAN_CFG(port), port, "ANA:VLAN_CFG");
            srvl_debug_reg_inst(pr, VTSS_ANA_PORT_DROP_CFG(port), port, "ANA:DROP_CFG");
        }
        srvl_debug_reg_inst(pr, VTSS_REW_PORT_PORT_VLAN_CFG(port), port, "REW:VLAN_CFG");
        srvl_debug_reg_inst(pr, VTSS_REW_PORT_TAG_CFG(port), port, "REW:TAG_CFG");
        pr("\n");
    }
    srvl_debug_reg(pr, VTSS_SYS_SYSTEM_VLAN_ETYPE_CFG, "ETYPE_CFG");
    srvl_debug_reg(pr, VTSS_ANA_ANA_ADVLEARN, "ADVLEARN");
    srvl_debug_reg(pr, VTSS_ANA_ANA_VLANMASK, "VLANMASK");
    pr("\n");
    
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        vlan_entry = &vtss_state->vlan_table[vid];
        if (!vlan_entry->enabled && !info->full)
            continue;

        SRVL_WR(VTSS_ANA_ANA_TABLES_VLANTIDX, VTSS_F_ANA_ANA_TABLES_VLANTIDX_V_INDEX(vid));
        SRVL_WR(VTSS_ANA_ANA_TABLES_VLANACCESS, 
                VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(VLAN_CMD_READ));
        if (srvl_vlan_table_idle() != VTSS_RC_OK)
            continue;
        SRVL_RD(VTSS_ANA_ANA_TABLES_VLANACCESS, &mask);
        mask = VTSS_X_ANA_ANA_TABLES_VLANACCESS_VLAN_PORT_MASK(mask);

        if (header)
            srvl_debug_print_port_header(pr, "VID   ");
        header = 0;

        pr("%-4u  ", vid);
        srvl_debug_print_mask(pr, mask);

        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  VCL
 * ================================================================= */

static vtss_rc srvl_vcl_port_conf_set(const vtss_port_no_t port_no)
{
    BOOL dmac_dip = vtss_state->vcl_port_conf[port_no].dmac_dip;
    u32  mask = VTSS_F_ANA_PORT_VCAP_CFG_S1_DMAC_DIP_ENA(2);
    u32  port = VTSS_CHIP_PORT(port_no);
    
    /* Enable/disable DMAC/DIP match in second IS1 lookup */
    SRVL_WRM(VTSS_ANA_PORT_VCAP_CFG(port), dmac_dip ? mask : 0, mask);

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Layer 2 - PVLAN / Isolated ports
 * ================================================================= */

static vtss_rc srvl_isolated_port_members_set(void)
{
    u32 mask = srvl_port_mask(vtss_state->isolated_port);

    /* Isolated ports: cleared (add CPU as not isolated) */
    mask = ((~mask) & VTSS_CHIP_PORT_MASK) | VTSS_BIT(VTSS_CHIP_PORT_CPU);
    SRVL_WR(VTSS_ANA_ANA_ISOLATED_PORTS, mask);
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_pvlan(const vtss_debug_printf_t pr,
                                const vtss_debug_info_t   *const info)
{
    srvl_debug_reg_header(pr, "ANA");
    srvl_debug_reg(pr, VTSS_ANA_ANA_ISOLATED_PORTS, "ISOLATED_PORTS");
    pr("\n");
    
    return srvl_debug_src_table(pr, info);
}

/* ================================================================= *
 *  Layer 2 - IP Multicast
 * ================================================================= */

static vtss_rc srvl_flood_conf_set(void)
{
    u32 pgid = (vtss_state->ipv6_mc_scope ? PGID_MCIPV6 : PGID_MC);

    /* Unicast flood mask */
    VTSS_RC(srvl_pgid_update(PGID_UC, vtss_state->uc_flood));
    
    /* Multicast flood mask */
    VTSS_RC(srvl_pgid_update(PGID_MC, vtss_state->mc_flood));
    
    /* IPv4 flood mask */
    VTSS_RC(srvl_pgid_update(PGID_MCIPV4, vtss_state->ipv4_mc_flood));

    /* IPv6 flood mask */
    VTSS_RC(srvl_pgid_update(PGID_MCIPV6, vtss_state->ipv6_mc_flood));

    /* IPv6 flood scope */
    SRVL_WRM(VTSS_ANA_ANA_FLOODING_IPMC, 
             VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC6_CTRL(pgid),
             VTSS_M_ANA_ANA_FLOODING_IPMC_FLD_MC6_CTRL);
    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
static vtss_rc srvl_ip_mc_update(vtss_ipmc_data_t *ipmc, vtss_ipmc_cmd_t cmd)
{
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */

static vtss_rc srvl_debug_ipmc(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Layer 2 - Mirror
 * ================================================================= */

#ifdef VTSS_FEATURE_MIRROR_CPU
/* CPU Ingress ports subjects for mirroring */
static vtss_rc srvl_mirror_cpu_ingress_set(void)
{
    BOOL           enabled = vtss_state->mirror_cpu_ingress;

   SRVL_WRM(VTSS_ANA_PORT_PORT_CFG(26), 
            (enabled ? VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA : 0), VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA); // CPU port is port 26, 6See Table 98 in data sheet
    return VTSS_RC_OK;
}

/* CPU Egress ports subjects for mirroring */
static vtss_rc srvl_mirror_cpu_egress_set(void)
{
    BOOL           enabled = vtss_state->mirror_cpu_egress;

    SRVL_WRM(VTSS_ANA_ANA_AGENCTRL, enabled ? VTSS_F_ANA_ANA_AGENCTRL_MIRROR_CPU : 0 , VTSS_F_ANA_ANA_AGENCTRL_MIRROR_CPU);
    return VTSS_RC_OK;
}
#endif // VTSS_FEATURE_MIRROR_CPU

static vtss_rc srvl_mirror_port_set(void)
{
    BOOL           member[VTSS_PORT_ARRAY_SIZE];
    vtss_port_no_t port_no;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        member[port_no] = (port_no == vtss_state->mirror_conf.port_no);
    }
    SRVL_WR(VTSS_ANA_ANA_MIRRORPORTS, srvl_port_mask(member));    

    /* Update all VLANs */
    return vtss_cmn_vlan_update_all();
}

static vtss_rc srvl_mirror_ingress_set(void)
{
#if defined (VTSS_FEATURE_MPLS)
    vtss_port_no_t           port_no;
    vtss_mpls_vprofile_idx_t vp_idx;
    BOOL                     set[VTSS_MPLS_VPROFILE_CNT];

    /* The ingress mirror bit is per virtual port profile. This means that if
     * we have MPLS enabled, the ingress mirroring setup must be applied for all
     * VProfiles (except the static and shared entries for OAM and LSR; ingress
     * mirroring won't work for those since they may be used with multiple
     * ports.)
     */

    memset(set, 0, sizeof(set));
    for (vp_idx = VTSS_MPLS_VPROFILE_RESERVED_CNT; vp_idx < VTSS_MPLS_VPROFILE_CNT; vp_idx++) {
        vtss_mpls_vprofile_t *vp = &VP_P(vp_idx);
        set[vp_idx] = (vp->port < vtss_state->port_count)  &&  vtss_state->mirror_ingress[vp->port];
    }

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        set[port_no] = vtss_state->mirror_ingress[port_no];
    }

    for (vp_idx = 0; vp_idx < VTSS_MPLS_VPROFILE_CNT; vp_idx++) {
        SRVL_WRM_CTL(VTSS_ANA_PORT_PORT_CFG(vp_idx), set[vp_idx],
                     VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA);
    }
#else
    vtss_port_no_t port_no;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        SRVL_WRM_CTL(VTSS_ANA_PORT_PORT_CFG(VTSS_CHIP_PORT(port_no)),
                     vtss_state->mirror_ingress[port_no],
                     VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA);
    }
#endif
    return VTSS_RC_OK;
}

static vtss_rc srvl_mirror_egress_set(void)
{
    SRVL_WR(VTSS_ANA_ANA_EMIRRORPORTS, srvl_port_mask(vtss_state->mirror_egress));
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_mirror(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    u32 port, value, mask = 0;
    
    /* Calculate ingress mirror mask */
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        SRVL_RD(VTSS_ANA_PORT_PORT_CFG(port), &value);
        if (value & VTSS_F_ANA_PORT_PORT_CFG_SRC_MIRROR_ENA)
            mask |= VTSS_BIT(port);
    }
    srvl_debug_print_port_header(pr, "Mirror   ");
    pr("Ingress  ");
    srvl_debug_print_mask(pr, mask);
    SRVL_RD(VTSS_ANA_ANA_EMIRRORPORTS, &mask);
    pr("Egress   ");
    srvl_debug_print_mask(pr, mask);
    SRVL_RD(VTSS_ANA_ANA_MIRRORPORTS, &mask);
    pr("Ports    ");
    srvl_debug_print_mask(pr, mask);
    pr("\n");

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  NPI
 * ================================================================= */

static vtss_rc srvl_npi_conf_set(const vtss_npi_conf_t *const conf)
{
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  ACL
 * ================================================================= */

static vtss_rc srvl_acl_policer_set(const vtss_acl_policer_no_t policer_no)
{
    vtss_acl_policer_conf_t *conf = &vtss_state->acl_policer_conf[policer_no];
    srvl_policer_conf_t     pol_conf;
    
    memset(&pol_conf, 0, sizeof(pol_conf));
    if (conf->bit_rate_enable) {
        pol_conf.eir = conf->bit_rate;
        pol_conf.ebs = 1; /* Minimum burst size */
    } else {
        pol_conf.frame_rate = 1;
        pol_conf.eir = conf->rate;
    }

    return srvl_policer_conf_set(policer_no + SRVL_POLICER_ACL, &pol_conf);
}

static vtss_rc srvl_acl_port_conf_set(const vtss_port_no_t port_no)
{
    const tcam_props_t   *tcam = &tcam_info[VTSS_TCAM_IS2];
    srvl_tcam_data_t     data;
    vtss_acl_port_conf_t *conf = &vtss_state->acl_port_conf[port_no];
    u32                  port = VTSS_CHIP_PORT(port_no);
    u32                  enable = (conf->policy_no == VTSS_ACL_POLICY_NO_NONE ? 0 : 1);

    VTSS_I("port_no: %u, port: %u", port_no, port);

    /* Enable/disable S2 and set default PAG */
    SRVL_WRM_CTL(VTSS_ANA_PORT_VCAP_S2_CFG(port), enable, VTSS_F_ANA_PORT_VCAP_S2_CFG_S2_ENA);
    SRVL_WRM(VTSS_ANA_PORT_VCAP_CFG(port), 
             VTSS_F_ANA_PORT_VCAP_CFG_PAG_VAL(enable ? (conf->policy_no & 0x3f) : 0),
             VTSS_M_ANA_PORT_VCAP_CFG_PAG_VAL);
    
    /* Set action */
    memset(&data, 0, sizeof(data));
    data.action_offset = tcam->action_type_width;
    VTSS_RC(srvl_is2_action_set(&data, &conf->action, 0));
    VTSS_RC(srvl_vcap_action2cache(tcam, &data));
    return srvl_vcap_port_cmd(tcam, port, VTSS_TCAM_CMD_WRITE, VTSS_TCAM_SEL_ACTION);
}

static vtss_rc srvl_acl_port_counter_get(const vtss_port_no_t    port_no,
                                         vtss_acl_port_counter_t *const counter)
{
    return srvl_vcap_port_get(VTSS_TCAM_IS2, VTSS_CHIP_PORT(port_no), counter, 0);
}

static vtss_rc srvl_acl_port_counter_clear(const vtss_port_no_t port_no)
{
    u32 counter;
    
    return srvl_vcap_port_get(VTSS_TCAM_IS2, VTSS_CHIP_PORT(port_no), &counter, 1);
}

static vtss_rc srvl_ace_add(const vtss_ace_id_t ace_id, const vtss_ace_t *const ace)
{
    vtss_vcap_obj_t             *obj = &vtss_state->is2.obj;
    vtss_vcap_user_t            user = VTSS_IS2_USER_ACL;
    vtss_vcap_data_t            data;
    vtss_is2_data_t             *is2 = &data.u.is2;
    vtss_is2_entry_t            entry;
    vtss_res_chg_t              chg;
    const vtss_ace_udp_tcp_t    *sport = NULL, *dport = NULL;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range; 
    BOOL                        sip_smac_new = 0, sip_smac_old = 0;

    /* Check the simple things */
    VTSS_RC(vtss_cmn_ace_add(ace_id, ace));
    
    /* Check that half entry can be added */
    memset(&chg, 0, sizeof(chg));
    chg.add_key[VTSS_VCAP_KEY_SIZE_HALF] = 1;
    if (vtss_vcap_lookup(obj, user, ace->id, &data, NULL) == VTSS_RC_OK) {
        chg.del_key[data.key_size] = 1;
        
        /* Free any old range checkers */
        VTSS_RC(vtss_vcap_range_free(&range_new, is2->srange));
        VTSS_RC(vtss_vcap_range_free(&range_new, is2->drange));
        
        /* Lookup SIP/SMAC rule */
        if (vtss_vcap_lookup(obj, VTSS_IS2_USER_ACL_SIP, ace->id, NULL, NULL) == VTSS_RC_OK) {
            sip_smac_old = 1;
            chg.del_key[VTSS_VCAP_KEY_SIZE_QUARTER] = 1;
        }
    }
    
    if (ace->type == VTSS_ACE_TYPE_IPV4) {
        if (ace->frame.ipv4.sip_smac.enable) {
            /* SMAC_SIP4 rule needed */
            sip_smac_new = 1;
            chg.add_key[VTSS_VCAP_KEY_SIZE_QUARTER] = 1;
        }
        if (vtss_vcap_udp_tcp_rule(&ace->frame.ipv4.proto)) {
            sport = &ace->frame.ipv4.sport;
            dport = &ace->frame.ipv4.dport;
        } 
    }
    if (ace->type == VTSS_ACE_TYPE_IPV6 && vtss_vcap_udp_tcp_rule(&ace->frame.ipv6.proto)) {
        sport = &ace->frame.ipv6.sport;
        dport = &ace->frame.ipv6.dport;
    }
    VTSS_RC(vtss_cmn_vcap_res_check(obj, &chg));

    vtss_vcap_is2_init(&data, &entry);
    if (sport && dport) {
        /* Allocate new range checkers */
        VTSS_RC(vtss_vcap_udp_tcp_range_alloc(&range_new, &is2->srange, sport, 1));
        VTSS_RC(vtss_vcap_udp_tcp_range_alloc(&range_new, &is2->drange, dport, 0));
    }

    /* Commit range checkers */
    VTSS_RC(vtss_vcap_range_commit(&range_new));

    /* Add main entry */
    entry.first = 1;
    entry.ace = *ace;
    if (sip_smac_new) {
        entry.host_match = 1;
        entry.ace.frame.ipv4.sip.value = ace->frame.ipv4.sip_smac.sip;
        entry.ace.frame.ipv4.sip.mask = 0xffffffff;
    }
    data.key_size = VTSS_VCAP_KEY_SIZE_HALF;
    VTSS_RC(vtss_vcap_add(obj, user, ace->id, ace_id, &data, 0));

    /* Add/delete SIP/SMAC entry */
    user = VTSS_IS2_USER_ACL_SIP;
    if (sip_smac_new) {
        data.key_size = VTSS_VCAP_KEY_SIZE_QUARTER;
        VTSS_RC(vtss_vcap_add(obj, user, ace->id, VTSS_VCAP_ID_LAST, &data, 0));
    } else if (sip_smac_old) {
        VTSS_RC(vtss_vcap_del(obj, user, ace->id));
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_ace_del(const vtss_ace_id_t ace_id)
{
    /* Delete main entry */
    VTSS_RC(vtss_cmn_ace_del(ace_id));

    /* Delete SIP/SMAC entry */
    VTSS_RC(vtss_vcap_del(&vtss_state->is2.obj, VTSS_IS2_USER_ACL_SIP, ace_id));
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_range_commit(void)
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
        SRVL_WR(VTSS_ANA_COMMON_VCAP_RNG_TYPE_CFG(i),
                VTSS_F_ANA_COMMON_VCAP_RNG_TYPE_CFG_VCAP_RNG_CFG(type));
        SRVL_WR(VTSS_ANA_COMMON_VCAP_RNG_VAL_CFG(i),
                VTSS_F_ANA_COMMON_VCAP_RNG_VAL_CFG_VCAP_RNG_MIN_VAL(entry->min) |
                VTSS_F_ANA_COMMON_VCAP_RNG_VAL_CFG_VCAP_RNG_MAX_VAL(entry->max))
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_range_checkers(const vtss_debug_printf_t pr,
                                         const vtss_debug_info_t   *const info)
{
    u32 i;
    
    srvl_debug_reg_header(pr, "Range Checkers");
    for (i = 0; i < VTSS_VCAP_RANGE_CHK_CNT; i++) {
        srvl_debug_reg_inst(pr, VTSS_ANA_COMMON_VCAP_RNG_TYPE_CFG(i), i, "RNG_TYPE_CFG");
        srvl_debug_reg_inst(pr, VTSS_ANA_COMMON_VCAP_RNG_VAL_CFG(i), i, "RNG_VAL_CFG");
    }
    pr("\n");
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_acl(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    VTSS_RC(srvl_debug_range_checkers(pr, info));
    return srvl_debug_is2_all(pr, info);
}

#ifdef VTSS_FEATURE_SFLOW
/* ================================================================= *
 *  SFLOW
 * ================================================================= */

/* sFlow H/W-related min/max */
#define SRVL_SFLOW_MIN_SAMPLE_RATE    0 /**< Minimum allowable sampling rate for sFlow */
#define SRVL_SFLOW_MAX_SAMPLE_RATE 4095 /**< Maximum allowable sampling rate for sFlow */

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
 * srvl_sflow_hw_rate()
 */
static u32 srvl_sflow_hw_rate(const u32 desired_sw_rate, u32 *const realizable_sw_rate)
{
    u32 hw_rate         = desired_sw_rate ? VTSS_ROUNDING_DIVISION(SRVL_SFLOW_MAX_SAMPLE_RATE + 1, desired_sw_rate) : 0;
    hw_rate             = hw_rate > SRVL_SFLOW_MIN_SAMPLE_RATE ? hw_rate - 1 : hw_rate;
    *realizable_sw_rate = VTSS_ROUNDING_DIVISION(SRVL_SFLOW_MAX_SAMPLE_RATE + 1, hw_rate + 1);
    return hw_rate;
}

/**
 * srvl_sflow_sampling_rate_convert()
 */
static vtss_rc srvl_sflow_sampling_rate_convert(const struct vtss_state_s *const state, const BOOL power2, const u32 rate_in, u32 *const rate_out)
{
    u32 modified_rate_in;
    // Could happen that two threads call this function simultaneously at boot, but we take the risk.
    // Once sflow_max_power_of_two has been computed, it's no longer a problem with simultaneous access.
    /*lint -esym(459, sflow_max_power_of_two) */
    static u32 sflow_max_power_of_two;

    if (sflow_max_power_of_two == 0) {
        sflow_max_power_of_two = next_power_of_two(SRVL_SFLOW_MAX_SAMPLE_RATE);
        if ((SRVL_SFLOW_MAX_SAMPLE_RATE & sflow_max_power_of_two) == 0) {
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

    (void)srvl_sflow_hw_rate(modified_rate_in, rate_out);
    return VTSS_RC_OK;
}

/**
 * srvl_sflow_port_conf_set()
 */
static vtss_rc srvl_sflow_port_conf_set(const vtss_port_no_t port_no, const vtss_sflow_port_conf_t *const new_conf)
{
    u32 hw_rate, value;
    vtss_sflow_port_conf_t *cur_conf = &vtss_state->sflow_conf[port_no];

    *cur_conf = *new_conf;
    hw_rate = srvl_sflow_hw_rate(new_conf->sampling_rate, &cur_conf->sampling_rate);

    value  = VTSS_F_ANA_ANA_SFLOW_CFG_SF_RATE(hw_rate);
    value |= new_conf->sampling_rate != 0 && (new_conf->type == VTSS_SFLOW_TYPE_ALL || new_conf->type == VTSS_SFLOW_TYPE_RX) ? VTSS_F_ANA_ANA_SFLOW_CFG_SF_SAMPLE_RX : 0;
    value |= new_conf->sampling_rate != 0 && (new_conf->type == VTSS_SFLOW_TYPE_ALL || new_conf->type == VTSS_SFLOW_TYPE_TX) ? VTSS_F_ANA_ANA_SFLOW_CFG_SF_SAMPLE_TX : 0;
    SRVL_WR(VTSS_ANA_ANA_SFLOW_CFG(VTSS_CHIP_PORT(port_no)), value);

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_SFLOW */

#if defined(VTSS_FEATURE_EVC)

/* ================================================================= *
 *  MCE
 * ================================================================= */

static vtss_vcap_key_size_t srvl_key_type_to_size(vtss_vcap_key_type_t key_type)
{
    return (key_type == VTSS_VCAP_KEY_TYPE_MAC_IP_ADDR ? VTSS_VCAP_KEY_SIZE_FULL :
            key_type == VTSS_VCAP_KEY_TYPE_DOUBLE_TAG ? VTSS_VCAP_KEY_SIZE_QUARTER :
            VTSS_VCAP_KEY_SIZE_HALF);
}

static vtss_vcap_key_size_t srvl_evc_port_key_get(vtss_port_no_t       port_no,
                                                  vtss_vcap_key_type_t *type)
{
    vtss_vcap_key_type_t key_type = vtss_state->evc_port_conf[port_no].key_type;
    
    if (type != NULL)
        *type = key_type;

    return srvl_key_type_to_size(key_type);
}

static vtss_rc srvl_isdx_table_idle(void)
{
    u32 value;
    
    do {
        SRVL_RD(VTSS_ANA_ANA_TABLES_ISDXACCESS, &value);
    } while (VTSS_X_ANA_ANA_TABLES_ISDXACCESS_ISDX_TBL_CMD(value) != ISDX_CMD_IDLE);

    return VTSS_RC_OK;
}

static vtss_rc srvl_isdx_update(BOOL isdx_ena, u32 isdx, u32 isdx_mask, u32 pol_idx, u32 voe_idx)
{
    /* Write to ISDX table */
    SRVL_WR(VTSS_ANA_ANA_TABLES_ISDXTIDX,
            (isdx_ena ? VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_ES0_KEY_ENA : 0) |
            VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_SDLBI(pol_idx) |
            VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_INDEX(isdx) |
            (isdx_ena ? VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_FORCE_ENA : 0));
    SRVL_WR(VTSS_ANA_ANA_TABLES_ISDXACCESS,
            VTSS_F_ANA_ANA_TABLES_ISDXACCESS_ISDX_PORT_MASK(isdx_mask) |
            VTSS_F_ANA_ANA_TABLES_ISDXACCESS_ISDX_TBL_CMD(ISDX_CMD_WRITE));
    
    /* Write VOE mapping to IPT table */
    SRVL_WR(VTSS_ANA_IPT_OAM_MEP_CFG(isdx),
            VTSS_F_ANA_IPT_OAM_MEP_CFG_MEP_IDX(voe_idx) |
            VTSS_F_ANA_IPT_OAM_MEP_CFG_MEP_IDX_P(voe_idx) |
            (voe_idx == VTSS_EVC_VOE_IDX_NONE ? 0 : VTSS_F_ANA_IPT_OAM_MEP_CFG_MEP_IDX_ENA));
    
    return srvl_isdx_table_idle();
}

static vtss_rc srvl_isdx_update_es0(BOOL isdx_ena, u32 isdx, u32 isdx_mask)
{
    /* Write to ISDX table */
    SRVL_WRM(VTSS_ANA_ANA_TABLES_ISDXTIDX,
             (isdx_ena ? VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_ES0_KEY_ENA : 0) |
             VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_INDEX(isdx)                  |
             (isdx_ena ? VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_FORCE_ENA : 0),
             VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_ES0_KEY_ENA |
             VTSS_M_ANA_ANA_TABLES_ISDXTIDX_ISDX_INDEX       |
             VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_FORCE_ENA);
    SRVL_WR(VTSS_ANA_ANA_TABLES_ISDXACCESS,
            VTSS_F_ANA_ANA_TABLES_ISDXACCESS_ISDX_PORT_MASK(isdx_mask) |
            VTSS_F_ANA_ANA_TABLES_ISDXACCESS_ISDX_TBL_CMD(ISDX_CMD_WRITE));

    return srvl_isdx_table_idle();
}

static void srvl_mce_sdx_realloc(vtss_mce_entry_t *mce, vtss_port_no_t port_no, BOOL isdx)
{
    vtss_sdx_entry_t *sdx, **list = (isdx ? &mce->isdx_list : &mce->esdx_list);

    if ((sdx = *list) != NULL) {
        if (sdx->port_no == port_no) {
            /* Reallocate */
            port_no = VTSS_PORT_NO_NONE;
        } else {
            /* Free */
            vtss_cmn_sdx_free(sdx, isdx);
            *list = NULL;
        }
    }
    if (port_no != VTSS_PORT_NO_NONE) {
        /* Allocate */
        *list = vtss_cmn_sdx_alloc(port_no, isdx);
    }
}

static void srvl_mce2is1_tag(const vtss_mce_tag_t *mce_tag, vtss_is1_tag_t *is1_tag)
{
    is1_tag->tagged = mce_tag->tagged;
    is1_tag->s_tag = mce_tag->s_tagged;
    is1_tag->vid.type = VTSS_VCAP_VR_TYPE_VALUE_MASK;
    is1_tag->vid.vr.v.value = mce_tag->vid.value;
    is1_tag->vid.vr.v.mask = mce_tag->vid.mask;
    is1_tag->pcp = mce_tag->pcp;
    is1_tag->dei = mce_tag->dei;
}

static vtss_vcap_key_size_t srvl_mce_port_key_get(const vtss_mce_t *mce,
                                                  vtss_vcap_key_type_t *type)
{
    vtss_vcap_key_type_t key_type;

    /* First lookup, EVC port key */
    if (mce->key.lookup == 0)
        return srvl_evc_port_key_get(mce->key.port_no, type);

    /* Second lookup, VCAP port key */
    key_type = vtss_state->vcap_port_conf[mce->key.port_no].key_type_is1_1;
    
    if (type != NULL)
        *type = key_type;

    return srvl_key_type_to_size(key_type);
}

static vtss_rc srvl_mce_is1_add(vtss_mce_entry_t *mce)
{
    vtss_vcap_data_t  data;
    vtss_is1_entry_t  is1;
    vtss_is1_key_t    *key = &is1.key;
    vtss_is1_action_t *action = &is1.action;
    vtss_mce_t        *conf = &mce->conf;
    vtss_mce_entry_t  *cur;
    u8                value, mask;
    
    vtss_vcap_is1_init(&data, &is1);
    data.u.is1.lookup = (conf->key.lookup ? 1 : 0); /* First/second lookup */
    data.key_size = srvl_mce_port_key_get(conf, &key->key_type);
    key->type = VTSS_IS1_TYPE_ETYPE;
    key->port_list[conf->key.port_no] = 1;
    srvl_mce2is1_tag(&conf->key.tag, &key->tag);
    srvl_mce2is1_tag(&conf->key.inner_tag, &key->inner_tag);

    /* ETYPE field encoding */
    key->frame.etype.etype.value[0] = 0x89;
    key->frame.etype.etype.mask[0] = 0xff;
    if (vtss_state->chip_id.revision == 0) {
        /* Revision A */
        value = 0x02;
        mask = 0xff;
    } else {
        /* Revision B or later */
        key->isdx.value[0] = 0x02; /* ISDX bit 9 indicates OAM frame */
        key->isdx.mask[0] = 0x02;
        value = (conf->key.mel.value & 0x7f);
        mask = (conf->key.mel.mask & 0x7f);
        if (conf->key.injected != VTSS_VCAP_BIT_ANY) {
            if (conf->key.injected == VTSS_VCAP_BIT_1)
                value |= (1 << 7);
            mask |= (1 << 7);
        }
    }
    key->frame.etype.etype.value[1] = value;
    key->frame.etype.etype.mask[1] = mask;

    action->pop_enable = 1;
    action->pop = conf->action.pop_cnt;
    if (conf->action.vid != VTSS_VID_NULL) {
        action->vid_enable = 1;
        action->vid = conf->action.vid;
    }
    action->isdx_enable = (conf->action.isdx == VTSS_MCE_ISDX_NONE ? 0 : 1);
    action->isdx = (mce->isdx_list == NULL ? 0 : mce->isdx_list->sdx);
    if (conf->action.prio_enable) {
        action->prio_enable = 1;
        action->prio = conf->action.prio;
        action->pcp_dei_enable = 1;
        action->pcp = conf->action.prio;
    }
    if (conf->action.policy_no != VTSS_ACL_POLICY_NO_NONE) {
        action->pag_enable = 1;
        action->pag = conf->action.policy_no;
    }
    action->oam_detect = conf->action.oam_detect;

    /* Find next ID using IS1 rules */
    for (cur = mce->next; cur != NULL; cur = cur->next) {
        if (cur->conf.key.port_no < vtss_state->port_count && cur->isdx_list != NULL) {
            break;
        }
    }
    return vtss_vcap_add(&vtss_state->is1.obj, VTSS_IS1_USER_MEP, mce->conf.id,
                         cur == NULL ? VTSS_VCAP_ID_LAST : cur->conf.id, &data, 0);
}

static BOOL srvl_mce_isdx_reuse(vtss_isdx_t isdx)
{
    return (isdx == 0 || isdx == VTSS_MCE_ISDX_NONE || isdx == VTSS_MCE_ISDX_NEW ? 0 : 1);
}

static vtss_rc srvl_mce_add(const vtss_mce_id_t mce_id, const vtss_mce_t *const mce)
{
    const vtss_mce_key_t    *key = &mce->key;
    const vtss_mce_action_t *action = &mce->action;
    vtss_mce_info_t         *mce_info = &vtss_state->mce_info;
    vtss_mce_entry_t        *cur, *prev = NULL;
    vtss_mce_entry_t        *old = NULL, *old_prev = NULL, *ins = NULL, *ins_prev = NULL;
    vtss_vcap_key_size_t    key_size;
    vtss_res_t              res;
    vtss_vcap_obj_t         *is1_obj = &vtss_state->is1.obj;
    vtss_vcap_obj_t         *es0_obj = &vtss_state->es0.obj;
    vtss_vcap_data_t        data;
    vtss_es0_entry_t        es0_entry;
    vtss_es0_key_t          *es0_key = &es0_entry.key;
    vtss_es0_action_t       *es0_action = &es0_entry.action;
    vtss_es0_tag_conf_t     *ot = &es0_action->outer_tag;
    vtss_sdx_entry_t        *isdx = NULL, *isdx_old = NULL, *esdx;
    u32                     isdx_mask, voe_idx;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "mce_id: %u", mce_id);

    /* Check MCE ID */
    if (mce->id == VTSS_MCE_ID_LAST || mce->id == mce_id) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal mce id: %u", mce->id);
        return VTSS_RC_ERROR;
    }

    /* Search for existing entry and place to add */
    for (cur = mce_info->used; cur != NULL; prev = cur, cur = cur->next) {
        if (cur->conf.id == mce->id) {
            /* Entry already exists */
            old_prev = prev;
            old = cur;
        }
        
        if (cur->conf.id == mce_id) {
            /* Found insertion point */
            ins_prev = prev;
            ins = cur;
        }

        if (cur->isdx_list != NULL && cur->isdx_list->sdx == action->isdx) {
            /* Found ISDX, which can be cloned */
            isdx = cur->isdx_list;
        }
    }
    if (mce_id == VTSS_MCE_ID_LAST)
        ins_prev = prev;

    /* Check if the place to insert was found */
    if (ins == NULL && mce_id != VTSS_MCE_ID_LAST) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "could not find mce ID: %u", mce_id);
        return VTSS_RC_ERROR;
    }
    
    /* Check if cloned ISDX was found */
    if (srvl_mce_isdx_reuse(action->isdx) && isdx == NULL) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "could not reuse ISDX %u", action->isdx);
        return VTSS_RC_ERROR;
    }

    /* Check that resources are available */
    vtss_cmn_res_init(&res);
    if (old != NULL) {
        /* Old resources */
        if (old->isdx_list != NULL && old->conf.action.isdx == 0) {
            res.isdx.del++;
        }
        if (old->esdx_list != NULL) {
            res.esdx.del++;
        }
        if (vtss_vcap_lookup(is1_obj, VTSS_IS1_USER_MEP, mce->id, NULL, NULL) != VTSS_RC_OK &&
            old->conf.key.port_no < vtss_state->port_count) {
            key_size = srvl_mce_port_key_get(&old->conf, NULL);
            res.is1.del_key[key_size]++;
        }
        if (vtss_vcap_lookup(es0_obj, VTSS_ES0_USER_MEP, mce->id, NULL, NULL) != VTSS_RC_OK) {
            res.es0.del++;
        }
    }

    /* New/changed resouces */
    if (key->port_no < vtss_state->port_count) {
        key_size = srvl_mce_port_key_get(mce, NULL);
        res.is1.add_key[key_size]++;
    }
    if (action->isdx == 0 && 
        (key->port_no < vtss_state->port_count || key->port_no == VTSS_PORT_NO_CPU)) {
        res.isdx.add++;
    }
    if (action->port_no != VTSS_PORT_NO_NONE) {
        res.esdx.add++;
        res.es0.add++;
    }

    /* Check resource availability */
    VTSS_RC(vtss_cmn_res_check(&res));

    cur = old;
    if (cur == NULL) {
        /* Take entry from free list */
        if ((cur = mce_info->free) == NULL) {
            VTSS_EG(VTSS_TRACE_GROUP_EVC, "no more MCEs");
            return VTSS_RC_ERROR;
        }
        mce_info->free = cur->next;
        mce_info->count++;
    } else {
        /* Take existing entry out of list */
        if (ins_prev == cur)
            ins_prev = old_prev;
        if (old_prev == NULL)
            mce_info->used = cur->next;
        else
            old_prev->next = cur->next;
        if (srvl_mce_isdx_reuse(cur->conf.action.isdx))
            isdx_old = cur->isdx_list; /* Remember old allocated ISDX */
        else
            cur->isdx_list = NULL;     /* Delete old cloned ISDX */
    }

    /* Insert new entry in list */
    if (ins_prev == NULL) {
        cur->next = mce_info->used;
        mce_info->used = cur;
    } else {
        cur->next = ins_prev->next;
        ins_prev->next = cur;
    }
    cur->conf = *mce;

    /* Free/allocate SDX resources */
    srvl_mce_sdx_realloc(cur, action->isdx == VTSS_MCE_ISDX_NEW ? key->port_no :
                         VTSS_PORT_NO_NONE, 1);
    if (isdx != NULL)
        cur->isdx_list = isdx;
    isdx = cur->isdx_list;
    srvl_mce_sdx_realloc(cur, action->port_no, 0);
    esdx = cur->esdx_list;

    /* Add/delete IS1 entry */
    voe_idx = (action->voe_idx == VTSS_OAM_VOE_IDX_NONE ? VTSS_EVC_VOE_IDX_NONE : action->voe_idx);
    if (key->port_no < vtss_state->port_count) {
        VTSS_RC(srvl_mce_is1_add(cur));
        if (action->isdx == VTSS_MCE_ISDX_NEW && isdx != NULL) {
            /* ISDX owner, update ISDX table */
            isdx_mask = (esdx == NULL  ? 0xffffffff : VTSS_BIT(VTSS_CHIP_PORT(esdx->port_no)));
            VTSS_RC(srvl_isdx_update(action->tx_lookup == VTSS_MCE_TX_LOOKUP_VID ? 0 : 1,
                                     isdx->sdx, isdx_mask, 0, voe_idx));
        }
    } else {
        VTSS_RC(vtss_vcap_del(is1_obj, VTSS_IS1_USER_MEP, mce->id));
    }

    /* Add/delete ES0 entry */
    if (esdx != NULL && isdx != NULL) {
        vtss_vcap_es0_init(&data, &es0_entry);
        es0_key->port_no = esdx->port_no;
        es0_key->isdx_neq0 = VTSS_VCAP_BIT_1;
        if (action->tx_lookup == VTSS_MCE_TX_LOOKUP_VID) {
            es0_key->type = VTSS_ES0_TYPE_VID;
            es0_key->data.vid.vid = action->vid;
        } else {
            es0_key->type = VTSS_ES0_TYPE_ISDX;
            es0_key->data.isdx.isdx = isdx->sdx;
        }
        if (action->tx_lookup == VTSS_MCE_TX_LOOKUP_ISDX_PCP && action->prio_enable) {
            es0_key->data.isdx.pcp.value = action->prio;
            es0_key->data.isdx.pcp.mask = 0x7;
        }
        if (action->outer_tag.enable) {
            /* Add ES0 tag */
            data.u.es0.flags = VTSS_ES0_FLAG_TPID;
            data.u.es0.port_no = esdx->port_no;
            ot->tag = VTSS_ES0_TAG_ES0;
            ot->tpid = VTSS_ES0_TPID_PORT;
            ot->vid.sel = 1;
            ot->vid.val = action->outer_tag.vid;
            ot->pcp.sel = (action->outer_tag.pcp_mode == VTSS_MCE_PCP_MODE_FIXED ?
                           VTSS_ES0_PCP_ES0 : VTSS_ES0_PCP_MAPPED);
            ot->pcp.val = action->outer_tag.pcp;
            ot->dei.sel = (action->outer_tag.dei_mode == VTSS_MCE_DEI_MODE_FIXED ?
                           VTSS_ES0_DEI_ES0 : VTSS_ES0_DEI_DP);
            ot->dei.val = action->outer_tag.dei;
        }
        es0_action->esdx = esdx->sdx;
        if (voe_idx != VTSS_EVC_VOE_IDX_NONE) {
            es0_action->mep_idx_enable = 1;
            es0_action->mep_idx = voe_idx;
        }

        /* Find the next ID using ES0 rules */
        for (cur = cur->next; cur != NULL; cur = cur->next) {
            if (cur->isdx_list != NULL && cur->esdx_list != NULL) {
                break;
            }
        }
        VTSS_RC(vtss_vcap_add(es0_obj, VTSS_ES0_USER_MEP, mce->id,
                              cur == NULL ? VTSS_VCAP_ID_LAST : cur->conf.id, &data, 0));
    } else {
        VTSS_RC(vtss_vcap_del(es0_obj, VTSS_ES0_USER_MEP, mce->id));
    }

    if (isdx == NULL && isdx_old != NULL) {
        /* Remove references to deleted ISDX */
        for (cur = mce_info->used; cur != NULL; cur = cur->next) {
            if (cur->isdx_list == isdx_old)
                cur->isdx_list = NULL;
        }
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_mce_del(const vtss_mce_id_t mce_id)
{
    vtss_mce_info_t  *mce_info = &vtss_state->mce_info;
    vtss_mce_entry_t *mce, *prev = NULL;
    
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "mce_id: %u", mce_id);
    
    /* Find MCE */
    for (mce = mce_info->used; mce != NULL; prev = mce, mce = mce->next) {
        if (mce->conf.id == mce_id) {
            break;
        }
    }

    /* Check if MCE was found */
    if (mce == NULL) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "could not find mce ID: %u", mce_id);
        return VTSS_RC_ERROR;
    }

    /* Free SDX resources */
    if (mce->conf.action.isdx == VTSS_MCE_ISDX_NEW) {
        /* Own ISDX */
        srvl_mce_sdx_realloc(mce, VTSS_PORT_NO_NONE, 1);
    }
    mce->isdx_list = NULL;
    srvl_mce_sdx_realloc(mce, VTSS_PORT_NO_NONE, 0);

    /* Free IS1 and ES0 entry */
    VTSS_RC(vtss_vcap_del(&vtss_state->is1.obj, VTSS_IS1_USER_MEP, mce_id));
    VTSS_RC(vtss_vcap_del(&vtss_state->es0.obj, VTSS_ES0_USER_MEP, mce_id));

    /* Move MCE from used to free list */
    mce_info->count--;
    if (prev == NULL)
        mce_info->used = mce->next;
    else
        prev->next = mce->next;
    mce->next = mce_info->free;
    mce_info->free = mce;

    return VTSS_RC_OK;
}

static vtss_rc srvl_mce_port_get(const vtss_mce_id_t mce_id, const vtss_port_no_t port_no,
                                 vtss_mce_port_info_t *const info)

{
    vtss_mce_entry_t *mce;
    vtss_sdx_entry_t *isdx;
    
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "mce_id: %u, port_no: %u", mce_id, port_no);

    /* Find MCE */
    for (mce = vtss_state->mce_info.used; mce != NULL; mce = mce->next) {
        if (mce->conf.id == mce_id) {
            break;
        }
    }

    if (mce == NULL) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "could not find mce ID: %u", mce_id);
        return VTSS_RC_ERROR;
    }
    
    isdx = mce->isdx_list;
    if (isdx == NULL || isdx->port_no != port_no) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "could not find ISDX for mce ID: %u, port_no: %u",
                mce_id, port_no);
        return VTSS_RC_ERROR;
    }
    info->isdx = isdx->sdx;

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  EVC
 * ================================================================= */

static vtss_rc srvl_evc_policer_conf_set(const vtss_evc_policer_id_t policer_id)
{
    vtss_evc_policer_conf_t *conf = &vtss_state->evc_policer_conf[policer_id];
    srvl_policer_conf_t     pol_conf;

    /* Convert to Serval policer configuration */
    memset(&pol_conf, 0, sizeof(pol_conf));
    pol_conf.dual = 1;
    if (conf->enable) {
        /* Use configured values if policer enabled */
        if (conf->eir == 0) {
            /* Use single leaky bucket if EIR is zero */
            pol_conf.dual = 0;
            pol_conf.eir = conf->cir;
            pol_conf.ebs = conf->cbs;
        } else {
            pol_conf.cir = conf->cir;
            pol_conf.cbs = conf->cbs;
            pol_conf.eir = conf->eir;
            pol_conf.ebs = conf->ebs;
            pol_conf.cf = conf->cf;
        }
    } else {
        /* Use maximum rates if policer disabled */
        pol_conf.cir = 100000000; /* 100 Gbps, will be rounded down */
        pol_conf.eir = 100000000; /* 100 Gbps, will be rounded down */
    }

    /* Make sure burst size is non-zero (value zero disables policer) */
    if (pol_conf.cbs == 0)
        pol_conf.cbs = 1;
    if (pol_conf.ebs == 0)
        pol_conf.ebs = 1;

    return srvl_policer_conf_set(SRVL_POLICER_EVC + policer_id, &pol_conf);
}

static vtss_vcap_id_t srvl_evc_vcap_id(vtss_ece_id_t ece_id, vtss_port_no_t port_no)
{
    vtss_vcap_id_t id = port_no;

    return ((id << 32) + ece_id);
}

/* Determine if IS1 rule is needed for UNI/NNI port */
static BOOL srvl_ece_is1_needed(BOOL nni, vtss_ece_dir_t dir, vtss_ece_rule_t rule) 
{
    return (nni ? 
            /* NNI port */
            (dir == VTSS_ECE_DIR_UNI_TO_NNI ? 0 :
             dir == VTSS_ECE_DIR_NNI_TO_UNI ? (rule == VTSS_ECE_RULE_TX ? 0 : 1) :
             (rule == VTSS_ECE_RULE_RX ? 0 : 1)) :
            /* UNI port */
            (dir == VTSS_ECE_DIR_NNI_TO_UNI ? 0 :
             dir == VTSS_ECE_DIR_UNI_TO_NNI ? (rule == VTSS_ECE_RULE_TX ? 0 : 1) :
             (rule == VTSS_ECE_RULE_TX ? 0 : 1)));
}

/* Determine if ES0 rule is needed for UNI/NNI port */
static BOOL srvl_ece_es0_needed(BOOL nni, vtss_ece_dir_t dir, vtss_ece_rule_t rule) 
{
    return (nni ? 
            /* NNI port */
            (dir == VTSS_ECE_DIR_NNI_TO_UNI ? 0 :
             dir == VTSS_ECE_DIR_UNI_TO_NNI ? (rule == VTSS_ECE_RULE_RX ? 0 : 1) :
             (rule == VTSS_ECE_RULE_RX ? 0 : 1)) :
            /* UNI port */
            (dir == VTSS_ECE_DIR_UNI_TO_NNI ? 0 :
             dir == VTSS_ECE_DIR_NNI_TO_UNI ? (rule == VTSS_ECE_RULE_RX ? 0 : 1) :
             (rule == VTSS_ECE_RULE_TX ? 0 : 1)));
}

static vtss_rc srvl_evc_range_alloc(vtss_vcap_range_chk_table_t *range, vtss_ece_entry_t *ece,
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

static vtss_rc srvl_evc_range_free(vtss_vcap_range_chk_table_t *range, vtss_vcap_id_t id)
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

static vtss_rc srvl_ece_isdx_update(vtss_evc_entry_t *evc, vtss_ece_entry_t *ece, 
                                    vtss_sdx_entry_t *isdx)
{
    BOOL             isdx_ena = (ece->act_flags & VTSS_ECE_ACT_TX_LOOKUP_ISDX ? 1 : 0);
    BOOL             nni = VTSS_PORT_BF_GET(evc->ports, isdx->port_no);
    u32              isdx_mask = (isdx_ena ? 0 : 0xffffffff), pol_idx, voe_idx;
    vtss_sdx_entry_t *esdx;
    
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "ece_id: %u, isdx: %u, port_no: %u",
            ece->ece_id, isdx->sdx, isdx->port_no);

    /* Find ISDX port mask */
    for (esdx = ece->esdx_list; isdx_ena && esdx != NULL; esdx = esdx->next) {
        if (esdx->port_no != isdx->port_no) {
            isdx_mask |= VTSS_BIT(VTSS_CHIP_PORT(esdx->port_no));
        }
    }
    
    /* Map to policer */
    if (nni && vtss_cmn_ece_dir_get(ece) == VTSS_ECE_DIR_BOTH) {
        /* NNI port with DIR_BOTH, no policing */
        pol_idx = VTSS_EVC_POLICER_ID_NONE;
    } else if ((pol_idx = ece->policer_id) == VTSS_EVC_POLICER_ID_EVC)
        pol_idx = evc->policer_id;

    if (pol_idx == VTSS_EVC_POLICER_ID_DISCARD)
        pol_idx = (SRVL_POLICER_DISCARD - SRVL_POLICER_EVC + 1);
    else if (pol_idx < vtss_state->evc_policer_max)
        pol_idx++;
    else
        pol_idx = 0;

    voe_idx = evc->voe_idx[isdx->port_no];
    return srvl_isdx_update(isdx_ena, isdx->sdx, isdx_mask, pol_idx, voe_idx);
}

static vtss_rc srvl_sdx_counters_update(vtss_sdx_entry_t *isdx, vtss_sdx_entry_t *esdx, 
                                        vtss_evc_counters_t *const counters, BOOL clear)
{
    u32                 base, *p = &base;
    vtss_sdx_counters_t *c;
    
    /* ISDX counters */
    if (isdx != NULL && isdx->port_no < vtss_state->port_count) {
        /* Setup counter view */
        SRVL_WR(VTSS_SYS_SYSTEM_STAT_CFG, VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_VIEW(isdx->sdx));
        
        c = &vtss_state->sdx_info.sdx_table[isdx->sdx];
        base = 0xc0;
        VTSS_RC(srvl_counter_update(p, &c->rx_green.bytes, clear));
        VTSS_RC(srvl_counter_update(p, &c->rx_green.frames, clear));
        VTSS_RC(srvl_counter_update(p, &c->rx_yellow.bytes, clear));
        VTSS_RC(srvl_counter_update(p, &c->rx_yellow.frames, clear));
        VTSS_RC(srvl_counter_update(p, &c->rx_red.bytes, clear));
        VTSS_RC(srvl_counter_update(p, &c->rx_red.frames, clear));
        VTSS_RC(srvl_counter_update(p, &c->rx_discard.bytes, clear));  /* drop_green */
        VTSS_RC(srvl_counter_update(p, &c->rx_discard.frames, clear)); /* drop_green */
        VTSS_RC(srvl_counter_update(p, &c->tx_discard.bytes, clear));  /* drop_yellow */
        VTSS_RC(srvl_counter_update(p, &c->tx_discard.frames, clear)); /* drop_yellow */

        if (counters != NULL) {
            /* Add counters */
            counters->rx_green.bytes += c->rx_green.bytes.value;
            counters->rx_green.frames += c->rx_green.frames.value;
            counters->rx_yellow.bytes += c->rx_yellow.bytes.value;
            counters->rx_yellow.frames += c->rx_yellow.frames.value;
            counters->rx_red.bytes += c->rx_red.bytes.value;
            counters->rx_red.frames += c->rx_red.frames.value;
            counters->rx_discard.bytes += (c->rx_discard.bytes.value + c->tx_discard.bytes.value);
            counters->rx_discard.frames += (c->rx_discard.frames.value + c->tx_discard.frames.value);
        }
    }

    /* ESDX counters */
    if (esdx != NULL && esdx->port_no < vtss_state->port_count) {
        /* Setup counter view */
        SRVL_WR(VTSS_SYS_SYSTEM_STAT_CFG, VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_VIEW(esdx->sdx));
        
        c = &vtss_state->sdx_info.sdx_table[esdx->sdx];
        base = 0x100;
        VTSS_RC(srvl_counter_update(p, &c->tx_green.bytes, clear));
        VTSS_RC(srvl_counter_update(p, &c->tx_green.frames, clear));
        VTSS_RC(srvl_counter_update(p, &c->tx_yellow.bytes, clear));
        VTSS_RC(srvl_counter_update(p, &c->tx_yellow.frames, clear));
        
        if (counters != NULL) {
            /* Add counters */
            counters->tx_green.bytes += c->tx_green.bytes.value;
            counters->tx_green.frames += c->tx_green.frames.value;
            counters->tx_yellow.bytes += c->tx_yellow.bytes.value;
            counters->tx_yellow.frames += c->tx_yellow.frames.value;
        }
    }
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_ece_sdx_alloc(vtss_ece_entry_t *ece, vtss_port_no_t port_no, BOOL isdx)
{
    return (vtss_cmn_ece_sdx_alloc(ece, port_no, isdx) == NULL ? VTSS_RC_ERROR : VTSS_RC_OK);
}

static void srvl_ece_nni_is1_tag_set(vtss_ece_entry_t *ece, vtss_is1_tag_t *tag, vtss_vid_t vid, 
                                     u8 pcp_val, vtss_vcap_u8_t *pcp_key, 
                                     u32 mask_pcp_fixed, u32 mask_pcp_mapped,
                                     u32 mask_dei, u32 mask_dei_fixed, u32 mask_dei_dp)
{
    /* NNI->UNI: Match the same tag as pushed in the UNI->NNI direction */
    
    /* Match VID */
    tag->tagged = VTSS_VCAP_BIT_1;
    tag->vid.vr.v.value = vid;
    tag->vid.vr.v.mask = 0xfff;

    if (ece->act_flags & mask_pcp_fixed) {
        /* Fixed PCP */
        tag->pcp.value = pcp_val;
        tag->pcp.mask = 0x7;
    } else if (ece->act_flags & mask_pcp_mapped) {
        /* Mapped (QOS, DP), match any PCP */
    } else {
        /* Preserved PCP */
        tag->pcp = *pcp_key;
    }

    if (ece->act_flags & mask_dei_fixed) {
        /* Fixed DEI */
        tag->dei = (ece->act_flags & mask_dei ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_0);
    } else if (ece->act_flags & mask_dei_dp) {
        /* DP based DEI, match any DEI */
        tag->dei = VTSS_VCAP_BIT_ANY;
    } else {
        /* Preserved DEI */
        tag->dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_DEI_VLD, VTSS_ECE_KEY_TAG_DEI_1);
    }
}

static vtss_ece_tx_lookup_t srvl_tx_lookup_get(vtss_ece_entry_t *ece)
{
    u32 flags = ece->act_flags;
    
    return ((flags & VTSS_ECE_ACT_TX_LOOKUP_VID_PCP) ? VTSS_ECE_TX_LOOKUP_VID_PCP :
            (flags & VTSS_ECE_ACT_TX_LOOKUP_ISDX) ? VTSS_ECE_TX_LOOKUP_ISDX : 
            VTSS_ECE_TX_LOOKUP_VID);
}

static vtss_rc srvl_ece_is1_add(vtss_vcap_id_t id, vtss_vcap_id_t id_next,
                                vtss_evc_entry_t *evc, vtss_ece_entry_t *ece, 
                                vtss_sdx_entry_t *isdx)
{
    vtss_vcap_data_t            data;
    vtss_is1_entry_t            is1;
    vtss_is1_key_t              *key = &is1.key;
    vtss_is1_mac_t              *mac = &key->mac;
    vtss_is1_tag_t              *tag;
    vtss_is1_frame_ipv4_t       *ipv4 = &key->frame.ipv4;
    vtss_is1_frame_ipv6_t       *ipv6 = &key->frame.ipv6;
    vtss_is1_action_t           *action = &is1.action;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range;
    vtss_port_no_t              port_no = isdx->port_no;
    BOOL                        prio_enable = (ece->act_flags & VTSS_ECE_ACT_PRIO_ENA ? 1 : 0);
    BOOL                        nni = VTSS_PORT_BF_GET(evc->ports, port_no);
    vtss_ece_dir_t              dir = vtss_cmn_ece_dir_get(ece);
    vtss_ece_tx_lookup_t        tx_lookup = srvl_tx_lookup_get(ece);

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "id: %s, id_next: %s",
            vtss_vcap_id_txt(id), vtss_vcap_id_txt(id_next));

    /* Free old range checkers */
    VTSS_RC(srvl_evc_range_free(&range_new, id));
    
    vtss_vcap_is1_init(&data, &is1);
    data.u.is1.lookup = 0; /* First lookup */

    data.key_size = srvl_evc_port_key_get(port_no, &key->key_type);
    key->type = VTSS_IS1_TYPE_ANY;
    key->port_list[port_no] = 1;
    
    action->pop_enable = 1;
    if (nni && dir == VTSS_ECE_DIR_BOTH) {
        /* NNI port with DIR_BOTH */

        /* Outer tag matching */
        srvl_ece_nni_is1_tag_set(ece, &key->tag, evc->vid, ece->ot_pcp, &ece->pcp,
                                 VTSS_ECE_ACT_OT_PCP_MODE_FIXED, 
                                 VTSS_ECE_ACT_OT_PCP_MODE_MAPPED, 
                                 VTSS_ECE_ACT_OT_DEI, 
                                 VTSS_ECE_ACT_OT_DEI_MODE_FIXED,
                                 VTSS_ECE_ACT_OT_DEI_MODE_DP);
        
        /* NNI->UNI: Pop the same number of tags as pushed in the UNI->NNI direction */
        action->pop = 1;
        tag = &key->inner_tag;
        if (tx_lookup == VTSS_ECE_TX_LOOKUP_ISDX) {
            /* ISDX-based ES0 lookup, inner tag popping/matching may be done */
            if (ece->act_flags & VTSS_ECE_ACT_IT_TYPE_USED) {
                /* Inner tag used */
                action->pop = 2;
                srvl_ece_nni_is1_tag_set(ece, tag, ece->it_vid, ece->it_pcp, &ece->pcp,
                                         VTSS_ECE_ACT_IT_PCP_MODE_FIXED,
                                         VTSS_ECE_ACT_IT_PCP_MODE_MAPPED,
                                         VTSS_ECE_ACT_IT_DEI, 
                                         VTSS_ECE_ACT_IT_DEI_MODE_FIXED,
                                         VTSS_ECE_ACT_IT_DEI_MODE_DP);
            } else {
                /* No inner tag pushed */
                if (!(ece->act_flags & VTSS_ECE_ACT_POP_1) && 
                    (ece->key_flags & VTSS_ECE_KEY_TAG_TAGGED_VLD) &&
                    (ece->key_flags & VTSS_ECE_KEY_TAG_TAGGED_1) &&
                    ece->vid.type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
                    /* Tag preserved in UNI->NNI direction, match tag in NNI->UNI direction */
                    tag->tagged = VTSS_VCAP_BIT_1;
                    tag->s_tag = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_S_TAGGED_VLD,
                                                      VTSS_ECE_KEY_TAG_S_TAGGED_1);
                    tag->vid = ece->vid;
                    tag->pcp = ece->pcp;
                    tag->dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_DEI_VLD,
                                                    VTSS_ECE_KEY_TAG_DEI_1);
                }
            }
        } else if (tx_lookup == VTSS_ECE_TX_LOOKUP_VID) {
            /* VID-based ES0 lookup, I-NNI assumed */
            prio_enable = 0; /* Use basic classification */
        }

    } else {
        /* UNI port or unidirectional NNI port */
        
        /* Allocate range checkers for UNI/NNIs */
        VTSS_RC(srvl_evc_range_alloc(&range_new, ece, &data.u.is1));

        /* MAC header matching */
        mac->dmac_mc = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_DMAC_MC_VLD, VTSS_ECE_KEY_DMAC_MC_1);
        mac->dmac_bc = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_DMAC_BC_VLD, VTSS_ECE_KEY_DMAC_BC_1);
        mac->smac = ece->smac;
        mac->dmac = ece->dmac;
        
        /* Outer tag matching */
        tag = &key->tag;
        tag->vid = ece->vid;
        tag->pcp = ece->pcp;
        tag->dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_DEI_VLD, VTSS_ECE_KEY_TAG_DEI_1);
        tag->tagged = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_TAGGED_VLD,
                                           VTSS_ECE_KEY_TAG_TAGGED_1);
        tag->s_tag = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_TAG_S_TAGGED_VLD,
                                          VTSS_ECE_KEY_TAG_S_TAGGED_1);

        /* Inner tag matching */
        tag = &key->inner_tag;
        tag->vid = ece->in_vid;
        tag->pcp = ece->in_pcp;
        tag->dei = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_IN_TAG_DEI_VLD,
                                        VTSS_ECE_KEY_IN_TAG_DEI_1);
        tag->tagged = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_IN_TAG_TAGGED_VLD,
                                           VTSS_ECE_KEY_IN_TAG_TAGGED_1);
        tag->s_tag = vtss_cmn_ece_bit_get(ece, VTSS_ECE_KEY_IN_TAG_S_TAGGED_VLD,
                                          VTSS_ECE_KEY_IN_TAG_S_TAGGED_1);
        
        /* IP header matching */
        if (ece->key_flags & VTSS_ECE_KEY_PROT_IPV4) {
            key->type = VTSS_IS1_TYPE_IPV4;
            ipv4->fragment = ece->frame.ipv4.fragment;
            ipv4->proto = ece->frame.ipv4.proto;
            ipv4->dscp = ece->frame.ipv4.dscp;
            ipv4->sip = ece->frame.ipv4.sip;
            ipv4->dip = ece->frame.ipv4.dip;
            ipv4->sport = ece->frame.ipv4.sport;
            ipv4->dport = ece->frame.ipv4.dport;
        } else if (ece->key_flags & VTSS_ECE_KEY_PROT_IPV6) {
            key->type = VTSS_IS1_TYPE_IPV6;
            ipv6->proto = ece->frame.ipv6.proto;
            ipv6->dscp = ece->frame.ipv6.dscp;
            ipv6->sip = ece->frame.ipv6.sip;
            ipv6->dip = ece->frame.ipv6.dip;
            ipv6->sport = ece->frame.ipv6.sport;
            ipv6->dport = ece->frame.ipv6.dport;
        }

        /* Tag pop count */
        if (ece->act_flags & VTSS_ECE_ACT_POP_1)
            action->pop = 1;
        else if (ece->act_flags & VTSS_ECE_ACT_POP_2)
            action->pop = 2;
    }

#if defined(VTSS_FEATURE_MPLS)
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "MPLS ingress XC %d, PB VID %d, IVID %d",
            evc->pw_ingress_xc, evc->vid, evc->ivid);

    if (VTSS_MPLS_IDX_IS_DEF(evc->pw_ingress_xc)) {
        const vtss_mpls_xc_internal_t *xc     = &XC_I(evc->pw_ingress_xc);
        const vtss_mpls_segment_idx_t seg_idx = xc->pub.in_seg_idx;

        if (VTSS_MPLS_IDX_IS_DEF(seg_idx)) {
            vtss_mpls_segment_state_t state;
            vtss_port_no_t            l2_port = srvl_mpls_port_no_get(seg_idx);

            (void) srvl_mpls_segment_state_get(seg_idx, &state);

            if (state == VTSS_MPLS_SEGMENT_STATE_UP  &&  l2_port == port_no) {
                key->isdx.value[0] = xc->isdx->sdx >> 8;
                key->isdx.value[1] = xc->isdx->sdx & 0xff;
                key->isdx.mask[0]  = 0xff;
                key->isdx.mask[1]  = 0xff;
                VTSS_DG(VTSS_TRACE_GROUP_MPLS, "MPLS IS1 (PW) cfg for evc_id %u: XC %d, seg %d, state %s, PW ISDX %d -> SDX %d",
                        ece->evc_id, evc->pw_ingress_xc, seg_idx,
                        vtss_mpls_segment_state_to_str(state),
                        xc->isdx->sdx, isdx->sdx);
            }
        }
    }
#endif /* VTSS_FEATURE_MPLS */

    /* Commit range checkers */
    VTSS_RC(vtss_vcap_range_commit(&range_new));

    action->vid_enable = 1;
    action->vid = evc->ivid;
    action->isdx_enable = 1;
    action->isdx = isdx->sdx;
    if (!(ece->act_flags & VTSS_ECE_ACT_POLICY_NONE)) {
        action->pag_enable = 1;
        action->pag = ece->policy_no;
    }

    if (nni && dir == VTSS_ECE_DIR_NNI_TO_UNI && 
        tx_lookup == VTSS_ECE_TX_LOOKUP_VID_PCP && ece->pcp.mask == 0) {
        /* Unidirectional rule for NNI port with (VID, PCP) egress lookup and matching any PCP.
           It is assumed that this is an I-NNI port using basic classification */
        prio_enable = 0;
    }

    if (prio_enable) {
        action->prio_enable = 1;
        action->prio = ece->prio;
        action->pcp_dei_enable = 1; /* Use COS = IFH.PCP = IFH.QOS */
        action->pcp = ece->prio;
    }
    if (ece->act_flags & VTSS_ECE_ACT_DP_ENA) {
        action->dp_enable = 1;
        action->dp = ece->dp;
    }
    return vtss_vcap_add(&vtss_state->is1.obj, VTSS_IS1_USER_EVC, id, id_next, &data, 0);
}

static void srvl_ece_es0_tag_set(vtss_ece_entry_t *ece, vtss_es0_tag_conf_t *tag,
                                 vtss_vid_t vid, u8 pcp, u32 mask_pcp_fixed, u32 mask_pcp_mapped, 
                                 u32 mask_dei_fixed, u32 mask_dei_dp, u32 mask_dei)
{
    tag->tag = VTSS_ES0_TAG_ES0;
    tag->tpid = VTSS_ES0_TPID_PORT;
    tag->vid.sel = 1;
    tag->vid.val = vid;
    tag->pcp.sel = ((ece->act_flags & mask_pcp_fixed) ? VTSS_ES0_PCP_ES0 : 
                    (ece->act_flags & mask_pcp_mapped) ? VTSS_ES0_PCP_MAPPED : VTSS_ES0_PCP_CLASS);
    tag->pcp.val = pcp;
    tag->dei.sel = ((ece->act_flags & mask_dei_fixed) ? VTSS_ES0_DEI_ES0 :
                    (ece->act_flags & mask_dei_dp) ? VTSS_ES0_DEI_DP : VTSS_ES0_DEI_CLASS);
    tag->dei.val = (ece->act_flags & mask_dei ? 1 : 0);
}

static vtss_rc srvl_ece_es0_add(vtss_vcap_id_t id, vtss_vcap_id_t id_next,
                                vtss_evc_entry_t *evc, vtss_ece_entry_t *ece, 
                                vtss_sdx_entry_t *esdx)
{
    vtss_vcap_data_t     data;
    vtss_es0_data_t      *es0 = &data.u.es0;
    vtss_es0_entry_t     entry;
    vtss_es0_key_t       *key = &entry.key;
    vtss_es0_action_t    *action = &entry.action;
    vtss_es0_tag_conf_t  *tag = &action->outer_tag;
    vtss_port_no_t       port_no = esdx->port_no;
    vtss_sdx_entry_t     *isdx;
    vtss_qos_port_conf_t *qos_port_conf;
    u8                   pcp, mep_idx;
    BOOL                 pcp_map, nni = VTSS_PORT_BF_GET(evc->ports, port_no);
    vtss_ece_dir_t       dir = vtss_cmn_ece_dir_get(ece);
    vtss_ece_tx_lookup_t tx_lookup = srvl_tx_lookup_get(ece);

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "id: %s, id_next: %s",
            vtss_vcap_id_txt(id), vtss_vcap_id_txt(id_next));

    vtss_vcap_es0_init(&data, &entry);
    es0->port_no = port_no;
    es0->flags = VTSS_ES0_FLAG_TPID;
    key->port_no = port_no;
    key->isdx_neq0 = VTSS_VCAP_BIT_1;
    action->esdx = esdx->sdx;
    if (tx_lookup == VTSS_ECE_TX_LOOKUP_ISDX) {
        /* ISDX based forwarding */
        key->type = VTSS_ES0_TYPE_ISDX;
        for (isdx = ece->isdx_list; isdx != NULL; isdx = isdx->next) {
            if (isdx->port_no != port_no) {
                key->data.isdx.isdx = isdx->sdx;
                break;
            }
        }
    } else {
        /* VLAN based forwarding */
        key->type = VTSS_ES0_TYPE_VID;
        key->data.vid.vid = evc->ivid;
        if (ece->act_flags & VTSS_ECE_ACT_PRIO_ENA) {
            pcp_map = 0;
            if (tx_lookup == VTSS_ECE_TX_LOOKUP_VID_PCP) {
                if (!nni && dir == VTSS_ECE_DIR_NNI_TO_UNI && ece->pcp.mask == 0) {
                    /* UNI port for unidirectional rule matching any PCP: I-NNI assumed */
                    pcp_map = 1;
                } else {
                    /* Match (VID, PCP), where PCP = COS */
                    key->data.vid.pcp.value = ece->prio;
                    key->data.vid.pcp.mask = 0x7;
                }
            } else if (!nni && dir == VTSS_ECE_DIR_BOTH &&
                       (ece->act_flags & VTSS_ECE_ACT_OT_PCP_MODE_MAPPED)) {
                /* UNI port for bidirectional rule with VID lookup and mapped PCP: I-NNI assumed */
                pcp_map = 1;
            }
            if (pcp_map) {
                /* On the I-NNI, find the PCP value mapping to the QOS and use that as key */
                for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
                    if (VTSS_PORT_BF_GET(evc->ports, port_no)) {
                        /* NNI port */
                        qos_port_conf = &vtss_state->qos_port_conf[port_no];
                        for (pcp = 0; pcp < 8; pcp++) {
                            if (qos_port_conf->qos_class_map[pcp][0] == ece->prio) {
                                es0->flags |= VTSS_ES0_FLAG_PCP_MAP;
                                es0->prio = ece->prio;
                                es0->nni = port_no;
                                key->data.vid.pcp.value = pcp;
                                key->data.vid.pcp.mask = 0x7;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
    } 
    mep_idx = evc->voe_idx[esdx->port_no];
    if (mep_idx != VTSS_EVC_VOE_IDX_NONE) {
        action->mep_idx_enable = 1;
        action->mep_idx = mep_idx;
    }

    if (nni) {
        /* NNI port */

        /* Outer tag */
        srvl_ece_es0_tag_set(ece, tag, evc->vid, ece->ot_pcp, 
                             VTSS_ECE_ACT_OT_PCP_MODE_FIXED, VTSS_ECE_ACT_OT_PCP_MODE_MAPPED,
                             VTSS_ECE_ACT_OT_DEI_MODE_FIXED, VTSS_ECE_ACT_OT_DEI_MODE_DP,
                             VTSS_ECE_ACT_OT_DEI);

        /* Inner tag */
        if (ece->act_flags & VTSS_ECE_ACT_IT_TYPE_USED) {
            tag = &action->inner_tag;
            srvl_ece_es0_tag_set(ece, tag, ece->it_vid, ece->it_pcp, 
                                 VTSS_ECE_ACT_IT_PCP_MODE_FIXED, VTSS_ECE_ACT_IT_PCP_MODE_MAPPED,
                                 VTSS_ECE_ACT_IT_DEI_MODE_FIXED, VTSS_ECE_ACT_IT_DEI_MODE_DP,
                                 VTSS_ECE_ACT_IT_DEI);
            if (ece->act_flags & VTSS_ECE_ACT_IT_TYPE_C)
                tag->tpid = VTSS_ES0_TPID_C;
            else if (ece->act_flags & VTSS_ECE_ACT_IT_TYPE_S)
                tag->tpid = VTSS_ES0_TPID_S;
        }
    } else if (ece->act_flags & VTSS_ECE_ACT_OT_ENA) {
        /* UNI port, direction NNI_TO_UNI, push one tag */
        srvl_ece_es0_tag_set(ece, tag, ece->ot_vid, ece->ot_pcp,
                             VTSS_ECE_ACT_OT_PCP_MODE_FIXED, VTSS_ECE_ACT_OT_PCP_MODE_MAPPED,
                             VTSS_ECE_ACT_OT_DEI_MODE_FIXED, VTSS_ECE_ACT_OT_DEI_MODE_DP,
                             VTSS_ECE_ACT_OT_DEI);
    } else if ((ece->act_flags & VTSS_ECE_ACT_POP_1) && 
               (ece->key_flags & VTSS_ECE_KEY_TAG_TAGGED_1) &&
               ece->vid.vr.v.mask == 0xfff && ece->vid.vr.v.value != VTSS_VID_NULL) {
        /* UNI port, direction BOTH, push one tag */
        srvl_ece_es0_tag_set(ece, tag, ece->vid.vr.v.value, 0, 
                             VTSS_ECE_ACT_OT_PCP_MODE_FIXED, VTSS_ECE_ACT_OT_PCP_MODE_MAPPED,
                             VTSS_ECE_ACT_OT_DEI_MODE_FIXED, VTSS_ECE_ACT_OT_DEI_MODE_DP,
                             VTSS_ECE_ACT_OT_DEI);

        if (ece->act_flags & VTSS_ECE_ACT_OT_PCP_MODE_FIXED) {
            /* If PCP is fixed, the smallest matching PCP is used */
            for (pcp = 0; pcp < 8; pcp++) {
                if ((pcp & ece->pcp.mask) == (ece->pcp.value & ece->pcp.mask)) {
                        tag->pcp.val = pcp;
                        break;
                }
            }
        }
        
        if (ece->act_flags & (VTSS_ECE_ACT_OT_DEI_MODE_FIXED | VTSS_ECE_ACT_OT_DEI_MODE_DP)) {
            /* If DEI is fixed or DP based, the smallest matching DEI is used */
            tag->dei.sel = VTSS_ES0_DEI_ES0;
            tag->dei.val = (ece->key_flags & VTSS_ECE_KEY_TAG_DEI_1 ? 1 : 0);
        }
    }

#if defined(VTSS_FEATURE_MPLS)
    VTSS_DG(VTSS_TRACE_GROUP_MPLS, "MPLS egress XC %d, PB VID %d, IVID %d",
            evc->pw_egress_xc, evc->vid, evc->ivid);

    if (VTSS_MPLS_IDX_IS_DEF(evc->pw_egress_xc)) {
        const vtss_mpls_segment_idx_t seg_idx = XC_P(evc->pw_egress_xc).out_seg_idx;

        if (VTSS_MPLS_IDX_IS_DEF(seg_idx)) {
            vtss_mpls_segment_state_t    state;
            vtss_mpls_segment_internal_t *seg    = &SEG_I(seg_idx);
            vtss_port_no_t               l2_port = srvl_mpls_port_no_get(seg_idx);

            (void) srvl_mpls_segment_state_get(seg_idx, &state);

            if (state == VTSS_MPLS_SEGMENT_STATE_UP  &&  l2_port == port_no) {
                action->mpls_encap_idx = seg->u.out.encap_idx;
                action->mpls_encap_len = srvl_bytes_to_encap_len(seg->u.out.encap_bytes);
                VTSS_DG(VTSS_TRACE_GROUP_MPLS, "MPLS encap cfg for evc_id %u: XC %d, seg %d, state %s, encap %d/%d",
                        ece->evc_id, evc->pw_egress_xc, seg_idx,
                        vtss_mpls_segment_state_to_str(state),
                        action->mpls_encap_idx, action->mpls_encap_len);
            }
        }
    }
#endif /* VTSS_FEATURE_MPLS */
    
    return vtss_vcap_add(&vtss_state->es0.obj, VTSS_ES0_USER_EVC, id, id_next, &data, 0);
}

static vtss_rc srvl_ece_update(vtss_ece_entry_t *ece, vtss_res_t *res, vtss_res_cmd_t cmd)
{
    vtss_evc_entry_t            *evc;
    vtss_ece_id_t               ece_id = ece->ece_id;
    vtss_ece_entry_t            *ece_next;
    vtss_vcap_id_t              id, id_next_is1, id_next_es0;
    vtss_port_no_t              port_no;
    BOOL                        nni;
    vtss_vcap_data_t            data;
    vtss_sdx_entry_t            *sdx, *sdx_next;
    vtss_ece_dir_t              dir;
    vtss_ece_rule_t             rule;
    vtss_vcap_key_size_t        key_size;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range;
    
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "ece_id: %u, cmd: %d", ece->ece_id, cmd);

    if (cmd == VTSS_RES_CMD_CALC) {
        /* Calculate resource usage */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            nni = res->port_nni[port_no];
            key_size = srvl_evc_port_key_get(port_no, NULL);
            if (res->port_add[port_no] || res->port_chg[port_no]) {
                /* Add/change port */
                if (srvl_ece_is1_needed(nni, res->dir_new, res->rule_new)) {
                    res->is1.add_key[key_size]++;
                    res->isdx.add++;
                }
                if (srvl_ece_es0_needed(nni, res->dir_new, res->rule_new)) {
                    res->es0.add++;
                    res->esdx.add++;
                }
            } 
            
            if (res->port_del[port_no] || res->port_chg[port_no]) {
                /* Delete/change port */
                if (srvl_ece_is1_needed(nni, res->dir_old, res->rule_old)) {
                    /* Free range checkers for each deleted port */
                    id = srvl_evc_vcap_id(ece_id, port_no);
                    VTSS_RC(srvl_evc_range_free(&range_new, id));
                    res->is1.del_key[key_size]++;
                    res->isdx.del++;
                }
                if (srvl_ece_es0_needed(nni, res->dir_old, res->rule_old)) {
                    res->es0.del++;
                    res->esdx.del++;
                }
            }
        }
        
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if ((res->port_add[port_no] || res->port_chg[port_no]) && 
                srvl_ece_is1_needed(res->port_nni[port_no], res->dir_new, res->rule_new)) {
                /* Allocate range checkers for each added port */
                VTSS_RC(srvl_evc_range_alloc(&range_new, ece, &data.u.is1));
            }
        }
    } else if (cmd == VTSS_RES_CMD_DEL) {
        /* Delete resources */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (res->port_del[port_no] || res->port_chg[port_no]) {
                /* Port deleted or changed (e.g. direction) */
                nni = res->port_nni[port_no];
                id = srvl_evc_vcap_id(ece_id, port_no);

                /* Delete IS1 entry, ISDX and range checkers */
                if (srvl_ece_is1_needed(nni, res->dir_old, res->rule_old)) {
                    VTSS_RC(srvl_evc_range_free(&vtss_state->vcap_range, id));
                    VTSS_RC(vtss_vcap_del(&vtss_state->is1.obj, VTSS_IS1_USER_EVC, id));
                    VTSS_RC(vtss_cmn_ece_sdx_free(ece, port_no, 1));
                }

                /* Delete ES0 entry and ESDX */
                if (srvl_ece_es0_needed(nni, res->dir_old, res->rule_old)) {
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
                if (srvl_ece_is1_needed(nni, res->dir_new, res->rule_new))
                    VTSS_RC(srvl_ece_sdx_alloc(ece, port_no, 1));

                /* Allocate ESDX */
                if (srvl_ece_es0_needed(nni, res->dir_new, res->rule_new))
                    VTSS_RC(srvl_ece_sdx_alloc(ece, port_no, 0));
            }
        }
        
        /* Find next VCAP IDs */
        id_next_is1 = VTSS_VCAP_ID_LAST;
        id_next_es0 = VTSS_VCAP_ID_LAST;
        for (ece_next = ece->next; ece_next != NULL; ece_next = ece_next->next) {
            dir = vtss_cmn_ece_dir_get(ece_next);
            rule = vtss_cmn_ece_rule_get(ece_next);
            for (sdx = ece_next->isdx_list; sdx != NULL && id_next_is1 == VTSS_VCAP_ID_LAST; 
                 sdx = sdx->next) {
                nni = (VTSS_PORT_BF_GET(ece_next->ports, sdx->port_no) ? 0 : 1);
                if (srvl_ece_is1_needed(nni, dir, rule))
                    id_next_is1 = srvl_evc_vcap_id(ece_next->ece_id, sdx->port_no);
            }
            for (sdx = ece_next->esdx_list; sdx != NULL && id_next_es0 == VTSS_VCAP_ID_LAST; 
                 sdx = sdx->next) {
                nni = (VTSS_PORT_BF_GET(ece_next->ports, sdx->port_no) ? 0 : 1);
                if (srvl_ece_es0_needed(nni, dir, rule))
                    id_next_es0 = srvl_evc_vcap_id(ece_next->ece_id, sdx->port_no);
            }
            if (id_next_is1 != VTSS_VCAP_ID_LAST && id_next_es0 != VTSS_VCAP_ID_LAST)
                break;
        }
        
        /* Add/update IS1, ES0 and ISDX entries */
        evc = &vtss_state->evc_info.table[ece->evc_id];
        sdx_next = NULL;
        while (sdx_next != ece->isdx_list) {
            /* Start with the last ISDX */
            for (sdx = ece->isdx_list; sdx != NULL; sdx = sdx->next) {
                if (sdx->next == sdx_next) {
                    /* Update ISDX entry */
                    VTSS_RC(srvl_ece_isdx_update(evc, ece, sdx));

                    /* Add IS1 entry */
                    id = srvl_evc_vcap_id(ece_id, sdx->port_no);
                    VTSS_RC(srvl_ece_is1_add(id, id_next_is1, evc, ece, sdx));
                    id_next_is1 = id;
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
                    id = srvl_evc_vcap_id(ece_id, sdx->port_no);
                    VTSS_RC(srvl_ece_es0_add(id, id_next_es0, evc, ece, sdx));
                    id_next_es0 = id;
                    sdx_next = sdx;
                    break;
                }
            }
        }
    }

    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_MPLS)
static vtss_rc srvl_evc_mpls_update(vtss_evc_id_t evc_id, vtss_res_t *res, vtss_res_cmd_t cmd)
{
    if (cmd == VTSS_RES_CMD_CALC) {
        VTSS_RC(srvl_mpls_xc_evc_attach_calc(evc_id, res));
    } else if (cmd == VTSS_RES_CMD_DEL) {
        VTSS_RC(srvl_mpls_xc_evc_detach(evc_id));
    } else if (cmd == VTSS_RES_CMD_ADD) {
        VTSS_RC(srvl_mpls_xc_evc_attach(evc_id));
    }
    return VTSS_RC_OK;
}
#endif

static vtss_rc srvl_evc_update(vtss_evc_id_t evc_id, vtss_res_t *res, vtss_res_cmd_t cmd)
{
    vtss_ece_entry_t *ece;
    
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "evc_id: %u, cmd: %d", evc_id, cmd);

#if defined(VTSS_FEATURE_MPLS)
    VTSS_RC(srvl_evc_mpls_update(evc_id, res, cmd));
#endif

    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        if (ece->evc_id == evc_id) {
            res->dir_old = vtss_cmn_ece_dir_get(ece);
            res->dir_new = res->dir_old;
            VTSS_RC(srvl_ece_update(ece, res, cmd));
        }
    }
    return VTSS_RC_OK;
}

#endif /* VTSS_FEATURE_EVC */

static vtss_rc srvl_vcap_port_key_set(const vtss_port_no_t port_no,
                                      u8 lookup,
                                      vtss_vcap_key_type_t key_new,
                                      vtss_vcap_key_type_t key_old)
{
#if defined(VTSS_FEATURE_EVC)
    vtss_vcap_key_size_t key_size, old_key_size;
    vtss_res_t           res;
    vtss_evc_entry_t     *evc;
    vtss_ece_entry_t     *ece;
    vtss_mce_entry_t     *mce;
    vtss_ece_dir_t       dir;
    vtss_ece_rule_t      rule;
    u32                  i, add = 0;
#endif /* VTSS_FEATURE_EVC */
    u32                  ip4, ip6, other, port = VTSS_CHIP_PORT(port_no);
    
    switch (key_new) {
    case VTSS_VCAP_KEY_TYPE_NORMAL:
        /* S1_NORMAL */
        ip4 = 0;
        ip6 = 0;
        other = 0;
        break;
    case VTSS_VCAP_KEY_TYPE_DOUBLE_TAG:
        /* S1_DBL_VID */
        ip4 = 3;
        ip6 = 5;
        other = 2;
        break;
    case VTSS_VCAP_KEY_TYPE_IP_ADDR:
        /* S1_5TUPLE_IP4/S1_NORMAL */
        ip4 = 2;
        ip6 = 2;
        other = 0;
        break;
    case VTSS_VCAP_KEY_TYPE_MAC_IP_ADDR:
        /* S1_7TUPLE */
        ip4 = 1;
        ip6 = 1;
        other = 1;
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal key_type, port_no: %u", port_no);
        return VTSS_RC_ERROR;
    }

#if defined(VTSS_FEATURE_EVC)
    key_size = srvl_key_type_to_size(key_new);
    old_key_size = srvl_key_type_to_size(key_old);
    vtss_cmn_res_init(&res);
    for (i = 0; i < 2 && key_new != key_old; i++) {
        /* Change key size in two steps:
           Step 1 (i = 0): Check if resources are available
           Step 2 (i = 1): Update resources */
        /* ECE resources (first lookup) */
        for (ece = vtss_state->ece_info.used; lookup == 0 && ece != NULL; ece = ece->next) {
            if (ece->evc_id == VTSS_EVC_ID_NONE)
                continue;
            
            evc = &vtss_state->evc_info.table[ece->evc_id];
            if (!evc->enable)
                continue;

            dir = vtss_cmn_ece_dir_get(ece);
            rule = vtss_cmn_ece_rule_get(ece);
            if ((VTSS_PORT_BF_GET(evc->ports, port_no) && srvl_ece_is1_needed(1, dir, rule)) ||
                (VTSS_PORT_BF_GET(ece->ports, port_no) && srvl_ece_is1_needed(0, dir, rule))) {
                /* IS1 rule needed for UNI/NNI port */
                if (i == 0) {
                    /* Update ressource information */
                    add = 1;
                    res.is1.add_key[key_size]++;
                    res.is1.del_key[old_key_size]++;
                } else {
                    /* Update resources */
                    res.dir_new = dir;
                    VTSS_RC(srvl_ece_update(ece, &res, VTSS_RES_CMD_ADD));
                }
            }
        }

        /* MCE resources (first or second lookup) */
        for (mce = vtss_state->mce_info.used; mce != NULL; mce = mce->next) {
            if (mce->conf.key.port_no == port_no && mce->conf.key.lookup == lookup) {
                /* IS1 rule needed for ingress port and lookup */
                if (i == 0) {
                    add = 1;
                    res.is1.add_key[key_size]++;
                    res.is1.del_key[old_key_size]++;
                } else {
                    /* Update IS1 entry */
                    VTSS_RC(srvl_mce_is1_add(mce));
                }
            }
        }

        if (i == 0 && add) {
            /* Check if resources are available. 
               The resource change operation implemented above does not delete old rules 
               before adding new rules, so we need an extra row to be able to do the update. */
            res.is1.add++;
            VTSS_RC(vtss_cmn_res_check(&res));

            /* Even if the total resource consumption goes down, we need that extra row */
            vtss_cmn_res_init(&res);
            res.is1.add++;
            VTSS_RC(vtss_cmn_res_check(&res));
        }
    }
#endif /* VTSS_FEATURE_EVC */
    
    /* Setup port key */
    SRVL_WR(VTSS_ANA_PORT_VCAP_S1_KEY_CFG(port, lookup), 
            VTSS_F_ANA_PORT_VCAP_S1_KEY_CFG_S1_KEY_IP6_CFG(ip6) |
            VTSS_F_ANA_PORT_VCAP_S1_KEY_CFG_S1_KEY_IP4_CFG(ip4) |
            VTSS_F_ANA_PORT_VCAP_S1_KEY_CFG_S1_KEY_OTHER_CFG(other));

    return VTSS_RC_OK;
}

static vtss_rc srvl_vcap_port_conf_set(const vtss_port_no_t port_no)
{
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "port_no: %u", port_no);
    
    return srvl_vcap_port_key_set(port_no, 1,
                                  vtss_state->vcap_port_conf[port_no].key_type_is1_1,
                                  vtss_state->vcap_port_conf_old.key_type_is1_1);
}

#if defined(VTSS_FEATURE_EVC)
static vtss_rc srvl_evc_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_evc_port_conf_t *conf = &vtss_state->evc_port_conf[port_no];
    u32                  mask, port = VTSS_CHIP_PORT(port_no);

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "port_no: %u", port_no);

    /* DMAC/DIP, first lookup */
    mask = VTSS_F_ANA_PORT_VCAP_CFG_S1_DMAC_DIP_ENA(1);
    SRVL_WRM(VTSS_ANA_PORT_VCAP_CFG(port), conf->dmac_dip ? mask : 0, mask);

    return srvl_vcap_port_key_set(port_no, 0,
                                  vtss_state->evc_port_conf[port_no].key_type,
                                  vtss_state->evc_port_conf_old.key_type);
}

static vtss_rc srvl_evc_poll_1sec(void)
{
    vtss_sdx_info_t  *sdx_info = &vtss_state->sdx_info;
    vtss_sdx_entry_t *isdx, *esdx;
    u32              i, idx;

    /* Poll counters for 30 SDX entries, giving 1023/30 = 34 seconds between each poll.
       This ensures that any counter can wrap only once between each poll.
       The worst case is considered 32-bit byte counter on a 1Gbps port, which takes about
       0xffffffff/(1.000.000.000/8) = 34.3 seconds to wrap. */
    for (i = 0; i < 30; i++) {
        idx = sdx_info->poll_idx;
        isdx = &sdx_info->isdx.table[idx];
        esdx = &sdx_info->esdx.table[idx];
        idx++;
        sdx_info->poll_idx = (idx < sdx_info->max_count ? idx : 0);
        VTSS_RC(srvl_sdx_counters_update(isdx, esdx, NULL, 0));
    }
    return VTSS_RC_OK;
}

static void srvl_debug_sdx_cnt(const vtss_debug_printf_t pr, const char *name,  
                               vtss_chip_counter_pair_t *c1, vtss_chip_counter_pair_t *c2)
{
    char buf[80];
    int  bytes;

    for (bytes = 0; bytes < 2; bytes++) {
        sprintf(buf, "%s_%s", name, bytes ? "bytes" : "frames");
        srvl_debug_cnt(pr, buf, c2 == NULL ? NULL : "", 
                       bytes ? &c1->bytes : &c1->frames, 
                       c2 == NULL ? NULL : (bytes ? &c2->bytes : &c2->frames));
    }
}

static vtss_rc srvl_debug_isdx_list(const vtss_debug_printf_t pr, vtss_sdx_entry_t *isdx_list, 
                                    u32 id, BOOL *header, BOOL ece)
{
    vtss_sdx_entry_t *isdx;
    u32              value;

    for (isdx = isdx_list; isdx != NULL; isdx = isdx->next) {
        if (*header) {
            pr("ISDX  %sCE ID  Port  FORCE_ENA  ES0_ENA  SDLBI  VOE  ", ece ? "E" : "M");
            srvl_debug_print_port_header(pr, "");
            *header = 0;
        }
        SRVL_WR(VTSS_ANA_ANA_TABLES_ISDXTIDX, 
                VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_INDEX(isdx->sdx));
        SRVL_WR(VTSS_ANA_ANA_TABLES_ISDXACCESS, 
                VTSS_F_ANA_ANA_TABLES_ISDXACCESS_ISDX_TBL_CMD(ISDX_CMD_READ));
        VTSS_RC(srvl_isdx_table_idle());
        SRVL_RD(VTSS_ANA_ANA_TABLES_ISDXTIDX, &value);
        pr("%-6u%-8u", isdx->sdx, id);
        if (isdx->port_no < vtss_state->port_count)
            pr("%-6u", VTSS_CHIP_PORT(isdx->port_no));
        else
            pr("%-6s", isdx->port_no == VTSS_PORT_NO_CPU ? "CPU" : "?");
        pr("%-11u%-9u%-7u",
           value & VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_FORCE_ENA ? 1 : 0,
           value & VTSS_F_ANA_ANA_TABLES_ISDXTIDX_ISDX_ES0_KEY_ENA ? 1 : 0,
           VTSS_X_ANA_ANA_TABLES_ISDXTIDX_ISDX_SDLBI(value));
        
        SRVL_RD(VTSS_ANA_IPT_OAM_MEP_CFG(isdx->sdx), &value);
        if (value & VTSS_F_ANA_IPT_OAM_MEP_CFG_MEP_IDX_ENA)
            pr("%-5u", VTSS_X_ANA_IPT_OAM_MEP_CFG_MEP_IDX(value));
        else
            pr("None ");
        
        SRVL_RD(VTSS_ANA_ANA_TABLES_ISDXACCESS, &value);
        srvl_debug_print_mask(pr, VTSS_X_ANA_ANA_TABLES_ISDXACCESS_ISDX_PORT_MASK(value));
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_sdx_counters(const vtss_debug_printf_t pr, 
                                       vtss_sdx_entry_t *isdx_list, vtss_sdx_entry_t *esdx_list,
                                       u32 id, BOOL ece)
{
    /*lint --e{454, 455} */ // Due to VTSS_EXIT_ENTER
    vtss_sdx_entry_t    *isdx, *esdx;
    vtss_sdx_counters_t *icnt, *ecnt, zero_cnt;
    vtss_port_no_t      port_no;

    memset(&zero_cnt, 0, sizeof(zero_cnt));
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        icnt = &zero_cnt;
        ecnt = &zero_cnt;
        for (isdx = isdx_list; isdx != NULL; isdx = isdx->next) {
            if (isdx->port_no == port_no) {
                icnt = &vtss_state->sdx_info.sdx_table[isdx->sdx];
                break;
            }
        }
        for (esdx = esdx_list; esdx != NULL; esdx = esdx->next) {
            if (esdx->port_no == port_no) {
                ecnt = &vtss_state->sdx_info.sdx_table[esdx->sdx];
                break;
            }
        }
        if (isdx == NULL && esdx == NULL)
            continue;
        VTSS_RC(srvl_sdx_counters_update(isdx, esdx, NULL, 0));
        pr("Counters for %sCE ID %u, port %u (chip_port: %u), ISDX: %u, ESDX: %u\n", 
           ece ? "E" : "M", id, port_no, VTSS_CHIP_PORT(port_no), 
           isdx == NULL ? 0: isdx->sdx, esdx == NULL ? 0: esdx->sdx);
        srvl_debug_sdx_cnt(pr, "green", &icnt->rx_green, &ecnt->tx_green);
        srvl_debug_sdx_cnt(pr, "yellow", &icnt->rx_yellow, &ecnt->tx_yellow);
        srvl_debug_sdx_cnt(pr, "red", &icnt->rx_red, NULL);
        srvl_debug_sdx_cnt(pr, "drop_green", &icnt->rx_discard, NULL);
        srvl_debug_sdx_cnt(pr, "drop_yellow", &icnt->tx_discard, NULL);
        pr("\n");
        VTSS_EXIT_ENTER();
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_evc(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */ // Due to VTSS_EXIT_ENTER
    vtss_ece_entry_t *ece;
    vtss_mce_entry_t *mce;
    BOOL             header = 1;

    VTSS_RC(srvl_debug_range_checkers(pr, info));
    VTSS_RC(srvl_debug_is1_all(pr, info));
    VTSS_RC(srvl_debug_es0_all(pr, info));

    /* ISDX table - ECEs */
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        VTSS_RC(srvl_debug_isdx_list(pr, ece->isdx_list, ece->ece_id, &header, 1));
    }
    if (!header)
        pr("\n");
    header = 1;

    /* ISDX table - MCEs */
    for (mce = vtss_state->mce_info.used; mce != NULL; mce = mce->next) {
        VTSS_RC(srvl_debug_isdx_list(pr, mce->isdx_list, mce->conf.id, &header, 0));
    }
    if (!header)
        pr("\n");

    /* SDX counters - ECEs */
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        VTSS_RC(srvl_debug_sdx_counters(pr, ece->isdx_list, ece->esdx_list, ece->ece_id, 1));
    }

    /* SDX counters - MCEs */
    for (mce = vtss_state->mce_info.used; mce != NULL; mce = mce->next) {
        VTSS_RC(srvl_debug_sdx_counters(pr, mce->isdx_list, mce->esdx_list, mce->conf.id, 0));
    }

    return VTSS_RC_OK;
}
#endif  /* VTSS_FEATURE_EVC */

#if defined(VTSS_FEATURE_TIMESTAMP)
/* ================================================================= *
 *  Time stamping
 * ================================================================= */

#define HW_NS_PR_SEC 1000000000L
//#define HW_CLK_CNT_PR_SEC 250000000L
#define HW_CLK_CNT_PR_SEC 156250000L

#define HW_CLK_50_MS (HW_NS_PR_SEC/20)
#define HW_CLK_M950_MS (-HW_NS_PR_SEC + HW_CLK_50_MS)
#define EXT_SYNC_INPUT_LATCH_LATENCY 5 //(1*HW_NS_PR_SEC/HW_CLK_CNT_PR_SEC)  /* 1 clock cycle added to EXT_SYNC_CURRENT TIME */
#define ADJ_UNITS_PR_NS 10
/* 5 clock cycle compensation in ingress and egress latency compenstation, because the timestamped clock is 
 * 5 cycles behind the master timer */
#define TIMESTAMPER_SKEW 32

static vtss_rc subTimestamp(i32 *r, const vtss_timestamp_t *x, const vtss_timestamp_t *y)
{
    i64 temp;
	temp = (x->sec_msb - y->sec_msb)*0x100000000LL;
	temp += x->seconds - y->seconds;
    if (temp < -1 || temp > 1) return VTSS_RC_ERROR;
    *r = temp*VTSS_ONE_MIA;
	*r += x->nanoseconds - y->nanoseconds;
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_timeofday_read(vtss_timestamp_t *ts)
{
    vtss_timestamp_t r1, r2;
    u32 value;
    /* read timestamp registers twice, and verify that no wrapping occurred between two register reads.
       return the value that did not wrap */
    SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_NSEC, &r1.nanoseconds);
    SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, &r1.seconds);
    SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_MSB, &value);
    r1.sec_msb = VTSS_X_SYS_PTP_TOD_PTP_TOD_MSB_PTP_TOD_MSB(value);

    SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_NSEC, &r2.nanoseconds);
    if (r1.nanoseconds < r2.nanoseconds) {
        *ts = r1; /* nothing wrapped */
    } else {
        SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, &r2.seconds);
        SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_MSB, &value);
        r2.sec_msb = VTSS_X_SYS_PTP_TOD_PTP_TOD_MSB_PTP_TOD_MSB(value);
        *ts = r2; /* nanoseconds wrapped and sec incremented*/
    }
    VTSS_D("ts->sec_msb: %u, ts->seconds: %u, ts->nanoseconds: %u, delta ns = %u", ts->sec_msb, ts->seconds, ts->nanoseconds, r2.nanoseconds - r1.nanoseconds);
    /* ns value may be >= 1 sec */
    if (ts->nanoseconds >= VTSS_ONE_MIA) {
        VTSS_D("ns >= one mia: ts->sec_msb: %u, ts->seconds: %u, ts->nanoseconds: %u", ts->sec_msb, ts->seconds, ts->nanoseconds);
        ts->nanoseconds -= VTSS_ONE_MIA;
        if (++ts->seconds == 0) ++ts->sec_msb;
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_timeofday_get(vtss_timestamp_t *ts,
                                     u32              *tc)
{
    i32 x;
    VTSS_RC(srvl_ts_timeofday_read(ts));
    /* tc is returned as 32 bit value, i.e. (seconds*10**9 + nanoseconds) mod 2*32 */
    /* Any value in sec_msb will not change this value, therefore it is omitted */
    *tc = ts->nanoseconds + ts->seconds*VTSS_ONE_MIA;
    x = ts->nanoseconds;
    /* compensate for outstanding adjustment */
    if (vtss_state->ts_conf.awaiting_adjustment) {
        x = ts->nanoseconds - vtss_state->ts_conf.outstanding_corr;
        if (x < 0) {
            ts->seconds--;
            x += HW_NS_PR_SEC;
        } else if (x >= HW_NS_PR_SEC) {
            ts->seconds++;
            x -= HW_NS_PR_SEC;
        }
        ts->nanoseconds = x;
    }
    VTSS_D("ts->seconds: %u, ts->nanoseconds: %u", ts->seconds, ts->nanoseconds);
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_timeofday_next_pps_get(vtss_timestamp_t               *ts)
{
    VTSS_RC(srvl_ts_timeofday_read(ts));
    ++ts->seconds; /* sec counter is maintained in SW */
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
 * In serval the ts_conf.sec_offset is only used for time adjustment. i.e if sec_offset != 0 
 *   then the sec counter is adjusted in the next onesec call: 
 *     1 => sec counter is incremented;
 *     2 => sec counter is decremented.
 */
static vtss_rc srvl_ts_timeofday_offset_set(i32 offset)
{
    if (vtss_state->ts_conf.awaiting_adjustment) {
        return VTSS_RC_ERROR;
     }
    vtss_state->ts_conf.awaiting_adjustment = TRUE;
    vtss_state->ts_conf.outstanding_adjustment = 0;

    if (offset > HW_CLK_50_MS) {
        vtss_state->ts_conf.sec_offset = 2; 
        vtss_state->ts_conf.outstanding_adjustment = offset;
        vtss_state->ts_conf.outstanding_corr = offset - HW_NS_PR_SEC;
    } else if (offset < HW_CLK_M950_MS) {
        vtss_state->ts_conf.sec_offset = 1;
        vtss_state->ts_conf.outstanding_adjustment = offset + 2*HW_NS_PR_SEC;
        vtss_state->ts_conf.outstanding_corr = offset + HW_NS_PR_SEC;
    } else {
        vtss_state->ts_conf.sec_offset = 0;
        vtss_state->ts_conf.outstanding_corr = offset;
        vtss_state->ts_conf.outstanding_adjustment = offset + HW_NS_PR_SEC;
    }
    /* we wait until TOD_NANOSECS wraps */
    VTSS_D("adjust = %u, offset: %d", vtss_state->ts_conf.outstanding_adjustment, vtss_state->ts_conf.outstanding_corr);
    return VTSS_RC_OK;
}
/*
 * When the time is set, the HW sec  counters are written. The HW clock is adjusted so that the timestamp nanoseconds
 * and the hw nanoseconds counters are aligned. This cannot be done with nanosecond precision, so the final alignment
 * is done by the clock rate adjustment. The hw nanoseconds counter is aligned by setting the
 * PTP_UPPER_LIMIT_1_TIME_ADJ. This register sets the next onesec period to [0.. 1.073.741.823] nanosec. 
 * A value of 0 makes no sense as this means two clock pulses with no time distance, therefore the algorithm sets the
 * period in the range [0,050..1,050] Sec.
 */
static vtss_rc srvl_ts_timeofday_set(const vtss_timestamp_t *ts)
{
    i32 corr;
    vtss_timestamp_t ts_hw;
    if (vtss_state->ts_conf.awaiting_adjustment) {
        return VTSS_RC_ERROR;
    }
    SRVL_WR(VTSS_SYS_PTP_TOD_PTP_TOD_MSB, VTSS_F_SYS_PTP_TOD_PTP_TOD_MSB_PTP_TOD_MSB(ts->sec_msb));
    SRVL_WR(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, ts->seconds);
    VTSS_RC(srvl_ts_timeofday_read(&ts_hw));
    VTSS_RC(subTimestamp(&corr, &ts_hw, ts));
    VTSS_RC(srvl_ts_timeofday_offset_set(corr));
    VTSS_D("ts->sec_msb: %u, ts->seconds: %u, ts->nanoseconds: %u", ts->sec_msb, ts->seconds, ts->nanoseconds);
    return VTSS_RC_OK;
}

/*
 * When the time is set, the HW sec  counters are written. The HW clock is adjusted so that the timestamp nanoseconds
 * and the hw nanoseconds counters are aligned. This cannot be done with nanosecond precision, so the final alignment
 * is done by the clock rate adjustment. The hw nanoseconds counter is aligned by setting the
 * PTP_UPPER_LIMIT_1_TIME_ADJ. This register sets the next onesec period to [0.. 1.073.741.823] nanosec. 
 * A value of 0 makes no sense as this means two clock pulses with no time distance, therefore the algorithm sets the
 * period in the range [0,050..1,050] Sec.
 */
static vtss_rc srvl_ts_timeofday_set_delta(const vtss_timestamp_t *ts, BOOL negative)
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
        VTSS_RC(srvl_ts_timeofday_offset_set(corr));
    }
    vtss_state->ts_conf.delta_sec = ts->seconds;
    vtss_state->ts_conf.delta_sec_msb = ts->sec_msb;
    vtss_state->ts_conf.delta_sign = negative ? 1 : 2;
    
    VTSS_D("ts->sec_msb: %u, ts->seconds: %u, ts->nanoseconds: %u", ts->sec_msb, ts->seconds, ts->nanoseconds);
    return VTSS_RC_OK;
}

/*
 * This function is called from interrupt, therefore the macros using vtss_state cannot be used.
 */
static u32 srvl_ts_ns_cnt_get(vtss_inst_t inst)
{
    /* this code is a copy of the srvl_ts_timeofday_read(ts) function , but without using the MACROS */
    vtss_timestamp_t r1, r2, ts;
    u32 value;
    /* read timestamp registers twice, and verify that no wrapping occurred between two register reads.
       return the value that did not wrap */
    (void)inst->init_conf.reg_read(0, VTSS_DEVCPU_GCB_PTP_STAT_PTP_CURRENT_TIME_STAT, &value);

    (void)inst->init_conf.reg_read(0, VTSS_SYS_PTP_TOD_PTP_TOD_NSEC, &r1.nanoseconds);
    (void)inst->init_conf.reg_read(0, VTSS_SYS_PTP_TOD_PTP_TOD_LSB, &r1.seconds);
    (void)inst->init_conf.reg_read(0, VTSS_SYS_PTP_TOD_PTP_TOD_MSB, &value);
    r1.sec_msb = VTSS_X_SYS_PTP_TOD_PTP_TOD_MSB_PTP_TOD_MSB(value);
    
    (void)inst->init_conf.reg_read(0, VTSS_SYS_PTP_TOD_PTP_TOD_NSEC, &r2.nanoseconds);
    if (r1.nanoseconds < r2.nanoseconds) {
        ts = r1; /* nothing wrapped */
    } else {
        (void)inst->init_conf.reg_read(0, VTSS_SYS_PTP_TOD_PTP_TOD_LSB, &r2.seconds);
        (void)inst->init_conf.reg_read(0, VTSS_SYS_PTP_TOD_PTP_TOD_MSB, &value);
        r2.sec_msb = VTSS_X_SYS_PTP_TOD_PTP_TOD_MSB_PTP_TOD_MSB(value);
        ts = r2; /* nanoseconds wrapped and sec incremented*/
    }
    /* ns value may be >= 1 sec */
    if (ts.nanoseconds >= VTSS_ONE_MIA) {
        ts.nanoseconds -= VTSS_ONE_MIA;
        if (++ts.seconds == 0) ++ts.sec_msb;
    }
    /* tc is returned as 32 bit value, i.e. (seconds*10**9 + nanoseconds) mod 2*32 */
    /* Any value in sec_msb will not change this value, therefore it is omitted */
    value = ts.nanoseconds + ts.seconds*VTSS_ONE_MIA ;
    return value;
}

static vtss_rc srvl_ts_timeofday_one_sec(void)
{
    u32 value, result, value_msb;
    i32 delta_msb = 0;
    u32 reg;
    /* update Delta Sec counter */
    if (vtss_state->ts_conf.delta_sign != 0) {
        SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, &value);
        SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_MSB, &value_msb);
        
        if (vtss_state->ts_conf.delta_sign == 1) {
            if (value < vtss_state->ts_conf.delta_sec) {
                value_msb--;
            }
            result = value - vtss_state->ts_conf.delta_sec;
            value_msb -= vtss_state->ts_conf.delta_sec_msb;
        } else {
            result = value + vtss_state->ts_conf.delta_sec;
            if (result < value) {
                value_msb++;
            }
            value_msb += vtss_state->ts_conf.delta_sec_msb;
        }
        SRVL_WR(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, result);
        SRVL_WR(VTSS_SYS_PTP_TOD_PTP_TOD_MSB, value_msb);
        VTSS_D("delta sec %d, sign %d", vtss_state->ts_conf.delta_sec, vtss_state->ts_conf.delta_sign);
        
        vtss_state->ts_conf.delta_sec = 0;
        vtss_state->ts_conf.delta_sec_msb = 0;
        vtss_state->ts_conf.delta_sign = 0;
    }
    if (vtss_state->ts_conf.awaiting_adjustment) {
        /* For debug: read the internal NS counter */
        SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_NSEC, &value);
        VTSS_D("TOD_NANOSECS %u", value);

        if (vtss_state->ts_conf.outstanding_adjustment != 0) {
            if (value > vtss_state->ts_conf.outstanding_adjustment) {
                VTSS_E("Too large interrupt latency to adjust %u (ns)", value);
            } else {
                /* update Sec counter */
                if (vtss_state->ts_conf.sec_offset != 0) {
                    SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, &value);
                    if (vtss_state->ts_conf.sec_offset == 2) {
                        if (value == 0) {
                            delta_msb = -1;
                        }
                        --value;
                    }
                    if (vtss_state->ts_conf.sec_offset == 1) {
                        ++value;
                        if (value == 0) {
                            delta_msb = 1;
                        }
                    }
                    SRVL_WR(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, value);
                    if (delta_msb != 0) {
                        SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_MSB, &value);
                        value += delta_msb;
                        SRVL_WR(VTSS_SYS_PTP_TOD_PTP_TOD_MSB, value);
                    }
                    vtss_state->ts_conf.sec_offset = 0;
                }
                /*PTP_UPPER_LIMIT_1_TIME_ADJ = x */
                SRVL_WR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG, 
                        vtss_state->ts_conf.outstanding_adjustment |
                        VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_SHOT);
                VTSS_D("onetime adjustment done %u", vtss_state->ts_conf.outstanding_adjustment);
                SRVL_RD(VTSS_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG,&reg);
                VTSS_D("1_TIME_ADJ_CFG reg %x", reg);
                if (vtss_state->ts_conf.outstanding_adjustment < 0x20000000) {
                    /* sec counter in serval does not increment if ns counter wraps before bit 29 is set */
                    vtss_state->ts_conf.delta_sec = 1;
                    vtss_state->ts_conf.delta_sign = 2;
                }
                vtss_state->ts_conf.outstanding_adjustment = 0;

            }
        } else {
            vtss_state->ts_conf.awaiting_adjustment = FALSE;
            VTSS_D("awaiting_adjustment cleared");
        }
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_adjtimer_set(void)
{
    /* adj in units of 0,1 pbb */
    i32 adj = vtss_state->ts_conf.adj;
    u32 clk_adj = HW_CLK_CNT_PR_SEC+1;
    u32 clk_adj_cfg = 0;
    i64 divisor = 0;
    
    /* CLK_ADJ = clock_rate/|adj|
    * CLK_ADJ_DIR = adj>0 ? Positive : Negative
    */
    if (adj != 0) {
        divisor =  VTSS_DIV64((i64)HW_CLK_CNT_PR_SEC*(i64)ADJ_UNITS_PR_NS, (i64)vtss_state->ts_conf.adj);
        if (divisor > (i64)HW_CLK_CNT_PR_SEC || divisor < -(i64)HW_CLK_CNT_PR_SEC) divisor = (i64)HW_CLK_CNT_PR_SEC+1;
        clk_adj = VTSS_LABS((i32)divisor);
    }
    vtss_state->ts_conf.adj_divisor = (i32)divisor;

    if ((adj != 0)) {
        clk_adj_cfg |= VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_ENA;
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(0), VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ADJ_ENA);
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(1), VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ADJ_ENA);
    } else {
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(0), VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ADJ_ENA);
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(1), VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ADJ_ENA);
    }
    if (adj < 0) {
        clk_adj_cfg |= VTSS_F_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG_CLK_ADJ_DIR;
    } 
    SRVL_WR(VTSS_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG, clk_adj_cfg);
    SRVL_WR(VTSS_DEVCPU_GCB_PTP_CFG_CLK_ADJ_FRQ, clk_adj);
    
    VTSS_D("adj: %d, CLK_ACJ_CFG: %x", adj, clk_adj);
    return VTSS_RC_OK;
}

/* This function assumes that instance 0 of the EXT_SYNC_CURRENT_TIME_STAT is used*/
static vtss_rc srvl_ts_freq_offset_get(i32 *const adj)
{
    i32 offset; /* frequency offset in clock counts pr sec */
    u32 cu;
    u32 lv;
    SRVL_RD(VTSS_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT(0),&cu);
    cu = VTSS_X_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT_EXT_SYNC_CURRENT_TIME(cu);
    /* the saved value is 3 clock cycles after the l/s, and the load value seems to be 4 cycles delayed, 
       i.e. the saved value is expected to be load value + 10^9 + a clock cycle */
    cu += EXT_SYNC_INPUT_LATCH_LATENCY;
    /* compensate for load value. i.e. the ns counter is expected to run from 'lv' to 10^9+'lv' - 1clock cycle*/
    SRVL_RD(VTSS_DEVCPU_GCB_PTP_CFG_PTP_EXT_SYNC_TIME_CFG(0), &lv);
    cu -= lv;
    offset = (i32)(cu) - (i32)HW_NS_PR_SEC + 1; /* frequency offset in ppb */
    /* convert to units of 0,1 ppb */
    VTSS_D("cu: %u adj: %d", cu, *adj);
    *adj = ADJ_UNITS_PR_NS*offset;
    VTSS_D("cu: %u adj: %d, lv: %u", cu, *adj, lv);
    return VTSS_RC_OK;
}


/* Set the clock mode for either idx 0 (GPIO pin 30,31) or idx 1 (GPIO pin 15,16) */
/* idx 0 = the alternative clock instance (PPS0), on the Ref board called RS422_1PPS
 * idx 1 = the default clock instance (PPS1), on the Ref board called SyncE1PPS, and also connected to the Tesla PHY's
 */
static vtss_rc srvl_ts_external_clock_mode_set(int idx)
{
    u32 dividers;
    u32 high_div;
    u32 low_div;
    u32 cfg_bit_field, ri;
    u16 gpio_ld_st, gpio_ext_clk_out;
    vtss_ts_ext_clock_mode_t *ext_clock_mode;
    vtss_gpio_mode_t gpio_alternate_mode;
    if (idx < 0 || idx > 1) {
        VTSS_E("Invalid external clock index: %d", idx);
        return VTSS_RC_ERROR;
    }
    if (idx == 0) {
        ext_clock_mode = &vtss_state->ts_conf.ext_clock_mode_alt;
        cfg_bit_field = 0x1;
        ri = 0;
        gpio_ld_st = 30;
        gpio_ext_clk_out = 31;
        gpio_alternate_mode = VTSS_GPIO_ALT_0;
    } else {
        ext_clock_mode = &vtss_state->ts_conf.ext_clock_mode;
        cfg_bit_field = 0x2;
        ri = 1;
        gpio_ld_st = 15;
        gpio_ext_clk_out = 16;
        gpio_alternate_mode = VTSS_GPIO_ALT_1;
    }
    if (!ext_clock_mode->enable && 
            (ext_clock_mode->one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_DISABLE)) {
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                    VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA(cfg_bit_field));
        /* disable clock output, disable 1PPS */
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(ri), 
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_SYNC_ENA);
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_CAP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
        /* both GPIO pins are unused: disable alternate  for both pins. */
        (void) srvl_gpio_mode(0, gpio_ld_st, VTSS_GPIO_IN);
        (void) srvl_gpio_mode(0, gpio_ext_clk_out, VTSS_GPIO_IN);
    } else if (ext_clock_mode->enable && 
                (ext_clock_mode->one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_DISABLE)) {
        /* enable clock output, disable 1PPS */
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(ri), 
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_SYNC_ENA);
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA(cfg_bit_field));
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_CAP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
        (void) srvl_gpio_mode(0, gpio_ld_st, VTSS_GPIO_IN);
        (void) srvl_gpio_mode(0, gpio_ext_clk_out, gpio_alternate_mode);
    } else if (ext_clock_mode->one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_OUTPUT) {
        /* disable clock output, 1 pps output enabled */
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(ri), 
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_SYNC_ENA);
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA(cfg_bit_field));
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_CAP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
        /* enable alternate mode for clock out. */
        (void) srvl_gpio_mode(0, gpio_ld_st, VTSS_GPIO_IN);
        (void) srvl_gpio_mode(0, gpio_ext_clk_out, gpio_alternate_mode);
    } else if (!ext_clock_mode->enable && 
               (ext_clock_mode->one_pps_mode == TS_EXT_CLOCK_MODE_ONE_PPS_INPUT)) {
        /* disable clock output, 1 pps input enabled */
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(ri), 
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_SYNC_ENA);
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_CAP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA(cfg_bit_field));
        /* enable alternate mode for load/store */
        (void) srvl_gpio_mode(0, gpio_ld_st, gpio_alternate_mode);
        (void) srvl_gpio_mode(0, gpio_ext_clk_out, VTSS_GPIO_IN);
    } else if (ext_clock_mode->enable && 
                (ext_clock_mode->one_pps_mode != TS_EXT_CLOCK_MODE_ONE_PPS_INPUT)) {
        /* enable clock output, 1 pps input enabled */
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(ri), 
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_SYNC_ENA);
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_CAP_ENA(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA(cfg_bit_field));
            /* enable alternate mode for clock out. */
        (void) srvl_gpio_mode(0, gpio_ld_st, gpio_alternate_mode);
        (void) srvl_gpio_mode(0, gpio_ext_clk_out, gpio_alternate_mode);
    }
    if (ext_clock_mode->enable) {
        /* set dividers; enable clock output. */
        dividers = (HW_CLK_CNT_PR_SEC/(ext_clock_mode->freq));
        high_div = (dividers/2)-1;
        low_div =  ((dividers+1)/2)-1;
        SRVL_WR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG(ri), high_div  &
               VTSS_M_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG_GEN_EXT_CLK_HIGH_PERIOD);
        SRVL_WR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG(ri), low_div  &
               VTSS_M_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG_GEN_EXT_CLK_LOW_PERIOD);
    }
    return VTSS_RC_OK;
}

/* Get the saved nanosec conuter from counter */
static vtss_rc srvl_ts_alt_clock_saved_get(u32    *const saved)
{
    SRVL_RD(VTSS_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT(0), saved);
    *saved = VTSS_X_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT_EXT_SYNC_CURRENT_TIME(*saved);
    return VTSS_RC_OK;
}

/* Set the clock mode for external clock idx 0 (GPIO pin 30,31) */
/* idx 0 = the alternative clock instance (PPS0), on the Ref board called RS422_1PPS
 * the modes are: 
 *    BOOL one_pps_out;        Enable 1pps output
 *    BOOL one_pps_in;         Enable 1pps input
 *    BOOL save;               Save actual time counter at next 1 PPS input
 *    BOOL load;               Load actual time counter at next 1 PPS input
 * Register setting in PTP_MISC_CFG:
 *                     one_pps_out | one_pps_in |   save   |   load
 *                        T    F   |   T    F   |  T    F  |  T    F  |
 * EXT_SYNC_OUTP_SEL      1    0
 * EXT_SYNC_OUTP_ENA      1    0
 * EXT_SYNC_INP_ENA                    1    0
 * EXT_SYNC_CAP_ENA                                1    0
 * EXT_SYNC_ENA                                               1    0
 */
static vtss_rc srvl_ts_alt_clock_mode_set(void)
{
    u32 cfg_bit_field, ri;
    u16 gpio_ld_st, gpio_ext_clk_out;
    vtss_ts_alt_clock_mode_t       *alt_clock_mode;
    vtss_gpio_mode_t gpio_alternate_mode;
    
    alt_clock_mode = &vtss_state->ts_conf.alt_clock_mode;
    cfg_bit_field = 0x1;
    ri = 0;
    gpio_ld_st = 30;
    gpio_ext_clk_out = 31;
    gpio_alternate_mode = VTSS_GPIO_ALT_0;
    
    SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(ri), 
                 VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_ENA |
                 VTSS_F_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG_GEN_EXT_CLK_SYNC_ENA);
    if (alt_clock_mode->one_pps_out) {
        /*  1 pps output enabled */
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA(cfg_bit_field));
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
        /* enable alternate mode for clock out. */
        (void) srvl_gpio_mode(0, gpio_ext_clk_out, gpio_alternate_mode);
    } else {
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_SEL(cfg_bit_field) |
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_OUTP_ENA(cfg_bit_field));
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
        /* disable alternate mode for clock out. */
        (void) srvl_gpio_mode(0, gpio_ext_clk_out, VTSS_GPIO_IN);
    }
        
    if (alt_clock_mode->one_pps_in) {
        /* L/S input enabled */
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA(cfg_bit_field));
        /* enable alternate mode for load/store */
        (void) srvl_gpio_mode(0, gpio_ld_st, gpio_alternate_mode);
    } else {
        /* disable clock L/S input */
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_INP_ENA(cfg_bit_field));
        /* enable alternate mode for clock out. */
        (void) srvl_gpio_mode(0, gpio_ld_st, VTSS_GPIO_IN);
    }
    if (alt_clock_mode->save) {
        /* save actual time at L/S input */
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_CAP_ENA(cfg_bit_field));
    } else {
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_CAP_ENA(cfg_bit_field));
    }
    if (alt_clock_mode->load) {
        /*  load enabled */
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
    } else {
        SRVL_WRM_CLR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG,
                     VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_EXT_SYNC_ENA(cfg_bit_field));
    }
    return VTSS_RC_OK;
}

/* Set the time to be loaded into the Serval timer at the next 1PPS
 * It is assumed that this function is called at the beginning of a sec */
static vtss_rc srvl_ts_timeofday_next_pps_set(const vtss_timestamp_t         *const ts)
{
    /* warn that the time is being set, awaiting_adjustment is cleared at the next 1PPS */
    vtss_state->ts_conf.awaiting_adjustment = TRUE;
    vtss_state->ts_conf.outstanding_adjustment = 0;
    SRVL_WR(VTSS_SYS_PTP_TOD_PTP_TOD_MSB, VTSS_F_SYS_PTP_TOD_PTP_TOD_MSB_PTP_TOD_MSB(ts->sec_msb));
    SRVL_WR(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, ts->seconds-1);
    SRVL_WR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_EXT_SYNC_TIME_CFG(0), ts->nanoseconds);
    VTSS_D("time at next pps: sec_msb %u, sec %u, ns %u", ts->sec_msb, ts->seconds, ts->nanoseconds);
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_ingress_latency_set(vtss_port_no_t port_no)
{
    vtss_ts_port_conf_t       *conf = &vtss_state->ts_port_conf[port_no];
    i32 rx_delay = VTSS_INTERVAL_NS(conf->ingress_latency);
    i32 default_ingr_latency = 0; /* TBD what the default ingress latency is */
    rx_delay += -TIMESTAMPER_SKEW + default_ingr_latency;
    if ((rx_delay < -TIMESTAMPER_SKEW) || (rx_delay > 0x7ff)) {
        VTSS_E("Invalid ingress latency %d", rx_delay);
        return VTSS_RC_ERROR;
    }
    /* the RX_PATH_DELAY is added to the rx timestamp, therefore it is -ingress_latency */
    rx_delay = -rx_delay;
    SRVL_WRM(VTSS_SYS_SYSTEM_IO_PATH_DELAY(VTSS_CHIP_PORT(port_no)), 
            VTSS_F_SYS_SYSTEM_IO_PATH_DELAY_RX_PATH_DELAY(rx_delay), 
            VTSS_M_SYS_SYSTEM_IO_PATH_DELAY_RX_PATH_DELAY);
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_p2p_delay_set(vtss_port_no_t port_no)
{
    vtss_ts_port_conf_t       *conf = &vtss_state->ts_port_conf[port_no];
    i32 p2p_delay = VTSS_INTERVAL_NS(conf->p2p_delay);
    if (p2p_delay < 0) {
        VTSS_E("Invalid p2p delay %d", p2p_delay);
        return VTSS_RC_ERROR;
    }
    p2p_delay += VTSS_INTERVAL_NS(conf->delay_asymmetry);
    SRVL_WR(VTSS_ANA_PORT_PTP_DLY2_CFG(VTSS_CHIP_PORT(port_no)), 
             p2p_delay);
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_egress_latency_set(vtss_port_no_t port_no)
{
    vtss_ts_port_conf_t       *conf = &vtss_state->ts_port_conf[port_no];
    i32 tx_delay = VTSS_INTERVAL_NS(conf->egress_latency);
    i32 default_egr_latency = 0; /* TBD what the default egress latency is */
    tx_delay += TIMESTAMPER_SKEW + default_egr_latency;
    if ((tx_delay < 0) || (tx_delay > 0x7ff)) {
        VTSS_E("Invalid egress latency %d", tx_delay);
        return VTSS_RC_ERROR;
    }
    SRVL_WRM(VTSS_SYS_SYSTEM_IO_PATH_DELAY(VTSS_CHIP_PORT(port_no)), 
             VTSS_F_SYS_SYSTEM_IO_PATH_DELAY_TX_PATH_DELAY(tx_delay), 
             VTSS_M_SYS_SYSTEM_IO_PATH_DELAY_TX_PATH_DELAY);
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_delay_asymmetry_set(vtss_port_no_t port_no)
{
    vtss_ts_port_conf_t       *conf = &vtss_state->ts_port_conf[port_no];
    i32 delay = VTSS_INTERVAL_NS(conf->delay_asymmetry);
    /* Used for Ingress asymmetry compensation (Pdelay_Resp) */
    SRVL_WR(VTSS_ANA_PORT_PTP_DLY1_CFG(VTSS_CHIP_PORT(port_no)), 
            delay);
    /* Used for Egress asymmetry compensation (on Delay_Req and Pdelay_Req) */
    SRVL_WR(VTSS_REW_PORT_PTP_DLY1_CFG(VTSS_CHIP_PORT(port_no)), 
            -delay);
    /* Used for Ingress asymmetry compensation (on Sync, therefore include p2p_delay ) */
    delay += VTSS_INTERVAL_NS(conf->p2p_delay);
    SRVL_WR(VTSS_ANA_PORT_PTP_DLY2_CFG(VTSS_CHIP_PORT(port_no)), 
            delay);
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_operation_mode_set(vtss_port_no_t port_no)
{
    vtss_ts_mode_t mode = vtss_state->ts_port_conf[port_no].mode.mode;
    u32 ana_mode = 0;
    u32 rew_mode = 0;
    if (mode == TS_MODE_INTERNAL) {
        ana_mode = VTSS_F_ANA_PORT_PTP_CFG_PTP_BACKPLANE_MODE;
        rew_mode = VTSS_F_REW_PORT_PTP_CFG_PTP_BACKPLANE_MODE;
    }
    SRVL_WRM(VTSS_ANA_PORT_PTP_CFG(VTSS_CHIP_PORT(port_no)),
             ana_mode,
             VTSS_F_ANA_PORT_PTP_CFG_PTP_BACKPLANE_MODE);
    SRVL_WRM(VTSS_REW_PORT_PTP_CFG(VTSS_CHIP_PORT(port_no)), 
             rew_mode, 
             VTSS_F_REW_PORT_PTP_CFG_PTP_BACKPLANE_MODE);
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_internal_mode_set(void)
{
    vtss_ts_internal_fmt_t fmt = vtss_state->ts_int_mode.int_fmt;
    u32 stamp_wid = 30;
    u32 roll_mode = 0;
    switch (fmt) {
        case TS_INTERNAL_FMT_RESERVED_LEN_30BIT:
            stamp_wid = 30;
            roll_mode = 0;
            break;
        case TS_INTERNAL_FMT_RESERVED_LEN_32BIT:
            stamp_wid = 32;
            roll_mode = 0;
            break;
        case TS_INTERNAL_FMT_SUB_ADD_LEN_44BIT_CF62:
            stamp_wid = 44;
            roll_mode = 2;
            break;
        case TS_INTERNAL_FMT_RESERVED_LEN_48BIT_CF_3_0:
            stamp_wid = 48;
            roll_mode = 0;
            break;
        case TS_INTERNAL_FMT_RESERVED_LEN_48BIT_CF_0:
            stamp_wid = 48;
            roll_mode = 1;
            break;
        default:
            break;
    }
    VTSS_D("timestamp_width %d, roll_mode %d",stamp_wid, roll_mode);
    if (stamp_wid >=44) {
        vtss_state->ts_add_sub_option = TRUE;
    } else {
        vtss_state->ts_add_sub_option = FALSE;
    }
    SRVL_WRM(VTSS_SYS_PTP_PTP_CFG, 
             VTSS_F_SYS_PTP_PTP_CFG_PTP_STAMP_WID(stamp_wid) | VTSS_F_SYS_PTP_PTP_CFG_PTP_CF_ROLL_MODE(roll_mode), 
             VTSS_M_SYS_PTP_PTP_CFG_PTP_STAMP_WID | VTSS_M_SYS_PTP_PTP_CFG_PTP_CF_ROLL_MODE);
    return VTSS_RC_OK;
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


static vtss_rc srvl_ts_timestamp_get(void)
{
    u32 value;
    u32 delay;
    u32 tx_port;
    u32 mess_id;
    u32 seq_no;
    u32 sec;
    BOOL overflow = FALSE;
    u32 ts_sec;

    /* PTP_STATUS */
    SRVL_RD(VTSS_SYS_PTP_PTP_STATUS, &value);
    while (value & VTSS_F_SYS_PTP_PTP_STATUS_PTP_MESS_VLD) {
        overflow |= (value & VTSS_F_SYS_PTP_PTP_STATUS_PTP_OVFL) ? TRUE : FALSE;
        SRVL_RD(VTSS_SYS_PTP_PTP_TXSTAMP, &delay);
        ts_sec = (delay & VTSS_F_SYS_PTP_PTP_TXSTAMP_PTP_TXSTAMP_SEC) ? 1 : 0;
        delay = VTSS_X_SYS_PTP_PTP_TXSTAMP_PTP_TXSTAMP(delay); /* extract nanoseconds */
        /* Convert the tx timestamp to 32 bit value to make it comparable to the rx timestamp */
        SRVL_RD(VTSS_SYS_PTP_TOD_PTP_TOD_LSB, &sec);
        if ((sec & 0x1) != ts_sec) { // sec has incremented since timestamp was registered
            --sec;
        }
        delay += sec*1000000000L;
        tx_port = api_port(VTSS_X_SYS_PTP_PTP_STATUS_PTP_MESS_TXPORT(value));
        mess_id = VTSS_X_SYS_PTP_PTP_STATUS_PTP_MESS_ID(value);
        seq_no  = VTSS_X_SYS_PTP_PTP_STATUS_PTP_MESS_SEQ_ID(value);
        if (value & VTSS_F_SYS_PTP_PTP_STATUS_PTP_TXSTAMP_OAM) {
            if (mess_id >= VTSS_TS_ID_SIZE) {
                VTSS_E("invalid mess_id (%u)", mess_id);
            } else {
#if defined(VTSS_ARCH_SERVAL_CE)
                VTSS_I("OAM timestamps received:");
                if (++vtss_state->oam_ts_status[mess_id].last >= VTSS_SERVAL_MAX_OAM_ENTRIES) vtss_state->oam_ts_status[mess_id].last = 0;
                vtss_state->oam_ts_status[mess_id].entry[vtss_state->oam_ts_status[mess_id].last].tc = delay;
                vtss_state->oam_ts_status[mess_id].entry[vtss_state->oam_ts_status[mess_id].last].id = mess_id;
                vtss_state->oam_ts_status[mess_id].entry[vtss_state->oam_ts_status[mess_id].last].sq = seq_no;
                vtss_state->oam_ts_status[mess_id].entry[vtss_state->oam_ts_status[mess_id].last].port = tx_port;
                vtss_state->oam_ts_status[mess_id].entry[vtss_state->oam_ts_status[mess_id].last].valid = TRUE;
                VTSS_I(" port %d, id %d, tc %u, sq %u", tx_port, mess_id, delay, seq_no);
#endif /* VTSS_ARCH_SERVAL_CE */
            }
        } else {
            if (mess_id >= VTSS_TS_ID_SIZE) {
                VTSS_E("invalid mess_id (%u)", mess_id);
            } else {
                if (tx_port < VTSS_PORT_ARRAY_SIZE) {
                    vtss_state->ts_status[mess_id].tx_tc[tx_port] = delay;
                    vtss_state->ts_status[mess_id].tx_id[tx_port] = mess_id;
                    vtss_state->ts_status[mess_id].tx_sq[tx_port] = seq_no;
                    vtss_state->ts_status[mess_id].valid_mask |= 1LL<<tx_port;
                } else {
                    VTSS_E("invalid port (%u)", tx_port);
                }
            }
        }
        VTSS_D("value %x, delay %u, tx_port %u, mess_id %u, seq_no %u", value, delay, tx_port, mess_id, seq_no);
        SRVL_WR(VTSS_SYS_PTP_PTP_NXT, VTSS_F_SYS_PTP_PTP_NXT_PTP_NXT);
        SRVL_RD(VTSS_SYS_PTP_PTP_STATUS, &value);
    }
    if (overflow ) {
        VTSS_E("Timestamp fifo overflow occurred");
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_ts_timestamp_id_release(u32 ts_id)
{
    /* Clear bit corresponding to ts_id */
    if (ts_id <TS_IDS_RESERVED_FOR_SW) {
        vtss_state->ts_conf.sw_ts_id_res[ts_id] = 0;
    } else if (ts_id <=31) {
        SRVL_WR(VTSS_ANA_ANA_TABLES_PTP_ID_LOW, VTSS_BIT(ts_id));
    } else if (ts_id <=63) {
        SRVL_WR(VTSS_ANA_ANA_TABLES_PTP_ID_HIGH, VTSS_BIT(ts_id-32));
    } else {
        VTSS_D("invalid ts_id %d", ts_id);
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_ts(const vtss_debug_printf_t pr,
                             const vtss_debug_info_t   *const info)
{
    u32  value;
    u32  port;
    int ix;
    char buf[32];

    /* SYS:PTP_TOD */
    srvl_debug_reg_header(pr, "SYS:PTP_TOD");
    srvl_debug_reg(pr, VTSS_SYS_PTP_TOD_PTP_TOD_MSB, "PTP_TOD_MSB");
    srvl_debug_reg(pr, VTSS_SYS_PTP_TOD_PTP_TOD_LSB, "PTP_TOD_LSB");
    srvl_debug_reg(pr, VTSS_SYS_PTP_TOD_PTP_TOD_NSEC, "PTP_TOD_NSEC");
    /* SYS:PTP */
    srvl_debug_reg_header(pr, "SYS:PTP");
    srvl_debug_reg(pr, VTSS_SYS_PTP_PTP_STATUS, "PTP_STATUS");
    srvl_debug_reg(pr, VTSS_SYS_PTP_PTP_TXSTAMP, "PTP_TXSTAMP");
    srvl_debug_reg(pr, VTSS_SYS_PTP_PTP_NXT, "PTP_NXT");
    srvl_debug_reg(pr, VTSS_SYS_PTP_PTP_CFG, "PTP_CFG");
    
    /* DEVCPU_GCB: PTP_CFG */
    srvl_debug_reg_header(pr, "GCB:PTP_CFG");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, "PTP_MISC_CFG");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_CFG, "PTP_UPPER_LIMIT_CFG");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_UPPER_LIMIT_1_TIME_ADJ_CFG, "PTP_UPPER_LIMIT_1_TIME_ADJ_CFG");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG, "PTP_SYNC_INTR_ENA_CFG");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_CLK_ADJ_CFG, "CLK_ADJ_CFG");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_CLK_ADJ_FRQ, "CLK_ADJ_FRQ");
    for (ix = 0; ix <= 1; ix++) {
        sprintf(buf, "GEN_EXT_CLK_HIGH_PERIOD_CFG_%u", ix);
        srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_HIGH_PERIOD_CFG(ix), buf);
        sprintf(buf, "GEN_EXT_CLK_LOW_PERIOD_CFG_%u", ix);
        srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_LOW_PERIOD_CFG(ix), buf);
        sprintf(buf, "GEN_EXT_CLK_CFG_%u", ix);
        srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_GEN_EXT_CLK_CFG(ix), buf);
        sprintf(buf, "PTP_EXT_SYNC_TIME_CFG_%u", ix);
        srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_CFG_PTP_EXT_SYNC_TIME_CFG(ix), buf);
    }    
    /* DEVCPU_GCB: PTP_STAT */
    srvl_debug_reg_header(pr, "GCB:PTP_STAT");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_STAT_PTP_CURRENT_TIME_STAT, "PTP_CURRENT_TIME_STAT");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT(0), "EXT_SYNC_CURRENT_TIME_STAT_0");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_STAT_EXT_SYNC_CURRENT_TIME_STAT(1), "EXT_SYNC_CURRENT_TIME_STAT_1");
    srvl_debug_reg(pr, VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, "PTP_EVT_STAT");

    SRVL_RD(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, &value);
    SRVL_WR(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, value);
    vtss_debug_print_sticky(pr, "CLK_ADJ_UPD_STICKY", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY);
    vtss_debug_print_sticky(pr, "EXT_SYNC_CURRENT_TIME_STICKY_0", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_EXT_SYNC_CURRENT_TIME_STICKY(0x1));
    vtss_debug_print_sticky(pr, "EXT_SYNC_CURRENT_TIME_STICKY_1", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_EXT_SYNC_CURRENT_TIME_STICKY(0x2));
    vtss_debug_print_sticky(pr, "SYNC_STAT", value, VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_SYNC_STAT);
    
    /* SYS_PORT:PTP */
    srvl_debug_reg_header(pr, "REW:PORT");
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        sprintf(buf, "PTP_CFG_%u", port);
        srvl_debug_reg(pr, VTSS_REW_PORT_PTP_CFG(port), buf);
        sprintf(buf, "PTP_DLY1_CFG_%u", port);
        srvl_debug_reg(pr, VTSS_REW_PORT_PTP_DLY1_CFG(port), buf);
    }
    /* SYS_SWITCH:PTP */
    srvl_debug_reg_header(pr, "SWITCH:PORT");
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        sprintf(buf, "IO_PATH_DELAY_%u", port);
        srvl_debug_reg(pr, VTSS_SYS_SYSTEM_IO_PATH_DELAY(port), buf);
    }
    /* ANA_PORT:PTP */
    srvl_debug_reg_header(pr, "ANA:PORT");
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        sprintf(buf, "PTP_CFG_%u", port);
        srvl_debug_reg(pr, VTSS_ANA_PORT_PTP_CFG(port), buf);
        sprintf(buf, "PTP_DLY1_CFG_%u", port);
        srvl_debug_reg(pr, VTSS_ANA_PORT_PTP_DLY1_CFG(port), buf);
        sprintf(buf, "PTP_DLY2_CFG_%u", port);
        srvl_debug_reg(pr, VTSS_ANA_PORT_PTP_DLY2_CFG(port), buf);
    }

    /* ANA:: */
    srvl_debug_reg_header(pr, "ANA::PTP");
    srvl_debug_reg(pr, VTSS_ANA_ANA_TABLES_PTP_ID_HIGH, "PTP_ID_HIGH");
    srvl_debug_reg(pr, VTSS_ANA_ANA_TABLES_PTP_ID_LOW, "PTP_ID_LOW");
    
    
    pr("\n");

    return VTSS_RC_OK;
}

#endif /* VTSS_FEATURE_TIMESTAMP */

/* ================================================================= *
 *  Packet control
 * ================================================================= */

static u32 srvl_cpu_fwd_mask_get(vtss_packet_reg_type_t type, BOOL redir, u32 i)
{
    u32 mask;

    /* Map NORMAL to CPU_ONLY/FORWARD */
    if (type == VTSS_PACKET_REG_NORMAL) {
        type = (redir ? VTSS_PACKET_REG_CPU_ONLY : VTSS_PACKET_REG_FORWARD); 
    }
    
    if (type == VTSS_PACKET_REG_CPU_ONLY) {
         /* Set REDIR bit */
        mask = VTSS_BIT(i);
    } else if (type == VTSS_PACKET_REG_DISCARD) {
        /* Set DROP bit */
        mask = VTSS_BIT(i + 16);
    } else if (type == VTSS_PACKET_REG_CPU_COPY) {
        /* Set REDIR and DROP bits */
        mask = (VTSS_BIT(i) | VTSS_BIT(i + 16));
    } else { 
        /* No bits set for VTSS_PACKET_REG_FORWARD */
        mask = 0;
    }
    return mask;
}

static vtss_rc srvl_rx_conf_set(void)
{
    vtss_packet_rx_conf_t      *conf = &vtss_state->rx_conf;
    vtss_packet_rx_reg_t       *reg = &conf->reg;
    vtss_packet_rx_queue_map_t *map = &conf->map;
    u32                        queue, i, value, port, bpdu, garp;
    vtss_port_no_t             port_no;
    vtss_packet_rx_port_conf_t *port_conf;

    /* Each CPU queue gets reserved extraction buffer space. No sharing at port or buffer level */
    for (queue = 0; queue < vtss_state->rx_queue_count; queue++) {
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(512 /* egress */ + VTSS_CHIP_PORT_CPU * VTSS_PRIOS + queue), conf->queue[queue].size / SRVL_BUFFER_CELL_SZ);
    }
    SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(512 /* egress */ + 224 /* per-port reservation */ + VTSS_CHIP_PORT_CPU), 0); /* No extra shared space at port level */

    /* Rx IPMC registration */
    value =
        (reg->ipmc_ctrl_cpu_copy ? VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_IPMC_CTRL_COPY_ENA  : 0) |
        (reg->igmp_cpu_only      ? VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_IGMP_REDIR_ENA      : 0) |
        (reg->mld_cpu_only       ? VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_MLD_REDIR_ENA       : 0);
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        SRVL_WRM(VTSS_ANA_PORT_CPU_FWD_CFG(port), value, 
                 VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_IPMC_CTRL_COPY_ENA |
                 VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_IGMP_REDIR_ENA |
                 VTSS_F_ANA_PORT_CPU_FWD_CFG_CPU_MLD_REDIR_ENA);
    }

    /* Rx BPDU and GARP registrations */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        port = VTSS_CHIP_PORT(port_no);
        port_conf = &vtss_state->rx_port_conf[port_no];
        for (i = 0, bpdu = 0, garp = 0; i < 16; i++) {
            bpdu |= srvl_cpu_fwd_mask_get(port_conf->bpdu_reg[i], reg->bpdu_cpu_only, i);
            garp |= srvl_cpu_fwd_mask_get(port_conf->garp_reg[i], reg->garp_cpu_only[i], i);
        }
        SRVL_WR(VTSS_ANA_PORT_CPU_FWD_BPDU_CFG(port), bpdu);
        SRVL_WR(VTSS_ANA_PORT_CPU_FWD_GARP_CFG(port), garp);
    }

    /* Fixme - chipset has more queues of classification the API expose */
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
    SRVL_WR(VTSS_ANA_COMMON_CPUQ_CFG, value);

    /* Setup each of the BPDU, GARP and CCM 'address' extraction queues */
    for (i = 0; i < 16; i++) {
        value =
            VTSS_F_ANA_COMMON_CPUQ_8021_CFG_CPUQ_BPDU_VAL(map->bpdu_queue-VTSS_PACKET_RX_QUEUE_START) |
            VTSS_F_ANA_COMMON_CPUQ_8021_CFG_CPUQ_GARP_VAL(map->garp_queue-VTSS_PACKET_RX_QUEUE_START) |
            VTSS_F_ANA_COMMON_CPUQ_8021_CFG_CPUQ_CCM_VAL(VTSS_PACKET_RX_QUEUE_START); /* Fixme */
        SRVL_WR(VTSS_ANA_COMMON_CPUQ_8021_CFG(i), value);
    }

    /* Configure Rx Queue #i to map to an Rx group. */
    for (value = i = 0; i < vtss_state->rx_queue_count; i++) {
        if (conf->grp_map[i]) {
            value |= VTSS_BIT(i);
        }
    }
    SRVL_WR(VTSS_QSYS_SYSTEM_CPU_GROUP_MAP, value);

#if defined(VTSS_FEATURE_NPI)
//    VTSS_RC(srvl_npi_mask_set());
#endif /* VTSS_FEATURE_NPI */

    return VTSS_RC_OK;
}

#define XTR_EOF_0     0x00000080U
#define XTR_EOF_1     0x01000080U
#define XTR_EOF_2     0x02000080U
#define XTR_EOF_3     0x03000080U
#define XTR_PRUNED    0x04000080U
#define XTR_ABORT     0x05000080U
#define XTR_ESCAPE    0x06000080U
#define XTR_NOT_READY 0x07000080U

static vtss_rc srvl_rx_frame_discard(const vtss_packet_rx_queue_t queue_no)
{
    BOOL done = FALSE;
    vtss_packet_rx_grp_t xtr_grp = vtss_state->rx_conf.grp_map[queue_no];

    while (!done) {
        u32 val;
        SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val);
        switch(val) {
        case XTR_ABORT:
        case XTR_PRUNED:
        case XTR_EOF_3:
        case XTR_EOF_2:
        case XTR_EOF_1:
        case XTR_EOF_0:
            SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val); /* Last data */
            done = TRUE;        /* Last 1-4 bytes */
            break;
        case XTR_ESCAPE:
            SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(xtr_grp), &val); /* Escaped data */
            break;
        case XTR_NOT_READY:
        default:
          break;
        }
    }
    return VTSS_RC_OK;
}

static inline u32 BYTE_SWAP(u32 v)
{
    register u32 v1 = v;
    v1 = ((v1 >> 24) & 0x000000FF) | ((v1 >> 8) & 0x0000FF00) | ((v1 << 8) & 0x00FF0000) | ((v1 << 24) & 0xFF000000);
    return v1;
}

/**
 * Return values:
 *  0 = Data OK.
 *  1 = EOF reached. Data OK. bytes_valid indicates the number of valid bytes in last word ([1; 4]).
 *  2 = Error. No data from queue system.
 */
static int srvl_rx_frame_word(vtss_packet_rx_grp_t grp, BOOL first_word, u32 *val, u32 *bytes_valid)
{
    SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(grp), val);

    if (*val == XTR_NOT_READY) {
        /** XTR_NOT_READY means two different things depending on whether this is the first
         * word read of a frame or after at least one word has been read.
         * When the first word, the group is empty, and we return an error.
         * Otherwise we have to wait for the FIFO to have received some more data. */
        if (first_word) {
            return 2; /* Error */
        }
        do {
            SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(grp), val);
        } while (*val == XTR_NOT_READY);
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
        *val = BYTE_SWAP(*val);
        *bytes_valid = 4 - ((*val) & 3);
        SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(grp), val);
        if (*val == XTR_ESCAPE) {
            SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(grp), val);
        }
        return 1; /* EOF */
    case XTR_ESCAPE:
        SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_RD(grp), val);
        /* FALLTHROUGH */
    default:
        *bytes_valid = 4;
        return 0;
    }
}

static vtss_rc srvl_rx_frame_get_internal(vtss_packet_rx_grp_t grp,
                                          u32                  *const ifh,
                                          u8                   *const frame,
                                          const u32            buf_length,
                                          u32                  *frm_length) /* Including CRC */
{
    u32     i, val, bytes_valid, buf_len = buf_length;
    BOOL    done = 0, first_word = 1;
    u8      *buf;
    int     result;


    *frm_length = 0;

    /* Read IFH. It consists of a 4 words */
    for (i = 0; i < 4; i++) {
        if (srvl_rx_frame_word(grp, first_word, &val, &bytes_valid) != 0) {
            /* We accept neither EOF nor ERROR when reading the IFH */
            return VTSS_RC_ERROR;
        }
        ifh[i] = BYTE_SWAP(val);
        first_word = 0;
    }

    buf = frame;

    /* Read the rest of the frame */
    while (!done && buf_len >= 4) {
        result = srvl_rx_frame_word(grp, 0, &val, &bytes_valid);
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
        (void)srvl_rx_frame_discard(grp);
        return VTSS_RC_ERROR;
    }

    if (*frm_length < 60) {
        VTSS_E("short frame, %u bytes read", *frm_length);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_rx_frame_get(const vtss_packet_rx_queue_t  queue_no,
                                 vtss_packet_rx_header_t       *const header,
                                 u8                            *const frame,
                                 const u32                     length)
{
    vtss_rc              rc;
    vtss_packet_rx_grp_t grp = vtss_state->rx_conf.grp_map[queue_no];
    u32                  val, port;
    u32                  ifh[4];
    vtss_port_no_t       port_no;
    BOOL                 found = FALSE;
    u16                  ethtype;

    /* Check if data is ready for grp */
    SRVL_RD(VTSS_DEVCPU_QS_XTR_XTR_DATA_PRESENT, &val);
    if (!(val & VTSS_F_DEVCPU_QS_XTR_XTR_DATA_PRESENT_DATA_PRESENT(VTSS_BIT(grp)))) {
        return VTSS_RC_INCOMPLETE;
    }

    if ((rc = srvl_rx_frame_get_internal(grp, ifh, frame, length, &header->length)) != VTSS_RC_OK) {
        return rc;
    }

    // Decode IFH.
    if (VTSS_EXTRACT_BITFIELD(ifh[0], 56 - 32, 8) != 0xFF) {
        VTSS_E("Invalid signature");
        return VTSS_RC_ERROR;
    }

    /* Note - VLAN tags are *not* stripped on ingress */
    header->tag.vid     = VTSS_EXTRACT_BITFIELD(ifh[3],  0, 12);
    header->tag.cfi     = VTSS_EXTRACT_BITFIELD(ifh[3], 12,  1);
    header->tag.tagprio = VTSS_EXTRACT_BITFIELD(ifh[3], 13,  3);
    header->queue_mask  = VTSS_EXTRACT_BITFIELD(ifh[3], 20,  8);
    header->learn      = (VTSS_EXTRACT_BITFIELD(ifh[3], 28,  2) ? 1 : 0);

    /* Map from chip port to API port */
    port = VTSS_EXTRACT_BITFIELD(ifh[2], 43 - 32, 4);
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (VTSS_CHIP_PORT(port_no) == port) {
            header->port_no = port_no;
            found = TRUE;
            break;
        }
    }
    if (!found) {
        VTSS_E("Unknown chip port: %u", port);
        return VTSS_RC_ERROR;
    }

    ethtype = (frame[12] << 8) + frame[13];
    header->arrived_tagged = (ethtype == VTSS_ETYPE_TAG_C || ethtype == VTSS_ETYPE_TAG_S); /* Emulated */

    return VTSS_RC_OK;
}

static vtss_rc srvl_tx_frame_port(const vtss_port_no_t  port_no,
                                  const u8              *const frame,
                                  const u32             length,
                                  const vtss_vid_t      vid)
{
    u32 status, w, count, last, port = VTSS_CHIP_PORT(port_no);
    const u8 *buf = frame;
    u32 ifh[4];
    vtss_packet_tx_grp_t grp = 0;

    VTSS_N("port_no: %u, length: %u, vid: %u", port_no, length, vid);

    SRVL_RD(VTSS_DEVCPU_QS_INJ_INJ_STATUS, &status);
    if (!(VTSS_X_DEVCPU_QS_INJ_INJ_STATUS_FIFO_RDY(status) & VTSS_BIT(grp))) {
        VTSS_E("Not ready");
        return VTSS_RC_ERROR;
    }

    if (VTSS_X_DEVCPU_QS_INJ_INJ_STATUS_WMARK_REACHED(status) & VTSS_BIT(grp)) {
        VTSS_E("Watermark reached");
        return VTSS_RC_ERROR;
    }

    /* Indicate SOF */
    SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_CTRL(grp), VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_GAP_SIZE(1) | VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_SOF);

    memset(ifh, 0, sizeof(ifh));

    ifh[0] = VTSS_ENCODE_BITFIELD(1,                 127 - 96, 1); // BYPASS
    // The DEST field is split over two 32-bit boundaries.
    if (port > 6) {
        port -= 6;
        ifh[1] = VTSS_ENCODE_BITFIELD(VTSS_BIT(port), 64 - 64, 4); // Don't make it possible to Tx back to the CPU port (hence length == 4 and not 5).
    } else {
        ifh[2] = VTSS_ENCODE_BITFIELD(VTSS_BIT(port), 57 - 32, 7);
    }

    ifh[3] = VTSS_ENCODE_BITFIELD(0x3,                28 -  0, 2); // Disable rewriter (this is the POP_CNT field of the injection header).

    // Write the IFH to the chip.
    for (w = 0; w < 4; w++) {
        SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(grp), BYTE_SWAP(ifh[w]));
    }

    /* Write whole words */
    count = (length / 4);
    for (w = 0; w < count; w++, buf += 4) {
        if (w == 3 && vid != VTSS_VID_NULL) {
            /* Insert C-tag */
            SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(grp), BYTE_SWAP((0x8100U << 16) | vid));
            w++;
        }
        SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(grp), (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
    }

    /* Write last, partial word */
    last = (length % 4);
    if (last) {
        SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(grp), (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
        w++;
    }

    /* Add padding */
    while (w < (60 / 4)) {
        SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(grp), 0);
        w++;
    }

    /* Indicate EOF and valid bytes in last word */
    SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_CTRL(grp),
           VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_GAP_SIZE(1)                       |
           VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_VLD_BYTES(length < 60 ? 0 : last) |
           VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_EOF);

    /* Add dummy CRC */
    SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_WR(grp), 0);
    w++;

    VTSS_N("wrote %u words, last: %u", w, last);

    return VTSS_RC_OK;
}

static vtss_rc srvl_rx_hdr_decode(const vtss_state_t          *const state,
                                  const vtss_packet_rx_meta_t *const meta,
                                        vtss_packet_rx_info_t *const info)
{
    u64                 ifh[2];
    u32                 sflow_id;
    vtss_phys_port_no_t chip_port;
    vtss_etype_t        etype;
    u8                  *xtr_hdr = meta->bin_hdr;
    u32                 rew_op, rew_op_lsb, rew_val;
    int                 i;
    vtss_trace_group_t  trc_grp = meta->no_wait ? VTSS_TRACE_GROUP_FDMA_IRQ : VTSS_TRACE_GROUP_PACKET;

    VTSS_DG(trc_grp, "IFH:");
    VTSS_DG_HEX(trc_grp, xtr_hdr, 8);

    for (i = 0; i < 2; i++) {
        int offset = 8 * i;
        ifh[i] = ((u64)xtr_hdr[offset + 0] << 56) | ((u64)xtr_hdr[offset + 1] << 48) | ((u64)xtr_hdr[offset + 2] << 40) | ((u64)xtr_hdr[offset + 3] << 32) |
                 ((u64)xtr_hdr[offset + 4] << 24) | ((u64)xtr_hdr[offset + 5] << 16) | ((u64)xtr_hdr[offset + 6] <<  8) | ((u64)xtr_hdr[offset + 7] <<  0);
    }

    memset(info, 0, sizeof(*info));

    info->sw_tstamp = meta->sw_tstamp;
    info->length    = meta->length;
    info->glag_no   = VTSS_GLAG_NO_NONE;

    // Map from chip port to API port
    chip_port = VTSS_EXTRACT_BITFIELD64(ifh[1], 43, 4);
    info->port_no = vtss_cmn_chip_to_logical_port(state, 0, chip_port);
    if (info->port_no == VTSS_PORT_NO_NONE) {
        VTSS_EG(trc_grp, "Unknown chip port: %u", chip_port);
        return VTSS_RC_ERROR;
    }

    info->tag.pcp     = VTSS_EXTRACT_BITFIELD64(ifh[1], 13,  3);
    info->tag.dei     = VTSS_EXTRACT_BITFIELD64(ifh[1], 12,  1);
    info->tag.vid     = VTSS_EXTRACT_BITFIELD64(ifh[1],  0, 12);
    info->xtr_qu_mask = VTSS_EXTRACT_BITFIELD64(ifh[1], 20,  8);
    info->cos         = VTSS_EXTRACT_BITFIELD64(ifh[1], 17,  3);

    etype = (meta->bin_hdr[16 /* xtr hdr */ + 2 * 6 /* 2 MAC addresses*/] << 8) | (meta->bin_hdr[16 + 2 * 6 + 1]);
    VTSS_RC(vtss_cmn_packet_hints_update(state, trc_grp, etype, info));

    info->acl_hit = VTSS_EXTRACT_BITFIELD64(ifh[1], 31, 1);
    if (info->acl_hit) {
        info->acl_idx = VTSS_EXTRACT_BITFIELD64(ifh[1], 37, 6);
    }

    rew_op  = VTSS_EXTRACT_BITFIELD64(ifh[0], 118 - 64,  9);
    rew_val = VTSS_EXTRACT_BITFIELD64(ifh[0],  86 - 64, 32);
    rew_op_lsb = rew_op & 0x7;

    if (rew_op_lsb == 3) {
        // Two-step PTP Tx timestamp in MSbits of the rew_op field
        info->tstamp_id         = rew_op >> 3;
        info->tstamp_id_decoded = TRUE;
    } else if (rew_op_lsb == 4) {
        // REW_VAL contains OAM info that we cannot decode without frame knowledge.
        // As a way out, we just save it in oam_info and let the application decode it itself :-(
        info->oam_info         = rew_val;
        info->oam_info_decoded = TRUE;
    }

    if (rew_op_lsb != 4) {
        // Rx timestamp in rew_val except when REW_OP[2:0] == 4
        info->hw_tstamp         = rew_val;
        info->hw_tstamp_decoded = TRUE;
    }

    // sflow_id:
    // 0-11 : Frame has been sFlow sampled by a Tx sampler on port given by @sflow_id.
    // 12   : Frame has been sFlow sampled by an Rx sampler on port given by @src_port.
    // 13-14: Reserved.
    // 15   : Frame has not been sFlow sampled.
    sflow_id = VTSS_EXTRACT_BITFIELD64(ifh[1], 32, 4);
    if (sflow_id == 12) {
        info->sflow_type    = VTSS_SFLOW_TYPE_RX;
        info->sflow_port_no = info->port_no;
    } else if (sflow_id < 12) {
        info->sflow_type    = VTSS_SFLOW_TYPE_TX;
        info->sflow_port_no = vtss_cmn_chip_to_logical_port(state, 0, sflow_id);
    }

    info->isdx = VTSS_EXTRACT_BITFIELD64(ifh[1], 47, 10);

    return VTSS_RC_OK;
}

/*****************************************************************************/
// srvl_oam_type_to_ifh()
/*****************************************************************************/
static vtss_rc srvl_oam_type_to_ifh(vtss_packet_oam_type_t oam_type, u32 *result)
{
    vtss_rc rc = VTSS_RC_OK;

    switch (oam_type) {
    case VTSS_PACKET_OAM_TYPE_NONE:
        *result = 0;
        break;
    case VTSS_PACKET_OAM_TYPE_CCM:
        *result = 2;
        break;
    case VTSS_PACKET_OAM_TYPE_CCM_LM:
        *result = 3;
        break;
    case VTSS_PACKET_OAM_TYPE_LBM:
        *result = 4;
        break;
    case VTSS_PACKET_OAM_TYPE_LBR:
        *result = 5;
        break;
    case VTSS_PACKET_OAM_TYPE_LMM:
        *result = 6;
        break;
    case VTSS_PACKET_OAM_TYPE_LMR:
        *result = 7;
        break;
    case VTSS_PACKET_OAM_TYPE_DMM:
        *result = 8;
        break;
    case VTSS_PACKET_OAM_TYPE_DMR:
        *result = 9;
        break;
    case VTSS_PACKET_OAM_TYPE_1DM:
        *result = 10;
        break;
    case VTSS_PACKET_OAM_TYPE_LTM:
        *result = 11;
        break;
    case VTSS_PACKET_OAM_TYPE_LTR:
        *result = 12;
        break;
    case VTSS_PACKET_OAM_TYPE_GENERIC:
        *result = 13;
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Invalid OAM type (%d)", oam_type);
        rc = VTSS_RC_ERROR;
        break;
    }
    *result = (*result << 3) | 4;
    return rc;
}

/*****************************************************************************/
// srvl_ptp_action_to_ifh()
/*****************************************************************************/
static vtss_rc srvl_ptp_action_to_ifh(vtss_packet_ptp_action_t ptp_action, u8 ptp_id, u32 *result)
{
    vtss_rc rc = VTSS_RC_OK;

    switch (ptp_action) {
    case VTSS_PACKET_PTP_ACTION_NONE:
        *result = 0;
        break;
    case VTSS_PACKET_PTP_ACTION_ONE_STEP:
        *result = 2; // Residence compensation must be done in S/W, so bit 5 is not set.
        break;
    case VTSS_PACKET_PTP_ACTION_TWO_STEP:
        *result = (3 | (ptp_id << 3)); // Select a PTP timestamp identifier for two-step PTP.
        break;
    case VTSS_PACKET_PTP_ACTION_ORIGIN_TIMESTAMP:
        *result = 5;
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Invalid PTP action (%d)", ptp_action);
        rc = VTSS_RC_ERROR;
        break;
    }
    return rc;
}

/*****************************************************************************/
// srvl_tx_hdr_encode()
/*****************************************************************************/
static vtss_rc srvl_tx_hdr_encode(      vtss_state_t          *const state,
                                  const vtss_packet_tx_info_t *const info,
                                        u8                    *const bin_hdr,
                                        u32                   *const bin_hdr_len)
{
    u64 inj_hdr[2];

    if (bin_hdr == NULL) {
        // Caller wants us to return the number of bytes required to fill
        // in #bin_hdr. We need 16 bytes for the IFH.
        *bin_hdr_len = 16;
        return VTSS_RC_OK;
    } else if (*bin_hdr_len < 16) {
        return VTSS_RC_ERROR;
    }

    *bin_hdr_len = 16;

    inj_hdr[0] = VTSS_ENCODE_BITFIELD64(!info->switch_frm, 127 - 64, 1); // BYPASS
    inj_hdr[1] = 0;

    if (info->switch_frm) {
        if (info->masquerade_port != VTSS_PORT_NO_NONE) {
            u32 chip_port;
            if (info->masquerade_port >= state->port_count) {
                VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Invalid masquerade port (%u)", info->masquerade_port);
                return VTSS_RC_ERROR;
            }

            chip_port = VTSS_CHIP_PORT_FROM_STATE(state, info->masquerade_port);
            inj_hdr[0] |= VTSS_ENCODE_BITFIELD64(1,         126 - 64, 1); // Enable masquerading.
            inj_hdr[0] |= VTSS_ENCODE_BITFIELD64(chip_port, 122 - 64, 4); // Masquerade port.
        }
        if (info->tag.tpid) {
            inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(1, 28 - 0, 2); // POP_CNT = 1, i.e. pop one tag, expected inserted by application/FDMA driver.
        }
    } else {
        u32            port_cnt, rew_op = 0, rew_val = 0, pop_cnt = 3 /* Disable rewriter */;
        u64            chip_port_mask;
        vtss_chip_no_t chip_no;
        vtss_port_no_t stack_port_no, port_no;

        VTSS_RC(vtss_cmn_logical_to_chip_port_mask(state, info->dst_port_mask, &chip_port_mask, &chip_no, &stack_port_no, &port_cnt, &port_no));

#ifdef VTSS_FEATURE_MIRROR_CPU
        // Add mirror port if enabled. Mirroring of directed frames must occur through the port mask.
        if (state->mirror_conf.port_no != VTSS_PORT_NO_NONE && state->mirror_cpu_ingress) {
            chip_port_mask |= VTSS_BIT64(VTSS_CHIP_PORT_FROM_STATE(state, state->mirror_conf.port_no));
        }
#endif

        if (info->ptp_action != VTSS_PACKET_PTP_ACTION_NONE) {
            VTSS_RC(srvl_ptp_action_to_ifh(info->ptp_action, info->ptp_id, &rew_op));
            rew_val = info->ptp_timestamp;
            pop_cnt = 0; // Don't disable rewriter.
        } else if (info->oam_type != VTSS_PACKET_OAM_TYPE_NONE) {
            VTSS_RC(srvl_oam_type_to_ifh(info->oam_type, &rew_op));
            pop_cnt = 0; // Don't disable rewriter.
        }

#if defined(VTSS_FEATURE_AFI_SWC)
        if (info->ptp_action == VTSS_PACKET_PTP_ACTION_NONE && info->afi_id != VTSS_AFI_ID_NONE) {
            vtss_afi_slot_conf_t *slot_conf = &state->afi_slots[info->afi_id];

            // The rew_val is re-used for AFI, so we can't periodically transmit a frame
            // if ptp_action != VTSS_PACKET_PTP_ACTION_NONE
            if (port_cnt != 1) {
                VTSS_EG(VTSS_TRACE_GROUP_PACKET, "When using AFI, exactly one port must be specified (dst_port_mask = 0x%llu).", info->dst_port_mask);
                return VTSS_RC_ERROR;
            }

            if (info->afi_id >= VTSS_ARRSZ(state->afi_slots)) {
                VTSS_EG(VTSS_TRACE_GROUP_PACKET, "Invalid AFI ID (%u)", info->afi_id);
                return VTSS_RC_ERROR;
            }

            VTSS_ENTER();
            if (slot_conf->state != VTSS_AFI_SLOT_STATE_RESERVED) {
                VTSS_EXIT();
                VTSS_EG(VTSS_TRACE_GROUP_PACKET, "AFI ID (%u) not allocated", info->afi_id);
                return VTSS_RC_ERROR;
            }

            slot_conf->state   = VTSS_AFI_SLOT_STATE_ENABLED;
            slot_conf->port_no = port_no;
            VTSS_EXIT();

            rew_val = info->afi_id;
            inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(slot_conf->timer_idx + 1, 37 -  0,  4);
        }
#endif

        inj_hdr[0] |= VTSS_ENCODE_BITFIELD64(rew_op,             118 - 64,  9); // REW_OP
        inj_hdr[0] |= VTSS_ENCODE_BITFIELD64(rew_val,             86 - 64, 32); // REW_VAL
        inj_hdr[0] |= VTSS_ENCODE_BITFIELD64(chip_port_mask >> 7, 64 - 64,  4); // Don't send to the CPU port (hence length == 4 and not 6)
        inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(chip_port_mask,      57 -  0,  7);
        if (info->isdx != VTSS_ISDX_NONE) {
            inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(info->isdx,      47 -  0, 10); // ISDX
            inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(1,               32 -  0,  1); // Use ISDX
        }
        inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(info->dp,            30 -  0,  1); // Drop Precedence
        inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(pop_cnt,             28 -  0,  2); // POP_CNT
        inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(info->cos >= 8 ? 7 : info->cos, 17 - 0, 3); // QOS Class
        inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(info->tag.tpid == 0 || info->tag.tpid == 0x8100 ? 0 : 1, 16 - 0, 1); // TAG_TYPE
        inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(info->tag.pcp,       13 -  0,  3); // PCP
        inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(info->tag.dei,       12 -  0,  1); // DEI
        inj_hdr[1] |= VTSS_ENCODE_BITFIELD64(info->tag.vid,        0 -  0, 16); // VID
    }

    // Time to store it in DDR SDRAM.
    bin_hdr[ 0] = inj_hdr[0] >> 56;
    bin_hdr[ 1] = inj_hdr[0] >> 48;
    bin_hdr[ 2] = inj_hdr[0] >> 40;
    bin_hdr[ 3] = inj_hdr[0] >> 32;
    bin_hdr[ 4] = inj_hdr[0] >> 24;
    bin_hdr[ 5] = inj_hdr[0] >> 16;
    bin_hdr[ 6] = inj_hdr[0] >>  8;
    bin_hdr[ 7] = inj_hdr[0] >>  0;
    bin_hdr[ 8] = inj_hdr[1] >> 56;
    bin_hdr[ 9] = inj_hdr[1] >> 48;
    bin_hdr[10] = inj_hdr[1] >> 40;
    bin_hdr[11] = inj_hdr[1] >> 32;
    bin_hdr[12] = inj_hdr[1] >> 24;
    bin_hdr[13] = inj_hdr[1] >> 16;
    bin_hdr[14] = inj_hdr[1] >>  8;
    bin_hdr[15] = inj_hdr[1] >>  0;

    VTSS_IG(VTSS_TRACE_GROUP_PACKET, "IFH:");
    VTSS_IG_HEX(VTSS_TRACE_GROUP_PACKET, &bin_hdr[0], *bin_hdr_len);

    return VTSS_RC_OK;
}

#define SRVL_DEBUG_CPU_FWD(pr, addr, i, name) srvl_debug_reg_inst(pr, VTSS_ANA_PORT_CPU_FWD_##addr, i, "FWD_"name)
#define SRVL_DEBUG_XTR(pr, addr, name) srvl_debug_reg(pr, VTSS_DEVCPU_QS_XTR_XTR_##addr, "XTR_"name)
#define SRVL_DEBUG_XTR_INST(pr, addr, i, name) srvl_debug_reg_inst(pr, VTSS_DEVCPU_QS_XTR_XTR_##addr, i, "XTR_"name)
#define SRVL_DEBUG_INJ(pr, addr, name) srvl_debug_reg(pr, VTSS_DEVCPU_QS_INJ_INJ_##addr, "INJ_"name)
#define SRVL_DEBUG_INJ_INST(pr, addr, i, name) srvl_debug_reg_inst(pr, VTSS_DEVCPU_QS_INJ_INJ_##addr, i, "INJ_"name)

static vtss_rc srvl_debug_pkt(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    u32  i, port;
    char buf[32];

    /* Analyzer CPU forwarding registers */
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        sprintf(buf, "Port %u", port);
        srvl_debug_reg_header(pr, buf);
        SRVL_DEBUG_CPU_FWD(pr, CFG(port), port, "CFG");
        SRVL_DEBUG_CPU_FWD(pr, BPDU_CFG(port), port, "BPDU_CFG");
        SRVL_DEBUG_CPU_FWD(pr, GARP_CFG(port), port, "GARP_CFG");
        SRVL_DEBUG_CPU_FWD(pr, CCM_CFG(port), port, "CCM_CFG");
        pr("\n");
    }

    /* Analyzer CPU queue mappings */
    srvl_debug_reg_header(pr, "CPU Queues");
    srvl_debug_reg(pr, VTSS_ANA_COMMON_CPUQ_CFG, "CPUQ_CFG");
    for (i = 0; i < 16; i++)
        srvl_debug_reg_inst(pr, VTSS_ANA_COMMON_CPUQ_8021_CFG(i), i, "CPUQ_8021_CFG");
    pr("\n");

    /* Packet extraction registers */
    srvl_debug_reg_header(pr, "Extraction");
    for (i = 0; i < VTSS_PACKET_RX_GRP_CNT; i++)
        SRVL_DEBUG_XTR_INST(pr, FRM_PRUNING(i), i, "FRM_PRUNING");
    for (i = 0; i < VTSS_PACKET_RX_GRP_CNT; i++)
        SRVL_DEBUG_XTR_INST(pr, GRP_CFG(i), i, "GRP_CFG");
    srvl_debug_reg(pr, VTSS_QSYS_SYSTEM_CPU_GROUP_MAP, "XTR_MAP");
    SRVL_DEBUG_XTR(pr, DATA_PRESENT, "DATA_PRESENT");
    pr("\n");

    /* Packet injection registers */
    srvl_debug_reg_header(pr, "Injection");
    for (i = 0; i < VTSS_PACKET_TX_GRP_CNT; i++)
        SRVL_DEBUG_INJ_INST(pr, GRP_CFG(i), i, "GRP_CFG");
    for (i = 0; i < VTSS_PACKET_TX_GRP_CNT; i++)
        SRVL_DEBUG_INJ_INST(pr, CTRL(i), i, "CTRL");
    for (i = 0; i < VTSS_PACKET_TX_GRP_CNT; i++)
        SRVL_DEBUG_INJ_INST(pr, ERR(i), i, "ERR");
    SRVL_DEBUG_INJ(pr, STATUS, "STATUS");
    pr("\n");

    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_OAM)

/* ================================================================= *
 *  Y.1731/802.1ag OAM
 * ================================================================= */
static vtss_rc srvl_oam_voe_counter_update_internal(const vtss_oam_voe_idx_t voe_idx, const u32 clear_mask);

/* Boolean-to-bits */
#define B2B(boolean, bits)        ((boolean) ? (bits) : 0)

static vtss_rc srvl_oam_vop_int_enable(u32 enable)
{
    SRVL_WRM(VTSS_OAM_MEP_COMMON_MASTER_INTR_CTRL, B2B(enable, VTSS_F_OAM_MEP_COMMON_MASTER_INTR_CTRL_OAM_MEP_INTR_ENA), VTSS_F_OAM_MEP_COMMON_MASTER_INTR_CTRL_OAM_MEP_INTR_ENA);
    return VTSS_RC_OK;
}

/* Determine VOP interrupt flag -- clear if no enabled VOEs have
 * interrupts enabled */
static vtss_rc srvl_oam_vop_int_update(void)
{
    u32 i;
    u32 must_enable;
    for (i=0, must_enable=0; !must_enable && i<VTSS_OAM_VOE_CNT; ++i) {
        if (vtss_state->oam_voe_internal[i].allocated) {
            SRVL_RD(VTSS_OAM_MEP_VOE_INTR_ENA(i), &must_enable);
            must_enable &= 0xff;  /* Only bits 0-7 are valid on Serval */
        }
    }
    return srvl_oam_vop_int_enable(must_enable);
}

/* CCM-LM: the HW period "runs at double speed", so we need to dedicate two
 * slots int the CCM period config register for double values and use those
 * slots when CCM-LM is configured.
 *
 * However, from an API standpoint the user doesn't need to know this, so we
 * use the standard enums from #vtss_oam_period_t in the API and convert them
 * to the HW-near values here.
 *
 * This, of course, invalidates the semantics of the enum, so we return u32
 * from this func. Also note that only the 100ms and 1-sec values are supported
 * by the API.
 */
static u32 srvl_oam_ccm_lm_period_index(vtss_oam_period_t p)
{
    switch (p) {
    case VTSS_OAM_PERIOD_100_MS:
        return VTSS_OAM_PERIOD_1_SEC + 1;
    case VTSS_OAM_PERIOD_1_SEC:
        return VTSS_OAM_PERIOD_1_SEC + 2;
    default:
        return 0;
    }
}

static vtss_rc doing_calculate(u32 voe_idx,  BOOL *doing_lb,  BOOL *doing_tst)
{
    u32 v;
    BOOL lb_hw_enabled, tst_hw_enabled;

    SRVL_RD(VTSS_OAM_MEP_VOE_OAM_HW_CTRL(voe_idx), &v);
    lb_hw_enabled = (v & (VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_LBM_ENA | VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_LBR_ENA));
    SRVL_RD(VTSS_OAM_MEP_VOE_OAM_FWD_CTRL(voe_idx), &v);
    tst_hw_enabled = VTSS_X_OAM_MEP_VOE_OAM_FWD_CTRL_GENERIC_FWD_MASK(v) & (1<<6);

    *doing_lb  = lb_hw_enabled && !tst_hw_enabled;
    *doing_tst = lb_hw_enabled &&  tst_hw_enabled;

    return(VTSS_RC_OK);
}

static vtss_rc srvl_oam_vop_cfg_set(void)
{
    u32 i, mask, external_cpu_portmask;
    u32 v;

    const vtss_oam_vop_conf_t *cfg = &vtss_state->oam_vop_conf;
    
    SRVL_WR(VTSS_OAM_MEP_COMMON_CCM_PERIOD_CFG(VTSS_OAM_PERIOD_3_3_MS-1),   16650);
    SRVL_WR(VTSS_OAM_MEP_COMMON_CCM_PERIOD_CFG(VTSS_OAM_PERIOD_10_MS-1),    50454);
    SRVL_WR(VTSS_OAM_MEP_COMMON_CCM_PERIOD_CFG(VTSS_OAM_PERIOD_100_MS-1),  504541);
    SRVL_WR(VTSS_OAM_MEP_COMMON_CCM_PERIOD_CFG(VTSS_OAM_PERIOD_1_SEC-1),  5045409);
    SRVL_WR(VTSS_OAM_MEP_COMMON_CCM_PERIOD_CFG(srvl_oam_ccm_lm_period_index(VTSS_OAM_PERIOD_100_MS)-1),  2*504541);
    SRVL_WR(VTSS_OAM_MEP_COMMON_CCM_PERIOD_CFG(srvl_oam_ccm_lm_period_index(VTSS_OAM_PERIOD_1_SEC)-1),  2*5045409);
    
    v = B2B(cfg->pdu_type.other.to_front,  VTSS_F_OAM_MEP_COMMON_CPU_CFG_DEF_EXT_PORT_ENA)     | VTSS_F_OAM_MEP_COMMON_CPU_CFG_DEF_COPY_QU(cfg->pdu_type.other.rx_queue) |
        B2B(cfg->pdu_type.err.to_front,    VTSS_F_OAM_MEP_COMMON_CPU_CFG_CPU_ERR_EXT_PORT_ENA) | VTSS_F_OAM_MEP_COMMON_CPU_CFG_CPU_ERR_QU(cfg->pdu_type.err.rx_queue)    |
        B2B(cfg->pdu_type.dmr.to_front,    VTSS_F_OAM_MEP_COMMON_CPU_CFG_DMR_EXT_PORT_ENA)     | VTSS_F_OAM_MEP_COMMON_CPU_CFG_DMR_CPU_QU(cfg->pdu_type.dmr.rx_queue)    |
        B2B(cfg->pdu_type.lmr.to_front,    VTSS_F_OAM_MEP_COMMON_CPU_CFG_LMR_EXT_PORT_ENA)     | VTSS_F_OAM_MEP_COMMON_CPU_CFG_LMR_CPU_QU(cfg->pdu_type.lmr.rx_queue)    |
        B2B(cfg->pdu_type.lbr.to_front,    VTSS_F_OAM_MEP_COMMON_CPU_CFG_LBR_EXT_PORT_ENA)     | VTSS_F_OAM_MEP_COMMON_CPU_CFG_LBR_CPU_QU(cfg->pdu_type.lbr.rx_queue)    |
        B2B(cfg->pdu_type.ccm_lm.to_front, VTSS_F_OAM_MEP_COMMON_CPU_CFG_CCM_LM_EXT_PORT_ENA)  | VTSS_F_OAM_MEP_COMMON_CPU_CFG_CCM_LM_CPU_QU(cfg->pdu_type.ccm_lm.rx_queue);
    SRVL_WR(VTSS_OAM_MEP_COMMON_CPU_CFG, v);
    
    v = B2B(cfg->pdu_type.lt.to_front,     VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_LT_EXT_PORT_ENA)    | VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_LT_CPU_QU(cfg->pdu_type.lt.rx_queue)    |
        B2B(cfg->pdu_type.dmm.to_front,    VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_DMM_EXT_PORT_ENA)   | VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_DMM_CPU_QU(cfg->pdu_type.dmm.rx_queue)  |
        B2B(cfg->pdu_type.lmm.to_front,    VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_LMM_EXT_PORT_ENA)   | VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_LMM_CPU_QU(cfg->pdu_type.lmm.rx_queue)  |
        B2B(cfg->pdu_type.lbm.to_front,    VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_LBM_EXT_PORT_ENA)   | VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_LBM_CPU_QU(cfg->pdu_type.lbm.rx_queue)  |
        B2B(cfg->pdu_type.ccm.to_front,    VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_CCM_EXT_PORT_ENA)   | VTSS_F_OAM_MEP_COMMON_CPU_CFG_1_CCM_CPU_QU(cfg->pdu_type.ccm.rx_queue);
    SRVL_WR(VTSS_OAM_MEP_COMMON_CPU_CFG_1, v);

    for (i=0; i<VTSS_OAM_GENERIC_OPCODE_CFG_CNT; ++i) {
        const vtss_oam_vop_generic_opcode_conf_t *g = &(cfg->pdu_type.generic[i]);
        v = B2B(g->extract.to_front, VTSS_F_OAM_MEP_COMMON_OAM_GENERIC_CFG_GENERIC_OPCODE_EXT_PORT_ENA) |
            VTSS_F_OAM_MEP_COMMON_OAM_GENERIC_CFG_GENERIC_OPCODE_CPU_QU(g->extract.rx_queue) |
            VTSS_F_OAM_MEP_COMMON_OAM_GENERIC_CFG_GENERIC_OPCODE_VAL(g->opcode) |
            B2B(!g->check_dmac, VTSS_F_OAM_MEP_COMMON_OAM_GENERIC_CFG_GENERIC_DMAC_CHK_DIS);
        SRVL_WR(VTSS_OAM_MEP_COMMON_OAM_GENERIC_CFG(i), v);
    }

    v = (cfg->common_multicast_dmac.addr[0] << 8) | cfg->common_multicast_dmac.addr[1];
    SRVL_WR(VTSS_OAM_MEP_COMMON_COMMON_MEP_MC_MAC_MSB, VTSS_F_OAM_MEP_COMMON_COMMON_MEP_MC_MAC_MSB_MEP_MC_MAC_MSB(v));

    v = (cfg->common_multicast_dmac.addr[2] << 24) |
        (cfg->common_multicast_dmac.addr[3] << 16) |
        (cfg->common_multicast_dmac.addr[4] <<  8) |
        (cfg->common_multicast_dmac.addr[5]);
    v >>= 4;   /* Value in reg. doesn't include the least significant nibble */
    SRVL_WR(VTSS_OAM_MEP_COMMON_COMMON_MEP_MC_MAC_LSB, VTSS_F_OAM_MEP_COMMON_COMMON_MEP_MC_MAC_LSB_MEP_MC_MAC_LSB(v));

    for (i=0, external_cpu_portmask=0, mask=0x01; i<VTSS_PORT_ARRAY_SIZE; ++i, mask<<=1) { /* Convert the logical port mask to a chip port mask */
        if (cfg->external_cpu_portmask & mask)
            external_cpu_portmask |= 1<<VTSS_CHIP_PORT(i);
    }

    v = B2B(cfg->ccm_lm_enable_rx_fcf_in_reserved_field, VTSS_F_OAM_MEP_COMMON_MEP_CTRL_CCM_LM_UPD_RSV_ENA) |
        B2B(cfg->down_mep_lmr_proprietary_fcf_use,       VTSS_F_OAM_MEP_COMMON_MEP_CTRL_LMR_UPD_RxFcL_ENA) |
        B2B(cfg->enable_all_voe,                         VTSS_F_OAM_MEP_COMMON_MEP_CTRL_MEP_ENA) |
        VTSS_F_OAM_MEP_COMMON_MEP_CTRL_CCM_SCAN_ENA |
        VTSS_F_OAM_MEP_COMMON_MEP_CTRL_EXT_CPU_PORTMASK(external_cpu_portmask);
    SRVL_WR(VTSS_OAM_MEP_COMMON_MEP_CTRL, v);

    SRVL_WR(VTSS_OAM_MEP_COMMON_OAM_SDLB_CPU_COPY, cfg->sdlb_cpy_copy_idx);
    
    /* We don't touch the interrupt enable bit here; it defaults to disabled, 
     * so we leave it to VOE interrupt configuration to modify it.
     */
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_voe_alloc(const vtss_oam_voe_type_t      type,
                                  const vtss_oam_voe_alloc_cfg_t *data,
                                  vtss_oam_voe_idx_t             *voe_idx)
{
    u32 i;
    if (type == VTSS_OAM_VOE_SERVICE) {
        for (i=0; i<VTSS_OAM_PATH_SERVICE_VOE_CNT  &&  vtss_state->oam_voe_internal[i].allocated; ++i)
            /* search for free VOE instance */ ;
        if (i == VTSS_OAM_PATH_SERVICE_VOE_CNT)
            return VTSS_RC_ERROR;
    }
    else {
        i = VTSS_CHIP_PORT(data->phys_port) + VTSS_OAM_PORT_VOE_BASE_IDX;
        if (i >= VTSS_OAM_VOE_CNT  ||  vtss_state->oam_voe_internal[i].allocated)
            return VTSS_RC_ERROR;
    }

    *voe_idx = i;
    vtss_state->oam_voe_internal[i].allocated = TRUE;
    SRVL_WR(VTSS_OAM_MEP_VOE_BASIC_CTRL(i), 0);   /* Make sure the VOE isn't physically enabled yet */

    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_voe_free(const vtss_oam_voe_idx_t voe_idx)
{
    if (vtss_state->oam_voe_internal[voe_idx].allocated) {
        /* Disable VOE, and we're done */
        // FIXME: Correct? HenrikB's has an idea about copy-to-CPU?
        vtss_state->oam_voe_internal[voe_idx].allocated = FALSE;
        SRVL_WRM(VTSS_OAM_MEP_VOE_BASIC_CTRL(voe_idx), 0, VTSS_F_OAM_MEP_VOE_BASIC_CTRL_VOE_ENA);
        return srvl_oam_vop_int_update();
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_event_poll(vtss_oam_event_mask_t *const mask)
{
    u32 i;
    for (i=0; i<VTSS_OAM_EVENT_MASK_ARRAY_SIZE; ++i)
        SRVL_RD(VTSS_OAM_MEP_COMMON_INTR(i), &(mask->voe_mask[i]));
    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_voe_event_enable(const vtss_oam_voe_idx_t        voe_idx,
                                         const vtss_oam_voe_event_mask_t mask,
                                         const BOOL                      enable)
{
    u32 m;
    SRVL_RD(VTSS_OAM_MEP_VOE_INTR_ENA(voe_idx), &m);
    m = enable ? (m | mask) : (m & ~mask);
    SRVL_WR(VTSS_OAM_MEP_VOE_INTR_ENA(voe_idx), m);
    return m ? srvl_oam_vop_int_enable(TRUE) : srvl_oam_vop_int_update();
}

static vtss_rc srvl_oam_voe_event_poll(const vtss_oam_voe_idx_t  voe_idx,
                                       vtss_oam_voe_event_mask_t *const mask)
{
    u32 intmask;
    SRVL_RD(VTSS_OAM_MEP_VOE_INTR_ENA(voe_idx), &intmask);
    SRVL_RD(VTSS_OAM_MEP_VOE_STICKY(voe_idx), mask);
    *mask &= intmask;
    SRVL_WR(VTSS_OAM_MEP_VOE_STICKY(voe_idx), *mask);
    return VTSS_RC_OK;
}    

/* Forward decl; the implementation really doesn't belong here. */
static vtss_rc srvl_oam_voe_counter_clear(const vtss_oam_voe_idx_t voe_idx, const u32 mask);

static vtss_rc srvl_oam_voe_cfg_set(const vtss_oam_voe_idx_t voe_idx)
{
    u32  i, v;
    u32  mask;
    BOOL doing_lb;
    BOOL doing_tst;
    BOOL doing_anything;
    const vtss_oam_voe_conf_t *cfg = &(vtss_state->oam_voe_conf[voe_idx]);
    const u8 *p;
    vtss_oam_voe_internal_counters_t *chipcnt = &vtss_state->oam_voe_internal[voe_idx].counters;

    if (cfg->lb.enable && cfg->tst.enable) {
        VTSS_D("VOE %d: Cannot enable simultaneous LB and TST on Serval", voe_idx);
        return VTSS_RC_ERROR;
    }

    /* If the VOE isn't enabled yet, i.e. it's only been allocated and this
     * is the first call to this  function, then we clear the logical
     * counters.
     */
    SRVL_RD(VTSS_OAM_MEP_VOE_BASIC_CTRL(voe_idx), &v);
    if ((v & VTSS_F_OAM_MEP_VOE_BASIC_CTRL_VOE_ENA) == 0)
        VTSS_RC(srvl_oam_voe_counter_clear(voe_idx, VTSS_OAM_CNT_ALL));

    /* Then, if we're enabling LB (and it wasn't enabled previously), we discard
     * any changes to the LB HW counters. Same thing for TST.
     * 
     * This is necessary since the two functions share registers on Serval-1.
     */
    VTSS_RC(doing_calculate(voe_idx, &doing_lb, &doing_tst));

    doing_anything = doing_lb || doing_tst;

    if ((!cfg->tst.enable && doing_tst) || (!cfg->lb.enable && doing_lb))  /* TST or LB is being disabled - update the counters */
        (void)srvl_oam_voe_counter_update_internal(voe_idx, 0);
    if (cfg->tst.enable && (doing_lb || !doing_anything)) {        /* Idle-to-TST or change from LB to TST - rebase TST counters */
        SRVL_RD(VTSS_OAM_MEP_VOE_LBR_RX_FRM_CNT(voe_idx), &v);
        vtss_cmn_counter_32_rebase(v, &chipcnt->tst.rx_tst);
        SRVL_RD(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_CFG(voe_idx), &v);
        vtss_cmn_counter_32_rebase(v, &chipcnt->tst.tx_tst);
    }
    if (cfg->lb.enable && (doing_tst || !doing_anything)) {        /* Idle-to-LB or change from TST to LB - rebase LB counters */
        SRVL_RD(VTSS_OAM_MEP_VOE_LBR_RX_FRM_CNT(voe_idx), &v);
        vtss_cmn_counter_32_rebase(v, &chipcnt->lb.rx_lbr);
        SRVL_RD(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_CFG(voe_idx), &v);
        vtss_cmn_counter_32_rebase(v, &chipcnt->lb.tx_lbm);
    }

    /* MEL_CTRL */
    v = 0;
    if (!cfg->proc.copy_next_only)
        v |= B2B(cfg->proc.copy_on_mel_too_low_err, VTSS_F_OAM_MEP_VOE_MEL_CTRL_CPU_COPY_MEL_TOO_LOW | VTSS_F_OAM_MEP_VOE_MEL_CTRL_CPU_COPY_CCM_MEL_TOO_LOW);
    v |= VTSS_F_OAM_MEP_VOE_MEL_CTRL_MEL_VAL(cfg->proc.meg_level);
    if (cfg->mep_type == VTSS_OAM_UPMEP) {
        v |= B2B(cfg->upmep.loopback, VTSS_F_OAM_MEP_VOE_MEL_CTRL_UPMEP_LB);
        v |= VTSS_F_OAM_MEP_VOE_MEL_CTRL_MEP_PORTMASK(1 << VTSS_CHIP_PORT(cfg->upmep.port));
    }
    SRVL_WR(VTSS_OAM_MEP_VOE_MEL_CTRL(voe_idx), v);  /* Debug bit 4 is always set to zero; don't expect it to be used */
    
    /* OAM_CPU_COPY_CTRL */
    v = 0;
    for (i=0, mask=0x01; i<VTSS_OAM_GENERIC_OPCODE_CFG_CNT; ++i, mask<<=1)
        v |= B2B(cfg->generic[i].enable  && cfg->generic[i].copy_to_cpu,          VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_GENERIC_COPY_MASK(mask));
    v |= B2B(cfg->unknown.enable         && cfg->unknown.copy_to_cpu,             VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_UNK_OPCODE_CPU_COPY_ENA);
    v |= B2B(cfg->single_ended_lm.enable && cfg->single_ended_lm.copy_lmm_to_cpu, VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_LMM_CPU_COPY_ENA);
    v |= B2B(cfg->single_ended_lm.enable && cfg->single_ended_lm.copy_lmr_to_cpu, VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_LMR_CPU_COPY_ENA);
    v |= B2B(cfg->tst.enable             && cfg->tst.copy_to_cpu,                 VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_LBR_CPU_COPY_ENA);  /* TST RX overloads LBR on Serval 1 */
    v |= B2B(cfg->lb.enable              && cfg->lb.copy_lbm_to_cpu,              VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_LBM_CPU_COPY_ENA);
    v |= B2B(cfg->lb.enable              && cfg->lb.copy_lbr_to_cpu,              VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_LBR_CPU_COPY_ENA);
    v |= B2B(cfg->lt.enable              && cfg->lt.copy_ltm_to_cpu,              VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_LTM_CPU_COPY_ENA);
    v |= B2B(cfg->lt.enable              && cfg->lt.copy_ltr_to_cpu,              VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_LTR_CPU_COPY_ENA);
    v |= B2B(cfg->dm.enable_dmm          && cfg->dm.copy_dmm_to_cpu,              VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_DMM_CPU_COPY_ENA);
    v |= B2B(cfg->dm.enable_dmm          && cfg->dm.copy_dmr_to_cpu,              VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_DMR_CPU_COPY_ENA);
    v |= B2B(cfg->dm.enable_1dm          && cfg->dm.copy_1dm_to_cpu,              VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_SM_CPU_COPY_ENA);
    v |= B2B(cfg->ccm_lm.enable          && cfg->ccm_lm.copy_to_cpu,              VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_CCM_LM_CPU_COPY_ENA);
    if (!cfg->proc.copy_next_only) {
        v |= B2B(                           cfg->ccm.copy_to_cpu,                 VTSS_F_OAM_MEP_VOE_OAM_CPU_COPY_CTRL_CCM_CPU_COPY_ENA);
    }    
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_CPU_COPY_CTRL(voe_idx), v);

    /* OAM_FWD_CTRL */
    v = 0;
    for (i=0, mask=0x01; i<VTSS_OAM_GENERIC_OPCODE_CFG_CNT; ++i, mask<<=1)
        v |= B2B(cfg->generic[i].enable  && cfg->generic[i].forward,          VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_GENERIC_FWD_MASK(mask));
    v |= B2B(                               cfg->ccm.forward,                 VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_CCM_FWD_ENA);
    v |= B2B(cfg->ccm_lm.enable          && cfg->ccm_lm.forward,              VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_CCM_LM_FWD_ENA);
    v |= B2B(cfg->single_ended_lm.enable && cfg->single_ended_lm.forward_lmm, VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_LMM_FWD_ENA);
    v |= B2B(cfg->single_ended_lm.enable && cfg->single_ended_lm.forward_lmr, VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_LMR_FWD_ENA);
    v |= B2B(cfg->lb.enable              && cfg->lb.forward_lbm,              VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_LBM_FWD_ENA);
    v |= B2B(cfg->lb.enable              && cfg->lb.forward_lbr,              VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_LBR_FWD_ENA);
    v |= B2B(cfg->lt.enable              && cfg->lt.forward_ltm,              VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_LTM_FWD_ENA);
    v |= B2B(cfg->lt.enable              && cfg->lt.forward_ltr,              VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_LTR_FWD_ENA);
    v |= B2B(cfg->dm.enable_dmm          && cfg->dm.forward_dmm,              VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_DMM_FWD_ENA);
    v |= B2B(cfg->dm.enable_dmm          && cfg->dm.forward_dmr,              VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_DMR_FWD_ENA);
    v |= B2B(cfg->dm.enable_1dm          && cfg->dm.forward_1dm,              VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_SM_FWD_ENA);
    v |= B2B(                               cfg->tst.enable,                  VTSS_F_OAM_MEP_VOE_OAM_FWD_CTRL_GENERIC_FWD_MASK(1<<6));
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_FWD_CTRL(voe_idx), v);

    /* TST setup. Overloaded LB register semantics on Serval 1. */
    if (cfg->tst.enable) {
        if (doing_tst) {
            /* Don't change anything if we're already running TST frames; it would mess up sequence numbering. */
        }
        else {
            SRVL_WRM(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_UPDATE(voe_idx),
                    B2B(cfg->tst.tx_seq_no_auto_update, VTSS_F_OAM_MEP_VOE_LBM_TX_TRANSID_UPDATE_LBM_TRANSID_UPDATE_ENA),
                    VTSS_F_OAM_MEP_VOE_LBM_TX_TRANSID_UPDATE_LBM_TRANSID_UPDATE_ENA);
            SRVL_WR(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_CFG(voe_idx), cfg->tst.tx_seq_no);
            SRVL_WR(VTSS_OAM_MEP_VOE_LBR_RX_TRANSID_CFG(voe_idx), cfg->tst.rx_seq_no);
            vtss_cmn_counter_32_rebase(cfg->tst.tx_seq_no, &chipcnt->tst.tx_tst);    /* Always rebase when transaction id is changed */
        }
    }
    else {
        if (cfg->lb.enable) {
            if (doing_lb) {
                /* Don't change anything if we're already running LB frames; it would mess up sequence numbering. */
            }
            else {
                SRVL_WRM(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_UPDATE(voe_idx), B2B(cfg->lb.tx_update_transaction_id, VTSS_F_OAM_MEP_VOE_LBM_TX_TRANSID_UPDATE_LBM_TRANSID_UPDATE_ENA), VTSS_F_OAM_MEP_VOE_LBM_TX_TRANSID_UPDATE_LBM_TRANSID_UPDATE_ENA);
                SRVL_WR(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_CFG(voe_idx), cfg->lb.tx_transaction_id);
                SRVL_WR(VTSS_OAM_MEP_VOE_LBR_RX_TRANSID_CFG(voe_idx), cfg->lb.rx_transaction_id);
                vtss_cmn_counter_32_rebase(cfg->lb.tx_transaction_id, &chipcnt->lb.tx_lbm);    /* Always rebase when transaction id is changed */
            }
        }
        else
            SRVL_WRM(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_UPDATE(voe_idx), 0, VTSS_F_OAM_MEP_VOE_LBM_TX_TRANSID_UPDATE_LBM_TRANSID_UPDATE_ENA);
    }

    /* OAM_SDLB_CTRL */
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_SDLB_CTRL(voe_idx),
        B2B(cfg->sdlb_enable, VTSS_F_OAM_MEP_VOE_OAM_SDLB_CTRL_OAM_SDLB_ENA) |
        VTSS_F_OAM_MEP_VOE_OAM_SDLB_CTRL_OAM_SDLB_IDX(cfg->sdlb_idx));

    /* OAM_CNT_OAM_CTRL */    
    v = 0;
    for (i=0, mask=0x01; i<VTSS_OAM_GENERIC_OPCODE_CFG_CNT; ++i, mask<<=1)
        v |= B2B(cfg->generic[i].enable  && cfg->generic[i].count_as_selected,            VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_GENERIC_OAM_CNT_MASK(mask));
    v |= B2B(cfg->unknown.enable         && cfg->unknown.count_as_selected,               VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_UNK_OPCODE_OAM_CNT_ENA);
    v |= B2B(                               cfg->ccm.count_as_selected,                   VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_CCM_OAM_CNT_ENA);
    v |= B2B(cfg->ccm_lm.enable          && cfg->ccm_lm.count_as_selected,                VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_CCM_LM_OAM_CNT_ENA);
    v |= B2B(cfg->single_ended_lm.enable && cfg->single_ended_lm.count_as_selected,       VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_LMM_OAM_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_LMR_OAM_CNT_ENA);
    v |= B2B(cfg->lb.enable              && cfg->lb.count_as_selected,                    VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_LBM_OAM_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_LBR_OAM_CNT_ENA);
    v |= B2B(cfg->tst.enable             && cfg->tst.count_as_selected,                   VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_LBM_OAM_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_LBR_OAM_CNT_ENA);
    v |= B2B(cfg->lt.enable              && cfg->lt.count_as_selected,                    VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_LTM_OAM_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_LTR_OAM_CNT_ENA);
    v |= B2B(cfg->dm.enable_dmm          && cfg->dm.count_as_selected,                    VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_DMM_OAM_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_DMR_OAM_CNT_ENA);
    v |= B2B(cfg->dm.enable_1dm          && cfg->dm.count_as_selected,                    VTSS_F_OAM_MEP_VOE_OAM_CNT_OAM_CTRL_SM_OAM_CNT_ENA);
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_CNT_OAM_CTRL(voe_idx), v);

    /* OAM_CNT_DATA_CTRL */    
    v = 0;
    for (i=0, mask=0x01; i<VTSS_OAM_GENERIC_OPCODE_CFG_CNT; ++i, mask<<=1)
        v |= B2B(cfg->generic[i].enable  && cfg->generic[i].count_as_data,            VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_GENERIC_DATA_CNT_MASK(mask));
    v |= B2B(cfg->unknown.enable         && cfg->unknown.count_as_data,               VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_UNK_OPCODE_DATA_CNT_ENA);
    v |= B2B(                               cfg->ccm.count_as_data,                   VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_CCM_DATA_CNT_ENA);
    v |= B2B(cfg->single_ended_lm.enable && cfg->single_ended_lm.count_as_data,       VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_LMM_DATA_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_LMR_DATA_CNT_ENA);
    v |= B2B(cfg->lb.enable              && cfg->lb.count_as_data,                    VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_LBM_DATA_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_LBR_DATA_CNT_ENA);
    v |= B2B(cfg->tst.enable             && cfg->tst.count_as_data,                   VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_LBM_DATA_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_LBR_DATA_CNT_ENA);
    v |= B2B(cfg->lt.enable              && cfg->lt.count_as_data,                    VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_LTM_DATA_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_LTR_DATA_CNT_ENA);
    v |= B2B(cfg->dm.enable_dmm          && cfg->dm.count_as_data,                    VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_DMM_DATA_CNT_ENA | VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_DMR_DATA_CNT_ENA);
    v |= B2B(cfg->dm.enable_1dm          && cfg->dm.count_as_data,                    VTSS_F_OAM_MEP_VOE_OAM_CNT_DATA_CTRL_SM_DATA_CNT_ENA);
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_CNT_DATA_CTRL(voe_idx), v);

    /* MEP_UC_MAC_LSB / MEP_UC_MAC_MSB */
    v = (cfg->unicast_mac.addr[0] << 8) | cfg->unicast_mac.addr[1];
    SRVL_WR(VTSS_OAM_MEP_VOE_MEP_UC_MAC_MSB(voe_idx), v);
    
    v = (cfg->unicast_mac.addr[2] << 24) |
        (cfg->unicast_mac.addr[3] << 16) |
        (cfg->unicast_mac.addr[4] <<  8) |
        (cfg->unicast_mac.addr[5]);
    SRVL_WR(VTSS_OAM_MEP_VOE_MEP_UC_MAC_LSB(voe_idx), v);

    /* OAM_HW_CTRL */
    v = 0;
    v |= B2B(cfg->ccm.enable,                   VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_CCM_ENA);
    v |= B2B(cfg->ccm_lm.enable,                VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_CCM_LM_ENA);
    v |= B2B(cfg->single_ended_lm.enable,       VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_LMM_ENA | VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_LMR_ENA);
    v |= B2B(cfg->lb.enable || cfg->tst.enable, VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_LBM_ENA | VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_LBR_ENA);
    v |= B2B(cfg->dm.enable_dmm,                VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_DMM_ENA | VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_DMR_ENA);
    v |= B2B(cfg->dm.enable_1dm,                VTSS_F_OAM_MEP_VOE_OAM_HW_CTRL_SDM_ENA);
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_HW_CTRL(voe_idx), v);

    /* LOOPBACK_CTRL */
    v = 0;
    switch (cfg->mep_type) {
    case VTSS_OAM_UPMEP:
        v = 0;
        break;
    case VTSS_OAM_DOWNMEP:
        v |= VTSS_F_OAM_MEP_VOE_LOOPBACK_CTRL_LB_LMM_ENA | VTSS_F_OAM_MEP_VOE_LOOPBACK_CTRL_LB_DMM_ENA;
        v |= VTSS_F_OAM_MEP_VOE_LOOPBACK_CTRL_LB_ISDX(cfg->loop_isdx_w);
        v |= VTSS_F_OAM_MEP_VOE_LOOPBACK_CTRL_LB_ISDX_P(cfg->loop_isdx_p);
        v |= VTSS_F_OAM_MEP_VOE_LOOPBACK_CTRL_LB_PORTNUM_P(cfg->loop_portidx_p);
        break;
    case VTSS_OAM_MIP:
        break;
    }
    SRVL_WR(VTSS_OAM_MEP_VOE_LOOPBACK_CTRL(voe_idx), v);

    /* CCM_CFG */
    v = 0;
    v |= B2B(cfg->mep_type == VTSS_OAM_DOWNMEP, VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_LM_PERIOD(srvl_oam_ccm_lm_period_index(cfg->ccm_lm.period)));
    switch (cfg->ccm.tx_seq_no_auto_upd_op) {
    case VTSS_OAM_AUTOSEQ_DONT:
        break;
    case VTSS_OAM_AUTOSEQ_INCREMENT_AND_UPDATE:
        if (cfg->mep_type == VTSS_OAM_DOWNMEP)
            v |= VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_SEQ_INCR_ENA | VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_SEQ_UPD_ENA;
        break;
    case VTSS_OAM_AUTOSEQ_UPDATE:
        if (cfg->mep_type == VTSS_OAM_DOWNMEP)
            v |= VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_SEQ_UPD_ENA;
        break;
    }
    v |= B2B(cfg->ccm.rx_seq_no_check, VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_RX_SEQ_CHK_ENA);
    v |= VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_PRIO(cfg->ccm.rx_priority);
    v |= VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_PERIOD(cfg->ccm.rx_period);
    v |= VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_MEGID_CHK_ENA | VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_MEPID_CHK_ENA;
    mask = VTSS_M_OAM_MEP_VOE_CCM_CFG_CCM_LM_PERIOD |
           VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_SEQ_INCR_ENA |
           VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_SEQ_UPD_ENA |
           VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_SEQ_INCR_ENA |
           VTSS_M_OAM_MEP_VOE_CCM_CFG_CCM_PRIO |
           VTSS_M_OAM_MEP_VOE_CCM_CFG_CCM_PERIOD |
           VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_MEGID_CHK_ENA |
           VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_MEPID_CHK_ENA;        
    SRVL_WRM(VTSS_OAM_MEP_VOE_CCM_CFG(voe_idx), v, mask);
    
    /* CCM_TX_SEQ_CFG */
    SRVL_WR(VTSS_OAM_MEP_VOE_CCM_TX_SEQ_CFG(voe_idx), cfg->ccm.tx_seq_no);

    /* CCM_RX_SEQ_CFG */
    SRVL_WR(VTSS_OAM_MEP_VOE_CCM_RX_SEQ_CFG(voe_idx), cfg->ccm.rx_seq_no);

    /* CCM_MEPID_CFG */
    SRVL_WR(VTSS_OAM_MEP_VOE_CCM_MEPID_CFG(voe_idx), VTSS_F_OAM_MEP_VOE_CCM_MEPID_CFG_CCM_MEPID(cfg->ccm.mepid));

    /* CCM_MEGID_CFG */
    p = &cfg->ccm.megid.data[47];    // MSB
    for (i=0; i<12; ++i, p-=4) {
        v = (*(p-3) << 24) |
            (*(p-2) << 16) |
            (*(p-1) <<  8) |
            (*(p-0));
        SRVL_WR(VTSS_OAM_MEP_VOE_CCM_MEGID_CFG(voe_idx, i), v);
    }

    if (cfg->voe_type == VTSS_OAM_VOE_SERVICE)
        SRVL_WR(VTSS_OAM_MEP_COMMON_VOE_CFG(voe_idx), B2B(cfg->mep_type == VTSS_OAM_UPMEP, VTSS_F_OAM_MEP_COMMON_VOE_CFG_UPMEP_VOE));    

    /* VOE_CNT_CTRL */
    v = 0;
    if (cfg->single_ended_lm.enable) {
        v |= B2B(cfg->single_ended_lm.count.yellow, VTSS_F_OAM_MEP_COMMON_VOE_CNT_CTRL_CNT_YELLOW_ENA);
        v |= VTSS_F_OAM_MEP_COMMON_VOE_CNT_CTRL_PRIO_CNT_MASK(cfg->single_ended_lm.count.priority_mask);
    } else if (cfg->ccm_lm.enable) {
        v |= B2B(cfg->ccm_lm.count.yellow, VTSS_F_OAM_MEP_COMMON_VOE_CNT_CTRL_CNT_YELLOW_ENA);
        v |= VTSS_F_OAM_MEP_COMMON_VOE_CNT_CTRL_PRIO_CNT_MASK(cfg->ccm_lm.count.priority_mask);
    }
    SRVL_WR(VTSS_OAM_MEP_COMMON_VOE_CNT_CTRL(voe_idx), v);
    
    /* VOE_MAP_CTRL */
    if (cfg->voe_type == VTSS_OAM_VOE_SERVICE) {
        v = 0;
        if (cfg->svc_to_path) {
            v |= VTSS_F_OAM_MEP_COMMON_VOE_MAP_CTRL_PATH_VOEID_ENA;
            v |= VTSS_F_OAM_MEP_COMMON_VOE_MAP_CTRL_PATH_VOEID(cfg->svc_to_path_idx_w);
            v |= VTSS_F_OAM_MEP_COMMON_VOE_MAP_CTRL_PATH_VOEID_P(cfg->svc_to_path_idx_p);
        }
        SRVL_WR(VTSS_OAM_MEP_COMMON_VOE_MAP_CTRL(voe_idx), v);
    }
    
    /* BASIC_CTRL. Comes last, as this is where we enable the VOE */
    switch (cfg->proc.dmac_check_type) {
    case VTSS_OAM_DMAC_CHECK_NONE:
        v = 0;
        break;
    case VTSS_OAM_DMAC_CHECK_UNICAST:
        v = VTSS_F_OAM_MEP_VOE_BASIC_CTRL_MAC_ADDR_ERR_REDIR_ENA | VTSS_F_OAM_MEP_VOE_BASIC_CTRL_RX_DMAC_CHK_SEL(0x01);
        break;
    case VTSS_OAM_DMAC_CHECK_MULTICAST:
        v = VTSS_F_OAM_MEP_VOE_BASIC_CTRL_MAC_ADDR_ERR_REDIR_ENA | VTSS_F_OAM_MEP_VOE_BASIC_CTRL_RX_DMAC_CHK_SEL(0x02);
        break;
    case VTSS_OAM_DMAC_CHECK_BOTH:
        v = VTSS_F_OAM_MEP_VOE_BASIC_CTRL_MAC_ADDR_ERR_REDIR_ENA | VTSS_F_OAM_MEP_VOE_BASIC_CTRL_RX_DMAC_CHK_SEL(0x03);
        break;
    }

    /* Hit Me Once is configured by calling @vtss_oam_voe_ccm_arm_hitme */

    if (!cfg->proc.copy_next_only) {
        v |= B2B(cfg->proc.copy_on_ccm_err,  VTSS_F_OAM_MEP_VOE_BASIC_CTRL_CCM_ERR_CPU_COPY_ENA);
        v |= B2B(cfg->proc.copy_on_dmac_err, VTSS_F_OAM_MEP_VOE_BASIC_CTRL_MAC_ADDR_ERR_REDIR_ENA);
    }
    v |= B2B(cfg->mep_type == VTSS_OAM_UPMEP  &&  cfg->upmep.discard_rx, VTSS_F_OAM_MEP_VOE_BASIC_CTRL_UPMEP_KILL_ALL_RX);
    v |= B2B(cfg->mep_type == VTSS_OAM_MIP, VTSS_F_OAM_MEP_VOE_BASIC_CTRL_MIP_UNICAST_ONLY_COPY_ENA | VTSS_F_OAM_MEP_VOE_BASIC_CTRL_VOE_AS_MIP_ENA);
    v |= VTSS_F_OAM_MEP_VOE_BASIC_CTRL_VOE_ENA;
    SRVL_WR(VTSS_OAM_MEP_VOE_BASIC_CTRL(voe_idx), v);
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_voe_ccm_arm_hitme(const vtss_oam_voe_idx_t voe_idx,
                                          const BOOL               enable)
{
    u32 bc = 0, mc = 0;
    const vtss_oam_voe_conf_t *cfg = &(vtss_state->oam_voe_conf[voe_idx]);
    
    if (cfg->proc.copy_next_only) {
        /* BASIC_CTRL */
        bc = B2B(cfg->proc.copy_on_ccm_more_than_one_tlv, VTSS_F_OAM_MEP_VOE_BASIC_CTRL_CPU_HITME_ONCE_CCM_TLV) |
             B2B(cfg->proc.copy_on_ccm_err, VTSS_F_OAM_MEP_VOE_BASIC_CTRL_CCM_ERR_CPU_HITME_ONCE) |
             B2B(cfg->ccm.copy_to_cpu, VTSS_F_OAM_MEP_VOE_BASIC_CTRL_CCM_NXT_CPU_HITME_ONCE);

        /* MEL_CTRL */
        mc = B2B(cfg->proc.copy_on_mel_too_low_err, VTSS_F_OAM_MEP_VOE_MEL_CTRL_CPU_HITME_ONCE_MEL_TOO_LOW | VTSS_F_OAM_MEP_VOE_MEL_CTRL_CPU_HITME_ONCE_CCM_MEL_TOO_LOW);
    }

    SRVL_WRM(VTSS_OAM_MEP_VOE_BASIC_CTRL(voe_idx), bc, bc);    
    SRVL_WRM(VTSS_OAM_MEP_VOE_MEL_CTRL(voe_idx),   mc, mc);    

    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_voe_ccm_set_rdi_flag(const vtss_oam_voe_idx_t voe_idx,
                                             const BOOL               flag)
{
    SRVL_WRM(VTSS_OAM_MEP_VOE_CCM_CFG(voe_idx), (flag) ? VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_TX_RDI : 0, VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_TX_RDI);
    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_ccm_status_get(const vtss_oam_voe_idx_t voe_idx,
                                       vtss_oam_ccm_status_t    *value)
{
    u32 v;
    
    SRVL_RD(VTSS_OAM_MEP_VOE_CCM_CFG(voe_idx), &v);
    value->period_err         = v & VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_PERIOD_ERR;
    value->priority_err       = v & VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_PRIO_ERR;
    value->zero_period_err    = v & VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_ZERO_PERIOD_ERR;
    value->rx_rdi             = v & VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_RX_RDI;
    value->mep_id_err         = v & VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_MEPID_ERR;
    value->meg_id_err         = v & VTSS_F_OAM_MEP_VOE_CCM_CFG_CCM_MEGID_ERR;
    value->loc                = VTSS_X_OAM_MEP_VOE_CCM_CFG_CCM_MISS_CNT(v) == 0x07;   // The CCM_LOC_DEFECT bit isn't wired properly in Serval 1

    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_pdu_seen_status_get(const vtss_oam_voe_idx_t   voe_idx,
                                            vtss_oam_pdu_seen_status_t *value)
{
    u32 i, v, mask;

    SRVL_RD(VTSS_OAM_MEP_VOE_OAM_RX_STICKY(voe_idx), &v);
    for (i=0; i<VTSS_OAM_GENERIC_OPCODE_CFG_CNT; ++i)
        value->generic_seen[i] = VTSS_X_OAM_MEP_VOE_OAM_RX_STICKY_GENERIC_RX_STICKY_MASK(v) & (1<<i);

    value->unknown_seen = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_UNK_OPCODE_RX_STICKY;
    value->ltm_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LTM_RX_STICKY;
    value->ltr_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LTR_RX_STICKY;
    value->lmm_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LMM_RX_STICKY;
    value->lmr_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LMR_RX_STICKY;
    value->lbm_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LBM_RX_STICKY;
    value->lbr_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LBR_RX_STICKY;
    value->dmm_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_DMM_RX_STICKY;
    value->dmr_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_DMR_RX_STICKY;
    value->one_dm_seen  = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_SM_RX_STICKY;
    value->ccm_seen     = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_CCM_RX_STICKY;
    value->ccm_lm_seen  = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_CCM_LM_RX_STICKY;
    
    mask = VTSS_M_OAM_MEP_VOE_OAM_RX_STICKY_GENERIC_RX_STICKY_MASK |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_UNK_OPCODE_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LTM_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LTR_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LMM_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LMR_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LBM_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LBR_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_DMM_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_DMR_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_SM_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_CCM_RX_STICKY |
           VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_CCM_LM_RX_STICKY;

    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_RX_STICKY(voe_idx), mask);

    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_proc_status_get(const vtss_oam_voe_idx_t voe_idx,
                                        vtss_oam_proc_status_t   *value)
{
    u32 v;

    SRVL_RD(VTSS_OAM_MEP_VOE_CCM_RX_SEQ_CFG(voe_idx), &v);
    value->rx_ccm_seq_no = (vtss_state->oam_voe_conf[voe_idx].mep_type == VTSS_OAM_UPMEP) ? v & 0xffff : v;

    SRVL_RD(VTSS_OAM_MEP_VOE_CCM_TX_SEQ_CFG(voe_idx), &value->tx_next_ccm_seq_no);
    SRVL_RD(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_CFG(voe_idx), &value->tx_next_lbm_transaction_id);
    SRVL_RD(VTSS_OAM_MEP_VOE_LBR_RX_TRANSID_CFG(voe_idx), &value->rx_lbr_transaction_id);

    value->rx_tst_seq_no = value->rx_lbr_transaction_id;  /* Same reg. on Serval-1 */

    SRVL_RD(VTSS_OAM_MEP_VOE_OAM_RX_STICKY(voe_idx), &v);
    value->rx_dmac_err_seen         = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_MAC_ADDR_ERR_STICKY;
    value->tx_meg_level_err_seen    = v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_MEP_EGR_BLOCK_STICKY;
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_RX_STICKY(voe_idx), VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_MAC_ADDR_ERR_STICKY | VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_MEP_EGR_BLOCK_STICKY);

    SRVL_RD(VTSS_OAM_MEP_VOE_STICKY(voe_idx), &v);
    value->rx_meg_level_err_seen    = v & VTSS_F_OAM_MEP_VOE_STICKY_OAM_MEL_STICKY;
    SRVL_WR(VTSS_OAM_MEP_VOE_STICKY(voe_idx), VTSS_F_OAM_MEP_VOE_STICKY_OAM_MEL_STICKY);
    
    SRVL_RD(VTSS_OAM_MEP_VOE_CCM_CFG(voe_idx), &v);
    value->rx_meg_level_err         = v & VTSS_F_OAM_MEP_VOE_CCM_CFG_OAM_MEL_ERR;

    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_voe_counter_update_serval_internal(const vtss_oam_voe_idx_t voe_idx, const u32 clear_mask)
{
    const u64 MAX_32U = 0xffffffffUL;
    u32 v;
    vtss_rc rc = VTSS_RC_OK;
    vtss_oam_voe_internal_counters_t *chipcnt = &vtss_state->oam_voe_internal[voe_idx].counters;

    /* CCM: For Serval we need to read a sticky bit and internally accumulate the value. */
    SRVL_RD(VTSS_OAM_MEP_VOE_OAM_RX_STICKY(voe_idx), &v);
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_RX_STICKY(voe_idx), VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_CCM_RX_SEQ_ERR_STICKY);
    v = (v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_CCM_RX_SEQ_ERR_STICKY) ? 1 : 0;
    vtss_cmn_counter_32_update(v + (chipcnt->ccm.rx_invalid_seq_no.prev & MAX_32U), &chipcnt->ccm.rx_invalid_seq_no, clear_mask & VTSS_OAM_CNT_CCM);

    /* LB + TST: Again, a sticky-accumulator: */
    SRVL_RD(VTSS_OAM_MEP_VOE_OAM_RX_STICKY(voe_idx), &v);
    SRVL_WR(VTSS_OAM_MEP_VOE_OAM_RX_STICKY(voe_idx), VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LBR_TRANSID_ERR_STICKY);
    v = (v & VTSS_F_OAM_MEP_VOE_OAM_RX_STICKY_LBR_TRANSID_ERR_STICKY) ? 1 : 0;
    if (vtss_state->oam_voe_conf[voe_idx].lb.enable  ||  (clear_mask & VTSS_OAM_CNT_LB)) {
        vtss_cmn_counter_32_update(v + (chipcnt->lb.rx_lbr_trans_id_err.prev  & MAX_32U), &chipcnt->lb.rx_lbr_trans_id_err, clear_mask & VTSS_OAM_CNT_LB);
    }
    else if (vtss_state->oam_voe_conf[voe_idx].tst.enable  ||  (clear_mask & VTSS_OAM_CNT_TST)) {
        vtss_cmn_counter_32_update(v + (chipcnt->tst.rx_tst_trans_id_err.prev & MAX_32U), &chipcnt->tst.rx_tst_trans_id_err, clear_mask & VTSS_OAM_CNT_TST);
    }

    return rc;
}

static vtss_rc srvl_oam_voe_counter_update_serval(const vtss_oam_voe_idx_t voe_idx)
{
    return srvl_oam_voe_counter_update_serval_internal(voe_idx, 0);
}

//#define CHIPREAD(reg, cnt, clear) { u32 v; SRVL_RD(reg, &v); vtss_cmn_counter_32_update(v, cnt, clear); }
#define CHIPREAD(reg, cnt, clear) {SRVL_RD(reg, &v); vtss_cmn_counter_32_update(v, cnt, clear); }
static vtss_rc srvl_oam_voe_counter_update_internal(const vtss_oam_voe_idx_t voe_idx, const u32 clear_mask)
{
    u32 v;
    u32 i, offset;
    BOOL doing_lb, doing_tst;
    vtss_rc rc = VTSS_RC_OK;
    vtss_oam_voe_internal_counters_t *chipcnt = &vtss_state->oam_voe_internal[voe_idx].counters;

    rc = srvl_oam_voe_counter_update_serval_internal(voe_idx, clear_mask);

    /* LM */

    switch (vtss_state->oam_voe_conf[voe_idx].voe_type) {
    case VTSS_OAM_VOE_SERVICE:
        offset = voe_idx * VTSS_PRIO_ARRAY_SIZE;
        for (i=0; i<VTSS_PRIO_ARRAY_SIZE; ++i) {
            CHIPREAD(VTSS_OAM_MEP_RX_VOE_PM_MEP_RX_FRM_CNT(offset+i), &chipcnt->lm.lm_count[i].rx, clear_mask & VTSS_OAM_CNT_LM);
            CHIPREAD(VTSS_OAM_MEP_TX_VOE_PM_MEP_TX_FRM_CNT(offset+i), &chipcnt->lm.lm_count[i].tx, clear_mask & VTSS_OAM_CNT_LM);
        }
        break;
    case VTSS_OAM_VOE_PORT:
        offset = (voe_idx - VTSS_OAM_PORT_VOE_BASE_IDX) * VTSS_PRIO_ARRAY_SIZE;
        for (i=0; i<VTSS_PRIO_ARRAY_SIZE; ++i) {
            CHIPREAD(VTSS_OAM_MEP_PORT_PM_PORT_RX_FRM_CNT(offset+i), &chipcnt->lm.lm_count[i].rx, clear_mask & VTSS_OAM_CNT_LM);
            CHIPREAD(VTSS_OAM_MEP_PORT_PM_PORT_TX_FRM_CNT(offset+i), &chipcnt->lm.lm_count[i].tx, clear_mask & VTSS_OAM_CNT_LM);
        }
        break;
    }

    /* CCM */

    CHIPREAD(VTSS_OAM_MEP_VOE_CCM_RX_VLD_FC_CNT(voe_idx),   &chipcnt->ccm.rx_valid_count,   clear_mask & VTSS_OAM_CNT_CCM);
    CHIPREAD(VTSS_OAM_MEP_VOE_CCM_RX_INVLD_FC_CNT(voe_idx), &chipcnt->ccm.rx_invalid_count, clear_mask & VTSS_OAM_CNT_CCM);

    VTSS_RC(doing_calculate(voe_idx, &doing_lb, &doing_tst));

    /* LB */

    if (doing_lb || (clear_mask & VTSS_OAM_CNT_LM)) {
        CHIPREAD(VTSS_OAM_MEP_VOE_LBR_RX_FRM_CNT(voe_idx),     &chipcnt->lb.rx_lbr, clear_mask & VTSS_OAM_CNT_LB);
        CHIPREAD(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_CFG(voe_idx), &chipcnt->lb.tx_lbm, clear_mask & VTSS_OAM_CNT_LB);
    }

    /* TST */

    if (doing_tst || (clear_mask & VTSS_OAM_CNT_TST)) {
        CHIPREAD(VTSS_OAM_MEP_VOE_LBR_RX_FRM_CNT(voe_idx),     &chipcnt->tst.rx_tst, clear_mask & VTSS_OAM_CNT_TST);
        CHIPREAD(VTSS_OAM_MEP_VOE_LBM_TX_TRANSID_CFG(voe_idx), &chipcnt->tst.tx_tst, clear_mask & VTSS_OAM_CNT_TST);
    }

    /* Selected */

    CHIPREAD(VTSS_OAM_MEP_VOE_RX_SEL_OAM_CNT(voe_idx), &chipcnt->sel.selected_frames.rx,    clear_mask & VTSS_OAM_CNT_SEL);
    CHIPREAD(VTSS_OAM_MEP_VOE_TX_SEL_OAM_CNT(voe_idx), &chipcnt->sel.selected_frames.tx,    clear_mask & VTSS_OAM_CNT_SEL);
    CHIPREAD(VTSS_OAM_MEP_VOE_RX_OAM_FRM_CNT(voe_idx), &chipcnt->sel.nonselected_frames.rx, clear_mask & VTSS_OAM_CNT_SEL);
    CHIPREAD(VTSS_OAM_MEP_VOE_TX_OAM_FRM_CNT(voe_idx), &chipcnt->sel.nonselected_frames.tx, clear_mask & VTSS_OAM_CNT_SEL);

    return rc;
}
#undef CHIPREAD

static vtss_rc srvl_oam_voe_counter_update(const vtss_oam_voe_idx_t voe_idx)
{
    return srvl_oam_voe_counter_update_internal(voe_idx, 0);
}

#define SET(v) value->v = chipcnt->v.value
static vtss_rc srvl_oam_voe_counter_get(const vtss_oam_voe_idx_t voe_idx,
                                        vtss_oam_voe_counter_t   *value)
{
    u32 v, i;
    vtss_rc rc = VTSS_RC_OK;
    const vtss_oam_voe_internal_counters_t *chipcnt = &vtss_state->oam_voe_internal[voe_idx].counters;

    /* Poll so we get the most recent values */
    rc = srvl_oam_voe_counter_update_internal(voe_idx, 0);

    /* LM */
    
    switch (vtss_state->oam_voe_conf[voe_idx].mep_type) {
    case VTSS_OAM_DOWNMEP:
        SRVL_RD(VTSS_OAM_MEP_VOE_CCM_CAP_TX_FCf(voe_idx), &value->lm.down_mep.tx_FCf);
        SRVL_RD(VTSS_OAM_MEP_VOE_CCM_CAP_RX_FCl(voe_idx), &value->lm.down_mep.rx_FCl);
        break;
    case VTSS_OAM_UPMEP:
        SRVL_RD(VTSS_OAM_MEP_COMMON_UPMEP_LM_CNT_VLD(voe_idx), &v);
        value->lm.up_mep.lmm_valid          = v & VTSS_F_OAM_MEP_COMMON_UPMEP_LM_CNT_VLD_LMM_RX_CNT_VLD;
        value->lm.up_mep.lmr_valid          = v & VTSS_F_OAM_MEP_COMMON_UPMEP_LM_CNT_VLD_LMR_RX_CNT_VLD;
        value->lm.up_mep.ccm_lm_valid       = v & VTSS_F_OAM_MEP_COMMON_UPMEP_LM_CNT_VLD_CCM_LM_RX_CNT_VLD;

        SRVL_RD(VTSS_OAM_MEP_VOE_CCM_CAP_TX_FCf(voe_idx), &value->lm.up_mep.lmm);
        SRVL_RD(VTSS_OAM_MEP_VOE_CCM_CAP_RX_FCl(voe_idx), &value->lm.up_mep.ccm_lm);
        SRVL_RD(VTSS_OAM_MEP_VOE_UPMEP_LMR_RX_LM_CNT(voe_idx), &value->lm.up_mep.lmr);

        SRVL_RD(VTSS_OAM_MEP_VOE_UPMEP_LM_CNT_STICKY(voe_idx), &v);
        value->lm.up_mep.lmm_sample_lost    = v & VTSS_F_OAM_MEP_VOE_UPMEP_LM_CNT_STICKY_LMM_RX_CNT_STICKY;
        value->lm.up_mep.lmr_sample_lost    = v & VTSS_F_OAM_MEP_VOE_UPMEP_LM_CNT_STICKY_LMR_RX_CNT_STICKY;
        value->lm.up_mep.ccm_lm_sample_lost = v & VTSS_F_OAM_MEP_VOE_UPMEP_LM_CNT_STICKY_CCM_LM_RX_CNT_STICKY;
        SRVL_WR(VTSS_OAM_MEP_VOE_UPMEP_LM_CNT_STICKY(voe_idx), 
            VTSS_F_OAM_MEP_VOE_UPMEP_LM_CNT_STICKY_LMM_RX_CNT_STICKY |
            VTSS_F_OAM_MEP_VOE_UPMEP_LM_CNT_STICKY_LMR_RX_CNT_STICKY |
            VTSS_F_OAM_MEP_VOE_UPMEP_LM_CNT_STICKY_CCM_LM_RX_CNT_STICKY);
        
        SRVL_RD(VTSS_OAM_MEP_VOE_CCM_RX_SEQ_CFG(voe_idx), &v);

        value->lm.up_mep.rx_ccm_lm_sample_seq_no = (v>>16) & 0x1f;
        value->lm.up_mep.rx_lmr_sample_seq_no    = (v>>21) & 0x1f;
        value->lm.up_mep.rx_lmm_sample_seq_no    = (v>>26) & 0x1f;

        break;
    case VTSS_OAM_MIP:
        rc = VTSS_RC_INV_STATE;
        break;
    }

    for (i=0; i<VTSS_PRIO_ARRAY_SIZE; ++i) {
        SET(lm.lm_count[i].rx);
        SET(lm.lm_count[i].tx);
    }

    /* CCM */
    
    SET(ccm.rx_valid_count);
    SET(ccm.rx_invalid_count);
    SET(ccm.rx_invalid_seq_no);

    /* LB */

    SET(lb.rx_lbr);
    SET(lb.tx_lbm);
    SET(lb.rx_lbr_trans_id_err);

    /* TST */

    SET(tst.rx_tst);
    SET(tst.tx_tst);
    SET(tst.rx_tst_trans_id_err);

    /* Selected */
 
    SET(sel.selected_frames.rx);
    SET(sel.selected_frames.tx);
    SET(sel.nonselected_frames.rx);
    SET(sel.nonselected_frames.tx);

    return rc;
}
#undef SET

static vtss_rc srvl_oam_voe_counter_clear(const vtss_oam_voe_idx_t voe_idx, const u32 mask)
{
    /* Poll & clear */
    return srvl_oam_voe_counter_update_internal(voe_idx, mask);
}

static vtss_rc srvl_oam_ipt_conf_get(u32 isdx, vtss_oam_ipt_conf_t *cfg)
{
    u32 v;
    SRVL_RD(VTSS_ANA_IPT_OAM_MEP_CFG(isdx), &v);
    cfg->enable          = v & VTSS_F_ANA_IPT_OAM_MEP_CFG_MEP_IDX_ENA;
    cfg->service_voe_idx = VTSS_X_ANA_IPT_OAM_MEP_CFG_MEP_IDX(v);
    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_ipt_conf_set(u32 isdx, const vtss_oam_ipt_conf_t *const cfg)
{
    SRVL_WR(VTSS_ANA_IPT_OAM_MEP_CFG(isdx),
        VTSS_F_ANA_IPT_OAM_MEP_CFG_MEP_IDX(cfg->service_voe_idx) |
        VTSS_F_ANA_IPT_OAM_MEP_CFG_MEP_IDX_P(cfg->service_voe_idx) |
        B2B(cfg->enable, VTSS_F_ANA_IPT_OAM_MEP_CFG_MEP_IDX_ENA));
    return VTSS_RC_OK;
}

static vtss_rc srvl_oam_voe_poll_1sec(void)
{
    /* Since all VOE counters are PDU counters (not byte counters) and Serval-1
     * has 75 VOEs in total, we can relax quite a bit; 32-bit wrap-around is
     * several minutes down the line, worst-case.
     */
    const u32 N = 1;

    vtss_oam_voe_idx_t *idx = &(vtss_state->oam_voe_poll_idx);  /* For readability */
    u32 i;

    for (i=0; i<N; ++i) {
        VTSS_RC(srvl_oam_voe_counter_update(*idx));
        *idx = (*idx == VTSS_OAM_VOE_CNT-1) ? 0 : *idx + 1;
    }

    return VTSS_RC_OK;
}


/*
 * OAM CIL debug. (AIL debug is in vtss_common.c)
 */

// D_COM: Debug COMmon; DR_COM: Debug Read COMmon. _I for Instance. Etc.
#define D_COM(pr, name)            srvl_debug_reg(pr, VTSS_OAM_MEP_COMMON_##name,                    "COMMON:"#name)
#define D_COM_I(pr, name, i)       srvl_debug_reg_inst(pr, VTSS_OAM_MEP_COMMON_##name(i),      (i),  "COMMON:"#name)
#define D_VOE_I(pr, name, i)       srvl_debug_reg_inst(pr, VTSS_OAM_MEP_VOE_##name(i),         (i),  "VOE:"#name)
#define D_VOE_II(pr, name, i1, i2) srvl_debug_reg_inst(pr, VTSS_OAM_MEP_VOE_##name((i1),(i2)), (i2), "VOE:"#name)
#define DR_COM(name, v)            { SRVL_RD(VTSS_OAM_MEP_COMMON_##name, &v); }
#define DR_COM_I(name, i, v)       { SRVL_RD(VTSS_OAM_MEP_COMMON_##name(i), &v); }
#define DR_VOE_I(name, i, v)       { SRVL_RD(VTSS_OAM_MEP_VOE_##name(i), &v); }
#define DR_VOE_II(name, i1, i2, v) { SRVL_RD(VTSS_OAM_MEP_VOE_##name((i1),(i2)), &v); }

static vtss_rc srvl_debug_oam(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    u32 i, k, v;
    char buf[10];

    srvl_debug_reg_header(pr, "VOP");
    D_COM(pr, MEP_CTRL);
    D_COM(pr, CPU_CFG);
    D_COM(pr, CPU_CFG_1);
    D_COM(pr, OAM_SDLB_CPU_COPY);
    for (i=0; i<8; ++i)
        D_COM_I(pr, OAM_GENERIC_CFG, i);
    for (i=0; i<7; ++i)
        D_COM_I(pr, CCM_PERIOD_CFG, i);
    D_COM(pr, CCM_CTRL);
    D_COM(pr, CCM_SCAN_STICKY);
    D_COM(pr, MASTER_INTR_CTRL);
    for (i=0; i<3; ++i)
        D_COM_I(pr, INTR, i);
    D_COM(pr, COMMON_MEP_MC_MAC_MSB);
    D_COM(pr, COMMON_MEP_MC_MAC_LSB);
    pr("\n");
    
    for (i=0; i<VTSS_OAM_VOE_CNT; ++i) {
        DR_VOE_I(BASIC_CTRL, i, v);
        if (info->full  ||  (v & VTSS_F_OAM_MEP_VOE_BASIC_CTRL_VOE_ENA)) {
            sprintf(buf, "VOE %u", i);
            srvl_debug_reg_header(pr, buf);
            D_COM_I(pr, VOE_CFG, i);
            D_COM_I(pr, UPMEP_LM_CNT_VLD, i);
            D_COM_I(pr, VOE_MAP_CTRL, i);
            D_COM_I(pr, VOE_CNT_CTRL, i);
            D_VOE_I(pr, BASIC_CTRL, i);
            D_VOE_I(pr, MEL_CTRL, i);
            D_VOE_I(pr, OAM_CPU_COPY_CTRL, i);
            D_VOE_I(pr, OAM_FWD_CTRL, i);
            D_VOE_I(pr, OAM_SDLB_CTRL, i);
            D_VOE_I(pr, OAM_CNT_OAM_CTRL, i);
            D_VOE_I(pr, OAM_CNT_DATA_CTRL, i);
            D_VOE_I(pr, MEP_UC_MAC_MSB, i);
            D_VOE_I(pr, MEP_UC_MAC_LSB, i);
            D_VOE_I(pr, OAM_HW_CTRL, i);
            D_VOE_I(pr, LOOPBACK_CTRL, i);
            D_VOE_I(pr, LBR_RX_FRM_CNT, i);
            D_VOE_I(pr, LBR_RX_TRANSID_CFG, i);
            D_VOE_I(pr, LBM_TX_TRANSID_UPDATE, i);
            D_VOE_I(pr, LBM_TX_TRANSID_CFG, i);
            D_VOE_I(pr, CCM_CFG, i);
            D_VOE_I(pr, CCM_RX_VLD_FC_CNT, i);
            D_VOE_I(pr, CCM_RX_INVLD_FC_CNT, i);
            D_VOE_I(pr, CCM_CAP_TX_FCf, i);
            D_VOE_I(pr, CCM_CAP_RX_FCl, i);
            D_VOE_I(pr, CCM_TX_SEQ_CFG, i);
            D_VOE_I(pr, CCM_RX_SEQ_CFG, i);
            D_VOE_I(pr, CCM_MEPID_CFG, i);
            for (k=0; k<12; ++k)
                D_VOE_II(pr, CCM_MEGID_CFG, i, 12-k);
            D_VOE_I(pr, OAM_RX_STICKY, i);
            D_VOE_I(pr, STICKY, i);
            D_VOE_I(pr, UPMEP_LM_CNT_STICKY, i);
            D_VOE_I(pr, UPMEP_LMR_RX_LM_CNT, i);
            D_VOE_I(pr, INTR_ENA, i);
            D_VOE_I(pr, RX_SEL_OAM_CNT, i);
            D_VOE_I(pr, TX_SEL_OAM_CNT, i);
            D_VOE_I(pr, RX_OAM_FRM_CNT, i);
            D_VOE_I(pr, TX_OAM_FRM_CNT, i);
            // Not included: PORT_OM, RX_VOE_OM, TX_VOE_PM
            //   -- too much output
        }
    }
    pr("\n");

    return VTSS_RC_OK;
}

#undef D_COM
#undef D_COM_I
#undef D_VOE_I
#undef D_VOE_II
#undef DR_COM
#undef DR_COM_I
#undef DR_VOE_I
#undef DR_VOE_II

#endif /* VTSS_FEATURE_OAM */

/* ================================================================= *
 *  MPLS / MPLS-TP / MPLS-OAM
 * ================================================================= */

#if defined (VTSS_FEATURE_MPLS)
static vtss_rc srvl_debug_mpls(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    // srvl_debug_print_header(pr, "MPLS Core");
    VTSS_RC(srvl_debug_is0_all(pr, info));
    VTSS_RC(srvl_debug_es0_all(pr, info));
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_mpls_oam(const vtss_debug_printf_t pr,
                                   const vtss_debug_info_t   *const info)
{
    srvl_debug_print_header(pr, "MPLS OAM");
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_MPLS */

/* ================================================================= *
 *  Miscellaneous
 * ================================================================= */

static vtss_rc srvl_reg_read(const vtss_chip_no_t chip_no, const u32 addr, u32 * const value)
{
    return srvl_rd(addr, value);
}

static vtss_rc srvl_reg_write(const vtss_chip_no_t chip_no, const u32 addr, const u32 value)
{
    return srvl_wr(addr, value);
}

static vtss_rc srvl_chip_id_get(vtss_chip_id_t *const chip_id)
{
    u32 value;

    SRVL_RD(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID, &value);

    if (value == 0 || value == 0xffffffff) {
        VTSS_E("CPU interface error, chipid: 0x%08x", value);
        return VTSS_RC_ERROR;
    }

    chip_id->part_number = VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_PART_ID(value);
    chip_id->revision = VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_REV_ID(value);

    return VTSS_RC_OK;
}

static vtss_rc srvl_ptp_event_poll(vtss_ptp_event_type_t *ev_mask)
{
    u32 sticky, mask;

    /* PTP events */
    *ev_mask = 0;
    SRVL_RD(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, &sticky);
    SRVL_WR(VTSS_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT, sticky);
    SRVL_RD(VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG, &mask);
    mask |= VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY; /* CLK ADJ event has no enable bit - do not generate interrupt */
    sticky &= mask;      /* Only handle enabled sources */

    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_SYNC_STAT) ? VTSS_PTP_SYNC_EV : 0;
    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_EXT_SYNC_CURRENT_TIME_STICKY(1)) ? VTSS_PTP_EXT_SYNC_EV : 0;
    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_EXT_SYNC_CURRENT_TIME_STICKY(2)) ? VTSS_PTP_EXT_1_SYNC_EV : 0;
    *ev_mask |= (sticky & VTSS_F_DEVCPU_GCB_PTP_STAT_PTP_EVT_STAT_CLK_ADJ_UPD_STICKY) ? VTSS_PTP_CLK_ADJ_EV : 0;
    VTSS_D("sticky: 0x%x, ev_mask 0x%x", sticky, *ev_mask);

    return VTSS_RC_OK;
}

static vtss_rc srvl_ptp_event_enable(vtss_ptp_event_type_t ev_mask, BOOL enable)
{
    /* PTP masks */

    if (ev_mask & VTSS_PTP_SYNC_EV) {
        SRVL_WRM(VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG,
                enable ? VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_SYNC_STAT_ENA : 0,
                VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_SYNC_STAT_ENA);
    }
    if (ev_mask & VTSS_PTP_EXT_SYNC_EV) {
        SRVL_WRM(VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG,
                enable ? VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_EXT_SYNC_CURRENT_TIME_ENA(1) : 0,
                VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_EXT_SYNC_CURRENT_TIME_ENA(1));
    }
    if (ev_mask & VTSS_PTP_EXT_1_SYNC_EV) {
        SRVL_WRM(VTSS_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG,
                 enable ? VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_EXT_SYNC_CURRENT_TIME_ENA(2) : 0,
                 VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_SYNC_INTR_ENA_CFG_EXT_SYNC_CURRENT_TIME_ENA(2));
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_intr_cfg(const u32  intr_mask, const BOOL polarity, const BOOL enable)
{
    return VTSS_RC_OK;
}

static vtss_rc srvl_intr_pol_negation(void)
{
    return VTSS_RC_OK;
}

static vtss_rc srvl_poll_1sec(void)
{
#if defined(VTSS_FEATURE_EVC)
    VTSS_RC(srvl_evc_poll_1sec());
#endif /* VTSS_FEATURE_EVC */
#if defined(VTSS_FEATURE_OAM)
    VTSS_RC(srvl_oam_voe_poll_1sec());
#endif /* VTSS_FEATURE_OAM */
    return VTSS_RC_OK;
}

/* =================================================================
 *  Miscellaneous - GPIO
 * =================================================================*/

static vtss_rc srvl_gpio_mode(const vtss_chip_no_t   chip_no,
                              const vtss_gpio_no_t   gpio_no,
                              const vtss_gpio_mode_t mode)
{
    u32 mask = VTSS_BIT(gpio_no), alt_0 = 0, alt_1 = 0;

    SRVL_WRM_CLR(VTSS_DEVCPU_GCB_GPIO_GPIO_INTR_ENA, mask); /* Disable IRQ */
    switch (mode) {
    case VTSS_GPIO_OUT:
    case VTSS_GPIO_IN:
    case VTSS_GPIO_IN_INT:
        SRVL_WRM_CTL(VTSS_DEVCPU_GCB_GPIO_GPIO_OE, mode == VTSS_GPIO_OUT, mask);
        break;
    case VTSS_GPIO_ALT_0:
        alt_0 = mask;
        break;
    case VTSS_GPIO_ALT_1:
        alt_1 = mask;
        break;
    case VTSS_GPIO_ALT_2:
        alt_0 = mask;
        alt_1 = mask;
        break;
    default:
        VTSS_E("illegal mode");
        return VTSS_RC_ERROR;
    }
    SRVL_WRM(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), alt_0, mask);
    SRVL_WRM(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(1), alt_1, mask);
    if (mode == VTSS_GPIO_IN_INT)
        SRVL_WRM_SET(VTSS_DEVCPU_GCB_GPIO_GPIO_INTR_ENA, mask);
    return VTSS_RC_OK;
}

static vtss_rc srvl_gpio_read(const vtss_chip_no_t  chip_no,
                              const vtss_gpio_no_t  gpio_no,
                              BOOL                  *const value)
{
    u32 val, mask = VTSS_BIT(gpio_no);

    SRVL_RD(VTSS_DEVCPU_GCB_GPIO_GPIO_IN, &val);
    *value = VTSS_BOOL(val & mask);
    return VTSS_RC_OK;
}

static vtss_rc srvl_gpio_write(const vtss_chip_no_t  chip_no,
                               const vtss_gpio_no_t  gpio_no,
                               const BOOL            value)
{
    u32 mask = VTSS_BIT(gpio_no);

    if (value) {
        SRVL_WR(VTSS_DEVCPU_GCB_GPIO_GPIO_OUT_SET, mask);
    } else {
        SRVL_WR(VTSS_DEVCPU_GCB_GPIO_GPIO_OUT_CLR, mask);
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_sgpio_event_poll(const vtss_chip_no_t     chip_no,
                                     const vtss_sgpio_group_t group,
                                     const u32                bit,
                                     BOOL                     *const events)
{
    u32 i, val;

    SRVL_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INT_REG(bit), &val);
    SRVL_WR(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INT_REG(bit), val);  /* Clear pending */

    /* Setup serial IO port enable register */
    for (i = 0; i < 32; i++) {
        events[i] = VTSS_BOOL(val & (1<<i));
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_sgpio_event_enable(const vtss_chip_no_t     chip_no,
                                       const vtss_sgpio_group_t group,
                                       const u32                port,
                                       const u32                bit,
                                       const BOOL               enable)
{
    /* The code commented out below is because for Serval we (for now) accept that the interrupt is level triggered and we can clear pending.
       if we at a later stage what to use the interrupt as done with Luton26, these has to be "reinserted" and a function like l26_poll_1sec implemented */
    // u32 data, pol;
    u32 i;

    if (enable) {
        /* SRVL_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INPUT_DATA(bit), &data); */
        /* SRVL_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INT_POL(bit), &pol); */
        /* pol = ~pol;  */   /* '0' means interrupt when input is one */
        /* data &= pol; */   /* Now data '1' means active interrupt */
        /*if (!(data & 1<<port)) {*/
        /* Only enable if not active interrupt - as interrupt pending cannot be cleared */
        SRVL_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_INT_ENA, 1<<port, 1<<port);
       /*}*/
        SRVL_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG,
                 VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit),
                 VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit));
    }
    else {
        SRVL_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_INT_ENA, 0, 1<<port);
        for (i = 0; i < 32; ++i) {
            if (vtss_state->sgpio_event_enable[group].enable[i][bit])
                break;
        }
        if (i == 32)
            SRVL_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG, 0,
                     VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA(1<<bit));
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_sgpio_conf_set(const vtss_chip_no_t     chip_no,
                                   const vtss_sgpio_group_t group,
                                   const vtss_sgpio_conf_t  *const conf)
{
    u32 i, port, val = 0, bmode[2], bit_idx;

    /* Setup serial IO port enable register */
    for (port = 0; port < 32; port++) {
        if (conf->port_conf[port].enabled)
            val |= VTSS_BIT(port);
    }
    SRVL_WR(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_ENABLE, val);

    /* Setup general configuration register
     *
     * The burst gap is 0x1f(33.55ms)
     * The load signal is active low
     * The auto burst repeat function is on
     * The SIO reverse output is off */
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
    SRVL_WRM(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG,
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BMODE_0(bmode[0]) |
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BMODE_1(bmode[1]) |
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BURST_GAP(0x1F) |
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_PORT_WIDTH(conf->bit_count - 1) |
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_AUTO_REPEAT,
             ~VTSS_M_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_INT_ENA);

    /* Setup the serial IO clock frequency - 12.5MHz (0x14) */
    SRVL_WR(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CLOCK, 0x14);

    /* Configuration of output data values
     * The placement of the source select bits for each output bit in the register:
     * Output bit0 : (2 downto 0)
     * Output bit1 : (5 downto 3)
     * Output bit2 : (8 downto 6)
     * Output bit3 : (11 downto 9) */
    for (port = 0; port < 32; port++) {
        for (val = 0, bit_idx = 0; bit_idx < 4; bit_idx++) {
            /* Set output bit n */
            val |= VTSS_ENCODE_BITFIELD(conf->port_conf[port].mode[bit_idx], bit_idx * 3, 3);
        }
        SRVL_WR(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_CONFIG(port), val);
    }

    return VTSS_RC_OK;
}

static vtss_rc srvl_sgpio_read(const vtss_chip_no_t     chip_no,
                               const vtss_sgpio_group_t group,
                               vtss_sgpio_port_data_t   data[VTSS_SGPIO_PORTS])
{
    u32 i, port, value;

    for (i = 0; i < 4; i++) {
        SRVL_RD(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INPUT_DATA(i), &value);
        for (port = 0; port < 32; port++) {
            data[port].value[i] = VTSS_BOOL(value & (1 << port));
        }
    }

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  Miscellaneous - Debug print
 * ================================================================= */

static void srvl_debug_tgt(const vtss_debug_printf_t pr, const char *name, u32 to)
{
    u32 tgt = ((to >> 16) & 0x3f);
    pr("%-10s  0x%02x (%u)\n", name, tgt, tgt);
}

#define SRVL_DEBUG_TGT(pr, name) srvl_debug_tgt(pr, #name, VTSS_TO_##name)
#define SRVL_DEBUG_GPIO(pr, addr, name) srvl_debug_reg(pr, VTSS_DEVCPU_GCB_GPIO_GPIO_##addr, "GPIO_"name)
#define SRVL_DEBUG_SIO(pr, addr, name) srvl_debug_reg(pr, VTSS_DEVCPU_GCB_SIO_CTRL_SIO_##addr, "SIO_"name)
#define SRVL_DEBUG_SIO_INST(pr, addr, i, name) srvl_debug_reg_inst(pr, VTSS_DEVCPU_GCB_SIO_CTRL_SIO_##addr, i, "SIO_"name)

static vtss_rc srvl_debug_misc(const vtss_debug_printf_t pr,
                               const vtss_debug_info_t   *const info)
{
    u32  port, i;
    char buf[32];
    
    pr("Name        Target\n");
    SRVL_DEBUG_TGT(pr, DEVCPU_ORG);
    SRVL_DEBUG_TGT(pr, SYS);
    SRVL_DEBUG_TGT(pr, REW);
    SRVL_DEBUG_TGT(pr, ES0);
    SRVL_DEBUG_TGT(pr, S1);
    SRVL_DEBUG_TGT(pr, S2);
    SRVL_DEBUG_TGT(pr, DEVCPU_GCB);
    SRVL_DEBUG_TGT(pr, DEVCPU_QS);
    SRVL_DEBUG_TGT(pr, HSIO);
    SRVL_DEBUG_TGT(pr, IS0);
    SRVL_DEBUG_TGT(pr, OAM_MEP);
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        sprintf(buf, "DEV_%u", port);
        srvl_debug_tgt(pr, buf, VTSS_TO_DEV(port));
    }
    pr("\n");
    SRVL_DEBUG_TGT(pr, QSYS);
    SRVL_DEBUG_TGT(pr, ANA);

    srvl_debug_reg_header(pr, "GPIOs");
    SRVL_DEBUG_GPIO(pr, OUT, "OUT");
    SRVL_DEBUG_GPIO(pr, IN, "IN");
    SRVL_DEBUG_GPIO(pr, OE, "OE");
    SRVL_DEBUG_GPIO(pr, INTR, "INTR");
    SRVL_DEBUG_GPIO(pr, INTR_ENA, "INTR_ENA");
    SRVL_DEBUG_GPIO(pr, INTR_IDENT, "INTR_IDENT");
    SRVL_DEBUG_GPIO(pr, ALT(0), "ALT_0");
    SRVL_DEBUG_GPIO(pr, ALT(1), "ALT_1");
    pr("\n");
    
    srvl_debug_reg_header(pr, "SGPIO");
    for (i = 0; i < 4; i++)
        SRVL_DEBUG_SIO_INST(pr, INPUT_DATA(i), i, "INPUT_DATA");
    for (i = 0; i < 4; i++)
        SRVL_DEBUG_SIO_INST(pr, INT_POL(i), i, "INT_POL");
    SRVL_DEBUG_SIO(pr, PORT_INT_ENA, "PORT_INT_ENA");
    for (i = 0; i < 32; i++)
        SRVL_DEBUG_SIO_INST(pr, PORT_CONFIG(i), i, "PORT_CONFIG");
    SRVL_DEBUG_SIO(pr, PORT_ENABLE, "PORT_ENABLE");
    SRVL_DEBUG_SIO(pr, CONFIG, "CONFIG");
    SRVL_DEBUG_SIO(pr, CLOCK, "CLOCK");
    for (i = 0; i < 4; i++)
        SRVL_DEBUG_SIO_INST(pr, INT_REG(i), i, "INT_REG");
    pr("\n");
    
    return VTSS_RC_OK;
}

static vtss_rc srvl_debug_info_print(const vtss_debug_printf_t pr,
                                     const vtss_debug_info_t   *const info)
{
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MISC, srvl_debug_misc, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PORT, srvl_debug_port, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PORT_CNT, srvl_debug_port_cnt, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_VLAN, srvl_debug_vlan, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PVLAN, srvl_debug_pvlan, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MAC_TABLE, srvl_debug_mac_table, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_ACL, srvl_debug_acl, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_VXLAT, srvl_debug_vxlat, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_QOS, srvl_debug_qos, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_AGGR, srvl_debug_aggr, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_STP, srvl_debug_stp, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MIRROR, srvl_debug_mirror, pr, info));
#if defined(VTSS_FEATURE_EVC)
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_EVC, srvl_debug_evc, pr, info));
#endif /* VTSS_FEATURE_EVC */
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_PACKET, srvl_debug_pkt, pr, info));
#if defined(VTSS_FEATURE_TIMESTAMP)
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_TS, srvl_debug_ts, pr, info));
#endif /* VTSS_FEATURE_TIMESTAMP */
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_WM,   srvl_debug_wm, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_IPMC, srvl_debug_ipmc, pr, info));
#if defined(VTSS_FEATURE_OAM)
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_OAM, srvl_debug_oam, pr, info));
#endif /* VTSS_FEATURE_OAM */
#if defined (VTSS_FEATURE_MPLS)
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MPLS, srvl_debug_mpls, pr, info));
    VTSS_RC(vtss_debug_print_group(VTSS_DEBUG_GROUP_MPLS_OAM, srvl_debug_mpls_oam, pr, info));
#endif
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
// Watermark values encoded at little odd for values larger than 2048
static u16 WmEnc(u16 value) {
    if (value >= 2048) {
        return 2048 + value/16;
    }
    return value;
}

static vtss_rc srvl_buf_conf_set(void)
{
    u32 port_no, port, q, dp;
    u32 buf_q_rsrv_i, buf_q_rsrv_e, ref_q_rsrv_i, ref_q_rsrv_e, buf_prio_shr_i[8], buf_prio_shr_e[8], ref_prio_shr_i[8], ref_prio_shr_e[8];
    u32 buf_p_rsrv_i, buf_p_rsrv_e, ref_p_rsrv_i, ref_p_rsrv_e, buf_col_shr_i, buf_col_shr_e, ref_col_shr_i, ref_col_shr_e;
    u32 buf_prio_rsrv, ref_prio_rsrv, guaranteed, q_rsrv_mask, prio_mem, prio_ref, oversubscription_factor;

    /* SYS::RES_CFG : 1024 watermarks for 1024000 byte shared buffer, unit is 60 byte */
    /* Is divided into 4 resource consumptions, ingress and egress memory (BUF) and ingress and egress frame reference (REF) blocks */   
    /* Queue reserved (q_rsrv) : starts at offset 0 within in each BUF and REF   */
    /* Prio shared (prio_shr)  : starts at offset 216 within in each BUF and REF */
    /* Port reserved (p_rsrv)  : starts at offset 224 within in each BUF and REF */
    /* Colour shared (col_shr) : starts at offset 254 within in each BUF and REF */

    /* Buffer values are in BYTES */ 
    buf_q_rsrv_i = 512;    /* Guarantees at least 1 MTU  */
    buf_p_rsrv_i = 0;      /* No additional ingress guarantees   */
    buf_q_rsrv_e = 3000;   /* Guarantees all QoS classes some space */
    buf_p_rsrv_e = 40960;  /* Guarantees a space to the egress ports */
    buf_col_shr_i = SRVL_BUFFER_MEMORY; /* Coloring - disabled for now */
    buf_col_shr_e = SRVL_BUFFER_MEMORY; /* Coloring - disabled for now */
    buf_prio_rsrv = 3072;  /* In the shared area, each priority is cut off 3kB before the others */

    /* Reference values in NUMBER of FRAMES */
    ref_q_rsrv_e = 10;     /* Number of frames that can be pending at each egress queue   */
    ref_q_rsrv_i = 10;     /* Number of frames that can be pending at each ingress queue  */
    ref_p_rsrv_e = 100;    /* Number of frames that can be pending shared between the QoS classes at egress */
    ref_p_rsrv_i = 20;     /* Number of frames that can be pending shared between the QoS classes at ingress */
    ref_col_shr_i = SRVL_BUFFER_REFERENCE; /* Coloring - disabled  */
    ref_col_shr_e = SRVL_BUFFER_REFERENCE; /* Coloring - disabled  */
    ref_prio_rsrv = 50;   /* Number of frames that can be pending for each class */

    /* The memory is oversubsrcribed by this factor (factor 1 = 100) */
    /* Oversubscription is possible (in some degree) because it's rare that all ports use their reserved space at the same time */
    oversubscription_factor = 100; /* No oversubscription */

    /* Note, the shared reserved space (buf_prio_shr_i, ref_prio_shr_i, buf_prio_shr_e, ref_prio_shr_e) is calulated based on above */

    /* The number of supported queues is given through the state structure                           */
    /* The supported queues (lowest to higest) are givin reserved buffer space as specified above.   */
    /* Frames in remaining queues (if any) are not getting any reserved space - but are allowed in the system.*/
    q_rsrv_mask = 0xff >> (8 - vtss_state->qos_conf.prios); 

    /* **************************************************  */
    /* BELOW, everything is calculated based on the above. */
    /* **************************************************  */

    /* Find the amount of guaranteeed space per port */
    guaranteed = buf_p_rsrv_i+buf_p_rsrv_e;
    for (q=0; q<VTSS_PRIOS; q++) {
        if (q_rsrv_mask & (1<<q)) 
            guaranteed+=(buf_q_rsrv_i+buf_q_rsrv_e);
    }

    prio_mem = (oversubscription_factor * SRVL_BUFFER_MEMORY)/100 - (vtss_state->port_count+1)*guaranteed;

    /* Find the amount of guaranteeed frame references */
    guaranteed = ref_p_rsrv_i+ref_p_rsrv_e;
    for (q=0; q<VTSS_PRIOS; q++) {
        if (q_rsrv_mask & (1<<q)) {
            guaranteed+=(ref_q_rsrv_i+ref_q_rsrv_e);
        }
    }
    prio_ref = SRVL_BUFFER_REFERENCE - (vtss_state->port_count+1)*guaranteed;

    for (q = VTSS_PRIOS-1; ; q--) {
        buf_prio_shr_i[q] = prio_mem;
        ref_prio_shr_i[q] = prio_ref;
        buf_prio_shr_e[q] = prio_mem;
        ref_prio_shr_e[q] = prio_ref;

        if (q_rsrv_mask & (1<<q)) {
            prio_mem -= buf_prio_rsrv;
            prio_ref -= ref_prio_rsrv;
        }
        if (q==0) break;
    }
 
    /* Configure reserved space for all QoS classes per port */
    for (port_no = 0; port_no <= vtss_state->port_count; port_no++) {
        if (port_no == vtss_state->port_count) {
            port = VTSS_CHIP_PORT_CPU;
        } else {
            port = VTSS_CHIP_PORT(port_no);
        }
        for (q = 0; q < VTSS_PRIOS; q++) {
            if (q_rsrv_mask&(1<<q)) {
                SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(port * VTSS_PRIOS + q + 0),   WmEnc(buf_q_rsrv_i/SRVL_BUFFER_CELL_SZ));
                SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(port * VTSS_PRIOS + q + 256), WmEnc(ref_q_rsrv_i));
                SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(port * VTSS_PRIOS + q + 512), WmEnc(buf_q_rsrv_e/SRVL_BUFFER_CELL_SZ));
                SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(port * VTSS_PRIOS + q + 768), WmEnc(ref_q_rsrv_e));
            }
        }
    }

    /* Configure shared space for all QoS classes */
    for (q = 0; q < VTSS_PRIOS; q++) {
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG((q + 216 + 0)),   WmEnc(buf_prio_shr_i[q]/SRVL_BUFFER_CELL_SZ));
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG((q + 216 + 256)), WmEnc(ref_prio_shr_i[q]));
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG((q + 216 + 512)), WmEnc(buf_prio_shr_e[q]/SRVL_BUFFER_CELL_SZ));
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG((q + 216 + 768)), WmEnc(ref_prio_shr_e[q]));
    }

    /* Configure reserved space for all ports */
    for (port_no = 0; port_no <= vtss_state->port_count; port_no++) {
        if (port_no == vtss_state->port_count) {
            port = VTSS_CHIP_PORT_CPU;
        } else {
            port = VTSS_CHIP_PORT(port_no);
        }
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(port + 224 +   0), WmEnc(buf_p_rsrv_i/SRVL_BUFFER_CELL_SZ));
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(port + 224 + 256), WmEnc(ref_p_rsrv_i));
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(port + 224 + 512), WmEnc(buf_p_rsrv_e/SRVL_BUFFER_CELL_SZ));
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(port + 224 + 768), WmEnc(ref_p_rsrv_e));
    }

    /* Configure shared space for  both DP levels (green:0 yellow:1) */
    for (dp = 0; dp < 2; dp++) {
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(dp + 254 +   0), WmEnc(buf_col_shr_i/SRVL_BUFFER_CELL_SZ));
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(dp + 254 + 256), ref_col_shr_i);
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(dp + 254 + 512), WmEnc(buf_col_shr_e/SRVL_BUFFER_CELL_SZ));
        SRVL_WR(VTSS_QSYS_RES_CTRL_RES_CFG(dp + 254 + 768), ref_col_shr_e);
    }
    return VTSS_RC_OK;
}

static vtss_rc srvl_port_map_set(void)
{
    /* We only need to setup the no of avail pgids */
    vtss_state->pgid_count = (VTSS_PGID_LUTON26 - VTSS_CHIP_PORTS + vtss_state->port_count);

    /* And then claim some for flooding */
    VTSS_RC(srvl_flood_conf_set());

    /* Setup flooding PGIDs */
    SRVL_WR(VTSS_ANA_ANA_FLOODING, 
            VTSS_F_ANA_ANA_FLOODING_FLD_UNICAST(PGID_UC) |
            VTSS_F_ANA_ANA_FLOODING_FLD_BROADCAST(PGID_MC) |
            VTSS_F_ANA_ANA_FLOODING_FLD_MULTICAST(PGID_MC));
    
    SRVL_WR(VTSS_ANA_ANA_FLOODING_IPMC, 
            VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC4_CTRL(PGID_MC) |
            VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC4_DATA(PGID_MCIPV4) |
            VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC6_CTRL(PGID_MC) |
            VTSS_F_ANA_ANA_FLOODING_IPMC_FLD_MC6_DATA(PGID_MCIPV6));

    /* Setup WM (depends on the port map)  */
    VTSS_RC(srvl_buf_conf_set());
       
    /* Setup Rx CPU queue system */
    VTSS_RC(srvl_rx_conf_set()); 

    return VTSS_RC_OK;
}   

#define SRVL_API_VERSION 1

static vtss_rc srvl_restart_conf_set(void)
{
    SRVL_WR(VTSS_DEVCPU_GCB_CHIP_REGS_GPR, vtss_cmn_restart_value_get());

    return VTSS_RC_OK;
}

static vtss_rc srvl_init_conf_set(void)
{
    vtss_vid_t vid;
    u32        value, i, port, pcp, dei;

    VTSS_D("enter");

#if VTSS_OPT_VCORE_III
    // For register access checking.
    vtss_state->reg_check.addr = VTSS_DEVCPU_ORG_ORG_ERR_CNTS;
#endif

    /* Read chip ID to check CPU interface */
    VTSS_RC(srvl_chip_id_get(&vtss_state->chip_id));
    VTSS_I("chip_id: 0x%04x, revision: 0x%04x", 
           vtss_state->chip_id.part_number, vtss_state->chip_id.revision);

    /* Read restart type */
    SRVL_RD(VTSS_DEVCPU_GCB_CHIP_REGS_GPR, &value);
    vtss_state->init_conf.warm_start_enable = 0; /* Warm start not supported */
    VTSS_RC(vtss_cmn_restart_update(value));

    /* Initialize memories */
    SRVL_RD(VTSS_SYS_SYSTEM_RESET_CFG, &value);
    if (!(value & VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_ENA)) {
        /* Avoid initialization if already done by VRAP strapping */
        SRVL_WR(VTSS_SYS_SYSTEM_RESET_CFG,
                VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_ENA | VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_INIT);
        for (i = 0; ; i++) {
            VTSS_MSLEEP(1); /* MEM_INIT should clear after appx. 22us */
            SRVL_RD(VTSS_SYS_SYSTEM_RESET_CFG, &value);
            if (value & VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_INIT) {
                if (i == 10) {
                    VTSS_E("Memory initialization error, SYS::RESET_CFG: 0x%08x", value);
                    return VTSS_RC_ERROR;
                }
            } else {
                break;
            }
        }
        
        /* Enable switch core */
        SRVL_WRM_SET(VTSS_SYS_SYSTEM_RESET_CFG, VTSS_F_SYS_SYSTEM_RESET_CFG_CORE_ENA);
    }

    /* Clear MAC table */
    SRVL_WR(VTSS_ANA_ANA_TABLES_MACACCESS,
            VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_TABLE_CLEAR));

    /* Clear VLAN table */
    SRVL_WR(VTSS_ANA_ANA_TABLES_VLANACCESS,
            VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(VLAN_CMD_TABLE_CLEAR));

    /* Setup chip ports */
    for (port = 0; port <= VTSS_CHIP_PORTS; port++) {
        /* Clear port counters */
        SRVL_WR(VTSS_SYS_SYSTEM_STAT_CFG,
                VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_CLEAR_SHOT(0x7) |
                VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_VIEW(port));

        if (port == VTSS_CHIP_PORT_CPU) {
            /* Setup the CPU port as VLAN aware to support switching frames based on tags */
            SRVL_WR(VTSS_ANA_PORT_VLAN_CFG(port), 
                    VTSS_F_ANA_PORT_VLAN_CFG_VLAN_AWARE_ENA  |
                    VTSS_F_ANA_PORT_VLAN_CFG_VLAN_POP_CNT(1) |
                    VTSS_F_ANA_PORT_VLAN_CFG_VLAN_VID(1));

            /* Disable learning (only RECV_ENA must be set) */
            SRVL_WR(VTSS_ANA_PORT_PORT_CFG(port), VTSS_F_ANA_PORT_PORT_CFG_RECV_ENA);
        } else {
            /* Default VLAN port configuration */
            VTSS_RC(srvl_vlan_port_conf_apply(port, 
                                              &vtss_state->vlan_port_conf[VTSS_PORT_NO_START]));
        }
    } /* Port loop */

#if defined(VTSS_FEATURE_EVC)
    /* Clear SDX counters */
    for (i = 0; i <= vtss_state->sdx_info.max_count; i++) {
        SRVL_WR(VTSS_SYS_SYSTEM_STAT_CFG,
                VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_CLEAR_SHOT(0x18) |
                VTSS_F_SYS_SYSTEM_STAT_CFG_STAT_VIEW(i));
    }
#endif /* VTSS_FEATURE_EVC */

    /* Only update ESDX statistics if ES0 hit */
    SRVL_WR(VTSS_REW_COMMON_STAT_CFG, VTSS_F_REW_COMMON_STAT_CFG_STAT_MODE(1));

    /* Clear VLAN table masks. This is done after changing the CPU port PVID above 
       to ensure that VRAP access is still working */
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        if (vid == VTSS_VID_DEFAULT) /* Default VLAN includes all ports */
            continue;
        SRVL_WR(VTSS_ANA_ANA_TABLES_VLANTIDX,
                VTSS_F_ANA_ANA_TABLES_VLANTIDX_V_INDEX(vid));
        SRVL_WR(VTSS_ANA_ANA_TABLES_VLANACCESS,
                VTSS_F_ANA_ANA_TABLES_VLANACCESS_VLAN_TBL_CMD(VLAN_CMD_WRITE));
        VTSS_RC(srvl_vlan_table_idle());
    }

    /* Setup VLAN configuration */
    VTSS_RC(srvl_vlan_conf_apply(0));

    /* Initialize TCAMs */
    VTSS_RC(srvl_vcap_initialize(VTSS_TCAM_IS0));
    VTSS_RC(srvl_vcap_initialize(VTSS_TCAM_IS1));
    VTSS_RC(srvl_vcap_initialize(VTSS_TCAM_IS2));
    VTSS_RC(srvl_vcap_initialize(VTSS_TCAM_ES0));

    /* Initialize VCAP port setup */
    for (port = 0; port < VTSS_CHIP_PORTS; port++) {
        /* Enable IS1 */
        SRVL_WR(VTSS_ANA_PORT_VCAP_CFG(port), VTSS_F_ANA_PORT_VCAP_CFG_S1_ENA);

        /* Enable IS2, treat IPv6 as IPv4 and treat OAM as ETYPE */
        SRVL_WR(VTSS_ANA_PORT_VCAP_S2_CFG(port),
                VTSS_F_ANA_PORT_VCAP_S2_CFG_S2_ENA |
                VTSS_F_ANA_PORT_VCAP_S2_CFG_S2_IP6_CFG(0xa) |
                VTSS_F_ANA_PORT_VCAP_S2_CFG_S2_OAM_DIS(0x3));

        /* Enable ES0 */
        SRVL_WRM_SET(VTSS_REW_PORT_PORT_CFG(port), VTSS_F_REW_PORT_PORT_CFG_ES0_ENA);
    }

    /* Setup discard policer */
    {
        srvl_policer_conf_t pol_conf;
    
        memset(&pol_conf, 0, sizeof(pol_conf));
        pol_conf.frame_rate = 1;
        VTSS_RC(srvl_policer_conf_set(SRVL_POLICER_DISCARD, &pol_conf));
    }

    /* Set-up default packet Rx endianness, position of status word, and who will be extracting. */
    for (i = 0; i < VTSS_PACKET_RX_GRP_CNT; i++) {
        /* Little-endian and status word before last data */
        SRVL_WRM_SET(VTSS_DEVCPU_QS_XTR_XTR_GRP_CFG(i), VTSS_F_DEVCPU_QS_XTR_XTR_GRP_CFG_BYTE_SWAP);
#if VTSS_OPT_VCORE_III
        /* If running on internal CPU, default to do register-based extraction.
         * Otherwise, default to using VRAP (which is the default in the chip).
         * If running on the internal CPU using the FDMA, the FDMA driver will make sure
         * to overwrite this field.
         */
        SRVL_WRM(VTSS_DEVCPU_QS_XTR_XTR_GRP_CFG(i), VTSS_F_DEVCPU_QS_XTR_XTR_GRP_CFG_MODE(1), VTSS_M_DEVCPU_QS_XTR_XTR_GRP_CFG_MODE);
#endif
    }

    /* Set-up default packet Tx endianness and who will be injecting. */
    for (i = 0; i < VTSS_PACKET_TX_GRP_CNT; i++) {
        /* Little-endian */
        SRVL_WRM_SET(VTSS_DEVCPU_QS_INJ_INJ_GRP_CFG(i), VTSS_F_DEVCPU_QS_INJ_INJ_GRP_CFG_BYTE_SWAP);
        /* According to datasheet, we must insert a small delay after every end-of-frame when injecting
         * to qs.
         */
        SRVL_WR(VTSS_DEVCPU_QS_INJ_INJ_CTRL(i), VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_GAP_SIZE(1));
#if VTSS_OPT_VCORE_III
        /* If running on internal CPU, default to do register-based injection.
         * Otherwise, default to using VRAP (which is the default in the chip).
         * If running on the internal CPU using the FDMA, the FDMA driver will make sure
         * to overwrite this field.
         */
        SRVL_WRM(VTSS_DEVCPU_QS_INJ_INJ_GRP_CFG(i), VTSS_F_DEVCPU_QS_INJ_INJ_GRP_CFG_MODE(1), VTSS_M_DEVCPU_QS_INJ_INJ_GRP_CFG_MODE);
#endif
    }

#if VTSS_OPT_VCORE_III
    /* Setup CPU port 0 and 1 */
    for (i = VTSS_CHIP_PORT_CPU_0; i <= VTSS_CHIP_PORT_CPU_1; i++) {
        /* Enable IFH insertion upon extraction */
        SRVL_WRM_SET(VTSS_SYS_SYSTEM_PORT_MODE(i), VTSS_F_SYS_SYSTEM_PORT_MODE_INCL_XTR_HDR(1));

        /* Enable IFH parsing upon injection */
        SRVL_WRM_SET(VTSS_SYS_SYSTEM_PORT_MODE(i), VTSS_F_SYS_SYSTEM_PORT_MODE_INCL_INJ_HDR(1));

        /* Strict priority when reading the two CPU ports. */
        SRVL_WRM(VTSS_QSYS_HSCH_SE_CFG(218 + i), VTSS_F_QSYS_HSCH_SE_CFG_SE_DWRR_CNT(0), VTSS_M_QSYS_HSCH_SE_CFG_SE_DWRR_CNT);
    }
#endif

    /* Enable CPU port */
    SRVL_WRM_SET(VTSS_QSYS_SYSTEM_SWITCH_PORT_MODE(VTSS_CHIP_PORT_CPU), VTSS_F_QSYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA);

    /* Setup CPU port 0 and 1 to allow for classification of transmission of
     * switched frames into a user-module-specifiable QoS class.
     * For the two CPU ports, we set a one-to-one mapping between a VLAN tag's
     * PCP and a QoS class. When transmitting switched frames, the PCP value
     * of the VLAN tag (which is always inserted to get it switched on a given
     * VID), then controls the priority. */
    /* Enable looking into PCP bits */
    SRVL_WR(VTSS_ANA_PORT_QOS_CFG(VTSS_CHIP_PORT_CPU), VTSS_F_ANA_PORT_QOS_CFG_QOS_PCP_ENA);

    /* Disable aging of Rx CPU queues to allow the frames to stay there longer than
     * on normal front ports. */
    SRVL_WRM_SET(VTSS_REW_PORT_PORT_CFG(VTSS_CHIP_PORT_CPU), VTSS_F_REW_PORT_PORT_CFG_AGE_DIS);

    /* Set-up the one-to-one mapping */
    for (pcp = 0; pcp < VTSS_PCP_END - VTSS_PCP_START; pcp++) {
        for (dei = 0; dei < VTSS_DEI_END - VTSS_DEI_START; dei++) {
            SRVL_WR(VTSS_ANA_PORT_QOS_PCP_DEI_MAP_CFG(VTSS_CHIP_PORT_CPU, (8 * dei + pcp)), 
                    VTSS_F_ANA_PORT_QOS_PCP_DEI_MAP_CFG_QOS_PCP_DEI_VAL(pcp));
        }
    }

    /* Setup aggregation mode */
    VTSS_RC(srvl_aggr_mode_set());
    
    /* Set MAC age time to default value */
    VTSS_RC(srvl_mac_table_age_time_set());

    /* Disable learning for frames discarded by VLAN ingress filtering */
    SRVL_WR(VTSS_ANA_ANA_ADVLEARN, VTSS_F_ANA_ANA_ADVLEARN_VLAN_CHK);

    /* Setup frame ageing - fixed value "2 sec" - in 6.5 us units */
    SRVL_WR(VTSS_SYS_SYSTEM_FRM_AGING, 
            VTSS_F_SYS_SYSTEM_FRM_AGING_AGE_TX_ENA | 
            VTSS_F_SYS_SYSTEM_FRM_AGING_MAX_AGE(2*2000000/13));

#if defined(VTSS_FEATURE_TIMESTAMP)
    SRVL_WR(VTSS_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG, VTSS_F_DEVCPU_GCB_PTP_CFG_PTP_MISC_CFG_PTP_ENA);
    /* release all timestamp id's except those that are reserved for SW*/
    SRVL_WR(VTSS_ANA_ANA_TABLES_PTP_ID_LOW, VTSS_ENCODE_BITMASK(TS_IDS_RESERVED_FOR_SW,32-TS_IDS_RESERVED_FOR_SW));
    SRVL_WR(VTSS_ANA_ANA_TABLES_PTP_ID_HIGH, 0xffffffff); /* assuming 32 are reserved for HW */
    /* pr default use 30 bit timestamping in the reserved field in the PTP packets (backplane mode) */
    SRVL_WRM(VTSS_SYS_PTP_PTP_CFG, VTSS_F_SYS_PTP_PTP_CFG_PTP_STAMP_WID(30), VTSS_M_SYS_PTP_PTP_CFG_PTP_STAMP_WID);

#endif /* VTSS_FEATURE_TIMESTAMP */

#if defined(VTSS_FEATURE_OAM)
    /* All VOEs are default enabled by hardware; we disable them here. This is
     * mainly to prevent the CLI command "debug api cil oam" from spewing out
     * loads of irrelevant information -- it only dumps enabled VOEs by
     * default. */
    for (i=0; i<VTSS_OAM_VOE_CNT; ++i)
        SRVL_WR(VTSS_OAM_MEP_VOE_BASIC_CTRL(i), 0);
#endif

#if defined(VTSS_FEATURE_MPLS)
    {
        u8 map, qos, tc;

        (void) srvl_mpls_vprofile_init();

        // TC mapping tables. All start out as identity mappings.

        for (map = 0; map < VTSS_MPLS_QOS_TO_TC_MAP_CNT; map++) {
            for (qos = 0; qos < VTSS_MPLS_QOS_TO_TC_ENTRY_CNT; qos++) {
                vtss_state->mpls_tc_conf.qos_to_tc_map[map][qos].dp0_tc = qos;
                vtss_state->mpls_tc_conf.qos_to_tc_map[map][qos].dp1_tc = qos;
            }
        }

        for (map = 0; map < VTSS_MPLS_TC_TO_QOS_MAP_CNT; map++) {
            for (tc = 0; tc < VTSS_MPLS_TC_TO_QOS_ENTRY_CNT; tc++) {
                vtss_state->mpls_tc_conf.tc_to_qos_map[map][tc].qos = tc;
                vtss_state->mpls_tc_conf.tc_to_qos_map[map][tc].dp = FALSE;
            }
        }

        (void) srvl_mpls_tc_conf_set(&vtss_state->mpls_tc_conf);
    }
#endif

    VTSS_D("exit");

    return VTSS_RC_OK;
}

vtss_rc vtss_serval_inst_create(void)
{
    vtss_cil_func_t *func = &vtss_state->cil_func;
#if defined(VTSS_FEATURE_IS0)
    vtss_vcap_obj_t *is0 = &vtss_state->is0.obj;
#endif
    vtss_vcap_obj_t *is1 = &vtss_state->is1.obj;
    vtss_vcap_obj_t *is2 = &vtss_state->is2.obj;
    vtss_vcap_obj_t *es0 = &vtss_state->es0.obj;

    /* Initialization */
    func->init_conf_set = srvl_init_conf_set;
    func->miim_read = srvl_miim_read;
    func->miim_write = srvl_miim_write;
    func->restart_conf_set = srvl_restart_conf_set;

    /* Miscellaneous */
    func->reg_read = srvl_reg_read;
    func->reg_write = srvl_reg_write;
    func->chip_id_get = srvl_chip_id_get;
    func->poll_1sec = srvl_poll_1sec;
    func->gpio_mode = srvl_gpio_mode;
    func->gpio_read = srvl_gpio_read;
    func->gpio_write = srvl_gpio_write;
    func->sgpio_conf_set = srvl_sgpio_conf_set;
    func->sgpio_read = srvl_sgpio_read;
    func->sgpio_event_enable = srvl_sgpio_event_enable;
    func->sgpio_event_poll = srvl_sgpio_event_poll;
    func->debug_info_print = srvl_debug_info_print;
    func->ptp_event_poll = srvl_ptp_event_poll;
    func->ptp_event_enable = srvl_ptp_event_enable;
    func->intr_cfg = srvl_intr_cfg;
    func->intr_pol_negation = srvl_intr_pol_negation;

    /* Port control */
    func->port_map_set = srvl_port_map_set;
    func->port_conf_set = srvl_port_conf_set;
    func->port_clause_37_status_get = srvl_port_clause_37_status_get;
    func->port_clause_37_control_set = srvl_port_clause_37_control_set;
    func->port_status_get = srvl_port_status_get;
    func->port_counters_update = srvl_port_counters_update;
    func->port_counters_clear = srvl_port_counters_clear;
    func->port_counters_get = srvl_port_counters_get;
    func->port_basic_counters_get = srvl_port_basic_counters_get;
    func->port_ifh_set = srvl_port_ifh_set;

    /* QoS */
    func->qos_conf_set = srvl_qos_conf_set;
    func->qos_port_conf_set = vtss_cmn_qos_port_conf_set;
    func->qos_port_conf_update = srvl_qos_port_conf_set;
    func->qce_add = vtss_cmn_qce_add;
    func->qce_del = vtss_cmn_qce_del;

    /* Layer 2 */
    func->mac_table_add = srvl_mac_table_add;
    func->mac_table_del = srvl_mac_table_del;
    func->mac_table_get = srvl_mac_table_get;
    func->mac_table_get_next = srvl_mac_table_get_next;
    func->mac_table_age_time_set = srvl_mac_table_age_time_set;
    func->mac_table_age = srvl_mac_table_age;
    func->mac_table_status_get = srvl_mac_table_status_get;
    func->learn_port_mode_set = srvl_learn_port_mode_set;
    func->learn_state_set = srvl_learn_state_set;
    func->port_forward_set = srvl_port_forward_set;
    func->mstp_state_set = vtss_cmn_mstp_state_set;
    func->mstp_vlan_msti_set = vtss_cmn_vlan_members_set;
    func->erps_vlan_member_set = vtss_cmn_erps_vlan_member_set;
    func->erps_port_state_set = vtss_cmn_erps_port_state_set;
    func->pgid_table_write = srvl_pgid_table_write;
    func->src_table_write = srvl_src_table_write;
    func->aggr_table_write = srvl_aggr_table_write;
    func->aggr_mode_set = srvl_aggr_mode_set;
    func->pmap_table_write = srvl_pmap_table_write;
    func->vlan_conf_set = srvl_vlan_conf_set;
    func->vlan_port_conf_set = vtss_cmn_vlan_port_conf_set;
    func->vlan_port_conf_update = srvl_vlan_port_conf_update;
    func->vlan_port_members_set = vtss_cmn_vlan_members_set;
    func->vlan_mask_update = srvl_vlan_mask_update;
    func->vlan_tx_tag_set = vtss_cmn_vlan_tx_tag_set;
    func->isolated_vlan_set = vtss_cmn_vlan_members_set;
    func->isolated_port_members_set = srvl_isolated_port_members_set;
    func->flood_conf_set = srvl_flood_conf_set;
#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
    func->ipv4_mc_add = vtss_cmn_ipv4_mc_add;
    func->ipv4_mc_del = vtss_cmn_ipv4_mc_del;
    func->ipv6_mc_add = vtss_cmn_ipv6_mc_add;
    func->ipv6_mc_del = vtss_cmn_ipv6_mc_del;
    func->ip_mc_update = srvl_ip_mc_update;
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */
    func->mirror_port_set = srvl_mirror_port_set;
    func->mirror_ingress_set = srvl_mirror_ingress_set;
    func->mirror_egress_set = srvl_mirror_egress_set;
    func->eps_port_set = vtss_cmn_eps_port_set;
#if defined(VTSS_FEATURE_SFLOW)
    func->sflow_port_conf_set         = srvl_sflow_port_conf_set;
    func->sflow_sampling_rate_convert = srvl_sflow_sampling_rate_convert;
#endif /* VTSS_FEATURE_SFLOW */
    func->vcl_port_conf_set = srvl_vcl_port_conf_set;
    func->vce_add = vtss_cmn_vce_add;
    func->vce_del = vtss_cmn_vce_del;
    func->vlan_trans_group_add = vtss_cmn_vlan_trans_group_add;
    func->vlan_trans_group_del = vtss_cmn_vlan_trans_group_del;
    func->vlan_trans_group_get = vtss_cmn_vlan_trans_group_get;
    func->vlan_trans_port_conf_set  = vtss_cmn_vlan_trans_port_conf_set;
    func->vlan_trans_port_conf_get  = vtss_cmn_vlan_trans_port_conf_get;
    func->vcap_port_conf_set = srvl_vcap_port_conf_set;

#if defined(VTSS_FEATURE_AFI_SWC)
    func->afi_alloc = srvl_afi_alloc;
    func->afi_free  = srvl_afi_free;
#endif /* VTSS_FEATURE_AFI_SWC */

    /* Packet control */
    func->rx_conf_set      = srvl_rx_conf_set;
    func->rx_frame_get     = srvl_rx_frame_get;
    func->rx_frame_discard = srvl_rx_frame_discard;
    func->tx_frame_port    = srvl_tx_frame_port;
    func->rx_hdr_decode    = srvl_rx_hdr_decode;
    func->tx_hdr_encode    = srvl_tx_hdr_encode;
    func->npi_conf_set     = srvl_npi_conf_set;

    /* EEE */
    func->eee_port_conf_set   = srvl_eee_port_conf_set;
    func->fan_controller_init = srvl_fan_controller_init;
    func->fan_cool_lvl_get    = srvl_fan_cool_lvl_get;
    func->fan_cool_lvl_set    = srvl_fan_cool_lvl_set;
    func->fan_rotation_get    = srvl_fan_rotation_get;

#if defined(VTSS_FEATURE_EVC)
    /* EVC */
    func->evc_policer_conf_set = srvl_evc_policer_conf_set;
    func->evc_port_conf_set = srvl_evc_port_conf_set;
    func->evc_add = vtss_cmn_evc_add;
    func->evc_del = vtss_cmn_evc_del;
    func->ece_add = vtss_cmn_ece_add;
    func->ece_del = vtss_cmn_ece_del;
    func->ece_update = srvl_ece_update;
    func->evc_update = srvl_evc_update;
    func->ece_counters_get = vtss_cmn_ece_counters_get;
    func->ece_counters_clear = vtss_cmn_ece_counters_clear;
    func->evc_counters_get = vtss_cmn_evc_counters_get;
    func->evc_counters_clear = vtss_cmn_evc_counters_clear;
    func->sdx_counters_update = srvl_sdx_counters_update;
    func->mce_add = srvl_mce_add;
    func->mce_del = srvl_mce_del;
    func->mce_port_get = srvl_mce_port_get;
    vtss_state->evc_info.max_count = SRVL_EVC_CNT;
    vtss_state->ece_info.max_count = SRVL_EVC_CNT;
    vtss_state->mce_info.max_count = VTSS_MCES;
    vtss_state->sdx_info.max_count = SRVL_EVC_CNT;
    if (vtss_state->sdx_info.max_count > VTSS_SRVL_SDX_CNT)
        vtss_state->sdx_info.max_count = VTSS_SRVL_SDX_CNT;
    vtss_state->evc_policer_max = 1022;
#endif /* VTSS_FEATURE_EVC */

    /* ACL */
    func->acl_policer_set = srvl_acl_policer_set;
    func->acl_port_set = srvl_acl_port_conf_set;
    func->acl_port_counter_get = srvl_acl_port_counter_get;
    func->acl_port_counter_clear = srvl_acl_port_counter_clear;
    func->acl_ace_add = srvl_ace_add;
    func->acl_ace_del = srvl_ace_del;
    func->acl_ace_counter_get = vtss_cmn_ace_counter_get;
    func->acl_ace_counter_clear = vtss_cmn_ace_counter_clear;
    func->vcap_range_commit = srvl_vcap_range_commit;

#if defined(VTSS_FEATURE_IS0)
    /* IS0 */
    is0->max_count = SRVL_IS0_CNT;
    is0->max_rule_count = VTSS_SRVL_IS0_CNT;
    is0->entry_get = srvl_is0_entry_get;
    is0->entry_add = srvl_is0_entry_add;
    is0->entry_del = srvl_is0_entry_del;
    is0->entry_move = srvl_is0_entry_move;
#endif

    /* IS1 */
    is1->max_count = SRVL_IS1_CNT;
    is1->max_rule_count = VTSS_SRVL_IS1_CNT;
    is1->entry_get = srvl_is1_entry_get;
    is1->entry_add = srvl_is1_entry_add;
    is1->entry_del = srvl_is1_entry_del;
    is1->entry_move = srvl_is1_entry_move;

    /* IS2 */
    is2->max_count = SRVL_IS2_CNT;
    is2->max_rule_count = VTSS_SRVL_IS2_CNT;
    is2->entry_get = srvl_is2_entry_get;
    is2->entry_add = srvl_is2_entry_add;
    is2->entry_del = srvl_is2_entry_del;
    is2->entry_move = srvl_is2_entry_move;

    /* ES0 */
    es0->max_count = SRVL_ES0_CNT;
    es0->max_rule_count = VTSS_SRVL_ES0_CNT;
    es0->entry_get = srvl_es0_entry_get;
    es0->entry_add = srvl_es0_entry_add;
    es0->entry_del = srvl_es0_entry_del;
    es0->entry_move = srvl_es0_entry_move;
    func->es0_entry_update = srvl_es0_entry_update;

#if defined(VTSS_FEATURE_TIMESTAMP)
    /* Time stamping features */
    func->ts_timeofday_get = srvl_ts_timeofday_get;
    func->ts_timeofday_set = srvl_ts_timeofday_set;
    func->ts_timeofday_set_delta = srvl_ts_timeofday_set_delta;
    func->ts_timeofday_next_pps_get = srvl_ts_timeofday_next_pps_get;
    func->ts_timeofday_offset_set = srvl_ts_timeofday_offset_set;
    func->ts_ns_cnt_get = srvl_ts_ns_cnt_get;
    func->ts_timeofday_one_sec = srvl_ts_timeofday_one_sec;
    func->ts_adjtimer_set = srvl_ts_adjtimer_set;
    func->ts_freq_offset_get = srvl_ts_freq_offset_get;
    func->ts_external_clock_mode_set = srvl_ts_external_clock_mode_set;
    func->ts_alt_clock_saved_get = srvl_ts_alt_clock_saved_get;
    func->ts_alt_clock_mode_set = srvl_ts_alt_clock_mode_set;
    func->ts_timeofday_next_pps_set = srvl_ts_timeofday_next_pps_set;
    func->ts_ingress_latency_set = srvl_ts_ingress_latency_set;
    func->ts_p2p_delay_set = srvl_ts_p2p_delay_set;
    func->ts_egress_latency_set = srvl_ts_egress_latency_set;
    func->ts_delay_asymmetry_set = srvl_ts_delay_asymmetry_set;
    func->ts_operation_mode_set = srvl_ts_operation_mode_set;
    func->ts_internal_mode_set = srvl_ts_internal_mode_set;
    func->ts_timestamp_get = srvl_ts_timestamp_get;
    func->ts_timestamp_id_release = srvl_ts_timestamp_id_release;
#endif /* VTSS_FEATURE_TIMESTAMP */
    
    /* SYNCE features */
    func->synce_clock_out_set = srvl_synce_clock_out_set;
    func->synce_clock_in_set = srvl_synce_clock_in_set;

#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
    serval_fdma_func_init();
#endif

#if defined(VTSS_FEATURE_OAM)
    /* Y.1731/802.1ag OAM */
    func->oam_vop_cfg_set               = srvl_oam_vop_cfg_set;
    func->oam_voe_alloc                 = srvl_oam_voe_alloc;
    func->oam_voe_free                  = srvl_oam_voe_free;
    func->oam_voe_cfg_set               = srvl_oam_voe_cfg_set;
    func->oam_event_poll                = srvl_oam_event_poll;
    func->oam_voe_event_poll            = srvl_oam_voe_event_poll;
    func->oam_voe_event_enable          = srvl_oam_voe_event_enable;
    func->oam_voe_ccm_arm_hitme         = srvl_oam_voe_ccm_arm_hitme;
    func->oam_voe_ccm_set_rdi_flag      = srvl_oam_voe_ccm_set_rdi_flag;
    func->oam_ccm_status_get            = srvl_oam_ccm_status_get;
    func->oam_pdu_seen_status_get       = srvl_oam_pdu_seen_status_get;
    func->oam_proc_status_get           = srvl_oam_proc_status_get;
    func->oam_voe_counter_update        = srvl_oam_voe_counter_update;
    func->oam_voe_counter_update_serval = srvl_oam_voe_counter_update_serval;
    func->oam_voe_counter_get           = srvl_oam_voe_counter_get;
    func->oam_voe_counter_clear         = srvl_oam_voe_counter_clear;
    func->oam_ipt_conf_get              = srvl_oam_ipt_conf_get;
    func->oam_ipt_conf_set              = srvl_oam_ipt_conf_set;
#endif /* VTSS_FEATURE_OAM */

#ifdef VTSS_FEATURE_MIRROR_CPU
    func->mirror_cpu_ingress_set        = srvl_mirror_cpu_ingress_set;
    func->mirror_cpu_egress_set         = srvl_mirror_cpu_egress_set;
#endif //VTSS_FEATURE_MIRROR_CPU

#if defined (VTSS_FEATURE_MPLS)
    /* MPLS / MPLS-TP */
    func->mpls_tc_conf_set              = srvl_mpls_tc_conf_set;
    func->mpls_l2_alloc                 = srvl_mpls_l2_alloc;
    func->mpls_l2_free                  = srvl_mpls_l2_free;
    func->mpls_l2_set                   = srvl_mpls_l2_set;
    func->mpls_l2_segment_attach        = srvl_mpls_l2_segment_attach;
    func->mpls_l2_segment_detach        = srvl_mpls_l2_segment_detach;
    func->mpls_xc_alloc                 = srvl_mpls_xc_alloc;
    func->mpls_xc_free                  = srvl_mpls_xc_free;
    func->mpls_xc_set                   = srvl_mpls_xc_set;
    func->mpls_xc_segment_attach        = srvl_mpls_xc_segment_attach;
    func->mpls_xc_segment_detach        = srvl_mpls_xc_segment_detach;
    func->mpls_xc_mc_segment_attach     = srvl_mpls_xc_mc_segment_attach;
    func->mpls_xc_mc_segment_detach     = srvl_mpls_xc_mc_segment_detach;
    func->mpls_segment_alloc            = srvl_mpls_segment_alloc;
    func->mpls_segment_free             = srvl_mpls_segment_free;
    func->mpls_segment_set              = srvl_mpls_segment_set;
    func->mpls_segment_state_get        = srvl_mpls_segment_state_get;
    func->mpls_segment_server_attach    = srvl_mpls_segment_server_attach;
    func->mpls_segment_server_detach    = srvl_mpls_segment_server_detach;
#endif /* VTSS_FEATURE_MPLS */

    /* State data */
    vtss_state->gpio_count = SRVL_GPIOS;
    vtss_state->sgpio_group_count = SRVL_SGPIO_GROUPS;
    vtss_state->prio_count = SRVL_PRIOS;
    vtss_state->ac_count = SRVL_ACS;
    vtss_state->rx_queue_count = VTSS_PACKET_RX_QUEUE_CNT;

    return VTSS_RC_OK;
}

#endif /* VTSS_ARCH_SERVAL */
