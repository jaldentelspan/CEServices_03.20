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

#include "main.h"
#include "conf_xml_api.h"
#include "conf_xml_trace_def.h"
#include "conf_xml_custom.h"
#include "conf_api.h"
#include "msg_api.h"
#include "misc_api.h"
#include "mgmt_api.h"
#include "port_api.h" /* Reqd for VTSS_FRONT_PORT_COUNT */
#include "critd_api.h"
#include "sys/time.h"

#if VTSS_SWITCH_STACKABLE
#include "topo_api.h"
#endif

#ifdef VTSS_SW_OPTION_VCLI
#include "conf_xml_cli.h"
#endif

#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"
#ifdef VTSS_SW_OPTION_LACP
#include "lacp_api.h"
#endif /* VTSS_SW_OPTION_LACP */
#include "aggr_xml.h"
#endif

#ifdef VTSS_SW_OPTION_DOT1X
#include "vtss_nas_api.h"
#include "dot1x_api.h"
#include "dot1x_xml.h"
#endif

#ifdef VTSS_SW_OPTION_MSTP
#include "mstp_api.h"
#if defined(VTSS_SW_OPTION_DOT1X)
#include "mstp_xml.h" /* For mstp_cx_set_state_t */
#endif
#endif

/* XML module table boundaries definition */
/*lint -e{19} ...it's need to declare xml module table boundary,
  so skip lint error */
CYG_HAL_TABLE_BEGIN(cx_mod_tab_start, cx_module_table);
/*lint -e{19} */
CYG_HAL_TABLE_END(cx_mod_tab_end, cx_module_table);
extern struct cx_module_entry_t cx_mod_tab_start[], cx_mod_tab_end;

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/
static critd_t cx_crit;

/* Tag IDs */
enum {
    /* Header tags */
    CX_TAG_HEADER,
    CX_TAG_CONF,

    /* Section tags */
    CX_TAG_PLATFORM,
    CX_TAG_GLOBAL,
    CX_TAG_SWITCH,

    /* Parameter tags */
    CX_TAG_PID,
    CX_TAG_VERSION,

    /* Last entry */
    CX_TAG_NONE
};

/* The local variable don't need to semaphore protected. Lint determine
   it is a "459 error : whose address was taken has an unprotected access
   to variable." That the variable never been modified during the process.
   So we skip it here. */
/*lint -esym(459,cx_tag_table)*/
/* Tag table */
static cx_tag_entry_t cx_tag_table[CX_TAG_NONE + 1] = {
    /* Header tags */
    [CX_TAG_HEADER] = {
        .name  = "?xml version=\"1.0\"?",
        .descr = "",
        .type = CX_TAG_TYPE_HEADER
    },
    [CX_TAG_CONF] = {
        .name  = "configuration",
        .descr = "",
        .type = CX_TAG_TYPE_HEADER
    },

    /* Section tags */
    [CX_TAG_PLATFORM] = {
        .name  = "platform",
        .descr = "Platform identification",
        .type = CX_TAG_TYPE_SECTION
    },
    [CX_TAG_GLOBAL] = {
        .name  = "global",
        .descr = "Global configuration",
        .type = CX_TAG_TYPE_SECTION
    },
    [CX_TAG_SWITCH] = {
        .name  = "switch",
        .descr = "",
        .type = CX_TAG_TYPE_SECTION
    },


    /* Parameter tags */
    [CX_TAG_PID] = {
        .name  = "pid",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_VERSION] = {
        .name  = "version",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },


    /* Last entry */
    [CX_TAG_NONE] = {
        .name  = "",
        .descr = "",
        .type = CX_TAG_TYPE_NONE
    }
};

#if (VTSS_TRACE_ENABLED)
static vtss_trace_reg_t trace_reg = {
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "conf_xml",
    .descr     = "Config XML module"
};

static vtss_trace_grp_t trace_grps[TRACE_GRP_CNT] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        .name      = "default",
        .descr     = "Default",
        .lvl       = VTSS_TRACE_LVL_ERROR,
        .timestamp = 1,
    },
    [VTSS_TRACE_GRP_MIRROR] = {
        .name      = "mirror",
        .descr     = "Mirror",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [VTSS_TRACE_GRP_POE] = {
        .name      = "poe",
        .descr     = "PoE",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_SYNCE] = {
        .name      = "synce",
        .descr     = "synce",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_EPS] = {
        .name      = "eps",
        .descr     = "eps",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_MEP] = {
        .name      = "mep",
        .descr     = "mep",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_VLAN] = {
        .name      = "VLAN",
        .descr     = "vlan",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_AUTH] = {
        .name      = "auth",
        .descr     = "auth",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_LLDP] = {
        .name      = "lldp",
        .descr     = "LLDP",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_EEE] = {
        .name      = "eee",
        .descr     = "EEE",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_LED_POW_REDUC] = {
        .name      = "led_pow",
        .descr     = "Led power reduction",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_THERMAL_PROTECT] = {
        .name      = "thermal",
        .descr     = "Thermal Protection",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },

    [VTSS_TRACE_GRP_QOS] = {
        .name      = "qos",
        .descr     = "qos",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [VTSS_TRACE_GRP_CRIT] = {
        .name      = "crit",
        .descr     = "Critical regions",
        .lvl       = VTSS_TRACE_LVL_ERROR,
        .timestamp = 1,
    },
};
#define CX_CRIT_ENTER() critd_enter(&cx_crit, VTSS_TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#define CX_CRIT_EXIT()  critd_exit( &cx_crit, VTSS_TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__)
#else
#define CX_CRIT_ENTER() critd_enter(&cx_crit)
#define CX_CRIT_EXIT()  critd_exit( &cx_crit)
#endif /* VTSS_TRACE_ENABLED */

static const cx_kw_t cx_kw_bool[] = {
    { "enabled",  1 },
    { "disabled", 0 },
    { NULL,       0 }
};

#define CX_MOD_STATE_SET(s, tab) (s)->mod_state = ((tab)->size ? (char *)(s) + sizeof(cx_set_state_t) + (tab)->offset : NULL)

#define CX_PLATFORM_VERSION 1

/****************************************************************************/
/*  Local XML build functions                                               */
/****************************************************************************/
vtss_rc cx_size_check(cx_get_state_t *s)
{
    /* Check that there is room for more data */
    if ((s->p + 10240) > (s->file + s->size)) {
        if (!s->error) {
            T_E("file size exceeding");
        }
        s->error = 1;
        return CONF_XML_ERROR_GEN;
    }
    return VTSS_OK;
}

/* Increase or decrease indentation level */
static void cx_indent_set(cx_get_state_t *s, BOOL add)
{
    if (add) {
        s->indent += 2;
    } else {
        s->indent -= 2;
    }
}

/* Add current indentation level */
static vtss_rc cx_indent_add(cx_get_state_t *s)
{
    memset(s->p, ' ', s->indent);
    s->p += s->indent;

    return cx_size_check(s);
}

/* Print tag to buffer */
static int cx_sprint_tag(char *p, cx_tag_entry_t *tab,
                         cx_tag_id_t id, BOOL end, BOOL nl)
{
    return sprintf(p, "<%s%s>%s%s",
                   end ? "/" : "",
                   tab[id].name,
                   end ? "" : tab[id].descr,
                   nl ? "\n" : "");
}

/* Add tag */
static vtss_rc cx_add_tag(cx_get_state_t *s, cx_tag_id_t id, BOOL end, BOOL nl)
{
    s->p += cx_sprint_tag(s->p, s->tag, id, end, nl);
    return cx_size_check(s);
}

/* Add tag line */
vtss_rc cx_add_tag_line(cx_get_state_t *s, cx_tag_id_t id, BOOL end)
{
    /* Decrease indentation if end tag */
    if (end) {
        cx_indent_set(s, 0);
    }

    CX_RC(cx_indent_add(s));
    CX_RC(cx_add_tag(s, id, end, 1));

    /* Increase indentation if start tag */
    if (!end) {
        cx_indent_set(s, 1);
    }

    return cx_size_check(s);
}

/* Add attribute start */
vtss_rc cx_add_attr_start(cx_get_state_t *s, cx_tag_id_t id)
{
    CX_RC(cx_indent_add(s));
    CX_RC(cx_add_tag(s, id, 0, 0));
    s->p--; /* Remove trailing '>' */

    return cx_size_check(s);
}

/* Add text attribute */
vtss_rc cx_add_attr_txt(cx_get_state_t *s, const char *name, const char *txt)
{
    s->p += sprintf(s->p, " %s=\"%s\"", name, txt);
    return cx_size_check(s);
}

/* Add text */
static vtss_rc cx_add_txt(cx_get_state_t *s, const char *txt)
{
    s->p += sprintf(s->p, "%s", txt);
    return cx_size_check(s);
}

/* Add comment start */
static vtss_rc cx_add_comment_start(cx_get_state_t *s)
{
    CX_RC(cx_indent_add(s));
    return cx_add_txt(s, "<!-- ");
}

/* Add syntax start */
vtss_rc cx_add_stx_start(cx_get_state_t *s)
{
    CX_RC(cx_add_comment_start(s));
    return cx_add_txt(s, "Syntax:");
}

/* Add syntax/comment end */
vtss_rc cx_add_stx_end(cx_get_state_t *s)
{
    return cx_add_txt(s, " -->\n");
}

/* Add comment */
vtss_rc cx_add_comment(cx_get_state_t *s, const char *txt)
{
    CX_RC(cx_add_comment_start(s));
    CX_RC(cx_add_txt(s, txt));
    return cx_add_stx_end(s);
}

/* Add syntax text */
vtss_rc cx_add_stx_txt(cx_get_state_t *s, const char *txt)
{
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_attr_txt(s, "val", txt));
    CX_RC(cx_add_stx_end(s));
    return cx_size_check(s);
}

/* Add syntax IPv4 */
vtss_rc cx_add_stx_ipv4(cx_get_state_t *s, const char *name)
{
    return cx_add_attr_txt(s, name, "any or a.b.c.d/n");
}

