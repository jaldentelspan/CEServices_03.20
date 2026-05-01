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
    > CP.Wang, 2011/05/10 14:39
        - create

******************************************************************************
*/
#ifndef __ICLI_PORTING_H__
#define __ICLI_PORTING_H__
//****************************************************************************
/*
******************************************************************************

    Include File

******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef ___LINUX___

#include "main.h"

#include <vtss_trace_api.h>

/* For debugging */
#if 1
#include <cyg/infra/diag.h>
#if 1
#define ICLI_DEBUG(args) { diag_printf("%s line %d, %s(): ", __FILE__, __LINE__, __FUNCTION__); diag_printf args; }
#else
#define ICLI_DEBUG(args) { diag_printf args; }
#endif
#else
#define _PRINTD(args)
#endif

#endif

/*
******************************************************************************

    Constant

******************************************************************************
*/
#define ICLI_SESSION_CNT                17
#define ICLI_SESSION_DEFAULT_CNT        17
#define ICLI_SESSION_ID_NONE            0xFFffFFff
#define ICLI_CMD_CNT                    1024
#define ICLI_CMD_WORD_CNT               512
#define ICLI_RANGE_LIST_CNT             128
#define ICLI_HISTORY_CMD_CNT            32
#define ICLI_PARAMETER_MEM_CNT          512
#define ICLI_NODE_PROPERTY_CNT          2048
#define ICLI_RUNTIME_CNT                ICLI_SESSION_CNT
#define ICLI_RANDOM_OPTIONAL_DEPTH      3
#define ICLI_RANDOM_OPTIONAL_CNT        128
#define ICLI_BYWORD_MAX_LEN             80

#define ICLI_USERNAME_MAX_LEN           32
#define ICLI_PASSWORD_MAX_LEN           32
#define ICLI_AUTH_MAX_CNT               5

#define ICLI_DEFAULT_PRIVILEGED_LEVEL   2

#define ICLI_STR_MAX_LEN                (3072)
#define ICLI_PUT_MAX_LEN                (512)
#define ICLI_ERR_MAX_LEN                (512)
#define ICLI_FILE_MAX_LEN               256
#define ICLI_NAME_MAX_LEN               32
#define ICLI_PROMPT_MAX_LEN             32
#define ICLI_DIGIT_MAX_LEN              64
#define ICLI_VALUE_STR_MAX_LEN          256
#define ICLI_DEFAULT_WAIT_TIME          (10 * 60)  //in seconds
#define ICLI_MODE_MAX_LEVEL             5
#define ICLI_BANNER_MAX_LEN             255

#if 1 /* CP, 2012/10/08 16:06, <hostname> */
#define ICLI_HOSTNAME_MAX_LEN           45
#endif

#define ICLI_DEFAULT_WIDTH              80
#define ICLI_MAX_WIDTH                  0x0fFFffFF
#define ICLI_MIN_WIDTH                  40

#define ICLI_DEFAULT_LINES              24
#define ICLI_MIN_LINES                  3

#define ICLI_DEFAULT_MIN_VLAN_ID        1
#define ICLI_DEFAULT_MAX_VLAN_ID        4095

#define ICLI_DEFAULT_MIN_PORT_ID        1
#define ICLI_DEFAULT_MAX_PORT_ID        24

#define ICLI_MAX_YEAR                   2037
#define ICLI_MIN_YEAR                   1970
#define ICLI_YEAR_MAX_MONTH             12
#define ICLI_YEAR_MIN_MONTH             1

#define ICLI_LOCATION_MAX_LEN           32

#define ICLI_DEBUG_PRIVI_CMD            "_debug_privilege_"

/*
    default properties of enable privilege password
*/
#define ICLI_DEFAULT_ENABLE_PASSWORD    "enable"
#define ICLI_ENABLE_PASSWORD_RETRY      3
#define ICLI_ENABLE_PASSWORD_WAIT       (30*1000)  // milli-second

/* default device name */
#define ICLI_DEFAULT_DEVICE_NAME        "Switch"

/*
    the following syntax symbol is used to customize the command
    reference guide.
*/
#define ICLI_HTM_MANDATORY_BEGIN        '{'
#define ICLI_HTM_MANDATORY_END          '}'
#define ICLI_HTM_LOOP_BEGIN             '('
#define ICLI_HTM_LOOP_END               ')'
#define ICLI_HTM_OPTIONAL_BEGIN         '['
#define ICLI_HTM_OPTIONAL_END           ']'
#define ICLI_HTM_VARIABLE_BEGIN         '<'
#define ICLI_HTM_VARIABLE_END           '>'

/*
    variable input style
*/
#define ICLI_VARIABLE_STYLE             0

/*
    support random optional or not
*/
#define ICLI_RANDOM_OPTIONAL            1

/*
    support random optional must number '}*1'
*/
#define ICLI_RANDOM_MUST_NUMBER         1
#define ICLI_RANDOM_MUST_CNT            8

/*
    Command mode
*/
typedef enum {
    ICLI_CMD_MODE_EXEC,
    ICLI_CMD_MODE_GLOBAL_CONFIG,
    ICLI_CMD_MODE_CONFIG_VLAN,
    ICLI_CMD_MODE_INTERFACE_PORT_LIST,
    ICLI_CMD_MODE_INTERFACE_VLAN,
    ICLI_CMD_MODE_CONFIG_LINE,
    ICLI_CMD_MODE_IPMC_PROFILE,
    ICLI_CMD_MODE_SNMPS_HOST,
    ICLI_CMD_MODE_STP_AGGR,
    //------- add above
    ICLI_CMD_MODE_MAX
} icli_cmd_mode_t;

