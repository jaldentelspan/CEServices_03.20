/*

 Vitesse API software.

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

#ifndef _VTSS_PHY_TS_H_
#define _VTSS_PHY_TS_H_

#include <vtss_options.h>
#include <vtss_types.h>

#if defined(VTSS_FEATURE_PHY_TIMESTAMP)

/**
 * \brief set the the 1588 block CSR registers.
 *
 * \param inst         [IN]      handle to an API instance
 * \param port_no      [IN]      port number
 * \param blk_id       [IN]      1588 CSR block Index [0-7]
 * \param csr_address  [IN]      1588 CSR block Register Offset [0x00 - 0x7f]
 * \param value        [IN]      32 bit value
 *
 * \return Return code.
 **/

extern vtss_rc vtss_phy_1588_csr_reg_write(const vtss_inst_t inst,
                                    const vtss_port_no_t port_no,
                                    const u32 blk_id,
                                    const u16 csr_address,
                                    const u32 *const value);

/**
 * \brief get the the 1588 block CSR registers.
 *
 * \param inst         [IN]      handle to an API instance
 * \param port_no      [IN]      port number
 * \param blk_id       [IN]      1588 CSR block Index [0-7]
 * \param csr_address  [IN]      1588 CSR block Register Offset [0x00 - 0x7f]
 * \param value        [OUT]     32 bit value
 *
 * \return Return code.
 **/

extern vtss_rc vtss_phy_1588_csr_reg_read(const vtss_inst_t inst,
                                   const vtss_port_no_t port_no,
                                   const u32 blk_id,
                                   const u16 csr_address,
                                   u32 *const value);

#ifdef VTSS_FEATURE_WARM_START
/**
 * \brief This is used to sync the vtss_state to PHY.
 *
 * \param port_no      [IN]      port number
 *
 * \return Return code.
 **/
extern vtss_rc vtss_phy_ts_sync(const vtss_port_no_t port_no);
#endif /* VTSS_FEATURE_WARM_START */

#endif /* VTSS_FEATURE_PHY_TIMESTAMP */

#endif /* _VTSS_PHY_TS_H_ */

