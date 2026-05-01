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

#ifndef _VTSS_VLAN_API_H_
#define _VTSS_VLAN_API_H_

/* Maximum number of VLANs that can be created in the system */
#define VLAN_MAX    4095
#define VLAN_PORT_SIZE VTSS_PORT_BF_SIZE(PORT_NUMBER)

#define VLAN_ID_START 1        /* First VLAN ID */
#define VLAN_ID_END   VLAN_MAX /* Last VLAN ID */

/* Minimum value of VID */
#define VLAN_TAG_MIN_VALID    1
/* Maximum value of VID */
#define VLAN_TAG_MAX_VALID    4095
#define VLAN_TAG_ALL_NOT_PVID (VLAN_TAG_MAX_VALID+2)
#define VLAN_TAG_DEFAULT      VLAN_TAG_MIN_VALID

#define VLAN_NAME_MAX_CNT       64      /* Number of VLANs which can have Names. This is to limit the usage of RAM */
#if (VLAN_NAME_MAX_CNT > VLAN_MAX)
#undef  VLAN_NAME_MAX_CNT
#define VLAN_NAME_MAX_CNT       VLAN_MAX
#endif

#define VLAN_NAME_MAX_LEN       33
#define VLAN_UNUSED_PARAM(n)    ((void)n)

#define VLAN_BUF_SIZE           128
#define VLAN_LIST_BUF_SIZE      8192

/** This enum identifies the VLAN User. While calling VLAN Management API, 
  * VLAN User should provide appropriate VLAN User as shown in the enum 
  **/
typedef enum{
    VLAN_USER_STATIC = 0, /* Do Not Change this position */
#ifdef VTSS_SW_OPTION_DOT1X
    VLAN_USER_DOT1X,
#endif



#ifdef VTSS_SW_OPTION_MVR
    VLAN_USER_MVR,
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    VLAN_USER_VOICE_VLAN,
#endif
#ifdef VTSS_SW_OPTION_MSTP
    VLAN_USER_MSTP,
#endif
#ifdef VTSS_SW_OPTION_ERPS
    VLAN_USER_ERPS,
#endif
#ifdef VTSS_SW_OPTION_MEP
    VLAN_USER_MEP,
#endif
#ifdef VTSS_SW_OPTION_EVC
    VLAN_USER_EVC,
#endif
#ifdef VTSS_SW_OPTION_VCL
    VLAN_USER_VCL,
#endif
#ifdef VTSS_SW_OPTION_RSPAN
    VLAN_USER_RSPAN,
#endif
    VLAN_USER_ALL,
    VLAN_USER_CNT
}vlan_user_t;

typedef enum{
    VLAN_ACTION_ADD,
    VLAN_ACTION_DELETE,
    VLAN_ACTION_UPDATE
} vlan_action_t;

/* VLAN API error codes (vtss_rc) */
enum {
    VLAN_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_VLAN),  /* Generic error code */
    VLAN_ERROR_PARM,                                      /* Illegal parameter */
    VLAN_ERROR_CONFIG_NOT_OPEN,                           /* Configuration open error */
    VLAN_ERROR_ENTRY_NOT_FOUND,                           /* VLAN not found */
    VLAN_ERROR_VLAN_TABLE_EMPTY,                          /* VLAN table empty */
    VLAN_ERROR_VLAN_TABLE_FULL,                           /* VLAN table full */
    VLAN_ERROR_VLAN_FORBIDDEN,
    VLAN_ERROR_REG_TABLE_FULL,                            /* Registration table full */
    VLAN_ERROR_REQ_TIMEOUT,                               /* Timeout on message request */
    VLAN_ERROR_STACK_STATE,                               /* Illegal MASTER/SLAVE state */
    VLAN_ERROR_DEL_INSTEAD_OF_ADD,                        /* The VLAN was deleted instead of added ( Due to the no ports was selected for the VALN ). This might not be an error */
    VLAN_ERROR_USER_VLAN_ID_MISMATCH,
    VLAN_ERROR_USER_PREVIOUSLY_CONFIGURED,
