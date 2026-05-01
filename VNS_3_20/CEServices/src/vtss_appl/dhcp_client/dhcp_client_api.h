/*

 Vitesse software.

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
#ifndef __vtss_dhcp_client_API_H__
#define __vtss_dhcp_client_API_H__

#include "vtss_types.h"

#define VTSS_DHCP_MAX_OFFERS                     4
#define VTSS_DHCP_OPTION_CODE_TIME_SERVER        4
#define VTSS_DHCP_OPTION_CODE_DOMAIN_NAME_SERVER 6

vtss_rc vtss_dhcp_client_init(vtss_init_data_t *data);
const char * dhcp_client_error_txt(vtss_rc rc);

typedef enum {
    DHCP4C_STATE_STOPPED,
    DHCP4C_STATE_INIT,
    DHCP4C_STATE_SELECTING,
    DHCP4C_STATE_REQUESTING,
    DHCP4C_STATE_REBINDING,
    DHCP4C_STATE_BOUND,
    DHCP4C_STATE_RENEWING
} vtss_dhcp4c_state_t;

const char * vtss_dhcp4c_state_to_txt(vtss_dhcp4c_state_t s);

typedef struct {
    vtss_ipv4_t server_ip;
    vtss_mac_t  server_mac;
    vtss_ipv4_network_t ip;
} vtss_dhcp_client_offer_t;

typedef struct {
    unsigned valid_offers;
    vtss_dhcp_client_offer_t list[VTSS_DHCP_MAX_OFFERS];
} vtss_dhcp_client_offer_list_t;

typedef struct {
    vtss_dhcp4c_state_t state;
    vtss_ipv4_t server_ip;
    vtss_dhcp_client_offer_list_t offers;
} vtss_dhcp_client_status_t;

int vtss_dhcp4c_status_to_txt(char                            *buf,
                              int                              size,
                              const vtss_dhcp_client_status_t *const st);

/* CONTROL AND STATE ------------------------------------------------- */

/* Start the DHCP client on the given VLAN */
vtss_rc vtss_dhcp_client_start(       vtss_vid_t                  vlan);

/* Stop the DHCP client on the given VLAN */
vtss_rc vtss_dhcp_client_stop(        vtss_vid_t                  vlan);

/* Check if the DHCP client is bound */
BOOL vtss_dhcp_client_bound_get(      vtss_vid_t                  vlan);

/* Inspect the list of received offers */
vtss_rc vtss_dhcp_client_offers_get(  vtss_vid_t                  vlan,
                                      vtss_dhcp_client_offer_list_t *list);

/* Accept one of the received offers */
vtss_rc vtss_dhcp_client_offer_accept(vtss_vid_t                  vlan,
                                      unsigned idx);

/* Get status */
vtss_rc vtss_dhcp_client_status(      vtss_vid_t                  vlan,
                                      vtss_dhcp_client_status_t  *status);

typedef void (*vtss_dhcp_client_callback_t)(vtss_vid_t);
vtss_rc vtss_dhcp_client_callback_add(vtss_vid_t                  vlan,
                                      vtss_dhcp_client_callback_t cb);
vtss_rc vtss_dhcp_client_callback_del(vtss_vid_t                  vlan,
                                      vtss_dhcp_client_callback_t cb);

/* Native protocol fields */
/* All native protocol fields will fail if we are not bound */
typedef struct {
    vtss_ipv4_t ciaddr;
    vtss_ipv4_t yiaddr;
    vtss_ipv4_t siaddr;
    vtss_ipv4_t giaddr;
    vtss_mac_t  chaddr;
    char        sname[64];
    char        file[128];
} vtss_dhcp_fields_t;

vtss_rc vtss_dhcp_client_fields_get(  vtss_vid_t                  vlan,
                                      vtss_dhcp_fields_t         *fields);

/* If netmask is present as an option, then it will be used to build
 * the prefix. Otherwise the prefix will be derived according to the
 * network class */
vtss_rc vtss_dhcp_client_ip_get(      vtss_vid_t                  vlan,
                                      vtss_ipv4_network_t        *ip);


/* OPTION-ACCESS ----------------------------------------------------- */
/* All option access API calls will fail if we are not bound */

/* Get an arbitary DHCP option */
vtss_rc vtss_dhcp_client_option_get(  vtss_vid_t                  vlan,
                                      u8                          option_code,
                                      unsigned                    buf_length,
                                      u8                         *buf);

/* Scan all the dhcp clients to see if any of them has got a given option. If
 * the prefered is set to anything else than 0.0.0.0, the function will scan for
 * the preferred and return it if found, other wise a random ip will be
 * returned.*/
vtss_rc
vtss_dhcp_client_option_ip_any_get(   u8                          option_code,
                                      vtss_ipv4_t                 prefered,
                                      vtss_ipv4_t                *ip);


/* Get an arbitary DHCP option and decode option payload as an IP
 * address. Warning only length checks are performed, it will cast any
 * payload to an IP address */
vtss_rc
vtss_dhcp_client_option_ip_get(       vtss_vid_t                  vlan,
                                      u8                          option_code,
                                      vtss_ipv4_t                *ip);

/* Like vtss_dhcp_client_option_ip_get, but assumes a list of IP addresses.
 * will return VTSS_RC_ERROR when no more addresses are aviable */
vtss_rc
vtss_dhcp_client_option_ipn_get(      vtss_vid_t                  vlan,
                                      u8                          option_code,
                                      u8                          option_number,
                                      vtss_ipv4_t                *ip);

/* The packet handler for the DHCP packet */
BOOL vtss_dhcp_client_packet_handler(void *contxt, const u8 *const frm,
                                     const vtss_packet_rx_info_t *const rx_info);

vtss_rc vtss_dhcp_client_release(vtss_vid_t vlan);
vtss_rc vtss_dhcp_client_decline(vtss_vid_t vlan);

#endif /* __vtss_dhcp_client_API_H__ */
