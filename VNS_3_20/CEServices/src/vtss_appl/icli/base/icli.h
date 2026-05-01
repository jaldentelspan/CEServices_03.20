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
    > CP.Wang, 2011/04/07 16:22
        - create

******************************************************************************
*/
#ifndef __ICLI_H__
#define __ICLI_H__
//****************************************************************************
/*
******************************************************************************

    Default Type Include File

******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "icli_porting.h"
#include "icli_types.h"
#include "icli_porting_trace.h"

/*
******************************************************************************

    Constant and Macro

******************************************************************************
*/
#define ICLI_EOS                        0
#define ICLI_BELL                       7
#define ICLI_TAB                        9
#define ICLI_NEWLINE                    10
#define ICLI_SPACE                      ' '
#define ICLI_PLUS                       '+'
#define ICLI_MINUS                      '-'
#define ICLI_DASH                       '-'
#define ICLI_EQUAL                      '='
#define ICLI_TILDE                      '~'
#define ICLI_STAR                       '*'
#define ICLI_SLASH                      '/'
#define ICLI_BACK_SLASH                 '\\'
#define ICLI_UNDERLINE                  '_'
#define ICLI_DOT                        '.'
#define ICLI_COMMA                      ','
#define ICLI_BACKSPACE                  '\b'

#define ICLI_COMMENT_0                  '#'
#define ICLI_COMMENT_1                  '!'

#define ICLI_OR                         '|'
#define ICLI_MANDATORY_BEGIN            '{'
#define ICLI_MANDATORY_END              '}'
#define ICLI_OPTIONAL_BEGIN             '['
#define ICLI_OPTIONAL_END               ']'
#define ICLI_VARIABLE_BEGIN             '<'
#define ICLI_VARIABLE_END               '>'
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
#define ICLI_LOOP_BEGIN                 '('
#define ICLI_LOOP_END                   ')'
#endif
#define ICLI_DATE_DELIMITER             '/'
#define ICLI_TIME_DELIMITER             ':'
#define ICLI_IPV6_DELIMITER             ':'
#define ICLI_MAC_Z_DELIMITER            ':'
#define ICLI_IPV4_DELIMITER             '.'
#define ICLI_MAC_C_DELIMITER            '.'
#define ICLI_SUBNET_DELIMITER           '/'
#define ICLI_STRING_BEGIN               '"'
#define ICLI_STRING_END                 '"'
#define ICLI_PORT_DELIMITER             '/'
#define ICLI_PLIST_DELIMITER            ';'
#define ICLI_POINT                      '.'
#define ICLI_MAC_A_DELIMITER            '-'

#define ICLI_KEY_BACKSPACE              8
#define ICLI_KEY_TAB                    9
#define ICLI_KEY_ENTER                  13
#define ICLI_KEY_ESC                    27
#define ICLI_KEY_SPACE                  32
#define ICLI_KEY_QUESTION               63
#define ICLI_KEY_DEL                    127

#define ICLI_KEY_CTRL_(key)             (key - 'A' + 1)

// S is the above ICLI_*
#define ICLI_IS_(S,c)                   ((c) == ICLI_##S)
#define ICLI_NOT_(S,c)                  ((c) != ICLI_##S)
#define ICLI_IS_COMMENT(c)              ( (c) == ICLI_COMMENT_0 || (c) == ICLI_COMMENT_1 )
#define ICLI_PLAY_BELL                  (void)icli_sutil_usr_char_put(handle, ICLI_BELL)
#define ICLI_PUT_NEWLINE                (void)icli_sutil_usr_char_put(handle, ICLI_NEWLINE)
#define ICLI_PUT_SPACE                  (void)icli_sutil_usr_char_put(handle, ICLI_SPACE)
#define ICLI_PUT_BACKSPACE              (void)icli_sutil_usr_char_put(handle, ICLI_BACKSPACE)

#define ICLI_INVALID_LINE_NUMBER        (-1)

