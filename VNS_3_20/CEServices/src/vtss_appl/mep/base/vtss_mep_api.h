/*

 Vitesse MEP software.

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
 
 $Id: vtss_mep_api.h,v 1.33 2011/03/16 12:26:55 henrikb Exp $
 $Revision: 1.33 $

*/

#ifndef _VTSS_MEP_API_H_
#define _VTSS_MEP_API_H_

#include "vtss_types.h"
#include "vtss_api.h"
#include "vtss_mep_supp_api.h"


#define VTSS_MEP_INSTANCE_MAX        100     /* Max number of MEP instance */
#define VTSS_MEP_PEER_MAX            5       /* Max number of peer MEP    */
#define VTSS_MEP_TRANSACTION_MAX     5       /* Max number of Link Trace transaction */
#define VTSS_MEP_REPLY_MAX           5       /* Max number of reply in a transaction */
#define VTSS_MEP_DM_MAX              2000    /* Max number of DM counters saved in RAM */
#define VTSS_MEP_CLIENT_FLOWS_MAX    10      /* MAX number of client flows */

#define VTSS_MEP_RC_OK                 0   /* Management operation is ok */
#define VTSS_MEP_RC_INVALID_PARAMETER  1   /* invalid parameter */
#define VTSS_MEP_RC_NOT_ENABLED        2   /* MEP instance is not enabled */
#define VTSS_MEP_RC_CAST               3   /* LM and CC sharing SW generated CCM must have same casting (multi/uni) */
#define VTSS_MEP_RC_PERIOD             4   /* LM and CC sharing SW generated CCM must have same period */
#define VTSS_MEP_RC_PEER_CNT           5   /* Invalid peer count for this configuration */
#define VTSS_MEP_RC_PEER_ID            6   /* Dublicate peer ID when more than one peer */
#define VTSS_MEP_RC_MIP                7   /* Not allowed on a MIP */
#define VTSS_MEP_RC_INVALID_EVC        8   /* EVC flow was found invalid */
#define VTSS_MEP_RC_APS_EGRESS         9   /* APS not allowed on an egress MEP */
#define VTSS_MEP_RC_APS_DOMAIN        10   /* R-APS is only allowed in port domain */
#define VTSS_MEP_RC_INVALID_VID       11   /* VLAN is not created on this VID */
#define VTSS_MEP_RC_INVALID_COS_ID    12   /* The COS ID is not defined for this EVC */
#define VTSS_MEP_RC_NO_VOE            13   /* There is no VOE available to be created */
#define VTSS_MEP_RC_NO_TIMESTAMP_DATA 14   /* There is no dmr timestamp data available f*/
#define VTSS_MEP_RC_PEER_MAC          15   /* Peer Unicast MAC must be known to do HW based CCM on SW MEP */

#define VTSS_MEP_APS_DATA_LENGTH      VTSS_MEP_SUPP_RAPS_DATA_LENGTH
#define VTSS_MEP_MAC_LENGTH            6
#define VTSS_MEP_MEG_CODE_LENGTH       9  /* Both Domain Name/ICC and MEG-ID can be max 8 charecters plus a NULL termination */



/****************************************************************************/
/*  MEP management interface                                                */
/****************************************************************************/

typedef enum
{
    VTSS_MEP_MGMT_PORT,      /* Domain is Port */
//    VTSS_MEP_MGMT_ESP,       /* Domain is ESP Path */
    VTSS_MEP_MGMT_EVC,       /* Domain is EVC Service */
//    VTSS_MEP_MGMT_MPLS       /* Domain is Mpls */
} vtss_mep_mgmt_domain_t;

typedef enum
{
    VTSS_MEP_MGMT_MEP,
    VTSS_MEP_MGMT_MIP
} vtss_mep_mgmt_mode_t;

typedef enum
{
    VTSS_MEP_MGMT_INGRESS,  /* Ingress/down MEP/MIP */
    VTSS_MEP_MGMT_EGRESS    /* Egress/up MEP/MIP */
} vtss_mep_mgmt_direction_t;

typedef enum
{
    VTSS_MEP_MGMT_ITU_ICC,    /* MEG-ID is ITU ICC format as described in Y.1731 ANNEX A */
    VTSS_MEP_MGMT_IEEE_STR    /* MEG-ID is IEEE Domain Name format as described in 802.1ag 21.6.5 - Domain format '4' and Short format '2' (Character string) */
} vtss_mep_mgmt_format_t;

