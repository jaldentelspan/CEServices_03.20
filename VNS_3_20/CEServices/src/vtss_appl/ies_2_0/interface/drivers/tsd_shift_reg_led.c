/*
 * tsd_shift_reg_led_core.c
 *
 *  Created on: May 14, 2018
 *      Author: eric
 */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tsd_shift_reg_led.h"
#include "tsd_register_common.h"

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_shift_reg_led_debug_print(uint32_t base)
{
	double version;
	printf("Shift Register LED Controller Core at 0x%X\n", (unsigned)base);
	if(0 == tsd_shift_reg_led_version(base, &version))
	{
		printf(" - FPGA Version:  %f\n", version);
	}
	else
	{
		printf(" - FPGA Version:  FAILED TO READ\n");
	}
	if(0 == tsd_shift_reg_led_driver_version(&version))
	{
		printf(" - Driver Version:  %f\n", version);
	}
	else
	{
		printf(" - Driver Version:  FAILED TO READ\n");
	}

	printf(" - REGISTERS:\n");
	printf("    => %02X: VERSION REG   = 0x%08X\n", TSD_SHIFT_REG_LED_VERSION_REG, TSD_SHIFT_REG_LED_IORD_VERSION(base));
	printf("    => %02X: CONTROL REG   = 0x%08X\n", TSD_SHIFT_REG_LED_CONTROL_REG, TSD_SHIFT_REG_LED_IORD_CONTROL(base));
	printf("    => %02X: STATUS  REG   = 0x%08X\n", TSD_SHIFT_REG_LED_STATUS_REG, TSD_SHIFT_REG_LED_IORD_STATUS(base));
	printf("    => %02X: FLASH RATE    = 0x%08X\n", TSD_SHIFT_REG_LED_FLASH_RATE_REG, TSD_SHIFT_REG_LED_IORD_FLASH_RATE(base));
	printf("    => %02X: NUM BITS      = 0x%08X\n", TSD_SHIFT_REG_LED_NUM_BITS, TSD_SHIFT_REG_LED_IORD_NUM_BITS(base));
	printf("    => %02X: VALUE[31:0]   = 0x%08X\n", TSD_SHIFT_REG_LED_VALUE_31_0, TSD_SHIFT_REG_LED_IORD_VALUE_31_0(base));
	printf("    => %02X: VALUE[64:32]  = 0x%08X\n", TSD_SHIFT_REG_LED_VALUE_63_32, TSD_SHIFT_REG_LED_IORD_VALUE_63_32(base));
	printf("    => %02X: VALUE[95:64]  = 0x%08X\n", TSD_SHIFT_REG_LED_VALUE_95_64, TSD_SHIFT_REG_LED_IORD_VALUE_95_64(base));
	printf("    => %02X: VALUE[127:96] = 0x%08X\n", TSD_SHIFT_REG_LED_VALUE_127_96, TSD_SHIFT_REG_LED_IORD_VALUE_127_96(base));
	printf("    => %02X: FLASH[31:0]   = 0x%08X\n", TSD_SHIFT_REG_LED_FLASH_31_0, TSD_SHIFT_REG_LED_IORD_FLASH_31_0(base));
	printf("    => %02X: FLASH[64:32]  = 0x%08X\n", TSD_SHIFT_REG_LED_FLASH_63_32, TSD_SHIFT_REG_LED_IORD_FLASH_63_32(base));
	printf("    => %02X: FLASH[95:64]  = 0x%08X\n", TSD_SHIFT_REG_LED_FLASH_95_64, TSD_SHIFT_REG_LED_IORD_FLASH_95_64(base));
	printf("    => %02X: FLASH[127:96] = 0x%08X\n", TSD_SHIFT_REG_LED_FLASH_127_96, TSD_SHIFT_REG_LED_IORD_FLASH_127_96(base));
}

/*
 * Gets the Telspan TMATS core version
 *
 * returns 0 on success.
 */
int tsd_shift_reg_led_version(uint32_t base, double *fpgaVersion)
{
	uint32_t version = TSD_SHIFT_REG_LED_IORD_VERSION(base);
	uint32_t vH, vL;

	if(version == 0xDEADBEEF)
	{
		return 1;
	}

	vH = (version >> 16);
	vL = (version & 0xFFFF);
	if(vL < 100)
		*fpgaVersion = (double)vH + (double)((double)vL/(double)100);
	else
		*fpgaVersion = (double)vH + (double)((double)vL/(double)1000);

	return 0;
}

/*
 * Gets the Telspan TMATS core Driver version
 *
 * returns 0 on success.
 */
int tsd_shift_reg_led_driver_version(double *driverVersion)
{
	*driverVersion = (double)TSD_SHIFT_REG_LED_DRIVER_VERSION;
	return 0;
}

// TODO: ADD CONTROL AND STATUS ACCESS FUNCTIONS
/*
 * Enables the TMATS core
 */
int tsd_shift_reg_led_enable(uint32_t base)
{
	return set_register_bits(base, TSD_SHIFT_REG_LED_CONTROL_REG, TSD_SHIFT_REG_LED_CONTROL_ENABLE);
}

