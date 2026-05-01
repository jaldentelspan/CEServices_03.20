/*
 * tsd_manual_inc_time.c
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */
#include <stdio.h>
#include "tsd_manual_inc_time.h"
#include "tsd_register_common.h"


/*
 * prints a summary of the core's setup to stdout
 */
void tsd_manual_inc_time_debug_print(uint32_t base)
{
	double fpgaVersion, driverVersion;
	printf("Manual Increment Core at 0x%X\n", (unsigned)base);
	if(0 == tsd_manual_inc_time_get_version(base, &fpgaVersion, &driverVersion))
	{
		printf(" - FPGA Version:  %f\n", fpgaVersion);
		printf(" - Driver Version:  %f\n", driverVersion);
	}
	else
	{
		printf(" - Version:  FAILED TO READ\n");
	}

	printf(" - REGISTERS:\n");
	printf("    => %02X: VERSION REG           = 0x%08X\n", TSD_MANUAL_INC_VERSION_REG, TSD_MANUAL_INC_IORD_VERSION(base));
	printf("    => %02X: CONTROL REG           = 0x%08X\n", TSD_MANUAL_INC_CONTROL_REG, TSD_MANUAL_INC_IORD_CONTROL(base));
	printf("    => %02X: STATUS REG            = 0x%08X\n", TSD_MANUAL_INC_STATUS_REG, TSD_MANUAL_INC_IORD_STATUS(base));
	printf("    => %02X: TIME LOW REG          = 0x%08X\n", TSD_MANUAL_INC_TIME_L_REG, TSD_MANUAL_INC_IORD_TIME_L(base));
	printf("    => %02X: TIME HIGH REG         = 0x%08X\n", TSD_MANUAL_INC_TIME_H_REG, TSD_MANUAL_INC_IORD_TIME_H(base));
	printf("    => %02X: TIME RESET OFFSET REG = 0x%08X\n", TSD_MANUAL_INC_TIME_RESET_OFFSET, TSD_MANUAL_INC_TIME_IORD_RESET_OFFSET(base));
	printf("    => %02X: LATCHED RTC L REG     = 0x%08X\n", TSD_MANUAL_INC_LATCHED_RTC_L_REG, TSD_MANUAL_INC_IORD_LATCHED_RTC_LSW(base));
	printf("    => %02X: LATCHED RTC H REG     = 0x%08X\n", TSD_MANUAL_INC_LATCHED_RTC_H_REG, TSD_MANUAL_INC_IORD_LATCHED_RTC_MSW(base));

}

/*
 * Gets the Telspan RX_IRIG core version
 *
 * returns 0 on success.
 */
int tsd_manual_inc_time_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
	uint32_t version = TSD_MANUAL_INC_IORD_VERSION(base);
	uint32_t vH, vL;

	if(version == 0xDEADBEEF)
	{
		return 1;
	}
	*driverVersion = (double)TSD_MANUAL_INC_TIME_DRIVER_VERSION;
	vH = (version >> 16);
	vL = (version & 0xFFFF);
	if(vL < 100)
		*fpgaVersion = (double)vH + (double)(vL/100);
	else
		*fpgaVersion = (double)vH + (double)(vL/1000);

	return 0;
}

/*
 * Enables the Telspan MANUAL_INC_TIME core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_manual_inc_time_enable(uint32_t base)
{
	return set_register_bits(base, TSD_MANUAL_INC_CONTROL_REG, TSD_MANUAL_INC_CONTROL_ENABLE);
}

/*
 * Enables the Telspan RX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_manual_inc_time_disable(uint32_t base)
{
	return clear_register_bits(base, TSD_MANUAL_INC_CONTROL_REG, TSD_MANUAL_INC_CONTROL_ENABLE);
}


/*
 * Gets the RX_IRIG Status/Error flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see TSD_MANUAL_INC_STATUS_* definitions in tsd_manual_inc_time_registers.h
 *
 * returns 0 on success..
 */
int tsd_manual_inc_time_get_status_flags(uint32_t base, uint32_t *statusFlags)
{
	uint32_t regVal = TSD_MANUAL_INC_IORD_STATUS(base);
	if(regVal == 0xDEADBEEF)
	{
		return 1;
	}
	*statusFlags = regVal;
	return 0;
}

