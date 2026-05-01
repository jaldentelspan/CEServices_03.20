/*
 * tsd_time_input_selector.c
 *
 *  Created on: Sep 6, 2017
 *      Author: eric
 */
#include <stdio.h>
#include "tsd_time_input_selector.h"
#include "tsd_register_common.h"


/*
 * prints a summary of the core's setup to stdout
 */
void tsd_time_input_selector_debug_print(uint32_t base)
{
    double fpgaVersion, driverVersion;

    mmdrv_printf("TSD Time Input Selector Core at 0x%X\n", (unsigned)base);
    if(0 == tsd_time_input_selector_get_version(base, &fpgaVersion, &driverVersion))
    {
        mmdrv_printf(" - FPGA Version:  %f\n", fpgaVersion);
        mmdrv_printf(" - Driver Version:  %f\n", driverVersion);
    }
    else
    {
        mmdrv_printf(" - Version:  FAILED TO READ\n");
    }

    mmdrv_printf(" - REGISTERS:\n");
    mmdrv_printf("    => %02X: VERSION REG   = 0x%08X\n", TSD_TIME_INPUT_SELECTOR_VERSION_REG, TSD_TIME_INPUT_SELECTOR_IORD_VERSION(base));
    mmdrv_printf("    => %02X: CONTROL REG   = 0x%08X\n", TSD_TIME_INPUT_SELECTOR_CONTROL_REG, TSD_TIME_INPUT_SELECTOR_IORD_CONTROL(base));
    mmdrv_printf("    => %02X: STATUS  REG   = 0x%08X\n", TSD_TIME_INPUT_SELECTOR_STATUS_REG, TSD_TIME_INPUT_SELECTOR_IORD_STATUS(base));
    mmdrv_printf("    => %02X: PRIO    REG   = 0x%08X\n", TSD_TIME_INPUT_SELECTOR_PRIO_REG, TSD_TIME_INPUT_SELECTOR_IORD_PRIO(base));
    mmdrv_printf("    => %02X: TIME L  REG   = 0x%08X\n", TSD_TIME_INPUT_SELECTOR_TIME_L_REG, TSD_TIME_INPUT_SELECTOR_IORD_TIME_L(base));
    mmdrv_printf("    => %02X: TIME H  REG   = 0x%08X\n", TSD_TIME_INPUT_SELECTOR_TIME_H_REG, TSD_TIME_INPUT_SELECTOR_IORD_TIME_H(base));
}

/*
 * Gets the Telspan TIME_INPUT_SELECTOR core version
 *
 * returns 0 on success.
 */
int tsd_time_input_selector_get_version(uint32_t base, double *fpgaVersion, double *driverVersion)
{
    uint32_t version = TSD_TIME_INPUT_SELECTOR_IORD_VERSION(base);
    uint32_t vH, vL;

    if(version == 0xDEADBEEF)
    {
        return 1;
    }
    *driverVersion = (double)TSD_TIME_INPUT_SELECTOR_CORE_DRIVER_VERSION;
    vH = (version >> 16);
    vL = (version & 0xFFFF);
    if(vL < 100)
        *fpgaVersion = (double)vH + (double)(vL/100);
    else
        *fpgaVersion = (double)vH + (double)(vL/1000);

    return 0;
}

/*
 * Enables the Telspan TIME_INPUT_SELECTOR core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_time_input_selector_enable(uint32_t base)
{
    return set_register_bits(base, TSD_TIME_INPUT_SELECTOR_CONTROL_REG, TSD_TIME_INPUT_SELECTOR_CONTROL_ENABLE);
}

/*
 * Enables the Telspan TIME_INPUT_SELECTOR core located at base address 'base'
 *
 * returns 0 on success.
 */
int tsd_time_input_selector_disable(uint32_t base)
{
    return clear_register_bits(base, TSD_TIME_INPUT_SELECTOR_CONTROL_REG, TSD_TIME_INPUT_SELECTOR_CONTROL_ENABLE);
}

/*
 * Reset control for the time input selector core
 * resetOn = 1 : core in reset
 * resetOn = 0 : core not in reset.
 * In order to reset the core, set resetOn to 1, wait for at least 100us,
 * then set resetOn to 0.
 * Returns 0 on success
 */
int tsd_time_input_selector_reset(uint32_t base, int resetOn)
{
    if(resetOn)
        return set_register_bits(base, TSD_TIME_INPUT_SELECTOR_CONTROL_REG, TSD_TIME_INPUT_SELECTOR_CONTROL_RESET);
    else
        return clear_register_bits(base, TSD_TIME_INPUT_SELECTOR_CONTROL_REG, TSD_TIME_INPUT_SELECTOR_CONTROL_RESET);
}

/*
 * Gets the currently selected channel (0 based).
 * Note that this changes automatically by the core based on lock status and priority.
 * returns 0 on success..
 */
int tsd_time_input_selector_get_selected_input(uint32_t base, uint8_t *sel)
{
    uint32_t val = TSD_TIME_INPUT_SELECTOR_IORD_STATUS(base);
    uint32_t iSrc = (val & TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_SEL_MASK) >> TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_SEL_SHFT;

    if((iSrc >= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_0) && (iSrc <= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_7))
    {
        *sel = iSrc;
    }
    else
    {
        *sel = TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_0;
        return 1;
    }
    return 0;
}