typedef struct
{
    BOOL                        enable;                                           /* Enable/disable                                  */
    vtss_mep_mgmt_mode_t        mode;                                             /* MEP or MIP mode                                 */
    vtss_mep_mgmt_direction_t   direction;                                        /* Ingress or Egress direction                     */
    vtss_mep_mgmt_domain_t      domain;                                           /* Domain                                          */
    u32                         flow;                                             /* Flow instance                                   */
    u32                         port;                                             /* Residence port - same as flow if domain is Port */
    u32                         level;                                            /* MEG level                                       */
    u16                         vid;                                              /* VID used for tagged OAM                         */
    BOOL                        voe;                                              /* This should be VOE based if possible            */

    vtss_mep_mgmt_format_t      format;                                           /* MEG-ID format.                                  */
    char                        name[VTSS_MEP_MEG_CODE_LENGTH];                   /* Domain Name or ITU Carrier Code (ICC). (string) */
    char                        meg[VTSS_MEP_MEG_CODE_LENGTH];                    /* Unique MEG ID Code.                    (string) */
    u32                         mep;                                              /* MEP id of this instance                         */
    u32                         peer_count;                                       /* Number of peer MEP’s  (VTSS_MEP_PEER_MAX)       */
    u16                         peer_mep[VTSS_MEP_PEER_MAX];                      /* MEP id of peer MEP’s                            */
    u8                          peer_mac[VTSS_MEP_PEER_MAX][VTSS_MEP_MAC_LENGTH]; /* Peer unicast MAC                                */

    u32                         evc_pag;                                          /* EVC generated policy PAG value. On Jaguar this is used as IS2 key. For Up-MEP (DS1076) this is used when creating MCE (IS1) entries */
    u32                         evc_qos;                                          /* EVC QoS value. Only used on Caracel for getting queue frame counters. For Up-MEP (DS1076) this is used when creating MCE (IS1) entries */
} vtss_mep_mgmt_conf_t;

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_conf_set(const u32                   instance,
                           const vtss_mep_mgmt_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* mac:         MAC of this MEP instance                */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_conf_get(const u32                  instance,
                           u8                         *const mac,
                           vtss_mep_mgmt_conf_t       *const conf);




typedef enum
{
    VTSS_MEP_MGMT_PERIOD_INV,
    VTSS_MEP_MGMT_PERIOD_300S,
    VTSS_MEP_MGMT_PERIOD_100S,
    VTSS_MEP_MGMT_PERIOD_10S,
    VTSS_MEP_MGMT_PERIOD_1S,
    VTSS_MEP_MGMT_PERIOD_6M,
    VTSS_MEP_MGMT_PERIOD_1M,
    VTSS_MEP_MGMT_PERIOD_6H,
} vtss_mep_mgmt_period_t;

typedef struct
{
    BOOL				      enable;         /* Enabled if true              */
    BOOL                      dei;            /* DEI for the CCM              */
    u32                       prio;           /* Priority for the CCM         */
    vtss_mep_mgmt_period_t    period;         /* CCM transmission period      */
} vtss_mep_mgmt_cc_conf_t;

/* instance:    Instance number of MEP.                                         */
/* CC based on CCM is enabled/disabled. When period is VTSS_MEP_MGMT_PERIOD_300S, CCM is HW driven - otherwise SW driven. */
/* The parameters 'period' and 'cast' must be the same as for vtss_mep_mgmt_lm_conf_set() if both CC and LM is configured */
/* to be based on SW driven CCM session.                                                                                  */
u32 vtss_mep_mgmt_cc_conf_set(const u32                        instance,
                              const vtss_mep_mgmt_cc_conf_t    *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_cc_conf_get(const u32                 instance,
                              vtss_mep_mgmt_cc_conf_t   *const conf);




typedef enum
{
    VTSS_MEP_MGMT_SINGEL_ENDED,  /* Singel ended LM based on LMM/LMR    */
    VTSS_MEP_MGMT_DUAL_ENDED     /* Dual ended LM based on CCM          */
} vtss_mep_mgmt_lm_ended_t;

typedef enum
{
    VTSS_MEP_MGMT_UNICAST,		/* Unicast  destination address */
    VTSS_MEP_MGMT_MULTICAST		/* Multicast  destination address */
} vtss_mep_mgmt_cast_t;

typedef struct
{
    BOOL                        enable;         /* Enabled if true                                           */
    BOOL                        dei;            /* DEI for the CCM/LMM                                       */
    u32                         prio;           /* Priority for the CCM/LMM                                  */
    vtss_mep_mgmt_period_t      period;         /* PDU transmission period                                   */
    vtss_mep_mgmt_cast_t	    cast;           /* Uni/multicast selection - only multicast on CCM based LM  */
    vtss_mep_mgmt_lm_ended_t    ended;          /* Dual/single ended selection                               */
    u32                         flr_interval;   /* Frame loss ratio time interval in sec.                    */
} vtss_mep_mgmt_lm_conf_t;

/* instance:    Instance number of MEP.                                     */
/* LM based on either CCM/LMM (depending on 'ended') is enabled/disabled.   */
/* The parameters 'period' and 'cast' must be the same as for vtss_mep_mgmt_cc_conf_set() */
/* if both CC and LM is configured to be based on SW driven CCM session.                  */
u32 vtss_mep_mgmt_lm_conf_set(const u32                      instance,
                              const vtss_mep_mgmt_lm_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_lm_conf_get(const u32                instance,
                             vtss_mep_mgmt_lm_conf_t  *const conf);




typedef enum
{
    VTSS_MEP_MGMT_ONE_WAY,    /* One Way DM ?based on 1DM    */
    VTSS_MEP_MGMT_TWO_WAY     /* Two Way DM ?based on DMM    */
} vtss_mep_mgmt_dm_way_t;
 
typedef enum
{
    VTSS_MEP_MGMT_STD,  /* Standard DM sending */
    VTSS_MEP_MGMT_PROP  /* Vitesse proprietary DM sending */
} vtss_mep_mgmt_dm_txway_t;

typedef enum
{
    VTSS_MEP_MGMT_RDTRP,   /* Use two timestamps to calculate DM */
    VTSS_MEP_MGMT_FLOW      /* Use four timestamps to calculate DM */
} vtss_mep_mgmt_dm_calcway_t;

#define VTSS_MEP_MGMT_MIN_GAP   10
#define VTSS_MEP_MGMT_MAX_GAP   65535

#define VTSS_MEP_MGMT_MIN_CNT   10
#define VTSS_MEP_MGMT_MAX_CNT   VTSS_MEP_DM_MAX

typedef enum
{
    VTSS_MEP_MGMT_US,   /* MicroSecond */
    VTSS_MEP_MGMT_NS    /* NanoSecond */
} vtss_mep_mgmt_dm_tunit_t;

typedef enum
{
    VTSS_MEP_MGMT_DISABLE,  /* When overflow, DM is disabled automatically */
    VTSS_MEP_MGMT_CONTINUE     /* When overflow, counter is reset and DM continues running */
} vtss_mep_mgmt_dm_act_t;


typedef struct
{
/* Modify/add this for DM */
    BOOL                        enable;         /* Enabled if true                                                         */
    BOOL                        dei;            /* DEI for the DM                                                          */
    u32                         prio;           /* Priority for the DM                                                     */
    vtss_mep_mgmt_cast_t	    cast;           /* Uni/multicast selection                                                 */
    u32                         mep;            /* Target Peer Mep to receive DMM/1DM - only used if unicast is selected   */
    vtss_mep_mgmt_dm_way_t      way;            /* One or two Way  Delay Measurement                                       */              
    vtss_mep_mgmt_dm_txway_t    txway;          /* Standard/Vitesse proprietary to send DM                                 */
    vtss_mep_mgmt_dm_calcway_t  calcway;        /* Calculation way selection                                               */   
    u32                         gap;            /* Gap between 1DM/DMM in second                                           */
    u32                         count;          /* The number of last records to calculate                                 */
    vtss_mep_mgmt_dm_tunit_t    tunit;          /* Time resolution                                                         */
    vtss_mep_mgmt_dm_act_t      act;            /* Action when counter overflow                                            */ 
    BOOL                        dm2fordm1;      /* Calculate 1DM with 2DM packets                                          */ 
} vtss_mep_mgmt_dm_conf_t;

/* instance:    Instance number of MEP.                                   */
/* DM based on either 1DM/DMM (depending on 'way') is enabled/disabled.   */
u32 vtss_mep_mgmt_dm_conf_set(const u32                      instance,
                              const vtss_mep_mgmt_dm_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_dm_conf_get(const u32                instance,
                             vtss_mep_mgmt_dm_conf_t  *const conf);




typedef enum
{
    VTSS_MEP_MGMT_INV_APS,  /* Invalid/undefined    */
    VTSS_MEP_MGMT_L_APS,    /* Liniar protection APS    */
    VTSS_MEP_MGMT_R_APS     /* Ring protection APS    */
} vtss_mep_mgmt_aps_type_t;

typedef struct
{
    BOOL                        enable;         /* Enabled if true              */
    BOOL                        dei;            /* DEI for the APS              */
    u32                         prio;           /* Priority for the APS         */
    vtss_mep_mgmt_aps_type_t    type;           /* Type of APS                  */
    vtss_mep_mgmt_cast_t	    cast;           /* Uni/multicast selection      */
    u32                         raps_octet;     /* Lats octet in the R-APS multicast DA */
} vtss_mep_mgmt_aps_conf_t;

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of APS. The APS data contained in the APS PDU is controlled by EPS */
u32 vtss_mep_mgmt_aps_conf_set(const u32                       instance,
                               const vtss_mep_mgmt_aps_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_aps_conf_get(const u32                 instance,
                               vtss_mep_mgmt_aps_conf_t  *const conf);




typedef struct
{
    BOOL        enable;                    /* Enabled if true                                                        */
    BOOL        dei;                       /* DEI for the LTM                                                        */
    u32         prio;                      /* Priority for the LTM                                                   */
    u32         mep;                       /* Target Peer Mep to receive LTM - only used if 'mac' is 'all zero'      */
    u8          mac[VTSS_MEP_MAC_LENGTH];  /* Unicast MAC address to receive LTM - has to be used to send LTM to MIP */
    u32         ttl;                       /* Time To Live                                                           */
} vtss_mep_mgmt_lt_conf_t;

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of Link Trace. */
u32 vtss_mep_mgmt_lt_conf_set(const u32                      instance,
                              const vtss_mep_mgmt_lt_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_lt_conf_get(const u32                 instance,
                               vtss_mep_mgmt_lt_conf_t  *const conf);



#define VTSS_MEP_LB_MGMT_TO_SEND_INFINITE  0
typedef struct
{
    BOOL                     enable;                    /* Enabled if true                                                                          */
    BOOL                     dei;                       /* DEI for the LBM                                                                          */
    u32                      prio;                      /* Priority for the LBM                                                                     */
    vtss_mep_mgmt_cast_t	 cast;                      /* Uni/multicast selection                                                                  */
    u32                      mep;                       /* Peer Mep to receive LBM - only used if unicast and 'mac' is 'all zero'                   */
    u8                       mac[VTSS_MEP_MAC_LENGTH];  /* Unicast MAC address to receive LBM - has to be used to send LBM to MIP                   */
    u32                      to_send;                   /* Number of LBM to send. VTSS_MEP_LB_MGMT_TO_SEND_INFINITE => test behaviour. Requires VOE */
    u32                      size;                      /* Size of data pattern to send - max. 1400 bytes                                           */
    u32                      interval;                  /* Frame interval. In 10 ms. if to_send != VTSS_MEP_LB_MGMT_TO_SEND_INFINITE                */
                                                        /*                 In 1 us. if to_send == VTSS_MEP_LB_MGMT_TO_SEND_INFINITE                 */
} vtss_mep_mgmt_lb_conf_t;

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of Loop Back. */
u32 vtss_mep_mgmt_lb_conf_set(const u32                      instance,
                              const vtss_mep_mgmt_lb_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_lb_conf_get(const u32                 instance,
                               vtss_mep_mgmt_lb_conf_t  *const conf);

typedef struct
{
    u8      mac[VTSS_MEP_MAC_LENGTH];
    u64     lbr_received;
    u64     out_of_order;
} vtss_mep_mgmt_lb_reply_t;

typedef struct
{
    u32                        transaction_id;
    u64                        lbm_transmitted;
    u32                        reply_cnt;
    vtss_mep_mgmt_lb_reply_t   reply[VTSS_MEP_PEER_MAX];
} vtss_mep_mgmt_lb_state_t;


/* instance:    Instance number of MEP.                 */
/* Get the Loop Back Result                             */
u32 vtss_mep_mgmt_lb_state_get(const u32                  instance,
                               vtss_mep_mgmt_lb_state_t   *const state);



typedef struct
{
    vtss_mep_mgmt_mode_t        mode;
    vtss_mep_mgmt_direction_t   direction;
    u8                          ttl;
    BOOL                        relayed;
    u8                          last_egress_mac[VTSS_MEP_MAC_LENGTH];
    u8                          next_egress_mac[VTSS_MEP_MAC_LENGTH];
} vtss_mep_mgmt_lt_reply_t;

typedef struct
{
    u32                        transaction_id;
    u32                        reply_cnt;
    vtss_mep_mgmt_lt_reply_t   reply[VTSS_MEP_REPLY_MAX];
} vtss_mep_mgmt_lt_transaction_t;

typedef struct
{
    u32                              transaction_cnt;
    vtss_mep_mgmt_lt_transaction_t   transaction[VTSS_MEP_TRANSACTION_MAX];
} vtss_mep_mgmt_lt_state_t;

/* instance:    Instance number of MEP.                 */
/* Get the Link Trace Result                            */
u32 vtss_mep_mgmt_lt_state_get(const u32                  instance,
                               vtss_mep_mgmt_lt_state_t   *const state);



typedef struct
{
    BOOL    cLevel;                        /* Incorrect CCM level received             */
    BOOL    cMeg;                          /* Incorrect CCM MEG id received            */
    BOOL    cMep;                          /* Incorrect CCM MEP id received            */
    BOOL    cSsf;                          /* SSF state                                */
    BOOL    cLoc[VTSS_MEP_PEER_MAX];       /* CCM LOC state from all peer MEP          */
    BOOL    cRdi[VTSS_MEP_PEER_MAX];       /* CCM RDI state from all peer MEP          */
    BOOL    cPeriod[VTSS_MEP_PEER_MAX];    /* CCM Period state from all peer MEP       */
    BOOL    cPrio[VTSS_MEP_PEER_MAX];      /* CCM Priority state from all peer MEP     */
    BOOL    cAis;                          /* AIS state                                */
    BOOL    cLck;                          /* Locked State                             */
    
    BOOL    aTsf;
    BOOL    aTsd;
    BOOL    aBlk;
} vtss_mep_mgmt_state_t;

/* instance:    Instance number of MEP. */
/* Get the ETH state of this MEP        */
u32 vtss_mep_mgmt_state_get(const u32                instance,
                            vtss_mep_mgmt_state_t    *const state);



typedef struct
{
    u32     tx_counter;         /* Transmitted PDU (LMM - CCM) containing counters     */
    u32     rx_counter;         /* Received PDU (LMM - CCM) containing counters        */
    i32     near_los_counter;   /* Near end loss counter                               */
    i32     far_los_counter;    /* Far end loss counter                                */
    u32     near_los_ratio;     /* Near end frame loss ratio                           */
    u32     far_los_ratio;      /* Near end frame loss ratio                           */
} vtss_mep_mgmt_lm_state_t;

/* instance:    Instance number of MEP. */
/* Get the LM counters of this MEP      */
u32 vtss_mep_mgmt_lm_state_get(const u32                   instance,
                               vtss_mep_mgmt_lm_state_t    *const state);

/* instance:    Instance number of MEP. */
/* Clear LM counters for this MEP       */
u32 vtss_mep_mgmt_lm_state_clear_set(const u32    instance);



/* instance:    Instance number of MEP. */
/* Get the Latest timestamps of this MEP         */
u32 vtss_mep_mgmt_dm_timestamp_get(const u32                    instance,
                               vtss_mep_mgmt_dm_timestamp_t *const dm1_timestamp_far_to_near,
                               vtss_mep_mgmt_dm_timestamp_t *const dm1_timestamp_near_to_far);                               

typedef struct
{
    u32                         tx_cnt;
    u32                         rx_cnt;
    u32                         rx_tout_cnt;
    u32                         rx_err_cnt;
    u32                         ovrflw_cnt;
    u32                         late_txtime; /* debug only */
    /* The unit of the counters below is nanosecond */
    u32                         avg_delay;      
    u32                         avg_n_delay;
    u32                         avg_delay_var;
    u32                         avg_n_delay_var;  
    u32                         best_delay;
    u32                         worst_delay;
    u32                         last_delay;
    vtss_mep_mgmt_dm_tunit_t    tunit;  
} vtss_mep_mgmt_dm_state_t;


/* instance:    Instance number of MEP. */
/* Get the DM state of this MEP         */
u32 vtss_mep_mgmt_dm_state_get(const u32                   instance,
                               vtss_mep_mgmt_dm_state_t    *const dmr_state,
                               vtss_mep_mgmt_dm_state_t    *const dm1_state_far_to_near,
                               vtss_mep_mgmt_dm_state_t    *const dm1_state_near_to_far);                               

/* instance:    Instance number of MEP. */
/* Get the debug DM state of this MEP         */
u32 vtss_mep_mgmt_dm_db_state_get(const u32  instance,
                                  u32        *delay,
                                  u32        *delay_var);

/* instance:    Instance number of MEP. */
/* Clear DM counters for this MEP       */
u32 vtss_mep_mgmt_dm_state_clear_set(const u32    instance);

typedef struct
{
    BOOL                     enable;
    BOOL                     protection;
    vtss_mep_mgmt_period_t   period;
    vtss_mep_mgmt_domain_t   client_domain;
    BOOL                     client_dei;            /* DEI for the AIS              */
    u8                       client_prio; 
    u8                       client_level;
    u32                      client_flow_count;
    u32                      client_flows[VTSS_MEP_CLIENT_FLOWS_MAX];
} vtss_mep_mgmt_ais_conf_t;

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of ETH-AIS. */
u32 vtss_mep_mgmt_ais_conf_set(const u32                      instance,
                               const vtss_mep_mgmt_ais_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_ais_conf_get(const u32                 instance,
                               vtss_mep_mgmt_ais_conf_t  *const conf);

typedef struct
{
    BOOL                     enable;
    vtss_mep_mgmt_period_t   period;
    vtss_mep_mgmt_domain_t   client_domain;
    BOOL                     client_dei;            /* DEI for the LCK              */
    u8                       client_prio; 
    u8                       client_level;
    u32                      client_flow_count;
    u32                      client_flows[VTSS_MEP_CLIENT_FLOWS_MAX];
} vtss_mep_mgmt_lck_conf_t;

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of ETH-LCK */
u32 vtss_mep_mgmt_lck_conf_set(const u32                       instance,
                               const vtss_mep_mgmt_lck_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_lck_conf_get(const u32                 instance,
                               vtss_mep_mgmt_lck_conf_t  *const conf);


typedef enum
{
    VTSS_MEP_MGMT_PATTERN_ALL_ZERO,  /* All zero data pattern    */
    VTSS_MEP_MGMT_PATTERN_ALL_ONE,   /* All zero data pattern    */
    VTSS_MEP_MGMT_PATTERN_0XAA,      /* 10101010 data pattern    */
} vtss_mep_mgmt_pattern_t;

typedef struct
{
    BOOL                     enable;                    /* Enable/disable transmission of TST PDU	                                		*/
    BOOL                     enable_rx;                 /* Enable/disable reception and analyze of TST PDU                          		*/
    u32                      prio;                      /* Priority for the TST frame                                               		*/
    BOOL                     dei;                       /* DEI for the TST frame                                                    		*/
    u32                      mep;                       /* Peer Mep to receive TST frame                                            		*/
    u32                      rate;                      /* The transmission rate in Mbps                                            		*/
    u32                      size;                      /* Size of frame to send - max. 1518 bytes                                  		*/
    vtss_mep_mgmt_pattern_t  pattern;                   /* Pattern in TST frame                                                     		*/
    BOOL                     sequence;                  /* Sequence number will be inserted - not checked in receiver on Caracal and Jaguar */
} vtss_mep_mgmt_tst_conf_t;

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of Test frame generation. */
u32 vtss_mep_mgmt_tst_conf_set(const u32                       instance,
                               const vtss_mep_mgmt_tst_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 vtss_mep_mgmt_tst_conf_get(const u32                 instance,
                               vtss_mep_mgmt_tst_conf_t  *const conf);

/* instance:    Instance number of MEP. */
/* Clear TST counters for this MEP      */
u32 vtss_mep_mgmt_tst_state_clear_set(const u32    instance);

typedef struct
{
    u64    tx_counter;
    u64    rx_counter;
    u64    oo_counter;
    u32    rx_rate;
    u32    time;
} vtss_mep_mgmt_tst_state_t;


/* instance:    Instance number of MEP.            */
/* Get the test Result                             */
u32 vtss_mep_mgmt_tst_state_get(const u32                   instance,
                                vtss_mep_mgmt_tst_state_t   *const state);


void  vtss_mep_mgmt_up_mep_enable(void);


/* instance:    Instance number of MEP.                 */
/* Change happend that require re-configuration of the MEP instance    */
u32 vtss_mep_mgmt_change_set(const u32                   instance);



/****************************************************************************/
/*  MEP module call in interface                                            */
/****************************************************************************/

/* instance:    Instance number of MEP.             */
/* aps:         transmitted APS specific info.	    */
/* event:       transmit APS as an ERPS event.      */
/* This called by EPS/ERPS to transmit APS specific info. The first three APS is transmitted with 3.3 ms. interval and thereafter with 5 s. interval.  */
/* The length of the 'aps' array depends on the aps type configuration but is VTSS_MEP_APS_DATA_LENGTH maximum                                         */
/* If 'aps' contains the already transmitting APS, nothing happens.                                                                                    */
u32 vtss_mep_tx_aps_info_set(const u32   instance,
                             const u8    *aps,
                             const BOOL  event);


/* instance:    Instance number of MEP      */
/* state:       SSF state                   */
/* This is called by module when new SSF state is detected */
void vtss_mep_new_ssf_state(const u32    instance,
                            const BOOL   state);


/* instance:    Instance number of MEP.                 */
/* enable:      TRUE means that R-APS is forwarded      */
/* R-APS forwarding from the MEP related port, can be enabled/disabled */
u32 vtss_mep_raps_forwarding(const u32    instance,
                             const BOOL   enable);


/* instance:    Instance number of MEP.                                    */
/* enable:      TRUE means that R-APS is transmitted at 5 sec. interval    */
/* R-APS transmission from the MEP related port, can be enabled/disabled   */
u32 vtss_mep_raps_transmission(const u32    instance,
                               const BOOL   enable);


/* instance:    Instance number of MEP.       */
/* This is called by external to activate MEP calling out with SF state and received APS */
u32 vtss_mep_signal_in(const u32   instance);


/* evc_inst:    Instance number of EVC.     */
/* This is called when there is any change to a EVC configuration  */
void vtss_mep_evc_change_callback(u16  evc_inst);



/****************************************************************************/
/*  MEP module call out interface                                           */
/****************************************************************************/
/* instance:    Instance number of MEP.              */
/* domain:      Domain.                              */
/* aps_info:	Received APS specific info.	         */
/* This is called by MEP to deliver received APS specific info to EPS/ERPS     */
void vtss_mep_rx_aps_info_set(const u32                      instance,
                              const vtss_mep_mgmt_domain_t   domain,
                              const u8                       *aps);


/* instance:    Instance number of MEP.                  */
/* domain:      Domain.                                  */
/* sf_state:    SF state delivered to EPS.               */
/* sd_state:    SD state delivered to EPS.               */
/* This is called by MEP to deliver SF/SD info to EPS    */
void vtss_mep_sf_sd_state_set(const u32                      instance,
                              const vtss_mep_mgmt_domain_t   domain,
                              const BOOL                     sf_state,
                              const BOOL                     sd_state);


/* instance:    Instance number of MEP.                    */
/* domain:      Domain.                                    */
/* This is called by MEP to activate external to call in on all in functions     */
void vtss_mep_signal_out(const u32                       instance,
                         const vtss_mep_mgmt_domain_t    domain);


/* instance:    Instance number of MEP.     */
/* port:        Port number.                */
/* This is called by MEP to register on the relevant ports     */
void vtss_mep_port_register(const u32   instance,
                            const BOOL  port[VTSS_PORT_ARRAY_SIZE]);


/* Expected to return the CPU queue to get OAM frames into. */
vtss_packet_rx_queue_t vtss_mep_oam_cpu_queue_get(void);

/****************************************************************************/
/*  MEP platform call in interface                                          */
/****************************************************************************/

/* stop:    Returnvalue indicating if calling this thread has to stop.                                                   */
/* This is the thread driving timing in the MEP. Has to be called every 'timer_resolution' ms. by platform until 'stop'  */
/* Initially this thread is not called until MEP callout on vtss_mep_timer_start()                                       */
void vtss_mep_timer_thread(BOOL  *const stop);


/* This is the thread driving the state machine. Has to be call when MEP is calling out on vtss_mep_run() */ 
void vtss_mep_run_thread(void);


/* This is the thread driving poll of counters and 'hit me once' of HW CCM ACL rules */ 
void vtss_mep_ccm_thread(void);


/* timer_resolution:    This is the interval of calling vtss_mep_run_thread() in ms.    */
/* This is the initializing of MEP. Has to be called by platform                        */
u32 vtss_mep_init(const u32  timer_resolution);






/****************************************************************************/
/*  MEP platform call out interface                                         */
/****************************************************************************/

/* This is call by MEP upper logic to lock/unlock critical code protection */
void vtss_mep_crit_lock(void);

void vtss_mep_crit_unlock(void);

/* This is called by MEP upper logic when vtss_mep_run_thread(void) has to be called */
void vtss_mep_run(void);

/* This is called by MEP upper logic when vtss_mep_timer_thread(BOOL  *const stop) has to be called until 'stop' is indicated */
void vtss_mep_timer_start(void);

/* This is called by MEP upper logic in order to do debug tracing */
void vtss_mep_trace(const char   *string,
                    const u32    param1,
                    const u32    param2,
                    const u32    param3,
                    const u32    param4);

/* This is called by MEP upper logic in order to get own MAC */
void vtss_mep_mac_get(u32 port,  u8 mac[VTSS_MEP_MAC_LENGTH]);

typedef enum
{
    VTSS_MEP_TAG_NONE,
    VTSS_MEP_TAG_C,
    VTSS_MEP_TAG_S,
    VTSS_MEP_TAG_S_CUSTOM,
} vtss_mep_tag_type_t;

/* evc_inst:    Instance number of EVC.            */
/* nni:         NNI for this EVC.                  */
/* vid:         Classified VID (internal)          */
/* evid:        EVC VID (external) on the NNI      */
/* ttype:       Tag type of inner vid - only relevant if tunnel */
/* tvid:        Tunnel VID (external) on the NNI   */
/* tunnel:      The EVC is in a tunnel with tvid   */
/* tag_cnt:     Number of tagges in received OAM frames - this is only for Up-MEP */
/* This is called by MEP to get relevant data for this EVC    */
BOOL vtss_mep_evc_flow_info_get(const u32                  evc_inst,
                                BOOL                       nni[VTSS_PORT_ARRAY_SIZE],
                                BOOL                       uni[VTSS_PORT_ARRAY_SIZE],
                                u32                        *const vid,
                                u32                        *const evid,
                                vtss_mep_tag_type_t        *const ttype,
                                u32                        *const tvid,
                                BOOL                       *const tunnel,
                                u32                        *const tag_cnt);

#define VTSS_MEP_COS_ID_SIZE 8
void vtss_mep_evc_cos_id_get(const u32  evc_inst,
                             BOOL       cos_id[VTSS_MEP_COS_ID_SIZE]);

/* This is called by MEP upper logic in order to get PVID and Port type on this port */
BOOL vtss_mep_pvid_get(u32 port, u32 *pvid, vtss_mep_tag_type_t *ptype);

BOOL vtss_mep_vlan_get(const u32 vid,   BOOL nni[VTSS_PORT_ARRAY_SIZE]);

BOOL vtss_mep_port_counters_get(u32 instance,  u32 port,  vtss_port_counters_t *counters);

u32 vtss_mep_port_count(void);

#if defined(VTSS_ARCH_JAGUAR_1)
BOOL vtss_mep_evc_counters_get(u32 instance,  u32 evc,  u32 port,  vtss_evc_counters_t *counters);
#endif

/* This is called by MEP upper logic in order to control VLAN aware on a port */
BOOL vtss_mep_vlan_aware_port(u32 port,  BOOL enable);

/* This is called by MEP upper logic in order to control VLAN port member ship */
BOOL vtss_mep_vlan_member(u32 vid,  BOOL *ports,  BOOL enable);

/* This is called by MEP upper logic in order to add MAC address for PHY h/w timestamp */
BOOL vtss_mep_phy_config_mac(const u8  port,
                             const u8  is_add,
                             const u8  is_ing,
                             const u8  mac[VTSS_MEP_SUPP_MAC_LENGTH]);


void vtss_mep_rule_update_failed(u32 vid);

#define VTSS_OAM_ANY_TYPE        0xFFFFFFFF
typedef struct
{
    u32   vid;
    u32   oam;      /* VTSS_OAM_ANY_TYPE is used to indicate any OAM type - not a specific OAM type */
    u32   level;
    u32   mask;
//    u8    mac[VTSS_MEP_MAC_LENGTH];
    BOOL  ing_port[VTSS_PORT_ARRAY_SIZE];
    BOOL  eg_port[VTSS_PORT_ARRAY_SIZE];
    BOOL  cpu;
    BOOL  ts;
    BOOL  hw_ccm;
} vtss_mep_acl_entry;

void vtss_mep_acl_del(u32    vid);

BOOL vtss_mep_acl_add(vtss_mep_acl_entry    *acl);

BOOL vtss_up_mep_acl_add(u32 pag_oam, u32 pag_port, u32 customer, u32 first_egress_loop);

void vtss_mep_acl_raps_add(u32 vid,  BOOL *ports,  u8 *mac,  u32 *id);

void vtss_mep_acl_dm_add(vtss_mep_mgmt_domain_t domain,  u32 vid,  u32 pag,  u8 level);

void vtss_mep_acl_raps_del(u32 id);

void vtss_mep_acl_ccm_add(u32 instance,  u32 vid,  BOOL *port,  u32 level,  u8 *smac,  u32 pag);

void vtss_mep_acl_ccm_del(u32 instance);

void vtss_mep_acl_ccm_hit(u32 instance);

void vtss_mep_acl_ccm_count(u32 instance,  u32 *count);

void vtss_mep_acl_tst_add(u32 instance,  u32 vid,  BOOL *port,  u32 level,  u32 pag);

void vtss_mep_acl_tst_count(u32 instance,  u32 *count);

void vtss_mep_acl_tst_clear(u32 instance);

void vtss_mep_acl_evc_add(BOOL mep,  BOOL *mip_nni,  BOOL *mip_uni,  u32 mep_pag,  u32 mip_pag);

void vtss_mep_acl_mip_lbm_add(u32 vid,  u32 dmac_port,  BOOL *ports,  u32 mip_pag,  BOOL isdx_to_zero);

#endif /* _VTSS_MEP_API_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
