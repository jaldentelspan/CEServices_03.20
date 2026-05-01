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
    > CP.Wang, 2011/05/02 10:53
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
******************************************************************************

    Constant and Macro

******************************************************************************
*/
#define _PARAMETER_CHECK(x)\
    if ( (x) == NULL ) {\
        T_E(#x" == NULL\n");\
        return ICLI_RC_ERR_PARAMETER;\
    }

#define _INIT_CHECK()\
    if ( g_is_init == FALSE ) {\
        T_E("not initialized yet\n");\
        return ICLI_RC_ERROR;\
    }

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
static BOOL                 g_is_init = FALSE;
static icli_init_data_t     g_init_data;

#if 1 /* CP, 2012/09/14 10:38, conf */

static icli_conf_data_t     g_conf_data;

#else /* CP, 2012/09/14 10:38, conf */

/* this must be initialized one by one mapping even empty string */
static char     g_mode_name[ICLI_CMD_MODE_MAX][ICLI_PROMPT_MAX_LEN + 1];

/* device name */
static char     g_dev_name[ICLI_NAME_MAX_LEN + 1];

/* banner */
static char     g_banner_login[ICLI_BANNER_MAX_LEN + 1];
static char     g_banner_motd[ICLI_BANNER_MAX_LEN + 1];
static char     g_banner_exec[ICLI_BANNER_MAX_LEN + 1];

/* enable password for each privilege */
static char     g_enable_password[ICLI_PRIVILEGE_MAX][ICLI_PASSWORD_MAX_LEN + 1];

/* TRUE: secret password, FALSE: clear password */
static BOOL     g_b_enable_secret[ICLI_PRIVILEGE_MAX];

#endif /* CP, 2012/09/14 10:38, conf */

static u8       g_enable_key[] = {0x77, 0x89, 0x51, 0x03, 0x42, 0x90, 0x68, 0x00, 0x14, 0x97, 0x88};
static size_t   g_enable_key_len = 11;

/* port config */
static icli_stack_port_range_t      g_port_range;

#if 1 /* CP, 2012/10/08 14:31, debug command, debug prompt */
static char     g_debug_prompt[ICLI_NAME_MAX_LEN + 1];
#endif

/*
******************************************************************************

    Static Function

******************************************************************************
*/
/*
    check if port in (begin_port, port_cnt)
*/
static BOOL _port_in_range(
    IN u32      port,
    IN u32      begin_port,
    IN u32      port_cnt
)
{
    if ( port < begin_port ) {
        return FALSE;
    }

    if ( port > (begin_port + port_cnt - 1) ) {
        return FALSE;
    }

    return TRUE;
}

/*
    port compare
    INDEX
        switch_range->port_type  : port type
        switch_range->switch_id  : switch ID
        switch_range->begin_port : port ID

    RETURN
        1 : a > b
        0 > a = b
       -1 : a < b
*/
static i32 _port_compare(
    IN icli_switch_port_range_t  *a,
    IN icli_switch_port_range_t  *b
)
{
    if ( a->port_type > b->port_type ) {
        return 1;
    }
    if ( a->port_type < b->port_type ) {
        return -1;
    }

    if ( a->switch_id > b->switch_id ) {
        return 1;
    }
    if ( a->switch_id < b->switch_id ) {
        return -1;
    }

    if ( a->begin_port > b->begin_port ) {
        return 1;
    }
    if ( a->begin_port < b->begin_port ) {
        return -1;
    }

    return 0;
}

static char _hex_to_char(
    IN u8 hex
)
{
    if ( hex < 10 ) {
        return '0' + hex;
    } else {
        return 'A' + hex - 10;
    }
}

static void _mac_to_string(
    IN  u8      *mac,
    OUT char    *str
)
{
    u32 i;

    // translate into string
    for ( i = 0; i < 16; i++ ) {
        str[2 * i]   = _hex_to_char( mac[i] & 0x0F );
        str[2 * i + 1] = _hex_to_char( (mac[i] & 0xF0) >> 4 );
    }
    str[ICLI_PASSWORD_MAX_LEN] = ICLI_EOS;
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
i32 icli_engine_init(
    IN  icli_init_data_t     *init_data
)
{
    if ( init_data == NULL ) {
        T_E("init_data == NULL\n");
        return ICLI_RC_ERROR;
    }

    if ( g_is_init ) {
        T_E("initialized already\n");
        return ICLI_RC_ERROR;
    }

#if 1 /* CP, 2012/09/14 10:38, conf */

    memset(&g_conf_data, 0, sizeof(g_conf_data));

#else /* CP, 2012/09/14 10:38, conf */
    memset(g_mode_name,       0, sizeof(g_mode_name));
    memset(g_dev_name,        0, sizeof(g_dev_name));
    memset(g_banner_login,    0, sizeof(g_banner_login));
    memset(g_banner_motd,     0, sizeof(g_banner_motd));
    memset(g_banner_exec,     0, sizeof(g_banner_exec));

    memset(g_enable_password, 0, sizeof(g_enable_password));
    memset(g_b_enable_secret, 0, sizeof(g_b_enable_secret));

    /* default enable password */
    (void)icli_str_cpy(g_enable_password[ICLI_PRIVILEGE_DEBUG - 1], ICLI_DEFAULT_ENABLE_PASSWORD);
#endif

    /* reset port range */
    icli_port_range_reset_x();

#if 1 /* CP, 2012/10/08 14:31, debug command, debug prompt */
    memset(g_debug_prompt, 0, sizeof(g_debug_prompt));
#endif

    _PARAMETER_CHECK( init_data );
    _PARAMETER_CHECK( init_data->thread_create_cb );
    //_PARAMETER_CHECK( init_data->malloc_cb );
    //_PARAMETER_CHECK( init_data->free_cb );
    _PARAMETER_CHECK( init_data->sema_take_cb );
    _PARAMETER_CHECK( init_data->sema_give_cb );
    _PARAMETER_CHECK( init_data->current_time_get_cb );
    _PARAMETER_CHECK( init_data->init_wait_cb );
    //_PARAMETER_CHECK( init_data->user_auth_cb );

    /* get init data */
    memcpy( &g_init_data, init_data, sizeof(icli_init_data_t) );

    /* init parsing */
    if ( icli_parsing_init() ) {
        T_E("fail to init parsing\n");
        return ICLI_RC_ERROR;
    }

    /* init register */
    if ( icli_register_init() ) {
        T_E("fail to init register\n");
        return ICLI_RC_ERROR;
    }

    /* init session */
    if ( icli_session_init() ) {
        T_E("fail to init session\n");
        return ICLI_RC_ERROR;
    }

    /* init execution */
    if ( icli_exec_init() ) {
        T_E("fail to init execution\n");
        return ICLI_RC_ERROR;
    }

    g_is_init = TRUE;
    return ICLI_RC_OK;
}

/*
    get if ICLI is initialized

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        TRUE  : initialized
        FALSE : not yet

    COMMENT
        n/a
*/
BOOL icli_is_init(void)
{
    return g_is_init;
}

#if 1 /* CP, 2012/09/14 10:38, conf */
/*
    get address of conf data
    this is for internal use only

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        address of conf data

    COMMENT
        n/a
*/
icli_conf_data_t *icli_conf_get_x(void)
{
    return &g_conf_data;
}
#endif

/*
    create thread

    INPUT
        session_id : session ID, mainly for stack memory in icli_porting.c
        name       : thread name
        priority   : thread priority
        entry_cb   : function entry of the thread
        entry_data : data for the input paramter of entry_cb

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_thread_create(
    IN  i32                     session_id,
    IN  char                    *name,
    IN  icli_thread_priority_t  priority,
    IN  icli_thread_entry_cb_t  entry_cb,
    IN  i32                     entry_data
)
{
    _INIT_CHECK();

    _PARAMETER_CHECK( name );
    _PARAMETER_CHECK( entry_cb );

    return g_init_data.thread_create_cb(session_id, name, priority, entry_cb, entry_data);
}

/*
    memory allocation

    INPUT
        size : memory size for allocation

    OUTPUT
        n/a

    RETURN
        not NULL : successful
        NULL     : failed

    COMMENT
        n/a
*/
void *icli_malloc(
    IN  u32     size
)
{
    if ( g_is_init == FALSE ) {
        T_E("not initialized yet\n");
        return NULL;
    }

    if ( size == 0 ) {
        return NULL;
    }

    if ( g_init_data.malloc_cb == NULL ) {
        return NULL;
    }

    return g_init_data.malloc_cb( size );
}

/*
    memory free

    INPUT
        mem : memory to be freed

    OUTPUT
        n/a

    RETURN
        n/a
*/
void icli_free(
    IN void     *mem
)
{
    if ( g_is_init == FALSE ) {
        T_E("not initialized yet\n");
        return;
    }

    if ( g_init_data.free_cb == NULL ) {
        return;
    }

    if ( mem ) {
        g_init_data.free_cb( mem );
    }
}

/*
    take semaphore
*/
i32 icli_sema_take(void)
{
    _INIT_CHECK();

    if ( g_init_data.sema_take_cb() == ICLI_RC_ERROR ) {
        T_E("fail to take\n");
        return ICLI_RC_ERR_SEMA;
    }
    return ICLI_RC_OK;
}

/*
    give semaphore
*/
i32 icli_sema_give(void)
{
    _INIT_CHECK();
    if ( g_init_data.sema_give_cb() == ICLI_RC_ERROR ) {
        T_E("fail to give\n");
        return ICLI_RC_ERR_SEMA;
    }
    return ICLI_RC_OK;
}

/*
    get the time elapsed from system start in milliseconds
*/
u32 icli_current_time_get(void)
{
    if ( g_is_init == FALSE ) {
        T_E("not initialized yet\n");
        return 0;
    }
    return g_init_data.current_time_get_cb();
}

/*
    wait for other module init at the begin of ICLI session
*/
i32 icli_init_wait(void)
{
    _INIT_CHECK();

    if ( g_init_data.init_wait_cb ) {
        return g_init_data.init_wait_cb();
    }

    return ICLI_RC_OK;
}

/*
    check if auth callback exists

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        TRUE  - yes, the callback exists
        FALSE - no, it is NULL

*/
BOOL icli_has_user_auth(void)
{
    if ( g_init_data.user_auth_cb ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
    authenticate user

    INPUT
        session_way  : way to access session
        username     : user name
        password     : password for the user

    OUTPUT
        privilege    : the privilege level for the authenticated user

    RETURN
        icli_rc_t
*/
i32 icli_user_auth(
    IN  icli_session_way_t  session_way,
    IN  char                *username,
    IN  char                *password,
    OUT i32                 *privilege
)
{
    _INIT_CHECK();

    if ( session_way >= ICLI_SESSION_WAY_MAX ) {
        T_E("invalid session_way = %d\n", session_way);
        return ICLI_RC_ERR_PARAMETER;
    }

    _PARAMETER_CHECK( username );
    _PARAMETER_CHECK( password );
    _PARAMETER_CHECK( privilege );

    if ( g_init_data.user_auth_cb ) {
        return g_init_data.user_auth_cb(session_way, username, password, privilege);
    }

    //NULL, then OK always
    return ICLI_RC_OK;
}

/*
    sleep for milli-seconds

    INPUT
        t : milli-seconds for sleep

    OUTPUT
        n/a

    RETURN
        n/a
*/
void icli_sleep(
    IN u32  t
)
{
    if ( g_is_init == FALSE ) {
        T_E("not initialized yet\n");
        return;
    }

    if ( g_init_data.sleep_cb ) {
        g_init_data.sleep_cb( t );
    }
}

#if 1 /* CP, 2012/08/31 17:00, enable secret */
void icli_hmac_md5(
    IN  const u8    *key,
    IN  size_t      key_len,
    IN  const u8    *data,
    IN  size_t      data_len,
    OUT u8          *mac
)
{
    if ( g_is_init == FALSE ) {
        T_E("not initialized yet\n");
        return;
    }

    if ( g_init_data.hmac_md5_cb ) {
        g_init_data.hmac_md5_cb(key, key_len, data, data_len, mac);
    } else {
        (void)icli_str_ncpy((char *)mac, (char *)data, 15);
    }
}
#endif

#if 1 /* CP, 2012/09/14 10:38, conf */
BOOL icli_conf_write( void )
{
    if ( g_is_init == FALSE ) {
        T_E("not initialized yet\n");
        return FALSE;
    }

    if ( g_init_data.conf_write_cb == NULL ) {
        T_E("no callback to write conf\n");
        return FALSE;
    }
    return g_init_data.conf_write_cb();
}
#endif

#if 1 /* CP, 2012/09/04 13:33, switch information for prompt when stacking */
BOOL icli_switch_info_get(
    OUT icli_switch_info_t  *switch_info
)
{
    if ( g_is_init == FALSE ) {
        T_E("not initialized yet\n");
        return FALSE;
    }

    if ( g_init_data.switch_info_cb == NULL ) {
        return FALSE;
    }

    g_init_data.switch_info_cb( switch_info );
    return TRUE;
}
#endif

#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
u32 icli_thread_id_get( void )
{
    if ( g_is_init == FALSE ) {
        T_E("not initialized yet\n");
        return 0;
    }

    if ( g_init_data.thread_id_get_cb == NULL ) {
        return 0;
    }

    return g_init_data.thread_id_get_cb();
}
#endif

/*
    get whick input_style the ICLI engine perform

    input -
        n/a

    output -
        n/a

    return -
        icli_input_style_t

    comment -
        n/a
*/
icli_input_style_t icli_input_style_get_x(void)
{
    return g_init_data.input_style;
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
i32 icli_input_style_set_x(
    IN icli_input_style_t   input_style
)
{
    if ( input_style >= ICLI_INPUT_STYLE_MAX ) {
        return ICLI_RC_ERR_PARAMETER;
    }
    g_init_data.input_style = input_style;
    return ICLI_RC_OK;
}

/*
    get case sensitive or not

    input -
        n/a

    output -
        n/a

    return -
        1 - yes
        0 - not

    comment -
        n/a
*/
u32 icli_case_sensitive_get(void)
{
    return g_init_data.case_sensitive;
}

/*
    get if console is always alive or not

    input -
        n/a

    output -
        n/a

    return -
        1 - yes
        0 - not

    comment -
        n/a
*/
u32 icli_console_alive_get(void)
{
    return g_init_data.console_alive;
}

/*
    get mode name without semaphore

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        not NULL : successful
        NULL     : failed

    COMMENT
        n/a
*/
char *icli_mode_name_get_x(
    IN  icli_cmd_mode_t     mode
)
{
    if ( mode >= ICLI_CMD_MODE_MAX ) {
        return NULL;
    }

    return g_conf_data.mode_name[ mode ];
}

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
i32 icli_mode_name_set_x(
    IN  icli_cmd_mode_t     mode,
    IN  char                *name
)
{
    if ( mode >= ICLI_CMD_MODE_MAX ) {
        T_E("invalud mode = %d\n", mode);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( name == NULL ) {
        T_E("name == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    (void)icli_str_ncpy(g_conf_data.mode_name[mode], name, ICLI_PROMPT_MAX_LEN);
    g_conf_data.mode_name[mode][ICLI_PROMPT_MAX_LEN] = 0;

    return ICLI_RC_OK;
}

/*
    get device name without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_conf_data.dev_name

    COMMENT
        n/a
*/
char *icli_dev_name_get_x(void)
{
    return g_conf_data.dev_name;
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
i32 icli_dev_name_set_x(
    IN  char    *dev_name
)
{
    if ( dev_name == NULL ) {
        T_E("dev_name == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    (void)icli_str_ncpy(g_conf_data.dev_name, dev_name, ICLI_NAME_MAX_LEN);
    g_conf_data.dev_name[ICLI_NAME_MAX_LEN] = 0;
    return ICLI_RC_OK;
}

/*
    get LOGIN banner without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_conf_data.banner_login

    COMMENT
        n/a
*/
char *icli_banner_login_get_x(void)
{
    return g_conf_data.banner_login;
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
i32 icli_banner_login_set_x(
    IN  char    *banner_login
)
{
    if ( banner_login == NULL ) {
        g_conf_data.banner_login[0] = 0;
    } else {
        (void)icli_str_ncpy(g_conf_data.banner_login, banner_login, ICLI_BANNER_MAX_LEN);
        g_conf_data.banner_login[ICLI_BANNER_MAX_LEN] = 0;
    }
    return ICLI_RC_OK;
}

/*
    get MOTD banner without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_conf_data.banner_motd

    COMMENT
        n/a
*/
char *icli_banner_motd_get_x(void)
{
    return g_conf_data.banner_motd;
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
i32 icli_banner_motd_set_x(
    IN  char    *banner_motd
)
{
    if ( banner_motd == NULL ) {
        g_conf_data.banner_motd[0] = 0;
    } else {
        (void)icli_str_ncpy(g_conf_data.banner_motd, banner_motd, ICLI_BANNER_MAX_LEN);
        g_conf_data.banner_motd[ICLI_BANNER_MAX_LEN] = 0;
    }
    return ICLI_RC_OK;
}

/*
    get EXEC banner without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_conf_data.banner_exec

    COMMENT
        n/a
*/
char *icli_banner_exec_get_x(void)
{
    return g_conf_data.banner_exec;
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
i32 icli_banner_exec_set_x(
    IN  char    *banner_exec
)
{
    if ( banner_exec == NULL ) {
        g_conf_data.banner_exec[0] = 0;
    } else {
        (void)icli_str_ncpy(g_conf_data.banner_exec, banner_exec, ICLI_BANNER_MAX_LEN);
        g_conf_data.banner_exec[ICLI_BANNER_MAX_LEN] = 0;
    }
    return ICLI_RC_OK;
}

/*
    check if the port type is present in this device

    INPUT
        port_type : port type

    OUTPUT
        n/a

    RETURN
        TRUE  : yes, the device has ports belong to this port type
        FALSE : no

    COMMENT
        n/a
*/
BOOL icli_port_type_present_x(
    IN icli_port_type_t     port_type
)
{
    u32     i;

    if ( port_type >= ICLI_PORT_TYPE_MAX ) {
        T_E("invalid port type %d\n", port_type);
        return FALSE;
    }

    for ( i = 0; i < g_port_range.cnt; i++ ) {
        if ( g_port_range.switch_range[i].port_type == port_type ) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
    find the next present type

    INPUT
        port_type : port type

    OUTPUT
        n/a

    RETURN
        others              : next present type
        ICLI_PORT_TYPE_NONE : no next

    COMMENT
        n/a
*/
icli_port_type_t icli_port_next_present_type_x(
    IN icli_port_type_t     port_type
)
{
    /* get next type */
    if ( port_type >= (ICLI_PORT_TYPE_MAX - 1) ) {
        return ICLI_PORT_TYPE_NONE;
    }

    for ( port_type++; port_type < ICLI_PORT_TYPE_MAX; port_type++ ) {
        if ( icli_port_type_present_x(port_type) ) {
            return port_type;
        }
    }
    return ICLI_PORT_TYPE_NONE;
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
void icli_port_range_reset_x( void )
{
    memset( &g_port_range, 0, sizeof(g_port_range) );
}

/*
    get port range

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        &g_port_range

    COMMENT
        n/a
*/
icli_stack_port_range_t *icli_port_range_get_x(
    void
)
{
    return &g_port_range;
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
        CIRTICAL ==> before set, port range must be reset by icli_port_range_reset_x().
        otherwise, the set may be failed because this set will check the
        port definition to avoid duplicate definitions.
*/
BOOL icli_port_range_set_x(
    IN icli_stack_port_range_t  *range
)
{
    u32                         i, j;
    u16                         begin_port;
    u16                         end_port;
    icli_switch_port_range_t    *switch_range;

    if ( range == NULL ) {
        T_E("range == NULL\n");
        return FALSE;
    }

    /* check range */
    for ( i = 0; i < range->cnt; i++ ) {
        /* get switch_range */
        switch_range = &(range->switch_range[i]);

        /* check port type */
        if ( switch_range->port_type >= ICLI_PORT_TYPE_MAX ) {
            T_E("invalid range->switch_range[%d].port_type == %u\n", i, switch_range->port_type);
            return FALSE;
        }

        /* check switch ID */
        if ( switch_range->switch_id == 0 ) {
            T_E("invalid range->switch_range[%d].switch_id == 0\n", i);
            return FALSE;
        }

        /* check port ID */
        if ( switch_range->begin_port == 0 ) {
            T_E("invalid range->switch_range[%d].begin_port == 0\n", i);
            return FALSE;
        }

        /* check port count */
        if ( switch_range->port_cnt == 0 ) {
            T_E("invalid range->switch_range[%d].port_cnt == 0\n", i);
            return FALSE;
        }

        /* check switch port */
        begin_port = switch_range->begin_port;
        end_port   = switch_range->begin_port + switch_range->port_cnt - 1;
        for ( j = 0; j < range->cnt; j++ ) {
            if ( j == i ) {
                continue;
            }
            if ( switch_range->port_type != range->switch_range[j].port_type ) {
                continue;
            }
            if ( switch_range->switch_id != range->switch_range[j].switch_id ) {
                continue;
            }
            if ( _port_in_range( begin_port,
                                 range->switch_range[j].begin_port,
                                 range->switch_range[j].port_cnt ) ) {
                T_E("switch port overlapped between %d and %d\n", i, j);
                return FALSE;
            }
            if ( _port_in_range( end_port,
                                 range->switch_range[j].begin_port,
                                 range->switch_range[j].port_cnt ) ) {
                T_E("switch port overlapped between %d and %d\n", i, j);
                return FALSE;
            }
        }

        /* check uport */
        begin_port = switch_range->begin_uport;
        end_port   = switch_range->begin_uport + switch_range->port_cnt - 1;
        for ( j = 0; j < range->cnt; j++ ) {
            if ( j == i ) {
                continue;
            }
            if ( switch_range->usid != range->switch_range[j].usid ) {
                continue;
            }
            if ( _port_in_range( begin_port,
                                 range->switch_range[j].begin_uport,
                                 range->switch_range[j].port_cnt ) ) {
                T_E("uport overlapped between %d and %d\n", i, j);
                return FALSE;
            }
            if ( _port_in_range( end_port,
                                 range->switch_range[j].begin_uport,
                                 range->switch_range[j].port_cnt ) ) {
                T_E("uport overlapped between %d and %d\n", i, j);
                return FALSE;
            }
        }
    }

    /* set */
    g_port_range = *range;
    return TRUE;
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
BOOL icli_port_from_usid_uport_x(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    u32     i;
    u32     end_uport;

    if ( switch_range == NULL ) {
        T_E("switch_range == NULL\n");
        return FALSE;
    }

    if ( switch_range->port_cnt == 0 ) {
        T_E("port_cnt == 0\n");
        return FALSE;
    }

    end_uport = switch_range->begin_uport + switch_range->port_cnt - 1;
    for ( i = 0; i < g_port_range.cnt; i++ ) {
        if ( g_port_range.switch_range[i].usid != switch_range->usid ) {
            continue;
        }
        if ( _port_in_range( switch_range->begin_uport,
                             g_port_range.switch_range[i].begin_uport,
                             g_port_range.switch_range[i].port_cnt ) == FALSE ) {
            continue;
        }
        if ( switch_range->port_cnt > 1 ) {
            if ( _port_in_range( end_uport,
                                 g_port_range.switch_range[i].begin_uport,
                                 g_port_range.switch_range[i].port_cnt ) == FALSE ) {
                continue;
            }
        }
        /* in range, get data */
        switch_range->port_type  = g_port_range.switch_range[i].port_type;
        switch_range->switch_id  = g_port_range.switch_range[i].switch_id;
        switch_range->begin_port = g_port_range.switch_range[i].begin_port +
                                   switch_range->begin_uport -
                                   g_port_range.switch_range[i].begin_uport;
        return TRUE;
    }

    /* not in range */
    return FALSE;
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
BOOL icli_port_get_first_x(
    OUT icli_switch_port_range_t  *switch_range
)
{
    u32     i;
    u32     min = 0;

    if ( switch_range == NULL ) {
        T_E("switch_range == NULL\n");
        return FALSE;
    }

    for ( i = 1; i < g_port_range.cnt; i++ ) {
        if ( _port_compare(&(g_port_range.switch_range[i]), &(g_port_range.switch_range[min])) == -1 ) {
            min = i;
        }
    }

    *switch_range = g_port_range.switch_range[min];
    return TRUE;
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
BOOL icli_port_get_x(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    u32     i;

    if ( switch_range == NULL ) {
        T_E("switch_range == NULL\n");
        return FALSE;
    }

    for ( i = 0; i < g_port_range.cnt; i++ ) {
        // same port type
        if ( g_port_range.switch_range[i].port_type != switch_range->port_type ) {
            continue;
        }
        if ( g_port_range.switch_range[i].switch_id != switch_range->switch_id ) {
            continue;
        }
        if ( _port_in_range( switch_range->begin_port,
                             g_port_range.switch_range[i].begin_port,
                             g_port_range.switch_range[i].port_cnt ) == FALSE ) {
            continue;
        }

        /* in range, get data */
        switch_range->usid        = g_port_range.switch_range[i].usid;
        switch_range->begin_uport = g_port_range.switch_range[i].begin_uport +
                                    switch_range->begin_port -
                                    g_port_range.switch_range[i].begin_port;
        return TRUE;
    }

    /* not in range */
    return FALSE;
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
BOOL icli_port_get_next_x(
    INOUT icli_switch_port_range_t  *switch_range
)
{
    u32     i;
    u8      port_type;
    u16     switch_id;
    u16     begin_port;

    if ( switch_range == NULL ) {
        T_E("switch_range == NULL\n");
        return FALSE;
    }

    port_type  = switch_range->port_type;
    switch_id  = switch_range->switch_id;
    begin_port = switch_range->begin_port;

    for ( ; port_type < ICLI_PORT_TYPE_MAX; port_type++ ) {
        for ( ; switch_id < 33; switch_id++ ) {
            for ( begin_port++; begin_port < 65; begin_port++ ) {
                for ( i = 0; i < g_port_range.cnt; i++ ) {
                    // same port type
                    if ( g_port_range.switch_range[i].port_type != port_type ) {
                        continue;
                    }
                    // same switch ID
                    if ( g_port_range.switch_range[i].switch_id != switch_id ) {
                        continue;
                    }
                    if ( _port_in_range( begin_port,
                                         g_port_range.switch_range[i].begin_port,
                                         g_port_range.switch_range[i].port_cnt ) ) {
                        switch_range->port_type   = g_port_range.switch_range[i].port_type;
                        switch_range->switch_id   = g_port_range.switch_range[i].switch_id;
                        switch_range->begin_port  = begin_port;
                        switch_range->port_cnt    = 1;
                        switch_range->usid        = g_port_range.switch_range[i].usid;
                        switch_range->begin_uport = g_port_range.switch_range[i].begin_uport +
                                                    begin_port -
                                                    g_port_range.switch_range[i].begin_port;
                        return TRUE;
                    }
                }
            }
            begin_port = 0;
        }
        switch_id = 0;
    }
    return FALSE;
}

/*
    check if the port is of the specific type

    INPUT
        switch_id : switch ID
        port_id   : port ID
        port_type : port type

    OUTPUT
        usid  : if valid, then give the corresponding usid
        uport : if valid, then give the corresponding uport

    RETURN
        TRUE  : yes, it is
        FALSE : no, it is not

    COMMENT
        n/a
*/
BOOL icli_port_check_type(
    IN  u16                 switch_id,
    IN  u16                 port_id,
    IN  icli_port_type_t    port_type,
    IN  u16                 *usid,
    IN  u16                 *uport
)
{
    u32     i;

    if ( port_type >= ICLI_PORT_TYPE_MAX ) {
        T_E("invalid port type %d\n", port_type);
        return FALSE;
    }

    for ( i = 0; i < g_port_range.cnt; i++ ) {
        if ( port_type != g_port_range.switch_range[i].port_type ) {
            continue;
        }
        if ( switch_id != g_port_range.switch_range[i].switch_id ) {
            continue;
        }
        if ( _port_in_range( port_id,
                             g_port_range.switch_range[i].begin_port,
                             g_port_range.switch_range[i].port_cnt) ) {
            /* get usid */
            if ( usid ) {
                *usid = g_port_range.switch_range[i].usid;
            }
            /* get uport */
            if ( uport ) {
                *uport = port_id - g_port_range.switch_range[i].begin_port + g_port_range.switch_range[i].begin_uport;
            }
            return TRUE;
        }
    }

    /* not in range */
    return FALSE;
}

/*
    check if all the port range belong a specific port type

    INPUT
        range     : port range to check
        port_type : the specific port type

    OUTPUT
        range->switch_range->port_type   : if valid, then set the correct value
        range->switch_range->begin_uport : if valid, then set the correct value

        switch_id, port_id: if invalid, then this is the invalid pair

    RETURN
        TRUE  : yes, it does
        FALSE : no

    COMMENT
        n/a
*/
BOOL icli_port_range_check_type(
    INOUT icli_stack_port_range_t   *range,
    IN    icli_port_type_t          port_type,
    OUT   u16                       *switch_id,
    OUT   u16                       *port_id
)
{
    u32     i;
    u32     cnt;
    u32     n = 0;
    u16     port;
    u16     end_port;
    u16     usid = 0;
    u16     uport = 0;
    u16     begin_usid = 0;
    u16     begin_uport = 0;
    u16     prev_usid = 0;
    u16     prev_uport = 0;
    BOOL    b_new_range;

    icli_switch_port_range_t    *switch_range;

    if ( range == NULL ) {
        T_E("range == NULL\n");
        return FALSE;
    }

    if ( port_type >= ICLI_PORT_TYPE_MAX ) {
        T_E("invalid port type %d\n", port_type);
        return FALSE;
    }

    cnt = range->cnt;
    for ( i = 0; i < cnt; i++ ) {
        /* get switch_range */
        switch_range = &(range->switch_range[i]);

        if ( switch_range->port_cnt == 0 ) {
            T_E("port count is 0 in range[%d] \n", i);
            return FALSE;
        }

        /* check port definition */
        end_port = switch_range->begin_port + switch_range->port_cnt - 1;

        // get begin
        port = switch_range->begin_port;
        if ( icli_port_check_type(switch_range->switch_id, port, port_type, &begin_usid, &begin_uport) == FALSE ) {
            if ( switch_id && port_id ) {
                *switch_id = switch_range->switch_id;
                *port_id   = port;
            }
            return FALSE;
        }

        switch_range->port_type   = port_type;
        switch_range->usid        = begin_usid;
        switch_range->begin_uport = begin_uport;

        prev_usid  = begin_usid;
        prev_uport = begin_uport;

        b_new_range = FALSE;
        for ( port++; port <= end_port; port++) {
            if ( icli_port_check_type(switch_range->switch_id, port, port_type, &usid, &uport) == FALSE ) {
                if ( switch_id && port_id ) {
                    *switch_id = switch_range->switch_id;
                    *port_id   = port;
                }
                return FALSE;
            }

            // the usid and uport is not in sequence
            // create a new switch range
            if ( (usid != prev_usid) || uport != (prev_uport + 1) ) {
                n  = range->cnt;
                if ( n >= ICLI_RANGE_LIST_CNT ) {
                    T_E("range list is full\n");
                    return FALSE;
                }
                (range->cnt)++;
                range->switch_range[n].port_type   = port_type;
                range->switch_range[n].switch_id   = switch_range->switch_id;

                range->switch_range[n].begin_port  = port;
                range->switch_range[n].port_cnt    = 1;
                range->switch_range[n].usid        = usid;
                range->switch_range[n].begin_uport = uport;

                if ( b_new_range == FALSE ) {
                    b_new_range = TRUE;
                    // update port count
                    switch_range->port_cnt = port - switch_range->begin_port;
                }

            } else if ( b_new_range ) {
                (range->switch_range[n].port_cnt)++;
            }

            prev_usid  = usid;
            prev_uport = uport;
        } // for port
    } // for range cnt
    return TRUE;
}

/*
    check if a specific switch port is defined or not

    INPUT
        switch_id : switch id
        port_id   : port id

    OUTPUT
        n/a

    RETURN
        TRUE  : all the specific switch port is valid
        FALSE : not valid

    COMMENT
        n/a
*/
BOOL icli_port_switch_port_valid(
    IN u16      switch_id,
    IN u16      port_id
)
{
    u32     i;

    for ( i = 0; i < g_port_range.cnt; i++ ) {
        if ( switch_id != g_port_range.switch_range[i].switch_id ) {
            continue;
        }
        if ( _port_in_range( port_id,
                             g_port_range.switch_range[i].begin_port,
                             g_port_range.switch_range[i].port_cnt) ) {
            /* in range */
            return TRUE;
        }
    }

    /* not in range */
    return FALSE;
}

/*
    check if the switch port range is defined or not

    INPUT
        switch_port : switch port range

    OUTPUT
        switch_id, port_id : the port is not valid

    RETURN
        TRUE  : all the switch ports are valid
        FALSE : at least one of switch port is not valid

    COMMENT
        n/a
*/
BOOL icli_port_switch_range_valid(
    IN  icli_switch_port_range_t    *switch_port,
    OUT u16                         *switch_id,
    OUT u16                         *port_id
)
{
    u16                 port;
    u16                 end_port;

    if ( switch_port == NULL ) {
        T_E("switch_port == NULL");
        return FALSE;
    }

    end_port = switch_port->begin_port + switch_port->port_cnt - 1;
    for ( port = switch_port->begin_port; port <= end_port; port++ ) {
        if ( icli_port_switch_port_valid(switch_port->switch_id, port) == FALSE ) {
            if ( switch_id && port_id ) {
                *switch_id = switch_port->switch_id;
                *port_id   = port;
            }
            return FALSE;
        }
    }
    return TRUE;
}

/*
    check if all the stack switch port range is defined or not

    INPUT
        stack_port : switch port range in stack

    OUTPUT
        switch_id, port_id : the port is not valid

    RETURN
        TRUE  : all the switch ports in stack are valid
        FALSE : at least one of switch port is not valid

    COMMENT
        n/a
*/
BOOL icli_port_stack_range_valid(
    IN  icli_stack_port_range_t     *stack_port,
    OUT u16                         *switch_id,
    OUT u16                         *port_id
)
{
    u32     i;

    if ( stack_port == NULL ) {
        T_E("stack_port == NULL");
        return FALSE;
    }

    for ( i = 0; i < stack_port->cnt; i++ ) {
        if ( icli_port_switch_range_valid(&(stack_port->switch_range[i]), switch_id, port_id) == FALSE ) {
            return FALSE;
        }
    }
    return TRUE;
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
BOOL icli_enable_password_get_x(
    IN  icli_privilege_t    priv,
    OUT char                *password
)
{
    if ( priv >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privilege %u\n", priv);
        return FALSE;
    }

    if ( password == NULL ) {
        T_E("password == NULL\n");
        return FALSE;
    }

    (void)icli_str_cpy(password, g_conf_data.enable_password[priv]);
    return TRUE;
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
BOOL icli_enable_password_set_x(
    IN  icli_privilege_t    priv,
    IN  char                *password
)
{
    if ( priv >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privilege %u\n", priv);
        return FALSE;
    }

    if ( password == NULL ) {
        memset(g_conf_data.enable_password[priv], 0, ICLI_PASSWORD_MAX_LEN + 1);
        return TRUE;
    }

    if ( icli_str_len(password) > ICLI_PASSWORD_MAX_LEN ) {
        T_E("the length of password is too long\n");
        return FALSE;
    }

    g_conf_data.b_enable_secret[priv] = FALSE;
    (void)icli_str_cpy(g_conf_data.enable_password[priv], password);
    return TRUE;
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
BOOL icli_enable_password_verify_x(
    IN  icli_privilege_t    priv,
    IN  char                *clear_password
)
{
    u8      mac[16];
    char    str[ICLI_PASSWORD_MAX_LEN + 1];

    if ( priv >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privilege %u\n", priv);
        return FALSE;
    }

    if ( clear_password == NULL ) {
        T_E("clear_password == NULL\n");
        return FALSE;
    }

    if ( g_conf_data.b_enable_secret[priv] ) {
        // get mac
        icli_hmac_md5(g_enable_key, g_enable_key_len, (u8 *)clear_password, icli_str_len(clear_password), mac);

        // translate into string
        _mac_to_string(mac, str);

        if ( icli_str_cmp(str, g_conf_data.enable_password[priv] ) == 0 ) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        if ( icli_str_cmp(clear_password, g_conf_data.enable_password[priv] ) == 0 ) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
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
BOOL icli_enable_secret_set_x(
    IN  icli_privilege_t    priv,
    IN  char                *secret
)
{
    if ( priv >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privilege %u\n", priv);
        return FALSE;
    }

    if ( secret == NULL ) {
        memset(g_conf_data.enable_password[priv], 0, ICLI_PASSWORD_MAX_LEN + 1);
        return TRUE;
    }

    if ( icli_str_len(secret) > ICLI_PASSWORD_MAX_LEN ) {
        T_E("the length of secret is too long\n");
        return FALSE;
    }

    g_conf_data.b_enable_secret[priv] = TRUE;
    (void)icli_str_cpy(g_conf_data.enable_password[priv], secret);
    return TRUE;
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
BOOL icli_enable_secret_set_clear_x(
    IN  icli_privilege_t    priv,
    IN  char                *clear_password
)
{
    u8      mac[16];

    if ( priv >= ICLI_PRIVILEGE_MAX ) {
        T_E("invalid privilege %u\n", priv);
        return FALSE;
    }

    if ( clear_password == NULL ) {
        memset(g_conf_data.enable_password[priv], 0, ICLI_PASSWORD_MAX_LEN + 1);
        return TRUE;
    }

    // get mac
    icli_hmac_md5(g_enable_key, g_enable_key_len, (u8 *)clear_password, icli_str_len(clear_password), mac);

    // translate into string
    _mac_to_string(mac, g_conf_data.enable_password[priv]);

    g_conf_data.b_enable_secret[priv] = TRUE;

    return TRUE;
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
BOOL icli_enable_password_if_secret_get_x(
    IN  icli_privilege_t    priv
)
{
    return g_conf_data.b_enable_secret[priv];
}

#endif


#if 1 /* CP, 2012/10/08 14:31, debug command, debug prompt */
/*
    get debug prompt without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_debug_prompt

    COMMENT
        n/a
*/
char *icli_debug_prompt_get_x(void)
{
    return g_debug_prompt;
}

/*
    set device name shown in command prompt

    INPUT
        debug_prompt : debug prompt

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_debug_prompt_set_x(
    IN  char    *debug_prompt
)
{
    if ( debug_prompt == NULL ) {
        T_E("debug_prompt == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    (void)icli_str_ncpy(g_debug_prompt, debug_prompt, ICLI_NAME_MAX_LEN);
    g_debug_prompt[ICLI_NAME_MAX_LEN] = 0;
    return ICLI_RC_OK;
}
#endif
