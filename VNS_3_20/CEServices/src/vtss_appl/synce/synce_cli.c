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

#include "synce_cli.h"
#include "cli.h"
#include "synce.h"
#include "cli_trace_def.h"

typedef struct {
    /* Memory address and size */
    ulong    mem_addr;
    ulong    mem_size;

    synce_mgmt_selection_mode_t selection_mode;
    uint                        clk_source;
    uint                        clk_priority;
    synce_mgmt_quality_level_t  ssm_holdover;
    synce_mgmt_quality_level_t  ssm_freerun;
    synce_mgmt_quality_level_t  ssm_overwrite;
    BOOL                        ssm_enable;
    uint                        wtr_time;
    uint                        holdoff;
    synce_mgmt_aneg_mode_t      aneg;

    BOOL   parsed_port;
    BOOL   parsed_clock;
    BOOL   parsed_aneg;
    BOOL   parsed_overwrite;
    BOOL   parsed_holdoff;

    BOOL   parsed_selection;
    BOOL   parsed_wtr;
    BOOL   parsed_freerun;
    BOOL   parsed_holdover;
} synce_cli_req_t;

/****************************************************************************/
/*  Initialization                                                          */
/****************************************************************************/

void synce_cli_init(void)
{
    /* register the size required for port req. structure */
    cli_req_size_register(sizeof(synce_cli_req_t));
}

/****************************************************************************/
/*  Command functions                                                       */
/****************************************************************************/

static char* ssm_string(synce_mgmt_quality_level_t ssm)
{
    switch(ssm)
    {
        case QL_NONE: return("   QL_NONE");
        case QL_PRC:  return("    QL_PRC");
        case QL_SSUA: return("   QL_SSUA");
        case QL_SSUB: return("   QL_SSUB");
        case QL_DNU:  return("    QL_DNU");
        case QL_EEC2: return("   QL_EEC2");
        case QL_EEC1: return("   QL_EEC1");
        case QL_INV:  return("    QL_INV");
        case QL_FAIL: return("   QL_FAIL");
        case QL_LINK: return("   QL_LINK");
        default:      return("   QL_NONE");
    }
}

static void synce_print_error(uint   rc)
{
    switch (rc)
    {
        case SYNCE_RC_INVALID_PARAMETER: CPRINTF("Invalid parameter error returned from SYNCE\n"); break;
        case SYNCE_RC_NOM_PORT:          CPRINTF("Port nominated to a clock source is already nominated\n"); break;
/*        case VTSS_SYNCE_RE_NOM:          CPRINTF("Not legal to change port on a nominated clock source - first de-nominate clock source\n"); break;*/
        case SYNCE_RC_SELECTION:         CPRINTF("NOT possible to make Manuel To Selected if not in locked mode\n"); break;
        case SYNCE_RC_INVALID_PORT:      CPRINTF("The selected port is not valid\n"); break;
        default:                         CPRINTF("Unknown error returned from SYNCE\n"); break;
    }
}

static void cli_cmd_synce_state(cli_req_t *req)
{
    uint                            rc;
    vtss_port_no_t                  iport;
    uint                            source, i;
    BOOL                            master;
    synce_mgmt_selector_state_t     selector_state;
    synce_mgmt_alarm_state_t        alarm_state;
    synce_mgmt_port_conf_blk_t      port_config;
    synce_mgmt_quality_level_t      ssm_rx;
    synce_mgmt_quality_level_t      ssm_tx;

    if ((rc = synce_mgmt_clock_state_get(&selector_state, &source, &alarm_state)) != SYNCE_RC_OK) {
        synce_print_error(rc);
        return;
    }

    CPRINTF("\nSelector State is: ");
    switch (selector_state)
    {
        case SYNCE_MGMT_SELECTOR_STATE_LOCKED:   CPRINTF("Locked to %d\n", source+1); break;
        case SYNCE_MGMT_SELECTOR_STATE_HOLDOVER: CPRINTF("Holdover\n"); break;
        case SYNCE_MGMT_SELECTOR_STATE_FREERUN:  CPRINTF("Free Run\n"); break;
    }

    CPRINTF("\nAlarm State is:\n");
    CPRINTF("Clk:");
    for (i=0; i<SYNCE_NOMINATED_MAX; ++i)  CPRINTF("       %2d", i+1);
    CPRINTF("\n");
    CPRINTF("LOCS:");
    for (i = 0; i < SYNCE_NOMINATED_MAX; ++i) {
        if (alarm_state.locs[i])
            CPRINTF("    TRUE ");
        else
            CPRINTF("    FALSE");
    }
/*    CPRINTF("\nFOS: ");
    for (i=0; i<SYNCE_NOMINATED_MAX; ++i)
    {
        if (alarm_state.fos[i]) CPRINTF("    TRUE ");
        else                    CPRINTF("    FALSE");
    }*/
    CPRINTF("\nSSM: ");
    for (i=0; i<SYNCE_NOMINATED_MAX; ++i) {
        if (alarm_state.ssm[i])
            CPRINTF("    TRUE ");
        else
            CPRINTF("    FALSE");
    }
    CPRINTF("\nWTR: ");
    for (i=0; i<SYNCE_NOMINATED_MAX; ++i) {
        if (alarm_state.wtr[i])
            CPRINTF("    TRUE ");
        else
            CPRINTF("    FALSE");
    }
/*    if (alarm_state.losx) CPRINTF("\nLOSX:   TRUE ");
    else                  CPRINTF("\nLOSX:   FALSE");*/
    CPRINTF("\n");
    if (alarm_state.lol)  CPRINTF("\nLOL:     TRUE");
    else                  CPRINTF("\nLOL:     FALSE");
    if (alarm_state.dhold)CPRINTF("\nDHOLD:   TRUE \n");
    else                  CPRINTF("\nDHOLD:   FALSE\n");

    rc = synce_mgmt_port_config_get(&port_config);

    CPRINTF("\nSSM State is:\n");
    CPRINTF("Port    Tx SSM    Rx SSM      Mode\n");

    for (iport = 0; iport < SYNCE_PORT_COUNT; iport++) {
        if (port_config.ssm_enabled[iport]) {
            if ((rc = synce_mgmt_port_state_get(iport + VTSS_PORT_NO_START, &ssm_rx, &ssm_tx, &master)) != SYNCE_RC_OK) {
                synce_print_error(rc);
                return;
            }
            CPRINTF("%4u%s%s%s\n", iport2uport(iport + VTSS_PORT_NO_START), ssm_string(ssm_tx), ssm_string(ssm_rx), (!master) ? "     Slave" : "    Master");
        }
    }
    CPRINTF("\n");

    return;
}

