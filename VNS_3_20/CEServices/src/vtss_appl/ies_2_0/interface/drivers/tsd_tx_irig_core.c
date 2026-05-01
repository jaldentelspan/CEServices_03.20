/*
 * tsd_tx_irig_core.c
 *
 *  Created on: Oct 24, 2019
 *      Author: eric
 */



#include <stdio.h>
#include "tsd_time_bcd.h"
#include "tsd_tx_irig_core.h"
#include "tsd_register_common.h"


/*
 * prints a summary of the core's setup to stdout
 */
void tsd_tx_irig_core_debug_print(uint32_t base)
{
	double fpgaVersion, driverVersion;
	printf("TX IRIG Core at 0x%X\n", (unsigned)base);
	if(0 == tsd_tx_irig_core_get_version(base, &fpgaVersion, &driverVersion))
	{
		printf(" - FPGA Version:  %f\n", fpgaVersion);
		printf(" - Driver Version:  %f\n", driverVersion);
	}
	else
	{
		printf(" - Version:  FAILED TO READ\n");
	}

	printf(" - REGISTERS:\n");
	printf("    => %02X: VERSION REG     = 0x%08X\n", TSD_TX_IRIG_VERSION_REG, TSD_TX_IRIG_IORD_VERSION(base));
	printf("    => %02X: CONTROL REG     = 0x%08X\n", TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_IORD_CONTROL(base));
	printf("    => %02X: STATUS REG      = 0x%08X\n", TSD_TX_IRIG_STATUS_REG, TSD_TX_IRIG_IORD_STATUS(base));
	printf("    => %02X: CAL FINE REG    = 0x%08X\n", TSD_TX_IRIG_CAL_FINE_REG, TSD_TX_IRIG_IORD_CAL_FINE(base));
	printf("    => %02X: CAL MAJOR REG   = 0x%08X\n", TSD_TX_IRIG_CAL_MAJOR_REG, TSD_TX_IRIG_IORD_CAL_MAJOR(base));
	printf("    => %02X: CAL MINOR REG   = 0x%08X\n", TSD_TX_IRIG_CAL_MINOR_REG, TSD_TX_IRIG_IORD_CAL_MINOR(base));
}

/*
 * Gets the Telspan TX_IRIG core version
 *
 * returns 0 on success.
 */
int tsd_tx_irig_core_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
	uint32_t version = TSD_TX_IRIG_IORD_VERSION(base);
	uint32_t vH, vL;

	if(version == 0xDEADBEEF)
	{
		return 1;
	}
	*driverVersion = (double)TSD_TX_IRIG_CORE_DRIVER_VERSION;
	vH = (version >> 16);
	vL = (version & 0xFFFF);
	if(vL < 100)
		*fpgaVersion = (double)vH + (double)(vL/100);
	else
		*fpgaVersion = (double)vH + (double)(vL/1000);

	return 0;
}

/*
 * Enables the Telspan TX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_tx_irig_core_enable(uint32_t base)
{
	return set_register_bits(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_ENABLE);
}

/*
 * Enables the Telspan TX_IRIG core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_tx_irig_core_disable(uint32_t base)
{
	return clear_register_bits(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_ENABLE);
}

/*
 * Enables the Telspan TX_IRIG core's rx loopback (output what is coming in on the rx)
 * returns 0 on success.
 */
int tsd_tx_irig_core_enable_rx_loopback(uint32_t base)
{
	return set_register_bits(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_RX_LOOPBACK);
}

/*
 * Enables the Telspan TX_IRIG core's rx loopback (output what is coming in on the rx)
 * returns 0 on success.
 */
int tsd_tx_irig_core_disable_rx_loopback(uint32_t base)
{
	return clear_register_bits(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_RX_LOOPBACK);
}

/*
 * Enables the Telspan TX_IRIG core's RTC Cal function (attempts to stay aligned with RTC)
 * this is typically used for replay of data.
 * returns 0 on success.
 */
