/*
 * tsd_hdlc_ch7_encoder.c
 *
 *  Created on: Jan 6, 2021
 *      Author: eric
 */

#include "stdlib.h"
#include "tsd_pll_reconfig.h"
#include "tsd_hdlc_ch7_encoder.h"
#include "tsd_ch7_encoder_2p0.h"
#include "tsd_hdlc_encoder_2p0.h"


/*
 * Sets up the HDLC CH7 encoder based on the information in the input structure
 * Note: the input struct MUST point to a valid object with valid base addresses (encoderBaseAddr and pllReconfigBaseAddr).
 * returns TSD_HDLC_CH7_ENCODER_SUCCESS on success
 */
int setupHdlcCh7Encoder(HdlcCh7EncoderInfo *EncoderInfo)
{
	int ret = 0;
	if(NULL == EncoderInfo) return TSD_HDLC_CH7_ENCODER_NULLPTR;

	// base addresses always start on a 32-bit boundary.
	if(EncoderInfo->ch7EncoderBaseAddr & 0x1F){
		return TSD_HDLC_CH7_ENCODER_INVALID_BASE;
	}
	if(EncoderInfo->hdlcEncoderBaseAddr & 0x1F) {
		return TSD_HDLC_CH7_ENCODER_INVALID_BASE;
	}

	// check the bitrate
	if((EncoderInfo->bitrate < 100000) || (EncoderInfo->bitrate > 50000000)){
		return TSD_HDLC_CH7_ENCODER_INVALID_RATE;
	}

	// disable the cores
	tsd_ch7_enc_disable(EncoderInfo->ch7EncoderBaseAddr);
	tsd_hdlc_enc_disable(EncoderInfo->hdlcEncoderBaseAddr);

	switch(EncoderInfo->encoderMode){
	case TSD_HDLC_CH7_ENCMODE_HDLC:
		// clear all control bits...
        TSD_HDLC_ENC_IOWR_CONTROL(EncoderInfo->hdlcEncoderBaseAddr, 0);
        ret += tsd_hdlc_enc_set_control_bits(EncoderInfo->hdlcEncoderBaseAddr,
        		TSD_HDLC_ENC_CTRL_REMOVE_FCS_BIT |
				(EncoderInfo->invertClock ? TSD_HDLC_ENC_CTRL_ETH_INVERT_CLOCK : 0));
        if(EncoderInfo->hdlcAddrCtrlEnable) {
        	ret += tsd_hdlc_enc_set_control_bits(EncoderInfo->hdlcEncoderBaseAddr, TSD_HDLC_ENC_CTRL_ADDR_CTRL_EN_BIT);
        	ret += tsd_hdlc_enc_set_ctrl_addr_values(EncoderInfo->hdlcEncoderBaseAddr, EncoderInfo->hdlcCtrlByte, EncoderInfo->hdlcAddrByte);
        }
        ret += tsd_hdlc_enc_enable(EncoderInfo->hdlcEncoderBaseAddr);
        ret += tsd_hdlc_enc_set_bitrate(EncoderInfo->hdlcEncoderBaseAddr, EncoderInfo->bitrate, EncoderInfo->sysClkFrequency);
		break;
	case TSD_HDLC_CH7_ENCMODE_CH7:
        ret += tsd_ch7_enc_set_sync_pattern(EncoderInfo->ch7EncoderBaseAddr, 0xFE6B2840, 32);
        ret += tsd_ch7_enc_remove_fcs(EncoderInfo->ch7EncoderBaseAddr, !EncoderInfo->fcsInsertion);
        ret += tsd_ch7_enc_set_frame_size(EncoderInfo->ch7EncoderBaseAddr, EncoderInfo->ch7FrameSize);
        ret += tsd_ch7_enc_set_port_select(EncoderInfo->ch7EncoderBaseAddr, 0);
        ret += tsd_ch7_enc_enable(EncoderInfo->ch7EncoderBaseAddr);
        ret += tsd_ch7_enc_set_bitrate(EncoderInfo->ch7EncoderBaseAddr, EncoderInfo->bitrate, EncoderInfo->sysClkFrequency);
        ret += tsd_ch7_enc_invert_clock(EncoderInfo->ch7EncoderBaseAddr, EncoderInfo->invertClock);
		break;
	default:
		// disable
		break;
	}

	return ret ? TSD_HDLC_CH7_ENCODER_SETUP_FAIL : TSD_HDLC_CH7_ENCODER_SUCCESS;
}


