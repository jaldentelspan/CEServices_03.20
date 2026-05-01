/*
 * tsd_TX_IRIG_registers.h
 *
 *  Created on: Sep 5, 2017
 *      Author: eric
 */

#ifndef TSD_TX_IRIG_REGISTERS_H_
#define TSD_TX_IRIG_REGISTERS_H_

#include <io.h>

/**************************************
 * TX_IRIG DEFINES
 **************************************/
#define TSD_TX_IRIG_CONTROL_ENABLE            (0x01)
#define TSD_TX_IRIG_CONTROL_RX_LOOPBACK        (0x02)
#define TSD_TX_IRIG_CONTROL_DISABLE_CAL_2_RTC (0x04)
#define TSD_TX_IRIG_CONTROL_AM_AMPLITUDE_MASK    (0x18)
#define TSD_TX_IRIG_CONTROL_AM_AMPLITUDE_SHFT    (3)
#define TSD_TX_IRIG_CONTROL_CODE_TYPE_MASK    (0x00000700)
#define TSD_TX_IRIG_CONTROL_CODE_TYPE_SHFT    (8)
#define TSD_TX_IRIG_TTL_OUT_1PPS_MASK        (0x1000)
#define TSD_TX_IRIG_TTL_OUT_1PPS_SHFT        (12)
#define TSD_TX_IRIG_CONTROL_RESET            (0x80000000)

#define TSD_TX_IRIG_CODE_TYPE_B                (0x0)
#define TSD_TX_IRIG_CODE_TYPE_A                (0x1)
#define TSD_TX_IRIG_CODE_TYPE_G                (0x2)

#define TSD_TX_IRIG_AMPLITUDE_FULL            (0)
#define TSD_TX_IRIG_AMPLITUDE_HALF            (1)
#define TSD_TX_IRIG_AMPLITUDE_QUARTER        (2)
#define TSD_TX_IRIG_AMPLITUDE_EIGHTH        (3)

#define TSD_TX_IRIG_STATUS_INTERNAL_ERR        (0x01)
#define TSD_TX_IRIG_STATUS_SETUP_ERR        (0x02)
#define TSD_TX_IRIG_STATUS_NO_SIGNAL_ERR    (0x04)
#define TSD_TX_IRIG_STATUS_BAD_SIGNAL_ERR    (0x08)
#define TSD_TX_IRIG_STATUS_NOT_LOCKED_ERR    (0x10)
#define TSD_TX_IRIG_STATUS_NOT_ENABLED_ERR    (0x20) /* TX_IRIG core not running... enable bit == 0 */

#define TSD_TX_IRIG_CORE_RTC_TICKS_PER_SEC (10000000)

#define TSD_TX_IRIG_TIME_L_BCD_MSEC_MASK    (0x000000FF)
#define TSD_TX_IRIG_TIME_L_BCD_SECONDS_MASK    (0x0000FF00)
#define TSD_TX_IRIG_TIME_L_BCD_MINUTES_MASK    (0x00FF0000)
#define TSD_TX_IRIG_TIME_L_BCD_HOURS_MASK    (0xFF000000)

#define TSD_TX_IRIG_TIME_H_BCD_DAYS_MASK    (0x00000FFF)
#define TSD_TX_IRIG_TIME_H_BCD_YEARS_MASK    (0x00FF0000)

#define TSD_TX_IRIG_TIME_L_BCD_MSEC_RSHFT        (0)
#define TSD_TX_IRIG_TIME_L_BCD_SECONDS_RSHFT    (8)
#define TSD_TX_IRIG_TIME_L_BCD_MINUTES_RSHFT    (16)
#define TSD_TX_IRIG_TIME_L_BCD_HOURS_RSHFT        (24)

#define TSD_TX_IRIG_TIME_H_BCD_DAYS_RSHFT    (0)
#define TSD_TX_IRIG_TIME_H_BCD_HDAYS_RSHFT    (8)
#define TSD_TX_IRIG_TIME_H_BCD_YEARS_RSHFT    (16)
/**************************************
 * TX_IRIG REGISTERS
 **************************************/
#define TSD_TX_IRIG_VERSION_REG            (0x00)
#define TSD_TX_IRIG_CONTROL_REG            (0x01)
#define TSD_TX_IRIG_STATUS_REG            (0x02)
#define TSD_TX_IRIG_CAL_FINE_REG        (0x03)
#define TSD_TX_IRIG_CAL_MAJOR_REG        (0x04)
#define TSD_TX_IRIG_CAL_MINOR_REG        (0x05)
#define TSD_TX_IRIG_TIME_L_REG            (0x06)
#define TSD_TX_IRIG_TIME_H_REG            (0x07)
#define TSD_TX_IRIG_LATCHED_RTC_H_REG    (0x08)
#define TSD_TX_IRIG_LATCHED_RTC_L_REG    (0x09)
#define TSD_TX_IRIG_CURRENT_RTC_H_REG    (0x0A)
#define TSD_TX_IRIG_CURRENT_RTC_L_REG    (0x0B)


/**************************************
 * TX_IRIG REGISTER ACCESS
 **************************************/

#define TSD_TX_IRIG_IORD_VERSION(base)            IORD(base, TSD_TX_IRIG_VERSION_REG)

#define TSD_TX_IRIG_IORD_CONTROL(base)            IORD(base, TSD_TX_IRIG_CONTROL_REG)
#define TSD_TX_IRIG_IOWR_CONTROL(base, val)        IOWR(base, TSD_TX_IRIG_CONTROL_REG, val)

#define TSD_TX_IRIG_IORD_STATUS(base)            IORD(base, TSD_TX_IRIG_STATUS_REG)

#define TSD_TX_IRIG_IORD_CAL_FINE(base)            IORD(base, TSD_TX_IRIG_CAL_FINE_REG)
#define TSD_TX_IRIG_IOWR_CAL_FINE(base, val)    IOWR(base, TSD_TX_IRIG_CAL_FINE_REG, val)

#define TSD_TX_IRIG_IORD_CAL_MAJOR(base)        IORD(base, TSD_TX_IRIG_CAL_MAJOR_REG)
#define TSD_TX_IRIG_IOWR_CAL_MAJOR(base, val)    IOWR(base, TSD_TX_IRIG_CAL_MAJOR_REG, val)

#define TSD_TX_IRIG_IORD_CAL_MINOR(base)        IORD(base, TSD_TX_IRIG_CAL_MINOR_REG)
#define TSD_TX_IRIG_IOWR_CAL_MINOR(base, val)    IOWR(base, TSD_TX_IRIG_CAL_MINOR_REG, val)

#define TSD_TX_IRIG_IORD_TIME_L(base)            IORD(base, TSD_TX_IRIG_TIME_L_REG)

#define TSD_TX_IRIG_IORD_TIME_H(base)            IORD(base, TSD_TX_IRIG_TIME_H_REG)

#define TSD_TX_IRIG_IORD_LATCHED_RTC_MSW(base) \
    IORD(base, TSD_TX_IRIG_LATCHED_RTC_H_REG)

#define TSD_TX_IRIG_IORD_LATCHED_RTC_LSW(base) \
    IORD(base, TSD_TX_IRIG_LATCHED_RTC_L_REG)

#define TSD_TX_IRIG_IORD_CURRENT_RTC_MSW(base) \
    IORD(base, TSD_TX_IRIG_CURRENT_RTC_H_REG)

#define TSD_TX_IRIG_IORD_CURRENT_RTC_LSW(base) \
    IORD(base, TSD_TX_IRIG_CURRENT_RTC_L_REG)

#endif /* TSD_TX_IRIG_REGISTERS_H_ */
