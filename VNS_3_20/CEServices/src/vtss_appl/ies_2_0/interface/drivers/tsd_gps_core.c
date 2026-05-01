/*
 * tsd_gps_core.c
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */
#include <stdio.h>
#include "mmdrv_printf.h"
#include "tsd_gps_core.h"
#include "tsd_register_common.h"



/*
 * prints a summary of the core's setup to stdout
 */
void tsd_gps_core_debug_print(uint32_t base)
{
    double fpgaVersion, driverVersion;
    mmdrv_printf("GPS Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_gps_core_get_version(base, &fpgaVersion, &driverVersion))
    {
        mmdrv_printf(" - FPGA Version:  %f\n", fpgaVersion);
        mmdrv_printf(" - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        mmdrv_printf(" - Version:  FAILED TO READ\n");
    }

    mmdrv_printf(" - REGISTERS:\n");
    mmdrv_printf("    => %02X: VERSION REG     = 0x%08X\n", TSD_GPS_VERSION_REG       , TSD_GPS_IORD_VERSION(base));
    mmdrv_printf("    => %02X: CONTROL REG     = 0x%08X\n", TSD_GPS_CONTROL_REG       , TSD_GPS_IORD_CONTROL(base));
    mmdrv_printf("    => %02X: STATUS REG      = 0x%08X\n", TSD_GPS_STATUS_REG        , TSD_GPS_IORD_STATUS(base));
    mmdrv_printf("    => %02X: LEAP SEC REG    = 0x%08X\n", TSD_GPS_LEAP_SEC_REG      , TSD_GPS_IORD_LEAP_SEC(base));
    mmdrv_printf("    => %02X: POLY MSG REG    = 0x%08X\n", TSD_GPS_POLY_MSG_REG      , TSD_GPS_IORD_POLY_MSG(base));
    mmdrv_printf("    => %02X: TIME LOW REG    = 0x%08X\n", TSD_GPS_TIME_L_REG        , TSD_GPS_IORD_TIME_L(base));
    mmdrv_printf("    => %02X: TIME HIGH REG   = 0x%08X\n", TSD_GPS_TIME_H_REG        , TSD_GPS_IORD_TIME_H(base));
    mmdrv_printf("    => %02X: PPS WND MIN REG = 0x%08X\n", TSD_GPS_PPS_WINDOW_MIN_REG, TSD_GPS_IORD_PPS_WINDOW_MIN(base));
    mmdrv_printf("    => %02X: PPS WND MAX REG = 0x%08X\n", TSD_GPS_PPS_WINDOW_MAX_REG, TSD_GPS_IORD_PPS_WINDOW_MAX(base));
    mmdrv_printf("    => %02X: MTICK CNT REG   = 0x%08X\n", TSD_GPS_PPS_MTICK_CNT_REG , TSD_GPS_IORD_PPS_MTICK_CNT(base));
    mmdrv_printf("    => %02X: LOST LOCK REG   = 0x%08X\n", TSD_GPS_LOST_LOCK_CNTR_REG, TSD_GPS_IORD_LOST_LOCK_CNTR(base));
}

/*
 * Gets the Telspan GPS core version
 *
 * returns 0 on success.
 */
int tsd_gps_core_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
    uint32_t version = TSD_GPS_IORD_VERSION(base);
    uint32_t vH, vL;

    if(version == 0xDEADBEEF)
    {
        return 1;
    }
    *driverVersion = (double)TSD_GPS_CORE_DRIVER_VERSION;
    vH = (version >> 16);
    vL = (version & 0xFFFF);
    if(vL < 100)
        *fpgaVersion = (double)vH + (double)(vL/100);
    else
        *fpgaVersion = (double)vH + (double)(vL/1000);

    return 0;
}

/*
 * Enables the Telspan GPS core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_gps_core_enable(uint32_t base)
{
    return set_register_bits(base, TSD_GPS_CONTROL_REG, TSD_GPS_CONTROL_ENABLE);
}

/*
 * Enables the Telspan GPS core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_gps_core_disable(uint32_t base)
{
    return clear_register_bits(base, TSD_GPS_CONTROL_REG, TSD_GPS_CONTROL_ENABLE);
}

/*
 * Sets the GPS Serial Baud rate in the Telspan GPS core
 * located at base address 'base'
 * module_freq should be set to the system.h _FREQ value for this module
 * returns 0 on success.
 */
int tsd_gps_core_set_serial_baudrate(uint32_t base, int module_freq, int baud)
{
    int divisor;

    if(baud == 0)
    {
        return 1;
    }

    divisor = (module_freq / (16*baud));

    if(divisor > 0x3FF)
        return 1;

    return set_register_region(base, TSD_GPS_CONTROL_REG,
            TSD_GPS_CONTROL_BAUD_RATE_MASK, TSD_GPS_CONTROL_BAUD_RATE_SHFT, divisor);
}

/*
 * Gets the GPS Serial Baud rate in the Telspan GPS core
 * located at base address 'base'
 * module_freq should be set to the system.h _FREQ value for this module
 * returns 0 on success.
 */
int tsd_gps_core_get_serial_baudrate(uint32_t base, int module_freq, int *baud)
{
    // baud = sysclk_rate / (16 * divisor)
    uint32_t regVal = TSD_GPS_IORD_CONTROL(base);
    int divisor;

    if(get_register_region(base, TSD_GPS_CONTROL_REG, TSD_GPS_CONTROL_BAUD_RATE_MASK, TSD_GPS_CONTROL_BAUD_RATE_SHFT, &regVal))
        return 1;

    divisor = (int)regVal;
    if(divisor == 0)
    {
        return 1;
    }
    *baud = (module_freq / (16*divisor));
    return 0;
}

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
int tsd_gps_core_set_power_flags(uint32_t base, uint32_t pwrFlags)
{
    if((pwrFlags & TSD_GPS_CONTROL_ANT_3P3V) && (pwrFlags & TSD_GPS_CONTROL_ANT_5P0V))
    {
        // cannot set both antenna voltages at the same time
        return -1;
    }
    return set_register_region(base, TSD_GPS_CONTROL_REG,
            TSD_GPS_CONTROL_PWR_FLAG_MASK, TSD_GPS_CONTROL_PWR_FLAG_SHFT, pwrFlags);

}

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
int tsd_gps_core_set_power_flags_bits(uint32_t base, uint32_t pwrFlagBits)
{
    int retval = 0;
    if((pwrFlagBits & TSD_GPS_CONTROL_ANT_3P3V) && (pwrFlagBits & TSD_GPS_CONTROL_ANT_5P0V))
    {
        // cannot set both antenna voltages at the same time
        return -1;
    }
    else if(pwrFlagBits & TSD_GPS_CONTROL_ANT_3P3V)
    {
        // make sure 5v antenna voltage is off so that we can turn on 3.3v
        retval += clear_register_bits(base, TSD_GPS_CONTROL_REG, TSD_GPS_CONTROL_ANT_5P0V);
    }
    else if(pwrFlagBits & TSD_GPS_CONTROL_ANT_5P0V)
    {
        // make sure 3.3v antenna voltage is off so that we can turn on 5v
        retval += clear_register_bits(base, TSD_GPS_CONTROL_REG, TSD_GPS_CONTROL_ANT_3P3V);
    }
    if(retval)
        return retval;

    return set_register_bits(base, TSD_GPS_CONTROL_REG, pwrFlagBits);
}

/*
 * Clears individual bits in the GPS Power Flags in the Telspan GPS core
 * located at base address 'base'
 *
 * Valid flags are defined in tsd_gps_core_registers.h
 *  - TSD_GPS_CONTROL_PWR_* and TSD_GPS_CONTROL_ANT_*
 *
 * returns 0 on success.
 */
int tsd_gps_core_clear_power_flags_bits(uint32_t base, uint32_t pwrFlagBits)
{
    return clear_register_bits(base, TSD_GPS_CONTROL_REG, pwrFlagBits);
}

/*
 * Gets the GPS Power Flags in the Telspan GPS core
 * located at base address 'base'
 *
 * see TSD_GPS_CONTROL_PWR_* and TSD_GPS_CONTROL_ANT_* definitions in tsd_gps_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_gps_core_get_power_flags(uint32_t base, uint32_t *pwrFlags)
{
    uint32_t regVal = TSD_GPS_IORD_CONTROL(base);
    if(regVal == 0xDEADBEEF)
    {
        return 1;
    }
    *pwrFlags = regVal & TSD_GPS_CONTROL_PWR_FLAG_MASK;
    return 0;
}

/*
 * Gets the GPS Status/Error flags in the Telspan GPS core
 * located at base address 'base'
 *
 * see TSD_GPS_STATUS_* definitions in tsd_gps_core_registers.h
 *
 * returns 0 on success..
 */
int tsd_gps_core_get_status_flags(uint32_t base, uint32_t *statusFlags)
{
    uint32_t regVal = TSD_GPS_IORD_STATUS(base);
    if(regVal == 0xDEADBEEF)
    {
        return 1;
    }
    *statusFlags = regVal;
    return 0;
}

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
 *          software control of the GPS is enabled.  If software control
 *          of the GPS is enabled, it is the responsibility of the software
 *          to setup the current leapsecond value in the receiver and this
 *          register is purely informational.
 *
 * returns 0 on success..
 */
int tsd_gps_core_set_current_leapsecond(uint32_t base, uint8_t leapsecondVal)
{
    // send to FPGA as BCD
    TSD_GPS_IOWR_LEAP_SEC(base, (uint32_t)(((leapsecondVal / 10) << 1) | ((leapsecondVal % 10))) );
    return 0;
}

/*
 * Gets the GPS Current Leap Second Value in the Telspan GPS core
 * located at base address 'base'
 *
 * returns 0 on success..
 */
int tsd_gps_core_get_current_leapsecond(uint32_t base, uint8_t *leapsecondVal)
{
    uint32_t regVal = TSD_GPS_IORD_LEAP_SEC(base);
    if(regVal == 0xDEADBEEF)
    {
        return 1;
    }
    // UN-BCD the regval...
    regVal &= TSD_GPS_LEAP_SECOND_REG_MASK;
    *leapsecondVal = (uint8_t)(((regVal >> 4) * 10) + (regVal & 0x0F));
    return 0;
}

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
int tsd_gps_core_get_poly_leapsecond(uint32_t base, uint8_t *polyLeapsecondVal)
{
    uint32_t regVal = TSD_GPS_IORD_POLY_MSG(base);
    if(regVal == 0xDEADBEEF)
    {
        return 1;
    }
    if(1)//(regVal & TSD_GPS_POLY_FOUND_LEAP_SEC_MSG) && (regVal & TSD_GPS_POLY_LEAP_SEC_MSG_DONE))
    {
        regVal &= TSD_GPS_POLY_LEAP_SEC_MASK;
        *polyLeapsecondVal = (uint8_t)(((regVal >> 4) * 10) + (regVal & 0x0F));
    }
    else
    {
        return -1;
    }
    return 0;
}

/*
 * Gets the current time in the Telspan GPS core located at base address 'base'
 * years: current year
 * ydays: day in year (number of elapsed days in the current year).
 * hours: elapsed hours in the current day.
 * minutes: elapsed minutes in the current hour.
 * seconds: elapsed seconds in the current minute.
 * milliseconds: elapsed milliseconds in the current second (not used)
 *
 * returns 0 on success, 1 on failure, -1 on invalid (gps not locked)
 */
int tsd_gps_core_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds)
{
    uint32_t timeL = TSD_GPS_IORD_TIME_L(base);
    uint32_t timeH = TSD_GPS_IORD_TIME_H(base);
    if((timeL == 0xDEADBEEF) || (timeH == 0xDEADBEEF))
    {
        return 1;
    }

    *milliseconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_GPS_TIME_L_BCD_MSEC_MASK) >> TSD_GPS_TIME_L_BCD_MSEC_SHFT) );
    *seconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_GPS_TIME_L_BCD_SECONDS_MASK) >> TSD_GPS_TIME_L_BCD_SECONDS_SHFT) );
    *minutes = bcd_u8_to_int( (uint8_t)((timeL & TSD_GPS_TIME_L_BCD_MINUTES_MASK) >> TSD_GPS_TIME_L_BCD_MINUTES_SHFT) );
    *hours = bcd_u8_to_int( (uint8_t)((timeL & TSD_GPS_TIME_L_BCD_HOURS_MASK) >> TSD_GPS_TIME_L_BCD_HOURS_SHFT) );
    *ydays = bcd_u16_to_int( (uint16_t)((timeH & TSD_GPS_TIME_H_BCD_DAYS_MASK) >> TSD_GPS_TIME_H_BCD_DAYS_SHFT) );
    *years = 2000 +
            bcd_u8_to_int( (uint8_t)((timeH & TSD_GPS_TIME_H_BCD_YEARS_MASK) >> TSD_GPS_TIME_H_BCD_YEARS_SHFT) );

    return 0;
}

