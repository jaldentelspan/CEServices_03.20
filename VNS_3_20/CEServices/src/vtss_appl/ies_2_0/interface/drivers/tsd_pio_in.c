/*
 *
 *  tsd_pio_in.c
 *
 * Created on: 2020-12-02 10:46
 *     Author: jalden
 *
 */

#include "tsd_register_common.h"
#include "tsd_pio_in.h"

///////////////////////////////////////////////////////////////////////////
//// Function Definitions
///////////////////////////////////////////////////////////////////////////

int get_tsd_pio_in_data( uint32_t base, uint32_t *data )
{
    return get_register_region(base,
        TSD_PIO_IN_DATA_REG,
        TSD_PIO_IN_DATA_DATA_MASK,
        TSD_PIO_IN_DATA_DATA_SHFT,
        data );
}


