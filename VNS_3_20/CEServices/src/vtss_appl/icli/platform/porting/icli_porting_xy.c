/*

 Vitesse Switch Application software.

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

/*
******************************************************************************

    Revision history
    > CP.Wang, 2011/05/02 14:58
        - Porting Layer.
        - except the API names, all others including the code body can be
          modified as need per project.

******************************************************************************
*/

/*
******************************************************************************

    Include File

******************************************************************************
*/
#include <stdio.h>
#include <unistd.h>

#include "icli.h"

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/

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
static char *g_mode_name[ICLI_CMD_MODE_MAX] = {
    "",               //ICLI_CMD_MODE_EXEC,
    "config",         //ICLI_CMD_MODE_GLOBAL_CONFIG,
    "config-vlan",    //ICLI_CMD_MODE_CONFIG_VLAN,
    "if-port",        //ICLI_CMD_MODE_INTERFACE_PORT_LIST
    "if-vlan",        //ICLI_CMD_MODE_INTERFACE_VLAN,
    "config-line",    //ICLI_CMD_MODE_CONFIG_LINE,
    "ipmc-profile",   //ICLI_CMD_MODE_IPMC_PROFILE,
    "snmps-host",     //ICLI_CMD_MODE_SNMPS_HOST,
    "stp-aggr",       //ICLI_CMD_MODE_STP_AGGR,
};

/*
******************************************************************************

    Static Function

******************************************************************************
*/
#define _SEQUENCE_INDICATOR     91  // [
#define _CURSOR_UP              65  // A
#define _CURSOR_DOWN            66  // B
#define _CURSOR_FORWARD         67  // C
#define _CURSOR_BACKWARD        68  // D

#define _CSI \
    icli_session_char_put(handle, ICLI_KEY_ESC); \
    icli_session_char_put(handle, _SEQUENCE_INDICATOR);

/*
******************************************************************************

    Public Function

******************************************************************************
*/
void icli_xy_cursor_up(
    IN icli_session_handle_t    *handle
)
{
    _CSI;
    icli_session_char_put(handle, _CURSOR_UP);
}

void icli_xy_cursor_down(
    IN icli_session_handle_t    *handle
)
{
    _CSI;
    icli_session_char_put(handle, _CURSOR_DOWN);
}

void icli_xy_cursor_forward(
    IN icli_session_handle_t    *handle
)
{
    i32     x, y;
    i32     max_x;

    // get current x,y
    icli_sutil_current_xy_get(handle, &x, &y);

    // get max x
    max_x = handle->runtime_data.width - 1;

#if 0 /* CP_DEBUG */
    T_E("\nx = %d, y = %d\n", x, y);
#endif

    if ( x < max_x ) {
        // just go forward
        _CSI;
        icli_session_char_put(handle, _CURSOR_FORWARD);
    } else {
        // go to the beginning of next line
        icli_xy_cursor_offset(handle, 0 - max_x, 1);
    }
}

void icli_xy_cursor_backward(
    IN icli_session_handle_t    *handle
)
{
    i32     x, y;

    // get current x,y
    icli_sutil_current_xy_get(handle, &x, &y);

#if 0 /* CP_DEBUG */
    T_E("\nx = %d, y = %d\n", x, y);
#endif

    if ( x ) {
        // just backward
        _CSI;
        icli_session_char_put(handle, _CURSOR_BACKWARD);
    } else {
        // go to the end of previous line
        icli_xy_cursor_offset(handle, handle->runtime_data.width - 1, -1);
    }

}

void icli_xy_cursor_offset(
    IN icli_session_handle_t    *handle,
    IN i32                      offset_x,
    IN i32                      offset_y
)
{
    char    s[32];

#if 0 /* CP_DEBUG */
    T_E("\noffset_x = %d, offset_y = %d\n", offset_x, offset_y);
#endif

    if ( offset_x > 0 ) {
        // forward
        _CSI;
        icli_sprintf(s, "%d", offset_x);
        icli_session_str_put(handle, s);
        icli_session_char_put(handle, _CURSOR_FORWARD);
    } else if ( offset_x < 0 ) {
        // backward
        _CSI;
        icli_sprintf(s, "%d", 0 - offset_x);
        icli_session_str_put(handle, s);
        icli_session_char_put(handle, _CURSOR_BACKWARD);
    }

    if ( offset_y > 0 ) {
        // down
        _CSI;
        icli_sprintf(s, "%d", offset_y);
        icli_session_str_put(handle, s);
        icli_session_char_put(handle, _CURSOR_DOWN);
    } else if ( offset_y < 0 ) {
        // up
        _CSI;
        icli_sprintf(s, "%d", 0 - offset_y);
        icli_session_str_put(handle, s);
        icli_session_char_put(handle, _CURSOR_UP);
    }
}

/*
    update cursor_pos, but not cmd_pos and cmd_len
*/
void icli_xy_cursor_backspace(
    IN  icli_session_handle_t   *handle
)
{
    i32     x, y;

    icli_xy_cursor_backward(handle);
    ICLI_PUT_SPACE;

    /*
        if just go backward when at the end of postion,
        then the cursor will go up one line.
        the root cause is if put char at the end of position
        then the cursor is still there and will not go to next line.
    */

    // get current x,y
    icli_sutil_current_xy_get(handle, &x, &y);
    if ( x == 0 ) {
        ICLI_PUT_SPACE;
        ICLI_PUT_BACKSPACE;
    }

    icli_xy_cursor_backward(handle);

    _DEC_1( handle->runtime_data.cursor_pos );
}

char *icli_xy_default_mode_name_get(
    IN  icli_cmd_mode_t     mode
)
{
    if ( mode >= ICLI_CMD_MODE_MAX ) {
        return "";
    }
    return g_mode_name[mode];
}

const char *icli_xy_error_txt(
    IN vtss_rc  rc
)
{
    return error_txt(rc);
}
