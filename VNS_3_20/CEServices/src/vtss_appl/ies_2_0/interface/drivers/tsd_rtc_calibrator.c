/*
 * tsd_rtc_calibrator.c
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */
#include <stdio.h>
#include "tsd_rtc_calibrator.h"
#include "tsd_register_common.h"


/*
 * prints a summary of the core's setup to stdout
 */
void tsd_rtc_calibrator_debug_print(uint32_t base)
{
	double fpgaVersion, driverVersion;
	printf("TSD RTC Calibrator Core at 0x%X\n", (unsigned)base);
	if(0 == tsd_rtc_calibrator_get_version(base, &fpgaVersion, &driverVersion))
	{
		printf(" - FPGA Version:  %f\n", fpgaVersion);
		printf(" - Driver Version:  %f\n", driverVersion);
	}
	else
	{
		printf(" - Version:  FAILED TO READ\n");
	}

	printf(" - REGISTERS:\n");
	printf("    => %02X: VERSION REG     = 0x%08X\n", TSD_RTC_CALIBRATOR_VERSION_REG, TSD_RTC_CALIBRATOR_IORD_VERSION(base));
	printf("    => %02X: CONTROL REG     = 0x%08X\n", TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_IORD_CONTROL(base));
	printf("    => %02X: STATUS  REG     = 0x%08X\n", TSD_RTC_CALIBRATOR_STATUS_REG, TSD_RTC_CALIBRATOR_IORD_STATUS(base));
	printf("    => %02X: ERROR TICKS REG = 0x%08X\n", TSD_RTC_CALIBRATOR_ERROR_TICKS_REG, TSD_RTC_CALIBRATOR_IORD_ERROR_TICKS(base));
	printf("    => %02X: CURRENT RTC_H   = 0x%08X\n", TSD_RTC_CALIBRATOR_CURRENT_RTC_H_REG, TSD_RTC_CALIBRATOR_IORD_CURRENT_RTC_H(base));
	printf("    => %02X: CURRENT RTC_L   = 0x%08X\n", TSD_RTC_CALIBRATOR_CURRENT_RTC_L_REG, TSD_RTC_CALIBRATOR_IORD_CURRENT_RTC_L(base));
	printf("    => %02X: LATCHED RTC_H   = 0x%08X\n", TSD_RTC_CALIBRATOR_LATCHED_RTC_H_REG, TSD_RTC_CALIBRATOR_IORD_LATCHED_RTC_H(base));
	printf("    => %02X: LATCHED RTC_L   = 0x%08X\n", TSD_RTC_CALIBRATOR_LATCHED_RTC_L_REG, TSD_RTC_CALIBRATOR_IORD_LATCHED_RTC_L(base));
}

/*
 * Gets the Telspan RTC_CALIBRATOR core version
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
	uint32_t version = TSD_RTC_CALIBRATOR_IORD_VERSION(base);
	uint32_t vH, vL;

	if(version == 0xDEADBEEF)
	{
		return 1;
	}
	*driverVersion = (double)TSD_RTC_CALIBRATOR_CORE_DRIVER_VERSION;
	vH = (version >> 16);
	vL = (version & 0xFFFF);
	if(vL < 100)
		*fpgaVersion = (double)vH + (double)(vL/100);
	else
		*fpgaVersion = (double)vH + (double)(vL/1000);

	return 0;
}

/*
 * Enables the Telspan RTC_CALIBRATOR core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_enable(uint32_t base)
{
	return set_register_bits(base, TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_CONTROL_ENABLE);
}

/*H
 * Enables the Telspan RTC_CALIBRATOR core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_disable(uint32_t base)
{
	return clear_register_bits(base, TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_CONTROL_ENABLE);
}

/*
 * Enables the Telspan RTC_CALIBRATOR core's PID controller
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_pid_enable(uint32_t base)
{
	return set_register_bits(base, TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_CONTROL_PID_ENABLE);
}

/*
 * Disables the Telspan RTC_CALIBRATOR core's PID controller
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_pid_disable(uint32_t base)
{
	return clear_register_bits(base, TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_CONTROL_PID_ENABLE);
}

/*
 * returns true if the PID controller is running (and the module is enabled)
 */
