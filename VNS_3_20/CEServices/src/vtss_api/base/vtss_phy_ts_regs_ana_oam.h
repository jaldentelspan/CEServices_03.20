#ifndef _VTSS_PHY_TS_REGS_ANA_OAM_H_
#define _VTSS_PHY_TS_REGS_ANA_OAM_H_

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

#include "vtss_phy_ts_regs_common.h"

/*********************************************************************** 
 *
 * Target: \a ANA_OAM
 *
 * \see vtss_target_ANA_OAM_e
 *
 * Analyzer Engine Configuration Registers
 *
 ***********************************************************************/

/**
 * Register Group: \a ANA_OAM:ETH1_NXT_PROTOCOL_A
 *
 * Ethernet Next Protocol Configuration
 */


/** 
 * \brief Ethernet Next Protocol Register
 *
 * \details
 * Register: \a ANA_OAM:ETH1_NXT_PROTOCOL_A:ETH1_NXT_PROTOCOL_A
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_NXT_PROTOCOL_A  VTSS_IOREG(0x0)

/** 
 * \brief
 * Points to the next comparator block after this Ethernet block
 *
 * \details 
 * 0 = Reserved
 * 1 = Ethernet comparator 2
 * 2 = IP/UDP/ACH comparator 1
 * 3 = IP/UDP/ACH comparator 2
 * 4 = MPLS comparator
 * 5 = PTP/OAM comparator
 * 6,7 = Reserved
 *
 * Field: VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_NXT_PROTOCOL_A . ETH1_NXT_COMPARATOR_A
 */
#define  VTSS_F_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_NXT_PROTOCOL_A_ETH1_NXT_COMPARATOR_A(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_NXT_PROTOCOL_A_ETH1_NXT_COMPARATOR_A     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_NXT_PROTOCOL_A_ETH1_NXT_COMPARATOR_A(x)  VTSS_EXTRACT_BITFIELD(x,0,3)


/** 
 * \brief VLAN TPID Configuration
 *
 * \details
 * Register: \a ANA_OAM:ETH1_NXT_PROTOCOL_A:ETH1_VLAN_TPID_CFG_A
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_VLAN_TPID_CFG_A  VTSS_IOREG(0x1)

/** 
 * \brief
 * Configurable VLAN TPID (S or B-tag).
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_VLAN_TPID_CFG_A . ETH1_VLAN_TPID_CFG_A
 */
#define  VTSS_F_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_VLAN_TPID_CFG_A_ETH1_VLAN_TPID_CFG_A(x)  VTSS_ENCODE_BITFIELD(x,16,16)
#define  VTSS_M_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_VLAN_TPID_CFG_A_ETH1_VLAN_TPID_CFG_A     VTSS_ENCODE_BITMASK(16,16)
#define  VTSS_X_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_VLAN_TPID_CFG_A_ETH1_VLAN_TPID_CFG_A(x)  VTSS_EXTRACT_BITFIELD(x,16,16)


/** 
 * \brief Ethernet Tag Mode
 *
 * \details
 * Register: \a ANA_OAM:ETH1_NXT_PROTOCOL_A:ETH1_TAG_MODE_A
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_TAG_MODE_A  VTSS_IOREG(0x2)

/** 
 * \brief
 * This bit enables the presence of PBB.  
 * 
 * The I-tag match bits are programmed in the ETH1_VLAN_TAG_RANGE
 * registers.  The mask bits are progrogrammed in the ETH1_VLAN_TAG2
 * registers.  A B-tag if present is configured in the ETH1_VLAN_TAG1
 * registers.
 *
 * \details 
 * 0 = PBB not enabled
 * 1 = Always expect PBB, last tag is always an I-tag
 *
 * Field: VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_TAG_MODE_A . ETH1_PBB_ENA_A
 */
#define  VTSS_F_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_TAG_MODE_A_ETH1_PBB_ENA_A  VTSS_BIT(0)


/** 
 * \brief Ethertype Match Register
 *
 * \details
 * Register: \a ANA_OAM:ETH1_NXT_PROTOCOL_A:ETH1_ETYPE_MATCH_A
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_ETYPE_MATCH_A  VTSS_IOREG(0x3)

/** 
 * \brief
 * If the Ethertype/length field is an Ethertype, then this register is
 * compared against the value.	If the field is a length, the length value
 * is not checked.
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_ETYPE_MATCH_A . ETH1_ETYPE_MATCH_A
 */
#define  VTSS_F_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_ETYPE_MATCH_A_ETH1_ETYPE_MATCH_A(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_ETYPE_MATCH_A_ETH1_ETYPE_MATCH_A     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_OAM_ETH1_NXT_PROTOCOL_A_ETH1_ETYPE_MATCH_A_ETH1_ETYPE_MATCH_A(x)  VTSS_EXTRACT_BITFIELD(x,0,16)

/**
 * Register Group: \a ANA_OAM:ETH1_NXT_PROTOCOL_B
 *
 * Ethernet Next Protocol Configuration
 */


/** 
 * \brief Ethernet Next Protocol Register
 *
 * \details
 * Register: \a ANA_OAM:ETH1_NXT_PROTOCOL_B:ETH1_NXT_PROTOCOL_B
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_NXT_PROTOCOL_B  VTSS_IOREG(0x10)

/** 
 * \brief
 * Points to the next comparator block after this Ethernet block
 *
 * \details 
 * 0 = Reserved
 * 1 = Ethernet comparator 2
 * 2 = IP/UDP/ACH comparator 1
 * 3 = IP/UDP/ACH comparator 2
 * 4 = MPLS comparator
 * 5 = PTP/OAM comparator
 * 6,7 = Reserved
 *
 * Field: VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_NXT_PROTOCOL_B . ETH1_NXT_COMPARATOR_B
 */
#define  VTSS_F_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_NXT_PROTOCOL_B_ETH1_NXT_COMPARATOR_B(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_NXT_PROTOCOL_B_ETH1_NXT_COMPARATOR_B     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_NXT_PROTOCOL_B_ETH1_NXT_COMPARATOR_B(x)  VTSS_EXTRACT_BITFIELD(x,0,3)


/** 
 * \details
 * Register: \a ANA_OAM:ETH1_NXT_PROTOCOL_B:ETH1_VLAN_TPID_CFG_B
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_VLAN_TPID_CFG_B  VTSS_IOREG(0x11)

/** 
 * \brief
 * Configurable VLAN TPID (S or B-tag).
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_VLAN_TPID_CFG_B . ETH1_VLAN_TPID_CFG_B
 */