/*
 * Disables the TMATS core
 */
int tsd_shift_reg_led_disable(uint32_t base)
{
	return clear_register_bits(base, TSD_SHIFT_REG_LED_CONTROL_REG, TSD_SHIFT_REG_LED_CONTROL_ENABLE);
}

/*
 * Reset control for the tmats core
 * resetOn = 1 : core in reset
 * resetOn = 0 : core not in reset.
 * In order to reset the core, set resetOn to 1, wait for at least 100us,
 * then set resetOn to 0.
 * Returns 0 on success
 */
int tsd_shift_reg_led_reset(uint32_t base, int resetOn)
{
	if(resetOn)
		return set_register_bits(base, TSD_SHIFT_REG_LED_CONTROL_REG, TSD_SHIFT_REG_LED_CONTROL_RESET);
	else
		return clear_register_bits(base, TSD_SHIFT_REG_LED_CONTROL_REG, TSD_SHIFT_REG_LED_CONTROL_RESET);
}

/*
 * Sets the number of bits in the shift register (or shift register array)
 * Note: this is the number of bits contained within the shift register (array)
 * even if some bits are not connected, they are counted.
 * Bit number 0 starts at the first shift register 'A' output
 */
int tsd_shift_reg_led_set_num_bits(uint32_t base, int numBits)
{
	if((numBits >=128) || (numBits < 0)) return 1;
	TSD_SHIFT_REG_LED_IOWR_NUM_BITS(base, (numBits & TSD_SHIFT_REG_LED_NUM_BITS_MASK));
	return 0;
}

/*
 * Sets the flash rate of the LED controller.  When a bit is set to flash, this is the
 * number of ticks that the timer will use before it toggles the output on/off.
 * The tick rate is based on the input clock to the module (typically the system clock rate).
 */
int tsd_shift_reg_led_set_flash_rate(uint32_t base, int clkTicks)
{
	TSD_SHIFT_REG_LED_IOWR_FLASH_RATE(base, (clkTicks & TSD_SHIFT_REG_LED_FLASH_RATE_MASK));
	return 0;
}

/*
 * clears all Led value and flash registers. (sets to 0)
 * returns 0 on success.
 */
int tsd_shift_reg_led_clear_all(uint32_t base, tsd_shift_reg_led_polarity_t polarity)
{
	uint32_t val = ((polarity == TSD_SHIFT_REG_LED_ACTIVE_LOW) ? 0xFFFFFFFF : 0);
	uint32_t flash = 0;
	TSD_SHIFT_REG_LED_IOWR_VALUE_31_0(base, val);
	TSD_SHIFT_REG_LED_IOWR_VALUE_63_32(base, val);
	TSD_SHIFT_REG_LED_IOWR_VALUE_95_64(base, val);
	TSD_SHIFT_REG_LED_IOWR_VALUE_127_96(base, val);
	TSD_SHIFT_REG_LED_IOWR_FLASH_31_0(base, flash);
	TSD_SHIFT_REG_LED_IOWR_FLASH_63_32(base, flash);
	TSD_SHIFT_REG_LED_IOWR_FLASH_95_64(base, flash);
	TSD_SHIFT_REG_LED_IOWR_FLASH_127_96(base, flash);
	return 0;
}

/*
 * Sets the shift register bit number bitNum to 1 (if on != 0) or 0 (if on == 0)
 * returns 0 on success.
 */
int tsd_shift_reg_led_set_bit_value(uint32_t base, int bitNum, int on, tsd_shift_reg_led_polarity_t polarity)
{
	int regBitNum = 0;
	if((bitNum >=128) || (bitNum < 0)) return 1;

	if(polarity == TSD_SHIFT_REG_LED_ACTIVE_LOW)
		on = (on ? 0 : 1);

	if(bitNum < 32)
	{
		regBitNum = bitNum;
		if(on)
			return set_register_bits(base, TSD_SHIFT_REG_LED_VALUE_31_0, (1 << regBitNum));
		else
			return clear_register_bits(base, TSD_SHIFT_REG_LED_VALUE_31_0, (1 << regBitNum));
	}
	else if((bitNum >= 32) && (bitNum < 64))
	{
		regBitNum = bitNum - 32;
		if(on)
			return set_register_bits(base, TSD_SHIFT_REG_LED_VALUE_63_32, (1 << regBitNum));
		else
			return clear_register_bits(base, TSD_SHIFT_REG_LED_VALUE_63_32, (1 << regBitNum));
	}
	else if((bitNum >= 64) && (bitNum < 96))
	{
		regBitNum = bitNum - 64;
		if(on)
			return set_register_bits(base, TSD_SHIFT_REG_LED_VALUE_95_64, (1 << regBitNum));
		else
			return clear_register_bits(base, TSD_SHIFT_REG_LED_VALUE_95_64, (1 << regBitNum));
	}
	else
	{
		regBitNum = bitNum - 96;
		if(on)
			return set_register_bits(base, TSD_SHIFT_REG_LED_VALUE_127_96, (1 << regBitNum));
		else
			return clear_register_bits(base, TSD_SHIFT_REG_LED_VALUE_127_96, (1 << regBitNum));
	}

	return 0;
}

