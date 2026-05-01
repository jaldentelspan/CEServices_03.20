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

// Avoid <netinet/in.h> not used in module vtss_auth_cli.c"
/*lint --e{766} */

#include "cli.h"
#include "cli_grp_help.h"
#include "vtss_auth_api.h"
#include "vtss_auth_cli.h"
#ifdef VTSS_SW_OPTION_RADIUS
#include "vtss_radius_api.h"
#endif
#include "cli_trace_def.h"
#include <netinet/in.h>

#ifdef VTSS_CLI_SEC_GRP
#define CLI_AUTH_PATH_AAA      VTSS_CLI_GRP_SEC_AAA_PATH
#define CLI_AUTH_PATH_SWITCH   VTSS_CLI_GRP_SEC_SWITCH_PATH "Auth "
#else
#define CLI_AUTH_PATH_AAA      "Auth "
#define CLI_AUTH_PATH_SWITCH   "Auth "
#endif

typedef struct {
    uchar                  username[VTSS_SYS_USERNAME_LEN];

    int                    ac_ix;              /* Index into agent configuration */
    vtss_auth_agent_conf_t ac;                 /* Agent configuration (List of authentication methods) */

    vtss_auth_agent_t      agent;
    u32                    auth_port;
    u32                    acct_port;
    u32                    timeout;
    u32                    retransmit;
    char                   key[VTSS_AUTH_KEY_LEN];
} auth_cli_req_t;

#if defined(VTSS_AUTH_ENABLE_CONSOLE) || defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_SW_OPTION_SSH) || defined(VTSS_SW_OPTION_WEB)
#define AUTH_INCLUDE_CLIENT_CONFIG 1 // Make a shorter name for later use
/*
 * The following strings are used in different places and are dependant of the modules included.
 * In order not to pollute the code with #ifdef's, the strings are generated dynamically.
 */
static char AUTH_agent_names[32];      // One or more of "console|telnet|ssh|http"
static char AUTH_agent_help[256];      // Help text for agents"
static char AUTH_method_names[32];     // "local|radius|tacacs" where radius and tacacs are optional
static char AUTH_method_help[256];     // Help text for method names
static char AUTH_all_method_names[32]; // "no|local|radius|tacacs" where radius and tacacs are optional
static char AUTH_all_method_help[256]; // Help text for all method names
static char AUTH_debug_rw_syntax[128]  = "Debug Auth";  // Must be initialized in order to be sorted correctly


static char AUTH_login_ro_syntax[VTSS_AUTH_AGENT_LAST][64] = {
    [VTSS_AUTH_AGENT_CONSOLE] = CLI_AUTH_PATH_SWITCH "Console",
    [VTSS_AUTH_AGENT_TELNET]  = CLI_AUTH_PATH_SWITCH "Telnet",
    [VTSS_AUTH_AGENT_SSH]     = CLI_AUTH_PATH_SWITCH "SSH",
    [VTSS_AUTH_AGENT_HTTP]    = CLI_AUTH_PATH_SWITCH "HTTP",
};
static char AUTH_login_rw_syntax[VTSS_AUTH_AGENT_LAST][256];
#endif

/****************************************************************************/
/*  Private functions                                                       */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_RADIUS
/* Setup our req_t with default RADIUS values */
static void AUTH_cli_default_radius(cli_req_t *req)
{
    auth_cli_req_t *auth_req = req->module_req;
    auth_req->auth_port = VTSS_AUTH_RADIUS_AUTH_PORT_DEFAULT;
    auth_req->acct_port = VTSS_AUTH_RADIUS_ACCT_PORT_DEFAULT;
}
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
/* Setup our req_t with default TACACS values */
static void AUTH_cli_default_tacacs(cli_req_t *req)
{
    auth_cli_req_t *auth_req = req->module_req;
    auth_req->auth_port = VTSS_AUTH_TACACS_PORT_DEFAULT;
    auth_req->acct_port = 0;
}
#endif /* VTSS_SW_OPTION_TACPLUS */

#ifdef AUTH_INCLUDE_CLIENT_CONFIG
/* Generate dynamic parser strings */
static void AUTH_create_parser_strings(void)
{
    int i;
    int cnt     = 0;
    int agents  = 0;
    int methods = 0;

#ifdef VTSS_AUTH_ENABLE_CONSOLE
    cnt += snprintf(AUTH_agent_names + cnt, sizeof(AUTH_agent_names) - cnt, "%s%s", agents++ ? "|" : "", vtss_auth_agent_names[VTSS_AUTH_AGENT_CONSOLE]);
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
    cnt += snprintf(AUTH_agent_names + cnt, sizeof(AUTH_agent_names) - cnt, "%s%s", agents++ ? "|" : "", vtss_auth_agent_names[VTSS_AUTH_AGENT_TELNET]);
#endif
#ifdef VTSS_SW_OPTION_SSH
    cnt += snprintf(AUTH_agent_names + cnt, sizeof(AUTH_agent_names) - cnt, "%s%s", agents++ ? "|" : "", vtss_auth_agent_names[VTSS_AUTH_AGENT_SSH]);
#endif
#ifdef VTSS_SW_OPTION_WEB
    cnt += snprintf(AUTH_agent_names + cnt, sizeof(AUTH_agent_names) - cnt, "%s%s", agents++ ? "|" : "", vtss_auth_agent_names[VTSS_AUTH_AGENT_HTTP]);
#endif
    if (cnt >= (int)sizeof(AUTH_agent_names)) {
        T_E("AUTH_agent_names truncated (%u,%zu)", cnt, sizeof(AUTH_agent_names));
    }

    cnt = 0;
    agents = 0;
#ifdef VTSS_AUTH_ENABLE_CONSOLE
    cnt += snprintf(AUTH_agent_help + cnt, sizeof(AUTH_agent_help) - cnt, "%s%s", agents++ ? "\n" : "", "console    : Authenticate as Console");
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
    cnt += snprintf(AUTH_agent_help + cnt, sizeof(AUTH_agent_help) - cnt, "%s%s", agents++ ? "\n" : "", "telnet     : Authenticate as Telnet");
#endif
#ifdef VTSS_SW_OPTION_SSH
    cnt += snprintf(AUTH_agent_help + cnt, sizeof(AUTH_agent_help) - cnt, "%s%s", agents++ ? "\n" : "", "ssh        : Authenticate as SSH");
#endif
#ifdef VTSS_SW_OPTION_WEB
    cnt += snprintf(AUTH_agent_help + cnt, sizeof(AUTH_agent_help) - cnt, "%s%s", agents++ ? "\n" : "", "http       : Authenticate as HTTP");
#endif
    if (cnt >= (int)sizeof(AUTH_agent_help)) {
        T_E("AUTH_agent_help truncated (%u,%zu)", cnt, sizeof(AUTH_agent_help));
    }

    cnt = snprintf(AUTH_method_names, sizeof(AUTH_method_names), "%s", vtss_auth_method_names[VTSS_AUTH_METHOD_LOCAL]);
#ifdef VTSS_SW_OPTION_RADIUS
    cnt += snprintf(AUTH_method_names + cnt, sizeof(AUTH_method_names) - cnt, "|%s", vtss_auth_method_names[VTSS_AUTH_METHOD_RADIUS]);
    methods++;
#endif
#ifdef VTSS_SW_OPTION_TACPLUS
    cnt += snprintf(AUTH_method_names + cnt, sizeof(AUTH_method_names) - cnt, "|%s", vtss_auth_method_names[VTSS_AUTH_METHOD_TACACS]);
    methods++;
#endif
    if (cnt >= (int)sizeof(AUTH_method_names)) {
        T_E("AUTH_method_names truncated (%u,%zu)", cnt, sizeof(AUTH_method_names));
    }

    cnt = snprintf(AUTH_all_method_names, sizeof(AUTH_all_method_names), "%s|%s", vtss_auth_method_names[VTSS_AUTH_METHOD_NONE], AUTH_method_names);
    if (cnt >= (int)sizeof(AUTH_all_method_names)) {
        T_E("AUTH_all_method_names truncated (%u,%zu)", cnt, sizeof(AUTH_all_method_names));
    }

    cnt = snprintf(AUTH_method_help, sizeof(AUTH_method_help),
                   "local      : Use local authentication");
#ifdef VTSS_SW_OPTION_RADIUS
    cnt += snprintf(AUTH_method_help + cnt, sizeof(AUTH_method_help) - cnt,
                    "\nradius     : Use remote RADIUS authentication");
#endif
#ifdef VTSS_SW_OPTION_TACPLUS
    cnt += snprintf(AUTH_method_help + cnt, sizeof(AUTH_method_help) - cnt,
                    "\ntacacs     : Use remote TACACS+ authentication");
#endif
    if (cnt >= (int)sizeof(AUTH_method_help)) {
        T_E("AUTH_method_help truncated (%u,%zu)", cnt, sizeof(AUTH_method_help));
    }

    cnt = snprintf(AUTH_all_method_help, sizeof(AUTH_all_method_help),
                   "no         : Authentication disabled\n"
                   "%s\n"
                   "             (default: Show authentication method)", AUTH_method_help);
    if (cnt >= (int)sizeof(AUTH_all_method_help)) {
        T_E("AUTH_all_method_help truncated (%u,%zu)", cnt, sizeof(AUTH_all_method_help));
    }

    cnt = snprintf(AUTH_debug_rw_syntax,  sizeof(AUTH_debug_rw_syntax),  "Debug Auth [%s] <user_name> <password>", AUTH_agent_names);
    if (cnt >= (int)sizeof(AUTH_debug_rw_syntax)) {
        T_E("AUTH_debug_rw_syntax truncated (%u,%zu)", cnt, sizeof(AUTH_debug_rw_syntax));
    }

    for (i = 0; i < VTSS_AUTH_AGENT_LAST; i++) {
        int m;
        cnt = snprintf(AUTH_login_rw_syntax[i], sizeof(AUTH_login_rw_syntax[i]), "%s [%s]", AUTH_login_ro_syntax[i], AUTH_all_method_names);
        for (m = 0; m < methods ; m++) {
            cnt += snprintf(AUTH_login_rw_syntax[i] + cnt, sizeof(AUTH_login_rw_syntax[i]) - cnt, " [%s]", AUTH_method_names);
        }
        if (cnt >= (int)sizeof(AUTH_login_rw_syntax)) {
            T_E("AUTH_login_rw_syntax truncated (%u,%zu)", cnt, sizeof(AUTH_login_rw_syntax[i]));
        }
    }
}
#endif