/*
 * Gets the HDLC CH7 settngs from the Encoder core
 * Note: the Base addresses and System clock rate MUST be valid.
 *       AND if setupHdlcCh7Encoder has not been called using the same EncoderInfo object,
 *           the bitrate value may be incorrect upon return.
 * returns TSD_HDLC_CH7_ENCODER_SUCCESS on success
 */
int getHdlcCh7EncoderSetup(HdlcCh7EncoderInfo *EncoderInfo)
{
	uint32_t ch7CtrlWord;
	uint32_t hdlcCtrlWord;

	if(NULL == EncoderInfo) return TSD_HDLC_CH7_ENCODER_NULLPTR;

	// base addresses always start on a 32-bit boundary.
	if(EncoderInfo->ch7EncoderBaseAddr & 0x1F){
		return TSD_HDLC_CH7_ENCODER_INVALID_BASE;
	}
	if(EncoderInfo->hdlcEncoderBaseAddr & 0x1F){
		return TSD_HDLC_CH7_ENCODER_INVALID_BASE;
	}
	if(EncoderInfo->sysClkFrequency == 0) {
		return TSD_HDLC_CH7_ENCODER_BR_SETUP_FAIL;
	}

	// see who is enabled....
	ch7CtrlWord = TSD_CH7_ENC_IORD_CONTROL(EncoderInfo->ch7EncoderBaseAddr);
	hdlcCtrlWord = TSD_HDLC_ENC_IORD_CONTROL(EncoderInfo->hdlcEncoderBaseAddr);
	if(ch7CtrlWord & TSD_CH7_ENC_CTRL_ENABLE_BIT) {
		EncoderInfo->encoderMode = TSD_HDLC_CH7_ENCMODE_CH7;
		// get bitrate
		tsd_ch7_enc_get_bitrate(EncoderInfo->ch7EncoderBaseAddr, &EncoderInfo->bitrate, EncoderInfo->sysClkFrequency);
		EncoderInfo->hdlcAddrCtrlEnable = 0;
		EncoderInfo->hdlcCtrlByte = 0;
		EncoderInfo->hdlcAddrByte = 0;
		tsd_ch7_enc_get_frame_size(EncoderInfo->ch7EncoderBaseAddr, &EncoderInfo->ch7FrameSize);
	} else if(hdlcCtrlWord & TSD_HDLC_ENC_CTRL_ENABLE_BIT) {
		EncoderInfo->encoderMode = TSD_HDLC_CH7_ENCMODE_HDLC;
		// get bitrate
		tsd_hdlc_enc_get_bitrate(EncoderInfo->hdlcEncoderBaseAddr, &EncoderInfo->bitrate, EncoderInfo->sysClkFrequency);
		if(hdlcCtrlWord & TSD_HDLC_ENC_CTRL_REMOVE_FCS_BIT) {
			EncoderInfo->hdlcAddrCtrlEnable = 1;
			tsd_hdlc_enc_get_ctrl_addr_values(EncoderInfo->hdlcEncoderBaseAddr, &EncoderInfo->hdlcCtrlByte, &EncoderInfo->hdlcAddrByte);
		} else {
			EncoderInfo->hdlcAddrCtrlEnable = 0;
			EncoderInfo->hdlcCtrlByte = 0;
			EncoderInfo->hdlcAddrByte = 0;
		}
	} else {
		EncoderInfo->encoderMode = TSD_HDLC_CH7_ENCMODE_DISABLED;
		EncoderInfo->bitrate = 0;
	}
	return TSD_HDLC_CH7_ENCODER_SUCCESS;
}

