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
 
 $Id$
 $Revision$

*/

#ifndef _VTSS_PHY_H_
#define _VTSS_PHY_H_

#include "vtss_phy_veriphy.h"


/* PHY family */
typedef enum {
    VTSS_PHY_FAMILY_NONE,     /* Unknown */
    VTSS_PHY_FAMILY_MUSTANG,  /* VSC8201 */
    VTSS_PHY_FAMILY_BLAZER,   /* VSC8204 */
    VTSS_PHY_FAMILY_COBRA,    /* VSC8211/21 */
    VTSS_PHY_FAMILY_QUATTRO,  /* VSC8224/34/44 */
    VTSS_PHY_FAMILY_SPYDER,   /* VSC8538/58/8658 */
    VTSS_PHY_FAMILY_COOPER,   /* VSC8601/41 */
    VTSS_PHY_FAMILY_LUTON,    /* VSC7385/88 */
    VTSS_PHY_FAMILY_LUTON24,  /* VSC7389/90/91 */
    VTSS_PHY_FAMILY_LUTON_E,  /* VSC7395/96/98 */
    VTSS_PHY_FAMILY_INTRIGUE, /* VSC7500-7507 */
    VTSS_PHY_FAMILY_ENZO,     /* VSC8634/64 */
    VTSS_PHY_FAMILY_ATOM,     /* VSC8512/8522 */
    VTSS_PHY_FAMILY_LUTON26,  /* VSC7420*/
    VTSS_PHY_FAMILY_TESLA     /* VSC8574*/
} vtss_phy_family_t;



/* Define revision for the different chips. */
#define VTSS_PHY_ATOM_REV_A 0
#define VTSS_PHY_ATOM_REV_B 1
#define VTSS_PHY_ATOM_REV_C 2
#define VTSS_PHY_ATOM_REV_D 3

#define VTSS_PHY_TESLA_REV_A 0
#define VTSS_PHY_TESLA_REV_B 1
#define VTSS_PHY_TESLA_REV_D 2

typedef enum {
    VTSS_CAP_INT = (1 << 0),    /* Interrupts */
    VTSS_CAP_EEE = (1 << 1),    /* EEE */
} vtss_phy_feature_t;

/* Power configuration */
typedef struct {
    BOOL vtss_phy_power_dynamic;
    BOOL vtss_phy_actiphy_on;
    u32  pwrusage;
} vtss_phy_power_t;

typedef struct {
    vtss_phy_clock_conf_t  conf;
    vtss_port_no_t         source;
} phy_clock_conf_t;


typedef struct _vtss_phy_port_state_info_t {
    vtss_phy_reset_conf_t  reset;      /* Reset setup */
    vtss_phy_family_t      family;     /* Family */
    vtss_phy_type_t        type;       /* Type */
    u16                    revision;   /* Revision number */
    u16                    features;   /* Features supported */
    vtss_phy_conf_t        setup;      /* Setup */
    vtss_port_status_t     status;     /* Status */
    vtss_phy_power_t       power;      /* Power Reduction Setup */
    vtss_phy_eee_conf_t    eee_conf;   /* EEE configuration.  */
    vtss_phy_power_conf_t  conf_power; 
    vtss_phy_conf_1g_t     conf_1g;    /* 1g setup */
    vtss_phy_status_1g_t   status_1g;  /* 1G Status */
#if VTSS_PHY_OPT_VERIPHY
    vtss_veriphy_task_t    veriphy;  /* VeriPHY task */
#endif /* VTSS_PHY_OPT_VERIPHY */
    vtss_phy_loopback_t    loopback; // Loop back
    BOOL                   conf_none; // Signaling the no PHY is found for this port */
    vtss_phy_event_t       ev_mask;    // Mask for interrupt events
    phy_clock_conf_t       clock_conf[VTSS_PHY_RECOV_CLK_NUM];
} vtss_phy_port_state_t;

vtss_rc vtss_phy_init_conf_set(void);
vtss_rc vtss_phy_restart_conf_set(void);

vtss_rc vtss_phy_status_get_private(const vtss_port_no_t port_no,
                                    vtss_port_status_t   *const status);


vtss_rc vtss_phy_chip_temp_get_private (const vtss_port_no_t  port_no, 
                                        i16                    *const temp);
    
vtss_rc vtss_phy_chip_temp_init_private(const vtss_port_no_t  port_no);