#define  VTSS_F_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_VLAN_TPID_CFG_B_ETH1_VLAN_TPID_CFG_B(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_VLAN_TPID_CFG_B_ETH1_VLAN_TPID_CFG_B     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_VLAN_TPID_CFG_B_ETH1_VLAN_TPID_CFG_B(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief Ethernet Tag Mode
 *
 * \details
 * Register: \a ANA_OAM:ETH1_NXT_PROTOCOL_B:ETH1_TAG_MODE_B
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_TAG_MODE_B  VTSS_IOREG(0x12)

/** 
 * \brief
 * This bit enables the presence of PBB.  
 * 
 * The I-tag match bits are programmed in the ETH1_VLAN_TAG_RANGE
 * registers.  The mask bits are progrogrammed in the ETH1_VLAN_TAG2
 * registers.  A B-tag if present is configured in the ETH1_VLAN_TAG1
 * registers.
 *
 * \details 
 * 0 = PBB not enabled
 * 1 = Always expect PBB, last tag is always an I-tag
 *
 * Field: VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_TAG_MODE_B . ETH1_PBB_ENA_B
 */
#define  VTSS_F_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_TAG_MODE_B_ETH1_PBB_ENA_B  VTSS_BIT(0)


/** 
 * \brief Ethertype Match Register
 *
 * \details
 * Register: \a ANA_OAM:ETH1_NXT_PROTOCOL_B:ETH1_ETYPE_MATCH_B
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_ETYPE_MATCH_B  VTSS_IOREG(0x13)

/** 
 * \brief
 * If the Ethertype/length field is an Ethertype, then this register is
 * compared against the value.	If the field is a length, the length value
 * is not checked.
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_ETYPE_MATCH_B . ETH1_ETYPE_MATCH_B
 */
#define  VTSS_F_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_ETYPE_MATCH_B_ETH1_ETYPE_MATCH_B(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_ETYPE_MATCH_B_ETH1_ETYPE_MATCH_B     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_OAM_ETH1_NXT_PROTOCOL_B_ETH1_ETYPE_MATCH_B_ETH1_ETYPE_MATCH_B(x)  VTSS_EXTRACT_BITFIELD(x,0,16)

/**
 * Register Group: \a ANA_OAM:ETH1_FLOW_CFG
 *
 * Not documented
 */


