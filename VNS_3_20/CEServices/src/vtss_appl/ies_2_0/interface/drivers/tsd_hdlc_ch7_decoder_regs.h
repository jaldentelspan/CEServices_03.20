/*
 * tsd_hdlc_ch7_decoder_regs.h
 *
 *  Created on: Jan 4, 2021
 *      Author: eric
 */

#ifndef TSD_HDLC_CH7_DECODER_REGS_H_
#define TSD_HDLC_CH7_DECODER_REGS_H_

#include "io.h"
/*********************************************************
 * HDLC CH7 DECODER REGISTERS
 *********************************************************/
#define TSD_HDLC_CH7_DECODER_VERSION_REG            (0x00)
#define TSD_HDLC_CH7_DECODER_CONTROL_REG			(0x01)
#define TSD_HDLC_CH7_DECODER_ERR_CNT_REG			(0x03)
#define TSD_HDLC_CH7_DECODER_GOLAY_ERR_CNT_REG		(0x04)
#define TSD_HDLC_CH7_DECODER_FRAME_COUNT_REG        (0x05)

/*********************************************************
 * CONTROL REGISTER DEFINITIONS
 *********************************************************/
#define TSD_HDLC_CH7_DECODER_CONTROL_CH7_FRAMESIZE_MASK         (0x0000000F)
#define TSD_HDLC_CH7_DECODER_CONTROL_CH7_FRAMESIZE_SHFT         (0)
#define TSD_HDLC_CH7_DECODER_CONTROL_HDLC_ENABLE                (0x00000010)
#define TSD_HDLC_CH7_DECODER_CONTROL_CH7_ENABLE                 (0x00000020)
#define TSD_HDLC_CH7_DECODER_CONTROL_USE_OLD_STREAMING_HEADER   (0x00000040)
#define TSD_HDLC_CH7_DECODER_CONTROL_CLEAR_COUNTERS				(0x00000080)

/*********************************************************
 * ERROR COUNT REGISTER DEFINITIONS
 *********************************************************/
#define TSD_HDLC_CH7_DECODER_ERR_CNT_HDLC_FAIL_CNT_MASK         (0x0000FFFF)
#define TSD_HDLC_CH7_DECODER_ERR_CNT_HDLC_FAIL_CNT_SHFT         (0)
#define TSD_HDLC_CH7_DECODER_ERR_CNT_CH7_FAIL_CNT_MASK          (0xFFFF0000)
#define TSD_HDLC_CH7_DECODER_ERR_CNT_CH7_FAIL_CNT_SHFT          (16)

/*********************************************************
 * GOLAY ERROR COUNT REGISTER DEFINITIONS
 *********************************************************/
#define TSD_HDLC_CH7_DECODER_GOLAY_ERR_CNT_MASK                 (0x0000FFFF)
#define TSD_HDLC_CH7_DECODER_GOLAY_ERR_CNT_SHFT                 (0)

/*********************************************************
 * FRAME COUNT REGISTER DEFINITIONS
 *********************************************************/
#define TSD_HDLC_CH7_DECODER_FRAME_COUNT_CH10_MASK             (0x0000FFFF)
#define TSD_HDLC_CH7_DECODER_FRAME_COUNT_CH10_SHFT             (0)
#define TSD_HDLC_CH7_DECODER_FRAME_COUNT_TSE_MASK              (0xFFFF0000)
#define TSD_HDLC_CH7_DECODER_FRAME_COUNT_TSE_SHFT              (16)

/*********************************************************
 * IORD/IOWR CALLS
 *********************************************************/
#define TSD_HDLC_CH7_DECODER_IORD_VERSION(base)            IORD(base, TSD_HDLC_CH7_DECODER_VERSION_REG)

#define TSD_HDLC_CH7_DECODER_IORD_CONTROL(base)            IORD(base, TSD_HDLC_CH7_DECODER_CONTROL_REG)
#define TSD_HDLC_CH7_DECODER_IOWR_CONTROL(base, val)       IOWR(base, TSD_HDLC_CH7_DECODER_CONTROL_REG, val)

#define TSD_HDLC_CH7_DECODER_IORD_ERR_CNT(base)            IORD(base, TSD_HDLC_CH7_DECODER_ERR_CNT_REG)

#define TSD_HDLC_CH7_DECODER_IORD_GOLAY_ERR_CNT(base)      IORD(base, TSD_HDLC_CH7_DECODER_GOLAY_ERR_CNT_REG)

#define TSD_HDLC_CH7_DECODER_IORD_FRAME_COUNT(base)        IORD(base, TSD_HDLC_CH7_DECODER_FRAME_COUNT_REG)


#endif /* TSD_HDLC_CH7_DECODER_REGS_H_ */