int tsd_tx_irig_core_enable_cal2rtc(uint32_t base)
{
	return clear_register_bits(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_DISABLE_CAL_2_RTC);
}

/*
 * Disables the Telspan TX_IRIG core's RTC Cal function (attempts to stay aligned with RTC)
 * this is typically used for replay of data.
 * returns 0 on success.
 */
int tsd_tx_irig_core_disable_cal2rtc(uint32_t base)
{
    return set_register_bits(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_DISABLE_CAL_2_RTC);
}

/*
 * Sets the IRIG-AM amplitude.
 * setting = 0 enables Full Amplitude
 * setting = 1 enables 1/2 Amplitude
 * setting = 2 enables 1/4 Amplitude
 * setting = 3 enables 1/8 Amplitude
 * returns 0 on success.
 */
int tsd_tx_irig_core_am_amplitude(uint32_t base, tsd_tx_irig_am_amplitude setting)
{
    if((setting < 0) || (setting > TSD_TX_IRIG_AM_AMPLITUDE_UNDEFINED))
    {
        return 1;
    }
    return set_register_region(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_AM_AMPLITUDE_MASK, TSD_TX_IRIG_CONTROL_AM_AMPLITUDE_SHFT, (uint32_t)setting);
}

/*
 * Sets the TX_IRIG Code type flags in the Telspan TX_IRIG core
 * located at base address 'base'
 *
 * see tsd_tx_irig_code_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_tx_irig_core_set_code_type(uint32_t base, tsd_tx_irig_code_type_t codeType)
{
	if(((int)codeType < 0) || ((int)codeType >= TSD_TX_IRIG_CODE_UNDEFINED))
	{
		return 1;
	}
	// base, offset, shift, value
	return set_register_region(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_CODE_TYPE_MASK, TSD_TX_IRIG_CONTROL_CODE_TYPE_SHFT, (uint32_t)codeType);
}

/*
 * Gets the TX_IRIG Code type flags in the Telspan TX_IRIG core
 * located at base address 'base'
 *
 * see tsd_tx_irig_code_type_t enumeration
 *
 * returns 0 on success..
 */
int tsd_tx_irig_core_get_code_type(uint32_t base, tsd_tx_irig_code_type_t *codeType)
{
	uint32_t regVal;
	if(NULL == codeType) return 1;
	if(get_register_region(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_CONTROL_CODE_TYPE_MASK, TSD_TX_IRIG_CONTROL_CODE_TYPE_SHFT, &regVal))
	{
		return 2;
	}
	if(((int)regVal < 0) || ((int)regVal >= TSD_TX_IRIG_CODE_UNDEFINED))
	{
		return 3;
	}
	*codeType = (tsd_tx_irig_code_type_t)regVal;
	return 0;
}

/*
 * Gets the DC 1PPS mode.
 * When on (set dc1ppsOn to true) the IRIG-DC output becomes a 1PPS output.
 * When off (set dc1ppsOn to false) the IRIG-DC output is IRIG DC Code.
 * returns 0 on success
 */
int tsd_tx_irig_core_get_dc_1pps(uint32_t base, bool *dc1ppsOn)
{
    uint32_t regVal = 0;
    int retval = get_register_region(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_TTL_OUT_1PPS_MASK, TSD_TX_IRIG_TTL_OUT_1PPS_SHFT, &regVal);
    if( retval == 0 )
    {
        if( regVal )
            *dc1ppsOn = true;
        else
            *dc1ppsOn = false;
    }
    return retval;
}

/*
 * Sets the DC 1PPS mode.
 * When on (set dc1ppsOn to true) the IRIG-DC output becomes a 1PPS output.
 * When off (set dc1ppsOn to false) the IRIG-DC output is IRIG DC Code.
 * returns 0 on success
 */