/** 
 * \brief Ethernet Flow Enable
 *
 * \details
 * Register: \a ANA_OAM:ETH1_FLOW_CFG:ETH1_FLOW_ENABLE
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH1_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE(gi)  VTSS_IOREG_IX(0x20, gi, 16, 0, 0)

/** 
 * \brief
 * Indicates which next-protocol configuration group is valid with this
 * flow
 *
 * \details 
 * 0 = Associate this flow with next-protocol group A
 * 1 = Associate this flow with next-protocol group B
 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE . ETH1_NXT_PROT_GRP_SEL
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE_ETH1_NXT_PROT_GRP_SEL  VTSS_BIT(16)

/** 
 * \details 
 * bit 0 = Flow valid for channel 0
 * bit 1 = Flow valid for channel 1

 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE . ETH1_CHANNEL_MASK
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE_ETH1_CHANNEL_MASK(x)  VTSS_ENCODE_BITFIELD(x,8,2)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE_ETH1_CHANNEL_MASK     VTSS_ENCODE_BITMASK(8,2)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE_ETH1_CHANNEL_MASK(x)  VTSS_EXTRACT_BITFIELD(x,8,2)

/** 
 * \brief
 * Flow enable
 *
 * \details 
 * 0 = Flow is disabled
 * 1 = Flow is enabled
 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE . ETH1_FLOW_ENABLE
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_FLOW_ENABLE_ETH1_FLOW_ENABLE  VTSS_BIT(0)


/** 
 * \brief Ethernet Protocol Match Mode
 *
 * \details
 * Register: \a ANA_OAM:ETH1_FLOW_CFG:ETH1_MATCH_MODE
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH1_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE(gi)  VTSS_IOREG_IX(0x20, gi, 16, 0, 1)

/** 
 * \details 
 * 0 = VLAN range checking disabled
 * 1 = VLAN range checking on tag 1
 * 2 = VLAN range checking on tag 2 (not supported with PBB)
 * 3 = reserved
 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE . ETH1_VLAN_TAG_MODE
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_TAG_MODE(x)  VTSS_ENCODE_BITFIELD(x,12,2)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_TAG_MODE     VTSS_ENCODE_BITMASK(12,2)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_TAG_MODE(x)  VTSS_EXTRACT_BITFIELD(x,12,2)

/** 
 * \brief
 * This register is only used if ETH1_VLAN_VERIFY_ENA = 1
 *
 * \details 
 * If PBB not enabled:
 * 0 = C tag (TPID of 0x8100)
 * 1 = S tag (match to CONF_VLAN_TPID)
 * 
 * If PBB enabled:
 * 0,1 = I tag (use range registers)
 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE . ETH1_VLAN_TAG2_TYPE
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_TAG2_TYPE  VTSS_BIT(9)

/** 
 * \brief
 * This register is only used if ETH1_VLAN_VERIFY_ENA = 1
 *
 * \details 
 * 0 = C tag (TPID of 0x8100)
 * 1 = S or B tag (match to CONF_VLAN_TPID)
 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE . ETH1_VLAN_TAG1_TYPE
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_TAG1_TYPE  VTSS_BIT(8)

/** 
 * \brief
 * This register is only used if ETH1_VLAN_VERIFY_ENA = 1
 *
 * \details 
 * 0 = No VLAN tags (not valid for PBB)
 * 1 = 1 VLAN tag (for PBB this would be the I-tag)
 * 2 = 2 VLAN tags (for PBB expect a B-tag and an I-tag)
 * 3 = Reserved
 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE . ETH1_VLAN_TAGS
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_TAGS(x)  VTSS_ENCODE_BITFIELD(x,6,2)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_TAGS     VTSS_ENCODE_BITMASK(6,2)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_TAGS(x)  VTSS_EXTRACT_BITFIELD(x,6,2)

/** 
 * \details 
 * 0 = Parse for VLAN tags, do not check values.  For PBB the I-tag is
 * always checked.
 * 1 = Verify configured VLAN tag configuration.

 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE . ETH1_VLAN_VERIFY_ENA
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_VLAN_VERIFY_ENA  VTSS_BIT(4)

/** 
 * \brief
 * When checking for presence of SNAP/LLC based upon ETH1_MATCH_MODE, this
 * field indicates if SNAP & 3-byte LLC is expected to be present
 *
 * \details 
 * 0 = Only Ethernet type II supported, no SNAP/LLC
 * 1 = Ethernet type II & Ethernet type I with SNAP/LLC, determine if
 * SNAP/LLC is present or not.	Type I always assumes that SNAP/LLC is
 * present
 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE . ETH1_ETHERTYPE_MODE
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_MATCH_MODE_ETH1_ETHERTYPE_MODE  VTSS_BIT(0)


/** 
 * \brief Ethernet Address Match Part 1
 *
 * \details
 * Register: \a ANA_OAM:ETH1_FLOW_CFG:ETH1_ADDR_MATCH_1
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH1_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_1(gi)  VTSS_IOREG_IX(0x20, gi, 16, 0, 2)


/** 
 * \brief Ethernet Address Match Part 2
 *
 * \details
 * Register: \a ANA_OAM:ETH1_FLOW_CFG:ETH1_ADDR_MATCH_2
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH1_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2(gi)  VTSS_IOREG_IX(0x20, gi, 16, 0, 3)

/** 
 * \brief
 * Selects how the addresses are matched.  Multiple bits can be set at once
 *
 * \details 
 * bit 0 = Full 48-bit address match
 * bit 1 = Match any unicast address
 * bit 2 = Match any muliticast address
 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2 . ETH1_ADDR_MATCH_MODE
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_MODE(x)  VTSS_ENCODE_BITFIELD(x,20,3)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_MODE     VTSS_ENCODE_BITMASK(20,3)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_MODE(x)  VTSS_EXTRACT_BITFIELD(x,20,3)

/** 
 * \brief
 * Selects which address to match
 *
 * \details 
 * 0 = Match the Destination Address
 * 1 = Match the Source Address
 * 2 = Match either the Source of Destination Address
 * 3 = Reserved

 *
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2 . ETH1_ADDR_MATCH_SELECT
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_SELECT(x)  VTSS_ENCODE_BITFIELD(x,16,2)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_SELECT     VTSS_ENCODE_BITMASK(16,2)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_SELECT(x)  VTSS_EXTRACT_BITFIELD(x,16,2)

/** 
 * \brief
 * Last 16 bits of the Ethernet address match field
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2 . ETH1_ADDR_MATCH_2
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_2(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_2     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_ADDR_MATCH_2_ETH1_ADDR_MATCH_2(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief Ethernet VLAN Tag Range Match Register
 *
 * \details
 * Register: \a ANA_OAM:ETH1_FLOW_CFG:ETH1_VLAN_TAG_RANGE_I_TAG
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH1_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG(gi)  VTSS_IOREG_IX(0x20, gi, 16, 0, 4)

/** 
 * \brief
 * If PBB mode is not enabled, then this register contains the upper range
 * of the VLAN tag range match. 
 * 
 * If PBB mode is enabled, then this register contains the upper 12 bits of
 * the I-tag
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG . ETH1_VLAN_TAG_RANGE_UPPER
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG_ETH1_VLAN_TAG_RANGE_UPPER(x)  VTSS_ENCODE_BITFIELD(x,16,12)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG_ETH1_VLAN_TAG_RANGE_UPPER     VTSS_ENCODE_BITMASK(16,12)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG_ETH1_VLAN_TAG_RANGE_UPPER(x)  VTSS_EXTRACT_BITFIELD(x,16,12)

/** 
 * \brief
 * If PBB mode is not enabled, then this register contains the lower range
 * of the VLAN tag range match.
 * 
 * If PBB mode is enabled, then this register contians the lower 12 bits of
 * the I-tag
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG . ETH1_VLAN_TAG_RANGE_LOWER
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG_ETH1_VLAN_TAG_RANGE_LOWER(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG_ETH1_VLAN_TAG_RANGE_LOWER     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG_RANGE_I_TAG_ETH1_VLAN_TAG_RANGE_LOWER(x)  VTSS_EXTRACT_BITFIELD(x,0,12)


/** 
 * \brief VLAN Tag 1 Match/Mask
 *
 * \details
 * Register: \a ANA_OAM:ETH1_FLOW_CFG:ETH1_VLAN_TAG1
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH1_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1(gi)  VTSS_IOREG_IX(0x20, gi, 16, 0, 5)

/** 
 * \brief
 * Mask value for VLAN tag 1
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1 . ETH1_VLAN_TAG1_MASK
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1_ETH1_VLAN_TAG1_MASK(x)  VTSS_ENCODE_BITFIELD(x,16,12)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1_ETH1_VLAN_TAG1_MASK     VTSS_ENCODE_BITMASK(16,12)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1_ETH1_VLAN_TAG1_MASK(x)  VTSS_EXTRACT_BITFIELD(x,16,12)

/** 
 * \brief
 * Match value for the first VLAN tag
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1 . ETH1_VLAN_TAG1_MATCH
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1_ETH1_VLAN_TAG1_MATCH(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1_ETH1_VLAN_TAG1_MATCH     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG1_ETH1_VLAN_TAG1_MATCH(x)  VTSS_EXTRACT_BITFIELD(x,0,12)


/** 
 * \brief Match/Mask For VLAN Tag 2 or I-Tag Match
 *
 * \details
 * Register: \a ANA_OAM:ETH1_FLOW_CFG:ETH1_VLAN_TAG2_I_TAG
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH1_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG(gi)  VTSS_IOREG_IX(0x20, gi, 16, 0, 6)

/** 
 * \brief
 * When PBB is not enabled, the mask field for VLAN tag 2
 * 
 * When PBB is enabled, the upper 12 bits of the I-tag mask
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG . ETH1_VLAN_TAG2_MASK
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG_ETH1_VLAN_TAG2_MASK(x)  VTSS_ENCODE_BITFIELD(x,16,12)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG_ETH1_VLAN_TAG2_MASK     VTSS_ENCODE_BITMASK(16,12)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG_ETH1_VLAN_TAG2_MASK(x)  VTSS_EXTRACT_BITFIELD(x,16,12)

/** 
 * \brief
 * When PBB is not enabled, the match field for VLAN Tag 2
 * 
 * When PBB is enabled, the lower 12 bits of the I-tag mask field
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG . ETH1_VLAN_TAG2_MATCH
 */
#define  VTSS_F_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG_ETH1_VLAN_TAG2_MATCH(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG_ETH1_VLAN_TAG2_MATCH     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_ANA_OAM_ETH1_FLOW_CFG_ETH1_VLAN_TAG2_I_TAG_ETH1_VLAN_TAG2_MATCH(x)  VTSS_EXTRACT_BITFIELD(x,0,12)

/**
 * Register Group: \a ANA_OAM:ETH2_NXT_PROTOCOL_A
 *
 * Ethernet Next Protocol Configuration
 */


