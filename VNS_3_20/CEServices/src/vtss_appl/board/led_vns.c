/* -*- Mode: C; c-basic-offset: 2; tab-width: 8; c-comment-only-line-offset: 0; -*- */
/*

 Vitesse Switch API software.

 Copyright (c) 2002-2012 Vitesse Semiconductor Corporation "Vitesse". All
 Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted. Permission to
 integrate into other products, disclose, transmit and distribute the software
 in an absolute machine readable format (e.g. HEX file) is also granted.  The
 source code of the software may not be disclosed, transmitted or distributed
 without the written permission of Vitesse. The software and its source code
 may only be used in products utilizing the Vitesse switch products.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software. Vitesse retains all ownership,
 copyright, trade secret and proprietary rights in the software.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
 INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR USE AND NON-INFRINGEMENT.

*/

#include "main.h"
#include "led_api.h"
#include "vtss_misc_api.h"
#include "port_custom_api.h"

/**
 * \led_vns.c
 * \brief This file contains the code for LED control for the Luton26 hardware platform.
 *
 *  The Luton26 hardware platform consists of 2 chip sets. The Luton26 chip, and a Atom12
 *  chip set. Both chip sets are used for controlling their corresponding port LEDs.
 *
 *
 */


/* SGPIO LED mapping */
typedef struct {
    u8  port;
    u8  bit;
} sgpio_mapping_t;


/****************************************************************************/
/*                                                                          */
/*  MODULE INTERNAL FUNCTIONS - LED board dependent functions               */
/*                                                                          */
/****************************************************************************/

/* The reference implementation - vns_6  */

void LED_led_init(void)
{

}


/* The reference implementation -  luton26_ref */
void LED_front_led_set(LED_led_colors_t color)
{
    static LED_led_colors_t old_color;

    if (old_color == color) {
        return;
    } else {
        old_color = color;
    }

}

