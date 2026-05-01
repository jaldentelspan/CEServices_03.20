/*
 * led.h
 *
 *  Created on: Oct 22, 2019
 *      Author: eric
 */

#ifndef LED_H_
#define LED_H_

/* #include "stdint.h" */
/* #include "stdbool.h" */
#include "main.h"
#include "vtss_api.h"

typedef enum eFrontPanelBoard {
    /* TODO: MAKE THESE USEFUL NAMES - VALUE: 0x00BBBBrr (BBBB is last 4 digits in board number (4 bytes), rr = revision number (1 byte)*/
    INTERCONNECT_BOARD_UNKNOWN = 0,
    INTERCONNECT_BOARD_1125_0004r1 = 0x000401,
    INTERCONNECT_BOARD_1125_0004r2 = 0x000402,
    INTERCONNECT_BOARD_1125_0004r3 = 0x000403,
    INTERCONNECT_BOARD_1125_0004r4 = 0x000404,
    INTERCONNECT_BOARD_1125_0204r1 = 0x020401,
    INTERCONNECT_BOARD_1125_0204r2 = 0x020402,
    INTERCONNECT_BOARD_1125_0204r3 = 0x020403,
    INTERCONNECT_BOARD_1125_0204r4 = 0x020404,
    INTERCONNECT_BOARD_1125_0222r1 = 0x022201,
    /* ADD NEW BOARDS HERE */
    INTERCONNECT_BOARD_UNUSED
}eFrontPanelBoard_t;

typedef enum eLED{
    IES_LED_STATUS,
    IES_LED_TIME_STATUS,
    IES_LED_USER_01,
    IES_LED_GPS,
    IES_LED_PORT_01,
    IES_LED_PORT_02,
    IES_LED_PORT_03,
    IES_LED_PORT_04,
    IES_LED_PORT_05,
    IES_LED_PORT_06,
    IES_LED_PORT_07,
    IES_LED_PORT_08,
    IES_LED_PORT_09,
    IES_LED_PORT_10,
    IES_LED_PORT_11,
    IES_LED_PORT_12,
    IES_LED_PORT_13,
    IES_LED_PORT_14,
    IES_LED_PORT_15,
    IES_LED_PORT_16,
    IES_LED_UNUSED
}eLED_t;

extern const char * ELED_NAME_STRING[];

typedef enum eLEDColor{
    DH3_LED_BOARD_COLOR_OFF     = 0,
    DH3_LED_BOARD_COLOR_BLUE    = 1,
    DH3_LED_BOARD_COLOR_GREEN   = 2,
    DH3_LED_BOARD_COLOR_CYAN    = 3,
    DH3_LED_BOARD_COLOR_RED     = 4,
    DH3_LED_BOARD_COLOR_MAGENTA = 5,
    DH3_LED_BOARD_COLOR_YELLOW  = 6,
    DH3_LED_BOARD_COLOR_WHITE   = 7,
}eLEDColor_t;


typedef struct sLEDState{
    eLEDColor_t color;
    bool flash;
}sLEDState_t;

int INIT_LED_SUBSYSTEM(uint32_t ledControllerBaseAddr, uint32_t sysClockFrequency, eFrontPanelBoard_t board);

void CLEANUP_LED_SUBSYSTEM();

int set_led(eLED_t led, eLEDColor_t color, bool flash);

void test_leds();





#endif /* LED_H_ */
