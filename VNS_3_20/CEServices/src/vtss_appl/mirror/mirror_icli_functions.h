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

 $Id$
 $Revision$

*/

/**
 * \file
 * \brief mirror icli functions
 * \details This header file describes mirror iCLI functions
 */


#ifndef _VTSS_ICLI_MIRROR_H_
#define _VTSS_ICLI_MIRROR_H_

#include "icli_porting.h"
#include "icli_types.h"


/**
 * \brief Function for setting the destination port.
 *
 * \param session_id [IN]  The session id.
 * \param monitor [IN] Always TRUE
 * \param destination [IN] Always TRUE
 * \param interface [IN] Always TRUE
 * \param port_type [IN] ICLI port type (Not used at the moment)
 * \param port_id [IN] The port that shall be mirrored.
 * \param no [IN] TRUE if destination port shall be disabled.
 *
 * \return None.
 **/
void mirror_destination(i32 session_id, BOOL monitor, BOOL destination, BOOL interface, icli_port_type_t port_type, icli_switch_port_range_t port_id, BOOL no);

/**
 * \brief Function for setting source mirror ports.
 *
 * \param session_id [IN]  The session id.
 * \param monitor [IN] Always TRUE
 * \param interface [IN] TRUE is only some ports should be shown, else FALSE
 * \param source [IN] Always TRUE
 * \param port_type [IN] ICLI port type (Not used at the moment)
 * \param port_list [IN] ICLI port list with the ports to set.
 * \param cpu [IN] TRUE if it is the CPU that shall be set.
 * \param cpu_switch_range [IN] List of switch id for which to shown the CPU mirroring configuration.
 * \param both [IN] TRUE if mirroring shall be enabled in both ingress and egress direction.
 * \param rx [IN] TRUE if mirroring shall be enabled in ingress direction.
 * \param tx [IN] TRUE if mirroring shall be enabled in egress direction.
 * \param no [IN] TRUE if port shall no be mirroring
 *
 * \return None.
 **/
void mirror_source(i32 session_id, BOOL monitor, BOOL source, BOOL interface, icli_port_type_t port_type,
                   icli_stack_port_range_t *port_list, BOOL cpu, icli_range_t *cpu_switch_range, BOOL both, BOOL rx, BOOL tx, BOOL no);



/**
 * \brief Function for initializing mirror icli cfg
 * \return vtss rc error code.
 **/
vtss_rc mirror_lib_icfg_init(void);


/**
 * \brief Function for getting if CPU mirroring is supported
 * \return See AN1047.
 **/
BOOL mirror_cpu_runtime(u32                session_id,
                        icli_runtime_ask_t ask,
                        icli_runtime_t     *runtime);

/**
 * \brief Function for getting the CPU switch ID range (if supported)
 * \return See AN1047.
 **/
BOOL mirror_switch_id_runtime(u32                session_id,
                              icli_runtime_ask_t ask,
                              icli_runtime_t     *runtime);


#endif /* _VTSS_ICLI_MIRROR_H_ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
