/*
 * tsd_manual_inc_time.h
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */

#ifndef TSD_MANUAL_INC_TIME_H_
#define TSD_MANUAL_INC_TIME_H_

#include <stddef.h>
/* #include "stdint.h" */
#include "main.h"
#include "tsd_time_bcd.h"
#include "tsd_manual_inc_time_regs.h"

#define TSD_MANUAL_INC_TIME_DRIVER_VERSION		(0.01)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_manual_inc_time_debug_print(uint32_t base);

/*
 * Gets the Telspan MANUAL_INC_TIME core version
 *
 * returns 0 on success.
 */
int tsd_manual_inc_time_get_version(uint32_t base, double *version, double *driverVersion);

/*
 * Enables the Telspan MANUAL_INC_TIME core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_manual_inc_time_enable(uint32_t base);

/*
 * Enables the Telspan MANUAL_INC_TIME core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_manual_inc_time_disable(uint32_t base);

/*
 * Gets the MANUAL_INC_TIME Status/Error flags in the Telspan MANUAL_INC_TIME core
 * located at base address 'base'
 *
 * see TSD_MANUAL_INC_STATUS_* definitions in tsd_manual_inc_time_registers.h
 *
 * returns 0 on success..
 */
int tsd_manual_inc_time_get_status_flags(uint32_t base, uint32_t *statusFlags);

/*
 * Gets the current time in the Telspan MANUAL_INC_TIME core located at base address 'base'
 * years: last 2 digits of the year. Actual year = (2000 + years)
 * ydays: day in year (number of elapsed days in the current year.
 * hours: elapsed hours in the current day.
 * minutes: elapsed minutes in the current hour.
 * seconds: elapsed seconds in the current minute.
 * milliseconds: elapsed milliseconds in the current second (not used)
 *
 * returns 0 on success, 1 on failure, -1 on invalid (rx_irig not locked)
 */
int tsd_manual_inc_time_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds);

/*
 * Gets the time from the MANUAL_INC_TIME core as struct timespec.
 */
int tsd_manual_inc_time_get_time_ts(uint32_t base, struct timespec *ts);

/*
 * Gets the time from the MANUAL_INC_TIME core as struct tm.
 */
int tsd_manual_inc_time_get_time_tm(uint32_t base, struct tm *tmtime);

/*
 * Sets the maximum offset (in rtc ticks) that can exist between input and output pulse before the
 * manual increment engine resets and abruptly re-synchronizes them (this should only matter while re-locking to a source).
 */
int tsd_manual_inc_time_set_reset_offset(uint32_t base, uint32_t rtcTicks);

/*
 * Gets the maximum offset (in rtc ticks) that can exist between input and output pulse before the
 * manual increment engine resets and abruptly re-synchronizes them (this should only matter while re-locking to a source).
 */
int tsd_manual_inc_time_get_reset_offset(uint32_t base, uint32_t *rtcTicks);


/*
 * Gets the RTC value at the time of the last 1PPS in number of ticks.
 * returns 0 on success..
 */
int tsd_manual_inc_time_get_latched_rtc(uint32_t base, uint64_t *latched_rtc);

#ifdef __cplusplus
}
#endif

#endif /* TSD_MANUAL_INC_TIME_H_ */