/****************************************************************************/
/*  Command functions                                                       */
/****************************************************************************/
#ifdef AUTH_INCLUDE_CLIENT_CONFIG
static void AUTH_cli_cmd_agent_login_show(vtss_auth_agent_t agent)
{
    vtss_rc                rc;
    vtss_auth_agent_conf_t conf;
    vtss_auth_method_t     method;
    int                    i;

    if ((rc = vtss_auth_mgmt_agent_conf_get(agent, &conf)) == VTSS_OK) {
        CPRINTF("%-7s : ", vtss_auth_agent_names[agent]);
        for (i = 0; i < VTSS_AUTH_METHOD_LAST; i++) {
            method = conf.method[i];
            if ((i == 0) || (method != VTSS_AUTH_METHOD_NONE)) {
                CPRINTF("%s ", vtss_auth_method_names[method]);
                if (method == VTSS_AUTH_METHOD_NONE) {
                    break;
                }
            }
        }
        CPRINTF("\n");
    } else {
        CPRINTF("%s\n", error_txt(rc));
    }
}

static void AUTH_cli_cmd_agent_login_check(void)
{
    vtss_auth_agent_conf_t conf;
    int                    auth_possible = 0;

#ifdef VTSS_AUTH_ENABLE_CONSOLE
    if ((vtss_auth_mgmt_agent_conf_get(VTSS_AUTH_AGENT_CONSOLE, &conf) == VTSS_OK) && (conf.method[0] != VTSS_AUTH_METHOD_NONE)) {
        auth_possible++;
    }
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
    if ((vtss_auth_mgmt_agent_conf_get(VTSS_AUTH_AGENT_TELNET, &conf) == VTSS_OK) && (conf.method[0] != VTSS_AUTH_METHOD_NONE)) {
        auth_possible++;
    }
#endif
#ifdef VTSS_SW_OPTION_SSH
    if ((vtss_auth_mgmt_agent_conf_get(VTSS_AUTH_AGENT_SSH, &conf) == VTSS_OK) && (conf.method[0] != VTSS_AUTH_METHOD_NONE)) {
        auth_possible++;
    }
#endif
#ifdef VTSS_SW_OPTION_WEB
    if ((vtss_auth_mgmt_agent_conf_get(VTSS_AUTH_AGENT_HTTP, &conf) == VTSS_OK) && (conf.method[0] != VTSS_AUTH_METHOD_NONE)) {
        auth_possible++;
    }
#endif

    if (!auth_possible) {
        CPRINTF("WARNING: All authentication agents are disabled. Login not possible.\n");
    }
}

static void AUTH_cli_cmd_agent_login(cli_req_t *req, BOOL console, BOOL telnet, BOOL ssh, BOOL http)
{
    vtss_rc        rc;
    auth_cli_req_t *auth_req = req->module_req;

    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

    if (req->set) {
#ifdef VTSS_AUTH_ENABLE_CONSOLE
        if (console) {
            if ((rc = vtss_auth_mgmt_agent_conf_set(VTSS_AUTH_AGENT_CONSOLE, &auth_req->ac)) != VTSS_OK) {
                CPRINTF("%s\n", error_txt(rc));
            }
        }
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
        if (telnet) {
            if ((rc = vtss_auth_mgmt_agent_conf_set(VTSS_AUTH_AGENT_TELNET, &auth_req->ac)) != VTSS_OK) {
                CPRINTF("%s\n", error_txt(rc));
            }
        }
#endif
#ifdef VTSS_SW_OPTION_SSH
        if (ssh) {
            if ((rc = vtss_auth_mgmt_agent_conf_set(VTSS_AUTH_AGENT_SSH, &auth_req->ac)) != VTSS_OK) {
                CPRINTF("%s\n", error_txt(rc));
            }
        }
#endif
#ifdef VTSS_SW_OPTION_WEB
        if (http) {
            if ((rc = vtss_auth_mgmt_agent_conf_set(VTSS_AUTH_AGENT_HTTP, &auth_req->ac)) != VTSS_OK) {
                CPRINTF("%s\n", error_txt(rc));
            }
        }
#endif
    } else {
#ifdef VTSS_AUTH_ENABLE_CONSOLE
        if (console) {
            AUTH_cli_cmd_agent_login_show(VTSS_AUTH_AGENT_CONSOLE);
        }
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
        if (telnet) {
            AUTH_cli_cmd_agent_login_show(VTSS_AUTH_AGENT_TELNET);
        }
#endif
#ifdef VTSS_SW_OPTION_SSH
        if (ssh) {
            AUTH_cli_cmd_agent_login_show(VTSS_AUTH_AGENT_SSH);
        }
#endif
#ifdef VTSS_SW_OPTION_WEB
        if (http) {
            AUTH_cli_cmd_agent_login_show(VTSS_AUTH_AGENT_HTTP);
        }
#endif
    }
    AUTH_cli_cmd_agent_login_check();
}

