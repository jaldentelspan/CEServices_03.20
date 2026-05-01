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

#include "vtss_api.h"
#include "vtss_appl.h"

#if defined(BOARD_SERVAL_REF)

/* ================================================================= *
 *  Register access
 * ================================================================= */

#if defined (IO_METHOD_UIO)

#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <asm/byteorder.h>

#if !defined(UIO_DEV)
#define UIO_DEV "/dev/uio0"
#endif

#define MMAP_SIZE   0x02000000
#define CHIPID_OFF (0x01070000 >> 2)
#define ENDIAN_OFF (0x01000000 >> 2)

#define HWSWAP_BE 0x81818181       /* Big-endian */
#define HWSWAP_LE 0x00000000       /* Little-endian */

#define false 0
#define true  1

static int fd;
static int do_swap = false;

static const char *iodev = UIO_DEV;
static volatile u_char *base_mem;

static vtss_rc reg_read(const vtss_chip_no_t chip_no,
                        const u32            addr,
                        u32                  *const value)
{
    if(!do_swap) {
        *value =               ((volatile u32 *) (base_mem))[addr];
    } else {
        *value = __le32_to_cpu(((volatile u32 *) (base_mem))[addr]);
    }
    return VTSS_RC_OK;
}

static vtss_rc reg_write(const vtss_chip_no_t chip_no,
                         const u32            addr,
                         const u32            value)
{
    if(!do_swap) {
        ((volatile u32 *) (base_mem))[addr] =              (value);
    } else {
        ((volatile u32 *) (base_mem))[addr] = __cpu_to_le32(value);
    }
    return VTSS_RC_OK;
}

static void board_io_init(void)
{
    u32 value;

    /* Open the UIO device file */
    fprintf(stderr, "Using UIO: %s\n", iodev);
    fd = open(iodev, O_RDWR);
    if (fd < 1) {
        perror(iodev);
        return;
    }

    /* mmap the UIO device */
    base_mem = mmap(NULL, MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(base_mem != MAP_FAILED) {
        fprintf(stderr, "Mapped register memory @ %p\n", base_mem);
        if(!do_swap) {
            fprintf(stderr, "UIO: Enabling hw-level BE swapping - value %08x\n", HWSWAP_BE);
            reg_write(0, ENDIAN_OFF, HWSWAP_BE);
        } else {
            fprintf(stderr, "UIO: Enabling normal PCIe byteorder value %08x\n", HWSWAP_LE);
            reg_write(0, ENDIAN_OFF, HWSWAP_LE);
        }
        reg_read(0, CHIPID_OFF, &value);
        fprintf(stderr, "Chipid: %08x\n", value);
    } else {
        perror("mmap");
    }
}

#else

#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/vitgenio.h>

#include <string.h>
#include <sys/types.h>
#undef __USE_EXTERN_INLINES /* Avoid warning */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>

static int fd;

/* Global frame buffers */
#define VRAP_LEN  28
#define FRAME_MAX 2000
static u8 req_frame[VRAP_LEN];
static u8 rep_frame[FRAME_MAX];

static struct sockaddr_ll sock_addr;

static void frame_dump(const char *name, u8 *frame, int len)
{
    u32  i;
    char buf[100], *p = &buf[0];
    
    T_D("%s:", name);
    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            p = &buf[0];
            p += sprintf(p, "%04x: ", i);
        }
        p += sprintf(p,"%02x%c", frame[i], ((i + 9) % 16) == 0 ? '-' : ' ');
        if (((i + 1) % 16) == 0 || (i + 1) == len) {
            T_D("%s", buf);
        }
    }
}