/** 
 * \brief Ethernet Next Protocol Register
 *
 * \details
 * Register: \a ANA_OAM:ETH2_NXT_PROTOCOL_A:ETH2_NXT_PROTOCOL_A
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_NXT_PROTOCOL_A  VTSS_IOREG(0xa0)

/** 
 * \brief
 * Points to the next comparator block after this Ethernet block.   If this
 * comparator blockk is not used, this field must be set to "0".
 *
 * \details 
 * 0 = Comparator block not used
 * 1 = Ethernet comparator 2
 * 2 = IP/UDP/ACH comparator 1
 * 3 = IP/UDP/ACH comparator 2
 * 4 = MPLS comparator
 * 5 = PTP/OAM comparator
 * 6,7 = Reserved
 *
 * Field: VTSS_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_NXT_PROTOCOL_A . ETH2_NXT_COMPARATOR_A
 */
#define  VTSS_F_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_NXT_PROTOCOL_A_ETH2_NXT_COMPARATOR_A(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_NXT_PROTOCOL_A_ETH2_NXT_COMPARATOR_A     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_NXT_PROTOCOL_A_ETH2_NXT_COMPARATOR_A(x)  VTSS_EXTRACT_BITFIELD(x,0,3)


/** 
 * \brief VLAN TPID Configuration
 *
 * \details
 * Register: \a ANA_OAM:ETH2_NXT_PROTOCOL_A:ETH2_VLAN_TPID_CFG_A
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_VLAN_TPID_CFG_A  VTSS_IOREG(0xa1)

/** 
 * \brief
 * Configurable S-tag TPID
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_VLAN_TPID_CFG_A . ETH2_VLAN_TPID_CFG_A
 */
#define  VTSS_F_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_VLAN_TPID_CFG_A_ETH2_VLAN_TPID_CFG_A(x)  VTSS_ENCODE_BITFIELD(x,16,16)
#define  VTSS_M_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_VLAN_TPID_CFG_A_ETH2_VLAN_TPID_CFG_A     VTSS_ENCODE_BITMASK(16,16)
#define  VTSS_X_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_VLAN_TPID_CFG_A_ETH2_VLAN_TPID_CFG_A(x)  VTSS_EXTRACT_BITFIELD(x,16,16)


/** 
 * \brief Ethertype Match Register
 *
 * \details
 * Register: \a ANA_OAM:ETH2_NXT_PROTOCOL_A:ETH2_ETYPE_MATCH_A
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_ETYPE_MATCH_A  VTSS_IOREG(0xa2)

/** 
 * \brief
 * If the Ethertype/length field is an Ethertype, then this register is
 * compared against the value.	If the field is a length, the length value
 * is not checked.
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_ETYPE_MATCH_A . ETH2_ETYPE_MATCH_A
 */
#define  VTSS_F_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_ETYPE_MATCH_A_ETH2_ETYPE_MATCH_A(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_ETYPE_MATCH_A_ETH2_ETYPE_MATCH_A     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_OAM_ETH2_NXT_PROTOCOL_A_ETH2_ETYPE_MATCH_A_ETH2_ETYPE_MATCH_A(x)  VTSS_EXTRACT_BITFIELD(x,0,16)

/**
 * Register Group: \a ANA_OAM:ETH2_FLOW_CFG
 *
 * Not documented
 */