vtss_rc vtss_phy_rd(const vtss_port_no_t port_no,
                    const u32            addr,
                    u16                  *const value);

vtss_rc vtss_phy_wr(const vtss_port_no_t port_no,
                    const u32            addr,
                    const u16            value);

vtss_rc vtss_phy_wr_masked(const vtss_port_no_t port_no,
                           const u32            addr,
                           const u16            value,
                           const u16            mask);

/* Write PHY register, masked (including the page) */
vtss_rc vtss_phy_wr_masked_page(const vtss_port_no_t port_no,
                                const u16            page,
                                const u32            addr,
                                const u16            value,
                                const u16            mask,
                                const u16            line);

/* Read PHY register (including the page) */
vtss_rc vtss_phy_rd_page(const vtss_port_no_t port_no,
                         const u16            page,
                         const u32            addr,
                         u16                  *const value,
                         const u16            line);

/* Write PHY register (including the page) */
vtss_rc vtss_phy_wr_page(const vtss_port_no_t port_no,
                         const u16            page,
                         const u32            addr,
                         const u16            value,
                         const u16            line);


#define PHY_WR_PAGE(port_no, page_addr, value) vtss_phy_wr_page(port_no, page_addr, value, __LINE__)
#define PHY_WR_MASKED_PAGE(port_no, page_addr, value, mask) vtss_phy_wr_masked_page(port_no, page_addr, value, mask, __LINE__)
#define PHY_RD_PAGE(port_no, page_addr, value) vtss_phy_rd_page(port_no, page_addr, value, __LINE__)

vtss_rc vtss_phy_page_std(vtss_port_no_t port_no);
vtss_rc vtss_phy_page_ext(vtss_port_no_t port_no);
vtss_rc vtss_phy_page_ext2(vtss_port_no_t port_no);
vtss_rc vtss_phy_page_ext3(vtss_port_no_t port_no);
vtss_rc vtss_phy_page_gpio(vtss_port_no_t port_no);
vtss_rc vtss_phy_page_test(vtss_port_no_t port_no);
vtss_rc vtss_phy_page_tr(vtss_port_no_t port_no);

vtss_rc vtss_phy_reset_private(const vtss_port_no_t port_no);
vtss_rc vtss_phy_sync(const vtss_port_no_t port_no);

vtss_rc vtss_phy_debug_info_print(const vtss_debug_printf_t pr,
                                  const vtss_debug_info_t   *const info,
                                  BOOL                      ail);

BOOL  vtss_phy_veriphy_running(vtss_port_no_t port_no, BOOL set, u8 running);
vtss_rc atom_patch_suspend(vtss_port_no_t iport, BOOL suspend);
vtss_rc vtss_phy_micro_assert_reset(const vtss_port_no_t port_no);

// *********** Registers *************


// Function for setting a bit in a vector.
#define VTSS_PHY_BIT(bit)       (1U << (bit))

// Set x bits to 1. E.g x=3 -> b111 (0x7)
#define VTSS_PHY_BITMASK(x)               ((1U << (x)) - 1)

#define VTSS_PHY_EXTRACT_BITFIELD(value, offset, width)  (((value) >> (offset)) & VTSS_PHY_BITMASK(width))
#define VTSS_PHY_ENCODE_BITFIELD(value, offset, width)   (((value) & VTSS_PHY_BITMASK(width)) << (offset))
#define VTSS_PHY_ENCODE_BITMASK(offset, width)      (VTSS_PHY_BITMASK(width) << (offset))



// ** Register definitions - Standard page **

// Register 0 + Bit fields
#define  VTSS_PHY_MODE_CONTROL VTSS_PHY_PAGE_STANDARD, 0
#define  VTSS_F_PHY_MODE_CONTROL_SW_RESET         VTSS_PHY_BIT(15)
#define  VTSS_F_PHY_MODE_CONTROL_LOOP             VTSS_PHY_BIT(14)
#define  VTSS_F_PHY_MODE_CONTROL_AUTO_NEG_ENA     VTSS_PHY_BIT(12)
#define  VTSS_F_PHY_MODE_CONTROL_POWER_DOWN       VTSS_PHY_BIT(11)
#define  VTSS_F_PHY_MODE_CONTROL_RESTART_AUTO_NEG VTSS_PHY_BIT(9)


// Register 1 + Bit fields
#define  VTSS_PHY_MODE_STATUS VTSS_PHY_PAGE_STANDARD, 1
#define  VTSS_F_PHY_UNIDIRECTIONAL_ABILITY        VTSS_PHY_BIT(7)
#define  VTSS_F_PHY_STATUS_LINK_STATUS            VTSS_PHY_BIT(2)