/*
    Privilege
*/
typedef enum {
    ICLI_PRIVILEGE_0,
    ICLI_PRIVILEGE_1,
    ICLI_PRIVILEGE_2,
    ICLI_PRIVILEGE_3,
    ICLI_PRIVILEGE_4,
    ICLI_PRIVILEGE_5,
    ICLI_PRIVILEGE_6,
    ICLI_PRIVILEGE_7,
    ICLI_PRIVILEGE_8,
    ICLI_PRIVILEGE_9,
    ICLI_PRIVILEGE_10,
    ICLI_PRIVILEGE_11,
    ICLI_PRIVILEGE_12,
    ICLI_PRIVILEGE_13,
    ICLI_PRIVILEGE_14,
    ICLI_PRIVILEGE_15,
    //------- add above
    ICLI_PRIVILEGE_DEBUG,
    ICLI_PRIVILEGE_MAX,
} icli_privilege_t;

/*
******************************************************************************

    Macro Definition

******************************************************************************
*/
#ifdef ___LINUX___

#define icli_sprintf        sprintf

#ifdef WIN32
#define icli_snprintf       _snprintf
#define icli_vsnprintf      _vsnprintf
#else
#define icli_snprintf       snprintf
#define icli_vsnprintf      vsnprintf
#endif

#else

#define icli_sprintf        sprintf
#define icli_snprintf       snprintf
#define icli_vsnprintf      vsnprintf

#endif

#ifndef T_E
#define T_E(...) \
    printf("Error: %s, %s, %d, ", __FILE__, __FUNCTION__, __LINE__); \
    printf(__VA_ARGS__);
#endif

#ifndef T_W
#define T_W(...) \
    printf("Warning: %s, %s, %d, ", __FILE__, __FUNCTION__, __LINE__); \
    printf(__VA_ARGS__);
#endif

/*
******************************************************************************

    Type

******************************************************************************
*/
//----------------------------------------------------------------------------
//
//  Porting for Vitesse
//
//----------------------------------------------------------------------------
#ifndef _VTSS_TYPES_H_

#if !defined(VTSS_HAVE_I_TYPES)
typedef signed char         i8;   /*  8-bit signed */
typedef signed short        i16;  /* 16-bit signed */
typedef signed int          i32;  /* 32-bit signed */
#endif

#if !defined(VTSS_HAVE_U_TYPES)
typedef unsigned char       u8;   /*  8-bit unsigned */
typedef unsigned short      u16;  /* 16-bit unsigned */
typedef unsigned int        u32;  /* 32-bit unsigned */
#endif

#ifndef _WINDEF_
typedef unsigned char       BOOL; /* Boolean implemented as 8-bit unsigned */
#endif

//VTSS return code
typedef int                 vtss_rc;

enum {
    VTSS_RC_OK         =  0,  /**< Success */
    VTSS_RC_ERROR      = -1,  /**< Unspecified error */
    VTSS_RC_INV_STATE  = -2,  /**< Invalid state for operation */
    VTSS_RC_INCOMPLETE = -3,  /**< Incomplete result */
}; // Leave it anonymous.

/** \brief IPv4 address/mask */
typedef u32                 vtss_ip_t;

/** \brief IPv6 address/mask */
typedef struct {
    u8 addr[16];
} vtss_ipv6_t;

/** \brief MAC Address */
typedef struct {
    u8 addr[6];   /* Network byte order */
} vtss_mac_t;

#endif //_VTSS_TYPES_H_

#ifndef _VTSS_MAIN_TYPES_H_

/* - Integer types -------------------------------------------------- */
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned short      ushort;
typedef unsigned char       uchar;
typedef signed char         schar;

typedef uint                vtss_isid_t;

/* Init command structure */
/* ================================================================= *
 *  Initialization
 * ================================================================= */

/* Init command */
typedef enum {
    INIT_CMD_INIT,             /* Initialize module. Called before scheduler is started. */
    INIT_CMD_START,            /* Start module. Called after scheduler is started. */
    INIT_CMD_CONF_DEF,         /* Create and activate factory defaults.
                                  The 'flags' field is used for special exceptions.
                                  When creating factory defaults, each module may
                                  be called multiple times with different parameters.
                                  The 'isid' field may take one of the following values:
                                  VTSS_ISID_LOCAL : Create defaults in local section.
                                  VTSS_ISID_GLOBAL: Create global defaults in global section.
                                  Specific isid   : Create switch defaults in global section. */
    INIT_CMD_MASTER_UP,        /* Change from SLAVE to MASTER state */
    INIT_CMD_MASTER_DOWN,      /* Change from MASTER to SLAVE state */
    INIT_CMD_SWITCH_ADD,       /* MASTER state: Add managed switch to stack (isid valid) */
    INIT_CMD_SWITCH_DEL,       /* MASTER state: Delete switch from stack (isid valid) */
    INIT_CMD_SUSPEND_RESUME,   /* Suspend/resume port module (resume valid) */
    INIT_CMD_WARMSTART_QUERY,  /* Query if a module is ready for warm start (warmstart is output parameter) */
} init_cmd_t;

typedef struct {
    init_cmd_t      cmd;       /* Command */
    vtss_isid_t     isid;      /* CONF_DEF/SWITCH_ADD/SWITCH_DEL */
    ulong           flags;     /* CONF_DEF */
    uchar           resume;    /* SUSPEND_RESUME */
    uchar           warmstart; /* WARMSTART_QUERY - Module must set warmstart to FALSE if it is not ready for warmstart yet */
} vtss_init_data_t;

#endif /* _VTSS_MAIN_TYPES_H_ */

//****************************************************************************
#endif //__ICLI_PORTING_H__

