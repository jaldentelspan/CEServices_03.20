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
    > CP.Wang, 2011/05/10 13:54
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
#define ICLI_EXEC_PUT_SPACE     icli_session_char_put(handle, ICLI_SPACE)
#define ICLI_EXEC_PUT_NEWLINE   icli_session_char_put(handle, ICLI_NEWLINE)

/*
******************************************************************************

    Type Definition

******************************************************************************
*/
static i32  _match_node_find(
    IN icli_session_handle_t    *handle,
    IN icli_parsing_node_t      *tree,
    IN char                     *w,
    IN BOOL                     enable_chk,
    IN BOOL                     cnt_reset
);

/*
******************************************************************************

    Static Variable

******************************************************************************
*/
static icli_parameter_t     g_parameter[ ICLI_PARAMETER_MEM_CNT ];
static icli_parameter_t     *g_para_free;
static i32                  g_para_cnt;

#if ICLI_RANDOM_OPTIONAL
static icli_parsing_node_t  *g_random_optional_node[ ICLI_RANDOM_OPTIONAL_CNT ];
#endif

#if 1 /* CP, 2012/10/11 16:09, performance enhancement: do not check same optional nodes */
static u32      g_match_optional_node[ICLI_CMD_WORD_CNT];
#endif

/*
******************************************************************************

    Static Function

******************************************************************************
*/
static icli_match_type_t _match_type_get(
    IN icli_variable_type_t     type,
    IN icli_rc_t                rc
)
{
    switch ( rc ) {
    case ICLI_RC_OK:
        switch ( type ) {
        case ICLI_VARIABLE_WORD:
        case ICLI_VARIABLE_KWORD:
        case ICLI_VARIABLE_STRING:
        case ICLI_VARIABLE_LINE:
        case ICLI_VARIABLE_RANGE_WORD:
        case ICLI_VARIABLE_RANGE_KWORD:
        case ICLI_VARIABLE_RANGE_STRING:
        case ICLI_VARIABLE_RANGE_LINE:
        case ICLI_VARIABLE_GREP_STRING:
#if 1 /* CP, 2012/10/08 16:06, <hostname> */
        case ICLI_VARIABLE_HOSTNAME:
#endif
            return ICLI_MATCH_TYPE_PARTIAL;
        default:
            return ICLI_MATCH_TYPE_EXACTLY;
        }

    case ICLI_RC_ERR_INCOMPLETE:
        return ICLI_MATCH_TYPE_INCOMPLETE;

    default:
        return ICLI_MATCH_TYPE_ERR;
    }
}

#if ICLI_RANDOM_MUST_NUMBER
#if 0 /* CP, 2012/10/24 11:01, avoid compile warning */
/*
    check if node is in the family of the ancestor

    return -
        TRUE  : yes, they are
        FALSE : no, they are not
*/
static BOOL _is_family(
    IN icli_parsing_node_t  *ancestor,
    IN icli_parsing_node_t  *node
)
{
    icli_parsing_node_t  *child;

    if ( ancestor->child == NULL ) {
        return FALSE;
    }

    for ( child = ancestor->child; child; ___SIBLING(child) ) {
        if ( node == child ) {
            return TRUE;
        }
        if ( _is_family(child, node) ) {
            return TRUE;
        }
    }
    return FALSE;
}
#endif

/*
    check if the node in the current random optional
    the node MUST be the leading node in the main path
    and must not be a sibling

    INPUT
        handle : current session handler
        node   : the node to be checked

    OUTPUT
        n/a

    RETURN
        TRUE  - yes, it is in
        FALSE - no

    COMMENT
        n/a
*/
static BOOL  _in_random_must(
    IN  icli_parsing_node_t     *must_head,
    IN  icli_parsing_node_t     *node
)
{
    u32                     must_level;
    icli_parsing_node_t     *child;

    if ( must_head == NULL ) {
        return FALSE;
    }

    if ( node == NULL ) {
        return FALSE;
    }

    must_level = must_head->random_must_level;

    if ( icli_parsing_random_head(must_head, node, NULL) ) {
        return TRUE;
    }
    if ( ___BIT_MASK_GET(node->optional_end, must_level) ) {
        return FALSE;
    }

    for ( child = node->child; child != NULL; ___CHILD(child) ) {
        if ( icli_parsing_random_head(must_head, child, NULL) ) {
            return TRUE;
        }
        if ( ___BIT_MASK_GET(child->optional_begin, must_level) ) {
            return FALSE;
        }
        if ( ___BIT_MASK_GET(child->optional_end, must_level) ) {
            return FALSE;
        }
    }
    return FALSE;
}

