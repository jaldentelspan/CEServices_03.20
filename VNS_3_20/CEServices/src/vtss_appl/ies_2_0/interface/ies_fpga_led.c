/*
 * led.cpp
 *
 *  Created on: Oct 22, 2019
 *      Author: eric
 */

#include <unistd.h>
#include "tsd_shift_reg_led.h"
#include "ies_fpga_led.h"


#define LED_POLARITY    TSD_SHIFT_REG_LED_ACTIVE_HIGH

const char * ELED_NAME_STRING[] = {
    [ IES_LED_STATUS ]      = "IES_LED_STATUS",
    [ IES_LED_TIME_STATUS ] = "IES_LED_TIME_STATUS",
    [ IES_LED_USER_01 ]     = "IES_LED_USER_01",
    [ IES_LED_GPS ]         = "IES_LED_GPS",
    [ IES_LED_PORT_01 ]     = "IES_LED_PORT_01",
    [ IES_LED_PORT_02 ]     = "IES_LED_PORT_02",
    [ IES_LED_PORT_03 ]     = "IES_LED_PORT_03",
    [ IES_LED_PORT_04 ]     = "IES_LED_PORT_04",
    [ IES_LED_PORT_05 ]     = "IES_LED_PORT_05",
    [ IES_LED_PORT_06 ]     = "IES_LED_PORT_06",
    [ IES_LED_PORT_07 ]     = "IES_LED_PORT_07",
    [ IES_LED_PORT_08 ]     = "IES_LED_PORT_08",
    [ IES_LED_PORT_09 ]     = "IES_LED_PORT_09",
    [ IES_LED_PORT_10 ]     = "IES_LED_PORT_10",
    [ IES_LED_PORT_11 ]     = "IES_LED_PORT_11",
    [ IES_LED_PORT_12 ]     = "IES_LED_PORT_12",
    [ IES_LED_PORT_13 ]     = "IES_LED_PORT_13",
    [ IES_LED_PORT_14 ]     = "IES_LED_PORT_14",
    [ IES_LED_PORT_15 ]     = "IES_LED_PORT_15",
    [ IES_LED_PORT_16 ]     = "IES_LED_PORT_16",
    [ IES_LED_UNUSED ]      = "IES_LED_UNUSED"
};



typedef struct sLED{
    sLEDState_t ledState;
    int led_r_bitnum; /* set to -1 if unused */
    int led_g_bitnum; /* set to -1 if unused */
    int led_b_bitnum; /* set to -1 if unused */
}sLED_t;

sLED_t ledArray[IES_LED_UNUSED];

static uint32_t ledControllerBase = 0xFFFFFFFF;

void InitLED(sLED_t * led, int r_bitnum, int g_bitnum, int b_bitnum)
{
    led->ledState.color = DH3_LED_BOARD_COLOR_OFF;
    led->ledState.flash = false;
    led->led_b_bitnum = b_bitnum;
    led->led_g_bitnum = g_bitnum;
    led->led_r_bitnum = r_bitnum;
}