/** 
 * \brief Ethernet Flow Enable
 *
 * \details
 * Register: \a ANA_OAM:ETH2_FLOW_CFG:ETH2_FLOW_ENABLE
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH2_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_FLOW_ENABLE(gi)  VTSS_IOREG_IX(0xc0, gi, 16, 0, 0)

/** 
 * \details 
 * bit 0 = Flow valid for channel 0
 * bit 1 = Flow valid for channel 1

 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_FLOW_ENABLE . ETH2_CHANNEL_MASK
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_FLOW_ENABLE_ETH2_CHANNEL_MASK(x)  VTSS_ENCODE_BITFIELD(x,8,2)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_FLOW_ENABLE_ETH2_CHANNEL_MASK     VTSS_ENCODE_BITMASK(8,2)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_FLOW_ENABLE_ETH2_CHANNEL_MASK(x)  VTSS_EXTRACT_BITFIELD(x,8,2)

/** 
 * \brief
 * Flow enable.  If this comparator block is not used, all flow enable bits
 * must be set to 0.
 *
 * \details 
 * 0 = Flow is disabled
 * 1 = Flow is enabled
 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_FLOW_ENABLE . ETH2_FLOW_ENABLE
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_FLOW_ENABLE_ETH2_FLOW_ENABLE  VTSS_BIT(0)


/** 
 * \brief Ethernet Protocol Match Mode
 *
 * \details
 * Register: \a ANA_OAM:ETH2_FLOW_CFG:ETH2_MATCH_MODE
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH2_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE(gi)  VTSS_IOREG_IX(0xc0, gi, 16, 0, 1)

/** 
 * \details 
 * 0 = VLAN range checking disabled
 * 1 = VLAN range checking on tag 1
 * 2 = VLAN range checking on tag 2 (not supported with PBB)
 * 3 = reserved
 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE . ETH2_VLAN_TAG_MODE
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_TAG_MODE(x)  VTSS_ENCODE_BITFIELD(x,12,2)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_TAG_MODE     VTSS_ENCODE_BITMASK(12,2)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_TAG_MODE(x)  VTSS_EXTRACT_BITFIELD(x,12,2)

/** 
 * \brief
 * This register is only used if ETH1_VLAN_VERIFY_ENA = 1
 *
 * \details 
 * 0 = C tag (TPID of 0x8100)
 * 1 = S tag (match to CONF_VLAN_TPID)
 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE . ETH2_VLAN_TAG2_TYPE
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_TAG2_TYPE  VTSS_BIT(9)

/** 
 * \brief
 * This register is only used if ETH1_VLAN_VERIFY_ENA = 1
 *
 * \details 
 * 0 = C tag (TPID of 0x8100)
 * 1 = S or B tag (match to CONF_VLAN_TPID)
 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE . ETH2_VLAN_TAG1_TYPE
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_TAG1_TYPE  VTSS_BIT(8)

/** 
 * \brief
 * This register is only used if ETH2_VLAN_VERIFY_ENA = 1
 *
 * \details 
 * 0 = No VLAN tags
 * 1 = 1 VLAN tag
 * 2 = 2 VLAN tags
 * 3 = Reserved
 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE . ETH2_VLAN_TAGS
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_TAGS(x)  VTSS_ENCODE_BITFIELD(x,6,2)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_TAGS     VTSS_ENCODE_BITMASK(6,2)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_TAGS(x)  VTSS_EXTRACT_BITFIELD(x,6,2)

/** 
 * \details 
 * 0 = Parse for VLAN tags, do not check values.
 * 1 = Verify configured VLAN tag configuration.

 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE . ETH2_VLAN_VERIFY_ENA
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_VLAN_VERIFY_ENA  VTSS_BIT(4)

/** 
 * \brief
 * When checking for presence of SNAP/LLC based upon ETH1_MATCH_MODE, this
 * field indicates if SNAP & 3-byte LLC is expected to be present
 *
 * \details 
 * 0 = Only Ethernet type II supported, no SNAP/LLC
 * 1 = Ethernet type II & Ethernet type I with SNAP/LLC, determine if
 * SNAP/LLC is present or not.	Type I always assumes that SNAP/LLC is
 * present
 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE . ETH2_ETHERTYPE_MODE
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_MATCH_MODE_ETH2_ETHERTYPE_MODE  VTSS_BIT(0)


/** 
 * \brief Ethernet Address Match Part 1
 *
 * \details
 * Register: \a ANA_OAM:ETH2_FLOW_CFG:ETH2_ADDR_MATCH_1
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH2_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_1(gi)  VTSS_IOREG_IX(0xc0, gi, 16, 0, 2)


/** 
 * \brief Ethernet Address Match Part 2
 *
 * \details
 * Register: \a ANA_OAM:ETH2_FLOW_CFG:ETH2_ADDR_MATCH_2
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH2_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2(gi)  VTSS_IOREG_IX(0xc0, gi, 16, 0, 3)

/** 
 * \brief
 * Selects how the addresses are matched.  Multiple bits can be set at once
 *
 * \details 
 * bit 0 = Full 48-bit address match
 * bit 1 = Match any unicast address
 * bit 2 = Match any muliticast address
 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2 . ETH2_ADDR_MATCH_MODE
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_MODE(x)  VTSS_ENCODE_BITFIELD(x,20,3)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_MODE     VTSS_ENCODE_BITMASK(20,3)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_MODE(x)  VTSS_EXTRACT_BITFIELD(x,20,3)

/** 
 * \brief
 * Selects which address to match
 *
 * \details 
 * 0 = Match the Destination Address
 * 1 = Match the Source Address
 * 2 = Match either the Source of Destination Address
 * 3 = Reserved

 *
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2 . ETH2_ADDR_MATCH_SELECT
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_SELECT(x)  VTSS_ENCODE_BITFIELD(x,16,2)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_SELECT     VTSS_ENCODE_BITMASK(16,2)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_SELECT(x)  VTSS_EXTRACT_BITFIELD(x,16,2)

/** 
 * \brief
 * Last 16 bits of the Ethernet address match field
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2 . ETH2_ADDR_MATCH_2
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_2(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_2     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_ADDR_MATCH_2_ETH2_ADDR_MATCH_2(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief Ethernet VLAN Tag Range Match Register
 *
 * \details
 * Register: \a ANA_OAM:ETH2_FLOW_CFG:ETH2_VLAN_TAG_RANGE_I_TAG
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH2_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG(gi)  VTSS_IOREG_IX(0xc0, gi, 16, 0, 4)

/** 
 * \brief
 * This register contains the upper range of the VLAN tag range match. 

 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG . ETH2_VLAN_TAG_RANGE_UPPER
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG_ETH2_VLAN_TAG_RANGE_UPPER(x)  VTSS_ENCODE_BITFIELD(x,16,12)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG_ETH2_VLAN_TAG_RANGE_UPPER     VTSS_ENCODE_BITMASK(16,12)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG_ETH2_VLAN_TAG_RANGE_UPPER(x)  VTSS_EXTRACT_BITFIELD(x,16,12)

/** 
 * \brief
 * This register contains the lower range of the VLAN tag range match.

 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG . ETH2_VLAN_TAG_RANGE_LOWER
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG_ETH2_VLAN_TAG_RANGE_LOWER(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG_ETH2_VLAN_TAG_RANGE_LOWER     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG_RANGE_I_TAG_ETH2_VLAN_TAG_RANGE_LOWER(x)  VTSS_EXTRACT_BITFIELD(x,0,12)


/** 
 * \brief VLAN Tag 1 Match/Mask
 *
 * \details
 * Register: \a ANA_OAM:ETH2_FLOW_CFG:ETH2_VLAN_TAG1
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH2_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1(gi)  VTSS_IOREG_IX(0xc0, gi, 16, 0, 5)

/** 
 * \brief
 * Mask value for VLAN tag 1
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1 . ETH2_VLAN_TAG1_MASK
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1_ETH2_VLAN_TAG1_MASK(x)  VTSS_ENCODE_BITFIELD(x,16,12)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1_ETH2_VLAN_TAG1_MASK     VTSS_ENCODE_BITMASK(16,12)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1_ETH2_VLAN_TAG1_MASK(x)  VTSS_EXTRACT_BITFIELD(x,16,12)

/** 
 * \brief
 * Match value for the first VLAN tag
 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1 . ETH2_VLAN_TAG1_MATCH
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1_ETH2_VLAN_TAG1_MATCH(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1_ETH2_VLAN_TAG1_MATCH     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG1_ETH2_VLAN_TAG1_MATCH(x)  VTSS_EXTRACT_BITFIELD(x,0,12)


/** 
 * \brief Match/Mask For VLAN Tag 2 or I-Tag Match
 *
 * \details
 * Register: \a ANA_OAM:ETH2_FLOW_CFG:ETH2_VLAN_TAG2_I_TAG
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: ETH2_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG(gi)  VTSS_IOREG_IX(0xc0, gi, 16, 0, 6)

/** 
 * \brief
 * Mask field for VLAN tag 2

 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG . ETH2_VLAN_TAG2_MASK
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG_ETH2_VLAN_TAG2_MASK(x)  VTSS_ENCODE_BITFIELD(x,16,12)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG_ETH2_VLAN_TAG2_MASK     VTSS_ENCODE_BITMASK(16,12)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG_ETH2_VLAN_TAG2_MASK(x)  VTSS_EXTRACT_BITFIELD(x,16,12)

/** 
 * \brief
 * Match field for VLAN Tag 2

 *
 * \details 
 * Field: VTSS_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG . ETH2_VLAN_TAG2_MATCH
 */
#define  VTSS_F_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG_ETH2_VLAN_TAG2_MATCH(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG_ETH2_VLAN_TAG2_MATCH     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_ANA_OAM_ETH2_FLOW_CFG_ETH2_VLAN_TAG2_I_TAG_ETH2_VLAN_TAG2_MATCH(x)  VTSS_EXTRACT_BITFIELD(x,0,12)

/**
 * Register Group: \a ANA_OAM:MPLS_NXT_COMPARATOR_A
 *
 * MPLS Next Protocol Register
 */


