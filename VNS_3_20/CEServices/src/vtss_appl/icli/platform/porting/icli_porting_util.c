/*

 Vitesse Switch Application software.

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

/*
******************************************************************************

    Include File

******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "icli_api.h"
#include "icli_porting_util.h"
#include "icli_porting_trace.h"

#include "mgmt_api.h"   //mgmt_iport_list2txt()
#include "misc_api.h"   //uport2iport(), iport2uport()
#include "port_api.h"   //switch_iter_init(), port_iter_init()
#if defined(VTSS_SWITCH_STACKABLE) && VTSS_SWITCH_STACKABLE
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#else
#include "standalone_api.h"
#endif /* VTSS_SWITCH_STACKABLE */

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/
#define ICLI_PORTING_STR_BUF_SIZE   80

/*
******************************************************************************

    Type Definition

******************************************************************************
*/

/*
******************************************************************************

    Static Variable

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/
/* Display header text with optional new line before and after */
static void icli_header_nl_char(i32 session_id, char *txt, BOOL pre, BOOL post, char c)
{
    int i, len;

    if (pre) {
        ICLI_PRINTF("\n");
    }
    ICLI_PRINTF("%s:\n", txt);
    len = (strlen(txt) + 1);
    for (i = 0; i < len; i++) {
        ICLI_PRINTF("%c", c);
    }
    ICLI_PRINTF("\n");
    if (post) {
        ICLI_PRINTF("\n");
    }
}

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/* Get enabled/disabled text */
char *icli_bool_txt(BOOL enabled)
{
    return (enabled ? "enabled" : "disabled");
}

/* Get port information text */
char *icli_port_info_txt(vtss_usid_t usid, vtss_port_no_t uport, char *str_buf_p)
{
    icli_switch_port_range_t switch_range;

    switch_range.usid = usid;
    switch_range.begin_uport = uport;
    switch_range.port_cnt = 1;
    str_buf_p[0] = '\0';
    if (icli_port_from_usid_uport(&switch_range)) {
        sprintf(str_buf_p, "%s %u/%u",
                icli_port_type_get_name(switch_range.port_type),
                switch_range.switch_id,
                switch_range.begin_port);
    }
    return str_buf_p;
}

/* Display port header text with optional new line before and after */
void icli_port_header(i32 session_id, vtss_isid_t usid, vtss_port_no_t uport, char *txt, BOOL pre, BOOL post)
{
    char str_buf[ICLI_PORTING_STR_BUF_SIZE];

    (void) icli_port_info_txt(usid, uport, str_buf);
    if (txt) {
        sprintf(str_buf + strlen(str_buf), " %s", txt);
    }
    icli_header_nl_char(session_id, str_buf, pre, post, '-');
}

static void icli_table_header_parm(i32 session_id, char *txt, BOOL parm)
{
    int i, j, len, count = 0;

    ICLI_PRINTF("%s\n", txt);
    while (*txt == ' ') {
        ICLI_PRINTF(" ");
        txt++;
    }
    len = strlen(txt);
    for (i = 0; i < len; i++) {
        //ignore CR/LF
        if (txt[i] == 0xa /* LF */ || txt[i] == 0xd /* CR */) {
            continue;
        } else if (txt[i] == ' ') {
            count++;
        } else {
            for (j = 0; j < count; j++) {
                ICLI_PRINTF("%c", count > 1 && (parm || j >= (count - 2)) ? ' ' : '-');
            }
            ICLI_PRINTF("-");
            count = 0;
        }
    }
    for (j = 0; j < count; j++) {
        ICLI_PRINTF("%c", count > 1 && (parm || j >= (count - 2)) ? ' ' : '-');
    }
    ICLI_PRINTF("\n");
}

/* Display underlined header with new line before and after */
void icli_header(i32 session_id, char *txt, BOOL post)
{
    icli_header_nl_char(session_id, txt, TRUE, post, '=');
}

/* Display header text with under line */
void icli_parm_header(i32 session_id, char *txt)
{
    icli_table_header_parm(session_id, txt, TRUE);
}

// Function printing the port interface and number - e.g. GiabitEthernet 1/1
// IN : Session_Id : The session_id
//      usid       : User switch id.
//      uport      : User port number
void icli_print_port_info_txt(i32 session_id, vtss_isid_t usid, vtss_port_no_t uport)
{
    char str_buf[ICLI_PORTING_STR_BUF_SIZE];
    (void) icli_port_info_txt(usid, uport, str_buf);
    ICLI_PRINTF("%s ", str_buf);
}

