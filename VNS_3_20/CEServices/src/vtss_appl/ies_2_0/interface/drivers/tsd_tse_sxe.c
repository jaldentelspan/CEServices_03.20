/*
 * tsd_tse_sxe.c
 *
 *  Created on: May 15, 2018
 *      Author: eric
 */

#include <stdio.h>
#include "mmdrv_printf.h"
#include "tsd_register_common.h"
#include "tsd_eth_tse_regs.h"
#include "tsd_tse_sxe.h"

/*
 * The following is based on the mode enumeration in the .h file
 * currently:
 *     TSE_MAC_MODE_FPGA_REASM_RX
 *     TSE_MAC_MODE_FPGA_PUB_TX
 *     TSE_MAC_MODE_FPGA_PUB_TX_STORE_AND_FORWARD
 *     TSE_HDLC_CH7_V1
 */
const uint32_t TSE_CONFIG_REG_FOR_MAC_MODE[] =
{
        /* for TSE_MAC_MODE_FPGA_REASM_RX */
        ( 0 /*mmac_cc_SW_RESET_mask*/ |
        mmac_cc_TX_ENA_mask |
        mmac_cc_RX_ENA_mask |
        mmac_cc_ETH_SPEED_mask |
        mmac_cc_PROMIS_EN_mask |
        mmac_cc_CRC_FWD_mask |
        0 /*mmac_cc_PAD_EN_mask*/ |
        mmac_cc_NO_LENGTH_CHECK_mask |
        mmac_cc_RX_ERR_DISCARD_mask ),

        /* for TSE_MAC_MODE_FPGA_PUB_TX */
        ( 0 /*mmac_cc_SW_RESET_mask*/ |
        mmac_cc_TX_ENA_mask |
        mmac_cc_RX_ENA_mask |
        mmac_cc_ETH_SPEED_mask |
        mmac_cc_PROMIS_EN_mask |
        mmac_cc_CRC_FWD_mask |
        0 /*mmac_cc_PAD_EN_mask*/ |
        mmac_cc_NO_LENGTH_CHECK_mask |
        mmac_cc_RX_ERR_DISCARD_mask ),

        /* for TSE_MAC_MODE_FPGA_PUB_TX_STORE_AND_FORWARD */
        ( 0 /*mmac_cc_SW_RESET_mask*/ |
        mmac_cc_TX_ENA_mask |
        mmac_cc_RX_ENA_mask |
        mmac_cc_ETH_SPEED_mask |
        mmac_cc_PROMIS_EN_mask |
        mmac_cc_CRC_FWD_mask |
        0 /*mmac_cc_PAD_EN_mask*/ |
        mmac_cc_NO_LENGTH_CHECK_mask |
        mmac_cc_RX_ERR_DISCARD_mask ),

        /* for TSE_HDLC_CH7_V1 */
        ( 0 /*mmac_cc_SW_RESET_mask*/ |
        mmac_cc_TX_ENA_mask |
        mmac_cc_RX_ENA_mask |
        mmac_cc_ETH_SPEED_mask |
        mmac_cc_PROMIS_EN_mask |
        mmac_cc_CRC_FWD_mask |
        0 /*mmac_cc_PAD_EN_mask*/ |
        mmac_cc_NO_LENGTH_CHECK_mask |
        mmac_cc_RX_ERR_DISCARD_mask )
};

/*
 * The following is based on the mode enumeration in the .h file
 * currently:
 *     TSE_MAC_MODE_FPGA_REASM_RX
 *     TSE_MAC_MODE_FPGA_PUB_TX
 *     TSE_MAC_MODE_FPGA_PUB_TX_STORE_AND_FORWARD
 *     TSE_HDLC_CH7_V1
 */