/*
 * Gets the currently selected channel (0 based).
 * Note that this changes automatically by the core based on lock status and priority.
 * returns 0 on success..
 */
int tsd_time_input_selector_get_selected_input_type(uint32_t base, uint8_t *type)
{
    uint32_t val = TSD_TIME_INPUT_SELECTOR_IORD_STATUS(base);
    uint32_t iType = (val & TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_TYPE_MASK) >> TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_TYPE_SHFT;

    if((iType >= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_TYPE_INTERNAL) && (iType <= TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_TYPE_1PPS))
    {
        *type = iType;
    }
    else
    {
        *type = TSD_TIME_INPUT_SELECTOR_STATUS_INPUT_TYPE_INTERNAL;
        return 1;
    }
    return 0;
}

/*
 * Sets the priority for the time input n (see system design for info about what is connected to n)
 * Set prio to 0xF to disable time input.
 * lower priority numbers indicate higher priority
 * returns 0 on success..
 */
int tsd_time_input_selector_set_input_prio(uint32_t base, uint8_t channel, uint8_t prio)
{
    uint32_t prioVal = TSD_TIME_INPUT_SELECTOR_IORD_PRIO(base);
    uint32_t channelMask;

    if((prio < 0x10) && (channel < TSD_TIME_INPUT_SELECTOR_MAX_CHANNELS))
    {
        channelMask = 0xF << (4*channel);
        prioVal &= ~channelMask;
        prioVal |= (prio << (4*channel));
        TSD_TIME_INPUT_SELECTOR_IOWR_PRIO(base, prioVal);
    }
    else
    {
        return 1;
    }
    return 0;
}

/*
 * Gets the priority for the time input n (see system design for info about what is connected to n)
 * returns 0 on success..
 */
int tsd_time_input_selector_get_input_prio(uint32_t base, uint8_t channel, uint8_t *prio)
{
    uint32_t prioVal = TSD_TIME_INPUT_SELECTOR_IORD_PRIO(base);
    if((channel < TSD_TIME_INPUT_SELECTOR_MAX_CHANNELS))
    {
        *prio = (prioVal >> (4*channel)) & 0xF;
    }
    else
    {
        return 1;
    }
    return 0;
}

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
int tsd_time_input_selector_get_time(uint32_t base, int *years, int *ydays, int *hours, int *minutes, int *seconds, int *milliseconds)
{
    uint32_t timeL = TSD_TIME_INPUT_SELECTOR_IORD_TIME_L(base);
    uint32_t timeH = TSD_TIME_INPUT_SELECTOR_IORD_TIME_H(base);
    if((timeL == 0xDEADBEEF) || (timeH == 0xDEADBEEF))
    {
        return 1;
    }

    *milliseconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_TIME_INPUT_SELECTOR_TIME_L_BCD_MSEC_MASK) >> TSD_TIME_INPUT_SELECTOR_TIME_L_BCD_MSEC_RSHFT) );
    *seconds = bcd_u8_to_int( (uint8_t)((timeL & TSD_TIME_INPUT_SELECTOR_TIME_L_BCD_SECONDS_MASK) >> TSD_TIME_INPUT_SELECTOR_TIME_L_BCD_SECONDS_RSHFT) );
    *minutes = bcd_u8_to_int( (uint8_t)((timeL & TSD_TIME_INPUT_SELECTOR_TIME_L_BCD_MINUTES_MASK) >> TSD_TIME_INPUT_SELECTOR_TIME_L_BCD_MINUTES_RSHFT) );
    *hours = bcd_u8_to_int( (uint8_t)((timeL & TSD_TIME_INPUT_SELECTOR_TIME_L_BCD_HOURS_MASK) >> TSD_TIME_INPUT_SELECTOR_TIME_L_BCD_HOURS_RSHFT) );
    *ydays = bcd_u16_to_int( (uint16_t)((timeH & TSD_TIME_INPUT_SELECTOR_TIME_H_BCD_DAYS_MASK) >> TSD_TIME_INPUT_SELECTOR_TIME_H_BCD_DAYS_RSHFT) );
    *years = 2000 +
            bcd_u8_to_int( (uint8_t)((timeH & TSD_TIME_INPUT_SELECTOR_TIME_H_BCD_YEARS_MASK) >> TSD_TIME_INPUT_SELECTOR_TIME_H_BCD_YEARS_RSHFT) );

    return 0;
}

/*
 * Gets the time from the Time Input Selector core as struct timespec.
 */
int tsd_time_input_selector_get_time_ts(uint32_t base, struct timespec *ts)
{
    if(NULL == ts) return 1;
    uint32_t bcd_h = TSD_TIME_INPUT_SELECTOR_IORD_TIME_H(base);
    uint32_t bcd_l = TSD_TIME_INPUT_SELECTOR_IORD_TIME_L(base);

    get_timespec_from_bcd(bcd_l, bcd_h, ts);

    return 0;
}

/*
 * Gets the time from the Time Input Selector core as struct tm.
 */
int tsd_time_input_selector_get_time_tm(uint32_t base, struct tm *tmtime)
{
    if(NULL == tmtime) return 1;
    uint32_t bcd_h = TSD_TIME_INPUT_SELECTOR_IORD_TIME_H(base);
    uint32_t bcd_l = TSD_TIME_INPUT_SELECTOR_IORD_TIME_L(base);

    get_tm_from_bcd(bcd_l, bcd_h, tmtime);

    return 0;
}
