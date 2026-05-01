/*
 * tsd_hdlc_decoder_2p0.c
 *
 *  Created on: May 9, 2022
 *      Author: eric
 */



/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */
#include <stdio.h>
#include "mmdrv_printf.h"
#include "tsd_hdlc_decoder_2p0.h"
#include "tsd_register_common.h"

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_hdlc_dec_debug_print(uint32_t base)
{
    double fpgaVersion, driverVersion;
    mmdrv_printf("HDLC Decoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_hdlc_dec_get_version(base, &fpgaVersion, &driverVersion))
    {
        mmdrv_printf(" - FPGA Version:  %f\n", fpgaVersion);
        mmdrv_printf(" - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        mmdrv_printf(" - Version:  FAILED TO READ\n");
    }

    mmdrv_printf(" - REGISTERS:\n");
    mmdrv_printf("    => %02X: VERSION REG          = 0x%08X\n", TSD_HDLC_DEC_VERSION_REG             , TSD_HDLC_DEC_IORD_VERSION(base));
    mmdrv_printf("    => %02X: IP ID REG            = 0x%08X\n", TSD_HDLC_DEC_IP_ID_REG               , TSD_HDLC_DEC_IORD_IP_ID(base));
    mmdrv_printf("    => %02X: STATUS REG           = 0x%08X\n", TSD_HDLC_DEC_STATUS_REG              , TSD_HDLC_DEC_IORD_STATUS(base));
    mmdrv_printf("    => %02X: CONTROL REG          = 0x%08X\n", TSD_HDLC_DEC_CTRL_REG                , TSD_HDLC_DEC_IORD_CONTROL(base));
    mmdrv_printf("    => %02X: PACKET CNT REG       = 0x%08X\n", TSD_HDLC_DEC_PACKET_CNT_REG          , TSD_HDLC_DEC_IORD_PACKET_CNT(base));
    mmdrv_printf("    => %02X: FIFO0 OFLOW CNT      = 0x%08X\n", TSD_HDLC_DEC_FIFO0_OFLOW_CNT_REG     , TSD_HDLC_DEC_IORD_FIFO0_OFLOW_CNT(base));
    mmdrv_printf("    => %02X: FIFO0 UFLOW CNT      = 0x%08X\n", TSD_HDLC_DEC_FIFO0_UFLOW_CNT_REG     , TSD_HDLC_DEC_IORD_FIFO0_UFLOW_CNT(base));
    mmdrv_printf("    => %02X: BIT STUFFING ERR CNT = 0x%08X\n", TSD_HDLC_DEC_STUFFING_ERR_CNT_REG     , TSD_HDLC_DEC_IORD_STUFFING_ERR_CNT(base));
}

/*
 * Gets the Telspan HDLC Decoder core version
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
    uint32_t version = TSD_HDLC_DEC_IORD_VERSION(base);
    uint32_t vH, vL;

    if(version == 0xDEADBEEF)
    {
        return 1;
    }
    *driverVersion = (double)TSD_HDLC_DEC_DRIVER_VERSION;
    vH = (version >> 16);
    vL = (version & 0xFFFF);
    if(vL < 100)
        *fpgaVersion = (double)vH + (double)(vL/100);
    else
        *fpgaVersion = (double)vH + (double)(vL/1000);

    return 0;
}

/*
 * Enables the HDLC mode of the Telspan HDLC Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_enable(uint32_t base) {
    return set_register_bits(base, TSD_HDLC_DEC_CTRL_REG, TSD_HDLC_DEC_CTRL_ENABLE_BIT);
}

/*
 * Disables the Telspan HDLC Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_disable(uint32_t base) {
    return clear_register_bits(base, TSD_HDLC_DEC_CTRL_REG, TSD_HDLC_DEC_CTRL_ENABLE_BIT);
}


/*
 * Gets the status word from the Telspan HDLC Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_status(uint32_t base) {
    return TSD_HDLC_DEC_IORD_STATUS(base);
}

/*
 * Sets other control bits in the Telspan HDLC Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_set_control_bits(uint32_t base, uint32_t ctrlBits) {
    return set_register_bits(base, TSD_HDLC_DEC_CTRL_REG, ctrlBits);
}

/*
 * Clears control bits in the Telspan HDLC Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_clear_control_bits(uint32_t base, uint32_t ctrlBits) {
    return clear_register_bits(base, TSD_HDLC_DEC_CTRL_REG, ctrlBits);
}

/*
 * Gets the packet count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_packet_count(uint32_t base, uint32_t *cnt) {
    uint32_t regVal = TSD_HDLC_DEC_IORD_PACKET_CNT(base);
    if(regVal != 0xDEADBEEF) {
        *cnt = regVal & TSD_HDLC_DEC_COUNTER_MASK;
        return 0;
    }
    return 1;
}

/*
 * Gets the FIFO0 underflow Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_fifo0_underflow_count(uint32_t base, uint32_t *cnt) {
    uint32_t regVal = TSD_HDLC_DEC_IORD_FIFO0_UFLOW_CNT(base);
    if(regVal != 0xDEADBEEF) {
        *cnt = regVal & TSD_HDLC_DEC_COUNTER_MASK;
        return 0;
    }
    return 1;
}
/*
 * Gets the FIFO0 underflow Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_fifo0_overflow_count(uint32_t base, uint32_t *cnt) {
    uint32_t regVal = TSD_HDLC_DEC_IORD_FIFO0_OFLOW_CNT(base);
    if(regVal != 0xDEADBEEF) {
        *cnt = regVal & TSD_HDLC_DEC_COUNTER_MASK;
        return 0;
    }
    return 1;
}

/*
 * Gets the bit stuffing error Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_bitstuffing_error_count(uint32_t base, uint32_t *cnt) {
    uint32_t regVal = TSD_HDLC_DEC_IORD_STUFFING_ERR_CNT(base);
    if(regVal != 0xDEADBEEF) {
        *cnt = regVal & TSD_HDLC_DEC_COUNTER_MASK;
        return 0;
    }
    return 1;
}

/*
 * Resets all of the HDLC Counters in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_reset_counters(uint32_t base) {
    TSD_HDLC_DEC_RESET_PACKET_CNT(base);
    TSD_HDLC_DEC_RESET_FIFO0_UFLOW_CNT(base);
    TSD_HDLC_DEC_RESET_FIFO0_OFLOW_CNT(base);
    TSD_HDLC_DEC_RESET_STUFFING_ERR_CNT(base);
    return 0;
}
