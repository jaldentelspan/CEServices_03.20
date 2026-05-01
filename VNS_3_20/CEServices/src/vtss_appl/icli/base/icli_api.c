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
    > CP.Wang, 2011/05/02 17:55
        - create

******************************************************************************
*/
/*
******************************************************************************

    Include File

******************************************************************************
*/
#include "icli.h"

/*
    ICLI engine is protected by locking at public APIs and thread,
    so it does not need to take semaphore every C file.
*/
//lint --e{429}
//lint -sem(icli_init,                              thread_protected)
//lint -sem(icli_conf_get,                          thread_protected)
//lint -sem(icli_conf_apply,                        thread_protected)
//lint -sem(icli_conf_default,                      thread_protected)
//lint -sem(icli_input_style_get,                   thread_protected)
//lint -sem(icli_input_style_set,                   thread_protected)
//lint -sem(icli_cmd_register,                      thread_protected)
//lint -sem(icli_cmd_is_enable,                     thread_protected)
//lint -sem(icli_cmd_enable,                        thread_protected)
//lint -sem(icli_cmd_disable,                       thread_protected)
//lint -sem(icli_cmd_is_visible,                    thread_protected)
//lint -sem(icli_cmd_visible,                       thread_protected)
//lint -sem(icli_cmd_invisible,                     thread_protected)
//lint -sem(icli_cmd_privilege_get,                 thread_protected)
//lint -sem(icli_cmd_privilege_set,                 thread_protected)
//lint -sem(icli_session_open,                      thread_protected)
//lint -sem(icli_session_close,                     thread_protected)
//lint -sem(icli_session_engine,                    thread_protected)
//lint -sem(icli_session_all_close,                 thread_protected)
//lint -sem(icli_session_max_get,                   thread_protected)
//lint -sem(icli_session_max_set,                   thread_protected)
//lint -sem(icli_session_cmd_exec,                  thread_protected)
//lint -sem(icli_session_cmd_exec_err_display,      thread_protected)
//lint -sem(icli_session_cmd_exec_err_file_line,    thread_protected)
//lint -sem(icli_session_cmd_parsing_begin,         thread_protected)
//lint -sem(icli_session_cmd_parsing_end,           thread_protected)
//lint -sem(icli_session_printf,                    thread_protected)
//lint -sem(icli_session_printf_lstr,               thread_protected)
//lint -sem(icli_session_printf_to_all,             thread_protected)
//lint -sem(icli_session_usr_str_get,               thread_protected)
//lint -sem(icli_session_ctrl_c_get,                thread_protected)
//lint -sem(icli_session_mode_enter,                thread_protected)
//lint -sem(icli_session_mode_exit,                 thread_protected)
//lint -sem(icli_session_data_get,                  thread_protected)
//lint -sem(icli_session_data_get_next,             thread_protected)
//lint -sem(icli_session_history_size_get,          thread_protected)
//lint -sem(icli_session_history_size_set,          thread_protected)
//lint -sem(icli_session_history_cmd_get_first,     thread_protected)
//lint -sem(icli_session_history_cmd_get,           thread_protected)
//lint -sem(icli_session_history_cmd_get_next,      thread_protected)
//lint -sem(icli_session_privilege_get,             thread_protected)
//lint -sem(icli_session_privilege_set,             thread_protected)
//lint -sem(icli_session_width_set,                 thread_protected)
//lint -sem(icli_session_lines_set,                 thread_protected)
//lint -sem(icli_session_wait_time_set,             thread_protected)
//lint -sem(icli_session_line_mode_get,             thread_protected)
//lint -sem(icli_session_line_mode_set,             thread_protected)
//lint -sem(icli_session_input_style_get,           thread_protected)
//lint -sem(icli_session_input_style_set,           thread_protected)
//lint -sem(icli_session_exec_banner_enable,        thread_protected)
//lint -sem(icli_session_motd_banner_enable,        thread_protected)
//lint -sem(icli_session_location_set,              thread_protected)
//lint -sem(icli_session_privileged_level_set,      thread_protected)
//lint -sem(icli_session_user_name_set,             thread_protected)
//lint -sem(icli_session_self_id,                   thread_protected)
//lint -sem(icli_session_self_printf,               thread_protected)
//lint -sem(icli_session_self_printf_lstr,          thread_protected)
//lint -sem(icli_session_rc_check_setup,            thread_protected)
//lint -sem(icli_session_rc_check,                  thread_protected)
//lint -sem(icli_mode_name_set,                     thread_protected)
//lint -sem(icli_dev_name_set,                      thread_protected)
//lint -sem(icli_dev_name_get,                      thread_protected)
//lint -sem(icli_banner_login_get,                  thread_protected)
//lint -sem(icli_banner_login_set,                  thread_protected)
//lint -sem(icli_banner_motd_get,                   thread_protected)
//lint -sem(icli_banner_motd_set,                   thread_protected)
//lint -sem(icli_banner_exec_get,                   thread_protected)
//lint -sem(icli_banner_exec_set,                   thread_protected)
//lint -sem(icli_port_type_present,                 thread_protected)
//lint -sem(icli_port_type_get_name,                thread_protected)
//lint -sem(icli_port_range_reset,                  thread_protected)
//lint -sem(icli_port_range_get,                    thread_protected)
//lint -sem(icli_port_range_set,                    thread_protected)
//lint -sem(icli_port_from_usid_uport,              thread_protected)
//lint -sem(icli_port_get_first,                    thread_protected)
//lint -sem(icli_port_get,                          thread_protected)
//lint -sem(icli_port_get_next,                     thread_protected)
//lint -sem(icli_enable_password_get,               thread_protected)
//lint -sem(icli_enable_password_set,               thread_protected)
//lint -sem(icli_enable_password_verify,            thread_protected)
//lint -sem(icli_enable_secret_set,                 thread_protected)
//lint -sem(icli_enable_secret_clear_set,           thread_protected)
//lint -sem(icli_enable_password_if_secret_get,     thread_protected)
//lint -sem(icli_debug_prompt_set,                  thread_protected)
//lint -sem(icli_debug_prompt_get,                  thread_protected)

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

/*
******************************************************************************

    Static Function

******************************************************************************
*/
/*
    get history command of the session

    INPUT
        session_id           : session ID
        history->history_pos : INDEX

    OUTPUT
        history_cmd : the history command of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _session_history_cmd_get(
    IN    icli_session_handle_t       *handle,
    INOUT icli_session_history_cmd_t  *history
)
{
    if ( history == NULL ) {
        T_E("history == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    /* if history is empty */
    if ( _history_empty(handle) ) {
        return ICLI_RC_ERR_EMPTY;
    }

    /* check history_pos */
    if ( handle->runtime_data.history_head > handle->runtime_data.history_tail ) {
        if ( history->history_pos > handle->runtime_data.history_head ||
             history->history_pos < handle->runtime_data.history_tail ) {
            return ICLI_RC_ERR_PARAMETER;
        }
    } else if ( handle->runtime_data.history_head < handle->runtime_data.history_tail ) {
        if ( history->history_pos > handle->runtime_data.history_head &&
             history->history_pos < handle->runtime_data.history_tail ) {
            return ICLI_RC_ERR_PARAMETER;
        }
    } else {
        if ( history->history_pos != handle->runtime_data.history_head ) {
            return ICLI_RC_ERR_PARAMETER;
        }
    }

    /* get history */
    (void)icli_str_cpy( history->history_cmd, handle->runtime_data.history_cmd[history->history_pos]);
    return ICLI_RC_OK;
}

