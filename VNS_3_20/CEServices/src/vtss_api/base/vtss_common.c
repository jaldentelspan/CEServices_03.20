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
#include "vtss_state.h"
#include "vtss_common.h"
#include "vtss_fdma_common.h"
#include "vtss_util.h"

/*lint -esym(459, vtss_trace_conf) */
/*lint -esym(459, vtss_state)      */
/*lint -sem(vtss_vcap_id_txt, thread_protected) */

/* Default mapping of PCP to QoS Class */
/* Can also be used for default mapping of QoS class to PCP */
/* This is the IEEE802.1Q-2011 recommended priority to traffic class mappings */
u32 vtss_cmn_pcp2qos(u32 pcp)
{
    switch (pcp) {
    case 0:
        return 1;
    case 1:
        return 0;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        return pcp;
    default:
        VTSS_E("Invalid PCP (%u)", pcp);
        return 0;
    }
}

vtss_rc vtss_port_no_check(const vtss_port_no_t port_no)
{
    if (port_no >= vtss_state->port_count) {
        VTSS_E("%s: illegal port_no: %u", vtss_func, port_no);
        return VTSS_RC_ERROR;
    }

#if defined(VTSS_FEATURE_PORT_CONTROL)
    /* Set currently selected device using port map */
    VTSS_SELECT_CHIP_PORT_NO(port_no);
#endif /* VTSS_FEATURE_PORT_CONTROL */

    return VTSS_RC_OK;
}

vtss_rc vtss_cmn_restart_update(u32 value)
{
    vtss_init_conf_t *conf = &vtss_state->init_conf;

    /* Return if restart has already been updated */
    if (vtss_state->restart_updated)
        return VTSS_RC_OK;
    vtss_state->restart_updated = 1;

    vtss_state->restart_prev = VTSS_EXTRACT_BITFIELD(value,
                                                     VTSS_RESTART_TYPE_OFFSET,
                                                     VTSS_RESTART_TYPE_WIDTH);
    vtss_state->version_prev = VTSS_EXTRACT_BITFIELD(value,
                                                     VTSS_RESTART_VERSION_OFFSET,
                                                     VTSS_RESTART_VERSION_WIDTH);
    vtss_state->restart_cur = vtss_state->restart_prev;
    vtss_state->version_cur = VTSS_API_VERSION;
    switch (vtss_state->restart_cur) {
    case VTSS_RESTART_COLD:
        VTSS_I("cold start detected");
        break;
    case VTSS_RESTART_COOL:
        VTSS_I("cool start detected");
        break;
    case VTSS_RESTART_WARM:
        VTSS_I("warm start detected");
        if (conf->warm_start_enable) {
            if (vtss_state->version_prev > vtss_state->version_cur) {
                VTSS_I("downgrade from version %u to %u",
                       vtss_state->version_prev, vtss_state->version_cur);
            } else {
                vtss_state->warm_start_cur = 1;
                VTSS_I("warm starting");
                return VTSS_RC_OK;
            }
        } else {
            VTSS_I("warm start disabled");
        }
        /* Fall back to cool start */
        vtss_state->restart_cur = VTSS_RESTART_COOL;
        vtss_state->restart_prev = VTSS_RESTART_COOL;
        break;
    default:
        VTSS_I("unknown restart type");
        break;
    }
    return VTSS_RC_OK;
}

u32 vtss_cmn_restart_value_get(void)
{
    return (VTSS_ENCODE_BITFIELD(vtss_state->restart_cur,
                                 VTSS_RESTART_TYPE_OFFSET,
                                 VTSS_RESTART_TYPE_WIDTH) |
            VTSS_ENCODE_BITFIELD(VTSS_API_VERSION,
                                 VTSS_RESTART_VERSION_OFFSET,
                                 VTSS_RESTART_VERSION_WIDTH));
}

#if defined(VTSS_FEATURE_PORT_CONTROL)  ||  defined(VTSS_ARCH_SERVAL)
/* Rebase 64-bit counter, i.e. discard changes since last update, based on 32-bit chip counter */
void vtss_cmn_counter_32_rebase(u32 new_base_value, vtss_chip_counter_t *counter)
{
    counter->prev = new_base_value;
}

/* Clear/increment 64-bit counter based on 32 bit chip counter */
void vtss_cmn_counter_32_update(u32 value, vtss_chip_counter_t *counter, BOOL clear)
{
    u64 add = 0, new = value;
    
    if (clear) {
        /* Clear counter */
        counter->value = 0;
    } else {
        /* Accumulate counter */
        if (new < counter->prev)
            add = (1ULL<<32); /* Wrapped */
        counter->value += (new + add - counter->prev);
    }
    counter->prev = new;
}

/* Rebase 64-bit counter, i.e. discard changes since last update, based on 40-bit chip counter */
void vtss_cmn_counter_40_rebase(u32 new_lsb, u32 new_msb, vtss_chip_counter_t *counter)
{
    counter->prev = new_msb;
    counter->prev = ((counter->prev << 32) + new_lsb);
}

/* Clear/increment 64-bit counter based on 40 bit chip counter */
void vtss_cmn_counter_40_update(u32 lsb, u32 msb, vtss_chip_counter_t *counter, BOOL clear)
{
    u64 add = 0, new = msb;

    new = ((new << 32) + lsb);
    if (clear) {
        /* Clear counter */
        counter->value = 0;
    } else {
        /* Accumulate counter */
        if (new < counter->prev)
            add = (1ULL<<40); /* Wrapped */
        counter->value += (new + add - counter->prev);
    }
    counter->prev = new;
}
#endif

#if defined(VTSS_FEATURE_PORT_CONTROL)
/* Decode advertisement word */
vtss_rc vtss_cmn_port_clause_37_adv_get(u32 value, vtss_port_clause_37_adv_t *adv)
{
    adv->fdx = VTSS_BOOL(value & (1<<5));
    adv->hdx = VTSS_BOOL(value & (1<<6));
    adv->symmetric_pause = VTSS_BOOL(value & (1<<7));
    adv->asymmetric_pause = VTSS_BOOL(value & (1<<8));
    switch ((value>>12) & 3) {
    case 0:
        adv->remote_fault = VTSS_PORT_CLAUSE_37_RF_LINK_OK;
        break;
    case 1:
        adv->remote_fault = VTSS_PORT_CLAUSE_37_RF_LINK_FAILURE;
        break;
    case 2:
        adv->remote_fault = VTSS_PORT_CLAUSE_37_RF_OFFLINE;
        break;
    default:
        adv->remote_fault = VTSS_PORT_CLAUSE_37_RF_AUTONEG_ERROR;
        break;
    } 
    adv->acknowledge = VTSS_BOOL(value & (1<<14));
    adv->next_page = VTSS_BOOL(value & (1<<15));
    
    return VTSS_RC_OK;
}

/* Encode advertisement word */
vtss_rc vtss_cmn_port_clause_37_adv_set(u32 *value, vtss_port_clause_37_adv_t *adv, BOOL aneg_enable)
{
    u32 rf;

    if (!aneg_enable) {
        *value = 0;
        return VTSS_RC_OK;
    }
    switch (adv->remote_fault) {
    case VTSS_PORT_CLAUSE_37_RF_LINK_OK:
        rf = 0;
        break;
    case VTSS_PORT_CLAUSE_37_RF_LINK_FAILURE:
        rf = 1;
        break;
    case VTSS_PORT_CLAUSE_37_RF_OFFLINE:
        rf = 2;
        break;
    default:
        rf = 3;
        break;
    }

    *value = (((adv->next_page ? 1 : 0)<<15) |
              ((adv->acknowledge ? 1 : 0)<<14) |
              (rf<<12) |
              ((adv->asymmetric_pause ? 1 : 0)<<8) |
              ((adv->symmetric_pause ? 1 : 0)<<7) |
              ((adv->hdx ? 1 : 0)<<6) |
              ((adv->fdx ? 1 : 0)<<5));
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_PORT_CONTROL */

#if defined(VTSS_FEATURE_LAYER2)
vtss_rc vtss_cmn_vlan_members_get(vtss_state_t *state, 
                                  const vtss_vid_t vid,
                                  BOOL member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_port_no_t    port_no, port_p, mirror_port;
    vtss_vlan_entry_t *vlan_entry;
    vtss_mstp_entry_t *mstp_entry;
    vtss_port_eps_t   *protect;

    VTSS_N("update %d", vid);

    /* Lookup VLAN, MSTP and ERPS entries */
    vlan_entry = &state->vlan_table[vid];
    mstp_entry = &state->mstp_table[vlan_entry->msti];
    
    /* Mirror port and forwarding mode */
    mirror_port = (state->mirror_conf.fwd_enable ? VTSS_PORT_NO_NONE : state->mirror_conf.port_no);

    /* Determine VLAN port members */
    for (port_no = VTSS_PORT_NO_START; port_no < state->port_count; port_no++) {
        member[port_no] = (
            /* Include VLAN member port */
            VTSS_PORT_BF_GET(vlan_entry->member, port_no) && 
            /* Exclude MSTP/ERPS discarding port */
            mstp_entry->state[port_no] == VTSS_STP_STATE_FORWARDING &&
            port_no != mirror_port &&
            vlan_entry->erps_discard_cnt[port_no] == 0);
#if defined(VTSS_FEATURE_VSTAX)
        /* Include Stack port */
        if (state->vstax_port_enabled[port_no])
            member[port_no] = 1;
#endif  /* VTSS_FEATURE_VSTAX */
#if defined(VTSS_FEATURE_NPI)
        /* Exclude NPI port */
        if (state->npi_conf.enable && port_no == state->npi_conf.port_no) {
            member[port_no] = 0;
        }
#endif /* VTSS_FEATURE_NPI */
    }

    /* Consider protections */
    for (port_no = VTSS_PORT_NO_START; port_no < state->port_count; port_no++) {
        protect = &state->port_protect[port_no];
        if ((port_p = protect->conf.port_no) == VTSS_PORT_NO_NONE)
            continue;

        if (protect->conf.type == VTSS_EPS_PORT_1_PLUS_1) {
            /* Port is 1+1 protected, include both ports if one of them is VLAN member */
            if (member[port_no])
                member[port_p] = 1;
            if (member[port_p])
                member[port_no] = 1;
        } else if (member[port_no]) {
            /* Port is 1:1 protected and VLAN member */
            if (protect->selector == VTSS_EPS_SELECTOR_WORKING) {
                /* Working port selected, protection port is not VLAN member */
                member[port_p] = 0;
            } else {
                /* Protection port selected, working port is not VLAN member */
                member[port_p] = 1;
                member[port_no] = 0;
            }
        }
    }

    return VTSS_RC_OK;
}

vtss_rc vtss_cmn_vlan_members_set(const vtss_vid_t vid)
{
    BOOL member[VTSS_PORT_ARRAY_SIZE];

    VTSS_N("update %d", vid);

    if(vtss_state->cil_func.vlan_mask_update == NULL)
        return VTSS_RC_ERROR;

    VTSS_RC(vtss_cmn_vlan_members_get(vtss_state, vid, member));

    return vtss_state->cil_func.vlan_mask_update(vid, member);
}

#if defined(VTSS_FEATURE_EVC)
static BOOL vtss_cmn_evc_port_check(void)
{
    return (vtss_state->arch == VTSS_ARCH_L26 || vtss_state->arch == VTSS_ARCH_JR1 ? 1 : 0);
}
#endif /* VTSS_FEATURE_EVC */

#if defined(VTSS_FEATURE_ES0)
static vtss_rc vtss_cmn_es0_update(const vtss_port_no_t port_no, u16 flags)
{
    vtss_vcap_entry_t *cur;
    vtss_es0_data_t   *data;
    vtss_vcap_idx_t   idx;
    vtss_es0_entry_t  entry;
        
    memset(&idx, 0, sizeof(idx));
    for (cur = vtss_state->es0.obj.used; cur != NULL; cur = cur->next, idx.row++) {
        data = &cur->data.u.es0;
        if ((data->port_no == port_no && (data->flags & flags & VTSS_ES0_FLAG_MASK_PORT)) || 
            (data->nni == port_no && (data->flags & flags & VTSS_ES0_FLAG_MASK_NNI))) {
            data->entry = &entry;
            vtss_cmn_es0_action_get(data);
            VTSS_FUNC_RC(es0_entry_update, &idx, data);
        }
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_ES0 */

vtss_rc vtss_cmn_vlan_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_rc                rc;
    vtss_vlan_port_conf_t  *conf;
    vtss_mirror_conf_t     *mirror_conf = &vtss_state->mirror_conf;
    vtss_vid_t             uvid;
    
    if (vtss_state->cil_func.vlan_port_conf_update == NULL)
        return VTSS_RC_ERROR;

    conf = &vtss_state->vlan_port_conf[port_no];
    uvid = conf->untagged_vid;
#if defined(VTSS_FEATURE_EVC)
    if (vtss_cmn_evc_port_check()) {
        /* Override UVID for UNI/NNI ports */
        if (vtss_state->evc_port_info[port_no].uni_count)
            conf->untagged_vid = VTSS_VID_ALL;
        else if (vtss_state->evc_port_info[port_no].nni_count)
            conf->untagged_vid = VTSS_VID_NULL;
    }
#endif /* VTSS_FEATURE_EVC */
    
    /* Avoid tagging on mirror port */
    if (port_no == mirror_conf->port_no && !mirror_conf->fwd_enable) {
        conf->untagged_vid = VTSS_VID_ALL;
#if defined(VTSS_ARCH_JAGUAR_1)
        /* Enable tagging on mirror port */
        if (mirror_conf->tag != VTSS_MIRROR_TAG_NONE)
            conf->untagged_vid = VTSS_VID_NULL;
#endif /* VTSS_ARCH_JAGUAR_1 */
    }

    rc = vtss_state->cil_func.vlan_port_conf_update(port_no, conf);
    conf->untagged_vid = uvid;
#if defined(VTSS_FEATURE_ES0)
    if (vtss_state->vlan_port_type[port_no] != conf->port_type) {
        /* VLAN port type changed, update ES0 rules */
        vtss_state->vlan_port_type[port_no]= conf->port_type;
        VTSS_RC(vtss_cmn_es0_update(port_no, VTSS_ES0_FLAG_TPID));
    }
#endif /* VTSS_FEATURE_ES0 */
    
    return rc;
}

#if defined(VTSS_FEATURE_VLAN_TX_TAG)
static void vtss_cmn_es0_data_set(vtss_port_no_t port_no, vtss_vid_t vid, vtss_vid_t new_vid, 
                                  BOOL tag_enable, 
                                  vtss_vcap_data_t *data, vtss_es0_entry_t *entry)
{
    vtss_es0_data_t   *es0 = &data->u.es0;
    vtss_es0_key_t    *key = &entry->key;
    vtss_es0_action_t *action = &entry->action;

    /* Use (VID, port) as key */
    vtss_vcap_es0_init(data, entry);
    key->port_no = port_no;
    key->type = VTSS_ES0_TYPE_VID;
    key->data.vid.vid = vid;
    
    if (tag_enable) {
        /* Enable tagging */
        action->tag = VTSS_ES0_TAG_ES0;
        es0->flags = (VTSS_ES0_FLAG_TPID | VTSS_ES0_FLAG_QOS); /* Use port TPID and QoS */
        es0->port_no = port_no;
        action->vid_b = new_vid;
        
#if defined(VTSS_ARCH_SERVAL)
        if (vtss_state->arch == VTSS_ARCH_SRVL) {
            action->outer_tag.tag = VTSS_ES0_TAG_ES0;
            action->outer_tag.vid.sel = 1;
            action->outer_tag.vid.val = new_vid;
        }
#endif /* VTSS_ARCH_SERVAL */
    } else {
        action->tag = VTSS_ES0_TAG_NONE;
        action->tpid = VTSS_ES0_TPID_S; /* Needed to overrule port configuration */
    }
}

static vtss_vcap_id_t vtss_tx_tag_vcap_id(vtss_vid_t vid, vtss_port_no_t port_no)
{
    vtss_vcap_id_t id = vid;

    return ((id << 32) + port_no);
}

vtss_rc vtss_cmn_vlan_tx_tag_set(const vtss_vid_t         vid,
                                 const vtss_vlan_tx_tag_t tx_tag[VTSS_PORT_ARRAY_SIZE])
{
    vtss_res_t        res;
    vtss_port_no_t    port_no, i;
    vtss_vlan_entry_t *vlan_entry = &vtss_state->vlan_table[vid];
    vtss_vcap_obj_t   *obj = &vtss_state->es0.obj;
    vtss_vcap_id_t    id, id_next;
    vtss_vid_t        vid_next;
    vtss_vcap_data_t  data;
    vtss_es0_entry_t  entry;
    BOOL              tag_enable;

    /* Check resource consumption */
    vtss_cmn_res_init(&res);
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (vlan_entry->tx_tag[port_no] == VTSS_VLAN_TX_TAG_PORT) {
            if (tx_tag[port_no] != VTSS_VLAN_TX_TAG_PORT) {
                /* Add new ES0 rule for port */
                res.es0.add++;
            }
        } else if (tx_tag[port_no] == VTSS_VLAN_TX_TAG_PORT) {
            /* Delete old ES0 rule for port */
            res.es0.del++;
        }
    }
    VTSS_RC(vtss_cmn_res_check(&res));

    /* Delete resources */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (vlan_entry->tx_tag[port_no] != VTSS_VLAN_TX_TAG_PORT && 
            tx_tag[port_no] == VTSS_VLAN_TX_TAG_PORT) {
            id = vtss_tx_tag_vcap_id(vid, port_no);
            VTSS_RC(vtss_vcap_del(obj, VTSS_ES0_USER_TX_TAG, id));
        }
        vlan_entry->tx_tag[port_no] = tx_tag[port_no];
    }

    /* Find next ID */
    id_next = VTSS_VCAP_ID_LAST;
    for (vid_next = (vid + 1); vid_next < VTSS_VIDS; vid_next++) {
        vlan_entry = &vtss_state->vlan_table[vid_next];
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (vlan_entry->tx_tag[port_no] != VTSS_VLAN_TX_TAG_PORT) {
                id_next = vtss_tx_tag_vcap_id(vid_next, port_no);
                vid_next = VTSS_VIDS;
                break;
            }
        }
    }

    /* Add/update resources */
    for (i = vtss_state->port_count; i != 0; i--) {
        port_no = (i - 1);
        switch (tx_tag[port_no]) {
        case VTSS_VLAN_TX_TAG_ENABLE:
            tag_enable = 1;
            break;
        case VTSS_VLAN_TX_TAG_DISABLE:
            tag_enable = 0;
            break;
        default:
            continue;
        }
        vtss_cmn_es0_data_set(port_no, vid, vid, tag_enable, &data, &entry);
        id = vtss_tx_tag_vcap_id(vid, port_no);
        VTSS_RC(vtss_vcap_add(obj, VTSS_ES0_USER_TX_TAG, id, id_next, &data, 0));
        id_next = id;
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_VLAN_TX_TAG */

static BOOL vtss_cmn_vlan_enabled(vtss_vlan_entry_t *vlan_entry)
{
    return (vlan_entry->enabled);
}

/* Update all VLANs */
vtss_rc vtss_cmn_vlan_update_all(void)
{
    vtss_vid_t vid;

    for (vid = VTSS_VID_DEFAULT; vid < VTSS_VIDS; vid++)
        VTSS_RC(vtss_cmn_vlan_members_set(vid));
    return VTSS_RC_OK;
}

vtss_rc vtss_cmn_mstp_state_set(const vtss_port_no_t   port_no,
                                const vtss_msti_t      msti)
{
    vtss_vid_t        vid;
    vtss_vlan_entry_t *vlan_entry;
    
    /* Update all VLANs mapping to MSTI */
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        vlan_entry = &vtss_state->vlan_table[vid];
        if (vtss_cmn_vlan_enabled(vlan_entry) && vlan_entry->msti == msti)
            VTSS_RC(vtss_cmn_vlan_members_set(vid));
    }
    
    return VTSS_RC_OK;
}

vtss_rc vtss_cmn_erps_vlan_member_set(const vtss_erpi_t erpi,
                                      const vtss_vid_t  vid)
{
    return vtss_cmn_vlan_members_set(vid);
}

vtss_rc vtss_cmn_erps_port_state_set(const vtss_erpi_t    erpi,
                                     const vtss_port_no_t port_no)
{
    vtss_vid_t        vid;
    vtss_vlan_entry_t *vlan_entry;

    /* Update all VLANs changed by ERPS */
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        vlan_entry = &vtss_state->vlan_table[vid];
        if (vlan_entry->update) {
            vlan_entry->update = 0;
            if (vtss_cmn_vlan_enabled(vlan_entry))
                VTSS_RC(vtss_cmn_vlan_members_set(vid));
        }
    }
    return VTSS_RC_OK;

}

vtss_rc vtss_cmn_eps_port_set(const vtss_port_no_t port_w)
{
    vtss_vid_t vid;

    /* Update all VLANs */
    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        if (vtss_cmn_vlan_enabled(&vtss_state->vlan_table[vid]))
            VTSS_RC(vtss_cmn_vlan_members_set(vid));
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_VCAP)

/* ================================================================= *
 *  VCAP resource utilities
 * ================================================================= */

/* Initialize resource change information */
void vtss_cmn_res_init(vtss_res_t *res)
{
    memset(res, 0, sizeof(*res));
}

/* Check VCAP resource usage */
vtss_rc vtss_cmn_vcap_res_check(vtss_vcap_obj_t *obj, vtss_res_chg_t *chg)
{
    u32                  add_row, del_row, add, del, old, new, key_count, count;
    vtss_vcap_key_size_t key_size;
    
    add_row = chg->add;
    del_row = chg->del;

    /* Calculate added/deleted rows for each key size */
    for (key_size = VTSS_VCAP_KEY_SIZE_FULL; key_size <= VTSS_VCAP_KEY_SIZE_LAST; key_size++) {
        count = vtss_vcap_key_rule_count(key_size);
        add = chg->add_key[key_size];
        del = chg->del_key[key_size];
        key_count = (obj->key_count[key_size] + count - 1);
        old = (key_count / count);
        new = ((key_count + add - del) / count);
        
        if (add > del) {
            /* Adding rules may cause addition of rows */
            add_row += (new - old);
        } else {
            /* Deleting rules may cause deletion of rows */
            del_row += (old - new);
        }
    }

    if ((obj->count + add_row) > (obj->max_count + del_row)) {
        VTSS_I("VCAP %s exceeded, add: %u, del: %u, count: %u, max: %u", 
               obj->name, add_row, del_row, obj->count, obj->max_count);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

/* Check SDK and VCAP resource usage */
vtss_rc vtss_cmn_res_check(vtss_res_t *res)
{
#if defined(VTSS_SDX_CNT)
    vtss_sdx_info_t *sdx_info = &vtss_state->sdx_info;
    
    if ((sdx_info->isdx.count + res->isdx.add - res->isdx.del) > sdx_info->max_count) {
        VTSS_I("ISDX exceeded");
        return VTSS_RC_ERROR;
    }

    if ((sdx_info->esdx.count + res->esdx.add - res->esdx.del) > sdx_info->max_count) {
        VTSS_I("ESDX exceeded");
        return VTSS_RC_ERROR;
    }
#endif /* VTSS_SDX_CNT */

#if defined(VTSS_FEATURE_IS0)
    /* VCAP IS0 */
    VTSS_RC(vtss_cmn_vcap_res_check(&vtss_state->is0.obj, &res->is0));
#endif /* VTSS_FEATURE_IS0 */
    
#if defined(VTSS_FEATURE_IS1)
    /* VCAP IS1 */
    VTSS_RC(vtss_cmn_vcap_res_check(&vtss_state->is1.obj, &res->is1));
#endif /* VTSS_FEATURE_IS1 */

#if defined(VTSS_FEATURE_IS2)
    /* VCAP IS2*/
    VTSS_RC(vtss_cmn_vcap_res_check(&vtss_state->is2.obj, &res->is2));
#endif /* VTSS_FEATURE_IS2 */

#if defined(VTSS_FEATURE_ES0)
    /* VCAP ES0 */
    VTSS_RC(vtss_cmn_vcap_res_check(&vtss_state->es0.obj, &res->es0));
#endif /* VTSS_FEATURE_IS0 */

    return VTSS_RC_OK;
}

/* ================================================================= *
 *  VCAP range checkers
 * ================================================================= */

/* Determine if UDP/TCP rule */
BOOL vtss_vcap_udp_tcp_rule(const vtss_vcap_u8_t *proto)
{
    return (proto->mask == 0xff && (proto->value == 6 || proto->value == 17));
}

/* Allocate range checker */
vtss_rc vtss_vcap_range_alloc(vtss_vcap_range_chk_table_t *range_chk_table,
                              u32 *range,
                              vtss_vcap_range_chk_type_t type,
                              u32 min,
                              u32 max)
{
    u32                   i, new = VTSS_VCAP_RANGE_CHK_NONE;
    vtss_vcap_range_chk_t *entry;
    
    for (i = 0; i < VTSS_VCAP_RANGE_CHK_CNT; i++) {
        entry = &range_chk_table->entry[i];
        if (entry->count == 0) {
            /* Free entry found, possibly reuse old range checker */
            if (new == VTSS_VCAP_RANGE_CHK_NONE || i == *range)
                new = i;
        } else if (entry->type == type && entry->min == min && entry->max == max) {
            /* Matching used entry */
            new = i;
            break;
        }
    }
    
    if (new == VTSS_VCAP_RANGE_CHK_NONE) {
        VTSS_I("no more range checkers");
        return VTSS_RC_ERROR;
    }

    VTSS_I("alloc range: %u, min: %u, max: %u", new, min, max);

    entry = &range_chk_table->entry[new];
    entry->count++;
    entry->type = type;
    entry->min = min;
    entry->max = max;
    *range = new;
    return VTSS_RC_OK;
}

/* Free VCAP range checker */
vtss_rc vtss_vcap_range_free(vtss_vcap_range_chk_table_t *range_chk_table,
                             u32 range)
{
    vtss_vcap_range_chk_t *entry;

    /* Ignore this special value */
    if (range == VTSS_VCAP_RANGE_CHK_NONE)
        return VTSS_RC_OK;

    if (range >= VTSS_VCAP_RANGE_CHK_CNT) {
        VTSS_E("illegal range: %u", range);
        return VTSS_RC_ERROR;
    }

    entry = &range_chk_table->entry[range];
    if (entry->count == 0) {
        VTSS_E("illegal range free: %u", range);
        return VTSS_RC_ERROR;
    }
    
    entry->count--;
    return VTSS_RC_OK;
}

/* Allocate UDP/TCP range checker */
vtss_rc vtss_vcap_udp_tcp_range_alloc(vtss_vcap_range_chk_table_t *range_chk_table,
                                      u32 *range, 
                                      const vtss_vcap_udp_tcp_t *port, 
                                      BOOL sport)
{
    vtss_vcap_range_chk_type_t type;
    
    if (port->low == port->high || (port->low == 0 && port->high == 0xffff)) {
        /* No range checker required */
        *range = VTSS_VCAP_RANGE_CHK_NONE;
        return VTSS_RC_OK;
    }
    type = (sport ? VTSS_VCAP_RANGE_TYPE_SPORT : VTSS_VCAP_RANGE_TYPE_DPORT);
    return vtss_vcap_range_alloc(range_chk_table, range, type, port->low, port->high);
}

/* Allocate universal range checker
   NOTE: If it is possible to change an inclusive range to a value/mask,
   the vr is modified here in order to save a range checker.
*/
vtss_rc vtss_vcap_vr_alloc(vtss_vcap_range_chk_table_t *range_chk_table,
                           u32 *range,
                           vtss_vcap_range_chk_type_t type,
                           vtss_vcap_vr_t *vr)
{
    u8 bits = (type == VTSS_VCAP_RANGE_TYPE_DSCP ? 6 :
               type == VTSS_VCAP_RANGE_TYPE_VID ? 12 : 16);
    u32 max_value = (1 << bits) - 1;

    /* Parameter check */
    if (vr->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
        if (vr->vr.v.value > max_value) {
            VTSS_E("illegal value: 0x%X (max value is 0x%X)", vr->vr.v.value, max_value);
            return VTSS_RC_ERROR;
        }
    }
    else {
        if ((vr->vr.r.low > max_value) || (vr->vr.r.high > max_value) || (vr->vr.r.low > vr->vr.r.high)){
            VTSS_E("illegal range: 0x%X-0x%X", vr->vr.r.low, vr->vr.r.high);
            return VTSS_RC_ERROR;
        }
    }

    /* Try to convert an inclusive range to value/mask
     * The following ranges can be converted:
     * [n;n]
     * [0;((2^n)-1)]
     * [(2^n);((2^(n+1))-1)]
     * Examples:
     * [0;0], [1;1], [2;2], [3;3], [4;4], [5;5], ...
     * [0;1], [0;3], [0;7], [0;15], [0;31], [0;63],...
     * [1;1], [2;3], [4;7], [8;15], [16;31], [32;63], ...
     */
    if (vr->type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE) {
        vtss_vcap_vr_value_t mask;
        vtss_vcap_vr_value_t low  = vr->vr.r.low;
        vtss_vcap_vr_value_t high = vr->vr.r.high;
        int bit;

        for (bit = bits - 1; bit >= 0; bit--) {
            mask = 1 << bit;
            if ((low & mask) != (high & mask)) {
                break;
            }
        }
        mask = (1 << (bit+1)) - 1;

        if (((low & ~mask) == low) && ((high & mask) == mask)) {
            vr->type = VTSS_VCAP_VR_TYPE_VALUE_MASK;
            vr->vr.v.value = low;            
            vr->vr.v.mask = ~mask;            
            VTSS_I("range %u-%u converted to value 0x%X, mask 0x%X", low, high, vr->vr.v.value, vr->vr.v.mask);
        }
    }

    if (vr->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
        /* No range checker required */
        *range = VTSS_VCAP_RANGE_CHK_NONE;
        return VTSS_RC_OK;
    }
    return vtss_vcap_range_alloc(range_chk_table, range, type, vr->vr.r.low, vr->vr.r.high);
}

vtss_rc vtss_vcap_range_commit(vtss_vcap_range_chk_table_t *range_new)
{
    if (memcmp(&vtss_state->vcap_range, range_new, sizeof(*range_new))) {
        /* The temporary working copy has changed - Save it and commit */
        vtss_state->vcap_range = *range_new;
        VTSS_FUNC_RC(vcap_range_commit);
    }
    return VTSS_RC_OK;
}

/* ================================================================= *
 *  VCAP 
 * ================================================================= */

/* Return number of rules per row for a given key */
u32 vtss_vcap_key_rule_count(vtss_vcap_key_size_t key_size)
{
    if (key_size > VTSS_VCAP_KEY_SIZE_LAST) {
        VTSS_E("illegal key_size");
    }
    return (key_size == VTSS_VCAP_KEY_SIZE_QUARTER ? 4 :
            key_size == VTSS_VCAP_KEY_SIZE_HALF ? 2 : 1);
}

const char *vtss_vcap_key_size_txt(vtss_vcap_key_size_t key_size)
{
    return (key_size == VTSS_VCAP_KEY_SIZE_FULL ? "Full" :
            key_size == VTSS_VCAP_KEY_SIZE_HALF ? "Half" :
            key_size == VTSS_VCAP_KEY_SIZE_QUARTER ? "Quarter" : "?");
}

/* Get (row, col) position of rule */
static void vtss_vcap_pos_get(vtss_vcap_obj_t *obj, vtss_vcap_idx_t *idx, u32 ndx)
{
    u32                  cnt;
    vtss_vcap_key_size_t key_size;

    /* Use index to find (row, col) within own block */
    cnt = vtss_vcap_key_rule_count(idx->key_size);
    idx->row = (ndx / cnt);
    idx->col = (ndx % cnt);

    /* Include quarter block rows */
    if ((key_size = VTSS_VCAP_KEY_SIZE_QUARTER) > idx->key_size) {
        cnt = vtss_vcap_key_rule_count(key_size);
        idx->row += ((obj->key_count[key_size] + cnt - 1) / cnt);
    }

    /* Include half block rows */
    if ((key_size = VTSS_VCAP_KEY_SIZE_HALF) > idx->key_size) {
        cnt = vtss_vcap_key_rule_count(key_size);
        idx->row += ((obj->key_count[key_size] + cnt - 1) / cnt);
    }
}

char *vtss_vcap_id_txt(vtss_vcap_id_t id)
{
    static char buf[64];
    static int  i = 0;
    u32         high = ((id >> 32) & 0xffffffff);
    u32         low = (id & 0xffffffff);
    char        *txt;

    i++;
    txt = &buf[(i & 1) ? 0 : 32];
    sprintf(txt, "0x%08x:0x%08x", high, low);
    return txt;
}

/* Lookup VCAP entry */
vtss_rc vtss_vcap_lookup(vtss_vcap_obj_t *obj, int user, vtss_vcap_id_t id, 
                         vtss_vcap_data_t *data, vtss_vcap_idx_t *idx)
{
    u32                  ndx[VTSS_VCAP_KEY_SIZE_MAX];
    vtss_vcap_entry_t    *cur;
    vtss_vcap_key_size_t key_size;

    VTSS_D("VCAP %s, id: %s", obj->name, vtss_vcap_id_txt(id));
    
    memset(ndx, 0, sizeof(ndx));

    for (cur = obj->used; cur != NULL; cur = cur->next) {
        key_size = cur->data.key_size;
        if (cur->user == user && cur->id == id) {
            if (idx != NULL) {
                idx->key_size = key_size;
                vtss_vcap_pos_get(obj, idx, ndx[key_size]);
            }
            if (data != NULL)
                *data = cur->data;
            return VTSS_RC_OK;
        }
        ndx[key_size]++;
    }
    return VTSS_RC_ERROR;
}

/* Delete rule found in list */
static vtss_rc vtss_vcap_del_rule(vtss_vcap_obj_t *obj, vtss_vcap_entry_t *cur, 
                                  vtss_vcap_entry_t *prev, u32 ndx)
{
    vtss_vcap_key_size_t key_size;
    vtss_vcap_idx_t      idx;
    u32                  cnt;
    
    VTSS_D("VCAP %s, ndx: %u", obj->name, ndx);

    /* Move rule to free list */
    if (prev == NULL)
        obj->used = cur->next;
    else
        prev->next = cur->next;
    cur->next = obj->free;
    obj->free = cur;
    obj->rule_count--;
    
    /* Delete VCAP entry from block */
    key_size = cur->data.key_size;
    idx.key_size = key_size;
    obj->key_count[key_size]--;
    cnt = obj->key_count[key_size];
    if (ndx == cnt) {
        /* Last rule in block, just delete */
        vtss_vcap_pos_get(obj, &idx, ndx);
        VTSS_RC(obj->entry_del(&idx));
    } else {
        /* Delete and contract by moving rules up */
        vtss_vcap_pos_get(obj, &idx, ndx + 1);
        VTSS_RC(obj->entry_move(&idx, cnt - ndx, 1));
    }
    
    /* Get position of the entry after the last entry in block */
    vtss_vcap_pos_get(obj, &idx, cnt);
    if (idx.col) {
        /* Done, there are more rules on the last row */
        return VTSS_RC_OK;
    }
    
    /* Delete and contract by moving rows up */
    obj->count--;
    if (idx.row != obj->count) {
        cnt = (obj->count - idx.row);
        idx.key_size = VTSS_VCAP_KEY_SIZE_FULL;
        idx.row++;
        VTSS_RC(obj->entry_move(&idx, cnt, 1));
    }
    return VTSS_RC_OK;
}

/* Delete VCAP rule */
vtss_rc vtss_vcap_del(vtss_vcap_obj_t *obj, int user, vtss_vcap_id_t id)
{
    vtss_vcap_entry_t    *cur, *prev = NULL;
    vtss_vcap_key_size_t key_size;
    u32                  ndx[VTSS_VCAP_KEY_SIZE_MAX];

    VTSS_D("VCAP %s, id: %s", obj->name, vtss_vcap_id_txt(id));

    memset(ndx, 0, sizeof(ndx));
    for (cur = obj->used; cur != NULL; prev = cur, cur = cur->next) {
        key_size = cur->data.key_size;
        if (cur->user == user && cur->id == id) {
            /* Found rule, delete it */
            return vtss_vcap_del_rule(obj, cur, prev, ndx[key_size]);
        }
        ndx[key_size]++;
    }

    /* Silently ignore if rule not found */
    return VTSS_RC_OK;
}

vtss_rc vtss_vcap_add(vtss_vcap_obj_t *obj, int user, vtss_vcap_id_t id, 
                      vtss_vcap_id_t ins_id, vtss_vcap_data_t *data, BOOL dont_add)
{
    u32                  cnt = 0, ndx_ins = 0, ndx_old = 0, ndx_old_key[VTSS_VCAP_KEY_SIZE_MAX];
    vtss_vcap_entry_t    *cur, *prev = NULL;
    vtss_vcap_entry_t    *old = NULL, *old_prev = NULL, *ins = NULL, *ins_prev = NULL;
    vtss_vcap_idx_t      idx;
    vtss_vcap_key_size_t key_size, key_size_new;
    vtss_res_chg_t       chg;
    vtss_vcap_id_t       cur_id;
    
    key_size_new = (data ? data->key_size : VTSS_VCAP_KEY_SIZE_FULL);
    memset(ndx_old_key, 0, sizeof(ndx_old_key));

    VTSS_D("VCAP %s, key_size: %s, id: %s, ins_id: %s", 
           obj->name, vtss_vcap_key_size_txt(key_size_new),
           vtss_vcap_id_txt(id), vtss_vcap_id_txt(ins_id));
    
    for (cur = obj->used; cur != NULL; prev = cur, cur = cur->next) {
        /* No further processing if bigger user found */
        if (cur->user > user)
            break;

        /* Look for existing ID and next ID */
        if (cur->user == user) {
            cur_id = cur->id;
            if (cur_id == id) {
                /* Found ID */
                VTSS_D("found old id");
                old = cur;
                old_prev = prev;
            }

            if (cur_id == ins_id) {
                /* Found place to insert */
                VTSS_D("found ins_id");
                ins = cur;
                ins_prev = prev;
            }
        }

        /* Count entries depending on key size */
        key_size = cur->data.key_size;
        if (key_size > VTSS_VCAP_KEY_SIZE_LAST) {
            VTSS_E("VCAP %s key size exceeded", obj->name);
            return VTSS_RC_ERROR;
        }
        
        /* Count number of rules smaller than insert entry */
        if (ins == NULL && key_size == key_size_new)
            ndx_ins++;

        /* Count number of rules smaller than old entry */
        if (old == NULL)
            ndx_old_key[key_size]++;
    }
    
    /* Check if insert ID is valid */
    if (ins == NULL) {
        if (ins_id != VTSS_VCAP_ID_LAST) {
            VTSS_E("VCAP %s: Could not find ID: %llu", obj->name, ins_id);
            return VTSS_RC_ERROR;
        }
        ins_prev = prev;
    }

    /* Check if resources are available */
    if (old == NULL || old->data.key_size != key_size_new) {
        memset(&chg, 0, sizeof(chg));

        /* Calculate added resources */
        if ((obj->key_count[key_size_new] % vtss_vcap_key_rule_count(key_size_new)) == 0)
            chg.add++;

        /* Calculate deleted resources */
        if (old != NULL) {
            key_size = old->data.key_size;
            if ((obj->key_count[key_size] % vtss_vcap_key_rule_count(key_size)) == 1)
                chg.del++;
        }
        VTSS_RC(vtss_cmn_vcap_res_check(obj, &chg));
    }
    
    /* Return here if only checking if entry can be added */
    if (data == NULL || dont_add)
        return VTSS_RC_OK;

    /* Read counter */
    if (old == NULL) {
        key_size = key_size_new; /* Just to please Lint */
    } else {
        key_size = old->data.key_size;
        ndx_old = ndx_old_key[key_size];
        idx.key_size = key_size;
        vtss_vcap_pos_get(obj, &idx, ndx_old);
        VTSS_RC(obj->entry_get(&idx, &cnt, 0));
    }
    
    if (old == NULL || key_size != key_size_new || (ndx_old + 1) != ndx_ins) {
        /* New entry or changed key size/position, delete/add is required */
        if (old == NULL) {
            VTSS_D("new rule, ndx_ins: %u", ndx_ins);
        } else {
            VTSS_D("changed key_size/position");
            VTSS_RC(vtss_vcap_del_rule(obj, old, old_prev, ndx_old));
            if (key_size == key_size_new) {
                VTSS_D("new position, ndx_ins: %u, ndx_old: %u", ndx_ins, ndx_old);
                if (ndx_ins > ndx_old) {
                    /* The deleted old entry was before the new index, adjust for that */
                    ndx_ins--;
                }
            } else {
                VTSS_D("new key_size, ndx_ins: %u", ndx_ins);
            }
            if (ins_prev == old) {
                /* Old entry was just deleted, adjust for that */
                ins_prev = old_prev;
            }
        }
        
        /* Insert new rule in used list */
        if ((cur = obj->free) == NULL) {
            VTSS_E("VCAP %s: No more free rules", obj->name);
            return VTSS_RC_ERROR;
        }
        obj->free = cur->next;
        if (ins_prev == NULL) {
            cur->next = obj->used;
            obj->used = cur;
        } else {
            cur->next = ins_prev->next;
            ins_prev->next = cur;
        }
        obj->rule_count++;
        cur->user = user;
        cur->id = id;
        
        /* Get position of the entry after the last entry in block */
        key_size = key_size_new;
        idx.key_size = key_size;
        vtss_vcap_pos_get(obj, &idx, key_size == VTSS_VCAP_KEY_SIZE_FULL ? ndx_ins : 
                          obj->key_count[key_size]);
        if (idx.col == 0) {
            if (idx.row < obj->count) {
                /* Move rows down */
                idx.key_size = VTSS_VCAP_KEY_SIZE_FULL;
                VTSS_RC(obj->entry_move(&idx, obj->count - idx.row, 0));
            }
            obj->count++;
        }

        /* Move rules down */
        if (key_size != VTSS_VCAP_KEY_SIZE_FULL && obj->key_count[key_size] > ndx_ins) {
            idx.key_size = key_size;
            vtss_vcap_pos_get(obj, &idx, ndx_ins);
            VTSS_RC(obj->entry_move(&idx, obj->key_count[key_size] - ndx_ins, 0));
        }
        obj->key_count[key_size]++;
    } else {
        VTSS_D("rule unchanged");
        cur = old;
        ndx_ins = ndx_old;
    }
    
    cur->data = *data;

    /* Write entry */
    idx.key_size = key_size_new;
    vtss_vcap_pos_get(obj, &idx, ndx_ins);
    return obj->entry_add(&idx, data, cnt);
}

/* Get next ID for one user based on another user (special function for PTP) */
vtss_rc vtss_vcap_get_next_id(vtss_vcap_obj_t *obj, int user1, int user2, 
                              vtss_vcap_id_t id, vtss_vcap_id_t *ins_id)
{
    vtss_vcap_entry_t *cur, *next;

    /* Look for entry in user1 list */
    *ins_id = VTSS_VCAP_ID_LAST;
    for (cur = obj->used; cur != NULL; cur = cur->next) {
        if (cur->user == user1 && cur->id == id) {
            /* Found entry */
            next = cur->next;
            break;
        }
    }
    
    if (cur == NULL) {
        VTSS_E("VCAP %s: ID not found", obj->name);
        return VTSS_RC_ERROR;
    }

    /* Look for entry in user2 list */
    for (next = cur->next; next != NULL && next->user == user1; next = next->next) {
        for (cur = obj->used; cur != NULL; cur = cur->next) {
            if (cur->user == user2 && cur->id == next->id) {
                *ins_id = cur->id;
                return VTSS_RC_OK;
            }
        }
    }
    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_IS1)
void vtss_vcap_is1_init(vtss_vcap_data_t *data, vtss_is1_entry_t *entry)
{
    vtss_is1_data_t *is1 = &data->u.is1;

    memset(data, 0, sizeof(*data));
    memset(entry, 0, sizeof(*entry));
    is1->vid_range = VTSS_VCAP_RANGE_CHK_NONE;
    is1->dscp_range = VTSS_VCAP_RANGE_CHK_NONE;
    is1->sport_range = VTSS_VCAP_RANGE_CHK_NONE;
    is1->dport_range = VTSS_VCAP_RANGE_CHK_NONE;
    is1->entry = entry;
}
#endif /* VTSS_FEATURE_IS1 */

#if defined(VTSS_FEATURE_IS2)
void vtss_vcap_is2_init(vtss_vcap_data_t *data, vtss_is2_entry_t *entry)
{
    vtss_is2_data_t *is2 = &data->u.is2;

    memset(data, 0, sizeof(*data));
    memset(entry, 0, sizeof(*entry));
    is2->srange = VTSS_VCAP_RANGE_CHK_NONE;
    is2->drange = VTSS_VCAP_RANGE_CHK_NONE;
    is2->entry = entry;
}
#endif /* VTSS_FEATURE_IS2 */

#if defined(VTSS_FEATURE_ES0)
void vtss_vcap_es0_init(vtss_vcap_data_t *data, vtss_es0_entry_t *entry)
{
    memset(data, 0, sizeof(*data));
    memset(entry, 0, sizeof(*entry));
    data->u.es0.entry = entry;
}

/* Update ES0 action fields based on VLAN and QoS port configuration */
void vtss_cmn_es0_action_get(vtss_es0_data_t *es0)
{
    vtss_es0_action_t *action = &es0->entry->action;
    
    if (es0->flags & VTSS_ES0_FLAG_TPID) {
        /* Update TPID action */
        switch (vtss_state->vlan_port_conf[es0->port_no].port_type) {
        case VTSS_VLAN_PORT_TYPE_S:
            action->tpid = VTSS_ES0_TPID_S;
            break;
        case VTSS_VLAN_PORT_TYPE_S_CUSTOM:
            action->tpid = VTSS_ES0_TPID_PORT;
            break;
        default:
            action->tpid = VTSS_ES0_TPID_C;
            break;
        }
#if defined(VTSS_ARCH_SERVAL)
        if (vtss_state->arch == VTSS_ARCH_SRVL) {
            action->outer_tag.tpid = action->tpid;
        }
#endif /* VTSS_ARCH_CARACAL */
    }

    if (es0->flags & VTSS_ES0_FLAG_QOS) {
        /* Update QoS action */
        vtss_qos_port_conf_t *qos = &vtss_state->qos_port_conf[es0->port_no];
        
        action->pcp = qos->tag_default_pcp;
        action->dei = qos->tag_default_dei;
        switch (qos->tag_remark_mode) {
        case VTSS_TAG_REMARK_MODE_CLASSIFIED:
            action->qos = VTSS_ES0_QOS_CLASS; 
            break;
        case VTSS_TAG_REMARK_MODE_MAPPED:
            action->qos = VTSS_ES0_QOS_MAPPED; 
            break;
        case VTSS_TAG_REMARK_MODE_DEFAULT:
        default:
            action->qos = VTSS_ES0_QOS_ES0; 
            break;
        }
#if defined(VTSS_ARCH_SERVAL)
        if (vtss_state->arch == VTSS_ARCH_SRVL) {
            vtss_es0_tag_conf_t *tag = &action->outer_tag;

            tag->pcp.val = action->pcp;
            tag->dei.val = action->dei;
            switch (qos->tag_remark_mode) {
            case VTSS_TAG_REMARK_MODE_CLASSIFIED:
                tag->pcp.sel = VTSS_ES0_PCP_CLASS; 
                tag->dei.sel = VTSS_ES0_DEI_CLASS;
                break;
            case VTSS_TAG_REMARK_MODE_MAPPED:
                tag->pcp.sel = VTSS_ES0_PCP_MAPPED; 
                tag->dei.sel = VTSS_ES0_DEI_MAPPED;
                break;
            case VTSS_TAG_REMARK_MODE_DEFAULT:
            default:
                tag->pcp.sel = VTSS_ES0_PCP_ES0; 
                tag->dei.sel = VTSS_ES0_DEI_ES0;
                break;
            }
        }
#endif /* VTSS_ARCH_CARACAL */
    }
}
#endif /* VTSS_FEATURE_ES0 */

static void vtss_debug_print_range_checkers(const vtss_debug_printf_t pr,
                                            const vtss_debug_info_t   *const info)
{
    u32                   i;
    vtss_vcap_range_chk_t *entry;

    vtss_debug_print_header(pr, "Range Checkers");
    pr("Index  Type  Count  Range\n");
    for (i = 0; i < VTSS_VCAP_RANGE_CHK_CNT; i++) {
        entry = &vtss_state->vcap_range.entry[i];
        pr("%-5u  %-4s  %-5u  %u-%u\n",
           i, 
           entry->type == VTSS_VCAP_RANGE_TYPE_SPORT ? "S" :
           entry->type == VTSS_VCAP_RANGE_TYPE_DPORT ? "D" :
           entry->type == VTSS_VCAP_RANGE_TYPE_SDPORT ? "SD" :
           entry->type == VTSS_VCAP_RANGE_TYPE_VID ? "VID" :
           entry->type == VTSS_VCAP_RANGE_TYPE_DSCP ? "DSCP" : "?",
           entry->count,
           entry->min,
           entry->max);
    }
    pr("\n");
}

static void vtss_debug_print_vcap(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info,
                                  vtss_vcap_obj_t           *obj)
{
    u32                  i, low, high;
    vtss_vcap_entry_t    *cur;
    BOOL                 header = 1;
    vtss_vcap_user_t     user;
    const char           *name;
    vtss_vcap_key_size_t key_size;
    
    vtss_debug_print_header(pr, obj->name);

    pr("max_count     : %u\n", obj->max_count);
    pr("count         : %u\n", obj->count);
    pr("max_rule_count: %u\n", obj->max_rule_count);
    pr("rule_count    : %u\n", obj->rule_count);
    pr("full_count    : %u\n", obj->key_count[VTSS_VCAP_KEY_SIZE_FULL]);
    pr("half_count    : %u\n", obj->key_count[VTSS_VCAP_KEY_SIZE_HALF]);
    pr("quarter_count : %u\n", obj->key_count[VTSS_VCAP_KEY_SIZE_QUARTER]);
    for (cur = obj->used, i = 0; cur != NULL; cur = cur->next, i++) {
        if (header)
            pr("\nIndex  Key Size  User  Name      ID\n");
        header = 0;
        low = (cur->id & 0xffffffff);
        high = ((cur->id >> 32) & 0xffffffff);
        name = "?";
        user = cur->user;
        name = (user == VTSS_IS0_USER_EVC ? "EVC " :
                user == VTSS_IS1_USER_VCL ? "VCL" :
                user == VTSS_IS1_USER_VLAN ? "VLAN" :
                user == VTSS_IS1_USER_MEP ? "MEP " :
                user == VTSS_IS1_USER_EVC ? "EVC " :
                user == VTSS_IS1_USER_QOS ? "QoS" :
                user == VTSS_IS1_USER_SSM ? "SSM" :
                user == VTSS_IS1_USER_ACL ? "ACL" :
                user == VTSS_IS2_USER_IGMP ? "IGMP" :
                user == VTSS_IS2_USER_SSM ? "SSM" :
                user == VTSS_IS2_USER_IGMP_ANY ? "IGMP_ANY" :
                user == VTSS_IS2_USER_EEE ? "EEE" :
                user == VTSS_IS2_USER_ACL_PTP ? "ACL_PTP" :
                user == VTSS_IS2_USER_ACL ? "ACL" :
                user == VTSS_IS2_USER_ACL_SIP ? "ACL_SIP " :
                user == VTSS_ES0_USER_VLAN ? "VLAN" :
                user == VTSS_ES0_USER_MEP ? "MEP " :
                user == VTSS_ES0_USER_EVC ? "EVC " :
                user == VTSS_ES0_USER_TX_TAG? "TX_TAG " : "?");
        key_size = cur->data.key_size;
        pr("%-7u%-10s%-6d%-10s0x%08x:0x%08x (%u:%u)\n", 
           i, vtss_vcap_key_size_txt(key_size), user, name, high, low, high, low);
    }
    pr("\n");
}

#if defined(VTSS_ARCH_JAGUAR_1) && defined(VTSS_FEATURE_EVC)
static void vtss_debug_print_vcap_is0(const vtss_debug_printf_t pr,
                                      const vtss_debug_info_t   *const info)
{
    vtss_debug_print_vcap(pr, info, &vtss_state->is0.obj);
}
#endif /* VTSS_ARCH_JAGUAR_1 && VTSS_FEATURE_EVC */

#if defined(VTSS_FEATURE_IS1)
static void vtss_debug_print_vcap_is1(const vtss_debug_printf_t pr,
                                      const vtss_debug_info_t   *const info)
{
    vtss_debug_print_vcap(pr, info, &vtss_state->is1.obj);
}
#endif /* VTSS_FEATURE_IS1 */    

#if defined(VTSS_FEATURE_IS2)
static void vtss_debug_print_vcap_is2(const vtss_debug_printf_t pr,
                                      const vtss_debug_info_t   *const info)
{
    vtss_debug_print_vcap(pr, info, &vtss_state->is2.obj);
}
#endif /* VTSS_FEATURE_IS2 */    

#if defined(VTSS_FEATURE_ES0)
static void vtss_debug_print_vcap_es0(const vtss_debug_printf_t pr,
                                      const vtss_debug_info_t   *const info)
{
    vtss_debug_print_vcap(pr, info, &vtss_state->es0.obj);
}
#endif /* VTSS_FEATURE_ES0 */    

#endif /* VTSS_FEATURE_VCAP */

#if defined(VTSS_FEATURE_ACL)

/* ================================================================= *
 *  ACL
 * ================================================================= */

/* Add ACE check */
vtss_rc vtss_cmn_ace_add(const vtss_ace_id_t ace_id, const vtss_ace_t *const ace)
{
    /* Check ACE ID */
    if (ace->id == VTSS_ACE_ID_LAST || ace->id == ace_id) {
        VTSS_E("illegal ace id: %u", ace->id);
        return VTSS_RC_ERROR;
    }
    
    /* Check frame type */
    if (ace->type > VTSS_ACE_TYPE_IPV6) {
        VTSS_E("illegal type: %d", ace->type);
        return VTSS_RC_ERROR;
    }
    
    return VTSS_RC_OK;
}

/* Delete ACE */
vtss_rc vtss_cmn_ace_del(const vtss_ace_id_t ace_id)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is2.obj;
    vtss_vcap_data_t data;
    
    if (vtss_vcap_lookup(obj, VTSS_IS2_USER_ACL, ace_id, &data, NULL) != VTSS_RC_OK) {
        VTSS_E("ace_id: %u not found", ace_id);
        return VTSS_RC_ERROR;
    }
    
    /* Delete range checkers and main entry */
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, data.u.is2.srange));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, data.u.is2.drange));
    VTSS_RC(vtss_vcap_del(obj, VTSS_IS2_USER_ACL, ace_id));
    
    return VTSS_RC_OK;
}

