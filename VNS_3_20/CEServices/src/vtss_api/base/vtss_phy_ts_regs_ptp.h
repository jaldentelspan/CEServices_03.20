#ifndef _VTSS_PHY_TS_REGS_PTP_H_
#define _VTSS_PHY_TS_REGS_PTP_H_

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
 * Target: \a PTP
 *
 * \see vtss_target_PTP_e
 *
 * 1588 IP Block Configuration & Status Registers
 *
 ***********************************************************************/

/**
 * Register Group: \a PTP:IP_1588_TOP_CFG_STAT
 *
 * 1588 IP Control & Status Registers
 */


/** 
 * \brief Interface Control Register
 *
 * \details
 * Register: \a PTP:IP_1588_TOP_CFG_STAT:INTERFACE_CTL
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL  VTSS_IOREG(0x0)

/** 
 * \brief
 * Clock enable for the 1588 IP block. This bit is an output of the IP and
 * can be used externally to gate the clocks to the block off when the
 * block is disabled. This bit is not used inside the IP block.
 *
 * \details 
 * 0 = Clocks Disabled
 * 1 = Clocks Enabled
 *
 * Field: VTSS_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL . CLK_ENA
 */
#define  VTSS_F_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL_CLK_ENA  VTSS_BIT(6)

/** 
 * \brief
 * When 1, the 1588 IP block is bypassed. This is the default state.
 * Changing this bit to 0 will allow 1588 processed data to flow out of the
 * block. This bit is internally registered so that it only takes effect
 * during an IDLE period in the data stream. This allows for a more
 * seamless transition from bypass to data passing modes.
 *
 * \details 
 * 0 = Data mode
 * 1 = Bypass mode
 *
 * Field: VTSS_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL . BYPASS
 */
#define  VTSS_F_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL_BYPASS  VTSS_BIT(2)

/** 
 * \brief
 * Defines the operating mode that the attached PCS block operates in
 *
 * \details 
 * 0 = XGMII-64
 * 1 = reserved
 * 2 = GMII
 * 3 = MII

 *
 * Field: VTSS_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL . MII_PROTOCOL
 */
#define  VTSS_F_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL_MII_PROTOCOL(x)  VTSS_ENCODE_BITFIELD(x,0,2)
#define  VTSS_M_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL_MII_PROTOCOL     VTSS_ENCODE_BITMASK(0,2)
#define  VTSS_X_PTP_IP_1588_TOP_CFG_STAT_INTERFACE_CTL_MII_PROTOCOL(x)  VTSS_EXTRACT_BITFIELD(x,0,2)


/** 
 * \brief Analyzer Mode Register
 *
 * \details
 * Register: \a PTP:IP_1588_TOP_CFG_STAT:ANALYZER_MODE
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE  VTSS_IOREG(0x1)

/** 
 * \brief
 * Defines how flow matching is performed in each encapsulation engine.  
 *
 * \details 
 * For each engine
 * 0 = Match any flow
 * 1 = Strict matching
 *
 * Field: VTSS_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE . ENCAP_FLOW_MODE
 */
#define  VTSS_F_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_ENCAP_FLOW_MODE(x)  VTSS_ENCODE_BITFIELD(x,16,3)
#define  VTSS_M_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_ENCAP_FLOW_MODE     VTSS_ENCODE_BITMASK(16,3)
#define  VTSS_X_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_ENCAP_FLOW_MODE(x)  VTSS_EXTRACT_BITFIELD(x,16,3)

/** 
 * \brief
 * Enables for the egress encapsulation engines.  Enable bit 0 & 1 are for
 * the PTP engines and bit 2 is for the OAM engine.
 *
 * \details 
 * For each engine
 * 0 = Disabled
 * 1 = Enabled
 *
 * Field: VTSS_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE . EGR_ENCAP_ENGINE_ENA
 */
#define  VTSS_F_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_EGR_ENCAP_ENGINE_ENA(x)  VTSS_ENCODE_BITFIELD(x,4,3)
#define  VTSS_M_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_EGR_ENCAP_ENGINE_ENA     VTSS_ENCODE_BITMASK(4,3)
#define  VTSS_X_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_EGR_ENCAP_ENGINE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,4,3)

/** 
 * \brief
 * Enables for the ingress encapsulation engines.  Enable bit 0 & 1 are for
 * the PTP engines and bit 2 is for the OAM engine.
 *
 * \details 
 * For each engine
 * 0 = Disabled
 * 1 = Enabled
 *
 * Field: VTSS_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE . INGR_ENCAP_ENGINE_ENA
 */
#define  VTSS_F_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_INGR_ENCAP_ENGINE_ENA(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_INGR_ENCAP_ENGINE_ENA     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_PTP_IP_1588_TOP_CFG_STAT_ANALYZER_MODE_INGR_ENCAP_ENGINE_ENA(x)  VTSS_EXTRACT_BITFIELD(x,0,3)


/** 
 * \brief Spare scratchpad register
 *
 * \details
 * Register: \a PTP:IP_1588_TOP_CFG_STAT:SPARE_REGISTER
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_TOP_CFG_STAT_SPARE_REGISTER  VTSS_IOREG(0x2)

/**
 * Register Group: \a PTP:IP_1588_LTC
 *
 * 1588 IP Local Time Counter
 */


