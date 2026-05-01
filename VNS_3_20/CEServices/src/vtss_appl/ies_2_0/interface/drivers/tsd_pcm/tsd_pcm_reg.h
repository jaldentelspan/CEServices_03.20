/*
 *
 *  tsd_pcm_reg.h
 *
 * Created on: 2020-12-08 13:16
 *     Author: jalden
 *
 */


#ifndef __TSD_PCM_REG_H__
#define __TSD_PCM_REG_H__
#include <io.h>



///////////////////////////////////////////////////////////////////////////
//// Regesters
///////////////////////////////////////////////////////////////////////////

#define TSD_PCM_CTRL_REG                                            (0x00)
#define TSD_PCM_PLL_CTRL_REG                                        (0x01)
#define TSD_PCM_PLL_WRITE_DATA_REG                                  (0x02)
#define TSD_PCM_PLL_READ_DATA_REG                                   (0x03)

///////////////////////////////////////////////////////////////////////////
//// Regester Bits\ Masks
///////////////////////////////////////////////////////////////////////////

#define TSD_PCM_CTRL_ENCODE_CH7_FRAME_SIZE_MASK               (0x0000000F)
#define TSD_PCM_CTRL_ENCODE_CH7_FRAME_SIZE_SHFT                        (0)
#define TSD_PCM_CTRL_ENCODE_HDLC_ENABLE_MASK                  (0x00000010)
#define TSD_PCM_CTRL_ENCODE_HDLC_ENABLE_SHFT                           (4)
#define TSD_PCM_CTRL_HDLC_SEND_ADDR_MASK                      (0x00000020)
#define TSD_PCM_CTRL_HDLC_SEND_ADDR_SHFT                               (5)
#define TSD_PCM_CTRL_HDLC_CH7_MUX_MASK                        (0x00000040)
#define TSD_PCM_CTRL_HDLC_CH7_MUX_SHFT                                 (6)
#define TSD_PCM_CTRL_ENCODE_CH7_ENABLE_MASK                   (0x00000080)
#define TSD_PCM_CTRL_ENCODE_CH7_ENABLE_SHFT                            (7)
#define TSD_PCM_CTRL_HDLC_ADDR_MASK                           (0x0000FF00)
#define TSD_PCM_CTRL_HDLC_ADDR_SHFT                                    (8)
#define TSD_PCM_CTRL_HDLC_CTRL_MASK                           (0x00FF0000)
#define TSD_PCM_CTRL_HDLC_CTRL_SHFT                                   (16)
#define TSD_PCM_CTRL_DECODE_CH7_FRAME_SIZE_MASK               (0x0F000000)
#define TSD_PCM_CTRL_DECODE_CH7_FRAME_SIZE_SHFT                       (24)
#define TSD_PCM_CTRL_DECODE_HDLC_ENABLE_MASK                  (0x10000010)
#define TSD_PCM_CTRL_DECODE_HDLC_ENABLE_SHFT                           (4)
#define TSD_PCM_CTRL_DECODE_CH7_ENABLE_MASK                   (0x20000080)
#define TSD_PCM_CTRL_DECODE_CH7_ENABLE_SHFT                            (7)

#define TSD_PCM_PLL_CTRL_ADDR_MASK                            (0x0000003F)
#define TSD_PCM_PLL_CTRL_ADDR_SHFT                                     (0)
#define TSD_PCM_PLL_CTRL_READ_BIT_MASK                        (0x00000080)
#define TSD_PCM_PLL_CTRL_READ_BIT_SHFT                                 (7)
#define TSD_PCM_PLL_CTRL_WRITE_BIT_MASK                       (0x00000100)
#define TSD_PCM_PLL_CTRL_WRITE_BIT_SHFT                                (8)

#define TSD_PCM_PLL_WRITE_DATA_REG_MASK                       (0xFFFFFFFF)
#define TSD_PCM_PLL_WRITE_DATA_REG_SHFT                                (0)

#define TSD_PCM_PLL_READ_DATA_REG_MASK                        (0xFFFFFFFF)
#define TSD_PCM_PLL_READ_DATA_REG_SHFT                                 (0)


///////////////////////////////////////////////////////////////////////////
//// Regester Access
///////////////////////////////////////////////////////////////////////////

#define TSD_PCM_IORD_CTRL(base)                            \
                      IORD(base, TSD_PCM_CTRL_REG)                           
#define TSD_PCM_IOWR_CTRL(base, val)                       \
                      IOWR(base, TSD_PCM_CTRL_REG, val)                      


#define TSD_PCM_IORD_PLL_CTRL(base)                        \
                      IORD(base, TSD_PCM_PLL_CTRL_REG)                       
#define TSD_PCM_IOWR_PLL_CTRL(base, val)                   \
                      IOWR(base, TSD_PCM_PLL_CTRL_REG, val)                  


#define TSD_PCM_IORD_PLL_WRITE_DATA(base)                  \
                      IORD(base, TSD_PCM_PLL_WRITE_DATA_REG)                 
#define TSD_PCM_IOWR_PLL_WRITE_DATA(base, val)             \
                      IOWR(base, TSD_PCM_PLL_WRITE_DATA_REG, val)            


#define TSD_PCM_IORD_PLL_READ_DATA(base)                   \
                      IORD(base, TSD_PCM_PLL_READ_DATA_REG)                  



#endif /* __TSD_PCM_REG_H__ */