/*
 * Gets the time from the GPS core as struct timespec.
 */
int tsd_gps_core_get_time_ts(uint32_t base, struct timespec *ts)
{
    if(NULL == ts) return 1;
    uint32_t bcd_h = TSD_GPS_IORD_TIME_H(base);
    uint32_t bcd_l = TSD_GPS_IORD_TIME_L(base);

    get_timespec_from_bcd(bcd_l, bcd_h, ts);

    return 0;
}

/*
 * Gets the time from the GPS core as struct tm.
 */
int tsd_gps_core_get_time_tm(uint32_t base, struct tm *tmtime)
{
    if(NULL == tmtime) return 1;
    uint32_t bcd_h = TSD_GPS_IORD_TIME_H(base);
    uint32_t bcd_l = TSD_GPS_IORD_TIME_L(base);

    get_tm_from_bcd(bcd_l, bcd_h, tmtime);

    return 0;
}

/*
 * Sets the minimum value for 1PPS detect window in the Telspan GPS core
 * located at base address 'base'.
 *
 * Value is the minimum number of clock ticks (at the clock rate of the core) that must elapse
 * between 1PPS Signals.
 * The 1PPS signal must be seen within the "Window" in order to declare lock.
 *
 * returns 0 on success.
 */
int tsd_gps_core_set_pps_window_min(uint32_t base, uint32_t ppsWindowMin)
{
    TSD_GPS_IOWR_PPS_WINDOW_MIN(base, ppsWindowMin);
    return 0;
}