static void synce_print_sources(void)
{
    uint      i;

    CPRINTF("Clock Source      ");
    for (i=0; i<SYNCE_NOMINATED_MAX; ++i)  CPRINTF("      %2d", i+1);
    CPRINTF("\n");
}

static void synce_print_nom(synce_mgmt_clock_conf_blk_t     *clock_config)
{
    uint      i;

    CPRINTF("Ports:            ");
    for (i = 0; i < SYNCE_NOMINATED_MAX; ++i)  CPRINTF("      %2u", clock_config->nominated[i] ? (iport2uport(clock_config->port[i])) : 0);
    CPRINTF("\nSSM Overwrite:    ");
    for (i=0; i<SYNCE_NOMINATED_MAX; ++i)
        switch (clock_config->ssm_overwrite[i])
        {
            case QL_NONE: CPRINTF(" QL_NONE"); break;
            case QL_PRC:  CPRINTF("  QL_PRC"); break;
            case QL_SSUA: CPRINTF(" QL_SSUA"); break;
            case QL_SSUB: CPRINTF(" QL_SSUB"); break;
            case QL_EEC2: CPRINTF(" QL_EEC2"); break;
            case QL_EEC1: CPRINTF(" QL_EEC1"); break;
            case QL_DNU:  CPRINTF("  QL_DNU"); break;
            default:      CPRINTF(" Invalid");
        }
    CPRINTF("\nHold Off:         ");
    for (i = 0; i < SYNCE_NOMINATED_MAX; ++i)  CPRINTF("     %3u", clock_config->holdoff_time[i]);
    CPRINTF("\nANEG Mode:        ");
    for (i=0; i<SYNCE_NOMINATED_MAX; ++i)
        switch (clock_config->aneg_mode[i])
        {
            case SYNCE_MGMT_ANEG_NONE:             CPRINTF("    None"); break;
            case SYNCE_MGMT_ANEG_PREFERED_SLAVE:   CPRINTF("   Slave"); break;
            case SYNCE_MGMT_ANEG_PREFERED_MASTER:  CPRINTF("  Master"); break;
            case SYNCE_MGMT_ANEG_FORCED_SLAVE:     CPRINTF("  Forced"); break;
            default:      CPRINTF(" Invalid");
        }
    CPRINTF("\n");
}

static void synce_print_prio(synce_mgmt_clock_conf_blk_t     *clock_config)
{
    uint      i;

    CPRINTF("Priority:         ");
    for (i=0; i<SYNCE_NOMINATED_MAX; ++i)  CPRINTF("      %2d", clock_config->priority[i]);
    CPRINTF("\n");
}

static void synce_print_select(synce_mgmt_clock_conf_blk_t     *clock_config)
{
    CPRINTF("Selection Mode: ");
    switch (clock_config->selection_mode)
    {
        case SYNCE_MGMT_SELECTOR_MODE_MANUEL:                  CPRINTF("    Manuel to source %d", clock_config->source+1); break;
        case SYNCE_MGMT_SELECTOR_MODE_MANUEL_TO_SELECTED:      CPRINTF("    Manuel to selected %d", clock_config->source+1); break;
        case SYNCE_MGMT_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE:  CPRINTF("    Automatic Nonrevertive"); break;
        case SYNCE_MGMT_SELECTOR_MODE_AUTOMATIC_REVERTIVE:     CPRINTF("    Automatic Revertive"); break;
        case SYNCE_MGMT_SELECTOR_MODE_FORCED_HOLDOVER:         CPRINTF("    Forced Holdover"); break;
        case SYNCE_MGMT_SELECTOR_MODE_FORCED_FREE_RUN:         CPRINTF("    Forced Free Run"); break;
        default:                                               CPRINTF("    Invalid"); break;
    }
    CPRINTF("\nWTR Time:  %d min.", clock_config->wtr_time);
    CPRINTF("\nSSM Hold Over:");
    switch (clock_config->ssm_holdover)
    {
        case QL_NONE: CPRINTF("  QL_NONE"); break;
        case QL_PRC:  CPRINTF("  QL_PRC"); break;
        case QL_SSUA: CPRINTF("  QL_SSUA"); break;
        case QL_SSUB: CPRINTF("  QL_SSUB"); break;
        case QL_EEC2: CPRINTF("  QL_EEC2"); break;
        case QL_EEC1: CPRINTF("  QL_EEC1"); break;
        case QL_DNU:  CPRINTF("  QL_DNU"); break;
        case QL_INV:  CPRINTF("  QL_INV"); break;
        default:      CPRINTF("  Invalid");
    }
    CPRINTF("\nSSM Free Run:");
    switch (clock_config->ssm_freerun)
    {
        case QL_NONE: CPRINTF("  QL_NONE"); break;
        case QL_PRC:  CPRINTF("  QL_PRC"); break;
        case QL_SSUA: CPRINTF("  QL_SSUA"); break;
        case QL_SSUB: CPRINTF("  QL_SSUB"); break;
        case QL_EEC2: CPRINTF("  QL_EEC2"); break;
        case QL_EEC1: CPRINTF("  QL_EEC1"); break;
        case QL_DNU:  CPRINTF("  QL_DNU"); break;
        case QL_INV:  CPRINTF("  QL_INV"); break;
        default:      CPRINTF("  Invalid");
    }
    CPRINTF("\n");
}