/* Get/clear ACE counter */
static vtss_rc vtss_cmn_ace_get(const vtss_ace_id_t ace_id, 
                                vtss_ace_counter_t *const counter, 
                                BOOL clear)
{
    vtss_vcap_idx_t idx;
    vtss_vcap_obj_t *obj = &vtss_state->is2.obj;
    
    if (vtss_vcap_lookup(obj, VTSS_IS2_USER_ACL, ace_id, NULL, &idx) != VTSS_RC_OK) {
        VTSS_E("ace_id: %u not found", ace_id);
        return VTSS_RC_ERROR;
    }
    VTSS_RC(obj->entry_get(&idx, counter, clear));
    
    return VTSS_RC_OK;
}

/* Get ACE counter */
vtss_rc vtss_cmn_ace_counter_get(const vtss_ace_id_t ace_id, vtss_ace_counter_t *const counter)
{
    return vtss_cmn_ace_get(ace_id, counter, 0);
}

/* Clear ACE counter */
vtss_rc vtss_cmn_ace_counter_clear(const vtss_ace_id_t ace_id)
{
    vtss_ace_counter_t counter;
    
    return vtss_cmn_ace_get(ace_id, &counter, 1);
}

static char *vtss_acl_policy_no_txt(vtss_acl_policy_no_t policy_no, char *buf)
{
    if (policy_no == VTSS_ACL_POLICY_NO_NONE)
        strcpy(buf, "None");
    else
        sprintf(buf, "%u", policy_no);
    return buf;
}
#endif /* VTSS_FEATURE_ACL */

#if defined(VTSS_FEATURE_QOS)

/* ================================================================= *
 *  QoS
 * ================================================================= */

vtss_rc vtss_cmn_qos_port_conf_set(const vtss_port_no_t port_no)
{
    vtss_rc                  rc = vtss_state->cil_func.qos_port_conf_update(port_no);
#if defined(VTSS_FEATURE_ES0)
    vtss_qos_port_conf_t     *old = &vtss_state->qos_port_conf_old;
    vtss_qos_port_conf_t     *new = &vtss_state->qos_port_conf[port_no];
    u8                       pcp;
    u16                      flags = 0;

    if (old->tag_remark_mode != new->tag_remark_mode ||
        old->tag_default_pcp != new->tag_default_pcp ||
        old->tag_default_dei != new->tag_default_dei) {
        /* PCP/DEI remark mode changed, update QOS */
        flags |= VTSS_ES0_FLAG_QOS;
    }
    for (pcp = 0; pcp < 8; pcp++) {
        if (old->qos_class_map[pcp][0] != new->qos_class_map[pcp][0]) {
            flags |= VTSS_ES0_FLAG_PCP_MAP;
            break;
        }
    }
    if (flags) {
        /* Update ES0 rules */
        VTSS_RC(vtss_cmn_es0_update(port_no, flags));
    }
#endif /* VTSS_FEATURE_ES0 */
    return rc;
}

/**
 * \brief Convert QoS scheduler weight to cost.
 *
 * \param weight [IN]    Array of weights. Range is 1..100.
 * \param cost [OUT]     Array of costs. Range is 0..(2^bit_width - 1).
 * \param num [IN]       Number of entries in weight and cost.
 * \param bit_width [IN] Bit-width of resulting cost. Range is 4..8. E.g. 5 corresponds to a cost between 0 and 31.
 *
 * \return Return code.
 **/
vtss_rc vtss_cmn_qos_weight2cost(const vtss_pct_t * weight, u8 * cost, size_t num, u8 bit_width)
{
    u32 i, c_max;
    vtss_pct_t w_min = 100;
    if ((bit_width < 4) || (bit_width > 8)) {
        VTSS_E("illegal bit_width: %u", bit_width);
        return VTSS_RC_ERROR;
    }
    c_max = 1 << bit_width;
    for (i = 0; i < num; i++) {
        if ((weight[i] < 1) || (weight[i] > 100)) {
            VTSS_E("illegal weight: %u", weight[i]);
            return VTSS_RC_ERROR;
        }
        w_min = MIN(w_min, weight[i]);
    }
    for (i = 0; i < num; i++) {
        // Round half up: Multiply with 16 before division, add 8 and divide result with 16 again
        u32 c = (((c_max << 4) * w_min / weight[i]) + 8) >> 4;
        cost[i] = MAX(1, c) - 1; // Force range to be 0..(c_max - 1)
    }
    return VTSS_RC_OK;
}

#if defined(VTSS_FEATURE_QCL_V2)
/* Add QCE */
vtss_rc vtss_cmn_qce_add(const vtss_qcl_id_t  qcl_id,
                         const vtss_qce_id_t  qce_id,
                         const vtss_qce_t     *const qce)
{
    vtss_vcap_obj_t             *is1_obj = &vtss_state->is1.obj;
    vtss_vcap_user_t            is1_user = VTSS_IS1_USER_QOS;
    vtss_vcap_data_t            data;
    vtss_is1_data_t             *is1 = &data.u.is1;
    vtss_is1_entry_t            entry;
    vtss_is1_action_t           *action = &entry.action;
    vtss_is1_key_t              *key = &entry.key;
    u32                         i;
    vtss_res_chg_t              res_chg;
    vtss_vcap_key_size_t        key_size = VTSS_VCAP_KEY_SIZE_FULL;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range; /* Make a temporary working copy of the range table */

    /* Check QCE ID */
    if (qce->id == VTSS_QCE_ID_LAST || qce->id == qce_id) {
        VTSS_E("illegal qce id: %u", qce->id);
        return VTSS_RC_ERROR;
    }

    /* Initialize entry data */
    vtss_vcap_is1_init(&data, &entry);
    
    is1->lookup = 1; /* Second lookup */

#if defined(VTSS_ARCH_SERVAL)
    /* For Serval, the default half key is used */
    if (vtss_state->arch == VTSS_ARCH_SRVL) {
        is1->lookup = 2; /* Third lookup */
        key_size = VTSS_VCAP_KEY_SIZE_HALF;
        key->key_type = VTSS_VCAP_KEY_TYPE_NORMAL;
    }
#endif /* VTSS_ARCH_SERVAL */
    
    /* Check if main entry exists */
    memset(&res_chg, 0, sizeof(res_chg));
    if (vtss_vcap_lookup(is1_obj, is1_user, qce->id, &data, NULL) == VTSS_RC_OK) {
        res_chg.del_key[key_size] = 1;
        /* Free eventually old range checkers */
        VTSS_RC(vtss_vcap_range_free(&range_new, is1->vid_range));
        VTSS_RC(vtss_vcap_range_free(&range_new, is1->dscp_range));
        VTSS_RC(vtss_vcap_range_free(&range_new, is1->sport_range));
        VTSS_RC(vtss_vcap_range_free(&range_new, is1->dport_range));
        is1->vid_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->dscp_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->sport_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->dport_range = VTSS_VCAP_RANGE_CHK_NONE;
    }

    /* Check that the entry can be added */
    res_chg.add_key[key_size] = 1;
    VTSS_RC(vtss_cmn_vcap_res_check(is1_obj, &res_chg));
    data.key_size = key_size;

    /* Copy action data */
    action->dscp_enable    = qce->action.dscp_enable;
    action->dscp           = qce->action.dscp;
    action->dp_enable      = qce->action.dp_enable;
    action->dp             = qce->action.dp;
    action->prio_enable    = qce->action.prio_enable;
    action->prio           = qce->action.prio;
#if (defined VTSS_FEATURE_QCL_PCP_DEI_ACTION)
    action->pcp_dei_enable = qce->action.pcp_dei_enable;
    action->pcp            = qce->action.pcp;
    action->dei            = qce->action.dei;
#endif /* (defined VTSS_FEATURE_QCL_PCP_DEI_ACTION) */

    /* Copy key data */
    memcpy(key->port_list, qce->key.port_list, sizeof(key->port_list));

    key->mac.dmac_mc = qce->key.mac.dmac_mc;
    key->mac.dmac_bc = qce->key.mac.dmac_bc;
    if (key->mac.dmac_bc == VTSS_VCAP_BIT_1) {
        key->mac.dmac_mc = VTSS_VCAP_BIT_1; /* mc must be 1 or don't care in order to match on a bc frame */
    }

    for (i = 0; i < 3; i++) {
        /* Copy the 24 bits from source into the upper 24 bits of the 48 bits destination */
        /* and keep the 24 lower bits in the destination as don't cares */
        key->mac.smac.value[i] = qce->key.mac.smac.value[i];
        key->mac.smac.mask[i]  = qce->key.mac.smac.mask[i];
    }

    key->tag.vid    = qce->key.tag.vid;
    VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->vid_range, VTSS_VCAP_RANGE_TYPE_VID, &key->tag.vid));
    key->tag.pcp    = qce->key.tag.pcp;
    key->tag.dei    = qce->key.tag.dei;
    key->tag.tagged = qce->key.tag.tagged;

    switch (qce->key.type) {
    case VTSS_QCE_TYPE_ANY:
        key->type = VTSS_IS1_TYPE_ANY;
        break;
    case VTSS_QCE_TYPE_ETYPE:
        key->type              = VTSS_IS1_TYPE_ETYPE;
        key->frame.etype.etype = qce->key.frame.etype.etype;
        key->frame.etype.data  = qce->key.frame.etype.data;
#if defined(VTSS_ARCH_JAGUAR_1)
        /* Jaguar classifies MPLS frames to EtherType 0x0000. If we want to match
         * specific on MPLS frames we will have to match on EtherType 0x0000 instead.
         *
         * Check if user wants to match on specific MPLS frame (EtherType = 0x8847 or 0x8848) */
        if ((vtss_state->arch == VTSS_ARCH_JR1) &&
            (qce->key.frame.etype.etype.mask[0] == 0xff) &&
            (qce->key.frame.etype.etype.mask[1] == 0xff) &&
            (qce->key.frame.etype.etype.value[0] == 0x88) &&
            ((qce->key.frame.etype.etype.value[1] == 0x47) || (qce->key.frame.etype.etype.value[1] == 0x48))) {
            key->frame.etype.etype.value[0] = 0; /* Change EtherType to 0x0000 */
            key->frame.etype.etype.value[1] = 0;
        }
#endif /* VTSS_ARCH_JAGUAR_1 */
        break;
    case VTSS_QCE_TYPE_LLC:
        key->type           = VTSS_IS1_TYPE_LLC;
        key->frame.llc.data = qce->key.frame.llc.data;
        break;
    case VTSS_QCE_TYPE_SNAP:
        key->type            = VTSS_IS1_TYPE_SNAP;
        key->frame.snap.data = qce->key.frame.snap.data;
        break;
    case VTSS_QCE_TYPE_IPV4:
    case VTSS_QCE_TYPE_IPV6:
        if (qce->key.type == VTSS_QCE_TYPE_IPV4) {
            key->type                = VTSS_IS1_TYPE_IPV4;
            key->frame.ipv4.fragment = qce->key.frame.ipv4.fragment;
            key->frame.ipv4.dscp     = qce->key.frame.ipv4.dscp;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->dscp_range, VTSS_VCAP_RANGE_TYPE_DSCP, &key->frame.ipv4.dscp));
            key->frame.ipv4.proto    = qce->key.frame.ipv4.proto;
            key->frame.ipv4.sip      = qce->key.frame.ipv4.sip;
            key->frame.ipv4.sport    = qce->key.frame.ipv4.sport;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->sport_range, VTSS_VCAP_RANGE_TYPE_SPORT, &key->frame.ipv4.sport));
            key->frame.ipv4.dport    = qce->key.frame.ipv4.dport;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->dport_range, VTSS_VCAP_RANGE_TYPE_DPORT, &key->frame.ipv4.dport));
        } else {
            key->type                = VTSS_IS1_TYPE_IPV6;
            key->frame.ipv6.dscp     = qce->key.frame.ipv6.dscp;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->dscp_range, VTSS_VCAP_RANGE_TYPE_DSCP, &key->frame.ipv6.dscp));
            key->frame.ipv6.proto    = qce->key.frame.ipv6.proto;
            for (i = 0; i < 4; i++) {
                key->frame.ipv6.sip.value[i + 12] = qce->key.frame.ipv6.sip.value[i];
                key->frame.ipv6.sip.mask[i + 12] = qce->key.frame.ipv6.sip.mask[i];
            }
            key->frame.ipv6.sport    = qce->key.frame.ipv6.sport;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->sport_range, VTSS_VCAP_RANGE_TYPE_SPORT, &key->frame.ipv6.sport));
            key->frame.ipv6.dport    = qce->key.frame.ipv6.dport;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->dport_range, VTSS_VCAP_RANGE_TYPE_DPORT, &key->frame.ipv6.dport));
        }
        break;
    default:
        VTSS_E("illegal type: %d", qce->key.type);
        return VTSS_RC_ERROR;
    }

    /* Commit range checkers */
    VTSS_RC(vtss_vcap_range_commit(&range_new));

    /* Add main entry */
    VTSS_RC(vtss_vcap_add(is1_obj, is1_user, qce->id, qce_id, &data, 0));
    return VTSS_RC_OK;
}

/* Delete QCE */
vtss_rc vtss_cmn_qce_del(const vtss_qcl_id_t  qcl_id,
                         const vtss_qce_id_t  qce_id)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is1.obj;
    vtss_vcap_data_t data;
    vtss_is1_data_t  *is1 = &data.u.is1;
    
    if (vtss_vcap_lookup(obj, VTSS_IS1_USER_QOS, qce_id, &data, NULL) != VTSS_RC_OK) {
        VTSS_E("qce_id: %u not found", qce_id);
        return VTSS_RC_ERROR;
    }
    
    /* Delete range checkers and main entry */
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->vid_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->dscp_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->sport_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->dport_range));
    is1->vid_range = VTSS_VCAP_RANGE_CHK_NONE;
    is1->dscp_range = VTSS_VCAP_RANGE_CHK_NONE;
    is1->sport_range = VTSS_VCAP_RANGE_CHK_NONE;
    is1->dport_range = VTSS_VCAP_RANGE_CHK_NONE;
    VTSS_RC(vtss_vcap_del(obj, VTSS_IS1_USER_QOS, qce_id));
    
    return VTSS_RC_OK;
}

#endif /* VTSS_FEATURE_QCL_V2 */
#endif /* VTSS_FEATURE_QOS */

#if defined(VTSS_FEATURE_VCL)
/* Add VCE */
vtss_rc vtss_cmn_vce_add(const vtss_vce_id_t  vce_id,
                         const vtss_vce_t     *const vce)
{
    vtss_vcap_obj_t             *is1_obj = &vtss_state->is1.obj;
    vtss_vcap_user_t            is1_user = VTSS_IS1_USER_VCL;
    vtss_vcap_data_t            data;
    vtss_is1_data_t             *is1 = &data.u.is1;
    vtss_is1_entry_t            entry;
    vtss_is1_action_t           *action = &entry.action;
    vtss_is1_key_t              *key = &entry.key;
    u32                         i;
    vtss_res_chg_t              res_chg;
    vtss_vcap_key_size_t        key_size = VTSS_VCAP_KEY_SIZE_FULL;
    vtss_vcap_range_chk_table_t range_new = vtss_state->vcap_range; /* Make a temporary working copy of the range table */

    /* Check VCE ID */
    if (vce->id == VTSS_VCE_ID_LAST || vce->id == vce_id) {
        VTSS_E("illegal vce id: %u", vce->id);
        return VTSS_RC_ERROR;
    }

    /* Initialize entry data */
    vtss_vcap_is1_init(&data, &entry);

    /* First Lookup */
    is1->lookup = 0;

#if defined(VTSS_ARCH_SERVAL)
    /* For Serval, the default half key is used */
    if (vtss_state->arch == VTSS_ARCH_SRVL) {
        is1->lookup = 1; /* Second lookup */
        key_size = VTSS_VCAP_KEY_SIZE_HALF;
        key->key_type = VTSS_VCAP_KEY_TYPE_NORMAL;
    }
#endif /* VTSS_ARCH_SERVAL */

    /* Check if main entry exists */
    memset(&res_chg, 0, sizeof(res_chg));
    if (vtss_vcap_lookup(is1_obj, is1_user, vce->id, &data, NULL) == VTSS_RC_OK) {
        res_chg.del_key[key_size] = 1;
        /* Free eventually old range checkers */
        VTSS_RC(vtss_vcap_range_free(&range_new, is1->vid_range));
        VTSS_RC(vtss_vcap_range_free(&range_new, is1->dscp_range));
        VTSS_RC(vtss_vcap_range_free(&range_new, is1->sport_range));
        VTSS_RC(vtss_vcap_range_free(&range_new, is1->dport_range));
        is1->vid_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->dscp_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->sport_range = VTSS_VCAP_RANGE_CHK_NONE;
        is1->dport_range = VTSS_VCAP_RANGE_CHK_NONE;
    }

    /* Check that the entry can be added */
    res_chg.add_key[key_size] = 1;
    VTSS_RC(vtss_cmn_vcap_res_check(is1_obj, &res_chg));
    data.key_size = key_size;

    /* Copy action data */
    action->vid = vce->action.vid;

    /* Copy key data */
    memcpy(key->port_list, vce->key.port_list, sizeof(key->port_list));

    key->mac.dmac_mc = vce->key.mac.dmac_mc;
    key->mac.dmac_bc = vce->key.mac.dmac_bc;
    for (i = 0; i < 6; i++) {
        key->mac.smac.value[i] = vce->key.mac.smac.value[i];
        key->mac.smac.mask[i]  = vce->key.mac.smac.mask[i];
    }

    key->tag.vid.type = VTSS_VCAP_VR_TYPE_VALUE_MASK;
    key->tag.vid.vr.v.value = vce->key.tag.vid.value;
    key->tag.vid.vr.v.mask = vce->key.tag.vid.mask;
    key->tag.pcp    = vce->key.tag.pcp;
    key->tag.dei    = vce->key.tag.dei;
    key->tag.tagged = vce->key.tag.tagged;

    switch (vce->key.type) {
    case VTSS_VCE_TYPE_ANY:
        key->type = VTSS_IS1_TYPE_ANY;
        break;
    case VTSS_VCE_TYPE_ETYPE:
        key->type              = VTSS_IS1_TYPE_ETYPE;
        key->frame.etype.etype = vce->key.frame.etype.etype;
        key->frame.etype.data  = vce->key.frame.etype.data;
        break;
    case VTSS_VCE_TYPE_LLC:
        key->type           = VTSS_IS1_TYPE_LLC;
        key->frame.llc.data = vce->key.frame.llc.data;
        break;
    case VTSS_VCE_TYPE_SNAP:
        key->type            = VTSS_IS1_TYPE_SNAP;
        key->frame.snap.data = vce->key.frame.snap.data;
        break;
    case VTSS_VCE_TYPE_IPV4:
    case VTSS_VCE_TYPE_IPV6:
        if (vce->key.type == VTSS_VCE_TYPE_IPV4) {
            key->type                = VTSS_IS1_TYPE_IPV4;
            key->frame.ipv4.fragment = vce->key.frame.ipv4.fragment;
            key->frame.ipv4.dscp     = vce->key.frame.ipv4.dscp;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->dscp_range, VTSS_VCAP_RANGE_TYPE_DSCP, &key->frame.ipv4.dscp));
            key->frame.ipv4.proto    = vce->key.frame.ipv4.proto;
            key->frame.ipv4.sip      = vce->key.frame.ipv4.sip;
            //key->frame.ipv4.sport    = vce->key.frame.ipv4.sport;
            //VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->sport_range, VTSS_VCAP_RANGE_TYPE_SPORT, &key->frame.ipv4.sport));
            key->frame.ipv4.dport    = vce->key.frame.ipv4.dport;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->dport_range, VTSS_VCAP_RANGE_TYPE_DPORT, &key->frame.ipv4.dport));
        } else {
            key->type                = VTSS_IS1_TYPE_IPV6;
            key->frame.ipv6.dscp     = vce->key.frame.ipv6.dscp;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->dscp_range, VTSS_VCAP_RANGE_TYPE_DSCP, &key->frame.ipv6.dscp));
            key->frame.ipv6.proto    = vce->key.frame.ipv6.proto;
            for (i = 0; i < 4; i++) {
                key->frame.ipv6.sip.value[i + 12] = vce->key.frame.ipv6.sip.value[i];
                key->frame.ipv6.sip.mask[i + 12] = vce->key.frame.ipv6.sip.mask[i];
            }
            //key->frame.ipv6.sport    = vce->key.frame.ipv6.sport;
            //VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->sport_range, VTSS_VCAP_RANGE_TYPE_SPORT, &key->frame.ipv6.sport));
            key->frame.ipv6.dport    = vce->key.frame.ipv6.dport;
            VTSS_RC(vtss_vcap_vr_alloc(&range_new, &is1->dport_range, VTSS_VCAP_RANGE_TYPE_DPORT, &key->frame.ipv6.dport));
        }
        break;
    default:
        VTSS_E("illegal type: %d", vce->key.type);
        return VTSS_RC_ERROR;
    }

    /* Commit range checkers */
    VTSS_RC(vtss_vcap_range_commit(&range_new));

    /* Add main entry */
    VTSS_RC(vtss_vcap_add(is1_obj, is1_user, vce->id, vce_id, &data, 0));
    return VTSS_RC_OK;
}