#ifdef VTSS_AUTH_ENABLE_CONSOLE
static void AUTH_cli_cmd_agent_login_console(cli_req_t *req)
{
    AUTH_cli_cmd_agent_login(req, 1, 0, 0, 0);
}
#endif /* VTSS_AUTH_ENABLE_CONSOLE */

#ifdef VTSS_SW_OPTION_CLI_TELNET
static void AUTH_cli_cmd_agent_login_telnet(cli_req_t *req)
{
    AUTH_cli_cmd_agent_login(req, 0, 1, 0, 0);
}
#endif /* VTSS_SW_OPTION_CLI_TELNET */

#ifdef VTSS_SW_OPTION_SSH
static void AUTH_cli_cmd_agent_login_ssh(cli_req_t *req)
{
    AUTH_cli_cmd_agent_login(req, 0, 0, 1, 0);
}
#endif /* VTSS_SW_OPTION_SSH */

#ifdef VTSS_SW_OPTION_WEB
static void AUTH_cli_cmd_agent_login_http(cli_req_t *req)
{
    AUTH_cli_cmd_agent_login(req, 0, 0, 0, 1);
}
#endif /* VTSS_SW_OPTION_WEB */

#ifdef VTSS_CLI_SEC_GRP
static void AUTH_cli_cmd_agent_conf(cli_req_t *req)
{
    cli_header("Auth Login Configuration", 0);
    AUTH_cli_cmd_agent_login(req, 1, 1, 1, 1);
}
#endif /* VTSS_CLI_SEC_GRP */

static void AUTH_cli_cmd_debug_auth(cli_req_t *req)
{
    auth_cli_req_t *auth_req = req->module_req;
    vtss_auth_dbg(cli_printf, auth_req->agent, (char *)auth_req->username, req->parm);
}
#endif /* AUTH_INCLUDE_CLIENT_CONFIG */

#if defined(VTSS_SW_OPTION_RADIUS) || defined(VTSS_SW_OPTION_TACPLUS)
static void AUTH_cli_cmd_global_host_conf(cli_req_t *req, vtss_auth_proto_t proto, BOOL timeout, BOOL retransmit, BOOL deadtime, BOOL key)
{
    auth_cli_req_t               *auth_req = req->module_req;
    vtss_auth_global_host_conf_t conf;
    vtss_rc                      rc;

    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

    if (vtss_auth_mgmt_global_host_conf_get(proto, &conf) == VTSS_OK) {
        if (req->set) {
            if (timeout) {
                conf.timeout = req->value;
            }
            if (retransmit) {
                conf.retransmit = req->value;
            }
            if (deadtime) {
                conf.deadtime = req->value;
            }
            if (key) {
                strncpy(conf.key, auth_req->key, sizeof(conf.key));
            }
            if ((rc = vtss_auth_mgmt_global_host_conf_set(proto, &conf)) != VTSS_OK) {
                CPRINTF("%s\n", error_txt(rc));
            }
        } else {
            if (timeout) {
                CPRINTF("Global %s Server Timeout    : %u seconds\n", vtss_auth_proto_names[proto], conf.timeout);
            }
            if (retransmit) {
                CPRINTF("Global %s Server Retransmit : %u times\n", vtss_auth_proto_names[proto], conf.retransmit);
            }
            if (deadtime) {
                CPRINTF("Global %s Server Deadtime   : %u minutes\n", vtss_auth_proto_names[proto], conf.deadtime);
            }
            if (key) {
                CPRINTF("Global %s Server Key        : %s\n", vtss_auth_proto_names[proto], conf.key);
            }
        }
    }
}

static void AUTH_cli_cmd_host_add(cli_req_t *req, vtss_auth_proto_t proto)
{
    auth_cli_req_t        *auth_req = req->module_req;
    vtss_auth_host_conf_t conf;
    vtss_rc               rc;

    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

    memset(&conf, 0, sizeof(conf));
    strncpy(conf.host, req->host_name, sizeof(conf.host));
    conf.auth_port  = auth_req->auth_port;
    conf.acct_port  = auth_req->acct_port;
    conf.timeout    = auth_req->timeout;
    conf.retransmit = auth_req->retransmit;
    strncpy(conf.key, auth_req->key, sizeof(conf.key));

    if ((rc = vtss_auth_mgmt_host_add(proto, &conf)) != VTSS_OK) {
        CPRINTF("%s\n", error_txt(rc));
    }
}

static void AUTH_cli_cmd_host_del(cli_req_t *req, vtss_auth_proto_t proto)
{
    auth_cli_req_t        *auth_req = req->module_req;
    vtss_auth_host_conf_t conf;
    vtss_rc               rc;

    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

    memset(&conf, 0, sizeof(conf));
    strncpy(conf.host, req->host_name, sizeof(conf.host));
    conf.auth_port  = auth_req->auth_port;
    conf.acct_port  = auth_req->acct_port;

    if ((rc = vtss_auth_mgmt_host_del(proto, &conf)) != VTSS_OK) {
        CPRINTF("%s\n", error_txt(rc));
    }
}

static void AUTH_cli_cmd_host_show_entry(vtss_auth_proto_t proto, const void *const contxt, const vtss_auth_host_conf_t *const conf, int number)
{
    CPRINTF("%s Server #%d:\n", vtss_auth_proto_names[proto], number + 1);
    CPRINTF("  Host name  : %s\n", conf->host);
    if (proto == VTSS_AUTH_PROTO_RADIUS) {
        CPRINTF("  Auth port  : %u\n", conf->auth_port);
        CPRINTF("  Acct port  : %u\n", conf->acct_port);
    } else {
        CPRINTF("  Port       : %u\n", conf->auth_port);
    }
    if (conf->timeout) {
        CPRINTF("  Timeout    : %u seconds\n", conf->timeout);
    } else {
        CPRINTF("  Timeout    :\n");
    }
    if (proto == VTSS_AUTH_PROTO_RADIUS) {
        if (conf->retransmit) {
            CPRINTF("  Retransmit : %u times\n", conf->retransmit);
        } else {
            CPRINTF("  Retransmit :\n");
        }
    }
    CPRINTF("  Key        : %s\n", conf->key);
}

static void AUTH_cli_cmd_host_show(cli_req_t *req, vtss_auth_proto_t proto)
{
    vtss_rc rc;
    int     cnt;

    if ((rc = vtss_auth_mgmt_host_iterate(proto, NULL, AUTH_cli_cmd_host_show_entry, &cnt)) != VTSS_OK) {
        CPRINTF("%s\n", error_txt(rc));
    }
    if (!cnt) {
        CPRINTF("No hosts configured!\n");
    }
}
#endif /* defined(VTSS_SW_OPTION_RADIUS) || defined(VTSS_SW_OPTION_TACPLUS) */