int INIT_LED_SUBSYSTEM(uint32_t ledControllerBaseAddr, uint32_t sysClockFrequency, eFrontPanelBoard_t board)
{
    int i;
    ledControllerBase = ledControllerBaseAddr;
    int flashRateHz = 10; /* flash 10 times per second */
    int flashRateClkTicks = sysClockFrequency/flashRateHz;
    for(i = 0; i < IES_LED_UNUSED; i++)
    {
        InitLED(&ledArray[i], -1, -1, -1);
    }
    switch(board)
    {
    case INTERCONNECT_BOARD_1125_0004r1:
    case INTERCONNECT_BOARD_1125_0004r2:
    case INTERCONNECT_BOARD_1125_0004r3:
        tsd_shift_reg_led_set_num_bits(ledControllerBase, 40);
        tsd_shift_reg_led_set_flash_rate(ledControllerBase, flashRateClkTicks);
        tsd_shift_reg_led_clear_all(ledControllerBase, LED_POLARITY); /* clear all bits */

        // setup our led array
        InitLED(&ledArray[IES_LED_USER_01],     11, 12, 13);
        InitLED(&ledArray[IES_LED_STATUS],      8,  9, 10);
        InitLED(&ledArray[IES_LED_TIME_STATUS], 4,  5,  6);
        InitLED(&ledArray[IES_LED_GPS],         0,  1,  2);
        InitLED(&ledArray[IES_LED_PORT_01],     38, 39, -1);
        InitLED(&ledArray[IES_LED_PORT_02],     36, 37, -1);
        InitLED(&ledArray[IES_LED_PORT_03],     34, 35, -1);
        InitLED(&ledArray[IES_LED_PORT_04],     32, 33, -1);
        InitLED(&ledArray[IES_LED_PORT_05],     30, 31, -1);
        InitLED(&ledArray[IES_LED_PORT_06],     28, 29, -1);
        InitLED(&ledArray[IES_LED_PORT_07],     26, 27, -1);
        InitLED(&ledArray[IES_LED_PORT_08],     24, 25, -1);
        InitLED(&ledArray[IES_LED_PORT_09],     22, 23, -1);
        InitLED(&ledArray[IES_LED_PORT_10],     20, 21, -1);
        InitLED(&ledArray[IES_LED_PORT_11],     18, 19, -1);
        InitLED(&ledArray[IES_LED_PORT_12],     16, 17, -1);
        InitLED(&ledArray[IES_LED_PORT_13],     -1, -1, -1);
        InitLED(&ledArray[IES_LED_PORT_14],     -1, -1, -1);
        InitLED(&ledArray[IES_LED_PORT_15],     -1, -1, -1);
        InitLED(&ledArray[IES_LED_PORT_16],     -1, -1, -1);

        break;
    case INTERCONNECT_BOARD_1125_0004r4:
    case INTERCONNECT_BOARD_1125_0204r1:
    case INTERCONNECT_BOARD_1125_0204r2:
    case INTERCONNECT_BOARD_1125_0204r3:
    case INTERCONNECT_BOARD_1125_0204r4:
        tsd_shift_reg_led_set_num_bits(ledControllerBase, 48);
        tsd_shift_reg_led_set_flash_rate(ledControllerBase, flashRateClkTicks);
        tsd_shift_reg_led_clear_all(ledControllerBase, LED_POLARITY); /* clear all bits */

        // setup our led array
        InitLED(&ledArray[IES_LED_USER_01],     9, 10, 11);
        InitLED(&ledArray[IES_LED_STATUS],      6,  7,  8);
        InitLED(&ledArray[IES_LED_TIME_STATUS], 3,  4,  5);
        InitLED(&ledArray[IES_LED_GPS],         0,  1,  2);
        InitLED(&ledArray[IES_LED_PORT_01],     45, 46, 47);
        InitLED(&ledArray[IES_LED_PORT_02],     42, 43, 44);
        InitLED(&ledArray[IES_LED_PORT_03],     39, 40, 41);
        InitLED(&ledArray[IES_LED_PORT_04],     36, 37, 38);
        InitLED(&ledArray[IES_LED_PORT_05],     33, 34, 35);
        InitLED(&ledArray[IES_LED_PORT_06],     30, 31, 32);
        InitLED(&ledArray[IES_LED_PORT_07],     27, 28, 29);
        InitLED(&ledArray[IES_LED_PORT_08],     24, 25, 26);
        InitLED(&ledArray[IES_LED_PORT_09],     21, 22, 23);
        InitLED(&ledArray[IES_LED_PORT_10],     18, 19, 20);
        InitLED(&ledArray[IES_LED_PORT_11],     15, 16, 17);
        InitLED(&ledArray[IES_LED_PORT_12],     12, 13, 14);
        InitLED(&ledArray[IES_LED_PORT_13],     -1, -1, -1);
        InitLED(&ledArray[IES_LED_PORT_14],     -1, -1, -1);
        InitLED(&ledArray[IES_LED_PORT_15],     -1, -1, -1);
        InitLED(&ledArray[IES_LED_PORT_16],     -1, -1, -1);
        break;
    case INTERCONNECT_BOARD_1125_0222r1:
        tsd_shift_reg_led_set_num_bits(ledControllerBase, 8);
        tsd_shift_reg_led_set_flash_rate(ledControllerBase, flashRateClkTicks);
        tsd_shift_reg_led_clear_all(ledControllerBase, LED_POLARITY); /* clear all bits */

        InitLED(&ledArray[IES_LED_STATUS],      5,  6,  7);
        InitLED(&ledArray[IES_LED_TIME_STATUS],	 0,  1,  2);

        break;
    default:
        // DISABLE ALL
        break;
    }

    tsd_shift_reg_led_enable(ledControllerBase);

    return 0;
}

