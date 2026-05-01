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

#ifndef _VLAN_ICFG_H_
#define _VLAN_ICFG_H_

/*
******************************************************************************

    Include files

******************************************************************************
*/

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define VLAN_NO_FORM_TEXT                       "no "
#define VLAN_GLOBAL_ETYPE_TEXT                  "vlan ethertype s-custom-port"
#define VLAN_CONF_MODE_NAME_TEXT                "name"
#define VLAN_PORT_MODE_TRUNK_TEXT               "switchport mode trunk"
#define VLAN_PORT_MODE_ACCESS_TEXT              "switchport mode access"
#define VLAN_PORT_MODE_HYBRID_TEXT              "switchport mode hybrid"
#define VLAN_PORT_ACCESS_VLAN_TEXT              "switchport access vlan"
#define VLAN_PORT_TRUNK_VLAN_TEXT               "switchport trunk native vlan"
#define VLAN_PORT_HYBRID_VLAN_TEXT              "switchport hybrid native vlan"
#define VLAN_PORT_TRUNK_ALLOWED_VLAN_TEXT       "switchport trunk allowed vlan"
#define VLAN_PORT_HYBRID_ALLOWED_VLAN_TEXT      "switchport hybrid allowed vlan"
#define VLAN_PORT_HYBRID_FRAME_TYPE_TEXT        "switchport hybrid acceptable-frame-type"
#define VLAN_PORT_HYBRID_ING_FILTER_TEXT        "switchport hybrid ingress-filtering"
#define VLAN_PORT_HYBRID_PORT_TYPE_TEXT         "switchport hybrid port-type"
#define VLAN_PORT_HYBRID_TX_TAG_TEXT            "switchport hybrid egress-tag"

#define VLAN_DEFAULT_TPID                       0x88A8

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/**
 * \file vlan_icfg.h
 * \brief This file defines the interface to the VLAN module's ICFG commands.
 */

/**
  * \brief Initialization function.
  *
  * Call once, preferably from the INIT_CMD_INIT section of
  * the module's _init() function.
  */
vtss_rc VLAN_icfg_init(void);

#endif /* _VLAN_ICFG_H_ */
