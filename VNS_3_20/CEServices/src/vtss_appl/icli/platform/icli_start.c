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
/*lint --e{454,455,456} */
/*
******************************************************************************

    Include File

******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "icli_api.h"
#include "icli_console.h"
#include "icli_porting_trace.h"
#include "port_api.h"
#include "port_custom_api.h"
#include "msg_api.h"

#ifdef ICLI_CMD_REG
#include "icli_cmd_reg.h"
#endif

#ifdef VTSS_SW_OPTION_AUTH
#include "vtss_auth_api.h"
#endif

#ifdef VTSS_SW_OPTION_MD5 /* CP, 2012/08/31 17:00, enable secret */
#include "vtss_md5_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICFG
#include "icli_icfg.h"
#endif

#if 1 /* CP, 2012/09/04 13:33, switch information for prompt when stacking */
#include "led_api.h"
#include "msg_api.h"
#endif

#if 1 /* CP, 2012/09/14 10:38, conf */
#include "icli_conf.h"
#endif

//lint -sem(_thread_create, thread_protected)
//lint -sem(_conf_write_cb, thread_protected)
//lint -sem(_sema_take,     thread_protected)
//lint -sem(_sema_give,     thread_protected)

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC         CLOCK_REALTIME
#endif

#if 1 /* CP, 2012/09/10 17:37, enlarge stack size */
#define ICLI_THREAD_STACK_SIZE  (16 * 1024)
#else
#define ICLI_THREAD_STACK_SIZE  THREAD_DEFAULT_STACK_SIZE
#endif

/*
    The following 3 citical section protestions,
    Only one of three can be 1.
 */
#define ICLI_USE_MUTEX          0
#define ICLI_USE_CRITD          1
#define ICLI_USE_SEMAPHORE      0

#if ICLI_USE_CRITD
#include "critd_api.h"
#endif

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
/* thread */
static cyg_handle_t g_thread_handle[ICLI_SESSION_CNT];
static cyg_thread   g_thread_block[ICLI_SESSION_CNT];
static char         g_thread_stack[ICLI_SESSION_CNT][ICLI_THREAD_STACK_SIZE];

#if ICLI_USE_MUTEX
static cyg_mutex_t  g_mutex;
#endif

#if ICLI_USE_SEMAPHORE
static cyg_sem_t    g_sema;
#endif

#if ICLI_USE_CRITD
static critd_t      g_critd;
#endif

/* Vitesse trace */
#if (VTSS_TRACE_ENABLED)
static vtss_trace_reg_t trace_reg = {
    .module_id = VTSS_TRACE_MODULE_ID,
    .name      = "icli",
    .descr     = "ICLI Engine"
};

static vtss_trace_grp_t trace_grps[TRACE_GRP_CNT] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        .name      = "default",
        .descr     = "Default",
        .lvl       = VTSS_TRACE_LVL_WARNING,
        .timestamp = 1,
    },
    [TRACE_GRP_CRIT] = {
        .name      = "crit",
        .descr     = "Critical regions",
        .lvl       = VTSS_TRACE_LVL_ERROR,
        .timestamp = 1,
    }
};
#endif