static void synce_print_port(synce_mgmt_port_conf_blk_t *port_config)
{
    vtss_port_no_t iport;

    CPRINTF("SSM enabled on these ports:\n");
    for (iport = 0; iport < SYNCE_PORT_COUNT; iport++) {
        if (port_config->ssm_enabled[iport]) {
            CPRINTF("%u\n", iport2uport(iport));
        }
    }
    CPRINTF("\n");
}

static void cli_cmd_synce_config(cli_req_t *req)
{
    uint   rc;
    synce_mgmt_clock_conf_blk_t   clock_config;
    synce_mgmt_port_conf_blk_t    port_config;

    if(!req->set) {
        cli_header("SyncE Configuration", 1);
    }

    if ((rc = synce_mgmt_clock_config_get(&clock_config)) != SYNCE_RC_OK)
    {
        synce_print_error(rc);
        return;
    }

    if ((rc = synce_mgmt_port_config_get(&port_config)) != SYNCE_RC_OK)
    {
        synce_print_error(rc);
        return;
    }

    CPRINTF("Nomination is:\n");
    synce_print_sources();
    synce_print_nom(&clock_config);
    synce_print_prio(&clock_config);
    CPRINTF("\n");
    synce_print_select(&clock_config);
    CPRINTF("\n");
    synce_print_port(&port_config);

    return;
}

static void cli_cmd_synce_nominated_source(cli_req_t *req)
{
    uint   rc;
    synce_mgmt_clock_conf_blk_t     clock_config;
    vtss_port_no_t iport;
    synce_cli_req_t *synce_req = req->module_req;
    synce_mgmt_aneg_mode_t  aneg;
    synce_mgmt_quality_level_t  overwrite;
    uint    holdoff;

    if ((rc = synce_mgmt_clock_config_get(&clock_config)) != SYNCE_RC_OK) {
        synce_print_error(rc);
        return;
    }
    if (req->set) {
        if (!synce_req->parsed_clock)      CPRINTF("Parameter 'clock_source' is missing\n");
        if (!synce_req->parsed_port)       iport = clock_config.port[synce_req->clk_source];
        else                               iport = (req->uport == 0) ? 0 : uport2iport(req->uport);
        if (!synce_req->parsed_overwrite)  overwrite = clock_config.ssm_overwrite[synce_req->clk_source];
        else                               overwrite = synce_req->ssm_overwrite;
        if (!synce_req->parsed_holdoff)    holdoff = clock_config.holdoff_time[synce_req->clk_source];
        else                               holdoff = synce_req->holdoff;
        if (!synce_req->parsed_aneg)       aneg = clock_config.aneg_mode[synce_req->clk_source];
        else                               aneg = synce_req->aneg;

        if ((rc = synce_mgmt_nominated_source_set(synce_req->clk_source, (iport != 0), iport, aneg, holdoff, overwrite)) != SYNCE_RC_OK) {
            synce_print_error(rc);
            return;
        }
    } else {
        CPRINTF("\n");
        CPRINTF("Nomination is:\n");
        synce_print_sources();
        synce_print_nom(&clock_config);
        CPRINTF("\n");
    }

    return;
}

static void cli_cmd_synce_nominated_selection_mode(cli_req_t *req)
{
    uint   rc;
    synce_mgmt_clock_conf_blk_t     clock_config;
    synce_cli_req_t *synce_req = req->module_req;
    synce_mgmt_selection_mode_t    selection;
    uint                           source;
    uint                           wtr;
    synce_mgmt_quality_level_t     holdover;
    synce_mgmt_quality_level_t     freerun;

    if ((rc = synce_mgmt_clock_config_get(&clock_config)) != SYNCE_RC_OK)
    {
        synce_print_error(rc);
        return;
    }
    if (req->set)
    {
        if (!synce_req->parsed_selection)  selection = clock_config.selection_mode;
        else                               selection = synce_req->selection_mode;
        if (!synce_req->parsed_clock)      source = clock_config.source;
        else                               source = synce_req->clk_source;
        if (!synce_req->parsed_wtr)        wtr = clock_config.wtr_time;
        else                               wtr = synce_req->wtr_time;
        if (!synce_req->parsed_holdover)   holdover = clock_config.ssm_holdover;
        else                               holdover = synce_req->ssm_holdover;
        if (!synce_req->parsed_freerun)    freerun = clock_config.ssm_freerun;
        else                               freerun = synce_req->ssm_freerun;

        if ((rc = synce_mgmt_nominated_selection_mode_set(selection, source, wtr, holdover, freerun)) != SYNCE_RC_OK)
        {
            synce_print_error(rc);
            return;
        }
    }
    else
    {
        CPRINTF("\n");
        synce_print_select(&clock_config);
        CPRINTF("\n");
    }

    return;
}