void CLEANUP_LED_SUBSYSTEM()
{
    int i;
    tsd_shift_reg_led_clear_all(ledControllerBase, LED_POLARITY); /* clear all bits */
    // DISABLE ALL
    for(i = 0; i < IES_LED_UNUSED; i++)
    {
        InitLED(&ledArray[i], -1, -1, -1);
    }
    tsd_shift_reg_led_disable(ledControllerBase);
}

int set_led(eLED_t led, eLEDColor_t color, bool flash)
{
    int result = 0;
    ledArray[led].ledState.color = color;
    ledArray[led].ledState.flash = flash;

        /* printf("PRINTF: Turning LED %d color %d fashing %s\n", (int)led, (int)color, flash ? "true" : "false" ); */
    if(ledArray[led].led_r_bitnum >= 0)
    {
        // SET RED BIT
        result += tsd_shift_reg_led_set_bit_value(ledControllerBase,
                ledArray[led].led_r_bitnum,
                (((int)color & DH3_LED_BOARD_COLOR_RED) ? 1 : 0),
                LED_POLARITY);
        result += tsd_shift_reg_led_set_bit_flash(ledControllerBase,
                ledArray[led].led_r_bitnum,
                (flash ? 1 : 0));
    }
    if(ledArray[led].led_g_bitnum >= 0)
    {
        // SET GREEN BIT
        result += tsd_shift_reg_led_set_bit_value(ledControllerBase,
                ledArray[led].led_g_bitnum,
                (((int)color & DH3_LED_BOARD_COLOR_GREEN) ? 1 : 0),
                LED_POLARITY);
        result += tsd_shift_reg_led_set_bit_flash(ledControllerBase,
                ledArray[led].led_g_bitnum,
                (flash ? 1 : 0));
    }
    if(ledArray[led].led_b_bitnum >= 0)
    {
        // SET BLUE BIT
        result += tsd_shift_reg_led_set_bit_value(ledControllerBase,
                ledArray[led].led_b_bitnum,
                (((int)color & DH3_LED_BOARD_COLOR_BLUE) ? 1 : 0),
                LED_POLARITY);
        result += tsd_shift_reg_led_set_bit_flash(ledControllerBase,
                ledArray[led].led_b_bitnum,
                (flash ? 1 : 0));
    }

    return result;
}

/* #include "nios_sleep.h" */
void test_leds()
{
    int i;
    tsd_shift_reg_led_clear_all(ledControllerBase, LED_POLARITY); /* clear all bits */
    // DISABLE ALL
    for(i = 0; i < IES_LED_UNUSED; i++)
    {
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_RED, false);
        sleep(1);
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_GREEN, false);
        sleep(1);
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_BLUE, false);
        sleep(1);
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_WHITE, true);
        sleep(2);
        set_led((eLED_t)i, DH3_LED_BOARD_COLOR_OFF, false);
    }
}
