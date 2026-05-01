/*
 * tsd_gps_core.h
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */

#ifndef TSD_GPS_CORE_H_
#define TSD_GPS_CORE_H_

#include <stddef.h>
/* #include "stdint.h" */
#include "main.h"
#include "tsd_time_bcd.h"
#include "tsd_gps_core_regs.h"

#define TSD_GPS_CORE_DRIVER_VERSION		(0.01)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_gps_core_debug_print(uint32_t base);

/*
 * Gets the Telspan GPS core version
 *
 * returns 0 on success.
 */
int tsd_gps_core_get_version(uint32_t base, double *version, double *driverVersion);

/*
 * Enables the Telspan GPS core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_gps_core_enable(uint32_t base);

/*
 * Enables the Telspan GPS core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_gps_core_disable(uint32_t base);

/*
 * Sets the GPS Serial Baud rate in the Telspan GPS core
 * located at base address 'base'
 * module_freq should be set to the system.h _FREQ value for this module
 * returns 0 on success.
 */
int tsd_gps_core_set_serial_baudrate(uint32_t base, int module_freq, int baud);

/*
 * Gets the GPS Serial Baud rate in the Telspan GPS core
 * located at base address 'base'
 * module_freq should be set to the system.h _FREQ value for this module
 * returns 0 on success.
 */
int tsd_gps_core_get_serial_baudrate(uint32_t base, int module_freq, int *baud);

/*
 * Sets the GPS Power Flags in the Telspan GPS core
 * located at base address 'base'
 *
 * Valid flags are defined in tsd_gps_core_registers.h
 *  - TSD_GPS_CONTROL_PWR_* and TSD_GPS_CONTROL_ANT_*
 *
 *  This function sets all of the power flags to the value in pwrFlags
 *
 * returns 0 on success.
 */
int tsd_gps_core_set_power_flags(uint32_t base, uint32_t pwrFlags);

/*
 * Sets individual bits in the GPS Power Flags in the Telspan GPS core
 * located at base address 'base'
 *
 * Valid flags are defined in tsd_gps_core_registers.h
 *  - TSD_GPS_CONTROL_PWR_* and TSD_GPS_CONTROL_ANT_*
 *
 * Unlike tsd_gps_core_set_power_flags, this function does not change the
 * state of any other flag... it only sets the additional bits in pwrFlagBits.
 *
 * Note: TSD_GPS_CONTROL_ANT_3P3V and TSD_GPS_CONTROL_ANT_5P0V may not be set at the same time.
 *       If an attempt is made to set one while the other is set, the new setting will be set
 *       and the old setting will be cleared.
 *       If an attempt is made to simultaneously set both, no power changes will be made and
 *       this function will return non-zero (1).
 *
 * returns 0 on success.
 */
int tsd_gps_core_set_power_flags_bits(uint32_t base, uint32_t pwrFlagBits);

/*
 * Clears individual bits in the GPS Power Flags in the Telspan GPS core
 * located at base address 'base'
 *
 * Valid flags are defined in tsd_gps_core_registers.h
 *  - TSD_GPS_CONTROL_PWR_* and TSD_GPS_CONTROL_ANT_*
 *
 * returns 0 on success.
 */
int tsd_gps_core_clear_power_flags_bits(uint32_t base, uint32_t pwrFlagBits);

/*
 * Gets the GPS Power Flags in the Telspan GPS core
 * located at base address 'base'
 *
 * see TSD_GPS_CONTROL_PWR_* and TSD_GPS_CONTROL_ANT_* definitions in tsd_gps_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_gps_core_get_power_flags(uint32_t base, uint32_t *pwrFlags);

/*
 * Gets the GPS Status/Error flags in the Telspan GPS core
 * located at base address 'base'
 *
 * see TSD_GPS_STATUS_* definitions in tsd_gps_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_gps_core_get_status_flags(uint32_t base, uint32_t *statusFlags);

/*
 * Sets the GPS Current Leap Second Value in the Telspan GPS core
 * located at base address 'base'
 *
 * This value is used to 'seed' the GPS receiver with the correct
 * leap second before it receives the actual value from the satellites.
 * The Poly message will contain the actual value received from the
 * satellites (once it has been received, up to 12.5 minutes after lock)
 *
 * Note: this value will be written to the GPS by the core unless
 * 		 software control of the GPS is enabled.  If software control
 * 		 of the GPS is enabled, it is the responsibility of the software
 * 		 to setup the current leapsecond value in the receiver and this
 * 		 register is purely informational.
 *
 * returns 0 on success..
 */