/** 
 * \brief LTC control
 *
 * \details
 * LTC control
 *
 * Register: \a PTP:IP_1588_LTC:LTC_CTRL
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_CTRL        VTSS_IOREG(0x10)

/** 
 * \brief
 * This field is used to select the clock source for the LTC block. The
 * actual clock mux is external to the IP block, this field merely provides
 * the select lines to the clock mux. These 3 select lines are outputs of
 * the IP block and are not used internally. The single clock signal is
 * then fed to the LTC input pin. The 3 bits allows for one of up to 8
 * possible clock sources to be selected.
 *
 * \details 
 * 0 = External Pin is source of clock
 * 1 = Client Rx Clock (XAUI lane 0 recovered clock)
 * 2 = Client Tx Clock
 * 3 = Line Rx Clock
 * 4 = Line Tx Clock
 * 5 = INVALID
 * 6 = INVALID
 * 7 = INVALID

 *
 * Field: VTSS_PTP_IP_1588_LTC_LTC_CTRL . LTC_CLK_SEL
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_CTRL_LTC_CLK_SEL(x)  VTSS_ENCODE_BITFIELD(x,12,3)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_CTRL_LTC_CLK_SEL     VTSS_ENCODE_BITMASK(12,3)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_CTRL_LTC_CLK_SEL(x)  VTSS_EXTRACT_BITFIELD(x,12,3)

/** 
 * \brief
 * Selects one bit of the LTC internal nanosecond counter (0-29) to drive
 * to the PPS pin when in the alternate mode (LTC_ALT_MODE register bit
 * set).
 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_CTRL . LTC_ALT_MODE_PPS_BIT
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_CTRL_LTC_ALT_MODE_PPS_BIT(x)  VTSS_ENCODE_BITFIELD(x,6,5)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_CTRL_LTC_ALT_MODE_PPS_BIT     VTSS_ENCODE_BITMASK(6,5)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_CTRL_LTC_ALT_MODE_PPS_BIT(x)  VTSS_EXTRACT_BITFIELD(x,6,5)

/** 
 * \brief
 * Enable an alternate mode that modifies the default PPS output. See the
 * LTC_ALT_MODE_PPS_BIT field description.
 *
 * \details 
 * 0 = normal mode
 * 1 = alternate mode
 *
 * Field: VTSS_PTP_IP_1588_LTC_LTC_CTRL . LTC_ALT_MODE
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_CTRL_LTC_ALT_MODE  VTSS_BIT(5)

/** 
 * \brief
 * When written to a '1' causes the Local Time Counter to update the
 * automatic adjustment values from the LTC_AUTO_ADJUST register.  The
 * current automatic adjustment is reset to start with the new values.
 * Automatically cleared.
 *
 * \details 
 * 0 = No change to any previous updates (write), or update has completed
 * (read)
 * 1 = Use new values from LTC_AUTO_ADJUST register.
 *
 * Field: VTSS_PTP_IP_1588_LTC_LTC_CTRL . LTC_AUTO_ADJUST_UPDATE
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_CTRL_LTC_AUTO_ADJUST_UPDATE  VTSS_BIT(4)

/** 
 * \brief
 * When written to a '1' causes a request for 1ns to be added or subtracted
 * (depending upon the LTC_ADD_SUB_1NS field) from the Local time.
 * Automatically cleared.
 *
 * \details 
 * 0 = No Add/Subtract from local time (write), Bit has auto cleared (read)
 * 1 = Add/Subtract 1ns from the local time.
 *
 * Field: VTSS_PTP_IP_1588_LTC_LTC_CTRL . LTC_ADD_SUB_1NS_REQ
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_CTRL_LTC_ADD_SUB_1NS_REQ  VTSS_BIT(3)

/** 
 * \brief
 * This bit selects whether a write to the LTC_ADD_SUB_1NS_REQ register
 * causes an add or subtract.
 *
 * \details 
 * 0 = Subtract 1 ns
 * 1 = Add 1ns to the local time.
 *
 * Field: VTSS_PTP_IP_1588_LTC_LTC_CTRL . LTC_ADD_SUB_1NS
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_CTRL_LTC_ADD_SUB_1NS  VTSS_BIT(2)

/** 
 * \brief
 * LTC save enable. Enables the chip save pin to save the LTC_SAVE
 * seconds/nanoseconds registers.
 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_CTRL . LTC_SAVE_ENA
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_CTRL_LTC_SAVE_ENA  VTSS_BIT(1)

/** 
 * \brief
 * LTC load enable. Enables the chip load pin to load the LTC_LOAD
 * seconds/nanoseconds registers.
 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_CTRL . LTC_LOAD_ENA
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_CTRL_LTC_LOAD_ENA  VTSS_BIT(0)


/** 
 * \brief LTC load seconds (high)
 *
 * \details
 * LTC load seconds (high)
 *
 * Register: \a PTP:IP_1588_LTC:LTC_LOAD_SEC_H
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_LOAD_SEC_H  VTSS_IOREG(0x11)

/** 
 * \brief
 * LTC load seconds (high)
 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_LOAD_SEC_H . LTC_LOAD_SEC_H
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_LOAD_SEC_H_LTC_LOAD_SEC_H(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_LOAD_SEC_H_LTC_LOAD_SEC_H     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_LOAD_SEC_H_LTC_LOAD_SEC_H(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief LTC load seconds (low)
 *
 * \details
 * LTC load seconds (low)
 *
 * Register: \a PTP:IP_1588_LTC:LTC_LOAD_SEC_L
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_LOAD_SEC_L  VTSS_IOREG(0x12)


/** 
 * \brief LTC load nanoseconds
 *
 * \details
 * LTC load nanoseconds
 *
 * Register: \a PTP:IP_1588_LTC:LTC_LOAD_NS
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_LOAD_NS     VTSS_IOREG(0x13)


/** 
 * \brief LTC saved seconds (high)
 *
 * \details
 * LTC saved seconds (high)
 *
 * Register: \a PTP:IP_1588_LTC:LTC_SAVED_SEC_H
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_SAVED_SEC_H  VTSS_IOREG(0x14)

/** 
 * \brief
 * LTC saved seconds (high)
 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_SAVED_SEC_H . LTC_SAVED_SEC_H
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_SAVED_SEC_H_LTC_SAVED_SEC_H(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_SAVED_SEC_H_LTC_SAVED_SEC_H     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_SAVED_SEC_H_LTC_SAVED_SEC_H(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief LTC saved seconds (low)
 *
 * \details
 * LTC saved seconds (low)
 *
 * Register: \a PTP:IP_1588_LTC:LTC_SAVED_SEC_L
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_SAVED_SEC_L  VTSS_IOREG(0x15)


/** 
 * \brief LTC saved nanoseconds
 *
 * \details
 * LTC saved nanoseconds
 *
 * Register: \a PTP:IP_1588_LTC:LTC_SAVED_NS
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_SAVED_NS    VTSS_IOREG(0x16)


/** 
 * \brief LTC sequence configuration
 *
 * \details
 * LTC sequence configuration
 *
 * Register: \a PTP:IP_1588_LTC:LTC_SEQUENCE
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_SEQUENCE    VTSS_IOREG(0x17)

/** 
 * \brief
 * LTC sequence of increments (nanoseconds)
 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_SEQUENCE . LTC_SEQUENCE_A
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_SEQUENCE_LTC_SEQUENCE_A(x)  VTSS_ENCODE_BITFIELD(x,0,4)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_SEQUENCE_LTC_SEQUENCE_A     VTSS_ENCODE_BITMASK(0,4)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_SEQUENCE_LTC_SEQUENCE_A(x)  VTSS_EXTRACT_BITFIELD(x,0,4)


/** 
 * \brief LTC sequence configuration
 *
 * \details
 * LTC sequence configuration
 *
 * Register: \a PTP:IP_1588_LTC:LTC_SEQ
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_SEQ         VTSS_IOREG(0x18)

/** 
 * \brief
 * LTC sequence correction sign
 *
 * \details 
 * 0 = subtract 1ns adjustment
 * 1 = add 1ns adjustment
 *
 * Field: VTSS_PTP_IP_1588_LTC_LTC_SEQ . LTC_SEQ_ADD_SUB
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_SEQ_LTC_SEQ_ADD_SUB  VTSS_BIT(19)

/** 
 * \brief
 * LTC sequence correction (nanoseconds * 1 million )
 * 
 * Example for 6.4 ns period (156.25MHz):
 * 
 * LTC_SEQUENCE.LTC_SEQUENCE_A = 6 (6 ns)
 * LTC_SEQ.LTC_SEQ_ADD_SUB = 1 (add 1ns)
 * LTC_SEQ.LTC_SEQ_E = 400000 (0.4ns * 1,000,000)

 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_SEQ . LTC_SEQ_E
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_SEQ_LTC_SEQ_E(x)  VTSS_ENCODE_BITFIELD(x,0,19)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_SEQ_LTC_SEQ_E     VTSS_ENCODE_BITMASK(0,19)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_SEQ_LTC_SEQ_E(x)  VTSS_EXTRACT_BITFIELD(x,0,19)


/** 
 * \brief LTC auto adjustment
 *
 * \details
 * LTC auto adjustment
 *
 * Register: \a PTP:IP_1588_LTC:LTC_AUTO_ADJUST
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_AUTO_ADJUST  VTSS_IOREG(0x1a)

/** 
 * \brief
 * LTC auto adjustment add/subtract 1ns
 *
 * \details 
 * 0,3 = No adjustment
 * 1 = Adjust by adding 1ns upon rollover.
 * 2 = Adjust by subtracting 1ns upon rollover.
 *
 * Field: VTSS_PTP_IP_1588_LTC_LTC_AUTO_ADJUST . LTC_AUTO_ADD_SUB_1NS
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_AUTO_ADJUST_LTC_AUTO_ADD_SUB_1NS(x)  VTSS_ENCODE_BITFIELD(x,30,2)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_AUTO_ADJUST_LTC_AUTO_ADD_SUB_1NS     VTSS_ENCODE_BITMASK(30,2)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_AUTO_ADJUST_LTC_AUTO_ADD_SUB_1NS(x)  VTSS_EXTRACT_BITFIELD(x,30,2)

/** 
 * \brief
 * LTC auto adjustment rollover (nanoseconds)
 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_AUTO_ADJUST . LTC_AUTO_ADJUST_NS
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_AUTO_ADJUST_LTC_AUTO_ADJUST_NS(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_AUTO_ADJUST_LTC_AUTO_ADJUST_NS     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_AUTO_ADJUST_LTC_AUTO_ADJUST_NS(x)  VTSS_EXTRACT_BITFIELD(x,0,30)


/** 
 * \brief LTC 1 pulse per second width adjustment
 *
 * \details
 * LTC 1 pulse per second width adjustment
 *
 * Register: \a PTP:IP_1588_LTC:LTC_1PPS_WIDTH_ADJ
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_IP_1588_LTC_LTC_1PPS_WIDTH_ADJ  VTSS_IOREG(0x1b)

/** 
 * \brief
 * The 1 pulse per second is high for the programmed number of nanoseconds
 * within +/- the sequence incremement value.
 *
 * \details 
 * Field: VTSS_PTP_IP_1588_LTC_LTC_1PPS_WIDTH_ADJ . LTC_1PPS_WIDTH_ADJ
 */
#define  VTSS_F_PTP_IP_1588_LTC_LTC_1PPS_WIDTH_ADJ_LTC_1PPS_WIDTH_ADJ(x)  VTSS_ENCODE_BITFIELD(x,0,30)
#define  VTSS_M_PTP_IP_1588_LTC_LTC_1PPS_WIDTH_ADJ_LTC_1PPS_WIDTH_ADJ     VTSS_ENCODE_BITMASK(0,30)
#define  VTSS_X_PTP_IP_1588_LTC_LTC_1PPS_WIDTH_ADJ_LTC_1PPS_WIDTH_ADJ(x)  VTSS_EXTRACT_BITFIELD(x,0,30)

/**
 * Register Group: \a PTP:TS_FIFO_SI
 *
 * Timestamp FIFO Serial Interface registers
 */


/** 
 * \brief Timestamp FIFO Serial Interface configuration register
 *
 * \details
 * Polarity and cycle counts are configurable from port 0 only.
 *
 * Register: \a PTP:TS_FIFO_SI:TS_FIFO_SI_CFG
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG   VTSS_IOREG(0x20)

/** 
 * \brief
 * Timestamp serial interface clock phase control.

 *
 * \details 
 * 0=SPI_CLK falling edge changes output data 
 * 1=SPI_CLK rising edge changes output data 

 *
 * Field: VTSS_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG . SI_CLK_PHA
 */
#define  VTSS_F_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_CLK_PHA  VTSS_BIT(25)

/** 
 * \brief
 * Timestamp FIFO serial interface clock polarity control.

 *
 * \details 
 * 0=SPI_CLK starts and ends (idles) low
 * 1=SPI_CLK starts and ends (idles) high

 *
 * Field: VTSS_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG . SI_CLK_POL
 */
#define  VTSS_F_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_CLK_POL  VTSS_BIT(24)

/** 
 * \brief
 * Number of CSR clock periods SPI_CS negates between writes (deselected). 
 * The CSR clock frequency is one-half the XREFCLK frequency.
 *
 * \details 
 * Field: VTSS_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG . SI_EN_DES_CYCS
 */
