/*
 * tsd_hdlc_encoder_2p0.h
 *
 *  Created on: Mar 31, 2020
 *      Author: eric
 */

#ifndef TSD_HDLC_ENCODER_2P0_H_
#define TSD_HDLC_ENCODER_2P0_H_

#include <stddef.h>
/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */
#include "tsd_hdlc_encoder_2p0_regs.h"

#define TSD_HDLC_ENC_DRIVER_VERSION        (2.0)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_hdlc_enc_debug_print(uint32_t base);

/*
 * Gets the Telspan HDLC2p0 core version
 *
 * returns 0 on success.
 */
int tsd_hdlc_enc_get_version(uint32_t base, double *fpgaVersion, double *driverVersion);

/*
 * Enables the HDLC mode of the Telspan HDLC2p0 core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_enc_enable(uint32_t base);

/*
 * Disables the Telspan HDLC2p0 core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_enc_disable(uint32_t base);

/*
 * val = true to remove the tse padding from the ethernet frames prior to encoding
 * val = false to not remove tse padding (meaning TSE is sending data without)
 * returns 0 on success.
 */
int tsd_hdlc_enc_remove_tse_padding(uint32_t base, bool val);

/*
 * Sets other control bits in the Telspan HDLC Encoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_enc_set_control_bits(uint32_t base, uint32_t ctrlBits);

/*
 * Clears control bits in the Telspan HDLC Encoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_enc_clear_control_bits(uint32_t base, uint32_t ctrlBits);

/*
 * Sets the hdlc ctrl/addr fields if enabled in the Telspan HDLC2p0 core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_enc_set_ctrl_addr_values(uint32_t base, uint8_t ctrl, uint8_t addr);

/*
 * Gets the hdlc ctrl/addr values if enabled in the Telspan HDLC2p0 core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_enc_get_ctrl_addr_values(uint32_t base, uint8_t *ctrl, uint8_t *addr);

/*
 * Sets the HDLC encoder bitrate.
 * returns 0 on success
 */
int tsd_hdlc_enc_set_bitrate(uint32_t base, int bitrate, int sysclkRate);

/*
 * Gets the HDLC encoder bitrate.
 * returns 0 on success
 */
int tsd_hdlc_enc_get_bitrate(uint32_t base, int *bitrate, int sysclkRate);

/*
 * Gets the number of eth frames received by the PCM from the Telspan HDLC Encoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_enc_get_rx_eth_frame_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the FIFO0 underflow Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_enc_get_fifo0_underflow_count(uint32_t base, uint32_t *cnt);
/*
 * Gets the FIFO0 underflow Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_enc_get_fifo0_overflow_count(uint32_t base, uint32_t *cnt);

/*
 * Resets all of the HDLC Counters in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_enc_reset_counters(uint32_t base);
#ifdef __cplusplus
}
#endif




#endif /* TSD_HDLC_ENCODER_2P0_H_ */