/*
    execute a command on a session

    INPUT
        session              : session ID
        cmd                  : command to be executed
        cmd_len              : length of cmd
        b_exec               : TRUE  - execute the command function
                               FALSE - parse the command only to check if the command is legal or not,
                                       but not execute
        app_err_msg          : error message from application or config file name according to line_number
        line_number          : line number in the config file
                               if -1
                               then app_err_msg is application error message
                               else app_err_msg is config file name
        b_display_err_syntax : TRUE  - display error syntax
                               FALSE - not display

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _session_cmd_exec(
    IN icli_session_handle_t    *handle,
    IN char                     *cmd,
    IN BOOL                     b_exec,
    IN char                     *app_err_msg,
    IN i32                      line_number,
    IN BOOL                     b_display_err_syntax
)
{
    icli_exec_type_t        orig_exec_type;
    i32                     rc;
    icli_parameter_t        *orig_cmd_var;
    u32                     cmd_len;

    /* session is alive or not */
    if ( handle->runtime_data.alive == FALSE ) {
        return ICLI_RC_ERR_PARAMETER;
    }

    /* skip space at beginning */
    ICLI_SPACE_SKIP(cmd, cmd);
    cmd_len = icli_str_len( cmd );

    /* pack cmd */
    // reset all cmd to be 0, otherwise, it will introduce issue for <line>
    memset(handle->runtime_data.cmd, 0, sizeof(handle->runtime_data.cmd));
    (void)icli_str_cpy(handle->runtime_data.cmd, cmd);
    handle->runtime_data.cmd_len   = cmd_len;
    handle->runtime_data.cmd_pos   = cmd_len;

    /* store original exec type */
    orig_exec_type = handle->runtime_data.exec_type;

    /* set corresponding exec type */
    if ( handle->runtime_data.b_in_parsing ) {
        handle->runtime_data.exec_type = ICLI_EXEC_TYPE_PARSING;
    } else {
        handle->runtime_data.exec_type = b_exec ? ICLI_EXEC_TYPE_CMD : ICLI_EXEC_TYPE_PARSING;
    }

    // store original command variables
    orig_cmd_var = handle->runtime_data.cmd_var;
    handle->runtime_data.cmd_var = NULL;

    // exec by API
    handle->runtime_data.b_exec_by_api = TRUE;

    // get application error message
    handle->runtime_data.app_err_msg = app_err_msg;
    handle->runtime_data.line_number = line_number;

    // display error syntax
    handle->runtime_data.err_display_mode = b_display_err_syntax ?
                                            ICLI_ERR_DISPLAY_MODE_PRINT : ICLI_ERR_DISPLAY_MODE_DROP;

    /* execute command */
    rc = icli_exec( handle );

    /* restore original exec type */
    handle->runtime_data.exec_type = orig_exec_type;

    // restore original command variables
    if ( orig_cmd_var ) {
        icli_exec_para_list_free( &(handle->runtime_data.cmd_var) );
        handle->runtime_data.cmd_var = orig_cmd_var;
    }

    // reset
    handle->runtime_data.b_exec_by_api        = FALSE;
    handle->runtime_data.app_err_msg          = NULL;
    handle->runtime_data.err_display_mode     = ICLI_ERR_DISPLAY_MODE_PRINT;

    return rc;
}

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/*
    initialize ICLI engine

    INPUT
        init : data for initialization

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_init(
    IN  icli_init_data_t     *init_data
)
{
    if ( init_data == NULL ) {
        T_E("init_data == NULL\n");
        return ICLI_RC_ERROR;
    }

    return icli_engine_init( init_data );
}

#if 1 /* CP, 2012/09/14 10:38, conf */
/*
    get ICLI engine config data

    INPUT
        b_sem - whether to use semaphore protection or not

    OUTPUT
        conf - config data to get

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_conf_get(
    OUT icli_conf_data_t    *conf,
    IN  BOOL                b_sem
)
{
    if ( conf == NULL ) {
        T_E("conf == NULL\n");
        return FALSE;
    }

    if ( b_sem ) {
        _SEMA_TAKE();
    }

    memcpy(conf, icli_conf_get_x(), sizeof(icli_conf_data_t));

    if ( b_sem ) {
        _SEMA_GIVE();
    }

    return TRUE;
}

/*
    apply config data to ICLI engine

    INPUT
        conf - config data to set

    OUTPUT
        n/a

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_conf_apply(
    IN  icli_conf_data_t     *conf
)
{
    icli_session_handle_t   *handle;
    u32                     i;

    if ( conf == NULL ) {
        T_E("conf == NULL\n");
        return FALSE;
    }

    _SEMA_TAKE();

    memcpy(icli_conf_get_x(), conf, sizeof(icli_conf_data_t));

    // apply to runtime
    for ( i = 0; i < ICLI_SESSION_CNT; i++ ) {
        handle = icli_session_handle_get(i);

        // reset history
        handle->runtime_data.history_head     = -1;
        handle->runtime_data.history_tail     = -1;

        // width
        if ( conf->session_config[i].width == 0 ) {
            handle->runtime_data.width = ICLI_MAX_WIDTH;
        } else if ( conf->session_config[i].width < ICLI_MIN_WIDTH ) {
            handle->runtime_data.width = ICLI_MIN_WIDTH;
        } else {
            handle->runtime_data.width = conf->session_config[i].width;
        }

        // lines
        if ( conf->session_config[i].lines == 0 ) {
            handle->runtime_data.lines = 0;
        } else if ( conf->session_config[i].lines < ICLI_MIN_LINES ) {
            handle->runtime_data.lines = ICLI_MIN_LINES;
        } else {
            handle->runtime_data.lines = conf->session_config[i].lines;
        }
    }

    _SEMA_GIVE();

    return TRUE;
}

/*
    reset config data to default

    INPUT
        n/a

    OUTPUT
        conf - config data to reset default

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_conf_default(
    OUT  icli_conf_data_t     *conf
)
{
    u32     i;

    if ( conf == NULL ) {
        T_E("conf == NULL\n");
        return FALSE;
    }

    // reset all
    memset(conf, 0, sizeof(icli_conf_data_t));

    // device name
    (void)icli_str_ncpy(conf->dev_name, ICLI_DEFAULT_DEVICE_NAME, ICLI_NAME_MAX_LEN);

    // mode name
    for ( i = 0; i < ICLI_CMD_MODE_MAX; i++ ) {
        (void)icli_str_ncpy(conf->mode_name[i], icli_xy_default_mode_name_get(i), ICLI_PROMPT_MAX_LEN);
    }

    // reset session config data to default
    for ( i = 0; i < ICLI_SESSION_CNT; i++ ) {
        (void)icli_session_config_data_default( &(conf->session_config[i]) );
    }
    return TRUE;
}
#endif

/*
    get whick input_style the ICLI engine perform

    input -
        n/a

    output -
        input_style: current input style

    return -
        icli_rc_t

    comment -
        n/a
*/
i32 icli_input_style_get(
    OUT icli_input_style_t  *input_style
)
{
    if ( input_style == NULL ) {
        T_E("input_style == NULL\n");
        return ICLI_RC_ERROR;
    }

    _SEMA_TAKE();

    *input_style = icli_input_style_get_x();

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set whick input_style the ICLI engine perform

    input -
        input_style : input style

    output -
        n/a

    return -
        icli_rc_t

    comment -
        n/a
*/
i32 icli_input_style_set(
    IN icli_input_style_t   input_style
)
{
    icli_rc_t   rc;

    _SEMA_TAKE();

    rc = icli_input_style_set_x( input_style );

    _SEMA_GIVE();

    return rc;
}

/*
    register ICLI command

    INPUT
        cmd_register : command data

    OUTPUT
        n/a

    RETURN
        >= 0 : command ID
        < 0  : icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_register(
    IN  icli_cmd_register_t     *cmd_register
)
{
    i32     cmd_id;

    _SEMA_TAKE();
    cmd_id = icli_register_cmd_x( cmd_register );
    _SEMA_GIVE();
    return cmd_id;
}

/*
    get if the command is enabled or disabled

    INPUT
        cmd_id : command ID

    OUTPUT
        enable : TRUE  - the command is enabled
                 FALSE - the command is disabled

    RETURN
        ICLI_RC_OK : get successfully
        ICLI_RC_ERR_PARAMETER : fail to get

    COMMENT
        n/a
*/
i32 icli_cmd_is_enable(
    IN  u32     cmd_id,
    OUT BOOL    *enable
)
{
    i32     rc;

    _SEMA_TAKE();
    rc = icli_register_cmd_is_enable( cmd_id, enable );
    _SEMA_GIVE();
    return rc;
}

/*
    enable the command

    INPUT
        cmd_id : command ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_enable(
    IN  u32     cmd_id
)
{
    i32     rc;

    _SEMA_TAKE();
    rc = icli_register_cmd_enable( cmd_id );
    _SEMA_GIVE();
    return rc;
}

/*
    disable the command

    INPUT
        cmd_id : command ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_disable(
    IN  u32     cmd_id
)
{
    i32     rc;

    _SEMA_TAKE();
    rc = icli_register_cmd_disable( cmd_id );
    _SEMA_GIVE();
    return rc;
}

/*
    get if the command is visible or invisible

    INPUT
        cmd_id : command ID

    OUTPUT
        visible : TRUE  - the command is visible
                  FALSE - the command is invisible

    RETURN
        ICLI_RC_OK : get successfully
        ICLI_RC_ERR_PARAMETER : fail to get

    COMMENT
        n/a
*/
i32 icli_cmd_is_visible(
    IN  u32     cmd_id,
    OUT BOOL    *visible
)
{
    i32     rc;

    _SEMA_TAKE();
    rc = icli_register_cmd_is_visible( cmd_id, visible );
    _SEMA_GIVE();
    return rc;
}

/*
    make the command visible to user

    INPUT
        cmd_id : command ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_visible(
    IN  u32     cmd_id
)
{
    i32     rc;

    _SEMA_TAKE();
    rc = icli_register_cmd_visible( cmd_id );
    _SEMA_GIVE();
    return rc;
}

/*
    make the command invisible to user

    INPUT
        cmd_id : command ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_invisible(
    IN  u32     cmd_id
)
{
    i32     rc;

    _SEMA_TAKE();
    rc = icli_register_cmd_invisible( cmd_id );
    _SEMA_GIVE();
    return rc;
}

/*
    get privilege of a command

    INPUT
        cmd_id : command ID

    OUTPUT
        privilege : session privilege

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_privilege_get(
    IN  u32     cmd_id,
    OUT u32     *privilege
)
{
    i32     rc;

    _SEMA_TAKE();
    rc = icli_register_cmd_privilege_get( cmd_id, privilege );
    _SEMA_GIVE();
    return rc;
}

/*
    set privilege of a command

    INPUT
        cmd_id    : command ID
        privilege : session privilege

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_cmd_privilege_set(
    IN u32     cmd_id,
    IN u32     privilege
)
{
    i32     rc;

    _SEMA_TAKE();
    rc = icli_register_cmd_privilege_set( cmd_id, privilege );
    _SEMA_GIVE();
    return rc;
}

/*
    open a ICLI session

    INPUT
        open_data : data for session open

    OUTPUT
        session_id : ID of session opened successfully

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_open(
    IN  icli_session_open_data_t    *open_data,
    OUT u32                         *session_id
)
{
    i32     rc;

    if ( open_data == NULL ) {
        T_E("open_data == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( session_id == NULL ) {
        T_E("session_id == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    switch ( open_data->way ) {
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        if ( open_data->char_get == NULL ) {
            T_E("open_data->char_get == NULL\n");
            return ICLI_RC_ERR_PARAMETER;
        }
        if ( open_data->str_put == NULL ) {
            T_E("open_data->str_put == NULL\n");
            return ICLI_RC_ERR_PARAMETER;
        }
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
#if 0
        if ( open_data->str_get == NULL ) {
            T_E("open_data->str_get == NULL\n");
            return ICLI_RC_ERR_PARAMETER;
        }
        if ( open_data->error_get == NULL ) {
            T_E("open_data->error_get == NULL\n");
            return ICLI_RC_ERR_PARAMETER;
        }
#endif
        break;

    default:
        T_E("invalid session type = %d\n", open_data->way);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();
    rc = icli_session_open_x( open_data, session_id );
    _SEMA_GIVE();
    return rc;
}

/*
    close a ICLI session

    INPUT
        session_id : session ID from icli_session_open()

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_close(
    IN u32  session_id
)
{
    i32    rc;

    _SEMA_TAKE();
    rc = icli_session_close_x( session_id );
    _SEMA_GIVE();
    return rc;
}

/*
    close all ICLI sessions

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_session_all_close(void)
{
    u32     session_id;

    _SEMA_TAKE();

    for ( session_id = 0; session_id < ICLI_SESSION_CNT; session_id++ ) {
        (void)icli_session_close_x( session_id );
    }

    _SEMA_GIVE();
}

/*
    get max number of sessions

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        max number of sessions

    COMMENT
        n/a
*/
u32 icli_session_max_get(void)
{
    u32     i;

    _SEMA_TAKE();

    i = icli_session_max_get_x();

    _SEMA_GIVE();

    return i;
}

/*
    set max number of sessions

    INPUT
        max_sessions : max number of sessions

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_max_set(
    IN u32 max_sessions
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_session_max_set_x( max_sessions );

    _SEMA_GIVE();

    return rc;
}

/*
    execute or parse a command on a session

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC(..., FALSE);
        ...
        ICLI_CMD_EXEC(..., FALSE);
        ICLI_CMD_PARSING_END();

    INPUT
        session : session ID
        cmd     : command to be executed
        b_exec  : TRUE  - execute the command function
                  FALSE - parse the command only to check if the command is legal or not,
                          but not execute

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec(
    IN u32      session_id,
    IN char     *cmd,
    IN BOOL     b_exec
)
{
    icli_session_handle_t   *handle;
    i32                     rc;
    u32                     cmd_len;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( cmd == NULL ) {
        T_E("cmd == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    cmd_len = icli_str_len( cmd );

    // empty command
    if ( cmd_len == 0 ) {
        return ICLI_RC_OK;
    }

    // cmd is too long
    if ( cmd_len > ICLI_STR_MAX_LEN ) {
        T_E("cmd length %d is too long, must less than %d\n", cmd_len, ICLI_STR_MAX_LEN);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    /* get handle */
    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = _session_cmd_exec(handle, cmd, b_exec, NULL, ICLI_INVALID_LINE_NUMBER, TRUE);

    _SEMA_GIVE();

    return rc;
}

