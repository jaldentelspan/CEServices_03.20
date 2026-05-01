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

#ifdef VTSS_SW_OPTION_CONF_XML
#include "main.h"
#include "thermal_protect_api.h"
#include "conf_xml_api.h"
#include "conf_xml_trace_def.h"
#include "mgmt_api.h"
#include "misc_api.h"
/* Tag IDs */
enum {
    /* Module tags */
    CX_TAG_THERMAL_PROTECT,

    /* Group tags */
    CX_TAG_PRIORITY_TABLE,

    CX_TAG_ENTRY,


    /* Last entry */
    CX_TAG_NONE
};

/* Tag table */
static cx_tag_entry_t thermal_protect_cx_tag_table[CX_TAG_NONE + 1] = {
    [CX_TAG_THERMAL_PROTECT] = {
        .name  = "THERMAL_PROTECT",
        .descr = "Energy Efficient Ethernet",
        .type = CX_TAG_TYPE_MODULE
    },
    [CX_TAG_ENTRY] = {
        .name  = "entry",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },

    [CX_TAG_PRIORITY_TABLE] = {
        .name  = "priority_table",
        .descr = "",
        .type = CX_TAG_TYPE_GROUP
    },

    /* Last entry */
    [CX_TAG_NONE] = {
        .name  = "",
        .descr = "",
        .type = CX_TAG_TYPE_NONE
    }
};


/* THERMAL_PROTECT specific set state structure */
typedef struct {
    cx_line_t  line_thermal_protect;  /* THERMAL_PROTECT line */
} thermal_protect_cx_set_state_t;


static BOOL cx_thermal_protect_match(const cx_table_context_t *context, ulong prio_a, ulong prio_b)
{
    thermal_protect_local_conf_t conf;
    thermal_protect_mgmt_get_switch_conf(&conf, context->isid);
    T_IG(VTSS_TRACE_GRP_THERMAL_PROTECT, "conf.glbl_conf.prio_temperatures[%u]:%d, conf.glbl_conf.prio_temperatures[%u]:%d", prio_a, conf.glbl_conf.prio_temperatures[prio_a], prio_b, conf.glbl_conf.prio_temperatures[prio_b]);
    return conf.glbl_conf.prio_temperatures[prio_a]  == conf.glbl_conf.prio_temperatures[prio_b];
}

static vtss_rc cx_thermal_protect_print(cx_get_state_t *s, const cx_table_context_t *context, ulong prio, char *txt)
{
    thermal_protect_local_conf_t conf;
    char buf[80];



    if (txt == NULL) {
        // Syntax
        CX_RC(cx_add_stx_start(s));
        CX_RC(cx_add_attr_txt(s, "prio", cx_list_txt(buf, THERMAL_PROTECT_PRIOS_MIN, THERMAL_PROTECT_PRIOS_MAX)));
        CX_RC(cx_add_stx_ulong(s, "temperature", THERMAL_PROTECT_TEMP_MIN, THERMAL_PROTECT_TEMP_MAX));
        return cx_add_stx_end(s);
    }


    thermal_protect_mgmt_get_switch_conf(&conf, context->isid);
    T_D("conf.glbl_conf.prio_temperatures[%u] = %d, sid= %d, s.sid = %d", prio, conf.glbl_conf.prio_temperatures[prio], context->isid, s->isid);
    CX_RC(cx_add_attr_start(s, CX_TAG_ENTRY));
    CX_RC(cx_add_attr_txt(s, "prio", txt));
    CX_RC(cx_add_attr_ulong(s, "temperature", conf.glbl_conf.prio_temperatures[prio]));
    CX_RC(cx_add_attr_end(s, CX_TAG_ENTRY));
    return VTSS_OK;
}



