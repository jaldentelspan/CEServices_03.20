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

// Avoid "vtss_api.h not used in module port_custom_vns_12.c"
/*lint --e{766} */
#include "vtss_api.h"

#if defined(VTSS_ARCH_LUTON26)

#include "port_custom_api.h"
#include "board_probe.h"
#include "misc_api.h"

static int board_type;
static vtss_i2c_read_t    i2c_read;     /**< I2C read function */

/*lint -esym(459, lu26_port_table) */
static port_custom_entry_t lu26_port_table[VTSS_PORT_ARRAY_SIZE];


/* Release ports from reset */
static vtss_rc lu26_reset(void)
{
    vtss_rc rc = VTSS_RC_OK;
    vtss_port_no_t port_idx;

    rc = vtss_phy_pre_reset(NULL, 0);
    /* this loop is setting the downshift enable bit on all interal phys for two pair wires */
    for (port_idx = 0; port_idx < VTSS_PORTS; port_idx++) {
        (void)vtss_phy_write_masked_page(NULL, port_idx, VTSS_PHY_PAGE_EXTENDED, 0x14, 0x10, 0x10);
    }
    return rc;
}


/* Post ports reset */
static vtss_rc lu26_post_reset(void)
{
    return vtss_phy_post_reset(NULL, 0);
}

/* Initialize port LEDs */
static vtss_rc lu26_led_init(void)
{
    //vtss_port_no_t port;

    return VTSS_RC_OK;
}


static BOOL lu26_sfp_accept(u8 *sfp_rom)
{
    return TRUE;
}

// Function for doing SFP i2c reads. The luton26 board supports 2 types of I2C controllers. One in the switch,
// and one in the ATOM12 PHY. The one in the switch is used for uplink SFPs, while the one in the PHY is used for the dual media SFPs.
// In : port_no - The physical port number
//      i2c_addr - The address for the i2c device
//      addr     - The address with the i2c device to access.
//      cnt      - The number of data to read
// In/Out  data     - The data read
static vtss_rc lu26_sfp_i2c_read(vtss_port_no_t port_no, u8 i2c_addr, u8 addr, u8 *const data, u8 cnt)
{
    // Dual media SFP ports - Connected to PHY i2c
    if (port_no >= 20 && port_no <= 23) {
        // Due to a hardware board issue only SFP i2c mux 0 works, so that is always used.
        if (vtss_phy_i2c_read(NULL, port_no, 0, addr, i2c_addr, data, cnt) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }
    } else {
        // Uplink ports - Connected to switch i2c

        /* Custom board SFP i2c enable function */
        if (board_sfp_i2c_enable(port_no) != VTSS_RC_OK) { 
            return VTSS_RC_ERROR;
        }
     
        if (i2c_read != NULL) {
            return i2c_read(port_no, i2c_addr, addr, data, cnt, (-1));
        }
    }
    return VTSS_RC_OK;
}

static void lu26_sfp_update_if(vtss_port_no_t port_no, vtss_port_interface_t mac_if)
{
    lu26_port_table[port_no].mac_if = mac_if;
}

static vtss_rc lu26_sfp_mod_detect(BOOL *detect_status)
{
    vtss_sgpio_port_data_t   data[VTSS_SGPIO_PORTS];
    //vtss_port_no_t port_no;
   // u16  i;
   
    if (vtss_sgpio_read(NULL, 0, 0, data) != VTSS_RC_OK)
        return VTSS_RC_ERROR;

    return VTSS_RC_OK;
}

/* All SFPs have the same i2c address. One is enabled at a time through SGPIOs */
static vtss_rc lu26_sfp_i2c_enable(vtss_port_no_t port_no)
{

    if (vtss_gpio_mode_set(NULL, 0, 11, VTSS_GPIO_OUT) != VTSS_RC_OK)
        return VTSS_RC_ERROR;

    if (vtss_gpio_write(NULL, 0, 11, (lu26_port_table[port_no].map.chip_port==24)? 0 : 1))
        return VTSS_RC_ERROR;

    return VTSS_RC_OK;
}

#define NUM_PACKETS_FOR_ACTIVITY	40
/* Update port LED */
static vtss_rc lu26_led_update(vtss_port_no_t port_no,
                               vtss_port_status_t *status,
                               vtss_port_counters_t *counters,
                               port_custom_conf_t *port_conf)
{
	return VTSS_RC_OK;//added by Eric
}