/* Add syntax IPv6 */
vtss_rc cx_add_stx_ipv6(cx_get_state_t *s, const char *name)
{
    return cx_add_attr_txt(s, name, "any or xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx");
}

/* Add syntax flag */
vtss_rc cx_add_stx_flag(cx_get_state_t *s, const char *name)
{
    char buf[32];

    sprintf(buf, "flag_%s", name);
    return cx_add_attr_txt(s, buf, "any/0/1");
}

/* Add hex attribute */
vtss_rc cx_add_attr_hex(cx_get_state_t *s, const char *name, ulong val)
{
    char buf[16];

    sprintf(buf, "0x%x", val);
    return cx_add_attr_txt(s, name, buf);
}

/* Add ulong attribute */
vtss_rc cx_add_attr_ulong(cx_get_state_t *s, const char *name, ulong val)
{
    char buf[16];

    sprintf(buf, "%u", val);
    return cx_add_attr_txt(s, name, buf);
}

/* Add ulong/word attribute */
vtss_rc cx_add_attr_ulong_word(cx_get_state_t *s, const char *name, ulong val,
                               const char *word, ulong match)
{
    return (val == match ? cx_add_attr_txt(s, name, word) :
            cx_add_attr_ulong(s, name, val));
}

/* Add long attribute */
vtss_rc cx_add_attr_long(cx_get_state_t *s, const char *name, long val)
{
    char buf[16];

    sprintf(buf, "%ld", val);
    return cx_add_attr_txt(s, name, buf);
}

/* Add keyword attribute */
vtss_rc cx_add_attr_kw(cx_get_state_t *s, const char *name,
                       const cx_kw_t *keyword, ulong val)
{
    for ( ; keyword->name != NULL; keyword++)
        if (keyword->val == val) {
            break;
        }
    return cx_add_attr_txt(s, name,
                           keyword->name == NULL ? "?" : keyword->name);
}

/* Add bool attribute */
vtss_rc cx_add_attr_bool(cx_get_state_t *s, const char *name, BOOL enable)
{
    return cx_add_attr_kw(s, name, cx_kw_bool, enable ? 1 : 0);
}

/* Add keyword syntax */
vtss_rc cx_add_stx_kw(cx_get_state_t *s, const char *name, const cx_kw_t *keyword)
{
    char buf[256], *p;
    BOOL first = 1;
    int  cnt_left = sizeof(buf) - 1, cnt;

    p = &buf[0];
    for (; keyword->name != NULL; keyword++) {
        if ((int)strlen(keyword->name) + 1 > cnt_left) { // '+1' == '/'
            T_E("Internal error: Buffer too small to hold keyword: %s", keyword->name);
            s->error = 1;
            return CONF_XML_ERROR_GEN;
        }
        cnt = snprintf(p, cnt_left, "%s%s", first ? "" : "/", keyword->name);
        cnt_left -= cnt;
        p += cnt;
        first = 0;
    }
    return cx_add_attr_txt(s, name, buf);
}

/* Add bool syntax */
vtss_rc cx_add_stx_bool(cx_get_state_t *s, const char *name)
{
    return cx_add_stx_kw(s, name, cx_kw_bool);
}

/* Add ulong syntax */
vtss_rc cx_add_stx_ulong(cx_get_state_t *s, const char *name, ulong min, ulong max)
{
    char buf[32];
    sprintf(buf, "%u-%u", min, max);
    return cx_add_attr_txt(s, name, buf);
}

/* Add ulong/word syntax */
vtss_rc cx_add_stx_ulong_word(cx_get_state_t *s, const char *name,
                              const char *word, ulong min, ulong max)
{
    char buf[64];
    sprintf(buf, "%s or %u-%u", word, min, max);
    return cx_add_attr_txt(s, name, buf);
}


char *cx_list_txt(char *buf, ulong min, ulong max)
{
    sprintf(buf, "%u-%u (list allowed, e.g. 1,3-5)", min, max);
    return buf;
}

/* Add port syntax */
vtss_rc cx_add_stx_port(cx_get_state_t *s)
{
    char buf[80];

    return cx_add_attr_txt(s, "port", cx_list_txt(buf, 1, s->port_count));
}

/* Add IP attribute */
vtss_rc cx_add_attr_ipv4(cx_get_state_t *s, const char *name, vtss_ipv4_t ipv4)
{
    char buf[16];

    return cx_add_attr_txt(s, name, misc_ipv4_txt(ipv4, buf));
}

/* Add IP attribute */
vtss_rc cx_add_attr_ipv6(cx_get_state_t *s, const char *name, vtss_ipv6_t ipv6)
{
    char buf[40];

    return cx_add_attr_txt(s, name, misc_ipv6_txt(&ipv6, buf));
}

/* Add IPv6 address parameter named 'val' */
vtss_rc cx_add_val_ipv6(cx_get_state_t *s, cx_tag_id_t id, vtss_ipv6_t ipv6)
{
    CX_RC(cx_add_stx_txt(s, "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx"));
    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_ipv6(s, "val", ipv6));
    return cx_add_attr_end(s, id);
}

/* Add attribute end */
vtss_rc cx_add_attr_end(cx_get_state_t *s, cx_tag_id_t id)
{
    s->p += sprintf(s->p, ">");

    /* Add end tag or newline */
    if ((s->tag == cx_tag_table) && (id == CX_TAG_SWITCH)) {
        s->p += sprintf(s->p, "\n");
    } else {
        CX_RC(cx_add_tag(s, id, 1, 1));
    }

    return cx_size_check(s);
}

/* Add ulong parameter */
static vtss_rc cx_add_parm_ulong(cx_get_state_t *s, cx_tag_id_t id, const char *name,
                                 ulong val, const char *stx)
{
    if (stx != NULL) {
        CX_RC(cx_add_stx_txt(s, stx));
    }
    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_ulong(s, name, val));
    return cx_add_attr_end(s, id);
}

/* Add ulong parameter named 'val' with given syntax */
vtss_rc cx_add_val_ulong_stx(cx_get_state_t *s,
                             cx_tag_id_t id, ulong val, const char *stx)
{
    return cx_add_parm_ulong(s, id, "val", val, stx);
}

/* Add ulong parameter named 'val' */
vtss_rc cx_add_val_ulong(cx_get_state_t *s, cx_tag_id_t id, ulong val,
                         ulong min, ulong max)
{
    char buf[32];

    if (min == max) {
        sprintf(buf, "%u", min);
    } else {
        sprintf(buf, "%u-%u", min, max);
    }
    return cx_add_val_ulong_stx(s, id, val, buf);
}

/* Add long parameter named 'val' */
vtss_rc cx_add_val_long(cx_get_state_t *s,
                        cx_tag_id_t id, long val, const char *stx)
{
    if (stx != NULL) {
        CX_RC(cx_add_stx_txt(s, stx));
    }
    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_long(s, "val", val));
    return cx_add_attr_end(s, id);
}

/* Add text parameter named 'val' */
vtss_rc cx_add_val_txt(cx_get_state_t *s, cx_tag_id_t id,
                       const char *txt, const char *stx)
{
    if (stx != NULL) {
        CX_RC(cx_add_stx_txt(s, stx));
    }
    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_txt(s, "val", txt));
    return cx_add_attr_end(s, id);
}

/* Add keyword parameter named 'val' */
vtss_rc cx_add_val_kw(cx_get_state_t *s, cx_tag_id_t id,
                      const cx_kw_t *keyword, ulong val)
{
    CX_RC(cx_add_stx_start(s));
    CX_RC(cx_add_stx_kw(s, "val", keyword));
    CX_RC(cx_add_stx_end(s));

    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_kw(s, "val", keyword, val));
    return cx_add_attr_end(s, id);
}

/* Add port list start */
vtss_rc cx_add_port_start(cx_get_state_t *s, cx_tag_id_t id, const char *ports)
{
    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_txt(s, "port", ports));
    return cx_size_check(s);
}

/* Add port list end */
vtss_rc cx_add_port_end(cx_get_state_t *s, cx_tag_id_t id)
{
    /* Add attribute end */
    return cx_add_attr_end(s, id);
}

/* Add bool parameter named 'val' */
vtss_rc cx_add_val_bool(cx_get_state_t *s, cx_tag_id_t id, BOOL enable)
{
    CX_RC(cx_add_stx_txt(s, "enabled/disabled"));
    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_bool(s, "val", enable));
    return cx_add_attr_end(s, id);
}

/* Add IPv4 address parameter named 'val' */
vtss_rc cx_add_val_ipv4(cx_get_state_t *s, cx_tag_id_t id, vtss_ipv4_t ipv4)
{
    CX_RC(cx_add_stx_txt(s, "a.b.c.d"));
    CX_RC(cx_add_attr_start(s, id));
    CX_RC(cx_add_attr_ipv4(s, "val", ipv4));
    return cx_add_attr_end(s, id);
}

/* Add table */
// In : port_list : TRUE is the list is a port list (Index starting from "min+1"), if FALSE indexing is from "min"
vtss_rc cx_add_table(cx_get_state_t *s,
                     const cx_table_context_t *context,
                     ulong min, ulong max,
                     BOOL entry_skip(ulong idx),
                     cx_table_entry_match_t entry_match,
                     cx_table_entry_print_t entry_print,
                     BOOL port_list)
{
    ulong idx_a, idx_b;
    BOOL  list[max];
    BOOL  done[max];
    BOOL  found;
    char  buf[128];

    /* Print syntax */
    CX_RC(entry_print(s, context, 0, NULL));

    // If this is not a port list we want to loop from min to max (that is <= max), so we simply add 1 to max.
    if (!port_list) {
        max++;
    }

    memset(done, 0, sizeof(done));
    for (idx_a = min; idx_a < max; idx_a++) {
        if (done[idx_a] || entry_skip(idx_a)) {
            continue;
        }
        for (found = 0, idx_b = min; idx_b < max; idx_b++) {
            list[idx_b] = 0;
            if (done[idx_b] || entry_skip(idx_b)) {
                continue;
            }
            if (entry_match(context, idx_a, idx_b)) {
                list[idx_b] = 1;
                done[idx_b] = 1;
                found = 1;
            }
        }
        if (found) {
            if (port_list) {
                CX_RC(entry_print(s, context, idx_a,
                                  mgmt_list2txt(list, min, max - 1, buf)));
            } else {
                CX_RC(entry_print(s, context, idx_a,
                                  mgmt_non_portlist2txt(list, min, max - 1, buf)));
            }
        }
    }
    return VTSS_OK;
}

