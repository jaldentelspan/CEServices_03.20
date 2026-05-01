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
    > CP.Wang, 2011/05/03 10:42
        - create

******************************************************************************
*/
/*
******************************************************************************

    Include File

******************************************************************************
*/
#include "icli.h"
#include <time.h>

/*
******************************************************************************

    Constant

******************************************************************************
*/
#define __ICLI_CR_STR               "<cr>"
#define __ICLI_HELP_MAX_LEN         256

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
static icli_session_handle_t    g_session_handle[ICLI_SESSION_CNT];
static u32                      g_max_sessions;

/*
******************************************************************************

    Static Function

******************************************************************************
*/

static void _goback_exec_mode(
    IN icli_session_handle_t   *handle
)
{
    i32     rc = 1;

    while ( rc ) {
        rc = icli_session_mode_exit_x( handle->session_id );
        if ( rc < 0 ) {
            break;
        }
    }
}

static void _runtime_data_default(
    IN icli_session_handle_t   *handle
)
{
    /* set to default */
    handle->runtime_data.history_head     = -1;
    handle->runtime_data.history_tail     = -1;
    handle->runtime_data.err_display_mode = ICLI_ERR_DISPLAY_MODE_PRINT;

    if ( handle->config_data->width == 0 ) {
        handle->runtime_data.width = ICLI_MAX_WIDTH;
    } else if ( handle->config_data->width < ICLI_MIN_WIDTH ) {
        handle->runtime_data.width = ICLI_MIN_WIDTH;
    } else {
        handle->runtime_data.width = handle->config_data->width;
    }

    if ( handle->config_data->lines == 0 ) {
        handle->runtime_data.lines = 0;
    } else if ( handle->config_data->lines < ICLI_MIN_LINES ) {
        handle->runtime_data.lines = ICLI_MIN_LINES;
    } else {
        handle->runtime_data.lines = handle->config_data->lines;
    }
}

static void _runtime_data_clear(
    IN icli_session_handle_t   *handle
)
{
    /* free mode */
    _goback_exec_mode( handle );

    /* free match_para */
    if ( handle->runtime_data.match_para ) {
        icli_exec_para_list_free( &(handle->runtime_data.match_para) );
    }

    /* free cmd_var */
    if ( handle->runtime_data.cmd_var ) {
        icli_exec_para_list_free( &(handle->runtime_data.cmd_var) );
    }

    /* clear to 0 */
    memset(&(handle->runtime_data), 0, sizeof(icli_session_runtime_data_t));

    /* set to default */
    _runtime_data_default( handle );
}

static i32 _session_get(void)
{
    u32     i;

    for ( i = 0; i < g_max_sessions; i++ ) {
        if ( g_session_handle[i].in_used == FALSE ) {
            g_session_handle[i].session_id = i;
            g_session_handle[i].in_used    = TRUE;
            return (i32)i;
        }
    }
    return -1;
}

static void _session_free(
    IN i32      session_id
)
{
    icli_session_handle_t   *handle = &(g_session_handle[session_id]);

    // clear runtime data
    _runtime_data_clear( handle );
    // make session available
    handle->in_used = FALSE;
}

/*
    init APP session
*/
static BOOL _usr_app_init(
    IN  icli_session_handle_t   *handle
)
{
    icli_session_app_init_t *app_init;
    i32                     app_id;
    BOOL                    b;

    if ( handle->open_data.app_init == NULL ) {
        return TRUE;
    }

    app_init = handle->open_data.app_init;
    app_id = handle->open_data.app_id;

    _SEMA_GIVE();

    b = app_init( app_id );

    _SEMA_TAKE();

    if ( b == FALSE ) {
        T_E("fail to init app\n");
    }
    return b;
}

/*
    close APP session
*/
static void _usr_app_close(
    IN  icli_session_handle_t           *handle,
    IN  icli_session_close_reason_t     reason
)
{
    icli_session_app_close_t    *app_close;
    i32                         app_id;

    if ( handle->open_data.app_close == NULL ) {
        return;
    }

    app_close = handle->open_data.app_close;
    app_id = handle->open_data.app_id;

    _SEMA_GIVE();

    app_close( app_id, reason );

    _SEMA_TAKE();
}

static void _history_cmd_add(
    IN  icli_session_handle_t   *handle
)
{
#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
    // history is disable
    if ( ICLI_HISTORY_MAX_CNT == 0 ) {
        return;
    }
#endif

    /* empty command */
    if ( icli_str_len(handle->runtime_data.cmd) == 0 ) {
        return;
    }

    /* command is the same with the latest history */
    if ( ! _history_empty(handle) ) {
        if ( icli_str_cmp(handle->runtime_data.cmd, handle->runtime_data.history_cmd[handle->runtime_data.history_head]) == 0 ) {
            return;
        }
    }

    if ( _history_full(handle) ) {
#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
        handle->runtime_data.history_tail = (handle->runtime_data.history_tail + 1) % (i32)ICLI_HISTORY_MAX_CNT;
#else
        handle->runtime_data.history_tail = (handle->runtime_data.history_tail + 1) % ICLI_HISTORY_CMD_CNT;
#endif
    } else if ( _history_empty(handle) ) {
        handle->runtime_data.history_tail = 0;
    }
#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
    handle->runtime_data.history_head = (handle->runtime_data.history_head + 1) % (i32)ICLI_HISTORY_MAX_CNT;
#else
    handle->runtime_data.history_head = (handle->runtime_data.history_head + 1) % ICLI_HISTORY_CMD_CNT;
#endif
    (void)icli_str_cpy(handle->runtime_data.history_cmd[handle->runtime_data.history_head], handle->runtime_data.cmd);
}

static void _cmd_display(
    IN  icli_session_handle_t   *handle
)
{
    /* usr input */
    switch ( _SESSON_INPUT_STYLE ) {
    case ICLI_INPUT_STYLE_SINGLE_LINE :
        icli_c_cmd_display( handle );
        break;

    case ICLI_INPUT_STYLE_MULTIPLE_LINE :
        icli_a_cmd_display( handle );
        break;

    case ICLI_INPUT_STYLE_SIMPLE :
        icli_z_cmd_display( handle );
        break;

    default :
        T_E("invalid input_style = %d\n", _SESSON_INPUT_STYLE);
        break;
    }
}

static void _cmd_redisplay(
    IN  icli_session_handle_t   *handle
)
{
    /* usr input */
    switch ( _SESSON_INPUT_STYLE ) {
    case ICLI_INPUT_STYLE_SINGLE_LINE :
        icli_c_cmd_redisplay( handle );
        break;

    case ICLI_INPUT_STYLE_MULTIPLE_LINE :
        icli_a_cmd_redisplay( handle );
        break;

    case ICLI_INPUT_STYLE_SIMPLE :
        icli_z_cmd_display( handle );
        break;

    default :
        T_E("invalid input_style = %d\n", _SESSON_INPUT_STYLE);
        break;
    }
}

