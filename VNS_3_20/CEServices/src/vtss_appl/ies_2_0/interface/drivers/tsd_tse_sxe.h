/*
 * tsd_tse_sxe.h
 *
 *  Created on: May 15, 2018
 *      Author: eric
 */
#include <io.h>
/* #include "stdint.h" */
#include "main.h"
/* #include "stdbool.h" */

#ifndef TSD_TSE_SXE_H_
#define TSD_TSE_SXE_H_

#define TSD_TSE_SXE_DRIVER_VERSION        (0.01)

//#define ALTERA_ETH_TSE_INSTANCE(name, dev) extern int alt_no_storage
//#define ALTERA_ETH_TSE_INIT(name, dev) while(0)

/**************************************
 * additional register accesses (PCS)
 **************************************/
#define ALTERA_TSEPCS_DWORD_OFFSET            (0x80)
#define ALTERA_TSEPCS_ADDR(offset)            (ALTERA_TSEPCS_DWORD_OFFSET + offset)

// USE PCS_CTL_XX enumerations
#define IORD_ALTERA_TSEPCS_CONTROL(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x0) )
#define IOWR_ALTERA_TSEPCS_CONTROL(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x0), data)

// USE PCS_ST_XX enumerations
#define IORD_ALTERA_TSEPCS_STATUS(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x1))
#define IOWR_ALTERA_TSEPCS_STATUS(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x1), data)

#define IORD_ALTERA_TSEPCS_PHYID0(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x2))
#define IOWR_ALTERA_TSEPCS_PHYID0(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x2), data)

#define IORD_ALTERA_TSEPCS_PHYID1(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x3))
#define IOWR_ALTERA_TSEPCS_PHYID1(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x3), data)

#define IORD_ALTERA_TSEPCS_DEV_ABILITY(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x4))
#define IOWR_ALTERA_TSEPCS_DEV_ABILITY(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x4), data)

#define IORD_ALTERA_TSEPCS_PART_ABILITY(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x5))
#define IOWR_ALTERA_TSEPCS_PART_ABILITY(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x5), data)

#define IORD_ALTERA_TSEPCS_AN_EXP(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x6))
#define IOWR_ALTERA_TSEPCS_AN_EXP(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x6), data)

#define IORD_ALTERA_TSEPCS_SCRATCH(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x10))
#define IOWR_ALTERA_TSEPCS_SCRATCH(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x10), data)

#define IORD_ALTERA_TSEPCS_REV(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x11))
#define IOWR_ALTERA_TSEPCS_REV(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x11), data)

#define IORD_ALTERA_TSEPCS_LINK_TMR0(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x12))
#define IOWR_ALTERA_TSEPCS_LINK_TMR0(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x12), data)

#define IORD_ALTERA_TSEPCS_LINK_TMR1(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x13))
#define IOWR_ALTERA_TSEPCS_LINK_TMR1(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x13), data)

enum {
        PCS_IF_MODE_sgmii_ena       = 1<<0,             // set to 1 to enable sgmii
        PCS_IF_MODE_use_sgmii_an    = 1<<1,             // set to 1 to use sgmii autonegotiation
        PCS_IF_MODE_sgmii_sp_10     = 0<<2,             // SGMII speed 10Mbps
        PCS_IF_MODE_sgmii_sp_100    = 1<<2,             // SGMII speed 100Mbps
        PCS_IF_MODE_sgmii_sp_1000   = 2<<2,             // SGMII speed 1000Mbps
        PCS_IF_MODE_sgmii_duplex    = 1<<4,             // set to 1 to enable half-duplex
        PCS_IF_MODE_sgmii_an_mode   = 1<<5                // sgmii autoneg mode 0=mac 1=sgmii phy
};

typedef enum ePCS_IF_SPEED_t{
    PCS_IF_SPEED_10Mbps = PCS_IF_MODE_sgmii_sp_10,
    PCS_IF_SPEED_100Mbps = PCS_IF_MODE_sgmii_sp_100,
    PCS_IF_SPEED_1000Mbps = PCS_IF_MODE_sgmii_sp_1000
}ePCS_IF_SPEED;

typedef enum eTSE_MAC_MODE_t {
    TSE_MAC_MODE_FPGA_REASM_RX,
    TSE_MAC_MODE_FPGA_PUB_TX,
    TSE_MAC_MODE_FPGA_PUB_TX_STORE_AND_FORWARD_MODE, /* this mode stores full frames and then sends on eof. */
    /*
     * Note: For now use Store and forward mode for publishers because we have an underflow issue at 0x900 bytes.
     * In this mode do not exceed the TX buffer size (this is the max MTU) with any frame ((TSE_TRANSMIT_FIFO_DEPTH * (TSE_FIFO_WIDTH/8))
     */
	TSE_HDLC_CH7_V1
}eTSE_MAC_MODE;

// USE PCS_IF_MODE_XX enumerations
#define IORD_ALTERA_TSEPCS_IF_MODE(base)      IORD(base, ALTERA_TSEPCS_ADDR(0x14))
#define IOWR_ALTERA_TSEPCS_IF_MODE(base,data) IOWR(base, ALTERA_TSEPCS_ADDR(0x14), data)

#ifdef __cplusplus
extern "C"
{
#endif


/*
 * prints a summary of the core's setup to stdout
 */
void tsd_tse_sxe_debug_print(uint32_t base);

/*
 * Gets the driver version (returned in version)
 * returns 0 on success.
 */
int tsd_tse_sxe_driver_version(double *version);

/*
 * Initializes the TSE for the FPGA.
 * returns 0 on success.
 */
int tsd_tse_sxe_init(uint32_t base, ePCS_IF_SPEED speed, eTSE_MAC_MODE mode, uint32_t txFifoDepth, uint32_t rxFifoDepth, uint8_t srcMAC[6]);

/*
 * Resets the TSE.
 * returns 0 on success.
 */
int tsd_tse_sxe_reset(uint32_t base);

/*
 * Sets TX CRC Generation mode on/off
 */
int tsd_tse_sxe_tx_crc_generation(uint32_t base, bool on);

/*
 * Gets a value indicating if CRC Generation mode is on or off.
 */
bool tsd_tse_tx_crc_generation_is_on(uint32_t base);

#ifdef __cplusplus
}
#endif

#endif /* TSD_TSE_SXE_H_ */

