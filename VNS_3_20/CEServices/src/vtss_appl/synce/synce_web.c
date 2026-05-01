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
#include "synce.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define SYNCE_WEB_BUF_LEN 512

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
//
// SYNCE handler
//

static char *synce_error_txt(u32 error)
{
    switch (error)
    {
        case SYNCE_RC_OK:                return("SYNCE_RC_OK");
        case SYNCE_RC_INVALID_PARAMETER: return("SYNCE_RC_INVALID_PARAMETER");
        case SYNCE_RC_NOM_PORT:          return("SYNCE_RC_NOM_PORT");
        case SYNCE_RC_SELECTION:         return("SYNCE_RC_SELECTION");
        case SYNCE_RC_INVALID_PORT:      return("SYNCE_RC_INVALID_PORT");
        default:                         return("SYNCE_RC_INVALID_PARAMETER");
    }
}

static cyg_int32 handler_config_synce(CYG_HTTPD_STATE* p)
{
    vtss_isid_t                     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_port_no_t                  iport;
    vtss_rc                         rc;
    char                            var_rate[32];
    synce_mgmt_clock_conf_blk_t     clock_config;
    synce_mgmt_port_conf_blk_t      port_config;
    synce_mgmt_selector_state_t     selector_state;
    synce_mgmt_alarm_state_t        alarm_state;
    synce_mgmt_quality_level_t      ssm_rx;
    synce_mgmt_quality_level_t      ssm_tx;
    BOOL                            master;
//    const BOOL                      (*src_port)[SYNCE_PORT_COUNT];
    synce_src_port                  src_port;
    uint                            i, port_cnt, source, nominated, enabled, uport, priority, mode, sel_source, overwrite, holdover, freerun, wtr, aneg, hold;
    int                             ct;
    static vtss_rc                  nom_error = VTSS_OK, prio_error = VTSS_OK, selection_error = VTSS_OK ; // Used to select an error message to be given back to the web page -- 0 = no error
    char                            data_string[2000];

    if(redirectUnmanagedOrInvalid(p, sid)) /* Redirect unmanaged/invalid access to handler */
        return -1;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYNCE))
        return -1;
