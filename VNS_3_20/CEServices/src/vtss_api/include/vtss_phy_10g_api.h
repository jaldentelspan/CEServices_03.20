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

/**
 * \file
 * \brief 10G PHY API
 * \details This header file describes 10G PHY control functions
 */

#ifndef _VTSS_PHY_10G_API_H_
#define _VTSS_PHY_10G_API_H_

#include <vtss_options.h>
#include <vtss_types.h>
#include <vtss_misc_api.h>

#ifdef VTSS_CHIP_10G_PHY

/** \brief 10G Phy operating mode */
typedef struct {
    enum {
        VTSS_PHY_LAN_MODE,          /**< LAN mode */
        VTSS_PHY_WAN_MODE,          /**< WAN mode */
        VTSS_PHY_1G_MODE,           /**< 1G pass-through mode */
#if defined(VTSS_FEATURE_SYNCE_10G)
        VTSS_PHY_LAN_SYNCE_MODE, /**< LAN SyncE */
        VTSS_PHY_WAN_SYNCE_MODE, /**< WAN SyncE */
        VTSS_PHY_LAN_MIXED_SYNCE_MODE, /**< Channels are in different modes, channel being configured is in LAN */
        VTSS_PHY_WAN_MIXED_SYNCE_MODE, /**< Channels are in different modes, channel being configured is in WAN */
#endif /* VTSS_FEATURE_SYNCE_10G */
    } oper_mode;                    /**< Phy operational mode */

    enum {
        VTSS_PHY_XAUI_XFI,   /**< XAUI  <-> XFI - Interface mode. */
        VTSS_PHY_XGMII_XFI,  /**< XGMII <-> XFI - Interface mode. Only for VSC8486 */
    } interface;             /**< Interface mode. */

    enum {
        VTSS_WREFCLK_155_52, /**< WREFCLK = 155.52Mhz - WAN ref clock */
        VTSS_WREFCLK_622_08  /**< WREFCLK = 622.08Mhz - WAN ref clock */
    } wrefclk;               /**< WAN ref clock */

    BOOL  high_input_gain;   /**< Disable=0 (default), Enable=1. Should not be enabled unless needed */
    BOOL  xfi_pol_invert;    /**< Selects polarity ot the TX XFI data. 1:Invert 0:Normal */
    BOOL  xaui_lane_flip;     /**< Swaps lane 0 <--> 3 and 1 <--> 2 for both RX and TX */

    enum {
        VTSS_CHANNEL_AUTO,   /**< Automatically detects the channel id based on the phy order.  
                                The phys be setup in the consecutive order, from the lowest MDIO to highest MDIO address */
        VTSS_CHANNEL_0,      /**< Channel id is hardcoded to 0  */
        VTSS_CHANNEL_1,      /**< Channel id is hardcoded to 1  */
        VTSS_CHANNEL_2,      /**< Channel id is hardcoded to 2  */
        VTSS_CHANNEL_3,      /**< Channel id is hardcoded to 3  */
    } channel_id;            /**< Channel id of this instance of the Phy  */

#if defined(VTSS_FEATURE_SYNCE_10G)
    BOOL hl_clk_synth;       /**< 0: Free running clock 
                                  1: Hitless clock */
    enum {
      VTSS_RECVRD_RXCLKOUT,  /**< RXCLKOUT is used for recovered clock */
      VTSS_RECVRD_TXCLKOUT,  /**< TXCLKOUT is used for recovered clock */
    } rcvrd_clk;              /**< RXCLKOUT/TXCLKOUT used as recovered clock */
    
    enum {
      VTSS_RECVRDCLK_CDR_DIV_64, /**< recovered clock is /64 */
      VTSS_RECVRDCLK_CDR_DIV_66, /**< recovered clock is /66 */
    } rcvrd_clk_div;              /**< recovered clock's divisor */
    
    enum {
     VTSS_SREFCLK_DIV_64,    /**< SREFCLK/64  */
     VTSS_SREFCLK_DIV_66,    /**< SREFCLK/64  */
     VTSS_SREFCLK_DIV_16,    /**< SREFCLK/16 */ 
    } sref_clk_div;           /**< SRERCLK divisor */

    enum {
     VTSS_WREFCLK_NONE,      /**< NA */
     VTSS_WREFCLK_DIV_16,    /**< WREFCLK/16 */
    } wref_clk_div;           /**< WREFCLK divisor */
#endif /* VTSS_FEATURE_SYNCE_10G */

#if defined(VTSS_FEATURE_EDC_FW_LOAD)
    enum {
     VTSS_EDC_FW_LOAD_MDIO,    /**< Load EDC FW through MDIO to iCPU */
     VTSS_EDC_FW_LOAD_NOTHING, /**< Do not load FW to iCPU */
    } edc_fw_load;             /**< EDC Firmware load */
#endif /* VTSS_FEATURE_EDC_FW_LOAD */

} vtss_phy_10g_mode_t; 

