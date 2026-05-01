/*
 *
 *  tsd_pio_out.c
 *
 * Created on: 2020-12-02 15:11
 *     Author: jalden
 *
 */

#include "tsd_register_common.h"
#include "tsd_pio_out.h"

///////////////////////////////////////////////////////////////////////////
//// Function Definitions
///////////////////////////////////////////////////////////////////////////

int get_tsd_pio_out_data( uint32_t base, uint32_t *data )
{
    return get_register_region(base,
        TSD_PIO_OUT_DATA_REG,
        TSD_PIO_OUT_DATA_DATA_MASK,
        TSD_PIO_OUT_DATA_DATA_SHFT,
        data );
}

int set_tsd_pio_out_data( uint32_t base, uint32_t data )
{
    return set_register_region(base,
        TSD_PIO_OUT_DATA_REG,
        TSD_PIO_OUT_DATA_DATA_MASK,
        TSD_PIO_OUT_DATA_DATA_SHFT,
        data );
}


