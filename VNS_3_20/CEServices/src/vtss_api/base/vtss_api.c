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

#include "vtss_api.h"
#include "vtss_state.h"
#include "vtss_common.h"
#include "vtss_api_internal.h"

#if defined(VTSS_ARCH_LUTON28)
#include "vtss_luton28.h"
#endif

#if defined(VTSS_ARCH_LUTON26)
#include "vtss_luton26.h"
#endif

#if defined(VTSS_ARCH_SERVAL)
#include "vtss_serval.h"
#endif

#if defined(VTSS_ARCH_B2)
#include "vtss_b2.h"
#endif

#if defined(VTSS_ARCH_JAGUAR_1)
#include "vtss_jaguar1.h"
#endif

#if defined(VTSS_ARCH_DAYTONA)
#include "daytona/vtss_daytona.h"
#endif

#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
#include "vtss_fdma_common.h"
#endif

#include "vtss_misc_api.h"

#if defined(VTSS_CHIP_CU_PHY) || defined(VTSS_CHIP_10G_PHY)
#if defined (VTSS_FEATURE_PHY_TIMESTAMP)
#include "vtss_phy_ts.h"
#endif /* VTSS_FEATURE_PHY_TIMESTAMP */
#endif /* VTSS_CHIP_CU_PHY || VTSS_CHIP_10G_PHY*/

/* Default instance */
vtss_inst_t vtss_default_inst = NULL;

/* Current state */
vtss_state_t *vtss_state;

/* Current function */
const char *vtss_func;

/* Trace group table */
vtss_trace_conf_t vtss_trace_conf[VTSS_TRACE_GROUP_COUNT] =
{
    [VTSS_TRACE_GROUP_DEFAULT] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_PORT] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_PHY] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_PACKET] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_QOS] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_L2] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_L3] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_SECURITY] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_EVC] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_FDMA_NORMAL] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_FDMA_IRQ] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
#ifdef VTSS_ARCH_DAYTONA
    [VTSS_TRACE_GROUP_XFI] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_UPI] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
#endif /* VTSS_ARCH_DAYTONA */
    [VTSS_TRACE_GROUP_REG_CHECK] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
    [VTSS_TRACE_GROUP_MPLS] = {
        .level = { VTSS_TRACE_LEVEL_ERROR, VTSS_TRACE_LEVEL_ERROR}
    },
};

/* ================================================================= *
 *  Initialization
 * ================================================================= */

/* Check and get instance */
static vtss_rc vtss_inst_check_get(const vtss_inst_t inst, vtss_state_t **state)
{
    /* Default instance is used if inst is NULL */
    *state = (inst == NULL ? vtss_default_inst : inst);

    VTSS_N("enter");

    /* Check cookie */
    if (*state == NULL || (*state)->cookie != VTSS_STATE_COOKIE) {
        VTSS_E("illegal inst: %p", inst);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/* Check instance and save state pointers */
vtss_rc vtss_inst_check(const vtss_inst_t inst)
{
    vtss_rc rc;

    if ((rc = vtss_inst_check_get(inst, &vtss_state)) == VTSS_RC_OK) {
        /* Select first device by default */
        VTSS_SELECT_CHIP(0);
    }
    return rc;
}

/* Check instance and return the state pointer */
static vtss_state_t *vtss_inst_check_no_persist(const vtss_inst_t inst)
{
    /* Default instance is used if inst is NULL */
    vtss_state_t *state = (inst == NULL ? vtss_default_inst : inst);

    VTSS_N("enter");

    /* Check cookie */
    if (state == NULL || state->cookie != VTSS_STATE_COOKIE) {
        VTSS_E("illegal inst: %p", inst);
    }

    return state;
}

vtss_rc vtss_inst_port_no_check(const vtss_inst_t    inst,
                                const vtss_port_no_t port_no)
{
    vtss_rc rc;

    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_port_no_check(port_no);

    return rc;
}

#if defined(VTSS_ARCH_B2)
// Function checking that an inst is valid + checking that a port number
//is within the valid range + Allowing port number to be set to VTSS_PORT_NO_NONE.
static vtss_rc vtss_inst_port_no_check_allow_port_none(const vtss_inst_t    inst,
                                                  const vtss_port_no_t port_no)
{
  if ((vtss_inst_check(inst)) == VTSS_RC_OK) {
    if (port_no >= vtss_state->port_count && port_no != VTSS_PORT_NO_NONE) {
      VTSS_E("illegal port_no: %u", port_no);
      return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
  } else {
    return VTSS_RC_ERROR;
  }
}
#endif /* VTSS_ARCH_B2 */

#if defined(VTSS_FEATURE_10G)
static vtss_rc vtss_mmd_check(vtss_port_no_t         port_no,
                              u8                     addr,
                              vtss_miim_controller_t *miim_controller,
                              u8                     *miim_addr)
{
    vtss_port_map_t *port_map;

    VTSS_RC(vtss_port_no_check(port_no));
    if (addr > 31) {
        VTSS_E("illegal addr: %u on port_no: %u", addr, port_no);
        return VTSS_RC_ERROR;
    }
    port_map = &vtss_state->port_map[port_no];
    *miim_controller = port_map->miim_controller;
    *miim_addr = port_map->miim_addr;
    if (*miim_controller < 0 || *miim_controller >= VTSS_MIIM_CONTROLLERS) {
        VTSS_E("illegal miim_controller: %d on port_no: %u", *miim_controller, port_no);
        return VTSS_RC_ERROR;
    }
    if (*miim_addr > 31) {
        VTSS_E("illegal miim_addr: %u on port_no: %u", *miim_addr, port_no);
        return VTSS_RC_ERROR;
    }
    VTSS_SELECT_CHIP(port_map->miim_chip_no);
    return VTSS_RC_OK;
}

static vtss_rc vtss_mmd_reg_read(const vtss_port_no_t  port_no,
                                 const u8              mmd,
                                 const u16             addr,
                                 u16                   *const value)
{
    vtss_rc                rc;
    vtss_miim_controller_t mdio_controller;
    u8                     mdio_addr;

    if ((rc = vtss_mmd_check(port_no, 0, &mdio_controller, &mdio_addr)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mmd_read, mdio_controller, mdio_addr, mmd, addr, value, TRUE);
    }

    if (rc == VTSS_RC_OK) {
        VTSS_N("port_no: %u, mmd: %u, addr: 0x%04x, value: 0x%04x",
               port_no, mmd, addr, *value);
    }
    return rc;
}

static vtss_rc vtss_mmd_reg_read_inc(const vtss_port_no_t  port_no,
                                 const u8              mmd,
                                 const u16             addr,
                                 u16                   *buf,
                                 u8                    count)
{
    vtss_rc                rc;
    vtss_miim_controller_t mdio_controller;
    u8                     mdio_addr;
    u8                     buf_count = count;

    if ((rc = vtss_mmd_check(port_no, 0, &mdio_controller, &mdio_addr)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mmd_read_inc, mdio_controller, mdio_addr, mmd, addr, buf, count, TRUE);
    }

    if (rc == VTSS_RC_OK) {
        VTSS_N("port_no: %u, mmd: %u, addr: 0x%04x", port_no, mmd, addr);
        while (buf_count) {
            VTSS_N(" value[%d]: 0x%04x",count,*buf);
            buf++;
            buf_count--;
        }
    }
    return rc;
}


static vtss_rc vtss_mmd_reg_write(const vtss_port_no_t  port_no,
                                  const u8              mmd,
                                  const u16             addr,
                                  const u16             value)
{
    vtss_rc                rc;
    vtss_miim_controller_t mdio_controller;
    u8                     mdio_addr;

    if ((rc = vtss_mmd_check(port_no, 0, &mdio_controller, &mdio_addr)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mmd_write, mdio_controller, mdio_addr, mmd, addr, value, TRUE);
    }

    if (rc == VTSS_RC_OK) {
        VTSS_N("port_no: %u, mmd: %u, addr: 0x%04x, value: 0x%04x", port_no, mmd, addr, value);
    }
    return rc;
}
#endif /* VTSS_FEATURE_10G */

#if defined(VTSS_FEATURE_PORT_CONTROL)

static vtss_rc vtss_port_no_none_check(const vtss_port_no_t port_no)
{
    return (port_no == VTSS_PORT_NO_NONE ? VTSS_RC_OK : vtss_port_no_check(port_no));
}

static vtss_rc vtss_miim_check(vtss_port_no_t         port_no,
                               u8                     addr,
                               vtss_miim_controller_t *miim_controller,
                               u8                     *miim_addr)
{
    vtss_port_map_t *port_map;

    VTSS_RC(vtss_port_no_check(port_no));
    if (addr > 31) {
        VTSS_E("illegal addr: %u on port_no: %u", addr, port_no);
        return VTSS_RC_ERROR;
    }
    port_map = &vtss_state->port_map[port_no];
    *miim_controller = port_map->miim_controller;
    *miim_addr = port_map->miim_addr;
    if (*miim_controller < 0 || *miim_controller >= VTSS_MIIM_CONTROLLERS) {
        VTSS_E("illegal miim_controller: %d on port_no: %u", *miim_controller, port_no);
        return VTSS_RC_ERROR;
    }
    if (*miim_addr > 31) {
        VTSS_E("illegal miim_addr: %u on port_no: %u", *miim_addr, port_no);
        return VTSS_RC_ERROR;
    }
    VTSS_SELECT_CHIP(port_map->miim_chip_no);
    return VTSS_RC_OK;
}


/* MII management read function (IEEE 802.3 clause 22) */
static vtss_rc vtss_port_miim_read(const vtss_port_no_t port_no,
                                   const u8             addr,
                                   u16                  *const value)
{
    vtss_rc                rc;
    vtss_miim_controller_t miim_controller;
    u8                     miim_addr;

    VTSS_RC(vtss_miim_check(port_no, addr, &miim_controller, &miim_addr));
    if ((rc = VTSS_FUNC(miim_read, miim_controller, miim_addr, addr, value, TRUE)) == VTSS_RC_OK) {
        VTSS_N("port_no: %u, addr: 0x%02x, value: 0x%04x", port_no, addr, *value);
    }

    return rc;
}

/* MII management write function (IEEE 802.3 clause 22) */
static vtss_rc vtss_port_miim_write(const vtss_port_no_t port_no,
                                    const u8             addr,
                                    const u16            value)
{
    vtss_rc                rc;
    vtss_miim_controller_t miim_controller;
    u8                     miim_addr;

    VTSS_RC(vtss_miim_check(port_no, addr, &miim_controller, &miim_addr));
    if ((rc = VTSS_FUNC(miim_write, miim_controller, miim_addr, addr, value, TRUE)) == VTSS_RC_OK) {
        VTSS_N("port_no: %u, addr: 0x%02x, value: 0x%04x", port_no, addr, value);
    }
    return rc;
}
#endif /* VTSS_FEATURE_PORT_CONTROL */

vtss_rc vtss_inst_get(const vtss_target_type_t target,
                      vtss_inst_create_t       *const create)
{
    VTSS_D("enter");
    create->target = target;
    VTSS_D("exit");

    return VTSS_RC_OK;
}

/* Initialize state to default values */
static vtss_rc vtss_inst_default_set(void)
{
    vtss_init_conf_t *init_conf;

    VTSS_D("enter");
    init_conf = &vtss_state->init_conf;

#if defined(VTSS_FEATURE_PORT_CONTROL)
    init_conf->miim_read = vtss_port_miim_read;
    init_conf->miim_write = vtss_port_miim_write;

#endif /* VTSS_FEATURE_PORT_CONTROL */

#if defined(VTSS_FEATURE_10G)
    init_conf->mmd_read = vtss_mmd_reg_read;
    init_conf->mmd_read_inc = vtss_mmd_reg_read_inc;
    init_conf->mmd_write = vtss_mmd_reg_write;
#endif /* VTSS_FEATURE_10G */

#if defined(VTSS_FEATURE_PORT_MUX)
    switch (vtss_state->create.target) {
#if defined(VTSS_ARCH_LUTON26)
    case VTSS_TARGET_SPARX_III_10:
    case VTSS_TARGET_SPARX_III_10_01:
        init_conf->mux_mode = VTSS_PORT_MUX_MODE_2;
        break;
    case VTSS_TARGET_CARACAL_1:
    case VTSS_TARGET_SPARX_III_10_UM:
        init_conf->mux_mode = VTSS_PORT_MUX_MODE_1;
        break;
#endif /* VTSS_ARCH_LUTON26 */
    default:
        init_conf->mux_mode = VTSS_PORT_MUX_MODE_0;
        break;
    }
#endif /* VTSS_FEATURE_PORT_MUX */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
    u32                   i;
    vtss_lport_no_t       lport_no;
    vtss_qos_lport_conf_t *qos;
    vtss_burst_level_t    level = 4096;

    init_conf->host_mode = VTSS_HOST_MODE_2;           
    for (i=0; i<4; i++) {
        vtss_state->xaui[i].hih.format = VTSS_HIH_PRE_SFD;
    }
    /* Lport and Lport Queue QoS defaults */
    for (lport_no = VTSS_PORT_NO_START; lport_no < VTSS_LPORTS; lport_no++) {
        qos = &vtss_state->qos_lport[lport_no];
        qos->shaper.rate = VTSS_BITRATE_DISABLED;
        qos->shaper.level = level;
        qos->lport_pct = 17;
        for (i = 0; i < VTSS_PRIOS; i++) {
            qos->red[i].enable = FALSE;
            qos->red[i].max_th = 100;
            qos->red[i].min_th = 0;
            qos->red[i].max_prob_1 = 1;
            qos->red[i].max_prob_2 = 5;
            qos->red[i].max_prob_3 = 10;
        }
        for (i = 0; i < 2; i++) {
            qos->scheduler.queue[i].rate  = VTSS_BITRATE_DISABLED;
            qos->scheduler.queue[i].level = level;
        }
    }

#endif /* (VTSS_ARCH_JAGUAR_1_CE_MAC) */

#if defined(VTSS_FEATURE_SERDES_MACRO_SETTINGS)
    init_conf->serdes.serdes1g_vdd = VTSS_VDD_1V0; /* Based on Ref board design */  
    init_conf->serdes.serdes6g_vdd = VTSS_VDD_1V0; /* Based on Ref board design */  
    init_conf->serdes.ib_cterm_ena = 1;            /* Based on Ref board design */
    init_conf->serdes.qsgmii.ob_post0 = 2;         /* Based on Ref board design */
    init_conf->serdes.qsgmii.ob_sr = 0;            /* Based on Ref board design */
#endif /* VTSS_FEATURE_SERDES_MACRO_SETTINGS */

#if defined(VTSS_ARCH_LUTON28)
    {
        vtss_pi_conf_t *pi = &init_conf->pi;
        pi->width = VTSS_PI_WIDTH_16;
        pi->use_extended_bus_cycle = 1;
    }
#endif /* VTSS_ARCH_LUTON28 */
#if defined(VTSS_ARCH_B2)
    {
        vtss_spi4_conf_t *spi4;
        vtss_xaui_conf_t *xaui;
        vtss_qs_conf_t   *qs;
        int i;

        /* General parameters */
        init_conf->host_mode = 4;
        init_conf->stag_etype = VTSS_ETYPE_TAG_S;
        init_conf->sch_max_rate = 1000000; /* 1 Gbps */

        /* Queue system */
        qs = &init_conf->qs;
        qs->rx_share = 0;
        qs->mtu = VTSS_MAX_FRAME_LENGTH_STANDARD;
        qs->qs_7 = VTSS_QUEUE_STRICT_UNRESERVE;
        qs->qs_6 = VTSS_QUEUE_STRICT_UNRESERVE;
        for(i=0; i<6; i++)
            qs->qw[i]= VTSS_QUEUE_WEIGHT_MAX_DISABLE;
        qs->qss_qsp_bwf_pct = 100;

        /* SPI-4 */
        spi4 = &vtss_state->spi4;
        spi4->fc.enable = 1;
        spi4->qos.shaper.rate = VTSS_BITRATE_DISABLED;
        spi4->ib.fcs = VTSS_SPI4_FCS_CHECK;
        spi4->ib.clock = VTSS_SPI4_CLOCK_450_TO_500;
        spi4->ob.clock = VTSS_SPI4_CLOCK_500_0;
        spi4->ob.burst_size = VTSS_SPI4_BURST_128;
        spi4->ob.max_burst_1 = 64;
        spi4->ob.max_burst_2 = 32;
        spi4->ob.link_up_limit = 2;
        spi4->ob.link_down_limit = 2;
        spi4->ob.training_mode = VTSS_SPI4_TRAINING_AUTO;
        spi4->ob.alpha = 1;
        spi4->ob.data_max_t = 10240;

        /* XAUI */
        xaui = &vtss_state->xaui[0];
        xaui->qos.shaper.rate = VTSS_BITRATE_DISABLED;

        memcpy(&vtss_state->xaui[1],xaui,sizeof(*xaui));
    }
#endif /* VTSS_ARCH_B2 */

    vtss_state->port_count = VTSS_PORTS;

#if defined(VTSS_FEATURE_SERIAL_GPIO)
    {
        vtss_sgpio_group_t group;

        for (group = 0; group < VTSS_SGPIO_GROUPS; group++) {
            vtss_state->sgpio_conf[0][group].bit_count = 4;
            vtss_state->sgpio_conf[1][group].bit_count = 4;
        }
    }
#endif /* VTSS_FEATURE_SERIAL_GPIO */

#if defined(VTSS_FEATURE_QOS)
    {
        vtss_qos_conf_t *qos = &vtss_state->qos_conf;

        qos->prios = vtss_state->prio_count;

#if defined(VTSS_FEATURE_QOS_DSCP_REMARK)
        {
            int i;
            for (i = 0; i < 64; i++) {
                qos->dscp_remark[i] = FALSE;
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
                qos->dscp_translate_map[i] = i;
                qos->dscp_remap[i] = i;
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
                qos->dscp_remap_dp1[i] = i;
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
            }
        }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */

#if defined(VTSS_FEATURE_QOS_POLICER_CPU_SWITCH)
        qos->policer_mac = VTSS_PACKET_RATE_DISABLED;
        qos->policer_cat = VTSS_PACKET_RATE_DISABLED;
        qos->policer_learn = VTSS_PACKET_RATE_DISABLED;
#endif /* defined(VTSS_FEATURE_QOS_POLICER_CPU_SWITCH) */
#if defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
        qos->policer_uc = VTSS_PACKET_RATE_DISABLED;
#endif /* defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */
#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH)
        qos->policer_mc = VTSS_PACKET_RATE_DISABLED;
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) */
#if defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
        qos->policer_bc = VTSS_PACKET_RATE_DISABLED;
#endif /* defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) */
    }
    {
        vtss_port_no_t       port_no;
        vtss_qos_port_conf_t *qos;
        u32                  i;
        vtss_burst_level_t   level = 0;
#if defined(VTSS_ARCH_B2)
        vtss_port_filter_t   *filter;
#endif /* VTSS_ARCH_B2 */
#if defined(VTSS_ARCH_LUTON28) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
        level = 4096;
#endif /* defined(VTSS_ARCH_LUTON28) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL) */

        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            qos = &vtss_state->qos_port_conf[port_no];

            for (i = 0; i < VTSS_PORT_POLICERS; i++) {
                qos->policer_port[i].level                    = level;
                qos->policer_port[i].rate                     = VTSS_BITRATE_DISABLED;

#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT)
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS)
                qos->policer_ext_port[i].frame_rate           = FALSE;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FPS */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL)
                qos->policer_ext_port[i].dp_bypass_level      = 0;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_DPBL */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM)
                qos->policer_ext_port[i].unicast              = TRUE;
                qos->policer_ext_port[i].multicast            = TRUE;
                qos->policer_ext_port[i].broadcast            = TRUE;
                qos->policer_ext_port[i].uc_no_flood          = FALSE;
                qos->policer_ext_port[i].mc_no_flood          = FALSE;
                qos->policer_ext_port[i].flooded              = TRUE;
                qos->policer_ext_port[i].learning             = TRUE;
                qos->policer_ext_port[i].to_cpu               = TRUE;
                {
                    int q;
                    for (q = 0; q < VTSS_PORT_POLICER_CPU_QUEUES; q++) {
                        qos->policer_ext_port[i].cpu_queue[q]     = TRUE;
                    }
                }
                qos->policer_ext_port[i].limit_noncpu_traffic = TRUE;
                qos->policer_ext_port[i].limit_cpu_traffic    = FALSE;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_TTM */
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
                qos->policer_ext_port[i].flow_control         = FALSE;
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC */
#endif /* VTSS_FEATURE_QOS_PORT_POLICER_EXT */
            }

#ifdef VTSS_FEATURE_QOS_QUEUE_POLICER
            for (i = VTSS_QUEUE_START; i < VTSS_QUEUE_END; i++) {
                qos->policer_queue[i].level = level;
                qos->policer_queue[i].rate  = VTSS_BITRATE_DISABLED;
            }
#endif /* VTSS_FEATURE_QOS_QUEUE_POLICER */

            qos->shaper_port.rate = VTSS_BITRATE_DISABLED;
            qos->shaper_port.level = level;

#if defined(VTSS_FEATURE_QOS_WRED)
#if defined(VTSS_ARCH_JAGUAR_1)
            for (i = VTSS_QUEUE_START; i < VTSS_QUEUE_END; i++) {
                qos->red[i].enable = FALSE;
                qos->red[i].max_th = 100;
                qos->red[i].min_th = 0;
                qos->red[i].max_prob_1 = 1;
                qos->red[i].max_prob_2 = 5;
                qos->red[i].max_prob_3 = 10;
            }
#endif /* VTSS_ARCH_JAGUAR_1 */
#endif /* VTSS_FEATURE_QOS_WRED */

#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)
            for (i = VTSS_QUEUE_START; i < VTSS_QUEUE_END; i++) {
                qos->shaper_queue[i].rate = VTSS_BITRATE_DISABLED;
                qos->shaper_queue[i].level = level;
                qos->excess_enable[i] = FALSE;
            }
#endif  /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */

#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
            qos->default_dpl = 0;
            qos->default_dei = 0;
            qos->tag_class_enable = FALSE;
            for (i = VTSS_PCP_START; i < VTSS_PCP_ARRAY_SIZE; i++) {
                int dei;
                for (dei = VTSS_DEI_START; dei < VTSS_DEI_ARRAY_SIZE; dei++) {
                    qos->qos_class_map[i][dei] = vtss_cmn_pcp2qos(i);
                    qos->dp_level_map[i][dei] = dei; // Defaults to same value as DEI
                }
            }
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

#if defined(VTSS_FEATURE_QOS_SCHEDULER_V2)
            qos->dwrr_enable = FALSE;
            for (i = VTSS_QUEUE_START; i < (VTSS_QUEUE_END - 2); i++) {
                qos->queue_pct[i] = 17; // ~17% to each queue (17 * 6 ~ 100%)
            }
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

#if defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)
            qos->tag_remark_mode = VTSS_TAG_REMARK_MODE_CLASSIFIED;
            qos->tag_default_pcp = 0;
            qos->tag_default_dei = 0;
            qos->tag_dp_map[0] = 0;
            qos->tag_dp_map[1] = 1;
            qos->tag_dp_map[2] = 1;
            qos->tag_dp_map[3] = 1;
            for (i = VTSS_PRIO_START; i < VTSS_PRIO_END; i++) {
                int dpl;
                for (dpl = 0; dpl < 2; dpl++) {
                    qos->tag_pcp_map[i][dpl] = vtss_cmn_pcp2qos(i);
                    qos->tag_dei_map[i][dpl] = dpl; // Defaults to same value as DP level
                }
            }
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#if defined(VTSS_CHIP_10G_PHY)
            vtss_state->phy_10g_api_base_no = VTSS_PORT_NO_NONE;
            vtss_state->phy_88_event_B[0] = FALSE;
            vtss_state->phy_88_event_B[1] = FALSE;
#endif

#if defined(VTSS_ARCH_B2)
            qos->vlan_aware = 1; /* Enable VLAN awareness for QoS and filtering */
            qos->custom_filter.local.forward = 1;
            for (i = VTSS_QUEUE_START; i < VTSS_QUEUE_END; i++)
                qos->policer_queue[i].rate = VTSS_BITRATE_DISABLED;
            qos->shaper_port.rate = VTSS_BITRATE_DISABLED;
            qos->shaper_host.rate = VTSS_BITRATE_DISABLED;
            qos->scheduler.rate = VTSS_BITRATE_DISABLED;
            qos->scheduler.queue_pct[0] = 0;  /* Q0 */
            qos->scheduler.queue_pct[1] = 10; /* Q1 */
            qos->scheduler.queue_pct[2] = 15; /* Q2 */
            qos->scheduler.queue_pct[3] = 20; /* Q3 */
            qos->scheduler.queue_pct[4] = 25; /* Q4 */
            qos->scheduler.queue_pct[5] = 30; /* Q5 */

            qos->default_qos.prio = VTSS_PRIO_START;
            qos->l2_control_qos.prio = VTSS_PRIO_START;
            qos->l3_control_qos.prio = VTSS_PRIO_START;
            qos->cfi_qos.prio = VTSS_PRIO_START;
            qos->ipv4_arp_qos.prio = VTSS_PRIO_START;
            qos->ipv6_qos.prio = VTSS_PRIO_START;
            for (i = 0; i < 8; i++) {
                qos->mpls_exp_qos[i].prio = VTSS_PRIO_START;
            qos->vlan_tag_qos[i].prio = VTSS_PRIO_START;
            }
            qos->llc_qos.prio = VTSS_PRIO_START;
            qos->dscp_table_no = VTSS_DSCP_TABLE_NO_START;
            for (i = 0; i < 2; i++)
                qos->udp_tcp.local.pair[i].qos.prio = VTSS_PRIO_START;
            qos->ip_proto.qos.prio = VTSS_PRIO_START;
            qos->etype.qos.prio = VTSS_PRIO_START;
            for (i = 0; i < 2; i++)
                qos->vlan[i].qos.prio = VTSS_PRIO_START;
            qos->custom_filter.local.forward = 1;
            qos->custom_filter.local.qos.prio = VTSS_PRIO_START;

            filter = &vtss_state->port_filter[port_no];
            filter->mac_ctrl_enable = 1;
            filter->mac_zero_enable = 1;
            filter->dmac_bc_enable = 1;
            filter->smac_mc_enable = 1;
            filter->untag_enable = 1;
            filter->prio_tag_enable = 1;
            filter->ctag_enable = 1;
            filter->stag_enable = 1;
            filter->max_tags = VTSS_TAG_ANY;
#endif /* VTSS_ARCH_B2 */

#if defined(VTSS_FEATURE_QCL_V1)
            qos->qcl_id = VTSS_QCL_ID_NONE;
#endif /* VTSS_FEAURE_QCL_PORT */
#if defined(VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER)
            qos->tx_rate_limiter.preamble_in_payload = TRUE;
            qos->tx_rate_limiter.payload_rate        = 100;
            qos->tx_rate_limiter.frame_overhead      = 12;
            qos->tx_rate_limiter.frame_rate          = 0x40;
#endif /* VTSS_FEATURE_QOS_DOT3AR_RATE_LIMITER */
        }
    }
#if defined(VTSS_ARCH_B2)
    {
        u32                  i,dscp;
        for (i = 0; i < VTSS_DSCP_TABLE_COUNT; i++)
            for (dscp = 0; dscp < VTSS_DSCP_TABLE_SIZE; dscp++)
                vtss_state->dscp_table[i][dscp].qos.prio = VTSS_PRIO_START;
    }
#endif /* VTSS_ARCH_B2 */

#if defined(VTSS_FEATURE_QCL)
    {
        vtss_qcl_id_t    qcl_id;
        vtss_qcl_t       *qcl;
        vtss_qcl_entry_t *qcl_entry;
        u32              i;

        /* Initialize QCL free/used lists */
        for (qcl_id = VTSS_QCL_ID_START; qcl_id < VTSS_QCL_ID_END; qcl_id++) {
            qcl = &vtss_state->qcl[qcl_id];
            for (i = 0; i < VTSS_QCL_LIST_SIZE; i++) {
                qcl_entry = &qcl->qcl_list[i];
                /* Insert in free list */
                qcl_entry->next = qcl->qcl_list_free;
                qcl->qcl_list_free = qcl_entry;
            }
        }
    }
#endif /* VTSS_FEATURE_QCL */
#endif /* VTSS_FEATURE_QOS */

#if defined(VTSS_FEATURE_PACKET)
    vtss_state->rx_conf.reg.bpdu_cpu_only = 1;
    /* Enabling SFlow queue has side-effects on some platforms (JR-48), so by default we don't. */
    vtss_state->rx_conf.map.sflow_queue = VTSS_PACKET_RX_QUEUE_NONE;
    /* Learn-all frames should only be enabled on JR-stacking and JR-48 platforms. */
    vtss_state->rx_conf.map.lrn_all_queue = VTSS_PACKET_RX_QUEUE_NONE;
    /* Stack-queue frames should only be enabled on JR-stacking platforms. */
    vtss_state->rx_conf.map.stack_queue    = VTSS_PACKET_RX_QUEUE_NONE;
#if defined(VTSS_SW_OPTION_L3RT)
    vtss_state->rx_conf.map.l3_uc_queue    = VTSS_PACKET_RX_QUEUE_NONE;
    vtss_state->rx_conf.map.l3_other_queue = VTSS_PACKET_RX_QUEUE_NONE;
#endif
    /* Enabling Queue 0 at default */
    vtss_state->rx_conf.queue[0].size =  8 * 1024;
#endif /* VTSS_FEATURE_PACKET */

#if defined(VTSS_FEATURE_NPI)
    vtss_state->npi_conf.port_no = VTSS_PORT_NO_NONE;
#endif

#if defined(VTSS_FEATURE_LAYER2)
    {
        vtss_port_no_t        port_no;
        vtss_vlan_port_conf_t *vlan;
        vtss_vid_t            vid;
        vtss_vlan_entry_t     *vlan_entry;
        vtss_mstp_entry_t     *mstp_entry;
        u32                   i;
#if defined(VTSS_FEATURE_AGGR_GLAG)
        u32                   glag_no;
        for (glag_no = VTSS_GLAG_NO_START; glag_no < VTSS_GLAG_NO_END; glag_no++) {
#if defined(VTSS_FEATURE_VSTAX_V1)
            vtss_state->glag_members[glag_no][VTSS_GLAG_PORT_START] = VTSS_PORT_NO_NONE;
#endif /* VTSS_FEATURE_VSTAX_V1 */
#if defined(VTSS_FEATURE_VSTAX_V2)
            vtss_state->glag_conf[glag_no][VTSS_GLAG_PORT_START].entry.upspn = VTSS_UPSPN_NONE;
#endif /* VTSS_FEATURE_VSTAX_V2 */
        }
#endif /* VTSS_FEATURE_AGGR_GLAG */

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        vtss_state->vlan_conf.s_etype = VTSS_ETYPE_TAG_S; /* Default S-tag Ethernet type */
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */

        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            vlan = &vtss_state->vlan_port_conf[port_no];
            vlan->pvid = VTSS_VID_DEFAULT;
            vlan->untagged_vid = VTSS_VID_DEFAULT;
            vtss_state->learn_mode[port_no].automatic = 1;
            vtss_state->stp_state[port_no] = VTSS_STP_STATE_FORWARDING;
            vtss_state->port_aggr_no[port_no] = VTSS_AGGR_NO_NONE;
            vtss_state->auth_state[port_no] = VTSS_AUTH_STATE_BOTH;
            vtss_state->port_forward[port_no] = VTSS_PORT_FORWARD_ENABLED;
#if defined(VTSS_FEATURE_PVLAN)
            vtss_state->pvlan_table[VTSS_PVLAN_NO_DEFAULT].member[port_no] = 1;
            {
                vtss_port_no_t e_port;

                for (e_port = VTSS_PORT_NO_START; e_port < VTSS_PORT_NO_END; e_port++) {
                    vtss_state->apvlan_table[port_no][e_port] = 1;
                }
            }
#endif /* VTSS_FEATURE_PVLAN */
            vtss_state->dgroup_port_conf[port_no].dgroup_no = port_no;
            vtss_state->uc_flood[port_no] = 1;
            vtss_state->mc_flood[port_no] = 1;
            vtss_state->ipv4_mc_flood[port_no] = 1;
            vtss_state->ipv6_mc_flood[port_no] = 1;
            vtss_state->port_protect[port_no].conf.port_no = VTSS_PORT_NO_NONE;
            vtss_state->port_protect[port_no].selector = VTSS_EPS_SELECTOR_WORKING;
#if defined(VTSS_FEATURE_AGGR_GLAG)
            vtss_state->port_glag_no[port_no] = VTSS_GLAG_NO_NONE;
#endif /* VTSS_FEATURE_AGGR_GLAG */
        }

        /* Initialize VLAN table */
        for (vid = 0; vid < VTSS_VIDS; vid++) {
            vlan_entry = &vtss_state->vlan_table[vid];
            vlan_entry->msti = VTSS_MSTI_START;
            if (vid != VTSS_VID_DEFAULT)
                continue;
            vlan_entry->enabled = 1;
            for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++)
                VTSS_PORT_BF_SET(vlan_entry->member, port_no, 1);
        }

        /* All ports are forwarding in first MSTP instance */
        mstp_entry = &vtss_state->mstp_table[VTSS_MSTI_START];
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++)
            mstp_entry->state[port_no] = VTSS_STP_STATE_FORWARDING;

        vtss_state->aggr_mode.smac_enable = 1;

        vtss_state->mirror_conf.port_no = VTSS_PORT_NO_NONE;

        /* Initialize MAC address table */
        vtss_state->mac_table_max = VTSS_MAC_ADDRS;
        for (i = 0; i < VTSS_MAC_ADDRS; i++) {
            /* Insert first in free list */
            vtss_state->mac_table[i].next = vtss_state->mac_list_free;
            vtss_state->mac_list_free = &vtss_state->mac_table[i];
        }
    }
    vtss_state->mac_age_time = 300;
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_VSTAX)
    vtss_state->vstax_conf.port_0       = VTSS_PORT_NO_NONE;
    vtss_state->vstax_conf.port_1       = VTSS_PORT_NO_NONE;
    vtss_state->vstax_info.master_upsid = VTSS_VSTAX_UPSID_UNDEF;
#endif /* VTSS_FEATURE_VSTAX */

