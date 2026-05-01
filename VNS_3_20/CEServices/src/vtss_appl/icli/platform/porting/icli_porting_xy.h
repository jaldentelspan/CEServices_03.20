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
    > CP.Wang, 2011/06/07 16:03
        - create

******************************************************************************
*/
#ifndef __ICLI_PORTING_XY_H__
#define __ICLI_PORTING_XY_H__
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
void icli_xy_cursor_up(
    IN icli_session_handle_t    *handle
);

void icli_xy_cursor_down(
    IN icli_session_handle_t    *handle
);

void icli_xy_cursor_forward(
    IN icli_session_handle_t    *handle
);

void icli_xy_cursor_backward(
    IN icli_session_handle_t    *handle
);

void icli_xy_cursor_offset(
    IN icli_session_handle_t    *handle,
    IN i32                      offset_x,
    IN i32                      offset_y
);

/*
    update cursor_pos, but not cmd_pos and cmd_len
*/
void icli_xy_cursor_backspace(
    IN  icli_session_handle_t   *handle
);

char *icli_xy_default_mode_name_get(
    IN  icli_cmd_mode_t     mode
);

const char *icli_xy_error_txt(
    IN vtss_rc  rc
);

//****************************************************************************
#endif //__ICLI_PORTING_XY_H__