static void cli_cmd_synce_nominated_priority(cli_req_t *req)
{
    uint   rc;
    synce_mgmt_clock_conf_blk_t     clock_config;
    synce_cli_req_t *synce_req = req->module_req;

    if (req->set)
    {
        if ((rc = synce_mgmt_nominated_priority_set(synce_req->clk_source, synce_req->clk_priority)) != SYNCE_RC_OK)
        {
            synce_print_error(rc);
            return;
        }
    }
    else
    {
        if ((rc = synce_mgmt_clock_config_get(&clock_config)) != SYNCE_RC_OK)
        {
            synce_print_error(rc);
            return;
        }

        CPRINTF("\n");
        CPRINTF("Priority is:\n");
        synce_print_sources();
        synce_print_prio(&clock_config);
        CPRINTF("\n");
    }

    return;
}

static void cli_cmd_synce_ssm(cli_req_t *req)
{
    uint   rc;
    synce_mgmt_port_conf_blk_t    port_config;
    synce_cli_req_t *synce_req = req->module_req;

    if (req->set) {
        if (!synce_req->parsed_port)    CPRINTF("Parameter 'port' is missing\n");
        if ((rc = synce_mgmt_ssm_set(uport2iport(req->uport), synce_req->ssm_enable)) != SYNCE_RC_OK) {
            synce_print_error(rc);
            return;
        }
    } else {
        if ((rc = synce_mgmt_port_config_get(&port_config)) != SYNCE_RC_OK) {
            synce_print_error(rc);
            return;
        }

        CPRINTF("\n");
        synce_print_port(&port_config);
    }

    return;
}


static void cli_cmd_synce_clear_wtr(cli_req_t *req)
{
    uint   rc;
    synce_cli_req_t *synce_req = req->module_req;

    if ((rc = synce_mgmt_wtr_clear_set(synce_req->clk_source)) != SYNCE_RC_OK)
        synce_print_error(rc);

    return;
}

static void cli_cmd_synce_debug_clock_read(cli_req_t *req)
{
    ulong  i;
    uint   value, n, rc;
    synce_cli_req_t *synce_req = req->module_req;

    if ((((synce_req->mem_addr + synce_req->mem_size - 1) > 55) && (synce_req->mem_addr < 128)) ||
        (((synce_req->mem_addr + synce_req->mem_size - 1) > 143) && (synce_req->mem_addr != 185)))
        CPRINTF("Undefined address space\n");

    for (i=0; i<synce_req->mem_size; i++)
    {
        if ((rc = synce_mgmt_register_get(i + synce_req->mem_addr, &value)) != SYNCE_RC_OK) {
            synce_print_error(rc);
            return;
        }
        n = (i & 0x7);
        if (n == 0)   CPRINTF("%03u: ", i + synce_req->mem_addr);
        CPRINTF("%02x ", value);
        if (n == 0x7 || i == (synce_req->mem_size - 1))  CPRINTF("\n");
    }

    return;
}

static void cli_cmd_synce_debug_clock_write(cli_req_t *req)
{
    uint rc;
    synce_cli_req_t *synce_req = req->module_req;

    if (((synce_req->mem_addr > 55) && (synce_req->mem_addr < 128)) || (synce_req->mem_addr > 143))
        CPRINTF("Undefined address space\n");

    if ((rc = synce_mgmt_register_set(synce_req->mem_addr, req->value)) != SYNCE_RC_OK)
        synce_print_error(rc);

    return;
}

