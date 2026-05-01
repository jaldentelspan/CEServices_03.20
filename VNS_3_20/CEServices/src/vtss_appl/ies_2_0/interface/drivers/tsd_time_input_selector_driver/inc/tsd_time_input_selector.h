/*
 * tsd_time_input_selector.h
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */

#ifndef TSD_TIME_INPUT_SELECTOR_H_
#define TSD_TIME_INPUT_SELECTOR_H_

#include <stddef.h>
#include "stdint.h"
#include "tsd_time_bcd.h"
#include "tsd_time_input_selector_regs.h"

#define TSD_TIME_INPUT_SELECTOR_CORE_DRIVER_VERSION        (0.01)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_time_input_selector_debug_print(uint32_t base);

/*
 * Gets the Telspan TIME_INPUT_SELECTOR core version
 *
 * returns 0 on success.
 */
int tsd_time_input_selector_get_version(uint32_t base, double *version, double *driverVersion);

/*
 * Enables the Telspan TIME_INPUT_SELECTOR core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_time_input_selector_enable(uint32_t base);

/*
 * Enables the Telspan TIME_INPUT_SELECTOR core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_time_input_selector_disable(uint32_t base);

/*
 * Reset control for the time input selector core
 * resetOn = 1 : core in reset
 * resetOn = 0 : core not in reset.
 * In order to reset the core, set resetOn to 1, wait for at least 100us,
 * then set resetOn to 0.
 * Returns 0 on success
 */
int tsd_time_input_selector_reset(uint32_t base, int resetOn);

/*
 * Gets the currently selected channel (0 based).
 * Note that this changes automatically by the core based on lock status and priority.
 * returns 0 on success..
 */
int tsd_time_input_selector_get_selected_input(uint32_t base, uint8_t *sel);

/*
 * Gets the currently selected channel (0 based).
 * Note that this changes automatically by the core based on lock status and priority.
 * returns 0 on success..
 */
int tsd_time_input_selector_get_selected_input_type(uint32_t base, uint8_t *type);

/*
 * Sets the priority for the time input n (see system design for info about what is connected to n)
 * lower priority numbers indicate higher priority
 * Set prio to 0xF to disable time input.
 * returns 0 on success..
 */
int tsd_time_input_selector_set_input_prio(uint32_t base, uint8_t channel, uint8_t prio);

/*
 * Gets the priority for the time input n (see system design for info about what is connected to n)
 * returns 0 on success..
 */
int tsd_time_input_selector_get_input_prio(uint32_t base, uint8_t channel, uint8_t *prio);


/*
 * Gets the current time in the Telspan Time Input Selector core located at base address 'base'
 * years: last 2 digits of the year. Actual year = (2000 + years)
 * ydays: day in year (number of elapsed days in the current year.
 * hours: elapsed hours in the current day.
 * minutes: elapsed minutes in the current hour.
 * seconds: elapsed seconds in the current minute.
 * milliseconds: elapsed milliseconds in the current second (not used)
 *
 * returns 0 on success, 1 on failure, -1 on invalid (rx_irig not locked)
 */
int tsd_time_input_selector_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds);

/*
 * Gets the time from the Time Input Selector core as struct timespec.
 */
int tsd_time_input_selector_get_time_ts(uint32_t base, struct timespec *ts);

/*
 * Gets the time from the Time Input Selector core as struct tm.
 */
int tsd_time_input_selector_get_time_tm(uint32_t base, struct tm *tmtime);

#ifdef __cplusplus
}
#endif

#endif /* TSD_TIME_INPUT_SELECTOR_H_ */