#define  VTSS_F_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_EN_DES_CYCS(x)  VTSS_ENCODE_BITFIELD(x,20,4)
#define  VTSS_M_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_EN_DES_CYCS     VTSS_ENCODE_BITMASK(20,4)
#define  VTSS_X_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_EN_DES_CYCS(x)  VTSS_EXTRACT_BITFIELD(x,20,4)

/** 
 * \brief
 * Number of CSR clock periods that the SPI_CLK is high.  Registers
 * SI_CLK_HI_CYCS and SI_CLK_LO_CYCS determine the frequency of the SPI_CLK
 * pin when the timestamp FIFO serial interface is enabled.  The CSR clock
 * frequency is one-half the XREFCLK frequency.  Zero is an invalid
 * setting.

 *
 * \details 
 * Field: VTSS_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG . SI_CLK_HI_CYCS
 */
#define  VTSS_F_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_CLK_HI_CYCS(x)  VTSS_ENCODE_BITFIELD(x,6,5)
#define  VTSS_M_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_CLK_HI_CYCS     VTSS_ENCODE_BITMASK(6,5)
#define  VTSS_X_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_CLK_HI_CYCS(x)  VTSS_EXTRACT_BITFIELD(x,6,5)

/** 
 * \brief
 * Number of CSR clock periods that the SPI_CLK is low.  Registers
 * SI_CLK_HI_CYCS and SI_CLK_LO_CYCS determine the frequency of the SPI_CLK
 * pin when the timestamp FIFO serial interface is enabled.  The CSR clock
 * frequency is one-half the XREFCLK frequency.  Zero is an invalid
 * setting.

 *
 * \details 
 * Field: VTSS_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG . SI_CLK_LO_CYCS
 */
#define  VTSS_F_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_CLK_LO_CYCS(x)  VTSS_ENCODE_BITFIELD(x,1,5)
#define  VTSS_M_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_CLK_LO_CYCS     VTSS_ENCODE_BITMASK(1,5)
#define  VTSS_X_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_SI_CLK_LO_CYCS(x)  VTSS_EXTRACT_BITFIELD(x,1,5)

/** 
 * \brief
 * When 1, the Timestamp FIFO Serial Interface block is enabled
 *
 * \details 
 * 0=Disabled
 * 1=Enabled
 *
 * Field: VTSS_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG . TS_FIFO_SI_ENA
 */
#define  VTSS_F_PTP_TS_FIFO_SI_TS_FIFO_SI_CFG_TS_FIFO_SI_ENA  VTSS_BIT(0)


/** 
 * \brief Transmitted Timestamp count
 *
 * \details
 * Counter for the number of timestamps transmitted to the interface.
 *
 * Register: \a PTP:TS_FIFO_SI:TS_FIFO_SI_TX_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_TS_FIFO_SI_TS_FIFO_SI_TX_CNT  VTSS_IOREG(0x21)

/**
 * Register Group: \a PTP:INGR_PREDICTOR
 *
 * Ingress (Rx) registers

 */


/** 
 * \brief Ingress configuration register
 *
 * \details
 * Register: \a PTP:INGR_PREDICTOR:IG_CFG
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_PREDICTOR_IG_CFG       VTSS_IOREG(0x22)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_INGR_PREDICTOR_IG_CFG . WAF
 */
#define  VTSS_F_PTP_INGR_PREDICTOR_IG_CFG_WAF(x)  VTSS_ENCODE_BITFIELD(x,16,6)
#define  VTSS_M_PTP_INGR_PREDICTOR_IG_CFG_WAF     VTSS_ENCODE_BITMASK(16,6)
#define  VTSS_X_PTP_INGR_PREDICTOR_IG_CFG_WAF(x)  VTSS_EXTRACT_BITFIELD(x,16,6)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_INGR_PREDICTOR_IG_CFG . PAF
 */
#define  VTSS_F_PTP_INGR_PREDICTOR_IG_CFG_PAF(x)  VTSS_ENCODE_BITFIELD(x,8,6)
#define  VTSS_M_PTP_INGR_PREDICTOR_IG_CFG_PAF     VTSS_ENCODE_BITMASK(8,6)
#define  VTSS_X_PTP_INGR_PREDICTOR_IG_CFG_PAF(x)  VTSS_EXTRACT_BITFIELD(x,8,6)

/** 
 * \brief
 * When 1, the Ingress prediction block is enabled
 *
 * \details 
 * 0=Disabled
 * 1=Enabled
 *
 * Field: VTSS_PTP_INGR_PREDICTOR_IG_CFG . IG_ENABLE
 */
#define  VTSS_F_PTP_INGR_PREDICTOR_IG_CFG_IG_ENABLE  VTSS_BIT(0)


/** 
 * \brief Period of PMA clock in fractional nanoseconds
 *
 * \details
 * Register: \a PTP:INGR_PREDICTOR:IG_PMA
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_PREDICTOR_IG_PMA       VTSS_IOREG(0x23)

/** 
 * \brief
 * Duration of 8192 XFI bit times in nS.  This bit group is used only in
 * WAN mode operation, not LAN mode.

 *
 * \details 
 * The duration of one bit time in WAN mode is 1/9.95328 nS.  This register
 * should be set to (1/9.95328) * 8192 = 823 nS = 0x337.

 *
 * Field: VTSS_PTP_INGR_PREDICTOR_IG_PMA . TPMA
 */
#define  VTSS_F_PTP_INGR_PREDICTOR_IG_PMA_TPMA(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_INGR_PREDICTOR_IG_PMA_TPMA     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_INGR_PREDICTOR_IG_PMA_TPMA(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief XFI delays in nanoseconds
 *
 * \details
 * Register: \a PTP:INGR_PREDICTOR:IG_XFI
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_PREDICTOR_IG_XFI       VTSS_IOREG(0x24)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_INGR_PREDICTOR_IG_XFI . XFI_MSB
 */
#define  VTSS_F_PTP_INGR_PREDICTOR_IG_XFI_XFI_MSB(x)  VTSS_ENCODE_BITFIELD(x,16,16)
#define  VTSS_M_PTP_INGR_PREDICTOR_IG_XFI_XFI_MSB     VTSS_ENCODE_BITMASK(16,16)
#define  VTSS_X_PTP_INGR_PREDICTOR_IG_XFI_XFI_MSB(x)  VTSS_EXTRACT_BITFIELD(x,16,16)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_INGR_PREDICTOR_IG_XFI . XFI_LSB
 */
#define  VTSS_F_PTP_INGR_PREDICTOR_IG_XFI_XFI_LSB(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_INGR_PREDICTOR_IG_XFI_XFI_LSB     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_INGR_PREDICTOR_IG_XFI_XFI_LSB(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief OTN configuration
 *
 * \details
 * Register: \a PTP:INGR_PREDICTOR:IG_OTN
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_PREDICTOR_IG_OTN       VTSS_IOREG(0x25)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_INGR_PREDICTOR_IG_OTN . GAP_PERIOD
 */
#define  VTSS_F_PTP_INGR_PREDICTOR_IG_OTN_GAP_PERIOD(x)  VTSS_ENCODE_BITFIELD(x,0,7)
#define  VTSS_M_PTP_INGR_PREDICTOR_IG_OTN_GAP_PERIOD     VTSS_ENCODE_BITMASK(0,7)
#define  VTSS_X_PTP_INGR_PREDICTOR_IG_OTN_GAP_PERIOD(x)  VTSS_EXTRACT_BITFIELD(x,0,7)

/**
 * Register Group: \a PTP:EGR_PREDICTOR
 *
 * Egress (Tx) registers
 */