/*
    execute or parsing a command on a session and display error message from application

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ...
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ICLI_CMD_PARSING_END();

    INPUT
        session              : session ID
        cmd                  : command to be executed
        b_exec               : TRUE  - execute the command function
                               FALSE - parse the command only to
                                       check if the command is legal syntax,
                                       but not execute
        app_err_msg          : error message from application
        b_display_err_syntax : TRUE  - display error syntax
                               FALSE - not display

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec_err_display(
    IN u32      session_id,
    IN char     *cmd,
    IN BOOL     b_exec,
    IN char     *app_err_msg,
    IN BOOL     b_display_err_syntax
)
{
    icli_session_handle_t   *handle;
    i32                     rc;
    u32                     cmd_len;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( cmd == NULL ) {
        T_E("cmd == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    cmd_len = icli_str_len( cmd );

    // empty command
    if ( cmd_len == 0 ) {
        return ICLI_RC_OK;
    }

    // cmd is too long
    if ( cmd_len > ICLI_STR_MAX_LEN ) {
        T_E("cmd length %d is too long, must less than %d\n", cmd_len, ICLI_STR_MAX_LEN);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    /* get handle */
    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = _session_cmd_exec(handle, cmd, b_exec, app_err_msg, ICLI_INVALID_LINE_NUMBER, b_display_err_syntax);

    _SEMA_GIVE();

    return rc;
}