/*
    check if child of the optional end node is allowed to go forward or not

    INPUT
        handle : session handler
        node   : optional end node

    OUTPUT
        n/a

    RETURN
        TRUE  : child is allowed to go forward
        FALSE : not allowed

    COMMENT
        n/a
*/
static BOOL _must_child_allowed(
    IN  icli_session_handle_t   *handle,
    IN icli_parsing_node_t      *node
)
{
    u32                     i;
    u32                     n;
    icli_parsing_node_t     *random_must_head;
    icli_parameter_t        *p;

    for ( i = 0; i < ICLI_RANDOM_OPTIONAL_DEPTH; i++ ) {
        if ( node->random_optional[i] && node->random_optional[i]->random_must_begin) {
            random_must_head   = node->random_optional[i];
            n = 0;
            for (p = handle->runtime_data.cmd_var; p; ___NEXT(p)) {
                if ( p->match_node->random_must_head == random_must_head ) {
                    n++;
                }
            }
            if ( n < random_must_head->random_must_number ) {
                if ( _in_random_must(random_must_head, node->child) == FALSE ) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

/*
    check if randome optional of the optional end node is allowed to go forward or not

    INPUT
        handle : session handler
        node   : optional end node
        deep   : which deep level to check
                 if 0 then it means to check if deep level 0 is enough and
                 whether to be able to go to deep level 1

    OUTPUT
        n/a

    RETURN
        TRUE  : child is allowed to go forward
        FALSE : not allowed

    COMMENT
        n/a
*/
static BOOL _must_random_allowed(
    IN icli_session_handle_t    *handle,
    IN icli_parsing_node_t      *node,
    IN u32                      deep
)
{
    u32                     n;
    icli_parsing_node_t     *random_must_head;
    icli_parameter_t        *p;

    if ( node->random_optional[deep] && node->random_optional[deep]->random_must_begin) {
        random_must_head = node->random_optional[deep];
        n = 0;
        for (p = handle->runtime_data.cmd_var; p; ___NEXT(p)) {
            if ( p->match_node->random_must_head == random_must_head ) {
                n++;
            }
        }
        if ( n < random_must_head->random_must_number ) {
            return FALSE;
        }
    }
    return TRUE;
}

/*
    check if child of the optional end node is allowed to go forward or not

    INPUT
        handle : session handler
        node   : optional end node

    OUTPUT
        n/a

    RETURN
        TRUE  : child is allowed to go forward
        FALSE : not allowed

    COMMENT
        n/a
*/
static BOOL _session_random_must_get(
    IN  icli_session_handle_t   *handle
)
{
    u32                 i;
    icli_parameter_t    *p;

    memset(handle->runtime_data.random_must_head,  0, sizeof(handle->runtime_data.random_must_head));
    memset(handle->runtime_data.random_must_match, 0, sizeof(handle->runtime_data.random_must_match));

    for ( p = handle->runtime_data.cmd_var; p; ___NEXT(p) ) {
        if ( p->match_node->random_must_head ) {
            for ( i = 0; i < ICLI_RANDOM_MUST_CNT; i++ ) {
                if ( handle->runtime_data.random_must_head[i] ) {
                    if ( handle->runtime_data.random_must_head[i] == p->match_node->random_must_head ) {
                        break;
                    }
                } else {
                    break;
                }
            }
            if ( i >= ICLI_RANDOM_OPTIONAL_DEPTH ) {
                T_E("Too many Random Must\n");
                return FALSE;
            }
            if ( handle->runtime_data.random_must_head[i] ) {
                (handle->runtime_data.random_must_match[i])++;
            } else {
                handle->runtime_data.random_must_head[i]  = p->match_node->random_must_head;
                handle->runtime_data.random_must_match[i] = 1;
            }
        }
    }
    return TRUE;
}
#endif

/*
    get the first word in cmd_str

    INPUT
        cmd : command string
    OUTPUT
        cmd : point to the second word
        w   : the first word
    RETURN
        icli_rc_t
    COMMENT
        n/a
*/
static i32  _cmd_word_get(
    IN  icli_session_handle_t   *handle,
    OUT char                    **w
)
{
    char    *c;

    //skip space
    ICLI_SPACE_SKIP(c, handle->runtime_data.cmd_walk);

    //EOS
    if ( ICLI_IS_(EOS, (*c)) ) {
        *w = c;
        handle->runtime_data.cmd_walk = c;
        return ICLI_RC_ERR_EMPTY;
    }

    //get word
    *w = c;
    if ( ICLI_IS_(STRING_BEGIN, *c) ) {
        for ( c++; ICLI_NOT_(EOS, *c); c++ ) {
            /*
                If \" then the " is not a STRING_END.
                On the other hand, a space should follow ".
            */
            if ( ICLI_NOT_(BACK_SLASH, *(c - 1)) &&
                 ICLI_IS_(STRING_END, *c)      &&
                 (ICLI_IS_(SPACE, *(c + 1)) || ICLI_IS_(EOS, *(c + 1))) ) {
                c++;
                break;
            }
        }
    } else {
        for ( ; ICLI_NOT_(SPACE, *c) && ICLI_NOT_(EOS, *c); c++ ) {
            ;
        }
    }

    if ( ICLI_IS_(SPACE, *(c)) ) {
        *c = ICLI_EOS;
        c++;
    }

    //update cmd_walk
    handle->runtime_data.cmd_walk = c;
    return ICLI_RC_OK;
}

/*
    check if the node is present or not

    INPUT
        handle : session handle
        node   : node

    OUTPUT
        n/a

    RETURN
        TRUE  : present
        FALSE : not present

    COMMENT
        n/a
*/
static BOOL _present_chk(
    IN  icli_session_handle_t   *handle,
    IN  icli_parsing_node_t     *node
)
{
    node_property_t     *node_prop;
    icli_runtime_cb_t   *runtime_cb;
    icli_runtime_ask_t  ask;
    icli_runtime_t      *runtime;
    u32                 b,
                        b_ask;

    /* get memory */
    runtime = (icli_runtime_t *)icli_malloc( sizeof(icli_runtime_t) );
    if ( runtime == NULL ) {
        T_E("memory insufficient\n");
        return FALSE;
    }
    memset(runtime, 0, sizeof(icli_runtime_t));

    b_ask = FALSE;
    node_prop = &(node->node_property);
    for ( ; node_prop; ___NEXT(node_prop) ) {
        runtime_cb = node_prop->cmd_property->runtime_cb[node_prop->word_id];
        if ( runtime_cb ) {
            b_ask = TRUE;
            ask = ICLI_ASK_PRESENT;
            _SEMA_GIVE();
            b = (*runtime_cb)(handle->session_id, ask, runtime);
            _SEMA_TAKE();
            if ( b == TRUE ) {
                icli_free(runtime);
                return TRUE;
            }
        }
    }
    icli_free(runtime);
    if ( b_ask ) {
        return FALSE;
    }
    return TRUE;
}

/*
    check if the session has the enough privilege to run the node

    INPUT
        handle : session handle
        node   : node

    OUTPUT
        n/a

    RETURN
        TRUE  : enough privilege
        FALSE : not enough

    COMMENT
        n/a
*/
static BOOL _privilege_chk(
    IN  icli_session_handle_t   *handle,
    IN  icli_parsing_node_t     *node
)
{
    node_property_t     *node_prop;

    for ( node_prop = &(node->node_property); node_prop; ___NEXT(node_prop) ) {
        if ( handle->runtime_data.privilege >= node_prop->cmd_property->privilege ) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
    check the node should be enabled or disabled

    INPUT
        node : node

    OUTPUT
        n/a

    RETURN
        TRUE  : enable
        FALSE : disable

    COMMENT
        n/a
*/
static BOOL _enable_chk(
    IN  icli_parsing_node_t     *node
)
{
    node_property_t     *node_prop;

    for ( node_prop = &(node->node_property); node_prop; ___NEXT(node_prop) ) {
        if ( ICLI_CMD_IS_ENABLE(node_prop->cmd_property->property) ) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
    check the node is visible or invisible

    INPUT
        node : node

    OUTPUT
        n/a

    RETURN
        TRUE  : visible
        FALSE : invisible

    COMMENT
        n/a
*/
static BOOL _visible_chk(
    IN  icli_parsing_node_t     *node
)
{
    node_property_t     *node_prop;

    for ( node_prop = &(node->node_property); node_prop; ___NEXT(node_prop) ) {
        if ( ICLI_CMD_IS_VISIBLE(node_prop->cmd_property->property) ) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
    check the node is matched or not
    The match is checked by
    - present
    - privilege
    - command property
    - type, variable or keyword
    - enable/visible check

    INPUT
        handle     : session handle
        node       : node
        w          : command word to match
        level_bit  : optional level
        enable_chk : TRUE  - enable check
                     FALSE - visible check
        para       : fill the result in this parameter

    OUTPUT
        para       : if (*para) is NULL, create a new one and fill the result
                     in

    RETURN
        icli_match_type_t

    COMMENT
        para cannot be NULL
*/
static icli_match_type_t _node_match(
    IN    icli_session_handle_t     *handle,
    IN    icli_parsing_node_t       *node,
    IN    char                      *w,
    IN    BOOL                      enable_chk,
    INOUT icli_parameter_t          **para
)
{
    icli_parameter_t            *p;
    icli_parameter_t            *last_node;
    i32                         rc;
    u32                         err_pos;

    icli_runtime_cb_t           *runtime_cb;
    icli_runtime_ask_t          ask;
    icli_runtime_t              *runtime;
    u32                         b;
    icli_match_type_t           match_type;

    icli_variable_type_t        type;
    icli_range_t                *range;
    icli_port_type_t            port_type;

    u16                         switch_id;
    u16                         port_id;

    u16                         usid;
    u16                         uport;

    char                        *c;
    u32                         len;

    /* check present */
    if ( _present_chk(handle, node) == FALSE ) {
        return ICLI_MATCH_TYPE_ERR;
    }

    /* check privilege */
    if ( _privilege_chk(handle, node) == FALSE ) {
        return ICLI_MATCH_TYPE_ERR;
    }

    /* check command property */
    if ( enable_chk ) {
        //check enable or disable
        if ( _enable_chk(node) == FALSE ) {
            return ICLI_MATCH_TYPE_ERR;
        }
    } else {
        //check show or hide
        if ( _visible_chk(node) == FALSE ) {
            return ICLI_MATCH_TYPE_ERR;
        }
    }

    if ( *para ) {
        p = *para;
    } else {
        p = icli_exec_parameter_get();
        if ( p == NULL ) {
            T_E("memory insufficient\n");
            return ICLI_MATCH_TYPE_ERR;
        }
    }

    /* get memory */
    runtime = (icli_runtime_t *)icli_malloc( sizeof(icli_runtime_t) );
    if ( runtime == NULL ) {
        T_E("memory insufficient\n");
        if ( *para == NULL ) {
            icli_exec_parameter_free( p );
        }
        return ICLI_MATCH_TYPE_ERR;
    }
    memset(runtime, 0, sizeof(icli_runtime_t));

    switch ( node->type ) {
    case ICLI_VARIABLE_KEYWORD :
        /* compare keyword */
        if ( icli_str_len(w) == 0 ) {
            rc = 1;
        } else {
            /* pre-process w */
            c = NULL;
            len = icli_str_len( w );
            if ( w[len - 1] == ICLI_SPACE ) {
                // find the last non-space char
                for ( c = &(w[len - 1]); ICLI_IS_(SPACE, *c); c-- ) {
                    ;
                }
                // replace with EOS
                *(++c) = ICLI_EOS;
            }
            /* get the result */
            rc = icli_str_sub(w, node->word, icli_case_sensitive_get(), NULL);
            /* post-process w */
            if ( c ) {
                // put space back
                *c = ICLI_SPACE;
            }
        }

        /* get match type */
        switch ( rc ) {
        case 1 : /* partial match */
            match_type = ICLI_MATCH_TYPE_PARTIAL;
            p->value.type = ICLI_VARIABLE_KEYWORD;
            (void)icli_str_ncpy(p->value.u.u_word, w, ICLI_VALUE_STR_MAX_LEN);
            break;

        case 0 : /* exactly match */
            match_type = ICLI_MATCH_TYPE_EXACTLY;
            p->value.type = ICLI_VARIABLE_KEYWORD;
            (void)icli_str_ncpy(p->value.u.u_word, w, ICLI_VALUE_STR_MAX_LEN);
            break;

        case -1 : /* not match */
        default :
            match_type = ICLI_MATCH_TYPE_ERR;
            break;
        }
        break;

    default :
        /* variables, check and get match type */
        if ( icli_str_len(w) == 0 ) {
            match_type = ICLI_MATCH_TYPE_PARTIAL;
            /* actually not match any type yet */
            p->value.type = ICLI_VARIABLE_MAX;
        } else {
            type = node->type;
            range = node->range;

            runtime_cb = node->node_property.cmd_property->runtime_cb[node->word_id];
            if ( runtime_cb ) {
                ask = ICLI_ASK_RANGE;
                _SEMA_GIVE();
                b = (*runtime_cb)(handle->session_id, ask, runtime);
                _SEMA_TAKE();
                if ( b == TRUE ) {
                    range = &(runtime->range);
                    switch ( type ) {
                    case ICLI_VARIABLE_RANGE_LIST:
                        switch ( range->type ) {
                        case ICLI_RANGE_TYPE_NONE:
                        default:
                            T_E("fail to get range type %u\n", range->type);
                            break;

                        case ICLI_RANGE_TYPE_SIGNED:
                            type = ICLI_VARIABLE_INT_RANGE_LIST;
                            break;

                        case ICLI_RANGE_TYPE_UNSIGNED:
                            type = ICLI_VARIABLE_UINT_RANGE_LIST;
                            break;
                        }
                        break;

                    case ICLI_VARIABLE_INT:
                    case ICLI_VARIABLE_UINT:
                        switch ( range->type ) {
                        case ICLI_RANGE_TYPE_NONE:
                        default:
                            T_E("fail to get range type %u\n", range->type);
                            break;

                        case ICLI_RANGE_TYPE_SIGNED:
                            type = ICLI_VARIABLE_INT_RANGE_ID;
                            break;

                        case ICLI_RANGE_TYPE_UNSIGNED:
                            type = ICLI_VARIABLE_UINT_RANGE_ID;
                            break;
                        }
                        break;

#if 1 /* CP, 2012/10/24 14:08, Bugzilla#10092 - runtime variable word length */
                    case ICLI_VARIABLE_WORD:
                        if ( range->type == ICLI_RANGE_TYPE_UNSIGNED ) {
                            type = ICLI_VARIABLE_RANGE_WORD;
                        } else {
                            T_E("Invalid range type %u, must be ICLI_RANGE_TYPE_UNSIGNED\n", range->type);
                        }
                        break;

                    case ICLI_VARIABLE_KWORD:
                        if ( range->type == ICLI_RANGE_TYPE_UNSIGNED ) {
                            type = ICLI_VARIABLE_RANGE_KWORD;
                        } else {
                            T_E("Invalid range type %u, must be ICLI_RANGE_TYPE_UNSIGNED\n", range->type);
                        }
                        break;

                    case ICLI_VARIABLE_STRING:
                        if ( range->type == ICLI_RANGE_TYPE_UNSIGNED ) {
                            type = ICLI_VARIABLE_RANGE_STRING;
                        } else {
                            T_E("Invalid range type %u, must be ICLI_RANGE_TYPE_UNSIGNED\n", range->type);
                        }
                        break;

                    case ICLI_VARIABLE_LINE:
                        if ( range->type == ICLI_RANGE_TYPE_UNSIGNED ) {
                            type = ICLI_VARIABLE_RANGE_LINE;
                        } else {
                            T_E("Invalid range type %u, must be ICLI_RANGE_TYPE_UNSIGNED\n", range->type);
                        }
                        break;
#endif

                    default:
                        break;
                    }
                }
            }

            /* parsing and get variable value */
            rc = icli_variable_get(type, w, range, &(p->value), &err_pos);

            /* get last node */
            port_type = ICLI_PORT_TYPE_NONE;
            if ( handle->runtime_data.last_port_type != ICLI_PORT_TYPE_NONE ) {
                port_type = handle->runtime_data.last_port_type;
                //handle->runtime_data.last_port_type = ICLI_PORT_TYPE_NONE;
            } else if ( handle->runtime_data.cmd_var ) {
                for ( last_node = handle->runtime_data.cmd_var;
                      last_node->next != NULL;
                      last_node = last_node->next ) {
                    ;
                }
                if (last_node->value.type == ICLI_VARIABLE_PORT_TYPE) {
                    port_type = last_node->value.u.u_port_type;
                }
            }

            /* runtime check value */
            switch ( rc ) {
            case ICLI_RC_OK:
                switch ( type ) {
                case ICLI_VARIABLE_PORT_ID:
#if 1 /* CP, 2012/09/05 17:42, <port_type_id> */
                case ICLI_VARIABLE_PORT_TYPE_ID:
#endif
                    if ( port_type != ICLI_PORT_TYPE_NONE ) {
                        if ( icli_port_check_type(  p->value.u.u_port_id.switch_id,
                                                    p->value.u.u_port_id.begin_port,
                                                    port_type,
                                                    &usid,
                                                    &uport) ) {
                            p->value.u.u_port_id.port_type   = port_type;
                            p->value.u.u_port_id.port_cnt    = 1;
                            p->value.u.u_port_id.usid        = usid;
                            p->value.u.u_port_id.begin_uport = uport;
                        } else {
                            rc = ICLI_RC_ERR_MATCH;
                            (void)icli_session_error_printf(handle, "%% No such port: %s %u/%u\n",
                                                            icli_variable_port_type_get_name(port_type),
                                                            p->value.u.u_port_id.switch_id,
                                                            p->value.u.u_port_id.begin_port);
                        }
#if 1 /* CP, 2012/09/05 18:21, if no port type at front, then do not check  */
                    }
#else
                    }
                    else if ( icli_port_switch_range_valid(&(p->value.u.u_port_id),
                                                           &switch_id,
                                                           &port_id) == FALSE ) {
                        rc = ICLI_RC_ERR_MATCH;
                        (void)icli_session_error_printf(handle, "%% switch %u port %u is not a legal port\n",
                                                        switch_id, port_id);
                    }
#endif
                    break;

                case ICLI_VARIABLE_PORT_LIST:
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
                case ICLI_VARIABLE_PORT_TYPE_LIST:
#endif
                    if ( port_type != ICLI_PORT_TYPE_NONE ) {
                        if ( icli_port_range_check_type(&(p->value.u.u_port_list),
                                                        port_type,
                                                        &switch_id,
                                                        &port_id) == FALSE ) {
                            rc = ICLI_RC_ERR_MATCH;
                            (void)icli_session_error_printf(handle, "%% No such port: %s %u/%u\n",
                                                            icli_variable_port_type_get_name(port_type), switch_id, port_id);
                        }
#if 1 /* CP, 2012/09/05 18:21, if no port type at front, then do not check  */
                    }
#else
                    }
                    else if ( icli_port_stack_range_valid(&(p->value.u.u_port_list),
                                                          &switch_id,
                                                          &port_id) == FALSE ) {
                        rc = ICLI_RC_ERR_MATCH;
                        (void)icli_session_error_printf(handle, "%% switch %u port %u is not a legal port\n",
                                                        switch_id, port_id);
                    }
#endif
                    break;

                default:
                    break;
                }

                if ( rc == ICLI_RC_ERR_MATCH ) {
                    break;
                }

                //fall through

            case ICLI_RC_ERR_INCOMPLETE:
                switch ( type ) {
                case ICLI_VARIABLE_PORT_TYPE:
                    if ( icli_port_type_present_x(p->value.u.u_port_type) == FALSE ) {
                        err_pos = 0;
                        rc = ICLI_RC_ERR_MATCH;
                    }
                    break;

                default:
                    break;
                }
                break;

            default:
                break;
            }

            /* get match_type */
            match_type = _match_type_get( type, rc );
        }
        break;
    }

    switch ( match_type )
    {
    case ICLI_MATCH_TYPE_EXACTLY : /* exactly match */
    case ICLI_MATCH_TYPE_PARTIAL : /* partial match */
        p->word_id    = node->word_id;
        p->match_node = node;
        p->match_type = match_type;

        if ( *para == NULL ) {
            *para = p;
        }
        break;

    default :
        match_type = ICLI_MATCH_TYPE_ERR;

        // fall through

    case ICLI_MATCH_TYPE_ERR : /* not match */
    case ICLI_MATCH_TYPE_INCOMPLETE :
        if ( *para == NULL ) {
            icli_exec_parameter_free( p );
        } else {
            p->match_type = match_type;
        }
        break;
    }
    icli_free(runtime);
    return match_type;
}

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
/*
    check if the node is matched or not only, but not check other relations
    this is for loop

    INPUT
        handle     : session handle
        node       : the node to match
        w          : command word to match
        enable_chk : TRUE  - enable check
                     FALSE - visible check
        cnt_reset  : reset counters or not

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        the result is stored in handle,
        match_cnt, match_node, match_para
        Return ICLI_RC_OK does not mean match node is found, but only mean
        the process works fine.
*/
static i32  _match_single_node(
    IN icli_session_handle_t    *handle,
    IN icli_parsing_node_t      *node,
    IN char                     *w,
    IN BOOL                     enable_chk,
    IN BOOL                     cnt_reset
)
{
    icli_match_type_t       match_type;
    icli_parameter_t        *para,
                            *p,
                            *prev;

    if ( cnt_reset ) {
        handle->runtime_data.exactly_cnt     = 0;
        handle->runtime_data.partial_cnt     = 0;
        handle->runtime_data.total_match_cnt = 0;
    }

    if ( node == NULL ) {
        return ICLI_RC_OK;
    }

    para = NULL;
    match_type = _node_match(handle, node, w, enable_chk, &para);
    switch ( match_type ) {
    case ICLI_MATCH_TYPE_REDO:
        /*
            if parsing ICLI_VARIABLE_LINE, then cmd word is re-aligned.
            So, redo the match for all possible next nodes.
        */
        return ICLI_RC_ERR_REDO;

    case ICLI_MATCH_TYPE_EXACTLY : /* exactly match */
    case ICLI_MATCH_TYPE_PARTIAL : /* partial match */
        // chain to match_para
        if ( handle->runtime_data.match_para ) {
            // chain
            p = handle->runtime_data.match_para;
            for ( prev = p, ___NEXT(p); p; prev = p, ___NEXT(p) ) {
                ;
            }
            para->next = NULL;
            prev->next = para;
        } else {
            // first para
            handle->runtime_data.match_para = para;
        }

        // not in loop
        para->b_in_loop = TRUE;

        // update cnt
        (handle->runtime_data.total_match_cnt)++;
        if ( match_type == ICLI_MATCH_TYPE_EXACTLY ) {
            (handle->runtime_data.exactly_cnt)++;
            // exactly match first, don't care after
            break;
        } else {
            (handle->runtime_data.partial_cnt)++;
        }
        /* fall through */

    case ICLI_MATCH_TYPE_INCOMPLETE : /* incomplete */
        if ( match_type == ICLI_MATCH_TYPE_INCOMPLETE ) {
            handle->runtime_data.rc = ICLI_RC_ERR_INCOMPLETE;
        }
        /* fall through */

    case ICLI_MATCH_TYPE_ERR : /* not match */
        break;

    default : /* error */
        T_E("invalid match type %d\n", match_type);
        return ICLI_RC_ERROR;
    }
    return ICLI_RC_OK;
}
#endif

#if ICLI_RANDOM_OPTIONAL
static void _random_optional_reset(void)
{
    i32     i;

    for ( i = 0; i < ICLI_RANDOM_OPTIONAL_CNT; i++ ) {
        g_random_optional_node[i] = NULL;
    }
}

static i32  _random_optional_match(
    IN icli_session_handle_t    *handle,
    IN icli_parsing_node_t      *node,
    IN char                     *w,
    IN BOOL                     enable_chk,
    IN BOOL                     cnt_reset,
    IN i32                      optional_level
)
{
    i32     d;
    i32     i;
    BOOL    b;
    i32     rc;

    for ( d = 0; d < ICLI_RANDOM_OPTIONAL_DEPTH; d++ ) {
#if ICLI_RANDOM_MUST_NUMBER
        /*
            check if the random must is enough at this level
            so that we can go to next deep level
        */
        if ( d ) {
            if ( _must_random_allowed(handle, node, d - 1) == FALSE ) {
                return ICLI_RC_OK;
            }
        }
#endif
        // with random optional
        if ( node->random_optional[d] == NULL ) {
            continue;
        }
        // check optional level
        if ( optional_level >= 0 ) {
            if ( node->random_optional_level[d] != (u32)optional_level ) {
                continue;
            }
        }
        // check if already found
        b = FALSE;
        for ( i = 0; i < ICLI_RANDOM_OPTIONAL_CNT; i++ ) {
            // check complete
            if ( g_random_optional_node[i] == NULL ) {
                break;
            }
            // already found
            if ( g_random_optional_node[i] == node->random_optional[d] ) {
                b = TRUE;
                break;
            }
        }
        // already found, check next
        if ( b ) {
            continue;
        }
        /* not find yet */
        // but overflow
        if ( i >= ICLI_RANDOM_OPTIONAL_CNT ) {
            T_E("g_random_optional overflow\n");
            return ICLI_RC_ERROR;
        }

        // record
        g_random_optional_node[i] = node->random_optional[d];

        // find match node
        rc = _match_node_find(handle, node->random_optional[d], w, enable_chk, cnt_reset);
        switch ( rc ) {
        case ICLI_RC_ERR_REDO:
            return ICLI_RC_ERR_REDO;
        default:
            break;
        }
    }
    return ICLI_RC_OK;
}
#endif

/*
    find the match nodes behind optional nodes

    INPUT
        handle     : session handle
        node       : node
        w          : command word to match
        level_bit  : optional level
        enable_chk : TRUE  - enable check
                     FALSE - visible check

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        the result is stored in handle,
        match_cnt, match_node, match_para
*/
static i32  _match_node_find_behind_optional(
    IN icli_session_handle_t    *handle,
    IN icli_parsing_node_t      *node,
    IN char                     *w,
    IN i32                      optional_level,
    IN BOOL                     enable_chk
)
{
    icli_parsing_node_t     *child_node;
    i32                     rc;

#if 1 /* CP, 2012/10/11 16:09, performance enhancement: do not check same optional nodes */
    /* already visited */
    if ( ___BIT_MASK_GET(g_match_optional_node[node->word_id], optional_level) ) {
        return ICLI_RC_OK;
    } else {
        ___BIT_MASK_SET(g_match_optional_node[node->word_id], optional_level);
    }
#endif

    // end at this node
    if ( ___BIT_MASK_GET(node->optional_end, optional_level) ) {

#if ICLI_RANDOM_MUST_NUMBER
        /*
            check if random must is enough or not
            if not then can not go to child
         */
        if ( _must_child_allowed(handle, node) == FALSE ) {
            return ICLI_RC_OK;
        }
#endif

        // find child of optional end
        rc = _match_node_find( handle, node->child, w, enable_chk, FALSE );
        switch ( rc ) {
        case ICLI_RC_ERR_REDO:
            return ICLI_RC_ERR_REDO;
        default:
            break;
        }

#if ICLI_RANDOM_OPTIONAL
        rc = _random_optional_match(handle, node, w, enable_chk, FALSE, optional_level);
        switch ( rc ) {
        case ICLI_RC_OK:
            break;
        case ICLI_RC_ERR_REDO:
            return ICLI_RC_ERR_REDO;
        default:
            return ICLI_RC_ERROR;
        }
#endif
        return ICLI_RC_OK;
    }

    // end at child
    for ( child_node = node->child; child_node != NULL; ___SIBLING(child_node) ) {
        rc = _match_node_find_behind_optional(handle, child_node, w, optional_level, enable_chk);
        switch ( rc ) {
        case ICLI_RC_ERR_REDO:
            return ICLI_RC_ERR_REDO;
        default:
            break;
        }
    }
    return ICLI_RC_OK;
}

/*
    find the match nodes in sibling or optional child
    so, the match may be multiple
    the results are stored in session handle, match_cnt, match_para

    INPUT
        handle     : session handle
        tree       : the tree to find
        w          : command word to match
        enable_chk : TRUE  - enable check
                     FALSE - visible check
        cnt_reset  : reset counters or not

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        the result is stored in handle,
        match_cnt, match_node, match_para
        Return ICLI_RC_OK does not mean match node is found, but only mean
        the process works fine.
*/
static i32  _match_node_find(
    IN icli_session_handle_t    *handle,
    IN icli_parsing_node_t      *tree,
    IN char                     *w,
    IN BOOL                     enable_chk,
    IN BOOL                     cnt_reset
)
{
    i32                     i,
                            rc;
    BOOL                    b;
    icli_match_type_t       match_type;
    icli_parameter_t        *para,
                            *p,
                            *prev;
    icli_parsing_node_t     *node,
                            *child_node;

#if ICLI_RANDOM_OPTIONAL
    icli_parsing_node_t     *sibling_node;
#endif

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
    i32                     b_in_loop;
#endif

    if ( cnt_reset ) {
        handle->runtime_data.exactly_cnt     = 0;
        handle->runtime_data.partial_cnt     = 0;
        handle->runtime_data.total_match_cnt = 0;
    }

    if ( tree == NULL ) {
        return ICLI_RC_OK;
    }

#if ICLI_RANDOM_OPTIONAL
    if ( handle->runtime_data.b_in_loop == FALSE ) {
        node = tree;
        for ( p = handle->runtime_data.cmd_var; p; ___NEXT(p) ) {
            // already traversed,
            // if its sibling already traversed, then it is traversed
            b = FALSE;
            for ( sibling_node = tree; sibling_node != NULL; ___SIBLING(sibling_node) ) {
                if ( p->match_node == sibling_node ) {
                    b = TRUE;
                    break;
                }
            }
            if ( b == FALSE ) {
                continue;
            }

            // not optional node
            if ( node->optional_begin == 0 ) {
                return ICLI_RC_OK;
            }

            // if the tree already traverse and it is an optional node
            // then try its following child
            for ( i = 0; i < 32; i++ ) {
#if 1 /* CP, 2012/10/22 14:21, performance enhance on execution */
                if ( ((node->optional_begin) >> i) == 0 ) {
                    break;
                }
#endif
                // optional begin
                if ( ___BIT_MASK_GET(node->optional_begin, i) == 0 ) {
                    continue;
                }

                // end at itself
                if ( ___BIT_MASK_GET(node->optional_end, i) ) {

#if ICLI_RANDOM_MUST_NUMBER
                    /*
                        check if random must is enough or not
                        if not then can not go to child
                     */
                    if ( _must_child_allowed(handle, node) == FALSE ) {
                        continue;
                    }
#endif

                    rc = _match_node_find( handle, node->child, w, enable_chk, cnt_reset );
                    switch ( rc ) {
                    case ICLI_RC_ERR_REDO:
                        return ICLI_RC_ERR_REDO;
                    default:
                        break;
                    }
                    //
                    //  random optional is not processed here,
                    //  because it is processed in cmd_walk() and next_node_find()
                    //
                    continue;
                }

                // end at child
                for ( child_node = node->child; child_node != NULL; ___SIBLING(child_node) ) {
                    rc = _match_node_find_behind_optional( handle, child_node, w, i, enable_chk );
                    switch ( rc ) {
                    case ICLI_RC_ERR_REDO:
                        return ICLI_RC_ERR_REDO;
                    default:
                        break;
                    }
                }
            }
            return ICLI_RC_OK;
        }
    } // if ( handle->runtime_data.b_in_loop == FALSE )
#endif

    // find match node
    for ( node = tree; node != NULL; node = node->sibling ) {
        para = NULL;
        match_type = _node_match(handle, node, w, enable_chk, &para);
        switch ( match_type ) {
        case ICLI_MATCH_TYPE_REDO:
            /*
                if parsing ICLI_VARIABLE_LINE, then cmd word is re-aligned.
                So, redo the match for all possible next nodes.
            */
            return ICLI_RC_ERR_REDO;

        case ICLI_MATCH_TYPE_EXACTLY : /* exactly match */
        case ICLI_MATCH_TYPE_PARTIAL : /* partial match */
            // chain to match_para
            if ( handle->runtime_data.match_para ) {
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
                if ( handle->runtime_data.b_in_loop == FALSE ) {
#endif
                    b = FALSE;
                    /* check duplicate */
                    for ( p = handle->runtime_data.match_para; p; ___NEXT(p) ) {
                        if ( p->match_node == node ) {
                            b = TRUE;
                            break;
                        }
                    }
#if ICLI_RANDOM_OPTIONAL
                    /* check if traversal */
                    if ( b == FALSE ) {
                        for ( p = handle->runtime_data.cmd_var; p; ___NEXT(p) ) {
                            // already traversed,
                            // if its sibling already traversed, then it is traversed
                            b = FALSE;
                            for ( sibling_node = node; sibling_node != NULL; ___SIBLING(sibling_node) ) {
                                if ( p->match_node == sibling_node ) {
                                    b = TRUE;
                                    break;
                                }
                            }
                            if ( b == TRUE ) {
                                break;
                            }
                        }
                    }
#endif
                    /*
                        already exist, also means ever traversed
                        so, do not need to fall through
                        just go to check next sibling
                    */
                    if ( b == TRUE ) {
                        icli_exec_parameter_free( para );
                        break;
                    }
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
                }
#endif

                // chain
                p = handle->runtime_data.match_para;
                for ( prev = p, ___NEXT(p); p; prev = p, ___NEXT(p) ) {
                    ;
                }
                para->next = NULL;
                prev->next = para;
            } else {
                // first para
                handle->runtime_data.match_para = para;
            }

            // update cnt
            (handle->runtime_data.total_match_cnt)++;
            if ( match_type == ICLI_MATCH_TYPE_EXACTLY ) {
                (handle->runtime_data.exactly_cnt)++;
                // exactly match first, don't care after
                break;
            } else {
                (handle->runtime_data.partial_cnt)++;
            }
            /* fall through */

        case ICLI_MATCH_TYPE_INCOMPLETE : /* incomplete */
            if ( match_type == ICLI_MATCH_TYPE_INCOMPLETE ) {
                handle->runtime_data.rc = ICLI_RC_ERR_INCOMPLETE;
            }
            /* fall through */

        case ICLI_MATCH_TYPE_ERR : /* not match */

            // not optional node
            if ( node->optional_begin == 0 ) {
                break;
            }

            // optional node, find child
            for ( i = 0; i < 32; i++ ) {
#if 1 /* CP, 2012/10/22 14:21, performance enhance on execution */
                if ( ((node->optional_begin) >> i) == 0 ) {
                    break;
                }
#endif
                // optional begin
                if ( ___BIT_MASK_GET(node->optional_begin, i) == 0 ) {
                    continue;
                }

                // end at itself
                if ( ___BIT_MASK_GET(node->optional_end, i) ) {

#if ICLI_RANDOM_MUST_NUMBER
                    /*
                        check if random must is enough or not
                        if not then can not go to child
                     */
                    if ( _must_child_allowed(handle, node) == FALSE ) {
                        continue;
                    }
#endif

                    rc = _match_node_find( handle, node->child, w, enable_chk, FALSE );
                    switch ( rc ) {
                    case ICLI_RC_ERR_REDO:
                        return ICLI_RC_ERR_REDO;
                    default:
                        break;
                    }
#if ICLI_RANDOM_OPTIONAL
                    rc = _random_optional_match(handle, node, w, enable_chk, FALSE, i);
                    switch ( rc ) {
                    case ICLI_RC_OK:
                        break;
                    case ICLI_RC_ERR_REDO:
                        return ICLI_RC_ERR_REDO;
                    default:
                        return ICLI_RC_ERROR;
                    }
#endif
                    continue;
                }

                // end at child
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
                b_in_loop = handle->runtime_data.b_in_loop;
                handle->runtime_data.b_in_loop = FALSE;
#endif
                for ( child_node = node->child; child_node != NULL; ___SIBLING(child_node) ) {
                    rc = _match_node_find_behind_optional( handle, child_node, w, i, enable_chk );
                    switch ( rc ) {
                    case ICLI_RC_ERR_REDO:
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
                        handle->runtime_data.b_in_loop = b_in_loop;
#endif
                        return ICLI_RC_ERR_REDO;
                    default:
                        break;
                    }
                }
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
                handle->runtime_data.b_in_loop = b_in_loop;
#endif
            }
            break;

        default : /* error */
            T_E("invalid match type %d\n", match_type);
            return ICLI_RC_ERROR;
        }
    }
    return ICLI_RC_OK;
}

static void _error_display(
    IN  icli_session_handle_t   *handle,
    IN  char                    *w,
    IN  char                    *err_descr
)
{
    u32     i, j, pos, w_pos;

#if 1 /* CP, 2012/08/20 11:10, because err_display_mode is for error syntax only, not for application error message */
    BOOL    b_change_mode;
#endif

    // if exec by API then do what application wants
    if ( handle->runtime_data.b_exec_by_api ) {
        if ( handle->runtime_data.app_err_msg ) {
#if 1 /* CP, 2012/08/20 11:10, because err_display_mode is for error syntax only, not for application error message */
            b_change_mode = FALSE;
            if ( handle->runtime_data.err_display_mode == ICLI_ERR_DISPLAY_MODE_DROP ) {
                handle->runtime_data.err_display_mode = ICLI_ERR_DISPLAY_MODE_PRINT;
                b_change_mode = TRUE;
            }
#endif
            if ( handle->runtime_data.line_number == ICLI_INVALID_LINE_NUMBER ) {
                icli_session_str_put(handle, handle->runtime_data.app_err_msg);
                ICLI_EXEC_PUT_NEWLINE;
                if ( handle->runtime_data.err_display_mode == ICLI_ERR_DISPLAY_MODE_DROP ) {
                    ICLI_EXEC_PUT_NEWLINE;
                }
            } else {
                icli_session_str_printf(handle, "%% Error in file %s, line %d:\n",
                                        handle->runtime_data.app_err_msg, handle->runtime_data.line_number);
            }
#if 1 /* CP, 2012/08/20 11:10, because err_display_mode is for error syntax only, not for application error message */
            if ( b_change_mode ) {
                handle->runtime_data.err_display_mode = ICLI_ERR_DISPLAY_MODE_DROP;
            }
#endif
        }

        if ( handle->runtime_data.err_display_mode == ICLI_ERR_DISPLAY_MODE_DROP ) {
            return;
        }
    }

    // TAB key, play beep
    switch ( handle->runtime_data.exec_type ) {
    case ICLI_EXEC_TYPE_TAB:
        ICLI_PLAY_BELL;
        break;

    default:
        break;
    }

    // drop error message
    if ( handle->runtime_data.err_display_mode == ICLI_ERR_DISPLAY_MODE_DROP ) {
        return;
    }

    if ( w == NULL ) {
        if ( handle->runtime_data.b_exec_by_api ) {
            if ( handle->runtime_data.app_err_msg ) {
                if ( handle->runtime_data.line_number == ICLI_INVALID_LINE_NUMBER ) {
                    ICLI_EXEC_PUT_NEWLINE;
                }
            }
            // revert the command
            for ( i = 0; i < (u32)(handle->runtime_data.cmd_len); i++ ) {
                if ( ICLI_IS_(EOS, handle->runtime_data.cmd[i]) ) {
                    handle->runtime_data.cmd[i] = ICLI_SPACE;
                }
            }
            icli_session_str_put(handle, handle->runtime_data.cmd);
            ICLI_EXEC_PUT_NEWLINE;
        }
        icli_session_str_put(handle, err_descr);
        ICLI_EXEC_PUT_NEWLINE;
        return;
    }

#if 1 /* CP, 2012/11/07 10:48, Bugzilla#10023 */
    /* check more print */
    if ( handle->runtime_data.more_print ) {
        icli_session_str_put(handle, handle->runtime_data.cmd_copy);
        handle->runtime_data.more_print = FALSE;
        memset(handle->runtime_data.cmd_copy, 0, sizeof(handle->runtime_data.cmd_copy));
        ICLI_EXEC_PUT_NEWLINE;
        return;
    }
#endif

    w_pos = w - handle->runtime_data.cmd_start;

    /* the word position is out of the line or call by api */
    if ( (handle->runtime_data.cmd_len > _CMD_LINE_LEN) ||
         handle->runtime_data.left_scroll               ||
         handle->runtime_data.right_scroll              ||
         handle->runtime_data.b_exec_by_api              ) {
        // revert the command
        for ( i = 0; i < (u32)(handle->runtime_data.cmd_len); i++ ) {
            if ( ICLI_IS_(EOS, handle->runtime_data.cmd[i]) ) {
                handle->runtime_data.cmd[i] = ICLI_SPACE;
            }
        }

        if ( (handle->runtime_data.cmd_len > _CMD_LINE_LEN) ||
             handle->runtime_data.left_scroll               ||
             handle->runtime_data.right_scroll              ||
             (handle->runtime_data.app_err_msg &&
              handle->runtime_data.line_number == ICLI_INVALID_LINE_NUMBER) ) {
            ICLI_EXEC_PUT_NEWLINE;
        }

        for ( i = 0; i < (u32)(handle->runtime_data.cmd_len); i++ ) {
            icli_session_char_put(handle, handle->runtime_data.cmd[i]);
            if ( (i + 1) % (handle->runtime_data.width - 1) == 0 ) {
                ICLI_EXEC_PUT_NEWLINE;
                if ( w_pos < i ) {
                    pos = w_pos % (handle->runtime_data.width - 1);
                    for ( j = 0; j < pos; j++ ) {
                        ICLI_EXEC_PUT_SPACE;
                    }
                    icli_session_char_put(handle, '^');
                    ICLI_EXEC_PUT_NEWLINE;
                    // set large to avoid print again
                    w_pos = 0x0000FFff;
                }
            }
        }
        ICLI_EXEC_PUT_NEWLINE;

        if ( w_pos < i ) {
            pos = w_pos % (handle->runtime_data.width - 1);
            for ( j = 0; j < pos; j++ ) {
                ICLI_EXEC_PUT_SPACE;
            }
            icli_session_char_put(handle, '^');
            ICLI_EXEC_PUT_NEWLINE;
        } else {
            ICLI_EXEC_PUT_NEWLINE;
        }
    } else {
        if ( handle->runtime_data.start_pos ) {
            // 1 is for left scroll
            pos = handle->runtime_data.prompt_len + w_pos - handle->runtime_data.start_pos + 1;
        } else {
            pos = handle->runtime_data.prompt_len + w_pos;
        }

        for ( i = 0; i < pos; i++ ) {
            ICLI_EXEC_PUT_SPACE;
        }
        icli_session_char_put(handle, '^');
        ICLI_EXEC_PUT_NEWLINE;
    }
    icli_session_str_printf(handle, "%% %s word detected at '^' marker.\n", err_descr);

#if 0 /* CP, 2012/11/07 10:48, Bugzilla#10023 */
    /* check more print */
    if ( handle->runtime_data.more_print ) {
        icli_session_str_put(handle, handle->runtime_data.cmd_copy);
        handle->runtime_data.more_print = FALSE;
        memset(handle->runtime_data.cmd_copy, 0, sizeof(handle->runtime_data.cmd_copy));
    }
#endif
    ICLI_EXEC_PUT_NEWLINE;
}

static void _match_para_free(
    IN  icli_session_handle_t   *handle
)
{
    icli_exec_para_list_free( &(handle->runtime_data.match_para) );
    handle->runtime_data.match_para      = NULL;

    handle->runtime_data.exactly_cnt     = 0;
    handle->runtime_data.partial_cnt     = 0;
    handle->runtime_data.total_match_cnt = 0;
}

static void _handle_para_free(
    IN  icli_session_handle_t   *handle
)
{
    icli_exec_para_list_free( &(handle->runtime_data.cmd_var) );

    _match_para_free( handle );

    handle->runtime_data.grep_var   = NULL;
    handle->runtime_data.grep_begin = 0;
}

static void _cmd_var_put(
    IN  icli_session_handle_t   *handle,
    IN  icli_parameter_t        *para,
    IN  BOOL                    b_sort
)
{
    icli_parameter_t    *p,
                        *prev;

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
    if ( handle->runtime_data.b_in_loop ) {
        if ( para->b_in_loop == FALSE ) {
            if ( para->match_node->loop ) {
                para->b_in_loop = TRUE;
            } else {
                handle->runtime_data.b_in_loop = FALSE;
            }
        }
    } else {
        if ( para->b_in_loop ) {
            handle->runtime_data.b_in_loop = TRUE;
        }
    }
#endif

    para->next = NULL;
    if ( handle->runtime_data.cmd_var == NULL ) {
        handle->runtime_data.cmd_var = para;
        return;
    }

    if ( b_sort ) {
        // sort and insert
        p = handle->runtime_data.cmd_var;
        if ( para->match_node->word_id < p->match_node->word_id ) {
            para->next = p;
            handle->runtime_data.cmd_var = para;
        } else {
            for ( prev = p, p = p->next;
                  (p != NULL) && (para->match_node->word_id >= p->match_node->word_id);
                  prev = p, p = p->next ) {
                ;
            }
            // insert
            prev->next = para;
            para->next = p;
        }
    } else {
        // go the last node
        for ( p = handle->runtime_data.cmd_var; p->next; ___NEXT(p) ) {
            ;
        }
#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
        if ( para->b_in_loop == TRUE ) {
            p->b_in_loop = TRUE;
        }
#endif
        // append
        p->next = para;
        para->next = NULL;
    }
}

static void _cmd_var_sort_put(
    IN  icli_session_handle_t   *handle,
    IN  icli_parameter_t        *para
)
{
    icli_parameter_t    *p,
                        *prev;

    para->next = NULL;
    if ( handle->runtime_data.cmd_var == NULL ) {
        handle->runtime_data.cmd_var = para;
        return;
    }

    // sort and insert
    p = handle->runtime_data.cmd_var;
    if ( icli_str_cmp(para->match_node->word, p->match_node->word) == -1 ) {
        para->next = p;
        handle->runtime_data.cmd_var = para;
    } else {
        for ( prev = p, ___NEXT(p);
              (p) && (icli_str_cmp(para->match_node->word, p->match_node->word) == 1);
              prev = p, ___NEXT(p) ) {
            ;
        }
        // insert
        prev->next = para;
        para->next = p;
    }
}

/*
    walk parsing tree by user command

    INPUT
        handle : session handle

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _cmd_walk(
    IN  icli_session_handle_t   *handle
)
{
    i32                     rc;
    char                    *w;
    icli_parsing_node_t     *tree;
    icli_parameter_t        *p,
                            *prev;
    BOOL                    b = TRUE;

#if 1 /* CP_LINE */
    char                    *c;
    i32                     forever_loop = 1;
#endif

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
    icli_parameter_t        *para;
    u32                     i;
    icli_stack_port_range_t *spr;
    u32                     *cnt;
    icli_stack_port_range_t *para_spr;
#endif

    // empty command
    if ( icli_str_len(handle->runtime_data.cmd_walk) == 0 ) {
        return ICLI_RC_ERR_EMPTY;
    }

    // get tree
#if 1
    if ( handle->runtime_data.b_in_parsing ) {
        tree = icli_parsing_tree_get( handle->runtime_data.current_parsing_mode );
    } else {
        tree = icli_parsing_tree_get( handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode );
    }
#else
    tree = icli_parsing_tree_get( handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode );
#endif
    if ( tree == NULL ) {
        _error_display(handle, NULL, "% No available command.\n");
        return ICLI_RC_ERR_EMPTY;
    }

    // execution
    while ( _cmd_word_get(handle, &w) == ICLI_RC_OK ) {

        // check tree
        if ( tree == NULL ) {
            // find the last one
            if ( handle->runtime_data.cmd_var ) {
                for ( p = handle->runtime_data.cmd_var; p->next; ___NEXT(p) ) {
                    ;
                }
                // check if loosely parsing
                if ( p->match_node->execution != NULL           &&
                     p->match_node->execution->cmd_cb != NULL   &&
                     ICLI_CMD_IS_LOOSELY(p->match_node->node_property.cmd_property->property) ) {
                    if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_CMD ) {
                        _match_para_free( handle );
                    } else {
                        _handle_para_free( handle );
                    }
                    return ICLI_RC_ERR_EXCESSIVE;
                }
            }
            if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_TAB ) {
                ICLI_PUT_NEWLINE;
            }
            _error_display(handle, w, "Invalid");
            _handle_para_free( handle );
            return ICLI_RC_ERR_MATCH;
        }

_CMD_WALK_REDO:
        // reset rc
        handle->runtime_data.rc = ICLI_RC_ERR_MATCH;

#if ICLI_RANDOM_OPTIONAL
        // reset random optional
        _random_optional_reset();
#endif

#if 1 /* CP, 2012/10/11 16:09, performance enhancement: do not check same optional nodes */
        memset(g_match_optional_node, 0, sizeof(g_match_optional_node));
#endif

        // find match nodes
        if ( b ) {
            rc = _match_node_find(handle, tree, w, TRUE, TRUE);
            switch ( rc ) {
            case ICLI_RC_OK:
                break;
            case ICLI_RC_ERR_REDO:
                _match_para_free( handle );
                handle->runtime_data.grep_var   = NULL;
                handle->runtime_data.grep_begin = 0;
                goto _CMD_WALK_REDO;
            default:
                _handle_para_free( handle );
                return ICLI_RC_ERROR;
            }
            b = FALSE;
        } else {
#if ICLI_RANDOM_MUST_NUMBER
            /*
                check if random must is enough or not
                if not then can not go to child
             */
            if ( _must_child_allowed(handle, tree) ) {
                rc = _match_node_find(handle, tree->child, w, TRUE, TRUE);
                switch ( rc ) {
                case ICLI_RC_OK:
                    break;
                case ICLI_RC_ERR_REDO:
                    _match_para_free( handle );
                    handle->runtime_data.grep_var   = NULL;
                    handle->runtime_data.grep_begin = 0;
                    goto _CMD_WALK_REDO;
                default:
                    _handle_para_free( handle );
                    return ICLI_RC_ERROR;
                }
            }
#else
            rc = _match_node_find(handle, tree->child, w, TRUE, TRUE);
            switch ( rc ) {
            case ICLI_RC_OK:
                break;
            case ICLI_RC_ERR_REDO:
                _match_para_free( handle );
                handle->runtime_data.grep_var   = NULL;
                handle->runtime_data.grep_begin = 0;
                goto _CMD_WALK_REDO;
            default:
                _handle_para_free( handle );
                return ICLI_RC_ERROR;
            }
#endif

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
            rc = _match_single_node(handle, tree->loop, w, TRUE, FALSE);
            switch ( rc ) {
            case ICLI_RC_OK:
                break;
            case ICLI_RC_ERR_REDO:
                _match_para_free( handle );
                handle->runtime_data.grep_var   = NULL;
                handle->runtime_data.grep_begin = 0;
                goto _CMD_WALK_REDO;
            default:
                _handle_para_free( handle );
                return ICLI_RC_ERROR;
            }
#endif

#if 1 /* CP, 2012/10/11 16:09, performance enhancement: do not check same optional nodes */
            memset(g_match_optional_node, 0, sizeof(g_match_optional_node));
#endif

#if ICLI_RANDOM_OPTIONAL
            rc = _random_optional_match(handle, tree, w, TRUE, FALSE, -1);
            switch ( rc ) {
            case ICLI_RC_OK:
                break;
            case ICLI_RC_ERR_REDO:
                _match_para_free( handle );
                handle->runtime_data.grep_var   = NULL;
                handle->runtime_data.grep_begin = 0;
                goto _CMD_WALK_REDO;
            default:
                _handle_para_free( handle );
                return ICLI_RC_ERROR;
            }
#endif
        }

#if 1 /* CP_LINE */
        /* pre-processing */
        if ( handle->runtime_data.total_match_cnt &&
             handle->runtime_data.exactly_cnt == 0 ) {
            for ( p = handle->runtime_data.match_para; p; ___NEXT(p) ) {
                switch ( p->match_node->type ) {
                case ICLI_VARIABLE_LINE:
                case ICLI_VARIABLE_RANGE_LINE:
                case ICLI_VARIABLE_GREP_STRING:
                    c = handle->runtime_data.cmd_walk;
                    if ( ICLI_NOT_(EOS, *c) || ICLI_NOT_(EOS, *(c + 1)) ) {

                        if ( ICLI_NOT_(EOS, *c) ) {
                            // for exec_cmd
                            // restore previous SPACE to let w
                            // to include the whole remaining string
                            *(c - 1) = ICLI_SPACE;
                        } else {
                            // for TAB and ?
                            // restore previous SPACE to let w
                            // to include the whole remaining string
                            *(c) = ICLI_SPACE;
                        }

                        // go to the end of the string
                        while ( forever_loop == 1 ) {
                            for ( c++; ICLI_NOT_(EOS, *c); c++ ) {
                                ;
                            }
                            if ( ICLI_IS_(EOS, *(c + 1)) ) {
                                // all done
                                break;
                            }
                            // restore previous SPACE to let w
                            // to include the whole remaining string
                            *(c) = ICLI_SPACE;
                        }
                        // set cmd_walk to the end
                        // to tell the command walk is over
                        handle->runtime_data.cmd_walk = c;
                        // update value
                        if ( p->match_node->type == ICLI_VARIABLE_LINE ||
                             p->match_node->type == ICLI_VARIABLE_GREP_STRING ) {
                            c = p->value.u.u_line;
                        } else {
                            c = p->value.u.u_range_line;
                        }
                        (void)icli_str_cpy(c, w);
                    }
                    break;

                default:
                    break;
                }
            }
        }
#endif

        // check the find result
        switch ( handle->runtime_data.total_match_cnt ) {
        case 0 : /* no match */
            /* check rc */
            if ( handle->runtime_data.rc == ICLI_RC_ERR_INCOMPLETE ) {
                if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_TAB ) {
                    ICLI_PUT_NEWLINE;
                }
                _error_display(handle, w, "Incomplete");
            } else {
                if ( handle->runtime_data.cmd_var ) {
                    // find the last one
                    for ( p = handle->runtime_data.cmd_var; p->next; ___NEXT(p) ) {
                        ;
                    }
                    // check if loosely parsing
                    if ( p->match_node->execution != NULL           &&
                         p->match_node->execution->cmd_cb != NULL   &&
                         ICLI_CMD_IS_LOOSELY(p->match_node->node_property.cmd_property->property) ) {
                        if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_CMD ) {
                            _match_para_free( handle );
                        } else {
                            _handle_para_free( handle );
                        }
                        return ICLI_RC_ERR_EXCESSIVE;
                    }
                }
                if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_TAB ) {
                    ICLI_PUT_NEWLINE;
                }
                _error_display(handle, w, "Invalid");
            }
            _handle_para_free( handle );
            return ICLI_RC_ERR_MATCH;

        case 1 : /* one match */
            _cmd_var_put( handle, handle->runtime_data.match_para, FALSE );
#if 1 /* RANDOM_OPTIONAL */
            tree = handle->runtime_data.match_para->match_node;
#else
            tree = handle->runtime_data.match_para->match_node->child;
#endif
            // use this
            handle->runtime_data.match_para = NULL;
            break;

        default : /* multiple match, then find the exact match */
            // pre-process match_para
            if ( handle->runtime_data.exactly_cnt == 0 ) {
                for ( p = handle->runtime_data.match_para; p; ___NEXT(p) ) {
                    switch ( p->match_node->type ) {
                    case ICLI_VARIABLE_WORD:
                    case ICLI_VARIABLE_KWORD:
                    case ICLI_VARIABLE_STRING:
                    case ICLI_VARIABLE_LINE:
                    case ICLI_VARIABLE_RANGE_WORD:
                    case ICLI_VARIABLE_RANGE_KWORD:
                    case ICLI_VARIABLE_RANGE_STRING:
                    case ICLI_VARIABLE_RANGE_LINE:
                    case ICLI_VARIABLE_GREP_STRING:
#if 1 /* CP, 2012/10/08 16:06, <hostname> */
                    case ICLI_VARIABLE_HOSTNAME:
#endif
                        p->match_type = ICLI_MATCH_TYPE_EXACTLY;
                        ++(handle->runtime_data.exactly_cnt);
                        break;

                    default:
                        break;
                    }
                }
            }
#if 1 /* keyword has the highest priority */
            else if ( handle->runtime_data.exactly_cnt > 1 ) {
                u32    keyword_cnt = 0;

                for ( p = handle->runtime_data.match_para; p; ___NEXT(p) ) {
                    if ( p->match_node->type == ICLI_VARIABLE_KEYWORD ) {
                        keyword_cnt++;
                    }
                }

                if ( keyword_cnt == 1 ) {
                    for ( p = handle->runtime_data.match_para; p; ___NEXT(p) ) {
                        if ( p->match_node->type != ICLI_VARIABLE_KEYWORD ) {
                            p->match_type = ICLI_MATCH_TYPE_PARTIAL;
                        }
                    }
                    handle->runtime_data.exactly_cnt = 1;
                }
            }
#endif

            if ( handle->runtime_data.exactly_cnt != 1 ) {
                _handle_para_free( handle );
                if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_TAB ) {
                    ICLI_PUT_NEWLINE;
                }
                _error_display(handle, w, "Ambiguous");
                return ICLI_RC_ERR_AMBIGUOUS;
            }

            // get the exact one
            p = handle->runtime_data.match_para;
            if ( p->match_type == ICLI_MATCH_TYPE_EXACTLY ) {
                handle->runtime_data.match_para = p->next;
            } else {
                for (prev = p, p = p->next; p; prev = p, ___NEXT(p)) {
                    if ( p->match_type == ICLI_MATCH_TYPE_EXACTLY ) {
                        prev->next = p->next;
                        break;
                    }
                }
            }
            _cmd_var_put( handle, p, FALSE );
#if 1 /* RANDOM_OPTIONAL */
            if ( p ) {
                tree = p->match_node;
            } else {
                T_E("p is NULL.\n");
                return ICLI_RC_ERROR;
            }
#else
            tree = p->match_node->child;
#endif
            break;
        }

        _match_para_free( handle );

    }// while

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
    /* integrate loop for execution */
    if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_CMD ) {
        b        = FALSE;
        prev     = NULL;
        para     = NULL;
        para_spr = NULL;
        for ( p = handle->runtime_data.cmd_var; p; ___NEXT(p) ) {
            if ( b ) {
                if ( p->b_in_loop == FALSE ) {
                    b    = FALSE;
                    para = NULL;
                    continue;
                }
                switch ( p->value.type ) {
                case ICLI_VARIABLE_PORT_TYPE:
                    prev = p;
                    break;

                case ICLI_VARIABLE_PORT_TYPE_LIST:
                    // integrate
                    if ( para_spr ) {
                        cnt = &( para_spr->cnt );
                    } else {
                        T_E("para_spr is NULL.\n");
                        return ICLI_RC_ERROR;
                    }
                    spr = &( p->value.u.u_port_type_list );
                    for ( i = 0; i < spr->cnt; i++ ) {
                        if ( (*cnt) >= ICLI_RANGE_LIST_CNT ) {
                            T_W("full switch range\n");
                            break;
                        }
                        para_spr->switch_range[(*cnt)] = spr->switch_range[i];
                        (*cnt)++;
                    }

                    // free prev
                    icli_exec_parameter_free( prev );
                    // free p
                    if ( para ) {
                        para->next = p->next;
                    } else {
                        T_E("para is NULL.\n");
                        return ICLI_RC_ERROR;
                    }
                    icli_exec_parameter_free( p );
                    p = para;
                    break;

                default:
                    T_W("the type, %d, not support loop\n", p->value.type);
                    break;
                }
            } else {
                if ( p->b_in_loop ) {
                    b        = TRUE;
                    para     = p;
                    para_spr = &( p->value.u.u_port_type_list );
                }
            }
        }
    }
#endif

    return ICLI_RC_OK;
}

