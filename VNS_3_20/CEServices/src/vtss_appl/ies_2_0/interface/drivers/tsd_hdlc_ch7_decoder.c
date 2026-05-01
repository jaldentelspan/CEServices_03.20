/*
 * tsd_hdlc_ch7_decoder.c
 *
 *  Created on: Jan 6, 2021
 *      Author: eric
 */
#include <stdlib.h>
/* #include <cstring> */
/* #include <cstdio> */
#include "tsd_tse_sxe.h"
#include "tsd_hdlc_ch7_decoder.h"
#include "tsd_ch7_decoder_2p0.h"
#include "tsd_hdlc_decoder_2p0.h"

/*
 * Sets up the HDLC CH7 decoder based on the information in the input structure
 * Note: the input struct MUST point to a valid object with valid base address(decoderBaseAddr).
 * returns TSD_HDLC_CH7_DECODER_SUCCESS on success
 */
int setupHdlcCh7Decoder(HdlcCh7DecoderInfo *DecoderInfo)
{
	int ret = 0;
	if(NULL == DecoderInfo) return TSD_HDLC_CH7_DECODER_NULLPTR;

	// base addresses always start on a 32-bit boundary.
	if(DecoderInfo->ch7DecoderBaseAddr & 0x1F){
		return TSD_HDLC_CH7_DECODER_INVALID_BASE;
	}
	if(DecoderInfo->hdlcDecoderBaseAddr & 0x1F){
		return TSD_HDLC_CH7_DECODER_INVALID_BASE;
	}

	switch(DecoderInfo->decoderMode){
            case TSD_HDLC_CH7_DECMODE_HDLC:
                TSD_HDLC_DEC_IOWR_CONTROL(DecoderInfo->hdlcDecoderBaseAddr, 0);
                ret += tsd_hdlc_dec_set_control_bits(DecoderInfo->hdlcDecoderBaseAddr,
                        ( TSD_HDLC_DEC_CTRL_FCS_REMOVE_32 |
                          TSD_HDLC_DEC_CTRL_CLR_SEQ_CNTR |
                          TSD_HDLC_DEC_CTRL_ETH_SHFT_2 |
                          (DecoderInfo->invertClock ? TSD_HDLC_DEC_CTRL_INVERT_CLOCK : 0) ) );
                ret += tsd_hdlc_dec_clear_control_bits(DecoderInfo->hdlcDecoderBaseAddr, (TSD_HDLC_DEC_CTRL_CLR_SEQ_CNTR));
                ret += tsd_hdlc_dec_enable(DecoderInfo->hdlcDecoderBaseAddr);
                break;
            case TSD_HDLC_CH7_DECMODE_CH7:
                ret += tsd_ch7_dec_set_sync_pattern(DecoderInfo->ch7DecoderBaseAddr, 0xFE6B2840, 32);
                TSD_CH7_DEC_IOWR_CONTROL(DecoderInfo->ch7DecoderBaseAddr, 0);
                ret += tsd_ch7_dec_set_control_bits(DecoderInfo->ch7DecoderBaseAddr,
                        ( (DecoderInfo->invertClock ? TSD_CH7_DEC_CTRL_INVERT_CLOCK : 0) |
                          TSD_CH7_DEC_CTRL_REMOVE_FCS_BIT /*|
                                                            TSD_CH7_DEC_CTRL_TSE_SHFT_EN |
                                                            TSD_CH7_DEC_CTRL_TSE_PAD_EN_BIT*/) );

                            ret += tsd_ch7_dec_set_frame_size(DecoderInfo->ch7DecoderBaseAddr, DecoderInfo->ch7FrameSize);
                ret += tsd_ch7_dec_set_port_select(DecoderInfo->ch7DecoderBaseAddr, TSD_CH7_DEC_PORT_SELECT_FIXED_ETH, true);
                ret += tsd_ch7_dec_enable(DecoderInfo->ch7DecoderBaseAddr);
                break;
            default:
                // disabled.
                ret += tsd_hdlc_dec_disable(DecoderInfo->hdlcDecoderBaseAddr);
                ret += tsd_ch7_dec_disable(DecoderInfo->ch7DecoderBaseAddr);
                break;
	}

	return ret ? TSD_HDLC_CH7_DECODER_FAIL : TSD_HDLC_CH7_DECODER_SUCCESS;
}