/* Display two counters in one row */
void icli_stats(i32 session_id, char *counter_str_1_p, char *counter_str_2_p, u32 counter_1, u32 counter_2)
{
    char str_buf[ICLI_PORTING_STR_BUF_SIZE];

    sprintf(str_buf, "%s:", counter_str_1_p);
    ICLI_PRINTF("%-28s%10u   ", str_buf, counter_1);
    if (counter_str_2_p != NULL) {
        sprintf(str_buf, "%s:", counter_str_2_p);
        ICLI_PRINTF("%-28s%10u", str_buf, counter_2);
    }
    ICLI_PRINTF("\n");
}

/* Port list string */
i8 *icli_iport_list_txt(BOOL iport_list[VTSS_PORT_ARRAY_SIZE], i8 *buf)
{
    if (strlen(mgmt_iport_list2txt(iport_list, buf)) == 0) {
        strcpy(buf, "None");
    }

    return buf;
}

char *icli_mac_txt(const uchar *mac, char *buf)
{
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

/* Convert to iport_list from icli_range_list.
   Return value -
    0 : success
    -1: fail - invalid parameter
    -2: fail - iport_list array size overflow */
int icli_rangelist2iportlist(icli_unsigned_range_t *icli_range_list_p, BOOL *iport_list_p, u32 iport_list_max_num)
{
    u32             range_idx, range_value;
    vtss_port_no_t  iport;

    if (!icli_range_list_p || !iport_list_p || !iport_list_max_num) {
        return -1; //invalid parameter
    }

    memset(iport_list_p, 0, sizeof(BOOL) * iport_list_max_num);

    for (range_idx = 0; range_idx < icli_range_list_p->cnt; range_idx++) {
        for (range_value = icli_range_list_p->range[range_idx].min; range_value <= icli_range_list_p->range[range_idx].max; range_value++) {
            iport = uport2iport(range_value);
            if (iport >= iport_list_max_num) {
                return -2; //iport_lis array size overflow
            }
            iport_list_p[iport] = TRUE;
        }
    }

    return 0;
}

/* Convert to iport_list from icli_port_type_list.
   Return value -
    0 : success
    -1: fail - invalid parameter
    -2: fail - iport_list array size overflow
    1 : fail - different switch ID existing in port type list */
int icli_porttypelist2iportlist(vtss_usid_t usid, icli_stack_port_range_t *port_type_list_p, BOOL *iport_list_p, u32 iport_list_max_num)
{
    u32             range_idx, cnt_idx;
    vtss_port_no_t  iport;

    if (!port_type_list_p || !iport_list_p || !iport_list_max_num) {
        return -1; //invalid parameter
    }

    memset(iport_list_p, 0, sizeof(BOOL) * iport_list_max_num);

    for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
        if (usid != VTSS_ISID_GLOBAL && usid != port_type_list_p->switch_range[range_idx].usid) {
            return 1; //different switch ID existing in port type list
        }
        for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
            iport = uport2iport(port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx);
            if (iport >= iport_list_max_num) {
                return -2; //iport_lis array size overflow
            }
            iport_list_p[iport] = TRUE;
        }
    }

    return 0;
}

/* Get port type text from switch_range */
static char *icli_switch_range_txt(BOOL *first_range_p, icli_switch_port_range_t *switch_range_p, char *str_buf_p)
{
    char temp_str_buf[8];

    sprintf(temp_str_buf, "-%u", switch_range_p->begin_port + switch_range_p->port_cnt - 1);
    sprintf(str_buf_p, "%s%s %u/%u%s",
            *first_range_p ? "" : ", ",
            icli_port_type_get_name(switch_range_p->port_type),
            switch_range_p->switch_id,
            switch_range_p->begin_port,
            switch_range_p->port_cnt == 1 ? "" : temp_str_buf);

    if (*first_range_p) {
        *first_range_p = FALSE;
    }
    return str_buf_p;
}


