/*
 * tsd_shift_reg_led_core.h
 *
 *  Created on: May 14, 2018
 *      Author: eric
 */

#ifndef TSD_SHIFT_REG_LED_H_
#define TSD_SHIFT_REG_LED_H_

#include <stddef.h>
/* #include "stdint.h" */
#include "main.h"
#include "tsd_shift_reg_led_regs.h"
#include "tsd_shift_reg_led_types.h"

#define TSD_SHIFT_REG_LED_DRIVER_VERSION		(0.01)


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * prints a summary of the core's setup to stdout
 */
void tsd_shift_reg_led_debug_print(uint32_t base);

/*
 * Gets the Telspan TMATS core version
 *
 * returns 0 on success.
 */
int tsd_shift_reg_led_version(uint32_t base, double *fpgaVersion);

/*
 * Gets the Telspan TMATS core Driver version
 *
 * returns 0 on success.
 */
int tsd_shift_reg_led_driver_version(double *driverVersion);


/*
 * Enables the TMATS core
 */
int tsd_shift_reg_led_enable(uint32_t base);

/*
 * Disables the TMATS core
 */
int tsd_shift_reg_led_disable(uint32_t base);

/*
 * Reset control for the tmats core
 * resetOn = 1 : core in reset
 * resetOn = 0 : core not in reset.
 * In order to reset the core, set resetOn to 1, wait for at least 100us,
 * then set resetOn to 0.
 * Returns 0 on success
 */
int tsd_shift_reg_led_reset(uint32_t base, int resetOn);

/*
 * Sets the number of bits in the shift register (or shift register array)
 * Note: this is the number of bits contained within the shift register (array)
 * even if some bits are not connected, they are counted.
 * Bit number 0 starts at the first shift register 'A' output
 */
int tsd_shift_reg_led_set_num_bits(uint32_t base, int numBits);

/*
 * Sets the flash rate of the LED controller.  When a bit is set to flash, this is the
 * number of ticks that the timer will use before it toggles the output on/off.
 * The tick rate is based on the input clock to the module (typically the system clock rate).
 */
int tsd_shift_reg_led_set_flash_rate(uint32_t base, int clkTicks);

/*
 * clears all Led value and flash registers. (sets to 0)
 * returns 0 on success.
 */
int tsd_shift_reg_led_clear_all(uint32_t base, tsd_shift_reg_led_polarity_t polarity);

/*
 * Sets the shift register bit number bitNum to 1 (if on != 0) or 0 (if on == 0)
 * returns 0 on success.
 */
int tsd_shift_reg_led_set_bit_value(uint32_t base, int bitNum, int on, tsd_shift_reg_led_polarity_t polarity);

/*
 * Gets the shift register bit number bitNum and stores the value in on
 * returns 0 on success.
 */
int tsd_shift_reg_led_get_bit_value(uint32_t base, int bitNum, int *on, tsd_shift_reg_led_polarity_t polarity);

/*
 * Sets the shift register bit number bitNum to flashing (if flash != 0) or not flashing (if flash == 0)
 * returns 0 on success.
 */
int tsd_shift_reg_led_set_bit_flash(uint32_t base, int bitNum, int flash);

/*
 * Sets the shift register bit number bitNum to 1 (if on != 0) or 0 (if on == 0)
 * and to flashing (if flash != 0) or not flashing (if flash == 0)
 * returns 0 on success.
 */
int tsd_shift_reg_led_set_bit(uint32_t base, int bitNum, int on, int flash, tsd_shift_reg_led_polarity_t polarity);

/*
 * Sets the shift register bit numbers defined by led to appropriate values for color and flash
 * returns 0 on success.
 */
int tsd_shift_reg_led_set_led(uint32_t base, tsd_shift_reg_led_t led, tsd_shift_reg_led_color_t color, int flash, tsd_shift_reg_led_polarity_t polarity);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* TSD_SHIFT_REG_LED_H_ */