/**
 * \brief Get the Phy operating mode.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param mode [IN]     Mode configuration.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/

vtss_rc vtss_phy_10g_mode_get (const vtss_inst_t inst, 
                               const vtss_port_no_t port_no, 
                               vtss_phy_10g_mode_t *const mode);


/**
 * \brief Identify, Reset and set the operating mode of the PHY.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param mode [IN]     Mode configuration.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_mode_set (const vtss_inst_t inst, 
                               const vtss_port_no_t port_no, 
                               const vtss_phy_10g_mode_t *const mode);


#if defined(VTSS_FEATURE_SYNCE_10G) 
/**
 * \brief Get the status of recovered clock from PHY.
 *
 * \param inst [IN]          Target instance reference.
 * \param port_no [IN]       Port number.
 * \param synce_clkout [IN]  Recovered clock configuration.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_synce_clkout_get (const vtss_inst_t inst,
                                       const vtss_port_no_t port_no,
                                       BOOL *const synce_clkout);

/**
 * \brief Enable or Disable the recovered clock from PHY.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param synce_clkout [IN]  Recovered clock to be enabled or disabled.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_synce_clkout_set (const vtss_inst_t inst,
                                       const vtss_port_no_t port_no,
                                       const BOOL synce_clkout);


/**
 * \brief Get the status of RXCLKOUT/TXCLKOUT from PHY.
 *
 * \param inst [IN]          Target instance reference.
 * \param port_no [IN]       Port number.
 * \param xfp_clkout [IN]    XFP clock configuration.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_xfp_clkout_get (const vtss_inst_t inst,
                                     const vtss_port_no_t port_no,
                                     BOOL *const xfp_clkout);

/**
 * \brief Enable or Disable the RXCLKOUT/TXCLKOUT from PHY.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param xfp_clkout [IN]  XFP clock to be enabled or disabled.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_xfp_clkout_set (const vtss_inst_t inst,
                                     const vtss_port_no_t port_no,
                                     const BOOL xfp_clkout);


#endif /* VTSS_FEATURE_SYNCE_10G */


/** \brief 10G Phy link and fault status */
typedef struct {
    BOOL           rx_link;    /**< The rx link status  */
    vtss_event_t   link_down;  /**< Link down event status. Clear on read  */
    BOOL           rx_fault;   /**< Rx fault event status.  Clear on read */
    BOOL           tx_fault;   /**< Tx fault event status.  Clear on read */
} vtss_sublayer_status_t; 

/** \brief 10G Phy link and fault status for all sublayers */
typedef struct {
    vtss_sublayer_status_t	pma; /**< Status for PMA sublayer */
    vtss_sublayer_status_t	wis; /**< Status for WIS sublayer */
    vtss_sublayer_status_t	pcs; /**< Status for PCS sublayer */
    vtss_sublayer_status_t	xs;  /**< Status for XS  sublayer */
} vtss_phy_10g_status_t; 

