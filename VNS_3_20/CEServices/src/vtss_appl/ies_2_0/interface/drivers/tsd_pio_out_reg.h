/*
 *
 *  tsd_pio_out_reg.h
 *
 * Created on: 2020-12-02 15:11
 *     Author: jalden
 *
 */


#ifndef __TSD_PIO_OUT_REG_H__
#define __TSD_PIO_OUT_REG_H__
#include <io.h>



///////////////////////////////////////////////////////////////////////////
//// Regesters
///////////////////////////////////////////////////////////////////////////

#define TSD_PIO_OUT_DATA_REG                                        (0x00)

///////////////////////////////////////////////////////////////////////////
//// Regester Bits\ Masks
///////////////////////////////////////////////////////////////////////////

#define TSD_PIO_OUT_DATA_DATA_MASK                            (0x00007FFF)
#define TSD_PIO_OUT_DATA_DATA_SHFT                                     (0)


///////////////////////////////////////////////////////////////////////////
//// Regester Access
///////////////////////////////////////////////////////////////////////////

#define TSD_PIO_OUT_IORD_DATA(base)                        \
                      IORD(base, TSD_PIO_OUT_DATA_REG)                       
#define TSD_PIO_OUT_IOWR_DATA(base, val)                   \
                      IOWR(base, TSD_PIO_OUT_DATA_REG, val)                  



#endif /* __TSD_PIO_OUT_REG_H__ */