/** 
 * \brief Egress configuration register
 *
 * \details
 * Register: \a PTP:EGR_PREDICTOR:EG_CFG
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_PREDICTOR_EG_CFG        VTSS_IOREG(0x26)

/** 
 * \brief
 * WIS Advanced (fixed) value.	Set to 0xC for both WAN and LAN modes of
 * operation.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_CFG . WAF
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_CFG_WAF(x)  VTSS_ENCODE_BITFIELD(x,16,6)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_CFG_WAF     VTSS_ENCODE_BITMASK(16,6)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_CFG_WAF(x)  VTSS_EXTRACT_BITFIELD(x,16,6)

/** 
 * \brief
 * PCS Advanced (fixed) value.	Set to 0xC for both WAN and LAN modes of
 * operation.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_CFG . PAF
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_CFG_PAF(x)  VTSS_ENCODE_BITFIELD(x,8,6)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_CFG_PAF     VTSS_ENCODE_BITMASK(8,6)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_CFG_PAF(x)  VTSS_EXTRACT_BITFIELD(x,8,6)

/** 
 * \brief
 * When 1, the Egress prediction block is enabled.
 *
 * \details 
 * 0=Disabled
 * 1=Enabled
 *
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_CFG . EG_ENABLE
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_CFG_EG_ENABLE  VTSS_BIT(0)


/** 
 * \brief Egress WIS frame characteristics in clocks
 *
 * \details
 * Register: \a PTP:EGR_PREDICTOR:EG_WIS_FRAME
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_PREDICTOR_EG_WIS_FRAME  VTSS_IOREG(0x27)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_WIS_FRAME . W_OH
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_WIS_FRAME_W_OH(x)  VTSS_ENCODE_BITFIELD(x,16,8)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_WIS_FRAME_W_OH     VTSS_ENCODE_BITMASK(16,8)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_WIS_FRAME_W_OH(x)  VTSS_EXTRACT_BITFIELD(x,16,8)

/** 
 * \brief
 * Size of the WIS frame in clocks. Typically 2160.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_WIS_FRAME . W_FSIZE
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_WIS_FRAME_W_FSIZE(x)  VTSS_ENCODE_BITFIELD(x,0,12)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_WIS_FRAME_W_FSIZE     VTSS_ENCODE_BITMASK(0,12)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_WIS_FRAME_W_FSIZE(x)  VTSS_EXTRACT_BITFIELD(x,0,12)


/** 
 * \brief Egress WIS delays in nanoseconds
 *
 * \details
 * Register: \a PTP:EGR_PREDICTOR:EG_WIS_DELAYS
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_PREDICTOR_EG_WIS_DELAYS  VTSS_IOREG(0x28)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_WIS_DELAYS . W_OH_NS
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_WIS_DELAYS_W_OH_NS(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_WIS_DELAYS_W_OH_NS     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_WIS_DELAYS_W_OH_NS(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief Egress PMA clock delay
 *
 * \details
 * Register: \a PTP:EGR_PREDICTOR:EG_PMA
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_PREDICTOR_EG_PMA        VTSS_IOREG(0x29)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_PMA . TPMA
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_PMA_TPMA(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_PMA_TPMA     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_PMA_TPMA(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief XFI delays in nanoseconds
 *
 * \details
 * Register: \a PTP:EGR_PREDICTOR:EG_XFI
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_PREDICTOR_EG_XFI        VTSS_IOREG(0x2a)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_XFI . XFI_MSB
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_XFI_XFI_MSB(x)  VTSS_ENCODE_BITFIELD(x,16,16)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_XFI_XFI_MSB     VTSS_ENCODE_BITMASK(16,16)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_XFI_XFI_MSB(x)  VTSS_EXTRACT_BITFIELD(x,16,16)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_XFI . XFI_LSB
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_XFI_XFI_LSB(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_XFI_XFI_LSB     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_XFI_XFI_LSB(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief OTN configuration
 *
 * \details
 * Register: \a PTP:EGR_PREDICTOR:EG_OTN
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_PREDICTOR_EG_OTN        VTSS_IOREG(0x2b)

/** 
 * \brief
 * Reserved for factory test use.  Do not modify this bit group.
 *
 * \details 
 * Field: VTSS_PTP_EGR_PREDICTOR_EG_OTN . GAP_PERIOD
 */
#define  VTSS_F_PTP_EGR_PREDICTOR_EG_OTN_GAP_PERIOD(x)  VTSS_ENCODE_BITFIELD(x,0,7)
#define  VTSS_M_PTP_EGR_PREDICTOR_EG_OTN_GAP_PERIOD     VTSS_ENCODE_BITMASK(0,7)
#define  VTSS_X_PTP_EGR_PREDICTOR_EG_OTN_GAP_PERIOD(x)  VTSS_EXTRACT_BITFIELD(x,0,7)

/**
 * Register Group: \a PTP:MISC_CFG
 *
 * Miscellaneous chip-specific configuration and control signals
 */


/** 
 * \brief Misc. Configuration and Control signals
 *
 * \details
 * Register: \a PTP:MISC_CFG:CFG
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_MISC_CFG_CFG                VTSS_IOREG(0x2c)

/** 
 * \brief
 * This bit controls an external mux that can, for debugging purposes,
 * select the SOF detection pulses for output instead of the 1
 * pulse-per-second (1 pps) signals. Either the Ingress or Egress SOF
 * detection pulses can be selected instead of the 1pps singnal for output.
 *
 * \details 
 * 0 = One PPS output
 * 1 = Ingress SOF Detect output
 * 2 = Egress SOF Detect output
 * 3 = INVALID
 *
 * Field: VTSS_PTP_MISC_CFG_CFG . SOF_OUT_SEL
 */
#define  VTSS_F_PTP_MISC_CFG_CFG_SOF_OUT_SEL(x)  VTSS_ENCODE_BITFIELD(x,0,2)
#define  VTSS_M_PTP_MISC_CFG_CFG_SOF_OUT_SEL     VTSS_ENCODE_BITMASK(0,2)
#define  VTSS_X_PTP_MISC_CFG_CFG_SOF_OUT_SEL(x)  VTSS_EXTRACT_BITFIELD(x,0,2)

/**
 * Register Group: \a PTP:INGR_IP_1588_CFG_STAT
 *
 * 1588 IP Control & Status Registers
 */


/** 
 * \brief IP 1588 Interrupt Status Register
 *
 * \details
 * Status sticky conditions for the 1588 IP
 *
 * Register: \a PTP:INGR_IP_1588_CFG_STAT:INGR_INT_STATUS
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS  VTSS_IOREG(0x2d)

/** 
 * \brief
 * Indicates that more than one engine has pruduced a match
 *
 * \details 
 * 0 = No error found
 * 1 = Duplicate match found
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS . INGR_ANALYZER_ERROR_STICKY
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS_INGR_ANALYZER_ERROR_STICKY  VTSS_BIT(6)

/** 
 * \brief
 * When set, indicates that a preamble that was too short to modify was
 * detected in a PTP frame. Write to 0 to clear. This occurs when the
 * Rewriter needs to shrink the preamble to append a timestamp but cannot
 * because the preamble is too short. A short preamble is any preamble that
 * is less than 8 characters long including the XGMII /S/ character and the
 * ending SFD of 0xD5. Other preamble values are not checked, only the
 * length.
 *
 * \details 
 * 0 = No error
 * 1 = Preamble too short error
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS . INGR_RW_PREAMBLE_ERR_STICKY
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS_INGR_RW_PREAMBLE_ERR_STICKY  VTSS_BIT(5)

/** 
 * \brief
 * When set, indicates that an FCS error was detected in a PTP frame. Write
 * to 0 to clear.
 *
 * \details 
 * 0 = No error
 * 1 = FCS error
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS . INGR_RW_FCS_ERR_STICKY
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS_INGR_RW_FCS_ERR_STICKY  VTSS_BIT(4)

/** 
 * \brief
 * When set, indicates that the level in the Timestamp FIFO has reached the
 * threshold TS_THRESH. The sticky bit should be reset by writing it to
 * zero.
 *
 * \details 
 * 0 = No overflow
 * 1 = Overflow
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS . INGR_TS_LEVEL_STICKY
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS_INGR_TS_LEVEL_STICKY  VTSS_BIT(3)

/** 
 * \brief
 * When set, indicates a timestamp was captured in the Timestamp FIFO. The
 * sticky bit should be reset by writing it to zero.
 *
 * \details 
 * 0 = No overflow
 * 1 = Overflow
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS . INGR_TS_LOADED_STICKY
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS_INGR_TS_LOADED_STICKY  VTSS_BIT(2)

/** 
 * \brief
 * When set, indicates an underflow in the Timestamp FIFO. The sticky bit
 * should be reset by writing it to zero.
 *
 * \details 
 * 0 = No overflow
 * 1 = Overflow
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS . INGR_TS_UNDERFLOW_STICKY
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS_INGR_TS_UNDERFLOW_STICKY  VTSS_BIT(1)

/** 
 * \brief
 * When set, indicates an overflow in the Timestamp FIFO. The sticky bit
 * should be reset by writing it to zero.
 *
 * \details 
 * 0 = No overflow
 * 1 = Overflow
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS . INGR_TS_OVERFLOW_STICKY
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_STATUS_INGR_TS_OVERFLOW_STICKY  VTSS_BIT(0)


/** 
 * \brief IP 1588 Interrupt Mask Register
 *
 * \details
 * Masks that enable and disable the interrupts
 *
 * Register: \a PTP:INGR_IP_1588_CFG_STAT:INGR_INT_MASK
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK  VTSS_IOREG(0x2e)

/** 
 * \brief
 * Mask bit for ANALYZER_ERROR_STICKY bit.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK . INGR_ANALYZER_ERROR_MASK
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK_INGR_ANALYZER_ERROR_MASK  VTSS_BIT(6)

/** 
 * \brief
 * Mask for the RW_PREAMBLE_ERR_STICKY bit.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK . INGR_RW_PREAMBLE_ERR_MASK
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK_INGR_RW_PREAMBLE_ERR_MASK  VTSS_BIT(5)

/** 
 * \brief
 * Mask for the RW_FCS_ERR_STICKY bit.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK . INGR_RW_FCS_ERR_MASK
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK_INGR_RW_FCS_ERR_MASK  VTSS_BIT(4)

/** 
 * \brief
 * Mask bit for TS_LEVEL_STICKY. When 1, the interrupt is enabled.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK . INGR_TS_LEVEL_MASK
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK_INGR_TS_LEVEL_MASK  VTSS_BIT(3)

/** 
 * \brief
 * Mask bit for TS_LOADED_STICKY. When 1, the interrupt is enabled.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK . INGR_TS_LOADED_MASK
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK_INGR_TS_LOADED_MASK  VTSS_BIT(2)

/** 
 * \brief
 * Mask bit for TS_UNDERFLOW_STICKY. When 1, the interrupt is enabled.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK . INGR_TS_UNDERFLOW_MASK
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK_INGR_TS_UNDERFLOW_MASK  VTSS_BIT(1)

/** 
 * \brief
 * Mask bit for TS_OVERFLOW_STICKY. When 1, the interrupt is enabled.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK . INGR_TS_OVERFLOW_MASK
 */