/**
 * \brief Get the link and fault status of the PHY sublayers.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param status [IN]   Status of all sublayers
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_status_get (const vtss_inst_t inst, 
                    		 const vtss_port_no_t port_no, 
                    		 vtss_phy_10g_status_t *const status);



/**
 * \brief Reset the phy.  Phy is reset to default values.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_reset(const vtss_inst_t       	inst,
                           const vtss_port_no_t    	port_no);


/** \brief 10G Phy system and network loopbacks */
typedef struct {
    enum {
        VTSS_LB_NONE, 	             /**< No looback   */
        VTSS_LB_SYSTEM_XS_SHALLOW,   /**< System Loopback B,  XAUI -> XS -> XAUI   4x800E.13    */
        VTSS_LB_SYSTEM_XS_DEEP,      /**< System Loopback C,  XAUI -> XS -> XAUI   4x800F.2     */
        VTSS_LB_SYSTEM_PCS_SHALLOW,  /**< System Loopback E,  XAUI -> PCS FIFO -> XAUI 3x8005.2 */
        VTSS_LB_SYSTEM_PCS_DEEP,     /**< System Loopback G,  XAUI -> PCS -> XAUI  3x0000.14    */
        VTSS_LB_SYSTEM_PMA,	     /**< System Loopback J,  XAUI -> PMA -> XAUI  1x0000.0     */        
        VTSS_LB_NETWORK_XS_SHALLOW,  /**< Network Loopback D,  XFI -> XS -> XFI   4x800F.1       */
        VTSS_LB_NETWORK_XS_DEEP,     /**< Network Loopback A,  XFI -> XS -> XFI   4x0000.1  4x800E.13=0  */
        VTSS_LB_NETWORK_PCS,	     /**< Network Loopback F,  XFI -> PCS -> XFI  3x8005.3       */
        VTSS_LB_NETWORK_WIS,	     /**< Network Loopback H,  XFI -> WIS -> XFI  2xE600.0       */
        VTSS_LB_NETWORK_PMA	     /**< Network Loopback K,  XFI -> PMA -> XFI  1x8000.8       */
    } lb_type;                       /**< Looback types                                          */  
    BOOL enable;                     /**< Enable/Disable loopback given in \<lb_type\>             */
} vtss_phy_10g_loopback_t; 

/**
 * \brief Enable/Disable a phy network or system loopback.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param loopback [IN] Loopback settings.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_loopback_set(const vtss_inst_t       	inst,
                                  const vtss_port_no_t   	port_no,
                                  const vtss_phy_10g_loopback_t	*const loopback);

/**
 * \brief Get loopback settings.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param loopback [OUT] Current loopback settings.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_loopback_get(const vtss_inst_t       	inst,
                                  const vtss_port_no_t   	port_no,
                                  vtss_phy_10g_loopback_t	*const loopback);



/** \brief 10G Phy PCS counters */
typedef struct {
    BOOL                        block_lock_latched; /**< Latched block status      */
    BOOL                        high_ber_latched;   /**< Lathced high ber status   */
    u8 				ber_cnt; 	    /**< BER counter. Saturating, clear on read */
    u8 				err_blk_cnt; 	    /**< ERROR block counter. Saturating, clear on read */
} vtss_phy_pcs_cnt_t; 

/** \brief 10G Phy Sublayer counters */
typedef struct {
//  vtss_phy_pma_cnt_t		pma;
//  vtss_phy_wis_cnt_t		wis;
    vtss_phy_pcs_cnt_t		pcs;  /**< PCS counters */
//  vtss_phy_xs_cnt_t		xs;
} vtss_phy_10g_cnt_t; 

/**
 * \brief Get counters.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param cnt [OUT] Phy counters
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_cnt_get(	const vtss_inst_t       inst,
				const vtss_port_no_t    port_no,
				vtss_phy_10g_cnt_t   	*const cnt);

/** \brief 10G Phy power setting */
typedef enum {    
    VTSS_PHY_10G_POWER_ENABLE,  /**< Enable Phy power for all sublayers */
    VTSS_PHY_10G_POWER_DISABLE  /**< Disable Phy power for all sublayers*/
} vtss_phy_10g_power_t; 


