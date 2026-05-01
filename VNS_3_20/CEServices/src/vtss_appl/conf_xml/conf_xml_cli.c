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
#include "cli.h"
#include "cli_api.h"
#include "conf_xml_api.h"
#include "conf_xml_cli.h"
#include "vtss_module_id.h"
#include "cli_trace_def.h"
#include "firmware_api.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <tftp_support.h>

typedef struct {
    cli_spec_t  ipv4_server_spec;
    vtss_ipv4_t ipv4_server;
    /* Keywords */
    BOOL        check;
} conf_xml_cli_req_t;

void confxml_cli_req_init(void)
{
    /* register the size required for conf_xml req. structure */
    cli_req_size_register(sizeof(conf_xml_cli_req_t));
}

static void cli_cmd_config(cli_req_t *req, BOOL save)
{
    ulong              size;
    char               *data;
    conf_xml_get_t     get;
    conf_xml_set_t     set;
    char               buf[16];
    struct sockaddr_in host;
    int                res, err;
    char     err_msg_str[100];
    conf_xml_cli_req_t *conf_xml_cli_req;

    if (cli_cmd_slave(req)) {
        return;
    }

    conf_xml_cli_req = req->module_req;
    size = CONF_XML_FILE_MAX;
    if ((data = malloc(size)) == NULL) {
        CPRINTF("Out of memory\n");
        return;
    }

    memset(&host, 0, sizeof(host));
    host.sin_len = sizeof(host);
    host.sin_family = AF_INET;
    (void)inet_aton(misc_ipv4_txt(conf_xml_cli_req->ipv4_server, buf), &host.sin_addr);
    if (save) {
        get.data = data;
        get.size = size;
        if (conf_xml_file_get(&get) != VTSS_OK) {
            CPRINTF("Failed to create file\n");
        } else if ((res = tftp_put(req->parm, &host, data, get.size, TFTP_OCTET, &err)) > 0) {
            CPRINTF("Saved %d bytes to server\n", res);
        } else {
            firmware_tftp_err2str(err, &err_msg_str[0]);
            CPRINTF("%s \n", err_msg_str);
        }
    } else {
        if ((res = tftp_get(req->parm, &host, data, size, TFTP_OCTET, &err)) <= 0) {
            firmware_tftp_err2str(err, &err_msg_str[0]);
            CPRINTF("%s \n", err_msg_str);
        }  else {
            set.data = data;
            set.size = res;
            set.apply = (conf_xml_cli_req->check ? 0 : 1);
            CPRINTF("Loaded %d bytes from server. %s configuration...\n", res, set.apply ? "Applying" : "Checking");
            cli_flush();
            if (conf_xml_file_set(&set) == VTSS_OK) {
                CPRINTF("Done\n");
            } else {
                CPRINTF("\nError:\n%s\n\n", set.msg);
                CPRINTF("Line %u:\n%s\n", set.line_no, set.line);
            }
        }
    }

    free(data);
}

static void cli_cmd_config_save ( cli_req_t *req )
{
    cli_cmd_config(req, 1);
}

static void cli_cmd_config_load ( cli_req_t *req )
{
    cli_cmd_config(req, 0);
}

static int32_t cli_conf_xml_generic_keyword_parse(char *cmd, cli_req_t *req, char *stx)
{
    char *found = cli_parse_find(cmd, stx);
    conf_xml_cli_req_t *conf_xml_cli_req;

    T_I("ALL %s", found);

    conf_xml_cli_req = req->module_req;
    if (found != NULL) {
        if (!strncmp(found, "check", 5)) {
            conf_xml_cli_req->check = 1;
        }
    }
    return (found == NULL ? 1 : 0);
}

static int32_t cli_conf_xml_ip_server_parse(char *cmd, char *cmd2, char *stx, char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    conf_xml_cli_req_t *conf_xml_cli_req;

    req->parm_parsed = 1;
    conf_xml_cli_req = req->module_req;
    error = cli_parse_ipv4(cmd, &conf_xml_cli_req->ipv4_server, NULL, &conf_xml_cli_req->ipv4_server_spec, 0);

    return error;
}

static int32_t cli_conf_xml_file_name_parse(char *cmd, char *cmd2, char *stx,
                                            char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    req->parm_parsed = 1;
    error = cli_parse_raw(cmd_org, req);
    return error;
}

static int32_t cli_conf_xml_check_parse(char *cmd, char *cmd2, char *stx,
                                        char *cmd_org, cli_req_t *req)
{
    int32_t error = 0;
    req->parm_parsed = 1;
    error = cli_conf_xml_generic_keyword_parse(cmd, req, stx);
    return error;
}

static cli_parm_t conf_xml_cli_parm_table[] = {
    {
        "<ip_server>",
        "TFTP server IPv4 address (a.b.c.d)",
        CLI_PARM_FLAG_NONE,
        cli_conf_xml_ip_server_parse,
        NULL
    },
    {
        "<file_name>",
        "Configuration file name",
        CLI_PARM_FLAG_NONE,
        cli_conf_xml_file_name_parse,
        NULL
    },
    {
        "check",
        "Check configuration file only, default: Check and apply file",
        CLI_PARM_FLAG_NONE,
        cli_conf_xml_check_parse,
        NULL
    },
    {
        NULL,
        NULL,
        0,
        0,
        NULL
    },
};

enum {
    PRIO_CONFIG_SAVE,
    PRIO_CONFIG_LOAD
};

cli_cmd_tab_entry (
    NULL,
    "Config Save <ip_server> <file_name>",
    "Save configuration to TFTP server",
    PRIO_CONFIG_SAVE,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_MISC,
    cli_cmd_config_save,
    NULL,
    conf_xml_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

cli_cmd_tab_entry (
    NULL,
    "Config Load <ip_server> <file_name> [check]",
    "Load configuration from TFTP server",
    PRIO_CONFIG_LOAD,
    CLI_CMD_TYPE_CONF,
    VTSS_MODULE_ID_MISC,
    cli_cmd_config_load,
    NULL,
    conf_xml_cli_parm_table,
    CLI_CMD_FLAG_NONE
);

