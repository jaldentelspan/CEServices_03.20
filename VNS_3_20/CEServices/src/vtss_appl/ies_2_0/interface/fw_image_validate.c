/*
 * fw_image_validate.c
 *  Copyright (c) [2006] - [2019] Telspan Data, LLC. All Rights Reserved
 *  Created on: 
 *  Authored by: eric
 *  Description: ...
 * 
 *  Revision History:
 *  	Dec 4, 2019: Initial Creation
 */

#include "fw_image_validate.h"

#define HEADER_LENGTH		32

//	These are the header signatures for hardware and software images.
//
#define SW_HDR_SIGNATURE	( 0xAA55A5A5 )
#define HW_HDR_SIGNATURE	( 0x55AA5A5A )

//
//	These are the byte offsets to the fields in the hw/sw images.
//
#define SIGNATURE_OFST		( 0 << 2 )
#define HW_ID_OFST			( 1 << 2 )
#define HW_TS_OFST			( 2 << 2 )
#define VERSION_OFST		( 3 << 2 )
#define IMAGE_LENGTH_OFST	( 4 << 2 )
#define IMAGE_CRC_OFST		( 5 << 2 )
#define RES_WD_TMO_OFST		( 6 << 2 )
#define HEADER_CRC_OFST		( 7 << 2 )

#define POLY32_REF      0xEDB88320
#define REMAINDER_REF   0x2144DF1C

//
//	Image header as a structure.
//
typedef struct image_header_s {
	uint32_t signature;
	uint32_t hw_id;
	uint32_t hw_ts;
	uint32_t version;
	uint32_t image_length;
	uint32_t image_crc;
	uint32_t res_wd_tmo;
	uint32_t header_crc;
	uint32_t calc_header_crc; /* extra, not part of actual header */
	uint32_t is_valid;		/* extra, not part of actual header */
} __attribute__((packed))image_header_t;

//
//	This is the crc32 algorithm it is called by the utility functions below
//
static uint32_t crc32_reflected( uint32_t crcval, uint8_t cval )
{
    int i;

    crcval ^= cval;
    for ( i = 8 ; i-- ; )
        crcval = crcval & 0x00000001 ?
                    ( crcval >> 1 ) ^ POLY32_REF :
                    ( crcval >> 1 );
    return crcval;
}

//
//	This routine assumes that a new crc is begin created so it initializes the
//	state and returns the crc value thru this initial length of the data.
//
uint32_t begin_compute_crc32( uint8_t *buf_ptr, uint32_t length ) {

    uint32_t initial = 0xFFFFFFFF;
    uint32_t crc;

    crc = initial;

    while( length > 0 ) {
        crc = crc32_reflected( crc, *buf_ptr++ );
        length--;
    }
    return( crc );
}

//
//	This routine assumes that it is continuing a previously initiated crc
//	calculation so it inherits the state of crc and continues to add to it thru
//	this interim length of the data.  It returns the interim value of crc.
//
uint32_t interim_compute_crc32( uint8_t *buf_ptr, uint32_t length, uint32_t crc ) {

    while( length > 0 ) {
        crc = crc32_reflected( crc, *buf_ptr++ );
        length--;
    }
    return( crc );
}

//
//	This routine assumes that it is continuing a previously intitiated crc
//	calculation so it inherits the state of crc and continues to add to it thru
//	this final length of data, and then it finalizes the result and returns the
//	final value of the crc.
//
uint32_t final_compute_crc32( uint8_t *buf_ptr, uint32_t length, uint32_t crc ) {

    uint32_t xor_out = 0xFFFFFFFF;

    while( length > 0 ) {
        crc = crc32_reflected( crc, *buf_ptr++ );
        length--;
    }
    return( crc ^ xor_out );
}

void init_header(image_header_t * hdr)
{
	hdr->header_crc = 0;
	hdr->hw_id = 0;
	hdr->hw_ts = 0;
	hdr->image_crc = 0;
	hdr->image_length = 0;
	hdr->res_wd_tmo = 0;
	hdr->signature = 0;
	hdr->version = 0;
	hdr->calc_header_crc = 0;
	hdr->is_valid = 0;
}

uint32_t read_uint32_t_from_byte_ptr(uint8_t * byte_ptr)
{
	return(
			( (uint32_t)(byte_ptr[3]) <<  0 ) |
			( (uint32_t)(byte_ptr[2]) <<  8 ) |
			( (uint32_t)(byte_ptr[1]) << 16 ) |
			( (uint32_t)(byte_ptr[0]) << 24 )
		);
}

void get_image_header_from_buffer(image_header_t * hdr, uint8_t * read_buf)
{

	init_header(hdr);
	hdr->is_valid = 1;
	// validate the header signature
	hdr->signature = read_uint32_t_from_byte_ptr(&read_buf[SIGNATURE_OFST]);

	if( hdr->signature != HW_HDR_SIGNATURE )
	{
		//do not continue
		hdr->is_valid = 0;
		//return;
	}

	// validate the header crc
	hdr->header_crc = read_uint32_t_from_byte_ptr( &read_buf[HEADER_CRC_OFST] );
	hdr->calc_header_crc = begin_compute_crc32(
		read_buf,	//	u8 *buf_ptr,
		28			//	u32 length
		);
	hdr->calc_header_crc = final_compute_crc32(
		0,						//	u8 *buf_ptr,
		0,						//	u32 length,
		hdr->calc_header_crc	//	u32 crc
		);
	if( hdr->header_crc != hdr->calc_header_crc )
	{
		hdr->is_valid = 0;
	}

	hdr->hw_id = read_uint32_t_from_byte_ptr( &read_buf[HW_ID_OFST] );
	hdr->hw_ts = read_uint32_t_from_byte_ptr( &read_buf[HW_TS_OFST] );
	hdr->version = read_uint32_t_from_byte_ptr( &read_buf[VERSION_OFST] );
	hdr->image_length = read_uint32_t_from_byte_ptr( &read_buf[IMAGE_LENGTH_OFST] );
	hdr->image_crc = read_uint32_t_from_byte_ptr( &read_buf[IMAGE_CRC_OFST] );
	hdr->res_wd_tmo = read_uint32_t_from_byte_ptr( &read_buf[RES_WD_TMO_OFST] );
}

/*
 * verifies the firmware image buffer is good and contains no checksum errors.
 * returns 0 on success.
 */
int verify_firmware_image(uint8_t* fwImageBuffer, uint32_t imgBufferSize)
{
	int retval = 0;
	uint32_t calc_crc;
	image_header_t hdr;

	get_image_header_from_buffer(&hdr, fwImageBuffer);
	if(hdr.is_valid)
	{
		if((hdr.image_length + HEADER_LENGTH) < imgBufferSize)
			return 2;

		//calculate the checksum and compare
		calc_crc = begin_compute_crc32(
				&fwImageBuffer[HEADER_LENGTH],	//	uint8_t *buf_ptr,
				hdr.image_length				//	uint32_t length
			);

		calc_crc = final_compute_crc32(
			0,						//	uint8_t *buf_ptr,
			0,						//	uint32_t length,
			calc_crc				//	uint32_t crc
			);

		if( calc_crc != hdr.image_crc )
		{
			retval = 3;
		}
		else
		{
			retval = 0;
		}
	}
	else
	{
		retval = 1;
	}

	return retval;
}