#define  VTSS_F_PTP_INGR_IP_1588_CFG_STAT_INGR_INT_MASK_INGR_TS_OVERFLOW_MASK  VTSS_BIT(0)


/** 
 * \brief Spare scratchpad register
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_CFG_STAT:INGR_SPARE_REGISTER
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_CFG_STAT_INGR_SPARE_REGISTER  VTSS_IOREG(0x2f)

/**
 * Register Group: \a PTP:INGR_IP_1588_TSP
 *
 * 1588 IP Timestamp Processor
 */


/** 
 * \brief TSP Control
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_TSP:INGR_TSP_CTRL
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSP_INGR_TSP_CTRL  VTSS_IOREG(0x35)

/** 
 * \brief
 * Selects a mode in which the fractional portion of a second (in units of
 * nanoseconds) is used for timestamping. Only the operation of the
 * WRITE_NS, WRITE_NS_P2P, and SUB_ADD PTP commands are affected by the
 * setting of this mode bit.
 *
 * \details 
 * 0 = Select the total (summed) nanoseconds for timestamping.
 * 1 = Select the fractional portion in nanoseconds for timestamping.
 *
 * Field: VTSS_PTP_INGR_IP_1588_TSP_INGR_TSP_CTRL . INGR_FRACT_NS_MODE
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSP_INGR_TSP_CTRL_INGR_FRACT_NS_MODE  VTSS_BIT(2)

/** 
 * \brief
 * Select external pin start of frame indicator.
 *
 * \details 
 * 0 = Select internal PCS as the source of SOF
 * 1 = Select external pin as the source of SOF.
 *
 * Field: VTSS_PTP_INGR_IP_1588_TSP_INGR_TSP_CTRL . INGR_SEL_EXT_SOF_IND
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSP_INGR_TSP_CTRL_INGR_SEL_EXT_SOF_IND  VTSS_BIT(1)

/** 
 * \brief
 * One-shot loads Local latency, Path delay, and DelayAsymmetry values into
 * the Timestamp Processor
 *
 * \details 
 * Field: VTSS_PTP_INGR_IP_1588_TSP_INGR_TSP_CTRL . INGR_LOAD_DELAYS
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSP_INGR_TSP_CTRL_INGR_LOAD_DELAYS  VTSS_BIT(0)


/** 
 * \brief TSP Status
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_TSP:INGR_TSP_STAT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSP_INGR_TSP_STAT  VTSS_IOREG(0x36)

/** 
 * \brief
 * Timestamp procesor marked a calculated correction field as too big.
 *
 * \details 
 * 0 = A calculated correction field that was too big did occur.
 * 1 = A calculated correction field that was too big did not occur.
 *
 * Field: VTSS_PTP_INGR_IP_1588_TSP_INGR_TSP_STAT . INGR_CF_TOO_BIG_STICKY
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSP_INGR_TSP_STAT_INGR_CF_TOO_BIG_STICKY  VTSS_BIT(0)


/** 
 * \brief Local latency
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_TSP:INGR_LOCAL_LATENCY
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSP_INGR_LOCAL_LATENCY  VTSS_IOREG(0x37)

/** 
 * \brief
 * Local latency (nanoseconds)
 *
 * \details 
 * 
 * The value programmed in this register is dependent upon the frequency of
 * the clock driving the Local Time Counter (LTC) and upon WAN/LAN mode of
 * operation.
 * 
 *   When in LAN mode and the LTC clock frequency is 250 MHz, set this
 * register to 106.
 *   When in LAN mode and the LTC clock frequency is 156.25 MHz, set this
 * register to 109.
 *   When in LAN mode and the LTC clock frequency is 125 MHz, set this
 * register to 112.
 * 
 *   When in WAN mode and the LTC clock frequency is 250 MHz, set this
 * register to 149.
 *   When in WAN mode and the LTC clock frequency is 156.25 MHz, set this
 * register to 152.
 *   When in WAN mode and the LTC clock frequency is 125 MHz, set this
 * register to 155.

 *
 * Field: VTSS_PTP_INGR_IP_1588_TSP_INGR_LOCAL_LATENCY . INGR_LOCAL_LATENCY
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSP_INGR_LOCAL_LATENCY_INGR_LOCAL_LATENCY(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_INGR_IP_1588_TSP_INGR_LOCAL_LATENCY_INGR_LOCAL_LATENCY     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_INGR_IP_1588_TSP_INGR_LOCAL_LATENCY_INGR_LOCAL_LATENCY(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief Path delay
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_TSP:INGR_PATH_DELAY
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSP_INGR_PATH_DELAY  VTSS_IOREG(0x38)


/** 
 * \brief DelayAsymmetry
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_TSP:INGR_DELAY_ASYMMETRY
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSP_INGR_DELAY_ASYMMETRY  VTSS_IOREG(0x39)

/**
 * Register Group: \a PTP:INGR_IP_1588_DF
 *
 * 1588 IP Delay FIFO
 */


/** 
 * \brief Configuration and Control register for the Delay FIFO
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_DF:INGR_DF_CTRL
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_DF_INGR_DF_CTRL  VTSS_IOREG(0x3a)

/** 
 * \brief
 * The index of the register stage in the Delay FIFO that is used for
 * output. The actual delay through the block is one more than the depth.
 * If depth is set to 2, then the delay is 3 clocks as data is taken from
 * stage 2. The depth MUST be greater than 0 (depth of 0 is not allowed).
 *
 * \details 
 * This bit group must be set to 0x0F in the VSC8487-15.

 *
 * Field: VTSS_PTP_INGR_IP_1588_DF_INGR_DF_CTRL . INGR_DF_DEPTH
 */
#define  VTSS_F_PTP_INGR_IP_1588_DF_INGR_DF_CTRL_INGR_DF_DEPTH(x)  VTSS_ENCODE_BITFIELD(x,0,5)
#define  VTSS_M_PTP_INGR_IP_1588_DF_INGR_DF_CTRL_INGR_DF_DEPTH     VTSS_ENCODE_BITMASK(0,5)
#define  VTSS_X_PTP_INGR_IP_1588_DF_INGR_DF_CTRL_INGR_DF_DEPTH(x)  VTSS_EXTRACT_BITFIELD(x,0,5)

/**
 * Register Group: \a PTP:INGR_IP_1588_TSFIFO
 *
 * 1588 IP Timestamp FIFO
 */


/** 
 * \brief Timestamp FIFO configuration and status
 *
 * \details
 * Configuration and status register for the Timestamp FIFO
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_CSR
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR  VTSS_IOREG(0x3b)

/** 
 * \brief
 * Selects a smaller timestamp size to be stored in the Timestamp FIFO (4
 * bytes vs. the default 10 bytes).
 *
 * \details 
 * 0 = full 10 byte timestamps are stored
 * 1 = Only 4 bytes of each timestamp are stored.
 *
 * Field: VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR . INGR_TS_4BYTES
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_4BYTES  VTSS_BIT(17)

/** 
 * \brief
 * Forces the TS_FIFO into the reset state.
 *
 * \details 
 * Field: VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR . INGR_TS_FIFO_RESET
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_FIFO_RESET  VTSS_BIT(16)

/** 
 * \brief
 * The FIFO level associated with the last read of the TS_EMPTY status
 * field of the TSFIFO_0 register.
 *
 * \details 
 * Binary number (0-8)
 *
 * Field: VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR . INGR_TS_LEVEL
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_LEVEL(x)  VTSS_ENCODE_BITFIELD(x,12,4)
#define  VTSS_M_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_LEVEL     VTSS_ENCODE_BITMASK(12,4)
#define  VTSS_X_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_LEVEL(x)  VTSS_EXTRACT_BITFIELD(x,12,4)

/** 
 * \brief
 * The threshold at which the Timestamp FIFO interrupt TS_LEVEL_STICKY will
 * be set. If the FIFO level reaches the threshold, the sticky bit
 * TS_LEVEL_STICKY will be set.
 *
 * \details 
 * Binary number (1-8)
 *
 * Field: VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR . INGR_TS_THRESH
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_THRESH(x)  VTSS_ENCODE_BITFIELD(x,8,4)
#define  VTSS_M_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_THRESH     VTSS_ENCODE_BITMASK(8,4)
#define  VTSS_X_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_THRESH(x)  VTSS_EXTRACT_BITFIELD(x,8,4)

/** 
 * \brief
 * Indicates the number of signature bytes used for timestamps in the
 * Timestamp FIFO (0-16).
 *
 * \details 
 * Field: VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR . INGR_TS_SIGNAT_BYTES
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_SIGNAT_BYTES(x)  VTSS_ENCODE_BITFIELD(x,0,5)
#define  VTSS_M_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_SIGNAT_BYTES     VTSS_ENCODE_BITMASK(0,5)
#define  VTSS_X_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_CSR_INGR_TS_SIGNAT_BYTES(x)  VTSS_EXTRACT_BITFIELD(x,0,5)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO along with the FIFO empty flag in
 * the MSB
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_0
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0  VTSS_IOREG(0x3c)

/** 
 * \brief
 * The FIFO empty flag from the Timestamp FIFO. If this bit is set, there
 * is no FIFO data to be read from the FIFO. The data in the TSFIFO_x
 * registers is not valid and should be discarded. When 0, the FIFO has
 * data and the TSFIFO_x has a valid set of data. This register can be
 * polled and when the bit is cleared, the other registers should be read
 * to get a full timestamp. When 1, the last data has already been read out
 * and the current read data should be discarded. 
 *   
 *   Timestamp/Frame signature bytes are packed such that the 10 or 4 byte
 * Timestamp resides in the LEAST significant bytes while the Frame
 * signature (0 to 16 bytes) resides in the MOST significant bytes. The
 * order of the bytes within each Timestamp/Frame signature field is also
 * most significant to least significant. 
 *   
 *  For example, a 26 byte timestamp/frame signature pairs are packed with
 * the 10 byte timestamp field ([79:0]) corresponding to Bits 79:0 in the
 * registers below, and a 16 byte frame signature field ([127:0])
 * corresponding to Bits 207:80 in the registers below.
 *
 * \details 
 * 0 = FIFO not empty, data valid
 * 1 = FIFO empty, data invalid
 *
 * Field: VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0 . INGR_TS_EMPTY
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0_INGR_TS_EMPTY  VTSS_BIT(31)

/** 
 * \brief
 * The FIFO flags from the Timestamp FIFO. These bits indicate how many
 * timestamps are valid in the current (not empty) 26 byte FIFO entry.
 *
 * \details 
 * 000 = Only the end of a partial timestamp is valid in the current FIFO
 * entry (any remaining data is invalid)
 *		      001 = 1 valid timestamp begins in the current FIFO
 * entry (any remaining data is invalid)
 *		      010 = 2 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      011 = 3 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      100 = 4 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      101 = 5 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      110 = 6 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      111 = The current FIFO entry is fully packed with
 * timestamps (all data is valid)
 *
 * Field: VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0 . INGR_TS_FLAGS
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0_INGR_TS_FLAGS(x)  VTSS_ENCODE_BITFIELD(x,28,3)
#define  VTSS_M_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0_INGR_TS_FLAGS     VTSS_ENCODE_BITMASK(28,3)
#define  VTSS_X_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0_INGR_TS_FLAGS(x)  VTSS_EXTRACT_BITFIELD(x,28,3)

/** 
 * \brief
 * 16 bits from the Timestamp FIFO. Bits 15:0.
 *
 * \details 
 * Field: VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0 . INGR_TSFIFO_0
 */