#ifdef VTSS_SW_OPTION_RADIUS
static void AUTH_cli_cmd_radius_timeout(cli_req_t *req)
{
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_RADIUS, 1, 0, 0, 0);
}

static void AUTH_cli_cmd_radius_retransmit(cli_req_t *req)
{
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_RADIUS, 0, 1, 0, 0);
}

static void AUTH_cli_cmd_radius_deadtime(cli_req_t *req)
{
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_RADIUS, 0, 0, 1, 0);
}

static void AUTH_cli_cmd_radius_key(cli_req_t *req)
{
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_RADIUS, 0, 0, 0, 1);
}

static void AUTH_cli_cmd_radius_conf(cli_req_t *req, BOOL ipv4, BOOL ipv6, BOOL id)
{
    vtss_auth_radius_conf_t conf;
    vtss_rc                 rc;

    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

    if (vtss_auth_mgmt_radius_conf_get(&conf) == VTSS_OK) {
        if (req->set) {
            if (ipv4) {
                if (req->ipv4_addr_spec == CLI_SPEC_VAL) {
                    conf.nas_ip_address = req->ipv4_addr;
                    conf.nas_ip_address_enable = TRUE;
                } else {
                    conf.nas_ip_address_enable = FALSE;
                }
            }
#ifdef VTSS_SW_OPTION_IPV6
            if (ipv6) {
                if (req->ipv6_addr_spec == CLI_SPEC_VAL) {
                    conf.nas_ipv6_address = req->ipv6_addr;
                    conf.nas_ipv6_address_enable = TRUE;
                } else {
                    conf.nas_ipv6_address_enable = FALSE;
                }
            }
#endif /* VTSS_SW_OPTION_IPV6 */
            if (id) {
                strncpy(conf.nas_identifier, req->parm, sizeof(conf.nas_identifier));
                conf.nas_identifier[sizeof(conf.nas_identifier) - 1] = '\0';
            }
            if ((rc = vtss_auth_mgmt_radius_conf_set(&conf)) != VTSS_OK) {
                CPRINTF("%s\n", error_txt(rc));
            }
        } else {
            char buf[INET6_ADDRSTRLEN];
            if (ipv4) {
                CPRINTF("Global %s NAS-IP-Address    : %s\n",
                        vtss_auth_proto_names[VTSS_AUTH_PROTO_RADIUS],
                        conf.nas_ip_address_enable ? misc_ipv4_txt(conf.nas_ip_address, buf) : "");
            }
#ifdef VTSS_SW_OPTION_IPV6
            if (ipv6) {
                CPRINTF("Global %s NAS-IPv6-Address  : %s\n",
                        vtss_auth_proto_names[VTSS_AUTH_PROTO_RADIUS],
                        conf.nas_ipv6_address_enable ? misc_ipv6_txt(&conf.nas_ipv6_address, buf) : "");
            }
#endif /* VTSS_SW_OPTION_IPV6 */
            if (id) {
                CPRINTF("Global %s NAS-Identifier    : %s\n",
                        vtss_auth_proto_names[VTSS_AUTH_PROTO_RADIUS],
                        conf.nas_identifier);
            }
        }
    }
}

static void AUTH_cli_cmd_radius_nas_ip_address(cli_req_t *req)
{
    AUTH_cli_cmd_radius_conf(req, 1, 0, 0);
}

#ifdef VTSS_SW_OPTION_IPV6
static void AUTH_cli_cmd_radius_nas_ipv6_address(cli_req_t *req)
{
    AUTH_cli_cmd_radius_conf(req, 0, 1, 0);
}
#endif /* VTSS_SW_OPTION_IPV6 */

static void AUTH_cli_cmd_radius_nas_identifier(cli_req_t *req)
{
    AUTH_cli_cmd_radius_conf(req, 0, 0, 1);
}

static void AUTH_cli_cmd_radius_host_add(cli_req_t *req)
{
    AUTH_cli_cmd_host_add(req, VTSS_AUTH_PROTO_RADIUS);
}

static void AUTH_cli_cmd_radius_host_del(cli_req_t *req)
{
    AUTH_cli_cmd_host_del(req, VTSS_AUTH_PROTO_RADIUS);
}

static void AUTH_cli_cmd_radius_host_show(cli_req_t *req)
{
    AUTH_cli_cmd_host_show(req, VTSS_AUTH_PROTO_RADIUS);
}

static void AUTH_cli_cmd_radius_statistics_entry(vtss_auth_proto_t proto, const void *const contxt, const vtss_auth_host_conf_t *const conf, int number)
{
    vtss_radius_auth_client_server_mib_s mib_auth;
    vtss_radius_acct_client_server_mib_s mib_acct;
    vtss_rc                              rc;
    char                                 buf[128];

    if (proto != VTSS_AUTH_PROTO_RADIUS) {
        cli_printf("Error: Statistics not supported for %s\n", vtss_auth_proto_names[proto]);
        return;
    }

    if ((rc = vtss_radius_auth_client_mib_get(number, &mib_auth)) != VTSS_OK) {
        cli_printf("Error: %s\n", error_txt(rc));
        return;
    }

    cli_printf("\n%s Server #%d (%s:%d) Authentication Statistics:\n",
               vtss_auth_proto_names[proto],
               number + 1,
               misc_ipv4_txt(mib_auth.radiusAuthServerInetAddress, buf),
               mib_auth.radiusAuthClientServerInetPortNumber);

    cli_cmd_stati("Rx Access Accepts",           "Tx Access Requests",        mib_auth.radiusAuthClientExtAccessAccepts,            mib_auth.radiusAuthClientExtAccessRequests);
    cli_cmd_stati("Rx Access Rejects",           "Tx Access Retransmissions", mib_auth.radiusAuthClientExtAccessRejects,            mib_auth.radiusAuthClientExtAccessRetransmissions);
    cli_cmd_stati("Rx Access Challenges",        "Tx Pending Requests",       mib_auth.radiusAuthClientExtAccessChallenges,         mib_auth.radiusAuthClientExtPendingRequests);
    cli_cmd_stati("Rx Malformed Acc. Responses", "Tx Timeouts",               mib_auth.radiusAuthClientExtMalformedAccessResponses, mib_auth.radiusAuthClientExtTimeouts);
    cli_cmd_stati("Rx Bad Authenticators",       NULL,                        mib_auth.radiusAuthClientExtBadAuthenticators,        0);
    cli_cmd_stati("Rx Unknown Types",            NULL,                        mib_auth.radiusAuthClientExtUnknownTypes,             0);
    cli_cmd_stati("Rx Packets Dropped",          NULL,                        mib_auth.radiusAuthClientExtPacketsDropped,           0);
    cli_printf("%-27s %10s", "State:",
               mib_auth.state == VTSS_RADIUS_SERVER_STATE_DISABLED  ? "Disabled"  :
               mib_auth.state == VTSS_RADIUS_SERVER_STATE_NOT_READY ? "Not Ready" :
               mib_auth.state == VTSS_RADIUS_SERVER_STATE_READY     ? "Ready"     :
               "Dead");

    if (mib_auth.state == VTSS_RADIUS_SERVER_STATE_DEAD) {
        cli_printf(" (%u seconds left)", mib_auth.dead_time_left_secs);
    }
    cli_printf("\n%-27s %10u ms\n", "Round-Trip Time:", mib_auth.radiusAuthClientExtRoundTripTime * ECOS_MSECS_PER_HWTICK);

    if ((rc = vtss_radius_acct_client_mib_get(number, &mib_acct)) != VTSS_OK) {
        cli_printf("Error: %s\n", error_txt(rc));
        return;
    }

    cli_printf("\n%s Server #%d (%s:%d) Accounting Statistics:\n",
               vtss_auth_proto_names[proto],
               number + 1,
               misc_ipv4_txt(mib_acct.radiusAccServerInetAddress, buf),
               mib_acct.radiusAccClientServerInetPortNumber);

    cli_cmd_stati("Rx Responses",           "Tx Requests",         mib_acct.radiusAccClientExtResponses,          mib_acct.radiusAccClientExtRequests);
    cli_cmd_stati("Rx Malformed Responses", "Tx Retransmissions",  mib_acct.radiusAccClientExtMalformedResponses, mib_acct.radiusAccClientExtRetransmissions);
    cli_cmd_stati("Rx Bad Authenticators",  "Tx Pending Requests", mib_acct.radiusAccClientExtBadAuthenticators,  mib_acct.radiusAccClientExtPendingRequests);
    cli_cmd_stati("Rx Unknown Types",       "Tx Timeouts",         mib_acct.radiusAccClientExtUnknownTypes,       mib_acct.radiusAccClientExtTimeouts);
    cli_cmd_stati("Rx Packets Dropped",     NULL,                  mib_acct.radiusAccClientExtPacketsDropped,     0);
    cli_printf("%-27s %10s", "State:",
               mib_acct.state == VTSS_RADIUS_SERVER_STATE_DISABLED  ? "Disabled"  :
               mib_acct.state == VTSS_RADIUS_SERVER_STATE_NOT_READY ? "Not Ready" :
               mib_acct.state == VTSS_RADIUS_SERVER_STATE_READY     ? "Ready"     :
               "Dead");
    if (mib_acct.state == VTSS_RADIUS_SERVER_STATE_DEAD) {
        cli_printf(" (%u seconds left)", mib_acct.dead_time_left_secs);
    }
    cli_printf("\n%-27s %10u ms\n", "Round-Trip Time:", mib_acct.radiusAccClientExtRoundTripTime * ECOS_MSECS_PER_HWTICK);
}

