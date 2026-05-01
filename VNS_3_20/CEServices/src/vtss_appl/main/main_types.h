/*

 Vitesse Switch API software.

 Copyright (c) 2002-2012 Vitesse Semiconductor Corporation "Vitesse". All
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

#ifndef _VTSS_MAIN_TYPES_H_
#define _VTSS_MAIN_TYPES_H_

#include "vtss_types.h" /* For vtss_port_no_t */

/* - Integer types -------------------------------------------------- */
typedef unsigned int        uint;
typedef u32                 ulong; /* NB! A 32 bit unsigned entity */
typedef unsigned short      ushort;
typedef unsigned char       uchar;
typedef signed char         schar;
typedef long long           longlong;
typedef unsigned long long  ulonglong;

/* ================================================================= *
 *  User Switch IDs (USIDs), used by management modules (CLI, Web)
 * ================================================================= */
#define VTSS_USID_START  1
#define VTSS_USID_CNT    VTSS_ISID_CNT
#define VTSS_USID_END    (VTSS_USID_START + VTSS_USID_CNT)
#define VTSS_USID_ALL    0  /* Special value for selecting all switches. Only valid in selected contexts! */
#define VTSS_USID_LEGAL(usid) (usid >= VTSS_USID_START && usid < VTSS_USID_END)

typedef uint vtss_usid_t;

/* ================================================================= *
 *  Internal Switch IDs (ISIDs)
 * ================================================================= */
#define VTSS_ISID_START   1
#if VTSS_SWITCH_STACKABLE
#define VTSS_ISID_CNT     16
#else
#define VTSS_ISID_CNT     1
#endif /* VTSS_SWITCH_STACKABLE */
#define VTSS_ISID_END     (VTSS_ISID_START + VTSS_ISID_CNT)
#define VTSS_ISID_LOCAL   0             /* Special value for local switch. Only valid in selected contexts! */
#define VTSS_ISID_UNKNOWN 0xff          /* Special value only used in selected contexts!                    */
#define VTSS_ISID_GLOBAL VTSS_ISID_END /* INIT_CMD_CONF_DEF: Reset global parameters */

#define VTSS_ISID_LEGAL(isid) (isid >= VTSS_ISID_START && isid < VTSS_ISID_END)

typedef uint vtss_isid_t;

/* ================================================================= *
 *  Unique IDs
 * ================================================================= */
// This is the interval between each switch.
#define SWITCH_INTERVAL 1000

// Macro for getting a unique port ID based on the switch isid (internal switch id), and the iport (internal port number) for each port in a stack.
// When the unique port ID is generates, the isid and iport are converted to usid (user switch number) and uport (user port number).
// Example: usid == 2 and uport == 3 gives a unique port id of 1003 (not 2003).
// This is inline with the way that SNMP assigns ifIndex based on switch ID and port numbers.
#define GET_UNIQUE_PORT_ID(isid, iport) (topo_isid2usid(isid) - VTSS_USID_START) * SWITCH_INTERVAL + iport2uport(iport)

// Same as GET_UNIQUE_PORT_I, just using usid directly.
#define GET_UNIQUE_PORT_ID_USID(usid, iport) (usid - VTSS_USID_START) * SWITCH_INTERVAL + iport2uport(iport)

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
    init_cmd_t     cmd;            /* Command */
    vtss_isid_t    isid;           /* CONF_DEF/SWITCH_ADD/SWITCH_DEL */
    ulong          flags;          /* CONF_DEF */
    uchar          resume;         /* SUSPEND_RESUME */
    uchar          warmstart;      /* WARMSTART_QUERY - Module must set warmstart to FALSE if it is not ready for warmstart yet */
    ulong          port_cnt;       /* Only valid in SWITCH_ADD events */
    vtss_port_no_t stack_ports[2]; /* Only valid in SWITCH_ADD events */
    int            board_type;     /* Only valid in SWITCH_ADD events */
    unsigned int   api_inst_id;    /* Only valid in SWITCH_ADD events */
} vtss_init_data_t;

char *control_init_cmd2str(init_cmd_t cmd);

#endif /* _VTSS_MAIN_TYPES_H_ */