char* getCh7DecoderStatusString(uint32_t statusWord) {
	static char statusStr[512];
	memset(statusStr, 0, 512);
	int pos = 0;
	pos += sprintf((char*)(statusStr+pos), "CH7 STATUS: ");
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_CH7_DEC_STATUS_ILLEGAL_FRAME_SZ) ? "ILLEGAL FRAME SIZE " : ""));
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_CH7_DEC_STATUS_AVST_ERROR      ) ? "AVST ERROR " : ""));
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_CH7_DEC_STATUS_START_OF_MAJOR  ) ? "START OF MAJOR " : ""));
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_CH7_DEC_STATUS_MISSED_CH7_SYNC ) ? "MISSED SYNC " : ""));
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_CH7_DEC_STATUS_GOLAY_ERROR     ) ? "GOLAY ERROR " : ""));
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_CH7_DEC_STATUS_FRAME_LOCK      ) ? "FRAME LOCK " : ""));
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_CH7_DEC_STATUS_SFID_CAPTURE    ) ? "SFID CAPTURE " : ""));
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_CH7_DEC_STATUS_AVMM_TIMEOUT    ) ? "AVMM TIMEOUT " : ""));

	return statusStr;
}
char* getHdlcDecoderStatusString(uint32_t statusWord) {
	static char statusStr[512];
	memset(statusStr, 0, 512);
	int pos = 0;
	pos += sprintf((char*)(statusStr+pos), "HDLC STATUS: ");
	pos += sprintf((char*)(statusStr+pos), "%s", ((statusWord & TSD_HDLC_DEC_STATUS_AVMM_TIMEOUT) ? "AVMM TIMEOUT " : ""));

	return statusStr;
}

uint32_t getHdlcCh7DecoderStatus(HdlcCh7DecoderInfo *DecoderInfo)
{
	uint32_t status;
	if(NULL == DecoderInfo) return TSD_HDLC_CH7_DECODER_NULLPTR;
		switch(DecoderInfo->decoderMode){
	case TSD_HDLC_CH7_DECMODE_CH7:
		if(tsd_ch7_dec_get_status(DecoderInfo->ch7DecoderBaseAddr, &status)){
			status = 0xffffffff;
		}
		break;
	case TSD_HDLC_CH7_DECMODE_HDLC:
		status = tsd_hdlc_dec_get_status(DecoderInfo->hdlcDecoderBaseAddr);
		break;
	default:
		status = 0xffffffff;
		break;
	}
	return status;
}

/*
 * Gets the HDLC CH7 settings from the Decoder core
 * Note: the Base addresses MUST be valid.
 * returns TSD_HDLC_CH7_DECODER_SUCCESS on success
 */
int getHdlcCh7DecoderSetup(HdlcCh7DecoderInfo *DecoderInfo)
{
	uint32_t ch7CtrlWord, hdlcCtrlWord;
	if(NULL == DecoderInfo) return TSD_HDLC_CH7_DECODER_NULLPTR;

	// base addresses always start on a 32-bit boundary.
	if(DecoderInfo->ch7DecoderBaseAddr & 0x1F){
		return TSD_HDLC_CH7_DECODER_INVALID_BASE;
	}
	if(DecoderInfo->hdlcDecoderBaseAddr & 0x1F){
		return TSD_HDLC_CH7_DECODER_INVALID_BASE;
	}

	hdlcCtrlWord = TSD_HDLC_DEC_IORD_CONTROL(DecoderInfo->hdlcDecoderBaseAddr);
	ch7CtrlWord = TSD_CH7_DEC_IORD_CONTROL(DecoderInfo->ch7DecoderBaseAddr);

	if(ch7CtrlWord & TSD_CH7_DEC_CTRL_ENABLE_BIT) {
		DecoderInfo->decoderMode = TSD_HDLC_CH7_DECMODE_CH7;
		tsd_ch7_dec_get_frame_size(DecoderInfo->ch7DecoderBaseAddr, &DecoderInfo->ch7FrameSize);
		DecoderInfo->fcsIsInserted = true;
	} else if(hdlcCtrlWord & TSD_HDLC_DEC_CTRL_ENABLE_BIT) {
		DecoderInfo->decoderMode = TSD_HDLC_CH7_DECMODE_HDLC;
		DecoderInfo->fcsIsInserted = false;
	} else {
		DecoderInfo->decoderMode = TSD_HDLC_CH7_DECMODE_DISABLED;
	}


	return TSD_HDLC_CH7_DECODER_SUCCESS;
}

