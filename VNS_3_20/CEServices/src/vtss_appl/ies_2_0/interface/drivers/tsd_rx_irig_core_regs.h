/*
 * tsd_RX_IRIG_registers.h
 *
 *  Created on: Sep 5, 2017
 *      Author: eric
 */

#ifndef TSD_RX_IRIG_REGISTERS_H_
#define TSD_RX_IRIG_REGISTERS_H_

#include <io.h>

/**************************************
 * RX_IRIG DEFINES
 **************************************/
#define TSD_RX_IRIG_CONTROL_ENABLE			(0x01)
#define TSD_RX_IRIG_CONTROL_AM_CODE			(0x02) /* 1 = Amplitude modulated code, 0 = TTL code */
#define TSD_RX_IRIG_CONTROL_RESET			(0x80000000)

#define TSD_RX_IRIG_CODE_TYPE_B				(0x0)
#define TSD_RX_IRIG_CODE_TYPE_A				(0x1)
#define TSD_RX_IRIG_CODE_TYPE_G				(0x2)

#define TSD_RX_IRIG_STATUS_INTERNAL_ERR		(0x01)
#define TSD_RX_IRIG_STATUS_SETUP_ERR		(0x02)
#define TSD_RX_IRIG_STATUS_NO_SIGNAL_ERR	(0x04)
#define TSD_RX_IRIG_STATUS_BAD_SIGNAL_ERR	(0x08)
#define TSD_RX_IRIG_STATUS_NOT_LOCKED_ERR	(0x10)
#define TSD_RX_IRIG_STATUS_NOT_ENABLED_ERR	(0x20) /* RX_IRIG core not running... enable bit == 0 */

#define TSD_RX_IRIG_TIME_L_BCD_MSEC_MASK	(0x000000FF)
#define TSD_RX_IRIG_TIME_L_BCD_SECONDS_MASK	(0x0000FF00)
#define TSD_RX_IRIG_TIME_L_BCD_MINUTES_MASK	(0x00FF0000)
#define TSD_RX_IRIG_TIME_L_BCD_HOURS_MASK	(0xFF000000)

#define TSD_RX_IRIG_TIME_H_BCD_DAYS_MASK	(0x00000FFF)
#define TSD_RX_IRIG_TIME_H_BCD_YEARS_MASK	(0x00FF0000)

#define TSD_RX_IRIG_TIME_L_BCD_MSEC_RSHFT		(0)
#define TSD_RX_IRIG_TIME_L_BCD_SECONDS_RSHFT	(8)
#define TSD_RX_IRIG_TIME_L_BCD_MINUTES_RSHFT	(16)
#define TSD_RX_IRIG_TIME_L_BCD_HOURS_RSHFT		(24)

#define TSD_RX_IRIG_TIME_H_BCD_DAYS_RSHFT	(0)
#define TSD_RX_IRIG_TIME_H_BCD_HDAYS_RSHFT	(8)
#define TSD_RX_IRIG_TIME_H_BCD_YEARS_RSHFT	(16)

/**************************************
 * RX_IRIG REGISTERS
 **************************************/
#define TSD_RX_IRIG_VERSION_REG			(0x00)
#define TSD_RX_IRIG_CONTROL_REG			(0x01)
#define TSD_RX_IRIG_STATUS_REG			(0x02)
#define TSD_RX_IRIG_CODE_TYPE_REG		(0x03)
#define TSD_RX_IRIG_TIME_L_REG			(0x04)
#define TSD_RX_IRIG_TIME_H_REG			(0x05)
#define TSD_RX_IRIG_DELAY_CAL_REG		(0x06) /* calibration delay in system clock ticks */

/**************************************
 * RX_IRIG REGISTER ACCESS
 **************************************/

#define TSD_RX_IRIG_IORD_VERSION(base)			IORD(base, TSD_RX_IRIG_VERSION_REG)

#define TSD_RX_IRIG_IORD_CONTROL(base)			IORD(base, TSD_RX_IRIG_CONTROL_REG)
#define TSD_RX_IRIG_IOWR_CONTROL(base, val)		IOWR(base, TSD_RX_IRIG_CONTROL_REG, val)

#define TSD_RX_IRIG_IORD_CODE_TYPE(base)		IORD(base, TSD_RX_IRIG_CODE_TYPE_REG)
#define TSD_RX_IRIG_IOWR_CODE_TYPE(base, val)	IOWR(base, TSD_RX_IRIG_CODE_TYPE_REG, val)

#define TSD_RX_IRIG_IORD_STATUS(base)			IORD(base, TSD_RX_IRIG_STATUS_REG)

#define TSD_RX_IRIG_IORD_TIME_L(base)			IORD(base, TSD_RX_IRIG_TIME_L_REG)

#define TSD_RX_IRIG_IORD_TIME_H(base)			IORD(base, TSD_RX_IRIG_TIME_H_REG)

#define TSD_RX_IRIG_IORD_DELAY_CAL(base)		IORD(base, TSD_RX_IRIG_DELAY_CAL_REG)
#define TSD_RX_IRIG_IOWR_DELAY_CAL(base, val)	IOWR(base, TSD_RX_IRIG_DELAY_CAL_REG, val)

#endif /* TSD_RX_IRIG_REGISTERS_H_ */
