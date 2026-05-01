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

#ifndef __ICLI_PORTING_UTIL_H__
#define __ICLI_PORTING_UTIL_H__

#include "port_api.h"
//****************************************************************************

/*
******************************************************************************

    Type

******************************************************************************
*/

/*
******************************************************************************

    Constant

******************************************************************************
*/

/*
******************************************************************************

    Macro Definition

******************************************************************************
*/

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/* Get enabled/disabled text */
char *icli_bool_txt(BOOL enabled);

/* Get port information text */
char *icli_port_info_txt(vtss_usid_t usid, vtss_port_no_t uport, char *str_buf_p);

/* Display port header text with optional new line before and after */
void icli_port_header(i32 session_id, vtss_isid_t usid, vtss_port_no_t uport, char *txt, BOOL pre, BOOL post);

/* Display underlined header with new line before and after */
void icli_header(i32 session_id, char *txt, BOOL post);

/* Display header text with under line */
void icli_parm_header(i32 session_id, char *txt);

/* Display two counters in one row */
void icli_stats(i32 session_id, char *counter_str_1_p, char *counter_str_2_p, u32 counter_1, u32 counter_2);

void icli_print_port_info_txt(i32 session_id, vtss_isid_t usid, vtss_port_no_t uport);

/* Port list string */
i8 *icli_iport_list_txt(BOOL iport_list[VTSS_PORT_ARRAY_SIZE], i8 *buf);



/* Returns a MAC address in a ICLI style */
char *icli_mac_txt(const uchar *mac, char *buf);
//****************************************************************************

#define VTSS_ICLI_RANGE_FOREACH(LIST, VAR_TYPE, VAR_NAME)               \
{                                                                       \
    u32 MACRO_VARIABLE_outer ## LIST;                                   \
    VAR_TYPE VAR_NAME;                                                  \
    for (MACRO_VARIABLE_outer ## LIST = 0;                              \
         MACRO_VARIABLE_outer ## LIST < LIST->cnt;                      \
         ++MACRO_VARIABLE_outer ## LIST) {                              \
        for (VAR_NAME = LIST->range[MACRO_VARIABLE_outer ## LIST].min;  \
             VAR_NAME <= LIST->range[MACRO_VARIABLE_outer ## LIST].max; \
             ++VAR_NAME)
#define VTSS_ICLI_RANGE_FOREACH_END() }}

/* Convert to iport_list from icli_range_list.
   Return value -
    0 : success
    -1: fail - invalid parameter
    -2: fail - iport_list array size overflow */
int icli_rangelist2iportlist(icli_unsigned_range_t *icli_range_list_p, BOOL *iport_list_p, u32 iport_list_max_num);

/* Convert to iport_list from icli_port_type_list.
   Return value -
    0 : success
    -1: fail - invalid parameter
    -2: fail - iport_list array size overflow
    1 : fail - different switch ID existing in port type list */
int icli_porttypelist2iportlist(vtss_usid_t usid, icli_stack_port_range_t *port_type_list_p, BOOL *iport_list_p, u32 iport_list_max_num);

/* Get port list information text */
char *icli_port_list_info_txt(vtss_isid_t isid, BOOL *iport_list_p, char *str_buf_p);

/* Is uport member of plist? */
BOOL icli_uport_is_included(vtss_usid_t usid, vtss_port_no_t uport, icli_stack_port_range_t *plist);

/* Is usid member of plist? */
BOOL icli_usid_is_included(vtss_usid_t usid, icli_stack_port_range_t *plist);

/*
 * Initialize iCLI switch iterator to iterate over all switches in USID order.
 * Use icli_switch_iter_getnext() to filter out non-selected, non-existing switches.
 */
vtss_rc icli_switch_iter_init(switch_iter_t *sit);

/*
 * iCLI switch iterator. Returns selected switches.
 * If list == NULL, all existing switches are returned.
 * Updates sit->first to match first selected switch.
 * NOTE: sit->last is not updated and therefore unreliable.
 */
BOOL icli_switch_iter_getnext(switch_iter_t *sit, icli_stack_port_range_t *list);

/*
 * Initialize iCLI port iterator to iterate over all ports in uport order.
 * Use icli_port_iter_getnext() to filter out non-selected ports.
 */
vtss_rc icli_port_iter_init(port_iter_t *pit, vtss_isid_t isid, port_iter_flags_t flags);

/*
 * iCLI port iterator. Returns selected ports.
 * If list == NULL, all existing ports are returned.
 * Updates pit->first to match first selected port.
 * NOTE: pit->last is not updated and therefore unreliable.
 */
BOOL icli_port_iter_getnext(port_iter_t *pit, icli_stack_port_range_t *list);

#endif //__ICLI_PORTING_UTIL_H__