#define ICLI_PARAMETER_NULL_CHECK(p) \
    if ( p == NULL ) {\
        T_E(#p" == NULL\n");\
        return ICLI_RC_ERR_PARAMETER;\
    }

#define ICLI_SPACE_SKIP(c,w) \
    for ( (c) = (w); ICLI_IS_(SPACE, (*c)); c++ );

#define ICLI_TAB_SPACE_SKIP(c,w) \
    for ( (c) = (w); ICLI_IS_(SPACE, (*c)) || ICLI_IS_(TAB, (*c)); c++ );

#define ICLI_IS_DIGIT(c) \
    ( (c) >='0' && (c) <= '9' )

#define ICLI_IS_ALPHABET(c) \
    ( ( (c) >='a' && (c) <= 'z' )   || \
      ( (c) >='A' && (c) <= 'Z' )    )

#define ICLI_IS_KEYWORD(x) \
    (   ICLI_IS_ALPHABET(x)     ||  \
        ICLI_IS_DIGIT(x)        ||  \
        ICLI_IS_(PLUS,x)        ||  \
        ICLI_IS_(MINUS,x)       ||  \
        ICLI_IS_(STAR,x)        ||  \
        ICLI_IS_(SLASH,x)       ||  \
        ICLI_IS_(UNDERLINE,x)   ||  \
        ICLI_IS_(COMMA,x)       ||  \
        ICLI_IS_(TILDE,x)       ||  \
        ICLI_IS_(DOT,x)          )

#define ICLI_IS_SPACE_CHAR(c) \
    ( ICLI_IS_(SPACE,c)           || \
      ICLI_IS_(MANDATORY_BEGIN,c) || \
      ICLI_IS_(MANDATORY_END,c)   || \
      ICLI_IS_(LOOP_BEGIN,c) || \
      ICLI_IS_(LOOP_END,c)   || \
      ICLI_IS_(OPTIONAL_BEGIN,c)  || \
      ICLI_IS_(OPTIONAL_END,c)    || \
      ICLI_IS_(OR,c)               )

#define ICLI_IS_RANDOM_MUST(c)   \
    ( ICLI_IS_(STAR,*((c)+1)) && \
      ICLI_IS_DIGIT(*((c)+2)) && \
      ( ICLI_IS_SPACE_CHAR(*((c)+3)) || ICLI_IS_(EOS,*((c)+3)) ) \
    )

#define ___NEXT(p)          ((p) = (p)->next)

#define _SEMA_TAKE()        (void)icli_sema_take()
#define _SEMA_GIVE()        (void)icli_sema_give()

#define ___BIT_MASK_SET(x, level) \
    ((x) |= (0x1L) << (level))

#define ___BIT_MASK_GET(x, level) \
    ((x) & ((0x1L) << (level)))

#define ___CHILD(p) \
    ((p) = (p)->child)

#define ___SIBLING(p) \
    ((p) = (p)->sibling)

#define ___PARENT(p) \
    ((p) = (p)->parent)

/*
******************************************************************************

    Type Definition

******************************************************************************
*/
typedef enum {
    ICLI_WORD_STATE_INVALID = -1,
    //------- add below
    ICLI_WORD_STATE_SPACE,
    ICLI_WORD_STATE_WORD,
    ICLI_WORD_STATE_VARIABLE_BEGIN,
    ICLI_WORD_STATE_VARIABLE,
    ICLI_WORD_STATE_VARIABLE_END,

    //-------- add above
    ICLI_WORD_STATE_MAX

} icli_word_state_t;

typedef enum {
    ICLI_WORD_TYPE_INVALID = -1,
    //------- add below
    ICLI_WORD_TYPE_KEYWORD,
    ICLI_WORD_TYPE_VARIABLE,
    ICLI_WORD_TYPE_MANDATORY_BEGIN,
    ICLI_WORD_TYPE_MANDATORY_END,
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
    ICLI_WORD_TYPE_LOOP_BEGIN,
    ICLI_WORD_TYPE_LOOP_END,
#endif
    ICLI_WORD_TYPE_OPTIONAL_BEGIN,
    ICLI_WORD_TYPE_OPTIONAL_END,
    ICLI_WORD_TYPE_OR,
    ICLI_WORD_TYPE_EOS,

    //-------- add above
    ICLI_WORD_TYPE_MAX

} icli_word_type_t;

/*
******************************************************************************

    Component Include File

******************************************************************************
*/
#include "icli_variable.h"
#include "icli_parsing.h"
#include "icli_register.h"
#include "icli_util.h"
#include "icli_session.h"
#include "icli_exec.h"
#include "icli_session_a.h"
#include "icli_session_c.h"
#include "icli_session_z.h"
#include "icli_session_util.h"
#include "icli_porting_xy.h"

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
);

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
BOOL icli_is_init(void);

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
icli_conf_data_t *icli_conf_get_x(void);
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
);

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
);

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
);

/*
    take semaphore
*/
i32 icli_sema_take(void);

/*
    give semaphore
*/
i32 icli_sema_give(void);

/*
    get the time elapsed from system start in milliseconds
*/
u32 icli_current_time_get(void);

/*
    wait for other module init at the begin of ICLI session
*/
i32 icli_init_wait(void);

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
BOOL icli_has_user_auth(void);

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
);

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
);

#if 1 /* CP, 2012/08/31 17:00, enable secret */
void icli_hmac_md5(
    IN  const u8    *key,
    IN  size_t      key_len,
    IN  const u8    *data,
    IN  size_t      data_len,
    OUT u8          *mac
);
#endif

#if 1 /* CP, 2012/09/14 10:38, conf */
BOOL icli_conf_write( void );
#endif

#if 1 /* CP, 2012/09/04 13:33, switch information for prompt when stacking */
BOOL icli_switch_info_get(
    OUT icli_switch_info_t  *switch_info
);
#endif

#if 1 /* CP, 2012/09/11 14:08, Use thread information to get session ID */
u32 icli_thread_id_get( void );
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
icli_input_style_t icli_input_style_get_x(void);

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
);

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
u32 icli_case_sensitive_get(void);

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
u32 icli_console_alive_get(void);

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
);

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
);

/*
    get device name without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_dev_name

    COMMENT
        n/a
*/
char *icli_dev_name_get_x(void);

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
);

/*
    get LOGIN banner without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_banner_login

    COMMENT
        n/a
*/
char *icli_banner_login_get_x(void);

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
);

/*
    get MOTD banner without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_banner_motd

    COMMENT
        n/a
*/
char *icli_banner_motd_get_x(void);

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
);

/*
    get EXEC banner without semaphore

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        g_banner_exec

    COMMENT
        n/a
*/
char *icli_banner_exec_get_x(void);

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
);

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
);

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
);

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
void icli_port_range_reset_x( void );

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
);

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
);

/*
    get switch range from usid and uport

    INPUT
        switch_range->usid        : usid
        switch_range->begin_uport : uport
        switch_range->port_cnt    : number of ports

    OUTPUT
        switch_range->port_type   : port type
        switch_range->switch_id   : switch ID
        switch_range->begin_port  : port ID

    RETURN
        TRUE  : successful
        FALSE : failed

    COMMENT
        n/a
*/
BOOL icli_port_from_usid_uport_x(
    INOUT icli_switch_port_range_t  *switch_range
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);
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
char *icli_debug_prompt_get_x(void);

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
);
#endif

//****************************************************************************
#endif //__ICLI_H__