static void _cmd_buf_cpy(
    IN  icli_session_handle_t   *handle
)
{
    char    *c = handle->runtime_data.cmd_copy,
             *w,
             *prev_w,
             *s;
    i32     len;

    // reset
    memset(c, 0, sizeof(handle->runtime_data.cmd_copy));

    // check length
    len = handle->runtime_data.cmd_len;
    if ( len == 0 ) {
        return;
    }

    // check if all spaces
    //skip space
    ICLI_SPACE_SKIP(w, handle->runtime_data.cmd);

    //EOS
    if ( ICLI_IS_(EOS, (*w)) ) {
        return;
    }

    // copy to cmd_copy
    *c = ICLI_EOS;
    c++;
    (void)icli_str_cpy(c, handle->runtime_data.cmd);
    handle->runtime_data.cmd_walk = c;
    handle->runtime_data.cmd_start = c;

    prev_w = NULL;
    while ( _cmd_word_get(handle, &w) == ICLI_RC_OK ) {
        if ( prev_w ) {
            for ( s = prev_w - 1; ICLI_NOT_(EOS, *s); s-- ) {
                ;
            }
            *s = ICLI_SPACE;
        }
        prev_w = w;
    }

    // make w go to EOS
    for ( w = prev_w; ICLI_NOT_(EOS, *w); w++ ) {
        ;
    }

    // check
    if ( handle->runtime_data.cmd_walk != w ) {
        for ( s = prev_w - 1; ICLI_NOT_(EOS, *s); s-- ) {
            ;
        }
        *s = ICLI_SPACE;
    }
}