// Register 4 + Bit fields
#define  VTSS_PHY_DEVICE_AUTONEG_ADVERTISEMENT VTSS_PHY_PAGE_STANDARD, 4

// Register 5 + Bit fields
#define  VTSS_PHY_AUTONEGOTIATION_LINK_PARTNER_ABILITY VTSS_PHY_PAGE_STANDARD, 5

// Register 9 + Bit fields
#define  VTSS_PHY_1000BASE_T_CONTROL  VTSS_PHY_PAGE_STANDARD, 9

// Register 18 + Bit fields
#define VTSS_PHY_BYPASS_CONTROL VTSS_PHY_PAGE_STANDARD, 18
#define VTSS_F_PHY_BYPASS_CONTROL_HP_AUTO_MDIX_AT_FORCE VTSS_PHY_BIT(7)
#define VTSS_F_PHY_BYPASS_CONTROL_DISABLE_PARI_SWAP_CORRECTION VTSS_PHY_BIT(5)

// Register 19 + Bit fields
#define VTSS_PHY_ERROR_COUNTER_1 VTSS_PHY_PAGE_STANDARD, 19
#define VTSS_M_PHY_ERROR_COUNTER_1_100_1000BASETX_RX_ERR_CNT  VTSS_PHY_ENCODE_BITMASK(0, 7)

// Register 22 + Bit fields
#define VTSS_PHY_EXTENDED_CONTROL_AND_STATUS VTSS_PHY_PAGE_STANDARD, 22
#define VTSS_PHY_EXTENDED_CONTROL_AND_STATUS_FORCE_10BASE_T_HIGH VTSS_PHY_BIT(15)

// Register 23 + Bit fields
#define  VTSS_PHY_EXTENDED_PHY_CONTROL VTSS_PHY_PAGE_STANDARD, 23
#define  VTSS_F_PHY_EXTENDED_PHY_CONTROL_MAC_INTERFACE_MODE     VTSS_PHY_BIT(12)
#define  VTSS_F_PHY_EXTENDED_PHY_CONTROL_MEDIA_OPERATING_MODE(value) VTSS_PHY_ENCODE_BITFIELD(value, 8, 3)
#define  VTSS_M_PHY_EXTENDED_PHY_CONTROL_MEDIA_OPERATING_MODE   VTSS_PHY_ENCODE_BITMASK(8,3)
#define  VTSS_F_PHY_EXTENDED_PHY_CONTROL_AMS_PREFERENCE         VTSS_PHY_BIT(11)
#define  VTSS_F_PHY_EXTENDED_PHY_CONTROL_FAR_END_LOOPBACK_MODE  VTSS_PHY_BIT(3)




// Register 25 + Bit fields
#define VTSS_PHY_INTERRUPT_MASK    VTSS_PHY_PAGE_STANDARD, 25

#define VTSS_F_PHY_INTERRUPT_MASK_INT_MASK                     VTSS_PHY_BIT(15)
#define VTSS_F_PHY_INTERRUPT_MASK_SPEED_STATE_CHANGE_MASK      VTSS_PHY_BIT(14)
#define VTSS_F_PHY_INTERRUPT_MASK_LINK_MASK                    VTSS_PHY_BIT(13)
#define VTSS_F_PHY_INTERRUPT_MASK_FDX_STATE_CHANGE_MASK        VTSS_PHY_BIT(12)
#define VTSS_F_PHY_INTERRUPT_MASK_AUTO_NEG_ERROR_MASK          VTSS_PHY_BIT(11)
#define VTSS_F_PHY_INTERRUPT_MASK_AUTO_NEG_COMPLETE_MASK       VTSS_PHY_BIT(10)
#define VTSS_F_PHY_INTERRUPT_MASK_INLINE_POW_DEV_DETECT_MASK   VTSS_PHY_BIT(9)
#define VTSS_F_PHY_INTERRUPT_MASK_SYMBOL_ERR_INT_MASK          VTSS_PHY_BIT(8)
#define VTSS_F_PHY_INTERRUPT_MASK_FAST_LINK_MASK               VTSS_PHY_BIT(7)
#define VTSS_F_PHY_INTERRUPT_MASK_TX_FIFO_OVERFLOW_INT_MASK    VTSS_PHY_BIT(6)
#define VTSS_F_PHY_INTERRUPT_MASK_RX_FIFO_OVERFLOW_INT_MASK    VTSS_PHY_BIT(5)
#define VTSS_F_PHY_INTERRUPT_MASK_AMS_MEDIA_CHANGE_MASK        VTSS_PHY_BIT(4)
#define VTSS_F_PHY_INTERRUPT_MASK_FALSE_CARRIER_INT_MASK       VTSS_PHY_BIT(3)
#define VTSS_F_PHY_INTERRUPT_MASK_LINK_SPEED_DS_DETECT_MASK    VTSS_PHY_BIT(2)
#define VTSS_F_PHY_INTERRUPT_MASK_MASTER_SLAVE_RES_ERR_MASK    VTSS_PHY_BIT(1)
#define VTSS_F_PHY_INTERRUPT_MASK_RX_ER_INT_MASK               VTSS_PHY_BIT(0)