/* Port skip function */
static BOOL cx_port_skip(ulong port_no)
{
    return 0;
}

/* Add port table */
vtss_rc cx_add_port_table_ex(cx_get_state_t *s,
                             vtss_isid_t isid,
                             void *custom_context,
                             cx_tag_id_t tag,
                             cx_table_entry_match_t port_match,
                             cx_table_entry_print_t port_print)
{
    cx_table_context_t context;
    context.isid = isid;
    context.custom = custom_context;

    CX_RC(cx_add_tag_line(s, tag, 0));
    CX_RC(cx_add_table(s, &context, VTSS_PORT_NO_START, s->port_count,
                       cx_port_skip, port_match, port_print, TRUE));
    return cx_add_tag_line(s, tag, 1);
}

/* Add port table */
vtss_rc cx_add_port_table(cx_get_state_t *s,
                          vtss_isid_t isid,
                          cx_tag_id_t tag,
                          cx_table_entry_match_t port_match,
                          cx_table_entry_print_t port_print)
{
    return cx_add_port_table_ex(s, isid, NULL, tag, port_match, port_print);
}

/****************************************************************************/
/*  Local XML parse functions                                               */
/****************************************************************************/

/* Parameter error message */
vtss_rc cx_parm_invalid(cx_set_state_t *s)
{
    int  i;
    char *p;

    if (s->rc == VTSS_OK) {
        p = s->msg;
        p += sprintf(p, "Parameter '");
        for (i = 0; i < s->name_len; i++, p++) {
            *p = s->name[i];
        }
        p += sprintf(p, "' invalid: ");
        for (i = 0; i < s->val_len && i < 64; i++, p++) {
            *p = s->val[i];
        }
        *p = '\0';
        s->rc = CONF_XML_ERROR_FILE_PARM;
    }
    return s->rc;
}

/* Parameter unknown message */
vtss_rc cx_parm_unknown(cx_set_state_t *s)
{
    int  i;
    char *p;

    if (s->rc == VTSS_OK) {
        p = s->msg;
        p += sprintf(p, "Unknown parameter: ");
        for (i = 0; i < s->name_len && i < 64; i++, p++) {
            *p = s->name[i];
        }
        *p = '\0';
        s->rc = CONF_XML_ERROR_FILE_PARM;
    }
    return s->rc;
}

/* Simple parameter error */
vtss_rc cx_parm_error(cx_set_state_t *s, const char *txt)
{
    if (s->rc == VTSS_OK) {
        strcpy(s->msg, txt);
        s->rc = CONF_XML_ERROR_FILE_PARM;
    }
    return s->rc;
}

vtss_rc cx_parm_found_error(cx_set_state_t *s, const char *txt)
{
    if (s->rc == VTSS_OK) {
        sprintf(s->msg, "Parameter '%s' not found", txt);
        s->rc = CONF_XML_ERROR_FILE_PARM;
    }
    return s->rc;
}

/* Syntax error message for attribute character */
static vtss_rc cx_syntax_attr_error(cx_set_state_t *s, const char *txt, char c)
{
    if (s->rc == VTSS_OK) {
        sprintf(s->msg, "Illegal attribute %s character: %c", txt, c);
        s->rc = CONF_XML_ERROR_FILE_SYNTAX;
    }
    return s->rc;
}

/* Simple syntax error */
static void cx_syntax_error(cx_set_state_t *s, const char *txt)
{
    if (s->rc == VTSS_OK) {
        strcpy(s->msg, txt);
        s->rc = CONF_XML_ERROR_FILE_SYNTAX;
    }
}

/* Platform syntax error */
static void cx_syntax_platform_error(cx_set_state_t *s)
{
    if (s->rc == VTSS_OK) {
        strcpy(s->msg, "Platform information missing");
        s->rc = CONF_XML_ERROR_FILE_SYNTAX;
    }
}

/* Simple generic error */
static void cx_gen_error(cx_set_state_t *s, const char *txt)
{
    if (s->rc == VTSS_OK) {
        strcpy(s->msg, txt);
        s->rc = CONF_XML_ERROR_GEN;
    }
}

/* Parse attribute */
vtss_rc cx_parse_attr(cx_set_state_t *s)
{
    char *p = s->p;

    /* Skip spaces */
    while (*p == ' ') {
        p++;
    }

    /* Check if more attributes */
    if (*p == '>') {
        T_D("no more attributes");
        return CONF_XML_ERROR_GEN;
    }

    /* Name must start with letter or underscore */
    if (!isalpha(*p) && *p != '_') {
        return cx_syntax_attr_error(s, "name", *p);
    }
    s->name = p;

    /* Name must be letter, digit, underscore, hyphen or period */
    while (isalpha(*p) || isdigit(*p) || *p == '_' || *p == '-' || *p == '.') {
        p++;
    }

    /* Name must be followed by '=' */
    if (*p != '=') {
        return cx_syntax_attr_error(s, "equal", *p);
    }
    s->name_len = (p - s->name);

    /* '=' must be followed by '"' */
    p++;
    if (*p != '"') {
        return cx_syntax_attr_error(s, "start", *p);
    }

    /* Value characters must be printable and followed by '"' */
    p++;
    s->val = p;
    while (*p != '"') {
        if (!isprint(*p)) {
            return cx_syntax_attr_error(s, "value", '?');
        }
        p++;
    }
    s->val_len = (p - s->val);

    /* Find next character */
    s->next = (p + 1);

    return VTSS_OK;
}

/* Parse attribute with a given name */
vtss_rc cx_parse_attr_name(cx_set_state_t *s, const char *name)
{
    vtss_rc rc;

    if ((rc = cx_parse_attr(s)) == VTSS_OK && (s->name_len != strlen(name) || memcmp(name, s->name, s->name_len) != 0)) {
        T_D("CONF_XML_ERROR_FILE_PARM , s->name_len = %d, strlen(name) =%d , memcmp(name, s->name, s->name_len) = %d, s->name = %s", s->name_len, strlen(name), memcmp(name, s->name, s->name_len), s->name);
        rc = CONF_XML_ERROR_FILE_PARM;
    }

    if (rc != VTSS_OK && strcmp(name, "val") == 0) {
        T_D("strcmp(name,val)  = %d, name = %s", strcmp(name, "val"), name);
        rc = cx_parm_found_error(s, name);
    }

    return rc;
}

/* Parse txt parameter */
vtss_rc cx_parse_txt(cx_set_state_t *s, const char *name, char *buf, int max)
{
    T_D("enter cx_parse_txt name = %s", name);
    CX_RC(cx_parse_attr_name(s, name));

    if (s->val_len >= max) {
        sprintf(s->msg, "Parameter '%s' string too long", name);
        s->rc = CONF_XML_ERROR_FILE_PARM;
        return s->rc;
    }

    memcpy(buf, s->val, s->val_len);
    buf[s->val_len] = '\0';
    T_D("name: %s, txt: %s", name, buf);

    return VTSS_OK;
}

/* Determine if word matches syntax */
BOOL cx_word_match(const char *stx, const char *word)
{
    return (strstr(stx, word) == stx);
}

/* Parse txt parameter with specific word */
vtss_rc cx_parse_word(cx_set_state_t *s, const char *name, const char *word)
{
    char buf[32];

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
    return (cx_word_match(word, buf) ? VTSS_OK : CONF_XML_ERROR_FILE_PARM);
}

/* Parse ulong or word parameter, e.g. 'disabled' or 1-10 */
vtss_rc cx_parse_ulong_word(cx_set_state_t *s, const char *name, ulong *val,
                            ulong min, ulong max, const char *word, ulong match)
{
    char *end = NULL;
    BOOL found;
    char *p;
    ulonglong temp;

    if (word != NULL && cx_parse_word(s, name, word) == VTSS_OK) {
        *val = match;
        return VTSS_OK;
    }

    CX_RC(cx_parse_attr_name(s, name));
    p = s->val;
    while (p[0] == '0' && isdigit(p[1])) {
        p++;    /* Skip leading zeros */
    }

    temp = strtoull(p, &end, 0);
    if (p == end || temp > 0xFFFFFFFF) {
        sprintf(s->msg, "Parameter '%s' must be %s%sinteger in range %u-%u",
                name, word == NULL ? "" : word, word == NULL ? "" :
                " or ", min, max);
        s->rc = CONF_XML_ERROR_FILE_PARM;
        return s->rc;
    }
    *val = (ulong)temp;
    found = (end == (s->val + s->val_len));
    T_D("value '%s' %sfound (%u)", name, found ? "" : "not ", *val);
    if (!found || *val < min || *val > max) {
        sprintf(s->msg, "Parameter '%s' must be %s%sinteger in range %u-%u",
                name, word == NULL ? "" : word, word == NULL ? "" :
                " or ", min, max);
        s->rc = CONF_XML_ERROR_FILE_PARM;
    }
    return s->rc;
}

/* Parse 'any' parameter */
vtss_rc cx_parse_any(cx_set_state_t *s, const char *name)
{
    return cx_parse_word(s, name, "any");
}

/* Parse 'all' parameter */
vtss_rc cx_parse_all(cx_set_state_t *s, const char *name)
{
    return cx_parse_word(s, name, "all");
}

/* Parse ulong parameter, e.g. 'val="300"' */
vtss_rc cx_parse_ulong(cx_set_state_t *s, const char *name,
                       ulong *val, ulong min, ulong max)
{
    return cx_parse_ulong_word(s, name, val, min, max, NULL, 0);
}

/* Parse ulong/disabled parameter */
vtss_rc cx_parse_ulong_dis(cx_set_state_t *s, const char *name, ulong *val,
                           ulong min, ulong max, ulong match)
{
    return cx_parse_ulong_word(s, name, val, min, max, "disabled", match);
}