bool tsd_rtc_calibrator_pid_running(uint32_t base)
{
	uint32_t val = TSD_RTC_CALIBRATOR_IORD_CONTROL(base);
	val &= (TSD_RTC_CALIBRATOR_CONTROL_ENABLE | TSD_RTC_CALIBRATOR_CONTROL_PID_ENABLE);
	return  (val == (TSD_RTC_CALIBRATOR_CONTROL_ENABLE | TSD_RTC_CALIBRATOR_CONTROL_PID_ENABLE));
}

/*
 * Enable the Telspan RTC_CALIBRATOR core's 100MHZ Calibrator
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_100MHz_cal_enable(uint32_t base)
{
	return set_register_bits(base, TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_CONTROL_100MHZ_CAL_ENABLE);
}

/*
 * Disables the Telspan RTC_CALIBRATOR core's 100MHZ Calibrator
 *
 * returns 0 on success.
 */
int tsd_rtc_calibrator_100MHz_cal_disable(uint32_t base)
{
	return clear_register_bits(base, TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_CONTROL_100MHZ_CAL_ENABLE);
}

/*
 * returns true if the 100MHz CALIBRATOR is running (and the module is enabled)
 */
bool tsd_rtc_calibrator_100MHz_cal_running(uint32_t base)
{
	uint32_t val = TSD_RTC_CALIBRATOR_IORD_CONTROL(base);
	val &= (TSD_RTC_CALIBRATOR_CONTROL_ENABLE | TSD_RTC_CALIBRATOR_CONTROL_100MHZ_CAL_ENABLE);
	return  (val == (TSD_RTC_CALIBRATOR_CONTROL_ENABLE | TSD_RTC_CALIBRATOR_CONTROL_100MHZ_CAL_ENABLE));
}


/*
 * Reset control for the time input selector core
 * resetOn = 1 : core in reset
 * resetOn = 0 : core not in reset.
 * In order to reset the core, set resetOn to 1, wait for at least 100us,
 * then set resetOn to 0.
 * Returns 0 on success
 */
int tsd_rtc_calibrator_reset(uint32_t base, int resetOn)
{
	if(resetOn)
		return set_register_bits(base, TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_CONTROL_RESET);
	else
		return clear_register_bits(base, TSD_RTC_CALIBRATOR_CONTROL_REG, TSD_RTC_CALIBRATOR_CONTROL_RESET);
}


/*
 * Gets the RTC Error in number of ticks.  This is the difference between the 10,000,000 and the actual
 * number of ticks between on time pulses (1PPS) from current time source
 * returns 0 on success..
 */
int tsd_rtc_calibrator_get_error_ticks(uint32_t base, int32_t *error_ticks)
{
	*error_ticks = TSD_RTC_CALIBRATOR_IORD_ERROR_TICKS(base);
	if((*error_ticks) == 0xDEADBEEF)
		return 1;
	return 0;
}

/*
 * Gets the current RTC value in number of ticks.
 * returns 0 on success..
 */
int tsd_rtc_calibrator_get_current_rtc(uint32_t base, uint64_t *current_rtc)
{
	uint32_t rtcH, rtcL;
	rtcH = TSD_RTC_CALIBRATOR_IORD_CURRENT_RTC_H(base);
	rtcL = TSD_RTC_CALIBRATOR_IORD_CURRENT_RTC_L(base);

	if((0xDEADBEEF == rtcH) || (0xDEADBEEF == rtcL))
	{
		return 1;
	}

	*current_rtc = ((uint64_t)rtcH << 32) | (uint64_t)rtcL;

	return 0;
}

/*
 * Gets the RTC value at the time of the last 1PPS in number of ticks.
 * returns 0 on success..
 */
int tsd_rtc_calibrator_get_latched_rtc(uint32_t base, uint64_t *latched_rtc)
{
	uint32_t rtcH, rtcL;
	rtcH = TSD_RTC_CALIBRATOR_IORD_LATCHED_RTC_H(base);
	rtcL = TSD_RTC_CALIBRATOR_IORD_LATCHED_RTC_L(base);

	if((0xDEADBEEF == rtcH) || (0xDEADBEEF == rtcL))
	{
		return 1;
	}

	*latched_rtc = ((uint64_t)rtcH << 32) | (uint64_t)rtcL;

	return 0;
}