/*
 * Sets the maximum value for 1PPS detect window in the Telspan GPS core
 * located at base address 'base'.
 *
 * Value is the maximum number of clock ticks (at the clock rate of the core) that must elapse
 * between 1PPS Signals.
 * The 1PPS signal must be seen within the "Window" in order to declare lock.
 *
 * returns 0 on success.
 */
int tsd_gps_core_set_pps_window_max(uint32_t base, uint32_t ppsWindowMax)
{
    TSD_GPS_IOWR_PPS_WINDOW_MAX(base, ppsWindowMax);
    return 0;
}

/*
 * Gets the minimum value for 1PPS detect window in the Telspan GPS core
 * located at base address 'base'.
 *
 * returns 0 on success.
 */
int tsd_gps_core_get_pps_window_min(uint32_t base, uint32_t *ppsWindowMin)
{
    *ppsWindowMin = TSD_GPS_IORD_PPS_WINDOW_MIN(base);
    return ( ((*ppsWindowMin) != 0xDEADBEEF) ? 0 : 1 );
}

/*
 * Gets the maximum value for 1PPS detect window in the Telspan GPS core
 * located at base address 'base'.
 *
 * returns 0 on success.
 */
int tsd_gps_core_get_pps_window_max(uint32_t base, uint32_t *ppsWindowMax)
{
    *ppsWindowMax = TSD_GPS_IORD_PPS_WINDOW_MAX(base);
    return ( ((*ppsWindowMax) != 0xDEADBEEF) ? 0 : 1 );
}