/* Parse ulong parameter named 'val' */
vtss_rc cx_parse_val_ulong(cx_set_state_t *s, ulong *val,
                           ulong min, ulong max)
{
    return cx_parse_ulong(s, "val", val, min, max);
}

/* Parse long parameter, e.g. 'val="-300"' */
vtss_rc cx_parse_long(cx_set_state_t *s, const char *name,
                      long *val, long min, long max)
{
    char *end = NULL;
    BOOL found;

    CX_RC(cx_parse_attr_name(s, name));

    *val = strtol(s->val, &end, 0);
    found = (end == (s->val + s->val_len));
    T_D("value '%s' %sfound (%lu)", name, found ? "" : "not ", *val);
    if (!found || *val < min || *val > max) {
        sprintf(s->msg, "Parameter '%s' must be integer in range %ld-%ld",
                name, min, max);
        s->rc = CONF_XML_ERROR_FILE_PARM;
    }
    return s->rc;
}

/* Parse long parameter named 'val' */
vtss_rc cx_parse_val_long(cx_set_state_t *s, long *val, long min, long max)
{
    return cx_parse_long(s, "val", val, min, max);
}

/* Parse txt parameter named 'val' */
vtss_rc cx_parse_val_txt(cx_set_state_t *s, char *buf, int max)
{
    return cx_parse_txt(s, "val", buf, max);
}


/* Parse txt (but only numbers)  parameter named 'val' */
vtss_rc cx_parse_val_txt_numbers_only(cx_set_state_t *s, char *buf, int max)
{
    vtss_rc rc;
    if ((rc = cx_parse_txt(s, "val", buf, max)) != VTSS_OK) {
        return rc;
    }
    return misc_str_chk_numbers_only(buf);
}

/* Parse keyword */
vtss_rc cx_parse_kw(cx_set_state_t *s, const char *name, const cx_kw_t *keyword,
                    ulong *val, BOOL force)
{
    char buf[32];
    int  count = 0;

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
    for ( ; keyword->name != NULL; keyword++) {
        if (cx_word_match(keyword->name, buf)) {
            count++;
            *val = keyword->val;
            T_D("name: %s, keyword: %s, val: %d, count = %d", name,
                keyword->name, keyword->val, count);

            // If it's a complete match, then return this one and only.
            if (strcmp(keyword->name, buf) == 0) {
                count = 1;
                break;
            }
        }
    }

    if (count > 1) {
        T_W("keyword %s isn't unique", keyword->name);
    }

    return (count == 1 ? VTSS_OK : (force ? cx_parm_invalid(s) :
                                    CONF_XML_ERROR_FILE_PARM));
}

/* Parse keyword named 'val' */
vtss_rc cx_parse_val_kw(cx_set_state_t *s, const cx_kw_t *keyword,
                        ulong *val, BOOL force)
{
    return cx_parse_kw(s, "val", keyword, val, force);
}

/* Parse boolean parameter */
vtss_rc cx_parse_bool(cx_set_state_t *s, const char *name, BOOL *val, BOOL force)
{
    ulong value;

    CX_RC(cx_parse_kw(s, name, cx_kw_bool, &value, force));
    *val = value;
    T_D("name: %s, val: %d", name, *val);
    return VTSS_OK;
}

/* Parse boolean parameter named 'val' */
vtss_rc cx_parse_val_bool(cx_set_state_t *s, BOOL *val, BOOL force)
{
    return cx_parse_bool(s, "val", val, force);
}

/* Parse port list that has any given attribute name */
vtss_rc cx_parse_ports_name(cx_set_state_t *s, BOOL *list, BOOL force, char *name)
{
    vtss_rc          rc;
    char             buf[128];
    vtss_uport_no_t  uport, iport;
    port_isid_info_t info;

    list[0] = 0;
    if ((rc = cx_parse_txt(s, name, buf, sizeof(buf))) == VTSS_OK && (rc = port_isid_info_get(s->isid, &info)) == VTSS_RC_OK) {
        if (strncmp(buf, "none", 4) == 0) {
            memset(list, 0, (info.port_count + 1) * sizeof(BOOL));
            return VTSS_RC_OK;
        }
        if (mgmt_txt2list(buf, list, 1, s->port_count, 0) == VTSS_OK) {
            /* Remove stack ports and ports exceeding the port count of the ISID */
            for (uport = 1; uport <= VTSS_PORTS; uport++) {
                iport = uport2iport(uport);
                if (uport > s->port_count ||
                    iport == info.stack_port_0 || iport == info.stack_port_1) {
                    list[uport] = 0;
                }
            }
        } else {
            CX_RC(cx_parm_invalid(s));
        }
    } else if (force) {
        CX_RC(cx_parm_found_error(s, name));
    }
    return (rc == VTSS_OK ? s->rc : rc);
}


/* Parse port list */
vtss_rc cx_parse_ports(cx_set_state_t *s, BOOL *list, BOOL force)
{
    return cx_parse_ports_name(s, list, force, "port");
}

/* Parse VLAN ID */
vtss_rc cx_parse_vid(cx_set_state_t *s, vtss_vid_t *vid, BOOL force)
{
    vtss_rc rc1;
    ulong   val;
    char    *name = "vid";

    if ((rc1 = cx_parse_ulong(s, name, &val, VTSS_VID_DEFAULT,
                              VTSS_VID_RESERVED)) == VTSS_OK) {
        *vid = val;
    } else if (force) {
        CX_RC(cx_parm_found_error(s, name));
    }
    return rc1;
}

/* Parse IPv4 address */
vtss_rc cx_parse_ipv4(cx_set_state_t *s, const char *name, vtss_ipv4_t *ipv4, vtss_ipv4_t *mask, BOOL is_mask)
{
    char buf[19]; /* 111.222.333.444/16 */

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
    if (mgmt_txt2ipv4(buf, ipv4, mask, is_mask) != VTSS_OK) {
        CX_RC(cx_parm_invalid(s));
    } else {
        s->rc = VTSS_OK;
    }
    return s->rc;
}

/* Parse IPv4 multicast address */
vtss_rc cx_parse_ipmcv4(cx_set_state_t *s, const char *name, vtss_ipv4_t *ipv4)
{
    char buf[19]; /* 233.222.333.444/16 */

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
    if (mgmt_txt2ipmcv4(buf, ipv4) != VTSS_OK) {
        CX_RC(cx_parm_invalid(s));
    } else {
        s->rc = VTSS_OK;
    }
    return s->rc;
}

/* Parse IPv4 address named 'val' */
vtss_rc cx_parse_val_ipv4(cx_set_state_t *s, vtss_ipv4_t *ipv4, vtss_ipv4_t *mask, BOOL is_mask)
{
    return cx_parse_ipv4(s, "val", ipv4, mask, is_mask);
}

/* Parse IPv6 address */
vtss_rc cx_parse_ipv6(cx_set_state_t *s, const char *name, vtss_ipv6_t *ipv6)
{
    char    buf[IPV6_ADDR_IBUF_MAX_LEN];
    int     buf_size;

    buf_size = sizeof(buf);
    memset(&buf[0], 0x0, buf_size);
    CX_RC(cx_parse_txt(s, name, buf, buf_size));
    if (mgmt_txt2ipv6_type(buf, ipv6) == TXT2IP6_ADDR_TYPE_INVALID) {
        CX_RC(cx_parm_invalid(s));
    } else {
        s->rc = VTSS_OK;
    }
    return s->rc;
}

vtss_rc cx_parse_ipmcv6(cx_set_state_t *s, const char *name, vtss_ipv6_t *ipv6)
{
    char    buf[IPV6_ADDR_IBUF_MAX_LEN];
    int     buf_size;

    buf_size = sizeof(buf);
    memset(&buf[0], 0x0, buf_size);
    CX_RC(cx_parse_txt(s, name, buf, buf_size));
    if (mgmt_txt2ipv6_type(buf, ipv6) == TXT2IP6_ADDR_TYPE_MCAST) {
        s->rc = VTSS_OK;
    } else {
        CX_RC(cx_parm_invalid(s));
    }
    return s->rc;
}

/* Parse IPv6 address named 'val' */
vtss_rc cx_parse_val_ipv6(cx_set_state_t *s, vtss_ipv6_t *ipv6)
{
    return cx_parse_ipv6(s, "val", ipv6);
}

BOOL cx_parse_str_to_hex(const char *str_p, ulong *hex_value_p)
{
    char token[20];
    int i = 0, j = 0;
    ulong k = 0, temp = 0;

    while (str_p[i] != '\0') {
        token[j++] = str_p[i++];
    }
    token[j] = '\0';

    if (strlen(token) > 8) {
        return FALSE;
    }

    i = 0;
    while (token[i] != '\0') {
        if (!((token[i] >= '0' && token[i] <= '9') ||
              (token[i] >= 'A' && token[i] <= 'F') ||
              (token[i] >= 'a' && token[i] <= 'f'))) {
            return FALSE;
        }
        i++;
    }

    temp = 0;
    for (i = 0; i < j; i++) {
        if (token[i] >= '0' && token[i] <= '9') {
            k = token[i] - '0';
        }   else if (token[i] >= 'A' && token[i] <= 'F') {
            k = token[i] - 'A' + 10;
        } else if (token[i] >= 'a' && token[i] <= 'f') {
            k = token[i] - 'a' + 10;
        }
        temp = 16 * temp + k;
    }

    *hex_value_p = temp;

    return TRUE;
}

/* Parse MAC address */
vtss_rc cx_parse_mac(cx_set_state_t *s, const char *name, uchar *mac)
{
    ulong i, j, k = 0, m;
    char mac_string[32], buf[4], sign = '-';

    CX_RC(cx_parse_txt(s, name, mac_string, sizeof(mac_string)));
    for (i = 0; i < strlen(mac_string); i++) {
        if (mac_string[i] == '.') {
            sign = '.';
        }
    }

    for (i = 0, j = 0, k = 0; i < strlen(mac_string); i++) {
        if (mac_string[i] == sign || j == 2) {
            buf[j] = '\0';
            j = 0;
            if (!cx_parse_str_to_hex(buf, &m)) {
                return cx_parm_invalid(s);
            }
            mac[k++] = (uchar) m;
            if (mac_string[i] != sign || j == 2) {
                buf[j++] = mac_string[i];
            }
        } else {
            buf[j++] = mac_string[i];
        }
    }
    buf[j] = '\0';
    if (!cx_parse_str_to_hex(buf, &m)) {
        return cx_parm_invalid(s);
    }
    mac[k] = (uchar)m;

    return s->rc;
}

