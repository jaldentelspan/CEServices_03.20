/*

 Vitesse Switch API software.

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

#ifndef _VTSS_SYNCE_H_
#define _VTSS_SYNCE_H_

#include "synce_custom_clock_api.h"
#include "main.h"
#include "port_api.h"


#define SYNCE_NOMINATED_MAX   CLOCK_INPUT_MAX           /* maximum number of norminated ports */
#define SYNCE_PRIORITY_MAX    CLOCK_INPUT_MAX           /* maximum number of priorities */
#define SYNCE_PORT_COUNT      VTSS_FRONT_PORT_COUNT     /* number of PHY ports possible running SSM */
#define SYNCE_WTR_MAX         12                        /* maximum value for wtr timer */
#define SYNCE_HOLDOFF_MIN      3                        /* minimum (except zero) value for hold off timer */
#define SYNCE_HOLDOFF_MAX     18                        /* maximum value for hold off timer */
#define SYNCE_HOLDOFF_TEST    100                       /* Test value for hold off timer */


#define SYNCE_RC_OK                 0
#define SYNCE_RC_INVALID_PARAMETER  1
#define SYNCE_RC_NOM_PORT           2
#define SYNCE_RC_SELECTION          3
#define SYNCE_RC_INVALID_PORT       4

        
typedef enum
{
    SYNCE_MGMT_ANEG_NONE,
    SYNCE_MGMT_ANEG_PREFERED_SLAVE,
    SYNCE_MGMT_ANEG_PREFERED_MASTER,
    SYNCE_MGMT_ANEG_FORCED_SLAVE,
} synce_mgmt_aneg_mode_t;

typedef enum
{
    QL_NONE,    /* No overwrite */
    QL_PRC,     /* Primary Reference Clock */
    QL_SSUA,    /* Synchronization supply unit */
    QL_SSUB,    /* Synchronization supply unit */
    QL_EEC2,    /* SyncE EEC option 2 */
    QL_EEC1,    /* SyncE EEC option 1 */
    QL_DNU,     /* Do Not Use */
    QL_INV,     /* receiving invalid SSM (not defined) - NOT possible to set */
    QL_FAIL,    /* NOT received SSM in five seconds - NOT possible to set */
    QL_LINK     /* Invalind SSM due to link down - NOT possible to set */
} synce_mgmt_quality_level_t;

uint synce_mgmt_nominated_source_set(const uint                         source,          /* nominated source - range is 0 - SYNCE_NOMINATED_MAX */
                                     const BOOL                         enable,          /* Enable/disable of this source */
                                     const uint                         port_no,         /* norminated PHY port */
                                     const synce_mgmt_aneg_mode_t       aneg_mode,       /* master/slave mode of port when nominated and not selected */
                                     const uint                         holdoff_time,    /* Hold Off timer value in 100ms (3 - 18) */
                                     const synce_mgmt_quality_level_t   ssm_overwrite);  /* rx overwrite SSM */



typedef enum
{
    SYNCE_MGMT_SELECTOR_MODE_MANUEL,
    SYNCE_MGMT_SELECTOR_MODE_MANUEL_TO_SELECTED,
    SYNCE_MGMT_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE,
    SYNCE_MGMT_SELECTOR_MODE_AUTOMATIC_REVERTIVE,
    SYNCE_MGMT_SELECTOR_MODE_FORCED_HOLDOVER,
    SYNCE_MGMT_SELECTOR_MODE_FORCED_FREE_RUN
} synce_mgmt_selection_mode_t;

uint synce_mgmt_nominated_selection_mode_set(synce_mgmt_selection_mode_t       mode,
                                             const uint                        source,         /* manuel selected nominated source - range is 0 - SYNCE_NOMINATED_MAX */
                                             const uint                        wtr_time,       /* WTR timer value in minutes */
                                             const synce_mgmt_quality_level_t  ssm_holdover,   /* tx overwrite SSM used when clock controller is hold over */
                                             const synce_mgmt_quality_level_t  ssm_freerun);   /* tx overwrite SSM used when clock controller is free run */

                                                
