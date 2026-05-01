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

#include "aggr_api.h"
#include "aggr_icfg.h"

#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_AGGR

//******************************************************************************
// ICFG callback functions 
//******************************************************************************

static vtss_rc aggr_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    vtss_rc               rc = VTSS_OK;
    vtss_aggr_mode_t      mode;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Global level: aggregation mode */

    if ((rc=aggr_mgmt_aggr_mode_get(&mode)) != VTSS_OK) {
        T_E("%s\n",aggr_error_txt(rc));
        return 0;
    }

    if (req->all_defaults || 
        ((mode.smac_enable && !mode.dmac_enable && mode.sip_dip_enable && mode.sport_dport_enable) != 1)) {
        VTSS_RC(vtss_icfg_printf(result, "aggregation mode%s%s%s%s\n",
                                 mode.smac_enable?" smac":"",
                                 mode.dmac_enable?" dmac":"",
                                 mode.sip_dip_enable?" ip":"",
                                 mode.sport_dport_enable?" port":""));
    }   
    return rc;
}

static vtss_rc aggr_icfg_intf_conf(const vtss_icfg_query_request_t *req,
                                   vtss_icfg_query_result_t *result)
{
    vtss_rc                  rc = VTSS_OK;
    vtss_isid_t              isid;
    vtss_port_no_t           iport;
    aggr_mgmt_group_member_t aggr_static;
    aggr_mgmt_group_no_t     search_group=0;
    BOOL                     found = 0;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Interface level: aggregation group <group_id> */

    isid = topo_usid2isid(req->instance_id.port.usid);
    iport = uport2iport(req->instance_id.port.begin_uport);

    while (aggr_mgmt_port_members_get(isid, search_group, &aggr_static, 1) == VTSS_RC_OK) {
        if (aggr_static.entry.member[iport]) {
            found = 1;
            break;
        }
        search_group = aggr_static.aggr_no;
    }
        
    if (req->all_defaults || found) {
        if (found) {
            VTSS_RC(vtss_icfg_printf(result, " aggregation group %lu\n",aggr_static.aggr_no));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " no aggregation group\n"));
        }
    }
    return rc;
}

//******************************************************************************
//   Public functions
//******************************************************************************

vtss_rc aggr_icfg_init(void)
{
    vtss_rc rc;

    /* Register callback functions to ICFG module. */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_AGGR_INTERFACE_CONF, aggr_icfg_intf_conf)) != VTSS_OK) {
        return rc;
    }

    if ((rc = vtss_icfg_query_register(VTSS_ICFG_AGGR_GLOBAL_CONF, aggr_icfg_global_conf)) != VTSS_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}