#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
    {
        u32                                 i;
        vtss_vlan_trans_grp2vlan_entry_t    *grp_entry;
        vtss_vlan_trans_grp2vlan_t          *grp_conf;
        vtss_vlan_trans_port2grp_entry_t    *port_entry;
        vtss_vlan_trans_port2grp_t          *port_conf;

        /* Initialize VLAN Translation Group free/used lists */
        grp_conf = &vtss_state->vt_trans_conf;
        grp_conf->used = NULL;
        for (i = VTSS_VLAN_TRANS_FIRST_GROUP_ID; i <= VTSS_VLAN_TRANS_MAX_CNT; i++) {
            grp_entry = &grp_conf->trans_list[i - VTSS_VLAN_TRANS_FIRST_GROUP_ID];
            /* Insert in free list */
            grp_entry->next = grp_conf->free;
            grp_conf->free = grp_entry;
        }
        /* Initialize VLAN Translation Port free/used lists */
        port_conf = &vtss_state->vt_port_conf;
        port_conf->used = NULL;
        for (i = VTSS_VLAN_TRANS_FIRST_GROUP_ID; i <= VTSS_VLAN_TRANS_GROUP_MAX_CNT; i++) {
            port_entry = &port_conf->port_list[i - VTSS_VLAN_TRANS_FIRST_GROUP_ID];
            /* Insert in free list */
            port_entry->next = port_conf->free;
            port_conf->free = port_entry;
        }
    }
#endif /* VTSS_FEATURE_VLAN_TRANSLATION */

#if defined(VTSS_FEATURE_ACL)
    {
        vtss_port_no_t    port_no;
        vtss_acl_action_t *action;

        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            action = &vtss_state->acl_port_conf[port_no].action;
#if defined(VTSS_FEATURE_ACL_V1)
            action->forward = 1;
#endif /* VTSS_FEATURE_ACL_V1 */
            action->learn = 1;
        }
    }
#endif /* VTSS_FEATURE_ACL */

#if defined(VTSS_FEATURE_IS0)
    {
        u32               i;
        vtss_is0_info_t   *is0 = &vtss_state->is0;
        vtss_vcap_entry_t *entry;
        
        /* Add IS0 entries to free list */
        is0->obj.name = "IS0";
        if (is0->obj.max_rule_count == 0)
            is0->obj.max_rule_count = is0->obj.max_count;
        for (i = 0; i < is0->obj.max_rule_count; i++) {
            entry = &is0->table[i];
            entry->next = is0->obj.free;
            is0->obj.free = entry;
        }
    }
#endif /* VTSS_FEATURE_IS0 */

#if defined(VTSS_FEATURE_IS1)
    {
        u32               i;
        vtss_is1_info_t   *is1 = &vtss_state->is1;
        vtss_vcap_entry_t *entry;
        
        /* Add IS1 entries to free list */
        is1->obj.name = "IS1";
        if (is1->obj.max_rule_count == 0)
            is1->obj.max_rule_count = is1->obj.max_count;
        for (i = 0; i < is1->obj.max_rule_count; i++) {
            entry = &is1->table[i];
            entry->next = is1->obj.free;
            is1->obj.free = entry;
        }
    }
#endif /* VTSS_FEATURE_IS1 */

#if defined(VTSS_FEATURE_IS2)
    {
        u32               i;
        vtss_is2_info_t   *is2 = &vtss_state->is2;
        vtss_vcap_entry_t *entry;
        
        /* Add IS2 entries to free list */
        is2->obj.name = "IS2";
        if (is2->obj.max_rule_count == 0)
            is2->obj.max_rule_count = is2->obj.max_count;
        for (i = 0; i < is2->obj.max_rule_count; i++) {
            entry = &is2->table[i];
            entry->next = is2->obj.free;
            is2->obj.free = entry;
        }
    }
#endif /* VTSS_FEATURE_IS2 */

#if defined(VTSS_FEATURE_ES0)
    {
        u32               i;
        vtss_es0_info_t   *es0 = &vtss_state->es0;
        vtss_vcap_entry_t *entry;
        
        /* Add ES0 entries to free list */
        es0->obj.name = "ES0";
        if (es0->obj.max_rule_count == 0)
            es0->obj.max_rule_count = es0->obj.max_count;
        for (i = 0; i < es0->obj.max_rule_count; i++) {
            entry = &es0->table[i];
            entry->next = es0->obj.free;
            es0->obj.free = entry;
        }
    }
#endif /* VTSS_FEATURE_ES0 */

#if defined(VTSS_FEATURE_SYNCE)
    {
        u32               i;
        for (i=0; i<VTSS_SYNCE_CLK_PORT_ARRAY_SIZE; i++) {
            vtss_state->old_port_no[i] = 0xFFFFFFFF;
        }
    }
#endif /* VTSS_FEATURE_SYNCE*/

#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
    {
        u32              i;
        vtss_ipmc_info_t *ipmc = &vtss_state->ipmc;
        vtss_ipmc_src_t  *src;
        vtss_ipmc_dst_t  *dst;
        
        ipmc->obj.src_max = VTSS_IPMC_SRC_MAX;
        ipmc->obj.dst_max = VTSS_IPMC_DST_MAX;
        
        /* Add IPMC entries to free list */
        ipmc->obj.name = "IPMC";
        for (i = 0; i < VTSS_IPMC_SRC_MAX; i++) {
            src = &ipmc->src_table[i];
            src->next = ipmc->obj.src_free;
            ipmc->obj.src_free = src;
        }
        for (i = 0; i < VTSS_IPMC_DST_MAX; i++) {
            dst = &ipmc->dst_table[i];
            dst->next = ipmc->obj.dst_free;
            ipmc->obj.dst_free = dst;
        }
    }
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */

#if defined(VTSS_FEATURE_EVC)
    {
        vtss_ece_info_t  *ece_info = &vtss_state->ece_info;
        vtss_ece_entry_t *ece;
        int              i;

#if defined(VTSS_ARCH_SERVAL)
        vtss_evc_info_t  *evc_info = &vtss_state->evc_info;
        vtss_evc_entry_t *evc;
        vtss_port_no_t   port_no;
        vtss_mce_info_t  *mce_info = &vtss_state->mce_info; 
        vtss_mce_entry_t *mce;

        /* Initialize VOE mappings */
        for (i = 0; i < evc_info->max_count; i++) {
            evc = &evc_info->table[i];
            for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
                evc->voe_idx[port_no] = VTSS_EVC_VOE_IDX_NONE;
            }
        }
        
        /* Add ECEs to free list */
        for (i = 0; i < mce_info->max_count; i++) {
            mce = &mce_info->table[i];
            mce->next = mce_info->free;
            mce_info->free = mce;
        }
#endif /* VTSS_ARCH_SERVAL */

        /* Add ECEs to free list */
        for (i = 0; i < ece_info->max_count; i++) {
            ece = &ece_info->table[i];
            ece->next = ece_info->free;
            ece_info->free = ece;
        }
    }
#endif /* VTSS_FEATURE_EVC */

#if defined(VTSS_FEATURE_OAM)
    {
        vtss_oam_voe_conf_t *v;
        int                 i;
        for (i=0; i<VTSS_OAM_VOE_CNT; ++i) {
            v = &vtss_state->oam_voe_conf[i];
            v->voe_type = (i<VTSS_OAM_PATH_SERVICE_VOE_CNT) ? VTSS_OAM_VOE_SERVICE : VTSS_OAM_VOE_PORT;
        }
    }
#endif
    
#if defined(VTSS_SDX_CNT)
    {
        vtss_sdx_info_t  *sdx_info = &vtss_state->sdx_info;
        vtss_sdx_list_t  *isdx_list = &sdx_info->isdx; 
        vtss_sdx_list_t  *esdx_list = &sdx_info->esdx; 
        vtss_sdx_entry_t *sdx;
        int              i;
        
        /* Add SDX entries to free lists */
        for (i = 0; i < sdx_info->max_count; i++) {
            /* ISDX */
            sdx = &isdx_list->table[i];
            sdx->port_no = VTSS_PORT_NO_NONE;
            sdx->sdx = (sdx_info->max_count - i);
            sdx->next = isdx_list->free;
            isdx_list->free = sdx;

            /* ESDX */
            sdx = &esdx_list->table[i];
            sdx->port_no = VTSS_PORT_NO_NONE;
            sdx->sdx = (sdx_info->max_count - i);
            sdx->next = esdx_list->free;
            esdx_list->free = sdx;
        }
    }
#endif /* VTSS_SDX_CNT */

#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
    vtss_state->evc_policer_max = VTSS_EVC_POLICERS;
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */

#if defined(VTSS_FEATURE_MPLS)
    {
        u32 i;

        /* TC mapping and Vprofiles are configured by (chip)_init_conf_set(). */

        vtss_state->mpls_vprofile_free_list = VTSS_MPLS_IDX_UNDEFINED;

        vtss_mpls_idxchain_init();

        for (i = 0; i < VTSS_MPLS_L2_CNT; i++) {
            vtss_state->mpls_l2_conf[i].next_free = i + 1;
            VTSS_MPLS_IDXCHAIN_UNDEF(vtss_state->mpls_l2_conf[i].pub.users);
        }
        vtss_state->mpls_l2_conf[VTSS_MPLS_L2_CNT - 1].next_free = VTSS_MPLS_IDX_UNDEFINED;
        vtss_state->mpls_l2_free_list = 0;

        for (i = 0; i < VTSS_MPLS_SEGMENT_CNT - 1; i++) {
            vtss_state->mpls_segment_conf[i].next_free = i + 1;
        }
        vtss_state->mpls_segment_conf[VTSS_MPLS_SEGMENT_CNT - 1].next_free = VTSS_MPLS_IDX_UNDEFINED;
        vtss_state->mpls_segment_free_list = 0;

        for (i = 0; i < VTSS_MPLS_XC_CNT - 1; i++) {
            vtss_state->mpls_xc_conf[i].next_free = i + 1;
        }
        vtss_state->mpls_xc_conf[VTSS_MPLS_XC_CNT - 1].next_free = VTSS_MPLS_IDX_UNDEFINED;
        vtss_state->mpls_xc_free_list = 0;

        VTSS_MPLS_IDXCHAIN_UNDEF(vtss_state->mpls_is0_mll_free_chain);
        VTSS_MPLS_IDXCHAIN_UNDEF(vtss_state->mpls_encap_free_chain);

        for (i = 0; i < VTSS_MPLS_IN_ENCAP_CNT; i++) {
            vtss_mpls_idxchain_insert_at_head(&vtss_state->mpls_is0_mll_free_chain, i);
        }
        for (i = 0; i < VTSS_MPLS_OUT_ENCAP_CNT; i++) {
            vtss_mpls_idxchain_insert_at_head(&vtss_state->mpls_encap_free_chain, i);
        }
    }
#endif /* VTSS_FEATURE_MPLS */

    // Register access checking is by default disabled.
    // (0 enables it). The reason for choosing this
    // polarity is that the API itself may wish to disable
    // access checking from time to time.
    vtss_state->reg_check.disable_cnt = 1;

    VTSS_D("exit");
    return VTSS_RC_OK;
}

vtss_rc vtss_inst_create(const vtss_inst_create_t *const create,
                         vtss_inst_t              *const inst)
{
    vtss_arch_t arch;
    
    VTSS_D("enter, sizeof(*vtss_state): %u", sizeof(*vtss_state));

    if ((vtss_state = VTSS_OS_MALLOC(sizeof(*vtss_state), VTSS_MEM_FLAGS_NONE)) == NULL)
        return VTSS_RC_ERROR;

    memset(vtss_state, 0, sizeof(*vtss_state));
    vtss_state->cookie = VTSS_STATE_COOKIE;
    vtss_state->create = *create;
    vtss_state->chip_count = 1;

    switch (create->target) {
    case VTSS_TARGET_CU_PHY:
        arch = VTSS_ARCH_CU_PHY;
#if defined(VTSS_FEATURE_WIS) && !defined(VTSS_ARCH_DAYTONA)
        if (vtss_phy_inst_ewis_create() != VTSS_RC_OK)  {
            VTSS_E("Could not hook up ewis functions")
        }
#endif
        break;
    case VTSS_TARGET_10G_PHY:
        arch = VTSS_ARCH_10G_PHY;
#if defined(VTSS_FEATURE_WIS) && !defined(VTSS_ARCH_DAYTONA)
        if (vtss_phy_inst_ewis_create() != VTSS_RC_OK)  {
            VTSS_E("Could not hook up ewis functions")
        }
#endif
        break;
#if defined(VTSS_ARCH_SERVAL)
    case VTSS_TARGET_SERVAL:
    case VTSS_TARGET_SERVAL_LITE:
    case VTSS_TARGET_SPARX_III_11:
    case VTSS_TARGET_SEVILLE:
        arch = VTSS_ARCH_SRVL;
        VTSS_RC(vtss_serval_inst_create());
        break;
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_ARCH_LUTON26)
    case VTSS_TARGET_SPARX_III_10_UM:
    case VTSS_TARGET_SPARX_III_17_UM:
    case VTSS_TARGET_SPARX_III_25_UM:
    case VTSS_TARGET_SPARX_III_10:
    case VTSS_TARGET_SPARX_III_18:
    case VTSS_TARGET_SPARX_III_24:
    case VTSS_TARGET_SPARX_III_26:
    case VTSS_TARGET_SPARX_III_10_01:
    case VTSS_TARGET_CARACAL_1:
    case VTSS_TARGET_CARACAL_2:
        arch = VTSS_ARCH_L26;
        VTSS_RC(vtss_luton26_inst_create());
        break;
#endif /* VTSS_ARCH_LUTON26 */
#if defined(VTSS_ARCH_LUTON28)
    case VTSS_TARGET_SPARX_II_16:
    case VTSS_TARGET_SPARX_II_24:
    case VTSS_TARGET_E_STAX_34:
        arch = VTSS_ARCH_L28;
        VTSS_RC(vtss_luton28_inst_create());
        break;
#endif /* VTSS_ARCH_LUTON28 */
#if defined(VTSS_ARCH_B2)
    case VTSS_TARGET_BARRINGTON_II:
    case VTSS_TARGET_SCHAUMBURG_II:
    case VTSS_TARGET_EXEC_1:
        arch = VTSS_ARCH_BR2;
        VTSS_RC(vtss_b2_inst_create());
        break;
#endif /* VTSS_ARCH_B2 */
#if defined(VTSS_ARCH_DAYTONA)
    case VTSS_TARGET_DAYTONA:
    case VTSS_TARGET_TALLADEGA:
        VTSS_RC(vtss_daytona_inst_create());
        arch = VTSS_ARCH_DAYT;
        VTSS_D("daytona inst create");
        break;
#endif /* VTSS_ARCH_B2 */
#if defined(VTSS_ARCH_JAGUAR_1)
    case VTSS_TARGET_JAGUAR_1_DUAL:
    case VTSS_TARGET_E_STAX_III_24_DUAL:
    case VTSS_TARGET_E_STAX_III_68_DUAL:
        /* Dual device targets */
        vtss_state->chip_count = 2;
        /* FALLTHROUGH */
    case VTSS_TARGET_JAGUAR_1:
    case VTSS_TARGET_LYNX_1:
    case VTSS_TARGET_CE_MAX_24:
    case VTSS_TARGET_CE_MAX_12:
    case VTSS_TARGET_E_STAX_III_48:
    case VTSS_TARGET_E_STAX_III_68:
        arch = VTSS_ARCH_JR1;
        VTSS_RC(vtss_jaguar1_inst_create());
        break;
#endif /* VTSS_ARCH_JAGUAR_1 */
    default:
        VTSS_E("unknown target: 0x%05x", create->target);
        return VTSS_RC_ERROR;
    }

    vtss_state->arch = arch;

    /* Set default configuration */
    VTSS_RC(vtss_inst_default_set());

    /* Setup default instance */
    if (vtss_default_inst == NULL)
        vtss_default_inst = vtss_state;

    /* Return instance to application */
    if (inst != NULL)
        *inst = vtss_state;

    VTSS_D("exit");

    return VTSS_RC_OK;
}

vtss_rc vtss_inst_destroy(const vtss_inst_t inst)
{
    vtss_rc rc;

    VTSS_D("enter");
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (vtss_state == vtss_default_inst)
            vtss_default_inst = NULL;
        VTSS_OS_FREE(vtss_state, VTSS_MEM_FLAGS_NONE);
    }
    VTSS_D("exit");

    return rc;
}

/* Get initialization configuration */
vtss_rc vtss_init_conf_get(const vtss_inst_t        inst,
                           vtss_init_conf_t * const conf)
{
    vtss_rc rc;

    VTSS_D("enter");
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *conf = vtss_state->init_conf;
    VTSS_D("exit");

    return rc;
}

/* Set initialization configuration */
vtss_rc vtss_init_conf_set(const vtss_inst_t              inst,
                           const vtss_init_conf_t * const conf)
{
    vtss_rc rc;

    VTSS_D("enter");
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->init_conf = *conf;
        if (vtss_state->create.target != VTSS_TARGET_CU_PHY &&
            vtss_state->create.target != VTSS_TARGET_10G_PHY) {
            rc = VTSS_FUNC(init_conf_set);
        } 
#if defined(VTSS_CHIP_CU_PHY)
        if (rc == VTSS_RC_OK)
            rc = vtss_phy_init_conf_set();
#endif /* VTSS_CHIP_CU_PHY */
#if defined(VTSS_CHIP_10G_PHY)
        if (rc == VTSS_RC_OK)
            rc = vtss_phy_10g_init_conf_set();
#endif /* VTSS_CHIP_10G_PHY */
#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
        if (rc == VTSS_RC_OK) {
            rc = vtss_fdma_cmn_init_conf_set();
        }
#endif /* defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA */
        vtss_state->warm_start_prev = vtss_state->warm_start_cur;
    } else {
        VTSS_E("Initialization check failed");
    }
    VTSS_D("exit");

    return rc;
}

/* ================================================================= *
 *  Miscellaneous
 * ================================================================= */

/* Get trace configuration */
vtss_rc vtss_trace_conf_get(const vtss_trace_group_t group,
                            vtss_trace_conf_t * const conf)
{
    if (group >= VTSS_TRACE_GROUP_COUNT) {
        VTSS_E("illegal group: %d", group);
        return VTSS_RC_ERROR;
    }

    *conf = vtss_trace_conf[group];
    return VTSS_RC_OK;
}

/* Set trace configuration */
vtss_rc vtss_trace_conf_set(const vtss_trace_group_t group,
                            const vtss_trace_conf_t * const conf)
{
    if (group >= VTSS_TRACE_GROUP_COUNT) {
        VTSS_E("illegal group: %d", group);
        return VTSS_RC_ERROR;
    }

    vtss_trace_conf[group] = *conf;
    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_MISC)
static vtss_rc vtss_inst_chip_no_check(const vtss_inst_t    inst,
                                       const vtss_chip_no_t chip_no)
{
    vtss_rc rc;

    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (chip_no >= vtss_state->chip_count) {
            VTSS_E("%s: illegal chip_no: %u", vtss_func, chip_no);
            rc = VTSS_RC_ERROR;
        } else {
            VTSS_SELECT_CHIP(chip_no);
        }
    }
    return rc;
}

/* Read register */
vtss_rc vtss_reg_read(const vtss_inst_t    inst,
                      const vtss_chip_no_t chip_no,
                      const u32            addr,
                      u32 * const          value)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(reg_read, chip_no, addr, value);
        VTSS_N("addr: 0x%08x, value: 0x%08x", addr, *value);
    }
    VTSS_EXIT();

    return rc;
}


/* Write register */
vtss_rc vtss_reg_write(const vtss_inst_t    inst,
                       const vtss_chip_no_t chip_no,
                       const u32            addr,
                       const u32            value)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(reg_write, chip_no, addr, value);
        VTSS_N("addr: 0x%08x, value: 0x%08x", addr, value);
    }
    VTSS_EXIT();

    return rc;
}

/* Write masked to register */
vtss_rc vtss_reg_write_masked(const vtss_inst_t    inst,
                              const vtss_chip_no_t chip_no,
                              const u32            addr,
                              const u32            value,
                              const u32            mask)
{
    vtss_rc rc;
    u32     val;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK &&
        (rc = VTSS_FUNC(reg_read, chip_no, addr, &val)) == VTSS_RC_OK) {
        val = ((val & ~mask) | (value & mask));
        rc = VTSS_FUNC(reg_write, chip_no, addr, val);
        VTSS_D("addr: 0x%08x, value: 0x%08x", addr, val);
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_chip_id_get(const vtss_inst_t  inst,
                         vtss_chip_id_t     *const chip_id)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(chip_id_get, chip_id);
    VTSS_EXIT();
    return rc;
}



vtss_rc vtss_intr_sticky_clear(const vtss_inst_t    inst,
                               u32                  ext)
{
    return VTSS_FUNC_FROM_STATE(vtss_inst_check_no_persist(inst), intr_sticky_clear, ext);
}