// Register 26 + Bit fields
#define VTSS_PHY_INTERRUPT_STATUS   VTSS_PHY_PAGE_STANDARD, 26

// Register 28 + Bit fields
#define COOPER_VTSS_PHY_AUXILIARY_CONTROL_AND_STATUS   VTSS_PHY_PAGE_STANDARD, 23
#define VTSS_PHY_AUXILIARY_CONTROL_AND_STATUS   VTSS_PHY_PAGE_STANDARD, 28
#define VTSS_F_PHY_AUXILIARY_CONTROL_AND_STATUS_HP_AUTO_MDIX_CROSSOVER_INDICATION VTSS_PHY_BIT(13)
#define VTSS_F_PHY_AUXILIARY_CONTROL_AND_STATUS_ACTIPHY_MODE_ENABLE    VTSS_PHY_BIT(6)

// Register 29 + Bit fields
#define VTSS_PHY_LED_MODE_SELECT VTSS_PHY_PAGE_STANDARD, 29


// Register 30 + Bit fields
#define VTSS_PHY_LED_BEHAVIOR  VTSS_PHY_PAGE_STANDARD, 30
#define VTSS_F_PHY_LED_BEHAVIOR_LED_PULSING_ENABLE VTSS_PHY_BIT(12)

//Register 31
#define VTSS_PHY_PAGE    31

// ** Register definitions -  EXT1 page**

// Register 18E1 + Bit fields
#define VTSS_PHY_CU_MEDIA_CRC_GOOD_COUNTER VTSS_PHY_PAGE_EXTENDED, 18
#define VTSS_F_PHY_CU_MEDIA_CRC_GOOD_COUNTER_PACKET_SINCE_LAST_READ  VTSS_PHY_ENCODE_BITMASK(15, 1)
#define VTSS_M_PHY_CU_MEDIA_CRC_GOOD_COUNTER_CONTENTS  VTSS_PHY_ENCODE_BITMASK(0, 14)

// Register 19E1 + Bit fields
#define VTSS_PHY_EXTENDED_MODE_CONTROL VTSS_PHY_PAGE_EXTENDED, 19
#define VTSS_M_PHY_EXTENDED_MODE_CONTROL_FORCE_MDI_CROSSOVER  VTSS_PHY_ENCODE_BITMASK(2, 2)



// Register 23E1 + Bit fields
#define VTSS_PHY_EXTENDED_PHY_CONTROL_4 VTSS_PHY_PAGE_EXTENDED, 23
#define VTSS_M_VTSS_PHY_EXTENDED_PHY_CONTROL_4_PHY_ADDRESS  VTSS_PHY_ENCODE_BITMASK(11, 5)
#define VTSS_M_VTSS_PHY_EXTENDED_PHY_CONTROL_4_CU_MEDIA_CRC_ERROR_COUNTER  VTSS_PHY_ENCODE_BITMASK(0, 7)


// Register 24E1 + Bit fields
#define VTSS_PHY_VERIPHY_CTRL_REG1 VTSS_PHY_PAGE_EXTENDED, 24
#define VTSS_F_PHY_VERIPHY_CTRL_REG1_TRIGGER VTSS_PHY_BIT(15)
#define VTSS_F_PHY_VERIPHY_CTRL_REG1_VALID   VTSS_PHY_BIT(14)
#define VTSS_M_PHY_VERIPHY_CTRL_REG1_PAIR_A_DISTANCE VTSS_PHY_ENCODE_BITMASK(8, 6)
#define VTSS_M_PHY_VERIPHY_CTRL_REG1_PAIR_B_DISTANCE VTSS_PHY_ENCODE_BITMASK(0, 6)

