/*
 * tsd_rx_irig_core.c
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */
#include <stdio.h>
#include "tsd_rx_irig_core.h"
#include "tsd_register_common.h"


/*
 * prints a summary of the core's setup to stdout
 */
void tsd_rx_irig_core_debug_print(uint32_t base)
{
	double fpgaVersion, driverVersion;
	printf("RX IRIG Core at 0x%X\n", (unsigned)base);
	if(0 == tsd_rx_irig_core_get_version(base, &fpgaVersion, &driverVersion))
	{
		printf(" - FPGA Version:  %f\n", fpgaVersion);
		printf(" - Driver Version:  %f\n", driverVersion);
	}
	else
	{
		printf(" - Version:  FAILED TO READ\n");
	}

	printf(" - REGISTERS:\n");
	printf("    => %02X: VERSION REG     = 0x%08X\n", TSD_RX_IRIG_VERSION_REG, TSD_RX_IRIG_IORD_VERSION(base));
	printf("    => %02X: CONTROL REG     = 0x%08X\n", TSD_RX_IRIG_CONTROL_REG, TSD_RX_IRIG_IORD_CONTROL(base));
	printf("    => %02X: STATUS REG      = 0x%08X\n", TSD_RX_IRIG_STATUS_REG, TSD_RX_IRIG_IORD_STATUS(base));
	printf("    => %02X: CODE TYPE REG   = 0x%08X\n", TSD_RX_IRIG_CODE_TYPE_REG, TSD_RX_IRIG_IORD_CODE_TYPE(base));
	printf("    => %02X: TIME LOW REG    = 0x%08X\n", TSD_RX_IRIG_TIME_L_REG, TSD_RX_IRIG_IORD_TIME_L(base));
	printf("    => %02X: TIME HIGH REG   = 0x%08X\n", TSD_RX_IRIG_TIME_H_REG, TSD_RX_IRIG_IORD_TIME_H(base));
	printf("    => %02X: DELAY CAL REG   = 0x%08X\n", TSD_RX_IRIG_DELAY_CAL_REG, TSD_RX_IRIG_IORD_DELAY_CAL(base));
}

/*
 * Gets the Telspan RX_IRIG core version
 *
 * returns 0 on success.
 */
int tsd_rx_irig_core_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
	uint32_t version = TSD_RX_IRIG_IORD_VERSION(base);
	uint32_t vH, vL;

	if(version == 0xDEADBEEF)
	{
		return 1;
	}
	*driverVersion = (double)TSD_RX_IRIG_CORE_DRIVER_VERSION;
	vH = (version >> 16);
	vL = (version & 0xFFFF);
	if(vL < 100)
		*fpgaVersion = (double)vH + (double)(vL/100);
	else
		*fpgaVersion = (double)vH + (double)(vL/1000);

	return 0;
}