/*
    get command from usr input
    1. function key will be provided
    2. the command will be stored in icli_session_handle_t.cmd

    INPUT
        session_id : the session ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 _usr_cmd_get(
    IN  u32     session_id
)
{
    icli_session_handle_t   *handle;
    i32                     rc;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get handle */
    handle = &g_session_handle[ session_id ];

    /* check alive or not */
    if ( handle->runtime_data.alive == FALSE ) {
        T_E("session_id = %d is not alive\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    rc = ICLI_RC_ERROR;
    handle->runtime_data.after_cmd_act = ICLI_AFTER_CMD_ACT_NONE;

    /* usr input */
    switch ( _SESSON_INPUT_STYLE ) {
    case ICLI_INPUT_STYLE_SINGLE_LINE :
        rc = icli_c_usr_cmd_get( handle );
        break;

    case ICLI_INPUT_STYLE_MULTIPLE_LINE :
        rc = icli_a_usr_cmd_get( handle );
        break;

    case ICLI_INPUT_STYLE_SIMPLE :
        rc = icli_z_usr_cmd_get( handle );
        //line count
        handle->runtime_data.line_cnt = (handle->runtime_data.prompt_len + handle->runtime_data.cmd_len + 1) / (i32)(handle->runtime_data.width) + 1;
        break;

    default :
        T_E("invalid input_style = %d\n", _SESSON_INPUT_STYLE);
        break;
    }

    return rc;

}

static void _prompt_display(
    IN icli_session_handle_t   *handle
)
{
    icli_cmd_mode_t     mode;
    char                *mode_prompt,
                        *dev_name;
    char                prompt[ICLI_NAME_MAX_LEN + ICLI_PROMPT_MAX_LEN + 10];
    i32                 len = ICLI_NAME_MAX_LEN + ICLI_PROMPT_MAX_LEN + 5;

#if 1 /* CP, 2012/09/04 13:33, switch information for prompt when stacking */
    icli_switch_info_t  switch_info;

    // initialize first for the case of returning FALSE
    switch_info.b_master = TRUE;
    switch_info.usid     = 0;

    // get switch information
    (void)icli_switch_info_get( &switch_info );
#endif

    //get mode prompt
    mode = handle->runtime_data.mode_para[ handle->runtime_data.mode_level ].mode;
    mode_prompt = icli_mode_name_get_x( mode );

    //get device name
#if 1 /* CP, 2012/10/08 14:31, debug command, debug prompt */
    dev_name = icli_debug_prompt_get_x();
    if ( ICLI_IS_(EOS, *dev_name) ) {
        dev_name = icli_dev_name_get_x();
    }
#else
    dev_name = icli_dev_name_get_x();
#endif

    /* pack prompt */
    memset(prompt, 0, sizeof(prompt));

#if 1 /* CP, 2012/09/04 13:33, switch information for prompt when stacking */
    if ( switch_info.b_master ) {
        if ( icli_str_len(mode_prompt) ) {
            (void)icli_snprintf(prompt, len,
                                "%s(%s)%c ", dev_name, mode_prompt, handle->runtime_data.privilege >= handle->config_data->privileged_level ? '#' : '>');
        } else {
            (void)icli_snprintf(prompt, len, "%s%c ",
                                dev_name, handle->runtime_data.privilege >= handle->config_data->privileged_level ? '#' : '>');
        }
    } else {
        if ( icli_str_len(mode_prompt) ) {
            (void)icli_snprintf(prompt, len,
                                "Slave_%u(%s)%c ", switch_info.usid, mode_prompt, handle->runtime_data.privilege >= handle->config_data->privileged_level ? '#' : '>');
        } else {
            (void)icli_snprintf(prompt, len, "Slave_%u%c ",
                                switch_info.usid, handle->runtime_data.privilege >= handle->config_data->privileged_level ? '#' : '>');
        }
    }
#else
    if ( icli_str_len(mode_prompt) ) {
        (void)icli_snprintf(prompt, len,
                            "%s(%s)%c ", dev_name, mode_prompt, handle->runtime_data.privilege >= handle->config_data->privileged_level ? '#' : '>');
    } else {
        (void)icli_snprintf(prompt, len, "%s%c ",
                            dev_name, handle->runtime_data.privilege >= handle->config_data->privileged_level ? '#' : '>');
    }
#endif

    icli_session_str_put(handle, prompt);
    handle->runtime_data.prompt_len = icli_str_len( prompt );
}

static char *_cmd_word_get(
    IN  icli_session_handle_t   *handle,
    IN  icli_parsing_node_t     *node
)
{
    icli_runtime_cb_t   *runtime_cb;
    icli_runtime_ask_t  ask;
    icli_runtime_t      *runtime;
    char                *byword;
    u32                 b;

#if 1 /* CP, 2012/11/22 13:55, Bugzilla#10151 - int range using constant */
    icli_byword_t       *bw;
#endif

    if ( node->type != ICLI_VARIABLE_KEYWORD ) {
        /* check runtime first */
        runtime_cb = node->node_property.cmd_property->runtime_cb[node->word_id];
        if ( runtime_cb ) {
            /* get memory */
            runtime = (icli_runtime_t *)icli_malloc( sizeof(icli_runtime_t) );
            if ( runtime == NULL ) {
                T_E("memory insufficient\n");
                return NULL;
            }
            memset(runtime, 0, sizeof(icli_runtime_t));

            ask = ICLI_ASK_BYWORD;
            _SEMA_GIVE();
            b = (*runtime_cb)(handle->session_id, ask, runtime);
            _SEMA_TAKE();
            if ( b == TRUE ) {
                byword = runtime->byword;
                icli_free(runtime);
                return byword;
            }
            icli_free(runtime);
        }

        /* check command property */
#if 1 /* CP, 2012/11/22 13:55, Bugzilla#10151 - int range using constant */
        bw = &(node->node_property.cmd_property->byword[node->word_id]);
        byword = bw->word;
        if ( icli_str_len(byword) ) {
            if ( bw->para_cnt ) {
                byword = (char *)malloc(ICLI_BYWORD_MAX_LEN + 1);
                if ( byword == NULL ) {
                    T_E("memory insufficient\n");
                    return FALSE;
                }
                memset(byword, 0, ICLI_BYWORD_MAX_LEN + 1);
                if ( bw->para_cnt == 1 ) {
                    (void)icli_snprintf(byword, ICLI_BYWORD_MAX_LEN, bw->word, bw->para[0]);
                } else {
                    (void)icli_snprintf(byword, ICLI_BYWORD_MAX_LEN, bw->word, bw->para[0], bw->para[1]);
                }
                bw->word = byword;
                bw->para_cnt = 0;
            }
            return byword;
        }
#else
        byword = node->node_property.cmd_property->byword[node->word_id];
        if ( icli_str_len(byword) ) {
            return byword;
        }
#endif
    }
    return node->word;
}

static i32 _max_word_len_get(
    IN  icli_session_handle_t   *handle
)
{
    i32                 max_word_len,
                        len;
    i32                 l;
    icli_parameter_t    *p;
    icli_port_type_t    port_type;

    max_word_len = 0;
    for ( p = handle->runtime_data.cmd_var; p != NULL; p = p->next ) {
        len = 0;
        switch ( p->match_node->type ) {
        case ICLI_VARIABLE_PORT_TYPE:
            for ( port_type = 1; port_type < ICLI_PORT_TYPE_MAX; port_type++ ) {
                if ( icli_port_type_present_x(port_type) ) {
                    l = icli_str_len( icli_variable_port_type_get_name(port_type) );
                    if ( l > len ) {
                        len = l;
                    }
                }
            }
            break;

        default:
            len = icli_str_len( _cmd_word_get(handle, p->match_node) );
            break;
        }
        if ( len > max_word_len ) {
            max_word_len = len;
        }
    }
    if ( handle->runtime_data.b_cr ) {
        if ( (i32)icli_str_len(__ICLI_CR_STR) > max_word_len ) {
            max_word_len = icli_str_len( __ICLI_CR_STR );
        }
    }
    return max_word_len;
}

static void _line_clear(
    IN  icli_session_handle_t   *handle
)
{
    /* usr input */
    switch ( _SESSON_INPUT_STYLE ) {
    case ICLI_INPUT_STYLE_SINGLE_LINE :
        icli_c_line_clear( handle );
        break;

    case ICLI_INPUT_STYLE_MULTIPLE_LINE :
        icli_a_line_clear( handle );
        break;

    case ICLI_INPUT_STYLE_SIMPLE :
        icli_z_line_clear( handle );
        break;

    default :
        T_E("invalid input_style = %d\n", _SESSON_INPUT_STYLE);
        break;
    }
}

#define __TAB_RESET_RETURN \
    if ( rc != ICLI_RC_OK ) { \
        icli_sutil_tab_reset( handle ); \
        return; \
    }

#define __TAB_WORD_DISPLAY(word) \
    if ( i && (i % word_cnt == 0) ) { \
        ICLI_PUT_NEWLINE; \
    } \
    w = word; \
    icli_session_str_put(handle, w); \
    j = max_word_len - icli_str_len(w); \
    for ( ; j > 0; j-- ) { \
        ICLI_PUT_SPACE; \
    } \
    icli_session_str_put(handle, "  "); \
    i++;

#define __TAB_PORT_TYPE_DISPLAY(t) \
    if ( icli_port_type_present_x(t) ) { \
        if ( (p->value.type == ICLI_VARIABLE_MAX) || \
             (p->value.type == ICLI_VARIABLE_PORT_TYPE && p->value.u.u_port_type == t)) { \
            __TAB_WORD_DISPLAY( icli_variable_port_type_get_name(t) ); \
        } \
    }


static void _tab_display(
    IN  icli_session_handle_t   *handle
)
{
    i32                 max_word_len,
                        word_cnt,
                        i,
                        j;
    icli_parameter_t    *p = NULL,
                         *q;
    char                *w = NULL,
                         *c;
    u32                 keyword = 0;
    icli_port_type_t    port_type;
    u32                 prefix_num;
    BOOL                b_last_cmd;
#if 1 /* CP, 2012/10/16 14:59, Bugzilla#10006 */
    u32                 cmd_len;
    u32                 width;
#endif

    if ( handle->runtime_data.cmd_var ) {
        if ( handle->runtime_data.cmd_len ) {
            w = &( handle->runtime_data.cmd[handle->runtime_data.cmd_len - 1] );
            if ( ICLI_IS_(SPACE, *w) ) {
                keyword = 0;
            } else {
                keyword = 1;
            }
        } else {
            keyword = 0;
        }

        if ( keyword ) {
            if ( handle->runtime_data.cmd_var->next != NULL ) {
                /*
                    multiple match,
                    if only one keyword then complete the keyword
                 */
                keyword = 0;
                for ( q = handle->runtime_data.cmd_var; q != NULL; q = q->next ) {
                    if ( q->match_node->type == ICLI_VARIABLE_KEYWORD       ||
                         q->match_node->type == ICLI_VARIABLE_PORT_TYPE     ||
                         q->match_node->type == ICLI_VARIABLE_GREP          ||
                         q->match_node->type == ICLI_VARIABLE_GREP_BEGIN    ||
                         q->match_node->type == ICLI_VARIABLE_GREP_INCLUDE  ||
                         q->match_node->type == ICLI_VARIABLE_GREP_EXCLUDE
                       ) {
                        p = q;
                        keyword++;
                    }
                }
                if ( keyword != 1 ) {
                    keyword = 0;
                }
            } else {
                /* single match */
                p = handle->runtime_data.cmd_var;
                if ( p->match_node->type != ICLI_VARIABLE_KEYWORD       &&
                     p->match_node->type != ICLI_VARIABLE_PORT_TYPE     &&
                     p->match_node->type != ICLI_VARIABLE_GREP          &&
                     p->match_node->type != ICLI_VARIABLE_GREP_BEGIN    &&
                     p->match_node->type != ICLI_VARIABLE_GREP_INCLUDE  &&
                     p->match_node->type != ICLI_VARIABLE_GREP_EXCLUDE
                   ) {
                    keyword = 0;
                }
            }
        }
    }

    if ( keyword ) {
        if ( p == NULL ) {
            T_E("p == NULL\n");
            return;
        }

        handle->runtime_data.tab_cnt = 1;

        if ( (p->match_node->type == ICLI_VARIABLE_PORT_TYPE) &&
             (handle->runtime_data.tab_port_type == ICLI_PORT_TYPE_NONE) ) {
            handle->runtime_data.tab_port_type = icli_port_next_present_type_x( ICLI_PORT_TYPE_NONE );
        }

        // find position to append
        if ( handle->runtime_data.cmd_len ) {
            w = &( handle->runtime_data.cmd[handle->runtime_data.cmd_len - 1] );
            if ( ICLI_IS_(SPACE, *w) ) {
                w++;
            } else {
                *w = ICLI_EOS;
                for ( i = handle->runtime_data.cmd_len - 1; i > 0; i-- ) {
                    w--;
                    if ( ICLI_IS_(SPACE, *w) ) {
                        w++;
                        break;
                    } else {
                        *w = ICLI_EOS;
                    }
                }
            }
        } else {
            w = handle->runtime_data.cmd;
        }

        // copy keyword
        switch (p->match_node->type) {
        case ICLI_VARIABLE_KEYWORD:
            (void)icli_str_cpy(w, p->match_node->word);
            break;

        case ICLI_VARIABLE_PORT_TYPE:
            if ( p->value.type == ICLI_VARIABLE_PORT_TYPE ) {
                (void)icli_str_cpy(w, icli_variable_port_type_get_name(p->value.u.u_port_type));
            } else {
                (void)icli_str_cpy(w, icli_variable_port_type_get_name(handle->runtime_data.tab_port_type));
            }
            break;

        case ICLI_VARIABLE_GREP:
            (void)icli_str_cpy(w, ICLI_GREP_KEYWORD);
            break;

        case ICLI_VARIABLE_GREP_BEGIN:
            (void)icli_str_cpy(w, ICLI_GREP_BEGIN_KEYWORD);
            break;

        case ICLI_VARIABLE_GREP_INCLUDE:
            (void)icli_str_cpy(w, ICLI_GREP_INCLUDE_KEYWORD);
            break;

        case ICLI_VARIABLE_GREP_EXCLUDE:
            (void)icli_str_cpy(w, ICLI_GREP_EXCLUDE_KEYWORD);
            break;

        default:
            break;
        }

        /* append " " at the end */
        for ( ; ICLI_NOT_(EOS, *w); w++ ) {
            ;
        }
        *w++ = ICLI_SPACE;
        *w   = ICLI_EOS;

        // clear line
        _line_clear( handle );

#if 1 /* CP, 2012/10/16 14:59, Bugzilla#10006 */
        // check if right scroll
        cmd_len = icli_str_len( handle->runtime_data.cmd );
        width = handle->runtime_data.width - handle->runtime_data.prompt_len - 1;
        if ( (cmd_len - handle->runtime_data.start_pos) > (width - _SCROLL_FACTOR) ) {
            handle->runtime_data.start_pos = cmd_len - (width - _SCROLL_FACTOR);
        }
#endif

        // display again
        _cmd_display( handle );

#if 0
        /* reset handle */
        icli_sutil_tab_reset( handle );
#endif

    } else {
        ICLI_PLAY_BELL;

        ICLI_PUT_NEWLINE;

        i = 0;
        word_cnt = 0;
        if ( handle->runtime_data.cmd_var ) {
            max_word_len = _max_word_len_get( handle );
            word_cnt = ((i32)(handle->runtime_data.width) - 1) / (max_word_len + 2);
            for ( i = 0, p = handle->runtime_data.cmd_var; p != NULL; p = p->next ) {
                switch ( p->match_node->type ) {
                case ICLI_VARIABLE_PORT_TYPE:
                    for ( port_type = 1; port_type < ICLI_PORT_TYPE_MAX; port_type++ ) {
                        __TAB_PORT_TYPE_DISPLAY( port_type );
                    }
                    break;

                default:
                    __TAB_WORD_DISPLAY( _cmd_word_get(handle, p->match_node) );
                    break;
                }
            }
        }

        if ( handle->runtime_data.b_cr ) {
            if ( i && word_cnt && (i % word_cnt == 0) ) {
                ICLI_PUT_NEWLINE;
            }
            icli_session_str_put(handle, __ICLI_CR_STR);
        }
        ICLI_PUT_NEWLINE;

        /* process prefix */
        keyword = 1;
        for ( p = handle->runtime_data.cmd_var; p != NULL; p = p->next ) {
            if ( p->match_node->type != ICLI_VARIABLE_KEYWORD ) {
                keyword = 0;
                break;
            }
        }

        if ( keyword == 0 ) {
            icli_sutil_tab_reset( handle );
            return;
        }

        /* get the beginning of the last command */
        b_last_cmd = FALSE;
        if ( handle->runtime_data.cmd_len ) {
            i = handle->runtime_data.cmd_len - 1;
            w = &( handle->runtime_data.cmd[i] );
            if ( ICLI_NOT_(SPACE, *w) ) {
                b_last_cmd = TRUE;
                for ( ; i > 0; i-- ) {
                    w--;
                    if ( ICLI_IS_(SPACE, *w) ) {
                        w++;
                        break;
                    }
                }
            }
        }

        if ( b_last_cmd == FALSE ) {
            icli_sutil_tab_reset( handle );
            return;
        }

        /* get the number of same prefix in multiple matches */
        if ( handle->runtime_data.cmd_var ) {
            prefix_num = 0;
            p = handle->runtime_data.cmd_var;
            for ( q = p->next; q != NULL; q = q->next ) {
                prefix_num = icli_str_prefix( p->match_node->word, q->match_node->word, prefix_num, 0);
            }

            j = icli_str_len(w);
            i = prefix_num - j;
            c = p->match_node->word + j;
            w = &( handle->runtime_data.cmd[handle->runtime_data.cmd_len] );
            for ( ; i > 0; i-- ) {
                *w++ = *c++;
            }
            *w = ICLI_EOS;
        }

        /* reset handle */
        icli_sutil_tab_reset( handle );
    }
}

static char *_help_get(
    IN  icli_session_handle_t   *handle,
    IN  icli_parsing_node_t     *node
)
{
    u32                 b;
    char                *help;
    icli_runtime_cb_t   *runtime_cb;
    icli_runtime_ask_t  ask;
    icli_runtime_t      *runtime;

    /* check runtime first */
    runtime_cb = node->node_property.cmd_property->runtime_cb[node->word_id];
    if ( runtime_cb ) {
        /* get memory */
        runtime = (icli_runtime_t *)icli_malloc( sizeof(icli_runtime_t) );
        if ( runtime == NULL ) {
            T_E("memory insufficient\n");
            return NULL;
        }
        memset(runtime, 0, sizeof(icli_runtime_t));

        ask = ICLI_ASK_HELP;
        _SEMA_GIVE();
        b = (*runtime_cb)(handle->session_id, ask, runtime);
        _SEMA_TAKE();
        if ( b == TRUE ) {
            help = runtime->help;
            icli_free(runtime);
            return help;
        }
        icli_free(runtime);
    }

    /* check command property */
    help = node->node_property.cmd_property->help[node->word_id];
    return help;
}

/*
    output formated string to a specific session

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
static void _session_printf(
    IN  icli_session_handle_t   *handle,
    IN  const char              *format,
    IN  ...
)
{
    va_list     arglist = NULL;

    switch ( handle->open_data.way ) {
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        if ( handle->runtime_data.line_mode == ICLI_LINE_MODE_BYPASS ) {
            return;
        }
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
        return;

    default:
        return;
    }

    va_start( arglist, format );
    (void)icli_session_va_str_put(handle->session_id, format, arglist);
    va_end( arglist );
}

#if 1 /* CP, 2012/09/25 10:47, <dscp> */
static void _help_display(
    IN  icli_session_handle_t   *handle,
    IN  char                    *word,
    IN  char                    *help
)
{
    i32                 max_word_len,
                        help_len,
                        space_len,
                        display_len,
                        n,
                        i;
    char                *c, *s;

    //4 for the gap to help string
    max_word_len = _max_word_len_get( handle ) + 4;

    //4 for first spaces and 4 for the gap to help string
    space_len    = 4 + max_word_len;

    //1 for EOS
    display_len  = handle->runtime_data.width - space_len - 1;

    //word
    _session_printf(handle, "    ");
    _session_printf(handle, word);

    for ( i = icli_str_len(word); i < max_word_len; i++ ) {
        _session_printf(handle, " ");
    }

    //help
    help_len = icli_str_len( help );
    if ( help_len > display_len ) {
        for ( c = help, n = 0, s = c; ; c++, n++ ) {

            //count number of char in a word
            if ( ICLI_NOT_(SPACE, *c) && ICLI_NOT_(EOS, *c) ) {
                continue;
            }

            //check if over one line
            if ( n > display_len ) {
                n = c - s;

                _session_printf(handle, "\n");

                for ( i = 0; i < space_len; i++ ) {
                    _session_printf(handle, " ");
                }
                //skip the first space
                if ( ICLI_IS_(SPACE, *s) ) {
                    s++;
                    n--;
                }
            }

            //display word
            for ( ; s != c; s++ ) {
                _session_printf(handle, "%c", *s);
            }

            //end of string
            if ( ICLI_IS_(EOS, *c) ) {
                _session_printf(handle, "\n");
                break;
            }
        }
    } else {
        _session_printf(handle, help);
        _session_printf(handle, "\n");
    }
}
#endif

#define __PORT_TYPE_HELP(t) \
    if ( icli_port_type_present_x(t) ) { \
        if ( (p->value.type == ICLI_VARIABLE_MAX) || \
             (p->value.type == ICLI_VARIABLE_PORT_TYPE && p->value.u.u_port_type == t)) { \
            _help_display(handle, icli_variable_port_type_get_name(t), icli_variable_port_type_get_help(t));\
        } \
    }

static BOOL _port_help_display(
    IN  icli_session_handle_t   *handle,
    IN  char                    *cmd_word,
    IN  BOOL                    b_port_list
)
{
    char                        *cmd_help;
    char                        *s;
    icli_stack_port_range_t     *range;
    u32                         i;
    icli_switch_port_range_t    *spr;
    u16                         last_switch_id;
    u16                         last_begin_port;
    u16                         last_end_port;

    switch ( handle->runtime_data.last_port_type ) {
    case ICLI_PORT_TYPE_FAST_ETHERNET:
    case ICLI_PORT_TYPE_GIGABIT_ETHERNET:
    case ICLI_PORT_TYPE_2_5_GIGABIT_ETHERNET:
    case ICLI_PORT_TYPE_FIVE_GIGABIT_ETHERNET:
    case ICLI_PORT_TYPE_TEN_GIGABIT_ETHERNET:
        break;

    default:
        return FALSE;
    }

    /* allocate memory */
    cmd_help = (char *)malloc(__ICLI_HELP_MAX_LEN + 1);
    if ( cmd_help == NULL ) {
        T_E("Memory insufficient\n");
        return FALSE;
    }
    memset(cmd_help, 0, __ICLI_HELP_MAX_LEN + 1);

    /* get port range */
    range = icli_port_range_get_x();

    /* make help string */
    s = cmd_help;
    if ( b_port_list ) {
        (void)icli_str_cpy(s, "Port list in ");
    } else {
        (void)icli_str_cpy(s, "Port ID in ");
    }
    s = cmd_help + icli_str_len(cmd_help);

    last_switch_id  = 0;
    last_begin_port = 0;
    last_end_port   = 0;
    for ( i = 0; i < range->cnt; i++ ) {
        spr = &(range->switch_range[i]);
        if ( spr->port_type != (u8)(handle->runtime_data.last_port_type) ) {
            continue;
        }
        if ( spr->switch_id != last_switch_id ) {
            if ( last_switch_id ) {
                /* not first */
                if ( last_begin_port == last_end_port ) {
                    icli_sprintf(s, ",%u/%u", spr->switch_id, spr->begin_port);
                } else {
                    icli_sprintf(s, "-%u,%u/%u", last_end_port, spr->switch_id, spr->begin_port);
                }
            } else {
                /* first */
                icli_sprintf(s, "%u/%u", spr->switch_id, spr->begin_port);
            }

        } else {
            /* check if port is continuous */
            if ( spr->begin_port == (last_end_port + 1) ) {
                /* continuous, only need to update last_end_port */
                last_end_port = spr->begin_port + spr->port_cnt - 1;
                continue;
            } else {
                /* not continuous */
                if ( last_begin_port == last_end_port ) {
                    icli_sprintf(s, ",%u", spr->begin_port);
                } else {
                    icli_sprintf(s, "-%u,%u", last_end_port, spr->begin_port);
                }
            }
        }

        /* update s */
        s = cmd_help + icli_str_len(cmd_help);

        /* update last data */
        last_switch_id  = spr->switch_id;
        last_begin_port = spr->begin_port;
        last_end_port   = spr->begin_port + spr->port_cnt - 1;
    }

    /* add last_end_port */
    if ( last_begin_port != last_end_port ) {
        icli_sprintf(s, "-%u", last_end_port);
    }

    /* display help */
    _help_display(handle, cmd_word, cmd_help);

    /* free memory */
    free( cmd_help );
    return TRUE;
}

static void _question_display(
    IN  icli_session_handle_t   *handle
)
{
    icli_parameter_t    *p;
    icli_port_type_t    port_type;
    char                *cmd_word;
    char                *cmd_help;

    for ( p = handle->runtime_data.cmd_var; p != NULL; p = p->next) {
        cmd_word = _cmd_word_get(handle, p->match_node);
        cmd_help = _help_get(handle, p->match_node);
        switch ( p->match_node->type ) {
        case ICLI_VARIABLE_PORT_TYPE:
            for ( port_type = 1; port_type < ICLI_PORT_TYPE_MAX; port_type++ ) {
                __PORT_TYPE_HELP( port_type );
            }
            break;

        case ICLI_VARIABLE_PORT_ID:
        case ICLI_VARIABLE_PORT_TYPE_ID:
            if ( _port_help_display(handle, cmd_word, FALSE) == FALSE ) {
                _help_display(handle, cmd_word, cmd_help);
            }
            break;

        case ICLI_VARIABLE_PORT_LIST:
        case ICLI_VARIABLE_PORT_TYPE_LIST:
            if ( _port_help_display(handle, cmd_word, TRUE) == FALSE ) {
                _help_display(handle, cmd_word, cmd_help);
            }
            break;

        default:
            _help_display(handle, cmd_word, cmd_help);
            break;
        }
    }

    if ( handle->runtime_data.b_cr ) {
        _session_printf(handle, "    ");
        _session_printf(handle, __ICLI_CR_STR);
        _session_printf(handle, "\n");
    }
}

static i32 _line_mode(
    IN icli_session_handle_t    *handle
)
{
    i32     rc = ICLI_RC_ERR_PARAMETER;

    switch ( _SESSON_INPUT_STYLE ) {
    case ICLI_INPUT_STYLE_SINGLE_LINE :
        rc = icli_c_line_mode( handle );
        break;

    case ICLI_INPUT_STYLE_MULTIPLE_LINE :
        rc = icli_a_line_mode( handle );
        break;

    case ICLI_INPUT_STYLE_SIMPLE :
        rc = icli_z_line_mode( handle );
        break;

    default :
        T_E("invalid input_style = %d\n", _SESSON_INPUT_STYLE);
        break;
    }
    return rc;
}

/*
    put string buffer to a session, processed line by line

    INPUT
        session_id : the session ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        process MORE page here
*/
static i32 _session_str_put(
    IN icli_session_handle_t   *handle
)
{
    char                *c,
                        x,
                        *l;
    i32                 rc;
    i32                 b_in_exec_cb;
    BOOL                b_more,
                        b_print;
    icli_parameter_t    *p;

    //output the string
    if ( handle->runtime_data.b_in_exec_cb == FALSE ) {
        handle->runtime_data.put_str = handle->runtime_data.str_buf;
        (void)icli_sutil_usr_str_put( handle );
        //empty string buffer
        handle->runtime_data.str_buf[0] = 0;
        return ICLI_RC_OK;
    }

    switch ( handle->runtime_data.line_mode ) {
    case ICLI_LINE_MODE_PAGE:
        b_print = FALSE;
        break;

    case ICLI_LINE_MODE_FLOOD:
        b_print = TRUE;
        break;

    case ICLI_LINE_MODE_BYPASS:
        //empty string buffer
        handle->runtime_data.str_buf[0] = 0;
        return ICLI_RC_OK;

    default :
        T_E("invalid line_mode = %d\n", handle->runtime_data.line_mode);
        return ICLI_RC_ERR_PARAMETER;
    }

    //get line count
    x       = 0;
    b_more  = FALSE;
    p       = handle->runtime_data.grep_var;
    l       = (char *)(handle->runtime_data.str_buf);
    for ( c = l; ICLI_NOT_(EOS, *c); c++ ) {
        /* new line ? */
        if ( ICLI_NOT_(NEWLINE, *c) ) {
            continue;
        }

        /* GREP */
        b_print = TRUE;
        if ( p ) {
            switch ( p->next->value.type ) {
            case ICLI_VARIABLE_GREP_BEGIN:
                if ( handle->runtime_data.grep_begin == 0 ) {
                    if ( icli_str_str(p->next->next->value.u.u_line, l) ) {
                        handle->runtime_data.grep_begin = 1;
                    } else {
                        b_print = FALSE;
                    }
                }
                break;

            case ICLI_VARIABLE_GREP_INCLUDE:
                if ( icli_str_str(p->next->next->value.u.u_line, l) == NULL ) {
                    b_print = FALSE;
                }
                break;

            case ICLI_VARIABLE_GREP_EXCLUDE:
                if ( icli_str_str(p->next->next->value.u.u_line, l) != NULL ) {
                    b_print = FALSE;
                }
                break;

            default:
                break;
            }
        }

        /* if print this line */
        if ( b_print == FALSE ) {
            (void)icli_str_cpy(handle->runtime_data.str_buf, c + 1);
            /* -1 because c++ in for() */
            c = l;
            c--;
            continue;
        }

        /* only PAGE mode to count line number */
        if ( handle->runtime_data.line_mode == ICLI_LINE_MODE_PAGE) {
            handle->runtime_data.line_cnt++;
        }

        /* print by line */
        c++;
        x  = *c;
        *c = ICLI_EOS;

        /* check if more */
        if ( handle->runtime_data.line_mode == ICLI_LINE_MODE_PAGE &&
             handle->runtime_data.lines &&
             handle->runtime_data.line_cnt >= (i32)(handle->runtime_data.lines - 2) ) {
            b_more = TRUE;
        }

        /* go output */
        break;
    }

    if ( b_print == FALSE ) {
        return ICLI_RC_OK;
    }

    //output the line
    handle->runtime_data.put_str = l;
    (void)icli_sutil_usr_str_put( handle );

    /* remove the line */
    *c = x;
    if ( x ) {
        (void)icli_str_cpy(handle->runtime_data.str_buf, c);
    } else {
        handle->runtime_data.str_buf[0] = 0;
    }

    if ( b_more ) {
        /* set to 0 to avoid grep in _line_mode() */
        b_in_exec_cb = handle->runtime_data.b_in_exec_cb;
        handle->runtime_data.b_in_exec_cb = FALSE;

        /* get usr input to see the display mode */
        rc = _line_mode( handle );

        /* restore to avoid grep in _line_mode() */
        handle->runtime_data.b_in_exec_cb = b_in_exec_cb;

        /* check result of _line_mode() */
        if ( rc != ICLI_RC_OK ) {
            return rc;
        }
    }

    /* if there is string behind, then print it */
    if ( x ) {
        //recursive
        (void)_session_str_put( handle );
    }

    return ICLI_RC_OK;
}

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/*
    initialization

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_init(void)
{
    u32                 i;

#if 1 /* CP, 2012/09/14 10:38, conf */
    icli_conf_data_t    *conf = icli_conf_get_x();
#endif

    memset(g_session_handle, 0, sizeof(g_session_handle));

    for ( i = 0; i < ICLI_SESSION_CNT; i++ ) {

#if 1 /* CP, 2012/09/14 10:38, conf */
        // link to session config data
        g_session_handle[i].config_data = &( conf->session_config[i] );
#endif

        (void)icli_session_config_data_default( g_session_handle[i].config_data );
        _runtime_data_default( &(g_session_handle[i]) );
        g_session_handle[i].session_id = i;
    }
    g_max_sessions = ICLI_SESSION_DEFAULT_CNT;
    return ICLI_RC_OK;
}

