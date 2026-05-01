/*
 * tsd_register_common.h
 *
 *  Created on: Oct 2, 2018
 *      Author: eric
 */

#ifndef TSD_REGISTER_COMMON_H_
#define TSD_REGISTER_COMMON_H_

/* #include "stdint.h" */
#include "main.h"
#include "io.h"

#ifdef __NIOS2__
#include <stdio.h>
#define mmdrv_printf printf
#endif
#include <stdio.h>
#define mmdrv_printf printf

/*
 * sets only the register bits that are set in the bits argument.
 * THIS FUNCTION ALWAYS RETURNS 0
 */
static __inline__ int __attribute__ ((always_inline)) set_register_bits(uint32_t base, uint32_t offset, uint32_t bits)
{
    uint32_t regVal = IORD(base, offset);
    if(regVal != (regVal | bits)){
        regVal |= bits;
        IOWR(base, offset, regVal);
    }
    return 0;
}

/*
 * clears only the register bits that are set in the bits argument
 * THIS FUNCTION ALWAYS RETURNS 0
 */
static __inline__ int __attribute__ ((always_inline)) clear_register_bits(uint32_t base, uint32_t offset, uint32_t bits)
{
    uint32_t regVal = IORD(base, offset);
    if(regVal != (regVal & ~bits)){
        regVal &= ~bits;
        IOWR(base, offset, regVal);
    }
    return 0;
}

/*
 * sets a region of a register at base[offset].
 * the region is defined by the mask (mask out valid bits) and the shift (number of bits to left shift the value)
 * basically, this function reads the register, clears the masked region, then sets value into the masked region.
 * the unmasked region of the register is left untouched.
 * for example:
 *         register initial value == 0xAAAAAAAA
 *         mask = 0x0000ff00
 *         shift (must be) = 8
 *         value = 55
 *         - the register will be set to 0xAAAA55AA
 * returns 0 on success
 */
static __inline__ int __attribute__ ((always_inline)) set_register_region(uint32_t base, uint32_t offset, uint32_t mask, uint32_t shift, uint32_t value)
{
    uint32_t regVal = IORD(base, offset);
    if(shift > 31)
    {
        return 1;
    }
    regVal &= ~mask;
    regVal |= ((value << shift) & mask);
    IOWR(base, offset, regVal);
    return 0;
}

/*
 * gets a region of a register at base[offset].
 * the region is defined by the mask (mask out valid bits) and the shift (number of bits from 0 where the region begins)
 * basically, this function reads the register, masks out un-masked bits, then sets value to the value in the masked region.
 * for example:
 *         register initial value == 0x12345678
 *         mask = 0x0000f000
 *         shift (must be) = 12
 *         *value will be set to 5
 * returns 0 on success
 */
static __inline__ int __attribute__ ((always_inline)) get_register_region(uint32_t base, uint32_t offset, uint32_t mask, uint32_t shift, uint32_t *value)
{
    uint32_t regVal = IORD(base, offset);
    if(shift > 31)
    {
        return 1;
    }
    *value = ((regVal & mask) >> shift);
    return 0;
}

#endif /* TSD_REGISTER_COMMON_H_ */