/*
 * Gets the current time in the Telspan RX_IRIG core located at base address 'base'
 * years: current year
 * ydays: day in year (number of elapsed days in the current year).
 * hours: elapsed hours in the current day.
 * minutes: elapsed minutes in the current hour.
 * seconds: elapsed seconds in the current minute.
 * milliseconds: elapsed milliseconds in the current second (not used)
 *
 * returns 0 on success, 1 on failure, -1 on invalid (rx_irig not locked)
 */
int tsd_manual_inc_time_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds)
{
	uint32_t timeL = TSD_MANUAL_INC_IORD_TIME_L(base);
	uint32_t timeH = TSD_MANUAL_INC_IORD_TIME_H(base);
	if((timeL == 0xDEADBEEF) || (timeH == 0xDEADBEEF))
	{
		return 1;
	}

	*milliseconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_MANUAL_INC_TIME_L_MILLISECONDS_MASK) >> TSD_MANUAL_INC_TIME_L_MILLISECONDS_SHFT) );
	*seconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_MANUAL_INC_TIME_L_SECONDS_MASK) >> TSD_MANUAL_INC_TIME_L_SECONDS_SHFT) );
	*minutes = bcd_u8_to_int( (uint8_t)((timeL & TSD_MANUAL_INC_TIME_L_MINUTES_MASK) >> TSD_MANUAL_INC_TIME_L_MINUTES_SHFT) );
	*hours = bcd_u8_to_int( (uint8_t)((timeL & TSD_MANUAL_INC_TIME_L_HOURS_MASK) >> TSD_MANUAL_INC_TIME_L_HOURS_SHFT) );
	*ydays = bcd_u16_to_int( (uint16_t)((timeH & TSD_MANUAL_INC_TIME_H_DAYS_MASK) >> TSD_MANUAL_INC_TIME_H_DAYS_SHFT) );
	*years = 2000 +
			bcd_u8_to_int( (uint8_t)((timeH & TSD_MANUAL_INC_TIME_H_YEARS_MASK) >> TSD_MANUAL_INC_TIME_H_YEARS_SHFT) );

	return 0;
}

/*
 * Gets the time from the RX_IRIG core as struct timespec.
 */
int tsd_manual_inc_time_get_time_ts(uint32_t base, struct timespec *ts)
{
	if(NULL == ts) return 1;
	uint32_t bcd_h = TSD_MANUAL_INC_IORD_TIME_H(base);
	uint32_t bcd_l = TSD_MANUAL_INC_IORD_TIME_L(base);

	get_timespec_from_bcd(bcd_l, bcd_h, ts);

	return 0;
}

/*
 * Gets the time from the RX_IRIG core as struct tm.
 */
int tsd_manual_inc_time_get_time_tm(uint32_t base, struct tm *tmtime)
{
	if(NULL == tmtime) return 1;
	uint32_t bcd_h = TSD_MANUAL_INC_IORD_TIME_H(base);
	uint32_t bcd_l = TSD_MANUAL_INC_IORD_TIME_L(base);

	get_tm_from_bcd(bcd_l, bcd_h, tmtime);

	return 0;
}

/*
 * Sets the maximum offset (in rtc ticks) that can exist between input and output pulse before the
 * manual increment engine resets and abruptly re-synchronizes them (this should only matter while re-locking to a source).
 */
int tsd_manual_inc_time_set_reset_offset(uint32_t base, uint32_t rtcTicks)
{
	TSD_MANUAL_INC_TIME_IOWR_RESET_OFFSET(base, rtcTicks);
	return 0;
}

/*
 * Gets the maximum offset (in rtc ticks) that can exist between input and output pulse before the
 * manual increment engine resets and abruptly re-synchronizes them (this should only matter while re-locking to a source).
 */
int tsd_manual_inc_time_get_reset_offset(uint32_t base, uint32_t *rtcTicks)
{
	*rtcTicks = TSD_MANUAL_INC_TIME_IORD_RESET_OFFSET(base);
	return 0;
}

/*
 * Gets the RTC value at the time of the last 1PPS in number of ticks.
 * returns 0 on success..
 */
int tsd_manual_inc_time_get_latched_rtc(uint32_t base, uint64_t *latched_rtc)
{
	uint32_t rtcH, rtcL;
	rtcH = TSD_MANUAL_INC_IORD_LATCHED_RTC_MSW(base);
	rtcL = TSD_MANUAL_INC_IORD_LATCHED_RTC_LSW(base);

	if((0xDEADBEEF == rtcH) || (0xDEADBEEF == rtcL))
	{
		return 1;
	}

	*latched_rtc = ((uint64_t)rtcH << 32) | (uint64_t)rtcL;

	return 0;
}