#if ICLI_RANDOM_MUST_NUMBER

static void _cr_get(
    IN icli_session_handle_t    *handle,
    IN icli_parameter_t         *prev_para,
    IN icli_parameter_t         *para,
    IN BOOL                     b_add,
    IN BOOL                     b_visible_chk
)
{
    int     d;
    u32     i;

    /* check if random must is enough, otherwise never <cr> */
    for ( i = 0; i < ICLI_RANDOM_MUST_CNT; i++ ) {
        if ( handle->runtime_data.random_must_head[i] ) {
            d = handle->runtime_data.random_must_match[i] - handle->runtime_data.random_must_head[i]->random_must_number;
            if ( d < 0 ) {
                if ( d == -1 ) {
                    if ( b_add ) {
                        if ( para->match_node != handle->runtime_data.random_must_head[i] ) {
                            if ( icli_parsing_random_head(handle->runtime_data.random_must_head[i], para->match_node, NULL) == FALSE ) {
                                return;
                            }
                        }
                    } else {
                        return;
                    }
                } else {
                    return;
                }
            }
        }
    }

    /* get <cr> */
    switch ( para->value.type ) {
    case ICLI_VARIABLE_PORT_ID:
        if ( prev_para && prev_para->value.type == ICLI_VARIABLE_PORT_TYPE ) {
            if ( icli_port_check_type(  para->value.u.u_port_id.switch_id,
                                        para->value.u.u_port_id.begin_port,
                                        prev_para->value.u.u_port_type,
                                        NULL,
                                        NULL) == TRUE ) {
                if ( b_visible_chk ) {
                    if ( _visible_chk(para->match_node) ) {
                        handle->runtime_data.b_cr = TRUE;
                    }
                } else {
                    handle->runtime_data.b_cr = TRUE;
                }
            }
        }
        break;

    case ICLI_VARIABLE_PORT_LIST:
        if ( prev_para && prev_para->value.type == ICLI_VARIABLE_PORT_TYPE ) {
            if ( icli_port_range_check_type(&(para->value.u.u_port_list),
                                            prev_para->value.u.u_port_type,
                                            NULL,
                                            NULL) == TRUE ) {
                if ( b_visible_chk ) {
                    if ( _visible_chk(para->match_node) ) {
                        handle->runtime_data.b_cr = TRUE;
                    }
                } else {
                    handle->runtime_data.b_cr = TRUE;
                }
            }
        }
        break;

    default:
        if ( b_visible_chk ) {
            if ( _visible_chk(para->match_node) ) {
                handle->runtime_data.b_cr = TRUE;
            }
        } else {
            handle->runtime_data.b_cr = TRUE;
        }
        break;
    }
}

