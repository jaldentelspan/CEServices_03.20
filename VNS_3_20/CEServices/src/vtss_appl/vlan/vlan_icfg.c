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
#include "vlan_api.h"
#include "misc_api.h"   //uport2iport(), iport2uport()
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#else
#include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */
#include "vlan_icfg.h"

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
static vtss_rc VLAN_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    vtss_rc                 rc = VTSS_OK;
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    vtss_etype_t            tpid;
    int                     conf_changed = 0;

    if ((vlan_mgmt_vlan_tpid_get(&tpid)) != VTSS_OK) {
        return rc;
    }
    /* If conf has changed from default */
    conf_changed = ((tpid != VLAN_DEFAULT_TPID) ? TRUE : FALSE);
    if (req->all_defaults || conf_changed) {
        rc = vtss_icfg_printf(result, "%s 0x%x\n", VLAN_GLOBAL_ETYPE_TEXT, tpid);
    }
#endif

    return rc;
}

static char *vlan_port_frame_type_txt(vtss_vlan_frame_t tagged)
{
    if (tagged == VTSS_VLAN_FRAME_ALL) {
        return "all";
    } else if (tagged == VTSS_VLAN_FRAME_TAGGED) {
        return "tagged";
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    } else if (tagged == VTSS_VLAN_FRAME_UNTAGGED) {
        return "untagged";
#endif
    }
    return "";
}

static char *vlan_port_type_txt(vlan_port_type_t pt)
{
    if (pt == VLAN_PORT_TYPE_UNAWARE) {
        return "unaware";
    } else if (pt == VLAN_PORT_TYPE_C) {
        return "c-port";
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    } else if (pt == VLAN_PORT_TYPE_S) {
        return "s-port";
    } else if (pt == VLAN_PORT_TYPE_S_CUSTOM) {
        return "s-custom-port";
#endif
    }
    return "";
}

static char *vlan_tag_type_txt(vlan_tx_tag_type_t tx_tag_type)
{
    if (tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_ALL) {
        return "none";
    } else if (tx_tag_type == VLAN_TX_TAG_TYPE_TAG_ALL) {
        return "all";
    }
    return "";
}

