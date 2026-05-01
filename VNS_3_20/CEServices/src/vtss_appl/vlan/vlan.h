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

#ifndef _VTSS_VLAN_H_
#define _VTSS_VLAN_H_

/* ========================================================= *
 * Trace definitions
 * ========================================================= */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <port_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_VLAN

#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_CRIT         1
#define TRACE_GRP_CRIT_CB      2
#define TRACE_GRP_CNT          3

#include <vtss_trace_api.h>

/* ================================================================= *
 * VLAN configuration blocks
 * ================================================================= */

#define VLAN_PORT_BLK_VERSION               0
#define VLAN_MEMBERSHIP_BLK_VERSION         1
#define VLAN_NAME_BLK_VERSION               0
#define VLAN_FORBID_MEMBERSHIP_BLK_VERSION  1

#define VLAN_DELETE             1
#define VLAN_ADD                0
#define VLAN_TPID_DEFAULT       0x88A8
#define VLAN_TPID_START         0x600

#define VLAN_ID_NONE 0
// Enumerate the modules requiring this feature.
typedef enum {
    VLAN_MULTI_STATIC,



#ifdef VTSS_SW_OPTION_MEP
    VLAN_MULTI_MEP,
#endif
#ifdef VTSS_SW_OPTION_EVC
    VLAN_MULTI_EVC,
#endif
#ifdef VTSS_SW_OPTION_MVR
    VLAN_MULTI_MVR,
#endif
  // This must come last:
    VLAN_MULTI_CNT
} multi_user_index_t;

typedef enum {
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    VLAN_SAME_VOICE_VLAN,
#endif
#ifdef VTSS_SW_OPTION_RSPAN
    VLAN_SAME_RSPAN,
#endif
    VLAN_SAME_CNT
} same_user_index_t;

typedef enum {
#ifdef VTSS_SW_OPTION_DOT1X
    VLAN_SINGLE_DOT1X,
#endif
    VLAN_SINGLE_CNT
} single_user_index_t;

typedef struct {
    vtss_vid_t vid;                                       /* VLAN ID */
    uchar ports[VTSS_ISID_END][VTSS_PORT_BF_SIZE];   /* Global Port mask */
} vlan_entry_conf_t;


#ifdef  VTSS_SW_OPTION_VLAN_NAME
typedef struct {
    u16             vid;                            /* VLAN ID              */
    i8              vlan_name[VLAN_NAME_MAX_LEN];   /* VLAN Name if valid   */
} vlan_name_conf_t;
#endif


#if (defined (VTSS_SW_OPTION_VOICE_VLAN) || defined (VTSS_SW_OPTION_RSPAN))
    #define VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
#endif
#if defined (VTSS_SW_OPTION_DOT1X)
    #define VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
#endif /* VTSS_SW_OPTION_DOT1X */

/* VLAN single membership configuration table */
typedef struct {
    // A VID = 0 indicates that this <isid:port> is not a member of any VLAN.
    // Entry 0 is used for switches with ID == VTSS_ISID_START, that is,
    // we don't keep track of the local configuration in this structure.
    vtss_vid_t vid[VTSS_ISID_CNT][VTSS_PORTS];
}vlan_single_membership_entry_t;

/* VLAN membership configuration table */
typedef struct {
    ulong               version;            /* Block version                    */
    ulong               count;              /* Number of entries                */
    ulong               size;               /* Size of each entry               */
    vlan_entry_conf_t   table[VLAN_MAX];    /* Entries                          */
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    vtss_etype_t        etype;              /* Ether Type for Custom S-ports    */
#endif
} vlan_table_blk_t;

/* VLAN forbidden table */
typedef struct {
    ulong               version;            /* Block version                    */
    ulong               fb_count;
    ulong               size;               /* Size of each entry               */
    vlan_entry_conf_t   forbidden[VLAN_MAX];    /* Entries                          */
} vlan_forbid_table_blk_t;


#ifdef  VTSS_SW_OPTION_VLAN_NAME
typedef struct {          
    ulong               version;                         /* Block version        */
    ulong               count;                           /* Number of entries    */
    ulong               size;                            /* Size of each entry   */
    vlan_name_conf_t    table[VLAN_NAME_MAX_CNT];        /* VLAN Name table      */
} vlan_name_table_blk_t;
#endif

/* VLAN port configuration table */
typedef struct {
    ulong            version;        /* Block version */
    vlan_port_conf_t conf[VTSS_ISID_CNT][VTSS_PORTS]; /* Entries */
} vlan_port_blk_t;

typedef struct {
  uchar ports[VLAN_MULTI_CNT][VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];
} vlan_multi_membership_entry_t;

typedef struct {
  uchar ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE];
} vlan_forbidden_list;