static void AUTH_cli_cmd_radius_statistics(cli_req_t *req)
{
    vtss_rc rc;
    int     cnt;

    if (req->value) {
        AUTH_cli_cmd_radius_statistics_entry(VTSS_AUTH_PROTO_RADIUS, NULL, NULL, req->value - 1);
    } else {
        if ((rc = vtss_auth_mgmt_host_iterate(VTSS_AUTH_PROTO_RADIUS, NULL, AUTH_cli_cmd_radius_statistics_entry, &cnt)) != VTSS_OK) {
            CPRINTF("%s\n", error_txt(rc));
        }
        if (!cnt) {
            CPRINTF("No hosts configured!\n");
        }
    }
}

#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
static void AUTH_cli_cmd_tacacs_timeout(cli_req_t *req)
{
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_TACACS, 1, 0, 0, 0);
}

static void AUTH_cli_cmd_tacacs_deadtime(cli_req_t *req)
{
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_TACACS, 0, 0, 1, 0);
}

static void AUTH_cli_cmd_tacacs_key(cli_req_t *req)
{
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_TACACS, 0, 0, 0, 1);
}

static void AUTH_cli_cmd_tacacs_host_add(cli_req_t *req)
{
    AUTH_cli_cmd_host_add(req, VTSS_AUTH_PROTO_TACACS);
}

static void AUTH_cli_cmd_tacacs_host_del(cli_req_t *req)
{
    AUTH_cli_cmd_host_del(req, VTSS_AUTH_PROTO_TACACS);
}

static void AUTH_cli_cmd_tacacs_host_show(cli_req_t *req)
{
    AUTH_cli_cmd_host_show(req, VTSS_AUTH_PROTO_TACACS);
}
#endif /* VTSS_SW_OPTION_TACPLUS */

static void AUTH_cli_cmd_conf(cli_req_t *req)
{
    if (cli_cmd_switch_none(req) || cli_cmd_slave(req)) {
        return;
    }

#ifdef VTSS_CLI_SEC_GRP
    if (!req->set) {
        cli_header("AAA Configuration", 0);
    }
#else
    if (!req->set) {
        cli_header("Auth Configuration", 1);
    }

#ifdef AUTH_INCLUDE_CLIENT_CONFIG
    cli_header("Auth Login Configuration", 0);
    AUTH_cli_cmd_agent_login(req, 1, 1, 1, 1);
#endif /* AUTH_INCLUDE_CLIENT_CONFIG */
#endif /* VTSS_CLI_SEC_GRP */

#ifdef VTSS_SW_OPTION_RADIUS
    cli_header("Global RADIUS Configuration", 0);
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_RADIUS, 1, 1, 1, 1);
    AUTH_cli_cmd_radius_conf(req, 1, 1, 1);
    cli_header("RADIUS Host Configuration", 0);
    AUTH_cli_cmd_radius_host_show(req);
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
    cli_header("Global TACACS+ Configuration", 0);
    AUTH_cli_cmd_global_host_conf(req, VTSS_AUTH_PROTO_TACACS, 1, 0, 1, 1);
    cli_header("TACACS+ Host Configuration", 0);
    AUTH_cli_cmd_tacacs_host_show(req);
#endif /* VTSS_SW_OPTION_TACPLUS */

}

/****************************************************************************/
/*  Parameter parse functions                                               */
/****************************************************************************/

#ifdef AUTH_INCLUDE_CLIENT_CONFIG
static int AUTH_cli_parse_keyword(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    char *found = cli_parse_find(cmd, stx);
    auth_cli_req_t *auth_req = req->module_req;

    if (found) {
        if (!strncmp(found, vtss_auth_method_names[VTSS_AUTH_METHOD_NONE], strlen(vtss_auth_method_names[VTSS_AUTH_METHOD_NONE]))) {
            if (auth_req->ac_ix < VTSS_AUTH_METHOD_LAST) {
                auth_req->ac.method[auth_req->ac_ix++] = VTSS_AUTH_METHOD_NONE;
            }
        } else if (!strncmp(found, vtss_auth_method_names[VTSS_AUTH_METHOD_LOCAL], strlen(vtss_auth_method_names[VTSS_AUTH_METHOD_LOCAL]))) {
            if (auth_req->ac_ix < VTSS_AUTH_METHOD_LAST) {
                auth_req->ac.method[auth_req->ac_ix++] = VTSS_AUTH_METHOD_LOCAL;
            }
        } else if (!strncmp(found, vtss_auth_method_names[VTSS_AUTH_METHOD_RADIUS], strlen(vtss_auth_method_names[VTSS_AUTH_METHOD_RADIUS]))) {
            if (auth_req->ac_ix < VTSS_AUTH_METHOD_LAST) {
                auth_req->ac.method[auth_req->ac_ix++] = VTSS_AUTH_METHOD_RADIUS;
            }
        } else if (!strncmp(found, vtss_auth_method_names[VTSS_AUTH_METHOD_TACACS], strlen(vtss_auth_method_names[VTSS_AUTH_METHOD_TACACS]))) {
            if (auth_req->ac_ix < VTSS_AUTH_METHOD_LAST) {
                auth_req->ac.method[auth_req->ac_ix++] = VTSS_AUTH_METHOD_TACACS;
            }
        } else if (!strncmp(found, vtss_auth_agent_names[VTSS_AUTH_AGENT_CONSOLE], strlen(vtss_auth_agent_names[VTSS_AUTH_AGENT_CONSOLE]))) {
            auth_req->agent = VTSS_AUTH_AGENT_CONSOLE;
        } else if (!strncmp(found, vtss_auth_agent_names[VTSS_AUTH_AGENT_TELNET], strlen(vtss_auth_agent_names[VTSS_AUTH_AGENT_TELNET]))) {
            auth_req->agent = VTSS_AUTH_AGENT_TELNET;
        } else if (!strncmp(found, vtss_auth_agent_names[VTSS_AUTH_AGENT_SSH], strlen(vtss_auth_agent_names[VTSS_AUTH_AGENT_SSH]))) {
            auth_req->agent = VTSS_AUTH_AGENT_SSH;
        } else if (!strncmp(found, vtss_auth_agent_names[VTSS_AUTH_AGENT_HTTP], strlen(vtss_auth_agent_names[VTSS_AUTH_AGENT_HTTP]))) {
            auth_req->agent = VTSS_AUTH_AGENT_HTTP;
        }
    }
    return (found == NULL ? 1 : 0);
}