/*
 * Enables the Telspan RX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rx_irig_core_enable(uint32_t base)
{
	return set_register_bits(base, TSD_RX_IRIG_CONTROL_REG, TSD_RX_IRIG_CONTROL_ENABLE);
}

/*
 * Enables the Telspan RX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rx_irig_core_disable(uint32_t base)
{
	return clear_register_bits(base, TSD_RX_IRIG_CONTROL_REG, TSD_RX_IRIG_CONTROL_ENABLE);
}

/*
 * Sets the RX_IRIG Signal type flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see tsd_rx_irig_signal_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_set_signal_type(uint32_t base, tsd_rx_irig_signal_type_t signalType)
{
	if(((int)signalType < 0) || ((int)signalType >= TSD_RX_IRIG_CODE_UNDEFINED))
	{
		return 1;
	}
	switch(signalType)
	{
	case TSD_RX_IRIG_SIGNAL_AM:
		set_register_bits(base, TSD_RX_IRIG_CONTROL_REG, TSD_RX_IRIG_CONTROL_AM_CODE );
		break;
	default:
		clear_register_bits(base, TSD_RX_IRIG_CONTROL_REG, TSD_RX_IRIG_CONTROL_AM_CODE );
		break;
	}
	return 0;
}

/*
 * Gets the RX_IRIG Signal type flags in the Telspan RX_IRIG core
 *
 * located at base address 'base'
 *
 * see tsd_rx_irig_signal_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_get_signal_type(uint32_t base, tsd_rx_irig_signal_type_t *signalType)
{
	uint32_t regVal = TSD_RX_IRIG_IORD_CONTROL(base);
	if(regVal == 0xDEADBEEF)
	{
		return 1;
	}
	if(regVal & TSD_RX_IRIG_CONTROL_AM_CODE)
	{
		*signalType = TSD_RX_IRIG_SIGNAL_AM;
	}
	else
	{
		*signalType = TSD_RX_IRIG_SIGNAL_TTL;
	}
	return 0;
}

/*
 * Sets the RX_IRIG Code type flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see tsd_rx_irig_code_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_set_code_type(uint32_t base, tsd_rx_irig_code_type_t codeType)
{
	if(((int)codeType < 0) || ((int)codeType >= TSD_RX_IRIG_CODE_UNDEFINED))
	{
		return 1;
	}
	TSD_RX_IRIG_IOWR_CODE_TYPE(base, (uint32_t)codeType);
	return 0;

}

/*
 * Gets the RX_IRIG Code type flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see tsd_rx_irig_code_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_get_code_type(uint32_t base, tsd_rx_irig_code_type_t *codeType)
{
	uint32_t regVal = TSD_RX_IRIG_IORD_CODE_TYPE(base);
	if(regVal == 0xDEADBEEF)
	{
		return 1;
	}
	if(((int)regVal < 0) || ((int)regVal >= TSD_RX_IRIG_CODE_UNDEFINED))
	{
		return 1;
	}
	*codeType = (tsd_rx_irig_code_type_t)regVal;
	return 0;
}

/*
 * Gets the RX_IRIG Status/Error flags in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * see TSD_RX_IRIG_STATUS_* definitions in tsd_rx_irig_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_get_status_flags(uint32_t base, uint32_t *statusFlags)
{
	uint32_t regVal = TSD_RX_IRIG_IORD_STATUS(base);
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
int tsd_rx_irig_core_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds)
{
	uint32_t timeL = TSD_RX_IRIG_IORD_TIME_L(base);
	uint32_t timeH = TSD_RX_IRIG_IORD_TIME_H(base);
	if((timeL == 0xDEADBEEF) || (timeH == 0xDEADBEEF))
	{
		return 1;
	}

	*milliseconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_RX_IRIG_TIME_L_BCD_MSEC_MASK) >> TSD_RX_IRIG_TIME_L_BCD_MSEC_RSHFT) );
	*seconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_RX_IRIG_TIME_L_BCD_SECONDS_MASK) >> TSD_RX_IRIG_TIME_L_BCD_SECONDS_RSHFT) );
	*minutes = bcd_u8_to_int( (uint8_t)((timeL & TSD_RX_IRIG_TIME_L_BCD_MINUTES_MASK) >> TSD_RX_IRIG_TIME_L_BCD_MINUTES_RSHFT) );
	*hours = bcd_u8_to_int( (uint8_t)((timeL & TSD_RX_IRIG_TIME_L_BCD_HOURS_MASK) >> TSD_RX_IRIG_TIME_L_BCD_HOURS_RSHFT) );
	*ydays = bcd_u16_to_int( (uint16_t)((timeH & TSD_RX_IRIG_TIME_H_BCD_DAYS_MASK) >> TSD_RX_IRIG_TIME_H_BCD_DAYS_RSHFT) );
	*years = 2000 +
			bcd_u8_to_int( (uint8_t)((timeH & TSD_RX_IRIG_TIME_H_BCD_YEARS_MASK) >> TSD_RX_IRIG_TIME_H_BCD_YEARS_RSHFT) );

	return 0;
}

/*
 * Gets the time from the RX_IRIG core as struct timespec.
 */
int tsd_rx_irig_core_get_time_ts(uint32_t base, struct timespec *ts)
{
	if(NULL == ts) return 1;
	uint32_t bcd_h = TSD_RX_IRIG_IORD_TIME_H(base);
	uint32_t bcd_l = TSD_RX_IRIG_IORD_TIME_L(base);

	get_timespec_from_bcd(bcd_l, bcd_h, ts);

	return 0;
}

/*
 * Gets the time from the RX_IRIG core as struct tm.
 */
int tsd_rx_irig_core_get_time_tm(uint32_t base, struct tm *tmtime)
{
	if(NULL == tmtime) return 1;
	uint32_t bcd_h = TSD_RX_IRIG_IORD_TIME_H(base);
	uint32_t bcd_l = TSD_RX_IRIG_IORD_TIME_L(base);

	get_tm_from_bcd(bcd_l, bcd_h, tmtime);

	return 0;
}

/*
 * Sets the RX_IRIG Delay calibration value in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * this value is used to calibrate out the delay through the input circuitry.
 * Value is in SYSTEM CLOCK ticks
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_set_delay_cal_value(uint32_t base, uint32_t dlyClkTicks)
{
	TSD_RX_IRIG_IOWR_DELAY_CAL(base, dlyClkTicks);
	return 0;
}

/*
 * Gets the RX_IRIG Delay calibration value  in the Telspan RX_IRIG core
 * located at base address 'base'
 *
 * this value is used to calibrate out the delay through the input circuitry.
 * Value is in SYSTEM CLOCK ticks
 *
 * returns 0 on success..
 */
int tsd_rx_irig_core_get_delay_cal_value(uint32_t base, uint32_t *dlyClkTicks)
{
	uint32_t regVal = TSD_RX_IRIG_IORD_DELAY_CAL(base);
	if(regVal == 0xDEADBEEF)
	{
		return 1;
	}
	*dlyClkTicks = regVal;
	return 0;
}