/*
******************************************************************************

    Static Function

******************************************************************************
*/
static i32 _thread_create(
    IN  i32                     session_id,
    IN  char                    *name,
    IN  icli_thread_priority_t  priority,
    IN  icli_thread_entry_cb_t  *entry_cb,
    IN  i32                     entry_data
)
{
    i32     thread_priority = THREAD_DEFAULT_PRIO;

    if ( priority == ICLI_THREAD_PRIORITY_HIGH ) {
        thread_priority = THREAD_HIGHEST_PRIO;
    }

    cyg_thread_create(  thread_priority,
                        (cyg_thread_entry_t *)entry_cb,
                        (cyg_addrword_t)session_id,
                        name,
                        g_thread_stack[session_id],
                        ICLI_THREAD_STACK_SIZE,
                        &(g_thread_handle[session_id]),
                        &(g_thread_block[session_id]) );

    cyg_thread_resume( g_thread_handle[session_id] );
    return ICLI_RC_OK;
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
*/
static void *_malloc(
    IN u32   size
)
{
    return malloc( size );
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
static void _free(
    IN void     *mem
)
{
    free( mem );
}

/*
    take semaphore
*/
static i32 _sema_take(void)
{
#if ICLI_USE_MUTEX
    if ( cyg_mutex_lock(&g_mutex) == FALSE ) {
        cyg_mutex_unlock( &g_mutex );
        return ICLI_RC_ERROR;
    }
#endif

#if ICLI_USE_SEMAPHORE
    if ( cyg_semaphore_wait(&g_sema) == FALSE ) {
        cyg_semaphore_post( &g_sema );
        return ICLI_RC_ERROR;
    }
#endif

#if ICLI_USE_CRITD
#if (VTSS_TRACE_ENABLED)
    critd_enter(&g_critd, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__);
#else
    critd_enter(&g_critd);
#endif
#endif

    return ICLI_RC_OK;
}

/*
    give semaphore
*/
static i32 _sema_give(void)
{
#if ICLI_USE_MUTEX
    cyg_mutex_unlock( &g_mutex );
#endif

#if ICLI_USE_SEMAPHORE
    cyg_semaphore_post( &g_sema );
#endif

#if ICLI_USE_CRITD
#if (VTSS_TRACE_ENABLED)
    critd_exit(&g_critd, TRACE_GRP_CRIT, VTSS_TRACE_LVL_NOISE, __FILE__, __LINE__);
#else
    critd_exit(&g_critd);
#endif
#endif

    return ICLI_RC_OK;
}

/*
    get the time elapsed from system start in milli-seconds
*/
static u32 _current_time_get(void)
{
    struct timespec     tp;

    if ( clock_gettime(CLOCK_MONOTONIC, &tp) == -1 ) {
        T_E("failed to get system up time\n");
        return 0;
    }
    return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

/*
    wait for other module init at the begin of ICLI session
*/
static i32 _init_wait(void)
{
    return ICLI_RC_OK;
}

#ifdef VTSS_SW_OPTION_AUTH
/*
    authenticate user

    INPUT
        session_type : session type
        username     : user name
        password     : password for the user

    OUTPUT
        privilege    : the privilege level for the authenticated user

    RETURN
        icli_rc_t
*/
static i32 _user_auth(
    IN  icli_session_way_t  session_way,
    IN  char                *username,
    IN  char                *password,
    OUT i32                 *privilege
)
{
    vtss_auth_agent_t   agent;

    switch (session_way) {
    case ICLI_SESSION_WAY_CONSOLE:
        agent = VTSS_AUTH_AGENT_CONSOLE;
        break;

    case ICLI_SESSION_WAY_TELNET:
        agent = VTSS_AUTH_AGENT_TELNET;
        break;

    case ICLI_SESSION_WAY_SSH:
        agent = VTSS_AUTH_AGENT_SSH;
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
        *privilege = ICLI_PRIVILEGE_15;
        return ICLI_RC_OK;

    default:
        T_E("Invalid session way = %d\n", session_way);
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( vtss_authenticate(agent, username, password, (int *)privilege) == VTSS_OK ) {
        return ICLI_RC_OK;
    }
    return ICLI_RC_ERROR;
}
#endif

/*
    sleep for milli-seconds

    INPUT
        t : milli-seconds for sleep

    OUTPUT
        n/a

    RETURN
        n/a
*/
static void _sleep_cb(
    IN u32  t
)
{
    VTSS_OS_MSLEEP( t );
}

#ifdef VTSS_SW_OPTION_MD5 /* CP, 2012/08/31 17:00, enable secret */
static void _hmac_md5_cb(
    IN  const unsigned char *key,
    IN  size_t              key_len,
    IN  const unsigned char *data,
    IN  size_t              data_len,
    OUT unsigned char       *mac
)
{
    vtss_hmac_md5(key, key_len, data, data_len, mac);
}
#endif

#if 1 /* CP, 2012/09/04 13:33, switch information for prompt when stacking */
static void _switch_info_cb(
    OUT icli_switch_info_t  *switch_info
)
{
    switch_info->b_master = msg_switch_is_master();
    switch_info->usid     = led_usid_get();
}
#endif

#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
static u32 _thread_id_get_cb( void )
{
    return (u32)cyg_thread_self();
}
#endif

#if 1 /* CP, 2012/09/14 10:38, conf */
static BOOL _conf_write_cb( void )
{
#if 1 /* CP, 2012/12/04 10:28, using ICFG, but not conf */
    return TRUE;
#else
    return icli_conf_file_write();
#endif
}
#endif

static BOOL build_port_range(void)
{
    icli_stack_port_range_t  *rng;
    switch_iter_t            s_iter;
    port_iter_t              p_iter;
    icli_port_type_t         type;
    vtss_rc                  rc;
    BOOL                     b_rc;
    u32                      r_idx = 0;

    T_D("entry");

    icli_port_range_reset();

    rng = (icli_stack_port_range_t *) malloc(sizeof(icli_stack_port_range_t));
    if (!rng) {
        T_E("Cannot allocate memory for building port range");
        return FALSE;
    }
    memset(rng, 0, sizeof(icli_stack_port_range_t));

    if ((rc = switch_iter_init(&s_iter, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID)) != VTSS_RC_OK) {
        T_E("Cannot init switch iteration; rc=%d", rc);
        free(rng);
        return FALSE;
    }

    vtss_board_type_t board_type;
    while (switch_iter_getnext(&s_iter)) {
        u32 fe_cnt = 0, g_cnt = 0, g_2_5_cnt = 0, g_5_cnt = 0, g_10_cnt = 0, begin = 0;

        if (! s_iter.exists) {
            continue;
        }

        board_type = port_isid_info_board_type_get(s_iter.isid);

        if ((rc = port_iter_init(&p_iter, NULL, s_iter.isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_STACK)) != VTSS_RC_OK) {
            T_E("Cannot init port iteration for isid=%u; rc=%d. Ignoring switch.", s_iter.isid, rc);
            continue;
        }

        while (port_iter_getnext(&p_iter)) {
            if (! p_iter.exists) {
                continue;
            }
            port_cap_t cap = vtss_board_port_cap(board_type, p_iter.iport);
            if (cap & (PORT_CAP_10G_FDX)) {
                type = ICLI_PORT_TYPE_TEN_GIGABIT_ETHERNET;
                begin = ++g_10_cnt;
            } else if (cap & (PORT_CAP_5G_FDX)) {
                type = ICLI_PORT_TYPE_FIVE_GIGABIT_ETHERNET;
                begin = ++g_5_cnt;
            } else if (cap & PORT_CAP_2_5G_FDX) {
                type = ICLI_PORT_TYPE_2_5_GIGABIT_ETHERNET;
                begin = ++g_2_5_cnt;
            } else if (cap & PORT_CAP_1G_FDX) {
                type = ICLI_PORT_TYPE_GIGABIT_ETHERNET;
                begin = ++g_cnt;
            } else if (cap & PORT_CAP_100M_FDX) {
                type = ICLI_PORT_TYPE_FAST_ETHERNET;
                begin = ++fe_cnt;
            } else {
                T_D("Unexpected port speed. Capabilities = 0x%08x", (unsigned int)cap);
                continue;
            }
            if (rng->switch_range[r_idx].port_cnt == 0) {  // First port, init range entry
                rng->switch_range[r_idx].port_type   = type;
                rng->switch_range[r_idx].switch_id   = s_iter.usid;
                rng->switch_range[r_idx].begin_port  = begin;
                rng->switch_range[r_idx].usid        = s_iter.usid;
                rng->switch_range[r_idx].begin_uport = p_iter.uport;
            }

            if (rng->switch_range[r_idx].port_type != type) {  // Type change, we need a new range entry
                if (r_idx == ICLI_RANGE_LIST_CNT - 1) {
                    T_E("ICLI port range enumeration full; remaining ports will be ignored");
                    goto done;
                }
                ++r_idx;
                rng->switch_range[r_idx].port_cnt    = 0;
                rng->switch_range[r_idx].port_type   = type;
                rng->switch_range[r_idx].switch_id   = s_iter.usid;
                rng->switch_range[r_idx].begin_port  = begin;
                rng->switch_range[r_idx].usid        = s_iter.usid;
                rng->switch_range[r_idx].begin_uport = p_iter.uport;
            }

            rng->switch_range[r_idx].port_cnt++;

        }  /* while (port_iter_getnext(...)) */

        r_idx += (rng->switch_range[0].port_cnt > 0) ? 1 : 0;
    }  /* while (switch_iter_getnext(...) */

done:
    rng->cnt = r_idx;
    b_rc  = icli_port_range_set(rng);
    free(rng);

    T_D("exit, cnt=%u; return=%d", r_idx, (int)b_rc);

    return b_rc;
}

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/*
    Initialize the ICLI module

    return
       VTSS_RC_OK.
*/
vtss_rc icli_start(
    IN vtss_init_data_t     *data
)
{
    icli_init_data_t    init_data;

#ifdef VTSS_SW_OPTION_ICFG
    vtss_rc             rc = VTSS_OK;
#endif

    switch (data->cmd) {
    case INIT_CMD_INIT:

#if (VTSS_TRACE_ENABLED)
        /* Initialize and register trace resources */
        VTSS_TRACE_REG_INIT(&trace_reg, trace_grps, TRACE_GRP_CNT);
        VTSS_TRACE_REGISTER(&trace_reg);
#endif

#if ICLI_USE_MUTEX
        cyg_mutex_init(&g_mutex);
#endif

#if ICLI_USE_SEMAPHORE
        cyg_semaphore_init(&g_sema, 1);
#endif

#if ICLI_USE_CRITD
        critd_init(&g_critd, "ICLI", VTSS_MODULE_ID_ICLI, VTSS_TRACE_MODULE_ID, CRITD_TYPE_MUTEX);
        /* if not give, then it will crash. Wierd! */
        (void)_sema_give();
#endif

        /* init engine */
        memset(&init_data, 0, sizeof(init_data));
        init_data.input_style         = ICLI_INPUT_STYLE_SINGLE_LINE;
        init_data.console_alive       = TRUE;
        init_data.case_sensitive      = FALSE;
        init_data.thread_create_cb    = _thread_create;
        init_data.malloc_cb           = _malloc;
        init_data.free_cb             = _free;
        init_data.sema_take_cb        = _sema_take;
        init_data.sema_give_cb        = _sema_give;
        init_data.current_time_get_cb = _current_time_get;
        init_data.init_wait_cb        = _init_wait;
#ifdef VTSS_SW_OPTION_AUTH
        init_data.user_auth_cb        = _user_auth;
#endif
        init_data.sleep_cb            = _sleep_cb;

#ifdef VTSS_SW_OPTION_MD5 /* CP, 2012/08/31 17:00, enable secret */
        init_data.hmac_md5_cb         = _hmac_md5_cb;
#endif

#if 1 /* CP, 2012/09/04 13:33, switch information for prompt when stacking */
        init_data.switch_info_cb      = _switch_info_cb;
#endif

#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
        init_data.thread_id_get_cb    = _thread_id_get_cb;
#endif

#if 1 /* CP, 2012/09/14 10:38, conf */
        init_data.conf_write_cb       = _conf_write_cb;
#endif

        if ( icli_init(&init_data) != ICLI_RC_OK ) {
            T_E("fail to initialize ICLI engine\n");
            return VTSS_RC_ERROR;
        }

        /* port definition; start empty, fill in later */
        icli_port_range_reset();

#ifdef ICLI_CMD_REG
        /* command register */
        if ( icli_cmd_reg() == FALSE ) {
            return VTSS_RC_ERROR;
        }
#endif

#ifdef VTSS_SW_OPTION_ICFG
        rc = icli_icfg_init();
        if (rc != VTSS_OK) {
            T_D("fail to init icfg registration, rc = %s", error_txt(rc));
        }
#endif
        break;

    case INIT_CMD_START:
        /* start console */
        if ( icli_console_start() == FALSE ) {
            T_E("fail to start Console\n");
            return VTSS_RC_ERROR;
        }
        break;

    case INIT_CMD_MASTER_UP:
#if 1 /* CP, 2012/12/04 10:28, using ICFG, but not conf */
        if ( icli_conf_file_load(TRUE) == FALSE ) {
            T_E("fail to load config\n");
            return VTSS_RC_ERROR;
        }
#else
#if 1 /* CP, 2012/09/14 10:38, conf */
        if ( icli_conf_file_load(FALSE) == FALSE ) {
            T_E("fail to load config\n");
            return VTSS_RC_ERROR;
        }
#endif
#endif
        // fall through

    case INIT_CMD_SWITCH_DEL:
    case INIT_CMD_SWITCH_ADD:
        if (msg_switch_is_master()) {
            if (!build_port_range()) {
                T_E("ICLI Init, cmd = %d: Failed to build port range", data->cmd);
                return VTSS_RC_ERROR;
            }
        }
        break;

    case INIT_CMD_CONF_DEF:
#if 1 /* CP, 2012/09/14 10:38, conf */
        if ( icli_conf_file_load(TRUE) == FALSE ) {
            T_E("fail to load config\n");
            return VTSS_RC_ERROR;
        }
        break;
#endif

    case INIT_CMD_MASTER_DOWN:
    default:
        break;
    }
    return VTSS_RC_OK;
}

/*
    get text string for each icli_rc_t

    INPUT
        rc : icli_rc_t

    OUTPUT
        n/a

    RETURN
        text string

    COMMENT
        n/a
*/
char *icli_error_txt(
    IN  vtss_rc   rc
)
{
    if ( rc ) {}
    return "";
}