static int AUTH_cli_parse_username(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    auth_cli_req_t *auth_req = req->module_req;

    return cli_parse_string(cmd_org, (char *)auth_req->username, 1, sizeof(auth_req->username));
}

static int AUTH_cli_parse_password(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    return cli_parse_text(cmd_org, req->parm, sizeof(req->parm));
}
#endif /* AUTH_INCLUDE_CLIENT_CONFIG */

#ifdef VTSS_SW_OPTION_RADIUS
static int AUTH_cli_parse_retransmit(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    return cli_parse_ulong(cmd, &req->value, VTSS_AUTH_RETRANSMIT_MIN, VTSS_AUTH_RETRANSMIT_MAX);
}

static int AUTH_cli_parse_ipv4_addr(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int error = cli_parse_ipv4(cmd, &req->ipv4_addr, NULL, &req->ipv4_addr_spec, FALSE);
    if (error) {
        error = cli_parse_disable(cmd);
    }
    return error;
}

#ifdef VTSS_SW_OPTION_IPV6
static int AUTH_cli_parse_ipv6_addr(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int error = cli_parse_ipv6(cmd, &req->ipv6_addr, &req->ipv6_addr_spec);
    if (error) {
        error = cli_parse_disable(cmd);
    }
    return error;
}
#endif /*VTSS_SW_OPTION_IPV6*/

static int AUTH_cli_parse_identifier(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    return cli_parse_quoted_string(cmd_org, req->parm, sizeof(req->parm));
}

static int AUTH_cli_parse_host_auth_port(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    auth_cli_req_t *auth_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "auth_port"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        error = cli_parse_ulong(cmd2, &auth_req->auth_port, 0, 0xffff);
    }
    return error;
}

static int AUTH_cli_parse_host_acct_port(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    auth_cli_req_t *auth_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "acct_port"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        error = cli_parse_ulong(cmd2, &auth_req->acct_port, 0, 0xffff);
    }
    return error;
}

static int AUTH_cli_parse_host_retransmit(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    auth_cli_req_t *auth_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "retransmit"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        error = cli_parse_ulong(cmd2, &auth_req->retransmit, VTSS_AUTH_RETRANSMIT_MIN, VTSS_AUTH_RETRANSMIT_MAX);
    }
    return error;
}

static int AUTH_cli_parse_host_index(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    return cli_parse_ulong(cmd, &req->value, 1, VTSS_AUTH_NUMBER_OF_SERVERS);
}
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
static int AUTH_cli_parse_host_port(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    auth_cli_req_t *auth_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "port"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        error = cli_parse_ulong(cmd2, &auth_req->auth_port, 0, 0xffff);
    }
    return error;
}
#endif /* VTSS_SW_OPTION_TACPLUS */

#if defined(VTSS_SW_OPTION_RADIUS) || defined(VTSS_SW_OPTION_TACPLUS)
static int AUTH_cli_parse_timeout(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    return cli_parse_ulong(cmd, &req->value, VTSS_AUTH_TIMEOUT_MIN, VTSS_AUTH_TIMEOUT_MAX);
}

static int AUTH_cli_parse_deadtime(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    return cli_parse_ulong(cmd, &req->value, VTSS_AUTH_DEADTIME_MIN, VTSS_AUTH_DEADTIME_MAX);
}

static int AUTH_cli_parse_host_timeout(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int            error;
    auth_cli_req_t *auth_req = req->module_req;

    if (!(error = cli_parse_word(cmd, "timeout"))) {
        if (cmd2 == NULL) {
            return 1;
        }
        req->parm_parsed = 2;
        error = cli_parse_ulong(cmd2, &auth_req->timeout, VTSS_AUTH_TIMEOUT_MIN, VTSS_AUTH_TIMEOUT_MAX);
    }
    return error;
}

static int AUTH_cli_parse_key(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    auth_cli_req_t *auth_req = req->module_req;

    return cli_parse_quoted_string(cmd_org, auth_req->key, sizeof(auth_req->key));
}
#endif  /* defined(VTSS_SW_OPTION_RADIUS) || defined(VTSS_SW_OPTION_TACPLUS) */

