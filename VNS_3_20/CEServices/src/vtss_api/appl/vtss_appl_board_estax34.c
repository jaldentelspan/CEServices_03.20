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
#include "vtss_appl.h"

#if defined(BOARD_ESTAX_34_REF)

/* ================================================================= *
 *  Register access
 * ================================================================= */

#if defined(VTSS_OPSYS_LINUX)
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/vitgenio.h>

static int fd;

static void board_io_init(void)
{
#if !VTSS_OPT_VCORE_II
    struct vitgenio_cpld_spi_setup timing = {
        /* char ss_select; Which of the CPLD_GPIOs is used for Slave Select */
        VITGENIO_SPI_SS_CPLD_GPIO0,
        VITGENIO_SPI_SS_ACTIVE_LOW, /* char ss_activelow; Slave Select (Chip Select) active low: true, active high: false */
        VITGENIO_SPI_CPOL_0, /* char sck_activelow; CPOL=0: false, CPOL=1: true */
        VITGENIO_SPI_CPHA_0, /* char sck_phase_early; CPHA=0: false, CPHA=1: true */
        VITGENIO_SPI_MSBIT_FIRST, /* char bitorder_msbfirst; */
        0, /* char reserved1; currently unused, only here for alignment purposes */
        0, /* char reserved2; currently unused, only here for alignment purposes */
        0, /* char reserved3; currently unused, only here for alignment purposes */
        500 /* unsigned int ndelay; minimum delay in nanoseconds, two of these delays are used per clock cycle */
    };
#endif /* VTSS_OPT_VCORE_II */

    if ((fd = open("/dev/vitgenio", 0)) < 0) {
        T_E("open /dev/vitgenio failed");
        return;
    }
    
#if !VTSS_OPT_VCORE_II
    ioctl(fd, VITGENIO_ENABLE_CPLD_SPI);
    ioctl(fd, VITGENIO_CPLD_SPI_SETUP, &timing);
#endif /* VTSS_OPT_VCORE_II */
}

static vtss_rc board_rd_wr(const u32 addr, u32 *const value, int write)
{
#if !VTSS_OPT_VCORE_II
    struct vitgenio_cpld_spi_readwrite buf;
  
    /* Convert to format: (block << 5) | (write << 4) | (sub << 0) */
    buf.buffer[0] = (((addr >> 7) & 0xe0) | (write << 4) | ((addr >> 8) & 0x0f));
    buf.buffer[1] = (addr & 0xff);
    if (write) {
        buf.buffer[2] = ((*value>>24) & 0xff);
        buf.buffer[3] = ((*value>>16) & 0xff);
        buf.buffer[4] = ((*value>>8) & 0xff);
        buf.buffer[5] = (*value & 0xff);
    } else {
        buf.buffer[2] = 0;
        buf.buffer[3] = 0;
        buf.buffer[4] = 0;
        buf.buffer[5] = 0;
    } 
    buf.length = 6;
    if (ioctl(fd, VITGENIO_CPLD_SPI_READWRITE, &buf) < 0) {
        T_E("SPI_READWRITE failed");
        return VTSS_RC_ERROR;
    }
    if (!write) {
        *value=((buf.buffer[2]<<24) | (buf.buffer[3]<<16) | 
                (buf.buffer[4]<<8) | (buf.buffer[5]<<0));
    }
#else
    struct vitgenio_32bit_readwrite readwrite;
    readwrite.offset = addr;
    if(write) {
        readwrite.value = *value;
        if (ioctl(fd, VITGENIO_32BIT_WRITE, &readwrite) < 0) {
            T_E("VITGENIO_32BIT_WRITE failed");
            return VTSS_RC_ERROR;
        }
    } else {
        if (ioctl(fd, VITGENIO_32BIT_READ, &readwrite) < 0) {
            T_E("VITGENIO_32BIT_READ failed");
            return VTSS_RC_ERROR;
        }
        *value = readwrite.value;
    }
#endif
    return VTSS_RC_OK;
}
#else
static volatile u32 *base_addr = (u32 *)0xa0000000;
#endif /* VTSS_OPSYS_LINUX */

static vtss_rc reg_read(const vtss_chip_no_t chip_no,
                        const u32            addr,
                        u32                  *const value)
{
#if defined(VTSS_OPSYS_LINUX)
    return board_rd_wr(addr, value, 0);
#else
    *value = base_addr[addr];
    return VTSS_RC_OK;
#endif /* VTSS_OPSYS_LINUX */
}

static vtss_rc reg_write(const vtss_chip_no_t chip_no,
                         const u32            addr,
                         const u32            value)
{
#if defined(VTSS_OPSYS_LINUX)
    u32 val = value;
    return board_rd_wr(addr, &val, 1);
#else
    base_addr[addr] = value;
    return VTSS_RC_OK;
#endif /* VTSS_OPSYS_LINUX */
}

#if defined(VTSS_FEATURE_PORT_CONTROL)
/* Board port map */
static vtss_port_map_t port_map[VTSS_PORT_ARRAY_SIZE];
#else
static vtss_rc l28_rd_wr(u32 blk, u32 sub, u32 reg, u32 *value, u8 write)
{
    u32 addr = ((blk<<12) | (sub<<8) | reg);

    return (write ? reg_write(0, addr, *value) : reg_read(0, addr, value));
}