static vtss_rc thermal_protect_cx_parse_func(cx_set_state_t *s)
{
    BOOL           global;
    BOOL           priority_list[THERMAL_PROTECT_PRIOS_CNT];
    thermal_protect_local_conf_t conf;
    u16            prio_idx;
    u32            temperature = 0;
    char           buf[64];
    BOOL           found_temperature = FALSE;
    switch (s->cmd) {
    case CX_PARSE_CMD_PARM:
        global = (s->isid == VTSS_ISID_GLOBAL);

        if (s->mod_tag == CX_TAG_THERMAL_PROTECT) {

            if (s->apply) {
                thermal_protect_mgmt_get_switch_conf(&conf, s->isid);
            }

            switch (s->id) {
            case CX_TAG_PRIORITY_TABLE:
                if (!global) {
                    s->ignored = 1;
                }
                break;
            case CX_TAG_ENTRY:
                CX_RC(cx_parse_txt(s, "prio", buf, sizeof(buf)));
                if (global && s->group == CX_TAG_PRIORITY_TABLE &&
                    mgmt_txt2list(buf, &priority_list[0], THERMAL_PROTECT_PRIOS_MIN, THERMAL_PROTECT_PRIOS_MAX, 0) == VTSS_OK) {

                    s->p = s->next;
                    for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                        if (cx_parse_ulong(s, "temperature", &temperature, THERMAL_PROTECT_TEMP_MIN, THERMAL_PROTECT_TEMP_MAX) == VTSS_OK) {
                            found_temperature = TRUE;
                        } else {
                            s->ignored = 1;
                        }
                    }

                    for (prio_idx = 0; prio_idx < THERMAL_PROTECT_PRIOS_CNT; prio_idx++) {
                        if (s->apply && priority_list[prio_idx]) {
                            if (found_temperature) {
                                conf.glbl_conf.prio_temperatures[prio_idx] = (u8) temperature;
                            }
                        }
                    }
                }
                break;

            default:
                s->ignored = 1;
                break;
            }

            if (s->apply) {
                if (global) {
                    if (thermal_protect_mgmt_set_switch_conf(&conf, s->isid) != VTSS_OK) {
                        T_W("Could not set THERMAL_PROTECT configuration");
                    }
                }
            }
            break;
        } // CX_PARSE_CMD_PARM
    default:
        break;
    }
    return s->rc;
//    return VTSS_OK;
}

static BOOL cx_skip_none(ulong n)
{
    return 0;
}

static vtss_rc thermal_protect_cx_gen_func(cx_get_state_t *s)
{
    cx_table_context_t context;

    switch (s->cmd) {
    case CX_GEN_CMD_GLOBAL:
        /* Global - THERMAL_PROTECT */
        CX_RC(cx_add_tag_line(s, CX_TAG_THERMAL_PROTECT, 0));

        CX_RC(cx_add_tag_line(s, CX_TAG_PRIORITY_TABLE, 0));

        context.isid = VTSS_ISID_LOCAL;
        CX_RC(cx_add_table(s, &context,
                           THERMAL_PROTECT_PRIOS_MIN, THERMAL_PROTECT_PRIOS_MAX,
                           cx_skip_none, cx_thermal_protect_match, cx_thermal_protect_print, FALSE));
        CX_RC(cx_add_tag_line(s, CX_TAG_PRIORITY_TABLE, 1));
        CX_RC(cx_add_tag_line(s, CX_TAG_THERMAL_PROTECT, 1));

        break;
    case CX_GEN_CMD_SWITCH:
        /* Switch - THERMAL_PROTECT */
        break;
    default:
        T_E("Unknown command");
        return VTSS_RC_ERROR;
    } /* End of Switch */

    return VTSS_OK;
}

/* Register the info in to the cx_module_table */
CX_MODULE_TAB_ENTRY(
    VTSS_MODULE_ID_THERMAL_PROTECT,
    thermal_protect_cx_tag_table,
    sizeof(thermal_protect_cx_set_state_t),
    0,
    NULL,                  /* Init function       */
    thermal_protect_cx_gen_func,       /* Generation function */
    thermal_protect_cx_parse_func      /* Parse function      */
);
#endif // VTSS_SW_OPTION_CONF_XML