/*
 * Gets the fifo underflow error count for the selected decoder
 */
int getHdlcCh7DecoderUnderflowCount(HdlcCh7DecoderInfo *DecoderInfo) {
		uint32_t cnt;
	switch(DecoderInfo->decoderMode){
	case TSD_HDLC_CH7_DECMODE_HDLC:
		if(tsd_hdlc_dec_get_fifo0_underflow_count(DecoderInfo->hdlcDecoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	case TSD_HDLC_CH7_DECMODE_CH7:
		if(tsd_ch7_dec_get_fifo_underflow_count(DecoderInfo->ch7DecoderBaseAddr, &cnt)){
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
 * Gets the fifo overflow error count for the selected decoder
 */
int getHdlcCh7DecoderOverflowCount(HdlcCh7DecoderInfo *DecoderInfo) {
	uint32_t cnt;
	switch(DecoderInfo->decoderMode){
	case TSD_HDLC_CH7_DECMODE_HDLC:
		if(tsd_hdlc_dec_get_fifo0_overflow_count(DecoderInfo->hdlcDecoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	case TSD_HDLC_CH7_DECMODE_CH7:
		if(tsd_ch7_dec_get_fifo_overflow_count(DecoderInfo->ch7DecoderBaseAddr, &cnt)){
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
 * Gets the HDLC bit-stuffing error count from the HDLC/CH7 Decoder Core
 * returns the count from the decoder
 */
int getHdlcDecoderBitStuffingErrorCount(HdlcCh7DecoderInfo *DecoderInfo) {
	uint32_t cnt;
	switch(DecoderInfo->decoderMode){
	case TSD_HDLC_CH7_DECMODE_HDLC:
		if(tsd_hdlc_dec_get_bitstuffing_error_count(DecoderInfo->hdlcDecoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	case TSD_HDLC_CH7_DECMODE_CH7:
		cnt = 0;
		break;
	default:
		cnt = 0;
		break;
	}
	return (int)(cnt & 0x7FFFFFFF);
}

/*
 * Gets the total raw CH10 received frame count from the HDLC/CH7 Decoder Core
 * Note: this count will be valid in CH7 mode only.
 * returns the count from the decoder
 */
int getCh7DecoderCH10FrameCount(HdlcCh7DecoderInfo *DecoderInfo) {
		uint32_t cnt;
	switch(DecoderInfo->decoderMode){
	case TSD_HDLC_CH7_DECMODE_HDLC:
		cnt = 0;
		break;
	case TSD_HDLC_CH7_DECMODE_CH7:
		if(tsd_ch7_dec_get_ch10_send_count(DecoderInfo->ch7DecoderBaseAddr, &cnt)){
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
 * Gets the total Ethernet received frame count from the HDLC/CH7 Decoder Core
 * returns the count from the decoder
 */
int getHdlcCh7DecoderEthFrameCount(HdlcCh7DecoderInfo *DecoderInfo) {
		uint32_t cnt;
	switch(DecoderInfo->decoderMode){
	case TSD_HDLC_CH7_DECMODE_HDLC:
		if(tsd_hdlc_dec_get_packet_count(DecoderInfo->hdlcDecoderBaseAddr, &cnt)){
			cnt = 0;
		}
		break;
	case TSD_HDLC_CH7_DECMODE_CH7:
		if(tsd_ch7_dec_get_tse_send_count(DecoderInfo->ch7DecoderBaseAddr, &cnt)){
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
 * Resets all counters in the HDLC/CH7 Decoder Core
 */
int resetHdlcCh7DecoderCounters(HdlcCh7DecoderInfo *DecoderInfo) {
	tsd_ch7_dec_reset_counters(DecoderInfo->ch7DecoderBaseAddr);
	tsd_hdlc_dec_reset_counters(DecoderInfo->hdlcDecoderBaseAddr);
	return 0;
}
