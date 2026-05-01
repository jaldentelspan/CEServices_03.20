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
    > CP.Wang, 2011/06/30 17:54
        - create
        - these APIs are actually internal APIs only for icli_session*.c
          so, the prefix '_' is still there

******************************************************************************
*/
/*
******************************************************************************

    Include File

******************************************************************************
*/
#include "icli.h"

/*
******************************************************************************

    Constant

******************************************************************************
*/
#define _WAIT_TIME_SLICE    100

/*
******************************************************************************

    Constant

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

/*
******************************************************************************

    Static Function

******************************************************************************
*/

/*
******************************************************************************

    Public Function

******************************************************************************
*/
BOOL icli_sutil_ring_full(
    IN  i32     cnt,
    IN  i32     head,
    IN  i32     tail
)
{
#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
    if ( cnt == 0 ) {
        return TRUE;
    }
#endif

    if ( head > tail ) {
        if ( (head == cnt - 1) && (tail == 0) ) {
            return TRUE;
        }
        return FALSE;
    }

    if ( head < tail ) {
        if ( tail - head == 1 ) {
            return TRUE;
        }
        return FALSE;
    }

    return FALSE;
}

BOOL icli_sutil_ring_empty(
    IN  i32     cnt,
    IN  i32     head,
    IN  i32     tail
)
{
    if ( cnt ) {}
    if ( tail ) {}

#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
    if ( cnt == 0 ) {
        return TRUE;
    }
#endif

    if ( head == -1 ) {
        return TRUE;
    }
    return FALSE;
}

void icli_sutil_cmd_reset(
    IN  icli_session_handle_t   *handle
)
{
    memset(handle->runtime_data.cmd, 0, sizeof(handle->runtime_data.cmd));
    handle->runtime_data.cmd_pos      = 0;
    handle->runtime_data.cmd_len      = 0;
    handle->runtime_data.start_pos    = 0;
    handle->runtime_data.cursor_pos   = 0;
    handle->runtime_data.left_scroll  = 0;
    handle->runtime_data.right_scroll = 0;
#if 0 /* CP, 2012/08/30 09:12, do command by CLI command */
    handle->runtime_data.cmd_do       = NULL;
#endif
}

/*
    get session input by char

    INPUT
        handle    - session handle
        wait_time - in millisecond
                    = 0 - no wait
                    < 0 - forever

    OUTPUT
        c - user input char

    RETURN
        TRUE  - successful
        FALSE - timeout
*/
BOOL icli_sutil_usr_char_get(
    IN  icli_session_handle_t   *handle,
    IN  i32                     wait_time,
    OUT i32                     *c
)
{
    icli_session_char_get_t     *char_get_cb;
    i32                         app_id;
    BOOL                        b;
    i32                         n, t = _WAIT_TIME_SLICE;

    if ( handle->open_data.char_get == NULL ) {
        T_E("char_get == NULL\n");
        return FALSE;
    }

    // get milli-seconds for waiting
    if ( wait_time == 0 ) {
        // no wait
        n = 1;
        t = 0;
    } else if ( wait_time < 0 ) {
        // wait forever
        n = -1;
    } else {
        n = wait_time / _WAIT_TIME_SLICE;
        if ( wait_time % _WAIT_TIME_SLICE ) {
            n++;
        }
    }

    char_get_cb = handle->open_data.char_get;
    app_id = handle->open_data.app_id;

    handle->runtime_data.idle_time = icli_current_time_get();

    while ( n ) {

        _SEMA_GIVE();

        b = (*char_get_cb)(app_id, t, c);

        _SEMA_TAKE();

        if ( handle->runtime_data.alive == FALSE ) {
            return FALSE;
        }

        if ( b ) {
            /* get idle time */
            handle->runtime_data.idle_time = icli_current_time_get();
            return TRUE;
        }

        if ( n > 0 ) {
            n--;
        }
    }

    return FALSE;
}

/*
    get user input char according to session wait time

    INPUT
        handle - session handle
            handle->wait_time - in millisecond
                                = 0 - forever

    OUTPUT
        c - user input char

    RETURN
        TRUE  - successful
        FALSE - timeout
*/
BOOL icli_sutil_usr_char_get_by_session(
    IN  icli_session_handle_t   *handle,
    OUT i32                     *c
)
{
    icli_session_char_get_t     *char_get_cb;
    i32                         app_id;
    BOOL                        b;
    u32                         idle_time;

    if ( handle->open_data.char_get == NULL ) {
        T_E("char_get == NULL\n");
        return FALSE;
    }

    char_get_cb = handle->open_data.char_get;
    app_id = handle->open_data.app_id;

    handle->runtime_data.idle_time = icli_current_time_get();

    for (;;) {

        _SEMA_GIVE();

        b = (*char_get_cb)(app_id, _WAIT_TIME_SLICE, c);

        _SEMA_TAKE();

        if ( handle->runtime_data.alive == FALSE ) {
            return FALSE;
        }

        if ( b ) {
            /* get idle time */
            handle->runtime_data.idle_time = icli_current_time_get();
            return TRUE;
        } else {
            /* timeout, check if expired */
            if ( handle->config_data->wait_time > 0 ) {
                idle_time = icli_current_time_get() - handle->runtime_data.idle_time;
                if ( idle_time >= (handle->config_data->wait_time * 1000) ) {
                    break;
                }
            }
        }
    }

    /* expire */
    return FALSE;
}

