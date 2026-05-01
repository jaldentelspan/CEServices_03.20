/*
 *
 *  tsd_pio_in.h
 *
 * Created on: 2020-12-02 10:46
 *     Author: jalden
 *
 */


#ifndef __TSD_PIO_IN_H__
#define __TSD_PIO_IN_H__

#include <io.h>
#include "tsd_pio_in_reg.h"


#define TSD_PIO_IN_DRIVER_VERSION (0.01)



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



///////////////////////////////////////////////////////////////////////////
//// Function Prototypes
///////////////////////////////////////////////////////////////////////////

int get_tsd_pio_in_data( uint32_t base, uint32_t *data );




#ifdef __cplusplus
}
#endif /* __cplusplus */




#endif /* __TSD_PIO_IN_H__ */
