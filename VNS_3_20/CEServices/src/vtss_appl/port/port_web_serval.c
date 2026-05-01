/*

   Vitesse Switch API software.

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
 */

#include "web_api.h"
#include "port_api.h"
#include "port_custom_api.h"
#include "vtss_api_if_api.h"
#include "msg_api.h"

/* Stock resources in sw_mgd_html/images/jack_.... */
#define PORT_ICON_WIDTH     32
#define PORT_ICON_HEIGHT    23
#define PORT_BLOCK_GAP      12

static void port_httpd_write(CYG_HTTPD_STATE *p, vtss_uport_no_t uport, int sfp, char *state,
                             char *speed, int xoff, int row)
{
    int ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%u/jack_%s_%s_%s.png/Port %u: %s/%u/%u/%u/%u/%d|",
                      uport,
                      sfp ? "sfp" : "copper",
                      state,
                      row && !sfp ? "top" : "bottom",
                      uport,
                      speed,
                      xoff,
                      PORT_ICON_HEIGHT*(2 - row),
                      PORT_ICON_WIDTH,
                      PORT_ICON_HEIGHT,
                      row ? -1 : 1);
    cyg_httpd_write_chunked(p->outbuffer, ct);
}

/*
 * The (basic) portstate handler
 */

void stat_portstate_switch(CYG_HTTPD_STATE* p, vtss_usid_t usid, vtss_isid_t isid)
{
    vtss_port_no_t     iport;
    vtss_uport_no_t    uport;
    port_conf_t        conf;
    port_status_t      port_status;
    vtss_port_status_t *ps = &port_status.status;
    BOOL               fiber;
    char               *state, *speed;
    int                ct, xoff_add, xoff, xoff_sfp = 0, row, sfp, pcb106;
    vtss_board_info_t  board_info;

    /* Board type */
    vtss_board_info_get(&board_info);
    pcb106 = (board_info.board_type == VTSS_BOARD_SERVAL_PCB106_REF ? 1 : 0);

    /* Backround, SID, managed */
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%s|", usid, "switch_small.png");
    cyg_httpd_write_chunked(p->outbuffer, ct);

    /* Emit port icons */
    xoff = (pcb106 ? (PORT_ICON_WIDTH + PORT_BLOCK_GAP) : 0);
    for (iport = VTSS_PORT_NO_START; iport < board_info.port_count; iport++) {
        uport = iport2uport(iport);
        if (port_mgmt_conf_get(isid, iport, &conf) < 0 ||
            port_mgmt_status_get(isid, iport, &port_status) < 0)
            break;

        fiber = port_status.fiber;
        state = (conf.enable ? ((ps->link && !fiber) ? "link" : "down") : "disabled");
        speed = (conf.enable ?
                 (ps->link ? port_mgmt_mode_txt(ps->speed, ps->fdx, fiber) : "Down") : "Disabled");

        sfp = 0;
        row = 0;
        xoff_add = PORT_ICON_WIDTH;
        if (iport == (board_info.port_count - 1)) {
            /* NPI port */
            if (pcb106)
                xoff = 0;
            else
                xoff_add += PORT_BLOCK_GAP;
        } else if (port_status.cap & PORT_CAP_10M_HDX) {
            /* Copper port */
            xoff_sfp = PORT_BLOCK_GAP;
            if (pcb106 && (iport & 1)) {
                /* Odd ports on PCB106 are in the top row */
                row = 1;
                xoff_add = 0;
            }
        } else {
            /* SFP port */
            sfp = 1;
            if (pcb106) {
                if (port_status.cap & PORT_CAP_2_5G_FDX) {
                    /* SFP 2.5G ports are on the bottom row */
                    xoff_add += ((iport & 1) ? 0 : PORT_BLOCK_GAP);
                } else if (iport & 1) {
                    /* Odd ports on PCB106 are in the top row */
                    row = 1;
                    xoff_add = 0;
                }
            }
            if (xoff_sfp) {
                xoff_add += xoff_sfp;
                xoff_sfp = 0;
            }
        }
        xoff += xoff_add;
        port_httpd_write(p, uport, sfp, state, speed, xoff, row);
        
        if (port_status.cap & PORT_CAP_DUAL_SFP_DETECT) {
            /* Dual media fiber port */
            sfp = 1;
            state = (conf.enable ? ((fiber && ps->link) ? "link" : "down") : "disabled");
            xoff_sfp = (2 * PORT_ICON_WIDTH + PORT_BLOCK_GAP);
            port_httpd_write(p, uport, sfp, state, speed, xoff + xoff_sfp, row);
        }
    }
    cyg_httpd_write_chunked(",", 1);
}