/* Switch the SFP modules on/off using the SGPIOs */
static vtss_rc lu26_sfp_power_set(vtss_port_no_t port_no, BOOL enable)
{
    vtss_sgpio_conf_t conf;
    u32 chipport = lu26_port_table[port_no].map.chip_port;

    if (vtss_sgpio_conf_get(NULL, 0, 0, &conf) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    if (board_type == VTSS_BOARD_VNS_6_REF || board_type == VTSS_BOARD_VNS_12_REF) {
        if (chipport == 24) {
            conf.port_conf[26].mode[2] = enable ? VTSS_SGPIO_MODE_ON : VTSS_SGPIO_MODE_OFF;
        } else if (chipport == 25) {
            conf.port_conf[27].mode[1] = enable ? VTSS_SGPIO_MODE_ON : VTSS_SGPIO_MODE_OFF;
        }
        (void)vtss_sgpio_conf_set(NULL, 0, 0, &conf);
    }
    return VTSS_RC_OK;
}

static void lu26_port_conf(vtss_port_no_t port_no, port_custom_conf_t *port_conf, vtss_port_status_t *port_status)
{
    /*  Enable/disable SFP module 'Tx_disable' */
    (void)lu26_sfp_power_set(port_no, port_conf->enable);        
}

// lu26_pre_reset(), called before system reset.
// Disables the SGPIO controller and waits the required amount of time for it to settle.
// The potential problem is that: When resetting the SIO controller (and GPIOs)
// when the chain is only partially shifted out is that; if the shift registers on
// the PCB latches the data during reset (i.e. GPIO latch / clk is tristating and
// then being pulled to an active value) then all sorts of bad things can happen
//(for example a customers could use serial GPIO controlling board reset).
static void lu26_pre_reset(void)
{
    vtss_gpio_no_t gpio;
    
    /* Disable SGPIO controller (GPIO 0-3 for SGPIO) */
    for (gpio = 0; gpio < 4; gpio++) {
        (void) vtss_gpio_mode_set(NULL, 0, gpio, VTSS_GPIO_IN);
    }
    VTSS_MSLEEP(10); /* Waits a period time */

    if (board_type == VTSS_BOARD_VNS_6_REF || board_type == VTSS_BOARD_VNS_12_REF) {
        (void) vtss_phy_pre_system_reset(NULL, 0);
    } else {
        // Internal PHY
        (void) vtss_phy_pre_system_reset(NULL, 0);

        // External PHY (Atom12)
        (void) vtss_phy_pre_system_reset(NULL, 12);
    }
}

// Function for doing special port initialization that depends upon the platform
static void lu26_init(void)
{
    vtss_gpio_no_t    gpio;
    vtss_sgpio_conf_t conf;    
    u32               port;

	/* Disable SGPIO controller (GPIO 0-3 for SGPIO) */
	for (gpio = 0; gpio < 4; gpio++) {
		(void) vtss_gpio_mode_set(NULL, 0, gpio, VTSS_GPIO_IN);
	}

	/* Setup SGPIO */
	if (vtss_sgpio_conf_get(NULL, 0, 0, &conf) == VTSS_RC_OK) {
		/* The blink mode 0 is 5 HZ for link activity and collion in half duplex. */
		conf.bmode[0] = VTSS_SGPIO_BMODE_5;

        if ((board_type == VTSS_BOARD_ESIR_REF)) {
            conf.bit_count = 2;
            for (port = 0; port < VTSS_SGPIO_PORTS; port++) 
                conf.port_conf[port].enabled = (port < 5);// || port > 19 ? 1 : 0);
//            conf.port_conf[30].mode[0] = VTSS_SGPIO_MODE_ON; /* SFP0 Tx enable */
//            conf.port_conf[31].mode[0] = VTSS_SGPIO_MODE_ON; /* SFP1 Tx enable */
        }
		(void)vtss_sgpio_conf_set(NULL, 0, 0, &conf);
	}

}

/*
 * Lu26/Lu10 board probe function.
 */
//TODO: fix for VNS function names! (leave this one the same....)
BOOL vtss_board_probe_lu26(vtss_board_t *board, vtss_board_info_t *board_info)
{
    /*lint -esym(459, i2c_read, board_type) */
//    u32 chip_port;
    memset(board, 0, sizeof(*board));
    i2c_read   = board_info->i2c_read;

    board_type = board_info->board_type;

    if (board_type != VTSS_BOARD_ESIR_REF ) {
        board_type = VTSS_BOARD_ESIR_REF;
    }

    board->type = board_type;
    board_info->board_type = board_type;
    board->features = 
        VTSS_BOARD_FEATURE_AMS |
        VTSS_BOARD_FEATURE_LOS |
        VTSS_BOARD_FEATURE_VCXO;

    board->name = "eSIR";

    board->custom_port_table = lu26_port_table;

    board->init = lu26_init;
    board->reset = lu26_reset;
    board->pre_reset = lu26_pre_reset;
    board->post_reset = lu26_post_reset;
    board->led_init = lu26_led_init;
    board->led_update = lu26_led_update;
    board->port_conf = lu26_port_conf;
    board->sfp_i2c_read = lu26_sfp_i2c_read;
    board->sfp_update_if = lu26_sfp_update_if;
    board->sfp_mod_detect = lu26_sfp_mod_detect;
    board->sfp_i2c_enable = lu26_sfp_i2c_enable;
    board->sfp_i2c_lock = 0;
    board->sfp_accept = lu26_sfp_accept;

    /* The following settings are based on board layout and measurements of the L26 Ref board */
    board_info->serdes.serdes1g_vdd = VTSS_VDD_1V0; 
    board_info->serdes.serdes6g_vdd = VTSS_VDD_1V0; 
    board_info->serdes.ib_cterm_ena = 1;            
    board_info->serdes.qsgmii.ob_post0 = 2;         
    board_info->serdes.qsgmii.ob_sr = 0;            
    
    vtss_port_no_t port_idx;

    port_idx = 0;
    /* UNUSED PORT */
    lu26_port_table[port_idx].map.chip_port = 0;
    lu26_port_table[port_idx].map.miim_controller = VTSS_MIIM_CONTROLLER_0;
    lu26_port_table[port_idx].map.miim_addr = 0;
    lu26_port_table[port_idx].mac_if = VTSS_PORT_INTERFACE_SGMII;
    lu26_port_table[port_idx].cap = PORT_CAP_TRI_SPEED_COPPER;

//     port_idx = 0;
     port_idx++;
     lu26_port_table[port_idx].map.chip_port = 3;
     lu26_port_table[port_idx].map.miim_controller = VTSS_MIIM_CONTROLLER_0;
     lu26_port_table[port_idx].map.miim_addr = 3;
     lu26_port_table[port_idx].mac_if = VTSS_PORT_INTERFACE_SGMII;
     lu26_port_table[port_idx].cap = PORT_CAP_TRI_SPEED_COPPER;

//     port_idx = 1;
     port_idx++;
     lu26_port_table[port_idx].map.chip_port = 4;
     lu26_port_table[port_idx].map.miim_controller = VTSS_MIIM_CONTROLLER_0;
     lu26_port_table[port_idx].map.miim_addr = 4;
     lu26_port_table[port_idx].mac_if = VTSS_PORT_INTERFACE_SGMII;
     lu26_port_table[port_idx].cap = PORT_CAP_DUO_SPEED_COPPER;

//     port_idx = 2;
     port_idx++;
     lu26_port_table[port_idx].map.chip_port = 5;
     lu26_port_table[port_idx].map.miim_controller = VTSS_MIIM_CONTROLLER_0;
     lu26_port_table[port_idx].map.miim_addr = 5;
     lu26_port_table[port_idx].mac_if = VTSS_PORT_INTERFACE_SGMII;
     lu26_port_table[port_idx].cap = PORT_CAP_TRI_SPEED_COPPER;

//     port_idx = 3;
     port_idx++;
     lu26_port_table[port_idx].map.chip_port = 6;
     lu26_port_table[port_idx].map.miim_controller = VTSS_MIIM_CONTROLLER_0;
     lu26_port_table[port_idx].map.miim_addr = 6;
     lu26_port_table[port_idx].mac_if = VTSS_PORT_INTERFACE_SGMII;
     lu26_port_table[port_idx].cap = PORT_CAP_TRI_SPEED_COPPER;

//     port_idx = 4;
     port_idx++;
     lu26_port_table[port_idx].map.chip_port = 7;
     lu26_port_table[port_idx].map.miim_controller = VTSS_MIIM_CONTROLLER_0;
     lu26_port_table[port_idx].map.miim_addr = 7;
     lu26_port_table[port_idx].mac_if = VTSS_PORT_INTERFACE_SGMII;
     lu26_port_table[port_idx].cap = PORT_CAP_DUO_SPEED_COPPER;




    return TRUE;
}

#endif /* defined(VTSS_ARCH_LUTON26) */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/