vtss_rc vtss_poll_1sec(const vtss_inst_t  inst)
{
    vtss_rc rc;

#if defined(VTSS_CHIP_10G_PHY)
    rc = vtss_phy_10g_poll_1sec(inst);
#endif

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_rc rc2 = VTSS_FUNC(poll_1sec);
        if (rc == VTSS_RC_OK) {
            rc = rc2;
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ptp_event_poll(const vtss_inst_t      inst,
                            vtss_ptp_event_type_t  *const ev_mask)
{
    vtss_rc rc;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(ptp_event_poll, ev_mask);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ptp_event_enable(const vtss_inst_t            inst,
                              const vtss_ptp_event_type_t  ev_mask,
                              const BOOL                   enable)
{
    vtss_rc rc;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if(rc == VTSS_RC_OK && ev_mask) {
            rc = VTSS_FUNC(ptp_event_enable, ev_mask, enable);
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_dev_all_event_poll(const vtss_inst_t                 inst,
                                const vtss_dev_all_event_poll_t   poll_type,
                                vtss_dev_all_event_type_t         *const ev_mask)
{
    vtss_rc rc;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(dev_all_event_poll, poll_type, ev_mask);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_dev_all_event_enable(const vtss_inst_t                inst,
                                  const vtss_port_no_t             port_no,
                                  const vtss_dev_all_event_type_t  ev_mask,
                                  const BOOL                       enable)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK && ev_mask) {
        rc = VTSS_FUNC(dev_all_event_enable, port_no, ev_mask, enable);
    }
    VTSS_EXIT();
    return rc;
}

#endif /* VTSS_FEATURE_MISC */

#if defined(VTSS_GPIOS)
static vtss_rc vtss_gpio_no_check(const vtss_gpio_no_t  gpio_no)
{
    if (gpio_no > vtss_state->gpio_count) {
        VTSS_E("illegal gpio_no: %u", gpio_no);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_gpio_mode_set(const vtss_inst_t      inst,
                           const vtss_chip_no_t   chip_no,
                           const vtss_gpio_no_t   gpio_no,
                           const vtss_gpio_mode_t mode)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK &&
        (rc = vtss_gpio_no_check(gpio_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(gpio_mode, chip_no, gpio_no, mode);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_gpio_direction_set(const vtss_inst_t     inst,
                                const vtss_chip_no_t  chip_no,
                                const vtss_gpio_no_t  gpio_no,
                                const BOOL            output)
{
    /* Use general form: vtss_gpio_mode_set() */
    return vtss_gpio_mode_set(inst, chip_no, gpio_no, output ? VTSS_GPIO_OUT : VTSS_GPIO_IN);
}

vtss_rc vtss_gpio_read(const vtss_inst_t     inst,
                       const vtss_chip_no_t  chip_no,
                       const vtss_gpio_no_t  gpio_no,
                       BOOL                  *const value)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK &&
        (rc = vtss_gpio_no_check(gpio_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(gpio_read, chip_no, gpio_no, value);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_gpio_write(const vtss_inst_t     inst,
                        const vtss_chip_no_t  chip_no,
                        const vtss_gpio_no_t  gpio_no,
                        const BOOL            value)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK &&
        (rc = vtss_gpio_no_check(gpio_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(gpio_write, chip_no, gpio_no, value);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_gpio_event_poll(const vtss_inst_t        inst,
                             const vtss_chip_no_t     chip_no,
                             BOOL                     *const events)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(gpio_event_poll, chip_no, events);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_gpio_event_enable(const vtss_inst_t       inst,
                               const vtss_chip_no_t    chip_no,
                               const vtss_gpio_no_t    gpio_no,
                               BOOL                    enable)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK &&
        (rc = vtss_gpio_no_check(gpio_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(gpio_event_enable, chip_no, gpio_no, enable);
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_ARCH_B2)
vtss_rc vtss_gpio_clk_set(const vtss_inst_t            inst,
                          const vtss_gpio_no_t         gpio_no,
                          const vtss_port_no_t         port_no,
                          const vtss_recovered_clock_t clk)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_gpio_no_check(gpio_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(gpio_clk_set, gpio_no, port_no, clk);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_B2 */
#endif /* GPIOS */

/* - Serial LED ---------------------------------------------------- */

#if defined(VTSS_FEATURE_SERIAL_LED)
vtss_rc vtss_serial_led_set(const vtss_inst_t      inst,
                            const vtss_led_port_t  port,
                            const vtss_led_mode_t  *const mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(serial_led_set, port, mode);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_serial_led_intensity_set(const vtss_inst_t      inst,
                                      const vtss_led_port_t  port,
                                      const i8               intensity)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(serial_led_intensity_set, port, intensity);
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_serial_led_intensity_get(const vtss_inst_t      inst,
                                      i8               *intensity)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(serial_led_intensity_get, intensity);
    VTSS_EXIT();
    return rc;
}
 

#endif /* VTSS_FEATURE_SERIAL_LED */


/* - Serial IO control ---------------------------------------------------- */

#if defined(VTSS_FEATURE_SERIAL_GPIO)
static vtss_rc vtss_sgpio_group_check(const vtss_sgpio_group_t group)
{
    if (group >= vtss_state->sgpio_group_count) {
        VTSS_E("illegal SGPIO group: %u", group);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_sgpio_conf_get(const vtss_inst_t        inst,
                            const vtss_chip_no_t     chip_no,
                            const vtss_sgpio_group_t group,
                            vtss_sgpio_conf_t        *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK &&
        (rc = vtss_sgpio_group_check(group)) == VTSS_RC_OK) {
        *conf = vtss_state->sgpio_conf[chip_no][group];
    }
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_sgpio_conf_set(const vtss_inst_t        inst,
                            const vtss_chip_no_t     chip_no,
                            const vtss_sgpio_group_t group,
                            const vtss_sgpio_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK &&
        (rc = vtss_sgpio_group_check(group)) == VTSS_RC_OK) {
        if (conf->bit_count == 0 || conf->bit_count > 4) {
            VTSS_E("illegal bit_count: %u", conf->bit_count);
            rc = VTSS_RC_ERROR;
        } else {
            vtss_state->sgpio_conf[chip_no][group] = *conf;
            rc = VTSS_FUNC(sgpio_conf_set, chip_no, group, conf);
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_sgpio_read(const vtss_inst_t        inst,
                        const vtss_chip_no_t     chip_no,
                        const vtss_sgpio_group_t group,
                        vtss_sgpio_port_data_t   data[VTSS_SGPIO_PORTS])
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK &&
        (rc = vtss_sgpio_group_check(group)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(sgpio_read, chip_no, group, data);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_sgpio_event_poll(const vtss_inst_t        inst,
                              const vtss_chip_no_t     chip_no,
                              const vtss_sgpio_group_t group,
                              const u32                bit,
                              BOOL                     *const events)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK && 
        (rc = vtss_sgpio_group_check(group)) == VTSS_RC_OK) {
        if (bit >= 4) {
            VTSS_E("illegal parameter  bit: %u", bit);
            rc = VTSS_RC_ERROR;
        } else {
            rc = VTSS_FUNC(sgpio_event_poll, chip_no, group, bit, events);
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_sgpio_event_enable(const vtss_inst_t        inst,
                                const vtss_chip_no_t     chip_no,
                                const vtss_sgpio_group_t group,
                                const vtss_port_no_t     port,
                                const u32                bit,
                                BOOL                     enable)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK && 
        (rc = vtss_sgpio_group_check(group)) == VTSS_RC_OK) {
        if ((port >= VTSS_SGPIO_PORTS) || (bit >= 4)) {
            VTSS_E("illegal parameter  port: %u  bit: %u", port, bit);
            rc = VTSS_RC_ERROR;
        } else {
            vtss_state->sgpio_event_enable[group].enable[port][bit] = enable;
            rc = VTSS_FUNC(sgpio_event_enable, chip_no, group, port, bit, enable);
        }
    }
    VTSS_EXIT();
    return rc;
}


#endif /* VTSS_FEATURE_SERIAL_GPIO */


/* ================================================================= *
 *  Port control
 * ================================================================= */

const char *vtss_port_if_txt(vtss_port_interface_t if_type)
{
    switch (if_type) {
    case VTSS_PORT_INTERFACE_NO_CONNECTION: return "N/C";
    case VTSS_PORT_INTERFACE_LOOPBACK:      return "LOOPBACK";
    case VTSS_PORT_INTERFACE_INTERNAL:      return "INTERNAL";
    case VTSS_PORT_INTERFACE_MII:           return "MII";
    case VTSS_PORT_INTERFACE_GMII:          return "GMII";
    case VTSS_PORT_INTERFACE_RGMII:         return "RGMII";
    case VTSS_PORT_INTERFACE_TBI:           return "TBI";
    case VTSS_PORT_INTERFACE_RTBI:          return "RTBI";
    case VTSS_PORT_INTERFACE_SGMII:         return "SGMII";
    case VTSS_PORT_INTERFACE_SERDES:        return "SERDES";
    case VTSS_PORT_INTERFACE_VAUI:          return "VAUI";
    case VTSS_PORT_INTERFACE_100FX:         return "100FX";
    case VTSS_PORT_INTERFACE_XAUI:          return "XAUI";
    case VTSS_PORT_INTERFACE_RXAUI:         return "RXAUI";
    case VTSS_PORT_INTERFACE_XGMII:         return "XGMII";
    case VTSS_PORT_INTERFACE_SPI4:          return "SPI4";
    case VTSS_PORT_INTERFACE_SGMII_CISCO:   return "SGMII_CISCO";
    case VTSS_PORT_INTERFACE_QSGMII:        return "QSGMII";
    }
    return "????";
}



#if defined(VTSS_FEATURE_PORT_CONTROL)
/* Get port map */
vtss_rc vtss_port_map_get(const vtss_inst_t  inst,
                          vtss_port_map_t    port_map[VTSS_PORT_ARRAY_SIZE])
{
    vtss_port_no_t port_no;
    vtss_rc        rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++)
            port_map[port_no] = vtss_state->port_map[port_no];
    }
    VTSS_EXIT();
    VTSS_D("exit");

    return rc;
}

/* Set port map */
vtss_rc vtss_port_map_set(const vtss_inst_t      inst,
                          const vtss_port_map_t  port_map[VTSS_PORT_ARRAY_SIZE])
{
    vtss_port_no_t  port_no;
    vtss_rc         rc;
    vtss_port_map_t *pmap;

    VTSS_D("enter");
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            pmap = &vtss_state->port_map[port_no];
            *pmap = port_map[port_no];

            /* For single chip targets, the chip_no is set to zero */
            if (vtss_state->chip_count < 2) {
                pmap->chip_no = 0;
                pmap->miim_chip_no = 0;
            }

            /* Port numbers greater than or equal to the first unmapped port are ignored */
            if (port_map[port_no].chip_port < 0 && port_no < vtss_state->port_count)
                vtss_state->port_count = port_no;
        }

#if defined(VTSS_FEATURE_LAYER2)
        /* Initialize PGID table */
        {
            u32               pgid;
            vtss_pgid_entry_t *pgid_entry;

            /* The first entries are reserved for forwarding to single port (unicast) */
            for (pgid = 0; pgid < vtss_state->port_count; pgid++) {
                pgid_entry = &vtss_state->pgid_table[pgid];
                pgid_entry->member[pgid] = 1;
                pgid_entry->references = 1;
            }

            /* Next entry is reserved for dropping */
            vtss_state->pgid_drop = pgid;
            vtss_state->pgid_table[pgid].references = 1;

            /* Next entry is reserved for flooding */
            pgid++;
            vtss_state->pgid_flood = pgid;
            pgid_entry = &vtss_state->pgid_table[pgid];
            pgid_entry->references = 1;
            for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
                pgid_entry->member[port_no] = 1;
        }
#endif /* VTSS_FEATURE_LAYER2 */
    }
    rc = VTSS_FUNC(port_map_set);
#if defined(VTSS_FEATURE_LAYER2)
    if (rc == VTSS_RC_OK) /* Update destination masks */
        rc = vtss_update_masks(0, 1, 0);
#endif /* VTSS_FEATURE_LAYER2 */
    VTSS_D("exit");

    return rc;
}

#if defined(VTSS_FEATURE_10G) || defined(VTSS_CHIP_10G_PHY) 
vtss_rc vtss_port_mmd_read(const vtss_inst_t     inst,
                           const vtss_port_no_t  port_no,
                           const u8              mmd,
                           const u16             addr,
                           u16                   *const value)
{
    vtss_rc                rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        rc = vtss_mmd_reg_read(port_no,mmd,addr,value);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_mmd_read_inc(const vtss_inst_t     inst,
                           const vtss_port_no_t  port_no,
                           const u8              mmd,
                           const u16             addr,
                           u16                   *const buf,
                           u8                    count)
{
    vtss_rc                rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        rc = vtss_mmd_reg_read_inc(port_no,mmd,addr,buf,count);
    }
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_port_mmd_write(const vtss_inst_t     inst,
                            const vtss_port_no_t  port_no,
                            const u8              mmd,
                            const u16             addr,
                            const u16             value)
{
    vtss_rc                rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        rc = vtss_mmd_reg_write(port_no,mmd,addr,value);
    }

    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_mmd_masked_write(const vtss_inst_t     inst,
                                   const vtss_port_no_t  port_no,
                                   const u8              mmd,
                                   const u16             addr,
                                   const u16             value,
                                   const u16             mask)
{
    vtss_rc                rc;
    vtss_miim_controller_t miim_controller;
    u8                     miim_addr;
    u16                    val;

    VTSS_ENTER();
    if ((rc = vtss_miim_check(port_no, 0, &miim_controller, &miim_addr)) == VTSS_RC_OK &&
        (rc = VTSS_FUNC(mmd_read, miim_controller, miim_addr, mmd, addr, &val, TRUE)) == VTSS_RC_OK) {
        val = ((val & ~mask) | (value & mask));
        rc = VTSS_FUNC(mmd_write, miim_controller, miim_addr, mmd, addr, val, TRUE);
        if (rc == VTSS_RC_OK) {
            VTSS_N("port_no: %u, mmd: %u, addr: 0x%04x, val: 0x%04x", port_no, mmd, addr, val);
        }
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_10G || VTSS_CHIP_10G_PHY */

#if defined(VTSS_FEATURE_CLAUSE_37)
vtss_rc vtss_port_clause_37_control_set(const vtss_inst_t                    inst,
                                        const vtss_port_no_t                 port_no,
                                        const vtss_port_clause_37_control_t  *const control)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->port_clause_37[port_no] = *control;
        rc = VTSS_FUNC_COLD(port_clause_37_control_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_clause_37_control_get(const vtss_inst_t              inst,
                                        const vtss_port_no_t           port_no,
                                        vtss_port_clause_37_control_t  *const control)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *control = vtss_state->port_clause_37[port_no];
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_CLAUSE_37 */

vtss_rc vtss_port_conf_get(const vtss_inst_t     inst,
                           const vtss_port_no_t  port_no,
                           vtss_port_conf_t      *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_conf_set(const vtss_inst_t       inst,
                           const vtss_port_no_t    port_no,
                           const vtss_port_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->port_conf[port_no] = *conf;
#if defined(VTSS_FEATURE_LAYER2)
        /* Update aggregation masks depending on power_down state */
        if ((rc = vtss_update_masks(0, 0, 1)) == VTSS_RC_OK)
#endif /* VTSS_FEATURE_LAYER2 */
            rc = VTSS_FUNC_COLD(port_conf_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_ARCH_SERVAL)
vtss_rc vtss_port_ifh_conf_set(const vtss_inst_t       inst,
                               const vtss_port_no_t    port_no,
                               const vtss_port_ifh_t  *const conf)
{
    vtss_rc rc;
    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->port_ifh_conf[port_no] = *conf;
        rc = VTSS_FUNC_COLD(port_ifh_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_ifh_conf_get(const vtss_inst_t     inst,
                               const vtss_port_no_t  port_no,
                               vtss_port_ifh_t      *const conf)
{
    vtss_rc rc;
    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        *conf = vtss_state->port_ifh_conf[port_no];
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_SERVAL */


#if defined(VTSS_ARCH_B2)
vtss_rc vtss_host_conf_get(const vtss_inst_t     inst,
                           const vtss_port_no_t  port_no,
                           vtss_host_conf_t      *const conf)
{
    vtss_rc rc;
    u32      chip_port = vtss_state->port_map[port_no].chip_port;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if (port_no == VTSS_PORT_NO_NONE) {
        conf->spi4 = vtss_state->spi4;
        conf->xaui = vtss_state->xaui[0];
        rc = VTSS_RC_OK;
    } else {
        if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
                if (chip_port > 23) {
                    if (chip_port == 26)
                        conf->spi4 = vtss_state->spi4;
                    else
                        conf->xaui = vtss_state->xaui[chip_port-24];
                } else {
                    VTSS_E("Not host port: %u", chip_port);
                    rc = VTSS_RC_ERROR;
                }
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_host_conf_set(const vtss_inst_t       inst,
                           const vtss_port_no_t    port_no,
                           const vtss_host_conf_t  *const conf)
{
    vtss_rc rc;
    u32      chip_port = vtss_state->port_map[port_no].chip_port;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        if (chip_port > 23) {
            if (chip_port == 26)
                vtss_state->spi4 = conf->spi4;
            else
                vtss_state->xaui[chip_port-24] = conf->xaui;
            
            rc = VTSS_FUNC(host_conf_set, port_no);
        } else {
            VTSS_E("Not host port: %u", chip_port);
            rc = VTSS_RC_ERROR;
        }
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_B2 */

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
vtss_rc vtss_host_conf_get(const vtss_inst_t     inst,
                           const vtss_port_no_t  port_no,
                           vtss_host_conf_t      *const conf)
{
    vtss_rc rc;
    u32      chip_port = vtss_state->port_map[port_no].chip_port;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        if (chip_port > 26 && chip_port < 31) {
            conf->xaui = vtss_state->xaui[chip_port-27];
        } else {
            VTSS_E("Not host port: %u", chip_port);
            rc = VTSS_RC_ERROR;
        }
    }            
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_host_conf_set(const vtss_inst_t       inst,
                           const vtss_port_no_t    port_no,
                           const vtss_host_conf_t  *const conf)
{
    vtss_rc rc;
    u32      chip_port = vtss_state->port_map[port_no].chip_port;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        if (chip_port > 26 && chip_port < 31) {
            vtss_state->xaui[chip_port-27] = conf->xaui;                
            rc = VTSS_FUNC(host_conf_set, port_no);
        } else {
            VTSS_E("Not host port: %u", chip_port);
            rc = VTSS_RC_ERROR;
        }
    }    
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

#if defined(VTSS_ARCH_B2) 
vtss_rc vtss_port_status_interface_set(const vtss_inst_t       inst,
                                       const u32              clock)
{
    vtss_rc rc;
    VTSS_ENTER();
    rc = VTSS_FUNC(port_status_interface_set, clock);
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_port_status_interface_get(const vtss_inst_t       inst,
                                       u32             *clock)
{
    vtss_rc rc;
    VTSS_ENTER();
    rc = VTSS_FUNC(port_status_interface_get, clock);
    VTSS_EXIT();
    return rc;
}

#endif //(VTSS_ARCH_B2)

#if defined(VTSS_ARCH_JAGUAR_1_CE_MAC)
BOOL vtss_port_is_host(const vtss_inst_t     inst,
                       const vtss_port_no_t  port_no)
{
    BOOL is_host = FALSE;

    VTSS_ENTER();
    do {
        if (vtss_inst_port_no_check(inst, port_no) != VTSS_RC_OK) {
            VTSS_E("Illegal instance");
            break;
        }
        is_host = VTSS_FUNC(port_is_host, port_no);
    } while (0); /* end of do-while */
    VTSS_EXIT();

    return is_host;
}


/* Get QoS setup for logical port */
vtss_rc vtss_qos_lport_conf_get(const vtss_inst_t      inst,
                                const vtss_lport_no_t  lport_no,
                                vtss_qos_lport_conf_t  *const conf)
{
    vtss_rc rc;

    if (conf == NULL) {
        return VTSS_RC_ERROR;
    }

    VTSS_ENTER();
    if (lport_no >= VTSS_LPORTS)
        rc = VTSS_RC_ERROR;
    else {
        if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
            *conf = vtss_state->qos_lport[lport_no];
    }
    VTSS_EXIT();
    return rc;
}

/* Set QoS setup for logical port */
vtss_rc vtss_qos_lport_conf_set(const vtss_inst_t            inst,
                                const vtss_lport_no_t        lport_no,
                                const vtss_qos_lport_conf_t  *const conf)
{
    vtss_rc rc;

    if (conf == NULL) {
        return VTSS_RC_ERROR;
    }

    VTSS_ENTER();
    if (lport_no >= VTSS_LPORTS)
        rc = VTSS_RC_ERROR;
    else {
        if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
            vtss_state->qos_lport[lport_no] = *conf;
            rc = VTSS_FUNC(qos_lport_conf_set, lport_no);
        }
    }
    VTSS_EXIT();
    return rc;
}

/* Get Lport statistics */
vtss_rc vtss_lport_counters_get(const vtss_inst_t      inst,
                                const vtss_lport_no_t  lport_no,
                                vtss_lport_counters_t  *const counters)
{
    vtss_rc rc;

    if (counters == NULL) {
        return VTSS_RC_ERROR;
    }

    VTSS_ENTER();
    if (lport_no >= VTSS_LPORTS)
        rc = VTSS_RC_ERROR;
    else {
        if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
            rc = VTSS_FUNC(lport_counters_get, lport_no, counters);
        }
    }
    VTSS_EXIT();
    return rc;

}

/* Clear Lport statistics */
vtss_rc vtss_lport_counters_clear(const vtss_inst_t     inst,
                                  const vtss_lport_no_t lport_no)
{
    vtss_rc rc;

    VTSS_ENTER();
    if (lport_no >= VTSS_LPORTS)
        rc = VTSS_RC_ERROR;
    else {
        if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
            rc = VTSS_FUNC(lport_counters_clear, lport_no);
        }
    }
    VTSS_EXIT();
    return rc;

}



#endif /* VTSS_ARCH_JAGUAR_1_CE_MAC */

#if defined(VTSS_FEATURE_CLAUSE_37)
static vtss_rc vtss_port_clause_37_status_get(const vtss_port_no_t port_no,
                                              vtss_port_status_t   *const status)
{
    vtss_rc                       rc;
    vtss_port_clause_37_status_t  clause_37_status;
    vtss_port_clause_37_control_t *control;
    vtss_port_clause_37_adv_t     *adv, *lp;

    VTSS_N("port_no: %u", port_no);
    if ((rc = VTSS_FUNC(port_clause_37_status_get, port_no, &clause_37_status)) != VTSS_RC_OK)
        return rc;
    status->link_down = (clause_37_status.link ? 0 : 1);
    status->aneg_complete = clause_37_status.autoneg.complete;

    if (status->link_down) {
        /* Link has been down, so get the current status */
        if ((rc = VTSS_FUNC(port_clause_37_status_get, port_no,
                            &clause_37_status)) != VTSS_RC_OK)
            return rc;
        status->link = clause_37_status.link;
    } else {
        /* Link is still up */
        status->link = 1;
    }

    /* Link is down */
    if (status->link == 0)
        return VTSS_RC_OK;

    /* Link is up */
    if (vtss_state->port_conf[port_no].if_type == VTSS_PORT_INTERFACE_SGMII_CISCO) {
        /* The SGMII aneg status is a status from SFP-Phy on the copper side */
        if (clause_37_status.autoneg.complete) {
            if (clause_37_status.autoneg.partner_adv_sgmii.speed_1G) {                 
                status->speed = VTSS_SPEED_1G;
            } else if (clause_37_status.autoneg.partner_adv_sgmii.speed_100M) {
                status->speed = VTSS_SPEED_100M;
            } else {
                status->speed = VTSS_SPEED_10M;
            }
            status->fdx = clause_37_status.autoneg.partner_adv_sgmii.fdx;        
            /* Flow control is not supported by SGMII aneg. */
            status->aneg.obey_pause = 0;
            status->aneg.generate_pause = 0;
        } else {
            status->link = 0;
        }
    } else {        
        control = &vtss_state->port_clause_37[port_no];
        if (control->enable) {
            /* Auto-negotiation enabled */
            adv = &control->advertisement;
            lp = &clause_37_status.autoneg.partner_advertisement;
            if (clause_37_status.autoneg.complete) {
                /* Speed and duplex mode auto negotiation result */
                if (adv->fdx && lp->fdx) {
                    status->speed = VTSS_SPEED_1G;
                    status->fdx = 1;
                } else if (adv->hdx && lp->hdx) {
                    status->speed = VTSS_SPEED_1G;
                    status->fdx = 0;
                } else
                    status->link = 0;
                
                /* Flow control auto negotiation result */
                status->aneg.obey_pause =
                    (adv->symmetric_pause &&
                     (lp->symmetric_pause ||
                      (adv->asymmetric_pause && lp->asymmetric_pause)) ? 1 : 0);
                status->aneg.generate_pause =
                    (lp->symmetric_pause &&
                     (adv->symmetric_pause ||
                      (adv->asymmetric_pause && lp->asymmetric_pause)) ? 1 : 0);
                
                /* Remote fault */
                if (lp->remote_fault != VTSS_PORT_CLAUSE_37_RF_LINK_OK)
                    status->remote_fault = 1;
            } else {
                /* Autoneg says that the link partner is not OK */
                status->link = 0;
            }
        } else {
            /* Forced speed */
            status->speed = VTSS_SPEED_1G;
            status->fdx = 1;
        }
    }

    return VTSS_RC_OK;

}
#endif /* VTSS_FEATURE_CLAUSE_37 */


vtss_rc vtss_port_status_get(const vtss_inst_t     inst,
                             const vtss_port_no_t  port_no,
                             vtss_port_status_t    *const status)
{
    vtss_rc rc;

    VTSS_N("port_no: %u", port_no);

    /* Initialize status */
    memset(status, 0, sizeof(*status));
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        switch (vtss_state->port_conf[port_no].if_type) {
        case VTSS_PORT_INTERFACE_SGMII:
        case VTSS_PORT_INTERFACE_QSGMII:
#if defined(VTSS_CHIP_CU_PHY)
            rc = vtss_phy_status_get_private(port_no, status);
#endif /* VTSS_CHIP_CU_PHY */
            break;
        case VTSS_PORT_INTERFACE_SERDES:
        case VTSS_PORT_INTERFACE_SGMII_CISCO:
#if defined(VTSS_FEATURE_CLAUSE_37)
            rc = vtss_port_clause_37_status_get(port_no, status);
#endif /* VTSS_FEATURE_CLAUSE_37 */
            break;
        default:
            rc = VTSS_FUNC(port_status_get, port_no, status);
            break;
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_counters_update(const vtss_inst_t    inst,
                                  const vtss_port_no_t port_no)
{
    vtss_rc rc;

    VTSS_N("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(port_counters_update, port_no);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_counters_clear(const vtss_inst_t    inst,
                                 const vtss_port_no_t port_no)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(port_counters_clear, port_no);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_counters_get(const vtss_inst_t    inst,
                               const vtss_port_no_t port_no,
                               vtss_port_counters_t *const counters)
{
    vtss_rc rc;

    VTSS_N("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(port_counters_get, port_no, counters);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_basic_counters_get(const vtss_inst_t     inst,
                                     const vtss_port_no_t  port_no,
                                     vtss_basic_counters_t *const counters)
{
    vtss_rc rc;

    VTSS_N("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(port_basic_counters_get, port_no, counters);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_PORT_CONTROL */

/* ================================================================= *
 *  Filter
 * ================================================================= */

#if defined(VTSS_ARCH_B2)

vtss_rc vtss_port_filter_get(const vtss_inst_t     inst,
                             const vtss_port_no_t  port_no,
                             vtss_port_filter_t    *const filter)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *filter = vtss_state->port_filter[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_filter_set(const vtss_inst_t         inst,
                             const vtss_port_no_t      port_no,
                             const vtss_port_filter_t  *const filter)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->port_filter[port_no] = *filter;
        rc = VTSS_FUNC(port_filter_set, port_no, filter);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_B2 */

/* ================================================================= *
 *  QoS
 * ================================================================= */

#if defined(VTSS_FEATURE_QOS)

#if defined(VTSS_ARCH_CARACAL)
vtss_rc vtss_mep_policer_conf_get(const vtss_inst_t       inst,
                                  const vtss_port_no_t    port_no,
                                  const vtss_prio_t       prio,
                                  vtss_dlb_policer_conf_t *const conf)
{
    VTSS_D("port: %u, prio: %u", port_no, prio);
    memset(conf, 0, sizeof(*conf));
    return VTSS_RC_OK;
}

vtss_rc vtss_mep_policer_conf_set(const vtss_inst_t             inst,
                                  const vtss_port_no_t          port_no,
                                  const vtss_prio_t             prio,
                                  const vtss_dlb_policer_conf_t *const conf)
{
    VTSS_D("port: %u, prio: %u", port_no, prio);
    return VTSS_RC_OK;
}
#endif /* defined(VTSS_ARCH_CARACAL) */

vtss_rc vtss_qos_conf_get(const vtss_inst_t  inst,
                          vtss_qos_conf_t    *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *conf = vtss_state->qos_conf;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_qos_conf_set(const vtss_inst_t      inst,
                          const vtss_qos_conf_t  *const conf)
{
    vtss_rc     rc;
    vtss_prio_t prios = conf->prios;
    BOOL        changed;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (prios == 1 || prios == 2 || prios == 4 || prios == vtss_state->prio_count) {
            changed = (vtss_state->qos_conf.prios != prios);
            vtss_state->qos_conf = *conf;
            rc = VTSS_FUNC_COLD(qos_conf_set, changed);
        } else {
            VTSS_E("illegal prios: %u", prios);
            rc = VTSS_RC_ERROR;
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_qos_port_conf_get(const vtss_inst_t     inst,
                               const vtss_port_no_t  port_no,
                               vtss_qos_port_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->qos_port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_qos_port_conf_set(const vtss_inst_t           inst,
                               const vtss_port_no_t        port_no,
                               const vtss_qos_port_conf_t  *const conf)
{
    vtss_rc              rc;
    vtss_qos_port_conf_t *port_conf;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        port_conf = &vtss_state->qos_port_conf[port_no];
        vtss_state->qos_port_conf_old = *port_conf;
        *port_conf = *conf;
        rc = VTSS_FUNC_COLD(qos_port_conf_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_ARCH_B2)
vtss_rc vtss_dscp_table_get(const vtss_inst_t           inst,
                            const vtss_dscp_table_no_t  table_no,
                            vtss_dscp_entry_t           dscp_table[VTSS_DSCP_TABLE_SIZE])
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (table_no < VTSS_DSCP_TABLE_NO_END)
            memcpy(dscp_table, vtss_state->dscp_table[table_no], sizeof(vtss_dscp_entry_t)*VTSS_DSCP_TABLE_SIZE);
        else
            rc = VTSS_RC_ERROR;
    }
    VTSS_EXIT();
    return rc;

}

vtss_rc vtss_dscp_table_set(const vtss_inst_t           inst,
                            const vtss_dscp_table_no_t  table_no,
                            const vtss_dscp_entry_t     dscp_table[VTSS_DSCP_TABLE_SIZE])
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        memcpy(vtss_state->dscp_table[table_no], dscp_table, sizeof(vtss_dscp_entry_t)*VTSS_DSCP_TABLE_SIZE);
        rc = VTSS_FUNC(dscp_table_set, table_no);
    }
    VTSS_EXIT();
    return rc;
}


/* Get QoS setup for logical port */
vtss_rc vtss_qos_lport_conf_get(const vtss_inst_t       inst,
                                const vtss_lport_no_t   lport_no,
                                vtss_qos_lport_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if (lport_no >= VTSS_LPORTS)
        rc = VTSS_RC_ERROR;
    else {
        if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
            *conf = vtss_state->qos_lport[lport_no];
    }
    VTSS_EXIT();
    return rc;
}

/* Set QoS setup for logical port */
vtss_rc vtss_qos_lport_conf_set(const vtss_inst_t             inst,
                                const vtss_lport_no_t         lport_no,
                                const vtss_qos_lport_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if (lport_no >= VTSS_LPORTS)
        rc = VTSS_RC_ERROR;
    else {
        if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
            vtss_state->qos_lport[lport_no] = *conf;
            rc = VTSS_FUNC(qos_lport_conf_set, lport_no);
        }
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_B2 */

#if defined(VTSS_FEATURE_QCL)
vtss_rc vtss_qce_init(const vtss_inst_t      inst,
                      const vtss_qce_type_t  type,
                      vtss_qce_t             *const qce)
{
    VTSS_D("type: %d", type);

    memset(qce, 0, sizeof(*qce));
#if defined(VTSS_FEATURE_QCL_V1)
    qce->type = type;
#endif /* VTSS_FEATURE_QCL_V1 */
#if defined(VTSS_FEATURE_QCL_V2)
    qce->key.type = type;
#endif /* VTSS_FEATURE_QCL_V2 */

    return VTSS_RC_OK;
}

vtss_rc vtss_qce_add(const vtss_inst_t    inst,
                     const vtss_qcl_id_t  qcl_id,
                     const vtss_qce_id_t  qce_id,
                     const vtss_qce_t     *const qce)
{
    vtss_rc rc;

    VTSS_D("qce_id: %u before %u %s",
           qce->id, qce_id, qce_id == VTSS_QCE_ID_LAST ? "(last)" : "");

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(qce_add, qcl_id, qce_id, qce);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_qce_del(const vtss_inst_t    inst,
                     const vtss_qcl_id_t  qcl_id,
                     const vtss_qce_id_t  qce_id)
{
    vtss_rc rc;

    VTSS_D("qce_id: %u", qce_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(qce_del, qcl_id, qce_id);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_QCL */
#endif /* VTSS_FEATURE_QOS */

/* ================================================================= *
 *  Layer 2
 * ================================================================= */

#if defined(VTSS_FEATURE_LAYER2)

/* Determine port membership considering aggregations etc. */
static void vtss_pgid_members_get(u32 pgid, BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_pgid_entry_t *pgid_entry;
    vtss_port_eps_t   *protect;
    vtss_port_no_t    port_no, port;
    vtss_aggr_no_t    aggr_no;
    vtss_dgroup_no_t  dgroup_no;

    /* Store raw port members */
    pgid_entry = &vtss_state->pgid_table[pgid];
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++)
        member[port_no] = (port_no < vtss_state->port_count ? pgid_entry->member[port_no] : 0);

    /* Reserved entries are used direcly (e.g. GLAG masks) */
    if (pgid_entry->resv)
        return;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        /* Check if 1+1 protected port is member */
        protect = &vtss_state->port_protect[port_no];
        if ((port = protect->conf.port_no) != VTSS_PORT_NO_NONE &&
            protect->conf.type == VTSS_EPS_PORT_1_PLUS_1) {
            if (member[port])    /* Include working port if protection port is member */
                member[port_no] = 1;
            if (member[port_no]) /* Include protection port if working port is member */
                member[port] = 1;
        }

        /* Check if aggregated ports or destination group members are port members */
        aggr_no = vtss_state->port_aggr_no[port_no];
        dgroup_no = vtss_state->dgroup_port_conf[port_no].dgroup_no;
        for (port = VTSS_PORT_NO_START; port < vtss_state->port_count; port++) {
            if (member[port] &&
                ((aggr_no != VTSS_AGGR_NO_NONE && vtss_state->port_aggr_no[port] == aggr_no) ||
                 vtss_state->dgroup_port_conf[port].dgroup_no == dgroup_no)) {
                member[port_no] = 1;
            }
        }
    }
#if defined VTSS_FEATURE_VSTAX_V2
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if ((aggr_no = vtss_state->port_glag_no[port_no]) == VTSS_GLAG_NO_NONE)
            continue;
        for (port = VTSS_PORT_NO_START; port < vtss_state->port_count; port++)
            if (vtss_state->port_glag_no[port] == aggr_no && member[port])
                member[port_no] = 1;
    }
#endif       
}

/* - MAC address table --------------------------------------------- */

void vtss_mach_macl_get(const vtss_vid_mac_t *vid_mac, u32 *mach, u32 *macl)
{
    *mach = ((vid_mac->vid<<16) | (vid_mac->mac.addr[0]<<8) | vid_mac->mac.addr[1]);
    *macl = ((vid_mac->mac.addr[2]<<24) | (vid_mac->mac.addr[3]<<16) |
             (vid_mac->mac.addr[4]<<8) | vid_mac->mac.addr[5]);
}

void vtss_mach_macl_set(vtss_vid_mac_t *vid_mac, u32 mach, u32 macl)
{
    vid_mac->vid = ((mach >> 16) & 0xfff);
    vid_mac->mac.addr[0] = ((mach >> 8) & 0xff);
    vid_mac->mac.addr[1] = ((mach >> 0) & 0xff);
    vid_mac->mac.addr[2] = ((macl >> 24) & 0xff);
    vid_mac->mac.addr[3] = ((macl >> 16) & 0xff);
    vid_mac->mac.addr[4] = ((macl >> 8) & 0xff);
    vid_mac->mac.addr[5] = ((macl >> 0) & 0xff);
}

static vtss_mac_entry_t *vtss_mac_entry_get(u32 mach, u32 macl, BOOL next)
{
    u32              first,last,mid;
    vtss_mac_entry_t *cur,*old;

    /* Check if table is empty */
    if (vtss_state->mac_ptr_count == 0)
        return NULL;

    /* Locate page using binary search */
    for (first = 0, last = vtss_state->mac_ptr_count-1; first != last; ) {
        mid = (first + last)/2;
        cur = vtss_state->mac_list_ptr[mid];
        if (cur->mach > mach || (cur->mach == mach && cur->macl >= macl)) {
            /* Entry greater or equal, search lower half */
            last = mid;
        } else {
            /* Entry is smaller, search upper half */
            first = mid + 1;
        }
    }

    cur = vtss_state->mac_list_ptr[first];
    /* Go back one page if entry greater */
    if (first != 0 && (cur->mach > mach || (cur->mach == mach && cur->macl > macl)))
        cur = vtss_state->mac_list_ptr[first-1];

    /* Look for greater entry using linear search */
    for (old = NULL; cur != NULL; old = cur, cur = cur->next)
        if (cur->mach > mach || (cur->mach == mach && cur->macl > macl))
            break;

    return (next ? cur : old);
}

/* Update MAC table page pointers */
static void vtss_mac_pages_update(void)
{
    u32              i,count;
    vtss_mac_entry_t *cur;

    for (i = 0, cur = vtss_state->mac_list_used; i < VTSS_MAC_PTR_SIZE && cur!=NULL ; i++) {
        vtss_state->mac_list_ptr[i] = cur;

        /* Move one page forward */
        for (count = 0; count != VTSS_MAC_PAGE_SIZE && cur != NULL ; cur = cur->next, count++)
            ;
    }
    vtss_state->mac_ptr_count = i;
}

/* Add MAC table entry */
static vtss_mac_entry_t *vtss_mac_entry_add(const vtss_mac_user_t user, 
                                            const vtss_vid_mac_t *vid_mac)
{
    u32              mach, macl;
    vtss_mac_entry_t *cur, *tmp;

    /* Calculate MACH and MACL */
    vtss_mach_macl_get(vid_mac, &mach, &macl);

    /* Look for previous or existing entry in used list */
    if ((tmp = vtss_mac_entry_get(mach, macl, 0)) != NULL &&
        tmp->mach == mach && tmp->macl == macl)
        return (tmp->user == user ? tmp : NULL);

    /* Allocate entry from free list */
    if ((cur = vtss_state->mac_list_free) == NULL) {
        VTSS_E("no free MAC entries");
        return NULL;
    }
    vtss_state->mac_list_free = cur->next;
    cur->mach = mach;
    cur->macl = macl;
    cur->user = user;

    if (tmp == NULL) {
        /* Insert first */
        cur->next = vtss_state->mac_list_used;
        vtss_state->mac_list_used = cur;
    } else {
        /* Insert after previous entry */
        cur->next = tmp->next;
        tmp->next = cur;
    }
    vtss_state->mac_table_count++;

    /* Update page pointers */
    vtss_mac_pages_update();

    return cur;
}

/* Delete MAC table entry */
static vtss_rc vtss_mac_entry_del(const vtss_mac_user_t user, const vtss_vid_mac_t *vid_mac)
{
    u32              mach, macl;
    vtss_mac_entry_t *cur, *old;

    /* Calculate MACH and MACL */
    vtss_mach_macl_get(vid_mac, &mach, &macl);

    /* Look for entry */
    for (old = NULL, cur = vtss_state->mac_list_used; cur != NULL; old = cur, cur = cur->next) {
        if (cur->mach == mach && cur->macl == macl) {
            if (cur->user != user) {
                /* Deleting entries added by other users is not allowed */
                return VTSS_RC_ERROR;
            }

            /* Remove from used list */
            if (old == NULL)
                vtss_state->mac_list_used = cur->next;
            else
                old->next = cur->next;

            /* Insert in free list */
            cur->next = vtss_state->mac_list_free;
            vtss_state->mac_list_free = cur;
            vtss_state->mac_table_count--;
            
            /* Update page pointers */
            vtss_mac_pages_update();
            break;
        }
    }
    
    return VTSS_RC_OK;
}

static vtss_rc vtss_mac_entry_update(vtss_mac_entry_t *cur, u32 pgid)
{
    vtss_mac_table_entry_t entry;

    vtss_mach_macl_set(&entry.vid_mac, cur->mach, cur->macl);
    entry.locked = 1;
    entry.aged = 0;
    entry.copy_to_cpu = cur->cpu_copy;
    return VTSS_FUNC(mac_table_add, &entry, pgid);
}

static BOOL vtss_ipmc_mac(const vtss_vid_mac_t *vid_mac)
{
    return (VTSS_MAC_IPV4_MC(vid_mac->mac.addr) ||
            VTSS_MAC_IPV6_MC(vid_mac->mac.addr));
}

/* Update IPv4 and IPv6 multicast entries on aggregation changes */
static vtss_rc vtss_mac_table_update(void)
{
    u32               pgid = VTSS_PGID_NONE;
    vtss_pgid_entry_t *pgid_entry = &vtss_state->pgid_table[pgid];
    vtss_mac_entry_t  *cur;
    vtss_port_no_t    port_no;
    vtss_vid_mac_t    vid_mac;      

    for (cur = vtss_state->mac_list_used; cur != NULL; cur = cur->next) {
        vtss_mach_macl_set(&vid_mac, cur->mach, cur->macl);
        if (!vtss_ipmc_mac(&vid_mac))
            continue;
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            pgid_entry->member[port_no] = VTSS_PORT_BF_GET(cur->member, port_no);
        vtss_pgid_members_get(pgid, pgid_entry->member);
        vtss_mac_entry_update(cur, pgid);
    }

    return VTSS_RC_OK;
}

/* Write PGID member to chip */
static vtss_rc vtss_pgid_table_write(u32 pgid)
{
    BOOL member[VTSS_PORT_ARRAY_SIZE];

    VTSS_N("pgid: %u", pgid);

    /* Ignore unused entries */
    if (vtss_state->pgid_table[pgid].references == 0)
        return VTSS_RC_OK;

    /* Get port members */
    vtss_pgid_members_get(pgid, member);

    /* Update PGID table */
    return VTSS_FUNC(pgid_table_write, pgid, member);
}

/* Allocate PGID */
static vtss_rc vtss_pgid_alloc(u32 *new, const BOOL member[VTSS_PORT_ARRAY_SIZE],
                               BOOL cpu_copy, vtss_packet_rx_queue_t cpu_queue)
{
    u32               pgid, pgid_free = *new;
    BOOL              pgid_found = 0;
    vtss_pgid_entry_t *pgid_entry;
    vtss_port_no_t    port_no;

    VTSS_D("enter");

    /* Search for matching or unused entry in PGID table */
    for (pgid = 0; pgid < vtss_state->pgid_count; pgid++)  {
        pgid_entry = &vtss_state->pgid_table[pgid];
        if (pgid_entry->resv) /* Skip reserved entries */
            continue;

        if (pgid_entry->references == 0) {
            /* Check if the first unused entry is found or the pgid can be reused */
            if (pgid_free == VTSS_PGID_NONE || pgid == *new) {
                pgid_found = 1;
                pgid_free = pgid;
            }
        } else {
            /* Check if an existing entry matches */
            for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
                if (member[port_no] != pgid_entry->member[port_no])
                    break;
            if (port_no == vtss_state->port_count && 
                pgid_entry->cpu_copy == cpu_copy && pgid_entry->cpu_queue == cpu_queue) {
                VTSS_D("reusing pgid: %u", pgid);
                *new = pgid;
                pgid_entry->references++;
                return VTSS_RC_OK;
            }
        }
    }

    /* No pgid found */
    if (pgid_found == 0) {
        VTSS_E("no more pgids");
        return VTSS_RC_ERROR;
    }

    VTSS_D("using pgid: %u", pgid_free);
    *new = pgid_free;
    pgid_entry = &vtss_state->pgid_table[pgid_free];
    pgid_entry->references = 1;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        pgid_entry->member[port_no] = member[port_no];
    pgid_entry->cpu_copy = cpu_copy;
    pgid_entry->cpu_queue = cpu_queue;
    return vtss_pgid_table_write(pgid_free);
}

/* Free PGID */
static vtss_rc vtss_pgid_free(u32 pgid)
{
    vtss_pgid_entry_t *pgid_entry;;

    VTSS_D("pgid: %u", pgid);

    /* Ignore IPv4/IPv6 MC free */
    if (pgid == VTSS_PGID_NONE)
        return VTSS_RC_OK;

    if (pgid > vtss_state->pgid_count) {
        VTSS_E("illegal pgid: %u", pgid);
        return VTSS_RC_ERROR;
    }

    /* Do not free reserved PGIDs */
    pgid_entry = &vtss_state->pgid_table[pgid];
    if (pgid_entry->resv)
        return VTSS_RC_OK;

    /* Check if pgid already free */
    if (pgid_entry->references == 0) {
        VTSS_E("pgid: %u already free", pgid);
        return VTSS_RC_ERROR;
    }

    pgid_entry->references--;
    return VTSS_RC_OK;
}

/* Determine logical port */
static vtss_port_no_t vtss_aggr_port(vtss_port_no_t port)
{
    vtss_port_no_t port_no;
    vtss_aggr_no_t aggr_no;
    vtss_port_no_t lport_no = port; /* Use port itself by default */

    if ((aggr_no = vtss_state->port_aggr_no[port]) != VTSS_AGGR_NO_NONE) {
        /* Aggregated, use first operational port in aggregation */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            if (vtss_state->port_state[port_no] &&
                vtss_state->port_aggr_no[port_no] == aggr_no) {
                lport_no = port_no;
                break;
            }
    }
#if defined(VTSS_FEATURE_AGGR_GLAG)
    if ((aggr_no = vtss_state->port_glag_no[port]) != VTSS_AGGR_NO_NONE) {
        /* Aggregated, use first operational port in aggregation */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            if (vtss_state->port_state[port_no] &&
                vtss_state->port_glag_no[port_no] == aggr_no) {
                lport_no = port_no;
                break;
            }
    }
#endif
    return lport_no;
}

/* Update source, destination and aggregation masks */
static vtss_rc vtss_update_masks(BOOL src_update, BOOL dest_update, BOOL aggr_update)
{
    vtss_rc             rc;
    vtss_port_no_t      i_port, e_port, port_p;
    vtss_aggr_no_t      aggr_no;
    BOOL                member[VTSS_PORT_ARRAY_SIZE], learn[VTSS_PORT_ARRAY_SIZE];
    BOOL                rx_forward[VTSS_PORT_ARRAY_SIZE], tx_forward[VTSS_PORT_ARRAY_SIZE];
    u32                 pgid;
    u32                 port_count = vtss_state->port_count;
    u32                 ac, aggr_count[VTSS_PORT_ARRAY_SIZE];
    u32                 aggr_index[VTSS_PORT_ARRAY_SIZE], n;
#if defined(VTSS_FEATURE_AGGR_GLAG)
    vtss_glag_no_t      glag_no;
    vtss_port_no_t      port_glag_index[VTSS_PORT_ARRAY_SIZE];
    u32                 glag_port_count[VTSS_GLAGS];
#endif /* VTSS_FEATURE_AGGR_GLAG */
    vtss_port_eps_t     *protect;
    vtss_chip_no_t      chip_no = vtss_state->chip_no;

    VTSS_N("enter");

    if (vtss_state->warm_start_cur) {
        VTSS_D("warm start, returning");
        return VTSS_RC_OK;
    }

    /* Determine learning, Rx and Tx forwarding state per port */
    for (i_port = VTSS_PORT_NO_START; i_port < port_count; i_port++) {
        protect = &vtss_state->port_protect[i_port];

        /* Learning */
        learn[i_port] = (vtss_state->port_state[i_port] &&
                         vtss_state->stp_state[i_port] != VTSS_STP_STATE_DISCARDING &&
                         VTSS_PORT_RX_FORWARDING(vtss_state->port_forward[i_port]) &&
                         vtss_state->auth_state[i_port] == VTSS_AUTH_STATE_BOTH &&
                         (protect->conf.port_no == VTSS_PORT_NO_NONE ||
                          protect->conf.type == VTSS_EPS_PORT_1_FOR_1 ||
                          protect->selector == VTSS_EPS_SELECTOR_WORKING));

        /* Rx forwarding */
        rx_forward[i_port] = (learn[i_port] &&
                              vtss_state->stp_state[i_port] == VTSS_STP_STATE_FORWARDING);

        /* Tx forwarding */
        tx_forward[i_port] = (vtss_state->port_state[i_port] &&
                              vtss_state->stp_state[i_port] == VTSS_STP_STATE_FORWARDING &&
                              vtss_state->auth_state[i_port] != VTSS_AUTH_STATE_NONE &&
                              !vtss_state->port_conf[i_port].power_down);
    }

    /* Determine state for protection ports */
    for (i_port = VTSS_PORT_NO_START; i_port < port_count; i_port++) {
        protect = &vtss_state->port_protect[i_port];
        port_p = protect->conf.port_no;

        /* If 1+1 working port is active, discard on Rx */
        if (port_p != VTSS_PORT_NO_NONE &&
            protect->conf.type == VTSS_EPS_PORT_1_PLUS_1 &&
            protect->selector == VTSS_EPS_SELECTOR_WORKING) {
            learn[port_p] = 0;
            rx_forward[port_p] = 0;
        }
    }

    /* Update learn mask */
    if (src_update && (rc = VTSS_FUNC(learn_state_set, learn)) != VTSS_RC_OK)
        return rc;

    /* Update source masks */
    for (i_port = VTSS_PORT_NO_START; src_update && i_port < port_count; i_port++) {
        /* Exclude all ports by default */
        memset(member, 0, sizeof(member));

        /* Store Rx forward information */
        vtss_state->rx_forward[i_port] = rx_forward[i_port];

        /* Check if ingress forwarding is allowed */
        if (rx_forward[i_port]) {
#if defined(VTSS_FEATURE_PVLAN)
            vtss_pvlan_no_t     pvlan_no;

            /* Include members of the same PVLAN */
            for (pvlan_no = VTSS_PVLAN_NO_START; pvlan_no < VTSS_PVLAN_NO_END; pvlan_no++) {
                if (vtss_state->pvlan_table[pvlan_no].member[i_port] == 0)
                    continue;
                /* The ingress port is a member of this PVLAN */
                for (e_port = VTSS_PORT_NO_START; e_port < port_count; e_port++)
                    if (vtss_state->pvlan_table[pvlan_no].member[e_port])
                        member[e_port] = 1; /* Egress port also member */
            }
#else
            /* Include all members if PVLANs are not supported */
            for (e_port = VTSS_PORT_NO_START; e_port < port_count; e_port++)
                member[e_port] = 1; /* Egress port also member */
#endif /* VTSS_FEATURE_PVLAN */

            /* Exclude protection port if it exists */
            if ((port_p = vtss_state->port_protect[i_port].conf.port_no) != VTSS_PORT_NO_NONE)
                member[port_p] = 0;

            member[i_port] = 0;
            aggr_no = vtss_state->port_aggr_no[i_port];
            for (e_port = VTSS_PORT_NO_START; e_port < port_count; e_port++) {
                /* Exclude members of the same aggregation */
                if (aggr_no != VTSS_AGGR_NO_NONE &&
                    vtss_state->port_aggr_no[e_port] == aggr_no)
                    member[e_port] = 0;

                /* Exclude working port if it exists */
                if (vtss_state->port_protect[e_port].conf.port_no == i_port)
                    member[e_port] = 0;
                VTSS_N("i_port: %u %sforwarding to e_port %u",
                       i_port, member[e_port] ? "" : "NOT ", e_port);

                /* Exclude ports, which are not egress forwarding */
                if (!VTSS_PORT_TX_FORWARDING(vtss_state->port_forward[e_port]))
                    member[e_port] = 0;
                
#if defined(VTSS_FEATURE_PVLAN)
                /* Exclude ports not allowed by assymmetric PVLANs */
                if (!vtss_state->apvlan_table[i_port][e_port])
                    member[e_port] = 0;
#endif /* VTSS_FEATURE_PVLAN */
            }
        }
        if ((rc = VTSS_FUNC(src_table_write, i_port, member)) != VTSS_RC_OK)
            return rc;
    } /* src_update */

    /* Update PGID table (destination masks) */
#if defined(VTSS_FEATURE_AGGR_GLAG)
    /* Setup GLAG masks */
    if (aggr_update)
        dest_update = 1;
    for (glag_no = VTSS_GLAG_NO_START; dest_update && glag_no < VTSS_GLAG_NO_END; glag_no++) {
        vtss_port_no_t port_no;
        u32            glag_idx, gport, gcount=0; 

        glag_idx = (glag_no - VTSS_GLAG_NO_START);
#if defined(VTSS_FEATURE_VSTAX_V1)
        if (vtss_state->arch == VTSS_ARCH_L28) {
            u32            pgid_dest, pgid_src, pgid_aggr_a, pgid_aggr_b, index;
            pgid_dest = (vtss_state->pgid_glag_dest + glag_idx);
            pgid_src = (vtss_state->pgid_glag_src + glag_idx);
            pgid_aggr_a = (vtss_state->pgid_glag_aggr_a + glag_idx);
            pgid_aggr_b = (vtss_state->pgid_glag_aggr_b + glag_idx);
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                vtss_state->pgid_table[pgid_dest].member[port_no] = 0;
                vtss_state->pgid_table[pgid_src].member[port_no] = 1;
                vtss_state->pgid_table[pgid_aggr_a].member[port_no] = 0;
                vtss_state->pgid_table[pgid_aggr_b].member[port_no] = 0;
            }

            /* Setup GLAG source and destination masks */
            for (gport = VTSS_GLAG_PORT_START; gport < VTSS_GLAG_PORT_END; gport++) {
                if ((port_no = vtss_state->glag_members[glag_idx][gport]) == VTSS_PORT_NO_NONE)
                    break;
                
                /* Include local ports and stack ports in destination mask */
                vtss_state->pgid_table[pgid_dest].member[port_no] = 1;
                
                /* Include locally aggregated stack ports in destination mask */
                if ((aggr_no = vtss_state->port_aggr_no[port_no]) != VTSS_AGGR_NO_NONE &&
                    vtss_state->vstax_port_enabled[port_no]) {
                    for (i_port = VTSS_PORT_NO_START; i_port < port_count; i_port++)
                        if (vtss_state->port_aggr_no[i_port] == aggr_no)
                            vtss_state->pgid_table[pgid_dest].member[i_port] = 1;
                }
                
                /* Exclude local ports from source mask */
                if (!vtss_state->vstax_port_enabled[port_no]) 
                    vtss_state->pgid_table[pgid_src].member[port_no] = 0;
                
                /* Count number of forwarding ports and assign index */
                if (tx_forward[port_no]) {
                    VTSS_D("glag_no: %u, index: %u, port_no: %u", glag_no, gcount, port_no);
                    port_glag_index[port_no] = gcount;
                    gcount++;
                }
            }
            /* Setup GLAG aggregation masks */
            for (ac = 0; gcount && ac < vtss_state->ac_count; ac++) {
                index = 0;
                for (gport = VTSS_GLAG_PORT_START; gport < VTSS_GLAG_PORT_END; gport++) {
                    if ((port_no = vtss_state->glag_members[glag_idx][gport]) == VTSS_PORT_NO_NONE)
                        break;
                    
                    if (tx_forward[port_no]) {
                        /* Enable stack port if AC owner */
                        if (vtss_state->vstax_port_enabled[port_no] &&
                            ((index + ac) % gcount) == 0) {
                            pgid = (((1 << vtss_state->port_map[port_no].chip_port) &
                                     vtss_state->vstax_info.chip_info[0].mask_a) ? pgid_aggr_a : pgid_aggr_b);
                            vtss_state->pgid_table[pgid].member[ac] = 1;
                            
                            /* If stack ports are aggregated, enable AC in both masks */
                            if (vtss_state->port_aggr_no[port_no] != VTSS_AGGR_NO_NONE) {
                                pgid = (pgid == pgid_aggr_a ? pgid_aggr_b : pgid_aggr_a);
                                vtss_state->pgid_table[pgid].member[ac] = 1;
                            }
                        }
                        index++;
                    }
                }
            }
        }
#endif /* VTSS_FEATURE_VSTAX_V1  */    
#if defined(VTSS_FEATURE_VSTAX_V2)
        if (vtss_state->arch == VTSS_ARCH_JR1) {
            BOOL               glag_src[VTSS_PORTS];
            vtss_glag_conf_t   *glag_conf;
            u32                glag_members, glag_indx;

            /* By default, all ports are included in the GLAG source mask */
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                glag_src[port_no] = 1;
            }

            /* Determine number of Glag members  */
            glag_members = 0;
            for (gport = VTSS_GLAG_PORT_START; gport < VTSS_GLAG_PORT_END; gport++, glag_members++) {
                if (vtss_state->glag_conf[glag_idx][gport].entry.upspn == VTSS_UPSPN_NONE) 
                    break;
            }                

            /* Calculate number of active ports and GLAG source mask */
            gcount = 0;
            for (gport = VTSS_GLAG_PORT_START; gport < VTSS_GLAG_PORT_END; gport++) {
                glag_conf = &vtss_state->glag_conf[glag_idx][gport];
                if (glag_conf->entry.upspn == VTSS_UPSPN_NONE) {
                    /* End of list */
                    break;
                }
                if (glag_conf->port_no != VTSS_PORT_NO_NONE) {
                    /* Local port */
                    if (!tx_forward[glag_conf->port_no]) {
                        /* Skip local ports not in forwarding state */
                        continue;
                    }

                    /* Exclude from GLAG source mask */
                    glag_src[glag_conf->port_no] = 0;
                    
                    /* Count number of forwarding ports and assign index */
                    VTSS_D("glag_no: %u, index: %u, port_no: %u", 
                           glag_no, gcount, glag_conf->port_no);
                    port_glag_index[glag_conf->port_no] = gcount;
                }
                
                /* Find the glag index that matches the aggregation mask index */
                glag_indx = (glag_members - gcount) % glag_members;

                /* Setup GLAG port entry */ 
                /* Note that the GLAG PGID table and the aggregation mask are AND'ed together to find a forwarding mask */
                /* and must therefore match for unicast entries. Therefore the 'glag_indx' is used in the below function */
                VTSS_FUNC_RC(glag_port_write, glag_no, glag_indx, &glag_conf->entry);
                gcount++;
            }
            
            VTSS_FUNC_RC(glag_src_table_write, glag_no, gcount, glag_src);
        }
#endif /* VTSS_FEATURE_VSTAX_V2 */        
        glag_port_count[glag_idx] = gcount;        
    } /* GLAG loop */
#endif /* VTSS_FEATURE_AGGR_GLAG */
    if (dest_update) {
        for (pgid = 0; pgid < vtss_state->pgid_count; pgid++) {
            VTSS_RC(vtss_pgid_table_write(pgid));
        }
        /* Update destination masks encoded in MAC address table */
        vtss_mac_table_update();
    } /* dest_update */

    /* Update aggregation masks */
    if (aggr_update) {

        /* Count number of operational ports and index of each port */
        for (i_port = VTSS_PORT_NO_START; i_port < port_count; i_port++) {
            aggr_count[i_port] = 0;
            aggr_index[i_port] = 0;

            /* If port is not forwarding, continue */
            if (!tx_forward[i_port])
                continue;

            aggr_no = vtss_state->port_aggr_no[i_port];
            VTSS_D("port_no: %u, aggr_no: %u is forwarding", i_port, aggr_no);

            if (aggr_no == VTSS_AGGR_NO_NONE) {
                /* Not aggregated */
                aggr_count[i_port]++;
                continue;
            }
            for (e_port = VTSS_PORT_NO_START; e_port < port_count; e_port++) {
                if (tx_forward[e_port] && vtss_state->port_aggr_no[e_port] == aggr_no) {
                    /* Port is forwarding and member of the same aggregation */
                    aggr_count[i_port]++;
                    if (e_port < i_port)
                        aggr_index[i_port]++;
                }
            }
        }

        for (ac = 0; ac < vtss_state->ac_count; ac++) {
            /* Include one forwarding port from each aggregation */
            for (i_port = VTSS_PORT_NO_START; i_port < port_count; i_port++) {
                n = (aggr_index[i_port] + ac);
                member[i_port] = (aggr_count[i_port] != 0 && (n % aggr_count[i_port]) == 0);

#if defined(VTSS_FEATURE_AGGR_GLAG)
                /* Exclude GLAG member port if not AC owner */
                glag_no = vtss_state->port_glag_no[i_port];
                if (member[i_port] &&
                    glag_no != VTSS_GLAG_NO_NONE &&
                    ((port_glag_index[i_port] + ac) %
                     glag_port_count[glag_no - VTSS_GLAG_NO_START]) != 0)
                    member[i_port] = 0;
#endif /* VTSS_FEATURE_AGGR_GLAG */
                if (ac == 0)
                    /* Store Tx forward information for the first aggregation code */
                    vtss_state->tx_forward[i_port] = member[i_port];
            }
            /* Write to aggregation table */
            if ((rc = VTSS_FUNC(aggr_table_write, ac, member)) != VTSS_RC_OK)
                return rc;
        }

        /* Update port map table on aggregation changes */
        for (i_port = VTSS_PORT_NO_START; i_port < port_count; i_port++) {
            if ((rc = VTSS_FUNC(pmap_table_write, i_port, vtss_aggr_port(i_port))) != VTSS_RC_OK)
                return rc;
        }
    } /* aggr_update */

    /* Restore chip number in case we were called from a port specific function,
       which had selected the chip number based on the port map. */
    VTSS_SELECT_CHIP(chip_no);

    return VTSS_RC_OK;
}

vtss_rc vtss_mac_add(vtss_mac_user_t user, const vtss_mac_table_entry_t *const entry)
{
    u32                    pgid, count = 0, port_count = vtss_state->port_count;
    vtss_mac_table_entry_t old_entry;
    vtss_mac_entry_t       *mac_entry;
    vtss_port_no_t         port_no;
    BOOL                   ipmc, member[VTSS_PORT_ARRAY_SIZE];
    vtss_vid_mac_t         vid_mac;
    vtss_pgid_entry_t      *pgid_entry;
    BOOL                   cpu_copy = 0;
    vtss_packet_rx_queue_t cpu_queue = 0;

    vid_mac = entry->vid_mac;
    VTSS_D("vid: %d, mac: %02x-%02x-%02x-%02x-%02x-%02x",
           vid_mac.vid,
           vid_mac.mac.addr[0], vid_mac.mac.addr[1], vid_mac.mac.addr[2],
           vid_mac.mac.addr[3], vid_mac.mac.addr[4], vid_mac.mac.addr[5]);

    ipmc = vtss_ipmc_mac(&vid_mac);

#if defined(VTSS_FEATURE_MAC_CPU_QUEUE)
    if (entry->copy_to_cpu) {
        cpu_copy = 1;
        cpu_queue = entry->cpu_queue;
    }
#endif /* VTSS_FEATURE_MAC_CPU_QUEUE */

    if (entry->locked) {
        /* Add all locked entries to state block */
        if ((mac_entry = vtss_mac_entry_add(user, &entry->vid_mac)) == NULL) {
            return VTSS_RC_ERROR;
        }
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++)
            VTSS_PORT_BF_SET(mac_entry->member, port_no, entry->destination[port_no]);
        mac_entry->cpu_copy = entry->copy_to_cpu;
    } else {
        /* Unlocked entry, must be non-IPMC with zero/one/all destination port and no CPU copy */
        if (ipmc) {
            VTSS_E("IP multicast entry must be locked");
            return VTSS_RC_ERROR;
        }

        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (entry->destination[port_no])
                count++;
        }
        if (cpu_copy || (count != 0 && count != 1 && count != port_count)) {
            VTSS_E("unlocked can only have zero, one or all destination ports and no cpu_copy");
            return VTSS_RC_ERROR;
        }

        /* Delete any previous locked entry from state block */
        VTSS_RC(vtss_mac_entry_del(user, &vid_mac));
    }

    /* No further processing in warm start mode */
    if (vtss_state->warm_start_cur)
        return VTSS_RC_OK;

    if (ipmc) {
        /* IPv4/IPv6 multicast address, use pseudo PGID */
        pgid = VTSS_PGID_NONE;
        pgid_entry = &vtss_state->pgid_table[pgid];
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++)
            pgid_entry->member[port_no] = VTSS_BOOL(entry->destination[port_no]);
        vtss_pgid_members_get(pgid, pgid_entry->member);
    } else {
        /* Free old PGID if the address exists */
        old_entry.vid_mac = vid_mac;
        if (VTSS_FUNC(mac_table_get, &old_entry, &pgid) == VTSS_RC_OK && old_entry.locked) {
            VTSS_RC(vtss_pgid_free(pgid));
        }

        /* Allocate new PGID */
        pgid = VTSS_PGID_NONE;
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            member[port_no] = VTSS_BOOL(entry->destination[port_no]);
            if (!entry->locked && member[port_no])
                pgid = port_no;
        }
        if (entry->locked) {
            VTSS_RC(vtss_pgid_alloc(&pgid, member, cpu_copy, cpu_queue));
        } else if (count == 0) {
            pgid = vtss_state->pgid_drop;
        } else if (count == port_count) {
            pgid = vtss_state->pgid_flood;
        }
    }

    vtss_state->mac_status.learned = 1;

    return VTSS_FUNC(mac_table_add, entry, pgid);
}

vtss_rc vtss_mac_table_add(const vtss_inst_t             inst,
                           const vtss_mac_table_entry_t  *const entry)
{
    vtss_rc rc;

    VTSS_D("vid: %d, mac: %02x-%02x-%02x-%02x-%02x-%02x, copy_to_cpu: %u locked: %u",
           entry->vid_mac.vid,
           entry->vid_mac.mac.addr[0], entry->vid_mac.mac.addr[1], entry->vid_mac.mac.addr[2],
           entry->vid_mac.mac.addr[3], entry->vid_mac.mac.addr[4], entry->vid_mac.mac.addr[5],
           entry->copy_to_cpu,
           entry->locked);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_add(VTSS_MAC_USER_NONE, entry);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mac_del(vtss_mac_user_t user, const vtss_vid_mac_t *const vid_mac)
{
    vtss_rc                rc;
    vtss_mac_table_entry_t entry;
    u32                    pgid;

    VTSS_D("vid: %d, mac: %02x-%02x-%02x-%02x-%02x-%02x",
           vid_mac->vid,
           vid_mac->mac.addr[0], vid_mac->mac.addr[1], vid_mac->mac.addr[2],
           vid_mac->mac.addr[3], vid_mac->mac.addr[4], vid_mac->mac.addr[5]);

    /* Delete entry from state block */
    VTSS_RC(vtss_mac_entry_del(user, vid_mac));

    /* No further processing in warm start mode */
    if (vtss_state->warm_start_cur)
        return VTSS_RC_OK;
    
    /* Free PGID and delete if the entry exists */
    entry.vid_mac = *vid_mac;
    if ((rc = VTSS_FUNC(mac_table_get, &entry, &pgid)) == VTSS_RC_OK) {
        if (entry.locked)
            vtss_pgid_free(pgid);
        vtss_state->mac_status.aged = 1;
        rc = VTSS_FUNC(mac_table_del, vid_mac);
    }
    return rc;
}

vtss_rc vtss_mac_table_del(const vtss_inst_t     inst,
                           const vtss_vid_mac_t  *const vid_mac)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_del(VTSS_MAC_USER_NONE, vid_mac);
    VTSS_EXIT();
    return rc;
}

static void vtss_mac_pgid_get(vtss_mac_table_entry_t *const entry, u32 pgid)
{
    vtss_pgid_members_get(pgid, entry->destination);
    
#if defined(VTSS_FEATURE_MAC_CPU_QUEUE)
    /* The CPU copy flag may be set in the MAC address table or PGID table */
    {
        vtss_pgid_entry_t *pgid_entry = &vtss_state->pgid_table[pgid];
    
        if (pgid_entry->cpu_copy) {
            entry->copy_to_cpu = 1;
            entry->cpu_queue = pgid_entry->cpu_queue;
        }
    }
#endif /* VTSS_FEATURE_MAC_CPU_QUEUE */
}

vtss_rc vtss_mac_get(const vtss_vid_mac_t   *const vid_mac,
                     vtss_mac_table_entry_t *const entry)
{
    vtss_rc rc;
    u32     pgid;

    VTSS_D("vid: %d, mac: %02x-%02x-%02x-%02x-%02x-%02x",
           vid_mac->vid,
           vid_mac->mac.addr[0], vid_mac->mac.addr[1], vid_mac->mac.addr[2],
           vid_mac->mac.addr[3], vid_mac->mac.addr[4], vid_mac->mac.addr[5]);

    entry->vid_mac = *vid_mac;
    if ((rc = VTSS_FUNC(mac_table_get, entry, &pgid)) != VTSS_RC_OK)
        return rc;

    vtss_mac_pgid_get(entry, pgid);

    return VTSS_RC_OK;
}

vtss_rc vtss_mac_table_get(const vtss_inst_t       inst,
                           const vtss_vid_mac_t    *const vid_mac,
                           vtss_mac_table_entry_t  *const entry)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_get(vid_mac, entry);
    VTSS_EXIT();
    return rc;
}

static vtss_rc vtss_mac_get_next(const vtss_vid_mac_t   *const vid_mac,
                                 vtss_mac_table_entry_t *const entry)
{
    vtss_rc                rc = VTSS_RC_ERROR;
    u32                    pgid;
    vtss_mac_table_entry_t mac_entry;
    u32                    mach, macl;
    vtss_mac_entry_t       *cur, *cmp;
    vtss_vid_mac_t         vid_mac_next;

    VTSS_D("vid: %d, mac: %02x-%02x-%02x-%02x-%02x-%02x",
           vid_mac->vid,
           vid_mac->mac.addr[0], vid_mac->mac.addr[1], vid_mac->mac.addr[2],
           vid_mac->mac.addr[3], vid_mac->mac.addr[4], vid_mac->mac.addr[5]);

    mac_entry.vid_mac = *vid_mac;
    vtss_mach_macl_get(vid_mac, &mach, &macl);
    for (cur = vtss_mac_entry_get(mach, macl, 1); cur != NULL; cur = cur->next) {
        if (cur->user != VTSS_MAC_USER_NONE) {
            continue;
        }

        /* Lookup in chip */
        vtss_mach_macl_set(&vid_mac_next, cur->mach, cur->macl);
        if ((rc = vtss_mac_get(&vid_mac_next, entry)) == VTSS_RC_OK) {
            VTSS_D("found sw entry 0x%08x%08x", cur->mach, cur->macl);
            break;
        }
    }

    while (VTSS_FUNC(mac_table_get_next, &mac_entry, &pgid) == VTSS_RC_OK) {
        vtss_mac_pgid_get(&mac_entry, pgid);
        vtss_mach_macl_get(&mac_entry.vid_mac, &mach, &macl);
        VTSS_D("found chip entry 0x%08x%08x", mach, macl);

        if ((cmp = vtss_mac_entry_get(mach, macl, 0)) != NULL &&
            cmp->mach == mach && cmp->macl == macl && cmp->user != VTSS_MAC_USER_NONE) {
            continue;
        }

        if (rc != VTSS_RC_OK || (mach < cur->mach || (mach == cur->mach && macl < cur->macl)))
            *entry = mac_entry;

        rc = VTSS_RC_OK;
        break;
    }

    return rc;
}


vtss_rc vtss_mac_table_get_next(const vtss_inst_t       inst,
                                const vtss_vid_mac_t    *const vid_mac,
                                vtss_mac_table_entry_t  *const entry)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_get_next(vid_mac, entry);
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_FEATURE_MAC_AGE_AUTO)
vtss_rc vtss_mac_table_age_time_get(const vtss_inst_t          inst,
                                    vtss_mac_table_age_time_t  *const age_time)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *age_time = vtss_state->mac_age_time;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mac_table_age_time_set(const vtss_inst_t                inst,
                                    const vtss_mac_table_age_time_t  age_time)
{
    vtss_rc rc;

    VTSS_D("age_time: %u", age_time);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->mac_age_time = age_time;
        rc = VTSS_FUNC(mac_table_age_time_set);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_MAC_AGE_AUTO */

static vtss_rc vtss_mac_age(BOOL             pgid_age,
                            u32              pgid,
                            BOOL             vid_age,
                            const vtss_vid_t vid,
                            u32              count)
{
    vtss_rc        rc;
    u32            i;
    vtss_port_no_t port_no = VTSS_PORT_NO_NONE;

    if (pgid_age && pgid < vtss_state->port_count)
        port_no = vtss_aggr_port(pgid);

    for (i = 0; i < count; i++) {
        if ((rc = VTSS_FUNC_COLD(mac_table_age, pgid_age, pgid, vid_age, vid)) != VTSS_RC_OK)
            return rc;

        /* Age logical port */
        if (port_no != VTSS_PORT_NO_NONE &&
            (rc = VTSS_FUNC_COLD(mac_table_age, pgid_age, port_no, vid_age, vid)) != VTSS_RC_OK)
            return rc;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_mac_table_age(const vtss_inst_t  inst)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_age(0, 0, 0, 0, 1);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mac_table_vlan_age(const vtss_inst_t  inst,
                                const vtss_vid_t   vid)
{
    vtss_rc rc;

    VTSS_D("vid: %u", vid);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_age(0, 0, 1, vid, 1);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mac_table_flush(const vtss_inst_t inst)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_age(0, 0, 0, 0, 2);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mac_table_port_flush(const vtss_inst_t     inst,
                                  const vtss_port_no_t  port_no)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_age(1, port_no, 0, 0, 2);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mac_table_vlan_flush(const vtss_inst_t  inst,
                                  const vtss_vid_t   vid)
{
    vtss_rc rc;

    VTSS_D("vid: %u", vid);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_age(0, 0, 1, vid, 2);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mac_table_vlan_port_flush(const vtss_inst_t     inst,
                                       const vtss_port_no_t  port_no,
                                       const vtss_vid_t      vid)
{
    vtss_rc rc;

    VTSS_D("port_no: %u, vid: %u", port_no, vid);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_age(1, port_no, 1, vid, 2);
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_FEATURE_VSTAX_V2)
vtss_rc vtss_mac_table_upsid_upspn_flush(const vtss_inst_t        inst,
                                         const vtss_vstax_upsid_t upsid,
                                         const vtss_vstax_upspn_t upspn)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        VTSS_VSTAX_UPSID_LEGAL(upsid)) {
        rc = VTSS_FUNC(mac_table_upsid_upspn_flush, upsid, upspn);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mac_table_upsid_flush(const vtss_inst_t        inst,
                                   const vtss_vstax_upsid_t upsid)
{
    return vtss_mac_table_upsid_upspn_flush(inst, upsid, VTSS_UPSPN_NONE);
}
#endif /* VTSS_FEATURE_VSTAX_V2 */

static vtss_rc vtss_vid_check(const vtss_vid_t vid)
{
    if (vid >= VTSS_VIDS) {
        VTSS_E("illegal vid: %u", vid);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_AGGR_GLAG)
static vtss_rc vtss_glag_check(const vtss_glag_no_t glag_no, u32 *pgid)
{
    if (glag_no < VTSS_GLAG_NO_START || glag_no >= VTSS_GLAG_NO_END) {
        VTSS_E("illegal glag_no: %d", glag_no);
        return VTSS_RC_ERROR;
    }
    *pgid = (vtss_state->pgid_glag_dest + glag_no - VTSS_GLAG_NO_START);
    return VTSS_RC_OK;
}

static vtss_rc vtss_mac_glag_add(const vtss_mac_table_entry_t  *const entry,
                                 const vtss_glag_no_t          glag_no)
{
    u32 pgid;

    VTSS_D("glag_no: %d", glag_no);

    VTSS_RC(vtss_glag_check(glag_no, &pgid));

    /* Delete entry if it exists (free IPv4/IPv6 MC and PGID) */
    vtss_mac_del(VTSS_MAC_USER_NONE, &entry->vid_mac);
    vtss_state->mac_status.learned = 1;

    return VTSS_FUNC(mac_table_add, entry, pgid);
}

vtss_rc vtss_mac_table_glag_add(const vtss_inst_t             inst,
                                const vtss_mac_table_entry_t  *const entry,
                                const vtss_glag_no_t          glag_no)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_glag_add(entry, glag_no);
    VTSS_EXIT();
    return rc;
}

static vtss_rc vtss_mac_glag_flush(const vtss_glag_no_t  glag_no)
{
    u32 pgid;

    VTSS_D("glag_no: %u", glag_no);
    VTSS_RC(vtss_glag_check(glag_no, &pgid));
    return vtss_mac_age(1, pgid, 0, 0, 2);
}

vtss_rc vtss_mac_table_glag_flush(const vtss_inst_t     inst,
                                  const vtss_glag_no_t  glag_no)
{
    vtss_rc rc;

    VTSS_D("glag_no: %u", glag_no);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_glag_flush(glag_no);
    VTSS_EXIT();
    return rc;
}

static vtss_rc vtss_mac_vlan_glag_flush(const vtss_glag_no_t  glag_no,
                                        const vtss_vid_t      vid)
{
    u32 pgid;

    VTSS_D("glag_no: %u", glag_no);
    VTSS_RC(vtss_glag_check(glag_no, &pgid));
    VTSS_RC(vtss_vid_check(vid));
    return vtss_mac_age(1, pgid, 1, vid, 2);
}

vtss_rc vtss_mac_table_vlan_glag_flush(const vtss_inst_t     inst,
                                       const vtss_glag_no_t  glag_no,
                                       const vtss_vid_t      vid)
{
    vtss_rc rc;

    VTSS_D("glag_no: %u vid: %d", glag_no, vid);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_mac_vlan_glag_flush(glag_no, vid);
    VTSS_EXIT();
    return rc;
}

#endif /* VTSS_FEATURE_AGGR_GLAG */

vtss_rc vtss_mac_table_status_get(const vtss_inst_t        inst,
                                  vtss_mac_table_status_t  *const status)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = VTSS_FUNC(mac_table_status_get, status)) == VTSS_RC_OK) {
        /* Read and clear events */
        if (vtss_state->mac_status.learned)
            status->learned = 1;
        if (vtss_state->mac_status.aged)
            status->aged = 1;
        memset(&vtss_state->mac_status, 0, sizeof(*status));
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_learn_port_mode_get(const vtss_inst_t     inst,
                                 const vtss_port_no_t  port_no,
                                 vtss_learn_mode_t     *const mode)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *mode = vtss_state->learn_mode[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_learn_port_mode_set(const vtss_inst_t        inst,
                                 const vtss_port_no_t     port_no,
                                 const vtss_learn_mode_t  *const mode)
{
    vtss_rc rc;

    VTSS_D("port_no: %u, auto: %u, cpu: %u, discard: %u",
           port_no, mode->automatic, mode->cpu, mode->discard);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->learn_mode[port_no] = *mode;
        rc = VTSS_FUNC_COLD(learn_port_mode_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

/* - Operational state --------------------------------------------- */

vtss_rc vtss_port_state_get(const vtss_inst_t     inst,
                            const vtss_port_no_t  port_no,
                            BOOL                  *const state)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *state = vtss_state->port_state[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_state_set(const vtss_inst_t     inst,
                            const vtss_port_no_t  port_no,
                            BOOL                  state)
{
    vtss_rc rc;

    VTSS_I("port_no: %u, state: %s", port_no, state ? "up" : "down");
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->port_state[port_no] = state;
        rc = vtss_update_masks(1, 0, 1);
    }
    VTSS_EXIT();
    return rc;
}

/* - Spanning Tree ------------------------------------------------- */

vtss_rc vtss_stp_port_state_get(const vtss_inst_t     inst,
                                const vtss_port_no_t  port_no,
                                vtss_stp_state_t      *const state)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *state = vtss_state->stp_state[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_stp_port_state_set(const vtss_inst_t       inst,
                                const vtss_port_no_t    port_no,
                                const vtss_stp_state_t  state)
{
    vtss_rc rc;

    VTSS_I("port_no: %u, state: %s",
           port_no,
           state == VTSS_STP_STATE_DISCARDING ? "discard" :
           state == VTSS_STP_STATE_LEARNING ? "learn" : "forward");
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->stp_state[port_no] = state;
        rc = vtss_update_masks(1, 0, 1);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mstp_vlan_msti_get(const vtss_inst_t  inst,
                                const vtss_vid_t   vid,
                                vtss_msti_t        *const msti)
{
    vtss_rc rc;

    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *msti = vtss_state->vlan_table[vid].msti;
    VTSS_EXIT();
    return rc;
}

static vtss_rc vtss_msti_check(const vtss_msti_t msti)
{
    if (msti < VTSS_MSTI_START || msti >= VTSS_MSTI_END) {
        VTSS_E("illegal msti: %u", msti);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_mstp_vlan_msti_set(const vtss_inst_t  inst,
                                const vtss_vid_t   vid,
                                const vtss_msti_t  msti)
{
    vtss_rc rc;

    VTSS_N("vid: %u, msti: %u", vid, msti);

    VTSS_RC(vtss_vid_check(vid));
    VTSS_RC(vtss_msti_check(msti));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->vlan_table[vid].msti = msti;
        rc = VTSS_FUNC_COLD(mstp_vlan_msti_set, vid);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mstp_port_msti_state_get(const vtss_inst_t     inst,
                                      const vtss_port_no_t  port_no,
                                      const vtss_msti_t     msti,
                                      vtss_stp_state_t      *const state)
{
    vtss_rc rc;

    VTSS_RC(vtss_msti_check(msti));
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *state = vtss_state->mstp_table[msti].state[port_no - VTSS_PORT_NO_START];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mstp_port_msti_state_set(const vtss_inst_t       inst,
                                      const vtss_port_no_t    port_no,
                                      const vtss_msti_t       msti,
                                      const vtss_stp_state_t  state)
{
    vtss_rc rc;

    VTSS_N("port_no: %u, msti: %u, state: %s",
           port_no, msti,
           state == VTSS_STP_STATE_DISCARDING ? "discard" :
           state == VTSS_STP_STATE_LEARNING ? "learn" : "forward");

    VTSS_RC(vtss_msti_check(msti));
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->mstp_table[msti].state[port_no - VTSS_PORT_NO_START] = state;
        rc = VTSS_FUNC_COLD(mstp_state_set, port_no, msti);
    }
    VTSS_EXIT();
    return rc;
}

static vtss_rc vtss_erpi_check(const vtss_erpi_t erpi)
{
    if (erpi >= VTSS_ERPI_END) {
        VTSS_E("illegal erpi: %u", erpi);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_erps_vlan_member_get(const vtss_inst_t inst,
                                  const vtss_erpi_t erpi,
                                  const vtss_vid_t  vid,
                                  BOOL              *const member)
{
    vtss_rc rc;

    VTSS_RC(vtss_erpi_check(erpi));
    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *member = VTSS_BF_GET(vtss_state->erps_table[erpi].vlan_member, vid);
    VTSS_EXIT();
    return rc;
}

/* Update ERPS discard state for VLAN */
static void vtss_erps_vlan_update(vtss_erpi_t erpi, vtss_vid_t vid, BOOL member)
{
    vtss_port_no_t    port_no;
    vtss_vlan_entry_t *vlan_entry;
    vtss_erps_entry_t *erps_entry = &vtss_state->erps_table[erpi];

    if (VTSS_BF_GET(erps_entry->vlan_member, vid) != member) {
        VTSS_BF_SET(erps_entry->vlan_member, vid, member);

        /* Update ERPI discard state for VLAN */
        vlan_entry = &vtss_state->vlan_table[vid];
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (!VTSS_PORT_BF_GET(erps_entry->port_member, port_no)) {
                /* Update number of rings with port in discarding state */
                if (member)
                    vlan_entry->erps_discard_cnt[port_no]++;
                else
                    vlan_entry->erps_discard_cnt[port_no]--;
            }
        }
    }
}

vtss_rc vtss_erps_vlan_member_set(const vtss_inst_t inst,
                                  const vtss_erpi_t erpi,
                                  const vtss_vid_t  vid,
                                  const BOOL        member)
{
    vtss_rc rc;

    VTSS_N("erpi: %u, vid: %u, member: %u", erpi, vid, member);

    VTSS_RC(vtss_erpi_check(erpi));
    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_erps_vlan_update(erpi, vid, VTSS_BOOL(member));
        rc = VTSS_FUNC_COLD(erps_vlan_member_set, erpi, vid);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_erps_port_state_get(const vtss_inst_t    inst,
                                 const vtss_erpi_t    erpi,
                                 const vtss_port_no_t port_no,
                                 vtss_erps_state_t    *const state)
{
    vtss_rc rc;

    VTSS_RC(vtss_erpi_check(erpi));
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *state = (VTSS_PORT_BF_GET(vtss_state->erps_table[erpi].port_member, port_no) ?
                  VTSS_ERPS_STATE_FORWARDING : VTSS_ERPS_STATE_DISCARDING);
    VTSS_EXIT();
    return rc;
}

/* Update ERPS discard state for port */
static BOOL vtss_erps_port_update(vtss_erpi_t erpi, vtss_port_no_t port_no, BOOL forward)
{
    vtss_vid_t          vid;
    vtss_vlan_entry_t   *vlan_entry;
    vtss_erps_entry_t   *erps_entry = &vtss_state->erps_table[erpi];
    vtss_erps_counter_t count;

    /* Check if forwarding state has changed */
    if (forward == VTSS_PORT_BF_GET(erps_entry->port_member, port_no))
        return 0;
    VTSS_PORT_BF_SET(erps_entry->port_member, port_no, forward);

    /* Update ERPI discard state for port in VLANs enabled for the ring */
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        if (VTSS_BF_GET(erps_entry->vlan_member, vid)) {
            vlan_entry = &vtss_state->vlan_table[vid];
            count = vlan_entry->erps_discard_cnt[port_no];
            if (forward) {
                /* Change to forwarding state */
                count--;
                if (count == 0)
                    vlan_entry->update = 1;
            } else {
                /* Change to discarding state */
                count++;
                if (count == 1)
                    vlan_entry->update = 1;
            }
            vlan_entry->erps_discard_cnt[port_no] = count;
        }
    }

    return 1;
}

vtss_rc vtss_erps_port_state_set(const vtss_inst_t       inst,
                                 const vtss_erpi_t       erpi,
                                 const vtss_port_no_t    port_no,
                                 const vtss_erps_state_t state)
{
    vtss_rc rc;
    BOOL    forward = VTSS_BOOL(state == VTSS_ERPS_STATE_FORWARDING);

    VTSS_N("erpi: %u, port_no: %u, state: %s",
           erpi, port_no, forward ? "forward" : "discard");

    VTSS_RC(vtss_erpi_check(erpi));
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        if (vtss_erps_port_update(erpi, port_no, forward))
            rc = VTSS_FUNC_COLD(erps_port_state_set, erpi, port_no);
    }
    VTSS_EXIT();
    return rc;
}

/* - VLAN ---------------------------------------------------------- */

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
vtss_rc vtss_vlan_conf_get(const vtss_inst_t inst,
                           vtss_vlan_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *conf = vtss_state->vlan_conf;
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_conf_set(const vtss_inst_t      inst,
                           const vtss_vlan_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->vlan_conf = *conf;
        rc = VTSS_FUNC_COLD(vlan_conf_set);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */

vtss_rc vtss_vlan_port_conf_get(const vtss_inst_t      inst,
                                const vtss_port_no_t   port_no,
                                vtss_vlan_port_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_N("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->vlan_port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_port_conf_set(const vtss_inst_t            inst,
                                const vtss_port_no_t         port_no,
                                const vtss_vlan_port_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->vlan_port_conf[port_no] = *conf;
        rc = VTSS_FUNC_COLD(vlan_port_conf_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_port_members_get(const vtss_inst_t  inst,
                                   const vtss_vid_t   vid,
                                   BOOL               member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = VTSS_PORT_BF_GET(vtss_state->vlan_table[vid].member, port_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_port_members_set(const vtss_inst_t  inst,
                                   const vtss_vid_t   vid,
                                   const BOOL         member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc           rc;
    vtss_port_no_t    port_no;
    vtss_vlan_entry_t *vlan_entry;

    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vlan_entry = &vtss_state->vlan_table[vid];
        vlan_entry->enabled = 0;
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            VTSS_PORT_BF_SET(vlan_entry->member, port_no, member[port_no]);
            if (member[port_no])
                vlan_entry->enabled = 1;
        }
        rc = VTSS_FUNC_COLD(vlan_port_members_set, vid);
    }
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_FEATURE_VLAN_TX_TAG)
vtss_rc vtss_vlan_tx_tag_get(const vtss_inst_t  inst,
                             const vtss_vid_t   vid,
                             vtss_vlan_tx_tag_t tx_tag[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("vid: %u", vid);
    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            tx_tag[port_no] = vtss_state->vlan_table[vid].tx_tag[port_no];
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_tx_tag_set(const vtss_inst_t        inst,
                             const vtss_vid_t         vid,
                             const vtss_vlan_tx_tag_t tx_tag[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    
    VTSS_D("vid: %u", vid);
    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(vlan_tx_tag_set, vid, tx_tag);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_VLAN_TX_TAG */

/* - Port Isolation------------------------------------------------- */

vtss_rc vtss_isolated_vlan_get(const vtss_inst_t  inst,
                               const vtss_vid_t   vid,
                               BOOL               *const isolated)
{
    vtss_rc rc;

    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *isolated = vtss_state->vlan_table[vid].isolated;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_isolated_vlan_set(const vtss_inst_t  inst,
                               const vtss_vid_t   vid,
                               const BOOL         isolated)
{
    vtss_rc rc;

    VTSS_RC(vtss_vid_check(vid));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->vlan_table[vid].isolated = isolated;
        rc = VTSS_FUNC_COLD(isolated_vlan_set, vid);
    }
    VTSS_EXIT();
    return rc;

}

vtss_rc vtss_isolated_port_members_get(const vtss_inst_t  inst,
                                       BOOL               member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = vtss_state->isolated_port[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_isolated_port_members_set(const vtss_inst_t  inst,
                                       const BOOL         member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            vtss_state->isolated_port[port_no] = member[port_no];
        rc = VTSS_FUNC_COLD(isolated_port_members_set);
    }
    VTSS_EXIT();
    return rc;
}

 /* - Private VLAN (PVLAN) ------------------------------------------ */

#if defined(VTSS_FEATURE_PVLAN)
static vtss_rc vtss_pvlan_no_check(const vtss_pvlan_no_t pvlan_no)
{
    if (pvlan_no < VTSS_PVLAN_NO_START || pvlan_no >= VTSS_PVLAN_NO_END) {
        VTSS_E("illegal pvlan_no: %u", pvlan_no);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_pvlan_port_members_get(const vtss_inst_t      inst,
                                    const vtss_pvlan_no_t  pvlan_no,
                                    BOOL                   member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc            rc;
    vtss_port_no_t     port_no;
    vtss_pvlan_entry_t *pvlan_entry;

    VTSS_RC(vtss_pvlan_no_check(pvlan_no));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        pvlan_entry = &vtss_state->pvlan_table[pvlan_no];
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = pvlan_entry->member[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_pvlan_port_members_set(const vtss_inst_t      inst,
                                    const vtss_pvlan_no_t  pvlan_no,
                                    const BOOL             member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc            rc;
    vtss_port_no_t     port_no;
    vtss_pvlan_entry_t *pvlan_entry;

    VTSS_RC(vtss_pvlan_no_check(pvlan_no));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        pvlan_entry = &vtss_state->pvlan_table[pvlan_no];
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            pvlan_entry->member[port_no] = member[port_no];
        rc = vtss_update_masks(1, 0, 0);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_apvlan_port_members_get(const vtss_inst_t    inst,
                                     const vtss_port_no_t port_no,
                                     BOOL                 member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t e_port;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        for (e_port = VTSS_PORT_NO_START; e_port < vtss_state->port_count; e_port++) {
            member[e_port] = vtss_state->apvlan_table[port_no][e_port];
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_apvlan_port_members_set(const vtss_inst_t    inst,
                                     const vtss_port_no_t port_no,
                                     const BOOL           member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t e_port;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        for (e_port = VTSS_PORT_NO_START; e_port < vtss_state->port_count; e_port++) {
            vtss_state->apvlan_table[port_no][e_port] = member[e_port];
        }
        rc = vtss_update_masks(1, 0, 0);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_PVLAN */

vtss_rc vtss_dgroup_port_conf_get(const vtss_inst_t       inst,
                                  const vtss_port_no_t    port_no,
                                  vtss_dgroup_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->dgroup_port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_dgroup_port_conf_set(const vtss_inst_t             inst,
                                  const vtss_port_no_t          port_no,
                                  const vtss_dgroup_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK &&
        (rc = vtss_port_no_check(conf->dgroup_no)) == VTSS_RC_OK) {
        vtss_state->dgroup_port_conf[port_no] = *conf;
        rc = vtss_update_masks(0, 1, 0);
    }
    VTSS_EXIT();
    return rc;
}

/* - Aggregation --------------------------------------------------- */

#if defined(VTSS_FEATURE_VSTAX_V2)
static vtss_rc vtss_vstax_glag_conf_set(const vtss_glag_no_t           glag_no,
                                        const vtss_vstax_glag_entry_t  entry[VTSS_GLAG_PORT_ARRAY_SIZE])
{
    u32              port, port_no, gport;
    vtss_port_map_t  *map;
    vtss_glag_conf_t *conf;

    /* Delete old local GLAG members */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (vtss_state->port_glag_no[port_no] == glag_no)
            vtss_state->port_glag_no[port_no] = VTSS_GLAG_NO_NONE;
    }
    
    for (gport = VTSS_GLAG_PORT_START; gport < VTSS_GLAG_PORT_END; gport++) {
        conf = &vtss_state->glag_conf[glag_no - VTSS_GLAG_NO_START][gport];
        conf->entry = entry[gport];
        if ((port = entry[gport].upspn) == VTSS_UPSPN_NONE) {
            /* End of list */
            if (gport == VTSS_GLAG_PORT_START) /* Deleting GLAG, flush MAC table */
                vtss_mac_glag_flush(glag_no);
            break;
        }
               
        /* Update local members */
        conf->port_no = VTSS_PORT_NO_NONE;
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            map = &vtss_state->port_map[port_no];
            if (vtss_state->vstax_info.upsid[map->chip_no] == entry[gport].upsid &&
                map->chip_port == port && !vtss_state->vstax_port_enabled[port]) {
                /* Found local front port */
                vtss_state->port_glag_no[port_no] = glag_no;
                conf->port_no = port_no;
            }
        }
    }

    /* Update masks */
    return vtss_update_masks(0, 1, 1);
}
#endif /* VTSS_FEATURE_VSTAX_V2 */

static vtss_rc vtss_aggr_no_check(const vtss_aggr_no_t aggr_no)
{
    if (aggr_no >= (vtss_state->port_count/2)) {
        VTSS_E("illegal aggr_no: %u", aggr_no);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_aggr_port_members_get(const vtss_inst_t     inst,
                                   const vtss_aggr_no_t  aggr_no,
                                   BOOL                  member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;
    vtss_glag_no_t glag_no;

    VTSS_D("aggr_no: %u", aggr_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_aggr_no_check(aggr_no)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            glag_no = vtss_state->port_aggr_no[port_no];
#if defined(VTSS_FEATURE_VSTAX_V2)
            if (vtss_state->chip_count == 2) {
                /* LLAGs implemented using GLAGs */
                glag_no = vtss_state->port_glag_no[port_no];
            }
#endif /* VTSS_FEATURE_VSTAX_V2 */
            member[port_no] = VTSS_BOOL(glag_no == aggr_no);
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_aggr_port_members_set(const vtss_inst_t     inst,
                                   const vtss_aggr_no_t  aggr_no,
                                   const BOOL            member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("aggr_no: %u", aggr_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_aggr_no_check(aggr_no)) == VTSS_RC_OK) {
#if defined(VTSS_FEATURE_VSTAX_V2)
        if (vtss_state->chip_count == 2) {
            /* LLAGs implemented using GLAGs */
            vtss_glag_no_t          glag_no = aggr_no;
            vtss_vstax_glag_entry_t entry[VTSS_GLAG_PORT_ARRAY_SIZE];
            u32                     gport = 0;
            vtss_port_map_t         *map;

            for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
                if (member[port_no]) {
                    map = &vtss_state->port_map[port_no];
                    entry[gport].upsid = vtss_state->vstax_info.upsid[map->chip_no];
                    entry[gport].upspn = map->chip_port;
                    gport++;
                    if (gport == VTSS_GLAG_PORT_ARRAY_SIZE)
                        break;
                }
            }
            if (gport != VTSS_GLAG_PORT_ARRAY_SIZE) /* Mark end of list */
                entry[gport].upspn = VTSS_UPSPN_NONE;
            rc = vtss_vstax_glag_conf_set(glag_no, entry);
        } else
#endif /* VTSS_FEATURE_VSTAX_V2 */
        {
            for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
                if (member[port_no])
                    vtss_state->port_aggr_no[port_no] = aggr_no;
                else if (vtss_state->port_aggr_no[port_no] == aggr_no)
                    vtss_state->port_aggr_no[port_no] = VTSS_AGGR_NO_NONE;
            }
            rc = vtss_update_masks(1, 1, 1);
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_aggr_mode_get(const vtss_inst_t  inst,
                           vtss_aggr_mode_t   *const mode)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *mode = vtss_state->aggr_mode;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_aggr_mode_set(const vtss_inst_t       inst,
                           const vtss_aggr_mode_t  *const mode)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->aggr_mode = *mode;
        rc = VTSS_FUNC(aggr_mode_set);
    }
    VTSS_EXIT();
    return rc;
}

/* - Global link aggregations across stack ------------------------- */

#if defined(VTSS_FEATURE_AGGR_GLAG)
static vtss_rc vtss_glag_no_check(const vtss_glag_no_t glag_no)
{
    if (glag_no < VTSS_GLAG_NO_START || glag_no >= VTSS_GLAG_NO_END) {
        VTSS_E("illegal glag_no: %u", glag_no);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_aggr_glag_members_get(const vtss_inst_t     inst,
                                   const vtss_glag_no_t  glag_no,
                                   BOOL                  member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("glag_no: %u", glag_no);
    VTSS_RC(vtss_glag_no_check(glag_no));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = VTSS_BOOL(vtss_state->port_glag_no[port_no] == glag_no &&
                                        vtss_state->port_state[port_no]);
    }
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_FEATURE_VSTAX_V1)
static vtss_rc vtss_glag_set(vtss_glag_no_t        glag_no,
                             const u32  member[VTSS_GLAG_PORT_ARRAY_SIZE])
{
    vtss_port_no_t port_no;
    u32            gport;

    /* Delete old GLAG members */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        if (vtss_state->port_glag_no[port_no] == glag_no)
            vtss_state->port_glag_no[port_no] = VTSS_GLAG_NO_NONE;

    /* Add new GLAG member state */
    for (gport = VTSS_GLAG_PORT_START; gport < VTSS_GLAG_PORT_END; gport++) {
        port_no = member[gport];
        vtss_state->glag_members[glag_no-VTSS_GLAG_NO_START][gport] = port_no;
        if (port_no == VTSS_PORT_NO_NONE) {
            /* End of list */
            if (gport == VTSS_GLAG_PORT_START) /* Deleting GLAG, flush MAC table */
                vtss_mac_glag_flush(glag_no);
            break;
        }

        VTSS_RC(vtss_port_no_check(port_no));

        if (!vtss_state->vstax_port_enabled[port_no])
            vtss_state->port_glag_no[port_no] = glag_no;
    }

    /* Update masks */
    return vtss_update_masks(0, 1, 1);
}


vtss_rc vtss_aggr_glag_set(const vtss_inst_t     inst,
                           vtss_glag_no_t        glag_no,
                           const u32 member[VTSS_GLAG_PORT_ARRAY_SIZE])
{
    vtss_rc rc;

    VTSS_D("glag_no: %u", glag_no);

    VTSS_RC(vtss_glag_no_check(glag_no));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_glag_set(glag_no, member);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_VSTAX_V1 */

#if defined(VTSS_FEATURE_VSTAX_V2)
vtss_rc vtss_vstax_glag_get(const vtss_inst_t        inst,
                            const vtss_glag_no_t     glag_no,
                            vtss_vstax_glag_entry_t  entry[VTSS_GLAG_PORT_ARRAY_SIZE])
{
    vtss_rc rc;
    u32     gentry;

    VTSS_D("glag_no: %u", glag_no);
    VTSS_RC(vtss_glag_no_check(glag_no));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (gentry = VTSS_GLAG_PORT_START; gentry < VTSS_GLAG_PORT_END; gentry++) {            
            entry[gentry] = vtss_state->glag_conf[glag_no-VTSS_GLAG_NO_START][gentry].entry;
        }        
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vstax_glag_set(const vtss_inst_t              inst,
                            const vtss_glag_no_t           glag_no,
                            const vtss_vstax_glag_entry_t  entry[VTSS_GLAG_PORT_ARRAY_SIZE])
{
    vtss_rc rc;

    VTSS_D("glag_no: %u", glag_no);

    VTSS_RC(vtss_glag_no_check(glag_no));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = vtss_vstax_glag_conf_set(glag_no, entry);        
    }
        
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_VSTAX_V2 */

#endif /* VTSS_FEATURE_AGGR_GLAG */

/* - Mirroring ----------------------------------------------------- */

vtss_rc vtss_mirror_conf_get(const vtss_inst_t  inst,
                             vtss_mirror_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *conf = vtss_state->mirror_conf;
    }
        
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_mirror_conf_set(const vtss_inst_t        inst,
                             const vtss_mirror_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_port_no_none_check(conf->port_no)) == VTSS_RC_OK) {
        vtss_state->mirror_conf = *conf;
        rc = VTSS_FUNC_COLD(mirror_port_set);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mirror_monitor_port_get(const vtss_inst_t  inst,
                                     vtss_port_no_t     *const port_no)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *port_no = vtss_state->mirror_conf.port_no;
    }
        
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mirror_monitor_port_set(const vtss_inst_t     inst,
                                     const vtss_port_no_t  port_no)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_port_no_none_check(port_no)) == VTSS_RC_OK) {
        vtss_state->mirror_conf.port_no = port_no;
        rc = VTSS_FUNC_COLD(mirror_port_set);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mirror_ingress_ports_get(const vtss_inst_t  inst,
                                      BOOL               member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = vtss_state->mirror_ingress[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mirror_ingress_ports_set(const vtss_inst_t  inst,
                                      const BOOL         member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            vtss_state->mirror_ingress[port_no] = VTSS_BOOL(member[port_no]);
        rc = VTSS_FUNC_COLD(mirror_ingress_set);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mirror_egress_ports_get(const vtss_inst_t  inst,
                                     BOOL               member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = vtss_state->mirror_egress[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mirror_egress_ports_set(const vtss_inst_t  inst,
                                     const BOOL         member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            vtss_state->mirror_egress[port_no] = VTSS_BOOL(member[port_no]);
        rc = VTSS_FUNC_COLD(mirror_egress_set);
    }
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_mirror_cpu_egress_get(const vtss_inst_t  inst,
                                   BOOL               *member)
{
    vtss_rc        rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *member = vtss_state->mirror_cpu_egress;
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mirror_cpu_egress_set(const vtss_inst_t  inst,
                                     const BOOL       member)
{
    vtss_rc        rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->mirror_cpu_egress = VTSS_BOOL(member);
        rc = VTSS_FUNC_COLD(mirror_cpu_egress_set);
    }
    VTSS_EXIT();
    return rc;
}





vtss_rc vtss_mirror_cpu_ingress_get(const vtss_inst_t  inst,
                                     BOOL              *member)
{
    vtss_rc        rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *member = vtss_state->mirror_cpu_ingress;
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mirror_cpu_ingress_set(const vtss_inst_t  inst,
                                    const BOOL         member)
{
    vtss_rc        rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->mirror_cpu_ingress = VTSS_BOOL(member);
        rc = VTSS_FUNC_COLD(mirror_cpu_ingress_set);
    }
    VTSS_EXIT();
    return rc;
}



vtss_rc vtss_eps_port_conf_get(const vtss_inst_t     inst,
                               const vtss_port_no_t  port_no,
                               vtss_eps_port_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->port_protect[port_no].conf;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_eps_port_conf_set(const vtss_inst_t           inst,
                               const vtss_port_no_t        port_no,
                               const vtss_eps_port_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u, type: %u, conf.port_no: %u", port_no, conf->type, conf->port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->port_protect[port_no].conf = *conf;
        rc = vtss_update_masks(1, 1, 1);
        if (rc == VTSS_RC_OK)
            rc = VTSS_FUNC_COLD(eps_port_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_eps_port_selector_get(const vtss_inst_t    inst,
                                   const vtss_port_no_t port_no,
                                   vtss_eps_selector_t  *const selector)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *selector = vtss_state->port_protect[port_no].selector;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_eps_port_selector_set(const vtss_inst_t          inst,
                                   const vtss_port_no_t       port_no,
                                   const vtss_eps_selector_t  selector)
{
    vtss_rc              rc;
    vtss_eps_port_conf_t *conf;

    VTSS_D("port_no: %u selector: %u", port_no, selector);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->port_protect[port_no].selector = selector;
        rc = vtss_update_masks(1, 0, 1);
        if (rc == VTSS_RC_OK)
            rc = VTSS_FUNC_COLD(eps_port_set, port_no);
        conf = &vtss_state->port_protect[port_no].conf;
        if (rc == VTSS_RC_OK && conf->type == VTSS_EPS_PORT_1_FOR_1) {
            /* Flush port for 1:1 protection */
            if (selector == VTSS_EPS_SELECTOR_PROTECTION) {
                /* Flush working port */
                rc = vtss_mac_age(1, port_no, 0, 0, 2);
            } else if (conf->port_no != VTSS_PORT_NO_NONE) {
                /* Flush protecting port */
                rc = vtss_mac_age(1, conf->port_no, 0, 0, 2);
            }
        }
    }
    VTSS_EXIT();
    return rc;
}

/* - Flooding control ---------------------------------------------- */
vtss_rc vtss_uc_flood_members_get(const vtss_inst_t  inst,
                                  BOOL               member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = vtss_state->uc_flood[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_uc_flood_members_set(const vtss_inst_t  inst,
                                  const BOOL         member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            vtss_state->uc_flood[port_no] = member[port_no];
        rc = VTSS_FUNC_COLD(flood_conf_set);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mc_flood_members_get(const vtss_inst_t  inst,
                                  BOOL               member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = vtss_state->mc_flood[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mc_flood_members_set(const vtss_inst_t  inst,
                                  const BOOL         member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            vtss_state->mc_flood[port_no] = member[port_no];
        rc = VTSS_FUNC_COLD(flood_conf_set);
    }
    VTSS_EXIT();
    return rc;
}

/* - IGMP ---------------------------------------------------- */

vtss_rc vtss_ipv4_mc_flood_members_get(const vtss_inst_t  inst,
                                       BOOL               member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = vtss_state->ipv4_mc_flood[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ipv4_mc_flood_members_set(const vtss_inst_t  inst,
                                       const BOOL         member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            vtss_state->ipv4_mc_flood[port_no] = VTSS_BOOL(member[port_no]);
        rc = VTSS_FUNC_COLD(flood_conf_set);
    }
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_FEATURE_IPV4_MC_SIP)
vtss_rc vtss_ipv4_mc_add(const vtss_inst_t inst,
                         const vtss_vid_t  vid,
                         const vtss_ip_t   sip,
                         const vtss_ip_t   dip,
                         const BOOL        member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ipv4_mc_add, vid, sip, dip, member);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ipv4_mc_del(const vtss_inst_t inst,
                         const vtss_vid_t  vid,
                         const vtss_ip_t   sip,
                         const vtss_ip_t   dip)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ipv4_mc_del, vid, sip, dip);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_IPV4_MC_SIP */

/* - MLD  ----------------------------------------------------- */

#if defined(VTSS_FEATURE_IPV6_MC_SIP)
vtss_rc vtss_ipv6_mc_add(const vtss_inst_t inst,
                         const vtss_vid_t  vid,
                         const vtss_ipv6_t sip,
                         const vtss_ipv6_t dip,
                         const BOOL        member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ipv6_mc_add, vid, sip, dip, member);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ipv6_mc_del(const vtss_inst_t inst,
                         const vtss_vid_t  vid,
                         const vtss_ipv6_t sip,
                         const vtss_ipv6_t dip)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ipv6_mc_del, vid, sip, dip);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_IPV6_MC_SIP */

vtss_rc vtss_ipv6_mc_flood_members_get(const vtss_inst_t  inst,
                                       BOOL               member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = vtss_state->ipv6_mc_flood[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ipv6_mc_flood_members_set(const vtss_inst_t  inst,
                                       const BOOL         member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc        rc;
    vtss_port_no_t port_no;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            vtss_state->ipv6_mc_flood[port_no] = VTSS_BOOL(member[port_no]);
        rc = VTSS_FUNC_COLD(flood_conf_set);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ipv6_mc_ctrl_flood_get(const vtss_inst_t  inst,
                                    BOOL               *const scope)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *scope = vtss_state->ipv6_mc_scope;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ipv6_mc_ctrl_flood_set(const vtss_inst_t  inst,
                                    const BOOL         scope)
{
    vtss_rc rc;

    VTSS_D("scope: %u", scope);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->ipv6_mc_scope = VTSS_BOOL(scope);
        rc = VTSS_FUNC_COLD(flood_conf_set);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_PORT_CONTROL)
// Gets the forwarding configuration for a port( via the forward pointer )
vtss_rc vtss_port_forward_state_get(const vtss_inst_t     inst,
                                    const vtss_port_no_t  port_no,
                                    vtss_port_forward_t   *const forward)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *forward = vtss_state->port_forward[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_port_forward_state_set(const vtss_inst_t          inst,
                                    const vtss_port_no_t       port_no,
                                    const vtss_port_forward_t  forward)
{
    vtss_rc rc;

    VTSS_D("port_no: %u, forward: %s",
           port_no,
           forward == VTSS_PORT_FORWARD_ENABLED ? "enabled" :
           forward == VTSS_PORT_FORWARD_DISABLED ? "disabled" :
           forward == VTSS_PORT_FORWARD_INGRESS ? "ingress" : "egress");
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->port_forward[port_no] = forward;
        rc = VTSS_FUNC_COLD(port_forward_set, port_no);
#if defined(VTSS_FEATURE_LAYER2)
        if (rc == VTSS_RC_OK)
            rc = vtss_update_masks(1, 0, 1);
#endif /* VTSS_FEATURE_LAYER2 */

    }
    VTSS_EXIT();
    return rc;
}

/* Internal feature: Access MII directly without lock */
#define VTSS_MIIM_ADDR_MASK 0x7f

/* MII management read function (direct - not via port map) */
vtss_rc vtss_miim_read(const vtss_inst_t            inst,
                       const vtss_chip_no_t         chip_no,
                       const vtss_miim_controller_t miim_controller,
                       const u8                     miim_addr,
                       const u8                     addr,
                       u16                          *const value)
{
    vtss_rc rc;
    u8      new_addr = (miim_addr & VTSS_MIIM_ADDR_MASK);

    /* Direct access */
    if (new_addr != miim_addr)
        return VTSS_FUNC(miim_read, miim_controller, new_addr, addr, value, FALSE);
    
    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(miim_read, miim_controller, miim_addr, addr, value, FALSE);
    VTSS_EXIT();
    return rc;
}

/* MII management write function - (direct - not via port map) */
vtss_rc vtss_miim_write(const vtss_inst_t            inst,
                        const vtss_chip_no_t         chip_no,
                        const vtss_miim_controller_t miim_controller,
                        const u8                     miim_addr,
                        const u8                     addr,
                        const u16                    value)
{
    vtss_rc rc;
    u8      new_addr = (miim_addr & VTSS_MIIM_ADDR_MASK);

    /* Direct access */
    if (new_addr != miim_addr)
        return VTSS_FUNC(miim_write, miim_controller, new_addr, addr, value, FALSE);

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(miim_write, miim_controller, miim_addr, addr, value, FALSE);
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_CHIP_10G_PHY) 
/* MMD management write function (direct - not via port map) */
vtss_rc vtss_mmd_write(const vtss_inst_t            inst,
                       const vtss_chip_no_t         chip_no,
                       const vtss_miim_controller_t miim_controller,
                       const u8                     miim_addr,
                       const u8                     mmd,
                       const u16                    addr,
                       const u16                    value)
{
    vtss_rc                rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mmd_write, miim_controller, miim_addr, mmd, addr, value, FALSE);
    }

    if (rc == VTSS_RC_OK) {
        VTSS_N("miim_addr: %u, mmd: %u, addr: 0x%04x, value: 0x%04x", miim_addr, mmd, addr, value);
    }
    VTSS_EXIT();
    return rc;
}
/* MMD management read function (direct - not via port map) */
vtss_rc vtss_mmd_read(const vtss_inst_t            inst,
                      const vtss_chip_no_t         chip_no,
                      const vtss_miim_controller_t miim_controller,
                      const u8                     miim_addr,
                      const u8                     mmd,
                      const u16                    addr,
                      u16                          *const value)
{
    vtss_rc                rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mmd_read, miim_controller, miim_addr, mmd, addr, value, FALSE);
    }

    if (rc == VTSS_RC_OK) {
        VTSS_N("miim_addro: %u, mmd: %u, addr: 0x%04x, value: 0x%04x",
               miim_addr, mmd, addr, *value);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_CHIP_10G_PHY */
#endif /* VTSS_FEATURE_PORT_CONTROL */

#if defined(VTSS_ARCH_B2)
vtss_rc vtss_vid2port_set(const vtss_inst_t     inst,
                          const vtss_vid_t     vid,
                          const vtss_port_no_t port_no)
{
    vtss_rc rc;

    VTSS_D("vid: %u, port_no: %u", vid, port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check_allow_port_none(inst, port_no)) == VTSS_RC_OK) {
        if (vid > VTSS_VID_RESERVED) {
          VTSS_E("illegal vid: %u", vid);
          rc = VTSS_RC_ERROR;
        } else {
          rc = VTSS_FUNC(vid2port_set, vid, port_no);
        }
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_vid2lport_set(const vtss_inst_t     inst,
                           const vtss_vid_t      vid,
                           const vtss_lport_no_t lport_no)
{
    vtss_rc rc;

    VTSS_D("vid: %u, lport_no: %u", vid, lport_no);
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (vid > VTSS_VID_RESERVED) {
            VTSS_E("illegal vid: %u", vid);
            return VTSS_RC_ERROR;
        }

        if (lport_no >= VTSS_LPORTS && lport_no != VTSS_LPORT_NONE) {
            VTSS_E("illegal lport_no: %u", lport_no);
            return VTSS_RC_ERROR;
        }

        rc = VTSS_FUNC(vid2lport_set, vid, lport_no);
    }
    return rc;
}
#endif /* VTSS_ARCH_B2 */


#if defined(VTSS_FEATURE_VSTAX)
vtss_rc vtss_vstax_conf_get(const vtss_inst_t  inst,
                            vtss_vstax_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *conf = vtss_state->vstax_conf;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vstax_conf_set(const vtss_inst_t        inst,
                            const vtss_vstax_conf_t  *const conf)
{
    vtss_rc        rc;
    vtss_port_no_t port_no;
    BOOL           enabled, changed = 0;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (!VTSS_VSTAX_UPSID_LEGAL(conf->upsid_0)) {
            VTSS_E("illegal upsid_0: %d", conf->upsid_0);
            rc = VTSS_RC_ERROR;
        } else if (!VTSS_VSTAX_UPSID_LEGAL(conf->upsid_1)) {
            VTSS_E("illegal upsid_1: %d", conf->upsid_1);
            rc = VTSS_RC_ERROR;
        } else if ((rc = vtss_port_no_none_check(conf->port_0)) == VTSS_RC_OK &&
                   (rc = vtss_port_no_none_check(conf->port_1)) == VTSS_RC_OK) {
            /* Store VStaX configuration directly */
            vtss_state->vstax_conf = *conf;

            /* Store UPSIDs in VStaX info */
            vtss_state->vstax_info.upsid[0] = vtss_state->vstax_conf.upsid_0;
            vtss_state->vstax_info.upsid[1] = vtss_state->vstax_conf.upsid_1;
            
            /* Store information about VStaX enabled ports */
            for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
                enabled = VTSS_BOOL(port_no == conf->port_0 || port_no == conf->port_1);
                if (vtss_state->vstax_port_enabled[port_no] != enabled) {
                    vtss_state->vstax_port_enabled[port_no] = enabled;
                    changed = 1;
                }
            }
            
            /* Set VStaX configuration */
            rc = VTSS_FUNC(vstax_conf_set);

            /* Update VLAN memberships if stack ports changed */
            if (changed)
                rc = vtss_cmn_vlan_update_all();
        }
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vstax_port_conf_get(const vtss_inst_t      inst,
                                 const vtss_chip_no_t   chip_no,
                                 const BOOL             stack_port_a,
                                 vtss_vstax_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("chip_no: %u, port %s", chip_no, stack_port_a ? "A" : "B");
    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK)
        *conf = vtss_state->vstax_info.chip_info[chip_no].port_conf[stack_port_a ? 0 : 1];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vstax_port_conf_set(const vtss_inst_t            inst,
                                 const vtss_chip_no_t         chip_no,
                                 const BOOL                   stack_port_a,
                                 const vtss_vstax_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("chip_no: %u, port %s", chip_no, stack_port_a ? "A" : "B");
    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK) {
        vtss_state->vstax_info.chip_info[chip_no].port_conf[stack_port_a ? 0 : 1] = *conf;
        rc = VTSS_FUNC(vstax_port_conf_set, stack_port_a);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vstax_master_upsid_get(const vtss_inst_t        inst,
                                          vtss_vstax_upsid_t *const master_upsid)
{
    vtss_rc rc;

    if (master_upsid == NULL) {
        return VTSS_RC_ERROR;
    }

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *master_upsid = vtss_state->vstax_info.master_upsid;
    }
    VTSS_EXIT();

    return VTSS_RC_OK;
}

vtss_rc vtss_vstax_master_upsid_set(const vtss_inst_t        inst,
                                    const vtss_vstax_upsid_t master_upsid)
{
    vtss_rc rc;

    if (!VTSS_VSTAX_UPSID_LEGAL(master_upsid) && master_upsid != VTSS_VSTAX_UPSID_UNDEF) {
        return VTSS_RC_ERROR;
    }

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->vstax_info.master_upsid = master_upsid;
        if (vtss_state->cil_func.vstax_master_upsid_set == NULL) {
            rc = VTSS_RC_OK; // If no CIL function exists, don't return an error.
        } else {
            rc = vtss_state->cil_func.vstax_master_upsid_set();
        }
    }
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_FEATURE_VSTAX_V2)
vtss_rc vtss_vstax_topology_set(const vtss_inst_t              inst, 
                                const vtss_chip_no_t           chip_no,
                                const vtss_vstax_route_table_t *table)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_chip_no_check(inst, chip_no)) == VTSS_RC_OK) {
        if (table->topology_type == VTSS_VSTAX_TOPOLOGY_CHAIN ||
            table->topology_type == VTSS_VSTAX_TOPOLOGY_RING) {
            vtss_state->vstax_info.chip_info[chip_no].rt_table = *table;
            rc = VTSS_FUNC(vstax_topology_set);
        } else {
            VTSS_E("illegal topology_type: %d", table->topology_type);
            rc = VTSS_RC_ERROR;
        }
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_VSTAX_V2 */


#endif /* VTSS_FEATURE_VSTAX */

#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
static vtss_rc vtss_evc_policer_id_check(const vtss_evc_policer_id_t policer_id, BOOL resv)
{
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    /* Allow reserved policers */
    if (resv && 
        (policer_id == VTSS_EVC_POLICER_ID_DISCARD || policer_id == VTSS_EVC_POLICER_ID_NONE))
        return VTSS_RC_OK;
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
    
    if (policer_id >= vtss_state->evc_policer_max) {
        VTSS_E("illegal policer_id: %u", policer_id);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_evc_policer_conf_get(const vtss_inst_t           inst,
                                  const vtss_evc_policer_id_t policer_id,
                                  vtss_evc_policer_conf_t     *const conf)
{
    vtss_rc rc;

    VTSS_D("policer_id: %u", policer_id);
    VTSS_RC(vtss_evc_policer_id_check(policer_id, 0));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *conf = vtss_state->evc_policer_conf[policer_id];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_evc_policer_conf_set(const vtss_inst_t             inst,
                                  const vtss_evc_policer_id_t   policer_id,
                                  const vtss_evc_policer_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("policer_id: %u", policer_id);
    VTSS_RC(vtss_evc_policer_id_check(policer_id, 0));
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->evc_policer_conf[policer_id] = *conf;
        rc = VTSS_FUNC_COLD(evc_policer_conf_set, policer_id);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */

#if defined(VTSS_FEATURE_EVC)

vtss_rc vtss_evc_port_conf_get(const vtss_inst_t    inst,
                               const vtss_port_no_t port_no,
                               vtss_evc_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->evc_port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_evc_port_conf_set(const vtss_inst_t          inst,
                               const vtss_port_no_t       port_no,
                               const vtss_evc_port_conf_t *const conf)
{
    vtss_rc              rc;
    vtss_evc_port_conf_t *port_conf;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        port_conf = &vtss_state->evc_port_conf[port_no];
        vtss_state->evc_port_conf_old = *port_conf;
        *port_conf = *conf;
        if ((rc = VTSS_FUNC_COLD(evc_port_conf_set, port_no)) != VTSS_RC_OK) {
            /* Restore configuration if operation failed */
            *port_conf = vtss_state->evc_port_conf_old;
        }
    }
    VTSS_EXIT();
    return rc;
}

static vtss_rc vtss_inst_evc_id_check(const vtss_inst_t   inst,
                                      const vtss_evc_id_t evc_id,
                                      BOOL                allow_none,
                                      BOOL                check_enabled)
{
    vtss_rc rc;

    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (allow_none && evc_id == VTSS_EVC_ID_NONE) {
        } else if (evc_id >= VTSS_EVCS) {
            VTSS_EG(VTSS_TRACE_GROUP_EVC, "%s: illegal evc_id: %u", vtss_func, evc_id);
            rc = VTSS_RC_ERROR;
        } else if (check_enabled && !vtss_state->evc_info.table[evc_id].enable) {
            VTSS_EG(VTSS_TRACE_GROUP_EVC, "%s: evc_id %u is disabled", vtss_func, evc_id);
            rc = VTSS_RC_ERROR;
        }
    }
    return rc;
}

vtss_rc vtss_evc_add(const vtss_inst_t     inst,
                     const vtss_evc_id_t   evc_id,
                     const vtss_evc_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "evc_id: %u", evc_id);
#if defined(VTSS_ARCH_JAGUAR_1)
    VTSS_RC(vtss_evc_policer_id_check(conf->policer_id, 1));
#endif /* VTSS_ARCH_JAGUAR_1 */

    VTSS_ENTER();
    if ((rc = vtss_inst_evc_id_check(inst, evc_id, 0, 0)) == VTSS_RC_OK)
        rc = VTSS_FUNC_COLD(evc_add, evc_id, conf);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_evc_del(const vtss_inst_t   inst,
                     const vtss_evc_id_t evc_id)
{
    vtss_rc rc;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "evc_id: %u", evc_id);
    VTSS_ENTER();
    if ((rc = vtss_inst_evc_id_check(inst, evc_id, 0, 1)) == VTSS_RC_OK)
        rc = VTSS_FUNC_COLD(evc_del, evc_id);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_evc_get(const vtss_inst_t   inst,
                     const vtss_evc_id_t evc_id,
                     vtss_evc_conf_t     *const conf)
{
    vtss_rc                 rc;
    vtss_port_no_t          port_no;
    vtss_evc_entry_t        *evc;
    vtss_evc_pb_conf_t      *pb   = &conf->network.pb;
#if defined(VTSS_FEATURE_MPLS)
    vtss_evc_mpls_tp_conf_t *mpls = &conf->network.mpls_tp;
#endif

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "evc_id: %u", evc_id);
    VTSS_ENTER();
    if ((rc = vtss_inst_evc_id_check(inst, evc_id, 0, 1)) == VTSS_RC_OK) {
        evc = &vtss_state->evc_info.table[evc_id];
        pb->vid       = evc->vid;
        pb->ivid      = evc->ivid;
#if defined(VTSS_ARCH_CARACAL)
        pb->uvid      = evc->uvid;
        pb->inner_tag = evc->inner_tag;
#endif /* VTSS_ARCH_CARACAL */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            pb->nni[port_no] = VTSS_PORT_BF_GET(evc->ports, port_no);
        }
#if defined(VTSS_FEATURE_MPLS)
        mpls->pw_ingress_xc = evc->pw_ingress_xc;
        mpls->pw_egress_xc  = evc->pw_egress_xc;
#endif
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ece_init(const vtss_inst_t     inst,
                      const vtss_ece_type_t type,
                      vtss_ece_t            *const ece)
{
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "type: %d", type);
    switch (type) {
    case VTSS_ECE_TYPE_ANY:
    case VTSS_ECE_TYPE_IPV4:
    case VTSS_ECE_TYPE_IPV6:
        memset(ece, 0, sizeof(*ece));
        ece->key.type = type;
        break;
    default:
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal type: %d", type);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

vtss_rc vtss_ece_add(const vtss_inst_t   inst,
                     const vtss_ece_id_t ece_id,
                     const vtss_ece_t    *const ece)
{
    vtss_rc       rc;
    vtss_evc_id_t evc_id = ece->action.evc_id;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "ece_id: %u, evc_id: %u", ece_id, evc_id);
    VTSS_ENTER();
    if ((rc = vtss_inst_evc_id_check(inst, evc_id, 1, 0)) == VTSS_RC_OK)
        rc = VTSS_FUNC_COLD(ece_add, ece_id, ece);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ece_del(const vtss_inst_t   inst,
                     const vtss_ece_id_t ece_id)
{
    vtss_rc rc;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "ece_id: %u", ece_id);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) 
        rc = VTSS_FUNC_COLD(ece_del, ece_id);
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_FEATURE_OAM)
vtss_rc vtss_evc_oam_port_conf_get(const vtss_inst_t        inst,
                                   const vtss_evc_id_t      evc_id,
                                   const vtss_port_no_t     port_no,
                                   vtss_evc_oam_port_conf_t *const conf)
{
    vtss_rc rc;
    u8      voe_idx;

    VTSS_D("evc_id: %u, port_no: %u", evc_id, port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_evc_id_check(inst, evc_id, 0, 0)) == VTSS_RC_OK &&
        (rc = vtss_port_no_check(port_no)) == VTSS_RC_OK) {
        voe_idx = vtss_state->evc_info.table[evc_id].voe_idx[port_no];
        conf->voe_idx = (voe_idx == VTSS_EVC_VOE_IDX_NONE ? VTSS_OAM_VOE_IDX_NONE : voe_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_evc_oam_port_conf_set(const vtss_inst_t              inst,
                                   const vtss_evc_id_t            evc_id,
                                   const vtss_port_no_t           port_no,
                                   const vtss_evc_oam_port_conf_t *const conf)
{
    vtss_rc          rc;
    vtss_res_t       res;
    vtss_evc_entry_t *evc;

    VTSS_D("evc_id: %u, port_no: %u", evc_id, port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_evc_id_check(inst, evc_id, 0, 0)) == VTSS_RC_OK &&
        (rc = vtss_port_no_check(port_no)) == VTSS_RC_OK) {
        evc = &vtss_state->evc_info.table[evc_id];
        evc->voe_idx[port_no] = (conf->voe_idx == VTSS_OAM_VOE_IDX_NONE ? 
                                 VTSS_EVC_VOE_IDX_NONE : conf->voe_idx);
        if (evc->enable) {
            vtss_cmn_res_init(&res);
            rc = VTSS_FUNC_COLD(evc_update, evc_id, &res, VTSS_RES_CMD_ADD);
        }
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_OAM */

#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
vtss_rc vtss_mce_init(const vtss_inst_t inst,
                      vtss_mce_t        *const mce)
{
    memset(mce, 0, sizeof(*mce));
    return VTSS_RC_OK;
}

vtss_rc vtss_mce_add(const vtss_inst_t   inst,
                     const vtss_mce_id_t mce_id,
                     const vtss_mce_t    *const mce)
{
    vtss_rc       rc;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "mce_id: %u", mce_id);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) 
        rc = VTSS_FUNC_COLD(mce_add, mce_id, mce);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mce_del(const vtss_inst_t   inst,
                     const vtss_mce_id_t mce_id)
{
    vtss_rc rc;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "mce_id: %u", mce_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) 
        rc = VTSS_FUNC_COLD(mce_del, mce_id);
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_ARCH_SERVAL)
vtss_rc vtss_mce_port_info_get(const vtss_inst_t    inst,
                               const vtss_mce_id_t  mce_id,
                               const vtss_port_no_t port_no,
                               vtss_mce_port_info_t *const info)
{
    vtss_rc rc;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "mce_id: %u, port_no: %u", mce_id, port_no);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) 
        rc = VTSS_FUNC_COLD(mce_port_get, mce_id, port_no, info);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_SERVAL */
#endif /* VTSS_ARCH_CARACAL/SERVAL */

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
vtss_rc vtss_evc_counters_get(const vtss_inst_t    inst,
                              const vtss_evc_id_t  evc_id,
                              const vtss_port_no_t port_no,
                              vtss_evc_counters_t  *const counters)
{
    vtss_rc rc;

    VTSS_D("evc_id: %u, port_no: %u", evc_id, port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK &&
        (rc = vtss_inst_evc_id_check(inst, evc_id, 0, 1)) == VTSS_RC_OK)
        rc = VTSS_FUNC_COLD(evc_counters_get, evc_id, port_no, counters);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_evc_counters_clear(const vtss_inst_t    inst,
                                const vtss_evc_id_t  evc_id,
                                const vtss_port_no_t port_no)
{
    vtss_rc rc;

    VTSS_D("evc_id: %u, port_no: %u", evc_id, port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK &&
        (rc = vtss_inst_evc_id_check(inst, evc_id, 0, 1)) == VTSS_RC_OK)
        rc = VTSS_FUNC_COLD(evc_counters_clear, evc_id, port_no);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ece_counters_get(const vtss_inst_t    inst,
                              const vtss_ece_id_t  ece_id,
                              const vtss_port_no_t port_no,
                              vtss_evc_counters_t  *const counters)
{
    vtss_rc rc;

    VTSS_D("ece_id: %u, port_no: %u", ece_id, port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC_COLD(ece_counters_get, ece_id, port_no, counters);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ece_counters_clear(const vtss_inst_t    inst,
                                const vtss_ece_id_t  ece_id,
                                const vtss_port_no_t port_no)
{
    vtss_rc rc;

    VTSS_D("ece_id: %u, port_no: %u", ece_id, port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC_COLD(ece_counters_clear, ece_id, port_no);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
#endif /* VTSS_FEATURE_EVC */

/* ================================================================= *
 *  Packet control
 * ================================================================= */
#if defined(VTSS_FEATURE_PACKET)

vtss_rc vtss_packet_rx_conf_get(const vtss_inst_t      inst,
                                vtss_packet_rx_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *conf = vtss_state->rx_conf;
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_packet_rx_conf_set(const vtss_inst_t            inst,
                                const vtss_packet_rx_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->rx_conf = *conf;
        rc = VTSS_FUNC(rx_conf_set);
    }
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_FEATURE_PACKET_PORT_REG)
vtss_rc vtss_packet_rx_port_conf_get(const vtss_inst_t          inst,
                                     const vtss_port_no_t       port_no,
                                     vtss_packet_rx_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->rx_port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_packet_rx_port_conf_set(const vtss_inst_t                inst,
                                     const vtss_port_no_t             port_no,
                                     const vtss_packet_rx_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("enter");
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->rx_port_conf[port_no] = *conf;
        rc = VTSS_FUNC(rx_conf_set);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_PACKET_PORT_REG */

static vtss_rc vtss_packet_queue_check(const vtss_packet_rx_queue_t  queue_no)
{
    if (queue_no >= vtss_state->rx_queue_count) {
        VTSS_E("illegal queue_no: %u", queue_no);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_packet_rx_frame_get(const vtss_inst_t             inst,
                                 const vtss_packet_rx_queue_t  queue_no,
                                 vtss_packet_rx_header_t       *const header,
                                 u8                            *const frame,
                                 const u32                     length)
{
    vtss_rc rc;

    VTSS_N("queue_no: %u", queue_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_packet_queue_check(queue_no)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(rx_frame_get, queue_no, header, frame, length);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_packet_rx_frame_discard(const vtss_inst_t             inst,
                                     const vtss_packet_rx_queue_t  queue_no)
{
    vtss_rc rc;

    VTSS_D("queue_no: %u", queue_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_packet_queue_check(queue_no)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(rx_frame_discard, queue_no);
    }
    VTSS_EXIT();
    return rc;
}

static vtss_rc vtss_packet_tx_port(const vtss_port_no_t  port_no,
                                   const u8              *const frame,
                                   const u32             length)
{
    vtss_rc                rc;
    vtss_port_no_t         mirror_port = vtss_state->mirror_conf.port_no;
#if defined(VTSS_FEATURE_VSTAX)
    vtss_vstax_tx_header_t vstax;

    vstax.fwd_mode = VTSS_VSTAX_FWD_MODE_CPU_ALL;
    vstax.ttl = 1;
    vstax.prio = VTSS_PRIO_SUPER;
    vstax.tci.vid = VTSS_VID_NULL;
    vstax.tci.cfi = 0;
    vstax.tci.tagprio = 0;
    vstax.port_no = VTSS_PORT_NO_NONE;
    vstax.glag_no = VTSS_GLAG_NO_NONE;
    vstax.queue_no = VTSS_PACKET_RX_QUEUE_START;

    vtss_state->vstax_tx_header = &vstax;
#endif /* VTSS_FEATURE_VSTAX */

    VTSS_N("port_no: %u, length: %u", port_no, length);

    /* If egress mirroring is enabled, send on mirror port */
    if (vtss_state->mirror_egress[port_no] &&
        mirror_port != VTSS_PORT_NO_NONE && port_no != mirror_port &&
        vtss_state->port_state[mirror_port]) {
        if ((rc = VTSS_FUNC(tx_frame_port, mirror_port,
                            frame, length, VTSS_VID_NULL)) != VTSS_RC_OK)
            return rc;
    }
    return VTSS_FUNC(tx_frame_port, port_no, frame, length, VTSS_VID_NULL);
}

vtss_rc vtss_packet_tx_frame_port(const vtss_inst_t     inst,
                                  const vtss_port_no_t  port_no,
                                  const u8              *const frame,
                                  const u32             length)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = vtss_packet_tx_port(port_no, frame, length);
    VTSS_EXIT();
    return rc;
}

static vtss_rc vtss_packet_port_filter(vtss_state_t                  *state,
                                       const vtss_packet_port_info_t *const info,
                                       vtss_packet_port_filter_t     filter[VTSS_PORT_ARRAY_SIZE],
                                       BOOL tx_filter)
{
    vtss_port_no_t            port_rx, port_tx;
    vtss_vid_t                vid, uvid;
    vtss_aggr_no_t            aggr_no;
    vtss_packet_port_filter_t *port_filter;
    BOOL                      vlan_filter = 0, vlan_member[VTSS_PORT_ARRAY_SIZE];
    vtss_vlan_port_conf_t     *vlan_port_conf;    

    port_rx = info->port_no;
    vid = info->vid;
    if (vid != VTSS_VID_NULL) {
        vlan_filter = 1;
        VTSS_RC(vtss_cmn_vlan_members_get(state, vid, vlan_member));
    }

    for (port_tx = VTSS_PORT_NO_START; port_tx < state->port_count; port_tx++) {
        /* Discard by default */
        port_filter = &filter[port_tx];
        port_filter->filter = VTSS_PACKET_FILTER_DISCARD;

        /* Ingress port filtering */
        if (port_rx != VTSS_PORT_NO_NONE) {
            if (!state->rx_forward[port_rx]) {
                /* Rx forward filtering */
                VTSS_N("port_rx %u not ingress forwarding", port_rx);
                continue;
            }
        
            if (vlan_filter && vlan_member[port_rx] == 0 &&
                state->vlan_port_conf[port_rx].ingress_filter) {
                /* VLAN/MSTP/ERPS/.. ingress filtering */
                VTSS_N("port_rx %u not member of VLAN %u", port_rx, vid);
                continue;
            }

            /* If Tx filtering is not required, return result for the first port only */
            if (!tx_filter) {
                port_filter->filter = VTSS_PACKET_FILTER_UNTAGGED;
                return VTSS_RC_OK;
            }

            /* Ingress and egress port filtering (forwarding) */
            if (port_rx == port_tx) {
                /* Source port filtering */
                VTSS_N("port_rx %u and port_tx %u are identical", port_rx, port_tx);
                continue;
            }
            
            aggr_no = state->port_aggr_no[port_rx];
            if (aggr_no != VTSS_AGGR_NO_NONE && state->port_aggr_no[port_tx] == aggr_no) {
                /* Ingress LLAG filter */
                VTSS_N("port_rx %u and port_tx %u are members of same LLAG %u",
                       port_rx, port_tx, aggr_no);
                continue;
            }
        }
        
        /* Egress port filtering */
#if defined(VTSS_FEATURE_AGGR_GLAG)
        if (info->glag_no != VTSS_GLAG_NO_NONE &&
            info->glag_no == state->port_glag_no[port_tx]) {
            /* Ingress GLAG filter */
            VTSS_N("port_tx %u is member of GLAG %u", port_tx, info->glag_no);
            continue;
        }
#endif /* VTSS_FEATURE_AGGR_GLAG */
        
        if (vlan_filter && vlan_member[port_tx] == 0) { 
            /* VLAN/MSTP/ERPS/.. egress filtering */
            VTSS_N("port_tx %u not member of VLAN %u", port_tx, vid);
            continue;
        }
        
        if (!state->tx_forward[port_tx]) {
            /* Egress LAG/STP check */
            VTSS_N("port_tx: %u not LAG/STP forwarding", port_tx);
            continue;
        }
        
        /* Determine whether to send tagged or untagged */
        vlan_port_conf = &state->vlan_port_conf[port_tx];
        uvid = vlan_port_conf->untagged_vid;
        port_filter->filter = (uvid != VTSS_VID_ALL && uvid != vid ?
                               VTSS_PACKET_FILTER_TAGGED : VTSS_PACKET_FILTER_UNTAGGED);

        /* Determine tag type */
        port_filter->tpid = VTSS_ETYPE_TAG_C;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        switch (vlan_port_conf->port_type) {
        case VTSS_VLAN_PORT_TYPE_S:
            port_filter->tpid = VTSS_ETYPE_TAG_S;
            break;
        case VTSS_VLAN_PORT_TYPE_S_CUSTOM:
            port_filter->tpid = state->vlan_conf.s_etype;
            break;
        default:
            break;
        }
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */
    } /* Port loop */
    return VTSS_RC_OK;
}

static vtss_rc vtss_packet_filter(vtss_state_t                    *state,
                                  const vtss_packet_frame_info_t  *const info,
                                  vtss_packet_filter_t            *const filter)
{
    vtss_packet_port_info_t   port_info;
    vtss_packet_port_filter_t port_filter[VTSS_PORT_ARRAY_SIZE];
    BOOL                      tx_filter = (info->port_tx == VTSS_PORT_NO_NONE ? 0 : 1);
    
    /* Copy fields to port filter */
    port_info.port_no = info->port_no;
#if defined(VTSS_FEATURE_AGGR_GLAG)
    port_info.glag_no = info->glag_no;
#endif /* VTSS_FEATURE_AGGR_GLAG */
    port_info.vid = info->vid;
    
    /* Use port filter function */
    VTSS_RC(vtss_packet_port_filter(state, &port_info, port_filter, tx_filter));
    *filter = port_filter[tx_filter ? info->port_tx : 0].filter;
    return VTSS_RC_OK;
}

static vtss_rc vtss_packet_tx_port_vlan(const vtss_port_no_t  port_no,
                                        const vtss_vid_t      vid,
                                        const u8              *const frame,
                                        const u32             length)
{
    vtss_rc                  rc;
    vtss_port_no_t           port;
    vtss_mac_table_entry_t   mac_entry;
    u32                      i;
    BOOL                     mac_found = 0, mirror = 0, mirror_done = 0;
    vtss_packet_frame_info_t info;
    vtss_packet_filter_t     filter;
    vtss_vid_t               uvid, tx_vid;
#if defined(VTSS_FEATURE_VSTAX)
    vtss_vstax_tx_header_t   vstax;

    vstax.fwd_mode = VTSS_VSTAX_FWD_MODE_LOOKUP;
    vstax.ttl = VTSS_VSTAX_TTL_PORT;
    vstax.prio = VTSS_PRIO_START;
    vstax.tci.vid = vid;
    vstax.tci.cfi = 0;
    vstax.tci.tagprio = 0;
    vtss_state->vstax_tx_header = &vstax;
#endif /* VTSS_FEATURE_VSTAX */

    VTSS_N("port_no: %u, vid: %u, length: %u", port_no, vid, length);

    VTSS_RC(vtss_vid_check(vid));

    if (port_no == VTSS_PORT_NO_NONE) {
        /* Lookup (VID, DMAC) */
        mac_entry.vid_mac.vid = vid;
        for (i = 0; i < 6; i++)
            mac_entry.vid_mac.mac.addr[i] = frame[i];
        mac_found = (vtss_mac_get(&mac_entry.vid_mac, &mac_entry) == VTSS_RC_OK);
    }

    /* Filter on VLAN, no ingress filtering */
    info.port_no = VTSS_PORT_NO_NONE;
#if defined(VTSS_FEATURE_AGGR_GLAG)
    info.glag_no = VTSS_GLAG_NO_NONE;
#endif /* VTSS_FEATURE_AGGR_GLAG */
    info.vid = vid;

    /* Transmit on ports */
    for (port = VTSS_PORT_NO_START; port < vtss_state->port_count; port++) {

        /* Port check */
        if (port_no != VTSS_PORT_NO_NONE && port != port_no) {
            VTSS_N("skip port: %u", port);
            continue;
        }

        /* Destination port check */
        if (mac_found && mac_entry.destination[port] == 0) {
            VTSS_N("port: %u not a destination member", port);
            continue;
        }

        /* Egress filtering */
        info.port_tx = port;
        if (vtss_packet_filter(vtss_state, &info, &filter) != VTSS_RC_OK ||
            filter == VTSS_PACKET_FILTER_DISCARD) {
            VTSS_N("port: %u filtered", port);
            continue;
        }

        /* All checks successful, transmit on port */
        VTSS_N("tx on port: %u", port);
        tx_vid = (filter == VTSS_PACKET_FILTER_TAGGED ? vid : VTSS_VID_NULL);
        if ((rc = VTSS_FUNC(tx_frame_port, port, frame, length, tx_vid)) != VTSS_RC_OK)
            return rc;

        /* Check if egress mirroring must be done */
        if (vtss_state->mirror_egress[port])
            mirror = 1;

        /* Check if egress mirroring has been done */
        if (port == vtss_state->mirror_conf.port_no)
            mirror_done = 1;
    }

    port = vtss_state->mirror_conf.port_no;
    if (mirror && !mirror_done && port != VTSS_PORT_NO_NONE &&
        vtss_state->stp_state[port] == VTSS_STP_STATE_FORWARDING) {
        uvid = vtss_state->vlan_port_conf[port_no].untagged_vid;
        tx_vid = (uvid != VTSS_VID_ALL && uvid != vid ? vid : VTSS_VID_NULL);
        if ((rc = VTSS_FUNC(tx_frame_port, port, frame, length, tx_vid)) != VTSS_RC_OK)
            return rc;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_packet_tx_frame_port_vlan(const vtss_inst_t     inst,
                                       const vtss_port_no_t  port_no,
                                       const vtss_vid_t      vid,
                                       const u8              *const frame,
                                       const u32             length)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = vtss_packet_tx_port_vlan(port_no, vid, frame, length);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_packet_tx_frame_vlan(const vtss_inst_t  inst,
                                  const vtss_vid_t   vid,
                                  const u8           *const frame,
                                  const u32          length)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_packet_tx_port_vlan(VTSS_PORT_NO_NONE, vid, frame, length);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_packet_frame_filter(const vtss_inst_t               inst,
                                 const vtss_packet_frame_info_t  *const info,
                                 vtss_packet_filter_t            *const filter)
{
    vtss_rc      rc;
    vtss_state_t *state;
    
    if ((rc = vtss_inst_check_get(inst, &state)) == VTSS_RC_OK)
        rc = vtss_packet_filter(state, info, filter);
    return rc;
}

vtss_rc vtss_packet_port_info_init(vtss_packet_port_info_t *const info)
{
    memset(info, 0, sizeof(*info));
    info->port_no = VTSS_PORT_NO_NONE;
#if defined(VTSS_FEATURE_AGGR_GLAG)
    info->glag_no = VTSS_GLAG_NO_NONE;
#endif /* VTSS_FEATURE_AGGR_GLAG */
    return VTSS_RC_OK;
}

vtss_rc vtss_packet_port_filter_get(const vtss_inst_t             inst,
                                    const vtss_packet_port_info_t *const info,
                                    vtss_packet_port_filter_t     filter[VTSS_PORT_ARRAY_SIZE])
{
    vtss_rc      rc;
    vtss_state_t *state;

    if ((rc = vtss_inst_check_get(inst, &state)) == VTSS_RC_OK)
        rc = vtss_packet_port_filter(state, info, filter, 1);
    return rc;
}

#if defined(VTSS_FEATURE_VSTAX)
vtss_rc vtss_packet_tx_frame_vstax(const vtss_inst_t             inst,
                                   const vtss_port_no_t          port_no,
                                   const vtss_vstax_tx_header_t  *const header,
                                   const u8                      *const frame,
                                   const u32                     length)
{
    vtss_rc rc;

    VTSS_N("port_no: %u, length: %u", port_no, length);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->vstax_tx_header = header;
        rc = VTSS_FUNC(tx_frame_port, port_no, frame, length, VTSS_VID_NULL);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_packet_vstax_header2frame(const vtss_inst_t             inst,
                                       const vtss_port_no_t          port_no,
                                       const vtss_vstax_tx_header_t  *const header,
                                       u8                            *const frame)
{
    // This function doesn't require vtss_state be set up correctly,
    // and if running stackable and doing a lengthy API function
    // (e.g. a debug print function), the stack will fall apart unless
    // this function executes without waiting for the API semaphore.
    return VTSS_FUNC_FROM_STATE(vtss_inst_check_no_persist(inst), vstax_header2frame, port_no, header, frame);
}

vtss_rc vtss_packet_vstax_frame2header(const vtss_inst_t       inst,
                                       const u8                *const frame,
                                       vtss_vstax_rx_header_t  *const header)
{
    // This function doesn't require vtss_state be set up correctly,
    // and if running stackable and doing a lengthy API function
    // (e.g. a debug print function), the stack will fall apart unless
    // this function executes without waiting for the API semaphore.
    return VTSS_FUNC_FROM_STATE(vtss_inst_check_no_persist(inst), vstax_frame2header, frame, header);
}
#endif /* VTSS_FEATURE_VSTAX */

/*
 * Decode an extraction header.
 */
vtss_rc vtss_packet_rx_hdr_decode(const vtss_inst_t                  inst,
                                  const vtss_packet_rx_meta_t *const meta,
                                        vtss_packet_rx_info_t *const info)
{
    if (meta == NULL || info == NULL || meta->bin_hdr == NULL) {
        return VTSS_RC_ERROR;
    }

    // This function executes without locking the API semaphore.
    return VTSS_FUNC_FROM_STATE(vtss_inst_check_no_persist(inst), rx_hdr_decode, meta, info);
}

/*
 * Encode an injection header.
 */
vtss_rc vtss_packet_tx_hdr_encode(const vtss_inst_t                  inst,
                                  const vtss_packet_tx_info_t *const info,
                                        u8                    *const bin_hdr,
                                        u32                   *const bin_hdr_len)
{
    if (info == NULL || bin_hdr_len == NULL) {
        return VTSS_RC_ERROR;
    }

    // This function executes without locking the API semaphore.
    // The only parameter it uses from the state variable (#inst) is
    // the port map, which is assumed to be constant once booted.
    return VTSS_FUNC_FROM_STATE(vtss_inst_check_no_persist(inst), tx_hdr_encode, info, bin_hdr, bin_hdr_len);
}

/*
 * Initialize a packet_tx_info_t structure.
 */
vtss_rc vtss_packet_tx_info_init(const vtss_inst_t                  inst,
                                       vtss_packet_tx_info_t *const info)
{
    if (info == NULL) {
        return VTSS_RC_ERROR;
    }

    memset(info, 0, sizeof(*info));
    info->masquerade_port = VTSS_PORT_NO_NONE; // Default to not masquerading.
#if VTSS_ISDX_NONE != 0
    info->isdx            = VTSS_ISDX_NONE;    // Default to not injecting on an ISDX.
#endif
#if defined(VTSS_FEATURE_AFI_SWC) && !VTSS_OPT_FDMA && VTSS_AFI_ID_NONE != 0
    // If using the FDMA, it will always overwrite this one, so no need to spend CPU cycles on setting it.
    info->afi_id          = VTSS_AFI_ID_NONE;  // Default to not repeating the frame.
#endif
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_PACKET */

#if defined(VTSS_FEATURE_AFI_SWC)
/*
 * Allocate an ID for a frame subject to periodical injection.
 */
vtss_rc vtss_afi_alloc(const vtss_inst_t inst, const vtss_afi_frm_dscr_t *const dscr, vtss_afi_id_t *const id)
{
    vtss_rc rc;

    if (dscr == NULL || id == NULL) {
        return VTSS_RC_ERROR;
    }

    if (dscr->fps <= 0 || dscr->fps > VTSS_AFI_FPS_MAX) {
        return VTSS_RC_ERROR;
    }

    VTSS_ENTER();
    rc = VTSS_FUNC(afi_alloc, dscr, id);
    VTSS_EXIT();

    return rc;
}

/*
 * Cancel and free a periodically injected frame.
 */
vtss_rc vtss_afi_free(const vtss_inst_t inst, vtss_afi_id_t id)
{
    vtss_rc rc;

    VTSS_ENTER();
    rc = VTSS_FUNC(afi_free, id);
    VTSS_EXIT();

    return rc;
}
#endif /* defined(VTSS_FEATURE_AFI_SWC) */

#if defined(VTSS_FEATURE_NPI)
vtss_rc vtss_npi_conf_get(const vtss_inst_t inst, vtss_npi_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *conf = vtss_state->npi_conf;
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_npi_conf_set(const vtss_inst_t inst, const vtss_npi_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, conf->enable ? conf->port_no : VTSS_PORT_NO_START)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(npi_conf_set, conf);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_NPI */

#if defined(VTSS_FEATURE_LAYER2)
/* - Port Based Network Access Control, 802.1X --------------------- */

vtss_rc vtss_auth_port_state_get(const vtss_inst_t     inst,
                                 const vtss_port_no_t  port_no,
                                 vtss_auth_state_t     *const state)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *state = vtss_state->auth_state[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_auth_port_state_set(const vtss_inst_t        inst,
                                 const vtss_port_no_t     port_no,
                                 const vtss_auth_state_t  state)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->auth_state[port_no] = state;
        rc = vtss_update_masks(1, 0, 1);
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_ACL)
/* - Access Control Lists ------------------------------------------ */

static vtss_rc vtss_acl_policer_no_check(const vtss_acl_policer_no_t  policer_no)
{
    if (policer_no >= VTSS_ACL_POLICERS) {
        VTSS_E("illegal policer_no: %u", policer_no);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_acl_policer_conf_get(const vtss_inst_t            inst,
                                  const vtss_acl_policer_no_t  policer_no,
                                  vtss_acl_policer_conf_t      *const conf)
{
    vtss_rc rc;

    VTSS_D("policer_no: %u", policer_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_acl_policer_no_check(policer_no)) == VTSS_RC_OK)
        *conf = vtss_state->acl_policer_conf[policer_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_acl_policer_conf_set(const vtss_inst_t              inst,
                                  const vtss_acl_policer_no_t    policer_no,
                                  const vtss_acl_policer_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("policer_no: %u", policer_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK &&
        (rc = vtss_acl_policer_no_check(policer_no)) == VTSS_RC_OK) {
        vtss_state->acl_policer_conf[policer_no] = *conf;
        rc = VTSS_FUNC_COLD(acl_policer_set, policer_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_acl_port_conf_get(const vtss_inst_t     inst,
                               const vtss_port_no_t  port_no,
                               vtss_acl_port_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->acl_port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_acl_port_conf_set(const vtss_inst_t           inst,
                               const vtss_port_no_t        port_no,
                               const vtss_acl_port_conf_t  *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->acl_old_port_conf = vtss_state->acl_port_conf[port_no];
        vtss_state->acl_port_conf[port_no] = *conf;
        rc = VTSS_FUNC_COLD(acl_port_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_acl_port_counter_get(const vtss_inst_t        inst,
                                  const vtss_port_no_t     port_no,
                                  vtss_acl_port_counter_t  *const counter)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(acl_port_counter_get, port_no, counter);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_acl_port_counter_clear(const vtss_inst_t     inst,
                                    const vtss_port_no_t  port_no)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        rc = VTSS_FUNC(acl_port_counter_clear, port_no);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ace_init(const vtss_inst_t      inst,
                      const vtss_ace_type_t  type,
                      vtss_ace_t             *const ace)
{
    VTSS_D("type: %d", type);

    memset(ace, 0, sizeof(*ace));
    ace->type = type;
#if defined(VTSS_FEATURE_ACL_V1)
    ace->action.forward = 1;
#endif /* VTSS_FEATURE_ACL_V1 */
    ace->action.learn = 1;
    ace->dmac_bc = VTSS_ACE_BIT_ANY;
    ace->dmac_mc = VTSS_ACE_BIT_ANY;
    ace->vlan.cfi = VTSS_ACE_BIT_ANY;

    switch (type) {
    case VTSS_ACE_TYPE_ANY:
    case VTSS_ACE_TYPE_ETYPE:
    case VTSS_ACE_TYPE_LLC:
    case VTSS_ACE_TYPE_SNAP:
    case VTSS_ACE_TYPE_IPV6:
        break;
    case VTSS_ACE_TYPE_ARP:
        ace->frame.arp.arp = VTSS_ACE_BIT_ANY;
        ace->frame.arp.req = VTSS_ACE_BIT_ANY;
        ace->frame.arp.unknown = VTSS_ACE_BIT_ANY;
        ace->frame.arp.smac_match = VTSS_ACE_BIT_ANY;
        ace->frame.arp.dmac_match = VTSS_ACE_BIT_ANY;
        ace->frame.arp.length = VTSS_ACE_BIT_ANY;
        ace->frame.arp.ip = VTSS_ACE_BIT_ANY;
        ace->frame.arp.ethernet = VTSS_ACE_BIT_ANY;
        break;
    case VTSS_ACE_TYPE_IPV4:
        ace->frame.ipv4.ttl = VTSS_ACE_BIT_ANY;
        ace->frame.ipv4.fragment = VTSS_ACE_BIT_ANY;
        ace->frame.ipv4.options = VTSS_ACE_BIT_ANY;
        ace->frame.ipv4.sport.high = 0xffff;
        ace->frame.ipv4.sport.in_range = 1;
        ace->frame.ipv4.dport.high = 0xffff;
        ace->frame.ipv4.dport.in_range = 1;
        ace->frame.ipv4.tcp_fin = VTSS_ACE_BIT_ANY;
        ace->frame.ipv4.tcp_syn = VTSS_ACE_BIT_ANY;
        ace->frame.ipv4.tcp_rst = VTSS_ACE_BIT_ANY;
        ace->frame.ipv4.tcp_psh = VTSS_ACE_BIT_ANY;
        ace->frame.ipv4.tcp_ack = VTSS_ACE_BIT_ANY;
        ace->frame.ipv4.tcp_urg = VTSS_ACE_BIT_ANY;
        break;
    default:
        VTSS_E("unknown type: %d", type);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

vtss_rc vtss_ace_add(const vtss_inst_t    inst,
                     const vtss_ace_id_t  ace_id,
                     const vtss_ace_t     *const ace)
{
    vtss_rc rc;

    VTSS_D("ace_id: %u before %u %s",
           ace->id, ace_id, ace_id == VTSS_ACE_ID_LAST ? "(last)" : "");

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(acl_ace_add, ace_id, ace);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ace_del(const vtss_inst_t    inst,
                     const vtss_ace_id_t  ace_id)
{
    vtss_rc rc;

    VTSS_D("ace_id: %u", ace_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(acl_ace_del, ace_id);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ace_counter_get(const vtss_inst_t    inst,
                             const vtss_ace_id_t  ace_id,
                             vtss_ace_counter_t   *const counter)
{
    vtss_rc rc;

    VTSS_D("ace_id: %u", ace_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(acl_ace_counter_get, ace_id, counter);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_ace_counter_clear(const vtss_inst_t    inst,
                               const vtss_ace_id_t  ace_id)
{
    vtss_rc rc;

    VTSS_D("ace_id: %u", ace_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(acl_ace_counter_clear, ace_id);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_ACL */


#if defined(VTSS_FEATURE_INTERRUPTS)
vtss_rc vtss_intr_cfg(const vtss_inst_t       inst,
                      const u32               mask,
                      const BOOL              polarity,
                      const BOOL              enable) {
    vtss_rc rc;
    VTSS_ENTER();
    rc = VTSS_FUNC(intr_cfg, mask, polarity, enable);
    VTSS_EXIT();
    return rc;

}
                      
vtss_rc vtss_intr_mask_set(const vtss_inst_t       inst,
                           vtss_intr_t *mask) {

    vtss_rc rc;
    VTSS_ENTER();
    rc = VTSS_FUNC(intr_mask_set,mask);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_intr_status_get(const vtss_inst_t       inst,
                             vtss_intr_t *status)
{
    vtss_rc rc;
    VTSS_ENTER();
    rc = VTSS_FUNC(intr_status_get, status);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_intr_pol_negation(const vtss_inst_t   inst)
{
    vtss_rc rc;
    VTSS_ENTER();
    rc = VTSS_FUNC(intr_pol_negation);
    VTSS_EXIT();
    return rc;
}

#endif /* VTSS_FEATURE_INTERRUPTS  */

/* - API tod functions -------------------------------------- */

/*
 * If an external ns read function must be called, this variable holds a pointer
 * to the function, otherwise it is NULL.
 */
static tod_get_ns_cnt_cb_t hw_get_ns_callout = 0;

void vtss_tod_set_ns_cnt_cb(tod_get_ns_cnt_cb_t cb) 
{
    hw_get_ns_callout = cb;
}

/**
 * \brief Get the current hw nanosec time
 *  Because this function can be called from interrupt and/or with interrupt disabled,
 *  the normal VTSS_ENTER macro is not used, neither is the VTSS_FUNC used, because it copies
 *  an instance pointer to a global variable.
 *  Therefore the CIL layer function is called via the default_inst pointer.
 *
 * \returns actual ns counter
 */
u32 vtss_tod_get_ns_cnt(void) 
{
    if (hw_get_ns_callout) {
        return hw_get_ns_callout();
#if defined(VTSS_FEATURE_TIMESTAMP)
    } else if (vtss_default_inst->cil_func.ts_ns_cnt_get){
        return vtss_default_inst->cil_func.ts_ns_cnt_get(vtss_default_inst);
#endif
    } else {
        return 0; /* currently no HW support */
    }
}

/* - API ts (time stamping) functions -------------------------------------- */

#if defined(VTSS_FEATURE_TIMESTAMP)
/* Get the current time in a Timestamp format, and the corresponding time counter */
vtss_rc vtss_ts_timeofday_get(const vtss_inst_t             inst,
                              vtss_timestamp_t                     * const ts,
                              u32                           * const tc)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(ts_timeofday_get,ts,tc);
    }
    VTSS_EXIT();
    return rc;
}

/* Get the time at the next 1PPS pulse edge in a Timestamp format. */
vtss_rc vtss_ts_timeofday_next_pps_get(const vtss_inst_t             inst,
                                       vtss_timestamp_t                     *const ts)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(ts_timeofday_next_pps_get,ts);
    }
    VTSS_EXIT();
    return rc;
}

/* Set the current time in a Timestamp format */
vtss_rc vtss_ts_timeofday_set(const vtss_inst_t             inst,
                              const vtss_timestamp_t               *ts)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ts_timeofday_set,ts);
    }
    VTSS_EXIT();
    return rc;
}

/* Set delta from the current time in a Timestamp format */
vtss_rc vtss_ts_timeofday_set_delta(const vtss_inst_t       inst,
                              const vtss_timestamp_t         *ts,
                              BOOL                      negative)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ts_timeofday_set_delta,ts, negative);
    }
    VTSS_EXIT();
    return rc;
}

/* Subtract offset from the current time */
vtss_rc vtss_ts_timeofday_offset_set(const vtss_inst_t          inst,
                                     const i32                  offset)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ts_timeofday_offset_set,offset);
    }
    VTSS_EXIT();
    return rc;
}



/* Do the one sec administration in the Timestamp function */
vtss_rc vtss_ts_adjtimer_one_sec(const vtss_inst_t             inst,
                                 BOOL                           *const ongoing_adjustment)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *ongoing_adjustment = vtss_state->ts_conf.awaiting_adjustment;
        rc = VTSS_FUNC_COLD(ts_timeofday_one_sec);
    }
    VTSS_EXIT();
    return rc;
}

/* check if clock adjustment is ongoing */
vtss_rc vtss_ts_ongoing_adjustment(const vtss_inst_t           inst,
                                   BOOL                        *const ongoing_adjustment)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *ongoing_adjustment = vtss_state->ts_conf.awaiting_adjustment;
    }
    VTSS_EXIT();
    return rc;
}

/* Adjust the clock timer ratio */
vtss_rc vtss_ts_adjtimer_set(const vtss_inst_t              inst,
                             const i32                      adj)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->ts_conf.adj = adj;
        rc = VTSS_FUNC_COLD(ts_adjtimer_set);
    }
    VTSS_EXIT();
    return rc;
}

/* Get Adjust the clock timer ratio */
vtss_rc vtss_ts_adjtimer_get(const vtss_inst_t              inst,
                             i32                            *const adj)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *adj = vtss_state->ts_conf.adj;
    }
    VTSS_EXIT();
    return rc;
}


/* Get get the clock internal timer frequency offset, compared to external clock input. */
vtss_rc vtss_ts_freq_offset_get(const vtss_inst_t              inst,
                                 i32                            *const adj)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ts_freq_offset_get, adj);
    }
    VTSS_EXIT();
    return rc;
}


/* Set the external clock mode. */
vtss_rc vtss_ts_external_clock_mode_set(
                                const vtss_inst_t           inst,
                                const vtss_ts_ext_clock_mode_t   *const ext_clock_mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->ts_conf.ext_clock_mode = *ext_clock_mode;
#if defined(VTSS_ARCH_SERVAL)
        rc = VTSS_FUNC_COLD(ts_external_clock_mode_set,1);
#else
        rc = VTSS_FUNC_COLD(ts_external_clock_mode_set);
#endif
    }
    VTSS_EXIT();
    return rc;
}

#if defined(VTSS_ARCH_SERVAL)
vtss_rc vtss_ts_alt_clock_saved_get(
    const vtss_inst_t           inst,
    u32    *const               saved)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ts_alt_clock_saved_get, saved);
    }
    VTSS_EXIT();
    return rc;
}
                                       
/* Get the alternative external clock mode. */
vtss_rc vtss_ts_alt_clock_mode_get(
    const vtss_inst_t              inst,
    vtss_ts_alt_clock_mode_t       *const alt_clock_mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *alt_clock_mode = vtss_state->ts_conf.alt_clock_mode;
    }
    VTSS_EXIT();
    return rc;
}
                                       
/* Set the alternative external clock mode. */
vtss_rc vtss_ts_alt_clock_mode_set(
    const vtss_inst_t              inst,
    const vtss_ts_alt_clock_mode_t *const alt_clock_mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->ts_conf.alt_clock_mode = *alt_clock_mode;
        rc = VTSS_FUNC_COLD(ts_alt_clock_mode_set);
    }
    VTSS_EXIT();
    return rc;
}
                                       
/* Set the time at the next 1PPS pulse edge in a Timestamp format. */
vtss_rc vtss_ts_timeofday_next_pps_set(const vtss_inst_t       inst,
                                       const vtss_timestamp_t         *const ts)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC_COLD(ts_timeofday_next_pps_set, ts);
    }
    VTSS_EXIT();
    return rc;
}

#endif /* (VTSS_ARCH_SERVAL) */

/* Get the external clock mode. */
vtss_rc vtss_ts_external_clock_mode_get(
                                const vtss_inst_t           inst,
                                vtss_ts_ext_clock_mode_t         *const ext_clock_mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *ext_clock_mode = vtss_state->ts_conf.ext_clock_mode;
    }
    VTSS_EXIT();
    return rc;
}

/* Set the ingress latency */
vtss_rc vtss_ts_ingress_latency_set(const vtss_inst_t              inst,
                                    const vtss_port_no_t           port_no,
                                    const vtss_timeinterval_t             *const ingress_latency)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->ts_port_conf[port_no].ingress_latency = *ingress_latency;
        rc = VTSS_FUNC_COLD(ts_ingress_latency_set,port_no);
    }
    VTSS_EXIT();
    return rc;
}

/* Get the ingress latency */
vtss_rc vtss_ts_ingress_latency_get(const vtss_inst_t              inst,
                                    const vtss_port_no_t           port_no,
                                    vtss_timeinterval_t                   *const ingress_latency)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        *ingress_latency = vtss_state->ts_port_conf[port_no].ingress_latency;
    }
    VTSS_EXIT();
    return rc;
}

/* Set the P2P delay */
vtss_rc vtss_ts_p2p_delay_set(const vtss_inst_t              inst,
                              const vtss_port_no_t           port_no,
                              const vtss_timeinterval_t             *const p2p_delay)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->ts_port_conf[port_no].p2p_delay = *p2p_delay;
        rc = VTSS_FUNC_COLD(ts_p2p_delay_set,port_no);
    }
    VTSS_EXIT();
    return rc;
}

/* Get the P2P delay */
vtss_rc vtss_ts_p2p_delay_get(const vtss_inst_t              inst,
                              const vtss_port_no_t           port_no,
                              vtss_timeinterval_t                   *const p2p_delay)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        *p2p_delay = vtss_state->ts_port_conf[port_no].p2p_delay;
    }
    VTSS_EXIT();
    return rc;
}

/* Set the egress latency */
vtss_rc vtss_ts_egress_latency_set(const vtss_inst_t              inst,
                                   const vtss_port_no_t           port_no,
                                   const vtss_timeinterval_t             *const egress_latency)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->ts_port_conf[port_no].egress_latency = *egress_latency;
        rc = VTSS_FUNC_COLD(ts_egress_latency_set,port_no);
    }
    VTSS_EXIT();
    return rc;
}

/* Get the egress latency */
vtss_rc vtss_ts_egress_latency_get(const vtss_inst_t        inst,
                                   const vtss_port_no_t     port_no,
                                   vtss_timeinterval_t             *const egress_latency)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        *egress_latency = vtss_state->ts_port_conf[port_no].egress_latency;
    }
    VTSS_EXIT();
    return rc;
}

/* Set the delay asymmetry */
vtss_rc vtss_ts_delay_asymmetry_set(const vtss_inst_t           inst,
                                   const vtss_port_no_t         port_no,
                                   const vtss_timeinterval_t    *const delay_asymmetry)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->ts_port_conf[port_no].delay_asymmetry = *delay_asymmetry;
        rc = VTSS_FUNC_COLD(ts_delay_asymmetry_set,port_no);
    }
    VTSS_EXIT();
    return rc;
}

/* Get the delay asymmetry */
vtss_rc vtss_ts_delay_asymmetry_get(const vtss_inst_t           inst,
                                   const vtss_port_no_t         port_no,
                                   vtss_timeinterval_t          *const delay_asymmetry)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        *delay_asymmetry = vtss_state->ts_port_conf[port_no].delay_asymmetry;
    }
    VTSS_EXIT();
    return rc;
}

/* Set the timestamping operation mode for a port */
vtss_rc vtss_ts_operation_mode_set(const vtss_inst_t              inst,
                                   const vtss_port_no_t           port_no,
                                   const vtss_ts_operation_mode_t      *const mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->ts_port_conf[port_no].mode = *mode;
        rc = VTSS_FUNC_COLD(ts_operation_mode_set,port_no);
    }
    VTSS_EXIT();
    return rc;
}

/* Get the timestamping operation mode for a port */
vtss_rc vtss_ts_operation_mode_get(const vtss_inst_t              inst,
                                   const vtss_port_no_t           port_no,
                                   vtss_ts_operation_mode_t            *const mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        *mode = vtss_state->ts_port_conf[port_no].mode;
    }
    VTSS_EXIT();
    return rc;
}

/* Set the timestamping internal mode */
vtss_rc vtss_ts_internal_mode_set(const vtss_inst_t              inst,
                                   const vtss_ts_internal_mode_t      *const mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->ts_int_mode = *mode;
        rc = VTSS_FUNC_COLD(ts_internal_mode_set);
    }
    VTSS_EXIT();
    return rc;
}

/* Get the timestamping internal mode */
vtss_rc vtss_ts_internal_mode_get(const vtss_inst_t              inst,
                                   vtss_ts_internal_mode_t            *const mode)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *mode = vtss_state->ts_int_mode;
    }
    VTSS_EXIT();
    return rc;
}

/* Flush the timestamp FIFO  */
static void vtss_timestamp_flush(void)
{
    vtss_rc rc;
    int id;
    rc = VTSS_FUNC(ts_timestamp_get);
    VTSS_D("Flushing timestamp fifo");
    for (id = 0; id < VTSS_TS_ID_SIZE; id++) {
        vtss_state->ts_status[id].reserved_mask = 0LL;
        vtss_state->ts_status[id].valid_mask = 0LL;
        vtss_state->ts_status[id].rx_tc_valid = FALSE;
        vtss_state->ts_status[id].age = 0;
        rc = VTSS_FUNC(ts_timestamp_id_release, id);
    }
}
                                   

/* Update the internal timestamp table, from HW */
vtss_rc vtss_tx_timestamp_update(const vtss_inst_t              inst)
{
    vtss_rc rc;
    u64 port_mask;
    vtss_ts_timestamp_t ts;
    vtss_state_t *my_vtss_state;
    int port_idx = 0;
    int ts_idx = 0;
    void (*cb)(void *context, u32 port_no, vtss_ts_timestamp_t *ts);
    void *cx;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(ts_timestamp_get);
        VTSS_D("rc = %d", rc);
        while (port_idx < VTSS_PORT_ARRAY_SIZE && 
               ts_idx < TS_IDS_RESERVED_FOR_SW) {
            port_mask = 1LL<<port_idx;
            if ((vtss_state->ts_status[ts_idx].valid_mask & port_mask) && 
                (vtss_state->ts_status[ts_idx].reserved_mask & port_mask)) {
                vtss_state->ts_status[ts_idx].valid_mask &= ~port_mask;
                vtss_state->ts_status[ts_idx].reserved_mask &= ~port_mask;
                ts.id = vtss_state->ts_status[ts_idx].tx_id[port_idx];
                ts.ts = vtss_state->ts_status[ts_idx].tx_tc[port_idx];
                ts.ts_valid = TRUE;
                if (vtss_state->ts_status[ts_idx].cb[port_idx] && vtss_state->ts_status[ts_idx].context[port_idx]) {
                    my_vtss_state = vtss_state; /* save context */
                    /* avoid using vtss_state while outside the API lock, as the API may be called from an other thread */
                    cb = vtss_state->ts_status[ts_idx].cb[port_idx];
                    cx = vtss_state->ts_status[ts_idx].context[port_idx];
                    VTSS_EXIT();
                    /* call out of the API */
                    cb(cx, port_idx, &ts);
                    VTSS_ENTER();
                    vtss_state = my_vtss_state; /* restore context */
                    vtss_state->ts_status[ts_idx].cb[port_idx] = NULL;
                    vtss_state->ts_status[ts_idx].context[port_idx] = NULL;
                } else {
                    VTSS_E("undefined TS callback port_idx %d, ts_idx %d", port_idx, ts_idx);
                }
                VTSS_D("port_no %d, ts_id %d, ts %d(%d)", port_idx, ts.id, ts.ts, ts.ts_valid);
            }
            if (++port_idx >= VTSS_PORT_ARRAY_SIZE) {
                port_idx = 0;
                ++ts_idx;
            }
        }
    }
    VTSS_EXIT();
    return rc;
}

/* Get the FIFO rx timestamp for a {timestampId} */
vtss_rc vtss_rx_timestamp_get(const vtss_inst_t              inst,
                              const vtss_ts_id_t             *const ts_id,
                              vtss_ts_timestamp_t                 *const ts)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(ts_timestamp_get);
        if (ts_id->ts_id >= VTSS_TS_ID_SIZE) {
            /* invalid timestamp id indicates FIFO overflow */
            vtss_timestamp_flush();
            rc = VTSS_RC_ERROR;
        } else {
            ts->ts = vtss_state->ts_status[ts_id->ts_id].rx_tc;
            ts->ts_valid = vtss_state->ts_status[ts_id->ts_id].rx_tc_valid;
            if (ts->ts_valid) {
                vtss_state->ts_status[ts_id->ts_id].rx_tc_valid = FALSE;
                if (vtss_state->ts_status[ts_id->ts_id].reserved_mask == 0LL) {
                    vtss_state->ts_status[ts_id->ts_id].age = 0;
                    rc = VTSS_FUNC(ts_timestamp_id_release, ts_id->ts_id);
                }
            }
        }
    }
    VTSS_EXIT();
    return rc;
}

#if defined (VTSS_ARCH_SERVAL_CE)
/* brief Get oam timestamp */
vtss_rc vtss_oam_timestamp_get(const vtss_inst_t             inst,
                               const vtss_oam_ts_id_t        *const id,
                               vtss_oam_ts_timestamp_t       *const ts)
{
    vtss_rc rc;
    i32 idx;
    u32 i;
    VTSS_ENTER();

    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (id->voe_id >= VTSS_VOE_ID_SIZE) {
            /* invalid timestamp id indicates FIFO overflow */
            rc = VTSS_RC_ERROR;
        } else {
            idx = vtss_state->oam_ts_status [id->voe_id].last;
            for (i = 0; i < VTSS_SERVAL_MAX_OAM_ENTRIES; i++) {
                if (vtss_state->oam_ts_status [id->voe_id].entry[idx].sq == id->voe_sq) {
                    /* an entry is found */
                    ts->ts = vtss_state->oam_ts_status [id->voe_id].entry[idx].tc;
                    ts->port_no = vtss_state->oam_ts_status [id->voe_id].entry[idx].port;
                    ts->ts_valid = vtss_state->oam_ts_status [id->voe_id].entry[idx].valid;
                    vtss_state->oam_ts_status [id->voe_id].entry[idx].valid = FALSE;
                    break;
                }
                if (--idx < 0) idx = VTSS_SERVAL_MAX_OAM_ENTRIES-1;
            }
            if (i == VTSS_SERVAL_MAX_OAM_ENTRIES) {
                /* no entries found */
                ts->ts = 0;
                ts->port_no = 0;
                ts->ts_valid = FALSE;
            }
        }
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_SERVAL_CE */



/* Release the FIFO rx timestamp id  */
vtss_rc vtss_rx_timestamp_id_release(const vtss_inst_t              inst,
                              const vtss_ts_id_t             *const ts_id)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (ts_id->ts_id >= VTSS_TS_ID_SIZE) {
            /* invalid timestamp id indicates FIFO overflow */
            vtss_timestamp_flush();
            rc = VTSS_RC_ERROR;
        } else {
            rc = VTSS_FUNC(ts_timestamp_id_release, ts_id->ts_id);
        }
    }
    VTSS_EXIT();
    return rc;
}

/* Get rx timestamp from a port (convert from slave time to the master time) */
vtss_rc vtss_rx_master_timestamp_get(const vtss_inst_t              inst,
                                     const vtss_port_no_t           port_no,
                                     vtss_ts_timestamp_t                 *const ts)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(ts_timestamp_convert, port_no, &ts->ts);
        if (rc == VTSS_RC_OK) {
            ts->ts_valid = TRUE;
        } else {
            ts->ts_valid = TRUE;
        }

    }
    VTSS_EXIT();
    return rc;
}


/* Allocate a timestamp id for a two step transmission */
vtss_rc vtss_tx_timestamp_idx_alloc(const vtss_inst_t          inst,
                                   const vtss_ts_timestamp_alloc_t *const alloc_parm,
                                   vtss_ts_id_t               *const ts_id)
{
    int port_idx = 0;
    vtss_rc rc = VTSS_RC_ERROR;
    u32 id = -1;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_RC_ERROR;
        /* Find a free ts_id */
        for (id = 0; id < TS_IDS_RESERVED_FOR_SW; id++) {
            if ((vtss_state->ts_status[id].reserved_mask & alloc_parm->port_mask) == 0) {
                vtss_state->ts_status[id].reserved_mask |= alloc_parm->port_mask;
                for (port_idx = 0; port_idx < VTSS_PORT_ARRAY_SIZE; port_idx++) {
                    if (alloc_parm->port_mask & (1LL<<port_idx)) {
                        vtss_state->ts_status[id].context[port_idx] = alloc_parm->context;
                        vtss_state->ts_status[id].cb[port_idx] = alloc_parm->cb;
                    }
                }
                vtss_state->ts_status[id].age = 0;
                ts_id->ts_id = id;
                VTSS_D("portmask = %llx, reserved_mask = %llx id = %u", alloc_parm->port_mask, vtss_state->ts_status[id].reserved_mask, ts_id->ts_id);
                rc = VTSS_RC_OK;
                break;
            }
        } 
    }
    VTSS_EXIT();
    return rc;
}


#define TSID_TX_MAX_TIMETICKS 3
#define TSID_RX_MAX_TIMETICKS 100
/* Age the FIFO timestamps */
vtss_rc vtss_timestamp_age(const vtss_inst_t              inst)
{
    int id;
    u64 port_mask;
    vtss_ts_timestamp_t ts;
    vtss_state_t *my_vtss_state;
    int port_idx = 0;
    vtss_rc rc = VTSS_RC_OK;
    void (*cb)(void *context, u32 port_no, vtss_ts_timestamp_t *ts);
    void *cx;
#if defined(VTSS_ARCH_LUTON26)
     /* LUTON26 does not generate tx timestamp interrupt, therefore it is first checked if there are any valid timestamps */
    rc = vtss_tx_timestamp_update(inst);
#endif
    if (rc == VTSS_RC_OK) {
        VTSS_ENTER();
        if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
            for (id = 0; id < VTSS_TS_ID_SIZE; id++) {
                if (vtss_state->ts_status[id].reserved_mask != 0LL || vtss_state->ts_status[id].rx_tc_valid) {
                    u32 max_age;
                    if (vtss_state->ts_status[id].reserved_mask != 0LL) {
                        max_age = TSID_TX_MAX_TIMETICKS;
                    } else {
                        max_age = TSID_RX_MAX_TIMETICKS;
                    }
                    if (++vtss_state->ts_status[id].age > max_age) {
                        VTSS_D("ageing timestamp ts_id = %d, reserved_mask = %llx", id, vtss_state->ts_status[id].reserved_mask);
                        port_idx = 0;
                        while (port_idx < VTSS_PORT_ARRAY_SIZE) {
                            port_mask = 1LL<<port_idx;
                            if ((vtss_state->ts_status[id].reserved_mask & port_mask)) {
                                vtss_state->ts_status[id].reserved_mask  &= ~port_mask;
                                ts.id = id;
                                ts.ts = 0;
                                ts.ts_valid = FALSE;
                                if (vtss_state->ts_status[id].cb[port_idx] && vtss_state->ts_status[id].context[port_idx]) {
                                    my_vtss_state = vtss_state; /* save context */
                                    /* avoid using vtss_state while outside the API lock, as the API may be called from an other thread */
                                    cb = vtss_state->ts_status[id].cb[port_idx];
                                    cx = vtss_state->ts_status[id].context[port_idx];
                                    VTSS_EXIT();
                                    /* call out of the API, to indicate timeout */
                                    cb(cx, port_idx, &ts);
                                    VTSS_ENTER();
                                    vtss_state = my_vtss_state; /* restore context */
                                    vtss_state->ts_status[id].cb[port_idx] = NULL;
                                    vtss_state->ts_status[id].context[port_idx] = NULL;
                                } else {
                                    VTSS_D("undefined TS callback port_idx %d, ts_idx %d", port_idx, id);
                                }
                                VTSS_D("port_no %d, ts_id %d, ts %d(%d)", port_idx, ts.id, ts.ts, ts.ts_valid);
                            }
                            ++ port_idx;
                        }
                        vtss_state->ts_status[id].reserved_mask = 0LL;
                        vtss_state->ts_status[id].valid_mask = 0LL;
                        vtss_state->ts_status[id].rx_tc_valid = FALSE;
                        vtss_state->ts_status[id].age = 0;
                        rc = VTSS_FUNC(ts_timestamp_id_release, id);
                    
                    }
                }
            }
        }
        VTSS_EXIT();
    }
    return rc;
}

#endif /* VTSS_FEATURE_TIMESTAMP */

/* - API SYNCE functions -------------------------------------- */

#if defined(VTSS_FEATURE_SYNCE)
#define clk_port_inx ((clk_port == VTSS_SYNCE_CLK_A) ? 0 : 1)
/* Set the configuration of a selected output clock port - against external clock controller. */
vtss_rc vtss_synce_clock_out_set(const vtss_inst_t              inst,
                                 const vtss_synce_clk_port_t    clk_port,
                                 const vtss_synce_clock_out_t   *const conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if ((clk_port == VTSS_SYNCE_CLK_A) || (clk_port == VTSS_SYNCE_CLK_B)) {
            vtss_state->synce_out_conf[clk_port_inx] = *conf;
            rc = VTSS_FUNC_COLD(synce_clock_out_set, clk_port_inx);
        }
        else    rc = VTSS_RC_ERROR;
    }
    VTSS_EXIT();
    return rc;
}

/* Get the configuration of a selected output clock port - against external clock controller. */
vtss_rc vtss_synce_clock_out_get(const vtss_inst_t            inst,
                                 const vtss_synce_clk_port_t  clk_port,
                                 vtss_synce_clock_out_t       *const conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if ((clk_port == VTSS_SYNCE_CLK_A) || (clk_port == VTSS_SYNCE_CLK_B))
            *conf = vtss_state->synce_out_conf[clk_port_inx];
        else    rc = VTSS_RC_ERROR;
    }
    VTSS_EXIT();
    return rc;
}

/* Set the configuration of input port for a selected output clock port */
vtss_rc vtss_synce_clock_in_set(const vtss_inst_t              inst,
                                const vtss_synce_clk_port_t    clk_port,
                                const vtss_synce_clock_in_t    *const conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if ((clk_port == VTSS_SYNCE_CLK_A) || (clk_port == VTSS_SYNCE_CLK_B)) {
            vtss_state->synce_in_conf[clk_port_inx] = *conf;
            rc = VTSS_FUNC_COLD(synce_clock_in_set, clk_port_inx);
        }
        else    rc = VTSS_RC_ERROR;
    }
    VTSS_EXIT();
    return rc;
}

/* Get the configuration of input port for a selected output clock port */
vtss_rc vtss_synce_clock_in_get(const vtss_inst_t            inst,
                                const vtss_synce_clk_port_t  clk_port,
                                vtss_synce_clock_in_t        *const conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if ((clk_port == VTSS_SYNCE_CLK_A) || (clk_port == VTSS_SYNCE_CLK_B))
            *conf = vtss_state->synce_in_conf[clk_port_inx];
        else    rc = VTSS_RC_ERROR;
    }
    VTSS_EXIT();
    return rc;
}

#endif /* VTSS_FEATURE_SYNCE */

#ifdef VTSS_FEATURE_SFLOW
vtss_rc vtss_sflow_port_conf_get(const vtss_inst_t      inst,
                                 const u32              port_no,
                                 vtss_sflow_port_conf_t *const conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
       *conf = vtss_state->sflow_conf[port_no];
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_sflow_port_conf_set(const vtss_inst_t            inst,
                                 const vtss_port_no_t         port_no,
                                 const vtss_sflow_port_conf_t *const conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
		rc = VTSS_FUNC(sflow_port_conf_set, port_no, conf);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_sflow_sampling_rate_convert(const vtss_inst_t  inst,
                                         const BOOL         power2,
                                         const u32          rate_in,
                                               u32          *const rate_out)
{
    // Don't take API lock and don't change vtss_state, since this is
    // a non-state-dependant function.
    return VTSS_FUNC_FROM_STATE(vtss_inst_check_no_persist(inst), sflow_sampling_rate_convert, power2, rate_in, rate_out);
}

#endif /* VTSS_FEATURE_SFLOW */

/* - Warm start functions -------------------------------------- */
#if defined(VTSS_FEATURE_WARM_START)
#if defined(VTSS_FEATURE_PORT_CONTROL)
static BOOL vtss_bool_changed(BOOL old, BOOL new)
{
    return ((old == new) || (old && new) ? 0 : 1);
}

#ifndef VTSS_ARCH_DAYTONA
/* Synchronize port configuration */
static vtss_rc vtss_port_conf_sync(vtss_port_no_t port_no)
{
    vtss_rc          rc;
    vtss_port_conf_t old, *new = &vtss_state->port_conf[port_no];

    if ((rc = VTSS_FUNC(port_conf_get, port_no, &old)) == VTSS_RC_OK &&
        (vtss_bool_changed(old.power_down, new->power_down) ||
         old.speed != new->speed ||
         vtss_bool_changed(old.fdx, new->fdx) ||
         vtss_bool_changed(old.flow_control.obey, new->flow_control.obey) ||
         vtss_bool_changed(old.flow_control.generate, new->flow_control.generate))) {
        /* Apply changed configuration */
        VTSS_I("port_no %u changed, apply port conf", port_no);
        rc = VTSS_FUNC(port_conf_set, port_no);
    }

    return rc;
}

/* Synchronize port clause 37 control */
static vtss_rc vtss_port_clause_37_sync(vtss_port_no_t port_no)
{
    vtss_rc                       rc = VTSS_RC_OK;
    vtss_port_clause_37_control_t old_ctrl, *new_ctrl = &vtss_state->port_clause_37[port_no];
    vtss_port_clause_37_adv_t     *old, *new;

    old = &old_ctrl.advertisement;
    new = &new_ctrl->advertisement;

    if (vtss_state->port_conf[port_no].if_type == VTSS_PORT_INTERFACE_SERDES &&
        (rc = VTSS_FUNC(port_clause_37_control_get, port_no, &old_ctrl)) == VTSS_RC_OK &&
        (vtss_bool_changed(old_ctrl.enable, new_ctrl->enable) ||
         vtss_bool_changed(old->next_page, new->next_page) ||
         vtss_bool_changed(old->acknowledge, new->acknowledge) ||
         old->remote_fault != new->remote_fault ||
         vtss_bool_changed(old->asymmetric_pause, new->asymmetric_pause) ||
         vtss_bool_changed(old->symmetric_pause, new->symmetric_pause) ||
         vtss_bool_changed(old->hdx, new->hdx) ||
         vtss_bool_changed(old->fdx, new->fdx))) {
        VTSS_I("port_no %u changed, apply clause 37 conf", port_no);
        rc = VTSS_FUNC(port_clause_37_control_set, port_no);
    }
    return rc;
}
#endif /* VTSS_ARCH_DAYTONA */
#endif /* VTSS_FEATURE_PORT_CONTROL */

#if defined(VTSS_FEATURE_LAYER2)
/* Synchronize static entries in the MAC address table */
static vtss_rc vtss_mac_table_sync_scan(BOOL skip_new)
{
    vtss_rc                rc;
    vtss_mac_table_entry_t mac_entry;
    vtss_vid_mac_t         vid_mac;
    u32                    pgid, mach, macl;
    vtss_mac_entry_t       *cur, *old = NULL, *cur_next;
    vtss_port_no_t         port_no;
    BOOL                   hw_found = 0, ipmc, del, next = 1, *member;

    memset(&mac_entry, 0, sizeof(mac_entry));
    cur = vtss_state->mac_list_used;
    while (1) {
        if (next && !skip_new) {
            /* Get next static HW entry */
            hw_found = 0;
            while (VTSS_FUNC(mac_table_get_next, &mac_entry, &pgid) == VTSS_RC_OK) {
                if (mac_entry.locked) {
                    hw_found = 1;
                    break;
                }
            }
        }

        del = 0;
        next = 0;
        if (hw_found) {
            /* HW entry found */
            vtss_mach_macl_get(&mac_entry.vid_mac, &mach, &macl);
            if (cur == NULL || cur->mach > mach || (cur->mach == mach && cur->macl > macl)) {
                /* SW entry non-existing or bigger, delete HW entry */
                del = 1;
                next = 1;
            } else {
                /* SW entry smaller or matching, add/update HW entry */
                next = (cur->mach == mach && cur->macl == macl);
            }
        } else if (cur == NULL) {
            /* No more entries, exit */
            break;
        }

        if (del) {
            /* Delete entry */
            VTSS_I("deleting %08x-%08x", mach, macl);
            rc = VTSS_FUNC(mac_table_del, &mac_entry.vid_mac);
            continue;
        }

        /* Add operation */
        cur_next = cur->next;
        vtss_mach_macl_set(&vid_mac, cur->mach, cur->macl);
        ipmc = vtss_ipmc_mac(&vid_mac);
        if (ipmc || !next) {
            /* IP MC or new entry */
            pgid = VTSS_PGID_NONE; /* Do not reuse PGID */
            if (skip_new) {
                /* Skip this entry */
                old = cur;
                cur = cur_next;
                continue;
            }
        }

        /* Add entry */
        member = vtss_state->pgid_table[VTSS_PGID_NONE].member;
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            member[port_no] = VTSS_PORT_BF_GET(cur->member, port_no);
        if (ipmc) {
            rc = VTSS_RC_OK;
            vtss_pgid_members_get(pgid, member);
            old = cur;
        } else {
            rc = vtss_pgid_alloc(&pgid, member, 0, 0);

            /* Remove entry from used list */
            if (old == NULL)
                vtss_state->mac_list_used = cur_next;
            else
                old->next = cur_next;

            /* Insert in free list */
            cur->next = vtss_state->mac_list_free;
            vtss_state->mac_list_free = cur;
        }
        if (rc == VTSS_RC_OK) {
            VTSS_I("adding %s: %08x-%08x, pgid: %u",
                   next ? "old" : "new" , cur->mach, cur->macl, pgid);
            rc = vtss_mac_entry_update(cur, pgid);
        }
        cur = cur_next;
    }

    return VTSS_RC_OK;
}

static vtss_rc vtss_mac_table_sync(void)
{
    /* Update existing non-IPMC entries, reusing the PGIDs */
    vtss_mac_table_sync_scan(1);

    /* Add new/IPMC entries, allocating new PGIDs */
    vtss_mac_table_sync_scan(0);

    /* Update page pointers */
    vtss_mac_pages_update();

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_LAYER2 */

static vtss_rc vtss_restart_sync(void)
{
#ifndef VTSS_ARCH_DAYTONA
    vtss_port_no_t port_no;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
#if defined(VTSS_CHIP_CU_PHY)
        VTSS_RC(vtss_phy_sync(port_no));
#endif /* VTSS_CHIP_CU_PHY */
#if defined(VTSS_CHIP_10G_PHY)
        VTSS_RC(vtss_phy_10g_sync(port_no));
#if defined(VTSS_FEATURE_WIS)
        VTSS_RC(vtss_phy_ewis_sync(port_no));
#endif /* VTSS_FEATURE_WIS */
#endif /* VTSS_CHIP_10G_PHY */
#if defined(VTSS_CHIP_CU_PHY) || defined(VTSS_CHIP_10G_PHY)
#if defined (VTSS_FEATURE_PHY_TIMESTAMP)
        VTSS_RC(vtss_phy_ts_sync(port_no));
#endif /* VTSS_FEATURE_PHY_TIMESTAMP */
#endif /* VTSS_CHIP_CU_PHY || VTSS_CHIP_10G_PHY*/

        if (vtss_state->create.target == VTSS_TARGET_CU_PHY ||
            vtss_state->create.target == VTSS_TARGET_10G_PHY) {
            continue;
        }
#if defined(VTSS_FEATURE_PORT_CONTROL)
        VTSS_RC(vtss_port_conf_sync(port_no));
        VTSS_RC(vtss_port_clause_37_sync(port_no));
#endif /* VTSS_FEATURE_PORT_CONTROL */
#if defined(VTSS_FEATURE_QOS)
        VTSS_FUNC_RC(qos_port_conf_set, port_no);
#endif /* VTSS_FEATURE_QOS */
#if defined(VTSS_FEATURE_LAYER2)
        VTSS_FUNC_RC(learn_port_mode_set, port_no);
        VTSS_FUNC_RC(vlan_port_conf_set, port_no);
#endif /* VTSS_FEATURE_LAYER2 */
    }

#if defined(VTSS_FEATURE_QOS)
    VTSS_FUNC_RC(qos_conf_set, 1);
#endif /* VTSS_FEATURE_QOS */
#if defined(VTSS_FEATURE_LAYER2)
    VTSS_FUNC_RC(isolated_port_members_set);
    VTSS_FUNC_RC(mirror_port_set);
    VTSS_FUNC_RC(mirror_ingress_set);
    VTSS_FUNC_RC(mirror_egress_set);
    VTSS_FUNC_RC(flood_conf_set);
    VTSS_RC(vtss_mac_table_sync());
    VTSS_RC(vtss_update_masks(1, 1, 1));

    {
        vtss_vid_t vid;
        for (vid = 0; vid < VTSS_VIDS; vid++) {
            VTSS_FUNC_RC(vlan_port_members_set, vid);
        }
    }
#endif /* VTSS_FEATURE_LAYER2 */
#endif /* VTSS_ARCH_DAYTONA */

    return VTSS_RC_OK;
}

static vtss_rc vtss_restart_cur_set(const vtss_restart_t restart)
{
    vtss_rc rc = VTSS_RC_OK;
    
    vtss_state->restart_cur = restart;

    if (vtss_state->create.target != VTSS_TARGET_CU_PHY &&
        vtss_state->create.target != VTSS_TARGET_10G_PHY) {
        rc = VTSS_FUNC(restart_conf_set);
    }
#if defined(VTSS_CHIP_CU_PHY)
    if (rc == VTSS_RC_OK)
        rc = vtss_phy_restart_conf_set();
#endif /* VTSS_CHIP_CU_PHY */
#if defined(VTSS_CHIP_10G_PHY)
    if (rc == VTSS_RC_OK)
        rc = vtss_phy_10g_restart_conf_set();
#endif /* VTSS_CHIP_10G_PHY */
    return rc;
}

vtss_rc vtss_restart_conf_end(const vtss_inst_t inst)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        if (vtss_state->warm_start_cur) {
            /* Leave warm start mode */
            vtss_state->warm_start_cur = 0;

            /* Apply configuration */
            VTSS_I("warm start sync start");
            rc = vtss_restart_sync();
            VTSS_I("warm start sync done");
        } else {
            VTSS_I("cold/cool start end");
        }
        /* Next restart is warm */
        rc = vtss_restart_cur_set(VTSS_RESTART_WARM);
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_restart_status_get(const vtss_inst_t inst,
                                vtss_restart_status_t *const status)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        status->restart = vtss_state->restart_prev;
        status->prev_version = vtss_state->version_prev;
        status->cur_version = vtss_state->version_cur;
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_restart_conf_get(const vtss_inst_t inst,
                              vtss_restart_t *const restart)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *restart = vtss_state->restart_cur;
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_restart_conf_set(const vtss_inst_t inst,
                              const vtss_restart_t restart)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK && vtss_state->warm_start_cur == 0) {
        rc = vtss_restart_cur_set(restart);
    }
    VTSS_EXIT();

    return rc;
}
#endif /* VTSS_FEATURE_WARM_START */



#if defined(VTSS_FEATURE_FAN)
// See vtss_misc_api.h
vtss_rc vtss_temp_sensor_init(const vtss_inst_t     inst,
                              const BOOL enable)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(chip_temp_init, enable);
    }
    VTSS_EXIT();

    return rc;
}

// See vtss_misc_api.h
vtss_rc vtss_temp_sensor_get(const vtss_inst_t     inst,
                             i16                   *temperature)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(chip_temp_get, temperature);
    }
    VTSS_EXIT();

    return rc;
}



// See vtss_misc_api.h
vtss_rc vtss_fan_rotation_get(const vtss_inst_t     inst,
                              vtss_fan_conf_t       *const fan_spec,
                              u32                   *const rotation_count)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(fan_rotation_get, fan_spec, rotation_count);
    }
    VTSS_EXIT();

    return rc;
}



// See vtss_misc_api.h
vtss_rc vtss_fan_controller_init(const vtss_inst_t      inst,
                                 const vtss_fan_conf_t *const fan_spec)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(fan_controller_init, fan_spec);
    }
    VTSS_EXIT();

    return rc;
}



// See vtss_misc_api.h
vtss_rc vtss_fan_cool_lvl_set(const vtss_inst_t     inst,
                              u8 lvl)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(fan_cool_lvl_set, lvl);
    }
    VTSS_EXIT();

    return rc;
}

// See vtss_misc_api.h
vtss_rc vtss_fan_cool_lvl_get(const vtss_inst_t     inst,
                              u8 *lvl)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(fan_cool_lvl_get, lvl);
    }
    VTSS_EXIT();

    return rc;
}

#endif /* VTSS_FEATURE_FAN */


#if defined(VTSS_FEATURE_EEE)
vtss_rc vtss_eee_port_conf_set(const vtss_inst_t                 inst,
                               vtss_port_no_t                    port_no,
                               const vtss_eee_port_conf_t *const eee_conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(eee_port_conf_set, port_no, eee_conf);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_eee_port_state_set(const vtss_inst_t                  inst,
                                vtss_port_no_t                     port_no,
                                const vtss_eee_port_state_t *const eee_state)
{
    // Cannot take API lock or change vtss_state on JR, since EEE
    // control is S/W-based and must go fast.
    return VTSS_FUNC_FROM_STATE(vtss_inst_check_no_persist(inst), eee_port_state_set, port_no, eee_state);
}

vtss_rc vtss_eee_port_counter_get(const vtss_inst_t                    inst,
                                  const vtss_port_no_t                 port_no,
                                        vtss_eee_port_counter_t *const eee_counter)
{
    // Cannot take API lock or change vtss_state on JR, since EEE
    // control is S/W-based and must go fast.
    return VTSS_FUNC_FROM_STATE(vtss_inst_check_no_persist(inst), eee_port_counter_get, port_no, eee_counter);
}


#endif /* VTSS_FEATURE_EEE */

#if defined(VTSS_FEATURE_VCL)
vtss_rc vtss_vcl_port_conf_get(const vtss_inst_t    inst,
                               const vtss_port_no_t port_no,
                               vtss_vcl_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->vcl_port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vcl_port_conf_set(const vtss_inst_t          inst,
                               const vtss_port_no_t       port_no,
                               const vtss_vcl_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        vtss_state->vcl_port_conf[port_no] = *conf;
        rc = VTSS_FUNC_COLD(vcl_port_conf_set, port_no);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vce_init(const vtss_inst_t inst,
                      const vtss_vce_type_t  type,
                      vtss_vce_t *const vce)
{
    vtss_vce_key_t *key = &vce->key;
    
    VTSS_D("type: %d", type);
    memset(vce, 0, sizeof(*vce));
    key->type = type;
    key->mac.dmac_mc = VTSS_VCAP_BIT_ANY;
    key->mac.dmac_bc = VTSS_VCAP_BIT_ANY;
    key->tag.dei = VTSS_VCAP_BIT_ANY;
    key->tag.tagged = VTSS_VCAP_BIT_ANY;
    key->tag.s_tag = VTSS_VCAP_BIT_ANY;

    switch (type) {
    case VTSS_VCE_TYPE_ANY:
    case VTSS_VCE_TYPE_ETYPE:
    case VTSS_VCE_TYPE_LLC:
    case VTSS_VCE_TYPE_SNAP:
    case VTSS_VCE_TYPE_IPV6:
        break;
    case VTSS_VCE_TYPE_IPV4:
        key->frame.ipv4.fragment = VTSS_VCAP_BIT_ANY;
        key->frame.ipv4.options = VTSS_VCAP_BIT_ANY;
        break;
    default:
        VTSS_E("unknown type: %d", type);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_vce_add(const vtss_inst_t    inst,
                     const vtss_vce_id_t  vce_id,
                     const vtss_vce_t     *const vce)
{
    vtss_rc rc;

    VTSS_D("vce_id: %u before %u %s",
            vce->id, vce_id, vce_id == VTSS_VCE_ID_LAST ? "(last)" : "");

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vce_add, vce_id, vce);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vce_del(const vtss_inst_t    inst,
                     const vtss_vce_id_t  vce_id)
{
    vtss_rc rc;

    VTSS_D("vce_id: %u", vce_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vce_del, vce_id);
    VTSS_EXIT();
    return rc;
}
#endif /* VCL */

#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
vtss_rc vtss_vlan_trans_group_add(const vtss_inst_t      inst,
                                  const u16              group_id,
                                  const vtss_vid_t       vid,
                                  const vtss_vid_t       trans_vid)
{
    vtss_rc rc;

    VTSS_D("group_id: %u, vid: %u, trans_vid: %u", group_id, vid, trans_vid);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vlan_trans_group_add, group_id, vid, trans_vid);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_trans_group_del(const vtss_inst_t      inst,
                                  const u16              group_id,
                                  const vtss_vid_t       vid)
{
    vtss_rc rc;

    VTSS_D("group_id: %u, vid: %u", group_id, vid);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vlan_trans_group_del, group_id, vid);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_trans_group_get(const vtss_inst_t                inst,
                                  vtss_vlan_trans_grp2vlan_conf_t  *conf,
                                  BOOL                             next)
{
    vtss_rc rc;

    VTSS_D("group_id: %u", conf->group_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vlan_trans_group_get, conf, next);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_trans_group_to_port_set(const vtss_inst_t                     inst,
                                          const vtss_vlan_trans_port2grp_conf_t *conf)
{
    vtss_rc rc;

    VTSS_D("group_id: %u", conf->group_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vlan_trans_port_conf_set, conf);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_trans_group_to_port_get(const vtss_inst_t                inst,
                                          vtss_vlan_trans_port2grp_conf_t  *conf,
                                          BOOL                             next)
{
    vtss_rc rc;

    VTSS_D("group_id: %u", conf->group_id);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vlan_trans_port_conf_get, conf, next);
    VTSS_EXIT();
    return rc;
}

#endif /* VLAN_TRANSLATION */

#if defined(VTSS_ARCH_SERVAL)
vtss_rc vtss_vcap_port_conf_get(const vtss_inst_t     inst,
                                const vtss_port_no_t  port_no,
                                vtss_vcap_port_conf_t *const conf)
{
    vtss_rc rc;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK)
        *conf = vtss_state->vcap_port_conf[port_no];
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vcap_port_conf_set(const vtss_inst_t           inst,
                                const vtss_port_no_t        port_no,
                                const vtss_vcap_port_conf_t *const conf)
{
    vtss_rc               rc;
    vtss_vcap_port_conf_t *port_conf;

    VTSS_D("port_no: %u", port_no);
    VTSS_ENTER();
    if ((rc = vtss_inst_port_no_check(inst, port_no)) == VTSS_RC_OK) {
        port_conf = &vtss_state->vcap_port_conf[port_no];
        vtss_state->vcap_port_conf_old = *port_conf;
        *port_conf = *conf;
        rc = VTSS_FUNC_COLD(vcap_port_conf_set, port_no);
        if (rc != VTSS_RC_OK) {
            /* Restore configuration if operation failed */
            *port_conf = vtss_state->vcap_port_conf_old;
        }
    }
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_ARCH_SERVAL */

/* VLAN counters */
#if defined(VTSS_FEATURE_VLAN_COUNTERS)

static vtss_rc vtss_vid_null_check(const vtss_vid_t vid)
{
    if (vid == VTSS_VID_NULL || vid >= VTSS_VIDS) {
        VTSS_E("illegal vid: %u", vid);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}


vtss_rc vtss_vlan_counters_get(const vtss_inst_t    inst,
                               const vtss_vid_t     vid,
                               vtss_vlan_counters_t *const counters)
{
    vtss_rc rc;

    VTSS_RC(vtss_vid_null_check(vid));

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vlan_counters_get, vid, counters);
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_vlan_counters_clear(const vtss_inst_t  inst,
                                 const vtss_vid_t   vid)
{
    vtss_rc rc;

    VTSS_RC(vtss_vid_null_check(vid));

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = VTSS_FUNC(vlan_counters_clear, vid);
    VTSS_EXIT();
    return rc;
}
#endif /* VTSS_FEATURE_VLAN_COUNTERS */

/* - OAM -------------------------------------------------- */

#if defined(VTSS_FEATURE_OAM)

vtss_rc vtss_oam_vop_conf_get(const vtss_inst_t          inst,
                              vtss_oam_vop_conf_t *const cfg)
{
    vtss_rc rc;

    VTSS_D("oam vop get");

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *cfg = vtss_state->oam_vop_conf;
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_vop_conf_set(const vtss_inst_t                inst,
                              const vtss_oam_vop_conf_t *const cfg)
{
    vtss_rc rc;

    VTSS_D("oam vop set");

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->oam_vop_conf = *cfg;
        rc = VTSS_FUNC(oam_vop_cfg_set);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_oam_event_poll(const vtss_inst_t            inst,
                            vtss_oam_event_mask_t *const mask)
{
    vtss_rc rc;

    VTSS_D("oam event poll");

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_event_poll, mask);
    }
    VTSS_EXIT();
    return rc;        

}

vtss_rc vtss_oam_voe_event_enable(const vtss_inst_t               inst,
                                  const vtss_oam_voe_idx_t        voe_idx,
                                  const vtss_oam_voe_event_mask_t mask,
                                  const BOOL                      enable)
{
    vtss_rc rc;

    VTSS_D("oam voe %d event enable: 0x%08x", voe_idx, mask);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_event_enable, voe_idx, mask, enable);
    }
    VTSS_EXIT();
    return rc;        
}

vtss_rc vtss_oam_voe_event_poll(const vtss_inst_t         inst,
                                const vtss_oam_voe_idx_t  voe_idx,
                                vtss_oam_voe_event_mask_t *const mask)
{
    vtss_rc rc;

    VTSS_D("oam voe %d event poll", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_event_poll, voe_idx, mask);
    }
    VTSS_EXIT();
    return rc;        
}

vtss_rc vtss_oam_voe_alloc(const vtss_inst_t              inst,
                           const vtss_oam_voe_type_t      type,
                           const vtss_oam_voe_alloc_cfg_t *data,
                           vtss_oam_voe_idx_t             *voe_idx)
{
    vtss_rc rc;

    VTSS_D("oam voe alloc, type %d", type);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_alloc, type, data, voe_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_oam_voe_free(const vtss_inst_t        inst,
                          const vtss_oam_voe_idx_t voe_idx)
{
    vtss_rc rc;

    VTSS_D("oam voe %d free", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_free, voe_idx);
    }
    VTSS_EXIT();
    return rc;

}

vtss_rc vtss_oam_voe_conf_get(const vtss_inst_t          inst,
                              const vtss_oam_voe_idx_t   voe_idx,
                              vtss_oam_voe_conf_t *const cfg)
{
    vtss_rc rc;

    VTSS_D("oam voe %d get", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        *cfg = vtss_state->oam_voe_conf[voe_idx];
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_voe_conf_set(const vtss_inst_t                inst,
                              const vtss_oam_voe_idx_t         voe_idx,
                              const vtss_oam_voe_conf_t *const cfg)
{
    vtss_rc rc;

    VTSS_D("oam voe %d set", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        vtss_state->oam_voe_conf[voe_idx] = *cfg;
        rc = VTSS_FUNC(oam_voe_cfg_set, voe_idx);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_voe_ccm_arm_hitme(const vtss_inst_t        inst,
                                   const vtss_oam_voe_idx_t voe_idx,
                                   BOOL                     enable)
{
    vtss_rc rc;

    VTSS_D("oam voe %d hitme arm: %d", voe_idx, enable);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_ccm_arm_hitme, voe_idx, enable);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_voe_ccm_set_rdi_flag(const vtss_inst_t        inst,
                                      const vtss_oam_voe_idx_t voe_idx,
                                      BOOL                     flag)
{
    vtss_rc rc;

    VTSS_D("oam voe %d rdi set: %d", voe_idx, flag);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_ccm_set_rdi_flag, voe_idx, flag);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_ccm_status_get(const vtss_inst_t        inst,
                                const vtss_oam_voe_idx_t voe_idx,
                                vtss_oam_ccm_status_t    *value)
{
    vtss_rc rc;

    VTSS_D("oam voe %d status get", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_ccm_status_get, voe_idx, value);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_pdu_seen_status_get(const vtss_inst_t          inst,
                                     const vtss_oam_voe_idx_t   voe_idx,
                                     vtss_oam_pdu_seen_status_t *value)
{
    vtss_rc rc;

    VTSS_D("oam voe %d status get", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_pdu_seen_status_get, voe_idx, value);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_proc_status_get(const vtss_inst_t        inst,
                                 const vtss_oam_voe_idx_t voe_idx,
                                 vtss_oam_proc_status_t   *value)
{
    vtss_rc rc;

    VTSS_D("oam voe %d processing status get", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_proc_status_get, voe_idx, value);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_voe_counter_update(const vtss_inst_t        inst,
                                    const vtss_oam_voe_idx_t voe_idx)
{
    vtss_rc rc;

    VTSS_D("oam voe %d counter update", voe_idx);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_counter_update, voe_idx);
    }
    VTSS_EXIT();
    return rc;

}

#if defined(VTSS_ARCH_SERVAL)
vtss_rc vtss_oam_voe_counter_update_serval(const vtss_inst_t        inst,
                                           const vtss_oam_voe_idx_t voe_idx)
{
    vtss_rc rc;

    VTSS_D("oam voe %d counter update (serval)", voe_idx);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_counter_update_serval, voe_idx);
    }
    VTSS_EXIT();
    return rc;

}
#endif

vtss_rc vtss_oam_voe_counter_get(const vtss_inst_t        inst,
                                 const vtss_oam_voe_idx_t voe_idx,
                                 vtss_oam_voe_counter_t   *value)
{
    vtss_rc rc;

    VTSS_D("oam voe %d counter get", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_counter_get, voe_idx, value);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_voe_counter_clear(const vtss_inst_t        inst,
                                   const vtss_oam_voe_idx_t voe_idx,
                                   const u32                mask)
{
    vtss_rc rc;

    VTSS_D("oam voe %d status get", voe_idx);

    if (voe_idx >= VTSS_OAM_VOE_CNT)
        return VTSS_RC_ERROR;
    
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_voe_counter_clear, voe_idx, mask);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_ipt_conf_get(const vtss_inst_t   inst,
                              const u32           isdx,
                              vtss_oam_ipt_conf_t *cfg)
{
    vtss_rc rc;

    VTSS_D("oam ipt %d conf get", isdx);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_ipt_conf_get, isdx, cfg);
    }
    VTSS_EXIT();
    return rc;    
}

vtss_rc vtss_oam_ipt_conf_set(const vtss_inst_t                inst,
                              const u32                        isdx,
                              const vtss_oam_ipt_conf_t *const cfg)
{
    vtss_rc rc;

    VTSS_D("oam ipt %d conf set", isdx);

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(oam_ipt_conf_set, isdx, cfg);
    }
    VTSS_EXIT();
    return rc;    
}

#endif  /* VTSS_FEATURE_OAM */

/* - MPLS ------------------------------------------------- */

#if defined(VTSS_FEATURE_MPLS)

vtss_rc vtss_mpls_tc_conf_get(vtss_inst_t                 inst,
                              vtss_mpls_tc_conf_t * const conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *conf = vtss_state->mpls_tc_conf;
    }
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_mpls_tc_conf_set(vtss_inst_t                       inst,
                              const vtss_mpls_tc_conf_t * const conf)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_tc_conf_set, conf);
    }
    VTSS_EXIT();
    return rc;
}


vtss_rc vtss_mpls_l2_segment_attach(vtss_inst_t                   inst,
                                    const vtss_mpls_l2_idx_t      idx,
                                    const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_l2_segment_attach, idx, seg_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_l2_segment_detach(vtss_inst_t                   inst,
                                    const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_l2_segment_detach, seg_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_l2_alloc(vtss_inst_t                inst,
                           vtss_mpls_l2_idx_t * const idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_l2_alloc, idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_l2_free(vtss_inst_t              inst,
                          const vtss_mpls_l2_idx_t idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_l2_free, idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_l2_get(vtss_inst_t              inst,
                         const vtss_mpls_l2_idx_t idx,
                         vtss_mpls_l2_t * const   l2)
{
    vtss_rc rc;
    VTSS_MPLS_IDX_CHECK_L2(idx);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *l2 = vtss_state->mpls_l2_conf[idx].pub;
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_l2_set(vtss_inst_t                  inst,
                         const vtss_mpls_l2_idx_t     idx,
                         const vtss_mpls_l2_t * const l2)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_l2_set, idx, l2);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_segment_alloc(vtss_inst_t                     inst,
                                const BOOL                      in_seg,
                                vtss_mpls_segment_idx_t * const idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_segment_alloc, in_seg, idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_segment_free(vtss_inst_t                   inst,
                               const vtss_mpls_segment_idx_t idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_segment_free, idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_xc_alloc(vtss_inst_t                inst,
                           const vtss_mpls_xc_type_t  type,
                           vtss_mpls_xc_idx_t * const idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_xc_alloc, type, idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_xc_free(vtss_inst_t              inst,
                          const vtss_mpls_xc_idx_t idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_xc_free, idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_segment_get(vtss_inst_t                   inst,
                              const vtss_mpls_segment_idx_t idx,
                              vtss_mpls_segment_t * const   seg)
{
    vtss_rc rc;
    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *seg = vtss_state->mpls_segment_conf[idx].pub;
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_segment_set(vtss_inst_t                       inst,
                              const vtss_mpls_segment_idx_t     idx,
                              const vtss_mpls_segment_t * const seg)
{
    vtss_rc rc;
    VTSS_MPLS_IDX_CHECK_SEGMENT(idx);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_segment_set, idx, seg);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_segment_state_get(vtss_inst_t                       inst,
                                    const vtss_mpls_segment_idx_t     idx,
                                    vtss_mpls_segment_state_t * const state)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_segment_state_get, idx, state);
    }
    VTSS_EXIT();
    return rc;

}

vtss_rc vtss_mpls_xc_get(vtss_inst_t              inst,
                         const vtss_mpls_xc_idx_t idx,
                         vtss_mpls_xc_t * const   xc)
{
    vtss_rc rc;
    VTSS_MPLS_IDX_CHECK_XC(idx);
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        *xc = vtss_state->mpls_xc_conf[idx].pub;
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_xc_set(vtss_inst_t                  inst,
                         const vtss_mpls_xc_idx_t     idx,
                         const vtss_mpls_xc_t * const xc)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_xc_set, idx, xc);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_xc_segment_attach(vtss_inst_t                   inst,
                                    const vtss_mpls_xc_idx_t      xc_idx,
                                    const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_xc_segment_attach, xc_idx, seg_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_xc_segment_detach(vtss_inst_t                   inst,
                                    const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_xc_segment_detach, seg_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_xc_mc_segment_attach(vtss_inst_t                   inst,
                                       const vtss_mpls_xc_idx_t      xc_idx,
                                       const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_xc_mc_segment_attach, xc_idx, seg_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_xc_mc_segment_detach(vtss_inst_t                   inst,
                                       const vtss_mpls_xc_idx_t      xc_idx,
                                       const vtss_mpls_segment_idx_t seg_idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_xc_mc_segment_detach, xc_idx, seg_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_segment_server_attach(vtss_inst_t                   inst,
                                        const vtss_mpls_segment_idx_t idx,
                                        const vtss_mpls_segment_idx_t srv_idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_segment_server_attach, idx, srv_idx);
    }
    VTSS_EXIT();
    return rc;
}

vtss_rc vtss_mpls_segment_server_detach(vtss_inst_t                   inst,
                                        const vtss_mpls_segment_idx_t idx)
{
    vtss_rc rc;
    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(mpls_segment_server_detach, idx);
    }
    VTSS_EXIT();
    return rc;
}

#endif  /* VTSS_FEATURE_MPLS */

/* - Daytona misc functions ------------------------------- */
#ifdef VTSS_ARCH_DAYTONA
vtss_rc vtss_interrupt_enable_set(const vtss_inst_t             inst,
                                  const vtss_interrupt_block_t  *const blocks)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(interrupt_enable_set, blocks);
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_interrupt_enable_get(const vtss_inst_t       inst,
                                  vtss_interrupt_block_t  *const blocks)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(interrupt_enable_get, blocks);
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_interrupt_status_get(const vtss_inst_t        inst,
                                  vtss_interrupt_block_t   *const status)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(interrupt_status_get, status);
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_pmtick_set(const vtss_inst_t     inst,
                        const vtss_port_no_t port_no,
                        const vtss_pmtick_control_t *const control)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(pmtick_enable_set, port_no, control);
    }
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_channel_mode_set(const vtss_inst_t inst,
                              const u16 channel,
                              const vtss_config_mode_t conf_mode,
                              const u32 two_lane_upi)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK) {
        rc = VTSS_FUNC(channel_mode_set, channel, conf_mode, two_lane_upi);
    }
    VTSS_EXIT();

    return rc;
}
#endif /* VTSS_ARCH_DAYTONA */


/* - Debug information functions ------------------------------- */

vtss_rc vtss_debug_info_get(vtss_debug_info_t *const info)
{
    vtss_port_no_t port_no;

    memset(info, 0, sizeof(*info));
    info->chip_no = VTSS_CHIP_NO_ALL;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        info->port_list[port_no] = 1;
    return VTSS_RC_OK;
}

vtss_rc vtss_debug_info_print(const vtss_inst_t         inst,
                              const vtss_debug_printf_t pr,
                              const vtss_debug_info_t   *const info)
{
    vtss_rc rc;

    VTSS_ENTER();
    if ((rc = vtss_inst_check(inst)) == VTSS_RC_OK)
        rc = vtss_cmn_debug_info_print(pr, info);
    VTSS_EXIT();

    return rc;
}

vtss_rc vtss_debug_reg_check_set(const vtss_inst_t inst,
                                 const BOOL        enable)
{
    vtss_state_t *state = vtss_inst_check_no_persist(inst);

    if (state == NULL || state != vtss_default_inst) {
        VTSS_EG(VTSS_TRACE_GROUP_REG_CHECK, "Register checking is only supported when only one instance of the API is instantiated.");
        return VTSS_RC_ERROR;
    }

    return vtss_cmn_debug_reg_check_set(state, enable);
}