static void cli_cmd_synce_debug_clock_test(cli_req_t *req)
{
    uint rc, reg;

    if ((vtss_board_type() == VTSS_BOARD_JAG_CU24_REF) || (vtss_board_type() == VTSS_BOARD_JAG_SFP24_REF))
    {
        /* On Jaguar platform clock port 2 has to be used for 156,25 MHz to 10G PHY */
        if ((rc = synce_mgmt_register_get(25, &reg)) != SYNCE_RC_OK)     synce_print_error(rc);    //spi_write_masked(25, 0x00, 0xE0);   /* N1_HS: 4 */
        reg &= ~0xE0;
        if ((rc = synce_mgmt_register_set(25, reg)) != SYNCE_RC_OK)     synce_print_error(rc);

        if ((rc = synce_mgmt_register_set(31, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(31, 0);   /* NC1_LS: 10 */
        if ((rc = synce_mgmt_register_set(32, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(32, 0);   /* NC1_LS: 10 */
        if ((rc = synce_mgmt_register_set(33, 9)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(33, 9);   /* NC1_LS: 10 */

        if ((rc = synce_mgmt_register_set(34, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(34, 0);   /* NC2_LS: 8 */
        if ((rc = synce_mgmt_register_set(35, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(35, 0);   /* NC2_LS: 8 */
        if ((rc = synce_mgmt_register_set(36, 7)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(36, 7);   /* NC2_LS: 8 */

        if ((rc = synce_mgmt_register_get(40, &reg)) != SYNCE_RC_OK)     synce_print_error(rc);     //spi_write_masked(40, 0x20, 0xE0);   /* N2_HS: 5 */
        reg &= ~0xEF;                                                                               //spi_write_masked(40, 0, 0x0F);      /* N2_LS: 600 */
        reg |= 0x20;
        if ((rc = synce_mgmt_register_set(40, reg)) != SYNCE_RC_OK)     synce_print_error(rc);

        if ((rc = synce_mgmt_register_set(41, 0x02)) != SYNCE_RC_OK)     synce_print_error(rc);     //spi_write(41, 0x02);                /* N2_LS: 600 */
        if ((rc = synce_mgmt_register_set(42, 0x57)) != SYNCE_RC_OK)     synce_print_error(rc);     //spi_write(42, 0x57);                /* N2_LS: 600 */
    
        if ((rc = synce_mgmt_register_set(43, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(43, 0);   /* N31: 75 */
        if ((rc = synce_mgmt_register_set(44, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(44, 0);   /* N31: 75 */
        if ((rc = synce_mgmt_register_set(45, 74)) != SYNCE_RC_OK)    synce_print_error(rc);        //spi_write(45, 74);  /* N31: 75 */
    
        if ((rc = synce_mgmt_register_set(46, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(46, 0);   /* N32: 6 */
        if ((rc = synce_mgmt_register_set(47, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(47, 0);   /* N32: 6 */
        if ((rc = synce_mgmt_register_set(48, 5)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(48, 5);   /* N32: 6 */
    }
    else
    {
		/* 10 MHz clockoutput on port 2 is wanted and possible */
        if ((rc = synce_mgmt_register_get(25, &reg)) != SYNCE_RC_OK)     synce_print_error(rc);    //spi_write_masked(25, 0x20, 0xE0);   /* N1_HS: 5 */
        reg &= ~0xE0;
        reg |= 0x20;
        if ((rc = synce_mgmt_register_set(25, reg)) != SYNCE_RC_OK)     synce_print_error(rc);

        if ((rc = synce_mgmt_register_set(31, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(31, 0);   /* NC1_LS: 8 */
        if ((rc = synce_mgmt_register_set(32, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(32, 0);   /* NC1_LS: 8 */
        if ((rc = synce_mgmt_register_set(33, 7)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(33, 7);   /* NC1_LS: 8 */

        if ((rc = synce_mgmt_register_set(34, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(34, 0);   /* NC2_LS: 100 */
        if ((rc = synce_mgmt_register_set(35, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(35, 0);   /* NC2_LS: 100 */
        if ((rc = synce_mgmt_register_set(36, 63)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(36, 63);   /* NC2_LS: 100 */

        if ((rc = synce_mgmt_register_get(40, &reg)) != SYNCE_RC_OK)     synce_print_error(rc);     //spi_write_masked(40, 0x00, 0xE0);   /* N2_HS: 4 */
        reg &= ~0xEF;                                                                               //spi_write_masked(40, 0, 0x0F);      /* N2_LS: 750 */
        if ((rc = synce_mgmt_register_set(40, reg)) != SYNCE_RC_OK)     synce_print_error(rc);

        if ((rc = synce_mgmt_register_set(41, 0x02)) != SYNCE_RC_OK)     synce_print_error(rc);     //spi_write(41, 0x02);                /* N2_LS: 750 */
        if ((rc = synce_mgmt_register_set(42, 0xED)) != SYNCE_RC_OK)     synce_print_error(rc);     //spi_write(42, 0xED);                /* N2_LS: 750 */
    
        if ((rc = synce_mgmt_register_set(43, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(43, 0);   /* N31: 75 */
        if ((rc = synce_mgmt_register_set(44, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(44, 0);   /* N31: 75 */
        if ((rc = synce_mgmt_register_set(45, 74)) != SYNCE_RC_OK)    synce_print_error(rc);        //spi_write(45, 74);  /* N31: 75 */
    
        if ((rc = synce_mgmt_register_set(46, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(46, 0);   /* N32: 6 */
        if ((rc = synce_mgmt_register_set(47, 0)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(47, 0);   /* N32: 6 */
        if ((rc = synce_mgmt_register_set(48, 5)) != SYNCE_RC_OK)     synce_print_error(rc);        //spi_write(48, 5);   /* N32: 6 */
    }

    return;
}

static void synce_cli_req_def_set(cli_req_t *req)
{
    synce_cli_req_t *synce_req = req->module_req;

    synce_req->ssm_enable = TRUE;
    synce_req->ssm_overwrite = QL_NONE;
    synce_req->ssm_holdover = QL_NONE;
    synce_req->ssm_freerun = QL_NONE;
    synce_req->selection_mode = SYNCE_MGMT_SELECTOR_MODE_AUTOMATIC_REVERTIVE;
    synce_req->clk_source = 0;
    synce_req->wtr_time = 5;  /* Default 5 min. WTR */
    synce_req->clk_priority = 0;
    synce_req->aneg = SYNCE_MGMT_ANEG_NONE;
    synce_req->holdoff = 0;

    synce_req->parsed_port = FALSE;
    synce_req->parsed_clock = FALSE;
    synce_req->parsed_aneg = FALSE;
    synce_req->parsed_overwrite = FALSE;
    synce_req->parsed_holdoff = FALSE;

    synce_req->parsed_selection = FALSE;
    synce_req->parsed_wtr = FALSE;
    synce_req->parsed_freerun = FALSE;
    synce_req->parsed_holdover = FALSE;

    return;
}

/****************************************************************************/
/*  Parameter functions                                                     */
/****************************************************************************/

static int cli_parse_selection_mode(char *cmd, cli_req_t *req, char *stx)
{
    synce_cli_req_t *synce_req = req->module_req;
    char *found = cli_parse_find(cmd, stx);

    if(!found) {
        return 1;
    } else if(!strncmp(found, "manuel", 6)) {
        synce_req->selection_mode = SYNCE_MGMT_SELECTOR_MODE_MANUEL;
    } else if(!strncmp(found, "selected", 8)) {
        synce_req->selection_mode = SYNCE_MGMT_SELECTOR_MODE_MANUEL_TO_SELECTED;
    } else if(!strncmp(found, "nonrevertive", 12)) {
        synce_req->selection_mode = SYNCE_MGMT_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE;
    } else if(!strncmp(found, "revertive", 9)) {
        synce_req->selection_mode = SYNCE_MGMT_SELECTOR_MODE_AUTOMATIC_REVERTIVE;
    } else if(!strncmp(found, "holdover", 8)) {
        synce_req->selection_mode = SYNCE_MGMT_SELECTOR_MODE_FORCED_HOLDOVER;
    } else if(!strncmp(found, "freerun", 7)) {
        synce_req->selection_mode = SYNCE_MGMT_SELECTOR_MODE_FORCED_FREE_RUN;
    } else return 1;
    synce_req->parsed_selection = TRUE;

    return 0;
} /* cli_parse_selection_mode */


static int cli_parse_ssm_overwrite(char *cmd, cli_req_t *req, char *stx)
{
    synce_cli_req_t *synce_req = req->module_req;
    char *found = cli_parse_find(cmd, stx);

    if(!found)      return 1;
    else if(!strncmp(found, "ql_none", 7))     synce_req->ssm_overwrite = QL_NONE;
    else if(!strncmp(found, "ql_prc", 6))      synce_req->ssm_overwrite = QL_PRC;
    else if(!strncmp(found, "ql_ssua", 7))     synce_req->ssm_overwrite = QL_SSUA;
    else if(!strncmp(found, "ql_ssub", 7))     synce_req->ssm_overwrite = QL_SSUB;
    else if(!strncmp(found, "ql_eec2", 7))     synce_req->ssm_overwrite = QL_EEC2;
    else if(!strncmp(found, "ql_eec1", 7))     synce_req->ssm_overwrite = QL_EEC1;
    else if(!strncmp(found, "ql_dnu", 6))      synce_req->ssm_overwrite = QL_DNU;
    else return 1;
    synce_req->parsed_overwrite = TRUE;

    return 0;
} /* cli_parse_ssm_holdover */

static int cli_parse_ssm_holdover(char *cmd, cli_req_t *req, char *stx)
{
    synce_cli_req_t *synce_req = req->module_req;
    char *found = cli_parse_find(cmd, stx);

    if(!found)      return 1;
    else if(!strncmp(found, "ho_none", 7))     synce_req->ssm_holdover = QL_NONE;
    else if(!strncmp(found, "ho_prc", 6))      synce_req->ssm_holdover = QL_PRC;
    else if(!strncmp(found, "ho_ssua", 7))     synce_req->ssm_holdover = QL_SSUA;
    else if(!strncmp(found, "ho_ssub", 7))     synce_req->ssm_holdover = QL_SSUB;
    else if(!strncmp(found, "ho_eec2", 7))     synce_req->ssm_holdover = QL_EEC2;
    else if(!strncmp(found, "ho_eec1", 7))     synce_req->ssm_holdover = QL_EEC1;
    else if(!strncmp(found, "ho_dnu", 6))      synce_req->ssm_holdover = QL_DNU;
    else if(!strncmp(found, "ho_inv", 6))      synce_req->ssm_holdover = QL_INV;
    else return 1;
    synce_req->parsed_holdover = TRUE;

    return 0;
} /* cli_parse_ssm_holdover */

static int cli_parse_ssm_freerun(char *cmd, cli_req_t *req, char *stx)
{
    synce_cli_req_t *synce_req = req->module_req;
    char *found = cli_parse_find(cmd, stx);

    if(!found)      return 1;
    else if(!strncmp(found, "fr_none", 7))     synce_req->ssm_freerun = QL_NONE;
    else if(!strncmp(found, "fr_prc", 6))      synce_req->ssm_freerun = QL_PRC;
    else if(!strncmp(found, "fr_ssua", 7))     synce_req->ssm_freerun = QL_SSUA;
    else if(!strncmp(found, "fr_ssub", 7))     synce_req->ssm_freerun = QL_SSUB;
    else if(!strncmp(found, "fr_eec2", 7))     synce_req->ssm_freerun = QL_EEC2;
    else if(!strncmp(found, "fr_eec1", 7))     synce_req->ssm_freerun = QL_EEC1;
    else if(!strncmp(found, "fr_dnu", 6))      synce_req->ssm_freerun = QL_DNU;
    else if(!strncmp(found, "fr_inv", 6))      synce_req->ssm_freerun = QL_INV;
    else return 1;
    synce_req->parsed_freerun = TRUE;

    return 0;
} /* cli_parse_ssm_freerun */

static int cli_parse_ssm_enable(char *cmd, cli_req_t *req, char *stx)
{
    synce_cli_req_t *synce_req = req->module_req;
    char *found = cli_parse_find(cmd, stx);

    if(!found)      return 1;

    else if(!strncmp(found, "enable", 6))       synce_req->ssm_enable = TRUE;
    else if(!strncmp(found, "disable", 7))      synce_req->ssm_enable = FALSE;
    else return 1;

    return 0;
} /* cli_parse_ssm_enable */

static int synce_cli_parm_parse_port(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    synce_cli_req_t *synce_req = req->module_req;
    int error = 0;
    ulong value = 0;
    ulong min = 1, max = VTSS_PORTS;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, min, max);
    if (error)
        error = cli_parse_ulong(cmd, &value, 0, 0);
    req->uport = value;
    if (!error) synce_req->parsed_port = TRUE;
    return(error);
}

static int synce_cli_parm_parse_holdoff(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    synce_cli_req_t *synce_req = req->module_req;
    int error = 0;
    ulong value = 0;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, SYNCE_HOLDOFF_MIN, SYNCE_HOLDOFF_MAX);
    if (error)
        error = cli_parse_ulong(cmd, &value, SYNCE_HOLDOFF_TEST, SYNCE_HOLDOFF_TEST);
    if (error)
        error = cli_parse_ulong(cmd, &value, 0, 0);
    synce_req->holdoff = value;
    if (!error) synce_req->parsed_holdoff = TRUE;
    return(error);
}

static int synce_cli_parm_parse_wtr_time(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;
    ulong value = 0;
    synce_cli_req_t *synce_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, 12);
    synce_req->wtr_time = value;
    if (!error) synce_req->parsed_wtr = TRUE;
    return(error);
}

static int synce_cli_parm_parse_clk_source(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;
    ulong value = 0;
    synce_cli_req_t *synce_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 1, SYNCE_NOMINATED_MAX);
    synce_req->clk_source = value-1;
    if (!error) synce_req->parsed_clock = TRUE;
    return(error);
}

static int synce_cli_parm_parse_clk_selection_mode(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;

    req->parm_parsed = 1;
    error = cli_parse_selection_mode(cmd, req, stx);
    return(error);
}

static int synce_cli_parm_parse_ssm_en_dis(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;

    req->parm_parsed = 1;
    error = cli_parse_ssm_enable(cmd, req, stx);
    return(error);
}

static int synce_cli_parm_parse_ssm_overwrite(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;

    req->parm_parsed = 1;
    error = cli_parse_ssm_overwrite(cmd, req, stx);
    return(error);
}

static int synce_cli_parm_parse_aneg(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    synce_cli_req_t *synce_req = req->module_req;
    char *found = cli_parse_find(cmd, stx);

    if(!found)      return 1;
    else if(!strncmp(found, "master", 6))     synce_req->aneg = SYNCE_MGMT_ANEG_PREFERED_MASTER;
    else if(!strncmp(found, "slave", 5))      synce_req->aneg = SYNCE_MGMT_ANEG_PREFERED_SLAVE;
    else if(!strncmp(found, "forced", 6))     synce_req->aneg = SYNCE_MGMT_ANEG_FORCED_SLAVE;
    else return 1;
    synce_req->parsed_aneg = TRUE;

    return 0;
}

static int synce_cli_parm_parse_ssm_holdover(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;

    req->parm_parsed = 1;
    error = cli_parse_ssm_holdover(cmd, req, stx);
    return(error);
}

static int synce_cli_parm_parse_ssm_freerun(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;

    req->parm_parsed = 1;
    error = cli_parse_ssm_freerun(cmd, req, stx);
    return(error);
}

static int synce_cli_parm_parse_clk_priority(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;
    ulong value = 0;
    synce_cli_req_t *synce_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &value, 0, SYNCE_PRIORITY_MAX - 1);
    synce_req->clk_priority = value;
    return(error);
}

static int synce_cli_parm_parse_value(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &req->value, 0, 0xffffffff);
    return(error);
}

static int synce_cli_parm_parse_mem_addr(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;
    synce_cli_req_t *synce_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &synce_req->mem_addr, 0, 0xffffffff);
    return(error);
}

static int synce_cli_parm_parse_mem_size(char *cmd, char *cmd2,
                        char *stx, char *cmd_org, cli_req_t *req)
{
    int error = 0;
    synce_cli_req_t *synce_req = req->module_req;

    req->parm_parsed = 1;
    error = cli_parse_ulong(cmd, &synce_req->mem_size, 0, 0xffffffff);
    return(error);
}

/****************************************************************************/
/*  Parameter table                                                         */
/****************************************************************************/

static cli_parm_t synce_cli_parm_table[] = {
    {
        "<port>",
        "Port number (0 is de-nominate)\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_port,
        cli_cmd_synce_nominated_source
    },
    {
        "<wtr_time>",
        "WTR time (0-12 min) '0' is disable\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_wtr_time,
        cli_cmd_synce_nominated_selection_mode
    },
    {
        "<clk_source>",
        "Clock source identification 1-n (only if manuel selection mode)\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_clk_source,
        cli_cmd_synce_nominated_selection_mode
    },
    {
        "manuel|selected|nonrevertive|revertive|holdover|freerun",
        "Clock source selection mode\n"
        "     manuel:       Selector is manually set to the chosen clock source\n"
        "     selected:     Selector is manually set to the pt. selected clocfk source (not possible in unlocked mode\n"
        "     nonrevertive: Selector is automatically selecting the best clock source - non revertively\n"
        "     revertive:    Selector is automatically selecting the best clock source - revertively\n"
        "     holdover:     Selector is forced in holdover\n"
        "     freerun:      Selector is forced in free run\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_clk_selection_mode,
        cli_cmd_synce_nominated_selection_mode
    },
    {
        "enable|disable",
        "enable/disable SSM\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_ssm_en_dis,
        cli_cmd_synce_ssm
    },
    {
        "ql_none|ql_prc|ql_ssua|ql_ssub|ql_eec2|ql_eec1|ql_dnu",
        "Clock source SSM overwrite\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_ssm_overwrite,
        cli_cmd_synce_nominated_source
    },
    {
        "master|slave|forced",
        "1000BaseT ANEG mode\n"
        "master: Activate prefer master negotiation\n"
        "slave: Activate prefer slave negotiation\n"
        "forced: Activate forced slave negotiation\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_aneg,
        cli_cmd_synce_nominated_source
    },
    {
        "<holdoff>",
        "The hold off timer value in 100 ms."
        "Valid values are: 0 for disable. The range 3-18. The value 100 for test\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_holdoff,
        cli_cmd_synce_nominated_source
    },
    {
        "ho_none|ho_prc|ho_ssua|ho_ssub|ho_eec2|ho_eec1|ho_dnu|ho_inv",
        "Hold Over SSM overwrite\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_ssm_holdover,
        cli_cmd_synce_nominated_selection_mode
    },
    {
        "fr_none|fr_prc|fr_ssua|fr_ssub|fr_eec2|fr_eec1|fr_dnu|fr_inv",
        "Hold Over SSM overwrite\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_ssm_freerun,
        cli_cmd_synce_nominated_selection_mode
    },
    {
        "<clk_source>",
        "Clock source identification 1-n\n",
        CLI_PARM_FLAG_NONE,
        synce_cli_parm_parse_clk_source,
        NULL
    },
    {
        "<clk_priority>",
        "Clock source priority setting\n",
        CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_clk_priority,
        NULL
    },
    {
        "<value>",
        "Register value",
        CLI_PARM_FLAG_NONE | CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_value,
        NULL
    },
    {
        "<mem_addr>",
        "Register memory address",
        CLI_PARM_FLAG_NONE | CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_mem_addr,
        NULL
    },
    {
        "<mem_size>",
        "Register memory size",
        CLI_PARM_FLAG_NONE | CLI_PARM_FLAG_SET,
        synce_cli_parm_parse_mem_size,
        NULL
    },
    {NULL, NULL, 0, 0, NULL}
};

/****************************************************************************/
/*  Command table                                                           */
/****************************************************************************/

enum {
    CLI_CMD_SYNCE_NOMINATED_SOURCE_PRIO = 0,
    CLI_CMD_SYNCE_NOMINATED_SELECTION_MODE_PRIO,
    CLI_CMD_SYNCE_NOMINATED_PRIORITY_PRIO,
    CLI_CMD_SYNCE_SSM_PRIO,
    CLI_CMD_SYNCE_CLEAR_WTR_PRIO,
    CLI_CMD_SYNCE_STATE_PRIO,
    CLI_CMD_SYNCE_CONFIG_PRIO,
    CLI_CMD_DEBUG_CLOCK_READ_PRIO = CLI_CMD_SORT_KEY_DEFAULT,
    CLI_CMD_DEBUG_CLOCK_WRITE_PRIO = CLI_CMD_SORT_KEY_DEFAULT,
    CLI_CMD_DEBUG_CLOCK_TEST_PRIO = CLI_CMD_SORT_KEY_DEFAULT
};

cli_cmd_tab_entry(
    "SyncE Nominate",
    "SyncE Nominate [<clk_source>] [<port>] [ql_none|ql_prc|ql_ssua|ql_ssub|ql_eec2|ql_eec1|ql_dnu] [<holdoff>] [master|slave|forced]",
    "Nominate a PHY port to become a selectable clock source",
    CLI_CMD_SYNCE_NOMINATED_SOURCE_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SYNCE,
    cli_cmd_synce_nominated_source,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    "SyncE Selection",
    "SyncE Selection [manuel|selected|nonrevertive|revertive|holdover|freerun] [<clk_source>] [<wtr_time>] [ho_none|ho_prc|ho_ssua|ho_ssub|ho_eec2|ho_eec1|ho_dnu|ho_inv] [fr_none|fr_prc|fr_ssua|fr_ssub|fr_eec2|fr_eec1|fr_dnu|fr_inv]",
    "Selection mode of nominated clock sources\n"
    "<clk_source> is the source for 'manuel' selection\n"
    "<ho_xxx> is the Hold Over Quality Level\n"
    "<fr_xxx> is the Free Run Quality Level\n",
    CLI_CMD_SYNCE_NOMINATED_SELECTION_MODE_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SYNCE,
    cli_cmd_synce_nominated_selection_mode,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    "SyncE Priority",
    "SyncE Priority [<clk_source>] [<clk_priority>]",
    "Priority of nominated clock sources",
    CLI_CMD_SYNCE_NOMINATED_PRIORITY_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SYNCE,
    cli_cmd_synce_nominated_priority,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    "SyncE Ssm",
    "SyncE Ssm [<port>] [enable|disable]",
    "SyncE SSM enable/disable",
    CLI_CMD_SYNCE_SSM_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SYNCE,
    cli_cmd_synce_ssm,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    NULL,
    "SyncE Clear <clk_source>",
    "SyncE Clear active WTR timer",
    CLI_CMD_SYNCE_CLEAR_WTR_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_SYNCE,
    cli_cmd_synce_clear_wtr,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    "SyncE State",
    NULL,
    "Get selector state",
    CLI_CMD_SYNCE_STATE_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_SYNCE,
    cli_cmd_synce_state,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    "SyncE Config",
    NULL,
    "Get SyncE config data",
    CLI_CMD_SYNCE_CONFIG_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_SYNCE,
    cli_cmd_synce_config,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_SYS_CONF
);

cli_cmd_tab_entry(
    "Debug Clock Read <mem_addr> [<mem_size>]",
    NULL,
    "Read clock controller register",
    CLI_CMD_DEBUG_CLOCK_READ_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_DEBUG,
    cli_cmd_synce_debug_clock_read,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    NULL,
    "Debug Clock Write <mem_addr> <value>",
    "Write clock controller register",
    CLI_CMD_DEBUG_CLOCK_WRITE_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_DEBUG,
    cli_cmd_synce_debug_clock_write,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    NULL,
    "Debug Clock Test Config",
    "Configure CLK2 input to 10 Mbit",
    CLI_CMD_DEBUG_CLOCK_TEST_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_DEBUG,
    cli_cmd_synce_debug_clock_test,
    synce_cli_req_def_set,
    synce_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