#else // ICLI_RANDOM_MUST_NUMBER

#define __ICLI_EXEC_CR_GET(prev_node) \
    switch ( p->value.type ) { \
    case ICLI_VARIABLE_PORT_ID: \
    case ICLI_VARIABLE_PORT_TYPE_ID: \
        if ( prev_node ) { \
            if ( icli_port_check_type(p->value.u.u_port_id.switch_id, \
                                        p->value.u.u_port_id.begin_port, \
                                        prev_node->value.u.u_port_type, \
                                        NULL, \
                                        NULL) == TRUE ) { \
                if ( _visible_chk(p->match_node) ) { \
                    handle->runtime_data.b_cr = TRUE; \
                } \
            } \
        } \
        break; \
    case ICLI_VARIABLE_PORT_LIST: \
    case ICLI_VARIABLE_PORT_TYPE_LIST: \
        if ( prev_node ) { \
            if ( icli_port_range_check_type(&(p->value.u.u_port_list), \
                                              prev_node->value.u.u_port_type, \
                                              NULL, \
                                              NULL) == TRUE ) { \
                if ( _visible_chk(p->match_node) ) { \
                    handle->runtime_data.b_cr = TRUE; \
                } \
            } \
        } \
        break; \
    default: \
        if ( _visible_chk(p->match_node) ) { \
            handle->runtime_data.b_cr = TRUE; \
        } \
        break; \
    }

