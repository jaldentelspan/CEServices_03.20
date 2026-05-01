/*
 * tsd_rx_1pps_core.c
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */
#include <stdio.h>
#include <string.h> 
#include "tsd_time_bcd.h"
#include "tsd_rx_1pps_core.h"
#include "tsd_register_common.h"


/*
 * prints a summary of the core's setup to stdout
 */
void tsd_rx_1pps_core_debug_print(uint32_t base)
{
    double fpgaVersion, driverVersion;
    mmdrv_printf("TSD 1PPS RX Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_rx_1pps_core_get_version(base, &fpgaVersion, &driverVersion))
    {
        mmdrv_printf(" - FPGA Version:  %f\n", fpgaVersion);
        mmdrv_printf(" - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        mmdrv_printf(" - Version:  FAILED TO READ\n");
    }

    mmdrv_printf(" - REGISTERS:\n");
    mmdrv_printf("    => %02X: VERSION REG    = 0x%08X\n", TSD_RX_1PPS_CORE_VERSION_REG, TSD_RX_1PPS_CORE_IORD_VERSION(base));
    mmdrv_printf("    => %02X: CONTROL REG    = 0x%08X\n", TSD_RX_1PPS_CORE_CONTROL_REG, TSD_RX_1PPS_CORE_IORD_CONTROL(base));
    mmdrv_printf("    => %02X: STATUS  REG    = 0x%08X\n", TSD_RX_1PPS_CORE_STATUS_REG, TSD_RX_1PPS_CORE_IORD_STATUS(base));
    mmdrv_printf("    => %02X: TIME L  REG    = 0x%08X\n", TSD_RX_1PPS_CORE_TIME_LOW_REG, TSD_RX_1PPS_CORE_IORD_TIME_LOW(base));
    mmdrv_printf("    => %02X: TIME H  REG    = 0x%08X\n", TSD_RX_1PPS_CORE_TIME_HIGH_REG, TSD_RX_1PPS_CORE_IORD_TIME_HIGH(base));
    mmdrv_printf("    => %02X: WINDOW MIN REG = 0x%08X\n", TSD_RX_1PPS_CORE_WINDOW_MIN_REG, TSD_RX_1PPS_CORE_IORD_WINDOW_MIN(base));
    mmdrv_printf("    => %02X: WINDOW MAX REG = 0x%08X\n", TSD_RX_1PPS_CORE_WINDOW_MAX_REG, TSD_RX_1PPS_CORE_IORD_WINDOW_MAX(base));


}

/*
 * Gets the Telspan RX_1PPS_CORE core version
 *
 * returns 0 on success.
 */
int tsd_rx_1pps_core_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
    uint32_t version = TSD_RX_1PPS_CORE_IORD_VERSION(base);
    uint32_t vH, vL;

    if(version == 0xDEADBEEF)
    {
        return 1;
    }
    *driverVersion = (double)TSD_RX_1PPS_CORE_CORE_DRIVER_VERSION;
    vH = (version >> 16);
    vL = (version & 0xFFFF);
    if(vL < 100)
        *fpgaVersion = (double)vH + (double)(vL/100);
    else
        *fpgaVersion = (double)vH + (double)(vL/1000);

    return 0;
}

/*
 * Enables the Telspan RX_1PPS_CORE core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rx_1pps_core_enable(uint32_t base)
{
    return set_register_bits(base, TSD_RX_1PPS_CORE_CONTROL_REG, TSD_RX_1PPS_CORE_CONTROL_ENABLE);
}

/*
 * Enables the Telspan RX_1PPS_CORE core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_rx_1pps_core_disable(uint32_t base)
{
    return clear_register_bits(base, TSD_RX_1PPS_CORE_CONTROL_REG, TSD_RX_1PPS_CORE_CONTROL_ENABLE);
}

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
int tsd_rx_1pps_core_time_valid(uint32_t base, int timeValid)
{
    if(timeValid)
        return set_register_bits(base, TSD_RX_1PPS_CORE_CONTROL_REG, TSD_RX_1PPS_CORE_TIME_VALID_BIT);
    else
        return clear_register_bits(base, TSD_RX_1PPS_CORE_CONTROL_REG, TSD_RX_1PPS_CORE_TIME_VALID_BIT);
}

/*
 * Reset control for the time input selector core
 * resetOn = 1 : core in reset
 * resetOn = 0 : core not in reset.
 * In order to reset the core, set resetOn to 1, wait for at least 100us,
 * then set resetOn to 0.
 * Returns 0 on success
 */
int tsd_rx_1pps_core_reset(uint32_t base, int resetOn)
{
    if(resetOn)
        return set_register_bits(base, TSD_RX_1PPS_CORE_CONTROL_REG, TSD_RX_1PPS_CORE_CONTROL_RESET);
    else
        return clear_register_bits(base, TSD_RX_1PPS_CORE_CONTROL_REG, TSD_RX_1PPS_CORE_CONTROL_RESET);
}

/*
 * Gets the 1PPS Status/Error flags in the Telspan 1PPS core
 * located at base address 'base'
 *
 * see TSD_1PPS_STATUS_* definitions in tsd_rx_1pps_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_rx_1pps_core_get_status_flags(uint32_t base, uint32_t *statusFlags)
{
    uint32_t regVal = TSD_RX_1PPS_CORE_IORD_STATUS(base);
    if(regVal == 0xDEADBEEF)
    {
        return 1;
    }
    *statusFlags = regVal;
    return 0;
}

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
int tsd_rx_1pps_core_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds)
{
    uint32_t timeL = TSD_RX_1PPS_CORE_IORD_TIME_LOW(base);
    uint32_t timeH = TSD_RX_1PPS_CORE_IORD_TIME_HIGH(base);
    if((timeL == 0xDEADBEEF) || (timeH == 0xDEADBEEF))
    {
        return 1;
    }

    *milliseconds = 10* bcd_u8_to_int( (uint8_t)((timeL & TSD_RX_1PPS_CORE_TIME_L_MSEC_MASK) >> TSD_RX_1PPS_CORE_TIME_L_MSEC_SHFT) );
    *seconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_RX_1PPS_CORE_TIME_L_SEC_MASK) >> TSD_RX_1PPS_CORE_TIME_L_SEC_SHFT) );
    *minutes = bcd_u8_to_int( (uint8_t)((timeL & TSD_RX_1PPS_CORE_TIME_L_MIN_MASK) >> TSD_RX_1PPS_CORE_TIME_L_MIN_SHFT) );
    *hours = bcd_u8_to_int( (uint8_t)((timeL & TSD_RX_1PPS_CORE_TIME_L_HOUR_MASK) >> TSD_RX_1PPS_CORE_TIME_L_HOUR_SHFT) );
    *ydays = bcd_u16_to_int( (uint16_t)((timeH & TSD_RX_1PPS_CORE_TIME_H_YDAY_MASK) >> TSD_RX_1PPS_CORE_TIME_H_YDAY_SHFT) );
    *years = 2000 +
            bcd_u8_to_int( (uint8_t)((timeH & TSD_RX_1PPS_CORE_TIME_H_YEAR_MASK) >> TSD_RX_1PPS_CORE_TIME_H_YEAR_SHFT) );

    return 0;
}

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
int tsd_rx_1pps_core_set_time(uint32_t base, int years, int ydays, int hours, int minutes, int seconds, int milliseconds)
{
    struct tm tmtime;
    struct timespec ts;
    memset(&tmtime, 0, sizeof(struct tm));

    tmtime.tm_year = (years >= 2000) ? (years - 1900) : years;
    tmtime.tm_yday = ydays;
    tmtime.tm_hour = hours;
    tmtime.tm_min = minutes;
    tmtime.tm_sec = seconds;

    ts.tv_sec = mktime(&tmtime);
    ts.tv_nsec = milliseconds * 1000000;
    return tsd_rx_1pps_core_set_time_ts(base, ts);
}

/*
 * Sets the time in the RX_1PPS_CORE using struct timespec.
 * This must be current with the second (written every second).
 */
int tsd_rx_1pps_core_set_time_ts(uint32_t base, struct timespec ts)
{
    int retval = 0;
    uint32_t bcd_h, bcd_l;
    retval = get_bcd_from_timespec(&ts, &bcd_l, &bcd_h);
    if(0 == retval)
    {
        TSD_RX_1PPS_CORE_IOWR_TIME_HIGH(base, bcd_h);
        TSD_RX_1PPS_CORE_IOWR_TIME_LOW(base, bcd_l);
    }
    return retval;
}

/*
 * Gets the time from the RX_1PPS_CORE as struct timespec.
 */
int tsd_rx_1pps_core_get_time_ts(uint32_t base, struct timespec *ts)
{
    if(NULL == ts) return 1;
    uint32_t bcd_h = TSD_RX_1PPS_CORE_IORD_TIME_HIGH(base);
    uint32_t bcd_l = TSD_RX_1PPS_CORE_IORD_TIME_LOW(base);

    get_timespec_from_bcd(bcd_l, bcd_h, ts);

    return 0;
}

/*
 * Sets the time in the RX_1PPS_CORE using struct tm.  This must be current with the second (written every second).
 */
int tsd_rx_1pps_core_set_time_tm(uint32_t base, struct tm tmtime)
{
    int retval = 0;
    uint32_t bcd_h, bcd_l;
    retval = get_bcd_from_tm(&tmtime, &bcd_l, &bcd_h);
    if(0 == retval)
    {
        TSD_RX_1PPS_CORE_IOWR_TIME_HIGH(base, bcd_h);
        TSD_RX_1PPS_CORE_IOWR_TIME_LOW(base, bcd_l);
    }
    return retval;
}

/*
 * Gets the time from the RX_1PPS_CORE as struct tm.
 */
int tsd_rx_1pps_core_get_time_tm(uint32_t base, struct tm *tmtime)
{
    if(NULL == tmtime) return 1;
    uint32_t bcd_h = TSD_RX_1PPS_CORE_IORD_TIME_HIGH(base);
    uint32_t bcd_l = TSD_RX_1PPS_CORE_IORD_TIME_LOW(base);

    get_tm_from_bcd(bcd_l, bcd_h, tmtime);

    return 0;
}

/*
 * Sets the 1PPS Window minimum value.
 */
int tsd_rx_1pps_core_set_window_min(uint32_t base, uint32_t winMin)
{
    TSD_RX_1PPS_CORE_IOWR_WINDOW_MIN(base, winMin);
    return 0;
}

/*
 * Gets the 1PPS Window minimum value.
 */
int tsd_rx_1pps_core_get_window_min(uint32_t base, uint32_t *winMin)
{
    *winMin = TSD_RX_1PPS_CORE_IORD_WINDOW_MIN(base);
    if(0xDEADBEEF == *winMin)
        return 1;
    return 0;
}

/*
 * Sets the 1PPS Window maximum value.
 */
int tsd_rx_1pps_core_set_window_max(uint32_t base, uint32_t winMax)
{
    TSD_RX_1PPS_CORE_IOWR_WINDOW_MAX(base, winMax);
    return 0;
}

/*
 * Gets the 1PPS Window maximum value.
 */
int tsd_rx_1pps_core_get_window_max(uint32_t base, uint32_t *winMax)
{
    *winMax = TSD_RX_1PPS_CORE_IORD_WINDOW_MIN(base);
    if(0xDEADBEEF == *winMax)
        return 1;
    return 0;
}
