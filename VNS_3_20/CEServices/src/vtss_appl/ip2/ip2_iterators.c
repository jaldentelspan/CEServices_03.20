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

*/
#include "ip2_api.h"
#include "ip2_iterators.h"

#define VTSS_TRACE_MODULE_ID    VTSS_MODULE_ID_IP2

/* Statistics Section: RFC-4293 */
/*
    Return first statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either IPv4 or IPv6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_syst_stat_iter_first( const vtss_ip_type_t        *version,
                                            vtss_ips_ip_stat_t          *const entry)
{
    vtss_ips_status_t   *fdx, all_ips[IP2_ITER_MAX_IPS_OBJS];
    u32                 cnt_ips;

    if (!version || !entry) {
        return VTSS_RC_ERROR;
    }

    memset(all_ips, 0x0, sizeof(all_ips));
    if (vtss_ip2_ips_status_get(VTSS_IPS_STATUS_TYPE_ANY, IP2_ITER_MAX_IPS_OBJS, &cnt_ips, all_ips) == VTSS_OK) {
        u32 idx;

        for (idx = 0; idx < cnt_ips; idx++) {
            fdx = &all_ips[idx];

            if ((fdx->type == VTSS_IPS_STATUS_TYPE_STAT_IPV4) ||
                (fdx->type == VTSS_IPS_STATUS_TYPE_STAT_IPV6)) {
                memcpy(entry, &fdx->u.ip_stat, sizeof(vtss_ips_ip_stat_t));
                return VTSS_OK;
            }
        }
    }

    return VTSS_RC_ERROR;
}

/*
    Return specific statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either IPv4 or IPv6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_syst_stat_iter_get(   const vtss_ip_type_t        *version,
                                            vtss_ips_ip_stat_t          *const entry)
{
    vtss_ips_status_t       *fdx, all_ips[IP2_ITER_SINGLE_IPS_OBJS];
    vtss_ips_status_type_t  get_type;
    u32                     cnt_ips;

    if (!version || !entry) {
        return VTSS_RC_ERROR;
    }

    get_type = VTSS_IPS_STATUS_TYPE_ANY;
    if (*version == VTSS_IP_TYPE_IPV4) {
        get_type = VTSS_IPS_STATUS_TYPE_STAT_IPV4;
    } else if (*version == VTSS_IP_TYPE_IPV6) {
        get_type = VTSS_IPS_STATUS_TYPE_STAT_IPV6;
    } else {
        return VTSS_RC_ERROR;
    }

    memset(all_ips, 0x0, sizeof(all_ips));
    if (vtss_ip2_ips_status_get(get_type, IP2_ITER_SINGLE_IPS_OBJS, &cnt_ips, all_ips) == VTSS_OK) {
        u32 idx;

        for (idx = 0; idx < cnt_ips; idx++) {
            fdx = &all_ips[idx];

            if ((fdx->type == VTSS_IPS_STATUS_TYPE_STAT_IPV4) &&
                (*version == VTSS_IP_TYPE_IPV4)) {
                memcpy(entry, &fdx->u.ip_stat, sizeof(vtss_ips_ip_stat_t));
                return VTSS_OK;
            }
            if ((fdx->type == VTSS_IPS_STATUS_TYPE_STAT_IPV6) &&
                (*version == VTSS_IP_TYPE_IPV6)) {
                memcpy(entry, &fdx->u.ip_stat, sizeof(vtss_ips_ip_stat_t));
                return VTSS_OK;
            }
        }
    }

    return VTSS_RC_ERROR;
}

/*
    Return next statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either IPv4 or IPv6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_syst_stat_iter_next(  const vtss_ip_type_t        *version,
                                            vtss_ips_ip_stat_t          *const entry)
{
    vtss_ips_status_t       *fdx, all_ips[IP2_ITER_SINGLE_IPS_OBJS];
    vtss_ips_status_type_t  get_type;
    u32                     cnt_ips;

    if (!version || !entry) {
        return VTSS_RC_ERROR;
    }

    get_type = VTSS_IPS_STATUS_TYPE_ANY;
    if (*version < VTSS_IP_TYPE_IPV4) {
        get_type = VTSS_IPS_STATUS_TYPE_STAT_IPV4;
    } else {
        if (*version < VTSS_IP_TYPE_IPV6) {
            get_type = VTSS_IPS_STATUS_TYPE_STAT_IPV6;
        } else {
            return VTSS_RC_ERROR;
        }
    }

    memset(all_ips, 0x0, sizeof(all_ips));
    if (vtss_ip2_ips_status_get(get_type, IP2_ITER_SINGLE_IPS_OBJS, &cnt_ips, all_ips) == VTSS_OK) {
        u32 idx;

        for (idx = 0; idx < cnt_ips; idx++) {
            fdx = &all_ips[idx];

            if ((fdx->type == VTSS_IPS_STATUS_TYPE_STAT_IPV4) ||
                (fdx->type == VTSS_IPS_STATUS_TYPE_STAT_IPV6)) {
                memcpy(entry, &fdx->u.ip_stat, sizeof(vtss_ips_ip_stat_t));
                return VTSS_OK;
            }
        }
    }

    return VTSS_RC_ERROR;
}

static BOOL _vtss_ip2_cntr_intf_stat_iter_next(vtss_if_id_vlan_t vidx,
                                               u32 cnt,
                                               vtss_if_status_t *all_ifs,
                                               vtss_if_status_ip_stat_t *entry)
{
    u32                 idx;
    vtss_if_status_t    *fdx, *fnd;

    if (!all_ifs || !entry) {
        return FALSE;
    }

    fnd = NULL;
    for (idx = 0; idx < cnt; idx++) {
        fdx = &all_ifs[idx];
        if (fdx->if_id.type != VTSS_ID_IF_TYPE_VLAN) {
            continue;
        }

        if (fdx->if_id.u.vlan > vidx) {
            if (!fnd) {
                fnd = fdx;
            } else {
                if (fdx->if_id.u.vlan < fnd->if_id.u.vlan) {
                    fnd = fdx;
                }
            }
        }
    }

    if (fnd) {
        memcpy(entry, &fnd->u.ip_stat, sizeof(vtss_if_status_ip_stat_t));
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
    Return first interface statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param vidx (IN) - vlan index to use as input key.

    \param entry (OUT) - statistics of the matched IPv4 or IPv6 interfrace.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_intf_stat_iter_first( const vtss_ip_type_t        *version,
                                            const vtss_if_id_vlan_t     *vidx,
                                            vtss_if_status_ip_stat_t    *const entry)
{
    vtss_if_status_t    *all_ifs;
    u32                 cnt_ifs;

    if (!version || !vidx || !entry) {
        return VTSS_RC_ERROR;
    }

    all_ifs = calloc(IP2_ITER_MAX_IFS_OBJS, sizeof(vtss_if_status_t));
    if (all_ifs == NULL) {
        return VTSS_RC_ERROR;
    }

    cnt_ifs = 0;
    memset(all_ifs, 0x0, sizeof(all_ifs));
    if (vtss_ip2_ifs_status_get(VTSS_IF_STATUS_TYPE_STAT_IPV4, IP2_ITER_MAX_IFS_OBJS, &cnt_ifs, all_ifs) == VTSS_OK) {
        if (!_vtss_ip2_cntr_intf_stat_iter_next(VTSS_VID_NULL, cnt_ifs, all_ifs, entry)) {
            free(all_ifs);
            return VTSS_RC_ERROR;
        }
    } else {
        cnt_ifs = 0;
        memset(all_ifs, 0x0, sizeof(all_ifs));
        if (vtss_ip2_ifs_status_get(VTSS_IF_STATUS_TYPE_STAT_IPV6, IP2_ITER_MAX_IFS_OBJS, &cnt_ifs, all_ifs) == VTSS_OK) {
            if (!_vtss_ip2_cntr_intf_stat_iter_next(VTSS_VID_NULL, cnt_ifs, all_ifs, entry)) {
                free(all_ifs);
                return VTSS_RC_ERROR;
            }
        } else {
            free(all_ifs);
            return VTSS_RC_ERROR;
        }
    }

    free(all_ifs);
    return VTSS_OK;
}

/*
    Return specific interface statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param vidx (IN) - vlan index to use as input key.

    \param entry (OUT) - statistics of the matched IPv4 or IPv6 interfrace.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_intf_stat_iter_get(   const vtss_ip_type_t        *version,
                                            const vtss_if_id_vlan_t     *vidx,
                                            vtss_if_status_ip_stat_t    *const entry)
{
    vtss_if_status_t        *fdx, *all_ifs;
    vtss_if_status_type_t   get_type;
    u32                     cnt_ifs;

    if (!version || !vidx || !entry) {
        return VTSS_RC_ERROR;
    }

    get_type = VTSS_IF_STATUS_TYPE_ANY;
    if (*version == VTSS_IP_TYPE_IPV4) {
        get_type = VTSS_IF_STATUS_TYPE_STAT_IPV4;
    } else if (*version == VTSS_IP_TYPE_IPV6) {
        get_type = VTSS_IF_STATUS_TYPE_STAT_IPV6;
    } else {
        return VTSS_RC_ERROR;
    }

    all_ifs = calloc(IP2_ITER_SINGLE_IFS_OBJS, sizeof(vtss_if_status_t));
    if (all_ifs == NULL) {
        return VTSS_RC_ERROR;
    }

    cnt_ifs = 0;
    memset(all_ifs, 0x0, sizeof(all_ifs));
    if (vtss_ip2_if_status_get(get_type, *vidx, IP2_ITER_SINGLE_IFS_OBJS, &cnt_ifs, all_ifs) == VTSS_OK) {
        u32 idx;

        for (idx = 0; idx < cnt_ifs; idx++) {
            fdx = &all_ifs[idx];

            if ((fdx->type == get_type) &&
                (fdx->if_id.u.vlan == *vidx)) {
                memcpy(entry, &fdx->u.ip_stat, sizeof(vtss_if_status_ip_stat_t));
                free(all_ifs);
                return VTSS_OK;
            }
        }
    }

    free(all_ifs);
    return VTSS_RC_ERROR;
}

/*
    Return next interface statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param vidx (IN) - vlan index to use as input key.

    \param entry (OUT) - statistics of the matched IPv4 or IPv6 interfrace.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_intf_stat_iter_next(  const vtss_ip_type_t        *version,
                                            const vtss_if_id_vlan_t     *vidx,
                                            vtss_if_status_ip_stat_t    *const entry)
{
    vtss_if_status_t        *all_ifs;
    vtss_if_status_type_t   get_type;
    vtss_if_id_vlan_t       get_vidx;
    u32                     cnt_ifs;

    if (!version || !vidx || !entry) {
        return VTSS_RC_ERROR;
    }

    get_type = VTSS_IF_STATUS_TYPE_ANY;
    get_vidx = VTSS_VID_NULL;
    if (*version < VTSS_IP_TYPE_IPV4) {
        get_type = VTSS_IF_STATUS_TYPE_STAT_IPV4;
    } else {
        if (*version < VTSS_IP_TYPE_IPV6) {
            get_type = VTSS_IF_STATUS_TYPE_STAT_IPV4;
            if (*vidx < VTSS_VID_RESERVED) {
                get_vidx = *vidx;
            } else {
                get_type = VTSS_IF_STATUS_TYPE_STAT_IPV6;
            }
        } else if (*version == VTSS_IP_TYPE_IPV6) {
            get_type = VTSS_IF_STATUS_TYPE_STAT_IPV6;
            if (*vidx < VTSS_VID_RESERVED) {
                get_vidx = *vidx;
            } else {
                return VTSS_RC_ERROR;
            }
        } else {
            return VTSS_RC_ERROR;
        }
    }

    all_ifs = calloc(IP2_ITER_MAX_IFS_OBJS, sizeof(vtss_if_status_t));
    if (all_ifs == NULL) {
        return VTSS_RC_ERROR;
    }

    cnt_ifs = 0;
    memset(all_ifs, 0x0, sizeof(all_ifs));
    if (vtss_ip2_ifs_status_get(get_type, IP2_ITER_MAX_IFS_OBJS, &cnt_ifs, all_ifs) == VTSS_OK) {
        if (!_vtss_ip2_cntr_intf_stat_iter_next(get_vidx, cnt_ifs, all_ifs, entry)) {
            if (get_type == VTSS_IF_STATUS_TYPE_STAT_IPV6) {
                free(all_ifs);
                return VTSS_RC_ERROR;
            }

            cnt_ifs = 0;
            memset(all_ifs, 0x0, sizeof(all_ifs));
            if (vtss_ip2_ifs_status_get(VTSS_IF_STATUS_TYPE_STAT_IPV6, IP2_ITER_MAX_IFS_OBJS, &cnt_ifs, all_ifs) == VTSS_OK) {
                if (!_vtss_ip2_cntr_intf_stat_iter_next(VTSS_VID_NULL, cnt_ifs, all_ifs, entry)) {
                    free(all_ifs);
                    return VTSS_RC_ERROR;
                }
            }
        }
    }

    free(all_ifs);
    return VTSS_OK;
}

/*
    Return first ICMP statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either ICMP4 or ICMP6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_ver_iter_first(  const vtss_ip_type_t        *version,
                                            vtss_ips_icmp_stat_t        *const entry)
{
    vtss_ips_status_t   *fdx, all_ips[IP2_ITER_MAX_IPS_OBJS];
    u32                 cnt_ips;

    if (!version || !entry) {
        return VTSS_RC_ERROR;
    }

    memset(all_ips, 0x0, sizeof(all_ips));
    if (vtss_ip2_ips_status_get(VTSS_IPS_STATUS_TYPE_ANY, IP2_ITER_MAX_IPS_OBJS, &cnt_ips, all_ips) == VTSS_OK) {
        u32 idx;

        for (idx = 0; idx < cnt_ips; idx++) {
            fdx = &all_ips[idx];

            if ((fdx->type == VTSS_IPS_STATUS_TYPE_STAT_ICMP4) ||
                (fdx->type == VTSS_IPS_STATUS_TYPE_STAT_ICMP6)) {
                memcpy(entry, &fdx->u.icmp_stat, sizeof(vtss_ips_icmp_stat_t));
                return VTSS_OK;
            }
        }
    }

    return VTSS_RC_ERROR;
}

/*
    Return specific ICMP statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either ICMP4 or ICMP6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_ver_iter_get(    const vtss_ip_type_t        *version,
                                            vtss_ips_icmp_stat_t        *const entry)
{
    vtss_ips_status_t       *fdx, all_ips[IP2_ITER_SINGLE_IPS_OBJS];
    vtss_ips_status_type_t  get_type;
    u32                     cnt_ips;

    if (!version || !entry) {
        return VTSS_RC_ERROR;
    }

    get_type = VTSS_IPS_STATUS_TYPE_ANY;
    if (*version == VTSS_IP_TYPE_IPV4) {
        get_type = VTSS_IPS_STATUS_TYPE_STAT_ICMP4;
    } else if (*version == VTSS_IP_TYPE_IPV6) {
        get_type = VTSS_IPS_STATUS_TYPE_STAT_ICMP6;
    } else {
        return VTSS_RC_ERROR;
    }

    memset(all_ips, 0x0, sizeof(all_ips));
    if (vtss_ip2_ips_status_get(get_type, IP2_ITER_SINGLE_IPS_OBJS, &cnt_ips, all_ips) == VTSS_OK) {
        u32 idx;

        for (idx = 0; idx < cnt_ips; idx++) {
            fdx = &all_ips[idx];

            if ((fdx->type == VTSS_IPS_STATUS_TYPE_STAT_ICMP4) &&
                (*version == VTSS_IP_TYPE_IPV4)) {
                memcpy(entry, &fdx->u.icmp_stat, sizeof(vtss_ips_icmp_stat_t));
                return VTSS_OK;
            }
            if ((fdx->type == VTSS_IPS_STATUS_TYPE_STAT_ICMP6) &&
                (*version == VTSS_IP_TYPE_IPV6)) {
                memcpy(entry, &fdx->u.icmp_stat, sizeof(vtss_ips_icmp_stat_t));
                return VTSS_OK;
            }
        }
    }

    return VTSS_RC_ERROR;
}

/*
    Return next ICMP statistics found in IP stack.

    \param version (IN) - version to use as input key.

    \param entry (OUT) - statistics of either ICMP4 or ICMP6.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_ver_iter_next(   const vtss_ip_type_t        *version,
                                            vtss_ips_icmp_stat_t        *const entry)
{
    vtss_ips_status_t       *fdx, all_ips[IP2_ITER_SINGLE_IPS_OBJS];
    vtss_ips_status_type_t  get_type;
    u32                     cnt_ips;

    if (!version || !entry) {
        return VTSS_RC_ERROR;
    }

    get_type = VTSS_IPS_STATUS_TYPE_ANY;
    if (*version < VTSS_IP_TYPE_IPV4) {
        get_type = VTSS_IPS_STATUS_TYPE_STAT_ICMP4;
    } else {
        if (*version < VTSS_IP_TYPE_IPV6) {
            get_type = VTSS_IPS_STATUS_TYPE_STAT_ICMP6;
        } else {
            return VTSS_RC_ERROR;
        }
    }

    memset(all_ips, 0x0, sizeof(all_ips));
    if (vtss_ip2_ips_status_get(get_type, IP2_ITER_SINGLE_IPS_OBJS, &cnt_ips, all_ips) == VTSS_OK) {
        u32 idx;

        for (idx = 0; idx < cnt_ips; idx++) {
            fdx = &all_ips[idx];

            if ((fdx->type == VTSS_IPS_STATUS_TYPE_STAT_ICMP4) ||
                (fdx->type == VTSS_IPS_STATUS_TYPE_STAT_ICMP6)) {
                memcpy(entry, &fdx->u.icmp_stat, sizeof(vtss_ips_icmp_stat_t));
                return VTSS_OK;
            }
        }
    }

    return VTSS_RC_ERROR;
}

/*
    Return first ICMP MSG statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param message (IN) - message type to use as input key.

    \param entry (OUT) - statistics of matched ICMP4 or ICMP6 message.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_msg_iter_first(  const vtss_ip_type_t        *version,
                                            const u32                   *message,
                                            vtss_ips_icmp_stat_t        *const entry)
{
    if (!version || !message || !entry) {
        return VTSS_RC_ERROR;
    }

    return vtss_ip2_stat_imsg_cntr_getfirst(VTSS_IP_TYPE_NONE, IP2_STAT_IMSG_MAX, entry);
}

/*
    Return specific ICMP MSG statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param message (IN) - message type to use as input key.

    \param entry (OUT) - statistics of matched ICMP4 or ICMP6 message.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_msg_iter_get(    const vtss_ip_type_t        *version,
                                            const u32                   *message,
                                            vtss_ips_icmp_stat_t        *const entry)
{
    if (!version || !message || !entry) {
        return VTSS_RC_ERROR;
    }

    return vtss_ip2_stat_imsg_cntr_get(*version, *message, entry);
}

/*
    Return next ICMP MSG statistics found in IP stack.

    \param version (IN) - version to use as input key.
    \param message (IN) - message type to use as input key.

    \param entry (OUT) - statistics of matched ICMP4 or ICMP6 message.

    \return VTSS_OK iff entry is found.
 */