/****************************************************************************/
/*  Parameter table                                                         */
/****************************************************************************/
static cli_parm_t auth_cli_parm_table[] = {
#ifdef AUTH_INCLUDE_CLIENT_CONFIG
    {
        "<user_name>",
        "A string identifying the user name that this entry should belong to",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_username,
        NULL
    },
    {
        "<password>",
        "The password for this user name",
        CLI_PARM_FLAG_NONE,
        AUTH_cli_parse_password,
        AUTH_cli_cmd_debug_auth
    },
    {
        AUTH_agent_names,
        AUTH_agent_help,
        CLI_PARM_FLAG_NO_TXT,
        AUTH_cli_parse_keyword,
        AUTH_cli_cmd_debug_auth
    },
    {
        AUTH_all_method_names,
        AUTH_all_method_help,
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        AUTH_cli_parse_keyword,
        NULL
    },
    {
        AUTH_method_names,
        AUTH_method_help,
        CLI_PARM_FLAG_NO_TXT | CLI_PARM_FLAG_SET,
        AUTH_cli_parse_keyword,
        NULL
    },
#endif /* AUTH_INCLUDE_CLIENT_CONFIG */

#ifdef VTSS_SW_OPTION_RADIUS
    {
        "<host_index>",
        "The host index (1-" vtss_xstr(VTSS_AUTH_NUMBER_OF_SERVERS) ")\n"
        "(default: Show statistics for all hosts)",
        CLI_PARM_FLAG_NO_TXT,
        AUTH_cli_parse_host_index,
        AUTH_cli_cmd_radius_statistics
    },
    {
        "<timeout>",
        "Global RADIUS server response timeout ("vtss_xstr(VTSS_AUTH_TIMEOUT_MIN)"-"vtss_xstr(VTSS_AUTH_TIMEOUT_MAX)" seconds))",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_timeout,
        AUTH_cli_cmd_radius_timeout
    },
    {
        "<retransmit>",
        "Global RADIUS server retries ("vtss_xstr(VTSS_AUTH_RETRANSMIT_MIN)"-"vtss_xstr(VTSS_AUTH_RETRANSMIT_MAX)" times))",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_retransmit,
        AUTH_cli_cmd_radius_retransmit
    },
    {
        "<deadtime>",
        "Global RADIUS server deadtime ("vtss_xstr(VTSS_AUTH_DEADTIME_MIN)"-"vtss_xstr(VTSS_AUTH_DEADTIME_MAX)" minutes))",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_deadtime,
        AUTH_cli_cmd_radius_deadtime
    },
    {
        "<key>",
        "Global RADIUS server secret key (1-63 characters)",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_key,
        AUTH_cli_cmd_radius_key,
    },
    {
        "<ipv4_addr>|disable",
        "IPv4 address (a.b.c.d) or 'disable'",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_ipv4_addr,
        NULL,
    },
#ifdef VTSS_SW_OPTION_IPV6
    {
        "<ipv6_addr>|disable",
        "IPv6 host address or 'disable'.\n"
        "IPv6 address is in 128-bit records represented as eight fields\n"
        "               of up to four hexadecimal digits with a colon separates each\n"
        "               field (:). For example, four hexadecimal digits with a colon\n"
        "               separates each field (:). For example,\n"
        "               'fe80::215:c5ff:fe03:4dc7'. The symbol '::' is a special\n"
        "               syntax that can be used as a shorthand way of representing\n"
        "               multiple 16-bit groups of contiguous zeros; but it can only\n"
        "               appear once. It also used a following legally IPv4 address.\n"
        "               For example,'::192.1.2.34'.",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_ipv6_addr,
        NULL,
    },
#endif
    {
        "<id>",
        "1-"vtss_xstr(VTSS_AUTH_HOST_LEN)" characters. Use \"\" to disable the identifier.",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_identifier,
        NULL,
    },
    {
        "<auth_port>",
        "UDP destination port for authentication requests (auth_port 0-65535, default: "vtss_xstr(VTSS_AUTH_RADIUS_AUTH_PORT_DEFAULT)")",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_host_auth_port,
        NULL,
    },
    {
        "<acct_port>",
        "UDP destination port for accounting requests (acct_port 0-65535, default: "vtss_xstr(VTSS_AUTH_RADIUS_ACCT_PORT_DEFAULT)")",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_host_acct_port,
        NULL,
    },
    {
        "<timeout>",
        "RADIUS server response timeout (timeout "vtss_xstr(VTSS_AUTH_TIMEOUT_MIN)"-"vtss_xstr(VTSS_AUTH_TIMEOUT_MAX)")",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_host_timeout,
        AUTH_cli_cmd_radius_host_add,
    },
    {
        "<retransmit>",
        "RADIUS server retries (retransmit "vtss_xstr(VTSS_AUTH_RETRANSMIT_MIN)"-"vtss_xstr(VTSS_AUTH_RETRANSMIT_MAX)")",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_host_retransmit,
        AUTH_cli_cmd_radius_host_add,
    },
    {
        "<key>",
        "RADIUS server secret key (1-63 characters)",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_key,
        AUTH_cli_cmd_radius_host_add,
    },
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
    {
        "<timeout>",
        "Global TACACS+ server response timeout ("vtss_xstr(VTSS_AUTH_TIMEOUT_MIN)"-"vtss_xstr(VTSS_AUTH_TIMEOUT_MAX)" seconds))",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_timeout,
        AUTH_cli_cmd_tacacs_timeout
    },
    {
        "<deadtime>",
        "Global TACACS+ server deadtime ("vtss_xstr(VTSS_AUTH_DEADTIME_MIN)"-"vtss_xstr(VTSS_AUTH_DEADTIME_MAX)" minutes))",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_deadtime,
        AUTH_cli_cmd_tacacs_deadtime
    },
    {
        "<key>",
        "Global TACACS+ server secret key (1-63 characters)",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_key,
        AUTH_cli_cmd_tacacs_key,
    },
    {
        "<port>",
        "TCP destination port for requests (port 0-65535, default: "vtss_xstr(VTSS_AUTH_TACACS_PORT_DEFAULT)")",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_host_port,
        NULL,
    },
    {
        "<timeout>",
        "TACACS+ server response timeout (timeout "vtss_xstr(VTSS_AUTH_TIMEOUT_MIN)"-"vtss_xstr(VTSS_AUTH_TIMEOUT_MAX)")",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_host_timeout,
        AUTH_cli_cmd_tacacs_host_add,
    },
    {
        "<key>",
        "TACACS+ server secret key (1-63 characters)",
        CLI_PARM_FLAG_SET,
        AUTH_cli_parse_key,
        AUTH_cli_cmd_tacacs_host_add,
    },
#endif /* VTSS_SW_OPTION_TACPLUS */

    {NULL, NULL, 0, 0, NULL}
};

/****************************************************************************/
/*  Command table                                                           */
/****************************************************************************/
/* AUTH CLI Command Sorting Order */
enum {
    CLI_CMD_AUTH_CONF_PRIO = 0,
    CLI_CMD_AUTH_AGENT_CONF_PRIO,
    CLI_CMD_AUTH_AGENT_LOGIN_CONSOLE_PRIO,
    CLI_CMD_AUTH_AGENT_LOGIN_TELNET_PRIO,
    CLI_CMD_AUTH_AGENT_LOGIN_SSH_PRIO,
    CLI_CMD_AUTH_AGENT_LOGIN_HTTP_PRIO,
    CLI_CMD_AUTH_RADIUS_TIMEOUT_PRIO,
    CLI_CMD_AUTH_RADIUS_RETRANSMIT_PRIO,
    CLI_CMD_AUTH_RADIUS_DEADTIME_PRIO,
    CLI_CMD_AUTH_RADIUS_KEY_PRIO,
    CLI_CMD_AUTH_RADIUS_NAS_IP_ADDRESS_PRIO,
    CLI_CMD_AUTH_RADIUS_NAS_IPV6_ADDRESS_PRIO,
    CLI_CMD_AUTH_RADIUS_NAS_IDENTIFIER_PRIO,
    CLI_CMD_AUTH_RADIUS_HOST_ADD_PRIO,
    CLI_CMD_AUTH_RADIUS_HOST_DEL_PRIO,
    CLI_CMD_AUTH_RADIUS_HOST_SHOW_PRIO,
    CLI_CMD_AUTH_RADIUS_STATISTICS_PRIO,
    CLI_CMD_AUTH_TACACS_TIMEOUT_PRIO,
    CLI_CMD_AUTH_TACACS_DEADTIME_PRIO,
    CLI_CMD_AUTH_TACACS_KEY_PRIO,
    CLI_CMD_AUTH_TACACS_HOST_ADD_PRIO,
    CLI_CMD_AUTH_TACACS_HOST_DEL_PRIO,
    CLI_CMD_AUTH_TACACS_HOST_SHOW_PRIO,
    CLI_CMD_DEBUG_AUTH_PRIO = CLI_CMD_SORT_KEY_DEFAULT,
};

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "Configuration",
    NULL,
    "Show Auth configuration",
    CLI_CMD_AUTH_CONF_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_conf,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_SYS_CONF
);

#ifdef AUTH_INCLUDE_CLIENT_CONFIG
#ifdef VTSS_CLI_SEC_GRP
cli_cmd_tab_entry(
    CLI_AUTH_PATH_SWITCH "Configuration",
    NULL,
    "Show Auth configuration",
    CLI_CMD_AUTH_AGENT_CONF_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_agent_conf,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_SYS_CONF
);
#endif /* VTSS_CLI_SEC_GRP */

