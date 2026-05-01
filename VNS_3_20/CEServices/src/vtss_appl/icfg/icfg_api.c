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

#include "icfg.h"
#include "icfg_api.h"
#include "msg_api.h"
#include "vtss_ecos_mutex_api.h"
#include "critd_api.h"
#if defined(CYGPKG_FS_RAM)
#include "os_file_api.h"
#endif /* CYGPKG_FS_RAM */
#ifdef VTSS_SW_OPTION_VLAN
#include "vlan_api.h"
#endif
#if defined(VTSS_SW_OPTION_MSTP) && defined(VTSS_SW_OPTION_AGGR)
#include "aggr_api.h"
#include "topo_api.h"
#include "misc_api.h"
#endif

#include <stdio.h>
#include <unistd.h>

#if defined(VTSS_SW_OPTION_IPMC)
#include "ipmc_api.h"
#endif /* VTSS_SW_OPTION_IPMC */
#if defined(VTSS_SW_OPTION_IPMC_LIB)
#include "ipmc_lib.h"
#endif /* VTSS_SW_OPTION_IPMC_LIB */

#if defined(VTSS_SW_OPTION_SNMP)
#include "vtss_snmp_api.h"
#endif /* VTSS_SW_OPTION_SNMP */


#define VTSS_RC(expr)   { vtss_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) return __rc__; }
#define ICLI_RC(expr)   { i32     __rc__ = (expr); if (__rc__ < ICLI_RC_OK) return VTSS_RC_ERROR; }

#define MAP_TABLE_CNT   (sizeof(mode_to_order_map)/sizeof(icfg_map_t))

#define DEFAULT_TEXT_BLOCK_SIZE (1024*1024)



/* Map: Mode => { first, last } */
typedef struct {
    icli_cmd_mode_t      mode;
    vtss_icfg_ordering_t first;
    vtss_icfg_ordering_t last;
} icfg_map_t;



/* Adapt this table when new modes are introduced: */
static const icfg_map_t mode_to_order_map[] =
    {
        { ICLI_CMD_MODE_CONFIG_VLAN,         VTSS_ICFG_VLAN_BEGIN,               VTSS_ICFG_VLAN_END               },
#if defined(VTSS_SW_OPTION_IPMC_LIB)
        { ICLI_CMD_MODE_IPMC_PROFILE,        VTSS_ICFG_IPMC_BEGIN,               VTSS_ICFG_IPMC_END               },
#endif /* VTSS_SW_OPTION_IPMC_LIB */
#if defined(VTSS_SW_OPTION_SNMP)
        { ICLI_CMD_MODE_SNMPS_HOST,          VTSS_ICFG_SNMPSERVER_HOST_BEGIN,    VTSS_ICFG_SNMPSERVER_HOST_END    },
#endif /* VTSS_SW_OPTION_SNMP */
        { ICLI_CMD_MODE_INTERFACE_PORT_LIST, VTSS_ICFG_INTERFACE_ETHERNET_BEGIN, VTSS_ICFG_INTERFACE_ETHERNET_END },
        { ICLI_CMD_MODE_INTERFACE_VLAN,      VTSS_ICFG_INTERFACE_VLAN_BEGIN,     VTSS_ICFG_INTERFACE_VLAN_END     },
        { ICLI_CMD_MODE_STP_AGGR,            VTSS_ICFG_STP_AGGR_BEGIN,           VTSS_ICFG_STP_AGGR_END           },
        { ICLI_CMD_MODE_CONFIG_LINE,         VTSS_ICFG_LINE_BEGIN,               VTSS_ICFG_LINE_END               },
    };


// List of callbacks. We waste a few entries for the ...BEGIN and ...END values,
// but that's so little we don't want to exchange it for more complex code.
static vtss_icfg_query_func_t icfg_callbacks[VTSS_ICFG_LAST];



// Mutex for protecting whole-file load/save ops: Only one can be in progress
// at any time; overlapping requests must be denied with an error message
/*lint -esym(459, icfg_io_mutex) */
static vtss_ecos_mutex_t icfg_io_mutex;



// Critical region for protection of icfg_callbacks and the callbacks themselves
static critd_t icfg_crit;


#if (VTSS_TRACE_ENABLED)
static vtss_trace_reg_t trace_reg = {
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "icfg",
    .descr     = "Industrial Configuration Engine"
};

