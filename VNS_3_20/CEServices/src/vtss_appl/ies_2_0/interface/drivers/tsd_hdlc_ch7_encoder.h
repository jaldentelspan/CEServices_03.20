/*
 * tsd_hdlc_ch7_encoder.h
 *
 *  Created on: Jan 6, 2021
 *      Author: eric
 */

#ifndef TSD_HDLC_CH7_ENCODER_H_
#define TSD_HDLC_CH7_ENCODER_H_

/* #include <stdint.h> */
#include "main.h"
/* #include <stdbool.h> */

#define TSD_HDLC_CH7_ENCODER_SUCCESS			0
#define TSD_HDLC_CH7_ENCODER_INVALID_RATE		1 /* invalid bitrate */
#define TSD_HDLC_CH7_ENCODER_INVALID_BASE		2 /* invalid base address(s) */
#define TSD_HDLC_CH7_ENCODER_BR_SETUP_FAIL  	3 /* bitrate setup failure */
#define TSD_HDLC_CH7_ENCODER_NULLPTR			4 /* null pointer passed into function */
#define TSD_HDLC_CH7_ENCODER_INVALID_REF_FREQ	5 /* invalid reference frequency for pll */
#define TSD_HDLC_CH7_ENCODER_SETUP_FAIL     	6 /* some other setup fail occurred */

typedef enum HdlcCh7EncoderMode_t {
	TSD_HDLC_CH7_ENCMODE_HDLC,
	TSD_HDLC_CH7_ENCMODE_CH7,
	TSD_HDLC_CH7_ENCMODE_DISABLED
}HdlcCh7EncoderMode;

typedef struct HdlcCh7EncoderInfo_t {
	uint32_t ch7EncoderBaseAddr;    /* base address of the ch7 encoder ip core */
	uint32_t hdlcEncoderBaseAddr;   /* base address of the hdlc encoder ip core */
	//uint32_t pllReconfigBaseAddr;   /* OBSOLETE: base address of the pll reconfig block that controls the bitrate */
	HdlcCh7EncoderMode encoderMode; /* sets the encoder mode (ch7 or HDLC) */
	//ReconfigPLLInfo pllPrivInfo;    /* OBSOLETE: stores pll information for back calculation of bitrate (treat as read-only) */
	//int pllReferenceFreq;           /* OBSOLETE: the reference (input) frequency of the pll that drives the bitrate. */
	int bitrate;                    /* encoder output bitrate, defined in bits/second (or Hz) */
	int sysClkFrequency;            /* frequency of the system clock in Hz (typically 100000000) */
	int invertClock; 				/* invert the tx pcm clock (phase 180) */
	int ch7FrameSize;               /* multiplier (233 * ch7FrameSize) bytes is the frame size */
	bool hdlcAddrCtrlEnable;        /* set to true to include the HDLC ADDR and CTRL fields */
	bool fcsInsertion;              /* set to true to include the FCS in the HDLC/CH7 stream */
	//bool useOldStreamingHeader;	    /* OBSOLETE: sets the streaming header type for the core false=new, true=legacy (ch7 in ch10 mode only) */
	uint8_t hdlcAddrByte;           /* addr byte value to use if hdlcAddrCtrlEnable is true */
	uint8_t hdlcCtrlByte;           /* ctrl byte value to use if hdlcAddrCtrlEnable is true */
	uint8_t reservedByte1;			/* reserved for structure alignment */
	uint8_t reservedByte2;			/* reserved for structure alignment */
}HdlcCh7EncoderInfo;


/*
 * Sets up the HDLC CH7 encoder based on the information in the input structure
 * Note: the input struct MUST point to a valid object with valid base addresses (encoderBaseAddr and pllReconfigBaseAddr).
 * During configuration, pllPrivInfo is filled in with actual values used.
 * returns TSD_HDLC_CH7_ENCODER_SUCCESS on success
 */
int setupHdlcCh7Encoder(HdlcCh7EncoderInfo *EncoderInfo);


/*
 * Gets the HDLC CH7 settngs from the Encoder core
 * Note: the Base addresses MUST be valid.
 *       AND if setupHdlcCh7Encoder has not been called using the same EncoderInfo object,
 *           the bitrate value may be incorrect upon return.
 */
int getHdlcCh7EncoderSetup(HdlcCh7EncoderInfo *EncoderInfo);

/*
 * Gets the fifo underflow count from the HDLC/CH7 Encoder Core
 * returns the count from the encoder
 */
int getHdlcCh7EncoderUnderflowCount(HdlcCh7EncoderInfo *EncoderInfo);

/*
 * Gets the fifo overflow count from the HDLC/CH7 Encoder Core
 * returns the count from the encoder
 */
int getHdlcCh7EncoderOverflowCount(HdlcCh7EncoderInfo *EncoderInfo);

/*
 * Gets the total ethernet frame count from the HDLC/CH7 Encoder Core
 * 16-bit counter... it will roll over at 65535
 * returns the count from the encoder
 */
int getHdlcCh7EncoderEthFrameCount(HdlcCh7EncoderInfo *EncoderInfo);

/*
 * Gets the total raw CH10 frame count from the HDLC/CH7 Encoder Core
 * -> ONLY VALID IN CH7 MODE
 * 16-bit counter... it will roll over at 65535
 * returns the count from the encoder
 */
int getHdlcCh7EncoderCH10FrameCount(HdlcCh7EncoderInfo *EncoderInfo);

/*
 * Resets all counters in the HDLC/CH7 Encoder Core
 */
int resetHdlcCh7EncoderCounters(HdlcCh7EncoderInfo *EncoderInfo);

#endif /* TSD_HDLC_CH7_ENCODER_H_ */