vtss_rc vtss_ip2_cntr_icmp_msg_iter_next(   const vtss_ip_type_t        *version,
                                            const u32                   *message,
                                            vtss_ips_icmp_stat_t        *const entry)
{
    vtss_ip_type_t  get_vers;
    u32             get_imsg;

    if (!version || !message || !entry) {
        return VTSS_RC_ERROR;
    }

    get_vers = VTSS_IP_TYPE_NONE;
    get_imsg = IP2_STAT_IMSG_MAX;
    if (*version < VTSS_IP_TYPE_IPV4) {
        get_vers = VTSS_IP_TYPE_IPV4;
        get_imsg = 0;

        return vtss_ip2_stat_imsg_cntr_get(get_vers, get_imsg, entry);
    } else {
        get_vers = *version;
        if (*version < VTSS_IP_TYPE_IPV6) {
            if (*message < IP2_STAT_IMSG_MAX) {
                get_imsg = *message;
            } else {
                get_vers = VTSS_IP_TYPE_IPV6;
                get_imsg = 0;

                return vtss_ip2_stat_imsg_cntr_get(get_vers, get_imsg, entry);
            }
        } else if (*version == VTSS_IP_TYPE_IPV6) {
            if (*message < IP2_STAT_IMSG_MAX) {
                get_imsg = *message;
            } else {
                return VTSS_RC_ERROR;
            }
        } else {
            return VTSS_RC_ERROR;
        }
    }

    return vtss_ip2_stat_imsg_cntr_getnext(get_vers, get_imsg, entry);
}
