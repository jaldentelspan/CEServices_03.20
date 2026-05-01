/*
 * tsd_hdlc_decoder_2p0_regs.h
 *
 *  Created on: Mar 31, 2020
 *      Author: eric
 */

#ifndef TSD_HDLC_ENCODER_2P0_REGS_H_
#define TSD_HDLC_ENCODER_2P0_REGS_H_

#include <io.h>

/**************************************
 * HDLC ENCODER DEFINES
 **************************************/
#define TSD_HDLC_ENC_CTRL_ENABLE_BIT            (0x00000001)
#define TSD_HDLC_ENC_CTRL_REMOVE_FCS_BIT        (0x00000004)
#define TSD_HDLC_ENC_CTRL_ADDR_CTRL_EN_BIT      (0x00000008)
#define TSD_HDLC_ENC_CTRL_ADDR_VAL_MASK         (0x00000FF0)
#define TSD_HDLC_ENC_CTRL_ADDR_VAL_SHFT         (4)
#define TSD_HDLC_ENC_CTRL_CTRL_VAL_MASK         (0x000FF000)
#define TSD_HDLC_ENC_CTRL_CTRL_VAL_SHFT         (12)
#define TSD_HDLC_ENC_CTRL_ETH_HDR_REMOVAL_BIT   (0x00100000)
#define TSD_HDLC_ENC_CTRL_ETH_CSDW_REMOVAL_BIT  (0x00200000)
#define TSD_HDLC_ENC_CTRL_ETH_SECHDR_REM_BIT    (0x00400000)
#define TSD_HDLC_ENC_CTRL_ETH_STRHDR_REM_BIT    (0x00800000)
#define TSD_HDLC_ENC_CTRL_ETH_INVERT_CLOCK      (0x04000000)
#define TSD_HDLC_ENC_CTRL_REMOVE_TSE_PADDING    (0x01000000)
#define TSD_HDLC_ENC_CTRL_RESET_BIT             (0x80000000)

#define TSD_HDLC_ENC_STATUS_AVMM_TIMEOUT        (0x80000000)

#define TSD_HDLC_ENC_COUNTER_MASK               (0x7FFFFFFF)
#define TSD_HDLC_ENC_COUNTER_RESET              (0x80000000)


/**************************************
 * HDLC ENCODER REGISTERS
 **************************************/
#define TSD_HDLC_ENC_VERSION_REG               (0x00)
#define TSD_HDLC_ENC_IP_ID_REG                 (0x01)
#define TSD_HDLC_ENC_STATUS_REG                (0x02)
#define TSD_HDLC_ENC_CTRL_REG                  (0x03)
#define TSD_HDLC_ENC_BITRATE_REG               (0x05)
#define TSD_HDLC_ENC_PACKET_CNT_REG            (0x06)
#define TSD_HDLC_ENC_FIFO0_OFLOW_CNT_REG       (0x08)
#define TSD_HDLC_ENC_FIFO0_UFLOW_CNT_REG       (0x09)

/**************************************
 * HDLC ENCODER REGISTER ACCESS
 **************************************/
#define TSD_HDLC_ENC_IORD_VERSION(base) \
    IORD(base, TSD_HDLC_ENC_VERSION_REG)

#define TSD_HDLC_ENC_IORD_IP_ID(base) \
    IORD(base, TSD_HDLC_ENC_IP_ID_REG)

#define TSD_HDLC_ENC_IORD_STATUS(base) \
    IORD(base, TSD_HDLC_ENC_STATUS_REG)
#define TSD_HDLC_ENC_IOWR_STATUS(base, val) \
    IOWR(base, TSD_HDLC_ENC_STATUS_REG, val)

#define TSD_HDLC_ENC_IORD_CONTROL(base) \
    IORD(base, TSD_HDLC_ENC_CTRL_REG)
#define TSD_HDLC_ENC_IOWR_CONTROL(base, val) \
    IOWR(base, TSD_HDLC_ENC_CTRL_REG, val)

#define TSD_HDLC_ENC_IORD_BITRATE(base) \
    IORD(base, TSD_HDLC_ENC_BITRATE_REG)
#define TSD_HDLC_ENC_IOWR_BITRATE(base, val) \
    IOWR(base, TSD_HDLC_ENC_BITRATE_REG, val)

#define TSD_HDLC_ENC_IORD_PACKET_CNT(base) \
    IORD(base, TSD_HDLC_ENC_PACKET_CNT_REG)
#define TSD_HDLC_ENC_RESET_PACKET_CNT(base) \
    IOWR(base, TSD_HDLC_ENC_PACKET_CNT_REG, TSD_HDLC_ENC_COUNTER_RESET); \
    IOWR(base, TSD_HDLC_ENC_PACKET_CNT_REG, 0)

#define TSD_HDLC_ENC_IORD_FIFO0_OFLOW_CNT(base) \
    IORD(base, TSD_HDLC_ENC_FIFO0_OFLOW_CNT_REG)
#define TSD_HDLC_ENC_RESET_FIFO0_OFLOW_CNT(base) \
    IOWR(base, TSD_HDLC_ENC_FIFO0_OFLOW_CNT_REG, TSD_HDLC_ENC_COUNTER_RESET); \
    IOWR(base, TSD_HDLC_ENC_FIFO0_OFLOW_CNT_REG, 0)

#define TSD_HDLC_ENC_IORD_FIFO0_UFLOW_CNT(base) \
    IORD(base, TSD_HDLC_ENC_FIFO0_UFLOW_CNT_REG)
#define TSD_HDLC_ENC_RESET_FIFO0_UFLOW_CNT(base) \
    IOWR(base, TSD_HDLC_ENC_FIFO0_UFLOW_CNT_REG, TSD_HDLC_ENC_COUNTER_RESET); \
    IOWR(base, TSD_HDLC_ENC_FIFO0_UFLOW_CNT_REG, 0)


#endif /* TSD_HDLC_ENCODER_2P0_REGS_H_ */
