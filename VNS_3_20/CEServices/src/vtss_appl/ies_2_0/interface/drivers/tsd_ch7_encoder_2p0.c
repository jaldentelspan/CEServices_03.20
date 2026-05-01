/*
 * tsd_ch7_encoder_2p0.c
 *
 *  Created on: May 9, 2022
 *      Author: eric
 */



/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */
#include <stdio.h>
#include <math.h>
#include "mmdrv_printf.h"
#include "tsd_ch7_encoder_2p0.h"
#include "tsd_register_common.h"

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_ch7_enc_debug_print(uint32_t base)
{
    double fpgaVersion, driverVersion;
    mmdrv_printf("CH7 Encoder Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_ch7_enc_get_version(base, &fpgaVersion, &driverVersion))
    {
        mmdrv_printf(" - FPGA Version:  %f\n", fpgaVersion);
        mmdrv_printf(" - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        mmdrv_printf(" - Version:  FAILED TO READ\n");
    }

    mmdrv_printf(" - REGISTERS:\n");
    mmdrv_printf("    => %02X: VERSION REG            = 0x%08X\n", TSD_CH7_ENC_VERSION_REG           , TSD_CH7_ENC_IORD_VERSION(base));
    mmdrv_printf("    => %02X: IP ID REG              = 0x%08X\n", TSD_CH7_ENC_IP_ID_REG             , TSD_CH7_ENC_IORD_IP_ID(base));
    mmdrv_printf("    => %02X: STATUS REG             = 0x%08X\n", TSD_CH7_ENC_STATUS_REG            , TSD_CH7_ENC_IORD_STATUS(base));
    mmdrv_printf("    => %02X: CONTROL REG            = 0x%08X\n", TSD_CH7_ENC_CTRL_REG              , TSD_CH7_ENC_IORD_CONTROL(base));
    mmdrv_printf("    => %02X: PORT SELECT REG        = 0x%08X\n", TSD_CH7_ENC_PORT_SELECT_REG       , TSD_CH7_ENC_IORD_PORT_SELECT(base));
    mmdrv_printf("    => %02X: BITRATE REG            = 0x%08X\n", TSD_CH7_ENC_PORT_BIT_RATE_REG     , TSD_CH7_ENC_IORD_BITRATE(base));
    mmdrv_printf("    => %02X: SYNC PATTERN REG       = 0x%08X\n", TSD_CH7_ENC_SYNC_PATTERN_REG      , TSD_CH7_ENC_IORD_SYNC_PATTERN(base));
    mmdrv_printf("    => %02X: SYNC MASK REG          = 0x%08X\n", TSD_CH7_ENC_SYNC_MASK_REG         , TSD_CH7_ENC_IORD_SYNC_MASK(base));
    mmdrv_printf("    => %02X: SYNC LENGTH REG        = 0x%08X\n", TSD_CH7_ENC_SYNC_LENGTH_REG       , TSD_CH7_ENC_IORD_SYNC_LENGTH(base));
    mmdrv_printf("    => %02X: ETH FRAME CNT REG      = 0x%08X\n", TSD_CH7_ENC_ETH_RX_CNT_REG        , TSD_CH7_ENC_IORD_ETH_RX_CNT(base));
    mmdrv_printf("    => %02X: CH10 PACKET CNT REG    = 0x%08X\n", TSD_CH7_ENC_CH10_RX_CNT_REG       , TSD_CH7_ENC_IORD_CH10_RX_CNT(base));
    mmdrv_printf("    => %02X: ETH OVERFLOW CNT REG   = 0x%08X\n", TSD_CH7_ENC_ETH_OVERFLOW_CNT_REG  , TSD_CH7_ENC_IORD_ETH_OVERFLOW_CNT(base));
    mmdrv_printf("    => %02X: ETH UNDERFLOW CNT REG  = 0x%08X\n", TSD_CH7_ENC_ETH_UNDERFLOW_CNT_REG , TSD_CH7_ENC_IORD_ETH_UNDERFLOW_CNT(base));
    mmdrv_printf("    => %02X: CH10 OVERFLOW CNT REG  = 0x%08X\n", TSD_CH7_ENC_CH10_OVERFLOW_CNT_REG , TSD_CH7_ENC_IORD_CH10_OVERFLOW_CNT(base));
    mmdrv_printf("    => %02X: CH10 UNDERFLOW CNT REG = 0x%08X\n", TSD_CH7_ENC_CH10_UNDERFLOW_CNT_REG, TSD_CH7_ENC_IORD_CH10_UNDERFLOW_CNT(base));
    mmdrv_printf("    => %02X: PACKET ERROR CNT REG   = 0x%08X\n", TSD_CH7_ENC_CH10_PKT_ERR_CNT_REG  , TSD_CH7_ENC_IORD_PKT_ERR_CNT(base));
}

/*
 * Gets the Telspan CH72p0 core version
 *
 * returns 0 on success.
 */
int tsd_ch7_enc_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
    uint32_t version = TSD_CH7_ENC_IORD_VERSION(base);
    uint32_t vH, vL;

    if(version == 0xDEADBEEF)
    {
        return 1;
    }
    *driverVersion = (double)TSD_CH7_ENC_DRIVER_VERSION;
    vH = (version >> 16);
    vL = (version & 0xFFFF);
    if(vL < 100)
        *fpgaVersion = (double)vH + (double)(vL/100);
    else
        *fpgaVersion = (double)vH + (double)(vL/1000);

    return 0;
}

/*
 * Enables the CH7 mode of the Telspan CH7 Encoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_enc_enable(uint32_t base) {
    return set_register_bits(base, TSD_CH7_ENC_CTRL_REG, TSD_CH7_ENC_CTRL_ENABLE_BIT);
}

/*
 * Disables the Telspan CH7 Encoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_enc_disable(uint32_t base) {
    return clear_register_bits(base, TSD_CH7_ENC_CTRL_REG, TSD_CH7_ENC_CTRL_ENABLE_BIT);
}

/*
 * val = true to remove the fcs from the ethernet encoded frames
 * val = false to keep the fcs in the ethernet encoded frames
 * returns 0 on success.
 */
int tsd_ch7_enc_remove_fcs(uint32_t base, bool val) {
    if(val) {
        return set_register_bits(base, TSD_CH7_ENC_CTRL_REG, TSD_CH7_ENC_CTRL_REMOVE_FCS);
    } else {
        return clear_register_bits(base, TSD_CH7_ENC_CTRL_REG, TSD_CH7_ENC_CTRL_REMOVE_FCS);
    }
}

/*
 * val = true to invert the tx pcm clock (phase 180)
 * val = false to not invert tx pcm clock (phase 0)
 * returns 0 on success.
 */
int tsd_ch7_enc_invert_clock(uint32_t base, bool val) {
    if(val) {
        return set_register_bits(base, TSD_CH7_ENC_CTRL_REG, TSD_CH7_ENC_CTRL_INVERT_CLK);
    } else {
        return clear_register_bits(base, TSD_CH7_ENC_CTRL_REG, TSD_CH7_ENC_CTRL_INVERT_CLK);
    }
}

/*
 * val = true to remove the 16-bits of padding from the tse ethernet frames
 *     == this is a setting in the TSE... if the TSE is adding padding set this to true
 * val = false do not remobe padding from tse ethernet frames
 *     == this means that TSE is not adding padding to the frame.
 * returns 0 on success.
 */
int tsd_ch7_enc_remove_tse_padding(uint32_t base, bool val) {
    if(val) {
        return set_register_bits(base, TSD_CH7_ENC_CTRL_REG, TSD_CH7_ENC_CTRL_REMOVE_TSE_PADDING);
    } else {
        return clear_register_bits(base, TSD_CH7_ENC_CTRL_REG, TSD_CH7_ENC_CTRL_REMOVE_TSE_PADDING);
    }
}

/*
 * Sets the CH7 frame size.
 * val is the number of 223 byte chunks in the frame (so bytes/frame = val*223)
 * val MUST be between 1 and 7
 * returns 0 on success
 */
int tsd_ch7_enc_set_frame_size(uint32_t base, int val) {
    if((val < 1) || (val > 8)) return 1;
    return set_register_region(base,
            TSD_CH7_ENC_CTRL_REG,
            TSD_CH7_ENC_CTRL_FRAME_SIZE_MASK,
            TSD_CH7_ENC_CTRL_FRAME_SIZE_SHFT,
            (uint32_t)val);
}

/*
 * Gets the CH7 frame size.
 * returns 0 on success
 */
int tsd_ch7_enc_get_frame_size(uint32_t base, int *val) {
    return get_register_region(base,
            TSD_CH7_ENC_CTRL_REG,
            TSD_CH7_ENC_CTRL_FRAME_SIZE_MASK,
            TSD_CH7_ENC_CTRL_FRAME_SIZE_SHFT,
            (uint32_t*)val);
}

/*
 * Sets the CH7 port selector.
 * returns 0 on success
 */
int tsd_ch7_enc_set_port_select(uint32_t base, int val) {
    return set_register_region(base,
            TSD_CH7_ENC_PORT_SELECT_REG,
            TSD_CH7_ENC_PORT_SELECT_MASK,
            TSD_CH7_ENC_PORT_SELECT_SHFT,
            (uint32_t)val);
}

/*
 * Sets the CH7 encoder bitrate.
 * returns 0 on success
 */
int tsd_ch7_enc_set_bitrate(uint32_t base, int bitrate, int sysclkRate) {
    uint64_t bitrate64 = ((uint64_t)bitrate) << 32;
    // TODO: Figure out round
    /* ulonglong setval = round((ulonglong)bitrate64 / (ulonglong)sysclkRate); */
    ulonglong setval = (ulonglong)bitrate64 / (ulonglong)sysclkRate;
    TSD_CH7_ENC_IOWR_BITRATE(base, (uint32_t)setval);
    return 0;
}

/*
 * Gets the CH7 encoder bitrate.
 * returns 0 on success
 */
int tsd_ch7_enc_get_bitrate(uint32_t base, int *bitrate, int sysclkRate) {
    uint32_t regval = 0;
    regval = TSD_CH7_ENC_IORD_BITRATE(base);
    *bitrate = (int) ((uint64_t)((uint64_t)regval * (uint64_t)sysclkRate ) >> 32);
    return 0;
}

/*
 * Sets the CH7 encoder sync pattern.
 * returns 0 on success
 */
int tsd_ch7_enc_set_sync_pattern(uint32_t base, uint32_t syncPattern, int syncLen) {
    TSD_CH7_ENC_IOWR_SYNC_PATTERN(base, syncPattern);
    TSD_CH7_ENC_IOWR_SYNC_MASK(base, 0); /* 1= DONT CARE */
    TSD_CH7_ENC_IOWR_SYNC_LENGTH(base, (uint32_t)(syncLen & 0xFF));
    return 0;
}

/*
 * Gets the CH7 encoder sync pattern.
 * returns 0 on success
 */
int tsd_ch7_enc_get_sync_pattern(uint32_t base, uint32_t *syncPattern, int *syncLen) {
    *syncPattern = TSD_CH7_ENC_IORD_SYNC_PATTERN(base);
    *syncLen = TSD_CH7_ENC_IORD_SYNC_LENGTH(base) & 0xFF;
    return 0;
}

/*
 * Gets the number of eth frames received by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_eth_frame_count(uint32_t base, uint32_t *cnt) {
    *cnt = TSD_CH7_ENC_IORD_ETH_RX_CNT(base) & TSD_CH7_ENC_COUNTER_MASK;
    return 0;
}

/*
 * Gets the number of ch10 pcakets received by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_ch10_packet_count(uint32_t base, uint32_t *cnt) {
    *cnt = TSD_CH7_ENC_IORD_CH10_RX_CNT(base) & TSD_CH7_ENC_COUNTER_MASK;
    return 0;
}

/*
 * Gets the number of overflow eth frames dropped by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_eth_overflow_count(uint32_t base, uint32_t *cnt) {
    *cnt = TSD_CH7_ENC_IORD_ETH_OVERFLOW_CNT(base) & TSD_CH7_ENC_COUNTER_MASK;
    return 0;
}

/*
 * Gets the number of overflow ch10 packets dropped by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_ch10_overflow_count(uint32_t base, uint32_t *cnt) {
    *cnt = TSD_CH7_ENC_IORD_CH10_OVERFLOW_CNT(base) & TSD_CH7_ENC_COUNTER_MASK;
    return 0;
}

/*
 * Gets the number of underflow eth frames dropped by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_eth_underflow_count(uint32_t base, uint32_t *cnt) {
    *cnt = TSD_CH7_ENC_IORD_ETH_UNDERFLOW_CNT(base) & TSD_CH7_ENC_COUNTER_MASK;
    return 0;
}

/*
 * Gets the number of underflow ch10 packets dropped by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_ch10_underflow_count(uint32_t base, uint32_t *cnt) {
    *cnt = TSD_CH7_ENC_IORD_CH10_UNDERFLOW_CNT(base) & TSD_CH7_ENC_COUNTER_MASK;
    return 0;
}

/*
 * Resets all of the CH7 Counters in the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_reset_counters(uint32_t base) {
    IOWR(base, TSD_CH7_ENC_ETH_RX_CNT_REG        , TSD_CH7_ENC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_ENC_CH10_RX_CNT_REG       , TSD_CH7_ENC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_ENC_ETH_OVERFLOW_CNT_REG  , TSD_CH7_ENC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_ENC_ETH_UNDERFLOW_CNT_REG , TSD_CH7_ENC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_ENC_CH10_OVERFLOW_CNT_REG , TSD_CH7_ENC_COUNTER_RESET_MASK);
    IOWR(base, TSD_CH7_ENC_CH10_UNDERFLOW_CNT_REG, TSD_CH7_ENC_COUNTER_RESET_MASK);

    IOWR(base, TSD_CH7_ENC_ETH_RX_CNT_REG        , 0);
    IOWR(base, TSD_CH7_ENC_CH10_RX_CNT_REG       , 0);
    IOWR(base, TSD_CH7_ENC_ETH_OVERFLOW_CNT_REG  , 0);
    IOWR(base, TSD_CH7_ENC_ETH_UNDERFLOW_CNT_REG , 0);
    IOWR(base, TSD_CH7_ENC_CH10_OVERFLOW_CNT_REG , 0);
    IOWR(base, TSD_CH7_ENC_CH10_UNDERFLOW_CNT_REG, 0);
    return 0;
}
