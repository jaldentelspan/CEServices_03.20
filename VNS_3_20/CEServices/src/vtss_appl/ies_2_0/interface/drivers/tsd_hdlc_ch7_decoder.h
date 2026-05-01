/*
 * tsd_hdlc_ch7_decoder.h
 *
 *  Created on: Jan 6, 2021
 *      Author: eric
 */

#ifndef TSD_HDLC_CH7_DECODER_H_
#define TSD_HDLC_CH7_DECODER_H_

/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */

#define TSD_HDLC_CH7_DECODER_SUCCESS			0
#define TSD_HDLC_CH7_DECODER_FAIL   			1
#define TSD_HDLC_CH7_DECODER_INVALID_BASE		2 /* invalid base address(s) */
#define TSD_HDLC_CH7_DECODER_NULLPTR			4 /* null pointer passed into function */

typedef enum HdlcCh7DecoderMode_t {
	TSD_HDLC_CH7_DECMODE_HDLC,
	TSD_HDLC_CH7_DECMODE_CH7,
	TSD_HDLC_CH7_DECMODE_DISABLED
}HdlcCh7DecoderMode;

typedef struct HdlcCh7DecoderInfo_t {
	uint32_t ch7DecoderBaseAddr;       /* base address of the hdlc/ch7 decoder ip core */
	uint32_t hdlcDecoderBaseAddr;       /* base address of the hdlc/ch7 decoder ip core */
	uint32_t tseBaseAddr;           /* base address of the TSE ip core that is connected to this decoder */
	HdlcCh7DecoderMode decoderMode; /* sets the decoder mode (ch7 or HDLC) */
	int ch7FrameSize;               /* multiplier (233 * ch7FrameSize) bytes is the frame size */
	//bool useOldStreamingHeader;	    /* OBSOLETE: sets the streaming header type for the core false=new, true=legacy (ch7 in ch10 mode only) */
	bool fcsIsInserted;             /* set to true to if the FCS is in the HDLC/CH7 stream */
	bool invertClock;               /* set to true to invert the input clock signal */
}HdlcCh7DecoderInfo;

/*
 * Sets up the HDLC CH7 decoder based on the information in the input structure
 * Note: the input struct MUST point to a valid object with valid base addresses (decoderBaseAddr and tseBaseAddr).
 * returns TSD_HDLC_CH7_DECODER_SUCCESS on success
 */
int setupHdlcCh7Decoder(HdlcCh7DecoderInfo *DecoderInfo);


/*
 * Gets the HDLC CH7 settngs from the Decoder core
 * Note: the Base addresses MUST be valid.
 * returns TSD_HDLC_CH7_DECODER_SUCCESS on success
 */
int getHdlcCh7DecoderSetup(HdlcCh7DecoderInfo *DecoderInfo);

char* getCh7DecoderStatusString(uint32_t statusWord);
char* getHdlcDecoderStatusString(uint32_t statusWord);
/*
 * gets the status word from the ch7 decoder.
 */
uint32_t getHdlcCh7DecoderStatus(HdlcCh7DecoderInfo *DecoderInfo);

/*
 * Gets the fifo underflow error count for the selected decoder
 */
int getHdlcCh7DecoderUnderflowCount(HdlcCh7DecoderInfo *DecoderInfo);

/*
 * Gets the fifo overflow error count for the selected decoder
 */
int getHdlcCh7DecoderOverflowCount(HdlcCh7DecoderInfo *DecoderInfo);

/*
 * Gets the HDLC bit-stuffing error count from the HDLC/CH7 Decoder Core
 * returns the count from the decoder
 */
int getHdlcDecoderBitStuffingErrorCount(HdlcCh7DecoderInfo *DecoderInfo);

/*
 * Gets the total raw CH10 received frame count from the HDLC/CH7 Decoder Core
 * Note: this count will be valid in CH7 mode only.
 * returns the count from the decoder
 */
int getCh7DecoderCH10FrameCount(HdlcCh7DecoderInfo *DecoderInfo);

/*
 * Gets the total Ethernet received frame count from the HDLC/CH7 Decoder Core
 * returns the count from the decoder
 */
int getHdlcCh7DecoderEthFrameCount(HdlcCh7DecoderInfo *DecoderInfo);

/*
 * Resets all counters in the HDLC/CH7 Decoder Core
 */
int resetHdlcCh7DecoderCounters(HdlcCh7DecoderInfo *DecoderInfo);


#endif /* TSD_HDLC_CH7_DECODER_H_ */