/**
 * \brief Get the power settings 
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param power [OUT] power settings
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_power_get(const vtss_inst_t      inst,
                               const vtss_port_no_t   port_no,
                               vtss_phy_10g_power_t  *const power);
/**
 * \brief Set the power settings.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param power [IN] power settings
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_power_set(const vtss_inst_t        inst,
                               const vtss_port_no_t     port_no,
                               const vtss_phy_10g_power_t  *const power);

/**
 * \brief Gives a True/False value if the Phy is supported by the API\n
 *  Only Vitesse phys are supported.  vtss_phy_10g_mode_set() must be applied.
 *
 * \param inst [IN] Target instance reference. 
 * \param port_no [IN]  Port number.
 *
 * \return
 *   TRUE  : Phy is supported.\n
 *   FALSE : Phy is not supported.
 **/
BOOL vtss_phy_10G_is_valid(const vtss_inst_t        inst,
                           const vtss_port_no_t port_no);

/** \brief 10G Phy power setting */
typedef enum {    
    VTSS_PHY_10G_PMA_TO_FROM_XAUI_NORMAL,         /**< PMA_0/1 to XAUI_0/1. 8487: XAUI 0 to PMA 0 */
    VTSS_PHY_10G_PMA_TO_FROM_XAUI_CROSSED,        /**< PMA_0/1 to XAUI_1/0. 8487: XAUI 1 to PMA 0 */
    VTSS_PHY_10G_PMA_0_TO_FROM_XAUI_0_TO_XAUI_1,  /**< PMA 0 to/from XAUI 0 and to XAUI 1 */
    VTSS_PHY_10G_PMA_0_TO_FROM_XAUI_1_TO_XAUI_0,  /**< PMA 0 to/from XAUI 1 and to XAUI 0 */ 
    VTSS_PHY_10G_PMA_1_TO_FROM_XAUI_0_TO_XAUI_1,  /**< PMA 1 to/from XAUI 0 and to XAUI 1.      VSC8487:N/A */
    VTSS_PHY_10G_PMA_1_TO_FROM_XAUI_1_TO_XAUI_0,  /**< PMA 1 to/from XAUI 1 and to XAUI 0.      VSC8487:N/A */
} vtss_phy_10g_failover_mode_t; 

/**
 * \brief Set the failover mode 
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number. (Use any port within the phy).
 * \param mode [IN]     Failover mode
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_failover_set(const vtss_inst_t      inst,
                                  const vtss_port_no_t   port_no,
                                  vtss_phy_10g_failover_mode_t  *const mode);
/**
 * \brief Get the failover mode 
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param mode [OUT] failover mode 
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_failover_get(const vtss_inst_t      inst,
                                  const vtss_port_no_t   port_no,
                                  vtss_phy_10g_failover_mode_t  *const mode);

/** \brief 10g PHY type */
typedef enum {
    VTSS_PHY_TYPE_10G_NONE = 0,/* Unknown  */
    VTSS_PHY_TYPE_8484 = 8484, /* VSC8484  */
    VTSS_PHY_TYPE_8486 = 8486, /* VSC8486  */
    VTSS_PHY_TYPE_8487 = 8487, /* VSC8487  */
    VTSS_PHY_TYPE_8488 = 8488, /* VSC8488  */
} vtss_phy_10g_type_t; 

/** \brief 10G PHY family */
typedef enum {
    VTSS_PHY_FAMILY_10G_NONE,        /* Unknown */
    VTSS_PHY_FAMILY_XAUI_XGMII_XFI,  /* VSC8486 */
    VTSS_PHY_FAMILY_XAUI_XFI,        /* VSC8484, VSC8487, VSC8488 */
} vtss_phy_10g_family_t;

/** \brief 10G Phy part number and revision */
typedef struct
{
    u16                   part_number;    /**< Part number (Hex)  */
    u16                   revision;       /**< Chip revision      */
    u16                   channel_id;     /**< Channel id         */
    vtss_phy_10g_family_t family;         /**< Phy Family         */
    vtss_phy_10g_type_t   type;           /**< Phy id (Decimal)   */
    vtss_port_no_t        phy_api_base_no;/**< First API no within this phy (in case of multiple channels) */
} vtss_phy_10g_id_t; 

/**
 * \brief Read the Phy Id
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param phy_id [OUT] The part number and revision.
 *
 * \return
 *   VTSS_RC_OK on success.\n
 *   VTSS_RC_ERROR on error.
 **/