#ifdef  VTSS_SW_OPTION_VLAN_NAME
    VLAN_ERROR_VLAN_NAME_TABLE_FULL,                      /* VLAN Name Table is full */
    VLAN_ERROR_VLAN_NAME_PREVIOUSLY_CONFIGURED,            /* VLAN Name is already configured */
#endif
    VLAN_ERROR_VLAN_DYNAMIC_USER,
};

typedef enum {
    VLAN_TX_TAG_TYPE_UNTAG_THIS, /**< Send .uvid untagged. User module doesn't care about other VIDs.                        */
    VLAN_TX_TAG_TYPE_TAG_THIS,   /**< Send .uvid tagged. User module doesn't care about other VIDs.                          */
    VLAN_TX_TAG_TYPE_TAG_ALL,    /**< All 4K VLANs shall be sent tagged, despite this user module's membership configuration */
    VLAN_TX_TAG_TYPE_UNTAG_ALL,  /**< All 4K VLANs shall be sent untagged, despite this user module's membership configuration */
} vlan_tx_tag_type_t;

/** Flags to indicate what part of VLAN port configuration, user wants to configure */
enum {
    VLAN_PORT_FLAGS_PVID        = (1 << 0), /**< Indicates if this user wants to control the PVID          */
    VLAN_PORT_FLAGS_AWARE       = (1 << 1), /**< Indicates if this user wants to control VLAN awareness    */
    VLAN_PORT_FLAGS_INGR_FILT   = (1 << 2), /**< Indicates if this user wants to control Ingress Filtering */
    VLAN_PORT_FLAGS_RX_TAG_TYPE = (1 << 3), /**< Indicates if this user wants to control the Rx Tag Type. Allowed combinations are defined as part of vtss_vlan_frame_t enum */
    VLAN_PORT_FLAGS_TX_TAG_TYPE = (1 << 4),  /**< Indicates if this user wants to control the Tx Tag Type. Allowed combinations are defined as part of vlan_tx_tag_type_t enum */
    VLAN_PORT_FLAGS_ALL         = (VLAN_PORT_FLAGS_PVID | VLAN_PORT_FLAGS_AWARE | VLAN_PORT_FLAGS_INGR_FILT | VLAN_PORT_FLAGS_RX_TAG_TYPE | VLAN_PORT_FLAGS_TX_TAG_TYPE)
}; // Anonymous to satisfy Lint.

//This macro gives the count of port VLAN parameters that may have conflicts.
#define VLAN_PORT_FLAGS_CNT    5

//Do not change the order of this enum.
typedef enum {
    VLAN_PORT_PVID_IDX = 0,
    VLAN_PORT_PTYPE_IDX,
    VLAN_PORT_INGR_FILT_IDX,
    VLAN_PORT_RX_TAG_TYPE_IDX,
    VLAN_PORT_TX_TAG_TYPE_IDX
} vlan_port_flags_idx_t;

/** \brief VLAN port type configuration */
typedef enum
{
    VLAN_PORT_TYPE_UNAWARE, /**< VLAN unaware port */
    VLAN_PORT_TYPE_C,       /**< C-port */
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
    VLAN_PORT_TYPE_S,       /**< S-port */
    VLAN_PORT_TYPE_S_CUSTOM /**< S-port using alternative Ethernet Type */
#endif
} vlan_port_type_t;

typedef enum
{
    VLAN_NORMAL = 0, /**< VLAN unaware port */
    VLAN_FIXED,       /**< C-port */
    VLAN_FORBIDDEN,
    VLAN_NONE
} vlan_registration_type_t;


//This structure is used for conflicts display in CLI/Web.
typedef struct
{
    //These flags indicate type of the conflict. For example, if there is a 
    //Ingress filter conflict, VLAN_PORT_FLAGS_INGR_FILT bit will be set.
    u8  port_flags;
    //This is a bit mask of conflicting VLAN Users.
    u8  vlan_users[VLAN_PORT_FLAGS_CNT];
} vlan_port_conflicts_t;