int tsd_gps_core_set_current_leapsecond(uint32_t base, uint8_t leapsecondVal);

/*
 * Gets the GPS Current Leap Second Value in the Telspan GPS core
 * located at base address 'base'
 *
 * returns 0 on success..
 */
int tsd_gps_core_get_current_leapsecond(uint32_t base, uint8_t *leapsecondVal);

/*
 * Gets the GPS Poly Leap Second Value in the Telspan GPS core
 * located at base address 'base'
 *
 * This is the actual leapsecond value received from the GPS satellites.
 * If the POLY message has not been received by the core, the value of
 * this message will be invalid... in this case the function returns -1.
 *
 * returns 0 on success, 1 on failure, -1 if invalid
 */
int tsd_gps_core_get_poly_leapsecond(uint32_t base, uint8_t *polyLeapsecondVal);

/*
 * Gets the current time in the Telspan GPS core located at base address 'base'
 * years: last 2 digits of the year. Actual year = (2000 + years)
 * ydays: day in year (number of elapsed days in the current year.
 * hours: elapsed hours in the current day.
 * minutes: elapsed minutes in the current hour.
 * seconds: elapsed seconds in the current minute.
 * milliseconds: elapsed milliseconds in the current second (not used)
 *
 * returns 0 on success, 1 on failure, -1 on invalid (gps not locked)
 */
int tsd_gps_core_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds);

/*
 * Gets the time from the GPS core as struct timespec.
 */
int tsd_gps_core_get_time_ts(uint32_t base, struct timespec *ts);

/*
 * Gets the time from the GPS core as struct tm.
 */
int tsd_gps_core_get_time_tm(uint32_t base, struct tm *tmtime);

/*
 * Sets the minimum value for 1PPS detect window in the Telspan GPS core
 * located at base address 'base'.
 *
 * Value is the minimum number of clock ticks (at the clock rate of the core) that must elapse
 * between 1PPS Signals.
 * The 1PPS signal must be seen within the "Window" in order to declare lock.
 *
 * Note: the clock rate of the core is typically == ALT_CPU_FREQ unless there is a clock-crossing
 *       bridge in between the CPU and the GPS core.
 *
 * returns 0 on success.
 */
int tsd_gps_core_set_pps_window_min(uint32_t base, uint32_t ppsWindowMin);

/*
 * Sets the maximum value for 1PPS detect window in the Telspan GPS core
 * located at base address 'base'.
 *
 * Value is the maximum number of clock ticks (at the clock rate of the core) that must elapse
 * between 1PPS Signals.
 * The 1PPS signal must be seen within the "Window" in order to declare lock.
 *
 * Note: the clock rate of the core is typically == ALT_CPU_FREQ unless there is a clock-crossing
 *       bridge in between the CPU and the GPS core.
 *
 * returns 0 on success.
 */
int tsd_gps_core_set_pps_window_max(uint32_t base, uint32_t ppsWindowMax);

/*
 * Gets the minimum value for 1PPS detect window in the Telspan GPS core
 * located at base address 'base'.
 *
 * returns 0 on success.
 */
int tsd_gps_core_get_pps_window_min(uint32_t base, uint32_t *ppsWindowMin);

/*
 * Gets the maximum value for 1PPS detect window in the Telspan GPS core
 * located at base address 'base'.
 *
 * returns 0 on success.
 */
int tsd_gps_core_get_pps_window_max(uint32_t base, uint32_t *ppsWindowMax);

/*
 * Gets the measured 1PPS tick count for the last tick
 * in the Telspan GPS core located at base address 'base'.
 *
 * returns 0 on success.
 */
int tsd_gps_core_get_pps_measured_tick_delta(uint32_t base, uint32_t *ppsDeltaTick);

/*
 * Gets the lost lock counter from the Telspan GPS core and places it into
 * the integer pointed to by count.
 *
 * returns 0 on success..
 */
int tsd_gps_core_get_lost_lock_count(uint32_t base, uint32_t *count);

/*
 * Resets the lost lock counter to 0.
 * returns 0 on success..
 */
int tsd_gps_core_reset_lost_lock_count(uint32_t base);

#ifdef __cplusplus
}
#endif

#endif /* TSD_GPS_CORE_H_ */