#define __AUTH_INPUT_AND_CHECK(s, t)\
    size = sizeof(s);\
    memset(s, 0, size);\
    rc = icli_session_usr_str_get_x(session_id, t, s, &size, NULL);\
    /* timeout */\
    if ( rc != ICLI_RC_OK ) {\
        if (handle->open_data.way == ICLI_SESSION_WAY_TELNET || handle->open_data.way == ICLI_SESSION_WAY_SSH) {\
            /* close connection */\
            handle->runtime_data.alive = FALSE;\
        } else {\
            /* console never die */\
            handle->runtime_data.alive = TRUE;\
            auth_cnt = 0;\
        }\
        continue;\
    }

/*
    ICLI engine for each session

    INPUT
        session_id : session ID

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_engine(
    IN i32      session_id
)
{
    icli_session_handle_t   *handle;
    char                    *banner;
    BOOL                    do_auth,
                            b,
                            b_space;
    char                    username[ICLI_USERNAME_MAX_LEN + 1],
                            password[ICLI_PASSWORD_MAX_LEN + 1],
                            *d;
    i32                     c,
                            rc,
                            size,
                            auth_cnt;
    i32                     b_in_exec_cb;

    if ( session_id < 0 || session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session ID = %d\n", session_id);
        return ICLI_RC_ERROR;
    }

    /* take semaphore */
    _SEMA_TAKE();

    /* get session handle */
    handle = &g_session_handle[session_id];

    if ( handle->in_used == FALSE ) {
        T_E("session %d is not in used\n", session_id);
        /* give semaphore */
        _SEMA_GIVE();
        return ICLI_RC_ERROR;
    }