/*
    execute or parsing a command on a session and display error message from application

    If parsing commands
    before parsing, begin the transaction by ICLI_CMD_PARSING_BEGIN()
    and end the transaction after finishing the parsing by ICLI_CMD_PARSING_END()
    the flow is as follows.

        ICLI_CMD_PARSING_BEGIN();
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ...
        ICLI_CMD_EXEC_ERR_DISPLAY(..., FALSE, ...);
        ICLI_CMD_PARSING_END();

    INPUT
        session_id           : session ID
        cmd                  : command to be executed
        b_exec               : TRUE  - execute the command function
                               FALSE - parse the command only to
                                       check if the command is legal syntax,
                                       but not execute
        filename             : config file name of the command
        line_number          : the line number in the config file for the command
        b_display_err_syntax : TRUE  - display error syntax
                               FALSE - not display

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_exec_err_file_line(
    IN u32      session_id,
    IN char     *cmd,
    IN BOOL     b_exec,
    IN char     *filename,
    IN u32      line_number,
    IN BOOL     b_display_err_syntax
)
{
    icli_session_handle_t   *handle;
    i32                     rc;
    u32                     cmd_len;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( cmd == NULL ) {
        T_E("cmd == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    cmd_len = icli_str_len( cmd );

    // empty command
    if ( cmd_len == 0 ) {
        return ICLI_RC_OK;
    }

    // cmd is too long
    if ( cmd_len > ICLI_STR_MAX_LEN ) {
        T_E("cmd length %d is too long, must less than %d\n", cmd_len, ICLI_STR_MAX_LEN);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( line_number > ICLI_MAX_INT ) {
        T_E("invalid line number %u\n", line_number);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    /* get handle */
    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = _session_cmd_exec(handle, cmd, b_exec, filename, (i32)line_number, b_display_err_syntax);

    _SEMA_GIVE();

    return rc;
}

/*
    begin transaction to parse a batch of commands on a session

    INPUT
        session : session ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_parsing_begin(
    IN u32      session_id
)
{
    icli_session_handle_t   *handle;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    /* get handle */
    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->runtime_data.b_in_parsing = TRUE;
    handle->runtime_data.current_parsing_mode =
        handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode;

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    end transaction to parse commands on a session

    INPUT
        session : session ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_cmd_parsing_end(
    IN u32      session_id
)
{
    icli_session_handle_t   *handle;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    /* get handle */
    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->runtime_data.b_in_parsing = FALSE;

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    output formated string to a specific session
    the maximum length to print is (ICLI_STR_MAX_LEN + 64),
    where ICLI_STR_MAX_LEN depends on project.

    * if the length is > (ICLI_STR_MAX_LEN + 64),
      then please use icli_session_self_printf_lstr()

    INPUT
        session_id : the session ID
        format     : string format
        ...        : parameters of format

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_printf(
    IN  u32         session_id,
    IN  const char  *format,
    IN  ...
)
{
    va_list                 arglist = NULL;
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("session %d handle is NULL\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    switch ( handle->open_data.way ) {
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        if ( handle->runtime_data.line_mode == ICLI_LINE_MODE_BYPASS ) {
            _SEMA_GIVE();
            return ICLI_RC_OK;
        }
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
        _SEMA_GIVE();
        return ICLI_RC_OK;

    default:
        _SEMA_GIVE();
        T_E("invalid session %d type %d\n", session_id, handle->open_data.way);
        return ICLI_RC_ERR_PARAMETER;
    }

    //format string buffer
    va_start( arglist, format );

    (void)icli_session_va_str_put(session_id, format, arglist);

    va_end( arglist );

    _SEMA_GIVE();
    return ICLI_RC_OK;
}

/*
    output long string to a specific session
    the length of long string is larger than (ICLI_STR_MAX_LEN),
    where ICLI_STR_MAX_LEN depends on project

    the long string must be a char array or dynamic allocated memory(write-able),
    and must not be a const string(read-only).

    INPUT
        session_id : the session ID
        lstr       : long string to print
                     *** str must be a char array or dynamic allocated memory(write-able),
                         and must not a const string(read_only).

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_printf_lstr(
    IN  u32         session_id,
    IN  char        *lstr
)
{
    char                    *s, *c;
    char                    bc;
    i32                     rc = ICLI_RC_OK;
    icli_session_handle_t   *handle;

    if ( icli_session_alive(session_id) == FALSE ) {
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        T_E("session %d handle is NULL\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    switch ( handle->open_data.way ) {
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        if ( handle->runtime_data.line_mode == ICLI_LINE_MODE_BYPASS ) {
            return ICLI_RC_OK;
        }
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
        return ICLI_RC_OK;

    default:
        T_E("invalid session %d type %d\n", session_id, handle->open_data.way);
        return ICLI_RC_ERR_PARAMETER;
    }

    s = lstr;
    for ( c = lstr; *c != 0; c++ ) {
        if ( *c == '\n' ) {
            /* store the char after '\n' */
            bc = *(c + 1);
            /* temp terminate the string for print */
            *(c + 1) = 0;
            /* print it */
            rc = icli_session_printf(session_id, s);
            if ( rc != ICLI_RC_OK ) {
                *(c + 1) = bc;
                return rc;
            }

            /* more string? */
            if ( bc ) {
                // restore the buffer char
                *(c + 1) = bc;
                // more and set the next string
                s = c + 1;
            } else {
                // no more and set to 0 to tell not print later
                s = 0;
            }
        }
    }
    /* last string */
    if ( s ) {
        rc = icli_session_printf(session_id, s);
    }
    return rc;
}

