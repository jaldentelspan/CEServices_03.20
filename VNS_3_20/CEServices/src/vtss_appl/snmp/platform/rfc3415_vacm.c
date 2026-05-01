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
#ifdef SNMP_SUPPORT_V3
#include "rfc3415_vacm.h"
#if defined(SNMP_UCD_SNMP_MIBS_SUPPORTED)
#include "ucd_snmp_rfc3415_vacm.h"
#elif defined(SNMP_NET_SNMP_MIBS_SUPPORTED)
#include "net_snmp_rfc3415_vacm.h"
#endif /* SNMP_UCD_SNMP_MIBS_SUPPORTED */
#endif /* SNMP_SUPPORT_V3 */

#if RFC3415_SUPPORTED_VACMCONTEXTTABLE
/*
 * Initializes the MIBObjects module
 */
void init_vacmContextTable(void)
{
#if defined(SNMP_UCD_SNMP_MIBS_SUPPORTED)
    ucd_snmp_init_vacmContextTable();
#elif defined(SNMP_NET_SNMP_MIBS_SUPPORTED)
    net_snmp_init_vacmContextTable();
#endif /* SNMP_UCD_SNMP_MIBS_SUPPORTED */
}
#endif /* RFC3415_SUPPORTED_VACMCONTEXTTABLE */

#if RFC3415_SUPPORTED_VACMSECURITYTOGROUPTABLE
/*
 * Initializes the MIBObjects module
 */
void init_vacmSecurityToGroupTable(void)
{
#if defined(SNMP_UCD_SNMP_MIBS_SUPPORTED)
    ucd_snmp_init_vacmSecurityToGroupTable();
#elif defined(SNMP_NET_SNMP_MIBS_SUPPORTED)
    net_snmp_init_vacmSecurityToGroupTable();
#endif /* SNMP_UCD_SNMP_MIBS_SUPPORTED */
}
#endif /* RFC3415_SUPPORTED_VACMSECURITYTOGROUPTABLE */

#if RFC3415_SUPPORTED_VACMACCESSTABLE
/*
 * Initializes the MIBObjects module
 */
void init_vacmAccessTable(void)
{
#if defined(SNMP_UCD_SNMP_MIBS_SUPPORTED)
    ucd_snmp_init_vacmAccessTable();
#elif defined(SNMP_NET_SNMP_MIBS_SUPPORTED)
    net_snmp_init_vacmAccessTable();
#endif /* SNMP_UCD_SNMP_MIBS_SUPPORTED */
}
#endif /* RFC3415_SUPPORTED_VACMACCESSTABLE */

#if RFC3415_SUPPORTED_VACMMIBVIEWS
/*
 * Initializes the MIBObjects module
 */
void init_vacmMIBViews(void)
{
#if defined(SNMP_UCD_SNMP_MIBS_SUPPORTED)
    ucd_snmp_init_vacmMIBViews();
#elif defined(SNMP_NET_SNMP_MIBS_SUPPORTED)
    net_snmp_init_vacmMIBViews();
#endif /* SNMP_UCD_SNMP_MIBS_SUPPORTED */
}
#endif /* RFC3415_SUPPORTED_VACMMIBVIEWS */

