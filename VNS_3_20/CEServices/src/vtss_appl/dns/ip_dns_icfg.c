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

#include "icfg_api.h"
#include "misc_api.h"
#ifndef VTSS_SW_OPTION_IP2
#include "ip_api.h"
#endif /* VTSS_SW_OPTION_IP2 */
#include "ip_dns_api.h"
#include "ip_dns_icfg.h"


/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_IP_DNS
#define IP_DNS_ICFG_REG(x, y, z)    (((x) = vtss_icfg_query_register((y), (z))) == VTSS_OK)

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/
/* ICFG callback functions */
static vtss_rc _ip_dns_icfg_state_ctrl(const vtss_icfg_query_request_t *req,
                                       vtss_icfg_query_result_t *result)
{
    vtss_rc     rc = VTSS_OK;
    BOOL        dns_proxy;
#ifdef VTSS_SW_OPTION_IP2
    vtss_ipv4_t dns_srv;
#else
    ip_conf_t   ipcfg;
#endif /* VTSS_SW_OPTION_IP2 */

    if (req && result) {
        /*
            COMMAND = ip name-server <ipv4_ucast>
            COMMAND = ip dns proxy
        */
#ifdef VTSS_SW_OPTION_IP2
        if (ip_dns_mgmt_get_server4(&dns_srv) == VTSS_OK) {
            if (req->all_defaults || dns_srv) {
                if (dns_srv) {
                    char    buf[40];

                    rc = vtss_icfg_printf(result, "ip name-server %s\n",
                                          misc_ipv4_txt(dns_srv, buf));
                } else {
                    rc = vtss_icfg_printf(result, "no ip name-server\n");
                }
            }
#else
        if (ip_mgmt_conf_get(&ipcfg) == VTSS_OK) {
            if (req->all_defaults || ipcfg.ipv4_dns) {
                if (ipcfg.ipv4_dns) {
                    char    buf[40];

                    rc = vtss_icfg_printf(result, "ip name-server %s\n",
                                          misc_ipv4_txt(ipcfg.ipv4_dns, buf));
                } else {
                    rc = vtss_icfg_printf(result, "no ip name-server\n");
                }
            }
#endif /* VTSS_SW_OPTION_IP2 */
        } else {
            if (req->all_defaults) {
                rc = vtss_icfg_printf(result, "no ip name-server\n");
            }
        }

        if (ip_dns_mgmt_get_proxy_status(&dns_proxy) == VTSS_OK) {
            if (req->all_defaults ||
                (dns_proxy != VTSS_DNS_PROXY_DEF_STATE)) {
                rc = vtss_icfg_printf(result, "%sip dns proxy\n",
                                      dns_proxy ? "" : "no ");
            }
        } else {
            if (req->all_defaults) {
                rc = vtss_icfg_printf(result, "no ip dns proxy\n");
            }
        }
    }

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/
/* Initialization function */
vtss_rc ip_dns_icfg_init(void)
{
    vtss_rc rc;

    /* Register callback functions to ICFG module. */
    if (IP_DNS_ICFG_REG(rc, VTSS_ICFG_IP_DNS_CONF, _ip_dns_icfg_state_ctrl)) {
        T_I("ip_dns ICFG done");
    }

    return rc;
}
