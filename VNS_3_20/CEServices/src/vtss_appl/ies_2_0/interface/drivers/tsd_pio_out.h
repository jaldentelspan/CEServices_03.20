/*
 *
 *  tsd_pio_out.h
 *
 * Created on: 2020-12-02 15:11
 *     Author: jalden
 *
 */


#ifndef __TSD_PIO_OUT_H__
#define __TSD_PIO_OUT_H__

#include <io.h>
#include "tsd_pio_out_reg.h"


#define TSD_PIO_OUT_DRIVER_VERSION (0.01)



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



///////////////////////////////////////////////////////////////////////////
//// Function Prototypes
///////////////////////////////////////////////////////////////////////////

int get_tsd_pio_out_data( uint32_t base, uint32_t *data );
int set_tsd_pio_out_data( uint32_t base, uint32_t data );




#ifdef __cplusplus
}
#endif /* __cplusplus */




#endif /* __TSD_PIO_OUT_H__ */