/* VLAN Identifier */
typedef vtss_vid_t vid_t; /* 0-4095 */

/* VLAN port configuration */
typedef struct {
    vtss_vid_t              pvid;           /**< Port VLAN ID (ingress), 0 - 4095 (ingress)                   */
    vtss_vid_t              untagged_vid;   /**< Port Untagged VLAN ID (egress), 0 - 4095,4096,4097 (ingress) */
    vtss_vlan_frame_t       frame_type;     /**< Acceptable frame type (ingress)                              */
    BOOL                    ingress_filter; /**< Ingress filtering                                            */
    vlan_tx_tag_type_t      tx_tag_type;    /**< Egress tag requirements                                      */
    vlan_port_type_t        port_type;      /**< Port type (ingress and egress)                               */
    u8                      flags;          /**< A bitwise or of VLAN_PORT_FLAGS_xxx                          */
} vlan_port_conf_t;

/* Enum to represent VLAN mgmt operation for VLAN mgmt calls */
typedef enum {
    VLAN_MEMBERSHIP_OP, /* Operate on VLAN member list                  */
    VLAN_FORBIDDEN_OP,  /* Operate on VLAN forbid list                  */
    VLAN_MIXED_OP       /* Operate on both VLAN member and forbid list  */
} vlan_op_type_t;

/* Enum to represent a port as member port, forbidden port, non-member port */
typedef enum {
    VLAN_NON_MEMBER_PORT = 0, /* This should always be 0; Never try to change this value */
    VLAN_MEMBER_PORT,
    VLAN_FORBIDDEN_PORT,
    VLAN_NON_FORBIDDEN_PORT
} vlan_member_port_type_t;

typedef struct {
    vtss_vid_t          vid;                            /* VLAN ID */
    vlan_member_port_type_t ports[VTSS_PORT_ARRAY_SIZE];    /* Port mask */
#ifdef  VTSS_SW_OPTION_VLAN_NAME
    i8                  vlan_name[VLAN_NAME_MAX_LEN];   /* VLAN Name */
#endif
} vlan_mgmt_entry_t;

typedef struct{
	BOOL ports[VTSS_PORT_ARRAY_SIZE];
}vlan_registration_entry_t;

#ifdef VTSS_SW_OPTION_VLAN_NAME
typedef struct {
    vtss_vid_t          vid;
    BOOL                name_valid;
    i8                  vlan_name[VLAN_NAME_MAX_LEN];
} vlan_mgmt_vlan_name_conf_t;

typedef struct {
    BOOL                name_valid;                     /* Flag to indicate whether VLAN name is valid  */
    u16                 vid;                            /* VLAN ID */
    i8                  vlan_name[VLAN_NAME_MAX_LEN];   /* VLAN Name if valid                           */
} vlan_name_entry_t;
#endif

typedef enum {
    VLAN_PORT_MODE_ACCESS,  /* Access port */
    VLAN_PORT_MODE_TRUNK,   /* Trunk port  */
    VLAN_PORT_MODE_HYBRID,  /* Hybrid port */
    VLAN_PORT_MODE_NONE     /* None        */
} vlan_port_mode_t;

#define     VLAN_BIT_MASK_LEN                           ((VLAN_TAG_MAX_VALID + 7) / 8)
#define     VLAN_SET_VLAN_BIT_IN_VLAN_MASK(arr, vid)    (arr[(vid - 1) / 8] |= ((1 << ((vid - 1) % 8))))
#define     VLAN_CLR_VLAN_BIT_IN_VLAN_MASK(arr, vid)    (arr[(vid - 1) / 8] &= (~(u8)((1 << ((vid - 1) % 8)))))
#define     VLAN_IS_BIT_SET(arr, vid)                   ((arr[(vid - 1) / 8] & ((1 << ((vid - 1) % 8)))) ? 1 : 0)

