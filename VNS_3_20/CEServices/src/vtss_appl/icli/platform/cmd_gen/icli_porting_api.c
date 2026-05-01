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
    > CP.Wang, 2011/09/16 10:16
        - create
    
******************************************************************************
*/

/*
******************************************************************************

    Include File
    
******************************************************************************
*/
#include <time.h>
#include <sys/time.h>

#include "icli.h"
#include "icli_porting_api.h"

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
/* modify as icli_cmd_mode_t */
static const icli_porting_cmd_mode_info_t g_cmd_mode_info[ICLI_CMD_MODE_MAX] = {
    {
        ICLI_CMD_MODE_EXEC,
        "ICLI_CMD_MODE_EXEC",
        "User EXEC Mode",
        "",
        FALSE
    },
    {
        ICLI_CMD_MODE_GLOBAL_CONFIG,
        "ICLI_CMD_MODE_GLOBAL_CONFIG",
        "Global Configuration Mode",
        "configure terminal",
        FALSE
    },
    {
        ICLI_CMD_MODE_CONFIG_VLAN,
        "ICLI_CMD_MODE_CONFIG_VLAN",
        "VLAN Configuration Mode",
        "vlan <vlan_list>",
        TRUE
    },
    {
        ICLI_CMD_MODE_INTERFACE_PORT_LIST,
        "ICLI_CMD_MODE_INTERFACE_PORT_LIST",
        "Port List Interface Mode",
        "interface <port_type> <port_list>",
        TRUE
    },
    {
        ICLI_CMD_MODE_INTERFACE_VLAN,
        "ICLI_CMD_MODE_INTERFACE_VLAN",
        "VLAN Interface Mode",
        "interface vlan <vlan_list>",
        TRUE
    },
    {
        ICLI_CMD_MODE_CONFIG_LINE,
        "ICLI_CMD_MODE_CONFIG_LINE",
        "Line Configuration Mode",
        "line { <0~16> | console 0 | vty <0~15> }",
        TRUE
    },
    {
        ICLI_CMD_MODE_IPMC_PROFILE,
        "ICLI_CMD_MODE_IPMC_PROFILE",
        "IPMC Profile Mode",
        "ipmc profile <word16>",
        TRUE
    },
    {
        ICLI_CMD_MODE_SNMPS_HOST,
        "ICLI_CMD_MODE_SNMPS_HOST",
        "SNMP Server Host Mode",
        "snmp-server host <word32>",
        TRUE
    },
    {
        ICLI_CMD_MODE_STP_AGGR,
        "ICLI_CMD_MODE_STP_AGGR",
        "STP Aggregation Mode",
        "spanning-tree aggregation",
        FALSE
    },
};

/* modify as icli_privilege_t */
static const char *g_privilege_str[] = {
    "ICLI_PRIVILEGE_0",
    "ICLI_PRIVILEGE_1",
    "ICLI_PRIVILEGE_2",
    "ICLI_PRIVILEGE_3",
    "ICLI_PRIVILEGE_4",
    "ICLI_PRIVILEGE_5",
    "ICLI_PRIVILEGE_6",
    "ICLI_PRIVILEGE_7",
    "ICLI_PRIVILEGE_8",
    "ICLI_PRIVILEGE_9",
    "ICLI_PRIVILEGE_10",
    "ICLI_PRIVILEGE_11",
    "ICLI_PRIVILEGE_12",
    "ICLI_PRIVILEGE_13",
    "ICLI_PRIVILEGE_14",
    "ICLI_PRIVILEGE_15",
    "ICLI_PRIVILEGE_DEBUG",
};

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
/*
    get command mode info by string
    
    INPUT
        mode_str : string of mode
    
    OUTPUT
        n/a
    
    RETURN
        icli_porting_cmd_mode_info_t * : successful
        NULL                           : failed
        
    COMMENT
        n/a
*/
const icli_porting_cmd_mode_info_t *icli_porting_cmd_mode_info_get_by_str(
    IN  char    *mode_str
)
{
    icli_cmd_mode_t     mode;
    
    for (mode = 0; mode < ICLI_CMD_MODE_MAX; mode++ ) {
        if ( icli_str_cmp(mode_str, g_cmd_mode_info[mode].str) == 0 ) {
            break;
        }
    }
    
    if ( mode == ICLI_CMD_MODE_MAX ) {
        return NULL;
    }
    
    return &g_cmd_mode_info[mode];
}

/*
    get command mode info by command mode
    
    INPUT
        mode : command mode
    
    OUTPUT
        n/a
    
    RETURN
        icli_porting_cmd_mode_info_t * : successful
        NULL                           : failed
        
    COMMENT
        n/a
*/
const icli_porting_cmd_mode_info_t *icli_porting_cmd_mode_info_get_by_mode(
    IN  icli_cmd_mode_t     mode
)
{
    if ( mode >= ICLI_CMD_MODE_MAX ) {
        return NULL;
    }
    return &g_cmd_mode_info[mode];
}

/*
    get privilege by string
    
    INPUT
        priv_str : string of privilege
    
    OUTPUT
        n/a
    
    RETURN
        icli_privilege_t   : successful
        ICLI_PRIVILEGE_MAX : failed
        
    COMMENT
        n/a
*/
icli_privilege_t icli_porting_privilege_get(
    IN char     *priv_str
)
{
    i32     priv;
    
    if ( icli_str_to_int(priv_str, &priv) == 0 ) {
        if ( priv >= 0 && priv < ICLI_PRIVILEGE_MAX ) {
            return priv;
        }
        return ICLI_PRIVILEGE_MAX;
    }
   
    for (priv = 0; priv < ICLI_PRIVILEGE_MAX; priv++ ) {
        if ( icli_str_cmp(priv_str, g_privilege_str[priv]) == 0 ) {
            break;
        }
    }
    return priv;
}

/*
    get the time elapsed from system start in milli-seconds
*/
u32 icli_porting_current_time_get(void)
{
    struct timespec     tp;
    
    if ( clock_gettime(CLOCK_MONOTONIC, &tp) == -1 ) {
        T_E("failed to get system up time\n");
        return 0;
    }
    return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
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
    if ( size == 0 ) {
        return NULL;
    }

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
void icli_free(
    IN void     *mem
)
{
    if ( mem ) {
        free( mem );
    }
}