static vtss_rc miim_read_write(BOOL read, vtss_port_no_t port_no, u8 addr, u16 *value)
{
    u32 blk = 3, sub, miim_addr, data;
    
    /* Simple port map */
    miim_addr = port_no;
    sub = (port_no < 8 ? 0 : 1);

    /* Enqueue MIIM operation to be executed */
    data = (((read ? 1 : 0)<<26) | (miim_addr<<21) | (addr<<16) | *value);
    l28_rd_wr(blk, sub, 0x01, &data, 1);

    /* Wait for MIIM operation to finish */
    while (1) {
        l28_rd_wr(blk, sub, 0x00, &data, 0);
        if (!(data & 0xf))
            break;
    }

    /* Read data */
    if (read) {
        l28_rd_wr(blk, sub, 0x02, &data, 0);
        if (data & (1<<16))
            return VTSS_RC_ERROR;
        *value = (data & 0xffff);
    }
    return VTSS_RC_OK;
}

static vtss_rc miim_read(const vtss_port_no_t port_no,
                         const u8             addr,
                         u16                  *const value)
{
    return miim_read_write(1, port_no, addr, value);
}

static vtss_rc miim_write(const vtss_port_no_t port_no,
                          const u8             addr,
                          const u16            value)
{
    u16 val = value;

    return miim_read_write(0, port_no, addr, &val);
}
#endif /* VTSS_FEATURE_PORT_CONTROL */

static vtss_port_interface_t port_interface(vtss_port_no_t port_no)
{
    return (port_no < 24 ? VTSS_PORT_INTERFACE_SGMII : VTSS_PORT_INTERFACE_VAUI);
}

/* ================================================================= *
 *  Board init.
 * ================================================================= */

static int board_init(int argc, const char **argv, vtss_board_t *board)
{
#if defined(VTSS_OPSYS_LINUX)
    board_io_init();
#endif /* VTSS_OPSYS_LINUX */
#if defined(VTSS_FEATURE_PORT_CONTROL)
    board->init.init_conf->reg_read = reg_read;
    board->init.init_conf->reg_write = reg_write;
#else
    {
        u32 value;

        value = 0x0c0c0c0c; /* PICONF: 16-bit width */
        l28_rd_wr(7, 0, 0x02, &value, 1);
        value = 0;          /* CPUCTRL: Use extended bus cycle */
        l28_rd_wr(7, 0, 0x30, &value, 1);
        board->init.init_conf->miim_read = miim_read;
        board->init.init_conf->miim_write = miim_write;
    }
#endif /* VTSS_FEATURE_PORT_CONTROL */

    board->port_count = VTSS_PORTS;
#if defined(VTSS_FEATURE_PORT_CONTROL)
    board->port_map = port_map;
    {
        vtss_port_no_t  port_no;
        vtss_port_map_t *map;

        /* Setup port map and update port count */
        for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
            map = &port_map[port_no];
            if (port_no < 24) {
                map->chip_port = port_no;
                map->miim_controller = (port_no < 8 ? VTSS_MIIM_CONTROLLER_0 : 
                                        VTSS_MIIM_CONTROLLER_1);
                map->miim_addr = port_no;
            } else if (port_no == 24) {
                map->chip_port = 24;
                map->miim_controller = VTSS_MIIM_CONTROLLER_NONE;
                map->miim_addr = -1;
            } else if (port_no == 25) {
                map->chip_port = 26;
                map->miim_controller = VTSS_MIIM_CONTROLLER_NONE;
                map->miim_addr = -1;
            } else {
                map->chip_port = CHIP_PORT_UNUSED;
                map->miim_controller = VTSS_MIIM_CONTROLLER_NONE;
                map->miim_addr = -1;
                if (port_no < board->port_count) {
                    board->port_count = port_no;
                }
            }
        }
    }
#endif /* VTSS_FEATURE_PORT_CONTROL */
    board->port_interface = port_interface;

    return 0;
}

static void board_init_post(vtss_board_t *board)
{
#if defined(VTSS_FEATURE_WARM_START)
    vtss_restart_status_t status;

    /* Avoid PHY reset if it is a warm start */
    if (vtss_restart_status_get(board->inst, &status) == VTSS_RC_OK && 
        status.restart == VTSS_RESTART_WARM)
        return;
#endif /* VTSS_FEATURE_WARM_START */
#if defined(VTSS_FEATURE_PORT_CONTROL)
    /* Release PHYs from reset (Switch GPIO9) */
    vtss_gpio_direction_set(board->inst, 0, 9, 1);
    vtss_gpio_write(board->inst, 0, 9, 0);
    vtss_gpio_write(board->inst, 0, 9, 1);
#endif /* VTSS_FEATURE_PORT_CONTROL */
    VTSS_MSLEEP(500);
}

void vtss_board_estax34_init(vtss_board_t *board)
{
#if defined(VTSS_FEATURE_PORT_CONTROL)
    board->descr = "E-StaX-34";
    board->target = VTSS_TARGET_E_STAX_34;
    board->feature.port_control = 1;
    board->feature.layer2 = 1;
    board->feature.packet = 1;
#else
    board->descr = "PHY";
    board->target = VTSS_TARGET_CU_PHY;
#endif /* VTSS_FEATURE_PORT_CONTROL */
    board->board_init = board_init;
    board->board_init_post = board_init_post;
}
#endif /* BOARD_ESTAX_34_REF */