int tsd_tx_irig_core_set_dc_1pps(uint32_t base, bool dc1ppsOn)
{
	if(dc1ppsOn)
		return set_register_bits(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_TTL_OUT_1PPS_MASK);
	else
		return clear_register_bits(base, TSD_TX_IRIG_CONTROL_REG, TSD_TX_IRIG_TTL_OUT_1PPS_MASK);
}

/*
 * Gets the TX_IRIG Status/Error flags in the Telspan TX_IRIG core
 * located at base address 'base'
 *
 * see TSD_TX_IRIG_STATUS_* definitions in tsd_tx_irig_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_tx_irig_core_get_status_flags(uint32_t base, uint32_t *statusFlags)
{
	uint32_t regVal = TSD_TX_IRIG_IORD_STATUS(base);
	if(regVal == 0xDEADBEEF)
	{
		return 1;
	}
	*statusFlags = regVal;
	return 0;
}

/*
 * Sets the TX_IRIG Calibration
 * returns 0 on success.
 */
int tsd_tx_irig_core_set_cal(uint32_t base, uint32_t calMajor, uint32_t calMinor, uint32_t calFine)
{
	TSD_TX_IRIG_IOWR_CAL_FINE(base, calFine);
	TSD_TX_IRIG_IOWR_CAL_MAJOR(base, calMajor);
	TSD_TX_IRIG_IOWR_CAL_MINOR(base, calMinor);
	return 0;
}

/*
 * Gets the TX_IRIG Calibration
 * returns 0 on success.
 */
int tsd_tx_irig_core_get_cal(uint32_t base, uint32_t *calMajor, uint32_t *calMinor, uint32_t *calFine)
{
    if((NULL == calMajor) || (NULL == calMinor) || (NULL == calFine)) return 1;

    *calMajor = TSD_TX_IRIG_IORD_CAL_MAJOR(base);
    *calMinor = TSD_TX_IRIG_IORD_CAL_MINOR(base);
    *calFine = TSD_TX_IRIG_IORD_CAL_FINE(base);
    return 0;
}


int tsd_tx_irig_core_get_time(uint32_t base, struct timespec* txtime)
{
    uint32_t timedata1 = TSD_TX_IRIG_IORD_TIME_L(base);
    uint32_t timedata2 = TSD_TX_IRIG_IORD_TIME_H(base);

    uint64_t current_rtc, tv_sec_rtc, rtcdiff;

    if(NULL == txtime) return 1;

    // get rtcs now for accuracy.
    current_rtc = tsd_tx_irig_core_get_current_rtc(base);
    tv_sec_rtc = tsd_tx_irig_core_get_latched_rtc(base);

    get_timespec_from_bcd(timedata1, timedata2, txtime);

    // now fill in the tv_nsecs.
    if(current_rtc > tv_sec_rtc)
    {
        rtcdiff = (current_rtc - tv_sec_rtc);
        if(rtcdiff < TSD_TX_IRIG_CORE_RTC_TICKS_PER_SEC)
        {
            txtime->tv_nsec += (long)(rtcdiff * 100);
        }
        else
        {
            txtime->tv_nsec += 0;
        }
    }

    return 0;
}

uint64_t tsd_tx_irig_core_get_current_rtc(uint32_t base)
{
    uint64_t rtcH = (uint64_t)TSD_TX_IRIG_IORD_CURRENT_RTC_MSW(base);
    uint64_t rtcL = (uint64_t)TSD_TX_IRIG_IORD_CURRENT_RTC_LSW(base);
    return (rtcH << 32) | rtcL;
}

uint64_t tsd_tx_irig_core_get_latched_rtc(uint32_t base)
{
    uint64_t rtcH = (uint64_t)TSD_TX_IRIG_IORD_LATCHED_RTC_MSW(base);
    uint64_t rtcL = (uint64_t)TSD_TX_IRIG_IORD_LATCHED_RTC_LSW(base);
    return (rtcH << 32) | rtcL;
}

