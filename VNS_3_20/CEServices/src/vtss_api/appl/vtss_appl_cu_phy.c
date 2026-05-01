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



#include "vtss_api.h"   // For board initialization
#include "vtss_appl.h"  // For vtsss_board_t 


/* ================================================================= *
 *  Board init.
 * ================================================================= */


/* Board specifics */
static vtss_board_t board_table[1];

BOOL port_is_cu_phy(vtss_port_no_t port_no)
{
#if defined(VTSS_CHIP_10G_PHY)
    if (port_no > 10) { /* Assuming that ports 11 and 12 are 10G ports */
        return 0;
    }
#endif    
    return 1;
}

BOOL port_is_10g_phy(vtss_port_no_t port_no)
{
#if defined(VTSS_CHIP_10G_PHY)
    if (port_no > 10) { /* Assuming that ports 11 and 12 are 10G ports */
        return 1;
    }
#endif    
    return 0;
}





// Example of how to use the loop back function
void phy_loop_back(void) {
    vtss_phy_loopback_t loopback;
    vtss_port_no_t port_no;

    loopback.near_end_enable = FALSE;  // Set to TRUE for enabling Near End loopback 
    loopback.far_end_enable  = TRUE;  // Set to TRUE for enabling Far End loopback 
    
    // Setup the loopback
   for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORTS; port_no++) {
       vtss_phy_loopback_set(NULL, port_no, loopback);
   }



    // Example of getting the current loopback settings
   for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORTS; port_no++) {
       vtss_phy_loopback_get(NULL, port_no, &loopback);
       T_I("loopback for port:%d near_end_loopback=%d, far_end_loopback = %d \n",
           port_no,
           loopback.near_end_enable,
           loopback.far_end_enable);
   }
} 


int main(int argc, const char **argv) {
     int  portno, n;
    vtss_board_t            *board;
    vtss_inst_create_t      create;
    vtss_port_no_t          port_no;
    vtss_phy_reset_conf_t   phy_reset;
    vtss_init_conf_t        init_conf;
    vtss_port_interface_t   mac_if;
    vtss_phy_media_interface_t media_if;
#if defined(VTSS_CHIP_10G_PHY)
    vtss_phy_10g_mode_t     oper_mode;
#endif /* VTSS_CHIP_10G_PHY */

    // Setup trace level for PHY group
    vtss_trace_conf_t vtss_appl_trace_conf = {.level = VTSS_TRACE_LEVEL_INFO };
    vtss_trace_conf_set(VTSS_TRACE_GROUP_PHY, &vtss_appl_trace_conf);

    
    // In this case we only have one board. Assign point to the board definition
    board = &board_table[0];    


    // Initialize 
    vtss_board_phy_init(board);   

    // "Create" the board
    vtss_inst_get(board->target, &create);
    vtss_inst_create(&create, &board->inst);
    vtss_init_conf_get(board->inst, &init_conf);


    

    printf ("//Comment: Setup of VSC8574 for  Auto neg.  and SGMII\n");
    printf ("//Comment: miim_read format  : miim_read(port_number, address) \n");
    printf ("//Comment: miim_write format : miim_write(port_number, address, value(hex)) \n");
    printf ("\n\n");



    board->init.init_conf = &init_conf;
    if (board->board_init(argc, argv, board)) {
        T_E("Could not initialize board");
        return 1;
    } else {
        printf ("//Comment: Board being initialized\n");
    }



    if (vtss_init_conf_set(board->inst, &init_conf) == VTSS_RC_ERROR) {
        T_E("Could not initialize");
        return 1;
    };



    // Reset stuff needed before PHY port reset, any port will do (Only at start-up initialization) 
    printf ("//Comment: PHY pre-reset\n");
    vtss_phy_pre_reset(board->inst, 0);
   

    // Example for MAC interfaces
    mac_if = VTSS_PORT_INTERFACE_QSGMII;
    mac_if = VTSS_PORT_INTERFACE_SGMII;
//    mac_if = VTSS_PORT_INTERFACE_SERDES;

    // Example for Media interfaces
//  media_if = VTSS_PHY_MEDIA_IF_FI_1000BX;
//   media_if = VTSS_PHY_MEDIA_IF_AMS_FI_1000BX;    
    media_if = VTSS_PHY_MEDIA_IF_AMS_FI_100FX;    
    media_if = VTSS_PHY_MEDIA_IF_FI_100FX; 
    media_if = VTSS_PHY_MEDIA_IF_CU;    
//  media_if= VTSS_PHY_MEDIA_IF_SFP_PASSTHRU;

    vtss_phy_conf_t         phy;

    // Example for PHY moode  
    phy.mode = VTSS_PHY_MODE_FORCED;
    phy.mode = VTSS_PHY_MODE_ANEG;

    // Example for PHY speed support for auto-neg 
    phy.aneg.speed_10m_hdx = 1;
    phy.aneg.speed_10m_fdx = 1;
    phy.aneg.speed_100m_hdx = 1;
    phy.aneg.speed_100m_fdx = 1;
    phy.aneg.speed_1g_fdx = 1;

    // Example for PHY flow control settings
    phy.aneg.symmetric_pause =  1;
    
    // Example for PHY speed (non-auto neg.)
    phy.forced.speed = VTSS_SPEED_1G;
    phy.forced.speed = VTSS_SPEED_100M;
    phy.forced.speed = VTSS_SPEED_10M;
//    phy.forced.fdx = 1;

    phy.mdi = VTSS_PHY_MDIX_AUTO; // always enable auto detection of crossed/non-crossed cables
    
    // Do PHY port reset + Port setup
    printf ("//Comment: PHY port reset\n");
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORTS; port_no++) {
        phy_reset.mac_if = mac_if; // Set the port interface
        phy_reset.media_if = media_if;         // Set media interface to CU
        vtss_phy_reset(board->inst, port_no, &phy_reset); // Do port reset
        vtss_phy_conf_set(board->inst, port_no, &phy);
    }



    // Do reset stuff that is needed after port reset, any port will do (Only at start-up initialization) 
    printf ("//Comment: PHY post-reset\n");
    vtss_phy_post_reset(board->inst, 0);

