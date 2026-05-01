/*

 Vitesse Switch Software.

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

#include <ucd-snmp/config.h>
#include <ucd-snmp/mibincl.h>   /* Standard set of SNMP includes */

#include "vtss_snmp_api.h"
#include "rfc3411_framework.h"
#if defined(SNMP_UCD_SNMP_MIBS_SUPPORTED)
#include "ucd_snmp_rfc3411_framework.h"
#elif defined(SNMP_NET_SNMP_MIBS_SUPPORTED)
#include "net_snmp_rfc3411_framework.h"
#endif /* SNMP_UCD_SNMP_MIBS_SUPPORTED */

#if RFC3411_SUPPORTED_FRAMEWORK_SNMPENGINE
/*
 * Initializes the MIBObjects module
 */
void init_snmpEngine(void)
{
#if defined(SNMP_UCD_SNMP_MIBS_SUPPORTED)
    ucd_snmp_init_snmpEngine();
#elif defined(SNMP_NET_SNMP_MIBS_SUPPORTED)
    net_snmp_init_snmpEngine();
#endif /* SNMP_UCD_SNMP_MIBS_SUPPORTED */
}
#endif /* RFC3411_SUPPORTED_FRAMEWORK_SNMPENGINE */

