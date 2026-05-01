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
#ifndef __ICLI_PORTING_API_H__     
#define __ICLI_PORTING_API_H__
//****************************************************************************

/*
******************************************************************************

    Constant
    
******************************************************************************
*/
#define ICLI_PORTING_STR_LEN        63

/*
******************************************************************************

    Type
    
******************************************************************************
*/
typedef struct {
    icli_cmd_mode_t     mode;
    char                str[ICLI_PORTING_STR_LEN + 1];
    char                desc[ICLI_PORTING_STR_LEN + 1];
    char                cmd[ICLI_PORTING_STR_LEN + 1];
    BOOL                b_mode_var;
} icli_porting_cmd_mode_info_t;

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
);

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
);

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
);

/*
    get the time elapsed from system start in milli-seconds
*/
u32 icli_porting_current_time_get(void);

//****************************************************************************
#endif //__ICLI_PORTING_API_H__