#define  VTSS_F_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0_INGR_TSFIFO_0(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0_INGR_TSFIFO_0     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_0_INGR_TSFIFO_0(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_1
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_1  VTSS_IOREG(0x3d)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_2
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_2  VTSS_IOREG(0x3e)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_3
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_3  VTSS_IOREG(0x3f)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_4
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_4  VTSS_IOREG(0x40)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_5
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_5  VTSS_IOREG(0x41)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_6
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_6  VTSS_IOREG(0x42)


/** 
 * \brief Count of dropped Timestamps
 *
 * \details
 * Count of dropped Timestamps not enqueued to the TS FIFO
 *
 * Register: \a PTP:INGR_IP_1588_TSFIFO:INGR_TSFIFO_DROP_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_TSFIFO_INGR_TSFIFO_DROP_CNT  VTSS_IOREG(0x43)

/**
 * Register Group: \a PTP:INGR_IP_1588_RW
 *
 * 1588 IP Rewriter
 */


/** 
 * \brief Rewriter configuration and control
 *
 * \details
 * Configuration for the Rewriter
 *
 * Register: \a PTP:INGR_IP_1588_RW:INGR_RW_CTRL
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_RW_INGR_RW_CTRL  VTSS_IOREG(0x44)

/** 
 * \brief
 * When set, the 1588 IP will reduce the preamble of ALL incoming frames by
 * 4 bytes to allow a timestamp to be appended to the ingress data frames.
 * This bit must be set along with proper configuration of the Analyzer to
 * ensure proper operation. ** VALID IN INGRESS DIRECTION ONLY **
 *
 * \details 
 * 0 = No preamble modification
 * 1 = Reduce preamble by 4 bytes
 *
 * Field: VTSS_PTP_INGR_IP_1588_RW_INGR_RW_CTRL . INGR_RW_REDUCE_PREAMBLE
 */
#define  VTSS_F_PTP_INGR_IP_1588_RW_INGR_RW_CTRL_INGR_RW_REDUCE_PREAMBLE  VTSS_BIT(4)

/** 
 * \brief
 * Value to write to the flag bit when it is overwritten.
 *
 * \details 
 * 0 = '0' will be written to the flag bit
 * 1 = '1' will be written to the flag bit
 *
 * Field: VTSS_PTP_INGR_IP_1588_RW_INGR_RW_CTRL . INGR_RW_FLAG_VAL
 */
#define  VTSS_F_PTP_INGR_IP_1588_RW_INGR_RW_CTRL_INGR_RW_FLAG_VAL  VTSS_BIT(3)

/** 
 * \brief
 * Bit offset within a byte of the flag bit which indicates if the frame
 * has been modified or not.
 *
 * \details 
 * Binary number
 *
 * Field: VTSS_PTP_INGR_IP_1588_RW_INGR_RW_CTRL . INGR_RW_FLAG_BIT
 */
#define  VTSS_F_PTP_INGR_IP_1588_RW_INGR_RW_CTRL_INGR_RW_FLAG_BIT(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_PTP_INGR_IP_1588_RW_INGR_RW_CTRL_INGR_RW_FLAG_BIT     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_PTP_INGR_IP_1588_RW_INGR_RW_CTRL_INGR_RW_FLAG_BIT(x)  VTSS_EXTRACT_BITFIELD(x,0,3)


/** 
 * \brief Count of modified frames
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_RW:INGR_RW_MODFRM_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_RW_INGR_RW_MODFRM_CNT  VTSS_IOREG(0x45)


/** 
 * \brief Count of FCS errors
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_RW:INGR_RW_FCS_ERR_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_RW_INGR_RW_FCS_ERR_CNT  VTSS_IOREG(0x46)


/** 
 * \brief Count of the number of preamble errors
 *
 * \details
 * Register: \a PTP:INGR_IP_1588_RW:INGR_RW_PREAMBLE_ERR_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_INGR_IP_1588_RW_INGR_RW_PREAMBLE_ERR_CNT  VTSS_IOREG(0x47)

/**
 * Register Group: \a PTP:EGR_IP_1588_CFG_STAT
 *
 * 1588 IP Control & Status Registers
 */


/** 
 * \brief IP 1588 Interrupt Status Register
 *
 * \details
 * Status sticky conditions for the 1588 IP
 *
 * Register: \a PTP:EGR_IP_1588_CFG_STAT:EGR_INT_STATUS
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS  VTSS_IOREG(0x4d)

/** 
 * \brief
 * Indicates that more than one engine has pruduced a match
 *
 * \details 
 * 0 = No error found
 * 1 = Duplicate match found
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS . EGR_ANALYZER_ERROR_STICKY
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS_EGR_ANALYZER_ERROR_STICKY  VTSS_BIT(6)

/** 
 * \brief
 * When set, indicates that a preamble that was too short to modify was
 * detected in a PTP frame. Write to 0 to clear. This occurs when the
 * Rewriter needs to shrink the preamble to append a timestamp but cannot
 * because the preamble is too short. A short preamble is any preamble that
 * is less than 8 characters long including the XGMII /S/ character and the
 * ending SFD of 0xD5. Other preamble values are not checked, only the
 * length.
 *
 * \details 
 * 0 = No error
 * 1 = Preamble too short error
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS . EGR_RW_PREAMBLE_ERR_STICKY
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS_EGR_RW_PREAMBLE_ERR_STICKY  VTSS_BIT(5)

/** 
 * \brief
 * When set, indicates that an FCS error was detected in a PTP frame. Write
 * to 0 to clear.
 *
 * \details 
 * 0 = No error
 * 1 = FCS error
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS . EGR_RW_FCS_ERR_STICKY
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS_EGR_RW_FCS_ERR_STICKY  VTSS_BIT(4)

/** 
 * \brief
 * When set, indicates that the level in the Timestamp FIFO has reached the
 * threshold TS_THRESH. The sticky bit should be reset by writing it to
 * zero.
 *
 * \details 
 * 0 = No overflow
 * 1 = Overflow
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS . EGR_TS_LEVEL_STICKY
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS_EGR_TS_LEVEL_STICKY  VTSS_BIT(3)

/** 
 * \brief
 * When set, indicates a timestamp was captured in the Timestamp FIFO. The
 * sticky bit should be reset by writing it to zero.
 *
 * \details 
 * 0 = No overflow
 * 1 = Overflow
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS . EGR_TS_LOADED_STICKY
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS_EGR_TS_LOADED_STICKY  VTSS_BIT(2)

/** 
 * \brief
 * When set, indicates an underflow in the Timestamp FIFO. The sticky bit
 * should be reset by writing it to zero.
 *
 * \details 
 * 0 = No overflow
 * 1 = Overflow
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS . EGR_TS_UNDERFLOW_STICKY
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS_EGR_TS_UNDERFLOW_STICKY  VTSS_BIT(1)

/** 
 * \brief
 * When set, indicates an overflow in the Timestamp FIFO. The sticky bit
 * should be reset by writing it to zero.
 *
 * \details 
 * 0 = No overflow
 * 1 = Overflow
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS . EGR_TS_OVERFLOW_STICKY
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_STATUS_EGR_TS_OVERFLOW_STICKY  VTSS_BIT(0)


/** 
 * \brief IP 1588 Interrupt Mask Register
 *
 * \details
 * Masks that enable and disable the interrupts
 *
 * Register: \a PTP:EGR_IP_1588_CFG_STAT:EGR_INT_MASK
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK  VTSS_IOREG(0x4e)

/** 
 * \brief
 * Mask bit for ANALYZER_ERROR_STICKY bit.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK . EGR_ANALYZER_ERROR_MASK
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK_EGR_ANALYZER_ERROR_MASK  VTSS_BIT(6)

/** 
 * \brief
 * Mask for the RW_PREAMBLE_ERR_STICKY bit.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK . EGR_RW_PREAMBLE_ERR_MASK
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK_EGR_RW_PREAMBLE_ERR_MASK  VTSS_BIT(5)

/** 
 * \brief
 * Mask for the RW_FCS_ERR_STICKY bit.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK . EGR_RW_FCS_ERR_MASK
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK_EGR_RW_FCS_ERR_MASK  VTSS_BIT(4)

/** 
 * \brief
 * Mask bit for TS_LEVEL_STICKY. When 1, the interrupt is enabled.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK . EGR_TS_LEVEL_MASK
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK_EGR_TS_LEVEL_MASK  VTSS_BIT(3)

/** 
 * \brief
 * Mask bit for TS_LOADED_STICKY. When 1, the interrupt is enabled.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK . EGR_TS_LOADED_MASK
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK_EGR_TS_LOADED_MASK  VTSS_BIT(2)

/** 
 * \brief
 * Mask bit for TS_UNDERFLOW_STICKY. When 1, the interrupt is enabled.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK . EGR_TS_UNDERFLOW_MASK
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK_EGR_TS_UNDERFLOW_MASK  VTSS_BIT(1)

/** 
 * \brief
 * Mask bit for TS_OVERFLOW_STICKY. When 1, the interrupt is enabled.
 *
 * \details 
 * 0 = Interrupt disabled
 * 1 = Interrupt enabled
 *
 * Field: VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK . EGR_TS_OVERFLOW_MASK
 */