/* Delete VCE */
vtss_rc vtss_cmn_vce_del(const vtss_vce_id_t  vce_id)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is1.obj;
    vtss_vcap_data_t data;
    vtss_is1_data_t  *is1 = &data.u.is1;

    if (vtss_vcap_lookup(obj, VTSS_IS1_USER_VCL, vce_id, &data, NULL) != VTSS_RC_OK) {
        /* This is possilbe as add may fail in some situations due to hardware limitation */
        VTSS_D("vce_id: %u not found", vce_id);
        return VTSS_RC_OK;
    }
    
    /* Delete range checkers and main entry */
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->vid_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->dscp_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->sport_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->dport_range));
    VTSS_RC(vtss_vcap_del(obj, VTSS_IS1_USER_VCL, vce_id));
    
    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_VCL */

#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
static void vtss_debug_print_vlan_trans(const vtss_debug_printf_t pr,
                                        const vtss_debug_info_t   *const info)
{
    vtss_vlan_trans_grp2vlan_conf_t conf;
    vtss_vlan_trans_port2grp_conf_t port_conf;
    BOOL                            next = FALSE, first = TRUE;
    pr("VLAN Translation Group Table\n");
    pr("----------------------------\n");
    memset(&conf, 0, sizeof(vtss_vlan_trans_grp2vlan_conf_t));
    while ((vtss_cmn_vlan_trans_group_get(&conf, next)) == VTSS_RC_OK) {
        if (first == TRUE) {
            pr("GroupID  VID   Trans_VID\n");
            pr("-------  ---   ---------\n");
            first = FALSE;
        }
        pr("%-7u  %-4u  %-4u\n", conf.group_id, conf.vid, conf.trans_vid);
        next = TRUE;
    }
    pr("\nVLAN Translation Port Table\n");
    pr("---------------------------\n");
    next = FALSE;
    first = TRUE;
    //vtss_cmn_vlan_trans_port_conf_get(vtss_vlan_trans_port2grp_conf_t *conf, BOOL next);
    memset(&port_conf, 0, sizeof(vtss_vlan_trans_port2grp_conf_t));
    while ((vtss_cmn_vlan_trans_port_conf_get(&port_conf, next)) == VTSS_RC_OK) {
        if (first == TRUE) {
            pr("GroupID  Ports\n");
            pr("-------  -----\n");
            first = FALSE;
        }
        pr("%-7u  ", port_conf.group_id);
        vtss_debug_print_ports(pr, port_conf.ports, 1);
        next = TRUE;
    }

    pr("\n");
    vtss_debug_print_vcap_is1(pr, info);
    vtss_debug_print_vcap_es0(pr, info);
}
/* Function to check VCAP resources */
static vtss_rc vcap_res_check(vtss_vcap_obj_t *obj, u16 add, u16 del)
{
    if ((obj->count + add - del) > obj->max_count) {
        VTSS_I("VCAP %s exceeded, add: %u, del: %u, count: %u, max: %u", 
               obj->name, add, del, obj->count, obj->max_count);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}
/* VTE(VLAN Translation Entry) ID ->  Ports: 47-0 bits; VID: 60-48 bits */
static void vtss_vt_is1_vte_id_get(const vtss_vid_t vid, const u8 *ports, u64 *const vte_id)
{
    u32     i;
    *vte_id = (u64)vid << 48;
    for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) {
        *vte_id |= ((u64)ports[i] << (i * 8));
    }
}
/* Add IS1 TCAM entry for the VLAN Translation */
static vtss_rc vtss_vt_is1_entry_add(const u64 vte_id, const vtss_vid_t vid, 
                                     const vtss_vid_t trans_vid, const u8 *ports)
{
    vtss_vcap_obj_t             *is1_obj = &vtss_state->is1.obj;
    vtss_vcap_user_t            is1_user = VTSS_IS1_USER_VLAN;
    vtss_vcap_data_t            data;
    vtss_is1_data_t             *is1 = &data.u.is1;
    vtss_is1_entry_t            entry;
    vtss_is1_action_t           *action = &entry.action;
    vtss_is1_key_t              *key = &entry.key;
    u32                         i;
    BOOL                        port_list[VTSS_PORT_ARRAY_SIZE];

    VTSS_D("vtss_vt_is1_entry_add - vte_id: 0x%llx, vid: %u, trans_vid: %u", vte_id, vid, trans_vid);
    /* Initialize entry data */
    vtss_vcap_is1_init(&data, &entry);
    
    /* First Lookup */
    is1->lookup = 0;

#if defined(VTSS_ARCH_SERVAL)
    /* For Serval, the default half key is used */
    if (vtss_state->arch == VTSS_ARCH_SRVL) {
        is1->lookup = 1; /* Second lookup */
        data.key_size = VTSS_VCAP_KEY_SIZE_HALF;
        key->key_type = VTSS_VCAP_KEY_TYPE_NORMAL;
    }
#endif /* VTSS_ARCH_SERVAL */
    
    /* Copy action data */
    action->vid = trans_vid;
    /* Convert Port Bit field to Ports */
    for (i = 0; i < VTSS_PORT_ARRAY_SIZE; i++) {
        port_list[i] = ((ports[i/8]) & (1 << (i % 8))) ? TRUE : FALSE;
    }
    /* Copy key data */
    memcpy(key->port_list, port_list, sizeof(key->port_list));
    /* VID is of type value/mask */
    key->tag.vid.type = VTSS_VCAP_VR_TYPE_VALUE_MASK;
    key->tag.vid.vr.v.value = vid;
    key->tag.vid.vr.v.mask = 0xFFFF;
    /* Allow only for tagged frames */
    key->tag.tagged = VTSS_VCAP_BIT_1;
    key->type = VTSS_IS1_TYPE_ANY;

    /* Add main entry */
    VTSS_RC(vtss_vcap_add(is1_obj, is1_user, vte_id, VTSS_VCAP_ID_LAST, &data, 0));
    return VTSS_RC_OK;
}
/* Delete IS1 TCAM entry for the VLAN Translation */
static vtss_rc vtss_vt_is1_entry_del(const u64 vte_id)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is1.obj;
    vtss_vcap_data_t data;
    vtss_is1_data_t  *is1 = &data.u.is1;

    if (vtss_vcap_lookup(obj, VTSS_IS1_USER_VLAN, vte_id, &data, NULL) != VTSS_RC_OK) {
        VTSS_E("vte_id: %llu not found", vte_id);
        return VTSS_RC_ERROR;
    }
    /* Delete range checkers and main entry */
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->vid_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->dscp_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->sport_range));
    VTSS_RC(vtss_vcap_range_free(&vtss_state->vcap_range, is1->dport_range));
    VTSS_RC(vtss_vcap_del(obj, VTSS_IS1_USER_VLAN, vte_id));
    return VTSS_RC_OK;
}
/* ES0 index derived from VID (11:0 bits) and Port number (64:12 bits) */ 
static void vtss_vt_es0_vte_id_get(const vtss_vid_t vid, const vtss_port_no_t port_no,
                                   vtss_vcap_id_t *const vte_id)
{
    *vte_id = (((u64)port_no << 12) | (vid & 0xFFF));
}
/* VLAN Translation function to add entry to ES0 */
static vtss_rc vtss_vt_es0_entry_add(const vtss_vid_t trans_vid, const vtss_vid_t vid,
                                     const vtss_port_no_t port_no)
{
    vtss_vcap_data_t data;
    vtss_es0_entry_t entry;
    vtss_vcap_id_t   id;

    vtss_vt_es0_vte_id_get(vid, port_no, &id);
    vtss_cmn_es0_data_set(port_no, trans_vid, vid, 1, &data, &entry);

    return vtss_vcap_add(&vtss_state->es0.obj, VTSS_ES0_USER_VLAN, id, VTSS_VCAP_ID_LAST, &data, 0);
}
/* VLAN Translation function to delete entry to ES0 */
static vtss_rc vtss_vt_es0_entry_del(const vtss_vid_t vid, const vtss_port_no_t port_no)
{
    vtss_vcap_id_t      id;
    vtss_vt_es0_vte_id_get(vid, port_no, &id);
    return vtss_vcap_del(&vtss_state->es0.obj, VTSS_ES0_USER_VLAN, id);
}
/* VLAN Translation function to check if group already exists in VLAN Translation list */
static vtss_rc vtss_vlan_trans_group_list_entry_exists(const u16 group_id, const vtss_vid_t vid, 
                                                       BOOL *exists)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp;
    *exists = FALSE;
    list = &vtss_state->vt_trans_conf;
    for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
        if ((group_id == tmp->conf.group_id) && (vid == tmp->conf.vid)) {
            *exists = TRUE;
        }
    }
    return VTSS_RC_OK;
}
/* Helper function to calculate number of set bits in a byte */
static u8 bitcount(u8 n)
{
    u8 cnt = 0;
    while (n)
    {
        n &= (n-1);
        cnt++;
    }
    return cnt;
}
/* Add entry to the VLAN Translation group list */
static vtss_rc vtss_vlan_trans_group_list_add(const u16 group_id, const vtss_vid_t vid, 
                                              const vtss_vid_t trans_vid)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp, *prev, *new = NULL;
    BOOL                                update = FALSE;
    list = &vtss_state->vt_trans_conf;
    /* Insert the new node into the used list */
    for (tmp = list->used, prev = NULL; tmp != NULL; prev = tmp, tmp = tmp->next) {
        /* Check to see if the vid is already configured for this group */
        if ((group_id == tmp->conf.group_id) && (vid == tmp->conf.vid)) {
            /* Overwrite the trans_vid */
            tmp->conf.trans_vid = trans_vid;
            update = TRUE;
            break;
        }
        /* List needs to be sorted based on group_id */
        if (group_id > tmp->conf.group_id) {
            break;
        }
    }
    if (update == FALSE) {
        /* Get free node from free list */
        new = list->free;
        if (new == NULL) {  /* No free entry exists */
            return VTSS_RC_ERROR;
        }   
        /* Update the free list */
        list->free = new->next;
        /* Copy the configuration */
        new->conf.group_id = group_id;
        new->conf.vid = vid;
        new->conf.trans_vid = trans_vid;
        new->next = NULL;
        /* Group to VLAN mapping can be added in to the used list in three ways:
           1. Add at the beginning of the list;
           2. Add somewhere in the middle of the list;
           3. Add at the end of the list.
         */
        if (tmp != NULL) { /* This means insertion point found */
            if (prev == NULL) { 
                /* Add at the beginning of the list */
                new->next = list->used;
                list->used = new;
            } else {
                /* Add somewhere in the middle of the list */
                prev->next = new;
                new->next = tmp;
            }
        } else { /* insertion point not found; add at the end of the list */
            if (prev == NULL) { /* used list is empty */
                list->used = new;
            } else {
                prev->next = new;
            } /* if (prev == NULL) */
        } /* if (tmp != NULL) */
    } /* if (update == FALSE) */
    return VTSS_RC_OK;
}
/* Delete entry to the VLAN Translation group list */
static vtss_rc vtss_vlan_trans_group_list_del(const u16 group_id, const vtss_vid_t vid)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp, *prev;
    list = &vtss_state->vt_trans_conf;
    /* Search used list to find out matching entry */
    for (tmp = list->used, prev = NULL; tmp != NULL; prev = tmp, tmp = tmp->next) {
        if ((group_id == tmp->conf.group_id) && (vid == tmp->conf.vid)) {
            break;
        }
    }
    if (tmp != NULL) { /* This means deletion point found */
        if (prev == NULL) { /* Delete the first node */
            /* Remove from the used list */
            list->used = tmp->next;
        } else {
            /* Remove from the used list */
            prev->next = tmp->next;
        }
        /* Add to the free list */
        tmp->next = list->free;
        list->free = tmp;
    }
    return ((tmp != NULL) ? VTSS_RC_OK : VTSS_RC_ERROR);
}
static vtss_rc vtss_vlan_trans_group_list_get(const u16 group_id, const vtss_vid_t vid,
                                              vtss_vlan_trans_grp2vlan_conf_t *conf)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp;
    list = &vtss_state->vt_trans_conf;
    /* Search used list to find out matching entry */
    for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
        if ((group_id == tmp->conf.group_id) && (vid == tmp->conf.vid)) {
            *conf = tmp->conf;
            break;
        }
    }
    return ((tmp != NULL) ? VTSS_RC_OK : VTSS_RC_ERROR);
}
/* VLAN Translation function to add IS1 and ES0 entries for a group */
static void vtss_vlan_trans_group_hw_entries_add(const u16 group_id, u8 *ports)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp;
    u64                                 vte_id = 0;
    u32                                 i, j;
    vtss_port_no_t                      port_no;
    list = &vtss_state->vt_trans_conf;
    /* Search used list to find out matching entry */
    for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
        if (group_id == tmp->conf.group_id) {
            vtss_vt_is1_vte_id_get(tmp->conf.vid, ports, &vte_id);
            if ((vtss_vt_is1_entry_add(vte_id, tmp->conf.vid, tmp->conf.trans_vid, ports)) != VTSS_RC_OK) {
                VTSS_D("vtss_vlan_trans_group_hw_entries_add: IS1 entry add failed");
            } /* if ((vtss_vt_is1_entry_add */
            /* Adding ES0 TCAM entry for egress VLAN Translation */
            for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) {
                for (j = 0; j < 8; j++) {
                    if (ports[i] & (1 << j)) {
                        port_no = (i * 8) + j;
                        if (vtss_vt_es0_entry_add(tmp->conf.trans_vid, tmp->conf.vid, port_no)) {
                            VTSS_D("vtss_vlan_trans_group_hw_entries_add: ES0 entry add failed");
                        } /* if (vtss_vt_es0_entry_add */
                    } /* if (ports[i] */
                } /* for (j = 0; j < 8; j++)  */
            } /* for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) */
        } /* if (group_id == tmp->conf.group_id) */
    } /* for (tmp = list->used, prev = NULL */
}
/* VLAN Translation function to delete IS1 and ES0 entries for a group */
static void vtss_vlan_trans_group_hw_entries_del(const u16 group_id, u8 *ports)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp;
    u64                                 vte_id = 0;
    u32                                 i, j;
    vtss_port_no_t                      port_no;
    list = &vtss_state->vt_trans_conf;
    /* Search used list to find out matching entry */
    for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
        if (group_id == tmp->conf.group_id) {
            vtss_vt_is1_vte_id_get(tmp->conf.vid, ports, &vte_id);
            if ((vtss_vt_is1_entry_del(vte_id)) != VTSS_RC_OK) {
                VTSS_D("vtss_vlan_trans_group_hw_entries_del: IS1 entry delete failed");
            } /* if ((vtss_vt_is1_entry_del( */
            /* Delete ES0 TCAM entries for egress VLAN Translation */
            for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) {
                for (j = 0; j < 8; j++) {
                    if (ports[i] & (1 << j)) {
                        port_no = (i * 8) + j;
                        if (vtss_vt_es0_entry_del(tmp->conf.trans_vid, port_no) != VTSS_RC_OK) {
                            VTSS_D("vtss_vlan_trans_group_hw_entries_del: ES0 entry delete failed");
                        } /* if (vtss_vt_es0_entry_del */
                    } /* if (ports[i] & (1 << j)) */
                } /* for (j = 0; j < 8; j++) */
            } /* for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) */
        } /* if (group_id == tmp->conf.group_id) */
    } /* for (tmp = list->used; */
}
/* VLAN Translation helper function to update all the IS1 and ES0 entries based on port list */
static vtss_rc vtss_vlan_trans_group_port_list_update(const u16 group_id, const u8 *ports, 
                                                      const u8 *new_ports, const BOOL update)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp;
    u64                                 vte_id = 0;
    u32                                 i, j;
    vtss_port_no_t                      port_no;
    list = &vtss_state->vt_trans_conf;
    /* Search used list to find out matching entry */
    for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
        if (group_id == tmp->conf.group_id) {
            if (update == TRUE) {
                vtss_vt_is1_vte_id_get(tmp->conf.vid, ports, &vte_id);
                if ((vtss_vt_is1_entry_del(vte_id)) != VTSS_RC_OK) {
                    VTSS_D("vtss_vlan_trans_group_port_list_update: IS1 entry delete failed");
                } /* if ((vtss_vt_is1_entry_del( */
                /* Delete ES0 TCAM entries for egress VLAN Translation */
                for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) {
                    for (j = 0; j < 8; j++) {
                        if (ports[i] & (1 << j)) {
                            port_no = (i * 8) + j;
                            if (vtss_vt_es0_entry_del(tmp->conf.trans_vid, port_no)) {
                                VTSS_D("vtss_vlan_trans_group_port_list_update: ES0 entry delete failed");
                            }
                        } /* if (ports[i] */
                    } /* for (j = 0; j < 8; */
                } /* for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE */
            } /* if (update == TRUE */
            vtss_vt_is1_vte_id_get(tmp->conf.vid, new_ports, &vte_id);
            if ((vtss_vt_is1_entry_add(vte_id, tmp->conf.vid, tmp->conf.trans_vid, 
                                       new_ports)) != VTSS_RC_OK) {
                VTSS_D("vtss_vlan_trans_group_port_list_update: IS1 entry addition failed");
            } /* if ((vtss_vt_is1_entry_add( */
            /* Adding ES0 TCAM entry for egress VLAN Translation */
            for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) {
                for (j = 0; j < 8; j++) {
                    if (new_ports[i] & (1 << j)) {
                        port_no = (i * 8) + j;
                        if (vtss_vt_es0_entry_add(tmp->conf.trans_vid, tmp->conf.vid, port_no)) {
                            VTSS_D("vtss_vlan_trans_group_port_list_update: ES0 entry addition failed");
                        } /* if (vtss_vt_es0_entry_add */
                    } /* if (new_ports[i] */
                } /* for (j = 0; j < 8; j++) */
            } /* for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE */
        } /* if (group_id == tmp->conf.group_id) */
    } /* for (tmp = list->used; */
    return VTSS_RC_OK;
}
/* VLAN Translation function to fetch port_list for this group */
static vtss_rc vtss_vlan_trans_port_list_get(const u16 group_id, u8 *ports)
{
    vtss_vlan_trans_port2grp_t          *list;
    vtss_vlan_trans_port2grp_entry_t    *tmp;
    BOOL                                found = FALSE;
    list = &vtss_state->vt_port_conf;
    memset(ports, 0, VTSS_VLAN_TRANS_PORT_BF_SIZE);
    for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
        if (tmp->conf.group_id == group_id) {
            memcpy(ports, tmp->conf.ports, VTSS_VLAN_TRANS_PORT_BF_SIZE);
            found = TRUE;
            break;
        }
    }
    return ((found == TRUE) ? VTSS_RC_OK : VTSS_RC_ERROR);
}
static void vtss_vlan_trans_group_trans_cnt(const u16 group_id, u32 *count)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp;
    u32                                 cnt = 0;
    list = &vtss_state->vt_trans_conf;
    /* Search used list to find out matching entry */
    for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
        if (group_id == tmp->conf.group_id) {
            cnt++;
        }
    }
    *count = cnt;
}
static vtss_rc vtss_vlan_trans_res_check(const u16 group_id, const u8 *ports)
{
    vtss_vlan_trans_port2grp_t          *port_list;
    vtss_vlan_trans_port2grp_entry_t    *tmp;
    BOOL                                del_entry, grp_found = FALSE;
    u8                                  tmp_ports[VTSS_PORT_BF_SIZE];
    u32                                 i, k, port_cnt, tmp_cnt, grp_cnt = 0;
    u32                                 orig_port_cnt = 0;
    u32                                 is1_add = 0, is1_del = 0, es0_add = 0, es0_del = 0;
    port_list = &vtss_state->vt_port_conf;
    for (tmp = port_list->used; tmp != NULL; tmp = tmp->next) {
        memcpy(tmp_ports, tmp->conf.ports, VTSS_PORT_BF_SIZE);
        if (group_id != tmp->conf.group_id) {
            port_cnt = 0;
            tmp_cnt = 0;
            del_entry = FALSE;
            for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                for (k = 0; k < 8; k++) {
                    if ((ports[i] & (1 << k)) && (tmp->conf.ports[i] & (1 << k))) {
                        port_cnt++;
                    }
                }
                /* Remove the ports */
                tmp_ports[i] &= ~(ports[i]);
                if (tmp_ports[i] == 0) {
                    tmp_cnt++;
                } /* if (tmp_ports[i] == 0) */
            } /* for (i = 0; i < VTSS_PORT_BF_SIZE; */
            /* Check if entry needs to be deleted */
            if (tmp_cnt == VTSS_PORT_BF_SIZE) {
                del_entry = TRUE;
            }
            if (port_cnt != 0) {
                /* Get number of VLAN Translations defined for this group */
                vtss_vlan_trans_group_trans_cnt(tmp->conf.group_id, &grp_cnt);
                if ((del_entry == TRUE) && (grp_cnt != 0)) {
                    is1_del++;
                }
                es0_del += (port_cnt * grp_cnt);
            } /* if (port_cnt != 0) */
        } else {
            grp_found = TRUE;
            orig_port_cnt = 0;
            port_cnt = 0;
            for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                orig_port_cnt += bitcount(tmp->conf.ports[i]);
                tmp_ports[i] = tmp->conf.ports[i] | ports[i];
                port_cnt += bitcount(tmp_ports[i]);
            }
            /* Get number of VLAN Translations defined for this group */
            vtss_vlan_trans_group_trans_cnt(group_id, &grp_cnt);
            VTSS_D("grp_cnt = %u, port_cnt = %u, orig_port_cnt = %u", grp_cnt, port_cnt, orig_port_cnt);
            /* IS1 entry will only get updated */
            es0_add += (grp_cnt * (port_cnt - orig_port_cnt));
        } /* if (group_id != tmp->conf.group_id) */
    } /* for (tmp = port_list->used; tmp != NULL */
    if (grp_found == FALSE) {
        port_cnt = 0;
        for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
            port_cnt += bitcount(ports[i]);
        }
        /* Get number of VLAN Translations defined for this group */
        vtss_vlan_trans_group_trans_cnt(group_id, &grp_cnt);
        is1_add += grp_cnt;
        VTSS_D("grp_cnt = %u, port_cnt = %u", grp_cnt, port_cnt);
        es0_add += grp_cnt * port_cnt;
    }
    VTSS_D("is1_add = %u, is1_del = %u, es0_add = %u, es0_del = %u\n", is1_add, is1_del, es0_add, es0_del);
    VTSS_RC(vcap_res_check(&vtss_state->is1.obj, is1_add, is1_del));
    VTSS_RC(vcap_res_check(&vtss_state->es0.obj, es0_add, es0_del));
    return VTSS_RC_OK;
}
/* VLAN Translation function to update all the port to group mappings and delete if none 
   of the ports exist for a group */
static vtss_rc vtss_vlan_trans_port_list_del(const u8 *ports)
{
    vtss_vlan_trans_port2grp_t          *list;
    vtss_vlan_trans_port2grp_entry_t    *tmp, *prev;
    BOOL                                modified_entry;
    u8                                  orig_ports[VTSS_PORT_BF_SIZE];
    u32                                 i, j;
    list = &vtss_state->vt_port_conf;
    /* Delete all the port to group mappings corresponding to the ports list */
    for (tmp = list->used, prev = NULL; tmp != NULL;) {
        modified_entry = FALSE;
        memcpy(orig_ports, tmp->conf.ports, VTSS_PORT_BF_SIZE);
        for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
            if (ports[i] & tmp->conf.ports[i]) {
                modified_entry = TRUE;
                /* Remove the ports */
                tmp->conf.ports[i] &= ~(ports[i]);
            } /* if (ports[i] & tmp->conf.ports[i]) */
        } /* for (i = 0; i < VTSS_PORT_BF_SIZE; i++) */
        /* As a result of port deletes, if an entry exists with no ports, delete the entry */
        if (modified_entry == TRUE) {
            vtss_vlan_trans_group_hw_entries_del(tmp->conf.group_id, orig_ports);
            for (i = 0, j = 0; i < VTSS_PORT_BF_SIZE; i++) {
                if (tmp->conf.ports[i] == 0) {
                    j++;
                }
            }
            if (j == VTSS_PORT_BF_SIZE) { /* None of the ports is valid, so delete the entry */
                if (prev == NULL) { /* Delete the first node */
                    /* Remove from the used list */
                    list->used = tmp->next;
                } else {
                    /* Remove from the used list */
                    prev->next = tmp->next;
                }
                /* Add to the free list */
                tmp->next = list->free;
                list->free = tmp;
                /* Update tmp to continue the loop */
                if (prev == NULL) { /* This is the first node in used list */
                    tmp = list->used;
                    continue;
                } else {
                    tmp = prev;
                } /* if (prev == NULL) */
            } else { /* If some ports are valid, we need to add the IS1 and ES0 entries */
                vtss_vlan_trans_group_hw_entries_add(tmp->conf.group_id, tmp->conf.ports);
            } /* if (j == VTSS_PORT_BF_SIZE */
        } /* if (modified_entry == TRUE */
        prev = tmp;
        tmp = tmp->next;
    } /* for (tmp = list->used, */
    return VTSS_RC_OK;
}
/* VLAN Translation function to add VLAN Translation entry into a group */
vtss_rc vtss_cmn_vlan_trans_group_add(const u16 group_id, const vtss_vid_t vid, const vtss_vid_t trans_vid)
{
    u8                                  ports[VTSS_VLAN_TRANS_PORT_BF_SIZE], port_cnt = 0;
    u64                                 vte_id = 0;
    u32                                 i, j, tmp = VTSS_VLAN_TRANS_PORT_BF_SIZE;
    vtss_port_no_t                      port_no;
    BOOL                                entry_exist = FALSE, ports_exist = TRUE;
    memset(ports, 0, VTSS_VLAN_TRANS_PORT_BF_SIZE);
    /* Fetch port_list for this group */
    if ((vtss_vlan_trans_port_list_get(group_id, ports)) != VTSS_RC_OK) {
        ports_exist = FALSE;
        VTSS_D("No Port mapping for this group");
    }
    /* VTSS_VLAN_TRANS_PORT_BF_SIZE cannot exceed 6 bytes i.e; 48 ports */
    if (tmp > 6) {
        VTSS_E("VLAN Translation: Chips with port count greater than 48 are not supported");
        return VTSS_RC_ERROR;
    }
    if (ports_exist == TRUE) {
        for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) {
            port_cnt += bitcount(ports[i]);
        }
        VTSS_I("VLAN Translation: Port Count = %u", port_cnt);
        VTSS_RC(vtss_vlan_trans_group_list_entry_exists(group_id, vid, &entry_exist));
        /* We need one IS1 entry and port_cnt number of ES0 VLAN Translations. Check for VCAP resources */
        if (entry_exist == FALSE) { /* If an entry already exists, we just need to update the entry */
            VTSS_RC(vcap_res_check(&vtss_state->is1.obj, 1, 0));
            VTSS_RC(vcap_res_check(&vtss_state->es0.obj, port_cnt, 0));
        }
    }
    /* Add to the state list */
    if ((vtss_vlan_trans_group_list_add(group_id, vid, trans_vid)) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    if (ports_exist == TRUE) {
        vtss_vt_is1_vte_id_get(vid, ports, &vte_id);
        /* Add IS1 TCAM entry for ingress VLAN Translation */
        if ((vtss_vt_is1_entry_add(vte_id, vid, trans_vid, ports)) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }
        /* Adding ES0 TCAM entry for egress VLAN Translation */
        for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) {
            for (j = 0; j < 8; j++) {
                if (ports[i] & (1 << j)) {
                    port_no = (i * 8) + j;
                    VTSS_RC(vtss_vt_es0_entry_add(trans_vid, vid, port_no));
                }
            }
        }
    }
    return VTSS_RC_OK;
}
/* VLAN Translation function to delete a VLAN Translation entry from a group */
vtss_rc vtss_cmn_vlan_trans_group_del(const u16 group_id, const vtss_vid_t vid)
{
    u8                                  ports[VTSS_VLAN_TRANS_PORT_BF_SIZE];
    u64                                 vte_id = 0;
    u32                                 i, j;
    vtss_port_no_t                      port_no;
    vtss_vlan_trans_grp2vlan_conf_t     conf;
    BOOL                                ports_exist = TRUE;
    memset(ports, 0, VTSS_VLAN_TRANS_PORT_BF_SIZE);
    /* Fetch port_list for this group */
    if ((vtss_vlan_trans_port_list_get(group_id, ports)) != VTSS_RC_OK) {
        ports_exist = FALSE;
        VTSS_D("No Port mapping for this group");
    }
    if (ports_exist == TRUE) {
        vtss_vt_is1_vte_id_get(vid, ports, &vte_id);
        /* Delete IS1 TCAM entry for ingress VLAN Translation */
        if ((vtss_vt_is1_entry_del(vte_id)) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }
        /* Get trans_vid to delete ES0 entry */
        VTSS_RC(vtss_vlan_trans_group_list_get(group_id, vid, &conf));
        /* Delete ES0 TCAM entries for egress VLAN Translation */
        for (i = 0; i < VTSS_VLAN_TRANS_PORT_BF_SIZE; i++) {
            for (j = 0; j < 8; j++) {
                if (ports[i] & (1 << j)) {
                    port_no = (i * 8) + j;
                    VTSS_RC(vtss_vt_es0_entry_del(vid, port_no));
                }
            }
        }
    }
    /* Delete from the state list */
    if ((vtss_vlan_trans_group_list_del(group_id, vid)) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}
/* VLAN Translation function to fetch a VLAN Translation entry for a group; next parameter is only valid 
   if both the group_id and vid are valid */
vtss_rc vtss_cmn_vlan_trans_group_get(vtss_vlan_trans_grp2vlan_conf_t *conf, BOOL next)
{
    vtss_vlan_trans_grp2vlan_t          *list;
    vtss_vlan_trans_grp2vlan_entry_t    *tmp;
    BOOL                                found = FALSE, next_entry = FALSE;
    if (conf == NULL) {
        VTSS_E("VLAN Translation: NULL pointer");
        return VTSS_RC_ERROR;
    }
    list = &vtss_state->vt_trans_conf;
    if (list->used == NULL) {
        VTSS_D("Group list is empty");
        return VTSS_RC_ERROR;
    }
    /* If group_id is 0, return first entry */
    if (conf->group_id == VTSS_VLAN_TRANS_NULL_GROUP_ID) {
        *conf = list->used->conf;
        found = TRUE;
    } else {
        for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
            if (next_entry == TRUE) {
                *conf = tmp->conf;
                found = TRUE;
                break;
            }
            if (conf->group_id == tmp->conf.group_id) {
                /* Return first entry if vid is 0 or return matching entry */
                if ((conf->vid == 0) || (conf->vid == tmp->conf.vid)) {
                    if ((next == FALSE) || (conf->vid == 0)) {
                        *conf = tmp->conf;
                        found = TRUE;
                        break;
                    } else {
                        next_entry = TRUE;
                    } /* if (next == FALSE) */
                } /* if ((conf->vid == 0) || (conf->vid == tmp->conf.vid)) */
            } /* if (conf->group_id == tmp->conf.group_id) */
        } /* for (tmp = list->used; tmp != NULL; tmp = tmp->next) */
    } /* if (conf->group_id == VTSS_VLAN_TRANS_NULL_GROUP_ID) */
    return ((found == TRUE) ? VTSS_RC_OK : VTSS_RC_ERROR);
}
/* VLAN Translation function to associate ports to a group. Only one port can be part 
   of one group not multiple groups */
vtss_rc vtss_cmn_vlan_trans_port_conf_set(const vtss_vlan_trans_port2grp_conf_t *conf)
{
    vtss_vlan_trans_port2grp_t          *list;
    vtss_vlan_trans_port2grp_entry_t    *tmp, *prev, *new = NULL;
    u32                                 i;
    BOOL                                update_entry = FALSE;
    u8                                  orig_ports[VTSS_PORT_BF_SIZE];
    if ((vtss_vlan_trans_res_check(conf->group_id, conf->ports)) != VTSS_RC_OK) {
        VTSS_D("VLAN Translation: Not enough resources to add the entry");
        return VTSS_RC_ERROR;
    }
    if ((vtss_vlan_trans_port_list_del(conf->ports)) != VTSS_RC_OK) {
        VTSS_D("VLAN Translation: vtss_vlan_trans_port_list_del failed");
    }
    list = &vtss_state->vt_port_conf;
    /* Insert the new node into the used list */
    for (tmp = list->used, prev = NULL; tmp != NULL; prev = tmp, tmp = tmp->next) {
        if (conf->group_id == tmp->conf.group_id) {
            memcpy(orig_ports, tmp->conf.ports, VTSS_PORT_BF_SIZE);
            for (i = 0; i < VTSS_PORT_BF_SIZE; i++) {
                tmp->conf.ports[i] |= conf->ports[i]; 
            }
            update_entry = TRUE;
            if (vtss_vlan_trans_group_port_list_update(conf->group_id, orig_ports,
                                                       tmp->conf.ports, TRUE) != VTSS_RC_OK)
            {
                VTSS_D("VLAN Translation: vtss_vlan_trans_group_port_list_update failed");
            }
            break;
        }
        /* List needs to be sorted based on group_id */
        if (conf->group_id > tmp->conf.group_id) {
            break;
        }
    }
    if (update_entry == FALSE) {
        /* Get free node from free list */
        new = list->free;
        if (new == NULL) {  /* No free entry exists */
            return VTSS_RC_ERROR;
        }   
        /* Update the free list */
        list->free = new->next;
        /* Copy the configuration */
        new->conf.group_id = conf->group_id;
        memcpy(new->conf.ports, conf->ports, VTSS_PORT_BF_SIZE);
        new->next = NULL;
        /* Port to Group mapping can be added in to the used list in three ways:
           1. Add at the beginning of the list;
           2. Add somewhere in the middle of the list;
           3. Add at the end of the list.
         */
        if (tmp != NULL) { /* This means insertion point found */
            if (prev == NULL) { 
                /* Add at the beginning of the list */
                new->next = list->used;
                list->used = new;
            } else {
                /* Add somewhere in the middle of the list */
                prev->next = new;
                new->next = tmp;
            }
        } else { /* insertion point not found; add at the end of the list */
            if (prev == NULL) { /* used list is empty */
                list->used = new;
            } else {
                prev->next = new;
            }
        }
        memset(orig_ports, 0, VTSS_PORT_BF_SIZE);
        if (vtss_vlan_trans_group_port_list_update(conf->group_id, orig_ports, 
                                                   conf->ports, FALSE) != VTSS_RC_OK)
        {
            VTSS_D("VLAN Translation: vtss_vlan_trans_group_port_list_update failed");
        }
    }
    return VTSS_RC_OK;
}
/* VLAN Translation function to fetch all ports for a group */
vtss_rc vtss_cmn_vlan_trans_port_conf_get(vtss_vlan_trans_port2grp_conf_t *conf, BOOL next)
{
    vtss_vlan_trans_port2grp_t          *list;
    vtss_vlan_trans_port2grp_entry_t    *tmp;
    BOOL                                found = FALSE, next_entry = FALSE;
    if (conf == NULL) {
        VTSS_E("VLAN Translation: NULL pointer");
        return VTSS_RC_ERROR;
    }
    list = &vtss_state->vt_port_conf;
    if (list->used == NULL) {
        VTSS_D("Port list is empty");
        return VTSS_RC_ERROR;
    }
    /* If group_id is 0, return first entry */
    if (conf->group_id == VTSS_VLAN_TRANS_NULL_GROUP_ID) {
        *conf = list->used->conf;
        found = TRUE;
    } else {
        for (tmp = list->used; tmp != NULL; tmp = tmp->next) {
            if (next_entry == TRUE) {
                *conf = tmp->conf;
                found = TRUE;
                break;
            }
            if (conf->group_id == tmp->conf.group_id) {
                if (next == FALSE) {
                    *conf = tmp->conf;
                    found = TRUE;
                    break;
                } else {
                    next_entry = TRUE;
                } /* if (next == FALSE) */
            } /* if (conf->group_id == tmp->conf.group_id) */
        } /* for (tmp = list->used; tmp != NULL; tmp = tmp->next) */
    } /* if (conf->group_id == VTSS_VLAN_TRANS_NULL_GROUP_ID) */
    return ((found == TRUE) ? VTSS_RC_OK : VTSS_RC_ERROR);
}
#endif /* VTSS_FEATURE_VLAN_TRANSLATION */

/* ================================================================= *
 *  EVCs
 * ================================================================= */

#if defined(VTSS_SDX_CNT)
vtss_sdx_entry_t *vtss_cmn_sdx_alloc(vtss_port_no_t port_no, BOOL isdx)
{
    vtss_sdx_entry_t *sdx;
    vtss_sdx_list_t  *list = (isdx ? &vtss_state->sdx_info.isdx : &vtss_state->sdx_info.esdx);

    if ((sdx = list->free) == NULL) {
        VTSS_E("%sSDX alloc failed, port_no: %u", isdx ? "I" : "E", port_no);
        return NULL;
    }

    /* Take out of free list */
    list->free = sdx->next;
    list->count++;
    sdx->port_no = port_no;
    sdx->next = NULL;

    /* Clear counters */
    (void)VTSS_FUNC(sdx_counters_update, isdx ? sdx : NULL, isdx ? NULL : sdx, NULL, 1);

    return sdx;
}

void vtss_cmn_sdx_free(vtss_sdx_entry_t *sdx, BOOL isdx)
{
    vtss_sdx_list_t *list = (isdx ? &vtss_state->sdx_info.isdx : &vtss_state->sdx_info.esdx);
    
    sdx->port_no = VTSS_PORT_NO_NONE;
    sdx->next = list->free;
    list->free = sdx;
    list->count--;
}

vtss_sdx_entry_t *vtss_cmn_ece_sdx_alloc(vtss_ece_entry_t *ece, vtss_port_no_t port_no, BOOL isdx)
{
    vtss_sdx_entry_t *cur, *prev = NULL, **list = (isdx ? &ece->isdx_list : &ece->esdx_list);

    for (cur = *list; cur != NULL; prev = cur, cur = cur->next) {
        if (cur->port_no == port_no) {
            /* Reallocate */
            return cur;
        } else if (cur->port_no > port_no) {
            /* Found place to insert new entry */
            break;
        }
    }

    if ((cur = vtss_cmn_sdx_alloc(port_no, isdx)) == NULL) {
        VTSS_E("SDX alloc failed, ece_id: %u, port_no: %u", ece->ece_id, port_no);
        return NULL;
    }

    /* Insert in ECE list */
    if (prev == NULL) {
        cur->next = *list;
        *list = cur;
    } else {
        cur->next = prev->next;
        prev->next = cur;
    }

    return cur;
}

