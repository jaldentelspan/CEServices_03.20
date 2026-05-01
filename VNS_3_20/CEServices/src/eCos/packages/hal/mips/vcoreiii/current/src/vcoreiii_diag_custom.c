//==========================================================================
//
//      vcoreiii_diag.c
//
//      Misc diagnostics (Customizable parts)
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    Lars Povlsen
// Contributors:
// Date:         2010-11-23
// Purpose:      Misc diagnostics (customizable parts)
// Description:
//
//####DESCRIPTIONEND####

#include <redboot.h>
#include <cyg/hal/plf_io.h>
#include <pkgconf/hal.h>
#include <cyg/hal/vcoreiii_diag.h>

/*
 * The following implementation is intended for customization support
 * for customer specific boards.
 */

#if 1
void led_mode(int mode)
{
    cyg_uint32 chipid = (VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID >> 12) & 0xFFFF;
    diag_printf("Chipid: %04x Telspan Data VNS Custom Build\n", chipid);
}

// note: copied from vcoreiii_diag.c, this overrides the weak alias defined there
// removed all led initialization for vns code... this includes sgpio init for gpio 0-3
//
void cyg_plf_redboot_startup(void)
{
//    int powerup_led = STATUS_LED_GREEN_ON;
	int powerup_led = 0;
#if defined(CYG_HAL_VCOREIII_CHIPTYPE_LUTON26)
    const char *board = "Unknown";
    int bit_count = 2;          /* Lu10 */
    cyg_uint32 port;
    cyg_uint32 chipid = (VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID >> 12) & 0xFFFF;

    if(chipid == 0x7428 || chipid == 0x7424) { /* Lu10 */
        board = "Luton10";
    } else {
        board = "Luton26";
    }

    diag_printf("%s board detected (VSC%04x Rev. %c).\n",
                board,
                (unsigned int)VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_PART_ID(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID),
                (char) (VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_REV_ID(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID)+'A'));

    /* Setup the serial IO clock frequency - 12.5MHz (0x14) */
    VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CLOCK = 0x14;

    /* Reset all SGPIO ports */
    for (port = 0; port < 32; port++)
        VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_CONFIG(port) = 0;

#elif defined(CYG_HAL_VCOREIII_CHIPTYPE_JAGUAR)
    gpio_out(22);               /* Green LED */
    gpio_out(23);               /* Red LED */

    diag_printf("%s board detected (VSC%04x Rev. %c).\n",
                "Jaguar-1",
                (unsigned int)VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_PART_ID(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID),
                (char) (VTSS_X_DEVCPU_GCB_CHIP_REGS_CHIP_ID_REV_ID(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID)+'A'));
#endif

    /* Not sure what this does, but I don't think it hurts anything */
    if(VTSS_DEVCPU_GCB_CHIP_REGS_HW_STAT & VTSS_F_DEVCPU_GCB_CHIP_REGS_HW_STAT_MEM_FAIL) {
        powerup_led = STATUS_LED_RED_ON;
        script = "diag -a\n";    /* Override automatic startup script */
        script_timeout = 1;      /* Speed up wait */
    }

    led_mode(powerup_led);
}

#endif