#define  VTSS_F_PTP_EGR_IP_1588_CFG_STAT_EGR_INT_MASK_EGR_TS_OVERFLOW_MASK  VTSS_BIT(0)


/** 
 * \brief Spare scratchpad register
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_CFG_STAT:EGR_SPARE_REGISTER
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_CFG_STAT_EGR_SPARE_REGISTER  VTSS_IOREG(0x4f)

/**
 * Register Group: \a PTP:EGR_IP_1588_TSP
 *
 * 1588 IP Timestamp Processor
 */


/** 
 * \brief TSP Control
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_TSP:EGR_TSP_CTRL
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSP_EGR_TSP_CTRL  VTSS_IOREG(0x55)

/** 
 * \brief
 * Selects a mode in which the fractional portion of a second (in units of
 * nanoseconds) is used for timestamping. Only the operation of the
 * WRITE_NS, WRITE_NS_P2P, and SUB_ADD PTP commands are affected by the
 * setting of this mode bit.
 *
 * \details 
 * 0 = Select the total (summed) nanoseconds for timestamping.
 * 1 = Select the fractional portion in nanoseconds for timestamping.
 *
 * Field: VTSS_PTP_EGR_IP_1588_TSP_EGR_TSP_CTRL . EGR_FRACT_NS_MODE
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSP_EGR_TSP_CTRL_EGR_FRACT_NS_MODE  VTSS_BIT(2)

/** 
 * \brief
 * Select external pin start of frame indicator.
 *
 * \details 
 * 0 = Select internal PCS as the source of SOF
 * 1 = Select external pin as the source of SOF.
 *
 * Field: VTSS_PTP_EGR_IP_1588_TSP_EGR_TSP_CTRL . EGR_SEL_EXT_SOF_IND
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSP_EGR_TSP_CTRL_EGR_SEL_EXT_SOF_IND  VTSS_BIT(1)

/** 
 * \brief
 * One-shot loads Local latency, Path delay, and DelayAsymmetry values into
 * the Timestamp Processor
 *
 * \details 
 * Field: VTSS_PTP_EGR_IP_1588_TSP_EGR_TSP_CTRL . EGR_LOAD_DELAYS
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSP_EGR_TSP_CTRL_EGR_LOAD_DELAYS  VTSS_BIT(0)


/** 
 * \brief TSP Status
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_TSP:EGR_TSP_STAT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSP_EGR_TSP_STAT  VTSS_IOREG(0x56)

/** 
 * \brief
 * Timestamp procesor marked a calculated correction field as too big.
 *
 * \details 
 * 0 = A calculated correction field that was too big did occur.
 * 1 = A calculated correction field that was too big did not occur.
 *
 * Field: VTSS_PTP_EGR_IP_1588_TSP_EGR_TSP_STAT . EGR_CF_TOO_BIG_STICKY
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSP_EGR_TSP_STAT_EGR_CF_TOO_BIG_STICKY  VTSS_BIT(0)


/** 
 * \brief Local latency
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_TSP:EGR_LOCAL_LATENCY
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSP_EGR_LOCAL_LATENCY  VTSS_IOREG(0x57)

/** 
 * \brief
 * Local latency (nanoseconds)
 *
 * \details 
 * 
 * The value programmed in this register is dependent upon the frequency of
 * the clock driving the Local Time Counter (LTC) and upon WAN/LAN mode of
 * operation.
 * 
 *   When in LAN mode and the LTC clock frequency is 250 MHz, set this
 * register to 206.
 *   When in LAN mode and the LTC clock frequency is 156.25 MHz, set this
 * register to 202.
 *   When in LAN mode and the LTC clock frequency is 125 MHz, set this
 * register to 200.
 * 
 *   When in WAN mode and the LTC clock frequency is 250 MHz, set this
 * register to 213.
 *   When in WAN mode and the LTC clock frequency is 156.25 MHz, set this
 * register to 210.
 *   When in WAN mode and the LTC clock frequency is 125 MHz, set this
 * register to 207.

 *
 * Field: VTSS_PTP_EGR_IP_1588_TSP_EGR_LOCAL_LATENCY . EGR_LOCAL_LATENCY
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSP_EGR_LOCAL_LATENCY_EGR_LOCAL_LATENCY(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_EGR_IP_1588_TSP_EGR_LOCAL_LATENCY_EGR_LOCAL_LATENCY     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_EGR_IP_1588_TSP_EGR_LOCAL_LATENCY_EGR_LOCAL_LATENCY(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief Path delay
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_TSP:EGR_PATH_DELAY
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSP_EGR_PATH_DELAY  VTSS_IOREG(0x58)


/** 
 * \brief DelayAsymmetry
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_TSP:EGR_DELAY_ASYMMETRY
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSP_EGR_DELAY_ASYMMETRY  VTSS_IOREG(0x59)

/**
 * Register Group: \a PTP:EGR_IP_1588_DF
 *
 * 1588 IP Delay FIFO
 */


/** 
 * \brief Configuration and Control register for the Delay FIFO
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_DF:EGR_DF_CTRL
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_DF_EGR_DF_CTRL  VTSS_IOREG(0x5a)

/** 
 * \brief
 * The index of the register stage in the Delay FIFO that is used for
 * output. The actual delay through the block is one more than the depth.
 * If depth is set to 2, then the delay is 3 clocks as data is taken from
 * stage 2. The depth MUST be greater than 0 (depth of 0 is not allowed).
 *
 * \details 
 * This bit group must be set to 0x0F in the VSC8487-15.

 *
 * Field: VTSS_PTP_EGR_IP_1588_DF_EGR_DF_CTRL . EGR_DF_DEPTH
 */
#define  VTSS_F_PTP_EGR_IP_1588_DF_EGR_DF_CTRL_EGR_DF_DEPTH(x)  VTSS_ENCODE_BITFIELD(x,0,5)
#define  VTSS_M_PTP_EGR_IP_1588_DF_EGR_DF_CTRL_EGR_DF_DEPTH     VTSS_ENCODE_BITMASK(0,5)
#define  VTSS_X_PTP_EGR_IP_1588_DF_EGR_DF_CTRL_EGR_DF_DEPTH(x)  VTSS_EXTRACT_BITFIELD(x,0,5)

/**
 * Register Group: \a PTP:EGR_IP_1588_TSFIFO
 *
 * 1588 IP Timestamp FIFO
 */