vtss_rc cx_parse_udp_tcp(cx_set_state_t *s, const char *name,
                         vtss_udp_tcp_t *min, vtss_udp_tcp_t *max)
{
    char  buf[32];
    ulong start = 0, end = 0xffff;

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf)));
    if (cx_parse_any(s, name) != VTSS_OK &&
        mgmt_txt2range(buf, &start, &end, start, end) != VTSS_OK) {
        CX_RC(cx_parm_invalid(s));
    }
    *min = start;
    *max = end;
    return s->rc;
}

#ifdef VTSS_SW_OPTION_QOS
/* Parse QoS class */
vtss_rc cx_parse_class(cx_set_state_t *s, const char *name, vtss_prio_t *prio)
{
    char buf[32];

    CX_RC(cx_parse_txt(s, name, buf, sizeof(buf) - 1));
    if (mgmt_txt2prio(buf, prio) != VTSS_OK) {
        CX_RC(cx_parm_invalid(s));
    }
    return s->rc;
}

#endif /* VTSS_SW_OPTION_QOS */
/****************************************************************************/
/*  Management API functions                                                */
/****************************************************************************/

/* Get current configuration to buffer */
static vtss_rc cx_file_get(conf_xml_get_t *file)
{
    cx_get_state_t state, *s;
    vtss_usid_t    usid;
    int            i;
    int            tab_size = (&cx_mod_tab_end - cx_mod_tab_start);
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
    struct timeval start_m = {0, 0}, end_m = {0, 0}, diff_m;
#endif

    /* Initialize state */
    s = &state;
    s->file = file->data;
    s->p = file->data;
    s->size = file->size;
    s->indent = 0;
    s->error = 0;

    if (s->size != CONF_XML_FILE_MAX) {
        T_E("illegal size %u, expected %d", s->size, CONF_XML_FILE_MAX);
        return CONF_XML_ERROR_GEN;
    }

    /* assign tag table */
    s->tag = cx_tag_table;

    /* XML header */
    T_D("XML header");
    CX_RC(cx_add_tag_line(s, CX_TAG_HEADER, 0));
    cx_indent_set(s, 0);

    /* Configuration start */
    T_D("config header");
    CX_RC(cx_add_tag_line(s, CX_TAG_CONF, 0));

    /* Platform section */
    T_D("platform section");
    CX_RC(cx_add_tag_line(s, CX_TAG_PLATFORM, 0));
    CX_RC(cx_add_val_ulong(s, CX_TAG_PID, CONF_XML_PLATFORM_ID,
                           CONF_XML_PLATFORM_ID, CONF_XML_PLATFORM_ID));
    CX_RC(cx_add_val_ulong(s, CX_TAG_VERSION, CX_PLATFORM_VERSION,
                           CX_PLATFORM_VERSION, CX_PLATFORM_VERSION));
    CX_RC(cx_add_tag_line(s, CX_TAG_PLATFORM, 1));

    /* Global section start */
    T_D("global section");
    CX_RC(cx_add_tag_line(s, CX_TAG_GLOBAL, 0));
    /* add global - module XML */
    s->cmd = CX_GEN_CMD_GLOBAL;
    s->port_count = VTSS_PORTS;
    for (i = 0; i < tab_size; i++) {
        s->tag = cx_mod_tab_start[i].tag;
        if (TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_DEBUG)) {
            if (s->tag->type == CX_TAG_TYPE_MODULE) {
                if (gettimeofday(&start_m, NULL) != 0) {
                    T_D("get time failed!");
                }
            }
        }
        CX_RC(cx_mod_tab_start[i].gen_func(s));
        if (TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_DEBUG)) {
            if (s->tag->type == CX_TAG_TYPE_MODULE) {
                if (gettimeofday(&end_m, NULL) != 0) {
                    T_D("get time failed!");
                }
                diff_m.tv_usec = end_m.tv_usec - start_m.tv_usec;
                if (diff_m.tv_usec < 0) {
                    end_m.tv_sec--;
                    diff_m.tv_usec = 1000000 + diff_m.tv_usec;
                }
                diff_m.tv_sec = end_m.tv_sec - start_m.tv_sec;
                T_D("Conf_XML_Save Time Taken:: Section:: Global Module:: %-20s Seconds:: %-4ld Micro-Seconds:: %ld", s->tag->name, diff_m.tv_sec, diff_m.tv_usec);
            }
        }
    }
    /* Global section end */
    s->tag = cx_tag_table;
    CX_RC(cx_add_tag_line(s, CX_TAG_GLOBAL, 1));

    /* Switch sections */
    for (usid = VTSS_USID_START; usid < VTSS_USID_END; usid++) {
#if VTSS_SWITCH_STACKABLE
        s->isid = topo_usid2isid(usid);
#else
        s->isid = usid;
#endif /* VTSS_SWITCH_STACKABLE */
        if (!msg_switch_exists(s->isid)) {
            continue;
        }
        s->port_count = port_isid_port_count(s->isid);

        /* Switch section start */
        T_D("switch %d", usid);
        s->tag = cx_tag_table;
        CX_RC(cx_add_parm_ulong(s, CX_TAG_SWITCH, "sid", usid, NULL));
        cx_indent_set(s, 1);

        s->cmd = CX_GEN_CMD_SWITCH;
        for (i = 0; i < tab_size; i++) {
            s->tag = cx_mod_tab_start[i].tag;
            if (TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_DEBUG)) {
                if (s->tag->type == CX_TAG_TYPE_MODULE) {
                    if (gettimeofday(&start_m, NULL) != 0) {
                        T_D("get time failed!");
                    }
                }
            }
            CX_RC(cx_mod_tab_start[i].gen_func(s));
            if (TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_DEBUG)) {
                if (s->tag->type == CX_TAG_TYPE_MODULE) {
                    if (gettimeofday(&end_m, NULL) != 0) {
                        T_D("get time failed!");
                    }
                    diff_m.tv_usec = end_m.tv_usec - start_m.tv_usec;
                    if (diff_m.tv_usec < 0) {
                        end_m.tv_sec--;
                        diff_m.tv_usec = 1000000 + diff_m.tv_usec;
                    }
                    diff_m.tv_sec = end_m.tv_sec - start_m.tv_sec;
                    T_D("Conf_XML_Save Time Taken:: Section:: Switch  ISID:: %-2d  Module:: %-20s Seconds:: %-4ld Micro-Seconds:: %ld", s->isid, s->tag->name, diff_m.tv_sec, diff_m.tv_usec);
                }
            }
        }

        /* Switch section end */
        s->tag = cx_tag_table;
        CX_RC(cx_add_tag_line(s, CX_TAG_SWITCH, 1));

        T_D("switch %d - end", usid);
    }

    /* Configuration end */
    T_D("config end");
    s->tag = cx_tag_table;
    CX_RC(cx_add_tag_line(s, CX_TAG_CONF, 1));

    file->size = (s->p - s->file);

    return VTSS_OK;
}
/* Get current configuration to buffer */
vtss_rc conf_xml_file_get(conf_xml_get_t *file)
{
    vtss_rc rc;
    CX_CRIT_ENTER();
    rc = cx_file_get(file);
    CX_CRIT_EXIT();
    return rc;
}

#if defined(VTSS_SW_OPTION_LACP) || defined(VTSS_SW_OPTION_AGGR) || defined(VTSS_SW_OPTION_DOT1X)
static void *cx_find_mod_state(cx_set_state_t *s, vtss_module_id_t module)
{
    int idx, tab_size = (&cx_mod_tab_end - cx_mod_tab_start);
    struct cx_module_entry_t    *mod_tab = NULL;

    for (idx = 0; idx < tab_size; idx++) {
        if (module == cx_mod_tab_start[idx].module) {
            mod_tab = &cx_mod_tab_start[idx];
            break;
        }
    }
    if (mod_tab != NULL) {
        return (mod_tab->size ?
                ((char *)s + sizeof(cx_set_state_t) + mod_tab->offset) : NULL);
    }
    return NULL;
}
#endif