const uint32_t TSE_RX_CMD_STAT_REG_FOR_MAC_MODE[] = {
		0 /*ALTERA_TSEMAC_RX_CMD_STAT_RXSHIFT16_MSK*/,
		0 /*ALTERA_TSEMAC_RX_CMD_STAT_RXSHIFT16_MSK*/,
		0 /*ALTERA_TSEMAC_RX_CMD_STAT_RXSHIFT16_MSK*/,
		0 /*ALTERA_TSEMAC_RX_CMD_STAT_RXSHIFT16_MSK*/
};

/*
 * The following is based on the mode enumeration in the .h file
 * currently:
 *     TSE_MAC_MODE_FPGA_REASM_RX
 *     TSE_MAC_MODE_FPGA_PUB_TX
 *     TSE_MAC_MODE_FPGA_PUB_TX_STORE_AND_FORWARD
 *     TSE_HDLC_CH7_V1
 */
const uint32_t TSE_TX_CMD_STAT_REG_FOR_MAC_MODE[] = {
		0 /*ALTERA_TSEMAC_TX_CMD_STAT_TXSHIFT16_MSK*/,
		ALTERA_TSEMAC_TX_CMD_STAT_TXSHIFT16_MSK,
		ALTERA_TSEMAC_TX_CMD_STAT_TXSHIFT16_MSK,
		0 /*ALTERA_TSEMAC_TX_CMD_STAT_TXSHIFT16_MSK*/
};

uint32_t SRC_MAC_H(uint8_t srcMAC[6])
{
    return (((uint32_t)srcMAC[0]) << 8) | ((uint32_t)srcMAC[1]);
}

