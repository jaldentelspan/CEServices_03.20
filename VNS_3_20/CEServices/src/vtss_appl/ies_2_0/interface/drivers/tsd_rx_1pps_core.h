/*
 * tsd_rx_1pps_core.h
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */

#ifndef TSD_RX_1PPS_CORE_H_
#define TSD_RX_1PPS_CORE_H_

#include <stddef.h>
/* #include <stdint.h> */
#include "main.h"
#include <time.h>
#include "tsd_rx_1pps_core_regs.h"

#define TSD_RX_1PPS_CORE_CORE_DRIVER_VERSION        (0.01)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_rx_1pps_core_debug_print(uint32_t base);

/*
 * Gets the Telspan RX_1PPS_CORE core version
 *
 * returns 0 on success.
 */
int tsd_rx_1pps_core_get_version(uint32_t base, double *version, double *driverVersion);

/*
 * Enables the Telspan RX_1PPS_CORE core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rx_1pps_core_enable(uint32_t base);

/*
 * Enables the Telspan RX_1PPS_CORE core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rx_1pps_core_disable(uint32_t base);

/*
 * Sets the Time Valid bit in the Telspan RX_1PPS_CORE core located at base address 'base'
 * This bit is ANDED with the 1PPS window validity to determine lock status.
 * If the Input is always running, this bit can be used to change the lock status of the 1PPS core.
 * For example, if used for IEEE-1588, software may know when the 1588 is locked to a master source, but
 * this core is unaware of the lock status, in this case this bit should be used to change the lock state
 * based on teh 1588 clock state.
 * Set to 1 for valid (locked) time.
 * set to 0 for invalid (unlocked) time.
 * returns 0 on success.
 */
int tsd_rx_1pps_core_time_valid(uint32_t base, int timeValid);

/*
 * Reset control for the time input selector core
 * resetOn = 1 : core in reset
 * resetOn = 0 : core not in reset.
 * In order to reset the core, set resetOn to 1, wait for at least 100us,
 * then set resetOn to 0.
 * Returns 0 on success
 */
int tsd_rx_1pps_core_reset(uint32_t base, int resetOn);

/*
 * Gets the 1PPS Status/Error flags in the Telspan 1PPS core
 * located at base address 'base'
 *
 * see TSD_1PPS_STATUS_* definitions in tsd_rx_1pps_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_rx_1pps_core_get_status_flags(uint32_t base, uint32_t *statusFlags);

/*
 * Gets the current time in the Telspan RX 1PPS core located at base address 'base'
 * years: current year
 * ydays: day in year (number of elapsed days in the current year).
 * hours: elapsed hours in the current day.
 * minutes: elapsed minutes in the current hour.
 * seconds: elapsed seconds in the current minute.
 * milliseconds: elapsed milliseconds in the current second (not used)
 *
 * returns 0 on success, 1 on failure, -1 on invalid (rx_irig not locked)
 */
int tsd_rx_1pps_core_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds);

/*
 * Sets the current time in the Telspan RX 1PPS core located at base address 'base'
 * years: current year
 * ydays: day in year (number of elapsed days in the current year).
 * hours: elapsed hours in the current day.
 * minutes: elapsed minutes in the current hour.
 * seconds: elapsed seconds in the current minute.
 * milliseconds: elapsed milliseconds in the current second (not used)
 *
 * returns 0 on success, 1 on failure, -1 on invalid (rx_irig not locked)
 */
int tsd_rx_1pps_core_set_time(uint32_t base, int years, int ydays, int hours, int minutes, int seconds, int milliseconds);

/*
 * Sets the time in the RX_1PPS_CORE using struct timespec.
 * This must be current with the second (written every second).
 */
int tsd_rx_1pps_core_set_time_ts(uint32_t base, struct timespec ts);

/*
 * Gets the time from the RX_1PPS_CORE as struct timespec.
 */
int tsd_rx_1pps_core_get_time_ts(uint32_t base, struct timespec *ts);

/*
 * Sets the time in the RX_1PPS_CORE using struct tm.  This must be current with the second (written every second).
 */
int tsd_rx_1pps_core_set_time_tm(uint32_t base, struct tm tmtime);

/*
 * Gets the time from the RX_1PPS_CORE as struct tm.
 */
int tsd_rx_1pps_core_get_time_tm(uint32_t base, struct tm *tmtime);

/*
 * Sets the 1PPS Window minimum value.
 */
int tsd_rx_1pps_core_set_window_min(uint32_t base, uint32_t winMin);

/*
 * Gets the 1PPS Window minimum value.
 */
int tsd_rx_1pps_core_get_window_min(uint32_t base, uint32_t *winMin);

/*
 * Sets the 1PPS Window maximum value.
 */
int tsd_rx_1pps_core_set_window_max(uint32_t base, uint32_t winMax);

/*
 * Gets the 1PPS Window maximum value.
 */
int tsd_rx_1pps_core_get_window_max(uint32_t base, uint32_t *winMax);

#ifdef __cplusplus
}
#endif

#endif /* TSD_RX_1PPS_CORE_H_ */
