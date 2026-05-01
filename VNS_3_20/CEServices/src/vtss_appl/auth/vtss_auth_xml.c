/*

 Vitesse Switch API software.

 Copyright (c) 2002-2010 Vitesse Semiconductor Corporation "Vitesse". All
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
#include "vtss_auth_api.h"

#include "conf_xml_api.h"
#include "conf_xml_trace_def.h"
#include "misc_api.h"

/* Tag IDs */
enum {
    /* Module tags */
    CX_TAG_AUTH,

    /* Group tags */
    CX_TAG_AUTH_AGENT_TABLE,
    CX_TAG_AUTH_RADIUS_TABLE,
    CX_TAG_AUTH_TACACS_TABLE,

    /* Parameter tags */
    CX_TAG_ENTRY,
    CX_TAG_RADIUS_TIMEOUT,
    CX_TAG_RADIUS_RETRANSMIT,
    CX_TAG_RADIUS_DEADTIME,
    CX_TAG_RADIUS_KEY,
    CX_TAG_RADIUS_NAS_IP_ADDRESS,
    CX_TAG_RADIUS_NAS_IPV6_ADDRESS,
    CX_TAG_RADIUS_NAS_IDENTIFIER,
    CX_TAG_TACACS_TIMEOUT,
    CX_TAG_TACACS_DEADTIME,
    CX_TAG_TACACS_KEY,

    /* Last entry */
    CX_TAG_NONE
};

#define AUTH_BUFF_LEN  256

