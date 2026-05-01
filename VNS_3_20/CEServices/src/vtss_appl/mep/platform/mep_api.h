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
 
 $Id$
 $Revision$

*/

#ifndef _MEP_API_H_
#define _MEP_API_H_

#include "main.h"
#include "vtss_types.h"
#include "vtss_mep_api.h"

#define MEP_INSTANCE_MAX   VTSS_MEP_INSTANCE_MAX
#define MEP_DM_MAX         VTSS_MEP_DM_MAX
#define MEP_EPS_MAX     5


#define MEP_RC_OK                     0
#define MEP_RC_INVALID_PARAMETER      1
#define MEP_RC_NOT_ENABLED            2
#define MEP_RC_CAST                   3
#define MEP_RC_PERIOD                 4
#define MEP_RC_PEER_CNT               5
#define MEP_RC_PEER_ID                6
#define MEP_RC_MIP                    7
#define MEP_RC_INVALID_EVC            8
#define MEP_RC_APS_EGRESS             9
#define MEP_RC_APS_DOMAIN            10
#define MEP_RC_INVALID_VID           11
#define MEP_RC_INVALID_COS_ID        12
#define MEP_RC_NO_VOE                13
#define MEP_RC_PEER_MAC              14


/****************************************************************************/
/*  MEP management interface                                                */
/****************************************************************************/

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 mep_mgmt_conf_set(const u32                    instance,
                      const vtss_mep_mgmt_conf_t   *const conf);

/* instance:    Instance number of MEP.                                                         */
/* mac:         MAC of this MEP instance                                                        */
/* eps_count:   Number of related EPS (MEP_EPS_MAX)                                             */
/* eps_inst:    EPS instance numbers. Only eps_inst[0] receives APS. All receives SF/SD state   */
/* conf:        Configuration data for this instance                                            */
u32 mep_mgmt_conf_get(const u32               instance,
                      u8                      *const mac,
                      u32                     *const eps_count,
                      u16                     *const eps_inst,
                      vtss_mep_mgmt_conf_t    *const conf);