/** 
 * \brief MPLS Next Protocol Comparator Register
 *
 * \details
 * Register: \a ANA_OAM:MPLS_NXT_COMPARATOR_A:MPLS_NXT_COMPARATOR_A
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 */
#define VTSS_ANA_OAM_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A  VTSS_IOREG(0x140)

/** 
 * \brief
 * Indicates the presence of a control word after the last label
 *
 * \details 
 * 0 = There is no control ward after the last label
 * 1 = There is a control word after the last label
 *
 * Field: VTSS_ANA_OAM_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A . MPLS_CTL_WORD_A
 */
#define  VTSS_F_ANA_OAM_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A_MPLS_CTL_WORD_A  VTSS_BIT(16)

/** 
 * \brief
 * Points to the next comparator stage.   If this comparator blockk is not
 * used, this field must be set to "0".
 *
 * \details 
 * 0 = Comparator block not used
 * 1 = Ethernet comparator 2
 * 2 = IP/UDP/ACH comparator 1
 * 3 = IP/UDP/ACH comparator 2
 * 4 = Reserved
 * 5 = PTP/OAM comparator
 * 6,7 = Reserved
 *
 * Field: VTSS_ANA_OAM_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A . MPLS_NXT_COMPARATOR_A
 */
#define  VTSS_F_ANA_OAM_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_ANA_OAM_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_ANA_OAM_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A_MPLS_NXT_COMPARATOR_A(x)  VTSS_EXTRACT_BITFIELD(x,0,3)

/**
 * Register Group: \a ANA_OAM:MPLS_FLOW_CFG
 *
 * Not documented
 */


/** 
 * \brief MPLS Flow Control Register
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_FLOW_CONTROL
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 0)

/** 
 * \details 
 * bit 0 = Flow valid for channel 0
 * bit 1 = Flow valid for channel 1

 *
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL . MPLS_CHANNEL_MASK
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL_MPLS_CHANNEL_MASK(x)  VTSS_ENCODE_BITFIELD(x,24,2)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL_MPLS_CHANNEL_MASK     VTSS_ENCODE_BITMASK(24,2)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL_MPLS_CHANNEL_MASK(x)  VTSS_EXTRACT_BITFIELD(x,24,2)

/** 
 * \brief
 * Defines the allowable stack depths for searches.   The direction that
 * the stack is referenced is determined by the setting of MPLS_REF_PNT
 * 
 * The following table maps bits to stack depths:
 *
 * \details 
 * bit 0 = stack allowed to be 1 label deep
 * bit 1 = stack allowed to be 2 labels deep
 * bit 2 = stack allowed to be 3 labels deep
 * bit 3 = stack allowed to be 4 labels deep
 *
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL . MPLS_STACK_DEPTH
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL_MPLS_STACK_DEPTH(x)  VTSS_ENCODE_BITFIELD(x,16,4)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL_MPLS_STACK_DEPTH     VTSS_ENCODE_BITMASK(16,4)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL_MPLS_STACK_DEPTH(x)  VTSS_EXTRACT_BITFIELD(x,16,4)

/** 
 * \brief
 * Defines the search direction for label matching
 *
 * \details 
 * 0 = All searching is performed starting from the top of the stack
 * 1 = All searching is performed from the end of the stack
 *
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL . MPLS_REF_PNT
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL_MPLS_REF_PNT  VTSS_BIT(4)

/** 
 * \brief
 * Flow enable.  If this comparator block is not used, all flow enable bits
 * must be set to 0.
 *
 * \details 
 * 0 = Flow is disabled
 * 1 = Flow is enabled
 *
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL . MPLS_FLOW_ENA
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_FLOW_CONTROL_MPLS_FLOW_ENA  VTSS_BIT(0)


/** 
 * \brief MPLS Label 0 Match Range Lower Value
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_LABEL_RANGE_LOWER_0
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_0(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 1)

/** 
 * \brief
 * Lower value for label 0 match range

 *
 * \details 
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_0 . MPLS_LABEL_RANGE_LOWER_0
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_0_MPLS_LABEL_RANGE_LOWER_0(x)  VTSS_ENCODE_BITFIELD(x,0,20)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_0_MPLS_LABEL_RANGE_LOWER_0     VTSS_ENCODE_BITMASK(0,20)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_0_MPLS_LABEL_RANGE_LOWER_0(x)  VTSS_EXTRACT_BITFIELD(x,0,20)


/** 
 * \brief MPLS Label 0 Match Range Lower Value
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_LABEL_RANGE_UPPER_0
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_0(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 2)

/** 
 * \brief
 * Upper value for label 0 match range
 *
 * \details 
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_0 . MPLS_LABEL_RANGE_UPPER_0
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_0_MPLS_LABEL_RANGE_UPPER_0(x)  VTSS_ENCODE_BITFIELD(x,0,20)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_0_MPLS_LABEL_RANGE_UPPER_0     VTSS_ENCODE_BITMASK(0,20)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_0_MPLS_LABEL_RANGE_UPPER_0(x)  VTSS_EXTRACT_BITFIELD(x,0,20)


/** 
 * \brief MPLS Label 1 Match Range Lower Value
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_LABEL_RANGE_LOWER_1
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_1(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 3)

/** 
 * \brief
 * Lower value for label 1 match range

 *
 * \details 
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_1 . MPLS_LABEL_RANGE_LOWER_1
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_1_MPLS_LABEL_RANGE_LOWER_1(x)  VTSS_ENCODE_BITFIELD(x,0,20)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_1_MPLS_LABEL_RANGE_LOWER_1     VTSS_ENCODE_BITMASK(0,20)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_1_MPLS_LABEL_RANGE_LOWER_1(x)  VTSS_EXTRACT_BITFIELD(x,0,20)


/** 
 * \brief MPLS Label 1 Match Range Lower Value
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_LABEL_RANGE_UPPER_1
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_1(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 4)

/** 
 * \brief
 * Upper value for label 1 match range
 *
 * \details 
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_1 . MPLS_LABEL_RANGE_UPPER_1
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_1_MPLS_LABEL_RANGE_UPPER_1(x)  VTSS_ENCODE_BITFIELD(x,0,20)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_1_MPLS_LABEL_RANGE_UPPER_1     VTSS_ENCODE_BITMASK(0,20)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_1_MPLS_LABEL_RANGE_UPPER_1(x)  VTSS_EXTRACT_BITFIELD(x,0,20)


/** 
 * \brief MPLS Label 2 Match Range Lower Value
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_LABEL_RANGE_LOWER_2
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_2(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 5)

/** 
 * \brief
 * Lower value for label 2 match range

 *
 * \details 
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_2 . MPLS_LABEL_RANGE_LOWER_2
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_2_MPLS_LABEL_RANGE_LOWER_2(x)  VTSS_ENCODE_BITFIELD(x,0,20)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_2_MPLS_LABEL_RANGE_LOWER_2     VTSS_ENCODE_BITMASK(0,20)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_2_MPLS_LABEL_RANGE_LOWER_2(x)  VTSS_EXTRACT_BITFIELD(x,0,20)


/** 
 * \brief MPLS Label 2 Match Range Lower Value
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_LABEL_RANGE_UPPER_2
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_2(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 6)

/** 
 * \brief
 * Upper value for label 2 match range
 *
 * \details 
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_2 . MPLS_LABEL_RANGE_UPPER_2
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_2_MPLS_LABEL_RANGE_UPPER_2(x)  VTSS_ENCODE_BITFIELD(x,0,20)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_2_MPLS_LABEL_RANGE_UPPER_2     VTSS_ENCODE_BITMASK(0,20)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_2_MPLS_LABEL_RANGE_UPPER_2(x)  VTSS_EXTRACT_BITFIELD(x,0,20)


/** 
 * \brief MPLS Label 3 Match Range Lower Value
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_LABEL_RANGE_LOWER_3
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_3(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 7)

/** 
 * \brief
 * Lower value for label 3 match range

 *
 * \details 
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_3 . MPLS_LABEL_RANGE_LOWER_3
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_3_MPLS_LABEL_RANGE_LOWER_3(x)  VTSS_ENCODE_BITFIELD(x,0,20)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_3_MPLS_LABEL_RANGE_LOWER_3     VTSS_ENCODE_BITMASK(0,20)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_LOWER_3_MPLS_LABEL_RANGE_LOWER_3(x)  VTSS_EXTRACT_BITFIELD(x,0,20)


/** 
 * \brief MPLS Label 3 Match Range Lower Value
 *
 * \details
 * Register: \a ANA_OAM:MPLS_FLOW_CFG:MPLS_LABEL_RANGE_UPPER_3
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: MPLS_FLOW_CFG (??), 0-7
 */