typedef struct {
    vlan_port_mode_t mode;                                      /* Port mode - access or trunk          */
    vtss_vid_t       access_vid;                                /* For access ports                     */
    vtss_vid_t       native_vid;                                /* For trunk ports                      */
    BOOL             tag_native_vlan;                           /* Tag native VLAN                      */
    u8               allowed_bit_mask[VLAN_BIT_MASK_LEN];       /* Allowed VLANs for trunks             */
    vtss_vid_t       hybrid_native_vid;                         /* For hybrid ports                     */
    u8               hyb_allowed_bit_mask[VLAN_BIT_MASK_LEN];   /* Allowed VLANs for hybrid ports       */
    vlan_port_conf_t hyb_port_conf;                             /* Hybrid VLAN port configuration       */
} vlan_port_mode_conf_t;

/* VLAN error text */
char *vlan_error_txt(vtss_rc rc);

/* 
 * Get vlan port configuration. The returned information varies with the Switch ID
 * passed to this function.
 *
 * isid = VTSS_ISID_LOCAL
 *   Can be called from both a slave and the master
 *   Either way, it will return the values currently
 *   stored in H/W on the local switch, and will therefore
 *   not be retrievable for a given VLAN user, which means
 *   that you must specify \@user == VLAN_USER_ALL, which
 *   is a synonym for the VLAN port conf state combined
 *   for all users. If you fail to specify VLAN_USER_ALL,
 *   this function will return an error code.
 *
 * isid == [VTSS_ISID_START; VTSS_ISID_END[:
 *   Can only be called on the master.
 *   user == VLAN_USER_ALL causes this function to
 *   retrieve the combined VLAN Port configuration.
 *
 * isid == VTSS_ISID_GLOBAL:
 *   This is not supported. Hence will return error.
 */
vtss_rc vlan_mgmt_port_conf_get(vtss_isid_t isid, vtss_port_no_t port, vlan_port_conf_t *conf, vlan_user_t vlan_user);

/* Set vlan port configuration */
vtss_rc vlan_mgmt_port_conf_set(vtss_isid_t isid, vtss_port_no_t port, vlan_port_conf_t *conf, vlan_user_t vlan_user);

/* Add vlan entry configuration by vid (Both forbidden and normal membership) */
vtss_rc vlan_mgmt_vlan_add(vtss_isid_t isid, vlan_mgmt_entry_t *vlan_mgmt_entry,vlan_user_t user);

/* get the port list information for given vid*/
vtss_rc vlan_mgmt_vlan_registration_get(vtss_isid_t isid, vtss_vid_t vid, vlan_registration_type_t *vlan_Regstar_entry, vlan_user_t user);

/* Add vlan entry configuration by VLAN name */
vtss_rc vlan_mgmt_vlan_add_by_name(vtss_isid_t isid_add, vlan_mgmt_entry_t *vlan_mgmt_entry, vlan_user_t user);

/* Delete vlan entry configuration by vid */
vtss_rc vlan_mgmt_vlan_del(vtss_isid_t isid,vtss_vid_t vid, vlan_user_t user, vlan_op_type_t oper);

/* Delete vlan entry configuration by VLAN name */
vtss_rc vlan_mgmt_vlan_del_by_name(vtss_isid_t isid_del, i8 *vlan_name, vlan_user_t user, vlan_op_type_t oper);

/* Get VLAN Port Conflicts */
vtss_rc vlan_mgmt_conflicts_get(vlan_port_conflicts_t* conflicts, vtss_isid_t isid, vtss_port_no_t port);

/* Get VLAN count for a VLAN user */
vtss_rc vlan_mgmt_vlan_count_get(vtss_isid_t isid, u16 *num_of_vlans, vlan_user_t user);