// Register 25E1 + Bit fields
#define VTSS_PHY_VERIPHY_CTRL_REG2 VTSS_PHY_PAGE_EXTENDED, 25
#define VTSS_M_PHY_VERIPHY_CTRL_REG2_PAIR_C_DISTANCE VTSS_PHY_ENCODE_BITMASK(8, 6)
#define VTSS_M_PHY_VERIPHY_CTRL_REG2_PAIR_D_DISTANCE VTSS_PHY_ENCODE_BITMASK(0, 6)

// Register 26E1 + Bit fields
#define VTSS_PHY_VERIPHY_CTRL_REG3 VTSS_PHY_PAGE_EXTENDED, 26
#define VTSS_M_PHY_VERIPHY_CTRL_REG3_PAIR_A_TERMINATION_STATUS VTSS_PHY_ENCODE_BITMASK(12, 4)
#define VTSS_M_PHY_VERIPHY_CTRL_REG3_PAIR_B_TERMINATION_STATUS VTSS_PHY_ENCODE_BITMASK(8, 4)
#define VTSS_M_PHY_VERIPHY_CTRL_REG3_PAIR_C_TERMINATION_STATUS VTSS_PHY_ENCODE_BITMASK(4, 4)
#define VTSS_M_PHY_VERIPHY_CTRL_REG3_PAIR_D_TERMINATION_STATUS VTSS_PHY_ENCODE_BITMASK(0, 4)



// ** Register definitions -  EXT2 page**
#define VTSS_PHY_EEE_CONTROL VTSS_PHY_PAGE_EXTENDED_2, 17
#define VTSS_F_PHY_EEE_CONTROL_ENABLE_10BASE_TE VTSS_PHY_BIT(15)



// ** Register definitions -  EXT3 page**

// Register 20E3
#define VTSS_PHY_MAC_SERDES_STATUS VTSS_PHY_PAGE_EXTENDED_3, 20


// Register 21E3
#define VTSS_PHY_MEDIA_SERDES_TX_GOOD_PACKET_COUNTER  VTSS_PHY_PAGE_EXTENDED_3, 21
#define VTSS_F_PHY_MEDIA_SERDES_TX_GOOD_PACKET_COUNTER_ACTIVE VTSS_PHY_BIT(15)
#define VTSS_M_PHY_MEDIA_SERDES_TX_GOOD_PACKET_COUNTER_CNT    VTSS_PHY_ENCODE_BITMASK(0, 7)

// Register 22E3
#define VTSS_PHY_MEDIA_SERDES_TX_CRC_ERROR_COUNTER  VTSS_PHY_PAGE_EXTENDED_3, 22
#define VTSS_M_PHY_MEDIA_SERDES_TX_CRC_ERROR_COUNTER_CNT    VTSS_PHY_ENCODE_BITMASK(0, 7)



// ** Register definitions -  GPIO page**
#define VTSS_PHY_GPIO_0 VTSS_PHY_PAGE_GPIO, 0


// Register 14G + Bit fields
#define VTSS_PHY_GPIO_CONTROL_2 VTSS_PHY_PAGE_GPIO, 14
#define VTSS_F_PHY_GPIO_CONTROL_2_COMA_MODE_OUTPUT_ENABLE  VTSS_PHY_BIT(13)
#define VTSS_F_PHY_GPIO_CONTROL_2_COMA_MODE_OUTPUT_DATA    VTSS_PHY_BIT(12)

// Register 18G 
#define VTSS_PHY_MICRO_PAGE VTSS_PHY_PAGE_GPIO, 18

// Register 19G + Bit fields
#define      VTSS_PHY_MAC_MODE_AND_FAST_LINK VTSS_PHY_PAGE_GPIO, 19
#define VTSS_F_PHY_MAC_MODE_AND_FAST_LINK_MAC_IF_MODE_SELECT(value)  VTSS_PHY_ENCODE_BITFIELD(value, 14, 2)
#define VTSS_M_MAC_MODE_AND_FAST_LINK_MAC_IF_MODE_SELECT VTSS_PHY_ENCODE_BITMASK(14, 2)