/* VLAN entry */
typedef struct vlan_t {
    struct vlan_t *next;             /* Next in list */
    vlan_multi_membership_entry_t conf;          /* Configuration */
    vlan_forbidden_list fb_port;
} vlan_t;

/* VLAN lists */
typedef struct {
    vlan_t *free;                    /* Free list */
} vlan_list_t;


/* ================================================================= *
 *  VLAN stack messages
 * ================================================================= */

/* VLAN messages IDs */
typedef enum {
    VLAN_MSG_ID_CONF_SET_REQ,              /* VLAN memberships configuration set request (no reply) */
    VLAN_MSG_ID_CONF_ADD_REQ,              /* VLAN memberships add request                          */
    VLAN_MSG_ID_CONF_DEL_REQ,              /* VLAN memberships delete request                       */
    VLAN_MSG_ID_CONF_TPID_REQ,             /* ETYPE for Custom S-ports set                          */
    VLAN_MSG_ID_PORT_CONF_SET_REQ,         /* VLAN port configuration set request for all ports(no reply) */
    VLAN_MSG_ID_SINGLE_PORT_CONF_SET_REQ   /* VLAN port configuration set request for single port(no reply) */
} vlan_msg_id_t;

/* Request message */
typedef struct {
    /* Message ID */
    vlan_msg_id_t msg_id;

    union {
        /* VLAN_MSG_ID_CONF_ADD_REQ */
        struct {
            vlan_entry_conf_t     conf;             /* Configuration */
        } add;

        /* VLAN_MSG_ID_CONF_DEL_REQ */
        struct {
            vlan_entry_conf_t     conf;             /* Configuration */
        } del;

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
        /* VLAN_MSG_ID_CONF_TPID_REQ */
        struct {
            vtss_etype_t          conf;             /* Configuration */
        } tpid;
#endif

        /* VLAN_MSG_ID_PORT_CONF_SET_REQ */
        struct {
            vlan_port_conf_t      conf[VTSS_PORTS]; /* Configuration */
        } port_set;

        /* VLAN_MSG_ID_SINGLE_PORT_CONF_SET_REQ */
        struct {
            vtss_port_no_t        port;             /* Port Num   */
            vtss_vlan_port_conf_t conf;             /* Configuration */
        } single_port_set;
    } req;
} vlan_msg_req_t;

/* Large request message */
typedef struct {
    /* Message ID */
    vlan_msg_id_t msg_id;

    union {
        /* VLAN_MSG_ID_CONF_SET_REQ */
        struct {
            ulong                 count;            /* Number of entries */
            vlan_entry_conf_t     table[VLAN_MAX];  /* Configuration */
        } set;
    } large_req;
} vlan_msg_large_req_t;

/* ========================================================================== 
 *
 * VLAN Global Structure                
 * =========================================================================*/
#define vlan_same_membership_entry_t vlan_entry_conf_t

typedef struct {
    critd_t                         crit;
    critd_t                         cb_crit;
    /** This array will store all the VLAN Users port 
      * configuration. Notice that one extra VLAN user entry is added here:
      * currently configured port properties are stored in the 
      * vlan_port_conf[VLAN_USER_CNT] entry.i
      * VLAN_USER_ALL is typecast to unsigned int to avoid LINT error.
      **/
    vlan_port_conf_t                vlan_port_conf[(u32)VLAN_USER_ALL + 1][VTSS_ISID_CNT][VTSS_PORT_ARRAY_SIZE];

    vlan_list_t                     stack_vlan;  // Holds the whole stack's configuration (entries [VTSS_ISID_START; VTSS_ISID_END[ used).
 
    vlan_t                          vlan_stack_table[VLAN_MAX];

    u16                             vlan_users[VTSS_ISID_CNT][VLAN_MAX];

#ifdef VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT
    vlan_same_membership_entry_t    vlan_same_membership_entries[VLAN_SAME_CNT];
#endif /* VTSS_SW_OPTION_VLAN_SAME_USER_SUPPORT */
#ifdef VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT
    vlan_single_membership_entry_t  vlan_single_membership_entries[VLAN_SINGLE_CNT];
#endif /* VTSS_SW_OPTION_VLAN_SINGLE_USER_SUPPORT */
#ifdef VTSS_SW_OPTION_VLAN_NAME
    vlan_name_entry_t               vlan_name_conf[VLAN_NAME_MAX_CNT];
#endif
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    vtss_etype_t                    tpid_s_custom_port;              /* Ether Type for Custom S-ports    */
#endif
    /* Request buffer message pool */
    void *request_pool;

    /* Request buffer pool for large requests */
    void *large_request_pool;

    /* ICLI support */
    vlan_port_mode_conf_t          port_mode_conf[VTSS_ISID_CNT][VTSS_PORT_ARRAY_SIZE];

} vlan_global_t;

#endif /* _VTSS_VLAN_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