#endif // ICLI_RANDOM_MUST_NUMBER

/*
    find next nodes for the user command
    the result is stored in cmd_var

    INPUT
        handle : session handle

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _next_node_find(
    IN  icli_session_handle_t   *handle
)
{
    char                    *w,
                            *c;
    i32                     rc;
    icli_match_type_t       match_type;
    icli_parsing_node_t     *tree;
    icli_parsing_node_t     *line_match_node;
    icli_parameter_t        *p = NULL,
                             *n,
                             *prev_node,
                             *last_node;
#if 1 /* RANDOM_OPTIONAL */
    BOOL                    b = FALSE;
#endif

#if ICLI_RANDOM_OPTIONAL
    i32                     b_in_loop;
#endif

    // get command into str buffer for further process
    _cmd_buf_cpy( handle );

    // get first word
    handle->runtime_data.cmd_walk = handle->runtime_data.cmd_copy;

    /* reset last port type */
    handle->runtime_data.last_port_type = ICLI_PORT_TYPE_NONE;

    if ( icli_str_len(handle->runtime_data.cmd_walk) == 0 ) {
        // empty then get first possible node
#if 1
        if ( handle->runtime_data.b_in_parsing ) {
            tree = icli_parsing_tree_get( handle->runtime_data.current_parsing_mode );
        } else {
            tree = icli_parsing_tree_get( handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode );
        }
#else
        tree = icli_parsing_tree_get( handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode );
#endif
    } else {
        // execute command
        rc = _cmd_walk( handle );
        switch ( rc ) {
        case ICLI_RC_ERR_EXCESSIVE:
            handle->runtime_data.b_cr = TRUE;
            return ICLI_RC_OK;

        case ICLI_RC_OK:
            break;

        default:
            return rc;
        }

        // find the last one
        for ( p = handle->runtime_data.cmd_var; p->next; ___NEXT(p) ) {
            ;
        }

        // get tree
#if 1 /* RANDOM_OPTIONAL */
        b = TRUE;
        tree = p->match_node;
#else
        tree = p->match_node->child;
#endif
    }

    // skip space, get the last word
    ICLI_SPACE_SKIP(w, (handle->runtime_data.cmd_walk + 1));
    // find EOS
    for ( c = w; ICLI_NOT_(EOS, *c); c++ ) {
        ;
    }
    // update cmd_walk
    handle->runtime_data.cmd_walk = c;

    /* get last node */
    prev_node = NULL;
    last_node = NULL;
    if ( handle->runtime_data.cmd_var ) {
        for ( p = handle->runtime_data.cmd_var;
              p->next != NULL;
              prev_node = p, p = p->next ) {
            ;
        }

        if ( p->value.type == ICLI_VARIABLE_PORT_TYPE ) {
            last_node = p;
            if ( prev_node ) {
                prev_node->next = NULL;
            } else {
                handle->runtime_data.cmd_var = NULL;
            }
            handle->runtime_data.last_port_type = p->value.u.u_port_type;
        } else {
            handle->runtime_data.last_port_type = ICLI_PORT_TYPE_NONE;
        }

        if (prev_node && prev_node->value.type != ICLI_VARIABLE_PORT_TYPE) {
            prev_node = NULL;
        }
    }

#if ICLI_RANDOM_MUST_NUMBER
    if ( _session_random_must_get(handle) == FALSE ) {
        // free all parameters
        _handle_para_free( handle );
        if ( last_node ) {
            icli_exec_parameter_free( last_node );
        }
        return ICLI_RC_ERROR;
    }
#endif

    /* check b_cr */
    handle->runtime_data.b_cr = FALSE;
    line_match_node = NULL;
    if ( ICLI_IS_(EOS, *w) && handle->runtime_data.cmd_var ) {
        // check if executable
        if ( p && p->match_node->execution && p->match_node->execution->cmd_cb ) {
#if ICLI_RANDOM_MUST_NUMBER
            _cr_get(handle, prev_node, p, FALSE, TRUE);
#else
            __ICLI_EXEC_CR_GET(prev_node);
#endif
            // get line_match_node
            switch ( p->value.type ) {
            case ICLI_VARIABLE_LINE:
            case ICLI_VARIABLE_RANGE_LINE:
            case ICLI_VARIABLE_GREP_STRING:
#if 1 /* CP, 2012/08/30 09:12, do command by CLI command */
                if ( _visible_chk(p->match_node) ) {
                    line_match_node = p->match_node;
                }
#else
                // find last non-EOS
                for ( c = w; ICLI_IS_(EOS, *c); c-- ) {
                    ;
                }

                if ( ICLI_NOT_(STRING_END, *c) ) {
                    line_match_node = p->match_node;
                }
#endif
                break;
            default:
                break;
            }
        }
    }

    /* free all parameters first */
#if 1 /* RANDOM_OPTIONAL */
    _match_para_free( handle );
    handle->runtime_data.grep_var   = NULL;
    handle->runtime_data.grep_begin = 0;
#else
    _handle_para_free( handle );
#endif

#if 1 /* CP, 2012/10/11 16:09, performance enhancement: do not check same optional nodes */
    memset(g_match_optional_node, 0, sizeof(g_match_optional_node));
