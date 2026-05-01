/*
 * tsd_shift_reg_led_regs.h
 *
 *  Created on: May 14, 2018
 *      Author: eric
 */

#ifndef TSD_SHIFT_REG_LED_REGS_H_
#define TSD_SHIFT_REG_LED_REGS_H_

#include <io.h>


/////////////////////////////////////////////////////////////////////
// REGISTERS
/////////////////////////////////////////////////////////////////////
#define TSD_SHIFT_REG_LED_VERSION_REG		(0x00)
#define TSD_SHIFT_REG_LED_CONTROL_REG		(0x01)
#define TSD_SHIFT_REG_LED_STATUS_REG		(0x02)
#define TSD_SHIFT_REG_LED_FLASH_RATE_REG	(0x03)
#define TSD_SHIFT_REG_LED_NUM_BITS			(0x04)
#define TSD_SHIFT_REG_LED_VALUE_31_0		(0x06)
#define TSD_SHIFT_REG_LED_VALUE_63_32		(0x07)
#define TSD_SHIFT_REG_LED_VALUE_95_64		(0x08)
#define TSD_SHIFT_REG_LED_VALUE_127_96		(0x09)
#define TSD_SHIFT_REG_LED_FLASH_31_0		(0x0A)
#define TSD_SHIFT_REG_LED_FLASH_63_32		(0x0B)
#define TSD_SHIFT_REG_LED_FLASH_95_64		(0x0C)
#define TSD_SHIFT_REG_LED_FLASH_127_96		(0x0D)

/////////////////////////////////////////////////////////////////////
// REGISTER BITS/MASKS
/////////////////////////////////////////////////////////////////////

#define TSD_SHIFT_REG_LED_CONTROL_ENABLE		(0x01)
#define TSD_SHIFT_REG_LED_CONTROL_RESET			(0x80000000)

#define TSD_SHIFT_REG_LED_FLASH_RATE_MASK		(0xFFFFFFFF)
#define TSD_SHIFT_REG_LED_NUM_BITS_MASK			(0x0000007F)

/////////////////////////////////////////////////////////////////////
// REGISTER ACCESS
/////////////////////////////////////////////////////////////////////

#define TSD_SHIFT_REG_LED_IORD_VERSION(base) \
	IORD(base, TSD_SHIFT_REG_LED_VERSION_REG)

#define TSD_SHIFT_REG_LED_IORD_CONTROL(base) \
	IORD(base, TSD_SHIFT_REG_LED_CONTROL_REG)
#define TSD_SHIFT_REG_LED_IOWR_CONTROL(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_CONTROL_REG, val)

#define TSD_SHIFT_REG_LED_IORD_STATUS(base) \
	IORD(base, TSD_SHIFT_REG_LED_STATUS_REG)

#define TSD_SHIFT_REG_LED_IORD_FLASH_RATE(base) \
	IORD(base, TSD_SHIFT_REG_LED_FLASH_RATE_REG)
#define TSD_SHIFT_REG_LED_IOWR_FLASH_RATE(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_FLASH_RATE_REG, val)

#define TSD_SHIFT_REG_LED_IORD_NUM_BITS(base) \
	IORD(base, TSD_SHIFT_REG_LED_NUM_BITS)
#define TSD_SHIFT_REG_LED_IOWR_NUM_BITS(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_NUM_BITS, val)

#define TSD_SHIFT_REG_LED_IORD_VALUE_31_0(base) \
	IORD(base, TSD_SHIFT_REG_LED_VALUE_31_0)
#define TSD_SHIFT_REG_LED_IOWR_VALUE_31_0(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_VALUE_31_0, val)

#define TSD_SHIFT_REG_LED_IORD_VALUE_63_32(base) \
	IORD(base, TSD_SHIFT_REG_LED_VALUE_63_32)
#define TSD_SHIFT_REG_LED_IOWR_VALUE_63_32(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_VALUE_63_32, val)

#define TSD_SHIFT_REG_LED_IORD_VALUE_95_64(base) \
	IORD(base, TSD_SHIFT_REG_LED_VALUE_95_64)
#define TSD_SHIFT_REG_LED_IOWR_VALUE_95_64(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_VALUE_95_64, val)

#define TSD_SHIFT_REG_LED_IORD_VALUE_127_96(base) \
	IORD(base, TSD_SHIFT_REG_LED_VALUE_127_96)
#define TSD_SHIFT_REG_LED_IOWR_VALUE_127_96(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_VALUE_127_96, val)

#define TSD_SHIFT_REG_LED_IORD_FLASH_31_0(base) \
	IORD(base, TSD_SHIFT_REG_LED_FLASH_31_0)
#define TSD_SHIFT_REG_LED_IOWR_FLASH_31_0(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_FLASH_31_0, val)

#define TSD_SHIFT_REG_LED_IORD_FLASH_63_32(base) \
	IORD(base, TSD_SHIFT_REG_LED_FLASH_63_32)
#define TSD_SHIFT_REG_LED_IOWR_FLASH_63_32(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_FLASH_63_32, val)

#define TSD_SHIFT_REG_LED_IORD_FLASH_95_64(base) \
	IORD(base, TSD_SHIFT_REG_LED_FLASH_95_64)
#define TSD_SHIFT_REG_LED_IOWR_FLASH_95_64(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_FLASH_95_64, val)

#define TSD_SHIFT_REG_LED_IORD_FLASH_127_96(base) \
	IORD(base, TSD_SHIFT_REG_LED_FLASH_127_96)
#define TSD_SHIFT_REG_LED_IOWR_FLASH_127_96(base, val) \
	IOWR(base, TSD_SHIFT_REG_LED_FLASH_127_96, val)


#endif /* TSD_SHIFT_REG_LED_REGS_H_ */