/**
 * \brief Get VLAN membership and forbid membership configuration
 *
 * Given a switch ID, get the (next) VLAN ID defined for
 * the VLAN user.
 * To iterate over all defined VLANs for a given user,
 * start by setting vlan_mgmt_entry->vid to 0 and next
 * to TRUE. For each call, you'll then get the next
 * defined VLAN ID for that user, and as long as this
 * function returns VTSS_OK, the information returned
 * in vlan_mgmt_entry is valid.
 *
 * If you want the membership information for a given
 * VLAN ID, set the VID in vlan_mgmt_entry->vid and
 * set next to FALSE. The VID is not defined for that
 * user if this function returns anything but VTSS_OK.
 *
 * When oper is VLAN_MIXED_OP, returned port can be of any of the following -  
 * VLAN_FORBIDDEN_PORT, VLAN_MEMBER_PORT or VLAN_NON_MEMBER_PORT.
 * If oper is VLAN_FORBIDDEN_OP, returned port can be of any of of the following -
 * VLAN_FORBIDDEN_PORT or VLAN_NON_FORBIDDEN_PORT.
 * If oper is VLAN_MEMBERSHIP_OP, returned port can be of any of of the following -
 * VLAN_MEMBER_PORT or VLAN_NON_MEMBER_PORT. 
 *
 * The returned information varies with the Switch ID
 * passed to this function.
 *
 * isid == VTSS_ISID_LOCAL:
 *   Can be called from both a slave and the master
 *   Either way, it will return the values currently
 *   stored in H/W on the local switch, and will therefore
 *   not be retrievable for a given VLAN user, which means
 *   that you must specify \@user == VLAN_USER_ALL, which
 *   is a synonym for the VLAN membership state combined
 *   for all users. If you fail to specify VLAN_USER_ALL,
 *   this function will return an error code.
 *
 * isid == [VTSS_ISID_START; VTSS_ISID_END[:
 *   Can only be called on the master.
 *   If the function call succeeds, vlan_mgmt_entry->vid
 *   contains the (next) VLAN ID and vlan_mgmt_entry->ports
 *   contains the member ports of this VLAN ID for the
 *   selected user.
 *   user == VLAN_USER_ALL causes this function to
 *   retrieve the combined memberships.
 *
 * isid == VTSS_ISID_GLOBAL:
 *   Can only be called on the master.
 *   If return value is VTSS_OK, this function returns
 *   the next VID defined on the whole stack for the
 *   selected user. That is, all switches in the stack
 *   (including those that don't currently exist) will
 *   contribute to the (next) VLAN ID.
 *   Only vlan_mgmt_entry->vid will be valid on return
 *   from this function; vlan_mgmt_entry->ports should
 *   not be used for anything by the caller.
 *   All values of \@user are supported, including
 *   VLAN_USER_ALL, which will cause this function to
 *   return the next VID defined by any user.
 */
vtss_rc vlan_mgmt_vlan_get(vtss_isid_t isid, vtss_vid_t vid, vlan_mgmt_entry_t *vlan_mgmt_entry, BOOL next, 
                           vlan_user_t user, vlan_op_type_t oper);

/* Lookup VLAN table by VLAN name */
vtss_rc vlan_mgmt_vlan_get_by_name(vtss_isid_t isid, i8 *vlan_name, vlan_mgmt_entry_t *vlan_mgmt_entry,
                                   BOOL next, vlan_user_t user, vlan_op_type_t oper);
#if defined(VTSS_FEATURE_VLAN_PORT_V2)
/* Set ETYPE for Custom S-port */
vtss_rc vlan_mgmt_vlan_tpid_set(vtss_etype_t tpid);
/* Get ETYPE for Custom S-port */
vtss_rc vlan_mgmt_vlan_tpid_get(vtss_etype_t *tpid);
vtss_rc vlan_mgmt_debug_vlan_tpid_get(vtss_etype_t *tpid);
#endif