#endif

    //
    // Setting new configuration
    //
    if(p->method == CYG_HTTPD_METHOD_POST)
    {
        for (source=0; source<SYNCE_NOMINATED_MAX; source++)
        {
            sprintf(var_rate, "wtrC_%d", source+1);
            if (cyg_httpd_form_varable_int(p, var_rate, &enabled))
                if (enabled != 0)    synce_mgmt_wtr_clear_set(source);

            sprintf(var_rate, "nom_%d", source+1);
            nominated = 0;
            if (cyg_httpd_form_varable_find(p, var_rate)) nominated = 1;

            sprintf(var_rate, "port_%d", source+1);
            if (cyg_httpd_form_varable_int(p, var_rate, &uport))
            {
                sprintf(var_rate, "pri_%d", source+1);
                if (cyg_httpd_form_varable_int(p, var_rate, &priority))
                {
                    sprintf(var_rate, "ssmO_%d", source+1);
                    if (cyg_httpd_form_varable_int(p, var_rate, &overwrite))
                    {
                        sprintf(var_rate, "aneg_%d", source+1);
                        if (cyg_httpd_form_varable_int(p, var_rate, &aneg))
                        {
                            sprintf(var_rate, "hold_%d", source+1);
                            if (cyg_httpd_form_varable_int(p, var_rate, &hold))
                            {
                                if ((rc = synce_mgmt_nominated_source_set(source,
                                                                        nominated,
                                                                        uport2iport(uport),
                                                                        aneg,
                                                                        hold,
                                                                        (synce_mgmt_quality_level_t)overwrite)) != VTSS_OK)
                                    if (nom_error == VTSS_OK) nom_error = rc;
                                if ((rc = synce_mgmt_nominated_priority_set(source, priority)) != VTSS_OK)
                                    if (prio_error == VTSS_OK) prio_error = rc;
                            }
                        }
                    }
                }
            }
        }

        sprintf(var_rate, "selM");
        if (cyg_httpd_form_varable_int(p, var_rate, &mode))
        {
            sprintf(var_rate, "selS");
            if (cyg_httpd_form_varable_int(p, var_rate, &source))
            {
                sprintf(var_rate, "wtrT");
                if (cyg_httpd_form_varable_int(p, var_rate, &wtr))
                {
                    sprintf(var_rate, "ssmHo");
                    if (cyg_httpd_form_varable_int(p, var_rate, &holdover))
                    {
                        sprintf(var_rate, "ssmFr");
                        if (cyg_httpd_form_varable_int(p, var_rate, &freerun))
                        {
                            if ((rc = synce_mgmt_nominated_selection_mode_set((synce_mgmt_selection_mode_t)mode,
                                                                              source-1,
                                                                              wtr,
                                                                              (synce_mgmt_quality_level_t)holdover,
                                                                              (synce_mgmt_quality_level_t)freerun)) != VTSS_OK)
                            if (selection_error == VTSS_OK) selection_error = rc;
                        }
                    }
                }
            }
        }

        for (iport = 0; iport < SYNCE_PORT_COUNT; iport++)
        {
            sprintf(var_rate, "ssmE_%u", iport2uport(iport));
            enabled = 0;
            if (cyg_httpd_form_varable_find(p, var_rate))
              enabled = 1;
            synce_mgmt_ssm_set(iport, enabled);
        }

        redirect(p, "/synce_config.htm");
    }
    else
    {
        rc = synce_mgmt_clock_config_get(&clock_config);
        rc = synce_mgmt_clock_state_get(&selector_state, &sel_source, &alarm_state);
        src_port = synce_mgmt_source_port_get();

        cyg_httpd_start_chunked("html");

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u|",
                      SYNCE_NOMINATED_MAX);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        for (source = 0; source<SYNCE_NOMINATED_MAX; source++)
        {
            /* Count how many ports can be selected for this clk source */
            for (i=0, port_cnt=0; i<SYNCE_PORT_COUNT; ++i)   if (src_port[source][i])    port_cnt++;

            data_string[0] = '\0';
            sprintf(data_string,"%u/", port_cnt);

            /* The possible selected ports for this source is written to the buffer */
            for (i=0; i<SYNCE_PORT_COUNT; ++i)   if (src_port[source][i])    sprintf(data_string,"%s%u/", data_string, iport2uport(i));
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", data_string);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            /* Rest of the information for this cource is written to the buffer */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u/%u/%u/%s/%s/%s/%s/%u|",
                          source+1,
                          clock_config.nominated[source],
                          iport2uport(clock_config.port[source]),
                          clock_config.priority[source],
                          clock_config.ssm_overwrite[source],
                          clock_config.holdoff_time[source],
                          (uint)clock_config.aneg_mode[source],
                          alarm_state.locs[source] ? "Down" : "Up",
                          alarm_state.fos[source] ? "Down" : "Up",
                          alarm_state.ssm[source] ? "Down" : "Up",
                          alarm_state.wtr[source] ? "Down" : "Up",
                          0);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|",
                      synce_error_txt((nom_error != VTSS_OK) ? nom_error : prio_error));
        cyg_httpd_write_chunked(p->outbuffer, ct);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u/%u/%u/%s/%s/%s|%s|",
                      (uint)(clock_config.selection_mode),
                      clock_config.source+1,
                      clock_config.wtr_time,
                      clock_config.ssm_holdover,
                      clock_config.ssm_freerun,
                      (uint)selector_state,
                      sel_source+1,
                      alarm_state.losx ? "Down" : "Up",
                      alarm_state.lol ? "Down" : "Up",
                      alarm_state.dhold ? "Down" : "Up",
                      synce_error_txt(selection_error));
        cyg_httpd_write_chunked(p->outbuffer, ct);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u|", SYNCE_PORT_COUNT);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        rc = synce_mgmt_port_config_get(&port_config);

        for (iport = 0; iport < SYNCE_PORT_COUNT; iport++)
        {
            rc = synce_mgmt_port_state_get(iport, &ssm_rx, &ssm_tx, &master);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u|",
                          iport2uport(iport),
                          port_config.ssm_enabled[iport],
                          ssm_tx,
                          ssm_rx,
                          master);
//printf("p->outbuffer = %s", p->outbuffer);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
//printf("\n");

        nom_error = VTSS_OK;
        prio_error = VTSS_OK;
        selection_error = VTSS_OK;

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

// Status
static cyg_int32 handler_status_synce(CYG_HTTPD_STATE* p)
{
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYNCE))
        return -1;
#endif

    if(p->method == CYG_HTTPD_METHOD_GET) {
        cyg_httpd_start_chunked("html");
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t synce_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[SYNCE_WEB_BUF_LEN];

    (void) snprintf(buff, SYNCE_WEB_BUF_LEN, 
                    "var configSyncePrioMax = %u;\n"
                    "var configSynceNormMax = %u;\n", 
                    SYNCE_PRIORITY_MAX, 
                    SYNCE_NOMINATED_MAX);

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(synce_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_synce, "/config/synceConfig", handler_config_synce);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_status_synce, "/stat/synce_status", handler_status_synce);

/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