/* Initialize or commit switch state */
static void cx_check_switch(cx_set_state_t *s)
{
    vtss_port_no_t        port_idx = 0;
#if defined(VTSS_SW_OPTION_LACP) || defined(VTSS_SW_OPTION_AGGR)
    aggr_mgmt_group_no_t  aggr_id;
    BOOL                  aggr = 0, lacp = 0;
    aggr_cx_set_state_t  *lacp_aggr_state;
#endif
#if defined(VTSS_SW_OPTION_MSTP) && defined(VTSS_SW_OPTION_DOT1X)
    mstp_cx_set_state_t  *mstp_state;
#endif
#if defined(VTSS_SW_OPTION_DOT1X)
    dot1x_cx_set_state_t *dot1x_state;
#endif

#if defined(VTSS_SW_OPTION_LACP) || defined(VTSS_SW_OPTION_AGGR)
    if ((lacp_aggr_state = cx_find_mod_state(s, VTSS_MODULE_ID_LACP)) == NULL) {
        return;
    }
#endif

#if defined(VTSS_SW_OPTION_MSTP) && defined(VTSS_SW_OPTION_DOT1X)
    if ((mstp_state = cx_find_mod_state(s, VTSS_MODULE_ID_RSTP)) == NULL) {
        return;
    }
#endif
#if defined(VTSS_SW_OPTION_DOT1X)
    if ((dot1x_state = cx_find_mod_state(s, VTSS_MODULE_ID_DOT1X)) == NULL) {
        return;
    }
#endif
    /* Check configuration */
    for (port_idx = 0; port_idx < port_isid_port_count(s->isid); port_idx++) {
#ifdef VTSS_SW_OPTION_LACP
        lacp = lacp_aggr_state->lacp[port_idx].enable_lacp;
#endif /* VTSS_SW_OPTION_LACP */
#ifdef VTSS_SW_OPTION_AGGR
        aggr = (lacp_aggr_state->aggr_no[port_idx] ? 1 : 0);
        aggr_id = mgmt_aggr_no2id(lacp_aggr_state->aggr_no[port_idx]);
        if (aggr && lacp) {
            sprintf(s->msg, "Port %u is included in static aggregation %u and LACP is enabled", iport2uport(port_idx), aggr_id);
            s->rc = CONF_XML_ERROR_FILE_PARM;
            if (lacp_aggr_state->line_lacp.number != 0) {
                s->line = lacp_aggr_state->line_lacp;
            } else if (lacp_aggr_state->line_aggr.number != 0) {
                s->line = lacp_aggr_state->line_aggr;
            }
            return;
        }
#endif /* VTSS_SW_OPTION_AGGR */
#ifdef VTSS_SW_OPTION_DOT1X
        if (dot1x_state->dot1x.port_cfg[port_idx - VTSS_PORT_NO_START].admin_state != NAS_PORT_CONTROL_FORCE_AUTHORIZED) {
#ifdef VTSS_SW_OPTION_AGGR
            if (aggr) {
                sprintf(s->msg, "Port %u is included in static aggregation %u and 802.1X is enabled", iport2uport(port_idx), aggr_id);
                s->rc = CONF_XML_ERROR_FILE_PARM;
                if (dot1x_state->line_dot1x.number != 0) {
                    s->line = dot1x_state->line_dot1x;
                } else if (lacp_aggr_state->line_aggr.number != 0) {
                    s->line = lacp_aggr_state->line_aggr;
                }
                return;
            }
#endif /* VTSS_SW_OPTION_AGGR */
#ifdef VTSS_SW_OPTION_LACP
            if (lacp) {
                sprintf(s->msg, "Port %u is enabled for both LACP and 802.1X", iport2uport(port_idx));
                s->rc = CONF_XML_ERROR_FILE_PARM;
                if (dot1x_state->line_dot1x.number != 0) {
                    s->line = dot1x_state->line_dot1x;
                } else if (lacp_aggr_state->line_lacp.number != 0) {
                    s->line = lacp_aggr_state->line_lacp;
                }
                return;
            }
#endif /* VTSS_SW_OPTION_LACP */
#if defined(VTSS_SW_OPTION_MSTP)
            if (mstp_state->stp[port_idx].enable_stp) {
                sprintf(s->msg, "Port %u is enabled for both STP and 802.1X", iport2uport(port_idx));
                s->rc = CONF_XML_ERROR_FILE_PARM;
                if (dot1x_state->line_dot1x.number != 0) {
                    s->line = dot1x_state->line_dot1x;
                } else if (mstp_state->line_stp.number != 0) {
                    s->line = mstp_state->line_stp;
                }
                return;
            }
#endif /* defined(VTSS_SW_OPTION_MSTP) */
        }
#endif /* VTSS_SW_OPTION_DOT1X */
    }
}

/* Lookup tag */
static vtss_rc cx_tag_lookup(cx_set_state_t *s, char *tag, BOOL *end)
{
    cx_tag_id_t id = CX_TAG_NONE;
    const char  *name;
    int         len;
    cx_tag_entry_t *entry = NULL, *found = NULL;
    int i, tab_size = (&cx_mod_tab_end - cx_mod_tab_start);
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    static cyg_tick_count_t start_ticks;
    static cx_tag_id_t section_tag = 0;
#endif

    len = (s->p - tag);
    if (*tag == '/') {
        *end = 1;
        tag++;
        len--;
    } else {
        *end = 0;
    }

    for (i = 0; i < tab_size; i++) {
        /* lookup should be restricted to a module if module ID is set */
        if (s->mod_id != VTSS_MODULE_ID_NONE &&
            s->mod_id != cx_mod_tab_start[i].module) {
            continue;
        }

        entry = cx_mod_tab_start[i].tag;
        for (id = 0; entry[id].type != CX_TAG_TYPE_NONE; id++) {
            name = entry[id].name;
            if (len == strlen(name) && memcmp(tag, name, len) == 0) {
                T_D("found tag: %s%s", *end ? "/" : "", name);
                found = &entry[id];
                break;
            }
        }
        if (found) {
            break;
        }
    }

    if ((found == NULL) && (*end == 0 || len != 0)) {
        cx_syntax_error(s, "Unknown tag");
    } else if (*end) {
        /* Pop tag */
        if (s->tag_count == 0) {
            cx_syntax_error(s, "Missing start tag for end tag");
        } else {
            s->tag_count--;
            if (len != 0 &&
                (s->tag[s->tag_count].module != cx_mod_tab_start[i].module ||
                 s->tag[s->tag_count].id != id)) {
                cx_syntax_error(s, "End tag does not match previous tag");
            } else {
                s->mod_id = s->tag[s->tag_count].module;
                s->id = s->tag[s->tag_count].id;
                s->tag_type = s->tag[s->tag_count].type;

                if (TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_INFO)) {
                    if (s->tag[s->tag_count].type == CX_TAG_TYPE_SECTION) {
                        if ((s->tag[s->tag_count].id == CX_TAG_GLOBAL) ||
                            (s->tag[s->tag_count].id == CX_TAG_SWITCH)) {
                            section_tag = 0;
                        }
                    } else if (s->tag[s->tag_count].type == CX_TAG_TYPE_MODULE) {
                        if (section_tag != 0) {
                            u32 msecs = (u32)(cyg_current_time() - start_ticks) * ECOS_MSECS_PER_HWTICK;
                            u32 secs  = msecs / 1000UL;
                            msecs = msecs - 1000UL * secs;
                            if (section_tag == CX_TAG_GLOBAL) {
                                T_I("Load(Global(I=%d, A=%d, mod=%-20s) = %4u.%03u seconds", s->init, s->apply, vtss_module_names[s->tag[s->tag_count].module], secs, msecs);
                            } else if (s->isid != VTSS_ISID_UNKNOWN) {
                                T_I("Load(Switch(I=%d, A=%d, mod=%-20s, isid=%d) = %4u.%03u seconds", s->init, s->apply, vtss_module_names[s->tag[s->tag_count].module], s->isid, secs, msecs);
                            }
                        }
                    }
                }

                if (found) {
                    s->mod_tab = &cx_mod_tab_start[i];
                } else {
                    for (i = 0; i < tab_size; i++) {
                        if (s->mod_id == cx_mod_tab_start[i].module) {
                            s->mod_tab = &cx_mod_tab_start[i];
                        }
                    }
                }
            }
        }
    } else {
        /* Push tag */
        if (s->tag_count == CX_TAG_MAX) {
            cx_syntax_error(s, "Too many tag levels");
        } else {
            /* found can not be NULL here, following check has been
               added just to make lint happy! */
            if (found == NULL) {
                cx_syntax_error(s, "Unknown tag");
            } else {
                s->tag[s->tag_count].module = cx_mod_tab_start[i].module;
                s->tag[s->tag_count].id = id;
                s->tag[s->tag_count].type = found->type;

                if (TRACE_IS_ENABLED(VTSS_TRACE_MODULE_ID, VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_INFO)) {
                    if (s->tag[s->tag_count].type == CX_TAG_TYPE_SECTION) {
                        if (s->tag[s->tag_count].id == CX_TAG_GLOBAL) {
                            section_tag = CX_TAG_GLOBAL;
                        } else if (s->tag[s->tag_count].id == CX_TAG_SWITCH) {
                            section_tag = CX_TAG_SWITCH;
                        }
                    } else if (s->tag[s->tag_count].type == CX_TAG_TYPE_MODULE) {
                        if (section_tag != 0) {
                            start_ticks = cyg_current_time();
                        }
                    }
                }

                s->tag_count++;

                s->mod_id = cx_mod_tab_start[i].module;
                s->id = id;
                s->tag_type = found->type;
                s->mod_tab = &cx_mod_tab_start[i];
            }
        }
    }

    return s->rc;
}

static char *cx_find_tag(vtss_module_id_t module, cx_tag_id_t id)
{
    int i, tab_size = (&cx_mod_tab_end - cx_mod_tab_start);
    cx_tag_entry_t *entry;

    for (i = 0; i < tab_size; i++) {
        if (module == cx_mod_tab_start[i].module) {
            entry = cx_mod_tab_start[i].tag;
            return entry[id].name;
        }
    }

    /* not found */
    return NULL;
}