vtss_rc vtss_phy_10g_id_get(const vtss_inst_t   inst, 
                            const vtss_port_no_t  port_no, 
                            vtss_phy_10g_id_t *const phy_id);

/* - GPIOs ------------------------------------------------------- */
/**
 * \brief GPIO configured mode
 **/
#ifndef VTSS_GPIOS
typedef u32 vtss_gpio_no_t; /**< GPIO type for 1G ports*/
#endif

/**
 * \brief GPIO configured mode
 **/
typedef struct
{
    enum
    {
        VTSS_10G_PHY_GPIO_NOT_INITIALIZED,   /**< This GPIO pin has has been initialized by a call to API from application. aregisters contain power-up default value */
        VTSS_10G_PHY_GPIO_OUT,               /**< Output enabled */
        VTSS_10G_PHY_GPIO_IN,                /**< Input enabled */
        VTSS_10G_PHY_GPIO_WIS_INT,           /**< Output WIS interrupt channel 0 or 1 (depending on port_no) enabled */
    /*    VTSS_10G_PHY_GPIO_INT_FALLING,*/   /**< Input interrupt generated on falling edge */
    /*    VTSS_10G_PHY_GPIO_INT_RAISING,*/   /**< Input interrupt generated on raising edge */
    /*    VTSS_10G_PHY_GPIO_INT_CHANGED,*/   /**< Input interrupt generated on raising and falling edge */
        VTSS_10G_PHY_GPIO_1588_LOAD_SAVE,    /**< Input 1588 load/save function */
        VTSS_10G_PHY_GPIO_1588_1PPS_0,       /**< Output 1588 1PPS channel 0 function */
        VTSS_10G_PHY_GPIO_1588_1PPS_1,       /**< Output 1588 1PPS channel 1 function */
        VTSS_10G_PHY_GPIO_PCS_RX_FAULT,      /**< PCS_RX_FAULT (from channel 0 or 1) is transmitted on GPIO */
        /* More to come.. */
    } mode;                                  /**< Mode of this GPIO pin */
    vtss_port_no_t port;                     /**< In case of VTSS_10G_PHY_GPIO_WIS_INT mode, this is the interrupt port number that is related to this GPIO
                                                  In case of VTSS_10G_PHY_GPIO_PCS_RX_FAULT  mode, this is the PCS status port number that is related to this GPIO */
} vtss_gpio_10g_gpio_mode_t;

typedef u16 vtss_gpio_10g_no_t; /**< GPIO type for 10G ports*/

#define VTSS_10G_PHY_GPIO_MAX   12  /**< Max value of gpio_no parameter */

/**
 * \brief Set GPIO mode. There is only one set og GPIO per PHY chip - not per port.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number that identify the PHY chip.
 * \param gpio_no [IN]  GPIO pin number < VTSS_10G_PHY_GPIO_MAX.
 * \param mode [IN]     GPIO mode.
 *
 * \return Return code.
 **/
vtss_rc vtss_phy_10g_gpio_mode_set(const vtss_inst_t                inst,
                                   const vtss_port_no_t             port_no,
                                   const vtss_gpio_10g_no_t         gpio_no,
                                   const vtss_gpio_10g_gpio_mode_t  *const mode);
/**
 * \brief Get GPIO mode.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number that identify the PHY chip.
 * \param gpio_no [IN]  GPIO pin number.
 * \param mode [OUT]    GPIO mode.
 *
 * \return Return code.
 **/

vtss_rc vtss_phy_10g_gpio_mode_get(const vtss_inst_t          inst,
                                   const vtss_port_no_t       port_no,
                                   const vtss_gpio_10g_no_t   gpio_no,
                                   vtss_gpio_10g_gpio_mode_t  *const mode);

/**
 * \brief Read from GPIO input pin.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param gpio_no [IN]  GPIO pin number.
 * \param value [OUT]   TRUE if pin is high, FALSE if it is low.
 *
 * \return Return code.
 **/
vtss_rc vtss_phy_10g_gpio_read(const vtss_inst_t     inst,
                               const vtss_port_no_t  port_no,
                               const vtss_gpio_10g_no_t  gpio_no,
                               BOOL                  *const value);