// Register 20G + Bit fields
#define VTSS_PHY_I2C_MUX_CONTROL_1 VTSS_PHY_PAGE_GPIO, 20
#define VTSS_F_PHY_I2C_MUX_CONTROL_1_DEV_ADDR(value)  VTSS_PHY_ENCODE_BITFIELD(value, 9, 7)
#define VTSS_M_PHY_I2C_MUX_CONTROL_1_DEV_ADDR         VTSS_PHY_ENCODE_BITMASK(9, 7)
#define VTSS_F_PHY_I2C_MUX_CONTROL_1_SCL_CLK_FREQ(value)  VTSS_PHY_ENCODE_BITFIELD(value, 4, 2)
#define VTSS_M_PHY_I2C_MUX_CONTROL_1_SCL_CLK_FREQ         VTSS_PHY_ENCODE_BITMASK(4, 2)
#define VTSS_F_PHY_I2C_MUX_CONTROL_1_PORT_3_ENABLE        VTSS_PHY_BIT(3)
#define VTSS_F_PHY_I2C_MUX_CONTROL_1_PORT_2_ENABLE        VTSS_PHY_BIT(2)
#define VTSS_F_PHY_I2C_MUX_CONTROL_1_PORT_1_ENABLE        VTSS_PHY_BIT(1)
#define VTSS_F_PHY_I2C_MUX_CONTROL_1_PORT_0_ENABLE        VTSS_PHY_BIT(0)

// Register 21G + Bit fields
#define VTSS_PHY_I2C_MUX_CONTROL_2 VTSS_PHY_PAGE_GPIO, 21
#define VTSS_F_PHY_I2C_MUX_CONTROL_2_MUX_READY            VTSS_PHY_BIT(15)
#define VTSS_F_PHY_I2C_MUX_CONTROL_2_PHY_PORT_ADDR(value) VTSS_PHY_ENCODE_BITFIELD(value, 10, 2)
#define VTSS_M_PHY_I2C_MUX_CONTROL_2_PHY_PORT_ADDR        VTSS_PHY_ENCODE_BITMASK(10, 1)
#define VTSS_F_PHY_I2C_MUX_CONTROL_2_ENA_I2C_MAX_ACCESS   VTSS_PHY_BIT(9)
#define VTSS_F_PHY_I2C_MUX_CONTROL_2_RD                   VTSS_PHY_BIT(8)
#define VTSS_F_PHY_I2C_MUX_CONTROL_2_ADDR(value)          VTSS_PHY_ENCODE_BITFIELD(value, 0, 8)
#define VTSS_M_PHY_I2C_MUX_CONTROL_2_ADDR                 VTSS_PHY_ENCODE_BITMASK(7, 0)

// Register 22G + Bit fields
#define VTSS_PHY_I2C_MUX_DATA_READ_WRITE VTSS_PHY_PAGE_GPIO, 22
#define VTSS_M_PHY_I2C_MUX_DATA_READ_WRITE_READ_DATA          VTSS_PHY_ENCODE_BITMASK(8, 8)
#define VTSS_F_PHY_I2C_MUX_DATA_READ_WRITE_WRITE_DATA(value)  VTSS_PHY_ENCODE_BITFIELD(value, 0, 8)
#define VTSS_M_PHY_I2C_MUX_DATA_READ_WRITE_WRITE_DATA         VTSS_PHY_ENCODE_BITMASK(0, 8)

// Register 23G + Bit fields
#define VTSS_PHY_RECOVERED_CLOCK_0_CONTROL VTSS_PHY_PAGE_GPIO, 23

// Register 24G + Bit fields
#define VTSS_PHY_RECOVERED_CLOCK_1_CONTROL VTSS_PHY_PAGE_GPIO, 24


// Register 25G + Bit fields
#define VTSS_PHY_ENHANCED_LED_CONTROL VTSS_PHY_PAGE_GPIO, 25


// ** Register definitions -  TEST page**
#define VTSS_PHY_TEST_PAGE_12 VTSS_PHY_PAGE_TEST, 12
#define VTSS_PHY_TEST_PAGE_24 VTSS_PHY_PAGE_TEST, 24
#define VTSS_PHY_TEST_PAGE_25 VTSS_PHY_PAGE_TEST, 25


// ** Register definitions -  TR page**
#define VTSS_PHY_PAGE_TR_16 VTSS_PHY_PAGE_TR, 16
#define VTSS_PHY_PAGE_TR_17 VTSS_PHY_PAGE_TR, 17
#define VTSS_PHY_PAGE_TR_18 VTSS_PHY_PAGE_TR, 18
#endif /* _VTSS_PHY_H_ */