/* Check or apply configuration file */
static vtss_rc cx_file_set(cx_set_state_t *s, conf_xml_set_t *file, BOOL apply)
{
    char           c, *p, *tag, buf[32];
    int            len;
    ulong          sid, val, i;
    BOOL           end;
    BOOL           platform_section, platform_id,
                   platform_version, platform_done;

    T_I("-------------- apply: %d --------------------", apply);

    /* Initialize state state */
    s->rc = VTSS_OK;
    s->line.number = 1;
    s->line.txt = file->data;
    s->msg = &file->msg[0];
    s->msg[0] = '\0';
    s->master_elect = 0;

    /* Check XML header tag */
    p = s->line.txt;
    len = cx_sprint_tag(buf, cx_tag_table, CX_TAG_HEADER, 0, 1);
    if (strncmp(p, buf, len) != 0) {
        cx_gen_error(s, "XML header not found");
    } else {
        /* Check configuration header tag */
        p += len;
        s->line.number++;
        s->line.txt = p;
        len = cx_sprint_tag(buf, cx_tag_table, CX_TAG_CONF, 0, 1);
        if (strncmp(p, buf, len) != 0) {
            cx_gen_error(s, "Configuration header not found");
        } else {
            p += len;
            s->line.number++;
            s->line.txt = p;
        }
    }

    /* Parse file and look for tags */
    s->p = p;
    s->isid = VTSS_ISID_UNKNOWN;
    for (sid = 0; sid < VTSS_USID_END; sid++) {
        s->sid_done[sid] = 0;
    }

    s->mod_tab = NULL;
    s->mod_id = VTSS_MODULE_ID_NONE;
    s->mod_tag = 0xFFFF; /* initialize with non-practical value */
    s->group = 0xFFFF;
    s->id = 0xFFFF;
    s->tag_type = CX_TAG_TYPE_NONE;
    s->tag[0].module = VTSS_MODULE_ID_CONF_XML;
    s->tag[0].id = CX_TAG_CONF;
    s->tag[0].type = cx_tag_table[CX_TAG_CONF].type;
    s->tag_count = 1;
    s->port_count = VTSS_PORTS;
    platform_section = 0;
    platform_id = 0;
    platform_version = 0;
    platform_done = 0;
    tag = NULL;
    while (s->rc == VTSS_OK) {
        c = *(s->p);
        if (c == '\0') { /* End of file */
            T_D("done, line: %u", s->line.number);
            break;
        }
        switch (c) {
        case '\n': /* New line */
            s->p++;
            s->line.number++;
            s->line.txt = s->p;
            break;

        case '<': /* Start of tag */
            if (tag != NULL) {
                cx_syntax_error(s, "Unexpected tag start");
                break;
            }
            s->p++;
            tag = s->p;
            break;

        case '>': /* End of tag */
            if (tag == NULL) {
                cx_syntax_error(s, "Unexpected tag end");
                break;
            }

            /* Comment */
            if (*tag == '!') {
                s->p++;
                tag = NULL;
                break;
            }

            /* Lookup tag */
            if (cx_tag_lookup(s, tag, &end) != VTSS_OK) {
                break;
            }

            if (!platform_done) {
                if (s->mod_id == VTSS_MODULE_ID_CONF_XML && s->id == CX_TAG_PLATFORM) {
                    if (end) {
                        if (platform_id && platform_version) {
                            platform_done = 1;
                        } else {
                            cx_syntax_platform_error(s);
                        }
                        platform_section = 0;
                        s->mod_id = VTSS_MODULE_ID_NONE;
                    } else {
                        platform_section = 1;
                    }
                } else if (!end) {
                    cx_syntax_platform_error(s);
                }
            } else if (s->mod_id == VTSS_MODULE_ID_CONF_XML && s->id == CX_TAG_GLOBAL) {
                int    indx;
                int    tab_size = (&cx_mod_tab_end - cx_mod_tab_start);

                /* Global section tag */
                s->cmd = CX_PARSE_CMD_GLOBAL;

                if (end) {
                    s->init = 0;
                } else {
                    s->isid = VTSS_ISID_GLOBAL;
                    s->init = 1;
                }

                s->apply = s->init ? 0 : apply; // Better make sure that people don't write configuration at the beginning of a new section.

                for (indx = 0; indx < tab_size; indx++) {
                    CX_MOD_STATE_SET(s, &cx_mod_tab_start[indx]);
                    if ((u32)s->mod_state & 0x7LU) {
                        T_E("Internal error. s->mod_state not 64-bit aligned");
                    }
                    if (cx_mod_tab_start[indx].parse_func(s) != VTSS_OK) {
                        break;
                    }
                }
                if (end) {
                    s->isid = VTSS_ISID_UNKNOWN;
                }
                s->mod_id = VTSS_MODULE_ID_NONE;
            } else if (s->mod_id == VTSS_MODULE_ID_CONF_XML && s->id == CX_TAG_SWITCH) {
                int    indx;
                int    tab_size = (&cx_mod_tab_end - cx_mod_tab_start);

                /* Switch section tag */
                if (!end) {
                    if (s->isid != VTSS_ISID_UNKNOWN) {
                        T_E("Internal error. s->isid expected to be unknown but was %u", s->isid);
                        s->rc = CONF_XML_ERROR_GEN;
                    }
                    // Got a <switch> start element with no attributes, i.e. s->isid is currently VTSS_ISID_UNKNOWN.
                    // This is OK on standalone switches and stackable switches in standalone mode.
                    // However, it's not OK on a stackable switch in stacking mode.
#if VTSS_SWITCH_STACKABLE
                    if (vtss_stacking_enabled()) {
                        // sid attribute must exist.
                        (void)cx_parm_found_error(s, "sid"); // Don't return, since that'll bypass the error transferring mechanism at the end of this function.
                        break; // Break out of switch()
                    } else {
                        // Get the local isid.
                        for (sid = VTSS_ISID_START; sid < VTSS_ISID_END; sid++) {
                            if (msg_switch_exists(sid))  {
                                s->isid = sid;
                                break;
                            }
                        }
                    }
#else
                    // Non-stackable switch (i.e. standalone).
                    s->isid = VTSS_ISID_START;
#endif
                    s->port_count = port_isid_port_count(s->isid);
                }

                s->cmd = CX_PARSE_CMD_SWITCH;
                s->init = !end;
                s->apply = s->init ? 0 : apply; // Better make sure that people don't write configuration at the beginning of a new section.

                T_D("Switch: init=%d, apply=%d, isid=%d", s->init, s->apply, s->isid);

                if (s->isid != VTSS_ISID_UNKNOWN) {
                    // s->isid can be unknown if we're in a switch section with a SID that currently doesn't exist in the stack.
                    for (indx = 0; indx < tab_size; indx++) {
                        CX_MOD_STATE_SET(s, &cx_mod_tab_start[indx]);
                        if (cx_mod_tab_start[indx].parse_func(s) != VTSS_OK) {
                            break;
                        }
                    }

                    /* Check the inter-module dependency */
                    if (!apply) {
                        cx_check_switch(s);
                    }
                }

                if (end) {
                    s->port_count = VTSS_PORTS;
                    s->isid = VTSS_ISID_UNKNOWN;
                }
                s->mod_id = VTSS_MODULE_ID_NONE;
            } else if (s->tag_type == CX_TAG_TYPE_PARM) {
                /* Parameter tag ignored */
            } else if (s->tag_type == CX_TAG_TYPE_GROUP) {
                /* Group tag */
                s->group = (end ? 0xFFFF : s->id);
                if (!end) {
                    if (s->isid == VTSS_ISID_UNKNOWN) {
                        T_D("ignoring tag outside global/switch section or unknown ISID");
                    } else {
                        s->cmd = CX_PARSE_CMD_PARM;
                        s->apply = apply;
                        CX_MOD_STATE_SET(s, s->mod_tab);
                        s->ignored = 0;
                        (void)s->mod_tab->parse_func(s);

                        if (s->ignored && s->rc == VTSS_OK) {
                            cx_tag_entry_t *tag_tab = s->mod_tab->tag;
                            sprintf(s->msg,
                                    "Misplaced tag, section: %s,"
                                    "module: %s, group: %s, tag: %s",
                                    cx_tag_table[(s->isid == VTSS_ISID_GLOBAL) ? CX_TAG_GLOBAL : CX_TAG_SWITCH].name,
                                    (s->mod_tag != 0xFFFF) ? tag_tab[s->mod_tag].name : "",
                                    tag_tab[s->group].name,
                                    tag_tab[s->id].name);
                            s->rc = CONF_XML_ERROR_FILE_SYNTAX;
                        }
                    }
                }
            } else if (s->tag_type == CX_TAG_TYPE_MODULE) {
                /* Module tag */
                if (end) {
                    s->mod_id = VTSS_MODULE_ID_NONE;
                    s->mod_tag = 0xFFFF;
                } else {
                    s->mod_tag = s->id;
                }
            }
            s->p++;
            tag = NULL;
            break;

        case ' ': /* Space may indicate tag with attributes */
            if (tag == NULL || *tag == '!') {
                s->p++;
                break;
            }

            /* Search for tag end mark on same line */
            for (p = s->p; isprint(*p) && *p != '>'; p++) {
                ;
            }
            if (!isprint(*p)) {
                cx_syntax_error(s, "Missing tag end character");
                break;
            }

            /* Lookup tag */
            if (cx_tag_lookup(s, tag, &end) != VTSS_OK) {
                break;
            }
            if (end) {
                cx_syntax_error(s, "Unexpected end tag");
                break;
            }

            /* Parse platform ID and version */
            if (!platform_done) {
                if (s->mod_id == VTSS_MODULE_ID_CONF_XML && s->id == CX_TAG_PID && platform_section && cx_parse_val_ulong(s, &val, 0, 0xffffffff) == VTSS_OK) {
                    if (val == CONF_XML_PLATFORM_ID) {
                        platform_id = 1;
                    } else {
                        sprintf(s->msg, "Illegal platform ID: %u, expected: %u", val, CONF_XML_PLATFORM_ID);
                        s->rc = CONF_XML_ERROR_FILE_SYNTAX;
                    }
                } else if (s->mod_id == VTSS_MODULE_ID_CONF_XML && s->id == CX_TAG_VERSION && platform_section && cx_parse_val_ulong(s, &val, 0, 0xffffffff) == VTSS_OK) {
                    if (val == CX_PLATFORM_VERSION) {
                        platform_version = 1;
                    } else {
                        sprintf(s->msg, "Illegal platform version: %u, expected: %u", val, CX_PLATFORM_VERSION);
                        s->rc = CONF_XML_ERROR_FILE_SYNTAX;
                    }
                } else {
                    cx_syntax_platform_error(s);
                }
            } else if (s->mod_id == VTSS_MODULE_ID_CONF_XML && s->id == CX_TAG_SWITCH) {
                int    indx;
                int    tab_size = (&cx_mod_tab_end - cx_mod_tab_start);

                /* Switch section start tag */
                if (cx_parse_ulong(s, "sid", &sid, VTSS_USID_START, VTSS_USID_END - 1) != VTSS_OK) {
                    (void)cx_parm_found_error(s, "sid");  // Don't return, since that'll bypass the error transferring mechanism at the end of this function.
                    break;
                }
#if VTSS_SWITCH_STACKABLE
                sid = topo_usid2isid(sid);
#endif /* VTSS_SWITCH_STACKABLE */
                if (s->sid_done[sid]) {
                    sprintf(s->msg, "Switch %u already done", sid);
                    s->rc = CONF_XML_ERROR_FILE_SYNTAX;
                    break;
                }

                // Ignore unknown switches.
                if (s->isid != VTSS_ISID_UNKNOWN) {
                    T_E("Internal error. s->isid expected to be unknown but was %u", s->isid);
                    s->rc = CONF_XML_ERROR_GEN;
                }

                if (msg_switch_exists(sid)) {
                    s->isid = sid;
                    s->port_count = port_isid_port_count(s->isid);
                }
                s->cmd = CX_PARSE_CMD_SWITCH;
                s->init = 1;
                s->apply = 0;

                T_D("Switch: init=%d, apply=%d, isid=%d", s->init, s->apply, s->isid);

                for (indx = 0; s->isid != VTSS_ISID_UNKNOWN && indx < tab_size; indx++) {
                    CX_MOD_STATE_SET(s, &cx_mod_tab_start[indx]);
                    if (cx_mod_tab_start[indx].parse_func(s) != VTSS_OK) {
                        break;
                    }
                }
                s->mod_id = VTSS_MODULE_ID_NONE;
            } else if (s->tag_type == CX_TAG_TYPE_PARM) {
                /* Parameter tag */
                if (s->isid == VTSS_ISID_UNKNOWN) {
                    T_D("ignoring tag outside global/switch section or unknown ISID");
                } else {
                    s->cmd = CX_PARSE_CMD_PARM;
                    s->apply = apply;
                    CX_MOD_STATE_SET(s, s->mod_tab);
                    s->ignored = 0;
                    (void)s->mod_tab->parse_func(s);

                    if (s->ignored && s->rc == VTSS_OK) {
                        cx_tag_entry_t *tag_tab = s->mod_tab->tag;

                        sprintf(s->msg,
                                "Misplaced tag, section: %s,"
                                "module: %s, group: %s, tag: %s",
                                cx_tag_table[(s->isid == VTSS_ISID_GLOBAL) ?
                                             CX_TAG_GLOBAL : CX_TAG_SWITCH].name,
                                (s->mod_tag != 0xFFFF) ?
                                tag_tab[s->mod_tag].name : "",
                                (s->group != 0xFFFF) ?
                                tag_tab[s->group].name : "",
                                tag_tab[s->id].name);

                        s->rc = CONF_XML_ERROR_FILE_SYNTAX;
                    }
                }
            }
            tag = NULL;
            s->p = (p + 1); /* Advance to next character after end tag */
            break;

        default:
            s->p++;
            break;
        } /* switch (*(s->p)) */
    } /* while loop */

    if (s->rc == VTSS_OK && s->tag_count != 0) {
        sprintf(s->msg, "Missing end tag for: %s",
                cx_find_tag(s->tag[s->tag_count - 1].module,
                            s->tag[s->tag_count - 1].id));
        s->rc = CONF_XML_ERROR_FILE_SYNTAX;
    }

    file->line_no = s->line.number;
    file->line[0] = '\0';
    if (s->rc == VTSS_OK) {
#if VTSS_SWITCH_STACKABLE
        if (vtss_stacking_enabled() && s->master_elect) {
            T_I("Doing re-election");
            if ((topo_parm_set(0, TOPO_PARM_MST_TIME_IGNORE, 1)) != VTSS_OK) {
                T_E("Master re-election failed\n");
            }
        }
#endif /* VTSS_SWITCH_STACKABLE */
    } else {
        /* Error, copy line number and line */
        for (i = 0; i < (sizeof(file->line) - 1); i++) {
            c = s->line.txt[i];
            if (c == '\n' || c == '\0') {
                break;
            }
            file->line[i] = c;
        }
        file->line[i] = '\0';
    }

    return s->rc;
}