static vtss_rc board_rd_wr(const u32 addr, u32 *const value, int write)
{
    u32     val = (write ? *value : 0);
    ssize_t n;
    BOOL    dump = 0;

    /* Address */
    req_frame[20] = ((addr >> 22) & 0xff);
    req_frame[21] = ((addr >> 14) & 0xff);
    req_frame[22] = ((addr >> 6) & 0xff);
    req_frame[23] = (((addr << 2) & 0xff) | (write ? 2 : 1));
    
    //dump = (req_frame[21] == 7 ? 1 : 0); /* DEVCPU_GCB */
    
    if (dump)
        T_D("%s, addr: 0x%08x, value: 0x%08x", write ? "WR" : "RD", addr, val);

    /* Value, only needed for write operations */
    req_frame[24] = ((val >> 24) & 0xff);
    req_frame[25] = ((val >> 16) & 0xff);
    req_frame[26] = ((val >> 8) & 0xff);
    req_frame[27] = (val & 0xff);
    
    n = VRAP_LEN;
    if (sendto(fd, req_frame, n, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
        T_E("sendto failed");
        return VTSS_RC_ERROR;
    }
    if (dump)
        frame_dump("VRAP Request", req_frame, n);

    if ((n = recvfrom(fd, rep_frame, FRAME_MAX, 0, NULL, NULL)) < 0) {
        T_E("recvfrom failed");
        return VTSS_RC_ERROR;
    }
    if (dump)
        frame_dump("VRAP Reply", rep_frame, n);
    
    if (!write) {
        *value = ((rep_frame[20] << 24) | (rep_frame[21] << 16) | 
                  (rep_frame[22] << 8) | rep_frame[23]);
    }

    return VTSS_RC_OK;
}

static vtss_rc reg_read(const vtss_chip_no_t chip_no,
                        const u32            addr,
                        u32                  *const value)
{
    return board_rd_wr(addr, value, 0);
}

static vtss_rc reg_write(const vtss_chip_no_t chip_no,
                         const u32            addr,
                         const u32            value)
{
    u32 val = value;
    return board_rd_wr(addr, &val, 1);
}

static void board_io_init(void)
{
    struct ifreq ifr;
    int    protocol = htons(0x8880);

    /* Open socket */
    if ((fd = socket(AF_PACKET, SOCK_RAW, protocol)) < 0) {
        T_E("socket create failed");
        return;
    }

    /* Get ifIndex */
    strcpy(ifr.ifr_name, "eth0");
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        T_E("SIOCGIFINDEX failed");
        return;
    }

    /* Initialize socket address */
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sll_family = htons(AF_PACKET);
    sock_addr.sll_protocol = protocol;
    sock_addr.sll_halen = ETH_ALEN;
    sock_addr.sll_ifindex = ifr.ifr_ifindex;

    /* Get SMAC */
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        T_E("SIOCGIFHWADDR failed");
        return;
    }

    /* Initialize request frame */
    req_frame[0] = 0x00;
    req_frame[1] = 0x01; /* DMAC 00-01-00-00-00-00 */
    req_frame[2] = 0x00;
    req_frame[3] = 0x00;
    req_frame[4] = 0x00;
    req_frame[5] = 0x00;
    memcpy(&req_frame[6], ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    req_frame[12] = 0x88;
    req_frame[13] = 0x80; 
    req_frame[15] = 0x04; /* EPID 0x0004 */
    req_frame[16] = 0x10; /* VRAP request */
    
    /* Copy DMAC */
    memcpy(&sock_addr.sll_addr, req_frame, ETH_ALEN);
}

#endif

/* Board port map */
static vtss_port_map_t port_map[VTSS_PORT_ARRAY_SIZE];

static vtss_port_interface_t port_interface(vtss_port_no_t port_no)
{
    return (port_map[port_no].miim_controller == VTSS_MIIM_CONTROLLER_NONE ? 
            VTSS_PORT_INTERFACE_SERDES : VTSS_PORT_INTERFACE_SGMII);
}

static BOOL port_poll(vtss_port_no_t port_no)
{
    /* Avoid polling NPI port */
    return (port_no == 10 ? 0 : 1);
}

/* ================================================================= *
 *  Board init.
 * ================================================================= */

static void board_init_post(vtss_board_t *board)
{
    /* NPI port is always forwarding */
    vtss_port_state_set(NULL, 10, 1);

    /* Enable GPIO_9 and GPIO_10 as MIIM controllers */
    vtss_gpio_mode_set(NULL, 0, 9, VTSS_GPIO_ALT_1);
    vtss_gpio_mode_set(NULL, 0, 10, VTSS_GPIO_ALT_1);

    /* Tesla pre reset */
    vtss_phy_pre_reset(NULL, 0);
}

static void board_init_done(vtss_board_t *board)
{
    /* Tesla post reset */
    vtss_phy_post_reset(NULL, 0);
}

static int board_init(int argc, const char **argv, vtss_board_t *board)
{
    vtss_port_no_t  port_no;
    vtss_port_map_t *map;
    
    board_io_init();
    board->board_init_post = board_init_post;
    board->board_init_done = board_init_done;
    board->port_map = port_map;
    board->port_interface = port_interface;
    board->port_poll = port_poll;
    board->init.init_conf->reg_read = reg_read;
    board->init.init_conf->reg_write = reg_write;

    /* Setup port map and calculate port count */
    board->port_count = VTSS_PORTS;
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORT_NO_END; port_no++) {
        map = &port_map[port_no];

        if (port_no < 4) {
            /* Port 0-3: Copper ports */
            map->chip_port = (7 - port_no);
            map->miim_controller = VTSS_MIIM_CONTROLLER_1;
            map->miim_addr = (16 + port_no);
        } else if (port_no < 8) {
            /* Port 4-7: 1G SFP */
            map->chip_port = (7 - port_no);
            map->miim_controller = VTSS_MIIM_CONTROLLER_NONE;
        } else if (port_no < 10) {
            /* Port 8-9: 2.5G SFP */
            map->chip_port = port_no;
            map->miim_controller = VTSS_MIIM_CONTROLLER_NONE;
        } else {
            /* Port 10: NPI port */
            map->chip_port = 10;
            map->miim_controller = VTSS_MIIM_CONTROLLER_1;
            map->miim_addr = 28;
        }
    }

    return 0;
}

void vtss_board_serval_ref_init(vtss_board_t *board)
{
    board->descr = "Serval";
    board->target = VTSS_TARGET_SERVAL;
    board->feature.port_control = 1;
    board->feature.layer2 = 1;
    //board->feature.packet = 1;
    board->board_init = board_init;
}
#endif /* BOARD_SERVAL_REF */