#endif

    // find next possible nodes
#if 1 /* RANDOM_OPTIONAL */
    if ( b ) {

#if ICLI_RANDOM_OPTIONAL
        // reset random optional
        _random_optional_reset();
#endif

#if ICLI_RANDOM_MUST_NUMBER
        /*
            check if random must is enough or not
            if not then can not go to child
         */
        if ( _must_child_allowed(handle, tree) ) {
            // find match node
            rc = _match_node_find(handle, tree->child, "", FALSE, TRUE);
            switch ( rc ) {
            case ICLI_RC_ERR_REDO:
            default:
                break;
            }
        }
#else
        // find match node
        rc = _match_node_find(handle, tree->child, "", FALSE, TRUE);
        switch ( rc ) {
        case ICLI_RC_ERR_REDO:
        default:
            break;
        }
#endif

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
        rc = _match_single_node(handle, tree->loop, "", FALSE, FALSE);
        switch ( rc ) {
        case ICLI_RC_ERR_REDO:
        default:
            break;
        }
#endif

#if 1 /* CP, 2012/10/11 16:09, performance enhancement: do not check same optional nodes */
        memset(g_match_optional_node, 0, sizeof(g_match_optional_node));
#endif

        // find in random optional
#if ICLI_RANDOM_OPTIONAL

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
        b_in_loop = handle->runtime_data.b_in_loop;
        handle->runtime_data.b_in_loop = FALSE;
#endif

        rc = _random_optional_match(handle, tree, "", FALSE, FALSE, -1);

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
        handle->runtime_data.b_in_loop = b_in_loop;
#endif
        switch ( rc ) {
        case ICLI_RC_OK:
            break;

        case ICLI_RC_ERR_REDO:
            if ( last_node ) {
                icli_exec_parameter_free( last_node );
            }
            return ICLI_RC_ERR_REDO;

        default:
            _handle_para_free( handle );
            if ( last_node ) {
                icli_exec_parameter_free( last_node );
            }
            return ICLI_RC_ERROR;
        }
#endif // ICLI_RANDOM_OPTIONAL

    } else {
        rc = _match_node_find(handle, tree, "", FALSE, TRUE);
        switch ( rc ) {
        case ICLI_RC_ERR_REDO:
        default:
            break;
        }
    }

    if ( *w &&
         handle->runtime_data.cmd_var &&
         handle->runtime_data.match_para == NULL  ) {
        // find the last one
        for ( p = handle->runtime_data.cmd_var; p->next; ___NEXT(p) ) {
            ;
        }
        // check if loosely parsing
        if ( p->match_node->execution != NULL           &&
             p->match_node->execution->cmd_cb != NULL   &&
             ICLI_CMD_IS_LOOSELY(p->match_node->node_property.cmd_property->property) ) {
            _handle_para_free( handle );
            handle->runtime_data.b_cr = TRUE;
            return ICLI_RC_OK;
        }
    }
#else
    (void)_match_node_find(handle, tree, "", FALSE, TRUE);
#endif

    /* clear cmd_var list for prepare the match list */
    icli_exec_para_list_free( &(handle->runtime_data.cmd_var) );

    //check the last word
    handle->runtime_data.exactly_cnt     = 0;
    handle->runtime_data.partial_cnt     = 0;
    handle->runtime_data.total_match_cnt = 0;
    for ( p = handle->runtime_data.match_para; p != NULL; p = n ) {
        n = p->next;
        match_type = _node_match(handle, p->match_node, w, FALSE, &p);
        switch ( match_type ) {
        case ICLI_MATCH_TYPE_REDO:
            /*
                if parsing ICLI_VARIABLE_LINE, then cmd word is re-aligned.
                So, redo the match for all possible next nodes.
            */
            return ICLI_RC_ERR_REDO;

        case ICLI_MATCH_TYPE_EXACTLY : /* exactly match */
            (handle->runtime_data.exactly_cnt)++;

            // fall through

        case ICLI_MATCH_TYPE_PARTIAL : /* partial match for KEYWORD */
            (handle->runtime_data.total_match_cnt)++;
            if ( match_type == ICLI_MATCH_TYPE_PARTIAL ) {
                (handle->runtime_data.partial_cnt)++;
            }

            // fall through

        case ICLI_MATCH_TYPE_INCOMPLETE : /* partial match for variables and can not count into CR */
            _cmd_var_sort_put(handle, p);
            break;

        case ICLI_MATCH_TYPE_ERR : /* not match */
            icli_exec_parameter_free( p );
            break;

        default : /* error, invalid match type */
            T_E("invalid match type(%d)\n", match_type);
            // free all parameters
            _handle_para_free( handle );
            if ( last_node ) {
                icli_exec_parameter_free( last_node );
            }
            return ICLI_RC_ERROR;
        }// switch
    }// for

    // reset match_para
    handle->runtime_data.match_para = NULL;

    if ( *w ) {
        if ( handle->runtime_data.cmd_var == NULL ) {
            if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_TAB ) {
                ICLI_PUT_NEWLINE;
            }
            _error_display(handle, w, "Invalid");
            if ( last_node ) {
                icli_exec_parameter_free( last_node );
            }
            return ICLI_RC_ERR_MATCH;
        }

        /* check b_cr */
        if ( handle->runtime_data.b_cr == FALSE && handle->runtime_data.cmd_var ) {
            switch (handle->runtime_data.total_match_cnt) {
            case 0:
                break;

            case 1: /* only one match */
                p = handle->runtime_data.cmd_var;
#if ICLI_RANDOM_MUST_NUMBER
                if ( p->match_type != ICLI_MATCH_TYPE_INCOMPLETE && p->match_node->execution && p->match_node->execution->cmd_cb ) {
                    _cr_get(handle, last_node, p, TRUE, TRUE);
                }
#else
                for ( p = handle->runtime_data.cmd_var; p; ___NEXT(p) ) {
                    if ( p->match_type != ICLI_MATCH_TYPE_INCOMPLETE && p->match_node->execution != NULL && p->match_node->execution->cmd_cb != NULL ) {
                        __ICLI_EXEC_CR_GET(last_node);
                    }
                }
#endif
                break;

            default: /* multiple matches */
                // pre-process match_para
                if ( handle->runtime_data.exactly_cnt == 0 ) {
                    for ( p = handle->runtime_data.cmd_var; p; ___NEXT(p) ) {
                        switch ( p->match_node->type ) {
                        case ICLI_VARIABLE_WORD:
                        case ICLI_VARIABLE_KWORD:
                        case ICLI_VARIABLE_STRING:
                        case ICLI_VARIABLE_LINE:
                        case ICLI_VARIABLE_RANGE_WORD:
                        case ICLI_VARIABLE_RANGE_KWORD:
                        case ICLI_VARIABLE_RANGE_STRING:
                        case ICLI_VARIABLE_RANGE_LINE:
                        case ICLI_VARIABLE_GREP_STRING:
#if 1 /* CP, 2012/10/08 16:06, <hostname> */
                        case ICLI_VARIABLE_HOSTNAME:
#endif
                            p->match_type = ICLI_MATCH_TYPE_EXACTLY;
                            ++(handle->runtime_data.exactly_cnt);
                            break;

                        default:
                            break;
                        }
                    }
                }

                if ( handle->runtime_data.exactly_cnt != 1 ) {
                    break;
                }
                for ( p = handle->runtime_data.cmd_var; p; ___NEXT(p) ) {
                    if ( p->match_type == ICLI_MATCH_TYPE_EXACTLY && p->match_node->execution && p->match_node->execution->cmd_cb ) {
#if ICLI_RANDOM_MUST_NUMBER
                        _cr_get(handle, last_node, p, TRUE, TRUE);
#else
                        if ( _visible_chk(p->match_node) ) {
                            handle->runtime_data.b_cr = TRUE;
                        }
#endif
                    }
                }
                break;
            }
        }
    }

    // put in cmd_var
    if ( line_match_node ) {
        p = icli_exec_parameter_get();
        if ( p ) {
            p->match_node = line_match_node;
            _cmd_var_sort_put(handle, p);
        } else {
            T_E("memory insufficient\n");
        }
    }

    if ( last_node ) {
        icli_exec_parameter_free( last_node );
    }

    return ICLI_RC_OK;
}

/*
    execute user command

    INPUT
        handle : session handle

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _exec_cmd(
    IN  icli_session_handle_t   *handle
)
{
    i32                     rc;
    icli_parameter_t        *p, *q;

    icli_cmd_cb_t           *cmd_cb;
    u32                     session_id;
    icli_parameter_t        *mode_var;
    icli_parameter_t        *cmd_var;
    i32                     usr_opt;
    char                    *c;

    if ( handle == NULL ) {
        T_E("handle == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

    if ( handle->runtime_data.cmd_len == 0 ) {
        return ICLI_RC_ERR_EMPTY;
    }

    /* check if cmd is all SPACE */
    for (c = handle->runtime_data.cmd; ICLI_IS_(SPACE, *c); c++) {
        ;
    }
    if ( ICLI_IS_(EOS, *c) ) {
        return ICLI_RC_ERR_EMPTY;
    }

    // execute command
    handle->runtime_data.cmd_walk = handle->runtime_data.cmd;
    handle->runtime_data.cmd_start = handle->runtime_data.cmd;

    rc = _cmd_walk( handle );
    switch ( rc ) {
    case ICLI_RC_ERR_EXCESSIVE:
    case ICLI_RC_OK:
        break;

    default:
        return rc;
    }

    // find the last one
    for ( q = NULL, p = handle->runtime_data.cmd_var; p->next; q = p, ___NEXT(p) ) {
        ;
    }

    // check if executable
    if ( p->match_node->execution == NULL || p->match_node->execution->cmd_cb == NULL ) {
        _handle_para_free( handle );
#if 0 /* CP, 2012/10/23 17:35, why need this? */
        if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_TAB ) {
            ICLI_PUT_NEWLINE;
        }
#endif
        _error_display(handle, NULL, "% Incomplete command.\n");
        return ICLI_RC_ERR_INCOMPLETE;
    }

#if ICLI_RANDOM_MUST_NUMBER
    if ( _session_random_must_get(handle) == FALSE ) {
        // free all parameters
        _handle_para_free( handle );
        return ICLI_RC_ERROR;
    }

    /* check if the number of random must is enough */
    handle->runtime_data.b_cr = FALSE;
    _cr_get(handle, q, p, FALSE, FALSE);

    if ( handle->runtime_data.b_cr == FALSE ) {
        _handle_para_free( handle );
        _error_display(handle, NULL, "% Incomplete command.\n");
        return ICLI_RC_ERR_INCOMPLETE;
    }
#endif

    /* get grep */
    handle->runtime_data.grep_var   = NULL;
    handle->runtime_data.grep_begin = 0;
    //if ( ICLI_CMD_IS_GREP(p->match_node->node_property.cmd_property->property) ) {
    if ( p->value.type == ICLI_VARIABLE_GREP_STRING ) {
        for ( q = handle->runtime_data.cmd_var; q; ___NEXT(q) ) {
            if ( q->value.type == ICLI_VARIABLE_GREP ) {
                handle->runtime_data.grep_var = q;
                break;
            }
        }
    }

    /* execute command */
#if 1
    switch ( handle->runtime_data.exec_type ) {
    case ICLI_EXEC_TYPE_CMD:
        // prepare command callback
        cmd_cb     = p->match_node->execution->cmd_cb;
        session_id = handle->session_id;
        mode_var   = handle->runtime_data.mode_para[handle->runtime_data.mode_level].cmd_var;
        cmd_var    = handle->runtime_data.cmd_var;
        usr_opt    = p->match_node->execution->usr_opt;

        /* set b_in_exec_cb for GREP */
        handle->runtime_data.b_in_exec_cb = TRUE;

        // give semaphore
        _SEMA_GIVE();

        // execute command callback
        (void)(*cmd_cb)(session_id, mode_var, cmd_var, usr_opt);

        // take semaphore
        _SEMA_TAKE();

        /* clear b_in_exec_cb */
        handle->runtime_data.b_in_exec_cb = FALSE;
        break;

    case ICLI_EXEC_TYPE_PARSING:
        if ( handle->runtime_data.b_in_parsing ) {
            if ( ICLI_CMD_IS_MODE(p->match_node->node_property.cmd_property->property) ) {
                handle->runtime_data.current_parsing_mode =
                    p->match_node->node_property.cmd_property->goto_mode;
            }
        }
        break;

    default:
        break;
    }