/*
    output formated string to all sessions
    this does not check ICLI_LINE_MODE_BYPASS
    because this is used for system message

    INPUT
        format     : string format
        ...        : parameters of format

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_printf_to_all(
    IN  const char  *format,
    IN  ...
)
{
    u32                     session_id;
    va_list                 arglist = NULL;
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    /* find the first alive session */
    for ( session_id = 0; session_id < ICLI_SESSION_CNT; session_id++ ) {
        if ( ! icli_session_alive(session_id) ) {
            continue;
        }
        handle = icli_session_handle_get( session_id );
        if ( handle == NULL ) {
            _SEMA_GIVE();
            T_E("session %d handle is NULL\n", session_id);
            return ICLI_RC_ERR_PARAMETER;
        }

        switch ( handle->open_data.way ) {
        case ICLI_SESSION_WAY_CONSOLE:
        case ICLI_SESSION_WAY_TELNET:
        case ICLI_SESSION_WAY_SSH:
        case ICLI_SESSION_WAY_THREAD_CONSOLE:
        case ICLI_SESSION_WAY_THREAD_TELNET:
        case ICLI_SESSION_WAY_THREAD_SSH:
            break;

        case ICLI_SESSION_WAY_APP_EXEC:
            continue;

        default:
            _SEMA_GIVE();
            T_E("invalid session %u type %d\n", session_id, handle->open_data.way);
            return ICLI_RC_ERR_PARAMETER;
        }
        break;
    }

    /* no session alive */
    if ( session_id == ICLI_SESSION_CNT ) {
        _SEMA_GIVE();
        return ICLI_RC_OK;
    }

    /* format string from parameters */
    va_start( arglist, format );

    (void)icli_session_va_str_put(session_id, format, arglist);

    va_end( arglist );

    /* print out to following alive sessions */
    for ( session_id++; session_id < ICLI_SESSION_CNT; session_id++ ) {
        if ( ! icli_session_alive(session_id) ) {
            continue;
        }
        (void)icli_session_va_str_put(session_id, format, arglist);
    }

    _SEMA_GIVE();
    return ICLI_RC_OK;
}

/*
    get string from usr input

    INPUT
        session_id : the session ID
        type       : input type
                     NORMAL   - echo the input char
                     PASSWORD - echo '*'
        str_size   : the buffer size of str

    OUTPUT
        str        : user input string
        str_size   : the length of user input
        end_key    : the key to terminate the input,
                     Enter, New line, or Ctrl-C

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_usr_str_get(
    IN      u32                     session_id,
    IN      icli_usr_input_type_t   input_type,
    OUT     char                    *str,
    INOUT   i32                     *str_size,
    OUT     i32                     *end_key
)
{
    i32                     rc;
    icli_line_mode_t        old_line_mode;
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get original line mode */
    old_line_mode = handle->runtime_data.line_mode;

    /* set FLOOD mode */
    handle->runtime_data.line_mode = ICLI_LINE_MODE_FLOOD;

    /* get user string */
    rc = icli_session_usr_str_get_x( session_id, input_type, str, str_size, end_key );

    /* set back to the original mode */
    handle->runtime_data.line_mode = old_line_mode;

    _SEMA_GIVE();
    return rc;
}

/*
    get Ctrl-C from user

    INPUT
        session_id : the session ID
        wait_time  : time to wait in milli-seconds
                     must be 2147483647 >= wait_time > 0

    OUTPUT
        n/a

    RETURN
        ICLI_RC_OK            : yes, the user press Ctrl-C
        ICLI_RC_ERR_EXPIRED   : time expired
        ICLI_RC_ERR_PARAMETER : input paramter error

    COMMENT
        n/a
*/
i32 icli_session_ctrl_c_get(
    IN  u32     session_id,
    IN  u32     wait_time
)
{
    i32     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( wait_time == 0 || wait_time > ICLI_MAX_INT) {
        T_E("invalid wait_time = %u\n", wait_time);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = icli_session_ctrl_c_get_x( session_id, wait_time );

    _SEMA_GIVE();

    return rc;
}

/*
    make session entering the mode

    INPUT
        session_id : the session ID
        mode       : mode entering

    OUTPUT
        n/a

    RETURN
        >= 0 : successful and the return value is mode level
        -1   : failed

    COMMENT
        n/a
*/
i32 icli_session_mode_enter(
    IN  u32                 session_id,
    IN  icli_cmd_mode_t     mode
)
{
    i32     rc;

    _SEMA_TAKE();

    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = icli_session_mode_enter_x( session_id, mode );

    _SEMA_GIVE();

    return rc;
}

/*
    make session exit the top mode

    INPUT
        session_id : the session ID

    OUTPUT
        n/a

    RETURN
        >= 0 : successful and the return value is mode level
        -1   : failed

    COMMENT
        n/a
*/
i32 icli_session_mode_exit(
    IN  u32     session_id
)
{
    i32     rc;

    _SEMA_TAKE();

    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = icli_session_mode_exit_x( session_id );

    _SEMA_GIVE();

    return rc;
}

/*
    get configuration data of the session

    INPUT
        session_id : the session ID, INDEX

    OUTPUT
        data: data of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_data_get(
    INOUT icli_session_data_t   *data
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_session_data_get_x( data );

    _SEMA_GIVE();

    return rc;
}

/*
    get configuration data of the next session

    INPUT
        session_id : the next session of the session ID, INDEX

    OUTPUT
        data : data of the next session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_data_get_next(
    INOUT icli_session_data_t   *data
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_session_data_get_next_x( data );

    _SEMA_GIVE();

    return rc;
}

#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
/*
    set history size of a session

    INPUT
        session_id   : session ID

    OUTPUT
        history_size : the size of history commands
                       0 means to disable history function

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_size_get(
    IN  u32     session_id,
    OUT u32     *history_size
)
{
    icli_session_handle_t   *handle;

    if ( history_size == NULL ) {
        T_E("invalid history_size = NULL, session_id = %u\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    *history_size = ICLI_HISTORY_MAX_CNT;

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set history size of a session

    INPUT
        session_id   : session ID
        history_size : the size of history commands
                       0 means to disable history function

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_size_set(
    IN  u32     session_id,
    IN  u32     history_size
)
{
    icli_session_handle_t   *handle;

    if ( history_size > ICLI_HISTORY_CMD_CNT ) {
        T_E("invalid history_size = %u, session_id = %u\n", history_size, session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    ICLI_HISTORY_MAX_CNT = history_size;

    // reset
    handle->runtime_data.history_head     = -1;
    handle->runtime_data.history_tail     = -1;

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}
#endif /* CP, 2012/08/29 09:25, history max count is configurable */

/*
    get the first history command of a session

    INPUT
        session_id  : session ID

    OUTPUT
        history_cmd : the first history command of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_cmd_get_first(
    IN  u32                         session_id,
    OUT icli_session_history_cmd_t  *history
)
{
    icli_session_handle_t   *handle;
    i32                     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( history == NULL ) {
        T_E("history == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    /* session alive */
    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = icli_session_handle_get(session_id);

    /* get first history */
    history->history_pos = handle->runtime_data.history_tail;
    rc = _session_history_cmd_get(handle, history);

    _SEMA_GIVE();

    return rc;
}

