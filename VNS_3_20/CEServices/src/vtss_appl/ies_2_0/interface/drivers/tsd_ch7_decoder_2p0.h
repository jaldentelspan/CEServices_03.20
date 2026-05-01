/*
 * tsd_ch7_decoder_2p0.h
 *
 *  Created on: Mar 31, 2020
 *      Author: eric
 */

#ifndef TSD_CH7_DECODER_2P0_H_
#define TSD_CH7_DECODER_2P0_H_

#include <stddef.h>
/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */
#include "tsd_ch7_decoder_2p0_regs.h"

#define TSD_CH7_DEC_DRIVER_VERSION        (2.0)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_ch7_dec_debug_print(uint32_t base);

/*
 * Gets the Telspan CH7 Decoder core version
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_get_version(uint32_t base, double *fpgaVersion, double *driverVersion);

/*
 * Enables the CH7 mode of the Telspan CH7 Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_enable(uint32_t base);

/*
 * Disables the Telspan CH7 Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_disable(uint32_t base);

/*
 * Sets other control bits in the Telspan CH7 Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_set_control_bits(uint32_t base, uint32_t ctrlBits);

/*
 * Clears control bits in the Telspan CH7 Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_ch7_dec_clear_control_bits(uint32_t base, uint32_t ctrlBits);

/*
 * Gets the CH7 decoder status.
 * returns 0 on success
 */
int tsd_ch7_dec_get_status(uint32_t base, uint32_t *status);

/*
 * Sets the CH7 frame size.
 * val is the number of 223 byte chunks in the frame (so bytes/frame = val*223)
 * val MUST be between 1 and 7
 * returns 0 on success
 */
int tsd_ch7_dec_set_frame_size(uint32_t base, int val);

/*
 * gets the CH7 frame size.
 * returns 0 on success
 */
int tsd_ch7_dec_get_frame_size(uint32_t base, int *val);

/*
 * Sets the CH7 decoder port select.
 * returns 0 on success
 */
int tsd_ch7_dec_set_port_select(uint32_t base, int sel, bool enable);

/*
 * Gets the CH7 decoder port select.
 * returns 0 on success
 */
int tsd_ch7_dec_get_port_select(uint32_t base, int *sel, bool *enable);

/*
 * Sets the CH7 decoder sync pattern.
 * returns 0 on success
 */
int tsd_ch7_dec_set_sync_pattern(uint32_t base, uint32_t syncPattern, int syncLen);

/*
 * Gets the CH7 decoder sync pattern.
 * returns 0 on success
 */
int tsd_ch7_dec_get_sync_pattern(uint32_t base, uint32_t *syncPattern, int *syncLen);

/*
 * Gets the FIFO underflow Count in the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_get_fifo_underflow_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the FIFO overflow Count in the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_get_fifo_overflow_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the number of frames sent to the TSE from the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_get_tse_send_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the number of ch10 frames sent from the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_get_ch10_send_count(uint32_t base, uint32_t *cnt);

/*
 * Resets all of the CH7 Counters in the Telspan CH7 Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_ch7_dec_reset_counters(uint32_t base);

#ifdef __cplusplus
}
#endif




#endif /* TSD_CH7_DECODER_2P0_H_ */