/*
    output a char on session
*/
BOOL icli_sutil_usr_char_put(
    IN  icli_session_handle_t   *handle,
    IN  char                    c
)
{
    icli_session_char_put_t     *char_put_cb;
    i32                         app_id;
    BOOL                        b;

    if ( handle->open_data.char_put == NULL ) {
        return TRUE;
    }

    char_put_cb = handle->open_data.char_put;
    app_id = handle->open_data.app_id;

    _SEMA_GIVE();

    b = (*char_put_cb)(app_id, c);

    _SEMA_TAKE();

    return b;
}

/*
    output string on session
*/
BOOL icli_sutil_usr_str_put(
    IN  icli_session_handle_t   *handle
)
{
    icli_session_str_put_t  *str_put_cb;
    i32                     app_id;
    char                    *str;
    BOOL                    b;

    if ( handle->open_data.str_put == NULL ) {
        return TRUE;
    }

    str = handle->runtime_data.put_str;
    if ( str == NULL ) {
        T_E("handle->runtime_data.put_str == NULL\n");
        return FALSE;
    }

    str_put_cb = handle->open_data.str_put;
    app_id = handle->open_data.app_id;

    _SEMA_GIVE();

    b = (*str_put_cb)(app_id, str);

    _SEMA_TAKE();

    return b;
}

void icli_sutil_tab_reset(
    IN  icli_session_handle_t   *handle
)
{
    handle->runtime_data.tab_cnt = 0;
    handle->runtime_data.tab_port_type = ICLI_PORT_TYPE_NONE;
    icli_exec_para_list_free( &(handle->runtime_data.cmd_var) );
    handle->runtime_data.grep_var   = NULL;
    handle->runtime_data.grep_begin = 0;
}

void icli_sutil_more_prompt_clear(
    IN icli_session_handle_t    *handle
)
{
    i32     len;

    for ( len = icli_str_len(handle->runtime_data.put_str); len > 0; len-- ) {
        icli_xy_cursor_backward( handle );
        ICLI_PUT_SPACE;
        icli_xy_cursor_backward( handle );
    }
}

/*
    cmd_pos, cmd_len : updated
    cursor_pos       : not updated
*/
BOOL icli_sutil_cmd_char_add(
    IN  icli_session_handle_t   *handle,
    IN  i32                     c
)
{
    i32     i;

    /* not supported keys, skip */
    if ( ! ICLI_KEY_VISIBLE(c) ) {
        ICLI_PLAY_BELL;
        return FALSE;
    }

#if 0 /* CP, 2012/08/17 09:38, accept space at beginning */
    /* SPACE at the begin, skip */
    if ( ICLI_IS_(SPACE, c) && handle->runtime_data.cmd_pos == 0 ) {
        return FALSE;
    }
#endif

    /* full in length */
    if ( handle->runtime_data.cmd_len >= ICLI_STR_MAX_LEN ) {
        return FALSE;
    }

    /* cursor at middle, shift command */
    if ( ! _CURSOR_AT_CMD_END ) {
        for ( i = handle->runtime_data.cmd_len; i > handle->runtime_data.cmd_pos; i-- ) {
            handle->runtime_data.cmd[i] = handle->runtime_data.cmd[i - 1];
        }
    }

    /* put into cmd */
    handle->runtime_data.cmd[handle->runtime_data.cmd_pos] = (char)c;

    /* update cmd_pos */
    _INC_1( handle->runtime_data.cmd_pos );

    /* increase command length */
    _INC_1( handle->runtime_data.cmd_len );

    return TRUE;
}

void icli_sutil_current_xy_get(
    IN  icli_session_handle_t   *handle,
    OUT i32                     *x,
    OUT i32                     *y
)
{
    i32     len;

    len = handle->runtime_data.prompt_len + handle->runtime_data.cursor_pos;
    *x = len % (i32)(handle->runtime_data.width);
    *y = len / (i32)(handle->runtime_data.width);
}

void icli_sutil_end_xy_get(
    IN  icli_session_handle_t   *handle,
    OUT i32                     *x,
    OUT i32                     *y
)
{
    i32     len;

    len = handle->runtime_data.prompt_len + handle->runtime_data.cmd_len;
    *x = len % (i32)(handle->runtime_data.width);
    *y = len / (i32)(handle->runtime_data.width);
}
