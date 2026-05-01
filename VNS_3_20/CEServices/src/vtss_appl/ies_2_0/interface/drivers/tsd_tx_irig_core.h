/*
 * tsd_tx_irig_core.h
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */

#ifndef TSD_TX_IRIG_CORE_H_
#define TSD_TX_IRIG_CORE_H_

#include <stddef.h>
/* #include <stdint.h> */
/* #include <stdbool.h> */
#include "main.h"
#include <time.h>
#include "tsd_tx_irig_core_regs.h"

#define TSD_TX_IRIG_CORE_DRIVER_VERSION		(0.01)

typedef enum {
	TSD_TX_IRIG_CODE_B = 0,
	TSD_TX_IRIG_CODE_A,
	TSD_TX_IRIG_CODE_G,
	TSD_TX_IRIG_CODE_UNDEFINED
}tsd_tx_irig_code_type_t;

typedef enum {
    TSD_TX_IRIG_AM_AMPLITUDE_FULL = 0,
    TSD_TX_IRIG_AM_AMPLITUDE_HALF,
    TSD_TX_IRIG_AM_AMPLITUDE_QUARTER,
    TSD_TX_IRIG_AM_AMPLITUDE_EIGHTH,
    TSD_TX_IRIG_AM_AMPLITUDE_UNDEFINED
}tsd_tx_irig_am_amplitude;


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_tx_irig_core_debug_print(uint32_t base);

/*
 * Gets the Telspan TX_IRIG core version
 *
 * returns 0 on success.
 */
int tsd_tx_irig_core_get_version(uint32_t base, double *version, double *driverVersion);

/*
 * Enables the Telspan TX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_tx_irig_core_enable(uint32_t base);

/*
 * Enables the Telspan TX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_tx_irig_core_disable(uint32_t base);

/*
 * Enables the Telspan TX_IRIG core's rx loopback (output what is coming in on the rx)
 * returns 0 on success.
 */
int tsd_tx_irig_core_enable_rx_loopback(uint32_t base);

/*
 * Enables the Telspan TX_IRIG core's rx loopback (output what is coming in on the rx)
 * returns 0 on success.
 */
int tsd_tx_irig_core_disable_rx_loopback(uint32_t base);
/*
 * Enables the Telspan TX_IRIG core's RTC Cal function (attempts to stay aligned with RTC)
 * this is typically used for replay of data.
 * returns 0 on success.
 */
int tsd_tx_irig_core_enable_cal2rtc(uint32_t base);

/*
 * Disables the Telspan TX_IRIG core's RTC Cal function (attempts to stay aligned with RTC)
 * this is typically used for replay of data.
 * returns 0 on success.
 */
int tsd_tx_irig_core_disable_cal2rtc(uint32_t base);

/*
 * Sets the IRIG-AM amplitude.
 * setting = 0 enables Full Amplitude
 * setting = 1 enables 1/2 Amplitude
 * setting = 2 enables 1/4 Amplitude
 * setting = 3 enables 1/8 Amplitude
 * returns 0 on success.
 */
int tsd_tx_irig_core_am_amplitude(uint32_t base, tsd_tx_irig_am_amplitude setting);

/*
 * Sets the TX_IRIG Code type flags in the Telspan TX_IRIG core
 * located at base address 'base'
 *
 * see tsd_tx_irig_code_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_tx_irig_core_set_code_type(uint32_t base, tsd_tx_irig_code_type_t codeType);

/*
 * Gets the TX_IRIG Code type flags in the Telspan TX_IRIG core
 * located at base address 'base'
 *
 * see tsd_tx_irig_code_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_tx_irig_core_get_code_type(uint32_t base, tsd_tx_irig_code_type_t *codeType);

/*
 * Sets the DC 1PPS mode.
 * When on (set dc1ppsOn to true) the IRIG-DC output becomes a 1PPS output.
 * When off (set dc1ppsOn to false) the IRIG-DC output is IRIG DC Code.
 * returns 0 on success
 */
int tsd_tx_irig_core_set_dc_1pps(uint32_t base, bool dc1ppsOn);

/*
 * Gets the DC 1PPS mode.
 * When on (set dc1ppsOn to true) the IRIG-DC output becomes a 1PPS output.
 * When off (set dc1ppsOn to false) the IRIG-DC output is IRIG DC Code.
 * returns 0 on success
 */
int tsd_tx_irig_core_get_dc_1pps(uint32_t base, bool *dc1ppsOn);

/*
 * Gets the TX_IRIG Status/Error flags in the Telspan TX_IRIG core
 * located at base address 'base'
 *
 * see TSD_TX_IRIG_STATUS_* definitions in tsd_tx_irig_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_tx_irig_core_get_status_flags(uint32_t base, uint32_t *statusFlags);

/*
 * Sets the TX_IRIG Calibration
 * returns 0 on success.
 */
int tsd_tx_irig_core_set_cal(uint32_t base, uint32_t calMajor, uint32_t calMinor, uint32_t calFine);

/*
 * Gets the TX_IRIG Calibration
 * returns 0 on success.
 */
int tsd_tx_irig_core_get_cal(uint32_t base, uint32_t *calMajor, uint32_t *calMinor, uint32_t *calFine);

int tsd_tx_irig_core_get_time(uint32_t base, struct timespec* txtime);

uint64_t tsd_tx_irig_core_get_current_rtc(uint32_t base);

uint64_t tsd_tx_irig_core_get_latched_rtc(uint32_t base);

#ifdef __cplusplus
}
#endif

#endif /* TSD_TX_IRIG_CORE_H_ */
