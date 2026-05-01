/*
 * tsd_rx_irig_core.h
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */

#ifndef TSD_RX_IRIG_CORE_H_
#define TSD_RX_IRIG_CORE_H_

#include <stddef.h>
/* #include "stdint.h" */
#include "main.h"
#include "tsd_time_bcd.h"
#include "tsd_rx_irig_core_regs.h"

#define TSD_RX_IRIG_CORE_DRIVER_VERSION		(0.01)

typedef enum {
	TSD_RX_IRIG_CODE_B = 0,
	TSD_RX_IRIG_CODE_A,
	TSD_RX_IRIG_CODE_G,
	TSD_RX_IRIG_CODE_UNDEFINED
}tsd_rx_irig_code_type_t;

typedef enum {
	TSD_RX_IRIG_SIGNAL_TTL = 0,
	TSD_RX_IRIG_SIGNAL_AM,
	TSD_RX_IRIG_SIGNAL_UNDEFINED
}tsd_rx_irig_signal_type_t;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_rx_irig_core_debug_print(uint32_t base);

/*
 * Gets the Telspan RX_IRIG core version
 *
 * returns 0 on success.
 */
int tsd_rx_irig_core_get_version(uint32_t base, double *version, double *driverVersion);

/*
 * Enables the Telspan RX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rx_irig_core_enable(uint32_t base);

/*
 * Enables the Telspan RX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rx_irig_core_disable(uint32_t base);

/*
 * Sets the RX_IRIG Signal type flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see tsd_rx_irig_signal_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_set_signal_type(uint32_t base, tsd_rx_irig_signal_type_t signalType);

/*
 * Gets the RX_IRIG Signal type flags in the Telspan RX_IRIG core
 *
 * located at base address 'base'
 *
 * see tsd_rx_irig_signal_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_get_signal_type(uint32_t base, tsd_rx_irig_signal_type_t *signalType);

/*
 * Sets the RX_IRIG Code type flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see tsd_rx_irig_code_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_set_code_type(uint32_t base, tsd_rx_irig_code_type_t codeType);

/*
 * Gets the RX_IRIG Code type flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see tsd_rx_irig_code_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_get_code_type(uint32_t base, tsd_rx_irig_code_type_t *codeType);

/*
 * Gets the RX_IRIG Status/Error flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see TSD_RX_IRIG_STATUS_* definitions in tsd_rx_irig_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_get_status_flags(uint32_t base, uint32_t *statusFlags);

/*
 * Gets the current time in the Telspan RX_IRIG core located at base address 'base'
 * years: last 2 digits of the year. Actual year = (2000 + years)
 * ydays: day in year (number of elapsed days in the current year.
 * hours: elapsed hours in the current day.
 * minutes: elapsed minutes in the current hour.
 * seconds: elapsed seconds in the current minute.
 * milliseconds: elapsed milliseconds in the current second (not used)
 *
 * returns 0 on success, 1 on failure, -1 on invalid (rx_irig not locked)
 */
int tsd_rx_irig_core_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds);

/*
 * Gets the time from the RX_IRIG core as struct timespec.
 */
int tsd_rx_irig_core_get_time_ts(uint32_t base, struct timespec *ts);

/*
 * Gets the time from the RX_IRIG core as struct tm.
 */
int tsd_rx_irig_core_get_time_tm(uint32_t base, struct tm *tmtime);

/*
 * Sets the RX_IRIG Delay calibration value in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * this value is used to calibrate out the delay through the input circuitry.
 * Value is in SYSTEM CLOCK ticks
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_set_delay_cal_value(uint32_t base, uint32_t dlyClkTicks);

/*
 * Gets the RX_IRIG Delay calibration value  in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * this value is used to calibrate out the delay through the input circuitry.
 * Value is in SYSTEM CLOCK ticks
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_get_delay_cal_value(uint32_t base, uint32_t *dlyClkTicks);

#ifdef __cplusplus
}
#endif

#endif /* TSD_RX_IRIG_CORE_H_ */