/* Total size of all modules set_state struct */
static u32 cx_state_size_total;

/* Set current configuration from buffer */
vtss_rc conf_xml_file_set(conf_xml_set_t *file)
{
    vtss_rc          rc;
    cx_set_state_t   *s;
    conf_mgmt_conf_t conf;

    /* Allocate state */
    if ((s = malloc(sizeof(cx_set_state_t) + cx_state_size_total)) == NULL) {
        return CONF_XML_ERROR_GEN;
    }

    memset(s, 0, sizeof(cx_set_state_t) + cx_state_size_total);

    /* Zero terminate file */
    file->data[file->size - 1] = '\0';

    /* Check and optionally apply file */
    CX_CRIT_ENTER();
    if ((rc = cx_file_set(s, file, 0)) == VTSS_OK && file->apply) {
        if (conf_mgmt_conf_get(&conf) == VTSS_OK) {
            /* For improved performance, configuration change detection is disabled
               during XML file parsing */
            conf.change_detect = 0;
            if (conf_mgmt_conf_set(&conf) == VTSS_OK) {
                rc = cx_file_set(s, file, 1);
                conf.change_detect = 1;
                (void)conf_mgmt_conf_set(&conf);
            }
        }
    }
    CX_CRIT_EXIT();

    /* Free state */
    free(s);

    return rc;
}

static vtss_rc conf_xml_parse_func(cx_set_state_t *s)
{
    return VTSS_OK;
}

static vtss_rc conf_xml_gen_func(cx_get_state_t *s)
{
    return VTSS_OK;
}

/* Register into the cx_module_table */
CX_MODULE_TAB_ENTRY(
    VTSS_MODULE_ID_CONF_XML,
    cx_tag_table,
    0,
    0,
    NULL,                 /* init function  */
    conf_xml_gen_func,    /* gen function   */
    conf_xml_parse_func); /* parse function */

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
static int cx_mod_tab_cmp_fun(const void *p1, const void *p2)
{
    const struct cx_module_entry_t *entry1 = (struct cx_module_entry_t *)p1;
    const struct cx_module_entry_t *entry2 = (struct cx_module_entry_t *)p2;

    /* Module IDs are unique */
    return(entry1->module - entry2->module);

}

/* Sorting cmd group table, cmd table and parameter table */
static void cx_mod_tab_sort(void)
{
    qsort(cx_mod_tab_start, (&cx_mod_tab_end - cx_mod_tab_start),
          sizeof(struct cx_module_entry_t), cx_mod_tab_cmp_fun);
}

// Make sure that all module callbacks are 64-bit aligned so that
// a structure can be assigned to them.
#define CX_ALIGN64(x) (8 * (((u32)(x) + 7) / 8))

/* Initialize module */
vtss_rc conf_xml_init(vtss_init_data_t *data)
{
    int i, tab_size = (&cx_mod_tab_end - cx_mod_tab_start);
    vtss_rc rc;

    if (data->cmd == INIT_CMD_INIT) {
        /* Initialize and register trace ressources */
        VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&trace_reg);

        /* Create semaphore for critical regions */
        critd_init(&cx_crit, "cx_crit", VTSS_MODULE_ID_CONF_XML, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        /* The critd's maximum lock time should be higher than what it takes to get or set the configuration.
         * If trace is enabled it might take up to several minutes, so we increase this particular semaphore's
         * lock time to 10 minutes.
         */
        /* It takes long time with full config and with full stack setup; increase the time to a large number */
        if (cx_crit.max_lock_time < 60000) {
            cx_crit.max_lock_time = 60000;
        }
        CX_CRIT_EXIT();

#ifdef VTSS_SW_OPTION_VCLI
        confxml_cli_req_init();
#endif

        /* Sort the module table */
        cx_mod_tab_sort();

        cx_state_size_total = CX_ALIGN64(sizeof(cx_set_state_t)) - sizeof(cx_set_state_t);
        for (i = 0; i < tab_size; i++) {
            /* Update the module starting set_state address */
            cx_mod_tab_start[i].offset = ((cx_mod_tab_start[i].size != 0) ? cx_state_size_total : 0);
            cx_state_size_total += CX_ALIGN64(cx_mod_tab_start[i].size);

            if (cx_mod_tab_start[i].init_func) {
                if ((rc = cx_mod_tab_start[i].init_func()) != VTSS_OK) {
                    T_E("XML init failed for module %d", cx_mod_tab_start[i].module);
                    return rc;
                }
            }
        }
    }

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x",
        data->cmd, data->isid, data->flags);

    return VTSS_OK;
}

#define CHAR_BUF_TO_STRING_BUF(x) ((x << 1) +1 )
void struct_to_string_size_cal(config_transport_t * conf)
{
	conf->hex_string.size = CHAR_BUF_TO_STRING_BUF(conf->data.size);
}

int struct_to_string(const config_transport_t * conf)
{
	int i;

	char* buf_ptr = conf->hex_string.pointer.c;
	/* Get rid of the invalid */
	if(conf->hex_string.pointer.c == NULL || conf->data.pointer.uc == NULL || conf->hex_string.size == 0 || conf->data.size ==0 )
		return -1;
	if((conf->hex_string.size) < CHAR_BUF_TO_STRING_BUF(conf->data.size))
		return -2;
	memset(conf->hex_string.pointer.c, 0, conf->hex_string.size);
	for (i = 0; i < conf->data.size ; i++) {
	    buf_ptr += sprintf(buf_ptr, "%02X", conf->data.pointer.uc[i] & 0xFF);
	}

	return 0;
}

int hexstring_to_struct( const config_transport_t * conf )
{
	const size_t tmp_size = 4;
	char tmp[tmp_size];
	int i,pos;

	if(conf->hex_string.pointer.c == NULL || conf->data.pointer.uc == NULL || conf->hex_string.size == 0 || conf->data.size ==0)
		return -1;
	if(CHAR_BUF_TO_STRING_BUF(conf->data.size) > (conf->hex_string.size + 1) )
		return -2;

	for(i=0, pos=0; conf->hex_string.pointer.c[pos] != 0; i++, pos +=2 ) {
		memset(tmp,0, tmp_size);
		tmp[0] = conf->hex_string.pointer.c[pos];
		tmp[1] = conf->hex_string.pointer.c[pos+1];
		tmp[3] = '\0';
		conf->data.pointer.uc[i] = strtol(tmp,NULL, 16);
	}
	return 0;
}





/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/

