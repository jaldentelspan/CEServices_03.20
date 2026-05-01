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

#ifndef _VTSS_CONF_XML_CUSTOM_H_
#define _VTSS_CONF_XML_CUSTOM_H_

/* Configuration file platform IDs */
#define CONF_XML_PLATFORM_SPARX_II_16        1 /* 16x1G */
#define CONF_XML_PLATFORM_SPARX_II_24        2 /* 24x1G */
#define CONF_XML_PLATFORM_ESTAX_34           3 /* 24x1G + 2x5G */
#define CONF_XML_PLATFORM_SPARX_III_10       4 /* 10x1G */
#define CONF_XML_PLATFORM_SPARX_III_18       5 /* 18x1G */
#define CONF_XML_PLATFORM_SPARX_III_24       6 /* 24x1G */
#define CONF_XML_PLATFORM_SPARX_III_26       7 /* 26x1G */
#define CONF_XML_PLATFORM_CARACAL_1          8
#define CONF_XML_PLATFORM_CARACAL_2          9
#define CONF_XML_PLATFORM_JAGUAR_1           10
#define CONF_XML_PLATFORM_LYNX_1             11
#define CONF_XML_PLATFORM_E_STAX_III_48      12 /* 24x1G + 2x10G/12G */
#define CONF_XML_PLATFORM_E_STAX_III_68      13 /* 24x1G + 2x10G + 2x10G/12G */
#define CONF_XML_PLATFORM_E_STAX_III_24_DUAL 14 /* Dual 24x1G + 2x10G/12G */
#define CONF_XML_PLATFORM_E_STAX_III_68_DUAL 15 /* Dual 24x1G + 2x10G + 2x10G/12G */
#define CONF_XML_PLATFORM_JAGUAR_1_DUAL      16 /* Dual Jaguar-1 */
#define CONF_XML_PLATFORM_SERVAL             17 /* Serval */
#define CONF_XML_PLATFORM_SPARX_III_11       18 /* SparX-III-11 */
#define CONF_XML_PLATFORM_SERVAL_LITE        19 /* VSC7416 */

#if !defined(CONF_XML_PLATFORM_ID)
#if defined(VTSS_CHIP_SPARX_II_16)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SPARX_II_16
#elif defined(VTSS_CHIP_SPARX_II_24)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SPARX_II_24
#elif defined(VTSS_CHIP_E_STAX_34)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_ESTAX_34
#elif defined(VTSS_CHIP_SPARX_III_10) || defined(VTSS_CHIP_SPARX_III_10_01)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SPARX_III_10
#elif defined(VTSS_CHIP_SPARX_III_18)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SPARX_III_18
#elif defined(VTSS_CHIP_SPARX_III_24)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SPARX_III_24
#elif defined(VTSS_CHIP_SPARX_III_26)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SPARX_III_26
#elif defined(VTSS_CHIP_CARACAL_1)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_CARACAL_1
#elif defined(VTSS_CHIP_CARACAL_2)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_CARACAL_2
#elif defined(VTSS_CHIP_JAGUAR_1)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_JAGUAR_1
#elif defined(VTSS_CHIP_LYNX_1)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_LYNX_1
#elif defined(VTSS_CHIP_E_STAX_III_48)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_E_STAX_III_48
#elif defined(VTSS_CHIP_E_STAX_III_68)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_E_STAX_III_68
#elif defined(VTSS_CHIP_E_STAX_III_24_DUAL)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_E_STAX_III_24_DUAL
#elif defined(VTSS_CHIP_E_STAX_III_68_DUAL)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_E_STAX_III_68_DUAL
#elif defined(VTSS_CHIP_JAGUAR_1_DUAL)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_JAGUAR_1_DUAL
#elif defined(VTSS_CHIP_SERVAL)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SERVAL
#elif defined(VTSS_CHIP_SERVAL_LITE)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SERVAL_LITE
#elif defined(VTSS_CHIP_SPARX_III_11)
#define CONF_XML_PLATFORM_ID CONF_XML_PLATFORM_SPARX_III_11
#else
#error CONF_XML_PLATFORM_ID must be defined
#endif
#endif /* CONF_XML_PLATFORM_ID */


#endif /* _VTSS_CONF_XML_CUSTOM_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
