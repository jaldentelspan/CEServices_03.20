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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "vcl_api.h"
#include "misc_api.h"   //uport2iport(), iport2uport()
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#else
#include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */
#include "vcl_icfg.h"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

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
static vtss_rc VCL_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result)
{
    vtss_rc                         rc = VTSS_OK;
    vcl_proto_vlan_proto_entry_t    entry;
    BOOL                            first_entry = TRUE, next = FALSE;

    while (vcl_proto_vlan_mgmt_proto_get(&entry, VCL_PROTO_VLAN_USER_STATIC, next, first_entry) == VTSS_RC_OK) {
        if (first_entry == TRUE) {
            first_entry = FALSE;
        }
        if (entry.proto_encap_type == VCL_PROTO_ENCAP_ETH2) {
            rc += vtss_icfg_printf(result, "%s %s 0x%x %s %s\n", VCL_GLOBAL_MODE_PROTO_TEXT, "eth2",
                                   entry.proto.eth2_proto.eth_type, "group", entry.group_id);
        } else if (entry.proto_encap_type == VCL_PROTO_ENCAP_LLC_SNAP) {
            rc += vtss_icfg_printf(result, "%s %s 0x%x %s %s\n", VCL_GLOBAL_MODE_PROTO_TEXT, "snap",
                                   entry.proto.eth2_proto.eth_type, "group", entry.group_id);
        } else {
            rc += vtss_icfg_printf(result, "%s %s 0x%x %s %s\n", VCL_GLOBAL_MODE_PROTO_TEXT, "llc",
                                   entry.proto.eth2_proto.eth_type, "group", entry.group_id);
        }
        next = TRUE;
    }

    return rc;
}

static vtss_rc VCL_ICFG_port_conf(const vtss_icfg_query_request_t *req,
                                  vtss_icfg_query_result_t *result)
{
    vtss_rc                             rc = VTSS_OK;
    vtss_isid_t                         isid = topo_usid2isid(req->instance_id.port.usid);
    vtss_port_no_t                      iport = uport2iport(req->instance_id.port.begin_uport);
    vcl_proto_vlan_vlan_entry_t         entry;
    BOOL                                next, first;

    next = FALSE;
    first = TRUE;
    while (vcl_proto_vlan_mgmt_group_entry_get(isid, &entry, VCL_PROTO_VLAN_USER_STATIC, next, first) == VTSS_RC_OK) {
        if (entry.ports[iport]) {
            rc += vtss_icfg_printf(result, "%s %s %s %u\n", VCL_PORT_MODE_PROTO_GROUP_TEXT, entry.group_id, "vlan", entry.vid);
        }
        next = TRUE;
        first = FALSE;
    }

    return (rc == VTSS_RC_OK) ? rc : VTSS_RC_ERROR;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
vtss_rc VCL_icfg_init(void)
{
    vtss_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_VCL_GLOBAL_CONF, VCL_ICFG_global_conf)) != VTSS_OK) {
        return rc;
    }
    rc = vtss_icfg_query_register(VTSS_ICFG_VCL_PORT_CONF, VCL_ICFG_port_conf);

    return rc;
}