#define VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_3(gi)  VTSS_IOREG_IX(0x160, gi, 16, 0, 8)

/** 
 * \brief
 * Upper value for label 3 match range
 *
 * \details 
 * Field: VTSS_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_3 . MPLS_LABEL_RANGE_UPPER_3
 */
#define  VTSS_F_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_3_MPLS_LABEL_RANGE_UPPER_3(x)  VTSS_ENCODE_BITFIELD(x,0,20)
#define  VTSS_M_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_3_MPLS_LABEL_RANGE_UPPER_3     VTSS_ENCODE_BITMASK(0,20)
#define  VTSS_X_ANA_OAM_MPLS_FLOW_CFG_MPLS_LABEL_RANGE_UPPER_3_MPLS_LABEL_RANGE_UPPER_3(x)  VTSS_EXTRACT_BITFIELD(x,0,20)

/**
 * Register Group: \a ANA_OAM:PTP_FLOW
 *
 * Not documented
 */


/** 
 * \brief PTP?OAM Flow Eanable
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_FLOW_ENA
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 0)

/** 
 * \brief
 * Indicates which next protocol groups that this flow is valid for.  For
 * each next protocol group, if the bit is 1, then this flow is valid for
 * that goup.  If it is 0, then it is not valid for the group.
 *
 * \details 
 * bit 0 = Mask bit for next protocol group A
 * bit 1 = Mask bit for next protocol group B
 *
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA . PTP_NXT_PROT_GRP_MASK
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA_PTP_NXT_PROT_GRP_MASK(x)  VTSS_ENCODE_BITFIELD(x,16,2)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA_PTP_NXT_PROT_GRP_MASK     VTSS_ENCODE_BITMASK(16,2)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA_PTP_NXT_PROT_GRP_MASK(x)  VTSS_EXTRACT_BITFIELD(x,16,2)

/** 
 * \details 
 * bit 0 = Flow valid for channel 0
 * bit 1 = Flow valid for channel 1

 *
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA . PTP_CHANNEL_MASK
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA_PTP_CHANNEL_MASK(x)  VTSS_ENCODE_BITFIELD(x,4,2)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA_PTP_CHANNEL_MASK     VTSS_ENCODE_BITMASK(4,2)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA_PTP_CHANNEL_MASK(x)  VTSS_EXTRACT_BITFIELD(x,4,2)

/** 
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA . PTP_FLOW_ENA
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_FLOW_ENA_PTP_FLOW_ENA  VTSS_BIT(0)


/** 
 * \brief Upper Half of PTP/OAM Flow Match Field
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_FLOW_MATCH_UPPER
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_FLOW_MATCH_UPPER(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 1)


/** 
 * \brief Lower Half of PTP/OAM Flow Match Field
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_FLOW_MATCH_LOWER
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_FLOW_MATCH_LOWER(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 2)


/** 
 * \brief Upper Half of PTP/OAM Flow Match Mask
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_FLOW_MASK_UPPER
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_FLOW_MASK_UPPER(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 3)


/** 
 * \brief Lower Half of PTP/OAM Flow Match Mask
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_FLOW_MASK_LOWER
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_FLOW_MASK_LOWER(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 4)


/** 
 * \brief PTP/OAM Range Match Register
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_DOMAIN_RANGE
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 5)

/** 
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE . PTP_DOMAIN_RANGE_OFFSET
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_OFFSET(x)  VTSS_ENCODE_BITFIELD(x,24,5)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_OFFSET     VTSS_ENCODE_BITMASK(24,5)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_OFFSET(x)  VTSS_EXTRACT_BITFIELD(x,24,5)

/** 
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE . PTP_DOMAIN_RANGE_UPPER
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_UPPER(x)  VTSS_ENCODE_BITFIELD(x,16,8)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_UPPER     VTSS_ENCODE_BITMASK(16,8)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_UPPER(x)  VTSS_EXTRACT_BITFIELD(x,16,8)

/** 
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE . PTP_DOMAIN_RANGE_LOWER
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_LOWER(x)  VTSS_ENCODE_BITFIELD(x,8,8)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_LOWER     VTSS_ENCODE_BITMASK(8,8)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_LOWER(x)  VTSS_EXTRACT_BITFIELD(x,8,8)

/** 
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE . PTP_DOMAIN_RANGE_ENA
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_DOMAIN_RANGE_PTP_DOMAIN_RANGE_ENA  VTSS_BIT(0)


/** 
 * \brief PTP Action Control Register
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_ACTION
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 6)

/** 
 * \details 
 * 1 = Tell the Rewriter to update the value of the Modified Frame Status
 * bit
 * 0 = Do not update the bit
 *
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION . PTP_MOD_FRAME_STAT_UPDATE
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_MOD_FRAME_STAT_UPDATE  VTSS_BIT(28)

/** 
 * \brief
 * Indicates the position relative to the start of the PTP frame in bytes
 * where the Modified_Frame_Status bit resides
 *
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION . PTP_MOD_FRAME_BYTE_OFFSET
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_MOD_FRAME_BYTE_OFFSET(x)  VTSS_ENCODE_BITFIELD(x,24,3)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_MOD_FRAME_BYTE_OFFSET     VTSS_ENCODE_BITMASK(24,3)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_MOD_FRAME_BYTE_OFFSET(x)  VTSS_EXTRACT_BITFIELD(x,24,3)

/** 
 * \details 
 * 1 = Signal the Timestamp block to subtract the asymmetry delay
 * 0 = Do not signal the Timestamp block to subtract the asymmetry delay
 *
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION . PTP_SUB_DELAY_ASYM_ENA
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_SUB_DELAY_ASYM_ENA  VTSS_BIT(21)

/** 
 * \details 
 * 1 = Signal the Timestamp block to add the asymmetry delay
 * 0 = Do not signal the Timestamp block to add the asymmetry delay
 *
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION . PTP_ADD_DELAY_ASYM_ENA
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_ADD_DELAY_ASYM_ENA  VTSS_BIT(20)

/** 
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION . PTP_TIME_STRG_FIELD_OFFSET
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_TIME_STRG_FIELD_OFFSET(x)  VTSS_ENCODE_BITFIELD(x,10,6)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_TIME_STRG_FIELD_OFFSET     VTSS_ENCODE_BITMASK(10,6)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_TIME_STRG_FIELD_OFFSET(x)  VTSS_EXTRACT_BITFIELD(x,10,6)

/** 
 * \brief
 * Points to the location of the correction field for updating the
 * timestamp.  Location is relative to the first byte of the PTP/OAM
 * header.
 * 
 * Note: If this flow is being used to match OAM frames, set this register
 * to 4
 *
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION . PTP_CORR_FIELD_OFFSET
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_CORR_FIELD_OFFSET(x)  VTSS_ENCODE_BITFIELD(x,5,5)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_CORR_FIELD_OFFSET     VTSS_ENCODE_BITMASK(5,5)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_CORR_FIELD_OFFSET(x)  VTSS_EXTRACT_BITFIELD(x,5,5)

/** 
 * \details 
 * 1 = Save the local time to the Timestamp FIFO
 * 0 = Do not save the time to the Timestamp FIFO
 *
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION . PTP_SAVE_LOCAL_TIME
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_SAVE_LOCAL_TIME  VTSS_BIT(4)

/** 
 * \details 
 * 0 = NOP
 * 1 = SUB
 * 2 = SUB_P2P
 * 3 = ADD
 * 4 = SUB_ADD
 * 5 = WRITE_1588
 * 6 = WRITE_P2P  (deprecated)
 * 7 = WRITE_NS
 * 8 = WRITE_NS_P2P
 *
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION . PTP_COMMAND
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_COMMAND(x)  VTSS_ENCODE_BITFIELD(x,0,4)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_COMMAND     VTSS_ENCODE_BITMASK(0,4)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ACTION_PTP_COMMAND(x)  VTSS_EXTRACT_BITFIELD(x,0,4)


/** 
 * \brief PTP Action Control Register 2
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_ACTION_2
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION_2(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 7)

/** 
 * \brief
 * Location of the new correction field relative to the PTP header start. 
 * Only even values are allowed.

 *
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION_2 . PTP_NEW_CF_LOC
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_NEW_CF_LOC(x)  VTSS_ENCODE_BITFIELD(x,16,8)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_NEW_CF_LOC     VTSS_ENCODE_BITMASK(16,8)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_NEW_CF_LOC(x)  VTSS_EXTRACT_BITFIELD(x,16,8)

/** 
 * \brief
 * Points to where in the frame relative to the SFD that the timestamp
 * should be updated
 *
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION_2 . PTP_REWRITE_OFFSET
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_REWRITE_OFFSET(x)  VTSS_ENCODE_BITFIELD(x,8,8)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_REWRITE_OFFSET     VTSS_ENCODE_BITMASK(8,8)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_REWRITE_OFFSET(x)  VTSS_EXTRACT_BITFIELD(x,8,8)

/** 
 * \brief
 * Number of bytes in the PTP or OAM frame that must be modified by the
 * Rewriter for the timestamp
 *
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ACTION_2 . PTP_REWRITE_BYTES
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_REWRITE_BYTES(x)  VTSS_ENCODE_BITFIELD(x,0,4)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_REWRITE_BYTES     VTSS_ENCODE_BITMASK(0,4)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ACTION_2_PTP_REWRITE_BYTES(x)  VTSS_EXTRACT_BITFIELD(x,0,4)


/** 
 * \brief Zero Field Control Register
 *
 * \details
 * Register: \a ANA_OAM:PTP_FLOW:PTP_ZERO_FIELD_CTL
 *
 * @param target A \a ::vtss_target_ANA_OAM_e target
 * @param gi Register: PTP_FLOW (??), 0-5
 */