/*
    get history command of the session

    INPUT
        session_id           : session ID
        history->history_pos : INDEX

    OUTPUT
        history_cmd : the history command of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_cmd_get(
    IN    u32                         session_id,
    INOUT icli_session_history_cmd_t  *history
)
{
    icli_session_handle_t   *handle;
    i32                     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( history == NULL ) {
        T_E("history == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    /* session alive */
    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = icli_session_handle_get(session_id);

    /* get history */
    rc = _session_history_cmd_get(handle, history);

    _SEMA_GIVE();

    return rc;
}

/*
    get the next history command of the session

    INPUT
        session_id           : session ID
        history->history_pos : INDEX

    OUTPUT
        history_cmd : the next history command of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_history_cmd_get_next(
    IN    u32                         session_id,
    INOUT icli_session_history_cmd_t  *history
)
{
    icli_session_handle_t   *handle;
    i32                     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( history == NULL ) {
        T_E("history == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    /* session alive */
    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get handle */
    handle = icli_session_handle_get(session_id);

    /* if history is empty */
    if ( _history_empty(handle) ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_EMPTY;
    }

    /* check if at the head (end of history) already */
    if ( history->history_pos == handle->runtime_data.history_head ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get next */
#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
    history->history_pos = (history->history_pos + 1) % (i32)ICLI_HISTORY_MAX_CNT;
#else
    history->history_pos = (history->history_pos + 1) % ICLI_HISTORY_CMD_CNT;
#endif
    rc = _session_history_cmd_get(handle, history);

    _SEMA_GIVE();

    return rc;
}