/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* A CCM session is enabled/disabled.                   */
u32 mep_mgmt_cc_conf_set(const u32                        instance,
                         const vtss_mep_mgmt_cc_conf_t    *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 mep_mgmt_cc_conf_get(const u32                 instance,
                         vtss_mep_mgmt_cc_conf_t   *const conf);



/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enabled/disable of frame Loss Measurement.           */
u32 mep_mgmt_lm_conf_set(const u32                      instance,
                         const vtss_mep_mgmt_lm_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 mep_mgmt_lm_conf_get(const u32                instance,
                         vtss_mep_mgmt_lm_conf_t  *const conf);



/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of frame Delay Measurement.           */
u32 mep_mgmt_dm_conf_set(const u32                      instance,
                         const vtss_mep_mgmt_dm_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 mep_mgmt_dm_conf_get(const u32                 instance,
                         vtss_mep_mgmt_dm_conf_t  *const conf);



/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* A APS session is enabled/disabled. This is always unicast and the MAC    */
u32 mep_mgmt_aps_conf_set(const u32                      instance,
                          const vtss_mep_mgmt_aps_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 mep_mgmt_aps_conf_get(const u32                 instance,
                          vtss_mep_mgmt_aps_conf_t  *const conf);



/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of Link Trace. */
u32 mep_mgmt_lt_conf_set(const u32                      instance,
                         const vtss_mep_mgmt_lt_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 mep_mgmt_lt_conf_get(const u32                 instance,
                         vtss_mep_mgmt_lt_conf_t  *const conf);




/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
/* Enable/disable of Loop Back. */
u32 mep_mgmt_lb_conf_set(const u32                      instance,
                         const vtss_mep_mgmt_lb_conf_t  *const conf);

/* instance:    Instance number of MEP.                 */
/* conf:        Configuration data for this instance    */
u32 mep_mgmt_lb_conf_get(const u32                 instance,
                         vtss_mep_mgmt_lb_conf_t  *const conf);


/* instance:		Instance number of MEP.      */
/* Set AIS configuration of this MEP             */
u32 mep_mgmt_ais_conf_set (const u32 instance, 
                           const vtss_mep_mgmt_ais_conf_t *const config);

/* instance:		Instance number of MEP.      */
/* Get AIS configuration of this MEP             */
u32 mep_mgmt_ais_conf_get (const u32 instance, 
                           vtss_mep_mgmt_ais_conf_t *const config);


/* instance:		Instance number of MEP.      */
/* Set LCK configuration of this MEP             */
u32 mep_mgmt_lck_conf_set (const u32 instance, 
                           const vtss_mep_mgmt_lck_conf_t *const config);

/* instance:		Instance number of MEP.      */
/* Get LCK configuration of this MEP             */
u32 mep_mgmt_lck_conf_get (const u32 instance, 
                           vtss_mep_mgmt_lck_conf_t *const config);


/* instance:		Instance number of MEP.      */
/* Set TST configuration of this MEP             */
u32 mep_mgmt_tst_conf_set (const u32 instance, 
                           const vtss_mep_mgmt_tst_conf_t *const config);

/* instance:		Instance number of MEP.      */
/* Get TST configuration of this MEP             */
u32 mep_mgmt_tst_conf_get (const u32 instance, 
                           vtss_mep_mgmt_tst_conf_t *const config);


/* instance:    Instance number of MEP.          */
/* Get the Link Trace Result                     */
u32 mep_mgmt_lt_state_get(const u32                  instance,
                          vtss_mep_mgmt_lt_state_t   *const state);

/* instance:    Instance number of MEP.          */
/* Get the Loop Back Result                      */
u32 mep_mgmt_lb_state_get(const u32                  instance,
                          vtss_mep_mgmt_lb_state_t   *const state);

/* instance:    Instance number of MEP.          */
/* Get the ETH state of this MEP                 */
u32 mep_mgmt_state_get(const u32              instance,
                       vtss_mep_mgmt_state_t  *const state);

/* instance:		Instance number of MEP.      */
/* Get the LM counter state of this MEP          */
u32 mep_mgmt_lm_state_get(const u32                   instance,
                          vtss_mep_mgmt_lm_state_t    *const state);

/* instance:    Instance number of MEP.          */
/* Clear LM counters for this MEP                */
u32 mep_mgmt_lm_state_clear_set(const u32    instance);

/* instance:		Instance number of MEP.      */
/* Get the DM state of this MEP                  */
u32 mep_mgmt_dm_state_get(const u32                   instance,
                          vtss_mep_mgmt_dm_state_t    *const dmr_state,
                          vtss_mep_mgmt_dm_state_t    *const dm1_state_far_to_near,
                          vtss_mep_mgmt_dm_state_t    *const dm1_state_near_to_far);

/* instance:		Instance number of MEP.      */
/* Get the DM state of this MEP                  */
u32 mep_mgmt_dm_state_clear_set(const u32    instance);

/* instance:		Instance number of MEP.      */
/* Get the debug DM state of this MEP            */
u32 mep_mgmt_dm_db_state_get(const u32                   instance,
                             u32                         *delay,
                             u32                         *delay_var);

/* instance:    Instance number of MEP.          */
/* Get the TST Result                            */
u32 mep_mgmt_tst_state_get(const u32                   instance,
                           vtss_mep_mgmt_tst_state_t   *const state);


/* instance:    Instance number of MEP.          */
/* Clear TST counters for this MEP               */
u32 mep_mgmt_tst_state_clear_set(const u32    instance);



void mep_mgmt_up_mep_enable(BOOL  enable);
/****************************************************************************/
/*  MEP module interface                                                    */
/****************************************************************************/

typedef enum
{
    MEP_EPS_TYPE_INV,
    MEP_EPS_TYPE_ELPS,
    MEP_EPS_TYPE_ERPS,
} mep_eps_type_t;


/* instance:    Instance number of MEP.               */
/* eps_inst:    Instance number of calling EPS/REPS   */
/* eps_type:    It is a ELPS or ERPS register         */
/* enable:      TRUE means register is enabled        */
/* Receiving and transmitting of APS from/to this MEP, can be enabled/disabled */
u32 mep_eps_aps_register(const u32              instance,
                         const u32              eps_inst,
                         const mep_eps_type_t   eps_type,
                         const BOOL             enable);


/* instance:    Instance number of MEP.               */
/* eps_inst:    Instance number of calling EPS/REPS   */
/* eps_type:    It is a ELPS or ERPS register         */
/* enable:      TRUE means register is enabled        */
/* Receiving of SF state from this MEP, can be enabled/disabled */
u32 mep_eps_sf_register(const u32              instance,
                        const u32              eps_inst,
                        const mep_eps_type_t   eps_type,
                        const BOOL             enable);


/* instance:    Instance number of MEP.             */
/* eps_inst:    Instance number of calling EPS/REPS */
/* aps:         transmitted APS specific info.      */
/* event:       transmit APS as an ERPS event.      */
/* This called by EPS to transmit APS info. The first three APS is transmitted with 3.3 ms. interval and thereafter with 5 s. interval.  */
/* The length of the 'aps' array must be VTSS_MEP_APS_DATA_LENGTH minimum to satisfy LINT                                                */
u32 mep_tx_aps_info_set(const u32   instance,
                        const u32   eps_inst,
                        const u8    *aps,
                        const BOOL  event);


/* instance:    Instance number of MEP.             */
/* eps_inst:    Instance number of calling EPS/REPS */
/* This is called by EPS to activate MEP calling out with SF state and received APS */
u32 mep_signal_in(const u32   instance,
                  const u32   eps_inst);


/* instance:    Instance number of MEP.               */
/* eps_inst:    Instance number of calling EPS/REPS   */
/* enable:      TRUE means that R-APS is forwarded    */
/* R-APS forwarding to the MEP related port, can be enabled/disabled */
u32 mep_raps_forwarding(const u32     instance,
                        const u32     eps_inst,
                        const BOOL    enable);


/* instance:    Instance number of MEP.                                    */
/* eps_inst:    Instance number of calling EPS/REPS                        */
/* enable:      TRUE means that R-APS is transmitted at 5 sec. interval    */
/* R-APS transmission from the MEP related port, can be enabled/disabled   */
u32 mep_raps_transmission(const u32     instance,
                          const u32     eps_inst,
                          const BOOL    enable);



typedef enum
{
    MEP_EPS_ARCHITECTURE_1F1,
    MEP_EPS_ARCHITECTURE_1P1
} mep_eps_architecture_t;

void mep_port_protection_create(mep_eps_architecture_t architecture,  const u32 w_port,  const u32 p_port);

void mep_port_protection_change(const u32         w_port,
                                const u32         p_port,
                                const BOOL        active);

void mep_port_protection_delete(const u32 w_port,  const u32 p_port);

void mep_ring_protection_block(const u32 port,  BOOL block);

/* Initialize module */
vtss_rc mep_init(vtss_init_data_t *data);

#endif /* _MEP_API_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
