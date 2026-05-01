/*
 * tsd_ch7_decoder_2p0_regs.h
 *
 *  Created on: Mar 31, 2020
 *      Author: eric
 */

#ifndef TSD_CH7_ENCODER_2P0_REGS_H_
#define TSD_CH7_ENCODER_2P0_REGS_H_

#include <io.h>

/**************************************
 * CH7 ENCODER DEFINES
 **************************************/
#define TSD_CH7_ENC_CTRL_FRAME_SIZE_MASK       (0x0000000F) /* 223*THIS VALUE */
#define TSD_CH7_ENC_CTRL_FRAME_SIZE_SHFT       (0)
#define TSD_CH7_ENC_CTRL_ENABLE_BIT            (0x00000080)
#define TSD_CH7_ENC_CTRL_REMOVE_TSE_PADDING    (0x00000100)
#define TSD_CH7_ENC_CTRL_INVERT_CLK            (0x00000200)
#define TSD_CH7_ENC_CTRL_REMOVE_FCS            (0x01000000)

#define TSD_CH7_ENC_STATUS_AVMM_TIMEOUT        (0x80000000)

#define TSD_CH7_ENC_COUNTER_MASK               (0x7FFFFFFF)
#define TSD_CH7_ENC_COUNTER_RESET_MASK         (0x80000000)

#define TSD_CH7_ENC_PORT_SELECT_MASK            (0x000000FF)
#define TSD_CH7_ENC_PORT_SELECT_SHFT            (0)

/**************************************
 * CH7 ENCODER REGISTERS
 **************************************/
#define TSD_CH7_ENC_VERSION_REG               (0x0)
#define TSD_CH7_ENC_IP_ID_REG                 (0x1)
#define TSD_CH7_ENC_STATUS_REG                (0x2)
#define TSD_CH7_ENC_CTRL_REG                  (0x3)
#define TSD_CH7_ENC_PORT_SELECT_REG           (0x4)
#define TSD_CH7_ENC_PORT_BIT_RATE_REG         (0x5)
#define TSD_CH7_ENC_SYNC_PATTERN_REG          (0x6)
#define TSD_CH7_ENC_SYNC_MASK_REG             (0x7)
#define TSD_CH7_ENC_SYNC_LENGTH_REG           (0x8)
#define TSD_CH7_ENC_ETH_RX_CNT_REG            (0x9)
#define TSD_CH7_ENC_CH10_RX_CNT_REG           (0xA)
#define TSD_CH7_ENC_ETH_OVERFLOW_CNT_REG      (0xB)
#define TSD_CH7_ENC_ETH_UNDERFLOW_CNT_REG     (0xC)
#define TSD_CH7_ENC_CH10_OVERFLOW_CNT_REG     (0xD)
#define TSD_CH7_ENC_CH10_UNDERFLOW_CNT_REG    (0xE)
#define TSD_CH7_ENC_CH10_PKT_ERR_CNT_REG      (0xF)

/**************************************
 * CH7 ENCODER REGISTER ACCESS
 **************************************/
#define TSD_CH7_ENC_IORD_VERSION(base) \
    IORD(base, TSD_CH7_ENC_VERSION_REG)

#define TSD_CH7_ENC_IORD_IP_ID(base) \
    IORD(base, TSD_CH7_ENC_IP_ID_REG)

#define TSD_CH7_ENC_IORD_STATUS(base)    \
    IORD(base, TSD_CH7_ENC_STATUS_REG)
#define TSD_CH7_ENC_IOWR_STATUS(base, val)\
    IOWR(base, TSD_CH7_ENC_STATUS_REG, val)

#define TSD_CH7_ENC_IORD_CONTROL(base)    \
    IORD(base, TSD_CH7_ENC_CTRL_REG)
#define TSD_CH7_ENC_IOWR_CONTROL(base, val)\
    IOWR(base, TSD_CH7_ENC_CTRL_REG, val)

#define TSD_CH7_ENC_IORD_PORT_SELECT(base)    \
    IORD(base, TSD_CH7_ENC_PORT_SELECT_REG)
#define TSD_CH7_ENC_IOWR_PORT_SELECT(base, val)\
    IOWR(base, TSD_CH7_ENC_PORT_SELECT_REG, val)

#define TSD_CH7_ENC_IORD_BITRATE(base) \
    IORD(base, TSD_CH7_ENC_PORT_BIT_RATE_REG)
#define TSD_CH7_ENC_IOWR_BITRATE(base, val) \
    IOWR(base, TSD_CH7_ENC_PORT_BIT_RATE_REG, val)

#define TSD_CH7_ENC_IORD_SYNC_PATTERN(base) \
    IORD(base, TSD_CH7_ENC_SYNC_PATTERN_REG)
#define TSD_CH7_ENC_IOWR_SYNC_PATTERN(base, val) \
    IOWR(base, TSD_CH7_ENC_SYNC_PATTERN_REG, val)

#define TSD_CH7_ENC_IORD_SYNC_MASK(base) \
    IORD(base, TSD_CH7_ENC_SYNC_MASK_REG)
#define TSD_CH7_ENC_IOWR_SYNC_MASK(base, val) \
    IOWR(base, TSD_CH7_ENC_SYNC_MASK_REG, val)

#define TSD_CH7_ENC_IORD_SYNC_LENGTH(base) \
    IORD(base, TSD_CH7_ENC_SYNC_LENGTH_REG)
#define TSD_CH7_ENC_IOWR_SYNC_LENGTH(base, val) \
    IOWR(base, TSD_CH7_ENC_SYNC_LENGTH_REG, val)

#define TSD_CH7_ENC_IORD_ETH_RX_CNT(base) \
    IORD(base, TSD_CH7_ENC_ETH_RX_CNT_REG)

#define TSD_CH7_ENC_IORD_CH10_RX_CNT(base) \
    IORD(base, TSD_CH7_ENC_CH10_RX_CNT_REG)

#define TSD_CH7_ENC_IORD_ETH_OVERFLOW_CNT(base) \
    IORD(base, TSD_CH7_ENC_ETH_OVERFLOW_CNT_REG)

#define TSD_CH7_ENC_IORD_CH10_OVERFLOW_CNT(base) \
    IORD(base, TSD_CH7_ENC_CH10_OVERFLOW_CNT_REG)

#define TSD_CH7_ENC_IORD_ETH_UNDERFLOW_CNT(base) \
    IORD(base, TSD_CH7_ENC_ETH_UNDERFLOW_CNT_REG)

#define TSD_CH7_ENC_IORD_CH10_UNDERFLOW_CNT(base) \
    IORD(base, TSD_CH7_ENC_CH10_UNDERFLOW_CNT_REG)

#define TSD_CH7_ENC_IORD_PKT_ERR_CNT(base) \
    IORD(base, TSD_CH7_ENC_CH10_PKT_ERR_CNT_REG)

#endif /* TSD_CH7_ENCODER_2P0_REGS_H_ */