#if defined(VTSS_CHIP_10G_PHY)
    for (port_no = VTSS_PORT_NO_START; port_no < VTSS_PORTS; port_no++) {
        if(port_is_10g_phy(port_no)) {
            if (vtss_phy_10g_mode_set(board->inst, port_no, &oper_mode) == VTSS_RC_ERROR) {
                T_E("Could not initialize 10G phy");
                return 1;
            }
        }
    }
#endif /* VTSS_CHIP_10G_PHY */
    phy_loop_back(); // Example for loopbacks 

    int value;

    char command[255];
    char port_no_str[255];
    char addr_str[255];
    char value_str[255];
    BOOL read_cmd = TRUE;
    int addr;

    while (1) {
        scanf("%s", &command[0]);
        if (strcmp(command, "?")  == 0) {
            printf ("**** Help ****");
            printf ("The following command is support\n");
            printf ("rd <port_no> <addr>\n");
            printf ("wr <port_no> <addr> <value> - Value MUST be in hex\n");
            printf ("status <port_no>  - Return PHY API status \n");
            continue;
        } else if (strcmp(command, "rd")  == 0) {
            read_cmd = TRUE;
        } else if (strcmp(command, "wr")  == 0) {
            read_cmd = FALSE;
        } else if (strcmp(command, "status")  == 0) {
            vtss_port_status_t status;
            scanf("%s", &port_no_str[0]);
            port_no = atoi(port_no_str);
            
            vtss_phy_status_get(NULL, port_no, &status);   
            printf("Link down:%d \n", status.link_down);
            continue;

        } else {
            printf ("Unknown command:%s \n", command);    
            continue;
        }

        scanf("%s", &port_no_str[0]);
        port_no = atoi(port_no_str);

        scanf("%s", &addr_str[0]);
        addr = atoi(addr_str);

        if (read_cmd) {
            miim_read(port_no, addr, &value);
            printf("%02d: 0x%04X \n", addr, value);
        } else {
            scanf("%X", &value);
            miim_write(port_no, addr, value);
            miim_read(port_no, addr, &value);
            printf("New register value for %02d: 0x%04X \n", addr, value);

        }
    
    }
    return 0; // All done 
}
