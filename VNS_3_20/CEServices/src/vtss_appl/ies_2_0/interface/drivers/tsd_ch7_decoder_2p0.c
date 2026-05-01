/*
 * tsd_ch7_decoder_2p0.c
 *
 *  Created on: May 9, 2022
 *      Author: eric
 */



/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */
#include <stdio.h>
#include "mmdrv_printf.h"
#include "tsd_ch7_decoder_2p0.h"
#include "tsd_register_common.h"

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_ch7_dec_debug_print(uint32_t base)
{
    double fpgaVersion, driverVersion;
    mmdrv_printf("CH7 Decoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_ch7_dec_get_version(base, &fpgaVersion, &driverVersion))
    {
        mmdrv_printf(" - FPGA Version:  %f\n", fpgaVersion);
        mmdrv_printf(" - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        mmdrv_printf(" - Version:  FAILED TO READ\n");
    }

    mmdrv_printf(" - REGISTERS:\n");
    mmdrv_printf("    => %02X: VERSION REG         = 0x%08X\n", TSD_CH7_DEC_VERSION_REG            , TSD_CH7_DEC_IORD_VERSION(base));
    mmdrv_printf("    => %02X: IP ID REG           = 0x%08X\n", TSD_CH7_DEC_IP_ID_REG              , TSD_CH7_DEC_IORD_IP_ID(base));
    mmdrv_printf("    => %02X: STATUS REG          = 0x%08X\n", TSD_CH7_DEC_STATUS_REG             , TSD_CH7_DEC_IORD_STATUS(base));
    mmdrv_printf("    => %02X: CONTROL REG         = 0x%08X\n", TSD_CH7_DEC_CTRL_REG               , TSD_CH7_DEC_IORD_CONTROL(base));
    mmdrv_printf("    => %02X: PORT SELECT REG     = 0x%08X\n", TSD_CH7_DEC_PORT_SELECT_REG        , TSD_CH7_DEC_IORD_PORT_SELECT(base));
    mmdrv_printf("    => %02X: SYNC PATTERN REG    = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_REG       , TSD_CH7_DEC_IORD_SYNC_PATTERN(base));
    mmdrv_printf("    => %02X: SYNC MASK REG       = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_MASK_REG  , TSD_CH7_DEC_IORD_SYNC_MASK(base));
    mmdrv_printf("    => %02X: SYNC LENGTH REG     = 0x%08X\n", TSD_CH7_DEC_SYNC_PATTERN_LEN_REG   , TSD_CH7_DEC_IORD_SYNC_LENGTH(base));
    mmdrv_printf("    => %02X: TX ETH PKT CNT REG  = 0x%08X\n", TSD_CH7_DEC_TX_ETH_PACKET_CNT_REG  , TSD_CH7_DEC_IORD_TX_ETH_PACKET_CNT(base));
    mmdrv_printf("    => %02X: TX CH10 PKT CNT REG = 0x%08X\n", TSD_CH7_DEC_TX_CH10_PACKET_CNT_REG , TSD_CH7_DEC_IORD_TX_CH10_PACKET_CNT(base));
    mmdrv_printf("    => %02X: DEC FIFO0 OFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO0_OVERFLOW_CNT_REG , TSD_CH7_DEC_IORD_FIFO0_OVERFLOW_CNT(base));
    mmdrv_printf("    => %02X: DEC FIFO0 UFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO0_UNDERFLOW_CNT_REG, TSD_CH7_DEC_IORD_FIFO0_UNDERFLOW_CNT(base));
    mmdrv_printf("    => %02X: DEC FIFO1 OFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO1_OVERFLOW_CNT_REG , TSD_CH7_DEC_IORD_FIFO1_OVERFLOW_CNT(base));
    mmdrv_printf("    => %02X: DEC FIFO1 UFLOW REG = 0x%08X\n", TSD_CH7_DEC_FIFO1_UNDERFLOW_CNT_REG, TSD_CH7_DEC_IORD_FIFO1_UNDERFLOW_CNT(base));
}

/*
 * Gets the Telspan CH7 Decoder core version
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
    uint32_t version = TSD_CH7_DEC_IORD_VERSION(base);
    uint32_t vH, vL;

    if(version == 0xDEADBEEF)
    {
        return 1;
    }
    *driverVersion = (double)TSD_CH7_DEC_DRIVER_VERSION;
    vH = (version >> 16);
    vL = (version & 0xFFFF);
    if(vL < 100)
        *fpgaVersion = (double)vH + (double)(vL/100);
    else
        *fpgaVersion = (double)vH + (double)(vL/1000);

    return 0;
}

/*
 * Enables the CH7 mode of the Telspan CH7 Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_enable(uint32_t base) {
    return set_register_bits(base, TSD_CH7_DEC_CTRL_REG, TSD_CH7_DEC_CTRL_ENABLE_BIT);
}

/*
 * Disables the Telspan CH7 Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_disable(uint32_t base) {
    return clear_register_bits(base, TSD_CH7_DEC_CTRL_REG, TSD_CH7_DEC_CTRL_ENABLE_BIT);
}

/*
 * Sets other control bits in the Telspan CH7 Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_set_control_bits(uint32_t base, uint32_t ctrlBits) {
    return set_register_bits(base, TSD_CH7_DEC_CTRL_REG, ctrlBits);
}

/*
 * Clears control bits in the Telspan CH7 Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_clear_control_bits(uint32_t base, uint32_t ctrlBits) {
    return clear_register_bits(base, TSD_CH7_DEC_CTRL_REG, ctrlBits);
}

/*
 * Gets the CH7 decoder status.
 * returns 0 on success
 */
int tsd_ch7_dec_get_status(uint32_t base, uint32_t *status) {
    *status = TSD_CH7_DEC_IORD_STATUS(base);
    return 0;
}

/*
 * Sets the CH7 frame size.
 * val is the number of 223 byte chunks in the frame (so bytes/frame = val*223)
 * val MUST be between 1 and 7
 * returns 0 on success
 */
int tsd_ch7_dec_set_frame_size(uint32_t base, int val) {
    if((val < 1) || (val > 8)) return 1;
    return set_register_region(base,
            TSD_CH7_DEC_CTRL_REG,
            TSD_CH7_DEC_CTRL_FRAME_SIZE_MASK,
            TSD_CH7_DEC_CTRL_FRAME_SIZE_SHFT,
            (uint32_t)val);
}

/*
 * gets the CH7 frame size.
 * returns 0 on success
 */
int tsd_ch7_dec_get_frame_size(uint32_t base, int *val) {
    return get_register_region(base,
            TSD_CH7_DEC_CTRL_REG,
            TSD_CH7_DEC_CTRL_FRAME_SIZE_MASK,
            TSD_CH7_DEC_CTRL_FRAME_SIZE_SHFT,
            (uint32_t*)val);
}

/*
 * Sets the CH7 decoder port select.
 * returns 0 on success
 */
int tsd_ch7_dec_set_port_select(uint32_t base, int sel, bool enable) {
    uint32_t val = (uint32_t)((sel & TSD_CH7_DEC_PORT_SELECT_MASK) | (enable ? TSD_CH7_DEC_PORT_SELECT_ENABLE : 0));
    TSD_CH7_DEC_IOWR_PORT_SELECT(base, val);
    return 0;
}

/*
 * Gets the CH7 decoder port select.
 * returns 0 on success
 */
int tsd_ch7_dec_get_port_select(uint32_t base, int *sel, bool *enable) {
    uint32_t val = TSD_CH7_DEC_IORD_PORT_SELECT(base);
    *sel = val & TSD_CH7_DEC_PORT_SELECT_MASK;
    *enable = ((val & TSD_CH7_DEC_PORT_SELECT_ENABLE) ? true : false);
    return 0;
}

/*
 * Sets the CH7 decoder sync pattern.
 * returns 0 on success
 */
int tsd_ch7_dec_set_sync_pattern(uint32_t base, uint32_t syncPattern, int syncLen) {
    TSD_CH7_DEC_IOWR_SYNC_PATTERN(base, syncPattern);
    TSD_CH7_DEC_IOWR_SYNC_MASK(base, 0); /* 1= DONT CARE */
    TSD_CH7_DEC_IOWR_SYNC_LENGTH(base, (uint32_t)(syncLen & 0xFF));
    return 0;
}

/*
 * Gets the CH7 decoder sync pattern.
 * returns 0 on success
 */
int tsd_ch7_dec_get_sync_pattern(uint32_t base, uint32_t *syncPattern, int *syncLen) {
    *syncPattern = TSD_CH7_DEC_IORD_SYNC_PATTERN(base);
    *syncLen = TSD_CH7_DEC_IORD_SYNC_LENGTH(base) & 0xFF;
    return 0;
}


/*
 * Gets the FIFO underflow Count in the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_get_fifo_underflow_count(uint32_t base, uint32_t *cnt) {
    *cnt = (TSD_CH7_DEC_IORD_FIFO0_UNDERFLOW_CNT(base) & TSD_CH7_DEC_COUNTER_MASK) +
            (TSD_CH7_DEC_IORD_FIFO1_UNDERFLOW_CNT(base) & TSD_CH7_DEC_COUNTER_MASK);
    return 0;
}

/*
 * Gets the FIFO overflow Count in the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_get_fifo_overflow_count(uint32_t base, uint32_t *cnt) {
    *cnt = (TSD_CH7_DEC_IORD_FIFO0_OVERFLOW_CNT(base) & TSD_CH7_DEC_COUNTER_MASK) +
            (TSD_CH7_DEC_IORD_FIFO1_OVERFLOW_CNT(base) & TSD_CH7_DEC_COUNTER_MASK);
    return 0;
}

/*
 * Gets the number of frames sent to the TSE from the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_get_tse_send_count(uint32_t base, uint32_t *cnt) {
    *cnt = TSD_CH7_DEC_IORD_TX_ETH_PACKET_CNT(base);
    return 0;
}

/*
 * Gets the number of ch10 frames sent from the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_get_ch10_send_count(uint32_t base, uint32_t *cnt) {
    *cnt = TSD_CH7_DEC_IORD_TX_CH10_PACKET_CNT(base);
    return 0;
}

/*
 * Resets all of the CH7 Counters in the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_reset_counters(uint32_t base) {
    IOWR(base, TSD_CH7_DEC_TX_ETH_PACKET_CNT_REG  , TSD_CH7_DEC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_DEC_TX_CH10_PACKET_CNT_REG , TSD_CH7_DEC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_DEC_FIFO0_OVERFLOW_CNT_REG , TSD_CH7_DEC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_DEC_FIFO0_UNDERFLOW_CNT_REG, TSD_CH7_DEC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_DEC_FIFO1_OVERFLOW_CNT_REG , TSD_CH7_DEC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_DEC_FIFO1_UNDERFLOW_CNT_REG, TSD_CH7_DEC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_DEC_TX_ETH_PACKET_CNT_REG  , 0);
    IOWR(base, TSD_CH7_DEC_TX_CH10_PACKET_CNT_REG , 0);
    IOWR(base, TSD_CH7_DEC_FIFO0_OVERFLOW_CNT_REG , 0);
    IOWR(base, TSD_CH7_DEC_FIFO0_UNDERFLOW_CNT_REG, 0);
    IOWR(base, TSD_CH7_DEC_FIFO1_OVERFLOW_CNT_REG , 0);
    IOWR(base, TSD_CH7_DEC_FIFO1_UNDERFLOW_CNT_REG, 0);

    return 0;
}
