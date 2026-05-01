/*

   Vitesse Switch API software.

 Copyright (c) 2002-2011 Vitesse Semiconductor Corporation "Vitesse". All
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

#include "web_api.h"
#include "dhcp_snooping_api.h"
#include "port_api.h"
#include "msg_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static cyg_int32 handler_config_dhcp_snooping(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                 sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t                 pit;
    int                         ct;
    dhcp_snooping_conf_t        conf, newconf;
    dhcp_snooping_port_conf_t   port_conf, port_newconf;
    int                         var_value;
    char                        var_name[32];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return -1;
    }
#endif

    (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        if (dhcp_snooping_mgmt_conf_get(&conf) != VTSS_OK ||
            dhcp_snooping_mgmt_port_conf_get(sid, &port_conf) != VTSS_OK) {
            redirect(p, "/dhcp_snooping.htm");
            return -1;
        }
        newconf = conf;

        /* snooping_mode */
        if (cyg_httpd_form_varable_int(p, "snooping_mode", &var_value)) {
            newconf.snooping_mode = var_value;
        }

        if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
            T_D("Calling dhcp_snooping_mgmt_conf_set()");
            if (dhcp_snooping_mgmt_conf_set(&newconf) < 0) {
                T_E("dhcp_snooping_mgmt_conf_set(): failed");
            }
        }

        /* store form data */
        port_newconf = port_conf;

        while (port_iter_getnext(&pit)) {
            /* port_mode */
            sprintf(var_name, "port_mode_%u", pit.uport);
            if (cyg_httpd_form_varable_int(p, var_name, &var_value)) {
                port_newconf.port_mode[pit.iport] = var_value;
            }
        }

        if (memcmp(&port_newconf, &port_conf, sizeof(port_newconf)) != 0) {
            T_D("Calling dhcp_snooping_mgmt_port_conf_set()");
            if (dhcp_snooping_mgmt_port_conf_set(sid, &port_newconf) < 0) {
                T_E("dhcp_snooping_mgmt_port_conf_set(): failed");
            }
        }
        redirect(p, "/dhcp_snooping.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        /* get form data
           Format: [snooping_mode],[uport]/[port_mode]|...
        */
        if (dhcp_snooping_mgmt_conf_get(&conf) == VTSS_OK &&
            dhcp_snooping_mgmt_port_conf_get(sid, &port_conf) == VTSS_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", conf.snooping_mode);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            while (port_iter_getnext(&pit)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%d|", pit.uport, port_conf.port_mode[pit.iport]);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static cyg_int32 handler_stat_dhcp_snooping_statistics(CYG_HTTPD_STATE *p)
{
    vtss_isid_t             sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_port_no_t          iport;
    dhcp_snooping_stats_t   stats;
    int                     ct;
    const char              *var_string;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    if ((var_string = cyg_httpd_form_varable_find(p, "port")) != NULL) {
        iport = uport2iport(atoi(var_string));
    } else {
        goto out;
    }

    if (!VTSS_ISID_LEGAL(sid) || !msg_switch_exists(sid)) {
        goto out;    /* Most likely stack error - bail out */
    }

    if ((cyg_httpd_form_varable_find(p, "clear") != NULL)) { /* Clear? */
        if (dhcp_snooping_stats_clear(sid, iport)) {
            goto out;
        }
    }

    if (dhcp_snooping_stats_get(sid, iport, &stats)) {
        goto out;
    }

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u",
                  iport2uport(iport),
                  stats.discover_rx,        stats.discover_tx,
                  stats.offer_rx,           stats.offer_tx,
                  stats.request_rx,         stats.request_tx,
                  stats.decline_rx,         stats.decline_tx,
                  stats.ack_rx,             stats.ack_tx,
                  stats.nak_rx,             stats.nak_tx,
                  stats.release_rx,         stats.release_tx,
                  stats.inform_rx,          stats.inform_tx,
                  stats.leasequery_rx,      stats.leasequery_tx,
                  stats.leaseunassigned_rx, stats.leaseunassigned_tx,
                  stats.leaseunknown_rx,    stats.leaseunknown_tx,
                  stats.leaseactive_rx,     stats.leaseactive_tx,
                  stats.discard_untrust_rx);
    cyg_httpd_write_chunked(p->outbuffer, ct);

out:
    cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_dhcp_snooping, "/config/dhcp_snooping", handler_config_dhcp_snooping);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_dhcp_snooping, "/stat/dhcp_snooping_statistics", handler_stat_dhcp_snooping_statistics);

/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
