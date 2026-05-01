/*
 * tsd_ch7_encoder_2p0.h
 *
 *  Created on: Mar 31, 2020
 *      Author: eric
 */

#ifndef TSD_CH7_ENCODER_2P0_H_
#define TSD_CH7_ENCODER_2P0_H_

#include <stddef.h>
/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */
#include "tsd_ch7_encoder_2p0_regs.h"

#define TSD_CH7_ENC_DRIVER_VERSION        (2.0)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_ch7_enc_debug_print(uint32_t base);

/*
 * Gets the Telspan CH7 Encoder core version
 *
 * returns 0 on success.
 */
int tsd_ch7_enc_get_version(uint32_t base, double *fpgaVersion, double *driverVersion);

/*
 * Enables the CH7 mode of the Telspan CH7 Encoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_enc_enable(uint32_t base);

/*
 * Disables the Telspan CH7 Encoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_enc_disable(uint32_t base);

/*
 * val = true to remove the fcs from the ethernet encoded frames
 * val = false to keep the fcs in the ethernet encoded frames
 * returns 0 on success.
 */
int tsd_ch7_enc_remove_fcs(uint32_t base, bool val);

/*
 * val = true to invert the tx pcm clock (phase 180)
 * val = false to not invert tx pcm clock (phase 0)
 * returns 0 on success.
 */
int tsd_ch7_enc_invert_clock(uint32_t base, bool val) ;

/*
 * val = true to remove the 16-bits of padding from the tse ethernet frames
 *     == this is a setting in the TSE... if the TSE is adding padding set this to true
 * val = false do not remobe padding from tse ethernet frames
 *     == this means that TSE is not adding padding to the frame.
 * returns 0 on success.
 */
int tsd_ch7_enc_remove_tse_padding(uint32_t base, bool val);

/*
 * Sets the CH7 frame size.
 * val is the number of 223 byte chunks in the frame (so bytes/frame = val*223)
 * val MUST be between 1 and 7
 * returns 0 on success
 */
int tsd_ch7_enc_set_frame_size(uint32_t base, int val);

/*
 * Gets the CH7 frame size.
 * returns 0 on success
 */
int tsd_ch7_enc_get_frame_size(uint32_t base, int *val);

/*
 * Sets the CH7 port selector.
 * returns 0 on success
 */
int tsd_ch7_enc_set_port_select(uint32_t base, int val);

/*
 * Sets the CH7 encoder bitrate.
 * returns 0 on success
 */
int tsd_ch7_enc_set_bitrate(uint32_t base, int bitrate, int sysclkRate);

/*
 * Gets the CH7 encoder bitrate.
 * returns 0 on success
 */
int tsd_ch7_enc_get_bitrate(uint32_t base, int *bitrate, int sysclkRate);

/*
 * Sets the CH7 encoder sync pattern.
 * returns 0 on success
 */
int tsd_ch7_enc_set_sync_pattern(uint32_t base, uint32_t syncPattern, int syncLen);

/*
 * Gets the CH7 encoder sync pattern.
 * returns 0 on success
 */
int tsd_ch7_enc_get_sync_pattern(uint32_t base, uint32_t *syncPattern, int *syncLen);

/*
 * Gets the number of eth frames received by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_eth_frame_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the number of ch10 pcakets received by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_ch10_packet_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the number of overflow eth frames dropped by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_eth_overflow_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the number of overflow ch10 packets dropped by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_ch10_overflow_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the number of underflow eth frames dropped by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_eth_underflow_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the number of underflow ch10 packets dropped by the PCM from the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_get_rx_ch10_underflow_count(uint32_t base, uint32_t *cnt);

/*
 * Resets all of the CH7 Counters in the Telspan CH7 Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_enc_reset_counters(uint32_t base);

#ifdef __cplusplus
}
#endif




#endif /* TSD_CH7_ENCODER_2P0_H_ */
