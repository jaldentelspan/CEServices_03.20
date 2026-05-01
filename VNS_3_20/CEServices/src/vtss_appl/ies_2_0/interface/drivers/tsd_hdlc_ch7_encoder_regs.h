/*
 * tsd_hdlc_ch7_encoder_regs.h
 *
 *  Created on: Jan 4, 2021
 *      Author: eric
 */

#ifndef TSD_HDLC_CH7_ENCODER_REGS_H_
#define TSD_HDLC_CH7_ENCODER_REGS_H_
#include "io.h"

/*********************************************************
 * HDLC CH7 ENCODER REGISTERS
 *********************************************************/
#define TSD_HDLC_CH7_ENCODER_VERSION_REG                        (0x00)
#define TSD_HDLC_CH7_ENCODER_CONTROL_REG                        (0x01)
#define TSD_HDLC_CH7_ENCODER_STATUS_REG                         (0x02)
#define TSD_HDLC_CH7_ENCODER_FRAME_COUNT_REG                    (0x03)

/*********************************************************
 * CONTROL REGISTER DEFINITIONS
 *********************************************************/
#define TSD_HDLC_CH7_ENCODER_CONTROL_CH7_FRAMESIZE_MASK         (0x0000000F)
#define TSD_HDLC_CH7_ENCODER_CONTROL_CH7_FRAMESIZE_SHFT         (0)
#define TSD_HDLC_CH7_ENCODER_CONTROL_HDLC_ENABLE                (0x00000010)
#define TSD_HDLC_CH7_ENCODER_CONTROL_HDLC_SEND_ADDR_CTRL        (0x00000020)
#define TSD_HDLC_CH7_ENCODER_CONTROL_CLEAR_COUNTERS             (0x00000040) /* MUST BE SET TO 1 AND BACK TO 0 */
#define TSD_HDLC_CH7_ENCODER_CONTROL_ENABLE      	        (0x00000080)
#define TSD_HDLC_CH7_ENCODER_CONTROL_HDLC_ADDR_BYTE_MASK        (0x0000FF00)
#define TSD_HDLC_CH7_ENCODER_CONTROL_HDLC_ADDR_BYTE_SHFT        (8)
#define TSD_HDLC_CH7_ENCODER_CONTROL_HDLC_CTRL_BYTE_MASK        (0x00FF0000)
#define TSD_HDLC_CH7_ENCODER_CONTROL_HDLC_CTRL_BYTE_SHFT        (16)
#define TSD_HDLC_CH7_ENCODER_CONTROL_FCS_INSERTION              (0x01000000)
#define TSD_HDLC_CH7_ENCODER_CONTROL_USE_OLD_STREAMING_HEADER   (0x04000000)

/*********************************************************
 * STATUS REGISTER DEFINITIONS
 *********************************************************/
#define TSD_HDLC_CH7_ENCODER_STATUS_OVERFLOW_CNT_MASK           (0x0000FFFF)
#define TSD_HDLC_CH7_ENCODER_STATUS_OVERFLOW_CNT_SHFT           (0)
#define TSD_HDLC_CH7_ENCODER_STATUS_UNDERFLOW_CNT_MASK          (0xFFFF0000)
#define TSD_HDLC_CH7_ENCODER_STATUS_UNDERFLOW_CNT_SHFT          (16)

/*********************************************************
 * FRAME COUNT REGISTER DEFINITIONS
 *********************************************************/
#define TSD_HDLC_CH7_ENCODER_FRAME_COUNT_CH10_MASK             (0x0000FFFF)
#define TSD_HDLC_CH7_ENCODER_FRAME_COUNT_CH10_SHFT             (0)
#define TSD_HDLC_CH7_ENCODER_FRAME_COUNT_TSE_MASK              (0xFFFF0000)
#define TSD_HDLC_CH7_ENCODER_FRAME_COUNT_TSE_SHFT              (16)

/*********************************************************
 * IORD/IOWR CALLS
 *********************************************************/
#define TSD_HDLC_CH7_ENCODER_IORD_VERSION(base)         IORD(base, TSD_HDLC_CH7_ENCODER_VERSION_REG)

#define TSD_HDLC_CH7_ENCODER_IORD_CONTROL(base)         IORD(base, TSD_HDLC_CH7_ENCODER_CONTROL_REG)
#define TSD_HDLC_CH7_ENCODER_IOWR_CONTROL(base, val)    IOWR(base, TSD_HDLC_CH7_ENCODER_CONTROL_REG, val)

#define TSD_HDLC_CH7_ENCODER_IORD_STATUS(base)          IORD(base, TSD_HDLC_CH7_ENCODER_STATUS_REG)

#define TSD_HDLC_CH7_ENCODER_IORD_FRAME_COUNT(base)     IORD(base, TSD_HDLC_CH7_ENCODER_FRAME_COUNT_REG)

#endif /* TSD_HDLC_CH7_ENCODER_REGS_H_ */