#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
    handle->runtime_data.thread_id = icli_thread_id_get();
    if ( handle->runtime_data.thread_id == 0 ) {
        T_E("invalid thread ID = %u\n", handle->runtime_data.thread_id);
        /* give semaphore */
        _SEMA_GIVE();
        return ICLI_RC_ERROR;
    }
#endif

    switch ( handle->open_data.way ) {
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_CONSOLE:
        /* invoke APP init */
        if ( _usr_app_init(handle) == FALSE ) {
            T_E("APP initialization fails on session %d\n", session_id);
            /* give semaphore */
            _SEMA_GIVE();
            return ICLI_RC_ERROR;
        }

        /* do authentication */
        do_auth = TRUE;
        break;

    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        /* authentication by application thread */
        do_auth = FALSE;
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
    default:
        T_E("invalid type %d of session ID = %d\n", handle->open_data.way, session_id);
        /* give semaphore */
        _SEMA_GIVE();
        return ICLI_RC_ERROR;
    }

    /* initialization */
    auth_cnt = 0;
    if ( handle->runtime_data.alive == FALSE ) {
        handle->runtime_data.alive = TRUE;
    }

    while ( handle->runtime_data.alive ) {

        if ( do_auth ) {
            /* display MOTD banner */
            if ( handle->config_data->b_motd_banner ) {
                banner = icli_banner_motd_get_x();
                if ( icli_str_len(banner) ) {
                    ICLI_PUT_NEWLINE;
                    icli_session_str_put(handle, banner);
                    ICLI_PUT_NEWLINE;
                }
            }

            /* reset line */
            handle->runtime_data.line_cnt = 0;

            /* usr authentication */
            switch ( handle->open_data.way ) {
            case ICLI_SESSION_WAY_CONSOLE:
                if ( auth_cnt == 0 ) {
                    ICLI_PUT_NEWLINE;
                    icli_session_str_put(handle, "press ENTER to get started");
                    for ( b = TRUE, c = 0;
                          b && ICLI_NOT_(KEY_ENTER, c) && ICLI_NOT_(NEWLINE, c);
                          b = icli_sutil_usr_char_get(handle, ICLI_TIMEOUT_FOREVER, &c) ) {
                        ;
                    }
                    if ( b == FALSE ) {
                        icli_sleep( 1000 );
                        continue;
                    }
                    ICLI_PUT_NEWLINE;
                }

                /* pass through */

            case ICLI_SESSION_WAY_TELNET:
                /* without auth */
                if ( ! icli_has_user_auth() ) {
                    handle->runtime_data.privilege = 0;
                    break;
                }

                /* display LOGIN banner */
                banner = icli_banner_login_get_x();
                if ( icli_str_len(banner) ) {
                    ICLI_PUT_NEWLINE;
                    icli_session_str_put(handle, banner);
                    ICLI_PUT_NEWLINE;
                }

                icli_session_str_put(handle, "Username: ");
                __AUTH_INPUT_AND_CHECK(username, ICLI_USR_INPUT_TYPE_NORMAL);

                icli_session_str_put(handle, "Password: ");
                __AUTH_INPUT_AND_CHECK(password, ICLI_USR_INPUT_TYPE_PASSWORD);

                rc = icli_user_auth(handle->open_data.way, username, password, (i32 *) & (handle->runtime_data.privilege));
                if ( rc != ICLI_RC_OK ) {
                    icli_session_str_put(handle, "Wrong username or password!\n\n");
                    if ( ++auth_cnt >= ICLI_AUTH_MAX_CNT ) {
                        /* try too many and close connection */
                        if (handle->open_data.way == ICLI_SESSION_WAY_TELNET || handle->open_data.way == ICLI_SESSION_WAY_SSH) {
                            handle->runtime_data.alive = FALSE;
                        }
                    }
                    continue;
                }
#if 1 /* CP, 2012/09/04 16:46, session user name */
                (void)icli_str_ncpy(handle->runtime_data.user_name, username, ICLI_USERNAME_MAX_LEN);
#endif
                break;

            case ICLI_SESSION_WAY_SSH:
                /*
                    SSH does AUTH by itself.
                    After SSH does it,
                    icli_ssh_init will set the privilege back.
                */
                break;

            default:
                T_E("invalid type %d of session ID = %d\n", handle->open_data.way, session_id);
                /* give semaphore */
                _SEMA_GIVE();
                return ICLI_RC_ERROR;
            }

            /* display EXEC banner */
            if ( handle->config_data->b_exec_banner ) {
                banner = icli_banner_exec_get_x();
                if ( icli_str_len(banner) ) {
                    ICLI_PUT_NEWLINE;
                    icli_session_str_put(handle, banner);
                    ICLI_PUT_NEWLINE;
                }
            }

            do_auth = FALSE;
            handle->runtime_data.history_head = -1;
            handle->runtime_data.history_tail = -1;
            handle->runtime_data.tab_cnt      = 0;
            handle->runtime_data.exec_type    = 0;
        }

        if ( handle->runtime_data.connect_time == 0 ) {
            handle->runtime_data.connect_time = icli_current_time_get();
        }

        /* reset line */
        handle->runtime_data.line_cnt = 0;
        if ( handle->runtime_data.lines ) {
            handle->runtime_data.line_mode = ICLI_LINE_MODE_PAGE;
        } else {
            // if lines is 0, then do not page it, just flood it.
            handle->runtime_data.line_mode = ICLI_LINE_MODE_FLOOD;
        }

        /* rewind history */
        handle->runtime_data.history_pos = -1;

        /* display prompt */
        if ( handle->runtime_data.tab_cnt == 0 ) {
            _prompt_display( handle );
        }

        /* restore original user command */
        if ( handle->runtime_data.b_comment && handle->runtime_data.comment_pos ) {
            *(handle->runtime_data.comment_pos) = (char)(handle->runtime_data.comment_ch);
        }

        /* reset cmd */
        switch ( handle->runtime_data.exec_type ) {
        case ICLI_EXEC_TYPE_TAB:
            if ( handle->runtime_data.tab_cnt ) {
                icli_sutil_tab_reset( handle );
                break;
            }
            //fall through

        case ICLI_EXEC_TYPE_QUESTION:
            _cmd_display( handle );
            break;

        case ICLI_EXEC_TYPE_CMD:
        case ICLI_EXEC_TYPE_PARSING:
        default :
            icli_sutil_cmd_reset( handle );
            break;

        case ICLI_EXEC_TYPE_REDISPLAY:
            _cmd_redisplay( handle );
            break;
        }

#if 1 /* CP, 2012/10/16 17:02, ICLI_RC_CHECK */
        handle->runtime_data.rc_context_string = NULL;
#endif

        /* get usr command */
        rc = _usr_cmd_get( session_id );

        switch ( rc ) {
        case ICLI_RC_OK:

            /* pre-process before command execution */
            switch ( handle->runtime_data.exec_type ) {
            case ICLI_EXEC_TYPE_CMD:
                if ( icli_str_cmp(handle->runtime_data.cmd, ICLI_DEBUG_PRIVI_CMD) == 0 ) {
                    handle->runtime_data.privilege = ICLI_PRIVILEGE_DEBUG;
                    continue;
                } else {
                    _history_cmd_add( handle );
                }
                break;

            case ICLI_EXEC_TYPE_PARSING:
            case ICLI_EXEC_TYPE_QUESTION:
            case ICLI_EXEC_TYPE_TAB:
                break;

            case ICLI_EXEC_TYPE_REDISPLAY:
                continue;

            default:
                break;
            }

            // skip space
            ICLI_SPACE_SKIP(d, handle->runtime_data.cmd);

            // comment
            if ( ICLI_IS_COMMENT(*d) ) {
                continue;
            }

            /* check COMMENT inside */
            handle->runtime_data.b_comment = FALSE;
            b_space = TRUE;
            for (handle->runtime_data.comment_pos = handle->runtime_data.cmd; ICLI_NOT_(EOS, *(handle->runtime_data.comment_pos)); (handle->runtime_data.comment_pos)++) {
                if (b_space && ICLI_IS_COMMENT(*(handle->runtime_data.comment_pos)) ) {
                    handle->runtime_data.comment_ch = *(handle->runtime_data.comment_pos);
                    *(handle->runtime_data.comment_pos) = ICLI_EOS;
                    handle->runtime_data.b_comment = TRUE;
                    break;
                }
                if ( ICLI_IS_(SPACE, *(handle->runtime_data.comment_pos)) ) {
                    b_space = TRUE;
                } else {
                    b_space = FALSE;
                }
            }
            if ( handle->runtime_data.b_comment && handle->runtime_data.cmd[0] == ICLI_EOS ) {
                // the first char is COMMENT symbol
                handle->runtime_data.b_comment = FALSE;
                for ( (handle->runtime_data.comment_pos)++; ICLI_NOT_(EOS, *(handle->runtime_data.comment_pos)); (handle->runtime_data.comment_pos)++) {
                    *(handle->runtime_data.comment_pos) = ICLI_EOS;
                }
                if ( handle->runtime_data.exec_type != ICLI_EXEC_TYPE_QUESTION ) {
                    ICLI_PUT_NEWLINE;
                }
                continue;
            }

            // exec by engine
            handle->runtime_data.b_exec_by_api    = FALSE;
            handle->runtime_data.app_err_msg      = NULL;
            handle->runtime_data.err_display_mode = ICLI_ERR_DISPLAY_MODE_PRINT;

            /* command execution */
            rc = icli_exec( handle );

            /* get result */
            handle->runtime_data.rc = rc;

            /* process after command execution */
            switch ( rc ) {
            case ICLI_RC_OK :
                switch ( handle->runtime_data.exec_type ) {
                case ICLI_EXEC_TYPE_TAB:
                    if ( handle->runtime_data.b_comment ) {
                        ICLI_PUT_NEWLINE;
                    } else {
                        _tab_display( handle );
                    }
                    break;

                case ICLI_EXEC_TYPE_QUESTION:
                    // if comment, do nothing
                    if ( handle->runtime_data.b_comment ) {
                        break;
                    }
                    /* set b_in_exec_cb */
                    b_in_exec_cb = handle->runtime_data.b_in_exec_cb;
                    handle->runtime_data.b_in_exec_cb = TRUE;

                    _question_display( handle );

                    /* clear b_in_exec_cb */
                    handle->runtime_data.b_in_exec_cb = b_in_exec_cb;

                    //clear str_buf
                    memset(handle->runtime_data.str_buf, 0, sizeof(handle->runtime_data.str_buf));

                    //fall through

                case ICLI_EXEC_TYPE_CMD:
                    // MUST be sure that all will always put \n anyway
                    // flush all output
                    (void)_session_str_put( handle );

                    // clear string buf
                    handle->runtime_data.str_buf[0] = 0;

                    // clear more buf
                    handle->runtime_data.cmd_copy[0] = 0;

                    //fall through

                case ICLI_EXEC_TYPE_PARSING:
                default:
                    icli_exec_para_list_free( &(handle->runtime_data.cmd_var) );
                    handle->runtime_data.grep_var   = NULL;
                    handle->runtime_data.grep_begin = 0;
                    break;
                }
                break;

            default :
                break;
            }

            //fall through

        case ICLI_RC_ERR_EMPTY:

            /* action after executing command */
            switch ( handle->runtime_data.after_cmd_act ) {
            case ICLI_AFTER_CMD_ACT_NONE:
            default:
                break;

            case ICLI_AFTER_CMD_ACT_GOTO_PREVIOUS_MODE:
                (void)icli_session_mode_exit_x( handle->session_id );
                break;

            case ICLI_AFTER_CMD_ACT_GOTO_EXEC_MODE:
                _goback_exec_mode( handle );
                break;
            }
            break;

        case ICLI_RC_ERR_EXPIRED:
            /* logout */
            handle->runtime_data.alive = FALSE;
            break;

        case ICLI_RC_ERROR:
        case ICLI_RC_ERR_PARAMETER:
            handle->runtime_data.alive = FALSE;
            break;
        }

        switch ( handle->open_data.way ) {
        case ICLI_SESSION_WAY_CONSOLE:
            if ( handle->runtime_data.alive == FALSE ) {
                if ( icli_console_alive_get() ) {
                    /* redo auth */
                    do_auth  = TRUE;
                    auth_cnt = 0;

                    /* reset runtime data */
                    _runtime_data_clear( handle );

                    /* alive again */
                    handle->runtime_data.alive = TRUE;
#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
                    handle->runtime_data.thread_id = icli_thread_id_get();
                    if ( handle->runtime_data.thread_id == 0 ) {
                        T_E("invalid thread ID = %u\n", handle->runtime_data.thread_id);
                        /* give semaphore */
                        _SEMA_GIVE();
                        return ICLI_RC_ERROR;
                    }
#endif
                }
            }
            break;

        case ICLI_SESSION_WAY_TELNET:
        case ICLI_SESSION_WAY_SSH:
            break;

        case ICLI_SESSION_WAY_THREAD_CONSOLE:
        case ICLI_SESSION_WAY_THREAD_TELNET:
        case ICLI_SESSION_WAY_THREAD_SSH:
            if ( handle->runtime_data.alive ) {
                rc = ICLI_RC_OK;
            } else {
                /*
                    the decision that the session is closed or not is by application,
                    so make it alive for the later close decision by application.
                 */
                handle->runtime_data.alive = TRUE;
                rc = ICLI_RC_ERROR;
            }
            /* give semaphore */
            _SEMA_GIVE();
            return rc;

        case ICLI_SESSION_WAY_APP_EXEC:
        default:
            T_E("invalid type %d of session ID = %d\n", handle->open_data.way, session_id);
            /* give semaphore */
            _SEMA_GIVE();
            return ICLI_RC_ERROR;
        }

    } //while ( handle->runtime_data.alive )

    /* invoke APP close */
    _usr_app_close( handle, ICLI_CLOSE_REASON_NORMAL );

    /* free session */
    _session_free( session_id );

    /* give semaphore */
    _SEMA_GIVE();
    return ICLI_RC_OK;

}//icli_session_engine

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
i32 icli_session_open_x(
    IN  icli_session_open_data_t    *open_data,
    OUT u32                         *session_id
)
{
    icli_session_handle_t   *handle;
    i32                     sid,
                            rc = ICLI_RC_ERROR;

    /* get free session */
    sid = _session_get();
    if ( sid < 0 ) {
        T_E("session full\n");
        return ICLI_RC_ERR_MEMORY;
    }

    /* get handle */
    handle = &g_session_handle[sid];

    /* get session data */
    handle->open_data = *open_data;

    /* create thread accordingly */
    switch ( handle->open_data.way ) {
    case ICLI_SESSION_WAY_CONSOLE:
        rc = icli_thread_create( sid,
                                 handle->open_data.name,
                                 ICLI_THREAD_PRIORITY_HIGH,
                                 icli_session_engine,
                                 sid);
        break;

    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
        rc = icli_thread_create( sid,
                                 handle->open_data.name,
                                 ICLI_THREAD_PRIORITY_NORMAL,
                                 icli_session_engine,
                                 sid);
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
        /* null the callback to avoid output */
        handle->open_data.char_put = NULL;
        handle->open_data.str_put  = NULL;

        // fall through

    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
        rc = ICLI_RC_OK;
        break;

    default:
        T_E("invalid session type %d\n", handle->open_data.way);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( rc != ICLI_RC_OK ) {
        T_E("fail to create thread\n");
        _session_free( sid );
        return rc;
    }

    *session_id = (u32)sid;
    return ICLI_RC_OK;

}// icli_session_open_x

/*
    close a ICLI session

    INPUT
        session_id : session ID from icli_session_open_x()

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_session_close_x(
    IN u32  session_id
)
{
    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* create thread accordingly */
    switch ( g_session_handle[session_id].open_data.way ) {
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_SSH:
        if ( g_session_handle[session_id].runtime_data.alive ) {
            g_session_handle[session_id].runtime_data.alive = FALSE;
            break;
        }

        /* fall through */

    case ICLI_SESSION_WAY_THREAD_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_TELNET:
    case ICLI_SESSION_WAY_THREAD_SSH:
    case ICLI_SESSION_WAY_APP_EXEC:
    default:
        _session_free( session_id );
        break;
    }
    return ICLI_RC_OK;
}

