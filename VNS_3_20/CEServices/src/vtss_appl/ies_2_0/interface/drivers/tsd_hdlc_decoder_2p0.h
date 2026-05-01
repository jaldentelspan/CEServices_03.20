/*
 * tsd_hdlc_decoder_2p0.h
 *
 *  Created on: Mar 31, 2020
 *      Author: eric
 */

#ifndef TSD_HDLC_DECODER_2P0_H_
#define TSD_HDLC_DECODER_2P0_H_

#include <stddef.h>
/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */
#include "tsd_hdlc_decoder_2p0_regs.h"

#define TSD_HDLC_DEC_DRIVER_VERSION        (2.0)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_hdlc_dec_debug_print(uint32_t base);

/*
 * Gets the Telspan HDLC2p0 core version
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_version(uint32_t base, double *fpgaVersion, double *driverVersion);

/*
 * Enables the HDLC mode of the Telspan HDLC2p0 core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_enable(uint32_t base);

/*
 * Disables the Telspan HDLC2p0 core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_disable(uint32_t base);

/*
 * Gets the status word from the Telspan HDLC Decoder core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_status(uint32_t base);

/*
 * Sets other control bits in the Telspan HDLC2p0 core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_set_control_bits(uint32_t base, uint32_t ctrlBits);

/*
 * Clears control bits in the Telspan HDLC2p0 core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_hdlc_dec_clear_control_bits(uint32_t base, uint32_t ctrlBits);

/*
 * Gets the packet count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_packet_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the FIFO0 underflow Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_fifo0_underflow_count(uint32_t base, uint32_t *cnt);
/*
 * Gets the FIFO0 underflow Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_fifo0_overflow_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the FIFO1 underflow Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_fifo1_underflow_count(uint32_t base, uint32_t *cnt);
/*
 * Gets the FIFO1 underflow Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_fifo1_overflow_count(uint32_t base, uint32_t *cnt);

/*
 * Gets the bit stuffing error Count in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_get_bitstuffing_error_count(uint32_t base, uint32_t *cnt);

/*
 * Resets all of the HDLC Counters in the Telspan HDLC Decoder core
 * located at base address 'base'
 * returns 0 on success.
 */
int tsd_hdlc_dec_reset_counters(uint32_t base);

#ifdef __cplusplus
}
#endif




#endif /* TSD_HDLC_DECODER_2P0_H_ */