uint32_t SRC_MAC_L(uint8_t srcMAC[6])
{
    return (((uint32_t)srcMAC[2]) << 24) | (((uint32_t)srcMAC[3]) << 16) | (((uint32_t)srcMAC[4]) << 8) | ((uint32_t)srcMAC[5]);
}

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_tse_sxe_debug_print(uint32_t base)
{
    double version;
    mmdrv_printf("TSD TSE SXE Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_tse_sxe_driver_version(&version))
    {
        mmdrv_printf(" - Driver Version:  %f\n", version);
    }
    else
    {
        mmdrv_printf(" - Driver Version:  FAILED TO READ\n");
    }

    mmdrv_printf(" - REGISTERS:\n");
    mmdrv_printf("    => CMD_CONFIG        = 0x%08X\n", IORD_ALTERA_TSEMAC_CMD_CONFIG(base));
    mmdrv_printf("    => PCS_CONTROL       = 0x%08X\n", IORD_ALTERA_TSEPCS_CONTROL(base));
    mmdrv_printf("    => PCS_STATUS        = 0x%08X\n", IORD_ALTERA_TSEPCS_STATUS(base));
    mmdrv_printf("    => MAC_0             = 0x%08X\n", IORD_ALTERA_TSEMAC_MAC_0(base));
    mmdrv_printf("    => MAC_1             = 0x%08X\n", IORD_ALTERA_TSEMAC_MAC_1(base));
    mmdrv_printf("    => TX_CMD_STAT       = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_CMD_STAT(base));
    mmdrv_printf("    => RX_CMD_STAT       = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_CMD_STAT(base));
    mmdrv_printf("    => RX_ALMOST_EMPTY   = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_ALMOST_EMPTY(base));
    mmdrv_printf("    => RX_ALMOST_FULL    = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_ALMOST_FULL(base));
    mmdrv_printf("    => TX_ALMOST_EMPTY   = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_ALMOST_EMPTY(base));
    mmdrv_printf("    => TX_ALMOST_FULL    = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_ALMOST_FULL(base));
    mmdrv_printf("    => TX_SECTION_EMPTY  = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_SECTION_EMPTY(base));
    mmdrv_printf("    => TX_SECTION_FULL   = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_SECTION_FULL(base));
    mmdrv_printf("    => RX_SECTION_EMPTY  = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_SECTION_EMPTY(base));
    mmdrv_printf("    => RX_SECTION_FULL   = 0x%08X\n", IORD_ALTERA_TSEMAC_RX_SECTION_FULL(base));
    mmdrv_printf("    => FRM_LENGTH        = 0x%08X\n", IORD_ALTERA_TSEMAC_FRM_LENGTH(base));
    mmdrv_printf("    => TX_IPG_LENGTH     = 0x%08X\n", IORD_ALTERA_TSEMAC_TX_IPG_LENGTH(base));
    mmdrv_printf("    => TX_PAUSE_QUANT    = 0x%08X\n", IORD_ALTERA_TSEMAC_PAUSE_QUANT(base));
    mmdrv_printf("    => IF RX ERRORS      = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_IN_ERRORS(base));
    mmdrv_printf("    => IF RX FCS ERRORS  = 0x%08X\n", IORD_ALTERA_TSEMAC_A_FRAME_CHECK_SEQ_ERRS(base));
    mmdrv_printf("    => IF RX JABBBERS    = 0x%08X\n", IORD_ALTERA_TSEMAC_ETHER_STATS_JABBERS(base));
    mmdrv_printf("    => IF RX FRAGMENTS   = 0x%08X\n", IORD_ALTERA_TSEMAC_ETHER_STATS_FRAGMENTS(base));
    mmdrv_printf("    => IF TX ERRORS      = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_ERRORS(base));
    mmdrv_printf("    => IF TX DISCARDS    = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_DISCARDS(base));
    mmdrv_printf("    => IF TX UCAST PKTS  = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_UCAST_PKTS(base));
    mmdrv_printf("    => IF TX MCAST PKTS  = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_MULTICAST_PKTS(base));
    mmdrv_printf("    => IF TX BCAST PKTS  = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_OUT_BROADCAST_PKTS(base));
    mmdrv_printf("    => UNDERSIZE PKTS    = 0x%08X\n", IORD_ALTERA_TSEMAC_ETHER_STATS_UNDERSIZE_PKTS(base));
    mmdrv_printf("    => OVERSIZE PKTS     = 0x%08X\n", IORD_ALTERA_TSEMAC_ETHER_STATS_OVERSIZE_PKTS(base));
    mmdrv_printf("    => RX UNICAST PKTS   = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_IN_UCAST_PKTS(base));
    mmdrv_printf("    => RX MULTICAST PKTS = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_IN_MULTICAST_PKTS(base));
    mmdrv_printf("    => RX BROADCAST PKTS = 0x%08X\n", IORD_ALTERA_TSEMAC_IF_IN_BROADCAST_PKTS(base));
    mmdrv_printf("    => RX GOOD FRAMES    = 0x%08X\n", IORD_ALTERA_TSEMAC_A_FRAMES_RX_OK(base));
    mmdrv_printf("    => TX GOOD FRAMES    = 0x%08X\n", IORD_ALTERA_TSEMAC_A_FRAMES_TX_OK(base));

}

/*
 * Gets the driver version (returned in version)
 * returns 0 on success.
 */
int tsd_tse_sxe_driver_version(double *version)
{
    *version = (double)TSD_TSE_SXE_DRIVER_VERSION;
    return 0;
}

/*
 * Initializes the TSE for the FPGA.
 * returns 0 on success.
 */
int tsd_tse_sxe_init(uint32_t base, ePCS_IF_SPEED speed, eTSE_MAC_MODE mode, uint32_t txFifoDepth, uint32_t rxFifoDepth, uint8_t srcMAC[6])
{
    uint32_t tmp, x;
    // first make sure we can read and write
    IOWR_ALTERA_TSEMAC_SCRATCH(base, 0xAAAA);
    tmp = IORD_ALTERA_TSEMAC_SCRATCH(base);
    if(tmp != 0xAAAA)
    {
        return 1;
    }

    // reset and disable tx and rx in command config register
    IOWR_ALTERA_TSEMAC_CMD_CONFIG(base,
            mmac_cc_SW_RESET_mask |
            0 /*mmac_cc_TX_ENA_mask */ |
            0 /*mmac_cc_RX_ENA_mask*/);

    // wait for reset to clear
    x = 0;
    while(IORD_ALTERA_TSEMAC_CMD_CONFIG(base) &
            ALTERA_TSEMAC_CMD_SW_RESET_MSK) {
        if( x++ > 10000 ) {
            break;
        }
    }
    if(x >= 10000) {
        mmdrv_printf("TSEMAC SW reset bit never cleared!\n");
        return 2;
    }

    // verify that RX and TX bits are cleared
    tmp = IORD_ALTERA_TSEMAC_CMD_CONFIG(base);
    if( (tmp & (mmac_cc_TX_ENA_mask | mmac_cc_RX_ENA_mask)) != 0 ) {
        mmdrv_printf("WARN: RX/TX not disabled after reset... missing PHY clock? CMD_CONFIG=0x%08x\n", (unsigned)tmp);
    }

    /* Set the MAC address (USING BASE ADDRESS AS LS 24 BITS) */
    IOWR_ALTERA_TSEMAC_MAC_0(base,
            ((int)( (base & 0xFFFFFF) ) |
                    (int)( (0xE8 << 24)) ));
    IOWR_ALTERA_TSEMAC_MAC_1(base,
            ((0x30 <<  8) | 0x2D) );

    /* Initialize MAC registers */

    IOWR_ALTERA_TSEMAC_MAC_0(base, SRC_MAC_L(srcMAC));
    IOWR_ALTERA_TSEMAC_MAC_1(base, SRC_MAC_H(srcMAC));

    IOWR_ALTERA_TSEMAC_RX_ALMOST_EMPTY(base, 8);
    IOWR_ALTERA_TSEMAC_RX_ALMOST_FULL(base, 8);
    IOWR_ALTERA_TSEMAC_TX_ALMOST_EMPTY(base, 8);
    IOWR_ALTERA_TSEMAC_TX_ALMOST_FULL(base, 3);
    IOWR_ALTERA_TSEMAC_TX_SECTION_EMPTY(base, txFifoDepth-16);
    if(TSE_MAC_MODE_FPGA_PUB_TX_STORE_AND_FORWARD_MODE == mode)
        IOWR_ALTERA_TSEMAC_TX_SECTION_FULL(base,  0);
    else
        IOWR_ALTERA_TSEMAC_TX_SECTION_FULL(base, 16);
    IOWR_ALTERA_TSEMAC_RX_SECTION_EMPTY(base, rxFifoDepth-16);
    IOWR_ALTERA_TSEMAC_RX_SECTION_FULL(base,  16);
    IOWR_ALTERA_TSEMAC_FRM_LENGTH(base, 0x2400); // 9216 BYTES, RX FIFO???

    IOWR_ALTERA_TSEMAC_TX_IPG_LENGTH(base, 12);
    IOWR_ALTERA_TSEMAC_PAUSE_QUANT(base, 0xFFFF);

    IOWR_ALTERA_TSEMAC_TX_CMD_STAT(base, TSE_TX_CMD_STAT_REG_FOR_MAC_MODE[mode]);
    IOWR_ALTERA_TSEMAC_RX_CMD_STAT(base, TSE_RX_CMD_STAT_REG_FOR_MAC_MODE[mode]);

    /* Setup PCS */
    IOWR_ALTERA_TSEPCS_IF_MODE(base,
            (speed |
                    PCS_IF_MODE_sgmii_ena)
                    );

    /* Restart MAC */
    IOWR_ALTERA_TSEMAC_CMD_CONFIG(base,
            TSE_CONFIG_REG_FOR_MAC_MODE[mode]
            );

    tmp = IORD_ALTERA_TSEMAC_CMD_CONFIG(base);
    mmdrv_printf("OK, x=%d, CMD_CONFIG=0x%08x\n", (int)x, (unsigned)tmp);

    /* Reset PCS */
    IOWR_ALTERA_TSEPCS_CONTROL(base,
            (PCS_CTL_sw_reset |
                    PCS_CTL_fullduplex |
                    PCS_CTL_speed1 /* 1Gbps */)
                    );
    x = 0;
    while(IORD_ALTERA_TSEPCS_CONTROL(base) & PCS_CTL_sw_reset){
        if( x++ > 10000 ) {
            mmdrv_printf("TSEPCS SW reset bit never cleared!\n");
        }
    }


    return 0;
}

/*
 * Resets the TSE.
 * returns 0 on success.
 */
int tsd_tse_sxe_reset(uint32_t base)
{
    uint32_t x, config, pcs_cfg;

    config = IORD_ALTERA_TSEMAC_CMD_CONFIG(base);

    // reset and disable tx and rx in command config register
    IOWR_ALTERA_TSEMAC_CMD_CONFIG(base,
            mmac_cc_SW_RESET_mask |
            0 /*mmac_cc_TX_ENA_mask */ |
            0 /*mmac_cc_RX_ENA_mask*/);

    // wait for reset to clear
    x = 0;
    while(IORD_ALTERA_TSEMAC_CMD_CONFIG(base) &
            ALTERA_TSEMAC_CMD_SW_RESET_MSK) {
        if( x++ > 10000 ) {
            break;
        }
    }
    if(x >= 10000) {
        mmdrv_printf("TSEMAC SW reset bit never cleared!\n");
        return 2;
    }

    /* Restart MAC with original config*/
    IOWR_ALTERA_TSEMAC_CMD_CONFIG(base,
            ( config )
            );

    pcs_cfg = IORD_ALTERA_TSEPCS_CONTROL(base);
    /* Reset PCS while holding original config */
    IOWR_ALTERA_TSEPCS_CONTROL(base,
            (PCS_CTL_sw_reset | pcs_cfg)
                    );
    x = 0;
    while(IORD_ALTERA_TSEPCS_CONTROL(base) & PCS_CTL_sw_reset){
        if( x++ > 10000 ) {
            mmdrv_printf("TSEPCS SW reset bit never cleared!\n");
        }
    }
    return 0;
}

/*
 * Sets TX CRC Generation mode on/off
 */
int tsd_tse_sxe_tx_crc_generation(uint32_t base, bool on)
{
	uint32_t cmdStatReg;
	bool reset = false;
	/*
	 * Setting ALTERA_TSEMAC_TX_CMD_STAT_OMITCRC_MSK omits the CRC calculation
	 * This means that the CRC is already in the payload (calculated by the application)
	 */
	cmdStatReg = IORD_ALTERA_TSEMAC_TX_CMD_STAT(base);
	if(on) {
		if(cmdStatReg & ALTERA_TSEMAC_TX_CMD_STAT_OMITCRC_MSK){
			cmdStatReg &= ~ALTERA_TSEMAC_TX_CMD_STAT_OMITCRC_MSK;
			IOWR_ALTERA_TSEMAC_TX_CMD_STAT(base, cmdStatReg);
			reset = true;
		}
	} else {
		if(0 == (cmdStatReg & ALTERA_TSEMAC_TX_CMD_STAT_OMITCRC_MSK)){
			cmdStatReg |= ALTERA_TSEMAC_TX_CMD_STAT_OMITCRC_MSK;
			IOWR_ALTERA_TSEMAC_TX_CMD_STAT(base, cmdStatReg);
			reset = true;
		}
	}
	// do we need to reset after doing this???
	if(reset){
		return tsd_tse_sxe_reset(base);
	}
	return 0;
}

/*
 * Gets a value indicating if CRC Generation mode is on or off.
 */
bool tsd_tse_tx_crc_generation_is_on(uint32_t base)
{
	uint32_t cmdStatReg;
	cmdStatReg = IORD_ALTERA_TSEMAC_TX_CMD_STAT(base);
	return (cmdStatReg & ALTERA_TSEMAC_TX_CMD_STAT_OMITCRC_MSK) ? false : true;
}
