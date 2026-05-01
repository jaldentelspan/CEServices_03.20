/*
 * tsd_time_bcd.h
 *
 *  Created on: Oct 24, 2019
 *      Author: eric
 */

#ifndef TSD_TIME_BCD_H_
#define TSD_TIME_BCD_H_

/* #include <stdint.h> */
#include "main.h"
#include <time.h>

#define TSD_BCD_TIME_TDL_HOURS_MASK                (0xFF000000)
#define TSD_BCD_TIME_TDL_HOURS_SHFT                (24)
#define TSD_BCD_TIME_TDL_MINUTES_MASK            (0x00FF0000)
#define TSD_BCD_TIME_TDL_MINUTES_SHFT            (16)
#define TSD_BCD_TIME_TDL_SECONDS_MASK            (0x0000FF00)
#define TSD_BCD_TIME_TDL_SECONDS_SHFT            (8)
#define TSD_BCD_TIME_TDL_MILLISECONDS_MASK        (0x000000FF)
#define TSD_BCD_TIME_TDL_MILLISECONDS_SHFT        (0)

#define TSD_BCD_TIME_TDH_YEARS_MASK                (0x00FF0000)
#define TSD_BCD_TIME_TDH_YEARS_SHFT                (16)
#define TSD_BCD_TIME_TDH_HDAYS_MASK                (0x00000F00)
#define TSD_BCD_TIME_TDH_HDAYS_SHFT                (8)
#define TSD_BCD_TIME_TDH_TDAYS_MASK                (0x000000F0)
#define TSD_BCD_TIME_TDH_TDAYS_SHFT                (4)
#define TSD_BCD_TIME_TDH_DAYS_MASK                (0x0000000F)
#define TSD_BCD_TIME_TDH_DAYS_SHFT                (0)

static __inline__ int __attribute__ ((always_inline)) bcd_u8_to_int(uint8_t bcd)
{
    return ( (bcd & 0x0F) + (((bcd >> 4) & 0x0F) * 10) );
}

static __inline__ int __attribute__ ((always_inline)) bcd_u16_to_int(uint16_t bcd)
{
    return ( (bcd & 0x0F) + (((bcd >> 4) & 0x0F) * 10) + (((bcd >> 8) & 0x0F) * 100) );
}

/*
 * converts the BCD on from the FPGA registers to a struct tm time.
 * timeDataL formatted as BCD: 0xHHMMSShh (hours bcd, minutes bcd, seconds bcd, 100ths of seconds bcd)
 * timeDataH formatted as BCD: 0x00YY0DDD (YY = BCD years since 2000, DDD = BCD day of year);
 * returns time_t value for the same time that will be stored in tmtime
 * if ts is NULL, only returns time_t value.
 */
time_t get_tm_from_bcd(uint32_t timeDataL, uint32_t timeDataH, struct tm* tmtime);

/*
 * converts the BCD on from the FPGA registers to a struct timespec time.
 * timeDataL formatted as BCD: 0xHHMMSShh (hours bcd, minutes bcd, seconds bcd, 100ths of seconds bcd)
 * timeDataH formatted as BCD: 0x00YY0DDD (YY = BCD years since 2000, DDD = BCD day of year);
 * returns time_t value for the same time that will be stored in tmtime
 * if ts is NULL, only returns time_t value.
 */
time_t get_timespec_from_bcd(uint32_t timeDataL, uint32_t timeDataH, struct timespec* ts);

/*
 * converts struct tm time to BCD.
 * tmtime (input) is not modified, used to generate bcd into timeDataL and timeDataH
 * timeDataL (output) formatted as BCD: 0xHHMMSShh (hours bcd, minutes bcd, seconds bcd, 100ths of seconds bcd)
 * timeDataH (output) formatted as BCD: 0x00YY0DDD (YY = BCD years since 2000, DDD = BCD day of year);
 * returns 0 on success;
 */
int get_bcd_from_tm(struct tm *tmtime, uint32_t *timeDataL, uint32_t *timeDataH);

/*
 * converts the struct timespec to BCD.
 * ts (input) is not modified, used to generate bcd into timeDataL and timeDataH
 * timeDataL formatted as BCD: 0xHHMMSShh (hours bcd, minutes bcd, seconds bcd, 100ths of seconds bcd)
 * timeDataH formatted as BCD: 0x00YY0DDD (YY = BCD years since 2000, DDD = BCD day of year);
 * returns 0 on success;
 */
int get_bcd_from_timespec(struct timespec *ts, uint32_t *timeDataL, uint32_t *timeDataH);

#endif /* TSD_TIME_BCD_H_ */