/*
    get privilege of a session

    INPUT
        session_id : session ID

    OUTPUT
        privilege  : session privilege

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_privilege_get(
    IN  u32                 session_id,
    OUT icli_privilege_t    *privilege
)
{
    icli_session_handle_t   *handle;

    if ( privilege == NULL ) {
        T_E("privilege == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get config data */
    *privilege = handle->runtime_data.privilege;

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set privilege of a session

    INPUT
        session_id : session ID
        privilege  : session privilege

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_privilege_set(
    IN  u32                 session_id,
    IN  icli_privilege_t    privilege
)
{
    icli_session_handle_t   *handle;

    if ( privilege >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privilege %u\n", privilege);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* set config data */
    handle->runtime_data.privilege = privilege;

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set width of a session

    INPUT
        session_id : session ID
        width      : width (in number of characters) of the session

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_width_set(
    IN  u32     session_id,
    IN  u32     width
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->width = width;

    if ( width == 0 ) {
        handle->runtime_data.width = ICLI_MAX_WIDTH;
    } else if ( width < ICLI_MIN_WIDTH ) {
        handle->runtime_data.width = ICLI_MIN_WIDTH;
    } else {
        handle->runtime_data.width = width;
    }

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set lines of a session

    INPUT
        session_id : session ID
        lines      : number of lines on a screen

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_lines_set(
    IN  u32     session_id,
    IN  u32     lines
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->lines = lines;

    if ( lines == 0 ) {
        handle->runtime_data.lines = 0;
    } else if ( lines < ICLI_MIN_LINES ) {
        handle->runtime_data.lines = ICLI_MIN_LINES;
    } else {
        handle->runtime_data.lines = lines;
    }

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set waiting time of a session

    INPUT
        session_id : session ID
        wait_time  : time to wait user input, in seconds
                     = 0 means wait forever

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_wait_time_set(
    IN  u32     session_id,
    IN  u32     wait_time
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    // convert to msec
    handle->config_data->wait_time = wait_time;

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    get line mode of the session

    INPUT
        session_id : the session ID

    OUTPUT
        line_mode : line mode of the session

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_line_mode_get(
    IN  u32                 session_id,
    OUT icli_line_mode_t    *line_mode
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    *line_mode = handle->runtime_data.line_mode;

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set line mode of the session

    INPUT
        session_id : the session ID
        line_mode  : line mode of the session

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_line_mode_set(
    IN  u32                 session_id,
    IN  icli_line_mode_t    line_mode
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->runtime_data.line_mode = line_mode;

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    get input style of the session

    INPUT
        session_id : the session ID

    OUTPUT
        input_style : input style

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_input_style_get(
    IN  u32                 session_id,
    OUT icli_input_style_t  *input_style
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    *input_style = handle->config_data->input_style;

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set input style of the session

    INPUT
        session_id  : the session ID
        input_style : input style of the session

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_input_style_set(
    IN  u32                 session_id,
    IN  icli_input_style_t  input_style
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->input_style = input_style;

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

#if 1 /* CP, 2012/08/31 07:51, enable/disable banner per line */
/*
    enable/disable the display of EXEC banner of the session

    INPUT
        session_id    : the session ID
        b_exec_banner : TRUE - enable, FALSE - disable

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_exec_banner_enable(
    IN  u32     session_id,
    IN  BOOL    b_exec_banner
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->b_exec_banner = b_exec_banner;

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    enable/disable the display of Day banner of the session

    INPUT
        session_id    : the session ID
        b_motd_banner : TRUE - enable, FALSE - disable

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_motd_banner_enable(
    IN  u32     session_id,
    IN  BOOL    b_motd_banner
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->b_motd_banner = b_motd_banner;

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}
#endif

#if 1 /* CP, 2012/08/31 09:31, location and default privilege */
/*
    set location of the session

    INPUT
        session_id : the session ID
        location   : where you are

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_location_set(
    IN  u32                 session_id,
    IN  char                *location
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( location == NULL ) {
        handle->config_data->location[0] = ICLI_EOS;
    } else {
        (void)icli_str_ncpy(handle->config_data->location, location, ICLI_LOCATION_MAX_LEN);
    }

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}

/*
    set privileged level of the session

    INPUT
        session_id        : the session ID
        default_privilege : default privilege level

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_privileged_level_set(
    IN  u32                 session_id,
    IN  icli_privilege_t    privileged_level
)
{
    icli_session_handle_t   *handle;

    if ( privileged_level >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privileged level %u\n", privileged_level);
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle->config_data->privileged_level = privileged_level;

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return ICLI_RC_OK;
}
#endif

#if 1 /* CP, 2012/09/04 16:46, session user name */
/*
    set user name of the session

    INPUT
        session_id : the session ID
        user_name  : who login

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_user_name_set(
    IN  u32     session_id,
    IN  char    *user_name
)
{
    icli_session_handle_t   *handle;

    if ( user_name == NULL ) {
        T_E("user_name == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    handle = icli_session_handle_get(session_id);
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("handle == NULL, session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    (void)icli_str_ncpy(handle->runtime_data.user_name, user_name, ICLI_USERNAME_MAX_LEN);

    _SEMA_GIVE();

    return ICLI_RC_OK;
}
#endif

#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
/*
    get myself session ID

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        my session ID    - successful
        ICLI_SESSION_CNT - failed

    COMMENT
        n/a
*/
u32 icli_session_self_id(
    void
)
{
    return icli_session_self_id_x();
}

/*
    output formated string to a specific session
    the maximum length to print is (ICLI_STR_MAX_LEN + 64),
    where ICLI_STR_MAX_LEN depends on project.

    * if the length is > (ICLI_STR_MAX_LEN + 64),
      then please use icli_session_self_printf_lstr()

    INPUT
        format     : string format
        ...        : parameters of format

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
int icli_session_self_printf(
    IN  const char  *format,
    IN  ...
)
{
    u32                     session_id;
    va_list                 arglist = NULL;
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    session_id = icli_session_self_id_x();
    if ( session_id >= icli_session_max_get_x() ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        T_E("session %d handle is NULL\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    switch ( handle->open_data.way ) {
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        if ( handle->runtime_data.line_mode == ICLI_LINE_MODE_BYPASS ) {
            _SEMA_GIVE();
            return ICLI_RC_OK;
        }
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
        _SEMA_GIVE();
        return ICLI_RC_OK;

    default:
        _SEMA_GIVE();
        T_E("invalid session %d type %d\n", session_id, handle->open_data.way);
        return ICLI_RC_ERR_PARAMETER;
    }

    //format string buffer
    va_start( arglist, format );

    (void)icli_session_va_str_put(session_id, format, arglist);

    va_end( arglist );

    _SEMA_GIVE();
    return ICLI_RC_OK;
}

/*
    output long string to a specific session
    the length of long string is larger than (ICLI_STR_MAX_LEN),
    where ICLI_STR_MAX_LEN depends on project

    the long string must be a char array or dynamic allocated memory(write-able),
    and must not be a const string(read-only).

    INPUT
        session_id : the session ID
        lstr       : long string to print
                     *** str must be a char array or dynamic allocated memory(write-able),
                         and must not a const string(read_only).

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_self_printf_lstr(
    IN  char        *lstr
)
{
    i32     rc;
    u32     session_id;

    session_id = icli_session_self_id_x();
    if ( session_id >= icli_session_max_get_x() ) {
        _SEMA_GIVE();
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = icli_session_printf_lstr(session_id, lstr);
    return rc;
}
#endif /* CP, 2012/09/11 14:08, Use thread information to get session ID */

#if 1 /* CP, 2012/10/16 17:02, ICLI_RC_CHECK */
/*
    set general error text string when error happen
    this error string will print out before error_txt(rc)

    INPUT
        session_id : ID of session
        msg        : general error message

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_session_rc_check_setup(
    IN  u32         session_id,
    IN  const char  *msg
)
{
    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    if ( session_id >= icli_session_max_get_x() ) {
        _SEMA_GIVE();
        return;
    }

    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return;
    }

    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        return;
    }

    handle->runtime_data.rc_context_string = msg;
    _SEMA_GIVE();
}

/*
    check if rc is ok or not.
    if rc is error, then print out sequence is as follows.
        1. msg (from this API)
        2. rc_context_string (from icli_session_rc_check_setup())
        3. error_txt(rc).

    INPUT
        session_id : ID of session
        rc         : error code
        msg        : general error message

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
vtss_rc icli_session_rc_check(
    IN  u32         session_id,
    IN  vtss_rc     rc,
    IN  const char  *msg
)
{
    const char  *rc_context_string;

    icli_session_handle_t   *handle;

    _SEMA_TAKE();

    if ( session_id >= icli_session_max_get_x() ) {
        _SEMA_GIVE();
        return VTSS_RC_ERROR;
    }

    if ( icli_session_alive(session_id) == FALSE ) {
        _SEMA_GIVE();
        return VTSS_RC_ERROR;
    }

    handle = icli_session_handle_get( session_id );
    if ( handle == NULL ) {
        _SEMA_GIVE();
        return VTSS_RC_ERROR;
    }

    rc_context_string = handle->runtime_data.rc_context_string;

    _SEMA_GIVE();

    if ( rc != VTSS_RC_OK ) {
        if ( msg ) {
            (void)icli_session_printf(session_id, "%s", msg);
        } else if ( rc_context_string ) {
            (void)icli_session_printf(session_id, "%s", rc_context_string);
        }
        (void)icli_session_printf(session_id, "%% (%s)\n", icli_xy_error_txt(rc));
    }
    return rc;
}
#endif

/*
    set mode name shown in command prompt

    INPUT
        mode : command mode
        name : mode name

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_mode_name_set(
    IN  icli_cmd_mode_t     mode,
    IN  char                *name
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_mode_name_set_x( mode, name );

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return rc;
}

/*
    set device name shown in command prompt

    INPUT
        dev_name : device name

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_dev_name_set(
    IN  char    *dev_name
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_dev_name_set_x( dev_name );

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return rc;
}

/*
    get device name

    INPUT
        n/a

    OUTPUT
        dev_name : device name

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_dev_name_get(
    OUT char    *dev_name
)
{
    char    *s;

    if ( dev_name == NULL ) {
        T_E("dev_name == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    s = icli_str_cpy(dev_name, icli_dev_name_get_x());

    _SEMA_GIVE();

    if ( s ) {
        return ICLI_RC_OK;
    } else {
        return ICLI_RC_ERROR;
    }
}

/*
    get LOGIN banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        n/a

    OUTPUT
        banner_login : LOGIN banner

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_login_get(
    OUT  char    *banner_login
)
{
    i32     rc = ICLI_RC_OK;

    if ( banner_login == NULL ) {
        T_E("banner_login == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    (void)icli_str_cpy(banner_login, icli_banner_login_get_x());

    _SEMA_GIVE();

    return rc;
}

/*
    set LOGIN banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        banner_login : LOGIN banner

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_login_set(
    IN  char    *banner_login
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_banner_login_set_x( banner_login );

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return rc;
}

/*
    get MOTD banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        n/a

    OUTPUT
        banner_motd : MOTD banner

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_motd_get(
    OUT  char    *banner_motd
)
{
    i32     rc = ICLI_RC_OK;

    if ( banner_motd == NULL ) {
        T_E("banner_motd == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    (void)icli_str_cpy(banner_motd, icli_banner_motd_get_x());

    _SEMA_GIVE();

    return rc;
}

/*
    set MOTD banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        banner_motd : MOTD banner

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_motd_set(
    IN  char    *banner_motd
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_banner_motd_set_x( banner_motd );

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return rc;
}

/*
    get EXEC banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        n/a

    OUTPUT
        banner_exec : EXEC banner

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_exec_get(
    OUT  char    *banner_exec
)
{
    i32     rc = ICLI_RC_OK;

    if ( banner_exec == NULL ) {
        T_E("banner_exec == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    (void)icli_str_cpy(banner_exec, icli_banner_exec_get_x());

    _SEMA_GIVE();

    return rc;
}

/*
    set EXEC banner.
    the maximum length is 255 chars, ICLI_BANNER_MAX_LEN.

    INPUT
        banner_exec : EXEC banner

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_banner_exec_set(
    IN  char    *banner_exec
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_banner_exec_set_x( banner_exec );

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return rc;
}

/*
    check if the port type is present in this device

    INPUT
        type : port type

    OUTPUT
        n/a

    RETURN
        TRUE  : yes, the device has ports belong to this port type
        FALSE : no

    COMMENT
        n/a
*/
BOOL icli_port_type_present(
    IN icli_port_type_t     type
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_port_type_present_x( type );

    _SEMA_GIVE();

    return b;
}

/*
    get port type for a specific switch port

    INPUT
        type : port type

    OUTPUT
        n/a

    RETURN
        name of the type
            if the type is invalid. then "Unknown" is return

    COMMENT
        n/a
*/
char *icli_port_type_get_name(
    IN  icli_port_type_t    type
)
{
    return icli_variable_port_type_get_name(type);
}

/*
    reset port range

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_port_range_reset( void )
{
    _SEMA_TAKE();

    icli_port_range_reset_x();

    _SEMA_GIVE();
}

/*
    get port range

    INPUT

    OUTPUT
        range : port range

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_range_get(
    OUT icli_stack_port_range_t  *range
)
{
    icli_stack_port_range_t *spr;

    if ( range == NULL ) {
        T_E("range == NULL\n");
        return FALSE;
    }

    _SEMA_TAKE();

    spr = icli_port_range_get_x();
    *range = *spr;

    _SEMA_GIVE();

    return TRUE;
}

/*
    set port range on a specific port type

    INPUT
        range  : port range set on the port type

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        CIRTICAL ==> before set, port range must be reset by icli_port_range_reset().
        otherwise, the set may be failed because this set will check the
        port definition to avoid duplicate definitions.
*/
BOOL icli_port_range_set(
    IN icli_stack_port_range_t  *range
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_port_range_set_x(range);

    _SEMA_GIVE();

    return b;
}

/*
    get switch range from usid and uport

    INPUT
        switch_range->usid        : usid
        switch_range->begin_uport : uport
        switch_range->port_cnt    : number of ports

    OUTPUT
        switch_range->port_type  : port type
        switch_range->switch_id  : switch ID
        switch_range->begin_port : port ID

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_from_usid_uport(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_port_from_usid_uport_x( switch_range );

    _SEMA_GIVE();

    return b;
}

/*
    get first type/switch_id/port

    INPUT
        n/a

    OUTPUT
        switch_range : first type/switch_id/port with usid/uport

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_get_first(
    OUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_port_get_first_x( switch_range );

    _SEMA_GIVE();

    return b;
}

/*
    get usid/uport from type/switch_id/port

    INPUT
        switch_range->port_type   : port type
        switch_range->switch_id   : switch ID
        switch_range->begin_port  : port ID
        switch_range->port_cnt    : donot care

    OUTPUT
        switch_range->usid        : usid
        switch_range->begin_uport : uport

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_get(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_port_get_x( switch_range );

    _SEMA_GIVE();

    return b;
}
/*
    get next type/switch_id/port with usid/uport from current switch_id/port

    INPUT
        switch_range->port_type   : port type
        switch_range->switch_id   : switch ID
        switch_range->begin_port  : port ID
        switch_range->port_cnt    : donot care

    OUTPUT
        switch_range->port_type   : next port type
        switch_range->switch_id   : next switch ID
        switch_range->begin_port  : next port ID
        switch_range->usid        : usid
        switch_range->begin_uport : uport

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_get_next(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_port_get_next_x( switch_range );

    _SEMA_GIVE();

    return b;
}

/*
    get password of privilege level

    INPUT
        priv : the privilege level

    OUTPUT
        password : the corresponding password with size (ICLI_PASSWORD_MAX_LEN + 1)

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_password_get(
    IN  icli_privilege_t    priv,
    OUT char                *password
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_enable_password_get_x(priv, password);

    _SEMA_GIVE();

    return b;
}

/*
    set password of privilege level

    INPUT
        priv     : the privilege level
        password : the corresponding password with size (ICLI_PASSWORD_MAX_LEN + 1)

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_password_set(
    IN  icli_privilege_t    priv,
    IN  char                *password
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_enable_password_set_x(priv, password);

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return b;
}

#if 1 /* CP, 2012/08/31 17:00, enable secret */
/*
    verify clear password of privilege level is correct or not
    according to password or secret

    INPUT
        priv           : the privilege level
        clear_password : the corresponding password with size (ICLI_PASSWORD_MAX_LEN + 1)

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_password_verify(
    IN  icli_privilege_t    priv,
    IN  char                *clear_password
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_enable_password_verify_x(priv, clear_password);

    _SEMA_GIVE();

    return b;
}

/*
    set secret of privilege level

    INPUT
        priv   : the privilege level
        secret : the corresponding secret with size (ICLI_PASSWORD_MAX_LEN + 1)

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_secret_set(
    IN  icli_privilege_t    priv,
    IN  char                *secret
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_enable_secret_set_x(priv, secret);

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return b;
}

/*
    translate clear password of privilege level to secret password

    INPUT
        priv           : the privilege level
        clear_password : the corresponding password with size (ICLI_PASSWORD_MAX_LEN + 1)

    OUTPUT
        n/a

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_enable_secret_clear_set(
    IN  icli_privilege_t    priv,
    IN  char                *clear_password
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_enable_secret_set_clear_x(priv, clear_password);

#if 1 /* CP, 2012/09/14 10:38, conf */
    if ( icli_conf_write() == FALSE ) {
        T_W("fail to write conf\n");
    }
#endif

    _SEMA_GIVE();

    return b;
}

/*
    get if the enable password is in secret or not

    INPUT
        priv : the privilege level

    OUTPUT
        n/a

    RETURN
        TRUE  : in secret
        FALSE : clear password

    COMMENT
        n/a
*/
BOOL icli_enable_password_if_secret_get(
    IN  icli_privilege_t    priv
)
{
    BOOL    b;

    _SEMA_TAKE();

    b = icli_enable_password_if_secret_get_x( priv );

    _SEMA_GIVE();

    return b;
}
#endif

#if 1 /* CP, 2012/10/08 14:31, debug command, debug prompt */
/*
    set debug prompt shown in command prompt

    INPUT
        debug_prompt : debug prompt

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_debug_prompt_set(
    IN  char    *debug_prompt
)
{
    i32     rc;

    _SEMA_TAKE();

    rc = icli_debug_prompt_set_x( debug_prompt );

    _SEMA_GIVE();

    return rc;
}

/*
    get debug prompt

    INPUT
        n/a

    OUTPUT
        debug_prompt : debug prompt

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_debug_prompt_get(
    OUT char    *debug_prompt
)
{
    char    *s;

    if ( debug_prompt == NULL ) {
        T_E("debug_prompt == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    _SEMA_TAKE();

    s = icli_str_cpy(debug_prompt, icli_debug_prompt_get_x());

    _SEMA_GIVE();

    if ( s ) {
        return ICLI_RC_OK;
    } else {
        return ICLI_RC_ERROR;
    }
}
#endif