/**
 * \brief Write to GPIO output pin.
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number.
 * \param gpio_no [IN]  GPIO pin number.
 * \param value [IN]    TRUE to set pin high, FALSE to set pin low.
 *
 * \return Return code.
 **/
vtss_rc vtss_phy_10g_gpio_write(const vtss_inst_t     inst,
                                const vtss_port_no_t  port_no,
                                const vtss_gpio_10g_no_t  gpio_no,
                                const BOOL            value);

/* - Events ------------------------------------------------------- */

/** \brief Event source identification mask values */
#define    VTSS_PHY_10G_LINK_LOS_EV            0x00000001 /**< PHY Link Los interrupt - only on 8486 */
#define    VTSS_PHY_10G_RX_LOL_EV              0x00000002 /**< PHY RXLOL interrupt - only on 8488 */
#define    VTSS_PHY_10G_TX_LOL_EV              0x00000004 /**< PHY TXLOL interrupt - only on 8488 */
#define    VTSS_PHY_10G_LOPC_EV                0x00000008 /**< PHY LOPC interrupt - only on 8488 */
#define    VTSS_PHY_10G_HIGH_BER_EV            0x00000010 /**< PHY HIGH_BER interrupt - only on 8488 */
#define    VTSS_PHY_10G_MODULE_STAT_EV         0x00000020 /**< PHY MODULE_STAT interrupt - only on 8488 */
#define    VTSS_PHY_10G_PCS_RECEIVE_FAULT_EV   0x00000040 /**< PHY PCS_RECEIVE_FAULT interrupt - only on 8488 */
#ifdef VTSS_FEATURE_WIS
#define    VTSS_PHY_EWIS_SEF_EV                0x00000080 /**< SEF has changed state - only for 8488 */
#define    VTSS_PHY_EWIS_FPLM_EV               0x00000100 /**< far-end (PLM-P) / (LCDP) - only for 8488 */
#define    VTSS_PHY_EWIS_FAIS_EV               0x00000200 /**< far-end (AIS-P) / (LOP) - only for 8488 */
#define    VTSS_PHY_EWIS_LOF_EV                0x00000400 /**< Loss of Frame (LOF) - only for 8488 */
#define    VTSS_PHY_EWIS_RDIL_EV               0x00000800 /**< Line Remote Defect Indication (RDI-L) - only for 8488 */
#define    VTSS_PHY_EWIS_AISL_EV               0x00001000 /**< Line Alarm Indication Signal (AIS-L) - only for 8488 */
#define    VTSS_PHY_EWIS_LCDP_EV               0x00002000 /**< Loss of Code-group Delineation (LCD-P) - only for 8488 */
#define    VTSS_PHY_EWIS_PLMP_EV               0x00004000 /**< Path Label Mismatch (PLMP) - only for 8488 */
#define    VTSS_PHY_EWIS_AISP_EV               0x00008000 /**< Path Alarm Indication Signal (AIS-P) - only for 8488 */
#define    VTSS_PHY_EWIS_LOPP_EV               0x00010000 /**< Path Loss of Pointer (LOP-P) - only for 8488 */
#define    VTSS_PHY_EWIS_UNEQP_EV              0x00020000 /**< Unequiped Path (UNEQ-P) - only for 8488 */
#define    VTSS_PHY_EWIS_FEUNEQP_EV            0x00040000 /**< Far-end Unequiped Path (UNEQ-P) - only for 8488 */
#define    VTSS_PHY_EWIS_FERDIP_EV             0x00080000 /**< Far-end Path Remote Defect Identifier (RDI-P) - only for 8488 */
#define    VTSS_PHY_EWIS_REIL_EV               0x00100000 /**< Line Remote Error Indication (REI-L) - only for 8488 */
#define    VTSS_PHY_EWIS_REIP_EV               0x00200000 /**< Path Remote Error Indication (REI-P) - only for 8488 */
#define    VTSS_PHY_EWIS_B1_NZ_EV              0x00400000 /**< PMTICK B1 BIP (B1_ERR_CNT) not zero - only for 8488 */
#define    VTSS_PHY_EWIS_B2_NZ_EV              0x00800000 /**< PMTICK B2 BIP (B1_ERR_CNT) not zero - only for 8488 */
#define    VTSS_PHY_EWIS_B3_NZ_EV              0x01000000 /**< PMTICK B3 BIP (B1_ERR_CNT) not zero - only for 8488 */
#define    VTSS_PHY_EWIS_REIL_NZ_EV            0x02000000 /**< PMTICK REI-L (REIL_ERR_CNT) not zero - only for 8488 */
#define    VTSS_PHY_EWIS_REIP_NZ_EV            0x04000000 /**< PMTICK REI-P (REIP_ERR_CNT) not zero - only for 8488 */
#define    VTSS_PHY_EWIS_B1_THRESH_EV          0x08000000 /**< B1_THRESH_ERR - only for 8488 */
#define    VTSS_PHY_EWIS_B2_THRESH_EV          0x10000000 /**< B2_THRESH_ERR - only for 8488 */
#define    VTSS_PHY_EWIS_B3_THRESH_EV          0x20000000 /**< B3_THRESH_ERR - only for 8488 */
#define    VTSS_PHY_EWIS_REIL_THRESH_EV        0x40000000 /**< REIL_THRESH_ERR - only for 8488 */
#define    VTSS_PHY_EWIS_REIP_THRESH_EV        0x80000000 /**< REIp_THRESH_ERR - only for 8488 */
#endif /* VTSS_FEATURE_WIS */
typedef u32 vtss_phy_10g_event_t;   /**< The type definition to contain the above defined evant mask */

