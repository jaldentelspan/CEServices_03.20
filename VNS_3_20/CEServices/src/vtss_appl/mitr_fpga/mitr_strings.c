/*
 * mitr_strings.c
 *
 *  Created on: Oct 2, 2015
 *      Author: eric
 */

#include "vtss_api.h"
#include "mitr_types.h"

const char * MITR_ERROR_CODE_STR[] = {
	[MITR_ERROR_INVALID_COMMAND] = "Invalid Command",
	[MITR_ERROR_INVALID_PARAMETER] = "Invalid Parameter",
	[MITR_ERROR_INVALID_MODE] = "Invalid Mode",
	[MITR_ERROR_NO_MEDIA] = "No Media",
	[MITR_ERROR_MEDIA_FULL] = "Media Full",
	[MITR_ERROR_COMMAND_FAILED] = "Command Failed",
};

//const char * mitr_status_name[] = {
//	[MITR_STATE_FAIL] 	= "FAIL",
//	[MITR_STATE_IDLE]	= "IDLE",
//	[MITR_STATE_BIT] 	= "BIT",
//	[MITR_STATE_BUSY]	= "BUSY",
//	[MITR_STATE_ERROR]	= "ERROR",
//	// ADD NEW STATE NAMES ABOVE THIS LINE
//	[MITR_STATE_UNKNOWN] = "UNKNOWN"
//};
//
//const char * IEEE_1588_TYPE_STR[] = {
//	[MITR_IEEE_1588_DISABLED] = "DISABLED",
//	[MITR_IEEE_1588_RESERVED1] = "RESERVED",
//	[MITR_IEEE_1588_SLAVE] = "IEEE-1588 SLAVE",
//	[MITR_IEEE_1588_MASTER] = "IEEE-1588 MASTER"
//};