vtss_rc vtss_cmn_ece_sdx_free(vtss_ece_entry_t *ece, vtss_port_no_t port_no, BOOL isdx)
{
    vtss_sdx_entry_t *cur, *prev = NULL, **list = (isdx ? &ece->isdx_list : &ece->esdx_list);
    
    for (cur = *list; cur != NULL; prev = cur, cur = cur->next) {
        if (cur->port_no == port_no) {
            /* Found entry, move to free list */
            if (prev == NULL)
                *list = cur->next;
            else
                prev->next = cur->next;
            vtss_cmn_sdx_free(cur, isdx);
            return VTSS_RC_OK;
        }
    }

    VTSS_E("%sSDX free failed, ece_id: %u, port_no: %u", isdx ? "I" : "E", ece->ece_id, port_no);
    return VTSS_RC_ERROR;
}
#endif /* VTSS_SDX_CNT */

#if defined(VTSS_FEATURE_EVC)

/* EVC port add/delete/update flags */
#define EVC_PORT_NONE 0x00
#define EVC_PORT_OLD  0x01 /* Port was enabled before */
#define EVC_PORT_NEW  0x02 /* Port is enabled now */
#define EVC_PORT_UNI  0x04 /* Port is UNI */
#define EVC_PORT_NNI  0x08 /* Port is NNI */

#define EVC_PORT_CHG  (EVC_PORT_OLD | EVC_PORT_NEW) 

/* Update EVC resource usage for port */
static vtss_rc vtss_cmn_res_evc_port_update(vtss_res_t *res, vtss_port_no_t port_no, u8 flags)
{
    vtss_evc_port_info_t *port_info = &vtss_state->evc_port_info[port_no];
    BOOL                 port_nni = VTSS_BOOL(flags & EVC_PORT_NNI);

    switch (flags & EVC_PORT_CHG) {
    case EVC_PORT_OLD: /* Delete port */
        res->port_del[port_no] = 1;
        break;
    case EVC_PORT_NEW: /* Add port */
        if (vtss_cmn_evc_port_check()) {
            if (port_nni && port_info->uni_count) {
                VTSS_E("port_no: %u, UNI->NNI change not allowed", port_no);
                return VTSS_RC_ERROR;
            }
            if (!port_nni && port_info->nni_count) {
                VTSS_E("port_no: %u, NNI->UNI change not allowed", port_no);
                return VTSS_RC_ERROR;
            }
        }
        res->port_add[port_no] = 1;
        break;
    case EVC_PORT_CHG:
        res->port_chg[port_no] = 1;
        break;
    default:
        break;
    } 
    res->port_nni[port_no] = port_nni;
    return VTSS_RC_OK;
}

static vtss_rc vtss_cmn_evc_port_update(vtss_port_no_t port_no, u8 flags)
{
    u16  *count;
    BOOL update = 0, nni = (flags & EVC_PORT_NNI);

    if (nni) {
        /* Update NNI counter */
        count = &vtss_state->evc_port_info[port_no].nni_count;
    } else {
        /* Update UNI counter */
        count = &vtss_state->evc_port_info[port_no].uni_count;
    }

    if (flags & EVC_PORT_NEW) {
        /* Adding port */
        if (*count == 0)
            update = 1;
        (*count)++;
    } else {
        /* Deleting port */
        if (*count == 0) {
            /* This should never happen */
            VTSS_E("%s port_no: %u count already zero", nni ? "NNI" : "UNI", port_no);
            return VTSS_RC_ERROR;
        }
        (*count)--;
        if (*count == 0)
            update = 1;
    }

    if (update) {
        /* Update VLAN port configuration if UNI/NNI mode changed */
        VTSS_RC(vtss_cmn_vlan_port_conf_set(port_no));

        /* Update EVC port configuration if NNI mode changed */
        if (nni)
            return VTSS_FUNC(evc_port_conf_set, port_no);
    }
    return VTSS_RC_OK;
}

static vtss_rc vtss_cmn_evc_vlan_update(vtss_vid_t vid, BOOL learning)
{
    if (vid == 0)
        return VTSS_RC_OK;
    
    vtss_state->vlan_table[vid].learn_disable = (learning ? 0 : 1);
    return vtss_cmn_vlan_members_set(vid);
}

/* Add EVC */
vtss_rc vtss_cmn_evc_add(const vtss_evc_id_t evc_id, const vtss_evc_conf_t *const conf)
{
    vtss_evc_entry_t              *evc = &vtss_state->evc_info.table[evc_id];
    const vtss_evc_pb_conf_t      *pb = &conf->network.pb;
#if defined(VTSS_FEATURE_MPLS)
    const vtss_evc_mpls_tp_conf_t *mpls_tp = &conf->network.mpls_tp;
#endif
    vtss_res_t                    res;
    vtss_port_no_t                port_no;
    u8                            flags;
    BOOL                          add;
    vtss_vid_t                    old_ivid;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "evc_id: %u", evc_id);
    
    /* Check port roles */
    vtss_cmn_res_init(&res);
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        flags = EVC_PORT_NNI;
        if (pb->nni[port_no])
            flags |= EVC_PORT_NEW;
        if (VTSS_PORT_BF_GET(evc->ports, port_no))
            flags |= EVC_PORT_OLD;
        VTSS_RC(vtss_cmn_res_evc_port_update(&res, port_no, flags));
    }
    
    /* Calculate resource usage */
    VTSS_FUNC_RC(evc_update, evc_id, &res, VTSS_RES_CMD_CALC);

    /* Check resource availability */
    VTSS_RC(vtss_cmn_res_check(&res));
    
    /* Save EVC configuration */
    vtss_state->evc_info.count++;
    evc->enable = 1;
    evc->learning = conf->learning;
    evc->vid = pb->vid;
    old_ivid = evc->ivid;
    evc->ivid = pb->ivid;
#if defined(VTSS_ARCH_CARACAL)
    evc->uvid = pb->uvid;
    evc->inner_tag = pb->inner_tag;
#endif /* VTSS_ARCH_CARACAL */
#if defined(VTSS_FEATURE_MPLS)
    evc->pw_ingress_xc = mpls_tp->pw_ingress_xc;
    evc->pw_egress_xc  = mpls_tp->pw_egress_xc;
#endif

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (res.port_add[port_no]) {
            add = 1;
            flags = EVC_PORT_NEW;
        } else if (res.port_del[port_no]) {
            add = 0;
            flags = EVC_PORT_OLD;
        } else
            continue;
        VTSS_PORT_BF_SET(evc->ports, port_no, add);
        VTSS_RC(vtss_cmn_evc_port_update(port_no, flags | EVC_PORT_NNI));
    }

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    evc->policer_id = conf->policer_id;
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */

    /* Update learning for old and new IVID */
    VTSS_RC(vtss_cmn_evc_vlan_update(old_ivid, 1));
    VTSS_RC(vtss_cmn_evc_vlan_update(evc->ivid, evc->learning));
                
    /* Delete resources */
    VTSS_FUNC_RC(evc_update, evc_id, &res, VTSS_RES_CMD_DEL);
    
    /* Add/update resources */
    VTSS_FUNC_RC(evc_update, evc_id, &res, VTSS_RES_CMD_ADD);
    
    return VTSS_RC_OK;
}

/* Determine if ES0 rules are needed for UNIs */
BOOL vtss_cmn_ece_es0_needed(vtss_ece_entry_t *ece)
{
    /* ES0 entry needed for DIR_NNI if outer tag enabled.
       ES0 entry needed for DIR_BOTH if popping enabled. */
    u32 flags = ece->act_flags;

    return ((flags & VTSS_ECE_ACT_OT_ENA) ||
            ((flags & VTSS_ECE_ACT_DIR_ONE) == 0 && (flags & VTSS_ECE_ACT_POP_1)));
}

vtss_ece_dir_t vtss_cmn_ece_dir_get(vtss_ece_entry_t *ece)
{
    u32 flags = ece->act_flags;

    return ((flags & VTSS_ECE_ACT_DIR_UNI_TO_NNI) ? VTSS_ECE_DIR_UNI_TO_NNI :
            (flags & VTSS_ECE_ACT_DIR_NNI_TO_UNI) ? VTSS_ECE_DIR_NNI_TO_UNI :
            VTSS_ECE_DIR_BOTH);
}

#if defined(VTSS_ARCH_SERVAL)
vtss_ece_rule_t vtss_cmn_ece_rule_get(vtss_ece_entry_t *ece)
{
    u32 flags = ece->act_flags;

    return ((flags & VTSS_ECE_ACT_RULE_RX) ? VTSS_ECE_RULE_RX :
            (flags & VTSS_ECE_ACT_RULE_TX) ? VTSS_ECE_RULE_TX : VTSS_ECE_RULE_BOTH);
}
#endif /* VTSS_ARCH_SERVAL */

/* Delete EVC */
vtss_rc vtss_cmn_evc_del(const vtss_evc_id_t evc_id)
{
    vtss_evc_entry_t *evc = &vtss_state->evc_info.table[evc_id], evc_zero;
    vtss_ece_entry_t *ece;
    vtss_res_t       res;
    vtss_port_no_t   port_no;
    BOOL             pop_done[VTSS_PORTS];

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "evc_id: %u", evc_id);

    /* Delete resources */
    memset(pop_done, 0, sizeof(pop_done));
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        if (ece->evc_id != evc_id)
            continue;
        /* Find deleted UNI/NNI ports */
        vtss_cmn_res_init(&res);
        res.ece_del = 1;
        res.dir_old = vtss_cmn_ece_dir_get(ece);
#if defined(VTSS_ARCH_SERVAL)
        res.rule_old = vtss_cmn_ece_rule_get(ece);
#endif /* VTSS_ARCH_SERVAL */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (VTSS_PORT_BF_GET(evc->ports, port_no)) {
                res.port_nni[port_no] = 1;
                res.port_del[port_no] = 1;
            }
            if (VTSS_PORT_BF_GET(ece->ports, port_no)) {
                res.port_del[port_no] = 1;
                if (pop_done[port_no] == 0 && vtss_cmn_ece_es0_needed(ece)) {
                    pop_done[port_no] = 1;
                    res.es0_del[port_no] = 1;
                }
            }
        }
        VTSS_FUNC_RC(ece_update, ece, &res, VTSS_RES_CMD_DEL);
        ece->evc_id = VTSS_EVC_ID_NONE;
    }

    /* Update NNI information */
    vtss_cmn_res_init(&res);
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (VTSS_PORT_BF_GET(evc->ports, port_no)) {
            VTSS_RC(vtss_cmn_evc_port_update(port_no, EVC_PORT_OLD | EVC_PORT_NNI));
            res.port_del[port_no] = 1;
        }
    }
    
    /* Delete EVC */
    VTSS_FUNC_RC(evc_update, evc_id, &res, VTSS_RES_CMD_DEL);
    
    /* Update learning for IVID */
    VTSS_RC(vtss_cmn_evc_vlan_update(evc->ivid, 1));

    /* Save EVC configuration */
    vtss_state->evc_info.count--;
    memset(&evc_zero, 0, sizeof(evc_zero));
#if defined(VTSS_ARCH_SERVAL)
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        /* Preserve VOE mappings */
        evc_zero.voe_idx[port_no] = evc->voe_idx[port_no];
    }
#endif /* VTSS_ARCH_SERVAL */
    *evc = evc_zero;
    return VTSS_RC_OK;
}

static void vtss_cmn_ece_flags_set(vtss_ece_entry_t *ece,
                                   vtss_vcap_bit_t fld, u32 mask_vld, u32 mask_1)
{
    if (fld == VTSS_VCAP_BIT_0)
        ece->key_flags |= mask_vld;
    else if (fld == VTSS_VCAP_BIT_1)
        ece->key_flags |= (mask_vld | mask_1);
}

vtss_vcap_bit_t vtss_cmn_ece_bit_get(vtss_ece_entry_t *ece, u32 mask_vld, u32 mask_1)
{
    return ((ece->key_flags & mask_vld) ?
            ((ece->key_flags & mask_1) ? VTSS_VCAP_BIT_1 : VTSS_VCAP_BIT_0) : VTSS_VCAP_BIT_ANY);
}

static vtss_rc vtss_cmn_ece_range_check(vtss_vcap_vr_t *vr, vtss_vcap_range_chk_type_t type, 
                                        const char *name, BOOL no_range)
{
    vtss_vcap_range_chk_table_t range_new;
    u32                         range;

    if (vtss_state->arch == VTSS_ARCH_JR1 || no_range) {
        /* Convert range to value/mask pair */
        memset(&range_new, 0, sizeof(range_new));
        VTSS_RC(vtss_vcap_vr_alloc(&range_new, &range, type, vr));
        if (range != VTSS_VCAP_RANGE_CHK_NONE) {
            VTSS_E("illegal %s range", name);
            return VTSS_RC_ERROR;
        }
    }    
    return VTSS_RC_OK;
}

/* Save ECE in API memory */
static vtss_rc vtss_cmn_ece_save(const vtss_ece_t *const ece, vtss_ece_entry_t *new, 
                                 BOOL port_update)
{
    const vtss_ece_key_t       *key = &ece->key;
    const vtss_ece_tag_t       *tag = &key->tag;
    const vtss_ece_action_t    *action = &ece->action;
    const vtss_ece_outer_tag_t *ot = &action->outer_tag;
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    const vtss_ece_inner_tag_t *it = &action->inner_tag;
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
    vtss_port_no_t             port_no;
    BOOL                       uni;

    new->ece_id = ece->id;
    new->key_flags = 0;
    new->act_flags = 0;
 
    /* UNI ports */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        uni = (key->port_list[port_no] == VTSS_ECE_PORT_NONE ? 0 : 1);
        if (VTSS_PORT_BF_GET(new->ports, port_no)) {
            if (!uni && port_update) {
                VTSS_RC(vtss_cmn_evc_port_update(port_no, EVC_PORT_OLD | EVC_PORT_UNI));
            }
        } else if (uni && port_update) {
            VTSS_RC(vtss_cmn_evc_port_update(port_no, EVC_PORT_NEW | EVC_PORT_UNI));
        }
        VTSS_PORT_BF_SET(new->ports, port_no, uni);
    }

    /* MAC header matching */
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
    vtss_cmn_ece_flags_set(new, key->mac.dmac_mc,
                           VTSS_ECE_KEY_DMAC_MC_VLD, VTSS_ECE_KEY_DMAC_MC_1);
    
    vtss_cmn_ece_flags_set(new, key->mac.dmac_bc,
                           VTSS_ECE_KEY_DMAC_BC_VLD, VTSS_ECE_KEY_DMAC_BC_1);
    new->smac = key->mac.smac;
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
    new->dmac = key->mac.dmac;
#endif /* VTSS_ARCH_SERVAL */

    /* Tag matching */
    new->vid = tag->vid;
    VTSS_RC(vtss_cmn_ece_range_check(&new->vid, VTSS_VCAP_RANGE_TYPE_VID, "outer VID", 0));
    new->pcp = tag->pcp;
    vtss_cmn_ece_flags_set(new, tag->dei, VTSS_ECE_KEY_TAG_DEI_VLD, VTSS_ECE_KEY_TAG_DEI_1);
    vtss_cmn_ece_flags_set(new, tag->tagged,
                           VTSS_ECE_KEY_TAG_TAGGED_VLD, VTSS_ECE_KEY_TAG_TAGGED_1);
    vtss_cmn_ece_flags_set(new, tag->s_tagged,
                           VTSS_ECE_KEY_TAG_S_TAGGED_VLD, VTSS_ECE_KEY_TAG_S_TAGGED_1);

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    /* Inner tag matching */
    tag = &key->inner_tag;
    new->in_vid = tag->vid;
    VTSS_RC(vtss_cmn_ece_range_check(&new->in_vid, VTSS_VCAP_RANGE_TYPE_VID, "inner VID", 1));
    new->in_pcp = tag->pcp;
    vtss_cmn_ece_flags_set(new, tag->dei,
                           VTSS_ECE_KEY_IN_TAG_DEI_VLD, VTSS_ECE_KEY_IN_TAG_DEI_1);
    vtss_cmn_ece_flags_set(new, tag->tagged,
                           VTSS_ECE_KEY_IN_TAG_TAGGED_VLD, VTSS_ECE_KEY_IN_TAG_TAGGED_1);
    vtss_cmn_ece_flags_set(new, tag->s_tagged,
                           VTSS_ECE_KEY_IN_TAG_S_TAGGED_VLD, VTSS_ECE_KEY_IN_TAG_S_TAGGED_1);
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */

    /* IP header matching */
    if (key->type == VTSS_ECE_TYPE_IPV4) {
        new->key_flags |= VTSS_ECE_KEY_PROT_IPV4;
        new->frame.ipv4 = key->frame.ipv4;
        VTSS_RC(vtss_cmn_ece_range_check(&new->frame.ipv4.dscp, VTSS_VCAP_RANGE_TYPE_DSCP,
                                         "IPv4 DSCP", 0));
    } else if (key->type == VTSS_ECE_TYPE_IPV6) {
        new->key_flags |= VTSS_ECE_KEY_PROT_IPV6;
        new->frame.ipv6 = key->frame.ipv6;
        VTSS_RC(vtss_cmn_ece_range_check(&new->frame.ipv6.dscp, VTSS_VCAP_RANGE_TYPE_DSCP,
                                         "IPv6 DSCP", 0));
    }

    /* Direction and outer tag */
    if (action->dir == VTSS_ECE_DIR_UNI_TO_NNI)
        new->act_flags |= VTSS_ECE_ACT_DIR_UNI_TO_NNI;
    if (action->dir == VTSS_ECE_DIR_NNI_TO_UNI) {
        new->act_flags |= VTSS_ECE_ACT_DIR_NNI_TO_UNI;
        if (ot->enable)
            new->act_flags |= VTSS_ECE_ACT_OT_ENA;
    }
    new->ot_pcp = ot->pcp;
#if defined(VTSS_ARCH_SERVAL)
    if (action->rule == VTSS_ECE_RULE_RX)
        new->act_flags |= VTSS_ECE_ACT_RULE_RX;
    else if (action->rule == VTSS_ECE_RULE_TX)
        new->act_flags |= VTSS_ECE_ACT_RULE_TX;
    if (action->tx_lookup == VTSS_ECE_TX_LOOKUP_VID_PCP)
        new->act_flags |= VTSS_ECE_ACT_TX_LOOKUP_VID_PCP;
    else if (action->tx_lookup == VTSS_ECE_TX_LOOKUP_ISDX)
        new->act_flags |= VTSS_ECE_ACT_TX_LOOKUP_ISDX;
    if (ot->pcp_mode == VTSS_ECE_PCP_MODE_FIXED)
        new->act_flags |= VTSS_ECE_ACT_OT_PCP_MODE_FIXED;
    else if (ot->pcp_mode == VTSS_ECE_PCP_MODE_MAPPED)
        new->act_flags |= VTSS_ECE_ACT_OT_PCP_MODE_MAPPED;
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_CARACAL)
    if (!ot->pcp_dei_preserve)
        new->act_flags |= VTSS_ECE_ACT_OT_PCP_MODE_FIXED;
#endif /* VTSS_ARCH_JAGUAR_1/CARACAL */
    if (ot->dei)
        new->act_flags |= VTSS_ECE_ACT_OT_DEI;
#if defined(VTSS_ARCH_SERVAL)
    if (ot->dei_mode == VTSS_ECE_DEI_MODE_FIXED)
        new->act_flags |= VTSS_ECE_ACT_OT_DEI_MODE_FIXED;
    else if (ot->dei_mode == VTSS_ECE_DEI_MODE_DP)
        new->act_flags |= VTSS_ECE_ACT_OT_DEI_MODE_DP;
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    new->ot_vid = ot->vid;
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */

    /* Tag popping */
    if (action->pop_tag == VTSS_ECE_POP_TAG_1)
        new->act_flags |= VTSS_ECE_ACT_POP_1;
    else if (action->pop_tag == VTSS_ECE_POP_TAG_2)
        new->act_flags |= VTSS_ECE_ACT_POP_2;

    /* Inner tag */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
    switch (it->type) {
    case VTSS_ECE_INNER_TAG_C:
        new->act_flags |= VTSS_ECE_ACT_IT_TYPE_C;
        break;
    case VTSS_ECE_INNER_TAG_S:
        new->act_flags |= VTSS_ECE_ACT_IT_TYPE_S;
        break;
    case VTSS_ECE_INNER_TAG_S_CUSTOM:
        new->act_flags |= VTSS_ECE_ACT_IT_TYPE_S_CUSTOM;
        break;
    case VTSS_ECE_INNER_TAG_NONE:
    default:
        break;
    }
#if defined(VTSS_ARCH_SERVAL)
    if (it->pcp_mode == VTSS_ECE_PCP_MODE_FIXED)
        new->act_flags |= VTSS_ECE_ACT_IT_PCP_MODE_FIXED;
    else if (it->pcp_mode == VTSS_ECE_PCP_MODE_MAPPED)
        new->act_flags |= VTSS_ECE_ACT_IT_PCP_MODE_MAPPED;
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_CARACAL)
    if (!it->pcp_dei_preserve)
        new->act_flags |= VTSS_ECE_ACT_IT_PCP_MODE_FIXED;
#endif /* VTSS_ARCH_JAGUAR_1/CARACAL */
    if (it->dei)
        new->act_flags |= VTSS_ECE_ACT_IT_DEI;
#if defined(VTSS_ARCH_SERVAL)
    if (it->dei_mode == VTSS_ECE_DEI_MODE_FIXED)
        new->act_flags |= VTSS_ECE_ACT_IT_DEI_MODE_FIXED;
    else if (it->dei_mode == VTSS_ECE_DEI_MODE_DP)
        new->act_flags |= VTSS_ECE_ACT_IT_DEI_MODE_DP;
#endif /* VTSS_ARCH_SERVAL */
    new->it_vid = it->vid;
    new->it_pcp = it->pcp;

    /* Policer, EVC, policy and priority */
    new->policer_id = action->policer_id;
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
    new->evc_id = action->evc_id;
    if (action->policy_no == VTSS_ACL_POLICY_NO_NONE)
        new->act_flags |= VTSS_ECE_ACT_POLICY_NONE;
    new->policy_no = action->policy_no;
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
    if (action->prio_enable)
        new->act_flags |= VTSS_ECE_ACT_PRIO_ENA;
    new->prio = action->prio;
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
    if (action->dp_enable)
        new->act_flags |= VTSS_ECE_ACT_DP_ENA;
    new->dp = action->dp;
#endif /* VTSS_ARCH_CARACAL/SERVAL */
    
    return VTSS_RC_OK;
}

/* Get EVC, if valid */
static vtss_evc_entry_t *vtss_cmn_evc_get(vtss_evc_id_t evc_id) 
{
    vtss_evc_entry_t *evc = NULL;
    
    if (evc_id != VTSS_EVC_ID_NONE) {
        evc = &vtss_state->evc_info.table[evc_id];
        if (!evc->enable)
            evc = NULL;
    }
    return evc;
}

typedef struct {
    u16 old_cnt[VTSS_PORTS];
    u16 new_cnt[VTSS_PORTS];
} vtss_evc_es0_info_t;

static void vtss_cmn_es0_info_get(vtss_evc_es0_info_t *info, 
                                  vtss_evc_id_t new_evc_id, vtss_evc_id_t old_evc_id)
{
    vtss_ece_entry_t *ece;
    vtss_port_no_t   port_no;
    vtss_evc_id_t    evc_id;

    memset(info, 0, sizeof(*info));
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        if ((evc_id = ece->evc_id) != VTSS_EVC_ID_NONE &&
            (evc_id == old_evc_id || evc_id == new_evc_id) && 
            vtss_cmn_ece_es0_needed(ece)) {
            for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
                if (VTSS_PORT_BF_GET(ece->ports, port_no)) {
                    if (evc_id == old_evc_id)
                        info->old_cnt[port_no]++;
                    if (evc_id == new_evc_id)
                        info->new_cnt[port_no]++;
                }
            }
        }
    }
}

/* Add ECE */
vtss_rc vtss_cmn_ece_add(const vtss_ece_id_t ece_id, const vtss_ece_t *const ece)
{
    vtss_ece_info_t     *ece_info = &vtss_state->ece_info;
    vtss_evc_id_t       evc_id = ece->action.evc_id, old_evc_id;
    vtss_evc_entry_t    *new_evc, *old_evc;
    vtss_ece_entry_t    *cur, *prev = NULL, new;
    vtss_ece_entry_t    *old = NULL, *old_prev = NULL, *ins = NULL, *ins_prev = NULL;
    vtss_res_t          res;
    vtss_port_no_t      port_no;
    u8                  flags;
    vtss_evc_es0_info_t es0_info;
    
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "ece_id: %u", ece_id);

    /* Check ECE ID */
    if (ece->id == VTSS_ECE_ID_LAST || ece->id == ece_id) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal ece id: %u", ece->id);
        return VTSS_RC_ERROR;
    }
    
    /* Search for existing entry and place to add */
    for (cur = ece_info->used; cur != NULL; prev = cur, cur = cur->next) {
        if (cur->ece_id == ece->id) {
            /* Entry already exists */
            old_prev = prev;
            old = cur;
        }
        
        if (cur->ece_id == ece_id) {
            /* Found insertion point */
            ins_prev = prev;
            ins = cur;
        }
    }
    if (ece_id == VTSS_ECE_ID_LAST)
        ins_prev = prev;

    /* Check if the place to insert was found */
    if (ins == NULL && ece_id != VTSS_ECE_ID_LAST) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "could not find ece ID: %u", ece_id);
        return VTSS_RC_ERROR;
    }
    
    /* Find and check UNI/NNI port changes */
    vtss_cmn_res_init(&res);

    /* New EVC, if valid */
    if ((new_evc = vtss_cmn_evc_get(evc_id)) != NULL)
        res.ece_add = 1;
    
    /* Old EVC, if valid */
    old_evc_id = (old == NULL ? VTSS_EVC_ID_NONE : old->evc_id);
    if ((old_evc = vtss_cmn_evc_get(old_evc_id)) != NULL) 
        res.ece_del = 1;
    
    /* Old and new direction */
    if (old != NULL) 
        res.dir_old = vtss_cmn_ece_dir_get(old);
    res.dir_new = ece->action.dir;

#if defined(VTSS_ARCH_SERVAL)
    /* Old and new rule type */
    if (old != NULL)
        res.rule_old = vtss_cmn_ece_rule_get(old);
    res.rule_new = ece->action.rule;
#endif /* VTSS_ARCH_SERVAL */
    
    /* Get POP information */
    vtss_cmn_es0_info_get(&es0_info, evc_id, old_evc_id);

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        flags = EVC_PORT_NONE;
        if (new_evc != NULL) {
            /* New ECE is using a valid EVC */
            if (ece->key.port_list[port_no] != VTSS_ECE_PORT_NONE) {
                flags |= (EVC_PORT_NEW | EVC_PORT_UNI);
                if (((ece->action.dir == VTSS_ECE_DIR_BOTH && 
                     ece->action.pop_tag == VTSS_ECE_POP_TAG_1) ||
                     (ece->action.dir == VTSS_ECE_DIR_NNI_TO_UNI && 
                      ece->action.outer_tag.enable))
                    && es0_info.new_cnt[port_no] == 0)
                    res.es0_add[port_no] = 1;
            }
            if (VTSS_PORT_BF_GET(new_evc->ports, port_no))
                flags |= (EVC_PORT_NEW | EVC_PORT_NNI);
        }
        if (old != NULL && old_evc != NULL) {
            /* Old ECE was using a valid EVC */
            if (VTSS_PORT_BF_GET(old->ports, port_no)) {
                flags |= (EVC_PORT_OLD | EVC_PORT_UNI);
                if (vtss_cmn_ece_es0_needed(old) && es0_info.old_cnt[port_no] == 1)
                    res.es0_del[port_no] = 1;
            }
            if (VTSS_PORT_BF_GET(old_evc->ports, port_no))
                flags |= (EVC_PORT_OLD | EVC_PORT_NNI);
        }
        VTSS_RC(vtss_cmn_res_evc_port_update(&res, port_no, flags));
    }

    /* Calculate resource usage changes */
    VTSS_RC(vtss_cmn_ece_save(ece, &new, 0));
    VTSS_FUNC_RC(ece_update, &new, &res, VTSS_RES_CMD_CALC);
    
    /* Check resource availability */
    VTSS_RC(vtss_cmn_res_check(&res));

    if (old == NULL) {
        /* Take entry from free list */
        if ((old = ece_info->free) == NULL) {
            VTSS_E("no more ECEs");
            return VTSS_RC_ERROR;
        }
        ece_info->free = old->next;
        ece_info->count++;
    } else {
        /* Take existing entry out of list */
        if (ins_prev == old)
            ins_prev = old_prev;
        if (old_prev == NULL)
            ece_info->used = old->next;
        else
            old_prev->next = old->next;
    }

    /* Insert new entry in list */
    if (ins_prev == NULL) {
        old->next = ece_info->used;
        ece_info->used = old;
    } else {
        old->next = ins_prev->next;
        ins_prev->next = old;
    }

    /* Save ECE configuration */
    VTSS_RC(vtss_cmn_ece_save(ece, old, 1));
    
    /* Delete resources */
    VTSS_FUNC_RC(ece_update, old, &res, VTSS_RES_CMD_DEL);

    /* Add/update resources */
    VTSS_FUNC_RC(ece_update, old, &res, VTSS_RES_CMD_ADD);

    return VTSS_RC_OK;
}

/* Delete ECE */
vtss_rc vtss_cmn_ece_del(const vtss_ece_id_t ece_id)
{
    vtss_ece_info_t      *ece_info = &vtss_state->ece_info;
    vtss_evc_entry_t     *evc;
    vtss_ece_entry_t     *ece, *prev = NULL;
    vtss_res_t           res;
    vtss_port_no_t       port_no;
    vtss_evc_es0_info_t  es0_info;
    
    VTSS_DG(VTSS_TRACE_GROUP_EVC, "ece_id: %u", ece_id);

    /* Find ECE */
    for (ece = ece_info->used; ece != NULL; prev = ece, ece = ece->next) {
        if (ece->ece_id == ece_id) {
            break;
        }
    }

    /* Check if ECE was found */
    if (ece == NULL) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "could not find ece ID: %u", ece_id);
        return VTSS_RC_ERROR;
    }

    /* Delete resources */
    if ((evc = vtss_cmn_evc_get(ece->evc_id)) != NULL) {
        /* Get POP information */
        vtss_cmn_es0_info_get(&es0_info, ece->evc_id, VTSS_EVC_ID_NONE);

        vtss_cmn_res_init(&res);
        res.ece_del = 1;
        res.dir_old = vtss_cmn_ece_dir_get(ece);
#if defined(VTSS_ARCH_SERVAL)
        res.rule_old = vtss_cmn_ece_rule_get(ece);
#endif /* VTSS_ARCH_SERVAL */
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (VTSS_PORT_BF_GET(evc->ports, port_no)) {
                res.port_del[port_no] = 1;
                res.port_nni[port_no] = 1;
            }
            if (VTSS_PORT_BF_GET(ece->ports, port_no)) {
                res.port_del[port_no] = 1;
                if (vtss_cmn_ece_es0_needed(ece) && es0_info.new_cnt[port_no] == 1)
                    res.es0_del[port_no] = 1;
            }
        }
        VTSS_FUNC_RC(ece_update, ece, &res, VTSS_RES_CMD_DEL);
    }
    
    /* Update UNI information */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (VTSS_PORT_BF_GET(ece->ports, port_no)) {
            VTSS_RC(vtss_cmn_evc_port_update(port_no, EVC_PORT_OLD | EVC_PORT_UNI));
        }
    }
    
    /* Save ECE configuration and move entry from used list to free list */
    ece_info->count--;
    if (prev == NULL)
        ece_info->used = ece->next;
    else
        prev->next = ece->next;
    memset(ece, 0, sizeof(*ece));
    ece->next = ece_info->free;
    ece_info->free = ece;

    return VTSS_RC_OK;
}

#if defined(VTSS_SDX_CNT)
static vtss_rc vtss_cmn_ece_counters_update(const vtss_ece_id_t ece_id, 
                                            const vtss_evc_id_t evc_id,
                                            vtss_port_no_t port_no,
                                            vtss_evc_counters_t *const counters, 
                                            BOOL clear)
{
    vtss_ece_entry_t *ece;
    vtss_sdx_entry_t *sdx;
    
    /* Search for matching ECE ID or EVC ID */
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        if (ece->ece_id == ece_id || (evc_id != VTSS_EVC_ID_NONE && ece->evc_id == evc_id)) {
            /* Found ECE matching ECE ID or EVC ID */
            for (sdx = ece->isdx_list; sdx != NULL; sdx = sdx->next) {
                if (sdx->port_no == port_no) {
                    VTSS_FUNC_RC(sdx_counters_update, sdx, NULL, counters, clear);
                    break;
                }
            }
            for (sdx = ece->esdx_list; sdx != NULL; sdx = sdx->next) {
                if (sdx->port_no == port_no) {
                    VTSS_FUNC_RC(sdx_counters_update, NULL, sdx, counters, clear);
                    break;
                }
            }
            if (evc_id == VTSS_EVC_ID_NONE)
                break; /* Found ECE ID */
        }
    }
    if (evc_id == VTSS_EVC_ID_NONE && ece == NULL) {
        VTSS_E("ece_id: %u not found", ece_id);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

vtss_rc vtss_cmn_ece_counters_get(const vtss_ece_id_t  ece_id,
                                  const vtss_port_no_t port_no,
                                  vtss_evc_counters_t  *const counters)
{
    memset(counters, 0, sizeof(*counters));
    return vtss_cmn_ece_counters_update(ece_id, VTSS_EVC_ID_NONE, port_no, counters, 0);
}

vtss_rc vtss_cmn_ece_counters_clear(const vtss_ece_id_t  ece_id,
                                    const vtss_port_no_t port_no)
{
    return vtss_cmn_ece_counters_update(ece_id, VTSS_EVC_ID_NONE, port_no, NULL, 1);
}

vtss_rc vtss_cmn_evc_counters_get(const vtss_evc_id_t  evc_id,
                                  const vtss_port_no_t port_no,
                                  vtss_evc_counters_t  *const counters)
{
    memset(counters, 0, sizeof(*counters));
    return vtss_cmn_ece_counters_update(VTSS_ECE_ID_LAST, evc_id, port_no, counters, 0);
}

vtss_rc vtss_cmn_evc_counters_clear(const vtss_evc_id_t  evc_id,
                                    const vtss_port_no_t port_no)
{
    return vtss_cmn_ece_counters_update(VTSS_ECE_ID_LAST, evc_id, port_no, NULL, 1);
}
#endif /* VTSS_SDX_CNT */
#endif /* VTSS_FEATURE_EVC */

#if defined(VTSS_ARCH_CARACAL)
/* Add MCE */
vtss_rc vtss_cmn_mce_add(const vtss_mce_id_t mce_id, const vtss_mce_t *const mce)
{
    vtss_vcap_obj_t             *is1_obj = &vtss_state->is1.obj;
    vtss_vcap_user_t            is1_user = VTSS_IS1_USER_MEP;
    vtss_vcap_data_t            data;
    vtss_is1_data_t             *is1 = &data.u.is1;
    vtss_is1_entry_t            entry;
    vtss_is1_action_t           *action = &entry.action;
    vtss_is1_key_t              *key = &entry.key;
    u32                         old = 0;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "mce_id: %u", mce_id);

    /* Check MCE ID */
    if (mce->id == VTSS_MCE_ID_LAST || mce->id == mce_id) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "illegal mce id: %u", mce->id);
        return VTSS_RC_ERROR;
    }

    /* Initialize entry data */
    vtss_vcap_is1_init(&data, &entry);

    /* First Lookup */
    is1->lookup = 0;

    /* Check if main entry exists */
    if (vtss_vcap_lookup(is1_obj, is1_user, mce->id, &data, NULL) == VTSS_RC_OK) {
        old = 1;
    }

    if (is1_obj->count >= (is1_obj->max_count + old)) {
        VTSS_IG(VTSS_TRACE_GROUP_EVC, "IS1 is full");
        return VTSS_RC_ERROR;
    }

    /* Copy action data */
    action->prio_enable = mce->action.prio_enable;
    action->prio        = mce->action.prio;
    action->pag_enable  = TRUE;
    action->pag         = mce->action.policy_no;
    action->pop_enable  = TRUE;
    action->pop         = mce->action.pop_cnt;
    action->vid         = mce->action.vid;

    /* Copy key data */
    memcpy(key->port_list, mce->key.port_list, sizeof(key->port_list));

    key->tag.vid.type              = VTSS_VCAP_VR_TYPE_VALUE_MASK;
    key->tag.vid.vr.v.value        = mce->key.vid.value;
    key->tag.vid.vr.v.mask         = mce->key.vid.mask;

    key->type                       = VTSS_IS1_TYPE_ETYPE;
    key->frame.etype.etype.value[0] = 0x89; /* OAM PDU */
    key->frame.etype.etype.value[1] = 0x02; /* OAM PDU */
    key->frame.etype.etype.mask[0]  = 0xFF;
    key->frame.etype.etype.mask[1]  = 0xFF;
    key->frame.etype.data.value[0]  = mce->key.data.value[0];
    key->frame.etype.data.value[1]  = mce->key.data.value[1];
    key->frame.etype.data.mask[0]   = mce->key.data.mask[0];
    key->frame.etype.data.mask[1]   = mce->key.data.mask[1];

    /* Add main entry */
    VTSS_RC(vtss_vcap_add(is1_obj, is1_user, mce->id, mce_id, &data, 0));
    return VTSS_RC_OK;
}

