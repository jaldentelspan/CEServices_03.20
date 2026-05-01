/*
 * tsd_rtc_calibrator.h
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */

#ifndef TSD_RTC_CALIBRATOR_H_
#define TSD_RTC_CALIBRATOR_H_

#include <stddef.h>
/* #include <stdint.h> */
/* #include <stdbool.h> */
#include "main.h"
#include "tsd_rtc_calibrator_regs.h"

#define TSD_RTC_CALIBRATOR_CORE_DRIVER_VERSION		(0.01)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_rtc_calibrator_debug_print(uint32_t base);

/*
 * Gets the Telspan RTC_CALIBRATOR core version
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_get_version(uint32_t base, double *version, double *driverVersion);

/*
 * Enables the Telspan RTC_CALIBRATOR core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_enable(uint32_t base);

/*
 * Enables the Telspan RTC_CALIBRATOR core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_disable(uint32_t base);

/*
 * Reset control for the time input selector core
 * resetOn = 1 : core in reset
 * resetOn = 0 : core not in reset.
 * In order to reset the core, set resetOn to 1, wait for at least 100us,
 * then set resetOn to 0.
 * Returns 0 on success
 */
int tsd_rtc_calibrator_reset(uint32_t base, int resetOn);


/*
 * Enables the Telspan RTC_CALIBRATOR core's PID controller
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_pid_enable(uint32_t base);

/*
 * Disables the Telspan RTC_CALIBRATOR core's PID controller
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_pid_disable(uint32_t base);

/*
 * returns true if the PID controller is running (and the module is enabled)
 */
bool tsd_rtc_calibrator_pid_running(uint32_t base);

/*
 * Enable the Telspan RTC_CALIBRATOR core's 100MHZ Calibrator
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_100MHz_cal_enable(uint32_t base);

/*
 * Disables the Telspan RTC_CALIBRATOR core's 100MHZ Calibrator
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_100MHz_cal_disable(uint32_t base);

/*
 * returns true if the 100MHz CALIBRATOR is running (and the module is enabled)
 */
bool tsd_rtc_calibrator_100MHz_cal_running(uint32_t base);

/*
 * Gets the RTC Error in number of ticks.  This is the difference between the 10,000,000 and the actual
 * number of ticks between on time pulses (1PPS) from current time source
 * returns 0 on success..
 */
int tsd_rtc_calibrator_get_error_ticks(uint32_t base, int32_t *error_ticks);

/*
 * Gets the current RTC value in number of ticks.
 * returns 0 on success..
 */
int tsd_rtc_calibrator_get_current_rtc(uint32_t base, uint64_t *current_rtc);

/*
 * Gets the RTC value at the time of the last 1PPS in number of ticks.
 * returns 0 on success..
 */
int tsd_rtc_calibrator_get_latched_rtc(uint32_t base, uint64_t *latched_rtc);


#ifdef __cplusplus
}
#endif

#endif /* TSD_RTC_CALIBRATOR_H_ */