/*
    get session handle according to session ID

    INPUT
        session_id : the session ID

    OUTPUT
        n/a

    RETURN
        not NULL : icli_session_handle_t *
        NULL     : failed to get

    COMMENT
        n/a
*/
icli_session_handle_t *icli_session_handle_get(
    IN u32  session_id
)
{
    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return NULL;
    }

    return &(g_session_handle[session_id]);
}

/*
    get if the session is alive or not

    INPUT
        session_id : the session ID

    OUTPUT
        n/a

    RETURN
        TRUE  : alive
        FALSE : dead

    COMMENT
        n/a
*/
BOOL icli_session_alive(
    IN u32  session_id
)
{
    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return FALSE;
    }

    if ( g_session_handle[session_id].runtime_data.alive ) {
        return TRUE;
    } else {
        return FALSE;
    }
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
u32 icli_session_max_get_x(void)
{
    return g_max_sessions;
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
i32 icli_session_max_set_x(
    IN u32 max_sessions
)
{
    u32     i;

    if ( max_sessions < 1 || max_sessions > ICLI_SESSION_CNT ) {
        T_E("invalid max_sessions = %d\n", max_sessions);
        return ICLI_RC_ERR_PARAMETER;
    }

    // close sessions
    if ( max_sessions < g_max_sessions ) {
        for (i = max_sessions; i < g_max_sessions; i++) {
            (void)icli_session_close_x(i);
        }
    }

    g_max_sessions = max_sessions;
    return ICLI_RC_OK;
}

/*
    make session entering the mode

    INPUT
        session_id : the session ID
        mode       : mode entering
        cmd_var    : the variables for the mode

    OUTPUT
        n/a

    RETURN
        >= 0 : successful and the return value is mode level
        -1   : failed

    COMMENT
        n/a
*/
i32 icli_session_mode_enter_x(
    IN  u32                 session_id,
    IN  icli_cmd_mode_t     mode
)
{
    icli_session_handle_t   *handle;
    icli_mode_para_t        *mode_para;

    /* parameter checking */
    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return -1;
    }

    if ( mode >= ICLI_CMD_MODE_MAX ) {
        T_E("invalid mode = %d\n", mode);
        return -1;
    }

    /* get session handle */
    handle = &g_session_handle[session_id];

    /* check level */
    if ( handle->runtime_data.mode_level >= (ICLI_MODE_MAX_LEVEL - 1) ) {
        return -1;
    }

    /* increase 1 level */
    handle->runtime_data.mode_level++;

    /* pack mode */
    mode_para = &( handle->runtime_data.mode_para[handle->runtime_data.mode_level] );
    mode_para->mode    = mode;
    mode_para->cmd_var = handle->runtime_data.cmd_var;

    /* to avoid free after command is over */
    handle->runtime_data.cmd_var = NULL;

    return( handle->runtime_data.mode_level );
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
i32 icli_session_mode_exit_x(
    IN  u32     session_id
)
{
    icli_session_handle_t   *handle;
    icli_mode_para_t        *mode_para;

    /* parameter checking */
    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get session handle */
    handle = &g_session_handle[session_id];

    if ( handle->runtime_data.mode_level == 0 ) {
        return 0;
    }

    /* pack mode */
    mode_para = &( handle->runtime_data.mode_para[handle->runtime_data.mode_level] );
    icli_exec_para_list_free( &(mode_para->cmd_var) );

    /* less 1 level */
    handle->runtime_data.mode_level--;

    return( handle->runtime_data.mode_level );
}

/*
    put string buffer to a session

    INPUT
        session_id : the session ID
        format     : output format
        arglist    : argument value list

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        none
*/
i32 icli_session_va_str_put(
    IN u32          session_id,
    IN const char   *format,
    IN va_list      arglist
)
{
    icli_session_handle_t   *handle;
    int                     r;
    char                    *str_buf;
    u32                     len;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = &(g_session_handle[session_id]);
    if ( handle->runtime_data.alive == FALSE ) {
        return ICLI_RC_ERR_PARAMETER;
    }

    str_buf = g_session_handle[session_id].runtime_data.str_buf;
    len = icli_str_len( str_buf );
    r = icli_vsnprintf(str_buf + len, ICLI_PUT_MAX_LEN - len, format, arglist);
    if ( r < 0 ) {
        T_E("fail to format the output string\n");
        return ICLI_RC_ERROR;
    }

    (void)_session_str_put( handle );
    return ICLI_RC_OK;
}

#define _MIN_STR_SIZE   1
#define _BACKWARD_CHAR \
    if ( pos ) { \
        ICLI_PUT_BACKSPACE; \
        ICLI_PUT_SPACE; \
        ICLI_PUT_BACKSPACE; \
        pos--; \
        str[pos] = 0; \
    } else {\
        ICLI_PLAY_BELL;\
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
i32 icli_session_usr_str_get_x(
    IN      u32                     session_id,
    IN      icli_usr_input_type_t   input_type,
    OUT     char                    *str,
    INOUT   i32                     *str_size,
    OUT     i32                     *end_key
)
{
    icli_session_handle_t   *handle;
    i32                     c;
    i32                     pos = 0;
    i32                     loop;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( input_type >= ICLI_USR_INPUT_TYPE_MAX ) {
        T_E("invalid input_type = %d\n", input_type);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( str == NULL ) {
        T_E("invalid str == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( str_size == NULL ) {
        T_E("invalid str_size == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( *str_size <= _MIN_STR_SIZE ) {
        T_E("invalid invalid *str_size = %d, should be > %d\n", *str_size, _MIN_STR_SIZE);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get handle */
    handle = &g_session_handle[ session_id ];

    /* check alive or not */
    if ( handle->runtime_data.alive == FALSE ) {
        T_E("session_id = %d is not alive\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    loop = 1;
    while ( loop == 1 ) {

        /* get char */
        switch ( input_type ) {
        case ICLI_USR_INPUT_TYPE_NORMAL:
        default:
            if ( icli_sutil_usr_char_get_by_session(handle, &c) == FALSE ) {
                handle->runtime_data.alive = FALSE;
                return ICLI_RC_ERR_EXPIRED;
            }
            break;

        case ICLI_USR_INPUT_TYPE_PASSWORD:
            /* follow c, the timer is for password only, not for session.
               it just displayed time out and not logout */
            if ( icli_sutil_usr_char_get(handle, ICLI_ENABLE_PASSWORD_WAIT, &c) == FALSE ) {
                return ICLI_RC_ERR_EXPIRED;
            }
            break;
        }

        /*
            avoid the cases of CR+LF or LF+CR
        */
        switch (c) {
        case ICLI_KEY_ENTER:
            if ( handle->runtime_data.prev_char == ICLI_NEWLINE ) {
                // reset previous input char
                handle->runtime_data.prev_char = 0;
                continue;
            }
            break;

        case ICLI_NEWLINE:
            if ( handle->runtime_data.prev_char == ICLI_KEY_ENTER ) {
                // reset previous input char
                handle->runtime_data.prev_char = 0;
                continue;
            }
            break;
        }

        /* get previous input char */
        handle->runtime_data.prev_char = c;

        /* process input char */
        switch (c) {
        case ICLI_KEY_FUNCTION0:
            /*
                function key is not supported
                consume next char
            */
            (void)icli_sutil_usr_char_get(handle, _FKEY_WAIT_TIME, &c);
            continue;

        case ICLI_KEY_FUNCTION1:
            if ( icli_sutil_usr_char_get(handle, _FKEY_WAIT_TIME, &c) == FALSE ) {
                /* invalid function key, skip it */
                continue;
            }
            switch ( c ) {
            case ICLI_FKEY1_LEFT:
                _BACKWARD_CHAR;
                break;

            default:
                break;
            }
            continue;

        case ICLI_HYPER_TERMINAL:

            /* get HT_BEGIN */
            if ( icli_sutil_usr_char_get(handle, _FKEY_WAIT_TIME, &c) == FALSE ) {
                continue;
            }

            if ( ICLI_NOT_(HT_BEGIN, c) ) {
                continue;
            }

            /* get VT100 keys */
            if ( icli_sutil_usr_char_get(handle, _FKEY_WAIT_TIME, &c) == FALSE ) {
                continue;
            }

            switch ( c ) {
            case ICLI_VKEY_LEFT:
                _BACKWARD_CHAR;
                break;

            default:
                break;
            }
            continue;

        case ICLI_KEY_BACKSPACE:
            /* case ICLI_KEY_CTRL_H: the same */
            _BACKWARD_CHAR;
            continue;

        case ICLI_KEY_ENTER:
        case ICLI_NEWLINE:
            /*
                windows CMD telnet will come ENTER + NEW_LINE
                so, consume NEW_LINE.

                Bug: if copy-paste, then the first char will always be consumed.
                     so, this is not good.
            */
            //icli_sutil_usr_char_get(handle, _BUF_WAIT_TIME, &c);

            // fall through

        case ICLI_KEY_CTRL_('C'):
            ICLI_PUT_NEWLINE;
            str[pos] = 0;
            *str_size = pos;
            if ( end_key ) {
                *end_key = c;
            }
            return ICLI_RC_OK;

        default :
            break;
        }

        /* not supported keys, skip */
        if ( ! ICLI_KEY_VISIBLE(c) ) {
            continue;
        }

        /* skip SPACE at the begin, otherwise, put into cmd */
        if ( pos == (*str_size - 1) ) {
            continue;
        }
        str[pos] = (char)c;
        pos++;
        if ( input_type == ICLI_USR_INPUT_TYPE_PASSWORD ) {
            c = '*';
        }
        icli_session_char_put(handle, (char)c);

    }/* while (1) */

    return ICLI_RC_ERROR;

}/* icli_session_usr_str_get_x */

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
i32 icli_session_ctrl_c_get_x(
    IN  u32     session_id,
    IN  u32     wait_time
)
{
    icli_session_handle_t   *handle;
    i32                     c;
    u32                     t;

    if ( session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( wait_time == 0 || wait_time > ICLI_MAX_INT) {
        T_E("invalid wait_time = %u\n", wait_time);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get handle */
    handle = &( g_session_handle[ session_id ] );

    /* check alive or not */
    if ( handle->runtime_data.alive == FALSE ) {
        T_E("session_id = %d is not alive\n", session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* get Ctrl-C */
    while (wait_time > 0) {
        /* current time */
        t = icli_current_time_get();

        /* wait Ctrl-C */
        if ( icli_sutil_usr_char_get(handle, (i32)wait_time, &c) == FALSE ) {
            return ICLI_RC_ERR_EXPIRED;
        }

        /* check if Ctrl-C */
        if ( c == ICLI_KEY_CTRL_('C') ) {
            return ICLI_RC_OK;
        }

        /* elapse time */
        t = icli_current_time_get() - t;

        /* update wait_time */
        if ( t < wait_time ) {
            wait_time = wait_time - t;
        } else {
            return ICLI_RC_ERR_EXPIRED;
        }
    }
    return ICLI_RC_ERR_EXPIRED;
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
i32 icli_session_data_get_x(
    INOUT icli_session_data_t   *data
)
{
    icli_session_handle_t   *handle;

    if ( data == NULL ) {
        T_E("data == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( data->session_id >= ICLI_SESSION_CNT ) {
        T_E("invalid session_id = %d\n", data->session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    handle = &( g_session_handle[ data->session_id ] );

    data->alive          = handle->runtime_data.alive;
    data->mode           = handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode;
    data->way            = handle->open_data.way;
    data->connect_time   = handle->runtime_data.connect_time;

    if ( handle->runtime_data.alive ) {
        data->elapsed_time = (icli_current_time_get() - handle->runtime_data.connect_time) / 1000;
    } else {
        data->elapsed_time = 0;
    }

    if ( handle->runtime_data.alive ) {
        data->idle_time = (icli_current_time_get() - handle->runtime_data.idle_time) / 1000;
    } else {
        data->idle_time = 0;
    }

    data->privilege      = handle->runtime_data.privilege;
    data->width          = handle->config_data->width;
    data->lines          = handle->config_data->lines;
    data->wait_time      = handle->config_data->wait_time;
    data->line_mode      = handle->runtime_data.line_mode;
    data->input_style    = handle->config_data->input_style;

#if 1 /* CP, 2012/08/29 09:25, history max count is configurable */
    data->history_size   = ICLI_HISTORY_MAX_CNT;
#endif

#if 1 /* CP, 2012/08/31 07:51, enable/disable banner per line */
    data->b_exec_banner  = handle->config_data->b_exec_banner;
    data->b_motd_banner  = handle->config_data->b_motd_banner;
#endif

#if 1 /* CP, 2012/08/31 09:31, location and default privilege */
    (void)icli_str_ncpy(data->location, handle->config_data->location, ICLI_LOCATION_MAX_LEN);
    data->privileged_level = handle->config_data->privileged_level;
#endif

#if 1 /* CP, 2012/09/04 16:46, session user name */
    (void)icli_str_ncpy(data->user_name, handle->runtime_data.user_name, ICLI_USERNAME_MAX_LEN);

    data->client_ip   = handle->open_data.client_ip;
    data->client_port = handle->open_data.client_port;
#endif

    return ICLI_RC_OK;
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
i32 icli_session_data_get_next_x(
    INOUT icli_session_data_t   *data
)
{
    if ( data == NULL ) {
        T_E("data == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( data->session_id >= (ICLI_SESSION_CNT - 1) ) {
        T_E("invalid session_id = %d\n", data->session_id);
        return ICLI_RC_ERR_PARAMETER;
    }

    /* set to next session */
    data->session_id += 1;

    /* get session data */
    return icli_session_data_get_x( data );
}

/*
    put one char directly to the session
    no line checking
    no buffering

    INPUT
        session_id : the session ID
        c          : the char for put

    OUTPUT
        n/a

    RETURN
        n/a
*/
void icli_session_char_put(
    IN  icli_session_handle_t   *handle,
    IN  char                    c
)
{
    u32     len;

    if ( handle->runtime_data.alive == FALSE ) {
        T_E("session_id = %d is not alive\n", handle->session_id);
        return;
    }

    switch ( handle->runtime_data.err_display_mode ) {
    case ICLI_ERR_DISPLAY_MODE_PRINT :
        (void)icli_sutil_usr_char_put(handle, c);
        break;

    case ICLI_ERR_DISPLAY_MODE_ERR_BUFFER :
        len = icli_str_len( handle->runtime_data.err_buf );
        if ( len >= ICLI_ERR_MAX_LEN ) {
            T_E("session_id = %d, error buffer is full\n", handle->session_id);
            return;
        }
        handle->runtime_data.err_buf[len] = c;
        handle->runtime_data.err_buf[len + 1] = ICLI_EOS;
        break;

    case ICLI_ERR_DISPLAY_MODE_DROP :
    default :
        break;
    }
}

/*
    put string directly to the session
    no line checking
    no buffering

    INPUT
        handle : the session handle
        str    : output string

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        none
*/
void icli_session_str_put(
    IN  icli_session_handle_t   *handle,
    IN  char                    *str
)
{
    u32     len;
    u32     str_len;

    if ( handle->runtime_data.alive == FALSE ) {
        T_E("session_id = %d is not alive\n", handle->session_id);
        return;
    }

    switch ( handle->runtime_data.err_display_mode ) {
    case ICLI_ERR_DISPLAY_MODE_PRINT :
        handle->runtime_data.put_str = str;
        (void)icli_sutil_usr_str_put( handle );
        handle->runtime_data.put_str = NULL;
        break;

    case ICLI_ERR_DISPLAY_MODE_ERR_BUFFER :
        len = icli_str_len( handle->runtime_data.err_buf );
        if ( len >= ICLI_ERR_MAX_LEN ) {
            T_E("session_id = %d, error buffer is full\n", handle->session_id);
            return;
        }
        str_len = icli_str_len( str );
        if ( (len + str_len) >= ICLI_ERR_MAX_LEN ) {
            T_E("session_id = %d, str is too long to error buffer\n", handle->session_id);
            T_E("str = %s\n", str);
            return;
        }
        // concate str in err_buf
        if ( icli_str_concat(handle->runtime_data.err_buf, str) == NULL ) {
            T_E("session_id = %d, fail to concate str in error buffer\n", handle->session_id);
        }
        break;

    case ICLI_ERR_DISPLAY_MODE_DROP :
    default :
        break;
    }
}

/*
    put string directly to the session
    no line checking
    no buffering

    INPUT
        handle : the session handle
        format : string format
        ...    : parameters of format

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_session_str_printf(
    IN  icli_session_handle_t   *handle,
    IN  const char              *format,
    IN  ...
)
{
    va_list     arglist = NULL;
    int         r;
    u32         len;
    u32         str_len;
    char        *str;

    if ( handle->runtime_data.alive == FALSE ) {
        T_E("session_id = %d is not alive\n", handle->session_id);
        return;
    }

    switch ( handle->runtime_data.err_display_mode ) {
    case ICLI_ERR_DISPLAY_MODE_PRINT :
        // format string buffer
        va_start( arglist, format );
        r = icli_vsnprintf(handle->runtime_data.str_buf, ICLI_PUT_MAX_LEN, format, arglist);
        va_end( arglist );

        // check result
        if ( r < 0 ) {
            T_E("fail to format the output string\n");
            return;
        }

        // set put_str
        handle->runtime_data.put_str = handle->runtime_data.str_buf;

        // output the string
        (void)icli_sutil_usr_str_put( handle );

        // reset
        handle->runtime_data.str_buf[0] = 0;
        handle->runtime_data.put_str = NULL;
        break;

    case ICLI_ERR_DISPLAY_MODE_ERR_BUFFER :
        // check length of error buffer
        len = icli_str_len( handle->runtime_data.err_buf );
        if ( len >= ICLI_ERR_MAX_LEN ) {
            T_E("session_id = %d, error buffer is full\n", handle->session_id);
            return;
        }

        // format string buffer
        va_start( arglist, format );
        r = icli_vsnprintf(handle->runtime_data.str_buf, ICLI_PUT_MAX_LEN, format, arglist);
        va_end( arglist );

        // check result
        if ( r < 0 ) {
            T_E("fail to format the output string\n");
            return;
        }

        str = handle->runtime_data.str_buf;
        str_len = icli_str_len( str );
        if ( (len + str_len) >= ICLI_ERR_MAX_LEN ) {
            T_E("session_id = %d, str is too long to error buffer\n", handle->session_id);
            T_E("str = %s\n", str);
            return;
        }
        // concate str in err_buf
        if ( icli_str_concat(handle->runtime_data.err_buf, str) == NULL ) {
            T_E("session_id = %d, fail to concate str in error buffer\n", handle->session_id);
        }

        // reset
        handle->runtime_data.str_buf[0] = 0;
        break;

    case ICLI_ERR_DISPLAY_MODE_DROP :
    default :
        break;
    }
}

/*
    put error message to a temp buffer(cmd_copy) for pending display

    INPUT
        handle : the session handle
        format : string format
        ...    : parameters of format

    OUTPUT
        n/a

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_session_error_printf(
    IN  icli_session_handle_t   *handle,
    IN  const char              *format,
    IN  ...
)
{
    va_list     arglist = NULL;
    int         r;

    if ( handle->runtime_data.alive == FALSE ) {
        T_E("session_id = %d is not alive\n", handle->session_id);
        return FALSE;
    }

    // format string buffer
    va_start( arglist, format );

    r = icli_vsnprintf(handle->runtime_data.cmd_copy, ICLI_STR_MAX_LEN, format, arglist);

    va_end( arglist );

    // check result
    if ( r < 0 ) {
        T_E("fail to format the output string\n");
        return FALSE;
    }

    // raise flag
    handle->runtime_data.more_print = TRUE;
    return TRUE;
}

#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
/*
    get session ID by thread ID

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
u32 icli_session_self_id_x(
    void
)
{
    u32     i;
    u32     thread_id = icli_thread_id_get();

    for ( i = 0; i < ICLI_SESSION_CNT; i++ ) {
        if ( g_session_handle[i].runtime_data.alive ) {
            if ( g_session_handle[i].runtime_data.thread_id == thread_id ) {
                return i;
            }
        }
    }
    return ICLI_SESSION_CNT;
}
#endif

/*
    reset session config data to default

    INPUT
        config_data - session config data to reset default

    OUTPUT
        n/a

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_session_config_data_default(
    IN icli_session_config_data_t   *config_data
)
{
    if ( config_data == NULL ) {
        T_E("config_data == NULL\n");
        return FALSE;
    }

    /* set to default */
    config_data->input_style      = icli_input_style_get_x();
    config_data->width            = ICLI_DEFAULT_WIDTH;
    config_data->lines            = ICLI_DEFAULT_LINES;
    config_data->wait_time        = ICLI_DEFAULT_WAIT_TIME;
    config_data->history_size     = ICLI_HISTORY_CMD_CNT;
    config_data->b_exec_banner    = TRUE;
    config_data->b_motd_banner    = TRUE;
    config_data->location[0]      = 0;
    config_data->privileged_level = ICLI_DEFAULT_PRIVILEGED_LEVEL;

    return TRUE;
}