/*
 * Gets the measured 1PPS tick count for the last tick
 * in the Telspan GPS core located at base address 'base'.
 *
 * returns 0 on success.
 */
int tsd_gps_core_get_pps_measured_tick_delta(uint32_t base, uint32_t *ppsDeltaTick)
{
    *ppsDeltaTick = TSD_GPS_IORD_PPS_MTICK_CNT(base);
    return ( ((*ppsDeltaTick) != 0xDEADBEEF) ? 0 : 1 );
}

/*
 * Gets the lost lock counter from the Telspan GPS core and places it into
 * the integer pointed to by count.
 *
 * returns 0 on success..
 */
int tsd_gps_core_get_lost_lock_count(uint32_t base, uint32_t *count)
{
    uint32_t regVal = TSD_GPS_IORD_LOST_LOCK_CNTR(base);
    if(regVal == 0xDEADBEEF)
    {
        return 1;
    }
    *count = (regVal & TSD_GPS_LOST_LOCK_CNTR_MASK);
    return 0;
}

/*
 * Resets the lost lock counter to 0.
 * returns 0 on success..
 */
int tsd_gps_core_reset_lost_lock_count(uint32_t base)
{
    TSD_GPS_IOWR_LOST_LOCK_CNTR(base, TSD_GPS_LOST_LOCK_CNTR_RESET);
    // todo: find out if we need any delay here...
    TSD_GPS_IOWR_LOST_LOCK_CNTR(base, 0);
    return 0;
}