/**
 * \brief Enabling / Disabling of events
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number 
 * \param ev_mask [IN]  Mask containing events that are enabled/disabled 
 * \param enable [IN]   Enable/disable of event
 *
 * \return Return code.
 **/

vtss_rc vtss_phy_10g_event_enable_set(const vtss_inst_t           inst,
                                      const vtss_port_no_t        port_no,
                                      const vtss_phy_10g_event_t  ev_mask,
                                      const BOOL                  enable);

/**
 * \brief Get Enabling of events
 *
 * \param inst [IN]      Target instance reference.
 * \param port_no [IN]   Port number 
 * \param ev_mask [OUT]  Mask containing events that are enabled 
 *
 * \return Return code.
 **/

vtss_rc vtss_phy_10g_event_enable_get(const vtss_inst_t      inst,
                                      const vtss_port_no_t   port_no,
                                      vtss_phy_10g_event_t   *const ev_mask);

/**
 * \brief Polling for active events
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number 
 * \param ev_mask [OUT] Mask containing events that are active
 *
 * \return Return code.
 **/
vtss_rc vtss_phy_10g_event_poll(const vtss_inst_t     inst,
                                const vtss_port_no_t  port_no,
                                vtss_phy_10g_event_t  *const ev_mask);


/**
 * \brief Function is called once a second
 *
 * \param inst [IN]     Target instance reference.
 * \return Return code.
 **/
vtss_rc vtss_phy_10g_poll_1sec(const vtss_inst_t  inst);

/** \brief Firmware status */
typedef struct {
    u16             edc_fw_rev;      /**< FW revision */
    BOOL            edc_fw_chksum;   /**< FW chksum.    Fail=0, Pass=1*/
    BOOL            icpu_activity;   /**< iCPU activity.  Not Running=0, Running=1   */
    BOOL            edc_fw_api_load; /**< EDC FW is loaded through API No=0, Yes=1  */
} vtss_phy_10g_fw_status_t; 

/**
 * \brief Internal microprocessor status
 *
 * \param inst [IN]     Target instance reference.
 * \param port_no [IN]  Port number 
 * \param status [OUT]  Status of the EDC FW running on the internal CPU
 *
 * \return Return code.
 **/
vtss_rc vtss_phy_10g_edc_fw_status_get(const vtss_inst_t     inst,
                                       const vtss_port_no_t  port_no,
                                       vtss_phy_10g_fw_status_t  *const status);

#endif /* VTSS_CHIP_10G_PHY */
#endif /* _VTSS_PHY_10G_API_H_ */