uint synce_mgmt_nominated_priority_set(const uint     source,     /* nominated source - range is 0 - SYNCE_NOMINATED_MAX */
                                       const uint     priority);  /* priority 0 is highest */


uint synce_mgmt_ssm_set(const uint    port_no,
                        const BOOL    ssm_enabled);


uint synce_mgmt_wtr_clear_set(const uint     source);     /* nominated source - range is 0 - SYNCE_NOMINATED_MAX */





typedef enum
{
    SYNCE_MGMT_SELECTOR_STATE_LOCKED,
    SYNCE_MGMT_SELECTOR_STATE_HOLDOVER,
    SYNCE_MGMT_SELECTOR_STATE_FREERUN
} synce_mgmt_selector_state_t;

typedef struct
{
    BOOL    locs[SYNCE_NOMINATED_MAX];
    BOOL    fos[SYNCE_NOMINATED_MAX];
    BOOL    ssm[SYNCE_NOMINATED_MAX];
    BOOL    wtr[SYNCE_NOMINATED_MAX];
    BOOL    losx;
    BOOL    lol;
    BOOL    dhold;
} synce_mgmt_alarm_state_t;

uint synce_mgmt_clock_state_get(synce_mgmt_selector_state_t    *const selector_state,
                                uint                           *const source,               /* locked nominated source - range is 0 - SYNCE_NOMINATED_MAX */
                                synce_mgmt_alarm_state_t       *const alarm_state);




uint synce_mgmt_port_state_get(const uint                     port_no,
                               synce_mgmt_quality_level_t     *const ssm_rx,
                               synce_mgmt_quality_level_t     *const ssm_tx,
                               BOOL                           *const master);




typedef struct
{
    synce_mgmt_selection_mode_t    selection_mode;                      /* selection mode */
    uint                           source;                              /* nominated source for manuel selection mode */
    uint                           wtr_time;                            /* WTR timer value in minutes */
    synce_mgmt_quality_level_t     ssm_holdover;                        /* tx overwrite SSM used when clock controller is hold over */
    synce_mgmt_quality_level_t     ssm_freerun;                         /* tx overwrite SSM used when clock controller is free run */
    BOOL                           nominated[SYNCE_NOMINATED_MAX];      /* false = sources is not norminated */
    uint                           port[SYNCE_NOMINATED_MAX];           /* port number of the norminated sources */
    uint                           priority[SYNCE_NOMINATED_MAX];       /* priority of the nominated source */
    synce_mgmt_aneg_mode_t         aneg_mode[SYNCE_NOMINATED_MAX];      /* Autonogotiation mode auto-master-slave */
    synce_mgmt_quality_level_t     ssm_overwrite[SYNCE_NOMINATED_MAX];  /* SSM overwirte quality */
    uint                           holdoff_time[SYNCE_NOMINATED_MAX];   /* Hold Off timer value in 100ms (3 - 18). Zero means no hold off */
} synce_mgmt_clock_conf_blk_t;

uint synce_mgmt_clock_config_get(synce_mgmt_clock_conf_blk_t *const clock_config);




typedef struct
{
    BOOL                           ssm_enabled[SYNCE_PORT_COUNT];   /* quality level via SSM enabled */
} synce_mgmt_port_conf_blk_t;

uint synce_mgmt_port_config_get(synce_mgmt_port_conf_blk_t  *const port_config);


//uint synce_mgmt_source_port_get(const BOOL   **src_port);
typedef BOOL (*synce_src_port)[SYNCE_PORT_COUNT];
synce_src_port synce_mgmt_source_port_get(void);
                              
uint synce_mgmt_register_get(const uint  reg,
                             uint        *const value);


uint synce_mgmt_register_set(const uint reg,
                             const uint value);


/* Initialize module */
vtss_rc synce_init(vtss_init_data_t *data);

#endif /* _VTSS_SYNCE_H_ */

/****************************************************************************/
/*                                                                                                                                                   */
/*  End of file.                                                                                                                             */
/*                                                                                                                                                   */
/****************************************************************************/