/*
 * Gets the shift register bit number bitNum and stores the value in on
 * returns 0 on success.
 */
int tsd_shift_reg_led_get_bit_value(uint32_t base, int bitNum, int *on, tsd_shift_reg_led_polarity_t polarity)
{
	int regBitNum = 0;
	uint32_t val;
	if((bitNum >=128) || (bitNum < 0)) return 1;

	if(bitNum < 32)
	{
		regBitNum = bitNum;
		get_register_region(base, TSD_SHIFT_REG_LED_VALUE_31_0, (1 << regBitNum), regBitNum, &val);
	}
	else if((bitNum >= 32) && (bitNum < 64))
	{
		regBitNum = bitNum - 32;
		get_register_region(base, TSD_SHIFT_REG_LED_VALUE_63_32, (1 << regBitNum), regBitNum, &val);
	}
	else if((bitNum >= 64) && (bitNum < 96))
	{
		regBitNum = bitNum - 64;
		get_register_region(base, TSD_SHIFT_REG_LED_VALUE_95_64, (1 << regBitNum), regBitNum, &val);
	}
	else
	{
		regBitNum = bitNum - 96;
		get_register_region(base, TSD_SHIFT_REG_LED_VALUE_127_96, (1 << regBitNum), regBitNum, &val);
	}

	if(polarity == TSD_SHIFT_REG_LED_ACTIVE_LOW)
		*on = (val ? 0 : 1);
	else
		*on = (val ? 1 : 0);

	return 0;
}

/*
 * Sets the shift register bit number bitNum to flashing (if flash != 0) or not flashing (if flash == 0)
 * returns 0 on success.
 */
int tsd_shift_reg_led_set_bit_flash(uint32_t base, int bitNum, int flash)
{
	int regBitNum = 0;
	if((bitNum >=128) || (bitNum < 0)) return 1;

	if(bitNum < 32)
	{
		regBitNum = bitNum;
		if(flash)
			return set_register_bits(base, TSD_SHIFT_REG_LED_FLASH_31_0, (1 << regBitNum));
		else
			return clear_register_bits(base, TSD_SHIFT_REG_LED_FLASH_31_0, (1 << regBitNum));
	}
	else if((bitNum >= 32) && (bitNum < 64))
	{
		regBitNum = bitNum - 32;
		if(flash)
			return set_register_bits(base, TSD_SHIFT_REG_LED_FLASH_63_32, (1 << regBitNum));
		else
			return clear_register_bits(base, TSD_SHIFT_REG_LED_FLASH_63_32, (1 << regBitNum));
	}
	else if((bitNum >= 64) && (bitNum < 96))
	{
		regBitNum = bitNum - 64;
		if(flash)
			return set_register_bits(base, TSD_SHIFT_REG_LED_FLASH_95_64, (1 << regBitNum));
		else
			return clear_register_bits(base, TSD_SHIFT_REG_LED_FLASH_95_64, (1 << regBitNum));
	}
	else
	{
		regBitNum = bitNum - 96;
		if(flash)
			return set_register_bits(base, TSD_SHIFT_REG_LED_FLASH_127_96, (1 << regBitNum));
		else
			return clear_register_bits(base, TSD_SHIFT_REG_LED_FLASH_127_96, (1 << regBitNum));
	}
	return 0;
}

/*
 * Sets the shift register bit number bitNum to 1 (if on != 0) or 0 (if on == 0)
 * and to flashing (if flash != 0) or not flashing (if flash == 0)
 * returns 0 on success.
 */
int tsd_shift_reg_led_set_bit(uint32_t base, int bitNum, int on, int flash, tsd_shift_reg_led_polarity_t polarity)
{
	int retval = 0;
	retval += tsd_shift_reg_led_set_bit_value(base, bitNum, on, polarity);
	retval += tsd_shift_reg_led_set_bit_flash(base, bitNum, flash);
	return retval;
}

/*
 * Sets the shift register bit numbers defined by led to appropriate values for color and flash
 * returns 0 on success.
 */
int tsd_shift_reg_led_set_led(uint32_t base, tsd_shift_reg_led_t led, tsd_shift_reg_led_color_t color, int flash, tsd_shift_reg_led_polarity_t polarity)
{
	int retval = 0;
	if(led.led_r_bit_num >= 0)
	{
		retval += tsd_shift_reg_led_set_bit(base, led.led_r_bit_num, (color & TSD_SHIFT_REG_LED_COLOR_RED), flash, polarity);
	}
	if(led.led_g_bit_num >= 0)
	{
		retval += tsd_shift_reg_led_set_bit(base, led.led_g_bit_num, (color & TSD_SHIFT_REG_LED_COLOR_GREEN), flash, polarity);
	}
	if(led.led_b_bit_num >= 0)
	{
		retval += tsd_shift_reg_led_set_bit(base, led.led_b_bit_num, (color & TSD_SHIFT_REG_LED_COLOR_BLUE), flash, polarity);
	}
	return retval;
}