#ifdef  VTSS_SW_OPTION_VLAN_NAME
/* Associate VLAN with a name. If user is sure of unique VLAN name, set the unique_name flag to TRUE */
vtss_rc vlan_mgmt_vlan_name_add(vlan_mgmt_vlan_name_conf_t *conf, BOOL unique_name);
vtss_rc vlan_mgmt_vlan_name_del(i8 *vlan_name);
vtss_rc vlan_mgmt_vlan_name_del_by_vid(u16 vid);
vtss_rc vlan_mgmt_vlan_name_get(u16 index, vlan_mgmt_vlan_name_conf_t *conf);
#endif

/* Register for VLAN changes */
typedef void (*vlan_forbid_change_callback_t)(vtss_isid_t isid, vtss_vid_t vid,
                                              u8 added_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                              u8 deleted_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                              u8 current_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE]);
typedef void (*vlan_membership_change_callback_t)(vtss_isid_t isid, vtss_vid_t vid, vlan_user_t user,
                                                  u8 added_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                                  u8 deleted_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                                  u8 current_ports[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE],
                                                  vlan_action_t action);

void vlan_forbid_change_register(vlan_forbid_change_callback_t cb);
void vlan_membership_change_register(vlan_membership_change_callback_t cb);

// Register for VLAN port configuration changes. new_conf will point to changed port configuration.
typedef void (*vlan_port_change_callback_t)(vtss_port_no_t port_no, const vlan_port_conf_t *new_conf);
void vlan_port_conf_change_register(vlan_port_change_callback_t cb);

#if defined(VTSS_FEATURE_VLAN_PORT_V2)
/* Register for VLAN TPID changes for Custom S-port */
typedef void (*vlan_s_port_tpid_conf_change_callback_t)(vtss_etype_t tpid);
void vlan_s_port_tpid_conf_change_register(vlan_s_port_tpid_conf_change_callback_t cb);
#endif

/* Initialize module */
vtss_rc vlan_init(vtss_init_data_t *data);

/* TO get the VLAN user to text mapping*/
char *vlan_mgmt_vlan_user_to_txt(vlan_user_t usr);

vtss_rc vlan_mgmt_port_mode_set(vtss_isid_t isid, vtss_port_no_t port, vlan_port_mode_conf_t *pmode);
vtss_rc vlan_mgmt_port_mode_get(vtss_isid_t isid, vtss_port_no_t port, vlan_port_mode_conf_t *pmode);
vtss_rc vlan_mgmt_port_access_vlan_set(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t vid);
vtss_rc vlan_mgmt_port_access_vlan_get(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t *vid);
vtss_rc vlan_mgmt_port_native_vlan_set(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t vid, vlan_port_mode_t port_mode);
vtss_rc vlan_mgmt_port_native_vlan_get(vtss_isid_t isid, vtss_port_no_t port_no, vtss_vid_t *vid, vlan_port_mode_t port_mode);
vtss_rc vlan_mgmt_port_allowed_vlans_set(vtss_isid_t isid, vtss_port_no_t port_no, u8 *vid_mask, vlan_port_mode_t port_mode);
vtss_rc vlan_mgmt_port_allowed_vlans_get(vtss_isid_t isid, vtss_port_no_t port_no, u8 *vid_mask, vlan_port_mode_t port_mode);
i8 *vlan_bit_mask_to_txt(u8 *vlan_mask, i8 *buf);
vtss_rc vlan_mgmt_native_vlan_tagging_set(vtss_isid_t isid, vtss_port_no_t port_no, BOOL enable);
vtss_rc vlan_mgmt_native_vlan_tagging_get(vtss_isid_t isid, vtss_port_no_t port_no, BOOL *tagging_enabled);
vtss_rc vlan_mgmt_hybrid_port_conf_set(vtss_isid_t isid, vtss_port_no_t port_no, vlan_port_conf_t *pconf);
vtss_rc vlan_mgmt_hybrid_port_conf_get(vtss_isid_t isid, vtss_port_no_t port_no, vlan_port_conf_t *pconf);
vtss_rc vlan_mgmt_default_port_conf_get(vlan_port_conf_t *vlan_port_conf);

#endif /* _VTSS_VLAN_API_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