#define VTSS_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL(gi)  VTSS_IOREG_IX(0x1e0, gi, 16, 0, 8)

/** 
 * \brief
 * Points to a location in the PTP/OAM frame relative to the start of the
 * PTP header that will be zeroed if this function is enabled
 *
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL . PTP_ZERO_FIELD_OFFSET
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL_PTP_ZERO_FIELD_OFFSET(x)  VTSS_ENCODE_BITFIELD(x,8,6)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL_PTP_ZERO_FIELD_OFFSET     VTSS_ENCODE_BITMASK(8,6)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL_PTP_ZERO_FIELD_OFFSET(x)  VTSS_EXTRACT_BITFIELD(x,8,6)

/** 
 * \brief
 * The number of bytes to be zeroed.  If this field is 0, then this
 * function is not enabled.
 *
 * \details 
 * Field: VTSS_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL . PTP_ZERO_FIELD_BYTE_CNT
 */
#define  VTSS_F_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL_PTP_ZERO_FIELD_BYTE_CNT(x)  VTSS_ENCODE_BITFIELD(x,0,4)
#define  VTSS_M_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL_PTP_ZERO_FIELD_BYTE_CNT     VTSS_ENCODE_BITMASK(0,4)
#define  VTSS_X_ANA_OAM_PTP_FLOW_PTP_ZERO_FIELD_CTL_PTP_ZERO_FIELD_BYTE_CNT(x)  VTSS_EXTRACT_BITFIELD(x,0,4)


#endif /* _VTSS_PHY_TS_REGS_ANA_OAM_H_ */
