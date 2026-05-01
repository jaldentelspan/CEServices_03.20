/*
 * tsd_RX_1PPS_CORE_registers.h
 *
 *  Created on: Sep 5, 2017
 *      Author: eric
 */

#ifndef TSD_RX_1PPS_CORE_REGISTERS_H_
#define TSD_RX_1PPS_CORE_REGISTERS_H_

#include <io.h>

#define TSD_RX_1PPS_CORE_MAX_CHANNELS       (8)
/**************************************
 * RX_1PPS_CORE DEFINES
 **************************************/
#define TSD_RX_1PPS_CORE_CONTROL_ENABLE     (0x01)
#define TSD_RX_1PPS_CORE_TIME_VALID_BIT     (0x02)
#define TSD_RX_1PPS_CORE_CONTROL_RESET      (0x80000000)


#define TSD_RX_1PPS_CORE_TIME_H_YDAY_MASK   (0x00000FFF)
#define TSD_RX_1PPS_CORE_TIME_H_YDAY_SHFT   (0)
#define TSD_RX_1PPS_CORE_TIME_H_YEAR_MASK   (0x00FF0000)
#define TSD_RX_1PPS_CORE_TIME_H_YEAR_SHFT   (16)

#define TSD_RX_1PPS_CORE_TIME_L_MSEC_MASK   (0x000000FF)
#define TSD_RX_1PPS_CORE_TIME_L_MSEC_SHFT   (0)
#define TSD_RX_1PPS_CORE_TIME_L_SEC_MASK    (0x0000FF00)
#define TSD_RX_1PPS_CORE_TIME_L_SEC_SHFT    (8)
#define TSD_RX_1PPS_CORE_TIME_L_MIN_MASK    (0x00FF0000)
#define TSD_RX_1PPS_CORE_TIME_L_MIN_SHFT    (16)
#define TSD_RX_1PPS_CORE_TIME_L_HOUR_MASK   (0xFF000000)
#define TSD_RX_1PPS_CORE_TIME_L_HOUR_SHFT   (24)

/**************************************
 * RX_1PPS_CORE REGISTERS
 **************************************/
#define TSD_RX_1PPS_CORE_VERSION_REG        (0x00)
#define TSD_RX_1PPS_CORE_CONTROL_REG        (0x01)
#define TSD_RX_1PPS_CORE_STATUS_REG         (0x02)
#define TSD_RX_1PPS_CORE_TIME_LOW_REG       (0x03)
#define TSD_RX_1PPS_CORE_TIME_HIGH_REG      (0x04)
#define TSD_RX_1PPS_CORE_WINDOW_MIN_REG     (0x05)
#define TSD_RX_1PPS_CORE_WINDOW_MAX_REG     (0x06)


/**************************************
 * RX_1PPS_CORE REGISTER ACCESS
 **************************************/

#define TSD_RX_1PPS_CORE_IORD_VERSION(base)         IORD(base, TSD_RX_1PPS_CORE_VERSION_REG)

#define TSD_RX_1PPS_CORE_IORD_CONTROL(base)         IORD(base, TSD_RX_1PPS_CORE_CONTROL_REG)
#define TSD_RX_1PPS_CORE_IOWR_CONTROL(base, val)    IOWR(base, TSD_RX_1PPS_CORE_CONTROL_REG, val)

#define TSD_RX_1PPS_CORE_IORD_STATUS(base)          IORD(base, TSD_RX_1PPS_CORE_STATUS_REG)

#define TSD_RX_1PPS_CORE_IORD_TIME_HIGH(base)       IORD(base, TSD_RX_1PPS_CORE_TIME_HIGH_REG)
#define TSD_RX_1PPS_CORE_IOWR_TIME_HIGH(base, val)  IOWR(base, TSD_RX_1PPS_CORE_TIME_HIGH_REG, val)

#define TSD_RX_1PPS_CORE_IORD_TIME_LOW(base)        IORD(base, TSD_RX_1PPS_CORE_TIME_LOW_REG)
#define TSD_RX_1PPS_CORE_IOWR_TIME_LOW(base, val)   IOWR(base, TSD_RX_1PPS_CORE_TIME_LOW_REG, val)

#define TSD_RX_1PPS_CORE_IORD_WINDOW_MIN(base)      IORD(base, TSD_RX_1PPS_CORE_WINDOW_MIN_REG)
#define TSD_RX_1PPS_CORE_IOWR_WINDOW_MIN(base, val) IOWR(base, TSD_RX_1PPS_CORE_WINDOW_MIN_REG, val)

#define TSD_RX_1PPS_CORE_IORD_WINDOW_MAX(base)      IORD(base, TSD_RX_1PPS_CORE_WINDOW_MAX_REG)
#define TSD_RX_1PPS_CORE_IOWR_WINDOW_MAX(base, val) IOWR(base, TSD_RX_1PPS_CORE_WINDOW_MAX_REG, val)

#endif /* TSD_RX_1PPS_CORE_REGISTERS_H_ */