/* Delete MCE */
vtss_rc vtss_cmn_mce_del(const vtss_mce_id_t mce_id)
{
    vtss_vcap_obj_t  *obj = &vtss_state->is1.obj;

    VTSS_DG(VTSS_TRACE_GROUP_EVC, "mce_id: %u", mce_id);

    if (vtss_vcap_lookup(obj, VTSS_IS1_USER_MEP, mce_id, NULL, NULL) != VTSS_RC_OK) {
        VTSS_EG(VTSS_TRACE_GROUP_EVC, "mce_id: %u not found", mce_id);
        return VTSS_RC_ERROR;
    }
    
    /* Delete entry */
    VTSS_RC(vtss_vcap_del(obj, VTSS_IS1_USER_MEP, mce_id));
    
    return VTSS_RC_OK;
}
#endif /* VTSS_ARCH_CARACAL */

/* ================================================================= *
 *  Debug print
 * ================================================================= */

static void vtss_debug_print_header_underlined(const vtss_debug_printf_t pr,
                                               const char                *header,
                                               BOOL                      layer)
{
    int i, len = strlen(header);

    pr("%s\n", header);
    for (i = 0; i < len; i++)
        pr(layer ? "=" : "-");
    pr("\n\n");
}

void vtss_debug_print_header(const vtss_debug_printf_t pr,
                             const char                *header)
{
    pr("%s:\n\n", header);
}

static const char *const vtss_debug_group_name[VTSS_DEBUG_GROUP_COUNT] = {
    [VTSS_DEBUG_GROUP_ALL]       = "All",       /**< All groups */
    [VTSS_DEBUG_GROUP_INIT]      = "Initialization",
    [VTSS_DEBUG_GROUP_MISC]      = "Miscellaneous",
    [VTSS_DEBUG_GROUP_PORT]      = "Port",
    [VTSS_DEBUG_GROUP_PORT_CNT]  = "Port Counters",
    [VTSS_DEBUG_GROUP_PHY]       = "PHY",
    [VTSS_DEBUG_GROUP_VLAN]      = "VLAN",
    [VTSS_DEBUG_GROUP_PVLAN]     = "PVLAN",
    [VTSS_DEBUG_GROUP_MAC_TABLE] = "MAC Table",
    [VTSS_DEBUG_GROUP_ACL]       = "ACL",
    [VTSS_DEBUG_GROUP_QOS]       = "QoS",
    [VTSS_DEBUG_GROUP_AGGR]      = "Aggregation",
    [VTSS_DEBUG_GROUP_GLAG]      = "Global aggregation",
    [VTSS_DEBUG_GROUP_STP]       = "Spanning Tree",
    [VTSS_DEBUG_GROUP_MIRROR]    = "Mirroring",
    [VTSS_DEBUG_GROUP_EVC]       = "EVC",
    [VTSS_DEBUG_GROUP_ERPS]      = "ERPS",
    [VTSS_DEBUG_GROUP_EPS]       = "EPS",
    [VTSS_DEBUG_GROUP_PACKET]    = "Packet",
    [VTSS_DEBUG_GROUP_FDMA]      = "FDMA",
    [VTSS_DEBUG_GROUP_TS]        = "Timestamping",
    [VTSS_DEBUG_GROUP_WM]        = "Watermarks",
    [VTSS_DEBUG_GROUP_LRN]       = "LRN:COMMON",
    [VTSS_DEBUG_GROUP_IPMC]      = "IP Multicast",
    [VTSS_DEBUG_GROUP_STACK]     = "Stacking",
    [VTSS_DEBUG_GROUP_CMEF]      = "Congestion Management",
    [VTSS_DEBUG_GROUP_HOST]      = "Host Configuration",
#if defined(VTSS_FEATURE_MPLS)
    [VTSS_DEBUG_GROUP_MPLS]      = "MPLS",
    [VTSS_DEBUG_GROUP_MPLS_OAM]  = "MPLS OAM",
#endif
    [VTSS_DEBUG_GROUP_VXLAT]     = "VLAN Translation",
    [VTSS_DEBUG_GROUP_SER_GPIO]  = "Serial GPIO",
#ifdef VTSS_ARCH_DAYTONA
    [VTSS_DEBUG_GROUP_XFI]       = "HSS Data Interface (XFI)",
    [VTSS_DEBUG_GROUP_UPI]       = "Universal PHY Interface (UPI)",
    [VTSS_DEBUG_GROUP_PCS_10GBASE_R] = "PCS 10GBaseR",
    [VTSS_DEBUG_GROUP_MAC10G]    = "MAC10G",
    [VTSS_DEBUG_GROUP_WIS]       = "WIS",
    [VTSS_DEBUG_GROUP_RAB]       = "Rate Adaptation Block (RAB)",
    [VTSS_DEBUG_GROUP_XAUI]      = "PCS XAUI Interface",
    [VTSS_DEBUG_GROUP_OTN]       = "OTN",
    [VTSS_DEBUG_GROUP_GFP]       = "GFP",
#endif /* VTSS_ARCH_DAYTONA */
#if defined(VTSS_FEATURE_OAM)
    [VTSS_DEBUG_GROUP_OAM]       = "OAM",
#endif
#if defined(VTSS_SW_OPTION_L3RT) || defined(VTSS_ARCH_JAGUAR_1)
    [VTSS_DEBUG_GROUP_L3]        = "L3",
#endif /* VTSS_SW_OPTION_L3RT || VTSS_ARCH_JAGUAR_1*/
#if defined(VTSS_FEATURE_AFI_SWC)
    [VTSS_DEBUG_GROUP_AFI]       = "AFI",
#endif /* VTSS_FEATURE_AFI_SWC */
};

BOOL vtss_debug_group_enabled(const vtss_debug_printf_t pr,
                              const vtss_debug_info_t *const info,
                              const vtss_debug_group_t group)
{
    if (info->group == VTSS_DEBUG_GROUP_ALL || info->group == group) {
        vtss_debug_print_header_underlined(pr, 
                                           group < VTSS_DEBUG_GROUP_COUNT ? 
                                           vtss_debug_group_name[group] : "?", 0);
        return 1;
    }
    return 0;
}

vtss_rc vtss_debug_print_group(const vtss_debug_group_t group,
                               vtss_rc (* dbg)(const vtss_debug_printf_t pr, 
                                               const vtss_debug_info_t   *const info),
                               const vtss_debug_printf_t pr,
                               const vtss_debug_info_t *const info)
{
    if (!vtss_debug_group_enabled(pr, info, group))
        return VTSS_RC_OK;

    return dbg(pr, info);
}

void vtss_debug_print_sticky(const vtss_debug_printf_t pr, 
                             const char *name, u32 value, u32 mask)
{
    pr("%-32s: %u\n", name, VTSS_BOOL(value & mask));
}

void vtss_debug_print_value(const vtss_debug_printf_t pr, const char *name, u32 value)
{
    pr("%-32s: %u\n", name, value);
}

void vtss_debug_print_reg_header(const vtss_debug_printf_t pr, const char *name)
{
    pr("%-32s  31    24.23    16.15     8.7      0 Hex\n", name);
}

void vtss_debug_print_reg(const vtss_debug_printf_t pr, const char *name, u32 value)
{
    u32 i;
    
    pr("%-32s: ", name);
    for (i = 0; i < 32; i++) {
        pr("%s%u", i == 0 || (i % 8) ? "" : ".", value & (1 << (31 - i)) ? 1 : 0);
    }
    pr(" 0x%08x\n", value);
}

static void vtss_debug_print_init(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_INIT))
        return;

    pr("Target    : 0x%04X\n", vtss_state->create.target);
#if defined(VTSS_FEATURE_PORT_MUX)
    pr("Mux mode  : 0x%04x\n", vtss_state->init_conf.mux_mode);
#endif /* VTSS_FEATURE_PORT_MUX */
    pr("State Size: %zu\n\n", sizeof(*vtss_state));
}

#if defined(VTSS_FEATURE_MISC)
static void vtss_debug_print_misc(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    vtss_chip_id_t chip_id;

    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_MISC))
        return;
    
    if (vtss_state->cil_func.chip_id_get != NULL && 
        vtss_state->cil_func.chip_id_get(&chip_id) == VTSS_RC_OK) {
        pr("Chip ID : 0x%04x\n", chip_id.part_number);
        pr("Revision: 0x%04x\n", chip_id.revision);
    }
    pr("\n");
}
#endif /* VTSS_FEATURE_MISC */

#if defined(VTSS_FEATURE_PORT_CONTROL)
vtss_port_no_t vtss_cmn_port2port_no(const vtss_debug_info_t *const info, u32 port)
{
    vtss_port_no_t port_no;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (VTSS_CHIP_PORT(port_no) == port && VTSS_PORT_CHIP_SELECTED(port_no)) {
            if (info->port_list[port_no])
                return port_no;
            break;
        }
    }
    return VTSS_PORT_NO_NONE;
}

static const char *vtss_bool_txt(BOOL enabled)
{
    return (enabled ? "Enabled" : "Disabled");
}

static void vtss_debug_print_port(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    vtss_port_no_t   port_no;
    vtss_port_map_t  *map;
    vtss_port_conf_t *conf;
    const char       *mode;
    BOOL             header = 1;
    
    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_PORT))
        return;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            vtss_debug_print_header(pr, "Mapping");
#if defined (VTSS_ARCH_JAGUAR_1_CE_MAC)
            pr("Api-Port Chip-Port  Lport       MIIM Bus   MIIM Addr\n");
#else
            pr("Port  Chip Port  Chip  MIIM Bus  MIIM Addr  MIIM Chip\n");
#endif

        }
        map = &vtss_state->port_map[port_no];
#if defined (VTSS_ARCH_JAGUAR_1_CE_MAC)
        pr("%-10u%-11d%-12u%-10d%-11u\n",
           port_no, map->chip_port, map->lport_no, 
           map->miim_controller, map->miim_addr);
#else
        pr("%-6u%-11d%-6u%-10d%-11u%u\n",
           port_no, map->chip_port, map->chip_no, 
           map->miim_controller, map->miim_addr, map->miim_chip_no);
#endif
      
    }
    if (!header)
        pr("\n");
    header = 1;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            vtss_debug_print_header(pr, "Configuration");
            pr("Port  Interface  Mode      Obey      Generate  Max Length\n");
        }
        conf = &vtss_state->port_conf[port_no];
        switch (conf->speed) {
        case VTSS_SPEED_10M:
            mode = (conf->fdx ? "10fdx" : "10hdx");
            break;
        case VTSS_SPEED_100M:
            mode = (conf->fdx ? "100fdx" : "100hdx");
            break;
        case VTSS_SPEED_1G:
            mode = (conf->fdx ? "1Gfdx" : "1Ghdx");
            break;
        case VTSS_SPEED_2500M:
            mode = (conf->fdx ? "2.5Gfdx" : "2.5Ghdx");
            break;
        case VTSS_SPEED_5G:
            mode = (conf->fdx ? "5Gfdx" : "5Ghdx");
            break;
        case VTSS_SPEED_10G:
            mode = (conf->fdx ? "10Gfdx" : "10Ghdx");
            break;
        default:
            mode = "?";
            break;
        }
        pr("%-6u%-11s%-10s%-10s%-10s%u+%s\n", 
           port_no, 
           vtss_port_if_txt(conf->if_type), 
           mode, 
           vtss_bool_txt(conf->flow_control.obey),
           vtss_bool_txt(conf->flow_control.generate),
           conf->max_frame_length,
           conf->max_tags == VTSS_PORT_MAX_TAGS_NONE ? "0" :
           conf->max_tags == VTSS_PORT_MAX_TAGS_ONE ? "4" :
           conf->max_tags == VTSS_PORT_MAX_TAGS_TWO ? "8" : "?");
    }
    if (!header)
        pr("\n");
    
#if defined(VTSS_FEATURE_LAYER2)
    header = 1;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_stp_state_t    stp = vtss_state->stp_state[port_no];
        vtss_auth_state_t   auth = vtss_state->auth_state[port_no];
        vtss_port_forward_t fwd = vtss_state->port_forward[port_no];
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            vtss_debug_print_header(pr, "Forwarding");
            pr("Port  State  Forwarding  STP State   Auth State  Rx Fwd    Tx Fwd   \n");
        }
        pr("%-6u%-7s%-12s%-12s%-12s%-10s%s\n",
           port_no,
           vtss_state->port_state[port_no] ? "Up" : "Down",
           fwd == VTSS_PORT_FORWARD_ENABLED ? "Enabled" :
           fwd == VTSS_PORT_FORWARD_DISABLED ? "Disabled" :
           fwd == VTSS_PORT_FORWARD_INGRESS ? "Ingress" :
           fwd == VTSS_PORT_FORWARD_EGRESS ? "Egress" : "?",
           stp == VTSS_STP_STATE_DISCARDING ? "Discarding" :
           stp == VTSS_STP_STATE_LEARNING ? "Learning" : 
           stp == VTSS_STP_STATE_FORWARDING  ? "Forwarding" : "?",
           auth == VTSS_AUTH_STATE_NONE ? "None" :
           auth == VTSS_AUTH_STATE_EGRESS ? "Egress" : 
           auth == VTSS_AUTH_STATE_BOTH ? "Both" : "?",
           vtss_bool_txt(vtss_state->rx_forward[port_no]),
           vtss_bool_txt(vtss_state->tx_forward[port_no]));
    }
    if (!header)
        pr("\n");
#endif /* VTSS_FEATURE_LAYER2 */
}

/* Print counters in two columns */
static void vtss_debug_port_cnt(const vtss_debug_printf_t pr,
                                const char *col1, const char *col2, 
                                vtss_port_counter_t c1, vtss_port_counter_t c2)
{
    char buf[80];

    sprintf(buf, "Rx %s:", col1);
    pr("%-19s%19llu   ", buf, c1);
    if (col2 != NULL) {
        sprintf(buf, "Tx %s:", strlen(col2) ? col2 : col1);
        pr("%-19s%19llu", buf, c2);
    }
    pr("\n");
}

static void vtss_debug_print_port_counters(const vtss_debug_printf_t pr,
                                           const vtss_debug_info_t   *const info)
{
    vtss_port_no_t                     port_no;
    vtss_prio_t                        prio, prio_end;
    vtss_port_counters_t               counters;
    vtss_port_rmon_counters_t          *rmon = &counters.rmon;
    vtss_port_if_group_counters_t      *ifg = &counters.if_group;
    vtss_port_ethernet_like_counters_t *eth = &counters.ethernet_like;
    vtss_port_proprietary_counters_t   *prop = &counters.prop;
    vtss_port_counter_t                rx, tx, rx_sum, tx_sum;
    char                               buf[80];
    const char                         *p = NULL;

    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_PORT_CNT))
        return;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_SELECT_CHIP_PORT_NO(port_no);
        if (!info->port_list[port_no] || 
            VTSS_FUNC(port_counters_get, port_no, &counters) != VTSS_RC_OK)
            continue;
        sprintf(buf, "Port %u Counters", port_no);
        vtss_debug_print_header(pr, buf);

        /* Basic counters */
        vtss_debug_port_cnt(pr, "Packets", "", rmon->rx_etherStatsPkts, 
                            rmon->tx_etherStatsPkts);
        vtss_debug_port_cnt(pr, "Octets", "", rmon->rx_etherStatsOctets, 
                            rmon->tx_etherStatsOctets);
        vtss_debug_port_cnt(pr, "Unicast", "", ifg->ifInUcastPkts, ifg->ifOutUcastPkts);
        vtss_debug_port_cnt(pr, "Multicast", "", rmon->rx_etherStatsMulticastPkts, 
                            rmon->tx_etherStatsMulticastPkts);
        vtss_debug_port_cnt(pr, "Broadcast", "", rmon->rx_etherStatsBroadcastPkts, 
                            rmon->tx_etherStatsBroadcastPkts);
        vtss_debug_port_cnt(pr, "Pause", "", eth->dot3InPauseFrames, eth->dot3OutPauseFrames);
        pr("\n");
        if (!info->full)
            continue;

        /* RMON counters */
        vtss_debug_port_cnt(pr, "64", "", rmon->rx_etherStatsPkts64Octets, 
                            rmon->tx_etherStatsPkts64Octets);
        vtss_debug_port_cnt(pr, "65-127", "", rmon->rx_etherStatsPkts65to127Octets,
                            rmon->tx_etherStatsPkts65to127Octets);
        vtss_debug_port_cnt(pr, "128-255", "", rmon->rx_etherStatsPkts128to255Octets,
                            rmon->tx_etherStatsPkts128to255Octets);
        vtss_debug_port_cnt(pr, "256-511", "", rmon->rx_etherStatsPkts256to511Octets,
                            rmon->tx_etherStatsPkts256to511Octets);
        vtss_debug_port_cnt(pr, "512-1023", "", rmon->rx_etherStatsPkts512to1023Octets,
                            rmon->tx_etherStatsPkts512to1023Octets);
        vtss_debug_port_cnt(pr, "1024-1526", "", rmon->rx_etherStatsPkts1024to1518Octets,
                            rmon->tx_etherStatsPkts1024to1518Octets);
        vtss_debug_port_cnt(pr, "1527-    ", "", rmon->rx_etherStatsPkts1519toMaxOctets,
                            rmon->tx_etherStatsPkts1519toMaxOctets);
        pr("\n");

        /* Priority counters */
        rx_sum = 0;
        tx_sum = 0;
        prio_end = VTSS_PRIO_END;
#if defined(VTSS_ARCH_JAGUAR_1)
        prio_end++;
#endif /* VTSS_ARCH_JAGUAR_1 */
        for (prio = VTSS_PRIO_START; prio < prio_end; prio++) {
            sprintf(buf, "Class %u", prio);
            if (prio == VTSS_PRIO_END) {
                /* Super priority counters based on total packets and sum of priority packets */
                rx = rmon->rx_etherStatsPkts;
                rx = (rx < rx_sum ? 0 : (rx - rx_sum));
                tx = rmon->tx_etherStatsPkts;
                tx = (tx < tx_sum ? 0 : (tx - tx_sum));
            } else {
                rx = prop->rx_prio[prio - VTSS_PRIO_START];
#if defined(VTSS_ARCH_LUTON28) || defined(VTSS_ARCH_LUTON26) || defined(VTSS_ARCH_JAGUAR_1)
                tx = prop->tx_prio[prio - VTSS_PRIO_START];
                p = "";
#else
                tx = 0;
#endif /* VTSS_ARCH_LUTON28 || VTSS_ARCH_LUTON26 || VTSS_ARCH_JAGUAR_1 */
                rx_sum += rx;
                tx_sum += tx;
            }
            vtss_debug_port_cnt(pr, buf, p, rx, tx);
        }
        pr("\n");
        
        /* Drop and error counters */
        vtss_debug_port_cnt(pr, "Drops", "", rmon->rx_etherStatsDropEvents,
                            rmon->tx_etherStatsDropEvents);
        vtss_debug_port_cnt(pr, "CRC/Alignment", "Late/Exc. Coll.",
                            rmon->rx_etherStatsCRCAlignErrors, ifg->ifOutErrors);
        vtss_debug_port_cnt(pr, "Undersize", NULL, rmon->rx_etherStatsUndersizePkts, 0);
        vtss_debug_port_cnt(pr, "Oversize", NULL, rmon->rx_etherStatsOversizePkts, 0);
        vtss_debug_port_cnt(pr, "Fragments", NULL, rmon->rx_etherStatsFragments, 0);
        vtss_debug_port_cnt(pr, "Jabbers", NULL, rmon->rx_etherStatsJabbers, 0);
#if defined(VTSS_FEATURE_PORT_CNT_BRIDGE)
        vtss_debug_port_cnt(pr, "Filtered", NULL, counters.bridge.dot1dTpPortInDiscards, 0);
#endif /* VTSS_FEATURE_PORT_CNT_BRIDGE */

#if defined(VTSS_ARCH_CARACAL)
        {
            /* EVC */
            vtss_port_evc_counters_t *evc = &counters.evc;

            for (prio = VTSS_PRIO_START; prio < VTSS_PRIO_END; prio++) {
                pr("\nClass %u:\n", prio);
                vtss_debug_port_cnt(pr, "Green", "", evc->rx_green[prio - VTSS_PRIO_START],
                                    evc->tx_green[prio - VTSS_PRIO_START]);
                vtss_debug_port_cnt(pr, "Yellow", "", evc->rx_yellow[prio - VTSS_PRIO_START],
                                    evc->tx_yellow[prio - VTSS_PRIO_START]);
                vtss_debug_port_cnt(pr, "Red", NULL, evc->rx_red[prio - VTSS_PRIO_START], 0);
                vtss_debug_port_cnt(pr, "Green Drops", NULL, 
                                    evc->rx_green_discard[prio - VTSS_PRIO_START], 0);
                vtss_debug_port_cnt(pr, "Yellow Drops", NULL, 
                                    evc->rx_yellow_discard[prio - VTSS_PRIO_START], 0);
            }
        }
#endif /* VTSS_ARCH_CARACAL */
        pr("\n");
    }
}
#endif /* VTSS_FEATURE_PORT_CONTROL */

/* Print port header, e.g, "0      7.8     15.16    23.24  28" */
void vtss_debug_print_port_header(const vtss_debug_printf_t pr,
                                  const char *txt, u32 count, BOOL nl)
{
    u32 i, port;

    if (count == 0)
        count = vtss_state->port_count;
    if (txt != NULL)
        pr(txt);
    for (port = 0; port < count; port++) {
        i = (port & 7);
        if (i == 0 || i == 7 || (port == (count - 1) && i > 2))
            pr ("%s%u", port != 0 && i == 0 ? "." : "", port);
        else if (port < 10 || i > 2)
            pr(" ");
    }
    if (nl)
        pr("\n");
}

void vtss_debug_print_ports(const vtss_debug_printf_t pr, u8 *member, BOOL nl)
{
    vtss_port_no_t port_no;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        pr("%s%s",
           port_no == 0 || (port_no & 7) ? "" : ".",
           VTSS_PORT_BF_GET(member, port_no) ? "1" : "0");
    }
    if (nl)
        pr("\n");
}

#if defined(VTSS_FEATURE_LAYER2)
static void vtss_debug_print_port_members(const vtss_debug_printf_t pr,
                                          BOOL port_member[VTSS_PORT_ARRAY_SIZE],
                                          BOOL nl)
{
    vtss_port_no_t port_no;
    u8             member[VTSS_PORT_BF_SIZE];
    
    VTSS_PORT_BF_CLR(member);
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
        VTSS_PORT_BF_SET(member, port_no, port_member[port_no]);
    vtss_debug_print_ports(pr, member, nl);
}

static void vtss_debug_print_vlan(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */  // Due to the VTSS_EXIT_ENTER
    vtss_vid_t            vid;
    vtss_vlan_entry_t     *entry;
    BOOL                  header = 1;
    vtss_port_no_t        port_no;
    vtss_vlan_port_conf_t *conf;
    u8                    erps_discard[VTSS_PORT_BF_SIZE];

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    pr("S-tag Etype: 0x%04x\n\n", vtss_state->vlan_conf.s_etype);
#endif /* VTSS_FEATURE_VLAN_PORT_V1 */
    
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            pr("Port  ");
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
            pr("Aware  S-Tag  ");
#endif /* VTSS_FEATURE_VLAN_PORT_V1 */
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
            pr("Type  ");
#endif /* VTSS_FEATURE_VLAN_PORT_V1 */
            pr("PVID  UVID  Frame  Filter\n");
        }
        conf = &vtss_state->vlan_port_conf[port_no];
        pr("%-4u  ", port_no);
#if defined(VTSS_FEATURE_VLAN_PORT_V1)
        pr("%-5u  %-5u  ", conf->aware, conf->stag);
#endif /* VTSS_FEATURE_VLAN_PORT_V1 */
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        pr("%-4s  ", 
           conf->port_type == VTSS_VLAN_PORT_TYPE_UNAWARE ? "U" :
           conf->port_type == VTSS_VLAN_PORT_TYPE_C ? "C" :
           conf->port_type == VTSS_VLAN_PORT_TYPE_S ? "S" :
           conf->port_type == VTSS_VLAN_PORT_TYPE_S_CUSTOM ? "S_CU" : "?");
#endif /* VTSS_FEATURE_VLAN_PORT_V2 */
        pr("%-4u  %-4u  %-5s  %u\n",
           conf->pvid, conf->untagged_vid,
           conf->frame_type == VTSS_VLAN_FRAME_ALL ? "all" :
           conf->frame_type == VTSS_VLAN_FRAME_TAGGED ? "tag" : "untag",
           conf->ingress_filter);
    }
    if (!header)
        pr("\n");
    header = 1;

    for (vid = VTSS_VID_NULL; vid < VTSS_VIDS; vid++) {
        entry = &vtss_state->vlan_table[vid];
        if (!entry->enabled && !info->full)
            continue;
        if (header)
            vtss_debug_print_port_header(pr, "VID   MSTI  Iso  ", 0, 1);
        header = 0;
        pr("%-4u  %-4u  %-3u  ", vid, entry->msti, entry->isolated);
        vtss_debug_print_ports(pr, entry->member, 0);
        pr(" <- VLAN Members\n");
        pr("%-17s", "");
        VTSS_PORT_BF_CLR(erps_discard);
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            VTSS_PORT_BF_SET(erps_discard, 
                             port_no, 
                             entry->erps_discard_cnt[port_no] ? 1 : 0);
        }
        vtss_debug_print_ports(pr, erps_discard, 0);
        pr(" <- ERPS Discard\n");

        /* Leave critical region briefly */
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");
}

static void vtss_debug_print_pvlan(const vtss_debug_printf_t pr,
                                   const vtss_debug_info_t   *const info)
{
    BOOL            header = 1;
    vtss_port_no_t  port_no;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            pr("Port  Isolation\n");
        }
        pr("%-6u%s\n", port_no, vtss_bool_txt(vtss_state->isolated_port[port_no]));
    }
    if (!header)
        pr("\n");

#if defined(VTSS_FEATURE_PVLAN)
    vtss_debug_print_port_header(pr, "PVLAN  ", 0, 1);
    {
        vtss_pvlan_no_t pvlan_no;

        for (pvlan_no = VTSS_PVLAN_NO_START; pvlan_no < VTSS_PVLAN_NO_END; pvlan_no++) {
            pr("%-7u", pvlan_no);
            vtss_debug_print_port_members(pr, vtss_state->pvlan_table[pvlan_no].member, 1);
        }
    }
    pr("\n");

    header = 1;
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            vtss_debug_print_port_header(pr, "APVLAN  ", 0, 1);
        }
        pr("%-6u  ", port_no);
        vtss_debug_print_port_members(pr, vtss_state->apvlan_table[port_no], 1);
    }
    if (!header)
        pr("\n");
#endif /* VTSS_FEATURE_PVLAN */
}

void vtss_debug_print_mac_entry(const vtss_debug_printf_t pr,
                                const char *name,
                                BOOL *header,
                                vtss_mac_table_entry_t *entry,
                                u32 pgid)
{
    u8 *p = &entry->vid_mac.mac.addr[0];
    
    if (*header) {
        *header = 0;
        vtss_debug_print_header(pr, name);
        pr("VID   MAC                PGID  CPU  Locked  ");
#if defined(VTSS_FEATURE_VSTAX_V2)
        pr("Remote  UPSID  UPSPN");
#endif /* VTSS_FEATURE_VSTAX_V2 */
        pr("\n");
    }
    pr("%-4u  %02x-%02x-%02x-%02x-%02x-%02x  %-4u  %-3u  %-6u  ",
       entry->vid_mac.vid, p[0], p[1], p[2], p[3], p[4], p[5], 
       pgid, entry->copy_to_cpu, entry->locked);
#if defined(VTSS_FEATURE_VSTAX_V2)
    pr("%-6u  %-5u  %-5u", 
       entry->vstax2.remote_entry, entry->vstax2.upsid, entry->vstax2.upspn);
#endif /* VTSS_FEATURE_VSTAX_V2 */
    pr("\n");
}

static void vtss_debug_print_mac_table(const vtss_debug_printf_t pr,
                                       const vtss_debug_info_t   *const info)
{
    /*lint --e{454, 455} */  // Due to the VTSS_EXIT_ENTER
    vtss_mac_entry_t       *entry;
    vtss_vid_mac_t         vid_mac;
    vtss_mac_table_entry_t mac_entry;
    u8                     *p = &vid_mac.mac.addr[0];
    BOOL                   header = 1;
    u32                    pgid;

    vtss_debug_print_value(pr, "Age time", vtss_state->mac_age_time);
    vtss_debug_print_value(pr, "MAC table size", sizeof(vtss_mac_entry_t)*VTSS_MAC_ADDRS);
    vtss_debug_print_value(pr, "MAC table maximum", vtss_state->mac_table_max);
    vtss_debug_print_value(pr, "MAC table count", vtss_state->mac_table_count);
    pr("\n");

    vtss_debug_print_port_header(pr, "Flood Members  ", 0, 1);
    pr("Unicast        ");
    vtss_debug_print_port_members(pr, vtss_state->uc_flood, 1);
    pr("Multicast      ");
    vtss_debug_print_port_members(pr, vtss_state->mc_flood, 1);
    pr("IPv4 MC        ");
    vtss_debug_print_port_members(pr, vtss_state->ipv4_mc_flood, 1);
    pr("IPv6 MC        ");
    vtss_debug_print_port_members(pr, vtss_state->ipv6_mc_flood, 1);
    pr("\n");

    /* MAC address table in state */
    for (entry = vtss_state->mac_list_used; entry != NULL; entry = entry->next) {
        if (header) {
            vtss_debug_print_header(pr, "API Entries");
            vtss_debug_print_port_header(pr, "User  VID   MAC                CPU  ", 0, 1);
            header = 0;
        }
        vtss_mach_macl_set(&vid_mac, entry->mach, entry->macl);
        pr("%-6s%-6u%02x-%02x-%02x-%02x-%02x-%02x  %-5u",
           entry->user == VTSS_MAC_USER_NONE ? "None" :
           entry->user == VTSS_MAC_USER_SSM ? "SSM" :
           entry->user == VTSS_MAC_USER_ASM ? "ASM" : "?",
           vid_mac.vid, p[0], p[1], p[2], p[3], p[4], p[5], entry->cpu_copy);
        vtss_debug_print_ports(pr, entry->member, 1);
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");

    /* MAC address table in chip */
    header = 1;
    memset(&mac_entry, 0, sizeof(mac_entry));
    while (vtss_state->cil_func.mac_table_get_next != NULL &&
           vtss_state->cil_func.mac_table_get_next(&mac_entry, &pgid) == VTSS_RC_OK) {
        vtss_debug_print_mac_entry(pr, "Chip Entries", &header, &mac_entry, pgid);
        VTSS_EXIT_ENTER();
    }
    if (!header)
        pr("\n");
}

static void vtss_debug_print_aggr(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    vtss_port_no_t    port_no;
    u8                member[VTSS_PORT_BF_SIZE];
    vtss_aggr_mode_t  *mode;
    vtss_pgid_entry_t *pgid_entry;
    u32               pgid;
    BOOL              empty;
    vtss_aggr_no_t    aggr_no;

    VTSS_PORT_BF_CLR(member);
    vtss_debug_print_port_header(pr, "LLAG  ", 0, 1);
    for (aggr_no = VTSS_AGGR_NO_START; aggr_no < VTSS_AGGR_NO_END; aggr_no++) {
        pr("%-4u  ", aggr_no);
        VTSS_PORT_BF_CLR(member);
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            VTSS_PORT_BF_SET(member, port_no, vtss_state->port_aggr_no[port_no] == aggr_no);
        vtss_debug_print_ports(pr, member, 1);
    }
    pr("\n");

    vtss_debug_print_header(pr, "Mode");
    mode = &vtss_state->aggr_mode;
    vtss_debug_print_value(pr, "SMAC", mode->smac_enable);
    vtss_debug_print_value(pr, "DMAC", mode->dmac_enable);
    vtss_debug_print_value(pr, "SIP/DIP", mode->sip_dip_enable);
    vtss_debug_print_value(pr, "SPORT/DPORT", mode->sport_dport_enable);
    pr("\n");

    vtss_debug_print_port_header(pr, "PGID  Count  Resv  CPU  Queue  ", 0, 1);
    for (pgid = 0; pgid < vtss_state->pgid_count; pgid++) {
        pgid_entry = &vtss_state->pgid_table[pgid];
        empty = pgid_entry->cpu_copy ? 0 : 1;
        for (port_no = VTSS_PORT_NO_START; empty && port_no < vtss_state->port_count; port_no++) {
            if (pgid_entry->member[port_no]) {
                empty = 0;
            }
        }
        if ((pgid_entry->references == 0 && !info->full) || (pgid > 50 && empty && !info->full)) {
            continue;
        }
        pr("%-4u  %-5u  %-4u  %-3u  %-5u  ", pgid, pgid_entry->references, pgid_entry->resv, pgid_entry->cpu_copy, pgid_entry->cpu_queue);

        vtss_debug_print_port_members(pr, pgid_entry->member, 1);
    }
    pr("\n");
    vtss_debug_print_value(pr, "PGID count", vtss_state->pgid_count);
    vtss_debug_print_value(pr, "PGID drop", vtss_state->pgid_drop);
    vtss_debug_print_value(pr, "PGID flood", vtss_state->pgid_flood);
    pr("\n");

    pr("Port  Destination Group\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        pr("%-6u%u\n", port_no, vtss_state->dgroup_port_conf[port_no].dgroup_no);
    }
    pr("\n");
    
}

#if defined(VTSS_FEATURE_AGGR_GLAG)
static void vtss_debug_print_glag(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    vtss_glag_no_t glag_no;
    vtss_port_no_t port_no;
    u8             member[VTSS_PORT_BF_SIZE];
    BOOL           header = 1;

    vtss_debug_print_port_header(pr, "GLAG  ", 0, 1);
    for (glag_no = VTSS_GLAG_NO_START; glag_no < VTSS_GLAG_NO_END; glag_no++) {
        pr("%-4u  ", glag_no);
        VTSS_PORT_BF_CLR(member);
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++)
            VTSS_PORT_BF_SET(member, port_no, vtss_state->port_glag_no[port_no] == glag_no);
        vtss_debug_print_ports(pr, member, 1);
    }
    pr("\n");

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {        
        glag_no = vtss_state->port_glag_no[port_no];
        if (glag_no == VTSS_GLAG_NO_NONE)
            continue;
        
        if (header) {
            pr("%-8s %-8s\n","Port","GLAG");
            header = 0;
        }
        pr("%-8u %-8u\n", port_no, glag_no);
    }
    if (!header)
        pr("\n");

#if defined(VTSS_FEATURE_VSTAX_V2)
    {
        vtss_glag_conf_t *conf;
        u32              gport;
        
        header = 1;
        for (glag_no = VTSS_GLAG_NO_START; glag_no < VTSS_GLAG_NO_END; glag_no++) {
            for (gport = VTSS_GLAG_PORT_START; gport < VTSS_GLAG_PORT_END; gport++) {
                conf = &vtss_state->glag_conf[glag_no][gport];
                if (conf->entry.upspn == VTSS_PORT_NO_NONE) {
                    break;
                }  
                if (header) {
                    pr("%-8s %-8s %-8s\n","GLAG","UPSID","UPSPN");
                    header = 0;
                }
                pr("%-8u %-8d %-8u\n", glag_no, conf->entry.upsid, conf->entry.upspn);
            }
        }
        if (!header)
            pr("\n");
    }
#endif /* VTSS_FEATURE_VSTAX_V2 */
}
#endif /* VTSS_FEATURE_AGGR_GLAG */

static void vtss_debug_print_stp_state(const vtss_debug_printf_t pr, vtss_stp_state_t *state)
{
    vtss_port_no_t   port_no;
    vtss_stp_state_t s;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        s = state[port_no];
        pr("%s%s",
           port_no == 0 || (port_no & 7) ? "" : ".",
           s == VTSS_STP_STATE_FORWARDING ? "F" : s == VTSS_STP_STATE_LEARNING ? "L" : "D");
    }
    pr("\n");

}

static void vtss_debug_print_stp(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    vtss_msti_t msti;

    vtss_debug_print_port_header(pr, "STP   ", 0, 1);
    pr("      ");
    vtss_debug_print_stp_state(pr, vtss_state->stp_state);
    pr("\n");

    vtss_debug_print_port_header(pr, "MSTI  ", 0, 1);
    for (msti = VTSS_MSTI_START; msti < VTSS_MSTI_END; msti++) {
        pr("%-4u  ", msti);
        vtss_debug_print_stp_state(pr, vtss_state->mstp_table[msti].state);
    }
    pr("\n");
}

static void vtss_debug_print_port_none(const vtss_debug_printf_t pr,
                                       const char *name,
                                       vtss_port_no_t port_no)
{
    pr("%s: ", name);
    if (port_no == VTSS_PORT_NO_NONE)
        pr("None");
    else
        pr("%u", port_no);
    pr("\n");
}

static void vtss_debug_print_mirror(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    vtss_mirror_conf_t *conf = &vtss_state->mirror_conf;

    vtss_debug_print_port_header(pr, "         ", 0, 1);
    pr("Ingress: ");
    vtss_debug_print_port_members(pr, vtss_state->mirror_ingress, 1);
    pr("Egress : ");
    vtss_debug_print_port_members(pr, vtss_state->mirror_egress, 1);
    pr("\n");
    
    vtss_debug_print_port_none(pr, "Mirror Port      ", conf->port_no);
    pr("Mirror Forwarding: %s\n", vtss_bool_txt(conf->fwd_enable));
#if defined(VTSS_ARCH_JAGUAR_1)
    pr("Mirror Tag       : %s\n", 
       conf->tag == VTSS_MIRROR_TAG_NONE ? "None" :
       conf->tag == VTSS_MIRROR_TAG_C ? "C-Tag" :
       conf->tag == VTSS_MIRROR_TAG_S ? "S-Tag" :
       conf->tag == VTSS_MIRROR_TAG_S_CUSTOM ? "S-Custom-Tag" : "?");
    pr("Mirror VID       : %u\n", conf->vid);
    pr("Mirror PCP       : %u\n", conf->pcp);
    pr("Mirror DEI       : %u\n", conf->dei);
#endif /* VTSS_ARCH_JAGUAR_1 */
    pr("\n");
}

