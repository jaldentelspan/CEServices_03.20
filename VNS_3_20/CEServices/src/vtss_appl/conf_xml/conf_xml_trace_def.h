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

#ifndef _VTSS_CONF_XML_TRACE_DEF_H_
#define _VTSS_CONF_XML_TRACE_DEF_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_CONF_XML

#define VTSS_TRACE_GRP_DEFAULT 0
#define VTSS_TRACE_GRP_MIRROR  1
#define VTSS_TRACE_GRP_POE     2
#define VTSS_TRACE_GRP_SYNCE   3
#define VTSS_TRACE_GRP_EPS     4
#define VTSS_TRACE_GRP_MEP     5
#define VTSS_TRACE_GRP_AUTH    6
#define VTSS_TRACE_GRP_VLAN    7
#define VTSS_TRACE_GRP_LLDP    8
#define VTSS_TRACE_GRP_QOS     9
#define VTSS_TRACE_GRP_CRIT   10
#define VTSS_TRACE_GRP_EEE    11
#define VTSS_TRACE_GRP_LED_POW_REDUC 12
#define VTSS_TRACE_GRP_THERMAL_PROTECT 13
#define TRACE_GRP_CNT         14

#include <vtss_trace_api.h>

#endif /* _VTSS_CONF_XML_TRACE_DEF_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
