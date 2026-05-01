/*
 * fw_image_validate.h
 *  Copyright (c) [2006] - [2019] Telspan Data, LLC. All Rights Reserved
 *  Created on: 
 *  Authored by: eric
 *  Description: ...
 * 
 *  Revision History:
 *  	Dec 4, 2019: Initial Creation
 */

/* #include <stdint.h> */
#include "main.h"

/*
 * verifies the firmware image buffer is good and contains no checksum errors.
 * returns 0 on success.
 */
int verify_firmware_image(uint8_t* fwImageBuffer, uint32_t imgBufferSize);