#if defined(VTSS_FEATURE_SERIAL_GPIO)
static void vtss_debug_print_ser_gpio(const vtss_debug_printf_t pr,
                                      const vtss_debug_info_t   *const info)
{
    vtss_chip_no_t         chip_no;
    vtss_sgpio_group_t     group;
    char                   buf[64];
    vtss_sgpio_conf_t      *conf;
    vtss_sgpio_port_conf_t *port_conf;
    u32                    port, i;

    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_SER_GPIO))
        return;

    /* Print CIL information for all devices and groups */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        for (group = 0; group < vtss_state->sgpio_group_count; group++) {
            sprintf(buf, "Device %u, Group %u", chip_no, group);
            vtss_debug_print_header(pr, buf);
            
            conf  = &vtss_state->sgpio_conf[chip_no][group]; 
            pr("Bit Count   : %u\n", conf->bit_count);

            // Print out blink mode 
            for (i = 0; i < 2; i++) {
                vtss_sgpio_bmode_t m = conf->bmode[i];
                pr("Blink Mode_%u: %s\n",
                   i, 
                   m == VTSS_SGPIO_BMODE_TOGGLE ? "Burst Toggle (mode 1)" :
                   m == VTSS_SGPIO_BMODE_0_625 ? "0.625 Hz (mode 0)" :
                   m == VTSS_SGPIO_BMODE_1_25 ? "1.25 Hz" :
                   m == VTSS_SGPIO_BMODE_2_5 ? "2.5 Hz" :
                   m == VTSS_SGPIO_BMODE_5 ? "5 Hz" : "?");
            }

            pr("\n");
            pr("Port  Status    ");
            for (i = 0; i < 4; i++)
                pr("Mode_%u     ", i);
            pr("\n");
            for (port = 0; port < VTSS_SGPIO_PORTS; port++) {
                port_conf = &conf->port_conf[port];
                pr("%-4u  %-10s", port, vtss_bool_txt(port_conf->enabled));
                for (i = 0; i < 4; i++) {
                    vtss_sgpio_mode_t m = port_conf->mode[i];
                    pr("%-11s", 
                       m == VTSS_SGPIO_MODE_OFF ? "Off" :
                       m == VTSS_SGPIO_MODE_ON ? "On" :
                       m == VTSS_SGPIO_MODE_0 ? "0" :
                       m == VTSS_SGPIO_MODE_1 ? "1" :
                       m == VTSS_SGPIO_MODE_0_ACTIVITY ? "0_ACT" :
                       m == VTSS_SGPIO_MODE_1_ACTIVITY ? "1_ACT" :
                       m == VTSS_SGPIO_MODE_0_ACTIVITY_INV ? "0_ACT_INV" :
                       m == VTSS_SGPIO_MODE_1_ACTIVITY_INV ? "1_ACT_INV" : "?");
                }
                pr("\n");
            }
            pr("\n");
        }
    }
}
#endif
    
#if defined(VTSS_FEATURE_QOS_POLICER_DLB)
static void vtss_debug_print_dlb(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    u32                     i;
    vtss_evc_policer_conf_t *pol_conf;
    BOOL                    cm = 1, header = 1;
    
    for (i = 0; i < VTSS_EVC_POLICERS; i++) {
#if defined(VTSS_ARCH_LUTON26)
        vtss_policer_alloc_t *pol_alloc = &vtss_state->evc_policer_alloc[i];
#endif /* VTSS_ARCH_LUTON26 */
        pol_conf = &vtss_state->evc_policer_conf[i];
        if (pol_conf->enable == 0
#if defined(VTSS_ARCH_LUTON26)
            && pol_alloc->count == 0
#endif /* VTSS_ARCH_LUTON26 */
            )
            continue;
        if (header) {
            header = 0;
            vtss_debug_print_header(pr, "Policers");
            pr("Policer  CM  CF  CIR         CBS         EIR         EBS         ");
#if defined(VTSS_ARCH_LUTON26)
            pr("Count  L26 Policer");
#endif /* VTSS_ARCH_LUTON26 */
            pr("\n");
        }
#if defined(VTSS_ARCH_JAGUAR_1)
        cm = pol_conf->cm;
#endif /* VTSS_ARCH_JAGUAR_1 */        
        pr("%-9u%-4u%-4u%-12u%-12u%-12u%-12u",
           i, cm, pol_conf->cf, 
           pol_conf->cir, pol_conf->cbs, pol_conf->eir, pol_conf->ebs);
#if defined(VTSS_ARCH_LUTON26)
        pr("%-7u%u", pol_alloc->count, pol_alloc->policer);
#endif /* VTSS_ARCH_LUTON26 */
        pr("\n");
    }
    if (!header)
        pr("\n");
}
#endif /* VTSS_FEATURE_QOS_POLICER_DLB */

#if defined(VTSS_FEATURE_EVC)
static const char *vtss_debug_ece_flags(vtss_ece_entry_t *ece, u32 mask_vld, u32 mask_1)
{
    return ((ece->key_flags & mask_1) ? "1" : (ece->key_flags & mask_vld) ? "0" : "X");
}

#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_CARACAL)
static const char *vtss_debug_ece_pcp_dei(vtss_ece_entry_t *ece, u32 mask_fixed)
{
    return ((ece->act_flags & mask_fixed) ? "Fixed" : "Preserved");
}
#endif /* VTSS_ARCH_JAGUAR_1/CARACAL */

#if defined(VTSS_ARCH_SERVAL)
static const char *vtss_debug_ece_dei_mode(vtss_ece_entry_t *ece, u32 mask_fixed, u32 mask_dp)
{
    u32 flags = ece->act_flags;

    return ((flags & mask_fixed) ? "Fixed" : (flags & mask_dp) ? "DP" : "Classified");
}

static const char *vtss_debug_ece_pcp_mode(vtss_ece_entry_t *ece, u32 mask_fixed, u32 mask_mapped)
{
    u32 flags = ece->act_flags;

    return ((flags & mask_fixed) ? "Fixed" : (flags & mask_mapped) ? "Mapped" : "Classified");
}
#endif /* VTSS_ARCH_SERVAL */

static void vtss_debug_print_port_list(const vtss_debug_printf_t pr, u8 *ports, BOOL nl)
{
    vtss_port_no_t port_no;
    u32            count = 0, max = (vtss_state->port_count - 1);
    BOOL           member, first = 1;

    for (port_no = VTSS_PORT_NO_START; port_no <= max; port_no++) {
        member = VTSS_PORT_BF_GET(ports, port_no);
        if ((member && (count == 0 || port_no == max)) || (!member && count > 1)) {
            pr("%s%u",
               first ? "" : count > (member ? 1 : 2) ? "-" : ",",
               member ? port_no : (port_no - 1));
            first = 0;
        }
        if (member)
            count++;
        else
            count=0;
    }
    if (first)
        pr("None");
    if (nl)
        pr("\n");
}

static void vtss_debug_print_vr(const vtss_debug_printf_t pr, vtss_vcap_vr_t *vr, u32 bits)
{
    vtss_vcap_vr_value_t value, mask, i, zeros = 0, range = 1;

    if (vr->type == VTSS_VCAP_VR_TYPE_VALUE_MASK) {
        value = vr->vr.v.value;
        mask = vr->vr.v.mask;
        pr("0x%x/0x%x", value, mask);

        /* Check if value/mask is a contiguous range */
        for (i = 0; i < bits; i++) {
            if (mask & (1 << (bits - i - 1))) {
                if (zeros)
                    range = 0;
            } else {
                zeros++;
            }
        }
        if (range) {
            pr(" (%u-%u)", value, value + (1 << zeros) - 1);
        }
    } else 
        pr("%u-%u (%s)", vr->vr.r.low, vr->vr.r.high, 
           vr->type == VTSS_VCAP_VR_TYPE_RANGE_INCLUSIVE ? "Incl" : "Excl");
    pr("\n");
}

#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
static void vtss_debug_print_ece_ipv6_addr(const vtss_debug_printf_t pr, u8 *p)
{
    int i;

    for (i = 0; i < 16; i++) {
        pr("%02x%s", p[i], i == 15 ? "\n" : (i & 1) ? ":" : "");
    }
}

static const char *vtss_vcap_bit_txt(vtss_vcap_bit_t value)
{
    return (value == VTSS_VCAP_BIT_0 ? "0" : value == VTSS_VCAP_BIT_1 ? "1" : "X");
}
#endif /* VTSS_ARCH_CARACAL/SERVAL */

#if defined(VTSS_SDX_CNT)
static void vtss_debug_print_sdx_list(const vtss_debug_printf_t pr, 
                                      vtss_sdx_entry_t *isdx_list,
                                      vtss_sdx_entry_t *esdx_list)
{
    vtss_port_no_t   port_no;
    vtss_sdx_entry_t *isdx;
    vtss_sdx_entry_t *esdx;
    BOOL             first = 1;
    
    for (port_no = VTSS_PORT_NO_START; port_no <= vtss_state->port_count; port_no++) {
        if (port_no == vtss_state->port_count)
            port_no = VTSS_PORT_NO_CPU;
        for (isdx = isdx_list; isdx != NULL; isdx = isdx->next) {
            if (isdx->port_no == port_no)
                break;
        }
        for (esdx = esdx_list; esdx != NULL; esdx = esdx->next) {
            if (esdx->port_no == port_no)
                break;
        }
        if (isdx != NULL || esdx != NULL) {
            pr("%s", first ? "" : ",");
            if (port_no == VTSS_PORT_NO_CPU)
                pr("CPU");
            else
                pr("%u", port_no);
            if (isdx == NULL)
                pr("/-");
            else
                pr("/%u", isdx->sdx);
            if (esdx == NULL)
                pr("/-");
            else
                pr("/%u", esdx->sdx);
            first = 0;
        }
    }
    pr("%s\n", first ? "-" : "");
}
#endif /* VTSS_SDX_CNT */

#if defined(VTSS_ARCH_SERVAL)
static void vtss_debug_print_mce_tag(const vtss_debug_printf_t pr, vtss_mce_tag_t *tag)
{
    pr("%s %s 0x%03x/%03x 0x%x/%x %s\n", 
       vtss_vcap_bit_txt(tag->tagged), 
       vtss_vcap_bit_txt(tag->s_tagged), 
       tag->vid.value, tag->vid.mask, 
       tag->pcp.value, tag->pcp.mask, 
       vtss_vcap_bit_txt(tag->dei));
}

static void vtss_debug_print_mce_port(const vtss_debug_printf_t pr, vtss_port_no_t port_no)
{
    if (port_no == VTSS_PORT_NO_NONE)
        pr("-");
    else if (port_no == VTSS_PORT_NO_CPU)
        pr("CPU ");
    else
        pr("%-4u", port_no);
    pr("\n");
}

static const char *vcap_key_type_txt(vtss_vcap_key_type_t key_type)
{
    return (key_type == VTSS_VCAP_KEY_TYPE_NORMAL ? "NORMAL" :
            key_type == VTSS_VCAP_KEY_TYPE_DOUBLE_TAG ? "DOUBLE_TAG" :
            key_type == VTSS_VCAP_KEY_TYPE_IP_ADDR ? "IP_ADDR" :
            key_type == VTSS_VCAP_KEY_TYPE_MAC_IP_ADDR ? "MAC_IP_ADDR" : "?");
}
#endif /* VTSS_ARCH_SERVAL */

static void vtss_debug_print_evc(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    u32                   i;
    vtss_port_no_t        port_no;
    vtss_evc_port_info_t  *port_info;
    vtss_evc_port_conf_t  *port_conf;
#if defined(VTSS_SDX_CNT)
    vtss_sdx_info_t       *sdx_info = &vtss_state->sdx_info;
#endif /* VTSS_SDX_CNT */
    vtss_evc_info_t       *evc_info = &vtss_state->evc_info;
    vtss_evc_entry_t      *evc;
    vtss_ece_info_t       *ece_info = &vtss_state->ece_info;
    vtss_ece_entry_t      *ece;
    int                   w = 15;
    BOOL                  header = 1, is_ipv6;
    vtss_ece_frame_ipv4_t *ipv4;
    vtss_ece_frame_ipv6_t *ipv6;
    vtss_vcap_vr_t        vr;
    char                  buf[16];
    
#if defined(VTSS_ARCH_JAGUAR_1)
    vtss_debug_print_vcap_is0(pr, info);
#endif /* VTSS_ARCH_JAGUAR_1 */

#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
    vtss_debug_print_vcap_is1(pr, info);
#endif /* VTSS_ARCH_CARACAL/SERVAL */

    vtss_debug_print_vcap_es0(pr, info);
    
    vtss_debug_print_range_checkers(pr, info);

    /* Port table */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            vtss_debug_print_header(pr, "Ports");
            pr("Port  UNI   NNI   ");
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_CARACAL)
            pr("DEI Colouring  ");
#endif /* VTSS_ARCH_JAGUAR_1/CARACAL */
#if defined(VTSS_ARCH_CARACAL)
            pr("Tag    ");
#endif /* VTSS_ARCH_CARACAL */
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
            pr("DMAC/DIP  ");
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
            pr("Key Type");
#endif /* VTSS_ARCH_SERVAL */

            pr("\n");
        }
        port_info = &vtss_state->evc_port_info[port_no];
        port_conf = &vtss_state->evc_port_conf[port_no];
        pr("%-6u%-6u%-6u", port_no, port_info->uni_count, port_info->nni_count);
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_CARACAL)
        pr("%-15s", vtss_bool_txt(port_conf->dei_colouring));
#endif /* VTSS_ARCH_JAGUAR_1/CARACAL */
#if defined(VTSS_ARCH_CARACAL)
        pr("%-7s", port_conf->inner_tag ? "Inner" : "Outer");
#endif /* VTSS_ARCH_CARACAL */
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
        pr("%-10s", vtss_bool_txt(port_conf->dmac_dip));
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
        pr("%s", vcap_key_type_txt(port_conf->key_type));
#endif /* VTSS_ARCH_SERVAL */
        pr("\n");
    }
    if (!header)
        pr("\n");
    header = 1;
    
    /* Policer table */
    vtss_debug_print_dlb(pr, info);

#if defined(VTSS_SDX_CNT)
    /* SDX table */
    vtss_debug_print_header(pr, "SDX");
    pr("state size: %zu\n", sizeof(*sdx_info));
    pr("max_count : %u\n", sdx_info->max_count);
    pr("ISDX count: %u\n", sdx_info->isdx.count);
    pr("ESDX count: %u\n", sdx_info->esdx.count);
    pr("poll_idx  : %u\n\n", sdx_info->poll_idx);
#endif /* VTSS_SDX_CNT */
    
#if defined(VTSS_ARCH_SERVAL)
    /* VCAP Port table */
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            vtss_debug_print_header(pr, "VCAP Port");
            pr("Port  Key Type\n");
        }
        pr("%-6u%s\n", port_no,
           vcap_key_type_txt(vtss_state->vcap_port_conf[port_no].key_type_is1_1));
    }
    if (!header)
        pr("\n");
    header = 1;
    
    /* MCEs */
    {
        vtss_mce_info_t      *mce_info = &vtss_state->mce_info;
        vtss_mce_entry_t     *mce;
        vtss_mce_key_t       *key;
        vtss_mce_action_t    *action;
        vtss_mce_outer_tag_t *ot;

        vtss_debug_print_header(pr, "MCEs");
        pr("state size: %zu\n", sizeof(*mce_info));
        pr("entry size: %zu\n", sizeof(*mce));
        pr("max_count : %u\n", mce_info->max_count);
        pr("count     : %u\n\n", mce_info->count);

        for (mce = mce_info->used; mce != NULL; mce = mce->next) {
            pr("%-*s: %u\n", w, "MCE ID", mce->conf.id);
            
            /* Key fields */
            key = &mce->conf.key;
            pr("%-*s: ", w, "Rx Port");
            vtss_debug_print_mce_port(pr, key->port_no);
            pr("%-*s: ", w, "Outer Tag");
            vtss_debug_print_mce_tag(pr, &key->tag);
            pr("%-*s: ", w, "Inner Tag");
            vtss_debug_print_mce_tag(pr, &key->inner_tag);
            pr("%-*s: 0x%02x/%02x\n", w, "MEL", key->mel.value, key->mel.mask);
            pr("%-*s: %s\n", w, "Injected", vtss_vcap_bit_txt(key->injected));
            pr("%-*s: %u\n", w, "Lookup", key->lookup);
            
            /* Action fields */
            action = &mce->conf.action;
            pr("%-*s: ", w, "Tx Port");
            vtss_debug_print_mce_port(pr, action->port_no);
            pr("%-*s: ", w, "VOE");
            if (action->voe_idx == VTSS_OAM_VOE_IDX_NONE)
                pr("-");
            else
                pr("%u", action->voe_idx);
            pr("\n");
            ot = &action->outer_tag;
            pr("%-*s: %s\n", w, "Egress Tag", vtss_bool_txt(ot->enable));
            pr("%-*s: %u\n", w, "VID", ot->vid);
            pr("%-*s: ", w, "PCP");
            if (ot->pcp_mode == VTSS_MCE_PCP_MODE_FIXED)
                pr("%u", ot->pcp);
            else
                pr("Mapped");
            pr("\n");
            pr("%-*s: ", w, "DEI");
            if (ot->dei_mode == VTSS_MCE_DEI_MODE_FIXED)
                pr("%u", ot->dei);
            else
                pr("DP"); 
            pr("\n");
            pr("%-*s: %s\n", w, "Tx Lookup",
               action->tx_lookup == VTSS_MCE_TX_LOOKUP_VID ? "VID" :
               action->tx_lookup == VTSS_MCE_TX_LOOKUP_ISDX ? "ISDX" : "ISDX_PCP");
            pr("%-*s: %s\n", w, "OAM Detect",
               action->oam_detect == VTSS_MCE_OAM_DETECT_UNTAGGED ? "Untagged" :
               action->oam_detect == VTSS_MCE_OAM_DETECT_SINGLE_TAGGED ? "Single Tagged" :
               action->oam_detect == VTSS_MCE_OAM_DETECT_DOUBLE_TAGGED ? "Double Tagged" : "None");
            pr("%-*s: %u\n", w, "ISDX", action->isdx);
            pr("%-*s: ", w, "Port/ISDX/ESDX");
            vtss_debug_print_sdx_list(pr, mce->isdx_list, mce->esdx_list);
            pr("%-*s: %u\n", w, "POP Count", action->pop_cnt);
            pr("%-*s: %s\n", w, "Policy", vtss_acl_policy_no_txt(action->policy_no, buf));
            pr("%-*s: ", w, "Priority");
            if (action->prio_enable)
                pr("%u", action->prio);
            else
                pr("-");
            pr("\n");
            pr("%-*s: %u\n", w, "VID", action->vid);
            pr("\n");
        }
    }
#endif /* VTSS_ARCH_SERVAL */

    /* EVCs */
    vtss_debug_print_header(pr, "EVCs");
    pr("state size: %zu\n", sizeof(*evc_info));
    pr("entry size: %zu\n", sizeof(*evc));
    pr("max_count : %u\n", evc_info->max_count);
    pr("count     : %u\n\n", evc_info->count);
    for (i = 0; i < VTSS_EVCS; i++) {
        evc = &evc_info->table[i];
        if (evc->enable == 0)
            continue;
        if (header) {
            header = 0;
            pr("EVC ID  VID   IVID  Learning  ");
#if defined(VTSS_ARCH_CARACAL)
            pr("UVID  Inner Tag           ");
#endif /* VTSS_ARCH_CARACAL */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
            pr("Policer  ");
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
            pr("NNI Ports\n");
        }
        pr("%-8u%-6u%-6u%-10s", 
           i, evc->vid, evc->ivid, vtss_bool_txt(evc->learning));
#if defined(VTSS_ARCH_CARACAL)
        {
            vtss_evc_inner_tag_t *it = &evc->inner_tag;
            pr("%-6u%-4s:%s/0x%03x/%s/%u/%u  ",
               evc->uvid,
               it->type == VTSS_EVC_INNER_TAG_NONE ? "None" :
               it->type == VTSS_EVC_INNER_TAG_C ? "C" :
               it->type == VTSS_EVC_INNER_TAG_S ? "S" :
               it->type == VTSS_EVC_INNER_TAG_S_CUSTOM ? "S-Cu" : "?",
               it->vid_mode == VTSS_EVC_VID_MODE_TUNNEL ? "T" : "N",
               it->vid, it->pcp_dei_preserve ? "P" : "F", it->pcp, it->dei);
        }
#endif /* VTSS_ARCH_CARACAL */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
        pr("%-9u", evc->policer_id);
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
        vtss_debug_print_port_list(pr, evc->ports, 1);
    }
    if (!header)
        pr("\n");

#if defined(VTSS_ARCH_SERVAL)
    /* EVC VOE mappings */
    header = 1;
    for (i = 0; i < VTSS_EVCS; i++) {
        evc = &evc_info->table[i];
        if (evc->enable == 0)
            continue;
        if (header)
            pr("EVC ID  VOEs\n");
        header = 1;
        pr("%-8u", i);
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (evc->voe_idx[port_no] == VTSS_EVC_VOE_IDX_NONE)
                continue;
            pr("%s%u/%u", header ? "" : ",", port_no, evc->voe_idx[port_no]);
            header = 0;
        }
        pr("%s\n", header ? "None" : "");
        header = 0;
    }
    if (!header)
        pr("\n");
#endif /* VTSS_ARCH_SERVAL */

    /* ECEs */
    vtss_debug_print_header(pr, "ECEs");
    pr("state size: %zu\n", sizeof(*ece_info));
    pr("entry size: %zu\n", sizeof(*ece));
    pr("max_count : %u\n", ece_info->max_count);
    pr("count     : %u\n\n", ece_info->count);
    for (ece = vtss_state->ece_info.used; ece != NULL; ece = ece->next) {
        pr("%-*s: %u\n", w, "ECE ID", ece->ece_id);

        /* Key fields */
        pr("%-*s: ", w, "UNI Ports");
        vtss_debug_print_port_list(pr, ece->ports, 1);
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %s\n", w, "DMAC MC", 
           vtss_debug_ece_flags(ece, VTSS_ECE_KEY_DMAC_MC_VLD, VTSS_ECE_KEY_DMAC_MC_1));
        pr("%-*s: %s\n", w, "DMAC BC", 
           vtss_debug_ece_flags(ece, VTSS_ECE_KEY_DMAC_BC_VLD, VTSS_ECE_KEY_DMAC_BC_1));
        pr("%-*s: ", w, "SMAC");
        for (i = 0; i < 12; i++) {
            pr("%02x%s", 
               i < 6 ? ece->smac.value[i] : ece->smac.mask[i - 6],
               i == 5 ? "/" : i == 11 ? "\n" : "-");
        }
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
        pr("%-*s: ", w, "DMAC");
        for (i = 0; i < 12; i++) {
            pr("%02x%s", 
               i < 6 ? ece->dmac.value[i] : ece->dmac.mask[i - 6],
               i == 5 ? "/" : i == 11 ? "\n" : "-");
        }
#endif /* VTSS_ARCH_SERVAL */
        pr("%-*s: %s\n", w, "Tagged",
           vtss_debug_ece_flags(ece, VTSS_ECE_KEY_TAG_TAGGED_VLD, VTSS_ECE_KEY_TAG_TAGGED_1));
        pr("%-*s: %s\n", w, "S-Tagged",
           vtss_debug_ece_flags(ece, VTSS_ECE_KEY_TAG_S_TAGGED_VLD, VTSS_ECE_KEY_TAG_S_TAGGED_1));
        pr("%-*s: ", w, "VID");
        vtss_debug_print_vr(pr, &ece->vid, 12);
        pr("%-*s: ", w, "PCP");
        vr.type = VTSS_VCAP_VR_TYPE_VALUE_MASK;
        vr.vr.v.value = ece->pcp.value;
        vr.vr.v.mask = ece->pcp.mask;
        vtss_debug_print_vr(pr, &vr, 3);
        pr("%-*s: %s\n", w, "DEI", 
           vtss_debug_ece_flags(ece, VTSS_ECE_KEY_TAG_DEI_VLD, VTSS_ECE_KEY_TAG_DEI_1));
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %s\n", w, "In-Tagged",
           vtss_debug_ece_flags(ece, VTSS_ECE_KEY_IN_TAG_TAGGED_VLD,
                                VTSS_ECE_KEY_IN_TAG_TAGGED_1));
        pr("%-*s: %s\n", w, "In-S-Tagged",
           vtss_debug_ece_flags(ece, VTSS_ECE_KEY_IN_TAG_S_TAGGED_VLD,
                                VTSS_ECE_KEY_IN_TAG_S_TAGGED_1));
        pr("%-*s: ", w, "In-VID");
        vtss_debug_print_vr(pr, &ece->in_vid, 12);
        pr("%-*s: ", w, "In-PCP");
        vr.vr.v.value = ece->in_pcp.value;
        vr.vr.v.mask = ece->in_pcp.mask;
        vtss_debug_print_vr(pr, &vr, 3);
        pr("%-*s: %s\n", w, "In-DEI", 
           vtss_debug_ece_flags(ece, VTSS_ECE_KEY_IN_TAG_DEI_VLD, VTSS_ECE_KEY_IN_TAG_DEI_1));
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
        ipv4 = ((ece->key_flags & VTSS_ECE_KEY_PROT_IPV4) ? &ece->frame.ipv4 : NULL);
        is_ipv6 = VTSS_BOOL(ece->key_flags & VTSS_ECE_KEY_PROT_IPV6);
        ipv6 = &ece->frame.ipv6;
        pr("%-*s: %s\n", w, "Frame", ipv4 ? "IPv4" : is_ipv6 ? "IPv6" : "Any");
        if (ipv4 || is_ipv6) { /* The is_ipv6 variable is used only to please Lint */
            vtss_vcap_vr_t  *dscp = (ipv4 ? &ipv4->dscp : &ipv6->dscp);
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
            vtss_vcap_u8_t  *proto = (ipv4 ? &ipv4->proto : &ipv6->proto);
            
            pr("%-*s: 0x%02x/0x%02x\n", w, "Protocol", proto->value, proto->mask);
#endif /* VTSS_ARCH_CARACAL/SERVAL */
            pr("%-*s: ", w, "DSCP");
            vtss_debug_print_vr(pr, dscp, 6);
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
            if (ipv4) {
                pr("%-*s: %s\n", w, "Fragment", vtss_vcap_bit_txt(ipv4->fragment));
                pr("%-*s: 0x%08x/0x%08x\n", w, "SIP", ipv4->sip.value, ipv4->sip.mask);
#if defined(VTSS_ARCH_SERVAL)
                pr("%-*s: 0x%08x/0x%08x\n", w, "DIP", ipv4->dip.value, ipv4->dip.mask);
#endif /* VTSS_ARCH_SERVAL */
            } else {
                pr("%-*s: ", w, "SIP Value");
                vtss_debug_print_ece_ipv6_addr(pr, ipv6->sip.value);
                pr("%-*s: ", w, "SIP Mask");
                vtss_debug_print_ece_ipv6_addr(pr, ipv6->sip.mask);
#if defined(VTSS_ARCH_SERVAL)
                pr("%-*s: ", w, "DIP Value");
                vtss_debug_print_ece_ipv6_addr(pr, ipv6->dip.value);
                pr("%-*s: ", w, "DIP Mask");
                vtss_debug_print_ece_ipv6_addr(pr, ipv6->dip.mask);
#endif /* VTSS_ARCH_SERVAL */
            }
            for (i = 0; i < 2; i++) {
                vtss_vcap_vr_t  *l4port = (i == 0 ? (ipv4 ? &ipv4->sport : &ipv6->sport) :
                                           (ipv4 ? &ipv4->dport : &ipv6->dport));
                pr("%-*s: ", w, i == 0 ? "SPort" : "DPort");
                vtss_debug_print_vr(pr, l4port, 16);
            }
#endif /* VTSS_ARCH_CARACAL/SERVAL */
        }
        
        /* Action fields */
        pr("%-*s: %s\n", w, "Direction", 
           (ece->act_flags & VTSS_ECE_ACT_DIR_UNI_TO_NNI) ? "UNI_TO_NNI" : 
           (ece->act_flags & VTSS_ECE_ACT_DIR_NNI_TO_UNI) ? "NNI_TO_UNI" : "BOTH");
#if defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %s\n", w, "Rule Type", 
           (ece->act_flags & VTSS_ECE_ACT_RULE_RX) ? "RX" : 
           (ece->act_flags & VTSS_ECE_ACT_RULE_TX) ? "TX" : "BOTH");
        pr("%-*s: %s\n", w, "Tx Lookup", 
           (ece->act_flags & VTSS_ECE_ACT_TX_LOOKUP_VID_PCP) ? "VID_PCP" : 
           (ece->act_flags & VTSS_ECE_ACT_TX_LOOKUP_ISDX) ? "ISDX" : "VID");
#endif /* VTSS_ARCH_SERVCAL */
        pr("%-*s: %u\n", w, "EVC ID", ece->evc_id);
        pr("%-*s: %s\n", w, "Policy",
           vtss_acl_policy_no_txt(ece->act_flags & VTSS_ECE_ACT_POLICY_NONE ?
                                  VTSS_ACL_POLICY_NO_NONE : ece->policy_no, buf));
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %u\n", w, "Policer", ece->policer_id);
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
#if defined(VTSS_ARCH_CARACAL) || defined(VTSS_ARCH_SERVAL)
        pr("%-*s: ", w, "Priority");
        if (ece->act_flags & VTSS_ECE_ACT_PRIO_ENA)
            pr("%u", ece->prio);
        else
            pr("-");
        pr("\n");
#endif /* VTSS_ARCH_CARACAL/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
        pr("%-*s: ", w, "DP");
        if (ece->act_flags & VTSS_ECE_ACT_DP_ENA)
            pr("%u", ece->dp);
        else
            pr("-");
        pr("\n");
#endif /* VTSS_ARCH_SERVCAL */
        pr("%-*s: %u\n", w, "Pop Count", 
           (ece->act_flags & VTSS_ECE_ACT_POP_2) ? 2 : 
           (ece->act_flags & VTSS_ECE_ACT_POP_1) ? 1 : 0);
        pr("%-*s: %s\n", w, "Outer Tag", 
           vtss_bool_txt(ece->act_flags & VTSS_ECE_ACT_OT_ENA ? 1 : 0)); 
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %u\n", w, "Outer VID", ece->ot_vid);
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
#if defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %s\n", w, "Outer PCP Mode", 
           vtss_debug_ece_pcp_mode(ece, VTSS_ECE_ACT_OT_PCP_MODE_FIXED,
                                   VTSS_ECE_ACT_OT_PCP_MODE_MAPPED));
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_CARACAL)
        pr("%-*s: %s\n", w, "Outer PCP/DEI",
           vtss_debug_ece_pcp_dei(ece, VTSS_ECE_ACT_OT_PCP_MODE_FIXED));
#endif /* VTSS_ARCH_JAGUAR_1/CARACAL */
        pr("%-*s: %u\n", w, "Outer PCP", ece->ot_pcp);
#if defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %s\n", w, "Outer DEI Mode", 
           vtss_debug_ece_dei_mode(ece, VTSS_ECE_ACT_OT_DEI_MODE_FIXED,
                                   VTSS_ECE_ACT_OT_DEI_MODE_DP));
#endif /* VTSS_ARCH_SERVAL */
        pr("%-*s: %u\n", w, "Outer DEI", ece->act_flags & VTSS_ECE_ACT_OT_DEI ? 1 : 0);
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %s\n", w, "Inner Tag", 
           ece->act_flags & VTSS_ECE_ACT_IT_TYPE_C ? "C" :
           ece->act_flags & VTSS_ECE_ACT_IT_TYPE_S ? "S" :
           ece->act_flags & VTSS_ECE_ACT_IT_TYPE_S_CUSTOM ? "S-Custom" : "None");
        pr("%-*s: %u\n", w, "Inner VID", ece->it_vid);
#if defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %s\n", w, "Inner PCP Mode", 
           vtss_debug_ece_pcp_mode(ece, VTSS_ECE_ACT_IT_PCP_MODE_FIXED,
                                   VTSS_ECE_ACT_IT_PCP_MODE_MAPPED));
#endif /* VTSS_ARCH_SERVAL */
#if defined(VTSS_ARCH_JAGUAR_1) || defined(VTSS_ARCH_CARACAL)
        pr("%-*s: %s\n", w, "Inner PCP/DEI",
           vtss_debug_ece_pcp_dei(ece, VTSS_ECE_ACT_IT_PCP_MODE_FIXED));
#endif /* VTSS_ARCH_JAGUAR_1/CARACAL */
        pr("%-*s: %u\n", w, "Inner PCP", ece->it_pcp);
#if defined(VTSS_ARCH_SERVAL)
        pr("%-*s: %s\n", w, "Inner DEI Mode", 
           vtss_debug_ece_dei_mode(ece, VTSS_ECE_ACT_IT_DEI_MODE_FIXED,
                                   VTSS_ECE_ACT_IT_DEI_MODE_DP));
#endif /* VTSS_ARCH_SERVAL */
        pr("%-*s: %u\n", w, "Inner DEI", ece->act_flags & VTSS_ECE_ACT_IT_DEI ? 1 : 0);
        pr("%-*s: ", w, "Port/ISDX/ESDX");
        vtss_debug_print_sdx_list(pr, ece->isdx_list, ece->esdx_list);
#endif /* VTSS_ARCH_JAGUAR_1/SERVAL */
        pr("\n");
    }
}
#endif /* VTSS_FEATURE_EVC */

static void vtss_debug_print_erps(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    vtss_erpi_t       erpi;
    vtss_erps_entry_t *entry;
    int               i;
    vtss_vid_t        vid;

    vtss_debug_print_port_header(pr, "ERPI  ", 0, 0);
    pr("  VLANs\n");
    for (erpi = VTSS_ERPI_START; erpi < VTSS_ERPI_END; erpi++) {
        entry = &vtss_state->erps_table[erpi];
        pr("%-4u  ", erpi);
        vtss_debug_print_ports(pr, entry->port_member, 0);
        pr("  ");
        for (vid = VTSS_VID_NULL, i = 0; vid < VTSS_VIDS; vid++) {
            if (VTSS_BF_GET(entry->vlan_member, vid)) {
                pr("%s%u", i ? "," : "", vid);
                i++;
            }
        }
        pr("%s\n", i ? "" : "-");
    }
    pr("\n");
}

static void vtss_debug_print_eps(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    vtss_port_no_t  port_no;
    vtss_port_eps_t *eps;
    char            buf[16];
    BOOL            header = 1;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        if (header) {
            header = 0;
            pr("Port  Protection  Type  Selector\n");
        }
        eps = &vtss_state->port_protect[port_no];
        if (eps->conf.port_no == VTSS_PORT_NO_NONE)
            strcpy(buf, "None");
        else
            sprintf(buf, "%-4u", eps->conf.port_no);
        pr("%-4u  %-10s  %-4s  %s\n",
           port_no, buf, eps->conf.type == VTSS_EPS_PORT_1_PLUS_1 ? "1+1" : "1:1",
           eps->selector == VTSS_EPS_SELECTOR_WORKING ? "Working" : "Protection");
    }
    if (!header)
        pr("\n");
}

static void vtss_debug_print_layer2(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_VLAN))
        vtss_debug_print_vlan(pr, info);
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_PVLAN))
        vtss_debug_print_pvlan(pr, info);
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_MAC_TABLE))
        vtss_debug_print_mac_table(pr, info);
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_AGGR))
        vtss_debug_print_aggr(pr, info);
#if defined(VTSS_FEATURE_AGGR_GLAG)
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_GLAG))
        vtss_debug_print_glag(pr, info);
#endif /* VTSS_FEATURE_AGGR_GLAG */
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_STP))
        vtss_debug_print_stp(pr, info);
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_MIRROR))
        vtss_debug_print_mirror(pr, info);
#if defined(VTSS_FEATURE_EVC) || defined(VTSS_FEATURE_QOS_POLICER_DLB)
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_EVC))
#if defined(VTSS_FEATURE_EVC)
        vtss_debug_print_evc(pr, info);
#else
        vtss_debug_print_dlb(pr, info);
#endif
#endif /* VTSS_FEATURE_EVC || VTSS_FEATURE_QOS_POLICER_DLB*/
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_ERPS))
        vtss_debug_print_erps(pr, info);
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_EPS))
        vtss_debug_print_eps(pr, info);
#if defined(VTSS_FEATURE_VLAN_TRANSLATION)
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_VXLAT))
        vtss_debug_print_vlan_trans(pr, info);
#endif /* VTSS_FEATURE_EVC */
}
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_ACL)
static void vtss_debug_print_acl(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    vtss_port_no_t        port_no;
    BOOL                  header = 1;
    vtss_acl_port_conf_t  *conf;
    vtss_acl_action_t     *act;
    vtss_acl_policer_no_t policer_no;
    char                  buf[64];
    
    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_ACL))
        return;

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        if (!info->port_list[port_no])
            continue;
        conf = &vtss_state->acl_port_conf[port_no];
        act = &conf->action;
        if (header) {
            header = 0;
            pr("Port  Policy  CPU  Once  Queue  Policer  Learn  ");
#if defined(VTSS_FEATURE_ACL_V1)
            pr("Forward  Port Copy");
#endif /* VTSS_FEATURE_ACL_V1 */
#if defined(VTSS_FEATURE_ACL_V2)
            vtss_debug_print_port_header(pr, "Mirror  PTP  Port  ", 0, 0);
#endif /* VTSS_FEATURE_ACL_V2 */
            pr("\n");
        }
        pr("%-6u%-8s%-5u%-6u%-7u",
           port_no, vtss_acl_policy_no_txt(conf->policy_no, buf),
           act->cpu, act->cpu_once, act->cpu_queue);
        if (act->police)
            sprintf(buf, "%u (ACL)", act->policer_no);
#if defined(VTSS_ARCH_LUTON26) && defined(VTSS_FEATURE_QOS_POLICER_DLB)
        else if (act->evc_police)
            sprintf(buf, "%u (EVC)", act->evc_policer_id);
#endif /* VTSS_ARCH_LUTON26 && VTSS_FEATURE_QOS_POLICER_DLB */
        else
            strcpy(buf, "Disabled");
        pr("%-9s%-7u", buf, act->learn);