/*
 * Gets the fifo underflow count from the HDLC/CH7 Encoder Core
 * returns the count from the encoder.
 */
int getHdlcCh7EncoderUnderflowCount(HdlcCh7EncoderInfo *EncoderInfo)
{
	uint32_t cnt;
	switch(EncoderInfo->encoderMode){
	case TSD_HDLC_CH7_ENCMODE_HDLC:
		if(tsd_hdlc_enc_get_fifo0_underflow_count(EncoderInfo->hdlcEncoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	case TSD_HDLC_CH7_ENCMODE_CH7:
		if(tsd_ch7_enc_get_rx_eth_underflow_count(EncoderInfo->ch7EncoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	default:
		cnt = 0;
		break;
	}
	return (int)(cnt & 0x7FFFFFFF);
}

/*
 * Gets the fifo overflow count from the HDLC/CH7 Encoder Core
 * returns the count from the encoder
 */
int getHdlcCh7EncoderOverflowCount(HdlcCh7EncoderInfo *EncoderInfo)
{
	uint32_t cnt;
	switch(EncoderInfo->encoderMode){
	case TSD_HDLC_CH7_ENCMODE_HDLC:
		if(tsd_hdlc_enc_get_fifo0_overflow_count(EncoderInfo->hdlcEncoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	case TSD_HDLC_CH7_ENCMODE_CH7:
		if(tsd_ch7_enc_get_rx_eth_overflow_count(EncoderInfo->ch7EncoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	default:
		cnt = 0;
		break;
	}
	return (int)(cnt & 0x7FFFFFFF);
}

/*
 * Gets the total ethernet frame count from the HDLC/CH7 Encoder Core
 * 16-bit counter... it will roll over at 65535
 * returns the count from the encoder
 */
int getHdlcCh7EncoderEthFrameCount(HdlcCh7EncoderInfo *EncoderInfo)
{
	uint32_t cnt;
	switch(EncoderInfo->encoderMode){
	case TSD_HDLC_CH7_ENCMODE_HDLC:
		if(tsd_hdlc_enc_get_rx_eth_frame_count(EncoderInfo->hdlcEncoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	case TSD_HDLC_CH7_ENCMODE_CH7:
		if(tsd_ch7_enc_get_rx_eth_frame_count(EncoderInfo->ch7EncoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	default:
		cnt = 0;
		break;
	}
	return (int)(cnt & 0x7FFFFFFF);
}

/*
 * Gets the total raw CH10 frame count from the HDLC/CH7 Encoder Core
 * -> ONLY VALID IN CH7 MODE
 * 16-bit counter... it will roll over at 65535
 * returns the count from the encoder
 */
int getHdlcCh7EncoderCH10FrameCount(HdlcCh7EncoderInfo *EncoderInfo)
{
	uint32_t cnt;
	switch(EncoderInfo->encoderMode){
	case TSD_HDLC_CH7_ENCMODE_HDLC:
		cnt = 0;
		break;
	case TSD_HDLC_CH7_ENCMODE_CH7:
		if(tsd_ch7_enc_get_rx_ch10_packet_count(EncoderInfo->ch7EncoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	default:
		cnt = 0;
		break;
	}
	return (int)(cnt & 0x7FFFFFFF);
}

/*
 * Resets all counters in the HDLC/CH7 Encoder Core
 */
int resetHdlcCh7EncoderCounters(HdlcCh7EncoderInfo *EncoderInfo)
{
	tsd_ch7_enc_reset_counters(EncoderInfo->ch7EncoderBaseAddr);
	tsd_hdlc_enc_reset_counters(EncoderInfo->hdlcEncoderBaseAddr);
	return TSD_HDLC_CH7_ENCODER_SUCCESS;
}
