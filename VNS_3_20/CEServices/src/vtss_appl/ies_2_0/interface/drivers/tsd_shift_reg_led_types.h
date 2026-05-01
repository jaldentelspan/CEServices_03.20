/*
 * tsd_shift_reg_led_types.h
 *
 *  Created on: Mar 12, 2019
 *      Author: eric
 */

#ifndef TSD_SHIFT_REG_LED_TYPES_H_
#define TSD_SHIFT_REG_LED_TYPES_H_


typedef enum tsd_shift_reg_led_color{
	TSD_SHIFT_REG_LED_COLOR_OFF = 0,
	TSD_SHIFT_REG_LED_COLOR_RED = 1,
	TSD_SHIFT_REG_LED_COLOR_BLUE = 2,
	TSD_SHIFT_REG_LED_COLOR_MAGENTA= 3,
	TSD_SHIFT_REG_LED_COLOR_GREEN = 4,
	TSD_SHIFT_REG_LED_COLOR_YELLOW = 5,
	TSD_SHIFT_REG_LED_COLOR_CYAN = 6,
	TSD_SHIFT_REG_LED_COLOR_WHITE = 7
}tsd_shift_reg_led_color_t;

typedef enum tsd_shift_reg_led_polarity{
	TSD_SHIFT_REG_LED_ACTIVE_HIGH,
	TSD_SHIFT_REG_LED_ACTIVE_LOW
}tsd_shift_reg_led_polarity_t;

typedef struct tsd_shift_reg_led
{
	int led_r_bit_num;						/* set to -1 if not used */
	int led_g_bit_num;						/* set to -1 if not used */
	int led_b_bit_num;						/* set to -1 if not used */
}tsd_shift_reg_led_t;




#endif /* TSD_SHIFT_REG_LED_TYPES_H_ */