static vtss_rc VLAN_ICFG_port_conf(const vtss_icfg_query_request_t *req,
                                   vtss_icfg_query_result_t *result)
{
    vtss_rc                     rc = VTSS_OK;
    vlan_port_mode_conf_t       pmode;
    vlan_port_mode_t            def_mode = VLAN_PORT_MODE_ACCESS;
    u8                          vid_bit_mask[VLAN_BIT_MASK_LEN], def_bit_mask[VLAN_BIT_MASK_LEN];
    vtss_vid_t                  vid, def_vid = VTSS_VID_DEFAULT;
    vtss_isid_t                 isid = topo_usid2isid(req->instance_id.port.usid);
    vtss_port_no_t              iport = uport2iport(req->instance_id.port.begin_uport);
    int                         conf_changed = 0;
    i8                          *buf;
    BOOL                        tagging;
    vlan_port_conf_t            port_conf, def_port_conf;

    memset(def_bit_mask, 0, sizeof(def_bit_mask));

    /* Port Mode */
    if ((rc = vlan_mgmt_port_mode_get(isid, iport, &pmode)) != VTSS_OK) {
        return rc;
    }
    conf_changed = (pmode.mode == def_mode) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc = vtss_icfg_printf(result, " %s\n", ((pmode.mode == VLAN_PORT_MODE_ACCESS) ? VLAN_PORT_MODE_ACCESS_TEXT : 
                                               ((pmode.mode == VLAN_PORT_MODE_TRUNK) ? VLAN_PORT_MODE_TRUNK_TEXT :
                                                VLAN_PORT_MODE_HYBRID_TEXT)));
    }

    /* Access VLAN */
    if ((rc = vlan_mgmt_port_access_vlan_get(isid, iport, &vid)) != VTSS_OK) {
        return rc;
    }
    conf_changed = (vid == def_vid) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc += vtss_icfg_printf(result, " %s %u\n", VLAN_PORT_ACCESS_VLAN_TEXT, vid);
    }

    /* Trunk - Native VLAN */
    if ((rc = vlan_mgmt_port_native_vlan_get(isid, iport, &vid, VLAN_PORT_MODE_TRUNK)) != VTSS_OK) {
        return rc;
    }
    conf_changed = (vid == def_vid) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc += vtss_icfg_printf(result, " %s %u\n", VLAN_PORT_TRUNK_VLAN_TEXT, vid);
    }

    /* Hybrid - Native VLAN */
    if ((rc = vlan_mgmt_port_native_vlan_get(isid, iport, &vid, VLAN_PORT_MODE_HYBRID)) != VTSS_OK) {
        return rc;
    }
    conf_changed = (vid == def_vid) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        if (vid == 0) {
            rc += vtss_icfg_printf(result, " %s %s\n", VLAN_PORT_HYBRID_VLAN_TEXT, "none");
        } else {
            rc += vtss_icfg_printf(result, " %s %u\n", VLAN_PORT_HYBRID_VLAN_TEXT, vid);
        }
    }

    /* Trunks - Allowed VLANs */
    if ((rc = vlan_mgmt_port_allowed_vlans_get(isid, iport, vid_bit_mask, VLAN_PORT_MODE_TRUNK)) != VTSS_OK) {
        return rc;
    }
    buf = (i8 *)malloc(VLAN_LIST_BUF_SIZE);
    if (buf == NULL) {
        return VTSS_RC_ERROR;
    }
    conf_changed = (!memcmp(vid_bit_mask, def_bit_mask, sizeof(def_bit_mask))) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc += vtss_icfg_printf(result, " %s %s\n", VLAN_PORT_TRUNK_ALLOWED_VLAN_TEXT, vlan_bit_mask_to_txt(vid_bit_mask, buf));
    }

    /* Hybrid - Allowed VLANs */
    if ((rc = vlan_mgmt_port_allowed_vlans_get(isid, iport, vid_bit_mask, VLAN_PORT_MODE_HYBRID)) != VTSS_OK) {
        free(buf);
        return rc;
    }
    conf_changed = (!memcmp(vid_bit_mask, def_bit_mask, sizeof(def_bit_mask))) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc += vtss_icfg_printf(result, " %s %s\n", VLAN_PORT_TRUNK_ALLOWED_VLAN_TEXT, vlan_bit_mask_to_txt(vid_bit_mask, buf));
    }
    free(buf);

    /* Native VLAN tagging */
    if ((rc = vlan_mgmt_native_vlan_tagging_get(isid, iport, &tagging)) != VTSS_OK) {
        return rc;
    }
    conf_changed = (tagging == TRUE) ? TRUE : FALSE;
    if (req->all_defaults || conf_changed) {
        rc += vtss_icfg_printf(result, " %s\n", VLAN_PORT_TRUNK_VLAN_TEXT);
    }
    
    /* Hybrid port stuff */
    if ((rc = vlan_mgmt_hybrid_port_conf_get(isid, iport, &port_conf)) != VTSS_OK) {
        return rc;
    }
    (void)vlan_mgmt_default_port_conf_get(&def_port_conf);
    /* Acceptable frame type */
    conf_changed = (port_conf.frame_type == def_port_conf.frame_type) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc += vtss_icfg_printf(result, " %s %s\n", VLAN_PORT_HYBRID_FRAME_TYPE_TEXT, vlan_port_frame_type_txt(port_conf.frame_type));
    }
    /* Ingress filter */
    conf_changed = (port_conf.ingress_filter == def_port_conf.ingress_filter) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc += vtss_icfg_printf(result, " %s\n", VLAN_PORT_HYBRID_ING_FILTER_TEXT);
    }
    /* port_type */
    conf_changed = (port_conf.port_type == def_port_conf.port_type) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        rc += vtss_icfg_printf(result, " %s %s\n", VLAN_PORT_HYBRID_PORT_TYPE_TEXT, vlan_port_type_txt(port_conf.port_type));
    }
    /* tx tag */
    conf_changed = (port_conf.tx_tag_type == def_port_conf.tx_tag_type) ? FALSE : TRUE;
    if (req->all_defaults || conf_changed) {
        if (port_conf.tx_tag_type == VLAN_TX_TAG_TYPE_UNTAG_THIS) {
            rc += vtss_icfg_printf(result, " %s %s %s\n", VLAN_PORT_HYBRID_TX_TAG_TEXT, "all", "except-native");
        } else {
            rc += vtss_icfg_printf(result, " %s %s\n", VLAN_PORT_HYBRID_TX_TAG_TEXT, vlan_tag_type_txt(port_conf.tx_tag_type));
        }
    }

    return (rc == VTSS_RC_OK) ? rc : VTSS_RC_ERROR;
}

static vtss_rc VLAN_ICFG_name_conf(const vtss_icfg_query_request_t *req,
                                   vtss_icfg_query_result_t *result)
{
    vtss_rc                 rc = VTSS_OK;
    int                     conf_changed = 0;
    vtss_vid_t              vid = req->instance_id.vlan;
    vlan_mgmt_entry_t       conf;

    if (vlan_mgmt_vlan_get(VTSS_ISID_GLOBAL, vid, &conf, FALSE, VLAN_USER_STATIC, VLAN_MEMBERSHIP_OP) == VTSS_OK) {
        if (vid != VLAN_TAG_DEFAULT) {
            /* If conf has changed from default */
            conf_changed = ((strcmp(conf.vlan_name, "\0")) == 0) ? FALSE : TRUE;
            if (req->all_defaults || conf_changed) {
                rc = vtss_icfg_printf(result, "%s %s\n", VLAN_CONF_MODE_NAME_TEXT, conf.vlan_name);
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
vtss_rc VLAN_icfg_init(void)
{
    vtss_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_VLAN_GLOBAL_CONF, VLAN_ICFG_global_conf)) != VTSS_OK) {
        return rc;
    }
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_VLAN_PORT_CONF, VLAN_ICFG_port_conf)) != VTSS_OK) {
        return rc;
    }
    rc = vtss_icfg_query_register(VTSS_ICFG_VLAN_NAME_CONF, VLAN_ICFG_name_conf);

    return rc;
}
