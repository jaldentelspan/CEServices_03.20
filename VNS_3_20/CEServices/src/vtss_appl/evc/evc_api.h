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

#ifndef _EVC_API_H_
#define _EVC_API_H_

/* Number of EVCs */
#ifndef EVC_ID_COUNT
#define EVC_ID_COUNT  VTSS_EVCS
#endif /* EVC_ID_COUNT */

/* Number of ECEs */
#ifndef EVC_ECE_COUNT
#define EVC_ECE_COUNT VTSS_EVCS
#endif /* EVC_ECE_COUNT */

/* Number of policers */
#ifndef EVC_POL_COUNT
#define EVC_POL_COUNT VTSS_EVC_POLICERS
#endif /* EVC_POL_COUNT */

/* ================================================================= *
 *  Management API
 * ================================================================= */

/* - Global Configuration ------------------------------------------ */

/* EVC global configuration */
typedef struct {
    BOOL port_check; /* Enable UNI/NNI port check */
} evc_mgmt_global_conf_t;

/* Get global configuration */
vtss_rc evc_mgmt_conf_get(evc_mgmt_global_conf_t *conf);

/* Set global configuration */
vtss_rc evc_mgmt_conf_set(evc_mgmt_global_conf_t *conf);

/* - Port Configuration -------------------------------------------- */

/* EVC port configuration using VTSS API structures */
typedef struct {
    vtss_evc_port_conf_t       conf;
    vtss_packet_rx_port_conf_t reg;
} evc_mgmt_port_conf_t;

/* Get port configuration */
vtss_rc evc_mgmt_port_conf_get(vtss_port_no_t port_no, evc_mgmt_port_conf_t *conf);

/* Set port configuration */
vtss_rc evc_mgmt_port_conf_set(vtss_port_no_t port_no, evc_mgmt_port_conf_t *conf);

/* - Policer Configuration ----------------------------------------- */

/* EVC policer configuration using the VTSS API structure */
typedef struct {
    vtss_evc_policer_conf_t conf;
} evc_mgmt_policer_conf_t;

#define EVC_POLICER_RATE_MAX  10000000 /* CIR/EIR maximum 10 Gbps */
#define EVC_POLICER_LEVEL_MAX 100000   /* CBS/EBS maximum 100 kB */

/* Get policer configuration */
vtss_rc evc_mgmt_policer_conf_get(vtss_evc_policer_id_t policer_id,
                                  evc_mgmt_policer_conf_t *conf);

/* Set port configuration */
vtss_rc evc_mgmt_policer_conf_set(vtss_evc_policer_id_t policer_id,
                                  evc_mgmt_policer_conf_t *conf);

/* - EVC Configuration --------------------------------------------- */

/* EVC entry configuration using the VTSS API structure */
typedef struct {
    vtss_evc_conf_t conf;
    BOOL            conflict; /* VTSS API failure indication */
} evc_mgmt_conf_t;

/* Add EVC */
vtss_rc evc_mgmt_add(vtss_evc_id_t evc_id, evc_mgmt_conf_t *conf);

/* Delete EVC */
vtss_rc evc_mgmt_del(vtss_evc_id_t evc_id);

/* Get EVC or get next EVC */
#define EVC_ID_FIRST VTSS_EVC_ID_NONE
vtss_rc evc_mgmt_get(vtss_evc_id_t *evc_id, evc_mgmt_conf_t *conf, BOOL next);

/* - ECE Configuration --------------------------------------------- */

/* ECE entry configuration using the VTSS API structure.
   The key.tag.vid.mask must be 0 (any) or 0xfff (specific) */
typedef struct {
    vtss_ece_t conf;
    BOOL       conflict; /* VTSS API failure indication */
} evc_mgmt_ece_conf_t;

/* Add ECE */
vtss_rc evc_mgmt_ece_add(vtss_ece_id_t next_id, evc_mgmt_ece_conf_t *conf);

/* Delete ECE */
vtss_rc evc_mgmt_ece_del(vtss_ece_id_t id);

/* Get ECE or get next ECE */
#define EVC_ECE_ID_FIRST VTSS_ECE_ID_LAST
vtss_rc evc_mgmt_ece_get(vtss_ece_id_t id, evc_mgmt_ece_conf_t *conf, BOOL next);

/* - Port information ---------------------------------------------- */

/* EVC port information */
typedef struct {
    u16 nni_count; /* Number of EVCs, where the port is an NNI */
    u16 uni_count; /* Number of ECEs, where the port is an UNI */
} evc_port_info_t;

/* Get port information */
vtss_rc evc_mgmt_port_info_get(evc_port_info_t info[VTSS_PORT_ARRAY_SIZE]);

/* ================================================================= *
 *  Control API
 * ================================================================= */

/* EVC change callback */
typedef void (*evc_change_callback_t)(vtss_evc_id_t evc_id);

/* EVC change callback registration */
vtss_rc evc_change_register(evc_change_callback_t callback);

/* ================================================================= *
 *  Initialization
 * ================================================================= */

/* Initialize module */
vtss_rc evc_init(vtss_init_data_t *data);

#endif /* _EVC_API_H_ */
