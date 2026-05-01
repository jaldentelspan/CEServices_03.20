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
#include "icli_api.h"
#include "icli_porting_util.h"
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"
#else
#include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */

#include "lacp_api.h"
#include "lacp_icfg.h"

#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_LACP

//******************************************************************************
// ICFG callback functions 
//******************************************************************************

static vtss_rc lacp_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    vtss_rc               rc = VTSS_OK;
    vtss_lacp_system_config_t    sysconf;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Global level: lacp system-priority <prio> */

    if (lacp_get_system_config(&sysconf) != VTSS_OK) {
        T_E("Could not get LACP sysconf\n");
        return VTSS_RC_ERROR;
    }

    if (req->all_defaults || sysconf.system_prio != 32768) {
        VTSS_RC(vtss_icfg_printf(result, "lacp system-priority %lu\n",sysconf.system_prio));
    }   
    return rc;
}

static vtss_rc lacp_icfg_intf_conf(const vtss_icfg_query_request_t *req,
                                   vtss_icfg_query_result_t *result)
{
    vtss_rc                  rc = VTSS_OK;
    vtss_isid_t              isid;
    vtss_port_no_t           iport;
    vtss_lacp_port_config_t  conf;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Interface level: lacp, lacp port-prio, lacp active|passive, lacp key <key>, lacp timeout fast|slow */

    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);

    if (lacp_get_port_config(isid, iport, &conf) != VTSS_OK) {
        T_E("Could not get LACP portconf\n");
        return VTSS_RC_ERROR;
    }

    if (conf.enable_lacp || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " %slacp\n",conf.enable_lacp?"":"no "));
    }

    if (conf.port_prio!=32768 || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " %slacp prio %lu\n",conf.port_prio==32768 ? "no " : "", conf.port_prio));
    }

    if (conf.active_or_passive != VTSS_LACP_ACTMODE_ACTIVE || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " %slacp role %s\n",conf.active_or_passive == VTSS_LACP_ACTMODE_ACTIVE ? "no " : "", 
                                 conf.active_or_passive == VTSS_LACP_ACTMODE_ACTIVE ? "active" : "passive"));
    }

    if (conf.port_key != VTSS_LACP_AUTOKEY || req->all_defaults) {
        if (conf.port_key == VTSS_LACP_AUTOKEY) {
            VTSS_RC(vtss_icfg_printf(result, " no lacp key auto\n"));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " lacp key %lu\n",conf.port_key));
        }
    }

    if (conf.xmit_mode == VTSS_LACP_FSMODE_SLOW || req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " %slacp role %s\n",conf.xmit_mode == VTSS_LACP_FSMODE_FAST ? "no " : "", 
                                 conf.xmit_mode == VTSS_LACP_FSMODE_FAST ? "fast":"slow"));
    }

    return rc;
}

//******************************************************************************
//   Public functions
//******************************************************************************

vtss_rc lacp_icfg_init(void)
{
    vtss_rc rc;

    /* Register callback functions to ICFG module. */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_LACP_INTERFACE_CONF, lacp_icfg_intf_conf)) != VTSS_OK) {
        return rc;
    }

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_LACP_GLOBAL_CONF, lacp_icfg_global_conf)) != VTSS_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}