static vtss_trace_grp_t trace_grps[TRACE_GRP_CNT] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        .name      = "default",
        .descr     = "Default",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [TRACE_GRP_CRIT] = {
        .name      = "crit",
        .descr     = "Critical regions ",
        .lvl       = VTSS_TRACE_LVL_ERROR,
        .timestamp = 1,
    }
};
#define ICFG_CRIT_ENTER() critd_enter(&icfg_crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#define ICFG_CRIT_EXIT()  critd_exit( &icfg_crit, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#else
#define ICFG_CRIT_ENTER() critd_enter(&icfg_crit)
#define ICFG_CRIT_EXIT()  critd_exit( &icfg_crit)
#endif /* VTSS_TRACE_ENABLED */



// Query Registration


vtss_rc vtss_icfg_query_register(vtss_icfg_ordering_t   order,
                                 vtss_icfg_query_func_t query_cb)
{
    /*lint --e{459} */
    vtss_rc rc = VTSS_RC_OK;

    if (order >= VTSS_ICFG_LAST  ||
        icfg_callbacks[order] != NULL) {
        rc = VTSS_RC_ERROR;
    }
    else {
        icfg_callbacks[order] = query_cb;
    }

    return rc;
}



// Query Request

static vtss_rc icfg_format_mode_header(vtss_icfg_query_request_t *req, vtss_icfg_query_result_t  *result)
{
    switch (req->cmd_mode) {
    case ICLI_CMD_MODE_CONFIG_VLAN:
        VTSS_RC(vtss_icfg_printf(result, "vlan %u\n", req->instance_id.vlan));
        break;

#if defined(VTSS_SW_OPTION_IPMC_LIB)
    case ICLI_CMD_MODE_IPMC_PROFILE:
        VTSS_RC(vtss_icfg_printf(result, "ipmc profile %s\n", req->instance_id.string));
        break;
#endif /* VTSS_SW_OPTION_IPMC_LIB */

#if defined(VTSS_SW_OPTION_SNMP)
    case ICLI_CMD_MODE_SNMPS_HOST:
        VTSS_RC(vtss_icfg_printf(result, "snmp-server host %s\n", req->instance_id.string));
        break;
#endif /* VTSS_SW_OPTION_SNMP */

    case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
        VTSS_RC(vtss_icfg_printf(result, "interface %s %u/%u\n",
            icli_port_type_get_name(req->instance_id.port.port_type),
            req->instance_id.port.switch_id,
            req->instance_id.port.begin_port));
        break;

    case ICLI_CMD_MODE_INTERFACE_VLAN:
        VTSS_RC(vtss_icfg_printf(result, "interface vlan %u\n", req->instance_id.vlan));
        break;

    case ICLI_CMD_MODE_STP_AGGR:
        VTSS_RC(vtss_icfg_printf(result, "spanning-tree aggregation\n"));
        break;

    case ICLI_CMD_MODE_CONFIG_LINE:
        if (req->instance_id.line == 0) {
            VTSS_RC(vtss_icfg_printf(result, "line console 0\n"));
        }
        else {
            VTSS_RC(vtss_icfg_printf(result, "line vty %u\n", req->instance_id.line - 1));
        }
        break;

    default:
        // Ignore
        break;
    }
    
    return VTSS_RC_OK;
    
}

static vtss_rc icfg_iterate_callbacks(vtss_icfg_query_request_t *req,
                                      vtss_icfg_ordering_t      first,
                                      vtss_icfg_ordering_t      last,
                                      vtss_icfg_query_result_t  *result)
{
    vtss_rc rc = VTSS_RC_OK;

    ICFG_CRIT_ENTER();
    for (; (first < last)  &&  (rc == VTSS_RC_OK); ++first) {
        if (icfg_callbacks[first]) {
            T_N("Invoking callback, order %d", first);
            req->order = first;
            rc = (icfg_callbacks[first])(req, result);
        }
    }
    ICFG_CRIT_EXIT();

    return rc;
}

