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
#include "conf_xml_api.h"
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

// Configuration Load handler
static cyg_int32 handler_conf_load(CYG_HTTPD_STATE *p)
{
    conf_xml_set_t set;

    form_data_t file;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        // Get configuration data
        file.value_len = 0;
        if (cyg_httpd_form_parse_formdata(p, &file, 1) && file.value_len) {
            char *buffer;
            /* Malloc, since we donøow the size of the configuration */
            if ((buffer = malloc(file.value_len + 1))) {
                memcpy(buffer, file.value, file.value_len);
                set.data = buffer;
                set.size = file.value_len;
                set.apply = 1;
                // Check if configuration is valid and then apply
                if (conf_xml_file_set(&set) == VTSS_OK) {
                    send_custom_error(p, "Configuration upload done", " ", 1);
                } else {
                    // Configuration isn't valid. Give error message with pointer to line in invalid cofiguration
                    char err_msg[2000] ;
                    (void) snprintf(err_msg, sizeof(err_msg), "%s \n <xmp>Line %u:%s \n</xmp>", set.msg, set.line_no, set.line);
                    send_custom_error(p, "Configuration Upload Error", &err_msg[0], strlen(err_msg));
                }
                free(buffer);

            } else {
                send_custom_error(p, "Allocation of configuration buffer failed", " ", 1);
            }
        } else if (file.value_len == 0) {
            // No configuration is receive.
            redirect(p, "/conf_xml_invalid.htm");
        } else {
            cyg_httpd_send_error(CYG_HTTPD_STATUS_BAD_REQUEST);
        }
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

// Configuration Save handler
static cyg_int32 handler_conf_save(CYG_HTTPD_STATE *p)
{
    char            *data;
    conf_xml_get_t  get;
    ulong           size = CONF_XML_FILE_MAX;

    T_D("Entering handler_conf_save");

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        if ((data = malloc(size)) == NULL) {
            send_custom_error(p, "Allocation of configuration buffer failed", " ", 1);
            return -1;
        }

        get.data = data;
        get.size = size;

        // get the configuration
        if (conf_xml_file_get(&get) != VTSS_OK) {
            send_custom_error(p, "Failed to get switch configuration", " ", 1);
            return -1;
        } else {
            T_D("Setting file name to config.xml, size = %u", get.size);
            T_N("Data = %s", get.data);

            // Send configurtion data to the browser as a file.
            cyg_httpd_ires_table_entry entry;
            entry.f_pname = "config.xml";
            entry.f_ptr  = (unsigned char *)get.data;
            entry.f_size = get.size;
            cyg_httpd_send_content_disposition(&entry);
        }

        free(data);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_conf_load, "/config/conf_load", handler_conf_load);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_conf_save, "/config/conf_save", handler_conf_save);

/****************************************************************************/
/*  End of file.                                                            */
/****************************************************************************/