#else
    if ( handle->runtime_data.exec_type == ICLI_EXEC_TYPE_CMD ) {
        // prepare command callback
        cmd_cb     = p->match_node->execution->cmd_cb;
        session_id = handle->session_id;
        mode_var   = handle->runtime_data.mode_para[handle->runtime_data.mode_level].cmd_var;
        cmd_var    = handle->runtime_data.cmd_var;
        usr_opt    = p->match_node->execution->usr_opt;

        /* set b_in_exec_cb for GREP */
        handle->runtime_data.b_in_exec_cb = TRUE;

        // give semaphore
        _SEMA_GIVE();

        // execute command callback
        (*cmd_cb)(session_id, mode_var, cmd_var, usr_opt);

        // take semaphore
        _SEMA_TAKE();

        /* clear b_in_exec_cb */
        handle->runtime_data.b_in_exec_cb = FALSE;
    }
#endif

    // free all parameters
    _handle_para_free( handle );

    return ICLI_RC_OK;
}

/*
    execute TAB

    INPUT
        handle : session handle

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _exec_tab(
    IN  icli_session_handle_t   *handle
)
{
    i32     rc;

    //find possible nodes in cmd_var
    rc = _next_node_find( handle );
    return rc;
}

/*
    execute ?, Question

    INPUT
        handle : session handle

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
static i32 _exec_question(
    IN  icli_session_handle_t   *handle
)
{
    i32     rc;

    //find possible nodes in cmd_var
    rc = _next_node_find( handle );
    return rc;
}

#if 1 /* CP, 2012/08/17 15:34, try global config mode when interface mode fails */
/*
    display message in error buffer

    INPUT
        handle : session handle

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
static void _exec_err_display(
    IN  icli_session_handle_t   *handle
)
{
    if ( handle->runtime_data.err_buf[0] != ICLI_EOS ) {
        handle->runtime_data.put_str = handle->runtime_data.err_buf;
        (void)icli_sutil_usr_str_put( handle );
        handle->runtime_data.put_str = NULL;
        handle->runtime_data.err_buf[0] = ICLI_EOS;
    }
}
#endif

/*
******************************************************************************

    Public Function

******************************************************************************
*/
/*
    Initialization

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_exec_init(void)
{
    i32     i;

    g_para_cnt = 0;
    memset(g_parameter, 0, sizeof(g_parameter));
    for ( i = 0; i < (ICLI_PARAMETER_MEM_CNT - 1); i++ ) {
        g_parameter[i].next = &(g_parameter[i + 1]);
        g_para_cnt++;
    }
    g_para_cnt++;
    g_para_free = g_parameter;

#if 1 /* CP, 2012/10/11 16:09, performance enhancement: do not check same optional nodes */
    memset(g_match_optional_node, 0, sizeof(g_match_optional_node));
#endif

    return ICLI_RC_OK;
}

/*
    execute user command

    INPUT
        handle : session handle

    OUTPUT
        n/a

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 icli_exec(
    IN  icli_session_handle_t   *handle
)
{
    i32     rc = ICLI_RC_OK;
#if 1 /* CP, 2012/08/17 09:38, accept space at beginning */
    char    *c;
#endif

#if 1 /* CP, 2012/08/17 15:34, try global config mode when interface mode fails */
    i32                         original_mode_level;
    i32                         level;
    icli_err_display_mode_t     original_err_display_mode;
    i32                         i;
    icli_cmd_mode_t             original_parsing_mode;
    i32                         original_exec_by_api;
#endif

    if ( handle == NULL ) {
        T_E("handle == NULL\n");
        return ICLI_RC_ERR_PARAMETER;
    }

#if 1 /* CP, 2012/08/17 09:38, accept space at beginning */
    //skip space
    ICLI_SPACE_SKIP(c, handle->runtime_data.cmd);

    // comment
    if ( ICLI_IS_COMMENT(*c) ) {
        return ICLI_RC_OK;
    }
#else
    /* comment */
    if ( ICLI_IS_COMMENT(handle->runtime_data.cmd[0]) ) {
        return ICLI_RC_OK;
    }
#endif

    /* continuous TABs */
    if ( handle->runtime_data.tab_cnt ) {
        return ICLI_RC_OK;
    }

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
    handle->runtime_data.b_in_loop = FALSE;
#endif

    switch ( handle->runtime_data.exec_type ) {
#if 1 /* CP, 2012/08/17 15:34, try global config mode when interface mode fails */
    case ICLI_EXEC_TYPE_CMD:
#if 1 /* CP, 2012/08/30 07:43, for do command */
        if ( handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode <= ICLI_CMD_MODE_GLOBAL_CONFIG ) {
#else
        if ( handle->runtime_data.mode_level < 2 ) {
#endif
            /* in Exec and Global config mode */
            rc = _exec_cmd( handle );
        } else {
            /* in Interface mode */
            // store original error display mode
            original_err_display_mode = handle->runtime_data.err_display_mode;
            // buffer error message first if failed, but if drop then just drop
            if ( original_err_display_mode == ICLI_ERR_DISPLAY_MODE_PRINT ) {
                handle->runtime_data.err_display_mode = ICLI_ERR_DISPLAY_MODE_ERR_BUFFER;
            }
            // empty error buffer
            handle->runtime_data.err_buf[0] = 0;
            // execute command in interface mode
            rc = _exec_cmd( handle );
            // fail ?
            if ( rc != ICLI_RC_OK ) {
                // store original mode level
                original_mode_level = handle->runtime_data.mode_level;
                // go to Global config mode
                handle->runtime_data.mode_level = 1;
                // try Global config mode silently
                handle->runtime_data.err_display_mode = ICLI_ERR_DISPLAY_MODE_DROP;
                // store original exec_by_api
                original_exec_by_api = handle->runtime_data.b_exec_by_api;
                // trigger by ICLI egnine
                handle->runtime_data.b_exec_by_api = FALSE;
                // recover cmd
                c = handle->runtime_data.cmd;
                for ( i = 0; i < handle->runtime_data.cmd_len; i++, c++ ) {
                    if ( ICLI_IS_(EOS, (*c)) ) {
                        *c = ICLI_SPACE;
                    }
                }
                // execute command in Global interface mode
                rc = _exec_cmd( handle );
                // restore original exec_by_api
                handle->runtime_data.b_exec_by_api = original_exec_by_api;
                // check exec result by global config mode
                if ( rc == ICLI_RC_OK ) {
                    // automatically go to Global config mode
                    // free Interface mode parameter
                    for ( level = 2; level <= original_mode_level; level++ ) {
                        icli_exec_para_list_free( &(handle->runtime_data.mode_para[level].cmd_var) );
                    }
                } else {
                    // display previous error message in Interface mode
                    _exec_err_display( handle );
                    // restore mode level
                    handle->runtime_data.mode_level = original_mode_level;
                }
            }
            // restore original error display mode
            handle->runtime_data.err_display_mode = original_err_display_mode;
        }
        break;

    case ICLI_EXEC_TYPE_PARSING:
        if ( handle->runtime_data.b_in_parsing ) {
            if ( handle->runtime_data.current_parsing_mode > ICLI_CMD_MODE_GLOBAL_CONFIG ) {
                original_mode_level = 2;
            } else {
                original_mode_level = 1;
            }
        } else {
            original_mode_level = handle->runtime_data.mode_level;
        }
        if ( original_mode_level < 2 ) {
            /* in Exec and Global config mode */
            rc = _exec_cmd( handle );
        } else {
            /* in Interface mode */
            // store original error display mode
            original_err_display_mode = handle->runtime_data.err_display_mode;
            // buffer error message first if failed, but if drop then just drop
            if ( original_err_display_mode == ICLI_ERR_DISPLAY_MODE_PRINT ) {
                handle->runtime_data.err_display_mode = ICLI_ERR_DISPLAY_MODE_ERR_BUFFER;
            }
            // empty error buffer
            handle->runtime_data.err_buf[0] = 0;
            // execute command in interface mode
            rc = _exec_cmd( handle );
            // fail ?
            if ( rc != ICLI_RC_OK ) {
                original_parsing_mode = ICLI_CMD_MODE_GLOBAL_CONFIG;
                if ( handle->runtime_data.b_in_parsing ) {
                    original_parsing_mode = handle->runtime_data.current_parsing_mode;
                    handle->runtime_data.current_parsing_mode = ICLI_CMD_MODE_GLOBAL_CONFIG;
                } else {
                    // store original mode level
                    original_mode_level = handle->runtime_data.mode_level;
                    // go to Global config mode
                    handle->runtime_data.mode_level = 1;
                }
                // try Global config mode silently
                handle->runtime_data.err_display_mode = ICLI_ERR_DISPLAY_MODE_DROP;
                // store original exec_by_api
                original_exec_by_api = handle->runtime_data.b_exec_by_api;
                // trigger by ICLI egnine
                handle->runtime_data.b_exec_by_api = FALSE;
                // recover cmd
                c = handle->runtime_data.cmd;
                for ( i = 0; i < handle->runtime_data.cmd_len; i++, c++ ) {
                    if ( ICLI_IS_(EOS, (*c)) ) {
                        *c = ICLI_SPACE;
                    }
                }
                // execute command in Global interface mode
                rc = _exec_cmd( handle );
                // restore original exec_by_api
                handle->runtime_data.b_exec_by_api = original_exec_by_api;
                // check exec result by global config mode
                if ( rc == ICLI_RC_OK ) {
                    // automatically go to Global config mode
                    if ( handle->runtime_data.b_in_parsing ) {
                        // already in Global config mode
                    } else {
                        // parsing only, not exec, so does not need to change mode
                    }
                } else {
                    // display previous error message in Interface mode
                    _exec_err_display( handle );
                    // restore mode level
                    if ( handle->runtime_data.b_in_parsing ) {
                        handle->runtime_data.current_parsing_mode = original_parsing_mode;
                    } else {
                        handle->runtime_data.mode_level = original_mode_level;
                    }
                }
            }
            // restore original error display mode
            handle->runtime_data.err_display_mode = original_err_display_mode;
        }
        break;
#else
    case ICLI_EXEC_TYPE_CMD:
    case ICLI_EXEC_TYPE_PARSING:
        rc = _exec_cmd( handle );
        break;
#endif

    case ICLI_EXEC_TYPE_TAB:
#if 0 /* CP, 2012/08/30 09:12, do command by CLI command */
        // if do command, do nothing
        if ( handle->runtime_data.cmd_do ) {
            break;
        }
#endif
        rc = _exec_tab( handle );
        break;

    case ICLI_EXEC_TYPE_QUESTION:
#if 0 /* CP, 2012/08/30 09:12, do command by CLI command */
        // if do command, do nothing
        if ( handle->runtime_data.cmd_do ) {
            break;
        }
#endif
        rc = _exec_question( handle );
        break;

    default:
        rc = ICLI_RC_ERR_PARAMETER;
        break;
    }

#if 1 /* CP, 2012/09/07 11:52, <port_type_list> */
    handle->runtime_data.b_in_loop = FALSE;
#endif

#if 1 /* CP, 2012/10/11 16:09, performance enhancement: do not check same optional nodes */
    memset(g_match_optional_node, 0, sizeof(g_match_optional_node));
#endif

    return rc;
}

/*
    get memory for icli_parameter_t

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        not NULL : successful, icli_parameter_t *
        NULL     : failed

    COMMENT
        n/a
*/
icli_parameter_t *icli_exec_parameter_get(void)
{
    icli_parameter_t    *p;

    if ( g_para_free == NULL ) {
        T_E("g_para_free == NULL\n");
        return NULL;
    }

    if ( g_para_cnt == 0 ) {
        T_E("g_para_cnt == 0\n");
        return NULL;
    }

    p = g_para_free;
    g_para_free = g_para_free->next;
    g_para_cnt--;
    memset(p, 0, sizeof(icli_parameter_t));
    return p;
}

/*
    free memory for icli_parameter_t

    INPUT
        p - parameter for free

    OUTPUT
        n/a

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_exec_parameter_free(
    IN icli_parameter_t    *p
)
{
    if ( p == NULL ) {
        return;
    }

    if ( g_para_free ) {
        p->next = g_para_free;
    } else {
        p->next = NULL;
    }
    g_para_free = p;
    g_para_cnt++;
}

/*
    free parameter list

    INPUT
        *para_list - parameter list to be free

    OUTPUT
        *para_list - reset to NULL

    RETURN
        n/a

    COMMENT
        n/a
*/
void icli_exec_para_list_free(
    INOUT icli_parameter_t  **para_list
)
{
    icli_parameter_t    *p, *n;

    if (para_list == NULL ) {
        return;
    }

    for ( p = *para_list; p != NULL; p = n ) {
        n = p->next;
        icli_exec_parameter_free( p );
    }
    *para_list = NULL;
}