/** 
 * \brief Timestamp FIFO configuration and status
 *
 * \details
 * Configuration and status register for the Timestamp FIFO
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_CSR
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR  VTSS_IOREG(0x5b)

/** 
 * \brief
 * Selects a smaller timestamp size to be stored in the Timestamp FIFO (4
 * bytes vs. the default 10 bytes).
 *
 * \details 
 * 0 = full 10 byte timestamps are stored
 * 1 = Only 4 bytes of each timestamp are stored.
 *
 * Field: VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR . EGR_TS_4BYTES
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_4BYTES  VTSS_BIT(17)

/** 
 * \brief
 * Forces the TS_FIFO into the reset state.
 *
 * \details 
 * Field: VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR . EGR_TS_FIFO_RESET
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_FIFO_RESET  VTSS_BIT(16)

/** 
 * \brief
 * The FIFO level associated with the last read of the TS_EMPTY status
 * field of the TSFIFO_0 register.
 *
 * \details 
 * Binary number (0-8)
 *
 * Field: VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR . EGR_TS_LEVEL
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_LEVEL(x)  VTSS_ENCODE_BITFIELD(x,12,4)
#define  VTSS_M_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_LEVEL     VTSS_ENCODE_BITMASK(12,4)
#define  VTSS_X_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_LEVEL(x)  VTSS_EXTRACT_BITFIELD(x,12,4)

/** 
 * \brief
 * The threshold at which the Timestamp FIFO interrupt TS_LEVEL_STICKY will
 * be set. If the FIFO level reaches the threshold, the sticky bit
 * TS_LEVEL_STICKY will be set.
 *
 * \details 
 * Binary number (1-8)
 *
 * Field: VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR . EGR_TS_THRESH
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_THRESH(x)  VTSS_ENCODE_BITFIELD(x,8,4)
#define  VTSS_M_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_THRESH     VTSS_ENCODE_BITMASK(8,4)
#define  VTSS_X_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_THRESH(x)  VTSS_EXTRACT_BITFIELD(x,8,4)

/** 
 * \brief
 * Indicates the number of signature bytes used for timestamps in the
 * Timestamp FIFO (0-16).
 *
 * \details 
 * Field: VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR . EGR_TS_SIGNAT_BYTES
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_SIGNAT_BYTES(x)  VTSS_ENCODE_BITFIELD(x,0,5)
#define  VTSS_M_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_SIGNAT_BYTES     VTSS_ENCODE_BITMASK(0,5)
#define  VTSS_X_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_CSR_EGR_TS_SIGNAT_BYTES(x)  VTSS_EXTRACT_BITFIELD(x,0,5)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO along with the FIFO empty flag in
 * the MSB
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_0
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0  VTSS_IOREG(0x5c)

/** 
 * \brief
 * The FIFO empty flag from the Timestamp FIFO. If this bit is set, there
 * is no FIFO data to be read from the FIFO. The data in the TSFIFO_x
 * registers is not valid and should be discarded. When 0, the FIFO has
 * data and the TSFIFO_x has a valid set of data. This register can be
 * polled and when the bit is cleared, the other registers should be read
 * to get a full timestamp. When 1, the last data has already been read out
 * and the current read data should be discarded. 
 *   
 *   Timestamp/Frame signature bytes are packed such that the 10 or 4 byte
 * Timestamp resides in the LEAST significant bytes while the Frame
 * signature (0 to 16 bytes) resides in the MOST significant bytes. The
 * order of the bytes within each Timestamp/Frame signature field is also
 * most significant to least significant. 
 *   
 *  For example, a 26 byte timestamp/frame signature pairs are packed with
 * the 10 byte timestamp field ([79:0]) corresponding to Bits 79:0 in the
 * registers below, and a 16 byte frame signature field ([127:0])
 * corresponding to Bits 207:80 in the registers below.
 *
 * \details 
 * 0 = FIFO not empty, data valid
 * 1 = FIFO empty, data invalid
 *
 * Field: VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0 . EGR_TS_EMPTY
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0_EGR_TS_EMPTY  VTSS_BIT(31)

/** 
 * \brief
 * The FIFO flags from the Timestamp FIFO. These bits indicate how many
 * timestamps are valid in the current (not empty) 26 byte FIFO entry.
 *
 * \details 
 * 000 = Only the end of a partial timestamp is valid in the current FIFO
 * entry (any remaining data is invalid)
 *		      001 = 1 valid timestamp begins in the current FIFO
 * entry (any remaining data is invalid)
 *		      010 = 2 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      011 = 3 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      100 = 4 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      101 = 5 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      110 = 6 valid timestamps begin in the current FIFO
 * entry (any remaining data is invalid)
 *		      111 = The current FIFO entry is fully packed with
 * timestamps (all data is valid)
 *
 * Field: VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0 . EGR_TS_FLAGS
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0_EGR_TS_FLAGS(x)  VTSS_ENCODE_BITFIELD(x,28,3)
#define  VTSS_M_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0_EGR_TS_FLAGS     VTSS_ENCODE_BITMASK(28,3)
#define  VTSS_X_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0_EGR_TS_FLAGS(x)  VTSS_EXTRACT_BITFIELD(x,28,3)

/** 
 * \brief
 * 16 bits from the Timestamp FIFO. Bits 15:0.
 *
 * \details 
 * Field: VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0 . EGR_TSFIFO_0
 */
#define  VTSS_F_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0_EGR_TSFIFO_0(x)  VTSS_ENCODE_BITFIELD(x,0,16)
#define  VTSS_M_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0_EGR_TSFIFO_0     VTSS_ENCODE_BITMASK(0,16)
#define  VTSS_X_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_0_EGR_TSFIFO_0(x)  VTSS_EXTRACT_BITFIELD(x,0,16)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_1
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_1  VTSS_IOREG(0x5d)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_2
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_2  VTSS_IOREG(0x5e)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_3
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_3  VTSS_IOREG(0x5f)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_4
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_4  VTSS_IOREG(0x60)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_5
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_5  VTSS_IOREG(0x61)


/** 
 * \brief Data value from the Timestamp FIFO
 *
 * \details
 * Read the data from the timestamp FIFO
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_6
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_6  VTSS_IOREG(0x62)


/** 
 * \brief Count of dropped Timestamps
 *
 * \details
 * Count of dropped Timestamps not enqueued to the TS FIFO
 *
 * Register: \a PTP:EGR_IP_1588_TSFIFO:EGR_TSFIFO_DROP_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_TSFIFO_EGR_TSFIFO_DROP_CNT  VTSS_IOREG(0x63)

/**
 * Register Group: \a PTP:EGR_IP_1588_RW
 *
 * 1588 IP Rewriter
 */


/** 
 * \brief Rewriter configuration and control
 *
 * \details
 * Configuration for the Rewriter
 *
 * Register: \a PTP:EGR_IP_1588_RW:EGR_RW_CTRL
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_RW_EGR_RW_CTRL  VTSS_IOREG(0x64)

/** 
 * \brief
 * When set, the 1588 IP will reduce the preamble of ALL incoming frames by
 * 4 bytes to allow a timestamp to be appended to the ingress data frames.
 * This bit must be set along with proper configuration of the Analyzer to
 * ensure proper operation. ** VALID IN INGRESS DIRECTION ONLY **
 *
 * \details 
 * 0 = No preamble modification
 * 1 = Reduce preamble by 4 bytes
 *
 * Field: VTSS_PTP_EGR_IP_1588_RW_EGR_RW_CTRL . EGR_RW_REDUCE_PREAMBLE
 */
#define  VTSS_F_PTP_EGR_IP_1588_RW_EGR_RW_CTRL_EGR_RW_REDUCE_PREAMBLE  VTSS_BIT(4)

/** 
 * \brief
 * Value to write to the flag bit when it is overwritten.
 *
 * \details 
 * 0 = '0' will be written to the flag bit
 * 1 = '1' will be written to the flag bit
 *
 * Field: VTSS_PTP_EGR_IP_1588_RW_EGR_RW_CTRL . EGR_RW_FLAG_VAL
 */
#define  VTSS_F_PTP_EGR_IP_1588_RW_EGR_RW_CTRL_EGR_RW_FLAG_VAL  VTSS_BIT(3)

/** 
 * \brief
 * Bit offset within a byte of the flag bit which indicates if the frame
 * has been modified or not.
 *
 * \details 
 * Binary number
 *
 * Field: VTSS_PTP_EGR_IP_1588_RW_EGR_RW_CTRL . EGR_RW_FLAG_BIT
 */
#define  VTSS_F_PTP_EGR_IP_1588_RW_EGR_RW_CTRL_EGR_RW_FLAG_BIT(x)  VTSS_ENCODE_BITFIELD(x,0,3)
#define  VTSS_M_PTP_EGR_IP_1588_RW_EGR_RW_CTRL_EGR_RW_FLAG_BIT     VTSS_ENCODE_BITMASK(0,3)
#define  VTSS_X_PTP_EGR_IP_1588_RW_EGR_RW_CTRL_EGR_RW_FLAG_BIT(x)  VTSS_EXTRACT_BITFIELD(x,0,3)


/** 
 * \brief Count of modified frames
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_RW:EGR_RW_MODFRM_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_RW_EGR_RW_MODFRM_CNT  VTSS_IOREG(0x65)


/** 
 * \brief Count of FCS errors
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_RW:EGR_RW_FCS_ERR_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_RW_EGR_RW_FCS_ERR_CNT  VTSS_IOREG(0x66)


/** 
 * \brief Count of the number of preamble errors
 *
 * \details
 * Register: \a PTP:EGR_IP_1588_RW:EGR_RW_PREAMBLE_ERR_CNT
 *
 * @param target A \a ::vtss_target_PTP_e target
 */
#define VTSS_PTP_EGR_IP_1588_RW_EGR_RW_PREAMBLE_ERR_CNT  VTSS_IOREG(0x67)


#endif /* _VTSS_PHY_TS_REGS_PTP_H_ */