/* Get port list information text */
char *icli_port_list_info_txt(vtss_isid_t isid, BOOL *iport_list_p, char *str_buf_p)
{
    port_iter_t                 pit;
    icli_switch_port_range_t    pre_switch_range, switch_range;
    BOOL                        first = TRUE, pre_switch_range_exist = FALSE;

    pre_switch_range.usid = topo_isid2usid(isid);
    str_buf_p[0] = '\0';

    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (!iport_list_p[pit.iport]) {
            if (pre_switch_range_exist) {
                pre_switch_range_exist = FALSE;
                (void)icli_switch_range_txt(&first, &pre_switch_range, str_buf_p + strlen(str_buf_p));
            }
            continue;
        }

        if (pre_switch_range_exist) {
            switch_range = pre_switch_range;
            switch_range.port_cnt++;
            if (icli_port_from_usid_uport(&switch_range)) {
                pre_switch_range = switch_range;
                continue;
            } else {
                if (pre_switch_range_exist) {
                    pre_switch_range_exist = FALSE;
                    (void)icli_switch_range_txt(&first, &pre_switch_range, str_buf_p + strlen(str_buf_p));
                }
            }
        }

        pre_switch_range.begin_uport = pit.uport;
        pre_switch_range.port_cnt = 1;
        if (icli_port_from_usid_uport(&pre_switch_range)) {
            pre_switch_range_exist = TRUE;
        }
    }

    if (pre_switch_range_exist) {
        (void)icli_switch_range_txt(&first, &pre_switch_range, str_buf_p + strlen(str_buf_p));
    }

    return str_buf_p;
}

BOOL icli_uport_is_included(vtss_usid_t usid, vtss_port_no_t uport, icli_stack_port_range_t *plist)
{
    u32             rdx;
    u16             bgn, cnt;

    if (!plist) {
        return FALSE;
    }
    for (rdx = 0; rdx < plist->cnt; rdx++) {
        if (usid != plist->switch_range[rdx].usid) {
            continue;
        }
        cnt = plist->switch_range[rdx].port_cnt;
        bgn = plist->switch_range[rdx].begin_uport;
        if ((uport >= bgn) && (uport < bgn + cnt)) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Check if the usid is a member of the portlist */
BOOL icli_usid_is_included(vtss_usid_t usid, icli_stack_port_range_t *plist)
{
    u32             rdx;

    if (!plist) {
        return FALSE;
    }
    for (rdx = 0; rdx < plist->cnt; rdx++) {
        if (usid == plist->switch_range[rdx].usid) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Initialize iCLI switch iterator to iterate over all switches in USID order.
 * Use icli_switch_iter_getnext() to filter out non-selected, non-existing switches.
 */
vtss_rc icli_switch_iter_init(switch_iter_t *sit)
{
    return switch_iter_init(sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
}

/*
 * iCLI switch iterator. Returns selected switches.
 * If list == NULL, all existing switches are returned.
 * Updates sit->first to match first selected switch.
 * NOTE: sit->last is not updated and therefore unreliable.
 */
BOOL icli_switch_iter_getnext(switch_iter_t *sit, icli_stack_port_range_t *list)
{
    BOOL first = FALSE;
    u32  i;
    while (switch_iter_getnext(sit)) {
        if (sit->first) {
            first = TRUE;
        }
        if (list) {
            // Loop though the list and check that current switch is specified.
            for (i = 0; i < list->cnt; i++) {
                if (sit->usid == list->switch_range[i].usid) {
                    sit->first = first;
                    return TRUE;
                }
            }
        } else {
            return TRUE; // No list - return every switch.
        }
    }
    return FALSE; // Done - No more switches.
}

/*
 * Initialize iCLI port iterator to iterate over all ports in uport order.
 * Use icli_port_iter_getnext() to filter out non-selected ports.
 */
vtss_rc icli_port_iter_init(port_iter_t *pit, vtss_isid_t isid, port_iter_flags_t flags)
{
    return port_iter_init(pit, NULL, isid, PORT_ITER_SORT_ORDER_UPORT, flags);
}

/*
 * iCLI port iterator. Returns selected ports.
 * If list == NULL, all existing ports are returned.
 * Updates pit->first to match first selected port.
 * NOTE: pit->last is not updated and therefore unreliable.
 */
BOOL icli_port_iter_getnext(port_iter_t *pit, icli_stack_port_range_t *list)
{
    BOOL first = FALSE;
    u32  i;
    while (port_iter_getnext(pit)) {
        if (pit->first) {
            first = TRUE;
        }
        if (list) {
            // Loop though the list and check that current switch and port is specified.
            for (i = 0; i < list->cnt; i++) {
                T_I("uport:%d, usid:%d, port_cnt:%u", list->switch_range[i].begin_uport, list->switch_range[i].usid, list->switch_range[i].port_cnt);
                if (pit->m_isid == topo_usid2isid(list->switch_range[i].usid) &&
                    pit->uport >= list->switch_range[i].begin_uport &&
                    pit->uport < list->switch_range[i].begin_uport + list->switch_range[i].port_cnt) {
                    pit->first = first;
                    return TRUE;
                }
            }
        } else {
            return TRUE; // No list - return every port.
        }
    }
    return FALSE; // Done  - No more ports.
}