static vtss_rc icfg_process_range(icli_cmd_mode_t          mode,
                                  vtss_icfg_ordering_t     first,
                                  vtss_icfg_ordering_t     last,
                                  BOOL                     all_defaults,
                                  vtss_icfg_query_result_t *result)
{
    vtss_icfg_query_request_t req;

    req.cmd_mode     = mode;
    req.all_defaults = all_defaults;

    switch (mode) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
        VTSS_RC(icfg_iterate_callbacks(&req, first, last, result));
        VTSS_RC(vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n"));
        break;

    case ICLI_CMD_MODE_CONFIG_VLAN:
        {
#ifdef VTSS_SW_OPTION_VLAN
            vlan_mgmt_entry_t v;
            v.vid = 0;
            while (vlan_mgmt_vlan_get(VTSS_ISID_GLOBAL, v.vid, &v, TRUE, VLAN_USER_ALL, VLAN_MEMBERSHIP_OP) == VTSS_RC_OK) {
                req.instance_id.vlan = v.vid;
                VTSS_RC(icfg_format_mode_header(&req, result));
                VTSS_RC(icfg_iterate_callbacks(&req, first, last, result));
                VTSS_RC(vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n"));
            }
#endif
        }
        break;

#if defined(VTSS_SW_OPTION_SNMP)
    case ICLI_CMD_MODE_SNMPS_HOST:
        {
            vtss_trap_entry_t trap_entry;
            // FIXME: Sgetz to provide host enumeration here
            memset(trap_entry.trap_conf_name, 0, sizeof(trap_entry.trap_conf_name));
            while ( VTSS_RC_OK == trap_mgmt_conf_get_next (&trap_entry)) {
                strncpy(req.instance_id.string, trap_entry.trap_conf_name, TRAP_MAX_NAME_LEN);
                req.instance_id.string[TRAP_MAX_NAME_LEN] = 0;
                VTSS_RC(icfg_format_mode_header(&req, result));
                VTSS_RC(icfg_iterate_callbacks(&req, first, last, result));
                VTSS_RC(vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n"));
            }
        }
        break;
#endif

#if defined(VTSS_SW_OPTION_IPMC_LIB)
    case ICLI_CMD_MODE_IPMC_PROFILE:
        {
            ipmc_lib_profile_mem_t      *pf;
            ipmc_lib_grp_fltr_profile_t *fltr_profile;

            if (!IPMC_MEM_PROFILE_MTAKE(pf)) {
                break;
            }

            fltr_profile = &pf->profile;
            memset(fltr_profile, 0x0, sizeof(ipmc_lib_grp_fltr_profile_t));
            while (ipmc_lib_mgmt_fltr_profile_get_next(fltr_profile, TRUE) == VTSS_OK) {
                memset(req.instance_id.string, 0x0, sizeof(req.instance_id.string));
                memcpy(req.instance_id.string, fltr_profile->data.name, sizeof(fltr_profile->data.name));
                if (icfg_format_mode_header(&req, result) == VTSS_OK) {
                    (void) icfg_iterate_callbacks(&req, first, last, result);
                    (void) vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n");
                }
            }

            IPMC_MEM_PROFILE_MGIVE(pf);
        }
        break;
#endif /* VTSS_SW_OPTION_IPMC_LIB */

    case ICLI_CMD_MODE_INTERFACE_PORT_LIST:
        {
            u32                     type_idx;
            u32                     r_idx;
            icli_stack_port_range_t *range       = (icli_stack_port_range_t*) malloc(sizeof(icli_stack_port_range_t));

            if (!range) {
                T_D("Not enough memory to retrieve stack range");
                return VTSS_RC_ERROR;
            }
            if (!icli_port_range_get(range)) {
                T_E("ICFG cannot retrieve port range from ICLI");
                free(range);
                return VTSS_RC_ERROR;
            }

            for (type_idx = ICLI_PORT_TYPE_NONE+1; type_idx < ICLI_PORT_TYPE_MAX; ++type_idx) {
                for (r_idx = 0; r_idx < range->cnt; ++r_idx) {
                    if (range->switch_range[r_idx].port_type == type_idx) {
                        u32 i;
                        req.instance_id.port          = range->switch_range[r_idx];
                        req.instance_id.port.port_cnt = 1;
                        for (i = 0; i < range->switch_range[r_idx].port_cnt; ++i) {
                            (void)icfg_format_mode_header(&req, result);
                            (void)icfg_iterate_callbacks(&req, first, last, result);
                            (void)vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n");
                            req.instance_id.port.begin_port++;
                            req.instance_id.port.begin_uport++;
                        }
                    }
                }
            }

            free(range);
        }
    break;

    case ICLI_CMD_MODE_INTERFACE_VLAN:
        {
            BOOL                do_cb;
            vtss_vid_t          vidx;
#if defined(VTSS_SW_OPTION_VLAN)
            vlan_mgmt_entry_t   vlanx;
#endif /* VTSS_SW_OPTION_VLAN */
#if defined(VTSS_SW_OPTION_IPMC)
            BOOL                dummy;
#endif /* VTSS_SW_OPTION_IPMC */
#if defined(VTSS_SW_OPTION_VLAN) || defined(VTSS_SW_OPTION_IPMC)
            vtss_vid_t          idx;
#endif /* VTSS_SW_OPTION_VLAN || VTSS_SW_OPTION_IPMC */

            for (vidx = VTSS_VID_NULL + 1; vidx < VTSS_VIDS; vidx++) {
                do_cb = FALSE;

#ifdef VTSS_SW_OPTION_VLAN
                vlanx.vid = idx = vidx;
                if (!do_cb &&
                    vlan_mgmt_vlan_get(VTSS_ISID_GLOBAL, idx, &vlanx, FALSE, VLAN_USER_ALL, VLAN_MEMBERSHIP_OP) == VTSS_RC_OK) {
                    do_cb = TRUE;
                }
#endif /* VTSS_SW_OPTION_VLAN */

#if defined(VTSS_SW_OPTION_IPMC)
                idx = vidx;
                if (!do_cb &&
                    ipmc_mgmt_get_intf_state_querier(TRUE, &idx, &dummy, &dummy, FALSE, IPMC_IP_VERSION_IGMP) == VTSS_OK) {
                    do_cb = TRUE;
                }

                idx = vidx;
                if (!do_cb &&
                    ipmc_mgmt_get_intf_state_querier(TRUE, &idx, &dummy, &dummy, FALSE, IPMC_IP_VERSION_MLD) == VTSS_OK) {
                    do_cb = TRUE;
                }
#endif /* VTSS_SW_OPTION_IPMC */

                if (do_cb) {
                    req.instance_id.vlan = vidx;
                    if (icfg_format_mode_header(&req, result) == VTSS_OK) {
                        (void) icfg_iterate_callbacks(&req, first, last, result);
                        (void) vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n");
                    }
                }
            }
        }
        break;

    case ICLI_CMD_MODE_STP_AGGR:
#if defined(VTSS_SW_OPTION_MSTP) && defined(VTSS_SW_OPTION_AGGR)
        {
	    BOOL                    done = FALSE;
            u32                     r_idx, i;
            icli_stack_port_range_t *range = (icli_stack_port_range_t*) malloc(sizeof(icli_stack_port_range_t));

            if (!range) {
                T_D("Not enough memory to retrieve stack range");
                return VTSS_RC_ERROR;
            }
            if (!icli_port_range_get(range)) {
                T_E("ICFG cannot retrieve port range from ICLI");
                free(range);
                return VTSS_RC_ERROR;
            }

	    for (r_idx = 0; !done  &&  r_idx < range->cnt; ++r_idx) {
		u32 isid = topo_usid2isid(range->switch_range[r_idx].usid);
		for (i = 0; !done  &&  i < range->switch_range[r_idx].port_cnt; ++i) {
		    if (aggr_mgmt_port_participation(isid, uport2iport(range->switch_range[r_idx].begin_port + i))) {
			(void)icfg_format_mode_header(&req, result);
			(void)icfg_iterate_callbacks(&req, first, last, result);
			(void)vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n");
			done = TRUE;
		    }
		}
	    }
            free(range);
        }
#endif
        break;

    case ICLI_CMD_MODE_CONFIG_LINE:
        {
            u32 max = icli_session_max_get();
            u32 i;
            icli_session_data_t ses;

            for (i = 0; i < max; ++i) {
                ses.session_id = i;
                ICLI_RC(icli_session_data_get(&ses));
                req.instance_id.line = i;
                VTSS_RC(icfg_format_mode_header(&req, result));
                VTSS_RC(icfg_iterate_callbacks(&req, first, last, result));
                VTSS_RC(vtss_icfg_printf(result, VTSS_ICFG_COMMENT_LEADIN"\n"));
            }
        }
        break;

    default:
        T_E("That's really odd; shouldn't get here. Porting issue?");
        break;
    }
    
    return VTSS_RC_OK;
}

vtss_rc vtss_icfg_query_all(BOOL all_defaults, vtss_icfg_query_result_t *result)
{
    u32                  map_idx = 0;
    vtss_icfg_ordering_t next = 0;

    if (!msg_switch_is_master()) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_icfg_init_query_result(0, result));

    while (next < VTSS_ICFG_LAST) {
        if (map_idx < MAP_TABLE_CNT) {
            if (next < mode_to_order_map[map_idx].first) {
                /* Global section prior to/in-between submode(s) */
                VTSS_RC(icfg_process_range(ICLI_CMD_MODE_GLOBAL_CONFIG, next, mode_to_order_map[map_idx].first, all_defaults, result));
                next = mode_to_order_map[map_idx].first + 1;
            }
            else {
                /* Submode */
                VTSS_RC(icfg_process_range(mode_to_order_map[map_idx].mode, next, mode_to_order_map[map_idx].last, all_defaults, result));
                next = mode_to_order_map[map_idx].last + 1;
                map_idx++;
            }
        }
        else {
            /* Global section after last submode */
            VTSS_RC(icfg_process_range(ICLI_CMD_MODE_GLOBAL_CONFIG, next, VTSS_ICFG_LAST, all_defaults, result));
            next = VTSS_ICFG_LAST;
        }
    }

    VTSS_RC(vtss_icfg_printf(result, "end\n"));
    return VTSS_RC_OK;
}

vtss_rc vtss_icfg_query_specific(vtss_icfg_query_request_t *req,
                                 vtss_icfg_query_result_t  *result)
{
    u32 i;

    if (!msg_switch_is_master()) {
        return VTSS_RC_ERROR;
    }

    for (i = 0; i < MAP_TABLE_CNT  &&  mode_to_order_map[i].mode != req->cmd_mode; ++i) {
        // search
    }

    if (i < MAP_TABLE_CNT) {
        VTSS_RC(icfg_format_mode_header(req, result));
        VTSS_RC(icfg_iterate_callbacks(req, mode_to_order_map[i].first + 1, mode_to_order_map[i].last, result));
    }

    return VTSS_RC_OK;
}



// Query Result

static vtss_icfg_query_result_buf_t *icfg_alloc_buf(u32 initial_size)
{
    vtss_icfg_query_result_buf_t *buf;

    if (!initial_size) {
        initial_size = DEFAULT_TEXT_BLOCK_SIZE;
    }

    buf = (vtss_icfg_query_result_buf_t *) malloc(sizeof(vtss_icfg_query_result_buf_t) + initial_size);

    if (buf) {
        buf->used = 0;
        buf->size = initial_size;
        buf->next = NULL;
    }

    return buf;
}

vtss_rc vtss_icfg_init_query_result(u32 initial_size,
                                    vtss_icfg_query_result_t *res)
{
    if (!msg_switch_is_master()) {
        res->head = NULL;
        res->tail = NULL;
        return VTSS_RC_ERROR;
    }

    res->head = icfg_alloc_buf(initial_size);
    res->tail = res->head;

    return res->head ? VTSS_RC_OK : VTSS_RC_ERROR;
}

void vtss_icfg_free_query_result(vtss_icfg_query_result_t *res)
{
    if (!msg_switch_is_master()) {
        return;
    }

    vtss_icfg_query_result_buf_t *current, *next;

    current = res->head;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    res->head = NULL;
    res->tail = NULL;
}

vtss_rc vtss_icfg_printf(vtss_icfg_query_result_t *res, const char *format, ...)
{
    /*lint --e{530}    Kill warning about use-before-initialization */
    va_list va;
    int n;
    int bytes_available;
    vtss_icfg_query_result_buf_t *next;

    if (!msg_switch_is_master()) {
        return VTSS_RC_ERROR;
    }

    /* The eCos implementation of vsnprintf() has a weakness: The return value
     * doesn't indicate whether the function ran out of destination buffer
     * space or not. It could return -1 if it did, or return the number of
     * characters that _would_ have been written to the destination (not
     * including the trailing '\0' byte).
     * 
     * But it doesn't. If called with a size of N bytes, it will always write
     * up to N-1 bytes and return that value, no matter if the desired result
     * would have been longer than that.
     * 
     * So the only way to determine if we're running out is this:
     * 
     *   * Supply buffer with capacity N
     *   * Max. return value from vsnprintf is then N-1
     *   * So if the return value is N-2 or less, then we know that there was
     *     room for at least one more char in the buffer, i.e. we printed all
     *     there was to print.
     */

    while (TRUE) {
        bytes_available = res->tail->size - res->tail->used;

        if (bytes_available < 3) {   // 3 == One byte for output, one for '\0', one to work-around non-C99 eCos vsnprintf()
            next = icfg_alloc_buf(0);
            if (next == NULL) {
                return VTSS_RC_ERROR;
            }
            res->tail->next = next;
            res->tail       = next;
            bytes_available = res->tail->size;
        }

        va_start(va, format);
        n = vsnprintf(res->tail->text + res->tail->used, bytes_available, format, va);
        va_end(va);

        if (n >= 0  &&  n < (bytes_available - 1)) {    // -1 due to eCos workaround
            res->tail->used += n;
            return VTSS_RC_OK;
        }
        else if (n >= DEFAULT_TEXT_BLOCK_SIZE - 1 - 1) {     // -1 for '\0', -1 for eCos workaround
            /* Not enough contiguous mem in our buffer _and_ the string is too
             * long for our (fairly large) buffer: Give up with an error
             * message
             */
            T_E("vtss_icfg_printf cannot print string of length %u; too long", n);
            return VTSS_RC_ERROR;
        }
        else {
            /* Not enough contiguous mem in our buffer, but the string will fit
             * inside an empty one: Allocate new buffer and vsnprintf() again.
             *
             * Note that is strategy is, of course, subject to an attack where
             * the requested buffer capacity is always half the max buffer size
             * plus one.
             */
            next = icfg_alloc_buf(0);
            if (next == NULL) {
                return VTSS_RC_ERROR;
            }
            res->tail->next = next;
            res->tail       = next;
        }
    }  /* while (TRUE) */
}



// URL utilities

#define MAX_PORT (0xffff)

void vtss_icfg_init_url_parts_t(vtss_icfg_url_parts_t *parts)
{
    parts->protocol[0] = 0;
    parts->host[0]     = 0;
    parts->port        = 0;
    parts->path[0]     = 0;
}

BOOL vtss_icfg_decompose_url(const char *url, vtss_icfg_url_parts_t *parts)
{
    const char *p;
    u32        n;

    vtss_icfg_init_url_parts_t(parts);

    // Protocol

    p = url;
    n = 0;
    while (*p  &&  *p != ':'  &&  n < sizeof(parts->protocol) - 1) {
      parts->protocol[n++] = *p++;
    }
    if ((*p != ':')  ||  (n == 0)) {
        return FALSE;
    }
    parts->protocol[n] = 0;
    p++;

    // For flash: protocol, only decompose protocol and path

    if (strcmp(parts->protocol, "flash") == 0) {
        n = 0;
        while (*p  &&  n < sizeof(parts->path) - 1) {
          parts->path[n++] = *p++;
        }
        parts->path[n] = 0;
        return (!*p)  &&  (n > 0)  &&  (n < NAME_MAX);
    }

    // Not flash:, decompose all

    // Double-slash

    if ((*p != '/')  ||  (*(p+1) != '/')) {
        return FALSE;
    }
    p += 2;

    // Host

    n = 0;
    while (*p  &&  *p != '/'  &&  *p != ':'  &&  n < sizeof(parts->host) - 1) {
        parts->host[n++] = *p++;
    }
    parts->host[n] = 0;
    if ((n == 0)  ||  (n == sizeof(parts->host) - 1)) {
        return FALSE;
    }

    // Optional port

    if (*p == ':') {
        int port = 0;
        ++p;
        if (!(*p >= '0'  &&  *p <= '9')) {
            return FALSE;
        }
        while (*p >= '0'  &&  *p <= '9') {
            port = 10*port + (*p - '0');
            if (port > MAX_PORT) {
                return FALSE;
            }
            ++p;
        }
        parts->port = (u16)port;
    }

    // Single slash

    if (*p != '/') {
        return FALSE;
    }

    // Path

    n = 0;
    while (*p  &&  n < sizeof(parts->path) - 1) {
      parts->path[n++] = *p++;
    }
    parts->path[n] = 0;

    return (!*p)  &&  (n > 0);
}



// Shortest possible URL (but meaningless in our world) is length 7: a://b/c

BOOL vtss_icfg_compose_url(char *url, int max_len, const vtss_icfg_url_parts_t *parts)
{
    int n;

    if (!url                 ||  max_len < 8      ||  !parts  ||
        !parts->protocol     ||  !parts->path     ||
        !(*parts->protocol)  ||  !(*parts->path)) {
        return FALSE;
    }

    if (strcmp(parts->protocol, "flash") == 0) {
        n = snprintf(url, max_len, "%s:%s", parts->protocol, parts->path);
    }
    else {
        if (!parts->host  ||  !(*parts->host)) {
            return FALSE;
        }
        if (parts->port) {
            n = snprintf(url, max_len, "%s://%s:%u/%s",
                parts->protocol, parts->host, parts->port, parts->path);
        }
        else {
            n = snprintf(url, max_len, "%s://%s/%s",
                parts->protocol, parts->host, parts->path);
        }
    }

    return (n > 0)  &&  (n < max_len - 1);   // Less-than because n doesn't count trailing '\0'; -1 because of eCos silliness regarding snprintf return value
}



#if 0
// Not included right now; awaits clarification about boot/restore-to-default:
// When are modules ready to receive configuration via ICLI/ICFG?

static inline BOOL is_end(const char *s)
{
    while (*s == ' '  ||  *s == '\t') {
        s++;
    }
    return ((*s++ == 'e')  &&  (*s++ == 'n')  &&  (*s++ == 'd')  &&
            (*s == '\0'  ||  *s == '\n'  ||  *s == '\r'  ||  *s == ' '  ||  *s == '\t'));
}

/* Commit one line to ICLI. */
static void icfg_commit_one_line_to_icli(char       *cmd,
                                         const char *source_name,
                                         u32        line_cnt,
                                         u32        *err_cnt)
{
    i32        rc;
    const char *p;
    const int  session_id = 0;   // Console

    // ICLI doesn't much like an empty line, so skip it here
    for (p = cmd; *p == ' '  ||  *p == '\t'  ||  *p == '\n'  ||  *p == '\r'; ++p) {
        /* loop */
    }
    if (!*p) {
        return;
    }

    if ((rc=ICLI_CMD_EXEC_ERR_FILE_LINE(cmd, TRUE, (char*)source_name, line_cnt, TRUE)) != ICLI_RC_OK) {
        ++(*err_cnt);
        T_D("ICLI cmd exec err: rc=%ld, file %s, line %lu", rc, source_name, line_cnt);
        T_D("                   cmd '%s'", cmd);
    }
}

/* Commit entire file to ICLI */
static void icfg_commit_to_icli(const char               *source_name,
                                vtss_icfg_query_result_t *res)
{
#define N 1024
    const int                    session_id = 0;     // Console
    const char                   *p;                 // Pointer into buffer; current char being processed
    u32                          len;                // Remaining bytes in current buffer
    char                         cmd[N];             // Command line being built for ICLI
    char                         *pcmd;              // Pointer into cmd[]
    u32                          err_cnt  = 0;       // Error count: How many times ICLI has complained
    u32                          line_cnt = 0;       // Line count in file
    i32                          rc;                 // Return code from ICLI calls
    vtss_icfg_query_result_buf_t *buf = res->head;   // Current result buffer block
    i32                          cmd_mode_level;     // ICLI command mode level

    /* Enter global config mode */

    if ((cmd_mode_level=icli_session_mode_enter(session_id, ICLI_CMD_MODE_GLOBAL_CONFIG)) < 0) {
        ICLI_PRINTF("%% Failed to enter configuration mode; configuration aborted.\n");
        T_E("Cannot enter config mode; rc=%ld", cmd_mode_level);
        return;
    }

    // We need to feed ICLI one command line at a time. We recognize CR, LF,
    // CR+LF, LF+CR as one end-of-line sequence.

    pcmd = cmd;
    while (buf != NULL  &&  buf->used > 0) {
        len = buf->used;
        p   = buf->text;
        while (len > 0) {
            if ( (len > 1)  &&  ((*p == '\n'  &&  *(p+1) == '\r')  ||  (*p == '\r'  &&  *(p+1) == '\n')) ) {
                p++;
                len--;
            }
            if (*p == '\n'  ||  *p == '\r') {
                line_cnt++;
            }
            else {
                *pcmd++ = *p;
            }
            if (*p == '\n'  ||  *p == '\r'  ||  (pcmd == cmd + N - 1)) {
                *pcmd = '\0';
                pcmd = cmd;
                if (is_end(cmd)) {
                    goto done;
                }
                icfg_commit_one_line_to_icli(cmd, source_name, line_cnt, &err_cnt);
            }
            p++;
            len--;
        }
        buf = buf->next;
    }

    // Handle case where last line isn't newline terminated
    if (pcmd != cmd) {
        *pcmd = '\0';
        line_cnt++;
        if (! is_end(cmd)) {
            icfg_commit_one_line_to_icli(cmd, source_name, line_cnt, &err_cnt);
        }
    }

done:
    while (((rc=icli_session_mode_exit(session_id)) != (cmd_mode_level - 1))  &&  (rc >= 0)) {
        /* pop levels off until we're back where we came from */
    }
    if (rc < 0) {
        T_E("Cannot exit command mode; rc=%ld\n", rc);
    }
#undef N
}



// Load and apply boot configuration file
static BOOL icfg_load_and_apply(const char *filename)
{
    struct stat              sbuf;
    ssize_t                  bytes_read;
    int                      fd;
    vtss_icfg_query_result_t res = { NULL, NULL };
    BOOL                     rc = FALSE;

    // Determine size of file, load it into freshly-allocated temp. buffer

    if (stat(filename, &sbuf) < 0) {
        T_D("Cannot retrieve info for %s: %s\n", filename, strerror(errno));
        goto out;
    }

    if (vtss_icfg_init_query_result(sbuf.st_size, &res) != VTSS_RC_OK) {
        T_E("Not enough free RAM to load file %s (%lu bytes)\n", filename, sbuf.st_size);
        goto out;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        T_E("Boot configuration: Cannot open flash:%s (%s)", filename, strerror(errno));
        goto out;
    }

    bytes_read = read(fd, res.head->text, sbuf.st_size);
    if (bytes_read < 0) {
        T_E("Cannot read from %s: %s\n", filename, strerror(errno));
        goto out;
    }
    if (bytes_read < sbuf.st_size) {
        T_E("Short read from %s, %lu bytes out of %lu: %s\n",
            filename, bytes_read, sbuf.st_size, strerror(errno));
        goto out;
    }
    res.head->used = bytes_read;

    close(fd);

    icfg_commit_to_icli(filename, &res);
    rc = TRUE;

out:
    vtss_icfg_free_query_result(&res);
    return rc;
}
#endif


// ICFG module init

vtss_rc icfg_init(vtss_init_data_t *data)
{
#if 0
    // Is this too fragile?
    static BOOL master_has_applied_startup_config = FALSE;
#endif

    switch (data->cmd) {
    case INIT_CMD_INIT:
        /* Initialize and register trace resources */
        VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&trace_reg);
        vtss_ecos_mutex_init(&icfg_io_mutex);
        critd_init(&icfg_crit, "icfg_crit", VTSS_MODULE_ID_ICFG, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        ICFG_CRIT_EXIT();
        break;

    case INIT_CMD_CONF_DEF:
#if 0
        // This is too simple; we need to know when modules are ready after
        // their load-default
        (void)icfg_load_and_apply("default-config");
#endif
        break;

    case INIT_CMD_MASTER_UP:
#if 0
        // This is also too simple. In stacking setups, we need to know when the
        // entire stack has been detected, and we need to know that the modules
        // are in fully initialized state
        if (!master_has_applied_startup_config) {
            master_has_applied_startup_config = TRUE;
            T_D("Master up: Attempting to load startup-config, fallback to default-config");
            if (!icfg_load_and_apply("startup-config")) {
                (void)icfg_load_and_apply("default-config");
            }
        }
#endif
        break;

    case INIT_CMD_MASTER_DOWN:
#if 0
        T_D("Becoming slave: Assuming master has applied/will apply boot config");
        master_has_applied_startup_config = TRUE;
#endif
        break;

    default:
        break;
    }

    return VTSS_OK;
}



// Mutex


BOOL vtss_icfg_try_lock_io_mutex(void)
{
    return VTSS_ECOS_MUTEX_TRYLOCK(&icfg_io_mutex);
}

void vtss_icfg_unlock_io_mutex(void)
{
    VTSS_ECOS_MUTEX_UNLOCK(&icfg_io_mutex);
}