#if defined(VTSS_FEATURE_ACL_V1)
        pr("%-9u", act->forward);
        if (act->port_forward)
            pr("%-9u", act->port_no);
        else
            pr("%-9s", "Disabled");
#endif /* VTSS_FEATURE_ACL_V1 */
#if defined(VTSS_FEATURE_ACL_V2)
        pr("%-8u%-5s%-6s", 
           act->mirror, 
           act->ptp_action == VTSS_ACL_PTP_ACTION_NONE ? "None" :
           act->ptp_action == VTSS_ACL_PTP_ACTION_ONE_STEP ? "One" :
           act->ptp_action == VTSS_ACL_PTP_ACTION_TWO_STEP ? "Two" : "?",
           act->port_action == VTSS_ACL_PORT_ACTION_NONE ? "None" :
           act->port_action == VTSS_ACL_PORT_ACTION_FILTER ? "Filt" :
           act->port_action == VTSS_ACL_PORT_ACTION_REDIR ? "Redir" : "?");
        vtss_debug_print_port_members(pr, act->port_list, 0);
#endif /* VTSS_FEATURE_ACL_V2 */
        pr("\n");
    }
    if (!header)
        pr("\n");

    pr("Policer  Rate        ");
#if defined(VTSS_ARCH_LUTON26)
    pr("Count  L26 Policer");
#endif /* VTSS_ARCH_LUTON26 */    
    pr("\n");
    for (policer_no = VTSS_ACL_POLICER_NO_START; policer_no < VTSS_ACL_POLICER_NO_END; policer_no++) {
        vtss_acl_policer_conf_t *pol_conf = &vtss_state->acl_policer_conf[policer_no];
        vtss_packet_rate_t      rate;
        
#if defined(VTSS_FEATURE_ACL_V2)
        if (pol_conf->bit_rate_enable) {
            rate = pol_conf->bit_rate;
            if (rate == VTSS_BITRATE_DISABLED) {
                strcpy(buf, "Disabled");
            } else if (rate < 100000) {
                sprintf(buf, "%u kbps", rate);
            } else if (rate < 100000000) {
                sprintf(buf, "%u Mbps", rate/1000);
            } else {
                sprintf(buf, "%u Gbps", rate/1000000);
            }
        } else 
#endif /* VTSS_FEATURE_ACL_V2 */
        {
            rate = pol_conf->rate;
            if (rate == VTSS_PACKET_RATE_DISABLED) {
                strcpy(buf, "Disabled");
            } else if (rate < 100000) {
                sprintf(buf, "%u pps", rate);
            } else if (rate < 100000000) {
                sprintf(buf, "%u kpps", rate/1000);
            } else {
                sprintf(buf, "%u Mpps", rate/1000000);
            }
        }
        pr("%-9u%-12s", policer_no, buf);
#if defined(VTSS_ARCH_LUTON26)
        {
            vtss_policer_alloc_t *pol_alloc = &vtss_state->acl_policer_alloc[policer_no];
            pr("%-7u%u", pol_alloc->count, pol_alloc->policer);
        }
#endif /* VTSS_ARCH_LUTON26 */    
        pr("\n");
    }
    pr("\n");

#if defined(VTSS_FEATURE_VCAP)
    vtss_debug_print_range_checkers(pr, info);
#endif /* VTSS_FEATURE_VCAP */

#if defined(VTSS_ARCH_LUTON26)
    vtss_debug_print_vcap_is1(pr, info);
#endif /* VTSS_ARCH_LUTON26 */    

#if defined(VTSS_FEATURE_IS2)
    vtss_debug_print_vcap_is2(pr, info);
#endif /* VTSS_FEATURE_IS2 */    
}
#endif /* VTSS_FEATURE_ACL */

#if defined(VTSS_FEATURE_QOS)
#if defined(VTSS_FEATURE_QOS_POLICER_CPU_SWITCH) || defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) || \
    defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH)  || defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
static void vtss_debug_print_packet_rate(const vtss_debug_printf_t pr,
                                         const vtss_debug_info_t   *const info,
                                         const char                *name,
                                         vtss_packet_rate_t        rate)
{
    if (rate == VTSS_PACKET_RATE_DISABLED) {
        pr("%-32s: Disabled\n", name);
    }
    else if (rate >= 1000) {
        pr("%-32s: %u kfps\n", name, rate/1000);
    }
    else {
        pr("%-32s: %u fps\n", name, rate);
    }
}
#endif /* defined(VTSS_FEATURE_QOS_POLICER_CPU_SWITCH) || defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) ||
          defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH)  || defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) */

static void vtss_debug_print_qos(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    vtss_qos_conf_t *conf = &vtss_state->qos_conf;
    vtss_port_no_t  port_no;
    
    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_QOS))
        return;
    
    vtss_debug_print_value(pr, "Number of priorities", conf->prios);
#if defined(VTSS_FEATURE_QOS_POLICER_CPU_SWITCH)
    vtss_debug_print_packet_rate(pr, info, "Storm MAC-based",   conf->policer_mac);
    vtss_debug_print_packet_rate(pr, info, "Storm Categorizer", conf->policer_cat);
    vtss_debug_print_packet_rate(pr, info, "Storm Learn",       conf->policer_learn);
#endif /* defined(VTSS_FEATURE_QOS_POLICER_CPU_SWITCH) */
#if defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH)
    vtss_debug_print_packet_rate(pr, info, "Storm Unicast",     conf->policer_uc);
#endif /* defined(VTSS_FEATURE_QOS_POLICER_UC_SWITCH) */
#if defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH)
    vtss_debug_print_packet_rate(pr, info, "Storm Multicast",   conf->policer_mc);
#endif /* defined(VTSS_FEATURE_QOS_POLICER_MC_SWITCH) */
#if defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH)
    vtss_debug_print_packet_rate(pr, info, "Storm Broadcast",   conf->policer_bc);
#endif /* defined(VTSS_FEATURE_QOS_POLICER_BC_SWITCH) */
    pr("\n");

#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    {
        int i;
        pr("QoS DSCP Classification Config:\n\n");
        pr("DSCP Trust QoS DPL\n");
        for (i = 0; i < 64; i++) {
            pr("%4d %5d %3u %3d\n", i, conf->dscp_trust[i], conf->dscp_qos_class_map[i], conf->dscp_dp_level_map[i]);
        }
        pr("\n");
    }
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */

#if defined(VTSS_FEATURE_QOS_DSCP_REMARK)
    {
        int i;
        pr("QoS DSCP Remarking Config:\n\n");
        pr("DSCP I_Remark ");
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
        pr("Translate QoS_Remap");
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
        pr("_DP0 QoS_Remap_DP1");
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
        pr("\n");

        for (i = 0; i < 64; i++) {
            pr("%4d %8d ", i, conf->dscp_remark[i]);
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
            pr("%9d %9d", conf->dscp_translate_map[i], conf->dscp_remap[i]);
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
            pr("     %9d", conf->dscp_remap_dp1[i]);
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE */
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
            pr("\n");
        }
        pr("\n");

#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
        pr("QoS DSCP Classification from QoS Config:\n\n");
        pr("QoS DSCP");
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
        pr("_DP0 DSCP_DP1");
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE */
        pr("\n");

        for (i = 0; i < VTSS_PRIO_ARRAY_SIZE; i++) {
            pr("%3d %4d", i, conf->dscp_qos_map[i]);
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE)
            pr("     %4d", conf->dscp_qos_map_dp1[i]);
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_DP_AWARE */
            pr("\n");
        }
        pr("\n");
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
    }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK */

#if defined(VTSS_FEATURE_LAYER2) || defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    pr("QoS Port Classification Config:\n\n");
    pr("Port PCP CLS ");
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    pr("DEI DPL T_EN D_EN");
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
    pr("\n");

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        if (info->port_list[port_no] == 0)
            continue;
        pr("%4u ", port_no);
#if defined(VTSS_FEATURE_LAYER2)
        pr("%3u %3u ", port_conf->usr_prio, port_conf->default_prio);
#endif /* VTSS_FEATURE_LAYER2 */
#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
        pr("%3u %3u %4u %4u", port_conf->default_dei, port_conf->default_dpl, port_conf->tag_class_enable, port_conf->dscp_class_enable);
#endif /* VTSS_FEATURE_QOS_CLASSIFICATION_V2 */
        pr("\n");
    }
    pr("\n");
#endif /* defined(VTSS_FEATURE_LAYER2) || defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) */

#if defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2)
    pr("QoS Port Classification PCP, DEI to QoS class, DP level Mapping:\n\n");
    pr("Port QoS class (2*PCP+DEI)           DP level (2*PCP+DEI)\n");

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int pcp, dei, class_ct = 0, dpl_ct = 0;
        char class_buf[40], dpl_buf[40];
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        if (info->port_list[port_no] == 0)
            continue;
        for (pcp = VTSS_PCP_START; pcp < VTSS_PCP_END; pcp++) {
            for (dei = VTSS_DEI_START; dei < VTSS_DEI_END; dei++) {
                const char *delim = ((pcp == VTSS_PCP_START) && (dei == VTSS_DEI_START)) ? "" : ",";
                class_ct += snprintf(class_buf + class_ct, sizeof(class_buf) - class_ct, "%s%u", delim, port_conf->qos_class_map[pcp][dei]);
                dpl_ct   += snprintf(dpl_buf   + dpl_ct,   sizeof(dpl_buf)   - dpl_ct,   "%s%u",  delim, port_conf->dp_level_map[pcp][dei]);
            }
        }
        pr("%4u %s %s\n", port_no, class_buf, dpl_buf);
    }
    pr("\n");
#endif /* defined(VTSS_FEATURE_QOS_CLASSIFICATION_V2) */

    pr("QoS Port Ingress Policer Config:\n\n");
    pr("Port Policer Burst      Rate");
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
    pr("       FC");
#endif
    pr("\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        int policer;
        if (info->port_list[port_no] == 0)
            continue;
        pr("%4u ", port_no);
        for (policer = 0; policer < VTSS_PORT_POLICERS; policer++) {
            pr("%7d 0x%08x 0x%08x",
               policer,
               port_conf->policer_port[policer].level,
               port_conf->policer_port[policer].rate);
#if defined(VTSS_FEATURE_QOS_PORT_POLICER_EXT_FC)
            pr(" %d", port_conf->policer_ext_port[policer].flow_control);
#endif
            pr("\n     ");
        }
        pr("\r");
    }
    pr("\n");

#if defined(VTSS_FEATURE_QOS_QUEUE_POLICER)
    pr("QoS Queue Ingress Policer Config:\n\n");
    pr("Port Queue Burst      Rate\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        int queue;
        if (info->port_list[port_no] == 0)
            continue;
        pr("%4u ", port_no);
        for (queue = 0; queue < VTSS_QUEUE_ARRAY_SIZE; queue++) {
            pr("%5d 0x%08x 0x%08x\n     ",
               queue,
               port_conf->policer_queue[queue].level,
               port_conf->policer_queue[queue].rate);
        }
        pr("\r");
    }
    pr("\n");
#endif  /* VTSS_FEATURE_QOS_QUEUE_POLICER */

#if defined(VTSS_FEATURE_QOS_SCHEDULER_V2)
    pr("QoS Port Scheduler Config:\n\n");
    pr("Port Sch Mode  Q0  Q1  Q2  Q3  Q4  Q5\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        int i;
        if (info->port_list[port_no] == 0)
            continue;
        pr("%4u %8s ", port_no, port_conf->dwrr_enable ? "Weighted" : "Strict");
        for (i=0; i<6; i++) {
            pr("%3u ", port_conf->queue_pct[i]);
        } 
        pr("\n");
    }
    pr("\n");
#endif /* VTSS_FEATURE_QOS_SCHEDULER_V2 */

#if defined(VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS)
    pr("QoS Queue Egress Shaper Config:\n\n");
    pr("Port Queue Burst      Rate       Excess\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        int queue;
        if (info->port_list[port_no] == 0)
            continue;
        pr("%4u ", port_no);
        for (queue = 0; queue < VTSS_QUEUE_ARRAY_SIZE; queue++) {
            pr("%5d 0x%08x 0x%08x %d\n     ",
               queue,
               port_conf->shaper_queue[queue].level,
               port_conf->shaper_queue[queue].rate,
               VTSS_BOOL(port_conf->excess_enable[queue]));
        }
        pr("\r");
    }
    pr("\n");
#endif  /* VTSS_FEATURE_QOS_EGRESS_QUEUE_SHAPERS */

    pr("QoS Port Egress Shaper Config:\n\n");
    pr("Port Burst      Rate\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        if (info->port_list[port_no] == 0)
            continue;
        pr("%4u 0x%08x 0x%08x\n",
           port_no,
           port_conf->shaper_port.level,
           port_conf->shaper_port.rate);
    }
    pr("\n");

#if defined(VTSS_FEATURE_QOS_TAG_REMARK_V2)
    pr("QoS Port Tag Remarking Config:\n\n");
    pr("Port Mode       PCP DEI DPL mapping\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        const char           *mode;
        if (info->port_list[port_no] == 0)
            continue;

        switch (port_conf->tag_remark_mode) {
        case VTSS_TAG_REMARK_MODE_CLASSIFIED:
            mode = "Classified";
            break;
        case VTSS_TAG_REMARK_MODE_DEFAULT:
            mode = "Default";
            break;
        case VTSS_TAG_REMARK_MODE_MAPPED:
            mode = "Mapped";
            break;
        default:
            mode = "?";
            break;
        }
        pr("%4u %-10s %3d %3d   %d %d %d %d\n",
           port_no,
           mode,
           port_conf->tag_default_pcp,
           port_conf->tag_default_dei,
           port_conf->tag_dp_map[0],
           port_conf->tag_dp_map[1],
           port_conf->tag_dp_map[2],
           port_conf->tag_dp_map[3]);
    }
    pr("\n");

    pr("QoS Port Tag Remarking Map:\n\n");
    pr("Port PCP (2*QoS class+DPL)           DEI (2*QoS class+DPL)\n");

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        int class, dpl, pcp_ct = 0, dei_ct = 0;
        char pcp_buf[40], dei_buf[40];
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        if (info->port_list[port_no] == 0)
            continue;
        for (class = VTSS_QUEUE_START; class < VTSS_QUEUE_END; class++) {
            for (dpl = 0; dpl < 2; dpl++) {
                const char *delim = ((class == VTSS_QUEUE_START) && (dpl == 0)) ? "" : ",";
                pcp_ct += snprintf(pcp_buf + pcp_ct, sizeof(pcp_buf) - pcp_ct, "%s%u", delim, port_conf->tag_pcp_map[class][dpl]);
                dei_ct += snprintf(dei_buf + dei_ct, sizeof(dei_buf) - dei_ct, "%s%u",  delim, port_conf->tag_dei_map[class][dpl]);
            }
        }
        pr("%4u %s %s\n", port_no, pcp_buf, dei_buf);
    }
    pr("\n");
#endif /* VTSS_FEATURE_QOS_TAG_REMARK_V2 */

#if defined(VTSS_FEATURE_QOS_DSCP_REMARK)
    pr("QoS Port DSCP Remarking Config:\n\n");
    pr("Port I_Mode E_Mode ");
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
    {
        int i;
        for (i = 0; i < VTSS_PRIO_ARRAY_SIZE; i++) {
            pr("Cl%d ", i);
        }
    }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
    pr("T_EN");
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
    pr("\n");

    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        if (info->port_list[port_no] == 0)
            continue;
        pr("%4u %6d ", port_no, port_conf->dscp_mode);
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V1)
        pr("%6d ", port_conf->dscp_remark);
        {
            int i;
            for (i = 0; i < VTSS_PRIO_ARRAY_SIZE; i++) {
                pr("%3d ", port_conf->dscp_map[i]);
            }
        }
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V1 */
#if defined(VTSS_FEATURE_QOS_DSCP_REMARK_V2)
        pr("%6d %4d", port_conf->dscp_emode, port_conf->dscp_translate);
#endif /* VTSS_FEATURE_QOS_DSCP_REMARK_V2 */
        pr("\n");
    }
    pr("\n");
#endif  /* VTSS_FEATURE_QOS_DSCP_REMARK */

#if defined(VTSS_FEATURE_QOS_WRED)
#if defined(VTSS_ARCH_JAGUAR_1)
    pr("QoS WRED Config:\n\n");
    pr("Port Queue Ena MinTh MaxTh MaxP1 MaxP2 MaxP3\n");
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        vtss_qos_port_conf_t *port_conf = &vtss_state->qos_port_conf[port_no];
        int queue;
        if (info->port_list[port_no] == 0)
            continue;
        pr("%4u ", port_no);
        for (queue = 0; queue < VTSS_QUEUE_ARRAY_SIZE; queue++) {
            pr("%5d %3d %5d %5d %5d %5d %5d\n     ",
               queue,
               VTSS_BOOL(port_conf->red[queue].enable),
               port_conf->red[queue].min_th,
               port_conf->red[queue].max_th,
               port_conf->red[queue].max_prob_1,
               port_conf->red[queue].max_prob_2,
               port_conf->red[queue].max_prob_3);
        }
        pr("\r");
    }
    pr("\n");
#endif /* VTSS_ARCH_JAGUAR_1 */
#endif  /* VTSS_FEATURE_QOS_WRED */

#if defined(VTSS_FEATURE_VCAP)
    vtss_debug_print_range_checkers(pr, info);
#endif /* VTSS_FEATURE_VCAP */

#if defined(VTSS_FEATURE_IS1)
    vtss_debug_print_vcap_is1(pr, info);
#endif /* VTSS_FEATURE_IS1 */    
}
#endif /* VTSS_FEATURE_QOS */

#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
u32 vtss_cmn_ip2u32(vtss_ip_addr_internal_t *ip, BOOL ipv6)
{
    u32 addr = 0, i;

    if (ipv6) {
        for (i = 12; i < 16; i++) {
            addr += (ip->ipv6.addr[i] << ((15 - i)*8));
        }
    } else {
        addr = ip->ipv4;
    }
    return addr;
}

/* IPMC source key is (SIP, SSM, VID) */
static u64 vtss_cmn_ip_mc_src_key(vtss_ipmc_src_data_t *src, BOOL ipv6)
{
    u64 key = vtss_cmn_ip2u32(&src->sip, ipv6);

    key = (key << 32);
    if (src->ssm)
        key += (1<<12);
    key += src->vid;
    return key;
}

/* IPMC destination key is (DIP32/DIP23, DIP) */
static u64 vtss_cmn_ip_mc_dst_key(vtss_ipmc_dst_data_t *dst, BOOL ipv6)
{
    u32 dip = vtss_cmn_ip2u32(&dst->dip, ipv6);
    u64 key = (ipv6 ? dip : (dip &  0x007fffff));

    key = (key << 32);
    key += dip;
    return key;
}

static vtss_rc vtss_cmn_ip_mc_add(vtss_ipmc_data_t *ipmc)
{
    vtss_ipmc_obj_t *obj = &vtss_state->ipmc.obj;
    vtss_ipmc_src_t *src, *src_prev = NULL, **src_list;
    vtss_ipmc_dst_t *dst = NULL, *dst_prev = NULL;
    vtss_vid_t      fid;
    u64             sip, ipmc_sip = vtss_cmn_ip_mc_src_key(&ipmc->src, ipmc->ipv6);
    u32             dip, ipmc_dip = vtss_cmn_ip_mc_dst_key(&ipmc->dst, ipmc->ipv6);

    /* Search for source entry or the place to insert the new entry */
    src_list = &obj->src_used[ipmc->ipv6];
    for (src = *src_list; src != NULL; src_prev = src, src = src->next) {
        sip = vtss_cmn_ip_mc_src_key(&src->data, ipmc->ipv6);
        if (sip > ipmc_sip) {
            /* Found bigger entry */
            src = NULL;
            break;
        } else if (sip == ipmc_sip) {
            /* Found matching key, save allocated FID */
            ipmc->src.fid = src->data.fid;
            if (!ipmc->ipv6 ||
                memcmp(&src->data.sip.ipv6, &ipmc->src.sip.ipv6, sizeof(vtss_ipv6_t)) == 0) {
                /* Found identical entry */
                break;
            }
        }
    }

    if (src == NULL) {
        /* Check if source entry can be allocated */
        if (obj->src_free == NULL) {
            VTSS_I("no more source entries");
            return VTSS_RC_ERROR;
        }
        ipmc->src_add = 1;
    } else {
        /* Source found, search for destination entry or the place to insert the new entry */
        for (dst = src->dest; dst != NULL; dst_prev = dst, dst = dst->next) {
            dip = vtss_cmn_ip_mc_dst_key(&dst->data, ipmc->ipv6);
            if (dip > ipmc_dip) {
                /* Found bigger entry */
                dst = NULL;
                break;
            } 
            
            if (dip == ipmc_dip && 
                (!ipmc->ipv6 || 
                 memcmp(&dst->data.dip.ipv6, &ipmc->dst.dip.ipv6, sizeof(vtss_ipv6_t)) == 0)) {
                /* Found identical entry */
                memcpy(dst->data.member, ipmc->dst.member, VTSS_PORT_BF_SIZE);
                break;
            }
        }
    }

    if (dst == NULL) {
        /* Check if destination entry can be allocated */
        if (obj->dst_free == NULL) {
            VTSS_I("no more destination entries");
            return VTSS_RC_ERROR;
        }
        ipmc->dst_add = 1;
    }

    /* Check that resources can be added in device */
    fid = ipmc->src.fid;
    VTSS_FUNC_RC(ip_mc_update, ipmc, VTSS_IPMC_CMD_CHECK);

    /* Now allocate resources */
    if (src == NULL) {
        /* Allocate new source entry */
        src = obj->src_free;
        obj->src_free = src->next;
        obj->src_count++;
        if (src_prev == NULL) {
            /* Insert first in list */
            src->next = *src_list;
            *src_list = src;
        } else {
            /* Insert after previous entry */
            src->next = src_prev->next;
            src_prev->next = src;
        }
        src->data = ipmc->src;
        src->dest = NULL;
    }

    if (dst == NULL) {
        /* Allocate new destination entry */
        dst = obj->dst_free;
        obj->dst_free = dst->next;
        obj->dst_count++;
        if (dst_prev == NULL) {
            /* Insert first in list */
            dst->next = src->dest;
            src->dest = dst;
        } else {
            /* Insert after previous entry */
            dst->next = dst_prev->next;
            dst_prev->next = dst;
        }
        dst->data = ipmc->dst;
    }

    /* Add ressources in device */
    ipmc->src.fid = fid;
    VTSS_FUNC_RC(ip_mc_update, ipmc, VTSS_IPMC_CMD_ADD);

    return VTSS_RC_OK;
}

static vtss_rc vtss_cmn_ip_mc_del(vtss_ipmc_data_t *ipmc)
{
    vtss_ipmc_obj_t *obj = &vtss_state->ipmc.obj;
    vtss_ipmc_src_t *src, *src_prev = NULL, **src_list;
    vtss_ipmc_dst_t *dst = NULL, *dst_prev = NULL;
    
    /* Search for source entry */
    src_list = &obj->src_used[ipmc->ipv6];
    for (src = *src_list; src != NULL; src_prev = src, src = src->next) {
        if (src->data.vid == ipmc->src.vid && 
            ((!ipmc->ipv6 && ipmc->src.sip.ipv4 == src->data.sip.ipv4) ||
             (ipmc->ipv6 && 
              memcmp(&src->data.sip.ipv6, &ipmc->src.sip.ipv6, sizeof(vtss_ipv6_t)) == 0))) {
            ipmc->src.fid = src->data.fid;
            /* Found entry */
            break;
        }
    }

    if (src == NULL) {
        VTSS_E("IPv%u SIP 0x%08x not found", 
               ipmc->ipv6 ? 6 : 4, vtss_cmn_ip2u32(&ipmc->src.sip, ipmc->ipv6));
        return VTSS_RC_ERROR;
    }

    /* Search for destination entry */
    for (dst = src->dest; dst != NULL; dst_prev = dst, dst = dst->next) {
        if ((!ipmc->ipv6 && dst->data.dip.ipv4 == ipmc->dst.dip.ipv4) ||
            (ipmc->ipv6 && 
             memcmp(&dst->data.dip.ipv6, &ipmc->dst.dip.ipv6, sizeof(vtss_ipv6_t)) == 0)) {
            /* Found entry */
            break;
        }
    }
    

    if (dst == NULL) {
        VTSS_E("IPv%u DIP 0x%08x not found", 
               ipmc->ipv6 ? 6 : 4, vtss_cmn_ip2u32(&ipmc->dst.dip, ipmc->ipv6));
        return VTSS_RC_ERROR;
    }

    /* Move destination entry to free list */
    if (dst_prev == NULL)
        src->dest = dst->next;
    else 
        dst_prev->next = dst->next;
    dst->next = obj->dst_free;
    obj->dst_free = dst;
    obj->dst_count--;

    /* Free source, if it was the last destination */
    if (src->dest == NULL) {
        if (src_prev == NULL)
            *src_list = src->next;
        else
            src_prev->next = src->next;
        src->next = obj->src_free;
        obj->src_free = src;
        obj->src_count--;
        ipmc->src_del = 1;
    }
    ipmc->dst_del = 1;

    /* Delete resources */
    VTSS_FUNC_RC(ip_mc_update, ipmc, VTSS_IPMC_CMD_DEL);
    
    return VTSS_RC_OK;
}

static void vtss_cmn_ipv4_mc_data_init(vtss_ipmc_data_t *ipmc, 
                                       const vtss_vid_t vid,
                                       const vtss_ip_t  sip,
                                       const vtss_ip_t  dip)
{
    memset(ipmc, 0, sizeof(*ipmc));
    ipmc->src.vid = vid;
    ipmc->src.sip.ipv4 = sip;
    ipmc->dst.dip.ipv4 = dip;
    if (sip == 0) {
        /* Zero SIP indicates ASM */
        ipmc->src.fid = vid;
    } else {
        ipmc->src.ssm = 1;
    }
}

vtss_rc vtss_cmn_ipv4_mc_add(const vtss_vid_t vid,
                             const vtss_ip_t  sip,
                             const vtss_ip_t  dip,
                             const BOOL       member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_ipmc_data_t ipmc;
    vtss_port_no_t   port_no;

    vtss_cmn_ipv4_mc_data_init(&ipmc, vid, sip, dip);
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_PORT_BF_SET(ipmc.dst.member, port_no, member[port_no]);
    }
    return vtss_cmn_ip_mc_add(&ipmc);
}

vtss_rc vtss_cmn_ipv4_mc_del(const vtss_vid_t  vid,
                             const vtss_ip_t   sip,
                             const vtss_ip_t   dip)
{
    vtss_ipmc_data_t ipmc;
    
    vtss_cmn_ipv4_mc_data_init(&ipmc, vid, sip, dip);
    return vtss_cmn_ip_mc_del(&ipmc);
}

static void vtss_cmn_ipv6_mc_data_init(vtss_ipmc_data_t  *ipmc, 
                                       const vtss_vid_t  vid,
                                       const vtss_ipv6_t sip,
                                       const vtss_ipv6_t dip)
{
    vtss_ipv6_t sipv6;
    
    memset(ipmc, 0, sizeof(*ipmc));
    ipmc->ipv6 = 1;
    ipmc->src.vid = vid;
    ipmc->src.sip.ipv6 = sip;
    ipmc->dst.dip.ipv6 = dip;
    memset(&sipv6, 0, sizeof(sipv6));
    if (memcmp(&sip, &sipv6, sizeof(sipv6)) == 0) {
        /* Zero SIP indicates ASM */
        ipmc->src.fid = vid;
    } else {
        ipmc->src.ssm = 1;
    }
}

vtss_rc vtss_cmn_ipv6_mc_add(const vtss_vid_t  vid,
                             const vtss_ipv6_t sip,
                             const vtss_ipv6_t dip,
                             const BOOL        member[VTSS_PORT_ARRAY_SIZE])
{
    vtss_ipmc_data_t ipmc;
    vtss_port_no_t   port_no;

    vtss_cmn_ipv6_mc_data_init(&ipmc, vid, sip, dip);
    for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
        VTSS_PORT_BF_SET(ipmc.dst.member, port_no, member[port_no]);
    }
    return vtss_cmn_ip_mc_add(&ipmc);
}

vtss_rc vtss_cmn_ipv6_mc_del(const vtss_vid_t  vid,
                             const vtss_ipv6_t sip,
                             const vtss_ipv6_t dip)
{
    vtss_ipmc_data_t ipmc;

    vtss_cmn_ipv6_mc_data_init(&ipmc, vid, sip, dip);
    return vtss_cmn_ip_mc_del(&ipmc);
}

static void vtss_debug_print_ipv6_addr(const vtss_debug_printf_t pr, vtss_ipv6_t *ipv6)
{
    int i;

    for (i = 0; i < 16; i++) {
        pr("%02x%s", ipv6->addr[i], (i & 1) && i != 15 ? ":" : "");
    }
}

static void vtss_debug_print_ipmc(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    vtss_ipmc_info_t *ipmc = &vtss_state->ipmc;
    vtss_ipmc_src_t  *src;
    vtss_ipmc_dst_t  *dst;
    char             buf[80];
    u32              ipv6, src_free_count = 0, dst_free_count = 0;
    BOOL             header;

    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_IPMC))
        return;

    for (src = ipmc->obj.src_free; src != NULL; src = src->next) {
        src_free_count++;
    }
    for (dst = ipmc->obj.dst_free; dst != NULL; dst = dst->next) {
        dst_free_count++;
    }

    vtss_debug_print_value(pr, "State size", sizeof(*ipmc));
    vtss_debug_print_value(pr, "Source count", ipmc->obj.src_count);
    vtss_debug_print_value(pr, "Source free", src_free_count);
    vtss_debug_print_value(pr, "Source maximum", ipmc->obj.src_max);
    vtss_debug_print_value(pr, "Destination count", ipmc->obj.dst_count);
    vtss_debug_print_value(pr, "Destination free", dst_free_count);
    vtss_debug_print_value(pr, "Destination maximum", ipmc->obj.dst_max);
    pr("\n");

    /* SIP Table */
    for (ipv6 = 0; ipv6 < 2; ipv6++) {
        sprintf(buf, ipv6 ? "  %-40s" : "  %-11s", "DIP");
        for (src = ipmc->obj.src_used[ipv6]; src != NULL; src = src->next) {
            pr("%-6s%-6s%-6s%s\n", "Type", "VID", "FID", "SIP");
            pr("%-6s%-6u%-6u", src->data.ssm ? "SSM" : "ASM", src->data.vid, src->data.fid);
            if (ipv6) 
                vtss_debug_print_ipv6_addr(pr, &src->data.sip.ipv6);
            else
                pr("0x%08x", src->data.sip.ipv4);
            pr("\n\n");
            header = 1;
            for (dst = src->dest; dst != NULL; dst = dst->next) {
                if (header) {
                    vtss_debug_print_port_header(pr, buf, 0, 1);
                    header = 0;
                }
                pr("  ");
                if (ipv6) 
                    vtss_debug_print_ipv6_addr(pr, &dst->data.dip.ipv6);
                else
                    pr("0x%08x", dst->data.dip.ipv4);
                pr(" ");
                vtss_debug_print_ports(pr, dst->data.member, 1);
            }
            pr("\n");
        }
    }
    
#if defined(VTSS_FEATURE_IS1)
    if (vtss_state->arch == VTSS_ARCH_L26) {
        vtss_debug_print_vcap_is1(pr, info);
    }
#endif /* VTSS_FEATURE_IS1 */    

#if defined(VTSS_FEATURE_IS2)
    if (vtss_state->arch == VTSS_ARCH_JR1) {
        vtss_debug_print_vcap_is2(pr, info);
    }
#endif /* VTSS_FEATURE_IS2 */

    vtss_debug_print_mac_table(pr, info);
}
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */

#if defined(VTSS_FEATURE_VSTAX)
static void vtss_debug_print_stack(const vtss_debug_printf_t pr,
                                   const vtss_debug_info_t   *const info)
{
    vtss_vstax_conf_t      *conf;
    vtss_vstax_port_conf_t *port_conf;
    vtss_vstax_info_t      *vstax_info = &vtss_state->vstax_info;
    vtss_vstax_chip_info_t *chip_info;
    vtss_chip_no_t         chip_no;
    int                    i;
    char                   buf[64];

    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_STACK))
       return;
    
    conf = &vtss_state->vstax_conf;
    pr("UPSID_0: %u (%u)\n", conf->upsid_0, vstax_info->upsid[0]);
    pr("UPSID_1: %u (%u)\n", conf->upsid_1, vstax_info->upsid[1]);
    vtss_debug_print_port_none(pr, "Port_0 ", conf->port_0);
    vtss_debug_print_port_none(pr, "Port_1 ", conf->port_1);
#ifdef VTSS_FEATURE_VSTAX_V2
    pr("CMEF   : %s\n", vtss_bool_txt(!conf->cmef_disable));
#endif /* VTSS_FEATURE_VSTAX_V2 */
    pr("\n");
    
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        sprintf(buf, "Chip %u", chip_no);
        vtss_debug_print_header_underlined(pr, buf, 0);

        chip_info = &vstax_info->chip_info[chip_no];
        vtss_debug_print_reg_header(pr, "Port Masks"); 
        for (i = 0; i < 2; i++) {
            sprintf(buf, "Mask %s", i ? "B" : "A");
            vtss_debug_print_reg(pr, buf, i ? chip_info->mask_b : chip_info->mask_a);
        }
        pr("\nPort  TTL  Mirror\n"); 
        for (i = 0; i < 2; i++) {
            port_conf = &chip_info->port_conf[i];
            pr("%-6s%-5u%u\n", i ? "B" : "A", port_conf->ttl, port_conf->mirror);
        }
        pr("\n");
#if defined(VTSS_FEATURE_VSTAX_V2)
        {
            vtss_vstax_route_entry_t *rt;
            vtss_vstax_upsid_t       upsid;
            
            pr("UPSID  A  B\n");
            for (upsid = VTSS_VSTAX_UPSID_MIN; upsid <= VTSS_VSTAX_UPSID_MAX; upsid++) {
                rt = &chip_info->rt_table.table[upsid];
                pr("%-7u%-3u%u\n", upsid, rt->stack_port_a, rt->stack_port_b);
            }     
            pr("\n");
        }
#endif  /* VTSS_FEATURE_VSTAX_V2 */
    }
    pr("\n");
}
#endif /* VTSS_FEATURE_VSTAX */

#if defined(VTSS_FEATURE_PACKET)
static void vtss_debug_print_packet(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    u32                   i;
    char                  buf[16];
    vtss_packet_rx_conf_t *conf = &vtss_state->rx_conf;
    
    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_PACKET))
        return;

    vtss_debug_print_header(pr, "Registrations");
#if defined(VTSS_FEATURE_PACKET_PORT_REG)
    {
        vtss_port_no_t             port_no;
        vtss_packet_rx_port_conf_t *port_conf;
        vtss_packet_reg_type_t     type;
        BOOL                       header = 1;
        
        for (port_no = VTSS_PORT_NO_START; port_no < vtss_state->port_count; port_no++) {
            if (!info->port_list[port_no])
                continue;
            if (header) {
                header = 0;
                pr("Port  BPDU              GARP\n");
            }
            pr("%-4u", port_no);
            port_conf = &vtss_state->rx_port_conf[port_no];
            for (i = 0; i < 32; i++) {
                type = (i < 16 ? port_conf->bpdu_reg[i] : port_conf->garp_reg[i - 16]);
                pr("%s%s",
                   i % 16  ? "" : "  ", 
                   type == VTSS_PACKET_REG_NORMAL ? "N" :
                   type == VTSS_PACKET_REG_FORWARD ? "F" :
#if defined(VTSS_ARCH_JAGUAR_1)
                   type == VTSS_PACKET_REG_DISCARD ? "D" :
                   type == VTSS_PACKET_REG_CPU_COPY ? "C" :
#endif /* VTSS_ARCH_JAGUAR_1 */
                   type == VTSS_PACKET_REG_CPU_ONLY ? "R" : "?");
            }
            pr("\n");
        }
        if (!header)
            pr("\n");
    }
#endif /* VTSS_FEATURE_PACKET_PORT_REG */

    vtss_debug_print_value(pr, "BPDU", conf->reg.bpdu_cpu_only);
    for (i = 0; i < 16; i++) {
        sprintf(buf, "GARP_%u", i);
        vtss_debug_print_value(pr, buf, conf->reg.garp_cpu_only[i]);
    }
    vtss_debug_print_value(pr, "IPMC", conf->reg.ipmc_ctrl_cpu_copy);
    vtss_debug_print_value(pr, "IGMP", conf->reg.igmp_cpu_only);
    vtss_debug_print_value(pr, "MLD", conf->reg.mld_cpu_only);
    pr("\n");
    
    vtss_debug_print_header(pr, "Queue Mappings");
    vtss_debug_print_value(pr, "BPDU", conf->map.bpdu_queue);
    vtss_debug_print_value(pr, "GARP", conf->map.garp_queue);
    vtss_debug_print_value(pr, "LEARN", conf->map.learn_queue);
    vtss_debug_print_value(pr, "IGMP", conf->map.igmp_queue);
    vtss_debug_print_value(pr, "IPMC", conf->map.ipmc_ctrl_queue);
    vtss_debug_print_value(pr, "MAC_VID", conf->map.mac_vid_queue);
    pr("\n");

#if defined(VTSS_FEATURE_NPI)
    vtss_debug_print_header(pr, "NPI");
    vtss_debug_print_value(pr, "Enabled", vtss_state->npi_conf.enable);
    if (vtss_state->npi_conf.enable) {
        vtss_packet_rx_conf_t *rx_conf = &vtss_state->rx_conf;

        vtss_debug_print_value(pr, "NPI_PORT", vtss_state->npi_conf.port_no);
        for (i = 0; i < vtss_state->rx_queue_count; i++) {
            sprintf(buf, "REDIR:CPUQ_%u", i);
            vtss_debug_print_value(pr, buf, rx_conf->queue[i].npi.enable);
        }
    }
    pr("\n");
#endif
}
#endif /* VTSS_FEATURE_PACKET */

#if defined(VTSS_FEATURE_TIMESTAMP)

