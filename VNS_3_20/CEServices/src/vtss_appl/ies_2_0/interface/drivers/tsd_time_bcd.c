/*
 * tsd_time_bcd.c
 *
 *  Created on: Oct 24, 2019
 *      Author: eric
 */
#include <string.h>
#include "tsd_time_bcd.h"



/*
 * converts the BCD on from the FPGA registers to a struct tm time.
 * timeDataL formatted as BCD: 0xHHMMSShh (hours bcd, minutes bcd, seconds bcd, 100ths of seconds bcd)
 * timeDataH formatted as BCD: 0x00YY0DDD (YY = BCD years since 2000, DDD = BCD day of year);
 * returns time_t value for the same time that will be stored in tmtime
 * if tmtime is NULL, only returns time_t value.
 */
time_t get_tm_from_bcd(uint32_t timeDataL, uint32_t timeDataH, struct tm* tmtime)
{
    struct tm tmtemp;
    uint8_t bcd_year  = (timeDataH & TSD_BCD_TIME_TDH_YEARS_MASK)   >> TSD_BCD_TIME_TDH_YEARS_SHFT;
    uint16_t bcd_hday = (timeDataH & TSD_BCD_TIME_TDH_HDAYS_MASK)   >> TSD_BCD_TIME_TDH_HDAYS_SHFT;
    uint16_t bcd_tday = (timeDataH & TSD_BCD_TIME_TDH_TDAYS_MASK)   >> TSD_BCD_TIME_TDH_TDAYS_SHFT;
    uint16_t bcd_day  = (timeDataH & TSD_BCD_TIME_TDH_DAYS_MASK)    >> TSD_BCD_TIME_TDH_DAYS_SHFT;
    uint8_t bcd_hour  = (timeDataL & TSD_BCD_TIME_TDL_HOURS_MASK)   >> TSD_BCD_TIME_TDL_HOURS_SHFT;
    uint8_t bcd_min   = (timeDataL & TSD_BCD_TIME_TDL_MINUTES_MASK) >> TSD_BCD_TIME_TDL_MINUTES_SHFT;
    uint8_t bcd_sec   = (timeDataL & TSD_BCD_TIME_TDL_SECONDS_MASK) >> TSD_BCD_TIME_TDL_SECONDS_SHFT;

    if(NULL == tmtime)
    {
        tmtime = &tmtemp;
    }

    memset(tmtime, 0, sizeof(struct tm));

    tmtime->tm_sec = (bcd_sec & 0xF) + (10 * ((bcd_sec >> 4) & 0xF));
    tmtime->tm_min = (bcd_min & 0xF) + (10 * ((bcd_min >> 4) & 0xF));
    tmtime->tm_hour = (bcd_hour & 0xF) + (10 * ((bcd_hour >> 4) & 0xF));
    tmtime->tm_year = 100 + (bcd_year & 0xF) + (10 * ((bcd_year >> 4) & 0xF)); // tm_year 0 is year 1900
    //fixme: bug if time source provides 0x70 for year (typically meaning 1970)..
    //       the following is the current hack to deal with it (until 2070)
    if(tmtime->tm_year >= 170) tmtime->tm_year -= 100;
    tmtime->tm_mon = 0; // set this to 0, and set tm_mday to year day...
                        // tm_yday will be 1 less after mktime call, but tmtime will be correct
    tmtime->tm_mday = (bcd_day) + (10 * bcd_tday) + (100 * bcd_hday);
    if(0 == tmtime->tm_mday)
        tmtime->tm_mday = 1; // CANNOT BE 0

    return mktime(tmtime);
}

/*
 * converts the BCD on from the FPGA registers to a struct tm time.
 * timeDataL formatted as BCD: 0xHHMMSShh (hours bcd, minutes bcd, seconds bcd, 100ths of seconds bcd)
 * timeDataH formatted as BCD: 0x00YY0DDD (YY = BCD years since 2000, DDD = BCD day of year);
 * returns time_t value for the same time that will be stored in tmtime
 * if ts is NULL, only returns time_t value.
 */
time_t get_timespec_from_bcd(uint32_t timeDataL, uint32_t timeDataH, struct timespec* ts)
{
    uint8_t bcd_msec   = (timeDataL & TSD_BCD_TIME_TDL_MILLISECONDS_MASK) >> TSD_BCD_TIME_TDL_MILLISECONDS_SHFT;

    if(NULL == ts)
    {
        return get_tm_from_bcd(timeDataL, timeDataH, NULL);
    }
    ts->tv_sec = get_tm_from_bcd(timeDataL, timeDataH, NULL);
    ts->tv_nsec = (bcd_msec & 0xF) + (10 * ((bcd_msec >> 4) & 0xF)) * 10000000;
    return ts->tv_sec;
}

/*
 * converts struct tm time to BCD.
 * tmtime (input) is not modified, used to generate bcd into timeDataL and timeDataH
 * timeDataL (output) formatted as BCD: 0xHHMMSShh (hours bcd, minutes bcd, seconds bcd, 100ths of seconds bcd)
 * timeDataH (output) formatted as BCD: 0x00YY0DDD (YY = BCD years since 2000, DDD = BCD day of year);
 * returns 0 on success;
 */
int get_bcd_from_tm(struct tm *tmtime, uint32_t *timeDataL, uint32_t *timeDataH)
{
    if((NULL == tmtime) || (NULL == timeDataL) || (NULL == timeDataH)) return 1;
    // just to be sure things are correct:
    mktime(tmtime);
    int yearsData = tmtime->tm_year % 100; // only use 2 digits
    *timeDataH = 0;
    *timeDataH |=  (yearsData / 10) << 20;
    *timeDataH |= (yearsData % 10) << 16;
    /* tm_yday is 0 - 356, bcd time is 1 - 366, so add 1 */
    tmtime->tm_yday += 1;
    *timeDataH |= (tmtime->tm_yday / 100) << 8;
    *timeDataH |= ((tmtime->tm_yday % 100) / 10) << 4;
    *timeDataH |= (tmtime->tm_yday % 10) << 0;

    *timeDataL = 0;
    *timeDataL |= (tmtime->tm_hour / 10) << 28;
    *timeDataL |= (tmtime->tm_hour % 10) << 24;
    *timeDataL |= (tmtime->tm_min / 10) << 20;
    *timeDataL |= (tmtime->tm_min % 10) << 16;
    *timeDataL |= (tmtime->tm_sec / 10) << 12;
    *timeDataL |= (tmtime->tm_sec % 10) << 8;

    return 0;
}

/*
 * converts the struct timespec to BCD.
 * ts (input) is not modified, used to generate bcd into timeDataL and timeDataH
 * timeDataL formatted as BCD: 0xHHMMSShh (hours bcd, minutes bcd, seconds bcd, 100ths of seconds bcd)
 * timeDataH formatted as BCD: 0x00YY0DDD (YY = BCD years since 2000, DDD = BCD day of year);
 * returns 0 on success;
 */
int get_bcd_from_timespec(struct timespec *ts, uint32_t *timeDataL, uint32_t *timeDataH)
{
    if((NULL == ts) || (NULL == timeDataL) || (NULL == timeDataH)) return 1;

    long hsec = ts->tv_nsec / 10000000; /* get hundreths of seconds from nanoseconds */
    struct tm *tmtime = gmtime(&(ts->tv_sec));
    if(get_bcd_from_tm(tmtime, timeDataL, timeDataH))
        return 1;

    *timeDataL |= (hsec/ 10) << 4;
    *timeDataL |= (hsec % 10) << 0;

    return 0;
}