#ifdef VTSS_AUTH_ENABLE_CONSOLE
cli_cmd_tab_entry(
    AUTH_login_ro_syntax[VTSS_AUTH_AGENT_CONSOLE],
    AUTH_login_rw_syntax[VTSS_AUTH_AGENT_CONSOLE],
    "Set or show Auth login method for Console",
    CLI_CMD_AUTH_AGENT_LOGIN_CONSOLE_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_agent_login_console,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_AUTH_ENABLE_CONSOLE */

#ifdef VTSS_SW_OPTION_CLI_TELNET
cli_cmd_tab_entry(
    AUTH_login_ro_syntax[VTSS_AUTH_AGENT_TELNET],
    AUTH_login_rw_syntax[VTSS_AUTH_AGENT_TELNET],
    "Set or show Auth login method for Telnet",
    CLI_CMD_AUTH_AGENT_LOGIN_TELNET_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_agent_login_telnet,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_SW_OPTION_CLI_TELNET */

#ifdef VTSS_SW_OPTION_SSH
cli_cmd_tab_entry(
    AUTH_login_ro_syntax[VTSS_AUTH_AGENT_SSH],
    AUTH_login_rw_syntax[VTSS_AUTH_AGENT_SSH],
    "Set or show Auth login method for SSH",
    CLI_CMD_AUTH_AGENT_LOGIN_SSH_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_agent_login_ssh,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_SW_OPTION_SSH */

#ifdef VTSS_SW_OPTION_WEB
cli_cmd_tab_entry(
    AUTH_login_ro_syntax[VTSS_AUTH_AGENT_HTTP],
    AUTH_login_rw_syntax[VTSS_AUTH_AGENT_HTTP],
    "Set or show Auth login method for HTTP",
    CLI_CMD_AUTH_AGENT_LOGIN_HTTP_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_agent_login_http,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_SW_OPTION_WEB */

cli_cmd_tab_entry(
    NULL,
    AUTH_debug_rw_syntax,
    "Authenticate via a remote server",
    CLI_CMD_DEBUG_AUTH_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_DEBUG,
    AUTH_cli_cmd_debug_auth,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* AUTH_INCLUDE_CLIENT_CONFIG */

#ifdef VTSS_SW_OPTION_RADIUS
cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server timeout",
    CLI_AUTH_PATH_AAA "radius-server timeout [<timeout>]",
    "Set or show global RADIUS timeout value",
    CLI_CMD_AUTH_RADIUS_TIMEOUT_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_timeout,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server retransmit",
    CLI_AUTH_PATH_AAA "radius-server retransmit [<retransmit>]",
    "Set or show global RADIUS retransmit value",
    CLI_CMD_AUTH_RADIUS_RETRANSMIT_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_retransmit,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server deadtime",
    CLI_AUTH_PATH_AAA "radius-server deadtime [<deadtime>]",
    "Set or show global RADIUS deadtime value",
    CLI_CMD_AUTH_RADIUS_DEADTIME_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_deadtime,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server key",
    CLI_AUTH_PATH_AAA "radius-server key [<key>]",
    "Set or show global RADIUS secret key",
    CLI_CMD_AUTH_RADIUS_KEY_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_key,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server nas-ip-address",
    CLI_AUTH_PATH_AAA "radius-server nas-ip-address [<ipv4_addr>|disable]",
    "Set or show NAS-IP-Address (RADIUS attribute 4)",
    CLI_CMD_AUTH_RADIUS_NAS_IP_ADDRESS_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_nas_ip_address,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

#ifdef VTSS_SW_OPTION_IPV6
cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server nas-ipv6-address",
    CLI_AUTH_PATH_AAA "radius-server nas-ipv6-address [<ipv6_addr>|disable]",
    "Set or show NAS-IPv6-Address (RADIUS attribute 95)",
    CLI_CMD_AUTH_RADIUS_NAS_IPV6_ADDRESS_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_nas_ipv6_address,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_SW_OPTION_IPV6 */

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server nas-identifier",
    CLI_AUTH_PATH_AAA "radius-server nas-identifier [<id>]",
    "Set or show NAS-Identifier (RADIUS attribute 32)",
    CLI_CMD_AUTH_RADIUS_NAS_IDENTIFIER_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_nas_identifier,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    NULL,
    CLI_AUTH_PATH_AAA "radius-server host add <ip_addr_string> [<auth_port>] [<acct_port>] [<timeout>] [<retransmit>] [<key>]",
    "Add or modify a RADIUS host",
    CLI_CMD_AUTH_RADIUS_HOST_ADD_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_host_add,
    AUTH_cli_default_radius,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    NULL,
    CLI_AUTH_PATH_AAA "radius-server host delete <ip_addr_string> [<auth_port>] [<acct_port>]",
    "Delete a RADIUS host",
    CLI_CMD_AUTH_RADIUS_HOST_DEL_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_host_del,
    AUTH_cli_default_radius,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server host show",
    NULL,
    "Show all RADIUS hosts",
    CLI_CMD_AUTH_RADIUS_HOST_SHOW_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_host_show,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "radius-server statistics [<host_index>]",
    NULL,
    "Show RADIUS statistics",
    CLI_CMD_AUTH_RADIUS_STATISTICS_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_radius_statistics,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "tacacs-server timeout",
    CLI_AUTH_PATH_AAA "tacacs-server timeout [<timeout>]",
    "Set or show global TACACS+ timeout value",
    CLI_CMD_AUTH_TACACS_TIMEOUT_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_tacacs_timeout,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "tacacs-server deadtime",
    CLI_AUTH_PATH_AAA "tacacs-server deadtime [<deadtime>]",
    "Set or show global TACACS+ deadtime value",
    CLI_CMD_AUTH_TACACS_DEADTIME_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_tacacs_deadtime,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "tacacs-server key",
    CLI_AUTH_PATH_AAA "tacacs-server key [<key>]",
    "Set or show global TACACS+ secret key",
    CLI_CMD_AUTH_TACACS_KEY_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_tacacs_key,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    NULL,
    CLI_AUTH_PATH_AAA "tacacs-server host add <ip_addr_string> [<port>] [<timeout>] [<key>]",
    "Add or modify a TACACS+ host",
    CLI_CMD_AUTH_TACACS_HOST_ADD_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_tacacs_host_add,
    AUTH_cli_default_tacacs,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    NULL,
    CLI_AUTH_PATH_AAA "tacacs-server host delete <ip_addr_string> [<port>]",
    "Delete a TACACS+ host",
    CLI_CMD_AUTH_TACACS_HOST_DEL_PRIO,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_tacacs_host_del,
    AUTH_cli_default_tacacs,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry(
    CLI_AUTH_PATH_AAA "tacacs-server host show",
    NULL,
    "Show all TACACS+ hosts",
    CLI_CMD_AUTH_TACACS_HOST_SHOW_PRIO,
    CLI_CMD_TYPE_STATUS,
    VTSS_MODULE_ID_SECURITY,
    AUTH_cli_cmd_tacacs_host_show,
    NULL,
    auth_cli_parm_table,
    CLI_CMD_FLAG_NONE
);
#endif /* VTSS_SW_OPTION_TACPLUS */

/****************************************************************************/
/*  Initialization                                                          */
/****************************************************************************/

void auth_cli_init(void)
{
#ifdef AUTH_INCLUDE_CLIENT_CONFIG
    AUTH_create_parser_strings();
#endif
    /* register the size required for auth req. structure */
    cli_req_size_register(sizeof(auth_cli_req_t));
}

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