static const char *one_pps_mode_disp(vtss_ts_ext_clock_one_pps_mode_t m)
{
    switch (m) {
        case TS_EXT_CLOCK_MODE_ONE_PPS_DISABLE: return "Disable";
        case TS_EXT_CLOCK_MODE_ONE_PPS_OUTPUT: return "Output";
        case TS_EXT_CLOCK_MODE_ONE_PPS_INPUT: return "Input";
        default: return "unknown";
    }
}

static void vtss_debug_print_ts(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    u32               i,j;
    vtss_ts_conf_t *ts_conf;
    vtss_ts_port_conf_t *ts_port_conf;
    BOOL first = TRUE;

    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_TS))
        return;

    ts_conf = &vtss_state->ts_conf;

    pr("One-Second Timer:\n");
    pr("Adjustment rate: %d ppb, \nOne_pps mode: %s ExternalClockOut mode: %s freq %d Hz\nClock offset %d sec\n",
                ts_conf->adj,

                one_pps_mode_disp(ts_conf->ext_clock_mode.one_pps_mode),
                ts_conf->ext_clock_mode.enable ? "enable" : "disable",
                ts_conf->ext_clock_mode.freq,
                ts_conf->sec_offset);

    pr("Port timestamp parameters:\n");
    pr("Port  IngressLatency  PeerDelay  EgressLatency  OperationMode\n");
    for (i = 0; i < VTSS_PORT_ARRAY_SIZE; i++) {
        ts_port_conf = &vtss_state->ts_port_conf[i];
        pr("%-4d  %-14d  %-9d  %-13d  %-d\n",
           i,
           VTSS_INTERVAL_NS(ts_port_conf->ingress_latency),
           VTSS_INTERVAL_NS(ts_port_conf->p2p_delay),
           VTSS_INTERVAL_NS(ts_port_conf->egress_latency),
           ts_port_conf->mode.mode);
    }
    
    
    (void)VTSS_FUNC(ts_timestamp_get);
    pr("Timestamp fifo data:\n");
    
    for (i = 0; i < VTSS_TS_ID_SIZE; i++) {
        if (vtss_state->ts_status[i].reserved_mask != 0) {
            pr("Timestamp_id : %d  Reserved_mask: %llx, age %d\n", i, vtss_state->ts_status[i].reserved_mask, vtss_state->ts_status[i].age);
        }
        if (vtss_state->ts_status[i].valid_mask != 0) {
            pr("                    Valid_mask: %llx\n", vtss_state->ts_status[i].valid_mask);
        }
        first = TRUE;
        for (j = 0; j < VTSS_PORT_ARRAY_SIZE; j++) {
            if (vtss_state->ts_status[i].valid_mask & 1LL<<j) {
                if (first) {
                    pr("Tx Port  time counter  time id\n");
                    first = FALSE;
                }
                pr("%-9d  %-14d %-14d \n", j, vtss_state->ts_status[i].tx_tc[j], 
                   vtss_state->ts_status[i].tx_id[j]);
            }
        }
        if (vtss_state->ts_status[i].rx_tc_valid) {
            pr("Rx Timestamp_id : %d  Rx timecounter: %d, \n", i, vtss_state->ts_status[i].rx_tc);
        }
    }
    pr("\n");
#if defined(VTSS_ARCH_SERVAL_CE)
    pr("OAM Timestamp fifo data:\n");
    
    for (i = 0; i < VTSS_VOE_ID_SIZE; i++) {
        first = TRUE;
        for (j = 0; j < VTSS_SERVAL_MAX_OAM_ENTRIES; j++) {
            if (vtss_state->oam_ts_status[i].entry[j].valid) {
                if (first) {
                    pr("VOE ID : %d  last: %u  entry: %u\n", i, vtss_state->oam_ts_status[i].last, j);
                    pr("Port  time counter  time id  sequence\n");
                    first = FALSE;
                }
                pr("%-4u  %-12u  %-7u  %-8u\n", vtss_state->oam_ts_status[i].entry[j].port, 
                   vtss_state->oam_ts_status[i].entry[j].tc, 
                   vtss_state->oam_ts_status[i].entry[j].id,
                   vtss_state->oam_ts_status[i].entry[j].sq);
            }
        }
    }
    pr("\n");
#endif /*VTSS_ARCH_SERVAL */

}
#endif /* VTSS_FEATURE_TIMESTAMP */

#if defined(VTSS_SW_OPTION_L3RT) || defined(VTSS_ARCH_JAGUAR_1)
/* implemented in vtss_l3.c */
void vtss_debug_print_l3(const vtss_debug_printf_t pr,
                         const vtss_debug_info_t   *const info);
#endif /* VTSS_SW_OPTION_L3RT || VTSS_ARCH_JAGUAR_1*/

#if defined(VTSS_FEATURE_OAM)
#define YN(x) ((x) ? "Yes" : "No ")

static void vtss_debug_print_mac(const vtss_debug_printf_t pr,
                                        const vtss_mac_t   *m)
{
     pr("%02X-%02X-%02X-%02X-%02X-%02X",
        m->addr[0], m->addr[1], m->addr[2], m->addr[3], m->addr[4], m->addr[5]);
}

static void vtss_debug_print_oam_extract(const vtss_debug_printf_t         pr,
                                         const vtss_oam_vop_extract_conf_t c)
{
    pr("%-5s %4u", (c.to_front ? "Front" : "CPU"), c.rx_queue);
}

static void vtss_debug_print_oam(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    u32 i, n;
    const vtss_oam_vop_conf_t *vop = &vtss_state->oam_vop_conf;
    const vtss_oam_voe_conf_t *voe;
    static const char *mep_type[3] = { "Down", "Up  ", "MIP " };
    static const char *ccm_tx_seq_type[3] = { "No     ", "Inc+Upd", "Update " };
    static const char *period[4] = { "3.3ms", "10ms ", "100ms", "1sec " };
    static const char *dmac_check[4] = { "Unicast   ", "Multicast ",
                                         "Uni+Multi ", "None      " };

    if (!vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_OAM))
        return;

    // VOP

    pr("VOP:\n\n");

    pr("Enable_VOEs:%s CCM-LM_propr:%s LMR_propr:%s MC-DMAC:",
        YN(vop->enable_all_voe),
        YN(vop->ccm_lm_enable_rx_fcf_in_reserved_field),
        YN(vop->down_mep_lmr_proprietary_fcf_use));
    vtss_debug_print_mac(pr, &vop->common_multicast_dmac);
    pr(" S-DLB_idx:%u Ext_CPU_portmask:0x%08x\n",
        vop->sdlb_cpy_copy_idx,
        vop->external_cpu_portmask);

    pr("PDU extract:\n");
    pr("Type      Dest. Idx. Opcode DMAC_check\n");
    for (i=0; i<VTSS_OAM_GENERIC_OPCODE_CFG_CNT; ++i) {
        pr("Generic_%u ", i);
        vtss_debug_print_oam_extract(pr, vop->pdu_type.generic[i].extract);
        pr(" %6u %s\n", vop->pdu_type.generic[i].opcode, YN(vop->pdu_type.generic[i].check_dmac));
    }
    pr("CCM       "); vtss_debug_print_oam_extract(pr, vop->pdu_type.ccm);    pr("\n");
    pr("CCM-LM    "); vtss_debug_print_oam_extract(pr, vop->pdu_type.ccm_lm); pr("\n");
    pr("LT        "); vtss_debug_print_oam_extract(pr, vop->pdu_type.lt);     pr("\n");
    pr("DMM       "); vtss_debug_print_oam_extract(pr, vop->pdu_type.dmm);    pr("\n");
    pr("DMR       "); vtss_debug_print_oam_extract(pr, vop->pdu_type.dmr);    pr("\n");
    pr("LMM       "); vtss_debug_print_oam_extract(pr, vop->pdu_type.lmm);    pr("\n");
    pr("LMR       "); vtss_debug_print_oam_extract(pr, vop->pdu_type.lmr);    pr("\n");
    pr("LBM       "); vtss_debug_print_oam_extract(pr, vop->pdu_type.lbm);    pr("\n");
    pr("LBR       "); vtss_debug_print_oam_extract(pr, vop->pdu_type.lbr);    pr("\n");
    pr("Errored   "); vtss_debug_print_oam_extract(pr, vop->pdu_type.err);    pr("\n");
    pr("Other     "); vtss_debug_print_oam_extract(pr, vop->pdu_type.other);  pr("\n");
    pr("\nVOEs:\n\n");

    for (i=0; i<VTSS_OAM_VOE_CNT; ++i) {
        voe = &vtss_state->oam_voe_conf[i];
        if (info->full  ||  vtss_state->oam_voe_internal[i].allocated) {
            pr("VOE:%4u Enabled:%s Type:%s MEP-type:%s UC-MAC:",
                i,
                YN(vtss_state->oam_voe_internal[i].allocated),
                (voe->voe_type == VTSS_OAM_VOE_SERVICE ? "Service" : "Port"),
                mep_type[voe->mep_type]);
            vtss_debug_print_mac(pr, &voe->unicast_mac);
            pr("\n");

            pr("         Cnt_in_path_MEP:%s W:%-4u P:%-4u "
                "Loop:W:%-4u P:%-4u PPort:%-2u S-DLB:%s S-DLB_Idx:%-4u\n",
                YN(voe->svc_to_path), voe->svc_to_path_idx_w,
                voe->svc_to_path_idx_p,
                voe->loop_isdx_w, voe->loop_isdx_p, voe->loop_portidx_p,
                YN(voe->sdlb_enable), voe->sdlb_idx);

            for (n=0; n<VTSS_OAM_GENERIC_OPCODE_CFG_CNT; ++n) {
                if (info->full  ||  voe->generic[n].enable) {
                    pr("         Generic:%u Opcode:%-3u Enable:%s "
                        "Copy_CPU:%s Fwd:%s CntSel:%s CntData:%s\n",
                        n, vop->pdu_type.generic[n].opcode,
                        YN(voe->generic[n].enable),
                        YN(voe->generic[n].copy_to_cpu),
                        YN(voe->generic[n].forward),
                        YN(voe->generic[n].count_as_selected),
                        YN(voe->generic[n].count_as_data));
                }
            }
            if (info->full  ||  voe->ccm.enable) {
                pr("         CCM:    Enable:%s Copy_CPU:%s Fwd:%s "
                    "CntSel:%s CntData:%s MEPid:%-5u\n"
                    "                 MEGid:",
                    YN(voe->ccm.enable),
                    YN(voe->ccm.copy_to_cpu),
                    YN(voe->ccm.forward),
                    YN(voe->ccm.count_as_selected),
                    YN(voe->ccm.count_as_data),
                    voe->ccm.mepid                  
                    );
                for (n=0; n<48; ++n) {
                    if (n && n % 12 == 0)
                        pr("\n                       ");
                    if (voe->ccm.megid.data[n] >= 32)
                        pr("%c", voe->ccm.megid.data[n]);
                    else
                        pr("\\x%02u", voe->ccm.megid.data[n]);
                }
                pr("\n");
                pr("                 AutoTXSeq:%s TxSeq:%-5u CheckRxSeq:%s "
                    "RxSeq:%-5u "
                    "RxPri:%1u RxPeriod:%s\n",
                    ccm_tx_seq_type[voe->ccm.tx_seq_no_auto_upd_op],
                    voe->ccm.tx_seq_no,
                    YN(voe->ccm.rx_seq_no_check),
                    voe->ccm.rx_seq_no,
                    voe->ccm.rx_priority,
                    period[voe->ccm.rx_period-1]);
            }
            if (info->full  ||  voe->ccm_lm.enable) {
                pr("         CCM-LM: Enable:%s Copy_CPU:%s Fwd:%s "
                   "CntSel:%s PriMask:0x%02x Yellow:%s Period:%s\n",
                   YN(voe->ccm_lm.enable),
                   YN(voe->ccm_lm.copy_to_cpu),
                   YN(voe->ccm_lm.forward),
                   YN(voe->ccm_lm.count_as_selected),
                   voe->ccm_lm.count.priority_mask,
                   YN(voe->ccm_lm.count.yellow),
                   period[voe->ccm_lm.period-1]);
            }
            if (info->full  ||  voe->single_ended_lm.enable) {
                pr("         SE-LM:  Enable:%s Copy_LMM_CPU:%s Copy_LMR_CPU:%s"
                    " Fwd_LMM:%s Fwd_LMR:%s CntSel:%s CntData:%s PriMask:0x%02x"
                    " Yellow:%s\n",
                    YN(voe->single_ended_lm.enable),
                    YN(voe->single_ended_lm.copy_lmm_to_cpu),
                    YN(voe->single_ended_lm.copy_lmr_to_cpu),
                    YN(voe->single_ended_lm.forward_lmm),
                    YN(voe->single_ended_lm.forward_lmr),
                    YN(voe->single_ended_lm.count_as_selected),
                    YN(voe->single_ended_lm.count_as_data),
                    voe->single_ended_lm.count.priority_mask,
                    YN(voe->single_ended_lm.count.yellow));
            }
            if (info->full  ||  voe->lt.enable) {
                pr("         LT:     Enable:%s Copy_LTM_CPU:%s Copy_LTR_CPU:%s"
                    " Fwd_LTM:%s Fwd_LTR:%s CntSel:%s CntData:%s\n",
                    YN(voe->lt.enable),
                    YN(voe->lt.copy_ltm_to_cpu),
                    YN(voe->lt.copy_ltr_to_cpu),
                    YN(voe->lt.forward_ltm),
                    YN(voe->lt.forward_ltr),
                    YN(voe->lt.count_as_selected),
                    YN(voe->lt.count_as_data));
            }
            if (info->full  ||  voe->lb.enable) {
                pr("         LB:     Enable:%s Copy_LBM_CPU:%s Copy_LBR_CPU:%s"
                    " Fwd_LBM:%s Fwd_LBR:%s CntSel:%s CntData:%s\n"
                    "                 Upd_TX_transid:%s TX_transid:0x%08x RX_transid:0x%08x\n",
                    YN(voe->lb.enable),
                    YN(voe->lb.copy_lbm_to_cpu),
                    YN(voe->lb.copy_lbr_to_cpu),
                    YN(voe->lb.forward_lbm),
                    YN(voe->lb.forward_lbr),
                    YN(voe->lb.count_as_selected),
                    YN(voe->lb.count_as_data),
                    YN(voe->lb.tx_update_transaction_id),
                    voe->lb.tx_transaction_id,
                    voe->lb.rx_transaction_id);
            }
            if (info->full  ||  voe->tst.enable) {
                pr("         TST:    Enable:%s Copy_CPU:%s"
                    " Fwd:%s CntSel:%s CntData:%s "
                    " Upd_TX_seq:%s TX_seq:0x%08x RX_seq:0x%08x\n",
                    YN(voe->tst.enable),
                    YN(voe->tst.copy_to_cpu),
                    YN(voe->tst.forward),
                    YN(voe->tst.count_as_selected),
                    YN(voe->tst.count_as_data),
                    YN(voe->tst.tx_seq_no_auto_update),
                    voe->tst.tx_seq_no,
                    voe->tst.rx_seq_no);
            }
            if (info->full  ||  voe->dm.enable_dmm  ||  voe->dm.enable_1dm) {
                pr("         DM:     CntSel:%s CntData:%s\n",
                    YN(voe->dm.count_as_selected),
                    YN(voe->dm.count_as_data));
                pr("         1DM:    Enable:%s Copy_CPU:%s Fwd:%s\n",
                    YN(voe->dm.enable_1dm),
                    YN(voe->dm.copy_1dm_to_cpu),
                    YN(voe->dm.forward_1dm));
                pr("         DMM:    Enable:%s Copy_DMM_CPU:%s Copy_DMR_CPU:%s "
                    "Fwd_DMM:%s Fwd_DMR:%s\n",
                    YN(voe->dm.enable_dmm),
                    YN(voe->dm.copy_dmm_to_cpu),
                    YN(voe->dm.copy_dmr_to_cpu),
                    YN(voe->dm.forward_dmm),
                    YN(voe->dm.forward_dmr));
            }
            pr("         Proc:   MEL:%u DMAC_chk:%s Chk_CCM_only:%s "
                "Cp_Nxt_Only:%s Cp_CCM_err:%s Cp_CCM_TLV:%s\n"
                "                 Cp_MEL_low:%s Cp_DMAC_err:%s\n",
                voe->proc.meg_level,
                dmac_check[voe->proc.dmac_check_type],
                YN(voe->proc.ccm_check_only),
                YN(voe->proc.copy_next_only),
                YN(voe->proc.copy_on_ccm_err),
                YN(voe->proc.copy_on_ccm_more_than_one_tlv),
                YN(voe->proc.copy_on_mel_too_low_err),
                YN(voe->proc.copy_on_dmac_err));
            if (info->full  ||  voe->mep_type == VTSS_OAM_UPMEP) {
                pr("         Up-MEP: Discard_RX:%s Loopback:%s Port:%-u\n",
                    YN(voe->upmep.discard_rx),
                    YN(voe->upmep.loopback),
                    voe->upmep.port);
            }
        }
    }
}
#undef YN
#endif /* VTSS_FEATURE_OAM */

#if defined(VTSS_FEATURE_MPLS)

static void vtss_debug_print_mpls(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_MPLS)) {
        pr("(Placeholder for MPLS debug output)\n");
    }
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_MPLS_OAM)) {
        pr("(Placeholder for MPLS-OAM debug output)\n");
    }
}

#endif /* VTSS_FEATURE_MPLS */

#if defined(VTSS_FEATURE_AFI_SWC)
static void vtss_debug_print_afi(const vtss_debug_printf_t pr,
                                 const vtss_debug_info_t   *const info)
{
    u32 slot, cnt = 0;

    // Print the DCB layout of the frames that are currently being injected.
    pr("Automatic Frame Injector\n");
    pr("ID   State    Frames/sec Port\n");
    pr("---- -------- ---------- ----\n");

    for (slot = 0; slot < VTSS_ARRSZ(vtss_state->afi_slots); slot++) {
        vtss_afi_slot_conf_t  *slot_conf = &vtss_state->afi_slots[slot];
        vtss_afi_timer_conf_t *timer_conf;

        if (slot_conf->state == VTSS_AFI_SLOT_STATE_FREE) {
            continue;
        }

        cnt++;
        timer_conf = &vtss_state->afi_timers[slot_conf->timer_idx];

        pr("%4u %-8s %10u %4u\n",
           slot,
           slot_conf->state == VTSS_AFI_SLOT_STATE_RESERVED ? "Reserved" :
           slot_conf->state == VTSS_AFI_SLOT_STATE_ENABLED  ? "Enabled"  :
           slot_conf->state == VTSS_AFI_SLOT_STATE_PAUSED   ? "Paused"   : "Unknown",
           timer_conf->fps,
           slot_conf->port_no);
    }

    if (cnt == 0) {
        pr("<none>\n");
    }
    pr("\n");
}
#endif

/* Print Application Interface Layer state */
static vtss_rc vtss_debug_ail_print(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    if (info->layer != VTSS_DEBUG_LAYER_ALL && info->layer != VTSS_DEBUG_LAYER_AIL)
        return VTSS_RC_OK;

    vtss_debug_print_header_underlined(pr, "Application Interface Layer", 1);

    vtss_debug_print_init(pr, info);

#if defined(VTSS_FEATURE_MISC)
    vtss_debug_print_misc(pr, info);
#endif /* VTSS_FEATURE_MISC */
#if defined(VTSS_CHIP_CU_PHY)
    VTSS_RC(vtss_phy_debug_info_print(pr, info, 1));
#endif

#if defined(VTSS_CHIP_10G_PHY)
    VTSS_RC(vtss_phy_10g_debug_info_print(pr, info, 1));
#endif /* VTSS_CHIP_10G_PHY */

#if defined(VTSS_FEATURE_PORT_CONTROL)
    vtss_debug_print_port(pr, info);
    vtss_debug_print_port_counters(pr, info);
#endif /* VTSS_FEATURE_PORT_CONTROL */

#if defined(VTSS_FEATURE_LAYER2)
    vtss_debug_print_layer2(pr, info);
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_ACL)
    vtss_debug_print_acl(pr, info);
#endif /* VTSS_FEATURE_ACL */

#if defined(VTSS_FEATURE_QOS)
    vtss_debug_print_qos(pr, info);
#endif /* VTSS_FEATURE_QOS */

#if defined(VTSS_FEATURE_IPV4_MC_SIP) || defined(VTSS_FEATURE_IPV6_MC_SIP)
    vtss_debug_print_ipmc(pr, info);
#endif /* VTSS_FEATURE_IPV4_MC_SIP || VTSS_FEATURE_IPV6_MC_SIP */

#if defined(VTSS_FEATURE_VSTAX)
    vtss_debug_print_stack(pr, info);
#endif /* VTSS_FEATURE_VSTAX */

#if defined(VTSS_FEATURE_PACKET)
    vtss_debug_print_packet(pr, info);
#endif /* VTSS_FEATURE_PACKET */

#if defined(VTSS_FEATURE_TIMESTAMP)
    vtss_debug_print_ts(pr, info);
#endif /* VTSS_FEATURE_LAYER2 */

#if defined(VTSS_FEATURE_SERIAL_GPIO)
    vtss_debug_print_ser_gpio(pr, info);
#endif

#if defined(VTSS_FEATURE_OAM)
    vtss_debug_print_oam(pr, info);
#endif
    
#if defined(VTSS_SW_OPTION_L3RT) || defined(VTSS_ARCH_JAGUAR_1)
    if( vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_L3) )
        vtss_debug_print_l3(pr, info);
#endif /* VTSS_SW_OPTION_L3RT || VTSS_ARCH_JAGUAR_1*/

#if defined(VTSS_FEATURE_FDMA) && VTSS_OPT_FDMA
    if (vtss_debug_group_enabled(pr, info, VTSS_DEBUG_GROUP_FDMA)) {
        VTSS_RC(vtss_fdma_cmn_debug_print_fdma(vtss_state, pr, info));
    }
#endif

#if defined(VTSS_FEATURE_MPLS)
    vtss_debug_print_mpls(pr, info);
#endif /* VTSS_FEATURE_MPLS */

#if defined(VTSS_FEATURE_AFI_SWC)
    vtss_debug_print_afi(pr, info);
#endif

    return VTSS_RC_OK;
}

/* Print Chip Interface Layer state */
static vtss_rc vtss_debug_cil_print(const vtss_debug_printf_t pr,
                                    const vtss_debug_info_t   *const info)
{
    vtss_rc        rc = VTSS_RC_OK;
    char           buf[80];
    vtss_chip_no_t chip_no;

    if (info->layer != VTSS_DEBUG_LAYER_ALL && info->layer != VTSS_DEBUG_LAYER_CIL)
        return VTSS_RC_OK;

    /* Print CIL information for all devices */
    for (chip_no = 0; chip_no < vtss_state->chip_count; chip_no++) {
        if (info->chip_no != VTSS_CHIP_NO_ALL && chip_no != info->chip_no)
            continue;
        VTSS_SELECT_CHIP(chip_no);
        sprintf(buf, "Chip Interface Layer[%u]", chip_no);
        vtss_debug_print_header_underlined(pr, buf, 1);
#if defined(VTSS_CHIP_CU_PHY)
        VTSS_RC(vtss_phy_debug_info_print(pr, info, 0));
#endif
        rc = VTSS_FUNC(debug_info_print, pr, info);
    }
    return rc;
}

vtss_rc vtss_cmn_debug_info_print(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info)
{
    VTSS_RC(vtss_debug_ail_print(pr, info));
    return vtss_debug_cil_print(pr, info);
}

/* This function snoops the last X calls to reg_read()/reg_write()
 * in a circular buffer. It also checks whether an access to the switch
 * core fails, and if so, prints an error message.
 */
static vtss_rc vtss_cmn_reg_check_access(const vtss_chip_no_t chip_no,
                                         const u32            addr,
                                         u32                  *const value,
                                         BOOL                 is_read)
{
    // Only default instance works.
    vtss_state_t            *state = (vtss_state_t *)vtss_default_inst;
    u32                     new_err_cnt;
    // We need to lock the scheduler here so that the accesses to the user address and
    // to the error counter address are indivisible.
    /*lint --e{529} */ // Avoid "Symbol 'flags' not subsequently referenced" Lint warning
    VTSS_OS_SCHEDULER_FLAGS flags = 0;
    vtss_rc rc;

    VTSS_OS_SCHEDULER_LOCK(flags);

    if (is_read) {
        rc = state->reg_check.orig_read(chip_no, addr, value);
    } else {
        rc = state->reg_check.orig_write(chip_no, addr, *value);
    }

    if (chip_no != 0 || rc != VTSS_RC_OK) {
        // We only support the register access checking on chip #0,
        // and only if the user-access went OK.
        goto do_exit;
    }

    // Save the address and access type of the register that is attempted accessed in a circular buffer.
    state->reg_check.last_accesses[state->reg_check.last_access_idx  ].addr    = addr;
    state->reg_check.last_accesses[state->reg_check.last_access_idx++].is_read = is_read;
    if (state->reg_check.last_access_idx >= VTSS_ARRSZ(state->reg_check.last_accesses)) {
        state->reg_check.last_access_idx = 0;
    }

    if (state->reg_check.orig_read(chip_no, state->reg_check.addr, &new_err_cnt) != VTSS_RC_OK) {
        VTSS_EG(VTSS_TRACE_GROUP_REG_CHECK, "Unable to read error counter address");
        goto do_exit; // Don't modify rc.
    }

    if (new_err_cnt != state->reg_check.last_error_cnt) {
        u32 i, cnt = 0;

        // Use VTSS_EG(), which may be called in both ISR, DSR; and thread context.
        VTSS_EG(VTSS_TRACE_GROUP_REG_CHECK, "Error when %sing 0x%06x (err_cnt = 0x%x):\n  ", is_read ? "read" : "writ", 4U * addr, new_err_cnt);
        for (i = state->reg_check.last_access_idx; cnt != VTSS_ARRSZ(state->reg_check.last_accesses); i++, cnt++) {
            if (i >= VTSS_ARRSZ(state->reg_check.last_accesses)) {
                i = 0;
            }
            if (state->reg_check.last_accesses[i].addr != 0) {
                VTSS_EG(VTSS_TRACE_GROUP_REG_CHECK, "[%2u] = %c(0x%06x) ", cnt, state->reg_check.last_accesses[i].is_read ? 'R' : 'W', 4U * state->reg_check.last_accesses[i].addr);
                state->reg_check.last_accesses[i].addr = 0;
            }
        }
        state->reg_check.last_error_cnt = new_err_cnt;
    }

do_exit:
    VTSS_OS_SCHEDULER_UNLOCK(flags);
    return rc;
}

static vtss_rc vtss_cmn_debug_reg_check_read(const vtss_chip_no_t chip_no,
                                             const u32            addr,
                                                   u32            *const value)
{
    return vtss_cmn_reg_check_access(chip_no, addr, value, TRUE);
}

static vtss_rc vtss_cmn_debug_reg_check_write(const vtss_chip_no_t chip_no,
                                              const u32            addr,
                                              const u32            value)
{
    u32 non_const_value = (u32)value;
    return vtss_cmn_reg_check_access(chip_no, addr, &non_const_value, FALSE);
}

vtss_rc vtss_cmn_debug_reg_check_set(vtss_state_t *state, BOOL enable)
{
    // We need to lock scheduler here when installing new reg_read/write() handlers.
    /*lint --e{529, 438} */ // Avoid "Symbol 'flags' not subsequently referenced" Lint warning
    VTSS_OS_SCHEDULER_FLAGS flags = 0;

    if (state->reg_check.addr == 0) {
        // The CIL layer says: Not supported on this platform.
        VTSS_IG(VTSS_TRACE_GROUP_REG_CHECK, "Register checking is not support on this platform");
        return VTSS_RC_ERROR;
    }

    VTSS_OS_SCHEDULER_LOCK(flags);

    if (enable) {
        if (state->reg_check.disable_cnt > 0) {
            if (--state->reg_check.disable_cnt == 0) {
                state->reg_check.orig_read  = state->init_conf.reg_read;
                state->reg_check.orig_write = state->init_conf.reg_write;
                state->init_conf.reg_read   = vtss_cmn_debug_reg_check_read;
                state->init_conf.reg_write  = vtss_cmn_debug_reg_check_write;
                // Update the error counter.
                (void)state->reg_check.orig_read(0, state->reg_check.addr, &state->reg_check.last_error_cnt);
            }
        }
    } else if (state->reg_check.disable_cnt++ == 0) {
        // Supported on this platform. Restore original handlers.
        state->init_conf.reg_read   = state->reg_check.orig_read;
        state->init_conf.reg_write  = state->reg_check.orig_write;
        state->reg_check.orig_read  = NULL;
        state->reg_check.orig_write = NULL;
    }

    VTSS_OS_SCHEDULER_UNLOCK(flags);
    return VTSS_RC_OK;
}

vtss_rc vtss_cmn_bit_from_one_hot_mask64(u64 mask, u32 *bit_pos)
{
    u32 msw = (u32)(mask >> 32), lsw = (u32)mask;

    // Exactly one bit must be set in #mask.
    if ((msw == 0 && lsw == 0) || (msw != 0 && lsw != 0)) {
        // Either both are 0 or both are non-zero, hence can't be a one-hot.
        goto err;
    }

    if (msw == 0) {
        *bit_pos = VTSS_OS_CTZ(lsw);
        lsw &= ~VTSS_BIT(*bit_pos);
        if (lsw) {
            // Two or more bits set in lsw
            goto err;
        }
    } else {
        *bit_pos = VTSS_OS_CTZ(msw);
        msw &= ~VTSS_BIT(*bit_pos);
        if (msw) {
            // Two or more bits set in msw
            goto err;
        }
        *bit_pos += 32;
    }
    return VTSS_RC_OK;

err:
    VTSS_E("0x%llx is not a one-hot mask", mask);
    return VTSS_RC_ERROR;
}

#if defined(VTSS_FEATURE_PACKET)
/*
 * Convert a 64-bit mask consisting of logical port numbers to another 64-bit
 * mask consisting of physical port numbers.
 * The function fails if no bits are set, if a port is not in the port map,
 * if two ports are located on two different physical chips, or if more than
 * bit is set and a stack port is among them.
 *
 * On successful exit:
 *   #chip_port_mask is set to the physical port mask corresponding to the logical port mask we were called with.
 *   #chip_no        is set to the chip number for which #chip_port_mask applies.
 *   #stack_port_no  is set to the logical stack port number if #logical_port_mask points out a stack port, VTSS_PORT_NO_NONE if not.
 *   #port_cnt       is set to the number of bits set in #logical_port_mask.
 *   #port_no        is set to the last logical port number investigated in #logical_port_mask. Usually only useful when #port_cnt is exactly 1.
 */
vtss_rc vtss_cmn_logical_to_chip_port_mask(const vtss_state_t *const state,
                                                 u64                 logical_port_mask,
                                                 u64                 *chip_port_mask,
                                                 vtss_chip_no_t      *chip_no,
                                                 vtss_port_no_t      *stack_port_no,
                                                 u32                 *port_cnt,
                                                 vtss_port_no_t      *port_no)
{
    u32 i, w, p;

    /* Lint cannot see that state->port_count < VTSS_PORT_ARRAY_SIZE, so it thinks
     * that port_map[p] can happen to exceed the array size.
     */
    /*lint --e{506, 550, 661, 662} */

    if (!logical_port_mask) {
       VTSS_E("Empty port mask");
       return VTSS_RC_ERROR;
    }

    *chip_port_mask = 0;
    *chip_no        = 0xFFFFFFFFUL;
    *stack_port_no  = VTSS_PORT_NO_NONE;
    *port_cnt       = 0;

    for (i = 0; i < 64; i += 32) {
        w = (u32)(logical_port_mask >> i);

        while ((p = VTSS_OS_CTZ(w)) < 32) {
            w &= ~VTSS_BIT(p);
            p += i;

            if (p >= state->port_count) {
                VTSS_E("port = %u out of range (mask = 0x%llx)", p, logical_port_mask);
                return VTSS_RC_ERROR;
            }

            if (*chip_no == 0xFFFFFFFFUL) {
                *chip_no = state->port_map[p].chip_no;
            } else if (*chip_no != state->port_map[p].chip_no) {
                VTSS_E("Maps to two different devices (mask = 0x%llx)", logical_port_mask);
                return VTSS_RC_ERROR;
            }

#if defined(VTSS_FEATURE_VSTAX)
            if (p == state->vstax_conf.port_0 || p == state->vstax_conf.port_1) {
                *stack_port_no = p;
            }
#endif

            *chip_port_mask |= (1ULL << state->port_map[p].chip_port);
            *port_no = p;
            (*port_cnt)++;
        }
    }

#if defined(VTSS_FEATURE_VSTAX)
    if (*stack_port_no != VTSS_PORT_NO_NONE && *port_cnt > 1) {
        VTSS_E("Both stack- and non-stack ports included in mask (0x%llx)", logical_port_mask);
        return VTSS_RC_ERROR;
    }
#endif

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_PACKET */

#if defined(VTSS_FEATURE_PACKET)
/*
 * Get logical port number from chip number and physical port number.
 * If <chip_no, chip_port> is not in the port map, the function
 * returns VTSS_PORT_NO_NONE.
 */
vtss_port_no_t vtss_cmn_chip_to_logical_port(const vtss_state_t       *const state,
                                             const vtss_chip_no_t            chip_no,
                                             const vtss_phys_port_no_t       chip_port)
{
    vtss_port_no_t port_no;
    for (port_no = VTSS_PORT_NO_START; port_no < state->port_count; port_no++) {
        if (VTSS_CHIP_PORT_FROM_STATE(state, port_no) == chip_port && VTSS_CHIP_NO_FROM_STATE(state, port_no) == chip_no) {
            return port_no;
        }
    }
    return VTSS_PORT_NO_NONE;
}
#endif /* VTSS_FEATURE_PACKET */

#if defined(VTSS_FEATURE_PACKET)
/*
 * Update Packet Rx hints and tag_type.
 * The classified VID (info->tag.vid) must be set prior to
 * calling this function.
 * On exit, info->tag.tpid is updated and info->tag_type and info->hints may
 * be updated.
 * This function doesn't support stack ports or internal ports, because
 * it's the original ingress port's properties that are required to be
 * able to set hints correctly.
 */
vtss_rc vtss_cmn_packet_hints_update(const vtss_state_t          *const state,
                                     const vtss_trace_group_t           trc_grp,
                                     const vtss_etype_t                 etype,
                                           vtss_packet_rx_info_t *const info)
{
    const vtss_vlan_port_conf_t *vlan_port_conf = &state->vlan_port_conf[info->port_no];

    if (info->port_no == VTSS_PORT_NO_NONE) {
        VTSS_EG(trc_grp, "Internal error");
        return VTSS_RC_ERROR;
    }

#if defined(VTSS_FEATURE_VSTAX)
    if (info->port_no == state->vstax_conf.port_0 || info->port_no == state->vstax_conf.port_1) {
        VTSS_EG(trc_grp, "Internal error. Stack ports (%u) not supported", info->port_no);
        return VTSS_RC_ERROR;
    }
#endif

     // Save the ethertype for the FDMA driver to be able to possibly re-insert a tag on Lu28.
    info->tag.tpid = etype;

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    switch (vlan_port_conf->port_type) {
    case VTSS_VLAN_PORT_TYPE_C:
        if (etype == VTSS_ETYPE_TAG_C) {
            info->tag_type = VTSS_TAG_TYPE_C_TAGGED;
        } else if (etype == VTSS_ETYPE_TAG_S || etype == state->vlan_conf.s_etype) {
            info->hints |= VTSS_PACKET_RX_HINTS_VLAN_TAG_MISMATCH;
        }
        break;

    case VTSS_VLAN_PORT_TYPE_S:
        if (etype == VTSS_ETYPE_TAG_S) {
            info->tag_type = VTSS_TAG_TYPE_S_TAGGED;
        } else if (etype == VTSS_ETYPE_TAG_C || etype == state->vlan_conf.s_etype) {
            info->hints |= VTSS_PACKET_RX_HINTS_VLAN_TAG_MISMATCH;
        }
        break;

    case VTSS_VLAN_PORT_TYPE_S_CUSTOM:
        if (etype == state->vlan_conf.s_etype) {
            info->tag_type = VTSS_TAG_TYPE_S_CUSTOM_TAGGED;
        } else if (etype == VTSS_ETYPE_TAG_C || etype == VTSS_ETYPE_TAG_S) {
            info->hints |= VTSS_PACKET_RX_HINTS_VLAN_TAG_MISMATCH;
        }
        break;

    default:
        break;
    }
#elif defined(VTSS_FEATURE_VLAN_PORT_V1)
    if (vlan_port_conf->aware && !vlan_port_conf->stag) {
        // C-Port.
        if (etype == VTSS_ETYPE_TAG_C) {
            info->tag_type = VTSS_TAG_TYPE_C_TAGGED;
        } else if (etype == VTSS_ETYPE_TAG_S) {
            info->hints |= VTSS_PACKET_RX_HINTS_VLAN_TAG_MISMATCH;
        }
    } else if (vlan_port_conf->aware && vlan_port_conf->stag) {
        if (etype == VTSS_ETYPE_TAG_S) {
            info->tag_type = VTSS_TAG_TYPE_S_TAGGED;
        } else if (etype == VTSS_ETYPE_TAG_C) {
            info->hints |= VTSS_PACKET_RX_HINTS_VLAN_TAG_MISMATCH;
        }
    }
#else
#error "Unsupported VLAN feature version"
#endif

    if ((info->tag_type == VTSS_TAG_TYPE_UNTAGGED && vlan_port_conf->frame_type == VTSS_VLAN_FRAME_TAGGED) ||
        (info->tag_type != VTSS_TAG_TYPE_UNTAGGED && vlan_port_conf->frame_type == VTSS_VLAN_FRAME_UNTAGGED)) {
        info->hints |= VTSS_PACKET_RX_HINTS_VLAN_FRAME_MISMATCH;
    }

    if (!VTSS_PORT_BF_GET(vtss_state->vlan_table[info->tag.vid].member, info->port_no)) {
        info->hints |= VTSS_PACKET_RX_HINTS_VID_MISMATCH;
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_FEATURE_PACKET */