/* Tag table */
static cx_tag_entry_t auth_cx_tag_table[CX_TAG_NONE + 1] = {
    [CX_TAG_AUTH] = {
        .name  = "auth",
        .descr = "Authentication settings",
        .type = CX_TAG_TYPE_MODULE
    },
    [CX_TAG_AUTH_AGENT_TABLE] = {
        .name  = "agent_table",
        .descr = "",
        .type = CX_TAG_TYPE_GROUP
    },
    [CX_TAG_AUTH_RADIUS_TABLE] = {
        .name  = "radius_table",
        .descr = "",
        .type = CX_TAG_TYPE_GROUP
    },
    [CX_TAG_AUTH_TACACS_TABLE] = {
        .name  = "tacacs_table",
        .descr = "",
        .type = CX_TAG_TYPE_GROUP
    },
    [CX_TAG_ENTRY] = {
        .name  = "entry",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_RADIUS_TIMEOUT] = {
        .name  = "radius_timeout",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_RADIUS_RETRANSMIT] = {
        .name  = "radius_retransmit",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_RADIUS_DEADTIME] = {
        .name  = "radius_deadtime",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_RADIUS_KEY] = {
        .name  = "radius_key",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_RADIUS_NAS_IP_ADDRESS] = {
        .name  = "radius_nas_ip_address",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_RADIUS_NAS_IPV6_ADDRESS] = {
        .name  = "radius_nas_ipv6_address",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_RADIUS_NAS_IDENTIFIER] = {
        .name  = "radius_nas_identifier",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_TACACS_TIMEOUT] = {
        .name  = "tacacs_timeout",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_TACACS_DEADTIME] = {
        .name  = "tacacs_deadtime",
        .descr = "",
        .type = CX_TAG_TYPE_PARM
    },
    [CX_TAG_TACACS_KEY] = {
        .name  = "tacacs_key",
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

static int agent2i(const char *str)
{
    int i, agent = -1;
    for (i = 0; i < VTSS_AUTH_AGENT_LAST; i++) {
        if (strcmp(str, vtss_auth_agent_names[i]) == 0) {
            agent = i;
            break;
        }
    }
    return agent;
}

static int method2i(const char *str)
{
    int i, method = -1;
    for (i = 0; i < VTSS_AUTH_METHOD_LAST; i++) {
        if (strcmp(str, vtss_auth_method_names[i]) == 0) {
            method = i;
            break;
        }
    }
    return method;
}

static vtss_rc auth_cx_parse_func(cx_set_state_t *s)
{
    switch (s->cmd) {
    case CX_PARSE_CMD_PARM: {
        BOOL             global;
        char             buf[256] = {0};
        vtss_ipv4_t      ipv4;
        vtss_ipv6_t      ipv6;
        ulong            val;

        global = (s->isid == VTSS_ISID_GLOBAL);
        if (!global) {
            s->ignored = 1;
            break;
        }

        switch (s->id) {
        case CX_TAG_RADIUS_TIMEOUT:
            if (cx_parse_val_ulong(s, &val, VTSS_AUTH_TIMEOUT_MIN, VTSS_AUTH_TIMEOUT_MAX) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating radius_timeout to \"%u\"", val);
                if (s->apply) {
                    vtss_auth_global_host_conf_t gc;
                    CX_RC(vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_RADIUS, &gc));
                    gc.timeout = val;
                    CX_RC(vtss_auth_mgmt_global_host_conf_set(VTSS_AUTH_PROTO_RADIUS, &gc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_RADIUS_RETRANSMIT:
            if (cx_parse_val_ulong(s, &val, VTSS_AUTH_RETRANSMIT_MIN, VTSS_AUTH_RETRANSMIT_MAX) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating radius_retransmit to \"%u\"", val);
                if (s->apply) {
                    vtss_auth_global_host_conf_t gc;
                    CX_RC(vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_RADIUS, &gc));
                    gc.retransmit = val;
                    CX_RC(vtss_auth_mgmt_global_host_conf_set(VTSS_AUTH_PROTO_RADIUS, &gc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_RADIUS_DEADTIME:
            if (cx_parse_val_ulong(s, &val, VTSS_AUTH_DEADTIME_MIN, VTSS_AUTH_DEADTIME_MAX) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating radius_deadtime to \"%u\"", val);
                if (s->apply) {
                    vtss_auth_global_host_conf_t gc;
                    CX_RC(vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_RADIUS, &gc));
                    gc.deadtime = val;
                    CX_RC(vtss_auth_mgmt_global_host_conf_set(VTSS_AUTH_PROTO_RADIUS, &gc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_RADIUS_KEY:
            if (cx_parse_val_txt(s, buf, VTSS_AUTH_KEY_LEN) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating radius_key to \"%s\"", buf);
                if (s->apply) {
                    vtss_auth_global_host_conf_t gc;
                    CX_RC(vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_RADIUS, &gc));
                    strcpy(gc.key, buf);
                    CX_RC(vtss_auth_mgmt_global_host_conf_set(VTSS_AUTH_PROTO_RADIUS, &gc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_RADIUS_NAS_IP_ADDRESS:
            if ((cx_parse_val_txt(s, buf, sizeof(buf)) == VTSS_OK) && (strlen(buf) == 0)) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Disable radius_nas_ip_address");
                if (s->apply) {
                    vtss_auth_radius_conf_t rc;
                    CX_RC(vtss_auth_mgmt_radius_conf_get(&rc));
                    rc.nas_ip_address_enable = FALSE;
                    CX_RC(vtss_auth_mgmt_radius_conf_set(&rc));
                }
            } else if (cx_parse_val_ipv4(s, &ipv4, NULL, FALSE) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating radius_nas_ip_address to \"%s\"", misc_ipv4_txt(ipv4, buf));
                if (s->apply) {
                    vtss_auth_radius_conf_t rc;
                    CX_RC(vtss_auth_mgmt_radius_conf_get(&rc));
                    rc.nas_ip_address_enable = TRUE;
                    rc.nas_ip_address = ipv4;
                    CX_RC(vtss_auth_mgmt_radius_conf_set(&rc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_RADIUS_NAS_IPV6_ADDRESS:
            if ((cx_parse_val_txt(s, buf, sizeof(buf)) == VTSS_OK) && (strlen(buf) == 0)) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Disable radius_nas_ipv6_address");
                if (s->apply) {
                    vtss_auth_radius_conf_t rc;
                    CX_RC(vtss_auth_mgmt_radius_conf_get(&rc));
                    rc.nas_ipv6_address_enable = FALSE;
                    CX_RC(vtss_auth_mgmt_radius_conf_set(&rc));
                }
            } else if (cx_parse_val_ipv6(s, &ipv6) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating radius_nas_ipv6_address to \"%s\"", misc_ipv6_txt(&ipv6, buf));
                if (s->apply) {
                    vtss_auth_radius_conf_t rc;
                    CX_RC(vtss_auth_mgmt_radius_conf_get(&rc));
                    rc.nas_ipv6_address_enable = TRUE;
                    rc.nas_ipv6_address = ipv6;
                    CX_RC(vtss_auth_mgmt_radius_conf_set(&rc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_RADIUS_NAS_IDENTIFIER:
            if (cx_parse_val_txt(s, buf, sizeof(buf)) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating radius_nas_identifier to \"%s\"", buf);
                if (s->apply) {
                    vtss_auth_radius_conf_t rc;
                    CX_RC(vtss_auth_mgmt_radius_conf_get(&rc));
                    strcpy(rc.nas_identifier, buf);
                    CX_RC(vtss_auth_mgmt_radius_conf_set(&rc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_TACACS_TIMEOUT:
            if (cx_parse_val_ulong(s, &val, VTSS_AUTH_TIMEOUT_MIN, VTSS_AUTH_TIMEOUT_MAX) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating tacacs_timeout to \"%u\"", val);
                if (s->apply) {
                    vtss_auth_global_host_conf_t gc;
                    CX_RC(vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_TACACS, &gc));
                    gc.retransmit = val;
                    CX_RC(vtss_auth_mgmt_global_host_conf_set(VTSS_AUTH_PROTO_TACACS, &gc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_TACACS_DEADTIME:
            if (cx_parse_val_ulong(s, &val, VTSS_AUTH_DEADTIME_MIN, VTSS_AUTH_DEADTIME_MAX) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating radius_deadtime to \"%u\"", val);
                if (s->apply) {
                    vtss_auth_global_host_conf_t gc;
                    CX_RC(vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_TACACS, &gc));
                    gc.deadtime = val;
                    CX_RC(vtss_auth_mgmt_global_host_conf_set(VTSS_AUTH_PROTO_TACACS, &gc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_TACACS_KEY:
            if (cx_parse_val_txt(s, buf, VTSS_AUTH_KEY_LEN) == VTSS_OK) {
                T_DG(VTSS_TRACE_GRP_AUTH, "Updating tacacs_key to \"%s\"", buf);
                if (s->apply) {
                    vtss_auth_global_host_conf_t gc;
                    CX_RC(vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_TACACS, &gc));
                    strcpy(gc.key, buf);
                    CX_RC(vtss_auth_mgmt_global_host_conf_set(VTSS_AUTH_PROTO_TACACS, &gc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        case CX_TAG_AUTH_AGENT_TABLE:
        case CX_TAG_AUTH_RADIUS_TABLE:
        case CX_TAG_AUTH_TACACS_TABLE:
            break;
        case CX_TAG_ENTRY:
            if (s->group == CX_TAG_AUTH_AGENT_TABLE) {
                int agent = -1, m1 = -1, m2 = -1, m3 = -1, m4 = -1;
                for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                    if (cx_parse_txt(s, "agent", buf, sizeof(buf)) == VTSS_OK) {
                        agent = agent2i(buf);
                    } else if (cx_parse_txt(s, "m1", buf, sizeof(buf)) == VTSS_OK) {
                        m1 = method2i(buf);
                    } else if (cx_parse_txt(s, "m2", buf, sizeof(buf)) == VTSS_OK) {
                        m2 = method2i(buf);
                    } else if (cx_parse_txt(s, "m3", buf, sizeof(buf)) == VTSS_OK) {
                        m3 = method2i(buf);
                    } else if (cx_parse_txt(s, "m4", buf, sizeof(buf)) == VTSS_OK) {
                        m4 = method2i(buf);
                    } else {
                        CX_RC(cx_parm_unknown(s));
                    }
                }
                if (s->apply && (agent >= 0)) {
                    vtss_auth_agent_conf_t ac;
                    CX_RC(vtss_auth_mgmt_agent_conf_get(agent, &ac));
                    if (m1 >= 0) {
                        ac.method[0] = m1;
                    }
                    if (m2 >= 0) {
                        ac.method[1] = m2;
                    }
                    if (m3 >= 0) {
                        ac.method[2] = m3;
                    }
                    if (m4 >= 0) {
                        ac.method[3] = m4;
                    }
                    CX_RC(vtss_auth_mgmt_agent_conf_set(agent, &ac));
                }
            } else if (s->group == CX_TAG_AUTH_RADIUS_TABLE) {
                BOOL found_host = FALSE;
                vtss_auth_host_conf_t hc = {{0}, VTSS_AUTH_RADIUS_AUTH_PORT_DEFAULT, VTSS_AUTH_RADIUS_ACCT_PORT_DEFAULT, 0, 0, {0}};

                for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                    if (cx_parse_txt(s, "host", hc.host, sizeof(hc.host)) == VTSS_OK) {
                        if (misc_str_is_hostname(hc.host) == VTSS_OK) {
                            found_host = TRUE;
                        } else {
                            CX_RC(cx_parm_invalid(s));
                        }
                    } else if (cx_parse_ulong(s, "auth_port",  &val, 0, 0xffff) == VTSS_OK) {
                        hc.auth_port = val;
                    } else if (cx_parse_ulong(s, "acct_port",  &val, 0, 0xffff) == VTSS_OK) {
                        hc.acct_port = val;
                    } else if (cx_parse_ulong(s, "timeout",    &val, 0, VTSS_AUTH_TIMEOUT_MAX) == VTSS_OK) {
                        hc.timeout = val;
                    } else if (cx_parse_ulong(s, "retransmit", &val, 0, VTSS_AUTH_RETRANSMIT_MAX) == VTSS_OK) {
                        hc.retransmit = val;
                    } else if (cx_parse_txt(s, "key", hc.key, sizeof(hc.key)) == VTSS_OK) {
                        T_DG(VTSS_TRACE_GRP_AUTH, "key = \"%s\"", hc.key);
                    } else {
                        CX_RC(cx_parm_unknown(s));
                    }
                }
                if (s->apply && found_host) {
                    CX_RC(vtss_auth_mgmt_host_add(VTSS_AUTH_PROTO_RADIUS, &hc));
                }
            } else if (s->group == CX_TAG_AUTH_TACACS_TABLE) {
                BOOL found_host = FALSE;
                vtss_auth_host_conf_t hc = {{0}, VTSS_AUTH_TACACS_PORT_DEFAULT, 0, 0, 0, {0}};

                for (; s->rc == VTSS_OK && cx_parse_attr(s) == VTSS_OK; s->p = s->next) {
                    if (cx_parse_txt(s, "host", hc.host, sizeof(hc.host)) == VTSS_OK) {
                        if (misc_str_is_hostname(hc.host) == VTSS_OK) {
                            found_host = TRUE;
                        } else {
                            CX_RC(cx_parm_invalid(s));
                        }
                    } else if (cx_parse_ulong(s, "port",    &val, 0, 0xffff) == VTSS_OK) {
                        hc.auth_port = val;
                    } else if (cx_parse_ulong(s, "timeout", &val, 0, VTSS_AUTH_TIMEOUT_MAX) == VTSS_OK) {
                        hc.timeout = val;
                    } else if (cx_parse_txt(s, "key", hc.key, sizeof(hc.key)) == VTSS_OK) {
                        T_DG(VTSS_TRACE_GRP_AUTH, "key = \"%s\"", hc.key);
                    } else {
                        CX_RC(cx_parm_unknown(s));
                    }
                }
                if (s->apply && found_host) {
                    CX_RC(vtss_auth_mgmt_host_add(VTSS_AUTH_PROTO_TACACS, &hc));
                }
            } else {
                s->ignored = 1;
            }
            break;
        default:
            s->ignored = 1;
            break;
        }
        break;
    } /* CX_PARSE_CMD_PARM */
    case CX_PARSE_CMD_GLOBAL:
        break;
    case CX_PARSE_CMD_SWITCH:
        break;
    default:
        break;
    }

    return s->rc;
}

static void auth_cx_host_cb(vtss_auth_proto_t proto, const void *const contxt, const vtss_auth_host_conf_t *const conf, int number)
{
    vtss_auth_host_conf_t *c = (vtss_auth_host_conf_t *)contxt;
    c[number] = *conf;
}

static vtss_rc auth_cx_gen_func(cx_get_state_t *s)
{
    char           buf[AUTH_BUFF_LEN];

    switch (s->cmd) {
    case CX_GEN_CMD_GLOBAL:
        /* Global - Authentication */
        T_D("global - auth");
        CX_RC(cx_add_tag_line(s, CX_TAG_AUTH, 0));
        {
            vtss_auth_agent_conf_t       ac;
            vtss_auth_global_host_conf_t gc;
            vtss_auth_radius_conf_t      rc;
            vtss_auth_host_conf_t        hc[VTSS_AUTH_NUMBER_OF_SERVERS] = {{{0}}};
            int  i, m, cnt;

            CX_RC(cx_add_tag_line(s, CX_TAG_AUTH_AGENT_TABLE, 0));
            /* Entry syntax */
            CX_RC(cx_add_stx_start(s));
            CX_RC(cx_add_attr_txt(s, "agent", "console/telnet/ssh/http"));
            CX_RC(cx_add_attr_txt(s, "method(s)", "1 to 4 of no/local/radius/tacacs"));
            CX_RC(cx_add_stx_end(s));

            for (i = 0; i < VTSS_AUTH_AGENT_LAST; i++) {
                if (vtss_auth_mgmt_agent_conf_get(i, &ac) == VTSS_OK) {
                    CX_RC(cx_add_attr_start(s, CX_TAG_ENTRY));
                    CX_RC(cx_add_attr_txt(s, "agent", vtss_auth_agent_names[i]));
                    for (m = 0; m < VTSS_AUTH_METHOD_LAST; m++) {
                        sprintf(buf, "m%d", m + 1);
                        CX_RC(cx_add_attr_txt(s, buf, vtss_auth_method_names[ac.method[m]]));
                    }
                    CX_RC(cx_add_attr_end(s, CX_TAG_ENTRY));
                }
            }
            CX_RC(cx_add_tag_line(s, CX_TAG_AUTH_AGENT_TABLE, 1));

            if (vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_RADIUS, &gc) == VTSS_OK) {
                CX_RC(cx_add_val_ulong(s, CX_TAG_RADIUS_TIMEOUT,    gc.timeout,    VTSS_AUTH_TIMEOUT_MIN,    VTSS_AUTH_TIMEOUT_MAX));
                CX_RC(cx_add_val_ulong(s, CX_TAG_RADIUS_RETRANSMIT, gc.retransmit, VTSS_AUTH_RETRANSMIT_MIN, VTSS_AUTH_RETRANSMIT_MAX));
                CX_RC(cx_add_val_ulong(s, CX_TAG_RADIUS_DEADTIME,   gc.deadtime,   VTSS_AUTH_DEADTIME_MIN,   VTSS_AUTH_DEADTIME_MAX));
                sprintf(buf, "0-%u characters", VTSS_AUTH_KEY_LEN - 1);
                CX_RC(cx_add_val_txt(s,   CX_TAG_RADIUS_KEY,        gc.key, buf));
            }

            if (vtss_auth_mgmt_radius_conf_get(&rc) == VTSS_OK) {
                CX_RC(cx_add_stx_txt(s, "a.b.c.d or empty string if disabled"));
                CX_RC(cx_add_attr_start(s, CX_TAG_RADIUS_NAS_IP_ADDRESS));
                if (rc.nas_ip_address_enable) {
                    CX_RC(cx_add_attr_ipv4(s, "val", rc.nas_ip_address));
                } else {
                    CX_RC(cx_add_attr_txt(s, "val", ""));
                }
                CX_RC(cx_add_attr_end(s, CX_TAG_RADIUS_NAS_IP_ADDRESS));
#ifdef VTSS_SW_OPTION_IPV6
                CX_RC(cx_add_stx_txt(s, "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx or empty string if disabled"));
                CX_RC(cx_add_attr_start(s, CX_TAG_RADIUS_NAS_IPV6_ADDRESS));
                if (rc.nas_ipv6_address_enable) {
                    CX_RC(cx_add_attr_ipv6(s, "val", rc.nas_ipv6_address));
                } else {
                    CX_RC(cx_add_attr_txt(s, "val", ""));
                }
                CX_RC(cx_add_attr_end(s, CX_TAG_RADIUS_NAS_IPV6_ADDRESS));
#endif /* VTSS_SW_OPTION_IPV6 */
                sprintf(buf, "0-%u characters", VTSS_AUTH_HOST_LEN - 1);
                CX_RC(cx_add_val_txt(s,   CX_TAG_RADIUS_NAS_IDENTIFIER, rc.nas_identifier, buf));
            }

            CX_RC(cx_add_tag_line(s, CX_TAG_AUTH_RADIUS_TABLE, 0));
            /* Entry syntax */
            CX_RC(cx_add_stx_start(s));
            CX_RC(cx_add_attr_txt(s,  "host",       "a.b.c.d or hostname"));
            CX_RC(cx_add_stx_ulong(s, "auth_port",  0, 0xffff));
            CX_RC(cx_add_stx_ulong(s, "acct_port",  0, 0xffff));
            CX_RC(cx_add_stx_ulong(s, "timeout",    VTSS_AUTH_TIMEOUT_MIN, VTSS_AUTH_TIMEOUT_MAX));
            CX_RC(cx_add_stx_ulong(s, "retransmit", VTSS_AUTH_RETRANSMIT_MIN, VTSS_AUTH_RETRANSMIT_MAX));
            sprintf(buf, "0-%u characters", VTSS_AUTH_KEY_LEN - 1);
            CX_RC(cx_add_attr_txt(s,  "key",        buf));
            CX_RC(cx_add_stx_end(s));

            if (vtss_auth_mgmt_host_iterate(VTSS_AUTH_PROTO_RADIUS, &hc[0], auth_cx_host_cb, &cnt) == VTSS_OK) {
                for (i = 0; i < cnt; i++) {
                    CX_RC(cx_add_attr_start(s, CX_TAG_ENTRY));
                    CX_RC(cx_add_attr_txt(s,   "host",       hc[i].host));
                    CX_RC(cx_add_attr_ulong(s, "auth_port",  hc[i].auth_port));
                    CX_RC(cx_add_attr_ulong(s, "acct_port",  hc[i].acct_port));
                    CX_RC(cx_add_attr_ulong(s, "timeout",    hc[i].timeout));
                    CX_RC(cx_add_attr_ulong(s, "retransmit", hc[i].retransmit));
                    CX_RC(cx_add_attr_txt(s,   "key",        hc[i].key));
                    CX_RC(cx_add_attr_end(s, CX_TAG_ENTRY));
                }
            }
            CX_RC(cx_add_tag_line(s, CX_TAG_AUTH_RADIUS_TABLE, 1));

            if (vtss_auth_mgmt_global_host_conf_get(VTSS_AUTH_PROTO_TACACS, &gc) == VTSS_OK) {
                CX_RC(cx_add_val_ulong(s, CX_TAG_TACACS_TIMEOUT,    gc.timeout,    VTSS_AUTH_TIMEOUT_MIN,    VTSS_AUTH_TIMEOUT_MAX));
                CX_RC(cx_add_val_ulong(s, CX_TAG_TACACS_DEADTIME,   gc.deadtime,   VTSS_AUTH_DEADTIME_MIN,   VTSS_AUTH_DEADTIME_MAX));
                sprintf(buf, "0-%u characters", VTSS_AUTH_KEY_LEN - 1);
                CX_RC(cx_add_val_txt(s,   CX_TAG_TACACS_KEY,        gc.key, buf));
            }

            CX_RC(cx_add_tag_line(s, CX_TAG_AUTH_TACACS_TABLE, 0));
            /* Entry syntax */
            CX_RC(cx_add_stx_start(s));
            CX_RC(cx_add_attr_txt(s,  "host",    "a.b.c.d or hostname"));
            CX_RC(cx_add_stx_ulong(s, "port",    0, 0xffff));
            CX_RC(cx_add_stx_ulong(s, "timeout", VTSS_AUTH_TIMEOUT_MIN, VTSS_AUTH_TIMEOUT_MAX));
            sprintf(buf, "0-%u characters", VTSS_AUTH_KEY_LEN - 1);
            CX_RC(cx_add_attr_txt(s,  "key",     buf));
            CX_RC(cx_add_stx_end(s));

            if (vtss_auth_mgmt_host_iterate(VTSS_AUTH_PROTO_TACACS, &hc[0], auth_cx_host_cb, &cnt) == VTSS_OK) {
                for (i = 0; i < cnt; i++) {
                    CX_RC(cx_add_attr_start(s, CX_TAG_ENTRY));
                    CX_RC(cx_add_attr_txt(s,   "host",    hc[i].host));
                    CX_RC(cx_add_attr_ulong(s, "port",    hc[i].auth_port));
                    CX_RC(cx_add_attr_ulong(s, "timeout", hc[i].timeout));
                    CX_RC(cx_add_attr_txt(s,   "key",     hc[i].key));
                    CX_RC(cx_add_attr_end(s, CX_TAG_ENTRY));
                }
            }
            CX_RC(cx_add_tag_line(s, CX_TAG_AUTH_TACACS_TABLE, 1));
        }
        CX_RC(cx_add_tag_line(s, CX_TAG_AUTH, 1));
        break;
    case CX_GEN_CMD_SWITCH:
        break;
    default:
        T_E("Unknown command");
        return VTSS_RC_ERROR;
    } /* End of Switch */
    return VTSS_OK;
}
/* Register the info in to the cx_module_table */
CX_MODULE_TAB_ENTRY(VTSS_MODULE_ID_AUTH, auth_cx_tag_table,
                    0, 0,
                    NULL,                   /* init function       */
                    auth_cx_gen_func,       /* Generation fucntion */
                    auth_cx_parse_func);    /* parse fucntion      */
